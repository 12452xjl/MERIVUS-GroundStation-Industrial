import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Values and explicit Root calls only. Opening this view never queries a camera.
FocusScope {
    id: root
    property var controller: null
    property var dialog: null
    property var life: ({alive: true, epoch: 0, exhausted: false, held: null, pending: null, view: null, opening: false,
        controlEpoch: 0, controlExhausted: false, controlHeld: null, controlPending: null})
    property var receipt: ({})
    property var controlReceipt: ({})
    property string controlTarget: ""
    property string controlMessage: ""
    property bool callBusy: false
    property string vehicleText: "1"
    property string componentText: "100"
    property string streamText: "1"
    property string message: ""
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var snapshot: controller ? controller.fleetCameraStreamCaptureSnapshot || {} : ({})
    readonly property var workflow: snapshot.workflow || ({})
    readonly property var identity: snapshot.operatorIdentity || ({})
    readonly property var controlSnapshot: controller ? controller.fleetCameraStreamControlSnapshot || {} : ({})
    readonly property var controlWorkflow: controlSnapshot.workflow || ({})
    readonly property string controlContext: controlBase()
    readonly property var targetRunning: controlTarget === "start" ? true : controlTarget === "stop" ? false : null
    readonly property int vehicleId: target(vehicleText, 1, 255)
    readonly property int componentId: target(componentText, 100, 105)
    readonly property int streamId: target(streamText, 1, 255)
    readonly property string targetError: vehicleId === 0 ? qsTr("目标机号无效：请输入1–255的整数，不接受空值、前导零或0／all。")
        : componentId === 0 ? qsTr("相机component无效：请输入100–105，不是autopilot component。")
        : streamId === 0 ? qsTr("视频流ID无效：请输入1–255的整数，不接受0／all。") : ""
    readonly property string stableContext: context()
    readonly property bool own: uuid(receipt.requestId) && receipt.owner === controller && receipt.base === stableContext
        && receipt.vehicleId === vehicleId && receipt.componentId === componentId && receipt.streamId === streamId
        && known(workflow) && snapshot.workflowScopeMatches === true && workflow.requestId === receipt.requestId
        && workflow.generation === receipt.generation && workflow.scopeToken === receipt.scopeToken
        && workflow.vehicleId === vehicleId && workflow.componentId === componentId && workflow.streamId === streamId
    readonly property bool canCapture: enabled && visible && !!dialog && !dialog.consentExhausted && !!stableContext
        && !callBusy && !life.exhausted && snapshot.busy === false && workflow.active === false && controlWorkflow.active !== true
        && vehicleId > 0 && componentId > 0 && streamId > 0 && identity.authenticated === true && roleAllowed()
        && typeof controller.requestFleetCameraStreamCapture === "function" && typeof controller.cancelFleetCameraStreamCapture === "function"
    readonly property bool controlOwn: uuid(controlReceipt.reviewId) && controlReceipt.owner === controller && controlReceipt.base === controlContext
        && controlReceipt.vehicleId === vehicleId && controlReceipt.componentId === componentId && controlReceipt.streamId === streamId
        && controlReceipt.targetRunning === targetRunning && controlKnown(controlWorkflow) && controlSnapshot.workflowScopeMatches === true
        && controlWorkflow.reviewId === controlReceipt.reviewId && controlWorkflow.generation === controlReceipt.generation
        && controlWorkflow.scopeToken === controlReceipt.scopeToken && controlWorkflow.bindingHash === controlReceipt.bindingHash
        && controlWorkflow.captureId === controlReceipt.captureId && controlWorkflow.targetRunning === targetRunning
        && controlWorkflow.vehicleId === controlReceipt.vehicleId && controlWorkflow.componentId === controlReceipt.componentId && controlWorkflow.streamId === controlReceipt.streamId
    readonly property bool canReviewControl: enabled && visible && !!dialog && !!controlContext && !life.controlExhausted
        && !dialog.controlConsentExhausted && !callBusy && controlSnapshot.busy === false && controlWorkflow.active === false
        && own && workflow.state === "observed" && workflow.active === false && typeof targetRunning === "boolean"
        && controlWorkflow.captureId !== receipt.requestId && controlRoleAllowed()
        && typeof controller.requestFleetCameraStreamControl === "function" && typeof controller.cancelFleetCameraStreamControl === "function"
    readonly property bool canApproveControl: enabled && visible && !!dialog && !!controlContext && !life.controlExhausted
        && !dialog.controlConsentExhausted && !callBusy && controlOwn && controlWorkflow.active === true && controlWorkflow.state === "review"
        && controlWorkflow.remainingMs > 0 && controlRoleAllowed() && typeof controller.approveFleetCameraStreamControl === "function"
    implicitHeight: entry.implicitHeight
    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    QGCPalette { id: palette; colorGroupEnabled: root.enabled }

    function map(value) { return value && typeof value === "object" && !Array.isArray(value) }
    function keys(value, names) {
        if (!map(value) || Object.keys(value).length !== names.length) return false
        for (var i = 0; i < names.length; i++) if (!Object.prototype.hasOwnProperty.call(value, names[i])) return false
        return true
    }
    function integer(value, minimum, maximum) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= minimum && value <= maximum }
    function target(text, minimum, maximum) {
        return typeof text === "string" && /^[1-9][0-9]{0,2}$/.test(text) && Number(text) >= minimum && Number(text) <= maximum ? Number(text) : 0
    }
    function counter(value) {
        return typeof value === "string" && /^(0|[1-9][0-9]{0,19})$/.test(value)
            && (value.length < 20 || value <= "18446744073709551615")
    }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function queryKnown(value, messageId) {
        return keys(value, ["requestId", "messageId", "requestIssued", "ackObserved", "reportObserved", "ackResult", "baselineReceiptSequence", "receiptSequence", "contentRevision", "remainingMs", "state", "reason"])
            && typeof value.requestId === "string" && (!value.requestId || uuid(value.requestId)) && value.messageId === messageId
            && typeof value.requestIssued === "boolean" && typeof value.ackObserved === "boolean" && typeof value.reportObserved === "boolean"
            && integer(value.ackResult, -1, 255) && (value.ackObserved ? value.ackResult >= 0 : value.ackResult === -1)
            && counter(value.baselineReceiptSequence) && counter(value.receiptSequence) && counter(value.contentRevision)
            && integer(value.remainingMs, 0, 4000) && typeof value.reason === "string"
            && ["ready", "waiting-ack-and-report", "waiting-report", "complete", "failed", "cancelled"].indexOf(value.state) >= 0
    }
    function observationKnown(value, vehicle, component, stream) {
        if (!keys(value, ["schemaVersion", "reportType", "requestId", "vehicleId", "componentId", "streamId", "active", "state", "reason", "sourceCurrent", "remainingMs", "stage", "informationQuery", "statusQuery", "information", "status", "sourceEpoch", "transportEpoch", "versionEpoch", "receiptSequence", "snapshotOnly", "authenticated", "wireCausalityProven", "catalogComplete", "remoteRunningVerified", "remoteStoppedVerified", "availableBandwidthVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"])
                || value.schemaVersion !== 1 || value.reportType !== "fleet-camera-stream-capture" || !uuid(value.requestId)
                || value.vehicleId !== vehicle || value.componentId !== component || value.streamId !== stream
                || typeof value.active !== "boolean" || typeof value.sourceCurrent !== "boolean" || typeof value.reason !== "string"
                || !integer(value.remainingMs, 0, 8000) || !integer(value.stage, 0, 2) || !counter(value.receiptSequence)
                || !map(value.information) || Object.keys(value.information).length !== 15 || !map(value.status) || Object.keys(value.status).length !== 15
                || value.snapshotOnly !== true || ["ready", "waiting-info", "waiting-status", "complete", "failed", "cancelled"].indexOf(value.state) < 0) return false
        for (var i = 0, names = ["authenticated", "wireCausalityProven", "catalogComplete", "remoteRunningVerified", "remoteStoppedVerified", "availableBandwidthVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"]; i < names.length; i++)
            if (value[names[i]] !== false) return false
        return queryKnown(value.informationQuery, 269) && queryKnown(value.statusQuery, 270)
    }
    function known(value) {
        return keys(value, ["schemaVersion", "requestId", "generation", "scopeToken", "vehicleId", "componentId", "streamId", "state", "active", "reason", "remainingMs", "observation", "snapshotOnly", "authenticated", "configurationPermitted", "executionPermit", "flightCommandReleased", "protocol", "maximumAgeMs", "requestAccepted"])
            && value.schemaVersion === 1 && typeof value.requestId === "string" && (!value.requestId || uuid(value.requestId))
            && counter(value.generation) && typeof value.scopeToken === "string" && integer(value.vehicleId, 0, 255)
            && (value.componentId === 0 || integer(value.componentId, 100, 105)) && integer(value.streamId, 0, 255)
            && typeof value.active === "boolean" && typeof value.reason === "string" && integer(value.remainingMs, 0, 8000)
            && ["empty", "preparing", "capturing", "observed", "failed", "stale", "cancelled"].indexOf(value.state) >= 0
            && value.active === (["preparing", "capturing"].indexOf(value.state) >= 0)
            && value.protocol === "video-stream-information-status" && value.maximumAgeMs === 3000
            && value.snapshotOnly === true && value.authenticated === false && value.configurationPermitted === false
            && value.executionPermit === false && value.flightCommandReleased === false && value.requestAccepted === !!value.requestId
            && map(value.observation) && (Object.keys(value.observation).length === 0 || observationKnown(value.observation, value.vehicleId, value.componentId, value.streamId))
    }
    function context() {
        if (!controller || snapshot.sourceKnown !== true || snapshot.uiWiringPresent !== true || snapshot.runtimeValidated !== false || !known(workflow)) return ""
        try {
            var parsed = JSON.parse(snapshot.sourceToken)
            if (!map(parsed) || typeof parsed.scope !== "string" || !parsed.scope || !map(parsed.policy)
                    || Object.keys(parsed.policy).length !== 9 || parsed.policy.action !== "fleet.video.stream.capture"
                    || (parsed.identity !== null && (!map(parsed.identity) || Object.keys(parsed.identity).length !== 5))) return ""
            return JSON.stringify({scope: parsed.scope, identity: parsed.identity, policy: parsed.policy})
        } catch (error) { return "" }
    }
    function roleAllowed() {
        try { var value = JSON.parse(stableContext); return value.policy.allowed === true && value.policy.requiresSecondApproval === false }
        catch (error) { return false }
    }
    function pressed() {
        return {owner: controller, epoch: life.epoch, base: stableContext, view: life.view, dialog: dialog,
            token: snapshot.sourceToken, vehicleId: vehicleId, componentId: componentId, streamId: streamId, held: life.held,
            consentEpoch: dialog ? dialog.consentEpoch : -1, consent: !!dialog && dialog.consentChecked === true}
    }
    function pressCurrent(value) {
        return value && life.alive && !life.exhausted && value.owner === controller && value.epoch === life.epoch
            && value.base === stableContext && value.view && value.view === life.view && value.dialog === dialog
            && value.vehicleId === vehicleId && value.componentId === componentId && value.streamId === streamId
            && !!dialog && visible && enabled && !dialog.consentExhausted && value.consentEpoch === dialog.consentEpoch
    }
    function retire() {
        var guard = life, held = guard ? guard.held : null, actualView = dialog
        if (!guard || !guard.alive) return
        if (guard.pending) guard.pending.cancelState.cancelled = true
        guard.held = null
        if (guard.epoch >= 9007199254740991) guard.exhausted = true
        else guard.epoch += 1
        var retiredEpoch = guard.epoch, empty = {}
        try {
            retireControl()
            if (!guard.alive || guard.epoch !== retiredEpoch || guard.held) return
            receipt = empty
            if (guard.alive && guard.epoch === retiredEpoch && !guard.held && receipt === empty && actualView && dialog === actualView) actualView.clearConfirmation()
        } finally {
            if (held && held.owner) try { held.owner.cancelFleetCameraStreamCapture(held.requestId) } catch (error) {}
        }
    }
    function authenticate(value) {
        var guard = life
        if (!pressCurrent(value) || callBusy || guard.pending || guard.controlPending || workflow.active === true || controlWorkflow.active === true || typeof controller.authenticateFleetOperator !== "function") return
        var owner = controller, base = stableContext, epoch = guard.epoch, ticket = {cancelState: {cancelled: false}}
        guard.pending = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.pending !== ticket || stableContext !== base) return
            owner.authenticateFleetOperator()
        } finally {
            if (guard.alive) { callBusy = false; if (guard.alive && guard.pending === ticket) guard.pending = null }
        }
    }
    function capture(value, confirmed) {
        var guard = life
        if (!canCapture || guard.pending || guard.controlPending || confirmed !== true || !pressCurrent(value) || value.consent !== true || value.token !== snapshot.sourceToken) return
        var owner = controller, base = stableContext, viewToken = guard.view, oldEpoch = guard.epoch, actualView = dialog
        var vehicle = vehicleId, component = componentId, stream = streamId, submittedToken = value.token
        retire()
        var afterClear = function() {
            return guard.alive && !guard.exhausted && guard.epoch === oldEpoch + 1 && owner === controller && stableContext === base
                && guard.view === viewToken && dialog === actualView && !!actualView && visible && enabled
                && vehicle === vehicleId && component === componentId && stream === streamId
                && !actualView.consentExhausted && actualView.consentEpoch === value.consentEpoch + 1 && actualView.consentChecked === false
        }
        if (!afterClear() || submittedToken !== snapshot.sourceToken) return
        var epoch = guard.epoch, ticket = {cancelState: {cancelled: false}}, result = {}
        var abandon = function() {
            try {
                var id = result ? result.requestId : undefined
                if (owner && typeof id === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(id)
                        && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(id)) owner.cancelFleetCameraStreamCapture(id)
            } catch (error) {} // Only frozen locals remain valid after this Panel dies.
        }
        guard.pending = ticket
        try {
            callBusy = true
            if (!afterClear() || guard.pending !== ticket || ticket.cancelState.cancelled || submittedToken !== snapshot.sourceToken) return
            message = ""
            if (!afterClear() || guard.pending !== ticket || ticket.cancelState.cancelled || submittedToken !== snapshot.sourceToken) return
            try { result = owner.requestFleetCameraStreamCapture(submittedToken, vehicle, component, stream, true, ticket.cancelState) || {} }
            catch (error) { result = {reason: "camera-stream-capture-call-unavailable"} }
            if (!guard.alive) { abandon(); return }
            if (!afterClear() || guard.pending !== ticket || ticket.cancelState.cancelled) { abandon(); return }
            var current = workflow
            if (!keys(result, ["ok", "requestAccepted", "requestId", "reason", "flightCommandReleased"])
                    || result.ok !== true || result.requestAccepted !== true || !uuid(result.requestId) || result.flightCommandReleased !== false
                    || !known(current) || current.requestId !== result.requestId || current.active !== true
                    || current.vehicleId !== vehicle || current.componentId !== component || current.streamId !== stream || snapshot.workflowScopeMatches !== true) {
                abandon()
                if (afterClear() && guard.pending === ticket) message = qsTr("未建立采集：%1。不会自动重试。").arg(result.reason || qsTr("未知"))
                return
            }
            var held = {owner: owner, base: base, requestId: result.requestId, generation: current.generation,
                scopeToken: current.scopeToken, vehicleId: vehicle, componentId: component, streamId: stream}
            guard.held = held
            receipt = held
            if (afterClear() && guard.pending === ticket && guard.held === held && receipt === held)
                message = qsTr("原生读取已接纳；Info→Status各至多一次，总期不超过最初8秒。ACK与实际报告分别显示。")
        } finally {
            if (guard.alive) { callBusy = false; if (guard.alive && guard.pending === ticket) guard.pending = null }
        }
    }
    function cancelOwned(value) {
        var guard = life
        if (!pressCurrent(value) || callBusy || guard.pending || !own || !value.held || value.held !== guard.held || value.held !== receipt) return
        var oldEpoch = guard.epoch, owner = controller, base = stableContext, actualView = dialog
        retire()
        if (guard.alive && guard.epoch === oldEpoch + 1 && owner === controller && stableContext === base && actualView === dialog)
            message = qsTr("已请求取消本页后续读取；已可能发出的字节和收到的ACK／报告不会撤回。")
    }
    function controlKnown(value) {
        if (!keys(value, ["schemaVersion", "reviewId", "generation", "scopeToken", "captureId", "vehicleId", "componentId", "streamId", "operation", "targetRunning",
                "state", "reason", "active", "remainingMs", "bindingHash", "basisSummary", "run", "authenticationConsumed", "requestsMayRemainInflight", "snapshotOnly",
                "revalidationRequired", "authenticated", "wireCausalityProven", "remoteRunningVerified", "remoteStoppedVerified", "availableBandwidthVerified",
                "configurationPermitted", "executionPermit", "flightCommandReleased", "permissionsRestored"])
                || value.schemaVersion !== 1 || typeof value.reviewId !== "string" || (value.reviewId && !uuid(value.reviewId))
                || !counter(value.generation) || typeof value.scopeToken !== "string" || typeof value.captureId !== "string" || (value.captureId && !uuid(value.captureId))
                || !integer(value.vehicleId, 0, 255) || (value.componentId !== 0 && !integer(value.componentId, 100, 105)) || !integer(value.streamId, 0, 255)
                || typeof value.targetRunning !== "boolean" || ["", "start", "stop"].indexOf(value.operation) < 0 || typeof value.reason !== "string"
                || ["empty", "preparing", "review", "approving", "applying", "reported-target-match", "failed", "stale", "cancelled"].indexOf(value.state) < 0
                || typeof value.active !== "boolean" || value.active !== (["preparing", "review", "approving", "applying"].indexOf(value.state) >= 0)
                || !integer(value.remainingMs, 0, 30000) || typeof value.bindingHash !== "string" || (value.bindingHash && !/^[0-9a-f]{64}$/.test(value.bindingHash))
                || typeof value.authenticationConsumed !== "boolean" || typeof value.requestsMayRemainInflight !== "boolean"
                || value.snapshotOnly !== true || value.revalidationRequired !== true || !map(value.basisSummary) || !map(value.run)) return false
        for (var i = 0, flags = ["authenticated", "wireCausalityProven", "remoteRunningVerified", "remoteStoppedVerified", "availableBandwidthVerified",
                "configurationPermitted", "executionPermit", "flightCommandReleased", "permissionsRestored"]; i < flags.length; i++) if (value[flags[i]] !== false) return false
        return true // Full native 20/34/8/12/15 values were closed-validated by the actual Root projection.
    }
    function controlBase() {
        if (!controller || controlSnapshot.sourceKnown !== true || controlSnapshot.uiWiringPresent !== true || controlSnapshot.runtimeValidated !== false || !controlKnown(controlWorkflow)) return ""
        try {
            var parsed = JSON.parse(controlSnapshot.sourceToken)
            if (!map(parsed) || typeof parsed.scope !== "string" || !parsed.scope || !map(parsed.policy) || Object.keys(parsed.policy).length !== 9
                    || parsed.policy.action !== "fleet.video.stream.control" || (parsed.identity !== null && (!map(parsed.identity) || Object.keys(parsed.identity).length !== 5))) return ""
            return JSON.stringify({scope: parsed.scope, identity: parsed.identity, policy: parsed.policy})
        } catch (error) { return "" }
    }
    function controlRoleAllowed() {
        try { var parsed = JSON.parse(controlContext); return !!parsed.identity && parsed.policy.allowed === true && parsed.policy.requiresSecondApproval === true }
        catch (error) { return false }
    }
    function controlPressed() {
        return {owner: controller, epoch: life.controlEpoch, base: controlContext, view: life.view, dialog: dialog, token: controlSnapshot.sourceToken,
            vehicleId: vehicleId, componentId: componentId, streamId: streamId, targetRunning: targetRunning, captureHeld: life.held, captureReceipt: receipt,
            captureId: receipt.requestId, held: life.controlHeld, reviewEpoch: dialog ? dialog.controlReviewEpoch : -1,
            applyEpoch: dialog ? dialog.controlApplyEpoch : -1, reviewConfirmed: !!dialog && dialog.controlReviewChecked === true,
            applyConfirmed: !!dialog && dialog.controlApplyChecked === true}
    }
    function controlPressCurrent(value) {
        return value && life.alive && !life.controlExhausted && value.owner === controller && value.epoch === life.controlEpoch && value.base === controlContext
            && value.view && value.view === life.view && value.dialog === dialog && !!dialog && visible && enabled && !dialog.controlConsentExhausted
            && value.vehicleId === vehicleId && value.componentId === componentId && value.streamId === streamId && value.targetRunning === targetRunning
            && value.reviewEpoch === dialog.controlReviewEpoch && value.applyEpoch === dialog.controlApplyEpoch
    }
    function retireControl(preservedTicket) {
        var guard = life, held = guard ? guard.controlHeld : null, actualView = dialog
        if (!guard || !guard.alive) return
        if (guard.controlPending && guard.controlPending !== preservedTicket) guard.controlPending.cancelState.cancelled = true
        if (held && held.cancelState) held.cancelState.cancelled = true
        guard.controlHeld = null
        if (guard.controlEpoch >= 9007199254740991) guard.controlExhausted = true
        else guard.controlEpoch += 1
        var retiredEpoch = guard.controlEpoch, empty = {}
        try {
            controlReceipt = empty
            if (guard.alive && guard.controlEpoch === retiredEpoch && !guard.controlHeld && controlReceipt === empty && actualView && dialog === actualView) actualView.clearControlConfirmations()
        } finally {
            if (held && held.owner) try { held.owner.cancelFleetCameraStreamControl(held.reviewId) } catch (error) {}
        }
    }
    function requestControl(value, confirmed) {
        var guard = life
        if (!canReviewControl || guard.pending || guard.controlPending || confirmed !== true || !controlPressCurrent(value) || value.reviewConfirmed !== true
                || value.token !== controlSnapshot.sourceToken || value.captureHeld !== guard.held || value.captureReceipt !== receipt || !own) return
        var owner = controller, base = controlContext, actualView = dialog, viewToken = guard.view, epoch = guard.controlEpoch
        var submittedToken = value.token, captureId = value.captureId, vehicle = vehicleId, component = componentId, stream = streamId, running = targetRunning
        var ticket = {cancelState: {cancelled: false}, resetting: true}, result = {}
        guard.controlPending = ticket
        var current = function() { return guard.alive && !guard.controlExhausted && guard.controlEpoch === epoch + 1 && guard.controlPending === ticket
            && !ticket.cancelState.cancelled && owner === controller && base === controlContext && guard.view === viewToken && dialog === actualView && !!actualView
            && visible && enabled && vehicle === vehicleId && component === componentId && stream === streamId && running === targetRunning
            && !actualView.controlConsentExhausted && actualView.controlReviewEpoch === value.reviewEpoch + 1 && !actualView.controlReviewChecked
            && actualView.controlApplyEpoch === value.applyEpoch + (value.applyConfirmed ? 1 : 0) && !actualView.controlApplyChecked }
        var abandon = function() {
            try {
                var id = result ? result.reviewId : undefined
                if (owner && typeof id === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(id)
                        && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(id)) owner.cancelFleetCameraStreamControl(id)
            } catch (error) {} // No method lookup on the destroyed Panel.
        }
        try {
            retireControl(ticket) // Do not retire the READ receipt supplying the actual opaque basis.
            ticket.resetting = false
            if (!current() || submittedToken !== controlSnapshot.sourceToken) return
            callBusy = true
            if (!current() || submittedToken !== controlSnapshot.sourceToken) return
            controlMessage = ""
            if (!current() || submittedToken !== controlSnapshot.sourceToken) return
            try { result = owner.requestFleetCameraStreamControl(submittedToken, captureId, running, true, ticket.cancelState) || {} }
            catch (error) { result = {reason: "camera-control-review-call-unavailable"} }
            if (!guard.alive) { abandon(); return }
            if (!current()) { abandon(); return }
            var workflowValue = controlWorkflow
            if (!keys(result, ["ok", "requestAccepted", "reviewId", "reason", "flightCommandReleased"]) || result.ok !== true || result.requestAccepted !== true
                    || !uuid(result.reviewId) || result.flightCommandReleased !== false || !controlKnown(workflowValue) || workflowValue.state !== "review"
                    || workflowValue.active !== true || workflowValue.reviewId !== result.reviewId || !/^[0-9a-f]{64}$/.test(workflowValue.bindingHash)
                    || workflowValue.captureId !== captureId || workflowValue.vehicleId !== vehicle || workflowValue.componentId !== component
                    || workflowValue.streamId !== stream || workflowValue.targetRunning !== running || controlSnapshot.workflowScopeMatches !== true) {
                abandon()
                if (current()) controlMessage = qsTr("未建立控制审阅：%1。旧基线不能自动重用，请重新明确读取后再审阅。").arg(result.reason || qsTr("未知"))
                return
            }
            var held = {owner: owner, base: base, reviewId: result.reviewId, bindingHash: workflowValue.bindingHash, generation: workflowValue.generation,
                scopeToken: workflowValue.scopeToken, captureId: captureId, vehicleId: vehicle, componentId: component, streamId: stream, targetRunning: running, cancelState: ticket.cancelState}
            guard.controlHeld = held
            controlReceipt = held
            if (current() && guard.controlHeld === held && controlReceipt === held)
                controlMessage = qsTr("仅建立审阅，尚未发送启停命令。请核对目标和动作，再另行确认并由不同账号批准；最初30秒期限不会因审批续期。")
        } finally {
            ticket.resetting = false
            if (guard.alive && guard.controlPending === ticket) { callBusy = false; if (guard.alive && guard.controlPending === ticket) guard.controlPending = null }
        }
    }
    function approveControl(value, confirmed) {
        var guard = life
        if (!canApproveControl || guard.pending || guard.controlPending || confirmed !== true || !controlPressCurrent(value) || value.applyConfirmed !== true
                || !value.held || value.held !== guard.controlHeld || value.held !== controlReceipt) return
        var held = value.held, owner = controller, actualView = dialog, epoch = guard.controlEpoch, base = controlContext, viewToken = guard.view
        var ticket = {cancelState: held.cancelState, resetting: true}, result = {}
        guard.controlPending = ticket
        var current = function() { return guard.alive && !guard.controlExhausted && guard.controlEpoch === epoch && guard.controlPending === ticket
            && !ticket.cancelState.cancelled && guard.controlHeld === held && controlReceipt === held && owner === controller && base === controlContext
            && guard.view === viewToken && dialog === actualView && !!actualView && visible && enabled && held.vehicleId === vehicleId
            && held.componentId === componentId && held.streamId === streamId && held.targetRunning === targetRunning && !actualView.controlConsentExhausted
            && actualView.controlReviewEpoch === value.reviewEpoch && actualView.controlApplyEpoch === value.applyEpoch + 1 && !actualView.controlApplyChecked }
        try {
            actualView.clearControlApplyConfirmation()
            ticket.resetting = false
            if (!current()) return
            callBusy = true
            if (!current()) return
            controlMessage = qsTr("正在申请本审阅的独立第二账号批准；取消或超时不会恢复旧权限。")
            if (!current()) return
            try { result = owner.approveFleetCameraStreamControl(held.reviewId, held.bindingHash, true) || {} }
            catch (error) { result = {reason: "camera-control-approval-call-unavailable"} }
            if (!guard.alive) return
            if (!current()) { try { owner.cancelFleetCameraStreamControl(held.reviewId) } catch (error) {} return }
            var state = controlWorkflow
            if (!keys(result, ["ok", "requestAccepted", "reviewId", "reason", "flightCommandReleased"]) || result.ok !== true || result.requestAccepted !== true
                    || result.reviewId !== held.reviewId || result.flightCommandReleased !== false || !controlKnown(state) || state.active !== true || state.state !== "applying"
                    || state.reviewId !== held.reviewId || state.bindingHash !== held.bindingHash || state.authenticationConsumed !== true || !controlOwn) {
                try { owner.cancelFleetCameraStreamControl(held.reviewId) } catch (error) {}
                if (current()) controlMessage = qsTr("未接纳执行：%1。不会自动重试或恢复。").arg(result.reason || qsTr("未知"))
                return
            }
            controlMessage = qsTr("双账号执行申请已接纳；仅后续定时推进原启停命令及Status回读，已发／ACK／报告事实分别保留。")
        } finally {
            ticket.resetting = false
            if (guard.alive && guard.controlPending === ticket) { callBusy = false; if (guard.alive && guard.controlPending === ticket) guard.controlPending = null }
        }
    }
    function cancelControl(value) {
        if (!controlPressCurrent(value) || callBusy || life.pending || life.controlPending || !controlOwn || value.held !== life.controlHeld || value.held !== controlReceipt) return
        var guard = life, epoch = guard.controlEpoch, owner = controller, base = controlContext
        retireControl()
        if (guard.alive && guard.controlEpoch === epoch + 1 && owner === controller && base === controlContext)
            controlMessage = qsTr("已请求退休本页控制审阅／后续步骤；已可能发送的命令不能撤回，不自动发反向命令。")
    }
    function controlSummary() {
        if (!controlKnown(controlWorkflow)) return qsTr("控制状态不可用；READ成功不会自动授予控制权。")
        var value = controlWorkflow, text = controlOwn ? qsTr("本页真实控制审阅") : qsTr("共享／历史控制快照，不属于本页")
        text += "\n" + qsTr("原目标：机号%1／相机%2／流%3；动作：%4\n状态：%5；原30秒剩余：%6 ms\n原因：%7")
            .arg(value.vehicleId || qsTr("未知")).arg(value.componentId || qsTr("未知")).arg(value.streamId || qsTr("未知"))
            .arg(value.operation === "start" ? "START" : value.operation === "stop" ? "STOP" : qsTr("未选择")).arg(value.state).arg(value.remainingMs).arg(value.reason || qsTr("无"))
        var basis = value.basisSummary
        if (map(basis) && Object.keys(basis).length === 20 && typeof basis.originalRunning === "boolean")
            text += "\n" + qsTr("原报告：%1（仅报告，非物理证明）；目标：%2")
                .arg(basis.originalRunning ? "RUNNING" : "not-running").arg(value.targetRunning ? "START" : "STOP")
        var run = value.run
        if (map(run) && Object.keys(run).length === 34 && map(run.action)) {
            text += "\n" + qsTr("启停命令已可能发送：%1；命令ACK：%2\n状态读取：").arg(run.action.requestIssued === true ? qsTr("是") : qsTr("否／未知"))
                .arg(run.action.ackObserved === true ? String(run.action.ackResult) : qsTr("未知")) + "\n" + querySummary("Status 270", run.statusQuery, 270)
            if (run.reportedTargetMatch === true) text += "\n" + qsTr("回读报告与目标一致；这不认证视频已实际启动／停止或无线因果。")
        }
        return text + "\n" + qsTr("START／恢复也必须新读取、新审阅和新二审；无自动恢复、重试、反向回滚或飞行许可。")
    }
    function finishView(actualView) {
        var guard = life
        if (!guard || !guard.alive || dialog !== actualView) return
        guard.view = null
        retire()
        if (!guard.alive || dialog !== actualView) return
        dialog = null
        if (guard.alive && !dialog && !guard.view && visible && enabled) entry.forceActiveFocus()
    }
    function dismiss() {
        var guard = life, actualView = dialog
        if (!guard || !guard.alive) return
        guard.view = null
        retire()
        if (!guard.alive) return
        if (dialog === actualView) dialog = null
        if (actualView) try { actualView.close() } catch (error) {}
    }
    function openView() {
        var guard = life, owner = controller
        if (!guard || !guard.alive || guard.exhausted || guard.opening || guard.pending || !owner || !visible || !enabled || dialog) return
        var epoch = guard.epoch
        guard.opening = true
        var actualView = null, token = {}
        try {
            actualView = viewComponent.createObject(mainWindow, {guard: guard, ownerPanel: root})
            if (!guard.alive || !actualView || owner !== controller || !visible || !enabled || dialog || guard.epoch !== epoch) {
                if (actualView) actualView.destroy()
                return
            }
            guard.view = token
            dialog = actualView
            if (!guard.alive || guard.view !== token || dialog !== actualView || guard.epoch !== epoch) { actualView.destroy(); return }
            actualView.open()
        } finally { guard.opening = false }
    }
    function ensureFocusVisible(item) {
        if (!dialog || !item) return
        var point = item.mapToItem(dialog.scrollContent, 0, 0), scroll = dialog.scrollArea
        if (point.y - pad < scroll.contentY) scroll.contentY = Math.max(0, point.y - pad)
        else if (point.y + item.height + pad > scroll.contentY + scroll.height)
            scroll.contentY = Math.min(Math.max(0, scroll.contentHeight - scroll.height), point.y + item.height + pad - scroll.height)
    }
    function querySummary(label, value, id) {
        if (!queryKnown(value, id)) return qsTr("%1：无完整原生读取快照").arg(label)
        return qsTr("%1：%2\n已可能发送：%3；ACK：%4；实际目标报告：%5\n原因：%6")
            .arg(label).arg(value.state).arg(value.requestIssued ? qsTr("是") : qsTr("否／未知"))
            .arg(value.ackObserved ? String(value.ackResult) : qsTr("未知"))
            .arg(value.reportObserved ? qsTr("已观察") : qsTr("未知")) .arg(value.reason || qsTr("无"))
    }
    function summary() {
        if (!known(workflow)) return qsTr("等待完整原生状态；未由本页建立读取。")
        var value = workflow.observation
        var title = own ? qsTr("本页真实接纳的读取快照") : qsTr("共享只读快照，不属于本页")
        if (snapshot.workflowHistorical === true) title += qsTr("（其它scope历史）")
        var text = title + "\n" + qsTr("该次目标：机号%1／相机%2／流%3\n状态：%4；该次剩余：%5 ms\n原因：%6")
            .arg(workflow.vehicleId || qsTr("未知")).arg(workflow.componentId || qsTr("未知")).arg(workflow.streamId || qsTr("未知"))
            .arg(workflow.state).arg(workflow.remainingMs).arg(workflow.reason || qsTr("无"))
        if (Object.keys(value).length) {
            text += "\n" + querySummary(qsTr("Info 269"), value.informationQuery, 269)
                + "\n" + querySummary(qsTr("Status 270"), value.statusQuery, 270)
                + "\n" + qsTr("该次Info已观察：%1；Status已观察：%2。报告新鲜度各自最多3秒，接纳读取不保证两项此刻同时新鲜。")
                    .arg(value.information.observed === true ? qsTr("是") : qsTr("否／未知"))
                    .arg(value.status.observed === true ? qsTr("是") : qsTr("否／未知"))
        }
        return text + "\n" + qsTr("ACK、RUNNING位和本地关联不认证实际送流／停流、无线因果、完整目录、可用带宽或恢复许可。")
    }
    onStableContextChanged: retire()
    onControlContextChanged: retireControl()
    onControlTargetChanged: retireControl()
    onVehicleTextChanged: retire()
    onComponentTextChanged: retire()
    onStreamTextChanged: retire()
    onControllerChanged: dismiss()
    onVisibleChanged: if (!visible) dismiss()
    onEnabledChanged: if (!enabled) dismiss()
    Component.onDestruction: {
        var guard = life, held = guard.held, actualView = dialog
        if (guard.pending) guard.pending.cancelState.cancelled = true
        var controlHeld = guard.controlHeld
        if (guard.controlPending) guard.controlPending.cancelState.cancelled = true
        if (controlHeld && controlHeld.cancelState) controlHeld.cancelState.cancelled = true
        guard.alive = false; guard.held = null; guard.view = null
        if (held && held.owner) try { held.owner.cancelFleetCameraStreamCapture(held.requestId) } catch (error) {}
        if (controlHeld && controlHeld.owner) try { controlHeld.owner.cancelFleetCameraStreamControl(controlHeld.reviewId) } catch (error) {}
        if (actualView) try { actualView.close() } catch (error) {}
    }
    QGCButton {
        id: entry
        width: parent.width; height: Math.max(root.rowHeight, implicitHeight); focusPolicy: Qt.StrongFocus
        text: qsTr("相机单流：读取／双账号控制审阅")
        enabled: root.controller !== null && !!root.life && !root.life.exhausted
        onClicked: root.openView()
        Accessible.name: text
        contentItem: QGCLabel { text: entry.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: entry._showHighlight ? palette.buttonHighlightText : palette.buttonText }
        background: Rectangle { color: entry._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: entry.activeFocus ? 3 : 1; radius: 3 }
    }
    Component {
        id: viewComponent
        Controls.Dialog {
            id: view
            property var ownerPanel: null
            property var guard: null
            property var viewLife: ({alive: true})
            property double consentEpoch: 0
            property bool consentExhausted: false
            property double controlReviewEpoch: 0
            property double controlApplyEpoch: 0
            property bool controlConsentExhausted: false
            readonly property bool consentChecked: consent.checked
            readonly property bool controlReviewChecked: reviewConsent.checked
            readonly property bool controlApplyChecked: applyConsent.checked
            property alias scrollArea: scroll
            property alias scrollContent: form
            parent: Controls.Overlay.overlay
            anchors.centerIn: parent
            modal: true; focus: true; closePolicy: Controls.Popup.NoAutoClose
            standardButtons: Controls.Dialog.NoButton; padding: root.pad
            width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 64, mainWindow.width - root.pad * 4))
            height: Math.max(1, Math.min(form.implicitHeight + heading.implicitHeight + root.pad * 4, mainWindow.height - root.pad * 4))
            background: Rectangle { color: palette.window; border.color: palette.text; border.width: 1; radius: 3 }
            header: QGCLabel { id: heading; padding: root.pad; text: qsTr("单相机单流：读取／独立双账号控制"); font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            function clearConfirmation() { if (guard && guard.alive && viewLife.alive) consent.checked = false }
            function clearControlConfirmations() {
                var epoch = guard ? guard.controlEpoch : -1
                if (guard && guard.alive && viewLife.alive) reviewConsent.checked = false
                if (guard && guard.alive && guard.controlEpoch === epoch && viewLife.alive && ownerPanel && ownerPanel.dialog === view) applyConsent.checked = false
            }
            function clearControlApplyConfirmation() { if (guard && guard.alive && viewLife.alive) applyConsent.checked = false }
            onOpened: closeButton.forceActiveFocus()
            onAboutToHide: if (guard && guard.alive && ownerPanel) ownerPanel.finishView(view)
            onClosed: {
                var actualView = view
                viewLife.alive = false
                if (guard && guard.alive && ownerPanel) ownerPanel.finishView(actualView)
                if (actualView) try { actualView.destroy() } catch (error) { /* already retired by a notification */ }
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
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("取消本页后续步骤／关闭（不撤回已发命令）")
                        onClicked: view.close()
                        Keys.onReturnPressed: { view.close(); event.accepted = true }
                        Keys.onEnterPressed: { view.close(); event.accepted = true }
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(closeButton)
                        contentItem: QGCLabel { text: closeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: closeButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: closeButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: closeButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: palette.warningText
                        text: qsTr("独立READ权限，无二审；实际来源满足原生检查时可读取飞行中的相机元数据。仅512请求269→270，固定单相机单流；不启停视频。最初8秒包含授权检查，每阶段至多4秒，不续期、不重试。270的param2索引仅作兼容尝试，不支持或无匹配报告则失败，不退0／all／广播。")
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("账号：%1 · 角色：%2").arg(root.identity.actorId || qsTr("未认证")).arg(root.identity.role || qsTr("未知")) }
                    QGCButton {
                        id: authButton
                        property var pressScope: ({})
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("认证当前操作员")
                        enabled: !!root.stableContext && !root.callBusy && root.workflow.active === false && root.identity.authenticated !== true
                            && !!root.controller && typeof root.controller.authenticateFleetOperator === "function"
                        onPressed: pressScope = root.pressed()
                        onClicked: root.authenticate(pressScope)
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(authButton)
                        contentItem: QGCLabel { text: authButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: authButton.enabled ? 1 : 0.5; color: authButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: authButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: authButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel { text: qsTr("目标机号（1–255）") }
                    QGCTextField {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: root.vehicleText; inputMethodHints: Qt.ImhDigitsOnly
                        enabled: !root.callBusy && root.workflow.active !== true && root.controlWorkflow.active !== true
                        onTextChanged: if (root.vehicleText !== text) root.vehicleText = text
                        Accessible.name: qsTr("相机元数据目标机号")
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(this)
                    }
                    QGCLabel { text: qsTr("相机component（100–105，不是autopilot component）") }
                    QGCTextField {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: root.componentText; inputMethodHints: Qt.ImhDigitsOnly
                        enabled: !root.callBusy && root.workflow.active !== true && root.controlWorkflow.active !== true
                        onTextChanged: if (root.componentText !== text) root.componentText = text
                        Accessible.name: qsTr("相机component编号")
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(this)
                    }
                    QGCLabel { text: qsTr("视频流ID（1–255，0／all不接受）") }
                    QGCTextField {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: root.streamText; inputMethodHints: Qt.ImhDigitsOnly
                        enabled: !root.callBusy && root.workflow.active !== true && root.controlWorkflow.active !== true
                        onTextChanged: if (root.streamText !== text) root.streamText = text
                        Accessible.name: qsTr("目标视频流ID")
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(this)
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        color: root.targetError ? palette.warningText : palette.text
                        text: root.targetError || qsTr("三个目标须明确填写；不会自动选择发现的相机，也不支持0／all或广播。")
                        Accessible.name: text
                    }
                    QGCCheckBox {
                        id: consent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: qsTr("确认仅读取此目标"); checked: false; activeFocusOnTab: true
                        enabled: root.canCapture
                        Accessible.name: qsTr("确认仅读取当前机号、相机和视频流")
                        Accessible.description: qsTr("只读取一次Info与Status，不启停视频，不自动重试，已可能发送的请求不能撤回。")
                        onCheckedChanged: {
                            if (view.consentEpoch >= 9007199254740991) view.consentExhausted = true
                            else view.consentEpoch += 1
                        }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(consent)
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: qsTr("确认仅向所填机号／相机／流读取一次Info与Status；不会启停视频、自动重试或恢复旧权限。")
                    }
                    QGCButton {
                        id: captureButton
                        property var pressScope: ({})
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("明确读取一次Info → Status")
                        enabled: root.canCapture && consent.checked
                        onPressed: pressScope = root.pressed()
                        onEnabledChanged: if (!enabled) pressScope = ({})
                        onClicked: root.capture(pressScope, consent.checked)
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(captureButton)
                        contentItem: QGCLabel { text: captureButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: captureButton.enabled ? 1 : 0.5; color: captureButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: captureButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: captureButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCButton {
                        id: cancelButton
                        property var pressScope: ({})
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("仅取消本页后续读取（保留已发／ACK／报告事实）")
                        enabled: root.own && root.workflow.active === true && !root.callBusy
                        onPressed: pressScope = root.pressed()
                        onClicked: root.cancelOwned(pressScope)
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(cancelButton)
                        contentItem: QGCLabel { text: cancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: cancelButton.enabled ? 1 : 0.5; color: cancelButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: cancelButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: cancelButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        id: status
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        activeFocusOnTab: visible; text: root.message + "\n" + root.summary()
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(status)
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: palette.text }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true
                        text: qsTr("独立控制审阅：STOP／START")
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: palette.warningText
                        text: qsTr("READ成功不是控制权。本页真实新读取回执仅供原生一次性基线选择；初取必须两报告新鲜且当前RUNNING报告与目标相反。先审阅，再另行确认并由不同账号批准；最初30秒包含准备和审批。STOP可能使操作员失去视频，包括飞行中的可视性。启停命令／Status读取每项最多4秒，读取ACK后等待最多1秒且只缩短。取消不能撤回已发命令，不自动重试、恢复或反向回滚；不改变飞行模式。")
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("明确目标状态（默认未选）") }
                    QGCComboBox {
                        id: controlChoice
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        model: [qsTr("未选择"), qsTr("STOP 停止此流"), qsTr("START 启动此流")]
                        currentIndex: root.controlTarget === "stop" ? 1 : root.controlTarget === "start" ? 2 : 0
                        enabled: !root.callBusy && root.workflow.active !== true && root.controlWorkflow.active !== true
                        onActivated: root.controlTarget = index === 1 ? "stop" : index === 2 ? "start" : ""
                        Accessible.name: qsTr("控制目标状态，停止或启动，默认未选")
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(controlChoice)
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: !root.own || root.workflow.state !== "observed" ? qsTr("请先由本页明确完成上方Info→Status读取；不能借共享／历史读取或导入资料创建控制审阅。")
                            : root.controlWorkflow.captureId === root.receipt.requestId ? qsTr("此读取已经用于一次审阅；取消、失败、反向动作或恢复均须先新读取。")
                            : typeof root.targetRunning !== "boolean" ? qsTr("请选择STOP或START；不会根据报告自动替你选择或提交。")
                            : !root.controlRoleAllowed() ? qsTr("当前账号没有独立控制权限或所需二审政策；READ权限不能代替它。")
                            : qsTr("按审阅时由原生复核真实新鲜基线；若过期／同目标／来源变化则拒绝，请重新读取。")
                    }
                    QGCCheckBox {
                        id: reviewConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; checked: false; activeFocusOnTab: true
                        text: qsTr("确认创建此目标审阅")
                        enabled: root.canReviewControl
                        onCheckedChanged: {
                            if (guard && guard.controlPending && !guard.controlPending.resetting) guard.controlPending.cancelState.cancelled = true
                            if (view.controlReviewEpoch >= 9007199254740991) view.controlConsentExhausted = true
                            else view.controlReviewEpoch += 1
                        }
                        Accessible.name: qsTr("第一次确认，仅创建当前目标的控制审阅")
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(reviewConsent)
                    }
                    QGCButton {
                        id: reviewButton
                        property var pressScope: ({})
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("审阅此目标STOP／START（尚不发送）")
                        enabled: root.canReviewControl && reviewConsent.checked
                        onPressed: pressScope = root.controlPressed()
                        onEnabledChanged: if (!enabled) pressScope = ({})
                        onClicked: root.requestControl(pressScope, reviewConsent.checked)
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(reviewButton)
                        contentItem: QGCLabel { text: reviewButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: reviewButton.enabled ? 1 : 0.5; color: reviewButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: reviewButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: reviewButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCCheckBox {
                        id: applyConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; checked: false; activeFocusOnTab: true
                        text: qsTr("确认二审后执行此动作")
                        enabled: root.canApproveControl
                        onCheckedChanged: {
                            if (guard && guard.controlPending && !guard.controlPending.resetting) guard.controlPending.cancelState.cancelled = true
                            if (view.controlApplyEpoch >= 9007199254740991) view.controlConsentExhausted = true
                            else view.controlApplyEpoch += 1
                        }
                        Accessible.name: qsTr("第二次确认，独立账号批准后执行本页原审阅")
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(applyConsent)
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("请先核对下方原目标和拟动作。此确认将请求不同账号审批；不是复用READ确认或旧批准。") }
                    QGCButton {
                        id: approveButton
                        property var pressScope: ({})
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("双账号批准并执行原审阅")
                        enabled: root.canApproveControl && applyConsent.checked
                        onPressed: pressScope = root.controlPressed()
                        onEnabledChanged: if (!enabled) pressScope = ({})
                        onClicked: root.approveControl(pressScope, applyConsent.checked)
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(approveButton)
                        contentItem: QGCLabel { text: approveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: approveButton.enabled ? 1 : 0.5; color: approveButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: approveButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: approveButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCButton {
                        id: controlCancelButton
                        property var pressScope: ({})
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("仅取消本页控制审阅／后续步骤")
                        enabled: root.controlOwn && root.controlWorkflow.active === true && !root.callBusy
                        onPressed: pressScope = root.controlPressed()
                        onEnabledChanged: if (!enabled) pressScope = ({})
                        onClicked: root.cancelControl(pressScope)
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(controlCancelButton)
                        contentItem: QGCLabel { text: controlCancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: controlCancelButton.enabled ? 1 : 0.5; color: controlCancelButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: controlCancelButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: controlCancelButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        id: controlStatus
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        activeFocusOnTab: visible; text: root.controlMessage + "\n" + root.controlSummary()
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(controlStatus)
                    }
                }
            }
        }
    }
}

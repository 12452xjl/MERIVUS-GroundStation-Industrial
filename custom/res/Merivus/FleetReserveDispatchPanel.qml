import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

ColumnLayout {
    id: root
    property var controller: null
    property bool inputSettled: false // Parent excludes this child's pending.
    property bool expanded: false
    property bool callBusy: false
    property int reserveIndex: -1
    property string profileText: ""
    property var profileOffer: null
    property var adoptedProfile: null
    property bool observeConsent: false
    property bool refreshConsent: false
    property var refreshJob: null
    property bool captureConsent: false
    property bool previewConsent: false
    property bool reviewConsent: false
    property bool adoptionConsent: false
    property bool requestConsent: false
    property bool approvalConsent: false
    property bool rereviewConsent: false
    property string message: ""
    property string profileError: ""
    property int routeMode: 0
    property int routePage: 0
    property var owned: ({capture: "", proposal: "", review: "", pending: "", reviewed: "", retainedReview: "", attempt: ""})
    property var life: ({alive: true, epoch: 0, consentEpoch: 0, exhausted: false, ticket: null,
        owner: null, episode: "", capture: "", proposal: "", review: "", retainedReview: "",
        pending: "", pendingProposal: "", pendingEpoch: 0, reviewed: "", attempt: "", refreshJob: null})
    signal reviewInvalidationRequested()
    signal focusVisibilityRequested(var item)

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var candidate: controller ? controller.fleetReserveDispatchSourceCandidate || {} : ({})
    readonly property var state: controller ? controller.fleetMissionRemainder || {} : ({})
    readonly property var capture: state.capture || ({})
    readonly property var proposal: state.proposal || ({})
    readonly property var sharedReview: controller ? controller.fleetMissionRemainderReview || {} : ({})
    readonly property var newReview: sharedReview.kind === "reserve-dispatch" ? sharedReview : ({})
    readonly property var status: state.review && state.review.kind === "reserve-dispatch" ? state.review : ({})
    readonly property var planning: controller ? controller.missionPlanningStatus || {} : ({})
    readonly property string episodeToken: controller && controller.fleetDeploymentLifecycle
        ? controller.fleetDeploymentLifecycle.currentToken || "" : ""
    readonly property bool editingLocked: sharedReview.accepted === true
    readonly property bool captureCurrent: !!owned.capture && capture.kind === "reserve-dispatch"
        && capture.token === owned.capture && capture.current === true
    readonly property bool proposalCurrent: captureCurrent && !!owned.proposal && proposal.kind === "reserve-dispatch"
        && proposal.token === owned.proposal && proposal.captureToken === owned.capture
    readonly property bool reviewCurrent: !!owned.review && newReview.token === owned.review
        && newReview.proposalToken === owned.reviewed && newReview.reviewAttemptToken === owned.attempt && newReview.current === true
    readonly property bool ownAccepted: newReview.accepted === true && !!owned.retainedReview
        && newReview.token === owned.retainedReview
    readonly property string approvalState: ownAccepted && controller && typeof controller.intentTaskState === "string"
        ? controller.intentTaskState : "unknown"
    readonly property bool planningBusy: !!owned.pending && status.requestId === owned.pending
        && status.proposalToken === owned.reviewed && status.planningPending === true
    readonly property bool attemptCurrent: captureCurrent && proposalCurrent && !!owned.attempt
        && status.reviewAttemptToken === owned.attempt && status.proposalToken === owned.proposal
    readonly property bool canRereview: ready && attemptCurrent && !planningBusy
        && planning.ready !== true && planning.status !== "completing"
    readonly property bool pendingInput: !ownAccepted && (!!owned.capture || callBusy
        || captureConsent || previewConsent || reviewConsent || adoptionConsent || rereviewConsent || !!owned.pending)
    readonly property bool ready: visible && expanded && enabled && !!controller && inputSettled
        && !callBusy && !refreshBusy && !life.exhausted && state.busy !== true && planning.active !== true
    readonly property var refreshAuthorization: controller ? controller.fleetEvidenceRefreshAuthorization || {} : ({})
    readonly property var refreshView: refreshProjection()
    readonly property bool refreshBusy: !!refreshJob && refreshJob.owner === controller && refreshJob.episode === episodeToken
        && (!refreshJob.requestId || !refreshKnown(refreshView, refreshJob) || refreshView.active === true)
    readonly property bool refreshAllowed: ready && captureCurrent && selectedReserveId > 0
        && (!editingLocked || ownAccepted) && !owned.pending && !profileEditor.startupPending
        && planning.ready !== true && planning.status !== "completing"
        && refreshAuthorization.allowed === true && (!controller.fleetEvidenceRefresh || controller.fleetEvidenceRefresh.active !== true)
    readonly property var reserveIds: captureCurrent && Array.isArray(capture.reserveVehicleIds)
        && capture.reserveVehicleIds.length <= 32 ? capture.reserveVehicleIds : []
    readonly property int selectedReserveId: reserveIndex >= 0 && reserveIndex < reserveIds.length ? reserveIds[reserveIndex] : 0
    readonly property bool profileOfferCurrent: !!profileOffer && profileOffer.owner === controller
        && profileOffer.episode === episodeToken && captureCurrent && profileOffer.capture === owned.capture
        && profileOffer.reserveId === selectedReserveId && profileOffer.dto.captureToken === owned.capture
        && profileOffer.dto.reserveVehicleId === selectedReserveId
    readonly property bool profileReady: profileOfferCurrent && !!adoptedProfile && adoptedProfile.offer === profileOffer
        && adoptedProfile.text === profileText && !profileEditor.pendingInput
    readonly property string profileSourceIdentity: captureCurrent && selectedReserveId > 0
        ? JSON.stringify({captureToken:owned.capture, reserveVehicleId:selectedReserveId, sourceTaskId:capture.sourceTaskId,
            planId:capture.planId, binding:capture.binding || {}, episode:episodeToken}) : ""
    readonly property var summary: proposalCurrent ? proposal.summary || {} : ({})
    readonly property var previewMember: proposalCurrent ? proposal.dispatchMember || {} : ({})
    readonly property bool previewComplete: proposalCurrent && previewValid(summary, previewMember)
    readonly property bool retainedReviewVisible: !!owned.retainedReview && newReview.token === owned.retainedReview
    readonly property var reviewedMembers: retainedReviewVisible && Array.isArray((newReview.plan || {}).members)
        ? newReview.plan.members : []
    readonly property var shownMember: proposalCurrent ? previewMember : reviewedMembers.length === 1 ? reviewedMembers[0] : ({})
    readonly property var shownRoute: routeMode === 0
        ? (Array.isArray(shownMember.waypoints) ? shownMember.waypoints : [])
        : (Array.isArray(shownMember.returnPath) ? shownMember.returnPath : [])
    readonly property int pageCount: Math.max(1, Math.ceil(Math.min(shownRoute.length || 0, 2048) / 8))
    readonly property int currentPage: Math.min(Math.max(0, routePage), pageCount - 1)
    spacing: pad
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function token(value) { return typeof value === "string" && value.length > 0 && value.length <= 256 && !/[\u0000-\u001f\u007f]/.test(value) }
    function text(value) { return typeof value === "string" && value ? value.substring(0, 512) : qsTr("未知 / 未提供") }
    function integer(value, low, high) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= low && value <= high }
    function keys(value, names) {
        if (!value || typeof value !== "object" || Array.isArray(value) || Object.keys(value).length !== names.length) return false
        for (var i = 0; i < names.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, names[i])) return false
        return true
    }
    function counter(value, positive) {
        return typeof value === "string" && /^(0|[1-9][0-9]{0,19})$/.test(value)
            && (value.length < 20 || value <= "18446744073709551615") && (!positive || value !== "0")
    }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) }
    function refreshKnown(value, job) {
        if (!job || !keys(value, ["schemaVersion", "ok", "error", "captureToken", "reserveVehicleId", "requestId",
                "generation", "current", "active", "status", "snapshotOnly", "revalidationRequired", "startupPermitGranted", "flightCommandReleased"])
                || value.schemaVersion !== 1 || typeof value.ok !== "boolean" || typeof value.error !== "string"
                || value.error.length > 512
                || typeof value.current !== "boolean" || typeof value.active !== "boolean" || value.snapshotOnly !== true
                || value.revalidationRequired !== true || value.startupPermitGranted !== false || value.flightCommandReleased !== false
                || (value.requestId !== "" && !uuid(value.requestId))
                || (value.generation !== "" && !counter(value.generation, true))) return false
        if (job.requestId && (value.requestId !== job.requestId || value.generation !== job.generation)) return false
        var s = value.status
        if (!job.requestId && value.captureToken === "" && value.reserveVehicleId === 0)
            return value.ok === false && value.requestId === "" && value.generation === ""
                && value.current === false && value.active === false && keys(s, [])
        if (value.captureToken !== job.capture || value.reserveVehicleId !== job.reserveId) return false
        if (keys(s, [])) return value.active === false && value.current === false
        if (!keys(s, ["ok", "requestId", "generation", "state", "reason", "active", "actorId", "vehicleIds", "vehicles",
                "completedVehicleCount", "totalVehicleCount", "currentVehicleId", "currentParameter", "remainingMs", "requestLifetimeMs",
                "stageTimeoutMs", "requestIntervalMs", "readOnlyRequestsMayRemainInflight", "snapshotOnly", "revalidationRequired",
                "responseAttributionVerified", "signatureVerified", "hardwareIdentityAuthenticated", "reviewRestored",
                "uploadApprovalRestored", "startupPermitGranted", "flightCommandReleased"])
                || !uuid(value.requestId) || !counter(value.generation, true)
                || s.requestId !== value.requestId || s.generation !== value.generation
                || (s.active !== value.active && (value.current !== false || value.active !== false))
                || typeof s.ok !== "boolean" || s.ok !== (s.state === "observations-renewed")
                || ["refreshing", "observations-renewed", "failed", "stale", "canceled"].indexOf(s.state) < 0
                || s.active !== (s.state === "refreshing") || typeof s.reason !== "string" || s.reason.length > 512
                || typeof s.actorId !== "string" || s.actorId.length > 256
                || !Array.isArray(s.vehicleIds) || s.vehicleIds.length !== 1 || s.vehicleIds[0] !== job.reserveId
                || !Array.isArray(s.vehicles) || s.vehicles.length !== 1 || s.totalVehicleCount !== 1
                || !integer(s.completedVehicleCount, 0, 1) || (s.currentVehicleId !== 0 && s.currentVehicleId !== job.reserveId)
                || typeof s.currentParameter !== "string" || s.currentParameter.length > 128
                || !integer(s.remainingMs, 0, 120000) || s.requestLifetimeMs !== 120000 || s.stageTimeoutMs !== 4000
                || s.requestIntervalMs !== 100 || typeof s.readOnlyRequestsMayRemainInflight !== "boolean"
                || s.snapshotOnly !== true || s.revalidationRequired !== true) return false
        var falseFlags = ["responseAttributionVerified", "signatureVerified", "hardwareIdentityAuthenticated", "reviewRestored",
            "uploadApprovalRestored", "startupPermitGranted", "flightCommandReleased"]
        for (var f = 0; f < falseFlags.length; ++f) if (s[falseFlags[f]] !== false) return false
        var row = s.vehicles[0]
        if (!keys(row, ["vehicleId", "componentId", "state", "versionSourceEpoch", "versionReceiptSequence", "managerEpoch", "parameterReceipts"])
                || row.vehicleId !== job.reserveId || !integer(row.componentId, 1, 255)
                || ["queued", "awaiting-version-receipt", "refreshing-parameters", "awaiting-parameter-receipt", "observations-renewed", "failed", "stale", "canceled"].indexOf(row.state) < 0
                || typeof row.versionSourceEpoch !== "string" || row.versionSourceEpoch.length > 128
                || typeof row.managerEpoch !== "string" || row.managerEpoch.length > 128
                || (row.versionReceiptSequence !== "" && !counter(row.versionReceiptSequence, true))
                || !Array.isArray(row.parameterReceipts) || row.parameterReceipts.length > 29) return false
        var seen = {}
        for (var p = 0; p < row.parameterReceipts.length; ++p) {
            var receipt = row.parameterReceipts[p]
            if (!keys(receipt, ["name", "receiptSequence", "managerEpoch", "source", "observedAtRefreshMonotonicMs", "responseAttributionVerified"])
                    || typeof receipt.name !== "string" || !/^[A-Z0-9_]{1,128}$/.test(receipt.name) || seen[receipt.name]
                    || !counter(receipt.receiptSequence, true) || receipt.managerEpoch !== row.managerEpoch
                    || receipt.source !== "live-param-value" || receipt.responseAttributionVerified !== false
                    || !integer(receipt.observedAtRefreshMonotonicMs, 0, 9007199254740991)) return false
            seen[receipt.name] = true
        }
        return s.completedVehicleCount === (row.state === "observations-renewed" ? 1 : 0)
            && (s.state !== "observations-renewed" || row.parameterReceipts.length === 29)
    }
    function refreshHistorical(job, reason) {
        // Value-only fallback: a remembered accepted/current receipt is not
        // evidence of current ownership after a failed or detached lookup.
        try {
            if (!job || !job.receipt) return ({})
            var value = JSON.parse(JSON.stringify(job.receipt))
            value.current = false
            value.error = reason
            return value
        } catch (error) { return ({}) }
    }
    function refreshProjection() {
        var g = life, job = refreshJob, owner = controller, historical = refreshHistorical
        // This existing NOTIFY-backed value is only a dependency. Never use
        // its latest request ID to claim/cancel another page's acquisition.
        var notification = owner ? owner.fleetEvidenceRefresh : null
        if (!g.alive || !job) return ({})
        if (!job.requestId) return historical(job, "reserve_dispatch_refresh_no_returned_job_id")
        if (job.owner !== owner || job.episode !== episodeToken)
            return historical(job, "reserve_dispatch_refresh_detached_history")
        try {
            var value = owner.fleetReserveDispatchEvidenceRefreshStatus(job.capture, job.reserveId, job.requestId)
            if (g.alive && g.refreshJob === job && owner === controller && refreshKnown(value, job)) return value
        } catch (error) {}
        return historical(job, "reserve_dispatch_refresh_current_status_unknown")
    }
    // Detached cleanup uses only frozen arguments, including after destruction.
    function detachedRefreshCancel(job) {
        if (!job) return
        job.cancellation.cancelled = true
        try {
            if (job.owner && typeof job.requestId === "string"
                    && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(job.requestId))
                job.owner.cancelFleetReserveDispatchEvidenceRefresh(job.capture, job.reserveId, job.requestId)
        } catch (error) {}
    }
    function pretty(value) { try { return JSON.stringify(value, null, 2) } catch (error) { return qsTr("资料不可显示，不能据此采用") } }
    function previewValid(value, member) {
        var keys = ["reserveVehicleId", "replacementVehicleId", "role", "outboundPointCount", "returnPointCount",
            "estimatedIngressSeconds", "estimatedTaskSeconds", "retainedHoldSeconds", "estimatedOutboundSeconds",
            "estimatedReturnSeconds", "standbyReplayed", "routeClearanceVerified", "performanceCertified", "flightCommandReleased"]
        if (!value || typeof value !== "object" || Array.isArray(value) || Object.keys(value).length !== 14
                || !member || typeof member !== "object" || Array.isArray(member)) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, keys[i])) return false
        if (!integer(value.reserveVehicleId, 1, 255) || value.reserveVehicleId !== selectedReserveId
                || member.vehicleId !== value.reserveVehicleId || !integer(value.replacementVehicleId, 1, 255)
                || typeof value.role !== "string" || !value.role || member.role !== value.role
                || value.standbyReplayed !== false || value.routeClearanceVerified !== false
                || value.performanceCertified !== false || value.flightCommandReleased !== false
                || !Array.isArray(member.waypoints) || !Array.isArray(member.returnPath)
                || !integer(value.outboundPointCount, 1, 2048) || !integer(value.returnPointCount, 1, 2048)
                || member.waypoints.length !== value.outboundPointCount || member.returnPath.length !== value.returnPointCount
                || member.waypoints.length + member.returnPath.length > 2048
                || !member.executionProfile || typeof member.executionProfile !== "object"
                || Array.isArray(member.executionProfile) || !Object.keys(member.executionProfile).length) return false
        for (var j = 5; j < 10; ++j) if (typeof value[keys[j]] !== "number" || !isFinite(value[keys[j]]) || value[keys[j]] < 0) return false
        return true
    }
    function reviewIdentity() {
        return JSON.stringify({token: newReview.token, proposalToken: newReview.proposalToken,
            kind: newReview.kind, archive: newReview.archive || {}})
    }
    function rereviewExpected() {
        return {captureToken: life.capture, proposalToken: life.proposal, reviewAttemptToken: life.attempt,
            reviewToken: newReview.reviewAttemptToken === life.attempt ? newReview.token || "" : "",
            requestId: status.reviewAttemptToken === life.attempt ? status.requestId || "" : ""}
    }
    function publish() {
        var g = life
        if (g.alive) owned = {capture: g.capture, proposal: g.proposal, review: g.review,
            pending: g.pending, reviewed: g.reviewed, retainedReview: g.retainedReview, attempt: g.attempt}
    }
    function consentChanged() {
        if (life.consentEpoch >= 9007199254740991) life.exhausted = true
        else life.consentEpoch += 1
    }
    function resetConsents() {
        var g = life
        captureConsent = false; if (!g.alive) return
        previewConsent = false; if (!g.alive) return
        reviewConsent = false; if (!g.alive) return
        adoptionConsent = false; if (!g.alive) return
        requestConsent = false; if (!g.alive) return
        approvalConsent = false; if (!g.alive) return
        rereviewConsent = false
        if (!g.alive) return
        observeConsent = false
        if (g.alive) refreshConsent = false
    }
    // This function deliberately uses ONLY its arguments. Callers capture it
    // before a notification; a late return must not look up a dead panel.
    function detachedCancel(owner, requestId, captureToken, reviewToken) {
        try { if (owner && typeof requestId === "string" && requestId) owner.cancelMissionPlanning(requestId) } catch (error) {}
        try {
            if (owner && typeof reviewToken === "string" && reviewToken) owner.clearFleetMissionRemainderReview(reviewToken)
            else if (owner && typeof captureToken === "string" && captureToken) owner.clearFleetMissionRemainder(captureToken)
        } catch (error) {}
    }
    function retire(inputChange) {
        var g = life, owner = g.owner, requestId = g.pending, cancel = detachedCancel
        var refreshing = g.refreshJob, stopRefresh = detachedRefreshCancel
        // Ordinary edits do not cancel or rewrite a completed historical
        // refresh. Only an unfinished/unknown original request is retired.
        var originalEpoch = g.epoch, refreshSnapshot = refreshView
        if (!g.alive || g.epoch !== originalEpoch || g.refreshJob !== refreshing) return
        var refreshEnded = refreshing && refreshing.requestId && refreshKnown(refreshSnapshot, refreshing) && refreshSnapshot.active === false
        if (refreshEnded) refreshing = null
        if (refreshing) refreshing.cancellation.cancelled = true
        var hadReviewInput = !!(g.proposal || g.review || g.pending || g.retainedReview)
        if (g.epoch >= 9007199254740991) g.exhausted = true
        else g.epoch += 1
        if (g.ticket) g.ticket.cancelled = true
        var expectedEpoch = g.epoch
        if (inputChange !== true) {
            profileOffer = null
            if (!g.alive || g.epoch !== expectedEpoch) return
            adoptedProfile = null
            if (!g.alive || g.epoch !== expectedEpoch) return
        }
        // Hidden views retain descriptive planning anchors, never adoption
        // consent. Reopening must explicitly re-review before re-adoption.
        if (inputChange === true) { g.proposal = ""; g.reviewed = ""; g.attempt = "" }
        g.review = ""; g.pending = ""; g.pendingProposal = ""
        // Hide/input edits retire local qualifications, not an already adopted
        // shared deployment source or someone else's new global result.
        try { resetConsents() }
        finally { stopRefresh(refreshing); cancel(owner, requestId, "", "") }
        if (!g.alive || g.epoch !== expectedEpoch) return
        publish()
        if (g.alive && g.epoch === expectedEpoch && inputChange === true && g.capture && hadReviewInput) reviewInvalidationRequested()
    }
    function leaveOwner() {
        var g = life, owner = g.owner, pending = g.pending, captureId = g.capture, cancel = detachedCancel
        // Clear only this exact frozen source. The Root keeps its separate
        // adopted reviewContext/record; hiding alone does not enter this path.
        g.owner = null; g.capture = ""; g.retainedReview = ""; g.proposal = ""; g.reviewed = ""; g.attempt = ""
        try { retire(false) }
        finally { cancel(owner, pending, captureId, "") }
    }
    function editProfile(value) {
        var g = life, epoch = g.epoch, owner = controller
        if (!g.alive || editingLocked || callBusy || refreshBusy || g.ticket || value === profileText) return
        profileText = value
        if (!g.alive || g.epoch !== epoch || owner !== controller) return
        retire(true)
    }
    function selectReserve(index) {
        if (!captureCurrent || editingLocked || callBusy || refreshBusy || life.ticket || !integer(index, 0, reserveIds.length - 1)) return
        var g = life, epoch = g.epoch, owner = controller
        reserveIndex = index
        if (!g.alive || g.epoch !== epoch || owner !== controller) return
        profileOffer = null
        if (!g.alive || g.epoch !== epoch || owner !== controller) return
        adoptedProfile = null
        if (g.alive && g.epoch === epoch && owner === controller) retire(true)
    }
    function adoptProfile(nextText, expectedSession, originalText) {
        var g = life, owner = controller, offer = profileOffer
        if (!g.alive || !ready || g.ticket || editingLocked || !profileOfferCurrent
                || expectedSession !== offer || originalText !== profileText) return
        editProfile(nextText)
        if (!g.alive || controller !== owner || profileOffer !== offer || !profileOfferCurrent || profileText !== nextText) return
        adoptedProfile = {offer: offer, text: nextText}
        if (g.alive && controller === owner && profileOffer === offer && profileText === nextText) profileError = ""
    }
    function startRefresh(value) {
        var g = life, owner = controller, stop = detachedRefreshCancel
        if (!value || !refreshAllowed || !refreshConsent || g.ticket || profileEditor.startupPending
                || !current(value, null) || value.action !== "refresh") return
        var ticket = {cancelled:false, action:"refresh"}
        var job = {owner:owner, episode:value.episode, capture:value.capture, reserveId:value.reserveId,
            requestId:"", generation:"", receipt:null, cancellation:{cancelled:false}}
        g.ticket = ticket
        g.refreshJob = job
        var returned = null, invoked = false, consumedEpoch = -1
        function same() {
            return g.alive && current(value, ticket) && (consumedEpoch < 0 || g.consentEpoch === consumedEpoch)
        }
        try {
            callBusy = true
            if (!same() || profileEditor.startupPending || !consume(value)) return
            consumedEpoch = g.consentEpoch
            if (!same() || profileEditor.startupPending) return
            // Retire only this page's qualifications. Root freezes its actual
            // capture before retiring the corresponding native review. Do not
            // signal the parent invalidator across that admission/bridge.
            profileOffer = null
            if (!same()) return
            adoptedProfile = null
            if (!same()) return
            g.proposal = ""; g.review = ""; g.pending = ""; g.pendingProposal = ""
            g.reviewed = ""; g.retainedReview = ""; g.attempt = ""
            publish()
            if (!same() || profileEditor.startupPending) return
            refreshJob = job
            if (!same() || g.refreshJob !== job || profileEditor.startupPending) return
            invoked = true
            returned = owner.requestFleetReserveDispatchEvidenceRefresh(value.capture, value.reserveId, true, job.cancellation)
            // Even a stale/failed wrapper can contain this call's real ID.
            // A late return cleans only that returned exact original request.
            var actualId = returned && returned.captureToken === job.capture && returned.reserveVehicleId === job.reserveId
                && typeof returned.requestId === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(returned.requestId)
                    ? returned.requestId : ""
            if (!same() || g.refreshJob !== job || job.cancellation.cancelled) {
                stop({owner:owner, capture:job.capture, reserveId:job.reserveId, requestId:actualId, cancellation:job.cancellation})
                // An alive, still-owned retired slot may retain only this
                // actual late receipt as history. It cannot qualify a new
                // source or overwrite any callback-installed successor.
                if (g.alive && g.refreshJob === job && actualId && refreshKnown(returned, job)) {
                    var history = JSON.parse(JSON.stringify(returned))
                    history.current = false; history.error = "reserve_dispatch_refresh_late_retired_receipt"
                    var lateJob = {owner:owner, episode:job.episode, capture:job.capture, reserveId:job.reserveId,
                        requestId:actualId, generation:returned.generation, receipt:history, cancellation:job.cancellation}
                    g.refreshJob = lateJob
                    refreshJob = lateJob
                }
                return
            }
            if (!refreshKnown(returned, job)) {
                stop({owner:owner, capture:job.capture, reserveId:job.reserveId, requestId:actualId, cancellation:job.cancellation})
                if (!same() || g.refreshJob !== job) return
                g.refreshJob = null
                refreshJob = null
                if (same()) {
                    message = qsTr("刷新回执不完整；已按本次真实返回ID尝试停止后续读取。不能断言未发送，旧配置依据不恢复；请核对共享刷新状态，不自动重试。")
                    if (same()) feedbackFocus()
                }
                return
            }
            if (!returned.requestId) {
                g.refreshJob = null
                refreshJob = null
                if (same()) {
                    message = qsTr("刷新没有返回本次实际请求ID：%1。旧依据和确认不恢复；检查地面状态、当前身份和空闲来源后重新明确操作。") .arg(text(returned.error))
                    if (same()) feedbackFocus()
                }
                return
            }
            var completedJob = {owner:owner, episode:job.episode, capture:job.capture, reserveId:job.reserveId,
                requestId:returned.requestId, generation:returned.generation,
                receipt:JSON.parse(JSON.stringify(returned)), cancellation:job.cancellation}
            g.refreshJob = completedJob
            refreshJob = completedJob
            if (!same() || g.refreshJob !== completedJob) return
            if (returned.ok !== true || returned.current !== true) {
                stop(completedJob)
                if (!same() || g.refreshJob !== completedJob) return
            }
            message = returned.ok === true && returned.current === true && returned.active === true
                ? qsTr("已提交本页所选备用机的版本及固定29项参数只读刷新。请等待本次状态；完成后仍须另点“读取本次参数依据”，重新采用速度、确认并生成配置。")
                : qsTr("刷新未形成可继续的当前请求：%1。已有读取／归档事实保留，不恢复旧依据或批准；可核对本次状态后重新明确操作。") .arg(text(returned.error))
            if (same() && returned.ok !== true) feedbackFocus()
        } catch (error) {
            stop(job)
            if (g.alive && g.refreshJob === job) {
                g.refreshJob = null
                refreshJob = null
                if (g.alive && operationCurrent(value, ticket)) {
                    message = qsTr("刷新调用中断；旧依据不恢复，已发只读请求可能仍在途。请核对实际刷新状态，不自动重试。")
                    if (g.alive && operationCurrent(value, ticket)) feedbackFocus()
                }
            }
        } finally {
            if (!invoked || !g.alive || ticket.cancelled) stop(job)
            // A local rejection before native admission must not leave an
            // ID-less busy slot. Never erase a callback-installed newer job.
            if (g.alive && g.refreshJob === job && !job.requestId) {
                g.refreshJob = null
                refreshJob = null
            }
            if (g.alive) callBusy = false
            if (g.ticket === ticket) g.ticket = null
        }
    }
    function refreshCancelIntent() {
        return {owner:controller, episode:episodeToken, epoch:life.epoch, job:refreshJob,
            requestId:refreshJob ? refreshJob.requestId : "", generation:refreshJob ? refreshJob.generation : ""}
    }
    function cancelRefresh(value) {
        var g = life, job = g.refreshJob, stop = detachedRefreshCancel
        if (!g.alive || !value || !job || value.job !== job || value.owner !== controller
                || value.episode !== episodeToken || value.epoch !== g.epoch
                || value.requestId !== job.requestId || value.generation !== job.generation) return
        // This flag revokes only this invocation, including the pre-ID OS
        // admission stack. It cannot grant permission or select another job.
        job.cancellation.cancelled = true
        if (!job.requestId) {
            if (g.ticket && g.ticket.action === "refresh") g.ticket.cancelled = true
            message = qsTr("已退休本次准备中的刷新意图；晚返回真实ID时仍精确取消。取消不撤回可能已发的只读请求。")
            return
        }
        if (g.ticket || callBusy) return
        var ticket = {cancelled:false, action:"cancel-refresh"}
        g.ticket = ticket
        try {
            callBusy = true
            if (!g.alive || g.refreshJob !== job || ticket.cancelled || value.epoch !== g.epoch) return
            stop(job)
            if (!g.alive || g.refreshJob !== job || ticket.cancelled || value.epoch !== g.epoch) return
            message = qsTr("已请求精确停止本页原刷新ID的后续读取；以本次状态为准。已发送／收到的只读材料不回滚，旧配置和批准不恢复。")
        } finally {
            if (g.alive) callBusy = false
            if (g.ticket === ticket) g.ticket = null
        }
    }
    function observeProfile(value) {
        var g = life, ticket = {cancelled:false, action:"observe"}, owner = controller
        if (!value || !ready || g.ticket || editingLocked || !captureCurrent || !observeConsent || profileEditor.startupPending
                || !integer(value.reserveId,1,255) || !current(value,null)) return
        g.ticket = ticket
        try {
            callBusy = true
            if (!g.alive || !current(value,ticket) || profileEditor.startupPending || !consume(value)
                    || !current(value,ticket) || profileEditor.startupPending) return
            // Retire the old local offer before any provider callback. Only
            // this call's returned token, never a shared latest token, is used.
            profileOffer = null
            if (!g.alive || !current(value,ticket)) return
            adoptedProfile = null
            if (!g.alive || !current(value,ticket) || profileEditor.startupPending) return
            var reply = value.owner.observeFleetReserveDispatchProfile(value.capture, value.reserveId, true)
            if (!g.alive || !current(value,ticket)) return
            if (!profileEditor.offerKnown(reply) || reply.captureToken !== value.capture || reply.reserveVehicleId !== value.reserveId) {
                message = qsTr("参数依据未确认：%1。保持未知；可检查实际连接／参数来源后重新明确读取，不请求刷新或写参数。")
                    .arg(text(reply && reply.error))
                if (g.alive && current(value,ticket)) feedbackFocus()
                return
            }
            var dto = JSON.parse(JSON.stringify(reply)), expectedEpoch = g.epoch + 1
            // This explicit new read retires old proposals/confirmations; the
            // exact one local retirement is not permission/source rebasing.
            retire(true)
            if (!g.alive || g.epoch !== expectedEpoch || controller !== owner || episodeToken !== value.episode
                    || !captureCurrent || life.capture !== value.capture || selectedReserveId !== value.reserveId) return
            var adopted = {owner:owner, episode:value.episode, capture:value.capture, reserveId:value.reserveId, dto:dto}
            profileOffer = adopted
            if (!g.alive || profileOffer !== adopted || g.epoch !== expectedEpoch || !profileOfferCurrent) return
            message = qsTr("只读取了本页所选备用机当前12参数缓存与原预算意图。请明确采用预算速度，再填写新动作／坐标／时序／策略；没有刷新参数、写参数或发飞行命令。")
        } catch (error) {
            if (g.alive && controller === owner) message = qsTr("本次只读依据回执中断，未采用旧hash；请核对来源后重新明确读取。")
        } finally {
            if (g.alive) callBusy = false
            if (g.ticket === ticket) g.ticket = null
        }
    }
    function intent(action) {
        return {action: action, owner: controller, epoch: life.epoch, consentEpoch: life.consentEpoch,
            episode: episodeToken, capture: life.capture, proposal: life.proposal, review: life.review,
            retainedReview: life.retainedReview, reserveId: selectedReserveId, profile: profileText,
            offer: profileOffer, profileBinding: adoptedProfile,
            pending: life.pending,
            reviewIdentity: reviewIdentity(), approvalState: approvalState,
            rereviewBinding: action === "rereview" ? rereviewExpected() : null,
            binding: action === "capture" ? JSON.parse(JSON.stringify(candidate.binding || {})) : null,
            candidateStamp: action === "capture" ? JSON.stringify(candidate.binding || {}) : ""}
    }
    function current(value, ticket) {
        return operationCurrent(value, ticket)
            && value.capture === life.capture && value.profile === profileText && value.reserveId === selectedReserveId
            && (value.action !== "preview" || (profileReady && value.offer === profileOffer && value.profileBinding === adoptedProfile))
            && (["accept", "request", "approve"].indexOf(value.action) < 0 || value.reviewIdentity === reviewIdentity())
    }
    function operationCurrent(value, ticket) {
        return life.alive && value && value.owner === controller && value.epoch === life.epoch
            && value.episode === episodeToken && visible && expanded && enabled && !life.exhausted
            && (!ticket || (life.ticket === ticket && ticket.cancelled !== true))
    }
    function consume(value) {
        if (value.consentEpoch !== life.consentEpoch) return false
        var g = life, expected = g.consentEpoch + (captureConsent ? 1 : 0) + (previewConsent ? 1 : 0)
            + (reviewConsent ? 1 : 0) + (adoptionConsent ? 1 : 0) + (requestConsent ? 1 : 0) + (approvalConsent ? 1 : 0) + (rereviewConsent ? 1 : 0) + (observeConsent ? 1 : 0) + (refreshConsent ? 1 : 0)
        if (expected > 9007199254740991) { g.exhausted = true; return false }
        resetConsents()
        return g.alive && !g.exhausted && g.consentEpoch === expected
            && !captureConsent && !previewConsent && !reviewConsent && !adoptionConsent && !requestConsent && !approvalConsent && !rereviewConsent && !observeConsent && !refreshConsent
    }
    function feedbackFocus() {
        var g = life, epoch = g.epoch, owner = controller
        if (!g.alive || !visible || !expanded) return
        feedback.forceActiveFocus()
        if (g.alive && epoch === g.epoch && owner === controller && visible && expanded) focusVisibilityRequested(feedback)
    }
    function reconcile() {
        var g = life
        if (!g.alive || g.ticket || !g.pending || g.owner !== controller || g.episode !== episodeToken
                || g.pendingEpoch !== g.epoch || !visible || !expanded) return
        if (newReview.kind !== "reserve-dispatch" || newReview.requestId !== g.pending
                || newReview.reviewAttemptToken !== g.attempt || status.reviewAttemptToken !== g.attempt
                || newReview.proposalToken !== g.pendingProposal || !token(newReview.token)
                || status.requestId !== g.pending || status.proposalToken !== g.pendingProposal
                || status.planningPending === true || status.planArchiveRecorded !== true
                || (newReview.archive || {}).recordId !== status.recordId) return
        g.review = newReview.token; g.retainedReview = newReview.token; g.pending = ""; g.pendingProposal = ""
        publish()
    }
    function perform(value) {
        var g = life, originalEpoch = g.epoch, originalOwner = controller
        if (!value || life.ticket || !ready || !current(value, null)) return
        var action = value.action
        if (action === "capture" && (!captureConsent || editingLocked || candidate.available !== true
                || state.capture && state.capture.token || value.candidateStamp !== JSON.stringify(candidate.binding || {}))) return
        if (action === "preview" && (!previewConsent || !captureCurrent || editingLocked || !profileReady)) return
        if (action === "review" && (!reviewConsent || !previewComplete || editingLocked
                || attemptCurrent || value.proposal !== life.proposal || life.reviewed === value.proposal)) return
        if (action === "accept" && (!adoptionConsent || !reviewCurrent || editingLocked || value.review !== life.review)) return
        if (action === "request" && (!requestConsent || !ownAccepted || newReview.current !== true
                || newReview.approvalAllowed !== true || value.approvalState !== "preview" || approvalState !== "preview")) return
        if (action === "approve" && (!approvalConsent || !ownAccepted || newReview.current !== true
                || newReview.approvalAllowed !== true || value.approvalState !== "awaiting-approval" || approvalState !== "awaiting-approval")) return
        if (action === "rereview" && (!rereviewConsent || !canRereview
                || JSON.stringify(value.rereviewBinding) !== JSON.stringify(rereviewExpected()))) return
        if (["capture", "preview", "review", "accept", "request", "approve", "rereview"].indexOf(action) < 0) return
        var profile = null
        if (action === "preview") {
            if (!integer(value.reserveId, 1, 255) || reserveIds.indexOf(value.reserveId) < 0) {
                message = qsTr("请在本次冻结来源的候选中明确选择一架备用机。未默认选机。")
                if (g.alive && originalEpoch === g.epoch && originalOwner === controller) feedbackFocus()
                return
            }
            try {
                if (!value.profile.trim() || value.profile.length > 16384) throw "profile-budget"
                profile = JSON.parse(value.profile)
                if (!profile || typeof profile !== "object" || Array.isArray(profile) || Object.keys(profile).length === 0) throw "profile-object"
            } catch (error) {
                var invalidProfileText = qsTr("请先用当前参数依据和结构化向导生成完整新配置，最多16384字符；不继承旧机配置或手填hash。")
                profileError = invalidProfileText
                if (!g.alive || originalEpoch !== g.epoch || originalOwner !== controller) return
                message = invalidProfileText
                if (g.alive && originalEpoch === g.epoch && originalOwner === controller) feedbackFocus()
                return
            }
        }
        var ticket = {cancelled: false, action: action}, cancel = detachedCancel, result = null, previewCalled = false
        g.ticket = ticket
        callBusy = true
        try {
            if (!g.alive || !current(value, ticket) || !consume(value) || !current(value, ticket)) return
            if (action === "capture") result = value.owner.captureFleetReserveDispatchSource(value.binding, true)
            else if (action === "preview") {
                previewCalled = true
                result = value.owner.previewFleetReserveDispatch(value.capture,
                    {reserveVehicleId: value.reserveId, executionProfile: profile, operatorConfirmed: true}, value.offer.dto.offerToken)
            }
            else if (action === "review") result = value.owner.reviewFleetMissionRemainder(value.proposal, true)
            else if (action === "rereview") result = value.owner.rereviewFleetReserveDispatch(value.rereviewBinding, true)
            else if (action === "accept") result = value.owner.acceptFleetMissionRemainderReview(value.review, true)
            else if (action === "request") result = value.owner.requestFleetMissionRemainderApproval(value.retainedReview)
            else result = value.owner.approveFleetMissionRemainder(value.retainedReview)
            var abandoned = !g.alive || ticket.cancelled || g.epoch !== value.epoch
            if (abandoned) {
                // Acceptance is a committed staging choice, not rollbackable
                // merely because the view disappeared during its notification.
                if (action === "rereview") cancel(value.owner, result && result.requestId || "", "", "")
                else if (["capture", "preview", "review"].indexOf(action) >= 0) cancel(value.owner, result && result.requestId || "",
                    action === "capture" ? result && result.captureToken || "" : value.capture,
                    result && result.reviewToken || "")
                return
            }
            if (!current(value, ticket)) return
            // Only the actual returned attempt can anchor an explicit retry,
            // including a committed record followed by a failed final fence.
            if ((action === "review" || action === "rereview") && result && token(result.reviewAttemptToken)
                    && status.reviewAttemptToken === result.reviewAttemptToken && status.proposalToken === value.proposal)
                g.attempt = result.reviewAttemptToken
            var malformed = !result || typeof result.ok !== "boolean" || typeof result.current !== "boolean"
                || typeof result.committed !== "boolean" || result.flightCommandReleased !== false
            if (malformed || !result.ok || !result.current) {
                var failureText = malformed ? qsTr("回执不完整，不能断言动作未发生。请核对本地记录，不自动重试。")
                    : qsTr("未完成：%1；本地动作可能已发生：%2。请核对记录后重新确认。")
                        .arg(text(result.error)).arg(result.committed ? qsTr("是") : qsTr("否"))
                if (result && result.priorReviewRetired === true)
                    failureText += qsTr(" 原审阅的采用／本地批准资格已退休，记录%1仍保留；不会自动恢复旧批准。").arg(text(result.priorRecordId))
                if (action === "preview") failureText += qsTr(" 本次参数依据按一次使用退休；修改或重试前须重新明确读取参数，不自动重试。")
                // A local commit followed by a failed final fence is not a
                // usable capability. Release only IDs this call returned,
                // never a shared latest ID or an archived review context.
                var failedCapture = action === "capture" && result && typeof result.captureToken === "string" ? result.captureToken : ""
                var failedRequest = (action === "review" || action === "rereview") && result && typeof result.requestId === "string" ? result.requestId : ""
                cancel(value.owner, failedRequest, failedCapture, "")
                if (g.alive && operationCurrent(value, ticket)) publish()
                if (g.alive && operationCurrent(value, ticket)) message = failureText
                return
            }
            if (action === "capture") {
                if (!token(result.captureToken) || capture.kind !== "reserve-dispatch" || capture.token !== result.captureToken
                        || capture.current !== true || JSON.stringify(capture.binding || {}) !== value.candidateStamp) return
                g.owner = value.owner; g.episode = value.episode; g.capture = result.captureToken
                g.proposal = ""; g.review = ""; g.retainedReview = ""; g.pending = ""; g.reviewed = ""; g.attempt = ""
                reserveIndex = -1
                if (!g.alive || !operationCurrent(value, ticket)) return
                message = qsTr("已冻结原完整备用方案。原审阅已退休；请选择备用机，明确读取参数依据并用向导生成新配置，不接管原任务。")
            } else if (action === "preview") {
                if (!token(result.proposalToken) || proposal.kind !== "reserve-dispatch" || proposal.token !== result.proposalToken
                        || proposal.captureToken !== value.capture) return
                g.proposal = result.proposalToken; g.review = ""; g.reviewed = ""; g.pending = ""; g.retainedReview = ""; g.attempt = ""
                message = qsTr("独立单机提案已生成。核对全部摘要、去返路线和新配置后，再单独提交当前审阅。")
            } else if (action === "review" || action === "rereview") {
                if (!token(result.reviewAttemptToken) || g.attempt !== result.reviewAttemptToken) {
                    message = qsTr("没有本次实际审阅attempt回执，不能认领共享结果。请核对记录后再明确操作。")
                    return
                }
                g.review = ""; g.retainedReview = ""; g.pending = ""; g.pendingProposal = ""
                g.reviewed = value.proposal
                if (result.planningPending === true && token(result.requestId)) {
                    g.pending = result.requestId; g.pendingProposal = value.proposal; g.pendingEpoch = value.epoch
                    message = qsTr("已提交一次新的完整后台审阅；尚无通过结论，可精确取消本次请求。")
                } else if (token(result.reviewToken) && newReview.token === result.reviewToken
                        && newReview.proposalToken === value.proposal && (newReview.archive || {}).recordId === result.recordId) {
                    g.review = result.reviewToken; g.retainedReview = result.reviewToken
                    message = qsTr("新的审阅已归档。记录存在不等于通过、审批、上传或起飞；核对原因后再明确采用。")
                } else message = qsTr("已返回本地记录，但没有本页可采用的完整对应回执。请核对记录，不认领共享区其它结果。")
            } else if (action === "accept") message = qsTr("已明确采用为唯一共享部署来源。仍须本页单独申请／批准，以及共享部署区的独立上传确认；没有发出飞行命令。")
            else if (action === "request") message = qsTr("已申请本地人工计划批准；尚未批准，也未上传或启动。请核对新状态后单独明确批准。")
            else message = qsTr("已返回本地批准回执。只有当前状态为approved才可继续共享部署区的独立上传流程；没有发出飞行命令。")
            if (g.alive && operationCurrent(value, ticket)) publish()
        } catch (error) {
            if (g.alive && !ticket.cancelled && g.epoch === value.epoch && value.owner === controller)
                message = qsTr("操作回执中断；本地保存／采用事实不会自动回滚。请核对共享区和记录，不自动重试。")
        }
        finally {
            // Native preview burns this exact offer once admitted, including
            // later failure. Retire only our matching local offer after all
            // result/context checks, never before the call or a later offer.
            if (previewCalled && g.alive && profileOffer === value.offer && adoptedProfile === value.profileBinding) {
                profileOffer = null
                if (g.alive && adoptedProfile === value.profileBinding) adoptedProfile = null
            }
            if (g.alive) callBusy = false
            if (g.ticket === ticket) g.ticket = null
        }
        if (g.alive && !ticket.cancelled && g.epoch === value.epoch) reconcile()
    }
    function exitFlow(value) {
        if (!value || !life.alive || life.ticket || callBusy || value.owner !== controller
                || value.epoch !== life.epoch || value.episode !== episodeToken || state.busy === true
                || life.owner !== value.owner || !life.capture || value.capture !== life.capture) return
        var g = life, ticket = {cancelled: false, action: "exit"}, result = null
        // A failed re-review may already have retired its prior context.
        // Exit the exact retained source when that old token no longer names
        // this shared context; never guess a replacement review token.
        var captureId = g.capture, reviewId = newReview.token === g.retainedReview ? g.retainedReview : ""
        g.ticket = ticket
        callBusy = true
        try {
            if (!g.alive || ticket.cancelled || g.epoch !== value.epoch || g.owner !== value.owner) return
            var stopRefresh = detachedRefreshCancel, refreshing = g.refreshJob
            if (refreshing) stopRefresh(refreshing)
            if (!g.alive || ticket.cancelled || g.epoch !== value.epoch || g.owner !== value.owner) return
            result = reviewId ? value.owner.clearFleetMissionRemainderReview(reviewId) : value.owner.clearFleetMissionRemainder(captureId)
            if (!g.alive || ticket.cancelled || g.epoch !== value.epoch) return
            if (result && result.ok === true && result.current === true && result.flightCommandReleased === false) {
                g.capture = ""; g.proposal = ""; g.review = ""; g.retainedReview = ""; g.pending = ""; g.reviewed = ""; g.attempt = ""; g.owner = null
                resetConsents()
                if (!g.alive || ticket.cancelled || g.epoch !== value.epoch || value.owner !== controller) return
                publish()
                if (g.alive && !ticket.cancelled && g.epoch === value.epoch && value.owner === controller)
                    message = qsTr("已退出本页本地流程，文字和归档保留。原任务、飞机动作及占用不变。")
            } else message = qsTr("未确认退出：%1。不会清理其它流程的新记录，请核对共享区。") .arg(text(result && result.error))
        } catch (error) {
            if (g.alive && !ticket.cancelled && g.epoch === value.epoch && value.owner === controller)
                message = qsTr("退出回执中断；不能据此撤销已经发生的本地事实，请核对记录。")
        } finally {
            if (g.alive) callBusy = false
            if (g.ticket === ticket) g.ticket = null
        }
    }
    function cancelPending(value) {
        var g = life
        if (!g.alive || !value || g.ticket || callBusy || value.owner !== controller || value.owner !== g.owner
                || value.epoch !== g.epoch || value.episode !== episodeToken || value.pending !== g.pending || !value.pending) return
        // Retire local eligibility first. Only the actual returned request ID
        // is cancelled; a newer shared job is never guessed or adopted.
        retire(false)
    }
    onCaptureConsentChanged: consentChanged()
    onPreviewConsentChanged: consentChanged()
    onReviewConsentChanged: consentChanged()
    onAdoptionConsentChanged: consentChanged()
    onRequestConsentChanged: consentChanged()
    onApprovalConsentChanged: consentChanged()
    onRereviewConsentChanged: consentChanged()
    onObserveConsentChanged: consentChanged()
    onRefreshConsentChanged: consentChanged()
    onNewReviewChanged: reconcile()
    onStatusChanged: reconcile()
    onControllerChanged: leaveOwner()
    onEpisodeTokenChanged: leaveOwner()
    onInputSettledChanged: if (!inputSettled) retire(false)
    onVisibleChanged: if (!visible) retire(false)
    onExpandedChanged: if (!expanded) retire(false)
    onEnabledChanged: if (!enabled) retire(false)
    Component.onDestruction: {
        var g = life, owner = g.owner, requestId = g.pending, captureId = g.capture, cancel = detachedCancel
        var refreshing = g.refreshJob, stopRefresh = detachedRefreshCancel
        if (refreshing) refreshing.cancellation.cancelled = true
        g.alive = false; if (g.ticket) g.ticket.cancelled = true
        g.pending = ""; stopRefresh(refreshing); cancel(owner, requestId, captureId, "")
    }

    QGCButton {
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: root.expanded ? qsTr("收起 · 备用机独立出勤") : qsTr("展开 · 备用机独立出勤")
        contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        onClicked: root.expanded = !root.expanded
    }
    ColumnLayout {
        Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("独立单机新任务：不会接管旧组，不恢复旧权限，不重演原待机。必须重新审阅当前能力、电池、净空和权限；本页不上传、不解锁、不启动。") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("来源任务 %1 · 计划 %2。来源可冻结：%3；不是执行许可。") .arg(root.text(root.candidate.sourceTaskId)).arg(root.text(root.candidate.planId)).arg(root.candidate.available === true ? qsTr("是") : qsTr("否 / 已退休")) }
        Controls.CheckBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            enabled: root.ready && !root.editingLocked && !root.captureCurrent && root.candidate.available === true && !(root.capture.token)
            checked: root.captureConsent; onClicked: root.captureConsent = checked
            text: qsTr("1 · 确认冻结当前完整备用方案并撤旧本地审阅；不是控制移交")
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            enabled: root.ready && root.captureConsent && !root.editingLocked && root.candidate.available === true && !(root.capture.token)
            text: qsTr("冻结当前来源")
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("capture")
            onCanceled: pressedIntent = null
            onEnabledChanged: if (!enabled) pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel { Layout.fillWidth: true; visible: !root.inputSettled; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("请先完成或取消其它输入流程；本页明确退出仍可使用。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.capture.token && !root.captureCurrent; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("共享区已有其它或已失效的来源。本页不会自动认领或清除它；请回到对应流程明确退出。") }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.captureCurrent; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("已冻结任务 %1 · 计划 %2。此处 current 仅表示保留槽及本部署scope仍对应，不代表旧审批仍有效。") .arg(root.text(root.capture.sourceTaskId)).arg(root.text(root.capture.planId)) }
            QGCLabel { Layout.fillWidth: true; text: qsTr("2 · 明确选择一架备用机（不默认选择）"); wrapMode: Text.WordWrap }
            Controls.ComboBox {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                model: root.reserveIds; currentIndex: root.reserveIndex
                enabled: !root.editingLocked && !root.callBusy && !root.refreshBusy
                displayText: root.selectedReserveId > 0 ? qsTr("备用机 ID %1").arg(root.selectedReserveId) : qsTr("请选择本次候选中的一架备用机")
                Accessible.name: qsTr("选择备用出勤机，不默认选择")
                onActivated: root.selectReserve(index)
            }
            QGCLabel {
                Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap
                text:qsTr("可选 · 当前依据缺失或过期时，单独确认刷新版本与固定29项参数，再另行读取519依据。只允许地面、未解锁／未飞行的真实PX4多旋翼；不写参数、不上传、不启动。120秒总期限、每阶段4秒，不因等待续期。")
            }
            Controls.CheckBox {
                Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
                enabled:root.refreshAllowed; checked:root.refreshConsent; onToggled:root.refreshConsent=checked
                text:qsTr("确认撤旧资格并刷新所选机"); Accessible.name:text
            }
            QGCLabel {
                Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap
                text:qsTr("撤销本页旧offer、提案、采用／批准资格及确认，保留同来源本页手填的动作／坐标和已明确采用的启动策略纯拟值及归档事实；新读取后仍须重新采用速度、确认并生成配置。未采用启动策略表单须先明确采用或放弃，刷新不会替您采用。")
            }
            QGCButton {
                Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
                enabled:root.refreshAllowed&&root.refreshConsent
                text:qsTr("显式刷新所选备用机版本和29项参数"); Accessible.name:text
                property var pressedIntent:null
                onPressed:pressedIntent=root.intent("refresh")
                onCanceled:pressedIntent=null
                onEnabledChanged:if(!enabled)pressedIntent=null
                onClicked:{var g=root.life,v=pressedIntent;pressedIntent=null;if(g.alive)root.startRefresh(v)}
                contentItem:QGCLabel{text:parent.text;font.underline:parent.activeFocus;color:parent._showHighlight?qgcPal.buttonHighlightText:qgcPal.buttonText;textFormat:Text.PlainText;wrapMode:Text.WordWrap;horizontalAlignment:Text.AlignHCenter}
            }
            QGCLabel {
                Layout.fillWidth:true; visible:!root.refreshAllowed; textFormat:Text.PlainText; wrapMode:Text.WordWrap
                text:profileEditor.startupPending?qsTr("启动策略仍有未采用子表单，不能刷新后默默清掉它。请先采用或明确放弃。")
                    : root.refreshBusy?qsTr("本页原刷新尚未结束或状态未知；先等待或精确取消，不并行读取、采用配置或生成提案。")
                    : root.refreshAuthorization.allowed!==true?qsTr("当前实际账号没有只读证据刷新权限；不能用配置读取的确认代替。")
                    : root.controller&&root.controller.fleetEvidenceRefresh&&root.controller.fleetEvidenceRefresh.active===true?qsTr("共享证据刷新服务正被占用；本页不会认领或取消其它页面请求。")
                    : qsTr("先选定本次备用机、结束其它编辑／后台审阅，并保持当前来源；地面和真实原生来源仍由后端独立检查。")
            }
            QGCLabel {
                Layout.fillWidth:true; visible:!!root.refreshJob; textFormat:Text.PlainText; wrapMode:Text.WrapAnywhere
                text:!root.refreshJob?"":!root.refreshJob.requestId?qsTr("本页刷新正在准备，尚无实际请求ID；关闭／取消会先退休本次意图，晚回ID只精确清理原请求。")
                    : qsTr("本页刷新 · 备用机 %1 · 原请求 %2 · 代次 %3\n当前归属对应：%4；状态：%5；当前参数：%6；剩余 %7 ms；完成 %8 / %9\n原因：%10\n只读请求可能仍在途：%11（不是实际发送／ACK／因果关联证明）。")
                        .arg(root.refreshJob.reserveId).arg(root.refreshJob.requestId).arg(root.refreshJob.generation)
                        .arg(root.refreshView.current===true?qsTr("是"):qsTr("否／历史"))
                        .arg(root.text((root.refreshView.status||{}).state)).arg(root.text((root.refreshView.status||{}).currentParameter))
                        .arg(root.integer((root.refreshView.status||{}).remainingMs,0,120000)?root.refreshView.status.remainingMs:qsTr("未知"))
                        .arg(root.integer((root.refreshView.status||{}).completedVehicleCount,0,1)?root.refreshView.status.completedVehicleCount:qsTr("未知"))
                        .arg(root.integer((root.refreshView.status||{}).totalVehicleCount,0,1)?root.refreshView.status.totalVehicleCount:qsTr("未知"))
                        .arg(root.text((root.refreshView.status||{}).reason||root.refreshView.error))
                        .arg((root.refreshView.status||{}).readOnlyRequestsMayRemainInflight===true?qsTr("是"):(root.refreshView.status||{}).readOnlyRequestsMayRemainInflight===false?qsTr("否／尚未尝试"):qsTr("未知"))
            }
            QGCLabel {
                Layout.fillWidth:true; visible:!!root.refreshJob&&!root.refreshBusy; textFormat:Text.PlainText; wrapMode:Text.WordWrap
                text:(root.refreshView.status||{}).state==="observations-renewed"
                    ?qsTr("本次有限刷新已采集完成；不保证此刻12项参数仍满足30秒新鲜度。请另点下方“读取本次参数依据”，再重新明确采用速度并生成配置；不会自动读取、审阅、批准或恢复旧权限。")
                    :qsTr("本次刷新已经结束或不再属于当前来源。部分回执仍是历史事实；取消／失败不撤回已发请求，不恢复旧依据或批准。可检查原因后重新明确操作。")
            }
            QGCButton {
                Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
                visible:!!root.refreshJob&&root.refreshBusy
                enabled:!!root.refreshJob&&(!root.callBusy||!root.refreshJob.requestId)
                text:qsTr("精确取消本页原刷新（不撤回已发）"); Accessible.name:text
                property var pressedIntent:null
                onPressed:pressedIntent=root.refreshCancelIntent()
                onCanceled:pressedIntent=null
                onEnabledChanged:if(!enabled)pressedIntent=null
                onClicked:{var g=root.life,v=pressedIntent;pressedIntent=null;if(g.alive)root.cancelRefresh(v)}
                contentItem:QGCLabel{text:parent.text;font.underline:parent.activeFocus;color:parent._showHighlight?qgcPal.buttonHighlightText:qgcPal.buttonText;textFormat:Text.PlainText;wrapMode:Text.WordWrap;horizontalAlignment:Text.AlignHCenter}
            }
            Controls.CheckBox { Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;enabled:root.ready&&!root.editingLocked&&root.selectedReserveId>0;checked:root.observeConsent;onToggled:root.observeConsent=checked;text:qsTr("确认仅读取所选机当前依据");Accessible.name:text }
            QGCLabel {Layout.fillWidth:true;text:qsTr("只读当前实际参数缓存，不请求参数刷新、不写参数；先撤本页旧依据，成功后退休旧提案资格；失败不恢复依据，原提案／归档事实保留。没有旧主机profile、手填hash或自动选择的备用机。");textFormat:Text.PlainText;wrapMode:Text.WordWrap}
            QGCButton {Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;enabled:root.ready&&!root.editingLocked&&root.observeConsent&&root.selectedReserveId>0&&!profileEditor.startupPending;text:qsTr("读取本次参数依据");Accessible.name:text;property var pressedIntent:null;onPressed:pressedIntent=root.intent("observe");onCanceled:pressedIntent=null;onClicked:{var g=root.life,v=pressedIntent;pressedIntent=null;if(g.alive)root.observeProfile(v)} contentItem:QGCLabel{text:parent.text;font.underline:parent.activeFocus;color:parent._showHighlight?qgcPal.buttonHighlightText:qgcPal.buttonText;textFormat:Text.PlainText;wrapMode:Text.WordWrap;horizontalAlignment:Text.AlignHCenter}}
            QGCLabel {Layout.fillWidth:true;visible:profileEditor.startupPending;text:qsTr("启动策略仍有未采用表单；先继续编辑并明确采用或放弃，再重新读取参数。重读不是采用策略。");textFormat:Text.PlainText;wrapMode:Text.WordWrap}
            FleetReserveDispatchProfileEditor {
                id:profileEditor
                Layout.fillWidth:true
                offer:root.profileOfferCurrent?root.profileOffer.dto:({})
                sessionToken:root.profileOfferCurrent?root.profileOffer:null
                sourceIdentity:root.profileSourceIdentity
                profileText:root.profileText
                editingEnabled:root.ready&&!root.editingLocked&&root.profileOfferCurrent
                onInputEdited:if(expectedSession===root.profileOffer&&!root.callBusy&&!root.editingLocked)root.retire(true)
                onProfileEdited:root.adoptProfile(nextText,expectedSession,expectedOriginalText)
                onFocusVisibilityRequested:root.focusVisibilityRequested(item)
            }
            QGCLabel {Layout.fillWidth:true;text:qsTr("本页生成的完整JSON（只读，最多16384字符；旧文字不表示当前依据或已采用）\n%1").arg(root.profileText||qsTr("尚未生成"));textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere}
            QGCLabel { Layout.fillWidth: true; visible: !!root.profileError; text: root.profileError; color: qgcPal.colorOrange; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            Controls.CheckBox {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: root.previewConsent; onClicked: root.previewConsent = checked; enabled: root.ready && !root.editingLocked
                text: qsTr("确认创建独立单机新任务、不接管原组、不重演旧待机，并重新审核当前电池及完整新profile")
                contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("生成独立出勤预览"); enabled: root.ready && root.previewConsent && !root.editingLocked && root.profileReady
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("preview")
                onCanceled: pressedIntent = null
                onEnabledChanged: if (!enabled) pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
                contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
        }
        QGCLabel {
            Layout.fillWidth:true; visible:!!root.refreshJob&&!root.captureCurrent; textFormat:Text.PlainText; wrapMode:Text.WrapAnywhere
            text:!root.refreshJob?"":qsTr("保留的本页刷新历史 · 备用机 %1 · 原请求 %2 · 代次 %3\n当前归属：否；最后已知阶段 %4；当前匹配原job在途：%5；快照当时active：%6\n%7\n来源已变或退出，不据此恢复旧确认；已发只读材料不回滚，也不会认领新scope请求。")
                .arg(root.refreshJob.reserveId).arg(root.text(root.refreshJob.requestId)).arg(root.text(root.refreshJob.generation))
                .arg(root.text((root.refreshView.status||{}).state)).arg(root.refreshView.active===true?qsTr("是"):root.refreshView.active===false?qsTr("否"):qsTr("未知"))
                .arg((root.refreshView.status||{}).active===true?qsTr("是／历史"):((root.refreshView.status||{}).active===false?qsTr("否／历史"):qsTr("未知")))
                .arg(root.text(root.refreshView.error))
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.proposalCurrent || root.retainedReviewVisible; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; text: root.proposalCurrent ? qsTr("3 · 本页提案的完整摘要与实际新路线") : qsTr("本页保留审阅资料（只读；没有持续核验或自动恢复确认）"); wrapMode: Text.WordWrap }
            QGCLabel { Layout.fillWidth: true; visible: root.proposalCurrent; text: root.pretty(root.summary); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            RowLayout {
                Layout.fillWidth: true
                QGCButton { Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || checked || activeFocus; text: qsTr("出航路线"); onClicked: { root.routeMode = 0; root.routePage = 0 } }
                QGCButton { Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || checked || activeFocus; text: qsTr("自身返程"); onClicked: { root.routeMode = 1; root.routePage = 0 } }
            }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("%1 · %2 点 · 第 %3 / %4 页（每页8点）。高度为AMSL，时间仅估算。") .arg(root.routeMode === 0 ? qsTr("自身出航") : qsTr("自身返程")).arg(root.shownRoute.length || 0).arg(root.currentPage + 1).arg(root.pageCount) }
            Repeater {
                model: root.shownRoute.length <= 2048 ? root.shownRoute.slice(root.currentPage * 8, root.currentPage * 8 + 8) : []
                QGCLabel { Layout.fillWidth: true; text: root.pretty(modelData); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            }
            RowLayout {
                Layout.fillWidth: true
                QGCButton { Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || checked || activeFocus; text: qsTr("上一页"); enabled: root.currentPage > 0; onClicked: root.routePage = root.currentPage - 1 }
                QGCButton { Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || checked || activeFocus; text: qsTr("下一页"); enabled: root.currentPage + 1 < root.pageCount; onClicked: root.routePage = root.currentPage + 1 }
            }
            QGCLabel { Layout.fillWidth: true; text: qsTr("实际新执行配置（只读）\n%1").arg(root.pretty(root.shownMember.executionProfile || {})); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            Controls.CheckBox {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: root.reviewConsent; onClicked: root.reviewConsent = checked
                enabled: root.ready && root.previewComplete && !root.editingLocked && !root.attemptCurrent && root.owned.reviewed !== root.owned.proposal
                text: qsTr("核对完整摘要、去返路线和新配置，确认发起一次新的当前能力／能量／净空审阅")
                contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("提交一次新的完整审阅"); enabled: root.ready && root.reviewConsent && root.previewComplete && !root.editingLocked && !root.attemptCurrent && root.owned.reviewed !== root.owned.proposal
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("review")
                onCanceled: pressedIntent = null
                onEnabledChanged: if (!enabled) pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
                contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
        }
        QGCLabel { Layout.fillWidth: true; visible: root.planningBusy; text: qsTr("本页的完整新任务正在后台审阅。关闭／收起将精确取消这次请求；记录事实不自动回滚。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        QGCLabel { Layout.fillWidth: true; visible: !!root.owned.pending && root.status.requestId === root.owned.pending && !root.planningBusy; text: qsTr("本次请求状态：%1；%2。若未形成对应完整记录，可精确取消／退出后重新明确操作，不认领其它结果。") .arg(root.text(root.status.error)).arg(root.pretty(root.status.reasons || [])); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: !!root.owned.pending; enabled: !root.callBusy && !!root.owned.pending
            text: qsTr("精确取消本页后台审阅（不删除归档）")
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("cancel")
            onCanceled: pressedIntent = null
            onEnabledChanged: if (!enabled) pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.cancelPending(value) }
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel { Layout.fillWidth: true; visible: root.retainedReviewVisible; text: qsTr("新记录 %1 · 审阅 %2 · 能量 %3\n%4") .arg(root.text((root.newReview.archive || {}).recordId)).arg(root.text(root.status.reviewStatus)).arg(root.text(root.status.energyStatus)).arg(root.pretty(root.status.reasons || [])); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
        FleetReserveDispatchReviewPanel {
            Layout.fillWidth: true
            visible: root.retainedReviewVisible
            reviewContext: root.retainedReviewVisible ? root.newReview : ({})
            onFocusVisibilityRequested: root.focusVisibilityRequested(item)
        }
        Controls.CheckBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            checked: root.adoptionConsent; onClicked: root.adoptionConsent = checked
            enabled: root.ready && root.reviewCurrent && !root.editingLocked
            text: qsTr("4 · 明确采用本页对应新记录为唯一共享部署来源；不是审批、上传或飞行许可")
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("明确采用到共享部署区"); enabled: root.ready && root.reviewCurrent && root.adoptionConsent && !root.editingLocked
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("accept")
            onCanceled: pressedIntent = null
            onEnabledChanged: if (!enabled) pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.ownAccepted; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("5 · 本页已采用记录的本地批准状态：%1；当前允许申请／批准：%2。若被阻断，请核对上方审阅、能量及原因；本页不绕过门禁。")
                .arg(root.text(root.approvalState)).arg(root.newReview.approvalAllowed === true ? qsTr("是") : qsTr("否 / 未确认"))
        }
        Controls.CheckBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.ownAccepted; checked: root.requestConsent; onClicked: root.requestConsent = checked
            enabled: root.ready && root.ownAccepted && root.newReview.current === true && root.newReview.approvalAllowed === true && root.approvalState === "preview"
            text: qsTr("确认仅为本页已采用的当前记录申请本地人工计划批准；不是上传或启动")
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.ownAccepted; text: qsTr("申请本地批准")
            enabled: root.ready && root.ownAccepted && root.requestConsent && root.newReview.current === true && root.newReview.approvalAllowed === true && root.approvalState === "preview"
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("request")
            onCanceled: pressedIntent = null
            onEnabledChanged: if (!enabled) pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        Controls.CheckBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.ownAccepted; checked: root.approvalConsent; onClicked: root.approvalConsent = checked
            enabled: root.ready && root.ownAccepted && root.newReview.current === true && root.newReview.approvalAllowed === true && root.approvalState === "awaiting-approval"
            text: qsTr("再次核对本页对应记录，明确批准当前本地人工计划；共享部署仍须独立上传确认")
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.ownAccepted; text: qsTr("明确批准本地计划")
            enabled: root.ready && root.ownAccepted && root.approvalConsent && root.newReview.current === true && root.newReview.approvalAllowed === true && root.approvalState === "awaiting-approval"
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("approve")
            onCanceled: pressedIntent = null
            onEnabledChanged: if (!enabled) pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.attemptCurrent; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("需要更新证据或审阅失败后，可明确保留同一提案重新审阅。不重选飞机、不修改profile或原政策；先撤旧采用／本地批准，再生成新的实际审阅。旧记录保留，新结果仍须重新采用和批准。没有自动刷新、重试或飞机动作。")
        }
        Controls.CheckBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.attemptCurrent; enabled: root.canRereview
            checked: root.rereviewConsent; onClicked: root.rereviewConsent = checked
            text: qsTr("确认撤旧采用／本地批准，保留原完整提案并重新审阅当前证据；不恢复旧权限")
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.attemptCurrent; enabled: root.canRereview && root.rereviewConsent
            text: qsTr("保留本提案，明确重新审阅")
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("rereview")
            onCanceled: pressedIntent = null
            onEnabledChanged: if (!enabled) pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("明确退出本页／撤销本页采用（保留文字和归档）")
            enabled: !root.callBusy && !!root.owned.capture && root.state.busy !== true
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("exit")
            onCanceled: pressedIntent = null
            onEnabledChanged: if (!enabled) pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.exitFlow(value) }
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.underline: parent.activeFocus; horizontalAlignment: Text.AlignHCenter; color: parent.pressed || parent.hovered ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel { id: feedback; Layout.fillWidth: true; text: root.message; visible: !!text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; activeFocusOnTab: true; Accessible.name: qsTr("备用出勤操作反馈"); Accessible.description: text }
    }
}

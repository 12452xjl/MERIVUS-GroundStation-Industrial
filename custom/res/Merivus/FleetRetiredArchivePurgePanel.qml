import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// The only input is the actual Root facade. Paths and value DTOs are display
// material, never a substitute for its private native backup basis/proof.
FocusScope {
    id: root
    property var workspace: null
    property bool sourceActive: true
    property bool expanded: false
    readonly property bool active: sourceActive && visible && enabled && expanded
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var state: workspace ? workspace.state || {} : ({})
    readonly property var nativePreview: workspace ? workspace.preview || {} : ({})
    readonly property var nativeResult: workspace ? workspace.result || {} : ({})
    readonly property bool stateValid: stateKnown(state)
    readonly property bool ready: active && lifetime.alive && !lifetime.exhausted && stateValid && !!workspace && typeof workspace.request === "function"
    // Display gates depend on notifying properties only. The actual entry
    // points separately enforce the plain ticket/retirement fence.
    readonly property bool canRequest: ready && !state.busy && !pendingJob && !callBusy
    readonly property bool canPreview: canRequest && state.basisReady && state.stage !== "unsupported" && state.stage !== "exhausted"
    readonly property bool previewCurrent: currentPreview(savedPreview)
    readonly property bool canCommit: canRequest && previewCurrent && backupConfirmed && deleteConfirmed
    readonly property bool canDiscard: canRequest && previewCurrent
    readonly property bool canCancel: active && stateValid && !!pendingJob && pendingJob.workspace === workspace && sameJob(pendingJob, state)
        && state.busy && ["running", "cancelling"].indexOf(state.stage) >= 0 && !callBusy
    property var lifetime: ({alive: true, epoch: 0, consent: 0, exhausted: false, ticket: null, retiring: false, cleanupRequested: false, dirty: false, draining: false, seen: null})
    property bool callBusy: false
    property bool backupConfirmed: false
    property bool deleteConfirmed: false
    property var savedPreview: null
    property var pendingJob: null
    property var ownResult: null
    property string feedback: ""

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: form.implicitHeight
    QGCPalette { id: pal; colorGroupEnabled: root.enabled }

    function map(v) { return !!v && typeof v === "object" && !Array.isArray(v) }
    function keys(v, names) {
        if (!map(v) || Object.keys(v).length !== names.length) return false
        for (var i = 0; i < names.length; ++i) if (!Object.prototype.hasOwnProperty.call(v, names[i])) return false
        return true
    }
    function text(v, maximum) { return typeof v === "string" && v.length <= maximum && v.indexOf("\u0000") < 0 }
    function uuid(v) { return typeof v === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(v) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(v) }
    function hash(v) { return typeof v === "string" && /^[0-9a-f]{64}$/.test(v) }
    function decimal(v) { return typeof v === "string" && /^(0|[1-9][0-9]{0,18})$/.test(v) && (v.length < 19 || v <= "9223372036854775807") }
    function fileIdentity(v) { return typeof v === "string" && /^windows-volume-file-index:[0-9a-f]{8}:[0-9a-f]{8}:[0-9a-f]{8}$/.test(v) }
    function directoryIdentity(v) { return fileIdentity(v) || (typeof v === "string" && /^posix-device-inode:(0|[1-9][0-9]{0,79}):(0|[1-9][0-9]{0,79})$/.test(v)) }
    function inert(v) { return v.historicalOnly === true && v.authenticated === false && v.permissionsRestored === false && v.executionPermit === false && v.flightCommandReleased === false }
    function commonKeys() { return ["historicalOnly", "authenticated", "signatureVerified", "permissionsRestored", "executionPermit", "flightCommandReleased", "executable", "completionVerified", "restoresAuthorization", "allRecordHashesVerified", "catalogCompletenessVerified", "crossSegmentAtomicSnapshot"] }
    function common(v) {
        if (!map(v) || v.historicalOnly !== true) return false
        var names = commonKeys()
        for (var i = 1; i < names.length; ++i) if (v[names[i]] !== false) return false
        return true
    }
    function descriptor(v, backup) {
        return keys(v, backup ? ["basename", "kind", "databaseEpoch", "sealRunId", "backupOperationId", "fileBytes", "fileFingerprint", "modifiedUtcMs"]
                    : ["basename", "kind", "databaseEpoch", "sealRunId", "fileBytes", "fileFingerprint", "modifiedUtcMs"])
            && v.kind === (backup ? "backup" : "retired") && uuid(v.databaseEpoch) && uuid(v.sealRunId)
            && (!backup || uuid(v.backupOperationId)) && decimal(v.fileBytes) && Number(v.fileBytes) >= 512 && Number(v.fileBytes) <= 4294967296
            && decimal(v.modifiedUtcMs) && v.modifiedUtcMs !== "0" && hash(v.fileFingerprint)
            && v.basename === "fleet-events-" + (backup ? "backup" : "retired") + "-" + v.databaseEpoch + "-" + v.sealRunId + (backup ? "-" + v.backupOperationId : "") + ".sqlite"
    }
    function clone(v) {
        var b = {nodes: 0, chars: 0}
        function copy(x, depth) {
            if (++b.nodes > 4096 || depth > 10) throw new Error("purge-value-limit")
            if (x === null || typeof x === "boolean") return x
            if (typeof x === "number") { if (!isFinite(x) || Math.abs(x) > 9007199254740991) throw new Error("purge-number"); return x }
            if (typeof x === "string") { b.chars += x.length; if (b.chars > 131072) throw new Error("purge-text-limit"); return x }
            if (!map(x)) throw new Error("purge-object")
            var names = Object.keys(x), out = {}
            if (names.length > 64) throw new Error("purge-map-limit")
            for (var i = 0; i < names.length; ++i) {
                var k = names[i]
                if (k.length > 128 || ["__proto__", "constructor", "prototype", "toJSON"].indexOf(k) >= 0) throw new Error("purge-key")
                b.chars += k.length; if (b.chars > 131072) throw new Error("purge-text-limit")
                out[k] = copy(x[k], depth + 1)
            }
            return out
        }
        try { return copy(v, 0) } catch (error) { return null }
    }
    function stamp(v) { try { return JSON.stringify(v) } catch (error) { return "" } }
    function stateKnown(v) {
        if (!keys(v, ["schemaVersion", "runId", "basisId", "basisReady", "source", "sourceDirectory", "backupDirectory", "backupBasename", "busy", "jobId", "action", "stage", "cancelRequested", "previewId", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
                || v.schemaVersion !== 1 || !inert(v) || !text(v.runId, 36) || (v.runId !== "" && !uuid(v.runId))
                || !text(v.basisId, 36) || (v.basisId !== "" && !uuid(v.basisId)) || !text(v.jobId, 36) || (v.jobId !== "" && !uuid(v.jobId))
                || !text(v.previewId, 36) || (v.previewId !== "" && !uuid(v.previewId)) || typeof v.basisReady !== "boolean" || typeof v.busy !== "boolean" || typeof v.cancelRequested !== "boolean"
                || ["", "preview", "commit"].indexOf(v.action) < 0 || ["idle", "basis-ready", "running", "cancelling", "prepared", "expired", "discarded", "complete", "failed", "cancelled", "unsupported", "exhausted"].indexOf(v.stage) < 0
                || !map(v.source) || !text(v.sourceDirectory, 4096) || !text(v.backupDirectory, 4096) || !text(v.backupBasename, 256)) return false
        if (Object.keys(v.source).length && !descriptor(v.source, false)) return false
        return !v.basisReady || (uuid(v.basisId) && descriptor(v.source, false) && v.sourceDirectory.length > 0 && v.backupDirectory.length > 0
            && /^fleet-events-backup-[0-9a-f-]+\.sqlite$/.test(v.backupBasename))
    }
    function previewKnown(v) {
        return keys(v, ["schemaVersion", "runId", "basisId", "previewId", "expiresAfterMs", "status", "canCommit", "source", "sourceDirectory", "backupDirectory", "backupBasename", "sourceFileIdentity", "backupFileIdentity", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
            && v.schemaVersion === 1 && inert(v) && text(v.runId, 36) && (v.runId === "" || uuid(v.runId)) && uuid(v.basisId) && uuid(v.previewId)
            && v.expiresAfterMs === 30000 && ["prepared", "expired", "discarded", "consumed", "retired"].indexOf(v.status) >= 0 && typeof v.canCommit === "boolean"
            && (!v.canCommit || v.status === "prepared") && descriptor(v.source, false) && text(v.sourceDirectory, 4096) && v.sourceDirectory.length > 0
            && text(v.backupDirectory, 4096) && v.backupDirectory.length > 0 && text(v.backupBasename, 256)
            && text(v.sourceFileIdentity, 256) && text(v.backupFileIdentity, 256)
            && (v.sourceFileIdentity === "" || fileIdentity(v.sourceFileIdentity)) && (v.backupFileIdentity === "" || fileIdentity(v.backupFileIdentity))
            && (!v.canCommit || (fileIdentity(v.sourceFileIdentity) && fileIdentity(v.backupFileIdentity) && v.sourceFileIdentity !== v.backupFileIdentity))
    }
    function receiptKnown(v, action) {
        return keys(v, ["schemaVersion", "action", "accepted", "jobId", "reason", "requestOnly", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
            && v.schemaVersion === 1 && v.action === action && typeof v.accepted === "boolean" && text(v.jobId, 36)
            && (v.jobId === "" || uuid(v.jobId)) && (!v.accepted || uuid(v.jobId)) && text(v.reason, 4096) && v.requestOnly === true && inert(v)
    }
    function resultKnown(v) {
        return keys(v, ["schemaVersion", "runId", "basisId", "jobId", "action", "ok", "error", "workerResult", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
            && v.schemaVersion === 1 && inert(v) && text(v.runId, 36) && (v.runId === "" || uuid(v.runId)) && uuid(v.basisId) && uuid(v.jobId)
            && ["preview", "commit"].indexOf(v.action) >= 0 && typeof v.ok === "boolean" && text(v.error, 4096) && map(v.workerResult)
            && (Object.keys(v.workerResult).length ? workerKnown(v.workerResult, v.action) && v.workerResult.operationId === v.jobId : !v.ok)
    }
    function backupBasisKnown(v) {
        if (!keys(v, commonKeys().concat(["format", "schemaVersion", "kind", "operationId", "source", "targetBasename", "expectedDirectoryIdentity", "observedDirectoryIdentity", "destinationDirectory", "destinationDirectoryIdentity", "decisionFinal", "targetCreateAttempted", "targetCreated", "mayHaveWritten", "bytesCopied", "copyCompleted", "backupVerified", "cancelObserved", "errorCode"]))
                || !common(v) || v.schemaVersion !== 1 || v.format !== "merivus-fleet-archive-maintenance-backup" || v.kind !== "backup" || !uuid(v.operationId)
                || !descriptor(v.source, false) || v.targetBasename !== "fleet-events-backup-" + v.source.databaseEpoch + "-" + v.source.sealRunId + "-" + v.operationId + ".sqlite"
                || !directoryIdentity(v.expectedDirectoryIdentity) || v.observedDirectoryIdentity !== v.expectedDirectoryIdentity || !directoryIdentity(v.destinationDirectoryIdentity)
                || !text(v.destinationDirectory, 4096) || v.destinationDirectory.length < 2 || v.decisionFinal !== true || v.targetCreateAttempted !== true || v.targetCreated !== true
                || v.mayHaveWritten !== true || v.bytesCopied !== v.source.fileBytes || v.copyCompleted !== true || v.backupVerified !== true || typeof v.cancelObserved !== "boolean" || v.errorCode !== "") return false
        return true
    }
    function proofKnown(v, basis) {
        return keys(v, ["sourceFileIdentity", "backupFileIdentity", "backupDescriptor"]) && fileIdentity(v.sourceFileIdentity) && fileIdentity(v.backupFileIdentity)
            && v.sourceFileIdentity !== v.backupFileIdentity && descriptor(v.backupDescriptor, true) && v.backupDescriptor.basename === basis.targetBasename
            && v.backupDescriptor.databaseEpoch === basis.source.databaseEpoch && v.backupDescriptor.sealRunId === basis.source.sealRunId
            && v.backupDescriptor.backupOperationId === basis.operationId && v.backupDescriptor.fileBytes === basis.source.fileBytes && v.backupDescriptor.fileFingerprint === basis.source.fileFingerprint
    }
    function workerKnown(v, action) {
        if (!keys(v, commonKeys().concat(["format", "schemaVersion", "kind", "operationId", "basis", "proof", "previewVerified", "observedDirectoryIdentity", "destinationDirectoryIdentity", "nativeDeleteSupported", "decisionFinal", "deleteAttempted", "deleteReportedSuccess", "mayHaveDeleted", "sourceAbsentObserved", "backupAfterVerified", "sourceHandleClosed", "backupHandleClosed", "cancelObserved", "errorCode", "diskSpaceReclaimedVerified", "sourceLogicalBytes", "receiptBasename", "receiptCreateAttempted", "receiptCreated", "intentWritten", "resultWritten", "receiptClosed", "receiptError"]))
                || !common(v) || v.schemaVersion !== 1 || v.format !== "merivus-fleet-archive-retired-purge" || v.kind !== (action === "preview" ? "purge-preview" : "purge-retired")
                || !uuid(v.operationId) || !backupBasisKnown(v.basis) || !map(v.proof) || !text(v.errorCode, 4096) || !text(v.receiptError, 4096)
                || !text(v.observedDirectoryIdentity, 256) || (v.observedDirectoryIdentity !== "" && !directoryIdentity(v.observedDirectoryIdentity))
                || !text(v.destinationDirectoryIdentity, 256) || (v.destinationDirectoryIdentity !== "" && !directoryIdentity(v.destinationDirectoryIdentity))
                || v.diskSpaceReclaimedVerified !== false || v.sourceLogicalBytes !== v.basis.source.fileBytes) return false
        var flags = ["previewVerified", "nativeDeleteSupported", "decisionFinal", "deleteAttempted", "deleteReportedSuccess", "mayHaveDeleted", "sourceAbsentObserved", "backupAfterVerified", "sourceHandleClosed", "backupHandleClosed", "cancelObserved", "receiptCreateAttempted", "receiptCreated", "intentWritten", "resultWritten", "receiptClosed"]
        for (var i = 0; i < flags.length; ++i) if (typeof v[flags[i]] !== "boolean") return false
        if (Object.keys(v.proof).length && !proofKnown(v.proof, v.basis)) return false
        if (v.previewVerified && (!v.nativeDeleteSupported || !proofKnown(v.proof, v.basis) || !v.sourceHandleClosed || !v.backupHandleClosed)) return false
        if (v.decisionFinal !== v.deleteAttempted || v.decisionFinal !== v.mayHaveDeleted || (v.deleteReportedSuccess && !v.deleteAttempted)
                || (v.sourceAbsentObserved && !v.decisionFinal) || (v.backupAfterVerified && !v.decisionFinal)
                || (v.receiptCreated && !v.receiptCreateAttempted) || (v.intentWritten && !v.receiptCreated) || (v.resultWritten && !v.intentWritten) || (v.receiptClosed && !v.receiptCreated)
                || (v.decisionFinal && (!v.intentWritten || !proofKnown(v.proof, v.basis) || !v.nativeDeleteSupported))
                || ((v.decisionFinal || v.previewVerified) && (v.observedDirectoryIdentity !== v.basis.expectedDirectoryIdentity || v.destinationDirectoryIdentity !== v.basis.destinationDirectoryIdentity))) return false
        if (action === "preview") return v.receiptBasename === "" && !v.decisionFinal && !v.deleteAttempted && !v.deleteReportedSuccess && !v.mayHaveDeleted
            && !v.sourceAbsentObserved && !v.backupAfterVerified && !v.receiptCreateAttempted && !v.receiptCreated && !v.intentWritten && !v.resultWritten && !v.receiptClosed && v.receiptError === ""
            && (v.previewVerified ? v.errorCode === "" : v.errorCode.length > 0 && Object.keys(v.proof).length === 0)
        return v.receiptBasename === "fleet-events-purge-" + v.operationId + ".jsonl" && proofKnown(v.proof, v.basis)
            && (v.errorCode.length > 0 || (v.decisionFinal && v.deleteReportedSuccess && v.sourceAbsentObserved && v.backupAfterVerified && v.previewVerified && v.resultWritten && v.receiptClosed && v.receiptError === ""))
    }
    function sameJob(job, v) { return job.id === v.jobId && job.intent.expectedBasisId === v.basisId && job.intent.expectedRunId === v.runId && job.intent.action === v.action }
    function currentPreview(p) {
        return !!p && ready && p.workspace === workspace && p.epoch === lifetime.epoch && stateValid && state.basisReady
            && p.dto.basisId === state.basisId && p.dto.runId === state.runId && p.jobId === state.jobId && p.dto.previewId === state.previewId
            && p.dto.status === "prepared" && p.dto.canCommit && stamp(p.dto) === p.stamp && stamp(nativePreview) === p.stamp && previewKnown(nativePreview)
    }
    function takeTicket() {
        var g = lifetime
        if (!g.alive || g.ticket || g.retiring) return null
        var ticket = {stale: false}, oldRoot = root
        g.ticket = ticket
        return {ticket: ticket, release: function() {
            if (g.alive && g.ticket === ticket) oldRoot.callBusy = false
            if (g.ticket === ticket) g.ticket = null
            if (g.alive) oldRoot.drain()
        }}
    }
    function advance(g, field) { if (g[field] >= 9007199254740991) g.exhausted = true; else ++g[field] }
    function clearConsents(g) {
        advance(g, "consent")
        backupConfirmed = false
        if (!g.alive) return false
        deleteConfirmed = false
        return g.alive
    }
    function detachedCleanup(job, p) {
        // Freeze before clearing any notifying property. This returned closure
        // never resolves a method/property on the Panel, even after deletion.
        var jobOwner = job ? job.workspace : null, previewOwner = p ? p.workspace : null
        var cancelIntent = job ? {action: "cancel", expectedBasisId: job.intent.expectedBasisId, expectedJobId: job.id, previewId: "", expectedRunId: job.intent.expectedRunId} : null
        var discardIntent = p ? {action: "discard", expectedBasisId: p.dto.basisId, expectedJobId: p.jobId, previewId: p.dto.previewId, expectedRunId: p.dto.runId} : null
        return function() {
            if (jobOwner && cancelIntent) { try { jobOwner.request(cancelIntent, true) } catch (error) {} }
            if (previewOwner && discardIntent) { try { previewOwner.request(discardIntent, true) } catch (error) {} }
        }
    }
    function retire(sendCleanup) {
        var g = lifetime
        if (!g.alive) return
        advance(g, "epoch")
        g.cleanupRequested = g.cleanupRequested || sendCleanup === true
        if (g.retiring) return
        var job = pendingJob, p = savedPreview, dispose = detachedCleanup(job, p), original = g.ticket, lease = null
        if (!original) lease = takeTicket()
        if (!original && !lease) return
        if (g.ticket) g.ticket.stale = true
        g.retiring = true
        try {
            if (!clearConsents(g)) return
            savedPreview = null
            if (!g.alive) return
            pendingJob = null
            if (!g.alive) return
        } finally {
            // Nested hide/destruction can request cleanup after the fields
            // have already been detached. Preserve that obligation in plain
            // state through the final callBusy notification as well.
            if (lease) lease.release()
            if (g.cleanupRequested) dispose()
            g.cleanupRequested = false
            g.retiring = false
            if (lease && g.alive) drain()
        }
    }
    function press(action) {
        if (lifetime.exhausted || lifetime.ticket || lifetime.retiring) return null
        if (action === "cancel" ? !canCancel : action === "commit" ? !canCommit : action === "discard" ? !canDiscard : !canPreview) return null
        return {workspace: workspace, epoch: lifetime.epoch, consent: lifetime.consent, preview: savedPreview, pending: pendingJob, basis: clone(state),
            stateStamp: stamp(state), backup: backupConfirmed, deletion: deleteConfirmed,
            receiptDirectory: action === "commit" && savedPreview ? savedPreview.dto.sourceDirectory : "",
            intent: {action: action, expectedBasisId: state.basisId, expectedJobId: state.jobId, previewId: action === "commit" || action === "discard" ? savedPreview.dto.previewId : "", expectedRunId: state.runId}}
    }
    function pressCurrent(p, consumed) {
        if (!p || !ready || lifetime.exhausted || p.workspace !== workspace || p.epoch !== lifetime.epoch || !stateValid || stamp(state) !== p.stateStamp) return false
        if (p.intent.action === "cancel") return p.pending === pendingJob && !!pendingJob && sameJob(pendingJob, state) && state.busy && ["running", "cancelling"].indexOf(state.stage) >= 0
        if (state.busy || pendingJob || !state.basisReady) return false
        if (p.intent.action === "preview") return state.stage !== "unsupported" && state.stage !== "exhausted"
        return p.preview === savedPreview && currentPreview(p.preview) && (consumed ? lifetime.consent === p.consumedConsent && !backupConfirmed && !deleteConfirmed
            : lifetime.consent === p.consent && (p.intent.action !== "commit" || (p.backup && p.deletion && backupConfirmed && deleteConfirmed)))
    }
    function submit(p) {
        if (!pressCurrent(p, false)) return
        var g = lifetime, lease = takeTicket()
        if (!lease) return
        var owner = p.workspace, originalIntent = p.intent, operation = null
        // The closure uses no QML method or property after destruction.
        function abandon(receipt) {
            try { if (receipt && receipt.accepted === true && typeof receipt.jobId === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(receipt.jobId))
                owner.request({action: "cancel", expectedBasisId: originalIntent.expectedBasisId, expectedJobId: receipt.jobId, previewId: "", expectedRunId: originalIntent.expectedRunId}, true) } catch (error) {}
        }
        try {
            callBusy = true
            if (!g.alive || lease.ticket.stale || !pressCurrent(p, false)) return
            if (p.intent.action !== "cancel") {
                if (!clearConsents(g)) return
                p.consumedConsent = g.consent
                if (!g.alive || lease.ticket.stale || !pressCurrent(p, true)) return
            }
            var receipt = null
            try { receipt = owner.request(originalIntent, true) } catch (error) { receipt = null }
            if (!g.alive) { if (["preview", "commit"].indexOf(originalIntent.action) >= 0) abandon(receipt); return }
            if (lease.ticket.stale || p.epoch !== g.epoch || owner !== workspace) { if (["preview", "commit"].indexOf(originalIntent.action) >= 0) abandon(receipt); return }
            var value = clone(receipt)
            if (!receiptKnown(value, originalIntent.action)) { feedback = qsTr("回执未知，可能已开始或已删除；不重试、不恢复确认。请核对下方原生历史事实。"); return }
            if (!value.accepted) { feedback = qsTr("请求未接受：%1。旧确认不恢复；核对事实后重新明确预览。").arg(value.reason || qsTr("未提供原因")); return }
            if (["preview", "commit"].indexOf(originalIntent.action) >= 0) {
                if (!uuid(value.jobId) || value.jobId === originalIntent.expectedJobId) { feedback = qsTr("没有取得新的原生job身份，结局未知；不自动重试。"); return }
                operation = {workspace: owner, epoch: p.epoch, id: value.jobId, intent: originalIntent, preview: p.preview, basis: p.basis, receiptDirectory: p.receiptDirectory}
                pendingJob = operation
                if (!g.alive || lease.ticket.stale || pendingJob !== operation || owner !== workspace) { abandon(value); return }
                savedPreview = null
                if (!g.alive || lease.ticket.stale || pendingJob !== operation) return
                feedback = qsTr("请求已接受；等待原job的结果与实际线程退出，不等同检查通过或删除完成。")
            } else if (value.jobId !== originalIntent.expectedJobId) {
                feedback = qsTr("同步回执没有对应原job，取消或作废结果未知；不认领别的请求。")
            } else if (originalIntent.action === "discard") {
                savedPreview = null
                if (g.alive) feedback = qsTr("已作废准确旧预览；未删除文件。需要重新明确预览及两项确认。")
            } else feedback = qsTr("仅已请求取消原job；决定后可能已经删除，不能撤回或据此声称文件仍在。")
        } finally { lease.release() }
    }
    function consentPress() { return !lifetime.exhausted && !lifetime.ticket && !lifetime.retiring && canRequest && previewCurrent ? {workspace: workspace, epoch: lifetime.epoch, consent: lifetime.consent, preview: savedPreview} : null }
    function confirm(which, checked, p) {
        var g = lifetime, lease = takeTicket()
        if (!lease) return
        try {
            if (!p || !canRequestInside(lease.ticket) || p.workspace !== workspace || p.epoch !== g.epoch || p.consent !== g.consent || p.preview !== savedPreview || !previewCurrent) return
            advance(g, "consent")
            if (g.exhausted) return
            if (which === "backup") backupConfirmed = checked === true
            else deleteConfirmed = checked === true
        } finally { lease.release() }
    }
    function canRequestInside(ticket) { return ready && !lifetime.exhausted && !state.busy && !pendingJob && lifetime.ticket === ticket && !ticket.stale && !lifetime.retiring }
    function changed() { var g = lifetime; if (!g.alive) return; g.dirty = true; drain() }
    function drain() {
        var g = lifetime
        if (!g.alive || g.ticket || g.retiring || g.draining) return
        g.draining = true
        try {
            if (g.dirty) {
                g.dirty = false
                var next = {workspace: workspace, basis: stateValid ? state.basisId : "?", run: stateValid ? state.runId : "?", job: stateValid ? state.jobId : "?"}, old = g.seen
                g.seen = next
                var different = !old || old.workspace !== next.workspace || old.basis !== next.basis || old.run !== next.run || old.job !== next.job
                if (different && !(pendingJob && pendingJob.workspace === workspace && stateValid && sameJob(pendingJob, state))) retire(false)
                else if (savedPreview && !currentPreview(savedPreview)) retire(false)
            }
            if (g.alive) settle()
        } finally { g.draining = false; if (g.alive && g.dirty) drain() }
    }
    function settle() {
        var g = lifetime, job = pendingJob
        if (!g.alive || g.ticket || g.retiring || !job || !stateValid || state.busy || !sameJob(job, state)) return
        // state/result/preview are separate bindings of one native signal. An
        // earlier binding must not consume an old result or old preview.
        if (!map(nativeResult) || nativeResult.jobId !== job.id || nativeResult.action !== job.intent.action || nativeResult.basisId !== job.intent.expectedBasisId || nativeResult.runId !== job.intent.expectedRunId) return
        var value = clone(nativeResult)
        if (!resultKnown(value)) return
        var facts = value.workerResult
        if (Object.keys(facts).length && (!job.basis || stamp(facts.basis.source) !== stamp(job.basis.source) || facts.basis.destinationDirectory !== job.basis.backupDirectory || facts.basis.targetBasename !== job.basis.backupBasename)) return
        var p = null
        if (value.ok && job.intent.action === "preview") {
            if (!previewKnown(nativePreview) || nativePreview.basisId !== value.basisId || nativePreview.runId !== value.runId || nativePreview.previewId !== state.previewId) return
            if (!workerKnown(facts, "preview") || !facts.previewVerified || !facts.nativeDeleteSupported || !proofKnown(facts.proof, facts.basis)
                    || stamp(nativePreview.source) !== stamp(job.basis.source) || nativePreview.sourceDirectory !== job.basis.sourceDirectory
                    || nativePreview.backupDirectory !== job.basis.backupDirectory || nativePreview.backupBasename !== job.basis.backupBasename
                    || nativePreview.sourceFileIdentity !== facts.proof.sourceFileIdentity || nativePreview.backupFileIdentity !== facts.proof.backupFileIdentity) return
            p = clone(nativePreview)
        }
        var lease = takeTicket()
        if (!lease) return
        try {
            if (!g.alive || job !== pendingJob || job.workspace !== workspace || job.epoch !== g.epoch || !active || !stateValid || state.busy || !sameJob(job, state)) return
            ownResult = {workspace: job.workspace, dto: value, receiptDirectory: job.receiptDirectory || ""}
            if (!g.alive || lease.ticket.stale || pendingJob !== job) return
            if (p && p.status === "prepared" && p.canCommit) {
                savedPreview = {workspace: job.workspace, epoch: job.epoch, jobId: job.id, dto: p, stamp: stamp(p)}
                if (!g.alive || lease.ticket.stale || pendingJob !== job) return
            }
            pendingJob = null
            if (!g.alive || lease.ticket.stale) return
            feedback = value.ok ? job.intent.action === "preview" ? qsTr("只读检查已完成。核对原件和副本全路径、身份及hash，再分别确认；原30秒窗口不会续期。") : qsTr("原job已结束。请逐项核对删除调用、原件缺失和备份后核事实；不据此声称物理磁盘空间已释放。")
                : qsTr("原job结束但未确认成功：%1。可能已有文件效果；不恢复旧确认、不自动重试。").arg(value.error || qsTr("未提供原因"))
        } finally { lease.release() }
    }
    function path(directory, basename) { return directory + "/" + basename }
    function flag(v) { return v === true ? qsTr("是（本次观察）") : v === false ? qsTr("未确认该事实") : qsTr("未知") }
    function basisText() {
        if (!stateValid) return qsTr("原生状态未知，不能开始删除。")
        if (!descriptor(state.source, false)) return qsTr("尚无本次真实备份依据。先在上方读取清单，选择retired原件并新做一次外部备份，等待backupVerified与实际线程退出。已有目录诊断或JSON不能建立删除依据。")
        return qsTr("原生备份依据 %1 · 本次可再预览 %2\n原件：%3\n副本：%4\n数据库UUID %5 · 封存run UUID %6\n原件逻辑字节 %7 · SHA256 %8\n这是本地文件观察，不证明副本在异地、介质独立、所有事件真实或掉电耐久。").arg(state.basisId).arg(flag(state.basisReady)).arg(path(state.sourceDirectory, state.source.basename)).arg(path(state.backupDirectory, state.backupBasename)).arg(state.source.databaseEpoch).arg(state.source.sealRunId).arg(state.source.fileBytes).arg(state.source.fileFingerprint)
    }
    function previewText() {
        if (!savedPreview) return qsTr("尚无本页真实job完成的删除前检查。不会自动检查、选择或删除。")
        var p = savedPreview.dto
        return qsTr("本页预览 %1 · 当前仍可确认 %2\n只删除原件：%3\n保留副本：%4\n原件文件ID %5\n副本文件ID %6\n数据库UUID %7 · 封存run UUID %8\n字节 %9 · SHA256 %10\n两项确认只绑定这一份原预览；原生30秒过期、新依据、隐藏或换源后不复活。").arg(p.previewId).arg(flag(previewCurrent)).arg(path(p.sourceDirectory, p.source.basename)).arg(path(p.backupDirectory, p.backupBasename)).arg(p.sourceFileIdentity).arg(p.backupFileIdentity).arg(p.source.databaseEpoch).arg(p.source.sealRunId).arg(p.source.fileBytes).arg(p.source.fileFingerprint)
    }
    function describeResult(v, heading, receiptDirectory) {
        var message = heading + qsTr("\n原job %1 · basis %2 · 原run %3\n动作 %4 · 原生ok %5\n错误：%6\n").arg(v.jobId).arg(v.basisId).arg(v.runId || qsTr("空")).arg(v.action).arg(flag(v.ok)).arg(v.error || qsTr("无"))
        var w = v.workerResult
        if (!Object.keys(w).length) return message + qsTr("没有完整worker事实，文件效果未知；不能推断原件未删或副本未变。")
        message += qsTr("原件逻辑字节 %1（不是物理释放量）\n副本：%2\n原件文件ID %3 · 副本文件ID %4\n删除前双文件核对 %5\n删除决定 / 已进入删除调用 / 系统报告成功 / 可能已删：%6 / %7 / %8 / %9\n事后观察原件缺失 / 副本后核：%10 / %11\n原件句柄已关闭 / 副本句柄已关闭：%12 / %13\n观察到取消：%14\nworker错误：%15\n").arg(w.sourceLogicalBytes).arg(path(w.basis.destinationDirectory, w.basis.targetBasename))
            .arg(w.proof.sourceFileIdentity || qsTr("未知")).arg(w.proof.backupFileIdentity || qsTr("未知")).arg(flag(w.previewVerified)).arg(flag(w.decisionFinal)).arg(flag(w.deleteAttempted)).arg(flag(w.deleteReportedSuccess)).arg(flag(w.mayHaveDeleted))
            .arg(flag(w.sourceAbsentObserved)).arg(flag(w.backupAfterVerified)).arg(flag(w.sourceHandleClosed)).arg(flag(w.backupHandleClosed)).arg(flag(w.cancelObserved)).arg(w.errorCode || qsTr("无"))
        if (v.action === "commit") message += qsTr("删除审计收据 %1\n尝试创建 / 已创建 / 意图行完整写入并flush / 结果行完整写入并flush / 收据已关闭：%2 / %3 / %4 / %5 / %6\n收据错误：%7\n").arg(text(receiptDirectory, 4096) && receiptDirectory.length > 0 ? path(receiptDirectory, w.receiptBasename) : w.receiptBasename + qsTr("（原固定归档目录；此诊断仅知文件名，不猜路径）")).arg(flag(w.receiptCreateAttempted)).arg(flag(w.receiptCreated)).arg(flag(w.intentWritten)).arg(flag(w.resultWritten)).arg(flag(w.receiptClosed)).arg(w.receiptError || qsTr("无"))
        return message + qsTr("false只表示该事实未确认，不能证明没有删除。diskSpaceReclaimedVerified=false；收据write/flush/close不等于fsync、掉电耐久、认证或恢复权限。")
    }
    function resultText() { return ownResult ? describeResult(ownResult.dto, qsTr("本页认领的原job结果 · 只读历史，不是新的删除许可"), ownResult.receiptDirectory) : qsTr("尚无本页认领的原job结果。请求接受、共享槽空闲或旧值非空都不是成功。") }
    function nativeFactsText() {
        var value = clone(nativeResult)
        if (!resultKnown(value) || (ownResult && ownResult.dto.jobId === value.jobId)) return ""
        return describeResult(value, qsTr("原生上次结果诊断 · 非本页认领，不恢复预览、确认、选择或取消资格"), "")
    }
    function ensureFocusVisible(item) {
        var g = lifetime, margin = pad, p = item.parent
        for (var depth = 0; p && depth < 64; ++depth) {
            if (!g.alive) return
            if (p.contentItem && typeof p.contentY === "number" && typeof p.contentHeight === "number") {
                var point = item.mapToItem(p.contentItem, 0, 0)
                if (!g.alive) return
                var top = point.y - margin, bottom = point.y + item.height + margin
                if (top < p.contentY) p.contentY = Math.max(0, top)
                else if (bottom > p.contentY + p.height) p.contentY = Math.min(Math.max(0, p.contentHeight - p.height), bottom - p.height)
                return
            }
            p = p.parent
        }
    }

    onWorkspaceChanged: { var g = lifetime; retire(true); if (g.alive) changed() }
    onActiveChanged: { if (!active) retire(true); else changed() }
    onStateChanged: changed()
    onNativePreviewChanged: changed()
    onNativeResultChanged: changed()
    Component.onDestruction: { var g = lifetime, dispose = detachedCleanup(pendingJob, savedPreview); g.cleanupRequested = true; g.alive = false; dispose() }

    ColumnLayout {
        id: form
        width: parent.width; spacing: root.pad
        QGCButton { id: expandButton; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.expanded ? qsTr("收起独立永久删除retired原件") : qsTr("展开独立永久删除retired原件（危险）"); Accessible.name: text; onClicked: root.expanded = !root.expanded; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: expandButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: expandButton._showHighlight ? pal.buttonHighlightText : pal.buttonText } }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; text: qsTr("与旧移出/恢复不同：这里永久删除一个已经retired的原件，不可撤销。先新备份并读回核实 → 明确只读检查 → 核对两份文件 → 两项确认 → 提交。不会删除current/sealed、副本或部分文件，不批量清理、不自动重试或恢复任务。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: pal.warningText }
            QGCLabel { Layout.fillWidth: true; text: root.basisText(); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCLabel { Layout.fillWidth: true; text: root.stateValid ? qsTr("原生阶段 %1 · 共享维护槽占用 %2\n原purge job %3 · 动作 %4 · 已请求取消 %5\n仅Windows支持精确原生删除；unsupported不表示没有备份事实。").arg(root.state.stage).arg(root.flag(root.state.busy)).arg(root.state.jobId || qsTr("无")).arg(root.state.action || qsTr("无")).arg(root.flag(root.state.cancelRequested)) : qsTr("状态不完整；保持拒绝。"); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCButton { id: previewButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.canPreview; text: qsTr("明确只读检查原件与备份（不删除）"); Accessible.name: text; onPressed: press = root.press("preview"); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: previewButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: previewButton._showHighlight ? pal.buttonHighlightText : pal.buttonText } }
            QGCLabel { Layout.fillWidth: true; text: root.previewText(); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            Controls.CheckBox { id: backupCheck; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.canRequest && root.previewCurrent; checked: root.backupConfirmed; text: qsTr("我已核对保留副本及风险"); Accessible.name: qsTr("确认这一份原预览的备份路径及文件身份；知道完整读回不是异地备份、独立介质或掉电耐久保证"); onPressed: press = root.consentPress(); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press, c = checked; press = null; if (g.alive) root.confirm("backup", c, v); if (g.alive) checked = Qt.binding(function() { return root.backupConfirmed }) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: backupCheck.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: backupCheck.indicator.width + root.pad } }
            QGCLabel { Layout.fillWidth: true; text: qsTr("我已核对本次原件及保留副本全路径、UUID、SHA256及文件ID。副本当前可读不等于异地、长期留存或掉电可靠；后续恢复仍需原备份导入与独立restore，不恢复批准或任务。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            Controls.CheckBox { id: deleteCheck; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.canRequest && root.previewCurrent; checked: root.deleteConfirmed; text: qsTr("另确认永久删此一个原件"); Accessible.name: qsTr("独立确认永久删除这一份retired原件，不可撤销，不删除保留副本，不等于已释放物理磁盘空间"); onPressed: press = root.consentPress(); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press, c = checked; press = null; if (g.alive) root.confirm("delete", c, v); if (g.alive) checked = Qt.binding(function() { return root.deleteConfirmed }) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: deleteCheck.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: deleteCheck.indicator.width + root.pad } }
            QGCLabel { Layout.fillWidth: true; text: qsTr("永久删除不可回滚。取消仅请求停止原job，决定后可能已经删除；结果未知须保留副本和回执交工程师核对，禁止重复点击猜测恢复。逻辑文件字节不等于实际释放磁盘空间。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: pal.warningText }
            QGCButton { id: commitButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.canCommit; text: qsTr("明确永久删除已核对的一个retired原件"); Accessible.name: text; onPressed: press = root.press("commit"); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: commitButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: commitButton._showHighlight ? pal.buttonHighlightText : pal.buttonText } }
            QGCButton { id: discardButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.canDiscard; text: qsTr("只作废本页准确预览（不删除文件）"); Accessible.name: text; onPressed: press = root.press("discard"); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: discardButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: discardButton._showHighlight ? pal.buttonHighlightText : pal.buttonText } }
            QGCButton { id: cancelButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.canCancel; text: qsTr("仅请求取消本页原job（不能撤回删除）"); Accessible.name: text; onPressed: press = root.press("cancel"); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: cancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: cancelButton._showHighlight ? pal.buttonHighlightText : pal.buttonText } }
            QGCLabel { id: feedbackLabel; Layout.fillWidth: true; visible: !!root.feedback; activeFocusOnTab: visible; text: root.feedback; Accessible.name: text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: pal.warningText; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCLabel { Layout.fillWidth: true; text: root.resultText(); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCLabel { Layout.fillWidth: true; visible: text.length > 0; text: root.nativeFactsText(); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
        }
    }
}

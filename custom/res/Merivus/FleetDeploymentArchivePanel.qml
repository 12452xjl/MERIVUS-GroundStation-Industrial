import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Local object retention only. File material never becomes a live Episode.
FocusScope {
    id: root
    property var controller: null
    property bool sourceActive: true
    property bool expanded: false
    property var lifetime: ({alive: true, ticket: null, epoch: 0, consent: 0, exhausted: false, retiring: false,
        jobDirty: false, drainingJobs: false, seenJob: null})
    property bool callBusy: false
    property bool riskConfirmed: false
    property bool overwriteConfirmed: false
    property bool openConfirmed: false
    property var selection: null
    property var pendingFile: null
    property var pendingJob: null
    property var fileDialog: null
    property var history: null
    property var reportView: null
    property var archiveFacts: null
    property string feedback: ""
    readonly property var workspace: controller ? controller.deploymentArchive : null
    readonly property var state: workspace ? workspace.state || {} : ({})
    readonly property var result: workspace ? workspace.result || {} : ({})
    readonly property bool stateValid: stateKnown(state)
    readonly property string latestJobId: stateValid ? state.jobId : ""
    readonly property string lifecycleStamp: stateValid ? state.lifecycleEpoch + ":" + state.lifecycleRevision : ""
    readonly property bool active: lifetime.alive && !lifetime.exhausted && sourceActive && visible && enabled && expanded
    readonly property bool ready: active && !!workspace && typeof workspace.request === "function" && stateValid
    readonly property bool busy: stateValid && state.busy
    readonly property bool canRequest: ready && !busy && !callBusy && !pendingJob && !fileDialog
    readonly property bool selectionCurrent: selectedCurrent(selection)
    readonly property bool historyCurrent: !!history && history.owner === controller && history.workspace === workspace && summaryKnown(history.summary)
    readonly property bool fileCurrent: fileIntentCurrent(pendingFile)
    readonly property bool canCancel: ready && !callBusy && !!pendingJob
        && pendingJob.owner === controller && pendingJob.workspace === workspace && pendingJob.id === latestJobId
        && busy && state.stage !== "destroying" && !state.cancelRequested
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property int chunkEnd: reportView ? textEnd(reportView.text, reportView.offset) : 0
    implicitHeight: form.implicitHeight
    QGCPalette { id: pal; colorGroupEnabled: root.enabled }

    function map(v) { return v !== null && typeof v === "object" && !Array.isArray(v) }
    function own(v, k) { return map(v) && Object.prototype.hasOwnProperty.call(v, k) }
    function keys(v, names) { if (!map(v) || Object.keys(v).length !== names.length) return false; for (var i = 0; i < names.length; ++i) if (!own(v, names[i])) return false; return true }
    function integer(v, low, high) { return typeof v === "number" && isFinite(v) && Math.floor(v) === v && v >= low && v <= high }
    function text(v, max) { return typeof v === "string" && v.length <= max }
    function uuid(v) { return text(v, 36) && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(v) && v !== "00000000-0000-0000-0000-000000000000" }
    function digest(v) { return text(v, 64) && /^[0-9a-f]{64}$/.test(v) }
    function counter(v) { return text(v, 20) && /^[1-9][0-9]{0,19}$/.test(v) && (v.length < 20 || v <= "18446744073709551615") }
    function flag(v) { return v === true ? qsTr("是") : v === false ? qsTr("否") : qsTr("未知") }
    function rowKnown(v) { return keys(v, ["episodeId", "published", "current", "selectedForObservation", "retiring", "sourceRetained"]) && uuid(v.episodeId) && typeof v.published === "boolean" && typeof v.current === "boolean" && typeof v.selectedForObservation === "boolean" && typeof v.retiring === "boolean" && typeof v.sourceRetained === "boolean" }
    function stateKnown(v) {
        if (!keys(v, ["schemaVersion", "lifecycleEpoch", "lifecycleRevision", "busy", "jobId", "action", "stage", "cancelRequested", "lifetimeAllocationAttempts", "liveAllocationCount", "capacity", "capacityRemaining", "candidates", "historicalOnly", "permissionsRestored", "executionPermit", "flightCommandReleased"])
                || v.schemaVersion !== 1 || !uuid(v.lifecycleEpoch) || !counter(v.lifecycleRevision) || typeof v.busy !== "boolean"
                || (v.jobId !== "" && !uuid(v.jobId)) || !text(v.action, 32)
                || ["idle", "running", "cancelling", "destroying", "complete", "failed", "cancelled"].indexOf(v.stage) < 0
                || typeof v.cancelRequested !== "boolean" || !counter(v.lifetimeAllocationAttempts) || v.capacity !== 8
                || !integer(v.liveAllocationCount, 0, 8) || v.capacityRemaining !== 8 - v.liveAllocationCount
                || !Array.isArray(v.candidates) || v.candidates.length > 8 || v.historicalOnly !== true
                || v.permissionsRestored !== false || v.executionPermit !== false || v.flightCommandReleased !== false) return false
        var seen = {}
        for (var i = 0; i < v.candidates.length; ++i) { var r = v.candidates[i]; if (!rowKnown(r) || seen[r.episodeId]) return false; seen[r.episodeId] = true }
        return !v.busy || (uuid(v.jobId) && ["archive-retire", "open", "report"].indexOf(v.action) >= 0)
    }
    function selectable(v) { return rowKnown(v) && !v.current && !v.selectedForObservation && !v.retiring && v.sourceRetained }
    function selectedCurrent(v) {
        if (!v || !stateValid || v.owner !== controller || v.workspace !== workspace || v.stamp !== lifecycleStamp || !selectable(v.row)) return false
        for (var i = 0; i < state.candidates.length; ++i) if (state.candidates[i].episodeId === v.row.episodeId) return JSON.stringify(state.candidates[i]) === v.rowStamp
        return false
    }
    function summaryKnown(v) {
        return keys(v, ["archiveId", "episodeId", "lifecycleEpoch", "capturedAtUtcMs", "published", "retirementKind", "reportCount", "contentHash", "historicalOnly"])
            && uuid(v.archiveId) && uuid(v.episodeId) && uuid(v.lifecycleEpoch) && integer(v.capturedAtUtcMs, 1, 9007199254740991)
            && typeof v.published === "boolean" && ["closed-observations", "unused-native", "unpublished-pristine"].indexOf(v.retirementKind) >= 0
            && integer(v.reportCount, 0, 9) && digest(v.contentHash) && v.historicalOnly === true
    }
    function receiptKnown(v, action) {
        return keys(v, ["schemaVersion", "action", "accepted", "jobId", "reason", "requestOnly", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
            && v.schemaVersion === 1 && v.action === action && typeof v.accepted === "boolean" && (v.jobId === "" || uuid(v.jobId))
            && text(v.reason, 4096) && v.requestOnly === true && v.historicalOnly === true && v.authenticated === false
            && v.permissionsRestored === false && v.executionPermit === false && v.flightCommandReleased === false
    }
    function resultKnown(v, job) {
        return keys(v, ["schemaVersion", "jobId", "action", "ok", "error", "episodeId", "reply", "retirementCommitted", "episodeDestroyed", "capacityReclaimed", "retiredTaskCount", "historicalOnly", "permissionsRestored", "executionPermit", "flightCommandReleased"])
            && v.schemaVersion === 1 && v.jobId === job.id && v.action === job.intent.action && typeof v.ok === "boolean" && text(v.error, 4096)
            && (v.episodeId === "" || uuid(v.episodeId)) && map(v.reply) && typeof v.retirementCommitted === "boolean"
            && typeof v.episodeDestroyed === "boolean" && typeof v.capacityReclaimed === "boolean" && integer(v.retiredTaskCount, 0, 9)
            && v.historicalOnly === true && v.permissionsRestored === false && v.executionPermit === false && v.flightCommandReleased === false
    }
    function saveKnown(v, job) {
        return keys(v, ["archiveId", "contentHash", "path", "fileWriteAttempted", "fileWrittenKnown", "fileWritten", "readbackVerified", "readbackError", "fsyncVerified", "durable", "authenticated", "summary"])
            && uuid(v.archiveId) && digest(v.contentHash) && v.path === job.intent.filePath && typeof v.fileWriteAttempted === "boolean"
            && typeof v.fileWrittenKnown === "boolean" && (v.fileWritten === null || typeof v.fileWritten === "boolean")
            && (!v.fileWrittenKnown || typeof v.fileWritten === "boolean") && typeof v.readbackVerified === "boolean"
            && (!v.readbackVerified || (v.fileWrittenKnown && v.fileWritten === true)) && text(v.readbackError, 4096)
            && v.fsyncVerified === false && v.durable === false && v.authenticated === false && summaryKnown(v.summary)
            && v.summary.archiveId === v.archiveId && v.summary.contentHash === v.contentHash && v.summary.episodeId === job.intent.episodeId
    }
    function bump(name) { if (lifetime[name] >= 9007199254740991) { lifetime.exhausted = true; return false } lifetime[name] += 1; return true }
    function takeTicket() {
        var g = lifetime
        if (!g.alive || g.ticket || g.retiring) return null
        var t = {}; g.ticket = t
        return {ticket: t, release: function() {
            if (!g.alive) { if (g.ticket === t) g.ticket = null; return }
            if (g.ticket !== t) return
            callBusy = false
            if (g.ticket === t) g.ticket = null
            if (g.alive) drainLatestJobs()
        }}
    }
    function fail(message) {
        var g = lifetime; if (!g.alive) return
        feedback = message
        if (g.alive && active) { feedbackLabel.forceActiveFocus(); if (g.alive) ensureFocusVisible(feedbackLabel) }
    }
    function ensureFocusVisible(item) {
        var p = item ? item.parent : null
        while (p) { if (p.contentItem && typeof p.contentY === "number" && typeof p.contentHeight === "number") { var point = item.mapToItem(p.contentItem, 0, 0), top = point.y - pad, bottom = point.y + item.height + pad; if (top < p.contentY) p.contentY = Math.max(0, top); else if (bottom > p.contentY + p.height) p.contentY = Math.min(Math.max(0, p.contentHeight - p.height), bottom - p.height); return } p = p.parent }
    }
    function clearConsents() {
        var g = lifetime
        riskConfirmed = false; if (!g.alive) return false
        overwriteConfirmed = false; if (!g.alive) return false
        openConfirmed = false; return g.alive
    }
    function neutral(action) { return {action: action, expectedJobId: state.jobId, lifecycleEpoch: state.lifecycleEpoch, lifecycleRevision: state.lifecycleRevision, episodeId: "", filePath: "", archiveId: "", reportIndex: -1, overwriteConfirmed: false} }
    function cancelOriginal(owner, base, id) {
        if (!owner || !uuid(id)) return
        try { owner.request({action: "cancel", expectedJobId: id, lifecycleEpoch: base.lifecycleEpoch, lifecycleRevision: base.lifecycleRevision, episodeId: "", filePath: "", archiveId: "", reportIndex: -1, overwriteConfirmed: false}, true) } catch (error) {}
    }
    function retireInput(cancelJob, clearOwned, clearSelection) {
        var g = lifetime
        if (!g.alive) return
        if (g.retiring) {
            if (clearOwned) g.clearOwned = true
            if (clearSelection) g.clearSelection = true
            if (cancelJob) g.cancelOriginal = true
            return
        }
        var prior = g.ticket, ticket = prior || {}
        if (!prior) g.ticket = ticket
        g.retiring = true
        g.clearOwned = clearOwned === true
        g.clearSelection = clearSelection === true
        g.cancelOriginal = cancelJob === true
        var oldDialog = fileDialog, job = pendingJob
        try {
            if (!prior) callBusy = true
            if (!g.alive || g.ticket !== ticket) return
            bump("epoch")
            if (!clearConsents() || !g.alive) return
            pendingFile = null; if (!g.alive) return
            fileDialog = null; if (!g.alive) return
            if (oldDialog) { try { oldDialog.close(); oldDialog.destroy() } catch (error) {} }
            if (!g.alive) return
            // Let the last busy notification request a nested owner cleanup,
            // but keep admission closed until that cleanup is also complete.
            if (!prior) { callBusy = false; if (!g.alive) return }
            if (g.cancelOriginal && job) cancelOriginal(job.workspace, job.intent, job.id)
            if (!g.alive) return
            if (g.clearOwned || g.clearSelection) { selection = null; if (!g.alive) return }
            if (g.clearOwned) {
                history = null; if (!g.alive) return
                reportView = null; if (!g.alive) return
                pendingJob = null; if (!g.alive) return
            }
        } finally {
            g.retiring = false
            g.clearOwned = false; g.clearSelection = false; g.cancelOriginal = false
            if (!prior && g.ticket === ticket) g.ticket = null
        }
        if (!prior && g.alive) drainLatestJobs()
    }
    function selectRow(value) {
        if (!value || !canRequest || value.owner !== controller || value.workspace !== workspace || value.epoch !== lifetime.epoch || value.stamp !== lifecycleStamp || !selectable(value.row) || state.candidates.indexOf(value.row) < 0) return
        var g = lifetime, lease = takeTicket(); if (!lease) return
        try {
            retireInput(false); if (!g.alive || g.ticket !== lease.ticket || value.owner !== controller || value.workspace !== workspace || value.stamp !== lifecycleStamp) return
            selection = {owner: value.owner, workspace: value.workspace, stamp: value.stamp, row: value.row, rowStamp: JSON.stringify(value.row)}
            if (g.alive) feedback = qsTr("仅选中了旧本地组。是否已经闭合、能否安全回收仍由原生在保存后最后核对；不是停飞或任务完成证明。")
        } finally { lease.release() }
    }
    function rowIntent(row) { return canRequest && selectable(row) ? {owner: controller, workspace: workspace, epoch: lifetime.epoch, stamp: lifecycleStamp, row: row} : null }
    function fileIntentCurrent(v) { return !!v && ready && v.owner === controller && v.workspace === workspace && v.epoch === lifetime.epoch && (v.action !== "archive-retire" || (selectionCurrent && v.selection === selection && v.stamp === lifecycleStamp)) }
    function choosePath(action, pressed) {
        if (!pressed || !canRequest || pressed.owner !== controller || pressed.workspace !== workspace || pressed.epoch !== lifetime.epoch || (action === "archive-retire" && (!selectionCurrent || pressed.selection !== selection))) return
        var g = lifetime, lease = takeTicket(); if (!lease) return
        var owner = pressed.owner, manager = pressed.workspace, epoch = pressed.epoch, selected = selection, stamp = lifecycleStamp
        function same() { return g.alive && g.ticket === lease.ticket && active && owner === controller && manager === workspace && epoch === g.epoch && (action !== "archive-retire" || (selectionCurrent && selection === selected && stamp === lifecycleStamp)) }
        try {
            if (!clearConsents() || !same()) return
            pendingFile = null; if (!same()) return
            var ticket = {owner: owner, workspace: manager, epoch: epoch, action: action, selection: selected, stamp: stamp}, dlg = fileComponent.createObject(mainWindow, {ownerPanel: root, life: g, ticket: ticket, selectExisting: action === "open", title: action === "open" ? qsTr("选择独立历史部署归档（尚不读取）") : qsTr("选择旧部署归档路径（尚不写入）")})
            if (!g.alive) { if (dlg) dlg.destroy(); return }
            if (!dlg || !same()) { if (dlg) dlg.destroy(); return }
            fileDialog = dlg
            if (!g.alive || fileDialog !== dlg) { dlg.destroy(); return }
            if (action === "open") dlg.openForLoad(); else dlg.openForSave()
        } finally { lease.release() }
    }
    function fileChosen(dlg, path) {
        var g = lifetime, v = dlg.ticket
        if (!g.alive || fileDialog !== dlg || !active || !text(path, 4096) || !path.length || v.owner !== controller || v.workspace !== workspace || v.epoch !== g.epoch || (v.action === "archive-retire" && (!selectionCurrent || v.selection !== selection || v.stamp !== lifecycleStamp))) { if (g.alive && fileDialog === dlg) fileDialog = null; try { dlg.close(); dlg.destroy() } catch (error) {}; return }
        var lease = takeTicket(); if (!lease) return
        function same() { return g.alive && g.ticket === lease.ticket && active && v.owner === controller && v.workspace === workspace && v.epoch === g.epoch && (v.action !== "archive-retire" || (selectionCurrent && v.selection === selection && v.stamp === lifecycleStamp)) }
        try {
            fileDialog = null; if (!same()) return
            if (!clearConsents() || !same()) return
            pendingFile = {owner: v.owner, workspace: v.workspace, epoch: v.epoch, action: v.action, path: path, selection: v.selection, stamp: v.stamp}
        } finally { lease.release(); try { dlg.close(); dlg.destroy() } catch (error) {} }
    }
    function fileRejected(dlg) { var g = lifetime; if (g.alive && fileDialog === dlg) fileDialog = null; try { dlg.destroy() } catch (error) {} }
    function requestIntent(action, index) {
        if (!canRequest) return null
        var v = neutral(action), file = null
        if (action === "archive-retire") {
            if (!selectionCurrent || !fileCurrent || pendingFile.action !== action || !riskConfirmed || !overwriteConfirmed) return null
            v.episodeId = selection.row.episodeId; v.filePath = pendingFile.path; v.overwriteConfirmed = true; file = pendingFile
        } else if (action === "open") {
            if (!fileCurrent || pendingFile.action !== action || !openConfirmed) return null
            v.filePath = pendingFile.path; file = pendingFile
        } else if (action === "report") {
            if (!historyCurrent || !integer(index, 0, history.summary.reportCount - 1)) return null
            v.archiveId = history.summary.archiveId; v.reportIndex = index
        } else if (action === "clear-file") { if (!historyCurrent) return null; v.archiveId = history.summary.archiveId }
        else return null
        return {owner: controller, workspace: workspace, epoch: lifetime.epoch, consent: lifetime.consent, stamp: lifecycleStamp, intent: v, file: file, selection: selection, history: history}
    }
    function requestCurrent(v, consent) { return !!v && ready && v.owner === controller && v.workspace === workspace && v.epoch === lifetime.epoch && consent === lifetime.consent && state.jobId === v.intent.expectedJobId && v.stamp === lifecycleStamp && (!v.file || (pendingFile === v.file && fileCurrent)) && (v.intent.action !== "archive-retire" || (selectionCurrent && selection === v.selection)) && ((v.intent.action !== "report" && v.intent.action !== "clear-file") || (historyCurrent && history === v.history)) }
    function submit(v) {
        if (!v || !canRequest || !requestCurrent(v, v.consent)) return
        var fresh = requestIntent(v.intent.action, v.intent.reportIndex)
        if (!fresh || JSON.stringify(fresh.intent) !== JSON.stringify(v.intent)) return
        var g = lifetime, lease = takeTicket(), consent = v.consent
        if (!lease) return
        var owner = v.workspace, base = {lifecycleEpoch: v.intent.lifecycleEpoch, lifecycleRevision: v.intent.lifecycleRevision}
        function returnedCurrent() { return g.alive && g.ticket === lease.ticket && v.owner === controller && owner === workspace && v.epoch === g.epoch && active && consent === g.consent && !riskConfirmed && !overwriteConfirmed && !openConfirmed }
        // This closure uses only frozen plain values if Qt notifications delete us.
        function abandon(receipt) {
            if (receipt && receipt.accepted === true && typeof receipt.jobId === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(receipt.jobId)) {
                try { owner.request({action: "cancel", expectedJobId: receipt.jobId, lifecycleEpoch: base.lifecycleEpoch, lifecycleRevision: base.lifecycleRevision, episodeId: "", filePath: "", archiveId: "", reportIndex: -1, overwriteConfirmed: false}, true) } catch (error) {}
            }
        }
        try {
            callBusy = true; if (!g.alive || !requestCurrent(v, consent)) return
            if (riskConfirmed) consent += 1
            riskConfirmed = false; if (!g.alive || !requestCurrent(v, consent) || riskConfirmed) return
            if (overwriteConfirmed) consent += 1
            overwriteConfirmed = false; if (!g.alive || !requestCurrent(v, consent) || overwriteConfirmed) return
            if (openConfirmed) consent += 1
            openConfirmed = false; if (!g.alive || !requestCurrent(v, consent) || openConfirmed) return
            var receipt = owner.request(v.intent, true)
            if (!g.alive) { abandon(receipt); return }
            if (!returnedCurrent()) { abandon(receipt); return }
            if (!receiptKnown(receipt, v.intent.action) || !receipt.accepted) { fail(qsTr("请求未确认受理：%1。操作可能已经发生；核对原生状态，不自动重试。").arg(receipt && text(receipt.reason, 4096) ? receipt.reason : qsTr("未知回执"))); return }
            if (v.file && pendingFile === v.file) { pendingFile = null; if (!returnedCurrent()) { abandon(receipt); return } }
            if (v.intent.action === "clear-file") {
                if (receipt.jobId !== v.intent.expectedJobId || history !== v.history) return
                history = null; if (!g.alive) return
                reportView = null; if (g.alive) feedback = qsTr("已精确清除本地打开槽；未删除文件、未清后来归档，也不撤销已发生的容量回收。")
            } else {
                if (!uuid(receipt.jobId) || receipt.jobId === v.intent.expectedJobId) { fail(qsTr("未取得本次新job身份；结局未知，请核对原生状态。")); return }
                var job = {id: receipt.jobId, owner: v.owner, workspace: owner, epoch: v.epoch, intent: v.intent, history: v.history}
                pendingJob = job; if (!returnedCurrent() || pendingJob !== job) { abandon(receipt); return }
                if (v.intent.action === "open") {
                    history = null; if (!returnedCurrent() || pendingJob !== job) { abandon(receipt); return }
                    reportView = null; if (!returnedCurrent() || pendingJob !== job) { abandon(receipt); return }
                }
                feedback = qsTr("已受理原job。保存、回读、退休提交、实际销毁与回收容量分别核对；不会自动重试。")
            }
        } catch (error) { if (g.alive) fail(qsTr("本次调用结局未知。文件可能已写或本地退休可能已提交；不自动重试或伪造回滚。")) }
        finally { lease.release(); if (g.alive) settle() }
    }
    function cancelIntent() { return canCancel ? {owner: controller, workspace: workspace, job: pendingJob, epoch: lifetime.epoch, intent: neutral("cancel")} : null }
    function cancel(v) {
        if (!v || !canCancel || v.owner !== controller || v.workspace !== workspace || v.job !== pendingJob || v.epoch !== lifetime.epoch || v.intent.expectedJobId !== pendingJob.id) return
        var g = lifetime, lease = takeTicket(); if (!lease) return
        try { callBusy = true; if (!g.alive || v.job !== pendingJob || v.owner !== controller || v.workspace !== workspace || state.jobId !== v.intent.expectedJobId || state.stage === "destroying") return; var r = v.workspace.request(v.intent, true); if (!g.alive || v.owner !== controller || v.workspace !== workspace || v.job !== pendingJob || v.epoch !== g.epoch || !active) return; feedback = receiptKnown(r, "cancel") && r.accepted && r.jobId === v.job.id ? qsTr("只已请求取消原job；终结前仍忙。不会撤回文件写入，退休已提交后不能取消。") : qsTr("取消未确认或已过退休提交点；请看原job事实，不取消后来任务。") }
        finally { lease.release(); if (g.alive) settle() }
    }
    function settle() {
        var g = lifetime, job = pendingJob
        if (!g.alive || g.ticket || !job || job.owner !== controller || job.workspace !== workspace || !stateValid || busy || latestJobId !== job.id) return
        // state/result are separate QML bindings: wait, rather than consume an old result.
        if (!map(result) || result.jobId !== job.id || result.action !== job.intent.action) return
        var lease = takeTicket(); if (!lease) return
        function same() { return g.alive && g.ticket === lease.ticket && pendingJob === job && job.owner === controller && job.workspace === workspace && stateValid && !busy && latestJobId === job.id }
        function present() { return same() && active && job.epoch === lifetime.epoch }
        try {
            callBusy = true; if (!same()) return
            var value = result
            if (!resultKnown(value, job)) { if (job.intent.action === "archive-retire") { archiveFacts = {known: false, jobId: job.id, episodeId: job.intent.episodeId, path: job.intent.filePath}; if (!same()) return } if (present()) fail(qsTr("本次终态回执不完整。保留结局未知，不把缺失当未写入或未退休。")); return }
            if (job.intent.action === "archive-retire") {
                archiveFacts = {known: true, jobId: job.id, episodeId: job.intent.episodeId, path: job.intent.filePath, ok: value.ok, error: value.error, retirementCommitted: value.retirementCommitted, episodeDestroyed: value.episodeDestroyed, capacityReclaimed: value.capacityReclaimed, retiredTaskCount: value.retiredTaskCount, fileKnown: saveKnown(value.reply, job), file: value.reply}
                if (!same()) return
                if (present()) feedback = value.capacityReclaimed ? qsTr("原生确认旧组已实际销毁并回收本地槽。仅本地容量变化，不证明飞机已停或任务已完成。") : qsTr("未确认回收容量；请分别核对下方已写文件与退休事实。不要自动重试或假定回滚。")
            } else if (present()) {
                if (!value.ok) {
                    fail((job.intent.action === "open" ? qsTr("打开未成功：%1。未恢复旧历史操作资格；已发生的文件事实不回滚，请核对后另行明确打开。")
                        : qsTr("操作未成功：%1。保留原历史与文件事实；可核对输入后另行明确操作。")).arg(value.error))
                    return
                }
                if (job.intent.action === "open") {
                    if (!summaryKnown(value.reply)) { fail(qsTr("历史目录不完整，未采用；不从当前live组补材料。")); return }
                    history = {owner: job.owner, workspace: job.workspace, summary: value.reply, jobId: job.id}
                    if (!present()) return
                    reportView = null; if (!present()) return
                    feedback = qsTr("已打开独立历史目录；请选择一份报告才读取。文件内容与时间均是未认证声称，不恢复live、批准或权限。")
                } else if (job.intent.action === "report") {
                    var reply = value.reply
                    if (history !== job.history || !historyCurrent || !keys(reply, ["archiveId", "reportIndex", "jsonText"]) || reply.archiveId !== job.intent.archiveId || reply.reportIndex !== job.intent.reportIndex || !text(reply.jsonText, 2097152) || !reply.jsonText.length) { fail(qsTr("报告文本不对应所选归档及序号，未采用；不跳到其它报告或裁剪内容。")); return }
                    reportView = {archiveId: reply.archiveId, index: reply.reportIndex, text: reply.jsonText, offset: 0, offsets: []}
                    if (present()) feedback = qsTr("显示原生已验证的完整canonical报告文本；分片仅供阅读，不执行JSON或恢复任何对象。")
                }
            }
        } finally { if (same()) pendingJob = null; lease.release() }
    }
    function textEnd(value, offset) { var end = Math.min(value.length, offset + 2048); if (end < value.length && end > offset) { var c = value.charCodeAt(end - 1); if (c >= 0xd800 && c <= 0xdbff) end -= 1 } return end }
    function page(direction, pressed) {
        if (!pressed || !canRequest || pressed.owner !== controller || pressed.workspace !== workspace || pressed.epoch !== lifetime.epoch || pressed.view !== reportView || !historyCurrent) return
        var g = lifetime, lease = takeTicket(), v = reportView; if (!lease) return
        try { var offset, stack; if (direction > 0 && chunkEnd < v.text.length) { offset = chunkEnd; stack = v.offsets.concat([v.offset]) } else if (direction < 0 && v.offsets.length) { offset = v.offsets[v.offsets.length - 1]; stack = v.offsets.slice(0, -1) } else return; reportView = {archiveId: v.archiveId, index: v.index, text: v.text, offset: offset, offsets: stack} }
        finally { lease.release() }
    }
    function pageIntent() { return canRequest && reportView ? {owner: controller, workspace: workspace, epoch: lifetime.epoch, view: reportView} : null }
    function latestChanged() {
        var g = lifetime
        if (!g.alive) return
        // A notification inside request/cleanup is not discardable. Its job
        // may have replaced the native opened cache while an old UI ticket ran.
        g.jobDirty = true
        drainLatestJobs()
    }
    function drainLatestJobs() {
        var g = lifetime
        if (!g.alive || !g.jobDirty || g.ticket || g.retiring || g.drainingJobs) return
        var lease = takeTicket()
        if (!lease) return
        g.drainingJobs = true
        try {
            callBusy = true
            if (!g.alive || g.ticket !== lease.ticket) return
            var actual = {owner: controller, workspace: workspace, id: latestJobId}, seen = g.seenJob, old = pendingJob
            g.seenJob = actual
            g.jobDirty = false
            if (seen && seen.owner === actual.owner && seen.workspace === actual.workspace && seen.id === actual.id) return
            function current() { return g.alive && g.ticket === lease.ticket && actual.owner === controller
                && actual.workspace === workspace && actual.id === latestJobId && old === pendingJob }
            var owned = old && old.owner === actual.owner && old.workspace === actual.workspace && old.id === actual.id
            if (owned) {
                // The actual return receipt is installed before releasing the
                // caller's ticket. Delayed job bindings must not retire a
                // normal report request's own source or advance its UI epoch.
                // An own open, however, has replaced the native file slot even
                // when hide/consent changes later prevent adopting its result.
                if (old.intent.action === "open") {
                    history = null
                    if (!current()) return
                    reportView = null
                }
            } else {
                if (old && old.intent.action === "archive-retire"
                        && (!archiveFacts || archiveFacts.jobId !== old.id || archiveFacts.known !== true)) {
                    archiveFacts = {known: false, jobId: old.id, episodeId: old.intent.episodeId, path: old.intent.filePath}
                    if (!current()) return
                }
                // archiveId is a file claim, not an opened-slot identity. A
                // different caller can open another hash with the same UUID.
                // Retire the whole old browser under the existing clear fence.
                retireInput(false, true, true)
            }
        } finally {
            lease.release()
            g.drainingJobs = false
            if (g.alive) {
                if (g.jobDirty) drainLatestJobs()
                else settle()
            }
        }
    }
    function factsDescription(v) {
        if (!v) return qsTr("尚无本面板认领的归档 / 回收回执。")
        var message = qsTr("本次历史事实\njob %1\n旧组 %2\n原路径 %3\n").arg(v.jobId).arg(v.episodeId).arg(v.path)
        if (!v.known) return message + qsTr("结局未知；可能已写入或退休。请人工核对，不自动重试。")
        message += qsTr("退休已提交：%1；实际组销毁：%2；容量已回收：%3；退休任务记录数：%4\n原因：%5\n").arg(flag(v.retirementCommitted)).arg(flag(v.episodeDestroyed)).arg(flag(v.capacityReclaimed)).arg(v.retiredTaskCount).arg(v.error || qsTr("无"))
        return message + (v.fileKnown ? qsTr("尝试写：%1；文件已写：%2；回读匹配：%3（%4）\nfsync：否；durable：否；非来源认证。\n归档 %5；内容hash %6\n本机归档捕获UTC ms %7（不是任务结束时间）。").arg(flag(v.file.fileWriteAttempted)).arg(v.file.fileWrittenKnown ? flag(v.file.fileWritten) : qsTr("未知")).arg(flag(v.file.readbackVerified)).arg(v.file.readbackError || qsTr("无" )).arg(v.file.archiveId).arg(v.file.contentHash).arg(v.file.summary.capturedAtUtcMs) : qsTr("文件回执未知，不能据此断言文件未写入。"))
    }
    function factsText() { return factsDescription(archiveFacts) }
    function nativeFactsText() {
        var value = result
        if (!map(value) || value.action !== "archive-retire" || !uuid(value.jobId) || (archiveFacts && archiveFacts.jobId === value.jobId)) return ""
        // Diagnostic only: never install a pending job, selection or history
        // owner from the public last-result value.
        var job = {id: value.jobId, intent: {action: "archive-retire", episodeId: value.episodeId, filePath: map(value.reply) && text(value.reply.path, 4096) ? value.reply.path : ""}}
        if (!resultKnown(value, job)) return qsTr("原生上次归档结果形状未知；不作为操作依据。")
        return qsTr("原生上次归档结果 · 非本页认领、非当前飞行状态，不作操作依据\n") + factsDescription({known: true, jobId: value.jobId, episodeId: value.episodeId, path: job.intent.filePath || qsTr("回执未提供"), error: value.error, retirementCommitted: value.retirementCommitted, episodeDestroyed: value.episodeDestroyed, capacityReclaimed: value.capacityReclaimed, retiredTaskCount: value.retiredTaskCount, fileKnown: saveKnown(value.reply, job), file: value.reply})
    }
    onRiskConfirmedChanged: bump("consent")
    onOverwriteConfirmedChanged: bump("consent")
    onOpenConfirmedChanged: bump("consent")
    onLifecycleStampChanged: { var g = lifetime; retireInput(false, false, true); if (g.alive) settle() }
    onLatestJobIdChanged: latestChanged()
    onStateChanged: { var g = lifetime; drainLatestJobs(); if (g.alive) settle() }
    onResultChanged: settle()
    onActiveChanged: { if (!active) retireInput(true); else settle() }
    onControllerChanged: retireInput(true, true, true)
    Component.onDestruction: {
        var g = lifetime, job = pendingJob, oldDialog = fileDialog
        g.alive = false
        if (oldDialog) { try { oldDialog.close(); oldDialog.destroy() } catch (error) {} }
        if (job) { try { job.workspace.request({action: "cancel", expectedJobId: job.id, lifecycleEpoch: job.intent.lifecycleEpoch, lifecycleRevision: job.intent.lifecycleRevision, episodeId: "", filePath: "", archiveId: "", reportIndex: -1, overwriteConfirmed: false}, true) } catch (error) {} }
    }

    ColumnLayout {
        id: form
        width: parent.width; spacing: root.pad
        QGCButton { id: expandButton; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.expanded ? qsTr("收起旧部署归档 / 本地容量回收") : qsTr("展开旧部署归档 / 本地容量回收"); Accessible.name: text; primary: activeFocus; onClicked: root.expanded = !root.expanded; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: expandButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: expandButton._showHighlight ? pal.buttonHighlightText : pal.buttonText } }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; text: qsTr("1 选旧组 → 2 只选路径 → 3 另确认保存 / 回收。只有保存及回读后原生最后核对、实际销毁旧组，才回收本地容量。旧飞机可能仍在飞；这不是停止、完成、控制权转移或恢复批准。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: pal.warningText }
            QGCLabel { Layout.fillWidth: true; text: root.stateValid ? qsTr("本地占用 %1 / 8，可用 %2；终身分配尝试 %3（不回绕，不等于当前占用）。\n原生job %4 · 阶段 %5 · 忙 %6").arg(root.state.liveAllocationCount).arg(root.state.capacityRemaining).arg(root.state.lifetimeAllocationAttempts).arg(root.latestJobId || qsTr("无")).arg(root.state.stage).arg(root.flag(root.state.busy)) : qsTr("归档状态未知；不将未知当可回收。独立打开历史无需live飞机或账户。"); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCLabel { id: feedbackLabel; Layout.fillWidth: true; visible: !!root.feedback; activeFocusOnTab: visible; text: root.feedback; Accessible.name: text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: pal.warningText }
            Repeater {
                model: root.stateValid ? root.state.candidates : []
                ColumnLayout {
                    Layout.fillWidth: true; property var row: modelData
                    QGCLabel { Layout.fillWidth: true; text: qsTr("组 %1\n已发布 %2 · 当前任务 %3 · 当前监视 %4 · 正在退休 %5 · 原来源保留 %6").arg(parent.row.episodeId).arg(root.flag(parent.row.published)).arg(root.flag(parent.row.current)).arg(root.flag(parent.row.selectedForObservation)).arg(root.flag(parent.row.retiring)).arg(root.flag(parent.row.sourceRetained)); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                    QGCButton { id: selectButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest && root.selectable(parent.row); text: root.selectionCurrent && root.selection.row.episodeId === parent.row.episodeId ? qsTr("已选择此旧组（尚未归档）") : qsTr("选择此旧组（不是已闭证明）"); Accessible.name: text + " " + parent.row.episodeId; onPressed: press = root.rowIntent(parent.row); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.selectRow(v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: selectButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: selectButton._showHighlight ? pal.buttonHighlightText : pal.buttonText } }
                }
            }
            QGCLabel { Layout.fillWidth: true; text: root.selectionCurrent ? qsTr("本页选中旧组：%1。当前组、正在监视组、退休中或失源组不可选；闭合及原生发送栈等仍在最后核对。").arg(root.selection.row.episodeId) : qsTr("请明确选择非当前且非监视的旧组；不自动选择第一组。未发布的失败组也须原生确认未使用，不能凭外观删除。"); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCLabel { Layout.fillWidth: true; text: qsTr("若原生提示观察未闭，请经原运行观察的结束审阅流程明确关闭本地监视，再把监视选择切离该组。这里不会自动结束观察或停止飞机。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            QGCButton { id: archivePathButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest && root.selectionCurrent; text: qsTr("只选择归档路径（不写文件）"); Accessible.name: text; onPressed: press = {owner: root.controller, workspace: root.workspace, epoch: root.lifetime.epoch, selection: root.selection}; onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.choosePath("archive-retire", v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCLabel { Layout.fillWidth: true; visible: root.fileCurrent; text: root.fileCurrent ? qsTr("待确认路径：%1\n仅为本页选择，尚未读写文件。").arg(root.pendingFile.path) : ""; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCCheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("确认仅回收本地"); Accessible.name: qsTr("我已核对旧组已闭，知道旧飞机可能仍在飞，只回收本地对象和容量，不停止或宣告完成任务"); checked: root.riskConfirmed; enabled: root.canRequest && root.fileCurrent && root.pendingFile.action === "archive-retire"; onClicked: { var g = root.lifetime; root.riskConfirmed = checked; if (g.alive) checked = Qt.binding(function() { return root.riskConfirmed }) } activeFocusOnTab: true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCLabel { Layout.fillWidth: true; text: qsTr("我已核对本旧组的本地观察已闭；仍须原生最后核对。旧飞机可能继续飞行；保存 / 回收不会 STOP、LAND、RTL、DISARM，也不把任何候选改成 Completed。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            QGCCheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("另确认路径及覆盖"); Accessible.name: qsTr("我另行确认准确归档路径并允许覆盖已有文件；不因文件不存在跳过确认"); checked: root.overwriteConfirmed; enabled: root.canRequest && root.fileCurrent && root.pendingFile.action === "archive-retire"; onClicked: { var g = root.lifetime; root.overwriteConfirmed = checked; if (g.alive) checked = Qt.binding(function() { return root.overwriteConfirmed }) } activeFocusOnTab: true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCButton { id: archiveButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest && root.selectionCurrent && root.fileCurrent && root.pendingFile.action === "archive-retire" && root.riskConfirmed && root.overwriteConfirmed; text: qsTr("明确保存 + 最后核对回收旧本地组"); Accessible.name: text; onPressed: press = root.requestIntent("archive-retire", -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); contentItem: QGCLabel { text: archiveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: archiveButton._showHighlight ? pal.buttonHighlightText : pal.buttonText } }
            QGCLabel { Layout.fillWidth: true; text: root.factsText(); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCLabel { Layout.fillWidth: true; visible: text.length > 0; text: root.nativeFactsText(); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCButton { id: openPathButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest; text: qsTr("选择历史归档文件（不覆盖live）"); Accessible.name: text; onPressed: press = {owner: root.controller, workspace: root.workspace, epoch: root.lifetime.epoch}; onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.choosePath("open", v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCCheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("确认只读打开历史"); Accessible.name: qsTr("明确只读打开此历史文件，不恢复live对象、账号、批准、监视或飞行权限"); checked: root.openConfirmed; enabled: root.canRequest && root.fileCurrent && root.pendingFile.action === "open"; onClicked: { var g = root.lifetime; root.openConfirmed = checked; if (g.alive) checked = Qt.binding(function() { return root.openConfirmed }) } activeFocusOnTab: true; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCButton { id: openButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest && root.fileCurrent && root.pendingFile.action === "open" && root.openConfirmed; text: qsTr("明确打开独立历史归档"); Accessible.name: text; onPressed: press = root.requestIntent("open", -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCLabel { Layout.fillWidth: true; visible: root.historyCurrent; text: root.historyCurrent ? qsTr("独立文件历史（未认证）\n归档 %1 · 旧组 %2\n文档声称捕获UTC ms %3（不是本次打开时间或任务结束时间）\n类型 %4 · 已发布 %5 · 报告数 %6\n内容hash %7（非签名）\n没有报告不表示任务完成；本页不自动选报告或恢复live。").arg(root.history.summary.archiveId).arg(root.history.summary.episodeId).arg(root.history.summary.capturedAtUtcMs).arg(root.history.summary.retirementKind).arg(root.flag(root.history.summary.published)).arg(root.history.summary.reportCount).arg(root.history.summary.contentHash) : ""; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            Repeater { model: root.historyCurrent ? root.history.summary.reportCount : 0; QGCButton { id: reportButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest; text: qsTr("明确读取第 %1 份历史报告").arg(index + 1); Accessible.name: text; onPressed: press = root.requestIntent("report", index); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) } }
            QGCLabel { Layout.fillWidth: true; visible: !!root.reportView; text: root.reportView ? qsTr("第 %1 份完整canonical JSON，只读文本\nUTF16偏移 %2–%3 / 总 %4。没有截断；前后片保代理对，不执行或反建权限。").arg(root.reportView.index + 1).arg(root.reportView.offset).arg(root.chunkEnd).arg(root.reportView.text.length) : ""; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            Controls.TextArea { Layout.fillWidth: true; visible: !!root.reportView; readOnly: true; selectByMouse: true; wrapMode: TextEdit.WrapAnywhere; textFormat: TextEdit.PlainText; text: root.reportView ? root.reportView.text.substring(root.reportView.offset, root.chunkEnd) : ""; color: pal.text; font.pointSize: ScreenTools.defaultFontPointSize; Accessible.name: qsTr("历史报告完整文本当前分片"); background: Rectangle { color: pal.windowShade; border.color: pal.text; border.width: parent.activeFocus ? 2 : 1 } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            RowLayout {
                Layout.fillWidth: true
                QGCButton { property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest && !!root.reportView && root.reportView.offsets.length > 0; text: qsTr("上一文本片"); Accessible.name: text; onPressed: press = root.pageIntent(); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.page(-1, v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
                QGCButton { property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest && !!root.reportView && root.chunkEnd < root.reportView.text.length; text: qsTr("下一文本片"); Accessible.name: text; onPressed: press = root.pageIntent(); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.page(1, v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            }
            QGCButton { id: clearButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canRequest && root.historyCurrent; text: qsTr("只清此准确历史槽（不删文件）"); Accessible.name: text; onPressed: press = root.requestIntent("clear-file", -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCButton { id: cancelButton; property var press: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; primary: activeFocus; enabled: root.canCancel; text: qsTr("仅请求取消本页原job"); Accessible.name: text; onPressed: press = root.cancelIntent(); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.cancel(v) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this) }
            QGCLabel { Layout.fillWidth: true; visible: root.busy; text: root.state.stage === "destroying" ? qsTr("原生已提交退休，正在等待实际组销毁；此后不能取消。不能提前把容量加回。") : qsTr("异步操作进行中。取消只请求停止原job，不能撤回已写文件；终态回执保持原业务action，不自动重试。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        }
    }
    Component {
        id: fileComponent
        QGCFileDialog { property var ownerPanel: null; property var life: null; property var ticket: null; folder: ""; nameFilters: ["Deployment archive (*.json)"]; onAcceptedForLoad: if (life && life.alive && ownerPanel) ownerPanel.fileChosen(this, file); onAcceptedForSave: if (life && life.alive && ownerPanel) ownerPanel.fileChosen(this, file); onRejected: if (life && life.alive && ownerPanel) ownerPanel.fileRejected(this) }
    }
}

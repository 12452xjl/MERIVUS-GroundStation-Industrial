import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Two immutable native views, never an import into the active passport.
FocusScope {
    id: root
    property var controller: null
    property bool expanded: false
    property var lifetime: ({alive: true, ticket: null, epoch: 0, consent: 0, fileEpoch: 0, exhausted: false})
    property bool callBusy: false
    property bool freezeConfirmed: false
    property bool fileConfirmed: false
    property bool freezeQualified: false
    property bool fileQualified: false
    property var snapshots: ({session: null, file: null})
    property var browser: ({origin: "", groups: null, group: null, records: null, record: null, detail: null, offsets: []})
    property var pendingJob: null
    property var pendingFile: null
    property var fileDialog: null
    property var saveFacts: null
    property string feedback: ""
    readonly property var workspace: controller ? controller.maintenanceWorkspace : null
    readonly property var state: workspace ? workspace.state || {} : ({})
    readonly property var result: workspace ? workspace.result || {} : ({})
    readonly property string latestJobId: typeof state.jobId === "string" ? state.jobId : ""
    readonly property bool stateValid: stateKnown(state)
    readonly property bool active: lifetime.alive && !lifetime.exhausted && visible && enabled && expanded
    readonly property bool ready: active && !!workspace && typeof workspace.request === "function" && stateValid
    readonly property bool busy: stateValid && state.busy === true
    readonly property bool canRequest: ready && !busy && !callBusy && !fileDialog && !pendingJob
    readonly property var selectedSnapshot: browser.origin === "session" ? snapshots.session : browser.origin === "file" ? snapshots.file : null
    readonly property bool selectedCurrent: snapshotCurrent(selectedSnapshot)
    readonly property bool frozenCurrent: snapshotCurrent(snapshots.session)
    readonly property bool fileCurrent: snapshotCurrent(snapshots.file)
    readonly property bool filePendingCurrent: pendingFileCurrent(pendingFile)
    readonly property bool canCancel: ready && busy && !callBusy && !fileDialog && !!pendingJob
        && pendingJob.owner === controller && pendingJob.workspace === workspace && pendingJob.id === latestJobId
        && state.cancelRequested === false
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    implicitHeight: form.implicitHeight
    QGCPalette { id: pal; colorGroupEnabled: root.enabled }

    function map(value) { return value !== null && typeof value === "object" && !Array.isArray(value) }
    function own(value, name) { return map(value) && Object.prototype.hasOwnProperty.call(value, name) }
    function keys(value, names) {
        if (!map(value) || Object.keys(value).length !== names.length) return false
        for (var i = 0; i < names.length; ++i) if (!own(value, names[i])) return false
        return true
    }
    function integer(value, low, high) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= low && value <= high }
    function text(value, limit) { return typeof value === "string" && value.length <= limit }
    function uuid(value) { return text(value, 36) && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && value !== "00000000-0000-0000-0000-000000000000" }
    function digest(value) { return text(value, 64) && /^[0-9a-f]{64}$/.test(value) }
    function counter(value) { return text(value, 20) && /^(0|[1-9][0-9]{0,19})$/.test(value) && (value.length < 20 || value <= "18446744073709551615") }
    function clone(value) { try { return JSON.parse(JSON.stringify(value)) } catch (error) { return null } }
    function flagText(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function sourceKnown(value, id) {
        return keys(value, ["runId", "headerHash", "fileHash", "sourceEpoch", "vehicleId", "componentId", "batteryId"])
            && uuid(value.runId) && digest(value.headerHash) && digest(value.fileHash) && uuid(value.sourceEpoch)
            && value.vehicleId === id && integer(value.componentId, 1, 255) && integer(value.batteryId, 0, 255)
    }
    function descriptorKnown(value, origin) {
        return keys(value, ["schemaVersion", "snapshotId", "origin", "sourceInstanceEpoch", "sourceRevision", "capturedAtUtcMs", "documentGeneratedAt",
                "recordCount", "groupCount", "contentHash", "sourceFileHash", "sourceCurrent", "historicalOnly", "authenticated",
                "physicalIdentityVerified", "permissionsRestored", "executionPermit", "flightCommandReleased"])
            && value.schemaVersion === 1 && uuid(value.snapshotId) && value.origin === origin
            && (origin === "session" ? uuid(value.sourceInstanceEpoch) && counter(value.sourceRevision) && value.sourceFileHash === ""
                : value.sourceInstanceEpoch === "" && value.sourceRevision === "0" && value.sourceFileHash === value.contentHash && value.sourceCurrent === false)
            && integer(value.capturedAtUtcMs, 1, 9007199254740991) && text(value.documentGeneratedAt, 128)
            && integer(value.recordCount, 0, 2000) && integer(value.groupCount, 0, value.recordCount)
            && digest(value.contentHash) && typeof value.sourceCurrent === "boolean" && value.historicalOnly === true
            && value.authenticated === false && value.physicalIdentityVerified === false && value.permissionsRestored === false
            && value.executionPermit === false && value.flightCommandReleased === false
    }
    function stateKnown(value) {
        return keys(value, ["schemaVersion", "busy", "jobId", "action", "stage", "cancelRequested", "frozenSnapshot", "openedSnapshot",
                "sourceInstanceEpoch", "sourceRevision", "revisionExhausted"])
            && value.schemaVersion === 1 && typeof value.busy === "boolean" && (value.jobId === "" || uuid(value.jobId))
            && text(value.action, 32) && ["idle", "preflight", "running", "cancelling", "complete", "failed", "cancelled"].indexOf(value.stage) >= 0 && typeof value.cancelRequested === "boolean"
            && map(value.frozenSnapshot) && (!Object.keys(value.frozenSnapshot).length || descriptorKnown(value.frozenSnapshot, "session"))
            && map(value.openedSnapshot) && (!Object.keys(value.openedSnapshot).length || descriptorKnown(value.openedSnapshot, "file"))
            && uuid(value.sourceInstanceEpoch) && counter(value.sourceRevision) && typeof value.revisionExhausted === "boolean"
            && (!value.busy || (uuid(value.jobId) && ["freeze", "open", "save", "groups", "records", "detail"].indexOf(value.action) >= 0))
    }
    function snapshotCurrent(value) {
        if (!value || !stateValid || !descriptorKnown(value.dto, value.dto.origin) || value.owner !== controller || value.workspace !== workspace) return false
        var actual = value.dto.origin === "session" ? state.frozenSnapshot : state.openedSnapshot
        return actual.snapshotId === value.dto.snapshotId && actual.contentHash === value.dto.contentHash && actual.origin === value.dto.origin
    }
    function groupKnown(value) {
        return keys(value, ["groupId", "vehicleId", "historyScopeId", "historicalOnly", "recordCount", "maintenanceStatus", "predictiveRisk",
                "estimatedRiskScore", "confidence", "sourceSelectionRequired", "manualReviewRequired", "historicalTelemetrySource", "physicalIdentityVerified"])
            && digest(value.groupId) && integer(value.vehicleId, 1, 255) && typeof value.historicalOnly === "boolean"
            && (value.historicalOnly ? digest(value.historyScopeId) && sourceKnown(value.historicalTelemetrySource, value.vehicleId)
                : value.historyScopeId === "" && map(value.historicalTelemetrySource) && !Object.keys(value.historicalTelemetrySource).length)
            && integer(value.recordCount, 1, 2000) && ["unknown", "monitor", "inspection_due", "grounding_recommended"].indexOf(value.maintenanceStatus) >= 0
            && ["unknown", "low", "medium", "high", "critical"].indexOf(value.predictiveRisk) >= 0 && integer(value.estimatedRiskScore, 0, 100)
            && ["low", "medium", "higher_but_not_validated"].indexOf(value.confidence) >= 0
            && typeof value.sourceSelectionRequired === "boolean" && value.manualReviewRequired === true && value.physicalIdentityVerified === false
    }
    function pageKnown(value, request, records) {
        var names = ["snapshotId", "afterIndex", "nextIndex", "hasMore", "totalCount", "rows"]
        if (records) names.push("groupId")
        if (!keys(value, names) || value.snapshotId !== request.intent.snapshotId || value.afterIndex !== request.intent.offset
                || !integer(value.totalCount, 0, 2000) || !integer(value.afterIndex, 0, value.totalCount)
                || !Array.isArray(value.rows) || value.rows.length > 8 || value.nextIndex !== value.afterIndex + value.rows.length
                || value.nextIndex > value.totalCount || value.hasMore !== (value.nextIndex < value.totalCount)
                || (value.afterIndex < value.totalCount && value.rows.length === 0)
                || (records && value.groupId !== request.intent.groupId)) return false
        var seen = {}
        for (var i = 0; i < value.rows.length; ++i) {
            var row = value.rows[i]
            if (!records) {
                if (!groupKnown(row) || seen[row.groupId]) return false
                seen[row.groupId] = true
            } else {
                if (!keys(row, ["recordIndex", "groupRecordIndex", "timestamp", "timestampMs", "fieldCount", "historicalOnly", "metrics", "hasModifications"])
                        || !integer(row.recordIndex, 0, 1999) || row.groupRecordIndex !== value.afterIndex + i || seen[row.recordIndex]
                        || !text(row.timestamp, 128) || !row.timestamp.length || !integer(row.timestampMs, 1, 9007199254740991)
                        || !integer(row.fieldCount, 1, 4096) || row.historicalOnly !== request.group.historicalOnly
                        || typeof row.hasModifications !== "boolean" || !map(row.metrics)) return false
                seen[row.recordIndex] = true
                var metricNames = Object.keys(row.metrics)
                if (metricNames.length > 5) return false
                for (var m = 0; m < metricNames.length; ++m) {
                    var key = metricNames[m], number = row.metrics[key]
                    if (["currentAmps", "temperatureC", "vibrationRms", "batteryDegradationPercent", "flightCount"].indexOf(key) < 0
                            || typeof number !== "number" || !isFinite(number) || Math.abs(number) > 9007199254740991
                            || (key === "temperatureC" ? number < -100 || number > 300 : number < 0)
                            || (key === "batteryDegradationPercent" && number > 100)
                            || (key === "flightCount" && Math.floor(number) !== number)) return false
                }
            }
        }
        return records ? value.totalCount === request.group.recordCount : value.totalCount === request.snapshot.dto.groupCount
    }
    function detailKnown(value, request) {
        return keys(value, ["snapshotId", "groupId", "recordIndex", "offset", "nextOffset", "hasMore", "totalCharacters", "contentHash", "text", "canonicalJson"])
            && value.snapshotId === request.intent.snapshotId && value.groupId === request.intent.groupId
            && value.recordIndex === request.intent.recordIndex && value.offset === request.intent.offset
            && integer(value.totalCharacters, 0, 67108864) && integer(value.offset, 0, value.totalCharacters)
            && text(value.text, 2048) && value.nextOffset === value.offset + value.text.length && value.nextOffset <= value.totalCharacters
            && value.hasMore === (value.nextOffset < value.totalCharacters) && (!value.hasMore || value.text.length > 0)
            && digest(value.contentHash) && value.canonicalJson === true
            && (!request.detailHash || (value.contentHash === request.detailHash && value.totalCharacters === request.detailLength))
    }
    function resultKnown(value, request) {
        return keys(value, ["schemaVersion", "jobId", "action", "ok", "error", "reply", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
            && value.schemaVersion === 1 && value.jobId === request.id && value.action === request.intent.action && typeof value.ok === "boolean"
            && text(value.error, 4096) && map(value.reply) && value.historicalOnly === true && value.authenticated === false
            && value.permissionsRestored === false && value.executionPermit === false && value.flightCommandReleased === false
    }
    function saveKnown(value, request) {
        return keys(value, ["snapshotId", "contentHash", "path", "fileWriteAttempted", "fileWrittenKnown", "fileWritten", "readbackVerified", "readbackError", "fsyncVerified", "durable", "authenticated"])
            && value.snapshotId === request.intent.snapshotId && value.contentHash === request.intent.contentHash && value.path === request.intent.filePath
            && typeof value.fileWriteAttempted === "boolean" && typeof value.fileWrittenKnown === "boolean"
            && (value.fileWritten === null || typeof value.fileWritten === "boolean")
            && (!value.fileWrittenKnown || typeof value.fileWritten === "boolean") && typeof value.readbackVerified === "boolean"
            && (!value.readbackVerified || (value.fileWrittenKnown && value.fileWritten === true)) && text(value.readbackError, 4096)
            && value.fsyncVerified === false && value.durable === false && value.authenticated === false
    }
    function receiptKnown(value, action) {
        return keys(value, ["schemaVersion", "action", "accepted", "jobId", "reason", "requestOnly", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
            && value.schemaVersion === 1 && value.action === action && typeof value.accepted === "boolean"
            && (value.jobId === "" || uuid(value.jobId)) && text(value.reason, 4096) && value.requestOnly === true && value.historicalOnly === true
            && value.authenticated === false && value.permissionsRestored === false && value.executionPermit === false && value.flightCommandReleased === false
    }
    function takeTicket(allowExisting) {
        var guard = lifetime, panel = root
        if (!guard.alive) return null
        if (guard.ticket) return allowExisting ? {ticket: guard.ticket, owns: false, release: function() {}} : null
        var ticket = {jobChanges: 0}
        guard.ticket = ticket
        // The finally path owns this plain closure even if a notification has
        // destroyed the QML panel. Only the still-live original panel is touched.
        return {ticket: ticket, owns: true, release: function() {
            if (guard.ticket !== ticket) return
            if (guard.alive) panel.callBusy = false
            if (guard.ticket === ticket) guard.ticket = null
        }}
    }
    function bump(name) {
        var guard = lifetime
        if (guard[name] >= 9007199254740991) { guard.exhausted = true; return false }
        guard[name] += 1
        return true
    }
    function fileContext(value) {
        return value && ready && value.owner === controller && value.workspace === workspace
            && value.epoch === lifetime.epoch && value.fileEpoch === lifetime.fileEpoch && value.jobId === latestJobId
            && (value.action === "open" || (value.action === "save" && value.snapshot === snapshots.session && snapshotCurrent(value.snapshot)))
    }
    function pendingFileCurrent(value) { return fileContext(value) && text(value.filePath, 4096) && value.filePath.length > 0 }
    function closeDialog(view) {
        if (!view) return
        var guard = lifetime, viewGuard = view.dialogGuard
        viewGuard.active = false
        view.activeIntent = false
        if (!guard.alive || !viewGuard.alive) return
        view.ownerPanel = null
        if (!guard.alive || !viewGuard.alive) return
        if (fileDialog === view) fileDialog = null
        if (!guard.alive || !viewGuard.alive) return
        view.close()
        if (guard.alive && viewGuard.alive) view.destroy()
    }
    function retireFile() {
        var guard = lifetime, lease = takeTicket(true)
        if (!lease) return false
        try {
            if (!bump("fileEpoch")) return false
            fileQualified = false
            if (!guard.alive || guard.ticket !== lease.ticket) return false
            pendingFile = null
            if (!guard.alive || guard.ticket !== lease.ticket) return false
            fileConfirmed = false
            if (!guard.alive || guard.ticket !== lease.ticket) return false
            var view = fileDialog
            if (view) closeDialog(view)
            return guard.alive && guard.ticket === lease.ticket && !fileConfirmed && !fileDialog
        } finally { lease.release() }
    }
    function retireLocal(sourceChanged) {
        var guard = lifetime, lease = takeTicket(true)
        if (!lease) return
        try {
            bump("epoch")
            if (!retireFile() || !guard.alive || guard.ticket !== lease.ticket) return
            freezeQualified = false
            if (!guard.alive || guard.ticket !== lease.ticket) return
            freezeConfirmed = false
            if (!guard.alive || guard.ticket !== lease.ticket) return
            if (sourceChanged) {
                var old = pendingJob
                if (old && old.intent.action === "save") {
                    saveFacts = {known: false, ok: false, error: "source_changed_before_receipt_adoption", jobId: old.id,
                        filePath: old.intent.filePath, snapshotId: old.intent.snapshotId, contentHash: old.intent.contentHash, reply: null}
                    if (!guard.alive || guard.ticket !== lease.ticket || pendingJob !== old) return
                }
                pendingJob = null
                if (!guard.alive || guard.ticket !== lease.ticket) return
                snapshots = {session: null, file: null}
                if (!guard.alive || guard.ticket !== lease.ticket) return
                browser = {origin: "", groups: null, group: null, records: null, record: null, detail: null, offsets: []}
            }
        } finally { lease.release() }
    }
    function failure(message, current) {
        var guard = lifetime
        if (!guard.alive || !current()) return
        feedback = message
        if (guard.alive && current() && active) feedbackLabel.forceActiveFocus()
    }
    function pickerIntent(action) {
        if (!canRequest || lifetime.ticket || (action !== "open" && action !== "save") || (action === "save" && !frozenCurrent)) return null
        return {action: action, owner: controller, workspace: workspace, epoch: lifetime.epoch, fileEpoch: lifetime.fileEpoch,
            consent: lifetime.consent, jobId: latestJobId, snapshot: action === "save" ? snapshots.session : null}
    }
    function openPicker(value) {
        if (!fileContext(value) || value.consent !== lifetime.consent || !canRequest) return
        var guard = lifetime, lease = takeTicket(false), view = null, viewGuard = {alive: true, active: true}
        if (!lease) return
        try {
            callBusy = true
            if (!guard.alive || !fileContext(value) || value.consent !== lifetime.consent) return
            var originalConsent = lifetime.consent, expectedConsent = originalConsent + (fileConfirmed ? 1 : 0)
            var originalFileEpoch = lifetime.fileEpoch
            if (!retireFile() || !guard.alive || lifetime.fileEpoch !== originalFileEpoch + 1
                    || lifetime.consent !== expectedConsent || fileConfirmed || value.epoch !== lifetime.epoch
                    || value.owner !== controller || value.workspace !== workspace || value.jobId !== latestJobId
                    || (value.action === "save" && value.snapshot !== snapshots.session)) return
            var frozen = {action: value.action, owner: value.owner, workspace: value.workspace, epoch: value.epoch,
                fileEpoch: lifetime.fileEpoch, consent: lifetime.consent, jobId: value.jobId, snapshot: value.snapshot}
            view = (value.action === "save" ? saveDialogComponent : openDialogComponent).createObject(root,
                {ownerPanel: root, panelGuard: guard, dialogGuard: viewGuard, frozenIntent: frozen})
            if (!guard.alive) return
            if (!view || !viewGuard.alive || !fileContext(frozen) || frozen.consent !== lifetime.consent || fileConfirmed) {
                if (view && viewGuard.alive) closeDialog(view)
                return
            }
            fileDialog = view
            if (!guard.alive || !viewGuard.alive || fileDialog !== view || !fileContext(frozen)
                    || frozen.consent !== lifetime.consent || fileConfirmed) { if (guard.alive && viewGuard.alive) closeDialog(view); return }
            if (value.action === "save") view.openForSave(); else view.openForLoad()
        } catch (error) {
            if (guard.alive && viewGuard.alive && view) closeDialog(view)
            if (guard.alive && value.epoch === lifetime.epoch) feedback = qsTr("文件窗口未确认打开；未请求读写，请重新选择路径。")
        } finally { lease.release() }
    }
    function fileChosen(view, filePath) {
        if (!lifetime.alive || fileDialog !== view || !view.activeIntent || !view.dialogGuard.active) return
        var guard = lifetime, value = view.frozenIntent, lease = takeTicket(true), viewGuard = view.dialogGuard
        if (!lease) return
        try {
            var valid = fileContext(value) && value.consent === lifetime.consent && !fileConfirmed
                && text(filePath, 4096) && filePath.length > 0 && !/[\u0000-\u001f\u007f]/.test(filePath)
            closeDialog(view)
            if (!guard.alive || !valid || !fileContext(value) || value.consent !== lifetime.consent || fileConfirmed || fileDialog) return
            var staged = {action: value.action, owner: value.owner, workspace: value.workspace, epoch: value.epoch,
                fileEpoch: value.fileEpoch, jobId: value.jobId, snapshot: value.snapshot, filePath: filePath}
            pendingFile = staged
            if (!guard.alive || pendingFile !== staged || !pendingFileCurrent(staged) || value.consent !== lifetime.consent || fileConfirmed) return
            feedback = value.action === "save" ? qsTr("仅已暂存路径，尚未写入。请核对冻结ID、hash、路径和覆盖风险，再另行确认。")
                : qsTr("仅已暂存路径，尚未读取。另行确认后仅打开独立历史，不覆盖本进程维护护照。")
        } finally { lease.release() }
    }
    function fileRejected(view) {
        if (!lifetime.alive || fileDialog !== view) return
        var guard = lifetime, value = view.frozenIntent, lease = takeTicket(true)
        if (!lease) return
        try {
            closeDialog(view)
            if (guard.alive && fileContext(value) && value.consent === lifetime.consent) feedback = qsTr("已取消路径选择，未请求文件读写。")
        } finally { lease.release() }
    }
    function requestIntent(action, offset, recordIndex) {
        if (!canRequest || lifetime.ticket) return null
        var intent = {action: action, expectedJobId: latestJobId, snapshotId: "", contentHash: "", filePath: "", groupId: "", recordIndex: -1, offset: 0, overwriteConfirmed: false}
        var snapshot = null, group = null, offsets = [], detailHash = "", detailLength = -1
        if (action === "freeze") { if (!freezeConfirmed || !freezeQualified) return null }
        else if (action === "open" || action === "save") {
            if (!filePendingCurrent || pendingFile.action !== action || !fileConfirmed || !fileQualified) return null
            intent.filePath = pendingFile.filePath
            if (action === "save") {
                snapshot = pendingFile.snapshot; intent.snapshotId = snapshot.dto.snapshotId
                intent.contentHash = snapshot.dto.contentHash; intent.overwriteConfirmed = true
            }
        } else if (action === "clear-frozen" || action === "clear-file") {
            snapshot = action === "clear-frozen" ? snapshots.session : snapshots.file
            if (!snapshotCurrent(snapshot)) return null
            intent.snapshotId = snapshot.dto.snapshotId
        } else {
            if (!selectedCurrent) return null
            snapshot = selectedSnapshot; intent.snapshotId = snapshot.dto.snapshotId
            if (action === "groups") { if (!integer(offset, 0, 2000)) return null; intent.offset = offset }
            else {
                if (!browser.group || !groupKnown(browser.group)) return null
                group = browser.group; intent.groupId = group.groupId
                if (action === "records") { if (!integer(offset, 0, 2000)) return null; intent.offset = offset }
                else if (action === "detail") {
                    if (!integer(offset, 0, 67108864) || !integer(recordIndex, -1, 1999)
                            || (recordIndex >= 0 && (!browser.record || browser.record.recordIndex !== recordIndex))) return null
                    intent.recordIndex = recordIndex; intent.offset = offset
                    var detail = browser.detail
                    var previous = detail && detail.recordIndex === recordIndex && browser.offsets.length
                        && offset === browser.offsets[browser.offsets.length - 1]
                    if (offset > 0 || previous) {
                        if (!detail || detail.recordIndex !== recordIndex) return null
                        if (offset === detail.nextOffset && detail.hasMore) offsets = browser.offsets.concat([detail.offset])
                        else if (previous) offsets = browser.offsets.slice(0, -1)
                        else return null
                        detailHash = detail.contentHash; detailLength = detail.totalCharacters
                    }
                } else return null
            }
        }
        return {owner: controller, workspace: workspace, epoch: lifetime.epoch, consent: lifetime.consent, browser: browser,
            intent: intent, snapshot: snapshot, group: group, file: action === "save" || action === "open" ? pendingFile : null,
            offsets: offsets, detailHash: detailHash, detailLength: detailLength}
    }
    function requestCurrent(value, consent, jobId) {
        return value && ready && value.owner === controller && value.workspace === workspace && value.epoch === lifetime.epoch
            && consent === lifetime.consent && browser === value.browser && latestJobId === jobId
            && (!value.snapshot || (value.snapshot === snapshots[value.snapshot.dto.origin] && snapshotCurrent(value.snapshot)))
    }
    function submit(value) {
        if (!value || !requestCurrent(value, value.consent, value.intent.expectedJobId) || !canRequest || lifetime.ticket) return
        var fresh = requestIntent(value.intent.action, value.intent.offset, value.intent.recordIndex)
        if (!fresh || JSON.stringify(fresh.intent) !== JSON.stringify(value.intent) || fresh.file !== value.file) return
        var guard = lifetime, lease = takeTicket(false), expectedConsent = value.consent
        if (!lease) return
        try {
            callBusy = true
            if (!guard.alive || !requestCurrent(value, expectedConsent, value.intent.expectedJobId)) return
            if (freezeConfirmed) expectedConsent += 1
            freezeConfirmed = false
            if (!guard.alive || !requestCurrent(value, expectedConsent, value.intent.expectedJobId) || freezeConfirmed) return
            if (fileConfirmed) expectedConsent += 1
            fileConfirmed = false
            if (!guard.alive || !requestCurrent(value, expectedConsent, value.intent.expectedJobId) || fileConfirmed) return
            if (value.file) {
                pendingFile = null
                if (!guard.alive || !requestCurrent(value, expectedConsent, value.intent.expectedJobId)) return
            }
            var receipt = value.workspace.request(value.intent, true)
            if (!guard.alive || guard.ticket !== lease.ticket || value.owner !== controller || value.workspace !== workspace) return
            if (!receiptKnown(receipt, value.intent.action) || !receipt.accepted) {
                failure(qsTr("请求未确认受理：%1。可能已发生本地操作；请核对原生状态，不自动重试。").arg(receipt && typeof receipt.reason === "string" ? receipt.reason : qsTr("未知返回")), function() { return guard.alive && value.epoch === lifetime.epoch })
                return
            }
            var sync = value.intent.action === "clear-file" || value.intent.action === "clear-frozen"
            if (lease.ticket.jobChanges !== (sync ? 0 : 1) || latestJobId !== receipt.jobId
                    || value.epoch !== lifetime.epoch || expectedConsent !== lifetime.consent || browser !== value.browser
                    || !active || freezeConfirmed || fileConfirmed) return
            if (sync) {
                var origin = value.intent.action === "clear-file" ? "file" : "session"
                var actualSlot = origin === "file" ? state.openedSnapshot : state.frozenSnapshot
                if (!stateValid || !map(actualSlot) || Object.keys(actualSlot).length) {
                    failure(qsTr("清槽回执后原生槽未确认已空；不清本地后来来源，请核对原生状态。"), function() { return guard.alive && value.epoch === lifetime.epoch })
                    return
                }
                var next = {session: snapshots.session, file: snapshots.file}; next[origin] = null
                snapshots = next
                if (!guard.alive || value.epoch !== lifetime.epoch) return
                if (browser.origin === origin) browser = {origin: "", groups: null, group: null, records: null, record: null, detail: null, offsets: []}
                if (guard.alive && value.epoch === lifetime.epoch) feedback = qsTr("仅清除了准确的本地快照槽；没有删除文件或清空active护照，也不抹文件写入事实。")
            } else {
                var job = {owner: value.owner, workspace: value.workspace, epoch: value.epoch, id: receipt.jobId, intent: value.intent,
                    snapshot: value.snapshot, group: value.group, browser: value.browser, offsets: value.offsets,
                    detailHash: value.detailHash, detailLength: value.detailLength}
                pendingJob = job
                if (!guard.alive || pendingJob !== job || value.epoch !== lifetime.epoch) return
                feedback = qsTr("本次请求已受理，等待结果与工作线程均结束。取消不会撤回可能已提交的文件写入。")
            }
        } catch (error) {
            if (guard.alive && value.epoch === lifetime.epoch) feedback = qsTr("本地操作结局未知；不自动重试、不声称文件未写或已回滚。")
        } finally { lease.release(); if (guard.alive) settle() }
    }
    function cancelIntent() {
        return canCancel && !lifetime.ticket ? {owner: controller, workspace: workspace, id: pendingJob.id, job: pendingJob, epoch: lifetime.epoch} : null
    }
    function cancel(value) {
        if (!value || !canCancel || lifetime.ticket || value.owner !== controller || value.workspace !== workspace
                || value.job !== pendingJob || value.id !== latestJobId || value.epoch !== lifetime.epoch) return
        var guard = lifetime, lease = takeTicket(false)
        if (!lease) return
        try {
            callBusy = true
            if (!guard.alive || value.epoch !== lifetime.epoch || !ready || value.job !== pendingJob
                    || value.id !== latestJobId || value.owner !== controller || value.workspace !== workspace) return
            var receipt = value.workspace.request({action: "cancel", expectedJobId: value.id, snapshotId: "", contentHash: "", filePath: "",
                groupId: "", recordIndex: -1, offset: 0, overwriteConfirmed: false}, true)
            if (!guard.alive || value.owner !== controller || value.workspace !== workspace || pendingJob !== value.job) return
            feedback = receiptKnown(receipt, "cancel") && receipt.accepted && receipt.jobId === value.id
                ? qsTr("仅已请求取消原job；双终结前仍占用槽。文件可能已写，不自动重试或回滚。") : qsTr("取消未确认；请核对原job，不操作后来任务。")
        } finally { lease.release(); if (guard.alive) settle() }
    }
    function settle() {
        var guard = lifetime, job = pendingJob
        if (!guard.alive || guard.ticket || callBusy || !job || job.owner !== controller || job.workspace !== workspace
                || !stateValid || busy || latestJobId !== job.id) return
        // Both public bindings originate in workspaceChanged, but their QML
        // refresh order is not a contract. An old/empty result is not this job's
        // malformed terminal result: retain ownership until its result arrives.
        if (!map(result) || result.jobId !== job.id || result.action !== job.intent.action) return
        var lease = takeTicket(false)
        if (!lease) return
        function same() { return guard.alive && guard.ticket === lease.ticket && pendingJob === job && job.owner === controller && job.workspace === workspace && stateValid && !busy && latestJobId === job.id }
        function presentation() { return same() && active && job.epoch === lifetime.epoch && browser === job.browser }
        try {
            callBusy = true
            if (!same()) return
            var value = clone(result)
            if (!resultKnown(value, job)) { if (presentation()) failure(qsTr("结果未对应本次完整回执，未采用；原文件事实可能已发生，请人工核对。"), presentation); return }
            if (job.intent.action === "save") {
                var known = saveKnown(value.reply, job)
                saveFacts = {known: known, ok: value.ok, error: value.error, jobId: job.id, filePath: job.intent.filePath,
                    snapshotId: job.intent.snapshotId, contentHash: job.intent.contentHash, reply: known ? value.reply : null}
                if (!same()) return
                if (presentation()) feedback = known && value.reply.fileWrittenKnown && value.reply.fileWritten === true
                    ? qsTr("原生回执确认文件已写；回读、fsync与durable分别列示，不等于维护或飞行认证。")
                    : qsTr("文件写入未完全确认；查看下方实际事实，人工核对路径，不自动重试。")
            } else if (presentation()) {
                if (!value.ok) { failure(qsTr("本次操作未成功：%1。保留已有历史，不自动重试。").arg(value.error), presentation); return }
                var nextBrowser, nextSnapshots
                if (job.intent.action === "freeze" || job.intent.action === "open") {
                    var origin = job.intent.action === "freeze" ? "session" : "file", descriptor = value.reply.descriptor
                    if (!keys(value.reply, ["descriptor"]) || !descriptorKnown(descriptor, origin)) { failure(qsTr("快照描述不完整，未采用；不会从active或其它文件补源。"), presentation); return }
                    var actual = origin === "session" ? state.frozenSnapshot : state.openedSnapshot
                    if (actual.snapshotId !== descriptor.snapshotId || actual.contentHash !== descriptor.contentHash) { failure(qsTr("快照槽已变化，不认领后来结果。"), presentation); return }
                    nextSnapshots = {session: snapshots.session, file: snapshots.file}
                    nextSnapshots[origin] = {owner: controller, workspace: workspace, dto: descriptor}
                    snapshots = nextSnapshots
                    if (!presentation()) return
                    nextBrowser = {origin: origin, groups: null, group: null, records: null, record: null, detail: null, offsets: []}
                } else if (job.intent.action === "groups") {
                    if (!snapshotCurrent(job.snapshot) || !pageKnown(value.reply, job, false)) { failure(qsTr("来源组页不对应原快照，未采用。"), presentation); return }
                    nextBrowser = {origin: browser.origin, groups: value.reply, group: null, records: null, record: null, detail: null, offsets: []}
                } else if (job.intent.action === "records") {
                    if (!snapshotCurrent(job.snapshot) || !pageKnown(value.reply, job, true)) { failure(qsTr("记录页不对应所选来源组，未采用。"), presentation); return }
                    nextBrowser = {origin: browser.origin, groups: browser.groups, group: browser.group, records: value.reply, record: null, detail: null, offsets: []}
                } else if (job.intent.action === "detail") {
                    if (!snapshotCurrent(job.snapshot) || !detailKnown(value.reply, job)) { failure(qsTr("文本分片、偏移或hash不对应原完整详情，未采用，不跳过缺片。"), presentation); return }
                    nextBrowser = {origin: browser.origin, groups: browser.groups, group: browser.group, records: browser.records,
                        record: browser.record, detail: value.reply, offsets: job.offsets}
                }
                if (!presentation()) return
                browser = nextBrowser
                if (!same() || job.epoch !== lifetime.epoch) return
                feedback = qsTr("仅已更新本次只读显示；未选首组、未恢复active记录、未授予任何维护或飞行许可。")
            }
        } finally {
            if (same()) pendingJob = null
            lease.release()
        }
    }
    function selectionIntent(kind, row) {
        return canRequest && !lifetime.ticket ? {owner: controller, workspace: workspace, epoch: lifetime.epoch, browser: browser, kind: kind, row: row} : null
    }
    function select(value) {
        if (!value || !canRequest || lifetime.ticket || value.owner !== controller || value.workspace !== workspace || value.epoch !== lifetime.epoch || value.browser !== browser) return
        var guard = lifetime, lease = takeTicket(false)
        if (!lease) return
        var expectedEpoch = value.epoch + 1
        function current() { return guard.alive && guard.ticket === lease.ticket && active && value.owner === controller
            && value.workspace === workspace && lifetime.epoch === expectedEpoch && browser === value.browser }
        try {
            if (!bump("epoch") || !retireFile() || !current()) return
            freezeConfirmed = false
            if (!current()) return
            var next
            if (value.kind === "session" || value.kind === "file") {
                if (!snapshotCurrent(snapshots[value.kind])) return
                next = {origin: value.kind, groups: null, group: null, records: null, record: null, detail: null, offsets: []}
            } else if (value.kind === "group") {
                if (!browser.groups || browser.groups.rows.indexOf(value.row) < 0 || !groupKnown(value.row)) return
                next = {origin: browser.origin, groups: browser.groups, group: value.row, records: null, record: null, detail: null, offsets: []}
            } else if (value.kind === "record") {
                if (!browser.records || browser.records.rows.indexOf(value.row) < 0) return
                next = {origin: browser.origin, groups: browser.groups, group: browser.group, records: browser.records, record: value.row, detail: null, offsets: []}
            } else return
            browser = next
        } finally { lease.release() }
    }
    function latestChanged() {
        var guard = lifetime, prior = guard.ticket
        if (!guard.alive) return
        if (prior && typeof prior.jobChanges === "number") prior.jobChanges += 1
        var lease = takeTicket(true)
        if (!lease) return
        try {
            if (!prior && !retireFile()) return
            if (!guard.alive || guard.ticket !== lease.ticket) return
            var old = pendingJob
            if (old && old.id !== latestJobId) {
                if (old.intent.action === "save") {
                    saveFacts = {known: false, ok: false, error: "job_replaced_before_receipt_adoption", jobId: old.id,
                        filePath: old.intent.filePath, snapshotId: old.intent.snapshotId, contentHash: old.intent.contentHash, reply: null}
                    if (!guard.alive || guard.ticket !== lease.ticket || pendingJob !== old) return
                }
                pendingJob = null
            }
        } finally { lease.release() }
        if (guard.alive) settle()
    }
    function description(value) {
        if (!value) return qsTr("尚无本面板实际取得的快照。")
        var d = value.dto
        return qsTr("%1\nID %2\nhash %3\n冻结本地UTC(ms) %4；原文档时间 %5\n记录%6 / 来源组%7；源仍同原revision：%8（不影响冻结bytes本身）\n原文件hash：%9\n未认证、未核物理身份、无恢复权限或飞行许可。")
            .arg(d.origin === "session" ? qsTr("本进程冻结快照，不包含后来追加") : qsTr("独立文件历史，不覆盖active护照"))
            .arg(d.snapshotId).arg(d.contentHash).arg(d.capturedAtUtcMs).arg(d.documentGeneratedAt || qsTr("原文件未提供"))
            .arg(d.recordCount).arg(d.groupCount).arg(flagText(d.origin === "session" && stateValid && state.frozenSnapshot.snapshotId === d.snapshotId ? state.frozenSnapshot.sourceCurrent : false))
            .arg(d.sourceFileHash || qsTr("非文件来源"))
    }
    function saveText() {
        if (!saveFacts) return qsTr("尚无本面板认领的文件写入回执。")
        var f = saveFacts, r = f.reply
        return qsTr("上次实际文件请求（历史事实，不复活旧路径或确认）\njob %1\n冻结ID %2\nhash %3\n路径 %4\n%5")
            .arg(f.jobId).arg(f.snapshotId).arg(f.contentHash).arg(f.filePath)
            .arg(!f.known ? qsTr("回执未知；文件可能已写，请人工核对。")
                : qsTr("尝试写入：%1；commit回执成功：%2；文件已写：%3\n回读核对：%4（%5）；fsync：否；durable：否\n原因：%6")
                    .arg(flagText(r.fileWriteAttempted)).arg(flagText(f.ok))
                    .arg(r.fileWrittenKnown ? flagText(r.fileWritten) : qsTr("未知，不等于未写"))
                    .arg(flagText(r.readbackVerified)).arg(r.readbackError || qsTr("未提供错误")) .arg(f.error || qsTr("无")))
    }
    onFreezeConfirmedChanged: {
        var guard = lifetime
        bump("consent")
        if (guard.alive) freezeQualified = freezeConfirmed && active && !guard.ticket
    }
    onFileConfirmedChanged: {
        var guard = lifetime
        bump("consent")
        if (guard.alive) fileQualified = fileConfirmed && active && !guard.ticket && filePendingCurrent
    }
    onControllerChanged: retireLocal(true)
    onWorkspaceChanged: retireLocal(true)
    onExpandedChanged: if (!expanded) retireLocal(false)
    onVisibleChanged: if (!visible) retireLocal(false)
    onEnabledChanged: if (!enabled) retireLocal(false)
    onLatestJobIdChanged: latestChanged()
    Component.onDestruction: { lifetime.alive = false; if (fileDialog && fileDialog.dialogGuard) fileDialog.dialogGuard.active = false }
    Connections {
        target: root.workspace
        ignoreUnknownSignals: true
        function onStateChanged() { root.settle() }
        function onResultChanged() { root.settle() }
    }

    ColumnLayout {
        id: form
        width: root.width
        spacing: root.pad
        QGCLabel { Layout.fillWidth: true; text: qsTr("维护护照工作区 · 冻结与文件历史"); font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.expanded ? qsTr("收起维护工作区") : qsTr("展开维护工作区"); Accessible.name: qsTr("展开或收起维护工作区，不自动读写"); onClicked: root.expanded = !root.expanded }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本进程冻结快照与独立文件历史互不覆盖。所有文件路径需手选并另确认；浏览只取原生8项小页及连续2048 UTF16文本片，不向界面复制完整32MiB文件。没有自动保存、合并、复制、刷新、active导入或权限恢复。") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.stateValid ? qsTr("原生阶段 %1；忙 %2；job %3；取消已请求 %4。只有结果与实际线程均结束才释放槽。") .arg(root.state.stage).arg(root.flagText(root.busy)).arg(root.latestJobId || qsTr("无")).arg(root.flagText(root.state.cancelRequested)) : qsTr("原生工作区不可用或合同不完整；不会用现有JSON或其它来源补造。") }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("确认冻结当前护照"); checked: root.freezeConfirmed; enabled: root.canRequest; onToggled: root.freezeConfirmed = checked; Accessible.name: qsTr("明确确认冻结此时本进程实际维护记录，不保存文件") }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("冻结当前护照 · 不写文件"); enabled: root.canRequest && root.freezeConfirmed && root.freezeQualified; Accessible.name: qsTr("冻结此时本进程实际维护护照，不写文件"); property var press: null; onPressed: press = root.requestIntent("freeze", 0, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            QGCLabel { Layout.fillWidth: true; text: root.description(root.snapshots.session); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("选择冻结快照保存路径"); enabled: root.canRequest && root.frozenCurrent; Accessible.name: text; property var press: null; onPressed: press = root.pickerIntent("save"); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.openPicker(v) } }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("选择独立历史文件"); enabled: root.canRequest; Accessible.name: qsTr("选择现有维护文件，之后另行确认只读打开，不覆盖active护照"); property var press: null; onPressed: press = root.pickerIntent("open"); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.openPicker(v) } }
            QGCLabel { Layout.fillWidth: true; visible: root.filePendingCurrent; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.filePendingCurrent ? qsTr("仅暂存路径，未请求IO：%1\n动作：%2\n冻结ID：%3\nhash：%4\n保存将明确允许覆盖此路径的原文件；文件窗口的覆盖提示不能替代下方确认。") .arg(root.pendingFile.filePath).arg(root.pendingFile.action).arg(root.pendingFile.snapshot ? root.pendingFile.snapshot.dto.snapshotId : qsTr("仅独立读取，不覆盖active")) .arg(root.pendingFile.snapshot ? root.pendingFile.snapshot.dto.contentHash : qsTr("读取后由原生返回")) : "" }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.filePendingCurrent && root.pendingFile.action === "save" ? qsTr("确认保存并允许覆盖") : qsTr("确认独立只读打开"); checked: root.fileConfirmed; enabled: root.canRequest && root.filePendingCurrent; onToggled: root.fileConfirmed = checked; Accessible.name: text }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.filePendingCurrent && root.pendingFile.action === "save" ? qsTr("实际保存此冻结快照") : qsTr("实际打开为独立历史"); enabled: root.canRequest && root.filePendingCurrent && root.fileConfirmed && root.fileQualified; Accessible.name: text; property var press: null; onPressed: press = root.requestIntent(root.pendingFile ? root.pendingFile.action : "", 0, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("取消路径选择"); enabled: !!root.pendingFile || !!root.fileDialog; Accessible.name: qsTr("取消暂存路径与文件窗口，不删除文件"); onClicked: root.retireFile() }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("请求取消本页原job"); enabled: root.canCancel; Accessible.name: qsTr("仅取消本页原job，不回滚可能已提交的文件"); property var press: null; onPressed: press = root.cancelIntent(); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.cancel(v) } }
            QGCLabel { Layout.fillWidth: true; text: root.description(root.snapshots.file); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            QGCLabel { Layout.fillWidth: true; text: qsTr("明确选择浏览来源；不会自动选首组。相同机号不是同物理飞机或电池。文件历史永远不成为active护照。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.fillWidth: true
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("浏览冻结快照"); enabled: root.canRequest && root.frozenCurrent; Accessible.name: text; property var press: null; onPressed: press = root.selectionIntent("session", null); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.select(v) } }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("浏览文件历史"); enabled: root.canRequest && root.fileCurrent; Accessible.name: text; property var press: null; onPressed: press = root.selectionIntent("file", null); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.select(v) } }
            }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.selectedSnapshot ? qsTr("所选来源：%1；原快照槽仍同ID/hash：%2。这里只显示冻结时资料，非当前维护／飞行许可。") .arg(root.browser.origin === "session" ? qsTr("本进程冻结") : qsTr("独立文件历史")).arg(root.flagText(root.selectedCurrent)) : qsTr("尚未选择浏览来源。") }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("来源组首页 · 每页8组"); enabled: root.canRequest && root.selectedCurrent; Accessible.name: qsTr("读取所选快照的来源组首页，每页最多8组"); property var press: null; onPressed: press = root.requestIntent("groups", 0, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            Repeater {
                model: root.browser.groups ? root.browser.groups.rows : []
                QGCButton {
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight * 2; focusPolicy: Qt.StrongFocus
                    text: qsTr("选择来源组 · 机号 %1 · %2\n组ID %3\n记录 %4；维护 %5；启发风险 %6 / %7；置信 %8；必须人工复核") .arg(modelData.vehicleId).arg(modelData.historicalOnly ? qsTr("历史source7，不认证实物") : qsTr("手工legacy，不与历史合并")).arg(modelData.groupId).arg(modelData.recordCount).arg(modelData.maintenanceStatus).arg(modelData.predictiveRisk).arg(modelData.estimatedRiskScore).arg(modelData.confidence)
                    enabled: root.canRequest && root.selectedCurrent; Accessible.name: text; property var press: null
                    onPressed: press = root.selectionIntent("group", modelData); onCanceled: press = null
                    onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.select(v) }
                    contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: parent._showHighlight ? pal.buttonHighlightText : pal.buttonText; font.underline: parent.activeFocus }
                }
            }
            RowLayout {
                Layout.fillWidth: true; visible: !!root.browser.groups
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("来源组上一页"); enabled: root.canRequest && root.selectedCurrent && root.browser.groups && root.browser.groups.afterIndex > 0; property var press: null; onPressed: press = root.requestIntent("groups", Math.max(0, root.browser.groups.afterIndex - 8), -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("来源组下一页"); enabled: root.canRequest && root.selectedCurrent && root.browser.groups && root.browser.groups.hasMore; property var press: null; onPressed: press = root.requestIntent("groups", root.browser.groups.nextIndex, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            }
            QGCLabel { Layout.fillWidth: true; visible: !!root.browser.groups; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.browser.groups ? qsTr("来源组范围 %1→%2 / %3；是否还有下一页 %4。未展示页不会被当空组。") .arg(root.browser.groups.afterIndex).arg(root.browser.groups.nextIndex).arg(root.browser.groups.totalCount).arg(root.flagText(root.browser.groups.hasMore)) : "" }
            QGCLabel { Layout.fillWidth: true; visible: !!root.browser.group; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.browser.group ? qsTr("所选来源组 %1\n历史scope %2\n完整来源7：%3\n仅此来源组的评估；不按数字ID混合其它组。") .arg(root.browser.group.groupId).arg(root.browser.group.historyScopeId || qsTr("legacy手工组")) .arg(JSON.stringify(root.browser.group.historicalTelemetrySource)) : "" }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("本组记录首页 · 每页8条"); enabled: root.canRequest && root.selectedCurrent && !!root.browser.group; Accessible.name: text; property var press: null; onPressed: press = root.requestIntent("records", 0, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("查看本组完整评估"); enabled: root.canRequest && root.selectedCurrent && !!root.browser.group; Accessible.name: qsTr("连续浏览本组完整assessment与证据缺口文本"); property var press: null; onPressed: press = root.requestIntent("detail", 0, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            Repeater {
                model: root.browser.records ? root.browser.records.rows : []
                QGCButton {
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight * 2; focusPolicy: Qt.StrongFocus
                    text: qsTr("选择记录 · 组内索引 %1 / 原全局索引 %2\n原时间 %3（%4 ms）\n实际known指标 %5\n历史 %6；有改装项 %7；完整字段 %8。缺键未知，不把负温／0当无数据。") .arg(modelData.groupRecordIndex).arg(modelData.recordIndex).arg(modelData.timestamp).arg(modelData.timestampMs).arg(JSON.stringify(modelData.metrics)).arg(root.flagText(modelData.historicalOnly)).arg(root.flagText(modelData.hasModifications)).arg(modelData.fieldCount)
                    enabled: root.canRequest && root.selectedCurrent; Accessible.name: text; property var press: null
                    onPressed: press = root.selectionIntent("record", modelData); onCanceled: press = null
                    onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.select(v) }
                    contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: parent._showHighlight ? pal.buttonHighlightText : pal.buttonText; font.underline: parent.activeFocus }
                }
            }
            RowLayout {
                Layout.fillWidth: true; visible: !!root.browser.records
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("记录上一页"); enabled: root.canRequest && root.selectedCurrent && root.browser.records && root.browser.records.afterIndex > 0; property var press: null; onPressed: press = root.requestIntent("records", Math.max(0, root.browser.records.afterIndex - 8), -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("记录下一页"); enabled: root.canRequest && root.selectedCurrent && root.browser.records && root.browser.records.hasMore; property var press: null; onPressed: press = root.requestIntent("records", root.browser.records.nextIndex, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            }
            QGCLabel { Layout.fillWidth: true; visible: !!root.browser.records; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.browser.records ? qsTr("记录范围 %1→%2 / %3。完整单条必须另选记录后明确打开；不只显示下载按钮。") .arg(root.browser.records.afterIndex).arg(root.browser.records.nextIndex).arg(root.browser.records.totalCount) : "" }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("查看所选记录全部字段"); enabled: root.canRequest && root.selectedCurrent && !!root.browser.record; Accessible.name: qsTr("连续浏览所选记录全部规范字段、source与evidence文本"); property var press: null; onPressed: press = root.requestIntent("detail", 0, root.browser.record.recordIndex); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            QGCLabel { Layout.fillWidth: true; visible: !!root.browser.detail; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.browser.detail ? qsTr("完整规范JSON的连续文本片（不是执行脚本／原文件字节）\n对象：%1\nhash %2\nUTF16偏移 %3→%4 / %5；还有后续 %6。按actual nextOffset翻页，不截断完整数据。") .arg(root.browser.detail.recordIndex === -1 ? qsTr("本组assessment") : qsTr("原全局记录 %1").arg(root.browser.detail.recordIndex)).arg(root.browser.detail.contentHash).arg(root.browser.detail.offset).arg(root.browser.detail.nextOffset).arg(root.browser.detail.totalCharacters).arg(root.flagText(root.browser.detail.hasMore)) : "" }
            QGCLabel { Layout.fillWidth: true; visible: !!root.browser.detail; activeFocusOnTab: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.browser.detail ? root.browser.detail.text : ""; Accessible.role: Accessible.StaticText; Accessible.name: text }
            RowLayout {
                Layout.fillWidth: true; visible: !!root.browser.detail
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("文本上一片"); enabled: root.canRequest && root.selectedCurrent && root.browser.offsets.length > 0; Accessible.name: text; property var press: null; onPressed: press = root.requestIntent("detail", root.browser.offsets[root.browser.offsets.length - 1], root.browser.detail.recordIndex); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("文本下一片"); enabled: root.canRequest && root.selectedCurrent && root.browser.detail && root.browser.detail.hasMore; Accessible.name: text; property var press: null; onPressed: press = root.requestIntent("detail", root.browser.detail.nextOffset, root.browser.detail.recordIndex); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            }
            RowLayout {
                Layout.fillWidth: true
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("清冻结槽"); enabled: root.canRequest && root.frozenCurrent; Accessible.name: qsTr("仅清准确冻结槽，不清active护照"); property var press: null; onPressed: press = root.requestIntent("clear-frozen", 0, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("清文件槽"); enabled: root.canRequest && root.fileCurrent; Accessible.name: qsTr("仅清准确文件历史槽，不删除文件"); property var press: null; onPressed: press = root.requestIntent("clear-file", 0, -1); onCanceled: press = null; onClicked: { var g = root.lifetime, v = press; press = null; if (g.alive) root.submit(v) } }
            }
            QGCLabel { id: feedbackLabel; Layout.fillWidth: true; activeFocusOnTab: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.feedback || qsTr("尚未请求操作。展开不读盘、不冻结；两个来源均未认证，不恢复active或权限。"); Accessible.role: Accessible.StaticText; Accessible.name: text }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.saveText() }
        }
    }
    Component {
        id: openDialogComponent
        QGCFileDialog {
            id: openView
            property var ownerPanel: null
            property var panelGuard: null
            property var dialogGuard: ({alive: true, active: true})
            property var frozenIntent: null
            property bool activeIntent: true
            Component.onDestruction: dialogGuard.alive = false
            selectExisting: true; selectFolder: false; folder: ""
            nameFilters: [qsTr("维护护照 (*.json)")]
            title: qsTr("选择维护文件 · 返回后另确认独立读取")
            onAcceptedForLoad: if (panelGuard && panelGuard.alive && dialogGuard.alive && dialogGuard.active && activeIntent && ownerPanel) ownerPanel.fileChosen(openView, file)
            onRejected: if (panelGuard && panelGuard.alive && dialogGuard.alive && dialogGuard.active && activeIntent && ownerPanel) ownerPanel.fileRejected(openView)
        }
    }
    Component {
        id: saveDialogComponent
        QGCFileDialog {
            id: saveView
            property var ownerPanel: null
            property var panelGuard: null
            property var dialogGuard: ({alive: true, active: true})
            property var frozenIntent: null
            property bool activeIntent: true
            Component.onDestruction: dialogGuard.alive = false
            selectExisting: false; selectFolder: false; folder: ""
            nameFilters: [qsTr("维护护照 (*.json)")]
            title: qsTr("选择冻结快照保存路径 · 返回后确认覆盖")
            onAcceptedForSave: if (panelGuard && panelGuard.alive && dialogGuard.alive && dialogGuard.active && activeIntent && ownerPanel) ownerPanel.fileChosen(saveView, file)
            onRejected: if (panelGuard && panelGuard.alive && dialogGuard.alive && dialogGuard.active && activeIntent && ownerPanel) ownerPanel.fileRejected(saveView)
        }
    }
}

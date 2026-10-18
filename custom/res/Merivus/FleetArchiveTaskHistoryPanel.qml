import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Retained diagnostics only. Hash selection is never a task/control restore.
FocusScope {
    id: root
    property var archive: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, call: null})
    property bool callBusy: false
    property int scopeIndex: 0
    property double scopeRevision: 0
    property double selectionRevision: 0
    property bool revisionExhausted: false
    property string selectedTaskHash: ""
    property string localError: ""
    property var lastRequest: ({})
    property var ownedQuery: null
    property string cancelledQueryId: ""
    property string consumedDirectoryQueryId: ""
    property string consumedRecordsQueryId: ""
    property var savedDirectory: null
    property var savedRecords: null
    property bool showDetails: false

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var current: archive ? archive.current || {} : ({})
    readonly property var options: archive ? archive.options || {} : ({})
    readonly property var actualDirectory: archive ? archive.historyTasks || {} : ({})
    readonly property var actualRecords: archive ? archive.historyTaskRecords || {} : ({})
    readonly property string runId: typeof current.runId === "string" ? current.runId : ""
    readonly property string optionsStamp: optionStamp(options)
    readonly property string scope: scopeIndex === 0 ? "sealed" : scopeIndex === 1 ? "sealed-and-offline" : "invalid"
    readonly property bool sourceReady: !!archive && lifetime.alive && !lifetime.exhausted && !revisionExhausted
        && optionsStamp.length > 0 && typeof archive.tasks === "function" && typeof archive.request === "function"
        && typeof current.historyRequestPending === "boolean" && typeof current.archivedReaderActive === "boolean"
        && typeof current.archiveMaintenanceActive === "boolean"
    readonly property bool historyBusy: current.historyRequestPending === true || current.archivedReaderActive === true
        || current.archiveMaintenanceActive === true
    readonly property bool idle: sourceReady && !callBusy && !historyBusy && scope !== "invalid"
        && current.writerActive === false && current.captureEnabled === false
    readonly property bool directoryCurrent: !!savedDirectory && savedCurrent(savedDirectory)
        && savedDirectory.nativeResult === actualDirectory && savedDirectory.dto.scope === scope
        && savedDirectory.scopeRevision === scopeRevision
    readonly property var taskRows: savedDirectory ? savedDirectory.dto.tasks : []
    readonly property var selectedTask: findTask(taskRows, selectedTaskHash)
    readonly property bool recordsCurrent: !!savedRecords && directoryCurrent && savedCurrent(savedRecords)
        && savedRecords.nativeResult === actualRecords && sameDirectoryBasis(savedRecords.directory, savedDirectory)
        && savedRecords.scopeRevision === scopeRevision && savedRecords.selectionRevision === selectionRevision
        && savedRecords.dto.taskIdHash === selectedTaskHash && !!selectedTask
    readonly property bool canContinueDirectory: idle && directoryCurrent && savedDirectory.dto.canContinue === true
        && savedDirectory.requestId !== consumedDirectoryQueryId
    readonly property bool canRecords: idle && directoryCurrent && !!selectedTask
    readonly property bool canContinueRecords: canRecords && recordsCurrent && savedRecords.dto.canContinue === true
        && savedRecords.requestId !== consumedRecordsQueryId
    readonly property bool canCancel: sourceReady && !callBusy && historyBusy && !!ownedQuery && !ownedQuery.adopted
        && ownedQuery.owner === archive && ownedQuery.optionsStamp === optionsStamp && ownedQuery.runId === runId
        && uuid(ownedQuery.requestId) && current.historyRequestKind === "search"
        && map(current.archivedReader) && current.archivedReader.queryId === ownedQuery.requestId
        && cancelledQueryId !== ownedQuery.requestId
    readonly property var recordRows: savedRecords && Array.isArray(savedRecords.dto.searchResult.rows) ? savedRecords.dto.searchResult.rows : []

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: form.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function map(value) { return !!value && typeof value === "object" && !Array.isArray(value) }
    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function decimal(value) { return typeof value === "string" && /^(0|[1-9][0-9]{0,18})$/.test(value) && (value.length < 19 || value <= "9223372036854775807") }
    function greater(first, second) { return decimal(first) && decimal(second) && (first.length > second.length || (first.length === second.length && first > second)) }
    function text(value) { return typeof value === "string" && value.length ? value : qsTr("未知 / 未记录") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function optionStamp(value) {
        if (!map(value) || Object.keys(value).length !== 7 || value.schemaVersion !== 1
                || typeof value.directoryPath !== "string" || !value.directoryPath.length || value.directoryPath.length > 4096
                || !map(value.policy) || Object.keys(value.policy).length !== 7
                || !integer(value.maxSealedSegments, 1, 128) || !integer(value.pageSize, 1, 256)
                || value.defaultEnabled !== false || value.storesRawPayload !== false) return ""
        try { return JSON.stringify([value.directoryPath, value.policy, value.maxSealedSegments, value.pageSize, false, false]) }
        catch (error) { return "" }
    }
    function copyValue(value) {
        var budget = {nodes: 0, chars: 0}
        function copy(item, depth) {
            if (depth > 16 || ++budget.nodes > 131072) throw new Error("task_history_value_limit")
            if (item === null || typeof item === "boolean") return item
            if (typeof item === "number") { if (!isFinite(item) || Math.abs(item) > 9007199254740991) throw new Error("task_history_number_invalid"); return item }
            if (typeof item === "string") { budget.chars += item.length; if (budget.chars > 2097152) throw new Error("task_history_text_limit"); return item }
            if (!item || typeof item !== "object") throw new Error("task_history_value_invalid")
            if (Array.isArray(item) || typeof item.length === "number") {
                if (!integer(item.length, 0, 256)) throw new Error("task_history_array_limit")
                var rows = []; for (var i = 0; i < item.length; ++i) rows.push(copy(item[i], depth + 1)); return rows
            }
            var keys = Object.keys(item), result = {}
            if (keys.length > 64) throw new Error("task_history_map_limit")
            // The native decoder keeps its canonical no-type nulls. Qt can
            // expose only these two validated absence slots as undefined.
            var exitKeys = ["schemaVersion", "observed", "count", "candidate", "first", "last", "requestCausalityVerified", "remoteOutcomeVerified", "automaticActionsIssued"]
            var absentExit = keys.length === exitKeys.length && item.schemaVersion === 1 && item.observed === false
                && item.count === "0" && item.candidate === "not-observed" && item.first == null && item.last == null
                && item.requestCausalityVerified === false && item.remoteOutcomeVerified === false && item.automaticActionsIssued === false
            if (absentExit) for (var e = 0; e < exitKeys.length; ++e) if (!Object.prototype.hasOwnProperty.call(item, exitKeys[e])) absentExit = false
            for (var k = 0; k < keys.length; ++k) {
                var key = keys[k]
                if (key.length > 128 || ["__proto__", "constructor", "prototype", "toJSON"].indexOf(key) >= 0) throw new Error("task_history_key_invalid")
                budget.chars += key.length; if (budget.chars > 2097152) throw new Error("task_history_text_limit")
                result[key] = copy(absentExit && (key === "first" || key === "last") ? null : item[key], depth + 1)
            }
            return result
        }
        try { return copy(value, 0) } catch (error) { return null }
    }
    function inert(value) {
        if (!map(value) || value.historicalOnly !== true) return false
        var flags = ["permissionsRestored", "authenticated", "signatureVerified", "executable", "completionVerified",
            "executionPermit", "flightCommandReleased", "restoresAuthorization", "historicalCompletenessVerified",
            "crossSegmentAtomicSnapshot", "wallClockOrderingUsed"]
        for (var i = 0; i < flags.length; ++i) if (value[flags[i]] !== false) return false
        return true
    }
    function envelopeKnown(value, query, records) {
        return inert(value) && Object.keys(value).length === (records ? 31 : 35) && value.schemaVersion === 1
            && value.format === (records ? "merivus-fleet-retained-task-records" : "merivus-fleet-retained-task-directory")
            && uuid(value.directoryId) && value.queryId === query.requestId && value.runId === query.runId
            && value.scope === query.intent.scope && digest(value.inventoryFingerprint)
            && ["partial", "completed", "budget-reached"].indexOf(value.status) >= 0
            && typeof value.scanComplete === "boolean" && typeof value.canContinue === "boolean"
            && typeof value.partial === "boolean" && typeof value.budgetReached === "boolean"
            && value.partial === !value.scanComplete && value.canContinue === (!value.scanComplete && !value.budgetReached)
            && value.status === (value.budgetReached ? "budget-reached" : value.scanComplete ? "completed" : "partial")
            && (!value.budgetReached || !value.scanComplete)
            && (value.budgetReached ? ["archive-local-tasks-batch-budget-reached", "archive-local-tasks-row-budget-reached"].indexOf(value.errorCode) >= 0 : value.errorCode === "")
            && integer(value.batchCount, 1, 2147483647)
    }
    function directoryRejectedBatchKnown(value, query) {
        if (!query.previous || query.intent.action !== "continue-tasks") return false
        var prior = query.previous.dto, keys = Object.keys(prior)
        for (var i = 0; i < keys.length; ++i) {
            var key = keys[i]
            if (["queryId", "status", "errorCode", "scanComplete", "canContinue", "partial", "budgetReached"].indexOf(key) < 0
                    && JSON.stringify(value[key]) !== JSON.stringify(prior[key])) return false
        }
        return true
    }
    function directoryKnown(value, query) {
        if (!envelopeKnown(value, query, false) || !Array.isArray(value.tasks) || value.tasks.length > 256
                || value.taskCount !== value.tasks.length || !integer(value.scannedRows, 0, 8192)
                || !integer(value.unattributedRows, 0, value.scannedRows) || !integer(value.warningRows, 0, value.scannedRows)
                || !integer(value.clockDiscontinuityRows, 0, value.scannedRows) || !integer(value.retentionWarningPages, 0, 2147483647)
                || !integer(value.filesTotal, value.scope === "sealed-and-offline" ? 1 : 0, 129)
                || !integer(value.filesCompleted, 0, value.filesTotal)
                || (value.scanComplete && value.filesCompleted !== value.filesTotal)
                || (query.intent.action === "tasks" && (value.batchCount !== 1 || value.errorCode === "archive-local-tasks-batch-budget-reached"))
                || (value.errorCode === "archive-local-tasks-row-budget-reached" && value.scannedRows !== 8192)
                || (query.intent.action === "continue-tasks" && (!query.previous
                    || value.directoryId !== query.previous.dto.directoryId || value.inventoryFingerprint !== query.previous.dto.inventoryFingerprint
                    || value.scannedRows < query.previous.dto.scannedRows
                    || (value.errorCode === "archive-local-tasks-batch-budget-reached" ? !directoryRejectedBatchKnown(value, query)
                        : value.batchCount !== query.previous.dto.batchCount + 1)))) return false
        var previous = "", total = value.unattributedRows
        for (var i = 0; i < value.tasks.length; ++i) {
            var row = value.tasks[i]
            if (!map(row) || Object.keys(row).length !== 9 || !digest(row.taskIdHash) || row.taskIdHash <= previous
                    || !integer(row.eventCount, 1, value.scannedRows) || !Array.isArray(row.vehicleIds) || row.vehicleIds.length > 255
                    || typeof row.vehicleAssociationKnown !== "boolean" || row.vehicleAssociationKnown !== (row.vehicleIds.length > 0)
                    || !map(row.eventCategories) || Object.keys(row.eventCategories).length > 8
                    || !integer(row.queuedTrueRows, 0, row.eventCount) || !integer(row.uncertainTrueRows, 0, row.eventCount)
                    || !integer(row.clockDiscontinuityRows, 0, row.eventCount) || !integer(row.warningRows, 0, row.eventCount)) return false
            var priorId = 0, categories = Object.keys(row.eventCategories), categoryTotal = 0
            for (var v = 0; v < row.vehicleIds.length; ++v) {
                if (!integer(row.vehicleIds[v], priorId + 1, 255)) return false
                priorId = row.vehicleIds[v]
            }
            for (var c = 0; c < categories.length; ++c) {
                if (["command", "transaction", "task", "formation", "fleet", "intent", "maintenance", "other"].indexOf(categories[c]) < 0
                        || !integer(row.eventCategories[categories[c]], 1, row.eventCount)) return false
                categoryTotal += row.eventCategories[categories[c]]
            }
            if (categoryTotal !== row.eventCount) return false
            total += row.eventCount; previous = row.taskIdHash
        }
        return total === value.scannedRows
    }
    function fileKnown(value, total, valueScope) {
        if (!map(value) || !integer(value.fileIndex, 0, total - 1) || !uuid(value.databaseEpoch)
                || !digest(value.fileFingerprint) || typeof value.offlineCurrentOnly !== "boolean") return false
        return value.offlineCurrentOnly ? valueScope === "sealed-and-offline" && value.basename === "fleet-events-v1.sqlite" && value.sealRunId === ""
            : uuid(value.sealRunId) && value.basename === "fleet-events-sealed-" + value.databaseEpoch + "-" + value.sealRunId + ".sqlite"
    }
    function sameFile(first, second) {
        return first.fileIndex === second.fileIndex && first.basename === second.basename && first.databaseEpoch === second.databaseEpoch
            && first.sealRunId === second.sealRunId && first.fileFingerprint === second.fileFingerprint && first.offlineCurrentOnly === second.offlineCurrentOnly
    }
    function recordsKnown(value, query) {
        var search = value && value.searchResult, directory = query.directory
        if (!envelopeKnown(value, query, true) || !directory || value.directoryId !== directory.dto.directoryId
                || value.inventoryFingerprint !== directory.dto.inventoryFingerprint || value.taskIdHash !== query.intent.taskIdHash
                || !digest(value.taskIdHash) || !map(search)) return false
        // The native whole-batch budget rejection publishes NO new rows. It
        // deliberately retains only the previous whole-batch cumulative count.
        if (value.errorCode === "archive-local-tasks-batch-budget-reached") {
            return query.intent.action === "continue-task-records" && !!query.previous
                && Object.keys(search).length === 0 && value.scannedRows === 0 && value.matchedRows === 0
                && value.cumulativeScannedRows === query.previous.dto.cumulativeScannedRows && value.batchCount === query.previous.dto.batchCount
                && value.budgetReached === true && value.scanComplete === false && value.canContinue === false
        }
        if (!inert(search) || Object.keys(search).length !== 29
                || search.schemaVersion !== 2 || search.format !== "merivus-fleet-event-history-search"
                || search.scope !== value.scope || search.taskIdHash !== value.taskIdHash || search.vehicleId !== 0 || search.operation !== ""
                || search.inventoryFingerprint !== value.inventoryFingerprint || search.ordering !== "basename-then-row-not-flight-time"
                || !integer(search.scannedRows, 0, 256) || !integer(search.matchedRows, 0, 64)
                || value.scannedRows !== search.scannedRows || value.matchedRows !== search.matchedRows
                || !integer(value.cumulativeScannedRows, 0, 8192) || value.cumulativeScannedRows < value.scannedRows
                || (query.intent.action === "task-records" && (value.batchCount !== 1 || value.cumulativeScannedRows !== value.scannedRows))
                || (value.errorCode === "archive-local-tasks-row-budget-reached" && value.cumulativeScannedRows !== 8192)
                || value.scanComplete !== search.scanComplete || !Array.isArray(search.rows) || search.rows.length !== search.matchedRows
                || !Array.isArray(search.scannedPages) || search.scannedPages.length > 4
                || !integer(search.filesTotal, value.scope === "sealed-and-offline" ? 1 : 0, 129)
                || !integer(search.filesCompleted, 0, search.filesTotal) || !map(search.nextCursor)
                || (query.intent.action === "continue-task-records" && (!query.previous
                    || value.cumulativeScannedRows !== query.previous.dto.cumulativeScannedRows + value.scannedRows
                    || value.batchCount !== query.previous.dto.batchCount + 1))) return false
        if (search.scanComplete) {
            if (Object.keys(search.nextCursor).length !== 0 || search.filesCompleted !== search.filesTotal) return false
        } else {
            var cursor = search.nextCursor
            if (Object.keys(cursor).length !== 5 || cursor.inventoryFingerprint !== value.inventoryFingerprint
                    || !integer(cursor.fileIndex, 0, search.filesTotal - 1) || cursor.fileIndex !== search.filesCompleted || !decimal(cursor.afterRowId)
                    || (cursor.afterRowId === "0" ? cursor.fileFingerprint !== "" || cursor.databaseEpoch !== ""
                        : !digest(cursor.fileFingerprint) || !uuid(cursor.databaseEpoch))) return false
        }
        var scanned = 0, matched = 0
        for (var p = 0; p < search.scannedPages.length; ++p) {
            var page = search.scannedPages[p]
            if (!fileKnown(page, search.filesTotal, value.scope) || Object.keys(page).length !== 18
                    || !decimal(page.afterRowId) || !decimal(page.nextRowId) || typeof page.hasMore !== "boolean"
                    || (page.nextRowId !== page.afterRowId && !greater(page.nextRowId, page.afterRowId))
                    || !decimal(page.retainedFromRowId) || !decimal(page.retainedThroughRowId) || !decimal(page.retiredCount)
                    || typeof page.cursorPrecedesRetainedHistory !== "boolean" || !integer(page.scannedRows, 0, 64)
                    || !integer(page.matchedRows, 0, page.scannedRows) || !integer(page.uncleanRows, 0, page.scannedRows)
                    || !integer(page.rowsWithDroppedEvents, 0, page.scannedRows) || !integer(page.rowsWithDiscardedEvents, 0, page.scannedRows)) return false
            scanned += page.scannedRows; matched += page.matchedRows
        }
        if (scanned !== value.scannedRows || matched !== value.matchedRows) return false
        var previousFile = -1, previousRow = "0"
        for (var r = 0; r < search.rows.length; ++r) {
            var entry = search.rows[r], row = map(entry) ? entry.row : null, record = map(row) ? row.record : null
            var payload = map(record) && map(record.event) ? record.event.payload : null
            if (!fileKnown(entry, search.filesTotal, value.scope) || Object.keys(entry).length !== 7 || !map(row) || Object.keys(row).length !== 8
                    || !greater(row.rowId, "0") || !digest(row.contentHash) || !map(record) || record.historicalOnly !== true
                    || record.authenticated !== false || record.restoresAuthorization !== false || record.flightCommandReleased !== false
                    || !map(payload) || payload.taskIdHash !== value.taskIdHash
                    || entry.fileIndex < previousFile || (entry.fileIndex === previousFile && !greater(row.rowId, previousRow))) return false
            var contained = false
            for (var c = 0; c < search.scannedPages.length; ++c) {
                var range = search.scannedPages[c]
                if (sameFile(entry, range) && greater(row.rowId, range.afterRowId)
                        && (row.rowId === range.nextRowId || greater(range.nextRowId, row.rowId))) contained = true
            }
            if (!contained) return false
            previousFile = entry.fileIndex; previousRow = row.rowId
        }
        return true
    }
    function findTask(rows, hash) { for (var i = 0; i < rows.length; ++i) if (rows[i].taskIdHash === hash) return rows[i]; return null }
    function savedCurrent(value) { return value.owner === archive && value.optionsStamp === optionsStamp && value.runId === runId && value.epoch === lifetime.epoch }
    function sameDirectoryBasis(first, second) {
        return !!first && !!second && savedCurrent(first) && savedCurrent(second)
            && first.scopeRevision === second.scopeRevision && first.scopeRevision === scopeRevision
            && first.dto.directoryId === second.dto.directoryId && first.dto.inventoryFingerprint === second.dto.inventoryFingerprint
            && first.dto.scope === second.dto.scope && first.dto.runId === second.dto.runId
    }
    function pressSnapshot(action) {
        var continuation = action === "continue-tasks" || action === "continue-task-records"
        var records = action === "task-records" || action === "continue-task-records"
        var previous = continuation ? (records ? savedRecords : savedDirectory) : null
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, scopeRevision: scopeRevision,
            selectionRevision: selectionRevision, directory: records ? savedDirectory : null, previous: previous,
            nativeResult: previous ? previous.nativeResult : null,
            intent: {action: action, expectedRunId: runId, scope: scope, previousQueryId: previous ? previous.requestId : "",
                directoryId: records ? (savedDirectory ? savedDirectory.dto.directoryId : "") : previous ? previous.dto.directoryId : "",
                taskIdHash: records ? selectedTaskHash : ""}}
    }
    function pressCurrent(pressed, consumed) {
        if (!pressed || !sourceReady || historyBusy || current.writerActive !== false || current.captureEnabled !== false
                || pressed.owner !== archive || pressed.epoch !== lifetime.epoch || pressed.optionsStamp !== optionsStamp
                || pressed.scopeRevision !== scopeRevision || pressed.intent.scope !== scope || scope === "invalid"
                || pressed.intent.expectedRunId !== runId) return false
        var action = pressed.intent.action
        if (action === "tasks") return !pressed.previous && pressed.intent.previousQueryId === "" && pressed.intent.directoryId === "" && pressed.intent.taskIdHash === ""
        if (action === "continue-tasks") return directoryCurrent && savedDirectory.dto.canContinue && pressed.previous === savedDirectory
            && pressed.nativeResult === actualDirectory && pressed.intent.previousQueryId === savedDirectory.requestId
            && pressed.intent.directoryId === savedDirectory.dto.directoryId && pressed.intent.taskIdHash === ""
            && (consumed || savedDirectory.requestId !== consumedDirectoryQueryId)
        if ((action !== "task-records" && action !== "continue-task-records") || !directoryCurrent || !selectedTask
                || pressed.directory !== savedDirectory || pressed.selectionRevision !== selectionRevision
                || pressed.intent.directoryId !== savedDirectory.dto.directoryId || pressed.intent.taskIdHash !== selectedTaskHash) return false
        return action === "task-records" ? !pressed.previous && pressed.intent.previousQueryId === ""
            : recordsCurrent && savedRecords.dto.canContinue && pressed.previous === savedRecords && pressed.nativeResult === actualRecords
                && pressed.intent.previousQueryId === savedRecords.requestId && (consumed || savedRecords.requestId !== consumedRecordsQueryId)
    }
    function receiptKnown(value, intent) {
        return map(value) && Object.keys(value).length === 9 && value.schemaVersion === 1 && value.action === intent.action
            && typeof value.accepted === "boolean" && typeof value.reason === "string" && typeof value.requestId === "string"
            && (uuid(value.requestId) || (!value.accepted && value.requestId === "")) && value.requestOnly === true
            && value.historicalOnly === true && value.permissionsRestored === false && value.flightCommandReleased === false
            && (intent.action !== "cancel" || value.requestId === intent.queryId || (!value.accepted && value.requestId === ""))
    }
    function submit(pressed) {
        var life = lifetime
        if (!life.alive || life.exhausted || life.call || callBusy || !pressCurrent(pressed, false)) {
            if (life.alive) localError = qsTr("来源、范围、所选任务或原查询已变化，未发送。请核对当前状态并从头读取。")
            return
        }
        var owner = pressed.owner, epoch = life.epoch, ticket = {}, query = null
        life.call = ticket
        try {
            callBusy = true
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || archive !== owner) return
            localError = ""
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !pressCurrent(pressed, false)) return
            var records = pressed.intent.action === "task-records" || pressed.intent.action === "continue-task-records"
            query = {owner: owner, epoch: epoch, optionsStamp: pressed.optionsStamp, runId: pressed.intent.expectedRunId, requestId: "", intent: pressed.intent,
                scopeRevision: pressed.scopeRevision, selectionRevision: pressed.selectionRevision, directory: pressed.directory,
                previous: pressed.previous, records: records, adopted: false}
            if (records) consumedRecordsQueryId = savedRecords ? savedRecords.requestId : ""
            else consumedDirectoryQueryId = savedDirectory ? savedDirectory.requestId : ""
            if (!life.alive || life.call !== ticket || life.epoch !== epoch) return
            ownedQuery = query
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || owner !== archive || ownedQuery !== query || !pressCurrent(pressed, true)) return
            var receipt = null
            try { receipt = owner.tasks(pressed.intent, true) } catch (error) { receipt = null }
            if (!life.alive) return
            var value = copyValue(receipt)
            if (!receiptKnown(value, pressed.intent)) { localError = qsTr("未取得可核实回执，请求效果未知；不自动重试或沿用已消耗的续页。"); return }
            lastRequest = value
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || owner !== archive || ownedQuery !== query || optionsStamp !== pressed.optionsStamp) return
            if (!value.accepted) { localError = qsTr("请求未被接受：%1。旧结果只供历史阅读；请核对后重新开始该查询。").arg(text(value.reason)); return }
            query = {owner: owner, epoch: epoch, optionsStamp: pressed.optionsStamp, runId: pressed.intent.expectedRunId, requestId: value.requestId, intent: pressed.intent,
                scopeRevision: pressed.scopeRevision, selectionRevision: pressed.selectionRevision, directory: pressed.directory,
                previous: pressed.previous, records: records, adopted: false}
            ownedQuery = query
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || ownedQuery !== query) return
            cancelledQueryId = ""
            if (life.alive && life.call === ticket && life.epoch === epoch && ownedQuery === query) adoptResult()
        } finally { if (life.alive && life.call === ticket) { life.call = null; callBusy = false } }
    }
    function adoptResult() {
        var life = lifetime, query = ownedQuery, owner = archive, stamp = optionsStamp
        if (!life.alive || !query || query.adopted || query.owner !== owner || query.epoch !== life.epoch || query.optionsStamp !== stamp || query.runId !== runId
                || query.scopeRevision !== scopeRevision || query.intent.scope !== scope || !uuid(query.requestId)
                || query.requestId === cancelledQueryId || historyBusy || current.archivedResultAccepted !== true
                || current.lastHistoryQueryId !== query.requestId || (query.records && (query.directory !== savedDirectory
                    || !directoryCurrent || query.selectionRevision !== selectionRevision || query.intent.taskIdHash !== selectedTaskHash))) return
        var original = query.records ? actualRecords : actualDirectory, value = copyValue(original)
        if (!(query.records ? recordsKnown(value, query) : directoryKnown(value, query))) return
        if (!life.alive || ownedQuery !== query || owner !== archive || query.epoch !== life.epoch || stamp !== optionsStamp || query.runId !== runId || historyBusy
                || query.scopeRevision !== scopeRevision || current.lastHistoryQueryId !== query.requestId
                || current.archivedResultAccepted !== true || original !== (query.records ? actualRecords : actualDirectory)
                || (query.records && (query.directory !== savedDirectory || !directoryCurrent
                    || query.selectionRevision !== selectionRevision || query.intent.taskIdHash !== selectedTaskHash))) return
        query.adopted = true
        var saved = {owner: owner, epoch: query.epoch, optionsStamp: stamp, runId: query.runId, requestId: query.requestId, nativeResult: original, dto: value,
            scopeRevision: query.scopeRevision, selectionRevision: query.selectionRevision, directory: query.directory}
        if (query.records) savedRecords = saved
        else savedDirectory = saved
    }
    function cancelSnapshot() {
        if (!canCancel) return null
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, query: ownedQuery,
            intent: {action: "cancel", expectedRunId: runId, catalogId: "", basename: "", afterRowId: "0", queryId: ownedQuery.requestId}}
    }
    function cancelCurrent(pressed) {
        return pressed && lifetime.alive && sourceReady && pressed.owner === archive && pressed.epoch === lifetime.epoch
            && pressed.optionsStamp === optionsStamp && pressed.query === ownedQuery && ownedQuery && !ownedQuery.adopted
            && pressed.intent.expectedRunId === runId && pressed.intent.queryId === ownedQuery.requestId
            && cancelledQueryId !== ownedQuery.requestId && historyBusy && current.historyRequestKind === "search"
            && map(current.archivedReader) && current.archivedReader.queryId === ownedQuery.requestId
    }
    function cancel(pressed) {
        var life = lifetime
        if (!life.alive || life.call || callBusy || !cancelCurrent(pressed)) return
        var ticket = {}, epoch = life.epoch, owner = pressed.owner
        life.call = ticket
        try {
            callBusy = true
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !cancelCurrent(pressed)) return
            var receipt = null
            try { receipt = owner.request(pressed.intent, true) } catch (error) { receipt = null }
            if (!life.alive) return
            var value = copyValue(receipt)
            if (!receiptKnown(value, pressed.intent)) { localError = qsTr("取消结果未知；等待实际读取器结束，不自动重试。"); return }
            lastRequest = value
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || archive !== owner || ownedQuery !== pressed.query || optionsStamp !== pressed.optionsStamp) return
            if (value.accepted) cancelledQueryId = value.requestId
            else localError = qsTr("取消请求未被接受：%1。原生槽是否释放仍以实际状态为准。").arg(text(value.reason))
        } finally { if (life.alive && life.call === ticket) { life.call = null; callBusy = false } }
    }
    function statusText() {
        if (!ownedQuery) return qsTr("尚无本页持有的查询；展开和选择不会读盘。")
        var reader = current.archivedReader, result = qsTr("本页请求：%1\n查询ID：%2\n实际读取器仍忙：%3").arg(ownedQuery.intent.action).arg(text(ownedQuery.requestId)).arg(known(historyBusy))
        if (map(reader) && reader.queryId === ownedQuery.requestId) result += qsTr("\n原生读取器状态：%1 · 错误：%2").arg(text(reader.state)).arg(reader.errorCode === "" ? qsTr("未报告") : text(reader.errorCode))
        else result += "\n" + qsTr("当前读取器快照不属于本页请求；不冒用其他入口的状态或错误。")
        var nativeValue = ownedQuery.records ? actualRecords : actualDirectory
        if (map(nativeValue) && nativeValue.queryId === ownedQuery.requestId) result += qsTr("\n本次Store诊断状态：%1 · 诊断码：%2").arg(text(nativeValue.status)).arg(nativeValue.errorCode === "" ? qsTr("未报告") : text(nativeValue.errorCode))
        if (current.lastHistoryQueryId === ownedQuery.requestId && current.archivedResultErrorCode) result += qsTr("\n原生接纳错误：%1").arg(text(current.archivedResultErrorCode))
        if (cancelledQueryId === ownedQuery.requestId) result += "\n" + qsTr("已请求取消；不代表线程已结束。该查询不得续批。")
        return result
    }
    function taskText(row) {
        return qsTr("%1任务哈希 %2\n已扫描保留事件行数 %3 · 曾明确观察到的关联机号：%4\n类别计数：%5\n已保留且明确 queued=true / uncertain=true 行：%6 / %7\n时钟异常 / 留存警告行：%8 / %9\n零计数不代表全部为false；机号列表不证明任务成员完整。仅诊断行数，不是执行次数、任务成功率、完成率或当前状态。")
            .arg(selectedTaskHash === row.taskIdHash ? qsTr("已选 · ") : "").arg(row.taskIdHash).arg(row.eventCount)
            .arg(row.vehicleAssociationKnown ? row.vehicleIds.join(", ") : qsTr("未知；不按时间或机号猜关联"))
            .arg(JSON.stringify(row.eventCategories)).arg(row.queuedTrueRows).arg(row.uncertainTrueRows).arg(row.clockDiscontinuityRows).arg(row.warningRows)
    }
    function modeExitText(value, mask, committed) {
        function exact(item, keys) {
            if (!map(item) || Object.keys(item).length !== keys.length) return false
            for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(item, keys[i])) return false
            return true
        }
        function counter(item, zero) {
            return typeof item === "string" && /^(0|[1-9][0-9]{0,19})$/.test(item)
                && (zero || item !== "0") && (item.length < 20 || item <= "18446744073709551615")
        }
        function compare(a, b) { return a.length !== b.length ? (a.length < b.length ? -1 : 1) : a === b ? 0 : a < b ? -1 : 1 }
        // At most twenty decimal digits: never round uint64 receipts through JS Number.
        function difference(a, b) {
            var result = "", borrow = 0
            for (var i = a.length - 1, j = b.length - 1; i >= 0; --i, --j) {
                var digit = Number(a.charAt(i)) - (j >= 0 ? Number(b.charAt(j)) : 0) - borrow
                borrow = digit < 0 ? 1 : 0; result = String(digit + borrow * 10) + result
            }
            return result.replace(/^0+(?=[0-9])/, "")
        }
        function half(a) {
            var result = "", remainder = 0
            for (var i = 0; i < a.length; ++i) { var digit = remainder * 10 + Number(a.charAt(i)); result += String(Math.floor(digit / 2)); remainder = digit % 2 }
            return result.replace(/^0+(?=[0-9])/, "")
        }
        function receipt(item) {
            return exact(item, ["sourceEpoch", "sourceSequence", "receivedMonotonicMs"]) && uuid(item.sourceEpoch)
                && counter(item.sourceSequence, false) && decimal(item.receivedMonotonicMs)
        }
        function exit(item) {
            if (!exact(item, ["customMode", "baseMode", "systemStatus", "priorMissionReceipt", "exitReceipt", "continuityUnknownAtExit"])
                    || !integer(item.customMode, 0, 4294967295) || !integer(item.baseMode, 0, 255)
                    || !integer(item.systemStatus, 0, 8) || typeof item.continuityUnknownAtExit !== "boolean"
                    || !receipt(item.priorMissionReceipt) || !receipt(item.exitReceipt)
                    || item.priorMissionReceipt.sourceEpoch !== item.exitReceipt.sourceEpoch
                    || compare(item.priorMissionReceipt.sourceSequence, item.exitReceipt.sourceSequence) >= 0
                    || compare(item.priorMissionReceipt.receivedMonotonicMs, item.exitReceipt.receivedMonotonicMs) > 0) return false
            return !((item.baseMode & 1) !== 0 && item.customMode % 65536 === 0
                && Math.floor(item.customMode / 65536) % 256 === 4 && Math.floor(item.customMode / 16777216) === 4)
        }
        function sameReceipt(a, b) { return a.sourceEpoch === b.sourceEpoch && a.sourceSequence === b.sourceSequence && a.receivedMonotonicMs === b.receivedMonotonicMs }
        function receiptText(item) { return qsTr("本地epoch %1 · 序号 %2 · 单调ms %3").arg(item.sourceEpoch).arg(item.sourceSequence).arg(item.receivedMonotonicMs) }
        function exitText(item) {
            return qsTr("customMode=%1 · baseMode=%2 · systemStatus=%3\n此前任务模式收件：%4\n退出收件：%5\n该次连续性未知：%6")
                .arg(item.customMode).arg(item.baseMode).arg(item.systemStatus).arg(receiptText(item.priorMissionReceipt))
                .arg(receiptText(item.exitReceipt)).arg(known(item.continuityUnknownAtExit))
        }
        if (!exact(value, ["schemaVersion", "observed", "count", "candidate", "first", "last", "requestCausalityVerified", "remoteOutcomeVerified", "automaticActionsIssued"])
                || value.schemaVersion !== 1 || typeof value.observed !== "boolean" || !counter(value.count, true)
                || value.observed !== (value.count !== "0") || value.observed !== ((mask & 4) !== 0)
                || value.requestCausalityVerified !== false || value.remoteOutcomeVerified !== false || value.automaticActionsIssued !== false) return null
        if (!value.observed) return value.candidate === "not-observed" && value.first === null && value.last === null
            ? qsTr("未观察到提交后任务模式退出收件对；不证明持续执行或完成。") : null
        if (!committed || value.candidate !== "mission-mode-exit-observed-not-outcome" || !exit(value.first) || !exit(value.last)
                || value.first.exitReceipt.sourceEpoch !== value.last.exitReceipt.sourceEpoch) return null
        var first = value.first, last = value.last
        if (value.count === "1") {
            if (first.customMode !== last.customMode || first.baseMode !== last.baseMode || first.systemStatus !== last.systemStatus
                    || first.continuityUnknownAtExit !== last.continuityUnknownAtExit || !sameReceipt(first.priorMissionReceipt, last.priorMissionReceipt)
                    || !sameReceipt(first.exitReceipt, last.exitReceipt)) return null
        } else if (compare(first.exitReceipt.sourceSequence, last.priorMissionReceipt.sourceSequence) >= 0
                   || compare(first.exitReceipt.receivedMonotonicMs, last.priorMissionReceipt.receivedMonotonicMs) > 0
                   || compare(difference(value.count, "1"), half(difference(last.exitReceipt.sourceSequence, first.exitReceipt.sourceSequence))) > 0) return null
        return qsTr("提交后任务模式退出观察 %1 次（非结果候选）\n首次：%2\n末次：%3\n仅保留首末收件对，非完整心跳流；本地epoch不是远端身份认证，未验证请求因果或远端结果，未自动发动作。")
            .arg(value.count).arg(exitText(first)).arg(exitText(last))
    }
    function runObservationText(payload) {
        if (!map(payload) || ["fixed-diagnostic-summary-v3", "fixed-diagnostic-summary-v4"].indexOf(payload.payloadProjection) < 0) return ""
        var versionFour = payload.payloadProjection === "fixed-diagnostic-summary-v4"
        var value = payload.runObservation
        var fields = ["schemaVersion", "transitionKind", "transitionSourceHash", "transitionSequence", "capturedAtUtcMs",
            "observationRevision", "revisionExhausted", "runIdHash", "outcomeCandidate", "modeQueueBegan", "modeQueueCommitted",
            "continuityUnknown", "anomalyMask", "reasonCode", "sourceRetirementCode", "observerClosed", "ownerDestroyed",
            "reviewRequired", "droppedTransitionCount", "snapshotOnly", "historyComplete", "remoteMissionInstanceVerified",
            "missionCompleted", "resultVerified", "executionVerified", "executionPermit", "flightCommandReleased", "permissionsRestored"]
        if (versionFour) fields.push("missionModeExitObservation")
        var unknown = qsTr("运行观察摘要的版本或字段不可识别；保持未知，不推断成功或权限。")
        if (!map(value) || Object.keys(value).length !== fields.length || value.schemaVersion !== (versionFour ? 2 : 1)) return unknown
        for (var f = 0; f < fields.length; f++) if (!Object.prototype.hasOwnProperty.call(value, fields[f])) return unknown
        function unsignedCounter(item, zero) {
            return typeof item === "string" && /^(0|[1-9][0-9]{0,19})$/.test(item)
                && (zero || item !== "0") && (item.length < 20 || item <= "18446744073709551615")
        }
        if (["registered", "changed", "closed", "owner-destroyed", "capture-gap"].indexOf(value.transitionKind) < 0
                || !digest(value.transitionSourceHash) || !unsignedCounter(value.transitionSequence, false)
                || !unsignedCounter(value.observationRevision, true) || !unsignedCounter(value.droppedTransitionCount, true)
                || !integer(value.capturedAtUtcMs, 1, 9007199254740991) || !integer(value.anomalyMask, 0, 255)
                || typeof value.reasonCode !== "string" || typeof value.sourceRetirementCode !== "string"
                || !/^(|[a-z0-9][a-z0-9-]{0,127})$/.test(value.reasonCode)
                || !/^(|[a-z0-9][a-z0-9-]{0,127})$/.test(value.sourceRetirementCode)
                || value.snapshotOnly !== true) return unknown
        var flags = ["revisionExhausted", "modeQueueBegan", "modeQueueCommitted", "continuityUnknown", "observerClosed", "ownerDestroyed", "reviewRequired"]
        for (var b = 0; b < flags.length; b++) if (typeof value[flags[b]] !== "boolean") return unknown
        var denied = ["historyComplete", "remoteMissionInstanceVerified", "missionCompleted", "resultVerified",
            "executionVerified", "executionPermit", "flightCommandReleased", "permissionsRestored"]
        for (var d = 0; d < denied.length; d++) if (value[denied[d]] !== false) return unknown
        if ((value.modeQueueCommitted && !value.modeQueueBegan) || (value.ownerDestroyed && !value.observerClosed)) return unknown
        if (value.transitionKind === "capture-gap") {
            if (versionFour || payload.vehicleId !== 0 || payload.taskIdHash !== "" || payload.startupIdHash !== "" || payload.deploymentIdHash !== ""
                    || !Array.isArray(payload.targetVehicleIds) || payload.targetVehicleIds.length !== 0 || value.runIdHash !== ""
                    || value.observationRevision !== "0" || value.outcomeCandidate !== "not-applicable"
                    || value.continuityUnknown !== true || value.reviewRequired !== true) return unknown
            return qsTr("原生运行观察事件流出现缺口（全局，不归属于单个任务）\n事件序号 %1 · 累计丢弃 %2\n不能据此补造遗漏异常、任务完成或完整历史。")
                .arg(value.transitionSequence).arg(value.droppedTransitionCount)
        }
        if (!integer(payload.vehicleId, 1, 255) || !digest(payload.taskIdHash) || !digest(payload.startupIdHash)
                || !digest(payload.deploymentIdHash) || !digest(value.runIdHash) || value.observationRevision === "0"
                || !Array.isArray(payload.targetVehicleIds) || payload.targetVehicleIds.length < 1 || payload.targetVehicleIds.length > 32
                || payload.targetVehicleIds.indexOf(payload.vehicleId) < 0) return unknown
        var candidates = ["no-terminal-candidate-observed", "terminal-item-observed-not-completion",
            "indefinite-terminal-hold-observed", "landed-after-terminal-observed-not-completion"]
        var candidateIndex = candidates.indexOf(value.outcomeCandidate)
        if (candidateIndex < 0) return unknown
        var exitSummary = versionFour ? modeExitText(value.missionModeExitObservation, value.anomalyMask, value.modeQueueCommitted)
            : qsTr("旧版摘要未采集模式退出的原值与首末收件对；不能补成未发生。")
        if (exitSummary === null) return unknown
        var candidateText = [qsTr("尚无终点候选"), qsTr("曾观察末项，非完成"), qsTr("曾观察无限驻留，非完成"),
            qsTr("曾观察末项后的落地，非完成")][candidateIndex]
        return qsTr("本地Run观察 · 机号 %1 · 转换 %2\n候选：%3\n本地MODE交接开始 / 提交：%4 / %5\n连续性未知 / 需人工复核：%6 / %7\n历史异常位图 %8 · 原因码 %9 · 源退休码 %10\n观察关闭 / 所有者销毁：%11 / %12\n事件序号 %13 · 原生观察修订 %14\n原生事件流累计丢弃 %15（非本任务损失数）\n这是捕获时的本地历史，不证明远端执行实例、正式完成、停飞或当前状态。")
            .arg(payload.vehicleId).arg(value.transitionKind).arg(candidateText)
            .arg(known(value.modeQueueBegan)).arg(known(value.modeQueueCommitted))
            .arg(known(value.continuityUnknown)).arg(known(value.reviewRequired)).arg(value.anomalyMask)
            .arg(value.reasonCode || qsTr("未记录原因码")).arg(value.sourceRetirementCode || qsTr("未观察源退休"))
            .arg(known(value.observerClosed)).arg(known(value.ownerDestroyed))
            .arg(value.transitionSequence).arg(value.observationRevision).arg(value.droppedTransitionCount) + "\n" + exitSummary
    }
    function recordText(entry) {
        var row = entry.row, record = row.record, event = record.event || {}, payload = event.payload || {}
        var runText = runObservationText(payload)
        return qsTr("文件 %1：%2\n原行号 %3 · run %4\n数据库 %5\n文件指纹 %6\n行内容哈希 %7\n事件类别 %8 · 任务哈希 %9\n明确 queued / uncertain：%10 / %11\n异常退出可能丢尾：%12 · run discarded / dropped 计数：%13 / %14\n只读历史摘要，不恢复权限，不从哈希还原原任务或推断成功。")
            .arg(entry.fileIndex).arg(entry.basename).arg(row.rowId).arg(text(record.runId)).arg(entry.databaseEpoch)
            .arg(entry.fileFingerprint).arg(row.contentHash).arg(text(event.type)).arg(text(payload.taskIdHash))
            .arg(known(payload.queued)).arg(known(payload.uncertain)).arg(known(row.uncleanRunMayHaveLostTail))
            .arg(text(row.runDiscardedCount)).arg(text(row.runDroppedCount))
            + (runText ? "\n" + runText : "")
    }
    function bump(which) {
        if (which === "scope") { if (scopeRevision >= 9007199254740991) revisionExhausted = true; else scopeRevision += 1 }
        else { if (selectionRevision >= 9007199254740991) revisionExhausted = true; else selectionRevision += 1 }
    }
    function sourceChanged() {
        var life = lifetime
        if (!life || !life.alive) return
        if (life.epoch >= 9007199254740991) life.exhausted = true; else life.epoch += 1
        ownedQuery = null; cancelledQueryId = ""
        // Preserve copies as visibly old history. Never cancel another owner.
    }
    onArchiveChanged: sourceChanged()
    onOptionsStampChanged: sourceChanged()
    onRunIdChanged: sourceChanged()
    onScopeIndexChanged: bump("scope")
    onSelectedTaskHashChanged: bump("selection")
    onCurrentChanged: adoptResult()
    onActualDirectoryChanged: adoptResult()
    onActualRecordsChanged: adoptResult()
    Component.onDestruction: lifetime.alive = false

    ColumnLayout {
        id: form
        anchors.left: parent.left; anchors.right: parent.right
        spacing: root.pad
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("留存任务诊断目录 → 原记录下钻") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("仅根据原记录明确 taskIdHash 聚合，缺失关联单独计数；不按时间、机号或遥测 run 猜任务。不读取实时任务、不恢复控制权，也不统计成功率、覆盖率或收益。") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("查询范围（改变后须从头读取；改回也不恢复旧续批资格）") }
        Controls.ComboBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; model: [qsTr("已封存库"), qsTr("已封存库 + 停止后的未封存库")]; currentIndex: root.scopeIndex; Accessible.name: qsTr("留存任务目录及下钻范围"); onActivated: root.scopeIndex = currentIndex }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("两种范围均要求录制线程实际退出、captureEnabled=false、查询槽及读取器空闲。包含未封存库时固定库必须存在；读取需创建/释放协作锁，Qt可能回收死进程锁。不会停录、清理旁路文件或自动重试。") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: root.idle ? qsTr("当前可显式读盘；目录与下钻均逐批点击，不会自动扫描全库。") : qsTr("当前不能开始读取：请检查原生来源、实际录制/读取线程、范围或正在进行的调用。取消 accepted 不代表空闲；未连接飞机或未登录不是本地只读诊断的前提。") }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("从头建立任务目录（显式读盘）"); enabled: root.idle; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("tasks"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("继续原任务目录下一批（显式读盘）"); enabled: root.canContinueDirectory; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("continue-tasks"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("仅请求取消本页查询（等待实际结束）"); enabled: root.canCancel; Accessible.name: text; property var press: null; onPressed: press = root.cancelSnapshot(); onCanceled: press = null; onClicked: { var value = press; press = null; root.cancel(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("最近请求：%1 · accepted=%2\n请求ID：%3 · 原因：%4\naccepted仅表示请求接受，不是读取完成或结果有效。").arg(root.text(root.lastRequest.action)).arg(root.known(root.lastRequest.accepted)).arg(root.text(root.lastRequest.requestId)).arg(root.text(root.lastRequest.reason)) }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.statusText() }
        QGCLabel { Layout.fillWidth: true; visible: !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: root.localError }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedDirectory ? qsTr("目录ID：%1\n本页已核实查询ID：%2 · 当前范围：%3\n原清单指纹：%4\n累计已扫描 / 未明确关联任务行：%5 / %6 · 任务哈希数：%7\n已越过文件 / 固定范围文件：%8 / %9\n状态：%10 · 扫描到冻结范围末尾：%11 · 部分结果：%12 · 达到预算：%13\n原生允许继续：%14 · 本页来源/范围仍一致：%15\n时钟异常行 / 留存警告行 / 保留区警告分页：%16 / %17 / %18")
            .arg(root.savedDirectory.dto.directoryId).arg(root.savedDirectory.requestId).arg(root.savedDirectory.dto.scope).arg(root.savedDirectory.dto.inventoryFingerprint)
            .arg(root.savedDirectory.dto.scannedRows).arg(root.savedDirectory.dto.unattributedRows).arg(root.savedDirectory.dto.taskCount)
            .arg(root.savedDirectory.dto.filesCompleted).arg(root.savedDirectory.dto.filesTotal).arg(root.savedDirectory.dto.status)
            .arg(root.known(root.savedDirectory.dto.scanComplete)).arg(root.known(root.savedDirectory.dto.partial)).arg(root.known(root.savedDirectory.dto.budgetReached))
            .arg(root.known(root.savedDirectory.dto.canContinue)).arg(root.known(root.directoryCurrent)).arg(root.savedDirectory.dto.clockDiscontinuityRows)
            .arg(root.savedDirectory.dto.warningRows).arg(root.savedDirectory.dto.retentionWarningPages) : qsTr("尚无本页成功核实的目录。其他入口留下的 historyTasks 不作为本页成功。") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("目录最多256个任务哈希、累计8192行；达到预算或扫描未结束时必须标为部分，不能称全库统计。完成只表示本次冻结清单的保留记录到末尾；并非完整历史、跨库原子快照、任务完成或真实性证明。跨文件/录制保留的重复行不作物理事件去重；警告行可重复引用同一问题run，不是独立丢失事件总数。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedDirectory && !root.taskRows.length; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("已扫描部分中没有明确任务哈希。请看未关联行、部分/预算标志；不表示没有任务或全部历史为空。") }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: root.taskRows.length ? Math.max(root.rowHeight * 4, Math.min(520, ScreenTools.defaultFontPixelHeight * 24)) : 0
            model: root.taskRows; clip: true; spacing: root.pad
            Controls.ScrollBar.vertical: Controls.ScrollBar {}
            delegate: QGCButton {
                width: ListView.view.width; height: Math.max(root.rowHeight, taskLabel.implicitHeight + root.pad * 2); focusPolicy: Qt.StrongFocus
                enabled: root.directoryCurrent && !root.callBusy && !root.historyBusy; text: root.taskText(modelData); Accessible.name: text
                onClicked: root.selectedTaskHash = modelData.taskIdHash
                onActiveFocusChanged: if (activeFocus) ListView.view.positionViewAtIndex(index, ListView.Contain)
                contentItem: QGCLabel { id: taskLabel; text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            }
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("已选任务哈希：%1。选中只改变本页，不读盘；下面按钮才由原生Reader按该哈希筛选原记录，不是对已显示页面做过滤。").arg(root.text(root.selectedTaskHash)) }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取所选任务原记录第一批（显式读盘）"); enabled: root.canRecords; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("task-records"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("继续所选任务原记录下一批（显式读盘）"); enabled: root.canContinueRecords; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("continue-task-records"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedRecords ? qsTr("原记录查询ID：%1\n该批任务哈希：%2\n本批扫描 / 匹配：%3 / %4 · 本次链累计扫描：%5\n状态：%6 · 范围末尾：%7 · 部分：%8 · 达到预算：%9\n仍是本页原任务/目录/范围及原生结果：%10\n仅显示本批最多64条，不累计原始记录。旧批保留只读，不能冒充新选择结果。")
            .arg(root.savedRecords.requestId).arg(root.savedRecords.dto.taskIdHash).arg(root.savedRecords.dto.scannedRows).arg(root.savedRecords.dto.matchedRows)
            .arg(root.savedRecords.dto.cumulativeScannedRows).arg(root.savedRecords.dto.status).arg(root.known(root.savedRecords.dto.scanComplete))
            .arg(root.known(root.savedRecords.dto.partial)).arg(root.known(root.savedRecords.dto.budgetReached)).arg(root.known(root.recordsCurrent)) : qsTr("尚无本页成功核实的任务原记录批次。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedRecords && root.savedRecords.dto.errorCode === "archive-local-tasks-batch-budget-reached"; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("本批整体超出预算，原生没有发布任何新记录；累计数仅保留此前完整接纳的批次，不把有效前缀当作本批结果，也不继续扫描。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedRecords && !root.recordRows.length; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本批没有匹配原记录；可能尚未扫到，也可能保留区已有变化。请阅读原生范围/错误/部分状态，不能据此证明整个任务没有事件。") }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: root.recordRows.length ? Math.max(root.rowHeight * 4, Math.min(520, ScreenTools.defaultFontPixelHeight * 24)) : 0
            model: root.recordRows; clip: true; spacing: root.pad
            Controls.ScrollBar.vertical: Controls.ScrollBar {}
            delegate: FocusScope {
                width: ListView.view.width; height: recordLabel.implicitHeight + root.pad * 2; activeFocusOnTab: true
                Accessible.role: Accessible.StaticText; Accessible.name: recordLabel.text
                onActiveFocusChanged: if (activeFocus) ListView.view.positionViewAtIndex(index, ListView.Contain)
                Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.warningText : qgcPal.text; border.width: parent.activeFocus ? 3 : 1; radius: 3 }
                QGCLabel { id: recordLabel; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad; text: root.recordText(modelData); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            }
        }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.showDetails ? qsTr("收起本页收到的完整只读DTO") : qsTr("展开本页收到的完整只读DTO"); Accessible.name: text; onClicked: root.showDetails = !root.showDetails; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        Loader {
            Layout.fillWidth: true; active: root.showDetails
            Layout.preferredHeight: item ? item.implicitHeight : 0
            sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: qgcPal.text; text: JSON.stringify({directory: root.savedDirectory ? root.savedDirectory.dto : null, records: root.savedRecords ? root.savedRecords.dto : null}, null, 2); Accessible.name: qsTr("本页核实的留存任务目录及原记录DTO，只读，不恢复权限"); background: Rectangle { color: qgcPal.windowShade } } }
        }
    }
}

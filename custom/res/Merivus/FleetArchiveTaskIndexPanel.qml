import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Explicit derived-index IO. No paths, SQL, native cursor or flight actions.
FocusScope {
    id: root
    property var archive: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, draining: false, dirty: false, queued: false, seenJob: ""})
    property bool callBusy: false
    property int scopeIndex: 0
    property bool contextValid: false
    property var ownedQuery: null
    property var saved: null
    property var savedPage: null
    property var savedRecords: null
    property var rebuildConfirmation: null
    property string selectedTaskHash: ""
    property string cancelledQueryId: ""
    property string localMessage: ""
    readonly property var current: archive ? archive.current || {} : ({})
    readonly property var options: archive ? archive.options || {} : ({})
    readonly property var actual: archive ? archive.taskIndex || {} : ({})
    readonly property string runId: typeof current.runId === "string" ? current.runId : ""
    readonly property string scope: scopeIndex === 0 ? "sealed" : scopeIndex === 1 ? "sealed-and-offline" : "invalid"
    readonly property string optionsStamp: stamp(options)
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property bool sourceReady: !!archive && lifetime.alive && !lifetime.exhausted && optionsStamp.length > 0 && scope !== "invalid"
        && typeof archive.index === "function" && typeof archive.request === "function"
        && typeof current.historyRequestPending === "boolean" && typeof current.archivedReaderActive === "boolean"
        && typeof current.archiveMaintenanceActive === "boolean"
        && typeof current.writerActive === "boolean" && typeof current.captureEnabled === "boolean"
    readonly property bool historyBusy: current.historyRequestPending === true || current.archivedReaderActive === true
        || current.archiveMaintenanceActive === true
    readonly property bool idle: sourceReady && !callBusy && !historyBusy
        && current.writerActive === false && current.captureEnabled === false
    readonly property bool savedCurrent: contextValid && saved && sourceCurrent(saved)
        && saved.nativeResult === actual && current.lastHistoryQueryId === saved.requestId
        && current.archivedResultAccepted === true && actual.queryId === saved.requestId
    readonly property bool pageCurrent: savedCurrent && savedPage && sameCheckpoint(savedPage, saved)
    readonly property var tasks: savedPage ? savedPage.dto.page.rows : []
    readonly property var selectedTask: taskFor(selectedTaskHash)
    readonly property bool recordsCurrent: savedCurrent && savedRecords && sameCheckpoint(savedRecords, saved)
        && savedRecords.requestId === saved.requestId && savedRecords.nativeResult === actual
        && savedRecords.dto.records.taskIdHash === selectedTaskHash && !!selectedTask
    readonly property var records: savedRecords ? savedRecords.dto.records.rows : []
    readonly property bool canCancel: sourceReady && !callBusy && historyBusy && ownedQuery && !ownedQuery.adopted
        && sourceCurrent(ownedQuery) && current.historyRequestKind === "task-index"
        && map(current.archivedReader) && current.archivedReader.queryId === ownedQuery.requestId
        && cancelledQueryId !== ownedQuery.requestId
    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: form.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function map(v) { return !!v && typeof v === "object" && !Array.isArray(v) }
    function exact(v, keys) {
        if (!map(v) || Object.keys(v).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(v, keys[i])) return false
        return true
    }
    function integer(v, lo, hi) { return typeof v === "number" && isFinite(v) && Math.floor(v) === v && v >= lo && v <= hi }
    function digest(v) { return typeof v === "string" && /^[0-9a-f]{64}$/.test(v) }
    function uuid(v) { return typeof v === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(v) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(v) }
    function u64(v) { return typeof v === "string" && /^(0|[1-9][0-9]{0,19})$/.test(v) && (v.length < 20 || v <= "18446744073709551615") }
    function counterAtMost(a, b) { return u64(a) && u64(b) && (a.length < b.length || (a.length === b.length && a <= b)) }
    function yes(v) { return v === true ? qsTr("是") : v === false ? qsTr("否") : qsTr("未知") }
    function stamp(v) {
        if (!exact(v, ["schemaVersion", "directoryPath", "policy", "maxSealedSegments", "pageSize", "defaultEnabled", "storesRawPayload"])
                || v.schemaVersion !== 1 || typeof v.directoryPath !== "string" || !v.directoryPath.length || v.directoryPath.length > 4096
                || !map(v.policy) || Object.keys(v.policy).length !== 7 || !integer(v.maxSealedSegments, 1, 128)
                || !integer(v.pageSize, 1, 256) || v.defaultEnabled !== false || v.storesRawPayload !== false) return ""
        try { return JSON.stringify(v) } catch (error) { return "" }
    }
    function copyValue(v) {
        var budget = {nodes: 0, chars: 0}
        function copy(item, depth) {
            if (depth > 16 || ++budget.nodes > 131072) throw new Error("index_value_limit")
            if (item === null || typeof item === "undefined") return null
            if (typeof item === "boolean") return item
            if (typeof item === "number") { if (!isFinite(item) || Math.abs(item) > 9007199254740991) throw new Error("index_number"); return item }
            if (typeof item === "string") { budget.chars += item.length; if (budget.chars > 2097152) throw new Error("index_text"); return item }
            if (!item || typeof item !== "object") throw new Error("index_type")
            if (Array.isArray(item) || typeof item.length === "number") {
                if (!integer(item.length, 0, 256)) throw new Error("index_array")
                var list = []; for (var i = 0; i < item.length; ++i) list.push(copy(item[i], depth + 1)); return list
            }
            var keys = Object.keys(item), result = {}
            if (keys.length > 64) throw new Error("index_map")
            for (var k = 0; k < keys.length; ++k) {
                if (keys[k].length > 128 || ["__proto__", "constructor", "prototype", "toJSON"].indexOf(keys[k]) >= 0) throw new Error("index_key")
                budget.chars += keys[k].length; if (budget.chars > 2097152) throw new Error("index_text")
                result[keys[k]] = copy(item[keys[k]], depth + 1)
            }
            return result
        }
        try { return copy(v, 0) } catch (error) { return null }
    }
    function checkpointKnown(v) {
        return exact(v, ["inventoryFingerprint", "scanComplete", "taskCount", "indexedEventCount", "unassignedEventCount", "sourceFileCount", "windowBatches", "windowScannedRows"])
            && digest(v.inventoryFingerprint) && typeof v.scanComplete === "boolean" && integer(v.taskCount, 0, 32768)
            && u64(v.indexedEventCount) && u64(v.unassignedEventCount) && integer(v.sourceFileCount, 0, 129)
            && counterAtMost(v.unassignedEventCount, v.indexedEventCount)
            && integer(v.windowBatches, 0, 128) && integer(v.windowScannedRows, 0, 8192)
            && v.windowScannedRows <= v.windowBatches * 64
    }
    function pageKnown(v, checkpoint) {
        if (!exact(v, ["afterTask", "nextAfterTask", "hasMore", "totalCount", "rows"])
                || (v.afterTask !== "" && !digest(v.afterTask)) || (v.nextAfterTask !== "" && !digest(v.nextAfterTask))
                || typeof v.hasMore !== "boolean" || v.totalCount !== checkpoint.taskCount
                || !Array.isArray(v.rows) || v.rows.length > 64 || v.rows.length > v.totalCount) return false
        var previous = v.afterTask
        for (var i = 0; i < v.rows.length; ++i) {
            var row = v.rows[i]
            if (!exact(row, ["taskIdHash", "eventCount"]) || !digest(row.taskIdHash)
                    || row.taskIdHash <= previous || !u64(row.eventCount) || row.eventCount === "0") return false
            previous = row.taskIdHash
        }
        return v.hasMore ? v.rows.length === 64 && v.nextAfterTask === previous : v.nextAfterTask === ""
    }
    function recordsKnown(v, query) {
        if (!exact(v, ["format", "schemaVersion", "scope", "vehicleId", "operation", "inventoryFingerprint", "filesTotal", "filesCompleted",
                "scannedRows", "matchedRows", "scanComplete", "nextCursor", "rows", "scannedPages", "ordering", "historicalOnly", "permissionsRestored",
                "authenticated", "signatureVerified", "executable", "completionVerified", "executionPermit", "flightCommandReleased", "restoresAuthorization",
                "historicalCompletenessVerified", "crossSegmentAtomicSnapshot", "wallClockOrderingUsed", "taskIdHash", "resultBytesWithoutThisField"])
                || v.schemaVersion !== 2 || v.format !== "merivus-fleet-event-history-search" || v.scope !== query.intent.scope
                || v.taskIdHash !== query.intent.taskIdHash || v.vehicleId !== 0 || v.operation !== ""
                || !digest(v.inventoryFingerprint) || !integer(v.scannedRows, 0, 256) || !integer(v.matchedRows, 0, 64)
                || !Array.isArray(v.rows) || v.rows.length !== v.matchedRows || !Array.isArray(v.scannedPages) || v.scannedPages.length > 4
                || typeof v.scanComplete !== "boolean" || !map(v.nextCursor) || !integer(v.filesTotal, 0, 129)
                || !integer(v.filesCompleted, 0, v.filesTotal) || v.ordering !== "basename-then-row-not-flight-time"
                || v.historicalOnly !== true || !integer(query.maximumHistoryMiB, 1, 8)
                || !integer(v.resultBytesWithoutThisField, 0, query.maximumHistoryMiB * 1048576)) return false
        var flags = ["permissionsRestored", "authenticated", "signatureVerified", "executable", "completionVerified", "executionPermit",
            "flightCommandReleased", "restoresAuthorization", "historicalCompletenessVerified", "crossSegmentAtomicSnapshot", "wallClockOrderingUsed"]
        for (var f = 0; f < flags.length; ++f) if (v[flags[f]] !== false) return false
        for (var r = 0; r < v.rows.length; ++r) if (!map(v.rows[r])) return false
        return !v.scanComplete || (v.filesCompleted === v.filesTotal && Object.keys(v.nextCursor).length === 0)
    }
    function resultKnown(v, q) {
        if (!exact(v, ["format", "schemaVersion", "action", "scope", "indexId", "checkpointHash", "checkpointCommitted", "cancelObservedAfterCommit",
                "checkpoint", "page", "records", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased", "queryId", "runId"])
                || v.format !== "merivus-fleet-archive-task-index" || v.schemaVersion !== 1 || v.queryId !== q.requestId
                || v.runId !== q.runId || v.action !== q.intent.action || v.scope !== q.intent.scope
                || !uuid(v.indexId) || !digest(v.checkpointHash) || !checkpointKnown(v.checkpoint)
                || typeof v.checkpointCommitted !== "boolean" || typeof v.cancelObservedAfterCommit !== "boolean"
                || v.historicalOnly !== true || v.authenticated !== false || v.permissionsRestored !== false
                || v.executionPermit !== false || v.flightCommandReleased !== false || !map(v.page) || !map(v.records)) return false
        var first = q.intent.action === "create" || q.intent.action === "open" || q.intent.action === "rebuild"
        var writeAction = q.intent.action === "create" || q.intent.action === "resume" || q.intent.action === "rebuild"
        if ((!first && v.indexId !== q.intent.indexId) || (!writeAction && v.checkpointCommitted)
                || (v.cancelObservedAfterCommit && !v.checkpointCommitted)
                || ((q.intent.action === "create" || q.intent.action === "rebuild") && !v.checkpointCommitted)
                || (v.checkpointCommitted ? v.checkpoint.windowBatches < 1 : v.checkpoint.windowBatches !== 0 || v.checkpoint.windowScannedRows !== 0)
                || (q.intent.action === "resume" && !v.checkpointCommitted && !v.checkpoint.scanComplete)
                || (v.scope === "sealed" && v.checkpoint.sourceFileCount > 128)) return false
        var recordAction = q.intent.action === "records" || q.intent.action === "continue-records"
        if (recordAction) return Object.keys(v.page).length === 0 && recordsKnown(v.records, q)
            && v.records.inventoryFingerprint === v.checkpoint.inventoryFingerprint
        return Object.keys(v.records).length === 0 && (q.intent.action === "page"
            ? pageKnown(v.page, v.checkpoint) && v.page.afterTask === q.intent.afterTask : Object.keys(v.page).length === 0)
    }
    function receiptKnown(v, action) {
        return exact(v, ["schemaVersion", "action", "accepted", "requestId", "reason", "requestOnly", "historicalOnly", "permissionsRestored", "flightCommandReleased"])
            && v.schemaVersion === 1 && v.action === action && typeof v.accepted === "boolean" && typeof v.reason === "string"
            && (v.accepted ? uuid(v.requestId) : typeof v.requestId === "string") && v.requestOnly === true
            && v.historicalOnly === true && v.permissionsRestored === false && v.flightCommandReleased === false
    }
    function sourceCurrent(value) {
        return lifetime.alive && !lifetime.exhausted && value && value.owner === archive && value.epoch === lifetime.epoch
            && value.optionsStamp === optionsStamp && value.runId === runId && value.scope === scope
    }
    function sameCheckpoint(a, b) {
        return sourceCurrent(a) && sourceCurrent(b) && a.dto.indexId === b.dto.indexId
            && a.dto.checkpointHash === b.dto.checkpointHash && a.dto.scope === b.dto.scope
            && a.indexSession === b.indexSession
    }
    function taskFor(hash) {
        for (var i = 0; i < tasks.length; ++i) if (tasks[i].taskIdHash === hash) return tasks[i]
        return null
    }
    function retire() {
        var state = lifetime
        if (!state.alive) return
        if (state.epoch >= 9007199254740991) state.exhausted = true
        else ++state.epoch
        var epoch = state.epoch
        contextValid = false
        if (!state.alive || state.epoch !== epoch) return
        rebuildConfirmation = null
        if (!state.alive || state.epoch !== epoch) return
        selectedTaskHash = ""
        if (!state.alive || state.epoch !== epoch) return
        ownedQuery = null
    }
    onArchiveChanged: retire()
    onOptionsStampChanged: retire()
    onRunIdChanged: retire()
    onScopeChanged: retire()
    onActualChanged: scheduleDrain()
    onCurrentChanged: scheduleDrain()
    Component.onDestruction: lifetime.alive = false
    function scheduleDrain() {
        var state = lifetime
        state.dirty = true
        if (!state.alive || callBusy || state.draining || state.queued) return
        state.queued = true
        Qt.callLater(function() {
            if (!state.alive || state !== root.lifetime) return
            state.queued = false
            root.drain()
        })
    }
    function drain() {
        var state = lifetime
        if (!state.alive || callBusy || state.draining) { state.dirty = true; return }
        state.draining = true; state.dirty = false
        try {
            var job = map(current.archivedReader) && typeof current.archivedReader.queryId === "string" ? current.archivedReader.queryId : ""
            if (job && job !== state.seenJob) {
                state.seenJob = job
                if (!ownedQuery || ownedQuery.requestId !== job) retire()
            }
            if (!state.alive) return
            var q = ownedQuery
            if (!q || q.adopted || !sourceCurrent(q) || historyBusy) return
            if (current.lastHistoryQueryId !== q.requestId || current.archivedResultAccepted !== true
                    || actual.queryId !== q.requestId) {
                q.adopted = true; contextValid = false
                if (!state.alive || !sourceCurrent(q) || ownedQuery !== q) return
                localMessage = qsTr("本次索引查询已结束，但未接纳成功结果。请查看原生状态／错误，明确重开索引后再继续。")
                return
            }
            var original = actual, dto = copyValue(original)
            if (!resultKnown(dto, q) || !sourceCurrent(q) || original !== actual || ownedQuery !== q) {
                q.adopted = true; contextValid = false
                if (!state.alive || !sourceCurrent(q) || ownedQuery !== q) return
                localMessage = qsTr("索引结果超过本页显示预算，或格式／来源不匹配；保留旧副本只读，不推断成功。请明确重开。")
                return
            }
            q.adopted = true
            var value = {owner: q.owner, epoch: q.epoch, optionsStamp: q.optionsStamp, runId: q.runId,
                scope: q.scope, requestId: q.requestId, nativeResult: original, dto: dto,
                indexSession: q.indexSession || q.requestId}
            saved = value
            if (!state.alive || !sourceCurrent(q) || ownedQuery !== q || actual !== original) return
            if (Object.keys(dto.page).length) {
                savedPage = value
                if (!state.alive || !sourceCurrent(q) || ownedQuery !== q || actual !== original) return
                if (!taskFor(selectedTaskHash)) selectedTaskHash = ""
                if (!state.alive || !sourceCurrent(q) || ownedQuery !== q || actual !== original) return
            }
            if (Object.keys(dto.records).length) savedRecords = value
            if (!state.alive || !sourceCurrent(q) || ownedQuery !== q || actual !== original) return
            contextValid = cancelledQueryId !== q.requestId && !dto.cancelObservedAfterCommit
            if (!state.alive || !sourceCurrent(q) || ownedQuery !== q || actual !== original) return
            var message = dto.checkpointCommitted ? qsTr("派生索引检查点已提交（不是原始事件、任务状态或权限保存）。")
                : qsTr("本次读取完成；没有据此恢复任何任务或权限。")
            if (!contextValid) message += qsTr(" 已观察到取消；提交事实保留，需明确重开才能继续。")
            localMessage = message
        } finally {
            state.draining = false
            if (state.alive && state.dirty) scheduleDrain()
        }
    }
    function capture(action, afterTask) {
        if (!idle || lifetime.draining || lifetime.exhausted || ["create", "open", "resume", "page", "records", "continue-records", "rebuild"].indexOf(action) < 0) return null
        var initial = action === "create" || action === "open" || action === "rebuild"
        if (!initial && !savedCurrent) return null
        if ((action === "records" || action === "continue-records") && (!pageCurrent || !selectedTask)) return null
        if (action === "continue-records" && (!recordsCurrent || savedRecords.dto.records.scanComplete)) return null
        if (action === "resume" && saved.dto.checkpoint.scanComplete) return null
        var after = afterTask || ""
        if (action === "page" && after !== "" && (!pageCurrent || after !== savedPage.dto.page.nextAfterTask || !savedPage.dto.page.hasMore)) return null
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, runId: runId, scope: scope,
            previous: saved, page: savedPage, selectedTaskHash: selectedTaskHash, indexSession: initial ? "" : saved.indexSession,
            maximumHistoryMiB: options.policy.maximumHistoryMiB,
            intent: {action: action, expectedRunId: runId, scope: scope, previousQueryId: initial ? "" : saved.requestId,
                indexId: initial ? "" : saved.dto.indexId, taskIdHash: action === "records" || action === "continue-records" ? selectedTaskHash : "", afterTask: action === "page" ? after : ""}}
    }
    function captureCurrent(c) {
        return sourceCurrent(c) && idle && (c.intent.action === "create" || c.intent.action === "open"
            || c.intent.action === "rebuild"
            || (c.previous === saved && savedCurrent && c.page === savedPage && c.selectedTaskHash === selectedTaskHash))
    }
    function submit(c) {
        if (lifetime.draining || !captureCurrent(c)) return
        var state = lifetime, owner = c.owner
        callBusy = true
        try {
            if (!state.alive || !sourceCurrent(c)) return
            contextValid = false
            if (!state.alive || !sourceCurrent(c)) return
            rebuildConfirmation = null
            if (!state.alive || !sourceCurrent(c)) return
            var receipt = owner.index(c.intent, true)
            if (!state.alive || !sourceCurrent(c)) return
            if (!receiptKnown(receipt, c.intent.action) || !receipt.accepted) {
                localMessage = qsTr("索引请求未确认：%1。旧结果仅历史；请明确重开，不自动重试。").arg(receipt && receipt.reason ? receipt.reason : qsTr("回执未知"))
                return
            }
            c.requestId = receipt.requestId; c.adopted = false
            ownedQuery = c
            if (!state.alive || !sourceCurrent(c) || ownedQuery !== c) return
            cancelledQueryId = ""
            if (!state.alive || !sourceCurrent(c) || ownedQuery !== c) return
            localMessage = qsTr("请求已接纳，等待原生线程实际结束；不是索引已保存或读取完成。")
        } catch (error) {
            if (state.alive && sourceCurrent(c)) localMessage = qsTr("调用中断，结果未知；不自动重试或声称没有写入。")
        } finally {
            if (state.alive) { callBusy = false; if (state.alive) scheduleDrain() }
        }
    }
    function cancel(c) {
        if (!c || lifetime.draining || !canCancel || c !== ownedQuery) return
        var state = lifetime, owner = c.owner
        callBusy = true
        try {
            if (!state.alive || !sourceCurrent(c) || ownedQuery !== c) return
            contextValid = false
            if (!state.alive || !sourceCurrent(c) || ownedQuery !== c) return
            // The user's cancellation attempt itself retires this page's
            // continuation, including an outcome-unknown/denied cancellation.
            cancelledQueryId = c.requestId
            if (!state.alive || !sourceCurrent(c) || ownedQuery !== c) return
            rebuildConfirmation = null
            if (!state.alive || !sourceCurrent(c) || ownedQuery !== c) return
            var receipt = owner.request({action: "cancel", expectedRunId: c.runId, catalogId: "", basename: "", afterRowId: "0", queryId: c.requestId}, true)
            if (!state.alive || !sourceCurrent(c)) return
            if (receiptKnown(receipt, "cancel") && receipt.accepted) {
                localMessage = qsTr("已请求取消；仍须等待实际线程结束。取消不撤回已经提交的派生检查点。")
            } else localMessage = qsTr("取消未确认；不能据此声称线程已结束或检查点未提交。请查看原生状态。")
        } catch (error) {
            if (state.alive && sourceCurrent(c)) localMessage = qsTr("取消调用中断，结果未知；仍须等待原生线程结束。")
        } finally { if (state.alive) { callBusy = false; if (state.alive) scheduleDrain() } }
    }
    function summary() {
        if (!saved) return qsTr("本页尚无已核实结果。展开、选择范围或任务不会自动读盘。")
        var d = saved.dto, c = d.checkpoint
        return qsTr("%1\n索引 %2\n检查点 %3\n冻结范围扫描到末尾：%4 · 索引任务：%5\n已索引事件行：%6 · 无任务哈希行：%7\n已冻结源文件：%8 · 本窗批次／扫描行：%9／%10\n本次检查点已提交：%11 · 提交后观察取消：%12")
            .arg(savedCurrent ? qsTr("本页当前原生索引来源（仍只是历史）") : qsTr("旧来源副本：仅供阅读，须明确重开"))
            .arg(d.indexId).arg(d.checkpointHash).arg(yes(c.scanComplete)).arg(c.taskCount)
            .arg(c.indexedEventCount).arg(c.unassignedEventCount).arg(c.sourceFileCount).arg(c.windowBatches).arg(c.windowScannedRows)
            .arg(yes(d.checkpointCommitted)).arg(yes(d.cancelObservedAfterCommit))
    }

    ColumnLayout {
        id: form
        width: parent.width; spacing: root.pad
        QGCLabel { Layout.fillWidth: true; font.bold: true; text: qsTr("持久化任务诊断索引"); wrapMode: Text.WordWrap }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("只管理派生索引，不删除原事件。停止录制且原生槽空闲后，明确创建／重开／续扫，再明确读取任务首页；每次仅显示64个任务或64条原记录。扫描完成仅指冻结保留范围，不证明全部历史、真实任务完成或授权恢复。收起不自动取消后台操作。") }
        QGCLabel { Layout.fillWidth: true; text: qsTr("索引范围（改变后需重新明确创建或重开）"); wrapMode: Text.WordWrap }
        QGCComboBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; enabled: !root.callBusy && !root.historyBusy; model: [qsTr("已封存段"), qsTr("已封存段＋停止后的本地库")]; currentIndex: root.scopeIndex; onActivated: root.scopeIndex = index }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.summary() }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; visible: text.length > 0; text: root.localMessage; color: qgcPal.warningText; Accessible.name: text }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("原生读取状态：%1 · %2\n共享读取／维护占用：%3；本页结果不能据其它查询冒领。")
            .arg(root.current.archivedReader && root.current.archivedReader.state || qsTr("无"))
            .arg(root.current.archivedReader && root.current.archivedReader.errorCode || qsTr("无错误记录／未知"))
            .arg(root.yes(root.historyBusy)) }
        Repeater {
            model: [{action: "create", label: qsTr("创建派生索引（写入检查点）")}, {action: "open", label: qsTr("重开已有索引（显式读盘）")}, {action: "resume", label: qsTr("继续下一扫描窗（写入检查点）")}]
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: modelData.label; enabled: root.idle && (modelData.action !== "resume" || root.savedCurrent && !root.saved.dto.checkpoint.scanComplete)
                property var pressedSource: null
                onPressed: pressedSource = root.capture(modelData.action, "")
                onCanceled: pressedSource = null
                onClicked: { var state = root.lifetime, c = pressedSource; pressedSource = null; if (state.alive) root.submit(c) }
            }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("准备重建派生索引…（原事件不删除）"); enabled: root.idle
            property var pressedSource: null
            onPressed: pressedSource = root.capture("rebuild", "")
            onCanceled: pressedSource = null
            onClicked: { var state = root.lifetime, c = pressedSource; pressedSource = null; if (state.alive && !state.draining && root.captureCurrent(c)) root.rebuildConfirmation = c }
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.rebuildConfirmation !== null
            QGCLabel { Layout.fillWidth: true; text: qsTr("确认按当前所选范围重新冻结来源并替换派生索引？旧派生进度将被替换；不会删除原归档，不恢复任何权限。旧索引缺失或损坏时由原生层明确拒绝。"); wrapMode: Text.WordWrap }
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("确认重建这一索引"); enabled: root.captureCurrent(root.rebuildConfirmation)
                property var pressedConfirmation: null
                onPressed: pressedConfirmation = root.rebuildConfirmation
                onCanceled: pressedConfirmation = null
                onClicked: { var state = root.lifetime, c = pressedConfirmation; pressedConfirmation = null; if (state.alive && c && c === root.rebuildConfirmation) root.submit(c) }
            }
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("放弃重建确认")
                property var pressedConfirmation: null
                onPressed: pressedConfirmation = root.rebuildConfirmation
                onCanceled: pressedConfirmation = null
                onClicked: { var state = root.lifetime, c = pressedConfirmation; pressedConfirmation = null; if (state.alive && c && c === root.rebuildConfirmation) root.rebuildConfirmation = null }
            }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("取消本页原查询（等线程结束）"); enabled: root.canCancel
            property var pressedSource: null
            onPressed: pressedSource = root.ownedQuery
            onCanceled: pressedSource = null
            onClicked: { var state = root.lifetime, q = pressedSource; pressedSource = null; if (state.alive) root.cancel(q) }
        }
        QGCLabel { Layout.fillWidth: true; text: qsTr("任务页（只显示当前64条；不是完整任务实例或飞行结果）"); wrapMode: Text.WordWrap }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("任务首页（显式读盘）"); enabled: root.idle && root.savedCurrent; property var pressedSource: null; onPressed: pressedSource = root.capture("page", ""); onCanceled: pressedSource = null; onClicked: { var state = root.lifetime, c = pressedSource; pressedSource = null; if (state.alive) root.submit(c) } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("下一任务页（显式读盘）"); enabled: root.idle && root.pageCurrent && root.savedPage.dto.page.hasMore; property var pressedSource: null; onPressed: pressedSource = root.capture("page", root.savedPage.dto.page.nextAfterTask); onCanceled: pressedSource = null; onClicked: { var state = root.lifetime, c = pressedSource; pressedSource = null; if (state.alive) root.submit(c) } }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: root.tasks.length ? Math.min(420, root.rowHeight * 5) : 0
            model: root.tasks; clip: true; spacing: root.pad; Controls.ScrollBar.vertical: Controls.ScrollBar {}
            delegate: QGCButton {
                id: taskButton
                width: ListView.view.width; implicitHeight: Math.max(root.rowHeight, taskLabel.implicitHeight + root.pad * 2)
                focusPolicy: Qt.StrongFocus; enabled: root.idle && root.pageCurrent
                property var pressedSource: null
                onPressed: pressedSource = {owner: root.archive, saved: root.saved, page: root.savedPage, hash: modelData.taskIdHash, epoch: root.lifetime.epoch}
                onCanceled: pressedSource = null
                onClicked: {
                    var state = root.lifetime, c = pressedSource
                    pressedSource = null
                    if (state.alive && c && c.owner === root.archive && c.saved === root.saved && c.page === root.savedPage && c.epoch === state.epoch && root.pageCurrent) {
                        root.selectedTaskHash = c.hash
                        if (state.alive && c.owner === root.archive && c.saved === root.saved && c.page === root.savedPage && c.epoch === state.epoch) root.rebuildConfirmation = null
                    }
                }
                contentItem: QGCLabel { id: taskLabel; text: (root.selectedTaskHash === modelData.taskIdHash ? qsTr("已选任务哈希：") : qsTr("任务哈希：")) + modelData.taskIdHash + qsTr("\n保留事件行：%1（非真实执行次数）").arg(modelData.eventCount); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: taskButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Accessible.name: taskLabel.text
            }
        }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedPage && root.tasks.length === 0; text: qsTr("本页没有任务条目；不代表未关联行不存在，也不证明历史完整。"); wrapMode: Text.WordWrap }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取所选任务的原保留记录"); enabled: root.idle && root.pageCurrent && !!root.selectedTask; property var pressedSource: null; onPressed: pressedSource = root.capture("records", ""); onCanceled: pressedSource = null; onClicked: { var state = root.lifetime, c = pressedSource; pressedSource = null; if (state.alive) root.submit(c) } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("继续所选任务下一批原记录"); enabled: root.idle && root.recordsCurrent && !root.savedRecords.dto.records.scanComplete; property var pressedSource: null; onPressed: pressedSource = root.capture("continue-records", ""); onCanceled: pressedSource = null; onClicked: { var state = root.lifetime, c = pressedSource; pressedSource = null; if (state.alive) root.submit(c) } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; visible: !!root.savedRecords; text: root.savedRecords ? qsTr("原记录任务哈希 %1\n本批扫描／匹配：%2／%3；到冻结末尾：%4\n%5；按文件名／行号，不是飞行时间；gap／缺失／部分历史不变成完成证明。")
            .arg(root.savedRecords.dto.records.taskIdHash).arg(root.savedRecords.dto.records.scannedRows).arg(root.savedRecords.dto.records.matchedRows).arg(root.yes(root.savedRecords.dto.records.scanComplete))
            .arg(root.recordsCurrent ? qsTr("当前原页绑定的历史") : qsTr("旧副本，仅阅读")) : "" }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: root.records.length ? Math.min(480, root.rowHeight * 6) : 0
            model: root.records; clip: true; spacing: root.pad; Controls.ScrollBar.vertical: Controls.ScrollBar {}
            delegate: FocusScope {
                width: ListView.view.width; height: recordLabel.implicitHeight + root.pad * 2; activeFocusOnTab: true
                Accessible.role: Accessible.StaticText; Accessible.name: recordLabel.text
                Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: parent.activeFocus ? 2 : 1 }
                QGCLabel { id: recordLabel; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad; text: JSON.stringify(modelData, null, 2); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            }
        }
    }
}

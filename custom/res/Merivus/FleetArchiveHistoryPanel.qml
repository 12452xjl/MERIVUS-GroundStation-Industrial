import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Explicit local diagnostics only. No raw Store, filesystem, SQL, aircraft,
// authentication, replay-import or authorization-restoration interface.
FocusScope {
    id: root
    property var archive: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, call: null})
    property bool expanded: false
    property bool callBusy: false
    property bool persistenceConfirmed: false
    property bool sealConfirmed: false
    property bool showDetails: false
    property string localError: ""
    property var lastRequest: ({})
    property var ownedQuery: null
    property string cancelledQueryId: ""
    property var savedCatalog: null
    property var savedPage: null
    property string selectedBasename: ""
    property string vehicleFilter: ""
    property int operationFilterIndex: 0
    property int searchScopeIndex: 0
    property string searchVehicleFilter: ""
    property int searchOperationIndex: 0
    property double searchFilterRevision: 0
    property bool searchFilterExhausted: false
    property var savedSearch: null
    property string consumedSearchId: ""
    property bool searchExpanded: false
    property bool taskHistoryExpanded: false
    property bool taskIndexExpanded: false
    property bool maintenanceExpanded: false

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var currentStatus: archive ? archive.current || {} : ({})
    readonly property var actualCatalog: archive ? archive.historyCatalog || {} : ({})
    readonly property var actualPage: archive ? archive.historyPage || {} : ({})
    readonly property var actualSearch: archive ? archive.historySearch || {} : ({})
    readonly property var options: archive ? archive.options || {} : ({})
    readonly property string runId: typeof currentStatus.runId === "string" ? currentStatus.runId : ""
    readonly property string optionsStamp: optionStamp(options)
    readonly property bool sourceReady: !!archive && lifetime.alive && !lifetime.exhausted
        && optionsStamp.length > 0 && typeof archive.request === "function"
        && typeof currentStatus.state === "string" && typeof currentStatus.captureEnabled === "boolean"
        && typeof currentStatus.historyRequestPending === "boolean" && typeof currentStatus.archivedReaderActive === "boolean"
        && typeof currentStatus.archiveMaintenanceActive === "boolean"
    readonly property bool historyBusy: currentStatus.historyRequestPending === true || currentStatus.archivedReaderActive === true
        || currentStatus.archiveMaintenanceActive === true
    readonly property bool recording: currentStatus.state === "recording" && currentStatus.captureEnabled === true && uuid(runId)
    readonly property bool canStart: sourceReady && !callBusy && !historyBusy && currentStatus.captureEnabled === false
        && currentStatus.sourceBound === true && ["disabled", "stopped", "failed"].indexOf(currentStatus.state) >= 0
    readonly property bool canStop: sourceReady && !callBusy
        && (currentStatus.captureEnabled === true || ["opening", "stopping-for-rollover"].indexOf(currentStatus.state) >= 0)
    readonly property bool canSeal: sourceReady && !callBusy && !historyBusy && recording
    readonly property bool canCatalog: sourceReady && !callBusy && !historyBusy
    readonly property bool canCurrent: sourceReady && !callBusy && !historyBusy && recording && uuid(currentStatus.databaseEpoch)
    readonly property bool canOffline: sourceReady && !callBusy && !historyBusy
        && currentStatus.writerActive === false && currentStatus.captureEnabled === false
    readonly property bool catalogCurrent: !!savedCatalog && savedCatalog.owner === archive && savedCatalog.optionsStamp === optionsStamp
        && savedCatalog.dto.catalogId === actualCatalog.catalogId && catalogKnown(actualCatalog)
    readonly property var catalogRows: savedCatalog && Array.isArray(savedCatalog.dto.entries) ? savedCatalog.dto.entries : []
    readonly property var selectedEntry: findEntry(catalogRows, selectedBasename)
    readonly property bool canSealed: canCatalog && catalogCurrent && !!selectedEntry
    readonly property bool pageLatest: pageIsLatest()
    readonly property bool canNext: !callBusy && nextScopeCurrent()
    readonly property bool canCancel: sourceReady && !callBusy && historyBusy && !!ownedQuery
        && ownedQuery.owner === archive && ownedQuery.optionsStamp === optionsStamp
        && !ownedQuery.adopted && currentStatus.historyRequestKind === ownedQuery.kind
        && (ownedQuery.kind === "current" ? runId === ownedQuery.intent.expectedRunId
            : currentStatus.archivedReader && currentStatus.archivedReader.queryId === ownedQuery.requestId)
        && cancelledQueryId !== ownedQuery.requestId
    readonly property bool filterValid: vehicleFilter === "" || (/^[1-9][0-9]{0,2}$/.test(vehicleFilter) && Number(vehicleFilter) <= 255)
    readonly property string filterOperation: operationFilterIndex >= 0 && operationFilterIndex < 6
        ? ["", "approve", "revoke", "approve-baseline", "revoke-baseline", "approve-performance"][operationFilterIndex] : "invalid"
    readonly property var pageRows: savedPage && Array.isArray(savedPage.dto.rows) ? savedPage.dto.rows : []
    readonly property var filteredRows: filterRows(pageRows)
    readonly property string searchScope: searchScopeIndex === 0 ? "sealed" : searchScopeIndex === 1 ? "sealed-and-offline" : "invalid"
    readonly property string searchOperation: searchOperationIndex >= 0 && searchOperationIndex < 6
        ? ["", "approve", "revoke", "approve-baseline", "revoke-baseline", "approve-performance"][searchOperationIndex] : "invalid"
    readonly property bool searchFiltersValid: !searchFilterExhausted && searchScope !== "invalid" && searchOperation !== "invalid"
        && (searchVehicleFilter === "" || (/^[1-9][0-9]{0,2}$/.test(searchVehicleFilter) && Number(searchVehicleFilter) <= 255))
    readonly property bool canSearch: canOffline && typeof archive.search === "function" && searchFiltersValid
    readonly property bool searchLatest: !!savedSearch && savedSearch.owner === archive && savedSearch.optionsStamp === optionsStamp
        && savedSearch.runId === runId && savedSearch.requestId === currentStatus.lastHistoryQueryId
        && savedSearch.nativeResult === actualSearch
    readonly property bool searchFiltersCurrent: !!savedSearch && savedSearch.filterRevision === searchFilterRevision
        && savedSearch.dto.scope === searchScope && savedSearch.dto.vehicleId === (searchVehicleFilter === "" ? 0 : Number(searchVehicleFilter))
        && savedSearch.dto.operation === searchOperation
    readonly property bool canContinueSearch: canSearch && searchLatest && searchFiltersCurrent && savedSearch.dto.scanComplete === false
        && savedSearch.requestId !== consumedSearchId && currentStatus.archivedResultAccepted === true
    readonly property var searchRows: savedSearch ? savedSearch.dto.rows : []
    readonly property var searchCoverage: savedSearch ? savedSearch.dto.scannedPages : []

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: form.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function map(value) { return !!value && typeof value === "object" && !Array.isArray(value) }
    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function decimal(value) { return typeof value === "string" && /^(0|[1-9][0-9]{0,18})$/.test(value) && (value.length < 19 || value <= "9223372036854775807") }
    function greaterDecimal(first, second) { return decimal(first) && decimal(second) && (first.length > second.length || (first.length === second.length && first > second)) }
    function text(value) { return typeof value === "string" && value.length ? value : qsTr("未知") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function tri(value) { return value === "yes" ? qsTr("是（历史摘要）") : value === "no" ? qsTr("否（历史摘要）") : qsTr("未知 / 未记录") }
    function operationText(value) {
        return value === "approve" ? qsTr("能力采纳") : value === "revoke" ? qsTr("能力撤销")
            : value === "approve-baseline" ? qsTr("基线批准") : value === "revoke-baseline" ? qsTr("基线撤销")
            : value === "approve-performance" ? qsTr("工程性能声明更新") : qsTr("未知操作")
    }
    function optionStamp(value) {
        if (!map(value) || Object.keys(value).length !== 7 || value.schemaVersion !== 1
                || typeof value.directoryPath !== "string" || !value.directoryPath.length
                || value.directoryPath.length > 4096 || !map(value.policy) || Object.keys(value.policy).length !== 7
                || !integer(value.maxSealedSegments, 1, 128) || !integer(value.pageSize, 1, 256)
                || value.defaultEnabled !== false || value.storesRawPayload !== false) return ""
        try { return JSON.stringify([value.directoryPath, value.policy, value.maxSealedSegments, value.pageSize, false, false]) }
        catch (error) { return "" }
    }
    function copyValue(value) {
        var budget = {nodes: 0, chars: 0}
        function copy(item, depth) {
            if (depth > 16 || ++budget.nodes > 65536) throw new Error("archive_value_limit")
            if (item === null || typeof item === "boolean") return item
            if (typeof item === "number") { if (!isFinite(item) || Math.abs(item) > 9007199254740991) throw new Error("archive_number_invalid"); return item }
            if (typeof item === "string") { budget.chars += item.length; if (budget.chars > 2097152) throw new Error("archive_text_limit"); return item }
            if (!item || typeof item !== "object") throw new Error("archive_value_invalid")
            if (Array.isArray(item) || typeof item.length === "number") {
                if (!integer(item.length, 0, 256)) throw new Error("archive_array_limit")
                var rows = []; for (var i = 0; i < item.length; ++i) rows.push(copy(item[i], depth + 1)); return rows
            }
            var keys = Object.keys(item), result = {}
            if (keys.length > 64) throw new Error("archive_map_limit")
            // The native decoder keeps its canonical no-type nulls. Qt can
            // expose only these two validated absence slots as undefined.
            var exitKeys = ["schemaVersion", "observed", "count", "candidate", "first", "last", "requestCausalityVerified", "remoteOutcomeVerified", "automaticActionsIssued"]
            var absentExit = keys.length === exitKeys.length && item.schemaVersion === 1 && item.observed === false
                && item.count === "0" && item.candidate === "not-observed" && item.first == null && item.last == null
                && item.requestCausalityVerified === false && item.remoteOutcomeVerified === false && item.automaticActionsIssued === false
            if (absentExit) for (var e = 0; e < exitKeys.length; ++e) if (!Object.prototype.hasOwnProperty.call(item, exitKeys[e])) absentExit = false
            for (var k = 0; k < keys.length; ++k) {
                var key = keys[k]
                if (key.length > 128 || ["__proto__", "constructor", "prototype", "toJSON"].indexOf(key) >= 0) throw new Error("archive_key_invalid")
                budget.chars += key.length; if (budget.chars > 2097152) throw new Error("archive_text_limit")
                result[key] = copy(absentExit && (key === "first" || key === "last") ? null : item[key], depth + 1)
            }
            return result
        }
        try { return copy(value, 0) } catch (error) { return null }
    }
    function fullJson(value) { try { return JSON.stringify(value, null, 2) } catch (error) { return qsTr("收到的数据无法完整展示；不据此推断成功或权限。") } }
    function inert(value) {
        return map(value) && value.historicalOnly === true && value.permissionsRestored === false
            && value.authenticated === false && value.signatureVerified === false && value.executable === false && value.completionVerified === false
    }
    function entryKnown(value) {
        return map(value) && uuid(value.databaseEpoch) && uuid(value.sealRunId)
            && value.basename === "fleet-events-sealed-" + value.databaseEpoch + "-" + value.sealRunId + ".sqlite"
            && decimal(value.retainedFromRowId) && decimal(value.retainedThroughRowId) && decimal(value.retiredCount) && decimal(value.fileBytes)
    }
    function catalogKnown(value) {
        if (!inert(value) || value.format !== "merivus-fleet-event-archive-catalog" || value.schemaVersion !== 1
                || !uuid(value.catalogId) || value.directoryInventoryEnumerationCompleted !== true
                || !Array.isArray(value.entries) || value.entries.length > 128) return false
        var seen = {}
        for (var i = 0; i < value.entries.length; ++i) { var row = value.entries[i]; if (!entryKnown(row) || seen[row.basename]) return false; seen[row.basename] = true }
        return true
    }
    function findEntry(rows, basename) { for (var i = 0; i < rows.length; ++i) if (rows[i].basename === basename) return rows[i]; return null }
    function pageKnown(value, query) {
        if (!inert(value) || value.format !== "merivus-fleet-event-history-page" || value.schemaVersion !== 1
                || !uuid(value.databaseEpoch) || (query.kind !== "offline" && value.databaseEpoch !== query.databaseEpoch)
                || value.afterRowId !== query.intent.afterRowId || !decimal(value.afterRowId) || !decimal(value.nextRowId)
                || typeof value.hasMore !== "boolean" || !Array.isArray(value.rows) || value.rows.length > 256
                || typeof value.runFilter !== "string") return false
        if (query.kind === "current") {
            if (value.runFilter !== query.intent.expectedRunId || value.sealedSegmentOnly === true || value.offlineCurrentOnly === true
                    || (typeof value.catalogId === "string" && value.catalogId.length)) return false
        } else if (query.kind === "offline") {
            // First read learns the actual stopped database identity; an empty
            // expected epoch is not an instruction to accept an empty epoch.
            if (value.basename !== "fleet-events-v1.sqlite" || value.offlineCurrentOnly !== true
                    || value.sealedSegmentOnly !== false || value.cooperativeOwnerLockHeldDuringRead !== true
                    || value.executionPermit !== false || value.flightCommandReleased !== false || value.restoresAuthorization !== false
                    || !digest(value.fileFingerprint) || value.runFilter !== "" || value.rows.length > 64
                    || Object.prototype.hasOwnProperty.call(value, "catalogId")
                    || Object.prototype.hasOwnProperty.call(value, "sealRunId")) return false
            if (query.intent.afterRowId !== "0" && (value.databaseEpoch !== query.databaseEpoch
                    || value.fileFingerprint !== query.fileFingerprint)) return false
        } else if (query.kind !== "page" || value.catalogId !== query.intent.catalogId || value.basename !== query.intent.basename
                || value.sealRunId !== query.sealRunId || value.sealedSegmentOnly !== true
                || value.offlineCurrentOnly === true || value.runFilter !== "") return false
        var previous = value.afterRowId
        for (var i = 0; i < value.rows.length; ++i) {
            var row = value.rows[i]
            if (!map(row) || !greaterDecimal(row.rowId, previous) || !digest(row.contentHash) || !map(row.record)
                    || row.record.historicalOnly !== true || row.record.authenticated !== false
                    || row.record.restoresAuthorization !== false || row.record.flightCommandReleased !== false
                    || (query.kind === "current" && row.record.runId !== query.intent.expectedRunId)) return false
            previous = row.rowId
        }
        return value.nextRowId === previous && (!value.hasMore || (value.rows.length > 0 && greaterDecimal(value.nextRowId, value.afterRowId)))
    }
    function engineering(row) {
        var payload = row && row.record && row.record.event ? row.record.event.payload : null
        if (!map(payload) || payload.payloadProjection !== "fixed-diagnostic-summary-v2") return null
        var value = payload.engineeringReview
        if (!map(value) || Object.keys(value).length !== 21 || value.schemaVersion !== 1
                || value.source !== "local-engineering-review-summary" || !integer(value.vehicleId, 1, 255)
                || ["approve", "revoke", "approve-baseline", "revoke-baseline", "approve-performance"].indexOf(value.operation) < 0
                || value.historicalOnly !== true || value.restoresAuthorization !== false || value.hardwareIdentityAuthenticated !== false
                || value.performanceCertified !== false || value.flightCommandReleased !== false) return null
        var flags = ["mutationCommitted", "nativeOk", "stillCurrent", "comparisonKnown", "comparisonTruncated"]
        for (var i = 0; i < flags.length; ++i) if (["yes", "no", "unknown"].indexOf(value[flags[i]]) < 0) return null
        var hashes = ["beforeTypedFingerprint", "afterTypedFingerprint", "baselineTypedFingerprint", "reviewIdHash"]
        for (var h = 0; h < hashes.length; ++h) if (typeof value[hashes[h]] !== "string" || (value[hashes[h]] !== "" && !digest(value[hashes[h]]))) return null
        return integer(value.reportedChangeCount, -1, 4096) && integer(value.displayedChangeCount, -1, 128)
            && integer(value.capabilityCount, -1, 128) ? value : null
    }
    function rowVehicleId(row) {
        var summary = engineering(row), payload = row && row.record && row.record.event ? row.record.event.payload : null
        return summary ? summary.vehicleId : map(payload) && integer(payload.vehicleId, 1, 255) ? payload.vehicleId : 0
    }
    function filterRows(rows) {
        if (!filterValid || filterOperation === "invalid") return []
        var result = []
        for (var i = 0; i < rows.length; ++i) {
            var value = engineering(rows[i])
            if ((vehicleFilter === "" || rowVehicleId(rows[i]) === Number(vehicleFilter))
                    && (filterOperation === "" || (value && value.operation === filterOperation))) result.push(rows[i])
        }
        return result
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
    function rowText(row) {
        var value = engineering(row), event = row && row.record ? row.record.event || {} : {}
        var heading = qsTr("行 %1 · run %2\n事件分类 %3 · 内容哈希 %4").arg(text(row.rowId)).arg(text((row.record || {}).runId)).arg(text(event.type)).arg(text(row.contentHash))
        var runText = runObservationText(event.payload)
        if (runText) return heading + "\n" + runText
        if (!value) return heading + "\n" + qsTr("旧v1 / 未记录有效工程摘要：操作、提交、结果和批准详情未知；不补成否，不从哈希还原账号或理由。")
        return heading + "\n" + qsTr("%1 · 本地机号 %2\n曾提交：%3 · 原生ok：%4 · 当次仍为当前：%5\n比较已知：%6 · 比较截断：%7\n检测变更数：%8 · 当次差异展示行：%9 · 能力数量：%10\n前有类型指纹：%11\n后有类型指纹：%12\n基线有类型指纹：%13\n审阅ID哈希：%14\n全部均为本地历史摘要，不证明现在有效、物理身份、性能或飞行许可。")
            .arg(operationText(value.operation)).arg(value.vehicleId).arg(tri(value.mutationCommitted)).arg(tri(value.nativeOk)).arg(tri(value.stillCurrent))
            .arg(tri(value.comparisonKnown)).arg(tri(value.comparisonTruncated))
            .arg(value.reportedChangeCount < 0 ? qsTr("未知") : String(value.reportedChangeCount))
            .arg(value.displayedChangeCount < 0 ? qsTr("未知") : String(value.displayedChangeCount))
            .arg(value.capabilityCount < 0 ? qsTr("未知") : String(value.capabilityCount))
            .arg(text(value.beforeTypedFingerprint)).arg(text(value.afterTypedFingerprint)).arg(text(value.baselineTypedFingerprint)).arg(text(value.reviewIdHash))
    }
    function pageIsLatest() {
        if (!savedPage || savedPage.owner !== archive || savedPage.optionsStamp !== optionsStamp
                || savedPage.requestId !== currentStatus.lastHistoryQueryId || actualPage.databaseEpoch !== savedPage.dto.databaseEpoch
                || actualPage.afterRowId !== savedPage.dto.afterRowId || actualPage.nextRowId !== savedPage.dto.nextRowId) return false
        if (savedPage.kind === "offline") return runId === savedPage.runId
            && currentStatus.writerActive === false && currentStatus.captureEnabled === false
            && actualPage.basename === "fleet-events-v1.sqlite" && actualPage.offlineCurrentOnly === true
            && actualPage.sealedSegmentOnly === false && actualPage.cooperativeOwnerLockHeldDuringRead === true
            && actualPage.executionPermit === false && actualPage.flightCommandReleased === false && actualPage.restoresAuthorization === false
            && actualPage.runFilter === "" && digest(actualPage.fileFingerprint)
            && actualPage.fileFingerprint === savedPage.dto.fileFingerprint
            && !Object.prototype.hasOwnProperty.call(actualPage, "catalogId")
            && !Object.prototype.hasOwnProperty.call(actualPage, "sealRunId")
        return savedPage.kind === "current" ? runId === savedPage.runId && currentStatus.databaseEpoch === savedPage.dto.databaseEpoch
                && actualPage.runFilter === savedPage.runId && actualPage.sealedSegmentOnly !== true && actualPage.offlineCurrentOnly !== true
            : savedPage.kind === "page" && savedPage.dto.catalogId === actualPage.catalogId && savedPage.dto.basename === actualPage.basename
                && savedPage.dto.sealRunId === actualPage.sealRunId && actualPage.sealedSegmentOnly === true
                && actualPage.offlineCurrentOnly !== true && actualPage.runFilter === ""
    }
    function searchFileKnown(value, total, scope) {
        if (!map(value) || !integer(value.fileIndex, 0, total - 1) || !uuid(value.databaseEpoch)
                || !digest(value.fileFingerprint) || typeof value.offlineCurrentOnly !== "boolean") return false
        return value.offlineCurrentOnly ? scope === "sealed-and-offline" && value.basename === "fleet-events-v1.sqlite" && value.sealRunId === ""
            : uuid(value.sealRunId) && value.basename === "fleet-events-sealed-" + value.databaseEpoch + "-" + value.sealRunId + ".sqlite"
    }
    function sameSearchFile(first, second) {
        return first.fileIndex === second.fileIndex && first.basename === second.basename && first.databaseEpoch === second.databaseEpoch
            && first.sealRunId === second.sealRunId && first.fileFingerprint === second.fileFingerprint && first.offlineCurrentOnly === second.offlineCurrentOnly
    }
    function searchKnown(value, query) {
        if (!inert(value) || Object.keys(value).length !== 28 || value.format !== "merivus-fleet-event-history-search" || value.schemaVersion !== 1
                || value.scope !== query.intent.scope || !integer(value.vehicleId, 0, 255)
                || value.vehicleId !== (query.intent.vehicleId === "" ? 0 : Number(query.intent.vehicleId)) || value.operation !== query.intent.operation
                || !digest(value.inventoryFingerprint) || value.ordering !== "basename-then-row-not-flight-time"
                || !integer(value.filesTotal, value.scope === "sealed" ? 0 : 1, value.scope === "sealed" ? 128 : 129)
                || !integer(value.filesCompleted, 0, value.filesTotal)
                || !integer(value.scannedRows, 0, 256) || !integer(value.matchedRows, 0, 64) || value.matchedRows > value.scannedRows
                || typeof value.scanComplete !== "boolean" || !map(value.nextCursor)
                || !Array.isArray(value.rows) || value.rows.length !== value.matchedRows
                || !Array.isArray(value.scannedPages) || value.scannedPages.length > 4
                || !integer(value.resultBytesWithoutThisField, 0, 2097152)) return false
        var flags = ["executionPermit", "flightCommandReleased", "restoresAuthorization", "historicalCompletenessVerified", "crossSegmentAtomicSnapshot", "wallClockOrderingUsed"]
        for (var f = 0; f < flags.length; ++f) if (value[flags[f]] !== false) return false
        if (query.previous && (value.inventoryFingerprint !== query.previous.dto.inventoryFingerprint
                || value.filesTotal !== query.previous.dto.filesTotal || value.filesCompleted < query.previous.dto.filesCompleted)) return false
        if (value.scanComplete) {
            if (Object.keys(value.nextCursor).length !== 0 || value.filesCompleted !== value.filesTotal) return false
        } else {
            var cursor = value.nextCursor
            if (Object.keys(cursor).length !== 5 || cursor.inventoryFingerprint !== value.inventoryFingerprint
                    || !integer(cursor.fileIndex, 0, value.filesTotal - 1) || cursor.fileIndex !== value.filesCompleted
                    || !decimal(cursor.afterRowId)
                    || (cursor.afterRowId === "0" ? cursor.fileFingerprint !== "" || cursor.databaseEpoch !== ""
                        : !digest(cursor.fileFingerprint) || !uuid(cursor.databaseEpoch))) return false
        }
        var scanned = 0, matched = 0
        for (var p = 0; p < value.scannedPages.length; ++p) {
            var coverage = value.scannedPages[p]
            if (!searchFileKnown(coverage, value.filesTotal, value.scope) || Object.keys(coverage).length !== 18
                    || !decimal(coverage.afterRowId) || !decimal(coverage.nextRowId) || typeof coverage.hasMore !== "boolean"
                    || (coverage.nextRowId !== coverage.afterRowId && !greaterDecimal(coverage.nextRowId, coverage.afterRowId))
                    || !decimal(coverage.retainedFromRowId) || !decimal(coverage.retainedThroughRowId) || !decimal(coverage.retiredCount)
                    || typeof coverage.cursorPrecedesRetainedHistory !== "boolean" || !integer(coverage.scannedRows, 0, 64)
                    || !integer(coverage.matchedRows, 0, coverage.scannedRows) || !integer(coverage.uncleanRows, 0, coverage.scannedRows)
                    || !integer(coverage.rowsWithDroppedEvents, 0, coverage.scannedRows) || !integer(coverage.rowsWithDiscardedEvents, 0, coverage.scannedRows)) return false
            scanned += coverage.scannedRows; matched += coverage.matchedRows
        }
        if (scanned !== value.scannedRows || matched !== value.matchedRows) return false
        var previousFile = -1, previousRow = "0"
        for (var r = 0; r < value.rows.length; ++r) {
            var entry = value.rows[r], row = map(entry) ? entry.row : null
            if (!searchFileKnown(entry, value.filesTotal, value.scope) || Object.keys(entry).length !== 7
                    || !map(row) || Object.keys(row).length !== 8 || !greaterDecimal(row.rowId, "0") || !digest(row.contentHash)
                    || !map(row.record) || row.record.historicalOnly !== true || row.record.authenticated !== false
                    || row.record.restoresAuthorization !== false || row.record.flightCommandReleased !== false
                    || entry.fileIndex < previousFile || (entry.fileIndex === previousFile && !greaterDecimal(row.rowId, previousRow))) return false
            var contained = false
            for (var c = 0; c < value.scannedPages.length; ++c) {
                var range = value.scannedPages[c]
                if (sameSearchFile(entry, range) && greaterDecimal(row.rowId, range.afterRowId)
                        && (row.rowId === range.nextRowId || greaterDecimal(range.nextRowId, row.rowId))) contained = true
            }
            if (!contained) return false
            var summary = engineering(row)
            if ((value.vehicleId !== 0 && rowVehicleId(row) !== value.vehicleId)
                    || (value.operation !== "" && (!summary || summary.operation !== value.operation))) return false
            previousFile = entry.fileIndex; previousRow = row.rowId
        }
        return true
    }
    function adoptSearchResult(query) {
        var life = lifetime, owner = archive, original = actualSearch, stamp = optionsStamp
        var value = copyValue(original)
        if (!life.alive || query !== ownedQuery || query.owner !== owner || query.optionsStamp !== stamp
                || query.requestId === cancelledQueryId || query.adopted || currentStatus.lastHistoryQueryId !== query.requestId
                || currentStatus.archivedResultAccepted !== true || original !== actualSearch || !searchKnown(value, query)) return
        query.adopted = true
        savedSearch = {owner: owner, optionsStamp: stamp, requestId: query.requestId, runId: query.intent.expectedRunId,
            filterRevision: query.filterRevision, nativeResult: original, dto: value}
    }
    function invalidateSearchFilters() {
        if (searchFilterRevision >= 9007199254740991) searchFilterExhausted = true
        else searchFilterRevision += 1
        // Keep the old result visibly historical; changing back does not revive
        // the continuation of a query issued with an earlier filter revision.
    }
    function searchPressSnapshot(continuing) {
        if (!canSearch || (continuing && !canContinueSearch)) return null
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, filterRevision: searchFilterRevision,
            previous: continuing ? savedSearch : null, nativeResult: continuing ? actualSearch : null,
            intent: {action: continuing ? "continue-search" : "search", expectedRunId: runId, scope: searchScope,
                vehicleId: searchVehicleFilter, operation: searchOperation, previousQueryId: continuing ? savedSearch.requestId : ""}}
    }
    function searchPressCurrent(pressed, consumedByThisCall) {
        if (!pressed || pressed.owner !== archive || pressed.epoch !== lifetime.epoch || pressed.optionsStamp !== optionsStamp
                || pressed.filterRevision !== searchFilterRevision || !sourceReady || !searchFiltersValid || typeof archive.search !== "function"
                || historyBusy || currentStatus.writerActive !== false || currentStatus.captureEnabled !== false
                || pressed.intent.expectedRunId !== runId || pressed.intent.scope !== searchScope
                || pressed.intent.vehicleId !== searchVehicleFilter || pressed.intent.operation !== searchOperation) return false
        if (pressed.intent.action === "search") return pressed.intent.previousQueryId === "" && !pressed.previous
        return pressed.intent.action === "continue-search" && searchLatest && searchFiltersCurrent
            && currentStatus.archivedResultAccepted === true && savedSearch.dto.scanComplete === false
            && (consumedByThisCall === true || savedSearch.requestId !== consumedSearchId) && pressed.previous === savedSearch
            && pressed.nativeResult === actualSearch && pressed.intent.previousQueryId === savedSearch.requestId
    }
    function submitSearch(pressed) {
        var life = lifetime
        if (!life.alive || life.exhausted || life.call || callBusy || !searchPressCurrent(pressed)) {
            if (life.alive) localError = qsTr("搜索来源、筛选条件、录制状态或原结果已变化；未发送，请重新核对并从头搜索。")
            return
        }
        var owner = pressed.owner, epoch = life.epoch, ticket = {}, query = null
        life.call = ticket
        try {
            callBusy = true
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || owner !== archive) return
            localError = ""
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !searchPressCurrent(pressed)) return
            // Freeze the actual attempted query before any native notification.
            // A synchronous completion is adopted after its receipt supplies ID.
            query = {owner: owner, optionsStamp: pressed.optionsStamp, intent: pressed.intent, kind: "search", requestId: "",
                filterRevision: pressed.filterRevision, previous: pressed.previous, adopted: false}
            consumedSearchId = savedSearch ? savedSearch.requestId : ""
            if (!life.alive || life.call !== ticket || life.epoch !== epoch) return
            ownedQuery = query
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || owner !== archive
                    || ownedQuery !== query || !searchPressCurrent(pressed, true)) return
            var receipt = null
            try { receipt = owner.search(pressed.intent, true) } catch (error) { receipt = null }
            if (!life.alive) return
            var value = copyValue(receipt)
            if (!receiptKnown(value, pressed.intent)) { localError = qsTr("搜索请求效果未知；不自动重试或沿用旧游标，请核对原生状态后从头搜索。"); return }
            lastRequest = value
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || owner !== archive || ownedQuery !== query || optionsStamp !== pressed.optionsStamp) return
            if (!value.accepted) { localError = qsTr("原生未接受搜索：%1。保留旧结果仅供阅读；请核对原因后从头搜索。").arg(text(value.reason)); return }
            // New identity notifies bindings; nested mutation of a JS map alone
            // must not be relied upon to enable this query's cancel action.
            query = {owner: owner, optionsStamp: pressed.optionsStamp, intent: pressed.intent, kind: "search", requestId: value.requestId,
                filterRevision: pressed.filterRevision, previous: pressed.previous, adopted: false}
            ownedQuery = query
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || ownedQuery !== query) return
            cancelledQueryId = ""
            if (life.alive && life.call === ticket && life.epoch === epoch && ownedQuery === query) adoptResult()
        } finally { if (life.alive && life.call === ticket) { life.call = null; callBusy = false } }
    }
    function searchCoverageText(value) {
        return qsTr("文件 %1：%2\n数据库：%3 · 指纹：%4\n本批区间 (%5, %6] · 后面仍有行：%7\n本段保留区间：%8–%9 · 已清理计数：%10 · 游标早于保留区：%11\n本页扫描/匹配行：%12 / %13\n引用异常退出/丢弃/拒收 run 的行数：%14 / %15 / %16（不是丢失事件总数）")
            .arg(value.fileIndex).arg(value.basename).arg(value.databaseEpoch).arg(value.fileFingerprint)
            .arg(value.afterRowId).arg(value.nextRowId).arg(known(value.hasMore)).arg(value.retainedFromRowId).arg(value.retainedThroughRowId)
            .arg(value.retiredCount).arg(known(value.cursorPrecedesRetainedHistory)).arg(value.scannedRows).arg(value.matchedRows)
            .arg(value.uncleanRows).arg(value.rowsWithDiscardedEvents).arg(value.rowsWithDroppedEvents)
    }
    function adoptResult() {
        var life = lifetime, query = ownedQuery, owner = archive, stamp = optionsStamp
        if (!life || !life.alive || !query || query.owner !== owner || query.optionsStamp !== stamp
                || query.requestId === cancelledQueryId || query.adopted || currentStatus.lastHistoryQueryId !== query.requestId) return
        if (query.kind === "search") { adoptSearchResult(query); return }
        var value = copyValue(query.kind === "catalog" ? actualCatalog : actualPage)
        if (!life.alive || ownedQuery !== query || archive !== owner || optionsStamp !== stamp
                || currentStatus.lastHistoryQueryId !== query.requestId) return
        if ((query.kind !== "current" && currentStatus.archivedResultAccepted !== true)
                || (query.kind === "catalog" ? !catalogKnown(value) : !pageKnown(value, query))) return
        // Only this original request may publish. No source getter follows a
        // false/unknown result to make it look like a successful current read.
        query.adopted = true
        if (query.kind === "catalog") {
            savedCatalog = {owner: owner, optionsStamp: stamp, requestId: query.requestId, dto: value}
            if (!life.alive || ownedQuery !== query) return
            selectedBasename = ""
        } else savedPage = {owner: owner, optionsStamp: stamp, requestId: query.requestId, kind: query.kind, runId: query.intent.expectedRunId, dto: value}
    }
    function pressSnapshot(action, after) {
        var intent = {action: action, expectedRunId: runId, catalogId: "", basename: "", afterRowId: "0", queryId: ""}
        var databaseEpoch = typeof currentStatus.databaseEpoch === "string" ? currentStatus.databaseEpoch : "", sealRunId = "", fileFingerprint = ""
        if (action === "current") intent.afterRowId = after || "0"
        if (action === "offline") {
            intent.afterRowId = after || "0"
            databaseEpoch = ""
            if (intent.afterRowId !== "0") {
                if (!savedPage || savedPage.kind !== "offline") return null
                databaseEpoch = savedPage.dto.databaseEpoch; fileFingerprint = savedPage.dto.fileFingerprint
            }
        }
        if (action === "page") {
            if (!catalogCurrent || !selectedEntry) return null
            intent.catalogId = savedCatalog.dto.catalogId; intent.basename = selectedEntry.basename; intent.afterRowId = after || "0"
            databaseEpoch = selectedEntry.databaseEpoch; sealRunId = selectedEntry.sealRunId
        }
        if (action === "cancel") { if (!ownedQuery || ownedQuery.owner !== archive) return null; intent.queryId = ownedQuery.requestId }
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, intent: intent,
            databaseEpoch: databaseEpoch, sealRunId: sealRunId, fileFingerprint: fileFingerprint,
            persistenceConfirmed: persistenceConfirmed, sealConfirmed: sealConfirmed,
            cursorPage: intent.afterRowId !== "0" ? savedPage : null, cancelledQuery: action === "cancel" ? ownedQuery : null}
    }
    function nextSnapshot() {
        if (!canNext) return null
        if (savedPage.kind === "page" && savedPage.dto.basename !== selectedBasename) return null
        return pressSnapshot(savedPage.kind, savedPage.dto.nextRowId)
    }
    function actionAllowed(action) {
        return action === "start" ? canStart && persistenceConfirmed : action === "stop" ? canStop
            : action === "seal" ? canSeal && sealConfirmed : action === "catalog" ? canCatalog
            : action === "current" ? canCurrent : action === "offline" ? canOffline : action === "page" ? canSealed : action === "cancel" && canCancel
    }
    function nextScopeCurrent() {
        return sourceReady && !historyBusy && pageLatest && savedPage.dto.hasMore === true
            && decimal(savedPage.dto.nextRowId) && greaterDecimal(savedPage.dto.nextRowId, savedPage.dto.afterRowId)
            && (savedPage.kind === "current" ? recording && uuid(currentStatus.databaseEpoch)
                : savedPage.kind === "offline" ? currentStatus.writerActive === false && currentStatus.captureEnabled === false : catalogCurrent)
    }
    function actionStateCurrent(action) {
        // Tail check while our callBusy latch is held. No toggling that latch
        // merely to ask whether the native state still permits this action.
        if (!sourceReady) return false
        if (action === "start") return !historyBusy && currentStatus.captureEnabled === false
            && currentStatus.sourceBound === true && ["disabled", "stopped", "failed"].indexOf(currentStatus.state) >= 0
        if (action === "stop") return currentStatus.captureEnabled === true || ["opening", "stopping-for-rollover"].indexOf(currentStatus.state) >= 0
        if (action === "seal") return !historyBusy && recording
        if (action === "catalog") return !historyBusy
        if (action === "current") return !historyBusy && recording && uuid(currentStatus.databaseEpoch)
        if (action === "offline") return !historyBusy && currentStatus.writerActive === false && currentStatus.captureEnabled === false
        if (action === "page") return !historyBusy && catalogCurrent && !!selectedEntry
        return action === "cancel" && historyBusy && !!ownedQuery && !ownedQuery.adopted
            && currentStatus.historyRequestKind === ownedQuery.kind && cancelledQueryId !== ownedQuery.requestId
            && (ownedQuery.kind === "current" ? runId === ownedQuery.intent.expectedRunId
                : currentStatus.archivedReader && currentStatus.archivedReader.queryId === ownedQuery.requestId)
    }
    function pressScopeCurrent(pressed) {
        if (!pressed || pressed.owner !== archive || pressed.epoch !== lifetime.epoch || pressed.optionsStamp !== optionsStamp
                || pressed.intent.expectedRunId !== runId) return false
        var action = pressed.intent.action
        if (action === "current" && pressed.databaseEpoch !== currentStatus.databaseEpoch) return false
        if (action === "page" && (!catalogCurrent || !selectedEntry || pressed.intent.catalogId !== savedCatalog.dto.catalogId
                || pressed.intent.basename !== selectedBasename || pressed.intent.basename !== selectedEntry.basename
                || pressed.databaseEpoch !== selectedEntry.databaseEpoch || pressed.sealRunId !== selectedEntry.sealRunId)) return false
        if (pressed.intent.afterRowId !== "0" && (!nextScopeCurrent() || pressed.cursorPage !== savedPage
                || savedPage.kind !== action || pressed.intent.afterRowId !== savedPage.dto.nextRowId)) return false
        if (action === "offline" && pressed.intent.afterRowId !== "0"
                && (pressed.databaseEpoch !== savedPage.dto.databaseEpoch || pressed.fileFingerprint !== savedPage.dto.fileFingerprint)) return false
        return action !== "cancel" || (pressed.cancelledQuery === ownedQuery && !!ownedQuery
            && pressed.intent.queryId === ownedQuery.requestId && cancelledQueryId !== ownedQuery.requestId)
    }
    function receiptKnown(value, intent) {
        if (!map(value) || Object.keys(value).length !== 9 || value.schemaVersion !== 1 || value.action !== intent.action
                || typeof value.accepted !== "boolean" || typeof value.reason !== "string" || typeof value.requestId !== "string"
                || value.requestOnly !== true || value.historicalOnly !== true || value.permissionsRestored !== false
                || value.flightCommandReleased !== false) return false
        // Native control actions have no query ID, including on acceptance.
        // A rejected query can fail before allocation (empty) or after it (UUID).
        if (["start", "stop", "seal"].indexOf(intent.action) >= 0) return value.requestId === ""
        if (intent.action === "cancel") return value.requestId === intent.queryId || (!value.accepted && value.requestId === "")
        return uuid(value.requestId) || (!value.accepted && value.requestId === "")
    }
    function queryStatusText() {
        if (!ownedQuery) return qsTr("尚无本页接受的查询。")
        var heading = qsTr("本页查询ID：%1\n原生最近完成ID：%2").arg(ownedQuery.requestId).arg(text(currentStatus.lastHistoryQueryId))
        if (ownedQuery.kind !== "current") {
            var reader = currentStatus.archivedReader
            if (map(reader) && reader.queryId === ownedQuery.requestId) {
                heading += qsTr("\n本页读取器状态：%1\n本页读取器错误：%2\n本页结果接纳错误：%3")
                    .arg(text(reader.state)).arg(reader.errorCode === "" ? qsTr("未报告") : text(reader.errorCode))
                    .arg(currentStatus.archivedResultErrorCode === "" ? qsTr("未报告") : text(currentStatus.archivedResultErrorCode))
            } else heading += "\n" + qsTr("原生读取器快照不属于本页ID；其状态/错误不归于本次查询。")
        } else heading += "\n" + qsTr("当前库查询不使用封存读取器错误；其完成以本页ID结果为准，录制器全局状态见上方。")
        if (cancelledQueryId === ownedQuery.requestId) heading += "\n" + qsTr("已请求取消本页查询；不据此推断原生槽已释放。")
        if (ownedQuery.kind === "offline") heading += "\n" + qsTr("停止录制后的未封存库：数据库只读，但需创建/释放协作锁，Qt 可能回收死进程锁；不会初始化、恢复或删除数据库。若文件变更，请重新读取第一页；若有 journal/wal/shm 或锁占用，保留数据库及旁路文件交工程师处理，不自动清理或重试。")
        if (ownedQuery.kind === "search") heading += "\n" + qsTr("跨库搜索：仅接纳该查询ID的独立搜索结果；失败或取消后从头搜索，不复用已消耗游标。空匹配批次不代表整个范围无记录。")
        if (ownedQuery.adopted) heading += "\n" + qsTr("已接纳本页匹配ID/作用域的只读结果；不是完整历史或权限证明。")
        return heading + "\n" + qsTr("收起或关闭本页不会自动取消全局查询或停止录制。")
    }
    function submit(pressed) {
        var life = lifetime
        if (!life.alive || life.exhausted || life.call || !pressScopeCurrent(pressed)
                || !actionAllowed(pressed.intent.action)) { if (life.alive) localError = qsTr("来源、录制批次或确认已变化；没有发送请求，请重新核对。"); return }
        if ((pressed.intent.action === "start" && pressed.persistenceConfirmed !== true)
                || (pressed.intent.action === "seal" && pressed.sealConfirmed !== true)) return
        var owner = pressed.owner, epoch = life.epoch, ticket = {}, receipt = null
        life.call = ticket
        try {
            callBusy = true
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || archive !== owner) return
            persistenceConfirmed = false; sealConfirmed = false; localError = ""
            // Property notifications may synchronously change the selection or
            // shared result. Check scope again, ignoring our own callBusy latch.
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || archive !== owner
                    || !pressScopeCurrent(pressed) || !actionStateCurrent(pressed.intent.action)) return
            try { receipt = owner.request(pressed.intent, true) } catch (error) { receipt = null }
            if (!life.alive) return
            var value = copyValue(receipt)
            if (!receiptKnown(value, pressed.intent)) {
                localError = qsTr("未取得可核实的请求回执；请求效果未知，不自动重试。请看原生状态。"); return
            }
            lastRequest = value
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || owner !== archive || optionsStamp !== pressed.optionsStamp) return
            if (!value.accepted) { localError = qsTr("原生未接受请求：%1。不会自动重试。").arg(text(value.reason)); return }
            if (["catalog", "current", "offline", "page"].indexOf(value.action) >= 0) {
                ownedQuery = {owner: owner, optionsStamp: pressed.optionsStamp, requestId: value.requestId, kind: value.action,
                    intent: pressed.intent, databaseEpoch: pressed.databaseEpoch, sealRunId: pressed.sealRunId,
                    fileFingerprint: pressed.fileFingerprint, adopted: false}
                if (!life.alive || life.call !== ticket || life.epoch !== epoch) return
                cancelledQueryId = ""
                adoptResult()
            } else if (value.action === "cancel" && value.requestId === pressed.intent.queryId) cancelledQueryId = value.requestId
        } finally { if (life.alive && life.call === ticket) { life.call = null; callBusy = false } }
    }
    function invalidateLocalSource() {
        var life = lifetime
        if (!life || !life.alive) return
        if (life.epoch >= 9007199254740991) life.exhausted = true; else life.epoch += 1
        persistenceConfirmed = false; sealConfirmed = false
        ownedQuery = null; savedCatalog = null; savedPage = null; selectedBasename = ""; cancelledQueryId = ""
        savedSearch = null; consumedSearchId = ""
        // Intentionally NO cancel: changing/closing this view must not call
        // a global Store cancellation or stop another panel's recorder.
    }
    onArchiveChanged: invalidateLocalSource()
    onOptionsStampChanged: invalidateLocalSource()
    onCurrentStatusChanged: adoptResult()
    onActualCatalogChanged: adoptResult()
    onActualPageChanged: adoptResult()
    onActualSearchChanged: adoptResult()
    onSearchScopeIndexChanged: invalidateSearchFilters()
    onSearchVehicleFilterChanged: invalidateSearchFilters()
    onSearchOperationIndexChanged: invalidateSearchFilters()
    onExpandedChanged: if (!expanded) { persistenceConfirmed = false; sealConfirmed = false; showDetails = false }
    onRunIdChanged: { persistenceConfirmed = false; sealConfirmed = false }
    Component.onDestruction: { lifetime.alive = false }

    ColumnLayout {
        id: form
        width: root.width; spacing: root.pad
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.expanded ? qsTr("收起本地事件归档（不停止录制/查询）") : qsTr("本地事件归档 · 显式录制 / 只读查询")
            Accessible.name: text
            onClicked: root.expanded = !root.expanded
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("默认不启用、不读盘。不发送飞行命令、不恢复旧批准；查询不要求飞行账号，离线也可使用。") }
        Loader {
            Layout.fillWidth: true; active: root.expanded
            Layout.preferredHeight: item ? item.implicitHeight : 0
            sourceComponent: Component {
                ColumnLayout {
                    width: parent ? parent.width : 1; spacing: root.pad
                    Keys.onPressed: if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) event.accepted = true
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("固定本机目录：%1\n原生状态：%2 · 当前录制批次：%3\n录制启用：%4 · 查询待完成：%5 · 历史读取器占用：%6\n录制线程仍占用：%7（停止请求不等于线程已结束）").arg(root.text(root.options.directoryPath)).arg(root.text(root.currentStatus.state)).arg(root.text(root.runId)).arg(root.known(root.currentStatus.captureEnabled)).arg(root.known(root.currentStatus.historyRequestPending)).arg(root.known(root.currentStatus.archivedReaderActive)).arg(root.known(root.currentStatus.writerActive)) }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("只保存有界诊断摘要：不保存自由理由、SID、原始构型或原始载荷。队列拒绝、轮转缺口、异常退出和限额可能造成缺失；摘要/哈希不是持久化、完整性或认证证明。活动库上限不是全部磁盘上限；封存文件累积，不自动删除或清理。") }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("策略：活动库最多100000条/256 MiB，队列512条/8 MiB，批64条，忙等250 ms；历史页最多2 MiB。本页请求%1条，封存上限%2段；实际固定策略见只读详情。停止并等待录制线程结束后，可显式读取未封存库中保留的各批次记录；不是完整历史恢复。").arg(root.options.pageSize || qsTr("未知")).arg(root.options.maxSealedSegments || qsTr("未知")) }
                    Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("我确认在上述固定目录启用本地诊断持久化及其隐私、磁盘和丢失风险；不自动启用、不保证全量。") ; checked: root.persistenceConfirmed; enabled: root.canStart; onToggled: root.persistenceConfirmed = checked; Accessible.name: text; contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad } }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("显式启用本地持久化（仅请求）"); enabled: root.canStart && root.persistenceConfirmed; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("start", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("请求停止本地录制并排空（不删除文件）"); enabled: root.canStop; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("stop", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("我确认封存当前库并尝试继续录制：会增加磁盘占用，衔接期间可有缺口；请求接受不等于封存或续录成功。") ; checked: root.sealConfirmed; enabled: root.canSeal; onToggled: root.sealConfirmed = checked; Accessible.name: text; contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad } }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("请求封存并继续录制（不清理旧段）"); enabled: root.canSeal && root.sealConfirmed; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("seal", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("只读查询：点击即确认读取固定目录；不会自动启用、停止、封存录制或恢复任何权限。录制中只查当前批次；停止后可查未封存库，也可查询已封存段。") }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取已封存目录（显式读盘）"); enabled: root.canCatalog; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("catalog", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取当前录制批次第一页"); enabled: root.canCurrent; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("current", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("未封存库读取：仅在录制关闭、线程退出且查询槽空闲时可用，不需飞机在线或账号。点击即确认数据库只读及协作锁的创建/释放；Qt 可能回收死进程锁。不会初始化、恢复、删库或清理数据库旁路文件。遗留 journal/wal/shm、锁占用或文件变化会拒绝读取，请保留数据库及旁路文件交工程师处理。") }
                    QGCButton { id: offlineButton; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取停止录制后的未封存库（只读数据库）"); enabled: root.canOffline; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("offline", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true
                        QGCPalette { id: offlinePalette; colorGroupEnabled: offlineButton.enabled }
                        contentItem: QGCLabel { text: offlineButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: offlineButton._showHighlight ? offlinePalette.buttonHighlightText : offlinePalette.buttonText } }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedCatalog ? qsTr("本页请求 %1 读取的目录：%2段；当前目录身份一致：%3。列表按文件名，不按飞行时间排序。").arg(root.savedCatalog.requestId).arg(root.catalogRows.length).arg(root.known(root.catalogCurrent)) : qsTr("尚未取得本页核实的目录；空白不代表磁盘没有历史。") }
                    QGCLabel { Layout.fillWidth: true; visible: !!root.savedCatalog && root.catalogRows.length === 0; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本次已接受的目录结果为空：没有返回可浏览封存段，不代表从未发生过飞行或审批。") }
                    ListView {
                        Layout.fillWidth: true; Layout.preferredHeight: Math.min(4, root.catalogRows.length) * (root.rowHeight * 2 + root.pad)
                        model: root.catalogRows; clip: true; spacing: root.pad / 2
                        Controls.ScrollBar.vertical: Controls.ScrollBar {}
                        delegate: QGCButton {
                            width: ListView.view.width; height: Math.max(root.rowHeight, nameLabel.implicitHeight + root.pad * 2); focusPolicy: Qt.StrongFocus
                            enabled: root.catalogCurrent && !root.callBusy; text: modelData.basename; Accessible.name: qsTr("仅选择封存段，不读文件：%1").arg(text)
                            onClicked: root.selectedBasename = modelData.basename
                            contentItem: QGCLabel { id: nameLabel; text: (root.selectedBasename === modelData.basename ? qsTr("已选 · ") : "") + modelData.basename; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                        }
                    }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取所选封存段第一页"); enabled: root.canSealed; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("page", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("按本次原生游标读取下一页（不跨库/段）"); enabled: root.canNext && (root.savedPage.kind !== "page" || root.savedPage.dto.basename === root.selectedBasename); Accessible.name: text; property var press: null; onPressed: press = root.nextSnapshot(); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("仅请求取消本页查询（等待原生结束）"); enabled: root.canCancel; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("cancel", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: qgcPal.warningText; text: qsTr("本页最近请求：%1 · accepted=%2\n请求ID：%3 · 原因：%4\naccepted仅为排队/请求接受，不是写盘、查询、封存或续录完成。%5").arg(root.text(root.lastRequest.action)).arg(root.known(root.lastRequest.accepted)).arg(root.text(root.lastRequest.requestId)).arg(root.text(root.lastRequest.reason)).arg(root.historyBusy ? qsTr("原生槽仍忙，不自动重试；其他入口结果不被本页接管。") : "") }
                    QGCLabel { Layout.fillWidth: true; visible: !!root.ownedQuery; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.queryStatusText() }
                    QGCLabel { Layout.fillWidth: true; visible: !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: root.localError }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("仅过滤本次已读页（不是全库搜索）；旧v1缺少操作/机号时可能不匹配。输入为空=全部机号。") }
                    Controls.TextField { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.vehicleFilter; maximumLength: 3; selectByMouse: true; inputMethodHints: Qt.ImhDigitsOnly; Accessible.name: qsTr("本页机号筛选，空为全部，1至255"); onTextChanged: root.vehicleFilter = text }
                    QGCLabel { Layout.fillWidth: true; visible: !root.filterValid; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("本页机号筛选只允许空值或无前导零的1–255。") }
                    Controls.ComboBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; model: [qsTr("本页全部操作"), qsTr("能力采纳"), qsTr("能力撤销"), qsTr("基线批准"), qsTr("基线撤销"), qsTr("工程性能声明更新")]; currentIndex: root.operationFilterIndex; Accessible.name: qsTr("仅本页工程操作筛选"); onActivated: root.operationFilterIndex = currentIndex }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedPage ? qsTr("本页已核实读取 %1 行，筛选显示 %2 行\n查询ID：%3 · 类型：%4\n原生当前结果身份仍一致：%5\n起始/后续游标：%6 / %7 · 仍有更多：%8\n历史清理计数：%9 · 游标早于保留区：%10\n本地摘要不代表当前有效性，不恢复账号或飞行权限；旧快照仍只读，下一页只用当前实际查询游标。").arg(root.pageRows.length).arg(root.filteredRows.length).arg(root.savedPage.requestId).arg(root.savedPage.kind === "current" ? qsTr("当次活动录制批次") : root.savedPage.kind === "offline" ? qsTr("停止后的未封存库（保留的各批次）") : qsTr("封存段"))
                        .arg(root.known(root.pageLatest)).arg(root.text(root.savedPage.dto.afterRowId)).arg(root.text(root.savedPage.dto.nextRowId)).arg(root.known(root.savedPage.dto.hasMore)).arg(root.text(root.savedPage.dto.retiredCount)).arg(root.known(root.savedPage.dto.cursorPrecedesRetainedHistory)) : qsTr("尚无本页成功核实的结果；不能把其他入口共享的historyPage当成本次成功。") }
                    QGCLabel { Layout.fillWidth: true; visible: !!root.savedPage && root.savedPage.kind === "offline"; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedPage && root.savedPage.kind === "offline" ? qsTr("未封存库数据库身份：%1\n本次整文件指纹：%2\n读取期间持有协作锁：%3；不表示现在仍持锁或具有密码学认证。后页必须保持原库身份、文件指纹和原生游标；文件变化后请重新读取第一页，旧结果仅保留为历史快照。").arg(root.text(root.savedPage.dto.databaseEpoch)).arg(root.text(root.savedPage.dto.fileFingerprint)).arg(root.known(root.savedPage.dto.cooperativeOwnerLockHeldDuringRead)) : "" }
                    QGCLabel { Layout.fillWidth: true; visible: !!root.savedPage && root.filteredRows.length === 0; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本页没有匹配行；不表示全库无该机号、操作或历史。") }
                    ListView {
                        Layout.fillWidth: true; Layout.preferredHeight: root.filteredRows.length ? Math.max(root.rowHeight * 4, Math.min(520, ScreenTools.defaultFontPixelHeight * 24)) : 0
                        model: root.filteredRows; clip: true; spacing: root.pad
                        Controls.ScrollBar.vertical: Controls.ScrollBar {}
                        delegate: FocusScope {
                            width: ListView.view.width; height: rowLabel.implicitHeight + root.pad * 2; activeFocusOnTab: true
                            Accessible.role: Accessible.StaticText; Accessible.name: rowLabel.text
                            Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.warningText : qgcPal.text; border.width: parent.activeFocus ? 3 : 1; radius: 3 }
                            QGCLabel { id: rowLabel; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad; text: root.rowText(modelData); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                        }
                    }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.searchExpanded ? qsTr("收起跨库保留记录搜索") : qsTr("跨库保留记录搜索（独立查询）"); Accessible.name: text; onClicked: root.searchExpanded = !root.searchExpanded }
                    Loader {
                        Layout.fillWidth: true; active: root.searchExpanded
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                        sourceComponent: Component {
                            ColumnLayout {
                                width: parent ? parent.width : 1; spacing: root.pad
                                QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("跨库搜索 · 原生扫描保留记录，不是上方当前页筛选") }
                                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("两种范围均要求本机录制线程退出且查询槽空闲；不自动停录、不需飞机或账号。点击搜索才读取固定目录，可能较慢。不输入路径、SQL 或原始游标。包含未封存库时，该固定库必须存在且可只读访问；需创建/释放协作锁，Qt 可能回收死进程锁。journal/wal/shm 等拒绝原因请保留文件交工程师处理，不自动清理。") }
                                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("搜索范围") }
                                Controls.ComboBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; model: [qsTr("已封存库"), qsTr("已封存库 + 停止后的未封存库")]; currentIndex: root.searchScopeIndex; Accessible.name: qsTr("跨库搜索范围"); onActivated: root.searchScopeIndex = currentIndex }
                                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("跨库搜索机号（空值为全部，1–255；与上方本页筛选独立）") }
                                Controls.TextField { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.searchVehicleFilter; maximumLength: 3; selectByMouse: true; inputMethodHints: Qt.ImhDigitsOnly; Accessible.name: qsTr("跨库搜索机号，空为全部，1至255"); onTextChanged: root.searchVehicleFilter = text }
                                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("跨库搜索操作（旧记录没有工程操作详情时，不会猜测其操作）") }
                                Controls.ComboBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; model: [qsTr("全部操作 / 未记录操作"), qsTr("能力采纳"), qsTr("能力撤销"), qsTr("基线批准"), qsTr("基线撤销"), qsTr("工程性能声明更新")]; currentIndex: root.searchOperationIndex; Accessible.name: qsTr("跨库搜索操作"); onActivated: root.searchOperationIndex = currentIndex }
                                QGCLabel { Layout.fillWidth: true; visible: !root.searchFiltersValid; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("搜索条件无效：机号只允许空值或无前导零的1–255，范围及操作须使用上方选项。条件版本耗尽时须重新打开该面板。") }
                                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("从头搜索（显式读盘）"); enabled: root.canSearch; Accessible.name: text; property var press: null; onPressed: press = root.searchPressSnapshot(false); onCanceled: press = null; onClicked: { var value = press; press = null; root.submitSearch(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("继续原搜索下一批（显式读盘）"); enabled: root.canContinueSearch; Accessible.name: text; property var press: null; onPressed: press = root.searchPressSnapshot(true); onCanceled: press = null; onClicked: { var value = press; press = null; root.submitSearch(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("仅请求取消本页搜索"); enabled: root.canCancel && root.ownedQuery.kind === "search"; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("cancel", "0"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("一次至多扫描4个分页/256行并返回64条匹配；每次只显示本批，不无限累积。筛选变化、其他查询替换结果、失败或取消后必须从头搜索；不会自动继续。录制未停、读取器仍占用或条件无效时按钮不可用，停止/取消请求接受不等于实际线程已结束。") }
                                QGCLabel { Layout.fillWidth: true; visible: !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: root.localError }
                                QGCLabel { Layout.fillWidth: true; visible: !!root.ownedQuery && root.ownedQuery.kind === "search"; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.queryStatusText() }
                                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedSearch ? qsTr("本批搜索ID：%1\n该结果范围：%2 · 机号：%3 · 操作：%4\n清单指纹：%5\n本批扫描行 / 匹配行：%6 / %7\n已越过的文件 / 范围文件：%8 / %9\n扫描已到范围末尾：%10\n仍为原生最近结果：%11 · 当前筛选仍属原查询：%12\n仅历史：是；历史完整性、跨库原子快照、认证、执行许可、权限恢复及飞行放行均未证明/未授予。")
                                    .arg(root.savedSearch.requestId).arg(root.savedSearch.dto.scope).arg(root.savedSearch.dto.vehicleId || qsTr("全部"))
                                    .arg(root.savedSearch.dto.operation ? root.operationText(root.savedSearch.dto.operation) : qsTr("全部 / 未记录"))
                                    .arg(root.savedSearch.dto.inventoryFingerprint).arg(root.savedSearch.dto.scannedRows).arg(root.savedSearch.dto.matchedRows)
                                    .arg(root.savedSearch.dto.filesCompleted).arg(root.savedSearch.dto.filesTotal).arg(root.known(root.savedSearch.dto.scanComplete))
                                    .arg(root.known(root.searchLatest)).arg(root.known(root.searchFiltersCurrent)) : qsTr("尚无本页成功核实的跨库搜索批次；已有historyPage不代表搜索成功。") }
                                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("按文件名和行号扫描，不按飞行时间排序。空匹配批次≠整个范围没有匹配；扫描完成只表示本次冻结范围的保留记录到达末尾，不证明全部历史、实时新鲜度或跨库原子快照。没有回填旧v1缺失详情，也不恢复任何权限。") }
                                QGCLabel { Layout.fillWidth: true; visible: !!root.savedSearch && root.searchRows.length === 0; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.savedSearch && root.savedSearch.dto.scanComplete ? qsTr("本批没有匹配且已到冻结范围末尾；前面批次可能存在匹配，本页不累计或证明全部历史。") : qsTr("本批没有匹配，搜索仍未完成；只有再次明确点击继续才会扫描下一批。") }
                                Repeater {
                                    model: root.searchCoverage
                                    delegate: QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.searchCoverageText(modelData) }
                                }
                                ListView {
                                    Layout.fillWidth: true; Layout.preferredHeight: root.searchRows.length ? Math.max(root.rowHeight * 4, Math.min(520, ScreenTools.defaultFontPixelHeight * 24)) : 0
                                    model: root.searchRows; clip: true; spacing: root.pad
                                    Controls.ScrollBar.vertical: Controls.ScrollBar {}
                                    delegate: FocusScope {
                                        width: ListView.view.width; height: searchRowLabel.implicitHeight + root.pad * 2; activeFocusOnTab: true
                                        Accessible.role: Accessible.StaticText; Accessible.name: searchRowLabel.text
                                        Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.warningText : qgcPal.text; border.width: parent.activeFocus ? 3 : 1; radius: 3 }
                                        QGCLabel { id: searchRowLabel; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad; text: qsTr("文件 %1：%2\n%3").arg(modelData.fileIndex).arg(modelData.basename).arg(root.rowText(modelData.row)); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                                    }
                                }
                            }
                        }
                    }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.taskHistoryExpanded ? qsTr("收起留存任务诊断目录") : qsTr("留存任务诊断目录及原记录（独立只读查询）"); Accessible.name: text; onClicked: root.taskHistoryExpanded = !root.taskHistoryExpanded }
                    Loader {
                        Layout.fillWidth: true; active: root.taskHistoryExpanded
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                        sourceComponent: Component { FleetArchiveTaskHistoryPanel { width: parent ? parent.width : 1; archive: root.archive } }
                    }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.taskIndexExpanded ? qsTr("收起持久化任务索引") : qsTr("持久化任务索引（独立创建 / 重开 / 续扫）"); Accessible.name: text; onClicked: root.taskIndexExpanded = !root.taskIndexExpanded }
                    Loader {
                        Layout.fillWidth: true; active: root.taskIndexExpanded
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                        sourceComponent: Component { FleetArchiveTaskIndexPanel { width: parent ? parent.width : 1; archive: root.archive } }
                    }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.maintenanceExpanded ? qsTr("收起本地封存段维护") : qsTr("本地归档维护（移动 / 备份 / 独立永久清理）"); Accessible.name: text; onClicked: root.maintenanceExpanded = !root.maintenanceExpanded }
                    Loader {
                        Layout.fillWidth: true; active: root.maintenanceExpanded
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                        sourceComponent: Component { FleetArchiveMaintenancePanel { width: parent ? parent.width : 1; archive: root.archive } }
                    }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.showDetails ? qsTr("折叠收到的完整只读DTO") : qsTr("展开收到的完整只读DTO（非原始载荷）"); Accessible.name: text; onClicked: root.showDetails = !root.showDetails }
                    Loader {
                        Layout.fillWidth: true; active: root.showDetails
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                        sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: qgcPal.text; text: root.fullJson({options: root.options, status: root.currentStatus, request: root.lastRequest, catalog: root.savedCatalog ? root.savedCatalog.dto : null, page: root.savedPage ? root.savedPage.dto : null, search: root.savedSearch ? root.savedSearch.dto : null}); Accessible.name: qsTr("完整收到的本地诊断DTO，只读，不恢复权限"); background: Rectangle { color: qgcPal.windowShade } } }
                    }
                }
            }
        }
    }
}

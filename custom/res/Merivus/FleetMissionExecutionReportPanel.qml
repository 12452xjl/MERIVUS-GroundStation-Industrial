import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Frozen, bounded run observations. No upload, approval or flight-command UI.
ColumnLayout {
    id: root
    property var controller: null
    property bool expanded: false
    property bool showVehicles: false
    property var lifetime: ({alive: true, epoch: 0, exhausted: false})
    property var fileDialog: null
    property var saveIntent: null
    property var callTicket: null
    property bool callBusy: false
    property string message: ""
    property bool protectedExpanded: false
    property bool protectedJsonVisible: false
    property bool protectedBusy: false
    property var protectedTicket: null
    property double protectedEpoch: 0
    property bool protectedExhausted: false
    property var protectedSelection: null
    property var protectedCaptured: null
    property string protectedMessage: ""
    // Plain fence precedes notifying properties. A path/consent ABA cannot
    // revive a pressed intent or a dialog from an earlier file operation.
    property var protectedFileGuard: ({epoch: 0, consentEpoch: 0, exhausted: false, ticket: null, resetting: false})
    property var protectedPendingFile: null
    property var protectedFileOutcome: null
    readonly property var protectedCatalog: controller ? controller.fleetProtectedRunReportCatalog || {} : ({})
    readonly property bool protectedCatalogKnown: checkedProtectedCatalog(protectedCatalog)
    readonly property int protectedSelectedIndex: selectedProtectedIndex()
    readonly property var protectedReport: protectedCaptured && protectedCaptured.owner === controller ? protectedCaptured.report : ({})
    readonly property bool protectedReportKnown: !!protectedCaptured
        && (protectedCaptured.origin === "captured" || protectedCaptured.origin === "imported")
        && checkedProtectedReport(protectedReport) && bounded(protectedCaptured.jsonText, 2097152)
    readonly property bool protectedReady: ready && protectedExpanded && !protectedExhausted
        && !protectedFileGuard.exhausted && !protectedFileGuard.ticket
    readonly property bool protectedCanCapture: protectedReady && protectedSelectedIndex >= 0
        && typeof controller.captureFleetProtectedRunReport === "function"
    readonly property bool protectedFileCurrent: protectedPendingCurrent(protectedPendingFile)
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var state: controller ? controller.fleetExecutionReport || {} : ({})
    readonly property var report: state.report || ({})
    readonly property bool reportKnown: checkedReport(report)
    readonly property string reportId: reportKnown ? report.reportId : ""
    readonly property string reportToken: reportKnown ? report.reportId + ":" + report.contentHash : ""
    readonly property var summary: reportKnown ? report.summary : ({})
    readonly property var observation: reportKnown ? report.observation : ({})
    readonly property string sourceToken: controller ? controller.fleetObservationSourceToken || "" : ""
    readonly property var run: controller ? controller.fleetMissionExecutionObservation || {} : ({})
    readonly property bool ready: visible && enabled && expanded && controller && lifetime.alive && !lifetime.exhausted
        && !callBusy && !callTicket && !protectedBusy && !protectedTicket && !protectedFileGuard.ticket && state.busy !== true && !fileDialog
    readonly property bool canCapture: ready && sourceToken.length > 0 && run.prepared === true && uuid(run.runId)
        && typeof controller.captureFleetExecutionReport === "function"
    readonly property bool saveCurrent: !!saveIntent && saveIntent.owner === controller && saveIntent.epoch === lifetime.epoch
        && saveIntent.reportId === reportId && saveIntent.reportToken === reportToken && reportId.length > 0
    spacing: pad / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function exact(value, fields) {
        if (!value || typeof value !== "object" || Array.isArray(value)) return false
        var names = Object.keys(value)
        if (names.length !== fields.length) return false
        for (var i = 0; i < fields.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, fields[i])) return false
        return true
    }
    function integer(value, minimum, maximum) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= minimum && value <= maximum }

    // Display checks only; the native shared validator verifies the complete
    // historical value/order contract before either report reaches this view.
    function exitCounter(value, zero) {
        return typeof value === "string" && /^(0|[1-9][0-9]{0,19})$/.test(value)
            && (zero || value !== "0") && (value.length < 20 || value <= "18446744073709551615")
    }
    function exitLess(a, b) { return a.length < b.length || (a.length === b.length && a < b) }
    function exitReceiptKnown(value) {
        return exact(value, ["sourceEpoch", "sourceSequence", "receivedMonotonicMs"]) && uuid(value.sourceEpoch)
            && exitCounter(value.sourceSequence, false) && exitCounter(value.receivedMonotonicMs, true)
            && (value.receivedMonotonicMs.length < 19 || value.receivedMonotonicMs <= "9223372036854775807")
    }
    function exitValueKnown(value) {
        if (!exact(value, ["customMode", "baseMode", "systemStatus", "priorMissionReceipt", "exitReceipt", "continuityUnknownAtExit"])
                || !integer(value.customMode, 0, 4294967295) || !integer(value.baseMode, 0, 255)
                || !integer(value.systemStatus, 0, 255) || typeof value.continuityUnknownAtExit !== "boolean"
                || !exitReceiptKnown(value.priorMissionReceipt) || !exitReceiptKnown(value.exitReceipt)) return false
        return value.priorMissionReceipt.sourceEpoch === value.exitReceipt.sourceEpoch
            && exitLess(value.priorMissionReceipt.sourceSequence, value.exitReceipt.sourceSequence)
            && !exitLess(value.exitReceipt.receivedMonotonicMs, value.priorMissionReceipt.receivedMonotonicMs)
    }
    function modeExitKnown(value) {
        if (!exact(value, ["schemaVersion", "observed", "count", "candidate", "first", "last",
                "requestCausalityVerified", "remoteOutcomeVerified", "automaticActionsIssued"])
                || value.schemaVersion !== 1 || !exitCounter(value.count, true)
                || typeof value.observed !== "boolean" || value.observed !== (value.count !== "0")
                || value.requestCausalityVerified !== false || value.remoteOutcomeVerified !== false
                || value.automaticActionsIssued !== false) return false
        if (!value.observed) return value.candidate === "not-observed" && value.first === null && value.last === null
        return value.candidate === "mission-mode-exit-observed-not-outcome"
            && exitValueKnown(value.first) && exitValueKnown(value.last)
            && value.first.exitReceipt.sourceEpoch === value.last.exitReceipt.sourceEpoch
            && (value.count === "1" ? JSON.stringify(value.first) === JSON.stringify(value.last)
                : exitLess(value.first.exitReceipt.sourceSequence, value.last.priorMissionReceipt.sourceSequence)
                    && !exitLess(value.last.priorMissionReceipt.receivedMonotonicMs, value.first.exitReceipt.receivedMonotonicMs))
    }
    function exitReceiptText(value) {
        return qsTr("源 %1 / 序号 %2 / 本机单调毫秒 %3")
            .arg(value.sourceEpoch).arg(value.sourceSequence).arg(value.receivedMonotonicMs)
    }
    function exitValueText(label, value) {
        return qsTr("%1：原 HEARTBEAT customMode=%2，baseMode=%3，systemStatus=%4；当时连续性未知=%5\n此前任务模式接收：%6\n退出接收：%7")
            .arg(label).arg(value.customMode).arg(value.baseMode).arg(value.systemStatus)
            .arg(value.continuityUnknownAtExit ? qsTr("是") : qsTr("否"))
            .arg(exitReceiptText(value.priorMissionReceipt)).arg(exitReceiptText(value.exitReceipt))
    }
    function modeExitText(value) {
        if (value === undefined) return qsTr("旧版资料未采集任务模式退出证据；不能当作零次退出。")
        if (!modeExitKnown(value)) return qsTr("任务模式退出证据未知；不据此推断结果。")
        if (!value.observed) return qsTr("本地观察中未见任务模式退出；不证明持续执行、任务完成或停飞。")
        return qsTr("曾观察任务模式退出 %1 次（不是中止原因或任务结果证明）。仅保留首次/末次，不是完整历史。").arg(value.count)
            + "\n" + exitValueText(qsTr("首次退出"), value.first)
            + "\n" + exitValueText(qsTr("末次退出"), value.last)
            + "\n" + qsTr("接收先后不证明请求因果、远端任务结果或自动动作。")
    }
    function bounded(value, maximum) { return typeof value === "string" && value.length <= maximum }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function text(value) { return typeof value === "string" && value.length ? value.substring(0, 512) : qsTr("未知 / 未提供") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function sequence(value) { return value === null ? qsTr("未观察到") : integer(value, 0, 2047) ? String(value) : qsTr("未知") }
    function positiveRevision(value) {
        return typeof value === "string" && /^[1-9][0-9]{0,19}$/.test(value)
            && (value.length < 20 || value <= "18446744073709551615")
    }
    function checkedProtectedCatalog(value) {
        if (!exact(value, ["schemaVersion", "catalogSource", "sourceId", "records", "readOnly", "historicalOnly"])
                || value.schemaVersion !== 1 || value.catalogSource !== "merivus-protected-run-records"
                || !uuid(value.sourceId) || value.readOnly !== true || value.historicalOnly !== true
                || !value.records || typeof value.records === "string" || value.records.length > 128) return false
        var seen = Object.create(null)
        for (var i = 0; i < value.records.length; ++i) {
            var row = value.records[i]
            if (!exact(row, ["taskId", "runId", "startupId", "deploymentId", "observationRevision", "observationRevisionExhausted",
                    "targetVehicleIds", "modeQueueCommittedObserved", "observationClosed", "observationOwnerDestroyed",
                    "observationUpdatedAt", "observationClosedAt", "selectionToken"])
                    || !uuid(row.taskId) || seen[row.taskId] || !uuid(row.runId) || !uuid(row.startupId) || !uuid(row.deploymentId)
                    || !positiveRevision(row.observationRevision) || !digest(row.selectionToken)
                    || typeof row.observationRevisionExhausted !== "boolean" || typeof row.modeQueueCommittedObserved !== "boolean"
                    || typeof row.observationClosed !== "boolean" || typeof row.observationOwnerDestroyed !== "boolean"
                    || !bounded(row.observationUpdatedAt, 32) || !bounded(row.observationClosedAt, 32)
                    || !row.targetVehicleIds || typeof row.targetVehicleIds === "string"
                    || !integer(row.targetVehicleIds.length, 1, 32)) return false
            var members = Object.create(null)
            for (var j = 0; j < row.targetVehicleIds.length; ++j) {
                var id = row.targetVehicleIds[j]
                if (!integer(id, 1, 255) || members[id]) return false
                members[id] = true
            }
            seen[row.taskId] = true
        }
        return integer(value.records.length, 0, 128)
    }
    function selectedProtectedIndex() {
        var selection = protectedSelection
        if (!protectedCatalogKnown || !selection || selection.owner !== controller
                || selection.epoch !== lifetime.epoch || selection.historyEpoch !== protectedEpoch
                || selection.request.sourceId !== protectedCatalog.sourceId) return -1
        for (var i = 0; i < protectedCatalog.records.length; ++i) {
            var row = protectedCatalog.records[i], request = selection.request
            if (row.taskId === request.taskId && row.observationRevision === request.observationRevision
                    && row.selectionToken === request.selectionToken) return i
        }
        return -1
    }
    function protectedLabels() {
        if (!protectedCatalogKnown) return []
        var labels = []
        for (var i = 0; i < protectedCatalog.records.length; ++i) {
            var row = protectedCatalog.records[i]
            labels.push(qsTr("任务 %1 · Run %2 · 观察修订 %3").arg(row.taskId).arg(row.runId).arg(row.observationRevision))
        }
        return labels
    }
    function invalidateProtected(clearReport, clearText) {
        var guard = lifetime
        if (!guard.alive) return false
        var ownsTicket = !protectedTicket, ticket = protectedTicket || {}, expectedEpoch = protectedEpoch
        var sameReset = function() { return guard.alive && protectedTicket === ticket && protectedEpoch === expectedEpoch }
        if (ownsTicket) protectedTicket = ticket
        if (!guard.alive) return false
        try {
            if (!sameReset()) return false
            if (expectedEpoch >= 9007199254740991) protectedExhausted = true
            else { expectedEpoch += 1; protectedEpoch = expectedEpoch }
            if (!sameReset()) return false
            retireProtectedFile()
            if (!sameReset()) return false
            protectedSelection = null
            if (!sameReset()) return false
            if (protectedCaptureButton) protectedCaptureButton.pressedIntent = null
            if (!sameReset()) return false
            if (protectedConsent) protectedConsent.checked = false
            if (!sameReset()) return false
            if (clearReport) {
                protectedCaptured = null
                if (!sameReset()) return false
                protectedJsonVisible = false
                if (!sameReset()) return false
            }
            if (typeof clearText === "string") protectedMessage = clearText
            return sameReset()
        } finally {
            // A passive invalidation may run inside an active select/capture;
            // never release its ticket. A standalone clear owns its own fence,
            // including its final message, and writes nothing after release.
            if (ownsTicket && guard.alive && protectedTicket === ticket) protectedTicket = null
        }
    }
    function selectProtected(index) {
        if (!protectedReady || !protectedCatalogKnown || !integer(index, 0, protectedCatalog.records.length - 1)) return
        var owner = controller, guard = lifetime, catalog = protectedCatalog, row = catalog.records[index]
        var inputEpoch = guard.epoch, historyEpoch = protectedEpoch, ticket = {}
        var stillSelecting = function(expectedEpoch) {
            return guard.alive && protectedTicket === ticket && owner === controller && guard.epoch === inputEpoch
                && protectedEpoch === expectedEpoch && !protectedExhausted && visible && expanded && protectedExpanded
                && !callBusy && !callTicket && state.busy !== true && !fileDialog && protectedCatalogKnown
                && catalog.sourceId === protectedCatalog.sourceId
        }
        // Hold a ticket before the first public property write. Selection
        // notifications cannot reenter and re-label an old row with a newer
        // panel/input epoch, or overwrite a newer selection made in a callback.
        protectedTicket = ticket
        if (!guard.alive) return
        try {
            if (!stillSelecting(historyEpoch)) return
            invalidateProtected(true)
            if (!stillSelecting(historyEpoch + 1)) return
            var selected = {owner: owner, epoch: inputEpoch, historyEpoch: historyEpoch + 1,
                request: {sourceId: catalog.sourceId, taskId: row.taskId, observationRevision: row.observationRevision, selectionToken: row.selectionToken}}
            protectedSelection = selected
            if (!stillSelecting(historyEpoch + 1) || protectedSelection !== selected) return
            if (selectedProtectedIndex() < 0) {
                invalidateProtected(false, qsTr("目录在选择期间已变化，请重新选择当前条目。"))
                return
            }
            protectedMessage = qsTr("已选择一条本地保留记录，尚未生成报告。请核对来源与修订后确认。")
        } finally {
            if (guard.alive && protectedTicket === ticket) protectedTicket = null
        }
    }
    function protectedIntent() {
        var selection = protectedSelection
        return selectedProtectedIndex() >= 0 && selection
            ? {owner: controller, epoch: lifetime.epoch, historyEpoch: protectedEpoch, selection: selection,
                consentEpoch: protectedFileGuard.consentEpoch,
                request: {sourceId: selection.request.sourceId, taskId: selection.request.taskId,
                    observationRevision: selection.request.observationRevision, selectionToken: selection.request.selectionToken}} : null
    }
    function protectedIntentCurrent(value) {
        return value && lifetime.alive && visible && expanded && protectedExpanded
            && value.owner === controller && value.epoch === lifetime.epoch && value.historyEpoch === protectedEpoch
            && value.consentEpoch === protectedFileGuard.consentEpoch
            && value.selection === protectedSelection && selectedProtectedIndex() >= 0
    }
    function checkedProtectedReport(value) {
        if (!exact(value, ["schemaVersion", "reportType", "reportId", "capturedAtUtc", "record", "summary", "claims", "contentHash"])
                || (value.schemaVersion !== 1 && value.schemaVersion !== 2) || value.reportType !== "fleet-protected-run-observation" || !uuid(value.reportId)
                || !bounded(value.capturedAtUtc, 32) || !value.capturedAtUtc.length || !digest(value.contentHash)) return false
        var record = value.record, s = value.summary
        if (!exact(record, ["schemaVersion", "source", "sourceId", "taskId", "observationRevision", "observationRevisionExhausted",
                "selectionToken", "identity", "targetVehicleIds", "observation", "modeQueueCommittedObserved", "observationBusy",
                "observationClosed", "observationOwnerDestroyed", "observationUpdatedAt", "observationClosedAt"])
                || record.schemaVersion !== value.schemaVersion || record.source !== "merivus-protected-run-record" || !uuid(record.sourceId)
                || !uuid(record.taskId) || !positiveRevision(record.observationRevision) || !digest(record.selectionToken)
                || typeof record.observationRevisionExhausted !== "boolean" || typeof record.observationClosed !== "boolean"
                || typeof record.observationOwnerDestroyed !== "boolean" || typeof record.modeQueueCommittedObserved !== "boolean"
                || typeof record.observationBusy !== "boolean" || !bounded(record.observationUpdatedAt, 32) || !bounded(record.observationClosedAt, 32)
                || !record.identity || !uuid(record.identity.runId) || !uuid(record.identity.startupId) || !uuid(record.identity.deploymentId)) return false
        var counts = ["modeQueueBeganVehicleCount", "modeCommittedVehicleCount", "uncommittedVehicleCount", "sourceUnknownVehicleCount",
            "noTerminalCandidateVehicleCount", "terminalCandidateVehicleCount", "indefiniteHoldCandidateVehicleCount", "landedCandidateVehicleCount"]
        if (!exact(s, ["targetVehicleCount"].concat(counts, ["observerRetired", "startupTerminalState", "startupTerminalReason", "revisionUnambiguous"]))
                || !integer(s.targetVehicleCount, 1, 32) || typeof s.observerRetired !== "boolean" || typeof s.revisionUnambiguous !== "boolean"
                || !bounded(s.startupTerminalState, 128) || !bounded(s.startupTerminalReason, 256)
                || s.revisionUnambiguous !== (!record.observationRevisionExhausted && record.observationRevision !== "18446744073709551615")) return false
        for (var i = 0; i < counts.length; ++i) if (!integer(s[counts[i]], 0, s.targetVehicleCount)) return false
        if (s.modeCommittedVehicleCount + s.uncommittedVehicleCount !== s.targetVehicleCount
                || s.noTerminalCandidateVehicleCount + s.terminalCandidateVehicleCount + s.indefiniteHoldCandidateVehicleCount + s.landedCandidateVehicleCount !== s.targetVehicleCount) return false
        var positive = ["historicalOnly", "snapshotOnly", "reportOnly", "observationOnly"]
        var negative = ["currentOnboardContentsProven", "remoteMissionInstanceVerified", "resultVerified", "missionCompleted", "executionVerified",
            "executionAttributionVerified", "requestCausalityVerified", "authenticated", "signatureVerified", "permissionsRestored", "startupPermitGranted",
            "executionPermit", "executable", "flightCommandReleased", "fullTelemetryCaptured", "tailObservationComplete", "sourceProvenanceVerified", "currentValuesFreshnessVerified"]
        if (!exact(value.claims, positive.concat(negative))) return false
        for (i = 0; i < positive.length; ++i) if (value.claims[positive[i]] !== true) return false
        for (i = 0; i < negative.length; ++i) if (value.claims[negative[i]] !== false) return false
        var compact = record.observation
        if (!compact || compact.schemaVersion !== value.schemaVersion || !Array.isArray(compact.vehicles)
                || compact.vehicles.length !== s.targetVehicleCount) return false
        for (i = 0; i < compact.vehicles.length; ++i) {
            var outcome = compact.vehicles[i].observedOutcome
            if (!outcome || outcome.schemaVersion !== value.schemaVersion
                    || (value.schemaVersion === 2 ? !modeExitKnown(outcome.missionModeExitObservation)
                        : Object.prototype.hasOwnProperty.call(outcome, "missionModeExitObservation"))) return false
        }
        return true // Native codec validates complete rows/refs; these are bounded display checks, not provenance.
    }
    function captureProtected(value) {
        if (!protectedCanCapture || !protectedIntentCurrent(value) || !protectedConsent.checked) return
        var owner = controller, guard = lifetime, ticket = {}, response = null, snapshot = null
        protectedTicket = ticket
        if (!guard.alive) return
        try {
            protectedBusy = true
            if (!guard.alive || protectedTicket !== ticket || !protectedIntentCurrent(value) || !protectedConsent.checked) return
            response = owner.captureFleetProtectedRunReport(value.request, true)
            if (!guard.alive || protectedTicket !== ticket || !protectedIntentCurrent(value) || !protectedConsent.checked) return
            if (!exact(response, ["ok", "error", "report", "jsonText"]) || typeof response.ok !== "boolean" || !bounded(response.error, 512)
                    || !bounded(response.jsonText, 2097152)) { protectedMessage = qsTr("报告回执未知；没有采用报告，也不自动重试。"); return }
            if (!response.ok) { protectedMessage = qsTr("未捕获报告：%1。请重新选择当前条目。").arg(text(response.error)); return }
            // Native compact JSON preserves null/empty fields. Do not
            // JSON.stringify the QML QVariant map: undefined could omit keys.
            var parsed = JSON.parse(response.jsonText)
            if (!checkedProtectedReport(parsed) || !response.report || parsed.reportId !== response.report.reportId
                    || parsed.contentHash !== response.report.contentHash || parsed.record.sourceId !== value.request.sourceId
                    || parsed.record.taskId !== value.request.taskId || parsed.record.observationRevision !== value.request.observationRevision
                    || parsed.record.selectionToken !== value.request.selectionToken || !protectedIntentCurrent(value)) {
                protectedMessage = qsTr("报告类型或所选来源不对应；未显示该报告。"); return
            }
            snapshot = {owner: owner, origin: "captured", report: parsed, jsonText: response.jsonText}
            protectedCaptured = snapshot
            if (!guard.alive || protectedCaptured !== snapshot || !protectedIntentCurrent(value)) return
            protectedJsonVisible = false
            if (!guard.alive || protectedCaptured !== snapshot || !protectedIntentCurrent(value)) return
            protectedMessage = qsTr("已捕获一份本地缓存历史报告，尚未写入文件。后续观察不会更新它；关闭或销毁不等于停飞或任务完成。")
        } catch (error) {
            if (guard.alive && protectedTicket === ticket && protectedIntentCurrent(value)) protectedMessage = qsTr("捕获或解析中断；不自动重试，请核对当前显示。")
        } finally {
            if (guard.alive && protectedTicket === ticket) {
                protectedBusy = false // Keep ticket held across this outward notification.
                if (guard.alive && protectedTicket === ticket) protectedTicket = null
            }
        }
    }
    function protectedFileContext(value) {
        return value && lifetime.alive && !lifetime.exhausted && !protectedExhausted
            && !protectedFileGuard.exhausted && visible && enabled && expanded && protectedExpanded
            && value.owner === controller && value.epoch === lifetime.epoch && value.historyEpoch === protectedEpoch
            && value.fileEpoch === protectedFileGuard.epoch && value.snapshot === protectedCaptured
            && !callBusy && !callTicket && state.busy !== true
            && (value.action === "load" || (value.action === "save" && protectedReportKnown
                && value.reportId === protectedReport.reportId && value.contentHash === protectedReport.contentHash
                && value.jsonText === protectedCaptured.jsonText))
    }
    function protectedFileIntent(action) {
        return {owner: controller, epoch: lifetime.epoch, historyEpoch: protectedEpoch,
            fileEpoch: protectedFileGuard.epoch, consentEpoch: protectedFileGuard.consentEpoch,
            action: action, snapshot: protectedCaptured,
            reportId: protectedReportKnown ? protectedReport.reportId : "",
            contentHash: protectedReportKnown ? protectedReport.contentHash : "",
            jsonText: protectedReportKnown ? protectedCaptured.jsonText : ""}
    }
    function protectedPendingCurrent(value) {
        return value && protectedFileContext(value) && bounded(value.filePath, 4096) && value.filePath.length > 0
    }
    function takeProtectedFileTicket(allowExisting) {
        var guard = lifetime, fence = protectedFileGuard
        if (!guard.alive) return null
        if (fence.ticket) return allowExisting && protectedTicket === fence.ticket ? {ticket: fence.ticket, owns: false} : null
        if (protectedTicket) return null
        var ticket = {}
        fence.ticket = ticket // Before any outward property notification.
        protectedTicket = ticket
        return guard.alive && fence.ticket === ticket && protectedTicket === ticket ? {ticket: ticket, owns: true} : null
    }
    function finishProtectedFileTicket(lease) {
        var guard = lifetime, fence = protectedFileGuard
        if (!lease || !lease.owns || !guard.alive || fence.ticket !== lease.ticket || protectedTicket !== lease.ticket) return
        protectedBusy = false
        if (!guard.alive || fence.ticket !== lease.ticket || protectedTicket !== lease.ticket) return
        fence.ticket = null
        protectedTicket = null // Last write: callbacks may now start a new operation.
    }
    function retireProtectedFile() {
        var guard = lifetime, fence = protectedFileGuard
        if (!guard || !guard.alive || !fence) return false
        var ownsTicket = !protectedTicket, ticket = protectedTicket || {}, ownsFence = !fence.ticket
        var previousEpoch = fence.epoch, requiredConsent = fence.consentEpoch
        var clearConsent = protectedFileConsent && protectedFileConsent.checked
        var clearedConsentEpoch = requiredConsent + (clearConsent ? 1 : 0)
        if (ownsFence) fence.ticket = ticket
        if (ownsTicket) protectedTicket = ticket
        if (!guard.alive) return false
        var expected = previousEpoch
        var same = function() { return guard.alive && protectedTicket === ticket && fence.epoch === expected
            && fence.consentEpoch === requiredConsent }
        var wasResetting = fence.resetting
        fence.resetting = true
        try {
            if (!same()) return false
            if (expected >= 9007199254740991) fence.exhausted = true
            else { expected += 1; fence.epoch = expected }
            if (!same()) return false
            protectedPendingFile = null
            if (!same()) return false
            if (protectedChooseSaveButton) protectedChooseSaveButton.pressedIntent = null
            if (!same()) return false
            if (protectedLoadButton) protectedLoadButton.pressedIntent = null
            if (!same()) return false
            if (protectedFileCommitButton) protectedFileCommitButton.pressedIntent = null
            if (!same()) return false
            if (protectedCancelFileButton) protectedCancelFileButton.pressedIntent = null
            if (!same()) return false
            requiredConsent = clearedConsentEpoch
            if (protectedFileConsent) protectedFileConsent.checked = false
            if (!same() || (protectedFileConsent && protectedFileConsent.checked)) return false
            var view = fileDialog
            if (view && view.protectedFile === true) detachDialog(view)
            return same() && (!protectedFileConsent || !protectedFileConsent.checked)
        } finally {
            fence.resetting = wasResetting
            if (guard.alive && protectedTicket === ticket) {
                if (ownsFence && fence.ticket === ticket) fence.ticket = null
                if (ownsTicket) protectedTicket = null
            }
        }
    }
    function protectedConfirmationChanged(fileConfirmation, checked) {
        var guard = lifetime, fence = protectedFileGuard
        if (!guard.alive) return
        // Even false -> true returns are distinct; do not compare just a bool.
        if (fence.consentEpoch >= 9007199254740991) fence.exhausted = true
        else fence.consentEpoch += 1
        var ownsTicket = !protectedTicket, ticket = protectedTicket || {}, ownsFence = !fence.ticket
        if (ownsFence) fence.ticket = ticket
        if (ownsTicket) protectedTicket = ticket
        if (!guard.alive) return
        try {
            if (protectedCaptureButton) protectedCaptureButton.pressedIntent = null
            if (!guard.alive || protectedTicket !== ticket) return
            if (protectedFileCommitButton) protectedFileCommitButton.pressedIntent = null
            if (!guard.alive || protectedTicket !== ticket) return
            // Revoking a file confirmation also discards that path. Checking it
            // for the first time keeps the newly staged path available.
            if (!fence.resetting && (!fileConfirmation || !checked)) retireProtectedFile()
        } finally {
            if (guard.alive && protectedTicket === ticket) {
                if (ownsFence && fence.ticket === ticket) fence.ticket = null
                if (ownsTicket) protectedTicket = null
            }
        }
    }
    function openProtectedPicker(value) {
        if (!protectedReady || !protectedFileContext(value) || value.consentEpoch !== protectedFileGuard.consentEpoch
                || (value.action !== "save" && value.action !== "load")
                || typeof controller[value.action === "save" ? "saveFleetProtectedRunReport" : "loadFleetProtectedRunReport"] !== "function") return
        var guard = lifetime, lease = takeProtectedFileTicket(false), view = null, viewLife = {alive: true}
        if (!lease) return
        try {
            if (!protectedFileContext(value) || value.consentEpoch !== protectedFileGuard.consentEpoch || fileDialog) return
            var oldEpoch = protectedFileGuard.epoch
            if (!retireProtectedFile() || !guard.alive || protectedFileGuard.epoch !== oldEpoch + 1
                    || value.owner !== controller || value.epoch !== guard.epoch || value.historyEpoch !== protectedEpoch
                    || value.snapshot !== protectedCaptured || protectedFileConsent.checked) return
            var frozen = protectedFileIntent(value.action)
            if (!protectedFileContext(frozen) || protectedFileConsent.checked) return
            view = (value.action === "save" ? protectedSaveDialogComponent : protectedLoadDialogComponent).createObject(root,
                {ownerPanel: root, guard: guard, frozenIntent: frozen, dialogGuard: viewLife})
            if (!guard.alive) return
            if (!view) { protectedMessage = qsTr("独立报告文件窗口未创建；尚未读写文件。"); return }
            if (!viewLife.alive || !protectedFileContext(frozen) || frozen.consentEpoch !== protectedFileGuard.consentEpoch
                    || protectedFileConsent.checked) {
                detachDialog(view, viewLife); return
            }
            fileDialog = view
            if (!guard.alive || !viewLife.alive || fileDialog !== view || !protectedFileContext(frozen)
                    || frozen.consentEpoch !== protectedFileGuard.consentEpoch || protectedFileConsent.checked) { if (guard.alive) detachDialog(view, viewLife); return }
            if (value.action === "save") view.openForSave(); else view.openForLoad()
        } catch (error) {
            if (guard.alive && viewLife.alive && view && fileDialog === view) detachDialog(view, viewLife)
            if (guard.alive && protectedTicket === lease.ticket && frozen && protectedFileContext(frozen)
                    && frozen.consentEpoch === protectedFileGuard.consentEpoch)
                protectedMessage = qsTr("文件窗口调用未确认；不自动读写或重试，请重新选择路径。")
        } finally { finishProtectedFileTicket(lease) }
    }
    function protectedFileChosen(view, value, filePath) {
        if (!lifetime.alive || fileDialog !== view || !view.activeIntent) return
        var guard = lifetime, viewLife = view.dialogGuard, lease = takeProtectedFileTicket(true)
        if (!lease) return
        try {
            if (!viewLife.alive || fileDialog !== view) return
            var accepted = protectedFileContext(value) && value.consentEpoch === protectedFileGuard.consentEpoch
                && !protectedFileConsent.checked && bounded(filePath, 4096) && filePath.length > 0
            detachDialog(view, viewLife)
            if (!guard.alive || protectedTicket !== lease.ticket || !accepted || !protectedFileContext(value)
                    || value.consentEpoch !== protectedFileGuard.consentEpoch || protectedFileConsent.checked || fileDialog) return
            var pending = {owner: value.owner, epoch: value.epoch, historyEpoch: value.historyEpoch, fileEpoch: value.fileEpoch,
                action: value.action, snapshot: value.snapshot, reportId: value.reportId, contentHash: value.contentHash,
                jsonText: value.jsonText, filePath: filePath}
            protectedPendingFile = pending
            if (!guard.alive || protectedTicket !== lease.ticket) return
            if (protectedPendingFile !== pending || !protectedPendingCurrent(pending)
                    || value.consentEpoch !== protectedFileGuard.consentEpoch || protectedFileConsent.checked) {
                if (protectedPendingFile === pending) retireProtectedFile()
                return
            }
            protectedMessage = value.action === "save"
                ? qsTr("路径已暂存，尚未写入。请核对本区报告 ID、hash、路径及覆盖风险，再另行确认。")
                : qsTr("路径已暂存，尚未读取。另行确认后才打开为独立、未认证历史，不恢复 Run 或权限。")
        } finally { finishProtectedFileTicket(lease) }
    }
    function protectedFileRejected(view) {
        if (!lifetime.alive || fileDialog !== view) return
        var guard = lifetime, viewLife = view.dialogGuard, value = view.frozenIntent, lease = takeProtectedFileTicket(true)
        if (!lease) return
        try {
            if (!viewLife.alive || fileDialog !== view) return
            detachDialog(view, viewLife)
            if (guard.alive && protectedTicket === lease.ticket && protectedFileContext(value)
                    && value.consentEpoch === protectedFileGuard.consentEpoch)
                protectedMessage = qsTr("已取消独立报告文件选择，未调用文件读写。")
        } finally { finishProtectedFileTicket(lease) }
    }
    function protectedCommitIntent() {
        return protectedFileCurrent ? {pending: protectedPendingFile, consentEpoch: protectedFileGuard.consentEpoch} : null
    }
    function protectedCommitCurrent(value) {
        return value && value.pending === protectedPendingFile && protectedPendingCurrent(value.pending)
            && value.consentEpoch === protectedFileGuard.consentEpoch && protectedFileConsent.checked && !fileDialog
    }
    function protectedFileFeedbackCurrent(pending, ticket, fileEpoch) {
        return lifetime.alive && protectedTicket === ticket && pending.owner === controller && pending.epoch === lifetime.epoch
            && pending.historyEpoch === protectedEpoch && pending.snapshot === protectedCaptured && protectedFileGuard.epoch === fileEpoch
            && visible && enabled && expanded && protectedExpanded
    }
    function cancelProtectedPending(value) {
        if (!protectedReady || value !== protectedPendingFile || !protectedPendingCurrent(value)) return
        var guard = lifetime, lease = takeProtectedFileTicket(false)
        if (!lease) return
        try {
            if (value !== protectedPendingFile || !protectedPendingCurrent(value)) return
            var expectedFileEpoch = protectedFileGuard.epoch + 1
            if (retireProtectedFile() && protectedFileFeedbackCurrent(value, lease.ticket, expectedFileEpoch))
                protectedMessage = qsTr("已取消本区待确认路径；没有发起文件读写。")
        } finally { finishProtectedFileTicket(lease) }
    }
    function performProtectedFile(value) {
        if (!protectedReady || !protectedCommitCurrent(value)) return
        var guard = lifetime, pending = value.pending, owner = pending.owner, lease = takeProtectedFileTicket(false)
        if (!lease) return
        try {
            protectedBusy = true
            if (!guard.alive || protectedTicket !== lease.ticket || !protectedCommitCurrent(value)) return
            var response = null, threw = false
            try {
                if (pending.action === "save") response = owner.saveFleetProtectedRunReport(pending.filePath, pending.jsonText,
                    pending.reportId, pending.contentHash, true)
                else response = owner.loadFleetProtectedRunReport(pending.filePath, true)
            } catch (error) { threw = true }
            if (!guard.alive || protectedTicket !== lease.ticket) return
            if (pending.action === "save") {
                var valid = !threw && resultKnown(response) && response.reportId === pending.reportId
                // Freeze file facts even when a synchronous callback retired
                // the selection. A stale context does not roll back a write.
                var outcome = {owner: owner, reportId: pending.reportId, filePath: pending.filePath,
                    known: valid, fileWritten: valid ? response.fileWritten : null,
                    fileWriteAttempted: valid ? response.fileWriteAttempted : null,
                    committed: valid ? response.committed : null, current: valid ? response.current : false,
                    error: valid ? response.error : "unconfirmed-file-result"}
                protectedFileOutcome = outcome
                if (!guard.alive || protectedTicket !== lease.ticket) return
                var savedContextCurrent = protectedCommitCurrent(value), beforeRetire = protectedFileGuard.epoch
                if (protectedPendingFile === pending) retireProtectedFile()
                if (guard.alive && savedContextCurrent && protectedFileFeedbackCurrent(pending, lease.ticket, beforeRetire + 1))
                    protectedMessage = qsTr("本次保存已返回；请查看独立文件事实。无论结果是否仍属当前，都不自动重试或宣称回滚。")
                return
            }
            if (!protectedCommitCurrent(value)) {
                if (protectedPendingFile === pending) retireProtectedFile()
                return
            }
            if (threw || !exact(response, ["ok", "error", "report", "jsonText"]) || typeof response.ok !== "boolean"
                    || !bounded(response.error, 512) || !bounded(response.jsonText, 2097152)) {
                var failedReadEpoch = protectedFileGuard.epoch + 1
                if (protectedPendingFile === pending && retireProtectedFile()
                        && protectedFileFeedbackCurrent(pending, lease.ticket, failedReadEpoch))
                    protectedMessage = qsTr("独立报告读取或回执未确认；未采用报告，不自动重试。")
                return
            }
            if (!response.ok) {
                var errorText = response.error
                var deniedReadEpoch = protectedFileGuard.epoch + 1
                if (retireProtectedFile() && protectedFileFeedbackCurrent(pending, lease.ticket, deniedReadEpoch))
                    protectedMessage = qsTr("未打开独立报告：%1。旧报告保留，请核对文件后重新选择。").arg(text(errorText))
                return
            }
            var jsonText = response.jsonText, parsed = JSON.parse(jsonText)
            if (!checkedProtectedReport(parsed) || !response.report || parsed.reportId !== response.report.reportId
                    || parsed.contentHash !== response.report.contentHash || !protectedCommitCurrent(value)) {
                var invalidReadEpoch = protectedFileGuard.epoch + 1
                if (retireProtectedFile() && protectedFileFeedbackCurrent(pending, lease.ticket, invalidReadEpoch))
                    protectedMessage = qsTr("不是完整的独立保护记录报告；未采用，也未交给下方完整 Run 报告接口。")
                return
            }
            var historyEpoch = pending.historyEpoch
            if (!invalidateProtected(false) || !guard.alive || protectedTicket !== lease.ticket
                    || owner !== controller || pending.epoch !== guard.epoch || protectedEpoch !== historyEpoch + 1
                    || protectedCaptured !== pending.snapshot || !visible || !enabled || !expanded || !protectedExpanded) return
            var snapshot = {owner: owner, origin: "imported", report: parsed, jsonText: jsonText, openedFrom: pending.filePath}
            var fileEpoch = protectedFileGuard.epoch
            protectedCaptured = snapshot
            if (!guard.alive || protectedTicket !== lease.ticket || protectedCaptured !== snapshot || owner !== controller
                    || pending.epoch !== guard.epoch || protectedEpoch !== historyEpoch + 1 || protectedFileGuard.epoch !== fileEpoch + 1) return
            protectedJsonVisible = false
            if (!guard.alive || protectedTicket !== lease.ticket || protectedCaptured !== snapshot || owner !== controller
                    || pending.epoch !== guard.epoch || protectedEpoch !== historyEpoch + 1 || protectedFileGuard.epoch !== fileEpoch + 1) return
            protectedMessage = qsTr("已打开本区独立历史文件。结构/hash 校验不认证来源；即使 sourceId 与目录相同，也不是当前原生记录，不恢复任务或权限。")
        } catch (error) {
            if (guard.alive && protectedTicket === lease.ticket) {
                var interruptedEpoch = protectedFileGuard.epoch + 1
                if (protectedPendingFile === pending && retireProtectedFile()
                        && protectedFileFeedbackCurrent(pending, lease.ticket, interruptedEpoch))
                    protectedMessage = qsTr("文件操作或解析中断；不自动重试。若已请求保存，文件可能已写入，请核对路径。")
            }
        } finally { finishProtectedFileTicket(lease) }
    }
    function checkedReport(value) {
        if (!exact(value, ["schemaVersion", "reportType", "reportId", "capturedAtUtc", "source", "observation", "summary", "claims", "contentHash"])
                || (value.schemaVersion !== 1 && value.schemaVersion !== 2) || value.reportType !== "fleet-run-observation" || !uuid(value.reportId)
                || !bounded(value.capturedAtUtc, 32) || !value.capturedAtUtc.length || !digest(value.contentHash)
                || !exact(value.source, ["lifecycleEpoch", "selectionGeneration", "episodeId"])
                || !uuid(value.source.lifecycleEpoch) || !uuid(value.source.episodeId)
                || typeof value.source.selectionGeneration !== "string" || !/^[1-9][0-9]{0,19}$/.test(value.source.selectionGeneration)
                || !value.observation || typeof value.observation !== "object" || !uuid(value.observation.runId)
                || value.observation.schemaVersion !== value.schemaVersion) return false
        var trueClaims = ["historicalOnly", "snapshotOnly", "reportOnly", "observationOnly"]
        var falseClaims = ["currentOnboardContentsProven", "remoteMissionInstanceVerified", "resultVerified", "missionCompleted",
            "executionVerified", "executionAttributionVerified", "requestCausalityVerified", "authenticated", "signatureVerified",
            "permissionsRestored", "startupPermitGranted", "executionPermit", "executable", "flightCommandReleased",
            "fullTelemetryCaptured", "tailObservationComplete", "sourceProvenanceVerified", "currentValuesFreshnessVerified"]
        if (!exact(value.claims, trueClaims.concat(falseClaims))) return false
        for (var i = 0; i < trueClaims.length; ++i) if (value.claims[trueClaims[i]] !== true) return false
        for (i = 0; i < falseClaims.length; ++i) if (value.claims[falseClaims[i]] !== false) return false
        var s = value.summary
        var counts = ["modeQueueBeganVehicleCount", "modeCommittedVehicleCount", "uncommittedVehicleCount", "sourceUnknownVehicleCount",
            "noTerminalCandidateVehicleCount", "terminalCandidateVehicleCount", "indefiniteHoldCandidateVehicleCount", "landedCandidateVehicleCount"]
        if (!exact(s, ["targetVehicleCount"].concat(counts, ["observerRetired", "startupTerminalState", "startupTerminalReason", "vehicles"]))
                || !integer(s.targetVehicleCount, 1, 32) || typeof s.observerRetired !== "boolean"
                || !bounded(s.startupTerminalState, 128) || !bounded(s.startupTerminalReason, 256)
                || !s.vehicles || typeof s.vehicles === "string" || s.vehicles.length !== s.targetVehicleCount) return false
        for (i = 0; i < counts.length; ++i) if (!integer(s[counts[i]], 0, s.targetVehicleCount)) return false
        if (s.modeCommittedVehicleCount + s.uncommittedVehicleCount !== s.targetVehicleCount
                || s.noTerminalCandidateVehicleCount + s.terminalCandidateVehicleCount + s.indefiniteHoldCandidateVehicleCount + s.landedCandidateVehicleCount !== s.targetVehicleCount) return false
        var seen = Object.create(null)
        for (i = 0; i < s.vehicles.length; ++i) {
            var row = s.vehicles[i]
            var rowKeys = ["vehicleId", "componentId", "phase", "reason", "modeQueueBegan", "modeQueueCommitted", "sourceCurrent", "continuityUnknown",
                    "currentSequence", "reachedSequence", "observedCurrentItemCount", "observedReachedItemCount", "outcomeCandidate", "observationWindow"]
            if (value.schemaVersion === 2) rowKeys.push("missionModeExitObservation")
            if (!exact(row, rowKeys)
                    || !integer(row.vehicleId, 1, 255) || !integer(row.componentId, 0, 255) || seen[row.vehicleId]
                    || !bounded(row.phase, 128) || !bounded(row.reason, 256)
                    || typeof row.modeQueueBegan !== "boolean" || typeof row.modeQueueCommitted !== "boolean"
                    || typeof row.sourceCurrent !== "boolean" || typeof row.continuityUnknown !== "boolean"
                    || !(row.currentSequence === null || integer(row.currentSequence, 0, 2047))
                    || !(row.reachedSequence === null || integer(row.reachedSequence, 0, 2047))
                    || !integer(row.observedCurrentItemCount, 0, 2048) || !integer(row.observedReachedItemCount, 0, 2048)
                    || ["no-terminal-candidate-observed", "terminal-item-observed-not-completion", "indefinite-terminal-hold-observed", "landed-after-terminal-observed-not-completion"].indexOf(row.outcomeCandidate) < 0
                    || ["observation-retired", "awaiting-native-mode-queue", "source-ended-or-unknown", "provisional-native-queue-window", "committed-native-source-bound"].indexOf(row.observationWindow) < 0
                    || (value.schemaVersion === 2 && (!modeExitKnown(row.missionModeExitObservation)
                        || (!row.modeQueueCommitted && row.missionModeExitObservation.observed)))) return false
            seen[row.vehicleId] = true
        }
        return true
    }
    function candidateText(value) {
        if (value === "no-terminal-candidate-observed") return qsTr("未观察到终端线索（不是失败结论）")
        if (value === "terminal-item-observed-not-completion") return qsTr("观察到终端项线索（不证明完成）")
        if (value === "indefinite-terminal-hold-observed") return qsTr("观察到无限期终端保持线索")
        if (value === "landed-after-terminal-observed-not-completion") return qsTr("终端后落地线索（不证明完成或当前落地）")
        return qsTr("未知")
    }
    function windowText(value) {
        if (value === "observation-retired") return qsTr("本地监视已退休，尾部不完整")
        if (value === "awaiting-native-mode-queue") return qsTr("尚未记录本地 MODE 入队")
        if (value === "source-ended-or-unknown") return qsTr("来源结束或未知")
        if (value === "provisional-native-queue-window") return qsTr("本地入队暂定观察窗口")
        if (value === "committed-native-source-bound") return qsTr("当次已提交入队且来源绑定（不是新鲜度证明）")
        return qsTr("未知")
    }
    function ensureFocusVisible(item) {
        if (!expanded || !item || !reportScroller) return
        var point = item.mapToItem(reportBody, 0, 0)
        var top = point.y - pad, bottom = point.y + item.height + pad
        var maximum = Math.max(0, reportScroller.contentHeight - reportScroller.height)
        if (top < reportScroller.contentY || item.height + pad * 2 > reportScroller.height)
            reportScroller.contentY = Math.min(maximum, Math.max(0, top))
        else if (bottom > reportScroller.contentY + reportScroller.height)
            reportScroller.contentY = Math.min(maximum, Math.max(0, bottom - reportScroller.height))
    }
    function clearPressed() {
        if (protectedCaptureButton) protectedCaptureButton.pressedIntent = null
        if (protectedChooseSaveButton) protectedChooseSaveButton.pressedIntent = null
        if (protectedLoadButton) protectedLoadButton.pressedIntent = null
        if (protectedFileCommitButton) protectedFileCommitButton.pressedIntent = null
        if (captureButton) captureButton.pressedIntent = null
        if (saveButton) saveButton.pressedIntent = null
        if (loadButton) loadButton.pressedIntent = null
        if (clearButton) clearButton.pressedIntent = null
        if (writeButton) writeButton.pressedIntent = null
    }
    function discardSave() {
        saveIntent = null
        if (writeConsent) writeConsent.checked = false
        if (writeButton) writeButton.pressedIntent = null
    }
    function detachDialog(view, knownDialogLife) {
        if (!view) return
        var guard = lifetime, dialogLife = knownDialogLife || view.dialogGuard
        if (!dialogLife || !dialogLife.alive) return
        view.activeIntent = false
        if (!guard.alive || !dialogLife.alive) return
        if (fileDialog === view) fileDialog = null
        if (!guard.alive || !dialogLife.alive) return // Parent destruction owns child disposal.
        try { view.close() } catch (error) { if (guard.alive) message = qsTr("窗口关闭未确认；该旧文件选择已失效。") }
        if (guard.alive && dialogLife.alive) view.destroy()
    }
    function invalidateIntents() {
        if (lifetime.epoch >= 9007199254740991) lifetime.exhausted = true
        else lifetime.epoch += 1
        clearPressed(); discardSave()
        invalidateProtected(true)
        var old = fileDialog
        fileDialog = null
        detachDialog(old)
    }
    function intent(action) {
        return {owner: controller, epoch: lifetime.epoch, action: action, reportId: reportId, reportToken: reportToken,
            sourceToken: sourceToken, runId: run.runId || "", filePath: action === "save" && saveCurrent ? saveIntent.filePath : ""}
    }
    function intentCurrent(value) {
        return value && lifetime.alive && value.owner === controller && value.epoch === lifetime.epoch && visible && expanded
            && (value.action === "capture" ? value.sourceToken === sourceToken && value.runId === run.runId
                : value.reportId === reportId && value.reportToken === reportToken)
    }
    function openPicker(value, save) {
        if (!ready || !intentCurrent(value) || (save && !reportKnown)) return
        var guard = lifetime
        discardSave()
        if (!guard.alive) return
        message = ""
        if (!guard.alive || !ready || !intentCurrent(value)) return
        var view = (save ? saveDialogComponent : loadDialogComponent).createObject(root, {ownerPanel: root, guard: guard, frozenIntent: value})
        if (!guard.alive) return
        if (!view) { message = qsTr("文件选择窗口未创建；请重试。尚未调用报告读写接口。"); return }
        fileDialog = view
        if (!guard.alive || fileDialog !== view) return
        if (!intentCurrent(value)) { detachDialog(view); return }
        try { if (save) view.openForSave(); else view.openForLoad() }
        catch (error) {
            if (!guard.alive) return
            if (fileDialog === view) detachDialog(view)
            message = qsTr("文件选择调用中断；未自动重试。若此前已提交操作，请核对本页结果和文件，不把窗口异常视为回滚。")
        }
    }
    function fileChosen(view, value, filePath, save) {
        if (fileDialog !== view || !view.activeIntent) return
        var guard = lifetime
        var accepted = intentCurrent(value) && !callBusy && state.busy !== true && bounded(filePath, 4096) && filePath.length > 0
        detachDialog(view)
        if (!guard.alive) return
        if (!accepted || !intentCurrent(value) || !ready) { message = qsTr("文件选择期间上下文或路径已失效；旧路径已废弃，请重新选择。"); return }
        if (save) {
            saveIntent = {owner: value.owner, epoch: value.epoch, reportId: value.reportId, reportToken: value.reportToken, filePath: filePath}
            writeConsent.checked = false
            message = qsTr("路径已选择，尚未写文件。请核对下面的报告 ID 和路径，再明确确认写入。")
        } else {
            value.action = "load"; value.filePath = filePath
            perform(value)
        }
    }
    function fileRejected(view) {
        if (fileDialog !== view) return
        var guard = lifetime
        detachDialog(view)
        if (guard.alive) message = qsTr("已取消文件选择，没有调用报告读写接口。")
    }
    function resultKnown(value) {
        return exact(value, ["ok", "error", "committed", "current", "fileWriteAttempted", "fileWritten", "reportId"])
            && typeof value.ok === "boolean" && bounded(value.error, 512) && typeof value.committed === "boolean"
            && typeof value.current === "boolean" && typeof value.fileWriteAttempted === "boolean"
            && (value.fileWritten === null || typeof value.fileWritten === "boolean") && bounded(value.reportId, 128)
    }
    function perform(value) {
        if (!ready || !intentCurrent(value) || (value.action === "capture" && !canCapture)
                || (value.action === "save" && (!saveCurrent || !writeConsent.checked || value.filePath !== saveIntent.filePath))) return
        var owner = controller, guard = lifetime, ticket = {}, result = null, threw = false
        callTicket = ticket; callBusy = true; clearPressed()
        if (!guard.alive) return
        if (!intentCurrent(value) || callTicket !== ticket
                || (value.action === "save" && (!saveCurrent || !writeConsent.checked || value.filePath !== saveIntent.filePath))) {
            callBusy = false
            if (guard.alive && callTicket === ticket) callTicket = null
            return
        }
        try {
            if (value.action === "capture") result = owner.captureFleetExecutionReport(value.sourceToken, value.runId)
            else if (value.action === "save") result = owner.saveFleetExecutionReport(value.filePath, value.reportId)
            else if (value.action === "load") result = owner.loadFleetExecutionReport(value.filePath)
            else if (value.action === "clear") result = owner.clearFleetExecutionReport(value.reportId)
        } catch (error) { threw = true }
        if (!guard.alive) return
        if (callTicket === ticket) callBusy = false
        if (!guard.alive) return
        if (callTicket === ticket) callTicket = null
        if (value.action === "save") discardSave()
        if (threw || !resultKnown(result)) {
            message = value.action === "save"
                ? qsTr("保存调用或回执未确认；文件可能已经写入。请核对所选路径，不自动重试，也不声称已经回滚。")
                : qsTr("报告操作或回执未确认；本地报告可能已变化。请核对当前报告，不自动重试。")
            return
        }
        var sameOwner = owner === controller && value.epoch === guard.epoch
        if (value.action === "save") {
            message = result.fileWritten === true
                ? qsTr("文件已写入（报告 %1）。%2").arg(text(result.reportId)).arg(result.ok && result.current && sameOwner ? qsTr("这是冻结历史报告，不是执行许可。") : qsTr("后续上下文未确认；已写事实不撤销，请核对文件。"))
                : result.fileWritten === null ? qsTr("已尝试保存，但文件是否写入未知；请核对路径，勿把失败当作未写入。")
                : qsTr("未确认文件写入成功；已尝试写入：%1。原因：%2。请核对路径后再决定是否重试。").arg(known(result.fileWriteAttempted)).arg(text(result.error))
        } else if (result.ok && result.current && sameOwner && result.committed) {
            message = value.action === "clear" ? qsTr("已清除本页冻结报告；没有删除文件或改变 Run。")
                : value.action === "load" ? qsTr("已打开只读历史报告。结构与内容校验不等于来源认证；没有恢复任务或权限。")
                : qsTr("已冻结当次实际 Run 观察；后续实时变化不会更新这份报告。尚未导出文件。")
        } else {
            message = qsTr("本地报告修改已提交：%1；结果仍属当前：%2。原因：%3。提交事实不因后续上下文变化而撤销。")
                .arg(known(result.committed)).arg(known(result.current && sameOwner)).arg(text(result.error))
        }
    }
    function showCheckpointReport(response) {
        // A recovered checkpoint is an imported historical report even when
        // its source UUID happens to equal this process's current source.
        if (!ready || !controller.runCheckpointWorkspace
                || !exact(response, ["ok", "error", "historyId", "index", "report", "jsonText"])
                || response.ok !== true || response.error !== "" || !uuid(response.historyId)
                || !integer(response.index, 0, 127) || !bounded(response.jsonText, 2097152)) return false
        var owner = controller, workspace = owner.runCheckpointWorkspace, guard = lifetime
        var epoch = guard.epoch, historyId = response.historyId, original = protectedCaptured
        if (workspace.history.historyId !== historyId) return false
        var parsed
        try { parsed = JSON.parse(response.jsonText) } catch (error) { return false }
        if (!checkedProtectedReport(parsed) || !response.report
                || parsed.reportId !== response.report.reportId || parsed.contentHash !== response.report.contentHash) return false
        var ticket = {}
        protectedTicket = ticket
        var current = function() {
            return guard.alive && owner === controller && epoch === guard.epoch
                && workspace === owner.runCheckpointWorkspace && workspace.history.historyId === historyId
                && protectedTicket === ticket && visible && enabled && expanded
        }
        try {
            if (!current() || protectedCaptured !== original || !invalidateProtected(false)) return false
            var historyEpoch = protectedEpoch
            if (!current() || protectedCaptured !== original) return false
            protectedExpanded = true
            if (!current() || protectedEpoch !== historyEpoch || protectedCaptured !== original) return false
            var snapshot = {owner: owner, origin: "imported", report: parsed,
                jsonText: response.jsonText, openedFrom: workspace.history.filePath}
            protectedCaptured = snapshot
            if (!current() || protectedEpoch !== historyEpoch || protectedCaptured !== snapshot) return false
            protectedJsonVisible = false
            if (!current() || protectedEpoch !== historyEpoch || protectedCaptured !== snapshot) return false
            protectedMessage = qsTr("已从独立检查点历史槽载入冻结报告。不是当前 Run，不恢复观察器、任务、占用或批准；本报告也不证明远端执行结果。")
            return current() && protectedCaptured === snapshot
        } finally { if (guard.alive && protectedTicket === ticket) protectedTicket = null }
    }

    onControllerChanged: { invalidateIntents(); showVehicles = false }
    onVisibleChanged: if (!visible) invalidateIntents()
    onEnabledChanged: if (!enabled) invalidateIntents()
    onExpandedChanged: if (!expanded) invalidateIntents()
    onReportTokenChanged: { clearPressed(); discardSave(); showVehicles = false; if (fileDialog && fileDialog.protectedFile !== true) detachDialog(fileDialog) }
    onSourceTokenChanged: if (captureButton) captureButton.pressedIntent = null
    onProtectedExpandedChanged: if (!protectedExpanded) invalidateProtected(true)
    onProtectedCapturedChanged: retireProtectedFile()
    onProtectedCatalogChanged: {
        if (protectedSelection && selectedProtectedIndex() < 0) invalidateProtected(false)
        if (protectedCaptured && protectedCaptured.origin === "captured"
                && (!protectedCatalogKnown || protectedCaptured.report.record.sourceId !== protectedCatalog.sourceId)) invalidateProtected(true)
    }
    Component.onDestruction: { lifetime.alive = false; invalidateIntents() }

    QGCButton {
        id: sectionButton
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: root.expanded ? qsTr("收起运行复盘报告 · 只读历史") : qsTr("展开运行复盘报告 · 捕获 / JSON 导出与打开")
        Accessible.name: text
        contentItem: QGCLabel { text: sectionButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: sectionButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        background: Rectangle { color: sectionButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: sectionButton.activeFocus ? 3 : 1; radius: 3 }
        onClicked: root.expanded = !root.expanded
    }
    Flickable {
        id: reportScroller
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(reportBody.implicitHeight, ScreenTools.defaultFontPixelHeight * 18)
        Layout.maximumHeight: ScreenTools.defaultFontPixelHeight * 18
        visible: root.expanded; clip: true
        contentWidth: width; contentHeight: reportBody.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        Controls.ScrollBar.vertical: Controls.ScrollBar { }
        ColumnLayout {
        id: reportBody
        width: reportScroller.width; spacing: root.pad
        FleetRunCheckpointPanel {
            Layout.fillWidth: true
            workspace: root.controller ? root.controller.runCheckpointWorkspace : null
            reportConsumer: function(value) { return root.showCheckpointReport(value) }
            onFocusRequested: root.ensureFocusVisible(item)
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
            text: qsTr("运行复盘不是飞前审阅或任务完成证明。仅冻结一次本地 Run 观察；无完整遥测、尾部完整性、实时新鲜度或远端任务归因保证。打开外部 JSON 无需飞机或账号，导入材料未认证、只读，不恢复任务、批准或权限。")
        }
        QGCButton {
            id: protectedSectionButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.protectedExpanded ? qsTr("收起本进程保留的 Run 记录") : qsTr("选择本进程保留的 Run · 独立缓存报告")
            enabled: !root.callBusy && !root.protectedBusy
            Accessible.name: text
            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedSectionButton)
            onClicked: root.protectedExpanded = !root.protectedExpanded
            contentItem: QGCLabel { text: protectedSectionButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: protectedSectionButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: protectedSectionButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: protectedSectionButton.activeFocus ? 3 : 1; radius: 3 }
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.protectedExpanded; spacing: root.pad / 2
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("这是同一地面站进程保留的保护记录，不是磁盘档案，也不是当前所选 Run。选择条目不会改变任务、读取飞机或恢复控制权；最多 128 条，必须明确选择，不自动生成报告。")
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("本区格式与下方完整 Run 报告分开。可另选路径并确认保存，或明确选择文件后再次确认打开；只读历史不恢复 Run、批准或权限。不自动读写、上传或写入剪贴板。")
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: root.protectedCatalogKnown
                    ? qsTr("本地来源：%1；记录数：%2。目录时间只表示本地记录更新时间，不是遥测新鲜度。").arg(root.protectedCatalog.sourceId).arg(root.protectedCatalog.records.length)
                    : qsTr("保留记录来源未知或目录不可用。没有用当前 Run 或导入材料替代。")
            }
            Controls.ComboBox {
                id: protectedPicker
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                model: root.protectedLabels()
                currentIndex: root.protectedSelectedIndex
                displayText: currentIndex >= 0 && currentIndex < model.length ? model[currentIndex] : qsTr("请选择一条任务 / Run / 观察修订")
                enabled: root.protectedReady && root.protectedCatalogKnown && root.protectedCatalog.records.length > 0
                Accessible.name: qsTr("本地受保护 Run 记录选择，不自动选第一条")
                onActivated: root.selectProtected(index)
                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedPicker)
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: root.protectedSelectedIndex >= 0
                    ? qsTr("已选任务：%1\n观察修订：%2\n选择标记：%3\n这是缓存选择，不证明当前机载状态；目录推进后需重新选择。").arg(root.protectedSelection.request.taskId).arg(root.protectedSelection.request.observationRevision).arg(root.protectedSelection.request.selectionToken)
                    : qsTr("尚无有效选择。已有冻结报告即使保留，也不会随目录推进而更新。")
            }
            Controls.CheckBox {
                id: protectedConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                enabled: root.protectedCanCapture; checked: false
                text: qsTr("确认缓存报告边界")
                Accessible.name: text
                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedConsent)
                onCheckedChanged: root.protectedConfirmationChanged(false, checked)
            }
            QGCButton {
                id: protectedCaptureButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("捕获所选保留记录（替换本区单份报告）")
                enabled: root.protectedCanCapture && protectedConsent.checked
                property var pressedIntent: null
                Accessible.name: text
                onPressed: pressedIntent = root.protectedIntent()
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.captureProtected(value) }
                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedCaptureButton)
                contentItem: QGCLabel { text: protectedCaptureButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: protectedCaptureButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                background: Rectangle { color: protectedCaptureButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: protectedCaptureButton.activeFocus ? 3 : 1; radius: 3 }
            }
            QGCLabel {
                Layout.fillWidth: true; visible: root.protectedBusy || root.protectedMessage.length > 0
                textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                text: root.protectedBusy ? qsTr("正在处理本区独立历史报告，请勿重复提交。") : root.protectedMessage
            }
            QGCButton {
                id: protectedLoadButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("选择独立保护记录报告文件（下一步确认后读取）")
                enabled: root.protectedReady && typeof root.controller.loadFleetProtectedRunReport === "function"
                Accessible.name: text
                property var pressedIntent: null
                onPressed: pressedIntent = root.protectedFileIntent("load")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.openProtectedPicker(value) }
                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedLoadButton)
                contentItem: QGCLabel { text: protectedLoadButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: protectedLoadButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                background: Rectangle { color: protectedLoadButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: protectedLoadButton.activeFocus ? 3 : 1; radius: 3 }
            }
            ColumnLayout {
                Layout.fillWidth: true; visible: root.protectedReportKnown; spacing: root.pad / 2
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: root.protectedReportKnown
                        ? (root.protectedCaptured.origin === "imported"
                            ? qsTr("显示来源：外部文件导入、未认证历史。文件：%1。sourceId 即使与目录相同，也不是本次原生目录选择。").arg(root.protectedCaptured.openedFrom)
                            : qsTr("显示来源：本进程保留记录的原生冻结捕获；后续目录推进不会更新此报告。")) : ""
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: root.protectedReportKnown ? qsTr("独立保护缓存报告：%1\n捕获时间（UTC）：%2\n任务 / Run：%3 / %4\n部署：%5\n观察修订：%6；修订未耗尽：%7\n本地观察已关闭：%8；原记录ownerDestroyed：%9\n内容 hash：%10（非签名或来源认证）")
                        .arg(root.protectedReport.reportId).arg(root.protectedReport.capturedAtUtc).arg(root.protectedReport.record.taskId)
                        .arg(root.protectedReport.record.identity.runId).arg(root.protectedReport.record.identity.deploymentId)
                        .arg(root.protectedReport.record.observationRevision).arg(root.known(root.protectedReport.summary.revisionUnambiguous))
                        .arg(root.known(root.protectedReport.record.observationClosed)).arg(root.known(root.protectedReport.record.observationOwnerDestroyed))
                        .arg(root.protectedReport.contentHash) : ""
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: root.protectedReportKnown ? qsTr("原组 %1 架；MODE 曾开始入队 %2 架，已捕获提交 %3 架，未捕获提交 %4 架。\n捕获的缓存中来源未知 %5 架；无终端线索 %6，终端项 %7，无限期保持 %8，终端后落地 %9。\n修订耗尽时仍是一次明确历史捕获，不宣称该修订唯一。关闭、销毁与任一候选都不证明任务完成、停飞或当前落地。").arg(root.protectedReport.summary.targetVehicleCount)
                        .arg(root.protectedReport.summary.modeQueueBeganVehicleCount).arg(root.protectedReport.summary.modeCommittedVehicleCount)
                        .arg(root.protectedReport.summary.uncommittedVehicleCount).arg(root.protectedReport.summary.sourceUnknownVehicleCount)
                        .arg(root.protectedReport.summary.noTerminalCandidateVehicleCount).arg(root.protectedReport.summary.terminalCandidateVehicleCount)
                        .arg(root.protectedReport.summary.indefiniteHoldCandidateVehicleCount).arg(root.protectedReport.summary.landedCandidateVehicleCount) : ""
                }
                QGCButton {
                    id: protectedChooseSaveButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("选择本区独立报告保存路径（下一步确认覆盖风险）")
                    enabled: root.protectedReady && root.protectedReportKnown && typeof root.controller.saveFleetProtectedRunReport === "function"
                    Accessible.name: text
                    property var pressedIntent: null
                    onPressed: pressedIntent = root.protectedFileIntent("save")
                    onCanceled: pressedIntent = null
                    onClicked: { var value = pressedIntent; pressedIntent = null; root.openProtectedPicker(value) }
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedChooseSaveButton)
                    contentItem: QGCLabel { text: protectedChooseSaveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: protectedChooseSaveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: protectedChooseSaveButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: protectedChooseSaveButton.activeFocus ? 3 : 1; radius: 3 }
                }
                QGCButton {
                    id: protectedJsonButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: root.protectedJsonVisible ? qsTr("收起退出证据与完整原生 JSON") : qsTr("展开退出证据与完整原生 JSON（只读）")
                    enabled: root.protectedReady
                    Accessible.name: text
                    onClicked: root.protectedJsonVisible = !root.protectedJsonVisible
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedJsonButton)
                    contentItem: QGCLabel { text: protectedJsonButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: protectedJsonButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: protectedJsonButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: protectedJsonButton.activeFocus ? 3 : 1; radius: 3 }
                }
                Repeater {
                    model: root.protectedJsonVisible && root.protectedReportKnown ? root.protectedReport.record.observation.vehicles : []
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("机 %1：%2").arg(modelData.vehicleId).arg(root.modeExitText(modelData.observedOutcome.missionModeExitObservation))
                    }
                }
                Loader {
                    Layout.fillWidth: true; active: root.protectedJsonVisible && root.protectedReportKnown
                    Layout.preferredHeight: active ? ScreenTools.defaultFontPixelHeight * 10 : 0
                    visible: active
                    sourceComponent: Component {
                        Controls.ScrollView {
                            width: parent ? parent.width : 1; height: ScreenTools.defaultFontPixelHeight * 10; clip: true
                            contentWidth: availableWidth
                            Controls.TextArea {
                                readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap
                                color: qgcPal.text
                                text: root.protectedCaptured && root.protectedReportKnown ? root.protectedCaptured.jsonText : ""
                                Accessible.name: qsTr("完整受保护缓存报告 JSON，只读，不自动保存、上传或写剪贴板")
                                background: Rectangle { color: qgcPal.windowShade }
                                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedJsonButton)
                            }
                        }
                    }
                }
                QGCButton {
                    id: protectedClearButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("清除此区报告和选择（不删除记录或文件）"); enabled: root.protectedReady
                    Accessible.name: text
                    onClicked: root.invalidateProtected(true, qsTr("本区显示已清除，保留记录和原 Run 均未改变。"))
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedClearButton)
                    contentItem: QGCLabel { text: protectedClearButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: protectedClearButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: protectedClearButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: protectedClearButton.activeFocus ? 3 : 1; radius: 3 }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; visible: root.protectedFileCurrent; spacing: root.pad / 2
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: root.protectedFileCurrent ? (root.protectedPendingFile.action === "save"
                        ? qsTr("待保存独立报告：%1\n内容 hash：%2\n目标路径：%3\n仅路径已选择，尚未写入。确认后可能覆盖同名文件；不会改写另一份新报告。")
                            .arg(root.protectedPendingFile.reportId).arg(root.protectedPendingFile.contentHash).arg(root.protectedPendingFile.filePath)
                        : qsTr("待打开独立历史文件：%1\n尚未读取；明确确认后替换本区单份显示，不影响下方完整 Run 报告，不恢复任务或权限。")
                            .arg(root.protectedPendingFile.filePath)) : ""
                }
                Controls.CheckBox {
                    id: protectedFileConsent
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    checked: false; enabled: root.protectedReady && root.protectedFileCurrent
                    text: root.protectedFileCurrent && root.protectedPendingFile.action === "save"
                        ? qsTr("我核对了报告 ID、hash 和路径，接受可能覆盖同名文件")
                        : qsTr("我确认读取该独立历史文件，只读且不认证来源或恢复权限")
                    Accessible.name: text
                    onCheckedChanged: root.protectedConfirmationChanged(true, checked)
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedFileConsent)
                    contentItem: QGCLabel { text: protectedFileConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (protectedFileConsent.indicator ? protectedFileConsent.indicator.width : 0) + root.pad }
                    background: Rectangle { color: "transparent"; border.color: qgcPal.text; border.width: protectedFileConsent.activeFocus ? 2 : 0; radius: 3 }
                    Keys.onReturnPressed: event.accepted = true
                    Keys.onEnterPressed: event.accepted = true
                }
                QGCButton {
                    id: protectedFileCommitButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: root.protectedFileCurrent && root.protectedPendingFile.action === "save"
                        ? qsTr("确认保存上述本区报告到该路径") : qsTr("确认读取上述文件为本区独立历史")
                    enabled: root.protectedReady && root.protectedFileCurrent && protectedFileConsent.checked
                    Accessible.name: text
                    property var pressedIntent: null
                    onPressed: pressedIntent = root.protectedCommitIntent()
                    onCanceled: pressedIntent = null
                    onClicked: { var value = pressedIntent; pressedIntent = null; root.performProtectedFile(value) }
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedFileCommitButton)
                    contentItem: QGCLabel { text: protectedFileCommitButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: protectedFileCommitButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: protectedFileCommitButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: protectedFileCommitButton.activeFocus ? 3 : 1; radius: 3 }
                }
                QGCButton {
                    id: protectedCancelFileButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("取消本区待确认路径（不读写文件）"); enabled: root.protectedReady
                    Accessible.name: text
                    property var pressedIntent: null
                    onPressed: pressedIntent = root.protectedPendingFile
                    onCanceled: pressedIntent = null
                    onClicked: { var value = pressedIntent; pressedIntent = null; root.cancelProtectedPending(value) }
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(protectedCancelFileButton)
                    contentItem: QGCLabel { text: protectedCancelFileButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: protectedCancelFileButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: protectedCancelFileButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: protectedCancelFileButton.activeFocus ? 3 : 1; radius: 3 }
                }
            }
            QGCLabel {
                Layout.fillWidth: true; visible: !!root.protectedFileOutcome && root.protectedFileOutcome.owner === root.controller
                textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: qgcPal.warningText
                text: visible ? qsTr("最近一次独立保存事实（不证明当前上下文）：\n报告：%1\n路径：%2\n%3\n不自动重试；后续上下文变化不会撤销已写事实，也不恢复任何权限。")
                    .arg(root.protectedFileOutcome.reportId).arg(root.protectedFileOutcome.filePath)
                    .arg(!root.protectedFileOutcome.known ? qsTr("回执未知，文件可能已写入，请核对路径。")
                        : root.protectedFileOutcome.fileWritten === true ? qsTr("文件已写入。提交：%1；回执当前：%2。").arg(root.known(root.protectedFileOutcome.committed)).arg(root.known(root.protectedFileOutcome.current))
                        : root.protectedFileOutcome.fileWritten === null ? qsTr("文件是否写入未知，请核对路径。")
                        : qsTr("未确认写入成功；已尝试：%1；原因：%2。").arg(root.known(root.protectedFileOutcome.fileWriteAttempted)).arg(root.text(root.protectedFileOutcome.error))) : ""
            }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: qsTr("当前可选观察 Run：%1。捕获会替换本页报告，不停止或更新该 Run。").arg(root.run.prepared === true ? root.text(root.run.runId) : qsTr("暂无已准备的真实 Run 观察（不是失败结论）"))
        }
        QGCButton {
            id: captureButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("捕获当前所选 Run（替换本页冻结报告）"); enabled: root.canCapture
            Accessible.name: text
            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(captureButton)
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("capture")
            onCanceled: pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
            contentItem: QGCLabel { text: captureButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: captureButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: captureButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: captureButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCButton {
            id: loadButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("打开报告 JSON（替换本页报告，只读）")
            Accessible.name: text
            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(loadButton)
            enabled: root.ready && typeof root.controller.loadFleetExecutionReport === "function"
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("load")
            onCanceled: pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.openPicker(value, false) }
            contentItem: QGCLabel { text: loadButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: loadButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: loadButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: loadButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            visible: !root.reportKnown
            text: Object.keys(root.report).length ? qsTr("报告形状未知；不能把缺失字段解释为正常或成功，也不能从此视图导出。") : qsTr("尚无冻结报告。展开本页不会自动捕获、读取或写入文件。")
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.reportKnown; spacing: root.pad / 2
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: qsTr("来源：%1\n报告 ID：%2\n冻结时间（UTC）：%3\nRun / 启动：%4 / %5\n部署：%6\n本地观察任务：%7\n内容 hash：%8（完整性校验，不认证来源）")
                    .arg(root.state.origin === "imported" ? qsTr("外部导入，未认证只读历史") : root.state.origin === "captured" ? qsTr("本地捕获的冻结历史") : qsTr("未知"))
                    .arg(root.reportId).arg(root.text(root.report.capturedAtUtc)).arg(root.text(root.observation.runId)).arg(root.text(root.observation.startupId))
                    .arg(root.text(root.observation.deploymentId)).arg(root.text(root.observation.executionObservationTaskId)).arg(root.text(root.report.contentHash))
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("原组 %1 架；本地 MODE 曾开始入队 %2 架、已捕获提交 %3 架、未捕获提交 %4 架。\n捕获时来源未知 %5 架（可与下列线索重叠，不是成功率）。\n无终端线索 %6；终端项线索 %7；无限期终端保持线索 %8；终端后落地线索 %9。\n本地监视已退休：%10。原启动状态：%11；原因：%12。\n零提交不等于未起飞或整组失败；退休与任何线索都不证明任务完成、停飞或当前落地。")
                    .arg(root.summary.targetVehicleCount).arg(root.summary.modeQueueBeganVehicleCount).arg(root.summary.modeCommittedVehicleCount).arg(root.summary.uncommittedVehicleCount)
                    .arg(root.summary.sourceUnknownVehicleCount).arg(root.summary.noTerminalCandidateVehicleCount).arg(root.summary.terminalCandidateVehicleCount)
                    .arg(root.summary.indefiniteHoldCandidateVehicleCount).arg(root.summary.landedCandidateVehicleCount).arg(root.known(root.summary.observerRetired))
                    .arg(root.text(root.summary.startupTerminalState)).arg(root.text(root.summary.startupTerminalReason))
            }
            QGCButton {
                id: detailsButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: root.showVehicles ? qsTr("收起逐机历史观察") : qsTr("展开逐机历史观察（最多 32 架）")
                Accessible.name: text
                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(detailsButton)
                onClicked: root.showVehicles = !root.showVehicles
                contentItem: QGCLabel { text: detailsButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: detailsButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                background: Rectangle { color: detailsButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: detailsButton.activeFocus ? 3 : 1; radius: 3 }
            }
            Repeater {
                model: root.expanded && root.showVehicles && root.reportKnown ? root.summary.vehicles : []
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("成员 %1 / 组件 %2\n历史阶段：%3；原因：%4\nMODE 曾开始入队 / 已捕获提交：%5 / %6\n捕获时来源绑定 / 连续性未知：%7 / %8\n最后 current / reached 协议序号（0 基）：%9 / %10\n观察到 current / reached 项数：%11 / %12\n线索：%13\n窗口：%14")
                        .arg(modelData.vehicleId).arg(modelData.componentId).arg(root.text(modelData.phase)).arg(root.text(modelData.reason))
                        .arg(root.known(modelData.modeQueueBegan)).arg(root.known(modelData.modeQueueCommitted)).arg(root.known(modelData.sourceCurrent)).arg(root.known(modelData.continuityUnknown))
                        .arg(root.sequence(modelData.currentSequence)).arg(root.sequence(modelData.reachedSequence)).arg(modelData.observedCurrentItemCount).arg(modelData.observedReachedItemCount)
                        .arg(root.candidateText(modelData.outcomeCandidate)).arg(root.windowText(modelData.observationWindow))
                        + "\n" + root.modeExitText(modelData.missionModeExitObservation)
                }
            }
            QGCButton {
                id: saveButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("选择导出 JSON 路径（下一步确认后写入）")
                Accessible.name: text
                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(saveButton)
                enabled: root.ready && root.reportKnown && typeof root.controller.saveFleetExecutionReport === "function"
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("choose-save")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.openPicker(value, true) }
                contentItem: QGCLabel { text: saveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: saveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                background: Rectangle { color: saveButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: saveButton.activeFocus ? 3 : 1; radius: 3 }
            }
            QGCButton {
                id: clearButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("清除本页冻结报告（不删除文件，不改变 Run）")
                Accessible.name: text
                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(clearButton)
                enabled: root.ready && root.reportKnown && typeof root.controller.clearFleetExecutionReport === "function"
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("clear")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
                contentItem: QGCLabel { text: clearButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: clearButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                background: Rectangle { color: clearButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: clearButton.activeFocus ? 3 : 1; radius: 3 }
            }
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.saveCurrent; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: root.saveCurrent ? qsTr("待写报告：%1\n目标路径：%2\n确认后可能替换同名文件；选择路径本身尚未调用报告保存接口。")
                .arg(root.saveIntent.reportId).arg(root.saveIntent.filePath) : ""
        }
        Controls.CheckBox {
            id: writeConsent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.saveCurrent; enabled: root.ready; checked: false
            text: qsTr("我确认将上述冻结报告写到该路径，并接受可能覆盖同名文件")
            Accessible.name: text
            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(writeConsent)
            contentItem: QGCLabel { text: writeConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (writeConsent.indicator ? writeConsent.indicator.width : 0) + root.pad }
            background: Rectangle { color: "transparent"; border.color: qgcPal.text; border.width: writeConsent.activeFocus ? 2 : 0; radius: 3 }
        }
        QGCButton {
            id: writeButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.saveCurrent; enabled: root.ready && root.saveCurrent && writeConsent.checked
            text: qsTr("确认写入上述路径（不会导出另一份新报告）")
            Accessible.name: text
            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(writeButton)
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("save")
            onCanceled: pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
            contentItem: QGCLabel { text: writeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: writeButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: writeButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: writeButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCButton {
            id: cancelSaveButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.saveCurrent; enabled: !root.callBusy
            text: qsTr("取消待写路径（不写文件）")
            Accessible.name: text
            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(cancelSaveButton)
            onClicked: root.discardSave()
            contentItem: QGCLabel { text: cancelSaveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: cancelSaveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: cancelSaveButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: cancelSaveButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.callBusy || root.state.busy === true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("正在处理本地报告；禁止重复提交。")
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.message.length > 0; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: root.message; color: qgcPal.warningText
        }
        QGCLabel {
            Layout.fillWidth: true; visible: typeof root.state.error === "string" && root.state.error.length > 0; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: qsTr("原生报告原因：%1").arg(root.text(root.state.error)); color: qgcPal.warningText
        }
        }
    }

    Component {
        id: loadDialogComponent
        QGCFileDialog {
            id: loadView
            property var ownerPanel: null
            property var guard: null
            property var frozenIntent: null
            property bool activeIntent: true
            property bool protectedFile: false
            property var dialogGuard: ({alive: true})
            Component.onDestruction: dialogGuard.alive = false
            selectExisting: true; selectFolder: false; folder: ""
            title: qsTr("打开运行复盘报告 · 只读")
            nameFilters: [qsTr("运行复盘报告 (*.json)")]
            onAcceptedForLoad: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.fileChosen(loadView, frozenIntent, file, false)
            onRejected: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.fileRejected(loadView)
        }
    }
    Component {
        id: saveDialogComponent
        QGCFileDialog {
            id: saveView
            property var ownerPanel: null
            property var guard: null
            property var frozenIntent: null
            property bool activeIntent: true
            property bool protectedFile: false
            property var dialogGuard: ({alive: true})
            Component.onDestruction: dialogGuard.alive = false
            selectExisting: false; selectFolder: false; folder: ""
            title: qsTr("选择运行复盘报告路径 · 返回后仍需确认写入")
            nameFilters: [qsTr("运行复盘报告 (*.json)")]
            onAcceptedForSave: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.fileChosen(saveView, frozenIntent, file, true)
            onRejected: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.fileRejected(saveView)
        }
    }
    Component {
        id: protectedLoadDialogComponent
        QGCFileDialog {
            id: protectedLoadView
            property var ownerPanel: null
            property var guard: null
            property var frozenIntent: null
            property bool activeIntent: true
            property bool protectedFile: true
            property var dialogGuard: ({alive: true})
            Component.onDestruction: dialogGuard.alive = false
            selectExisting: true; selectFolder: false; folder: ""
            title: qsTr("选择独立保护记录报告 · 返回后仍需确认读取")
            nameFilters: [qsTr("独立保护记录报告 (*.json)")]
            onAcceptedForLoad: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.protectedFileChosen(protectedLoadView, frozenIntent, file)
            onRejected: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.protectedFileRejected(protectedLoadView)
        }
    }
    Component {
        id: protectedSaveDialogComponent
        QGCFileDialog {
            id: protectedSaveView
            property var ownerPanel: null
            property var guard: null
            property var frozenIntent: null
            property bool activeIntent: true
            property bool protectedFile: true
            property var dialogGuard: ({alive: true})
            Component.onDestruction: dialogGuard.alive = false
            selectExisting: false; selectFolder: false; folder: ""
            title: qsTr("选择独立保护记录报告保存路径 · 返回后确认覆盖风险")
            nameFilters: [qsTr("独立保护记录报告 (*.json)")]
            onAcceptedForSave: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.protectedFileChosen(protectedSaveView, frozenIntent, file)
            onRejected: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.protectedFileRejected(protectedSaveView)
        }
    }
}

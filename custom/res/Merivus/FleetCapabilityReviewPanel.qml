import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Value-only review UI. The actual Registry, account checks and one-use commit
// remain owned by the controller; an entered ID is not a physical identity.
FocusScope {
    id: root
    property var controller: null
    property var dialog: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, view: null, opening: null, activeCall: null, owned: null})
    property var ownedReceipt: ({})
    property var reviewOwner: null
    property bool callBusy: false
    property string localError: ""
    property string vehicleIdInput: ""
    property int operationIndex: 0
    property string capabilitiesInput: ""
    property string engineeringInput: ""
    property string maxSpeedInput: ""
    property string enduranceInput: ""
    property bool maxSpeedUnknown: true
    property bool enduranceUnknown: true
    property bool maxSpeedTouched: false
    property bool enduranceTouched: false
    property bool idTouched: false
    property bool capabilitiesTouched: false
    property bool reasonTouched: false
    property var lastResult: ({})
    property var committedFact: ({})
    property double clockMs: Date.now()
    property double reviewCapturedMs: -1
    property double reviewExpiresMs: -1
    property bool clockInvalid: false

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var reviewState: controller ? controller.fleetCapabilityReview || {} : ({})
    readonly property var workflow: reviewState.workflow || ({})
    readonly property var identity: reviewState.operatorIdentity || ({})
    readonly property string sourceToken: token(reviewState.sourceToken)
    readonly property string operation: operationIndex >= 0 && operationIndex < 5
        ? ["approve", "revoke", "approve-baseline", "revoke-baseline", "approve-performance"][operationIndex] : ""
    readonly property bool baselineOperation: operation === "approve-baseline" || operation === "revoke-baseline"
    readonly property bool performanceOperation: operation === "approve-performance"
    readonly property bool capabilityInputRequired: operation === "approve" || performanceOperation
    readonly property int selectedVehicleId: /^[1-9][0-9]{0,2}$/.test(vehicleIdInput) && Number(vehicleIdInput) <= 255 ? Number(vehicleIdInput) : 0
    readonly property var proposedCapabilities: capabilityInputRequired ? inputCapabilities(capabilitiesInput) : []
    readonly property var proposedPerformance: inputPerformance(maxSpeedInput, maxSpeedUnknown, enduranceInput, enduranceUnknown)
    readonly property bool reasonValid: boundedText(engineeringInput, 1024)
    readonly property string inputSignature: JSON.stringify([vehicleIdInput, operationIndex, capabilitiesInput, engineeringInput,
        maxSpeedInput, maxSpeedUnknown, enduranceInput, enduranceUnknown])
    readonly property bool inputValid: operation.length > 0 && selectedVehicleId > 0 && proposedCapabilities !== null && reasonValid
        && (!performanceOperation || proposedPerformance !== null)
    readonly property bool sourceReady: reviewState.schemaVersion === 1 && reviewState.sourceKnown === true
        && reviewState.uiWiringPresent === true && reviewState.runtimeValidated === false
        && sourceToken.length > 0 && workflowKnown(workflow)
    readonly property bool backendBusy: reviewState.busy !== false
    readonly property string ownToken: receiptToken(ownedReceipt)
    readonly property bool ownReviewCurrent: reviewOwner === controller && ownToken.length > 0
        && ownedReceipt.scopeToken === sourceToken && ownToken === receiptToken(workflow)
    readonly property bool previewReady: ownReviewCurrent && previewUsable(workflow.preview)
        && workflow.vehicleId === selectedVehicleId && workflow.operation === operation
        && workflow.engineeringReason === engineeringInput
        && JSON.stringify(workflow.capabilities) === JSON.stringify(proposedCapabilities)
        && (!performanceOperation || performanceToken(workflow.preview.proposedPerformance) === performanceToken(proposedPerformance))
    readonly property var comparisonRecord: previewReady ? workflow.preview.configurationComparison : ({})
    readonly property var configurationDiff: comparisonRecord.comparison || ({})
    readonly property var configurationChanges: Array.isArray(configurationDiff.changes) ? configurationDiff.changes : []
    readonly property double displayRemainingMs: !clockInvalid && reviewCapturedMs >= 0 && clockMs >= reviewCapturedMs
        ? Math.max(0, reviewExpiresMs - clockMs) : 0
    readonly property bool reviewPending: ["preparing", "review", "approving"].indexOf(workflow.state) >= 0
    readonly property bool canAuthenticate: !!controller && lifetime.alive && !lifetime.exhausted && !!dialog
        && sourceToken.length > 0 && !callBusy && !backendBusy && !ownToken.length && !reviewPending
        && typeof controller.authenticateFleetOperator === "function"
    readonly property bool canRequest: !!controller && lifetime.alive && !lifetime.exhausted && !!dialog
        && sourceReady && inputValid && identity.authenticated === true
        && ["maintenance", "administrator"].indexOf(identity.role) >= 0
        && !callBusy && !backendBusy && !ownToken.length && !reviewPending
        && (performanceOperation ? typeof controller.requestFleetPerformanceReview === "function"
            : typeof controller.requestFleetCapabilityReview === "function") && typeof controller.cancelFleetCapabilityReview === "function"
    readonly property bool canApprove: sourceReady && previewReady && ownReviewCurrent && workflow.state === "review"
        && integer(workflow.remainingReviewMs, 1, 30000) && displayRemainingMs > 0
        && !callBusy && !backendBusy && typeof controller.approveFleetCapabilityReview === "function"

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: entry.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function token(value) { return typeof value === "string" && value.length > 0 && value.length <= 32768 ? value : "" }
    function hex(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function map(value) { return !!value && typeof value === "object" && !Array.isArray(value) }
    function text(value) { return typeof value === "string" && value.length ? value : qsTr("未知") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function auditText(value) {
        return value === true ? qsTr("本地追加已确认（非持久化或防篡改证明）")
            : value === false ? qsTr("本次追加未确认（不能推断日志不存在）") : qsTr("未知（未返回严格布尔确认）")
    }
    function reportedList(value) {
        if (!Array.isArray(value)) return qsTr("未知")
        for (var i = 0; i < value.length; ++i) if (typeof value[i] !== "string") return qsTr("未知")
        return value.length ? value.join("\n") : qsTr("原记录为空（不等于独立核实无问题）")
    }
    function boundedText(value, maximum) {
        if (typeof value !== "string" || !value.length || value.length > maximum || value !== value.trim()
                || /[\u0000-\u001f\u007f-\u009f\u00ad\u061c\u180e\u200b-\u200f\u202a-\u202e\u2060-\u206f\ufeff\ufff9-\ufffb]/.test(value)) return false
        for (var i = 0; i < value.length; ++i) {
            var code = value.charCodeAt(i)
            if (code >= 0xd800 && code <= 0xdbff) { if (++i >= value.length || value.charCodeAt(i) < 0xdc00 || value.charCodeAt(i) > 0xdfff) return false }
            else if (code >= 0xdc00 && code <= 0xdfff) return false
        }
        return true
    }
    function inputCapabilities(value) {
        // Never parse user JSON or silently inherit capabilities from an old
        // approval. Normalized labels are shown before any review is requested.
        if (typeof value !== "string" || !value.length || value.length > 128 * 129) return null
        var parts = value.split(","), output = []
        if (parts.length < 1 || parts.length > 128) return null
        for (var i = 0; i < parts.length; ++i) {
            var item = parts[i].trim().toLowerCase()
            if (!boundedText(item, 128) || output.indexOf(item) >= 0) return null
            output.push(item)
        }
        output.sort()
        return output
    }
    function canonicalCapabilities(value, allowEmpty) {
        if (!Array.isArray(value) || value.length > 128 || (!allowEmpty && !value.length)) return false
        var previous = null
        for (var i = 0; i < value.length; ++i) {
            if (!boundedText(value[i], 128) || value[i] !== value[i].toLowerCase() || (previous !== null && value[i] <= previous)) return false
            previous = value[i]
        }
        return true
    }
    function performanceNumber(input, unknown, endurance) {
        if (unknown === true) return -1
        // Explicit decimal input only: empty/whitespace, exponents, aliases,
        // locale punctuation and a typed '-1' must never silently become zero.
        if (typeof input !== "string" || input.length > 64 || !/^(0|[1-9][0-9]*)(\.[0-9]+)?$/.test(input)) return null
        var value = Number(input)
        return isFinite(value) && value <= 9007199254740991 && (endurance ? value > 0 : value >= 0) ? value : null
    }
    function inputPerformance(speedText, speedUnknown, enduranceText, timeUnknown) {
        var speed = performanceNumber(speedText, speedUnknown, false), duration = performanceNumber(enduranceText, timeUnknown, true)
        return speed !== null && duration !== null && (speed >= 0 || duration > 0)
            ? {maxSpeedMps: speed, enduranceSeconds: duration} : null
    }
    function performanceToken(value) {
        if (!map(value) || Object.keys(value).length !== 2
                || typeof value.maxSpeedMps !== "number" || !isFinite(value.maxSpeedMps)
                || Math.abs(value.maxSpeedMps) > 9007199254740991 || (value.maxSpeedMps < 0 && value.maxSpeedMps !== -1)
                || typeof value.enduranceSeconds !== "number" || !isFinite(value.enduranceSeconds)
                || Math.abs(value.enduranceSeconds) > 9007199254740991 || (value.enduranceSeconds <= 0 && value.enduranceSeconds !== -1)
                || (value.maxSpeedMps === -1 && value.enduranceSeconds === -1)) return ""
        return JSON.stringify([value.maxSpeedMps, value.enduranceSeconds])
    }
    function contextKnown(value) {
        return map(value) && Object.keys(value).length === 6 && uuid(value.registryEpoch) && uuid(value.vehicleRecordEpoch)
            && integer(value.vehicleId, 1, 255) && typeof value.capabilityRevision === "string" && /^[1-9][0-9]{0,19}$/.test(value.capabilityRevision)
            && hex(value.identityFingerprint) && hex(value.configurationFingerprint)
    }
    function approvalKnown(value) {
        return map(value) && (Object.keys(value).length === 0 || (Object.keys(value).length === 14
            && value.schemaVersion === 1 && uuid(value.approvalId) && uuid(value.registryEpoch) && uuid(value.vehicleRecordEpoch)
            && canonicalCapabilities(value.capabilities, false) && value.source === "local-engineering-attestation"
            && ["current", "superseded", "revoked"].indexOf(value.state) >= 0
            && value.hardwareIdentityAuthenticated === false && value.flightCommandReleased === false))
    }
    function receiptToken(value) {
        if (!value || !uuid(value.workflowEpoch) || !uuid(value.reviewId)
                || typeof value.reviewGeneration !== "string" || !/^[1-9][0-9]{0,19}$/.test(value.reviewGeneration)
                || !hex(value.bindingHash) || !token(value.scopeToken)) return ""
        return JSON.stringify({workflowEpoch: value.workflowEpoch, reviewId: value.reviewId,
            reviewGeneration: value.reviewGeneration, bindingHash: value.bindingHash, scopeToken: value.scopeToken})
    }
    function workflowKnown(value) {
        if (!map(value) || Object.keys(value).length !== 30 || value.schemaVersion !== 1 || !uuid(value.workflowEpoch)
                || ["empty", "preparing", "review", "approving", "applied", "stale", "expired", "cancelled"].indexOf(value.state) < 0
                || !integer(value.remainingReviewMs, -1, 30000)) return false
        var flags = ["busy", "promptInProgress", "operationConfirmed", "authenticationConsumed", "mutationCommitted", "authorizationOwnerRetained"]
        for (var i = 0; i < flags.length; ++i) if (typeof value[flags[i]] !== "boolean") return false
        var denied = ["reviewUiIntegrated", "hardwareIdentityAuthenticated", "performanceCertified", "executionPermitted", "flightCommandReleased"]
        for (var j = 0; j < denied.length; ++j) if (value[denied[j]] !== false) return false
        return typeof value.reason === "string" && map(value.preview)
    }
    function comparisonValueKnown(value) {
        return map(value) && Object.keys(value).length === 9 && typeof value.type === "string" && value.type.length > 0 && value.type.length <= 256
            && typeof value.valid === "boolean" && typeof value.null === "boolean"
            && typeof value.representation === "string" && value.representation.length > 0 && value.representation.length <= 256
            && typeof value.display === "string" && value.display.length <= 512 && typeof value.displayTruncated === "boolean"
            && integer(value.byteCount, -1, 131072) && integer(value.elementCount, -1, 2048)
            && typeof value.typedFingerprint === "string" && (value.typedFingerprint === "" || hex(value.typedFingerprint))
    }
    function comparisonReportKnown(value) {
        if (!map(value) || Object.keys(value).length !== 13 || value.schemaVersion !== 1
                || ["unknown", "same", "different", "truncated", "invalid"].indexOf(value.comparisonState) < 0
                || typeof value.comparisonKnown !== "boolean" || typeof value.complete !== "boolean" || typeof value.truncated !== "boolean"
                || typeof value.reason !== "string" || typeof value.beforeKnown !== "boolean" || typeof value.afterKnown !== "boolean"
                || typeof value.beforeTypedFingerprint !== "string" || (value.beforeTypedFingerprint !== "" && !hex(value.beforeTypedFingerprint))
                || typeof value.afterTypedFingerprint !== "string" || (value.afterTypedFingerprint !== "" && !hex(value.afterTypedFingerprint))
                || !integer(value.changeCount, -1, 4096) || !integer(value.reportedChangeCount, 0, 128)
                || !Array.isArray(value.changes) || value.changes.length !== value.reportedChangeCount
                || (value.changeCount >= 0 && value.reportedChangeCount > value.changeCount)
                || (value.comparisonKnown && (!value.beforeKnown || !value.afterKnown
                    || !hex(value.beforeTypedFingerprint) || !hex(value.afterTypedFingerprint)))
                || (value.complete && value.truncated)) return false
        for (var i = 0; i < value.changes.length; ++i) {
            var row = value.changes[i]
            if (!map(row) || Object.keys(row).length !== 6 || typeof row.path !== "string" || row.path.length > 65536
                    || (row.path !== "" && row.path.charAt(0) !== "/")
                    || ["added", "removed", "type-changed", "null-changed", "value-changed"].indexOf(row.kind) < 0
                    || typeof row.beforePresent !== "boolean" || typeof row.afterPresent !== "boolean"
                    || !comparisonValueKnown(row.beforeValue) || !comparisonValueKnown(row.afterValue)
                    || (row.kind === "added" && (row.beforePresent || !row.afterPresent))
                    || (row.kind === "removed" && (!row.beforePresent || row.afterPresent))
                    || (["type-changed", "null-changed", "value-changed"].indexOf(row.kind) >= 0 && (!row.beforePresent || !row.afterPresent))) return false
        }
        return true
    }
    function configurationApprovalKnown(value, vehicleId) {
        if (!map(value) || Object.keys(value).length < 2 || Object.keys(value).length > 5
                || typeof value.source !== "string" || value.source.trim() !== "local_approval"
                || typeof value.fingerprint !== "string" || !hex(value.fingerprint.trim().toLowerCase())) return false
        var keys = Object.keys(value), allowed = ["systemId", "fingerprint", "source", "reason", "approvedAtUtc"]
        for (var i = 0; i < keys.length; ++i) if (allowed.indexOf(keys[i]) < 0) return false
        return (value.systemId === undefined || value.systemId === vehicleId)
            && (value.reason === undefined || typeof value.reason === "string")
            && (value.approvedAtUtc === undefined || typeof value.approvedAtUtc === "string")
    }
    function comparisonRecordKnown(value, expectedContext, vehicleId, allowEmptyContext) {
        var contextUsable = contextKnown(expectedContext)
            || (allowEmptyContext && map(expectedContext) && Object.keys(expectedContext).length === 0)
        if (!map(value) || Object.keys(value).length !== 16 || value.schemaVersion !== 1
                || !integer(value.systemId, 1, 255) || !contextUsable || !map(value.registryContext)
                || value.systemId !== vehicleId || JSON.stringify(value.registryContext) !== JSON.stringify(expectedContext)
                || !uuid(value.observationEpoch) || typeof value.comparisonRevision !== "string" || !/^[1-9][0-9]{0,19}$/.test(value.comparisonRevision)
                || ["local-approved-baseline", "previous-different-observed", "unknown"].indexOf(value.basis) < 0
                || typeof value.localBaselineKnown !== "boolean" || typeof value.previousObservationKnown !== "boolean" || typeof value.currentFullKnown !== "boolean"
                || !comparisonReportKnown(value.comparison) || value.hardwareIdentityAuthenticated !== false
                || value.performanceCertified !== false || value.flightCommandReleased !== false) return false
        var hashes = ["baselineConfigurationFingerprint", "previousConfigurationFingerprint", "currentConfigurationFingerprint"]
        for (var i = 0; i < hashes.length; ++i) if (typeof value[hashes[i]] !== "string" || (value[hashes[i]] !== "" && !hex(value[hashes[i]]))) return false
        return (value.basis !== "local-approved-baseline" || value.localBaselineKnown)
            && (value.basis !== "previous-different-observed" || value.previousObservationKnown)
            && (!contextKnown(expectedContext) || (expectedContext.vehicleId === vehicleId
                && value.currentConfigurationFingerprint === expectedContext.configurationFingerprint))
    }
    function previewUsable(value) {
        var revokeBaseline = map(value) && value.proposedOperation === "revoke-baseline"
        var performanceReview = map(value) && value.proposedOperation === "approve-performance"
        if (!map(value) || Object.keys(value).length !== (performanceReview ? 20 : 18) || value.schemaVersion !== (performanceReview ? 3 : 2)
                || value.source !== "actual-native-registry-record"
                || !approvalKnown(value.previousApproval)
                || !comparisonRecordKnown(value.configurationComparison, value.registryContext, workflow.vehicleId, revokeBaseline)
                || !map(value.previousReviewIdentity) || !map(value.configurationApproval) || !map(value.descriptor)
                || typeof value.descriptor.configurationJson !== "string" || typeof value.descriptor.configurationKnown !== "boolean"
                || value.descriptor.configurationKnown !== value.configurationComparison.currentFullKnown
                || value.descriptor.configurationRepresentation !== "qt-json-display-not-typed-export"
                || (value.descriptor.configurationKnown === true && !value.descriptor.configurationJson.length)
                || !canonicalCapabilities(value.observedCapabilities, true) || !canonicalCapabilities(value.declaredCapabilities, true)
                || !canonicalCapabilities(value.effectiveCapabilities, true)
                || ["approve", "revoke", "approve-baseline", "revoke-baseline", "approve-performance"].indexOf(value.proposedOperation) < 0
                || !canonicalCapabilities(value.proposedCapabilities, value.proposedOperation !== "approve" && !performanceReview)
                || (value.proposedOperation !== "approve" && !performanceReview && value.proposedCapabilities.length !== 0)
                || (value.proposedOperation === "revoke" && (value.proposedCapabilities.length || !Object.keys(value.previousApproval).length))
                || (value.proposedOperation === "approve-baseline" && (value.descriptor.configurationKnown !== true
                    || !hex(value.configurationComparison.comparison.afterTypedFingerprint)))
                || (revokeBaseline && (!configurationApprovalKnown(value.configurationApproval, workflow.vehicleId)
                    || value.descriptor.baselineSource !== "local_approval"
                    || value.descriptor.baselineConfigurationFingerprint !== value.configurationApproval.fingerprint.trim().toLowerCase()))
                || !boundedText(value.engineeringReason, 1024) || value.localDeclarationOnly !== true
                || value.hardwareIdentityAuthenticated !== false || value.performanceCertified !== false || value.flightCommandReleased !== false) return false
        if (performanceReview && (!performanceToken(value.proposedPerformance) || !contextKnown(value.proposedContext)
                || value.proposedContext.vehicleId !== workflow.vehicleId
                || value.proposedContext.registryEpoch !== value.registryContext.registryEpoch
                || value.proposedContext.vehicleRecordEpoch !== value.registryContext.vehicleRecordEpoch
                || value.proposedContext.configurationFingerprint !== value.registryContext.configurationFingerprint
                || value.proposedContext.capabilityRevision === value.registryContext.capabilityRevision
                || value.configurationComparison.localBaselineKnown !== true || value.configurationComparison.currentFullKnown !== true)) return false
        return value.descriptor.systemId === workflow.vehicleId && value.proposedOperation === workflow.operation
            && value.engineeringReason === workflow.engineeringReason
            && JSON.stringify(value.proposedCapabilities) === JSON.stringify(workflow.capabilities)
            && JSON.stringify(value.registryContext) === JSON.stringify(workflow.registryContext)
    }
    function fullJson(value) { try { return JSON.stringify(value, null, 2) } catch (error) { return qsTr("完整数据无法显示；不可据此审批。") } }
    function capabilityText(value) { return Array.isArray(value) ? (value.length ? value.join(", ") : qsTr("无")) : qsTr("未知") }
    function operationText(value) {
        return value === "approve" ? qsTr("工程采纳能力声明") : value === "revoke" ? qsTr("撤销原能力声明采纳")
            : value === "approve-baseline" ? qsTr("批准当前完整构型为本机参考基线")
            : value === "revoke-baseline" ? qsTr("撤销本机构型基线批准")
            : value === "approve-performance" ? qsTr("更新工程性能声明并采纳完整能力清单") : qsTr("未知操作")
    }
    function operationConsentText(value) {
        if (value === "approve-baseline") return qsTr("确认用预览的当前完整构型替换本机参考基线；旧能力采纳和任务依据将失效，需重新审阅。下一步使用不同账号独立审批，不写入机载、不放行飞行。")
        if (value === "revoke-baseline") return qsTr("确认撤销预览中的本机基线批准；即使当前构型未知也仅移除本地批准，不恢复硬件/参数，不取消既有飞行。下一步使用不同账号独立审批。")
        if (value === "approve-performance") return qsTr("确认一次替换预览的最大速度、满电续航和完整能力清单；未知保持未知。原构型基线不变，旧任务依据失效。下一步用不同账号审批本用途；不认证实测性能，不写机载或放行飞行。")
        return value === "approve" ? qsTr("确认以预览完整清单替换本地能力采纳，旧任务依据将失效；下一步使用不同账号独立审批，不更改基线或放行飞行")
            : qsTr("确认撤销预览中的原能力采纳，旧任务依据将失效；不恢复硬件/参数/基线，下一步使用不同账号独立审批")
    }
    function approvalButtonText(value) {
        return value === "approve-baseline" ? qsTr("3 · 独立账号审批并批准本机构型基线")
            : value === "revoke-baseline" ? qsTr("3 · 独立账号审批并撤销本机基线")
            : value === "approve-performance" ? qsTr("3 · 独立账号审批并更新工程性能声明")
            : value === "approve" ? qsTr("3 · 独立账号审批并采纳本地声明") : qsTr("3 · 独立账号审批并撤销原采纳")
    }
    function performance(value, unit) { return typeof value === "number" && isFinite(value) && value >= 0 ? String(value) + unit + qsTr("（声明值，非实测）") : qsTr("未知，未提供实测证明") }
    function baselineText(value, comparison) {
        var current = value.configurationFingerprint, baseline = value.baselineConfigurationFingerprint
        var report = !hex(baseline) ? qsTr("没有可核实的报告基线哈希。") : !hex(current) ? qsTr("当前报告哈希未知。")
            : current === baseline ? qsTr("报告哈希相同；不证明有类型字段或实物相同。") : qsTr("报告哈希不同；需工程核查。")
        var basis = comparison.basis === "local-approved-baseline" ? qsTr("字段比较基准：本机明确保存的本地批准完整构型快照；不是硬件认证。")
            : comparison.basis === "previous-different-observed" ? qsTr("字段比较基准：前次不同的完整上报观察；它不是批准基线。")
            : qsTr("字段比较基准未知：没有可用的完整本地批准快照或前次不同完整观察；不从哈希补造配置。")
        return report + "\n" + basis + "\n" + (baselineOperation
            ? qsTr("本次仅改变本机基线批准；不自动采纳能力，不认证改装原因、实物、性能或飞行安全。完整上报不代表机型所有必要字段齐全。")
            : qsTr("本次能力声明审批不批准、不更新构型基线，也不认证改装原因或飞行安全。"))
    }
    function comparisonSummary(record) {
        var value = record.comparison || {}, count = value.comparisonKnown === true && integer(value.changeCount, 0, 4096) ? String(value.changeCount) : qsTr("未知")
        var status = value.comparisonKnown !== true ? qsTr("字段比较未知，绝不是“没有变化”。")
            : value.comparisonState === "same" && value.complete === true ? qsTr("相对选定记录的有类型构型值相同；不证明实物未改装。")
            : qsTr("相对选定记录存在构型差异或展示限制；不能据此判断改装原因与安全性。")
        if (record.currentFullKnown !== true) status += "\n" + qsTr("当前完整配置缺失：可能仅有报告哈希或已经清除，不能判定字段相同。")
        if (value.truncated === true || value.complete !== true) status += "\n" + qsTr("报告不完整 / 未知：可能有未展示的差异、路径或值。最多展示128行，每值最多512个UTF-16单元；容器摘要不等于完整内容。不得把已展示项当全量。")
        return status + "\n" + qsTr("原生状态：%1 · 原因：%2\n检测差异数：%3 · 本页收到并可展开的行：%4\n前值可用：%5 · 后值可用：%6\n前有类型指纹：%7\n后有类型指纹：%8")
            .arg(text(value.comparisonState)).arg(text(value.reason)).arg(count).arg(value.reportedChangeCount)
            .arg(known(value.beforeKnown)).arg(known(value.afterKnown)).arg(text(value.beforeTypedFingerprint)).arg(text(value.afterTypedFingerprint))
    }
    function pointerText(value) {
        if (value === "") return qsTr("根路径（JSON Pointer 空字符串）")
        // Quote control/format/surrogate code units so unusual keys cannot
        // masquerade as additional display rows or directional labels.
        return JSON.stringify(value).replace(/[\u007f-\u009f\u00ad\u0600-\u0605\u061c\u06dd\u070f\u0890-\u0891\u08e2\u17b4-\u17b5\u180e\u200b-\u200f\u2028-\u202e\u2060-\u206f\ud800-\udfff\ufeff\ufff9-\ufffb]/g,
            function(character) { return "\\u" + ("0000" + character.charCodeAt(0).toString(16)).slice(-4) })
    }
    function changeKind(value) {
        return value === "added" ? qsTr("字段新增") : value === "removed" ? qsTr("字段移除") : value === "type-changed" ? qsTr("类型变化")
            : value === "null-changed" ? qsTr("null状态变化") : qsTr("值变化")
    }
    function comparedValue(present, value) {
        var state = present === false ? qsTr("Missing：字段不存在（不是 null，也不是空字符串）。")
            : value.valid !== true ? qsTr("字段存在，但原 QVariant 无效；不是 Missing。")
            : value.null === true ? qsTr("null：字段存在且为 null；不是 Missing。") : qsTr("字段存在且非 null。")
        return state + "\n" + qsTr("类型：%1 · valid：%2 · null：%3\n展示方式：%4\n值展示：\n%5\n字节数：%6 · 元素数：%7\n有类型指纹：%8%9")
            .arg(text(value.type)).arg(known(value.valid)).arg(known(value.null)).arg(text(value.representation))
            .arg(value.display.length ? value.display : qsTr("（空展示文本；不等于字段缺失）"))
            .arg(value.byteCount < 0 ? qsTr("不适用") : String(value.byteCount)).arg(value.elementCount < 0 ? qsTr("不适用") : String(value.elementCount))
            .arg(text(value.typedFingerprint)).arg(value.displayTruncated === true ? "\n" + qsTr("警告：此值仅为截断展示 / 容器摘要，内容不完整。") : "")
    }
    function capabilityDelta(value) {
        if (value.proposedOperation === "approve-baseline") return qsTr("拟以当前完整有类型构型替换本机参考基线；旧能力采纳将失效，须另行审阅。此处没有提交任何新能力标签。")
        if (value.proposedOperation === "revoke-baseline") return qsTr("拟撤销本机基线报告 %1；不要求当前构型完整，也不恢复硬件或参数。旧能力采纳及任务依据将失效。").arg(text((value.configurationApproval || {}).fingerprint))
        if (value.proposedOperation === "revoke") return qsTr("拟撤销原审批 %1；不自动恢复硬件、参数或构型基线。").arg(text((value.previousApproval || {}).approvalId))
        var added = [], removed = [], before = value.effectiveCapabilities, after = value.proposedCapabilities
        for (var a = 0; a < after.length; ++a) if (before.indexOf(after[a]) < 0) added.push(after[a])
        for (var b = 0; b < before.length; ++b) if (after.indexOf(before[b]) < 0) removed.push(before[b])
        return qsTr("相对当前有效本地采纳：新增 [%1]；不再采纳 [%2]。这是完整能力清单替换，不是追加。").arg(capabilityText(added)).arg(capabilityText(removed))
    }
    function cancelExact(owner, receipt) { if (owner && receipt) try { owner.cancelFleetCapabilityReview(receipt) } catch (error) {} }
    function clearChecks() { var guard = lifetime, actualView = dialog; if (guard.alive && actualView) actualView.clearConfirmations() }
    function invalidateReview(message) {
        var guard = lifetime, owned = guard.owned
        if (!guard.alive) return
        guard.owned = null
        if (guard.epoch >= 9007199254740991) guard.exhausted = true
        else guard.epoch += 1
        var epoch = guard.epoch
        if (owned) cancelExact(owned.owner, owned.receipt)
        if (!guard.alive || guard.epoch !== epoch) return
        ownedReceipt = ({})
        if (!guard.alive || guard.epoch !== epoch) return
        reviewOwner = null
        if (!guard.alive || guard.epoch !== epoch) return
        reviewCapturedMs = -1
        if (!guard.alive || guard.epoch !== epoch) return
        reviewExpiresMs = -1
        if (!guard.alive || guard.epoch !== epoch) return
        clearChecks()
        if (guard.alive && guard.epoch === epoch && message && dialog) localError = message
    }
    function pressSnapshot() { return {owner: controller, epoch: lifetime.epoch, source: sourceToken, input: inputSignature, receipt: ownToken, view: lifetime.view} }
    function pressCurrent(pressed) {
        return pressed && pressed.owner === controller && pressed.epoch === lifetime.epoch && pressed.source === sourceToken
            && pressed.input === inputSignature && pressed.view && pressed.view === lifetime.view && !!dialog && visible
    }
    function finishCall(guard, ticket) {
        if (!guard.alive || guard.activeCall !== ticket) return
        callBusy = false
        if (guard.activeCall === ticket) guard.activeCall = null
    }
    function requestReview(pressed, confirmed) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall) return
        if (!canRequest || !pressCurrent(pressed) || confirmed !== true) { invalidateReview(qsTr("输入、来源或确认已变化；请重新核对后申请。")); return }
        var owner = controller, source = sourceToken, input = inputSignature, epoch = guard.epoch, view = guard.view
        var id = selectedVehicleId, op = operation, caps = proposedCapabilities.slice(), reason = engineeringInput, started = Date.now(), ticket = {}, result = {}
        var numbers = op === "approve-performance" ? {maxSpeedMps: proposedPerformance.maxSpeedMps, enduranceSeconds: proposedPerformance.enduranceSeconds} : null
        // Original-owner cancellation and this value-only extractor remain
        // usable if a native callback destroys this QML object during the call.
        var extract = function(value) {
            if (!value || Object.keys(value).length !== 5) return null
            var keys = ["workflowEpoch", "reviewId", "reviewGeneration", "bindingHash", "scopeToken"]
            for (var k = 0; k < keys.length; ++k) if (typeof value[keys[k]] !== "string") return null
            if (!/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value.workflowEpoch)
                    || !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value.reviewId)
                    || !/^[1-9][0-9]{0,19}$/.test(value.reviewGeneration) || !/^[0-9a-f]{64}$/.test(value.bindingHash)
                    || !value.scopeToken.length || value.scopeToken.length > 32768) return null
            return {workflowEpoch: value.workflowEpoch, reviewId: value.reviewId, reviewGeneration: value.reviewGeneration, bindingHash: value.bindingHash, scopeToken: value.scopeToken}
        }
        var abandon = function(receipt) { if (owner && receipt) try { owner.cancelFleetCapabilityReview(receipt) } catch (error) {} }
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken || input !== inputSignature) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || input !== inputSignature) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            try { result = (op === "approve-performance"
                ? owner.requestFleetPerformanceReview(source, id, caps, numbers, reason, true)
                : owner.requestFleetCapabilityReview(source, id, op, caps, reason, true)) || {} } catch (error) { result = {reason: "capability-preview-call-unavailable"} }
            var receipt = extract(result.receipt)
            if (!guard.alive) { abandon(receipt); return }
            if (guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken || input !== inputSignature || !visible || !dialog) { abandon(receipt); return }
            if (result.ok !== true || result.state !== "review" || workflow.state !== "review" || !receipt || receipt.scopeToken !== source
                    || !receiptToken(receipt) || receiptToken(workflow) !== receiptToken(receipt)
                    || !integer(workflow.remainingReviewMs, 1, 30000) || !previewUsable(workflow.preview)
                    || workflow.vehicleId !== id || workflow.operation !== op || workflow.engineeringReason !== reason
                    || JSON.stringify(workflow.capabilities) !== JSON.stringify(caps)
                    || (op === "approve-performance" && performanceToken(workflow.preview.proposedPerformance) !== performanceToken(numbers))) {
                abandon(receipt)
                if (guard.alive && guard.epoch === epoch) localError = qsTr("未建立本页完整审阅：%1。请核对登记ID、输入和原生原因；没有自动重试。").arg(text(result.reason || result.state))
                return
            }
            guard.owned = {owner: owner, receipt: receipt}
            reviewOwner = owner
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            ownedReceipt = receipt
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            reviewCapturedMs = started
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            reviewExpiresMs = started + workflow.remainingReviewMs
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clockMs = Date.now()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clockInvalid = clockMs < started
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function approveReview(pressed, evidenceChecked, operationChecked) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall) return
        if (!canApprove || !pressCurrent(pressed) || pressed.receipt !== ownToken || evidenceChecked !== true || operationChecked !== true) {
            invalidateReview(qsTr("预览已变化、到期或两项确认不完整；没有重复提交。")); return
        }
        var owned = guard.owned, epoch = guard.epoch, view = guard.view, ticket = {}, result = {}
        if (!owned || owned.owner !== reviewOwner || receiptToken(owned.receipt) !== ownToken) return
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownReviewCurrent) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownReviewCurrent) return
            try { result = owned.owner.approveFleetCapabilityReview(owned.receipt, true) || {} } catch (error) { result = {reason: "capability-review-call-unavailable"} }
            if (!guard.alive) return
            // An actual mutation remains a historical fact after source changes,
            // revocation, exhaustion, post-call logging failure or ok=false.
            var fact = {ok: result.ok === true, reason: typeof result.reason === "string" ? result.reason : "capability-result-unverified",
                reviewId: owned.receipt.reviewId, workflowEpoch: owned.receipt.workflowEpoch,
                vehicleId: typeof result.vehicleId === "number" ? result.vehicleId : null,
                operation: typeof result.operation === "string" ? result.operation : "",
                mutationCommitted: typeof result.mutationCommitted === "boolean" ? result.mutationCommitted : null,
                authenticationConsumed: typeof result.authenticationConsumed === "boolean" ? result.authenticationConsumed : null,
                auditRecordConfirmed: typeof result.auditRecordConfirmed === "boolean" ? result.auditRecordConfirmed : null,
                auditRecordOutcome: typeof result.auditRecordOutcome === "string" ? result.auditRecordOutcome : "",
                archiveSummaryRecordConfirmed: typeof result.archiveSummaryRecordConfirmed === "boolean" ? result.archiveSummaryRecordConfirmed : null,
                archiveSummaryOutcome: typeof result.archiveSummaryOutcome === "string" ? result.archiveSummaryOutcome : "",
                approvalEvidence: ({}), commitReceipt: ({}), commitReceiptAvailable: false}
            if (result.commitReceipt && result.commitReceipt.mutationCommitted === true) fact.mutationCommitted = true
            try { if (map(result.approvalEvidence) && result.approvalEvidence.source === "local-account-review") fact.approvalEvidence = JSON.parse(JSON.stringify(result.approvalEvidence)) } catch (error) {}
            try { if (map(result.commitReceipt) && Object.keys(result.commitReceipt).length > 0) { fact.commitReceipt = JSON.parse(JSON.stringify(result.commitReceipt)); fact.commitReceiptAvailable = true } } catch (error) {}
            if (fact.mutationCommitted === true) committedFact = fact
            if (!guard.alive) return
            lastResult = fact
            if (!guard.alive) return
            if (guard.epoch === epoch && guard.view === view && result.ok !== true)
                localError = qsTr("审批返回未核实：%1。请先看已提交事实与回执；不自动重试、不推断回滚。").arg(text(result.reason))
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function authenticate(pressed) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall || !canAuthenticate || !pressCurrent(pressed)) return
        var owner = controller, epoch = guard.epoch, view = guard.view, ticket = {}, result = {}
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller) return
            try { result = owner.authenticateFleetOperator() || {} } catch (error) { result = {reason: "capability-operator-authentication-unavailable"} }
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller) return
            if (result.ok !== true) localError = qsTr("账号未核实：%1。认证不会自动采纳、撤销或申请预览。").arg(text(result.reason))
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function ensureFocusVisible(item) {
        if (!dialog || !item) return
        var point = item.mapToItem(dialog.scrollContent, 0, 0), scroll = dialog.scrollArea, bottom = point.y + item.height + pad
        if (point.y - pad < scroll.contentY) scroll.contentY = Math.max(0, point.y - pad)
        else if (bottom > scroll.contentY + scroll.height) scroll.contentY = Math.min(Math.max(0, scroll.contentHeight - scroll.height), bottom - scroll.height)
    }
    function finishView(actualView) {
        var guard = lifetime
        if (!guard.alive || dialog !== actualView) return
        guard.view = null
        invalidateReview("")
        if (!guard.alive) return
        dialog = null
        if (guard.alive && visible && !dialog) entryButton.forceActiveFocus()
    }
    function dismissView() {
        var guard = lifetime, actualView = dialog
        if (!guard.alive) return
        guard.view = null
        invalidateReview("")
        if (!guard.alive) return
        dialog = null
        if (actualView) try { actualView.close() } catch (error) {}
    }
    function openView() {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.opening || guard.activeCall || !controller || !visible || dialog) return
        var opening = {}, owner = controller, actualView = null
        guard.opening = opening
        try {
            localError = ""
            if (!guard.alive || guard.opening !== opening) return
            actualView = viewComponent.createObject(mainWindow, {ownerPanel: root, guard: guard})
            if (!guard.alive) { if (actualView) actualView.destroy(); return }
            if (owner !== controller || !visible || dialog || !actualView) { if (actualView) actualView.destroy(); return }
            guard.view = opening
            dialog = actualView
            if (!guard.alive || guard.view !== opening) { actualView.destroy(); return }
            actualView.open()
        } finally { if (guard.opening === opening) guard.opening = null }
    }
    function resultText() {
        var busyText = callBusy || backendBusy ? qsTr("原生核验或系统账号提示中；不重复提交，耗时计入原30秒期限。") : ""
        if (committedFact.mutationCommitted === true) return (busyText ? busyText + "\n" : "") + qsTr("历史事实：本页审阅 %1 曾提交本地登记变更。即使 ok=false、来源变化或后置审计失败也不等于回滚；不据此宣布采纳仍有效、撤销完整成功或允许飞行，请核查原生回执。").arg(text(committedFact.reviewId))
        if (busyText) return busyText
        if (reviewPending && !ownReviewCurrent) return qsTr("当前审阅属于另一个入口；本页不接管、不取消它。请在原入口处理。")
        if (ownReviewCurrent) return qsTr("仅本页拥有本次审阅；两项确认后仍需不同 SID 的安全员、总指挥或管理员独立审批。")
        return qsTr("尚无本页核实的变更事实。原生状态：%1；原因：%2。未知、取消、失败不代表先前变更回滚。").arg(text(workflow.state)).arg(text(workflow.reason))
    }
    onInputSignatureChanged: {
        var guard = lifetime, hadReviewOrCall = !!guard.owned || !!guard.activeCall
        invalidateReview(hadReviewOrCall ? qsTr("目标、操作、能力清单、性能输入或理由已修改；已取消本页原审阅，请重新生成。") : "")
        // Ordinary editing is not a failed submission. Avoid moving focus to
        // the error summary for every keystroke in a new numeric/text field.
        if (guard.alive && !hadReviewOrCall) localError = ""
    }
    onSourceTokenChanged: invalidateReview(qsTr("任务组、账号、策略或登记来源已变化；本页原确认已取消。"))
    onSourceReadyChanged: if (!sourceReady && lifetime.alive && lifetime.owned) invalidateReview(qsTr("来源不可核实；本页原审阅已取消。"))
    onOwnReviewCurrentChanged: if (!ownReviewCurrent && lifetime.alive && lifetime.owned) invalidateReview(qsTr("本页原收据已失效；不接管其他入口。"))
    onPreviewReadyChanged: if (!previewReady && lifetime.alive && lifetime.owned) invalidateReview(qsTr("冻结预览与本页输入不再一致；原审阅已取消。"))
    onWorkflowChanged: if (lifetime.alive && lifetime.owned && ["review", "approving"].indexOf(workflow.state) < 0) invalidateReview("")
    onControllerChanged: dismissView()
    onVisibleChanged: if (!visible) dismissView()
    onLocalErrorChanged: if (localError && dialog) dialog.focusError()
    Component.onDestruction: {
        var guard = lifetime, owned = guard.owned, actualView = dialog
        guard.owned = null; guard.alive = false; guard.view = null
        if (owned && owned.owner) try { owned.owner.cancelFleetCapabilityReview(owned.receipt) } catch (error) {}
        if (actualView) try { actualView.close() } catch (error) {}
    }
    Timer {
        interval: 250; repeat: true; running: !!root.dialog || root.callBusy
        onTriggered: {
            var guard = root.lifetime, now = Date.now()
            if (now < root.clockMs) {
                root.clockInvalid = true
                if (!guard.alive) return
                root.invalidateReview(qsTr("显示时钟回退；已取消本页审阅，请手动重新申请。"))
                if (!guard.alive) return
            }
            root.clockMs = now
            if (!guard.alive) return
            if (guard.owned && root.displayRemainingMs <= 0) root.invalidateReview(qsTr("本页显示期限已到；原生30秒期限仍为最终依据。"))
        }
    }

    ColumnLayout {
        id: entry
        width: root.width; spacing: root.pad / 2
        QGCButton {
            id: entryButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("单机工程审阅 · 能力 / 性能声明 / 构型基线")
            Accessible.name: text; enabled: !!root.controller && !root.callBusy && !root.lifetime.exhausted
            onClicked: root.openView()
            contentItem: QGCLabel { text: entryButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: entryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: entryButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: entryButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("分别审批本地能力、性能声明或构型基线；不实测性能、不认证实物、不写机载、不发送飞行命令。") }
        QGCLabel { Layout.fillWidth: true; visible: root.committedFact.mutationCommitted === true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.resultText(); color: qgcPal.warningText }
        QGCLabel { Layout.fillWidth: true; visible: !root.dialog && !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.localError; color: qgcPal.warningText }
    }

    Component {
        id: viewComponent
        Controls.Dialog {
            id: view
            property var ownerPanel: null
            property var guard: null
            property var localViewLife: ({alive: true})
            property bool showPreview: false
            property bool showConfiguration: false
            property bool showDifferences: false
            property bool showCommit: false
            property alias scrollArea: scroll
            property alias scrollContent: form
            parent: Controls.Overlay.overlay
            anchors.centerIn: parent
            modal: true; focus: true; closePolicy: Controls.Popup.NoAutoClose
            standardButtons: Controls.Dialog.NoButton
            padding: root.pad
            width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 66, mainWindow.width - root.pad * 4))
            height: Math.max(1, Math.min(form.implicitHeight + heading.implicitHeight + root.pad * 4, mainWindow.height - root.pad * 4))
            background: Rectangle { color: qgcPal.window; border.color: qgcPal.text; border.width: 1; radius: 3 }
            header: QGCLabel { id: heading; padding: root.pad; text: qsTr("单机工程审查 · 能力、性能声明与构型基线分用途批准"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true }
            function clearConfirmations() {
                var life = guard, ownLife = localViewLife
                if (!life || !life.alive || !ownLife.alive) return
                previewConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                evidenceConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                operationConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                requestButton.pressScope = ({})
                if (!life.alive || !ownLife.alive) return
                approveButton.pressScope = ({})
            }
            function focusError() {
                var life = guard, ownLife = localViewLife
                if (!life || !life.alive || !ownLife.alive || !errorSummary.visible) return
                errorSummary.forceActiveFocus()
                if (life.alive && ownLife.alive && ownerPanel) ownerPanel.ensureFocusVisible(errorSummary)
            }
            onOpened: closeButton.forceActiveFocus()
            onAboutToHide: { var life = guard; if (life && life.alive && ownerPanel) ownerPanel.finishView(view) }
            onClosed: { var life = guard, actualView = view; localViewLife.alive = false; if (life && life.alive && ownerPanel) ownerPanel.finishView(actualView); try { actualView.destroy() } catch (error) {} }
            Component.onDestruction: { var life = guard; localViewLife.alive = false; if (life && life.alive && ownerPanel) ownerPanel.finishView(view) }
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
                        text: qsTr("取消 / 关闭本页（不回滚已提交的登记变更）"); Accessible.name: text
                        onClicked: view.close()
                        Keys.onReturnPressed: { view.close(); event.accepted = true }
                        Keys.onEnterPressed: { view.close(); event.accepted = true }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(closeButton)
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                        text: qsTr("能力采纳只替换能力清单；性能声明在已有完整匹配基线下，一次更新两项工程数值和完整能力清单；基线批准只保存当前完整上报构型作为本机参考。三类审批用途独立，不可互用。数值或基线变更使旧任务依据失效；撤销不恢复硬件或参数，不取消既有飞行。账号批准不证明实物、改装或实测性能；本流程不上传任务、不解锁、不起飞，仍须工程师验证放行。")
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("当前账号：%1\n角色：%2\n原生状态：%3").arg(root.text(root.identity.actorId)).arg(root.text(root.identity.role)).arg(root.text(root.workflow.state)) }
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
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("认证本地操作员（维修员 / 管理员方可申请）"); Accessible.name: text; enabled: root.canAuthenticate
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, ownLife = view.localViewLife, pressed = pressScope; pressScope = ({}); if (life && life.alive && ownLife.alive) root.authenticate(pressed) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(authenticateButton)
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("1 · 输入本地登记条目与审查意图（修改任一项会立即取消本页原审阅）") ; font.bold: true }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("登记机号（必填，1–255；不是飞机数量、物理身份或硬件UID）") }
                    Controls.TextField {
                        id: vehicleField
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: root.vehicleIdInput; maximumLength: 3; selectByMouse: true; enabled: !root.callBusy && !root.backendBusy
                        inputMethodHints: Qt.ImhDigitsOnly; Accessible.name: qsTr("本地登记机号，1到255，必填")
                        onTextChanged: root.vehicleIdInput = text
                        onEditingFinished: root.idTouched = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(vehicleField)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.idTouched && root.selectedVehicleId === 0; color: qgcPal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("请输入无前导零的1–255单个登记机号；是否存在由原生登记表核查。") }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("操作（五选一；能力、性能声明与基线审批不可互用）") }
                    Controls.ComboBox {
                        id: operationField
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        model: [qsTr("能力 · 工程采纳声明（完整替换）"), qsTr("能力 · 撤销原声明采纳"),
                            qsTr("基线 · 批准当前完整构型"), qsTr("基线 · 撤销本机构型批准"), qsTr("性能 · 更新工程数值及完整能力清单")]
                        currentIndex: root.operationIndex; enabled: !root.callBusy && !root.backendBusy
                        Accessible.name: qsTr("工程操作类型，能力、性能声明与构型基线分用途批准")
                        onActivated: root.operationIndex = currentIndex
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(operationField)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.capabilityInputRequired; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("拟采纳完整能力标签（必填；英文逗号分隔，最多128个；将转小写并排序，重复/空标签拒绝。不自动继承原清单。）") }
                    Controls.TextField {
                        id: capabilitiesField
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        visible: root.capabilityInputRequired; enabled: !root.callBusy && !root.backendBusy
                        text: root.capabilitiesInput; maximumLength: 128 * 129; selectByMouse: true; Accessible.name: qsTr("拟采纳完整能力清单，英文逗号分隔，必填")
                        onTextChanged: root.capabilitiesInput = text
                        onEditingFinished: root.capabilitiesTouched = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(capabilitiesField)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.capabilityInputRequired && root.capabilitiesTouched && root.proposedCapabilities === null; color: qgcPal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("每项需1–128字符，无空项、重复、控制字符；请保留完整清单，不输入JSON。") }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.capabilityInputRequired ? qsTr("将提交的规范完整清单：%1").arg(root.proposedCapabilities === null ? qsTr("输入尚不完整") : root.capabilityText(root.proposedCapabilities))
                        : root.baselineOperation ? qsTr("基线操作不接受能力清单。批准需当前完整构型；撤销需实际已有的本机批准记录，当前构型可以未知。请先申请原生预览。")
                        : qsTr("撤销不接受新能力清单；待原生预览显示将撤销的原审批与完整能力。") }
                    ColumnLayout {
                        Layout.fillWidth: true; visible: root.performanceOperation; spacing: root.pad
                        QGCLabel {
                            Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                            text: qsTr("工程性能声明（非实测认证）· 必须已有当前完整且匹配的本机构型基线。两项都明确提交，至少一项已知；不自动继承旧值。")
                        }
                        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("最大速度（m/s，非 km/h；已知值允许0）") }
                        Controls.CheckBox {
                            id: speedUnknownField
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            text: qsTr("最大速度未知（提交−1，不以空白或0代替未知）")
                            checked: root.maxSpeedUnknown; enabled: !root.callBusy && !root.backendBusy; Accessible.name: text
                            contentItem: QGCLabel { text: speedUnknownField.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (speedUnknownField.indicator ? speedUnknownField.indicator.width : 0) + root.pad }
                            onToggled: root.maxSpeedUnknown = checked
                            onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(speedUnknownField)
                        }
                        Controls.TextField {
                            id: speedField
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            text: root.maxSpeedInput; maximumLength: 64; selectByMouse: true
                            enabled: !root.maxSpeedUnknown && !root.callBusy && !root.backendBusy
                            inputMethodHints: Qt.ImhFormattedNumbersOnly; Accessible.name: qsTr("声明最大速度，单位米每秒；取消未知选择后必填，允许0")
                            onTextChanged: root.maxSpeedInput = text
                            onEditingFinished: root.maxSpeedTouched = true
                            onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(speedField)
                        }
                        QGCLabel { Layout.fillWidth: true; visible: !root.maxSpeedUnknown && root.maxSpeedTouched && root.performanceNumber(root.maxSpeedInput, false, false) === null; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("最大速度请输入0或正的十进制数，不含空格、单位、逗号或指数；上限2^53−1。不知道时请勾选“最大速度未知”。") }
                        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("满电续航（秒；已知值必须大于0，不是当前剩余时间）") }
                        Controls.CheckBox {
                            id: enduranceUnknownField
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            text: qsTr("满电续航未知（提交−1，不填默认续航）")
                            checked: root.enduranceUnknown; enabled: !root.callBusy && !root.backendBusy; Accessible.name: text
                            contentItem: QGCLabel { text: enduranceUnknownField.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (enduranceUnknownField.indicator ? enduranceUnknownField.indicator.width : 0) + root.pad }
                            onToggled: root.enduranceUnknown = checked
                            onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(enduranceUnknownField)
                        }
                        Controls.TextField {
                            id: enduranceField
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            text: root.enduranceInput; maximumLength: 64; selectByMouse: true
                            enabled: !root.enduranceUnknown && !root.callBusy && !root.backendBusy
                            inputMethodHints: Qt.ImhFormattedNumbersOnly; Accessible.name: qsTr("声明满电续航，单位秒；取消未知选择后必填，必须大于0")
                            onTextChanged: root.enduranceInput = text
                            onEditingFinished: root.enduranceTouched = true
                            onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(enduranceField)
                        }
                        QGCLabel { Layout.fillWidth: true; visible: !root.enduranceUnknown && root.enduranceTouched && root.performanceNumber(root.enduranceInput, false, true) === null; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("满电续航请输入正的十进制秒数，不含空格、单位、逗号或指数；上限2^53−1。不知道时请勾选“满电续航未知”。") }
                        QGCLabel { Layout.fillWidth: true; visible: root.maxSpeedUnknown && root.enduranceUnknown; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("至少明确一项工程数值才能申请预览；两项未知不会生成默认性能。仅速度已知也不能通过严格能量预算。") }
                        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.proposedPerformance ? qsTr("将提交的数值：最大速度 %1；满电续航 %2。请在冻结预览中再次核对。").arg(root.performance(root.proposedPerformance.maxSpeedMps, " m/s")).arg(root.performance(root.proposedPerformance.enduranceSeconds, " s")) : qsTr("数值输入尚不完整；不会用旧值或空白补齐。") }
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; text: qsTr("工程理由（必填，1–1024字符，无首尾空白/控制字符）") }
                    Controls.TextField {
                        id: reasonField
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: root.engineeringInput; maximumLength: 1024; selectByMouse: true; enabled: !root.callBusy && !root.backendBusy
                        Accessible.name: qsTr("工程审查理由，必填")
                        onTextChanged: root.engineeringInput = text
                        onEditingFinished: root.reasonTouched = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(reasonField)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.reasonTouched && !root.reasonValid; color: qgcPal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("请填写具体工程依据或撤销原因；不允许空白、首尾空白、控制/隐藏方向字符。") }
                    Controls.CheckBox {
                        id: previewConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("仅冻结该本地条目的构型、原审批与本次意图生成预览；此步不修改登记表")
                        checked: false; enabled: root.canRequest; Accessible.name: text
                        contentItem: QGCLabel { text: previewConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (previewConsent.indicator ? previewConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(previewConsent)
                    }
                    QGCButton {
                        id: requestButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("生成本机冻结预览（不采纳、不撤销）"); Accessible.name: text; enabled: root.canRequest && previewConsent.checked
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, ownLife = view.localViewLife, pressed = pressScope, confirmed = previewConsent.checked; pressScope = ({}); if (life && life.alive && ownLife.alive) root.requestReview(pressed, confirmed) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(requestButton)
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.resultText(); color: qgcPal.warningText }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.ownReviewCurrent; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("2 · 本页冻结审阅 %1\n登记机号 %2 · %3\n剩余显示时间 %4 s（原生期限不续期）\n绑定 SHA-256：%5")
                            .arg(root.text(root.ownedReceipt.reviewId)).arg(root.workflow.vehicleId).arg(root.operationText(root.workflow.operation))
                            .arg(Math.floor(root.displayRemainingMs / 1000)).arg(root.text(root.ownedReceipt.bindingHash))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.previewReady; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("登记记录 epoch：%1\n能力 revision：%2\n记录内容指纹（非硬件认证）：%3\n当前构型报告指纹：%4\n基线报告指纹：%5\n基线报告来源：%6\n报告哈希状态（非实物认证）：%7\n%8")
                            .arg(root.text((root.workflow.registryContext || {}).vehicleRecordEpoch)).arg(root.text((root.workflow.registryContext || {}).capabilityRevision))
                            .arg(root.text((root.workflow.registryContext || {}).identityFingerprint)).arg(root.text((root.workflow.registryContext || {}).configurationFingerprint))
                            .arg(root.text(((root.workflow.preview || {}).descriptor || {}).baselineConfigurationFingerprint))
                            .arg(root.text(((root.workflow.preview || {}).descriptor || {}).baselineSource)).arg(root.text(((root.workflow.preview || {}).descriptor || {}).configurationState))
                            .arg(root.baselineText(((root.workflow.preview || {}).descriptor || {}), root.comparisonRecord))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.previewReady && root.baselineOperation
                        textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("本次基线操作：%1\n原生观察 epoch：%2\n观察 revision：%3\n当前完整构型：%4 · 本机完整基线快照：%5\n原批准报告哈希：%6\n原批准来源：%7\n原批准理由：%8\n原批准时间：%9\n本次工程理由：%10\n缺失的历史理由或时间保持未知；不补造。仅撤销基线可在能力上下文未知时继续，不代表未知配置可建立新基线。基线批准仅在本进程保存完整有类型参考，重启或导入后不能沿用为完整基线。")
                            .arg(root.operationText(root.workflow.operation)).arg(root.text(root.comparisonRecord.observationEpoch))
                            .arg(root.text(root.comparisonRecord.comparisonRevision)).arg(root.known(root.comparisonRecord.currentFullKnown))
                            .arg(root.known(root.comparisonRecord.localBaselineKnown))
                            .arg(root.text(((root.workflow.preview || {}).configurationApproval || {}).fingerprint))
                            .arg(root.text(((root.workflow.preview || {}).configurationApproval || {}).source))
                            .arg(root.text(((root.workflow.preview || {}).configurationApproval || {}).reason))
                            .arg(root.text(((root.workflow.preview || {}).configurationApproval || {}).approvedAtUtc))
                            .arg(root.text((root.workflow.preview || {}).engineeringReason))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.previewReady; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("本地声明发现状态：%1\n记录的构型证据来源：\n%2\n记录的发现错误：\n%3\n这些是本条目声明/发现过程的记录；未知或空记录不证明实物完整、无改装或已通过检查。")
                            .arg(root.text(((root.workflow.preview || {}).descriptor || {}).discoveryState))
                            .arg(root.reportedList(((root.workflow.preview || {}).descriptor || {}).configurationEvidenceSources))
                            .arg(root.reportedList(((root.workflow.preview || {}).descriptor || {}).discoveryErrors))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.previewReady; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("报告机型：%1 · 飞控类型：%2\n报告固件：%3\n最大速度：%4\n续航：%5\nUID及当前构型展示见下方只读预览；存在UID不代表已认证。")
                            .arg(root.text(((root.workflow.preview || {}).descriptor || {}).vehicleType)).arg(root.text(((root.workflow.preview || {}).descriptor || {}).autopilotType))
                            .arg(root.text(((root.workflow.preview || {}).descriptor || {}).firmwareVersion))
                            .arg(root.performance(((root.workflow.preview || {}).descriptor || {}).maxSpeedMps, " m/s"))
                            .arg(root.performance(((root.workflow.preview || {}).descriptor || {}).enduranceSeconds, " s"))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.previewReady && root.performanceOperation
                        textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("拟一次提交的工程性能：\n最大速度：%1\n满电续航：%2\n拟能力 revision：%3\n拟记录指纹：%4\n与上方原值对照：这是两项数值及完整能力清单的整份替换。原构型基线不变，旧任务依据失效；不证明实际飞机达到这些数值。")
                            .arg(root.performance(((root.workflow.preview || {}).proposedPerformance || {}).maxSpeedMps, " m/s"))
                            .arg(root.performance(((root.workflow.preview || {}).proposedPerformance || {}).enduranceSeconds, " s"))
                            .arg(root.text(((root.workflow.preview || {}).proposedContext || {}).capabilityRevision))
                            .arg(root.text(((root.workflow.preview || {}).proposedContext || {}).identityFingerprint))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.previewReady; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("报告能力：%1\n外部声明能力（未经本流程采纳）：%2\n当前有效本地工程采纳：%3\n本次拟操作：%4\n拟采纳完整清单：%5\n%6\n本次工程理由：%7\n原审批ID：%8 · 原状态：%9\n原审批工程理由：%10\n原审批时间：%11")
                            .arg(root.capabilityText((root.workflow.preview || {}).observedCapabilities)).arg(root.capabilityText((root.workflow.preview || {}).declaredCapabilities))
                            .arg(root.capabilityText((root.workflow.preview || {}).effectiveCapabilities)).arg(root.operationText(root.workflow.operation))
                            .arg(root.capabilityText((root.workflow.preview || {}).proposedCapabilities)).arg(root.previewReady ? root.capabilityDelta(root.workflow.preview) : "")
                            .arg(root.text((root.workflow.preview || {}).engineeringReason)).arg(root.text(((root.workflow.preview || {}).previousApproval || {}).approvalId))
                            .arg(root.text(((root.workflow.preview || {}).previousApproval || {}).state)).arg(root.text(((root.workflow.preview || {}).previousApproval || {}).reason))
                            .arg(root.text(((root.workflow.preview || {}).previousApproval || {}).approvedAtUtc))
                    }
                    QGCButton {
                        id: configurationDetailsButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; visible: root.previewReady
                        text: view.showConfiguration ? qsTr("折叠冻结构型的只读展示") : qsTr("展开冻结构型的完整只读展示（不截断，非类型化导出）")
                        Accessible.name: text; onClicked: view.showConfiguration = !view.showConfiguration
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(configurationDetailsButton)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.previewReady; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("当前构型文本仅为Qt JSON只读展示，不能还原所有原始Qt类型；不是导入文件、类型化备份或实物证明。字段比较以原生保留的有类型记录为准，比较基准与限制见下方，不从JSON反推。") }
                    Loader {
                        Layout.fillWidth: true; active: view.showConfiguration && root.previewReady
                        sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText; text: root.workflow.preview.descriptor.configurationKnown === true ? root.workflow.preview.descriptor.configurationJson : qsTr("当前构型内容未知；不可据此推断未改装或配置完整。"); color: qgcPal.text; Accessible.name: qsTr("冻结构型Qt JSON完整只读展示，非类型化导出"); background: Rectangle { color: qgcPal.windowShade } } }
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                    }
                    QGCButton {
                        id: differencesButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; visible: root.previewReady
                        text: view.showDifferences ? qsTr("折叠已报告的逐字段构型差异") : qsTr("展开已报告的逐字段差异（纯文本，可能不完整）")
                        Accessible.name: text; onClicked: view.showDifferences = !view.showDifferences
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(differencesButton)
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.previewReady; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        color: qgcPal.warningText
                        text: qsTr("构型比较 · 观察 epoch：%1 · 比较 revision：%2\n本地完整批准快照可用：%3\n前次不同完整观察可用：%4\n当前完整配置可用：%5\n%6")
                            .arg(root.text(root.comparisonRecord.observationEpoch)).arg(root.text(root.comparisonRecord.comparisonRevision))
                            .arg(root.known(root.comparisonRecord.localBaselineKnown)).arg(root.known(root.comparisonRecord.previousObservationKnown))
                            .arg(root.known(root.comparisonRecord.currentFullKnown)).arg(root.comparisonSummary(root.comparisonRecord))
                    }
                    Loader {
                        Layout.fillWidth: true; active: view.showDifferences && root.previewReady
                        sourceComponent: Component {
                            ColumnLayout {
                                width: parent ? parent.width : 1; spacing: root.pad
                                QGCLabel { Layout.fillWidth: true; visible: root.configurationChanges.length === 0; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.configurationDiff.comparisonKnown === true && root.configurationDiff.complete === true ? qsTr("原生完整比较未报告差异；仅相对所选本地记录，不证明实物未改变。") : qsTr("未提供可展示差异行；比较未知或不完整，不能称为没有变化。") }
                                Repeater {
                                    model: root.configurationChanges
                                    ColumnLayout {
                                        id: differenceRow
                                        Layout.fillWidth: true; spacing: root.pad / 2
                                        readonly property var change: modelData
                                        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("%1 · %2\n路径（JSON Pointer）：%3").arg(index + 1).arg(root.changeKind(differenceRow.change.kind)).arg(root.pointerText(differenceRow.change.path)) }
                                        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: differenceRow.change.beforeValue.displayTruncated === true ? qgcPal.warningText : qgcPal.text; text: qsTr("比较前：\n%1").arg(root.comparedValue(differenceRow.change.beforePresent, differenceRow.change.beforeValue)) }
                                        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: differenceRow.change.afterValue.displayTruncated === true ? qgcPal.warningText : qgcPal.text; text: qsTr("比较后：\n%1").arg(root.comparedValue(differenceRow.change.afterPresent, differenceRow.change.afterValue)) }
                                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: qgcPal.text; opacity: 0.35 }
                                    }
                                }
                            }
                        }
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                    }
                    QGCButton {
                        id: previewDetailsButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; visible: root.previewReady
                        text: view.showPreview ? qsTr("折叠完整冻结预览DTO") : qsTr("展开完整预览DTO（原生差异可能受限，见警告）")
                        Accessible.name: text; onClicked: view.showPreview = !view.showPreview
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(previewDetailsButton)
                    }
                    Loader {
                        Layout.fillWidth: true; active: view.showPreview && root.previewReady
                        sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText; text: root.fullJson(root.workflow.preview); color: qgcPal.text; Accessible.name: qsTr("完整冻结工程预览，只读且不截断"); background: Rectangle { color: qgcPal.windowShade } } }
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                    }
                    Controls.CheckBox {
                        id: evidenceConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: root.performanceOperation
                            ? qsTr("已核对原值、拟最大速度与满电续航（含未知）、完整能力清单、匹配基线和工程理由；这是声明采纳，不是实测性能或硬件认证。")
                            : root.baselineOperation
                            ? qsTr("已核对原基线批准、当前完整性、原类型指纹及字段差异的未知/截断限制；确认这是本机参考记录，不是实物/性能认证。")
                            : qsTr("已核对登记记录、构型比较来源及未知/截断限制、原审批、能力增删与工程理由；不把声明、UID或本地差异当成实物/性能认证，不因此批准基线")
                        checked: false; enabled: root.canApprove; Accessible.name: text
                        contentItem: QGCLabel { text: evidenceConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (evidenceConsent.indicator ? evidenceConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(evidenceConsent)
                    }
                    Controls.CheckBox {
                        id: operationConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: root.operationConsentText(root.operation)
                        checked: false; enabled: root.canApprove; Accessible.name: text
                        contentItem: QGCLabel { text: operationConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (operationConsent.indicator ? operationConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(operationConsent)
                    }
                    QGCButton {
                        id: approveButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: root.approvalButtonText(root.operation)
                        Accessible.name: text; enabled: root.canApprove && evidenceConsent.checked && operationConsent.checked
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, ownLife = view.localViewLife, pressed = pressScope, evidence = evidenceConsent.checked, confirmed = operationConsent.checked; pressScope = ({}); if (life && life.alive && ownLife.alive) root.approveReview(pressed, evidence, confirmed) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(approveButton)
                        contentItem: QGCLabel { text: approveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: approveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: approveButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.warningText; border.width: approveButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: !!root.lastResult.reviewId; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("本页上次审批返回（非当前有效性/性能认证）：\n审阅 %1 · 机号 %2 · %3\nok：%4 · 账号审批已消费：%5 · 本地变更已提交：%6\n原因：%7\n原生申请账号：%8 · 角色：%9\n原生审批账号：%10 · 角色：%11\n审计追加：%12\n审计返回原因：%13\n审计追加状态与变更提交相互独立。账号记录不是硬件签名或性能认证。已提交为真不等于本次完整成功，亦不会因 ok=false 或取消变成未提交。")
                            .arg(root.text(root.lastResult.reviewId)).arg(root.lastResult.vehicleId).arg(root.operationText(root.lastResult.operation))
                            .arg(root.known(root.lastResult.ok)).arg(root.known(root.lastResult.authenticationConsumed)).arg(root.known(root.lastResult.mutationCommitted)).arg(root.text(root.lastResult.reason))
                            .arg(root.text((root.lastResult.approvalEvidence || {}).applicantActorId)).arg(root.text((root.lastResult.approvalEvidence || {}).applicantRole))
                            .arg(root.text((root.lastResult.approvalEvidence || {}).approverActorId)).arg(root.text((root.lastResult.approvalEvidence || {}).approverRole))
                            .arg(root.auditText(root.lastResult.auditRecordConfirmed)).arg(root.text(root.lastResult.auditRecordOutcome))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: !!root.lastResult.reviewId
                        textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("独立历史摘要的内存接纳：%1\n摘要返回：%2\n这不是SQL落盘确认。须先显式开启归档；可在诊断归档中查看当前录制批次或已封存段。原账号、自由理由和完整配置不会写入此摘要；失败不撤回已经提交的登记变更。")
                            .arg(root.auditText(root.lastResult.archiveSummaryRecordConfirmed)).arg(root.text(root.lastResult.archiveSummaryOutcome))
                    }
                    QGCButton {
                        id: commitDetailsButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        visible: root.committedFact.mutationCommitted === true
                        text: view.showCommit ? qsTr("折叠历史原生提交回执") : qsTr("展开历史原生提交回执（完整只读）")
                        Accessible.name: text; onClicked: view.showCommit = !view.showCommit
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(commitDetailsButton)
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.committedFact.mutationCommitted === true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("保留的历史变更：审阅 %1 · 机号 %2 · %3\n原生申请账号 %4（%5）\n原生独立审批账号 %6（%7）\n绑定 SHA-256：%8\n当次审计追加：%9\n审计返回原因：%10\n审计确认不是持久化证明，未确认或未知不推断回滚。本地账号记录不是硬件签名或性能认证；后续失败结果不会覆盖这条已提交事实。")
                            .arg(root.text(root.committedFact.reviewId)).arg(root.committedFact.vehicleId).arg(root.operationText(root.committedFact.operation))
                            .arg(root.text((root.committedFact.approvalEvidence || {}).applicantActorId)).arg(root.text((root.committedFact.approvalEvidence || {}).applicantRole))
                            .arg(root.text((root.committedFact.approvalEvidence || {}).approverActorId)).arg(root.text((root.committedFact.approvalEvidence || {}).approverRole))
                            .arg(root.text((root.committedFact.approvalEvidence || {}).bindingHash))
                            .arg(root.auditText(root.committedFact.auditRecordConfirmed)).arg(root.text(root.committedFact.auditRecordOutcome))
                    }
                    Loader {
                        Layout.fillWidth: true; active: view.showCommit && root.committedFact.mutationCommitted === true
                        sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText; text: root.committedFact.commitReceiptAvailable === true ? root.fullJson(root.committedFact.commitReceipt) : qsTr("已返回变更提交为真，但完整回执无法显示；不推断回滚、完整成功或仍为当前，请交工程师核查。"); color: qgcPal.text; Accessible.name: qsTr("历史原生提交回执，只读且不截断"); background: Rectangle { color: qgcPal.windowShade } } }
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                    }
                }
            }
        }
    }
}

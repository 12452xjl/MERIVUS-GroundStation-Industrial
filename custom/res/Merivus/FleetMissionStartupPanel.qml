import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// The controller owns all native sources, identities, gates and commands.
// Selecting, opening or closing this panel never invokes a native action.
ColumnLayout {
    id: root
    property var controller: null
    property bool callBusy: false
    property bool intentExhausted: false
    property var lifetime: ({alive: true, epoch: 0, ticket: null})
    property int modeIndex: 0
    property string actionKey: ""
    property int detailIndex: 0
    property int page: 0
    property string expandedRowKey: ""
    property bool showGroupRisk: false
    property var consent: null
    property string notice: ""
    property string lastReceipt: ""
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var workbench: controller && map(controller.fleetStartupWorkbench) ? controller.fleetStartupWorkbench : ({})
    readonly property var snapshot: snapshotKnown(workbench.snapshot) ? workbench.snapshot : ({})
    readonly property string snapshotKey: snapshotKnown(snapshot) ? JSON.stringify(snapshot) : ""
    readonly property string sourceToken: isString(workbench.sourceToken) ? workbench.sourceToken : ""
    readonly property string identityToken: isString(workbench.identityToken) ? workbench.identityToken : ""
    readonly property var identity: map(workbench.identity) ? workbench.identity : ({})
    readonly property var deployment: map(workbench.deployment) ? workbench.deployment : ({})
    readonly property string deploymentIdentity: JSON.stringify([deployment.deploymentId, deployment.generation, deployment.binding])
    readonly property bool ground: modeIndex === 1
    readonly property var review: safeMap(ground ? workbench.groundReview : workbench.review)
    readonly property var approval: safeMap(ground ? workbench.groundApproval : workbench.approval)
    readonly property var authority: safeMap(ground ? workbench.groundAuthority : workbench.authority)
    readonly property var transaction: safeMap(ground ? workbench.groundTransaction : workbench.transaction)
    readonly property var risk: safeMap(ground ? workbench.groundRisk : workbench.review)
    readonly property var contentCheck: safeMap(workbench.contentCheck)
    readonly property var series: safeMap(workbench.series)
    readonly property bool ready: visible && workbenchPopup.visible && lifetime.alive && !intentExhausted
        && controller && typeof controller.performFleetStartupAction === "function" && workbench.sourceKnown === true
        && snapshotKey.length > 0 && !callBusy
    readonly property var actions: actionDefinitions(ground)
    readonly property var action: selectedAction()
    readonly property string actionTitle: action ? action.title : qsTr("尚未选择操作")
    readonly property string actionBlock: blockedAction()
    readonly property bool confirmationCurrent: confirmationMatches(consent)
    readonly property var rowSource: detailIndex === 0 ? (ground ? review : contentCheck) : detailIndex === 1 ? risk : transaction
    readonly property var allRows: boundedRows(rowSource.vehicles)
    readonly property int pageCount: Math.ceil(allRows.length / 8)
    readonly property var pageRows: allRows.slice(page * 8, page * 8 + 8)
    readonly property string rowIdentity: JSON.stringify([sourceToken, ground, detailIndex, rowSource.checkId,
        rowSource.reviewId, rowSource.startupId, rowSource.generation, rowSource.localGeneration,
        rowSource.artifactFingerprint, rowSource.sourceFingerprint, rowSource.sourceHash])
    spacing: pad / 2
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function map(value) { return value !== null && typeof value === "object" && !Array.isArray(value) }
    function safeMap(value) { return map(value) ? value : ({}) }
    function isString(value) { return typeof value === "string" }
    function exact(value, keys) {
        if (!map(value) || Object.keys(value).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, keys[i])) return false
        return true
    }
    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function text(value) {
        if (typeof value === "string" && value.length > 0) return value
        if (typeof value === "number" && isFinite(value)) return String(value)
        return qsTr("未知 / 未提供")
    }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function snapshotKnown(value) {
        return exact(value, ["sourceToken", "identityToken", "stamp"]) && isString(value.sourceToken) && value.sourceToken.length > 0
            && isString(value.identityToken) && isString(value.stamp) && value.stamp.length > 0 && value.stamp.length <= 65536
    }
    function boundedRows(value) { return Array.isArray(value) && value.length <= 32 ? value : [] }
    function ids(value) {
        if (!Array.isArray(value) || value.length > 32) return qsTr("未知 / 非完整目标列表")
        for (var i = 0; i < value.length; ++i) if (!integer(value[i], 1, 255)) return qsTr("未知 / 非完整目标列表")
        return value.length ? value.join(", ") : qsTr("尚无目标")
    }
    function json(value) { return map(value) || Array.isArray(value) ? JSON.stringify(value, null, 2) : qsTr("未知 / 未提供") }
    function groupRiskText() {
        if (!riskReadable()) return qsTr("尚无完整可查看的本次风险来源；不能把空对象视为没有风险。")
        var result = {}, keys = Object.keys(risk)
        for (var i = 0; i < keys.length; ++i) if (keys[i] !== "vehicles") result[keys[i]] = risk[keys[i]]
        return json(result)
    }
    function riskReadable() {
        // The ground display is supplied by the native whitelist, never by
        // importing a report or promoting the previous startup's approval.
        if (!map(risk) || Object.keys(risk).length === 0 || !presentId(review.reviewId)
                || !Array.isArray(risk.vehicles) || risk.vehicles.length < 1 || risk.vehicles.length > 32) return false
        if (!ground) return risk.reviewId === review.reviewId && presentId(risk.artifactFingerprint) && risk.reasonsTruncated !== true
        if (!exact(risk, ["schemaVersion", "reviewId", "reviewEpoch", "previousStartupId", "sourceFingerprint", "state", "expired", "available", "reason",
            "targetVehicleIds", "constraints", "healthGate", "blockedReasons", "unverifiedRequirements", "reasonsTruncated", "vehicles", "limitations",
            "allNewRawReadsMatched", "readonly", "historicalOnly", "authenticated", "startupPermitGranted", "executable", "flightCommandReleased"])
                || risk.schemaVersion !== 1 || risk.available !== true || risk.expired !== false || risk.reasonsTruncated !== false
                || risk.reviewId !== review.reviewId || risk.reviewEpoch !== review.reviewEpoch || risk.sourceFingerprint !== review.sourceFingerprint
                || risk.previousStartupId !== review.previousStartupId || risk.state !== review.state
                || risk.readonly !== true || risk.historicalOnly !== true || risk.authenticated !== false
                || risk.startupPermitGranted !== false || risk.executable !== false || risk.flightCommandReleased !== false
                || !map(risk.constraints) || !map(risk.healthGate) || !Array.isArray(risk.blockedReasons)
                || !Array.isArray(risk.unverifiedRequirements) || !Array.isArray(risk.limitations)) return false
        for (var i = 0; i < risk.vehicles.length; ++i) {
            var row = risk.vehicles[i]
            if (!exact(row, ["vehicleId", "componentId", "frozenArmed", "blockedReasons", "unverifiedRequirements", "reasonsTruncated", "readinessSummary", "startupTransitionPolicy"])
                    || !integer(row.vehicleId, 1, 255) || !integer(row.componentId, 1, 255) || typeof row.frozenArmed !== "boolean"
                    || row.reasonsTruncated !== false || !Array.isArray(row.blockedReasons) || !Array.isArray(row.unverifiedRequirements)
                    || !map(row.readinessSummary) || !map(row.startupTransitionPolicy)) return false
        }
        return true
    }
    function actionDefinitions(isGround) {
        if (isGround) return [
            {key: "ground-read", title: qsTr("1 · 地面再审：新整组读取与风险捕获"), effect: qsTr("真实发送整组机载任务读取请求。只接受最新实际已停止、排空、从未可能发送 MODE 的原事务；混合已解锁 / 未解锁仍需保持地面状态。撤旧依据，不恢复旧批准。"), kind: "read"},
            {key: "ground-ack", title: qsTr("2 · 明确已阅本次地面风险"), effect: qsTr("记录对本份新风险的已阅确认，并重新核对来源。不是飞行许可，也不续期。"), kind: "ack"},
            {key: "ground-approval-request", title: qsTr("3 · 请求另一真实账号的地面条件批准"), effect: qsTr("将打开原生独立账号认证。主身份与原审阅者必须一致，第二账号必须是不同真实 SID 且具备审批角色；不会自动提交下一步。"), kind: "approval"},
            {key: "ground-authority-request", title: qsTr("4 · 一次接管地面再审的有限依据"), effect: qsTr("原生工作流接管当前真实审批 / 来源和本地整组占用；旧批准不复活，期限只缩短。此动作本身不发送解锁或模式命令。"), kind: "authority"},
            {key: "ground-start", title: qsTr("5 · 真发送：地面再审 Seq0 / 必要 ARM / MISSION"), effect: qsTr("启动有界异步事务，可能真实发送序号 0、普通 ARM 和任务模式命令。原已审阅为解锁的成员跳过 ARM，但仍核整组屏障；无强制解锁、自动重试、自动 DISARM 或失败回滚。"), kind: "flight"},
            {key: "ground-cancel", title: qsTr("停止地面再审读取 / 撤销该风险依据"), effect: qsTr("停止该原生范围后续读取并等待排空；已经排队的读取可能仍在途。不回滚、不停止飞机。"), kind: "revoke"},
            {key: "ground-approval-revoke", title: qsTr("撤销本次地面条件批准"), effect: qsTr("仅撤销该本地批准范围，不撤回已排队包，不改变飞机 armed / 模式。"), kind: "revoke"},
            {key: "ground-authority-revoke", title: qsTr("撤销地面再审有限依据"), effect: qsTr("仅撤销对应来源依据及后续发送资格，按原生流程排空；不发 DISARM，不恢复旧授权。"), kind: "revoke"},
            {key: "ground-stop", title: qsTr("停止当前地面再审启动事务（不回滚）"), effect: qsTr("停止对应事务后续发送；已经排队的 Seq0 / ARM / MODE 仍可能生效。保留真实部分结果和占用排空，不自动停机或降落。"), kind: "revoke"}
        ]
        return [
            {key: "content-read", title: qsTr("1 · 新整组机载任务核对（真实只读请求）"), effect: qsTr("撤销旧审阅 / 审批并真实发送完整机载 Mission INT 读取；不上传、解锁或启动。来源必须是实际已上传且历史整组回读匹配的部署，不是当前草稿或归档导入。"), kind: "read"},
            {key: "review-create", title: qsTr("2 · 生成独立启动风险审阅"), effect: qsTr("使用本次新鲜整组读取和实际风险来源生成不可变审阅；不发送飞行命令，不续旧读取期限。"), kind: "review"},
            {key: "review-ack", title: qsTr("3 · 明确已阅本次启动风险"), effect: qsTr("仅记录已阅读这份风险，并完整重新核对来源；已阅不等于可起飞，未知和未证实项目不会消失。"), kind: "ack"},
            {key: "approval-request", title: qsTr("4 · 请求另一真实账号的条件启动批准"), effect: qsTr("将打开原生独立账号认证。主账号须具备启动角色并与原审阅者一致；另一真实 SID 需具备安全审批角色。一个勾选或昵称不能代替两账号认证。"), kind: "approval"},
            {key: "authority-request", title: qsTr("5 · 一次接管本次有限启动依据"), effect: qsTr("原生工作流接管真实身份 / 审批 / 来源和整组本地占用。它本身不发命令，也不是最终 StartPermit；只保留原短期预算。"), kind: "authority"},
            {key: "start", title: qsTr("6 · 真发送：Seq0 → 普通 ARM → MISSION"), effect: qsTr("启动有界异步事务，可能真实修改起始序号、普通解锁并切换任务模式。全组每阶段依新 RAW 观测过屏障，不凭 ACK 宣告成功；禁止 force arm、自动重试、自动 DISARM 和回滚。"), kind: "flight"},
            {key: "content-cancel", title: qsTr("停止内容读取 / 撤销读取依据"), effect: qsTr("停止本范围后续读取并等待原协议排空；在途只读请求无法撤回。不恢复旧批准。"), kind: "revoke"},
            {key: "review-revoke", title: qsTr("撤销本次风险审阅"), effect: qsTr("撤销本地风险已阅依据；不删除或回滚已上传任务，不改变飞机状态。"), kind: "revoke"},
            {key: "approval-revoke", title: qsTr("撤销本次条件启动批准"), effect: qsTr("仅撤销该批准范围；不撤回已发送命令，不自动 DISARM。"), kind: "revoke"},
            {key: "authority-revoke", title: qsTr("撤销本次有限启动依据"), effect: qsTr("撤销该原生依据及后续发送资格；可能需要等待本地占用排空。不是回滚飞机动作。"), kind: "revoke"},
            {key: "cancel", title: qsTr("停止初次启动事务（已发命令不回滚）"), effect: qsTr("停止当前事务后续发送。已经排队的序号 / ARM / MODE 仍可能生效；保留部分结果，不自动停机、降落或撤回在途包。"), kind: "revoke"}
        ]
    }
    function selectedAction() {
        for (var i = 0; i < actions.length; ++i) if (actions[i].key === actionKey) return actions[i]
        return null
    }
    function actionChoices() {
        var result = [qsTr("先选择一个明确操作")]
        for (var i = 0; i < actions.length; ++i) result.push(actions[i].title)
        return result
    }
    function actionIndex() {
        for (var i = 0; i < actions.length; ++i) if (actions[i].key === actionKey) return i + 1
        return 0
    }
    function presentId(value) { return typeof value === "string" && value.length > 0 && value.length <= 128 }
    function blockedAction() {
        if (!controller || workbench.sourceKnown !== true || !snapshotKey) return qsTr("当前实际部署组来源未知；不会改用选中的旧运行或导入报告。")
        if (intentExhausted) return qsTr("本页确认序号已耗尽，请重建界面后重新核对。")
        if (callBusy) return qsTr("当前调用尚未返回，禁止重复提交；关闭页面不回滚动作。")
        if (!action) return qsTr("请选择动作，再阅读它对实际飞机或本地依据的影响。")
        if (action.kind !== "revoke" && identity.authenticated !== true) return qsTr("请先在原操作员身份入口完成实际认证；本页不手填或伪造身份。")
        if (actionKey === "content-read" && !presentId(deployment.deploymentId)) return qsTr("尚无可核对的真实部署；必须先完成原上传流程。")
        if (actionKey === "content-cancel" && !presentId(contentCheck.checkId)) return qsTr("尚无可停止的本次内容读取。")
        if (actionKey === "review-create" && (contentCheck.state !== "content-matched" || contentCheck.ok !== true)) return qsTr("需要本次完整且仍有效的机载内容匹配；未完成、未知或过期不能继续。")
        if (["review-ack", "review-revoke", "approval-request", "authority-request", "ground-ack", "ground-cancel", "ground-approval-request", "ground-authority-request"].indexOf(actionKey) >= 0 && !presentId(review.reviewId)) return qsTr("尚无对应的真实风险审阅。")
        if (action.kind === "ack" && (!riskReadable() || !showGroupRisk)) return qsTr("请先展开本次整组风险字段，并查看逐机风险页；缺失内容不是零风险。")
        if (action.kind === "ack" && review.state !== (ground ? "content-ready" : "reviewable")) return qsTr("本次风险不处于可已阅状态；请核对过期、来源变化和原生原因。")
        if (action.kind === "approval" && review.state !== "acknowledged") return qsTr("需要同一份仍有效的已阅风险；旧批准不恢复。")
        if (action.kind === "authority" && approval.conditionalIntentApproved !== true) return qsTr("尚无本次真实两账号条件批准，不能接管有限依据。")
        if (["approval-revoke", "ground-approval-revoke"].indexOf(actionKey) >= 0 && !presentId(approval.approvalId)) return qsTr("尚无对应批准可撤销。")
        if (["authority-revoke", "ground-authority-revoke"].indexOf(actionKey) >= 0 && !presentId(authority.startupId)) return qsTr("尚无对应有限依据可撤销。")
        if (action.kind === "flight" && (!presentId(authority.startupId) || authority.authorityAccepted !== true)) return qsTr("原生当前有限依据尚未接受或已失效；禁止真发送请求。")
        if (["cancel", "ground-stop"].indexOf(actionKey) >= 0 && !presentId(transaction.startupId)) return qsTr("尚无本分支的实际事务可停止；不会回退到另一事务。")
        if (actionKey === "ground-read" && (!presentId(series.previousStartupId) || series.evidenceLost !== false)) return qsTr("需要当前组保留的最新实际事务；来源丢失不回退旧 owner。")
        return ""
    }
    function clearConfirmation() { consent = null; if (executeButton) executeButton.pressedIntent = null }
    function invalidateContext() {
        if (!integer(lifetime.epoch, 0, 9007199254740990)) intentExhausted = true
        else lifetime.epoch += 1
        clearConfirmation(); showGroupRisk = false; expandedRowKey = ""
    }
    function freezeIntent() {
        return ready && !lifetime.ticket && action && !actionBlock ? {owner: controller, epoch: lifetime.epoch, action: actionKey,
            sourceToken: sourceToken, identityToken: identityToken, snapshotKey: snapshotKey,
            expected: JSON.parse(JSON.stringify(snapshot)), title: actionTitle,
            deploymentId: text(deployment.deploymentId), recordId: text(safeMap(deployment.binding).recordId)} : null
    }
    function contextCurrent(value) {
        return value && lifetime.alive && !intentExhausted && value.owner === controller && value.epoch === lifetime.epoch
            && value.sourceToken === sourceToken && value.identityToken === identityToken && value.action === actionKey
            && visible && workbenchPopup.visible
    }
    function confirmationMatches(value) {
        return contextCurrent(value) && value.action === actionKey && value.snapshotKey === snapshotKey
            && snapshotKnown(value.expected) && JSON.stringify(value.expected) === snapshotKey
    }
    function outcomeKnown(value) {
        return exact(value, ["ok", "reason", "invoked", "accepted", "current", "flightCommandReleased", "result"])
            && typeof value.ok === "boolean" && isString(value.reason) && typeof value.invoked === "boolean"
            && (value.accepted === null || typeof value.accepted === "boolean") && typeof value.current === "boolean"
            && (value.flightCommandReleased === null || typeof value.flightCommandReleased === "boolean")
    }
    function dispatch(value) {
        if (!ready || lifetime.ticket || !confirmationCurrent || !confirmationMatches(value) || actionBlock) {
            clearConfirmation(); notice = qsTr("动作、身份、来源或阶段状态已变化，未发起该请求；请重新查看影响并确认。"); return
        }
        var guard = lifetime, ticket = {}, result = null, entered = false
        guard.ticket = ticket; callBusy = true
        if (!guard.alive) { if (guard.ticket === ticket) guard.ticket = null; return }
        clearConfirmation()
        try {
            if (contextCurrent(value) && value.snapshotKey === snapshotKey && guard.ticket === ticket) {
                entered = true
                result = value.owner.performFleetStartupAction(value.action, value.expected, true)
            }
            if (!guard.alive) return
            if (!outcomeKnown(result)) {
                lastReceipt = qsTr("动作：%1\n原部署：%2 · 原记录：%3\n%4。不要据此判断没有排队或飞机已经停止；没有自动重试。")
                    .arg(value.title).arg(value.deploymentId).arg(value.recordId)
                    .arg(entered ? qsTr("未取得完整调用回执，接受 / 提交 / 飞行命令状态未知") : qsTr("调用前来源变化，本次尚未调用统一入口"))
            } else {
                lastReceipt = qsTr("动作：%1\n原部署：%2 · 原记录：%3\n已调用原入口：%4；请求接受 / 本地操作返回：%5；返回时上下文仍当前：%6。\n此返回的飞行命令排队标记：%7。\n原生原因：%8\n接受不等于阶段完成；异步事务可能在返回后继续排队。请检查当前逐机账单，不推断任务执行、完成或已经安全停机。")
                    .arg(value.title).arg(value.deploymentId).arg(value.recordId).arg(known(result.invoked)).arg(known(result.accepted))
                    .arg(known(result.current === true && contextCurrent(value))).arg(known(result.flightCommandReleased)).arg(text(result.reason))
            }
            notice = ""
        } catch (error) {
            if (guard.alive) lastReceipt = qsTr("动作 %1 的调用回执中断；原部署 %2 / 记录 %3。是否已接受、排队或提交未知，不能按失败回执推断无动作。请核对原生状态，不自动重试、不自动回滚。")
                .arg(value.title).arg(value.deploymentId).arg(value.recordId)
        } finally {
            if (guard.alive) callBusy = false
            if (guard.ticket === ticket) guard.ticket = null
        }
        if (guard.alive && !contextCurrent(value)) notice = qsTr("界面或身份已变化：上方回执仅为原调用的历史事实；不抹除已经接受或可能排队的动作，须重新核对当前来源。")
    }
    function remaining(value) {
        var raw = value.remainingLifetimeMs !== undefined ? value.remainingLifetimeMs : value.remainingMs
        return typeof raw === "number" && isFinite(raw) && raw >= 0 ? qsTr("%1 毫秒（仅最后快照，非续期或当前保证）").arg(raw) : qsTr("未知")
    }
    function stageSummary(value, label) {
        return qsTr("%1：%2\n原因：%3\n活动 %4 · 本地占用 %5 · 协议排空 %6\n剩余预算：%7")
            .arg(label).arg(text(value.state)).arg(text(value.reason)).arg(known(value.active))
            .arg(known(value.reservationHeld)).arg(known(value.transportDraining)).arg(remaining(value))
    }
    function rowKey(row) { return rowIdentity + ":" + text(row.vehicleId) }
    function rowSummary(row) {
        if (!map(row)) return qsTr("未知逐机值；不能视为匹配。")
        var value = qsTr("机 %1：%2\n原因：%3").arg(text(row.vehicleId)).arg(text(row.state || row.phase)).arg(text(row.reason))
        if (detailIndex === 0) return value + qsTr("\n内容项数：%1；冻结已解锁：%2（未知不补未解锁）。").arg(text(row.itemCount)).arg(known(row.frozenArmed))
        if (detailIndex === 1) return value + qsTr("\n完整风险 / 政策字段可在本行展开，缺失或空值保持未知。")
        if (!Array.isArray(row.steps) || row.steps.length !== 3) return value + qsTr("\n三步收据未知 / 不完整，不证明未发送。")
        for (var i = 0; i < row.steps.length; ++i) {
            var step = row.steps[i], name = step.step === "Seq0" ? qsTr("序号 0") : step.step === "NormalArm" ? qsTr("普通 ARM") : step.step === "MissionModeOnly" ? qsTr("任务模式") : text(step.step)
            value += qsTr("\n%1：请求已排队 %2 / 新 RAW 状态已观察 %3 / 可能已提交 %4 / 原已解锁跳过 %5。")
                .arg(name).arg(known(step.requestIssued)).arg(known(step.rawStateObserved)).arg(known(step.mayHaveCommitted)).arg(known(step.skippedReviewedAlreadyArmed))
        }
        return value + qsTr("\n这些是本地队列与观察事实，不是 ACK 因果、任务执行或完成证明。")
    }
    function ensureFocusVisible(item) {
        if (!workbenchPopup.visible || !item) return
        var point = item.mapToItem(body, 0, 0), top = point.y - pad, bottom = point.y + item.height + pad
        var maximum = Math.max(0, scroller.contentHeight - scroller.height)
        if (top < scroller.contentY || item.height + pad * 2 > scroller.height) scroller.contentY = Math.min(maximum, Math.max(0, top))
        else if (bottom > scroller.contentY + scroller.height) scroller.contentY = Math.min(maximum, Math.max(0, bottom - scroller.height))
    }
    function keepFocusVisible() {
        if (previousPage.activeFocus) ensureFocusVisible(previousPage)
        else if (nextPage.activeFocus) ensureFocusVisible(nextPage)
        else if (executeButton.activeFocus) ensureFocusVisible(executeButton)
    }
    onControllerChanged: { invalidateContext(); page = 0 }
    onSourceTokenChanged: { invalidateContext(); page = 0 }
    onIdentityTokenChanged: invalidateContext()
    onSnapshotKeyChanged: { clearConfirmation(); showGroupRisk = false }
    onDeploymentIdentityChanged: { page = 0; expandedRowKey = "" }
    onModeIndexChanged: { invalidateContext(); actionKey = ""; detailIndex = 0; page = 0 }
    onActionKeyChanged: clearConfirmation()
    onDetailIndexChanged: { page = 0; expandedRowKey = "" }
    onRowIdentityChanged: { page = 0; expandedRowKey = "" }
    onAllRowsChanged: if (page >= Math.ceil(allRows.length / 8)) { page = 0; expandedRowKey = "" }
    onVisibleChanged: if (!visible && lifetime.alive) { invalidateContext(); if (workbenchPopup) workbenchPopup.close() }
    Component.onDestruction: lifetime.alive = false

    QGCButton {
        id: openButton
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: qsTr("打开实际部署启动工作台"); Accessible.name: text
        contentItem: QGCLabel { text: openButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: openButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: openButton.activeFocus }
        onClicked: { root.invalidateContext(); workbenchPopup.open() }
    }
    Controls.Popup {
        id: workbenchPopup
        parent: Controls.Overlay.overlay
        modal: true; focus: true; closePolicy: Controls.Popup.CloseOnEscape
        padding: root.pad
        width: parent ? Math.max(0, Math.min(parent.width - root.pad * 2, ScreenTools.defaultFontPixelWidth * 84)) : 0
        height: parent ? Math.max(0, Math.min(parent.height - root.pad * 2,
            root.pad * 3 + Math.max(root.rowHeight, closeButton.implicitHeight) + Math.min(body.implicitHeight, ScreenTools.defaultFontPixelHeight * 26))) : 0
        x: parent ? Math.max(0, (parent.width - width) / 2) : 0
        y: parent ? Math.max(0, (parent.height - height) / 2) : 0
        background: Rectangle { color: qgcPal.window; border.width: 1; border.color: qgcPal.text }
        onOpened: closeButton.forceActiveFocus()
        onVisibleChanged: if (!visible && root.lifetime.alive) root.invalidateContext()
        onClosed: if (root.lifetime.alive && root.visible) openButton.forceActiveFocus()
        contentItem: ColumnLayout {
            spacing: root.pad
            QGCButton {
                id: closeButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("关闭工作台（不会停止飞机或回滚事务）"); Accessible.name: text
                contentItem: QGCLabel { text: closeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: closeButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: closeButton.activeFocus }
                onClicked: workbenchPopup.close()
            }
            Flickable {
                id: scroller
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 0
                clip: true; contentWidth: width; contentHeight: body.implicitHeight; boundsBehavior: Flickable.StopAtBounds
                onContentHeightChanged: root.keepFocusVisible()
                Controls.ScrollBar.vertical: Controls.ScrollBar { }
                ColumnLayout {
                    id: body
                    width: Math.max(0, scroller.width - root.pad); spacing: root.pad
                    QGCLabel { Layout.fillWidth: true; font.bold: true; text: qsTr("实际启动操作 · 一步一确认，不自动串联"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("本页真发送操作可能解锁电机并切换任务模式。最终飞行放行、桨区净空和硬件安全仍未证明，这不表示后端绝不会发命令：实际条件链允许时会真实发送。请核对冻结目标、现场状态及每一步影响。") }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: qsTr("当前实际部署组来源：%1\n部署 %2 · 代次 %3 · 状态 %4\n归档记录 %5 · 版本 %6\n原冻结目标：%7\n当前主身份已认证：%8 · 角色 %9 · 账户 %10\n选中的旧运行 / 导入报告不是本页启动来源。")
                        .arg(root.known(root.workbench.sourceKnown)).arg(root.text(root.deployment.deploymentId)).arg(root.text(root.deployment.generation)).arg(root.text(root.deployment.state))
                        .arg(root.text(root.safeMap(root.deployment.binding).recordId)).arg(root.text(root.safeMap(root.deployment.binding).recordRevision)).arg(root.ids(root.deployment.targetVehicleIds))
                        .arg(root.known(root.identity.authenticated)).arg(root.text(root.identity.primaryRole)).arg(root.text(root.identity.actorId)) }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("原部署本地占用：%1；协议排空：%2。未知不等于已释放，启动读取仍由原生检查完整历史上传来源。").arg(root.known(root.deployment.reservationHeld)).arg(root.known(root.deployment.transportDraining)) }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("操作分支（不会自动选择或继续原生事务）"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    QGCComboBox {
                        id: branchChoice
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: !root.callBusy
                        model: [qsTr("初次启动链"), qsTr("停止后的地面再审链")]; currentIndex: root.modeIndex; Accessible.name: qsTr("启动工作台操作分支")
                        onActivated: { root.modeIndex = index; currentIndex = Qt.binding(function() { return root.modeIndex }) }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: branchChoice.activeFocus }
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.ground; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: qsTr("最新原事务：%1；来源丢失：%2；地面新事务已用 %3 / 容量 %4。\n必须原事务停止并排空、从未可能发送 MODE；不能借本页从空中重新接管。原已解锁成员不代表可以直接继续。")
                        .arg(root.text(root.series.previousStartupId)).arg(root.known(root.series.evidenceLost)).arg(root.text(root.series.groundConsumerCount)).arg(root.text(root.series.capacity)) }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: root.stageSummary(root.ground ? root.review : root.contentCheck, root.ground ? qsTr("地面新读取 / 风险") : qsTr("本次整组内容核对")) }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: qsTr("风险审阅：%1 · %2 · 已阅 %3\n条件批准：%4 · %5 · 条件意图已批准 %6\n有限依据：%7 · %8 · 原生已接受 %9\n这些状态不能替代净空、硬件认证、执行 / 完成证明。所有短期限从原最早取证起算，不因已阅、认证或本页显示续期。")
                        .arg(root.text(root.review.reviewId)).arg(root.text(root.review.state)).arg(root.known(root.ground ? root.review.reviewAcknowledged : root.review.acknowledged))
                        .arg(root.text(root.approval.approvalId)).arg(root.text(root.approval.state)).arg(root.known(root.approval.conditionalIntentApproved))
                        .arg(root.text(root.authority.startupId)).arg(root.text(root.authority.state)).arg(root.known(root.authority.authorityAccepted)) }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: root.stageSummary(root.transaction, qsTr("本分支实际事务")) + qsTr("\n阶段：%1；可能已提交命令：%2；任务序号可能改变：%3；飞行命令实际排队标记：%4。\nstarted-observed 只指该范围的新 RAW 状态已见，不是任务完成。")
                        .arg(root.text(root.transaction.stage)).arg(root.known(root.transaction.commandsMayHaveCommitted)).arg(root.known(root.transaction.missionStateMayHaveChanged)).arg(root.known(root.transaction.flightCommandReleased)) }
                    QGCButton {
                        id: riskButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.riskReadable() && !root.callBusy; opacity: enabled ? 1 : 0.45
                        text: root.showGroupRisk ? qsTr("收起整组风险字段（同时撤销本页确认）") : qsTr("展开本次完整整组风险字段（逐机见下方分页）"); Accessible.name: text
                        contentItem: QGCLabel { text: riskButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: riskButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: riskButton.activeFocus }
                        onClicked: { root.clearConfirmation(); root.showGroupRisk = !root.showGroupRisk }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: !root.riskReadable(); text: qsTr("完整风险来源未知、失效或尚未生成；不显示为无风险，也不允许已阅。原生原因：%1").arg(root.text(root.risk.reason)); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                    QGCLabel { Layout.fillWidth: true; visible: root.showGroupRisk; text: qsTr("以下为原生风险字段原名和原值；false、null、缺失不能补成通过。逐机政策和缺口需在风险页展开查看。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    QGCLabel { Layout.fillWidth: true; visible: root.showGroupRisk; text: root.showGroupRisk ? root.groupRiskText() : ""; textFormat: Text.PlainText; wrapMode: Text.Wrap }
                    FleetMissionStartupSpatialPanel {
                        Layout.fillWidth: true
                        controller: root.controller
                        workbench: root.workbench
                        ground: root.ground
                        onFocusVisibilityRequested: root.ensureFocusVisible(item)
                        onMapRequested: { root.clearConfirmation(); workbenchPopup.close() }
                    }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("逐机账单（每页 8 架，最多 32 架）"); font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    QGCComboBox {
                        id: rowChoice
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        model: [qsTr("本次读取"), qsTr("本次风险 / 政策"), qsTr("本分支三步事务")]; currentIndex: root.detailIndex; Accessible.name: qsTr("逐机账单类别")
                        onActivated: { root.detailIndex = index; currentIndex = Qt.binding(function() { return root.detailIndex }) }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: rowChoice.activeFocus }
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.allRows.length ? qsTr("第 %1 / %2 页，共 %3 架。未显示页不是不存在。").arg(root.page + 1).arg(root.pageCount).arg(root.allRows.length) : qsTr("尚无可显示的完整逐机列表；未知 / 未取得，不是全部通过。") }
                    Repeater {
                        model: workbenchPopup.visible ? root.pageRows : []
                        ColumnLayout {
                            id: vehicleRow
                            readonly property var row: modelData
                            readonly property string entryKey: root.rowKey(row)
                            readonly property bool detailsOpen: root.expandedRowKey === entryKey
                            Layout.fillWidth: true; spacing: root.pad / 2
                            QGCLabel { Layout.fillWidth: true; text: root.rowSummary(vehicleRow.row); textFormat: Text.PlainText; wrapMode: Text.Wrap }
                            QGCButton {
                                id: rowDetails
                                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                                text: vehicleRow.detailsOpen ? qsTr("收起该机完整原生字段") : qsTr("展开该机完整原生字段 / 原因 / 政策"); Accessible.name: qsTr("机 %1：%2").arg(root.text(vehicleRow.row.vehicleId)).arg(text)
                                contentItem: QGCLabel { text: rowDetails.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: rowDetails._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: rowDetails.activeFocus }
                                onClicked: { root.expandedRowKey = vehicleRow.detailsOpen ? "" : vehicleRow.entryKey; root.ensureFocusVisible(this) }
                                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            }
                            QGCLabel { Layout.fillWidth: true; visible: vehicleRow.detailsOpen; text: vehicleRow.detailsOpen ? root.json(vehicleRow.row) : ""; textFormat: Text.PlainText; wrapMode: Text.Wrap }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: root.pad
                        QGCButton { id: previousPage; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("上一页"); Accessible.name: text; showBorder: true; enabled: root.page > 0; opacity: enabled ? 1 : 0.45; onClicked: { root.expandedRowKey = ""; root.page -= 1; root.ensureFocusVisible(this) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: previousPage.activeFocus } }
                        QGCButton { id: nextPage; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("下一页"); Accessible.name: text; showBorder: true; enabled: root.page + 1 < root.pageCount; opacity: enabled ? 1 : 0.45; onClicked: { root.expandedRowKey = ""; root.page += 1; root.ensureFocusVisible(this) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: nextPage.activeFocus } }
                    }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("选择本次唯一操作，先阅读影响，再明确确认"); font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    QGCComboBox {
                        id: actionChoice
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: !root.callBusy
                        model: root.actionChoices(); currentIndex: root.actionIndex(); Accessible.name: qsTr("选择一个原生启动工作流操作")
                        onActivated: { root.actionKey = index > 0 && index <= root.actions.length ? root.actions[index - 1].key : ""; currentIndex = Qt.binding(function() { return root.actionIndex() }) }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: actionChoice.activeFocus }
                    }
                    QGCLabel { Layout.fillWidth: true; text: root.actionTitle; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.Wrap }
                    QGCLabel { Layout.fillWidth: true; text: root.action ? root.action.effect : qsTr("没有默认启动动作；打开窗口不会读取、认证、审批或发送命令。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: root.action && root.action.kind === "flight" ? qgcPal.warningText : qgcPal.text }
                    QGCLabel { Layout.fillWidth: true; text: root.actionBlock || qsTr("可明确请求原生再次核对；这不是本页颁发许可，实际身份 / 来源 / 期限 / 整组状态仍由后端逐次检查。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                    Controls.CheckBox {
                        id: confirmBox
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.ready && !!root.action && !root.actionBlock
                        text: qsTr("已核对本次目标、阶段、风险和该动作的真实影响"); Accessible.name: text
                        checked: root.confirmationCurrent
                        contentItem: QGCLabel { text: confirmBox.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (confirmBox.indicator ? confirmBox.indicator.width : 0) + root.pad; color: qgcPal.text }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: confirmBox.activeFocus }
                        onClicked: { root.consent = checked ? root.freezeIntent() : null; checked = Qt.binding(function() { return root.confirmationCurrent }) }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                    }
                    QGCButton {
                        id: executeButton
                        property var pressedIntent: null
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        enabled: root.ready && root.confirmationCurrent && !root.actionBlock; opacity: enabled ? 1 : 0.45
                        text: root.callBusy ? qsTr("正在等待本次原生调用回执…") : root.action && root.action.kind === "flight" ? qsTr("明确请求真发送上述启动事务") : qsTr("明确执行上面选定的唯一操作"); Accessible.name: text
                        contentItem: QGCLabel { text: executeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: executeButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: executeButton.activeFocus }
                        onPressed: pressedIntent = root.freezeIntent()
                        onCanceled: pressedIntent = null
                        onClicked: { var original = pressedIntent; pressedIntent = null; root.dispatch(original) }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.notice.length > 0; text: root.notice; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: qgcPal.warningText }
                    QGCLabel { Layout.fillWidth: true; visible: root.lastReceipt.length > 0; text: qsTr("最近一次明确调用回执（历史，不随实时阶段改写）\n%1").arg(root.lastReceipt); textFormat: Text.PlainText; wrapMode: Text.Wrap }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("停止 / 撤销和关闭窗口不同：关闭只清本页确认；停止事务也不会自动 DISARM、降落、回滚或证明飞机安全停止。协议仍排空或占用未释放时必须保留未知与部分结果。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                }
            }
        }
    }
}

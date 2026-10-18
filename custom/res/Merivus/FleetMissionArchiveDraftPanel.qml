import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Explicit local-history selection. No disk reload, task execution or flight
// command; complete planning data remains inside the controller's private source.
FocusScope {
    id: root
    property var controller: null
    property bool inputSettled: false
    property bool expanded: false
    property bool dispatchBusy: false
    property double interactionEpoch: 0
    property var lifetime: ({alive: true})
    property string selectedTaskId: ""
    property var versionSnapshot: ({})
    property var versionOwner: null
    property var selectedBinding: ({})
    property bool selectedSourcePresent: false
    property int page: 0
    property string message: ""

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.4)
    readonly property var archive: controller ? controller.missionPlanArchive : null
    readonly property string storeEpoch: archive && typeof archive.storeEpoch === "string" ? archive.storeEpoch : ""
    readonly property var draftState: controller && controller.fleetMissionArchiveDraft ? controller.fleetMissionArchiveDraft : ({})
    readonly property string draftToken: typeof draftState.draftToken === "string" ? draftState.draftToken : ""
    readonly property string sourceToken: typeof draftState.sourceToken === "string" ? draftState.sourceToken : ""
    readonly property string episodeToken: controller && controller.fleetDeploymentLifecycle
        && typeof controller.fleetDeploymentLifecycle.currentToken === "string" ? controller.fleetDeploymentLifecycle.currentToken : ""
    readonly property var tasks: taskChoices()
    readonly property bool versionsCurrent: versionOwner === archive && versionSnapshot.ok === true
        && versionSnapshot.storeEpoch === storeEpoch && versionSnapshot.sourceTaskId === selectedTaskId
    readonly property var versions: versionsCurrent ? versionSnapshot.versions : []
    readonly property bool selectionCurrent: versionsCurrent && bindingKnown(selectedBinding)
        && selectedBinding.storeEpoch === storeEpoch && selectedBinding.sourceTaskId === selectedTaskId
    readonly property bool sourceCurrent: selectionCurrent && draftState.sourceCurrent === true
        && bindingsEqual((draftState.source || {}).binding, selectedBinding) && !!sourceToken
    readonly property bool trafficBusy: nativeTrafficBusy()
    readonly property bool localReady: visible && expanded && controller && archive && !!episodeToken && !!draftToken
        && inputSettled && !dispatchBusy && draftState.busy === false && !trafficBusy && interactionEpoch < 9007199254740991
    readonly property bool canList: localReady && identifier(selectedTaskId) && uuid(storeEpoch) && typeof archive.historyVersions === "function"
    readonly property bool canInspect: localReady && selectionCurrent && typeof controller.inspectFleetMissionArchiveDraft === "function"
    readonly property bool canCreate: localReady && sourceCurrent && draftState.destructionPending === false
        && typeof controller.createFleetMissionArchiveDraft === "function"
    readonly property bool canRestore: localReady && draftState.hasPrevious === true && typeof controller.restorePreviousFleetMissionDraft === "function"

    implicitHeight: content.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function integer(value, minimum, maximum) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= minimum && value <= maximum }
    function identifier(value) { return typeof value === "string" && value.length > 0 && value.length <= 128 && value === value.trim() && !/[\u0000-\u001f\u007f-\u009f]/.test(value) }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && value !== "00000000-0000-0000-0000-000000000000" }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function keys(value, expected) {
        if (!value || typeof value !== "object" || Array.isArray(value) || Object.keys(value).length !== expected.length) return false
        for (var i = 0; i < expected.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, expected[i])) return false
        return true
    }
    function bindingKnown(value) {
        return keys(value, ["storeEpoch", "sourceTaskId", "recordId", "recordRevision", "planId", "contentHash", "reviewEvidenceHash"])
            && uuid(value.storeEpoch) && identifier(value.sourceTaskId) && identifier(value.recordId)
            && integer(value.recordRevision, 1, 9007199254740991) && digest(value.planId) && digest(value.contentHash) && digest(value.reviewEvidenceHash)
    }
    function bindingOf(row) {
        return {storeEpoch: row.storeEpoch, sourceTaskId: row.sourceTaskId, recordId: row.recordId, recordRevision: row.recordRevision,
            planId: row.planId, contentHash: row.contentHash, reviewEvidenceHash: row.reviewEvidenceHash}
    }
    function bindingsEqual(left, right) {
        if (!bindingKnown(left) || !bindingKnown(right)) return false
        var fields = Object.keys(left)
        for (var i = 0; i < fields.length; ++i) if (left[fields[i]] !== right[fields[i]]) return false
        return true
    }
    function taskChoices() {
        var values = archive ? archive.summaries : null, rows = [], seen = Object.create(null)
        if (!values || !integer(values.length, 0, 256)) return []
        for (var i = 0; i < values.length; ++i) {
            var row = values[i]
            if (!row || !identifier(row.sourceTaskId) || !integer(row.recordRevision, 1, 9007199254740991) || seen[row.sourceTaskId]) return []
            seen[row.sourceTaskId] = true
            rows.push({sourceTaskId: row.sourceTaskId, label: row.sourceTaskId + qsTr(" · 最新保留版本 %1").arg(row.recordRevision)})
        }
        return rows
    }
    function taskIndex() {
        for (var i = 0; i < tasks.length; ++i) if (tasks[i].sourceTaskId === selectedTaskId) return i
        return -1
    }
    function nativeTrafficBusy() {
        if (!controller) return false
        var deployment = controller.missionDeployment ? controller.missionDeployment.current || {} : {}
        var values = [deployment, controller.missionDeployment ? controller.missionDeployment.reconciliation || {} : {},
            controller.fleetEvidenceRefresh || {}, controller.fleetStartContentCheck || {}]
        if (["preparing", "uploading", "readback"].indexOf(deployment.state) >= 0) return true
        for (var i = 0; i < values.length; ++i) if (values[i].active === true || values[i].reservationHeld === true || values[i].transportDraining === true) return true
        return false // Other actual startup owners are rechecked by Root.
    }
    function clearConfirmations() {
        if (interactionEpoch < 9007199254740991) interactionEpoch += 1
        if (createConsent) createConsent.checked = false
        if (replaceConsent) replaceConsent.checked = false
        if (restoreConsent) restoreConsent.checked = false
        if (listButton) listButton.pressedIntent = null
        if (inspectButton) inspectButton.pressedIntent = null
        if (createButton) createButton.pressedIntent = null
        if (cancelButton) cancelButton.pressedIntent = null
        if (restoreButton) restoreButton.pressedIntent = null
    }
    function clearHistorySelection() {
        clearConfirmations()
        versionSnapshot = ({}); versionOwner = null; selectedBinding = ({}); selectedSourcePresent = false; page = 0
    }
    function selectSource(id) {
        if (!localReady || !identifier(id)) return
        clearHistorySelection(); selectedTaskId = id; message = ""
    }
    function versionResultKnown(value, task, epoch) {
        if (!keys(value, ["ok", "schemaVersion", "storeEpoch", "sourceTaskId", "versions", "versionCount", "allVersionsForSource", "historicalOnly", "flightCommandReleased", "executable"])
                || value.ok !== true || value.schemaVersion !== 1 || value.storeEpoch !== epoch || value.sourceTaskId !== task
                || value.allVersionsForSource !== true || value.historicalOnly !== true || value.flightCommandReleased !== false || value.executable !== false
                || !integer(value.versionCount, 0, 128) || !value.versions || typeof value.versions === "string"
                || !integer(value.versions.length, 0, 128) || value.versions.length !== value.versionCount) return false
        var previous = 9007199254740992, ids = Object.create(null)
        for (var i = 0; i < value.versions.length; ++i) {
            var row = value.versions[i]
            if (!keys(row, ["storeEpoch", "sourceTaskId", "recordId", "recordRevision", "planId", "contentHash", "reviewEvidenceHash", "recordedAtUtc", "restoredFromArchive", "planningSourcePresent"])
                    || !bindingKnown(bindingOf(row)) || row.storeEpoch !== epoch || row.sourceTaskId !== task || row.recordRevision >= previous
                    || ids[row.recordId] || typeof row.recordedAtUtc !== "string" || row.recordedAtUtc.length > 64
                    || typeof row.restoredFromArchive !== "boolean" || typeof row.planningSourcePresent !== "boolean") return false
            ids[row.recordId] = true; previous = row.recordRevision
        }
        return true
    }
    function intent(action) {
        return {action: action, owner: controller, archive: archive, epoch: storeEpoch, task: selectedTaskId,
            interactionEpoch: interactionEpoch,
            binding: selectionCurrent ? bindingOf(selectedBinding) : {}, draftToken: draftToken, sourceToken: sourceToken,
            episodeToken: episodeToken, confirmed: action === "create" ? createConsent.checked : action === "restore" ? restoreConsent.checked : true,
            replace: replaceConsent.checked}
    }
    function dispatch(original) {
        if (!original || !localReady || original.owner !== controller || original.archive !== archive || original.epoch !== storeEpoch
                || original.interactionEpoch !== interactionEpoch || original.task !== selectedTaskId
                || original.episodeToken !== episodeToken || original.draftToken !== draftToken) return
        var action = original.action, ready = action === "list" ? canList
            : action === "inspect" ? canInspect && bindingsEqual(original.binding, selectedBinding)
            : action === "create" ? canCreate && bindingsEqual(original.binding, selectedBinding) && original.sourceToken === sourceToken
                && original.confirmed && createConsent.checked && (draftState.hasPrevious !== true || (original.replace && replaceConsent.checked))
            : action === "restore" ? canRestore && original.confirmed && restoreConsent.checked
            : action === "cancel" && !!sourceToken && original.sourceToken === sourceToken
        if (!ready) { message = qsTr("所选历史版本或本地草稿已变化，请重新选择并确认。"); return }
        var guard = lifetime, result = {}
        dispatchBusy = true
        if (!guard.alive) return
        clearConfirmations()
        if (!guard.alive) return
        if (!visible || !expanded || original.owner !== controller || original.archive !== archive || original.epoch !== storeEpoch
                || original.task !== selectedTaskId || original.episodeToken !== episodeToken || original.draftToken !== draftToken
                || ((action === "inspect" || action === "create") && !bindingsEqual(original.binding, selectedBinding))) {
            dispatchBusy = false
            if (guard.alive) message = qsTr("发起前来源已变化；本次没有调用恢复操作，请重新选择并确认。")
            return
        }
        try {
            if (action === "list") result = original.archive.historyVersions(original.task, original.epoch) || {}
            else if (action === "inspect") result = original.owner.inspectFleetMissionArchiveDraft(original.binding, original.draftToken) || {}
            else if (action === "create") result = original.owner.createFleetMissionArchiveDraft(original.sourceToken, original.replace, true) || {}
            else if (action === "restore") result = original.owner.restorePreviousFleetMissionDraft(original.draftToken, true) || {}
            else result = {ok: original.owner.cancelFleetMissionArchiveDraftPreview(original.sourceToken) === true}
        } catch (error) { result = {error: "archive_draft_call_interrupted"} }
        if (!guard.alive) return
        dispatchBusy = false
        if (!guard.alive) return
        if (!visible || !expanded || original.owner !== controller || original.archive !== archive || original.epoch !== storeEpoch
                || original.episodeToken !== episodeToken || original.task !== selectedTaskId) {
            message = qsTr("返回时来源已变化；请核对当前草稿，不能把中断当作未发生修改。"); return
        }
        if (action === "list") {
            if (!versionResultKnown(result, original.task, original.epoch)) {
                message = qsTr("未取得受支持的历史版本目录：%1。").arg(errorText(result.error)); return
            }
            clearHistorySelection(); versionOwner = original.archive; versionSnapshot = result
            message = qsTr("已列出本实例已加载的该来源全部保留版本；不会自动选择最新版本，也没有重新读取磁盘。"); return
        }
        if (action === "cancel" && result.ok === true) { message = qsTr("已清除本地来源预览；当前草稿与档案不变。"); return }
        var accepted = result.ok === true && result.current === true && result.flightCommandReleased === false && result.executable === false
            && result.draftToken === draftToken && (action === "inspect" || result.committed === true)
        if (!accepted) {
            message = qsTr("未完整确认：%1。已发布草稿：%2；候选装入：%3；备用坐标确认已改变：%4。请核对当前草稿；不自动上传或回滚。")
                .arg(errorText(result.error)).arg(fact(result.committed)).arg(fact(result.candidateInitialized)).arg(fact(result.backupChanged)); return
        }
        message = action === "inspect" ? qsTr("已读取所选历史版本的有效规划来源。它不是部署、机载内容或当前设备证据；尚未创建草稿。")
            : qsTr("已%1，原当前草稿保留为共享备用。需重新确认地图基准、重新审阅与审批；没有继承历史任务 ID 或批准，也没有上传 / 启动。审计追加：%2。")
                .arg(action === "restore" ? qsTr("切回备用内容") : qsTr("恢复为独立可编辑副本"))
                .arg(result.auditRecorded === true ? qsTr("已记录") : qsTr("未确认；不撤销已发生的本地修改"))
    }
    function errorText(value) { return typeof value === "string" && value.length ? value.substring(0, 180) : qsTr("未知原因") }
    function fact(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function selectVersion(row, owner, epoch, originalInteraction) {
        if (!localReady || owner !== archive || epoch !== storeEpoch || originalInteraction !== interactionEpoch || !versionsCurrent) return
        var binding = bindingOf(row), found = false
        for (var i = 0; i < versions.length; ++i) if (bindingsEqual(binding, bindingOf(versions[i]))) found = true
        if (!found) return
        clearConfirmations(); selectedBinding = binding; selectedSourcePresent = row.planningSourcePresent; message = ""
    }
    onControllerChanged: { selectedTaskId = ""; clearHistorySelection() }
    onStoreEpochChanged: { selectedTaskId = ""; clearHistorySelection() }
    onEpisodeTokenChanged: clearConfirmations()
    onDraftTokenChanged: clearConfirmations()
    onSourceTokenChanged: clearConfirmations()
    onVisibleChanged: if (!visible) clearConfirmations()
    onExpandedChanged: if (!expanded) clearConfirmations()
    Component.onDestruction: lifetime.alive = false

    ColumnLayout {
        id: content
        width: root.width; spacing: root.pad
        QGCLabel { Layout.fillWidth: true; text: qsTr("历史规划版本 · 恢复独立本地草稿"); textFormat: Text.PlainText; font.bold: true; wrapMode: Text.WordWrap }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.expanded ? qsTr("收起历史草稿恢复") : qsTr("展开历史版本选择（不自动读取）")
            enabled: !root.dispatchBusy
            onClicked: root.expanded = !root.expanded
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("来源是本实例已加载的本地档案历史快照，不是磁盘实时最新、部署记录、机载内容或权限证据。选择具体旧版本即可恢复，不要求失败部署、已连接飞机或账号。源任务 ID 只作追溯；新审阅使用当前控制器自身的任务 ID，不承诺每次恢复另建一个任务 ID。")
            }
            QGCLabel { Layout.fillWidth: true; text: qsTr("1 · 选择已归档来源，再明确列出该来源历史版本"); wrapMode: Text.WordWrap }
            Controls.ComboBox {
                id: sourceCombo
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                model: root.tasks; textRole: "label"; currentIndex: root.taskIndex()
                enabled: root.localReady && root.tasks.length > 0
                Accessible.name: qsTr("历史规划来源")
                onActivated: if (index >= 0 && index < root.tasks.length) root.selectSource(root.tasks[index].sourceTaskId)
                contentItem: QGCLabel {
                    text: sourceCombo.displayText; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    rightPadding: sourceCombo.indicator ? sourceCombo.indicator.width : 0
                    verticalAlignment: Text.AlignVCenter
                }
                delegate: Controls.ItemDelegate {
                    id: sourceChoice
                    width: sourceCombo.width; text: modelData.label
                    highlighted: sourceCombo.highlightedIndex === index
                    contentItem: QGCLabel { text: sourceChoice.text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                }
            }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.selectedTaskId ? qsTr("所选来源：%1").arg(root.selectedTaskId) : qsTr("尚未选择来源；没有记录不代表不存在任务或可以执行。") }
            QGCButton {
                id: listButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("列出该来源的全部保留版本（只读）"); enabled: root.canList
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("list")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: listButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: listButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: root.versionsCurrent ? qsTr("该来源保留 %1 版（上限128）；本页最多32条，第 %2 页。倒序列出具体版本，不将最新版本作为默认选择；保留范围不是全历史完整性证明。").arg(root.versions.length).arg(root.page + 1)
                    : qsTr("尚未取得本来源版本目录。")
            }
            Repeater {
                model: root.versions.slice(root.page * 32, root.page * 32 + 32)
                delegate: QGCButton {
                    id: versionButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("%1版本 %2 · %3\n记录 %4\n规划源字段：%5（存在仍须校验） · 跨重启载入：%6")
                        .arg(root.bindingsEqual(root.selectedBinding, root.bindingOf(modelData)) ? qsTr("已选 · ") : "")
                        .arg(modelData.recordRevision).arg(modelData.recordedAtUtc).arg(modelData.recordId)
                        .arg(modelData.planningSourcePresent ? qsTr("存在") : qsTr("缺失，不能恢复"))
                        .arg(root.fact(modelData.restoredFromArchive))
                    enabled: root.localReady
                    property var pressedRow: null
                    onPressed: pressedRow = {row: modelData, owner: root.archive, epoch: root.storeEpoch, interactionEpoch: root.interactionEpoch}
                    onCanceled: pressedRow = null
                    onClicked: { var saved = pressedRow; pressedRow = null; if (saved) root.selectVersion(saved.row, saved.owner, saved.epoch, saved.interactionEpoch) }
                    contentItem: QGCLabel { text: versionButton.text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: versionButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("上一页版本"); enabled: root.localReady && root.page > 0; onClicked: { root.clearConfirmations(); root.page -= 1 } }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("下一页版本"); enabled: root.localReady && (root.page + 1) * 32 < root.versions.length; onClicked: { root.clearConfirmations(); root.page += 1 } }
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: root.selectionCurrent ? qsTr("所选版本 %1 · 记录 %2\n内容哈希 %3\n%4")
                    .arg(root.selectedBinding.recordRevision).arg(root.selectedBinding.recordId).arg(root.selectedBinding.contentHash)
                    .arg(root.selectedSourcePresent ? qsTr("可申请检查原规划来源，字段存在不是可恢复证明。") : qsTr("该旧版本缺少原规划来源，不能恢复；不会从编译航线反推区域。"))
                    : qsTr("2 · 请明确选择一个具体历史版本。")
            }
            QGCButton {
                id: inspectButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("检查所选版本的规划来源（不改草稿）"); enabled: root.canInspect
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("inspect")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: inspectButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: inspectButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: root.sourceCurrent ? qsTr("已检查历史快照：原规划修订 %1，区域顶点 %2，保留成员 %3。恢复只复制可编辑意图，不继承原 fleet、审阅证据、任务 ID 或批准。")
                    .arg(root.draftState.source.sourceRevision).arg(root.draftState.source.vertexCount).arg(root.draftState.source.memberCount)
                    : qsTr("尚无与所选版本匹配的有效来源预览；不会自动恢复。")
            }
            Controls.CheckBox {
                id: createConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: false; enabled: root.canCreate
                text: qsTr("确认恢复为独立可编辑副本；保留当前草稿、撤销旧审阅 / 审批，并重新确认地图基准")
                contentItem: QGCLabel { text: createConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (createConsent.indicator ? createConsent.indicator.width : 0) + root.pad }
            }
            Controls.CheckBox {
                id: replaceConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: false; visible: root.draftState.hasPrevious === true; enabled: root.canCreate
                text: qsTr("确认成功发布后替换更早的共享备用：仅保留本次操作前的当前草稿，旧备用将无法从此入口切回")
                contentItem: QGCLabel { text: replaceConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (replaceConsent.indicator ? replaceConsent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                id: createButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("恢复所选历史版本为可编辑副本（不上传）")
                enabled: root.canCreate && createConsent.checked && (root.draftState.hasPrevious !== true || replaceConsent.checked)
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("create")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: createButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: createButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCButton {
                id: cancelButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("取消历史来源预览（不改草稿）"); enabled: root.localReady && !!root.sourceToken
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("cancel")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: cancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: cancelButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("与失败部署修复入口共用一份本地备用，当前%1。备用只保留在本次应用内存中。切回内容仍需重新确认地图基准及审阅，不恢复旧权限；本地草稿不等于已经修复或可执行。")
                    .arg(root.draftState.hasPrevious === true ? qsTr("有可切回备用") : qsTr("没有备用"))
            }
            Controls.CheckBox {
                id: restoreConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: false; enabled: root.canRestore
                text: qsTr("确认切回共享备用内容，保留当前草稿为备用；重新确认坐标基准和审阅，不上传 / 启动")
                contentItem: QGCLabel { text: restoreConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (restoreConsent.indicator ? restoreConsent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                id: restoreButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("切回共享备用草稿"); enabled: root.canRestore && restoreConsent.checked
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("restore")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: restoreButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: restoreButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: !root.inputSettled ? qsTr("请先保存或放弃未提交输入 / 地图选点，不能把它们静默丢弃。")
                    : root.dispatchBusy || root.draftState.busy === true ? qsTr("正在处理明确发起的本地操作。")
                    : root.trafficBusy ? qsTr("实际任务读取、上传或协议排空仍忙；本地切换不会释放这些占用。")
                    : root.draftState.destructionPending === true ? qsTr("旧本地对象尚待实际销毁；暂不允许再次创建，仍可切回备用。")
                    : root.archive && root.archive.writeBlocked === true ? qsTr("档案写入当前被阻断：历史副本仍可编辑，但后续新审阅可能无法归档。不会自动修复磁盘或重载文件。")
                    : qsTr("只操作已加载历史和本地草稿；仍须重新审阅、重新审批，后续飞行放行留给工程师。")
            }
            QGCLabel { Layout.fillWidth: true; visible: root.message.length > 0; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.message; Accessible.name: text }
        }
    }
}

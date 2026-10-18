import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Explicit, inert single-member or complete-Run planning archive. Imported
// directory rows are never live Run objects or automatic member selections.
FocusScope {
    id: root
    property var controller: null
    property bool inputSettled: false
    property bool nativeCaptureSelected: false
    property bool allowLoad: false
    property bool allowUse: false
    property bool expanded: false
    property bool callBusy: false
    property var callTicket: null
    property var fileDialog: null
    property var saveIntent: null
    property var lifetime: ({alive: true})
    property double interactionEpoch: 0
    property double contextEpoch: 0
    property int selectedVehicleId: 0
    property double memberSelectionEpoch: 0
    property int memberPage: 0
    property var feedbackState: ({})
    signal captureAdopted(string token)
    signal focusVisibilityRequested(var item)

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var archiveState: controller ? controller.fleetMissionRemainderArchive || {} : ({})
    readonly property var archive: archiveState.archive || ({})
    readonly property string archiveToken: typeof archive.token === "string" ? archive.token : ""
    readonly property string captureToken: typeof archiveState.captureToken === "string" ? archiveState.captureToken : ""
    readonly property string collectionSourceToken: archiveState.collectionSourceToken || ""
    readonly property string collectionRunId: archiveState.collectionRunId || ""
    readonly property var members: archive.members && archive.members.length <= 32 ? archive.members : []
    readonly property var selectedMember: memberForId(selectedVehicleId)
    readonly property int memberPageCount: Math.max(1, Math.ceil(members.length / 8))
    readonly property int currentMemberPage: Math.max(0, Math.min(memberPage, memberPageCount - 1))
    readonly property string episodeToken: controller && controller.fleetDeploymentLifecycle
        ? controller.fleetDeploymentLifecycle.currentToken || "" : ""
    readonly property bool pendingInput: callBusy || !!fileDialog || !!saveIntent || useConsent.checked
    readonly property bool ready: visible && expanded && controller && inputSettled && !callBusy && !callTicket
        && !fileDialog && archiveState.busy !== true && interactionEpoch < 9007199254740991 && contextEpoch < 9007199254740991
        && memberSelectionEpoch < 9007199254740991
    readonly property bool saveCurrent: !!saveIntent && saveIntent.owner === controller && saveIntent.contextEpoch === contextEpoch
        && saveIntent.episodeToken === episodeToken && saveIntent.archiveToken === archiveToken && !!archiveToken
    readonly property bool feedbackCurrent: feedbackState.owner === controller && feedbackState.contextEpoch === contextEpoch
        && feedbackState.episodeToken === episodeToken && visible && expanded && inputSettled
        && (feedbackState.action !== "use" || (feedbackState.sourceVehicleId === selectedVehicleId
            && feedbackState.memberSelectionEpoch === memberSelectionEpoch))
    readonly property string message: !feedbackState.text ? "" : feedbackCurrent ? feedbackState.text
        : feedbackState.committed === true ? qsTr("原操作的本地提交已发生，但界面上下文已变化；不代表当前来源通过，不要自动重试。")
        : feedbackState.fileWriteAttempted === true ? qsTr("原保存调用已尝试，结果仍未确认；请核对所选路径，不能当作未写入或已回滚。")
        : qsTr("原操作结果不属于当前界面上下文，请重新核对来源。")
    readonly property bool messageError: !!feedbackState.text && (feedbackState.error === true || !feedbackCurrent)
    implicitHeight: body.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function memberForId(id) {
        for (var i = 0; i < members.length; ++i) if (members[i].vehicleId === id) return members[i]
        return null
    }
    function memberIntent(id) {
        return ready && allowUse && archiveToken && memberForId(id)
            ? {owner: controller, epoch: interactionEpoch, contextEpoch: contextEpoch, episodeToken: episodeToken,
                archiveToken: archiveToken, sourceVehicleId: id} : null
    }
    function selectMember(value) {
        if (!ready || !allowUse || !contextCurrent(value) || value.epoch !== interactionEpoch
                || value.archiveToken !== archiveToken || !memberForId(value.sourceVehicleId)) return
        // Even a sole-member legacy file requires a new human choice. The
        // selection's change handler retires any previous write/use intent.
        selectedVehicleId = value.sourceVehicleId
    }
    function clearConfirmations() {
        var guard = lifetime
        saveIntent = null
        if (!guard.alive) return
        writeConsent.checked = false
        if (!guard.alive) return
        useConsent.checked = false
    }
    function retireIntents() {
        var guard = lifetime
        if (interactionEpoch < 9007199254740991) interactionEpoch += 1
        if (guard.alive) clearConfirmations()
    }
    function detachDialog(view) {
        if (!view) return
        var guard = lifetime
        view.activeIntent = false
        if (!guard.alive) return
        if (fileDialog === view) fileDialog = null
        if (!guard.alive) return
        try { view.close() } catch (error) { /* Intent is already retired. */ }
        if (guard.alive) view.destroy()
    }
    function retireContext() {
        var guard = lifetime
        if (contextEpoch < 9007199254740991) contextEpoch += 1
        if (!guard.alive) return
        retireIntents()
        if (guard.alive && fileDialog) detachDialog(fileDialog)
    }
    function actionReady(action) {
        if (action === "prepare") return nativeCaptureSelected && archiveState.canPrepare === true && !!captureToken
        if (action === "prepare-collection") return allowLoad && archiveState.canPrepareCollection === true && !!collectionSourceToken && !!collectionRunId
        if (action === "pick-load" || action === "load") return allowLoad
        if (action === "pick-save") return !!archiveToken
        if (action === "write") return saveCurrent && writeConsent.checked
        if (action === "use") return allowUse && !!archiveToken && !!selectedMember && selectedMember.available === true
            && selectedMember.waypointCount > 0 && useConsent.checked
        return action === "clear" && allowLoad && !!archiveToken
    }
    function intent(action) {
        return ready && actionReady(action) ? {action: action, owner: controller, epoch: interactionEpoch,
            contextEpoch: contextEpoch, episodeToken: episodeToken, archiveToken: archiveToken,
            captureToken: captureToken, sourceVehicleId: selectedVehicleId,
            memberSelectionEpoch: memberSelectionEpoch,
            collectionSourceToken: collectionSourceToken, collectionRunId: collectionRunId,
            filePath: action === "write" && saveCurrent ? saveIntent.filePath : ""} : null
    }
    function contextCurrent(value) {
        return value && value.owner === controller && value.contextEpoch === contextEpoch && value.episodeToken === episodeToken
            && visible && expanded && inputSettled && contextEpoch < 9007199254740991
    }
    function intentCurrent(value) {
        return contextCurrent(value) && value.epoch === interactionEpoch && value.archiveToken === archiveToken
            && value.captureToken === captureToken && archiveState.busy !== true && actionReady(value.action)
            && (value.action !== "use" || (value.sourceVehicleId === selectedVehicleId && value.memberSelectionEpoch === memberSelectionEpoch))
            && (value.action !== "prepare-collection" || (value.collectionSourceToken === collectionSourceToken && value.collectionRunId === collectionRunId))
            && (value.action !== "write" || value.filePath === saveIntent.filePath)
    }
    function publish(value, text, error, committed, attempted) {
        feedbackState = {owner: value.owner, contextEpoch: value.contextEpoch, episodeToken: value.episodeToken,
            action: value.action, sourceVehicleId: value.sourceVehicleId, memberSelectionEpoch: value.memberSelectionEpoch,
            text: text, error: error, committed: committed, fileWriteAttempted: attempted}
    }
    function openPicker(value, save) {
        if (!ready || !intentCurrent(value)) return
        var guard = lifetime
        clearConfirmations()
        if (!guard.alive || !ready || !intentCurrent(value)) return
        var view = (save ? saveDialogComponent : loadDialogComponent).createObject(root,
            {ownerPanel: root, guard: guard, frozenIntent: value})
        if (!guard.alive) return
        if (!view) { publish(value, qsTr("文件窗口未创建；尚未调用资料包读写。"), true, false, false); return }
        fileDialog = view
        if (!guard.alive) return
        if (fileDialog !== view || !intentCurrent(value)) { detachDialog(view); return }
        try { if (save) view.openForSave(); else view.openForLoad() }
        catch (error) {
            if (!guard.alive) return
            if (fileDialog === view) detachDialog(view)
            if (guard.alive) publish(value, qsTr("文件选择中断；没有自动重试，请重新选择。"), true, false, false)
        }
    }
    function fileChosen(view, value, filePath, save) {
        if (fileDialog !== view || !view.activeIntent) return
        var guard = lifetime
        var accepted = intentCurrent(value) && !callBusy && typeof filePath === "string" && filePath.length > 0 && filePath.length <= 4096
        detachDialog(view)
        if (!guard.alive) return
        if (!accepted || !intentCurrent(value) || !ready) {
            publish(value, qsTr("文件选择期间来源或输入已变化；旧路径已废弃，请重新选择。"), true, false, false)
            return
        }
        if (save) {
            saveIntent = {owner: value.owner, contextEpoch: value.contextEpoch, episodeToken: value.episodeToken,
                archiveToken: value.archiveToken, filePath: filePath}
            if (!guard.alive) return
            writeConsent.checked = false
            if (guard.alive) publish(value, qsTr("路径已选择，尚未写文件；请核对资料包和路径后单独确认覆盖。"), false, false, false)
        } else {
            value.action = "load"; value.filePath = filePath
            perform(value)
        }
    }
    function fileRejected(view) {
        if (fileDialog !== view) return
        var guard = lifetime, value = view.frozenIntent
        detachDialog(view)
        if (guard.alive) publish(value, qsTr("已取消文件选择；没有调用资料包读写。"), false, false, false)
    }
    function perform(value) {
        if (!ready || !intentCurrent(value)) return
        var guard = lifetime, ticket = {}, result = null, threw = false
        callTicket = ticket
        if (!guard.alive) return
        callBusy = true
        if (!guard.alive) return
        try {
            if (!intentCurrent(value) || callTicket !== ticket) result = {ok: false, error: "remainder_archive_intent_changed"}
            else if (value.action === "prepare") result = value.owner.prepareFleetMissionRemainderArchive(value.captureToken)
            else if (value.action === "prepare-collection") result = value.owner.prepareFleetMissionRemainderCollectionArchive(value.collectionSourceToken, value.collectionRunId)
            else if (value.action === "load") result = value.owner.loadFleetMissionRemainderArchive(value.filePath, value.archiveToken)
            else if (value.action === "write") result = value.owner.saveFleetMissionRemainderArchive(value.filePath, value.archiveToken, true)
            else if (value.action === "use") result = value.owner.captureFleetMissionRemainderArchiveMember(value.archiveToken, value.sourceVehicleId, true)
            else if (value.action === "clear") result = value.owner.clearFleetMissionRemainderArchive(value.archiveToken)
        } catch (error) { threw = true }
        if (!guard.alive) return
        var committed = !!result && result.committed === true, fileCommitted = result ? result.fileCommitted : null
        var attempted = !!result && result.fileWriteAttempted === true
        var succeeded = !threw && result && result.ok === true && result.current === true
            && (value.action !== "use" || (result.sourceVehicleId === value.sourceVehicleId
                && value.sourceVehicleId === selectedVehicleId && value.memberSelectionEpoch === memberSelectionEpoch && value.archiveToken === archiveToken))
        var selectedToken = result && typeof result.captureToken === "string" ? result.captureToken : ""
        var errorText = result && typeof result.error === "string" && result.error ? result.error : "remainder_archive_result_unknown"
        var text = value.action === "write"
            ? fileCommitted === true ? qsTr("资料包文件已提交。%1").arg(succeeded ? qsTr("完整读回一致；仍是未认证历史资料，不是飞行许可。") : qsTr("后续校验或当前性未确认；请核对文件，不要自动重试。"))
                : threw || fileCommitted === null ? qsTr("保存是否提交未确认；文件可能已变化。请核对路径，不自动重试或声称回滚。")
                : qsTr("未确认保存成功：%1；已尝试写入：%2。请核对路径再决定是否重试。").arg(errorText).arg(attempted ? qsTr("是") : qsTr("否"))
            : succeeded ? value.action === "prepare" || value.action === "prepare-collection" ? qsTr("完整资料包已冻结在本页，尚未写文件。请明确选择目录中的原成员；包哈希不是认证。")
                : value.action === "load" ? qsTr("完整历史资料已打开，但尚未选用；不会恢复 Run、批准、占用或已完成航段。")
                : value.action === "use" ? qsTr("已选用原成员 %1 的历史来源；请在原余段编辑区重新核对输入，再生成新提案并重新审阅。").arg(value.sourceVehicleId)
                : qsTr("已清除本页资料预览；未删除文件或改变 Run。")
            : qsTr("未完成：%1；本地提交已发生：%2。输入保留，请核对当前状态。").arg(errorText).arg(committed ? qsTr("是") : qsTr("否"))
        // Keep the ticket until confirmations and feedback are retired;
        // busy=false notification is not permission to repeat a write/use.
        clearConfirmations()
        if (!guard.alive) return
        callBusy = false
        if (!guard.alive) return
        publish(value, text, !succeeded, committed, attempted || (value.action === "write" && threw))
        if (!guard.alive) return
        if (messageError && contextCurrent(value)) {
            feedback.forceActiveFocus()
            if (guard.alive && contextCurrent(value)) focusVisibilityRequested(feedback)
        }
        if (!guard.alive) return
        if (callTicket === ticket) callTicket = null
        if (guard.alive && succeeded && value.action === "use" && selectedToken && contextCurrent(value)
                && value.sourceVehicleId === selectedVehicleId && value.memberSelectionEpoch === memberSelectionEpoch
                && value.archiveToken === archiveToken) captureAdopted(selectedToken)
    }

    onControllerChanged: retireContext()
    onEpisodeTokenChanged: retireContext()
    onInputSettledChanged: retireContext()
    onVisibleChanged: retireContext()
    onExpandedChanged: retireContext()
    onArchiveTokenChanged: {
        var guard = lifetime
        retireIntents()
        if (!guard.alive) return
        selectedVehicleId = 0
        if (!guard.alive) return
        memberPage = 0
        if (guard.alive && fileDialog) detachDialog(fileDialog)
    }
    onSelectedVehicleIdChanged: {
        var guard = lifetime
        if (memberSelectionEpoch < 9007199254740991) memberSelectionEpoch += 1
        if (guard.alive) retireIntents()
    }
    onCollectionSourceTokenChanged: retireIntents()
    onCollectionRunIdChanged: retireIntents()
    onCaptureTokenChanged: retireIntents()
    onAllowUseChanged: clearConfirmations()
    onAllowLoadChanged: { clearConfirmations(); if (lifetime.alive && !allowLoad && fileDialog) detachDialog(fileDialog) }
    Component.onDestruction: { lifetime.alive = false }

    ColumnLayout {
        id: body
        width: root.width; spacing: root.pad
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.expanded ? qsTr("收起完整历史重规划资料包") : qsTr("展开完整历史重规划资料包 · 保存 / 打开")
            enabled: !root.callBusy
            onClicked: root.expanded = !root.expanded
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("可保存同一实际 Run 的完整成员资料，或兼容旧单成员资料包。整组资料只在原对象仍存在时明确捕获，保留未提交成员但禁止选用；没有把其它成员的路线补造成执行证据。打开只替换本页目录，必须另选成员并确认；不是自动续飞、多机联合改派或原任务恢复。")
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: root.archiveToken ? qsTr("资料包 %1 · 格式 v%2\n原 Run %3 · 保存成员 %4\n原计划 %5\n捕获时刻 %6\n包 SHA-256 %7\n来源：%8。未认证，只供新人工重规划。")
                    .arg(root.archive.archiveId).arg(root.archive.schemaVersion).arg(root.archive.runId).arg(root.members.length)
                    .arg(root.archive.planId).arg(root.archive.capturedAtUtc).arg(root.archive.contentHash)
                    .arg(root.archive.origin === "imported" ? qsTr("文件导入") : qsTr("当次原生捕获"))
                    : qsTr("本页尚无完整资料包。可从上方选中的实际 Run 捕获整组，或先读取单成员来源再冻结旧格式。打开文件不要求原飞机在线；旧摘要缺失的资料不能补造。")
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: qsTr("当前可捕获的实际 Run：%1。整组取源会替换本页旧预览，但不关闭 Run 或释放占用；不会自动写文件。").arg(root.collectionRunId || qsTr("未选择"))
            }
            QGCButton {
                id: prepareCollectionButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("明确捕获此 Run 全部成员资料（尚不写文件）")
                enabled: root.ready && root.actionReady("prepare-collection")
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("prepare-collection")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
                contentItem: QGCLabel { text: prepareCollectionButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: prepareCollectionButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCButton {
                id: prepareButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("仅冻结当前单成员来源（兼容旧格式，尚不写文件）"); enabled: root.ready && root.nativeCaptureSelected && root.archiveState.canPrepare === true
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("prepare")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
                contentItem: QGCLabel { text: prepareButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: prepareButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCButton {
                id: loadButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("打开完整资料包（替换本页预览，不选用）"); enabled: root.ready && root.allowLoad
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("pick-load")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.openPicker(value, false) }
                contentItem: QGCLabel { text: loadButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: loadButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCButton {
                id: saveButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("选择资料包保存路径（尚不写文件）"); enabled: root.ready && !!root.archiveToken
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("pick-save")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.openPicker(value, true) }
                contentItem: QGCLabel { text: saveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: saveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                Layout.fillWidth: true; visible: root.saveCurrent; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: root.saveCurrent ? qsTr("待写资料包 %1\n路径：%2\n确认后可能覆盖同名文件；取消不删除文件。").arg(root.archive.archiveId).arg(root.saveIntent.filePath) : ""
            }
            Controls.CheckBox {
                id: writeConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                visible: root.saveCurrent; enabled: root.ready && root.saveCurrent; checked: false
                text: qsTr("确认把上述完整历史资料包写到该路径，并接受覆盖同名文件")
                contentItem: QGCLabel { text: writeConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (writeConsent.indicator ? writeConsent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                id: writeButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("确认写入并完整读回"); enabled: root.ready && root.saveCurrent && writeConsent.checked
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("write")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
                contentItem: QGCLabel { text: writeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: writeButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                Layout.fillWidth: true; visible: !!root.archiveToken; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("完整成员目录 · 第 %1 / %2 页。MODE 提交只决定历史取源资格，不证明任务完成；即使只有一项也必须明确选择。未提交或无独立出航航点的成员不能用于此余段编辑器。").arg(root.currentMemberPage + 1).arg(root.memberPageCount)
            }
            Repeater {
                model: root.members.slice(root.currentMemberPage * 8, root.currentMemberPage * 8 + 8)
                QGCButton {
                    id: memberButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("%1原成员 %2 · 出航点 %3 · MODE 已入队 %4 / 已提交 %5\n%6")
                        .arg(root.selectedVehicleId === modelData.vehicleId ? qsTr("已选择 · ") : "")
                        .arg(modelData.vehicleId).arg(modelData.waypointCount)
                        .arg(modelData.modeQueueBegan === true ? qsTr("是") : modelData.modeQueueBegan === false ? qsTr("否") : qsTr("未知（旧格式）"))
                        .arg(modelData.modeQueueCommitted ? qsTr("是") : qsTr("否"))
                        .arg(modelData.available !== true ? qsTr("不可选用：%1").arg(modelData.reason)
                            : modelData.waypointCount < 1 ? qsTr("资料已保留，但当前编辑器不支持无独立出航航点的成员") : qsTr("可明确选择，再单独确认用于新人工任务"))
                    enabled: root.ready && root.allowUse
                    property var pressedIntent: null
                    onPressed: pressedIntent = root.memberIntent(modelData.vehicleId)
                    onCanceled: pressedIntent = null
                    onClicked: { var value = pressedIntent; pressedIntent = null; root.selectMember(value) }
                    onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
                    Accessible.name: text
                    contentItem: QGCLabel { text: memberButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter; color: memberButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                }
            }
            RowLayout {
                Layout.fillWidth: true; visible: root.memberPageCount > 1
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("成员上一页"); enabled: root.ready && root.currentMemberPage > 0; onClicked: root.memberPage = root.currentMemberPage - 1 }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("成员下一页"); enabled: root.ready && root.currentMemberPage + 1 < root.memberPageCount; onClicked: root.memberPage = root.currentMemberPage + 1 }
            }
            QGCLabel {
                Layout.fillWidth: true; visible: !!root.archiveToken; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: root.selectedMember ? qsTr("明确选择：原成员 %1。%2").arg(root.selectedVehicleId)
                    .arg(root.selectedMember.available !== true ? qsTr("尚未真实提交，禁止选用。") : root.selectedMember.waypointCount < 1
                        ? qsTr("当前余段编辑器不支持该路线，未创建来源。") : qsTr("下方确认只针对该成员，不会同时改派整组。"))
                    : qsTr("尚未明确选择原成员；没有默认第一项，也不会沿用上一份包的同编号成员。")
            }
            Controls.CheckBox {
                id: useConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                enabled: root.ready && root.allowUse && !!root.archiveToken && !!root.selectedMember
                    && root.selectedMember.available === true && root.selectedMember.waypointCount > 0; checked: false
                text: qsTr("确认仅把明确选择的原成员 %1 的未认证历史资料用于新的人工重规划；撤销旧本地审阅，不恢复原任务、批准、控制权或已完成航段").arg(root.selectedVehicleId || qsTr("未选择"))
                contentItem: QGCLabel { text: useConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (useConsent.indicator ? useConsent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                id: useButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("明确选用所选成员的历史来源（仍需新提案 / 新审阅）"); enabled: root.ready && root.actionReady("use")
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("use")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
                contentItem: QGCLabel { text: useButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: useButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCButton {
                id: clearButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("清除本页资料预览（不删除文件）"); enabled: root.ready && root.allowLoad && !!root.archiveToken
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("clear")
                onCanceled: pressedIntent = null
                onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
                contentItem: QGCLabel { text: clearButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: clearButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                id: feedback
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                activeFocusOnTab: true; Accessible.name: text
                text: root.message || qsTr("输入文字和新执行配置不会由旧包自动填写。仅一份有界资料预览；无自动保存、自动恢复或跨站控制权转移。尚未编译或实测。")
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
            selectExisting: true; selectFolder: false; folder: ""
            title: qsTr("打开完整历史重规划资料包")
            nameFilters: [qsTr("完整重规划资料包 (*.json)")]
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
            selectExisting: false; selectFolder: false; folder: ""
            title: qsTr("选择完整资料包路径 · 返回后仍需确认")
            nameFilters: [qsTr("完整重规划资料包 (*.json)")]
            onAcceptedForSave: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.fileChosen(saveView, frozenIntent, file, true)
            onRejected: if (guard && guard.alive && activeIntent && ownerPanel) ownerPanel.fileRejected(saveView)
        }
    }
}

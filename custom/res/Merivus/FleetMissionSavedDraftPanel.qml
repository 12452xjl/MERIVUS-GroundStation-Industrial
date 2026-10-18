import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Explicit local editing checkpoints. Expanding never reads/writes disk;
// incomplete canonical drafts are allowed, uncommitted field text is not.
FocusScope {
    id: root
    property var controller: null
    property bool inputSettled: false
    property bool expanded: false
    property bool dispatchBusy: false
    property double interactionEpoch: 0
    property double contextEpoch: 0
    property var lifetime: ({alive: true})
    property var feedbackState: ({})
    readonly property bool feedbackCurrent: feedbackState.owner === controller && feedbackState.contextEpoch === contextEpoch
        && feedbackState.episodeToken === episodeToken && visible && expanded && inputSettled
    readonly property string message: !feedbackState.text ? "" : feedbackCurrent ? feedbackState.text
        : feedbackState.committed === true ? qsTr("原操作的%1已提交，但面板上下文已改变；不代表当前来源通过，请重新检查，不要自动重试。").arg(feedbackState.effect)
        : qsTr("原操作结果不再属于当前面板上下文；当前来源需要重新检查。")
    readonly property bool messageError: !!feedbackState.text && (feedbackState.error === true || !feedbackCurrent)
    signal focusVisibilityRequested(var item)

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.4)
    readonly property var draftState: controller && controller.fleetMissionSavedDraft ? controller.fleetMissionSavedDraft : ({})
    readonly property var checkpoint: draftState.store || ({})
    readonly property string draftToken: typeof draftState.draftToken === "string" ? draftState.draftToken : ""
    readonly property string saveToken: typeof draftState.saveToken === "string" ? draftState.saveToken : ""
    readonly property string sourceToken: typeof draftState.sourceToken === "string" ? draftState.sourceToken : ""
    readonly property string nativeToken: typeof checkpoint.previewToken === "string" ? checkpoint.previewToken : ""
    readonly property string episodeToken: controller && controller.fleetDeploymentLifecycle
        && typeof controller.fleetDeploymentLifecycle.currentToken === "string" ? controller.fleetDeploymentLifecycle.currentToken : ""
    readonly property bool localReady: visible && expanded && controller && inputSettled && !dispatchBusy
        && draftState.busy === false && !!draftToken && !!episodeToken && interactionEpoch < 9007199254740991
        && contextEpoch < 9007199254740991
    readonly property bool canSave: localReady && draftState.saveCurrent === true && !!saveToken
    readonly property bool canCreate: localReady && draftState.sourceCurrent === true && !!sourceToken && draftState.destructionPending === false

    implicitHeight: content.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function clearConfirmations() {
        var guard = lifetime
        if (saveConsent) saveConsent.checked = false
        if (!guard.alive) return
        if (createConsent) createConsent.checked = false
        if (!guard.alive) return
        if (replaceConsent) replaceConsent.checked = false
        if (!guard.alive) return
        if (undoConsent) undoConsent.checked = false
        if (!guard.alive) return
        if (interactionEpoch < 9007199254740991) interactionEpoch += 1
    }

    function retireContext() {
        var guard = lifetime
        if (contextEpoch < 9007199254740991) contextEpoch += 1
        if (guard.alive) clearConfirmations()
    }

    function actionReady(action) {
        if (action === "prepare" || action === "inspect") return true
        if (action === "save") return draftState.saveCurrent === true && !!saveToken && saveConsent.checked
        if (action === "create") return draftState.sourceCurrent === true && !!sourceToken && draftState.destructionPending === false
            && createConsent.checked && (draftState.hasPrevious !== true || replaceConsent.checked)
        if (action === "undo") return draftState.hasPrevious === true && undoConsent.checked
        return action === "cancel" && (!!saveToken || !!sourceToken)
    }

    function allowed(action) { return localReady && actionReady(action) }

    function contextCurrent(original) {
        return original && original.owner === controller && original.contextEpoch === contextEpoch
            && original.episodeToken === episodeToken && visible && expanded && inputSettled
            && contextEpoch < 9007199254740991
    }

    function intentCurrent(original) {
        return contextCurrent(original) && original.draftToken === draftToken && original.saveToken === saveToken
            && original.sourceToken === sourceToken && original.nativeToken === nativeToken && original.epoch === interactionEpoch
            && draftState.busy === false && !!draftToken && !!episodeToken && interactionEpoch < 9007199254740991
            && (original.action !== "create" || original.replace === replaceConsent.checked) && actionReady(original.action)
    }

    function intent(action) {
        return allowed(action) ? {action: action, owner: controller, draftToken: draftToken, saveToken: saveToken,
            sourceToken: sourceToken, nativeToken: nativeToken, episodeToken: episodeToken,
            replace: replaceConsent.checked, epoch: interactionEpoch, contextEpoch: contextEpoch} : null
    }

    function dispatch(original) {
        if (!intentCurrent(original) || !localReady) return
        var guard = lifetime, result = null, owner = controller
        dispatchBusy = true
        if (!guard.alive) return
        try {
            // Setting busy emits notifications: do not send a captured request
            // if those callbacks changed its owner, source, input or consent.
            if (!intentCurrent(original)) result = {ok: false, error: "editing_checkpoint_intent_changed"}
            else if (original.action === "prepare") result = owner.prepareFleetMissionDraftSave(original.draftToken)
            else if (original.action === "inspect") result = owner.inspectFleetMissionSavedDraft(original.draftToken)
            else if (original.action === "save") result = owner.commitFleetMissionDraftSave(original.saveToken, original.draftToken, true)
            else if (original.action === "create") result = owner.createFleetMissionSavedDraft(original.sourceToken, original.replace, true)
            else if (original.action === "undo") result = owner.restorePreviousFleetMissionDraft(original.draftToken, true)
            else result = {ok: owner.cancelFleetMissionSavedDraftPreview(original.saveToken || original.sourceToken) === true}
        } catch (error) { result = {ok: false, error: "editing_checkpoint_call_interrupted"} }
        if (!guard.alive) return
        var committed = result && result.committed === true, current = result && result.current === true
        var succeeded = result && result.ok === true
        var errorText = result && typeof result.error === "string" && result.error ? result.error : "editing_checkpoint_not_current"
        var effect = original.action === "save" ? qsTr("本机草稿文件") : qsTr("草稿切换")
        var text = committed ? effect + qsTr("已提交") + (current ? qsTr("；当前性检查通过。")
            : qsTr("，但后续当前性未确认；不要把它当作未发生，也不要自动重试，请重新检查。"))
            : succeeded ? (original.action === "prepare" ? qsTr("保存预览已准备，尚未写入文件。请核对修订和覆盖提示，再单独确认。")
                : original.action === "inspect" ? qsTr("已检查本次本机保存快照；当前草稿未改变。恢复仍需单独确认。")
                : qsTr("本次预览已取消，草稿和保存文件未改变。"))
            : qsTr("未完成：%1。请重新检查来源或输入；坏文件和外部改动不会被自动覆盖。").arg(errorText)
        dispatchBusy = false
        if (!guard.alive) return
        clearConfirmations()
        if (!guard.alive) return
        // One publication keeps result facts and their original context bound.
        // Later owner/Episode changes (including A->B->A) cannot revive this text.
        feedbackState = {owner: owner, contextEpoch: original.contextEpoch, episodeToken: original.episodeToken,
            text: text, error: !succeeded, committed: committed, effect: effect}
        if (!guard.alive) return
        if (messageError && contextCurrent(original)) {
            feedback.forceActiveFocus()
            if (guard.alive && contextCurrent(original)) focusVisibilityRequested(feedback)
        }
    }

    onControllerChanged: retireContext()
    onDraftTokenChanged: clearConfirmations()
    onSaveTokenChanged: clearConfirmations()
    onSourceTokenChanged: clearConfirmations()
    onNativeTokenChanged: clearConfirmations()
    onEpisodeTokenChanged: retireContext()
    onInputSettledChanged: retireContext()
    onVisibleChanged: retireContext()
    onExpandedChanged: retireContext()
    Component.onDestruction: lifetime.alive = false

    ColumnLayout {
        id: content
        width: root.width; spacing: root.pad
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true
            text: qsTr("未完成草稿 · 本机保存与重启恢复")
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.expanded ? qsTr("收起本机草稿保存") : qsTr("展开本机草稿保存（不自动读写）")
            enabled: !root.dispatchBusy
            onClicked: root.expanded = !root.expanded
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("可保存尚未完成区域、成员或路线的已提交原生草稿，不要求通过编译。仅本机固定一个保存槽；不保存输入框内尚未应用的文字，不保存审批、飞机状态或执行权限。展开、重启均不会自动读取或恢复。")
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: !root.inputSettled ? qsTr("有未提交的输入、地图选点或确认尚未结束：请先应用或取消，避免把旧原生内容当成最新输入保存。")
                    : root.draftState.busy === true ? qsTr("本地操作正在处理，请等待本次结果；不会自动重复提交。")
                    : qsTr("1 · 准备保存预览，或明确读取上次保存。两者都会替换旧的本机来源预览。")
            }
            QGCButton {
                id: prepareButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("预览保存当前草稿（不写文件）"); enabled: root.localReady
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("prepare")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: prepareButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: prepareButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: root.draftState.saveCurrent === true ? qsTr("待保存原生修订 %1：区域顶点 %2，成员 %3。保存槽修订将为 %4；%5。尚未保存。")
                    .arg(root.checkpoint.sourceRevision).arg(root.checkpoint.vertexCount).arg(root.checkpoint.memberCount)
                    .arg(root.checkpoint.checkpointRevision).arg(root.checkpoint.overwritesExisting === true ? qsTr("会替换已有唯一保存槽") : qsTr("将新建保存槽"))
                    : qsTr("尚无与当前草稿修订一致的保存预览。")
            }
            Controls.CheckBox {
                id: saveConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: false; enabled: root.canSave
                text: qsTr("确认保存已提交的原生编辑内容；若已有保存槽，明确同意替换该槽（不更改当前草稿或审批）")
                contentItem: QGCLabel { text: saveConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (saveConsent.indicator ? saveConsent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                id: saveButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("确认写入本机保存槽"); enabled: root.canSave && saveConsent.checked
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("save")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: saveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: saveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCButton {
                id: inspectButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("读取本机已保存草稿（不改编辑器）"); enabled: root.localReady
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("inspect")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: inspectButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: inspectButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: root.draftState.sourceCurrent === true ? qsTr("本次已检查保存槽 %1，修订 %2；历史编辑修订 %3，顶点 %4，成员 %5。不是磁盘持续监视；确认恢复时会重新核对原文件。")
                    .arg(root.draftState.source.checkpointId).arg(root.draftState.source.checkpointRevision).arg(root.draftState.source.sourceRevision)
                    .arg(root.draftState.source.vertexCount).arg(root.draftState.source.memberCount)
                    : qsTr("2 · 尚无有效保存来源预览。没有来源不表示可以执行，也不会用默认空草稿覆盖坏文件。")
            }
            Controls.CheckBox {
                id: createConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: false; enabled: root.canCreate
                text: qsTr("确认恢复为独立编辑副本；保留当前草稿为会话备用，撤销旧审阅／审批，重新确认地图基准；不上传或启动")
                contentItem: QGCLabel { text: createConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (createConsent.indicator ? createConsent.indicator.width : 0) + root.pad }
            }
            Controls.CheckBox {
                id: replaceConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: false; visible: root.draftState.hasPrevious === true; enabled: root.canCreate
                text: qsTr("确认成功发布后替换更早的共享会话备用，仅保留本次操作前的当前草稿")
                contentItem: QGCLabel { text: replaceConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (replaceConsent.indicator ? replaceConsent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                id: createButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("恢复保存槽为独立可编辑副本")
                enabled: root.canCreate && createConsent.checked && (root.draftState.hasPrevious !== true || replaceConsent.checked)
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("create")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: createButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: createButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            Controls.CheckBox {
                id: undoConsent
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                checked: false; enabled: root.localReady && root.draftState.hasPrevious === true
                text: qsTr("确认切回共享会话备用（非磁盘旧版本）；当前草稿成为备用，重新确认地图基准和审阅")
                contentItem: QGCLabel { text: undoConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (undoConsent.indicator ? undoConsent.indicator.width : 0) + root.pad }
            }
            QGCButton {
                id: undoButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("切回上一份会话草稿"); enabled: root.localReady && root.draftState.hasPrevious === true && undoConsent.checked
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("undo")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: undoButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: undoButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCButton {
                id: cancelButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("取消本次本机预览（不删文件）"); enabled: root.localReady && (!!root.saveToken || !!root.sourceToken)
                property var pressedIntent: null
                onPressed: pressedIntent = root.intent("cancel")
                onCanceled: pressedIntent = null
                onClicked: { var saved = pressedIntent; pressedIntent = null; root.dispatch(saved) }
                contentItem: QGCLabel { text: cancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: cancelButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            }
            QGCLabel {
                id: feedback
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                activeFocusOnTab: true; Accessible.name: text
                text: root.message || qsTr("本功能尚未编译或实测。单槽保存不等于多版本备份、跨设备同步、断电持久性证明或飞行许可。")
            }
        }
    }
}

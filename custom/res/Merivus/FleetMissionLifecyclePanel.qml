import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Local retained object groups, not aircraft missions or control leases.
// This panel receives metadata only; the controller keeps native references.
FocusScope {
    id: root
    property var controller: null
    property var dialog: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, activeCall: null, view: null, opening: null, clearing: null})
    property var viewTicket: null
    property bool callBusy: false
    property var callTicket: null
    property string action: ""
    property string actionEpisodeId: ""
    property string actionToken: ""
    property var actionOwner: null
    property string localError: ""
    property var lastResult: ({})

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var state: controller ? controller.fleetDeploymentLifecycle || {} : ({})
    readonly property var rows: checkedRows(state.episodes)
    readonly property string currentToken: token(state.currentToken)
    readonly property string observationToken: token(state.observationToken)
    readonly property bool sourceReady: state.schemaVersion === 1 && state.sourceKnown === true && state.sourceLost === false
        && currentToken.length > 0 && observationToken.length > 0
        && integer(state.capacity, 1, 8) && integer(state.allocationCount, 1, state.capacity)
        && integer(state.capacityRemaining, 0, state.capacity) && state.capacityRemaining === state.capacity - state.allocationCount
        && currentRowKnown()
    readonly property bool backendBusy: state.busy !== false
    readonly property string contextToken: JSON.stringify({current: currentToken, observation: observationToken,
        sourceKnown: sourceReady, allocationCount: state.allocationCount})
    readonly property bool canPrepare: controller && lifetime.alive && !lifetime.exhausted && sourceReady
        && !callBusy && !backendBusy && state.capacityRemaining > 0 && typeof controller.prepareNextFleetDeployment === "function"
    readonly property bool canSelect: controller && lifetime.alive && !lifetime.exhausted && sourceReady
        && !callBusy && !backendBusy && typeof controller.selectFleetObservationEpisode === "function"
    readonly property bool actionCurrent: actionOwner === controller && actionToken.length > 0 && actionToken === contextToken
        && (action === "prepare" ? canPrepare && actionEpisodeId === state.currentEpisodeId
            : action === "select" && canSelect && selectable(actionEpisodeId))

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: entry.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function token(value) { return typeof value === "string" && value.length > 0 && value.length <= 1024 ? value : "" }
    function text(value) { return typeof value === "string" && value.length > 0 ? value.substring(0, 512) : qsTr("未知") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function checkedRows(value) {
        if (!Array.isArray(value) || value.length > 8) return []
        var result = [], seen = {}
        for (var i = 0; i < value.length; ++i) {
            var row = value[i]
            if (!row || !uuid(row.episodeId) || seen[row.episodeId]) return []
            var keys = ["published", "sourceRetained", "actualSourceUsable", "current", "selectedForObservation"]
            for (var k = 0; k < keys.length; ++k) if (typeof row[keys[k]] !== "boolean") return []
            seen[row.episodeId] = true
            result.push(row)
        }
        return result
    }
    function rowById(id) {
        if (!uuid(id)) return null
        for (var i = 0; i < rows.length; ++i) if (rows[i].episodeId === id) return rows[i]
        return null
    }
    function currentRowKnown() {
        var current = rowById(state.currentEpisodeId), selected = rowById(state.selectedObservationEpisodeId)
        return current && selected && current.current === true && selected.selectedForObservation === true
            && current.published === true && current.actualSourceUsable === true
            && selected.published === true && selected.actualSourceUsable === true
    }
    function selectable(id) {
        var row = rowById(id)
        return row && row.published === true && row.sourceRetained === true && row.actualSourceUsable === true
            && row.selectedForObservation === false
    }
    function advanceEpoch() {
        if (lifetime.epoch >= 9007199254740991) lifetime.exhausted = true
        else lifetime.epoch += 1
    }
    function clearAction() {
        var guard = lifetime, clearing = {}, view = dialog
        if (!guard.alive) return false
        guard.clearing = clearing
        action = ""
        if (!guard.alive || guard.clearing !== clearing) return false
        actionEpisodeId = ""
        if (!guard.alive || guard.clearing !== clearing) return false
        actionToken = ""
        if (!guard.alive || guard.clearing !== clearing) return false
        actionOwner = null
        if (!guard.alive || guard.clearing !== clearing) return false
        if (view && dialog === view) view.clearConfirmations()
        return guard.alive && guard.clearing === clearing
    }
    function armAction(kind, id, pressed) {
        var guard = lifetime
        if (!guard.alive || guard.activeCall) return
        if (!pressed || pressed.owner !== controller || pressed.epoch !== guard.epoch || pressed.context !== contextToken
                || !viewTicket || guard.view !== viewTicket
                || (kind === "prepare" ? !canPrepare : kind !== "select" || !canSelect || !selectable(id))) {
            localError = qsTr("对象或容量在点击期间已变化；请核对当前组和监视组后重新选择。")
            if (guard.alive) clearAction()
            return
        }
        var owner = controller, view = viewTicket, context = contextToken
        advanceEpoch()
        var epoch = guard.epoch
        if (!clearAction() || !guard.alive) return
        if (guard.view !== view || guard.epoch !== epoch || owner !== controller || context !== contextToken) return
        actionOwner = owner
        if (!guard.alive || guard.view !== view || guard.epoch !== epoch) return
        actionEpisodeId = id
        if (!guard.alive || guard.view !== view || guard.epoch !== epoch) return
        actionToken = context
        if (!guard.alive || guard.view !== view || guard.epoch !== epoch) return
        action = kind
        if (!guard.alive || guard.view !== view || guard.epoch !== epoch) return
        localError = ""
    }
    function commit(pressed, continuityConfirmed, freshIdentityConfirmed) {
        var guard = lifetime
        if (!guard.alive || guard.activeCall) return
        if (!actionCurrent || !pressed || pressed.owner !== controller || pressed.epoch !== guard.epoch
                || !viewTicket || guard.view !== viewTicket
                || pressed.context !== contextToken || pressed.action !== action || pressed.episodeId !== actionEpisodeId
                || continuityConfirmed !== true || (action === "prepare" && freshIdentityConfirmed !== true)) {
            localError = qsTr("确认已失效或不完整；没有提交。请重新选择并核对影响。")
            if (guard.alive) clearAction()
            return
        }
        var epoch = guard.epoch, owner = controller, kind = action, id = actionEpisodeId, view = viewTicket, context = contextToken
        var expected = kind === "prepare" ? currentToken : observationToken, ticket = {}, result = {}
        // Consume this gesture before callbacks. Never auto-retry a failed
        // allocation: failed attempts can consume native retention capacity.
        guard.activeCall = ticket
        try {
            callTicket = ticket
            if (!guard.alive || guard.activeCall !== ticket) return
            callBusy = true
            if (!guard.alive || guard.activeCall !== ticket) return
            localError = ""
            if (!guard.alive || guard.activeCall !== ticket) return
            if (!clearAction() || !guard.alive || guard.activeCall !== ticket) return
            if (guard.view !== view || viewTicket !== view || owner !== controller || !dialog
                    || guard.epoch !== epoch || contextToken !== context) {
                if (guard.view === view && viewTicket === view && owner === controller && dialog)
                    localError = qsTr("提交前组来源已变化；未调用后端。请重新选择并确认。")
                return
            }
            try {
                result = kind === "prepare" ? owner.prepareNextFleetDeployment(expected, true)
                    : owner.selectFleetObservationEpisode(id, expected, true)
                result = result || {}
            } catch (error) { result = {ok: false, reason: "lifecycle-call-interrupted-effects-unknown"} }
            if (!guard.alive || guard.activeCall !== ticket) return
            // Successful mutation normally changes the source generation. A
            // separate view ticket rejects close/reopen, not that expected
            // source change. No old result may populate a replacement view.
            if (guard.view !== view || viewTicket !== view || owner !== controller || !dialog) return
            lastResult = {action: kind, requestedEpisodeId: id, ok: result.ok === true, reason: text(result.reason),
                mutationCommitted: result.mutationCommitted, newEpisodePublished: result.newEpisodePublished,
                observationSelectionChanged: result.observationSelectionChanged, controllerCallReleased: result.controllerCallReleased,
                currentEpisodeId: result.currentEpisodeId, selectedObservationEpisodeId: result.selectedObservationEpisodeId}
            if (!guard.alive || guard.activeCall !== ticket) return
            if (guard.view !== view || viewTicket !== view || owner !== controller || !dialog) return
            if (result.ok !== true) localError = qsTr("本次请求未核实：%1。切换可能已发生；请查看下方原生事实和当前列表，不自动重试。").arg(text(result.reason))
        } finally {
            if (guard.alive && guard.activeCall === ticket) {
                if (callTicket === ticket) callTicket = null
                if (guard.alive && guard.activeCall === ticket) callBusy = false
            }
            // Keep the plain admission lock during the final Qt notification;
            // after this point touch only the original plain lifetime object.
            if (guard.activeCall === ticket) guard.activeCall = null
        }
    }
    function ensureFocusVisible(item) {
        var parent = item ? item.parent : null
        while (parent) {
            if (parent.contentItem && typeof parent.contentY === "number" && typeof parent.contentHeight === "number") {
                var point = item.mapToItem(parent.contentItem, 0, 0), top = point.y - pad, bottom = point.y + item.height + pad
                if (top < parent.contentY) parent.contentY = Math.max(0, top)
                else if (bottom > parent.contentY + parent.height) parent.contentY = Math.min(Math.max(0, parent.contentHeight - parent.height), bottom - parent.height)
                return
            }
            parent = parent.parent
        }
    }
    function dismissView() {
        var guard = lifetime
        if (!guard.alive) return
        var old = dialog
        guard.view = null
        advanceEpoch()
        viewTicket = null
        if (guard.alive && guard.view === null) dialog = null
        if (guard.alive && guard.view === null) clearAction()
        if (old) { try { old.close() } catch (error) {} }
    }
    function finishView(view) {
        var guard = lifetime
        if (!guard.alive) return
        if (dialog !== view) return
        guard.view = null
        advanceEpoch()
        viewTicket = null
        if (guard.alive && guard.view === null) dialog = null
        if (guard.alive && guard.view === null) clearAction()
        if (guard.alive && guard.view === null && !dialog && visible) entryButton.forceActiveFocus()
    }
    function openView() {
        var guard = lifetime
        if (!guard.alive || guard.opening || !controller || !visible || dialog || guard.exhausted) return
        var ticket = {}, owner = controller, view = null
        guard.opening = ticket
        try {
            localError = ""
            if (!guard.alive || guard.opening !== ticket) return
            lastResult = ({})
            if (!guard.alive || guard.opening !== ticket) return
            view = viewComponent.createObject(mainWindow, {ownerPanel: root, guard: guard})
            if (!guard.alive) { if (view) view.destroy(); return }
            if (owner !== controller || !visible || dialog) { if (view) view.destroy(); return }
            if (!view) { localError = qsTr("无法打开本地组管理视图；没有分配或切换。"); return }
            guard.view = ticket
            viewTicket = ticket
            if (!guard.alive || guard.view !== ticket) { view.destroy(); return }
            dialog = view
            if (!guard.alive || guard.view !== ticket) { view.destroy(); return }
            view.open()
        } finally { if (guard.opening === ticket) guard.opening = null }
    }
    onContextTokenChanged: {
        var guard = lifetime
        if (!guard.alive) return
        advanceEpoch(); clearAction()
        if (guard.alive && dialog && !callBusy) localError = qsTr("部署组、监视选择或容量已变化；旧确认已清除。请重新核对。")
    }
    onControllerChanged: {
        var guard = lifetime
        if (guard.alive) dismissView()
        if (guard.alive) lastResult = ({})
    }
    onVisibleChanged: if (!visible) dismissView()
    onLocalErrorChanged: if (localError && dialog) dialog.focusError()
    Component.onDestruction: {
        var guard = lifetime, old = dialog
        guard.alive = false; guard.view = null
        if (old) { try { old.close() } catch (error) {} }
    }

    ColumnLayout {
        id: entry
        width: parent.width; spacing: root.pad / 2
        QGCButton {
            id: entryButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("部署组 / 历史监视（本地管理）")
            Accessible.name: text
            enabled: !!root.controller
            onClicked: root.openView()
            contentItem: QGCLabel { text: entryButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: entryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: entryButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: entryButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: root.sourceReady ? qsTr("本地槽占用 %1 / %2；创建新组不自动切换旧监视。").arg(root.state.allocationCount).arg(root.state.capacity)
                : qsTr("组来源未核实；可查看诊断，不允许凭未知状态创建或切换。")
        }
        QGCLabel { Layout.fillWidth: true; visible: !root.dialog && !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.localError; color: qgcPal.warningText }
    }

    Component {
        id: viewComponent
        QGCPopupDialog {
            id: view
            property var ownerPanel: null
            property var guard: null
            property var localViewLife: ({alive: true})
            title: qsTr("本地部署组与监视选择 · 不发送飞行指令")
            buttons: 0; acceptAllowed: false; rejectAllowed: false
            destroyOnClose: false; closePolicy: Controls.Popup.NoAutoClose
            function clearConfirmations() {
                var life = guard, ownLife = localViewLife
                if (!life || !life.alive || !ownLife.alive) return
                continuityConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                identityConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                commitButton.pressScope = ({})
            }
            function focusError() {
                var life = guard, ownLife = localViewLife
                if (!life || !life.alive || !ownLife.alive || !errorSummary.visible) return
                errorSummary.forceActiveFocus()
                if (life.alive && ownLife.alive && ownerPanel) ownerPanel.ensureFocusVisible(errorSummary)
            }
            onOpened: closeButton.forceActiveFocus()
            onClosed: {
                var life = guard, actualView = view
                localViewLife.alive = false
                if (life && life.alive && ownerPanel) ownerPanel.finishView(actualView)
                try { actualView.destroy() } catch (error) {}
            }
            Component.onDestruction: localViewLife.alive = false
            ColumnLayout {
                width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 54, view._maxContentWidth - view._contentMargin * 2))
                spacing: root.pad
                Keys.onPressed: {
                    if (event.key === Qt.Key_Escape) { view.close(); event.accepted = true }
                    else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) event.accepted = true
                }
                QGCButton {
                    id: closeButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("关闭 / 放弃本页确认（不撤销已发生的切换）")
                    Accessible.name: text
                    onClicked: view.close()
                    Keys.onReturnPressed: { view.close(); event.accepted = true }
                    Keys.onEnterPressed: { view.close(); event.accepted = true }
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(closeButton)
                    contentItem: QGCLabel { text: closeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: closeButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: closeButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: closeButton.activeFocus ? 3 : 1; radius: 3 }
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                    text: qsTr("新建只分配全新的本地对象组，不结束旧Run、不释放其占用、不继承登录或批准；旧飞机可能继续飞行。选择历史组只改变本地监视，不移交远端控制。停止飞机请使用独立飞行安全操作。")
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("当前任务组：%1\n当前监视组：%2\n本地槽占用 / 上限：%3 / %4\n保留和待销毁组仍占槽；只有显式归档、最后核对及实际销毁才回收。终身尝试不重置，不自动重试。")
                        .arg(root.text(root.state.currentEpisodeId)).arg(root.text(root.state.selectedObservationEpisodeId))
                        .arg(root.integer(root.state.allocationCount, 0, 8) ? String(root.state.allocationCount) : qsTr("未知"))
                        .arg(root.integer(root.state.capacity, 1, 8) ? String(root.state.capacity) : qsTr("未知"))
                }
                FocusScope {
                    id: errorSummary
                    Layout.fillWidth: true; Layout.preferredHeight: errorLabel.implicitHeight + root.pad
                    visible: !!root.localError; activeFocusOnTab: visible
                    Accessible.role: Accessible.StaticText; Accessible.name: root.localError
                    Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: qgcPal.warningText; border.width: errorSummary.activeFocus ? 3 : 1; radius: 3 }
                    QGCLabel { id: errorLabel; anchors.fill: parent; anchors.margins: root.pad / 2; text: root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                }
                QGCButton {
                    id: prepareButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("选择创建下一部署组（下一步确认）")
                    Accessible.name: text; enabled: root.canPrepare
                    property var pressScope: ({})
                    onPressed: pressScope = {owner: root.controller, epoch: root.lifetime.epoch, context: root.contextToken, target: root.state.currentEpisodeId}
                    onCanceled: pressScope = ({})
                    onClicked: {
                        var life = root.lifetime, ownLife = view.localViewLife, pressed = pressScope
                        pressScope = ({})
                        if (life.alive && ownLife.alive) root.armAction("prepare", pressed.target, pressed)
                    }
                    Keys.onReturnPressed: event.accepted = true
                    Keys.onEnterPressed: event.accepted = true
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(prepareButton)
                    contentItem: QGCLabel { text: prepareButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: prepareButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: prepareButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: prepareButton.activeFocus ? 3 : 1; radius: 3 }
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: root.callBusy || root.backendBusy ? qsTr("有调用、切换、身份提示或未知忙状态；不能重复提交。")
                        : !root.sourceReady ? qsTr("实际来源不完整；请保留状态交工程师核查，不把未知当空闲。")
                        : root.state.capacityRemaining <= 0 ? qsTr("本地容量已满；可在下方明确归档已闭旧组后申请回收。不会自动驱逐旧Run、清空占用或跳过最后核对。")
                        : qsTr("可申请不代表已排空或允许飞行；原生后端仍核全部保留组与原实际来源。")
                }
                Repeater {
                    model: root.rows
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: root.pad / 2
                        property var row: modelData
                        QGCLabel {
                            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                            text: qsTr("组：%1\n当前任务 / 当前监视：%2 / %3\n已发布 / 实际来源可用：%4 / %5")
                                .arg(root.text(parent.row.episodeId)).arg(root.known(parent.row.current))
                                .arg(root.known(parent.row.selectedForObservation)).arg(root.known(parent.row.published))
                                .arg(root.known(parent.row.actualSourceUsable))
                        }
                        QGCButton {
                            id: selectButton
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            text: parent.row.selectedForObservation === true ? qsTr("已选为监视组")
                                : parent.row.published !== true || parent.row.actualSourceUsable !== true ? qsTr("未发布或来源不可用，不能选择") : qsTr("选择本组监视（下一步确认）")
                            Accessible.name: text + " " + parent.row.episodeId
                            enabled: root.canSelect && root.selectable(parent.row.episodeId)
                            property var pressScope: ({})
                            onPressed: pressScope = {owner: root.controller, epoch: root.lifetime.epoch, context: root.contextToken, target: parent.row.episodeId}
                            onCanceled: pressScope = ({})
                            onClicked: {
                                var life = root.lifetime, ownLife = view.localViewLife, pressed = pressScope
                                pressScope = ({})
                                if (life.alive && ownLife.alive) root.armAction("select", pressed.target, pressed)
                            }
                            Keys.onReturnPressed: event.accepted = true
                            Keys.onEnterPressed: event.accepted = true
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(selectButton)
                            contentItem: QGCLabel { text: selectButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: selectButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            background: Rectangle { color: selectButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: selectButton.activeFocus ? 3 : 1; radius: 3 }
                        }
                    }
                }
                QGCLabel {
                    Layout.fillWidth: true; visible: root.action.length > 0; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: (root.action === "prepare" ? qsTr("待确认：从本组创建下一新组（不自动切换监视）\n%1") : qsTr("待确认：仅选择下组本地监视\n%1")).arg(root.text(root.actionEpisodeId))
                }
                Controls.CheckBox {
                    id: continuityConsent
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: root.action.length > 0; enabled: root.actionCurrent; checked: false
                    text: root.action === "prepare" ? qsTr("我知道旧飞机可能继续飞行；新建不会停止或释放旧Run占用")
                        : qsTr("我只切换本地监视；不移交控制权、不改变飞行任务")
                    Accessible.name: text
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(continuityConsent)
                    contentItem: QGCLabel { text: continuityConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (continuityConsent.indicator ? continuityConsent.indicator.width : 0) + root.pad }
                }
                Controls.CheckBox {
                    id: identityConsent
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: root.action === "prepare"; enabled: root.actionCurrent; checked: false
                    text: qsTr("我同意界面旧确认失效；新组必须重新认证与审批，不复用旧凭据")
                    Accessible.name: text
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(identityConsent)
                    contentItem: QGCLabel { text: identityConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (identityConsent.indicator ? identityConsent.indicator.width : 0) + root.pad }
                }
                QGCButton {
                    id: commitButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: root.action.length > 0
                    text: root.action === "prepare" ? qsTr("确认创建下一部署组（不启动飞行）") : qsTr("确认切换本地监视")
                    Accessible.name: text
                    enabled: root.actionCurrent && continuityConsent.checked && (root.action !== "prepare" || identityConsent.checked)
                    property var pressScope: ({})
                    onPressed: pressScope = {owner: root.controller, epoch: root.lifetime.epoch, context: root.contextToken, action: root.action, episodeId: root.actionEpisodeId}
                    onCanceled: pressScope = ({})
                    onClicked: {
                        var life = root.lifetime, ownLife = view.localViewLife, pressed = pressScope
                        var continuity = continuityConsent.checked, identity = identityConsent.checked
                        pressScope = ({})
                        if (life.alive && ownLife.alive) root.commit(pressed, continuity, identity)
                    }
                    Keys.onReturnPressed: event.accepted = true
                    Keys.onEnterPressed: event.accepted = true
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(commitButton)
                    contentItem: QGCLabel { text: commitButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: commitButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: commitButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.warningText; border.width: commitButton.activeFocus ? 3 : 1; radius: 3 }
                }
                QGCLabel {
                    Layout.fillWidth: true; visible: !!root.lastResult.action; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("本页上次请求（不是当前飞行状态）：%1\n请求原组：%2\n原生变更已提交：%3 · 新组已发布：%4 · 监视已改变：%5\n原调用结束已核实：%6\n返回的任务组 / 监视组：%7 / %8\n原因：%9\n失败或关闭视图不回滚；缺失事实保持未知，不自动重试。")
                        .arg(root.lastResult.action === "prepare" ? qsTr("创建新组") : qsTr("选择监视")).arg(root.text(root.lastResult.requestedEpisodeId))
                        .arg(root.known(root.lastResult.mutationCommitted)).arg(root.known(root.lastResult.newEpisodePublished))
                        .arg(root.known(root.lastResult.observationSelectionChanged)).arg(root.known(root.lastResult.controllerCallReleased))
                        .arg(root.text(root.lastResult.currentEpisodeId)).arg(root.text(root.lastResult.selectedObservationEpisodeId)).arg(root.text(root.lastResult.reason))
                }
                FleetDeploymentArchivePanel {
                    Layout.fillWidth: true
                    Layout.minimumHeight: implicitHeight
                    controller: root.controller
                    sourceActive: view.visible && view.localViewLife.alive && root.visible && root.enabled
                }
            }
        }
    }
}

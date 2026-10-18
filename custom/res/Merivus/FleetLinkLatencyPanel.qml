import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Only value projections and explicit Root calls; no native owner or auto probe.
FocusScope {
    id: root
    property var controller: null
    property var dialog: null
    property var life: ({alive: true, epoch: 0, exhausted: false, held: null, pending: null, view: null, opening: false})
    property var receipt: ({})
    property bool callBusy: false
    property string vehicleText: "1"
    property string message: ""
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var snapshot: controller ? controller.fleetLinkLatencyCaptureSnapshot || {} : ({})
    readonly property var workflow: snapshot.workflow || ({})
    readonly property var identity: snapshot.operatorIdentity || ({})
    readonly property int vehicleId: /^[1-9][0-9]{0,2}$/.test(vehicleText) && Number(vehicleText) <= 255 ? Number(vehicleText) : 0
    readonly property string stableContext: context()
    readonly property bool own: uuid(receipt.requestId) && receipt.owner === controller && receipt.base === stableContext
        && receipt.vehicleId === vehicleId && known(workflow) && snapshot.workflowScopeMatches === true
        && workflow.requestId === receipt.requestId && workflow.generation === receipt.generation
        && workflow.scopeToken === receipt.scopeToken && workflow.vehicleId === vehicleId
    readonly property bool canCapture: enabled && !!dialog && !dialog.consentExhausted && !!stableContext && !callBusy && !life.exhausted
        && snapshot.busy === false && workflow.active === false && vehicleId > 0 && identity.authenticated === true && roleAllowed()
        && typeof controller.requestFleetLinkLatencyCapture === "function" && typeof controller.cancelFleetLinkLatencyCapture === "function"
    implicitHeight: entry.implicitHeight
    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    QGCPalette { id: palette; colorGroupEnabled: root.enabled }

    function map(value) { return value && typeof value === "object" && !Array.isArray(value) }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function known(value) {
        return map(value) && Object.keys(value).length === 18 && value.schemaVersion === 1
            && typeof value.generation === "string" && /^(0|[1-9][0-9]{0,19})$/.test(value.generation)
            && typeof value.active === "boolean" && typeof value.state === "string" && typeof value.reason === "string"
            && value.protocol === "targeted-timesync-v2" && value.maximumAgeMs === 30000
            && value.snapshotOnly === true && value.authenticated === false && value.configurationPermitted === false
            && value.executionPermit === false && value.flightCommandReleased === false && map(value.observation)
    }
    function context() {
        if (!controller || snapshot.sourceKnown !== true || snapshot.uiWiringPresent !== true
                || snapshot.runtimeValidated !== false || !known(workflow)) return ""
        try {
            var parsed = JSON.parse(snapshot.sourceToken)
            if (!map(parsed) || typeof parsed.scope !== "string" || !parsed.scope || !map(parsed.policy)
                    || Object.keys(parsed.policy).length !== 9 || parsed.policy.action !== "fleet.telemetry.rtt.capture"
                    || (parsed.identity !== null && (!map(parsed.identity) || Object.keys(parsed.identity).length !== 5))) return ""
            return JSON.stringify({scope: parsed.scope, identity: parsed.identity, policy: parsed.policy})
        } catch (error) { return "" }
    }
    function pressed() {
        return {owner: controller, epoch: life.epoch, base: stableContext, view: life.view,
            token: snapshot.sourceToken, vehicleId: vehicleId,
            consentEpoch: dialog ? dialog.consentEpoch : -1, consent: !!dialog && dialog.consentChecked === true}
    }
    function roleAllowed() {
        try {
            var value = JSON.parse(stableContext)
            return value.policy.allowed === true && value.policy.requiresSecondApproval === false
        } catch (error) { return false }
    }
    function pressCurrent(value) {
        return value && life.alive && !life.exhausted && value.owner === controller && value.epoch === life.epoch
            && value.base === stableContext && value.view && value.view === life.view && value.vehicleId === vehicleId
            && !!dialog && visible && enabled && !dialog.consentExhausted && value.consentEpoch === dialog.consentEpoch
    }
    function retire() {
        var guard = life, held = guard ? guard.held : null
        if (!guard || !guard.alive) return
        if (guard.pending) guard.pending.cancelState.cancelled = true
        guard.held = null
        if (guard.epoch >= 9007199254740991) guard.exhausted = true
        else guard.epoch += 1
        // Cleanup responsibility is detached BEFORE any QML notification.
        try {
            receipt = ({})
            if (guard.alive && dialog) dialog.clearConfirmation()
        } finally {
            if (held && held.owner) try { held.owner.cancelFleetLinkLatencyCapture(held.requestId) } catch (error) {}
        }
    }
    function authenticate(value) {
        var guard = life
        if (!pressCurrent(value) || callBusy || guard.pending || workflow.active === true
                || typeof controller.authenticateFleetOperator !== "function") return
        var owner = controller, base = stableContext, epoch = guard.epoch, ticket = {cancelState: {cancelled: false}}
        guard.pending = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.pending !== ticket || stableContext !== base) return
            owner.authenticateFleetOperator()
        } finally {
            if (guard.alive) {
                callBusy = false
                if (guard.alive && guard.pending === ticket) guard.pending = null
            }
        }
    }
    function capture(value, confirmed) {
        var guard = life
        if (!canCapture || guard.pending || confirmed !== true || !pressCurrent(value) || value.consent !== true
                || value.token !== snapshot.sourceToken) return
        var owner = controller, base = stableContext, viewToken = guard.view, oldEpoch = guard.epoch, target = vehicleId
        var submittedToken = value.token // Never rebase an old press after cancellation notifications.
        retire()
        if (!guard.alive || guard.exhausted || guard.epoch !== oldEpoch + 1 || stableContext !== base
                || owner !== controller || guard.view !== viewToken || target !== vehicleId || !dialog || !visible
                || submittedToken !== snapshot.sourceToken || dialog.consentExhausted
                || dialog.consentEpoch !== value.consentEpoch + 1 || dialog.consentChecked !== false) return
        var epoch = guard.epoch, ticket = {cancelState: {cancelled: false}}, result = {}
        var abandon = function() {
            if (result && typeof result.requestId === "string"
                    && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(result.requestId))
                try { owner.cancelFleetLinkLatencyCapture(result.requestId) } catch (error) {}
        }
        guard.pending = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.pending !== ticket || guard.epoch !== epoch || stableContext !== base
                    || submittedToken !== snapshot.sourceToken) return
            message = ""
            if (!guard.alive || guard.pending !== ticket || guard.epoch !== epoch || stableContext !== base
                    || submittedToken !== snapshot.sourceToken) return
            try { result = owner.requestFleetLinkLatencyCapture(submittedToken, target, true, ticket.cancelState) || {} }
            catch (error) { result = {reason: "rtt-capture-call-unavailable"} }
            if (!guard.alive) { abandon(); return }
            if (guard.pending !== ticket || ticket.cancelState.cancelled || guard.epoch !== epoch || guard.view !== viewToken
                    || owner !== controller || stableContext !== base || target !== vehicleId || !dialog || !visible) { abandon(); return }
            var current = workflow
            if (Object.keys(result).length !== 5 || result.ok !== true || result.requestAccepted !== true || !uuid(result.requestId)
                    || result.flightCommandReleased !== false || !known(current) || current.requestId !== result.requestId
                    || current.active !== true || current.vehicleId !== target || snapshot.workflowScopeMatches !== true) {
                abandon()
                if (guard.alive && guard.epoch === epoch) message = qsTr("未建立采集：%1。不会自动重试。").arg(result.reason || qsTr("未知"))
                return
            }
            var held = {owner: owner, base: base, requestId: result.requestId, generation: current.generation,
                scopeToken: current.scopeToken, vehicleId: target}
            guard.held = held
            receipt = held
            if (guard.alive && guard.epoch === epoch) message = qsTr("原生采集已接纳；最多单次定向请求，结果以回显或超时为准。")
        } finally {
            if (guard.alive) {
                callBusy = false
                if (guard.alive && guard.pending === ticket) guard.pending = null
            }
        }
    }
    function finishView(actualView) {
        var guard = life
        if (!guard || !guard.alive || dialog !== actualView) return
        guard.view = null
        retire()
        if (!guard || !guard.alive) return
        dialog = null
        if (guard.alive && visible) entry.forceActiveFocus()
    }
    function dismiss() {
        var guard = life, actualView = dialog
        if (!guard || !guard.alive) return
        guard.view = null
        retire()
        if (!guard.alive) return
        dialog = null
        if (actualView) try { actualView.close() } catch (error) {}
    }
    function openView() {
        var guard = life, owner = controller
        if (!guard || !guard.alive || guard.exhausted || guard.opening || guard.pending || !owner || !visible || dialog) return
        guard.opening = true
        var actualView = null, token = {}
        try {
            actualView = viewComponent.createObject(mainWindow, {guard: guard, ownerPanel: root})
            if (!guard.alive || !actualView || owner !== controller || !visible || dialog) {
                if (actualView) actualView.destroy()
                return
            }
            guard.view = token
            dialog = actualView
            if (!guard.alive || guard.view !== token) { actualView.destroy(); return }
            actualView.open()
        } finally { guard.opening = false }
    }
    function ensureFocusVisible(item) {
        if (!dialog || !item) return
        var point = item.mapToItem(dialog.scrollContent, 0, 0), scroll = dialog.scrollArea
        if (point.y - pad < scroll.contentY) scroll.contentY = Math.max(0, point.y - pad)
        else if (point.y + item.height + pad > scroll.contentY + scroll.height)
            scroll.contentY = Math.min(Math.max(0, scroll.contentHeight - scroll.height), point.y + item.height + pad - scroll.height)
    }
    function summary() {
        var value = map(workflow.observation) ? workflow.observation : {}
        return (own ? qsTr("本页真实接纳的采集快照\n") : qsTr("共享只读快照，不属于本页\n"))
            + qsTr("该次状态：%1\n原因：%2\n已可能发送：%3；匹配回显：%4\n该次本地往返耗时：%5\n这是历史快照，不持续证明来源或链路正常。")
            .arg(workflow.state || qsTr("未知")).arg(workflow.reason || qsTr("无"))
            .arg(value.requestIssued === true ? qsTr("是") : qsTr("否／未知"))
            .arg(value.responseObserved === true ? qsTr("是") : qsTr("否／未知"))
            .arg(value.responseObserved === true && typeof value.roundTripMs === "number" && value.roundTripMs >= 0
                ? value.roundTripMs.toFixed(3) + " ms" : qsTr("未知"))
    }
    onStableContextChanged: retire()
    onVehicleTextChanged: retire()
    onControllerChanged: dismiss()
    onVisibleChanged: if (!visible) dismiss()
    onEnabledChanged: if (!enabled) dismiss()
    Component.onDestruction: {
        var guard = life, held = guard.held, actualView = dialog
        if (guard.pending) guard.pending.cancelState.cancelled = true
        guard.alive = false; guard.held = null; guard.view = null
        if (held && held.owner) try { held.owner.cancelFleetLinkLatencyCapture(held.requestId) } catch (error) {}
        if (actualView) try { actualView.close() } catch (error) {}
    }
    QGCButton {
        id: entry
        width: parent.width; height: Math.max(root.rowHeight, implicitHeight); focusPolicy: Qt.StrongFocus
        text: qsTr("地面单次定向往返耗时采集")
        enabled: root.controller !== null && !!root.life && !root.life.exhausted
        onClicked: root.openView()
        Accessible.name: text
        contentItem: QGCLabel { text: entry.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: entry._showHighlight ? palette.buttonHighlightText : palette.buttonText }
        background: Rectangle { color: entry._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: entry.activeFocus ? 3 : 1; radius: 3 }
    }
    Component {
        id: viewComponent
        Controls.Dialog {
            id: view
            property var ownerPanel: null
            property var guard: null
            property var viewLife: ({alive: true})
            property double consentEpoch: 0
            property bool consentExhausted: false
            readonly property bool consentChecked: consent.checked
            property alias scrollArea: scroll
            property alias scrollContent: form
            parent: Controls.Overlay.overlay
            anchors.centerIn: parent
            modal: true; focus: true; closePolicy: Controls.Popup.NoAutoClose
            standardButtons: Controls.Dialog.NoButton; padding: root.pad
            width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 64, mainWindow.width - root.pad * 4))
            height: Math.max(1, Math.min(form.implicitHeight + heading.implicitHeight + root.pad * 4, mainWindow.height - root.pad * 4))
            background: Rectangle { color: palette.window; border.color: palette.text; border.width: 1; radius: 3 }
            header: QGCLabel { id: heading; padding: root.pad; text: qsTr("单次定向往返耗时 · 不是纯网络延迟"); font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            function clearConfirmation() { if (guard && guard.alive && viewLife.alive) consent.checked = false }
            onOpened: closeButton.forceActiveFocus()
            onAboutToHide: if (guard && guard.alive && ownerPanel) ownerPanel.finishView(view)
            onClosed: {
                var actualView = view
                viewLife.alive = false
                if (guard && guard.alive && ownerPanel) ownerPanel.finishView(actualView)
                actualView.destroy()
            }
            Component.onDestruction: { viewLife.alive = false; if (guard && guard.alive && ownerPanel) ownerPanel.finishView(view) }
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
                        text: qsTr("取消本页后续采集 / 关闭（不撤回已发字节）")
                        onClicked: view.close()
                        Keys.onReturnPressed: { view.close(); event.accepted = true }
                        Keys.onEnterPressed: { view.close(); event.accepted = true }
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(closeButton)
                        contentItem: QGCLabel { text: closeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: closeButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: closeButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: closeButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: palette.warningText
                        text: qsTr("仅地面未解锁。单次MAVLink2定向TIMESYNC，最初4秒期限含授权检查；不支持或未回应只给未知，不回退PING或广播，不重试。耗时含发送排队、远端处理和本地投递；不测时钟偏移、丢包或可用带宽，不改配置或飞行。")
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("账号：%1 · 角色：%2").arg(root.identity.actorId || qsTr("未认证")).arg(root.identity.role || qsTr("未知")) }
                    QGCButton {
                        id: authButton
                        property var pressScope: ({})
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("认证当前操作员")
                        enabled: !!root.stableContext && !root.callBusy && root.workflow.active === false && root.identity.authenticated !== true
                            && !!root.controller && typeof root.controller.authenticateFleetOperator === "function"
                        onPressed: pressScope = root.pressed()
                        onClicked: root.authenticate(pressScope)
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(authButton)
                        contentItem: QGCLabel { text: authButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: authButton.enabled ? 1 : 0.5; color: authButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: authButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: authButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel { text: qsTr("目标机号（1–255）") }
                    QGCTextField {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: root.vehicleText; inputMethodHints: Qt.ImhDigitsOnly
                        enabled: !root.callBusy && root.workflow.active !== true
                        onTextChanged: if (root.vehicleText !== text) root.vehicleText = text
                        Accessible.name: qsTr("单次定向采集目标机号")
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(this)
                    }
                    QGCCheckBox {
                        id: consent
                        Layout.fillWidth: true; text: qsTr("确认仅向该目标采集一次，不自动重试"); checked: false
                        enabled: root.canCapture
                        onCheckedChanged: {
                            if (view.consentEpoch >= 9007199254740991) view.consentExhausted = true
                            else view.consentEpoch += 1
                        }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(consent)
                    }
                    QGCButton {
                        id: captureButton
                        property var pressScope: ({})
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("明确采集一次往返耗时")
                        enabled: root.canCapture && consent.checked
                        onPressed: pressScope = root.pressed()
                        onClicked: root.capture(pressScope, consent.checked)
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(captureButton)
                        contentItem: QGCLabel { text: captureButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: captureButton.enabled ? 1 : 0.5; color: captureButton._showHighlight ? palette.buttonHighlightText : palette.buttonText }
                        background: Rectangle { color: captureButton._showHighlight ? palette.buttonHighlight : palette.button; border.color: palette.buttonText; border.width: captureButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        id: status
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        activeFocusOnTab: visible
                        text: root.message + "\n" + root.summary()
                        Accessible.name: text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(status)
                    }
                }
            }
        }
    }
}

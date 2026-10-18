import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Nominal 2D planning intent only. Source changes retain uncommitted text;
// neither this editor nor its revision-CAS setter releases flight commands.
ColumnLayout {
    id: root
    property var draft: null
    property string sourceIdentity: ""
    property bool inputSettled: false
    signal focusVisibilityRequested(var item)
    property bool editing: false
    property string inputText: ""
    property string setbackText: ""
    property bool confirmed: false
    property string message: ""
    property var session: null
    property bool callBusy: false
    property var callTicket: null
    property var contextEpoch: ({})
    property var lifetime: ({alive: true, ticket: null})
    readonly property bool pendingInput: editing || callBusy
    readonly property bool sourceCurrent: bindingCurrent(session)
    readonly property bool ready: visible && enabled && inputSettled && !callBusy && callTicket === null
        && draft !== null && !draft.revisionExhausted && sourceIdentity.length > 0
    readonly property var saved: draft && draft.settings && Array.isArray(draft.settings.exclusionPolygons)
        ? draft.settings.exclusionPolygons : []
    readonly property string savedSetbackText: draft && draft.settings && typeof draft.settings.boundarySetbackMeters === "number"
        ? String(draft.settings.boundarySetbackMeters) : ""
    readonly property real rowHeight: Math.max(36, ScreenTools.defaultFontPixelHeight * 2.4)
    spacing: ScreenTools.defaultFontPixelWidth
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }
    Component.onDestruction: { lifetime.alive = false; lifetime.ticket = null }
    onDraftChanged: retireSource()
    onSourceIdentityChanged: retireSource()
    onInputSettledChanged: if (!inputSettled) revokeConsent()
    onVisibleChanged: if (!visible) revokeConsent()
    onEnabledChanged: if (!enabled) revokeConsent()

    function revokeConsent() {
        var guard = lifetime
        confirmed = false
        if (guard.alive) contextEpoch = ({})
    }

    function bindingCurrent(value) {
        return visible && enabled && !!value && value.draft === draft && value.source === sourceIdentity
            && sourceIdentity.length > 0 && draft && value.revision === draft.revision
    }
    function retireSource() {
        var guard = lifetime
        session = null
        if (!guard.alive) return
        revokeConsent()
        if (guard.alive && editing) message = qsTr("草稿来源已改变。原文字保留供复制，但不能提交到新来源；请明确取消后重新编辑。")
    }
    function fail(text) {
        var guard = lifetime
        confirmed = false
        if (!guard.alive) return false
        message = text
        if (guard.alive && visible) {
            feedback.forceActiveFocus()
            if (guard.alive) focusVisibilityRequested(feedback)
        }
        return false
    }
    function begin() {
        var guard = lifetime
        if (!guard.alive || guard.ticket !== null || !ready || editing) return false
        var owner = draft, source = sourceIdentity, revision = owner.revision, ticket = ({}), epoch = contextEpoch
        guard.ticket = ticket
        try {
            callTicket = ticket
            if (!guard.alive) return false
            callBusy = true
            if (!guard.alive || epoch !== contextEpoch || !visible || !enabled || draft !== owner || sourceIdentity !== source || owner.revision !== revision || !inputSettled) return false
            var accepted = owner.invalidatePreview()
            if (!guard.alive || epoch !== contextEpoch || !visible || !enabled || accepted !== true || draft !== owner || sourceIdentity !== source || owner.revision !== revision || !inputSettled) return false
            inputText = JSON.stringify(saved, null, 2)
            if (!guard.alive) return false
            setbackText = savedSetbackText
            if (!guard.alive) return false
            session = {draft: owner, source: source, revision: revision}
            if (!guard.alive) return false
            confirmed = false
            if (!guard.alive) return false
            message = ""
            if (!guard.alive) return false
            editing = true
            if (!guard.alive || epoch !== contextEpoch || !bindingCurrent(session) || !inputSettled) return false
            coordinates.forceActiveFocus()
            if (guard.alive) focusVisibilityRequested(coordinates)
            return guard.alive
        } finally {
            if (guard.alive && guard.ticket === ticket) {
                callBusy = false
                if (guard.alive && guard.ticket === ticket) callTicket = null
                if (guard.ticket === ticket) guard.ticket = null
            }
        }
    }
    function editText(value) {
        if (!editing || value === inputText) return
        var guard = lifetime, owner = draft, source = sourceIdentity, originalSession = session
        // Consent is retired before publishing new text: Changed callbacks
        // must never submit the new value with the old checkbox approval.
        revokeConsent()
        if (!guard.alive || !editing || owner !== draft || source !== sourceIdentity || session !== originalSession) return
        inputText = value
        if (!guard.alive) return
        message = ""
        if (guard.alive && owner && owner === draft) owner.invalidatePreview()
    }
    function editSetback(value) {
        if (!editing || value === setbackText) return
        var guard = lifetime, owner = draft, source = sourceIdentity, originalSession = session
        revokeConsent()
        if (!guard.alive || !editing || owner !== draft || source !== sourceIdentity || session !== originalSession) return
        setbackText = value
        if (!guard.alive) return
        message = ""
        if (guard.alive && owner && owner === draft) owner.invalidatePreview()
    }
    function cancel() {
        if (callBusy || lifetime.ticket !== null) return
        var guard = lifetime, owner = draft, source = sourceIdentity, epoch = contextEpoch
        var originalSession = session, originalText = inputText, originalSetback = setbackText, ticket = ({})
        guard.ticket = ticket
        function current(expectedSession, text, margin) {
            return guard.alive && guard.ticket === ticket && epoch === root.contextEpoch
                && owner === root.draft && source === root.sourceIdentity && root.session === expectedSession
                && root.inputText === text && root.setbackText === margin
        }
        try {
            callTicket = ticket
            if (!current(originalSession, originalText, originalSetback)) return
            callBusy = true
            if (!current(originalSession, originalText, originalSetback)) return
            // Keep the action slot until every notification has drained. A
            // cancellation callback cannot open an edit then have it cleared.
            inputText = ""
            if (!current(originalSession, "", originalSetback)) return
            setbackText = ""
            if (!current(originalSession, "", "")) return
            session = null
            if (!current(null, "", "")) return
            confirmed = false
            if (!current(null, "", "")) return
            message = ""
            if (!current(null, "", "")) return
            editing = false
        } finally {
            if (guard.alive && guard.ticket === ticket) {
                callBusy = false
                if (guard.alive && guard.ticket === ticket) callTicket = null
                if (guard.ticket === ticket) guard.ticket = null
            }
        }
    }
    function retainInterruptedCommit(guard) {
        if (!guard.alive) return false
        editing = true
        if (!guard.alive) return false
        confirmed = false
        if (guard.alive) message = qsTr("原本地保存可能已提交，但收尾时编辑内容或来源发生变化。保留当前输入，请核对实际草稿版本；不会自动重试或回滚。")
        return false
    }
    function apply() {
        var guard = lifetime, binding = session
        if (!guard.alive || guard.ticket !== null || !ready || !editing || !confirmed || !bindingCurrent(binding)) return false
        var originalText = inputText, originalSetback = setbackText, polygons, epoch = contextEpoch
        if (originalText.length > 32768) return fail(qsTr("输入超过 32768 字符，请缩小排除区数据。"))
        try { polygons = JSON.parse(originalText) }
        catch (error) { return fail(qsTr("JSON 无法读取。请使用多边形顶点数组；原文字未丢弃。")) }
        if (!Array.isArray(polygons) || polygons.length > 8) return fail(qsTr("最外层必须是最多 8 个多边形组成的数组；[] 表示明确不设排除区。"))
        var policy = {exclusionPolygons: polygons}, margin = originalSetback.trim()
        if (margin.length > 0) {
            if (margin.length > 128 || !/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(margin))
                return fail(qsTr("规划退让距离必须为 0–500 米的有限十进制数；留空表示明确移除该声明。"))
            var distance = Number(margin)
            if (!isFinite(distance) || distance < 0 || distance > 500)
                return fail(qsTr("规划退让距离超出 0–500 米。请核对单位，不会自动改成 0。"))
            policy.boundarySetbackMeters = distance
        }
        var ticket = ({})
        guard.ticket = ticket
        try {
            callTicket = ticket
            if (!guard.alive) return false
            callBusy = true
            // Notifications from busy/publication may change the source or
            // consent. Never retarget the frozen edit to the current draft.
            if (!guard.alive || epoch !== contextEpoch || !bindingCurrent(binding) || !inputSettled || !confirmed
                    || inputText !== originalText || setbackText !== originalSetback) return false
            var accepted = binding.draft.setAreaPlanningPolicy(policy, binding.revision)
            if (!guard.alive || epoch !== contextEpoch || binding.draft !== draft || binding.source !== sourceIdentity) return false
            if (accepted !== true) return fail(qsTr("本地修改未确认：%1。请核对草稿实际版本；不会自动回滚可能已提交的修改。")
                .arg(draft.lastError || qsTr("草稿版本或输入已改变")))
            var committedRevision = binding.draft.revision
            function current(expectedSession, expectedEditing) {
                return guard.alive && guard.ticket === ticket && epoch === root.contextEpoch
                    && binding.draft === root.draft && binding.source === root.sourceIdentity
                    && committedRevision === binding.draft.revision && root.session === expectedSession
                    && root.editing === expectedEditing && root.inputText === originalText
                    && root.setbackText === originalSetback
            }
            // A synchronous signal may open a different edit while committing.
            if (!current(binding, true)) return retainInterruptedCommit(guard)
            session = null
            if (!current(null, true)) return retainInterruptedCommit(guard)
            confirmed = false
            if (!current(null, true)) return retainInterruptedCommit(guard)
            editing = false
            if (!current(null, false)) return retainInterruptedCommit(guard)
            message = qsTr("已同时保存本地排除区与规划退让声明，旧审阅已失效。必须重新生成／审阅全部路线；未发送指令。")
            return current(null, false) ? true : retainInterruptedCommit(guard)
        } finally {
            if (guard.alive && guard.ticket === ticket) {
                callBusy = false
                if (guard.alive && guard.ticket === ticket) callTicket = null
                if (guard.ticket === ticket) guard.ticket = null
            }
        }
    }

    QGCLabel {
        Layout.fillWidth: true; font.bold: true
        text: qsTr("区域边界策略 · %1 个排除区 · 规划退让 %2").arg(root.saved.length)
            .arg(root.savedSetbackText.length ? root.savedSetbackText + qsTr(" m") : qsTr("未声明"))
        wrapMode: Text.WordWrap
    }
    QGCLabel {
        Layout.fillWidth: true
        text: qsTr("仅约束名义二维路线，不是实际避障或净空认证。正退让要求所有主机、辅助机、备用预算和返程在原区域内并远离原边界；区域外端点将被拒绝。0 或未声明保留原无退让规则；数值不能代替机体尺寸、定位误差或动力学证据。")
        wrapMode: Text.WordWrap
    }
    QGCButton {
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
        text: qsTr("编辑排除区与规划退让…"); focusPolicy: Qt.StrongFocus
        visible: !root.editing; enabled: root.ready
        onClicked: root.begin()
    }
    ColumnLayout {
        Layout.fillWidth: true; visible: root.editing
        QGCLabel { Layout.fillWidth: true; text: qsTr("WGS84 顶点 JSON · 每个多边形单独一个数组"); wrapMode: Text.WordWrap }
        QGCLabel {
            Layout.fillWidth: true
            text: qsTr("格式：[[{\"latitude\": 纬度, \"longitude\": 经度}, …], …]。最多 8 环，每环 3–64 点；连同外边界最多 128 点。孔必须完全在区域内，不能相交、相触或嵌套。输入 [] 并确认可移除全部几何排除区。")
            wrapMode: Text.WordWrap
        }
        Controls.TextArea {
            id: coordinates
            Layout.fillWidth: true; Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 8
            text: root.inputText; selectByMouse: true; wrapMode: TextEdit.Wrap
            enabled: !root.callBusy
            Accessible.name: qsTr("区域内排除区 WGS84 顶点 JSON")
            onTextChanged: if (activeFocus) root.editText(text)
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
        }
        QGCLabel { Layout.fillWidth: true; text: qsTr("名义边界退让距离 (m) · 可选 0–500"); wrapMode: Text.WordWrap }
        Controls.TextField {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            text: root.setbackText; enabled: !root.callBusy
            selectByMouse: true
            Accessible.name: qsTr("名义边界退让距离，米，留空移除声明")
            onTextChanged: if (activeFocus) root.editSetback(text)
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
        }
        QGCLabel {
            Layout.fillWidth: true
            text: qsTr("不自动填安全值。留空明确移除退让声明，0 明确不退让；正值使候选区域外边界内缩、排除区外扩。过窄／分裂后飞机不足／端点不可达会阻断，不自动减小数值或遗漏子区域。地图边界仍是原输入，不是退让后安全域。")
            wrapMode: Text.WordWrap
        }
        QGCLabel {
            Layout.fillWidth: true; visible: !root.sourceCurrent
            text: qsTr("原编辑版本已不再是当前版本；原文保留，请取消后重新编辑。")
            color: qgcPal.colorOrange; wrapMode: Text.WordWrap
        }
        QGCCheckBox {
            Layout.fillWidth: true
            text: qsTr("确认替换孔和退让声明（非飞行批准）")
            checked: root.confirmed; enabled: root.ready && root.sourceCurrent
            onClicked: root.confirmed = checked
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            text: root.callBusy ? qsTr("正在保存本地编辑…") : qsTr("保存并使旧审阅失效")
            enabled: root.ready && root.sourceCurrent && root.confirmed; focusPolicy: Qt.StrongFocus
            onClicked: root.apply()
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            text: qsTr("取消本次编辑"); enabled: !root.callBusy; focusPolicy: Qt.StrongFocus
            onClicked: root.cancel()
        }
    }
    QGCLabel {
        id: feedback
        Layout.fillWidth: true; visible: text.length > 0
        text: root.message; textFormat: Text.PlainText; wrapMode: Text.WordWrap
        activeFocusOnTab: visible; Accessible.name: text
        onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
    }
}

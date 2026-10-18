import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Local planning policy only. Engineering declarations are NOT measured
// performance or flight permission. Unapplied edits remain separate from Draft.
ColumnLayout {
    id: root
    property var draft: null
    property string sourceIdentity: ""
    property bool inputSettled: false
    signal focusVisibilityRequested(var item)
    property bool editing: false
    property int policyIndex: 0
    property var floorForm: ({speedEnabled:false, speedText:"", enduranceEnabled:false, enduranceText:""})
    property bool confirmed: false
    property string message: ""
    property var session: null
    property bool callBusy: false
    property var callTicket: null
    property var contextEpoch: ({})
    property var lifetime: ({alive: true, ticket: null, consentEpoch:0, exhausted:false})
    readonly property bool pendingInput: editing || callBusy
    readonly property int savedIndex: indexForSettings(draft ? draft.settings : null)
    readonly property var savedPolicy: admissionForSettings(draft ? draft.settings : null)
    readonly property string savedPolicyToken: savedPolicy === null ? "" : JSON.stringify(savedPolicy)
    readonly property var proposedPolicy: admissionForInput()
    readonly property bool sourceCurrent: bindingCurrent(session)
    readonly property bool ready: visible && enabled && inputSettled && !callBusy && callTicket === null
        && !lifetime.exhausted && draft !== null && !draft.revisionExhausted && sourceIdentity.length > 0
        && savedIndex >= 0 && savedPolicy !== null && typeof draft.setPerformanceAdmissionPolicy === "function"
        && typeof draft.invalidatePreview === "function"
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    spacing: ScreenTools.defaultFontPixelWidth
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }
    Component.onDestruction: { lifetime.alive = false; lifetime.ticket = null }
    onDraftChanged: retireSource()
    onSourceIdentityChanged: retireSource()
    onInputSettledChanged: if (!inputSettled) revokeConsent()
    onVisibleChanged: if (!visible) revokeConsent()
    onEnabledChanged: if (!enabled) revokeConsent()
    onSourceCurrentChanged: if (!sourceCurrent && !callBusy && editing) revokeConsent()
    onConfirmedChanged: {
        if (lifetime.consentEpoch >= 9007199254740991) lifetime.exhausted = true
        else lifetime.consentEpoch += 1
    }

    function indexForSettings(value) {
        if (!value || typeof value !== "object" || Array.isArray(value)) return -1
        if (!Object.prototype.hasOwnProperty.call(value, "requirePerformanceEvidence")) return 0
        return value.requirePerformanceEvidence === true ? 2 : value.requirePerformanceEvidence === false ? 1 : -1
    }
    function policyText(index) {
        return index === 2 ? qsTr("严格：要求当前已审核且构型匹配的工程性能声明")
            : index === 1 ? qsTr("明确关闭严格性能准入（非性能达标证明）")
            : index === 0 ? qsTr("未声明strict政策（不移除本次门槛选择）") : qsTr("政策未知或格式无效，不能提交")
    }
    function validFloor(value) { return typeof value === "number" && isFinite(value) && value >= 0 && value <= 9007199254740991 }
    function admissionForSettings(value) {
        var index = indexForSettings(value), result = {}
        if (index < 0) return null
        if (index !== 0) result.requirePerformanceEvidence = index === 2
        var names = ["minimumSpeedMps", "minimumEnduranceSeconds"]
        for (var i = 0; i < names.length; ++i) if (Object.prototype.hasOwnProperty.call(value, names[i])) {
            if (!validFloor(value[names[i]])) return null
            result[names[i]] = value[names[i]]
        }
        return result
    }
    function floorNumber(value) {
        // Empty/units/exponents are not Number("") == 0. Only an explicit
        // finite non-negative decimal becomes a native numeric requirement.
        if (typeof value !== "string" || value.length < 1 || value.length > 384
                || !/^(0|[1-9][0-9]*)(\.[0-9]+)?$/.test(value)) return null
        var number = Number(value)
        return validFloor(number) && (number !== 0 || !/[1-9]/.test(value)) ? number : null
    }
    function decimalText(value) {
        // Preserve legitimate native small doubles without presenting an
        // exponent which this decimal-only form would reject on reopening.
        var text = String(value), parts = text.toLowerCase().split("e")
        if (parts.length === 1) return text
        var mantissa = parts[0].split("."), digits = mantissa.join(""), point = mantissa[0].length + Number(parts[1])
        if (point <= 0) return "0." + new Array(1 - point).join("0") + digits
        if (point >= digits.length) return digits + new Array(point - digits.length + 1).join("0")
        return digits.substring(0, point) + "." + digits.substring(point)
    }
    function admissionForInput() {
        if (policyIndex < 0 || policyIndex > 2) return null
        var form = floorForm, result = {}
        if (policyIndex !== 0) result.requirePerformanceEvidence = policyIndex === 2
        if (form.speedEnabled) {
            var speed = floorNumber(form.speedText)
            if (speed === null) return null
            result.minimumSpeedMps = speed
        }
        if (form.enduranceEnabled) {
            var endurance = floorNumber(form.enduranceText)
            if (endurance === null) return null
            result.minimumEnduranceSeconds = endurance
        }
        return result
    }
    function floorText(value, key, units) {
        if (value === null) return qsTr("资料未知／格式无效")
        return value && Object.prototype.hasOwnProperty.call(value, key) ? String(value[key]) + units : qsTr("未设置（缺省，不是0）")
    }
    function clearConfirmation() {
        var guard = lifetime, expected = guard.consentEpoch + (confirmed ? 1 : 0)
        confirmed = false
        return guard.alive && !guard.exhausted && guard.consentEpoch === expected && !confirmed
    }
    function revokeConsent() {
        var guard = lifetime, expected = guard.consentEpoch + (confirmed ? 1 : 0)
        var cleared = clearConfirmation()
        if (!guard.alive) return false
        var epoch = ({})
        contextEpoch = epoch
        return guard.alive && cleared && !guard.exhausted && contextEpoch === epoch
            && guard.consentEpoch === expected && !confirmed
    }
    function bindingCurrent(binding) {
        return visible && enabled && !!binding && binding.draft === draft && binding.source === sourceIdentity
            && sourceIdentity.length > 0 && draft && binding.revision === draft.revision
            && binding.policy === savedIndex && binding.admission === savedPolicyToken
    }
    function retireSource() {
        var guard = lifetime
        session = null
        if (!guard.alive) return
        revokeConsent()
        if (guard.alive && editing) message = qsTr("原草稿来源已改变。保留拟选政策，但不能保存到新草稿；请取消后重新编辑。")
    }
    function finishCall(guard, ticket) {
        if (!guard.alive || guard.ticket !== ticket) return
        callBusy = false
        if (guard.alive && guard.ticket === ticket) callTicket = null
        if (guard.ticket === ticket) guard.ticket = null
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
        if (!guard.alive || guard.exhausted || guard.ticket !== null || !ready || editing) return false
        var owner = draft, source = sourceIdentity, revision = owner.revision, originalPolicy = savedIndex
        var originalAdmission = savedPolicyToken, values = savedPolicy
        var ticket = ({}), epoch = contextEpoch, finish = finishCall
        guard.ticket = ticket
        try {
            callTicket = ticket
            if (!guard.alive) return false
            callBusy = true
            if (!guard.alive || epoch !== contextEpoch || owner !== draft || source !== sourceIdentity
                    || revision !== owner.revision || originalPolicy !== savedIndex || originalAdmission !== savedPolicyToken || !inputSettled || !visible || !enabled) return false
            var accepted = owner.invalidatePreview()
            if (!guard.alive || epoch !== contextEpoch || accepted !== true || owner !== draft || source !== sourceIdentity
                    || revision !== owner.revision || originalPolicy !== savedIndex || originalAdmission !== savedPolicyToken || !inputSettled || !visible || !enabled) return false
            policyIndex = originalPolicy
            if (!guard.alive || epoch !== contextEpoch) return false
            floorForm = {speedEnabled:Object.prototype.hasOwnProperty.call(values,"minimumSpeedMps"),
                speedText:Object.prototype.hasOwnProperty.call(values,"minimumSpeedMps")?decimalText(values.minimumSpeedMps):"",
                enduranceEnabled:Object.prototype.hasOwnProperty.call(values,"minimumEnduranceSeconds"),
                enduranceText:Object.prototype.hasOwnProperty.call(values,"minimumEnduranceSeconds")?decimalText(values.minimumEnduranceSeconds):""}
            if (!guard.alive || epoch !== contextEpoch || owner !== draft || revision !== owner.revision || originalAdmission !== savedPolicyToken) return false
            session = {draft: owner, source: source, revision: revision, policy: originalPolicy, admission:originalAdmission}
            if (!guard.alive || epoch !== contextEpoch) return false
            if (!clearConfirmation() || !guard.alive || epoch !== contextEpoch) return false
            message = ""
            if (!guard.alive || epoch !== contextEpoch) return false
            editing = true
            if (!guard.alive || epoch !== contextEpoch || !bindingCurrent(session) || !inputSettled) return false
            cancelButton.forceActiveFocus()
            if (guard.alive) focusVisibilityRequested(cancelButton)
            return guard.alive
        } finally { finish(guard, ticket) }
    }
    function choose(index) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.ticket !== null || !editing || callBusy || !sourceCurrent
                || !ready || index < 0 || index > 2 || Math.floor(index) !== index || index === policyIndex) return
        var binding = session, ticket = ({}), original = policyIndex, finish = finishCall
        guard.ticket = ticket
        try {
            callTicket = ticket
            if (!guard.alive) return
            callBusy = true
            if (!guard.alive || session !== binding || policyIndex !== original || !bindingCurrent(binding) || !inputSettled) return
            // Retire old consent BEFORE publishing the new policy selection.
            if (!revokeConsent()) return
            if (!guard.alive || session !== binding || policyIndex !== original || !bindingCurrent(binding) || !inputSettled) return
            var epoch = contextEpoch
            policyIndex = index
            if (!guard.alive || epoch !== contextEpoch || session !== binding || policyIndex !== index) return
            message = ""
        } finally { finish(guard, ticket) }
    }
    function editFloor(key, value) {
        var guard = lifetime, binding = session, form = floorForm, finish = finishCall
        if (!guard.alive || guard.exhausted || guard.ticket || !editing || callBusy || !ready || !bindingCurrent(binding)
                || ["speedEnabled","speedText","enduranceEnabled","enduranceText"].indexOf(key) < 0
                || ((key === "speedEnabled" || key === "enduranceEnabled") ? typeof value !== "boolean" : typeof value !== "string")
                || form[key] === value) return
        var ticket = ({}), next = {speedEnabled:form.speedEnabled,speedText:form.speedText,
            enduranceEnabled:form.enduranceEnabled,enduranceText:form.enduranceText}
        next[key] = value
        guard.ticket = ticket
        try {
            // Pure text edits hold the plain reentry lease without toggling
            // enabled/focus for every keystroke. No native operation occurs.
            if (!guard.alive || session !== binding || floorForm !== form || !bindingCurrent(binding) || !inputSettled) return
            if (!revokeConsent()) return
            if (!guard.alive || session !== binding || floorForm !== form || !bindingCurrent(binding) || !inputSettled) return
            var epoch = contextEpoch
            floorForm = next
            if (!guard.alive || epoch !== contextEpoch || session !== binding || floorForm !== next) return
            message = ""
        } finally { finish(guard, ticket) }
    }
    function cancel() {
        var guard = lifetime
        if (!guard.alive || guard.ticket !== null || callBusy) return
        var binding = session, choice = policyIndex, ticket = ({}), epoch = contextEpoch, originalEditing = editing, finish = finishCall
        guard.ticket = ticket
        function current(expectedSession, expectedEditing) {
            return guard.alive && guard.ticket === ticket && epoch === root.contextEpoch
                && root.session === expectedSession && root.editing === expectedEditing && root.policyIndex === choice
        }
        try {
            callTicket = ticket
            if (!current(binding, originalEditing)) return
            callBusy = true
            if (!current(binding, originalEditing)) return
            confirmed = false
            if (!current(binding, originalEditing)) return
            session = null
            if (!current(null, originalEditing)) return
            editing = false
            if (!current(null, false)) return
            message = ""
        } finally { finish(guard, ticket) }
    }
    function pressSnapshot() {
        return {binding: session, epoch: contextEpoch, policy: policyIndex, form:floorForm,
            admission:proposedPolicy === null ? "" : JSON.stringify(proposedPolicy), consentEpoch:lifetime.consentEpoch}
    }
    function apply(pressed) {
        var guard = lifetime, binding = session, choice = policyIndex, epoch = contextEpoch, form = floorForm
        var proposed = proposedPolicy, admission = proposed === null ? "" : JSON.stringify(proposed)
        if (!guard.alive || guard.exhausted || guard.consentEpoch >= 9007199254740991
                || guard.ticket !== null || !ready || !editing || !confirmed || !bindingCurrent(binding)
                || choice < 0 || choice > 2 || !pressed || pressed.binding !== binding
                || pressed.epoch !== epoch || pressed.policy !== choice || pressed.form !== form
                || !admission || pressed.admission !== admission || pressed.consentEpoch !== guard.consentEpoch) return false
        var ticket = ({}), finish = finishCall, consentEpoch = guard.consentEpoch
        var values = JSON.parse(admission)
        guard.ticket = ticket
        try {
            callTicket = ticket
            if (!guard.alive) return false
            callBusy = true
            if (!guard.alive || epoch !== contextEpoch || session !== binding || policyIndex !== choice
                    || guard.exhausted || floorForm !== form || JSON.stringify(proposedPolicy) !== admission || guard.consentEpoch !== consentEpoch
                    || !bindingCurrent(binding) || !inputSettled || !confirmed) return false
            var accepted = binding.draft.setPerformanceAdmissionPolicy(values, binding.revision)
            if (!guard.alive) return false
            if (accepted !== true) return fail(qsTr("本地修改未确认。请核对实际草稿版本；可能已保存，不自动重试或推断回滚。"))
            // Presence and explicit zero participate in the atomic identity.
            // A strict-only comparison would miss floor changes/no-op saves.
            var expectedRevision = binding.revision + (admission === binding.admission ? 0 : 1)
            function current(expectedSession, expectedEditing, expectedConsent) {
                return guard.alive && !guard.exhausted && guard.ticket === ticket && epoch === root.contextEpoch && visible && enabled
                    && binding.draft === root.draft && binding.source === root.sourceIdentity && inputSettled
                    && binding.draft.revision === expectedRevision && root.savedIndex === choice
                    && root.savedPolicyToken === admission && root.floorForm === form
                    && JSON.stringify(root.proposedPolicy) === admission && guard.consentEpoch === expectedConsent
                    && root.session === expectedSession && root.editing === expectedEditing && root.policyIndex === choice
            }
            if (!current(binding, true, consentEpoch)) return fail(qsTr("保存可能已发生，但收尾时来源或输入改变。保留拟选政策，请核对原草稿；不自动重试或回滚。"))
            confirmed = false
            var consumedConsent = consentEpoch + 1
            if (!current(binding, true, consumedConsent)) return false
            session = null
            if (!current(null, true, consumedConsent)) return false
            editing = false
            if (!current(null, false, consumedConsent)) return false
            message = qsTr("已一次保存本地strict政策及两项可选门槛，旧审阅已失效。须重新生成与审阅；未修改速度／能量预算、机载参数或批准，也未认证真实性能。")
            return current(null, false, consumedConsent)
        } finally { finish(guard, ticket) }
    }

    QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("性能准入政策 · %1").arg(root.policyText(root.savedIndex)) }
    QGCLabel {
        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
        text: qsTr("严格模式要求每架主／备用机有当前工程能力批准和完整匹配构型基线，按本机声明速度／续航做规划与预算；未知不能冒充达标。声明仍非实测认证。关闭或旧缺省不移除已明确设置的正数门槛，也不证明飞机性能足够；其它安全门禁不会因此关闭。")
    }
    QGCLabel {
        Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap
        text:qsTr("已保存任务门槛 · 最低速度能力：%1；最低续航能力：%2。0表示明确没有正数下限；缺省表示未声明，两者不会互相替代。")
            .arg(root.floorText(root.savedPolicy,"minimumSpeedMps"," m/s"))
            .arg(root.floorText(root.savedPolicy,"minimumEnduranceSeconds",qsTr(" 秒")))
    }
    QGCButton {
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: qsTr("编辑性能政策和任务门槛…"); visible: !root.editing; enabled: root.ready; Accessible.name: text
        onClicked: root.begin()
    }
    ColumnLayout {
        Layout.fillWidth: true; visible: root.editing
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("拟政策（改变选择会撤销本次确认；尚未保存时不用于预览／归档）") }
        Controls.ComboBox {
            id: policyChoice
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            model: [qsTr("未声明 · 仅移除strict字段"), qsTr("明确关闭 · 保留false声明"), qsTr("严格开启 · 要求当前审核性能依据")]
            currentIndex: root.policyIndex; enabled: root.ready && root.sourceCurrent
            Accessible.name: qsTr("拟性能准入政策，未声明、明确关闭或严格开启")
            onActivated: { var guard=root.lifetime; root.choose(index); if(guard.alive)currentIndex = Qt.binding(function() { return root.policyIndex }) }
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(policyChoice)
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.policyText(root.policyIndex) }
        QGCLabel {
            Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap
            text:qsTr("可选任务门槛与strict政策分别选择，再一次保存。速度是任务要求的最低速度能力，不是本次巡航速度或对声明max的改写；最低续航能力（秒）不是当前剩余电量／飞行时间或试飞认证。严格模式只用当前本机工程声明；非严格仍只按原规划描述检查。不会自动改速度、备用／返程预算或批准。")
        }
        Controls.CheckBox {
            Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
            text:qsTr("设置最低速度能力门槛"); Accessible.name:text
            enabled:root.ready&&root.sourceCurrent; checked:root.floorForm.speedEnabled
            onToggled:root.editFloor("speedEnabled",checked)
        }
        QGCLabel { Layout.fillWidth:true; text:qsTr("最低速度能力（m/s）"); textFormat:Text.PlainText; wrapMode:Text.WordWrap }
        QGCTextField {
            id:speedField
            Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; maximumLength:384; numericValuesOnly:true
            enabled:root.ready&&root.sourceCurrent&&root.floorForm.speedEnabled
            text:root.floorForm.speedText; placeholderText:qsTr("启用后明确输入数值；空白不是0")
            Accessible.name:qsTr("任务最低速度能力，米每秒")
            onTextChanged:root.editFloor("speedText",text)
            onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(speedField)
        }
        QGCLabel {
            Layout.fillWidth:true; visible:root.floorForm.speedEnabled&&root.floorNumber(root.floorForm.speedText)===null
            color:qgcPal.warningText; textFormat:Text.PlainText; wrapMode:Text.WordWrap
            text:qsTr("速度门槛请输入0到9007199254740991的有限非负十进制数；不含空格、单位、逗号或指数。空白／下溢不变成0；不需要时取消勾选，保存后移除此键。")
        }
        Controls.CheckBox {
            Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
            text:qsTr("设置最低续航能力门槛"); Accessible.name:text
            enabled:root.ready&&root.sourceCurrent; checked:root.floorForm.enduranceEnabled
            onToggled:root.editFloor("enduranceEnabled",checked)
        }
        QGCLabel { Layout.fillWidth:true; text:qsTr("最低续航能力（秒）"); textFormat:Text.PlainText; wrapMode:Text.WordWrap }
        QGCTextField {
            id:enduranceField
            Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; maximumLength:384; numericValuesOnly:true
            enabled:root.ready&&root.sourceCurrent&&root.floorForm.enduranceEnabled
            text:root.floorForm.enduranceText; placeholderText:qsTr("启用后明确输入数值；空白不是0")
            Accessible.name:qsTr("任务最低续航能力，秒，不是当前剩余时间")
            onTextChanged:root.editFloor("enduranceText",text)
            onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(enduranceField)
        }
        QGCLabel {
            Layout.fillWidth:true; visible:root.floorForm.enduranceEnabled&&root.floorNumber(root.floorForm.enduranceText)===null
            color:qgcPal.warningText; textFormat:Text.PlainText; wrapMode:Text.WordWrap
            text:qsTr("续航门槛请输入0到9007199254740991的有限非负十进制秒数；不含空格、单位、逗号或指数。空白／下溢不变成0；不需要时取消勾选，保存后移除此键。")
        }
        QGCLabel {
            Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap
            text:qsTr("将一次保存 · %1\n最低速度能力：%2；最低续航能力：%3。未勾选的门槛会明确移除，不沿用旧值。")
                .arg(root.policyText(root.policyIndex))
                .arg(root.proposedPolicy===null?qsTr("输入待完成"):root.floorText(root.proposedPolicy,"minimumSpeedMps"," m/s"))
                .arg(root.proposedPolicy===null?qsTr("输入待完成"):root.floorText(root.proposedPolicy,"minimumEnduranceSeconds",qsTr(" 秒")))
        }
        QGCLabel { Layout.fillWidth: true; visible: !root.sourceCurrent; color: qgcPal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("原草稿／版本已变化；拟选政策保留，但不能提交到新来源。请取消后重新编辑。") }
        Controls.CheckBox {
            id: consent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("确认一次保存三态strict政策和上述两项可选门槛到原草稿；关闭／移除也会撤旧任务审阅，须重新审阅，不是性能认证或飞行放行。")
            checked: root.confirmed; enabled: root.ready && root.sourceCurrent && root.proposedPolicy!==null; Accessible.name: text
            contentItem: QGCLabel { text: consent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (consent.indicator ? consent.indicator.width : 0) + ScreenTools.defaultFontPixelWidth }
            onToggled: root.confirmed = checked
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(consent)
        }
        QGCButton {
            id: applyButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.callBusy ? qsTr("正在保存本地政策和门槛…") : qsTr("一次保存政策／门槛并撤旧审阅")
            enabled: root.ready && root.sourceCurrent && root.confirmed && root.proposedPolicy!==null; Accessible.name: text
            property var pressScope: ({})
            onPressed: pressScope = root.pressSnapshot()
            onCanceled: pressScope = ({})
            onEnabledChanged:if(!enabled)pressScope=({})
            onClicked: { var guard=root.lifetime, original = pressScope; pressScope = ({}); if(guard.alive)root.apply(original) }
            Keys.onReturnPressed: event.accepted = true
            Keys.onEnterPressed: event.accepted = true
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(applyButton)
        }
        QGCButton {
            id: cancelButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("取消本次政策编辑"); enabled: !root.callBusy; Accessible.name: text
            onClicked: root.cancel()
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(cancelButton)
        }
    }
    QGCLabel {
        id: feedback
        Layout.fillWidth: true; visible: text.length > 0
        text: root.message; textFormat: Text.PlainText; wrapMode: Text.WordWrap
        activeFocusOnTab: visible; Accessible.name: text
        onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(feedback)
    }
}

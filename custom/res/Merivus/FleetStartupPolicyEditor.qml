import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Local policy authoring only. The parent owns the sole canonical text and
// must compare both signal expectations before accepting an edit.
ColumnLayout {
    id: root
    property string policyText: ""
    property bool editingEnabled: false
    property var sessionToken: null
    signal policyEdited(string nextText, var expectedSession, string expectedOriginalText)
    signal focusVisibilityRequested(var item)
    readonly property bool pendingInput: draftOpen
    readonly property real controlHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var canonical: inspect(policyText)
    readonly property bool sourceCurrent: draftOpen && sessionToken === originalSession && policyText === originalText
    readonly property bool canEdit: editingEnabled && visible && editorPopup.visible && sourceCurrent && !emitting && !exhausted
    property bool draftOpen: false
    property int version: 0
    property bool clearing: false
    property bool replacingInvalid: false
    property string originalText: ""
    property var originalSession: null
    property var fields: ({})
    property var preview: null
    property bool confirmed: false
    property var pressedIntent: null
    property double revision: 0
    property bool exhausted: false
    property bool emitting: false
    property string message: ""
    property var lifetime: ({alive: true, ticket: null})
    readonly property var definitions: [
        {group:"time", key:"maximumRawGapMs", label:qsTr("原始报文最大间隙"), unit:"ms", min:100, max:3000, integer:true},
        {group:"time", key:"maximumPhaseDurationMs", label:qsTr("单阶段最长持续时间"), unit:"ms", min:100, max:30000, integer:true},
        {group:"postModeEnvelope", key:"minAltitudeAmslMeters", label:qsTr("最低 AMSL 海拔（非离地高度）"), unit:"m", min:-1000, max:20000},
        {group:"postModeEnvelope", key:"maxAltitudeAmslMeters", label:qsTr("最高 AMSL 海拔（须大于最低值）"), unit:"m", min:-1000, max:20000},
        {group:"postModeEnvelope", key:"horizontalCorridorMeters", label:qsTr("水平走廊上限"), unit:"m", min:0.01, max:1000},
        {group:"postModeEnvelope", key:"maxHorizontalSpeedMetersPerSecond", label:qsTr("水平速度上限"), unit:"m/s", min:0.01, max:300},
        {group:"postModeEnvelope", key:"maxClimbSpeedMetersPerSecond", label:qsTr("爬升速度上限"), unit:"m/s", min:0.01, max:20},
        {group:"postModeEnvelope", key:"maxDescentSpeedMetersPerSecond", label:qsTr("下降速度上限"), unit:"m/s", min:0.01, max:20},
        {group:"postModeEnvelope", key:"maxRelativeAltitudeMagnitudeMeters", label:qsTr("相对高度绝对值上限"), unit:"m", min:0.01, max:21000},
        {group:"localMotionEnvelope", key:"maxHorizontalDisplacementMeters", label:qsTr("局部水平位移上限"), unit:"m", min:0.01, max:1000},
        {group:"localMotionEnvelope", key:"maxClimbDisplacementMeters", label:qsTr("局部爬升位移上限"), unit:"m", min:0.01, max:1000},
        {group:"localMotionEnvelope", key:"maxDescentDisplacementMeters", label:qsTr("局部下降位移上限"), unit:"m", min:0.01, max:1000},
        {group:"localMotionEnvelope", key:"maxHorizontalSpeedMetersPerSecond", label:qsTr("局部水平速度上限（不高于全球同项）"), unit:"m/s", min:0.01, max:300},
        {group:"localMotionEnvelope", key:"maxClimbSpeedMetersPerSecond", label:qsTr("局部爬升速度上限（不高于全球同项）"), unit:"m/s", min:0.01, max:20},
        {group:"localMotionEnvelope", key:"maxDescentSpeedMetersPerSecond", label:qsTr("局部下降速度上限（不高于全球同项）"), unit:"m/s", min:0.01, max:20},
        {group:"headingEnvelope", key:"maxDeviationDegrees", label:qsTr("航向相对审阅值最大偏离"), unit:"°", min:0.01, max:180},
        {group:"headingEnvelope", key:"maxAdjacentChangeDegrees", label:qsTr("相邻航向最大变化（不大于偏离上限）"), unit:"°", min:0.01, max:180},
        {group:"headingEnvelope", key:"maxObservedTravelDegrees", label:qsTr("累计已观察转角上限（不小于相邻上限）"), unit:"°", min:0.01, max:3600},
        {group:"homeUpdateEnvelope", key:"maximumHomeUpdates", label:qsTr("HOME 更新次数上限"), unit:qsTr("次"), min:1, max:128, integer:true},
        {group:"homeUpdateEnvelope", key:"maximumPreModeRelativeAltitudeChanges", label:qsTr("进入任务模式前相对高度变化次数上限"), unit:qsTr("次"), min:1, max:256, integer:true},
        {group:"homeUpdateEnvelope", key:"maxHorizontalDisplacementMeters", label:qsTr("HOME 全球水平位移上限（允许 0）"), unit:"m", min:0, max:1000},
        {group:"homeUpdateEnvelope", key:"maxAltitudeDisplacementMeters", label:qsTr("HOME 海拔位移上限（允许 0）"), unit:"m", min:0, max:1000},
        {group:"homeUpdateEnvelope", key:"maxLocalHorizontalDisplacementMeters", label:qsTr("HOME 局部水平位移上限（允许 0）"), unit:"m", min:0, max:1000},
        {group:"homeUpdateEnvelope", key:"maxLocalVerticalDisplacementMeters", label:qsTr("HOME 局部垂直位移上限（允许 0）"), unit:"m", min:0, max:1000},
        {group:"homeUpdateEnvelope", key:"maxPreModeRelativeAltitudeChangeMeters", label:qsTr("进入任务模式前相对高度变化上限（允许 0）"), unit:"m", min:0, max:1000},
        {group:"altitudeReferenceEnvelope", key:"maxAmslMinusLocalDriftMeters", label:qsTr("AMSL-local 相对原审阅差值的最大变化（允许 0）"), unit:"m", min:0, max:1000},
        {group:"altitudeReferenceEnvelope", key:"maxAmslMinusRelativeDriftMeters", label:qsTr("AMSL-relative 相对原审阅差值的最大变化（允许 0）"), unit:"m", min:0, max:1000},
        {group:"altitudeReferenceEnvelope", key:"maxRelativeMinusLocalDriftMeters", label:qsTr("relative-local 相对原审阅差值的最大变化（允许 0）"), unit:"m", min:0, max:1000},
        {group:"altitudeReferenceEnvelope", key:"maxAmslMinusLocalToOriginResidualMeters", label:qsTr("AMSL-local 对原点报告海拔的最大残差（允许 0）"), unit:"m", min:0, max:1000},
        {group:"altitudeReferenceEnvelope", key:"maxAmslMinusRelativeToHomeResidualMeters", label:qsTr("AMSL-relative 对 HOME 报告海拔的最大残差（允许 0）"), unit:"m", min:0, max:1000},
        {group:"altitudeReferenceEnvelope", key:"maxRelativeMinusLocalToHomeLocalZResidualMeters", label:qsTr("relative-local 对 HOME localZ 的最大残差（允许 0）"), unit:"m", min:0, max:1000}
    ]
    readonly property var shownDefinitions: definitions.filter(function(d) { return relevant(d, version) })
    spacing: ScreenTools.defaultFontPixelWidth
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function object(value) { return value !== null && typeof value === "object" && !Array.isArray(value) }
    function exact(value, keys) {
        if (!object(value) || Object.keys(value).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, keys[i])) return false
        return true
    }
    function relevant(d, schema) {
        return schema >= 1 && (d.group === "time" || d.group === "postModeEnvelope"
            || (schema >= 2 && (d.group === "localMotionEnvelope" || d.group === "headingEnvelope"))
            || (schema === 3 && d.group === "homeUpdateEnvelope")
            || (schema === 4 && d.group === "altitudeReferenceEnvelope"))
    }
    function fieldKey(d) { return d.group + "." + d.key }
    function numeric(text) {
        if (typeof text !== "string") return NaN
        var trimmed = text.trim()
        return /^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(trimmed) ? Number(trimmed) : NaN
    }
    function validNumber(value, d) {
        return typeof value === "number" && isFinite(value) && value >= d.min && value <= d.max
            && (d.integer !== true || Math.floor(value) === value)
    }
    function rangeText(d) {
        return qsTr("%1 至 %2 %3；%4").arg(d.min).arg(d.max).arg(d.unit)
            .arg(d.integer === true ? qsTr("必须为整数") : qsTr("有限十进制数"))
    }
    function fieldError(d) {
        return validNumber(numeric(fields[fieldKey(d)]), d) ? "" : qsTr("%1：%2。空白不是 0。").arg(d.label).arg(rangeText(d))
    }
    function groupKeys(group) {
        var keys = []
        for (var i = 0; i < definitions.length; ++i) if (definitions[i].group === group) keys.push(definitions[i].key)
        if (group === "localMotionEnvelope" || group === "homeUpdateEnvelope" || group === "altitudeReferenceEnvelope") keys.push("referenceBasis")
        return keys
    }
    function policyError(value) {
        if (!object(value) || typeof value.schemaVersion !== "number" || [1,2,3,4].indexOf(value.schemaVersion) < 0)
            return qsTr("策略须为完整对象，并明确选择整数版本 1、2、3 或 4。")
        var schema = value.schemaVersion
        var keys = ["schemaVersion", "homeChangeHandling", "postModeEnvelope", "headingHandling", "maximumRawGapMs", "maximumPhaseDurationMs"]
        if (schema >= 2) keys = keys.concat(["localMotionEnvelope", "headingEnvelope"])
        if (schema === 3) keys.push("homeUpdateEnvelope")
        if (schema === 4) keys.push("altitudeReferenceEnvelope")
        if (!exact(value, keys)) return qsTr("该版本存在缺失或未知字段，不能自动丢弃或迁移。")
        if (value.homeChangeHandling !== (schema === 3 ? "bounded-observed-after-owned-arm-or-mode" : "stop-and-review")
                || value.headingHandling !== (schema >= 2 ? "bounded-observed" : "preserve-reviewed"))
            return qsTr("HOME 或航向处理声明与所选版本不一致。")
        var groups = ["postModeEnvelope"]
        if (schema >= 2) groups = groups.concat(["localMotionEnvelope", "headingEnvelope"])
        if (schema === 3) groups.push("homeUpdateEnvelope")
        if (schema === 4) groups.push("altitudeReferenceEnvelope")
        for (var g = 0; g < groups.length; ++g)
            if (!exact(value[groups[g]], groupKeys(groups[g]))) return qsTr("%1 包线存在缺失或未知字段。").arg(groups[g])
        if (schema >= 2 && value.localMotionEnvelope.referenceBasis !== "observed-native-odometry-ned-not-origin-proof")
            return qsTr("局部运动参考声明不符合原生 ODOMETRY / NED 观察契约。")
        if (schema === 3 && value.homeUpdateEnvelope.referenceBasis !== "original-reviewed-home-observation-not-frame-rebase")
            return qsTr("HOME 参考声明必须保留原审阅观察，不是坐标系重新基准。")
        if (schema === 4 && value.altitudeReferenceEnvelope.referenceBasis !== "reported-altitude-relations-not-origin-validity-or-same-sample-proof")
            return qsTr("高度参考声明必须限定为报文关系，不证明共原点、内部有效性或同一采样。")
        for (var i = 0; i < definitions.length; ++i) {
            var d = definitions[i]
            if (!relevant(d, schema)) continue
            var number = d.group === "time" ? value[d.key] : value[d.group][d.key]
            if (!validNumber(number, d)) return qsTr("%1：要求实际数值，%2；拒绝字符串、空值和布尔值。").arg(d.label).arg(rangeText(d))
        }
        var post = value.postModeEnvelope
        if (post.minAltitudeAmslMeters >= post.maxAltitudeAmslMeters) return qsTr("最低 AMSL 海拔必须严格小于最高海拔。")
        if (schema >= 2) {
            var speedKeys = ["maxHorizontalSpeedMetersPerSecond", "maxClimbSpeedMetersPerSecond", "maxDescentSpeedMetersPerSecond"]
            for (var s = 0; s < speedKeys.length; ++s)
                if (value.localMotionEnvelope[speedKeys[s]] > post[speedKeys[s]]) return qsTr("局部水平、爬升和下降速度上限各自不得高于全球包线同项；相等允许。")
            if (value.headingEnvelope.maxAdjacentChangeDegrees > value.headingEnvelope.maxDeviationDegrees)
                return qsTr("相邻航向变化上限不得大于相对审阅值的偏离上限。")
            if (value.headingEnvelope.maxObservedTravelDegrees < value.headingEnvelope.maxAdjacentChangeDegrees)
                return qsTr("累计已观察转角上限不得小于相邻变化上限。")
        }
        return ""
    }
    function inspect(text) {
        if (typeof text !== "string" || text.length > 16384) return {ok:false, error:qsTr("策略文本超过 16384 字符上限；原文保留，请在高级文本中修正。"), policy:null, absent:false}
        if (text.trim().length === 0) return {ok:true, error:"", policy:null, absent:true}
        var value
        try { value = JSON.parse(text) } catch (error) { return {ok:false, error:qsTr("原策略不是有效 JSON；不自动修补或覆盖。"), policy:null, absent:false} }
        var problem = policyError(value)
        return {ok:problem.length === 0, error:problem, policy:problem.length === 0 ? value : null, absent:false}
    }
    function clearConfirmation() { preview = null; confirmed = false; pressedIntent = null }
    function changed() {
        clearConfirmation()
        message = ""
        if (revision >= 9007199254740991) exhausted = true
        else revision += 1
    }
    function reset() {
        draftOpen = false; originalSession = null; originalText = ""
        version = 0; fields = ({}); clearing = false; replacingInvalid = false
        clearConfirmation(); message = ""
        if (revision >= 9007199254740991) exhausted = true
        else revision += 1
        if (editorPopup.visible) editorPopup.close()
    }
    function load() {
        if (pendingInput || emitting || !editingEnabled || !visible || exhausted) return false
        var guard = lifetime, expected = sessionToken, text = policyText
        var checked = inspect(policyText)
        if (!checked.ok) { message = checked.error; return false }
        originalText = policyText; originalSession = sessionToken
        var next = {}, value = checked.policy
        version = value ? value.schemaVersion : 0
        for (var i = 0; i < definitions.length; ++i) {
            var d = definitions[i]
            if (value && relevant(d, version)) next[fieldKey(d)] = String(d.group === "time" ? value[d.key] : value[d.group][d.key])
        }
        fields = next; clearing = false; replacingInvalid = false
        draftOpen = true
        if (!guard.alive) return false
        if (!draftOpen || originalSession !== expected || sessionToken !== expected || policyText !== text) return false
        changed()
        return true
    }
    function openEditor() {
        if ((!editingEnabled && !pendingInput) || !visible || emitting) return
        var guard = lifetime, expected = sessionToken, text = policyText
        if (!pendingInput) load()
        if (!guard.alive) return
        if (!visible || sessionToken !== expected || policyText !== text) return
        editorPopup.open()
    }
    function startReplacement() {
        if (!editingEnabled || !visible || emitting || !editorPopup.visible || pendingInput || canonical.ok || exhausted) return
        var guard = lifetime, expected = sessionToken, text = policyText
        originalText = policyText; originalSession = sessionToken
        fields = ({}); version = 0; clearing = false; replacingInvalid = true
        draftOpen = true
        if (!guard.alive) return
        if (!draftOpen || originalSession !== expected || sessionToken !== expected || policyText !== text) return
        changed()
        message = qsTr("正在新建替换表单。原文尚未更改；只有预览、确认并采用后才整体替换。")
    }
    function discard() {
        if (emitting) return
        var guard = lifetime
        reset()
        if (guard.alive) message = qsTr("已放弃未采用的表单；父配置中的策略文本未被修改。")
    }
    function editField(d, text) {
        if (!canEdit) return
        var next = {}
        for (var key in fields) if (Object.prototype.hasOwnProperty.call(fields, key)) next[key] = fields[key]
        next[fieldKey(d)] = text; fields = next; changed()
    }
    function selectVersion(schema) {
        if (!canEdit || [1,2,3,4].indexOf(schema) < 0) return
        version = schema; clearing = false; changed()
    }
    function stageClear() {
        if (!canEdit) return
        clearing = true; changed()
        message = qsTr("待移除策略：预览并确认后才采用空文本。缺策略仍会阻断受控启动，不会解除飞行限制。")
    }
    function build() {
        if (!sourceCurrent) return {ok:false, error:qsTr("父配置或编辑会话已变化。未采用内容仍保留；请放弃后重新读取，不能覆盖新来源。"), policy:null, text:""}
        if (clearing) return {ok:true, error:"", policy:null, text:""}
        if ([1,2,3,4].indexOf(version) < 0) return {ok:false, error:qsTr("请明确选择版本；未选版本不会创建默认策略。"), policy:null, text:""}
        var value = {schemaVersion:version, homeChangeHandling:version === 3 ? "bounded-observed-after-owned-arm-or-mode" : "stop-and-review",
            headingHandling:version >= 2 ? "bounded-observed" : "preserve-reviewed", postModeEnvelope:{}}
        if (version >= 2) {
            value.localMotionEnvelope = {referenceBasis:"observed-native-odometry-ned-not-origin-proof"}
            value.headingEnvelope = {}
        }
        if (version === 3) value.homeUpdateEnvelope = {referenceBasis:"original-reviewed-home-observation-not-frame-rebase"}
        if (version === 4) value.altitudeReferenceEnvelope = {referenceBasis:"reported-altitude-relations-not-origin-validity-or-same-sample-proof"}
        for (var i = 0; i < definitions.length; ++i) {
            var d = definitions[i]
            if (!relevant(d, version)) continue
            var number = numeric(fields[fieldKey(d)])
            if (!validNumber(number, d)) return {ok:false, error:fieldError(d), policy:null, text:""}
            if (d.group === "time") value[d.key] = number
            else value[d.group][d.key] = number
        }
        var problem = policyError(value)
        return {ok:problem.length === 0, error:problem, policy:problem.length === 0 ? value : null,
            text:problem.length === 0 ? JSON.stringify(value, null, 2) : ""}
    }
    function validate() { var result = build(); return {ok:result.ok, error:result.error} }
    function previewPolicy() {
        if (!canEdit || lifetime.ticket) return
        clearConfirmation()
        var value = build()
        if (!value.ok) { message = value.error; status.forceActiveFocus(); ensureVisible(status); return }
        preview = {text:value.text, revision:revision, session:originalSession, original:originalText, clearing:clearing}
        message = qsTr("结构与范围检查通过；这不是性能、净空、数据质量或启动许可验证。请核对完整预览后采用。")
    }
    function previewCurrent(value) {
        return value && canEdit && value.revision === revision && value.session === originalSession
            && value.original === originalText && sourceCurrent && preview === value
    }
    function adopt(intent) {
        if (!confirmed || !previewCurrent(intent) || lifetime.ticket) return
        var guard = lifetime, ticket = {}, expected = originalSession, original = originalText, text = intent.text
        guard.ticket = ticket; emitting = true
        try {
            if (!guard.alive || sessionToken !== expected || policyText !== original || preview !== intent) return
            policyEdited(text, expected, original)
            if (!guard.alive) return
            if (sessionToken === expected && policyText === text && originalSession === expected && preview === intent) {
                reset()
                message = qsTr("策略已采用到父配置的未保存文本。仍须核对参数、保存完整配置并重新审阅；未发送飞行指令。")
            } else {
                clearConfirmation()
                message = qsTr("采用后的父文本或会话已变化，结果不能确认为当前；请核对父配置。未自动重试或回滚已发生的文本变更。")
            }
        } catch (error) {
            if (guard.alive) { clearConfirmation(); message = qsTr("采用回执中断；父文本可能已变化，请核对后再操作。未自动重试。") }
        } finally {
            if (guard.alive) emitting = false
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    function sectionTitle(group) {
        if (group === "time") return qsTr("时间边界 · 不会续期原审阅或认证")
        if (group === "postModeEnvelope") return qsTr("全球运动包线 · 任务模式后")
        if (group === "localMotionEnvelope") return qsTr("局部运动包线 · 原生 ODOMETRY / NED 观察")
        if (group === "headingEnvelope") return qsTr("航向包线 · 已观察变化，不是包间运动证明")
        if (group === "altitudeReferenceEnvelope") return qsTr("高度关系包线 · 报文差值与原参考，不是内部有效性证明")
        return qsTr("HOME 更新包线 · 基于原审阅观察，不重新定义坐标系")
    }
    function ensureVisible(item) {
        if (!editorPopup.visible || !item) return
        var point = item.mapToItem(formColumn, 0, 0), margin = ScreenTools.defaultFontPixelHeight
        if (point.y < formScroll.contentY + margin) formScroll.contentY = Math.max(0, point.y - margin)
        else if (point.y + item.height > formScroll.contentY + formScroll.height - margin)
            formScroll.contentY = Math.min(Math.max(0, formScroll.contentHeight - formScroll.height), point.y + item.height - formScroll.height + margin)
    }
    onPolicyTextChanged: { confirmed = false; pressedIntent = null; if (!emitting) preview = null }
    onSessionTokenChanged: reset()
    onEditingEnabledChanged: if (!editingEnabled) { clearConfirmation(); if (editorPopup.visible) editorPopup.close() }
    onVisibleChanged: if (!visible) { clearConfirmation(); if (editorPopup.visible) editorPopup.close() }
    Component.onDestruction: lifetime.alive = false

    QGCLabel {
        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
        text: root.pendingInput ? qsTr("启动策略有未采用的表单，完整配置保存暂不可用。关闭窗口会保留输入；采用或明确放弃后才能继续。")
            : !root.canonical.ok ? qsTr("原策略无法进入结构化表单：%1 原文没有改动，可在高级文本中修正或明确新建替换。").arg(root.canonical.error)
            : root.canonical.absent ? qsTr("尚无启动策略。可暂存规划配置，但原受控启动门禁仍拒绝缺策略。")
            : qsTr("当前策略版本 %1；结构与数值范围符合输入格式，不代表安全或允许启动。").arg(root.canonical.policy.schemaVersion)
    }
    QGCButton {
        id: openButton
        Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight
        text: root.pendingInput ? qsTr("继续编辑启动策略…") : qsTr("配置启动阶段策略…")
        enabled: (root.pendingInput || (root.editingEnabled && !root.exhausted)) && !root.emitting
        focusPolicy: Qt.StrongFocus
        onClicked: root.openEditor()
        onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
        background: Rectangle { color: openButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; opacity: openButton.enabled ? 1 : 0.45; border.width: openButton.activeFocus ? 2 : 0; border.color: qgcPal.text }
    }
    QGCLabel { Layout.fillWidth: true; visible: !editorPopup.visible && root.message.length > 0; text: root.message; textFormat: Text.PlainText; wrapMode: Text.WordWrap }

    Controls.Popup {
        id: editorPopup
        parent: Controls.Overlay.overlay
        modal: true; focus: true
        closePolicy: Controls.Popup.CloseOnEscape
        width: parent ? Math.max(0, Math.min(parent.width - 16, ScreenTools.defaultFontPixelWidth * 82)) : 0
        height: parent ? Math.max(0, Math.min(parent.height - 16, ScreenTools.defaultFontPixelHeight * 34)) : 0
        x: parent ? (parent.width - width) / 2 : 0
        y: parent ? (parent.height - height) / 2 : 0
        padding: ScreenTools.defaultFontPixelWidth
        background: Rectangle { color: qgcPal.window; border.color: qgcPal.text; border.width: 1 }
        onOpened: closeEditorButton.forceActiveFocus()
        onClosed: { root.clearConfirmation(); if (root.visible) openButton.forceActiveFocus() }
        contentItem: ColumnLayout {
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { Layout.fillWidth: true; text: qsTr("启动阶段策略 · 仅编辑本地配置文本"); font.bold: true; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
            QGCFlickable {
                id: formScroll
                Layout.fillWidth: true; Layout.fillHeight: true
                clip: true; contentWidth: width; contentHeight: formColumn.implicitHeight
                flickableDirection: Flickable.VerticalFlick
                ColumnLayout {
                    id: formColumn
                    width: formScroll.width
                    spacing: ScreenTools.defaultFontPixelWidth
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("输入范围不是安全推荐值。所有数值由工程师明确给出，不从 Home、遥测或机体型号补值。v2 / v3 / v4 的观察契约不证明共原点、真实北向、未观察运动或净空。") }
                    QGCLabel { Layout.fillWidth: true; visible: root.pendingInput && (!root.sourceCurrent || !root.editingEnabled); text: qsTr("父编辑来源、文本或会话已变化，当前表单不能采用；输入仍保留供查看或明确放弃，请回到父配置重新确认来源。"); color: qgcPal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    QGCLabel { Layout.fillWidth: true; visible: !root.pendingInput && !root.canonical.ok; text: root.canonical.error; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight; visible: !root.pendingInput && !root.canonical.ok; text: qsTr("新建替换表单（尚不改原文）"); enabled: root.editingEnabled && !root.emitting; focusPolicy: Qt.StrongFocus; onClicked: root.startReplacement(); onActiveFocusChanged: if (activeFocus) root.ensureVisible(this) }
                    QGCLabel { Layout.fillWidth: true; visible: root.pendingInput; text: qsTr("策略版本（必须明确选择；切换不填数值，采用时会省略其他版本的包线）"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    QGCComboBox {
                        Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight
                        visible: root.pendingInput; enabled: root.canEdit; focusPolicy: Qt.StrongFocus
                        model: [qsTr("v1 · 保留原航向，HOME 变化停止再审"), qsTr("v2 · 有界运动 / 航向观察，HOME 变化停止再审"), qsTr("v3 · 另加显式 HOME 更新观察包线"), qsTr("v4 · 固定 HOME，另加显式高度关系容差")]
                        currentIndex: root.version - 1
                        onActivated: root.selectVersion(index + 1)
                        onActiveFocusChanged: if (activeFocus) root.ensureVisible(this)
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.pendingInput && root.version > 0
                        textFormat: Text.PlainText; wrapMode: Text.Wrap
                        text: (root.version === 3 ? qsTr("HOME：仅在自身已发起 ARM / MODE 后按显式观察包线检查。") : qsTr("HOME：变化即停止并重新审阅。"))
                            + "\n" + (root.version >= 2 ? qsTr("航向：有界已观察变化；局部参考固定为 observed-native-odometry-ned-not-origin-proof。") : qsTr("航向：严格保留原审阅值，不补航向容差。"))
                            + (root.version === 3 ? "\n" + qsTr("HOME 参考固定为 original-reviewed-home-observation-not-frame-rebase。五项位移上限允许明确为 0，空白仍非法。") : "")
                            + (root.version === 4 ? "\n" + qsTr("高度：需要原始 ALTITUDE、原点与 HOME 报告；六项容差允许明确为 0，不补安全默认。只检查报告关系，非共同原点、内部 valid 或同采样证明。连续越界 / 断档记录从本次实际启动 scope 绑定开始，不证明此前审阅等待期间未越界。") : "")
                    }
                    Repeater {
                        model: root.pendingInput && !root.clearing ? root.shownDefinitions : []
                        ColumnLayout {
                            Layout.fillWidth: true
                            readonly property var definition: modelData
                            QGCLabel { Layout.fillWidth: true; visible: index === 0 || root.shownDefinitions[index - 1].group !== definition.group; text: root.sectionTitle(definition.group); font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                            QGCLabel { Layout.fillWidth: true; text: definition.label + " (" + definition.unit + ")"; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                            Controls.TextField {
                                id: numericInput
                                property bool visited: false
                                Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight
                                enabled: root.canEdit; selectByMouse: true; maximumLength: 64
                                text: typeof root.fields[root.fieldKey(definition)] === "string" ? root.fields[root.fieldKey(definition)] : ""
                                color: qgcPal.text; Accessible.name: definition.label + " (" + definition.unit + ")"
                                onTextEdited: root.editField(definition, text)
                                onActiveFocusChanged: { if (activeFocus) root.ensureVisible(this); else visited = true }
                                background: Rectangle { color: qgcPal.windowShade; opacity: numericInput.enabled ? 1 : 0.45; border.color: numericInput.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: numericInput.activeFocus ? 2 : 1 }
                            }
                            QGCLabel { Layout.fillWidth: true; text: root.rangeText(definition); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                            QGCLabel { Layout.fillWidth: true; visible: numericInput.visited && text.length > 0; text: root.fieldError(definition); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                        }
                    }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight; visible: root.pendingInput; enabled: root.canEdit; text: qsTr("准备移除策略（仍需预览确认）"); focusPolicy: Qt.StrongFocus; onClicked: root.stageClear(); onActiveFocusChanged: if (activeFocus) root.ensureVisible(this) }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight; visible: root.pendingInput; enabled: root.canEdit; text: qsTr("校验并预览完整策略"); focusPolicy: Qt.StrongFocus; onClicked: root.previewPolicy(); onActiveFocusChanged: if (activeFocus) root.ensureVisible(this) }
                    QGCLabel {
                        Layout.fillWidth: true; visible: !!root.preview; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: root.preview && root.preview.clearing ? qsTr("将采用空策略文本：完整配置可按原流程暂存，但缺策略不能进入受控启动。原配置尚未改变。")
                            : root.replacingInvalid ? qsTr("将整体替换无法读取的原策略；原未知字段不会被合并或悄悄保留。下方是待采用的完整对象。")
                            : qsTr("下方是所选版本的完整对象。只有明确采用后才替换父文本；没有隐式版本升级。")
                    }
                    QGCLabel { Layout.fillWidth: true; visible: !!root.preview && !root.preview.clearing; text: root.preview ? root.preview.text : ""; textFormat: Text.PlainText; wrapMode: Text.Wrap }
                    Controls.CheckBox {
                        id: adoptionCheck
                        Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight
                        visible: !!root.preview; enabled: root.previewCurrent(root.preview)
                        checked: root.confirmed; focusPolicy: Qt.StrongFocus
                        text: qsTr("已核对版本、全部值及单位；仅采用到父配置文本，不发送指令")
                        palette.text: qgcPal.text
                        palette.buttonText: qgcPal.buttonText
                        palette.highlight: qgcPal.buttonHighlight
                        contentItem: QGCLabel { text: adoptionCheck.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (adoptionCheck.indicator ? adoptionCheck.indicator.width : 0) + adoptionCheck.spacing; color: qgcPal.text; opacity: adoptionCheck.enabled ? 1 : 0.45 }
                        background: Rectangle { color: "transparent"; border.color: qgcPal.buttonHighlight; border.width: adoptionCheck.activeFocus ? 2 : 0 }
                        onClicked: { root.confirmed = checked; checked = Qt.binding(function() { return root.confirmed }) }
                        onActiveFocusChanged: if (activeFocus) root.ensureVisible(this)
                    }
                    QGCButton {
                        id: adoptButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight
                        visible: !!root.preview; enabled: root.confirmed && root.previewCurrent(root.preview)
                        text: root.preview && root.preview.clearing ? qsTr("确认采用空策略文本") : qsTr("采用本次完整策略")
                        focusPolicy: Qt.StrongFocus
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onPressed: root.pressedIntent = root.confirmed && root.previewCurrent(root.preview) ? root.preview : null
                        onCanceled: root.pressedIntent = null
                        onClicked: { var intent = root.pressedIntent; root.pressedIntent = null; root.adopt(intent) }
                        onActiveFocusChanged: if (activeFocus) root.ensureVisible(this)
                        background: Rectangle { color: adoptButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; opacity: adoptButton.enabled ? 1 : 0.45; border.width: adoptButton.activeFocus ? 2 : 0; border.color: qgcPal.text }
                    }
                    QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight; visible: root.pendingInput; enabled: !root.emitting; text: qsTr("放弃未采用表单（保留父文本）"); focusPolicy: Qt.StrongFocus; onClicked: root.discard(); onActiveFocusChanged: if (activeFocus) root.ensureVisible(this) }
                    QGCLabel { id: status; Layout.fillWidth: true; visible: root.message.length > 0; text: root.message; textFormat: Text.PlainText; wrapMode: Text.WordWrap; activeFocusOnTab: visible; Accessible.name: text; color: qgcPal.warningText; onActiveFocusChanged: if (activeFocus) root.ensureVisible(this) }
                }
            }
            QGCButton {
                id: closeEditorButton
                Layout.fillWidth: true; Layout.minimumHeight: root.controlHeight
                text: qsTr("关闭窗口（保留未采用输入）"); focusPolicy: Qt.StrongFocus
                onClicked: editorPopup.close()
                background: Rectangle { color: closeEditorButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.text; border.width: closeEditorButton.activeFocus ? 2 : 0 }
            }
        }
    }
}

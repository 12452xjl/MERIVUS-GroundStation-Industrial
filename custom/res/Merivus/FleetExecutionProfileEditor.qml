import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Per-member intent, not authority. No defaults for physical tolerances,
// parameter writes, upload, startup command or claimed clearance.
ColumnLayout {
    id: root
    property var draft: null
    property string sourceIdentity: ""
    property var member: null
    signal focusVisibilityRequested(var item)
    readonly property int vehicleId: member ? Number(member.vehicleId) : 0
    property bool editing: false
    property double expectedRevision: -1
    property var fields: ({})
    property string launchType: ""
    property string endType: ""
    property string timingPolicy: ""
    property string startupText: ""
    property bool spatialGuardPresent: false
    property var spatialGuardValue: undefined
    property bool spatialGuardBusy: false
    readonly property bool spatialGuardValid: !spatialGuardPresent
        || (typeof spatialGuardValue === "string" && spatialGuardValue === "integer-e7-amsl-mm-v1")
    readonly property bool spatialGuardSelected: spatialGuardPresent && spatialGuardValid
    readonly property bool spatialGuardPolicyAvailable: policyObjectAvailable(startupText)
    property bool showAdvancedStartupJson: false
    property var policySession: null
    property var editorBinding: null
    property var clearBinding: null
    property var observation: ({})
    readonly property var speedSuggestion: checkedSpeedSuggestion(observation)
    property bool confirmed: false
    property double clearRevision: -1
    property string message: ""
    property var lifetime: ({alive: true, spatialGuardBusy: false, inputGeneration: 0, inputExhausted: false})
    property var editSession: null
    readonly property bool pendingInput: editing || clearRevision >= 0
    readonly property bool sourceCurrent: editing && bindingCurrent(editorBinding)
    readonly property real rowHeight: Math.max(36, ScreenTools.defaultFontPixelHeight * 2.4)
    readonly property var saved: member && member.executionProfile ? member.executionProfile : ({})
    readonly property var definitions: [
        {key: "cruise", label: qsTr("执行巡航速度 (m/s)"), min: 0.1, max: 300},
        {key: "returnSpeed", label: qsTr("执行返航段速度 (m/s)"), min: 0.1, max: 300},
        {key: "climb", label: qsTr("自动爬升速度 · 必须匹配当前参数 (m/s)"), min: 0.1, max: 20},
        {key: "descent", label: qsTr("自动下降速度 · 必须匹配当前参数 (m/s)"), min: 0.1, max: 20},
        {key: "launchLat", label: qsTr("起飞点纬度 (°)"), min: -80, max: 80, group: "launch"},
        {key: "launchLon", label: qsTr("起飞点经度 (°)"), min: -180, max: 180, group: "launch"},
        {key: "launchAlt", label: qsTr("起飞目标 AMSL 海拔 · 非离地高 (m)"), min: -1000, max: 20000, group: "launch"},
        {key: "endLat", label: qsTr("结束点纬度 (°)"), min: -80, max: 80, group: "end"},
        {key: "endLon", label: qsTr("结束点经度 (°)"), min: -180, max: 180, group: "end"},
        {key: "endAlt", label: qsTr("结束点 AMSL 海拔 · 非离地高 (m)"), min: -1000, max: 20000, group: "end"}
    ]
    spacing: ScreenTools.defaultFontPixelWidth
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }
    Component.onDestruction: lifetime.alive = false

    function field(value) { return typeof value === "number" && isFinite(value) ? String(value) : "" }
    function exactMap(value, keys) {
        if (!value || typeof value !== "object" || Array.isArray(value) || Object.keys(value).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, keys[i])) return false
        return true
    }
    function checkedSpeedSuggestion(offer) {
        if (!offer || offer.ok !== true || offer.sourceCurrent !== true || offer.allParametersUsable !== true
                || offer.vehicleId !== vehicleId || typeof offer.parameterProfileHash !== "string"
                || !/^[0-9a-f]{64}$/.test(offer.parameterProfileHash) || typeof offer.observationFingerprint !== "string"
                || !/^[0-9a-f]{64}$/.test(offer.observationFingerprint)) return null
        var value = offer.speedSuggestion
        var keys = ["cruiseSpeedMetersPerSecond", "returnSpeedMetersPerSecond", "climbSpeedMetersPerSecond", "descentSpeedMetersPerSecond"]
        if (!exactMap(value, ["schemaVersion", "type", "usable", "reason", "speeds", "sourceParameters", "returnSpeedPolicy", "configurationOnly", "performanceCertified"])
                || value.schemaVersion !== 1 || value.type !== "observed-parameter-speed-suggestion"
                || typeof value.usable !== "boolean" || typeof value.reason !== "string" || value.reason.length > 512
                || value.reason.indexOf("\u0000") >= 0 || (value.usable ? value.reason.length !== 0 : value.reason.length === 0)
                || value.returnSpeedPolicy !== "inherit-observed-cruise-for-return-nav-not-rtl"
                || value.configurationOnly !== true || value.performanceCertified !== false
                || !exactMap(value.sourceParameters, keys)) return null
        var names = ["MPC_XY_CRUISE", "MPC_XY_CRUISE", "MPC_Z_V_AUTO_UP", "MPC_Z_V_AUTO_DN"]
        for (var i = 0; i < keys.length; ++i) if (value.sourceParameters[keys[i]] !== names[i]) return null
        if (!value.usable) return exactMap(value.speeds, []) ? value : null
        if (!exactMap(value.speeds, keys)) return null
        for (var j = 0; j < keys.length; ++j) {
            var number = value.speeds[keys[j]]
            if (typeof number !== "number" || !isFinite(number) || number < 0.1 || number > (j < 2 ? 300 : 20)) return null
        }
        return value.speeds.returnSpeedMetersPerSecond === value.speeds.cruiseSpeedMetersPerSecond ? value : null
    }
    function advanceInputGeneration() {
        var guard = lifetime
        if (guard.inputGeneration >= 9007199254740991) guard.inputExhausted = true
        else ++guard.inputGeneration
    }
    function inputStamp(candidateFields) {
        try {
            return JSON.stringify({fields: candidateFields === undefined ? fields : candidateFields, launchType: launchType, endType: endType, timingPolicy: timingPolicy,
                startupText: startupText, spatialGuardPresent: spatialGuardPresent, spatialGuardValue: spatialGuardValue})
        } catch (error) { return "" }
    }
    function captureSpeedAdoption() {
        var guard = lifetime, suggestion = speedSuggestion
        if (!guard.alive || guard.spatialGuardBusy || guard.inputExhausted || !visible || !enabled || !editing
                || !bindingCurrent(editorBinding) || startupPolicyEditor.pendingInput || !suggestion || !suggestion.usable) return null
        return {draft: draft, binding: editorBinding, session: editSession, source: sourceIdentity, vehicleId: vehicleId,
            revision: expectedRevision, generation: guard.inputGeneration, input: inputStamp(), offer: observation,
            fingerprint: observation.observationFingerprint, hash: observation.parameterProfileHash,
            suggestion: JSON.stringify(suggestion)}
    }
    function adoptSpeeds(pressed) {
        return exclusiveEdit(function() { return adoptProfileSpeeds(pressed) })
    }
    function adoptProfileSpeeds(pressed) {
        var guard = lifetime, originalFields = fields
        var originalContext = function() {
            return guard.alive && !guard.inputExhausted && pressed && visible && enabled && editing
                && pressed.draft === draft && pressed.binding === editorBinding && bindingCurrent(pressed.binding)
                && pressed.session === editSession && pressed.source === sourceIdentity && pressed.vehicleId === vehicleId
                && pressed.revision === expectedRevision && pressed.revision === draft.revision
                && pressed.generation === guard.inputGeneration && pressed.input.length > 0 && pressed.input === inputStamp()
                && !startupPolicyEditor.pendingInput
        }
        var current = function() {
            return originalContext() && pressed.offer === observation
        }
        var failHere = function(text, predicate) {
            if (!predicate()) return false
            root.message = text
            if (predicate()) status.forceActiveFocus()
            return false
        }
        if (!current()) return false
        var fresh = pressed.draft.observeExecutionProfile(pressed.vehicleId)
        if (!current()) return false
        var suggestion = checkedSpeedSuggestion(fresh)
        if (!suggestion || !suggestion.usable || fresh.observationFingerprint !== pressed.fingerprint
                || fresh.parameterProfileHash !== pressed.hash || JSON.stringify(suggestion) !== pressed.suggestion) {
            observation = ({})
            if (!originalContext()) return false
            confirmed = false
            return failHere(qsTr("参数来源、内容或速度建议已变化；未采用。请重新读取并查看建议后再明确采纳。"), originalContext)
        }
        var next = {}
        for (var key in originalFields) next[key] = originalFields[key]
        next.cruise = field(suggestion.speeds.cruiseSpeedMetersPerSecond)
        next.returnSpeed = field(suggestion.speeds.returnSpeedMetersPerSecond)
        next.climb = field(suggestion.speeds.climbSpeedMetersPerSecond)
        next.descent = field(suggestion.speeds.descentSpeedMetersPerSecond)
        if (!current()) return false
        // Revoke consent before fieldsChanged can call even the underlying
        // saveProfile function directly. Retire the old preview before any
        // replacement; a refusal must leave all four old inputs intact.
        confirmed = false
        if (!current()) return false
        clearRevision = -1
        if (!current()) return false
        var invalidated = pressed.draft.invalidatePreview()
        if (!current()) return false
        if (invalidated !== true) return failHere(qsTr("旧预演退休未确认；四项输入未采用。请核对草稿后重新读取。"), current)
        var installedInput = inputStamp(next)
        fields = next // One whole local replacement: no partially adopted four-speed form.
        if (!guard.alive || pressed.session !== editSession || pressed.binding !== editorBinding || !bindingCurrent(pressed.binding)
                || fields !== next || pressed.offer !== observation || startupPolicyEditor.pendingInput
                || pressed.generation !== guard.inputGeneration || installedInput !== inputStamp()) return false
        advanceInputGeneration()
        var installedGeneration = guard.inputGeneration
        var installedCurrent = function() {
            return guard.alive && !guard.inputExhausted && pressed.session === editSession && pressed.binding === editorBinding
                && bindingCurrent(pressed.binding) && fields === next && pressed.offer === observation
                && guard.inputGeneration === installedGeneration && installedInput === inputStamp()
                && visible && enabled && !startupPolicyEditor.pendingInput
        }
        confirmed = false
        if (!installedCurrent()) return false
        message = qsTr("已明确采用四项配置速度到未保存输入；返程 NAV 沿用本次巡航值，不是 RTL 参数。未改飞控参数、起降点或策略；保存前仍需重新核对确认。")
        return installedCurrent()
    }
    function exclusiveEdit(operation) {
        var guard = lifetime
        if (!guard.alive || guard.spatialGuardBusy) return false
        guard.spatialGuardBusy = true
        spatialGuardBusy = true
        try {
            return guard.alive ? operation() : false
        } finally {
            if (guard.alive) spatialGuardBusy = false
            guard.spatialGuardBusy = false
        }
    }
    function policyObjectAvailable(value) {
        if (typeof value !== "string" || value.length > 16384 || value.trim().length === 0) return false
        try {
            var parsed = JSON.parse(value)
            return parsed && !Array.isArray(parsed) && typeof parsed === "object"
                && typeof parsed.schemaVersion === "number" && [1, 2, 3, 4].indexOf(parsed.schemaVersion) >= 0
        } catch (error) { return false }
    }
    function selectSpatialGuard(selected) {
        var guard = lifetime, session = editSession, binding = editorBinding, originalText = startupText
        if (!guard.alive || guard.spatialGuardBusy || !editing || !sourceCurrent || startupPolicyEditor.pendingInput)
            return false
        if (typeof selected !== "boolean" || !spatialGuardValid || (selected && !spatialGuardPolicyAvailable))
            return fail(qsTr("整数空间域需要有效的原始声明和显式启动策略；请先修正策略，不能自动补默认或删除坏声明。"))
        // Revoke old consent before publishing the new selected value. Keep a
        // plain guard through notifying busy/confirmation writes and release.
        guard.spatialGuardBusy = true
        spatialGuardBusy = true
        try {
            if (!guard.alive || session !== editSession || binding !== editorBinding || !bindingCurrent(binding)) return false
            confirmed = false
            if (!guard.alive || session !== editSession || binding !== editorBinding || !bindingCurrent(binding)
                    || originalText !== startupText) return false
            spatialGuardPresent = selected
            if (!guard.alive || session !== editSession || binding !== editorBinding || !bindingCurrent(binding)) return false
            spatialGuardValue = selected ? "integer-e7-amsl-mm-v1" : undefined
            if (!guard.alive || session !== editSession || binding !== editorBinding || !bindingCurrent(binding)
                    || originalText !== startupText) return false
            changedInput()
            return guard.alive && session === editSession && binding === editorBinding && bindingCurrent(binding)
        } finally {
            if (guard.alive) spatialGuardBusy = false
            guard.spatialGuardBusy = false
        }
    }
    function bindingCurrent(binding) {
        return binding && binding.draft === draft && binding.sourceIdentity === sourceIdentity && !!sourceIdentity
            && binding.vehicleId === vehicleId && draft && binding.revision === draft.revision
    }
    function sourceChanged() {
        advanceInputGeneration()
        clearRevision = -1; clearBinding = null; confirmed = false; observation = ({})
        if (editing) {
            // Retain the old input for inspection/copying. It is never silently
            // loaded into or submitted against the newly selected draft.
            editSession = ({})
            message = qsTr("草稿来源或成员已变化；保留原输入供核对，但不能保存到当前来源。若变化发生在保存期间，请核对原草稿实际版本，不能据此认定已回滚。明确取消后可重新编辑。")
        }
    }
    function changedInput() {
        advanceInputGeneration()
        confirmed = false
        clearRevision = -1
        message = ""
        if (draft) draft.invalidatePreview()
    }
    function editField(key, value) {
        var next = {}
        for (var name in fields) next[name] = fields[name]
        next[key] = value
        fields = next
        changedInput()
    }
    function number(value) {
        var text = String(value === undefined ? "" : value).trim()
        return /^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(text) ? Number(text) : NaN
    }
    function relevant(definition) {
        return !definition.group || (definition.group === "launch" ? launchType === "takeoff" : endType === "hold" || endType === "land")
    }
    function fieldError(definition) {
        var value = number(fields[definition.key])
        return !isFinite(value) || value < definition.min || value > definition.max
            ? qsTr("请输入 %1 至 %2 的有限十进制数字。").arg(definition.min).arg(definition.max) : ""
    }
    function begin() {
        return exclusiveEdit(function() { return beginProfile() })
    }
    function beginProfile() {
        var guard = lifetime, actualDraft = draft, id = vehicleId
        var source = sourceIdentity
        if (!actualDraft || !member || !source) return fail(qsTr("当前草稿来源不可编辑，请重新打开规划页。"))
        var revision = actualDraft.revision
        var accepted = actualDraft.invalidatePreview()
        if (!guard.alive) return false
        if (!accepted || actualDraft !== draft || source !== sourceIdentity || id !== vehicleId || revision !== actualDraft.revision)
            return fail(qsTr("草稿或成员已变化，请重新核对后编辑。"))
        var profile = saved, vertical = profile.verticalPolicy || {}
        var launch = profile.launchAction || {}, end = profile.endAction || {}
        var start = launch.coordinate || {}, finish = end.coordinate || {}
        fields = {cruise: field(profile.cruiseSpeedMetersPerSecond), returnSpeed: field(profile.returnSpeedMetersPerSecond),
            climb: field(vertical.climbSpeedMetersPerSecond), descent: field(vertical.descentSpeedMetersPerSecond),
            launchLat: field(start.latitude), launchLon: field(start.longitude), launchAlt: field(start.altitude),
            endLat: field(finish.latitude), endLon: field(finish.longitude), endAlt: field(finish.altitude)}
        launchType = launch.type || ""
        endType = end.type || ""
        timingPolicy = profile.timingPolicy || ""
        startupText = profile.startupTransitionPolicy ? JSON.stringify(profile.startupTransitionPolicy, null, 2) : ""
        spatialGuardPresent = Object.prototype.hasOwnProperty.call(profile, "startupSpatialGuard")
        spatialGuardValue = spatialGuardPresent ? profile.startupSpatialGuard : undefined
        showAdvancedStartupJson = false
        policySession = ({})
        editorBinding = {draft: actualDraft, sourceIdentity: source, vehicleId: id, revision: revision}
        observation = ({})
        confirmed = false
        clearRevision = -1
        expectedRevision = revision
        editSession = ({})
        editing = true
        message = ""
    }
    function cancel() {
        return exclusiveEdit(function() { return cancelProfile() })
    }
    function cancelProfile() {
        editSession = null; policySession = null; editorBinding = null; clearBinding = null
        editing = false; confirmed = false; clearRevision = -1; observation = ({}); message = ""
        spatialGuardPresent = false; spatialGuardValue = undefined
    }
    function clearSaved() {
        return exclusiveEdit(function() { return clearSavedProfile() })
    }
    function clearSavedProfile() {
        var guard = lifetime, actualDraft = draft, id = vehicleId
        var source = sourceIdentity
        if (!actualDraft || !member || !source || editing) return
        if (clearRevision < 0) {
            var revision = actualDraft.revision
            var invalidated = actualDraft.invalidatePreview()
            if (!guard.alive) return false
            if (!invalidated || actualDraft !== draft || source !== sourceIdentity || id !== vehicleId || revision !== actualDraft.revision)
                return fail(qsTr("草稿已变化，请重新核对。"))
            clearRevision = revision
            clearBinding = {draft: actualDraft, sourceIdentity: source, vehicleId: id, revision: revision}
            message = qsTr("再次点击清除，将移除 ID %1 的本地执行配置；不修改机载参数或任务。").arg(vehicleId)
            return
        }
        var expected = clearRevision
        var binding = clearBinding
        clearRevision = -1; clearBinding = null
        if (!bindingCurrent(binding)) return fail(qsTr("原草稿或成员已变化，本次清除未提交，请重新核对。"))
        var cleared = actualDraft.clearExecutionProfile(id, expected)
        if (!guard.alive || actualDraft !== draft || source !== sourceIdentity || id !== vehicleId) return false
        if (cleared !== true) return fail(qsTr("清除未确认，请重新检查当前草稿；未自动回滚已经发生的本地修改。"))
        message = qsTr("已清除本地执行配置，旧审批已失效。")
    }
    function fail(text) { message = text; status.forceActiveFocus(); return false }
    function observe() {
        return exclusiveEdit(function() { return observeProfile() })
    }
    function observeProfile() {
        var guard = lifetime, session = editSession, actualDraft = draft, id = vehicleId, revision = expectedRevision
        advanceInputGeneration()
        confirmed = false
        observation = ({})
        if (!actualDraft || !editing || !session || !bindingCurrent(editorBinding)) return fail(qsTr("草稿已变化，请取消编辑并重新打开该成员配置。"))
        var result = actualDraft.observeExecutionProfile(id)
        if (!guard.alive || session !== editSession || !bindingCurrent(editorBinding) || actualDraft !== draft || id !== vehicleId) return false
        if (actualDraft.revision !== revision) return fail(qsTr("读取期间草稿已变化，请重新编辑。"))
        observation = result || ({})
        if (observation.ok !== true) return fail(qsTr("参数依据不可绑定：%1。此按钮只读，不会请求或写入参数。").arg(observation.reason || "unknown"))
        message = qsTr("已读取当前 12 项参数，请核对下方值；保存时会再次检查来源及内容。")
    }
    function point(prefix) {
        return {coordinateSystem: "geodetic", altitudeReference: "amsl", latitude: number(fields[prefix + "Lat"]),
            longitude: number(fields[prefix + "Lon"]), altitude: number(fields[prefix + "Alt"])}
    }
    function save() {
        return exclusiveEdit(function() { return saveProfile() })
    }
    function saveProfile() {
        var guard = lifetime, session = editSession, actualDraft = draft, id = vehicleId, revision = expectedRevision
        if (!draft || !editing || !bindingCurrent(editorBinding))
            return fail(qsTr("草稿已变化，请取消编辑并重新打开。未自动覆盖新版本。"))
        if (startupPolicyEditor.pendingInput) return fail(qsTr("启动策略表单尚未采用或明确放弃，请先处理表单，不能保存旧策略。"))
        if (!spatialGuardValid) return fail(qsTr("原整数空间域声明不是支持的精确字符串，已保留原值；本次不能保存或静默删除。请取消并检查来源。"))
        if (!confirmed || observation.ok !== true) return fail(qsTr("先读取有效参数，并确认当前机号、配置和参数依据。"))
        if (["takeoff", "none"].indexOf(launchType) < 0 || ["hold", "land", "rtl"].indexOf(endType) < 0
                || ["relative-dwell", "absolute-required"].indexOf(timingPolicy) < 0)
            return fail(qsTr("请选择起飞、结束动作和时序要求。"))
        for (var i = 0; i < definitions.length; ++i) {
            var definition = definitions[i], value = number(fields[definition.key])
            if (relevant(definition) && (!isFinite(value) || value < definition.min || value > definition.max))
                return fail(qsTr("%1：请输入 %2 至 %3 的有限十进制数字。").arg(definition.label).arg(definition.min).arg(definition.max))
        }
        var policy = null
        if (startupText.length > 16384) return fail(qsTr("启动阶段约束超过 16384 字符上限。"))
        if (startupText.trim().length > 0) {
            try { policy = JSON.parse(startupText) } catch (error) { return fail(qsTr("启动阶段约束不是有效 JSON；请修正后保存。")) }
            if (!policy || Array.isArray(policy) || typeof policy !== "object") return fail(qsTr("启动阶段约束必须是完整对象。"))
        }
        if (spatialGuardPresent && (!policy || !spatialGuardPolicyAvailable))
            return fail(qsTr("已选择整数空间域，必须同时保留完整启动策略；请修正策略或明确取消该选择。"))
        var profile = {schemaVersion: 1, adapter: "px4-multirotor-mission-v1",
            cruiseSpeedMetersPerSecond: number(fields.cruise), returnSpeedMetersPerSecond: number(fields.returnSpeed),
            timingPolicy: timingPolicy, verticalPolicy: {mode: "vehicle-parameters", climbSpeedMetersPerSecond: number(fields.climb),
                descentSpeedMetersPerSecond: number(fields.descent), requiredProfileHash: observation.parameterProfileHash},
            launchAction: launchType === "takeoff" ? {type: launchType, coordinate: point("launch")} : {type: launchType},
            endAction: endType === "rtl" ? {type: endType} : {type: endType, coordinate: point("end")}}
        if (policy) profile.startupTransitionPolicy = policy
        if (spatialGuardPresent) profile.startupSpatialGuard = spatialGuardValue
        var accepted = actualDraft.setExecutionProfile(id, profile, revision, observation.observationFingerprint)
        if (!guard.alive || session !== editSession || actualDraft !== draft || id !== vehicleId) return false
        if (accepted !== true) {
            confirmed = false
            observation = ({})
            return fail(qsTr("保存未确认：%1。请重新读取参数；若草稿版本变化，请取消后重新编辑。").arg(draft.lastError || qsTr("参数绑定或来源不匹配")))
        }
        editing = false
        policySession = null; editorBinding = null
        editSession = null
        confirmed = false
        observation = ({})
        message = qsTr("已保存该成员的本地执行配置；必须重新生成和审批任务，不代表允许起飞。")
    }
    onVehicleIdChanged: sourceChanged()
    onDraftChanged: sourceChanged()
    onSourceIdentityChanged: sourceChanged()
    onVisibleChanged: if (!visible) {
        // Preserve unfinished text, but no in-flight read/save can restore a
        // hidden page's confirmation or parameter offer on its way back.
        advanceInputGeneration()
        editSession = editing ? ({}) : null
        confirmed = false; clearRevision = -1; clearBinding = null; observation = ({})
    }

    QGCLabel { Layout.fillWidth: true; text: qsTr("逐机执行配置 · ID %1").arg(root.vehicleId); font.bold: true; wrapMode: Text.WordWrap }
    QGCLabel {
        Layout.fillWidth: true; visible: root.editing && !root.sourceCurrent
        text: qsTr("保留的是原 ID %1 / 修订 %2 的编辑输入。当前来源已变化，采用表单、读取参数和保存均已禁用；取消编辑后才可开始新来源。")
            .arg(root.editorBinding ? root.editorBinding.vehicleId : "?").arg(root.expectedRevision)
        textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
    }
    QGCLabel {
        Layout.fillWidth: true
        text: root.saved.adapter ? qsTr("已保存：%1 / %2；巡航 %3 m/s。绑定参数仍须重新核实。").arg((root.saved.launchAction || {}).type || "?").arg((root.saved.endAction || {}).type || "?").arg(root.saved.cruiseSpeedMetersPerSecond)
            : qsTr("未配置。地图规划速度不会自动变成机载执行配置；缺失时启动检查拒绝。")
        wrapMode: Text.WordWrap
    }
    QGCButton { text: qsTr("编辑该机完整配置"); visible: !root.editing; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; onClicked: root.begin() }
    RowLayout {
        visible: !root.editing && !!root.saved.adapter
        Layout.fillWidth: true
        QGCButton {
            text: root.clearRevision < 0 ? qsTr("清除该机本地配置…") : qsTr("确认清除本地配置")
            Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            Keys.onReturnPressed: event.accepted = true
            Keys.onEnterPressed: event.accepted = true
            onClicked: root.clearSaved()
        }
        QGCButton {
            visible: root.clearRevision >= 0; text: qsTr("取消清除")
            Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            onClicked: { root.clearRevision = -1; root.clearBinding = null; root.message = "" }
        }
    }
    ColumnLayout {
        visible: root.editing
        enabled: !root.spatialGuardBusy
        Layout.fillWidth: true
        QGCLabel { Layout.fillWidth: true; text: qsTr("固定适配器：PX4 多旋翼。数值范围只是输入边界，不是安全飞行推荐值。不会自动取 Home 或规划点作为起降点。"); wrapMode: Text.WordWrap }
        QGCLabel { text: qsTr("起飞动作（必选）") }
        QGCComboBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            model: [qsTr("显式起飞 Takeoff"), qsTr("无起飞项 None（仍受启动条件限制）")]
            focusPolicy: Qt.StrongFocus
            currentIndex: ["takeoff", "none"].indexOf(root.launchType)
            onActivated: { root.launchType = ["takeoff", "none"][index]; root.changedInput() }
        }
        QGCLabel { text: qsTr("结束动作（必选）") }
        QGCComboBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            model: [qsTr("定点保持 Hold"), qsTr("降落 Land"), qsTr("返航 RTL（未支持的编码仍会阻断）")]
            focusPolicy: Qt.StrongFocus
            currentIndex: ["hold", "land", "rtl"].indexOf(root.endType)
            onActivated: { root.endType = ["hold", "land", "rtl"][index]; root.changedInput() }
        }
        QGCLabel { text: qsTr("时序要求（必选）") }
        QGCComboBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            model: [qsTr("相对驻留 Relative dwell"), qsTr("要求绝对时刻（未支持时明确阻断）")]
            focusPolicy: Qt.StrongFocus
            currentIndex: ["relative-dwell", "absolute-required"].indexOf(root.timingPolicy)
            onActivated: { root.timingPolicy = ["relative-dwell", "absolute-required"][index]; root.changedInput() }
        }
        Repeater {
            model: root.definitions
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.relevant(modelData)
                readonly property var definition: modelData
                QGCLabel { Layout.fillWidth: true; text: definition.label; wrapMode: Text.WordWrap }
                Controls.TextField {
                    id: numericInput
                    property bool visited: false
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                    text: root.fields[definition.key] || ""
                    maximumLength: 32; selectByMouse: true; color: qgcPal.text
                    Accessible.name: definition.label
                    onTextEdited: root.editField(definition.key, text)
                    onActiveFocusChanged: if (!activeFocus) visited = true
                    background: Rectangle { color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: parent.activeFocus ? 2 : 1 }
                }
                QGCLabel {
                    Layout.fillWidth: true
                    visible: numericInput.visited && text.length > 0
                    text: root.fieldError(definition)
                    color: qgcPal.colorOrange; wrapMode: Text.WordWrap
                }
            }
        }
        FleetStartupPolicyEditor {
            id: startupPolicyEditor
            Layout.fillWidth: true
            policyText: root.startupText
            sessionToken: root.policySession
            editingEnabled: root.editing && root.sourceCurrent
            onFocusVisibilityRequested: root.focusVisibilityRequested(item)
            onPendingInputChanged: if (pendingInput && root.editing) root.changedInput()
            onPolicyEdited: {
                if (!root.lifetime.alive || !root.editing || !root.sourceCurrent || expectedSession !== root.policySession
                        || expectedOriginalText !== root.startupText) {
                    root.fail(qsTr("表单来源已变化，未采用到当前配置；请核对保留输入后取消并重新编辑。"))
                    return
                }
                root.startupText = nextText
                root.changedInput()
            }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: Math.max(44, root.rowHeight); focusPolicy: Qt.StrongFocus
            text: root.showAdvancedStartupJson ? qsTr("收起高级策略 JSON") : qsTr("展开高级策略 JSON（可保留或修复原文本）")
            onClicked: root.showAdvancedStartupJson = !root.showAdvancedStartupJson
        }
        QGCLabel {
            Layout.fillWidth: true
            visible: root.showAdvancedStartupJson
            text: startupPolicyEditor.pendingInput ? qsTr("表单存在尚未采用的输入；先采用或明确放弃后才能修改 JSON 或保存配置。")
                : qsTr("JSON 是本地策略的同一份文本；表单采用后会更新这里。留空只保存规划配置，不能完成受控启动。不自动补充物理容差。")
            wrapMode: Text.WordWrap
        }
        Controls.TextArea {
            visible: root.showAdvancedStartupJson
            enabled: root.sourceCurrent && !startupPolicyEditor.pendingInput
            Layout.fillWidth: true; Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 8
            text: root.startupText; selectByMouse: true; wrapMode: TextEdit.Wrap; color: qgcPal.text
            Accessible.name: qsTr("启动阶段约束 JSON")
            onTextChanged: if (activeFocus && text !== root.startupText) { root.startupText = text; root.changedInput() }
            background: Rectangle { color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: parent.activeFocus ? 2 : 1 }
        }
        Controls.CheckBox {
            id: spatialGuardChoice
            Layout.fillWidth: true
            Layout.minimumHeight: Math.max(44, root.rowHeight)
            focusPolicy: Qt.StrongFocus
            text: qsTr("显式启用整数 E7 / AMSL 毫米空间域")
            Accessible.name: text
            checked: root.spatialGuardSelected
            enabled: root.sourceCurrent && !root.spatialGuardBusy && !startupPolicyEditor.pendingInput
                && root.spatialGuardValid && (root.spatialGuardSelected || root.spatialGuardPolicyAvailable)
            contentItem: QGCLabel {
                text: spatialGuardChoice.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                leftPadding: (spatialGuardChoice.indicator ? spatialGuardChoice.indicator.width : 0) + ScreenTools.defaultFontPixelWidth
                color: qgcPal.text
            }
            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: spatialGuardChoice.activeFocus }
            onClicked: {
                var guard = root.lifetime
                root.selectSpatialGuard(checked)
                if (guard.alive) checked = Qt.binding(function() { return root.spatialGuardSelected })
            }
        }
        QGCLabel {
            Layout.fillWidth: true
            text: qsTr("默认不选，不升级旧策略。选择后运行检查与审阅共享完整整数 E7 / AMSL 毫米域，并额外限制原 Qt 异常域；需要重新生成、审阅和批准，旧审批不能沿用。这不认证动力学、实际路径、净空或 StartPermit。必须同时保留显式启动策略，取消本选择才可移除该声明。")
            textFormat: Text.PlainText; wrapMode: Text.WordWrap
        }
        QGCLabel {
            Layout.fillWidth: true
            visible: !root.spatialGuardValid || (root.spatialGuardSelected && !root.spatialGuardPolicyAvailable)
            text: !root.spatialGuardValid
                ? qsTr("原空间域声明无效，原值已保留，不能保存或自动转成未选；请取消并检查来源。")
                : qsTr("整数空间域仍保持已选。启动策略已缺失或无可识别版本；请修正策略或明确取消该选择，不能直接保存。")
            textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
        }
        QGCButton { text: qsTr("读取该机当前参数依据（只读）"); enabled: root.sourceCurrent && !startupPolicyEditor.pendingInput; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; onClicked: root.observe() }
        QGCLabel {
            Layout.fillWidth: true; wrapMode: Text.WrapAnywhere
            text: root.observation.ok === true ? qsTr("参数内容 SHA-256：%1").arg(root.observation.parameterProfileHash) : qsTr("尚无本次有效参数依据。旧配置里的哈希不会自动复用。")
        }
        Repeater {
            model: root.observation.parameters || []
            QGCLabel {
                Layout.fillWidth: true; wrapMode: Text.WrapAnywhere
                text: String(modelData.name) + ": " + (modelData.value === null || modelData.value === undefined ? qsTr("未知") : String(modelData.value))
                    + qsTr(" · age %1 ms · %2").arg(modelData.ageMs).arg(modelData.usable === true ? qsTr("读取时可用") : qsTr("不可绑定"))
            }
        }
        QGCLabel {
            Layout.fillWidth: true
            text: root.speedSuggestion && root.speedSuggestion.usable
                ? qsTr("本次配置建议（m/s）：巡航 %1，返程 NAV %2，自动爬升 %3，自动下降 %4。\n巡航/返程取 MPC_XY_CRUISE；爬升/下降取 MPC_Z_V_AUTO_UP/DN。返程值是显式沿用巡航的本地规划选择，不是读取了独立 RTL 参数。")
                    .arg(root.speedSuggestion.speeds.cruiseSpeedMetersPerSecond).arg(root.speedSuggestion.speeds.returnSpeedMetersPerSecond)
                    .arg(root.speedSuggestion.speeds.climbSpeedMetersPerSecond).arg(root.speedSuggestion.speeds.descentSpeedMetersPerSecond)
                : root.speedSuggestion ? qsTr("本次参数仍可查看，但没有可采纳的配置速度建议：%1").arg(root.speedSuggestion.reason)
                    : qsTr("先读取并查看本次原生参数与速度建议；缺失或未知不自动补值。")
            textFormat: Text.PlainText; wrapMode: Text.WordWrap
        }
        QGCLabel {
            Layout.fillWidth: true
            text: qsTr("配置不等于实测性能、最大可达速度或续航认证。采用只替换本地四项未保存输入，不自动保存、不写飞控参数，不产生审批或起飞许可；其它手工输入保持。")
            textFormat: Text.PlainText; wrapMode: Text.WordWrap
        }
        QGCButton {
            id: adoptSpeedsButton
            Layout.fillWidth: true; Layout.minimumHeight: Math.max(44, root.rowHeight)
            focusPolicy: Qt.StrongFocus
            text: qsTr("采用当前飞控配置速度")
            Accessible.name: text
            enabled: root.sourceCurrent && !root.spatialGuardBusy && !startupPolicyEditor.pendingInput
                && root.speedSuggestion !== null && root.speedSuggestion.usable
            property var pressedSource: null
            onPressed: pressedSource = root.captureSpeedAdoption()
            onCanceled: pressedSource = null
            onClicked: { var guard = root.lifetime, pressed = pressedSource; pressedSource = null; if (guard.alive) root.adoptSpeeds(pressed) }
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(adoptSpeedsButton)
            contentItem: QGCLabel { text: adoptSpeedsButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; font.underline: adoptSpeedsButton.activeFocus; color: adoptSpeedsButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        Controls.CheckBox {
            id: profileConfirmation
            Layout.fillWidth: true
            Layout.minimumHeight: Math.max(44, root.rowHeight)
            focusPolicy: Qt.StrongFocus
            text: qsTr("已核对 ID %1、起降坐标/动作、速度、阶段约束、整数域选择和本次参数；仅保存本地配置").arg(root.vehicleId)
            Accessible.name: text
            checked: root.confirmed; enabled: root.sourceCurrent && !root.spatialGuardBusy && !startupPolicyEditor.pendingInput
                && root.spatialGuardValid && (!root.spatialGuardPresent || root.spatialGuardPolicyAvailable) && root.observation.ok === true
            contentItem: QGCLabel {
                text: profileConfirmation.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                leftPadding: (profileConfirmation.indicator ? profileConfirmation.indicator.width : 0) + ScreenTools.defaultFontPixelWidth
                color: qgcPal.text
            }
            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: profileConfirmation.activeFocus }
            onClicked: {
                root.confirmed = checked
                checked = Qt.binding(function() { return root.confirmed })
            }
        }
        RowLayout {
            Layout.fillWidth: true
            QGCButton { text: qsTr("取消编辑"); Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; onClicked: root.cancel() }
            QGCButton {
                text: qsTr("保存完整配置"); Layout.minimumHeight: root.rowHeight
                focusPolicy: Qt.StrongFocus
                enabled: root.sourceCurrent && !root.spatialGuardBusy && !startupPolicyEditor.pendingInput
                    && root.spatialGuardValid && (!root.spatialGuardPresent || root.spatialGuardPolicyAvailable)
                    && root.confirmed && root.observation.ok === true
                Keys.onReturnPressed: event.accepted = true
                Keys.onEnterPressed: event.accepted = true
                onClicked: root.save()
            }
        }
    }
    QGCLabel {
        id: status
        Layout.fillWidth: true; visible: text.length > 0; text: root.message
        wrapMode: Text.WordWrap; color: qgcPal.colorOrange; activeFocusOnTab: visible
        Accessible.name: text
    }
}

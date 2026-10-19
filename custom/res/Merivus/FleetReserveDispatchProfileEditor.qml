import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Pure local values. No Draft, controller, native read or permission interface.
ColumnLayout {
    id: root
    property var offer: ({})
    property var sessionToken: null
    property string sourceIdentity: ""
    property string profileText: ""
    property bool editingEnabled: false
    property var life: ({alive: true, ticket: null, exhausted: false, sourceKey: ""})
    property var form: emptyForm()
    property string message: ""
    signal inputEdited(var expectedSession)
    signal profileEdited(string nextText, var expectedSession, string expectedOriginalText)
    signal focusVisibilityRequested(var item)
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property bool offerValid: offerKnown(offer)
    readonly property bool ready: life.alive && !life.exhausted && editingEnabled && visible && enabled && offerValid && sessionToken !== null && sourceIdentity.length > 0
    readonly property bool pendingInput: startup.pendingInput || (form.dirty && !form.adopted)
    readonly property bool startupPending: startup.pendingInput
    readonly property var definitions: [
        {key:"launchLat", label:qsTr("新起飞纬度 (°)"), min:-80, max:80, group:"launch"},
        {key:"launchLon", label:qsTr("新起飞经度 (°)"), min:-180, max:180, group:"launch"},
        {key:"launchAlt", label:qsTr("新起飞目标 AMSL 海拔 (m，非离地高)"), min:-1000, max:20000, group:"launch"},
        {key:"endLat", label:qsTr("新结束点纬度 (°)"), min:-80, max:80, group:"end"},
        {key:"endLon", label:qsTr("新结束点经度 (°)"), min:-180, max:180, group:"end"},
        {key:"endAlt", label:qsTr("新结束点 AMSL 海拔 (m，非离地高)"), min:-1000, max:20000, group:"end"}
    ]
    spacing: pad
    QGCPalette { id: pal; colorGroupEnabled: root.enabled }

    function map(value) { return value !== null && typeof value === "object" && !Array.isArray(value) }
    function exact(value, names) {
        if (!map(value) || Object.keys(value).length !== names.length) return false
        for (var i = 0; i < names.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, names[i])) return false
        return true
    }
    function number(value, low, high) { return typeof value === "number" && isFinite(value) && value >= low && value <= high }
    function integer(value, low, high) { return number(value, low, high) && Math.floor(value) === value }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && value !== "00000000-0000-0000-0000-000000000000" }
    function counter(value) { return typeof value === "string" && /^[1-9][0-9]{0,19}$/.test(value) && (value.length < 20 || value <= "18446744073709551615") }
    function token(value) { return typeof value === "string" && value.length > 0 && value.length <= 256 && !/[\u0000-\u001f\u007f]/.test(value) }
    function speedSuggestionKnown(value) {
        var keys = ["cruiseSpeedMetersPerSecond", "returnSpeedMetersPerSecond", "climbSpeedMetersPerSecond", "descentSpeedMetersPerSecond"]
        if (!exact(value, ["schemaVersion", "type", "usable", "reason", "speeds", "sourceParameters", "returnSpeedPolicy", "configurationOnly", "performanceCertified"])
                || value.schemaVersion !== 1 || value.type !== "observed-parameter-speed-suggestion"
                || typeof value.usable !== "boolean" || typeof value.reason !== "string" || value.reason.length > 512
                || value.reason.indexOf("\u0000") >= 0 || (value.usable ? value.reason.length !== 0 : value.reason.length === 0)
                || value.returnSpeedPolicy !== "inherit-observed-cruise-for-return-nav-not-rtl"
                || value.configurationOnly !== true || value.performanceCertified !== false
                || !exact(value.sourceParameters, keys)) return false
        var names = ["MPC_XY_CRUISE", "MPC_XY_CRUISE", "MPC_Z_V_AUTO_UP", "MPC_Z_V_AUTO_DN"]
        for (var i = 0; i < keys.length; ++i) if (value.sourceParameters[keys[i]] !== names[i]) return false
        if (!value.usable) return exact(value.speeds, [])
        if (!exact(value.speeds, keys)) return false
        for (var j = 0; j < keys.length; ++j) if (!number(value.speeds[keys[j]], 0.1, j < 2 ? 300 : 20)) return false
        return value.speeds.returnSpeedMetersPerSecond === value.speeds.cruiseSpeedMetersPerSecond
    }
    function offerKnown(value) {
        if (!exact(value, ["ok", "error", "captureToken", "reserveVehicleId", "offerToken", "policy", "observation", "historicalOnly", "executionPermit", "flightCommandReleased"])
                || value.ok !== true || value.error !== "" || !token(value.captureToken) || !integer(value.reserveVehicleId, 1, 255)
                || !token(value.offerToken) || value.historicalOnly !== true || value.executionPermit !== false || value.flightCommandReleased !== false) return false
        var p = value.policy, o = value.observation
        if (!exact(p, ["schemaVersion", "replacementVehicleId", "standbyMode", "standbyDurationSeconds", "standbyConsumptionFactor", "cruiseSpeedMetersPerSecond", "climbSpeedMetersPerSecond", "descentSpeedMetersPerSecond"])
                || p.schemaVersion !== 1 || !integer(p.replacementVehicleId, 1, 255) || ["ground-disarmed", "airborne-hold"].indexOf(p.standbyMode) < 0
                || !number(p.standbyDurationSeconds, 0, 86400) || !number(p.standbyConsumptionFactor, 0, 10) || p.standbyConsumptionFactor <= 0
                || !number(p.cruiseSpeedMetersPerSecond, 0, 1000) || p.cruiseSpeedMetersPerSecond <= 0
                || !number(p.climbSpeedMetersPerSecond, 0, 1000) || p.climbSpeedMetersPerSecond <= 0
                || !number(p.descentSpeedMetersPerSecond, 0, 1000) || p.descentSpeedMetersPerSecond <= 0) return false
        var observationFields = ["schemaVersion", "ok", "vehicleId", "componentId", "parameterProfileHash", "observationFingerprint", "managerEpoch", "parameters", "parameterCount", "maximumParameterAgeMs", "source", "sourceCurrent", "allParametersUsable", "reason", "reasons", "hashPurpose", "hardwareIdentityAuthenticated", "flightCommandReleased", "executionPermitted"]
        if (map(o) && Object.prototype.hasOwnProperty.call(o, "speedSuggestion")) {
            if (!speedSuggestionKnown(o.speedSuggestion)) return false
            observationFields.push("speedSuggestion") // Read compatibility, not a second speed source.
        }
        if (!exact(o, observationFields)
                || o.schemaVersion !== 1 || o.ok !== true || o.vehicleId !== value.reserveVehicleId || !integer(o.componentId, 1, 255)
                || !digest(o.parameterProfileHash) || !digest(o.observationFingerprint) || !uuid(o.managerEpoch)
                || o.parameterCount !== 12 || o.maximumParameterAgeMs !== 30000 || o.source !== "live-param-value-receipts-only"
                || o.sourceCurrent !== true || o.allParametersUsable !== true || o.reason !== "" || !Array.isArray(o.reasons) || o.reasons.length
                || o.hashPurpose !== "operator-profile-binding-not-authorization" || o.hardwareIdentityAuthenticated !== false
                || o.flightCommandReleased !== false || o.executionPermitted !== false || !Array.isArray(o.parameters) || o.parameters.length !== 12) return false
        var names = ["MPC_XY_CRUISE", "MPC_XY_VEL_MAX", "MPC_Z_V_AUTO_UP", "MPC_Z_V_AUTO_DN", "MPC_Z_VEL_MAX_UP", "MPC_Z_VEL_MAX_DN", "MPC_TKO_SPEED", "MPC_LAND_SPEED", "MIS_TAKEOFF_ALT", "NAV_ACC_RAD", "MPC_YAW_MODE", "MPC_YAWRAUTO_MAX"].sort()
        for (var i = 0; i < names.length; ++i) {
            var r = o.parameters[i]
            if (!exact(r, ["name", "componentId", "ageMs", "source", "mavParamType", "contentRevision", "managerEpoch", "observed", "available", "fresh", "cachedValueMatches", "pendingWrite", "rangeValid", "usable", "value"])
                    || r.name !== names[i] || r.componentId !== o.componentId || !integer(r.ageMs, 0, 30000) || r.source !== "live-param-value"
                    || !integer(r.mavParamType, 1, 10) || !counter(r.contentRevision) || r.managerEpoch !== o.managerEpoch
                    || r.observed !== true || r.available !== true || r.fresh !== true || r.cachedValueMatches !== true || r.pendingWrite !== false
                    || r.rangeValid !== true || r.usable !== true || !number(r.value, -9007199254740991, 9007199254740991)) return false
        }
        return true
    }
    function emptyForm() { return {launch:"", end:"", timing:"", fields:{}, speeds:false, startup:"", spatial:false, confirmed:false, dirty:false, adopted:false, serial:0} }
    function change(expected, updates) {
        var g = life, original = form, session = sessionToken, ticket = {}
        if (!ready || g.ticket || expected !== original || original.serial >= 9007199254740991) { if (original.serial >= 9007199254740991) g.exhausted = true; return }
        var next = {launch:original.launch, end:original.end, timing:original.timing, fields:original.fields, speeds:original.speeds,
            startup:original.startup, spatial:original.spatial, confirmed:false, dirty:true, adopted:false, serial:original.serial + 1}
        for (var key in updates) if (Object.prototype.hasOwnProperty.call(updates, key)) next[key] = updates[key]
        g.ticket = ticket
        try {
            form = next
            if (!g.alive || session !== sessionToken || form !== next || !ready) return
            inputEdited(session)
        } finally { if (g.ticket === ticket) g.ticket = null }
    }
    function editField(key, value) {
        var next = {}, original = form
        for (var name in original.fields) if (Object.prototype.hasOwnProperty.call(original.fields, name)) next[name] = original.fields[name]
        next[key] = value; change(original, {fields:next})
    }
    function numeric(value) {
        return typeof value === "string" && /^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(value.trim()) ? Number(value.trim()) : NaN
    }
    function relevant(d) { return d.group === "launch" ? form.launch === "takeoff" : form.end === "hold" || form.end === "land" }
    function fieldError(d) { return number(numeric(form.fields[d.key]), d.min, d.max) ? "" : qsTr("请填 %1 至 %2 的有限十进制数；空白不是0。").arg(d.min).arg(d.max) }
    function coordinate(prefix) { return {coordinateSystem:"geodetic", altitudeReference:"amsl", latitude:numeric(form.fields[prefix + "Lat"]), longitude:numeric(form.fields[prefix + "Lon"]), altitude:numeric(form.fields[prefix + "Alt"])} }
    function build() {
        if (!ready || !form.speeds) return {ok:false, error:qsTr("先明确采用本次备用预算的四项执行速度；不会静默继承旧主机。")}
        var p = offer.policy, o = offer.observation
        if (!number(p.cruiseSpeedMetersPerSecond, 0.1, 300) || !number(p.climbSpeedMetersPerSecond, 0.1, 20) || !number(p.descentSpeedMetersPerSecond, 0.1, 20))
            return {ok:false, error:qsTr("原预算速度超出执行profile范围；请返回原方案重做备用预算，不私改速度或参数。")}
        if (["none", "takeoff"].indexOf(form.launch) < 0 || ["rtl", "hold", "land"].indexOf(form.end) < 0)
            return {ok:false, error:qsTr("请明确选择新起飞与结束动作；不会默认None、RTL或借旧坐标。")}
        if (form.timing !== "relative-dwell") return {ok:false, error:qsTr("本独立出勤仅支持明确relative-dwell；绝对时序保持阻断，不自动转换。")}
        for (var i = 0; i < definitions.length; ++i) if (relevant(definitions[i]) && fieldError(definitions[i])) return {ok:false, error:definitions[i].label + "：" + fieldError(definitions[i])}
        var checked = startup.inspect(form.startup)
        if (!checked.ok || startup.pendingInput) return {ok:false, error:checked.error || qsTr("启动策略还有未采用表单；请明确采用或放弃后再生成完整配置。")}
        if (form.spatial && checked.absent) return {ok:false, error:qsTr("整数空间保护需要完整显式启动策略，不能补默认容差。")}
        var profile = {schemaVersion:1, adapter:"px4-multirotor-mission-v1", cruiseSpeedMetersPerSecond:p.cruiseSpeedMetersPerSecond,
            returnSpeedMetersPerSecond:p.cruiseSpeedMetersPerSecond, timingPolicy:form.timing,
            verticalPolicy:{mode:"vehicle-parameters", climbSpeedMetersPerSecond:p.climbSpeedMetersPerSecond, descentSpeedMetersPerSecond:p.descentSpeedMetersPerSecond, requiredProfileHash:o.parameterProfileHash},
            launchAction:{type:form.launch}, endAction:{type:form.end}}
        if (form.launch === "takeoff") profile.launchAction.coordinate = coordinate("launch")
        if (form.end === "hold" || form.end === "land") profile.endAction.coordinate = coordinate("end")
        if (!checked.absent) profile.startupTransitionPolicy = checked.policy
        if (form.spatial) profile.startupSpatialGuard = "integer-e7-amsl-mm-v1"
        var text = JSON.stringify(profile, null, 2)
        return text.length <= 16384 ? {ok:true, error:"", text:text} : {ok:false, error:qsTr("完整配置超过16384字符，未采用前缀。")}
    }
    function stamp() { return ready && !life.ticket ? {session:sessionToken, form:form, original:profileText} : null }
    function adopt(value) {
        var g = life, ticket = {}
        if (!value || !ready || g.ticket || value.session !== sessionToken || value.form !== form || value.original !== profileText || !form.confirmed) return
        var built = build()
        if (!built.ok) {
            message = built.error
            if (g.alive && value.session === sessionToken && value.form === form && ready) errorLabel.forceActiveFocus()
            if (g.alive && value.session === sessionToken && value.form === form && ready) focusVisibilityRequested(errorLabel)
            return
        }
        g.ticket = ticket
        try {
            profileEdited(built.text, value.session, value.original)
            if (!g.alive || sessionToken !== value.session || form !== value.form) return
            if (profileText !== built.text) { message = qsTr("父配置未确认采用，未自动重试；请核对原来源。"); return }
            var next = {}
            for (var key in form) if (Object.prototype.hasOwnProperty.call(form, key)) next[key] = form[key]
            next.adopted = true; next.confirmed = false
            form = next
            if (g.alive && sessionToken === value.session && form === next) message = qsTr("仅已采用到本页新配置。预览前Root仍重新核原参数依据；之后还须当前审阅、采用与批准。")
        } finally { if (g.ticket === ticket) g.ticket = null }
    }
    function revoke() {
        var g = life, original = form, next = {}
        for (var key in original) if (Object.prototype.hasOwnProperty.call(original, key)) next[key] = original[key]
        next.confirmed = false
        form = next
    }
    function sessionChanged() {
        var g = life
        var original = form, next = emptyForm()
        // Retain only this page's operator-authored draft for the same frozen
        // capture/ID. No source proof, hash, speed adoption or consent survives.
        if (sourceIdentity.length > 0 && sourceIdentity === g.sourceKey) {
            next.launch = original.launch; next.end = original.end; next.timing = original.timing
            next.fields = original.fields; next.startup = original.startup; next.spatial = original.spatial
            next.dirty = original.dirty
        }
        form = next
    }
    onSessionTokenChanged: sessionChanged()
    onSourceIdentityChanged: { life.sourceKey = sourceIdentity; form = emptyForm() }
    onEditingEnabledChanged: if (!editingEnabled) revoke()
    onVisibleChanged: if (!visible) revoke()
    onEnabledChanged: if (!enabled) revoke()
    Component.onDestruction: life.alive = false

    QGCLabel { Layout.fillWidth:true; text:qsTr("新执行配置向导 · 仅本页纯值"); font.bold:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap }
    QGCLabel { Layout.fillWidth:true; text:qsTr("固定PX4多旋翼适配器。新起降点、动作、时序和容差都由操作员明确输入；不会读Home、旧主机profile或自动请求／写参数。同capture与同ID重读只保本页拟值供核对，速度采用与确认必须重做。数值范围不是安全推荐。"); textFormat:Text.PlainText; wrapMode:Text.WordWrap }
    QGCLabel { Layout.fillWidth:true; visible:root.offerValid; text:root.offerValid ? qsTr("原备用预算：替代目标ID %1；待命 %2 / %3 s / 系数%4（本次不重演待命）。\n巡航 = 返航 %5 m/s；爬升 %6；下降 %7。这里只采用原规划意图，不证明真实性能。") .arg(root.offer.policy.replacementVehicleId).arg(root.offer.policy.standbyMode).arg(root.offer.policy.standbyDurationSeconds).arg(root.offer.policy.standbyConsumptionFactor).arg(root.offer.policy.cruiseSpeedMetersPerSecond).arg(root.offer.policy.climbSpeedMetersPerSecond).arg(root.offer.policy.descentSpeedMetersPerSecond) : ""; textFormat:Text.PlainText; wrapMode:Text.WordWrap }
    QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; enabled:root.ready; text:qsTr("明确采用本次备用预算速度"); Accessible.name:text; property var pressedIntent:null; onPressed:pressedIntent=root.stamp(); onCanceled:pressedIntent=null; onClicked:{ var g=root.life,v=pressedIntent; pressedIntent=null; if(g.alive&&v&&v.session===root.sessionToken&&v.form===root.form)root.change(v.form,{speeds:true}) } contentItem:QGCLabel{text:parent.text;font.underline:parent.activeFocus;color:parent._showHighlight?pal.buttonHighlightText:pal.buttonText;textFormat:Text.PlainText;wrapMode:Text.WordWrap;horizontalAlignment:Text.AlignHCenter} }
    QGCLabel { Layout.fillWidth:true; text:root.form.speeds ? qsTr("已明确采用四项速度；若实际垂直参数不匹配，须回原方案重做预算，不在此暗改。") : qsTr("尚未采用速度；不自动给数值。") ; textFormat:Text.PlainText;wrapMode:Text.WordWrap }
    Repeater {
        model:[{key:"launch",label:qsTr("新起飞动作（必选）"),values:["takeoff","none"],labels:[qsTr("显式起飞 Takeoff"),qsTr("无起飞项 None")]},
            {key:"end",label:qsTr("新结束动作（必选）"),values:["hold","land","rtl"],labels:[qsTr("定点保持 Hold"),qsTr("降落 Land"),qsTr("返航 RTL，仍须核编码")]},
            {key:"timing",label:qsTr("新时序要求（必选）"),values:["relative-dwell","absolute-required"],labels:[qsTr("相对驻留 Relative dwell"),qsTr("绝对时刻，本出勤不支持")]}]
        ColumnLayout { Layout.fillWidth:true; property var definition:modelData
            QGCLabel {Layout.fillWidth:true;text:parent.definition.label;textFormat:Text.PlainText;wrapMode:Text.WordWrap}
            Controls.ComboBox {Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;enabled:root.ready;model:parent.definition.labels;currentIndex:parent.definition.values.indexOf(root.form[parent.definition.key]);displayText:currentIndex<0?qsTr("请选择，不默认动作"):currentText;Accessible.name:parent.definition.label;onActivated:{var updates={};updates[parent.definition.key]=parent.definition.values[index];root.change(root.form,updates)} onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)}
        }
    }
    Repeater {
        model:root.definitions
        ColumnLayout { Layout.fillWidth:true;visible:root.relevant(modelData);property var definition:modelData
            QGCLabel {Layout.fillWidth:true;text:parent.definition.label;textFormat:Text.PlainText;wrapMode:Text.WordWrap}
            QGCTextField {Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;enabled:root.ready;text:root.form.fields[parent.definition.key]||"";Accessible.name:parent.definition.label;onTextChanged:if(text!==(root.form.fields[parent.definition.key]||""))root.editField(parent.definition.key,text);onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)}
            QGCLabel {Layout.fillWidth:true;visible:root.fieldError(parent.definition).length>0;text:root.fieldError(parent.definition);color:pal.warningText;textFormat:Text.PlainText;wrapMode:Text.WordWrap}
        }
    }
    FleetStartupPolicyEditor {
        id:startup
        Layout.fillWidth:true
        policyText:root.form.startup;sessionToken:root.sessionToken;editingEnabled:root.ready
        onPolicyEdited:{if(expectedSession===root.sessionToken&&expectedOriginalText===root.form.startup)root.change(root.form,{startup:nextText})}
        onFocusVisibilityRequested:root.focusVisibilityRequested(item)
    }
    Controls.CheckBox {Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;enabled:root.ready&&!startup.pendingInput;checked:root.form.spatial;text:qsTr("明确要求整数空间保护");Accessible.name:text;onToggled:if(checked!==root.form.spatial)root.change(root.form,{spatial:checked})}
    QGCLabel {Layout.fillWidth:true;text:qsTr("空间保护可选，但选中必须有完整启动策略。参数hash仅取本次actual offer，不是认证／性能／许可；下方12项年龄固定在读取时，不实时续鲜。");textFormat:Text.PlainText;wrapMode:Text.WordWrap}
    Repeater { model:root.offerValid?root.offer.observation.parameters:[];QGCLabel {Layout.fillWidth:true;text:qsTr("%1 = %2；类型%3；读取年龄%4ms；内容版本%5").arg(modelData.name).arg(modelData.value).arg(modelData.mavParamType).arg(modelData.ageMs).arg(modelData.contentRevision);textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere} }
    QGCLabel {Layout.fillWidth:true;text:root.offerValid?qsTr("参数hash %1\n观察fingerprint %2").arg(root.offer.observation.parameterProfileHash).arg(root.offer.observation.observationFingerprint):qsTr("尚无本页实际取得的参数依据；不能手填hash。");textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere}
    Controls.CheckBox {Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;enabled:root.ready&&!startup.pendingInput;checked:root.form.confirmed;text:qsTr("核对后采用此新配置");Accessible.name:text;onToggled:if(checked!==root.form.confirmed)root.change(root.form,{confirmed:checked})}
    QGCButton {Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;enabled:root.ready&&root.form.confirmed&&!startup.pendingInput;text:qsTr("生成并采用本页完整profile");Accessible.name:text;property var pressedIntent:null;onPressed:pressedIntent=root.stamp();onCanceled:pressedIntent=null;onClicked:{var g=root.life,v=pressedIntent;pressedIntent=null;if(g.alive)root.adopt(v)} contentItem:QGCLabel{text:parent.text;font.underline:parent.activeFocus;color:parent._showHighlight?pal.buttonHighlightText:pal.buttonText;textFormat:Text.PlainText;wrapMode:Text.WordWrap;horizontalAlignment:Text.AlignHCenter}}
    QGCLabel {id:errorLabel;Layout.fillWidth:true;activeFocusOnTab:true;text:root.message;textFormat:Text.PlainText;wrapMode:Text.WordWrap;Accessible.name:text}
}

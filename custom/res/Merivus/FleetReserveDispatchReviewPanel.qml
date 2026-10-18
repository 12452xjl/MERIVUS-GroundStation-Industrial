import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Read-only detail for the parent's OWN actual reserve-dispatch review.
// No controller, native source lookup, permission or action interface.
ColumnLayout {
    id: root
    property var reviewContext: ({})
    property var lifetime: ({alive: true})
    property var navigation: ({expanded: false, section: 1, path: [], page: 0, textPage: 0, keyPage: 0, item: 0, encodedPage: 0})
    signal focusVisibilityRequested(var item)

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property bool contextKnown: contextValid(reviewContext)
    readonly property var plan: contextKnown ? reviewContext.plan : ({})
    readonly property var review: contextKnown ? reviewContext.review : ({})
    readonly property var view: contextKnown && map(reviewContext.view) ? reviewContext.view : ({})
    readonly property bool spatialKnown: contextKnown && spatialValid(view, plan)
    readonly property var vehicle: spatialKnown ? view.vehicles[0] : ({})
    readonly property var items: spatialKnown ? vehicle.encodedItems : []
    readonly property string stableIdentity: contextKnown ? JSON.stringify({token: reviewContext.token,
        recordId: reviewContext.archive.recordId, sourceTaskId: reviewContext.archive.sourceTaskId,
        recordRevision: reviewContext.archive.recordRevision, storeEpoch: reviewContext.archive.storeEpoch,
        contentHash: reviewContext.archive.contentHash, reviewEvidenceHash: reviewContext.archive.reviewEvidenceHash,
        planId: plan.planId, vehicleId: plan.members[0].vehicleId}) : ""
    readonly property int encodedPages: Math.max(1, Math.ceil(items.length / 8))
    readonly property int encodedPage: Math.min(navigation.encodedPage, encodedPages - 1)
    readonly property var sectionNames: [qsTr("单项完整协议字段"), qsTr("完整能量计划"), qsTr("完整风险评估"),
        qsTr("完整空域报告"), qsTr("空域输入检查"), qsTr("归档空域显示依据"), qsTr("几何／时间／动作显示"),
        qsTr("完整协议构建结果"), qsTr("完整新审阅及其余原因"), qsTr("显示限制")]
    readonly property var sectionValue: sectionAt(navigation.section)
    readonly property var located: locate(sectionValue, navigation.path)
    readonly property var node: located.value
    readonly property string nodeType: located.present ? typeName(node) : qsTr("缺失／未知")
    readonly property var childNames: childKeys(located)
    readonly property int childPages: Math.max(1, Math.ceil(childNames.length / 8))
    readonly property int childPage: Math.min(navigation.page, childPages - 1)
    readonly property string scalarText: scalar(located)
    readonly property int scalarPages: Math.max(1, Math.ceil(scalarText.length / 256))
    readonly property int scalarPage: Math.min(navigation.textPage, scalarPages - 1)
    readonly property string lastKey: navigation.path.length ? String(navigation.path[navigation.path.length - 1]) : ""
    readonly property int keyPages: Math.max(1, Math.ceil(lastKey.length / 256))
    readonly property int keyPage: Math.min(navigation.keyPage, keyPages - 1)
    spacing: pad
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function map(value) { return value !== null && typeof value === "object" && !Array.isArray(value) }
    function own(value, key) { return map(value) && Object.prototype.hasOwnProperty.call(value, key) }
    function keys(value, expected) {
        if (!map(value) || Object.keys(value).length !== expected.length) return false
        for (var i = 0; i < expected.length; ++i) if (!own(value, expected[i])) return false
        return true
    }
    function integer(value, low, high) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= low && value <= high }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i.test(value) && value !== "00000000-0000-0000-0000-000000000000" }
    function token(value) { return typeof value === "string" && value.length > 0 && value.length <= 256 && !/[\u0000-\u001f\u007f]/.test(value) }
    function contextValid(value) {
        if (!map(value) || value.kind !== "reserve-dispatch" || !token(value.token) || !token(value.proposalToken)
                || typeof value.current !== "boolean" || typeof value.accepted !== "boolean" || typeof value.approvalAllowed !== "boolean"
                || value.flightCommandReleased !== false || value.executable !== false || !map(value.archive)
                || !map(value.plan) || !map(value.review)) return false
        var archived = value.archive, p = value.plan, actual = value.review, source = p.reserveDispatchPlanningSource
        return uuid(archived.recordId) && uuid(archived.sourceTaskId) && uuid(archived.storeEpoch) && integer(archived.recordRevision, 1, 9007199254740991)
            && digest(archived.planId) && digest(archived.contentHash) && digest(archived.reviewEvidenceHash)
            && p.planId === archived.planId && p.sourceTaskId === archived.sourceTaskId && p.schemaVersion === 1 && p.manualConfirmationRequired === true && p.flightCommandReleased === false
            && Array.isArray(p.members) && p.members.length === 1 && map(p.members[0]) && integer(p.members[0].vehicleId, 1, 255)
            && ["leader", "scout", "relay", "responder"].indexOf(p.members[0].role) >= 0
            && Array.isArray(p.reserveVehicleIds) && p.reserveVehicleIds.length === 0
            && map(source) && source.schemaVersion === 1 && source.sourceKind === "reserve-plan-dispatch-proposal"
            && source.reserveVehicleId === p.members[0].vehicleId && source.historicalOnly === true && source.authorityRestored === false
            && source.flightCommandReleased === false && source.executable === false
            && actual.schemaVersion === 1 && actual.manualConfirmationRequired === true && actual.flightCommandReleased === false
            && map(actual.plan) && actual.plan.planId === p.planId && actual.plan.sourceTaskId === p.sourceTaskId && Array.isArray(actual.plan.members)
            && actual.plan.members.length === 1 && map(actual.plan.members[0]) && actual.plan.members[0].vehicleId === p.members[0].vehicleId && actual.plan.members[0].role === p.members[0].role
    }
    function itemValid(value, index) {
        return keys(value, ["sequenceNumber", "command", "frame", "sourcePath", "sourcePointIndexKnown", "sourcePointIndex", "rawItem", "canonicalItem", "wireQuantization"])
            && value.sequenceNumber === index && integer(value.command, 0, 65535) && integer(value.frame, 0, 255)
            && typeof value.sourcePath === "string" && value.sourcePath.length <= 256 && typeof value.sourcePointIndexKnown === "boolean"
            && (value.sourcePointIndexKnown ? integer(value.sourcePointIndex, 0, 2047) : value.sourcePointIndex === null)
            && map(value.rawItem) && map(value.canonicalItem) && value.rawItem.sequenceNumber === index && value.canonicalItem.sequenceNumber === index
            && keys(value.wireQuantization, ["coordinate", "floatParameters"])
            && map(value.wireQuantization.coordinate) && map(value.wireQuantization.floatParameters)
            && value.wireQuantization.coordinate.sequenceNumber === index && value.wireQuantization.floatParameters.sequenceNumber === index
    }
    function encodingErrorValid(value) {
        return keys(value, ["errorCode", "errorDetail", "vehicleId", "route", "pointIndex"])
            && typeof value.errorCode === "string" && value.errorCode.length > 0 && typeof value.errorDetail === "string"
            && integer(value.vehicleId, 0, 255) && typeof value.route === "string"
            && (value.pointIndex === null || integer(value.pointIndex, 0, 2047))
    }
    function spatialValid(value, p) {
        if (!keys(value, ["schemaVersion", "planId", "memberCount", "totalPointCount", "totalItemCount", "geometryComplete", "encodingAvailable", "encodingError", "vehicles", "reserveVehicleIds", "limitations", "snapshotOnly", "executable", "flightCommandReleased", "onboardContentsProven", "clearanceCertified", "airspace"])
                || value.schemaVersion !== 1 || value.planId !== p.planId || value.memberCount !== 1
                || !integer(value.totalPointCount, 0, 2048) || !integer(value.totalItemCount, 0, 2048)
                || typeof value.geometryComplete !== "boolean" || typeof value.encodingAvailable !== "boolean"
                || !map(value.encodingError) || !Array.isArray(value.vehicles) || value.vehicles.length !== 1
                || !Array.isArray(value.reserveVehicleIds) || value.reserveVehicleIds.length !== 0 || !Array.isArray(value.limitations)
                || value.snapshotOnly !== true || value.executable !== false || value.flightCommandReleased !== false
                || value.onboardContentsProven !== false || value.clearanceCertified !== false || !map(value.airspace)) return false
        var row = value.vehicles[0]
        if (!keys(row, ["vehicleId", "role", "geometryAvailable", "geometryError", "routes", "pointCount", "executionProfileAvailable", "launchAction", "endAction", "encodingAvailable", "encodingError", "encodedItems", "payloadHash", "unencodedRequirements", "sourceSchedule"])
                || row.vehicleId !== p.members[0].vehicleId || row.role !== p.members[0].role
                || typeof row.geometryAvailable !== "boolean" || typeof row.geometryError !== "string" || !Array.isArray(row.routes)
                || !integer(row.pointCount, 0, 2048) || row.pointCount !== value.totalPointCount || row.geometryAvailable !== value.geometryComplete
                || typeof row.executionProfileAvailable !== "boolean" || !map(row.launchAction) || !map(row.endAction)
                || row.encodingAvailable !== value.encodingAvailable || !map(row.encodingError) || !Array.isArray(row.encodedItems)
                || row.encodedItems.length !== value.totalItemCount || !Array.isArray(row.unencodedRequirements) || !map(row.sourceSchedule)) return false
        if (!value.encodingAvailable) return row.encodedItems.length === 0 && row.payloadHash === ""
            && encodingErrorValid(value.encodingError) && encodingErrorValid(row.encodingError)
        if (!digest(row.payloadHash) || !row.encodedItems.length || Object.keys(value.encodingError).length || Object.keys(row.encodingError).length) return false
        for (var i = 0; i < row.encodedItems.length; ++i) if (!itemValid(row.encodedItems[i], i)) return false
        return true
    }
    function typeName(value) {
        if (value === null) return "null"
        if (value === undefined) return qsTr("缺失／未知")
        if (Array.isArray(value)) return qsTr("数组")
        if (map(value)) return qsTr("对象")
        if (typeof value === "string") return qsTr("字符串")
        if (typeof value === "boolean") return qsTr("布尔")
        if (typeof value === "number") return isFinite(value) ? qsTr("数字") : qsTr("无效非有限数字")
        return qsTr("不支持的值")
    }
    function sectionAt(index) {
        if (!contextKnown || !navigation.expanded) return {present: false, value: undefined}
        var parent, name
        if (index === 0) return {present: spatialKnown && navigation.item >= 0 && navigation.item < items.length, value: items[navigation.item]}
        if (index === 1) { parent = review; name = "energyPlan" }
        else if (index === 2) { parent = review; name = "riskAssessment" }
        else if (index === 3) { parent = review; name = "airspaceReport" }
        else if (index === 4) { parent = review; name = "airspaceInputCheck" }
        else if (index === 5) return {present: spatialKnown, value: view.airspace}
        else if (index === 6) return {present: spatialKnown, value: vehicle}
        else if (index === 7) { parent = reviewContext; name = "encoding" }
        else if (index === 8) return {present: true, value: review}
        else if (index === 9) return {present: spatialKnown, value: view.limitations}
        else return {present: false, value: undefined}
        return {present: own(parent, name), value: parent[name]}
    }
    function locate(start, path) {
        if (!start.present || !Array.isArray(path) || path.length > 24) return {present: false, value: undefined}
        var value = start.value
        for (var i = 0; i < path.length; ++i) {
            var key = path[i]
            if (Array.isArray(value)) {
                if (!integer(key, 0, value.length - 1)) return {present: false, value: undefined}
            } else if (!own(value, key)) return {present: false, value: undefined}
            value = value[key]
        }
        return {present: true, value: value}
    }
    function childKeys(location) {
        if (!location.present) return []
        var value = location.value, result = []
        if (Array.isArray(value)) {
            if (value.length > 32768) return []
            // Only an eight-row page is materialized, not all child values.
            for (var i = 0; i < value.length; ++i) result.push(i)
        } else if (map(value)) {
            result = Object.keys(value).sort()
            if (result.length > 1024) return []
        }
        return result
    }
    function scalar(location) {
        if (!location.present) return qsTr("该来源未提供此部分；未知不等于已检查或空结果。")
        var value = location.value
        if (value === null) return "null"
        if (value === undefined) return qsTr("undefined（缺失／未知，不补为null或0）")
        if (Array.isArray(value)) return value.length > 32768 ? qsTr("数组超出32768项显示界限；未显示有效前缀。") : value.length === 0 ? "[]（空数组）" : qsTr("逐项下钻；没有整份截断摘要。")
        if (map(value)) return Object.keys(value).length > 1024 ? qsTr("对象超出1024键显示界限；未显示有效前缀。") : Object.keys(value).length === 0 ? "{}（空对象）" : qsTr("逐键下钻；没有整份截断摘要。")
        if (typeof value === "string") return value.length === 0 ? '""（空字符串）' : value
        if (typeof value === "boolean") return value ? "true" : "false"
        if (typeof value === "number") return isFinite(value) ? String(value) : qsTr("无效非有限数字；不补为null或0")
        return qsTr("不支持的值；未归一化或补全。")
    }
    function chunk(value, page) {
        var start = page * 256, end = Math.min(value.length, start + 256)
        // Preserve a UTF-16 surrogate pair at a boundary. The shared pair can
        // appear on adjacent labelled chunks; no source character is lost.
        if (start > 0 && value.charCodeAt(start) >= 0xdc00 && value.charCodeAt(start) <= 0xdfff
                && value.charCodeAt(start - 1) >= 0xd800 && value.charCodeAt(start - 1) <= 0xdbff) --start
        if (end < value.length && value.charCodeAt(end - 1) >= 0xd800 && value.charCodeAt(end - 1) <= 0xdbff
                && value.charCodeAt(end) >= 0xdc00 && value.charCodeAt(end) <= 0xdfff) ++end
        return value.slice(start, end)
    }
    function stamp() { return {identity: stableIdentity, navigation: navigation} }
    function change(expected, changes) {
        var g = lifetime
        if (!g.alive || !expected || expected.identity !== stableIdentity || expected.navigation !== navigation || !visible || !enabled) return
        var next = {expanded: navigation.expanded, section: navigation.section, path: navigation.path.slice(), page: navigation.page,
            textPage: navigation.textPage, keyPage: navigation.keyPage, item: navigation.item, encodedPage: navigation.encodedPage}
        var names = Object.keys(changes)
        for (var i = 0; i < names.length; ++i) next[names[i]] = changes[names[i]]
        navigation = next
        return next // Only a local return after the notifying write.
    }
    function descend(expected, key) {
        if (!expected || expected.navigation !== navigation || navigation.path.length >= 24) return
        if (!located.present || (Array.isArray(node) ? !integer(key, 0, node.length - 1) : !own(node, key))) return
        var path = navigation.path.slice(); path.push(key)
        change(expected, {path: path, page: 0, textPage: 0, keyPage: 0})
    }
    function childType(key) {
        return Array.isArray(node) || map(node) ? typeName(node[key]) : qsTr("缺失／未知")
    }
    function focusDetails() {
        var g = lifetime, identity = stableIdentity, nav = navigation
        detailsHeading.forceActiveFocus()
        if (g.alive && identity === stableIdentity && nav === navigation && visible) focusVisibilityRequested(detailsHeading)
    }
    onStableIdentityChanged: navigation = {expanded: false, section: 1, path: [], page: 0, textPage: 0, keyPage: 0, item: 0, encodedPage: 0}
    Component.onDestruction: lifetime.alive = false

    QGCLabel { Layout.fillWidth: true; text: qsTr("协议、能量与风险详情（只读）"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
    QGCLabel { Layout.fillWidth: true; text: qsTr("只显示本页对应的新归档审阅。编码请求、估算与启发式风险不是机载已接纳、真实执行、净空认证或飞行许可。没有批准、上传或启动接口。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
    QGCLabel { Layout.fillWidth: true; visible: !root.contextKnown; text: qsTr("缺少同kind、token、归档、计划或唯一成员的完整一致身份／固定false边界。详情不认领为此新审阅；请核对对应记录。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
    QGCButton {
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.contextKnown
        text: root.navigation.expanded ? qsTr("收起协议／能量／风险详情（保留页）") : qsTr("展开此记录的协议／能量／风险详情")
        property var pressedState: null
        onPressed: pressedState = root.stamp()
        onCanceled: pressedState = null
        onEnabledChanged: if (!enabled) pressedState = null
        onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.change(s, {expanded: !s.navigation.expanded}) }
        contentItem: QGCLabel { text: parent.text; font.underline: parent.activeFocus; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
    }
    ColumnLayout {
        Layout.fillWidth: true; visible: root.contextKnown && root.navigation.expanded; spacing: root.pad
        QGCLabel { Layout.fillWidth: true; text: qsTr("记录 %1 · 版本 %2\n计划 %3 · 唯一成员 %4\n绑定检查时current=%5；accepted=%6；approvalAllowed=%7。状态变化不重置下方历史浏览，也不恢复确认。") .arg(root.reviewContext.archive ? root.reviewContext.archive.recordId : "").arg(root.reviewContext.archive ? root.reviewContext.archive.recordRevision : "").arg(root.plan.planId || "").arg(root.plan.members ? root.plan.members[0].vehicleId : "").arg(String(root.reviewContext.current)).arg(String(root.reviewContext.accepted)).arg(String(root.reviewContext.approvalAllowed)); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
        QGCLabel { Layout.fillWidth: true; text: root.spatialKnown ? qsTr("几何完整=%1；整份编码可用=%2；协议项总数=%3。几何与编码分别判断，索引均从0起。") .arg(String(root.view.geometryComplete)).arg(String(root.view.encodingAvailable)).arg(root.items.length) : qsTr("Spatial显示投影缺失或合同不一致。不会补编码；仍可浏览同记录的完整能量、风险、空域和审阅原因。父区原规划几何不因此隐藏。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        QGCLabel { Layout.fillWidth: true; visible: root.spatialKnown && !root.view.encodingAvailable; text: qsTr("整份编码失败：没有成功项前缀，不把空items当作没有任务。请从下方‘完整协议构建结果’和‘几何／时间／动作显示’逐字段读取完整错误及未编码要求。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        Repeater {
            model: root.navigation.expanded ? root.items.slice(root.encodedPage * 8, root.encodedPage * 8 + 8) : []
            ColumnLayout {
                id: encodedRow
                readonly property var item: root.map(modelData) ? modelData : ({})
                Layout.fillWidth: true
                QGCLabel { Layout.fillWidth: true; text: qsTr("协议seq %1 · command %2 · frame %3\n源字段 %4 · 源点索引 %5（不是协议seq）；速度／动作项不推作坐标。") .arg(encodedRow.item.sequenceNumber).arg(encodedRow.item.command).arg(encodedRow.item.frame).arg(encodedRow.item.sourcePath).arg(encodedRow.item.sourcePointIndexKnown ? String(encodedRow.item.sourcePointIndex) : "null（未知）"); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                QGCButton {
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("逐字段展开本项 raw／canonical／量化／来源")
                    property var pressedState: null
                    onPressed: { var s = root.stamp(); s.sequence = encodedRow.item.sequenceNumber; pressedState = root.integer(s.sequence, 0, root.items.length - 1) ? s : null }
                    onCanceled: pressedState = null
                    onEnabledChanged: if (!enabled) pressedState = null
                    onClicked: {
                        var g = root.lifetime, value = pressedState
                        pressedState = null
                        if (!g.alive || !value) return
                        var next = root.change(value, {section: 0, path: [], page: 0, textPage: 0, keyPage: 0, item: value.sequence})
                        if (g.alive && next && root.navigation === next) root.focusDetails()
                    }
                    contentItem: QGCLabel { text: parent.text; font.underline: parent.activeFocus; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
                }
            }
        }
        GridLayout {
            Layout.fillWidth: true; visible: root.items.length > 0; columns: width < root.pad * 48 ? 1 : 3
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || activeFocus; text: qsTr("协议上一页"); enabled: root.encodedPage > 0; property var pressedState: null; onPressed: pressedState = root.stamp(); onCanceled: pressedState = null; onEnabledChanged: if (!enabled) pressedState = null; onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive) root.change(s, {encodedPage: Math.max(0, s ? s.navigation.encodedPage - 1 : 0)}) } }
            QGCLabel { Layout.fillWidth: true; text: qsTr("%1 / %2 · 每页8项").arg(root.encodedPage + 1).arg(root.encodedPages); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || activeFocus; text: qsTr("协议下一页"); enabled: root.encodedPage + 1 < root.encodedPages; property var pressedState: null; onPressed: pressedState = root.stamp(); onCanceled: pressedState = null; onEnabledChanged: if (!enabled) pressedState = null; onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive) root.change(s, {encodedPage: s ? s.navigation.encodedPage + 1 : 0}) } }
        }
        QGCLabel { id: detailsHeading; Layout.fillWidth: true; text: qsTr("完整值分层浏览：每层8项，长字符串／键每片约256个UTF-16单位，边界完整保留字符；没有整份大对象pretty或截断冒充完整。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; activeFocusOnTab: true }
        Controls.ComboBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            model: root.sectionNames; currentIndex: root.navigation.section
            Accessible.name: qsTr("选择只读审阅部分")
            onActivated: root.change(root.stamp(), {section: index, path: [], page: 0, textPage: 0, keyPage: 0})
        }
        QGCLabel { Layout.fillWidth: true; text: qsTr("层级 %1 · 类型 %2 · 子项总数 %3；空／缺失／null／false／0分别保留。") .arg(root.navigation.path.length).arg(root.nodeType).arg(root.childNames.length); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        QGCLabel { Layout.fillWidth: true; visible: root.navigation.path.length > 0; text: qsTr("当前完整键／数组索引 · 第 %1 / %2 片：\n%3") .arg(root.keyPage + 1).arg(root.keyPages).arg(root.chunk(root.lastKey, root.keyPage)); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
        RowLayout {
            Layout.fillWidth: true; visible: root.keyPages > 1
            QGCButton { Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || activeFocus; text: qsTr("键上一片"); enabled: root.keyPage > 0; property var pressedState: null; onPressed: pressedState = root.stamp(); onCanceled: pressedState = null; onEnabledChanged: if (!enabled) pressedState = null; onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.change(s, {keyPage: s.navigation.keyPage - 1}) } }
            QGCButton { Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || activeFocus; text: qsTr("键下一片"); enabled: root.keyPage + 1 < root.keyPages; property var pressedState: null; onPressed: pressedState = root.stamp(); onCanceled: pressedState = null; onEnabledChanged: if (!enabled) pressedState = null; onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.change(s, {keyPage: s.navigation.keyPage + 1}) } }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("返回上一级（不离开此记录）"); enabled: root.navigation.path.length > 0
            property var pressedState: null
            onPressed: pressedState = root.stamp()
            onCanceled: pressedState = null
            onEnabledChanged: if (!enabled) pressedState = null
            onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.change(s, {path: s.navigation.path.slice(0, -1), page: 0, textPage: 0, keyPage: 0}) }
            contentItem: QGCLabel { text: parent.text; font.underline: parent.activeFocus; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
        }
        Repeater {
            model: root.navigation.expanded ? root.childNames.slice(root.childPage * 8, root.childPage * 8 + 8) : []
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("展开 %1：%2%3 · %4") .arg(root.childPage * 8 + index).arg(String(modelData).slice(0, 96)).arg(String(modelData).length > 96 ? qsTr("（仅键第1片；展开后可看完整键）") : "").arg(root.childType(modelData))
                property var pressedState: null
                enabled: root.navigation.path.length < 24
                onPressed: { var s = root.stamp(); s.key = modelData; pressedState = s }
                onCanceled: pressedState = null
                onEnabledChanged: if (!enabled) pressedState = null
                onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.descend(s, s.key) }
                contentItem: QGCLabel { text: parent.text; font.underline: parent.activeFocus; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; horizontalAlignment: Text.AlignLeft }
            }
        }
        QGCLabel { Layout.fillWidth: true; visible: root.navigation.path.length >= 24 && root.childNames.length > 0; text: qsTr("达到24层浏览界限；更深内容未显示、未被核验，不把当前页当完整检查。请返回上级核对原记录。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        GridLayout {
            Layout.fillWidth: true; visible: root.childNames.length > 0; columns: width < root.pad * 48 ? 1 : 3
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || activeFocus; text: qsTr("本层上一页"); enabled: root.childPage > 0; property var pressedState: null; onPressed: pressedState = root.stamp(); onCanceled: pressedState = null; onEnabledChanged: if (!enabled) pressedState = null; onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.change(s, {page: s.navigation.page - 1}) } }
            QGCLabel { Layout.fillWidth: true; text: qsTr("%1 / %2 · 每页8项").arg(root.childPage + 1).arg(root.childPages); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || activeFocus; text: qsTr("本层下一页"); enabled: root.childPage + 1 < root.childPages; property var pressedState: null; onPressed: pressedState = root.stamp(); onCanceled: pressedState = null; onEnabledChanged: if (!enabled) pressedState = null; onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.change(s, {page: s.navigation.page + 1}) } }
        }
        QGCLabel { Layout.fillWidth: true; visible: root.located.present && typeof root.node === "string"; text: qsTr("原字符串长度：%1个UTF-16单位；类型标签区分字符串与null／布尔／数字。").arg(typeof root.node === "string" ? root.node.length : 0); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        QGCLabel { Layout.fillWidth: true; text: qsTr("值 · 第 %1 / %2 片：\n%3") .arg(root.scalarPage + 1).arg(root.scalarPages).arg(root.chunk(root.scalarText, root.scalarPage)); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
        RowLayout {
            Layout.fillWidth: true; visible: root.scalarPages > 1
            QGCButton { Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || activeFocus; text: qsTr("值上一片"); enabled: root.scalarPage > 0; property var pressedState: null; onPressed: pressedState = root.stamp(); onCanceled: pressedState = null; onEnabledChanged: if (!enabled) pressedState = null; onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.change(s, {textPage: s.navigation.textPage - 1}) } }
            QGCButton { Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; _showHighlight: pressed || hovered || activeFocus; text: qsTr("值下一片"); enabled: root.scalarPage + 1 < root.scalarPages; property var pressedState: null; onPressed: pressedState = root.stamp(); onCanceled: pressedState = null; onEnabledChanged: if (!enabled) pressedState = null; onClicked: { var g = root.lifetime, s = pressedState; pressedState = null; if (g.alive && s) root.change(s, {textPage: s.navigation.textPage + 1}) } }
        }
    }
}

import QtQuick 2.12
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Presentation only. The native view was built from this exact archived new
// plan, not from the old area draft, a live aircraft or an imported report.
ColumnLayout {
    id: root
    property var reviewContext: ({})
    property bool expanded: false
    property int routeMode: 0
    property int page: 0
    readonly property var archive: reviewContext && reviewContext.archive ? reviewContext.archive : ({})
    readonly property var view: reviewContext && reviewContext.view ? reviewContext.view : ({})
    readonly property string identity: reviewIdentity(reviewContext)
    readonly property bool viewKnown: knownView(view)
    readonly property bool geometryReady: viewKnown && view.geometryAvailable === true
    readonly property bool encodingReady: viewKnown && view.encodingAvailable === true
    readonly property bool selectedAvailable: routeMode === 2 ? encodingReady : geometryReady
    readonly property var selectedRows: rowsForMode()
    readonly property int pageCount: Math.max(1, Math.ceil(selectedRows.length / 8))
    readonly property int currentPage: Math.max(0, Math.min(page, pageCount - 1))
    // Only one page has delegates. Collapsing releases all item delegates;
    // opening per-item detail never creates rows for another page.
    readonly property var pageRows: visible && expanded ? selectedRows.slice(currentPage * 8, currentPage * 8 + 8) : []
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    spacing: pad
    signal focusVisibilityRequested(var item)
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function map(value) { return value !== null && typeof value === "object" && !Array.isArray(value) }
    function list(value, limit) { return Array.isArray(value) && value.length <= limit }
    function finite(value) { return typeof value === "number" && isFinite(value) }
    function integer(value, min, max) { return finite(value) && Math.floor(value) === value && value >= min && value <= max }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function bounded(value, limit) { return typeof value === "string" && value.length <= limit }
    function text(value) { return bounded(value, 2048) && value.length ? value : qsTr("未知 / 未提供") }
    function number(value) { return finite(value) ? String(value) : qsTr("未知") }
    function booleanText(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function reviewIdentity(context) {
        if (!map(context) || !bounded(context.token, 128) || !context.token || !map(context.archive)) return ""
        var a = context.archive
        return JSON.stringify({token: context.token, sourceTaskId: a.sourceTaskId, recordId: a.recordId,
            recordRevision: a.recordRevision, planId: a.planId, contentHash: a.contentHash,
            reviewEvidenceHash: a.reviewEvidenceHash, storeEpoch: a.storeEpoch,
            contextPlanId: (context.plan || {}).planId, viewPlanId: (context.view || {}).planId})
    }
    function knownView(value) {
        if (!identity || !map(value) || Object.keys(value).length !== 15 || value.schemaVersion !== 1
                || typeof value.geometryAvailable !== "boolean" || typeof value.encodingAvailable !== "boolean"
                || !bounded(value.planId, 64) || !bounded(value.geometryError, 256) || !map(value.encodingError)
                || !integer(value.vehicleId, 0, 255) || !bounded(value.role, 128)
                || !list(value.routes, 2) || !list(value.encodedItems, 2048) || !integer(value.pointCount, 0, 2048)
                || !bounded(value.payloadHash, 64) || !list(value.limitations, 255)
                || value.flightCommandReleased !== false || value.executable !== false) return false
        if (value.geometryAvailable || value.encodingAvailable) {
            var plan = reviewContext.plan || {}, members = plan.members
            if (!digest(value.planId) || value.planId !== archive.planId || value.planId !== plan.planId
                    || !list(members, 1) || members.length !== 1 || !map(members[0])
                    || value.vehicleId !== members[0].vehicleId || value.role !== members[0].role) return false
        }
        if (value.geometryAvailable) {
            if (value.routes.length !== 2 || value.pointCount < 1) return false
            var count = 0, seen = Object.create(null)
            for (var i = 0; i < value.routes.length; ++i) {
                var route = value.routes[i]
                if (!map(route) || Object.keys(route).length !== 2 || (route.sourcePath !== "waypoints" && route.sourcePath !== "returnPath")
                        || seen[route.sourcePath] || !list(route.points, 2048)) return false
                seen[route.sourcePath] = true; count += route.points.length
            }
            if (count !== value.pointCount) return false
        } else if (value.routes.length || value.pointCount !== 0) return false
        if (value.encodingAvailable) {
            if (!value.encodedItems.length || !digest(value.payloadHash)) return false
        } else if (value.encodedItems.length || value.payloadHash !== "") return false
        return true
    }
    function rowsForMode() {
        if (routeMode === 2) return encodingReady ? view.encodedItems : []
        if (!geometryReady) return []
        var path = routeMode === 0 ? "waypoints" : "returnPath"
        for (var i = 0; i < view.routes.length; ++i) if (view.routes[i].sourcePath === path) return view.routes[i].points
        return []
    }
    function optionalNumber(row, knownKey, valueKey, min, max) {
        return row[knownKey] === false ? row[valueKey] === null
            : row[knownKey] === true && finite(row[valueKey]) && row[valueKey] >= min && row[valueKey] <= max
    }
    function pointKnown(row) {
        return map(row) && Object.keys(row).length === 14 && integer(row.pointIndex, 0, 2047)
            && row.coordinateSystem === "geodetic" && row.altitudeReference === "amsl"
            && finite(row.latitude) && Math.abs(row.latitude) <= 80 && finite(row.longitude) && Math.abs(row.longitude) <= 180
            && finite(row.altitude) && row.altitude >= -1000 && row.altitude <= 20000
            && optionalNumber(row, "headingKnown", "heading", -Infinity, Infinity)
            && optionalNumber(row, "holdKnown", "holdSeconds", 0, 86400)
            && optionalNumber(row, "elapsedKnown", "estimatedElapsedSeconds", 0, Infinity)
            && (row.originalSourcePointIndexKnown === false ? row.originalSourcePointIndex === null
                : row.originalSourcePointIndexKnown === true && integer(row.originalSourcePointIndex, 0, 2047))
    }
    function itemKnown(row) {
        return map(row) && Object.keys(row).length === 9 && integer(row.sequenceNumber, 0, 2047)
            && integer(row.command, 0, 65535) && integer(row.frame, 0, 255) && bounded(row.sourcePath, 128)
            && (row.sourcePointIndexKnown === false ? row.sourcePointIndex === null
                : row.sourcePointIndexKnown === true && integer(row.sourcePointIndex, 0, 2047))
            && map(row.rawItem) && map(row.canonicalItem) && map(row.wireQuantization)
            && Object.keys(row.rawItem).length <= 16 && Object.keys(row.canonicalItem).length <= 16
            && row.rawItem.sequenceNumber === row.sequenceNumber && row.canonicalItem.sequenceNumber === row.sequenceNumber
    }
    function sourcePathText(value) {
        if (value === "waypoints") return qsTr("新出航航点（waypoints）")
        if (value === "returnPath") return qsTr("新返程航点（returnPath）")
        if (value === "executionProfile.cruiseSpeedMetersPerSecond") return qsTr("巡航速度配置（executionProfile.cruiseSpeedMetersPerSecond）")
        if (value === "executionProfile.returnSpeedMetersPerSecond") return qsTr("返程速度配置（executionProfile.returnSpeedMetersPerSecond）")
        if (value === "executionProfile.launchAction") return qsTr("起始动作配置（executionProfile.launchAction）")
        if (value === "executionProfile.endAction") return qsTr("结束动作配置（executionProfile.endAction）")
        return text(value)
    }
    function pointSummary(row) {
        if (!pointKnown(row)) return qsTr("本行几何字段未确认，未用零值补齐。")
        return qsTr("新%1航点索引 %2（0 基）\n纬度 %3° · 经度 %4° · AMSL 海拔 %5 m\n原出航点索引：%6")
            .arg(routeMode === 0 ? qsTr("出航") : qsTr("返程")).arg(row.pointIndex)
            .arg(number(row.latitude)).arg(number(row.longitude)).arg(number(row.altitude))
            .arg(row.originalSourcePointIndexKnown ? String(row.originalSourcePointIndex) + qsTr("（0 基）") : qsTr("无对应原点 / 未提供；不自行推导"))
    }
    function pointDetails(row) {
        if (!pointKnown(row)) return qsTr("几何明细未确认。")
        return qsTr("坐标系：geodetic；高度基准：AMSL（不是离地高）。\n航向：%1\n完整停留：%2\n本段累计移动估计：%3\n移动估计不含停留；返程段从零重新计时，不是到达时刻或 ETA。")
            .arg(row.headingKnown ? number(row.heading) + qsTr("°（原计划值）") : qsTr("未知 / 未指定；不能视为 0°"))
            .arg(row.holdKnown ? number(row.holdSeconds) + " s" : qsTr("未知 / 未提供；不能视为 0 s"))
            .arg(row.elapsedKnown ? number(row.estimatedElapsedSeconds) + " s" : qsTr("未知 / 未提供"))
    }
    function itemSummary(row) {
        if (!itemKnown(row)) return qsTr("本行协议字段未确认，未自行重建编码。")
        return qsTr("协议序号 %1（0 基，非航点索引） · command %2 · frame %3\n来源：%4\n映射的新路线点索引：%5")
            .arg(row.sequenceNumber).arg(row.command).arg(row.frame).arg(sourcePathText(row.sourcePath))
            .arg(row.sourcePointIndexKnown ? String(row.sourcePointIndex) + qsTr("（0 基）") : qsTr("无路线点映射 / 动作项；未补值"))
    }
    function scalar(value) {
        if (value === null) return qsTr("null（明确空值）")
        if (finite(value)) return String(value)
        if (typeof value === "boolean") return value ? "true" : "false"
        if (value === "system-default") return qsTr("system-default（系统默认偏航，不是 0°）")
        if (value === "not-applicable") return qsTr("not-applicable（不适用）")
        if (value === "absolute-degrees") return qsTr("absolute-degrees（明确机头朝向）")
        return qsTr("未提供 / 不支持的显示值")
    }
    function fieldsText(value, keys) {
        if (!map(value)) return qsTr("未提供有效字段对象")
        var lines = []
        for (var i = 0; i < keys.length; ++i) {
            var key = keys[i]
            if (Object.prototype.hasOwnProperty.call(value, key)) lines.push(key + " = " + scalar(value[key]))
        }
        return lines.length ? lines.join("\n") : qsTr("没有已提供字段；不补零")
    }
    function headingEncodingText(value) {
        if (typeof value === "undefined") return qsTr("没有明确朝向编码记录；不能解释为 0°。")
        if (!map(value) || Object.keys(value).length !== 10
                || value.schemaVersion !== 1 || value.model !== "nav-waypoint-heading-tick256-px4-float-bound-v1"
                || !finite(value.requestedDegrees) || value.requestedDegrees < 0 || value.requestedDegrees >= 360
                || !finite(value.uploadDegrees) || value.uploadDegrees < 0 || value.uploadDegrees >= 360
                || !integer(value.tick, 0, 92159) || value.uploadDegrees !== value.tick / 256
                || !finite(value.circularDeltaDegrees) || Math.abs(value.circularDeltaDegrees) > 0.001953125
                || typeof value.wrappedToZero !== "boolean" || value.stepDegrees !== 0.00390625
                || value.maximumCircularQuantizationErrorDegrees !== 0.001953125
                || value.maximumReadbackErrorDegrees !== 0.0006866455078125) return qsTr("朝向编码记录未知；不推断已接受。")
        return qsTr("逐 NAV 朝向（度）：原请求 %1 → 上传 %2；圆周量化差 %3；跨 360° 归零：%4。\n编码步长 %5；最大量化差 %6；声明回读舍入界 %7。编码精度不是实飞朝向精度，不认证固件或转向轨迹。")
            .arg(value.requestedDegrees).arg(value.uploadDegrees).arg(value.circularDeltaDegrees).arg(value.wrappedToZero ? qsTr("是") : qsTr("否"))
            .arg(value.stepDegrees).arg(value.maximumCircularQuantizationErrorDegrees).arg(value.maximumReadbackErrorDegrees)
    }
    function encodedDetails(row) {
        if (!itemKnown(row)) return qsTr("协议明细未确认。")
        var rawKeys = ["sequenceNumber", "command", "frame", "autoContinue", "isCurrentItem", "param1", "param2", "param3", "param4", "param5", "param6", "param7", "yawMode"]
        var wireKeys = ["sequenceNumber", "command", "frame", "autoContinue", "param1", "param2", "param3", "param4", "param7", "latitudeE7", "longitudeE7", "param5Integer", "param6Integer", "yawMode"]
        var quantization = row.wireQuantization, coordinate = quantization.coordinate, parameters = quantization.floatParameters
        return qsTr("原始任务项 rawItem（上传前字段，不是机载回读）：\n%1\n\n规范化任务项 canonicalItem：\n%2\n\n坐标量化差（度 / 米）：\n%3\n\n浮点量化差（各参数原单位）：\n%4\n%5\n仅显示本地编码；不证明已经上传、机载内容相同或具备执行许可。")
            .arg(fieldsText(row.rawItem, rawKeys)).arg(fieldsText(row.canonicalItem, wireKeys))
            .arg(fieldsText(coordinate, ["sequenceNumber", "coordinateItem", "latitudeDeltaDegrees", "longitudeDeltaDegrees", "altitudeDeltaMeters"]))
            .arg(fieldsText(parameters, ["sequenceNumber"]) + "\n" + fieldsText(map(parameters) ? parameters.deltas : null, ["param1", "param2", "param3", "param4", "param7"]))
            .arg((row.command === 178 ? qsTr("此项是 DO_CHANGE_SPEED：param5 / param6 / param7 的保留零不是纬度、经度或海拔。")
                : qsTr("参数语义依 command / frame；不能把所有 param5 / param6 / param7 都当作坐标。latitudeE7 / longitudeE7 为整数缩放坐标。"))
                + "\n" + headingEncodingText(map(parameters) ? parameters.headingEncoding : undefined))
    }
    function encodingErrorText() {
        if (!viewKnown) return qsTr("未取得匹配本记录的有效显示投影；不能推断编码成功。")
        var error = view.encodingError
        var lines = [qsTr("协议编码不可用：%1").arg(text(error.errorCode)), text(error.errorDetail)]
        if (Object.prototype.hasOwnProperty.call(error, "vehicleId")) lines.push(qsTr("目标机：%1").arg(number(error.vehicleId)))
        if (Object.prototype.hasOwnProperty.call(error, "route")) lines.push(qsTr("路径：%1").arg(sourcePathText(error.route)))
        if (Object.prototype.hasOwnProperty.call(error, "pointIndex")) lines.push(qsTr("点索引（0 基）：%1").arg(number(error.pointIndex)))
        return lines.join("\n")
    }
    function limitationText() {
        if (!viewKnown) return ""
        var rows = view.limitations, lines = []
        for (var i = 0; i < Math.min(rows.length, 12); ++i) lines.push(text(rows[i]))
        if (rows.length > 12) lines.push(qsTr("此处列出前 12 条；原记录仍保留其余限制。"))
        return lines.length ? qsTr("原生显示限制（原文）：\n%1").arg(lines.join("\n")) : qsTr("没有附加限制文本，不表示具备许可。")
    }
    function resetPresentation() { expanded = false; page = 0; routeMode = 0 }
    function keepPaginationFocusVisible() {
        if (!visible) return
        if (heading.activeFocus) { focusVisibilityRequested(heading); return }
        if (!expanded) return
        if (modeChoice.activeFocus) focusVisibilityRequested(modeChoice)
        else if (previousPage.activeFocus) focusVisibilityRequested(previousPage)
        else if (nextPage.activeFocus) focusVisibilityRequested(nextPage)
        else for (var i = 0; i < pageRepeater.count; ++i) {
            var item = pageRepeater.itemAt(i)
            if (item && item.detailButton.activeFocus) { focusVisibilityRequested(item.detailButton); return }
        }
    }
    onIdentityChanged: resetPresentation()
    onRouteModeChanged: page = 0

    QGCButton {
        id: heading
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: root.expanded ? qsTr("收起 · 本次新记录的路线与协议项") : qsTr("展开 · 本次新记录的路线与协议项（只读）")
        Accessible.name: text
        contentItem: QGCLabel { text: heading.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: heading._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: heading.activeFocus }
        onClicked: root.expanded = !root.expanded
        onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
    }
    ColumnLayout {
        Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap
            text: qsTr("记录 %1 · 版本 %2\n计划 %3\n新目标机 %4 · 角色 %5\n已采用：%6；身份当前有效：%7。%8")
                .arg(root.text(root.archive.recordId)).arg(root.number(root.archive.recordRevision)).arg(root.text(root.archive.planId))
                .arg(root.viewKnown && root.integer(root.view.vehicleId, 1, 255) ? root.number(root.view.vehicleId) : qsTr("未知")).arg(root.viewKnown ? root.text(root.view.role) : qsTr("未知"))
                .arg(root.booleanText(root.reviewContext.accepted)).arg(root.booleanText(root.reviewContext.current))
                .arg(root.reviewContext.current === true ? qsTr("仍只是本地审阅，不表示允许上传或飞行。") : qsTr("过期 / 未确认的历史视图，仅供查阅，不能用于放行。"))
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("采用前即可查看本记录；当前地图在明确采用后才切到新路线。航点索引与协议序号均从 0 开始，但不是同一编号。AMSL 不是离地高；未验证障碍、地形、动力学或净空。")
        }
        QGCLabel { Layout.fillWidth: true; visible: !root.geometryReady; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: qgcPal.warningText; text: qsTr("几何路线不可用：%1").arg(root.viewKnown ? root.text(root.view.geometryError) : qsTr("显示投影未确认")) }
        QGCLabel { Layout.fillWidth: true; visible: !root.encodingReady; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: qgcPal.warningText; text: root.encodingErrorText() }
        QGCComboBox {
            id: modeChoice
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            model: [qsTr("出航航点"), qsTr("返程航点"), qsTr("协议编码项（含动作）")]
            currentIndex: root.routeMode; Accessible.name: qsTr("选择新记录审阅内容")
            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: modeChoice.activeFocus }
            onActivated: { root.routeMode = index; currentIndex = Qt.binding(function() { return root.routeMode }) }
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap
            text: root.routeMode === 2
                ? qsTr("协议项 %1；第 %2 / %3 页，每页最多 8 项。\n本地 payload 摘要：%4（完整性标识，不是来源认证）").arg(root.selectedRows.length).arg(root.currentPage + 1).arg(root.pageCount).arg(root.text(root.view.payloadHash))
                : qsTr("本路径 %1 点；第 %2 / %3 页，每页最多 8 点。全部几何路线合计 %4 点。")
                    .arg(root.selectedRows.length).arg(root.currentPage + 1).arg(root.pageCount).arg(root.viewKnown ? root.number(root.view.pointCount) : qsTr("未知"))
        }
        QGCLabel { Layout.fillWidth: true; visible: root.selectedRows.length === 0; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.selectedAvailable ? qsTr("此路径没有已提供条目；不能据此判断任务完成。") : qsTr("所选内容不可用，请查看上方对应原因；另一类内容可独立查阅。") }
        Repeater {
            id: pageRepeater
            model: root.pageRows
            ColumnLayout {
                id: entry
                Layout.fillWidth: true; spacing: root.pad / 2
                readonly property var row: modelData
                readonly property string entryIdentity: root.identity + ":" + root.routeMode + ":" + root.currentPage + ":" + index
                readonly property bool rowKnown: root.routeMode === 2 ? root.itemKnown(row) : root.pointKnown(row)
                property bool detailExpanded: false
                property alias detailButton: detailToggle
                onEntryIdentityChanged: detailExpanded = false
                Rectangle { Layout.fillWidth: true; height: 1; color: qgcPal.text; opacity: 0.25 }
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: root.routeMode === 2 ? root.itemSummary(entry.row) : root.pointSummary(entry.row) }
                QGCButton {
                    id: detailToggle
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    enabled: entry.rowKnown; opacity: enabled ? 1 : 0.45
                    text: entry.detailExpanded ? qsTr("收起本项明细") : root.routeMode === 2 ? qsTr("展开本项原始 / 规范编码与量化差") : qsTr("展开本点航向、停留与移动估计")
                    Accessible.name: text
                    contentItem: QGCLabel { text: detailToggle.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: detailToggle._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: detailToggle.activeFocus }
                    onClicked: entry.detailExpanded = !entry.detailExpanded
                    onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
                }
                QGCLabel { Layout.fillWidth: true; visible: entry.detailExpanded; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: entry.detailExpanded ? (root.routeMode === 2 ? root.encodedDetails(entry.row) : root.pointDetails(entry.row)) : "" }
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: root.pad
            QGCButton {
                id: previousPage
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("上一页"); enabled: root.currentPage > 0; opacity: enabled ? 1 : 0.45; Accessible.name: qsTr("本次新记录上一页")
                contentItem: QGCLabel { text: previousPage.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: previousPage._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: previousPage.activeFocus }
                onClicked: root.page = root.currentPage - 1
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
            QGCButton {
                id: nextPage
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("下一页"); enabled: root.currentPage + 1 < root.pageCount; opacity: enabled ? 1 : 0.45; Accessible.name: qsTr("本次新记录下一页")
                contentItem: QGCLabel { text: nextPage.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: nextPage._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: nextPage.activeFocus }
                onClicked: root.page = root.currentPage + 1
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
        }
        QGCLabel { Layout.fillWidth: true; visible: root.viewKnown; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: root.limitationText() }
    }
}

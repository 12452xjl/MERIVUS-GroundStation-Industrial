import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

ColumnLayout {
    id: root
    property var controller: null
    property var workbench: ({})
    property bool ground: false
    signal mapRequested()
    signal focusVisibilityRequested(var item)
    property bool expanded: false
    property bool busy: false
    property var captureConsent: null
    property var mapConsent: null
    property var capturePress: null
    property var mapPress: null
    property int vehiclePage: 0
    property int candidatePage: 0
    property int detailVehicleId: 0
    property string receipt: ""
    property var lifetime: ({alive:true, ticket:null, epoch:0, exhausted:false})
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var capture: controller && object(controller.fleetStartupSpatialReview) ? controller.fleetStartupSpatialReview : ({})
    readonly property var view: viewKnown(capture.view) ? capture.view : ({})
    readonly property bool captured: typeof capture.captureToken === "string" && capture.captureToken.length > 0 && view.schemaVersion === 1
    readonly property string captureKey: captured ? JSON.stringify([capture.captureToken, capture.binding, view.reviewKind, view.reviewId, view.reviewFingerprint]) : ""
    readonly property string snapshotKey: snapshotKnown(workbench.snapshot) ? JSON.stringify(workbench.snapshot) : ""
    readonly property var sourceReview: object(ground ? workbench.groundReview : workbench.review) ? (ground ? workbench.groundReview : workbench.review) : ({})
    readonly property var vehicles: captured ? view.vehicles : []
    readonly property var shownVehicles: vehicles.slice(vehiclePage * 8, vehiclePage * 8 + 8)
    readonly property var separation: captured && separationKnown(view.separation) ? view.separation : ({})
    readonly property var candidates: Array.isArray(separation.candidates) ? separation.candidates : []
    readonly property var shownCandidates: candidates.slice(candidatePage * 8, candidatePage * 8 + 8)
    readonly property bool captureReady: visible && expanded && !busy && !lifetime.exhausted && controller
        && typeof controller.prepareFleetStartupSpatialReview === "function" && workbench.sourceKnown === true
        && snapshotKey.length > 0 && typeof sourceReview.reviewId === "string" && sourceReview.reviewId.length > 0
    readonly property bool mapReady: visible && expanded && !busy && !lifetime.exhausted && captured && controller
        && typeof controller.selectFleetStartupSpatialMap === "function"
    spacing: ScreenTools.defaultFontPixelWidth
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function object(v) { return v !== null && typeof v === "object" && !Array.isArray(v) }
    function exact(v, keys) {
        if (!object(v) || Object.keys(v).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(v, keys[i])) return false
        return true
    }
    function integer(v, min, max) { return typeof v === "number" && isFinite(v) && Math.floor(v) === v && v >= min && v <= max }
    function text(v) { return typeof v === "string" && v.length ? v : typeof v === "number" && isFinite(v) ? String(v) : qsTr("未知 / 未提供") }
    function yes(v) { return v === true ? qsTr("是") : v === false ? qsTr("否") : qsTr("未知") }
    function focusAfterLayout(item) {
        var guard = lifetime, owner = controller, key = captureKey
        Qt.callLater(function() {
            if (guard.alive && root.visible && root.expanded && owner === root.controller && key === root.captureKey
                    && item && item.activeFocus) root.focusVisibilityRequested(item)
        })
    }
    function snapshotKnown(v) { return exact(v,["sourceToken","identityToken","stamp"]) && typeof v.sourceToken === "string" && v.sourceToken.length > 0 && typeof v.identityToken === "string" && typeof v.stamp === "string" && v.stamp.length > 0 && v.stamp.length <= 65536 }
    function pointKnown(v) { return exact(v,["latitude","longitude","altitude"]) && typeof v.latitude === "number" && isFinite(v.latitude) && Math.abs(v.latitude) <= 90 && typeof v.longitude === "number" && isFinite(v.longitude) && Math.abs(v.longitude) <= 180 && typeof v.altitude === "number" && isFinite(v.altitude) }
    function viewKnown(v) {
        if (!exact(v,["schemaVersion","reviewKind","reviewId","reviewFingerprint","vehicles","segments","summary","limitations","minimumSeparationMeters","separation","snapshotOnly","executable","flightCommandReleased","clearanceCertified","timingKnown"])
                || v.schemaVersion !== 1 || ["normal","ground"].indexOf(v.reviewKind) < 0 || typeof v.reviewId !== "string" || !v.reviewId.length
                || typeof v.reviewFingerprint !== "string" || !/^[0-9a-f]{64}$/.test(v.reviewFingerprint)
                || !Array.isArray(v.vehicles) || v.vehicles.length < 1 || v.vehicles.length > 32 || !Array.isArray(v.segments) || v.segments.length > 96
                || !Array.isArray(v.limitations) || v.snapshotOnly !== true || v.executable !== false || v.flightCommandReleased !== false || v.clearanceCertified !== false || v.timingKnown !== false) return false
        var summary = v.summary
        if (!exact(summary,["vehicleCount","availableVehicleCount","partialVehicleCount","unavailableVehicleCount","segmentCount","allTargetGeometryAvailable"])
                || summary.vehicleCount !== v.vehicles.length || summary.segmentCount !== v.segments.length || typeof summary.allTargetGeometryAvailable !== "boolean") return false
        var ids = Object.create(null), segmentIds = Object.create(null), counts = {available:0,partial:0,unavailable:0}
        for (var i = 0; i < v.vehicles.length; ++i) {
            var row = v.vehicles[i]
            if (!exact(row,["vehicleId","launchType","availability","reasonCodes","currentAltitudeSummary","takeoffDerivation","segmentIds"])
                    || !integer(row.vehicleId,1,255) || ids[row.vehicleId] || ["takeoff","none","unknown"].indexOf(row.launchType) < 0
                    || ["available","partial","unavailable"].indexOf(row.availability) < 0 || !Array.isArray(row.reasonCodes) || !Array.isArray(row.segmentIds) || row.segmentIds.length > 3
                    || !object(row.currentAltitudeSummary) || !object(row.takeoffDerivation)) return false
            ids[row.vehicleId] = row; counts[row.availability] += 1
        }
        for (var j = 0; j < v.segments.length; ++j) {
            var segment = v.segments[j]
            if (!exact(segment,["segmentId","vehicleId","kind","from","to","nominalOnly","actualPathCertified","timingKnown"])
                    || typeof segment.segmentId !== "string" || !segment.segmentId.length || segment.segmentId.length > 128 || segmentIds[segment.segmentId]
                    || !ids[segment.vehicleId] || ["vertical-takeoff","takeoff-request-connection","current-first-waypoint"].indexOf(segment.kind) < 0
                    || !pointKnown(segment.from) || !pointKnown(segment.to) || segment.nominalOnly !== true || segment.actualPathCertified !== false || segment.timingKnown !== false) return false
            segmentIds[segment.segmentId] = segment
        }
        for (var k = 0; k < v.vehicles.length; ++k) {
            var member = v.vehicles[k], seen = Object.create(null)
            for (var n = 0; n < member.segmentIds.length; ++n) {
                var id = member.segmentIds[n]
                if (typeof id !== "string" || seen[id] || !segmentIds[id] || segmentIds[id].vehicleId !== member.vehicleId) return false
                seen[id] = true
            }
        }
        return summary.availableVehicleCount === counts.available && summary.partialVehicleCount === counts.partial
            && summary.unavailableVehicleCount === counts.unavailable && summary.allTargetGeometryAvailable === (counts.available === v.vehicles.length)
    }
    function separationKnown(v) {
        if (!exact(v,["ok","error","comparisonComplete","candidateCount","pairCount","candidates","candidatesTruncated","projection","minimumSeparationMeters","timingKnown","clearanceCertified","executable"])
                || typeof v.ok !== "boolean" || typeof v.error !== "string" || typeof v.comparisonComplete !== "boolean" || typeof v.candidatesTruncated !== "boolean"
                || !integer(v.candidateCount,0,4464) || !integer(v.pairCount,0,4464) || !Array.isArray(v.candidates) || v.candidates.length > 512
                || v.candidates.length > v.candidateCount || !object(v.projection) || v.timingKnown !== false || v.clearanceCertified !== false || v.executable !== false) return false
        for (var i = 0; i < v.candidates.length; ++i) {
            var row = v.candidates[i]
            if (!exact(row,["vehicleA","segmentA","vehicleB","segmentB","distanceMeters","kind"]) || !integer(row.vehicleA,1,255) || !integer(row.vehicleB,1,255)
                    || typeof row.segmentA !== "string" || typeof row.segmentB !== "string" || typeof row.distanceMeters !== "number" || !isFinite(row.distanceMeters) || row.distanceMeters < 0 || row.kind !== "nominal-spatial-proximity") return false
        }
        return true
    }
    function segmentLabel(kind) {
        return kind === "vertical-takeoff" ? qsTr("名义垂直起飞段") : kind === "takeoff-request-connection" ? qsTr("起飞后至 TAKEOFF 请求点的名义连接") : qsTr("捕获机位至第一个 NAV_WAYPOINT 的参考连接")
    }
    function geometryText(row) {
        var parts = []
        for (var i = 0; i < view.segments.length; ++i) {
            var segment = view.segments[i]
            if (segment.vehicleId !== row.vehicleId) continue
            parts.push(segmentLabel(segment.kind) + qsTr("\n起点 %1°, %2° / AMSL %3 m\n终点 %4°, %5° / AMSL %6 m")
                .arg(segment.from.latitude).arg(segment.from.longitude).arg(segment.from.altitude).arg(segment.to.latitude).arg(segment.to.longitude).arg(segment.to.altitude))
        }
        return parts.length ? parts.join("\n\n") : qsTr("没有可绘制的完整几何段。未知不是零高度或无风险。")
    }
    function clearConfirmations() { captureConsent = null; mapConsent = null; capturePress = null; mapPress = null }
    function invalidate() {
        clearConfirmations()
        if (lifetime.epoch >= 9007199254740991) lifetime.exhausted = true
        else lifetime.epoch += 1
    }
    function freezeCapture() {
        return captureReady && !lifetime.ticket ? {owner:controller,epoch:lifetime.epoch,kind:ground ? "ground" : "normal",snapshot:JSON.parse(snapshotKey),key:snapshotKey} : null
    }
    function captureCurrent(value) { return value && value.owner === controller && value.epoch === lifetime.epoch && visible && expanded && value.kind === (ground ? "ground" : "normal") && value.key === snapshotKey }
    function freezeMap(enabled) { return mapReady && !lifetime.ticket ? {owner:controller,epoch:lifetime.epoch,token:capture.captureToken,key:captureKey,enabled:enabled} : null }
    function mapCurrent(value) { return value && value.owner === controller && value.epoch === lifetime.epoch && visible && expanded && value.key === captureKey && value.token === capture.captureToken }
    function perform(kind, intent) {
        if (busy || lifetime.ticket || !intent || (kind === "capture" ? !captureCurrent(intent) || !captureCurrent(captureConsent) : !mapCurrent(intent) || (intent.enabled && !mapCurrent(mapConsent)))) return
        var guard = lifetime, ticket = {}, value = null
        guard.ticket = ticket; busy = true
        try {
            if (!guard.alive || (kind === "capture" ? !captureCurrent(intent) : !mapCurrent(intent))) return
            clearConfirmations()
            value = kind === "capture" ? intent.owner.prepareFleetStartupSpatialReview(intent.kind,intent.snapshot,true)
                : intent.owner.selectFleetStartupSpatialMap(intent.token,intent.enabled,true)
            if (!guard.alive) return
            if (!exact(value,["ok","error","token","current"]) || typeof value.ok !== "boolean" || typeof value.error !== "string" || typeof value.token !== "string" || typeof value.current !== "boolean") {
                receipt = qsTr("本地视图操作回执不完整，当前冻结或选择结果未知；请核对界面，不自动重试。")
                return
            }
            receipt = qsTr("最近一次本地视图操作（历史）：%1；返回时当前来源 %2；原因 %3。\n返回捕获标识 %4。仅冻结/选择显示，没有飞行指令；未确认接受不等于视图绝未变化。")
                .arg(value.ok ? qsTr("已接受") : qsTr("未确认接受")).arg(yes(value.current)).arg(value.error.length ? value.error : qsTr("无")).arg(text(value.token))
            if (kind === "map" && intent.enabled && value.ok && mapCurrent(intent) && capture.mapSelected === true) mapRequested()
        } catch (error) {
            if (guard.alive) receipt = qsTr("本地视图调用中断，冻结或选择可能已变化；请核对当前结果，未自动重试。")
        } finally {
            if (guard.alive) busy = false
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    onControllerChanged: invalidate()
    onGroundChanged: invalidate()
    onSnapshotKeyChanged: { captureConsent = null; capturePress = null }
    onCaptureKeyChanged: { mapConsent = null; mapPress = null; vehiclePage = 0; candidatePage = 0; detailVehicleId = 0 }
    onVisibleChanged: if (!visible) invalidate()
    Component.onDestruction: lifetime.alive = false

    QGCButton {
        id: expandButton
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: root.expanded ? qsTr("收起启动空间核对（只读）") : qsTr("展开启动空间核对（只读）")
        contentItem: QGCLabel { text: expandButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: expandButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        onClicked: { root.expanded = !root.expanded; root.invalidate() }
        onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
        Rectangle { anchors.fill: parent; color:"transparent"; border.width:expandButton.activeFocus ? 2 : 0; border.color:qgcPal.buttonHighlight }
    }
    ColumnLayout {
        visible: root.expanded; Layout.fillWidth: true; spacing: ScreenTools.defaultFontPixelWidth
        QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap; text:qsTr("空间核对是名义几何，不是实际航迹、碰撞认证或启动许可。垂直段在二维地图上只是同一经纬度标记；必须同时核对下方 AMSL 高度。相对高度不自动当作离地高或换算坐标系。") }
        QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.Wrap; text:qsTr("待冻结来源：%1；审阅 %2。读取已有审阅快照，不刷新遥测，不自动重取或换成选中的历史 Run。").arg(root.ground ? qsTr("本次地面再审") : qsTr("本次初次启动审阅")).arg(root.text(root.sourceReview.reviewId)) }
        Controls.CheckBox {
            id:captureCheck
            Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; enabled:root.captureReady
            text:qsTr("明确冻结本次审阅的空间依据；新捕获会替换旧空间视图并取消其地图选择")
            checked:root.captureCurrent(root.captureConsent)
            contentItem:QGCLabel { text:captureCheck.text; textFormat:Text.PlainText; wrapMode:Text.WordWrap; leftPadding:(captureCheck.indicator ? captureCheck.indicator.width : 0)+captureCheck.spacing; color:qgcPal.text }
            onClicked:{ root.captureConsent = checked ? root.freezeCapture() : null; checked = Qt.binding(function(){return root.captureCurrent(root.captureConsent)}) }
            onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
        }
        QGCButton {
            Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
            text:qsTr("冻结本次启动空间快照"); enabled:root.captureReady && root.captureCurrent(root.captureConsent)
            onPressed:root.capturePress=root.freezeCapture(); onCanceled:root.capturePress=null
            onClicked:{ var intent=root.capturePress; root.capturePress=null; root.perform("capture",intent) }
            onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
        }
        QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.Wrap; visible:!root.captured; text:qsTr("尚无完整可查看的冻结空间视图：%1。空对象不表示没有风险。").arg(root.text(root.capture.error)) }
        ColumnLayout {
            visible:root.captured; Layout.fillWidth:true; spacing:ScreenTools.defaultFontPixelWidth
            QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.Wrap; text:root.captured ? qsTr("已冻结 %1 · %2\n审阅 %3\n指纹 %4\n归档记录 %5 / 版本 %6\n捕获 %7\n来源过期只保留历史参考，不自动选中新审阅。").arg(root.view.reviewKind === "ground" ? qsTr("地面再审") : qsTr("初次审阅")).arg(root.capture.current === true ? qsTr("当前绑定快照") : qsTr("已过期历史快照"))
                .arg(root.text(root.view.reviewId)).arg(root.text(root.view.reviewFingerprint)).arg(root.text((root.capture.binding||{}).recordId)).arg(root.text((root.capture.binding||{}).recordRevision)).arg(root.text(root.capture.captureToken)) : "" }
            QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap; text:root.captured ? qsTr("目标 %1 架：几何可展示 %2、部分 %3、不可用 %4；共 %5 段。可展示只指输入几何完整，绝不是可起飞。").arg(root.view.summary.vehicleCount).arg(root.view.summary.availableVehicleCount).arg(root.view.summary.partialVehicleCount).arg(root.view.summary.unavailableVehicleCount).arg(root.view.summary.segmentCount) : "" }
            QGCLabel { Layout.fillWidth:true; visible:root.captured && root.view.reviewKind !== (root.ground ? "ground" : "normal"); textFormat:Text.PlainText; wrapMode:Text.WordWrap; text:qsTr("当前工作台分支与已冻结来源不同。下表仍属于标题所示的原捕获；不会自动解释成当前分支，需明确重新冻结才替换。") }
            QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap; text:qsTr("三类段彼此独立：垂直起飞、至 TAKEOFF 请求点连接、捕获机位至首 NAV_WAYPOINT 参考连接；不能拼成已验证的连续真实航迹。") }
            Repeater {
                model:root.expanded ? root.shownVehicles : []
                ColumnLayout {
                    Layout.fillWidth:true
                    readonly property var row:modelData
                    QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.Wrap; text:qsTr("机 %1 · 起飞类型 %2 · 几何 %3\n原因：%4\n%5").arg(row.vehicleId).arg(root.text(row.launchType)).arg(row.availability === "available" ? qsTr("可展示") : row.availability === "partial" ? qsTr("部分可展示") : qsTr("不可用"))
                        .arg(row.reasonCodes.length ? row.reasonCodes.join("；") : qsTr("无几何拒绝原因；未认证边界仍存在")).arg(root.geometryText(row)) }
                    QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; text:root.detailVehicleId===row.vehicleId ? qsTr("收起该机高度依据") : qsTr("展开该机原始高度依据 / 推导边界"); onClicked:{root.detailVehicleId=root.detailVehicleId===row.vehicleId ? 0 : row.vehicleId;root.focusAfterLayout(this)}; onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this) }
                    QGCLabel { Layout.fillWidth:true; visible:root.detailVehicleId===row.vehicleId; textFormat:Text.PlainText; wrapMode:Text.Wrap; text:visible ? JSON.stringify({currentAltitudeSummary:row.currentAltitudeSummary,takeoffDerivation:row.takeoffDerivation},null,2) : "" }
                }
            }
            QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap; text:qsTr("逐机第 %1 / %2 页；每页最多 8 架。").arg(root.vehiclePage+1).arg(Math.max(1,Math.ceil(root.vehicles.length/8))) }
            RowLayout {
                Layout.fillWidth:true
                QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; text:qsTr("上一组飞机"); enabled:root.vehiclePage>0; opacity:enabled?1:0.45; onClicked:{root.vehiclePage-=1;root.detailVehicleId=0;root.focusAfterLayout(this)}; onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this) }
                QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; text:qsTr("下一组飞机"); enabled:(root.vehiclePage+1)*8<root.vehicles.length; opacity:enabled?1:0.45; onClicked:{root.vehiclePage+=1;root.detailVehicleId=0;root.focusAfterLayout(this)}; onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this) }
            }
            QGCLabel { Layout.fillWidth:true; font.bold:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap; text:qsTr("跨机名义空间接近候选（时间未知）") }
            QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.Wrap; text:root.separation.comparisonComplete === true
                ? qsTr("比较 %1 对，发现 %2 个候选；显示明细 %3 个，截断 %4。声明间距 %5 m。%6\n即使候选为 0，也不证明时序、地形、空域或净空安全。").arg(root.separation.pairCount).arg(root.separation.candidateCount).arg(root.candidates.length).arg(root.yes(root.separation.candidatesTruncated)).arg(root.text(root.separation.minimumSeparationMeters)).arg(root.separation.error||"")
                : qsTr("空间比较未完成 / 不可用：%1；声明间距 %2 m。没有候选列表不能解释成已排除接近。").arg(root.text(root.separation.error)).arg(root.text(root.view.minimumSeparationMeters)) }
            QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WordWrap; text:root.captured && (root.view.summary.allTargetGeometryAvailable !== true || root.view.summary.segmentCount === 0)
                ? qsTr("整组几何尚不完整：比较完成仅覆盖已表示的段，未知或未表示目标没有被证明安全。候选是静态名义接近，不是碰撞或同时到达预测。")
                : qsTr("候选是静态名义接近，不是碰撞或同时到达预测。报告完整也不等于风险通过；单机没有跨机配对仍不能证明启动安全。") }
            Repeater { model:root.expanded ? root.shownCandidates : []; QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.Wrap; text:qsTr("机 %1 [%2] ↔ 机 %3 [%4]：名义最近距离 %5 m；无同时到达证据。").arg(modelData.vehicleA).arg(modelData.segmentA).arg(modelData.vehicleB).arg(modelData.segmentB).arg(modelData.distanceMeters) } }
            RowLayout {
                Layout.fillWidth:true; visible:root.candidates.length>8
                QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; text:qsTr("上一页候选"); enabled:root.candidatePage>0; opacity:enabled?1:0.45; onClicked:{root.candidatePage-=1;root.focusAfterLayout(this)}; onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this) }
                QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; text:qsTr("下一页候选"); enabled:(root.candidatePage+1)*8<root.candidates.length; opacity:enabled?1:0.45; onClicked:{root.candidatePage+=1;root.focusAfterLayout(this)}; onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this) }
            }
            QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.Wrap; text:root.captured ? qsTr("投影依据：%1\n未认证项：%2\n以上接近候选只比较启动参考段，不覆盖下方完整任务出航、返程或终结下降。").arg(JSON.stringify(root.separation.projection||{},null,2)).arg(root.view.limitations.join("；")) : "" }
            FleetMissionSpatialPanel {
                Layout.fillWidth:true
                capture:root.capture
                onFocusVisibilityRequested:root.focusVisibilityRequested(item)
            }
            Controls.CheckBox {
                id:mapCheck
                Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; enabled:root.mapReady && root.capture.mapSelected !== true
                text:qsTr("仅在地图显示这份冻结来源的启动参考段与原计划独立图层；过期时只作历史参考，不回退其他路线")
                checked:root.mapCurrent(root.mapConsent)
                contentItem:QGCLabel { text:mapCheck.text; textFormat:Text.PlainText; wrapMode:Text.WordWrap; leftPadding:(mapCheck.indicator ? mapCheck.indicator.width : 0)+mapCheck.spacing; color:qgcPal.text }
                onClicked:{root.mapConsent=checked ? root.freezeMap(true) : null;checked=Qt.binding(function(){return root.mapCurrent(root.mapConsent)})}
                onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
            }
            QGCButton {
                Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
                text:root.capture.mapSelected === true ? qsTr("结束本次启动空间地图显示") : qsTr("在地图核对这份冻结空间来源")
                enabled:root.mapReady && (root.capture.mapSelected === true || root.mapCurrent(root.mapConsent))
                onPressed:root.mapPress=root.freezeMap(root.capture.mapSelected !== true);onCanceled:root.mapPress=null
                onClicked:{var intent=root.mapPress;root.mapPress=null;root.perform("map",intent)}
                onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
            }
        }
        QGCLabel { Layout.fillWidth:true; visible:root.receipt.length>0; text:root.receipt; textFormat:Text.PlainText; wrapMode:Text.Wrap }
    }
}

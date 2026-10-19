import QtQuick 2.12
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Read-only inspection of the complete archived plan in the same startup
// capture. No controls here can approve, upload, start or change a mission.
ColumnLayout {
    id: root
    property var capture: ({})
    signal focusVisibilityRequested(var item)
    property bool expanded: false
    property int memberIndex: 0
    property int routeMode: 0
    property int page: 0
    property bool detailsExpanded: false
    property int detailsMode: 0
    property var detailsPath: []
    property int detailsPage: 0
    property var selectedTiming: null
    property var lifetime: ({alive:true})
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property string identity: object(capture.binding) && typeof capture.captureToken === "string"
        ? JSON.stringify([capture.captureToken,capture.sourceToken,capture.binding]) : ""
    readonly property string expectedPlanId: object(capture.binding) && typeof capture.binding.planId === "string" ? capture.binding.planId : ""
    // The controller retains this detached value across live current/age
    // updates. Never stringify the complete 8192-item view for UI identity.
    readonly property var missionReview: object(capture.missionReview) ? capture.missionReview : ({})
    readonly property var view: knownReview(missionReview,expectedPlanId) ? missionReview.view : ({})
    readonly property bool available: view.schemaVersion === 1
    // Independently inspect the new result from THIS capture. Its failure
    // must neither hide the archived plan nor borrow an earlier joint view.
    readonly property var jointReview: object(capture.jointReview) ? capture.jointReview : ({})
    readonly property var jointView: knownJointReview(jointReview) ? jointReview.view : ({})
    readonly property bool jointAvailable: jointView.schemaVersion === 1
    readonly property var vehicles: available ? view.vehicles : []
    readonly property var member: memberIndex >= 0 && memberIndex < vehicles.length ? vehicles[memberIndex] : ({})
    readonly property var memberChoices: {
        var choices=[]
        for(var i=0;i<vehicles.length;++i) choices.push(qsTr("机 %1 · %2").arg(vehicles[i].vehicleId).arg(vehicles[i].role))
        return choices
    }
    readonly property var route: routeForMode()
    readonly property var selectedRows: routeMode === 2 ? (member.encodingAvailable === true ? member.encodedItems : []) : (member.geometryAvailable === true ? route.points || [] : [])
    readonly property int pageCount: Math.max(1,Math.ceil(selectedRows.length/8))
    readonly property int currentPage: Math.max(0,Math.min(page,pageCount-1))
    readonly property var pageRows: visible && expanded ? selectedRows.slice(currentPage*8,currentPage*8+8) : []
    readonly property var airspace: available && knownAirspace(view.airspace) ? view.airspace : ({})
    readonly property var detailsRoot: detailRoot()
    readonly property var detailsNode: nodeAt(detailsRoot,detailsPath)
    readonly property var detailKeys: object(detailsNode) ? Object.keys(detailsNode) : []
    readonly property int detailCount: Array.isArray(detailsNode) ? detailsNode.length : object(detailsNode) ? detailKeys.length : typeof detailsNode === "string" ? Math.max(1,Math.ceil(detailsNode.length/2048)) : 1
    readonly property int detailPageCount: Math.max(1,Math.ceil(detailCount/8))
    readonly property var detailRows: detailEntries()
    spacing: pad
    QGCPalette { id:qgcPal; colorGroupEnabled:root.enabled }

    function object(v) { return v !== null && typeof v === "object" && !Array.isArray(v) }
    function exact(v,keys) {
        if(!object(v)||Object.keys(v).length!==keys.length)return false
        for(var i=0;i<keys.length;++i)if(!Object.prototype.hasOwnProperty.call(v,keys[i]))return false
        return true
    }
    function finite(v) { return typeof v === "number" && isFinite(v) }
    function integer(v,min,max) { return finite(v)&&Math.floor(v)===v&&v>=min&&v<=max }
    function digest(v) { return typeof v === "string" && /^[0-9a-f]{64}$/.test(v) }
    function text(v) { return typeof v === "string" && v.length ? v : qsTr("未知 / 未提供") }
    function number(v) { return finite(v) ? String(v) : qsTr("未知") }
    function yes(v) { return v===true ? qsTr("是") : v===false ? qsTr("否") : qsTr("未知") }
    function airspaceInputText(v) {
        if(v==="recorded-list")return qsTr("已记录列表（内容不因此有效）")+" / "+v
        if(v==="recorded-omitted")return qsTr("当时未提供列表")+" / "+v
        if(v==="recorded-invalid")return qsTr("当时输入无效")+" / "+v
        if(v==="legacy-unknown")return qsTr("旧记录来源未知")+" / "+v
        return qsTr("未知 / 未确认")
    }
    function actionKnown(v) {
        return exact(v,["type","declared","coordinateKnown","coordinate","encodingKnown","encoded","sequenceNumber"])
            && typeof v.type==="string" && typeof v.declared==="boolean" && typeof v.coordinateKnown==="boolean"
            && object(v.coordinate) && typeof v.encodingKnown==="boolean" && typeof v.encoded==="boolean"
            && (v.sequenceNumber===null || integer(v.sequenceNumber,0,2047))
    }
    function knownReview(result,planId) {
        if(!exact(result,["ok","error","view"])||result.ok!==true||typeof result.error!=="string")return false
        var v=result.view
        if(!exact(v,["schemaVersion","planId","memberCount","totalPointCount","totalItemCount","geometryComplete","encodingAvailable","encodingError","vehicles","reserveVehicleIds","limitations","snapshotOnly","executable","flightCommandReleased","onboardContentsProven","clearanceCertified","airspace"])
                ||v.schemaVersion!==1||!digest(v.planId)||v.planId!==planId||!integer(v.memberCount,1,32)
                ||!integer(v.totalPointCount,0,8192)||!integer(v.totalItemCount,0,8192)||typeof v.geometryComplete!=="boolean"||typeof v.encodingAvailable!=="boolean"
                ||!object(v.encodingError)||!Array.isArray(v.vehicles)||v.vehicles.length!==v.memberCount||!Array.isArray(v.reserveVehicleIds)||v.reserveVehicleIds.length>255
                ||!Array.isArray(v.limitations)||v.snapshotOnly!==true||v.executable!==false||v.flightCommandReleased!==false||v.onboardContentsProven!==false||v.clearanceCertified!==false)return false
        var ids=Object.create(null),points=0,items=0,allGeometry=true
        for(var i=0;i<v.vehicles.length;++i){
            var row=v.vehicles[i]
            if(!exact(row,["vehicleId","role","geometryAvailable","geometryError","routes","pointCount","executionProfileAvailable","launchAction","endAction","encodingAvailable","encodingError","encodedItems","payloadHash","unencodedRequirements","sourceSchedule"])
                    ||!integer(row.vehicleId,1,255)||ids[row.vehicleId]||typeof row.role!=="string"||typeof row.geometryAvailable!=="boolean"||typeof row.geometryError!=="string"
                    ||!Array.isArray(row.routes)||row.routes.length>2||!integer(row.pointCount,0,2048)||typeof row.executionProfileAvailable!=="boolean"
                    ||!actionKnown(row.launchAction)||!actionKnown(row.endAction)||typeof row.encodingAvailable!=="boolean"||!object(row.encodingError)
                    ||!Array.isArray(row.encodedItems)||row.encodedItems.length>2048||typeof row.payloadHash!=="string"||!Array.isArray(row.unencodedRequirements)||!object(row.sourceSchedule))return false
            ids[row.vehicleId]=true
            var count=0,paths=Object.create(null)
            for(var j=0;j<row.routes.length;++j){
                var r=row.routes[j]
                if(!exact(r,["sourcePath","sourceScope","sourceMetadata","points"])||["waypoints","returnPath"].indexOf(r.sourcePath)<0||paths[r.sourcePath]
                        ||["member","plan-global"].indexOf(r.sourceScope)<0||!object(r.sourceMetadata)||!Array.isArray(r.points)||r.points.length>2048)return false
                paths[r.sourcePath]=true;count+=r.points.length
            }
            if(count!==row.pointCount||(!row.geometryAvailable&&(count!==0||row.routes.length!==0))
                    ||(row.encodingAvailable ? !digest(row.payloadHash)||!row.encodedItems.length : row.payloadHash!==""||row.encodedItems.length!==0))return false
            allGeometry=allGeometry&&row.geometryAvailable;points+=count;items+=row.encodedItems.length
        }
        return points===v.totalPointCount&&items===v.totalItemCount&&allGeometry===v.geometryComplete
    }
    function knownAirspace(v) {
        return exact(v,["available","inputState","noFlyZones","zoneCount","minimumSeparationMeters","thresholdSource","report","scope","limitations","coverageCertified","clearanceCertified","timingCertified"])
            &&typeof v.available==="boolean"&&typeof v.inputState==="string"&&Array.isArray(v.noFlyZones)
            &&(v.zoneCount===null||integer(v.zoneCount,0,1000000))&&(v.minimumSeparationMeters===null||finite(v.minimumSeparationMeters)&&v.minimumSeparationMeters>0)
            &&typeof v.thresholdSource==="string"&&object(v.report)&&v.scope==="archived-planning-routes-only"&&Array.isArray(v.limitations)
            &&v.coverageCertified===false&&v.clearanceCertified===false&&v.timingCertified===false
    }
    function jointAnalysisKnown(v) {
        if(!exact(v,["ok","error","comparisonComplete","segmentCount","vehicleCount","zoneCount","zoneInputCheck","pairCount","candidateCount","candidates","candidatesTruncated","zoneComparisonCount","zoneHitCount","zoneHits","zoneHitsTruncated","projection","minimumSeparationMeters","timingKnown","clearanceCertified","executable","flightCommandReleased"])
                ||typeof v.ok!=="boolean"||typeof v.error!=="string"||typeof v.comparisonComplete!=="boolean"
                ||(v.segmentCount!==null&&!integer(v.segmentCount,0,8320))||(v.vehicleCount!==null&&!integer(v.vehicleCount,0,32))
                ||(v.zoneCount!==null&&!integer(v.zoneCount,0,1024))||!integer(v.pairCount,0,1000000)||!integer(v.candidateCount,0,1000000)
                ||!integer(v.zoneComparisonCount,0,1000000)||!integer(v.zoneHitCount,0,1000000)
                ||!Array.isArray(v.candidates)||v.candidates.length>512||v.candidates.length>v.candidateCount||typeof v.candidatesTruncated!=="boolean"
                ||!Array.isArray(v.zoneHits)||v.zoneHits.length>512||v.zoneHits.length>v.zoneHitCount||typeof v.zoneHitsTruncated!=="boolean"
                ||(v.minimumSeparationMeters!==null&&(!finite(v.minimumSeparationMeters)||v.minimumSeparationMeters<=0))
                ||v.timingKnown!==false||v.clearanceCertified!==false||v.executable!==false||v.flightCommandReleased!==false)return false
        var input=v.zoneInputCheck,projection=v.projection
        if(!exact(input,["valid","state","reason","zoneCount","fingerprint"])||typeof input.valid!=="boolean"
                ||["list","invalid"].indexOf(input.state)<0||typeof input.reason!=="string"||(input.zoneCount!==null&&!integer(input.zoneCount,0,1024))
                ||typeof input.fingerprint!=="string"||(input.fingerprint!==""&&!digest(input.fingerprint))
                ||!exact(projection,["model","referenceLatitude","referenceLongitude","latitudeSpanDegrees","longitudeSpanDegrees","maxAbsoluteLatitudeDegrees","withinSupportedDomain"])
                ||projection.model!=="local-equirectangular-approximation"||typeof projection.withinSupportedDomain!=="boolean")return false
        var numbers=["referenceLatitude","referenceLongitude","latitudeSpanDegrees","longitudeSpanDegrees","maxAbsoluteLatitudeDegrees"]
        for(var i=0;i<numbers.length;++i)if(projection[numbers[i]]!==null&&!finite(projection[numbers[i]]))return false
        // Candidate/zone-hit internals remain bounded, read-only field data.
        // Do not recalculate geometry or derive safety from their counts here.
        return true
    }
    function knownJointReview(result) {
        if(!exact(result,["ok","error","view"])||result.ok!==true||typeof result.error!=="string"
                ||typeof capture.captureToken!=="string"||!capture.captureToken.length||!object(capture.view))return false
        var v=result.view,startup=capture.view
        if(!exact(v,["schemaVersion","planId","reviewKind","reviewId","reviewFingerprint","memberCount","segments","segmentSources","coverageComplete","memberCoverage","zoneInputState","minimumSeparationMeters","thresholdSource","analysis","limitations","snapshotOnly","executable","flightCommandReleased","actualPathCertified","clearanceCertified","timingKnown","onboardContentsProven"])
                ||v.schemaVersion!==1||!digest(v.planId)||v.planId!==expectedPlanId
                ||["normal","ground"].indexOf(v.reviewKind)<0||v.reviewKind!==startup.reviewKind
                ||typeof v.reviewId!=="string"||!v.reviewId.length||v.reviewId!==startup.reviewId
                ||!digest(v.reviewFingerprint)||v.reviewFingerprint!==startup.reviewFingerprint||!integer(v.memberCount,1,32)
                ||!Array.isArray(v.segments)||v.segments.length>8320||!Array.isArray(v.segmentSources)||v.segmentSources.length!==v.segments.length
                ||typeof v.coverageComplete!=="boolean"||!Array.isArray(v.memberCoverage)||v.memberCoverage.length!==v.memberCount
                ||!Array.isArray(startup.vehicles)||startup.vehicles.length!==v.memberCount
                ||["recorded-list","recorded-omitted","recorded-invalid","legacy-unknown"].indexOf(v.zoneInputState)<0
                ||(v.minimumSeparationMeters!==null&&(!finite(v.minimumSeparationMeters)||v.minimumSeparationMeters<=0))
                ||typeof v.thresholdSource!=="string"||!jointAnalysisKnown(v.analysis)||!Array.isArray(v.limitations)
                ||v.snapshotOnly!==true||v.executable!==false||v.flightCommandReleased!==false||v.actualPathCertified!==false
                ||v.clearanceCertified!==false||v.timingKnown!==false||v.onboardContentsProven!==false)return false
        // Both native displays have the complete original member order. If
        // the mission display is unavailable, keep that failure independent;
        // the startup identity/member set still binds this joint report.
        if(available&&vehicles.length!==v.memberCount)return false
        var ids=Object.create(null),segments=0
        for(var i=0;i<v.memberCoverage.length;++i){
            var row=v.memberCoverage[i]
            if(!exact(row,["vehicleId","role","encodingAvailable","executionProfileAvailable","startupAvailability","payloadHash","coverageComplete","reasonCodes","segmentCount","encodedSegmentCount","startupSegmentCount","takeoffSequenceVerified"])
                    ||!integer(row.vehicleId,1,255)||ids[row.vehicleId]||!object(startup.vehicles[i])||row.vehicleId!==startup.vehicles[i].vehicleId
                    ||(available&&row.vehicleId!==vehicles[i].vehicleId)||typeof row.role!=="string"
                    ||typeof row.encodingAvailable!=="boolean"||typeof row.executionProfileAvailable!=="boolean"
                    ||["available","partial","unavailable"].indexOf(row.startupAvailability)<0
                    ||typeof row.payloadHash!=="string"||(row.payloadHash!==""&&!digest(row.payloadHash))||typeof row.coverageComplete!=="boolean"
                    ||!Array.isArray(row.reasonCodes)||!integer(row.segmentCount,0,8320)||!integer(row.encodedSegmentCount,0,2048)
                    ||!integer(row.startupSegmentCount,0,3)||row.segmentCount!==row.encodedSegmentCount+row.startupSegmentCount
                    ||typeof row.takeoffSequenceVerified!=="boolean")return false
            for(var j=0;j<row.reasonCodes.length;++j)if(typeof row.reasonCodes[j]!=="string")return false
            ids[row.vehicleId]=true;segments+=row.segmentCount
        }
        return segments===v.segments.length
    }
    function jointFailure() {
        return exact(jointReview,["ok","error","view"])&&jointReview.ok===false&&typeof jointReview.error==="string"&&jointReview.error.length
            ? jointReview.error : qsTr("联合报告缺失、字段不完整或与本次捕获身份不一致；不借用旧报告")
    }
    function jointSummary() {
        if(!jointAvailable)return qsTr("本次联合静态报告不可展示：%1。原计划与归档空域报告仍独立保留；未知不补成零候选。").arg(jointFailure())
        var v=jointView,a=v.analysis
        return qsTr("本次联合静态报告 · 审阅 %1 / %2\n计划 %3；与上方同一捕获、原记录及全目标集绑定。%4。\n表示范围齐备 %5（coverageComplete）；已表示几何的计算完成 %6（comparisonComplete）。两者独立，不表示实际路径已验证。\n段 %7；参与计算的机数 %8；区域数 %9；跨机段对 %10；段/区域比较 %11。\n空间接近候选 %12：保留明细 %13，明细截断 %14。区域命中 %15：保留明细 %16，明细截断 %17。\n禁飞区来源 %18；间距 %19 m / 来源 %20。\n计算诊断：%21\n本地 canonical 请求链与独立启动假设分别分析，不拼成实际航迹。零候选 / 零命中不表示没有风险；地形、净空、动态四维时序、实际起降轨迹与机载内容均未认证。请在第六类字段中查看各机缺口、段来源、限制和候选原值。")
            .arg(v.reviewKind).arg(v.reviewId).arg(v.planId)
            .arg(capture.current===true?qsTr("当前绑定的历史快照"):qsTr("已过期 / 非当前的历史快照"))
            .arg(yes(v.coverageComplete)).arg(yes(a.comparisonComplete)).arg(number(a.segmentCount)).arg(number(a.vehicleCount)).arg(number(a.zoneCount))
            .arg(a.pairCount).arg(a.zoneComparisonCount).arg(a.comparisonComplete?String(a.candidateCount):qsTr("未知（本次字段 %1，非完整总数）").arg(a.candidateCount)).arg(a.candidates.length).arg(yes(a.candidatesTruncated))
            .arg(a.comparisonComplete?String(a.zoneHitCount):qsTr("未知（本次字段 %1，非完整总数）").arg(a.zoneHitCount)).arg(a.zoneHits.length).arg(yes(a.zoneHitsTruncated)).arg(airspaceInputText(v.zoneInputState))
            .arg(number(v.minimumSeparationMeters)).arg(text(v.thresholdSource)).arg(a.error.length?a.error:qsTr("无计算拒绝原因；不代表没有候选或范围齐备"))
    }
    function jointMemberSummary() {
        if(!jointAvailable||!available||!integer(member.vehicleId,1,255))return ""
        var rows=jointView.memberCoverage
        for(var i=0;i<rows.length;++i)if(rows[i].vehicleId===member.vehicleId){
            var row=rows[i]
            return qsTr("所选机 %1 · 联合表示范围齐备 %2；本地编码可用 %3；执行配置可用 %4；启动几何 %5。\n已表示 %6 段：编码请求段 %7 / 启动假设段 %8；TAKEOFF 序号对应已核 %9（非飞行结果）。\n原生缺口原因：%10\n其它成员的完整缺口、来源与限制在第六类 memberCoverage 中按页查看。")
                .arg(row.vehicleId).arg(yes(row.coverageComplete)).arg(yes(row.encodingAvailable)).arg(yes(row.executionProfileAvailable))
                .arg(row.startupAvailability).arg(row.segmentCount).arg(row.encodedSegmentCount).arg(row.startupSegmentCount)
                .arg(yes(row.takeoffSequenceVerified)).arg(row.reasonCodes.length?row.reasonCodes.join("\n"):qsTr("未列出；不代表实际轨迹或净空已知"))
        }
        return qsTr("所选机的联合缺口摘要不可用，不借其他成员结果。")
    }
    function optional(v,known,key,min,max) { return v[known]===false ? v[key]===null : v[known]===true&&finite(v[key])&&v[key]>=min&&v[key]<=max }
    function pointKnown(v) {
        return exact(v,["pointIndex","coordinateSystem","latitude","longitude","altitude","altitudeReference","headingKnown","heading","holdKnown","holdSeconds","elapsedKnown","estimatedElapsedSeconds","sourceTiming"])
            &&integer(v.pointIndex,0,2047)&&v.coordinateSystem==="geodetic"&&v.altitudeReference==="amsl"
            &&finite(v.latitude)&&Math.abs(v.latitude)<=80&&finite(v.longitude)&&Math.abs(v.longitude)<=180&&finite(v.altitude)&&v.altitude>=-1000&&v.altitude<=20000
            &&optional(v,"headingKnown","heading",-Infinity,Infinity)&&optional(v,"holdKnown","holdSeconds",0,86400)&&optional(v,"elapsedKnown","estimatedElapsedSeconds",0,Infinity)&&object(v.sourceTiming)
    }
    function itemKnown(v) {
        return exact(v,["sequenceNumber","command","frame","sourcePath","sourcePointIndexKnown","sourcePointIndex","rawItem","canonicalItem","wireQuantization"])
            &&integer(v.sequenceNumber,0,2047)&&integer(v.command,0,65535)&&integer(v.frame,0,255)&&typeof v.sourcePath==="string"
            &&(v.sourcePointIndexKnown===false ? v.sourcePointIndex===null : v.sourcePointIndexKnown===true&&integer(v.sourcePointIndex,0,2047))
            &&object(v.rawItem)&&object(v.canonicalItem)&&object(v.wireQuantization)&&v.rawItem.sequenceNumber===v.sequenceNumber&&v.canonicalItem.sequenceNumber===v.sequenceNumber
    }
    function routeForMode() {
        if(!Array.isArray(member.routes))return {}
        var wanted=routeMode===1 ? "returnPath" : "waypoints"
        for(var i=0;i<member.routes.length;++i)if(member.routes[i].sourcePath===wanted)return member.routes[i]
        return {}
    }
    function sourcePath(v) {
        if(v==="waypoints")return qsTr("规划出航 waypoints")
        if(v==="returnPath")return qsTr("规划返程 returnPath（非 RTL）")
        if(v==="executionProfile.launchAction")return qsTr("起始动作配置")
        if(v==="executionProfile.endAction")return qsTr("结束动作配置")
        if(v==="executionProfile.cruiseSpeedMetersPerSecond")return qsTr("巡航速度配置")
        if(v==="executionProfile.returnSpeedMetersPerSecond")return qsTr("返程速度配置")
        return text(v)
    }
    function rowSummary(v) {
        if(routeMode===2)return itemKnown(v) ? qsTr("协议序号 %1（0 基，非航点索引） · command %2 · frame %3\n来源 %4；原路线点 %5").arg(v.sequenceNumber).arg(v.command).arg(v.frame).arg(sourcePath(v.sourcePath)).arg(v.sourcePointIndexKnown ? String(v.sourcePointIndex)+qsTr("（0 基）") : qsTr("无对应点 / 动作项，不补值")) : qsTr("该协议项结构未确认，不自行补出编码。")
        return pointKnown(v) ? qsTr("%1 · 点索引 %2（0 基）\n%3°, %4° / AMSL %5 m\n航向 %6；停留 %7；原始累计时长估计 %8（是否含停留由原来源决定，非 ETA）。").arg(sourcePath(route.sourcePath)).arg(v.pointIndex).arg(v.latitude).arg(v.longitude).arg(v.altitude)
            .arg(v.headingKnown ? String(v.heading)+"°" : qsTr("未指定，不是 0°")).arg(v.holdKnown ? String(v.holdSeconds)+" s" : qsTr("未知，不补 0 s")).arg(v.elapsedKnown ? String(v.estimatedElapsedSeconds)+" s" : qsTr("未知")) : qsTr("该点几何字段未确认，不补零。")
    }
    function scalar(v) {
        if(v===null)return "null（明确空值）"
        if(typeof v==="boolean")return v ? "true" : "false"
        if(finite(v))return String(v)
        if(typeof v==="string")return v.length ? v : qsTr("空字符串")
        return qsTr("缺失 / 不支持的值")
    }
    function fields(v,keys) {
        if(!object(v))return qsTr("未知字段对象")
        var lines=[]
        for(var i=0;i<keys.length;++i)if(Object.prototype.hasOwnProperty.call(v,keys[i]))lines.push(keys[i]+" = "+scalar(v[keys[i]]))
        return lines.length ? lines.join("\n") : qsTr("无已提供字段，不补零")
    }
    function headingEncodingText(value) {
        if (typeof value === "undefined") return qsTr("没有明确朝向编码记录；不能解释为 0°。")
        if (!exact(value, ["schemaVersion", "model", "requestedDegrees", "uploadDegrees", "tick", "circularDeltaDegrees",
                "wrappedToZero", "stepDegrees", "maximumCircularQuantizationErrorDegrees", "maximumReadbackErrorDegrees"])
                || value.schemaVersion !== 1 || value.model !== "nav-waypoint-heading-tick256-px4-float-bound-v1"
                || !finite(value.requestedDegrees) || value.requestedDegrees < 0 || value.requestedDegrees >= 360
                || !finite(value.uploadDegrees) || value.uploadDegrees < 0 || value.uploadDegrees >= 360
                || !finite(value.tick) || Math.floor(value.tick) !== value.tick || value.tick < 0 || value.tick > 92159
                || value.uploadDegrees !== value.tick / 256 || !finite(value.circularDeltaDegrees)
                || Math.abs(value.circularDeltaDegrees) > 0.001953125 || typeof value.wrappedToZero !== "boolean"
                || value.stepDegrees !== 0.00390625 || value.maximumCircularQuantizationErrorDegrees !== 0.001953125
                || value.maximumReadbackErrorDegrees !== 0.0006866455078125) return qsTr("朝向编码记录未知；不推断已接受。")
        return qsTr("逐 NAV 朝向（度）：原请求 %1 → 上传 %2；圆周量化差 %3；跨 360° 归零：%4。\n编码步长 %5；最大量化差 %6；声明回读舍入界 %7。编码精度不是实飞朝向精度，不认证固件或转向轨迹。")
            .arg(value.requestedDegrees).arg(value.uploadDegrees).arg(value.circularDeltaDegrees).arg(yes(value.wrappedToZero))
            .arg(value.stepDegrees).arg(value.maximumCircularQuantizationErrorDegrees).arg(value.maximumReadbackErrorDegrees)
    }
    function itemDetails(v) {
        if(!itemKnown(v))return qsTr("编码明细未确认。")
        var q=v.wireQuantization
        return qsTr("本地 rawItem（不是机载回读）：\n%1\n\ncanonicalItem：\n%2\n\n坐标量化差（度 / 米）：\n%3\n\n参数量化差（原参数单位）：\n%4\n%5")
            .arg(fields(v.rawItem,["sequenceNumber","command","frame","autoContinue","isCurrentItem","param1","param2","param3","param4","param5","param6","param7","yawMode"]))
            .arg(fields(v.canonicalItem,["sequenceNumber","command","frame","autoContinue","param1","param2","param3","param4","param7","latitudeE7","longitudeE7","param5Integer","param6Integer","yawMode"]))
            .arg(fields(q.coordinate,["sequenceNumber","coordinateItem","latitudeDeltaDegrees","longitudeDeltaDegrees","altitudeDeltaMeters"]))
            .arg(fields(q.floatParameters,["sequenceNumber"])+"\n"+fields(object(q.floatParameters)?q.floatParameters.deltas:null,["param1","param2","param3","param4","param7"]))
            .arg((v.command===178 ? qsTr("DO_CHANGE_SPEED 的保留零不是坐标。") : qsTr("参数解释依 command / frame；system-default 偏航不是 0°，编码不证明已上传或实飞。"))
                + "\n" + headingEncodingText(object(q.floatParameters) ? q.floatParameters.headingEncoding : undefined))
    }
    function actionText(action,label) {
        if(!actionKnown(action))return label+qsTr("：未知")
        var p=action.coordinate
        return qsTr("%1：%2；显式声明 %3；编码状态已知 %4 / 已编码 %5；协议序号 %6\n%7").arg(label).arg(text(action.type)).arg(yes(action.declared)).arg(yes(action.encodingKnown)).arg(yes(action.encoded)).arg(number(action.sequenceNumber))
            .arg(action.type==="rtl" ? qsTr("RTL 没有在此补画 HOME 航线；不是规划 returnPath 的别名。") : action.coordinateKnown ? qsTr("声明位置 %1°, %2° / AMSL %3 m；不是实际飞行位置。").arg(number(p.latitude)).arg(number(p.longitude)).arg(number(p.altitude)) : qsTr("没有已确认动作坐标；不补当前机位或 HOME。"))
    }
    function detailRoot() {
        if(detailsMode===5)return jointAvailable ? jointReview : {error:jointFailure()}
        if(!available)return {}
        if(detailsMode===0)return {launchAction:member.launchAction,endAction:member.endAction,sourceSchedule:member.sourceSchedule}
        if(detailsMode===1)return {limitations:view.limitations,unencodedRequirements:member.unencodedRequirements,reserveVehicleIds:view.reserveVehicleIds}
        if(detailsMode===2)return airspace
        if(detailsMode===3)return Array.isArray(airspace.noFlyZones) ? airspace.noFlyZones : []
        if(detailsMode===4)return {sourceScope:route.sourceScope,sourceMetadata:route.sourceMetadata,sourceTiming:selectedTiming}
        return {}
    }
    function nodeAt(node,path) {
        for(var i=0;i<path.length;++i){
            var key=path[i]
            if((!object(node)&&!Array.isArray(node))||!Object.prototype.hasOwnProperty.call(node,key))return undefined
            node=node[key]
        }
        return node
    }
    function detailEntries() {
        if(!visible||!expanded||!detailsExpanded)return []
        var list=[],start=Math.max(0,Math.min(detailsPage,detailPageCount-1))*8,end=Math.min(detailCount,start+8)
        for(var i=start;i<end;++i){
            var key=Array.isArray(detailsNode)?i:object(detailsNode)?detailKeys[i]:i
            var value=Array.isArray(detailsNode)||object(detailsNode)?detailsNode[key]:typeof detailsNode==="string"?detailsNode.slice(i*2048,(i+1)*2048):detailsNode
            list.push({key:key,value:value,nested:object(value)||Array.isArray(value)||(typeof value==="string"&&value.length>2048),label:Array.isArray(detailsNode)?"["+i+"]":object(detailsNode)?String(key):typeof detailsNode==="string"?qsTr("文本片段 %1（每段最多 2048 字符）").arg(i+1):qsTr("值")})
        }
        return list
    }
    function detailSummary(value) { return Array.isArray(value)?qsTr("数组，共 %1 项；展开后分页").arg(value.length):object(value)?qsTr("对象，共 %1 个字段；展开后分页").arg(Object.keys(value).length):typeof value==="string"&&value.length>2048?qsTr("文本，共 %1 字符；展开后分段查看，不截掉剩余内容").arg(value.length):scalar(value) }
    function resetRows() { page=0;detailsPath=[];detailsPage=0;selectedTiming=null }
    function afterLayout(item) {
        var guard=lifetime,key=identity
        Qt.callLater(function(){if(guard.alive&&root.visible&&root.expanded&&root.identity===key&&item&&item.activeFocus)root.focusVisibilityRequested(item)})
    }
    onIdentityChanged: { expanded=false;memberIndex=0;routeMode=0;detailsExpanded=false;detailsMode=0;resetRows() }
    onMemberIndexChanged: resetRows()
    onRouteModeChanged: resetRows()
    onDetailsModeChanged: {detailsPath=[];detailsPage=0}
    Component.onDestruction:lifetime.alive=false

    QGCButton {
        id:heading
        Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;Accessible.name:text
        text:root.expanded?qsTr("收起 · 原计划 / 协议项 / 联合静态报告"):qsTr("展开 · 原计划 / 协议项 / 联合静态报告（只读）")
        contentItem:QGCLabel{text:heading.text;textFormat:Text.PlainText;wrapMode:Text.WordWrap;color:heading._showHighlight?qgcPal.buttonHighlightText:qgcPal.buttonText}
        Rectangle{anchors.fill:parent;color:"transparent";border.width:heading.activeFocus?2:0;border.color:qgcPal.buttonHighlight}
        onClicked:{root.expanded=!root.expanded;root.afterLayout(this)}
        onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
    }
    ColumnLayout {
        Layout.fillWidth:true;visible:root.expanded;spacing:root.pad
        QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:qsTr("同一冻结捕获 %1\n记录 %2 / 版本 %3 · 计划 %4\n%5。完整任务几何、本地协议编码、启动名义段是不同依据，不拼接成实际航迹。").arg(root.text(root.capture.captureToken)).arg(root.text((root.capture.binding||{}).recordId)).arg(root.number((root.capture.binding||{}).recordRevision)).arg(root.text(root.expectedPlanId)).arg(root.capture.current===true?qsTr("当前绑定的历史快照"):qsTr("已过期 / 非当前的历史快照"))}
        QGCLabel{Layout.fillWidth:true;visible:!root.available;textFormat:Text.PlainText;wrapMode:Text.Wrap;color:qgcPal.warningText;text:qsTr("完整原计划视图不可用：%1。不会显示旧区域或余段计划代替；启动参考层可独立保留。").arg(root.text(root.missionReview.error))}
        ColumnLayout {
            Layout.fillWidth:true;visible:root.available;spacing:root.pad
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:root.available?qsTr("原计划成员 %1；规划点 %2 / 8192；本地协议项 %3 / 8192。整组几何完整 %4；整组编码可用 %5。\n备用成员不因显示而成为执行成员；完整 Builder 成功时规划 returnPath 已生成航点请求，但不是 RTL、上传或机载一致性证明。").arg(root.view.memberCount).arg(root.view.totalPointCount).arg(root.view.totalItemCount).arg(root.yes(root.view.geometryComplete)).arg(root.yes(root.view.encodingAvailable)):""}
            QGCComboBox {
                id:memberChoice
                Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;Accessible.name:qsTr("选择原计划成员")
                model:root.memberChoices;currentIndex:root.memberIndex
                onActivated:{root.memberIndex=index;currentIndex=Qt.binding(function(){return root.memberIndex})}
                onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
                Rectangle{anchors.fill:parent;color:"transparent";border.width:memberChoice.activeFocus?2:0;border.color:qgcPal.buttonHighlight}
            }
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:qsTr("本机几何 %1；原因 %2\n本机编码 %3；错误 %4\n载荷摘要 %5（完整性标识，不是认证）").arg(root.yes(root.member.geometryAvailable)).arg(root.member.geometryError===""?qsTr("无几何拒绝原因"):root.text(root.member.geometryError)).arg(root.yes(root.member.encodingAvailable)).arg(root.member.encodingAvailable===true?qsTr("无编码拒绝原因"):JSON.stringify(root.member.encodingError||{})).arg(root.text(root.member.payloadHash))}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:root.actionText(root.member.launchAction,qsTr("起始动作"))+"\n"+root.actionText(root.member.endAction,qsTr("结束动作"))+qsTr("\nHOLD 若为无限保持，不显示成 0 秒或任务完成。LAND 请求不证明已落地。")}
            QGCComboBox {
                id:modeChoice
                Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;Accessible.name:qsTr("选择规划路线或本地协议项")
                model:[qsTr("规划出航航点"),qsTr("规划返程航点（非 RTL）"),qsTr("本地协议项（含 TAKEOFF / HOLD / LAND / 速度）")];currentIndex:root.routeMode
                onActivated:{root.routeMode=index;currentIndex=Qt.binding(function(){return root.routeMode})}
                onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
                Rectangle{anchors.fill:parent;color:"transparent";border.width:modeChoice.activeFocus?2:0;border.color:qgcPal.buttonHighlight}
            }
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:qsTr("本类 %1 项 · 第 %2 / %3 页，每页最多 8 项。%4").arg(root.selectedRows.length).arg(root.currentPage+1).arg(root.pageCount).arg(root.routeMode===2?qsTr("协议序号不是航点索引；均从 0 开始。"):qsTr("来源范围 %1；原始累计时长估计是否含停留由原来源决定，非 ETA，也不表示时序已编码；AMSL 不是离地高。").arg(root.text(root.route.sourceScope)))}
            QGCLabel{Layout.fillWidth:true;visible:root.selectedRows.length===0;textFormat:Text.PlainText;wrapMode:Text.WordWrap;text:qsTr("此类没有可展示条目；请核对可用性与错误，不解释为任务已完成或已排除风险。")}
            Repeater {
                model:root.pageRows
                ColumnLayout {
                    id:entry
                    Layout.fillWidth:true;spacing:root.pad/2
                    readonly property var row:modelData
                    readonly property string rowKey:root.identity+":"+root.memberIndex+":"+root.routeMode+":"+root.currentPage+":"+index
                    property bool detailOpen:false
                    onRowKeyChanged:detailOpen=false
                    Rectangle{Layout.fillWidth:true;height:1;color:qgcPal.text;opacity:0.25}
                    QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:root.rowSummary(entry.row)}
                    QGCButton {
                        id:itemDetail
                        Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;Accessible.name:text
                        text:root.routeMode===2?(entry.detailOpen?qsTr("收起编码原值 / 量化差"):qsTr("展开编码原值 / 量化差")):qsTr("在下方逐字段查看该点原时序 / 路线元数据")
                        enabled:root.routeMode===2?root.itemKnown(entry.row):root.pointKnown(entry.row);opacity:enabled?1:0.45
                        contentItem:QGCLabel{text:itemDetail.text;textFormat:Text.PlainText;wrapMode:Text.WordWrap;color:itemDetail._showHighlight?qgcPal.buttonHighlightText:qgcPal.buttonText}
                        Rectangle{anchors.fill:parent;color:"transparent";border.width:itemDetail.activeFocus?2:0;border.color:qgcPal.buttonHighlight}
                        onClicked:{if(root.routeMode===2)entry.detailOpen=!entry.detailOpen;else{root.selectedTiming=entry.row.sourceTiming;root.detailsMode=4;root.detailsPath=[];root.detailsPage=0;root.detailsExpanded=true}root.afterLayout(this)}
                        onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
                    }
                    QGCLabel{Layout.fillWidth:true;visible:entry.detailOpen&&root.routeMode===2;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:visible?root.itemDetails(entry.row):""}
                }
            }
            RowLayout {
                Layout.fillWidth:true;spacing:root.pad
                QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;text:qsTr("上一页条目");enabled:root.currentPage>0;opacity:enabled?1:0.45;onClicked:{root.page=root.currentPage-1;root.afterLayout(this)}onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)}
                QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;text:qsTr("下一页条目");enabled:root.currentPage+1<root.pageCount;opacity:enabled?1:0.45;onClicked:{root.page=root.currentPage+1;root.afterLayout(this)}onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)}
            }
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:qsTr("归档规划空域：报告可展示 %1；禁飞区输入状态 %2；记录数量 %3；间距 %4 m / 来源 %5。\n原报告仍只覆盖当时规划路线，不因下方新分析扩展为启动名义段、终结下降、RTL/HOME、动态时序或地形报告。原报告实际阈值与启动名义候选来源不同，不能互换；下方是本捕获的独立联合静态分析。\n空列表 / 缺省 / 旧记录未知不表示没有禁飞区，也不表示已加载权威空域。").arg(root.yes(root.airspace.available)).arg(root.airspaceInputText(root.airspace.inputState)).arg(root.number(root.airspace.zoneCount)).arg(root.number(root.airspace.minimumSeparationMeters)).arg(root.text(root.airspace.thresholdSource))}
        }
        QGCLabel {
            Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap
            color:root.jointAvailable?qgcPal.text:qgcPal.warningText
            text:root.visible&&root.expanded?root.jointSummary():""
        }
        QGCLabel{Layout.fillWidth:true;visible:root.jointAvailable&&root.available;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:root.visible&&root.expanded?root.jointMemberSummary():""}
        ColumnLayout {
            Layout.fillWidth:true;spacing:root.pad
            QGCButton {
                id:detailToggle
                Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;Accessible.name:text
                text:root.detailsExpanded?qsTr("收起原始字段浏览"):qsTr("展开原始字段 / 本次联合静态报告（逐层分页）")
                contentItem:QGCLabel{text:detailToggle.text;textFormat:Text.PlainText;wrapMode:Text.WordWrap;color:detailToggle._showHighlight?qgcPal.buttonHighlightText:qgcPal.buttonText}
                Rectangle{anchors.fill:parent;color:"transparent";border.width:detailToggle.activeFocus?2:0;border.color:qgcPal.buttonHighlight}
                onClicked:{root.detailsExpanded=!root.detailsExpanded;root.afterLayout(this)}
                onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
            }
            ColumnLayout {
                Layout.fillWidth:true;visible:root.detailsExpanded;spacing:root.pad
                QGCComboBox{ id:detailsChoice;Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;Accessible.name:qsTr("选择原始字段类别");model:[qsTr("本机动作与原时序"),qsTr("全部限制与未编码要求"),qsTr("原规划空域报告及依据"),qsTr("归档禁飞区原值（不是认证几何）"),qsTr("所选点时序与路线元数据"),qsTr("本次联合静态报告")];currentIndex:root.detailsMode;onActivated:{root.detailsMode=index;currentIndex=Qt.binding(function(){return root.detailsMode})}onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
                    Rectangle{anchors.fill:parent;color:"transparent";border.width:detailsChoice.activeFocus?2:0;border.color:qgcPal.buttonHighlight}
                }
                QGCLabel{Layout.fillWidth:true;visible:root.detailsMode!==5&&!root.available;textFormat:Text.PlainText;wrapMode:Text.Wrap;color:qgcPal.warningText;text:qsTr("该类原计划字段当前不可展示，不从联合报告反推。可切换第六类查看本次联合结果或不可用原因。")}
                QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:qsTr("字段路径 $%1 · 第 %2 / %3 页；本层 %4 项。一次最多 8 项，不常驻展开整份报告。原字段名与值不等于已通过；未知/null 保持未知。").arg(root.detailsPath.length?" / "+root.detailsPath.join(" / "):"").arg(Math.min(root.detailsPage,root.detailPageCount-1)+1).arg(root.detailPageCount).arg(root.detailCount)}
                QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;text:qsTr("返回上一层字段");enabled:root.detailsPath.length>0;opacity:enabled?1:0.45;onClicked:{root.detailsPath=root.detailsPath.slice(0,-1);root.detailsPage=0;root.afterLayout(this)}onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)}
                Repeater {
                    model:root.detailRows
                    ColumnLayout {
                        Layout.fillWidth:true
                        readonly property var valueRow:modelData
                        QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.Wrap;text:valueRow.label+"\n"+root.detailSummary(valueRow.value)}
                        QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;visible:valueRow.nested;focusPolicy:Qt.StrongFocus;text:qsTr("查看此字段下一层");onClicked:{root.detailsPath=root.detailsPath.concat([valueRow.key]);root.detailsPage=0;detailsChoice.forceActiveFocus();root.afterLayout(detailsChoice)}onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)}
                    }
                }
                RowLayout {
                    Layout.fillWidth:true;spacing:root.pad
                    QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;text:qsTr("上一页字段");enabled:root.detailsPage>0;opacity:enabled?1:0.45;onClicked:{root.detailsPage-=1;root.afterLayout(this)}onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)}
                    QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;text:qsTr("下一页字段");enabled:root.detailsPage+1<root.detailPageCount;opacity:enabled?1:0.45;onClicked:{root.detailsPage+=1;root.afterLayout(this)}onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)}
                }
            }
        }
    }
}

import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Independent GNSS history. Preview values are labels, never append authority.
FocusScope {
    id: root
    property var workspace: null
    property string reportToken: ""
    property var reportPreview: null
    property bool sourceActive: true
    property bool expanded: false
    property bool callBusy: false
    property bool appendConfirmed: false
    property bool freezeConfirmed: false
    property bool fileConfirmed: false
    property bool appendQualified: false
    property bool freezeQualified: false
    property bool fileQualified: false
    property var lifetime: ({alive:true,ticket:null,epoch:0,consent:0,fileEpoch:0,appendEpoch:0,exhausted:false,mapLease:null})
    property var snapshots: ({session:null,file:null})
    property var browser: ({origin:"",page:null,offsets:[],report:null})
    property var pendingJob: null
    property var pendingFile: null
    property var fileDialog: null
    property var appendFacts: null
    property var saveFacts: null
    property string feedback: ""
    readonly property var state: workspace ? workspace.state || {} : ({})
    readonly property var result: workspace ? workspace.result || {} : ({})
    readonly property bool stateValid: stateKnown(state)
    readonly property string latestJobId: typeof state.jobId === "string" ? state.jobId : ""
    readonly property string memoryStamp: stateValid ? JSON.stringify([state.memoryInstanceEpoch,state.memoryRevision,state.reportCount,state.sampleCount]) : ""
    readonly property string selectedToken: workspace && typeof workspace.selectedReportToken === "string" ? workspace.selectedReportToken : ""
    readonly property bool active: lifetime.alive && !lifetime.exhausted && sourceActive && visible && enabled && expanded
    readonly property bool ready: active && !!workspace && typeof workspace.request === "function" && stateValid
    readonly property bool busy: stateValid && state.busy === true
    readonly property bool canRequest: ready && !busy && !callBusy && !fileDialog && !pendingJob
    readonly property bool canCancel: ready && busy && !callBusy && !fileDialog && !!pendingJob
        && pendingJob.owner === workspace && pendingJob.id === latestJobId && state.cancelRequested === false
    readonly property var selectedSnapshot: browser.origin === "session" ? snapshots.session : browser.origin === "file" ? snapshots.file : null
    readonly property bool selectedCurrent: snapshotCurrent(selectedSnapshot)
    readonly property bool selectedReportCurrent: selectedCurrent && !!browser.report && reportKnown(browser.report.dto)
        && JSON.stringify(browser.report.dto) === browser.report.stamp && selectedToken === browser.report.token
        && JSON.stringify(workspace.selectedReport) === browser.report.stamp
        && bindingKnown(workspace.selectedReportBinding, selectedSnapshot, browser.report.index)
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    implicitHeight: form.implicitHeight
    QGCPalette { id: pal; colorGroupEnabled: root.enabled }

    function map(v) { return v !== null && typeof v === "object" && !Array.isArray(v) }
    function keys(v, names) { if (!map(v) || Object.keys(v).length !== names.length) return false; for (var i=0;i<names.length;++i) if (!Object.prototype.hasOwnProperty.call(v,names[i])) return false; return true }
    function integer(v,a,b) { return typeof v === "number" && isFinite(v) && Math.floor(v) === v && v >= a && v <= b }
    function text(v,n) { return typeof v === "string" && v.length <= n }
    function uuid(v) { return text(v,36) && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(v) && v !== "00000000-0000-0000-0000-000000000000" }
    function hash(v) { return text(v,64) && /^[0-9a-f]{64}$/.test(v) }
    function counter(v,positive) { return text(v,20) && /^(0|[1-9][0-9]{0,19})$/.test(v) && (!positive || v !== "0") && (v.length<20 || v<="18446744073709551615") }
    function sequence(v,positive) { return counter(v,positive) && (v.length<19 || (v.length===19 && v<="9223372036854775807")) }
    function atMost(a,b) { return a.length<b.length || (a.length===b.length && a<=b) }
    function clone(v) { try { return JSON.parse(JSON.stringify(v)) } catch (e) { return null } }
    function known(v) { return v===true ? qsTr("是") : v===false ? qsTr("否") : qsTr("未知") }
    function identityKnown(v) { return keys(v,["queryId","catalogId","runId","headerHash","fileHash","afterSequence","nextSequence"])
        && text(v.queryId,128) && /^[A-Za-z0-9_-]{1,128}$/.test(v.queryId) && uuid(v.catalogId) && uuid(v.runId)
        && hash(v.headerHash) && hash(v.fileHash) && sequence(v.afterSequence,false) && sequence(v.nextSequence,false) && atMost(v.afterSequence,v.nextSequence) }
    function reportKnown(v) {
        if (!keys(v,["schemaVersion","source","ok","reason","positionValuesConfirmed","sourceIdentity","rowCount","gnssSampleCount","locatedSampleCount","unlocatedSampleCount","qualityCautionSampleCount","qualityUnknownSampleCount","withinThresholdSampleCount","groupCount","samples","displayPolicy","hasMore","footerPresent","partialTail","historicalOnly","singlePageOnly","historyComplete","rawPayloadExposed","absoluteCoordinatesExposed","authenticated","crcVerified","accuracyVerified","taskOwnershipVerified","driftDiagnosed","trajectorySafetyVerified","executionPermit","flightCommandReleased","permissionsRestored"])
                || v.schemaVersion!==1 || v.source!=="merivus-fleet-telemetry-gnss-review" || v.ok!==true || v.reason!==""
                || v.positionValuesConfirmed!==true || v.historicalOnly!==true || v.singlePageOnly!==true || !identityKnown(v.sourceIdentity)
                || !integer(v.rowCount,0,64) || !Array.isArray(v.samples) || v.samples.length>v.rowCount || v.gnssSampleCount!==v.samples.length
                || !integer(v.groupCount,0,v.samples.length) || typeof v.hasMore!=="boolean" || typeof v.footerPresent!=="boolean" || typeof v.partialTail!=="boolean"
                || !keys(v.displayPolicy,["maximumHdop","minimumSatellites","semantics"]) || v.displayPolicy.maximumHdop!==2.5 || v.displayPolicy.minimumSatellites!==6 || v.displayPolicy.semantics!=="display-heuristic-not-accuracy") return false
        var flags=["historyComplete","rawPayloadExposed","authenticated","crcVerified","accuracyVerified","taskOwnershipVerified","driftDiagnosed","trajectorySafetyVerified","executionPermit","flightCommandReleased","permissionsRestored"]
        for(var f=0;f<flags.length;++f) if(v[flags[f]]!==false)return false
        var previous=v.sourceIdentity.afterSequence, groups={}, last={}, located=0,caution=0,unknown=0,within=0
        for(var i=0;i<v.samples.length;++i) {
            var r=v.samples[i]
            if(!keys(r,["sequence","sourceEpoch","sourceSequence","groupOrdinal","vehicleId","componentId","receivedUtcMs","fixType","positionKnown","coordinate","positionReason","hdopKnown","hdop","satellitesKnown","satellitesVisible","qualityCode","hdopCaution","satelliteCaution"])
                    || !sequence(r.sequence,true) || atMost(r.sequence,previous) || !atMost(r.sequence,v.sourceIdentity.nextSequence)
                    || !uuid(r.sourceEpoch) || !counter(r.sourceSequence,true) || !integer(r.vehicleId,1,255) || !integer(r.componentId,1,255)
                    || !integer(r.groupOrdinal,1,64) || !integer(r.receivedUtcMs,1,9007199254740991) || !integer(r.fixType,0,255)
                    || typeof r.positionKnown!=="boolean" || typeof r.hdopKnown!=="boolean" || typeof r.satellitesKnown!=="boolean"
                    || typeof r.hdop!=="number" || !isFinite(r.hdop) || (r.hdopKnown ? r.hdop<0 || r.hdop>655.34 : r.hdop!==-1)
                    || (r.satellitesKnown ? !integer(r.satellitesVisible,0,254) : r.satellitesVisible!==-1)
                    || r.hdopCaution!==(r.hdopKnown && r.hdop>2.5) || r.satelliteCaution!==(r.satellitesKnown && r.satellitesVisible<6)) return false
            previous=r.sequence
            var key=JSON.stringify([r.sourceEpoch,r.vehicleId,r.componentId])
            if(groups[key]===undefined)groups[key]=Object.keys(groups).length+1
            if(groups[key]!==r.groupOrdinal || (last[key]!==undefined && atMost(r.sourceSequence,last[key])))return false
            last[key]=r.sourceSequence
            if(r.positionKnown) {
                if([3,4,5,6,8].indexOf(r.fixType)<0 || r.positionReason!=="reported-position" || !keys(r.coordinate,["latitude","longitude"])
                        || typeof r.coordinate.latitude!=="number" || !isFinite(r.coordinate.latitude) || r.coordinate.latitude < -90 || r.coordinate.latitude>90
                        || typeof r.coordinate.longitude!=="number" || !isFinite(r.coordinate.longitude) || r.coordinate.longitude < -180 || r.coordinate.longitude>180)return false
                var quality=r.hdopCaution||r.satelliteCaution ? "caution" : !r.hdopKnown||!r.satellitesKnown ? "unknown" : "within-display-thresholds"
                if(r.qualityCode!==quality)return false
                ++located; if(quality==="caution")++caution; else if(quality==="unknown")++unknown; else ++within
            } else {
                var reason=r.fixType===0?"no-gps":r.fixType===1?"no-fix":r.fixType===2?"fix-2d-not-used":r.fixType===7?"static-fix-not-used":[3,4,5,6,8].indexOf(r.fixType)>=0?"coordinates-out-of-range":"unknown-fix"
                if(!keys(r.coordinate,[]) || r.positionReason!==reason || r.qualityCode!=="unlocated")return false
            }
        }
        return v.locatedSampleCount===located && v.unlocatedSampleCount===v.samples.length-located && v.qualityCautionSampleCount===caution
            && v.qualityUnknownSampleCount===unknown && v.withinThresholdSampleCount===within && v.groupCount===Object.keys(groups).length && v.absoluteCoordinatesExposed===(located>0)
    }
    function descriptorKnown(v,origin) { return keys(v,["schemaVersion","snapshotId","origin","sourceInstanceEpoch","sourceRevision","capturedAtUtcMs","reportCount","sampleCount","locatedSampleCount","contentHash","sourceFileHash","sourceCurrent","historicalOnly","authenticated","permissionsRestored","executionPermit","flightCommandReleased"])
        && v.schemaVersion===1 && uuid(v.snapshotId) && v.origin===origin && (origin==="session" ? uuid(v.sourceInstanceEpoch) && counter(v.sourceRevision,false) && v.sourceFileHash==="" : v.sourceInstanceEpoch==="" && v.sourceRevision==="0" && v.sourceFileHash===v.contentHash && v.sourceCurrent===false)
        && integer(v.capturedAtUtcMs,1,9007199254740991) && integer(v.reportCount,0,32) && integer(v.sampleCount,0,2048) && integer(v.locatedSampleCount,0,v.sampleCount)
        && hash(v.contentHash) && typeof v.sourceCurrent==="boolean" && v.historicalOnly===true && v.authenticated===false && v.permissionsRestored===false && v.executionPermit===false && v.flightCommandReleased===false }
    function slotKnown(v,origin) { return keys(v,[]) || descriptorKnown(v,origin) }
    function stateKnown(v) { return keys(v,["schemaVersion","jobId","action","stage","busy","cancelRequested","frozen","file","selectedReportToken","memoryInstanceEpoch","memoryRevision","reportCount","sampleCount","historicalOnly","permissionsRestored","executionPermit","flightCommandReleased"])
        && v.schemaVersion===1 && (v.jobId==="" || uuid(v.jobId)) && text(v.action,32) && ["idle","preflight","running","cancelling","complete","failed","cancelled"].indexOf(v.stage)>=0
        && typeof v.busy==="boolean" && typeof v.cancelRequested==="boolean" && slotKnown(v.frozen,"session") && slotKnown(v.file,"file")
        && (v.selectedReportToken==="" || uuid(v.selectedReportToken)) && uuid(v.memoryInstanceEpoch) && counter(v.memoryRevision,false)
        && integer(v.reportCount,0,32) && integer(v.sampleCount,0,2048) && v.historicalOnly===true && v.permissionsRestored===false && v.executionPermit===false && v.flightCommandReleased===false }
    function snapshotCurrent(v) { if(!v || !ready || v.owner!==workspace)return false; var actual=v.dto.origin==="session"?state.frozen:state.file; return descriptorKnown(actual,v.dto.origin) && actual.snapshotId===v.dto.snapshotId && actual.contentHash===v.dto.contentHash }
    function bindingKnown(v,snapshot,index) { return !!snapshot && keys(v,["snapshotId","contentHash","sourceFileHash","origin","reportIndex"]) && v.snapshotId===snapshot.dto.snapshotId && v.contentHash===snapshot.dto.contentHash && v.sourceFileHash===snapshot.dto.sourceFileHash && v.origin===snapshot.dto.origin && v.reportIndex===index }
    function summaryKnown(v) { return keys(v,["reportIndex","sourceIdentity","rowCount","gnssSampleCount","locatedSampleCount","unlocatedSampleCount","qualityCautionSampleCount","qualityUnknownSampleCount","withinThresholdSampleCount","groupCount","historicalOnly"])
        && integer(v.reportIndex,0,31) && identityKnown(v.sourceIdentity) && integer(v.rowCount,0,64) && integer(v.gnssSampleCount,0,v.rowCount)
        && integer(v.locatedSampleCount,0,v.gnssSampleCount) && v.unlocatedSampleCount===v.gnssSampleCount-v.locatedSampleCount
        && integer(v.qualityCautionSampleCount,0,v.locatedSampleCount) && integer(v.qualityUnknownSampleCount,0,v.locatedSampleCount)
        && integer(v.withinThresholdSampleCount,0,v.locatedSampleCount) && v.qualityCautionSampleCount+v.qualityUnknownSampleCount+v.withinThresholdSampleCount===v.locatedSampleCount
        && integer(v.groupCount,0,v.gnssSampleCount) && v.historicalOnly===true }
    function pageKnown(v,job) { if(!keys(v,["snapshotId","afterIndex","nextIndex","totalCount","hasMore","rows"]) || v.snapshotId!==job.intent.snapshotId || v.afterIndex!==job.intent.offset || v.totalCount!==job.snapshot.dto.reportCount || !Array.isArray(v.rows) || v.rows.length>8 || v.nextIndex!==v.afterIndex+v.rows.length || !integer(v.nextIndex,v.afterIndex,v.totalCount) || v.hasMore!==(v.nextIndex<v.totalCount) || (v.afterIndex<v.totalCount && !v.rows.length))return false; for(var i=0;i<v.rows.length;++i)if(!summaryKnown(v.rows[i]) || v.rows[i].reportIndex!==v.afterIndex+i)return false;return true }
    function receiptKnown(v,action) { return keys(v,["schemaVersion","action","accepted","jobId","reason","requestOnly","historicalOnly","authenticated","permissionsRestored","executionPermit","flightCommandReleased"]) && v.schemaVersion===1 && v.action===action && typeof v.accepted==="boolean" && (v.jobId===""||uuid(v.jobId)) && text(v.reason,4096) && v.requestOnly===true && v.historicalOnly===true && v.authenticated===false && v.permissionsRestored===false && v.executionPermit===false && v.flightCommandReleased===false }
    function resultKnown(v,job) { return keys(v,["jobId","action","ok","error","reply","historicalOnly","authenticated","permissionsRestored","executionPermit","flightCommandReleased"]) && v.jobId===job.id && v.action===job.intent.action && typeof v.ok==="boolean" && text(v.error,4096) && map(v.reply) && v.historicalOnly===true && v.authenticated===false && v.permissionsRestored===false && v.executionPermit===false && v.flightCommandReleased===false }
    function saveKnown(v,job) { return keys(v,["snapshotId","contentHash","path","fileWriteAttempted","fileWrittenKnown","fileWritten","readbackVerified","readbackError","fsyncVerified","durable","authenticated"])
        && v.snapshotId===job.intent.snapshotId && v.contentHash===job.intent.contentHash && v.path===job.intent.filePath
        && typeof v.fileWriteAttempted==="boolean" && typeof v.fileWrittenKnown==="boolean" && (v.fileWritten===null || typeof v.fileWritten==="boolean")
        && (!v.fileWrittenKnown || typeof v.fileWritten==="boolean") && typeof v.readbackVerified==="boolean"
        && (!v.readbackVerified || (v.fileWrittenKnown && v.fileWritten===true)) && text(v.readbackError,4096) && v.fsyncVerified===false && v.durable===false && v.authenticated===false }
    function bump(name) { var g=lifetime;if(g[name]>=9007199254740991){g.exhausted=true;return false}g[name]+=1;return true }
    function takeTicket(existing) {
        var g=lifetime,panel=root;if(!g||!g.alive||(!existing&&g.exhausted))return null
        if(g.ticket)return existing?{ticket:g.ticket,release:function(){}}:null
        var t={jobChanges:0};g.ticket=t
        return {ticket:t,release:function(){if(g.ticket!==t)return;if(g.alive)panel.callBusy=false;if(g.ticket===t)g.ticket=null}}
    }
    function failure(message,current) { var g=lifetime;if(!g.alive || !current())return;feedback=message;if(g.alive && current() && active)feedbackLabel.forceActiveFocus() }
    function retireMap() { var g=lifetime,v=g.mapLease;if(!v)return;g.mapLease=null;v.revocation.cancelled=true;try{if(v.token)v.owner.clearMap(v.token)}catch(e){} }
    function retireAppend() { var g=lifetime,lease=takeTicket(true);if(!lease)return;try{bump("appendEpoch");appendQualified=false;if(!g.alive)return;appendConfirmed=false}finally{lease.release()} }
    function retireFreeze(){var g=lifetime,lease=takeTicket(true);if(!lease)return;try{freezeQualified=false;if(g.alive)freezeConfirmed=false}finally{lease.release()}}
    function closeDialog(view) { if(!view)return;var g=lifetime,d=view.dialogGuard;d.active=false;view.activeIntent=false;if(!g.alive||!d.alive)return;view.ownerPanel=null;if(!g.alive||!d.alive)return;if(fileDialog===view)fileDialog=null;if(!g.alive||!d.alive)return;view.close();if(g.alive&&d.alive)view.destroy() }
    function retireFile() { var g=lifetime,lease=takeTicket(true);if(!lease)return false;try{if(!bump("fileEpoch"))return false;fileQualified=false;if(!g.alive)return false;pendingFile=null;if(!g.alive)return false;fileConfirmed=false;if(!g.alive)return false;if(fileDialog)closeDialog(fileDialog);return g.alive&&!fileDialog&&!fileConfirmed}finally{lease.release()} }
    function retireLocal(sourceChanged) {
        var g=lifetime,lease=takeTicket(true);if(!lease)return
        try { bump("epoch");retireMap();if(!g.alive)return;retireAppend();if(!g.alive||!retireFile())return;freezeQualified=false;if(!g.alive)return;freezeConfirmed=false;if(!g.alive)return
            if(sourceChanged){var old=pendingJob;if(old&&old.intent.action==="save"){saveFacts={known:false,ok:false,error:"source_changed_before_receipt_adoption",jobId:old.id,intent:old.intent,reply:null};if(!g.alive||pendingJob!==old)return}pendingJob=null;if(!g.alive)return;snapshots={session:null,file:null};if(!g.alive)return;browser={origin:"",page:null,offsets:[],report:null}}
        } finally { lease.release() }
    }
    function appendIntent() { return canRequest&&!lifetime.ticket&&appendConfirmed&&appendQualified&&uuid(reportToken)&&reportKnown(reportPreview)&&typeof workspace.append==="function" ? {owner:workspace,epoch:lifetime.epoch,appendEpoch:lifetime.appendEpoch,consent:lifetime.consent,token:reportToken,stamp:JSON.stringify(reportPreview)} : null }
    function append(value) {
        var g=lifetime;if(!value||g.ticket||!canRequest)return
        function same(consent){return g.alive&&value.owner===workspace&&active&&value.epoch===g.epoch&&value.appendEpoch===g.appendEpoch&&value.token===reportToken&&value.stamp===JSON.stringify(reportPreview)&&g.consent===consent}
        if(!same(value.consent)||!appendConfirmed||!appendQualified)return
        var lease=takeTicket(false);if(!lease)return
        try { callBusy=true;if(!same(value.consent))return;var expected=value.consent+1;appendConfirmed=false;if(!same(expected)||appendConfirmed)return
            var reply=value.owner.append(value.token,true)
            if(!g.alive||g.ticket!==lease.ticket||value.owner!==workspace)return
            var valid=keys(reply,["appendAccepted","reason","memoryInstanceEpoch","memoryRevision","reportIndex","reportCount","sampleCount"])
                && typeof reply.appendAccepted==="boolean"&&text(reply.reason,4096)&&(reply.memoryInstanceEpoch===""||uuid(reply.memoryInstanceEpoch))&&counter(reply.memoryRevision,false)
                && integer(reply.reportIndex,-1,31)&&integer(reply.reportCount,0,32)&&integer(reply.sampleCount,0,2048)
            appendFacts={known:valid,receipt:valid?clone(reply):null,reportToken:value.token}
            if(!g.alive||!same(expected))return
            feedback=valid&&reply.appendAccepted ? qsTr("已确认追加本页历史GNSS记忆；未保存文件，不证明跨任务归属、物理同机、定位准确或飞行许可。") : qsTr("追加未确认：%1。可能已发生本地接纳；核对记忆计数，不自动重试。").arg(valid?reply.reason:qsTr("未知回执"))
        } catch(e){if(g.alive&&same(expected))feedback=qsTr("追加结局未知；不自动重试，不把异常当回滚。")}finally{lease.release()}
    }
    function fileContext(v) { return !!v&&ready&&v.owner===workspace&&v.epoch===lifetime.epoch&&v.fileEpoch===lifetime.fileEpoch&&v.jobId===latestJobId&&(v.action==="open"||(v.action==="save"&&v.snapshot===snapshots.session&&snapshotCurrent(v.snapshot))) }
    function pickerIntent(action) { return canRequest&&!lifetime.ticket&&(action==="open"||(action==="save"&&snapshotCurrent(snapshots.session))) ? {owner:workspace,action:action,epoch:lifetime.epoch,fileEpoch:lifetime.fileEpoch,consent:lifetime.consent,jobId:latestJobId,snapshot:action==="save"?snapshots.session:null}:null }
    function openPicker(v) {
        if(!fileContext(v)||v.consent!==lifetime.consent||!canRequest)return
        var g=lifetime,lease=takeTicket(false),view=null,d={alive:true,active:true};if(!lease)return
        try {callBusy=true;if(!g.alive||!fileContext(v))return;var expected=g.consent+(fileConfirmed?1:0),epoch=g.fileEpoch
            if(!retireFile()||!g.alive||g.fileEpoch!==epoch+1||g.consent!==expected||fileConfirmed||v.owner!==workspace||v.epoch!==g.epoch||v.jobId!==latestJobId)return
            var frozen={owner:v.owner,action:v.action,epoch:v.epoch,fileEpoch:g.fileEpoch,consent:g.consent,jobId:v.jobId,snapshot:v.snapshot}
            view=(v.action==="save"?saveDialogComponent:openDialogComponent).createObject(root,{ownerPanel:root,panelGuard:g,dialogGuard:d,frozenIntent:frozen})
            if(!g.alive)return;if(!view||!d.alive||!fileContext(frozen)||g.consent!==frozen.consent||fileConfirmed){if(view&&d.alive)closeDialog(view);return}
            fileDialog=view;if(!g.alive||!d.alive||fileDialog!==view||!fileContext(frozen)||g.consent!==frozen.consent||fileConfirmed){if(g.alive&&d.alive)closeDialog(view);return}
            if(v.action==="save")view.openForSave();else view.openForLoad()
        }catch(e){if(g.alive&&d.alive&&view)closeDialog(view);if(g.alive&&v.owner===workspace&&v.epoch===g.epoch)feedback=qsTr("文件窗口未确认打开；尚未请求文件IO，请重新选路径。")}finally{lease.release()}
    }
    function fileChosen(view,path) {
        if(!lifetime.alive||fileDialog!==view||!view.activeIntent||!view.dialogGuard.active)return
        var g=lifetime,v=view.frozenIntent,lease=takeTicket(true);if(!lease)return
        try{var valid=fileContext(v)&&v.consent===g.consent&&!fileConfirmed&&text(path,4096)&&path.length>0&&!/[\u0000-\u001f\u007f]/.test(path)
            closeDialog(view);if(!g.alive||!valid||!fileContext(v)||g.consent!==v.consent||fileConfirmed||fileDialog)return
            var staged={owner:v.owner,action:v.action,epoch:v.epoch,fileEpoch:v.fileEpoch,jobId:v.jobId,snapshot:v.snapshot,filePath:path}
            pendingFile=staged;if(!g.alive||pendingFile!==staged||!fileContext(staged)||g.consent!==v.consent||fileConfirmed)return
            feedback=qsTr("仅暂存路径，未读取或写入。请核对后另勾确认；保存明确同意覆盖，打开只产生独立文件历史。")
        }finally{lease.release()}
    }
    function fileRejected(view){if(!lifetime.alive||fileDialog!==view)return;var g=lifetime,v=view.frozenIntent,lease=takeTicket(true);if(!lease)return;try{closeDialog(view);if(g.alive&&fileContext(v)&&v.consent===g.consent)feedback=qsTr("已取消选择路径，未请求文件读写。")}finally{lease.release()}}
    function requestIntent(action,offset,row) {
        if(!canRequest||lifetime.ticket)return null
        var intent={action:action,expectedJobId:latestJobId,snapshotId:"",contentHash:"",filePath:"",reportIndex:-1,offset:0,overwriteConfirmed:false},snapshot=null,offsets=[]
        if(action==="freeze"){if(!freezeConfirmed||!freezeQualified)return null}
        else if(action==="open"||action==="save") {if(!pendingFile||!fileContext(pendingFile)||pendingFile.action!==action||!fileConfirmed||!fileQualified)return null;intent.filePath=pendingFile.filePath;if(action==="save"){snapshot=pendingFile.snapshot;intent.snapshotId=snapshot.dto.snapshotId;intent.contentHash=snapshot.dto.contentHash;intent.overwriteConfirmed=true}}
        else if(action==="clear-file"||action==="clear-frozen"){snapshot=action==="clear-file"?snapshots.file:snapshots.session;if(!snapshotCurrent(snapshot))return null;intent.snapshotId=snapshot.dto.snapshotId}
        else {if(!selectedCurrent)return null;snapshot=selectedSnapshot;intent.snapshotId=snapshot.dto.snapshotId
            if(action==="reports"){if(!integer(offset,0,snapshot.dto.reportCount))return null;intent.offset=offset
                if(browser.page&&offset===browser.page.nextIndex&&browser.page.hasMore)offsets=browser.offsets.concat([browser.page.afterIndex])
                else if(browser.offsets.length&&offset===browser.offsets[browser.offsets.length-1])offsets=browser.offsets.slice(0,-1)
                else if(offset!==0)return null
            }else if(action==="report"){if(!browser.page||browser.page.rows.indexOf(row)<0||!summaryKnown(row))return null;intent.reportIndex=row.reportIndex}else return null
        }
        return {owner:workspace,epoch:lifetime.epoch,consent:lifetime.consent,browser:browser,intent:intent,snapshot:snapshot,row:row||null,offsets:offsets,file:action==="save"||action==="open"?pendingFile:null,memoryStamp:action==="freeze"?memoryStamp:""}
    }
    function requestCurrent(v,consent,id){return !!v&&ready&&v.owner===workspace&&v.epoch===lifetime.epoch&&v.browser===browser&&lifetime.consent===consent&&latestJobId===id&&(v.intent.action!=="freeze"||v.memoryStamp===memoryStamp)&&(!v.snapshot||(v.snapshot===snapshots[v.snapshot.dto.origin]&&snapshotCurrent(v.snapshot)))}
    function submit(v) {
        if(!v||!canRequest||lifetime.ticket||!requestCurrent(v,v.consent,v.intent.expectedJobId))return
        var fresh=requestIntent(v.intent.action,v.intent.offset,v.row);if(!fresh||JSON.stringify(fresh.intent)!==JSON.stringify(v.intent)||fresh.file!==v.file)return
        var g=lifetime,lease=takeTicket(false);if(!lease)return
        try{callBusy=true;if(!g.alive||!requestCurrent(v,v.consent,v.intent.expectedJobId))return;var expected=v.consent+(appendConfirmed?1:0)+(freezeConfirmed?1:0)+(fileConfirmed?1:0)
            appendConfirmed=false;if(!g.alive)return;freezeConfirmed=false;if(!g.alive)return;fileConfirmed=false;if(!g.alive)return
            if(!requestCurrent(v,expected,v.intent.expectedJobId)||appendConfirmed||freezeConfirmed||fileConfirmed)return
            retireMap();if(!g.alive||!requestCurrent(v,expected,v.intent.expectedJobId))return
            if(v.file){pendingFile=null;if(!g.alive||!requestCurrent(v,expected,v.intent.expectedJobId))return}
            var receipt=v.owner.request(v.intent,true)
            if(!g.alive||g.ticket!==lease.ticket||v.owner!==workspace)return
            if(!receiptKnown(receipt,v.intent.action)||!receipt.accepted){failure(qsTr("请求未确认受理：%1。可能已发生本地操作，不自动重试。").arg(receipt&&typeof receipt.reason==="string"?receipt.reason:qsTr("未知回执")),function(){return g.alive&&v.epoch===g.epoch});return}
            var sync=v.intent.action==="clear-file"||v.intent.action==="clear-frozen"
            if(lease.ticket.jobChanges!==(sync?0:1)||latestJobId!==receipt.jobId||v.epoch!==g.epoch||g.consent!==expected||browser!==v.browser||!active)return
            if(sync){var origin=v.intent.action==="clear-file"?"file":"session",slot=origin==="file"?state.file:state.frozen
                if(!keys(slot,[])){failure(qsTr("原生槽未确认已清，不清后来来源。"),function(){return g.alive&&v.epoch===g.epoch});return}
                var next={session:snapshots.session,file:snapshots.file};next[origin]=null;snapshots=next;if(!g.alive)return
                if(browser.origin===origin)browser={origin:"",page:null,offsets:[],report:null}
                if(g.alive&&v.epoch===g.epoch)feedback=qsTr("已同步清准确快照槽；没有删除文件、清active记忆或抹去写入回执。")
            }else{var job={owner:v.owner,epoch:v.epoch,id:receipt.jobId,intent:v.intent,snapshot:v.snapshot,browser:v.browser,row:v.row,offsets:v.offsets};pendingJob=job;if(!g.alive||pendingJob!==job||g.epoch!==v.epoch)return;feedback=qsTr("请求已受理，等待结果与工作线程双终结；不会自动选槽、读首条或选图。")}
        }catch(e){if(g.alive&&v.owner===workspace&&v.epoch===g.epoch)feedback=qsTr("操作结局未知；不自动重试，不声称文件未写或已回滚。")}finally{lease.release();if(g.alive)settle()}
    }
    function settle() {
        var g=lifetime,job=pendingJob
        if(!g.alive||g.ticket||callBusy||!job||job.owner!==workspace||!stateValid||busy||latestJobId!==job.id)return
        if(!map(result)||result.jobId!==job.id||result.action!==job.intent.action)return
        // state/result/selectedReport/token/binding share a native notification,
        // not a QML binding evaluation order. Wait for all original values.
        if(job.intent.action==="report"&&result.ok===true&&keys(result.reply,["snapshotId","reportIndex","report"])) {
            if(selectedToken!==state.selectedReportToken||!uuid(selectedToken)
                    ||!bindingKnown(workspace.selectedReportBinding,job.snapshot,job.intent.reportIndex)
                    ||JSON.stringify(workspace.selectedReport)!==JSON.stringify(result.reply.report))return
        }
        var lease=takeTicket(false);if(!lease)return
        function same(){return g.alive&&g.ticket===lease.ticket&&pendingJob===job&&job.owner===workspace&&stateValid&&!busy&&latestJobId===job.id}
        function presentation(){return same()&&active&&g.epoch===job.epoch&&browser===job.browser}
        try{callBusy=true;if(!same())return;var value=clone(result)
            if(!resultKnown(value,job)){if(presentation())failure(qsTr("本次结果形状或归属未通过；不采用旧结果。"),presentation);return}
            if(job.intent.action==="save"){var valid=saveKnown(value.reply,job)&&(!value.ok||(value.reply.fileWrittenKnown&&value.reply.fileWritten===true));saveFacts={known:valid,ok:value.ok,error:value.error,jobId:job.id,intent:job.intent,reply:valid?value.reply:null};if(same()&&presentation())feedback=valid&&value.reply.fileWrittenKnown&&value.reply.fileWritten===true?qsTr("实际回执确认文件已写；回读与持久化保证分别列示。") : qsTr("尚未确认成功保存；实际写入事实见下方回执，不自动重试。");return}
            if(!presentation())return
            if(!value.ok){failure(qsTr("操作未成功：%1。保留历史，不自动重试。").arg(value.error),presentation);return}
            if(job.intent.action==="freeze"||job.intent.action==="open") {var origin=job.intent.action==="freeze"?"session":"file",d=value.reply.descriptor,actual=origin==="session"?state.frozen:state.file
                if(!keys(value.reply,["descriptor"])||!descriptorKnown(d,origin)||actual.snapshotId!==d.snapshotId||actual.contentHash!==d.contentHash){failure(qsTr("快照不对应原job或已被换槽，不认领后来结果。"),presentation);return}
                var next={session:snapshots.session,file:snapshots.file};next[origin]={owner:workspace,dto:d};snapshots=next;if(!presentation())return
                feedback=qsTr("已取得不可变快照。请明确选择来源槽，再读取报告目录；不自动选择首条。")
            }else if(job.intent.action==="reports"){if(!snapshotCurrent(job.snapshot)||!pageKnown(value.reply,job)){failure(qsTr("目录页不对应原快照，不采用。"),presentation);return}browser={origin:browser.origin,page:value.reply,offsets:job.offsets,report:null}}
            else if(job.intent.action==="report"){var r=value.reply.report
                if(!keys(value.reply,["snapshotId","reportIndex","report"])||value.reply.snapshotId!==job.intent.snapshotId||value.reply.reportIndex!==job.intent.reportIndex||!reportKnown(r)||!uuid(selectedToken)||!bindingKnown(workspace.selectedReportBinding,job.snapshot,job.intent.reportIndex)||JSON.stringify(workspace.selectedReport)!==JSON.stringify(r)) {failure(qsTr("报告、原选择及实际绑定不一致，不认领共享最新报告。"),presentation);return}
                var summary=job.row,names=["sourceIdentity","rowCount","gnssSampleCount","locatedSampleCount","unlocatedSampleCount","qualityCautionSampleCount","qualityUnknownSampleCount","withinThresholdSampleCount","groupCount"]
                for(var n=0;n<names.length;++n)if(JSON.stringify(summary[names[n]])!==JSON.stringify(r[names[n]])){failure(qsTr("报告内容与原目录条目不一致。"),presentation);return}
                var adopted={dto:r,stamp:JSON.stringify(r),token:selectedToken,index:job.intent.reportIndex}
                browser={origin:browser.origin,page:browser.page,offsets:browser.offsets,report:adopted}
            }
        }finally{if(same())pendingJob=null;lease.release()}
    }
    function selectSlot(origin) {
        if(!canRequest||lifetime.ticket||!snapshotCurrent(snapshots[origin]))return
        var g=lifetime,owner=workspace,snapshot=snapshots[origin],epoch=g.epoch+1
        var expected=g.consent+(appendConfirmed?1:0)+(freezeConfirmed?1:0)+(fileConfirmed?1:0),lease=takeTicket(false);if(!lease)return
        function same(){return g.alive&&g.ticket===lease.ticket&&active&&owner===workspace&&g.epoch===epoch&&snapshots[origin]===snapshot&&snapshotCurrent(snapshot)}
        try{if(!bump("epoch"))return;retireMap();if(!same()||!retireFile()||!same())return;appendConfirmed=false;if(!same())return;freezeConfirmed=false;if(!same()||g.consent!==expected||appendConfirmed||freezeConfirmed||fileConfirmed)return;browser={origin:origin,page:null,offsets:[],report:null}}finally{lease.release()}
    }
    function slotIntent(origin){return canRequest&&!lifetime.ticket&&snapshotCurrent(snapshots[origin])?{owner:workspace,epoch:lifetime.epoch,origin:origin,snapshot:snapshots[origin]}:null}
    function chooseSlot(v){if(v&&v.owner===workspace&&v.epoch===lifetime.epoch&&v.snapshot===snapshots[v.origin])selectSlot(v.origin)}
    function cancelIntent(){return canCancel&&!lifetime.ticket?{owner:workspace,epoch:lifetime.epoch,job:pendingJob,id:pendingJob.id}:null}
    function cancel(v){if(!v||!canCancel||lifetime.ticket||v.owner!==workspace||v.epoch!==lifetime.epoch||v.job!==pendingJob||v.id!==latestJobId)return;var g=lifetime,lease=takeTicket(false);if(!lease)return;try{callBusy=true;if(!g.alive||v.job!==pendingJob)return;var r=v.owner.request({action:"cancel",expectedJobId:v.id,snapshotId:"",contentHash:"",filePath:"",reportIndex:-1,offset:0,overwriteConfirmed:false},true);if(g.alive&&v.owner===workspace&&v.job===pendingJob)feedback=receiptKnown(r,"cancel")&&r.accepted&&r.jobId===v.id?qsTr("仅请求取消原job；双终结前仍忙，可能已写文件。") : qsTr("取消未确认，请核对原job。")}finally{lease.release();if(g.alive)settle()}}
    function exitPending(v){var g=lifetime,job=pendingJob;if(!v||!g.alive||g.ticket||callBusy||busy||!job||v.job!==job||v.owner!==workspace||v.epoch!==g.epoch||v.id!==latestJobId||job.owner!==workspace||job.id!==v.id)return;var lease=takeTicket(false);if(!lease)return;try{pendingJob=null;if(g.alive&&job.owner===workspace)feedback=qsTr("仅退出本页未接纳的终态回执；不撤回追加/文件操作、不重试，原生历史事实仍保留。")}finally{lease.release()}}
    function mapIntent(){return canRequest&&!lifetime.ticket&&selectedReportCurrent&&browser.report.dto.locatedSampleCount>0&&typeof workspace.selectMap==="function"?{owner:workspace,epoch:lifetime.epoch,consent:lifetime.consent,browser:browser,token:browser.report.token}:null}
    function selectMap(v){if(!v||!canRequest||lifetime.ticket)return;var g=lifetime,lease=takeTicket(false);if(!lease)return
        function same(){return g.alive&&v.owner===workspace&&active&&v.epoch===g.epoch&&v.consent===g.consent&&v.browser===browser&&selectedReportCurrent&&v.token===selectedToken}
        var held={owner:v.owner,token:"",revocation:{cancelled:false}}
        try{callBusy=true;if(!same())return;retireMap();if(!same())return;g.mapLease=held;var r=v.owner.selectMap(v.token,true,held.revocation)
            var suffix=r&&typeof r.selectionToken==="string"&&r.selectionToken.indexOf(v.token+":")===0?r.selectionToken.substring(v.token.length+1):""
            var returned=r&&r.accepted===true&&/^[1-9][0-9]{0,15}$/.test(suffix)&&(suffix.length<16||suffix<="9007199254740991")?r.selectionToken:""
            if(returned)held.token=returned
            if(!g.alive||g.ticket!==lease.ticket||g.mapLease!==held||held.revocation.cancelled||!same()){held.revocation.cancelled=true;if(returned){try{v.owner.clearMap(returned)}catch(e){}}return}
            if(!keys(r,["accepted","selectionToken","reason","readOnly","historicalOnly"])||!returned||r.reason!==""||r.readOnly!==true||r.historicalOnly!==true){held.revocation.cancelled=true;g.mapLease=null;if(returned){try{v.owner.clearMap(returned)}catch(e){}}if(g.alive&&same())feedback=qsTr("地图选择回执未知，不认领共享最新图层。");return}
            feedback=qsTr("已明确选择这份历史报告的离散地图点；不居中、不连线、不发指令。")
        }catch(e){held.revocation.cancelled=true;if(g.mapLease===held)g.mapLease=null;if(held.token){try{v.owner.clearMap(held.token)}catch(cleanup){}}if(g.alive&&same())feedback=qsTr("地图选择结局未知，不自动重试。")}finally{lease.release()}}
    function latestChanged(){var g=lifetime;if(!g.alive)return;if(g.ticket)g.ticket.jobChanges+=1;else retireFile();if(!g.alive)return;var old=pendingJob;if(old&&old.id!==latestJobId){if(old.intent.action==="save"){saveFacts={known:false,ok:false,error:"job_replaced_before_receipt_adoption",jobId:old.id,intent:old.intent,reply:null};if(!g.alive||pendingJob!==old)return}pendingJob=null}if(g.alive)settle()}
    function descriptorText(v){if(!v)return qsTr("尚无本面板实际取得的快照。");var d=v.dto;return qsTr("%1\nID %2\nhash %3\n报告%4 / 样本%5 / 定位报告点%6\n%7：%8；原文件hash %9\n仍同原记忆revision：%10（变旧仍可显式保存这些固定bytes）").arg(d.origin==="session"?qsTr("本进程冻结历史，不含后来追加"):qsTr("独立文件历史，未覆盖active记忆")).arg(d.snapshotId).arg(d.contentHash).arg(d.reportCount).arg(d.sampleCount).arg(d.locatedSampleCount).arg(d.origin==="session"?qsTr("本地冻结UTC ms"):qsTr("文档声称UTC ms（未认证）")).arg(d.capturedAtUtcMs).arg(d.sourceFileHash||qsTr("非文件源")).arg(known(d.origin==="session"&&stateValid&&state.frozen.snapshotId===d.snapshotId?state.frozen.sourceCurrent:false))}
    function summaryText(r){return qsTr("报告#%1 · 原采集run %2（非可信任务归属）\n原遥测文件hash %3\n原页(%4,%5] · GNSS%6 · 定位%7 / 未定位%8 · 来源组%9\n阈值提示%10 / 质量未知%11 / 显示阈值内%12（非精度认证）").arg(r.reportIndex).arg(r.sourceIdentity.runId).arg(r.sourceIdentity.fileHash).arg(r.sourceIdentity.afterSequence).arg(r.sourceIdentity.nextSequence).arg(r.gnssSampleCount).arg(r.locatedSampleCount).arg(r.unlocatedSampleCount).arg(r.groupCount).arg(r.qualityCautionSampleCount).arg(r.qualityUnknownSampleCount).arg(r.withinThresholdSampleCount)}
    function rowText(r){return qsTr("G%1 · UAV%2/%3 · seq%4 / source%5\nepoch %6；本地UTC ms %7；fix%8\n%9\nHDOP无单位：%10；卫星：%11；%12").arg(r.groupOrdinal).arg(r.vehicleId).arg(r.componentId).arg(r.sequence).arg(r.sourceSequence).arg(r.sourceEpoch).arg(r.receivedUtcMs).arg(r.fixType).arg(r.positionKnown?r.coordinate.latitude.toFixed(7)+", "+r.coordinate.longitude.toFixed(7):r.positionReason).arg(r.hdopKnown?String(r.hdop):qsTr("未知")).arg(r.satellitesKnown?String(r.satellitesVisible):qsTr("未知")).arg(r.qualityCode)}
    function saveText(){if(!saveFacts)return qsTr("尚无本面板实际文件回执。");var f=saveFacts,r=f.reply;return qsTr("实际文件请求 %1\n冻结ID %2\nhash %3\n路径 %4\n%5").arg(f.jobId).arg(f.intent.snapshotId).arg(f.intent.contentHash).arg(f.intent.filePath).arg(!f.known?qsTr("回执未知，文件可能已写。"):qsTr("writeAttempted %1；commit %2；fileWritten %3\nreadback %4（%5）；fsync否；durable否\n%6").arg(known(r.fileWriteAttempted)).arg(known(f.ok)).arg(r.fileWrittenKnown?known(r.fileWritten):qsTr("未知，不是未写")).arg(known(r.readbackVerified)).arg(r.readbackError||qsTr("无错误")).arg(f.error))}
    onAppendConfirmedChanged:{var g=lifetime;bump("consent");if(g.alive)appendQualified=appendConfirmed&&active&&!g.ticket}
    onFreezeConfirmedChanged:{var g=lifetime;bump("consent");if(g.alive)freezeQualified=freezeConfirmed&&active&&!g.ticket}
    onFileConfirmedChanged:{var g=lifetime;bump("consent");if(g.alive)fileQualified=fileConfirmed&&active&&!g.ticket&&!!pendingFile&&fileContext(pendingFile)}
    onReportTokenChanged:retireAppend()
    onReportPreviewChanged:retireAppend()
    onMemoryStampChanged:retireFreeze()
    onWorkspaceChanged:retireLocal(true)
    onExpandedChanged:if(!expanded)retireLocal(false)
    onSourceActiveChanged:if(!sourceActive)retireLocal(false)
    onVisibleChanged:if(!visible)retireLocal(false)
    onEnabledChanged:if(!enabled)retireLocal(false)
    onLatestJobIdChanged:latestChanged()
    onSelectedTokenChanged:{var g=lifetime;if(g&&g.alive){var v=g.mapLease;if(v&&(!browser.report||browser.report.token!==selectedToken))retireMap()}if(g&&g.alive)settle()}
    Component.onDestruction:{var g=lifetime,v=g.mapLease;g.alive=false;g.mapLease=null;if(v){v.revocation.cancelled=true;if(v.token){try{v.owner.clearMap(v.token)}catch(e){}}}if(fileDialog&&fileDialog.dialogGuard)fileDialog.dialogGuard.active=false}
    Connections{target:root.workspace;ignoreUnknownSignals:true;function onStateChanged(){root.settle()}function onResultChanged(){root.settle()}function onSelectedReportChanged(){root.settle()}function onSelectedReportBindingChanged(){root.settle()}}

    ColumnLayout {
        id:form;width:root.width;spacing:root.pad
        QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:root.expanded?qsTr("收起GNSS记忆工作区"):qsTr("GNSS记忆工作区");Accessible.name:text;onClicked:root.expanded=!root.expanded}
        ColumnLayout{Layout.fillWidth:true;visible:root.expanded;spacing:root.pad
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WordWrap;text:qsTr("独立历史区，不是维护记录或实时飞机状态。跨页/文件不自动合并同机或同任务；最多32报告/2048样本，满额拒绝，不静默淘汰。所有地图默认关闭，文件不会恢复权限。")}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;text:root.stateValid?qsTr("本进程已接纳：报告%1 / 样本%2；revision %3\njob %4 · %5 · %6").arg(root.state.reportCount).arg(root.state.sampleCount).arg(root.state.memoryRevision).arg(root.latestJobId||qsTr("无")).arg(root.state.action).arg(root.state.stage):qsTr("原生工作区不可用；不退回任意Map导入。")}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WordWrap;text:root.reportKnown(root.reportPreview)&&root.uuid(root.reportToken)?qsTr("待明确追加的当前页：GNSS%1 / 定位%2 / 未定位%3。此预览只供计数核对，native仅接原token。").arg(root.reportPreview.gnssSampleCount).arg(root.reportPreview.locatedSampleCount).arg(root.reportPreview.unlocatedSampleCount):qsTr("先在上方另行同意并生成当前页GNSS诊断；不会自动追加。")}
            Controls.CheckBox{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;text:qsTr("确认追加本页记忆");checked:root.appendConfirmed;enabled:root.canRequest&&root.uuid(root.reportToken)&&root.reportKnown(root.reportPreview);onToggled:root.appendConfirmed=checked;Accessible.name:qsTr("独立确认追加当前原生GNSS报告，不写文件")}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("追加当前页历史");enabled:root.canRequest&&root.appendConfirmed&&root.appendQualified;Accessible.name:text;property var press:null;onPressed:press=root.appendIntent();onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.append(v)}}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;text:root.appendFacts?qsTr("上次追加回执：%1\n报告token %2\n原因：%3").arg(root.appendFacts.known?root.known(root.appendFacts.receipt.appendAccepted):qsTr("未知，不等于未提交")).arg(root.appendFacts.reportToken).arg(root.appendFacts.known?root.appendFacts.receipt.reason||qsTr("无"):qsTr("回执不完整，请核对实际记忆计数")):qsTr("暂无本面板追加回执。")}
            Controls.CheckBox{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;text:qsTr("确认冻结当前记忆");checked:root.freezeConfirmed;enabled:root.canRequest;onToggled:root.freezeConfirmed=checked;Accessible.name:qsTr("独立确认冻结当前actual历史记忆，未保存文件")}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("冻结本进程历史");enabled:root.canRequest&&root.freezeConfirmed&&root.freezeQualified;Accessible.name:text;property var press:null;onPressed:press=root.requestIntent("freeze",0,null);onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.submit(v)}}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;text:root.descriptorText(root.snapshots.session)}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("选保存路径（不写入）");enabled:root.canRequest&&root.snapshotCurrent(root.snapshots.session);Accessible.name:text;property var press:null;onPressed:press=root.pickerIntent("save");onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.openPicker(v)}}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("选历史文件（不读取）");enabled:root.canRequest;Accessible.name:text;property var press:null;onPressed:press=root.pickerIntent("open");onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.openPicker(v)}}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;text:root.pendingFile?qsTr("仅暂存路径：%1\n动作：%2；冻结ID %3\nhash %4\n保存将覆盖该路径原内容；打开只读独立文件，不导入active。").arg(root.pendingFile.filePath).arg(root.pendingFile.action).arg(root.pendingFile.snapshot?root.pendingFile.snapshot.dto.snapshotId:qsTr("文件历史")).arg(root.pendingFile.snapshot?root.pendingFile.snapshot.dto.contentHash:qsTr("读后由原生返回")):qsTr("尚未选路径。文件窗口的覆盖提示不能替代下方本次确认。")}
            Controls.CheckBox{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;text:qsTr("确认本次文件操作");checked:root.fileConfirmed;enabled:root.canRequest&&!!root.pendingFile&&root.fileContext(root.pendingFile);onToggled:root.fileConfirmed=checked;Accessible.name:qsTr("另行确认读文件，或保存固定快照并同意覆盖所选路径")}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:root.pendingFile&&root.pendingFile.action==="save"?qsTr("保存并覆盖此路径"):qsTr("确认只读打开文件");enabled:root.canRequest&&root.fileConfirmed&&root.fileQualified&&!!root.pendingFile;Accessible.name:text;property var press:null;onPressed:press=root.pendingFile?root.requestIntent(root.pendingFile.action,0,null):null;onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.submit(v)}}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;text:root.saveText()}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;text:root.descriptorText(root.snapshots.file)}
            Repeater{model:[{origin:"session",label:qsTr("选择本进程冻结历史")},{origin:"file",label:qsTr("选择独立文件历史")}];delegate:QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:modelData.label;enabled:root.canRequest&&root.snapshotCurrent(root.snapshots[modelData.origin]);Accessible.name:text;property var press:null;onPressed:press=root.slotIntent(modelData.origin);onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.chooseSlot(v)}}}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WordWrap;text:root.browser.origin==="session"?qsTr("当前浏览：本进程冻结历史（不是当前飞机）。"):root.browser.origin==="file"?qsTr("当前浏览：文件历史，来源未认证，未覆盖active记忆。"):qsTr("尚未明确选择来源槽；不默认选首份快照或报告。")}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("读取报告目录首页");enabled:root.canRequest&&root.selectedCurrent;Accessible.name:text;property var press:null;onPressed:press=root.requestIntent("reports",0,null);onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.submit(v)}}
            ListView{Layout.fillWidth:true;Layout.preferredHeight:root.browser.page?Math.min(580,root.rowHeight*9):0;model:root.browser.page?root.browser.page.rows:[];clip:true;spacing:root.pad;Controls.ScrollBar.vertical:Controls.ScrollBar{}
                delegate:ColumnLayout{width:ListView.view.width;spacing:root.pad
                    QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;activeFocusOnTab:true;Accessible.role:Accessible.StaticText;Accessible.name:text;text:root.summaryText(modelData)}
                    QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("读取此报告（不选图）");enabled:root.canRequest&&root.selectedCurrent;Accessible.name:text;property var press:null;onPressed:press=root.requestIntent("report",0,modelData);onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.submit(v)}}
                }}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("目录上一页");enabled:root.canRequest&&root.selectedCurrent&&root.browser.offsets.length>0;Accessible.name:text;property var press:null;onPressed:press=root.requestIntent("reports",root.browser.offsets[root.browser.offsets.length-1],null);onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.submit(v)}}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("明确读取目录下一页");enabled:root.canRequest&&root.selectedCurrent&&!!root.browser.page&&root.browser.page.hasMore;Accessible.name:text;property var press:null;onPressed:press=root.requestIntent("reports",root.browser.page.nextIndex,null);onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.submit(v)}}
            QGCLabel{Layout.fillWidth:true;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;text:root.selectedReportCurrent?qsTr("本页实际读取报告#%1；token %2\n原文件 %3；query %4\n后续原档案页%5 / Footer%6 / 尾部不完整%7；均不证明完整历史。").arg(root.browser.report.index).arg(root.browser.report.token).arg(root.browser.report.dto.sourceIdentity.fileHash).arg(root.browser.report.dto.sourceIdentity.queryId).arg(root.known(root.browser.report.dto.hasMore)).arg(root.known(root.browser.report.dto.footerPresent)).arg(root.known(root.browser.report.dto.partialTail)):qsTr("尚无本面板实际取得且仍同绑定的报告；不认领共享最新记录。")}
            ListView{Layout.fillWidth:true;Layout.preferredHeight:root.selectedReportCurrent?Math.min(580,root.rowHeight*9):0;model:root.selectedReportCurrent?root.browser.report.dto.samples:[];clip:true;spacing:root.pad;Controls.ScrollBar.vertical:Controls.ScrollBar{}delegate:QGCLabel{width:ListView.view.width;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;activeFocusOnTab:true;Accessible.role:Accessible.StaticText;Accessible.name:text;text:root.rowText(modelData)}}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("显示此历史点图层");enabled:root.canRequest&&root.selectedReportCurrent&&root.browser.report.dto.locatedSampleCount>0;Accessible.name:qsTr("明确在地图显示此历史报告，不定位视野");property var press:null;onPressed:press=root.mapIntent();onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.selectMap(v)}}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("关闭本页原图层");enabled:!root.callBusy&&root.workspace&&root.map(root.workspace.gnssMap)&&root.workspace.gnssMap.active===true&&!!root.lifetime.mapLease&&root.workspace.gnssMap.selectionToken===root.lifetime.mapLease.token;Accessible.name:text;onClicked:root.retireMap()}
            Repeater{model:[{action:"clear-frozen",origin:"session",label:qsTr("清冻结槽（不清记忆）")},{action:"clear-file",origin:"file",label:qsTr("清文件槽（不删文件）")}];delegate:QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:modelData.label;enabled:root.canRequest&&root.snapshotCurrent(root.snapshots[modelData.origin]);Accessible.name:text;property var press:null;onPressed:press=root.requestIntent(modelData.action,0,null);onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.submit(v)}}}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("仅取消本页原job");enabled:root.canCancel;Accessible.name:text;property var press:null;onPressed:press=root.cancelIntent();onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.cancel(v)}}
            QGCButton{Layout.fillWidth:true;Layout.minimumHeight:root.rowHeight;focusPolicy:Qt.StrongFocus;primary:activeFocus;text:qsTr("退出待接纳的终态回执");visible:!!root.pendingJob&&!root.busy;enabled:root.ready&&!root.callBusy&&!root.lifetime.ticket&&!!root.pendingJob&&!root.busy;Accessible.name:qsTr("仅退出本页未接纳回执，不撤回追加或文件操作");property var press:null;onPressed:press=root.pendingJob?{owner:root.workspace,epoch:root.lifetime.epoch,job:root.pendingJob,id:root.pendingJob.id}:null;onCanceled:press=null;onClicked:{var g=root.lifetime,v=press;press=null;if(g.alive)root.exitPending(v)}}
            QGCLabel{id:feedbackLabel;Layout.fillWidth:true;activeFocusOnTab:true;Accessible.role:Accessible.StaticText;Accessible.name:text;textFormat:Text.PlainText;wrapMode:Text.WrapAnywhere;text:root.feedback||qsTr("展开不追加、不读写、不选图。请逐步明确选择来源与操作。")}
            QGCLabel{Layout.fillWidth:true;color:pal.warningText;textFormat:Text.PlainText;wrapMode:Text.WordWrap;text:qsTr("HDOP无单位，不是米。阈值内也不是安全或精度认证；文件hash只对应内容，不证明真实性、同机、任务归属或完整轨迹。历史不能恢复参数、批准、StartPermit、飞行或当前状态。")}
        }
    }
    Component{id:openDialogComponent;QGCFileDialog{id:openView;property var ownerPanel:null;property var panelGuard:null;property var dialogGuard:({alive:true,active:true});property var frozenIntent:null;property bool activeIntent:true;Component.onDestruction:dialogGuard.alive=false;selectExisting:true;selectFolder:false;folder:"";nameFilters:[qsTr("GNSS历史记忆 (*.json)")];title:qsTr("选择GNSS文件 · 返回后另确认只读打开");onAcceptedForLoad:if(panelGuard&&panelGuard.alive&&dialogGuard.alive&&dialogGuard.active&&activeIntent&&ownerPanel)ownerPanel.fileChosen(openView,file);onRejected:if(panelGuard&&panelGuard.alive&&dialogGuard.alive&&dialogGuard.active&&activeIntent&&ownerPanel)ownerPanel.fileRejected(openView)}}
    Component{id:saveDialogComponent;QGCFileDialog{id:saveView;property var ownerPanel:null;property var panelGuard:null;property var dialogGuard:({alive:true,active:true});property var frozenIntent:null;property bool activeIntent:true;Component.onDestruction:dialogGuard.alive=false;selectExisting:false;selectFolder:false;folder:"";nameFilters:[qsTr("GNSS历史记忆 (*.json)")];title:qsTr("选择固定快照保存路径 · 返回后确认覆盖");onAcceptedForSave:if(panelGuard&&panelGuard.alive&&dialogGuard.alive&&dialogGuard.active&&activeIntent&&ownerPanel)ownerPanel.fileChosen(saveView,file);onRejected:if(panelGuard&&panelGuard.alive&&dialogGuard.alive&&dialogGuard.active&&activeIntent&&ownerPanel)ownerPanel.fileRejected(saveView)}}
}

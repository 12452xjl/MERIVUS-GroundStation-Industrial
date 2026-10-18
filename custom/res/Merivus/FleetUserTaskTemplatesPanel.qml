import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Named local editing presets only. The parent planner owns the scroll area;
// this surface never submits, approves or starts a vehicle task.
ColumnLayout {
    id: root
    property var controller: null
    property bool inputSettled: false
    property bool expanded: false
    property bool callBusy: false
    property var lifetime: ({alive:true,epoch:0})
    property var form: ({id:"",version:"",name:"",category:"",description:"",path:""})
    property var consents: ({capture:false,overwrite:false,open:false,create:false,replace:false})
    property var browse: ({versions:false,page:0})
    property string feedback: ""
    readonly property var workspace: controller ? controller.userTaskTemplates : null
    readonly property var status: workspace ? workspace.state : ({})
    readonly property var workflow: workspace ? workspace.workflow : ({})
    readonly property var operation: workspace ? workspace.result : ({})
    readonly property var latestRows: workspace && Array.isArray(workspace.rows) && workspace.rows.length <= 32 ? workspace.rows : []
    readonly property var versionRows: { var revision = status.libraryRevision; return workspace && form.id ? workspace.versions(form.id) : [] }
    readonly property var browseRows: browse.versions && Array.isArray(versionRows) && versionRows.length <= 64 ? versionRows : latestRows
    readonly property int pageIndex: Math.min(browse.page, Math.max(0, Math.ceil(browseRows.length / 4) - 1))
    readonly property var pageRows: browseRows.slice(pageIndex * 4, pageIndex * 4 + 4)
    readonly property string draftToken: typeof workflow.draftToken === "string" ? workflow.draftToken : ""
    readonly property string sourceToken: typeof workflow.sourceToken === "string" ? workflow.sourceToken : ""
    readonly property bool ready: visible && expanded && workspace && lifetime.alive && !callBusy && status.busy === false
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.4)
    spacing: ScreenTools.defaultFontPixelHeight * 0.5
    signal focusVisibilityRequested(var item)
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function retire() {
        var guard = lifetime
        if (!guard.alive) return -1
        if (guard.epoch >= 9007199254740990) { guard.alive = false; return -1 }
        var epoch = ++guard.epoch
        consents = ({capture:false,overwrite:false,open:false,create:false,replace:false})
        return guard.alive && guard.epoch === epoch ? epoch : -1
    }
    function edit(key, value) {
        var guard = lifetime, original = form
        var epoch = retire()
        if (!guard.alive || epoch < 0 || guard.epoch !== epoch || form !== original) return
        var copy = {id:original.id,version:original.version,name:original.name,category:original.category,
            description:original.description,path:original.path}
        copy[key] = value
        form = copy
    }
    function choose(value, expected) {
        if (expected && !current(expected)) return
        var guard = lifetime, path = form.path, epoch = retire()
        if (!guard.alive || epoch < 0 || guard.epoch !== epoch) return
        form = value ? {id:value.templateId,version:value.version,name:value.name,category:value.category,description:value.description,path:path}
                     : {id:"",version:"",name:"",category:"",description:"",path:path}
    }
    function capture(action) {
        if (!workspace || !lifetime.alive || callBusy || !visible || !expanded) return null
        return {owner:workspace,epoch:lifetime.epoch,action:action,form:form,
            revision:status.libraryRevision,instance:status.instanceEpoch,jobId:status.jobId,
            draftToken:draftToken,sourceToken:sourceToken,consents:consents,
            hasPrevious:workflow.hasPrevious === true}
    }
    function current(intent) {
        return intent && lifetime.alive && intent.owner === workspace && intent.epoch === lifetime.epoch
            && intent.form === form && intent.revision === status.libraryRevision && intent.instance === status.instanceEpoch
            && intent.jobId === status.jobId && intent.draftToken === draftToken && intent.sourceToken === sourceToken
            && intent.consents === consents && visible && expanded && !callBusy
    }
    function allowed(intent) {
        if (!current(intent)) return false
        if (intent.action === "cancel-file") return status.busy === true
        if (!ready) return false
        if (intent.action === "capture" || intent.action === "metadata")
            return intent.consents.capture === true && form.name.trim().length > 0 && form.name.length <= 128
                && form.category.length <= 128 && form.description.length <= 4096
                && (intent.action !== "capture" || inputSettled)
                && (intent.action !== "metadata" || !!form.id)
        if (intent.action === "save") return intent.consents.overwrite === true && form.path.length > 0
        if (intent.action === "open") return intent.consents.open === true && form.path.length > 0
        if (intent.action === "inspect") return inputSettled && !!form.id && !!form.version && !!draftToken
        if (intent.action === "create") return inputSettled && workflow.sourceCurrent === true && !!intent.sourceToken
            && intent.consents.create === true && (!intent.hasPrevious || intent.consents.replace === true)
        return intent.action === "cancel-preview" && !!intent.sourceToken
    }
    function execute(intent) {
        if (!allowed(intent)) return
        var guard = lifetime, owner = intent.owner, epoch = guard.epoch, action = intent.action
        var fields = {name:intent.form.name,category:intent.form.category,description:intent.form.description}
        callBusy = true
        try {
            if (!guard.alive || owner !== workspace || epoch !== guard.epoch) return
            consents = ({capture:false,overwrite:false,open:false,create:false,replace:false})
            if (!guard.alive || owner !== workspace || epoch !== guard.epoch) return
            var result
            if (action === "capture") result = owner.capture(intent.form.id,intent.form.version,fields,intent.revision,intent.draftToken,true)
            else if (action === "metadata") result = owner.editMetadata(intent.form.id,intent.form.version,fields,intent.revision,true)
            else if (action === "save" || action === "open")
                result = owner.requestFile(action,intent.form.path,intent.jobId,intent.revision,action === "save",true)
            else if (action === "inspect") result = owner.inspect(intent.form.id,intent.form.version,intent.draftToken)
            else if (action === "create") result = owner.create(intent.sourceToken,intent.consents.replace,true)
            else if (action === "cancel-preview") result = owner.cancelPreview(intent.sourceToken)
            else result = owner.cancelFile(intent.jobId)
            if (!guard.alive || owner !== workspace) return
            if (result && result.committed === true) feedback = qsTr("本地变更已提交；若界面来源已变化，不代表新来源已通过。请按下方原生结果核对。")
            else if (result && (result.accepted === true || result.ok === true) || result === true)
                feedback = action === "inspect" ? qsTr("已取得精确版本预览；请另行确认复制为新草案。")
                    : qsTr("请求已受理。文件动作须等待真实后台结果；取消不撤销已写文件。")
            else feedback = qsTr("未确认受理：%1。请重新选择当前版本、核对输入和原生状态；不要推断先前动作未提交。")
                .arg(result && (result.reason || result.error) ? String(result.reason || result.error) : qsTr("上下文变化或回执不可用"))
        } catch (error) {
            if (guard.alive && owner === workspace) feedback = qsTr("回执未知；请核对原生结果，不自动重试或假定未提交。")
        } finally { if (guard.alive) callBusy = false }
    }
    function consent(key, value, intent) {
        if (!current(intent) || !ready) return
        var copy = {capture:consents.capture,overwrite:consents.overwrite,open:consents.open,create:consents.create,replace:consents.replace}
        copy[key] = value
        consents = copy
    }
    function operationText() {
        if (!operation.jobId) return qsTr("尚无用户模板操作；展开不读写磁盘。")
        var result = qsTr("当前槽 %1 · %2\n请求 %3\n最近已结束动作 %4\n其原请求 %5")
            .arg(String(status.action || "")).arg(String(status.stage || "")).arg(String(status.jobId || ""))
            .arg(String(operation.action)).arg(String(operation.jobId))
        var reply = operation.reply || {}
        if (operation.action === "save") {
            result += "\n" + (reply.fileWrittenKnown !== true ? qsTr("文件写入事实未知。")
                : reply.fileWritten === true ? qsTr("文件已写入。") : qsTr("未确认文件写入。"))
            result += " " + (reply.readbackVerified === true ? qsTr("本次读回一致。") : qsTr("读回未确认。"))
        } else if (operation.libraryCommitted === true) result += "\n" + qsTr("独立用户库变更已提交。")
        if (operation.error) result += "\n" + qsTr("原因：%1").arg(String(operation.error))
        return result + "\n" + qsTr("不是签名、磁盘持久性、审批或飞行结果证明。")
    }
    onWorkspaceChanged: { var guard = lifetime; retire(); if (guard.alive) browse = ({versions:false,page:0}) }
    onDraftTokenChanged: retire()
    onInputSettledChanged: retire()
    onVisibleChanged: if (!visible) retire()
    onExpandedChanged: if (!expanded) retire()
    Connections { target: root.workspace; function onStateChanged() { if (root.lifetime.alive) root.retire() } }
    Component.onDestruction: { lifetime.alive = false }

    QGCButton {
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: root.expanded ? qsTr("收起用户草案模板") : qsTr("用户草案模板：保存、版本与复制")
        wrapMode: Text.Wrap
        enabled: root.workspace !== null
        onClicked: root.expanded = !root.expanded
    }
    ColumnLayout {
        visible: root.expanded
        Layout.fillWidth: true
        spacing: root.spacing
        QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.Wrap;
            text:qsTr("用户库与内建模板目录分离。保存完整已提交编辑状态，不保存尚未提交的字段文字。复制只创建新草案，清除datum确认；必须重新审阅和审批。") }
        QGCLabel { Layout.fillWidth:true; textFormat:Text.PlainText; wrapMode:Text.WrapAnywhere;
            text:qsTr("模板 %1/32 · 总版本 %2/64 · 库修订 %3\n当前：%4 / v%5").arg(root.status.templateCount || 0)
                .arg(root.status.versionCount || 0).arg(root.status.libraryRevision || "").arg(root.form.id || qsTr("新模板")).arg(root.form.version || "—") }
        RowLayout {
            Layout.fillWidth:true
            QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; text:qsTr("新模板"); enabled:root.ready;
                onClicked:root.choose(null,null) }
            QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus;
                text:root.browse.versions ? qsTr("查看最新模板") : qsTr("查看所选模板版本"); enabled:root.ready && (!root.browse.versions ? !!root.form.id : true);
                onClicked:root.browse = ({versions:!root.browse.versions,page:0}) }
        }
        Repeater { model:root.pageRows
            delegate:QGCButton {
                id: entryButton
                Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
                enabled:root.ready
                property var pressedValue:null
                text:qsTr("%1 · v%2\n%3").arg(modelData.name).arg(modelData.version).arg(modelData.category)
                contentItem:QGCLabel { text:entryButton.text; textFormat:Text.PlainText; wrapMode:Text.WrapAnywhere;
                    color:entryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                onPressed:pressedValue={intent:root.capture("select"),row:{templateId:modelData.templateId,version:modelData.version,
                    name:modelData.name,category:modelData.category,description:modelData.description}}
                onClicked:{var guard=root.lifetime,value=pressedValue;pressedValue=null;if(guard.alive && value)root.choose(value.row,value.intent)}
                onCanceled:pressedValue=null
                onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
            }
        }
        RowLayout { Layout.fillWidth:true
            QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; text:qsTr("上一页");
                enabled:root.ready && root.pageIndex>0; onClicked:root.browse=({versions:root.browse.versions,page:root.pageIndex-1}) }
            QGCLabel { text:qsTr("%1 / %2").arg(root.pageIndex+1).arg(Math.max(1,Math.ceil(root.browseRows.length/4))) }
            QGCButton { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus; text:qsTr("下一页");
                enabled:root.ready && (root.pageIndex+1)*4<root.browseRows.length; onClicked:root.browse=({versions:root.browse.versions,page:root.pageIndex+1}) }
        }
        Repeater { model:[{key:"name",label:qsTr("模板名称（必填，最多128字）"),max:128},
            {key:"category",label:qsTr("分类（最多128字）"),max:128},{key:"description",label:qsTr("说明（最多4096字）"),max:4096},
            {key:"path",label:qsTr("用户库 JSON 的完整本地路径"),max:4096}]
            delegate:ColumnLayout { Layout.fillWidth:true
                QGCLabel { Layout.fillWidth:true; text:modelData.label; wrapMode:Text.Wrap; textFormat:Text.PlainText }
                Controls.TextField { Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; maximumLength:modelData.max;
                    text:root.form[modelData.key]; enabled:root.ready; color:qgcPal.text; selectByMouse:true;
                    Accessible.name:modelData.label; onTextEdited:root.edit(modelData.key,text)
                    background:Rectangle{color:qgcPal.windowShade;border.width:parent.activeFocus?2:1;border.color:parent.activeFocus?qgcPal.buttonHighlight:qgcPal.text}
                    onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
                }
            }
        }
        QGCLabel { Layout.fillWidth:true; visible:root.form.name.trim().length===0; text:qsTr("捕获或修改元数据前请填写模板名称。");
            wrapMode:Text.Wrap; color:qgcPal.warningText; textFormat:Text.PlainText }
        Repeater { model:[{key:"capture",label:qsTr("确认把当前完整草案捕获为新模板／新版本，或追加元数据版本")},
            {key:"overwrite",label:qsTr("确认保存此用户库，并允许替换当前路径的同名文件")},
            {key:"open",label:qsTr("确认完整重开此文件，并替换独立用户库（不改当前草案）")},
            {key:"create",label:qsTr("确认从已预览精确版本复制新草案，保留当前草案为上一稿")},
            {key:"replace",label:qsTr("确认已有上一稿将被替换；当前稿仍会保留为新的上一稿")}]
            delegate:Controls.CheckBox {
                id: consentBox
                Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
                visible:modelData.key!=="replace" || root.workflow.hasPrevious===true
                enabled:root.ready; checked:root.consents[modelData.key]
                property var pressedContext:null
                text:modelData.label
                contentItem:QGCLabel{text:consentBox.text;textFormat:Text.PlainText;wrapMode:Text.Wrap;
                    leftPadding:consentBox.indicator.width+ScreenTools.defaultFontPixelWidth;verticalAlignment:Text.AlignVCenter}
                indicator:Rectangle {
                    implicitWidth:ScreenTools.checkBoxIndicatorSize;implicitHeight:implicitWidth
                    x:consentBox.leftPadding;y:(consentBox.height-height)/2
                    color:qgcPal.windowShade;border.width:consentBox.activeFocus?2:1
                    border.color:consentBox.activeFocus?qgcPal.buttonHighlight:qgcPal.text
                    QGCColoredImage {anchors.fill:parent;anchors.margins:parent.width*0.2;source:"/qmlimages/checkbox-check.svg";
                        color:qgcPal.text;visible:consentBox.checked}
                }
                onPressed:pressedContext=root.capture("consent")
                onCanceled:pressedContext=null
                onClicked:{var guard=root.lifetime,intent=pressedContext,value=checked,key=modelData.key;pressedContext=null;
                    if(!guard.alive)return;root.consent(key,value,intent);if(guard.alive)checked=Qt.binding(function(){return root.consents[modelData.key]})}
                onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
            }
        }
        Repeater { model:[{action:"capture",label:qsTr("捕获当前完整草案为模板版本")},{action:"metadata",label:qsTr("仅追加元数据版本（不改变模板内容）")},
            {action:"save",label:qsTr("保存用户库到文件")},{action:"open",label:qsTr("重开独立用户库")},
            {action:"inspect",label:qsTr("预览所选精确版本")},{action:"create",label:qsTr("从预览复制新草案")},
            {action:"cancel-preview",label:qsTr("放弃此模板预览")},{action:"cancel-file",label:qsTr("请求取消当前文件操作（不撤销已提交文件）")}]
            delegate:QGCButton {
                id: actionButton
                Layout.fillWidth:true; Layout.minimumHeight:root.rowHeight; focusPolicy:Qt.StrongFocus
                text:modelData.label
                property var pressedIntent:null
                enabled:root.allowed(root.capture(modelData.action))
                contentItem:QGCLabel{text:actionButton.text;textFormat:Text.PlainText;wrapMode:Text.Wrap;
                    color:actionButton._showHighlight?qgcPal.buttonHighlightText:qgcPal.buttonText}
                onPressed:pressedIntent=root.capture(modelData.action)
                onClicked:{var guard=root.lifetime,intent=pressedIntent;pressedIntent=null;if(guard.alive)root.execute(intent)}
                onCanceled:pressedIntent=null
                onActiveFocusChanged:if(activeFocus)root.focusVisibilityRequested(this)
            }
        }
        QGCLabel { Layout.fillWidth:true; visible:root.status.busy===true; wrapMode:Text.Wrap; textFormat:Text.PlainText;
            text:qsTr("后台文件处理中，等待实际线程结束；不会控制或中断无人机。") }
        QGCLabel { Layout.fillWidth:true; visible:root.workflow.sourceCurrent===true; wrapMode:Text.WrapAnywhere; textFormat:Text.PlainText;
            text:qsTr("精确历史预览：%1 / v%2\n成员%3，边界点%4；不是已批准任务。").arg(root.workflow.source ? root.workflow.source.templateId : "")
                .arg(root.workflow.source ? root.workflow.source.version : "").arg(root.workflow.source ? root.workflow.source.memberCount : 0)
                .arg(root.workflow.source ? root.workflow.source.vertexCount : 0) }
        QGCLabel { Layout.fillWidth:true; visible:root.feedback.length>0; text:root.feedback; textFormat:Text.PlainText; wrapMode:Text.WrapAnywhere }
        QGCLabel { Layout.fillWidth:true; text:root.operationText(); textFormat:Text.PlainText; wrapMode:Text.WrapAnywhere }
    }
}

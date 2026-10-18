# 完整历史重规划资料包的实际入口（473，静态交接）

## 474：整组目录与明确成员选择已接实际Root

原私有Archive服务新增全包metadata；Root单槽保存value与原生全校后的exact12窄目录，public只给token／origin／包身份／binding和exact6成员行，不发布完整共同task／plan／payload。整组入口从selected-observation实际owner.remainderPlanningCollection捕获，经原scope／owner／identity尾核后发布；构造、展开、目录及分页不取源或读写文件。

v2完整保存有序成员，每页8项；未MODE提交行保不可选原因，无独立出航航点保waypointCount0但禁用于当前single编辑器。v1照旧读，begin缺失／非Bool显示未知，不补false。换包清selectedVehicleId=0，绝不默认第一行或沿用旧同ID；成员按钮冻结按下时包／ID／controller／上下文／interaction代。use另冻结memberSelectionEpoch，阻断1→2→1旧确认；busy通知前后及captureAdopted前重核，正常captureToken变化不误杀本次采用。

新公共captureFleetMissionRemainderArchiveMember(token,id,true)先核目录与明确ID，再调用原生planningSourceForVehicle全包校验后投history20；旧两参数API只保v1唯一源，v2明确要求选择。全部仍经既有单成员人工参数、完整停留、新提案／当前审阅，不是多成员联合新任务。沿用QGC主题控件、换行说明、独立默认未勾选确认、输入保留与错误焦点；未验证实际布局或可访问性。

折叠／隐藏不清后台来源，需原明确退出后才能换包／改选并用另成员；473精确关联退出保持，旧归档／Run／占用不变。覆盖路径及实际commit／读回事实、无自动重试／删除和未知副作用反馈保持。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 产品用途和非用途

原 [RemainderPanel](FleetMissionRemainderPanel.md) 只可在原 Startup／Run 所有者仍存在时取得
完整规划来源；[观察报告](FleetMissionExecutionReport.md)、brief、任务摘要和占用日志没有完整原任务政策、
计划、载荷或 sourceItemMap，不能在重启后据它们补造来源。

新增 [ArchivePanel](../../res/Merivus/FleetMissionRemainderArchivePanel.qml) 嵌入原余段入口，
配合 [Archive](FleetMissionRemainderArchive.md) 显式保存一份完整的单原 Run／单成员规划资料。
重启后可打开、核对并明确选用历史内容，人工填写新任务输入，再走原有新提案／当前审阅链。
不是自动持久化所有 Run、正式任务结果、已完成区域识别、故障自动接替或原任务续飞。

## 实际工作流

1. 原对象仍存在且目标实际 MODE 提交后，在原余段面板明确读取所选成员的原生完整来源。
   未提交目标、旧缺来源、对象已销毁或新代没有保留原来源，不能生成包；不从 latest 计划猜补。
2. 展开本资料区并明确冻结当前原生来源。Root 经实际原捕获首尾核对，调用 Archive.build；
   这里只替换一份本页资料预览，尚不写文件。原 task 是原政策副本，不包含已剥除的旧运行采集证据。
3. 明确选择保存路径，返回后显示包 ID／路径并要求单独勾选覆盖确认；选择路径不写文件。
   保存调用完整校包、原子替换及整文件读回。成功提交与之后校验／当前性分开，不自动重试或回滚。
4. 退出原编辑后可明确打开资料包；构造、属性读取、展开及重启无自动IO。
   打开只替换本页只读资料预览，不选择它作新来源，不创建或恢复 Run，也不恢复旧批准。
5. 核对包 ID、原 Run／成员、点数、原计划、捕获时间、完整包摘要及“未认证”提示。
   独立勾选仅用于新人工重规划的同意，再明确选用。Root 撤销旧本地审阅／批准，
   将已校整包转换为独立历史 schema2／exact20，不把导入包改名原生来源。
6. 成功回执的精确 captureToken 由原余段面板再次核对后采用。原输入文字保留，不自动复制旧
   起返点、目标、执行配置或“下一个”索引。人工选择包含式原航点、新目标、起返点、速度及全新 profile，
   生成新提案，再单独审阅／采用／申请批准。绝对时序仍显式拒绝，完整停留及原安全政策继续保留。
7. 清除本页预览只释放该资料槽，不删除文件、Run、占用或历史记录。正在用历史来源编辑时，
   须先退出，才能加载另一包或清槽；原生捕获也可继续使用，两个来源不能冒名混合。

收起／隐藏只退休界面确认，不自动取消后台来源；再次展开会明确提示仍有历史来源占用本地槽。
显式退出在本地编辑token已被收起清除时，仍从当前历史来源取得精确token，并在按下至点击之间
冻结控制器／epoch／token后调用原Root清理，不能误清后来出现的新来源。
本地确认退休产生的同步通知之后也再次核同一owner、预期epoch、新审阅为空及精确capture token，
不因“已经按下退出”就在回调换源后继续清理。已进入新审阅时，
退出／重新编辑仅在原review.proposalToken与同一proposal.captureToken对应时清该提案的
capture／proposal，保留退休审阅和不可变归档事实；不同新来源不被清除。

## 私有值与当前性

[Root](../../res/Merivus/GuidedActionsController.qml) 私有持有 Archive 服务和完整包。
公开 `fleetMissionRemainderArchive` 仅显示状态、原生捕获可冻结性和包的标量摘要，不暴露
sourceTask／sourcePlan／sourcePayload 或原服务对象。原余段显示仍只公开原路线／progress／binding。

新增操作：`prepareFleetMissionRemainderArchive`、`saveFleetMissionRemainderArchive`、
`loadFleetMissionRemainderArchive`、`clearFleetMissionRemainderArchive`、`captureFleetMissionRemainderArchive`。
复用原 `_fleetRemainderOperation` 独占票据、serial及最终 publication 尾核；回执新增
archiveToken／archiveId／captureToken／fileWriteAttempted／fileCommitted。
fileCommitted 在保存调用前为 null，实际 native commit 事实先保存再检查 Root 生命周期；
`committed` 也可能表示本地包预览／来源选择提交，不能仅凭该字段声称文件已写入。

历史 capture 锁实际私有槽对象、槽token、完整包UUID／hash和当前目标 Episode scope；
不要求原观察 Run 仍存在，但不把文件里的 sourceCurrent 当当前证据。
原生 capture 仍锁实际 selected owner／identity。两者随后共用原新任务审阅，旧批准、源证据、
占用、账号、运行实例或最终 StartPermit 不进入恢复路径。

只有一个资料槽和一份 capture，不按全部128历史 Run或32目标累积复制；每份来源／包仍受32MiB编码界。
历史 capture 持有当前包时不允许替换／清除槽；退出后再换。编码字节界不是实际内存占用或响应时间保证，
包构建、校验和文件操作仍为同步显式调用，不能据本次静读宣称界面不卡顿或吞吐提升。

## 确认与反馈

按下捕获实际控制器、Episode、交互／上下文epoch、原capture和包token；点击及busy通知后再核。
文件窗口为同一个冻结意图的动态 QGCFileDialog；换控制器、Episode、输入或显隐即退休，
迟到文件回调不能读写新来源。保存及选用确认默认不勾选，待写路径属于明确的同一包。

正常操作改变包／capture token只退休交互确认，不错误退休成功操作的上下文。
反馈以单份原上下文绑定状态发布，控制器／Episode／输入／显隐ABA后不能显示旧结果为当前通过。
本地票据保留到清确认、busy=false和反馈发布之后，保存／选用不能在busy通知里重复提交。
错误摘要的焦点和滚动请求分别复核原上下文。

UI/UX技能只用于沿用原QGC控件／主题、默认确认、可换行说明及键盘错误焦点；
未运行搜索脚本、QML解析、渲染、预览、对比度测量或无障碍测试。
完整未执行用例见[工程师交接473段](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 静态交接边界

该源码入口已登记到 [qrc](../../qgroundcontrol.qrc) 和 [qmldir](../../res/Merivus/qmldir)。
只做源码静读与普通文本／路径／Markdown目标／git空白核对；未编译、执行helper／业务脚本／
hash／文件读写／UI、测试、仿真、设备操作、刷机或放行。声明、注册和限定静审不是运行证据。
原25目标及六类实质开发保持，尤其完整启动／净空／正式结果和真正机载自愈不因此完成。

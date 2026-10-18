# 新部署生命周期：保留实际对象组的原生核心

## 524 当前：已闭旧组归档、精确退休和实际容量回收

新增 `archiveState`、`archiveResult` 与 `requestArchive(actualController, intent9, confirmed)`；只接受本 Lifecycle 实际 parent/同线程与完整显式确认，不解引用调用者给出的 raw controller。产品窄 facade 为 `deploymentArchive`，文件工作由 [FleetDeploymentArchiveWorker](FleetDeploymentArchiveWorker.md) 单次线程处理。不是重置当前部署、通用删除、停飞、正式任务完成或权限恢复接口。

intent 固定9键：`action/expectedJobId/lifecycleEpoch/lifecycleRevision/episodeId/filePath/archiveId/reportIndex/overwriteConfirmed`。所有动作带当前 epoch、revision、原 jobId；archive-retire 只带原 episodeId、明确路径及 overwriteConfirmed=true，open 只带路径，report 只带实际已打开 archiveId 与0–8索引，clear-file 只带原 archiveId，cancel 只带原 job上下文，其余业务字段为空字符串、-1、false。cancel/clear-file 同步返回且不创建新 job；cancel 不撤销实际磁盘提交，在已提交退休/销毁阶段拒绝。open/report 不要求 live飞机、账户、shared来源或sourceLost=false；文件永远不能 pin/恢复实际对象、观察或执行权限。

state 固定17键：`schemaVersion/lifecycleEpoch/lifecycleRevision/busy/jobId/action/stage/cancelRequested/lifetimeAllocationAttempts/liveAllocationCount/capacity/capacityRemaining/candidates/historicalOnly/permissionsRestored/executionPermit/flightCommandReleased`。stage 为 idle/running/cancelling/destroying/complete/failed/cancelled。candidate最多8，固定6键 `episodeId/published/current/selectedForObservation/retiring/sourceRetained`，只是实际保留目录，不是已闭证明。receipt固定11键，result固定15键；result 的 `retirementCommitted/episodeDestroyed/capacityReclaimed/retiredTaskCount` 各自记录实际本地事实，不用存盘成功替代退休，不用 deleteLater 或父 destroyed 替代全组销毁。

旧 `current.allocationCount` 现表示**实际活着或待销毁的槽占用**，总上限8；含初始、已发布和未发布失败组。新 `lifetimeAllocationAttempts` 为规范正 uint64 字符串，初值1，每次真实分配 mutation 增一，永不递减/回绕；不复用 Episode UUID 或原生代次。第一次完整 shared5、实际 Swarm→Orchestrator、线程和全部原有门禁通过、真正进入分配 mutation 时即 sticky pin 原五个实际对象；即使稍后候选发布失败也不换源。无效/未确认请求与文件打开不会 pin。以下旧批次“8次终身分配”“首次成功才pin”的描述只作历史，本节覆盖其容量和pin语义。

### 归档与退休边界

只允许实际 retained、非 current、非 selected 的原对象组。已发布组必须原生已 superseded；未发布组必须完整 `freshForPublication`、原14服务与Series明确 pristine，不能从缺失/空 map 或普通失败猜 idle。所有真实 Run 须原独立 Close 真正 commit；唯一额外支路是实际 unissuedPreparationClosed+retired、逐目标从未 queueBegan/queueCommitted、Txn MODE queued/uncertain均false且原实际占用为空的准备关闭。零报告因此可对应unused/pristine或没有注册TaskRecord的明确never-began Run，不造假记录、删除quiet Run或推断物理停飞。

完整 quiet18、原 Series/全部1–9实际事务、Run与protected record16按序冻结。除了既有目标投影，还在最后 native source probe 后扫描 Swarm **整张实际 owner表**，该组所有 deployment/startup/Run owner ID 在目标之外也不能仍占用。存盘线程完成 QSaveFile提交与实际readback、结果和线程finished双终结后，native再核原对象/14构件/完整受控后代/线程/parent/原record全值/quiet全值/当前选择/提示与本地调用状态；文件固定root、quiet全树及每报告record亦必须对应原快照。仅支持JSON数值与原整数的纯值对应、Invalid/Nullptr未知互等，不把Bool/String混数值，不在GUI整份JSON重编码/哈希32MiB。

全部校验成功才走 Orchestrator 私有精确批次退休，不放宽generic removeTask；保留已发布事件值和FIFO，不释放任何Swarm占用、不制造Completed。原记录移除、该组retiring标记与析构跟踪归属按原Episode/job/serial绑定；原lost连接按精确条目断开，不让有意旧组析构触发全局sourceLost或数组重排污染后组。

原Episode及其全部实际QObject后代（最多256，含Series子Txn、Run和timer）在排队销毁前捕获QPointer。QObject::destroyed只触发后续owner-bound复核；**原Episode和全部原后代QQ都空**才移除目录、减槽并完成结果。父destroyed先于子对象销毁，不能同步回收；被reparent仍活的后代保持pending，无超时强过、无自动补删或伪回收。归档退休忙期间prepare/select拒绝，已commit不能cancel。应用退出只请求线程取消，不wait/terminate或删除运行线程。

本批是限定源码静态收口，不是实际Qt构建、运行、磁盘耐久、任务完成、飞行安全或放行证明。工程师仍须验证首次分配失败后同原源回收、换源拒绝、独立Close/never-began边界、目标外占用、写成功回读失败、源变更/双终结顺序、cancel commit竞态、父先毁/后代仍活/重parent、8槽多轮回收及128protected记录容量。未编译、执行helper/解析器/业务/UI/认证/SQL/hash实现、测试、仿真、操作PX4/硬件、刷机、放行、提交、更新PR或打包。

## 371–375：原生端到端 transition 与真实产品入口接线

[Lifecycle.h](FleetMissionDeploymentLifecycle.h)/[Lifecycle.cc](FleetMissionDeploymentLifecycle.cc)新增两个供实际控制器调用的端到端接口；[Guided](../../res/Merivus/GuidedActionsController.qml)与[本地组管理面板](FleetMissionLifecyclePanel.md)本批接入新部署和保留观察选择入口。既有运行监视面板未在本批改动，沿用selected投影。这里记录源码合同，不表示目标Qt环境已经运行通过，也不把局部入口接通称为全部生命周期完成。下列366–370及更早“入口/API仍缺”的描述是当时历史，由本节覆盖。

```cpp
QVariantMap prepareNextEpisodeForController(QObject* actualController,
    const QVariantMap& expectedCurrentScope,
    const QVariantMap& actualSharedSources, bool operatorConfirmed);
QVariantMap selectObservationEpisodeForController(QObject* actualController,
    const QVariantMap& expectedSelectedScope,
    const QString& episodeId, bool operatorConfirmed);
```

两者均为 `Q_INVOKABLE`。传入controller只与工厂实际parent QQ地址比较，不解引用调用者raw指针；expected必须精确匹配原22键current或selected scope，不能混用retained-close、JSON描述或旧代次。wrapper在内部begin原current/selected同步call，私有栈 `ControllerTransitionPermit` 只允许这一次原owner/ID/purpose进入私有mutation实现和end；没有可公开设置的bypass bool。原公开legacy prepare/select仍拒普通active call，公开end不能提前解开仍持私有permit的transition。

mutation前重核原scope、prompt、实际来源及全部既有门禁；发布/选择合法改变scope后，改核私有receipt捕获的**新**scope，不再用旧scope的generic matches误拒，也不重写原call scope。原owner+ID的end独立于旧scope是否变化；`ok=true`还须native接受、后置新scope及结束原call核实。end失败不自动解锁、超时复活或清新call；对象销毁后只返回仍可确认的本地事实。

返回固定12键值DTO，不含QObject/raw scope：`ok`、`reason`、`transitionKind`、`mutationCommitted`、`newEpisodePublished`、`localObjectGroupAllocated`、`observationSelectionChanged`、`controllerCallReleased`、`currentEpisodeId`、`selectedObservationEpisodeId`、`authorityGranted`、`flightCommandReleased`。kind精确为 `prepare-next-episode` / `select-observation-episode`；reason与两ID为QString，ok/released及末两字段为Bool，末两恒false。四个mutation事实为Bool或空QVariant未知，allocated沿旧语义对应本次published；未进入transition的admission拒绝为false，失去owner且未取得commit收据则保留未知，已记真实commit不因后置/end失败改成false。选相同组可以接受：committed=true、selectionChanged=false且不递增选择代次；`controllerCallReleased`只表示核实结束本地调用，不是远端排空。

prepare保留全部retained quiet、shared5、候选fresh/QQ/parent/UUID和回调后核验；初始及失败分配尝试仍计入8次容量，不驱逐历史。构造已有initial，不自动prepare第二组；新Session保持signed-out，不自动登录、复制批准或授予权限。发布新current不自动改变selected；观察选择也不重启/停止Run，不释放原任务或Swarm占用。scope和操作员确认不是身份或飞行许可，真实业务仍走原full gate。

本批只作源码及限定人工/文本审阅，native范围已由独立只读交叉审阅，未发现确定新增静态阻断；这不是编译、测试、运行或安全通过。未运行Qt/UI、认证、API/helper、应用或设备，未刷机、更新PR或打包。`outboundIntegrationComplete=false`保留；正式执行结果、权限/容量留存、跨站与长期运营等仍有实质源码及工程验证缺口，原25目标/六类不缩减。交接必须用累计完整工程，不把本批增量当独立安装包。

## 历史366–370：QML工厂接线与同步调用原生占用

5源码仅为累计工程增量，需完整源码/pri/注册/原生依赖一并交接，不是独立构建包。QML22键scope exact Int/QObjectStar往返、方法绑定、Root parent和销毁均尚未运行验证，不放宽合同。最新private桥在nested draft后只撤原scope意图，无法核实则 `retiredCurrentScope` 持续拒同组current入口/回调，不超时复活，selected仍可读；此机制不改变本原生scope的纯引用含义。

[CustomPlugin](../CustomPlugin.cc)现注册 `MerivusInternal 1.0` 的可创建 Lifecycle 与不可创建 Episode；[Guided](../../res/Merivus/GuidedActionsController.qml)把工厂作为 Root 的直接 QObject 子对象，用私有14引用接现有工作流。构造函数已经分配/发布 initial，不能再自动 `prepareNextEpisode(null, "", ...)`。旧实际对象、Run、占用、8次分配额度（含初始及失败尝试）、原18键静默证据和22键引用 scope 均保留。

新增三个 native `Q_INVOKABLE`：`beginControllerCall(actualController, purpose, actualCloseWorkflow)`、`controllerCallMatches(actualController, callId)`、`endControllerCall(actualController, callId)`。purpose 只允许 `current`、`selected-observation`、`retained-close`；前两 Close 必须 null，后者仅按原 retained QQ 地址找实际 Close。begin 要求原实际 parent/线程、无既有 call/切换/源失效/销毁/认证提示，callId 由 factory epoch 与只增原生计数生成，耗尽拒绝不 wrap；成功固定六键 `ok/callId/purpose/scope/authorityGranted/flightCommandReleased`，后两恒 false。

matches 只核原 owner QQ、当前 parent、ID及捕获 scope，不查 prompt/quiet/fullgate；scope 查询也不因 call 忙自拒。end 仅按原 owner+ID清本槽，即使 parent/source/scope 已变化也可收尾，但提示栈未退不能提前 end；无超时、owner死亡自动解锁或清除新 call。prepare/select 在入口、回调后及最终 mutation 拒 active call，`current.controllerCallBusy` 为本地栈诊断、`busy` 为其与 switching 的 OR，不证明远端状态。

Guided plain lifetime/finally 原槽清理、nested draft stale、已 queue/未知不回滚，以及 selected 四值同 scope/选择代次 token/原 issued 收据路由已写入源码。面向用户的新部署/选择入口及与本占用兼容的专用 transition 原生 API 仍未开发，`outboundIntegrationComplete=false`，不称多部署产品完成；25目标/六类继续。实际5源码/18文档，已完成限定文本审阅，不是运行或安全通过；未编译、测试或运行。以下为历史记录，旧未注册/未迁移状态由本节覆盖。

## 历史361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 历史356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 历史351–355：普通部署的底层逐包发送范围

[原生传输契约](FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

## 历史346–350：十二类服务已接，尚缺core每包与控制器闭环

除345的Session身份侧，本轮在另外12个既有cc接actual current Episode及owned同组：Deployment/EvidenceRefresh/ContentCheck/ArmedGroundReview，OperationApproval/StartReview/StartApproval/StartupAuthority，GroundApproval/GroundAuthority/Txn/Series。请求前、live上下文与原same/unchanged、OS/Direct owner返回、审批消费/移交前后及已有scoped发送fence重核；双legacy保留原行为，managed混配/跨组/owner丢失/reparent拒绝。

StartupAuthority和Txn非ground认证移交回调去掉full _gate，保留pure preparing/busy/actual QQ/UUID/epoch/代次/原grant或context/hash/未持proof或reservation及原剩余期限比对；request与移交外层继续前后完整OS/风险检查。Series初始化是一次原生bootstrap，actual Episode槽/owner/parent/pristine状态匹配，不机械要求尚未发布组current；新child先继承private provenance并登记后才通过current pair。普通current/历史/previousStartup/consumerIsCurrent、旧Close/Run、精确cancel/revoke/drain与真实queue记账不成为新发送授权，也不据此删除。

这些仍不等于新部署闭环。普通Deployment使用legacy PlanManager上传/回读，item adoption/operationStarted/插件后的最终queue与异步每包/重试还没有Episode scoped fence；现有caller boundary不冒充覆盖。Guided actual Episode/epoch/来源回调、共享Role身份上下文、全部retained modal忙锁与selected旧Run/Series/Close/Session一组投影仍待接入。新核心未注册QML、无新部署入口，outboundIntegrationComplete=false保持；不编译、测试或运行认证/应用/设备，原25目标/六类不缩减。

## 历史340–344 原生核心与345身份接线：尚未接入控制器

新增原生`FleetMissionDeploymentEpisode.h/.cc`和`FleetMissionDeploymentLifecycle.h/.cc`，拆开“同一部署的地面再审轮次”和“下一次独立部署”。这不是把旧Txn generation清零，也不是覆盖旧Deployment的current/targets。340–344实现实际对象所有权、静默切代证据和有界工厂，345接Session非Close身份门禁；**没有QML类型注册、没有替换Guided中的现有对象，没有新部署按钮，也没有上线此切换路径**。当前应用仍沿用原固定链，不可据此称后续新部署已能实际操作。

已完成的335–339监视面板、330–334关闭审批及既有事务行为不因这个未接入模块而改变。原25目标/六类继续；本核心不是完整新部署、长任务或容量管理的验收结论。

## 每个实际 Episode 的组成

每组拥有14个全新实际QObject：Deployment、OperatorSession、OperationApproval、EvidenceRefresh、StartContentCheck、StartReview、StartApproval、StartupAuthority、初始StartupTransaction、ArmedGroundReview、GroundStartApproval、GroundStartupAuthority、StartupSeries、ObservationClose。Series只一次绑定本组初始Txn，保留最多8个原同部署ground child的既有约束。

新Session初始为signed-out，不复制旧主身份、审批、认证proof、原剩余期限或令牌。旧组连同原Session/Txn/Run/Deployment/QQ与历史原样保留，不销毁/reset/取消或释放旧占用来腾位置。Swarm、同一Orchestrator、Role、PlanStore和Health是共享实际来源，工厂固定五个typed同线程QObject，要求Swarm实际backend就是该Orchestrator；首次成功后不替换。

## 原生切代与保留语义

以下 `prepareNextEpisode` 为保留的legacy native接口说明；当前控制器产品入口使用371–375的ForController wrapper，不在普通call里end后再调用此接口绕开原同步占用。

`prepareNextEpisode(expectedActualEpisode, expectedEpisodeId, actualSharedSources, operatorConfirmed)`只分配新本地对象组，不上传/读取/审批/解锁/切模式，也不授予StartPermit。expected必须是本工厂当前实际对象和规范UUID；五来源恰为swarm/orchestrator/roles/store/health的实际QObject，不接受ID、导入描述或相同系统ID的替代来源。

申请先锁住工厂，并从**全部保留Episode**直接检查原生操作栈/所有者/实际map，不能用可能滞后的public current.busy作门禁；特别不能调用Session.current/身份查询或QML/OS来制造切代证据。原生 `switchEvidence` 拒绝/未知返回空map，绝不解释为空占用。固定18键快照包含本Episode/各构件与Series/真实Txn/Run、原binding、实际占用及撤销状态，scope固定为native-quiescent-outbound-not-flight-or-run-retirement；不带SID、receipt、raw或QObject值。

旧启动/发送者必须终态且真实本机协议/回调栈静默；旧Run可以继续观察，原占用与unknown尾部保留，不能为了新建其它不重叠任务而自动退休它。新任务仍须原Swarm预约与Orch assignmentConflicts，不能占用其它运行成员。Series或14个owned来源丢失是不可重建证据，不能fallback；外部遥测丢失不等于Run/占用自动消失，也不能假造零busy或用同ID新对象核实旧状态。

原Vehicle/MM/PM/Links/primary QQ丢失不再一律阻止分配新的空本地组：只检查原捕获对象，存活的Vehicle核真实ID/线程/接收栈，存活MM独立核transportCallInProgress/inProgress，存活PM核pendingWrites；存活Vehicle与MM/PM/Links须仍是原实际归属。原Links与primary都活时只比较原Links当前primary，不获取同ID替代；primary角色变化仅使连续性未知。任何存活原对象busy仍拒，owned/内部owner/共享源丢失仍拒。

固定top18不变；componentStates行固定name/state/generation/targetEvidence，9个实际捕获目标组件使用有界列表，其余为null。Txn行增加自己的targetEvidence，初始Txn不在组件行重复；每target固定vehicleId/sourceRetained/localTransport/sourceContinuityUnknown，sourceRetained恰5源，localTransport恰9键、scope固定local-original-object-stacks-only。缺失或从未捕获的相应检查为JSON null，不写成inProgress=false；false unknown只表示未观察到该缺失/primary不匹配，绝非远端或物理连续性认证。Run逐原QQ对齐并保留未知尾部。每快照上限512KiB，与Factory的depth12/32768节点/512KiB预算一致；超界拒绝，不截断目标或历史来制造通过。

工厂冻结每个实际14对象QQ/父级、完整原生快照及共享五来源，再构造未发布候选。候选自身也必须fresh/quiet且从未业务使用，规范UUID不与任何历史ID相同；构造与setParent/ChildAdded窗口后重新核实际父级、记录ID、14QQ和快照。未完成候选不替换current。最终无回调发布只置旧组sticky superseded、新组published并切单个current指针，不迁移批准或触碰旧map；公开通知排队。

容量恰8次对象组分配，初始组占1，失败构造/发布尝试也消耗额度；不会自动重试、驱逐历史、删除closed任务、归还旧Orch容量或重建Orch绕过其128条native/10000总任务上限。保留的未发布候选不成为可执行组。任何mandatory owned来源死亡保持sticky sourceLost，不回退到较早组。

## 被动观察和新请求必须分开

工厂另有显式selectedObservationEpisode。发布新current不会丢掉旧观察选择；选择只选实际保留且曾发布的Episode，未知UUID拒绝，不把旧Episode重新设为outbound current。current/selected getter、isCurrent、Close实际查找及显式选择共用纯native保留记录检查：本工厂parent、actual QQ、record ID与实际UUID一致、published且无lost、14对象及其private provenance仍匹配；不调用OS/身份/public current或switchEvidence来制造可用状态。公开episodes中的sourceRetained/current/selectedForObservation只是历史指向；actualSourceUsable才表示该原生检查当前可用，不能从历史标记推导操作权。按实际Close QObject寻找其保留owner只是观察关闭的来源定位，不授予历史组上传、ARM或MODE权限。

Episode的native outbound门禁字段一经superseded不可恢复；14个真实owned类私有保存sticky managed标记和原owner QPointer，Series新child公开前也继承；owner丢失、reparent或未知类型不回退legacy，无可写动态property授权。真实旧Run与ObservationClose不属于新发送许可。345及346–350已接身份与12类服务边界，351–355另补普通Deployment core逐包/重试/插件后发送范围。历史阶段所述“QML作用域/类型注册/实际控制器未接”已由366–375后续源码覆盖，但不能仅靠入口存在推导全部隔离或运行可靠性；当前仍明确outboundIntegrationComplete=false。

345已把[Session非Close用途](FleetOperatorSession.md)接到current原生门禁及实际owner配对：普通身份事实/显式登录/Close专用用途不调用这一outbound helper，非Close审批与凭据消费/移交/Current证明才调用。两对象仅可同一实际managed组或双方真正legacy；unknown/跨组拒绝。346–350推进其余12类服务，351–355补core普通传输范围，366–375继续QML/身份投影和transition入口；不能从身份、服务或入口单点推导全部隔离完成。

## 历史接线清单（340–365，后续源码进度见最新节）

下列为当时的剩余接线要求，其中controller引用、来源回调、selected投影、类型注册和transition产品入口已在366–375接入源码，不再当作本轮仍未开发。其行为尚未运行验证，正式StartPermit/净空/结果、长期权限/容量和跨站生命周期缺口仍保留。

1. 351–355已写普通Deployment专用core scoped上传/回读：实际Vehicle/MM/Link/操作身份冻结，覆盖异步每包/重试及插件后最终queue，完整回调与pure检查分开。旧观察/精确取消/revoke/drain与真实已queue事实单独保留；只完成该源码边界，不代替后续完整应用接线或工程师验证。
2. Guided14个私有对象改为actual current Episode引用，不能有旧单例fallback。所有OS/原生调用前冻结Episode/epoch/实际服务，返回及finally仅改原作用域，不用新别名取消/写回replacement。
3. 356–360已补20处Direct回调的实际native调用者首参并同步Guided19签名，不再只凭binding/角色来源map识别producer。后续仍须冻结current Episode/epoch/原服务和callback上下文；QML Connections必须绑定真正sender组，旧Session后台changed不能覆盖共享Role当前actor，不能据本次参数接线推导完整应用隔离。
4. 明确current任务身份操作与selected旧观察身份操作；旧Episode Session要可显式认证，不能拿新Session冒充，不能从后台通知猜身份意图。361–365已补所有Session认证入口的全进程GUI调用栈锁，切代/选择实时核该锁；外层控制器调用scope/finally与其它界面模态协调仍须接线。
5. selected Episode的Run、Series、Close和Session须一组投影；Panel要用专门observationSeries/episode token，而原startupSeries仍代表current任务。切选择时仅取消旧页实际Close/review，不把旧reviewId交给新服务；历史组不得复活旧飞行发送权。
6. native类型/工程资源与人工新部署/保留观察选择UI最后一起接线，随后再交工程师。最终StartPermit/净空/远端正式结果、授权留存/容量清理和长任务完整生命周期仍另有实质代码工作，不能由本地monitor关闭推导。

## 验证边界

只读源码、编辑与限定文本审阅；没有编译/moc/解析器/测试，未运行构造器、工厂、认证、应用、helper/业务、SQL/日志/仿真或设备/刷机，未更新PR或打包。实际构造/父级事件、生命周期重入、Qt方法/精确类型往返、Windows身份及多任务隔离仍待工程师验证，源码接线不替代这些验证。

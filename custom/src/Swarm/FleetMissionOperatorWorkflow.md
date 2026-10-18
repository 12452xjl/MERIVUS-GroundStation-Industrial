# 操作员地面上传与独立风险审阅

## 514：备用机独立出勤接入唯一共享部署来源

规划页在联合余段区之后嵌入 [FleetReserveDispatchPanel](FleetReserveDispatchPanel.md)，以真实当前完整备用方案创建独立单机新任务。子区的 pendingInput 加入全局未完成输入；每个子编辑器的 inputSettled 排除自己的 pending，但包含其它子区，避免自锁或同时采用。默认展开、勾选或显示候选都不发命令、不失效原来源；明确捕获由 Root 先冻结再退休旧审阅。

唯一共享部署源显式区分 draft／remainder／reserve-dispatch。采用的未知 kind 标 unknown 阻断，不降级为草稿；备用出勤使用同一新 review token、archive、实际 plan、encoding、current 与 approvalAllowed，不另建第二上传区。区域草稿审批在独立来源被采用期间保持禁用，文字指向对应子区完成本地申请批准／批准或明确退出。

新任务的采用不等于本地批准、独立账号上传审批、上传、旧任务接管或飞行许可。旧 single 区不展示、采用、批准或清理 group／reserve-dispatch 记录。正常当前能力、电池、风险、完整归档绑定和原生上传门禁不放宽，最后 StartPermit 仍待完整开发。

仅地面站源码与静态对读；没有编译、运行 QML／应用、测试、仿真、硬件／PX4操作、刷机或放行。UI/UX 技能用于既有主题、明确反馈、可见标签和焦点的源码核对；布局、交互、朗读及资源实际加载仍由工程师验证。下方为历史。

## 418：先审阅能力声明，再重新生成任务依据

指挥中心和地图规划页新增[单机工程采纳/撤销面板](FleetCapabilityReviewPanel.md)。输入明确本地机号和完整能力/理由，取得原生冻结构型与批准记录，完成两项确认后再由不同账号审批。撤销是独立操作，不重置物理配置或基线；审批可使旧任务审阅依据失效，需沿原任务流程重新生成。

当前批准仅是工程人员对当前本地描述的采纳，速度/续航仍是声明值，不能自动判断未上报改装。提交、当前有效性与日志追加结果分开，不按失败自动重试。完整执行/自愈/跨站/长期运营和真实模型仍继续；没有编译、认证/UI运行、测试或设备操作。

## 417：固定工程目录也有显式操作入口

指挥中心与 Planner 新增同一[固件工程目录面板](FleetFirmwareCatalogPanel.md)。操作为登录实际维护／管理员账号、预览固定文件、核完整原新声明及整批替换风险、双项确认、不同账号审批。Root 私有实际来源、当前 scope、原显示身份／政策与 Catalog 身份绑定；来源变化只取消原收据，不能用另一个入口或新的会话继续旧确认。

运行中重载只改变内存工程目录，文件由工程人员另行准备；旧自动启动加载未改为统一审批。目录变化使旧任务审阅失效，必须按原工作流重新获取依据，不能绕过固件匹配、参数绑定或完整 StartPermit。已提交替换、外层返回状态和本地审计追加分别报告，不把错误当回滚。未编译、运行认证/UI或测试，仍有完整执行、可信能力、自愈、跨站、长期运营和真实模型/生态等实质工作。

## 416：先保存本机完整配置，再重新生成真实审阅

Planner 当前成员下新增[逐机编辑器](../../res/Merivus/FleetExecutionProfileEditor.qml)。核心动作、WGS84/AMSL 坐标、执行速度和时序结构化输入，完整 startupTransitionPolicy 则用高级 JSON schema1–3；不是所有约束的表单。不会自动用规划速度、Home 或另一成员配置作默认值。政策可留空保存意图，但后续受控启动继续拒绝；输入范围不是飞行安全推荐值。

操作顺序为：选择实际成员并开始编辑 → 填完整意图 → 只读该机当前12参数 → 核对值及机号、明确确认 → 保存前再次读取相同来源指纹/hash → [Draft 完整 revision CAS](FleetMissionDraft.md) → 重新预览、归档并走原独立审阅/审批。读取失败或过期时只显示有界原因，不自动请求/refresh或写参数，不沿用旧 profile 里的 hash。`requiredProfileHash` 与 `observationFingerprint` 都必须匹配当前新读依据，任意 caller 字符串不被当作来源证明。

保存桥先结束只读 `runCurrent` 观察调用，再以当前准确 revision 做本地 Draft CAS；不是在仍打开的只读调用里提交草稿、再被它自己的 changed 通知误拒。旧 review/approval 仍照常失效，不因避免自拒而放宽。清除 profile 另走两步确认及原revision；不改机载参数。高级 JSON 是 trim 前最多16384个 UTF-16代码单元，不是16 KiB。隐藏清除确认和参数依据并换 edit-session token，换草稿或成员取消编辑，旧同步返回不能恢复失效会话的确认。

编辑开始、字段变化及保存/清除尝试通过原草稿 edit-generation 使旧 review 失效。retained 成员角色改变保留本机 profile，reserve 不作为活动 assignment 输出；删除再加同号不继承。准确 revision 不符拒绝覆盖。native Draft 在通知前已提交内容/revision，因此通知重入/销毁后的 false 可能已有本地提交；必须读回状态，不能自动重试或声称未发生。

新 profile 走 Compiler 已有合法路径，保存/读取成功不触发上传、ARM、模式切换或任务完成。完整 StartPermit、可信原点/净空、固件工程目录操作入口、机载正式任务/结果与25目标/六类仍未完成。UI 技能仅影响标签、明确确认、可见错误与原QGC palette；未编译、测试、运行 helper/UI/认证/应用/设备或改 PX4、刷机、更新 PR、打包。下方旧批次为历史，旧“草稿不带 profile”不覆盖本节。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../../../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../../../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

本批未新增/修改界面入口、账号提示或启动按钮。现有只读Run值自动带新observedOutcome，但不将候选转换为执行权或正式任务完成；原scope/issued收据及本地组切换合同不变。

## 371–375：本地下一部署/历史监视的双步操作入口

[LifecyclePanel](FleetMissionLifecyclePanel.md)已接指挥中心和规划页各一处：查看只读12键 metadata与最多8条6键组行，先选择创建新组/选择历史监视，再作明确确认；默认关闭/取消，Enter不提交，原 view/press ticket 与来源代次防止旧视图动作。Root 仅固定公开 `prepareNextFleetDeployment(currentToken, true)` / `selectFleetObservationEpisode(id, observationToken, true)`，不让面板选择 raw QObject 或恢复22键引用。

专用 `ForController` native 接口持实际 Root 的同一 begin → 私有栈 permit mutation → 新 scope/原 ID end，不先释放再调用 legacy；独立 JS transitionBusy 覆盖整个返回栈，pure scope/来源回调不因此自拒。提交发布/选择与 end 独立报告，失败不回滚；失败尝试也计8组容量，allocated=false 不证明未计费。已返回 publication=true 时先作本地旧草稿 token 失效/身份意图协调，再读取后置来源；协调失败不复活旧批准。

prepare 不自动换 selected、不登录或继承 grant、不结束旧 Run、不释放占用；历史选择只改变本地监视，不移交远端控制。新组须重新完成真实身份和原 full gates，旧 Close 仍按 retained 实际 workflow 与原完整收据路由。8源码/19文档的局部入口不等于全链验收：`outboundIntegrationComplete=false`、25目标/六类缺口保持，限定文本检查记录已同步本轮静态审计。未编译、测试、解析/运行 helper、认证、应用或设备，未更新 PR/包。以下366–370及更早为历史，旧无入口/API描述由本节覆盖。

## 366–370：当前组工作流接线与监视身份分离

nested draft 的 finally 只对原scope做cancel/revoke-only补偿；撤销不能核实时 `retiredCurrentScope` 持续拒同组current入口/回调，selected监视仍可读，不超时复活。selected投影先读同组Session的OS身份，再配对纯Series/Run并末核，不声称原子快照。交接应带累计完整源码/pri/注册/原生依赖，五源码只是增量；Qt精确类型/方法/parent/销毁待工程师实核。

Guided 已用 private factory/14个 actual owned 引用替代独立实例，30个 Fleet 入口和24个来源回调固定原组；[Lifecycle](FleetMissionDeploymentLifecycle.md)可创建、Episode不可由QML直接创建。构造已有 initial，应用不自动再 prepare；不公开 raw refs、不从失效当前源回退 legacy。普通业务的外层同步 call 占用阻止同栈切代/改选择，plain lifetime/finally 只清原 owner+ID，pure callback scope 本身不查瞬时 busy/fullgate 或授予权限。

明确身份意图绑定实际 Session/scope，不在认证返回时自动认领新 current Session；shared Role 只是界面角色上下文，原账户、期限、来源和 full gate 仍须重验。nested draft 会使 active current 调用 stale；后置失败保留已 queue/可能发生/未知事实，不声称回滚。selected 的 Series/Run/Close/Session 四值同 scope，sourceToken 含选择代次；监视显式登录可能撤当前未消费批准，但不结束 Run。Close 批准/取消须完整 issued 收据，取消路由原 retained workflow，不按 UUID 找组。

新部署/历史选择的实际产品入口及专用 transition 原生 API 尚未实现，不能把普通受保护 call 当切代通道，`outboundIntegrationComplete=false`；原25目标/六类仍有实质源码工作。5源码/18文档已完成限定文本审阅，不是运行或安全通过，未编译、测试、运行认证/应用或更新PR/包。以下为历史增量。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

## 346–350：十二类原生服务门禁接线

[Episode服务接线](FleetMissionDeploymentLifecycle.md)已覆盖另外12类服务的新请求、live依据、审批/身份移交及现有scoped发送fence：归组来源须仍为current且owned peer属于同一实际组；旧取消/revoke/drain、被动Run/Close和真实已queue记账不借此封禁。Authority/Txn认证移交上下文改为纯本地比对，完整身份/风险门禁仍在调用两侧；Series保留严格一次未发布构造绑定。

仍未形成新部署可用闭环：普通Deployment上传/回读的legacy PlanManager在插件后、异步每包/重试没有独立Episode fence；Guided actual Episode/epoch、共享Role身份上下文和selected旧观察整组投影尚未接入，无新QML注册/部署入口。只改源码及限定静态审阅，不编译、测试、运行认证/应用/设备、更新PR或打包；原25目标/六类继续。


## 345：非 Close 用途的原生身份门禁

[Session 身份接线](FleetOperatorSession.md)已加入上传/启动/ground用途的current Episode检查、实际owner配对及OS/owner回调后的再次核对；旧组普通身份事实、显式登录/退出、精确撤销和独立Close用途保留。相同账号/UUID或不同工厂的当前对象不能替代同一实际组。

这是340–344核心后的源码增量，不是全链隔离完成：其余owned发送/审批入口、Guided actual Episode/epoch和selected旧观察投影仍未完整接线；无QML注册/新部署入口，现有应用仍单链。只做源码与限定静态审阅，不编译、测试或运行认证/应用/设备，未更新PR或包，原25目标/六类继续。


## 340–344：每代新身份对象，工作流界面尚未迁移

[新 Episode / Lifecycle 原生核心](FleetMissionDeploymentLifecycle.md)为每组新建14个 actual owned 对象，包括 signed-out OperatorSession、EvidenceRefresh 和独立审批/Authority 链。旧完整组/Run/占用保留，不复制身份、proof、批准、剩余期限或启动许可；同 binding 不是跨代复用依据。五个 actual typed 共享来源冻结，8次分配含初始/失败尝试，不驱逐历史。

切代证据只读全部 retained native 操作状态，固定18键，不调 public current getter、OS/WTS 或 QML；静默不是落地，也不会关闭旧监视。**尚未注册 QML、替换 Guided 或增加新部署入口，现有应用仍单链**，native sticky outbound helper 也尚未接满 owned 请求/继续点。

下一阶段须冻结每次调用前后的 actual Episode/epoch/服务，OS 返回及 finally 只处理原组；QML/source callbacks 核真实来源，shared Role 显式区分 current 任务与 selected retained Close 身份，全部 retained modal/OS 操作统一忙锁。selected Run/Series/Close/Session 必须同组投影，不能拿新 Session 结束旧组监视。限定检查结果已记录；未编译、测试、运行 API/helper/认证/应用/设备，未更新 PR 或打包。

## 335–339：关闭监视的实际操作面板

新增[监视面板](FleetMissionObservationPanel.md)，在CommandCenter和Planner固定显示，独立于本地草稿/upload权限。查看不生成review，request只捕获原生范围，三个默认未勾确认与另一个账号审批后才沿原有native关闭。两个入口各自冻结完整review token/controller，不能相互接管或取消；modal调用ticket贯穿实际同步调用，不由滞后busy快照提前解锁。

335–339后已有关闭UI源码，覆盖330–334“无新视觉按钮”的历史状态。它不新增Start/ARM/STOP/LAND/DISARM或Completed入口，不替代原生身份/范围/期限核验；未知效果不变成功。ui-ux-pro-max用于默认取消、危险Enter拦截、换行及键盘焦点，未运行脚本/渲染/对比度验证。未编译/测试/运行认证或设备，完整路线图仍继续。

## 330–334：监视结束与原地面上传/启动审批分离

新增[独立Close后端](FleetMissionObservationClose.md)，controller内部实际对象单独提供requestReview、approveAndClose（三项确认）和cancelReview函数，公开仅readonly current值；没有新增视觉按钮，不把原上传面板/已阅/旧启动批准复用为结束监视权限。

新动作只绑定actual Series当前Txn/Run/Swarm/Orch及完整context18，实际两账号、独立purpose、原30秒和真实map占用/撤销均重审。停止本地monitor、释放本地占用、飞机可能继续飞行三项必须明确确认；未知释放不报成功，取消只取消本review。这个动作不上传、不解锁、不停止飞行、不宣布机载任务完成，既有地面上传/启动语义保持。当前只后端接线/源码审阅，未编译、测试、调用认证/业务/helper或操作设备；完整执行UI、StartPermit/净空/正式结果与原25目标/六类继续。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

320–324（2026-10-15）新增[显式 HOME 观测过渡](FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

315–319（2026-10-15）新增[本次启动的连续运行观测](FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

310–314（2026-10-15）新增[错误可区分的原生目录清点](FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

305–309 新增[遥测只读历史检索](FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

300–304 新增[原生入站遥测记录器](FleetTelemetryArchive.md)：独立Vehicle pre-plugin被动入口、实际PRIMARY/Vehicle/observer绑定与退休、27消息固定Codec、parentless有界NDJSON worker及native确认/公开只读值已接线。新增默认关闭，不改旧.tlog，不请求数据流或控制；定位类另明确确认，原len payload与RX序号只观察，签名/CRC/身份不认证。文件NewOnly不覆盖或删除，入队/write/flush/durable分层，源失效停止整组并保留unknown尾部；fullTelemetryCaptured始终false，不把此子集当完整遥测或任务许可。运行/编译/测试/解析helper/设备均未执行，未改PX4/刷机/更新PR/打包；原25目标/六类不缩减。下方295–299及更早为历史批次。

295–299 新增[跨封存段只读浏览](FleetPersistentEventStore.md)：Sql唯一schema/身份/严格页抽取、parentless单请求Reader与Store native目录/分页/取消及public值facade已接。actual目录条目生成本catalogId，页域绑定文件/DBepoch/sealRun/filter，最多128个每段单filter游标，只0或实际next；当前页/Reader共单槽，取消与错误不提前复用，历史不回灌rules/Replay/任务/权限。目录128段/4096条目与JSON页有界，合法1MiB政策的内部预留预算已修；目录全集、全库记录hash、跨段原子快照/飞行序都不宣称已验证。仍缺全集索引/自动归档清理、完整遥测/运营集成及最终StartPermit等原六类/25目标；未编译/测试、运行SQL/目录或hash helper、改飞控/刷机、更新PR或打包。下方290–294及更早说明为历史批次。

290–294 新增[显式维护封存轮换](FleetPersistentEventStore.md)：实际worker协作锁贯穿排空/SQL关闭与同目录旧库保留，native确认后才请求，不自动删旧段；actual clean/sealed/finished、精确DBepoch/文件名和同源QQ/epoch核对后才沿本次明确配置建立新run。取消/退出/源丢失只退休续建，不抹掉可能已发生的封存事实；暂停序号窗口显式known/unknown及缺口摘要，不称连续无损。封存段数有界、未知文件不覆盖，hash/协作锁不认证或保证恶意并发/失电安全。仍缺跨段查询/自动归档清理、全遥测/运营集成及最终StartPermit等原六类/25完整目标；未编译/测试/运行库或锁/文件轮换、执行SQL、改飞控/刷机、更新PR或打包。下方285–289及更早记录为历史批次。

285–289 新增[默认关闭的连续诊断归档](FleetPersistentEventStore.md)：实际BlackBox暂时收据/同类型事件身份、固定摘要Codec、专属QtSql线程与Store门面已接pri/type/controller；仅native显式配置/确认后异步开库，默认不读取或写盘，不回录开库前历史。queue+in-flight/批次/主库页/历史结果有界，实际commit后才durable，来源/投影/入队缺口保留诊断；历史单槽直到GUI交付，不进入实时回放、规则、机队、审批或控制链。无原payload/凭据/精确GPS，哈希非签名/匿名化；clean非完整性认证，未知库与需hot-journal恢复库拒绝不重建。仍缺全遥测/长期轮换/运营集成及最终StartPermit等原六类/25完整目标；未编译/运行/测试、建库/SQL执行、改飞控、刷机、更新PR或打包。下方280–284及更早批次为历史记录。

280–284 新增[显式局部运动与航向观察政策](FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

275–279 新增[被动 ODOMETRY 原生账本与契约](../../../src/Vehicle/FleetStartupOdometry.md)：原始 MAV2 解码50/reference42，Vehicle full27/reference21；实际 PRIMARY/PX4/版本与飞行来源、接收整栈、prepare前计数和token/epoch防嵌套覆盖。sticky everObservedRawFrame 不把曾断流/无效/被过滤的源当初始缺流；Readiness仅存去age的20字段参考投影，普通收包/时间/运动数字/quality分值不造成身份抖动，reset/frame/状态与已观察ABA撤销旧依据；地面READ冻结该投影，Transition前后重核实际来源。稀疏协方差NaN保留未知，扩展尾零/LOCAL_FRD/时钟不认证参考有效性或同采样。不增加传感器、不请求/启用流、不改原六通道/read34/startup32、不放宽HOME/local严格政策。源码接线与有限静态审阅，不是完整运动/转向/净空/最终StartPermit或执行完成；原25目标/六类继续，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方270–274及更早批次是历史记录。

本轮270–274操作顺序：显式请求最新terminal owner的新Review→精确退休旧GroundAuthority/Approval链→冻结实际QQ+UUID/epoch/generation/hash→新完整READ及已阅→新两账号条件审批→accepted新Authority→Series分配独立consumer→原固定受控动作。步骤仍由后端显式申请，无自动取证/审批或新执行按钮。空消费者初始null由脱敏snapshot helper处理；编辑/登录/退出捕获actual consumer QQ+旧UUID后再取消，通知即使改了pool指针也不取消replacement。容量8/来源丢失/MODE事实/未排空直接拒绝，不静默删历史或沿旧第一终态放行。

270–274 新增[有界实际启动事务序列](FleetMissionStartupSeries.md)及多轮地面再审后端：最新已初始化owner不回退，逐轮冻结actual QQ/UUID/epoch/generation/hash，旧转交链精确退休后重新完整读取/已阅/两账号审批；每次新消费者保留前代，初始/终态对象不能重用覆盖。native固定历史摘要6防递归复制，完整当轮任务/新风险/真实queued事实仍保留；同一原组/记录/部署，最多8个ground child（空候选也计费），源丢失/容量满/MODE已可能排队/未排空均不淘汰或恢复授权。实际pri/type/controller已接，限定源码复核完成，未编译/运行/测试/API/飞控改动/刷机/PR或打包。容量/新部署重置、HOME有效分支/local frame/运动转向、净空/最终StartPermit/时序/执行完成及原25目标/六类仍待开发。下方265–269及更早批次是历史记录。

265–269 新增[地面再审后的分阶段启动事务](FleetMissionStartupTransaction.md)源码：actual8/context27、一次28→独立30（Session6/Authority10）、同UUID连续占用；独立第二实例不覆盖旧终态。已解锁成员不发ARM，但仍核实际Seq0完整journal与整组屏障；未解锁成员仅普通ARM(param2=0)，之后仅mode-only。首次Seq0前要求实际周期RAW基线/政策gap，错误实例在换generation前拒绝；后端接线且限定源码契约复核完成，无新执行按钮。本机未调用接口/编译/运行/测试、改飞控、刷机或更新PR/包。最终StartPermit/净空/完成、第二事务再次终止后的实际owner重审路线、HOME有效分支/local frame/运动转向及原25目标/六类仍待开发；旧第一终态不能代替第二事务事实。下方260–264及更早批次为历史记录。

260–264 新增[地面再审启动依据一次接管](FleetMissionGroundStartupAuthority.md)源码：实际10来源/原始context27、完整新RAW/全风险/记录/typed当前地面状态进入新owner，旧22一次移交Session独立28（6字段返回真实remaining）、Ledger10字段接管。旧账本退休不盲撤新owner，原最早READ START/两身份期限只缩短，独立实时查身份/WTS/源与整组占用；public不泄露SID/RAW，所有执行/启动/飞行许可false。实际后端/pri/type已接，本批限定源码契约静态收口已完成，无读取/ARM/MODE或UI执行按钮。真正分阶段模式消费者、HOME分支/local frame/运动转向、净空/最终StartPermit/完成及其它六类/原25项目标继续；未编译/运行/测试、调用接口、改飞控、更新PR/源码包/安装包。下方255–259及更早批次为历史记录。

255–259 新增[地面再审独立账号条件意图](FleetMissionGroundStartApproval.md)源码：实际新 Review20/九对象/冻结整组与 armed 状态、完整最新记录/风险/新 RAW，真实两账号新提示和 Session 独立22证明，一次消费/撤销及 native14 grant；public27 receipt 无 SID/RAW。最早 READ START 原30秒不续，旧证明不恢复，实际提示期间主身份仍查 SID/role/WTS/旧期限；active READ与消费不放宽。实际 backend/pri/内部类型已接，本批已完成限定源码契约静态收口，没有新增执行按钮/新 startup authority/MODE续作。最终许可/净空/HOME有效分支/local运动转向/完成及其它六类/原25目标继续；未编译/运行/测试或调用接口、改飞控、更新PR/源码包/安装包。下方250–254及更早批次为历史记录。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

## 244–249：事务后端，未加执行界面

265–269新增后端 `requestFleetGroundStartupTransaction(startupId,true)`、精确cancel及第二实例只读状态；所有新请求仍需显式当前GroundAuthority、actual8、完整新风险/原计划和原期限。编辑/身份切换/退出先取消第二事务，再撤精确Authority/旧账本。其准备态可完成同startupId的旧owner移交，持续核对不靠旧ack/valid保活；没有新Start/ARM按钮或自动调用。

已解锁成员不重ARM且不显示伪造ACK，未解锁成员普通ARM后全组模式屏障。工厂接收不是动作成功；只有真实ARM/MODE queue才能将flightCommandReleased记true。第二事务再次终止后的重审不能回退旧第一owner，当前明确拒绝，待实际新owner路线开发；完整飞行/完成仍未交付。

controller新增实际八对象requestFleetMissionStartupTransaction(startupId,true)、精确cancel及只读值，编辑/身份切换/退出先cancel实际事务。原计划/风险/独立账号/显式政策仍匹配才一次接管并异步固定三步，工厂接收不是成功。没有Start/ARM按钮或自动运行，本批未调用函数/编译/测试。[事务](FleetMissionStartupTransaction.md)与[阶段](FleetMissionStartupTransition.md)保留partial queue及HOME/local参考/运动/转向/净空/完成缺口，停止不等于回滚或“没有起飞”。

## 当前 239–243：一次Startup依据接管，尚无执行界面

controller新增内部实际[StartupAuthority](FleetMissionStartupAuthority.md)的只读值、`requestFleetMissionStartupAuthority(reviewId,true)`与精确`revokeFleetMissionStartupAuthority(startupId)`。只有当前实际已阅风险、独立真实账号依据与全组声明域BeforeArm事实仍一致时，原生流程一次消费/移交并取得最多原最早30秒内的本地占用；没有新增按钮、默认确认或实际发指令入口，不能把本API当作start/arm。

旧StartApproval转transferred后不能继续consume/grant，旧Review/Content仍是不可变历史，不作为新owner持续身份校验；新owner仍核原SID/角色/WTS/来源/原TTL。优先用精确startupId取消已移交依据；旧账本显式撤销只可精确转发保存的实际owner/原startupId，不能由旧Review失效自动清新proof。原owner已取消/到期/销毁后只终结旧账本，不能清同实例的新startup或复活旧grant。退出或草稿编辑亦失效，没有自动renew、回滚或再次批准。当前preARM-only，实际ARM依旧会撤销依据，未来ActualStartup事务尚须单独接入允许差异和命令派发。

本地typed地面POSCTL候选不证明遥控中立、机载可解锁或净空。旧插件与MISSION_START隐式解锁不能当纯模式动作；本批没有运行这些接口、认证、界面、编译/测试/仿真，不授最终StartPermit/执行完成。下方为此前批次背景。

## 启动前整组任务只读核对（234–238）

现有面板新增显式 [StartContentCheck](FleetMissionStartContentCheck.md) 入口，机号来自当前实际部署的冻结目标，而不是可变草稿。当前原生系统身份和 `mission.start.content.read` 权限必须一致；默认未勾选“撤销旧审阅/审批，仅只读核对”，按下至点击冻结控制器、部署 UUID、七字段绑定、机号、身份与九字段权限。返回后来源变化只尝试取消原匹配核对，不显示新控制器成功，不回滚机载任务。

控制器在新读取前撤销原上传批准、启动风险/条件意图与草稿批准；原生 Deployment 在冻结上下文前撤销旧 live verified 暂存。所有新请求、重试和读完 ACK 均受实际源/原账号/期限/自身读取作用域的原生 fence，失败/取消不把旧缓存当新证据。面板显示逐机完整项数、匹配数、原因、排空占用及保守倒计时，区分“已匹配但过期”和“仍可供原生复核”，停止精确本次请求；不自动重试或再次批准。

采集最多 120 秒，但风险审阅证据只保留各机 READ START 起算最多 30 秒，整组取最早截止，完成和打开审阅都不续期。大批次可整表匹配但已不新鲜；不能通过等待或旧审批补齐。独立账号提示忙本身不抹掉已完成的只读事实，但原账号/角色/锁屏/来源/期限变化仍使其失效。新审阅要求这份实际整组来源并保留历史上传/缓存/新 RAW 三种区别。

UI/UX 指引用于默认未确认、长文本换行、明确禁用原因与逐机/排空反馈，沿用 Qt5/QGC 原生控件与主题；没有启动/解锁按钮，也未运行界面或认证、编译、测试、发送 MAVLink。下方章节为此前批次背景，真正受控启动/完成及其它六类剩余源码工作继续保留。

## 条件启动意图的独立账号依据（229–233）

新增 [StartApproval](FleetMissionStartApproval.md)，它与上传批准、风险已阅、真正启动许可是四种不同状态。controller只公开当前值与申请/匹配撤销；实际私有Session/Role/Store/StartReview/Deployment、不可变已阅风险、最新非恢复全记录和固定机组来源必须匹配。申请人是当前真实operator/commander/administrator，独立批准必须为不同SID的safety_officer/commander/administrator；mission.upload凭据或4分钟上传租约不能作mission.start依据。

这是**条件意图**批准，不是允许飞行。原风险artifact的未解锁、未知/缺机载内容认证、secondApprovalEvidencePresent=false等字段不修改；独立账号证据由另一个对象保存，不用checkbox消除技术缺口。Session独立认证最多原60秒，但本账本更早受原风险审阅30秒限制，包括风险已阅和系统凭据窗口；时间过长须新审阅/已阅/申请，不自动rebase或续期。后续原生消费仅产生conditional grant，三个startupPermit/executable/flightCommand字段仍false。

原风险RAW输入保持完整v1；风险v2独立语义revision排除同值新收包/GP输送时钟，不忽略实际位置、速度、heading、armed、任务序号、fresh/unknown/来源或A→B→A。健康通知按原实际风险gate/Leader判断，纯年龄推进不重置审阅，真实门槛/来源变化仍撤销。草稿编辑和只读刷新撤销原已阅风险及条件意图。

本阶段没有新Start/ARM按钮、实际启动消费入口或Vehicle指令调用。鲜机载内容、最终StartPermit、受控解锁/启动状态过渡、事务和完成归属仍需继续开发，不交给工程师假装这些已由测试补齐。本轮仅源码/静态审查，系统凭据窗口也未运行。

## 受控只读刷新（225–228）

现有面板从当前草稿成员列出刷新机号，而不是自动选择其它飞机或依赖已有归档。先显式认证具备 `mission.evidence.refresh` 权限的本地角色，再勾选“撤销旧审阅/审批，仅刷新所列飞机证据”；默认不勾选。按下至点击冻结实际 controller、完整身份、九字段权限和唯一整数机号。控制器先复制目标并撤销当前审阅、意图批准和上传批准，再调用实际原生 [EvidenceRefresh](FleetMissionEvidenceRefresh.md)。刷新拒绝或取消也不恢复旧批准。

最多32架按版本后十二固定参数顺序请求；显示冻结请求/机号/阶段/参数/原因/完成机数，停止仅取消匹配请求，不撤回已经进入链路的只读包。总120秒、每阶段4秒、间隔至少100毫秒；只有明确未发出的占用/限流/缺地面状态才在原期限等待，已知解锁或空中状态不当临时等待。完成仅证明有限历史观测采集，不证明报文回应本次请求、硬件认证、持续新鲜或任务可执行。后续审阅保持独立30秒参数年龄；大批次可能采集结束即需分批重新审阅。

只读动作不要求独立上传批准，也绝不提供参数写入/上传/模式/Start/ARM；原上传的双账号与租约政策不放宽。UI/UX指引用于明确撤销后果、默认未确认、按下冻结和禁用/失败/逐机反馈；本轮未运行界面或验证键盘/主题/布局。

## 当前有限租约口径（221–224）

短期审批申请仍包含系统凭据窗口耗时，60秒内只能消费一次。审批窗口和面板同时说明消费后最多4分钟的**本次部署上传/回读**租约；原操作员5分钟会话仍为更早的硬上限，没有自动续期。22字段schema2审批包含明确租约政策，13字段schema2 grant绑定消费前生成的实际deploymentId。原生双方当前角色及实际Windows活动/未锁定桌面继续核对；锁定、注销、断连、退出、换身份、撤权、未知或到期停止后续步骤，不回滚。

独立审批60秒过去后不是把旧认证伪装为仍新鲜：在60秒内实际消费才能产生一份不同的原生有限租约，其历史批准依据、当前来源和剩余期限分别核验。租约不能移交另一部署，验证不得刷新截止；使用原生获准duration形成另一个clock的保守deadline，不直接比较不同实例单调时间。快照剩余不是实时倒计时。新审批只能在旧已消费授权明确撤销或到期后申请，避免暗中替换正在上传的授权；完成回读后仍可显式撤销原租约再申请下一任务。

当前适配器要求Windows10+且实际WTS注册/会话来源可用；不支持旧系统反向锁定flag猜测或“上次未锁定”缓存放行。工作站事件/查询仍有调度及查询竞态，不是硬实时或企业认证。没有锁定用户工作站或实际执行登录、运行界面、编译、测试/设备操作；下方217–220的“60秒覆盖全部续作”是历史状态，已由本有限租约口径替代。长期续授权、跨站租约、独立启动和完成闭环仍待开发。

## 已接入的源码流程

任务地图规划面板末部接入 `FleetMissionDeploymentPanel.qml`，复用现有 Qt5/QGC 控件、布局和主题。没有新建另一份控制器、机队或任务状态；界面只使用实际 controller 的只读 facade 和狭窄操作函数。

上传是覆盖机载任务的危险写入，不是飞行：

1. 先通过已有规划审阅/版本归档/人工计划批准；编辑未保存、地图选点未结束、版本或原生证据变化时不得沿用旧批准。
2. 申请确认仅生成本地有期限的请求，不触达 MissionManager。确认框冻结 recordId/revision/planId、目标 ID、实际编码任务项总数、到期时间和操作者/角色/策略 epoch/revision；实际项数来自 `transportEncoding.members[].itemCount`，不是路径点数，也不是没有暴露的完整 payload 列表。
3. 默认焦点取消；内置自动 accept 和 Enter 危险提交关闭，显式确认按钮不为默认动作。取消/到期/旧回调只能撤销匹配的请求，不清除新请求。按下至松开再到点击期间发生版本、输入或角色变化同样拒绝。
4. 最终确认一次性消费请求，再由原生 Deployment 复核归档、实际私有角色来源与快照、健康/忙机/链接/地面未解锁和逐步事务归属。显示逐机上传与独立回读状态；ACK、回读匹配、写入可能已发生和执行许可保持分开。
5. 停止只阻止后续步骤并等待旧传输排空，不清空/回滚机载数据，不保证已写入可撤销。

## 本地身份与独立账号批准

新增 `mission.upload` 与 `mission.start.review` 权限及原生实际策略快照。空身份/observer默认拒绝；未知新用户和退出不能继承原高权限。策略实例 epoch 和不回绕十进制 revision 防止角色 A→B→A 或重新定义角色后复用旧确认；九字段先按实际 String/Bool 类型校验，不接受整数/字符串冒充 bool。

RolePolicy本身仍是本地合作式权限门禁，**不是登录或密码验证**。新的[OperatorSession](FleetOperatorSession.md)通过显式Windows系统凭据认证取得实际SID，并以工程师配置的固定专用本地组直接成员关系取得角色；controller只从这份原生来源同步策略。系统身份、九字段角色快照和[OperationApproval](FleetOperationApproval.md)的独立账号收据保持分开。源匹配回调不是对恶意同进程插件的隔离，也没有企业/服务端认证、跨站控制权或数字签名。界面不提供任意填写操作者、选择管理员或修改权限的捷径。

现有operator/commander/administrator仍要求额外批准。先显式认证操作员，再为当前显示的任务/版本申请独立账号审批；系统窗口要求不同实际SID且具有safety_officer/commander/administrator角色。申请等待计入60秒寿命，收据只可消费一次，消费不延长期限且限制后续全部传输gate；长批次/慢链路可能停止并排空。再单独申请/确认覆盖上传。两个账号不证明两名不同人员，单个确认框也不冒充第二审批。未预设身份或改变默认要求；源码不创建Windows组/账号，非Windows/未配置/未知或冲突来源拒绝。审批可匹配撤销，退出也撤销后续授权；二者不回滚已写数据。快照剩余毫秒不是实时倒计时，真实有效性由原生单调时钟和每步核验决定。

## 只读状态与新的启动风险审阅

主动状态检查提供最多1秒的 snapshot-only显示，并允许展开有界阻断/未验证原因。任务序号只是与当前缓存的未归属对照，不显示任务完成率或“安全可飞”。UI可保留明确标记过期的历史参考，但不能将其用于批准；定时器通知受GUI事件循环调度影响，非硬实时保护。

Deployment 在完整实际回读成功时保留本runtime历史内容回执；失败/取消/ARM依旧撤销live readbackVerified，不恢复旧审阅。只有全部成员历史回读匹配、实际对象未替换且传输空闲，原生只读 `startReviewContext` 才可给新的独立 [StartReview](FleetMissionStartReview.md) 提供来源。它不是机载当前内容认证，也不接受文件恢复的历史当有效会话。

独立风险审阅使用新reviewId、当前角色/原生来源、最新非恢复计划记录、健康、参数/版本/目录与当前缓存核对，30秒到期或来源变化须重审。生成入口可在ARM导致staging failed之后工作，不能靠重新标verified绕过；部分上传、换机、忙事务或旧记录由backend拒绝。界面只生成/展示风险；backend的“已阅”仅记录同一不可变风险快照，不是批准启动、第二人证明或执行令牌。

所有风险审阅/状态结果的启动、执行、任务完成许可仍false，风险artifact不会因本地登录变成启动令牌。没有Start、ARM、MISSION_START或模式切换按钮。完整独立启动事务、新鲜机载整表身份、受控命令释放、完成归属、企业身份/跨站租约及其它目标仍属原路线图后续源码工作；本批只补Windows本地身份与上传审批路径。

## 本轮验证边界

UI/UX技能用于危险动作二次确认、默认取消、键盘/焦点、冻结数据、语义禁用及明确未知/过期/非完成标签。只做源码/资源/类型接线与静态审阅；未运行软件、渲染界面、测对比度/尺寸、编译、测试、仿真或触达真机。工程师需在受支持Qt环境核查Popup生命周期、窄屏/大字号/键盘/深浅主题、双击/Enter/Space/取消/版本切换、账号/审批、错误/排空及真实遥测时序。

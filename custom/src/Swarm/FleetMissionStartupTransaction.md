# 原生有界三步启动事务（源码实现，未编译或运行）

## 524 当前：独立观察关闭的原生提交凭据

仅 `commitObservationClose` 真正按原上下文成功提交并退休原Run后，安装私有sticky `_independentObservationCloseCommitted`。普通startupRetired、owner死亡、stop/cancel、空观察或文件声明都不能安装；不新增公开可写Bool、身份、许可或观察关闭旁路。后续[旧部署归档退休](FleetMissionDeploymentLifecycle.md)逐个实际事务读该原生事实，不能只看到Run.retired就声称已获独立Close。原严格never-began准备自动关闭另以原私有标志、逐目标queue事实和实际占用全核，不恢复审批或推断物理停飞。

这不改变三步发包、原独立Close审批、占用释放或正式结果语义。仅源码静态修改；未编译、执行项目/helper/测试、仿真、操作硬件或放行。

## 492：队列observer getter的原对象尾栅

executionObserverForQueue先固定本次actual observer、原Txn QQ／generation／startup epoch／stage和目标值，不再持_targets引用跨readyForQueue（该函数仍可能live Link回调）。返回后先核QQ、同observer与原上下文，再按原commandContextCurrent重核；失败不能返回后来新observer或继续访问已死this。

观察关闭上下文调用Orchestrator的RetirementMatches也可能间接触发live回调，返回后须先核原Txn／Run／Orchestrator QQ及原上下文，再继续读取或给出上下文；不得将retirement检查称为无回调的纯getter。

原完整command source／阶段封条／PM29、30秒总期限及阶段预算不续期，命令步骤／skip ARM／实际write后commit事实不改。stop收到startupRetired回调及drain关闭未提交准备后也先核原QQ／generation／epoch／state及原observer；不借新观察快照恢复旧权限；公开只读executionObservation直接返回值，不在外调返回后继续解引用原owner。

本轮不新增遥测／协议、真实动态轨迹或StartPermit认证。Run／Vehicle各自的guard也不能替代原Authority、风险、同源、角色与最终write门。见[Run](FleetMissionExecutionObservation.md)、[Vehicle](../../../src/Vehicle/VehicleMissionExecutionObserver.md)和[未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490：原29与首次PRIMARY控制块贯穿各stage

Target增加WeakLinkInterfacePtr primaryOwner，头显式include LinkInterface.h。_requestStartup取得实际Vehicle／MM／PM／LinkManager／PRIMARY后、groundRead等后续逐机回调前，固定该弱控制块，严格取beforeArm.parameterSummary.originalExecutionParameters并调用原PM29 MemoryMatch(30000)。缺失／错型／null／旧行或原源不匹配直接拒绝，普通启动和armed-ground共用。

_targetsCurrent唯一capture调用现传实际PM、Target的原owner、原29与原policy六参；在任何后续per-target回调前捕获。逐机末尾、整组末尾及既有_phaseMemoryCurrent／Vehicle.commandContextCurrent均复核同seal，不从后采样值刷新。之前无clearanceInput时最后native queue不含参数内容的缺口由此接齐；有clearance时Witness独立原29检查保持。

PM按semantic content而非冻结receipt判定：同值新原包可刷新原ledger年龄，变化／ABA／Fact或manager来源替换、pending、过龄及ramp符号差异失效。没有改_expiresAtMs／stage截止、认证转移、命令步骤／重试／ARM跳过或其它业务权限；不构成完整StartPermit或正式结果，不能为缺真实动作／动态净空的路径改true。见[封条](../../../src/Vehicle/FleetStartupMemorySeal.md)与[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 481：显式新域沿原Motion进入实际contains

Txn仍按原payload／policy／baseline／ceiling调用同一Motion.matchesSource，有资料时须与Witness原对象完整对应；无资料只在原准备点生成一次本次模型。显式profileguard决定source2／原IntegerDomain，缺失保持source1；本轮没有重采样或从source Map替原模型。

Transition初始及有序GLOBAL_POSITION采样仍调用同一原Motion.contains，新模型在旧Qt／高度／速度／航向之前加真实整数域gate。该gate只约束被采到的编码位置，不证明两包之间、真实起降／平滑器或整个飞行轨迹。源身份和旧审批须因新选择变化而失效；没有新增命令、旧Run恢复或占用豁免。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 478：资料内容重放与当前执行构建分离

`validateRemainderPlanningCollection`及原单成员取源中的两处完整计划重编码使用 `FleetMissionPayloadBuilder::reproduceHistoricalPayloads`；它只证明原frozen plan与原payload值相同，不读取飞机、不改变Run或恢复权限。缺policy旧档保builder2.0及原payload形状；含policy新档仍走严格正常build。

原plan已有完整policy时，整组validator与native单成员getter另外从原sourceTask.area和原完整plan.members重建并要求相等，不能保active plan却删掉原task孔／退让而生成无约束新提案。各自原snapshot预算、原owner／有序targets／一次观察及尾核保持；单成员共享validator同样配对原域。

当前实际Deployment／Readiness不走历史重现口。没有改动ARM／MODE／READ时序、最终memory gate、StartPermit或动作净空；这些仍有实质未完成代码。仅静读未运行、编译或测试；下方为历史。

下方为历史记录。

## 474：同一真实 Run 的完整整组历史规划快照

新增只读 `Q_INVOKABLE remainderPlanningCollection(expectedRunId)`，用于新的[整组余段资料包](FleetMissionRemainderArchive.md)。原单成员 `remainderPlanningSource(expectedRunId, sourceVehicleId)` 的 native18 合同和原完整检查保留，不改为循环取源或放宽未提交 MODE 成员的准入。新集合不是 Run 恢复、审批、占用、控制权或飞行入口。

新 getter 先冻结调用时 Run ID、原实际 Startup／observer 对象、transaction epoch／generation、startup／deployment ID、完整 binding、原 task／plan／vehicleIds，以及两个原生 owner 的完整 target 值集合。每个 target 的 component、原 member／payload、READ 引用、MODE step、observer itemCount／sourceItemMap／payload hash、queueBegan／queueCommitted 与 source epoch／operation／wire hash 均需对应。它不跨外调持有原容器引用，并只对整个实际 observer 调用 **一次 `current()`**，从这同一份 Run 快照获取所有逐机 progress；不是调用 N 次旧 public getter，也不拼接先后不同的观察。

实际来源在 native `current()` 前后、完整纯值检查后再核同一 owner／generation／全部原顺序和目标绑定。普通进度计数前移不要求计数永久不变；但不能在取源期间换 Run、换目标、换 member／payload／READ／step、变 queue 身份或将未提交队列变为已提交后混入旧共同材料。任一身份、材料、映射或资源失配均整包失败，不丢坏行、只收成功成员或另取较新来源补齐。

### 新集合 exact16 / 每行 exact6

根字段固定为 `schemaVersion=1`、`ok`、`reason`、
`sourceKind="native-run-planning-collection"`、`runId`、`startupId`、
`transactionEpoch`、`transactionGeneration`、`deploymentId`、`binding`、
`sourceTask`、`sourcePlan`、`vehicleIds`、`targets`、`readonly=true`、
`flightCommandReleased=false`。共同 task、plan 和 Run 身份仅根部保存一次。

`vehicleIds` 为 1–32 个原实际唯一 ID，`targets` 按这同一原生顺序保留所有目标，每行固定六字段：`vehicleId`、`available`、`reason`、`sourceMember`、`sourcePayload`、`progress`。逐行不能省略未 MODE 成员或把其缺值补成成功 native18。

`progress.modeQueueBegan` 和 `progress.modeQueueCommitted` 必须是真实非 null Bool，committed 不能先于 began。`available` 严格等于两者都 true；可选行 reason 为明确空字符串，合法未提交行固定为 `original-target-mode-not-committed`。未开始与已开始未提交都保留其真实 flags、完整原 payload 和全部 progress，包括独立 provisional 观察；它们不是可恢复余段来源。已提交但后来退休、失联或 `sourceCurrent=false` 的行仍可保留为历史规划材料，不把历史可选称为飞机目前可执行。

成功 `ok=true`、reason 明确空；失败仍返回原16字段信封：`ok=false`、非空 reason，所有身份字符串为空、binding／task／plan 为空 map、vehicleIds／targets 为空 list，原只读与不发命令标记保持。失败没有可误用的完整成员或成功前缀。

### Archive 共用的纯值完整校验与选择

`static validateCollection(collection)` 返回 exact3 `{ok, reason, collection}`。成功返回独立 JSON-owned 的整个集合，保留显式 JSON null；失败 collection={}。它先核 exact16／exact6、整树资源和原完整 binding／plan hash，再对整个 plan **一次**运行原 PayloadBuilder，核全部唯一成员和完整 payload。plan 的内部 members／payload 数组按唯一 ID 配对，不要求它们碰巧与原生 vehicleIds 顺序一致；输出 target 顺序始终不改。

每个 payload 为 1–2048 项，整组最多8192项；所有原成员、完整 sourceItemMap、itemCount、component／MODE step／queue 身份和 current／reached 序号映射都校验，未提交成员也不能保留坏载荷或坏映射。current／reached 正式观察不得出现在未提交行，独立 provisional 值不提升为已提交进度。sourceCurrent 保留真实 Bool；unknownTail／last-observed／未认证新鲜度等固定边界不变。

`static sourceFromCollection(collection, sourceVehicleId)` 返回 exact3 `{ok, reason, source}`。它先完整重验同一个集合的所有行，再仅为指定 available 行构建原 native18，并调用 [Planner::validateSource](FleetMissionRemainderPlanner.md) 再核整份单成员来源。不可选、不存在或坏 ID 均 source={}，不投影未 MODE 成员。Archive v2 的文件读取另外负责完整包身份／内容哈希与显式历史20来源转换；这两个 static 值层接口不认证 caller、Run 所有者、文件真实性或当前飞机，不保存原生 owner、不恢复权限。

整个集合和完整文件来源沿用32MiB JSON／累计键字符串字节、深度24、1,048,576节点、map1024键、list32768项、单字符串32768码元／键1024的门禁，不按成员放大预算。新集合数值使用 Planner 可表达范围，显式 null 保留、opaque／typed-null对象及非有限值拒绝；原单成员取源的既有默认检查保持。所有失败整体拒绝，无截断模式。

本批仅源码修改与直接静读／文本空白核对，没有运行 helper、解析器、归档读写、编译、测试、应用、仿真或设备。工程师须验证单／32成员与总项边界、全部未 MODE／部分 MODE／全部提交、provisional 与正式映射分离、同一 current 调用、目标重排与重复／遗漏、坏未提交 payload、完整 plan hash 与来源切换、对象销毁／重入／queue改变、JSON null 往返和 v1 单成员兼容。观察不等于完成、历史可选不等于当前可飞，原启动／权限／占用／PX4 与正式结果未因本功能改变。下方旧批次保留为历史。

## 471：有资料任务只接原Review／Worker／Witness缓存

有资料路径先经Witness.originalGeographicMotion取同次原Ptr，继续原memoryCurrent及取后alive检查，再调用[GeographicMotion.matchesSource](FleetMissionStartupGeographicMotion.md)严格核本Target原beforeArm GPI9、完整actualPayload canonicalWireItems与hash、规范policy及明确ceiling。该方法保原完整输入校验和invalid／显式null yaw规范化，不运行Qt距离／方位、不替换对象；任一不对应则整组接纳失败。

原先为比较source而先prepare的步骤只保留在无资料旧任务分支。有资料任务直接保存Witness原Ptr，后续Transition仍读取Target同一个对象；没有改contains、阶段时序／允许域、航向累计、原初始检查或最终全组phase seal。字段／参数校验、authorityContext、时效和最终write门禁不因对象仍存活而省略。

这是实际缓存与source连续性推进，不是Qt数值接受域证明、完整StartPermit或正式远端结果。原numericfalse及Compiler pending保持；没有解锁新入口、修改PX4或执行任何编译／运行／测试／仿真／刷机。下方459及更早为历史，资料分支的比较重建以本节为准。

## 459：首次接纳缓存原生地理运动模型，阶段只读取同一对象

`requestStartup`／`requestGroundStartup` 在提交整组 Target 之前，从各自原 `beforeArm`（地面为本次完整 READ 的 `currentRisk`）的 `flightStateSummary.channels.globalPosition.value`、同一 member 已规范化的 `startupTransitionPolicy`、原 payload 和原 constraints 的明确 `maxAltitudeMeters`，一次性准备 [GeographicMotion](FleetMissionStartupGeographicMotion.md)。缺失、坏类型或模型不支持均拒绝，不补高度上限、不读取最新位置替换原样本。任一成员失败不提交部分缓存。

有 `clearanceInput` 的任务还须经实际 [Witness](FleetMissionStartupClearanceWitness.md) 的 `originalGeographicMotion(vehicleId)` 取得其原生缓存对象；该入口先核原 Witness 的内存来源／时间。事务完整比较两份 `source()` 的紧凑 JSON，包含原 GPI9、全部 canonical 请求、规范 policy、ceiling 和 payload hash，而不只比较一个摘要。相同后保存 **Witness 原共享对象**；无资料旧任务保留本次首次准备的对象。此处不将原已有 GPI9／sourceFingerprint 绑定描述为缺失或漏洞。

新增非 QML 的 `startupGeographicMotionBasis(actualVehicle,startupId,exactReadiness)`，只在原 owner 线程、`_same`／原期限仍有效、实际目标 Vehicle／PRIMARY 存在且整份原 `beforeArm` 相等时返回 Target 缓存。它不再次 prepare，不调用 Witness／插件／Link、重新采集、认证或续期；`phaseBasisCurrent` 同时要求该缓存存在。[Transition](FleetMissionStartupTransition.md) 各阶段使用同一个不可变模型，原初始 `contains`、有序 RAW journal、全路线走廊、AMSL／速度／航向检查及其累计过程不改变。

原 Authority、总截止／阶段截止、占用、三步收据、全组 phase seal 和末次 write 的 Witness／内存尾核保持。本次只消除每 phase 的重复准备并明确原生对象传递；没有证明整个实际允许域被净空求解包络包含，没有修改 policy v1–v4、公开 DTO、权限或解除 Compiler pending。仅源码和限定静态对读／文本检查，未编译、测试、执行 helper／线程／UI／仿真／设备，未改 PX4、提交或打包。下方为历史。

## 457：整组原生阶段状态封存与发送前尾核

每次 `_targetsCurrent` 在该机所有可能回调的来源、READ 收据与阶段检查**之前**，用实际 Vehicle、原 PRIMARY 和原 `beforeArm.startupTransitionPolicy` 捕获 [FleetStartupMemorySeal](../../../src/Vehicle/FleetStartupMemorySeal.md)。每机完整阶段检查后核同一封存对象，最后再无回调地核全部原目标；后机的插件或 Link 回调改变前机的实际状态时不能仅换取前机新封存对象而省略其阶段重审。最多32机，任一缺失、接收中、失源、变化或期限失效不返回成功前缀。

`_gate` 进入正式检查时先退休旧集合；真实身份、认证、占用、健康、原记录、部署、三轮完整原生阶段检查与 Root 回调都成功后，再核原净空 Witness 和全组封存对象。只有成功尾核才保存同一 generation、startupId、阶段和原目标顺序的完整集合。`commandContextCurrent` 只读该集合，既不重采新状态，也不运行插件、Link 虚方法、weak.lock 或 solver；它仍叠加原权限、占用、发送成员与原 Witness 门禁，不把封存对象当许可。

`_tick` 的诊断读取也可能进入回调，因此其后重新执行完整 `_gate`，再使用本次 `allObserved` 推进整组屏障；实际执行观察器准备完成后，进入 Vehicle 发送入口之前同样重做完整门禁。新 generation、阶段推进、停止及析构退休旧集合。原阶段／总截止不延长，已有接收栈忙的分支仅在原期限内等待；无法证明当前状态的封存失败按 stale 拒绝，不以旧集合继续。

新 prepared、尚未实际排队的 sender slot 会合法改变原生状态，因此 Vehicle 仍先用旧完整集合核原调用入口，建立该槽后再先调用完整 `authorizationFence`，随后才核新集合的纯 `commandContextCurrent`。未绑定 BeforeSeq0、已关闭旧阶段和自身 `entryBusy` 的值可以被封存，但不能当成已经发送；不跳过初次所有权门禁、不排除 prepared 身份字段。真实最后 write 前继续纯整组核对；write 后的已提交事实、journal、取消与双排空规则保持。

此处闭合的是原生阶段检查与实际发送边界之间的本机状态连续性，不证明报文真实性、PX4 内部 valid、实际净空、运动包络被已求解体积包含或正式任务完成。原 step33、Witness 来源、启动政策 v1–v4、Compiler pending 和公开权限字段不变。仅源码及直接静态阅读，未编译、测试、执行 helper／线程／UI／仿真／设备，未刷机、提交、推送或打包。下方456及更早为历史。

## 456：独立持有原净空Witness到实际末写

request由实际普通／地面Authority非QML getter取得同一[Witness](FleetMissionStartupClearanceWitness.md)，required取原plan.constraints是否明确含clearanceInput，并核原reviewArtifact完整报告的inputFingerprint；缺少必需原生对象拒绝，无资料旧任务只接受无Witness。该ptr和required随原context冻结，不从map恢复，也不借旧Review后续活性。

每轮_gate先跑可能回调的sourceCurrent(pure scope fence)，随后保原角色／账号／认证／占用／健康／目录／记录／Readiness／授权owner门禁；最后所有回调结束再全组memoryCurrent。commandContextCurrent在既有Vehicle实际最后write前所有权检查点再次执行原全组memory核验，之后到write只本地journal／收据赋值。本批未改Vehicle文件，不执行solver、插件、Link虚方法或weak.lock于memory路径。

Txn不把自己elapsed绝对值写回Witness；它保原Review QElapsedTimer／截止、原until和共享UTC水位，只缩短不续期。取消／停止仍走原active／stage／gen／认证和排空门禁，持ptr不是执行许可。

此尾核仅净空原来源／datum／时间连续性，不替代全组motion／receipt／EST／原点／ALT最终一致性。已定位逐机callback核后缺全组motion尾栅，以及普通RAW先observer后depth的窗；Transition球面走廊并集＋独立AMSL slab与已求解三维包络包含仍未证明。Compiler pending保持，完整StartPermit及正式机载结果继续。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 445：显式 v4 高度参考政策与原生 scope 监视

工厂接受完整归一化的 policy v4/9 字段，不把旧 v1–v3 自动升级。v4 沿用 v2 的局部运动/航向观察与 HOME `stop-and-review`，新增 `altitudeReferenceEnvelope`（固定 referenceBasis 与六项显式容差）；实际 `odometryContinuityRequired` 同样要求原 v4 的 motion 依据，原 EST gap、全组屏障、认证、占用、发送与已提交事实保持。

新增非 invokable `reviewedAltitudeContext(actualVehicle,actualPrimary,startupId)`，返回 exact4 `required/policy/reference/sample`。它只读取本实例实际目标 `beforeArm` 的 `startupAltitudeReferenceObservation`、`startupAltitudeReferenceSample`、`startupAltitudeReferenceRiskFingerprint` 和 Required/Captured/Matches 三个 Bool，核归一化的完整 member policy、目标 Vehicle/PRIMARY/ID/组件与原 reference20/sample20。v1–v3 返回 `required=false` 与空 reference/sample；无效来源返回空 map。sample 是原首次已阅样本，不读取当前最新高度替换。

该接口允许终态保留目标的**只读历史取源**；没有放宽 `commandContextCurrent/_same` 的 active/phase/deadline 条件，也不代表当前源、身份或旧批准仍有效。显式新的 GroundReview/READ 可以建立新的 reference/sample，并将该次首次样本保持至新审批/Authority/事务；它不强制复用上一轮 HOME/高度基准，也不恢复上一轮 scope 或抹掉旧违规。当前 GroundReview 不调用本接口强行继承旧样本。terminal row13/outer20 不因这个纯值入口增加递归载荷。

Vehicle 在原实际 owner/startup 首次 scope 绑定一次新 [ALT 监视器](../../../src/Vehicle/VehicleStartupAltitude.md)，每个实际原始 ALT 帧在 Link 回调前锁存数值越界、坏帧、来源/时钟变化和连续接收 gap；每一步与最后 write 前纯内存核验。独立 monitor18 不改原 step33；BeforeSeq0 尚未绑定可无 monitor，后续实际阶段必须使用原绑定监视。scope 绑定之前没有连续历史的时段不宣称已覆盖。

`_stop/_detachDrain` 显式退休原 scope monitor，覆盖尚未创建 step 收据就失败的首次绑定。退休不清违规、原 reference/sample 或已实际 write 事实；正常阶段结束不退休，不能在下一步重绑抹掉越界。专属 scope/transactionEpoch 排空函数仅检查 ALT 接收深度，不因 stale/violation/retired/ok=false 永久等待；原无 owner 通用 quiescence 不改，v1–v3 不新增 ALT 接收依赖。ALT gap 使用独立本机 ALT 时钟，不与原 step elapsed 绝对值混算。

此增量不是最终 StartPermit、PX4 内部有效分支证明、同源/同采样证明或飞行放行。只做源码与静态审阅，未编译、测试、运行软件/仿真/设备，未改 PX4、提交或打包。

## 443：终态新审阅保留原 GPS_GLOBAL_ORIGIN 依据

`terminalReassessmentContext` 的 `physicalTargets` 逐机行由9键增加到13键，原终态外层20键和 step schema3/33键不变。新增 `startupGlobalOriginReferenceObservation`、`startupGlobalOriginReferenceRiskFingerprint`、`startupGlobalOriginReferenceCaptured`、`startupGlobalOriginReferenceMatches`，直接来自本次实际 `captureForStartup` 的[稳定18键原点参考](FleetMissionStartupGlobalOrigin.md)，与原实际 Vehicle / PRIMARY 配对；不会从最新值反补旧的终态依据。

原点18键及其指纹完整进入 `terminalRiskProjection` 和 `phaseFingerprint`，不按周期 age 剥除或重基。所有 flight / health / eligibility 等可回调读取结束后，再以原目标实际对象、PRIMARY 和原生内存 getter / source matcher 对照最初18键。原点变化、失源或已观察的 A→B→A 使本次捕获失效；对比用途捕获仍不替换已保留的终态引用。[ArmedGroundReview](FleetMissionArmedGroundReview.md)须与13键生产者一并接收，并在新 READ 及后续阶段继续保留同一原点四键。

真正从未观察过原点的 typed absence 仅沿既有来源门禁保持兼容；与已观察分支一样，最终仍须核实际 PRIMARY 的原生来源连续性，不能把缺席当有效原点证明。曾观察后失效不能退回缺席。此通道是被动事件参考，不添加周期新鲜度、请求、参数或 profile policy 字段，不证明 HOME / LOCAL / ODOM 共原点、PX4 内部 valid、跨消息同样本或净空，也不增加最终启动许可。仅源码和静态读回，未编译、测试或运行。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../../../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../../../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

本批没有在此三步事务、Readiness/Transition或startup32中新增EST逐包通道；新native健康证据经既有机队/HealthCenter门禁消费，不能据此声称Seq0/ARM/MODE全阶段已有EST gap、origin或最终StartPermit证明。显式schema1–3 HOME/运动政策及原授权/最终queue fence均保留。

## 371–375：下一部署组不会复用原事务或运行授权

[Lifecycle](FleetMissionDeploymentLifecycle.md)新增两个 `ForController` native 切换，连续持有实际 Root begin → 私有栈 permit mutation → 新 scope/原 callId end；Guided 固定 prepare/select public API 与 [LifecyclePanel](FleetMissionLifecyclePanel.md)在指挥中心、规划页各一处接入，公开12键 metadata/6键组行，不暴露实际事务或其他 raw refs。面板先选动作再明确确认，默认关闭/取消，不用 Enter 提交，原 view ticket 防过期。

prepare 分配新的实际组，不清零旧 Txn generation、不覆盖旧 Series/Run、不搬登录/身份 proof/grant，不自动切 selected、停止旧 Run 或释放其占用。原 all-retained sender/OS/审批/上传/读取 quiet 与8组上限保留；失败尝试也计费，allocated=false 不表示未消耗容量。mutation 已提交与 end 分开，后置失效不能抹掉已发布事实；已知 publication=true 先作本地 token/身份意图协调，不能据协调失败恢复旧授权。

本批不修改原事务 schema/TTL/有限阶段、队列事实或最终 StartPermit 边界。8源码/19文档仍是累计工程增量，`outboundIntegrationComplete=false`、完整25目标/六类缺口保留；限定文本检查记录已同步本轮静态审计，本机未编译、测试、解析/运行 helper、认证、应用或设备，未更新 PR/包。以下366–370及更早是历史，旧“未有入口/专用 transition”说明由本节覆盖。

## 366–370：原实际事务接入外层控制器调用

草稿失效收尾只撤原scope尚存意图，不能核实时私有 `retiredCurrentScope` 持续拒同组current入口/回调，selected观察仍可读，无超时复活；这不是撤回已发命令或结束Run。Qt Int/QObjectStar往返、方法绑定及销毁待实际验证，五源码增量必须连同累计完整工程/pri/注册交接。

[Guided](../../res/Merivus/GuidedActionsController.qml)现从私有 Lifecycle 的当前 Episode 获取14个原生服务引用，30个 Fleet 入口/24个来源回调固定原组；初始事务来自构造已有 initial，不以新建独立旧对象或自动 prepare 回退。外层[controller-call](FleetMissionControllerScope.md)先 capture 原22引用 scope，plain lifetime/finally 只结束原 owner+callId；pure 来源回调不因同步调用/prompt 忙而自拒，嵌套草稿失效会标记 active current 调用 stale。

这不改原事务 schema/TTL/阶段、一次身份移交或 StartPermit；业务仍须逐次原生 full gate。外层返回检查失败不抹真实已 queue/可能提交事实，不把未知视为未发送，也不结束独立 Run。selected 的 Series/Run/Close/Session 四值同 scope，完整 issued Close 收据不按 UUID 搜索或恢复旧授权；显式监视登录可撤未消费批准，不等于停止任务。

新部署/历史选择产品入口及专用 transition 原生 API 仍缺，`outboundIntegrationComplete=false`，完整25目标/六类继续。实际本批5源码/18文档；已完成限定文本审阅，不是运行或安全通过，未编译、测试、运行、更新PR或打包。下方未注册/未迁移说明为历史批次。

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

本轮修改Deployment的普通upload/readback，不修改StartupTransaction三步sender、其RAW依据、StartPermit或正式结果契约；本页后续保留原阶段记录，不能把新普通读取fence当启动许可。

## 346–350：十二类原生服务门禁接线

[Episode服务接线](FleetMissionDeploymentLifecycle.md)已覆盖另外12类服务的新请求、live依据、审批/身份移交及现有scoped发送fence：归组来源须仍为current且owned peer属于同一实际组；旧取消/revoke/drain、被动Run/Close和真实已queue记账不借此封禁。Authority/Txn认证移交上下文改为纯本地比对，完整身份/风险门禁仍在调用两侧；Series保留严格一次未发布构造绑定。

仍未形成新部署可用闭环：普通Deployment上传/回读的legacy PlanManager在插件后、异步每包/重试没有独立Episode fence；Guided actual Episode/epoch、共享Role身份上下文和selected旧观察整组投影尚未接入，无新QML注册/部署入口。只改源码及限定静态审阅，不编译、测试、运行认证/应用/设备、更新PR或打包；原25目标/六类继续。

本事务的commandContextCurrent、请求alive、_gate与实际scoped预queue fence核当前同组。两类authenticationTransferContextCurrent均为pure native，按实际Authority的constructor epoch及normal authorityGeneration/ground generation、UUID/hash/deployment/binding/原成员与原期限比对；外层request及Authority移交前后完整gate保留。发送返回先保存实际issued/uncertain，再检查是否还能继续发送；Close/retire、_stop/_drain/_detachDrain仍核原范围，不因主动授权_same失效吞历史或清理。


## 345：非 Close 用途的原生身份门禁

[Session 身份接线](FleetOperatorSession.md)已加入上传/启动/ground用途的current Episode检查、实际owner配对及OS/owner回调后的再次核对；旧组普通身份事实、显式登录/退出、精确撤销和独立Close用途保留。相同账号/UUID或不同工厂的当前对象不能替代同一实际组。

这是340–344核心后的源码增量，不是全链隔离完成：其余owned发送/审批入口、Guided actual Episode/epoch和selected旧观察投影仍未完整接线；无QML注册/新部署入口，现有应用仍单链。只做源码与限定静态审阅，不编译、测试或运行认证/应用/设备，未更新PR或包，原25目标/六类继续。


## 340–344：独立部署不复位旧事务，应用尚未切换

新增 [Episode / Lifecycle 原生核心](FleetMissionDeploymentLifecycle.md)，每代新建实际初始 Txn 和其余13个 owned 对象（含 signed-out Session）；旧 Txn、Series、Run、generation、绑定及占用保留，不 reset/reuse 或用相同 binding 迁移批准。原事务32键 receipt 等契约未因本批改变。

工厂要求全部retained组的owned原生操作栈静默，直接读取private native状态形成固定18键证据，不调用OS/public current getter。owned/内部owner或共享依据丢失、任一存活原目标native busy均拒绝；已失外部目标源明确保留null/unknown，不用同ID替代，也不清占用/推导落地，但不直接阻止分配不相关任务的新空组。容量8次含初始和失败尝试，不驱逐历史。

本批Txn头增加核心读取friend及private sticky managed/owner来源，Series新child在公开前继承；**native owned outbound helper尚未接入本事务各请求/继续点**。也未注册QML、替换Guided或新增部署入口，应用仍单链。后续须actual Episode/epoch fence覆盖发送及OS/源回调，保留旧精确撤销/drain/观察而不复活发送权。限定检查结果已记录；未编译、测试、执行API/helper/认证/应用或设备。

## 335–339：原生关闭入口的独立UI消费者

[运行监视面板](FleetMissionObservationPanel.md)已接两个固定入口，仅通过controller的原request/approve/cancel消费Close后端，不开放本Txn五个private接口，不发新飞行命令。UI只取实际Series当前Run，不回退初始来源；完整context18、原生owner/占用与原期限仍在最终native检查，界面token/三勾选/倒计时不认证权限或效果。

本批未改C++关闭门禁；view/生成review/另一账号最终确认分离，取消仅本页审阅，released/held/unknown保留。没有运行UI、认证、编译、测试或设备操作；ObservationClosed不是STOP/LAND/DISARM/Completed或可重新分配证明，完整执行/生命周期目标继续。

## 330–334：独立监视结束的 private 原生接口

新增[Close workflow](FleetMissionObservationClose.md)是friend而非QML任意调用者；五个private接口分别捕获稳定范围、读取实际主owner QQ、重新核同一范围、唯一commit和事后释放诊断。没有恢复旧Session/启动许可或调用新飞行命令。

context恰18键：原17键关闭范围另有 `localReservationState`，含 `run` / `startup` 两个snapshot，各恰 `owner` / `occupiedVehicleIds` / `authorityRevoked`。从actual Swarm扫描所有属于原UUID的map项，使用本Run从实际Vehicle冻结的原生Int成员核子集及排序，不把原输入允许的integral Double误作新飞机。两个CurrentlyHeld仅表示实际map非空，不等于有效整组权限；revoked=true也可以held=true，完整列表及撤销变化都使review失效。

前置要求本Txn终态、发送/授权/协议栈静默，本Run真实prepared/startupRetired且未retired，与原startup/epoch/generation/完整binding/有序成员及同Swarm/Orch一致。原Vehicle/MM已丢失可保留unknown，不查同ID替代源；实际Run/Swarm/Orch不能被替换，SourceObjects仅给friend三个真实指针、不公开。Orch退休match不借MODE admission或无关task冲突阻止结束旧monitor。

commit重新核同一context后只调用现有retireExecutionObservation(startupId,true)；Run释放自己已移交的本地占用，Txn排空旧未移交占用，不释放其它owner。事后4bool只在原同源/身份仍能核实且实际map检查成功时提供，empty表示unknown，不是全部释放。无需恢复旧启动权限、实时遥测或原PlanStore才能结束原monitor，但仍必须真实当前两账号独立审批。非STOP/LAND/DISARM/Completed或成员可重新分配证明，源码/限定静态审阅，未编译/测试/执行。

325–329补充：连续Run在首MODE前登记到同实际Swarm关联的protected任务模型；零MODE停止时，_drain先等待_busy/_checking/_sendingIndex退栈及全native协议quiescent，再由实际Run严格核未发出/无uncertain的全部MODE后关闭unused准备。该自动收尾不设置operator请求事实、不回滚ARM/序号；缺源/未知或任何commit都不按未用准备关闭。未退休Run直接阻止地面重审资格，避免旧Draft占用到下一轮末段才使新任务登记失败。真实queue事实、旧组占用移交及后续正式结果/许可边界保持，未编译/运行/测试。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

320–324（2026-10-15）新增[显式 HOME 观测过渡](FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

## 315–319：启动权限结束后保留本次运行观察

每个实际 Transaction 只能接纳一代启动，已经有 startupId/generation 的对象不再复用覆盖；后续地面再审仍由实际 Series 分配独立 child。stage2 首次 MODE 发送前创建并保存实际 Run，再由其私有 `prepareFromStartup(this)` 冻结真实来源。`executionObserverForQueue` 仅当同 actual Vehicle/PRIMARY/startupId/MODE step、当前 sending index/full command context 与 actual Run ready 均匹配时返回 native observer，不接受 caller 替身或 map。core 在实际 write 前独立安装观察槽；真正 write 返回后才保存 captured commit。

`_stop()` 仍结束原阶段和撤销原 Session 发送认证、取消旧启动 lane，但 `startupRetired` 只通知独立观察器保留原终态，不能清其 provisional/已 commit 事实。即使取消发生于 write 栈，晚到 commit 仍由活着的 actual Run 保存。`_drain()` 必须先等 Vehicle/MM 栈真正排空；只要任一机实际 committed MODE，就尝试将原完整整组 local reservation 原子转给 Run UUID。成功后旧 startup UUID 的 release/析构不能释放新 Run；旧 lease 已被安全逃生撤销/丢失时不重建。若 old lease 仍真实拥有但移交失败，则保留它并报告排空/移交未结束，不制造 release/reserve 空窗。

新增 `executionObservation` 是独立 Q_PROPERTY 值及 notify，不把每250ms运行摘要塞进旧 current/Series 历史。native `retireExecutionObservation(startupId,true)` 仅在本次 startup 已终态、无本机发送/核验栈且协议排空时结束监视，并允许清理仍在等待的旧占用；不是 Q_INVOKABLE，不发送停机/降落/模式，也不证明飞行停止。独立 Run 和原占用生命周期的细节见本页顶部模块链接。

原 HOME stop-and-review、完整来源/账号/期限门禁、receipt32、三步指令与审批政策未放宽，没有新增启动按钮或最终 StartPermit。当前 stage2成功仍只叫 started-observed，正式任务结果/可信机载实例/合法HOME有效分支、净空、完整执行时序等原目标未完成。本轮未运行这些 API、编译或测试。

315–319（2026-10-15）新增[本次启动的连续运行观测](FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

310–314（2026-10-15）新增[错误可区分的原生目录清点](FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

305–309 新增[遥测只读历史检索](FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

300–304 新增[原生入站遥测记录器](FleetTelemetryArchive.md)：独立Vehicle pre-plugin被动入口、实际PRIMARY/Vehicle/observer绑定与退休、27消息固定Codec、parentless有界NDJSON worker及native确认/公开只读值已接线。新增默认关闭，不改旧.tlog，不请求数据流或控制；定位类另明确确认，原len payload与RX序号只观察，签名/CRC/身份不认证。文件NewOnly不覆盖或删除，入队/write/flush/durable分层，源失效停止整组并保留unknown尾部；fullTelemetryCaptured始终false，不把此子集当完整遥测或任务许可。运行/编译/测试/解析helper/设备均未执行，未改PX4/刷机/更新PR/打包；原25目标/六类不缩减。下方295–299及更早为历史批次。

295–299 新增[跨封存段只读浏览](FleetPersistentEventStore.md)：Sql唯一schema/身份/严格页抽取、parentless单请求Reader与Store native目录/分页/取消及public值facade已接。actual目录条目生成本catalogId，页域绑定文件/DBepoch/sealRun/filter，最多128个每段单filter游标，只0或实际next；当前页/Reader共单槽，取消与错误不提前复用，历史不回灌rules/Replay/任务/权限。目录128段/4096条目与JSON页有界，合法1MiB政策的内部预留预算已修；目录全集、全库记录hash、跨段原子快照/飞行序都不宣称已验证。仍缺全集索引/自动归档清理、完整遥测/运营集成及最终StartPermit等原六类/25目标；未编译/测试、运行SQL/目录或hash helper、改飞控/刷机、更新PR或打包。下方290–294及更早说明为历史批次。

290–294 新增[显式维护封存轮换](FleetPersistentEventStore.md)：实际worker协作锁贯穿排空/SQL关闭与同目录旧库保留，native确认后才请求，不自动删旧段；actual clean/sealed/finished、精确DBepoch/文件名和同源QQ/epoch核对后才沿本次明确配置建立新run。取消/退出/源丢失只退休续建，不抹掉可能已发生的封存事实；暂停序号窗口显式known/unknown及缺口摘要，不称连续无损。封存段数有界、未知文件不覆盖，hash/协作锁不认证或保证恶意并发/失电安全。仍缺跨段查询/自动归档清理、全遥测/运营集成及最终StartPermit等原六类/25完整目标；未编译/测试/运行库或锁/文件轮换、执行SQL、改飞控/刷机、更新PR或打包。下方285–289及更早记录为历史批次。

285–289 新增[默认关闭的连续诊断归档](FleetPersistentEventStore.md)：实际BlackBox暂时收据/同类型事件身份、固定摘要Codec、专属QtSql线程与Store门面已接pri/type/controller；仅native显式配置/确认后异步开库，默认不读取或写盘，不回录开库前历史。queue+in-flight/批次/主库页/历史结果有界，实际commit后才durable，来源/投影/入队缺口保留诊断；历史单槽直到GUI交付，不进入实时回放、规则、机队、审批或控制链。无原payload/凭据/精确GPS，哈希非签名/匿名化；clean非完整性认证，未知库与需hot-journal恢复库拒绝不重建。仍缺全遥测/长期轮换/运营集成及最终StartPermit等原六类/25完整目标；未编译/运行/测试、建库/SQL执行、改飞控、刷机、更新PR或打包。下方280–284及更早批次为历史记录。

280–284新政策真实消费：工厂不再统一硬编码六字段，只接受schema1/6或schema2/8且与完整normalize/profile、原beforeArm/currentRisk冻结政策完全一致。没有把原六字段补成新政策或续旧审批。原生纯odometryContinuityRequired(actualVehicle,startupId)仅从本actual target/private beforeArm的schema2与motion flags推导；Vehicle在final真实queue前冻结该周期要求，不接受caller bool，不触OS或反调fullgate。追加ODOM初始/逐包/尾gap不改32字段step收据、不增加第七flight风险通道或新的控制命令；原group barrier/queued/uncertain/drain/deadline保持。观察包线只在实际MODE后的原journal阶段应用，最终StartPermit/净空/完成仍未实现。

280–284 新增[显式局部运动与航向观察政策](FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

275–279 新增[被动 ODOMETRY 原生账本与契约](../../../src/Vehicle/FleetStartupOdometry.md)：原始 MAV2 解码50/reference42，Vehicle full27/reference21；实际 PRIMARY/PX4/版本与飞行来源、接收整栈、prepare前计数和token/epoch防嵌套覆盖。sticky everObservedRawFrame 不把曾断流/无效/被过滤的源当初始缺流；Readiness仅存去age的20字段参考投影，普通收包/时间/运动数字/quality分值不造成身份抖动，reset/frame/状态与已观察ABA撤销旧依据；地面READ冻结该投影，Transition前后重核实际来源。稀疏协方差NaN保留未知，扩展尾零/LOCAL_FRD/时钟不认证参考有效性或同采样。不增加传感器、不请求/启用流、不改原六通道/read34/startup32、不放宽HOME/local严格政策。源码接线与有限静态审阅，不是完整运动/转向/净空/最终StartPermit或执行完成；原25目标/六类继续，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方270–274及更早批次是历史记录。

270–274 新增[有界实际启动事务序列](FleetMissionStartupSeries.md)及多轮地面再审后端：最新已初始化owner不回退，逐轮冻结actual QQ/UUID/epoch/generation/hash，旧转交链精确退休后重新完整读取/已阅/两账号审批；每次新消费者保留前代，初始/终态对象不能重用覆盖。native固定历史摘要6防递归复制，完整当轮任务/新风险/真实queued事实仍保留；同一原组/记录/部署，最多8个ground child（空候选也计费），源丢失/容量满/MODE已可能排队/未排空均不淘汰或恢复授权。实际pri/type/controller已接，限定源码复核完成，未编译/运行/测试/API/飞控改动/刷机/PR或打包。容量/新部署重置、HOME有效分支/local frame/运动转向、净空/最终StartPermit/时序/执行完成及原25目标/六类仍待开发。下方265–269及更早批次是历史记录。

本轮新增终态历史的原生有界摘要：Terminal20 的 `previousAuthorityContext` 不再嵌套完整 Authority25/27 与上一轮 Review/Reference 链，而是固定 6 字段摘要；完整当轮实际授权上下文仍保存在 Transaction 内并参与原发送/接管核对。生成失败在替换旧 generation/终态前拒绝，旧授权不复活。显式新地面审阅仍须实际终态、全栈排空、从未 MODE 排队及完整新 RAW/风险，不能把摘要当认证、启动许可或飞行证明；本轮只改源码和本模块说明，未编译、测试、运行 API 或刷机。

265–269 新增[地面再审后的分阶段启动事务](FleetMissionStartupTransaction.md)源码：actual8/context27、一次28→独立30（Session6/Authority10）、同UUID连续占用；独立第二实例不覆盖旧终态。已解锁成员不发ARM，但仍核实际Seq0完整journal与整组屏障；未解锁成员仅普通ARM(param2=0)，之后仅mode-only。首次Seq0前要求实际周期RAW基线/政策gap，错误实例在换generation前拒绝；后端接线且限定源码契约复核完成，无新执行按钮。本机未调用接口/编译/运行/测试、改飞控、刷机或更新PR/包。最终StartPermit/净空/完成、第二事务再次终止后的实际owner重审路线、HOME有效分支/local frame/运动转向及原25目标/六类仍待开发；旧第一终态不能代替第二事务事实。下方260–264及更早批次为历史记录。

260–264 新增[地面再审启动依据一次接管](FleetMissionGroundStartupAuthority.md)源码：实际10来源/原始context27、完整新RAW/全风险/记录/typed当前地面状态进入新owner，旧22一次移交Session独立28（6字段返回真实remaining）、Ledger10字段接管。旧账本退休不盲撤新owner，原最早READ START/两身份期限只缩短，独立实时查身份/WTS/源与整组占用；public不泄露SID/RAW，所有执行/启动/飞行许可false。实际后端/pri/type已接，本批限定源码契约静态收口已完成，无读取/ARM/MODE或UI执行按钮。真正分阶段模式消费者、HOME分支/local frame/运动转向、净空/最终StartPermit/完成及其它六类/原25项目标继续；未编译/运行/测试、调用接口、改飞控、更新PR/源码包/安装包。下方255–259及更早批次为历史记录。

255–259 新增[地面再审独立账号条件意图](FleetMissionGroundStartApproval.md)源码：实际新 Review20/九对象/冻结整组与 armed 状态、完整最新记录/风险/新 RAW，真实两账号新提示和 Session 独立22证明，一次消费/撤销及 native14 grant；public27 receipt 无 SID/RAW。最早 READ START 原30秒不续，旧证明不恢复，实际提示期间主身份仍查 SID/role/WTS/旧期限；active READ与消费不放宽。实际 backend/pri/内部类型已接，本批已完成限定源码契约静态收口，没有新增执行按钮/新 startup authority/MODE续作。最终许可/净空/HOME有效分支/local运动转向/完成及其它六类/原25目标继续；未编译/运行/测试或调用接口、改飞控、更新PR/源码包/安装包。下方250–254及更早批次为历史记录。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

本模块接管一次真实 `FleetMissionStartupAuthority` 的原始身份/任务/物理来源依据及同一 `startupId` 的整组本地占用，然后执行固定三步。不是任意 MAVLink 指令入口，不调用旧 `startMission`、`MISSION_START`，不允许 force arm、自动重试或失败回滚。本批没有执行软件、认证窗口、仿真、编译、测试、刷机或真实飞行。

## 接入与权威来源

### 新地面再审入口（265–269）

`requestGroundStartup(actualSources8,startupId,true)` 只接受实际 GroundStartupAuthority/context27；原 `requestStartup` 仍只接受原 Authority25。两条工厂在任何 generation/目标替换前先核 private root 的实际消费者实例与来源，错误实例不破坏旧终态。控制器使用第二个实际 Transaction，第一事务仍是本次新审阅的历史来源。

新工厂按原 typed frozenArmed 逐机冻结初始状态。Authority 中去 age 的 newReadReceipt33 不能补假 age：必须再从 actual nativeGroundReview/MM 取真实34、available/fresh 且稳定部分一致。移交后按中立 retained RAW/cache/source 验证，不依赖旧 Review.valid，原未解锁/地面读工厂不放宽。

Session 从独立28一次移交独立30（旧24仓不覆盖），Session6含所有核对后的真实remaining、Authority10保留同UUID占用。pendingContext27/Transaction回调只核实际冻结内存；root/full源/身份在移交前后与每次实际发送围栏独立核对，原最早READ START和两身份截止只缩短。

新已解锁成员只有 Seq0/ModeOnly 两份真实native receipt：全组ARM屏障仍重验其 grounded armed/POSCTL 与持续Seq0风险历史，`queued[1]=false/requestIssued=false`，明确 skipped 诊断不造ACK。未解锁成员仍逐机发 normal ARM(param2=0)，原全部目标通过屏障后才后续tick发mode-only；没有子集降级、隐式retry、force或自动DISARM。

历史265–269限制：当时尚无第二事务终止后的多轮新取证。270–274 已由[实际 StartupSeries](FleetMissionStartupSeries.md)补齐最新 owner 选择及最多8个独立 ground child 的重新取证/审批，不允许回退旧终态；但任何 MODE 已排队/不确定后仍禁止沿地面路线再审，容量/新部署重置也未完成。HOME再次变化仍按实际政策停止，不算自动恢复；完整local frame/运动转向、净空/最终StartPermit、忠实执行时序、启动界面与完成归属仍未完成。

`requestStartup(actualSources, startupId, operatorConfirmed)` 要求 exact8 个实际 QObject：`session/roles/store/authority/deployment/swarm/health/authorizationOwner`。每个强类型对象和线程都检查；传入快照或同字段假对象不能替代实际源。私有 owner 须严格返回实际 Bool：`fleetStartupTransactionSourcesMatch(actualTransaction, bundle)` 与 `fleetStartupTransactionContextAllowed(actualTransaction, startupId, binding, orderedVehicleIds)`。

Admission 读取 actual Authority25、完整非恢复且最新 Store 记录字节、完整 payload/机载 RAW read34、原 reviewed 风险、主 SID/角色、真实 Catalog/Coordinator/Registry/FaultPolicy 与所有 Vehicle/MM/PM/VLM/PRIMARY 指针。上限 32 机、每机 2048 项、整组 8192 项。原任务必须已有完整、经原审阅冻结的 `startupTransitionPolicy`：schema、HOME `stop-and-review`、7 项 post-mode 活动包络、保留 reviewed heading、实际 RAW 最大间隔和历史阶段时长；缺失不能补默认，必须重新规划/读回/审阅/独立审批。

一次 native Authority → Session transaction24 身份接管后，旧 Authority/Review/Content/Approval 的 `valid` 不再成为过渡门禁；真实身份/角色、记录、参数、版本、Catalog、缓存、来源对象、链路、health/Leader 与原 RAW 依据仍逐次检查。整组 reservation 保留原 UUID，无释放再获取空窗；原身份、Review、READ START 最早的 ≤30 秒窗口仅缩短。跨不同单调时钟只传原剩余预算，并保守扣验证耗时，不比较绝对时间或续期。

## 三步与屏障

1. 全组逐机排队 `MISSION_SET_CURRENT` wire seq=0。无 ACK，要求实际新 RAW CURRENT receipt 超过该原生请求 baseline 且序号为 0、PRIMARY/来源不变。
2. 全组逐机排队 normal ARM：`MAV_CMD_COMPONENT_ARM_DISARM` param1=1、param2=0。要求新 RAW armed、仍 on-ground/POSCTL；ACK 本身不是阶段成功。
3. 全组逐机排队仅模式：`MAV_CMD_DO_SET_MODE` param1=1、param2=4、param3=4。要求新 RAW armed AUTO_MISSION；不使用可能隐式 ARM 的 mission-start adapter。

每一步只发一次，每 ≥100ms 至多一机一步；全部目标满足当前阶段新 RAW 后才在后续 tick 跨全组屏障。每阶段至多 6 秒且不得超过原总期限；native 单 sender lane 自己也有 4 秒窗口。32 机/慢链路不保证能在窗口完成，超限停止，不放大预算、不隐式重试。factory/handoff、原生 receive/finish 栈和 cache 信号不直接启动下一步。

实际 Vehicle 每机最多保留 3 个 schema3/33-field request scope（381–385升级），单 sender lane；queueEstimatorObservation为实际EST稳定11，RAW pre-plugin heartbeat/current/ACK与EST原包、真实queue基线、连续risk journal、实际RAW gap/history时长供typed `FleetMissionStartupTransition`核对。关闭sender lane不停止journal；只有下一步真实queue或显式终止才封尾，因此屏障等待中的ABA/HOME/EST风险不能由准备下一slot的空窗丢失。收包栈（含health）未退出只有限等待，不排新包。每次发送前、outgoing plugin后、最终queue前均核固定actual source/generation/phase/deadline的mandatory by-value fence，最后callback后另核纯native EST来源/语义/原gap事实。

已真实 MODE queue 的机只允许 typed helper 已有原 reviewed post-mode 活动包络，未 MODE queue 的机仍要求 ground 严格门禁；未知/回退/未解释的 risk revision、journal 缺失/耗尽或参数/版本/链路变化停止。本阶段没有证明 LOCAL frame/reset/参考有效性，实际 local XYZ 变化保持保守停止，不能声称完整动态启动运动支持。所有实际 HOME 变化本批 sticky 拒绝：LOCAL/HOME 数值吻合不证明 PX4 内部 `z_valid/home.valid_alt` 分支，不能算术重基线绕过审批。HOME 合法更新后的完整重新审阅及 local-motion 参考策略仍属后续工作。

## 取消与结果解释

### 多轮地面审阅的有界历史引用

工厂从实际 Authority 的原生 getter 取得完整 context，并通过其 `authorityContextCurrent` 核对后、换 generation 前生成 `_nativeAuthorityReference`；在保存完整 `_authorityContext` 的同一次 commit 冻结该摘要。不是从 caller 的 JSON、旧终态投影或归档恢复生成，也不在终态查询期间调用旧 Authority.current/认证。

`previousAuthorityContext` 保留该固定 6 字段：`schemaVersion=1`、实际 `authorityEpoch`、原 `sourceHash`、`sourceContextHash`、`scope`、`previousAuthorizationRestored=false`。`sourceContextHash` 是本次完整实际 admission context **仅删除外层 `remainingLifetimeMs`** 后的紧凑 JSON SHA-256；其它身份、完整风险、原始证明与嵌套原生来源内容都参与哈希，没有由摘要重新派生的剩余期限。`scope` 由本实际工厂入口确定为 `pre-arm-authority-reference-not-permission` 或 `ground-reassessment-authority-reference-not-permission`。原编码资源限制继续适用，编码/形状失败不能换掉旧终态来源。

Terminal20 的其它字段及完整 `deploymentContext`、`binding`、整组 ordered IDs、每机 `member/payload/previousReadReceipt`、真实 `previousSteps`、当前完整 RAW 风险、health/Catalog、事务 epoch/generation/sourceHash/phaseFingerprint 都保留。摘要只压缩上一轮授权历史，不能删掉新取证、审批或发送所需风险，不能证明 MAVLink 真实性、PX4 内部 HOME/local-frame 有效性或任务执行。下一轮 Authority 即使引用本 Terminal20，也只增加有界当前轮依据，不递归复制所有旧轮授权树。

显式 `terminalReassessmentContext` 仍按现有终态/排空/从未 MODE queue 规则获取**新的实际**风险并保存为新 Review 的原生引用；允许原 Review 过期或 HOME 稳定后重新取证，不永久锁死第一捕获。`terminalReassessmentContextCurrent` 的比较捕获 (`retainReference=false`) 绝不覆盖原已保存引用；`terminalReassessmentSourcesRetained` 同时要求传入上下文等于实际保存引用及实际 Review 自己冻结的引用，后续显式新 capture 不能继续沿用旧 Review 的来源。两条路径均不续旧授权、旧预算或恢复发包权限。

`cancel(startupId)` / native `revokeStartup(startupId)` 只精确撤销当前 scope 的后续发送和认证；不自动 DISARM、不改回模式、不清任务、不回滚，已 queue 的包仍可能生效。部分 seq0 写和 ARM/MODE 已发/新 RAW 已见/仍不确定分别保留。terminal/destruction 在 native send/RAW/MM 栈排空后才释放同 UUID 本地占用；独立 drain 只查实际栈状态，绝不恢复认证、发包或重试。

成功工厂返回 `ok/requestAccepted=true` 只表示接收了有界异步事务，不表示已发 ARM/MODE 或动作成功；拒绝不得保留可继续排队的 active 请求。`current` 是 `snapshotOnly/revalidationRequired` 的有界状态，不是权限证明。`startupPermitGranted` 始终 false；`scopedDispatchAuthorizationHeld` 仅表示尚持有本事务有限来源依据，真发送仍需每次完整原生门禁。seq0 只属 `missionStateMayHaveChanged/commandsMayHaveCommitted`，`flightCommandReleased` 仅 normal ARM/MODE 的实际 queue 事实。

`started-observed` 只表示全组观察到本轮 scope 后新 RAW seq0、grounded normal ARM、armed AUTO_MISSION 模式；不是指令响应因果认证、任务实际执行、到达/完成、硬件认证或飞行放行。`requestCausalityVerified/authenticatedAircraft/executionVerified/missionCompleted` 仍 false。normal ARM 条件不证明 neutral RC、桨区净空、机载 can-arm 或城市巡逻安全认证，工程师仍须实际编译、协议/仿真和硬件验收。

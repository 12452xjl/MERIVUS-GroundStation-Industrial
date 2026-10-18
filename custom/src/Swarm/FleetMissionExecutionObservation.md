# 本次原生 MODE queue 的连续运行观测

## 541 当前：保留实际任务模式退出候选

正式 post-commit HEARTBEAT 在原 actual Vehicle/PRIMARY/default component/epoch、严格本地序号及有效 PX4 帧门之后，只有前一有效模式为 AUTO_MISSION、本包有效模式非 AUTO_MISSION 才累加原 `missionModeDepartures`。另保最新有效任务模式 receipt，不改已有 first-ever `missionModeReceipt`；初始非任务模式、provisional 样本和连续非任务模式均不制造退出。重新进入任务模式再退出才增加计数/末次，首次记录不变。

每机至多保存首次、末次两个模式/状态原值及各自 prior/exit receipt；customMode 完整 UInt32，baseMode UInt8，systemStatus 有效 MAV_STATE，continuityUnknownAtExit 保当时缺口未知。receipt 沿原同 epoch/正 UInt64 sourceSequence/非负单调接收时间；这不是飞机采样时间、响应因果、物理连续性、退出原因或远端任务身份。源退休、独立 Close 与析构保旧值且未知尾不消失，不用候选推停飞、完成或释放预约，也不按每帧发送审计。

current 根及 TaskModelEvidence.observation 根升级 schemaVersion=2；每机 observedOutcome 升级 schemaVersion=2 并只新增 `missionModeExitObservation`，原 row 字段集合及终点/无限 HOLD/落地 outcomeCandidate 不变，组摘要仍 schema1。新候选是独立接收事实，不覆盖原终点候选。其 exact9/exit6/ref3 由纯值 [FleetMissionModeExitObservation](FleetMissionModeExitObservation.md) 统一闭验；count 必须强等于原行 missionModeDepartures，首末至多两个，不建立无限帧历史。

requestCausalityVerified/remoteOutcomeVerified/automaticActionsIssued 均 false；全部原认证/完成/权限 false 保持。旧报告/持久字段的严格兼容由相应版本合同处理，不能从 public map、历史文件或诊断字段反向安装本机 native 观察。工程师需覆盖初始非任务、mission→nonmission、连续 nonmission、再次进入/退出、provisional 与 commit 交错、缺口、计数/序号极值、源退休/Close/析构保留及旧新报告分支。本批仅源码/静读；未编译、测试、执行校验器/业务/解析器/应用或操作硬件。

## 524 当前：begin后未write的Run留给独立Close

`retireUnissuedPreparation` 现在只有逐目标明确从未queueBegan、从未queueCommitted、实际Txn MODE queued/uncertain均false且原完整receipt/transport/上下文仍核实，才自动关闭准备。Vehicle可在begin已安装后、真正write前的后续围栏失败；即使requestIssued=false，也不能抹掉begin或先自动把Run置retired，再让严格归档例外和原人工Close同时不可达。

任一queueBegan为true时helper返回false，不改Run退休/绑定/事实。实际Txn `_drain` 只在helper真实true才置 `_executionObservationRetired`，false且无MODE commit时按既有逻辑排空原startup预约、保留原observer和timer/TaskRecord，不destroy或伪关闭。terminal Tx在原busy/checking/sending栈退出后，原 `observationCloseContext` 仍可取得这个prepared、startupRetired但未retired的已登记Run；原完整两账号独立Close流程才可真正关闭。无TaskRecord的注册失败never-began支路不变；不恢复旧授权、发包、推断物理停飞或制造Completed。

本窄修仅为[524旧组归档退休](FleetMissionDeploymentLifecycle.md)保留原合法退出路线，不放宽其never-began条件、全owner占用或完整原生尾核。仅静读源码链和普通文本核对，未编译、测试、执行项目/helper/解析器/业务/API/界面、操作PX4/硬件或放行。下方旧批次为历史。

## 511 当前：发送前先提交真实 queueBegan 到保护记录

`beginExecutionQueue` 沿原 ready／完整 queue 身份检查安装 `queue/context + queueBegan/sourceActive/tracking` 后，不再直接返回 true。先复制本次实际 Target、离开所有内部 Target 引用作用域，冻结安装后的 ReadContext，再立即走同一个 Orchestrator 的受保护更新。只有模型接纳成功、self 与原上下文／serial 仍同、原实际对象／queue 可重新定位、尚未 retired／startupRetired／耗尽／queueCommitted 且仍 tracking/sourceActive 时才 true。

这补上 [510 低频事件链](SwarmMissionOrchestrator.md) 的历史局限：510只在 TaskRecord 真正提交时冻结语义，旧 begin 自身不提交，正常 began／commit 常合并到后一次模型更新；本批在实际 write 之前增加真实的 began 模型检查点。它不是捏造一份远端 ACK、已发送或 MODE commit；begin 已安装但后续拒绝仍保留原事实，不 reset、rollback 或抹为“从未准备”。不同 native 提交仍可被异步合并通知，但各自的语义事件已在实际提交点冻结。

private `updateTaskModel()` 现在返回 bool：原实际 Orchestrator 的 protected 更新必须成功，且原 ReadContext 在全过程仍匹配才 true。缺模型、错误线程、owner／source／serial变化或原保护 CAS 拒绝均 false；原失败原因诊断仍保留，不自动重试或续期。没有新增公开 setter、报告 schema、wire、账号权限、完成状态或占用释放。

**完整 Evidence 不是无回调的纯内存读取。** 它会通过 nativeTargetCurrent／nativeBindingCurrent 调用 Link 虚函数，可能嵌套 RAW、退休、取消、推进同 Run 或销毁对象。因此新 begin 返回后先查 QPointer／原 ReadContext，再访问或定位目标，不能跨更新持内部行引用；通知只由原模型／510异步通道排队，不从这里直接 emit、写文件或发控制。Link 自身或嵌套事件循环仍可能触发回调，不能宣称本栈绝无外部执行。

Vehicle 原 caller 在 begin 返回后仍按同 observer、slot revision／source epoch、PRIMARY 和 QQ 复核，并继续原 BindingMatches／授权／immutable 围栏；真正 `writeBytesThreadSafe` 之后仍从捕获的原 QueueContext 记录 `executionQueueCommitted`。commit 先保已进入实际 write 的事实，之后更新模型；模型更新失败、源退休、嵌套取消／销毁或后续 false 不能撤回该事实。原 commit、source 退休、startup 退休、adopt、显式／未发送准备关闭、poll 和析构调用可以忽略新的 bool，其原清理和事实保留语义不改。

这里新增的是本地受保护模型检查点，不是最终启动许可、实际网络写入完成、远端任务实例或正式执行结果认证。工程师仍需验证：正常独立 began→write→commit；源虚调时嵌套 RAW／serial变化；同栈模型更新、取消／退休；删 Observer／Vehicle／Orchestrator；begin已安装但发送前拒绝；write内晚 commit；修订耗尽；异步事件队列溢出等。所有这些场景本批均未执行。本模块涉及h／cc及本说明三个既有文件；整批为2源码＋13文档＝15既有路径，见[修改清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。仅直接静读／普通文本核对，未编译、运行 helper／解析器／业务／模型／QML／认证／SQL／hash 实现、测试、仿真、设备操作、改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余不缩减。

## 492 当前：source外调、组快照与历史事实的生命周期

构造期一次pin原app／Toolbox／MVM，失效后不从残留raw重建QQ或换源。原nativeTargetCurrent直接三次Link虚调且各caller持成员Target引用；外调可能同步销毁／退休或推进本Run。现保原live语义，source及Binding两层探测使用owned目标、QQ／原Run／变更代次及共同重入保护；每次外调后先核活性／同原上下文，再访问this或重新定位原vehicle＋queue身份。RAW／poll的false分支同样不能直接invalidate已失效引用。

prepare先冻结owner全部targets，注册返回值暂存本地并在原QQ／代次核对后采用。ready／begin、RAW／source退休、poll、startup退休、adopt／release、detach和关闭链均覆盖私有变更代次与二级调用QQ；updateTaskModel返回后也不能假定本对象仍活。detach清理按原Run及每行完整queue身份逐机进行，不因合法sameQueue commit推进serial而漏清后续旧slot，也不回写旧快照或误退新绑定。const current／TaskModelEvidence冻结整组原targets和值元数据，回调后核同原代次，不以当时新值拼接旧snapshot的行／summary。

代次不回绕；耗尽禁止新的可用source探测／状态准入，但历史投影不能因此删去已捕获的sameQueue实际MODE commit。commit先记真实事实，再更新保护记录；源结束、观察退休或write内嵌套变化不能改称未发送、回滚或任务完成。原记录／候选／引用／未知尾与公开shape保持，无自动控制、完成或放行。

[Orchestrator](SwarmMissionOrchestrator.md)同样不跨Evidence外调持TaskRecord*，原TaskRecord真正推进时旧外层结果不能覆盖它。参见[Vehicle lane](../../../src/Vehicle/VehicleMissionExecutionObserver.md)、[Txn](FleetMissionStartupTransaction.md)和[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：已知任务序号回退后结束单机关联

受控payload只允许speed／waypoint／takeoff／hold／land，没有jump指令。正式post-commit `MISSION_CURRENT` 的合法seq若小于该target已有 `currentSequence`，不再只累计诊断后继续关联；先保本帧 `missionCurrentReceipt` 与完整 `reportedMission`，安全增加 `currentSequenceRegressions`，再按固定 `execution-mission-current-regressed-correlation-ended` 原因调用既有invalidate并返回。

invalidate立即置 `tracking=false / continuityUnknown=true`。后续原始包在既有tracking门返回，不能以回退前的terminal／airborne／missionMode引用形成新的REACHED／落地候选。原 `currentSequence/currentSeen` 仍是此前已关联的历史；原报告中的回退seq独立保留，不被新“进度”覆盖。初始−1、重复同seq与合法前进不触发回退；计数耗尽仍沿原安全advance路径，不回绕。

不清空旧receipt或已观察候选、不撤销真实queue commit、不停止其他target、不自动发STOP／LAND／DISARM或释放Swarm占用。sourceCurrent的直接Run投影立即为false；Orchestrator已缓存摘要仍按原250ms poll合并更新，poll解绑实际旧epoch并排原通知，不在接收栈同步公共emit。不认证回退原因、远端任务实例或正式结果，不以安全停止关联代替完整执行闭环。

单机26／整组20等公开shape保持。[本批32项交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)含12项运行观察场景，全部未执行；没有编译、运行helper／模型／业务／应用、测试、仿真、改PX4、硬件／刷机／放行、提交、更新PR或打包。下方为历史，不覆盖本节行为。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380 当前态：同一 Run / TaskRecord 的有界观测结果

本批直接扩展既有 `Target`、`current()` 和原生 `TaskModelEvidence`，没有另建结果占位类、第二套任务模型或 QML 可写结果入口。实际 Run 的 `current().observedOutcome` 和每个 `vehicles[].observedOutcome` 分别提供整组与单机值；同样的私有 `outcomeSummary()` / `targetOutcome()` 也产生进入 [Orchestrator](SwarmMissionOrchestrator.md) 的投影。Orchestrator 顶层 `observedOutcome` 直接读取同一受保护 TaskRecord，不能由 caller JSON、普通 progress 或同 ID 新对象恢复。

这是可解释的**本地接收观察结果**，不是远端任务实例认证、完成证明或飞行许可。所有 `resultVerified / missionCompleted / executionVerified / remoteMissionInstanceVerified` 继续为 false；`automaticOutcomeActionsIssued=false` 仅表示本结果模型不触发自动动作，不否认已经保存的 MODE queue 事实。不存在 generic complete/fail、自动 DISARM/LAND/RTL、直接/强制释放 Run/Swarm 占用或凭落地候选恢复权限的路径。

### 两个轴与固定字段

单机 `observedOutcome` 恰 26 键：

- `schemaVersion, basis, outcomeCandidate, observationWindow`；
- `modeQueueBegan, modeQueueCommitted, sourceCurrent, continuityUnknown, lastCommand`；
- `missionModeBeforeTerminalObserved, terminalCurrentObserved, terminalReachedObserved, terminalReachedAfterCurrentObserved, nonGroundBeforeTerminalObserved, landingAfterTerminalObserved, landedAfterTerminalObserved`；
- `reportedMission, receiptReferences, sourceRetirement, candidateIsHistoricalObservation`；
- `currentValuesFreshnessVerified, remoteMissionInstanceVerified, resultVerified, missionCompleted, executionVerified, automaticOutcomeActionsIssued`。

`basis` 固定为 `native-local-run-post-commit-reception-order-only`。`outcomeCandidate` 仅为 `no-terminal-candidate-observed`、`terminal-item-observed-not-completion`、`indefinite-terminal-hold-observed`、`landed-after-terminal-observed-not-completion` 之一。`observationWindow` 独立表示 `awaiting-native-mode-queue`、`provisional-native-queue-window`、`committed-native-source-bound`、`source-ended-or-unknown` 或 `observation-retired`。来源断开或监视退休只改变窗口和来源状态，不抹掉历史候选及其接收引用；`sourceCurrent` 仍不等于样本 fresh、物理连续性或候选已验证。原有 `phase` 等字段保留兼容，新两轴不把来源未知重新解释成成功。

整组 `observedOutcome` 恰 20 键：`schemaVersion, basis, modeQueueCoverage, targetVehicleCount, modeQueueBeganVehicleCount, modeCommittedVehicleCount, uncommittedVehicleCount, noTerminalCandidateVehicleCount, terminalItemCandidateVehicleCount, indefiniteHoldCandidateVehicleCount, landedAfterTerminalCandidateVehicleCount, missionModeBeforeTerminalVehicleCount, terminalReachedAfterCurrentVehicleCount, observerRetired, unknownTail, remoteMissionInstanceVerified, resultVerified, missionCompleted, executionVerified, automaticOutcomeActionsIssued`。

`modeQueueCoverage` 区分 `no-targets-prepared / none-committed / partial-committed / all-committed`。**none-committed 只是尚未记录 commit，不是未发送或已回滚证明**；begin 到 captured commit 之间仍可能存在真实写栈或不确定事实。四个互斥候选计数之和保持原目标数，部分 MODE 与未提交成员不会被移出分母；这些只是候选数量，绝不汇总成整组成功、百分比或净空。

### 十一个固定 post-commit 引用

每个 Target 只增加固定 11 个 `ReceiptRef`：`missionMode, missionCurrent, missionItemReached, terminalCurrent, terminalAfterMissionMode, terminalReached, terminalReachedAfterCurrent, airborneState, terminalAfterAirborne, landingAfterTerminal, landedAfterTerminal`。每个公开引用要么 null，要么恰为 `{sourceEpoch, sourceSequence, receivedMonotonicMs}`；序号和时间均为规范十进制字符串。它们必须属于本次实际 sourceEpoch，序号大于本次 commit baseline、不超出已经收到的原生序号，时间也只能来自该 core 接收单调钟。它们不是 PX4 sample timestamp、跨流对时、请求因果、原包签名或远端任务 token。

`missionCurrent` / `missionItemReached` 保留最后实际报告，其余是固定顺序条件首次满足的引用；不会积累无界事件列表。重复 reached 仍只作为重复接收计数和有界接收引用，不能增加成功航点数。`airborneState` 沿用已知非 ON_GROUND 状态观察，包含 TAKEOFF/LANDING，不认证真实离地。LAND 候选保留原来的本 Run 非 UNDEFINED 非地面→后续 terminal-current→再后续 landing/on-ground 顺序，不把到点或 Mission 模式单独当完成。

所有新增引用均在原有 provisional 分支返回之后才写入。write 前和嵌套 write 中的 provisional current/reached/mode/landed 历史仍单独存放，后到的 captured commit 不回填为正式证据。即使退休后才收到真实 commit，也只补 queue 历史，不重开窗口、续租或制造结果。

### 原始任务报告与负面证据

`reportedMission` 恰 11 键：`observed, sequence, total, missionState, missionMode, totalKnown, missionStateKnown, missionModeKnown, noMissionReported, totalMatchesExpected, completeStateReportedNotVerified`。无正式报告时值为 null、known 为 false；协议的 total=0、mission_state=UNKNOWN、mission_mode=0 仍表示 unknown/unsupported，不由零扩展或缓存填成 known。即使原包报告 COMPLETE，也只设置 `completeStateReportedNotVerified`，不认证其属于本次部署。

合法枚举中的 `total=UINT16_MAX` 或 `MISSION_STATE_NO_MISSION` 作为负面报告先保留；已知 total 不匹配或 seq 超出冻结任务范围也先保存原报告，再按相关性失效路径停止跟踪并保留具体原因。不会因此索引越界、删除先前历史候选，或把 no-mission/范围不配当任务完成。

### 原生来源退休与同记录尾部

`sourceRetirement` 恰 5 键：`observed, code, codeOrigin, sourceEpoch, lastSourceSequence`。只允许当前 Vehicle 执行观察 lane 已定义的 18 个固定退休 code 原样进入投影，`codeOrigin=vehicle-native-fixed-code`；未识别 code 使用固定 `execution-source-retired-code-unrecognized` 与 `unknown-native-code`，不传播任意文本或冒充已知原因。没有事件时标为 `not-observed`，不补造原生退休事件。

迟到的旧 epoch 原生退休通知，在 self / 原 target epoch 仍匹配时通过既有私有 `updateTaskModel()` 保存到同一记录；即使显式监视退休已停止 250ms timer，也不会只因缺少下次 poll 而漏掉该已知原因。该调用只排已有通知，不新增同步公共 emit、控制、自动关闭或释放。QObject owner 销毁的强制闭合继续保留候选/引用，将实际来源与窗口置为 unknown/retired，不转成普通可写任务。

### 范围与尚未完成的证据

每机固定 11 个引用，最多 32 机；投影没有坐标、原 payload、账号凭据、原始收据或每次 poll 自增的 hostQuietMs。复用既有 observationRevision、资源界限和 queued 通知；最后 current 引用只因真实原包到达改变，不是时钟轮询续期。原 128 native 记录、总任务容量和长期授权清理限制不变。

同期 Vehicle 新增 PRIMARY 原始 SYS_STATUS / ESTIMATOR_STATUS 的被动健康收据，Swarm 使用其真实本地 age / 来源 / known 状态，而不是以反复读取显示缓存续期；它不改变本结果模型的完成边界。**完整 EST 逐包阶段 gap/连续性、最终 StartPermit、远端任务实例与可信完成协议仍未实现**。本地固件仅发送序号的事实不能由地面站补成 instance_count/finished/failure 或本次 Run token。

376–380 的累计增量涉及 7 个源码路径、19 份文档；这是源码/文档路径范围，不是原 25 项开发目标的完成比例。原六类剩余任务仍保留。延续 371–375 已接线的实际部署/历史选择来源，本批不新增 UI 或业务入口；没有编译、moc、测试、解析器、helper/API/认证/应用/SQL/设备/仿真/刷机执行，也没有更新 PR 或打包。下面早期批次内容仅作为历史记录，其中“尚未注册/尚无跨代入口”等旧状态不能当作本节当前态。

## 历史记录：340–344 保留实际 Run 的原生部署核心，尚无跨代界面

[Episode / Lifecycle](FleetMissionDeploymentLifecycle.md)保留旧完整实际对象、Run、未知尾部与占用；每代另建14个 owned 对象及 signed-out Session，共享五个冻结 actual 来源。新组构造不要求旧 monitor 关闭，不 reset Run/source 或把未知变成落地；不重叠成员仍须现有 Swarm/Orchestrator 门禁。

工厂只用全部 retained 固定18键 native 静默证据，保留真实 Run 与原 owner/full binding/成员关系；8次分配含初始/失败尝试，不驱逐旧历史。独立 selectedObservationEpisode 不因发布新 current 自动改选，实际 Close 指针只能定位所属保留组，不恢复旧飞行发送权。

**本批未注册 QML、未替换 Guided、无新部署/历史组选择入口，现有应用仍单链。** sticky outbound helper 尚待 owned 门禁接入，selected Run/Series/Close/Session 同组投影及每次 UI/OS 回调的 actual Episode/epoch 冻结也未完成。335–339 原监视关闭面板不因新核心自动具备跨代选择；ObservationClosed 仍非 Completed/STOP/LAND/DISARM。限定检查结果已记录；未编译、测试、执行 API/helper/认证/应用或设备。

## 335–339：独立监视入口已接线，UI 不构成执行权威

[FleetMissionObservationPanel](FleetMissionObservationPanel.md) 已在 CommandCenter 左栏及 Planner 标题下、主草稿滚动区之外各接一个固定入口；不依赖草稿/预览有效性、upload 权限、当前选机或飞机在线状态，原有父界面的尺寸/折叠规则保留。没有可操作来源时仍显示未知原因，而不是把旧 Run 变成当前 Run。Guided 只从实际 `StartupSeries.previousStartup` 读取 Run，来源丢失返回空值，绝不 fallback 到初始 Transaction 或同 ID 的替代对象。

每个面板单独持有 controller、lifetime epoch 和本页冻结 review token（reviewId/workflowEpoch/generation/完整 context）。它只申请、批准或取消由自己同一 controller 建立且仍精确匹配的审阅；另一个入口的当前审阅不能被接管或取消。关闭/隐藏/销毁面板、切换 controller 会使本页点击范围失效，并仅尝试取消本页仍匹配的 token；这不是结束监视、停止飞机或回滚已提交动作。

审阅默认三项全部未勾，controller/token 变更或本页审阅匹配失效后清空；弹窗默认焦点是关闭/取消，无默认接受，危险批准按钮禁止 Enter/Return 触发。真正批准仍须三项独立确认和原生另一个账号认证；面板打开和状态读取不认证、不申请许可、不退休 Run。UI 倒计时只是保守显示，不核实来源、不验证账号、不延长原 30 秒期限；context18、完整 binding7/9、真实占用、排空及一次消费依然以原生后端为准。

结果区继续区分已知释放、已知仍占用和 unknown/null。`ObservationClosed` 不是 `Completed`，不发 STOP/LAND/DISARM；UI 不能恢复旧批准、认证来源连续性或确认停飞。335–339 覆盖下节330–334“无操作UI”的历史状态，但本批未编译、测试、执行脚本/认证/业务 API/helper、启动应用或渲染；真实结果归因和长期生命周期仍未完成。

## 330–334：独立两账号结束本地监视（后端已接线，未验证）

新增独立 [FleetMissionObservationClose](FleetMissionObservationClose.md) 后端流程，以 `task.observation.close` 权限和 `task.observation.close.approve` 精确审批 purpose 结束本次实际 Run 的本地监视/本地占用，不复用 upload/start/ground 批准。当前真实主账号、当前角色政策及不同 SID 的合格审批账号都须通过原生核验，保留 `requiresSecondApproval=true`。最初 request 的固定 30 秒单调预算包含人工审阅、全部来源检查和 Windows 凭据窗口，任何重试/回调不续期；最终必须分别确认“结束监视”“释放本次本地占用”“飞机仍可能继续飞行”。Session 私有一次消费只烧待审批身份，既不生成长期 lease/proof，也不授权飞行。

对象只能从实际 `StartupSeries.previousStartup()` 取得当前 Transaction，再取得其实际 Run；没有初始 Transaction fallback、同 ID 查找或 JSON 恢复。流程冻结 Session/RolePolicy/Series/Transaction/authorization owner 及实际 Run/Swarm/Orchestrator 的 QPointer 与同线程身份，来源车辆/遥测未知不阻止结束自己实际 owner 绑定的监视，但不能换成同 ID 新对象或恢复来源证明。实际 Txn 已终态、发送/授权回调及 native transport 栈排空是独立前置条件。

私有 context 恰好 18 键，冻结原 startup/run/task UUID、Transaction epoch/generation、deployment、完整 7/9 键 binding、原有序整组成员、固定目标诊断及本次释放范围；不是只比较 wire hash。`localReservationState` 的 `run` / `startup` 两项各恰为 `{owner, occupiedVehicleIds, authorityRevoked}`，从实际 Swarm owner map 扫描对应 UUID 的全部占用，ID 为原组子集、升序唯一真实 Int，越出原组或两 owner 重叠均拒绝。该完整快照进入审阅与审批 hash；`authorityRevoked=true` 不等于占用 absent，也不授予可转移/可执行权限。`runReservationCurrentlyHeld` / `startupReservationCurrentlyHeld` 只表示对应实际 map 是否非空，不能用 `missionDeploymentOwns=false` 推断已释放。

最终流程只沿实际 Transaction 的 `commitObservationClose` 调用原生退休/排空：Run 释放自己的 run UUID，实际旧 startup 占用仍由其原 owner 排空处理，不能释放另一个 owner。后置 disposition 用实际 map 再核两个 UUID；缺失、对象销毁或形状错误保持 unknown/null，不假称释放成功。即便本地 map 已确认为空，也不证明飞机停飞、远端控制已交接或成员已适合重分配。任务模型只转 `ObservationClosed`，绝不转 `Completed`，也不发送 STOP、LAND、DISARM。

Orchestrator 的退休匹配现在独立于 MODE queue admission：来源失效、其它任务冲突或观测计数耗尽不能被误作新发包许可，也不会仅因此禁止真正 owner 退休。`observationRevision` 达最大值时仍允许必须的原生闭合，保留最大值与 exhausted 标记，不回绕、不制造新唯一观测版本或真实成功结果。

330–334当时仅后端源码接线和限定文本复读，没有操作 UI；335–339 的入口接线见上节，仍未编译、测试、运行认证/API/helper 或操作设备。此节覆盖 325–329 中“独立关闭审批仍待开发”的历史状态，不代表可信远端任务结果、长期运行/留存、新部署生命周期、最终放飞许可或其它原路线图目标已完成。

## 325–329：同一实际 Run 进入受保护任务模型

`SwarmController.executionTaskOrchestrator` 仅供内部controller关联首个同线程typed Orchestrator；null/错类型拒绝，一经接受即使对象销毁也不重绑。运行准备从实际Swarm取得该原生backend，在第一MODE前登记到[同一任务编排器](SwarmMissionOrchestrator.md)；没有新发包、caller map或QML可写任务注册。缺少实际来源、容量或成员冲突时不允许进入MODE queue；后续每个目标的readyForQueue还核实际记录归属，不能仅凭相同taskId/runId恢复。

每次Run对应新的Orchestrator UUID taskId，不复用FleetIntentTask的sourceTaskId、归档recordId、Run UUID或controller编队activeSwarmTaskId。immutable身份恰6键：runId/startupId/transactionEpoch/transactionGeneration/deploymentId/binding；binding保留实际7字段或成对inputGeneration/reviewGeneration的9字段，不能只按wirehash或删代次比较。真正Vehicle/PRIMARY/队列证明仍由原Run/Startup拥有，该值身份不授予权限。

private friend TaskModelEvidence只从本实际Run字段生成最多32机固定观测投影，不接受外部JSON或成功bool；无精确位置、原payload、账号凭据、原始收据或仅随时钟增长的hostQuietMs。当前源匹配与last-observed样本新鲜度继续分开，序号/次数不是成功百分比。所有普通task mutation对native记录拒绝，owner丢失或closed后也不降级成通用任务。

登记时仅是protected Draft；真实captured MODE commit后为Observing，仍不是Running执行认证或Completed。观测变化使用独立uint64 observationRevision和专用readonly通知，不触发普通setTaskProgress、taskUpdated黑匣子路径或改变结构taskRevision；控制/本地生命周期变化另记。公开事件延迟合并，实际注册/queue事实在原栈中同步保留，不从250ms后通知反造事实。

native准备期已阻止其它Orchestrator任务占同组成员，但不提前进入legacy busyVehicleIds以免拦自己的后续MODE gate；Swarm原真实整组lease始终保护发送。startup认证退休且确有MODE commit后，才进入legacy忙碌摘要，未知来源不释放观察任务成员。监视退休后的晚commit只补历史，不重开观察/续租或改变为正式完成。

明确native retireObservation(true)的请求事实`observationRetirementExplicitlyRequested`与析构`observerOwnerDestroying/ownerDestroyed`分开，二者都不认证操作账号或两名人员。析构先保存末次本地事实、退订/释放自己的监视占用，再由QObject destroyed使模型保留未知尾部；先前明确请求事实不被销毁抹掉。ObservationClosed只关闭监视，绝不意味着落地/停飞/任务成功，也不自动发DISARM/Land/RTL或恢复旧批准。

325–329阶段controller仅新增missionExecutionObservationTasks只读值列表，不暴露backend、恢复接口或完成/停机按钮；330–334另接独立监视结束后端与受限门禁，当时尚无操作UI，现由335–339顶部节补上入口。运行current只附本地executionObservationTaskId和诊断，实际记录仍在同一Orchestrator，不建立第二套任务真相。观察记录单独最多128，总任务仍受旧10000限制；满时阻断新登记，不淘汰closed/unknown历史。独立关闭审批的当前源码状态见上节；授权清理/长期留存、新部署生命周期及可信远端正式结果仍待开发。

没有MODE实际发出的prepare可在原Txn终态、所有发送/授权回调及native协议栈排空后，由私有retireUnissuedPreparation自动关闭。它逐机核同原对象/步骤、queued与uncertain均false、Run无commit，已有真实receipt须明确requestIssued=false；空receipt只允许本Run从未begin该lane。任何缺源/不确定均拒绝，未退休Run也提前阻止新地面重审，不让它走到末段才遇旧Draft冲突。该收尾只置unissuedModePreparationClosed，不设置explicit retirement事实、不推断未解锁/停飞、不回滚ARM或序号；真正MODE写入之后不可走这条路线。Txn在busy/checking/sending栈里终止时继续保留原占用并延后排空，不在回调尚未返回时判定“未发出”。

本轮只源码/限定静态复读，未运行任何API、编译、测试、渲染或设备。工程师需验证首/部分/晚MODE、零MODE停止后地面重审、其他飞机继续queue、源/Orchestrator销毁、显式退休/unused准备收尾/析构区别、两代次完整身份、generic mutation全拒、独立revision/通知与容量边界；这不是已经运行的结果。

315–319 新增仅 native 的 `FleetMissionExecutionObservation`，派生独立 [VehicleMissionExecutionObserver](../../../src/Vehicle/VehicleMissionExecutionObserver.h)。本地 run UUID 将实际启动事务、实际 MODE 排队事实与后续入站序号观察关联起来，不是远端 mission instance、密码学身份、执行许可或任务完成证明。没有 QML 类型注册、Q_PROPERTY、Q_INVOKABLE、import/replay、文件/数据库、请求新遥测流或控制接口；对外仅 `current()` 值快照、合并 `changed` 信号和 native 显式停止观察。

本批只实现源码并作限定文本审阅，未编译、测试、运行 API/helper、启动应用、读取日志或操作硬件。它补上启动后的连续观测链，不把原六类剩余目标缩成“只等测试”，也不宣称最终 StartPermit、净空或完成归属已经实现。

## 实际来源冻结，不接 caller JSON

`prepareFromStartup(actualTransaction)` 为私有接口，只有实际 [StartupTransaction](FleetMissionStartupTransaction.md) friend 调用。事务先保存本 observer 指针，Run 反查同一实际指针、epoch/generation/startupId、stage2、尚活动与整组占用；冻结实际 deploymentId/binding、每机 Vehicle/MissionManager/VehicleLinkManager/PRIMARY、原始 readReceipt、payload content hash、sourceItemMap、item count 和最后 command。最多 32 机、每机 2048 项、全组 8192 项。

prepare 核实际机队同 ID 对象、PX4 多旋翼、初连完成、非离线编辑、真实连通非 mock/replay PRIMARY；Link 可以运行在自己的工作线程，不要求 Link QObject 与 GUI 同线程。它检查实际完整机载读取的 retained 来源、ProgressObservation 的真实缓存/wire 匹配和稳定来源，再独立冻结真实 MissionManager cache fingerprint。不从 public current、历史归档、反序列化 map 或调用者 bool 恢复这些依据。

原 READ 的 120 秒规则仅用于本次 admission，不每隔一段时间将旧读取伪装成重新读取，也不把该短期门禁当整个飞行的续租。运行中独立检查实际对象/PRIMARY、core 绑定及实际 cache fingerprint；`MissionManager::operationStarted` 直接保留已知外部操作，即使操作在两个 250 ms tick 间结束并恢复同样缓存，也不把 A→B→A 当连续不变。相同 ID 的新 Vehicle/MM 不接管旧观察。

## 原生 queue 切点与 provisional 区分

`readyForQueue` 仅返回本 actual target 的已冻结观察是否还能接入，不授予发送权限。真正发送仍走原启动事务 full fence、固定 mode-only wire 和完整实际来源重验。core 只有在真实 MissionModeOnly queue 路径、实际 write 前，才私有安装独立 epoch/slot 并调用 `beginExecutionQueue`；不存在公开 bind/prepare 来伪造已发送。

QueueContext 精确包含 sourceEpoch/startupId/stepId/operationId/wirePayloadHash/vehicleId/componentId，Run 核规范 UUID、非回绕正操作序号、固定 hash 形状、实际对象和冻结 MODE step。begin 只保存值与 provisional 状态，不发 signal、不读写磁盘、不触发控制。core 随后仍可因二次 fence 失败退休该 slot 而根本没有 write。

begin 后的最后 fence/immutable 检查还须从实际 owner 重新取得同一个 observer；`readyForQueue` 在同一活动阶段既允许尚未 begin，也允许仍 tracking/sourceActive、尚未 commit 的同一 provisional target，两种情况均拒绝 continuityUnknown 或来源变化。MM operation ABA/非法 provisional 一旦退休依据，就不能只因 core slot 尚在而继续 write；`beginExecutionQueue` 自己另拒绝重复 begin，不覆盖旧 QueueContext。

真正 write 调用返回后、公共通知之前，core 向当时捕获的 observer/context 调用 `executionQueueCommitted`。该事实即使 startup 已取消、source 已退休、Vehicle 已删除或 observer 已明确停止，也按仍活着 observer 中唯一 context 保存；不再要求当前 slot 有效，不把真实已排队事实抹成“从未发送”。这仅是本地 queue fact，不证明无线收到、动作成功或响应因果。

write 前或嵌套 write 栈里可收到真实 RX；它们只进入单独 `provisionalFrames/provisionalObservations`（最多各一个 current/reached、若干实际观察 bool），不进入正式位集、周期时钟、空中/terminal/LAND 候选。commit 保存当时的 core sequence baseline，**不会将 provisional 历史回填为 post-commit 证据**。正式进度必须由 commit 之后的新 native callback 推进。未知扩展字段不被补为 supported/complete。

## 持续有界状态，而不是成功百分比

core 对同实际 PRIMARY/default component 的原始 pre-plugin 帧计严格递增 UInt64 source sequence。Run 再核 epoch/对象/system/component、单调接收时间与 core 当前绑定；重复/回退/无效长度或值保留 unknown，停止该来源跟踪但不清 queue 事实。native sequence 缺口只计未消费的本地回调序列，不能叫网络丢包数。无关消息只计 ignored，不保留 payload。

正式解码 HEARTBEAT、MISSION_CURRENT、MISSION_ITEM_REACHED、EXTENDED_SYS_STATE、GLOBAL_POSITION_INT、HOME_POSITION。支持 MAV1 固定 minimum length 和 MAV2 合法尾零；解码前将未接收的 MAV2 trailing bytes 规范置零，但不把协议定义的 unknown 零扩展变成已知状态。GLOBAL 只检查编码范围/heading/boot-time 回退，既不保存精确坐标/航迹，也不认证位置或重启。HOME 仅在内存比较当前 raw payload hash 并累加改变次数，不公开该 hash，不认证 HOME 有效性、local origin 或 PX4 内部高度分支；hash 也不是加密/匿名化保证。

每机 current/reached 各一个至多 2048 bit 的集合、最后实际序号和对应 sourceItemMap 行；另外保留重复 reached、current 跳号/回退、Mission 模式退出、HOME 改变等有界不回绕计数。集合表示“本地本次观察窗口内见过这些 wire 序号”，不是成功项数、唯一到点动作、飞行距离/时间/面积或百分比。PX4 会重发最后 reached，不以新收包次数制造新到点。

armed/Mission mode/landed 的 known 和值统一使用 `lastObserved*` 名称，表示最后一次有效 post-commit 样本，断流后保留历史，不声称它们仍是当前状态。`currentValuesFreshnessVerified=false/valuesAreLastObservedOnly=true` 明示未采用新的运行 freshness 政策；`sourceCurrent` 只查当前实际 native 绑定，不能替代各通道 fresh 或健康证明，不发明未审批的超时阈值。

四个周期通道分别记录 heartbeat/current/landed/global 的最大已观察 raw 间隔；首个正式样本从 core binding clock 的 0 点保守计时，包含 binding/queue provisional 窗口，不声称重建实际传输时延。`hostQuietMs` 来自 Run 自己的单调 callback clock，两个独立 clock 不相减；未观察返回 null。reached/HOME 不被当作周期心跳。周期数字只是诊断，不证明报文间真实物理连续性，没有隐式新增政策阈值或无限等待保证。

每条消息最多固定规模解码/计数，无 per-frame Qt signal、无原包/签名/坐标/完整风险历史保存。一个 250 ms timer 合并状态/源与缓存复核/changed。bitsets、source maps、四通道时钟和计数均有界；不承诺整个进程 RSS、硬实时或绝对 I/O/调度期限。

## 终结动作仍分开

公开 phase 区分等待实际 MODE queue、provisional、序号观察、terminal item、无限 terminal HOLD、LAND 过程、落地候选和来源未知。所有正式状态都仅由 post-commit callback 推进；LAND 的候选顺序额外要求该 run 内先见真实非 UNDEFINED 的空中状态，再收到 terminal-current，再见后续 landing/on-ground 转换；不能用 provisional、最初默认地面或旧 landed 快照填空。当前/到点/地面均不包含机载计划 hash 或启动 token，顺序匹配仍只是观察候选，不是任务成功。

当前本地配套固件源码快照 [mavlink_mission.cpp](../../../../FirmwarePX4/src/modules/mavlink/mavlink_mission.cpp) 的 current/reached 发射主要携带 seq，内部 MissionResult 的 instance_count/finished/failure/execution_mode 不经这条路径完整传出。这是对这个本地目录的源码分析，未建立 Git 身份、构建签名或真机二进制对应关系，不能推广到所有 PX4 版本；仅本地 Run 不补造远端 instance。无限 LOITER_UNLIM 不自行完成；LAND 候选不证明正确地面、净空、成功停机、任务目标完成或远端因果。`unknownTail=true/tailObservationComplete=false` 保留真实未观察窗口。

`missionCompleted/executionVerified/executionAttributionVerified/requestCausalityVerified/authenticated/authenticatedAircraft/signatureVerified/crcVerified/startupPermitGranted/executionPermit/executable/flightCommandReleased/permissionsRestored/fullTelemetryCaptured/localReferenceValidityCertified` 均 false。flightCommandReleased=false 描述本观察器没有释放命令；旁边实际 modeQueueCommitted 仍保留原事务已排队事实，不能将 false 解释为飞机没有飞行指令。

## 退休启动授权与观测占用分开

`startupRetired(state,reason)` 仅保存原启动的终态；原账号/发送权限退休或过期后，独立被动观测可以继续，但没有任何续授权/发命令能力。partial MODE、取消/失败和 source retirement 都不能清已 captured commit。

原事务等协议/接收栈排空后，如任一 target 实际 committed MODE，调用私有 `adoptReservation(actualSwarm,oldStartupId,orderedIds)`。Run 核真实 Swarm 和原整组；ID 先与冻结 actual Vehicle IDs 逐项比较，传给 `transferMissionDeployment` 的是冻结 native Int 列表，不把 legacy JSON doubles 放进严格 transfer API。原子整组换 owner，不先 release/reserve，没有空窗；旧 startupId 的后续 release 不能释放新 runId。重复采用必须重新查 `missionDeploymentOwns(runId,ids)`，安全撤销/丢失不重建，不从历史 adopted=true 推出当前仍有占用。

native `retireObservation(true)` 幂等结束观察、标记 unknown、解绑实际 observer/epoch，并只释放自己曾采用的 runId；从不释放旧 startupId 或其他 owner。不发送降落、解锁、停机、回滚或模式切换，因此停止观察不等于停止飞行。析构同样先关闭观察/解绑并释放自己的占用，已发送/来源丢失事实不变。工程部署必须保证实际运行对象生命周期与操作员职责，不能把监视对象销毁当飞行已经结束。

## 仍待工程师验证及后续代码

本批未执行以下任何验证：目标 Qt/MSVC/moc 构建、真实工作线程 Link、完整/partial queue、provisional 与 commit 交错、source 退休晚到/Vehicle 析构、同值 MM 操作 ABA、cache 变化、反复 reached/跳序/回退、四周期 clock/UInt64、LAND 顺序、HOLD、原子占用移交/安全撤销、明确停止与对象销毁。它们是工程师后续事项，不是已运行证据。

仍需最终 StartPermit/起降净空与完整时序、可信机载 mission instance/终态来源、正式任务目标/结果合同与经过授权的任务状态提交，以及原路线图其它六类实质工作；本地连续观测不可替代这些要求。

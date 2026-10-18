# FleetMissionReviewCoordinator

## 546：保持完整报告的原摘要

Builder已经为完整v1报告（包括固定boundaries）计算reportId，Coordinator不再事后向该report根添加manualConfirmationRequired/flightCommandReleased而破坏摘要。外层review的同名固定边界不变；所有报告消费者仍只读。这让实际lastReview.report可以被[四受众工作区](FleetMissionReportWorkspace.md)完整冻结、查看和交付，不改变任务门禁或PlanStore默认历史格式，也不把报告中的clear/safety建议变成许可。仅源码静读，未执行构建/解析/测试/文件往返。

## 543：实际逐机航线估算进入能量预算

Compiler 重新生成的 `nominalRouteTiming` 随实际成员路线进入 `missionPlansFor`。`relativeDwellBudget` 严格检查其11字段、真实数值/Bool、未知null与原因，分别把本机出程、返程航行估值加上既有 DwellEncoding 保守驻留预算；不同估值取max，不把估算重复相加。坏报告使预算无效；缺报告保旧行为，合法未知保null，不借别机路线补值。

名义出程只提高已经存在的显式时长、已有估值或任务时限预算；没有这些基础预算时任务时长仍为0并由原Scheduler拒绝，不能用理想航线或驻留补成完整任务。返程只提高既有独立返程预算。这个“提高”是规划防低估，不是真实飞行时间的物理下界，更不保证起降、转弯、风、负载、到首点及终末动作已建模。

原逐主机 `memberEnergyBudgets` 仍exact5并固定原机号/角色。details 增加 `missionBudgetBeforeNominalFloorSeconds/nominalOutboundFloorApplied/returnBudgetBeforeNominalFloorSeconds/nominalReturnFloorApplied`，以及本机 `nominalRouteTiming/nominalOutboundWithHoldSeconds/nominalReturnWithHoldSeconds`。调度器沿原 `budgetDetails` 传递到真实能量结果和已有明细浏览。组级驻留摘要保已知项数量与已知项最大值（数量0时最大值0不是已知路线时长）。储备机仍用自身独立出勤预算；不改电池模型、性能可信度、批准或StartPermit。

原生参数速度建议见下方同批观察合同及 [配置编辑器](FleetExecutionProfileEditor.md)。本批只修改源码与直接静态阅读，所有编译、模型执行和工程验证未执行。

## 537：逐 NAV 驻留的共享数值语义

相对驻留预算改用共享 [DwellEncoding](FleetMissionDwellEncoding.md) 的原请求／Float32上传值／声明微秒等待量三者的保守上界；去返逐点、两段总和及估算航行时间加驻留均使用有向保守加法。现有outboundHoldSeconds／returnHoldSeconds／totalHoldSeconds等字段是该数值模型的预算，不再声称为原请求的简单算术和；原点holdSeconds仍在plan／source中保持。精确可表示的秒数和加法不无故多加ULP；非零归零、模型不支持或溢出整份预算无效。

明确路线／成员duration与相邻权威到达窗口也检查同一预算，而非只检查较小原请求。未知航行、真实达到／调度、起降动态仍未知，不由驻留预算补成已认证时间。现有报告形状与false认证字段不变。

即使没有executionProfile，实际编码含明确驻留也必须进入原编码／实时版本／当前工程声明审阅门。原Catalog只匹配PX4多旋翼观测，本批不把普通NAV驻留扩为固定翼支持，不提升本地声明为真实二进制认证。不增加任务启动许可或正式结果。

仅源码及限定直接静读，未运行helper／模型、编译、测试、仿真或修改PX4。实际工具链浮点语义、动态行为与最终飞行放行由工程师验证；下方536及更早为历史。



## 536：逐航点朝向编码与原生消费者接线

transportEncodingFor 仍来自实际 PayloadBuilder，新增只读提取逐成员 wireSchema；显式 heading v3 即使不带 executionProfile 也进入 encodingRequired 证据门。实际原生逐机版本／目录匹配之后，还须由同一当前 Catalog::acceptsWireSchema 核该机的编码域，不将 v2 声明当 v3 支持。未覆盖时新增 blocking risk firmware_adapter_wire_semantics_unmatched；原版本不足／声明不匹配原因保留。

没有扩大 native capture／catalog matching 的 DTO、恢复批准或新增飞行命令；同一原计划、目录指纹及原来源仍随审阅绑定。无heading的 v2 payload／目录语义保持。

共享合同见 [HeadingEncoding](FleetMissionHeadingEncoding.md)。本批仅源码修改、直接静读与普通文本核对；未编译、运行实现／解析器／算法、测试、仿真、硬件操作、修改PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 516：共享近期 SOC 窗身份与实际能量消费

新增 owning-thread 纯值 `nativeBatteryDrainReviewIdentity(vehicleSnapshot)`，返回固定6键：`state,identity,usable,remainingFlightSeconds,percentPerSecond,ageMs`。state 为 missing／invalid／available；缺 sibling 为 missing，存在但坏型／跨旧20来源／公式不一致为 invalid，不把坏值变合法未知。available 仅表示共享 [inspectDrain](FleetNativeBatteryEvidence.md) 值合同有效，不认证 Registry、来源、真实性能或操作权限。线程不符为 invalid，无查询、信号或 review 修改。

identity 保原 schema／reportType／vehicle/component/epoch／独立下降代次／batteryId／sourceCurrent／observed／state／reason／drainKnown／rate、去掉 endAge 的固定历史 window，以及 usable。它不包含随 flat 包或自然时间增长的最新 receipt、raw age、endpoint age、扣龄 remaining 或旧全局电池代次；不 hash 整份 fleet。旧20身份照旧单独核。窗口首次成熟、原 source／内容改变、清除、unknown、过期和 usable 转换仍改变稳定身份，A→B→A 不能被旧代次假认领。

只有 usable=true 时三个动态值有效：rate>0、remaining≥0、ageMs为0–30000ms的 effectiveAge=max(最新原RAW age,下降 endpoint age)，用于预算扣龄，不是远端测量年龄。usable=false、missing、invalid时统一-1；完整原始年龄与窗口诊断保在 energyPlan 独立 top。Root 实际取 Registry snapshot 送此同一校验入口，不接受导入值作为授权；稳定指纹与最后预算尾检各有用途，不用一次缓存身份代替当前来源。

原 deriveEnergyTask 和实际 Scheduler 链继续生成完整主备预算，现完整 energyPlan 可含 `nativeBatteryDrainAdjustments`；assignment18／21与编译 ReserveEnergyPlan29／32、原 archive schema、审批和 StartPermit 不变。515 明细按实际 review.energyPlan 有界下钻即可看到完整前后值／window，无新 UI 或命令。只做地面站源码与静态阅读，未编译、执行、测试、仿真、刷机或工程放行；近期 SOC 线性外推不是电量／负载／未来续航模型的保证。下方为历史合同。

## 513：备用独立出勤提案进入真实原生后台入口

新增 owning-thread `buildReserveDispatchProposal(source,request)`，薄调用 [ReserveDispatchPlanner](FleetReserveDispatchPlanner.md)。source 精确三字段 `binding/task/plan`，request 精确三字段 `reserveVehicleId/executionProfile/operatorConfirmed`；完整形状与值均由中央 Planner 检查，旧来源只能作为 retained 原材料，派生任务仅带新的 `reserveDispatchPlanningSource`。返回固定七字段 `ok/reason/task/summary/provenance/flightCommandReleased/executable`，两权限恒 false；线程不符返回固定失败、空 task／summary／provenance，不改 `lastReview` 或发批准通知。

这是纯值新任务构建，不读取文件或飞机、不消费账号凭据、不恢复旧 Run 或批准，不发送命令。新任务经现有 Compiler 的完整 task／实际 plan 双侧来源核验与 PlanStore 一致性检查；当前真实舰队／能力／电池／参数、角色／健康／能量／空域／风险及归档仍由一次正常新审阅取得，不能从提案或旧档补成当前证据。原 strict Bool、clearance／边界政策、完整路线和各自 floor 不被该薄入口丢失或放宽。

`custom.pri` 登记新 Planner 的 h／cc 各一次，没有 QML 可构造类型或新授权器。本批完成的是原生后台消费者，不宣称 Root／可见人工出勤入口、实际出勤、StartPermit 或全部目标完成。仅直接静态修改／对读，未编译、执行、测试、仿真或操作设备；下方为历史合同。

## 483：完整工程性能来源进入原审阅身份

`capabilityEvidenceFor` 当前schema3，每成员在原能力／基线身份之外保 `declaredPerformanceEvidence` 完整map及 `declaredPerformanceEvidenceUsable`，由同一个[共享helper](FleetCapabilityEvidence.md)读取原fleet快照，完整成员列表进入原evidenceFingerprint。无效声明保空map／false；usable不表示两项数字都known、任务可行或性能认证。这个材料升级总是发生，不宣称false策略时全系统输出schema都不变。

严格任务仍经原RoleAllocator／Compiler，再由deriveEnergyTask原成员账单送EnergyScheduler；true策略不丢、主备预算不换成其他飞机或共享最长任务估值。原native snapshots／原规划输入和来源连续性检查继续约束实际产品；pure证据map和hash本身不产生Registry真实性或操作批准。

只消费既有工程批准数字，没有新性能录入器、UI、Draft.settings支持、实测清单或标定。原能力批准／identity hash schema及所有P0false／StartPermit阻断保留。工程交接30项全部未执行；未编译、运行helper／模型／业务／QML、测试、仿真或操作设备。下方为历史。

## 480：完整起降参数证据进入真实风险

nativeVehicleEvidenceFor继续持完整FleetMissionVehicleEvidence::capture输出；现schema4独立actionPhaseEvidence包含固定14行、模型及兼容结果。原parameterEvidenceUsable消费者会对缺失／过期／SYS_RESP失配及明确动作的RC／外部注入不兼容生成原native_parameter_evidence_unusable阻断，不只显示标签。

完整stable证据指纹保动作值、类型、来源与语义revision，改变时旧review／approval不能续用；最终原28内存尾核在所有getters之后。原executionParameterRows十二行和公开profile观察exact19不扩容，Contact／几何域不借模型ok放宽。本地参数分支和固件目录匹配不是机载二进制、真实轨迹、净空或StartPermit。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：消费新完整证据，原12项hash offer不扩大

`nativeVehicleEvidenceFor`沿既有实际Vehicle采集入口获得schema3，完整captured成员与evidenceFingerprint进入原审阅材料；`parameterEvidenceUsable=false`继续触发真实 `native_parameter_evidence_unusable` 风险。新guard来源／模型／14项最终memory检查已经在collector实际消费，不是只显示建议或标签。

原 `executionProfileObservation` 不改公开19键、12项name/type/value排序hash、parameterCount12、原12行memorytail；它仅提供操作者profile内容绑定，故意不把未绑定profile、无catalog或guard未知变成取得旧12hash的循环要求。取得此hash不意味着当前完整14项证据可用，更不恢复审批；当前native review和Readiness各自还要复核完整来源。

公开接口与Compiler／plan archive格式未改，原guard变化会通过新完整native指纹影响审阅与启动阶段。不存在以纯模型或Refresh完成冒实际速度／固件认证／StartPermit的新路径。仅静读未编译、运行或测试；下方为历史。

下方为历史记录。

## 475：联合余段提案和逐成员返程预算的真实消费

新增同线程纯值入口 `buildGroupRemainderProposal(source,request)`，薄调用 [GroupPlanner](FleetMissionGroupRemainderPlanner.md)，返回原提案 exact7；不保存 lastReview、不读文件或飞机、不批准或发指令。`buildGroupRemainderView(plan)` 薄调用 [View 新 schema2](FleetMissionRemainderView.md)，旧单机入口和值合同保持。线程不符固定失败，不回退旧提案。

联合任务仍经原完整一次新审阅、真实编译、当前舰队／原生来源、角色／健康／能量／空域／风险以及归档门禁。Compiler 对每个实际 member 自身 `minimumSpeedMps` 与原当前 fleet 描述的 `maxSpeedMps` 做独立 blocking 核对，不用全组最快值或其它飞机代替；缺资料不是测得零或性能已证明。逐主机能量账现在实际取该成员 `returnReserveSeconds` 与原本机运动／停留／共享返程预算的最大值；坏 floor 使该账输入无效，不吸收成其它机的预算。原各自 `durationSeconds`／returnDuration／时间限制继续由既有推导消费，备用仍用自己独立 energyPlan，源 reservePlans／新备用 ID 不匹配仍由原编译及风险明确阻断。

新来源不恢复原 Run、控制权、批准、执行结果或飞行许可；原同步／后台和最后当前性门禁不放宽。只做静态源修改与交叉对读，未编译、执行、测试、仿真或设备验证；下方为历史。

## 460：原能量／健康阻断消费新增原生电池负面

本Coordinator源码本批不改。实际[EnergyScheduler](FleetEnergyScheduler.md)共享新[电池检查](FleetNativeBatteryEvidence.md)，明确负面及过期未解决负面使原主／备用eligible=false；原完整账单、RiskEvaluator blocking与needsReview继续被消费。[HealthCenter](FleetHealthCenter.md)本次原生负面同样进入原健康快照，不由caller healthReports覆盖。

Guided当前来源身份支持原生schema2，charge／fault纯变化即可使旧审阅不再对应；缺扩展旧schema不自动升级为健康证明。完整运动域、最终启动许可、正式结果及原25目标未因此完成，未编译、运行或测试。下方为历史。

## 451：后台编译与显式完成原生审阅

新增原6参数的 `beginReviewWithNativeEvidence(task,fleet,healthReports,noFlyZones,options,riskSignals)`，返回固定4字段 `accepted/requestId/status/error`。开始首先撤销旧审阅和旧pending，**lastReview为空也不能跳过取消**。原6输入经[Worker](FleetMissionPlanningWorker.md)完整纯JSON冻结；GUI侧按原规则产生rolePlan和compilerTask，冻结原输入指纹及compilerTask／fleet指纹，随后仅将不可变编译输入交给一个后台线程。

`planningStatus` 固定8字段：`requestId,status,active,ready,error,inputFingerprint,manualConfirmationRequired=true,flightCommandReleased=false`。status为idle/running/cancelling/ready/failed/cancelled/completing/completed；`active`仅代表旧线程尚未结束，不表示可以发命令。没有全计划／原生QObject／参数数组在状态字段中公开。`planningStatusChanged`只更新此状态；实际线程finished且结果冻结后才发 `planningReady(requestId)`，不会自动写 `lastReview`、审批或归档。

同一实际线程未finished时再次begin会先取消旧请求，再返回 `mission-planning-worker-still-finishing`；不堆队列、不自动重新开始。源目录变化、clear或明确取消都使原epoch失效；A→B→A不能恢复原请求。外层Root仍需用自己的输入／身份／health／fleet代次追踪其它实际变化，不能只比较最后相同的文本。

开始阶段在GUI冻结候选原actual Vehicle与ParameterManager的QPointer身份及原目录对象／快照。完成仅允许所选主机仍对应原实际对象；销毁、替身或同ID的新对象不能补位。开始时缺实际机体只保留缺失诊断，不可借等待中新出现机体冒充原对象。开始**不冻结参数数值**：实际参数／固件证据仍在显式complete阶段沿原GUI native捕获首次读取；Worker没有访问这些对象。目录和选中对象在native回调后以及最终发布前再次核对。

`completePlanningReview(requestId)` 只接受当前ready请求，消费一次私有已完成计划，进入completing，再由原 `_review` 路径采集实际证据、检查角色／能量／空域／风险并构造原完整review。原compilerTask／fleet指纹重新核对，只有Coordinator私有override能替换编译步骤，QML不能提交任意plan。正常完成只在此处发一次原 `reviewChanged`；Root须在complete前后核原Intent／输入／健康／机队／禁飞区和归档CAS，ready本身不等于采用。

`cancelPlanningReview(requestId)` 仅作用于当前同ID的running/ready/completing/cancelling请求。completing也递增原reviewEpoch，令尚在native采集中的旧续作尾部拒绝。若取消发生在本次 `reviewChanged` 同步通知内，已经发生的publication不能抹称未发生；但本次live lastReview与缓存会立即撤回并通知失效，不能保旧可批准值。取消不回滚历史归档事实，也不发送任何飞机命令。调用方不能以失败／取消返回推断此前从未发布，而应保实际回执并按末尾current性处理。

原同步 `review`／`reviewWithNativeEvidence` 遇 `constraints.clearanceInput` 明确清除旧审阅并返回 `mission-planning-async-required`，不能绕过后台路径在GUI重新跑净空solver。未含该资料的旧同步路径保留，但会退休旧pending。complete后的原证据／能量／空域／风险收口仍在GUI线程，不宣称所有计算或所有I/O已移出GUI；只读证据采集不请求遥测、不写参数、不认证场景、不授予启动许可。

Coordinator析构只取消并解绑；Worker无parent并在finished后自清理，不阻塞wait或销毁运行中QThread。取消是有界阶段内的协作检查，无立即结束保证。未编译、测试、执行线程／helper、UI、仿真或设备；交叉取消、关闭、替身、目录更新、错误输出和正常显式完成均待工程师验证。450及以下为既有历史说明。

### 451 Root与三个实际入口

`GuidedActionsController.prepareFleetIntent(intent,parameters,completion)` 对带`constraints.clearanceInput`的任务走本次原生begin；返回`planningPending=true/requestId`只表示接受后台计算，不能当最终成功或拒绝。Root只读`missionPlanningStatus`组合原生线程占位和本地待完成票据，保持计算／等待完成期间可见忙态。`cancelMissionPlanning(expectedRequestId)`可核按下时冻结的准确ID，只取消同一次本地计算，不取消飞机动作；省略ID仅用于现有内部兼容调用。

开始前退休原批准，独立`preparation`对象隔离同步重入；等待票据绑定原Intent、Reviewer、Swarm／Registry／Health对象、taskId、输入代次、原生请求及指纹和稳定health／fleet指纹。健康或机队先变坏后恢复也会永久退休票据，不以最终相同值复活。草稿尚未提交的编辑同样退休pending。只有本次begin清旧通知和本次complete发布通知可各获一次**准确reviewGeneration**豁免；其它reviewChanged继续失效，不能广泛忽略通知。

实际finished后由Root核票再显式complete，真实Vehicle／ParameterManager观察仍在GUI进行，之后进入原风险与准确版本归档。归档首尾核原输入、禁飞区、记录CAS，逐项公开binding时再次核原版本。仅`recordReview`本次通知栈允许保留票据，后置不匹配不能产生批准资格。`planArchive.ok=true`保留实际已提交事实；`planArchiveRecorded=false`表示当前审阅绑定不可用，不是回滚。completing中的退休延后完成回调直至实际原生／归档返回，保留已知receipt；回执中断时明确未知，不推断从未提交。

地图Draft的完成回调核原实际Draft、编辑代次、revision和修复选择；[余段面板](FleetMissionRemainderPanel.md)最终更新必须进入一个新的本地操作ticket，并核原capture／proposal／部署scope；AI面板核原controller、原proposal对象、输入token、内容指纹和panel lifetime。旧回调不恢复旧批准或改写新提案。三个入口都保留输入、显示等待／取消／失效和错误，且不自动批准、上传或发送命令。

本轮只按既有Qt5/QML桌面交互规则静态调整反馈与键盘取消，没有运行技能脚本、QML解析器、UI或交互验证。原生采集与complete中的旧能量／空域／风险计算仍在GUI；JSON、原Joint提取等部分操作仅在边界检查取消。没有固定响应时间保证，也未完成起降净空／最终StartPermit。工程师仍须实际构建并复核线程、回调、析构及正常／取消／换源全链；见[交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 450：原样投影本次Compiler规划净空结果

Compiler已在完整plan中实际冻结planningClearanceReview；本协调器仅在该字段存在时，把**本次plan中的完整值**原样投影到本次result，不另取lastReview／地图、不再求解，也不借新报告替换原风险或空域结论。完整plan仍按原归档路径保存，不依赖新增QML白名单字段才能保留它。

构造失败／模型不满足的新增blocking由原风险链实际消费，原clearance_geometry_review_pending保持。规划范围与actual startup明确分开，独立geometryBasis不产生审批、上传／启动或执行许可；其余真实来源／健康／模板／能量／编码／airspace门槛不变。未编译、测试或运行；下方447及更早为历史。

## 447：严格消费实际风险阻断列表

原 `needsReview` 汇总改用 [RiskEvaluator](FleetMissionRiskEvaluator.md) 的原生纯静态 `hasNoBlockingRisks(riskAssessment)`。`inputValid=true` 只说明输入结构有效，不能覆盖合法的业务阻断风险；任何非空 `blockingReasons` 都令本次 `needsReview=true`、`reviewStatus=needs-review`。输入有效标记必须为非null真实Bool，阻断列表必须为非null实际QStringList／QVariantList且为空；缺失或坏型不能通过转换成空列表获得审阅资格。

正常原生空QStringList和合法JSON空QVariantList允许继续其余原门槛；不把恒为true的 `requiresOperatorReview` 当作拒绝条件。原计划、主／备用能量、空域、模板、编码、实际原生来源、健康及生命周期条件全部保留。未命名critical／major信号由Evaluator统一补同一个 `caller_risk_signal`，既写报告风险，也写阻断原因，不会只显示而漏门禁。

本次仍提交并发布当前审阅结果，不凭新风险阻断提前保留上一份可批准审阅；实际审批沿既有 `needsReview/reviewable` 与绑定代次核对，Deployment另独立核冻结风险。ReportBuilder的只读安全展示不是这里唯一的门禁依据，也不把报告缺覆盖／人工复核提示一律冒充新的认证结论。无DTO／schema／权限或新命令变化，不证明完整风险真实性、净空或完整StartPermit。

未编译、测试或运行；原生两种合法空列表、非空阻断／坏型、匿名风险、警告正路径和旧审批失效及Deployment独立拒绝均待工程师执行验证。下方442及更早为历史。

## 442：同一捕获的联合名义空间重审入口

新增同线程纯值入口jointSpatialReview(source)，只转调[FleetMissionJointSpatialReview](FleetMissionJointSpatialReview.md)，source为准确两字段missionReview／startupView，返回ok／error／view。错线程返回明确错误及空view。不读取Vehicle、Registry或当前地图，不重跑Builder、不修改lastReview、不发通知或授予许可。

Root在原准确归档记录／整组目标／同一实际风险来源核对内调用，完整任务投影和启动快照均来自本次捕获；结果以同capture token缓存和显示。新结果独立于原归档airspaceReport，不更改其历史范围；缺失或不完整新报告不能借旧报告补成功。未编译、测试或运行；下方441及更早为历史。

## 441：保留禁飞区原值并阻断坏输入或双源不一致

`review()`、`reviewWithNativeEvidence()` 与内部 `_review()` 的第4参数改为 `const QVariant&`。省略时仍用 `QVariant(QVariantList())`，真实空列表和既有 C++ 列表调用保持兼容；显式 null、字符串、对象等不会先被转换成空列表。公开参数名与顺序不变，不新增审批、上传或飞行入口。

本次 epoch 开始时，用 [Checker](FleetAirspaceConflictChecker.md) 的 `inspectNoFlyZoneInput()` 分别检查原参数与 task 中实际存在的 `noFlyZones`。检查只接受非 null 真列表，最多1024项、16 MiB／20层／1M节点的纯 JSON 值及可精确表达的整数；不验证几何、来源认证、数据时效或覆盖完整性。合法 compact JSON 指纹仅用于比较本次两份值，不是签名。task 缺少该字段时仍允许原列表参数；task 存在时两份必须有效且指纹一致，不能用显式 `[]` 掩盖坏 task 字段，也不从另一份取有效前缀。

结果与 report evidence 新增固定7键 `airspaceInputCheck`：`valid/reason/argument/taskProvided/task/sourceMatches/coverageCertified=false`。`argument` 为固定5键 `valid/state/reason/zoneCount/fingerprint`；`task` 在字段缺失时为空 map，否则同为5键。合法结果的 reason 是明确空字符串；失败优先保留参数检查 reason，再为 `task_no_fly_zones_input_invalid` 或 `no_fly_zones_source_mismatch`。无效 count 为显式 JSON null，指纹为空字符串。旧 `airspaceInputEvidence` 四字段仍只记录原字段存在性／列表类型／数量，不能代替这份有界输入检查。

Coordinator 删除调用者 options 中的内部 `noFlyZonesSourceInvalid/noFlyZonesSourceInvalidReason`，只按本次原值比较结果设置。内部来源 reason 闭集为 `invalid_no_fly_zones_source/task_no_fly_zones_input_invalid/no_fly_zones_source_mismatch`；参数自身坏型的具体原因仍留在 check 的 `reason/argument`。原 `QVariant` 不经 `toList()` 传给 Checker；坏类型、超限或双源不一致由 Checker 产生 `analysisComplete=false/feasible=false` 的 blocking 违规报告，不在替代空列表上执行几何。原违规列表继续进入 RiskEvaluator 的 `riskItems/blockingReasons`，再进入 ReportBuilder；Coordinator 同时显式设置 `needsReview=true`，使现有审批和 Deployment 的 `reviewable` 门禁一起阻断。

新增只读 `inspectNoFlyZoneInput(input) const`，同线程转调上述纯检查，供调用者在归档／审批时检查当前原始输入。错线程仍返回固定5键、`valid=false/state=invalid/reason=no_fly_zone_input_owner_thread_required`、null count 和空指纹；不更新审阅或发送 signal。

原参数在任何原生采集回调之前复制。本次结果另带固定4键 `airspaceInputSnapshot={available,provided,value,evidence}`：provided 是本次 task 实际字段存在性，evidence 为原440的4键类型证据；available 只表示原参数能在有界纯 JSON 中完整表示。原值为列表时，必须自身通过 argument 检查才可保留，1025项等失败列表不能借单项包装越过区域数量上限。包装检查仅用于非列表的坏 scalar/map/null，且仍须满足同一深度／节点／字节限制；opaque、非有限或超限值为 available=false 且 value 为明确 null，不输出有效前缀。该小额额外包装层采用保守深度预算。整个快照经 JSON 值转换明确 null，避免 QML undefined 丢键。

归档应只用该原生冻结副本，不在 capture 回调后重新取可变 JS 参数代替。有效来源还须与当前私有 Intent 的 provided 和原值指纹相同，才可绑定新归档或进入原审批门禁，不能用调用期间 A→B→A 的参数变化把实际审阅 B 归档成 A。安全 JSON 坏值可按原值保存 blocked 诊断记录，但始终不能审批或执行；opaque／超限等没有可用快照时，本次 blocked review 仍提交，不记录新归档。直接 C++ API 在 task 无字段时允许单独传列表，保留原调用语义；这不是允许控制器忽略其自身 Intent 字段存在性核对。

坏禁飞区输入不使用提前返回保留上一份可批准审阅：除既有线程／生命周期失源拒绝外，仍完成当前 epoch 的报告提交、替换 `lastReview` 和本次原生缓存并发送原 `reviewChanged`。没有新许可、自动清除任务、MAVLink 或重新解释空域证据。下方440历史显示与归档能力保留；本批仅源码和静态核对，未编译、测试或运行。

## 440：原完整任务共同空间显示入口

新增 `missionSpatialReview(plan, reviewEvidence) const`，同线程纯转调 [FleetMissionSpatialReview](FleetMissionSpatialReview.md)，返回固定 `ok/error/view`。输入应来自同一实际归档记录；函数不查当前任务／Registry／Vehicle、不认证来源、不更新 `lastReview`、不发送信号或新空域检查，不改变审批与上传规则。

另有同线程纯类型入口 `airspaceInputEvidence(input) const`，input严格为 `{provided:真实bool,value:原始noFlyZones或明确null}`。合法返回固定4键 `schemaVersion=1/noFlyZonesProvided/noFlyZonesListValid/noFlyZonesCount`；只有provided=true且原值为非null QVariantList才记录列表及真实count，否则count为显式JSON null。错线程、坏shape或provided坏类型返回空map，不能当有效元数据归档。该边界兼容真实JS数组和Qt原生列表包装器，不用QML `Array.isArray`误判后者；只记录类型及存在性，不验证禁飞区几何、数据完整性／时效、身份或权限，不改变原检查／发送路径。

共同视图把全部活动成员的完整规划双路线、原整 plan 编码结果与保留的原空域证据分别显示。全局路线的规划继承不被注入编码器，成员错误不被成功前缀掩盖；RTL、实际返航、起飞／着陆轨迹、净空或任务完成均不由显示推断。源时间、估算和编码语义保持区分，未知用显式 JSON null。

16 MiB／20层／1M节点与32机／每机2048／全组8192边界有明确失败。仅完成源码与静态核对，未编译、测试或运行；439启动假设与本轮原任务几何不是已完成的联合安全检查。下方439及更早保留为历史说明。

## 439：冻结启动几何的纯值入口

新增`startupSpatialReview(source)`，仅转调[FleetMissionStartupSpatialReview::build](FleetMissionStartupSpatialReview.md)。输入是明确review身份、有序整组readinessSummary和声明间距，不查询Registry／Vehicle／Session，不更新lastReview／原审批或发送signal，不做I/O。实际原生来源与精确归档记录由Guided在调用前后核对，不把公开纯值helper当身份认证。

输出只解释三种独立名义几何与有限空间接近候选，保留未知时间／地形／净空；未编译、运行或测试，不改变原任务审阅／许可合同。下方435及更早为历史。

## 435：独立余段显示的纯值构建入口

新增 `buildRemainderView(plan) const`，同线程委托[有限显示投影](FleetMissionRemainderView.md)。从同一实际新编译计划构建规划点和完整编码映射，不把既有 transportEncoding 摘要伪装为完整载荷；几何和编码分别报告可用性。该入口不更新 lastReview、不收集证据、不查Vehicle、不写文件或授权。

Root仅在创建新余段审阅上下文时调用并缓存；UI深拷贝显示不供审批／上传门禁消费。新helper与工程源／头文件登记同步，未编译、测试或运行；原审阅和编码服务的许可边界不变。

## 433：实际Run余段提案的纯值构建入口

新增 `buildRemainderProposal(source, request) const`，同线程委托 [RemainderPlanner](FleetMissionRemainderPlanner.md)，返回固定七字段 `ok/reason/task/summary/provenance/flightCommandReleased/executable`；错线程返回空提案和明确原因。此方法不访问Vehicle、改变lastReview、发送信号、分配成员或授予权限。

来源的实际所有者由 [Startup只读取源](FleetMissionRemainderSource.md) 与 Guided 当前选定Run首尾核对建立，不能因调用纯值helper成功就认证外部JSON。操作员确认后，[余段面板](FleetMissionRemainderPanel.md)把私有冻结task交给原 `prepareFleetIntent()`：重新取当前机队/健康、验证模板并创建新审阅记录；不使用旧fleet、旧能力批准或旧审批。

派生成员的移动估算不包含hold，沿用原相对停留账单加一次hold；自身返程与原显式/默认保留量继续取保守下限。原task的安全、能力、能源、禁飞区与审阅选项保留，单成员重做范围明确重建，不修改原运行组。新记录不是新执行任务UUID，也不是接替完成。未编译、测试或运行。

## 432：独立备用几何与待命开销进入原账单

`deriveEnergyTask()` 保留主机逐成员推导，备用改用 Compiler 的 `reserveMembers.energyPlan`。逐机身份仍以原 `members`／`reserveVehicleIds` 为准，匹配唯一完整备用结果；缺项、重复或与身份不符的结果不借最长主机预算。helper 的政策、路线 hash、待命／进场／作业／停留／返程分量进入有限 details，路径保留在编译计划中。

备用估算为自身起点→被指定主机完整已编译航点路径→自身返程点。它不是从当前执行进度续飞，不继承主机起降／终态动作或速度，也没有实际改派。非覆盖任务仅在成员未声明自身路线且有合法显式共享路线时，Compiler 可构造预算专用副本；显式空／坏成员路线不回退，不补猜 AMSL，不因此改变 active members 或现有上传要求。

共同任务下限只取 task 及 task.constraints 的显式正数 durationSeconds／missionDurationSeconds／taskDurationSeconds／timeLimitSeconds；不取其他主机最长路线。两项账单为：

- 任务预算 = 待命时长 × 显式名义负载系数 + max(共同任务下限, 自身进场 + 完整目标路线运动 + 目标停留)。
- 返程保留 = max(原显式／默认返程保留量, 自身返程估算)。

所有分量必须有限非负，并核 helper 总量等于 standby + ingress + task + holds。待命是额外开销，不能被共同任务下限吸收；最后目标点停留只计一次。任务预算须为正。缺失或不一致时输出明确 -1/-1 和 required=true／complete=false／非空 reason，由 [Scheduler](FleetEnergyScheduler.md) 只拒绝该备用并保留原主机账单；原编译和风险阻断仍有效。

`routeTimingCertified`、`batteryModelCertified` 仍 false。直连几何、速度声明、待命系数和飞控剩余估算不构成性能／净空认证；431 的来源及扣龄复核不放宽。没有新增命令、批准、机载改派或执行入口。界面入口见[备用编辑说明](FleetReserveEnergyPlanPanel.md)，完整估算合同见[helper](FleetReserveEnergyPlan.md)。未编译、测试或运行；下方430备用最长预算为历史记录。

## 430：按实际编译身份生成逐机能量预算

`deriveEnergyTask()` 在原共同预算推导之上，逐个使用实际 `plan.members` 的路线、相对停留与返程资料，生成 [Scheduler](FleetEnergyScheduler.md) 的 exact5 `memberEnergyBudgets`。实际 primary 与 `reserveVehicleIds` 分组、数量和候选集合由编译结果覆盖，不接受调用者同名预算替换这些身份。用户原始数量要求仍由 Compiler 的 unmet requirements 保留；这里不是降低任务数量要求。

每机保留共同显式任务时长、time limit 和默认／显式返程保留量作为预算下限，因此有共同下限时不同路线仍可能得到相同时长。每机 `budgetDetails` 说明任务／返程依据、停留下限、可用的进场／作业／返程估算及距离；全部是估算，`routeTimingCertified` 与 `batteryModelCertified` 均为 false。旧全组最长预算继续作为摘要，不再替代各主机实际账单。

备用机尚无编译接替路线，暂取主机最长任务预算和最长返程保留量，明确 `reserveRouteProvided=false`、`reserveWaitingTimeIncluded=false`。这不证明备用待命耗电、角色等价、接替路线或实际接管。总身份最多255；空、坏或过大成员不能生成有效的部分绑定预算。旧 `relativeDwellBudget` 不以驻留时长填补未知航线，返程保留量不等于完整返程路线。

结果进入 [RiskEvaluator](FleetMissionRiskEvaluator.md) 的原主／备用身份复核，并由现有规划页的[能量账单区](FleetMissionEnergyBudgetPanel.md)只读展示。没有新增遥测源、飞行命令或审批资格。仍缺实测性能、电池耗能／风／载荷模型和实际返航、接替模型；原25目标及六类实质工作继续。未编译、测试、运行helper／UI／应用／设备、改PX4、刷机、提交、更新PR或打包。

## 420：基线值桥与审阅归档 schema2

新增 Q_INVOKABLE configurationBaselineIdentity(const QVariantMap&) const，只对传入那份 snapshot 调共享 [stableConfigurationBaselineIdentity](FleetConfigurationBaselineEvidence.md)。错线程返回固定 invalid；不查当前 Registry、不回调、不改审阅、不认证来源或授飞行许可。Guided 用它比较旧/新原始快照，不能用当前记录替换旧输入。

capabilityReviewEvidence 独立升 schemaVersion=2，顶层8键不变，member 从7变9，加入 configurationBaselineIdentity 与 currentConfigurationBaseline 并纳入原 members 哈希。captureComplete 保留原条件且要求新身份 available，但不要求正向 baseline=true；完整负面证据可被完整归档。原能力context6/approval14和计划顶层 schema 不变，旧归档不自动拥有新依据。

Guided 仍整体保存到既有 reviewEvidence；Deployment 补归档与实际审阅相等检查，并保留当前实际来源门禁。这里的源标签/哈希不是认证。未编译、测试或运行。

## 417：拥有固定文件重载工作流

新增唯一 owned [FleetFirmwareCatalogReload](FleetFirmwareCatalogReload.md)，复用已有 AppData 固定目录路径。只读 QObject 属性仅由 Guided 私有实际来源消费；公共界面取得值投影。`ownsFirmwareCatalogReload` 只核本实例的实际 Reload/Catalog 地址、parent 与线程，不按 UUID 搜索或建立权限。

目录 changed 继续 clear 原任务审阅；不清 Run、不释放占用、不发送指令。原始启动自动 load 和 `firmwareAdapterCatalogLoadStatus` 仍只表示构造时尝试，不能拿它当后续重载结果；运行中结果看新工作流的提交回执。原生界面/签名/执行标志未升级，未编译或运行。

## 543：同一次稳定观察的四速建议

`executionProfileObservation` 顶层现为固定20键，在原19键上新增 `speedSuggestion`；原 `ok`、12参数行、内容 hash／来源指纹和失败语义不变。建议只从第二次采集并已通过完整稳定行相等检查的12行构造，在最后 `parameterObservationsMemoryMatch` 整组内存尾栅之前完成；尾栅失败仍返回原观察失败及不可用建议，不发布成功建议。无额外参数请求、写入、刷新、采纳或保存。

`speedSuggestion` 固定9键：`schemaVersion=1`、`type=observed-parameter-speed-suggestion`、`usable`（实际 Bool）、`reason`（非 null String）、`speeds`（Map）、`sourceParameters`（Map）、`returnSpeedPolicy=inherit-observed-cruise-for-return-nav-not-rtl`、`configurationOnly=true`、`performanceCertified=false`。`usable=true` 时 `reason` 为显式空串，`speeds` 固定四个实际 Double；不可用时 `speeds={}`，不放0或-1作未知速度。

四个长键均为 `cruiseSpeedMetersPerSecond`、`returnSpeedMetersPerSecond`、`climbSpeedMetersPerSecond`、`descentSpeedMetersPerSecond`；`sourceParameters` 始终以这四键明确映射到 `MPC_XY_CRUISE`、`MPC_XY_CRUISE`、`MPC_Z_V_AUTO_UP`、`MPC_Z_V_AUTO_DN`。该映射是固定推导规则，不是额外参数观察。返程速度是显式采用所观察巡航值的返程 NAV 请求建议，**不是独立 RTL 参数、实际返航速度或返航性能证明**。

仅当两次实际捕获的 `context.px4Multirotor` 均为真且原实际 Vehicle 当前仍为 PX4 多旋翼时可给建议。巡航／返程须在现有 profile 的0.1–300m/s，升／降须在0.1–20m/s；巡航不得大于 `MPC_XY_VEL_MAX`，`MPC_Z_V_AUTO_UP`／`MPC_TKO_SPEED` 不得大于 `MPC_Z_VEL_MAX_UP`，`MPC_Z_V_AUTO_DN`／`MPC_LAND_SPEED` 不得大于 `MPC_Z_VEL_MAX_DN`。不按MAX生成建议、不夹值、不补默认值。原观察可以成功而建议不可用；成功的12项来源观察不等于参数彼此兼容或完整 native review 通过。

不可用理由闭集为 `execution-profile-observation-unavailable`、`execution-profile-speed-suggestion-px4-multirotor-required`、`execution-profile-speed-suggestion-parameter-values-invalid`、`execution-profile-speed-suggestion-profile-range`、`execution-profile-speed-suggestion-xy-limit`、`execution-profile-speed-suggestion-z-limit`。它们不替换原观察 `reason/reasons`。配置类别是原生当前报告的类别，不是可信硬件身份或真实性能认证；后续完整版本／目录／其余参数／动作／高度／路径／健康／账号与启动门保持。

Editor 只能显式将完整四速建议采纳为本页拟值，不能自动保存；采纳前仍须重读原观察并比较原来源指纹／参数 hash 和完整建议，保原 Draft 修订与编辑会话。真正保存继续走完整 `setExecutionProfile` normalize 与原 CAS，不把建议 Map 或 caller 参数值当新授权。仅源码修改和直接静读，未编译、运行实现、测试、仿真或放行。

## 416：只读逐机执行参数观察（原19键，543增加一键）

新增 `Q_INVOKABLE QVariantMap executionProfileObservation(const QVariant& vehicleId) const`，从实际 MVM 的唯一同号 Vehicle 固定原 model、Vehicle、PM、VLM、PRIMARY 地址与共享所有者，再复用 `FleetMissionVehicleEvidence::capture` 的12参数采集和原内容 hash 算法。它不要求已有 `executionProfile` 或已绑定 hash，不发参数 refresh/read 请求、不写参数、不认证硬件、不生成 review/账号 proof 或飞行权限。目录不匹配与缺 `requiredProfileHash` 不阻断“取得实际参数待绑定 hash”，但后续正常 native review 不因此放宽。

原固定19键为：`schemaVersion=1`、`ok`、`vehicleId`、`componentId`、`parameterProfileHash`、`observationFingerprint`、`managerEpoch`、`parameters`、`parameterCount=12`、`maximumParameterAgeMs=30000`、`source=live-param-value-receipts-only`、`sourceCurrent`、`allParametersUsable`、`reason`、`reasons`、`hashPurpose=operator-profile-binding-not-authorization`、`hardwareIdentityAuthenticated=false`、`flightCommandReleased=false`、`executionPermitted=false`；543另加上述 `speedSuggestion`，当前固定20键。非法/未知 ID 投影为0；失败可保留有界诊断，只有成功时可绑定 hash/指纹非空。

参数固定为 `MIS_TAKEOFF_ALT`、`MPC_LAND_SPEED`、`MPC_TKO_SPEED`、`MPC_XY_CRUISE`、`MPC_XY_VEL_MAX`、`MPC_YAW_MODE`、`MPC_YAWRAUTO_MAX`、`MPC_Z_VEL_MAX_DN`、`MPC_Z_VEL_MAX_UP`、`MPC_Z_V_AUTO_DN`、`MPC_Z_V_AUTO_UP`、`NAV_ACC_RAD`，按名称排序。每行必须是 live-param-value、当前同一 PM epoch、正语义 revision、原主链路来源、observed/available/fresh/缓存相符/rangeValid/usable，年龄0–30000ms且无待写；仅旧内容可知或非空 hash 不足。公开诊断行包括 name/component/age/source/type/revision/epoch 与7个 bool，真实有限 value 存在才提供；未知类型为-1、revision为0、epoch为空，不放无效 QVariant 当新值。

`parameterProfileHash` 保持原排序 `{name,mavParamType,value}` 内容数组的 compact JSON SHA-256；`observationFingerprint` 另保留实际车辆身份、共同 PM epoch、逐项语义 revision/来源/可用状态与内容，剔除持续增长的 age/receipt、缺 profile 理由和目录波动。Guided 保存前须重新读取并精确匹配原 fingerprint，且参数 hash 等于本机 `verticalPolicy.requiredProfileHash`；内容 hash 不是替代来源身份，指纹也不是授权或硬件认证。

全部可回调的来源检查与最终 capture 后，Coordinator 调用新增 native `ParameterManager::parameterObservationsMemoryMatch` 整组尾栅，再发布成功值。该尾栅按原PRIMARY地址/弱共享控制块、PM epoch、固定12行的语义revision/type/value、缓存吻合、未更新/无待写及当前年龄核对；不再调用 Link 虚函数、不锁 weak link、不请求/写参数。它关闭“后机/后参数来源回调改变先前参数，但复制行未更新”的尾窗，不把同时比较12行说成机载原子采样。原单项 getter/source matcher 同时补原QQ/账本代次/链路检查，回调销毁或换源不继续读旧成员。

本接口只提供操作员意图绑定依据，不是12参数原子事务或完整 StartPermit；之后实际 native review 继续复核参数、版本、适配器、计划及审批。Editor 的完整启动 JSON可空但不能据此启动；原25目标/六类、可信 HOME/local/净空、固件目录真实操作入口与正式机载结果继续。阶段仅416，未编译、测试、运行 helper/UI/认证/应用/设备、改 PX4、刷机、更新 PR 或打包；下方386–390及更早为历史。

## 386–390：本地能力审阅证据进入提案与归档

协调器新增 `capabilityReviewEvidence`，合同见 [FleetCapabilityEvidence](FleetCapabilityEvidence.md)。它从所提供 fleet 值快照按唯一车辆 ID 排序，保留通过 shape 校验的完整 `capabilityReviewContext`/`capabilityApproval`、稳定审阅身份、`currentLocalReview` 和当前 `reviewedCapabilities`，并给出组级 `evidenceFingerprint`。来源明确为 `supplied-fleet-value-snapshot`，`hardwareIdentityAuthenticated=false`、`performanceCertified=false`、`flightCommandReleased=false`；不是额外原生遥测通道。

`captureComplete` 与 `currentLocalReview` 是不同判断：完整捕获快照上下文不表示每机已有当前批准，更不表示硬件、性能、账号或飞行安全已经认证。是否要求当前工程审阅仍由任务的显式 verified/baseline/能力政策决定，不默认把所有普通提案改成 verified-only。裸 verified 列表、AUTOPILOT_VERSION、组件元数据或名为 trustedManifest 的输入不能自动提供当前工程批准。

该证据同时进入 report 的 evidence 和 review result。Guided 将 `review.capabilityReviewEvidence` 纳入实际 PlanStore `reviewEvidenceHash`，机队稳定指纹也带本地审阅 epoch/revision/状态及静态声明；需要新审阅的变化不能只凭同标签恢复旧批准。历史档案仍不恢复账号、批准或控制权。`nativeEvidenceCurrent()` 的原生 Vehicle/参数/版本来源检查并不单独认证该能力 map；控制器仍须复核其实际 Registry 当前来源与完整审阅绑定。

显式 assignments 保留，不由角色建议覆盖；[Compiler](FleetMissionCompiler.md)现在会在最终覆盖成员及备用机确定后，核原 fleet 唯一 ID 的全局/实际 role/verified/baseline 政策，避免未请求 role allocation 时漏检。当前有效工程声明可正向补入普通规划，但 token 审阅不是数值性能或能量模型认证。

本轮没有新增命令、账号授权界面或硬件自动发现。角色等价替补、真实模型、企业权限与原25目标/六类继续；限定源码读回未发现确定新增问题，不等于编译、运行或安全通过。本批未编译、测试、运行 helper/认证/应用/UI、仿真或设备。

`FleetMissionReviewCoordinator` is the ground-station-only composition layer for an intent preview. It joins the value-snapshot stages into one review artifact:

1. `FleetRoleAllocator` proposes roles and reserves from the supplied fleet snapshot.
2. `FleetMissionCompiler` normalizes the task and route evidence.
3. `FleetEnergyScheduler` checks duration, return reserve, and energy evidence.
4. `FleetAirspaceConflictChecker` reviews the supplied paths and no-fly zones.
5. `FleetMissionRiskEvaluator` combines plan, health, energy, and risk evidence.
6. `FleetMissionReportBuilder` creates an audit-oriented report.

Explicit `holdSeconds` now has a separate `energyPlan.relativeDwellBudget`.
Outbound and return holds are counted separately, enlarging the same member's
generated travel estimate and existing duration/reserve budgets. A hold-only
lower bound does not fill otherwise unknown mission duration. Known durations
or compatible numeric time windows shorter than the relevant hold sum, and
authoritative adjacent arrival intervals shorter than the prior waypoint hold,
are invalid; a last-point hold can occur after its final arrival timestamp.
Estimated labels are not promoted to authority. The scan is bounded to 512
members and 32768 total points; invalid input has no partial budget prefix.
These are resource limits, not fleet-size safety claims. Launch, landing and
unlimited terminal hold dynamics remain outside the duration model, and neither
this budget nor an execution profile certifies arrival schedules or flight.

`options.events` is an optional value snapshot. When supplied, the
coordinator forwards it to `FleetMissionReportBuilder` so the report can
include normalized timeline/incidents. `GuidedActionsController` supplies the
current `FleetEventBlackBox.events` when the caller does not provide an
alternative snapshot. This is still read-only evidence: it does not persist,
mutate, or dispatch a mission.

The `review()` result contains each stage under `plan`, `rolePlan`, `energyPlan`, `capabilityReviewEvidence`, `airspaceReport`, `riskAssessment`, and `report`. Missing or unknown required evidence is retained as `needsReview`; it is never treated as a safety pass. Explicit task assignments remain authoritative but are still checked against the task's explicit capability policy by the compiler. Role suggestions are injected into compiler input only when the task explicitly requests roles and omitted assignments, and they remain marked for manual confirmation.

Member routes, return paths and explicit route schedules survive compilation.
In coverage mode, each member's independent route is authoritative for review;
a global route is not silently copied onto auxiliary roles. Generated route
distance and constant-speed duration remain estimates, not authoritative 4D
timestamps. Task time limits and deadline windows are planning budgets, not
actual route timing. The energy budget conservatively includes supplied
physical durations, per-member route estimates and upper time budgets; missing
return/energy evidence is not supplied by the coverage generator.

The airspace checker treats estimated/unknown route timing spatially, including
time-windowed no-fly zones, rather than suppressing a possible conflict using
synthetic times. Return timing has its own schedule and does not inherit the
outbound start/duration as if the aircraft flew both paths simultaneously.

The controller may set `options.healthSnapshotMismatch=true` when a caller's
health list differs from the current `FleetHealthCenter` snapshot. That flag
is a blocking review condition even if the current snapshot is otherwise
complete; the coordinator never treats a stale caller-owned list as an
approval source.

This is a proposal/report path only. `manualConfirmationRequired` is always `true`, `flightCommandReleased` is always `false`, `commandDispatch` is `not-released`, and `missionMutated` is `false`. Value-only `review()` does not access native vehicles. The opt-in native path below only observes native data: it does not send MAVLink, request/write parameters, upload missions, run a simulator, or call command handlers. The engineer must compile and validate the integration before any operational use.

Endpoint coverage proposals keep ingress/survey sections and an independent
return path. Energy reserve is at least the greatest supplied/default reserve,
explicit return duration, or generated return estimate. The latter remains a
separate estimate with `routeTimingCertified=false`; it does not make return
timing authoritative. The controller can persist the complete plan plus bounded
review identity in `FleetMissionPlanStore`, without duplicating giant report or
event trees. Every fresh review has its own record revision even for identical
compiler plan IDs; restored history is never an approval token.

## Native source evidence and encoding preview (2026-10-14)

`reviewWithNativeEvidence()` captures the actual, uniquely registered `Vehicle`
and its exact `ParameterManager` for each compiled member, up to 32 unique
system IDs. It calls the read-only [`FleetMissionVehicleEvidence`](FleetMissionVehicleEvidence.md)
helper, not a caller-supplied evidence map. Wrong-thread or missing/replaced
native sources fail closed. The existing `review()` remains a value-only
planning API; it cannot be used as the live approval/deployment source.

Both APIs retain `transportEncoding`, a bounded preview of the existing
payload builder: wire/content identity, item/route counts, requested semantics,
source-mapping counts and unencoded requirements. It does not duplicate the
full wire payload or manifest into review history. An explicit profile that
cannot encode adds a blocking review reason. Native mode also adds blocking
reasons for incomplete capture and, when an execution profile is requested,
unusable live parameter evidence or a missing/mismatched parameter-profile
binding. Geometry-only proposals do not invent an execution profile.

`nativeVehicleEvidence` contains the per-member observations and a stable group
fingerprint. Actual parameter values/types, source/content revisions, manager
epoch, fresh/stale transitions, cache agreement, pending writes, identity and
native context affect that fingerprint. Continuously increasing age does not;
same-value receipts alone do not replace content identity. Capture completion
only means bounded source capture, not parameter eligibility or flight safety.

`nativeEvidenceCurrent()` freezes the original review and source instances,
re-observes them without requests, and compares stable fingerprints. It rejects
clear/new-review reentry using a local non-wrapping invocation epoch, deletion,
same-ID vehicle/manager replacement and evidence invalidation/expiry. A new
review returns its own local result after direct `reviewChanged` callbacks,
never a newer nested result. This is source-level lifecycle protection, not a
runtime, thread-safety or malicious-plugin proof.

The MERIVUS controller uses native review, includes both new evidence maps in
`FleetMissionPlanStore`'s `reviewEvidenceHash`, and rechecks source continuity at
approval and upload boundaries. Historical restored records remain non-live.
`parameterEvidenceUsable` is not an approved firmware adapter, certified
MISSION_INT support, takeoff/landing clearance, a startup permit or completion
evidence. All flight/execution flags remain false. The default parameter age
limit is 30 seconds; expiry requires fresh real receipts and a new review,
not an implicit refresh or cached value promotion.

## Version-source and native catalog binding (0.3.0)

The coordinator owns a private [`FleetFirmwareAdapterCatalog`](FleetFirmwareAdapterCatalog.md)
child; `firmwareAdapterCatalogBackend()` is a non-QML C++ configuration getter.
QML sees only readonly catalog/load-status value maps. At construction it can
load `AppData/fleet-os/firmware-adapters-v1.json` with revision CAS if the file
exists. An absent or invalid file does not create a default reviewed adapter.
Load status describes that startup attempt, not current directory health.
Only native backend methods explicitly register/revoke/import/load/save;
external file edits are not an automatic directory update or authentication.

Native group evidence now keeps a complete directory snapshot once, while each
member carries its small catalog binding/match and separate real version
observation. `allFirmwareObservationsUsable` and `allAdapterDeclarationsMatched`
remain distinct from parameter evidence. For an explicit execution profile,
missing/stale real version evidence or an unmatched declaration is a blocking
reason, even if the cache says capabilities are known. A geometry-only proposal
does not invent an execution profile or a reviewed firmware contract.

Directory change/destruction clears live review; `nativeEvidenceCurrent()` also
freezes and compares the exact catalog instance and source identity. Version
age is excluded from the stable hash but fresh/stale transitions and first/new
version anchoring revoke old parameter evidence. Group source/evidence maps
remain included in the exact archive review hash and upload gate.

The controller records a bounded native-review/coverage summary, aggregate
source hash and exact archive reference instead of repeating raw parameters,
version fields, reports and the complete catalog into a black-box event. Full
review evidence remains in the plan archive when recording succeeds; summaries
explicitly declare `fullSnapshotIncluded=false` and no restored command approval.
Neither declaration matching nor the summary is a signed build artifact,
startup permit, real takeoff path or mission-completion proof.

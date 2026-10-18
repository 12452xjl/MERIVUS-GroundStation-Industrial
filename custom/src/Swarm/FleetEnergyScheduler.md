# FleetEnergyScheduler 能量与备用调度契约

## 543：消费本机路线修正后的既有预算

本模块代码和exact5逐机账单格式不变。上游 [Coordinator](FleetMissionReviewCoordinator.md) 现在可用Compiler重新生成的本机名义路线航行时间加驻留，提高已有任务/返程预算；原 `budgetDetails` 保来源、未知项、修正前值及是否提高，现有明细浏览无需换协议。仅名义航线不能建立缺失的完整任务时长，不能代替工程性能声明、当前原生电池来源或产生新批准；更高requiredSeconds仍由现有预算、margin、投影与资格链消费。未运行Scheduler、测试或编译。

## 516：近期原生 SOC 下降窗只降低既有已知预算

新 sibling 经 [FleetNativeBatteryEvidence::inspectDrain](FleetNativeBatteryEvidence.md) 同一纯值校验。存在坏 sibling 使原成员以 `native_battery_drain_evidence_invalid` 拒绝，不静默变 missing；valid unknown／warming／stale 保诊断但不补能量模型。旧来源、辅助电池负面、健康、当前声明和严格性能前置保持；窗口不能替代 `requirePerformanceEvidence=true` 所需当前声明满电续航和已知电量。

先按原 direct／声明 endurance／legacy drain 模型计算，再只对已知 available 取 `min(oldAvailable,windowRemaining)`；原 energyKnown=false 不会因窗口变 true。独立投影也只在原 projectionKnown=true 时计算 `min(oldProjected, clamp(SOC-rate*(requiredSeconds+effectiveAge/1000),0,100))`，其中 effectiveAge=max(最新原 RAW 龄,历史下降端点龄)，不让 flat 包复新预算。确实更低时对应 basis 才标 `native-soc-decline-window`，否则保原 basis；之后重新计算 margin、资格、替补阈值、score 与组级瓶颈／缺口。窗口不改变 requiredSeconds、成员、主备角色或原 budgetDetails。

仅有新 sibling 时新增 top `nativeBatteryDrainAdjustments`。每行固定14键：`vehicleId,present,inputValid,usable,reason,evidence,availableKnownBefore,availableFlightSecondsBefore,availableFlightSecondsAfter,availableLowered,projectionKnownBefore,projectedBatteryPercentBefore,projectedBatteryPercentAfter,projectionLowered`。evidence 是完整合法25／8诊断（坏型为空）；未计算／未知值明确-1、known=false，不能把原被拒成员写成已比较。reason 区分原 unavailable、`member-not-evaluated`、`existing-estimates-unknown`、`not-lower-than-existing-estimates` 与 `conservative-estimate-lowered`；原拒绝原因仍在 rejectedVehicles。bound 先按完整预算顺序、独立模式先按选中顺序，剩余被观察但拒绝／未选成员跟随，duplicate 只保无来源 invalid 诊断，不能借同ID另一行。

assignment 保原 legacy18／strict21，ReserveEnergyPlan29／32 与编译预算不扩；missing sibling 不新增此 top，保既有输出 shape。Coordinator 的完整 review／energyPlan、历史工程报告与515逐层明细可直接保存／浏览该 top，不把新列表塞进闭合旧账单或重写旧归档。所有结果仍是报告与声明的线性估算，既不保证实际续航，也不授予审批、上传或飞行许可。

Root 以 Coordinator 共享身份检查窗口语义，并在批准期重采实际 Registry 值、复核扣龄预算及原替补阈值；旧 review 不被重算覆盖。只读降额不是自动返航／替补／命令。当前仅源码及静态阅读，未编译、运行、测试、仿真、刷机或放行；known-only/min、flat／过期、坏值、严格声明缺失、负面健康、主备完整账单与审批边界均待工程师验证。下方是历史合同。

## 483：严格逐机估算只消费当前声明续航

既有 `requirePerformanceEvidence=true` 通过[共享声明](FleetCapabilityEvidence.md)核当前工程批准和完整基线；每架主／备用机还须声明续航>0、原电量有效。可用时间以声明满电续航×原电量比例估算，投影同源；不使用metadata消耗率、untrusted直接剩余时间、旧endurance alias或投影fallback独立满足严格准入。原真实native电池inspect的鲜度／故障／不安全状态检查不改；其已验证的实际direct时间只可额外取min，不能在缺声明续航或电量时独立通过。

严格assignment在原18键基础增加 `requirePerformanceEvidence,declaredPerformanceEvidence,declaredPerformanceEvidenceUsable`，根保完整有序sources及策略。声明无效、缺所需数字、重复ID或预算不完整的成员不能从另一架机借证据；usable仅说明exact11有效，声明只有速度known仍不能通过能量估算。false／absent维持旧读取、公式和结果shape，不自动升级历史规划。

此为声明性线性预算，不是真实电池／载荷／风／老化模型或保证续航。现有显式task／Intent.parameters可提出策略；没有新UI／Draft.settings字段或性能录入器。原native来源和Coordinator审阅身份仍必需，全部权限及性能认证false。30项交接全部未执行；未编译、执行或测试。下方历史18键说明适用于非严格结果。

## 460：共享原生电池故障校验进入原逐机能量账单

`plan()` 改为调用 [FleetNativeBatteryEvidence::inspect](FleetNativeBatteryEvidence.md)，不再维护第二份匿名电池解析器。共享结果的 `required/valid/usable/evidence/reason` 直接接入原拒绝路径；合法信封在拒绝前保留，坏信封不伪造来源。原预算、主备身份、输入顺序、能量公式、扣龄检查、未知值、备用完整性和 `assignments` exact18 均不改变。

原 schema1 的 outer20／row15 继续接受；新增原生 schema2 仍为 outer20，逐槽 row19 增加 `chargeStateKnown`、`chargeState`（未知 -1，已知 1–255）、`faultBitmaskKnown`、`faultBitmask`（未知 -1，已知 0–UINT32_MAX）。共享校验保留原全部行、来源 epoch、序号、年龄、百分比和剩余时间标量一致性检查，不把坏类型转换为空或未知后通过。

在当前来源中，任意 fresh 电池行（包括辅助电池）明确报告非零 fault mask，或 charge state 为 2–7／尚不支持的更大状态值，都会使 `reportedUnsafe=true`、`usable=false`。Scheduler 以 `native_battery_reported_fault` 或 `native_battery_reported_charge_state_unsafe` 保留原成员的拒绝来源；高电量、静态满电续航、直接剩余时间和其他备用机不能覆盖此拒绝。主机和备用机共用这一检查，原 RiskEvaluator 的绑定成员／备用资格阻断继续消费结果。

缺扩展、charge state 0 和未提供故障依据仍是未知，不自动认证健康，也不因此一律阻断原 schema1／独立无 marker 计算。全部 known／fresh／reportedUnsafe 仅描述所收到报告及本地来源时效，不证明硬件身份、真实电池性能、未来续航或飞行安全；未新增认证、命令或启动许可。仅源码与静态读回／文本检查，未编译、测试或运行。下方为历史合同；其中 row15 对应 schema1，当前新原生输出为 schema2／row19。

## 432：备用机使用独立待命／完整路线重做预算

实际 Coordinator 现在从编译后的 `reserveMembers.energyPlan` 取得[备用独立估算](FleetReserveEnergyPlan.md)，不再以最长主机任务／返程替代。`memberEnergyBudgets` 仍 exact5，输出 assignments 仍 exact18；新增信息放在原 `details`／`budgetDetails`，不把大路径复制进有限账单。

新备用 details 固定标记 `basis=compiled-reserve-route-budget`、严格 Bool `reserveEnergyPlanRequired=true` 和 `reserveEnergyPlanComplete`，以及明确 String `reason`。完整时 reason 为空、任务秒数为正、返程秒数非负；不完整时 reason 非空且两项秒数都精确为数值 -1。单独缺标记、假布尔、错误分组／basis、半未知预算或坏 details 仍是整份预算结构错误。完全不带两标记的独立 legacy 调用保留原正数合同，不因此获得原生来源或实际任务资格。

合法“不完整备用行”不抹掉主机账单，也不把整个列表误报为结构坏输入；该备用永不 eligible，即使 `requireEnergyEvidence=false`。其 `requiredFlightSeconds=-1`、能量结果未知，不把 -1 + -1 算成所需 -2 秒。若先因来源或健康拒绝，保留该原因，同时 budgetDetails 仍显示缺失预算；未知行使组级瓶颈未知、缺口合计不完整。Compiler 的缺预算阻断和 RiskEvaluator 的 `planned_reserve_energy_ineligible` 保持，不以主机可行覆盖备用缺口。

预算公式及共同下限见[协调器](FleetMissionReviewCoordinator.md)。待命系数是工程声明；飞控 time_remaining 的负载基准未必与声明的名义飞行负载一致，比较只能是未标定估算。这里没有剩余航段续接、自动替补、实际起降／避障模型或飞行许可。431 的实际电池来源、鲜度与审批扣龄检查保持。未编译、测试、运行；下方430“备用取最长主机预算”仅为历史状态。

## 431：原生电池报告来源与审批时扣龄核对

真实 Registry 快照现在设置 `nativeBatteryEvidenceRequired=true`，并从其私有绑定的当前 Vehicle 读取 `nativeBatteryEvidence`；不能由护照、历史元数据或旧核心电量补回当前来源。Scheduler 对存在的 marker 要求严格 Bool true；false、坏类型、坏 envelope、失源、超过 3000 ms、未知用途、多推进电池或槽溢出均拒绝，不能借静态满电续航和旧百分比绕过。只有完全没有 marker 的独立 legacy 输入保留原有纯计算接口。这项检查是来源一致性和本地收包时效门禁，不是身份认证或真实电池性能保证。

输入 envelope exact20：`schemaVersion`、`vehicleId`、`componentId`、`sourceEpoch`、`sourceCurrent`、`rows`、`aggregationState`、`batteryPercentKnown`、`batteryPercent`、`remainingSecondsKnown`、`remainingFlightSeconds`、`reportedRemainingSeconds`、`ageMs`、`batteryId`、`receiptSequence`、`contentRevision`、`readonly`、`authenticated`、`performanceCertified`、`flightCommandReleased`。核对 schema/身份/UUID 或明确空来源、规范十进制计数、固定只读否定权限标志、known 与数值 sentinel；最多 16 个不同电池 ID 的 exact15 原生行也检查类型和有限范围。只有 `reported-estimate`、连续来源及唯一 fresh ALL/PROPULSION 行可用，top 聚合字段必须与该行一致；辅助 AVIONICS/PAYLOAD 不加入推进预算，未知用途不能忽略。`ageMs` 是本地收包年龄，不证明远端测量年龄。

Registry 顶层 `batteryPercent` 必须等于 native top，未知为 -1；`remainingFlightSeconds` 只在 native known 时存在。出现的旧 `battery`/`availableFlightSeconds` 别名也必须一致，不得成为旁路。原报告剩余秒数必须为正；扣龄值为 `max(0, reportedRemainingSeconds - ageMs / 1000)`。原生直接依据命名为 `autopilot-reported-estimate`，并与其他有效模型取保守最小值；不是实测或经标定的续航。唯一 fresh 电池只提供有效百分比、未提供剩余秒数时，仍允许按已有静态满电续航/消耗率计算，不能把缺少该协议字段当作必须永远拒绝的机型条件。

每行输出改为 **exact18**：下方原 exact17 字段加 `nativeBatteryEvidence`。合法 native envelope 原样保留（包括最多 16 槽），即使该原成员因健康、拓扑或能量被拒绝；缺失/损坏 envelope 或 duplicate 身份不伪造来源，输出空 map，并保留拒绝原因。新增拒绝码为 `native_battery_evidence_invalid`、`native_battery_source_unavailable_or_ambiguous`。旧 standalone 账单的此字段为空，不宣称原生证据。

Guided 的审阅指纹绑定 source epoch、内容版本、来源连续性、聚合状态、known、原始报告值和电量；不纳入自然增加的 age、重复同内容 receipt 计数或扣龄秒数。审批检查两次重新读取同一实际 Registry 路径，覆盖已审阅 primary 与 reserve：来源/内容变化或过期要求重审；在仍为同一报告的情况下，当前扣龄剩余秒数跌破原账单 `requiredFlightSeconds` 也拒绝。无剩余秒数时不虚构该依据，保留原静态模型政策。该只读检查不改写旧账单、不重新选择成员、不授予启动许可。它不保证之后的真实耗电、任务完成或传输至飞机时仍然充足。

界面只增加审阅采样时的来源、年龄、状态和原报告/扣龄时长说明，不新增交互、IO、轮询或飞行操作。本轮只做静态源码检查；原生消息收取、刷新/过期、来源切换、重复报告、协议 sentinel、双电池拓扑、逐机拒绝来源与审批预算边界仍须工程师实际验证。

## 430：编译成员绑定的逐机能量账单

真实 Review 路径现在由 Coordinator 从实际编译 `plan.members` 和 `plan.reserveVehicleIds` 生成 `memberEnergyBudgets`；不把用户提供的同名字段当作原生任务分配。Scheduler 按每机任务/返程预算分别计算余量，保持原主机和备用机身份与输入顺序。不合格原成员仍有账单和原因，高电量备用机不能被内部排序替换成另一架主机。没有此字段的独立 `plan()` 调用保留原推荐排序，但不是绑定账单，不能据此证明另一个任务的原成员都有能量。

`memberEnergyBudgets` 是 1–255 行列表，每行 exact5：`vehicleId`（唯一数值整数 1–255）、`group`（`primary`/`reserve`）、`missionDurationSeconds`（有限正数）、`returnReserveSeconds`（有限非负数）、`details`（Map）。身份集合必须与非空 `candidateVehicleIds` 完全一致，primary/reserve 数量分别等于 `vehicleCount`/`reserveCount`，逐行及总预算求和不得溢出。坏预算使 `parametersValid=false`，不输出部分账单、不回退重新选机。`details` 只作说明，不改变计算或授权，限 JSON 类型、深度 8、4096 节点、64 KiB；原生空值与 JSON null 均保留。

Coordinator 复用原有 duration、相对停留和返程模型，逐机计算进场/作业路线估算、停留及返程保留量，并保留共同显式任务时长与 time limit 的保守下限。备用机尚无真实接替路线，使用主机最长任务预算加最长返程预算，并明确标记这不是备用路线、待命耗电、性能等价或已完成接替。起降动力学、风、载荷、温度、电池老化、非线性压降仍未建模；不得把预算缩短理解为真实性能认证。

### 能量依据与百分比修正

`batteryPercent`、`linkQualityPercent`、`enduranceSeconds` 的精确数值 `-1` 与 Registry 的 unknown 口径相容；它不产生已知证据，也不等于 0。其他负值、字符串、布尔、null、非有限值及越界值仍拒绝。`enduranceSeconds=0` 仍非法；直接剩余秒数 0 表示已知无剩余时间。`minimumBatteryPercent>0` 时未知电量不能满足门槛；显式取消该门槛和 `requireEnergyEvidence=false` 不会把未知预测变成已知。

对每机同时提供的有效依据取最小可用秒数，并用 `energyBasis` 说明实际限制来源：

- `direct-remaining`：直接剩余续航秒数。
- `percent-drain-rate`：当前电量百分比 ÷ 每分钟消耗百分点 × 60。
- `full-battery-endurance`：满电续航秒数 × 当前电量比例。
- `unknown`：没有可计算依据。

百分比预测独立使用 rate/endurance 中较大的消耗率，`projectionBasis` 仅为 `percent-drain-rate`、`full-battery-endurance` 或 `unknown`。满电续航模型按“当前电量 - 100 × 所需秒数 / 满电续航秒数”计算，不再乘以当前剩余比例而低估耗电。例如 50% 电量、满电 600 秒、预算 120 秒，模型预测为 30%，不是 40%；这是公式说明，不是本轮运行测试。仅有直接剩余秒数不能推出百分比预测，保持 `projectionKnown=false`、`projectedBatteryPercent=-1`，不把当前电量伪装为预计剩余电量。已知预测只在完成计算后夹到 0–100%，低于零的实际时间缺口仍由 margin 保留。

这些是所给值的估算，不证明实测性能或电池寿命。431 已接入上方被动原生报告及本地收包年龄边界；静态满电续航声明仍不等于已完成真实电池模型，legacy 输入仍不自带来源或时效保证。

### 输出合同

`boundAssignments=true` 表示采用完整绑定预算。`assignments` 包含每个原成员（含不合格成员），430 原有 17 字段如下（431 添加 `nativeBatteryEvidence`，当前共 exact18）：`vehicleId`、`group`、`batteryPercent`、`availableFlightSeconds`、`requiredFlightSeconds`、`marginSeconds`、`projectedBatteryPercent`、`replacementRecommended`、`energyEvidenceKnown`、`eligible`、`reason`、`missionDurationSeconds`、`returnReserveSeconds`、`budgetDetails`、`energyBasis`、`projectionKnown`、`projectionBasis`。eligible 成功的 reason 为明确空字符串，失败为非空原因。缺失、重复、离线或坏输入的原成员不能借用同号其他行的数据。

`selectedVehicleIds` 仅包含原 primary 中合格的身份，`reserveVehicleIds` 仅包含原 reserve 中合格的身份；`feasible` 仅在参数合法且所有原 primary 合格时为 true。备用不足仍进入原风险消费者，不能以主任务可行覆盖备用缺口。所有实际规划建议仍要求人工审阅，不发送飞行命令。

组摘要新增：

- `bottleneckKnown`、`bottleneckVehicleId`、`bottleneckMarginSeconds`：全组账单都有可计算能量时，报告最小 margin 的机体；相同时保持原预算顺序。任何未知则 known=false、ID=0、margin=-1，不能用已知子集冒充全组瓶颈。
- `totalShortfallSeconds`：已知负 margin 绝对值之和；不是任务延迟、总航程或消耗电量。`shortfallComplete=false` 时只表示已知缺口下界，不能把未观察到的余量当作充足。
- `shortfallComplete`：参数合法、账单非空且所有账单的能量均已知；不是遥测完整性或真实续航认证。
- `unknownEnergyVehicleCount`：本次账单中能量未知的行数。
- `status`/`reviewStatus`：`invalid-input`、`estimate-incomplete`、`estimate-insufficient`、`estimate-reserve-shortfall` 或 `estimate-feasible`。绝不命名为飞行许可或安全通过。

必须依据 `energyEvidenceKnown` 判断 margin 是否已知；数值 `-1` 既可能是未知占位，也可能是真实计算的短缺 1 秒。预测同理由 `projectionKnown` 决定。旧独立模式的组摘要只覆盖其推荐输出，不冒充全 Fleet 或另一份编译任务。

新增或延续的逐机原因包括 `assigned_vehicle_missing`、`duplicate_vehicle_id`、`battery_evidence_missing`、`battery_below_minimum`、`energy_evidence_missing`、`insufficient_energy_margin` 以及原健康/非法输入原因。输入错另有 `invalid_member_energy_budgets` 风险。合法不足与结构错误保持区别。

实现仅静态读回，未编译、测试、运行 helper/应用/仿真或设备。工程师后续须验证不同长度路线、共同时间预算、停留/返程计数、缺成员/重复身份、备用不替主机、三种能量依据冲突时的保守选择、unknown 哨兵、百分比预测与边界值，以及实际遥测/电池标定。

## 原有独立调用与共同输入

`FleetEnergyScheduler` 是地面站侧的纯计算模块。它按任务时长、返航余量、电池证据和替补阈值生成可审阅的主机/备用机建议；不会调用 `Vehicle`、MAVLink、任务执行器，也不会自动改写任务。

## 输入

任务参数至少可以提供：

- `vehicleCount` 或 `minVehicleCount`：主任务所需机数；
- `reserveCount`：备用机数量；
- `missionDurationSeconds`：任务预计持续秒数；
- `returnReserveSeconds`：返航、降落和保守余量的合计秒数；
- `minimumBatteryPercent`：候选机最低当前电量，默认 30%；
- `replacementThresholdPercent`：预计任务结束时触发替补关注的电量，默认 35%；
- `requireEnergyEvidence`：是否拒绝没有可解释能量证据的车辆，默认 `true`。
- `candidateVehicleIds`：可选的权威成员/备用机 ID 集合；由任务编译/角色分配传入后，能量调度只能在该集合内选机，不会从整支 Fleet 另选高电量旁路机体。

车辆条目可提供 `systemId`/`vehicleId`、`online`、`healthState`、`batteryPercent`，以及以下任一能量证据：

- `availableFlightSeconds`/`remainingFlightSeconds`：直接给出的剩余续航；
- `consumptionPercentPerMinute`：按当前电量计算剩余时间；
- `enduranceSeconds`：按满电续航估计，并按当前电量比例缩放。

缺少能量证据时，默认拒绝该车辆并返回 `energy_evidence_missing`。使用 `enduranceSeconds` 时应确认它代表满电估计；若它已经是剩余续航，应使用 `availableFlightSeconds`。

任务数、电量阈值、时长和 `requireEnergyEvidence` 现在按严格类型与范围读取：拒绝小数/布尔冒充整数、NaN/Inf、缺失或非正任务时长、负时长、阈值越界以及反向或溢出的时长组合，并以 `parametersValid=false` 和阻断风险 `invalid_task_parameters` 返回。`candidateVehicleIds` 也必须是非空、无重复的真实 1–255 整数列表，否则返回 `invalid_authoritative_vehicle_ids`。车辆 ID 限定为 1–255；电量、链路质量和能量证据出现但类型、有限性或范围不合法时拒绝该车辆，不再静默截断或夹逼。健康状态为 `degraded`、`warning`、`limited` 时返回 `health_degraded`，`fault`、`failed`、`unhealthy`、`emergency`、`lost`、`disconnected` 等终态时也不会进入候选集。

## 输出与边界

输出包含 `selectedVehicleIds`、`reserveVehicleIds`、每机 `marginSeconds`、`projectedBatteryPercent`、`replacementRecommended`、拒绝原因和风险信号。始终带有 `manualConfirmationRequired=true`、`flightCommandReleased=false`。

`feasible=true` 只表示所给估算证据满足计算门槛，不代表可以起飞。任务编译成员与逐机预算已按 430 接线，431 增加被动原生电池报告来源；实测耗能/返航模型及工程师编译、仿真、日志回放和硬件验证仍未因此完成。

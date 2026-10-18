# FleetMissionRiskEvaluator 契约

## 460：新增原生电池负面沿原主备账单阻断

本RiskEvaluator源码本批不改。[Scheduler](FleetEnergyScheduler.md)现复用[共享原生电池检查](FleetNativeBatteryEvidence.md)，所有指定主／备用的明确fresh故障／不安全charge，以及过期未解决负面均不得eligible。合法拒绝保原nativeBatteryEvidence，原assigned_vehicle_energy_ineligible／planned_reserve_energy_ineligible和blockingReasons继续进入Coordinator与实际审批／部署检查，不靠“有另一架满电飞机”替换原身份。

[HealthCenter](FleetHealthCenter.md)还独立消费原Registry负面；此处不是新耗电预测、传感／硬件认证、命令或许可。未编译、运行或测试，完整运动域、StartPermit和正式结果仍须开发。下方为历史。

## 447：阻断原因进入原生审阅门禁

新增原生 C++ 纯静态方法 `hasNoBlockingRisks(assessment)`，不暴露为 QML invokable，不修改评估结果或发送通知。只有 `inputValid` 是非null的真实Bool且为true，并且 `blockingReasons` 是非null的实际 `QStringList` 或 `QVariantList`、确实为空时，才返回true。缺失、null、字符串／数字／Bool／Map、其它可转换类型或任意非空列表均返回false；先核真实类型，再读取相应容器，不让坏值经宽松转换变为空列表。

当前 `evaluate()` 原生输出的空 `QStringList` 保持正常；经合法JSON值往返得到的空 `QVariantList` 也兼容。非空列表不论其中项是否有合法解释均阻断，不需要先将坏元素转换成字符串。`requiresOperatorReview=true` 是本模块固定的人工审阅提示，不能要求其为false；否则全部合法任务都会被误阻断。warning／minor不会仅因该提示进入阻断列表，但仍受已有其它风险及实际审批条件约束。

额外风险信号的同一个 `effectiveCode` 同时用于 `riskItems` 与 `blockingReasons`：原code／reason／type经原规则解析后为空时，统一使用 `caller_risk_signal`。因此未命名的critical／major等阻断风险不能在报告里出现、却因空code被 `appendUnique` 丢出阻断列表。已有严重级别归一化与rank规则不变。

[Coordinator](FleetMissionReviewCoordinator.md) 的 `needsReview` 消费该共享方法，不再只看输入结构有效；[Deployment](FleetMissionDeployment.md) 对冻结的实际审阅风险也作独立消费。此方法只回答这份输入的两个门禁字段是否满足合同，不验证风险真实性、来源完整性、净空或StartPermit；不会因手工构造两个合法字段就认证整份风险。DTO／schema及许可语义不变，`flightCommandReleased=false` 保持。Intent使用另一套FleetRiskRadar合同，不套用本方法。

本批仅源码与限定静态核对，未编译、测试或运行。工程师仍需验证原生QStringList／JSON空List正路径、缺失／坏型／typed-null、命名与未命名blocking／major、warning正常空列表，以及审阅→请求批准→批准→冻结Deployment风险的实际阻断和旧批准失效链。

## 430：逐机能量账单与实际主/备用身份核对

`energyPlan.boundAssignments=true` 时，风险汇总独立核对真实 `missionPlan.members` 和 `reserveVehicleIds`，而不是只比较数量。两组须为无重复、无交集的数值型1–255整数ID；成员别名若同时存在必须一致，数组最多255项。`assignments` 必须完整保留原主机和原备用机，包括不合格行；每行 `group` 仅为 `primary` / `reserve`，`eligible` 为真实bool，`reason` 为非null字符串（合格为空、不合格非空）。`selectedVehicleIds` / 能量 `reserveVehicleIds` 必须恰好等于相应组的合格子集，不允许备用顶替主机、遗漏拒绝行或加入别的飞机。

坏结构产生 `invalid_bound_energy_identity`，成员或选择集合错配产生 `energy_assignment_identity_mismatch`，二者均为输入错误和阻断风险。合法账单中原主机不合格则为 `assigned_vehicle_energy_ineligible`，不会仅因该业务结果将输入结构判错；若同一账单却声称 `feasible=true`，另报 `energy_feasibility_identity_mismatch`。指定备用不合格为 `planned_reserve_energy_ineligible`，原数量缺口检查仍保留。该核对不重新挑选飞机，也不推断未知能量已足够。

缺失或显式为false的 `boundAssignments` 保留旧独立调度输入，不要求新增逐机账单字段；但若提供 `selectedVehicleIds`，其合法集合必须对应实际任务成员，否则明确报告 `energy_selection_mission_mismatch`。旧模式ID仍兼容原数字字符串；新绑定账单只接受数值ID。绑定标记若提供坏类型，不会当缺省false忽略。未提供身份的旧摘要仍不被本模块升级为逐机身份已验证。

本模块是值一致性检查，不认证 Registry/遥测来源、实测性能或飞行许可；`inputValid=true` 不代表无业务风险，仍须查看阻断项及真实原生审阅链。只完成源码和限定静态读回，未编译、测试、运行 helper、应用或设备。

`FleetMissionRiskEvaluator` 是地面站侧的只读任务风险汇总器，对已经编译的 mission plan、逐机健康快照、能量计划和额外风险信号做保守计算。

它输出：

- `worstCaseOutcome` / `worstCaseSeverity`：最坏结果及严重级别；
- `riskItems`、`blockingReasons`：可解释风险来源；
- `completionProbabilityEstimate`：确定性启发式估算，且明确带 `completionProbabilityIsEstimate=true`，不是统计保证；
- `coverageRateEstimatePercent`：只有调用方提供覆盖证据时才计算，否则为 `-1` 且 `coverageEvidenceAvailable=false`；
- `alternatives`：缩小任务、补充备用机、重新健康检查等人工替代方案。
- `inputValid`：输入结构、类型、ID、布尔值、覆盖率和风险权重是否通过严格校验；为 `false` 时结果不可作为任务放行依据。

安全边界：

- 固定输出 `manualConfirmationRequired=true`、`requiresOperatorReview=true`、`flightCommandReleased=false`；
- 不连接 Vehicle、MAVLink 或 MissionManager，不修改任务、不重排成员、不发送命令；
- 缺失覆盖证据、健康证据或能量证据不会被推断成“已完成”或“安全”；
- 该模块不能证明真实飞行完成率、避障能力或硬件安全性，必须由工程师通过编译、Mock/SITL、故障注入和硬件流程验证。

## 输入约定

`evaluate(missionPlan, healthReports, energyPlan, riskSignals)`：

- `missionPlan` 通常来自 `FleetMissionCompiler::compile()`；支持 `planId`、`feasible`、`members/assignments`、`waypoints/areaSummary`、`riskSignals` 和覆盖证据字段；
- `healthReports` 是逐机 map 列表，至少包含 `vehicleId` 与 `healthState`/`online`；未知状态按风险处理；
- `energyPlan` 通常来自 `FleetEnergyScheduler::plan()`，其 `feasible`、`reserveVehicleIds` 和能量证据会影响估算；
- `riskSignals` 为额外 `{code, severity|weight, detail}` 列表，按保守方式叠加。

输入校验规则：车辆 ID 限定为 1–255 的整数（数值或数字字符串），数值字段必须是有限数值；任务中的航点/覆盖计数和 `reserveCount` 还必须是非负整数。`online`、`feasible`、`parametersValid` 等布尔字段不接受字符串替代。覆盖率必须在 0–100%，越界或非法覆盖率会产生阻断风险，不会被静默截断；缺少备用 ID 也会如实产生 `reserve_shortfall`，不会因为空列表而跳过检查。风险信号的显式严重级别必须是已知级别，权重必须是有限的 0–100 数值；重复成员/健康报告和其他非法字段同样会阻断审阅。

该接口用于生成审阅结果，不是自动放行器。

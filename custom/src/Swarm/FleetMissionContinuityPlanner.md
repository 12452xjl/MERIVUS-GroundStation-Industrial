# FleetMissionContinuityPlanner 契约

## 420：连续性建议绑定完整基线观察

每个 VehicleSnapshot 从传入原 map 缓存 [currentConfigurationBaseline](FleetConfigurationBaselineEvidence.md)；原成员硬门槛、替补门槛和替补排序共用该结果，不再给 baseline/unverified 标签提供信任等级。稳定 fleet 指纹另加入固定2键基线身份，包括合法负面观察的 epoch/revision/hash。

A→未知→A 的新提案可再次满足参考基线，但旧建议不可复用。原占用、任务版本、实际来源与最终本地角色 CAS 保持，不能据此称机载自动改派完成；任务 schema/metadata 与命令边界不扩展。未编译、测试或运行。

## 411–415：专用恢复入口已接，不等于机载自愈

[恢复面板合同](FleetOccupancyRecoveryPanel.md)补上原生在线restart/history再审的源码操作入口，指挥中心及FlyView地图区域规划复用原Root。仅双账号原审阅能局部解除这些holds，日志接纳不代替原worker实际提交。

规划器值schema、候选排除及最终old∪new占用门禁未放宽；当前真实unknown、owner或busy任务仍拒，不能用UI解除事实作远端free或自动改派许可。离线/报废现实处置、真实unknown解决、正式机载版本/租约/自愈及原25目标/六类继续。未编译、测试、运行；旧无UI段为历史。

## 406–410 原生恢复当前边界

[恢复复核合同](FleetOccupancyRecovery.md)：符合条件的在线机可解除独立restart/history holds，但不改Planner建议/old∪new门禁，不清真实unknown或owner，不发机载改派。新actual身份必须真实commit；离线/报废旧机、本轮真实unknown及完整自愈继续待开发。本批无新操作UI、远端空闲证明或完整自愈交付；仅源码和限定静态读回，未编译、测试或运行。

## 401–405 当前：消费重启待审的实际源

规划器值schema和明确required语义未变。[实际Swarm留存](FleetOccupancyJournal.md)在bootstrap/当前identity尚未提交、坏文件/失源/存储错误时使原生快照无效；产品仍只hold，不能降级采用旧空来源。有效新实例快照仍含1–255的独立restart-review unresolved行，恢复历史不解除它们，故当前产品连续性保持待专用再审，不生成可应用替补。该集合不是实际255架飞机记录，也不是健康/任务标签。

本轮未增加规划器clear、持久化导入授权或远端接管；最终仍由原实际源与六参CAS守门。专用恢复审阅purpose/现实处置依据尚未开发，不能把暂时hold当完整产品终态或自愈成功；原25目标/六类继续，未编译或运行。

## 396–400：实际本地占用证据（当前）

任务摘要的 `taskReservations` 和391批的 `proposalClaimedVehicleIds` 不是 Swarm 原生占用。当前纯规划器通过 [FleetNativeOccupancyEvidence](FleetNativeOccupancyEvidence.md) 消费可选 `context.nativeOccupancySnapshot`，实际产品刷新/人工应用路径必须同时设置真实非 null bool `requireNativeOccupancyEvidence=true` 并由原实际 Swarm 取快照。缺省 false 只保留脱离产品来源的兼容规划诊断；此时缺证据的建议明确 `requiresNativeOccupancyReview=true`，不接受到产品应用入口。显式提供的坏类型/null/坏 hash/坏 shape 快照使输入无效，不能静默退化成“没有提供”。要求证据而缺失时，活动普通任务只输出 hold，不生成完整替补、恢复或加入建议。

快照恰9键：`schemaVersion=1`、`swarmEpoch`、规范正整数字符串 `occupancyRevision`、`fingerprint`、`sourceCurrent=true`、`entries`、`scope=local-swarm-occupancy-at-read-not-remote-lease`、`authorityGranted=false`、`flightCommandReleased=false`。`entries` 最多255行且按ID排序，每行恰 `{vehicleId,blockers,owners,authorityRevoked,uncertain}`；blockers/owners 为规范有序不重复字符串列表，不输出 free 行。helper 验证排除 fingerprint 后的其余8键 compact JSON SHA-256，并兼容 QML 数值往返后规范 schema/ID。该一致性检查不认证任意 caller map 的原生来源，也不认证飞行器状态。

每一个占用/不确定 entry 都阻止该机作为候选；即使 authority 已 revoked、observer 已 closed、或 owner 字符串碰巧等于 taskId，也不豁免。原分配中任一成员存在 entry 时，整项普通任务只输出 `native_assigned_vehicle_occupied_or_uncertain` hold，保留相关原条目供解释；不能把占用当成普通退化成员替掉，从而冒充原占用已释放。其余任务仍可为真正没有这些本地阻断行的候选形成完整建议，但这也不等于远端空闲、可起飞或取得控制权。受保护 native execution observation 仍无通用改派建议。

逐任务、集合和每条 suggestion 均显式输出 `sourceNativeOccupancyIdentity`（有效时恰4个字符串：`swarmEpoch/occupancyRevision/fingerprint/scope`，否则空 map）及真实 bool `requiresNativeOccupancyReview`。稳定身份及 require 政策纳入 suggestion ID；完整源修订变化会撤旧建议，普通遥测 age 不进入此身份。`nativeOccupancyAssignedVehicleIds/Entries`、`nativeOccupancyExcludedVehicleIds/Exclusions` 只解释本地阻断，不是新许可、lease、释放操作或既有 owner 证明。

产品最终应用使用六参数 `reassignTaskRolesWithOccupancyAtRevision()`，还需同一实际、粘性绑定的 Swarm/Orchestrator 在原生角色 CAS 写点验证这份完整身份仍当前，并拒绝原成员与拟分配成员中的任何真实阻断；QML 的先前快照、同ID新对象或“Owns=false”不能代替最终检查。这里没有按 owner 标签授予权限，不恢复、转移、直接/强制释放原 Run/Swarm 占用，不发送飞行命令、不修改 PlanStore schema。已有排空/安全逃生流程保持各自原合同。

本批只进行源码与限定文本读回，未编译、测试、解析、运行 helper/业务/认证/应用/设备。原25目标及六类工程落地缺口继续，未证明远端任务完成或获得最终 StartPermit/放飞许可。

## 391–395：按任务角色完整替补（历史，算法继续适用）

连续性替补现在将 Leader 和全部受影响成员放进同一个 [FleetReplacementMatcher](FleetReplacementMatcher.md) 一对一匹配图。每个缺口的边同时满足任务全局能力、该角色能力，以及现有 verified/baseline、健康、链路、电量门槛；通用健康候选不能自动代表 scout、relay 或 responder。匹配是有界、确定性的最大数量匹配，不是逐缺口贪心取第一架，也不会把同一候选用于两个缺口。

角色来源是任务 `metadata.roleAssignments`，可兼容顶层同名字段；两层同时提供时必须在 ID/行序规范化后保持一致。数组每行恰为 `{vehicleId, role}`，覆盖全部目标且 ID 唯一、无额外成员；角色原串必须恰为 canonical `leader/scout/relay/responder/member`（不接受 `SCOUT` 或前后空白），唯一 Leader 必须与摘要已经解析的 Leader 一致。`metadata` 显式存在时必须是真实非 null map，不能用 `toMap()` 将坏类型空化。角色图坏类型、冲突或不闭合使输入无效；有显式 `leaderCapabilities/scoutCapabilities/relayCapabilities/responderCapabilities` 却缺完整角色图时产生 `role_source_required` hold，而非猜测 Registry.role。角色需求与全局需求都经共享严格 canonical 校验，合法空列表不补默认，typed-null、数字或坏项不变成“无要求”。全局 `requiredCapabilities` 的原 precedence 不变；每字段 metadata 优先顶层，显式 required（即使为空）优先兼容 capabilities。

旧任务无显式角色政策时，保留摘要已有的 Leader/成员首项兼容规则和其余 `member`，标记 `legacy-global-member-policy`；这仅检查全局规划约束，不证明专业角色等价。显式 unknown/null Leader 不补造，保持 `leader_source_required` hold。原成员和候选使用同一角色要求及已知链路质量门槛；正常能力观察与本地工程声明依然不是硬件、性能或账号认证。

仅所有缺口完整覆盖时，生成一条不可拆分的完整建议：只有单个 Leader 缺口用 `replace_leader`，其它情况（包括 Leader+成员复合退化）用 `reallocate_task`。两者 `details` 都包含：

- `replacementCoverageComplete=true`；
- `proposedAssignments`：完整目标集合，保留全部健康原成员并替换每个受影响成员；
- `proposedLeaderVehicleId`：健康原 Leader 或其实际匹配候选，必须属于该集合；
- `proposedRoleAssignments`：对应完整集合的恰两键 `{vehicleId,role}` 行，角色随原缺口保留；
- `replacementBindings`：受影响缺口的 `{previousVehicleId,replacementVehicleId,role,requiredCapabilities}`；
- `unfilledSlotIds=[]`，以及原因、保留成员和角色来源诊断。

部分匹配仅保留 hold / split 诊断及未填缺口，`replacementCoverageComplete=false`、没有可应用的残缺 `proposedAssignments`。不再将 Leader 与其它故障成员拆成相互覆盖的两份改派，也不因“有一个候选”提交缩小后的任务。中途加入仍只对无缺口且无显式角色能力政策的旧通用任务给出待人工定义角色/航段的候选建议，不是角色替补或自动加入。

`plan()` 按 taskId 确定顺序，为同批完整替补建议维护单独的候选排他账，后续任务不能再次承诺这些候选；输出 `proposalClaimedVehicleIds` / `proposalExcludedVehicleIds`，明确范围 `this-review-batch-not-actual-ownership`。这不是实际 lease、全站空闲证明或占用转移，也不改旧 Run、Swarm map 或未排空占用。孤立 `planMission()` 仍不具备跨任务占用证据。建议 ID 进一步绑定角色政策、完整候选/映射、批次排他内容，以及实际采用的 verified/baseline/battery/join 布尔值和三项电量/链路阈值；不包含时间/连续 age，不能同 ID 换用另一份完整 proposal 或政策。

人工应用接缝同步使用角色感知的 `reassignTaskRolesAtRevision()`：必须重新取当前规范建议、检查整组覆盖、完整角色闭合及唯一 Leader，并在单次任务版本 CAS 中同时更新 IDs/Leader/角色 metadata。只更新普通本地任务模型，不提交 PlanStore 新 schema、不修改已审阅的计划内容、不发送命令；真实任务的新机载部署仍需重新编译计划和审批。native execution observation 仍完全不走此通用建议/改派，观察成功不等于任务完成，不自动释放任何实际占用。

本批仅源码与限定文本读回，未编译、测试、解析或执行 helper/业务/应用/设备。原25目标及六类工程落地缺口继续，未获得最终 StartPermit 或放飞许可。

## 386–390：构型绑定的工程声明与原成员复核（历史，角色替补见上）

能力消费统一使用 [FleetCapabilityEvidence](FleetCapabilityEvidence.md)。`requireVerifiedCapabilities=true` 要求当前一致的本地工程审阅记录，即使需求列表为空也不能由裸顶层/metadata verified 标签、协议/组件声明或运行时布尔值代替。普通规划保留原 observed/runtime 规则，并且只合入 helper 当前有效的工程声明；没有声明的观察缺口不自动推断为硬件不存在。

全局要求先取 metadata，缺少相应字段时再取任务顶层；`requiredCapabilities` 存在时优先，否则使用兼容 `capabilities`，不将两字段静默并集。两字段都必须通过类型/null 校验，显式策略布尔值必须是真实非 null bool。原默认 verified/baseline=false 不变。

现在已有成员也执行任务明确的 required/verified/baseline 策略，而非只检查替补候选。失配成员进入 `affectedVehicleIds`，原因包括 `configuration_review_required`、`capability_review_not_current`、`required_capability_missing`；逐任务 `continuityIssueCodes` 和替补建议的 `details.reviewReasonCodes` 保留解释。声明失效不是电机故障，也不触发自动返航、改派或控制发送。

候选及“当前审阅优先”排序使用同一 helper。`sourceFleetFingerprint` 额外纳入 `stableReviewIdentity` 的上下文/记录 epoch、revision、状态和能力身份，保留 superseded/revoked 身份，排除理由/审批日期；因此已观察到的构型 A→B→A 或撤销→重批不能只凭同标签恢复旧建议。控制器仍须取得实际 Registry 当前快照；纯规划器无法认证任意输入 map。

该历史增量不改变受保护 native Run 的占用/owner 边界，旧记录不恢复实际 observer 或飞行权限。该批当时仅核全局能力政策；391–395 已加入上述显式角色替补。真实能量/动力学模型、硬件/性能/账号认证及原25目标/六类仍继续。没有新增命令；仅源码读回和限定文本核对，未编译、测试、运行 helper/认证/应用或设备。

## 原生执行观察任务隔离（2026-10-15）

顶层 `nativeExecutionObservation` 若存在，必须是非空的真实 QVariant `bool`；字符串、数字、null 或无效值使输入无效，原因含 `invalid_native_execution_observation_marker`。只有值为 true 的任务可以使用 `observing` / `observation-closed`；缺少该标记或值为 false 的特殊 phase 会被拒绝。`metadata.nativeExecutionObservation` 不代替顶层字段。

原生观察任务在实际 MODE 写入提交前保持 `draft`，提交后可为 `observing`，观察源退休/owner 销毁后为 `observation-closed`；关闭观察不等于任务完成。对顶层真实标记为 true 的输入，只要 phase 不是 `observation-closed`，规划器就保守保留全部成员占用，包括 `draft`，以及导入数据伪称的 `completed/failed/cancelled`。`activeMissionCount` 对这些未关闭观察任务同样计数，`taskReservations` 的相应条目回显 `nativeExecutionObservation=true` 并纳入占用指纹。其它任务仍会排除这些成员。

此类任务自身固定没有通用改派建议：`suggestions=[]`、`candidateVehicleIds=[]`，逐任务结果仅增加 `nativeOwnerWorkflowRequired=true` 和 `continuityDiagnostic=needs-native-owner-workflow`。集合 `plan()` 将这些诊断放在 `nativeOwnerWorkflowDiagnostics` 数组，每项为 `taskId`、`phase` 与上述两个字段；它们不是 suggestion，不能提交到通用连续性应用入口。已有输入错误仍会使集合 fail closed，只产生集合级阻断 hold，不会产生原生任务成员变更。

原生观察摘要缺少明确 Leader 时，诊断结果保持 unknown/null，不从成员首项推定 Leader；显式传入的 Leader 标签仍只是待审阅的数据，不能认证实际角色或赋予控制权。普通任务原有的成员首项 fallback 不变。

规划器只审阅数据，不能认证标记来源，也不验证或恢复 `executionObservationIdentity` 对应的实际 owner。导入的 true 标记最多导致保守多占用和禁止通用建议，不能授予原生修改权限、恢复观察槽、转移控制权或证明实际飞行结果。原生 owner 注册/身份绑定及通用变更拒绝必须由编排器和实际 Run 的独立原生路径维护，不能依赖此标记完成认证。

本批仅源码与限定文本核对；没有编译、测试或执行规划器/应用。实际生命周期、占用互斥及工程环境行为由工程师后续验收。

## 本地任务成员占用（2026-10-13）

`plan()` 应接收完整任务集合。普通任务的 running/paused/pending 保留成员，Draft/终态不占用（原生观察任务例外见上）；替补候选排除其它活动任务成员并输出 `ownershipExclusions`。任务 ID、phase、成员别名冲突、非法活动成员或重复占用使全集合 `inputValid=false`，仅输出 `continuity_collection_invalid` 的 hold，不遗留可应用替补。

建议 ID 绑定规范排序后的 `sourceTaskOwnershipFingerprint`：其它任务启停/重分配改变占用会使旧建议失效，枚举顺序和无关进度不会引起抖动。单任务 `planMission()` 明示 `ownershipEvidenceAvailable=false`、`requiresTaskOwnershipReview=true`，不证明全队空闲。控制器应用前重审全部本地 taskSummaries 并要求占用证据，编排器在最终写入前再次拒绝冲突；它不是跨站租约或机载授权。

`FleetMissionContinuityPlanner` 是地面站侧的纯数据连续性建议器。它读取
`SwarmMissionOrchestrator::taskSummaries()` 与 `FleetRegistry::vehicles()` 的
快照，识别 Leader 故障、失联、低电量、成员退化和可恢复任务，再输出可审阅的
替补、重分配、拆分、恢复与中途加入建议。

## 输入

`plan(missions, fleet, context)` 的 `missions` 使用任务摘要字段：

- `taskId`/`id`、`taskType`/`type`、`phase`、`targetVehicleIds`/`vehicleIds`、`progress`；
- 可选 `taskRevision`/`revision`，必须为非负整数；编排器每次有效状态、进度或成员变更都会递增；
- 可选 `leaderVehicleId`/`leaderId` 与 `metadata`。普通任务 Leader 未显式声明时使用成员首项；原生观察任务保持 unknown，见上方隔离规则；
- `metadata.requiredCapabilities` 同时用于复核原成员和筛选替补机；`metadata.continuityState = recoverable` 或
  `metadata.continuityRecoveryAvailable = true` 可标记暂停任务具备恢复条件。
- 普通任务的 `phase=draft` 是合法的编排器快照，但表示任务尚未开始；规划器不会为草稿生成替补、拆分、恢复或中途加入建议。
- 普通任务的 `phase=completed/failed/cancelled` 等终态同样只保留审计摘要，不生成替补、重分配、拆分或中途加入建议；标记为原生观察的任务不据此释放成员。
- 显式 `leaderVehicleId/leaderId` 必须属于任务成员；否则结果 `inputValid=false` 并生成阻断性 hold 建议。
- 任务 `metadata` 或顶层可以覆盖连续性策略：`allowMidMissionJoin`、`requireVerifiedCapabilities`、`requireConfigurationBaseline`、`requireBatteryEvidence`；类型/null 错误会阻断规划。明确要求当前工程审阅或 baseline 时，原成员失配进入复核，相同门槛的替补候选也会被排除。
- Fleet fingerprint 排除连续增长的 `faultEvidenceAgeMs`，但加入由配置超时决定的 `fresh/stale/unknown` 离散桶；故障证据跨越新鲜度边界时旧建议 ID 会刷新。
- 替补候选按构型 baseline、当前本地工程能力审阅、已知电量/较高余量、已知链路/较高质量和稳定车辆 ID 做确定性排序；排序只影响建议优先级，不会自动下发命令。

`fleet` 使用 `FleetRegistry` 的归一化车辆快照。车辆 ID 接受 `systemId`、`vehicleId` 或
`id`，在线/健康/链路/电量字段接受现有 Registry 别名。未知电量默认不能作为替补机（可在
`context.requireBatteryEvidence = false` 时放宽为待人工复核候选）。

`context` 可覆盖 `lowBatteryPercent`（默认 30）、`candidateBatteryPercent`（默认 45）、
`linkLossQualityPercent`（默认 35）、`allowMidMissionJoin`（默认 true）、
`requireVerifiedCapabilities`（默认 false）、`requireConfigurationBaseline`（默认 false），并可提供
`recoveryReadyTaskIds` 与固定的 `evaluatedAt`。

Estimator、position 和 fault evidence 必须显式 known；`faultEvidenceAgeMs` 还必须是非负且在
默认3秒、或快照声明的有效 `offlineTimeoutMs` 新鲜度窗口内。未知或陈旧证据会产生 `health_unknown` 人工复核原因，并且不会被提升为
Leader、替补或中途加入候选。

车辆快照中的 `online`、链路丢失、估计器/位置/fault known、能力标记和 `hasFault` 只接受
真实 QVariant `bool`；字符串 `"false"`/`"true"` 不会通过 Qt 的隐式 `toBool()` 变成健康或故障
证据。显式布尔类型错误会使本次输入无效，并保持保守的人工复核路径。

车辆 ID 必须是 1–255 的有限整数，任务成员和机队快照中的重复/非法 ID、非法布尔值、
越界阈值、非有限指标或无法解析的 `evaluatedAt` 会使本次规划 `inputValid=false`，只输出
`hold_for_operator/continuity_input_invalid`，不会基于不一致快照生成替补、重分配或中途加入建议。

## 输出

结果含 `suggestions`、逐任务统计和输入摘要。每条建议至少包含：

- `action`：`replace_leader`、`reallocate_task`、`split_task`、`recover_task`、
  `mid_mission_join` 或无替补时的 `hold_for_operator`；
- `trigger`、`severity`、`reasonCode`、`affectedVehicleIds`、`candidateVehicleIds`；
- `sourceTaskRevision` 回显来源任务版本；缺失或非法版本为 null，不能据此提交重规划；
- `sourceFleetFingerprint` 与 `suggestionId` 绑定稳定的机队风险快照 SHA-256（排除心跳时间戳和连续年龄，纳入健康/链路/电量/能力/构型及本地审阅身份）；机队证据变化后旧建议 ID 不会被刷新规划重新解释为新建议，hash 不认证硬件或账号；
- `inputValid`/`inputIssues` 说明快照是否通过一致性门禁；`inputValid=false` 时不得提交任何
  成员变更；
- `details.proposedAssignments` 或 `details.proposedGroups`，以及人工复核所需的任务/航段信息；对
  `replace_leader`/`reallocate_task`，`proposedAssignments` 是保留健康成员并加入替补后的完整目标集合，
  不是只包含新增候选的差量列表；两种完整替补动作都带有必须属于该集合的
  `details.proposedLeaderVehicleId`、`replacementCoverageComplete=true` 与完整 `proposedRoleAssignments`，见当前契约。

所有结果固定带有 `recommendationOnly = true`、`manualConfirmationRequired = true`、
`flightCommandReleased = false`、`tasksMutated = false` 和 `vehiclesCommanded = false`。
规划器没有 `Vehicle`、MAVLink、Mission 上传或任务写入依赖；QML 只显示建议队列，操作者必须
自行核对任务版本、空域/间隔、电量、链路和机载故障保护后再通过既有人工流程处理。
如上层选择应用已审阅的成员变更，应通过 `GuidedActionsController.applyContinuitySuggestion(suggestionId, true)`
按当前建议 ID 重新取回规范建议；控制器会检查人工确认标志、动作类型、来源版本和当前任务快照，随后才使用
编排器的 `reassignTaskRolesAtRevision()`。任务已变化、建议已刷新或载荷不再属于当前建议队列时返回 false，不修改现有任务。
旧 `reassignTaskAtRevision()` 不作为上述角色替补的降级路径；不应由普通 QML 直接取得可变编排器对象后绕过包装。

## 接入边界

`GuidedActionsController.qml` 周期性用共享 `SwarmController.fleetRegistry` 与
`SwarmMissionOrchestrator` 快照刷新规划器。`CommandCenterOverlay.qml` 的面板只读展示按严重度排序的前六条建议，并显示任务 revision、Fleet 指纹前缀、原因、受影响机和候选机；更多建议会明确提示仍有未展开项。面板没有执行按钮，也不会调用 `SwarmController`、`MissionHandoffManager`
或任何飞行命令入口。源码提供的人工应用接缝不在面板自动流程中，仍需工程师/上层人工确认后显式调用。

工程师需要在 Qt 5.15/qmake 环境确认 moc、QML 资源和真实快照字段，再用 Mock/SITL 注入
Leader 故障、链路断开、低电量、重连恢复、拆分和中途加入案例。源码 groundwork 不构成任务
连续性保证、PX4 故障保护证明或放飞许可。

# FleetRoleAllocator 契约

## 521：角色排序前执行普通任务的速度与续航下限

实际任务的`minimumSpeedMps`／`minimumEnduranceSeconds`不再只在严格性能模式下读取。完整真实数值须在0..9007199254740991，错误类型、null、负数、非有限或越界整体拒绝；字段缺省与显式0不新增正下限，但保留原任务身份差异。

存在正下限时，在角色评分及主／备用选择之前核候选自己的对应值。严格模式仍只用当前工程声明；非严格使用原Registry的`maxSpeedMps`及`enduranceSeconds`／既有`enduranceSec`别名规则。未知或实际−1按缺失拒绝，坏类型及其它负值拒绝；低于要求不进入候选。没有要求的另一项不因本次增加新读取依赖。电量／链路得分不能抵消性能不达标，既有一对一角色分配及Compiler最后逐机门禁不变。

这不是性能估算、实测认证或调速动作；最低续航能力也不是当前剩余飞行时间。缺／零下限的旧角色规则、严格来源、能量预算及其它安全门保持。全部仅源码与直接静读，工程场景未执行。下方483／430为历史。

## 483：自动角色排序前过滤严格性能声明

既有实际Bool `requirePerformanceEvidence=true` 时，在角色打分／分配之前调用[共享当前工程声明](FleetCapabilityEvidence.md)，要求当前批准与完整匹配构型基线，并逐机比较全任务最低速度／续航。未知所需值或不达阈值拒绝，避免高电量／链路分数使自动分配先选无声明飞机，再仅由Compiler否决；主机和备用候选都走同一过滤，不改变一对一角色覆盖或原assignment行shape。

严格根结果增加true策略及有序 `declaredPerformanceSources`；false／absent不新增这组字段或自动升级旧角色规划。成员自己的速度floor、实际profile及coverage速度仍由Compiler继续核，能量仍需逐机声明续航和原电量，角色选择不是后续准入替代品。

这是显式task／Intent.parameters入口，没有新区域草稿设置／UI或性能录入器。原工程能力审批及identity hash不重做，数字仍是工程声明，performanceCertified及flight许可不改变。工程验证全部未执行；未编译、运行或测试。下方430“原本不读取性能”仅为历史。

## 430：保留 Registry 电量/链路未知语义

本模块读取的 Registry 核心 `batteryPercent`、`linkQualityPercent`（含原支持别名）若为真实有限数值且精确等于 `-1`，现在按缺失证据处理。不会再把原生未知哨兵误报为 `invalid_vehicle_evidence`；字符串 `"-1"`、bool、null、其它负数或非有限值仍不接受，距离及任务阈值不放宽。

未知继续进入原 `requireBatteryEvidence` / `requireLinkEvidence` 分支：要求证据时拒绝，否则只保留待审建议并给出原 missing 风险、不加对应分数。本模块原本不读取速度/续航，因此未新增性能推断或角色性能政策；`-1` 兼容不授予能力、实测资格或飞行许可。只做源码及限定静态检查，未编译、测试或运行。

## 420：基线门槛、加分和提示统一

候选的必需基线、基线15分和可选基线风险均使用 [currentConfigurationBaseline](FleetConfigurationBaselineEvidence.md)，不以旧 configurationState/configState 标签获得资格或优待。输出中的旧 configurationState 仍只作原始报告，不是新批准布尔；角色资格/一对一备用覆盖不改变。

未知与不匹配在显式要求时拒绝；未要求时可继续产生待审方案，但不能加基线分。该逻辑不认证实测性能或发布飞行命令。未编译、测试或运行。

`FleetRoleAllocator` 是地面站侧的动态角色规划入口。它把已经归一化的任务约束和机队快照转换成可审阅的 `leader`、`scout`、`relay`、`responder`、`reserve` 建议。

## 391–395：按实际主角色筛选备用并区分同时替补缺口

备用不再只是按原通用分数填满 `reserveCount`。在原在线、健康、电量/链路、距离、构型基线与当前能力审阅等候选门禁之后，先完成原主角色分配，再对尚未分配的候选逐一检查**本次实际已分配主角色**的 `leaderCapabilities`、`scoutCapabilities`、`relayCapabilities`、`responderCapabilities`。一个角色都不满足的候选不会进入备用；要求为空只表示没有额外声明标签限制，不代表证实了同等飞行性能。

能力证据仍走 [FleetCapabilityEvidence](FleetCapabilityEvidence.md)：verified-only 只用当前本地工程声明，普通模式合并原 observed/runtime 能力与当前声明。本阶段还把任务全局能力政策前移到同一候选池：`requiredCapabilities` 若存在则权威（包括显式空列表），否则使用兼容 `capabilities`；两个字段只要提供都必须通过共享 canonical 校验，坏类型/null/越界 token 不因另一个字段优先而被忽略。候选缺少任一全局能力即返回 `global_capability_missing:<capability>`，主角色与备用都不能先推荐后仅依靠编译器拒绝。

四个角色 `*Capabilities` 也使用同一个有界 canonical 入口：保留大小写/首尾空白归一化、去重与规范空要求，拒绝 typed-null、非字符串元素、空 token、超128项或单 token 超128字符等输入；失败时整个任务 `parametersValid=false`，不产生主/备用分配。角色资格图不再靠宽松列表转换吞掉坏要求。

备用选择优先增加尚未覆盖的不同角色，再比较原候选分数、车辆 ID；不会在备用选择中更换主成员或把“覆盖角色较多”变成实测性能评分。主角色原计数、角色评分、排序算法及未声明全局政策的默认行为保留；显式全局政策现在也会过滤主候选，故不声称所有主规划行为完全未变。`feasible` 原义仍只表示主角色数量分配可行，不表示备用数量、每个角色或全部主成员同时缺失都已得到保证。主 assignment 的 `requiredCapabilities` 展示全局与该角色要求去重后的集合。

新增结果：

- 每条 `role=reserve` 的 assignment 增加 `eligibleReplacementRoles`，仅列该候选满足显式能力要求的实际主角色。
- `reserveRoleCoverage` 按实际角色给出 `role`、`requiredCapabilities`（全局与该角色要求去重）、`candidateVehicleIds`、`selectedReserveVehicleIds`、`covered`。候选列表不包含已占主角色的成员；候选与最终选中备用分开，`covered` 只表示至少一个选中备用满足该角色的声明要求。
- `simultaneousReserveCoverage` 使用 [FleetReplacementMatcher](FleetReplacementMatcher.md) 对每个实际主 assignment 创建一个独立 slot，只以本次选中备用作为候选，求最大数量的一对一匹配。保留 helper 的 `inputValid`、`complete`、`assignments`、`unfilledSlotIds`、`inputIssues`、`recommendationOnly=true`、`flightCommandReleased=false`，增加 `activeAssignmentCount`、`matchedAssignmentCount`、`unfilledAssignmentCount` 和 `slots` 明细。明细包含 `slotId`、`primaryVehicleId`、`role`、`eligibleVehicleIds`；传入 helper 的 slot 则严格只有 `slotId` 和 `eligibleVehicleIds`。

这里的 `complete` 是“假设这些已分配主成员同时缺失，选中备用能否在声明约束下逐一对应”的静态结果；一架通用备用最多匹配一个 slot，不会因支持多个角色就被计算为多机同时替补。最大匹配只针对已经选定的备用集合，不承诺该集合是所有候选中的全局最优性能/韧性组合。无效任务的该结果为 `inputValid=false/complete=false`，不会因为主分配为空而误报完整。

请求备用时，数量不足保留 `insufficient_reserve_vehicles`，实际角色没有选中备用覆盖增加 `reserve_role_uncovered:<role>`，一对一同时覆盖不完整增加 `simultaneous_reserve_coverage_incomplete`。`reserveCount=0` 仍如实提供覆盖/缺口，但不会静默新增强制备用要求或改变主可行性。没有实际主角色时不捏造可接替角色，不为凑数量选入无用途的备用。

本批不预留真实飞机、不执行替换、不修改任务或发命令；本地工程声明不是硬件、实测性能、当前账号或飞行安全认证。全局显式能力政策在本模块和最终[任务编译器](FleetMissionCompiler.md)中分别核对，角色覆盖不是绕过全局政策的许可。真实角色等价性能/任务协议、能量模型、运行闭环以及原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、运行 helper/应用或设备。

## 386–390：角色建议消费当前本地工程声明

本模块通过 [FleetCapabilityEvidence](FleetCapabilityEvidence.md) 的 `currentCapabilities` 消费与当前构型/静态身份/revision 一致的本地工程能力审阅。verified-only 候选即使没有角色能力要求也必须有当前记录，缺失、撤销、superseded 或不一致记录返回 `capability_review_not_current`；不再用顶层或 metadata 中的裸 verified 列表授予能力。AUTOPILOT_VERSION、组件元数据、护照或 trustedManifest 声明不会自动变成工程批准。

普通角色规划保留原 observed/runtime 能力，同时只并入当前有效的工程声明，允许显式声明观察列表没有的能力。`leaderCapabilities`、`scoutCapabilities`、`relayCapabilities`、`responderCapabilities` 继续约束对应主角色。本阶段当时备用机仍是通用候选，后续391–395已补上前述角色声明筛选和一对一缺口；两者均不宣称真实性能等价。[任务编译器](FleetMissionCompiler.md)还会在最终成员确定后独立检查全局和对应角色的显式政策，手工分配不跳过该检查。

任务 bool、`requiredRoles` 或角色 `*Capabilities` 若显式提供 null/坏类型，不能退回缺失字段的默认值来形成可行分配；未提供的原默认行为保留。

这里的当前声明一致性不认证真实硬件、实测性能或账号，也不向任务/飞控授予权限。本阶段没有新增命令；391–395补的是静态声明约束下的备用建议，不等于完成角色等价性能、性能/能量实测模型、授权产品接线与原25目标/六类。只做源码读回和限定文本核对，未编译、测试、运行 helper/认证/应用或设备。

## 输入

- `task`：`leaderRequired`、`scoutCount`、`relayCount`、`responderCount`、`reserveCount`；也可用 `requiredRoles` 显式列出角色（未提供对应 `*Count` 时默认每个角色 1 架）。可选最低电量、最低链路、最大距离、配置基线、全局 `requiredCapabilities`/兼容 `capabilities` 和角色能力标签。
- `fleet`：`systemId/vehicleId`、`online`、`healthState`、`configurationState`、`capabilities`、`batteryPercent`、`linkQualityPercent`、`distanceMeters` 等归一化字段。
- 任务可选 `requireVerifiedCapabilities=true`；此时角色所需能力只从当前本地工程审阅记录读取，`canUse*` 运行时观测和旧 verified 字符串不会替代该记录。

未知健康默认拒绝；未知电量/链路可作为风险保留，任务设置 `requireBatteryEvidence` 或 `requireLinkEvidence` 时会拒绝。设置 `requireConfigurationBaseline=true` 时，只接受 `configurationState=baseline`。

输入门禁：车辆 ID 必须是 1–255 的有限整数；只要同时提供
`systemId`、`vehicleId`、`id`，每个别名都必须是数值型整数且彼此一致。数字字符串、空值、分数、非有限值或别名冲突会逐机拒绝，不能静默取第一个别名。任务计数必须是有限非负整数（最大 255），`leaderRequired` 及其余布尔字段必须使用真实布尔类型；阈值、距离必须是有限且在声明范围内的数值。车辆电量/链路越界、负距离、重复或非法车辆条目会被拒绝。参数不合法时返回 `parametersValid=false` 与 `invalid_task_parameters`，并保持 `assignments`/`reserveVehicleIds` 为空，不会通过默认值、隐式转换或截断把坏输入变成看似可执行计划。

## 输出与边界

结果包含 `assignments`、`rejected`、`risk`、`feasible`、`selectedVehicleIds`、`reserveVehicleIds`、`reserveRoleCoverage` 和 `simultaneousReserveCoverage`。同时固定返回 `manualConfirmationRequired=true`、`flightCommandReleased=false`。后两项是静态建议与缺口，不能当作已替换、已预留或飞行完成证据。

该类不连接 `Vehicle`、MAVLink 或任务执行器，不修改输入任务，也不自动执行替补、起飞、编队或航线。当前本地能力审阅后端及消费者已写入源码；真实证据/账号授权接入、角色协议、运行行为与安全性仍需后续实现和工程师验证，不能称只等待开启按钮。

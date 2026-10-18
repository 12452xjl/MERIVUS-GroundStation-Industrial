# 构型绑定的本地工程能力审阅

## 483：当前工程声明中的规范性能数字

新增纯值 `currentDeclaredPerformance(snapshot,out)`。原 `identityFingerprint` 本来已绑定 `maxSpeedMps/enduranceSeconds`；本轮不改原context6／approval14／hash schema、不新造批准。函数同时要求原 `currentCapabilities` 和 `currentConfigurationBaseline` 成功且同快照context匹配，才提取当前工程能力声明已经绑定的数值。完整基线仍为原exact14，不以hash-only、配置标签或metadata同名字段替代。

输出exact11：`schemaVersion=1,source="current-local-engineering-declared-performance",vehicleId,maxSpeedMps,enduranceSeconds,capabilityReviewIdentity,configurationBaselineIdentity,performanceCertified=false,hardwareIdentityAuthenticated=false,executionPermitted=false,flightCommandReleased=false`。前一nested身份沿原stableReviewIdentity，后一为原完整基线身份；它们独立复制容器、keys及QString存储，保QVariant类型而不靠JSON重编码。全部读取／复制完成后才清写out，支持whole／预持nested输出别名；失败清out，无成功前缀。

只读canonical两字段，top／metadata须按原selected规则同type／同value；任意 `enduranceSec` 出现都拒绝，不作fallback。实际有限数值且abs≤2^53−1；缺失或精确−1保unknown，speed0可表示明确0，endurance必须>0或unknown，至少一项known。bool、字符串、null、非有限、其它负数、endurance0及坏别名整份拒绝；`usable`由消费者表示函数成功，不表示两个数字都known或任务可行。

本函数仅校值和一致性，不认证caller、实物或实测性能。实际产品仍靠Coordinator原native快照和来源连续性；本轮只消费既有批准数字，没有新录入器／实测清单／标定／二进制认证或飞行许可。工程边界及30项全部未执行场景见[交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、运行或测试。下方为历史。

## 421：基线变更不等于能力批准

[基线操作者流程](FleetConfigurationBaselineEvidence.md)已接入原 Review/Session/Root/Panel，使用独立操作和账号用途。能力 approve/revoke 不修改构型基线；基线 approve/revoke 不提交能力清单，并使旧能力批准失效，即使重复批准相同配置也必须重新审阅能力。

currentConfigurationBaseline 的14键投影与 context6/approval14 不变。未知当前构型可在真实原记录与比较仍可用时撤销本机基线，不允许据此批准未知配置或恢复旧能力。下方420“无基线批准界面”仅为历史状态；完整 typed 基线仍不持久化，不认证实物/性能。仅源码及限定静态核对，未编译、测试或运行。

## 420：独立的完整构型基线依据

新增 [configurationBaselineEvidence 固定14键与共享函数](FleetConfigurationBaselineEvidence.md)，原 capability context6/approval14 不变。currentConfigurationBaseline 只接受顶层规范证据及相同实际快照身份，要求两边完整匹配；stableConfigurationBaselineIdentity 固定2键并保留合法负面观察代次。它们是值一致性检查，不认证调用方伪造 map。

Registry 四出口生成投影，入站/护照剥离同名声明；typed 指纹来自原私有快照，不重算经 Qt/QML 往返的配置类型。四类任务消费者和角色排序使用新函数，Guided/Coordinator/Deployment绑定相同观察。下方419“统一完整任务消费者仍待”由此覆盖；不新增实物/性能认证、基线批准界面或历史持久化。未编译、测试或运行。

## 419：有类型变化与完整观察失效

[构型历史与比较](FleetConfigurationDiff.md)进入实际Registry：有类型变化即使旧JSON哈希相同，或currentFullKnown失去/恢复，也使旧能力批准失效；普通健康和重复同配置不推进。审批-only导入触及条目现在也退役旧能力record与观察epoch，未触及增量记录不变。原context6/approval14不扩字段。

能力预览升级schema2/18键并绑定原比较；新基线批准必须仍有当前完整观察。旧configurationState及requireConfigurationBaseline仍是哈希级合同，不因此变成有类型基线或实物认证，完整任务消费者升级仍待开发。以下418及更早为历史，局部新行为以本节为准；未编译、测试或运行。

## 418：真实操作者路径已接，原证据合同不变

[双账号工作流](FleetCapabilityReview.md)和[双入口面板](FleetCapabilityReviewPanel.md)已调用实际Registry的批准/撤销，新native重载增加最终纯提交围栏和14键事实回执。原context6/approval14、静态变更失效、下游有效能力消费不变；操作不改构型基线。

实际维护/管理员申请和不同合格账号审批仅代表本地工程采纳，非硬件或实测性能认证。旧bool仍是受信后端兼容入口，不能把新路径描述成所有native入口的强制双账号。UI明确构型JSON只读显示、无完整基线原配置不能识别具体改装；原配置类型/空值保留在native复核。

下方386–390“没有新增操作者界面”的表述是当时历史，由本节限定路径覆盖；企业权限/历史恢复、真实硬件/性能仍待开发。未编译、测试、运行认证/UI或放行。

## 391–395 后续角色消费

当前记录继续作为显式角色资格依据；[一对一替补](FleetReplacementMatcher.md)及RoleAllocator/Continuity已把静态角色要求、单机不能复用和完整本地角色提案接入。这里的可接替仅满足已声明约束，不完成同等实测性能或真实机载自愈执行；能力审批API和6/14合同不改变，原始观察/导入仍不能恢复当前批准。下方386–390“角色等价替补仍待”仅是当时历史，真实性能、实际占用/授权与受控执行仍待完成。

## 386–390 实现范围

本模块把观察/清单声明、当前本地工程审阅、硬件或实测认证分开。[FleetRegistry](FleetRegistry.h) 私有持有当前审阅记录；[值校验器](FleetCapabilityEvidence.h)核对规划快照的一致性，不能认证调用方提供的 map。普通遥测、AUTOPILOT_VERSION、组件元数据或名为 trustedManifest 的输入，都不能仅凭能力字符串获得当前审阅。

旧 `verifiedCapabilities` 键继续存在，但现在只投影实际 Registry 当前有效的本地工程审阅能力。旧输入保留在 `declaredVerifiedCapabilities`，导入的旧审批另存 `importedCapabilityApproval` 诊断；它们都不激活旧批准。`requireVerifiedCapabilities=true` 沿用原规划开关名称，含义是要求本轮明确的当前工程审阅，并不表示硬件真实性、实测性能或账户身份已经认证。

## 原生正向路径

1. 后端提交规范化的观察/构型证据。地面站只识别已上报证据的变化，不能自动检测未上报的桨、电机、电池、载荷或所有同ID物理替换。
2. 工程后端获取 `capabilityReviewContext(vehicleId)`，连同实际构型、能力适用范围与工程证据交给审阅者。合法 hash-only 构型可以申请，但不能因此宣称已检查完整实物；有配置 map 时必须与当前 hash 一致。
3. 明确调用 `approveCurrentCapabilities(vehicleId, exactContext, capabilities, reason)`。它只在上下文仍相同且理由、非空能力集合有效时提交；可以声明观察列表没有的能力，如 thermal，不把观察缺失误当硬件不存在。
4. 当前记录供选机、角色、模板、最终任务成员和连续性建议消费。普通规划可并入当前工程声明；verified-only 必须有当前记录，即使任务需求列表为空。
5. 构型、静态身份、能力或性能声明、基线批准状态改变后，旧记录成为 superseded；显式 `revokeCapabilityApproval(vehicleId, exactContext, reason)` 产生 revoked。需要新上下文和重新审阅，A→B→A 不恢复旧批准。

三个 API 都是 C++ native 方法，不是 QML 写入/审批接口。本轮没有新增操作者授权界面、专用 Session 审批用途或企业权限服务；工程后端仍必须在实际接入时实施身份与授权。存在原生 API 不等于完整人工授权产品已上线。原 `approveCurrentConfiguration` / `rollbackConfigurationBaseline` 继续只管构型基线，不暗中批准能力。

## 固定值合同

`capabilityReviewContext` 恰有6键：registryEpoch、vehicleRecordEpoch、vehicleId、capabilityRevision、identityFingerprint、configurationFingerprint。两个 epoch 是 UUID；revision 是不经 Double 的规范十进制 uint64 字符串。identity 对规范静态型号/固件、构型与基线、观察/声明能力、速度/续航/canUse 声明及已提供 UID 证据计算稳定 hash，不含健康年龄、收包次数、心跳时间或审批理由/日期。

`capabilityApproval` 是空 map 或固定14键：schemaVersion=1、approvalId、registryEpoch、vehicleRecordEpoch、capabilityRevision、identityFingerprint、configurationFingerprint、capabilities、source、reason、approvedAtUtc、state、hardwareIdentityAuthenticated=false、flightCommandReleased=false。source 固定 local-engineering-attestation；state 为 current/superseded/revoked。能力为1–128个规范小写、去重排序标签，每标签最多128字符；理由非空且最多1024字符。

生产者使用原生整数 schema/vehicleId，纯值消费者也接受等值的有限 QML/JSON 数值整数，拒绝 bool、字符串、小数和 null；只有这两项在重复记录比较前规范为 Int。revision 始终要求精确字符串。能力声明支持既有 CSV/列表输入，但批准记录本身必须已规范；上下文、审批及静态字段 top/metadata 重复内容不得矛盾。

Registry 在 VehicleCapability 的字段规范化前，核对型号、飞控类型、固件版本及 type/autopilot/firmware 别名的顶层/metadata原始值为至多512字符且UTF-8可往返的QString；空QString仍是未知，不把bool、数字、容器或JSON null通过toString变成可审阅静态证据。这不是新增硬件真实性来源。

每车辆只保留最新一条原生审批（包括其撤销/替代状态），不是无限历史库。静态变化和批准/撤销均推进单调 revision；耗尽不回绕并关闭正向审批。unregister/重新注册使用新记录 epoch，clear 更换 Registry epoch。护照导入触及的车辆重建记录 epoch、旧审批只作声明/诊断；增量导入未触及的车辆保留原生记录。普通更新仅在三项只读回显与实际私有投影完全一致时忽略自有回显，不能据复制字段恢复权限。

## 实际消费与审计

[Matcher](FleetCapabilityMatcher.md)、[角色分配](FleetRoleAllocator.md)、模板和[连续性规划](FleetMissionContinuityPlanner.md)统一从 currentCapabilities 读取当前能力，不再由裸 verified 字符串赋权。连续性对已有成员也核任务明确要求，并把记录 epoch/revision/状态加入稳定建议身份；声明失效是人工复核原因，不是电机故障或自动返航命令。

[任务编译器](FleetMissionCompiler.md)在覆盖规划已完成、最终成员与备用机已确定后核原 fleet 唯一ID，手工分配不能跳过显式 verified/baseline/全局及对应角色要求。没有 fleet 时仍可普通 planning-only，但不能声称满足明确要求的证据。requiredCapabilities 存在时保持权威优先级，否则用兼容 capabilities；不静默并集、不换成员、不重新选机。

[审阅协调器](FleetMissionReviewCoordinator.md)把固定记录、稳定身份和当前审阅结果加入报告/值快照，Guided 将其纳入实际 PlanStore reviewEvidence hash。Guided 机队指纹同时纳入能力记录、基线状态、速度/续航及 canUse 声明，普通时间增长不改变该身份。新记录/撤销/构型变化会要求新审阅；已 queue 的事实、受保护 Run 与未知占用不因此回滚或自动释放。

本轮不把能力 token 审阅当成数值性能认证；EnergyScheduler 的续航/耗电模型、角色等价替补、真实机载来源/构建身份认证、授权界面和企业协作仍需后续开发，原25目标/六类未完成。未增加飞行命令或放宽最终 StartPermit。

## 工程师后续验证

本轮仅源码、工程登记与限定文本核对，未编译、测试、运行 helper、认证、应用/UI、SQL、仿真或设备。工程师需验证正向新能力声明、过期上下文、重复批准/撤销、静态A→B→A、导入不复活、部分导入保留、只读快照往返、CSV与Qt/QML整数/列表往返、metadata别名/冲突及信号重入。最终成员政策、模板和替补结果也需在目标环境复核；这里列的是待执行要求，不是通过记录。

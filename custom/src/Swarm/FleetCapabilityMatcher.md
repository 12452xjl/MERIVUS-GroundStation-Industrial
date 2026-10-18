# FleetCapabilityMatcher 契约

## 521：显式任务下限独立于严格声明开关

普通区域草稿现在能保留`minimumSpeedMps`／`minimumEnduranceSeconds`任务根要求。两者仅接受实际非null有限数值0..9007199254740991；缺字段是未声明，显式0仍保在来源中但不要求正值证据。任何正下限都要求候选自己的对应已知值；缺失或Registry实际数值−1不能在strict=false／缺省时被放过。低于下限仍使用原speed／endurance拒绝原因，不由高电量、其它机性能或评分补足。

`requirePerformanceEvidence=true`继续仅消费当前构型匹配的canonical工程声明；false／缺省可沿原Registry规划描述符，但不因此认证实测性能、载荷、环境或硬件身份。健康、能力、角色、备用和逐机能量门保持。无下限／显式0的旧缺省不会被升级成严格声明要求。本模块未运行、编译或测试；下方旧‘非严格可保未知’只适用于没有对应正下限的规划。

## 483：严格性能政策消费当前工程声明

既有实际非null Bool `task.requirePerformanceEvidence=true` 不再只检查raw数字存在；候选必须经[共享exact11](FleetCapabilityEvidence.md)取得当前工程批准及完整匹配构型基线的canonical速度／续航。按本机数字比较任务两个全局阈值，所需数字unknown不通过；高电量／链路／能力标签不能补出声明。重复ID在严格候选中不允许第一份记录继续当唯一可信源。

严格assignment增加 `requirePerformanceEvidence=true,declaredPerformanceEvidence,declaredPerformanceEvidenceUsable`，根追加有序 `declaredPerformanceSources`；失败源为空map，usable只表示完整helper声明有效，不等于对应数字known或任务可行。原根requirePerformanceEvidence字段继续保留，false／absent沿旧raw读取／候选行为且不新增严格assignment字段。数字阈值、健康、主备数量和最终人工确认仍另核。

这是现有显式task／Intent.parameters入口的严格消费，不新增性能录入器、UI开关或Draft.settings字段；区域草稿不能擅加该extra键。保原审批／hash schema，不恢复历史权限；全部认证及flight字段仍false。未编译、运行或测试，工程场景全部未执行；下方430及更早是旧政策历史。

## 430：Registry 未知数值与缺失证据一致

Registry 核心 `maxSpeedMps`、`enduranceSeconds`、`batteryPercent`、`linkQualityPercent`（及本模块原已支持的别名）的真实有限数值 `-1` 现在按缺失证据处理，不再逐机误报格式错误。字符串 `"-1"`、bool、null、其它负值或非有限值仍不接受；任务阈值没有引入 `-1` 缺省规则。数字宽度差异不改变这一约定。

缺失继续沿原政策：`requirePerformanceEvidence=true` 且实际声明了对应正阈值时，未知性能仍返回 `performance_evidence_missing`；未要求证据的规划可保留未知候选，但不因此证明达到性能阈值、增加电量/链路评分或获得 verified 能力。原 matcher 没有新增电量/链路强制证据开关；产品任务的能量审阅仍由独立逐机账单处理。这里不创建、估测或认证飞机性能输入，不改变工程能力审阅和构型基线身份。

只完成源码修改与限定静态读回；未编译、测试、运行 helper、应用或设备。

## 420：必需基线改用完整原生依据

requireConfigurationBaseline=true 改为 [currentConfigurationBaseline](FleetConfigurationBaselineEvidence.md)；只有旧 configurationState=baseline、hash-only、导入批准或缺完整原生快照不再通过。拒绝代码仍为 configuration_not_approved，其余健康、角色、能力政策不变；未要求基线时不额外阻断。

传入 map 仍是值快照，helper 不认证来源；产品审批还须实际 Registry 的当前快照与审阅指纹一致。以下旧按标签判断的说明由本节覆盖。未编译、测试或运行。

`FleetCapabilityMatcher` 只根据地面站已经采集的能力、健康、性能、电量和链路快照生成候选编组，不连接飞控或发送任务。

## 386–390：当前构型绑定的本地能力审阅

能力来源见 [FleetCapabilityEvidence](FleetCapabilityEvidence.md)。`requireVerifiedCapabilities=true` 现在只消费 `currentCapabilities(snapshot, ...)` 验证为当前的本地工程审阅记录；即使任务的需求列表为空，候选也必须有当前记录，否则返回 `capability_review_not_current`。裸顶层/metadata `verifiedCapabilities`、AUTOPILOT_VERSION、组件元数据或名为 trustedManifest 的输入不能自动获得该资格，旧列表不再是能力赋权依据。

普通规划保留原观察能力与 `canUse*`/`canRelay` 路径，并且只在 helper 成功时并入当前工程审阅标签。因此工程师可以明确声明观察流尚未列出的 `thermal` 等能力；这不是地面站发现了对应传感器，也不证明实物配置。构型或静态声明改变、撤销及重新批准由 Registry 的原生上下文/revision 管理，旧字符串或恢复同值不能恢复过期记录。`requireConfigurationBaseline` 与健康门槛保持独立，缺失的策略开关仍沿用原默认，不把全部规划自动升级成 verified-only。

helper 只校验输入值的一致性；任意 map、标签、hash 或 `feasible=true` 都不是硬件身份、性能、操作者账号或飞行许可认证。本轮不新增命令、传感器或数值性能模型；角色等价替补、真实模型及原25目标/六类仍继续开发。已做限定源码读回与文本核对，未编译、测试、运行 helper/认证/应用或设备。

## 原有输入与输出边界

资源门禁：任务/车辆能力标签最多 128 项、每项原文最多 128 字符（含空白）；Fleet 最多 2048 条候选。任务参数无效或候选总数超限时不遍历/排序候选，输出空选择组与阻断证据。合法候选输入中的单机坏证据仍以逐机拒绝原因解释，不等于整份 task 参数无效。

任务和车辆输入采用严格门禁：车辆 ID 必须是 1–255 的整数，所有 `systemId`/`vehicleId`/`id` 别名必须一致；数字字符串、别名冲突或小数 ID 都会拒绝。任务数量/阈值必须是有限数值且在声明范围内，布尔策略必须使用非 null 的真实布尔类型，显式 null 的 `requiredCapabilities` 不能降级为空需求；已知电量、链路必须位于 0–100%，已知速度、续航不能为负，核心数值的精确 `-1` 未知约定见430。坏输入会返回 `parametersValid=false`、`feasible=false` 及 `invalid_task_parameters` 或逐机拒绝原因，不会被静默转换或截断。

输出固定包含 `selectedVehicleIds`、`reserveVehicleIds`、`assignments`、`rejectedVehicles`、`riskSignals`，并保持 `manualConfirmationRequired=true`、`flightCommandReleased=false`。任务参数无效时选择/分配列表保持为空，只输出拒绝和风险证据；`allowUnverified` 和 `requireConfigurationBaseline` 只改变审阅门槛，不代表硬件能力已被实测证明。

既有开关名称 `requireVerifiedCapabilities` 只改变候选筛选；其当前含义是本地工程能力审阅，不再采用早期“协议/元数据自动生成可信标签”的解释。实际上传、启动与完成仍由独立原生链核查，本规划器不执行飞行命令。

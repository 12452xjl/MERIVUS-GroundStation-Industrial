# 当前完整构型与本地批准基线

## 421：操作者批准与撤销入口

[实际审阅流程](FleetCapabilityReview.md)和[两处共用面板](FleetCapabilityReviewPanel.md)新增 `approve-baseline` / `revoke-baseline`，与原能力 `approve` / `revoke` 分开。只有能力 approve 接受非空能力清单，其余三种操作必须为空。构型基线仅是本机工程参考，不批准能力、不写参数、不发送飞行命令。

操作使用精确角色动作 `fleet.configuration.review` 和独立账号用途 `fleet.configuration.review.approve`；能力用途保持原值。维护/管理员申请，不同合格 SID 审批；原30秒及身份 TTL 不续期。Root 同时冻结两类 policy 到稳定 sourceToken，再按原 operation 选择精确政策，不能通过切换下拉框复用原批准。

### 允许路径与拒绝边界

- 批准要求当前非空完整有类型配置、有效原能力上下文和实际比较身份。首次无旧基线/前值、比较报告 unknown 时仍可批准当前完整配置；不要求已有差异报告完整。hash-only、clear 或合并残留旧 map 不可建立新基线。
- 撤销必须来自 `configurationApprovalRecord()` 返回的真实私有批准，不能使用旧 `configurationApproval()` 从描述符构造的兼容回退。真实旧记录可为2–5键，允许缺历史理由/时间，不补造。当前构型未知时，仅此操作可接受空能力上下文；仍核原条目独立静态指纹、实际比较16键与观察 epoch/revision。无实际记录或比较不可用时拒绝。
- 原生匹配按 source.trimmed 和 fingerprint.trimmed().toLower 判断，保存和冻结的批准原值不改。界面相应按字符串 trim/lower 比较，不因旧大写哈希或首尾空白误拒撤销；这不是把规范化展示值当 native CAS 依据。
- 所有实际基线批准/撤销，包括重复批准相同哈希，都推进能力依据代次并退役旧能力批准；需另行批准能力。普通健康更新仍不滚动代次。该变更不取消已有 Run 或自动释放占用。

### 原生提交与回执

Registry 两个新四参数重载冻结实际比较、工程理由和最终纯围栏。比较报告用独立8192节点/12深度的严格类型相等检查，不误用原始配置2048节点预算。所有下一状态先在局部构造；最终围栏前后核同一车辆、能力审阅、原批准与私有历史，再在无回调的同一调用栈更新四份记录。新重载本身不认证账号，实际 Review/Session 负责该工作。

返回 schemaVersion=1、恰好14键：`schemaVersion`、`ok`、`reason`、`action`、`vehicleId`、`mutationCommitted`、`previousComparison`、`comparison`、`approval`、`baselineEvidence`、`stillCurrent`、`hardwareIdentityAuthenticated`、`performanceCertified`、`flightCommandReleased`。末三项恒 false；这不是旧能力批准14键或旧能力 mutation 回执，不能仅按键数互换解析。

提交后、公开通知前冻结 comparison/approval/baselineEvidence 和 mutationCommitted。通知后源被删除、改变或已耗尽，仍保留真实提交事实；`ok=false` / `stillCurrent=false` 不证明回滚，不自动重试。撤销后的 approval={}；拒绝回执可保原记录。Review current30/result16、preview schema2/18、approvalEvidence8 保持原形状，操作用途进入原身份关联。

旧 bool 原生入口仍是受信后端兼容调用，保留默认空理由和实际提交 bool 语义；不宣称所有入口都强制双账号。本轮不新增构型历史持久化，重启/护照导入不能恢复完整有类型批准基线。全量25目标/六类开发继续。

本节覆盖下方420“无基线批准界面”的历史状态，不改原14键投影与四类任务消费者合同。仅源码与限定静态审阅，未编译、测试、运行 UI/认证/helper、更新 PR 或打包；工程师待执行项目见[交接说明](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 420：实现与边界

任务明确要求 `requireConfigurationBaseline=true` 时，选机、角色分配、最终成员/备用机和连续性替补统一使用实际 Registry 投影的完整有类型基线依据。旧 `configurationState=baseline` 仍只报告旧 JSON 哈希关系，不能单独满足该要求。可选基线政策不变成所有任务的强制要求；本批不新增飞行命令或放宽最终许可。

“完整观察”仍是419的输入约定：显式上报非空且通过既有有界检查的配置 map；不证明某机型所需字段全部存在，也不证明实物、电机/桨叶、性能或未上报改装。哈希不替代认证。

## 原生生产者

[FleetRegistry](FleetRegistry.h) 的 `vehicles`、`vehicle`、`onlineVehicles`、`vehiclesWithCapability` 四个出口统一添加顶层只读 `configurationBaselineEvidence`。来源是同一实际车辆记录的私有 ConfigurationHistory、当前 CapabilityReviewState 和原构型批准记录，不从导入标签或 metadata 反推权限。

注册、更新及护照导入在描述符预算检查前移除顶层和 metadata 下同名只读字段；导出使用原存储描述符并去除该投影。不会递归删除原始 configuration 内合法的同名工程字段。输入字段不能构造私有完整基线，导入旧批准也不能恢复旧观察 epoch 或旧能力审批。

指纹与匹配缓存随实际历史变更更新，getter 不重新比较完整配置。原始类型快照仍在本机私有内存；本批不新增历史持久化或构型基线批准界面，既有原生批准入口仍是受信后端调用。

## 固定值合同

有效投影为 schemaVersion=1、恰好14键：

| 字段 | 含义 |
| --- | --- |
| schemaVersion / source | 1 / `native-local-configuration-baseline` |
| registryContext | 原6键能力上下文；合法负面观察可为空 |
| observationEpoch / observationRevision | 私有观察 UUID 与不经过浮点的正 uint64 十进制字符串 |
| currentFullKnown / baselineFullKnown | 严格 bool，当前完整观察与本地完整基线是否存在 |
| currentTypedFingerprint / baselineTypedFingerprint | 原有类型值的 SHA-256；对应 known=false 时必须为空 |
| baselineApprovalHash | 原私有构型批准5键的有类型哈希；baselineFullKnown=false 时为空 |
| baselineMatches | 两边完整且有类型哈希相同；原生缓存还核原类型值相等 |
| hardwareIdentityAuthenticated / performanceCertified / flightCommandReleased | 始终 false |

无记录、历史耗尽、原记录/原批准不匹配等情况输出空 map，不伪造一个正向投影。`registryContext={}` 不能获得正向基线资格。

[FleetCapabilityEvidence](FleetCapabilityEvidence.h) 提供两个共享纯值函数：

- `currentConfigurationBaseline(snapshot, identity)`：严格核14键、顶层来源、原车辆 ID/上下文/静态身份一致性，两边完整且匹配才返回 true；成功可返回规范14键，失败清空输出。
- `stableConfigurationBaselineIdentity(snapshot)`：固定2键 `{state, evidence}`。state 为 missing、invalid 或 available；前两者 evidence={}，available 保留规范14键，包括合法的负面/未知观察。available 不等于批准通过。

整数 schema/车辆 ID 可规范化合法 Qt/QML 整数往返；bool、UUID、SHA、revision 仍严格。metadata 中同名基线投影直接无效，不作为回退源。坏字段不进入稳定身份，不将任意 raw 配置复制到归档。Qt/QML 往返后的原配置不再重新求 typed hash，以免类型转换伪造变化；类型判定由实际原生生产者承担。纯值函数自身不能认证调用者伪造的 map。

## 变化、恢复与旧审阅

普通健康更新与重复同完整配置不推进观察 revision。完整配置改变、hash-only/clear 导致完整性丢失、恢复完整观察、批准或撤销基线都会改变观察身份。原配置 A→未知→A 可以在新的审阅中再次匹配仍保存的 A 基线，但不能复活旧任务审阅或旧能力批准。

本机基线是参考快照，不是一次性执行凭证。不能为了阻止旧审批复用而永久禁止重新审阅相同的合法配置；真正隔离旧依据的是记录/观察代次与当前批准关联。

## 消费与归档链

1. Matcher 与 RoleAllocator 的必需基线门槛使用共享正向函数。RoleAllocator 的基线加分与可选政策风险也使用它，不给旧标签加分。
2. Compiler 对最终成员和备用机核同一要求，手工分配或区域覆盖不绕过；不满足保留 `configuration_not_approved`。
3. Continuity 同一快照缓存正向结果，原成员、替补和排序共用；稳定建议指纹加入观察身份，旧提案不能因 A→B→A 复用。
4. Coordinator 新增只读纯值桥 `configurationBaselineIdentity(snapshot)`。Guided 对传入的那份旧/新 snapshot 调它，绝不按 ID 换成当前 Registry 值；缺桥、异常或实际 reviewer 改变返回空指纹，旧审批不能继续。
5. `capabilityReviewEvidence` 升为 schema2，顶层仍8键，member 由7变9：新增 `configurationBaselineIdentity` 和 `currentConfigurationBaseline`。二者纳入原成员整体哈希；captureComplete 增加 available 条件，但不要求每机 baselineMatches=true。归档完整不代表基线通过。
6. Guided 原样保存到 PlanStore 既有 reviewEvidence；不扩计划顶层 schema。Deployment 额外要求归档 capabilityReviewEvidence 与实际审阅完全一致，保留最终实际控制器/Registry 快照核对。历史归档不成为当前来源或飞行许可。

## 工程师待执行矩阵

以下均未执行，不是通过记录：

- 正向：当前显式完整 A→本机批准 A→四类任务消费者与双端旧/新快照一致；普通健康更新/重复 A 不误使审批过期。
- 负向：只有旧 baseline 标签、仅 hash、空/clear 配置、导入批准、metadata 伪造、空上下文、格式/类型错误，均不能满足显式要求。
- 连续性：A→B→A、A→hash-only→A、撤销→重新批准、注销→同 ID 重建、导入触达；旧任务/建议身份必须改变，重新人工审阅仍可使用合法现值。
- 类型：Int/Double、null/Missing、bytes/date、Qt/QML 整数往返；展示截断不应改写原型哈希或被当作无变化。
- 可选政策：没有要求基线时，负面证据仍能被记录且不强迫所有任务满足基线；未知不得获得基线加分。
- 归档：schema2 的9键 member 原样保存，删改任一基线身份导致旧归档与实际审阅不匹配；旧版归档不得默默升级为具有新证据。
- 集成：实际工程包含全部累计新文件；验证原 Registry 自有回显、资源预算、QML 方法绑定、源销毁与审阅失效通知。

本批只修改地面站源码并进行限定静态审阅；未编译、测试、运行 helper/应用/UI/认证/SQL/设备、修改 PX4、刷机、放行、更新 PR 或打包。完整25目标仍有执行闭环、可信性能、自愈、跨站、长期运营及模型/生态等实质开发工作。

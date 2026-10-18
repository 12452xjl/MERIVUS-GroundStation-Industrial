# FleetReserveDispatchPlanner

## 543：备用独立出勤重新派生自己的名义时间

`memberPolicies` 不再携带旧 Compiler 的 `nominalRouteTiming`。建立新出勤路线、返程和全新executionProfile后由本次Compiler重算，不把旧任务主成员或旧备用材料的估算误作不变政策。`memberProjection` 对该Compiler派生诊断作同样窄排除，避免实际新compiled-plan比原请求多一个字段就恒拒；真正原route／profile及所有其它保留政策仍完整匹配，源／provenance与当前审阅门不放宽。PayloadBuilder对本次完整名义诊断另行重算核对，不靠忽略字段获得许可。

只apply_patch及直接静读，未执行Planner、hash、编译、测试、业务或设备。

## 514：后台提案已有真实人工工作流接线，非实际出勤完成

513 的纯值 Planner 与 Compiler／PlanStore／PayloadBuilder 消费者不改。本轮由 [Root工作流](FleetReserveDispatchWorkflow.md) 读取实际当前 task／plan／完整绑定，先冻结再撤旧；[可见面板](FleetReserveDispatchPanel.md) 明确选一架实际备用、填写全新 executionProfile，核对实际新成员完整路线后，重新走正常审阅、归档、明确采用与原本地批准。

唯一共享部署区将其标记 reserve-dispatch，而非旧 remainder／区域草稿。独立编码视图由同一实际新 Store 记录及其 reviewEvidence 经既有 missionSpatialReview 生成；二维地图只画完整出航／自身返程，不伪造执行轨迹或接管结果。

本轮没有新增 C++、算法字段、依赖、wire、自动任务接替或启动权限；原 summary14／provenance17、未知政策／共同 floor、完整新配置及当前能力／电量审阅继续。可见入口和源码接线不等于工程可用或真机放行；未编译、运行、测试、仿真或操作设备。下方513是当时后台阶段的历史。

## 513：从完整备用预算创建独立单机出勤提案

本模块把原完整备用保障中的一架备用机，显式重新规划成一个独立单机新任务。原任务、成员、Run、机载任务、占用和审批都不修改；不是原 Run 的继续、空中接管、自动补位或启动许可。源码接口及消费者接线不等于已有可操作 UI；本批不新增界面按钮，人工入口后续继续。

### 输入与公共接口

四个 native static 接口：`build(source, request)`、`validateProvenance(provenance)`、`matchesTask(provenance, task)`、`matchesPlan(provenance, plan)`。不注册 QML 类型，不读取 Vehicle、账户、Registry、磁盘或网络，不执行上传和飞行命令。

`source` 固定三键 `binding, task, plan`，均为非 null Map。binding 是原完整 exact7／exact9：`sourceTaskId, recordId, recordRevision, planId, contentHash, reviewEvidenceHash, storeEpoch`，九键另有 `inputGeneration, reviewGeneration`。原 task 的 `taskId`、plan 的 `sourceTaskId` 必须与 binding 的 sourceTaskId 同值，planId 相等，完整 plan hash 等于 contentHash。哈希只核值一致，不认证调用者、原文件、当前设备、原审批或实物能力；真正产品入口必须持有同一个实际存档／审阅的完整来源，不能将自行拼接的 map 当 native 认证。

source plan 必须有完整非空 active 成员和独立备用列表（各最多 32），assignments 与 members 完整一致，成员／备用 ID 1..255 唯一且不交叉。原任务完整 reservePlans 必须恰覆盖原实际备用集合，每个描述都是 exact4 `vehicleId, start, returnTo, reservePlan`，原政策走 [ReserveEnergyPlan](FleetReserveEnergyPlan.md) exact8 normalize；端点和替补对象逐一验证，不从有效前缀挑出一份表面正常的来源。

原 task constraints 若出现，必须是非 null Map；再按原 [Compiler](FleetMissionCompiler.md) 同一闭集八键规则，仅在 constraints map 未提供该键时从 task 根补 `timeLimitSeconds, deadline, startAt, minimumSeparationMeters, maxAltitudeMeters, maxAltitudeReference, geofenceRequired, returnReserveSeconds`，其完整结果须与原 plan.constraints 相等。未知约束不能被计划副本覆盖丢弃。保留原 task.area 并以原完整 members 重建已有 routeBoundaryPolicy，存在的原 policy 必须同值；不通过删孔洞、setback 或改 area 偷换来源。新任务还会对新单机完整路线重建自己的共同边界政策。

`request` 固定三键 `reserveVehicleId, executionProfile, operatorConfirmed`。必须明确 true 确认，选中 ID 必须属于来源的实际备用集合；profile 是完整新非空 Map，走 [ExecutionProfile](FleetMissionExecutionProfile.md) normalize，不借原主机 profile 或默认值。这里只支持 relative-dwell 估算：新去程巡航、返程巡航以及明确爬升／下降速度必须与选中 reserve policy 对应数值一致；vertical 两个速度缺失也拒绝。profile 的 none／RTL 等有效但尚不支持编码的要求不被改写成成功，仍由现编码和审阅链阻断。

### 路线与预算算法

先用同一个 `FleetReserveEnergyPlan::build` 对原选中描述、原完整 primaryMembers 和完整 constraints 重建预算，核对选中 source energyPlan 的全部几何／成本／目标 route hash／固定 false 声明。两项能力标量允许实际已知或未知的严格形状；strict policy 的三个声明字段不得省略或夹入 legacy 行，并核声明 exact11 以及 review／baseline 的固定闭子形状。这里只检查保留值的内部一致性，不重建当前能力认证；新任务必须重新取得当前工程声明、基线及电池依据。

随后不用预算简化后的 outboundPath 直接出任务：那只是时间预算视图，会省略主机原 heading 等要求。新 active 路线明确为选中备用自身 start ＋所替补主机完整原 waypoints；完整原序航点全部校验、重编号、保留原 heading 与完整 holdSeconds，不依据任务进度跳过任何点。去程最后一点到备用自己的 returnTo 构成独立两点返程，返程开始点不重复计停留，也不凭空补返程 heading。经纬只支持现编码的 ±80／±180 域，AMSL −1000..20000 且服从原显式 AMSL ceiling。

原路线、task、constraints、主机成员、原始 assignment 或 routeTiming 的绝对时间要求、未知航点字段和不可表达时序会拒绝；不会为了重规划删除绝对调度。新 member 的 movement 时长、出返距离和返回时长来自上述同次重建，hold 仍保留为各真实点的完整独立停留，供现 Coordinator 只加一次。原 standby 不成为新飞行任务动作，估算不再加一遍原 standby 等效耗能；原保留的显式任务时长／安全 floor 仍不降低，不能将来源电量或旧余量当新飞机当前电池状态。

完整原共同安全、能力、能量、电量、模板、禁飞、区域以及未知有界政策保留。原主机 compiled member 与对应原 raw assignment 合并，raw ID 采用原 Compiler 的 vehicleId／systemId／id 一致整数别名（可只提供其中一个），已提供 ID／角色别名和多份 assignment 别名须一致；除被新几何／用途明确替换的已知字段外，未知 member 政策也保留。原本机 duration／return／minimumSpeed floor 保守取最大值；Compiler 若丢掉未支持的 member 要求，完整 member hash 必须拒绝，不能先删除该字段取得通过。

显式新用途会替换原 roster／角色数量／Leader 要求／active assignments／备用集合／运行审批等作用域，生成 vehicleCount=1、目标对应单角色、reserveCount=0；这不宣称原多机任务减少角色后仍能安全继续。原 clearanceInput 完整保留，不猜新 ID、接地声明或净空图，故仍可能因原 ID 不适用而阻断。当前任务占用、权限、上传、StartPermit、动力学与实际路径认证均走原新任务审阅链，不能由本提案的值匹配放行。

### 输出与强匹配

build 返回 exact7 `ok, reason, task, summary, provenance, flightCommandReleased, executable`。失败时 task／summary／provenance 全空，不交付可用前缀；两个许可布尔恒 false。成功 reason 是明确空字符串。

task 唯一新来源字段为 `reserveDispatchPlanningSource`，与旧区域／single remainder／group remainder 三来源互斥。provenance 固定 schema1／exact17：

`schemaVersion, sourceKind, sourceBinding, sourceTaskHash, sourcePlanHash, sourceDescriptorHash, sourcePrimaryRouteHash, sourceReserveBudgetHash, reserveVehicleId, replacementVehicleId, derivedTaskHash, newRouteHash, routeBoundaryPolicy, historicalOnly, authorityRestored, flightCommandReleased, executable`。

sourceKind 固定 `reserve-plan-dispatch-proposal`；historicalOnly=true，其余三个权限布尔 false。完整 source plan hash 还必须等于 sourceBinding.contentHash。没有 Run／startup ID、进度或旧执行权限恢复字段，不给备用伪造 active payload 或原生 Run 来源。

`derivedTaskHash` 除 fleet／intent／taskId／templateValidation／本来源键外绑定完整新 task。`newRouteHash` 绑定完整单机 member（只去三个 Compiler bookkeeping 字段）、完整 constraints、完整 routeBoundaryPolicy 和 canonical strict performance 布尔：缺／false 同为 false，存在坏值或 null 拒绝，true 不能整体剥掉后退 legacy。

`matchesTask` 走真实 task assignments 和由 area 重建的 policy；`matchesPlan` 走真实 compiled members=assignments，必须严格单机、空 reserveVehicleIds／reserveMembers、完整 policy 一致，不通过任务投影或顶层描述符绕过。允许 Compiler 插入来源前尚无本来源键；若已有则必须与原 provenance 完整相等。不是只核来源标签、前缀、首尾、总时长或一个部分 hash。

summary 固定 14 键：`reserveVehicleId, replacementVehicleId, role, outboundPointCount, returnPointCount, estimatedIngressSeconds, estimatedTaskSeconds, retainedHoldSeconds, estimatedOutboundSeconds, estimatedReturnSeconds, standbyReplayed, routeClearanceVerified, performanceCertified, flightCommandReleased`。最后四个布尔均 false（standbyReplayed 表示没有重演旧待机）。summary 不作为可恢复或可执行来源。

### 资源和工程交接边界

新单机出返路线加全部动作不超过 2048 项，故也不超过原全组 8192；原完整 source routes 另核每机 2048 点／全组 8192 点。完整源／result 32 MiB、task／plan 匹配 16 MiB、request 128 KiB、provenance 64 KiB。树深≤24、节点≤1,048,576、map≤1024、list≤32768、字符串≤32768；有限数值在现 JSON 精确整数支持域内。不能截断原航点、丢损坏后缀或增大预算来产生成功。

需工程师后续验证：合法 binding7/9 的完整 source3 与新 profile；原头部／中间／末尾损坏；绑定与 constraints 根别名补齐；raw 三种 ID 别名及冲突；原 area／policy 冲突；strict top 和行声明整体剥离；原未知政策被 Compiler 丢弃；新 profile 速度／端点不匹配；保留完整 heading／hold；旧 standby 不重演但所有 floor 不减少；2048／8192、32 MiB／16 MiB／128 KiB／64 KiB 全边界；新 source 四路互斥和普通 Planner→Compiler→当前能力／能量→PlanStore→PayloadBuilder 贯通；原实际 StartPermit 或来源不充分时仍阻断。

本批只写源码、工程登记和交接说明并直接静读。没有编译、测试、执行算法／parser／helper／hash／QML／SQL、打开应用、仿真、操作设备、修改 PX4、刷机或放行。保留值一致性不是实际性能、净空、任务完成、原生身份或飞行安全证明。

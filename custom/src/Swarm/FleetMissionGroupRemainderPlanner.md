# FleetMissionGroupRemainderPlanner

## 543：新任务重新派生名义时间，不继承旧路线估算

`memberPolicies` 在建立新成员时剥离 Compiler 本次派生的 `nominalRouteTiming`，不把旧路线／旧速度的名义时间当作不可变政策传给新任务。来源的 `memberProjection` 同样不将该派生诊断与请求等同：新任务的真正geometry／executionProfile／其余政策仍完整参与原匹配；新Compiler将按实际新路线重新计算，PayloadBuilder另行核本次原生派生值。此窄排除不剥其它未知政策、不放宽原始Run／task／group provenance、批准或占用边界；旧无字段来源仍保持。

只编辑及直接静读，未运行Planner、编译、测试、hash或业务。

## 513：新备用出勤来源不能与联合余段混用

新增 [备用出勤Planner](FleetReserveDispatchPlanner.md) 为独立单机新任务用途，不属于本模块的历史整组余段。task及compiled-plan共同几何匹配明确拒绝同时携带reserveDispatchPlanningSource。若该新任务将来真实形成Run，再从其完整原生／历史材料规划新的联合余段，commonPolicies只清掉旧出勤规划来源，新的group provenance重新绑定本次全部映射及政策；不保留两个竞争来源，不恢复旧批准或执行权。原schema1／2／3及512备用配置合同保持。

备用出勤本批为后台规划、Compiler及PlanStore真实消费者接线；Root／可见操作入口与完整执行仍待开发。未编译、运行算法或测试，不能据新增来源将原25目标计为完成。

## 512：联合余段任务显式重建备用保障

本轮补的是实际新任务规划配置，不是取消旧安全拒绝，也不是自动空中接管。旧请求 exact3 保留原 schema2／exact27 输出及原 `reservePlans`；只有显式增加 `reservePlans` 键的 exact4 请求进入 schema3。字段存在但为 null、Map、布尔或其他非 List 类型时拒绝，不退回旧模式。

### 新请求与配置边界

新请求固定 `members, reserveVehicleIds, operatorConfirmed, reservePlans` 四键。前三项原规则不变；`reservePlans` 为非 null List，长度与新备用 ID 列表完全相等，最多 32 项，每个新备用 ID 恰出现一次，与所有新 active ID 不交叉。空备用集合只能配显式空列表，仍不得减少原实际／声明最低备用数量。

每个新描述符固定四键 `vehicleId, start, returnTo, reservePlan`。两个端点必须是完整五键 WGS84／AMSL：`coordinateSystem="geodetic", altitudeReference="amsl", latitude, longitude, altitude`；经纬范围分别为 ±90／±180，AMSL 为 −1000..20000 米，数值必须有限且不能借字符串或布尔转换。不会从旧起点、新主机起点、当前位置或旧返航点推断缺项。

`reservePlan` 直接调用 [FleetReserveEnergyPlan](FleetReserveEnergyPlan.md) 的原 exact8 `normalize`：`schemaVersion, replacementVehicleId, standbyMode, standbyDurationSeconds, standbyConsumptionFactor, cruiseSpeedMetersPerSecond, climbSpeedMetersPerSecond, descentSpeedMetersPerSecond`。替补目标必须属于本次新 active 集合，不能指向不在新集合中的旧主机、备用机或自身；明确保留为新 active 的原机号仍可选。模式、待机时间、因子和速度都是操作员明确重新填写的新运行配置，允许与旧描述不同；这不是宣称旧备用政策全部不变，也不从旧配置自动猜测对应关系。端点与策略均规范化，最终按 `reserveVehicleIds` 原顺序保存完整描述符，拒绝重复、缺失、外来目标或未知键，不保留有效前缀。

原共同任务政策先完整生成 `sourceTaskPolicyHash`，包括旧备用描述；随后仅在 explicit 新模式中以完整新值替换 task 的 `reservePlans`。原最低备用数量、角色、能力要求、任务／返航时长 floor、区域、禁飞、安全、电量、未知有界政策均沿原规则，不因新备用配置而删除。`clearanceInput` 不自动换 ID／换端点，仍可能阻断新组审阅；本轮不解决动态实际轨迹、StartPermit 或断链自动替补。

### schema3 与两个真实匹配入口

新 provenance 是 schema3／exact28：原 schema2 的完整 27 键加完整 canonical `reservePlans`。仍限 64 KiB，旧 schema1／2 验证分支保留。`derivedTaskHash` 绑定完整新任务；仅新 schema3 的 `newRoutesHash` 除原成员／约束／备用 ID 外还绑定完整新备用意图及 canonical `requirePerformanceEvidence` 布尔（缺省／显式 false 都为 false，存在但非 bool 或为 null 则拒绝）。不能通过同时删掉计划顶层 strict 标志和全部备用声明证据退回 legacy 匹配；完整 task hash 仍区分原政策显式 false 与缺省。原 summary18、result7、成员14、物理／许可 false 字段和现有输出预算不扩。

`matchesTask` 明确消费新 task 的 canonical 描述，核完整任务 hash 和共同几何／成员映射；它不伪装成已编译备用结果。`matchesPlan` 的 schema3 路径则只消费真实 Compiler 已有的 `reserveMembers`，不接受顶层同名 `reservePlans` 作为绕过证据。每个原序备用行须完整包含 vehicle／reserve role／人工确认／energyPlan；用同一 `FleetReserveEnergyPlan::build` 对新描述符、实际编译主机完整路线与共享 constraints 重新生成几何和成本，再比对所有生成字段：包括 policy、vehicle/replacement、完整出返路径、目标 route hash、完整停留、待机等效成本、进入／任务／返程时间与全部固定 false 声明。不是只比较首尾或单一总时长。重建共用每路径 4096 点、全备用 32768 生成点的原上限；预算不逐备用重置，超限整份匹配失败。

实际编译器可能额外携带声明能力信息：两个能力标量严格核 bool／有限值／正速度限制，额外声明字段为闭集并核 exact11 证据及其固定 review／baseline 子形状；这些额外字段的存在性必须与计划顶层实际 strict 布尔一致，不能由行自身选择是否需要声明。这里只核保留值的类型与内部一致性，不把历史 `current` 标签当当前 Registry、设备身份、工程声明或实测能力；外层当前审阅和最终原生门禁仍必须重新提供真实证据。缺当前能力导致的原 Compiler unmet 不补成成功，不新增 PlanStore 顶层字段或放宽其许可。

实际链仍为本模块生成新 task → [Compiler](FleetMissionCompiler.md) 构造对应备用 energyPlan → [ReviewCoordinator](FleetMissionReviewCoordinator.md) 按各实际主／备用预算派生 → [EnergyScheduler](FleetEnergyScheduler.md) 当前能量审阅；不是新建无人消费的 metadata helper。备用仍按其自身明确端点对本次新主机的完整新路线进行预算，不把原 Run 的观察进度解释为已完成工作，不替飞控动态接管。规划耗时、续航和待机系数都是估算／声明，不是实测负载模型或安全认证。

### 工程师后续需执行（本轮全部未执行）

- 旧 exact3 请求、schema1／2 来源保持原语义；新 exact4／schema3 可为空或为完整新备用集，错误存在值不退回旧模式。
- 原主机 1 改派 4、备用 3 保留或更换为 5：显式指定新目标和独立两端点，确认 Compiler 及逐备用能量预算真正采用新配置；仅换 ID 不填描述仍拒绝。
- 顺序不同的完整新描述输入应规范化到新备用 ID 原序；重复、漏项、foreign ID、非主机替补目标、缺端点、非法模式／因子／速度／高度、额外键整份拒绝。
- 不得减少原最低备用数；原共享能量／安全 floor 和未知政策不被擦除。clearance 旧 ID 未重新提供时仍明确阻断。
- 修改新描述、编译备用 policy、任一中间路径点、首尾、替补主机、目标 route hash、停留或成本后，task／compiled-plan 匹配分别拒绝；不能把伪造顶层 reservePlans 当真实 reserveMembers。
- 32 架备用、每路径 4096 点、全组 32768 生成点、64 KiB provenance 等边界及错误后缀拒绝，不留成功前缀；实际性能证据缺失或撤销仍不取得当前放行。

本轮仅源码及文档静态修改与直接阅读；没有执行规划算法、编译、测试、仿真、硬件或刷机。以下为历史合同；与本节不同的旧请求与备用政策描述仅指旧 exact3／schema1／2 路径。

## 478：联合来源schema2保存完整共同边界政策

新provenance为schema2／exact27，新增完整canonical `routeBoundaryPolicy`。`validateProvenance`严格核政策，`matchesTask`由原task.area＋完整assignments重建，再把该明确policy传给同一plan投影；`matchesPlan`核双方policy存在性及值，再核全部成员／顺序／原路线与共同约束。不是首机摘要或只有policy哈希标签。

旧schema1／26键仍仅原历史合同，不补无约束policy，不把旧机载执行身份恢复为新任务。原资料集合若有新policy，StartupTransaction共享validator要求sourceTask.area与sourcePlan完整policy一致；旧无policy集合保原形复现。后续实际编码仍normal build，缺policy不能新上传。

commonPolicies清全部旧区域／single／group来源及派生policy，以原area重新计算，不继承另一组旧审批。全部member与保留备用政策仍按原合同；无自动改派、控制权恢复或飞行许可。未编译、运行或测试；下方schema1描述为历史。

下方为历史记录。

## 475：完整历史 Run 的联合剩余任务提案

本模块新增一条纯值规划路径：一个完整的 Archive v2 原组来源，加上操作员为每个原成员填写的新参数，生成一个联合多成员新任务。它不恢复旧 Run，不把观察进度解释为完成，不重建旧审批或执行权限，不发指令；源码修改不代表已编译、测试、仿真或具备真机放行条件。

相关来源与消费者见 [FleetMissionRemainderArchive.md](FleetMissionRemainderArchive.md)、[FleetMissionStartupTransaction.md](FleetMissionStartupTransaction.md)、[FleetMissionCompiler.md](FleetMissionCompiler.md)、[FleetMissionReviewCoordinator.md](FleetMissionReviewCoordinator.md)、[FleetMissionRemainderView.md](FleetMissionRemainderView.md) 和 [FleetMissionPlanStore.md](FleetMissionPlanStore.md)。旧单成员 Planner 的来源与 provenance 合同不变。

## 公共接口

`build(source, request)` 返回固定 7 键：`ok, reason, task, summary, provenance, flightCommandReleased, executable`。成功时 `reason` 是显式空字符串；失败时三个业务 map 全空，不输出成功成员前缀。两个权限布尔始终为 `false`。

`source` 必须是完整的 Archive v2 固定 8 键封套，先走 `FleetMissionRemainderArchive::archiveValid`，再走 `FleetMissionStartupTransaction::validateCollection`。不能以报告摘要、某几个可用成员或自行命名的 native 来源替代它。Archive ID、完整包内容 hash、原 Run/Startup/事务身份与 binding 都进入新 provenance。hash 只表示值一致性，不表示文件真实性、当前设备身份或硬件认证。

`request` 固定 3 键：

- `operatorConfirmed`：严格的 `true` 布尔。
- `members`：与原 `vehicleIds` 同长度、同顺序的完整参数列表。
- `reserveVehicleIds`：操作员显式给出的新备用 ID 列表。

每个请求成员固定 10 键：`sourceVehicleId, sourceWaypointIndex, targetVehicleId, start, returnTo, cruiseSpeedMetersPerSecond, climbSpeedMetersPerSecond, descentSpeedMetersPerSecond, executionProfile, operatorConfirmed`。其中后 9 键复用旧单成员请求的完整校验：人工选择的包含式航点索引、显式 WGS84/AMSL 两端点、正有限速度和全量新执行 profile。空 profile 不等于已具备上传条件。

`validateProvenance(provenance)` 返回固定 3 键：`ok, reason, provenance`；失败为空 provenance。`matchesTask` 校全任务 hash，并复用完整成员/顺序/路线/逐机 floor 投影；`matchesPlan` 校最终 Compiler 成员与同一共享 constraints，不能仅靠任务总 hash 忽略逐机映射。二者只证明值匹配，不认证 native 所有权。

## 整组算法与拒绝条件

全部原目标必须保留，原 ID 每个出现一次；请求中的原 ID 不得换序，新目标 ID 必须唯一。任一原成员尚未 MODE 提交、缺少完整材料、角色不支持、映射或预算不合法，都拒绝整个联合提案。不会自动排除不可用成员，也不会把“已观察到的航点”当成已完成的任务。

每个成员将完整材料标记为 `historical-run-planning-snapshot` 后复用旧单成员几何/时序算法；这里不制造 native 来源，子结果不作为独立新任务交付。所选原航点本身及之后所有航点均保留，包括完整停留时间和原航向要求；仅重新估计显式进入段、保留路线和显式返程。水平、爬升、下降耗时顺序相加，结果是估算，不是同步或权威时序证明。原任务、成员、约束、路线或 profile 中存在不能继承的绝对时序要求时，整组拒绝，不能静默降成估算。

所有子几何完成后，构造一次联合任务：保持原角色、完整共享政策和 constraints，注入一组原序 assignments，再基于最终的共享 constraints 重算每个 `newRouteHash` 和全组 `newRoutesHash`。不会拼接子任务各自已经提升过的全局政策或共享返航预算。

已知的旧路线/成员分配/审批等作用域被显式替换；原 `planningSource`、`remainderPlanningSource`、`groupRemainderPlanningSource` 不嵌入新任务。其他有界任务政策原样保留，包括 capability、模板、安全、区域、禁飞、能量、时间限制以及未知字段。原成员字段先与对应原始 assignment 合并，除已知被新几何替换的字段外继续保留；Compiler 若丢掉这些未支持成员政策，完整成员投影不匹配，当前审阅必须阻断，不能靠删字段取得匹配。保留字段不等于本模块认证其政策语义。

旧 coverage 生成的 `coverageLegStart/coverageLegCount`、原成员的 `flightCommandReleased=false` 和进入/分段/覆盖估算等明确派生字段，在重建新路线时移除；它们不是必须由新普通 Compiler 继承的未知政策。不会把旧 coverage 腿号沿用成新路线索引，也不会因保留旧派生标记而必然拒绝正常面积来源。新任务与 provenance 的飞行释放标志仍固定为 false。

若原任务提供多组 assignment 别名，必须完整相等；原始 assignment 必须能唯一关联原 active ID，且已提供的 ID/角色别名必须与对应原编译成员一致。外来 reserve/foreign assignment 等不能忠实归属的作用域明确拒绝，不静默过滤。正常 Draft 已在 reserve 分支建立独立 `reserveVehicleIds`/`reservePlans` 后跳过 active assignments；Compiler 的 reserve 预算也只消费这套独立描述，不从 active assignment 中猜测备用策略。

原显式角色数与原组实际角色不能冲突，且最多一名 Leader。`leaderRequired=true` 而实际无 Leader 时拒绝；原显式 `false` 被保留，它表示“不要求”，不是“禁止已有 Leader”。

## 逐机性能、时长与备用政策

每个最终 assignment 都有自身的 `minimumSpeedMps` 和 `returnReserveSeconds`；有原正时长 floor 时另写 `durationSeconds`。它们实际进入 Compiler 和当前能力/能量审阅，不只是 summary。

- 本机任务时长 floor：原编译成员和原始 assignment 的 `durationSeconds/missionDurationSeconds/taskDurationSeconds/timeLimitSeconds` 中全部正有限值的最大值。原 `timeLimitSeconds` 本身仍保留，不能把限制字段当成已完成时间。
- 本机返航 floor：原全局 task/constraints 的两个 reserve 别名、本机两个 reserve 别名及本机 `returnDurationSeconds` 的最大值。仅原全局两个 scope 的 reserve 别名全部缺省时，以原流程的 120 秒默认值为起点。共享 constraints 不被任一成员的长返程预算抬高；其他机的账不受它污染。
- 本机最低速度：原任务全局 floor、原编译成员/原始 assignment 的本机 floor、新人工去程与返程巡航速度的最大值。不把全组最快机要求提升成所有异构飞机共同的最低速度。实际 Compiler 必须以对应目标机当前的 `maxSpeedMps` 证据核对，缺省、未知或低于本机要求均不能借用别机证据。

`reserveVehicleIds` 必须唯一且与所有新 active ID 不相交，数量不能低于原实际备用数量或原声明 reserveCount。完整原 `reservePlans` 原样保留，不重写原备用 ID、replacement 目标或预算；新人工 ID 若与原描述不匹配，真实 Compiler/备用能量审阅应明确阻断。本轮没有“顺手改写备用政策”的隐含接口，也不把缺失备用描述解释成零成本备用机。

完整原 `clearanceInput` 保留，不裁剪、不变更成员 ID/顺序、不替换地形/接触声明。新 ID 与旧声明不匹配时仍由原当前审阅阻断。startup 数值接受域包含、动作阶段接触以及原执行许可门禁未被本模块解决；不能将其 pending 或 `numericContainmentProven=false` 改为成功。

## 输出形状与约束

新任务使用独立字段 `groupRemainderPlanningSource`，与区域和单成员来源互斥。provenance 固定 26 键、schemaVersion 1、sourceKind `historical-run-group-planning`：

`schemaVersion, sourceKind, sourceArchiveId, sourceArchiveContentHash, runId, startupId, transactionEpoch, transactionGeneration, deploymentId, binding, sourceVehicleIds, targetVehicleIds, reserveVehicleIds, memberCount, members, sourceTaskPolicyHash, derivedTaskHash, newRoutesHash, selectionPolicy, timingBasis, historicalOnly, progressUsedToSkip, completionProven, authorityRestored, flightCommandReleased, executable`。

其成员固定 14 键：`sourceVehicleId, targetVehicleId, targetRole, sourcePayloadHash, originalRouteHash, sourceMemberHash, sourceWaypointIndex, sourceWaypointCount, outboundPointCount, returnPointCount, newRouteHash, missionDurationFloorSeconds, returnReserveFloorSeconds, minimumSpeedMps`。

summary 固定 18 键：`sourceKind, sourceArchiveId, sourceArchiveContentHash, memberCount, sourceVehicleIds, targetVehicleIds, reserveVehicleIds, members, estimatedMaximumOutboundSeconds, estimatedMaximumReturnSeconds, totalRetainedHoldSeconds, totalPointCount, selectionPolicy, timingBasis, routeClearanceVerified, performanceCertified, missionCompleted, flightCommandReleased`。summary 成员固定 17 键，包含原/新 ID、角色、包含式索引、点数、进入/作业/停留/去程/返程估算、本机三项 floor 和新 profile 是否提供。summary 不附全部原资料，不作为可恢复来源。

固定选择政策为 `inclusive-waypoint-full-dwell-replay`，固定时序依据为 `sequential-horizontal-climb-descent-estimate`。`historicalOnly=true`；`progressUsedToSkip/completionProven/authorityRestored/flightCommandReleased/executable` 全为 `false`。summary 的路径/性能/完成认证也全为 `false`。

原组最多 32 架，备用列表最多 32 项；ID 1–255。每机路线与 profile 动作合计最多 2048 项，全组最多 8192 项；全树深度最多 24，节点最多 1,048,576，map 最多 1024 键，list 最多 32768 项，字符串最多 32768 字符。完整源和结果各限 32 MiB，任务/匹配计划各限 16 MiB，联合请求限 4 MiB且逐成员仍走旧 128 KiB 请求限制，provenance 限 64 KiB。数值必须有限且不超过 JSON 精确整数域上限 2^53−1，枚举和布尔不靠宽松转换接受。超限整份拒绝，不扩大既有预算、不截断前缀。

## 工程师接手案例（本轮未执行）

- 同一组 2–32 个完整 committed 成员：一次人工提案、一次全组当前审阅，核对全部原序 ID 与完整路线，没有逐机重复审阅或漏成员。
- 任一中间/末尾成员未提交、重复新 ID、换序、超项或原成员政策损坏：整个提案失败，三个业务 map 全空。
- 慢机/快机、本机不同返航和时长 floor：逐机当前速度证据和能量账不同，长预算不污染别机，所有 hash 使用最终共享 constraints。
- `leaderRequired=false` 且存在一名 Leader：保留 false；true 但无 Leader、两名 Leader 或原角色政策冲突：明确拒绝。
- 有独立 reservePlans 的来源：保持全部描述；备用数量减少拒绝，新 ID/替补目标不匹配继续当前审阅阻断，不丢弃备用政策。
- 原绝对时序、未知成员政策被 Compiler 丢弃、旧 clearance ID 不匹配：不擦除政策以取得通过，保留明确阻断。
- 篡改来源 archive ID/hash、成员映射/floor/路线/停留/profile 或最终 constraints：完整源/中央匹配不得接受不一致值。这个案例不意味着无签名文件具有真实性认证。

以上是需工程师执行的验证与交接范围，不是测试记录。本文档及代码仅经直接静读与文本空白检查；没有运行算法、编译、测试、仿真或真机操作。

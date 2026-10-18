# FleetMissionCompiler 任务编译契约

## 544：普通公共路线在实际成员中完整展开

修正“Compiler／预算可用公共route，但实际Payload只接受成员自身route”的断点。仅在没有coverage请求、原task明确提供且完整规范有效global waypoints／route、成员完全没有waypoints字段时，将规范出程整份复制到实际member并更新waypointCount；显式空／坏成员路线不救，coverage辅助角色不继承公共survey。不是在上传端临时猜fallback，也不新增接口、类或wire版本。

新增路径先检查**原始**所有global别名，以及真正选中的原始return来源（成员return别名优先、逐机task.returnPaths次之、最后common return别名）；不能只检查normalizer剥字段后的map。输入限实际List或带waypoints的Map，每条最多4096点、非空。wrapper闭域为waypoints、coordinateSystem／altitudeReference／localFrameId、startTime／startAt、endTime／deadline、durationSeconds／missionDurationSeconds／taskDurationSeconds、timingEvidence／timeReference。point仅接受原normalizer确实保留语义的坐标／高度别名、heading／yaw、holdSeconds、timeSeconds／timestamp／time／t／etaSeconds、estimatedElapsedSeconds及坐标／高度／local frame／time reference／timing evidence声明；数值字段须实际非null数值，原sequence若提供须实际整值等于本路线原索引（含JSON Double的精确整数，不收Bool或String）。当前normalizer未消费的timestampSeconds、其它speed／action／camera／schedule或陌生字段均拒，不让这些要求被删掉后进入新可序列化路径。坏别名不能被另一份已规范相等的别名掩盖。失败增加blocking且不物化fallback，原Payload仍因缺成员route拒绝，不能只剥字段获得可下发结果。

同一新inherit分支还独立限定原始member输入：只接受原Compiler真正保留的identity／role别名、完整profile、route／return、既有时序／估算／性能门槛／reference及returnRequired；不接从未复制的member.heading、acceleration、其它capability／动作或未知要求。原显式成员路线不因本窄gate而改变接受域。不得删除这些未编码要求后把过去因缺member route拒绝的请求变成可下发。

global wrapper原start／end／duration三类分别按**成员整族presence**优先，成员startAt存在即不再插global startTime；duration各别名、deadline同理，坏原值不被修复。补齐后重新核最终别名一致和start/end不反向，例如成员startAt=100与global endTime=50必须阻断，不各自合法就称组合合法；不改原值、不跨任务推断。timingEvidence／timeReference缺失才继承。原plan.routeTiming完整保留，constraints／task原后续预算优先级不改，不把低优先级task字段提升到member。点级原时间、heading、驻留及reference均在同一规范路线保留，已知未编码时序仍由原审阅／Payload sourceSchedule和unencodedRequirements明确保留，不能声称飞控已执行绝对时序。

仅至少一个成员真正完成global物化时，实际plan根同名保留原task的有限25个可选声明：18个数值字段timeSeconds／time／timestampSeconds／timestamp／t／etaSeconds／startTime／startAt／endTime／deadline／durationSeconds／missionDurationSeconds／taskDurationSeconds／returnStartTime／returnEndTime／returnDurationSeconds／timeLimitSeconds／returnReserveSeconds，4个标签timeReference／returnTimeReference／timingEvidence／returnTimingEvidence，以及3个参考coordinateSystem／altitudeReference／localFrameId。原值、typed-null、别名presence均保留供诊断；按现Payload schedule数值／正预算／标签／alias和正反程端点规则增加blocking，不修坏值。nested task.schedule没有本协议，存在即拒新物化。旧显式成员路线／coverage路径不因此新增根字段，也不开任意task白名单。PlanStore同步保存这25项，Payload实际构建仍核根声明并保sourceSchedule；完整task进入planId不替代语义保留。

只有完整出程**和**显式返程所有点已有相同且非空coordinateSystem／altitudeReference时，才把该明确共识汇总到缺失的member字段；没有AMSL、真实原点／高度或return几何的默认值。已存在成员声明不改写，冲突保原值并阻断。task根3项参考仅断言实际完整出返点与已有member声明一致，不作为缺失datum／local原点的默认值；坏型、冲突或缺完整点声明保原根值并blocking。local／localENU等规划域继续保原规则；当前Payload只支持WGS84／AMSL，不因此获得local转换能力。returnRequired仅在成员缺省时复制明确存在的task实际Bool，缺省不补true，false不改为true，坏型阻断。

543的预算副本及名义时间副本不再各自补global；最终route存在检查与profile端点核也只读actual member，不保另一条规划旁路。完整实际members在reserve预算、边界／exclusion、nominal时间、planId、assignments、Store、几何view、Payload sourceItemMap与后续同源review中使用同一路线。原全源身份、手动确认、风险／容量／高度／profile／编码／净空／StartPermit门不放宽；少于两点、缺returnRequired、未知datum、超Payload总量等仍被原硬门拒绝，根metadata总量64也不扩大。只apply_patch和直接静读，未编译、运行normalizer／模型／hash／业务、测试、仿真或设备；下方543及更早为历史。

## 543：从最终逐机路线派生名义时间，不采信同名注入

在覆盖／原生profile检查、最终几何及边界政策完成之后、planId完整身份生成之前，为每个实际主成员计算独立 `nominalRouteTiming`。复用 [PreviewTimeline](FleetMissionPreviewTimeline.md) 的纯C++ travel-only入口与同一几何内核：actual成员waypoints优先；只有完全缺省成员路线且未请求coverage时才可使用原合法顶层普通route fallback。显式空／坏成员路线不退回公共路线；coverage辅助角色不继承survey路线。独立returnPath含出程末点到首返连接，不凭reserve秒数推造返程。

原caller的同名nominalRouteTiming未进入member复制字段，不能直接保留、改flags或以一份导入摘要当原生产者；本次派生的完整exact11进入实际members／assignments及planId身份。原用户给的estimatedDurationSeconds／timingEvidence／时刻／预算依旧保真，既不被名义值覆盖，也不被用来设置routeTimingCertified或完整任务已知。成员独立缺速度／高度参考／return geometry为明确未知，不借另一架机、group最大值或默认飞行速度补齐。

每个helper调用总出返最多4096点；超界／非法整份nominal诊断为unknown，没有成功前缀。实际后台compileCancelable在各成员前后检查原取消票据，取消仍返回原exact2且无部分计划。已有编译阻断、几何／当前参数／能源／许可政策保持；名义值只能由真实Coordinator扩大已有预算，不是实测时长、耗电模型、任务完成或StartPermit。未编译、运行编译器／helper／模型／hash或测试。

## 537：显式NAV驻留保原请求并拒编码塌零

`normalizeWaypoint` 的可选 `holdSeconds` 统一调用 [DwellEncoding](FleetMissionDwellEncoding.md)：仅实际非null数值、finite `0..86400`，正请求必须经过Float32上传及所述旋翼NAV微秒计时模型后仍非零；坏型／塌零保原invalid waypoint整批阻断，不夹紧、不删字段。成功的规范路线和原plan／source身份只保 `requestedSeconds`，不以wire或保守预算改写原值。去返路线、辅助成员及已有余段／备用来源沿原同一规范化入口；覆盖自动生成路线不推断新驻留。

缺省与明确0保持不同；驻留不是ETA、绝对时序、同步启动或持续半径／精确悬停证据。原Compiler可行性、能量和最终StartPermit门不放宽。仅编辑和直接静读，未运行编译器／helper／parser／算法／hash、编译、测试、仿真、业务或设备。下方为历史。

## 536：逐航点朝向保留原请求，不借扫描轴或编码值替换来源

`normalizeWaypoint` 对实际逐点 `heading`／`yaw` 复用 [HeadingEncoding](FleetMissionHeadingEncoding.md) 的有限数值、别名一致和 `[0,360)` 规则；明确零仍有字段，缺省不补零。规范计划只保存原请求 `heading`，不在编译时把它替换为1/256度网格值。完整task、去／返航点和members仍共同进入原planId及归档内容身份。任务级／成员级heading不被当作逐点命令；覆盖规划 `headingDegrees` 仍仅为扫描线方位。

该语义沿实际Draft的独立NAV路线输入／恢复、原route和returnRoute别名、逐机returnPaths及单／组余段、备用独立出勤原路线保留。重新规划的人工起点、returnTo和新生成返航连接不推断或复制一个机头朝向。实际编码、量化诊断和读回比较由 [PayloadBuilder](FleetMissionPayloadBuilder.md) 的显式v3完成；不会仅凭编译保留字段而认证实机达到朝向、强制等待、起降yaw、时序或启动许可。

仅源码及直接静态／普通文本核对，未执行编译器、helper、解析器、模型、hash实现、业务、测试、仿真或设备；未改PX4。下方为历史。

## 521：普通任务的明确速度／续航准入门槛

完整区域 `planningSource` 现在通过中央 Draft validator 接受 schema6。重建任务仍为原八项，加实际存在的 `reservePlans`、`requirePerformanceEvidence`、`minimumSpeedMps`、`minimumEnduranceSeconds`，最多十二项（不计附回的 planningSource）；逐键全值核对，不允许删掉门槛后降成旧来源。旧 schema1–5 的闭键和 presence 关系仍由原 validator 保持。Root 的历史／修复来源前检允许同一 schema6，窄 Draft facade `setPerformanceAdmissionPolicy(policy, expectedRevision)` 只转交实际原 Draft 的完整三项可选 Map，保原锁、精确 revision、选择和通知尾核，不授予性能或飞行权限。

两个 task 根门槛不受 strict Bool 开关控制：提供即须实际非 null、有限数值，范围 `0..9007199254740991`；缺省与显式零不同，字符串／Bool 不转换为数值。正门槛对每个实际主机和备用机分别执行，要求唯一原 fleet 快照和该机已知的自身值；缺失、精确 −1、null 或坏型不能满足正门槛，已知但不足单独给 unmet／blocking。非严格模式仅读该机原规划 descriptor 的 `maxSpeedMps` 和第一项存在的 `enduranceSeconds`／`enduranceSec`，不回退到另一架机、组最大值或其它能量模型；这不是真实性能认证。严格 true 仍只采用完整当前本地工程声明，原成员速度、实际 profile、覆盖速度和声明来源门不变。零／缺省不额外强求相应数值 known，也不关闭其它安全或严格政策。

实际 plan 根仅在 task 明确提供时保留两门槛，包含零；完整原 task 本来参与 planId 身份。合法区域源的 strict 三态继续同 presence 保存；旧非严格版本号不变。PlanStore 的完整归档核和正常 PayloadBuilder 的区域源核均要求两门槛与 sourceTask 同 presence、同有限数值，不只把未知字段透传。单机／联合余段及备用独立出勤保留原共同政策并走真实 Compiler 重新检查；没有放宽原路线、能量、clearance、当前证据或 StartPermit 阻断。

未编译、运行编译器／模型／脚本、测试或操作设备。待工程师验证：普通 false／缺省下主备各自 known、unknown、低于／等于门槛；显式零与缺省；严格声明缺失但 raw 数字存在；坏类型／null／非有限／越界；schema6 保存、重开、修复、重审及派生任务保值；源／plan 任一门槛删改时整批拒绝。下方为历史合同。

## 513：完整备用方案派生独立出勤任务的实际编译

新增独立 `reserveDispatchPlanningSource`，与 `planningSource`、`remainderPlanningSource`、`groupRemainderPlanningSource` 四者互斥。仅接受真实非 null Map，由 [ReserveDispatchPlanner](FleetReserveDispatchPlanner.md) 完整 `validateProvenance` 返回相同规范值，并经 `matchesTask` 绑定整个新任务后才缓存；坏类型、坏来源、字段删改或混用均增加真实 blocking 风险／unmet，不删来源后冒充普通可行任务。

实际主成员、完整去返路线／停留、executionProfile、constraints、边界和性能政策成形后，另调用 `matchesPlan`；只有实际计划匹配才保存完整来源。失败保留阻断诊断，不把该来源插入可用计划。新任务来自所选备用自己的起返点及原被替补主机完整路线，不把旧备用直接变成旧 Run 的活动成员，也不把仅顶层 descriptor 视为实际成员／预算已一致。

严格性能仍沿原真实消费者：actual Bool true 保 `compilerVersion=0.8.0`、当前声明证据和主备限制；合法 false／缺省在该新来源的路线投影中规范为非严格，但 true 不能剥成 false／缺省或坏类型。原区域 schema5 显式 false 的 presence 特例不扩至新来源，原非严格版本和输出规则保持。完整新任务与 provenance 继续进入原计划身份；旧未知约束、完整 clearance／边界政策、原共同安全／能力／能量 floor、风险及 pending 保留；数量／角色仅按明确独立单机新 scope 重设，不宣称原组备用保障或任务已接管。

本批仅完整原生后台接线；Root／可见人工出勤入口仍待后续接入。提案、内容摘要和完整匹配都不是来源认证、旧批准恢复、控制权、占用、实际出勤或 StartPermit。未编译、运行实现／模型／脚本、测试、仿真或飞行；下方为历史合同。

## 485：schema5来源的完整政策重建与显式false

区域planningSource经中央Draft validator重建的task为8＋reservePlans presence＋requirePerformanceEvidence presence；所有重建字段逐值匹配，真实task额外键仍按闭集合拒绝。不能给旧1..4源额外塞top策略、把schema5 true删掉／改false，或靠schema数字代替完整源；没有planningSource的普通任务原宽合同不变。

true仍采用483严格消费者、compilerVersion0.8.0和plan.top=true。仅当合法区域来源明确保存false时，输出plan.top=false以保该编辑政策，版本仍0.7.0；旧缺省及无source explicit false任务保持原0.7.0／不新增flag的输出shape。完整task／来源本来参与planId，三态和源revision不可相互冒充。

PlanStore进一步绑定完整源／计划presence和实际Bool值。单／组余段的原政策复制保此字段及derivedTaskHash，不假装已经受控机载续飞；false的无区域源余段仍沿普通非严格输出shape。原几何／净空pending、source互斥及最终许可不放宽。32工程场景全未执行，未编译／运行／测试。下方483的“false一律无plan flag”仅由本节针对合法schema5区域来源扩展。

## 483：严格性能声明实际约束主备任务

既有 `task.requirePerformanceEvidence` 仅接实际非null Bool。true时所有真实primary及reserve须有唯一原fleet快照及[共享当前工程声明](FleetCapabilityEvidence.md)：核全任务最低速度／续航、成员minimumSpeedMps，以及成员实际executionProfile巡航／返航请求；实际生成coverage参与机的共同cruise也逐机比较。unknown不当达到阈值，不借另一架机数字；备用完整预算两处build调用传第5个策略原QVariant，原constraints保持原样。

严格计划 `compilerVersion="0.8.0"` 并保存 `requirePerformanceEvidence=true`；planId的完整identity额外绑定固定算法token `current-local-engineering-declaration-v1` 和主备完整声明sources。false／absent仍0.7.0、不新增计划flag，沿旧规划行为；planVersion1.1及完整routeBoundaryPolicy、原风险和pending不放宽。坏声明增加blocking／unmet而非静默raw回退。

现有显式task／Intent.parameters可承载此策略，未扩Draft.settings／区域表单或新增性能录入器；不能通过删坏字段让草稿接受，也不回填历史批准。声明值是工程意图，不认证真实性能、机载二进制或未上报改装；P0动态状态／实际路径缺证与StartPermit阻断仍保。工程场景见[30项全部未执行矩阵](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)，未编译、运行或测试。下方0.7.0是非严格及历史版本。

## 478：完整原始边界策略进入计划与身份

当前 `compilerVersion=0.7.0`、`planVersion=1.1`，外层 `schemaVersion=1` 保持。最终实际成员、合法全局路线回退及完整备用预算仍先走原名义全段检查，再调用 [Planner](FleetAreaCoveragePlanner.md) 的 `buildRouteBoundaryPolicy(task.area,exclusionMembers)`。成功时完整 `routeBoundaryPolicy` 同时进入计划及 planId 的完整 identity；无孔／零退让也必须显式记录规范的 inactive 策略，而不是让下游从缺字段猜“没有约束”。`areaSummary` 仍只是显示摘要，不承担恢复外环／孔洞／退让／帧的职责。

策略构建失败增加真实 `route_boundary_policy_invalid` blocking 风险与 unmet，不把坏原数据替换成 inactive，不伪造完整几何。此类缺完整策略的新计划可以保留内存诊断，但严格 PlanStore／PayloadBuilder 不保证接受归档或部署。既有名义路线冲突及其它风险不删除，也不借完整策略去掉启动许可／动态净空限制。

区域 `planningSource` 仍须完整重建原任务。新的单机／联合余段 provenance 完整绑定同一规范策略，编译前重建 task 策略、编译后比较 plan 策略；旧未绑定几何的 provenance 不能与带新策略的计划混用后冒充同源。历史原始记录保持 inert，须从完整源重新形成提案与审阅，不能从 summary 或旧批准补出几何／权限。具体版本见 [单机余段](FleetMissionRemainderPlanner.md)、[联合余段](FleetMissionGroupRemainderPlanner.md)。

工程师待执行（本轮未执行）：无约束／孔洞／正退让（含无孔）计划均显式保存正确策略；原坐标／帧／孔洞改动改变完整计划身份；坏声明 blocked 且不能填补 inactive；Draft 源与计划策略替换／删改拒绝；新旧余段 provenance 不混用几何身份；编码后检查仍由 [PayloadBuilder](FleetMissionPayloadBuilder.md) 实际重算，不从本编译阶段继承“已通过”。没有编译、测试、仿真或飞行验证。

下方 477 及更早章节是对应历史版本；其中 `compilerVersion=0.6.0` 不代表当前版本。

## 477：原area中的正退让不被无孔no-op绕过

原最终共享validateExclusionRoutes调用仍传完整原task.area，包含独立boundarySetbackMeters；无孔正值亦读原域并核整段到原outer／hole每边的欧氏距离，所有主／辅助、合法全局回退、完整备用预算及去返桥都适用。正值明确要求原outer内，不沿用476的outside例外。单／联合余段保原area，所以同样重新消费距离声明。

共享显式checker只读取原始拓扑／退让，不为已有路线构造保守Qt候选；候选构造／分裂／预算只属于generate搜索。Compiler已有失败blocking分支保留，不把nominalBoundaryDistanceChecked或source4当启动许可。source4和area字段仍进入完整task／coveragePlan身份，旧精确零／省略保原几何版本。

当前仍是名义路线：PayloadBuilder的E7／float32量化可能改变坐标，areaSummary不保完整边界，不能仅把它传给编码器就声称已复核编码净空。后续须把完整原始边界策略保到实际wire／最终原生门禁并重新核；真实3D运动、误差包络、转弯、StartPermit与正式结果仍未完成。本轮未编译或执行算法／编码测试。

## 476：实际全名义路径消费area.holes

最终identity之前，以原task.area而非areaSummary调用FleetAreaCoveragePlanner::validateExclusionRoutes。使用真实normalized成员，并保留既有合法全局路线回退；完整备用energyPlan的outboundPath／returnPath另按预算副本加入检查，不将备用加入执行assignments。未完成预算原本已blocking，不补造路径。

共享检查包括每个去程、返程及去程末点到返程首点的连续桥接；不因coverage字段已移除而跳过单机／联合余段提案。失败追加route_exclusion_check_failed blocking风险，不丢其它原风险或伪装成成功。无孔旧输入不新增几何要求；错误位置／类型的已知排除声明不能静默忽略。

原task与coveragePlan原本均进入任务identity，schema3 source与完整规范孔坐标随同存档。当前检查只针对名义二维折线，不能认证高度、机体尺寸、误差、实际转弯／起降动态、净空或最终启动许可。本次未编译或运行验证。

## 475：完整原组的联合余段提案进入真实编译

新增独立 `groupRemainderPlanningSource`，与区域 `planningSource`、旧单机 `remainderPlanningSource` 三者互斥。实际编译前调用 [GroupPlanner](FleetMissionGroupRemainderPlanner.md) 的完整中央 provenance／task 校验，编译后再核全部成员、路线、profile、时长与共同 constraints；不是只检查第一机或拼接已编译单机结果。坏来源／来源混用／投影不匹配均增加真实 blocking 风险和 unmet，不删字段后冒充可行。

逐成员 `durationSeconds`／原时间限制继续走既有保留与校验；新增实际保留、非负有限校验 `minimumSpeedMps` 和 `returnReserveSeconds`。显式正速度门槛必须由该 ID 的唯一原 fleet 快照中自身 `maxSpeedMps` 满足：缺失、非数值、非有限、负值（含 Registry 未知 -1）或低于本成员门槛均 blocking。没有用其它飞机的速度补位，也不把最快新路线升成共同最低速度；这些描述证据不是实测性能认证。共享原最低速度及其它原政策仍由原普通审阅链消费。

源 `reservePlans` 保持原值，仍由真实备用集合 ID／replacement 目标／独立能量资料校验；新备用 ID 不对应或替补资料不完整不能静默重写，原 blocking 保留。每主机返程 floor 由 [Coordinator](FleetMissionReviewCoordinator.md) 真实加入自己的能量账，不移给其它成员。绝对／权威时序不降成估算，clearance 原作用 ID 不重写，未通过必须阻断。新计划不恢复 Run、控制权、占用、旧批准或正式完成；原 pending／批准／上传／启动门禁不放开。仅源码静态修改与对读，未编译、执行或测试；下方为历史。

## 473：显式历史余段来源沿用完整编译合同

新[余段规划资料包](FleetMissionRemainderArchive.md)通过[显式保存／打开入口](FleetMissionRemainderArchivePanel.md)保存单 Run、单成员完整原任务／计划／载荷／观察，不以复盘摘要或最新 Draft 补源。原实际 getter 的 native18／schema1 不变；文件完整校验后产生历史20／schema2／`historical-run-planning-snapshot`，不得重命名为 native 以走旧取源身份。

Compiler 已实际复用 [Planner 中央 validateProvenance／matchesTask／matchesPlan](FleetMissionRemainderPlanner.md)。因此 native provenance28／schema1 与 historical provenance30／schema2 进入同一原完整任务／新路线／profile／constraints 比较；历史两项资料包身份保留于新计划并参与既有输入／归档内容摘要。无需放宽 Compiler 外层计划 schema、混用区域 `planningSource`、删除坏来源或新增权限旁路；PlanStore／View 的实际消费者也使用同一中央校验。

共享 Planner 仍保原任务政策与数据、包含式所选原航点和完整停留，绝对时间／权威时序要求仍明确拒绝，不降成估算以制造可行。新目标机／起返点／速度／配置及当前能力、健康、能量、空域、净空等均须经新审阅；原 pending、审批、上传和启动门禁保持。观察不是作业完成，历史资料不是认证；批准恢复、执行许可、命令放行仍为 false，不恢复实际 Run、控制权、占用或正式结果。本批仅源码静态对读，未编译、执行或测试。下方旧批次保留为历史。

## 452：内部接地资料版本自然进入实际后台编译

`constraints.clearanceInput`现在可由原统一normalize接受内部schema2，外层区域编辑planningSource仍为2；不增加顶层任务字段、不重写旧schema1。完整车辆／支承尺寸和动作绑定contacts参与原task／planId及准确记录内容。原实际后台编译仍调用geometry-basis Review；input2生成[报告schema3](FleetMissionClearanceReview.md)，包括每段protected/support两组件与有限接触模型，而非未使用helper。

缺／错contact保留不完整报告及blocking风险，不删除段或仅做解析通过。正确声明模型也不删除原`clearance_geometry_review_pending`：配套PX4 LAND实际下降不以p7作为硬停止点，实际构型／datum／资料时效、同次原生启动报告及最终write门禁仍需继续实现。原4MiB资料、24MiB Review、2MiB报告和共享2M工作预算不因双组件而扩大；取消整份丢弃。未编译、运行或测试。

## 451：纯值可取消编译入口

新增非 QML、`public static` 的 `compileCancelable(task,fleet,cancellation)`，第三参数是 [Geometry::CancellationToken](FleetMissionClearanceGeometry.md)。它复用原完整编译算法，使用调用者冻结的任务／舰队值；不读取或写入 Compiler 实例的 `_lastPlan`，不发 `planChanged`，也不访问真实 Vehicle、账号或 GUI 对象。原规划源 validator 的线程内临时 Draft 仍只用于相同纯值构建规则，不借用现有编辑器实例。旧 `compile(task,fleet)` 改成空 token 包装：获得完整结果后仍按原方式更新 `_lastPlan` 并发原信号，保持同步调用兼容。

取消只返回固定2键 `{cancelled:true,reason:"mission-compile-cancelled"}`，不是普通成功或 blocked plan，也没有 plan／报告／成员前缀；不得进入归档或审阅发布。入口、原规划源／profile／路线处理阶段、成员／备用循环、覆盖生成前后、身份摘要前后、净空资料检查之后、Joint源提取前后以及最终返回前均检查同一 token。净空实际求解调用 `ClearanceReview::buildForGeometryBasisCancelable`，将共享取消信号传至每段与 Geometry work／并集循环；该层取消也转换成同一编译取消标记。

取消是协作式而非强杀线程。Joint完整源提取（原16 MiB预算）、区域生成、单次原规则规范化和 Qt JSON／hash 等仍在子调用前后检查，未全部变成细粒度可中断循环；不能承诺瞬时或固定毫秒时延。Worker须持有 token 的共享生命周期，拥有者只false→true，不复用。上层必须等实际线程 finished 再释放任务占用，并在发布前复核 request、原冻结来源和 token；本静态函数返回之后发生的取消仍由发布者拒绝。

本批不改变原净空pending、审批、上传／启动门禁或 report schema。仅源码和静态检查，未运行编译、测试、helper、GUI或设备；后台调度和界面状态由对应调用模块实现，不由此静态函数暗中创建线程。

## 450：实际冻结完整规划环境结果，尚不解除总体pending

对完整合法且主机同序的clearanceInput，在原plan所有非释放字段成形后调用[Joint::buildPlanning(plan,{})](FleetMissionJointSpatialReview.md)，不构造startupView、不取实时位置；再实际调用[ClearanceReview::buildForGeometryBasis](FleetMissionClearanceReview.md)，冻结完整result3于新增可选根字段planningClearanceReview。整个原plan只做同一完整编码来源投影，不能拆成功成员或回退当前草稿。

来源／计算构造失败增加clearance_planning_review_unavailable；报告已构造但声明模型不满足（含范围、比较或接地缺口）增加clearance_planning_model_incomplete_or_violated。完整结果、风险、unmet／feasible／reviewStatus一起提交并通知；旧成功计划不会代替新失败。原pending也保留，因为同次真实启动／接地语义、实际构型／高程对应和最后write尚未接齐，不能因规划正常就开放启动。

新结果随plan进入contentHash，但规划geometryBasis不hash含派生报告的整个plan，避免自引用。旧无clearanceInput任务不新增报告；坏资料保持原拒绝。旧noFlyZones／真实Coordinator空域门槛仍独立，空reviewEvidence不是空域已通过。未编译、测试、运行或改变PX4／权限；下方448及更早为历史。

450时compile仍在Coordinator GUI栈同步调用，异步／取消尚未实现。451已补上静态可取消入口及单任务Worker，实际带clearanceInput的Root入口改走后台编译，再在GUI核来源并显式完成原生审阅；直接同步Coordinator入口明确拒绝该重型分支。原compile本身仍同步，complete中的其他原计算也未全部迁移。没有processEvents或将Vehicle采集搬入worker，原pending继续阻断，运行性能尚未验证。

## 448：工程净空资料绑定，几何审查尚未完成

可选资料仅从 `task.constraints.clearanceInput` 进入完整输出 `plan.constraints`，不增加计划根字段。成员、备用机和约束构造完毕后调用 [FleetMissionClearanceInput](FleetMissionClearanceInput.md) 的 `inspectForPlan()`，核完整声明形状及全部活动 `members` 的 ID 和顺序；备用机不会冒充活动成员。无此字段的旧任务不增加净空门槛，不补默认资料。

显式 `constraints` 为非 map 或 null 时保留原坏值，添加 `invalid_constraints` 的 unmet requirement 与 blocking risk；不能通过 `toMap()` 把未知来源当成空约束。存在但不合法的净空声明保留原值，并把 helper 的具体失败原因（含成员身份不匹配；无原因时为 `clearance_input_invalid`）加入同一阻断链，不归一化覆盖、不丢坏项后接受前缀。余段换机仍保留原 envelope，由实际新成员匹配失败明确阻断，不把旧机体资料自动冒配给新飞机。

**即使资料完整且 ID 匹配，本批也产生 `clearance_geometry_review_pending` blocking 风险和 `feasible=false`。** 当前只有输入接线，没有对完整任务、启动或终结动作执行障碍／地形／走廊几何求解；不能让新声明被忽略后自动批准。后续真正的净空求解应替换该待审阻断，不能只删原因或改变标签。失败仍沿正常完整编译结果更新 `lastPlan`、发出 `planChanged`，不保留旧成功计划冒充本次结果。

原 task 中的完整声明参与既有 `planId`，原输出再参与归档 `contentHash`；哈希不认证来源、时效、实测机体、实时环境或避障能力。区域源无资料仍为 schema 1／11 settings；有资料的完整源为 schema 2／12 settings，外层仍精确五字段，由同一 Draft validator 重建含该约束的 task。旧档不改写，恢复仍是新草稿和新审阅，不恢复批准。

坏净空计划会被 PlanStore 拒绝整份记录；合法但几何待审的 blocked 结果可以归档。这里没有修改 PX4、上传／启动权限，也没有编译、测试、执行 helper、解析器、应用或设备。

## 433：独立余段提案来源与实际输出一致性

当前 `compilerVersion=0.6.0`；下方432的0.5.0为历史。新的可选 `remainderPlanningSource` 与区域编辑源 `planningSource` 互斥：前者表示从实际Run人工选定原航点后缀的新单成员提案，不能反向恢复原区域或继续旧执行身份。

入口共用 [RemainderPlanner](FleetMissionRemainderPlanner.md) 的规范来源与 `matchesTask()`：核完整派生任务摘要，排除原审阅链合法注入的 fleet/intent/taskId/templateValidation 以及来源字段自身；模板选择和其他安全政策仍在摘要内，当前模板验证由正常审阅重新生成。来源不合法、政策被改或混合两种源产生 `invalid_remainder_planning_source` 阻断，不输出可用余段来源。

完成原有成员/能力/备用/约束编译后，`matchesPlan()` 再核唯一目标、实际规范路线、停留、完整profile、估算和constraints，以及members/assignments一致、无备用。失败产生 `remainder_plan_projection_mismatch`，不能把改过的路线当作原确认提案。合法来源随plan保存，并由原task参与planId、由完整plan参与归档contentHash。

这是规划一致性，不是原Run认证、连续完成证明、机载任务接管或新执行许可。原风险、当前能力、能量、模板和人工批准路径仍适用；既有无此字段的任务保持原合同。未编译、测试或运行。

## 432：实际备用待命与指定目标接替能量方案

Compiler 版本为 **0.5.0**。任务可提供独立 `reservePlans` 列表，至多 255 项；每项严格为 `{vehicleId, start, returnTo, reservePlan}`，ID 必须唯一并属于实际 `reserveVehicleIds`，不能属于活动 assignments，也不接受额外字段。容器、条目形状或身份映射损坏时整份描述列表拒绝，不保留合法前缀。缺某个备用描述仍生成该机的明确缺失诊断，不推断默认策略。

在最终主任务 geometry 已编译后，对每个备用机调用 [FleetReserveEnergyPlan](FleetReserveEnergyPlan.md) 的纯值 `build(reserveDescriptor, members, constraints, reserveVehicle)`，结果作为 `plan.reserveMembers[].energyPlan` 保存。`reserveVehicle` 仅来自这次输入 fleet 的唯一原 ID 记录，不另取实时资料。每机 `energyPlan` 是固定 29 字段，含 `complete/reason`、规范策略、待命折算、进场/目标移动/目标停留/独立返程预算、原目标路线哈希和有界出航/返程几何。完整性不是飞行可行性认证；全部路线净空、性能、名义负载假设和执行放行标志仍为 false。

对非 coverage 旧式共享路线任务，沿用 Compiler 与审阅器既有规划语义：仅当成员**没有** `waypoints` 键且全局路线明确提供并规范化有效时，在 `reserveBudgetMembers` 私有值副本中使用该全局路线。显式空/坏成员路线不回退，coverage 冲突不借全局路径修复；规范点没有明确 AMSL 时不补猜基准，仍由 helper 报不完整。该副本仅供备用预算，输出 `members/assignments/waypoints` 及实际上传保持不变。PayloadBuilder 仍要求逐成员显式完整出航/返程，不能因预算能计算就称旧式共享任务已可上传。

没有策略、缺端点、目标不在实际非备用成员、目标路线不完整、速度/高度等输入不支持时，保留 helper 的具体 `reason`，并加入 `reserve_energy_plan_incomplete` blocking 风险。旧草稿仍可读取并重新编辑，但编译时不再把缺失的备用待命/接替方案伪装为已完成。缺失结果不会通过最长主机预算取得完整性。

每条实际生成路径最多 **4096 点**，全组备用出航加返程累计最多 **32768 点**。任一路径/总量超限时，所有备用 `energyPlan` 统一变为 `complete=false`、空路径、未知预算，并给出 `reserve_generated_paths_budget_exceeded`；此前成功的前缀也不会继续当作完整组方案。输入或超限拒绝不增加实际上传、启动或自动接替行为。

备用机始终只存在于 `reserveVehicleIds/reserveMembers`，不加入 `members/assignments`，不参加覆盖区域分配，也不交给实际任务 PayloadBuilder 上传。完整 `reserveMembers` 加入 planId 身份输入，原任务中的策略/端点和输出计划中的预算/路线随现有 contentHash 保存；改变任一政策/目标/路线需要新审阅，而非沿用旧批准。

规划源仍是外层 exact5/schema1：Compiler 核对八个旧规划字段，以及存在时的第九个 `reservePlans`。无新策略的旧源不补默认字段、不改写归档，不改变 PlanStore 原 canonical JSON 比较。历史恢复仍只得到新草稿，不恢复当前机载任务或权限。旧程序可能不识别新可选成员字段，应交付整套 producer/consumer 源码。本批没有修改 PlanStore 保存格式或运行任何编译、测试、helper、解析器、应用、仿真或设备操作。

## 427：完整区域草稿输入随计划保留

本节历史版本为0.4.0，当前版本与可选备用策略扩展见432。由[Draft](FleetMissionDraft.md)产生的任务可以携带有界`planningSource`：schemaVersion、sourceRevision、完整vertices/members/settings。编译前用同一原生校验器重建规划字段，逐字段核对实际task；数值接受JSON原生数字宽度变化，不接受字符串/布尔转换。原区域、备用成员中保留的profile、未启用端点/路线和heading=360的编辑值不会从生成航线猜测还原。

带此源的任务只接受校验器重建的原八字段、存在时的432 `reservePlans`，以及planningSource、fleet、intent、taskId；额外任务政策、字段缺失或内容不一致产生`invalid_planning_source`阻断，并不输出可恢复源。未携带源的一般任务继续使用原广义Compiler合同，不因此被限制为区域编辑器。源进入原task的planId哈希输入，规范完整源另进入输出plan及存档contentHash。该来源不是签名、历史机载内容或批准。

完整源供[失败部署修复草稿](FleetMissionRepairDraft.md)显式复制为新编辑副本。重新编辑/生成/审阅不继承旧fleet、健康、批准或上传收据；新副本还要求重新声明地图WGS84/AMSL基准。旧计划没有源时不补造。源码已修改，未编译、测试或执行恢复流程。

## 420：最终成员与备用机的完整基线

最终成员和备用机的 requireConfigurationBaseline 改用 [共享完整基线校验](FleetConfigurationBaselineEvidence.md)，而非 configurationState 标签。手工分配和覆盖规划均在最终成员确定后核同一输入 fleet 记录；保留 configuration_not_approved 错误并明确旧哈希标签/导入批准不足。

不重选成员、不改变计划顶层 schema 或上传/启动许可；原政策默认 false 不变。下方386–390“要求原标签=baseline”仅是历史口径，由本节替代。未编译、测试或运行。

`FleetMissionCompiler` 是目标 6 的地面站 groundwork。它把已经由上游意图任务、能力匹配或人工角色分配得到的输入整理成一份可审阅的 mission plan，供 QML 展示、风险审查和后续工程师接入使用。

## 386–390：最终成员能力政策收口

显式 `assignments` 不再依赖上游是否请求角色分配。编译器在覆盖路线生成完成、最终 members 和 reserves 已确定后，用输入 fleet 的原始唯一车辆 ID 行复核政策；重复/缺失 ID 不能选取某一份正向记录。成员和路线保持原样，不为满足政策重新选机或丢弃不合格成员，失败进入 `unmetRequirements` 与 blocking risk。

任务 `requireVerifiedCapabilities`、`requireConfigurationBaseline` 必须是非 null 的真实 bool，缺失时仍默认 false。`requiredCapabilities`、兼容 `capabilities` 及四个 `leader/scout/relay/responderCapabilities` 均独立经 [FleetCapabilityEvidence](FleetCapabilityEvidence.md) 严格规范化：拒绝 null、坏类型/元素或超限，不把坏值降为无要求。保留原优先级：`requiredCapabilities` 存在则为权威，包括显式空列表；否则才用 `capabilities`，不静默并集。

每个最终成员须满足全局及其实际 role 的要求；备用机只核全局/verified/baseline，不宣称角色等价。verified-only 即使要求为空也必须有 helper 当前的构型绑定本地工程审阅，不能用裸 verified 字符串或协议/组件元数据替代。普通模式可使用原 observed/runtime 与当前有效工程声明的并集；`requireConfigurationBaseline=true` 仍独立要求原快照 `configurationState=baseline`。

对应错误包括 `invalid_capability_policy_boolean`、`invalid_task_capability_requirements`、`capability_review_not_current`、`configuration_not_approved`、`required_capability_missing` 和 `invalid_vehicle_capability_evidence`。空 fleet 仍是 planning-only，但实际成员若明确要求上述证据，则 `capability_policy_fleet_evidence_missing` 阻断，不能称已满足要求。

这些检查是值快照一致性和任务要求核对，不认证 Registry 来源、真实硬件、实测性能或账号，不产生 StartPermit、上传或新命令。真实模型、角色等价替补和原25目标/六类仍有实质开发；本批仅源码读回与限定文本检查，未编译、测试、运行 helper/认证/应用或设备。

## 输入

`compile(task, fleet)` 接受 `QVariantMap` 任务和可选的归一化机队快照：

- `area` / `target`：区域或目标摘要；
- `waypoints` 或 `route.waypoints`：包含 `lat/lon/alt` 或 `x/y/z` 的航点；也可只提供每个成员的完整路线；
- `coverage`：显式请求只读区域覆盖提案，字段、角色和几何边界见 [`FleetAreaCoveragePlanner.md`](FleetAreaCoveragePlanner.md)；不得同时提供全局路线或覆盖成员已有路线；
- `timeLimitSeconds`、`deadline`、`minimumSeparationMeters`、`maxAltitudeMeters`、`geofenceRequired` 等限制；也可以放在 `constraints` 子对象；
- `assignments`（别名 `vehicleAssignments` / `members`）：包含 `vehicleId`、`role` 和可选成员航点；
- `executionProfile`：可选完整任务默认策略；成员同名字段整份覆盖，严格契约见 [`FleetMissionExecutionProfile.md`](FleetMissionExecutionProfile.md)；
- `reserveVehicleIds`（别名 `reserve`）和 `reserveCount`：备用成员及最低备用数量。

输入门禁：车辆 ID 必须是与 `vehicleId/systemId/id` 别名一致的真实 1--255 整数（字符串、`0`、负数、分数、NaN/Inf
会被拒绝）；assignment role 必须是 `leader/scout/relay/responder` 的真实字符串，最多一个 Leader；航点的经纬度必须有限且分别位于 `[-90,90]`、`[-180,180]`，
高度和航向如提供也必须是有限数。空对象/空数组 `target` 不满足目标要求。
`deadline` 只接受正有限数或有效 ISO-8601 日期时间；`reserveCount` 必须是
非负整数；`constraints.geofenceRequired`（以及从任务顶层复制到 constraints
的同名字段）必须是实际 `bool`，不会接受字符串或数值的隐式转换。布尔类型错误
会产生阻断风险 `invalid_geofence_required` 并进入 `unmetRequirements`，不会被
伪装成可执行方案。上述门禁失败均会保留在审计字段中，不会被伪装成可执行方案。

当 `fleet` 非空时，它被视为闭合机队快照：每个 fleet 条目必须有合法且唯一的 ID，assignment/reserve 必须存在于该快照，否则产生 blocking unmet/risk；`fleet` 为空才是明确的 planning-only 模式。模块不会登记、修改或查询 Vehicle。

`geofenceRequired=true` 需要真实的非空三点边界/多边形，或有效的 bounds。`west/east/south/north` 按经纬度解析，必须有限、在经纬度范围内且 `west<east`、`south<north`；`minX/maxX/minY/maxY` 按调用方声明的本地米制坐标解析，必须有限且正向有序。为避免坐标系和轴顺序歧义，四数字 bounds 列表不作为有效证据。只提供字段、空 Map、null、字符串或包含无效顶点的列表都会产生 `geofence_missing`。几何仍是地面站入口校验，不替代专业 GIS/空域服务。

## 输出与边界

输出包含 `planId`、`schemaVersion`/`planVersion`、区域摘要、规范化航点、成员角色、备用成员、`fleetProvided`/`planningOnly`、约束、`risks`、`unmetRequirements` 和 `feasible`。任何缺失区域/目标、航点、成员、角色、时间限制或备用要求都会留下可解释的风险/未满足项；不完整方案不会被伪装成可执行。

所有输出固定带有：

- `manualConfirmationRequired=true`；
- `requiresOperatorReview=true`；
- `flightCommandReleased=false`；
- `dispatch=not-released`。

该类不持有 `Vehicle`、`MAVLink` 或 Mission upload 接口，不发送命令。区域覆盖只生成有界的几何提案；本批新增的显式能力政策检查也不替代空域冲突检查、障碍/地形/动力学路线优化、实时原生权限/来源门禁、PX4 安全逻辑和工程师验证。

2026-10-13 增量：保留并严格核对航点时间别名、坐标/高度基准、成员返航路线、成员起止时间和时长。任一损坏航点使该路线整体阻断，不将合法前缀当成完整路线；匀速生成的 `estimatedElapsedSeconds` 不转换为权威时间。仅有完整逐机路线时不再因缺少全局路线而错误阻断。

覆盖任务比较 `maxAltitudeMeters` 时需要相同的显式 `maxAltitudeReference`，缺失或不同基准阻断，不能将 Home 相对高度与 AMSL 数字直接比较。生成成员的基准声明不得与 coverage 选项冲突。

端点模式的高度上限覆盖完整进场、支援、巡测及返回点；任一缺高/非有限/基准不一致或超限点均阻断。独立进场/巡测/返回长度与时间估算保留到审阅，不能当作实际时间证据。编译计划及审阅身份可经 [`FleetMissionPlanStore`](FleetMissionPlanStore.md) 进入本地不可变版本档案，存档不恢复批准或机载任务。

## 显式执行策略与驻留（compilerVersion 0.3.0）

`task.executionProfile` 经纯静态规范化后作为完整默认，保存在计划顶层及身份内容中。每个成员保存最终解析出的完整 `executionProfile`：成员提供同字段时整份替换，不逐字段合并；明确 null/空对象/未知字段/错误类型的成员覆盖会阻断，不能退回任务默认。显式非法任务默认也会阻断，即使成员各自有有效覆盖。完全没有 profile 时维持原几何提案，不推断起飞、降落、返航速度或垂直参数。

覆盖规划器复制完整成员 Map，保留解析后的 profile；端点一致性检查在生成进场/巡测/返回后进行，不拿未生成的中间草图判断。辅助角色不因 coveragePlan 的巡航/爬升估算自动继承覆盖速度要求；只有显式任务默认或完整成员策略才是其执行意图。

仅对实际覆盖参与者核对两套已声明要求：profile 巡航速度与 coveragePlan 巡航速度必须一致；双方明确提供的爬升速度也必须一致，否则 `execution_coverage_speed_conflict` 阻断，而不是先显示可行、到上传才失败。不为辅助机套用该比较，不为缺失的垂直策略字段填入覆盖估算速度。

`launchAction=takeoff` 的规范 WGS84/AMSL 点必须精确等于完整出航第一点，`endAction=hold` 必须精确等于返航最后一点，包含高度及明确基准。`endAction=land` 要求与返航最后进近点经纬度精确相同，但显式降落地面 AMSL 高度必须严格低于该进近点，不能为了坐标三维一致把前一 WAYPOINT 直接放在地面。缺路线、缺基准、坐标不一致或降落没有明确下降均进入 `execution_profile_endpoint_mismatch`，不补连接、不推断地面、不换算高度。动作坐标与 LAND 进近点同时受有效 `maxAltitudeMeters`、`maxAltitudeReference=amsl` 约束；缺失/范围错误/不同基准或任一点超限均阻断。这里一致性与高度比较只是静态输入约束，不证明进近/降落净空、真实 ground datum、land_detected 或起降安全。

航点可明确提供真实有限 `holdSeconds`，范围 0–86400 秒；提供的零也保留到规范路线、成员和 planId。不从 `timeSeconds`、ETA、成员 duration 或 `estimatedElapsedSeconds` 推导驻留。损坏的驻留值使整条显式路线无效，不保留一个合法前缀。

`none` 起飞策略、`rtl` 终止策略和 `absolute-required` 时序仍保留，另有 `execution_profile_requires_adapter` 风险提示，而不是悄悄改成 takeoff、hold 或相对驻留。规范化/可审阅 `feasible` 不等于编码完成或可执行；所有输出仍人工审阅、未放行，真实固件/实际参数快照、权限、机载回读、实时健康与启动许可由独立适配链核查。本编译器没有读取/改写垂直速度参数或调用任何上传/启动。

工程师后续核查（本轮未执行）：实际编译；任务默认与完整成员覆盖/非法覆盖/无策略；生成覆盖成员策略不丢失；takeoff/HOLD 与首末点三维精确一致；LAND 同经纬度且显式地面高度严格低于进近、两点高度上限及真实降落状态独立核查；holdSeconds 的真实类型/边界/指纹变化；合法但未适配的绝对时序/RTL/none 不产生假执行许可。

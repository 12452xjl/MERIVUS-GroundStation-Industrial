# 启动异机完整声明包络比较（535）

本模块把已经生成的完整 Startup Domain 与 NAV／LAND／TAKEOFF 参考包络用于异机之间的实际整数几何计算，不是名义中心线距离的别名或新授权服务。重叠是声明外包相交候选，不是已观察物理碰撞、真实飞行轨迹、可靠同步时序或 StartPermit。

## 原生生产者与完整性

[StartupDomainReview](FleetMissionStartupDomainReview.md) 与 [WaypointSwitchReview](FleetMissionWaypointSwitchReview.md) 的 `build` 在原取消参数之后增加可选 `PreparedPtr*` 输出；原调用者不传时行为和公开 result3 不变。只有本次 `prepare` 的完整结果才能封装：原逐机、逐 split-cell／reference、protected／support 身份与 `Polytope.bounds`、全部 extra planes（含 strict 属性）原样保留。不从 public report 构造，不按相同显示身份去重，也不裁掉后成员。

`Prepared` 构造、内容均私有，只有两个生产者及本比较类有访问权；输出是 `shared_ptr<const Prepared>`，不可公开改写、复制构造或导入 caller map。它保原 geometryBasis、完整规范 clearanceInput 指纹、各自 models 指纹、同序 memberIds，并以私有共同 Motion-row2 指纹核对两个来源使用相同 Motion 描述。Waypoint 完整 row3 指纹仍包含原 parameterSnapshot，没有剥掉参数绑定。

合法但完整几何不支持时，生产者可给带完整来源元数据的 incomplete 封装，该来源没有任何 components 前缀。环境 Unknown／违规不把已完整生成的 geometry 变成不完整，原报告独立记录环境结论。非法来源、取消或失败构建不输出封装。

两生产者 `resultMatches` 的可选输出只封装本次值复核重建的几何，标为 matcher-only；实际 `InterVehicleEnvelopeReview::build` 拒绝此封装。它只能供新 `resultMatches` 核来源／计数／明细身份，不能变成 actual build 或恢复原 native Models、设备依据及审批。

## 算法与边界

先完整核对来源类型、basis／input／共同 Motion 指纹、成员顺序／唯一性、每个组件身份、全部坐标及每个平面。然后按原 member 顺序选不同 vehicle 的两组，再对每组 Domain＋Waypoint 完整组件做笛卡尔积，覆盖 DD、DW、WD、WW 及全部 protected／support 组合；同机独立假设不互报异机冲突。

每 pair 先取两个完整 Bounds 的交集，只有严格分离才直接判不相交；相等边界、点、线及零厚度轴不删。Bounds 重叠不直接判冲突：保两方所有真实平面后调用现 [ClearanceGeometry::feasible](FleetMissionClearanceGeometry.md)。仅在 Bounds 不能排除、需要窄相交时，额外面合计超过现 solver 的64面支持域才为 Unknown，不裁面当 AABB；完整 Bounds 已严格分离不需要组合这些平面。当前生产者的闭包络在面／边／点接触时计相交候选；原平面 strict 属性不被暗改。

不使用 startTime、估算速度或不同任务时间消除空间相交；没有可靠同步时序就作全空间保守比较。地形接触 pad、allowedTerrainCellIds、contactVolume 不导出，不能豁免另一架飞机。原各机构型、tracking、scene／projection／datum／safety 膨胀都保留；两边已有误差不相减，也不作相关性／概率推断。

旧 `minimumSeparationMeters` 与 [原空域检查](FleetAirspaceConflictChecker.md) 继续独立消费；新模块只比较已扩张外包是否相交，不替代原最小间距或声称额外净距已满足。

## API／结果

`build(domainPtr, waypointPtr, cancellation)` 返回 exact3 `{ok,error,report}`。来源／完整几何校验失败或取消时 ok=false、固定短 error、空 report，没有成功前缀。成功构造的报告也可能 incomplete、Unknown 或有冲突，不等于模型通过。

report exact28：

`schemaVersion, reportType, geometryBasis, clearanceInputFingerprint, domainModelsFingerprint, waypointModelsFingerprint, memberIds, memberCount, domainComponentCount, waypointComponentCount, componentCount, geometryComplete, pairCount, processedPairCount, comparisonComplete, declaredModelSatisfied, conflictCount, conflicts, conflictsTruncated, reasonCodes, workUnits, limits, snapshotOnly, timingKnown, actualPathCertified, clearanceCertified, executionPermit, flightCommandReleased`。

schemaVersion=1、reportType=`fleet-inter-vehicle-envelope-review`；三来源指纹和 geometryBasis 均为原64字符小写十六进制 SHA-256 字符串。memberIds 保完整顺序。各来源 componentCount 是封装中实际完整数组大小，合计为 componentCount；某一来源 incomplete 时其大小为0，另一来源仍可保完整计数，但 geometryComplete=false、pairCount=0、processed／conflict=0、comparisonComplete=false，不能称未知全组“零 pair 已通过”。

都完整时，pairCount 用 qint64 累计各异机组件数量的乘积（先转 qint64 再乘），不是 detail 数量；processedPairCount 只计明确 Verdict 的完整 pair。Unknown 当前 pair 不计 processed，早前完整候选仅作 incomplete 诊断。完成时 processed=pairCount；单机0 pair 也须全部原来源／几何先核完。公开值沿原 PlanningWorker 标准 JSON snapshot，整数元型可为 Double，仍须是精确非负整数。

conflicts 最多512行，每行 exact10 `{vehicleA,sourceA,componentIndexA,componentKindA,referenceA,vehicleB,sourceB,componentIndexB,componentKindB,referenceB}`。source 为 domain／waypoint，index 是该来源实际完整数组位置，kind 为 protected／support。domain reference exact2 `{segmentIndex,endpointIndex}`；waypoint reference exact3 `{fromSequence,toSequence,regionKind}`，保原 NAV、TAKEOFF／LAND 参考身份。A／B 按原成员顺序，细节按真实 pair 顺序；相同显示身份的 split cells 仍有独立 index。

`declaredModelSatisfied = geometryComplete && comparisonComplete && conflictCount==0`。conflictCount 保实际明确相交数，明细满仍继续算／计数；conflictsTruncated 只表示超过512条。snapshotOnly 恒 true；timingKnown、actualPathCertified、clearanceCertified、executionPermit、flightCommandReleased 恒 false。

`resultMatches(domainPtr,waypointPtr,result)` 不执行 feasible、SAT、覆盖或其它求解器。只复核完整来源／几何、计数、28字段／limits／原因码／flags、10字段原组件身份、唯一递增 pair 序及已处理前缀，不能认证报告的几何结论。work 不低于完整源验证及每个已处理 pair 的固定收费；本版 feasible 不消耗 fragments，resource reason 必须对应 work=2000000，不能借它表示未耗预算的平面／算术 Unknown。上层仍保真实 worker 和原 native 来源。

## 预算与实际消费

最多32成员，两来源各65536组件，合计131072。新比较共用一份2000000 work／2048 fragments Budget 和原取消 token，完整源／plane 校验、复制、pair 枚举、Bounds 判断和 feasible 全计同一份，不按 pair 或来源重置。旧两个环境计算仍各沿原预算，不重跑或放宽。limits exact5 为 maximumComponents=131072、maximumWorkUnits=2000000、maximumFragments=2048、maximumConflictDetails=512、maximumOutputBytes=2097152。

完整笛卡尔积不承诺最大容量必能算完；Bounds 快速分离仍收费，大组可耗尽而 Unknown，不能只算近邻／前缀后报 clear。源全量校验本身超预算会失败空 report；比较期耗尽则保完整来源／计数、comparisonComplete=false及早前完整候选。预算不是峰值内存、CPU时间或实际物理实时保证。

[StartupClearanceWorker](FleetMissionStartupClearanceWorker.md) 接同次两个封装，新结论进入原 StartReview／Witness／Authority 共同准入。不能解除 Domain 原 actualPathCertified=true 要求；当前该值仍 false，Compiler pending、动态接受／平滑／采样间真实路径及完整 StartPermit 未完成。

仅源码／合同和直接静读；未执行模块、算法、解析器、helper、hash实现、QML、UI预览、SQL、仿真、下载或真实设备；未编译、测试、修改 PX4、刷机、提交或打包。工程场景均未执行。

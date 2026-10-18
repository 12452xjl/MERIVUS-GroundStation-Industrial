# FleetMissionClearanceReview（450／保留449接口）

## 467：整域审阅复用原完整几何来源校验

新增纯值 `geometrySourceValid(clearanceInput, jointView, normalizedInput, cancellation)`，先冻结输入再清输出，复用原24 MiB整树／16 MiB Joint、完整资料normalize和原geometry-basis Joint同序成员／载荷／全部段来源校验，不运行环境solver、不复制第二套Joint解释器。成功输出完整规范资料；失败或观察取消时输出为空，未知／预算／取消不返回可用前缀。

该入口供新增理想球面整域环境计算复用，只校值和范围来源，不认证原生来源或当前Qt数值谓词包含。原build、schema1／2／3报告和geometryResultMatches语义不变；整域报告独立留在启动Worker新result6，不向旧名义报告塞未知字段。

仅源码及限定静态阅读，未执行此入口、normalize、solver、parser、编译或测试。整个原数值域与最终StartPermit、Compiler pending保持未完成，不以本接口替代实际来源或许可。

## 455：共享冻结几何来源的报告匹配

新增 native-only `geometryResultMatches(clearanceInput, frozenJointView, result)`，第三参仍为完整固定3键 `{ok,error,report}`。只接受成功的input1/report2或input2/report3组合；原失败诊断不借此函数变成认可结果。完整资料规范指纹、全部有序成员、planId、geometryBasis和完整Joint指纹必须对应原冻结来源。

共享校验支持且严格区分 `encoded-planning/startupIncluded=false` 与 `startup-and-encoded-planning/startupIncluded=true`；报告范围、标志必须与Joint一致，`actualStartupReviewed`在两者中仍必须为false。保留原完整schema、真实类型、固定非许可标志、计数／处理进度、checks、limits、512条明细和全部违反引用检查。input2继续重建纯ContactModel组件身份及完整applications/issues，不能借新入口省略接地来源或放宽终点规则。

资料与Joint合计24 MiB／20层／1000000节点，Joint独立16 MiB、结果独立2 MiB，沿原完整输入预算；不截前缀。此处只规范化、校验来源／表示并构造接地组件，不运行环境覆盖／求交solver，不重算空间checker，也不采集Vehicle或认证报告数学结论。真正计算结果必须另由实际owner绑定工作线程、原请求、来源、时效和取消状态；匹配成功不等于模型成立、启动许可或最后发送放行。

`planningResultMatches(plan,result)`保留原规划专用合同：先 `inspectForPlan` 核完整资料与活动成员，按原空reviewEvidence重建 `Joint::buildPlanning(plan,{})`，只接受规划范围，再委托共享匹配。旧PlanStore／Compiler调用不需要改接口，不会因此接受启动范围报告替代规划报告。派生报告排除仍只作用于本地来源副本，原计划／归档事实不改。

仅源码实现和静态对读；未编译、运行helper、测试或验证设备。动态来源、完整审批／Authority与最终write责任仍在实际调用链，不能以本节纯值匹配声称已完成完整净空许可。

## 452：显式接地声明进入完整规划计算与归档匹配

输入schema1保持原单机体包线、报告1／2及缺接地阻断，不补起落架尺寸。输入[schema2](FleetMissionClearanceInput.md)只进入geometry-basis入口，以[ContactModel](FleetMissionContactModel.md)从同一完整Joint编码／原启动假设构建每段两个独立扫掠体：protected机体／旋翼与support起落架。公共坐标／凸面算法移至ContactModel，旧路径仍调用同一实现，不另设舍入规则。

只有正确绑定的LAND声明能在**计算副本**把参考终点改为独立声明的referencePoint.groundAmslMm加显式supportBelowReferenceMm，得到精确整数静止参考高度；原canonical p7高度完整区间仅核wireGroundToleranceMm一致性，不用编码舍入或较大容差使模型提前在支承面上方结束。原Joint/wire/sequence/payloadHash不改变，原接近点必须严格高于这个声明终点。它不是PX4 LAND任务的位置目标／硬下降下限，本地配套多旋翼PX4实际使用下降速度并依赖landed完成。原pending保留，不把声明表面当真实制动／触地证明。

真实起飞vertical.from不重定位，需核原TAKEOFF seq／payload与声明平面加支承偏移的显式容差；同一from的current-first-waypoint独立假设也检查相同接地上下文，不能删除该段或给launch none自动接地许可。规划范围明确排除startup，合法takeoff声明只核源不冒充实际起飞检查。

两个组件都完整检查coverage、走廊、地形覆盖和所有障碍。protected始终检查全部地形，绝不接触豁免。support只有属于该动作、该terrainCellId且其**整个support∩terrain交集**被pad凸域及ground-depth至ground有限闭高度带完整包含，才允许该地形接触；仍调用精确coveredByUnion，并消耗同一预算，Unknown不能通过。障碍、其它cell和有限带外均不豁免，不截短末尾／抬高整条路径或只查端点。

新版报告schema3为原scope报告29字段加5字段，exact34：`contactModelVersion=1,componentCount,processedComponentCount,contactApplications,contactIssues`。segmentCount仍为原wire／假设段数；componentCount严格两倍，checks原10项按组件计，processedSegmentCount为完整处理组件数整除2。limits增`maximumComponents=16640`成为exact9，原2M工作／2048分片／512明细／2MiB输出不扩大；缺上下文与source mismatch保留完整contactIssues，零段成员以空segmentId明确无定位，不捏造几何。

环境violation详情exact7，原5字段加componentId/componentKind；接地上下文诊断的这两项为空，不冒称某一物理组件。contactApplications固定exact6，contactIssues固定exact3，详见ContactModel。完整模型不满足仍`ok=true`表示报告构造成功，取消整份空report；所有certified／permit／flight字段仍false。

`planningResultMatches`同时支持input1/report2与input2/report3，严格版本对应，按同一Joint和ContactModel重建纯来源／声明参考组件，核完整applications/issues、组件身份、计数、限制及报告来源。它不重跑环境solver、不认证存档结论或真实触地；完整plan仍由Store最后contentHash保存，不能借报告改变自身basis。旧历史不自动升级。仅源码与限定静态审阅，未编译、运行、测试或放行；实际构型／datum／时效、同次启动及最终write仍待实现。

## 451：原生可取消计算，取消整份丢弃

新增非 QML 原生入口 `buildCancelable(input,jointView,cancellation)` 与 `buildForGeometryBasisCancelable(input,jointView,cancellation)`；token 类型为 [Geometry::CancellationToken](FleetMissionClearanceGeometry.md)，即共享 `const std::atomic_bool`。旧两入口继续使用空 token，共用同一计算算法，schema1／schema2和成功报告字段不变。

计算在纯 JSON 树遍历、完整成员／段来源检查、逐段毫米投影、各体积构造、接地缺口枚举、覆盖／走廊／地形／障碍比较循环、输出预算与 JSON 脱离前后检查取消。Geometry 使用同一 token，不创建新 token、不重置预算。观察到取消，或几何 Budget 已锁存 cancelled，统一返回固定3键 `{ok:false,error:"clearance-review-cancelled",report:{}}`：整份报告丢弃，不保留已证违反、计数或成功前缀，也不把取消包装成可归档的成功未知报告。

资料规范化、Qt JSON 单次编码／脱离、指纹计算等有界底层操作仍只有边界检查，不保证即时中断或固定取消时延。输入和输出的原资源上限不因后台或取消而扩大；取消后没有输出许可、没有保存、没有修改共享原生对象。上层发布者还必须在 worker actual finished 后核原请求、token及来源代次，避免最后一次检查之后发生取消而发布陈旧结果。非取消的几何 Unknown 仍沿旧合同保留有界已证诊断，不能与本节整份取消混同。

本批仅修改源码并静态读回，未编译、测试、运行 helper 或验证实际多线程时延。

## 450：规划范围、独立几何基准与来源一致性

新增 `buildForGeometryBasis(clearanceInput, jointView)`，仍返回严格 `{ok,error,report}`，但消费 Joint 新范围22字段。旧 `build` 继续只接受449原捕获审阅22字段并产生原schema1报告，不把 `geometryBasis` 冒充最终 `reviewFingerprint`。两者复用同一整数投影、扫掠包络及环境求交实现，不能以改变范围名绕过几何或接地检查。

- `scope="encoded-planning"`：仅原真实 Builder 完整编码所表示的规划几何。成员的 `startupAvailability="not-included"`、启动段数0、起飞序号配对false是明确未纳入，不作为本范围自身缺失；不生成当前位置、离地垂直段或虚构启动观察。规划完整性仍要求原成员几何、编码、执行配置及终结范围完整，RTL等缺口保留。原禁飞区／间距检查在 Coordinator 独立守门，不因本范围不要求它们而自动通过。
- `scope="startup-and-encoded-planning"`：包含提供的独立启动假设，保留原全范围要求；`vertical-takeoff` 和 `mission-terminal-land` 仍缺独立接地上下文，不能自动豁免地形或机体包络。
- 新联合视图的 `analysis` 固定 `{available:false,reason:"not-run-in-geometry-scope-extraction"}`，仅表示提取阶段没有执行原联合禁飞区／跨机比较，不伪造旧analysis21。完整旧联合审阅入口仍保持原行为。

新报告 `schemaVersion=2`，固定29字段：

`schemaVersion,reportType="fleet-declared-clearance-review",planId,scope,geometryBasis,startupIncluded,actualStartupReviewed=false,clearanceInputFingerprint,jointViewFingerprint,memberIds,segmentCount,processedSegmentCount,scopeCoverageComplete,comparisonComplete,declaredModelSatisfied,contactContextComplete,reasonCodes,checks,violationCount,violations,violationsTruncated,workUnits,limits,limitations,snapshotOnly=true,clearanceCertified=false,executionPermit=false,flightCommandReleased=false,actualPathCertified=false`。

即原schema1的28字段去掉 `reviewKind/reviewId/reviewFingerprint`，加入 `scope/geometryBasis/startupIncluded/actualStartupReviewed`。`startupIncluded` 只描述输入范围；两种范围都固定 `actualStartupReviewed=false`，纯值输入不是实际原生审阅／批准证据。`scopeCoverageComplete` 只能按对应scope理解：规划范围满足不代表已检查实际起飞。起飞请求锚点不是地面接触位置；原LAND地面AMSL也不自动等于带起落架的机体参考点。接地声明模型尚未实现，本批没有默许接触部件、地面豁免或输入版本升级。

新增native-only `planningResultMatches(plan,result)`，检查成功schema2规划报告与当前同一份计划的**内容来源一致性**。它完整规范化 `constraints.clearanceInput`，核原有序活动成员、input指纹、planId、新geometryBasis以及完整联合视图指纹；重建固定 `Joint::buildPlanning(plan,{})` 的编码／几何投影，不调用环境solver，也不执行原联合几何checker。首次生成规划报告必须同样使用空reviewEvidence，不能将额外归档证据混入首次投影后又按空证据匹配。

该检查也校验报告29字段及固定false、checks10／limits8、处理数、contact数量、违反计数与有界明细的结构一致性。违反code限定为原计算实际六种输出，collection必须对应；并集／缺接地上下文的volumeId为空，地形／障碍候选必须引用本资料真实cellId／volumeId；contact行只能引用实际垂直起飞或LAND段。成功并不认证调用者报告的数学结论、场景来源、实际构型或当前有效期。使用者仍须单独判断 `declaredModelSatisfied`，更不能把该值等同飞行许可。失败结果不由此函数认可；归档方若保留失败诊断，应另核严格 `{ok:false,error:非空字符串,report:{}}`，不得伪装成功报告。

计划可携带派生 `planningClearanceReview`；Joint仅在本地源副本排除此单独有界字段，避免报告参与自身几何指纹，其他规划、成员、执行配置与约束内容不变。排除前仍检查其纯JSON／非null map／2 MiB预算，不认证该字段真实性；本匹配函数另按完整成功报告核验。Compiler现可保存真实规划范围计算结果，但旧pending仍保留，未完成可信起降接地、实际构型／datum和最后发送前门禁时不得因此解除阻断。本模块没有新增UI、审批、命令或完整StartPermit。

本轮只实现源码并静态对读。新旧范围、无启动的正常规划、缺几何／RTL、LAND接触缺口、预算Unknown、归档往返和输入／顺序／编码变化的源不匹配，均待工程师后续实际编译与验证；本轮未运行这些验证。

## 449原接口与共用计算边界

本模块是工程声明模型的原生纯计算核心，不是净空认证或完整产品入口。`build(clearanceInput, jointView)` 固定返回 `{ok,error,report}`：坏输入、资源预算、源结构矛盾或不支持的精确投影整份失败，report 为空；`ok=true` 仅表示报告已构造，允许报告不完整或已有违反。它不使用 QObject、时钟、文件、Vehicle、参数请求或命令，也不改变 Compiler pending、审批、上传和实际启动门禁。后续仍需可信原生固定消费，不得仅靠 UI 字段放行。

## 来源与预算

输入资料经 [FleetMissionClearanceInput](FleetMissionClearanceInput.md) 完整规范化，保留原场地身份、日期、投影声明、机体尺寸、误差及策略。日期只检查原格式和先后，不检查当前有效期；配置指纹不证明实际机体一致。仅该资料原有 4 MiB／16 层／131072 节点预算继续适用。

联合视图使用 [FleetMissionJointSpatialReview](FleetMissionJointSpatialReview.md) 实际固定22字段；成员12字段、段8字段、来源6字段均严格核对。全部1–32主机必须与资料 vehicles **完整同序**，不从备用机推断替换。核对计数、原顺序、唯一段 ID、编码连续连接、启动来源类型、两起飞段配对、coverage 与缺口关系、固定非许可标志。原 analysis 仅保留为独立诊断 map，不重解释为本环境模型的通过条件；本模块不重建原协议或认证 payload hash。

全部输入合计限制24 MiB紧凑 JSON、20层、1000000节点、8320段；数值均要求有限且绝对值不超过2^53−1，不接受不安全整数、opaque、坏 Unicode 和 typed-null 容器。原联合来源另独立检查16 MiB紧凑 JSON，不被合计预算扩大。完整资料规范指纹及**完整联合视图**指纹进入报告，不只 hash 已算前缀。hash 标识内容，不认证来源。输出最多2 MiB；违反明细最多512项，仍统计所有实际已证违反。几何共享最多2000000工作单位和2048碎片；不拆批重置预算凑完成。

## 精确数值包络

经纬点必须能精确往返为 `double(E7整数)/1e7`，纬度±80°、经度±180°，且距声明 originE7 不超过声明 delta。整数差乘工程声明的纳米/E7比例，绝对值最多4e14纳米，再用整数向下／向上除1000000得到毫米包络。负数向下取整不同于 C++ 向零截断，例如 −1纳米包络为[−1,0]毫米；不四舍五入到0。

AMSL 必须有限且在[−1000,20000]米。先要求8字节 IEC559、二进制 radix、53位精度及 binary64 指数域，再用 `memcpy` 读取符号、指数和尾数，精确将尾数乘1000；正规值指数为 biasedExponent−1075，次正规为−1074。整数移位与余数得到真实表示值的毫米 floor/ceil，**不是先计算浮点 `altitude*1000` 再 floor**；±0数学上均为0。整段两端选 floor 作为整数中心，每轴给声明包络增加两端 `max(ceil−floor)`，仅0或1毫米。这是对数值表示的外包络，不是发明物理安全余量；它可能保守地产生候选，不能反向称为真实碰撞。

水平半宽为机体半径＋水平跟踪误差＋场景水平误差＋投影误差＋水平安全余量；垂直半宽为机体半厚＋垂直跟踪误差＋场景垂直误差＋高程基准误差＋垂直安全余量。机体水平半径在两轴使用同一半宽，以轴对齐正方形保守包住圆盘；不声称精确机身朝向、外形或动力学。每项均来自明确输入，合计每轴最多500000毫米，另加上述数值包络最多1毫米。`sweptBox` 构造**线段与三轴盒的 Minkowski 和（zonotope）**，不是整段轴对齐外接大盒；整数中心限±50000000毫米、包线 bounds 限±51000000毫米，超出支持域明确拒绝。

## 声明环境检查

逆时针凸 polygon 的每边生成闭半空间 `dy*x−dx*y <= dy*x0−dx*y0`；体积另加AMSL上下边界。用精确 `coveredByUnion` 检查整个扫掠体是否包含于 coverageVolumes 的并集、corridorVolumes 的并集；不会只测端点或中心线，也不会要求单个凸体独自包含整段。

每个 terrain cell 先沿本查询完整Z范围提升，求并集覆盖，检查是否有整包线地形资料。地形净空另将原 cell XY 与 `z <= maximumTerrainAmslMm` 同扫掠体求交。障碍柱也同扫掠体求交；边界接触算候选。零长度锚点、垂直段和所有独立启动假设都保留；不把三种启动假设拼成实际航迹。

资料的接触策略要求独立 contact context，但当前 API 没有该上下文。每个 `vertical-takeoff`／`mission-terminal-land` 固定产生 `contact-context-missing`；不删除这些段、不把地面自动豁免、不声称起飞／降落净空通过。其他车型接触语义、实时障碍、实际动力学、风、多机包线间交叠及同步时间均不在本环境模型内。旧联合禁飞区／机间候选检查仍独立存在，不能被本报告替代。

几何 `Unknown` 立即停止余下比较，comparisonComplete=false；此前已证违反可保留诊断，但不是完整报告。已证未覆盖／接触不会提前停止其余可算比较；每类完成数只在三态得到确定结论后增加。

## 报告合同

旧 `build` 的report固定28字段（新范围报告使用上节schema2／29字段）：

`schemaVersion=1,reportType="fleet-declared-clearance-review",planId,reviewKind,reviewId,reviewFingerprint,clearanceInputFingerprint,jointViewFingerprint,memberIds,segmentCount,processedSegmentCount,scopeCoverageComplete,comparisonComplete,declaredModelSatisfied,contactContextComplete,reasonCodes,checks,violationCount,violations,violationsTruncated,workUnits,limits,limitations,snapshotOnly=true,clearanceCertified=false,executionPermit=false,flightCommandReleased=false,actualPathCertified=false`。

- `scopeCoverageComplete` 保留原联合视图的全组名义范围完整性，缺几何／RTL等不会因已知子集成功而消失。仅在所有点已通过本模块支持域时才会产生报告；不支持点整份拒绝。
- `comparisonComplete` 仅本次所有已表示段的环境比较结束；不含独立多机/空域比较，也不证明资料可信。
- `declaredModelSatisfied` 仅当 scope完整、全部比较结束、contact完整且没有任何已知违反时为true；仍不是认证、真实净空或执行许可。
- `processedSegmentCount` 仅计该段所有环境检查均结束者；发生 Unknown 的段可能已有部分 checks，但不计作处理完整。
- 明细截断不丢 violationCount 总数，不停止计算；截断时不把512项称为所有明细。

`checks` 固定10字段：`coverageCompleted,coverageViolationCount,corridorCompleted,corridorViolationCount,terrainCoverageCompleted,terrainCoverageViolationCount,terrainCellComparisons,terrainHitCount,obstacleComparisons,obstacleHitCount`。违反行固定5字段 `code,vehicleId,segmentId,collection,volumeId`；并集未覆盖/接触缺上下文没有单一volumeId，使用明确空串。contact缺口预先遍历全部已验证段记录；计算停止不隐藏未算后段已知缺contact事实。

`limits` 固定8字段：`maximumInputBytes,maximumOutputBytes,maximumDepth,maximumNodes,maximumSegments,maximumWorkUnits,maximumFragments,maximumViolationDetails`。workUnits 是共享几何预算实际扣除值，不是耗时或证明完整性。reasonCodes/limitations是有限说明，不含自动修复建议或权限。

## 静态交付边界

仅源码实现、阅读及文本检查；未编译、运行 helper、解析用例、测试、仿真、UI或连接设备。工程师仍需验证正负／次正规／整毫米边界、E7往返、斜段zonotope、跨凸体并集覆盖、严格开补集与闭接触、地形覆盖缺口、几何预算Unknown、零长度／垂直段、原缺口及contact拒绝。实际数据可信度、配置绑定、当前有效期、原生审批固定消费和完整StartPermit不由本批完成。

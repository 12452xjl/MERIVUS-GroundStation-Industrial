# FleetMissionJointSpatialReview

## 537：逐 NAV 驻留的共享数值语义

联合审阅在原raw／canonical逐项核验中对NAV param1调用共享 [DwellEncoding](FleetMissionDwellEncoding.md)，保持与Compiler／Builder的正值不归零接受域一致。原holdKnown与原连续holdSeconds的source对应、raw请求值与canonical Float32及其原delta核对不变；预算不覆写来源。

可编辑驻留仍不是绝对航点时刻或达向后完整等待；本地旋翼NAV计时从首次位置接受开始，yaw等待可能消耗它。非NAV动作、heading规则、几何及完整联合审阅身份不放宽，未新增StartPermit。

仅源码及限定直接静读，未运行helper／模型、编译、测试、仿真或修改PX4。实际工具链浮点语义、动态行为与最终飞行放行由工程师验证；下方536及更早为历史。



## 536：逐航点朝向编码与原生消费者接线

rawItem 与 canonicalItem 均走共享 HeadingEncoding 的严格 upload(false)，模式与语义代表必须相同；不把收到的原始 Float 当此处输入。显式 NAV 还须从 floatParameters.headingEncoding 的原 requestedDegrees 重建 Request，核完整 exact10 诊断与实际上传 tick；缺省、速度和其它动作不得携带该诊断。

有原 route 视图时，逐点 headingKnown 与原连续 heading 必须对应编码请求，不能只核经纬高／hold 而漏掉朝向。既有 sourcePointIndex、源路径、原始及规范字段、完整 hash 与几何检查仍保留；canonical v3 hash 是声明的朝向语义域而非上传／回读字节认证。Joint 仍不证明实际转向、动作时序或最终许可。

共享合同见 [HeadingEncoding](FleetMissionHeadingEncoding.md)。本批仅源码修改、直接静读与普通文本核对；未编译、运行实现／解析器／算法、测试、仿真、硬件操作、修改PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 496 当前边界：原名义起飞段与完整编码参考分开

Joint源码、segment8／source6／view22及三个入口未改。原vertical仍是startup-hypothesis，from/to协议序号本来未知；bound TAKEOFF contact绑定encoded anchor的实际22。新TakeoffReference的动作归属不得反写或伪造本原source序号。

[Waypoint v5](FleetMissionWaypointSwitchReview.md)从原GPI9／PM29／wire重算full current和full top，以原baseline E7构完整垂直参考；并核本Joint.from完整投影与同次成功Contact ref，要求full current在原窗口。新detail的22→22只作动作关联，不证明current端在wire22或真实时序。原名义components每段两份／applications／issues和LAND参考保持，planning不获得本新PM29模型。

仅新vertical/support指定terrain完整交集可按同有限pad判例外，495 top→NAV及Domain不继承。完整参考不等于实际ground／上升／动态接受／平滑／连续路径或StartPermit；实际[Worker第三报告](FleetMissionStartupClearanceWorker.md)七false和原来源／期限不变。[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前边界：名义起飞两线与完整参考连接分开

本Joint源码、segment8／source6／view22和三个构造入口均未改。它仍把原readiness27键derived的名义top与原起飞请求对应成两条startup中心线；不是重算真实上下界／控制器状态，也不把独立启动假设合成实际连续飞行。

新[Waypoint v4](FleetMissionWaypointSwitchReview.md)另从原GPI9、PM29与wire独立重算编码top完整区间，核同member／payload／TAKEOFFseq／首NAV及本Joint名义来源对应，对首NAV完整接受帽构造参考连接并实际送环境。首outbound NAV与TAKEOFF本来要求同编码XYZ，因此无需造一个独立请求拐点；真正遗漏是当前起飞XY/top区间与首NAV非零接受范围，不是把同点HOLD重复加帽。

新域不是给本Joint增segment或扩大planning，原名义Contact components×2及旧LAND参考绑定保持；新起飞连接不继承接地豁免。实际消费在原[Worker第三报告](FleetMissionStartupClearanceWorker.md)，所有真实轨迹／许可false与完整源／期限仍保留。地面分支条件、动态接受、真实起飞垂直域／平滑轨迹及StartPermit仍未认证。

[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方491为历史。

## 491 当前边界：名义中心线不等于LAND接受区域

本模块源码、segment8／source6／view22及build／buildPlanning／buildStartup三个范围保持。当前支持合同已要求LAND目标与前NAV编码E7 XY相同且高度更低，因此原mission-terminal-land中心线本来就是垂直线；不能把本次漏项误称一条异点编码对角线。

新增[Waypoint v3](FleetMissionWaypointSwitchReview.md)另用同次原PM29，把前NAV非零XY／高度接受区域到已绑定支撑面的完整声明参考体送环境求解。它不是给本Joint偷偷增加段或放大planning范围；planning无原29来源，不能声称已检查机载接受区域、真实着陆阶段或完整动态路径。原Contact名义components仍每段两份。

[Worker](FleetMissionStartupClearanceWorker.md)沿原结果第三报告实际消费新域，原名义报告、原几何来源和认证／许可false继续。真实动态接受、MOVE内部高度／分支、平滑器、实际地面与最终StartPermit仍待完成。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 458：Worker row3只显式投影为本入口原row2

[Worker输入2](FleetMissionStartupClearanceWorker.md)另有必需的[原运动描述](FleetMissionStartupGeographicMotion.md)，但调用本buildStartup前只提取vehicleId和原四键readinessSummary。buildStartup原startupSource4、名义段、独立启动假设、view22及memberCoverage12不变；没有直接接收row3或把source12当额外名义段。

完整运动描述参与外层原inputFingerprint／geometryBasis；Worker还将其payloadContentHash与本完整原计划memberCoverage.payloadHash按成员同序对应。这个内容绑定没有扩大本scope或证明原球面走廊×独立高度区间的包含关系。三个独立启动假设不拼接为实际连续航迹，原认证／许可仍false，Compiler pending保持。下方为历史与仍适用合同。

## 455：完整计划与冻结启动来源的单次构造入口

新增 native-only `buildStartup(plan, reviewEvidence, startupSource)`，返回原固定3键 `ok/error/view`；成功为 `scope="startup-and-encoded-planning"`、`startupIncluded=true` 的范围22字段。`startupSource` 必须是既有 StartupSpatial basis 入口的完整4键：`reviewKind/geometryBasis/vehicles/minimumSeparationMeters`。它由实际原生 owner 冻结，不能从规划航点或地图补出当前位置／起飞观察。

与 `buildPlanning` 共用派生字段处理：只在本地副本排除经原纯JSON／2 MiB检查的 `planningClearanceReview`，不改变原plan或其它字段。完整剩余plan、reviewEvidence和startupSource合并接受原16 MiB／20层／1000000节点预算。先调用一次 `StartupSpatial::buildForGeometryBasis`，再一次整计划 `Spatial::build`，最后复用既有 `buildForGeometryBasis` 的同序成员、编码／动作／序号、独立起飞假设和覆盖校验，不复制解释器或逐机重新编码。

这条入口只构造来源与几何：不会调用环境solver、旧联合空间checker或StartupSpatial旧名义间距checker；analysis继续明确 `available=false/not-run-in-geometry-scope-extraction`。不生成当前Vehicle、来源认证、实际飞行、批准、许可或最终发送证据。纯值 `geometryBasis` 仍不是最终artifact指纹；实际异步调用者必须继续保留原对象、版本、期限和取消归属。失败返回空view，不把已构造的一层当成完整成功。

本次只进行了源码实现与静态对读，未编译、运行、测试或操作设备；完整StartPermit与最终write门槛不由这个入口完成。

## 450：真实编码规划范围与独立前置几何基准

旧 `build(source)`、原最终审阅身份和下方 view22 合同保持不变。新增两个纯值入口也返回固定3键 `ok/error/view`，但成功 view 是**另一种**22键合同，不能混作旧审阅视图：

- `buildPlanning(plan, reviewEvidence)`：一次调用实际 `FleetMissionSpatialReview::build`，取得完整原计划／证据的编码视图，再走共用的 `routeViews`、`encodedItems`、`appendEncoded`。不逐机重建编码、不另抄一套航点解释器、不生成假的 startupView。
- `buildForGeometryBasis({missionReview,startupView})`：使用同一完整任务视图与新 StartupSpatial 的 basis view14。它仍核全部成员同序、原 canonical 请求／动作／来源映射及独立启动段对应，原生调用者必须绑定同次真实 readiness；不从当前地图或最新任务补源。

新 view22：`schemaVersion/planId/scope/geometryBasis/startupIncluded/memberCount/segments/segmentSources/coverageComplete/memberCoverage/zoneInputState/minimumSeparationMeters/thresholdSource/analysis/limitations/snapshotOnly/executable/flightCommandReleased/actualPathCertified/clearanceCertified/timingKnown/onboardContentsProven`。

`scope` 只允许 `encoded-planning` 或 `startup-and-encoded-planning`；`startupIncluded` 分别为 false／true。没有最终 `reviewId`、`reviewKind`、`reviewFingerprint`，也没有 `fullScope`。全部原认证／执行 false 保持。新 `analysis` 固定2键 `{available:false,reason:"not-run-in-geometry-scope-extraction"}`，**没有运行**旧联合空间比较器。其后环境求解由独立 ClearanceReview 执行；完整性校核仅重建编码投影，不重复百万次空间比较。旧 `build` 仍使用原 checker21。

规划 `memberCoverage` 仍为12键，保留全部原活动成员及次序；`startupAvailability="not-included"`、`startupSegmentCount=0`、`takeoffSequenceVerified=false`。这不是启动可用或已检查，而是本入口根本不包含实际当前位置／起飞假设。规划 `coverageComplete` 仅核完整编码、原几何、执行配置和终结语义；缺配置、RTL未编码等仍有明确缺口。不因未提供旧禁飞区或间距而删去已知编码范围，也不把这些资料改成已知：原 `zoneInputState`／null阈值保留，Coordinator 的独立空域门禁仍须满足。带启动基准的 scope 保留原全组范围对启动、原禁飞区和正间距的要求。两者都不是地形、实际航迹、接地或许可证明。

`geometryBasis` 是小写 SHA-256 字符串，绝不是最终 artifact hash 的别名。规划入口摘要精确11键投影：`schemaVersion/scope/planId/memberCount/segments/segmentSources/memberCoverage/zoneInputState/noFlyZones/minimumSeparationMeters/thresholdSource`。其中包含每机已核完整 payload hash 和原禁飞区值，不包含未来派生的环境报告，避免报告递归参与自己的摘要。实际启动入口保留原生调用者在最终 artifact 形成前，对原记录、同次 readiness 几何及工程资料版本建立的摘要；值函数本身不认证该来源。原记录内容／证据／净空资料绑定由调用者另外保持，摘要不是机载或工程配置认证。

复核已带报告的 plan 时，只在本地值副本排除自身派生的 `planningClearanceReview` 字段，防止报告反向挤占原16 MiB源预算／64键上限；任务、几何、执行配置、成员、约束和其它字段均不改。排除前，该字段必须是真实非null map，通过原有纯 JSON／深度／节点检查，且自身紧凑 JSON 不超过2 MiB；坏型、opaque或超限拒绝，不能借排除隐藏。此检查不认可报告真假／来源，完整报告仍由 PlanStore 等实际调用者独立验证。排除后的完整源 plan/evidence 继续原有16 MiB校验和整组 Builder，不截成员／航点前缀。

新入口沿原16 MiB／20层／1,000,000节点预算，成员最多32、整组编码最多8192。范围错误／资源超限整份拒绝，无成功前缀；所有未知值仍显式 JSON null。450目前只接真实规划报告的数据通路；原 `clearance_geometry_review_pending` 未因单一规划报告而删除，接地、实际构型／高程对应和最终发送核验未完成时不得声称完整启动净空竖切已完成。本批未编译、测试或运行任何 helper／应用／设备。

## 449：同一完整联合段的独立声明模型消费

新[FleetMissionClearanceReview](FleetMissionClearanceReview.md)严格消费本view22、全部同序memberCoverage12／segment8／source6，并对原canonical E7／AMSL名义段构造完整机体／误差箱扫掠。场地覆盖、允许走廊、地形范围／上界和障碍使用[精确几何](FleetMissionClearanceGeometry.md)；不改写本analysis或把三种独立启动假设串成实际航迹。

这是纯值独立报告，不是原Joint的scope被扩大为实际安全认证，也不替代跨机时间／动态冲突。未接实际审批／启动消费者；明确接地、时效／实际构型和高程对应仍缺，Compiler pending保持，原六个false与全部来源边界不变。下方442合同继续描述原producer。

## 442：同次冻结捕获的联合名义空间检查

`FleetMissionJointSpatialReview::build(source)` 是纯值函数。输入必须来自调用者绑定的同一次实际捕获；它本身不查询 Vehicle、PlanStore、当前任务或账号，不重跑 PayloadBuilder，不发信号、不写文件、不授予权限，也不修改此前两份独立视图或原空域报告。

输入固定2键：

- `missionReview`：原 [完整任务视图](FleetMissionSpatialReview.md) 的固定3键 `ok/error/view`，须有可用 view17。
- `startupView`：原 [启动名义视图](FleetMissionStartupSpatialReview.md) 的 view15。

两份视图须有相同的1–32个活动成员 ID 和原始顺序，且没有重复、额外目标或备用身份混入。Root 仍负责实际记录／启动审阅身份绑定与捕获首尾复核；值、UUID、内容 hash 或 helper 成功不认证这些来源。

输入／输出均先限纯 JSON 树：16 MiB 紧凑 JSON、深度20、1,000,000节点；原生64位整数必须能被 JSON/QML 数字精确表示。坏类型、非有限值、资源超限或结构矛盾整份拒绝，不发布有效前缀。嵌套重复源值可能更早达到整体预算；这不是允许放大旧输入预算。输出经 JSON 值转换，未知项是明确 null 而非 QML undefined。

返回固定3键 `ok/error/view`。`ok=true` 仅表示能构造诊断视图；不代表输入覆盖完整、空间候选为空、安全、已上传或可执行。失败时 `ok=false`、非空 error、view为空 map。

## 固定输出

view 固定22键：`schemaVersion=1/planId/reviewKind/reviewId/reviewFingerprint/memberCount/segments/segmentSources/coverageComplete/memberCoverage/zoneInputState/minimumSeparationMeters/thresholdSource/analysis/limitations/snapshotOnly/executable/flightCommandReleased/actualPathCertified/clearanceCertified/timingKnown/onboardContentsProven`。

固定标志是 `snapshotOnly=true`，其余六个执行／实飞／净空／时间／机载证明标志全部 false。没有 `safe` 或 `feasible` 字段；也不生成审阅 ACK、启动许可或控制权。

`memberCoverage` 每行固定12键：`vehicleId/role/encodingAvailable/executionProfileAvailable/startupAvailability/payloadHash/coverageComplete/reasonCodes/segmentCount/encodedSegmentCount/startupSegmentCount/takeoffSequenceVerified`。保留全部原目标，原顺序不变；所有计数只统计实际加入的新联合视图，不统计被拒绝的几何。

- `encodedSegmentCount` 最多2048，`startupSegmentCount` 最多3，二者之和为该行 `segmentCount`。
- 全行计数之和等于 `segments.length` 和 `segmentSources.length`。整组编码项最多8192，总段最多8320，不拆批凑一个完整结果。
- 单机 coverageComplete 仅在该机编码／规划几何／配置／所需启动假设／终结动作均无缺口时成立。
- 全组 coverageComplete 还要求原禁飞区输入为 `recorded-list`、原实际正间距已知；它仅表示输入／名义范围已表示，不依赖 analysis.ok 或 comparisonComplete，不证明禁飞区几何受支持、投影有效或计算完成。超出比较预算时可同时出现 coverageComplete=true、comparisonComplete=false，二者必须分别解释。
- `analysis.ok=false` 可能是诊断明细截断；不能把它与“不曾完成全部比较”混为一谈。反之已知子集的 comparisonComplete=true 不能覆盖成员缺口。

## 编码主干

每机严格消费完整 `encodedItems`，同时检查原序号、指令／frame、raw与canonical一致性、sourcePath／sourcePointIndex顺序、动作身份和完整 canonical JSON hash。不从规划路线重新编码，不把独立成员成功拼成原整组编码成功。

地理项仅允许 `MAV_FRAME_GLOBAL=0` 的 NAV_WAYPOINT=16、NAV_TAKEOFF=22、NAV_LOITER_UNLIM=17、NAV_LAND=21。坐标使用原 canonical 的 E7 与 float32 AMSL，而非更理想的原始 double。DO_CHANGE_SPEED=178 必须为原支持的 MISSION frame=2 和速度参数；它的保留零值不是经纬度，跳过时不打断前后地理项连接。

顺序遵守现有编码器的语义：可选配置巡航速度、可选起飞请求、完整出航点、可选返程速度、完整返程点、可选 HOLD／LAND。首个地理点加零长度 `mission-anchor`，后续每对地理项保留名义直线段；即使出航末与返程首或 HOLD 锚点重合，也保留零长度连接用于空间覆盖，不当成进度或停留时间证明。

段固定8键，与439一致：`segmentId/vehicleId/kind/from/to/nominalOnly=true/actualPathCertified=false/timingKnown=false`；端点固定3键 `latitude/longitude/altitude`。ID包含成员、kind与协议序号，稳定且少于128字符。

kind闭集为：

- `mission-anchor`、`mission-leg`；
- `mission-return-connection`、`mission-return-leg`；
- `mission-launch-connection`；
- `mission-terminal-hold`、`mission-terminal-land`；
- 原 `vertical-takeoff`、`takeoff-request-connection`、`current-first-waypoint`。

独立 `segmentSources` 每行固定6键：`segmentId/sourceKind/fromSequenceNumber/toSequenceNumber/fromSourcePath/toSourcePath`；与段一一同序对应。sourceKind 为 `encoded-plan` 或 `startup-hypothesis`；未知协议序号为 null，不用 -1 或数组位置冒充。

LAND 只从最后返程接近点到同一 XY、更低的已声明地面 AMSL，表示名义下降请求。HOLD 是末点锚定请求，不证明任务结束；RTL未编码且没有当前 HOME 路径，不补画。缺 executionProfile 时即便 NAV 编码完整也保留起止语义缺口。编码不可用时不回退规划 routes 或发布编码成功前缀。

## 启动假设保持独立

原439的三种假设不被串成一条实际航迹。本 helper 保留经匹配的独立段，允许检查它们与任务／返程／终结几何的空间关系，但不声称飞机依次飞过三者。

起飞两段必须共同核对 `takeoffDerivation.requestedMissionSequence`、原配置坐标、canonical TAKEOFF 目标及名义垂直段／连接段的端点和连续性。成功时 takeoffSequenceVerified=true；这不是机载执行序号或实飞轨迹认证。

current-first-waypoint 独立核对终点是否等于原完整编码中的首个 NAV_WAYPOINT 坐标。原 startup view 未保留它的 `missionSequence`，因此该段两端来源序号始终为 null，并明确只做坐标对应；不能声称原观察序号已经认证。起飞已知时它仍是替代假设，而非在起飞两段后追加的航段。

编码不可用的成员不能核对其启动请求与任务目标，所以不把这些无法对应的启动段加入联合 analysis；原439独立视图仍可查看。编码已知但启动资料部分缺失时，仅保经匹配的已知段，并将该机 coverageComplete=false、保留具体 reasonCodes。来源身份、缺 profile、RTL、未知启动、错误对应等缺口不会因已知子集没有候选而消失。

## 原禁飞区与间距

新 [checkJointSpatialSegments](FleetAirspaceConflictChecker.md) 只接收上述已验证段、原归档禁飞区值和原归档实际正间距。`zoneInputState` 保留 `recorded-list/recorded-omitted/recorded-invalid/legacy-unknown`；只有 recorded-list 才传原 noFlyZones，其他状态传明确 null，让 analysis 保持输入不完整，绝不变成空列表。

`minimumSeparationMeters` 仅使用 missionView.airspace 中原报告实际采用的正数；未知为 null。不借较小的 startupView 间距、不补5米默认、不把历史实际值说成显式批准政策。`thresholdSource` 保留 `archived-applied-value-not-explicit-policy` 或 `unavailable`。

analysis 固定21键，详见 Checker 文档：独立完整性、总比较数、接近候选／禁飞区命中、各自截断标志及投影适用域。最多1M比较工作、每类512条明细；超预算不能用拆批或截取段前缀伪装全组完成。所有区时间窗按空间保守处理，不推断同步执行时间。

## 工程边界

本轮仅源码与静态阅读。未编译、测试、运行 helper／应用／UI／设备或仿真。AMSL不是离地高度；没有新增地形、障碍物、风、动力学、飞控真实路径或净空证据。新联合候选是供工程师和操作员解释的名义空间诊断，不改变审批、上传、启动或机载权限。

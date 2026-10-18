# FleetMissionWaypointSwitchReview：普通NAV接受区与连接域的静态环境审阅

## 535：同次完整参考包络进入异机比较

`build(..., cancellation, PreparedPtr* = nullptr)` 可在原 result3／report31／v5 不变时，输出同次 `prepare` 的完整 native 参考组件给 [InterVehicleEnvelopeReview](FleetMissionInterVehicleEnvelopeReview.md)。完整 NAV 接受区／连接、LAND 入口／下降、TAKEOFF 垂直／首NAV连接、原顺序及全部 split 区、protected／support 和所有 Polytope planes 原样保留。封装私有、不可变，不能由公开 map／report 安装。无完整 model 时没有组件前缀；环境 Unknown／违规与几何完整性分开。

保原 geometryBasis／clearanceInput／完整 row3 models 指纹及有序 memberIds；另以私有 Motion-row2 指纹与同次 Domain 交叉核，不删 row3 的 parameterSnapshot 绑定。跨机比较不导出或继承任何地形 contact pad／allowedTerrainCellIds／contactVolume；起飞和着陆支撑与另一飞机的接触不能获地形例外。

失败／取消清可选输出。`resultMatches(..., result, PreparedPtr* = nullptr)` 在原完整复核后只给 matcher-only 几何，不能送实际新 build，不执行环境／异机求解器或恢复 native 来源。旧不传输出调用及所有模型／预算／结果语义保持。

新增异机比较保无可靠时序的全空间保守、旧 minimumSeparation 独立要求和真实 planes，Unknown 不当 clear。原动态高度接受／平滑／真实路径 flags 仍 false，Domain 的 actualPathCertified 阻断不放开，不产生 StartPermit。仅源码及直接静读，未编译、运行、测试、预览、执行算法或修改 PX4；下方为历史。

## 496 当前：完整起飞垂直编码参考与原有限支承接触

模型升为 `ordinary-nav-land-takeoff-vertical-reference-xy-hull-prism-v5`，旧v4不匹配。API、result exact3、report exact31/schema1及Worker7不变；新增 `takeoff-vertical-reference` 纳入既有reference／component／processed／violation计数，NAV计数仍仅普通NAV。

### 同次原始材料与完整窗口

保495完整Motion source／GPI9／wire／PM29、实际MIS REAL32=9＋exactfloat、ActionPhase及Joint四类段／路径／序号／点对应。TakeoffHeights另保同运算顺序的current下／上界，与原predicted完整双端分别精确floor／ceil毫米、按原baseline E7完整投影，不读取caller derived界或换请求XY。无22成员不继承该专属前置。

实际TAKEOFF需要normalized schema2。与LAND共用同一次Contact.prepare，TakeoffReference只来自原成功applied的vertical/support；按vehicle／原segment／实际22序号核对。同原Joint.from重做pointMillimetres，全部lower/upper须等原ref；整数GPI的直接投影只需被旧数组包含，因为double(mm)/1000的精确毫米界未必与整数点数组完全相同。不能把应用表的referenceTo当起点。

原窗口为groundAmslMm＋supportBelowReferenceMm±referenceHeightToleranceMm。整个full current编码高度区间须落其中；零容差合法但可能不足，不能clip尾部／加epsilon／扩默认。缺schema2、缺成功参考或窗口不容纳时整组不完整／失败，无可用几何前缀；全局contactContextComplete=false不先抹其它已成功参考，但任一本组需要的ref缺失仍不完整。

### 两端完整盒与有限例外

用full current盒和full predicted top盒走原connectionHull，保完整XY hull×min全部Z下界..max全部Z上界，再分别protected／support外扩。两个编码区间可重叠，不能只取currentLower→topUpper、要求严格升高或宣称新体对每输入严格更大。旧Contact名义components恰两份／段、应用／问题及LAND参考保持，新增体不是替换旧体。新详情from/to均关联原TAKEOFF动作序号；这不表示捕获current起点本身是wire22或实飞序号／轨迹。

新体真实走原三类完整覆盖与terrain／obstacle接触。仅本vertical/support带同次原ref有限pad：指定terrain中整个expanded component∩该cell须由同finitepad band覆盖才可例外，Unknown不允许；该包含检查使用同一原work／fragment预算。protected/body、障碍、未列cell、前三覆盖、495 top→首NAV连接、普通NAV、LAND入口及Domain整运行域不继承；LAND下降沿原自身合同。

32成员／8192全组items、32768region／65536components、2M work／2048fragments、512详情和2MiB不增额。resultMatches仍同prepare重建完整源、版本、region／component／counts／detail引用，不重跑solver认证caller verdict。七认证／许可false及snapshotOnly=true保持，原Worker actualPath真要求不删除。

条件参考不认证当前landed、ground-valid、真实上升／接受域、内部平滑器、连续物理轨迹、永久HOLD／抗风漂移、StartPermit或正式任务结果；planning仍无原PM29不能借此扩大。见[Contact](FleetMissionContactModel.md)、[Worker](FleetMissionStartupClearanceWorker.md)与[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方495及更早为历史。

## 495 当前：完整起飞编码参考区间连接首NAV全接受帽

模型名升级 `ordinary-nav-land-takeoff-reference-xy-hull-prism-v4`；旧v3不能通过新resultMatches。公开API、返回exact3／report31／schema1不变，navWaypointCount／navConnectionCount仍仅普通NAV；新区域 `takeoff-nav-reference-connection` 计入既有referenceRegionCount／componentCount／processed及详情，不加无人消费count。actionBoundaries的22与独立动作未认证原因保留。

### 原材料与条件范围

同次完整Motion source先按原prepare全量对应，再读PM exact6的29行。只有wire含TAKEOFF的成员新增MIS_TAKEOFF_ALT REAL32=9、非负、精确float32及原ActionPhaseReview.takeoffCompatible检查；纯NAV或仅LAND不继承起飞专属限制。唯一22、实际首16及其同请求XYZ／原payloadHash，Joint member／原encoded anchor与launch连接、两条startup名义段及其序号／sourcePath都须对应，不拿速度178当位置或选另一个NAV。无完整来源或不支持分支整组不完整／失败，不能比较部分成员。

不能取caller `predictedTakeoffLower/Upper` 或只信derived旗标。原GPI9的AMSL整数毫米除1000、转float，加原实际MIS，再与canonical22的float p7取最大；依Readiness同序对current±0.001及每级float结果向外nextafter，独立得到完整predicted下／上界。要求IEEE binary32与有限、有序、完整支持域；MIS零可用，其他参数型或double先舍入冒充REAL32不接纳。原基线E7／名义点／profile／序号对应只保来源一致性，不自证landed／ground-valid，真实地面/新鲜前置仍在Readiness与原生外层链。

### 完整参考体与真实比较

源端固定原baseline E7处的完整top编码高度区间：lower精确floor毫米、upper精确ceil毫米，再用Contact原整数E7／纳米投影包住完整端点盒，不走degrees往返。目标端是首NAV既有全部cap分裂与完整高度接受slab；每一分量都与源端构造完整XY凸包×两端完整Z，保闭端点、点／线退化及斜边面。不能用名义top、请求中心、声明最高高度或近半边替代完整域，无法完整投影／表示就拒绝，不clip。

新体分别沿原protected／support误差扩张，真实进入coverage／corridor／terrainHorizontal覆盖和terrain／obstacle接触。新区域一律无contactId／volume／指定terrain；即使低端触地也不能继承旧current-first、TAKEOFF或LAND pad。原NAV/LAND区域定义、原LAND support有限接地范围保持，不新增起飞垂直整域或TAKEOFF接受帽。

32成员、2048单机／8192全组项、32768 reference／65536 components、work2M／fragments2048、512详情及2MiB原预算不扩。Unknown／耗尽仍不完整，取消或无法完整构模无可用前缀。resultMatches沿同prepare重建完整源、版本、区域／分量、计数和详情引用，不独立重跑solver或认证caller verdict。

### 仍非实际轨迹或许可

七个数值接受／实际路径／动态高度／平滑器／执行／飞行／净空证明保持false。modelComplete／comparisonComplete／declaredModelSatisfied只描述本声明范围；本批没有实际起飞垂直可达域、内部ground分支、动态接受扩大、采样间真实平滑路径或持续HOLD／抗风漂移证据。原[Worker](FleetMissionStartupClearanceWorker.md)实际消费并保actualPath真要求，[Joint](FleetMissionJointSpatialReview.md)仍是旧名义段，不扩大planning scope。

[36项工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方491为历史。

## 491 当前：完整LAND声明参考体进入原五类环境比较

本模型在普通NAV帽与相邻连接之外，使用同次原PM29和成功应用的schema2 LAND接地参考，增加接受区域到支撑面的声明几何。报告仍返回3／report31／schema1，Worker结果7键不变；模型名升级为ordinary-nav-land-reference-xy-hull-prism-v3，旧v2不能通过新resultMatches。actionBoundaries保原全部动作及独立未认证说明，navWaypointCount／navConnectionCount仍仅NAV，新增体进入referenceRegionCount／componentCount与详情同序对应。

只对实际wire含LAND才执行原ActionPhaseReview.evaluate(legacy,phases)并核landCompatible，纯NAV不继承LAND专属参数失败。LAND需同次schema2 ContactModel.prepare产出的原LandingReference：vehicle、目标sequence和实际mission-terminal-land segment对应，不拿p7作停止高度、不默认地面／支承尺寸。RC辅助／外部注入或分支不兼容、缺接地参考、完整入口slab不能全在rest之上时，整份不完整或失败且无可用几何前缀；不得clip入口低端。

前NAV cap保全部XY接受范围、完整floor／ceil(p7)±NAV_MC_ALT_RAD区间及反经线分裂。对每个原接受盒与原目标XY，先构造完整XY凸包：

- land-entry-connection：凸包×原完整入口Z区间，不得继承任何接地豁免。
- land-descent-reference：同一完整XY凸包×原rest参考面至入口Z上界，不是只在目标XY的一根竖柱；它保留在XY尚未收敛时同步下降的全部声明仿射连接。

分别按原构型／误差膨胀protected与support；全部新体实际进入coverage／corridor／terrainHorizontal并集覆盖及terrain／obstacle接触检查，沿原索引／完整面／work2M／fragments2048／取消／2MiB。只有land-descent-reference的support带原contactId／指定terrain／有限pad；命中terrain后再用同Budget核完整support∩terrain被该pad volume包含，Unknown不能当豁免。body、obstacle、入口和其它region无豁免；pad不覆盖整个NAV接受范围时应保留违规。

配套PX4仅在当前位置超出接受半径时插MOVE_TO_LAND，该分支高度取现有POSITION目标或当前global；直接LAND可水平收敛同时下降，Z位置目标为NaN而非p7停止。当前Joint强制LAND与前NAV同编码XY，原名义中心线已经垂直，新范围针对非零接受区域。动态高度接受可由内部状态放大，真实入口／MOVE分支／平滑／地面／时间仍未证明；numericAcceptanceContainmentProven／actualPathCertified／dynamicAltitudeAcceptanceProven及权限等false保持，不能称完整实际可达域或StartPermit。见[原生参考](FleetMissionContactModel.md)、[Worker](FleetMissionStartupClearanceWorker.md)和[未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前：完整XY凸包查询沿原并集覆盖消费

coverage／corridor／terrainHorizontal均先准备完整索引，后以原component.envelope调用coveredByIndexedPrisms。凸包的非轴斜面、额外严格约束及完整Z继续进入原union；bounds仅作候选粗筛，不把原凸包重新放大成AABB执行覆盖结论。全部原scene先校验／自持，严格分离才剔除，触边与退化query保留。

terrainHorizontal用独立全Z±51000000mm，不能借有terrainTop的接触索引漏掉高处query的XY覆盖。原terrain／obstacle接触索引及完整query精确可行性检查不变。最多五份索引准备与全部环境操作共享原work2M／fragments2048和token，不reset／加预算；准备失败无成功前缀，index成本可增加Unknown。

模型名仍ordinary-nav-acceptance-connection-xy-hull-prism-v2，返回3／report31／schema1不变，workUnits允许因实际算法变化不同。原numericAcceptanceContainmentProven／actualPathCertified及执行权限false不升格；普通NAV连接并非LAND移动／下降、TAKEOFF／HOLD或平滑器真实动态轨迹。见[几何API](FleetMissionClearanceGeometry.md)和[未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：XY凸包棱柱连接，非真实动态路径认证

当前模型名为 `ordinary-nav-acceptance-connection-xy-hull-prism-v2`，返回3／report31／schema1保持。替换482的 `ordinary-nav-acceptance-connection-aabb-v1`；旧报告不能通过新 `resultMatches` 的模型名核对。没有新公开API／QObject、Worker或工程登记。

### 完整连接域与余量

每对相邻NAV接受区的完整投影矩形取最多8个XY角点，整数排序／去重／monotone hull构成XY凸包；独立保两端最小／最大Z完整闭区间。所有前后帽分量组合继续构建，包括经度接缝；不能完整投影／表示时整份失败，不clip或只选较近半边。它包含端接受区间的所有仿射直连，但不证明PositionSmoothing或动态控制器实际遵循这些直连。

单点用bounds，线保相反闭边约束与端点bounds；多边形保CCW支持半平面。纯轴向边与bounds相同约束可省略，所有非轴向面必须保留。边法向先以正gcd精确约分，再以整数顶点形成等价limit，不四舍五入方向或截limit。坐标±51000000mm、法向±100000000、limit±20000000000000000；cross及外扩乘积在int64内检查，不支持的面／扩张失败无前缀。

protected与support分别按既有声明扩张。水平bound扩H，同时每个保留边面 `limit += (abs(east)+abs(north))*H`；只扩bounds会把斜边机体余量截掉。Z继续按各自below／above及跟踪／场景／datum／安全误差非对称扩张；最终bounds及面limit再次核域。

### 真实消费者、预算和边界

coverage union、corridor union、terrain水平覆盖都消费完整凸域。接触先用明确无planes的AABB副本调用原索引；无额外面查询保留原indexed SAT路径，带非轴向面的查询追加**同一prepared.input、同一行顺序**的原场景polygon／altitude平面，再调用Geometry::feasible。粗筛命中不直接变接触候选，不向AABB专用API偷送带面query；地形下界−51000000在整个允许query域内等价，普通NAV不继承动作接地豁免。

scene每行／两索引完整准备，原共享2M work／2048 fragments及取消／Unknown机制不变，不逐机重置。带面的窄相交可能增加成本，不能保证更快；资源不足只给comparisonComplete=false，不补成功。原Worker／两Review／Witness／Authority消费同一报告；resultMatches同prepare重建完整输入、域、计数及v2标识，不独立重跑solver或认证caller伪造verdict。

`numericAcceptanceContainmentProven / actualPathCertified / dynamicAltitudeAcceptanceProven / smoothingReachableSetProven / executionPermit / flightReleased / clearanceCertified`仍false。真实动态高度、平滑器可达集、采样间连续性、动作／净空及完整StartPermit继续待开发。[32项交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；没有编译、执行本模型／helper／业务／应用、测试、仿真、改PX4、硬件／刷机／放行、提交、更新PR或打包。下方482为历史AABB模型，不覆盖本节v2。

## 482 当前：真实第三报告，非动力学或StartPermit认证

本模块是无QObject的纯值审阅器，源／头由 `custom/custom.pri` 登记。它真正构造并比较普通 `MAV_CMD_NAV_WAYPOINT`（16）的到点接受区和相邻NAV连接外包，不把航点折线或固定 `NAV_ACC_RAD` 冒充完整真实运动域。当前模型名 `ordinary-nav-acceptance-connection-aabb-v1`，报告schema1。

### 输入与真实消费者

`build(clearanceInput,models,frozenJointView,geometryBasis,cancellation)` 接完整原净空资料、已冻结Joint及同一geometryBasis。models按完整成员顺序，每行exact3：`vehicleId`、完整 `geographicMotionModel`、原 `parameterSnapshot`；不是任意自报radius。参数由Readiness同次冻结、两Review原owner保源，Worker/schema3成员row4投影为此row3。

参数快照必须是PM严格exact6：schema1、legacyRows12、rateGuardRows2、actionPhaseRows14、arrivalRows1和RAMP负零marker。模型只实际消费旧12中的 `NAV_ACC_RAD`、独立arrival中的 `NAV_MC_ALT_RAD` 和动作14中的 `COM_OBS_AVOID`；其余原行仍完整绑定。数值codec不认证来源；两Review和Witness另以原PM／PRIMARY弱owner核29项，异步完成后不换样。

`FleetMissionStartupClearanceWorker::run` 实际运行本模块，追加结果 `waypointSwitchReview` 为result7；两Review、Witness和两Authority保完整结果身份。共用准入要求本报告模型与比较完整、无violation且声明满足，但仍保整域actualPathCertified=true条件；现producer仍false。

### 构造范围

- 只建command16接受区。水平半径REAL32以精确binary64向上取毫米整数，纬度E7帽采用向外界、保完整经度范围与反经线两端编码；完整帽越±80°时整份unsupported失败，不clip，也不借用受控startup整数域裁剪机载NAV接受集合。垂直独立用目标p7精确AMSL毫米floor／ceil再加减垂直接受半径，避免先做p7±radius丢掉小半径。
- 帽的完整整数盒由Contact精确仿射投影到局部毫米AABB。连续两个NAV的所有前／后帽分量组合取完整包络AABB，作为 `nav-transition`；不是仅检查端点或抽样中心线。
- command178速度项不打断NAV邻接；TAKEOFF22、LAND21及HOLD等非NAV项记录 `actionBoundaries` 并断开普通NAV链，保独立动作语义。没有普通NAV则明确失败，不虚构一个空成功。
- 依据原vehicle／policy分别扩张protected与schema2的support分量，包括机体尺寸、跟踪误差、场景／投影／datum误差及安全余量。超域或无法完整建模则失败，不返回已构造前缀。
- 全组中任一 `COM_OBS_AVOID=1` 则标静态分支不完整，返回明确reason且不比较部分成员。`COM_OBS_AVOID=0` 也不证明先前控制器动态状态已经消失，不能据此认证动态高度接受域。

### 实际环境比较与预算

每个完整protected／support分量实际送入coverage union、corridor union、terrain水平覆盖、terrain候选prism及obstacle候选prism比较；普通NAV分量不能借独立动作的Contact接地豁免。全部原terrain／obstacle prism及两索引先完整准备，准备与全组所有比较共享**同一个**Geometry预算，不逐机／逐分量重置。unknown／耗尽使comparisonComplete=false，不补成功；取消不发布成功前缀。

| 有界材料／计算 | 固定上限 |
| --- | ---: |
| 成员 | 32 |
| 单机canonical items／全组items | 2048／8192 |
| reference regions／expanded components | 32768／65536 |
| 独立action boundaries | 8192 |
| 全组work units／fragments | 2000000／2048 |
| violation明细 | 512（总数与截断标志另保） |
| 完整输出JSON字节 | 2097152 |

输入沿既有PlanningWorker完整快照材料预算；局部坐标及扩张结果保±51000000mm域。范围／成员／顺序／wire／payload hash／源不一致或材料超限不能裁剪后成功。

### 输出合同与匹配限制

返回exact3 `{ok,error,report}`。失败report为空；成功report恰31键：

```text
schemaVersion, reportType, modelName, geometryBasis,
clearanceInputFingerprint, modelsFingerprint, memberIds, memberCount,
navWaypointCount, navConnectionCount, referenceRegionCount, componentCount,
processedComponentCount, actionBoundaries, modelComplete, comparisonComplete,
declaredModelSatisfied, violationCount, violations, violationsTruncated,
reasonCodes, workUnits, limits, snapshotOnly,
numericAcceptanceContainmentProven, actualPathCertified,
dynamicAltitudeAcceptanceProven, smoothingReachableSetProven,
executionPermit, flightReleased, clearanceCertified
```

最后七项证明／权限字段固定false，snapshotOnly固定true。`declaredModelSatisfied` 仅为本声明静态模型完整、比较完整且零violation，不是安全飞行结论。默认reason明确保动态高度接受未观测、平滑器可达域未证明、独立动作未认证；环境碰撞为候选，不充当物理事故断言。

`resultMatches` 重建完整输入、原参数／Motion源、接受区／连接／分量和计数，并核报告完整shape、来源fingerprint、明细引用／上限和一致性。它不独立重跑环境solver、不认证外部伪造verdict；实际owner必须保原Worker、同次input／result及原native source链，不能只凭一张合法map或相同hash授权。

### 未完成与工程验证

PX4可能采用当前controller扩大高度接受域，PositionSmoothing还依赖当前位置／速度／加速度／jerk／内部航点更新；本模块没有这些完整动态状态。因此不能宣称固定参数帽或连接AABB覆盖所有实际轨迹，不能把其结果当起降／HOLD动力学、采样间连续性、避障能力或StartPermit证明。完整后续模型与许可工作仍待开发。

本批只有源码及文档静态工作，未编译、执行本模型／helper／业务／QML、测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。[工程师交接场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；最终普通文本核对已记录于本批静态审计。整体约40%仅源码工作量粗估，不因本模块或41路径而提高。

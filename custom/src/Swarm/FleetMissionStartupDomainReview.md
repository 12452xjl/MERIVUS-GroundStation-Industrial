# 启动完整理想运动域的声明环境计算（467）

## 535：同次完整原生包络交给异机比较

现 `build(..., cancellation, PreparedPtr* = nullptr)` 在原 result3／report28 不变时，可把本次 `prepare` 的完整原生 components 输出为 [InterVehicleEnvelopeReview](FleetMissionInterVehicleEnvelopeReview.md) 私有不可变封装。保原 Models build 路径、geometryBasis／input／models 指纹、成员顺序、每个 split cell 原数组位置／segmentIndex／endpointIndex、protected／support 及全部 Polytope 平面，不从 public report 重建或按显示身份去重。只有完整 outerCover 才带 components；几何不完整的合法来源封装为空，无前缀。环境 Unknown／违规与完整几何导出分开，原环境结果和预算不改。

封装仅同次成功构造且未取消后发布，失败／取消清输出。`resultMatches(..., result, PreparedPtr* = nullptr)` 只在原全量形状／来源复核成功后输出 matcher-only 封装；临时 value-only 几何不可送实际 Inter.build，不能恢复原 Models 指针或设备／审批来源。

新异机计算覆盖 Domain×Domain 及与 Waypoint 全组件组合，不继承地形接触例外，不替代旧 minimumSeparation。原报告 schema2／model v3、outer／numeric／actualPathCertified 与所有环境语义不变；actualPathCertified 仍 false，不开放完整 StartPermit。新比较自己的统一预算覆盖完整输入／平面校验及全部 pair，未重复本模块环境求解。

仅源码／直接静读，未运行、编译、测试、预览、执行算法或修改 PX4；下方为历史。

## 490：原完整域的三类并集覆盖接入索引

prepared.input完整normalized scene分别准备coverage／corridor／terrainHorizontal原凸柱索引；加原terrainContact／obstacleContact共最多五份，全部准备成功后才开始逐component环境判断。coverage和corridor保原min／max AMSL，terrainHorizontal用独立±51000000mm全Z域，terrainContact仍保原实际top，不能混用。

三处覆盖调用改为Geometry.coveredByIndexedPrisms，传原component.envelope全部额外／严格面。仅剔除bounds严格分离的原scene行，候选恢复原闭面后交旧union；原接触SAT、顺序和接地例外不改变。所有准备／查询共享2M工作／2048片段与原取消token，准备失败不发布成功前缀；新增准备可更早Unknown，无普遍性能保证。

原schema2／model v3、numericContainmentProven规则、完整源与同原Models链不变；workUnits可不同。actualPathCertified、executionPermit与flightCommandReleased仍false，Worker完整启动许可仍被缺失的真实动作／动态路径证明阻断。见[几何API](FleetMissionClearanceGeometry.md)和[未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 481：schema2／versioned-startup-domain-cover-v3

报告字段数保持，schema升2、modelName为versioned-startup-domain-cover-v3；旧report1不当作本版本当前依据。旧Motion/source1与新显式source2均能进入各自分支；actual build必须传完整原Models，逐机源值对应原Ptr，不能以值重建替原对象。

numericContainmentProven只在全组每个Cover.nativeIntegerDomainCovered且完整外包／构型扩张均成功时为true。旧模型、混组、任何模型／全组预算／frame／扩张失败都false；value matcher按完整输入重现临时模型、全源比较并重算该属性，不仅相信bool。新source的自报numeric字段仍false。numeric是空间包含属性，环境comparisonComplete／declaredModelSatisfied／违规数量另核，未完成环境比较不能作安全通过。

原32模型／8192items／32768cells／65536components与共享work2M／fragments2048保持；任一后成员失败不发布可用components前缀。名义TAKEOFF／LAND接地pad不转移到整域，实际动作与采样间路径未证明的reason始终保留，actualPathCertified／executionPermit／flightCommandReleased仍false。

即使新numeric=true，Worker实际startupModelSatisfied也要求actualPathCertified=true，而本报告固定false，故完整启动许可仍阻断。没有工程clearance输入时不生成此报告或包含结论。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 471：实际原Ptr进入整域计算，纯值匹配不恢复数值来源

实际 `build` 必须另传完整 `GeographicMotion::Models originalModels`，由[Worker](FleetMissionStartupClearanceWorker.md)保存同次Review的原工厂不可变对象。models值行、资料vehicles和Joint成员必须同序完整对应；原map的key set数量必须完全相同，各ID非空原Ptr的完整source12必须与值行规范JSON一致。实际准备不调用prepare重建或作缺省回退，而是对**同一个原Ptr**调用Cover，固定原Qt段长／方位角缓存的对象来源；有相同JSON的另一对象不能由本模块自行替换成为原数值依据。实际身份由上层原生owner及finished尾核逐Ptr保留，不是此纯值比较能认证。

`resultMatches` 的公开值签名不改，继续在明确的value-only分支临时重建模型，用来核完整描述、外包、计数及报告形状，**不能证明与实际worker使用的是同一对象或相同Qt缓存操作数**，也不能把它的临时对象送回实际计算／Witness／运行阶段。实际build始终传非可选原map；空、缺、多、错源map失败，不会转入value-only分支。

input/source/report全部schema、完整JSON指纹、v2模型名和预算不变；原Ptr不进入JSON。numericContainmentProven和所有认证／许可／释放仍false，原Compiler pending及接纳门禁保持。此次同对象传递不证明原Qt/libm全接受域已包含于声明场景，不改变全部地理腿／独立AMSL域或扩大动作接触例外。只做源码及限定静态核对，未编译、执行helper、测试或测量性能。以下为较早增量，实际模型来源以本节为准。

## 470：原行身份空间索引与完整候选后的真实SAT

在469完整terrain／obstacle凸柱都准备完成后，沿**同一份**环境Budget各构造一个原生 `PreparedPrismIndex`。两个索引必须都完成才开始任何component环境判断；索引持有完整原凸柱值和原行身份，不借可变调用者数组、不重排资料行或用BVH节点号代替cell／volume身份。原凸柱／索引准备任一取消返回失败空report；资源耗尽保留完整outer/counts并返回comparisonComplete=false／processed=0的incomplete报告，计入所有已消耗work；索引非法使用独立固定 `startup-domain-scene-prism-index-invalid`，不是碰撞或有效空资料。

每component原coverage／corridor／terrainXY并集覆盖完全保留。之后分别对terrain和obstacle索引调用 `prismCandidates`，**只有bool成功且候选完整**才使用其唯一、升序的原始行号；空候选是合法完整结果，不伪造一条相交。不完整／超预算候选输出绝不消费前缀，转为Unknown停止该component及余下计算，已有其它完整比较产生的候选仍只作incomplete诊断；取消最终仍丢弃整份报告。

每个候选必须继续调用 `axisAlignedBoxIntersectsIndexedPrism`进行真实闭SAT，**不是包围盒相交即碰撞**。索引只排除被精确证明AABB分离的凸柱，不省去最终XY边法线及闭Z判断；接触仍算相交，地形真实顶面和障碍原上下高度不变。报告明细按原行查回真实cellId／volumeId，不暴露树节点或借索引改变动作接触范围。indexedSAT的Unknown继续原停止与取消规则，所有准备、候选遍历、SAT和并集共享2000000 work／2048 fragments，没有逐查询预算重置。

模型仍v2、result3／report28／limits8／row6／Worker6及numericContainmentProven=false保持；没有新增报告字段、资源放宽、Qt数值证明、物理认证或许可。workUnits包括本版实际索引准备及遍历扣耗，不是实测CPU／功耗或旧算法耗时对照。本轮仅源码和静态读回，未编译、运行helper或测试。下面469及更早节为历史增量，当前环境求交以本节完整索引＋SAT流程为准。

## 469：完整场景一次准备、整域逐对精确SAT求交

当前环境求交改用[Geometry](FleetMissionClearanceGeometry.md)新增的原生私有状态 `PreparedPrism`。在全部输入／原Joint／motion来源及完整外包准备成功后，先建立全组唯一的2000000 work／2048 fragments Budget，按完整terrainCells、完整obstacleVolumes顺序一次准备所有凸柱；每次 `prepareConvexPrism` 的完整严格凸／逆时针／坐标／闭高度验证也消耗这份预算，不按资料组或后续查询重置。只有全部凸柱准备完成，才开始任何component的环境判断；默认无效或准备失败的对象绝不参与查询。

地形凸柱下界为−51000000毫米、上界仍是该cell的真实maximumTerrainAmslMm：由于所有query的完整Bounds本已处于同一数值域，这与原 `z<=maximumTerrainAmslMm` 在query内的交集精确相同。障碍原minimumAmslMm／maximumAmslMm完全保留，未抬高地形、删高度或改变支撑尺寸。实际每对terrain／obstacle交集调用 `axisAlignedBoxIntersectsPrism`：完整query必须没有额外平面，以闭Z区间、XY矩形轴及完整凸多边形边法线进行双向整数分离轴判断，边界接触仍相交，点／线／零厚度query不因退化丢失。覆盖、走廊、地形XY竖柱的原精确并集覆盖solver保持不变。

准备期间取消返回失败空report；资源耗尽返回保留整组outer/counts的完整incomplete报告，comparisonComplete=false、processedComponentCount=0、violationCount=0，workUnits保留已经消耗的准备工作；非法凸柱／不支持几何返回失败空report，不能把资源、坏输入和取消统称成功或使用准备前缀。之后的SAT及并集继续使用同一budget和token，原Unknown停止规则及明细前缀检查保持。

数学域、接触语义和结论不变，因此模型名仍为 `ideal-sphere-E7-two-cap-slab-outer-cover-v2`，不增加或伪造算法身份字段；result3、report28、limits8、违反row6和Worker6不变。workUnits是此版实际算法扣除值，包含一次场景准备、不是与旧通用顶点枚举直接可比的耗时；没有执行CPU／时延／功耗测量。旧v1仍不接纳，numericContainmentProven仍false，原Compiler pending及全部实际消费者门禁保持。本轮无空间索引、不改原Qt谓词，未编译、运行helper或测试。以下468／467为历史增量与仍适用合同，环境求交实现以本节为准。

## 468：纬度分段外包版本及不完整报告的真实处理前缀

当前模型名升级为 `ideal-sphere-E7-two-cap-slab-outer-cover-v2`，配合[DomainCover](FleetMissionStartupDomainCover.md)新的保守纬度分段经度下界。这里继续消费完整两端帽与完整独立AMSL slab，不删原腿、不收紧原策略、不扩大投影域或几何预算。旧v1报告不自动升级、不作为当前结果接纳；必须从同一完整来源重新计算。result3、report28、limits8及全部原成员／payload／geometryBasis绑定保持。

同时收紧纯值 `resultMatches`：当comparisonComplete=false时，违反明细必须引用已处理完整前缀 `[0,processedComponentCount)` 或当前尚未完成的component `processedComponentCount`。此前只核“属于整份准备几何”，可能接纳不可能由顺序solver产生的末尾成员／腿明细。现在建立row6几何身份到**最早**component索引的对应；反经线分裂可以共享身份，保留任一真实前缀出现的合法细节，不强求唯一cell、不错误要求明细按最早索引单调排列。完整报告不增加前缀限制；计数、原资源／取消和512明细规则不变。

地形资料覆盖检查也移除逐component复制和重复高度面：直接把完整规范terrainCells的XY凸多边形半空间作为无界竖柱并集交给Geometry。查询Q本身已有同一完整闭Z区间ZQ，因此 `Q∩((P×R)∩ZQ) = Q∩(P×R)`；只省去对query冗余的两平面，不改变完整覆盖集合。公共solver允许1–18个闭半空间的无界union成员，查询始终有界。资料仍先完整规范化／校验，使用同一work／fragment预算及取消token；terrain接触的 `z<=maximumTerrainAmslMm`、coverage／corridor高度和障碍上下高度完全保留。这减少源码中的向量复制／面构造及实际求解所扣work，workUnits可以与旧算法不同；没有运行测量CPU、时延或功耗，也不声称已验证性能提升。旧v1报告仍不混作当前v2结果。本轮不引入SAT或空间索引。

这些是更紧的理想外包与真实报告进度一致性，不是原Qt／libm数值接受域证明。numericContainmentProven仍false，所有认证／许可／飞行释放false、原Compiler pending和实际共同消费者门禁保持。只做源码和静态文本／空白核对，没有运行helper、编译、测试或设备操作。以下为467基础合同，模型名以本节v2为准。

`FleetMissionStartupDomainReview` 真正计算[DomainCover](FleetMissionStartupDomainCover.md)准备的**全部理想球面两端帽并集 × 独立完整 AMSL slab**外包络与声明场地、走廊、地形及障碍的关系，不只是增加来源或 pending 字段。每个外包 box 都按原机体／支撑尺寸及误差完整扩张，并实际调用[精确整数几何](FleetMissionClearanceGeometry.md)的并集覆盖和凸体求交。

它和原[名义 ClearanceReview](FleetMissionClearanceReview.md)是两份独立报告；不改写 Joint 名义段，不把两份范围拼成实飞路径，不改变原运动策略。当前模型固定为 `ideal-sphere-E7-two-cap-slab-outer-cover-v2`。**理想球面外包不等于原 Qt/libm `contains()` 的接受域包含证明。** 本报告的 `numericContainmentProven` 始终 false，reasonCodes 始终包含 `original-qt-numeric-predicate-containment-unproven`；即使理想声明模型满足，也不能据此发启动许可、解除原 Compiler pending 或放行命令。

本轮仅源码实现与静态读回；未编译、运行 helper／线程／脚本／解析器／UI、测试、仿真、SQL、归档操作、设备或飞行。

## 原生 API 与完整来源

头文件：[FleetMissionStartupDomainReview.h](FleetMissionStartupDomainReview.h)；实现：[FleetMissionStartupDomainReview.cc](FleetMissionStartupDomainReview.cc)。

```cpp
static QVariantMap build(const QVariant& clearanceInput, const QVariantList& models,
                         const FleetMissionStartupGeographicMotion::Models& originalModels,
                         const QVariantMap& frozenJointView, const QString& geometryBasis,
                         const CancellationToken& cancellation = CancellationToken());
static bool resultMatches(const QVariant& clearanceInput, const QVariantList& models,
                          const QVariantMap& frozenJointView, const QString& geometryBasis,
                          const QVariantMap& result);
```

`models` 为完整同序1–32行 exact2 `{vehicleId,geographicMotionModel}`。实际build必须与完整originalModels各ID原工厂Ptr的完整source12紧凑Qt JSON相同；纯值resultMatches才通过共享[GeographicMotion::prepare](FleetMissionStartupGeographicMotion.md)临时重建描述。不能只读取E7、挑地理项或忽略速度项后认可hash，不能凭值匹配冒称原Ptr身份。整组canonical项最多8192，每机1–2048；全部地理项生成的腿数完整计入segmentCount，speed178只不生成腿，不断开地理链。

整个输入先经既有[PlanningWorker快照](FleetMissionPlanningWorker.h)的32 MiB／24层／100万节点纯JSON上限。随后用原ClearanceReview新增 `geometrySourceValid` 复用唯一的24 MiB合并／16 MiB Joint、资料规范化、完整Joint22／成员12／段8／来源6形状和来源规则，不复制另一套Joint解释器，也不调用名义环境solver。输入必须为 `startup-and-encoded-planning`、startupIncluded=true，原geometryBasis为小写SHA256且完全相同。规范资料仍受其4 MiB／16层／131072节点限制。

资料vehicles、Joint.memberCoverage、models必须全部同序一一对应，ID不重复；Joint须有完整encodingAvailable，每机原payloadHash等于motion source的payloadContentHash。`clearanceInputFingerprint`为完整规范资料紧凑Qt JSON的SHA256；`motionModelsFingerprint`为整个有序row2列表的完整紧凑Qt JSON SHA256。原geometryBasis仍绑定调用者同次worker输入；这些值检查不认证实际Vehicle、场景、构型、高程、时间或权限。实际来源和有效期必须由原生owner继续保留并核验。

## 全域准备，不保留可用前缀

先准备完整组所有Cover，再开始任何环境solver。单机Cover含全部原地理腿、每腿两端帽（跨反经线可拆）；每个reference box已保守包含整个独立slab，**未含机体／误差**。本模块完整加入：

- protected：水平为bodyHorizontalRadiusMm加tracking／scene／projection／safety，垂直为bodyVerticalHalfExtentMm加tracking／scene／datum／safety。
- schema2 support：水平改为supportHorizontalRadiusMm，上下分别为supportAboveReferenceMm和supportBelowReferenceMm，加相同误差，保留不对称性及显式0上方尺寸。
- schema1每box一个protected；schema2每box先protected、后support。整数reference外包已包含数值floor／ceil，不重复添加虚构物理默认余量。

整组最多32768个cover cells、65536个components；扩张后bounds必须仍在Geometry的±51000000毫米域。任何Cover的frame／投影不支持、容量超限、身份矛盾或扩张越界，都不返回已准备的前缀：成功构造一份incomplete报告，outerCoverComplete=false、comparisonComplete=false，coverCellCount/componentCount/processedComponentCount为0，workUnits和违反数为0；仍保留**全部成员及从全部原wire求出的segmentCount**。这不是空域通过，也不把失败的0当原路线为空。

取消和非法输入不同：取消或来源坏型返回 `{ok:false,error:非空,report:{}}`，包括此前已算出的候选全部丢弃。合法但不支持完整外包则 `ok=true` 仅表示完整失败诊断报告已构造，不能解释为净空通过。原Cover内部已知raw-cell计数不作为成功投影数发布。

## 真正的环境比较

完整场景凸柱／索引准备、完整候选枚举和全部components共享一份 `Geometry::Budget`：2000000 work、2048 fragments和同一个只读取消token；不按成员、cap、box、资料组或比较重置预算。

每component依次执行：整个外包是否被coverageVolumes并集覆盖；是否被corridorVolumes并集覆盖；是否被全部terrain cell的XY无界竖柱并集覆盖（由query自带完整Z Bounds限定，与逐cell附相同Z区间等价）；完整枚举原行terrain索引候选后逐一作精确SAT（在query支持域内保留 `z<=maximumTerrainAmslMm`）；同样完整枚举并实际SAT核原行障碍候选。被索引排除的凸柱已具有精确AABB分离依据，不是丢弃未知项。不是要求单一volume包含整个query，不只测端点／中心，不沿名义段高度插值，不裁去资料域外部分。边界接触算外包候选。

几何Unknown停止后续比较，comparisonComplete=false，已知候选及有界总计可以保留，但不把已处理前缀算成全组完成。processedComponentCount只在该component的全部环境检查得到确定结论后加一。已知违反不提前停止余下可计算检查；明细最多512项，violationCount仍累计全部实际已证外包候选。外包可能比真实理想tube大很多；候选只说明当前保守充分条件未满足，**不是原Qt谓词的反例，也不是实际碰撞观测**。

## 起降动作接触例外不得扩散到整个运动域

schema2复用唯一[ContactModel](FleetMissionContactModel.md)对同一完整Joint的原payload／序号／位置／地面高度绑定。ContactModel缺口或不支持则contactContextComplete=false，完整外包几何仍可继续计算。这里的true只说明原名义接触上下文可构造，不是给整域增加接触许可。

原ContactModel的有限pad-band只绑定指定起飞／落地support组件。原运行contains取所有地理腿并集，独立完整高度区间并不随当前任务序号／动作缩小；因此即使整域box携带某条腿的索引，也不能证明此时正在执行其起降动作。**本报告不聚合vehicle／terrain的pad并集，也不给任何整域support接地候选豁免。** 所有被精确证明非空的support∩terrain交集都保留候选；protected与障碍同样不豁免，Unknown不能通过。不裁掉低高度范围、不抬高slab、不修改原contains或名义ClearanceReview中的动作限定接触规则。未来若要接受合法起降接触，仍须实际动作／阶段范围证明与对应消费者，不能仅凭同机号、cellId或segmentIndex解除。

schema1没有pad豁免；本报告contactContextComplete为true只表示没有新schema2支撑绑定问题，**不替代原名义报告对TAKEOFF/LAND缺接触上下文的独立阻断**。

## report 固定28键

`schemaVersion=1,reportType="fleet-startup-domain-review",geometryBasis,clearanceInputFingerprint,motionModelsFingerprint,memberIds,memberCount,segmentCount,coverCellCount,componentCount,processedComponentCount,outerCoverComplete,comparisonComplete,declaredModelSatisfied,contactContextComplete,numericContainmentProven=false,violationCount,violations,violationsTruncated,reasonCodes,workUnits,limits,modelName,snapshotOnly=true,clearanceCertified=false,executionPermit=false,flightCommandReleased=false,actualPathCertified=false`。

`declaredModelSatisfied = outerCoverComplete && comparisonComplete && contactContextComplete && violationCount==0`，只针对此理想外包和工程声明。**不含、也不暗示numericContainmentProven**；调用方必须把原数值包含作为另外必需条件。完整StartPermit、机载实例和正式结果仍未由本模块完成。

`limits`固定8键：maximumModels=32、maximumCanonicalItems=8192、maximumCoverCells=32768、maximumComponents=65536、maximumWorkUnits=2000000、maximumFragments=2048、maximumViolationDetails=512、maximumOutputBytes=2097152。成功完整result3必须在2 MiB内；超输出预算返回失败空report，不能删必需来源凑成功。

每条违反exact6 `{vehicleId,segmentIndex,endpointIndex,componentKind,code,volumeId}`；segmentIndex为原地理腿0起序号（非协议序号），endpointIndex仅0/1，componentKind仅protected/support。反经线拆开的两个raw box可共享endpoint身份，所以不宣称该六字段是唯一cell ID。code闭集为：

- `domain-envelope-not-covered`、`domain-envelope-outside-corridor-union`、`domain-envelope-terrain-coverage-incomplete`：volumeId明确空串，因为缺口针对并集，不捏造一个volume。
- `domain-envelope-terrain-contact-candidate`：volumeId引用本资料真实cellId。
- `domain-envelope-obstacle-contact-candidate`：volumeId引用本资料真实obstacle volumeId。

接触上下文缺口用reasonCodes，不伪造某个endpoint的接触违反。reasonCodes按确定顺序输出：始终包含原Qt数值未证说明；必要时加接触不完整、外包不支持／容量或扩张域原因、几何预算或Unknown、外包候选存在、明细截断。workUnits仅本次环境几何／索引准备、候选遍历和solver实际扣除的整数工作量，不是CPU时间。

`resultMatches`重新检查完整来源、全部Cover及接触准备，核固定28字段、成员／来源指纹、counts、processed／complete关系、模型满足公式、精确limits／reasonCodes、512明细及真实component/cell/obstacle引用。不完整报告的每条明细还必须在真实处理前缀或当前component中至少有一次可能的身份出现，不能引用仅在未知停止点之后才存在的身份。它不重新运行环境solver，不认证存档结论；只有实际worker与原生owner的身份／完成／时效链才能绑定真实计算。该有界检查仍可能准备整个组，并不承诺零耗时。ok=false的失败信封不由此函数认可为成功报告。

工程师后续仍需实际构建并验证完整正向及拒绝路径、组容量、全部高度带、shape/source/数值往返、末尾成员越界不留前缀、protected/support差异、指定起降pad不豁免普通腿或整域接地、边界接触、几何预算Unknown、取消及完成发布；本轮没有执行这些验证。原Qt/libm数值包含、动作／阶段范围证明、资料／实机对应、实时障碍、物理动力学和最终许可仍需独立完成。

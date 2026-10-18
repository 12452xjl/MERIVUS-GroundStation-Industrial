# 启动净空纯值工作线程（455；467更新）

## 535 当前：同次完整异机扩张包络进入第四报告和共同准入

输入仍为schema3/exact8、成员row4和原Motion Ptr集合；结果升级为exact8，在原七键上必需加入完整 `interVehicleEnvelopeReview`，其值为 [InterVehicleEnvelopeReview](FleetMissionInterVehicleEnvelopeReview.md) 的 `{ok,error,report}`／report28。旧result7、缺报告或部分成功前缀不能作为当前结果。任何输入、取消、构造或整体预算失败都返回四报告及joint空Map，不发布可用前缀。

真实 `run()` 从本次 DomainReview.build 和 WaypointSwitchReview.build 各取得原生不可变 PreparedPtr，然后直接交给新比较器；没有从公开计数、名义中心线或报告重建少量包络。原生集合保留各自产生的完整扩张组件、所有约束面、完整有序成员和原geometryBasis／clearanceInput／models指纹。只比较异机的domain-domain、domain-waypoint和waypoint-waypoint全部组合；没有完整时序，按全空间保守比较，不套terrain接地豁免。新结论仅表示声明扩张包络的闭交叠候选，不替代原minimumSeparation检查，也不证明时序分离、真实碰撞或额外最小距离。

`resultMatches` 仍只做值对应及完整计数／shape验证：调用原两个matcher取得matcher-only集合，再核新报告来源、成员、组件及组合计数和有界冲突引用，不重跑环境／异机交叠solver。matcher-only集合不能传给实际build取得计算资格；原始native Ptr身份仍由真实owner、generation和actual finished链独立核对。

共同 `startupModelSatisfied` 增加新报告geometryComplete、comparisonComplete、declaredModelSatisfied均true且conflictCount=0条件。两Review及Witness工厂实际消费此门；新报告完整进入原完整artifact预算，不能裁掉后准入。原三报告、数值包含及 `actualPathCertified=true` 要求全部保留；当前producer实际路径仍false，新比较也固定时序／实际路径／净空认证／权限false，因此本批不能形成完整StartPermit。

本批仅源码与直接静态／普通文本核对；未编译、执行项目、helper、解析器、模型、solver、线程、QML、SQL或hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交或打包。下方为历史合同，result7及更早计数由本节更新。

## 496 当前：原第三报告实际消费完整垂直编码参考

Worker源码、schema3/exact8／row4输入、result7未改。原waypointModels的row3保同Motion source／PM29；Waypoint v5另保full current并与full top建垂直参考，LAND与TAKEOFF共用同次Contact.prepare。缺schema2／成功ref或full current超原窗口，整份incomplete无组件前缀，不把其余成员当通过。

新vertical实际走原三覆盖及terrain／obstacle检查；只有该support指定terrain完整交集在同finitepad内可例外，top→NAV和Domain不借。NAV计数／旧LAND及名义组件保留，v5和完整总计数由原实际build／resultMatches重建，旧v4不匹配。共享预算不增额，无第四报告槽、来源刷新、UI或平行授权器。

原三报告完整／零违规／声明满足、numeric及actualPathCertified=true要求仍在；当前producer实际路径false，不能形成StartPermit。来源／PM／PRIMARY／期限与原对象链不变，本几何不认证landed／动态接受／平滑／连续物理轨迹。见[Waypoint](FleetMissionWaypointSwitchReview.md)、[Contact](FleetMissionContactModel.md)及[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前：原第三报告实际消费起飞参考连接

Worker源码、输入schema3/exact8／成员row4、结果exact7均未改。原waypointModels继续把同次完整Motion source与PM29投影为row3；Waypoint v4现在从原GPI9／MIS实际REAL32／wire与Joint对应独立重算top编码区间，连首NAV完整帽，不增后采样／刷新／平行授权器或第四报告槽。

run原实际build和resultMatches都进入同一新prepare，新增reference／body/support组件真实走五类环境与原预算，v4模型名和完整总计数被原matcher绑定；旧v3不能充当当前结果。NAV计数和既有NAV/LAND定义保持，新体不借Contact旧起飞／LAND的接地例外。原全量输入指纹、原Models Ptr、原PM／PRIMARY／时间与真实worker对象链不换源／不续期。

startupModelSatisfied仍核原三报告完整、零违规与声明满足，并保整域numeric与actualPathCertified=true要求；当前producer实际路径仍false，故本批不形成完整StartPermit。new top reference不是当前landed、真正起飞接受域／垂直整域、内部平滑器或连续物理轨迹证明。planning没有原PM29，不能借此扩旧名义报告。

见[Waypoint完整合同](FleetMissionWaypointSwitchReview.md)、[Joint名义范围](FleetMissionJointSpatialReview.md)与[36项全未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方491为历史。

## 491 当前：原第三报告实际消费LAND参考体

Worker源码、输入schema3／exact8／row4及结果exact7保持。已有run真实调用WaypointSwitchReview.build，现由其v3同时计算原NAV与LAND接受参考体；原resultMatches重建同输入和几何／计数／版本，旧v2或遗漏新体的计数不能充当当前结果。不新增第四报告槽、后采样、参数刷新或独立授权器。

LandingReference由Waypoint在同次规范input／Joint内调用ContactModel.prepare获得，旧Contact components×2与原名义报告保持；新增参考体实际进入原五类环境检查，不只是多一个报告字段。StartupModelSatisfied沿原第三报告modelComplete／comparisonComplete／declaredModelSatisfied／零违规消费，Review／Witness原调用链继续。

原Domain的actualPathCertified仍false且共享门禁必须true，故本批不形成完整启动许可。声明参考体不能证明动态接受扩大、真正MOVE入口、平滑器、真实地面／时间或正式结果。planning没有PM29，不借此升级名义报告范围；原来源／期限与同native Models链不改。见[Waypoint](FleetMissionWaypointSwitchReview.md)与[未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 482 当前：schema3 exact8／row4，真实第三报告进入准入

当前输入恰8键：`schemaVersion:3`、`captureIdentity`、`plan`、`reviewEvidence`、`vehicles`、`minimumSeparationMeters`、`clearanceSourceEvidence`、`clearanceDatumEvidence`。每成员恰4键 `{vehicleId,readinessSummary,geographicMotionModel,parameterSnapshot}`；readinessSummary仍是原几何exact4，Joint名义输入仍仅row2，不把参数／诊断悄悄混进旧几何合同。

`parameterSnapshot` 必须过PM的exact6、12／2／14／1严格值codec；只解决跨JSON数值类型与RAMP负零，不能证明来源。实际owner在UI线程以同次原PM／PRIMARY弱owner核29项。原不可变Motion另传原Ptr且与完整source双向匹配；线程不触Vehicle／Link或重建实际模型。

`run` 在原名义及整域计算后真正调用 [WaypointSwitchReview](FleetMissionWaypointSwitchReview.md)，给它完整row3 `{vehicleId,geographicMotionModel,parameterSnapshot}`。结果当前exact7：`ok,error,inputFingerprint,jointView,clearanceReview,startupDomainReview,waypointSwitchReview`；任何失败三报告／joint均空，不发布成功前缀。matcher完整重建source／接受区／连接／计数和报告shape，但不独立重跑或认证solver verdict。

共用 `startupModelSatisfied` 新增NAV模型完整、比较完整、声明满足、零violation条件；原名义／整域／numeric及actualPathCertified=true要求全部保。现producer实际路径字段仍false，故本轮不会令两Review／Witness取得StartPermit。新报告动态高度接受、平滑器、真实路径与权限字段也固定false。

本批只静态修改，未编译、运行线程／模型、测试、仿真或放行；最终文本核对已记录于本批静态审计。下方schema2／row3／exact6均为历史。

## 481：新source2原对象及显式选择双向对应

输入schema2／row3与结果exact6保持；原Models继续单独持有，真实run不重建。新source2显式guard与对应plan.member.executionProfile的presence和精确值须双向一致，policy、原wire／payload hash／ceiling及baseline继续完整对应。删guard、带错profile或只补domain Map不能恢复原模型资格。

只有snapshot／resultMatches值型检查可带原profile选择重现临时Motion；DomainReview也保完整source2，不能遗漏新选择而按旧source1重建。nativeModelsMatchInput／originalGeographicModels／Witness／Txn仍延续同一实际Motion及其整数Ptr，不因matcher成功认证原来源或更新TTL。

新全整数域能给数值包含，但不证明动作／采样间路径。startupModelSatisfied在原全部名义／整域／数值条件之外，实际消费actualPathCertified=true；现producer固定false，因此两种模型仍clearance-blocked。不能删此项或把数值包含视为最终StartPermit。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 471：实际计算持有原始不可变模型，不从source12重建替代

构造函数现为 `(input, originalModels, cancellation)`，其中 `originalModels` 是 [GeographicMotion::Models](FleetMissionStartupGeographicMotion.h)，按原vehicleId保存完整1–32成员的 `shared_ptr<const ...>`。Worker先逐项构造**独立外层QMap**，只共享原工厂创建的不可变模型对象；然后以原32 MiB纯JSON运输快照及原完整input2规则校验输入，并逐机比原Ptr的完整source12与同序row3。缺少、多余、重复目标、空Ptr或错源整份拒绝；没有prepare回退或凭相同JSON替换原对象。

`run()` 再核完整原native模型集合与冻结input，实际把同一原Ptr集合交给[DomainReview](FleetMissionStartupDomainReview.md)计算。此前prepare得到的Qt段长／方位角缓存不再因worker重新构造而换成另一组数值操作数。不可变模型不是QObject、Vehicle、权限或当前来源凭证；跨计算线程共享不延长原Review／资料期限，也不代替actual finished后原生owner复核。

新增非QML接口 `nativeModelsMatchInput(frozenInput, originalModels)`：只对**已经完成快照的**固定input2核完整目标key set、非空原Ptr及source12规范JSON，不调用prepare，不认证任意caller拥有的是实际Review原对象，也不授予身份、权限或数值证明。`originalGeographicModels()` 每次逐项构造独立外层map，返回Worker保留的同一Ptr，供真实owner在actual finished时与自己的冻结集合逐ID比shared_ptr；不能用该getter或source/hash替代owner原对象及期限检查。

公共 `snapshot(input,out)`／`resultMatches(input,result)` 的值接口不改，仍可为形状／完整source12／计数检查临时prepare；普通Review使用该公共snapshot时也可能进行这种临时结构重建。**这条值校验分支不成为实际计算模型、不补回缺少的native模型，不证明缓存Qt操作数相同。** 实际constructor走独立native输入校验分支，实际run和DomainReview不经该回退。不是声称本次彻底删除所有prepare调用或已经实测CPU改善。

原schema2 input8、row3、source12、result6、source18／datum14、inputFingerprint和全部资源上限不变；原inputFingerprint仍只对完整原八键JSON计算，原Ptr不放入QVariant、JSON、QML或哈希。整域仍v2/report28/limits8，numericContainmentProven=false，原共同接纳门禁／Compiler pending不变。同对象只固定操作数来源，不解决Qt/libm接受域包含、实际动作接触、完整StartPermit或正式结果。仅源码与静态文本／空白核对，未编译、运行helper、测试或操作设备。下方较早“构造／实际计算重建source”的描述由本节区分native与value-only路径。

## 467：完整运动域报告进入原后台结果与统一接纳门禁

内部输入仍为 schema2 exact8、逐机 row3 和完整 motion source12，不修改原观察、模型、payload 或策略。实际 `run()` 在原 Joint 和名义净空结果后，将同一行明确投影为 `{vehicleId, geographicMotionModel}`，调用 [StartupDomainReview](FleetMissionStartupDomainReview.md) 检查整组理想球面走廊外包域与声明环境。整份 inputFingerprint 同时作为其 geometryBasis；不是从较晚位置、最新草稿或成功几何前缀重建范围。

当前结果升级为 **exact6**，在原五键外必需追加 `startupDomainReview` 的完整 `{ok,error,report}`；不接受旧 result5 或缺失／空的成功报告。根 `ok=true` 只表示名义和整域两报告均成功构造，不表示任一模型满足。取消、输入或构造失败返回 `ok=false`、非空 error 及 `jointView/clearanceReview/startupDomainReview` 三个空 map，不发布部分结果。结果仍受原整体快照与 owner artifact 预算约束，不能为通过预算而省略整域报告。

`resultMatches` 严格核六键、完整原输入指纹、完整 Joint 与 payload 配对、原名义报告及 DomainReview 的原输入／模型／Joint／geometryBasis 对应；它不再次求解环境。新增 `startupModelSatisfied(result)` **只允许在调用方已经对实际原输入通过 resultMatches 后使用**：名义报告的 scope／comparison／contact／model 必须完整满足且零违规，整域报告还必须 outerCoverComplete／comparisonComplete／contactContextComplete／declaredModelSatisfied／numericContainmentProven 全部真实 true 且零违规。此共享门禁由两 Review 和 Witness 工厂实际消费，不是仅供显示的诊断字段。

本批整域报告明确固定 `numericContainmentProven=false`，并保留 `original-qt-numeric-predicate-containment-unproven` 原因。理想球面外包及环境充分检查不证明现有 Qt 数值谓词全部被包含：Qt 的地球半径约定与旧混合表达式及 libm 误差界尚未被严格包络。因此带资料的两个 Review 在完整记录两报告后明确保持 `clearance-blocked`，Witness 工厂不得发布凭证；Compiler pending、原权限、最终 StartPermit、来源期限和无资料旧路径不变。不把理想模型成功解释成飞行或物理认证。

本批仅源码、直接静态阅读和普通文本／路径／Markdown／git 空白核对；未编译、执行 parser／helper／脚本／线程／UI／SQL／应用、测试、仿真、飞控修改、刷机、放行、提交或打包。下方459／458是历史合同；涉及 result5、两空 map 或仅名义接纳的表述由本467明确升级。

## 459：input2/result5不变，模型由原生Witness移交实际阶段

本Worker源码和input2 row3合同未改。新[Witness](FleetMissionStartupClearanceWitness.md)从原完整input2逐机准备并缓存同源不可变模型，原生getter经过原memoryCurrent；[Txn](FleetMissionStartupTransaction.md)核完整source后保存原Ptr，Transition实际消费，不在phase新prepare。

这个原生持有不使纯JSON成为可信来源，不改变原source18／datum14、原时钟／UTC和实际finished结果接纳，也不扩大原名义环境solver范围。完整运动域、Qt数值包络、最终StartPermit与正式结果仍未完成，Compiler pending保持。下方458合同继续适用。

## 458：完整原运动描述进入同一输入，不冒充已求解范围

内部输入现为schema2的exact8，每机row3必须同时携带原四键几何摘要与完整[GeographicMotion](FleetMissionStartupGeographicMotion.md) source12。旧瞬时schema1不默认升级，不接受缺省模型；结果exact5及原线程／取消／来源／期限合同保持。

实际名义环境solver仍只消费Joint的原几何。完整原球面走廊并集×独立AMSL区间及机体／误差包络的净空包含尚未实现，所有numericContainmentProven／clearanceCertified／flightCommandReleased仍false，Compiler pending不删。本模块和描述绑定都不是完整StartPermit、实际飞行或正式结果证明。

`FleetMissionStartupClearanceWorker` 把同一次实际普通启动审阅或地面重新审阅的环境模型计算放到独立 `QThread`。它不访问 `Vehicle`、Registry、目录、会话、时钟、地图或文件，也不发送消息、授予权限或更新审阅对象。原生来源、未续期的单调期限、UTC 水位、原审批和任务身份由调用者保留并在计算结束后重新核对。

本模块仅完成源码实现及静态读回。未编译、运行、测试或证明线程运行时行为和模型正确性。

## 输入和来源边界

`snapshot(input, out)` 先复用 [FleetMissionPlanningWorker](FleetMissionPlanningWorker.md) 的纯 JSON 快照：整份 32 MiB、深度 24、最多 1,000,000 节点和 256 字符键；拒绝 opaque 对象、非有限浮点值及不能无损进入 JSON 的整数，显式 null 保留为 JSON null。然后检查固定输入合同；任一错误整份拒绝，输出空 map，不采用有效前缀。构造函数也自行完成这一深拷贝，持有 `const` 输入值，不借调用方可变容器。

输入为 exact8：

- `schemaVersion: 2`。
- `captureIdentity` exact4：`reviewKind` 为 `normal` 或 `ground`；`reviewEpoch` 和 `reviewId` 为非空规范 UUID；`reviewGeneration` 为正的规范 uint64 十进制字符串。
- 原完整 `plan`、原 `reviewEvidence`。
- `vehicles`：1–32 行 exact3 `{vehicleId, readinessSummary, geographicMotionModel}`。ID 为 1–255，不重复，和原计划、来源证据、基准对应证据成员数量及顺序完全一致。
- `minimumSeparationMeters`：调用方显式值或未知 null，不补默认间距。
- `clearanceSourceEvidence`：453 的冻结 18 字段值副本。
- `clearanceDatumEvidence`：454 的冻结 14 字段值副本。

每行 `readinessSummary` 固定四键：`launchType`、`currentPositionAltitudeSummary`、`takeoffDerivation`、`currentPositionToFirstWaypoint`。调用方明确选取这些几何输入；worker 不默默删除额外字段，不把可选 ALTITUDE 显示诊断或 v4 动态样本纳入该几何快照。

每行 `geographicMotionModel` 是共享模型的完整source12：原GPI9、完整canonicalWireItems和payloadContentHash、原规范startupTransitionPolicy、原maximumAltitudeAmslMeters，以及固定schema／模型名／半径／只快照和false证明字段。实际构造／计算比同一原工厂Ptr的完整描述；公共value-only snapshot／matcher才临时以共享prepare重建完整描述并比较规范JSON。未知键、坏型、null数字、缺失字段、hash不符或任何固定声明不符整份拒绝。每机1–2048项、整组最多8192项、每份描述4 MiB；不截取地理子集或成功前缀。

描述中的policy须和同一原plan member.executionProfile.startupTransitionPolicy规范JSON相同；高度上限须为同一constraints.maxAltitudeMeters真实有限数值。原GPI的E7经纬与currentPositionToFirstWaypoint原from字段对应，AMSL毫米除1000与原reportedAltitudeAmslMeters相同；完整payloadHash再与Joint整计划memberCoverage按顺序配对。这些都是值对应，不认证实际来源或模型真值。

普通Review从同一次原Readiness capture的flightStateSummary取GPI，策略从同一原member的规范化reviewedProfile取值并和审阅行共用；不能误读仅captureForStartup才有的字段。地面Review只取原completed READ currentRisk及同一target.payload，不另取最新位置或可选诊断。原native source18／datum14和时钟仍由owner保留。

值边界检查保留 source18/datum14 的固定声明和 false 权限语义、相同输入指纹及逐机配置证据一致性。内部原生证据的 JSON 副本只是计算输入和身份材料，不重新认证来源、代替 live native 对象或恢复审批。调用者必须继续保留原 native 类型的 18/14 字段证据，而不是把线程 JSON 副本回填到原生门禁。

## 计算与输出

`inputFingerprint` 是整个脱离来源的输入紧凑 JSON 的 SHA-256，包含本次审阅 epoch/generation/id。它作为 `geometryBasis`，不包含最终净空报告或最终风险 artifact，因此没有自引用哈希。它不是来源认证。

`run()` 只调用一次 `Joint::buildStartup(plan, reviewEvidence, startupSource4)`；显式将row3投影为其原row2，仅传vehicleId和原四键readinessSummary。完整source12不作为额外名义线段。该入口内部生成启动空间投影并对整计划编码一次，完整memberCoverage.payloadHash必须逐机对应source12。随后分别调用 `ClearanceReview::buildForGeometryBasisCancelable` 和整域 `StartupDomainReview::build`；前者保留原起飞假设、LAND 接地、机体保护和支承语义，后者使用完整 source12，不把规划阶段结果或名义段通过当作整域或 actual-startup 数值谓词通过。

`finishedResult()` 固定 exact6：

| 字段 | 合同 |
| --- | --- |
| `ok` | 计算成功生成完整两份报告，而非净空通过或启动许可 |
| `error` | 成功为明确空字符串，失败为非空有界原因 |
| `inputFingerprint` | 有效输入的 lower SHA-256；构造输入无法形成有效快照时为空字符串 |
| `jointView` | 成功的完整 geometry-scope view22；失败为空 map |
| `clearanceReview` | 成功的完整 `{ok,error,report}`；失败为空 map |
| `startupDomainReview` | 同源整域的完整 `{ok,error,report}`；失败为空 map |

取消、输入错误、几何来源错误、报告错误或结果预算超限全部丢弃 joint/report，不发布部分成功前缀。成功报告的 `clearanceInputFingerprint` 必须对应冻结 source18 中的输入指纹。原报告中的 `actualStartupReviewed`、`clearanceCertified`、`executionPermit` 和飞行释放字段不被 worker 改为 true。

调用方不能只检查 worker 的 `ok`。必须区分 `scopeCoverageComplete`、`comparisonComplete`、`contactContextComplete`、`declaredModelSatisfied`、实际violation数、未知／超预算结果，并结合原native门禁和本次身份尾核。worker不扩大各owner原预算：普通完整artifact 2 MiB；地面内部artifact 32 MiB、公开riskSummary 2 MiB。完整必需材料放不下时必须明确阻断，不截去报告后批准；名义scope完整也不证明整个允许的运动域完整。

## GUI 回收验证与生命周期

`resultMatches(input, result)` 是只值验证：重新快照、核原完整运动描述、计算原输入指纹、轻量提取同一完整Joint及完整payload配对，核对其完整JSON，再使用 `geometryResultMatches` 核对报告形状、输入和分量身份。它不重跑环境可行性／union solver；仍包含有界JSON、共享模型端点准备、完整任务编码提取和接地分量准备，不承诺零耗时。既有[原生Witness](FleetMissionStartupClearanceWitness.md)沿此路径保原input2和result5，不从JSON恢复可信源。

该方法也接受与有效输入指纹一致的完整失败信封（非空错误、三个空 map）。返回 true 不表示 `ok`、模型满足或来源仍然有效。失败结果不重提取几何。原输入本身不合法时返回 false。

生命周期沿 451：

- worker 无 QObject parent；`Cancellation` 是共享 `std::atomic_bool`，owner 只允许 false→true，不复位复用。
- owner 连接实际 `QThread::finished` 的 queued 回调后才读结果；连接独立的 `finished → deleteLater` 负责最终清理。
- owner 销毁/退休时置取消并断开其回调，不在 GUI 析构中无界 `wait()`，不删除运行中的 QThread，不把取消请求当作线程已经结束。
- 新任务是否可开始由 owner 的实际线程排空状态决定，不在 worker 自动排队或重试。
- 计算入口、Joint 返回、solver 返回和结果快照后都检查取消；solver 内部沿共享 token 协作取消。JSON、规范化、编码等阶段只在边界观察取消，不声称固定最大取消延迟。
- owner 收到实际 finished 后仍须核原对象、epoch/generation/id、原证据、UTC/单调期限和当前门禁；旧结果不能恢复被取消或退休的审阅。

## 工程师后续验证

需要实际编译并覆盖：普通/地面各自正向结果、同代不同 reviewId、计算中取消及 owner 销毁、完成通知中的重入、原源先坏后恢复、等待期间资料过期/UTC 回退、完整结果超原 artifact 预算、多成员顺序差异、未知间距、无接地声明、schema1/2 环境输入及 JSON 数值/null 跨线程保真。当前没有执行这些验证。

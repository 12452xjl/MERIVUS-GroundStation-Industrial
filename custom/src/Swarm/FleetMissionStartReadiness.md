# 原生启动前置条件观测（不授予启动许可）

## 536：逐航点朝向编码与原生消费者接线

使用实际 payload.wireSchema，在首次 nativeEvidence 及最终 finalEvidence 两处调用同一当前目录 acceptsWireSchema；旧 v2 声明不得满足 heading-tick v3 的前置证据。首尾都核与真实版本匹配返回的 declaration 及当前目录身份，不靠 UI 标签或历史声明升权。

共用 captureReadiness 同时覆盖普通启动与地面受控阶段，输出形状不变；原 payload 完整重建比对、参数／版本／原生对象与期限检查保留。该声明接纳不是实际固件／二进制认证、朝向达成、净空或最终启动许可。

共享合同见 [HeadingEncoding](FleetMissionHeadingEncoding.md)。本批仅源码修改、直接静读与普通文本核对；未编译、运行实现／解析器／算法、测试、仿真、硬件操作、修改PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 482 当前：同次原29参数冻结进真实审阅输入

私有 `ExecutionParameterSnapshot` 保最初 Vehicle／PM／links／PRIMARY弱owner及12／2／14／1四组行。`parameterSummary.arrivalEvidence` 保完整稳定三键到点证据；只有同次snapshot已captured时才输出 `originalExecutionParameters` exact6：`schemaVersion:1`、`legacyRows`、`rateGuardRows`、`actionPhaseRows`、`arrivalRows`、`takeoffRampNegativeZero`。各组固定独立排序、稳定行各14字段，只去age；负零marker由同次原RAMP行的signbit生成，不重采样。

新增完整快照进入普通及启动风险身份，交给两Review冻结到Worker的 `parameterSnapshot`，不是调用者填写的半径。普通finish及startup phase最后回调之后均核原29行；失败清全部源／前置／phase资格及 rate／action／arrival 三个嵌套证据的 `usable`／`sourceCurrent`。仅保诊断值，不重新capture覆盖原样本。

PM值codec只恢复有界形状和RAMP符号；真实来源仍须以原PM、原PRIMARY控制块、原epoch／revision／Fact／pending／期限作29尾核。公开快照、相同hash或JSON可读值不能自行取得原生凭证。旧12hash、旧观察口与原takeoffDerivation／策略合同不变。

本批仅静态源码／材料工作，未编译、运行、测试、仿真或放行；最终文本核对已记录于本批静态审计。固定半径及新几何审阅不认证动态高度接受、真实轨迹或StartPermit。下方为历史。

## 480：原28参数贯穿普通及启动phase回调

私有ExecutionParameterSnapshot保最初Vehicle／PM／links／PRIMARY弱owner及12／2／14三组行，不序列化原生引用；从同一次Evidence/schema4保原行，仅去age。parameterSummary独立保完整stable actionPhaseEvidence，与旧rateOverrideEvidence一起进入sourceFingerprint及现有普通／地面风险身份。

实际nativeEvidence消费parameterEvidenceUsable，因此明确LAND的RC辅助、起降外部setpoint或新动作参数失配阻conditions和启动候选，不由新model.ok获得飞行许可。普通finish在高度诊断等最后回调之后、captureForStartup在EST／HOME／ODOM／origin／高度等之后，均仅对原28行作memory fence。失败清source／preconditions和phase／deferred资格，嵌套两个证据usable／sourceCurrent也置false；不重capture替旧源。

原exact27 takeoffDerivation、Contact几何／接地例外、六flight／startup观察、profile schema与运行策略不改。没有当前dist_bottom／控制器阶段／sticks／dt，不从GPI推造它们；完整动作域、数值包含、动态净空及StartPermit仍未完成。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：原十四参数快照经过全部回调后的内存终检

普通 capture 仍消费原生 VehicleEvidence 的完整 `parameterEvidenceUsable`、真实版本观察及本地固件声明对应，不把旧十二参数 hash 当作两个宏已被绑定。新增 `parameterSummary.rateOverrideEvidence` 保存本次 guard 与源码模型复核；guard 行只去掉推进中的 `ageMs`，其余鲜度、来源、epoch/revision和模型不删，因此不会因正常接收年龄增长而无意义地改变阶段身份。

cc 私有 `ExecutionParameterSnapshot` 冻结第一次实际采证的原 Vehicle/PM/link-manager、PRIMARY 地址/weak owner 和十二行＋两行 guard 的稳定行。它不序列化到公开结果，不新增可伪造的许可字段，也不改变公开 capture 签名。普通 capture 的 `finish` 在高度关系与最后高度诊断全部返回后，以 PM 的 callback-free 整组 API 核原快照；失败撤销 source/current 与所有普通/受控解锁/地面模式候选，加入有界阻断原因。不会以同 ID 新实例、不同 PRIMARY、重采样或更新后的参数行代替原依据。

`captureForStartup` 复用同一私有快照，在 phase EST/coordinate/ODOM/HOME/origin/ALTITUDE 获取与计算之后，再作原十四行内存尾核。失败清空 deferred action，令 `startupPhasePreconditionsMatch=false`；原观测值和阻断记录保留。所有真实许可/认证 flags 仍为 false。当前普通/阶段判断均不因新模型而放宽原 v1–v4 政策、已存在的估计器/原点/高度门槛。

这只是 capture 返回前的有界一致性，不证明两次 capture 之间或全飞行阶段的每次参数变化均被记录。宏对应检查不等于内部有效速率、飞行性能、已编码垂直命令、完整时间计划或动态净空已获证明。未编译、运行 helper/脚本、测试、仿真、启动应用或操作硬件，不修改 PX4。下方为历史记录。

## 478：单机编码复核保留同一完整原域策略

实际输入payload必须携canonical完整 `routeBoundaryPolicy`，先用Planner严格验证；本地inputHash同时包括member／constraints／该policy。synthetic单成员validationPlan明确保同policy并调用严格normal Builder，不从缺失区域补inactive，也不借历史复现口通过当前前置检查。

完整expected payload固定字段逐项比较包含policy；整组检查统计仅在外层build结果，不放每机payload，因此真实单机重建不需要豁免新字段。既有coverage及unencodedRequirements的专门核对、native Vehicle／参数／链路／估计器及最终来源检查全部保留。

这个本地重建只核输入内容和编码，不能认证policy原作者、完整fleet批准、动态净空、原数值域或StartPermit；实际完整plan仍由调用者native审阅链绑定。仅静读未编译、运行或测试；下方为历史。

下方为历史记录。

## 445：显式 v4 高度关系政策与首次基准

只有规范化 `startupTransitionPolicy.schemaVersion=4` 才要求此依据；v1–v3 不新增 ALTITUDE 字段或依赖。v4 沿 v2 的运动／航向包线，HOME 变化仍停止并要求重新审阅，不采用 v3 的 HOME 更新分支。

普通 `capture` 在原最终参数／flight／EST 核对之前，从本次实际 Vehicle、原 PRIMARY、原生 ALTITUDE27、已观察的 GLOBAL_ORIGIN18 和 coordinate-HOME9 初始化一次基准。不是读取444可选诊断。结束时只作原生内存 ALT／origin／完整 HOME 尾核；`captureForStartup` 复用此次原样本，所有新增可回调 coordinate 获取先于原最终 EST／origin 核对，最后只用内存方法核来源及关系，不能以新样本替代。

v4 新增六键：`startupAltitudeReferenceObservation`（稳定20键）、`startupAltitudeReferenceSample`（必需三高度及差值／来源20键）、`startupAltitudeReferenceRiskFingerprint`（仅稳定 reference）、`startupAltitudeReferenceRequired/Captured/Matches`。它们是强制风险材料，不使用可选诊断剩余预算。sample 保留首次接收身份和原值；仅 sample 从 phase identity 和当前／冻结风险身份投影排除，但仍须另对首次 sample 比容差。reference、政策、来源／时钟变化及匹配状态不能被排除。

六个显式非负米制容差见[纯高度关系 helper](FleetMissionStartupAltitude.md)，零合法、不补默认值。必需 AMSL/local/relative 三高度及三差值须在既有 JSON 数值支持域内；无关 terrain、bottom-clearance、monotonic 不作为此政策门槛。缺报文、必需字段未知、原点／HOME不完整、过龄、失源或越界使显式 v4 候选不成立，不认证内部 valid、共同原点、跨消息同样本或最终 StartPermit。原444诊断独立保留；未编译、测试或运行。

## 444：冻结的 ALTITUDE 高度诊断，不增加启动门槛

普通 `capture` 的 `finish` 在所有先前回调返回后，以本次已核对成员／载荷 ID 的实际 Vehicle 和原 PRIMARY，通过纯内存 `startupAltitudeObservation(3000)` 与 `startupAltitudeObservationSourceMatches` 取得一次诊断。输入／目标尚未验证、对象／链路失源或原生形状不完整时，仅显示未知及原因，不从 Fact、历史同 ID 或其它流补值；正常 `captureForStartup` 继承该快照，不另采样覆盖。

`startupAltitudeDiagnostic` 固定11键：`schemaVersion/vehicleId/componentId/observation/captured/sourceCurrent/captureReason/snapshotOnly/diagnosticOnly/executionPermit/flightCommandReleased`。schemaVersion=1，未知 ID=0，observation 为实际原生27键或空 map，后四项固定 true/true/false/false。captured 只指通过严格结构和显示预算、保留了该原生封套，不等于 observed、available、fresh 或六个高度均已知；captureReason 为有界固定原因，正常已观察／可用／接收年龄新鲜时为空字符串。

原生 value 经 [FleetStartupAltitude](../../../src/Vehicle/FleetStartupAltitude.md) 的 `stateProjection` 严格复核 decoded18；保留原始字段状态、原始 bit 信息和同包算术差值，未知不变成0。诊断独立限制 depth≤8、节点≤256、compact JSON≤16KiB，所有真实数值（包括高度、差值和本机时间）须符合原风险材料的 `abs(value)≤2^53−1` 范围。原生合法但超显示数值预算时，observation={}、captured=false、captureReason=`native-altitude-diagnostic-numeric-budget`；结构／字节超限同样只丢该诊断。不会夹紧、字符串代换数值、放宽旧材料预算或使其它原风险／审批因为该附加诊断而不可用。原生账本仍保留其完整值。

完整诊断不进入 `sourceFingerprint`、`startupPhaseFingerprint`、blockedReasons、任何 preconditions 或 deferred action；缺 ALTITUDE、字段未知、过龄、时钟回退／源变化均只在诊断中说明，不新增 v1/v2/v3 政策或许可门槛。普通 Authority、地面 Authority 和 GroundReview 的当前／冻结 readiness 比较显式排除此字段，冻结材料本身仍保留当时值。

完整 Readiness 本身提供11键诊断；下游普通／地面风险材料先满足原完整风险预算，再用剩余容量逐机纳入整份诊断。下游可能省略该可选键，含义是“未提供诊断”，不是正常或零值；不因此丢其它风险字段、成员或让原风险变成不可用。

fresh/age 只描述捕获时本机收包年龄，不证明传感器样本新鲜或当前实时高度。clockDiscontinuityObserved 描述当前来源序列；clockDiscontinuityRevision 可跨序列累计，false不要求累计值归零。此诊断不证明 ALTITUDE 与 GPI/HOME/LOCAL/ODOM 共原点、PX4 内部 valid、跨消息同样本、净空或许可；不用于替换原起飞高度推导、原点参考或 EST 门槛。仅源码与静态读回，未编译、测试、运行。

## 443：被动 GPS_GLOBAL_ORIGIN 声明参考

`captureForStartup` 从实际 Vehicle 的原生24键观察，经 [GlobalOrigin helper](FleetMissionStartupGlobalOrigin.md) 生成稳定18键，增加 `startupGlobalOriginReferenceObservation/ReferenceRiskFingerprint/ReferenceCaptured/ReferenceMatches` 四项（均带完整 `startupGlobalOrigin` 前缀）。它们进入原阶段指纹；无论已观察或真正缺席，来源检查均使用实际 PRIMARY，尾部在其它回调取证之后再次捕获同一稳定参考，不以新值覆盖原快照。缺席仍不等于原点有效。

事件式原点不使用3秒周期鲜度；参考 timestamp、三个 epoch 和单调 revision 仍保留。只有原生确实 neverSeen、完整空值／零计数的缺流分支兼容旧 v1/v2/v3；看到过报文之后的错误、失源、越界或 ABA 不能退回缺流。失败只补充阻断原阶段候选和 deferred action，保留原许可 false、原 blockedReasons 及两项不认证本地参考／同样本的声明。普通 capture、原六 flight、LOCAL/HOME 两通道、ODOM 和政策 schema 不变。

这提供原点声明来源连续性的源码检查，不认证 HOME/local/ODOM 共原点或 PX4 内部 valid 分支，不补请求、不接 ALTITUDE、不授最终 StartPermit。未编译、测试或运行任何程序／设备。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 320–324：schema 3 明确 HOME 观察基线

规范化过渡政策的 schema 3 继承 v2 全部运动/heading 和 ODOM NED/NED、known frame、显式 reset/estimator、fresh age≤policy.maximumRawGapMs 前置，不会落回 v1 的可缺流分支。`startupMotionReferenceRequired/Matches` 保持原字段，原 schema 1/2 语义不变。

`captureForStartup` 对 v3 另以当前真实 Vehicle 的 HOME 地理三字段、补充 HOME 九字段和 global risk 九字段调用共享[HOME helper](FleetMissionStartupHome.md)的 `initialize`。要求实际当前 PRIMARY 的 coordinate 源、HOME/global 观测均 observed/available/fresh，local XYZ 已知且为有限 float32、HOME 更新时间已知且为正规范 uint64、两套 HOME 地理值一致。helper 不把时间戳当作跨消息同样本或相同时钟证明。

基线缺失、未知、过期或不一致时，保留原始 source-known/unknown 和原 `sourceEvidenceComplete` 含义，仅令 `startupPhasePreconditionsMatch=false`、清空 deferred action，并在既有有界 `blockedReasons` 加入 `schema3-current-known-timestamped-home-observation-basis-required`。继续保留原阻断原因；超限沿用 `reasonsTruncated`。没有新增顶层 HOME 许可、known 或恢复字段；完整政策及结果继续进入原 phase fingerprint。

这只是显式观察政策的 admission 基线，不是 HOME/z 内部有效分支、origin/北向、样本对齐、净空、动力学或最终 StartPermit。新 HOME/相对高度过渡仍必须经实际 startup transaction 和完整有序 journal 消费；候选 bool 不能代替它们。未运行 helper/API、编译、测试、仿真或硬件。本节以下带旧批次号的状态为历史记录。

280–284 新增[显式局部运动与航向观察政策](FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

## 275–279：被动原生参考观测

280–284追加startupMotionReferenceRequired/startupMotionReferenceMatches：只在规范化startupPolicy schema2请求独立观察政策时要求真实ODOM NED/NED、known frame、显式resetCounter、明确AUTOPILOT8和原生age≤其maximumRawGapMs；这两个bool随原实际依据与READ物理冻结保存。schema1不额外要求这一周期条件，原optional/everObserved与valid/fresh补充gate保持。它们不是两个uORB流同原点/同样本证明，也不是独立StartPermit；新增旗标同时收紧阶段候选和deferredStartupAction，不删原blockedReasons或许可false。

`captureForStartup`额外读取实际Vehicle的`startupOdometryReferenceObservation(3000)`，原生21字段去除推进中的`ageMs`后保存20字段`startupOdometryReferenceObservation`及独立`startupOdometryReferenceRiskFingerprint`。不复制full27里的样本时间、运动数字、完整协方差、收包序号/原始hash到阶段身份。reference42只含frame/reset/estimator/quality分类、明确presence与未知/无效状态；普通quality 1..100分值不进阶段身份。`startupOdometryReferenceCaptured`只表示原生形状与域匹配，`startupOdometryReferenceMatches`只表示当前补充观察没有本模块发现的缺口；两者均不是最终启动许可。

未出现过实际源报文的optional流保持`observed=false/everObservedRawFrame=false`、empty projection/hash、available/fresh=false；不请求流，不把缺流当参考有效。实际来源核验后收到任何报文（包括无效包）会置native sticky everObservedRawFrame，之后row失效或被清空不能重新进入初始缺流分支；已有流必须实际PRIMARY来源、fresh≤3000ms、reference hash匹配、observationValid=true且observationInvalid=false。稀疏covariance NaN可与valid共存，不能只按observationNonFinite拒绝。恢复需要新的真实可用报文和全新依据，不恢复旧epoch/revision/批准。

补充gate只收紧原`startupPhasePreconditionsMatch`与deferredStartupAction，原manual-arm风险、blockedReasons、capture()语义与所有许可false不删。`phaseDoesNotCertifyLocalReferenceValidity=true/phaseDoesNotProveCrossMessageSameSample=true`明确保留PX4内部有效分支、origin、北向、跨流同样本缺口；合法HOME重置/运动与转向政策仍待开发。详见[原生契约](../../../src/Vehicle/FleetStartupOdometry.md)，未编译/运行/测试。

## 244–249：坐标与过渡政策

captureForStartup保存真实补充LOCAL/HOME RAW、风险指纹及原规范化过渡政策。phase身份仅排除补充账本的age/receipt/raw-payload传送身份与LOCAL timeBootMs，保留unknown/fresh、源epoch、risk hash/revision及HOME更新时刻；完整RAW仍另存。startupConfigurationFingerprint只将armed/flying交由typed阶段另核，其余原参数/版本/目录/源和完整profile不删除。原manual-arm阻断及所有认证/许可false保持；[阶段观测](FleetMissionStartupTransition.md)必须独立查真实queued步骤与历史，不凭候选bool发送。合法HOME重置再审、转向、净空和完成仍待开发；未编译/运行/测试。

## 当前 239–243：分阶段的声明域候选

新增 native `captureForStartup(Vehicle*, member, constraints, payload, Phase, catalog)`，`Phase` 仅支持 `BeforeArm` 与 `ArmedGroundReady`。它重用完整原生输入/参数/版本/风险双哈希/真实机位与起飞高度推导，另外区分地面未解锁与已解锁但仍地面、尚未进入 Mission 的两种本地观测。未知 enum、缺失/过期、输入截断、任务/时序/高度/版本/参数缺口均不返回候选。

`controlledArmPreconditionsMatch` 用实际 typed armingDeferred 与**所有其它真实 conditions**推导，不过滤或删除 `blockedReasons` 中的字符串，也不把原 `preconditionsMatch=false` 和 manual-arming 阻断改为已解锁。候选仅认 fresh RAW 普通 PX4 POSCTL main=3/sub=0、custom-mode标志、无AUTO/HIL、已知STANDBY/ACTIVE、真实ON_GROUND和not-flying，takeoff请求与其余声明域边界通过；Orbit、Mission/Takeoff/RTL/Land、Offboard及未知模式不能混作同一入口。`ArmedGroundReady` 另要求实际原preconditionsMatch与无deferred arming。

`startupPhasePreconditionsMatch` 与 `startupPhaseFingerprint` 是新原生阶段事实；`deferredStartupAction` 只描述未来普通 ARM(param2=0)，不是命令、不允许force2989、不接管其它飞行模式。阶段明确 `phaseDoesNotProveNeutralPilotInputs=true` / `phaseDoesNotCertifyMotorOrGroundClearance=true`；本模块没有RAW遥控中立/油门、完整机载HealthAndArmingChecks/grace期、地面人员或电机净空证明，绝不称 `firmwareCanArm` 或最终安全启动许可。

Review 单独汇总 `allControlledArmPreconditionsMatch` 和每机候选，原未解锁/未知/第二人缺失、未认证风险原样保留。新 [StartupAuthority](FleetMissionStartupAuthority.md) 只能接管真实一次账号与完整来源依据，后续真正命令事务仍需普通ARM拒绝处理、seq0原始观测、mode-only适配和允许差异的精确状态过渡；不能复用旧插件或把MISSION_START当纯切模式。本批未发指令、编译、测试或运行接口。

## 当前 229–233 契约增量

本模块改用真实 `Vehicle::flightStateRiskObservation()` v2，并不是删掉 raw 证据检查或新增 ready 许可。v2 保留完整 raw `value/contentHash/contentRevision/receiptSequence`，附加原生 `riskContentHash/riskContentRevision` 与固定 `riskIdentityPurpose=risk-content-not-reception-sequence`。仅 `GLOBAL_POSITION_INT` 风险 value 去除 `timeBootMs`，其他物理字段不删。原生风险 revision 不回绕，内容 A→B→A 仍改变风险身份，不能用恢复到相同 hash 冒充“未改变”。

`stableFlight` 先核对严格形状、实际源、raw value hash、age/fresh 边界，再重新计算风险 value hash 并核对原生 risk hash/revision，最后才将风险 hash/revision 作为投影身份并去除 `ageMs/receiptSequence`。实际机位/高度、航向/已知标志、速度、armed、HOME/landed/任务字段、observed/available/fresh 和未知状态仍保留。最终重捕获比较的是该风险投影；同风险的例行收包或正常 boot tick 不自动导致重审，但物理风险、来源/epoch、未知/fresh 边界和其他原生证据改变仍拒绝。

Raw `Vehicle::flightStateObservation()` v1 未删减：仍保留原 `timeBootMs`、原始收包序号和 source 检查。同源 GLOBAL_POSITION_INT 的有界 boot-time 回退仍保守撤销版本/参数/物理证据；可能是重启，也可能是迟到/乱序包，不能称作机载重启认证。常量零和自然 uint32 wrap 不被伪装为已认证启动周期。

这些观测为审阅模块的原生 17 字段 `conditional-human-context` 提供当前风险来源，不延长原审阅 30 秒；人工 ack、后续独立审批提示和消费不得自动续期。健康 gate/具体 Leader 由实际审阅源重验，例行 HealthChanged 本身不是新的风险。独立 Startup、本次新鲜机载内容/序号绑定、净空与执行完成仍需后续独立链路；reached 观测依然 unbound/no-completion，不是 execution proof。本轮只更新源码说明，未编译、测试或运行程序/API。

`FleetMissionStartReadiness::capture(Vehicle*, member, constraints, payload, catalog)` 是非 QML 的 C++ 静态只读接口。它从实际 `Vehicle`、其实际 `ParameterManager`、连接对象和可选原生工程声明目录重新取证，返回当前物理状态与显式任务请求之间的前置条件对照。

没有参数请求、刷新、写入、上传/删除任务、模式切换、解锁、启动、回滚、文件读写或事件循环。没有可伪造的“ready=true”。本轮只有源码与静态检查，未编译、测试、运行程序、仿真或读取真机。

## 结果含义

- `sourceEvidenceComplete`：当前原生参数/版本/工程声明对应关系，以及所需 heartbeat/globalPosition/landedState 源证据完整，并在结束时重新捕获确认。工程声明对应不等于固件、硬件或二进制认证。
- `preconditionsMatch`：本模块实际检查的请求一致性、源状态、解锁状态、起飞/空中入口及高度边界当前匹配；不是所有执行审批、健康、净空、时序或任务完成证据都已经通过。
- `needSeparateManualArming`：takeoff 分支已观察到地面状态但尚未解锁。仍返回预审源与高度推导，但 `preconditionsMatch=false`，明确阻断真正启动。必须由独立操作完成解锁，再重新捕获并取得全新的启动授权；不能复用地面上传审批。
- `blockedReasons` / `unverifiedRequirements`：明确阻断与尚未认证的要求，各最多 32 条、单条最多 256 字符；超出时带遗漏标记和 `reasonsTruncated=true`。另以最多 32 条的 `sourceUnencodedRequirements` 原样保留输入 payload 的未编码要求，不因已有局部观测而静默删除原需求。
- `takeoffDerivation`、`currentPositionAltitudeSummary` 与小型 `flightStateSummary`：当前报文取证及局部数学推导，始终不宣称实际航迹、地面高度或净空已经被证明。

`startupPermitGranted`、`executionPermitted`、`executable`、`flightCommandReleased`、`executionVerified`、`firmwareAdapterCertified`、`signatureVerified`、`authenticated`、`actualTakeoffPathCertified`、`landingClearanceCertified`、`missionCompleteVerified` 永远为 false。即使前两个观测 bool 为 true，依然不能据此发启动命令。

## 请求一致性与资源边界

member、constraints、payload 均为实际有界 map，每层最多 64 个字段；深度最多 12、节点总数最多 262144、紧凑 JSON 输入总量最多 8 MiB、字符串最多 2048 字符、每个列表最多 2048 项。源码任务/返回路线和全部 speed/takeoff/terminal 任务项仍受真实 Builder 的合计 2048 项约束；不接受无限大 metadata 或一批失败后输出已通过的前缀。字段数字必须真实有限数值，bool 和字符串不能冒充数值。

明确规范化 member 的 executionProfile；无 profile 的几何暂存仍可存在于部署模块，但不能在本模块冒充完整启动意图。要求 payload/member/actual Vehicle 的 system ID 同一，constraints 明确 AMSL 最高高度。复用现有 `FleetMissionPayloadBuilder` 以原 member+constraints 重建单机请求：raw items、canonical wire、哈希、sourceItemMap、编码 flags、完整 metadata 等与输入 payload 比对。

内部单机校验 envelope 的摘要仅用来重用确定性编码器，**不是计划审批身份**，不会替换真实 planId 或宣布全舰队风险已经审阅。全局 coverage 不在此 API 的参数内；payload 中的 coverage 必须保持严格形状、唯一成员 ID、WGS84/AMSL、真实速度/高度、参与者绑定及 profile 无冲突，并继续保留全局未认证要求。原始未编码要求必须至少包含当前真实编码器输出的全部条目。

绝对时序、同步启动、显式源时间表/截止时间及 RTL 未编码缺口阻断 `preconditionsMatch`，不能以相对 dwell 删除它们。vehicle-parameters 垂直策略的真实参数/requiredProfileHash 匹配只提供实际配置证据，不是 DO_CHANGE_SPEED type 2/3 垂直指令，也不是动态飞行认证。

## 原生物理取证与最终重捕获

调用实际 `Vehicle::flightStateRiskObservation(3000,5000,60000,10000)` v2：位置最长 3000 ms、heartbeat/landedState 最长 5000 ms、HOME 最长 60000 ms、missionCurrent/reached 最长 10000 ms。每个固定通道先检查真实 bool observed/available/fresh、年龄、精确来源、规范十进制 raw contentRevision/receiptSequence、实际 value 字段类型和紧凑 JSON raw SHA-256；observed 的 revision/receipt 必须为正。随后独立核验去除 GP `timeBootMs` 后的 risk hash 及原生 risk revision，不能用 caller 自造风险摘要绕过完整 raw 检查。根 sourceEpoch 为实际无括号 UUID。

所需启动物理证据是 fresh heartbeat、globalPosition 与明确 landedState；heartbeat armed/baseMode、autopilot/type 必须与实际 Vehicle 状态一致。HOME 没收到或过期时明确保留缺口，不伪造默认 HOME，也不用 HOME 代替 ground 分支的当前全球高度。missionCurrent/reached 为独立可未知的观测，不把缺报扩展字段补成“正在执行/已完成”。`GLOBAL_POSITION_INT` 经纬度 0,0 不统一判无效；GNSS、估计器健康和位置真实性仍需独立可信证据。

另调用 `FleetMissionVehicleEvidence::capture` 获取实际 12 项参数、当前版本报文与目录对应关系，不相信传入 options 或缓存显示。结束时对同一 Vehicle/PM/link/catalog 重新捕获参数/版本/目录与六个物理通道，复核 actual object、ID/component、线程、连接、初连状态、source epoch、原生内容/风险身份，以及参数/版本/声明对应的三个实际可用 bool。对象被替换、manager epoch/报文源变化、实际风险内容或风险 revision 变化、参数写入或未知/fresh 年龄边界失效时保守失败；不接管相同 ID 的新对象，不仅因相同风险的新 receipt 而失败。

`sourceFingerprint` 的物理通道部分使用经双 hash 检查的风险投影，去除推进中的 age/receiptSequence，并用原生 risk hash/revision 代替 raw 内容身份；保留源 epoch、当前实际物理风险、未知/fresh 状态、原生参数/版本/目录身份、payload 请求与约束/未编码要求摘要。因此相同风险的例行报文不会仅因计数/boot tick 推进而换指纹，物理或证据风险改变仍改变指纹，**不是规划审批、持久放行令牌或人工启动授权**。只输出小摘要，不复制全任务项、整个目录或所有参数到每架机历史。

## takeoff 与 none 入口

takeoff 必须有 fresh `ON_GROUND`。规范 wire TAKEOFF 项与原 profile/首 outbound 点的对应仍由现有 Builder 严格检查；不创造未声明任务项或连接航点。允许地面 disarmed 预审并计算推导，但明确要求独立 manual arming，不能自动解锁。

本地 PX4 `Mission::calculate_takeoff_altitude` 的 ground 分支是 `max(item AMSL, current global altitude + MIS_TAKEOFF_ALT)`；它先在当前实际纬经度垂直起飞，再去请求点。in-air 的 home+min 分支不能拿来当作 ground 分支。

模块以真实 canonical wire `param7`、收到的 global AMSL mm 和真实 MIS_TAKEOFF_ALT 构造 float32 **nominal** 推导。`GLOBAL_POSITION_INT` mm 是量化后的遥测，不能重建导航器内部精确 float 高度。因此输出 `telemetryAltitudeResolutionMeters=0.001`、`nominalEstimateOnly=true`，给出 ±1 mm 及 float32 `nextafterf` 边界包围的当前高度/预计起飞高度区间；以保守预计上界核对 AMSL 最高高度。非有限、超范围或上界超限均阻断，不夹紧或编造高度。

区间只处理本次遥测编码分辨率和 float32 边界，**不包含传感器偏差、估计器误差、报文延迟中的位移、风、动态状态、地形或障碍净空**，不是完整实飞误差预算。nominal、bounds 和 predicted 不得在界面改名为“已认证实际目标”。输出当前 E7 位置的垂直段及其后到请求第一点的连接段；分别保留当前位置与 profile 原始 XY / wire XY 的差异，不能隐藏 E7 量化后看似相同的原始坐标差异。

none 只允许 fresh armed + `IN_AIR`，不能冒充起飞。它同样检查当前收到的 AMSL 是否落在支持域及保守最高高度边界内，不能用路线终点/首点合法来抹去当前位置超限。当前机位到第一航点的连接只显示遥测几何，净空依然未认证。

## 结束动作与完成边界

LOITER_UNLIM 无限保持不会自行完成任务；一个 waypoint/reached 收据不能把它变成“已完成”。本地 PX4 LAND 的 move-to-land 阶段可能维持此前/当前高度；`FlightTaskAuto::_prepareLandSetpoints` 使用目标 XY、Z=NAN，并按距地变化生成下降速度。LAND.param7 的显式 ground AMSL 请求不等于触地停止高度或已认证地面基准。

本地 mission_block 的 LAND 完成依据实际 land detector。未来必须捕获真正 land-detected 的后续证据与净空；**仅看到一次 landedState 或重复 `MISSION_ITEM_REACHED` 仍不能证明它属于这个 payload、这次启动或正确结束动作**。MISSION_CURRENT.seq、未知 total/state/mode 和重发 reached 收据均不是完整任务完成证明，本模块始终 `missionCompleteVerified=false`。

后续独立 Startup/启动适配器仍需完整的人工作用域授权链、本次新鲜机载任务内容/执行序号绑定、实际状态重新检查、健康/净空和已批准固件构建证据。独立启动意图人工认证本身不能代替这些技术条件或执行完成归属。实际编译、仿真、硬件试验及发布放行仍由工程师承担；此模块没有提前宣称完成它们。

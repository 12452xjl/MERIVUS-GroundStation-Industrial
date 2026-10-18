# 原生启动阶段观测契约

## 459：复用实际事务持有的原生地理运动模型

阶段判定在原完整 `phaseBasisCurrent`、已阅 policy、initial GPI 与 ceiling 检查之后，通过实际 [StartupTransaction](FleetMissionStartupTransaction.md) 的非 QML `startupGeographicMotionBasis(actualVehicle, startupId, exactReadiness)` 获取原 Target 已绑定的 immutable 模型。本模块不再每次阶段重放调用 `prepare`；取源失败沿原拒绝路径结束，不从 JSON、当前样本或调用方值回退重建。

初始 `contains(initialPosition, initialPosition)`、有序 journal 中每条 stage2 GPI 的 `contains(value, previousHeading)`、跨全部原步骤的航向累计与成功提交行为保持。模型只是原判定的同源复用，不携带可变阶段进度；原 HOME／LOCAL／ODOM／EST／ALT、真实收据、完整来源与最后 memory seal 门禁不变。

本批是明确原生模型所有权和避免重复 prepare 的消费者优化，不是修复已经证实的 GPI 重基漏洞：原普通 Authority 风险指纹已绑定该 GPI。它不新增球面走廊到局部图表的数值包含证明、不完成整个运动域净空检查、不扩大 pad 接触豁免，也不解除 Compiler pending 或最终 StartPermit 边界。仅源码与静态读回；未编译、测试或运行。下方 458 的每次阶段内 prepare 描述为历史。

## 458：公共地理运动模型由同次阶段重放复用

原内部 `segmentDistance/movingPosition` 判定移至 [FleetMissionStartupGeographicMotion](FleetMissionStartupGeographicMotion.md)。本模块在原 `initialPosition`、完整已冻结 payload、normalized startup policy 和明确 ceiling 均确定后，只 `prepare` 一次 immutable 模型；首次已阅位置与 stage2 的全部 GLOBAL_POSITION_INT 风险叶均调用该对象的 `contains`。失败继续沿原风险拒绝路径，不补默认或从后续样本重新建立参考。

路径仍是原初始位置起，按完整 canonicalWireItems 中 geographic 项顺序逐边连接的**整条球面走廊并集**，不是原点到各点的星形、当前进度单段或局部 NED 路线。原独立全局 AMSL slab、relative altitude 幅度、速度、量化外包比较和 v1／v2–v4 航向语义保持；`observedHeadingTravel` 仍只在同次完整 ordered journal 中累计，模型不持有可重基的运行进度。HOME、LOCAL、ODOM、EST、ALT、来源、真实步骤收据及整组 memory seal 的原门禁不变。

这是同一实际判定模型的抽取与真实消费者接线，不是新增球面到局部图表的几何外包证明、整运动域净空求解、接地豁免或许可。没有把 model `source()` 值投影加入原阶段 DTO 或当成原生来源认证；原 Compiler pending 和最终 StartPermit 边界保留。仅直接源码与静态读回，未编译、测试、运行 helper／UI／线程／仿真／设备，未修改 PX4、刷机、提交或打包。下方457及更早为历史。

## 457：原生Txn在阶段判定外补同一整组memory尾栅

本模块现有完整phase／journal重放语义不改；实际[Txn消费者](FleetMissionStartupTransaction.md)在逐机完整capture前取[原生memory seal](../../../src/Vehicle/FleetStartupMemorySeal.md)，phase和原READ尾核后先核同seal，再在全组所有回调结束后核完整集合。后机回调改变前机raw token／receipt／来源时，不可保留前机旧通过再用最新值替换seal。

成功集合绑定原generation／stage／startupId，最终命令上下文只读复核；新prepared slot必须先经过完整gate。普通RAW首回调前锁存原scope与journal，closed记录和原step33保持。上述增加本地观察连续性，不证明物理轨迹、同采样、整个球面走廊并集＋AMSL slab包含于已检查空间或最终许可；原pending与StartPermit／正式结果缺口继续。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 445：v4 高度关系包线及原生连续监视

v4 使用原已阅的稳定 reference20 和首次 sample20，逐阶段直接取实际 Vehicle／PRIMARY 的原生 ALTITUDE，对当前必需字段执行固定来源、原点／HOME参考残差及相对首次差值漂移检查。样本不能每阶段重基；完整 HOME 内存尾核和原点核位于全部可回调获取之后。

只在 `BeforeSeq0` 尚未绑定时允许原生 monitor 缺席；其余阶段必须取得实际 owner／PRIMARY／startupId／reference 对应的18键监视结果，原 referenceHash、连续接收 gap、完整源及包线状态均满足，且没有 sticky violation。gap 不借最新好包掩盖先前坏包或超限；监视职责由真实 Vehicle scope 承担，不改 step33。最终尾部再核，不用 caller bool 替代实际监视。

v4 沿 v2 的运动／航向逻辑，HOME 仍 stop-and-review；v3 的可观测 HOME 更新分支不套到 v4。v1–v3 不新增 ALTITUDE 依赖。所有结果仍只是已观察报文关系，内部valid、共同原点、同样本、净空、最终许可和任务完成均未认证。未编译、测试或运行。

## 443：原点声明在原阶段依据中的连续比较

新增 [GlobalOrigin 稳定18键](FleetMissionStartupGlobalOrigin.md) 的原 baseline、本次 Readiness、独立当前 Vehicle 及最终尾部四处比较。baseline 必须带完整 `startupGlobalOriginReferenceObservation/ReferenceRiskFingerprint/ReferenceCaptured/ReferenceMatches` 四项；所有前缀均为 `startupGlobalOrigin`。已观察与真正缺席两支均另核实际 PRIMARY 原生来源，缺席不是有效原点证明。最终检查位于 callback-capable health／receipt／owner取证之后，不按最新声明替换原参考。

原点未见且原生完整空状态可继续原 v1/v2/v3；首次观察、语义改变、来源丢失和已观察 A→B→A 都需要新依据。GPS_GLOBAL_ORIGIN 不是周期心跳，重复同声明的 age／receipt 不进入稳定身份，也不建立三秒周期缺包门槛。原 LOCAL/HOME、ODOM、EST、有序 step journal、实际 owner／阶段及所有原飞行权限门槛保留；新参考不是其中任何一项的替代。

只检查实际原点声明连续性，不认证共原点、PX4内部有效分支或跨流同样本，不放宽 HOME 更新政策、运动包线或最终许可。未新增遥测请求、policy键或命令；旧 flight／coordinate／ODOM 合同及 step33 保持，独立原点合同与终态row13须配套接收。未编译、运行 helper、测试、仿真或设备。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 320–324：HOME 正向观测分支（不是内部有效性证明）

新的 schema3 是完整显式九字段政策，HOME 处理恰为 `bounded-observed-after-owned-arm-or-mode`，继承 schema2 的完整 local/heading/ODOM/gap/阶段约束，并新增[HOME 八字段包线](FleetMissionStartupHome.md)。v1/v2 原六/八字段和 stop-and-review 保持，不从旧计划或旧批准自动升级。以下带旧批次号的 HOME 严格政策只描述对应旧版本。

实际 owner/完整冻结 profile、原始两本账和 PRIMARY 仍先核对。每次 capture 从原 baseline HOMEgeo3、HOMEcoord9、GP9 初始化一次 State，随后按全部真实 ordered receipts 重建；不在步骤或 tick 中重新选参考，计数跨步骤累计。只有真实已排队 NormalArm 的 stage1 或 MissionModeOnly 的 stage2 可接纳 HOME 改变；跳过 ARM 的已解锁路径没有 stage1，不获得伪造 ARM 窗口。Seq0 仍拒 HOME 改变。

真实 geo 改变时，原生 journal 必须紧邻记录 geo3 和 coord9，两叶同 startup RAW receipt、payload hash、地面时刻与 stage，且地理子集一致；coord-only 允许在原 geo 未变时更新 local/timestamp。pending 后插入其它叶或每步结束仍不完整均拒绝，半对不跨 step。HOME 更新时间须严格推进，完整 coordinate 事件计一次，timestamp-only 也计次。flight 与 coordinate 两个全局 risk revision 域各自按其实际改变推进；原队列投影、全 journal、最后 actual current 精确比较均保留。

stage1 的 GP 正向分支仅允许 relativeAltitudeMm 实际改变；其余八个风险字段必须与前值完全相同。偏移受原 GP relative 基线、原绝对幅度与全程次数三项约束。GP 可以先于 HOME 到达，两个异步报文不按因果或同采样配对；这不允许 MODE 前全球坐标、AMSL、速度、heading 或 LOCAL XYZ 改变。stage2 仍走原全球航线走廊、速度/高度/heading 和 local 包线，不绕过任何运动检查。

v3 的输出新增 acceptedHomeObservationUpdates/acceptedPreModeRelativeAltitudeChanges 及明确 false 的 homeReferenceRebased/homeInternalValidityCertified/homeGlobalSameSampleVerified/homeUpdateRequestCausalityVerified；旧版本不增加这些诊断键。原 ACK、新 armed/mode HB、地面状态、全组屏障、来源、参数/版本/任务依据及原期限不变。homeLocalArithmeticDiagnostic 的 homeResetAllowed 始终 false，算术诊断不是本政策的授权来源。

这解决了明确观测预算内正常 HOME 更新的一条源码接续路线，不是所有 HOME reset/估计器切支、飞行器动力学、净空或最终 StartPermit。工程师后续需验证双叶/单叶/缺叶/错序与 ABA、跨步骤累计/零限幅/最大预算、时间回退、GP先到及其它字段改变拒绝、真实 ARM/MODE/ACK/HB 时序与组屏障。这里只做源码和限定文本审阅，未运行这些场景或 helper、编译、测试、仿真及设备操作。

280–284 新增[显式局部运动与航向观察政策](FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

275–279新增独立原生参考风险复核：读取实际Vehicle ODOM reference21、仅去age得20，与原真实Transaction冻结的Readiness20/reference风险hash及最新Readiness一一比较；结束再取实际源复核，已经observed时还需PRIMARY SourceMatches。reference revision为RAW prepare阶段非回绕序列，reset/frame/状态观测A→B→A、过滤后清空、源epoch与时钟降序不能因最新hash恢复为A而复活旧依据。正常time_usec/pose/速度/quality 1..100分值更新不改变该身份，fresh/unknown及状态变化仍保留。

本补充账本不并入原6通道或32字段step receipt，不替代原LOCAL/HOME、队列基线、逐通道gap及风险journal；沒有同sample/同origin/参考有效性认证。最初从未收到流可以保留明确unknown，而曾收到源失效后须新真实有效报文，不能当optional缺流绕过。输出`odometryReferenceValidityCertified=false/crossMessageSameSampleVerified=false`，原HOME/local严格政策与最终StartPermit缺口不放宽。详情见[ODOMETRY契约](../../../src/Vehicle/FleetStartupOdometry.md)；只完成源码和有限静态审閱，未编译/运行/测试。

265–269：新增实际 GroundStartup Transaction 冻结依据路径。`phaseBasisCurrent` 在实际 owner 内纯核 Vehicle/UUID/当前阶段、完整 member/constraints/payload/readiness 和真实步骤前缀，允许发送围栏的 checking/busy 栈但不递归授权。`reviewedArmedGroundBasisCurrent` 只从私有新地面再审依据判定跳 ARM，不接受调用者 bool。

原未解锁路径 UUID 数量仍为 0/1/1/2/2/3/3；新已解锁路径为 0/1/1/拒绝 AwaitArm/1/2/2，receipt 索引对应实际 opcode 0/2。Seq0 journal 持续覆盖全组 ARM 屏障，不造 ARM receipt/ACK；完整初始 armed 状态必须符合真实审阅，自动停机/未批准变化仍停止。

首次 Seq0 前重读实际 RAW，HB/GP/HOME/landed/CURRENT 及 LOCAL 的周期基线须完整且实际年龄不超过原政策 maximumRawGapMs。未知或陈旧 CURRENT 不能先发包后补基线；没有新增取证请求、政策默认值或容差。有限源码契约审阅完成，未运行/编译/测试；HOME 分支/local frame/运动转向、最终 StartPermit/净空/完成仍待开发。下方244–249为原契约历史说明。

244–249增量；本批有限源码静态收口已完成，未编译、运行、测试、仿真或刷机。模块只读取实际对象与原生记录，不发包、登录或恢复审批。

## 接口

capture(Vehicle*, actualTransaction, startupId, member, constraints, payload, baselineBeforeArm, orderedStepIds, Phase, catalog)要求实际Transaction/Vehicle/PRIMARY/目录及原BeforeArm依据，不接受调用者成功bool。BeforeSeq0/AwaitSeq0/ReadyToArm/AwaitArm/ReadyToMode/AwaitMode/ModeObserved分别要求0/1/1/2/2/3/3个实际步骤UUID，逐步重读Vehicle私有保留记录。

match只表示当前来源/阶段/允许变化一致；waiting表示新RAW未完整，不能越组屏障。接收栈未退出时match=false、waiting=true，冻结发送且原期限不延长。所有StartPermit、执行/完成/因果认证标志仍false。

## 原审阅政策

executionProfile可选startupTransitionPolicy，无政策仍可提案/暂存但实际Startup拒绝。政策进入原计划、payload metadata、归档、风险审阅、独立账号依据和sourceHash，不能在启动时补默认容差。

兼容政策schemaVersion=1恰有homeChangeHandling=stop-and-review、headingHandling=preserve-reviewed、maximumRawGapMs（真实整数100–3000）、maximumPhaseDurationMs（真实整数100–30000）、postModeEnvelope连同版本共六字段。新schemaVersion=2恰有八字段：相同HOME/gap/duration/postModeEnvelope，headingHandling=bounded-observed，另须localMotionEnvelope与headingEnvelope；见下节。版本1不能夹带新字段，版本2不能遗漏或靠默认补值。原Review/READ START/身份期限只缩短，不能续期。

postModeEnvelope恰有minAltitudeAmslMeters/maxAltitudeAmslMeters（-1000–20000且前者更低）、horizontalCorridorMeters（0.01–1000）、maxHorizontalSpeedMetersPerSecond（0.01–300）、maxClimbSpeedMetersPerSecond/maxDescentSpeedMetersPerSecond（0.01–20）、maxRelativeAltitudeMagnitudeMeters（0.01–21000）七个真实有限数字。解析上限不是机体性能建议；最高高度不越原约束；仅兼容schema1要求航向精确保留原已知值。

### 280–284：schema2独立观察包线

localMotionEnvelope恰有referenceBasis=observed-native-odometry-ned-not-origin-proof、maxHorizontalDisplacementMeters/maxClimbDisplacementMeters/maxDescentDisplacementMeters（0.01–1000m）、maxHorizontalSpeedMetersPerSecond（0.01–300m/s）、maxClimbSpeedMetersPerSecond/maxDescentSpeedMetersPerSecond（0.01–20m/s）共七字段；三速度不大于postModeEnvelope对应上限。只独立约束原LPN reviewed baseline到当前观察的NED半径、上/下位移及当前速度，local z向下，不用HOME或ODOM pose替换坐标、对齐原点或证明同样本。仅实际owned MODE的stage2风险叶可应用；Seq0/ARM和HOME仍是原严格政策。

headingEnvelope恰有maxDeviationDegrees/maxAdjacentChangeDegrees（0.01–180度）、maxObservedTravelDegrees（0.01–3600度），adjacent≤deviation、travel≥adjacent。原始GP已知整数headingCdeg通过35999/0最短圆周差检查，分别核原基线偏离、上一真实GP相邻变化、全部journal累计观察变化。累计变量仅一次capture开始置0，然后从固定原基线重建所有ordered steps，不在每步/包/tick续作中重设或用新基线替换历史。每对编码保守外包0.02度；仅heading编码改变才累计，同位置仅heading变仍检查。合法0.01度输入可能在原点自检被拒，不补额外容差；累计不是yaw rate或真实总旋转角，不能排除未看见的半圈/多圈。

schema2另要求实际ODOM已观测、fresh age≤policy.maximumRawGapMs、NED/NED、显式resetCounter与estimator=AUTOPILOT8，以及原reference/source/revision未改变；稀疏covariance NaN/unknown quality不一律误拒。其native周期gap从真实queue前row、每次原始收包和阶段尾计算，等内容包也更新时间，新的fresh包不能清除此前已记录超gap。32字段收据形状不增；schema1不新增该ODOM周期要求。LOCAL_POSITION_NED与ODOMETRY来自不同uORB流，匹配只解释明确观察契约，所有reference/运动/净空/StartPermit/执行完成认证仍false。

## RAW历史

当前schema3步骤收据33字段：原26字段加queueFlightObservation、queueCoordinateObservation、queueEstimatorObservation、riskObservationsJournal、journalExhausted、maximumObservedRawGapMs、historyDurationMs。ACK/HB/current叶为receiptSequence、receivedAtMonotonicMs、rawPayloadHash、value。journal叶仍为journalSequence、receiptSequence、channel、receivedAtMonotonicMs、rawPayloadHash、riskContentHash、value、source；EST是独立channel，不增加第七个flight通道。schema2/32是381–385之前的历史形状，不双接受为新连续性依据。

每步256个风险变化上限，溢出/非法源/缺口保持拒绝。lane关闭后最新历史继续到下一步实际queue或scope取消。Startup RAW baseline/journal同计数域；六通道风险revision与补充坐标revision各自非回绕，不跨域或转Double比较。逐叶重建风险后核当前投影/revision，防已观察ABA。间隙逐连续通道计算，包含同值包、排队前年龄及尾间隙；MISSION_REACHED是事件，不要求周期流。

完整RAW保留，GP/LPN timeBootMs只从风险移除。HOME time_usec是更新时刻，精确十进制uint64保留为风险。历史不证明无线未收到的物理运动连续安全。

## 允许变化与未完成边界

seq0为实际MISSION_SET_CURRENT(seq=0)后新RAW CURRENT；不走旧PX4展示序号重映射，无COMMAND_ACK，未知扩展不补任务身份。普通ARM固定param1=1/param2=0，仅允许精确armed/baseMode/systemStatus过渡并保持地面POSCTL及其它原风险；接受ACK与新状态分开核，不force或重试，不用MISSION_START隐式ARM。

仅本机MODE实际queue后可按政策逐叶接受位置/速度/着陆阶段/范围内非回退序号；其它未发MODE机仍需地面状态。球面wire航线走廊含原起点至第一点连接，是遥测几何模型，不是障碍/地形/椭球误差/动力学净空证明；厘米/mm编码边界保守向外，不变成任意传感器误差容差。

补充LOCAL_POSITION_NED与HOME geo/local xyz/time_usec不能证明PX4 lpos.z_valid或home.valid_alt/valid_lpos，也不证明同一估计器样本。诊断仅计算假定local分支float32 -(localZ-homeZ)*1000向零截断；非有限/int32越界不计算。算术吻合仍branchValidityProven/sameEstimatorSampleProven/homeResetAllowed=false。HOME/更新时刻或未批准相对高度变化停止后续步骤，不删字段或偷偷重设基线。

取消保留可能已写序号/解锁/起飞事实，不自动DISARM、恢复序号、清任务或宣称回滚。合法HOME有效分支、受信local origin/参考有效性、完整动态/净空、执行时序与完成归属仍在原目标中；本批观察包线不是这些证明。工程师后续验证真实时序、HOME分支、量化/间隙/ABA、组屏障与部分飞行接手；本轮未做这些验证。

兼容schema1在MODE后仍精确保持reviewed local x/y/z；新的schema2仅以全新审阅的明确观察包线接受符合范围的运动，不忽略reference/revision/gap变化。着陆状态仍仅允许ground→takeoff/in-air及takeoff→in-air的已观察过渡，不能任意枚举往返、降落/重新起飞或把跳采样当连续物理证明。因此本批不是完整动态启动/着陆完成系统；后续必须补足受信参考/HOME分支、真实净空/动态/时序和完成归属，不能以观察匹配冒充最终许可。

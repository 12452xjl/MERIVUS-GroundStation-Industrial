# FleetMissionPreviewTimeline — 540

## 543：共用几何内核的逐机名义路线时间

新增纯C++ `nominalRouteTiming(member)`，与 `build` 共用同一个point／duration内核，不另写另一套距离／速度公式。调用者提供已解析实际出程的effective member；Compiler只在允许的普通公共路线缺省时提供原global fallback，helper不自行复制顶层路线、换成员或选速度。每机出程和返程合计最多4096点；类型、范围、原profile或任一点非法则两段均未知，不保留成功前缀。所有点先完整验证；已知输入下任一阶段存在未知时只令该阶段null，不将失败前累计片段称为完整阶段。缺独立returnPath不补零，不凭返航reserve预算推造几何。

固定exact11：`schemaVersion:1/basis:'ideal-simultaneous-horizontal-vertical-v1'/outboundKnown/outboundTravelSeconds/returnKnown/returnTravelSeconds/outboundReason/returnReason/routeTimingCertified:false/completeMissionDurationKnown:false/holdsIncluded:false`。Known为Bool，未知秒数显式null，已知为非负Double；空成功reason显式非null空String。出程只从首规划点到末点，单点可有已知零段长；返程包括出程末点→返程首点及后续完整航段。无首点前真实飞机进场、起飞、落地／终端HOLD等模型。明确缺省profile不补cruise或爬降速度；原profile坏值不能退回其它profile。既有returnSpeed缺省沿显式cruise持续的政策不变。

这里只累计travel，不含任何驻留；用已有DwellEncoding的向上保守累加保有限和值，精确零／精确和不额外增加ULP。驻留继续由Coordinator按同机原 `budgetSeconds` 独立纳入，不能把预演的 `modelWaitMicroseconds` 显示时长直接拿来或重复累计。此名义值只可扩大已有任务／返程预算，不能把nominal-only、hold-only或部分路线提升为完整任务已知；真实耗电、风、机动／加减速、首点前／终端时间和时序仍未知，所有原能源与StartPermit门保持。

本批仅apply_patch及直接静读，未运行模型／编码器／hash／项目、编译或测试。下面540描述仍是原 `build(plan,energyPlan)` 的事件预演域；其事件规模、每机独立路线要求和不证明完整任务时长均未改变。

纯值、有限规模的规划时间化预演。`build(plan, energyPlan)` 消费同一条真实审阅记录中的完整计划和能量计划；不读时钟、飞机、文件、网络，不改计划或启动权限。最多 32 个实际成员、出程与独立返程合计 4096 点、8192 事件；非法点、缺少成员独立路线或超界均整份失败，不发布成功前缀。没有把顶层公共路线复制给所有飞机。

每机按真实成员顺序连接出程各点，再连接出程末点到其独立返程首点及返程后续点。第一规划点只设本机相对零时刻，**不含真实飞机到首点的飞行、起飞或任务末动作**。无绝对到时含义。事件闭 11 字段为 `index/vehicleId/phase/kind/pointIndex/startSeconds/endSeconds/durationSeconds/durationKnown/timeKnown/reason`；phase 为 outbound/return，kind 为 origin/travel/dwell。未知时间为 null；前段未知后，后段可以有已知局部 duration，但累计起止不恢复成已知。

地理水平长度使用声明半径 6371008.8 m 的球面大圆短弧（跨日期线规范化），本地长度使用二维欧氏距离且要求原点/终点明确相同 `localFrameId`。高度须两点都有数值、明确相同 `altitudeReference`，仅 geodetic+amsl 或 local+同 frame+local_origin 支持比较；AGL/terrain 等保持未知，不能把两点离地高度直接相减。不推定 AMSL，不连接不同坐标系。沿现覆盖规划值域，本地 x/y 绝对值最多 1e7 m，高度在 -1000..20000 m，超界整份拒绝而非发布极端有限时长。显式成员执行 profile 通过原验证器后才读取速度；没有显式 cruise 或必要 climb/descent 不补默认。返程有显式 returnSpeed 时采用它，否则沿同 profile 已显式 cruise：这与原 PayloadBuilder 仅有 returnSpeed 才发 DO_CHANGE_SPEED 的持续指令语义相同，不是给未知机载速率填默认。理想航段时间为 `max(水平长度/水平速率, 竖直变化/对应速率)`，假设两轴同时匀速；这不是 PX4 实际加减速/拐弯/风场/导航半径模型。零位移不需要假造速度。无穷距离/时长保未知。

显式驻留经共用 DwellEncoding 验证，事件使用 `modelWaitMicroseconds/1e6`。原请求、Float32 上传、声明模型整数等待与保守 budget 不是同一个值：原请求仍留在原计划中，此处不把 budget 当真实经过时间。缺失驻留无事件；显式 0 保留零时事件。旋翼实际驻留从首次位置接受开始，等待机头可能消耗计时，不能称精准悬停或达向后完整驻留。

摘要有 available/error/sourceTaskId/planId/memberCount/eventCount/knownTimeCount/unknownTimeCount、相对起点/模型名/returnSpeedPolicy 与 members。每机 members 保 vehicleId/role/eventCount/knownTimeCount/unknownTimeCount/routeDurationSeconds(nullable)/routeDurationKnown/returnPathProvided/batteryEvidence。电量小摘要只按该机原 group=primary 的 energy assignment 匹配；reserve 不冒充任务成员预算。最多 255 项且重复身份、非法 group 整份拒绝。保原 -1、false 和未知 null，不均摊总能量、不重算耗电、不拿 returnReserveSeconds 代替实际返程时长。缺 primary assignment 明确 available=false。能量、路径、时序均未认证。

所有成功摘要仍固定 `launchModeled=false/endActionModeled=false/completeMissionDurationKnown=false/timingCertified=false/energyModelCertified=false/actualPathCertified=false/flightCommandReleased=false`。有路线时长不等于完整任务可行。故障覆盖仍由 Twin 原建议层显示，不改变本路线模型或伪造重分配执行。本批仅源码静读；未运行模型、解析器、测试、编译、仿真或真机。

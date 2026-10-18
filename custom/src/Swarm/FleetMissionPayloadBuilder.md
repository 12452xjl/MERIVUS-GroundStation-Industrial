# FleetMissionPayloadBuilder 显式任务请求载荷契约（v2）

## 543：名义路线时间是可核对的规划元数据

成员白名单增加可选 `nominalRouteTiming`。存在时，在两段完整路线的既有类型、数量和wrapper检查之后，用同一 [Timeline](FleetMissionPreviewTimeline.md) 计算核对全部11字段；Bool/String严格同型，数值必须实际有限数值，未知仅null，拒绝增删键或与实际本机profile/路线不一致的报告，错误为 `nominal_route_timing_mismatch`。不拿调用者地图、旧报告或别机估值替代计算。

无该字段的旧计划仍按原门检查。此元数据不写入MAVLink，不新增指令、不改变wireSchema、时序授权、起降及clearance要求。Builder仍要求每成员两段各至少2个显式点；Compiler规划支持的公共路线回退不因此自动取得上传适配。仅源码静读，未运行计算、序列化、测试或编译。

## 537：NAV驻留的Float32／微秒模型统一编码

路线点解析及canonical `NAV_WAYPOINT.param1` 都调用 [DwellEncoding](FleetMissionDwellEncoding.md)，拒非实际数值、域外和正请求在Float32／微秒计时中塌零；缺省仍p1=0，非NAV动作p1=0与速度type1规则不变。原 `items.param1` 保原请求，canonical存一次Float32上传值；原 `wireQuantization.floatParameterDeltas` 继续显式记录p1差，不新增payload字段／报告map／schema。直接Float32读回用原严格规范身份，不加驻留容差。无hold旧输出身份不变。

共享helper另给原请求／上传秒数／float乘微秒再截断模型的保守预算，供真实审阅能量消费者使用；预算上界不替换请求或RAW值。仅旋翼普通NAV首次位置接受后的等待模型，yaw等待可能消耗这段时间，不证明连续半径停留、精确悬停、绝对同步或实际任务时长。原SourceItemMap、去返序号、坐标／heading、StartPermit及false权限边界保持。仅源码和直接静读，未运行编码器／模型／helper／parser／hash、编译、测试、仿真、业务IO或PX4修改。下方为历史。

## 536：NAV显式朝向的v3角度网格语义

逐点原 `heading`／`yaw` 必须经 [HeadingEncoding](FleetMissionHeadingEncoding.md) 完整校验；仅普通 `NAV_WAYPOINT` 编码有限 `param4`／`yawMode='absolute-degrees'`。原连续请求按明确最近1/256度tick量化，正半格向上，末格到360时以0发出并明确诊断。原请求保在plan／source身份；`items.param4` 是精确binary32上传tick，不被回读结果替换，也不二次量化。缺heading仍为原null／system-default→NaN，显式0不等缺省。TAKEOFF／LAND／HOLD仍只支持缺省yaw，DO_CHANGE_SPEED仍为原type1和reserved-zero。

无显式heading的完整单机项列表保持原 `mission-item-int-plan-v2` 与原规范内容；有显式heading则为 `mission-item-int-heading-tick-v3`。canonical param4仍以度表示tick／256，不换成整数tick单位。实际读回有限float32原值保在raw数据中，只有共享helper的唯一最近圆周tick及明确推导界内才归入同一语义代表；这不是任意epsilon、原上传字节相同、固件二进制认证或实际yaw达成证明。v3 `contentHash` 标识完整角度语义比较域，原RAW payloadHash／回执fingerprint及原task／plan哈希保持各自用途，不冒充同一身份。

原每项 `sourceItemMap` 及item9审阅形状不扩。`wireQuantization.floatParameterDeltas` 每项仍有sequenceNumber／deltas，仅显式NAV追加 `headingEncoding` exact10：schemaVersion、model、requestedDegrees、uploadDegrees、tick、circularDeltaDegrees、wrappedToZero、stepDegrees、maximumCircularQuantizationErrorDegrees、maximumReadbackErrorDegrees。共享helper给出原请求、实际编码、圆周量化差／wrap以及声明界；缺省项不加字段，不能隐去量化后声称原值原样执行。新增encodedSemantics `absolute-waypoint-heading-tick256-request` 指向原路线点。

仅显式heading成员追加未编码要求 `waypoint-heading-attainment-and-turn-dynamics-not-certified`，编码后仍须明确真实达向／转向动力学未认证；无heading旧身份不增字段。原 `exactGeometryPreserved` 仍只表示XYZ／E7坐标量化未变，不覆盖yaw网格量化；朝向差另在headingEncoding明确显示。原坐标E7、其它Float32、完整去返连接／动作、边界、总预算及全部权限false不变；heading不成为扫描线轴、起降朝向、强制达到朝向或完整StartPermit。仅源码和静态／普通文本核对，未运行编码器／helper／算法／hash、编译、测试、仿真、上传或改PX4。下方为历史。

## 521：正常编码不丢区域来源的明确性能门槛

正常 `build(plan)` 在完整来源检查阶段，对任何提供的 plan 根 `minimumSpeedMps`、`minimumEnduranceSeconds` 检查实际非 null 数值、有限及 `0..9007199254740991`，坏型／域外返回 `planning_performance_source_invalid`。原区域 `planningSource` 分支继续先调用中央 Draft validator 并要求完整规范源相同，因而可接 schema6、保持旧1–5；再把重建 sourceTask 的两个门槛与 plan 根逐项核对：同 presence、同有限数值。原 `requirePerformanceEvidence` 也须同 presence、实际非 null Bool 且同值，不可借直接编码绕过 Store 的严格政策对应检查。缺失／旁加／改值返回 `planning_performance_source_mismatch`，整批 payloads 为空，不删来源后编码，不只依赖先经过 PlanStore。

该检查只保存完整输入政策，不重新认证原性能、重跑当前机载能力或把门槛当许可；真实普通／严格逐机准入仍由 Compiler／Matcher／Allocator 完成。原边界、INT／E7／float32、实际 profile、全组预算、原 StartPermit 和所有 false 许可声明不变。`buildImpl` 的历史开关仍为 private；正常 `build` 固定关闭。惰性 `reproduceHistoricalPayloads` 仅供原冻结 payload／provenance 自洽比较，有完整 policy 即回正常 build；旧无 policy 的独立复现不恢复当前审阅、上传、执行资格或权限。本批未编译、运行编码器／模型／脚本、测试、仿真或操作设备；待验证源与 plan 的缺／零／正值、严格与非严格、单边删改／坏类型及旧源兼容。下方为历史合同。

## 513：备用独立出勤来源进入正常编码前完整核验

正常 `build(plan)` 的 `boundarySourceProblem` 新增 `reserveDispatchPlanningSource`，与区域、单机余段、联合余段来源四者互斥；混用返回原 `route_boundary_source_conflict`。新字段提供即须真实非 null Map，再调用 [ReserveDispatchPlanner](FleetReserveDispatchPlanner.md) 的完整 `validateProvenance`、规范 provenance 紧凑 JSON 比较和 `matchesPlan`，绑定实际成员／完整去返路线与停留／profile／constraints／边界及 canonical strict Bool。坏型返回原 `route_boundary_source_invalid`；无效或不匹配返回原 `route_boundary_source_mismatch`，整批 payloads 为空，不删来源后编码或放行成员前缀。

真实新任务仍走原完整 profile、动作、INT／E7／float32 编码与编码后原始边界复核，成员／点数／工作预算和旧历史复现隔离不变。合法 false／缺省不升级严格性能，true 不可在新计划投影中剥掉；完整来源匹配只是值一致性，不认证来源、机载能力、实际轨迹或已批准。没有新增 Mission 命令、备用自动激活、旧 Run／控制权／占用恢复、上传副作用或 StartPermit；原 `executable=false`／`flightCommandReleased=false` 保持。

本批为完整原生后台消费者，Root／可见人工出勤入口仍待后续接入。仅静态修改／对读，未编译、执行编码器／模型／脚本、测试、仿真或飞行；下方为历史合同。

## 478：完整原始边界策略与实际 INT 编码链

当前 `builderVersion=2.1`，正常 `build(plan)` 一律要求完整规范 `routeBoundaryPolicy`，包括显式 inactive。没有字段返回 `route_boundary_policy_missing`；null、错误实际类型、未知键、失效区域或非规范值返回 `route_boundary_policy_invalid`。不能从 `areaSummary`、规划器版本、零孔计数或删掉版本标签推断“没有区域限制”。旧历史仍能保存和查看，但恢复成当前可准备的计划必须从完整源重新编译和审阅。

策略由 `FleetAreaCoveragePlanner` 的共享纯值 API 校验：exact3 `{schemaVersion:1, active:bool, area:Map}`；inactive 的 area 必须为空；active 的 area 保留**原始**外环、完整孔列表、坐标系／本地基准和 `boundarySetbackMeters`，不是丢失外环的摘要或逆投影候选轮廓。其自身不带 checked、认证或许可声明。

正常构建时，`planningSource`、`remainderPlanningSource`、`groupRemainderPlanningSource` 最多一种，冲突返回 `route_boundary_source_conflict`。草稿源用 `FleetMissionDraft::validatePlanningSource()` 完整重建原 task.area，再归一化策略并与保存值逐字节比较；单／多成员余段源用各自完整中央 provenance validator 和 `matchesPlan()` 绑定，不接受旧的无完整策略来源冒充已绑定。已知源失效或不匹配返回 `route_boundary_source_invalid`／`route_boundary_source_mismatch`。这些都是内容一致性，不是原生所有权或认证。

有覆盖计划时，active 策略必须仍有完整 `holes/holeCount/boundarySetbackMeters` 摘要；三者任一存在也要求三者完整且策略 active。holeCount 必须是与真实列表长度相等的实际整数。将摘要孔和退让距离与策略的完整原始外环／frame 合并，再用同一 Planner API 重建且比较完整 canonical policy；不同、缺失或删掉整组摘要返回 `route_boundary_coverage_mismatch`。不将无孔无退让的普通新计划强加 64 顶点覆盖限制。

编码完成后，以每台机**实际完整 Mission item 顺序**构造检查专用坐标链：地理项使用 canonical 的 `latitudeE7/1e7`、`longitudeE7/1e7` 和实际 float32 `param7`。包含已编码 TAKEOFF、出航、返航、HOLD／LAND，保留重复连接点和垂直段。只过滤 `MAV_FRAME_MISSION` 的无坐标速度命令；速度项保留零值不是 `(0,0)` 地理点，过滤也不得切断两侧航段。

整组所有检查链一次传入 `validateBoundaryPolicyRoutes(policy, encodedMembers)`，共享既有 8192 点与可见性检查预算，不能逐机重置预算。原先完整 items 预检／最终预算仍包含速度及起降动作；新增检查再核全组坐标数。任一编码后点、完整航段、原外边界距离、孔内穿越或资源限制不成立，返回 `wire_route_boundary_conflict`／`wire_boundary_item_budget_mismatch`，payloads 始终为空，不输出前缀、不降低 margin、不夹紧或移动路线。

检查不修改原始 `items` double；PlanManager 必须仍从它们恰好做一次 E7 朝零截断，不能先写回 E7/1e7 再二次截断。`canonicalWireItems`、wire schema 和实际 Mission item 的 contentHash 格式不变。新正常结果和每个 payload 保存同一完整 `routeBoundaryPolicy`，使单成员启动准备的纯编码复核不必从摘要猜测限制；全组统计只在顶层 `wireBoundaryCheck`，不写进单成员载荷。顶层字段为 `active/nominal2DPolicyChecked/wireSchema/memberCount/coordinateItemCount/numericalErrorCertified/physicalClearanceVerified/clearanceCertified/flightCommandReleased`；inactive 的 nominal2DPolicyChecked=false，其余四项认证／许可声明始终 false。active 成功只表示有界投影中的编码后名义二维点／线检查，不是浮点误差认证或真实净空。

静读的实际传输依据：`FleetMissionDeployment` 门禁要求 MISSION_INT，并走 `PlanManager::writeFleetDeploymentMissionItems()`；`PlanManager::_handleMissionRequest()` 只调用 `mavlink_msg_mission_item_int_pack_chan()`，即使收到 legacy MISSION_REQUEST 也不改成 float MISSION_ITEM。原始 double 经纬度乘 1e7 后隐式 int32 截断、参数／高度 float32，与既有 canonical 一致。独立 RAW 回读收到 MISSION_ITEM 明确取消。因此本链检查 INT 的 E7 XY 和 float32 高度，不发明不存在的 float32 XY 回退兼容承诺。synthetic HOME 不被专用 PX4 上传列表采纳，不把 HOME→TAKEOFF 的真实运动当作已检查链。

### 仅惰性历史复现的独立入口

`reproduceHistoricalPayloads(plan)` 只供旧 archive／provenance 的完整内容比较，不能用于当前审阅、准备或上传。有完整 policy 时仍调用正常 build；没有 policy 且不带当前 compiler 0.7.0／plan 1.1 标记时才按旧 builder 2.0 的字段和值重现，不加入新策略、全组检查元数据或新的来源门禁。当前标记丢失 policy 直接拒绝。历史复现从不产生启动／上传许可，也不是把旧无策略计划迁移成新计划；当前 live flow 不能调用它。

### 工程师后续验证（本轮全部未执行）

- 缺失／null／错误类型／多余键／损坏 outer、孔或 margin 的策略；删除 compiler/version 仍不能让正常 build 绕过缺失策略；旧惰性 archive 可复现但正常准备被拒。
- 草稿源（含 source 3／4）、单成员新 provenance 3／4 和多成员新 provenance 2 的原始 outer／孔／frame／margin 与保存值不一致，两个来源混入，旧来源配新策略，以及覆盖三摘要任一缺失、全删、计数伪装或内容冲突，均整批拒绝。
- 原始坐标恰好满足 margin、E7 正／负经纬度朝零截断后进入孔／过外边界／不足原 margin，包含去返连接、重复点、TAKEOFF／HOLD／LAND；全链不得修点或降低 margin，原始上传 double 保持不变。
- 不把速度项保留零变成地理原点，也不因其位于出返之间就中断完整线段；垂直和零 XY 段仍检查原边界与孔约束。
- 多机完整动作数 8192／8193、单机 2048／2049、全组可见性预算耗尽、最后一机失败时无任何前缀；不同机器不能各自重新得到 8192 点预算。
- 旧历史 builder 2.0 完整 payload 字节形状与新策略存在时的严格路径分开；当前审批、上传、启动准备不调用历史复现；新单机 payload 的同一完整策略参与其启动准备输入身份，不能替换成 inactive。
- 真实 Qt／平台编译及原生 INT 上传／回读、插件／对象生命周期和实际设备边界仍交工程师；没有以源码静审替代这些验证。

## 当前 v2 支持范围

本类仍是纯数据构造器，没有 Vehicle、文件、网络、上传或飞行命令副作用。`ok=true` 只证明整个批次可以按当前有界 wire 契约序列化，所有结果保持 `executable=false`、`flightCommandReleased=false`。**编码请求不等于真机已接受、已执行、飞行安全或工程放行。**

旧计划没有 `executionProfile` 时继续构造旧几何路径，不补造默认速度或起降动作；显式 `holdSeconds` 仍可以编码。新成员 `executionProfile` 必须为实际 Map，经 `FleetMissionExecutionProfile::normalize()` 全量严格校验。计划顶层默认如存在也必须合法，且每个成员已经由编译器解析出显式 profile；成员整体覆盖可以与顶层默认不同，此类不自行继承。非法/null profile 不能回落为无 profile。

当前序列化以下显式请求：

- 出航前插入 `MAV_CMD_DO_CHANGE_SPEED`、`MAV_FRAME_MISSION`，param1=1（水平地速）、param2=`cruiseSpeedMetersPerSecond`、param3=-1（不改油门）、param4–7=0。可选 `returnSpeedMetersPerSecond` 仅在返航段前增加第二条速度指令；没有该字段则不补默认。辅助角色只使用自身 profile，不自动继承覆盖机速度。
- `launchAction.type=takeoff` 插入 `MAV_CMD_NAV_TAKEOFF`，WGS84/AMSL 坐标必须与出航第一点完全相同，param1–3=0、param4 为 null/system-default。不删掉原第一 waypoint，不构造起飞连接航段。`type=none` 原样保留为未编码启动状态要求。
- waypoint 显式 `holdSeconds` 必须为有限实际数值 0–86400，写入 param1；缺省仍为0。只表达**到点之后**的相对停留，不根据 `timeSeconds`、deadline、duration、估算或点间时间差反推。param2/3 仍为0默认接受/通过半径。
- `endAction.type=hold|land` 在完整返航路径后分别增加 `MAV_CMD_NAV_LOITER_UNLIM` 或 `MAV_CMD_NAV_LAND`，param1–3=0、param4 为 null/system-default。HOLD 必须与返航最后一点经纬度及 AMSL 高度完全相同；LAND 的显式地面坐标必须与最后返航**进近点**经纬度完全相同，但 ground AMSL 必须严格低于该进近点高度。原返航 waypoint 保持显式进近高度，随后 LAND 使用独立显式地面高度，不让先行 waypoint 飞向地面。不猜测新连接段、默认高度、精确降落能力或净空。`type=rtl` 不编码，保留明确缺口。
- 起飞、终结动作和所有路径点均检查原始及 float32 编码高度不得超过同一显式 AMSL 上限。速度项的保留 param7=0 **不是高度**，不参与 AMSL 上限比较。
- LAND 的 ground-below-approach 顺序在原始 double 和实际 float32 编码后都必须严格成立；量化后高度重合或顺序失效返回 `wire_landing_approach_collapsed`，不自动加高度、夹紧或声称存在安全下降间距。坐标不同返回 `execution_landing_endpoint_mismatch`，原高度顺序无效返回 `execution_landing_approach_required`；错误定位为对应车辆及 `executionProfile.endAction.coordinate`。
- 完整 canonical 列表再次核对：最后 LAND 必须立即跟在最后返航 NAV_WAYPOINT 后，两项都为 GLOBAL、E7 经纬度相同，实际编码 waypoint.param7 必须大于 LAND.param7。任何结构或高度不匹配返回 `wire_landing_approach_contract`，定位为车辆和完整 Mission item 序号；不输出部分载荷，也不修改原请求高度。

覆盖参与机的显式 profile 巡航速度必须与 `coverageRequirements.cruiseSpeedMetersPerSecond` 相同；若覆盖及 profile 同时要求爬升速度，也必须一致。冲突导致整批失败，不自动替换请求。覆盖的爬升估算只保留，不能提升为实际垂直速度控制或时间证据。

## wire v2、来源映射和资源

`canonicalWireItems()` 的 schema 为 `mission-item-int-plan-v2`，只接受上述五类命令及准确参数范围，不成为任意 MAV_CMD 的通用透传器。地理项只接受 GLOBAL，param4=null/yawMode=system-default；速度项只接受 MISSION、type=1、0.1–300 m/s、throttle=-1、所有保留值为数值0、yawMode=not-applicable。非默认 heading、其它 speed type、NAV_DELAY、RTL、精确降落参数和任意额外 wire 字段均拒绝。

地理项 canonical Map 保留 `latitudeE7/longitudeE7`；MISSION 速度项改为 `param5Integer/param6Integer=0`，不得按经纬度缩放。浮点参数全部按实际 float32 编码纳入指纹，地理 yaw 采用 JSON 安全 null 哨兵。原始坐标度数只在 PlanManager 写入时乘 1e7 朝零截断一次；INT 回读用 round 恢复整数。可变 current flag 不属于身份，autocontinue 仍属于身份。全部 canonical 项的紧凑 JSON Array 字节以 SHA-256 哈希；不是审批签名或机载执行证明。v2 是显式协议版本，不能把未知固件、legacy float 回退或旧 archive 标签直接当成同一适配能力。

每个 payload 增加 `sourceItemMap`，将完整 Mission sequence 映射到原路径/点序号或 profile 动作；`encodedSemantics` 逐项标明已编码**请求**和源字段，`executionCertified=false`。`wireQuantization.deltas` 逐项标明 `coordinateItem` 并保留地理量化差值，速度项不冒充地理点；`floatParameterDeltas` 另记录各项 param1–4/7 的 float32 编码差值，null yaw 不被转成0。`totalPointCount` 与 `totalItemCount` 分开，后者包含新增速度、起飞和终结动作。

所有新增动作都计入预检和最终预算：32 成员，每成员最多2048个完整 Mission item，全批次最多8192个。预检统计所有真实路径及动作，最终输出再次核对总数；超限或任一项错误只返回空 payloads，不放行前缀。

## 显式未编码／未认证边界

每机保留规范 `executionProfile`、`unencodedRequirements`。`relativeDwellEncoded` 仅在存在显式停留时为true；`launchActionEncoded`、`endActionEncoded` 和各速度编码标记按实际输出报告。顶层 `endActionEncoded=true` 要求所有成员都编码了显式 hold/land；它不是动作完成状态。`temporalSemanticsEncoded=false` 和 `sourceScheduleEncoded=false` 不变。

- 本地 PX4 的 DO_CHANGE_SPEED 处理没有实现独立 climb/descent speed type，故 `verticalPolicy` 及所需参数 profile hash 只保留为未认证要求，不伪装成机载垂直速率命令。
- `absolute-required`、同步起飞、指定到达时刻、deadline／预算自动处置没有被编码。既有完整 sourceSchedule 和非权威估算仍旁路保留，不因相对停留编码而改变证据等级。
- 本地 PX4 自动起飞可能先在**实际当前位置**垂直爬升，并按 `MIS_TAKEOFF_ALT` 提升目标；profile 端点完全一致不证明真实航迹、起飞高度或净空。该实际参数/路径上限缺口明确列出。
- LOITER_UNLIM 不会自行完成 Mission；LAND 完成需要 land-detected 等真实证据。即使编码了进近/地面两个显式高度，真实 ground datum、进近与下降净空、能量持续性、当前 Home、真实完成状态仍缺失，保留对应未认证阻断项。
- 角色、围栏、最小间距、能力／参数认证、实际航迹及动力学均不由此类执行或认证。独立启动许可仍需完整版本、实时健康、权限、读回、参数及动作证据门禁；上传服务不得因 wire v2 增加了命令就解除几何暂存的启动隔离。

本轮只修改并静态审阅源码与契约；未编译、运行、测试、仿真或联网传输。

## v1 几何阶段历史记录（以下不是当前完整契约）

`FleetMissionPayloadBuilder::build(plan)` 是无状态、确定性的纯 C++ 帮助类。只构造逐机几何载荷，不实例化 Vehicle、MissionItem 或 QObject，不访问文件/网络，也不发送上传、启动、解锁、返航等指令。上层上传服务独立负责当前计划版本、审批、权限、健康状态和事务归属门禁；`ok=true` 本身不是允许上传或执行。

## 支持范围与严格拒绝

- 真实数值 `schemaVersion=1`、64 位小写十六进制 `planId`、真实布尔 `manualConfirmationRequired=true` 与 `flightCommandReleased=false`。可选 `feasible` 必须为真实布尔，但此类不以它代替审批。可选 `requiresOperatorReview`、`dispatch` 不得与审阅/未放行相矛盾。
- `members` 为 1–32 项实际 List，真实整数 Vehicle ID 为 1–255、唯一且不与 `reserveVehicleIds` 重合。角色只能是规范小写 Scout/Leader/Relay/Responder，最多一个 Leader，不把备用机编入上传批次。
- 每个成员明确 `coordinateSystem=geodetic`（本工程契约中的 WGS84）和 `altitudeReference=amsl`，`returnRequired` 必须为真实布尔 true。没有本地坐标、Home 相对高度、隐式机体/地图坐标转换。
- 出航 `waypoints`、返航 `returnPath` 各至少两个点；只接受实际 List 或带实际 `waypoints` List 的包装 Map。包装 Map 和各点也必须明确一致的地理坐标系与 AMSL 高度基准，不借缺失值推断。经纬度/高度只接受有限真实数，纬度绝对值不超过 80 度、经度 ±180 度、高度 -1000–20000 米；这些是解析范围，不是飞行许可。
- 必须提供实际 `constraints` Map 及有限 `maxAltitudeMeters`、`maxAltitudeReference=amsl`。逐点核对该海拔上限，不能把相对高度与海拔比较。
- 点别名 `latitude/lat`、`longitude/lon/lng`、`altitude/alt` 如同时提供必须完全一致；未知点或包装字段直接拒绝。点 `sequence` 是检查元数据；输出序号按完整逐机列表重新从零连续编号。
- 返航第一点必须与出航最后一点的经纬度及高度完全相同。不删掉共享连接点，也不补造飞行连接段，因此上传列表包含该共享点两次；分别保留出航/返航计数。
- 原有权威时间字段、`timeReference`、成员/包装起止时间与 duration 严格验证后完整保存在 `sourceSchedule`：必须为有限真实时间数值、正 duration/预算、一致别名、不逆序的起止/逐点时间，基准/证据标签必须为有界非空字符串。约束 deadline 另允许编译器支持的有效 ISO 日期字符串。null/空值、冲突、非法类型和未知全局安排字段拒绝；嵌套 `schedule` 尚无协议，拒绝而不默默丢弃。**这些要求没有被编码成机上时序**，只允许地面静态准备/上传阶段，不能由成功几何回读升级成启动许可。任何请求的点/成员 `heading/yaw` 仍返回 `unsupported_heading`。
- 有限非负 `estimatedElapsedSeconds` 原样作为非权威旁路元数据保留，并检查逐路线不下降；每点未提供的估算以 JSON null 保留。成员长度/时间估算也保留为非权威摘要，不转换成定时命令。`timingEvidence` 标签原样保留，标签文字自身不是对其真实性的认证。
- 未知成员要求或与手动/未放行相矛盾的成员声明拒绝；inspection 进场/巡测视图和 routeSections 是已知描述字段，只由规范出航/返航路径构造载荷，不将描述视图再上传一遍，也不推断缺失的成员路径。

最先预检原始成员/路径数量，然后才解析点和分配输出：每架机出航加返航最多 2048 点、全批次最多 8192 点。成员/计划/约束 Map 最多 64 字段、点 24 字段、包装 Map 16 字段。任何错误丢弃整个批次，只返回有界错误定位及空 `payloads`，不接受部分前缀。

## 传输载荷与指纹

成功返回 `payloads` List，每项含 `vehicleId/role/items/canonicalWireItems/wireSchema/contentHash/outboundPointCount/returnPointCount/itemCount/routeEstimates/estimateSummary/sourceSchedule/wireQuantization`。`items` 使用仓库 `QGCMAVLink.h` 所包含枚举 `MAV_CMD_NAV_WAYPOINT` 和 `MAV_FRAME_GLOBAL`；没有硬编码第三方协议编号。坐标高度进入 param5–7；param1–3 为零，autoContinue=true，isCurrentItem=false。原始 `items` 始终保留未预量化的经纬度 double，以供原 PlanManager 恰好量化一次。

每项还保留 `coverageRequirements`。若源计划有覆盖规划，必须包含已验证的显式巡航速度 0.1–300 m/s、AMSL 巡测高度 -1000–20000 米且不超任务海拔上限；爬升速度仅输入明确提供时保留并检查 0.1–20 m/s，不补默认值。源参与机 ID 必须是唯一的已分配 Scout/Leader 子集。`available` 表示源要求存在，`appliesToMember` 区分覆盖机与辅助机，所有结果 `encoded=false`；Relay/Responder 不因共享元数据被赋予覆盖速度要求。老版本缺少这些显式要求的 coveragePlan 必须重新规划/审阅，不从估算反推速度。新增源字段经编译器整个 coveragePlan 和计划版本库进入批准版本身份；wire `contentHash` 仍只代表实际几何编码，不能证明速度被执行。

特别注意：仓库 MAVLink 枚举对 waypoint param4 定义 NaN 为沿用系统航向模式，**零不是“未指定航向”，它会请求零度航向**。JSON 安全载荷因此显式使用 `param4=null`、`yawMode=system-default`（C++ Map 的 null 为无效 `QVariant()`，不是空 QString/数值）。传输适配器必须仅对这对已校验声明转换成 NaN，回读时再将 NaN 规范化回同样的 `QVariant()`；禁止通过通用 `toDouble()` 把 null 变成零。此类不会把 NaN 放进 JSON 或指纹输入。

`canonicalWireItems(items, fromReadback=false)` 是同一个有界纯静态帮助函数，返回 `{ok, items, contentHash, wireSchema}`。规范 wireSchema 为 `mission-item-int-geometry-v1`，每项字段恰为 `sequenceNumber/command/frame/param1/param2/param3/param4/param7/latitudeE7/longitudeE7/autoContinue/yawMode`。它不是对所有飞控、固件版本和旧浮点 MISSION_ITEM 的兼容承诺；上层必须独立限制已确认 MISSION_INT 的实际适配能力。

参照仓库 PlanManager 的 MISSION_ITEM_INT 写入逻辑：param1–3/7 转 float32 再以精确等值 double 表示，原始经纬度乘 1e7 后朝零截断为 int32；编码序号从零连续。对真正解码自 INT 的回读项使用 `fromReadback=true`，将解码度数乘 1e7 四舍五入恢复原整数，而不是放宽比较容差。输入 command/frame 只能为 waypoint/GLOBAL，回读适配器须先按原 PlanManager 正常化 GLOBAL_INT 和 synthetic Home/序号；legacy float fallback 不能冒充该 schema。current-item 是飞行中变化状态，真实布尔校验后不进入不可变身份；autocontinue 仍进入指纹。

`contentHash` 是该规范 wire Map List 转成 QJsonArray 后，紧凑 QJsonDocument 字节的 SHA-256，不是原 double 或 NaN 的哈希。它是几何载荷身份，不是任务启动指纹、审批凭证、加密签名或回读成功证据。传输层必须实际回读后按该契约规范化并逐项比对；不能仅根据飞控 ACK 或原发送列表宣称回读匹配。

每点 `wireQuantization.deltas` 显式记录经纬度度数/高度米数的编码差值，并提供最大绝对差值、变化点数和 `exactGeometryPreserved`。原高度与 float32 编码高度都必须低于同一 AMSL 上限；量化向上越过海拔上限返回 `wire_altitude_ceiling_exceeded`，不截断/夹紧原计划。差值元数据不是净空、动力学或测绘误差认证，`clearanceCertified` 始终 false。

所有成功结果仍固定 `executable=false`、`temporalSemanticsEncoded=false`、`endActionEncoded=false`、`flightCommandReleased=false`，并返回阻断限制：未编码权威时序、速度/爬升、截止时间、同步启动、起飞、降落、终止动作、围栏、机上角色逻辑和真实净空。coveragePlan 的扫描线方向不是机体 heading；巡航/爬升参数只参与上游估算，不在此伪装成已编码的速度要求。零接受半径/通过半径按飞控默认处理，但不能据此认证转弯、动力学或严格到点精度。

## 工程师后续验证（本轮未执行）

- Qt/平台真实编译；合法规范计划、包装路线、全别名一致与冲突、实际类型和各种边界。
- 任一点损坏/未知字段/超限导致全批失败；备用机、重复 ID、多个 Leader 或缺失返航不得穿过预检。
- 相对高度/本地基准/超海拔（包括 float32 量化后超限）、断开的返航、未编码 heading、损坏/冲突时序明确拒绝，不产生部分上传；合法时序虽保留且可地面准备，但始终未编码、不得启动。
- 系统默认 yaw 的 null→NaN→null 转换；不要因 QVariant 或 JSON 转换把 yaw 改成零。
- 地理坐标 MissionItem 与 MISSION_ITEM_INT、负经纬度朝零截断、浮点高度边界、current 变化、序号和 home 项差异的独立回读规范化；不把 MISSION_ITEM 浮点回退当作同一 INT 证据。
- 对照版本绑定上传服务核查：几何 ACK/回读成功也不能允许未编码语义的启动、解锁或放行。

这里只进行了源码编写与静态核对，没有编译、运行此类、仿真、测试、联网传输、刷机或放行。

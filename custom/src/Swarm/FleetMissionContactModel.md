# FleetMissionContactModel — 声明式支承接触几何

## 496：同次已应用TAKEOFF支承组件保留原起点参考

新增native TakeoffReference／Result.takeoffReferences：vehicleId、missionSequenceNumber、segmentId、referenceLower[3]／referenceUpper[3]、minimumReferenceAltitudeMm／maximumReferenceAltitudeMm、contactId、allowedTerrainCellIds、contactVolume。只在原成功applied、kind=vertical-takeoff且contact.kind=takeoff的support分支产生；missionSequenceNumber取已绑定encoded anchor的原contact实际22序号，不取vertical本来未知的toSequenceNumber。

两数组直接保原segment.from/fromUpper，不是application.referenceTo，也不移动捕获起点。min/max精确保原groundAmslMm＋supportBelowReferenceMm±referenceHeightToleranceMm整数窗口；不clip、不加epsilon或默认扩容。pad／terrain由本次同support组件直接复制，非caller可移植凭证、地面状态、来源认证或新接地豁免。Waypoint须在同一次normalized schema2／完整Joint调用中复核原投影及身份，并让整个full current编码区间落原窗口。

原prepare前置、所有来源绑定、旧components每段先protected后support恰两份、contactApplications／contactIssues及LandingReference定义和产出不变。合法规划只绑定但未applied不产此ref；全局context不完整不抹其它有效ref。任何后续失败／取消返回新空Result，包括两类ref均无前缀。prepare仍不执行solver；新增值提取不新增公开JSON／schema／token／权限。

新vertical/support只有指定terrain完整交集被同finitepad band覆盖才可例外；body／obstacle／top→NAV／Domain不借。旧名义几何有自身投影宽度外扩，不能宣称新参考体对每输入严格更大；它是另一个完整编码参考检查，不是实际动力学。见[Waypoint完整合同](FleetMissionWaypointSwitchReview.md)与[36项未执行场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 491：同次已应用LAND支撑面的原生参考材料

Result新增QVector<LandingReference> landingReferences。每项含vehicleId、missionSequenceNumber、segmentId、精确referenceLower[3]／referenceUpper[3]、contactId、allowedTerrainCellIds和contactVolume；仅在原成功applied的mission-terminal-land support组件生成。目标sequence取原segment.source.toSequenceNumber，参考点取原segment.to/toUpper（Z已由ground＋supportBelow替换），pad与指定terrain直接复用该support组件，绝不从LAND p7或独立caller地图重算参考。

prepare原前置仍是完整normalized schema2与已验证Joint；既有payload／动作／E7／wire-ground容差／名义from高于rest的校验不放松。缺LAND声明不造参考；不因全局contactContextComplete=false就误删其它有效LAND参考（例如另一个TAKEOFF缺证）。任意失败／取消返回全新空Result，包括landingReferences无可用前缀。

原components仍每段protected后support恰两份，identity／contactApplications／contactIssues、JSON报告和名义接地豁免均不改。本向量不是新凭证、当前位置、内部着陆阶段或可移植JSON接口。Waypoint只在同次输入内按完整vehicle／sequence／segment对应消费，缺参考使LAND模型不完整；其新完整接受参考体另受原预算和有限pad交集判据。完整prepare仍会构造原全部名义组件，新增消费有有界准备成本，不声称性能提升。见[Waypoint算法](FleetMissionWaypointSwitchReview.md)及[未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 481：原E7／毫米精确投影，不回转浮点度数

新增altitudeMillimetreBounds(value,lower,upper)封装原binary64位分解，给精确×1000 floor／ceil；非IEC559／非finite／超原−1000..20000域或输出无效时失败、不改输出。没有浮点乘1000后再补epsilon。

新增integerPointMillimetres(latE7,lonE7,altMm,frame,lower3,upper3)：核完整exact7声明frame／origin±delta／正比例尺，直接整数差×nm尺度；先限差≤2M、scale≤200M，乘积≤4e14nm，负数精确floor／ceil至mm。Z直接原mm，三轴≤±50Mmm，失败不发布前缀。旧pointMillimetres及接地匹配／豁免合同不变。

新IntegerDomain取高度界，Cover投影同原域所有角点；这是声明chart中的坐标包含，不认证该chart与实物位置／datum／机体关系。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 452 范围与限制

本模块把 `FleetMissionClearanceInput` schema 2 的工程声明和已经完整校验的 Joint 几何绑定，准备两类扫掠体；它不计算净空结论、不读取飞机、没有 I/O、没有权限或飞行命令。它不是 PX4 的接地控制器，也不证明飞机会在声明支承面停止下降。

schema 1 保持原单一机体模型及缺接地声明的阻断，不自动升级，不编造支承尺寸。schema 2 的配置、场景、接触区域和容差均为声明；配置指纹、资料版本及哈希只能约束内容一致性，不能认证实际机体、场景、测量时间或地面强度。原编译 pending 与实际启动许可缺口不由本模块解除。

本轮仅源码实现与静态对读，未执行编译、用例、仿真、UI、硬件或飞行验证。

## 原生接口与前置条件

头文件：[FleetMissionContactModel.h](FleetMissionContactModel.h)；实现：[FleetMissionContactModel.cc](FleetMissionContactModel.cc)。

- `pointMillimetres(point, frame, lower, upper)`：原地理点与声明局部坐标框架到整数毫米的向外包围。
- `polygonPlanes(points)`：已规范化的严格凸、逆时针多边形转换为闭半空间。
- `altitudePlanes(planes, lower, upper)`：追加闭高度区间两平面。
- `prepare(normalizedInput, jointView, cancellation)`：只接受已规范化 schema 2，以及调用方已经核完整形状、成员同序、协议序号、源映射、范围和预算的 Joint 视图。不接受任意 QML map 直接替代该原生前置校验。

Review 的 schema 1/2 路径共用本模块唯一的整数投影及平面实现，避免两套数值算法。`prepare` 不调用 `feasible` 或 `coveredByUnion`；取消在有界循环中检查，不能撤销已经发生的外部操作（本模块本来也不发外部操作）。

`Result` 是内部 C++ 结构，不是公开 JSON 许可：

- `ok / reason`：准备完成与否。取消、不支持的投影或扫掠域返回 `ok=false`，组件、LAND参考、应用和问题列表均为空，无成功前缀。
- `contactContextComplete`：所有本范围需要的接触上下文是否绑定完整，不是几何比较完成、实际接地或净空认证。
- `components`：每个原始 segment 恰两个，按原段顺序先 `protected`、后 `support`；不删除任何段。
- `landingReferences`（491）：同次已成功应用的LAND support原生参考，按原段顺序保留实际身份、精确rest上下界与有限接地材料；非JSON／凭证，不认证真实着陆。
- `contactApplications`：真正用于本范围几何的声明应用。
- `contactIssues`：缺少或不匹配的接触声明诊断。结构合法但声明不匹配时仍 `ok=true`，完整性为 false，相关段沿原 wire 几何保守检查。

每个 Component 保留原完整八字段 `identity`，以及 `componentId`、`componentKind`、`swept`、`contactId`、`allowedTerrainCellIds`、`contactVolume`。标识是原 `segmentId + /protected` 或 `/support`。只有成功应用接触声明的支承组件带非空接触区域；保护组件绝不取得接触豁免。

## 唯一数值投影

保留原 Review 的算法：纬经度必须可精确往返声明的 E7 形式，纬度在 ±80°、经度在 ±180°；相对声明原点的 E7 差不能越过声明范围。E7 差乘整数纳米比例，再作整数 floor/ceil 到毫米，负数不使用 C++ 向零截断作为 floor。

高度直接分解原 binary64 的符号、指数、尾数，尾数以整数乘 1000，得到严格 floor/ceil；不是先计算浮点 `altitude*1000` 再取整。支持 IEEE 754 binary64 布局并显式核 `radix/digits/min_exponent/max_exponent`，AMSL 数值范围仍为 −1000 至 20000 米。±0 代表同一数学零。失败不发布部分坐标。

每个端点以 floor 为整数参考，逐轴把两个端点中较大的 `upper-lower`（0 或 1 毫米）加到上下偏移。这是表示值的数值包围，不是新增物理安全默认值。极小非零高度、负高度或非整数毫米不得因取整缩掉包络。

多边形边 `a→b` 使用外法线 `(dy,-dx,0)` 与 `limit=dy*a.x-dx*a.y`；高度闭区间追加 `-z<=-lower` 和 `z<=upper`。边界接触保留给后续精确几何判断。

## 两类组件，不把整机都当起落架

横向误差为 tracking + scene + projection + safety；纵向误差为 tracking + scene + datum + safety，所有项均来自原声明。

- `protected`：XY 各为 `±(bodyHorizontalRadiusMm + 横向误差)`；Z 为 `±(bodyVerticalHalfExtentMm + 纵向误差)`。该声明应包住不可接触的机体、旋翼等部分，不能靠本模块从机型自动推算。
- `support`：XY 各为 `±(supportHorizontalRadiusMm + 横向误差)`；Z 为 `[-supportBelowReferenceMm-纵向误差, supportAboveReferenceMm+纵向误差]`。

两者再按上一节加入数值包围，调用 `Geometry::sweptBoxOffsets` 保留真实的不对称偏移，不把上下尺寸取平均或偷偷扩大为某个安全默认值。圆形水平半径用轴对齐方盒保守外包；这是工程声明模型，不是姿态、旋翼动力学或实际轨迹的证明。支承上方尺寸允许显式 0，不能因假值判断丢掉它。

## 声明如何绑定原动作

声明里的 `vehicleId + kind` 全表唯一，`payloadHash` 必须等于同序 `memberCoverage` 的原载荷摘要，`missionSequenceNumber` 必须等于原 `segmentSources` 的实际目标协议序号。没有取未来 planId 或给声明自身放自引用哈希。

### LAND

必须对应 `mission-terminal-land`，源为 `encoded-plan`、目标路径为 `executionProfile.endAction`。声明 E7 位置必须与原 wire `to` 相等。原 wire ground 高度整个毫米包围必须位于 `groundAmslMm ± wireGroundToleranceMm` 内。

只有这些条件成立，才为**声明净空模型**把参考点 `to` 的高度上下界同时设为精确整数 `groundAmslMm + supportBelowReferenceMm`；原 `from` 的下界必须严格高于该值。终点来自独立声明的支承面，不是原 wire p7 加偏移。`wireGroundToleranceMm` 仅核对请求与支承面的声明一致性；即使显式容差很大，也不能让模型在支承面上方提前结束。该变换不写回原 segment identity、原载荷或 PX4 任务，不发送新目标，也不认证实际 touchdown。失败仍检查原 wire 段，并记录不完整原因。

### TAKEOFF

声明先绑定 `mission-anchor` 的 `executionProfile.launchAction` 及原协议序号/载荷。规划范围不包含起飞现场，所以合法 TAKEOFF 声明可以只绑定、不应用；不生成 application，也不把规划范围完整性冒充完整启动审阅。

包含启动的范围还必须有实际原 `vertical-takeoff`、`takeoffSequenceVerified=true`。声明 E7 必须等于垂直段原 `from`；原 `from` 的完整高度包围必须落在 `groundAmslMm + supportBelowReferenceMm ± referenceHeightToleranceMm` 中；原 `to` 下界必须严格高于 `from` 上界。**不移动原生捕获的起点**。

同一成功绑定的 TAKEOFF 声明可用于原 `from` 数值完全相同的 `current-first-waypoint` 独立假设。`takeoff-request-connection` 不得到接触豁免；三个启动假设仍独立，不能连接后冒充实际飞行路线。`launch none` 没有 TAKEOFF 声明来源，不能借此取得起飞接触豁免。

多余或无法绑定的声明也必须报告不完整，不能静默丢弃。问题一般定位该成员首段；成员完全没有段时保真实 vehicleId，`segmentId=""` 明确表示无可定位几何，不捏造一段。每个真实段最多一条问题，空定位每成员最多一条。

## 应用与问题值

应用每条 exact6：`contactId, vehicleId, segmentId, kind, referenceToLowerMm, referenceToUpperMm`。kind 为 `takeoff` 或 `landing`；两个坐标数组固定三项 `[east,north,altitude]` 整数毫米。TAKEOFF 不移动端点，数组记录原段终点包围；LAND 保留原 XY 包围，高度上下界均为独立声明支承面加 `supportBelowReferenceMm`，仅用于该声明模型。

问题每条 exact3：`segmentId, vehicleId, reason`。固定原因包括动作绑定不匹配、缺声明、E7 不匹配、起飞原生来源未核、起飞高度容差不符、起飞段不向上、启动垂直段不可得、独立首航点假设起点不一致、LAND ground 容差不符、进场不高于声明参考点。

## 接触只作用于指定支承/地形交集

只有支承组件可携带 `padPolygon × [groundAmslMm-maximumSupportPenetrationMm, groundAmslMm]` 的闭接触体积，以及声明中绑定到实际场景、顶面等于该 ground 的 `terrainCellIds`。

调用方必须先形成**整个**支承扫掠与单个地形 cell 的交集，且仅对所列 cell 判断该完整交集是否被接触体积覆盖；不是看一个交点，也不是因为接触声明存在就跳过下降段。Unknown 不能当允许。

保护组件、未列 cell、障碍物交集均不豁免。两个完整组件仍必须接受 coverage/corridor/terrainCoverage 的全扫掠检查。误差或数值包围导致支承穿透超过显式深度，或支承越出 pad，都可继续拒绝；不自动扩大容差、接触区域或穿透深度。

## 本地 PX4 语义依据

- [PayloadBuilder](FleetMissionPayloadBuilder.cc) 的 TAKEOFF 是目标请求，要求等于首 outbound 点；LAND 的 p7 是独立显式 ground AMSL，低于同 XY 的末返程进场点。原 `sourceItemMap` 保各动作和每个航点的真实序号，速度指令不作为坐标。
- 配套 PX4 `src/modules/mavlink/mavlink_mission.cpp` 的 `parse_mavlink_mission_item` 把 z/p7 存为任务高度；GLOBAL frame 不相对 HOME。
- 配套 PX4 `src/modules/navigator/mission.cpp` 的 `calculate_takeoff_altitude` 在 landed 分支使用 `max(request, current global alt + MIS_TAKEOFF_ALT)`，并在当前 XY 插入垂直起飞；这不是地面支承面声明。
- 配套 PX4 `src/modules/flight_mode_manager/tasks/Auto/FlightTaskAuto.cpp::_prepareLandSetpoints` 把降落 Z 位置设为 NaN，提供向下速度；`MissionBlock::is_mission_item_reached_or_completed` 对 LAND 使用 landed 检测。因此 LAND p7 不是保证下降停止的高度。
- 接近地面的降速可用测距、HOME 高度或本地 Z 退化值；落地检测是运动、推力及滞回等状态逻辑，不是对声明 pad/起落架形状的认证。当前参数证据清单也不是全部降落行为配置的证明。
- `GLOBAL_POSITION_INT` 高度在该 PX4 源码中可来自 local Z/ref_alt 或 baro 退化，XY 与时间来自另一路 global position。收到的 `vertical.from` 是有来源约束的报告参考点，不能声称精确内部导航器同样本或支承接触点。

工程师仍须验证真实机体参考点、起落架/旋翼包络、场景与高度基准、固件/参数适配、落地/起飞动态、实时来源与最终发送围栏；上述纯声明模型不替代这些工作。

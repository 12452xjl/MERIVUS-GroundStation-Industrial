# 439：冻结启动审阅的名义空间视图

## 450：artifact 形成前的独立 geometryBasis 入口

新增纯值 `buildForGeometryBasis(source)`，与旧 `build` 共用同一 readiness 行解释、三类独立段提取和资源校验，不制造假的最终审阅身份。输入固定4键：`reviewKind`（normal／ground）、`geometryBasis`（小写 SHA-256 字符串）、`vehicles`、`minimumSeparationMeters`。后两者与旧合同相同，必须由实际原生捕获保存全部同次目标。

成功仍返回 `ok/error/view`，新 view 为14键：原 view15 **删除** `reviewId/reviewFingerprint`，**新增** `geometryBasis`，其它字段及逐机7键／summary6／segment8不变。旧 build/view15 保持原样，不能把 basis view 当旧最终审阅。新 `separation` 固定2键 `{available:false,reason:"not-run-in-geometry-scope-extraction"}`，只提取原几何，不重跑旧名义间距比较；旧 build 仍返回真实原 separation。

`geometryBasis` 来自原生调用者在最终 artifact hash 之前冻结的原记录、同次 readiness 几何和工程资料版本摘要；本函数只检查类型及范围，不认证来源、不授予许可。报告随后可包含在 artifact 里，不能把该 artifact 的最终 hash 再放回 `geometryBasis` 导致循环。新 Joint `buildForGeometryBasis` 专门消费此独立合同；原 UI／最终 reviewFingerprint 显示路径不改。

450仍未完成实际接地、构型和发送末端净空核验；新入口只是可复用前置几何通路，不是启动准入。仅源码／静态读回，未编译、测试、运行 helper 或设备。

`FleetMissionStartupSpatialReview::build(source)` 是纯值函数；`FleetMissionReviewCoordinator::startupSpatialReview(source) const` 只在自身线程转调，既不改 `lastReview`，也不发审阅信号，不取得实际 Vehicle、不请求报文、不刷新证据、不写文件、不发飞行指令。真实 normal/ground 审阅、完整目标集及源首尾绑定由 Root 负责；合法 UUID/哈希及本函数的形状检查不认证来源。

## 输入与结果

输入精确 5 键：`reviewKind`（`normal|ground`）、`reviewId`（规范 UUID）、`reviewFingerprint`（小写 SHA-256）、`vehicles`（1–32 个唯一机号）、`minimumSeparationMeters`（原始值，缺失须明确 null）。每个 vehicle 精确 2 键：数值整数 `vehicleId` 1–255、`readinessSummary` map。不得用另一批审阅、最新草稿、当前实时坐标或旧飞行报告补齐缺项。

返回精确 3 键 `{ok,error,view}`。输入信封、纯 JSON 类型或资源预算失败时 `ok=false`、固定错误码和空 `view`，没有有效前缀；成功 `error` 明确空串。`ok=true` 只代表完整显示投影已构造，不表示每架机几何可用、间距检查成功或允许启动。

view 精确 15 键：`schemaVersion=1`、`reviewKind`、`reviewId`、`reviewFingerprint`、`vehicles`、`segments`、`summary`、`limitations`、`minimumSeparationMeters`、`separation`、`snapshotOnly=true`、`executable=false`、`flightCommandReleased=false`、`clearanceCertified=false`、`timingKnown=false`。

每机精确 7 键：`vehicleId`、`launchType`（`takeoff|none|unknown`）、`availability`（`available|partial|unavailable`）、`reasonCodes`、`currentAltitudeSummary`、`takeoffDerivation`、`segmentIds`。保留输入全部目标与顺序。`summary` 精确 6 键：`vehicleCount`、`availableVehicleCount`、`partialVehicleCount`、`unavailableVehicleCount`、`segmentCount`、`allTargetGeometryAvailable`。这些状态仅描述几何显示完整度；高度越界、其它物理阻断、认证及授权仍按原审阅判断，不能把 `available` 显示为“安全”。

## 三种独立名义段

最多 96 段。每段精确 8 键：`segmentId`、`vehicleId`、`kind`、`from`、`to`、`nominalOnly=true`、`actualPathCertified=false`、`timingKnown=false`。`segmentId` 固定为 `uav-{vehicleId}/{kind}`；每点精确 3 键 `{latitude,longitude,altitude}`。

| kind | 真实摘要来源 | 含义 |
| --- | --- | --- |
| `vertical-takeoff` | `takeoffDerivation.verticalSegment` | 报告位置同 XY 的名义垂直段；2D 地图可能只见一点，必须结合 AMSL 高度明细 |
| `takeoff-request-connection` | `takeoffDerivation.subsequentConnection` | 从名义起飞点到 TAKEOFF 请求位置，**不是首个 NAV_WAYPOINT** |
| `current-first-waypoint` | `currentPositionToFirstWaypoint` | 报告当前位置到首个 NAV_WAYPOINT 的独立几何提示 |

E7 坐标须真实整数，纬度 ±900000000、经度 ±1800000000；除以 1e7 转为度。AMSL 保留原有限值，含支持域外的实际报告值，不夹到 -1000–20000，也不补 0 或换成相对/离地高度。各段不能拼接并命名为实际航迹。`launchType=none` 不派生垂直起飞或 TAKEOFF 连接；合法的 `derived=false,branch=not-computed` 是正常分支，不是字段错误。

原 `currentPositionAltitudeSummary` 精确 6 键、`takeoffDerivation` 精确 4 或 27 键保留供明细；其中编码上下界、名义数值和 false 认证标志保持原意。子对象、数值类型、固定声明、边界有无和共享起点的一致性均检查；格式错误的整行不发布任何段及原始错误 map，只留下该目标和原因。合法缺项或未知可保留其它**独立**有效段并标 `partial`，不会以截断前缀宣称整行通过。没有足够段则 `unavailable`。

未知标量使用 JSON null；Qt 原生 `QString()` 仍按空字符串接受，不把合法早退 readiness 误当 opaque 类型。公开返回经 `QJsonDocument` 往返保留 null，避免 Qt 5 的 invalid QVariant 进入 QML 成 undefined 后被 `JSON.stringify` 丢键。

## 间距提示与边界

`separation` 调用同模块体系的纯静态 `FleetAirspaceConflictChecker::checkNominalStartupSegments(segments, minimumSeparationMeters)`，不调用修改 `lastReport`/发信号的 `check()`。最小间距使用原值；缺失或格式错误不借默认值。外层仅将真实有限数值保留到 `minimumSeparationMeters`，其它值显示 null；检查结果自身保留拒绝原因。跨机名义空间接近候选不证明同时相遇、碰撞或安全，同机的三个独立假设不互查。

这不是完整原任务路线、返程、RTL/Home 路径、禁飞区、地形、障碍、动力学或 4D 时序审阅。编码高度区间只处理毫米遥测和 float32 量化，不包括估计器偏差、风或延迟位移。原审批、权限、排空与启动门禁完全不变；UI 必须继续显示原 blocked/unverified、过期与来源状态。

输入及最终输出均限制纯 JSON 树深度 12、100000 节点、紧凑 JSON 2 MiB；拒绝 opaque、非有限数字及超预算整份结果。不截取有效前缀，不生成假时间。

本批仅源码实现与静态读回，未运行 helper、解析器、编译、测试、UI 或设备。工程师仍须检查 Qt 5.15.2 构建、实际 normal/ground 正向形状、早退/none/未知/null、全部几何和数值边界、2D 垂直段展示、组内局部缺口、间距候选截断与源切换。

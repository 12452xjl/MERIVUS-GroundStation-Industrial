# StartupIntegerDomain 原生受控 E7／毫米域

## 537：逐 NAV 驻留的共享数值语义

原生整数域的完整canonical NAV param1复用 [DwellEncoding](FleetMissionDwellEncoding.md)，与实际载荷编码保持相同数值域，正值不能经微秒转换变成零等待。其它命令param1、完整字段和坐标整数域不放宽；不把驻留预算或合法性当成真实时间域／轨迹认证。

仅源码及限定直接静读，未运行helper／模型、编译、测试、仿真或修改PX4。实际工具链浮点语义、动态行为与最终飞行放行由工程师验证；下方536及更早为历史。



## 536：逐航点朝向编码与原生消费者接线

严格12字段 canonical wire 接纳使用共享 HeadingEncoding::canonicalParam4(false)，允许 NAV 的精确网格朝向；拒非网格、typed-null、非NAV明确朝向及模式冲突。读回误差窗口不进入已规范化模型；旧缺省 yaw 和速度保留零不变。移除已无调用的旧 constant 辅助函数。

E7／毫米域、源指纹、几何算法、预算及各认证／许可边界不变；输入保朝向不是认证真实偏航动态。

共享合同见 [HeadingEncoding](FleetMissionHeadingEncoding.md)。本批仅源码修改、直接静读与普通文本核对；未编译、运行实现／解析器／算法、测试、仿真、硬件操作、修改PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 481：实际运行门与完整外包使用同一不可变域

`FleetMissionStartupIntegerDomain` 是无 QObject 的纯原生不可变值对象。只有 [GeographicMotion](FleetMissionStartupGeographicMotion.md) 的实际 payload executionProfile 明确带 `startupSpatialGuard="integer-e7-amsl-mm-v1"` 且有与本次实参相等的 startupTransitionPolicy 时才选择它。不按 policy v1–v4、资料存在、旧报告或缺失字段自动启用。

新 Motion 的实际 `contains` 先用原生整数域检查收到的 E7 经纬和毫米 AMSL，再执行旧 Qt 走廊／高度／速度／航向的额外 AND 条件。故新接受域严格受完整整数域约束；不再需要把 Qt／libm 浮点接受域认作理想球面集合。此门可能拒绝旧 Qt 在完整整数外包之外接受的点，属于新模型语义，必须以新 source 身份、风险指纹和人工审阅体现，不能将旧档当成已升级。

本模块不声称 Qt 半径和本地理想球半径相同，不添加任意工程 epsilon，不修改飞控或原任务参数，也不授予执行许可。旧无 selector 的 Qt 模型及 source1/12 保持。

## API

`Ptr=shared_ptr<const FleetMissionStartupIntegerDomain>`；构造私有、不可复制，不提供从 source Map 恢复可信原生对象的入口。

- `prepare(GPI9,canonicalWireItems,postModeEnvelope,ceiling)`：检查完整来源标量形状、预算和数值范围后创建整个域；任一失败返回空 Ptr，没有前缀。完整载荷内容 hash 仍由 enclosing Motion 原流程验证，不由此工厂认证真实车辆来源。
- `cells() const`：同一对象的完整闭 raw E7 boxes；`Cell` 固定 minLatE7／maxLatE7／minLonE7／maxLonE7 和 segmentIndex／endpointIndex。
- `segmentCount()`：原完整地理链腿数。
- `minimumAltitudeMm()`／`maximumAltitudeMm()`：精确二进制64声明值导出的原闭 slab 整数内界，不是当前高度或机载地形。
- `contains(latE7,lonE7,altMm)`：要求收到的整数点落在完整 cells 并集，且 `[altMm−1,altMm+1]` 全部落入闭 slab。该 1 毫米是已声明编码外包，不是实际定位精度或物理跟踪误差。
- `source()`：完整有界 cells／slab／算法描述，不含 self-certified flag；不能由值相同代替原 native Ptr。

输入 GPI 为原完整 9 字段：经纬、AMSL／relative 毫米、三轴厘米每秒、headingCdeg、headingKnown=true。纬度限±80°，经度闭±180°，AMSL 为−1000..20000米；其余范围沿原 Motion。typed-null、字符串数字、非有限值、缺键和未知键不折叠为0。

wire 1..2048 项，每项原规范 12 个标量字段。完整核 waypoint16／takeoff22／hold17／land21 与 speed178 的命令、frame、默认 yaw、float32 参数和 E7 整数；地理 yaw 接受原 Builder invalid 或明确 JSON null，不接 typed-null 数值。speed 保留原序且不断开连接，不生成虚构0,0地理段。全部返程、起飞和终结项参与原完整链。

postModeEnvelope 恰原规范 7 键和范围，最低AMSL严格小于最高；horizontalCorridor .01..1000米。ceiling 独立要求有限−1000..20000米。这里只验证实际消费合同，不观察 Vehicle、场景或时效。

## 精确高度域

复用 `FleetMissionContactModel::altitudeMillimetreBounds` 的原 binary64 位分解整数缩放，不先做有舍入的浮点 `value*1000`。

设 `lo=ceil(1000*minAltitude)`，`hi=floor(1000*min(maxAltitude,ceiling))`。这两个 ceil／floor 都针对原 binary64 值的精确有理数，不是将十进制文字重新解释。工厂必须满足 `lo+2<=hi`，否则没有任何可容纳完整±1毫米编码区间的整数中心，返回失败，不让空域成为通过证明。实际门用整数 `altMm−1>=lo && altMm+1<=hi`；有界原 altitude 先验证，故加减1不溢出。

domain 的 slab 可供上层完整外包；不得按当前点、missionCurrent、p7 插值、HOME 或 Qt 计算收窄。相对高度、速度及航向仍为原额外观察门，不据其推断未观察轨迹或停止距离。

## 完整水平域与整数界

复制 [DomainCover](FleetMissionStartupDomainCover.md) 已有完整两帽 raw-box 算法，不调用 `distanceTo`／`azimuthTo`／三角函数，不读工程 frame。

每腿以原 E7 精确整数算 `dLat=abs(lat2−lat1)`、`dLon=min(abs(lon2−lon1),3600000000−abs(lon2−lon1))`。理想半径6371000米下，每 E7 的经纬路径长度小于12毫米，故 `L=12*(dLat+dLon)` 是整段最短球面弧长上界。每个理想 tube 点可由某个端点半径 `B=ceil(L/2)+ceil(1000*corridor)` 的完整球帽覆盖；这不是只取两个端点。

纬度半宽 `ceil(B/10)`，仅与原合法±80°相交。经度下界按整个纬度闭外包的最大绝对值选择 k：≤30°为6、≤45°为4、≤60°为3、≤65°为2、其余至80°为1毫米／E7，再取经度半宽 `ceil(B/k)`。该 k 来自原两帽理想集合的纸面球面下界，不是 Qt 数值误差预算。完整推导见 DomainCover 的468记录。

经度圆周必须处理跨越或恰触±180°，两侧闭编码都保留，包括接缝另一侧零宽 box；半宽达到180°取整个合法经度域。每端点至多两个 boxes，每腿至多4个，2048腿至多8192 cells。保留每个原腿／端点身份，不去重删除看似冗余部分。腿长上界≤40.8e9毫米，帽半径≤20.401e9毫米，全部求和差／取整在 qint64 内。

这里的 cells 现在也是新模型实际运行门的定义域：`contains=true` 按整数比较必有某一完整 raw box 包含收到的点，无需对 Qt／libm 求误差界。保守性仍可能使环境比较失败；不能裁工程 frame、缩小 corridor、只保最大子域或把外包重叠称为已证实的真实碰撞。

## source 恰10键

`schemaVersion=1`、`modelName="native-integer-e7-amsl-mm-domain-v1"`、`coverAlgorithm="ideal-sphere-E7-two-cap-raw-boxes-v2"`、`sphereRadiusMeters=6371000`、`segmentCount`、`cellCount`、`minimumAltitudeMm`、`maximumAltitudeMm`、`altitudeEncodingPaddingMm=1`、`cells`。

cells 每行恰6键：segmentIndex、endpointIndex、minLatE7、maxLatE7、minLonE7、maxLonE7。描述完整保留全部 cells，无截断；本对象 source 紧凑 JSON ≤4MiB，外层 Motion 的完整 source 仍另受总4MiB约束。bounded source 不含 Vehicle 身份、当前时间、来源认证、净空或许可字段。Cover／Review 必须持 enclosing Motion 原对象的同一 Ptr，并检查全部 cells/slab；报告结论不能仅由此 source Map 的字段决定。

只有原生同域完整外包和环境比较的真实消费者能形成针对新模型的包含证据；本工厂本身不调用 solver，不声称机体／datum／场景有效、物理净空、持续轨迹安全、StartPermit 或正式任务结果。仅完成源码与静读／文本检查，未执行本模型、编译、测试、仿真或操作硬件。

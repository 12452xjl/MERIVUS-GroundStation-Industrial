# FleetAreaCoveragePlanner 区域覆盖提案契约

## 478 · 完整原始边界策略与编码后路线复用

新增三个纯值接口 `buildRouteBoundaryPolicy(area,members)`、`validateRouteBoundaryPolicy(policy)`、`validateBoundaryPolicyRoutes(policy,members)`；都恰好返回 `{ok,reason,policy}`，失败 `policy={}`。策略恰好为 `{schemaVersion:1,active:bool,area:Map}`。无非空孔洞且退让为省略／精确零时是 `active=false,area={}`，仍先拒绝已知误置声明、坏孔列表和坏退让标量；不解析普通外环、不把覆盖专用 64 点限额套给旧普通 100 点区域，也不把旧区域元数据升级为新的围栏约束。

激活策略的 `area` 完整保留规范原始 `coordinateSystem`（`geodetic` 或 `localENU`）、`vertices`、`holes`、显式 `boundarySetbackMeters`，本地坐标另有明确 `localFrameId`。地理点只含 `latitude/longitude`，本地点只含 `x/y`；闭合重复末点按原解析去掉，原环顺序和数值保持。坐标直接来自原输入解析后的 double，**不从投影点逆算**，因此原字典序最小参考点及原边界不因规范化发生投影漂移。缺省区域帧只能由实际完整路线中的明确同源帧补足，不根据数值范围猜经纬度；本地帧不得空白、控制字符或超过 128 字符。

构建／独立校验都复用原 `readDomain(...,false)` 的完整有界拓扑、原外环／孔洞／投影规则，不构造候选退让域。独立校验先核每个环的实际List／3至64点、孔至多8／全部128点，以及每个点实际Map恰好两个规范坐标键和有限真实数字，之后才重建与完整紧凑JSON比较；不能把未知深树附在合法点上触发先序列化再拒绝。本地frameId还要求UTF-8往返不变。不接受额外字段、截短环、非规范别名、`active=true`却无实际约束或自报的`checked/certified/permission`标记。策略只是输入值，不是计算通过、可信来源、物理净空或飞行许可。

完整路线接口先校验策略，再实际调用原 `validateExclusionRoutes`，复用全部原边界与完整出航／返程／去返拼接段检查。[PayloadBuilder](FleetMissionPayloadBuilder.md)实际消费者传当前INT编码的E7 XY／float32高度完整坐标链，不是float32经纬度回退，也不把Compiler名义检查布尔值复制到编码结果。本接口只检查名义二维边界，高度上限和末端高度顺序另由Builder检查；实际顺序／末端动作／无坐标项过滤由真实消费者负责，本模块不决定上传协议、许可或执行状态。

工程师待执行（本轮没有执行）：严格主动／非主动规范形、误置／typed-null／坏型声明与额外字段；旧无约束 100 顶点兼容；根帧省略、根本地帧 ID 省略后由实际路线补足、混帧／本地 ID 冲突拒绝；地理原坐标与原投影参考点保持；孔洞单边／桥接／垂直段；编码后点移入孔或距边不足整份拒绝。未编译、运行算法、测试或仿真，不认证任何数值误差或实际安全。

## 477 · 正退让距离参与真实候选域、分量分配及完整路线消费

新增顶层 `area.boundarySetbackMeters`：可省略，提供时必须为有限真实数值 `[0,500]` 米，不接受字符串、布尔、typed-null、负数或非有限值。**仅省略或精确 `0` 走 476 兼容路径**，不使用 fuzzy-zero、不把正小数降为零。`bounds` 内、外环直接 List/所有已知别名的顶点内误置的同名声明均拒绝，包括根层没有孔洞或退让字段的情况。外环/原孔洞的简单性、严格包含、不相交/相切/嵌套及原 64/8/128 点限制不变。

正值不再调用原 `QGCMapPolygon::offset` 的相邻平移线相交近似，也没有引入新几何包。复用工程已依赖的 QtGui：每条原始闭合边与轴对齐正方形 `[-r,r]^2` 做八角点凸包，`r=boundarySetbackMeters+0.000016`；全部边缓冲用 `QPainterPath::united` 合并，再从原外环减去原孔洞填充及所有边缓冲。正方形包含半径为 `r` 的圆，因此它是**保守候选**，不是精确圆盘/欧氏 offset。仅使用直线和显式 `closeSubpath`，不用 Stroker 的圆弧近似。布尔运算可处理凹环、外扩孔洞合并/接外边界、颈部消失造成的分裂；原始孔洞自身仍不得重叠。

输出用 `toSubpathPolygons` 提取所有真实子轮廓，不用 `toFillPolygon(s)` 的 rewinding 桥接线。转换语义参考[Qt官方路径说明](https://doc.qt.io/qt-6.8/qpainterpath.html#toSubpathPolygons)；该文档不认证工程实际链接的Qt5版本、数值误差或线程／资源行为，须工程师独立核对。各轮廓全量做有限性、简单性/退化、相互触交与嵌套奇偶核对；不只保留最大一块，不截取前缀。每步布尔结果 path 最多 2048 个仅 MoveTo/LineTo 元素，派生域最多 32 环、总共 256 顶点；原始点仍受 64/8/128 限制。空域、数值冲突、接触的歧义拓扑、超预算均整份失败。这些运行库运算尚未编译/执行验证，不能把静审或 Qt 成功返回当作几何正确性认证。

扫描使用全部派生轮廓奇偶裁剪；每条腿记录真实候选连通分量，保持分量内部原扫描行/往返顺序。无显式端点时，确定性为每个分量先分配一架，再按剩余扫描段负载分配其他飞机；有端点时，按每个真实参与者 ID 的 `start/returnTo` 所在分量分配，两端必须属于同一候选分量，不能先随意按 ID 绑区域而拒绝本可交换的任务。每个分量必须有采样腿、至少一架飞机，每架至少一条腿。飞机不足、端点冲突、某分量未被间距采样或无成员时明确整份拒绝，要求调整成员/端点/间距，不静默漏掉分量。

**候选域不是最终检查依据。** 每条实际候选图边、直连、扫描腿及最终输出的完整出航/返回路线，均独立复用原外环/全部原孔洞的完整线段求交分段判内；同时计算与每条原始边的完整欧氏 segment distance，要求 `distance >= 原请求 m`，不使用 `distance+epsilon>=m` 放宽。距离采用点到段投影、完整交叉判断及四个端点到对段距离的最小值，不是顶点采样或包围盒。单点路线、同 XY 的垂直段、重复 XY 桥接均作为点到全部边的检查，不除零或跳过。大孔洞中心即使离边超过 `m` 仍被原孔洞内部检查拒绝。

正退让约束适用于**全部实际路线**：覆盖参与者、原有辅助路线、既有返回路径、出航末点到返回首点的拼接桥、端点进场/返回以及 Compiler 最终成员调用的 `validateExclusionRoutes`（包括没有 `coverage` 的剩余任务）。正值没有 `allowOutside` 例外；端点/路线外逸外环或距外边不够均拒绝，不借旧区外直连绕过。生成后重新核对实际坐标，包含地理逆投影再投影误差，不仅核对中间 `QPointF`。已有路线仍不自动改写。独立显式路线 API 只读取原域并检查完整原路线，不生成退让候选，因此不受仅用于搜索的正方形空域/32环/256点/2048元素限制影响；原域及路线本身的有界检查仍执行。正退让无孔洞也激活完整帧/路线核对，独立 API 恰好五项返回形状不变，`holesProvided` 仍仅表示原非空孔洞，不能解释成退让是否启用。

正值版本为 `plannerVersion=0.4.0`。保留原孔洞/原域面积字段（即使孔洞数为零），原 `areaSquareMetersApproximate` 仍是原名义净面积，另记候选退让面积，不混充实际观测覆盖面积。新增：

- `boundarySetbackMeters`：原始请求；`holeBoundaryPolicy=original-boundaries-full-segment-euclidean-distance-checked`。
- `setbackCandidateModel=axis-aligned-square-minkowski-edge-boolean`、`setbackConstructionGuardMeters=0.000016`。
- `setbackContourCount`、`setbackVertexCount`、`setbackComponentCount`、`setbackCandidateAreaSquareMetersApproximate`。
- `setbackComponentAssignments`：全部 `{componentIndex, vehicleIds, legCount}`，与真实逐分量分配一致，不伪造地理认证。
- `nominalBoundaryDistanceChecked=true`、`numericalErrorCertified=false`、`physicalClearanceVerified=false`；原 `exclusionClearanceVerified=false`、`executable=false`、`flightCommandReleased=false` 保持。

图使用全部派生候选点；原单次 32768 次完整线段检查和总 8192 保存点/1024 扫描行及扫描腿预算继续适用，并包含最终输出复核，不按分量或飞机重置。接近 256 点的图可能先耗尽原线段预算，整份返回明确失败。正小数的构造 guard 可能显著大于请求，某些欧氏可行路线在保守正方形候选域中仍不可表达；没有宣称搜索完备或最大覆盖。退让只针对输入名义二维边界及有界投影；没有认证浮点/投影误差、机体包络、跟踪误差、转弯半径、动态障碍、地形净空、观测覆盖或实际飞行许可，也不从飞机尺寸/能力档案自动猜一个距离。

工程师待执行（本轮未执行）：凸/凹域正退让、顺/逆方向、平行/近共线边及尖角；外扩孔合并/接外边、窄颈分裂/消失、全域消失、完整多分量及间距漏采样；参与者足够/不足、显式端点 ID 与分量顺序相反仍按真实端点分配、同机两端不同分量拒绝；扫描/直连/图连接沿候选边，原边距离不足的全段拒绝；大孔中心、跨孔/外环凹口、辅助/返回/剩余路线/拼接桥、单点和同 XY 垂直段；无孔正值也检查外边，旧区外起返点不能逃逸；精确零/省略旧版本身份兼容，非法标量/误置声明/极小正数不降零；本地/地理逆投影复核；32/256/2048/1024/8192/32768 临界及超限无前缀；元数据进入原完整计划指纹且无命令/权限恢复。

下方 476 为原有零退让路径的历史契约；其中“尚未实现安全退让”“可沿原边”“区外允许”仅适用于省略/精确零，不覆盖本节正值策略。

## 476 · 排除区域参与真实扫描、连接与显式路线检查

本批仍是纯地面站源码增量，未调用规划器、业务函数、解析器、编译、测试或仿真。`area.holes` 现在可以提供 `List<vertexList>`：最多 8 个环，每环去掉完全重复的末尾闭合点后为 3–64 个顶点，外环与全部孔洞合计最多 128 点。每个孔洞只接受顶点 List，不能是别名 Map、嵌套孔洞或多区域。孔洞点沿用外环的坐标输入/明确同源声明规则；`bounds.holes` / `bounds.polygons` 和点内误置的同名结构拒绝，不会被边界别名静默吞掉。

`validateExclusionRoutes` 的无洞 no-op 之前仍做固定深度的已知声明检查：顶层 `polygons`、`bounds` 内误置声明，以及外环直接 List / 全部 `vertices/points/boundary/coordinates/polygon` 已知别名内的点 `holes/polygons` 均拒绝。这是对已持有、隐式共享的列表做一层 `O(N)` 字段检查，不递归或深拷贝任意 QVariant 树，也不把覆盖规划专用的 64 点上限套给原本合法的普通显式路线区域元数据；原 ReviewWorker 的完整快照预算保持适用。无洞的普通 100 点区域仍可 no-op 通过此检查；非空孔洞进入 `readDomain` 后，外环/孔洞各 64 点及合计 128 点限制仍严格执行。不能靠根层没有 `holes` 绕过一个已知但放错位置的排除要求。

外环和所有孔洞使用同一参考点、同一有界米制投影和几何容差。每环都必须简单、非退化、无回折重叠边；孔洞严格位于外环内部，不能触碰或穿越外边界。孔洞之间相交、相切、重叠、嵌套均拒绝。拓扑核对使用全部边，不只是包围盒或一个顶点。数值判断复用原有约 `1e-6` 米局部容差，它不是测绘/定位精度认证。

真实扫描现对全部环的交点做半开边奇偶裁剪，在同一行扣除孔洞内部，随后校验每条扫描航段。真实可视图包括外环和全部孔洞顶点；每一候选边在与所有相关边求交后按区间中点检查完整名义线段，Dijkstra 连接不能仅因端点合法而穿洞。区内连接继续受外环约束；区外起点/返回点采用**另一份**允许外环外路径的图，仅绕开已声明孔洞，不会把区外边复用成巡测捷径。孔洞内部的起返点直接拒绝。所有辅助成员和已有 `returnPath` 的完整名义路线也在同一投影/排除区域/总预算下检查；原始路线和其中的时序、策略字段保留，不自动改写成一条绕洞路线。

独立出航/返回各自合法还不够：真实 payload 拼接会形成 `waypoints.last → returnPath.first` 的过渡边。生成前已有辅助路线、生成后覆盖参与者路线和独立显式路线 API 都复用 `returnBridgeAvoidsHoles` 检查这条完整桥接段；两端各自合法但桥穿洞时整份拒绝，不以文档限制代替实际消费。桥接检查也计入同一总线段预算。

**边界可以沿行，安全退让尚未实现。** 本批排除的是名义二维孔洞内部，不扩张孔洞、不内缩外环，不推断机体尺寸、定位误差、转弯半径、动态障碍、净空或安全飞行许可。外环外/垂直路径仍无障碍或地形净空证明。即使面积、航段和连接都生成成功，也不证明传感器观测覆盖、机间隔离或任务完成。

有非空孔洞时 `coveragePlan.plannerVersion=0.3.0`，保存完整规范 `holes`（每环仍是独立 List，保留原始坐标顺序而非逆投影重建）、`holeCount`、`domainVertexCount`、`outerAreaSquareMetersApproximate`、`excludedAreaSquareMetersApproximate`，原 `areaSquareMetersApproximate` 为扣除孔洞后的近似净面积；另有 `holeBoundaryPolicy=interior-excluded-boundary-may-be-followed-no-setback`、`boundarySetbackMeters=0`、`exclusionClearanceVerified=false`、`executable=false`。完整结果由现有 Compiler 保存并进入计划指纹。没有孔洞或显式合法空 List 时不添加上述新字段，仍使用原 `0.1.0` / 端点模式 `0.2.0` 输出；非法空值、错误类型并非无孔洞。

### 显式路线消费者

新增静态 `validateExclusionRoutes(area, members)` 与 `generate` 共用真实 `readDomain` 和完整名义线段检查，不生成扫描或路径，不调用 `generate` 来伪造规划。它返回恰好五项 `{ok, reason, holesProvided, holeCount, flightCommandReleased:false}`，成功的 `reason` 为显式非 null 空字符串。无孔洞/合法空 List 为 `ok:true, holesProvided:false, holeCount:0` 的兼容 no-op；误置或格式错误的孔洞失败。非空孔洞必须从每个实际出航/返回航点明确的 `coordinateSystem` 验证全组同一坐标系，本地每点或其成员须有同一个显式 `localFrameId`，点/成员矛盾、混合原点、超范围或缺失值均拒绝。成员上限 255，所有 `waypoints` 与存在的 `returnPath` 必须非空，合计不超过 8192 点。它允许路径在外环外，但禁止路径通过孔洞内部；缺少返回路径由原任务返回门禁处理，并不补一条返回路线。

此 API 供 Compiler 对**最终实际成员路线**调用，尤其是已经去掉 `coverage`、但保留原 `area.holes` 政策的单机/联合剩余任务。规划源、完整任务/计划身份和其他安全门禁仍由原消费者负责；返回 `ok` 不是 authority、净空或 StartPermit，不恢复旧 Run 或飞行权限。

资源仍有界：扫描线/总扫描段最多 1024，保存航点最多 8192，单次规划的辅助段、扫描段、两类图边/连接合计最多 32768 次完整线段可视性检查。域拓扑为有界 `O(V²)`；扫描交点为每行 `O(V log V)`；图初始化至多 `O(V²)` 候选边、每边检查全部环并排序切点，单次最短路为 `O(V²)`，`V<=128`。两张图缓存分离，候选点/边耗尽或复杂输入超预算整份失败，绝不返回前缀分配。独立显式路线 API 每次也有相同线段上限；不是把每个孔洞分别重置为一份新预算。

工程师待执行（本轮未执行）：凸/凹外环与凸/凹孔洞、顺/逆环方向、扫描过顶点/水平边/切线、同一扫描行多孔洞、多机完整不重复分配；全段穿洞但两端合法、沿洞边、外部进场/返回绕洞、内部端点拒绝、辅助/既有返回穿洞拒绝、出航与返回各自合法但桥接段穿洞拒绝；洞外逸/触外环/自交/互触/重叠/嵌套/退化；非法 alias/递归 Map/数值字符串/typed-null/混合坐标/错原点/日期变更线/超投影范围；根无洞但直接/别名外环顶点误置声明、根 `polygons`、`bounds` 误置声明不得 no-op 绕过，普通无洞 100 点区域不新增覆盖专用顶点限制；8/64/128、1024、8192、32768 临界与超限整份失败；非空洞完整坐标进入身份、无洞/空洞旧版本身份兼容；单机/联合剩余任务新进场与返回仍受原孔洞约束；生成/检查成功仍不触发上传/启动/飞行权限。实际投影误差、安全退让与转弯动力学仍需独立后续开发及工程师验证。

本模块为 Fleet OS 目标 6 生成可审阅的多机区域路线，不是飞行执行器，也不是 ADR-0001 的完整植保功能实现。没有引入外部几何依赖、地图绘图工具、Mission 上传或自动启动。

## 调用与输入

操作员/上游通过现有 `prepareFleetIntent(intent, parameters)` 审阅入口提供 `task.coverage`。编译器完成成员身份和角色归一化后调用纯 C++ `FleetAreaCoveragePlanner::generate(area, coverage, members)`。

`coverage` 字段仅支持以下选项，未知字段或错误类型均阻断，避免悄悄忽略任务要求：

- `strategy`：可省略，唯一支持值为 `lawnmower`。
- `coordinateSystem`：必须显式为 `geodetic`（WGS84 经纬度输入）或 `localENU`（x 东、y 北，单位米）。
- `localFrameId`：localENU 必须有非空字符串，最多 128 字符；geodetic 不接受该字段。
- `altitudeReference`：geodetic 必须为 `amsl`；localENU 必须为 `local_origin`。不把相对 Home 高度转换成海拔；其他基准需要后续适配。
- `altitudeMeters`：有限真实数值，算法输入范围 -1000 至 20000 米；这不是许可飞行高度。若任务有 `maxAltitudeMeters`，必须同时提供与路线相同的 `maxAltitudeReference`，编译器才按同一基准比较高度上限。
- `spacingMeters`：0.25 至 500 米；只是规划扫描间距，不证明摄像头/设备观测覆盖。
- `cruiseSpeedMetersPerSecond`：0.1 至 300 米/秒；只用于匀速时间估算，不证明飞机能达到该速度。
- `headingDegrees`：可省略为 0；从北顺时针，范围 `[0,360)`。
- `vehicleIds`：可省略为全部 Scout；显式提供时必须是非空、无重复的真实整数 ID 列表（1–255），且成员角色为 Scout 或 Leader。
- `vehicleEndpoints`：可省略；提供时必须是 Map，键恰好为全部覆盖参与者的规范十进制 ID，每项只能有 `start` 和 `returnTo` 两个点。点需显式坐标系、同一高度基准/本地原点以及有限坐标和高度；不能与已有参与者返航路径冲突。
- `climbSpeedMetersPerSecond`：端点高度不同于巡测高度时必须提供 0.1–20 的真实数值；即使不需要高度变化，显式非法值也拒绝。

输入不接受数值字符串、布尔数值、typed-null 或非有限数。`area` 支持由点 Map 组成的顶点 List，或 Map 中的一种 `vertices/points/boundary/coordinates/polygon`；也支持地理 `west/east/south/north` 和本地 `minX/maxX/minY/maxY` bounds。多个互相冲突的边界别名、顶点与 bounds 同时提供、经纬度与本地字段混用、多区域都拒绝。孔洞按上方 476 的顶层有界契约提供。点序末尾可重复首点，其余重复/自交/退化/重叠边界拒绝。

选中的覆盖成员不能同时自带路线，任务也不能同时提供全局路线。Relay、Responder 和未选中的 Leader 必须自带独立成员路线，不会被复制一条覆盖路线。辅助路线及已有返航路线必须显式提供与 coverage 一致的高度基准；本地路线还需同一 `localFrameId`，可在点或成员字段中声明。任务级公共/逐机返航回退先归一化成成员路径再校验，不绕过基准门禁。备用机不参与扫描分配。

区域及顶点若声明了坐标系或 `localFrameId`，必须与选项一致，不能重新贴上另一个原点标签。每份区域只能提供一个边界别名，重复别名不作深层 QVariant 比较，直接拒绝。辅助/返航输入另有合计 8192 点预算。

## 几何与输出

扫描线与外环/全部孔洞边求交并奇偶裁剪，凹形及孔洞一行可产生多个独立扫描航段；行向交替。按车辆 ID 排序后，将连续且不重复的航段按数量分配。连接段先检查完整直连是否在允许的名义域内，否则通过全部环顶点可视图和最短路径连接。连接可能沿边界，没有机体半径/安全退让，必须后续开发并由工程师验证，不能仅因人工复核就宣称获得安全距离。

地理点以固定参考点用 QGeoCoordinate 建立局部米制投影，拒绝跨日期变更线、纬度绝对值超过 80 度及距参考点超过 5 km 的区域；本地范围限制为距参考点 10 km。它不是有误差认证的专业 GIS 投影。

资源边界：每环最多 64 顶点、孔洞最多 8 个、所有环合计最多 128 点，1024 条扫描线/扫描航段、8192 个总保存航点、32768 次完整线段可视性检查。总点数包括既有辅助路线；端点模式还计入进场/巡测检查视图、完整出航与独立返航，不能靠复制路径视图绕过预算。超限返回失败，不保留一个看似完整的前缀分配。

输出 `assignments` 保留辅助成员路线，并为覆盖成员增加 `waypoints`、`coverageLegStart/coverageLegCount`、`pathLengthMeters`、`estimatedDurationSeconds`、`timingEvidence=constant-speed-estimate`。航点只带 `estimatedElapsedSeconds`，不伪造 `timeSeconds`。`coveragePlan` 保存参与机体、间距、扫描数量、近似面积、总路径长度、最长估算时长和限制说明，并进入编译计划指纹。

`coveragePlan` 还保留已验证的显式 `altitudeMeters`、`cruiseSpeedMetersPerSecond`，以及输入提供时的 `climbSpeedMetersPerSecond`。没有提供爬升速度就不补默认值；不能根据距离/时间估算反推出原批准速度。编译器按整个帮助类结果保存该计划，因此这些字段进入计划版本身份。它们仍是覆盖参与者的规划要求，不是已编码/强制执行的机上速度或高度控制；辅助角色不自动继承其覆盖速度。

端点模式的进场先在起点垂直调整到巡测高度，再水平连接巡测起点；返航先在巡测高度水平连接返回点，再垂直调整到返回高度。区内水平连接复用区内图；无洞的区外连接仍为旧 `outside-area-direct-unverified` 提案，有洞则采用独立外部图绕洞并标记 `outside-area-hole-interiors-avoided-no-clearance`。外部路径与垂直段均不证明净空。端点与区域必须在同一有界投影范围内，不推断起飞/降落。

端点模式还输出 `ingressWaypoints`、`surveyWaypoints`、独立 `returnPath`、`routeSections` 与长度/时间估算。分段索引为从零开始、端点共享的闭区间，`path` 区分出航与返航，`kind/phase` 区分进场/巡测/连接/返回。水平和垂直时间分别按巡航/爬升速度估算；返航 `estimatedElapsedSeconds` 独立归零，仍为非权威时序。返回点完全重合时可产生单点零估算提案，由审阅门禁处理，不伪造运动。

审阅协调器用完整出航及返航路线审查，匀速/爬升估算不升级为权威时序；能量储备取默认/显式储备、最长显式返航与最长返航估算的最大值。高度上限核对所有进场、辅助、巡测和返回点，不只核对巡测高度；未知高度/基准或超限均阻断。没有端点请求时保持原有覆盖提案，不代填返航证据。

所有结果固定 `manualConfirmationRequired=true`、`flightCommandReleased=false`。不同飞机拥有不同扫描航段，不意味着连接段无交叉，也不证明任务已覆盖、能量足够、避障可靠或可以实飞。

覆盖 `intent.preview` 黑匣子事件只保存计划 ID 和有界摘要，标记 `fullSnapshotIncluded=false`，避免完整审阅中重复路线树超过单事件资源限制。完整路线留在审阅 facade，不是日志摘要；证据链同时记录 `previewEventRecorded`，不会把记录失败伪装成成功。

## 工程师后续验证清单（本轮未执行）

- 矩形、旋转区域、凹形、颈部、闭合重复点、顶点切线及极窄条带。
- 自交、重叠、混合坐标/基准、日期变更线、超限、损坏数值和未知选项。
- 多机分配完整且扫描航段不重复，辅助角色路线不被覆盖；成员过多没有扫描航段时明确阻断。
- 凹形连接不得穿出区域；真实安全距离、转弯、机体尺寸和地理误差单独验证。
- 逐机时间、返航路径、估算标记经过编译/审阅不丢失；未知时序不跳过时间受限禁飞区。
- 生成路线不触发任何命令/上传；有明确人工确认也不能绕过运行时健康/权限/任务版本门禁。

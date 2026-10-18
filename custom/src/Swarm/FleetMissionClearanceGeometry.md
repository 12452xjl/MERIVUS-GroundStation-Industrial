# 全扫掠体与并集的精确整数几何核心（449；470更新）

## 490：完整原凸柱索引覆盖接口

新增静态C++接口 `coveredByIndexedPrisms(const Polytope&, const PreparedPrismIndex&, Budget*)`；旧coveredByUnion与索引API、私有PreparedPrism状态保持。返回仍为三态Verdict，不新增报告schema或许可。

先校验原完整query（包括额外／严格面）、预算及私有有效index；仅将bounds复制给候选遍历。完整index先前已校验并自持所有原prism，候选按原row序唯一返回，仅去掉与query bounds严格分离者，触边保留。对每个候选，以原严格CCW边的axis.maximum恢复原闭支持面，并保原上下Z；每柱3..16个XY面＋2个Z面，不约分／舍入原面。最终仍把原完整query交旧coveredByUnion，不以AABB替斜面或strict面。

数学范围：query包含于其bounds，严格分离的prism不可能覆盖query中的点，故完整并集与候选并集对该query的覆盖等价。空候选仍走旧union；原完整query可能为空，不能直接False。索引／重建／并集共用同Budget，新增每row／每面及Z面计工，尾部conclusion再核取消；无效、取消或耗尽仍Unknown。此处是源码与纸面集合关系静读，未运行数学实现或差分验证。

两个消费者新增coverage、corridor及独立terrainHorizontal索引；后者全Z为±51000000mm，不能使用terrainContact的顶面索引。原2M工作／2048片段不增额；index准备有成本，稀疏场景可能减少重复面准备，稠密／小场景可更贵或更早Unknown，不承诺普遍提速、功耗或真实净空。见[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 470：自持原始凸柱的完整候选索引

新增原生 copy-only `PreparedPrismIndex`，以及三个接口：

```cpp
static bool preparePrismIndex(const QVector<PreparedPrism>& prisms,
                              PreparedPrismIndex* result, Budget* budget);
static bool prismCandidates(const Polytope& query, const PreparedPrismIndex& index,
                            QVector<int>* result, Budget* budget);
static Verdict axisAlignedBoxIntersectsIndexedPrism(const Polytope& query,
                                                    const PreparedPrismIndex& index,
                                                    int originalRow, Budget* budget);
```

索引仅作 broad phase：**候选表示原凸柱 Bounds 与 query 闭盒相交，不表示凸柱已相交**。每个候选仍须经 indexed SAT，从同一 index 内保存的原行凸柱做469精确判断。接口不允许提交“索引甲的候选 + 另一份凸柱数组乙”，不因 AABB 命中直接发布 ProvenTrue。原任意 polytope、完整并集覆盖、接触规则、坐标域和数值／权限门禁不变。

### 所有权、全部准备与空索引

索引私有保存原行顺序的 `QVector<PreparedPrism>`、完整二叉 heap 节点、首叶偏移和有效位。每个节点只有 Bounds、occupied 和原 row；调用者没有 mutable getter、map／JSON恢复或有效位 setter。默认索引无效；显式默认 copy constructor／copy assignment，不声明 move，防止源对象仍 valid 而私有数组已移空。

`preparePrismIndex` 先保留原输入值，再把非空输出清成默认无效索引。只接受0–1024个已准备好的凸柱；**全部**原行有效位检查完成后才建树，不能让某个早期 Bounds 排除隐藏未验证末行。最终 `_prisms` 用 reserve 后逐元素 append 建立独立容器存储，不直接共享调用者原 QVector 的元素容器；因此调用者此前保留的可变元素引用不能事后改坏索引保存的凸柱／Bounds 对应。每个凸柱内部轴数组仍是私有不可变值，可安全沿 copy-only 类型共享。

0个凸柱是合法的**有效空索引**：没有节点、首叶偏移为0。候选查询仍须完整检查 query、索引和 Budget，成功返回空行列表；不能把空索引当成非法或碰撞。任何 indexed SAT 原行都越界并返回 Unknown。准备失败／取消始终无效输出，不返回可用前缀；完成赋值后再次复核取消，若已观察到则清回无效对象并 false。

### Morton 排列不代替几何

每个原凸柱 XY Bounds 的 doubled midpoint 分别为 `minimum+maximum`，加100000000后处于 `[0,200000000]`，严格小于2²⁸。交织两个28位整数形成至多56位 quint64 Morton key，**每个输出 bit 都扣原共享 work**；没有浮点中心、除法截断或坐标重新编码。Z不参与排列，但仍在所有真实节点 Bounds 和剪枝中完整保留。

初始记录保存 `{key,originalRow}`。可取消的 bottom-up stable merge sort 每次输出一条记录都扣 work，等 key 保持原行稳定次序；不在 `std::sort` 比较器中半途取消或返回任意比较结果。排列只决定哪些叶相邻，不能使任何几何行丢失、改写坐标或获得相交结论。

非空索引取不小于行数的最小2次幂 base≤1024，使用 `2*base-1≤2047` 个完整 heap 节点。按排序后的记录填叶，padding 叶 occupied=false，不制造原行；内部节点自底向上逐轴取已占用孩子 Bounds 的闭并集外包。每个叶或内部节点扣一次 work，无递归。原行仍是原输入列表的0起索引，不是 Morton 排序后的序号。

### 完整候选、闭边界与原行排序

`prismCandidates` 的 query 必须没有任何 extra planes，并通过原完整 Bounds 的 `±51000000` 毫米／min≤max检查。非空 planes、坏 Bounds、默认／失败索引或坏 Budget 都 false，非空输出先清空；不能只看可用 Bounds 后忽略 query 的其余约束。

两个确定的根节点快路均在**已完成全部上述校验**后，扣一次根工作再判定。若 query 与 occupied 根在任一 XYZ 轴严格分离，全部候选必为空，尾核取消后直接成功返回已清空的输出，不先分配遍历／候选数组。否则若 query 的闭 Bounds 完整包含 occupied 根的全部 XYZ Bounds，每个原凸柱的非空 Bounds 都在 query 内，此时逐原行 `0..n-1` 扣 work 输出全候选，不必遍历或重新排序。不使用密度／命中率阈值，也不能用快路绕过工厂的完整输入验证。空索引仍走完整 query 校验，不从数量直接返回。

否则使用显式 DFS 栈，每个访问节点扣 work。完整 heap 最多1024叶、深度10，DFS只保留每层一个待处理兄弟，栈预留12项而不是整个2047节点数组；这不是候选截断限额。只有某个 XYZ 轴出现严格 gap（query.max<node.min 或 query.min>node.max）才剪枝；等号接触保留，点／线／平面 query 不被丢弃。occupied=false节点不产生候选，原叶命中仅收其 originalRow。内部外包包含全部后代，所以被严格分离的节点不可能藏着真实凸柱交集；未被分离的叶全部保留，不取最近若干、限额截断或凭 Morton 距离猜测。

DFS结果用同一可取消 merge sort 按 originalRow 升序排列，再检查唯一性并逐行扣 work 转成输出；完整成功输出是**所有**唯一原行的升序列表，不按空间顺序改变原报告顺序。任何中途失败、工作耗尽或取消都不发布已搜到的前缀；最终赋值前后都复核取消，赋值后观察到则清空输出再 false。返回后仍可能取消，上层必须继续原请求身份／actual finished 和尾核。

`axisAlignedBoxIntersectsIndexedPrism` 校同一个索引及原行范围，扣一次索引门禁 work，再委托469精确 SAT 读取 index-owned 原凸柱；不复制／重做凸性验证，不对错误索引或原行回退到别的数组。默认索引、坏行、非空 extra planes、预算不足或取消只能 Unknown。

三个接口始终使用调用者同一份 Budget／只读取消 token，不 reset、不新增额度、不消耗 fragment。索引准备、Morton位、merge输出、树节点、候选查询和 indexed SAT 的实际 work 全部计入上层工作量；不能在另一份 Budget 中免费建索引。最坏候选仍可能为全部行，密集部分重叠场景可能增加索引／排序 work；没有运行或测得耗时、CPU、功耗改善，不声称所有场景更快。

本轮仅独立源码实现、纸面证明与静态读回，未编译、执行 helper／业务程序、测试、仿真或连接设备。工程师后续需验证空／单行／1024行、非法末行清输出、相同 key 稳定次序、padding、根全包含与严格分离、XYZ闭接触、退化 query、候选原行升序完整性、原行越界、外部旧元素引用修改后的所有权隔离、copy-only对象、merge／DFS／发布期间取消和共享预算不足。索引不认证场景、实际路径、原 Qt 数值域包含或最终 StartPermit。以下469及更早原合同继续适用。

## 469：一次准备的闭凸柱与轴对齐盒精确 SAT

新增原生 `Point2 {qint64 east,north}`、`PreparedPrism` 和两个接口：

```cpp
static bool prepareConvexPrism(const QVector<Point2>& footprint,
                               qint64 minimumAltitude, qint64 maximumAltitude,
                               PreparedPrism* result, Budget* budget);
static Verdict axisAlignedBoxIntersectsPrism(const Polytope& query,
                                            const PreparedPrism& prism,
                                            Budget* budget);
```

这是独立编写的整数凸几何实现；标准分离轴思想可参见 David Eberly 的官方论文 [Intersection of Convex Objects: The Method of Separating Axes](https://www.geometrictools.com/Documentation/MethodOfSeparatingAxes.pdf) 第2–3节。仅借鉴公开数学原理，没有复制其示例代码，不使用其浮点实现或将其论文当成本地源码已经验证的证据。

`PreparedPrism` 是原生值类型，缓存完整 Bounds、每条 footprint 边法线及所有顶点在该轴的精确投影上下界；有效位和缓存均私有，只有 Geometry 工厂可建立有效实例，不提供 map／JSON 恢复、外部轴数组或公开 setter。默认对象无效；显式默认 copy constructor／copy assignment，不声明 move，避免出现 valid 仍真、但轴数组已被移空的 moved-from 对象。调用者仍须保留并验证真实资料来源；私有缓存仅保证这份几何数值已完成工厂检查，不认证场景或机体。

### 完整准备与资源合同

`prepareConvexPrism` 先把非空输出清成默认无效对象，再校原共享 Budget 和机器整数支持，随后完整检查：

- footprint 恰有3–16点；每个 east／north 都在闭整数范围 `[-50000000,50000000]` **毫米**，并在任何差分或乘法之前完成全部点的坐标域检查。
- 高度上下界都在 `[-51000000,51000000]` **毫米**，要求 minimum≤maximum；允许零厚度闭柱。此底层允许值不放宽上层工程资料原有的 minimum<maximum 规则。
- 各边非零；对每条 CCW 边，用外法线 `(dy,-dx)`，所有非端点顶点都必须严格满足 `n·vertex < n·edgeStart`。这等价于原严格正叉积，拒绝顺时针、凹、自交、重复及共线顶点，不自动修复顺序或删除点。
- 每条轴的 minimum／maximum 扫描**全部**原顶点取得，不能只保存一个边面常数后忽略反向投影区间。所有边完成并复核取消后，才把本地完整对象置有效并复制到输出；复制后再检查一次取消，若已观察到则清回默认无效对象并返回 false。返回后仍可能发生取消，最终发布者仍须自身复核。

准备复杂度 O(n²)，一次最多289个当前实现所定义的 work 单位（入口1、坐标n、边n、逐轴投影n²）。调用者应为同一完整场景准备一次，再供全部 query 复用，不能每个 component 重做凸性检查，也不能把准备所耗工作藏在另一份新 Budget 中。准备失败不退还已消耗 work，不返回可用前缀；取消／非法值／工作不足都返回 false，非空输出仍无效，Budget 的 cancelled／exhausted 沿原语义区分。没有新额度或自动重试。

### 完整闭集合 SAT，而不是只看 AABB

query 必须是**没有额外平面**的 Polytope：所有 Bounds 在 `[-51000000,51000000]` 毫米且逐轴 min≤max。非空 query.planes 即返回 Unknown，不能忽略斜面、strict 面或把一般扫掠 zonotope 当成盒；这个范围拒绝发生在任何早期分离结论之前。未准备的 prism、坏 query、坏 Budget、资源耗尽或观察到取消均不能得到确定几何结论。

完整范围核对后，先检查闭 east／north／altitude 区间是否严格分离，再逐个准备好的 polygon 边法线精确求 query 的两个支持值。正法线分量的 minimum 选对应 Bounds 下界，负分量选上界，maximum 相反；只有 `boxMinimum > polygonMaximum` 或 `boxMaximum < polygonMinimum` 才判分离。**等号不分离**，保留边、顶点、面接触，也支持点／轴向线／平面等退化 query 和零厚度 prism。全部轴无分离才返回 ProvenTrue。

完备性的范围是 `prism = convexFootprint × closedZ` 与 `query = axisAlignedRectangle × closedZ`。三维相交等价于 Z 闭区间相交并且两个 XY 凸集合相交；XY 所需分离轴是矩形的两轴和凸多边形各边法线，已全部覆盖。不会拿这个结论扩展到旋转三维柱、任意三维 polytope 或非凸多边形。一般 `feasible`、`coveredByUnion` 和 zonotope 算法均保持原实现与范围；本接口不是并集覆盖证明，多个独立不相交结论也不能代替完整联合包含求解。

查询复杂度 O(n)，当前路径消耗入口1、XYZ至多3、边轴至多n个 work；每个早期 False 与最终 True 都经原 `conclusion` 重核取消。不分片，因此不消耗 fragment，但仍受同一完整 Budget 的合法范围及已耗尽状态约束。取消在返回后仍可能发生，上层仍须原实际 finished／身份尾核，不能把数学 True 当许可。

全部运算为有界 qint64，没有浮点、epsilon、归一化、平方根或采样。边法线每项绝对值最多100000000；footprint 投影绝对值最多10000000000000000，query 的两项投影和绝对值最多10200000000000000，严格凸性对应叉积幅值最多20000000000000000，都远低于 qint64 上限。坐标单位始终是整数毫米，`50000000`／`51000000` 毫米不是50／51米。

本轮仅源码实现、纸面推导与静态读回，未编译、执行几何函数或 helper、解析业务资料、测试、仿真或连接设备。工程师后续仍需验证严格凸性／非法后缀、双向投影分离、仅斜边能分离的 AABB 重叠例、点线面接触、零厚度、正负坐标极值、有效对象复制、默认／失败对象、非空额外平面拒绝、精确预算耗尽与取消。所有新 True／False 仍只对声明的数学集合有效，不认证来源、净空、起降动作、原 Qt 运动域数值包含或最终 StartPermit。下方452／451及449原合同继续适用。

## 452：非对称偏移盒的完整扫掠

新增 `sweptBoxOffsets(from,to,lowerOffset,upperOffset,result)`，将导航参考点与盒的上下界分开表达。三轴每个偏移均为闭区间 `[-500001,500001]` 内的整数毫米，逐轴要求 `lowerOffset<=upperOffset`；允许上下偏移同号、upper 为0、零宽度以及整盒退化为点。端点仍各在 ±50000000，输出 Bounds 仍各在 ±51000000。result 非空时先清为默认 Polytope，任何非法输入或输出超界只返回 false，不保部分几何。

轴界为 `min(from,to)+lowerOffset` 与 `max(from,to)+upperOffset`。斜面仍使用 `±cross(e_i,to-from)`，支持值精确取 `max(n·from,n·to)+Σ n_i*(n_i>=0 ? upperOffset_i : lowerOffset_i)`。没有浮点、半高向上取整、对称化或以整段 AABB 代替扫掠集合。该偏移支持值允许为负，不能改为绝对值。平移只改支持值，不改法线方向；零宽度情况下保留成对闭半空间，它们仍表达退化集合的仿射约束。

旧 `sweptBox` 先独立校验各 extent 在1–500001，再以 `[-extent,+extent]` 委托新接口。原合法输入的 Bounds、斜面顺序和支持值保持一致；旧接口不因此接受0或负 extent。两接口都只是固定小工作量的纯整数构造，不改既有 solver、取消 token 或权限；调用者仍负责每段前后的取消检查。

纸面例（未执行）：零长度段 `(0,0,0)` 配偏移 `(-2,-3,-7)` 到 `(5,4,-1)`，结果正是该非对称闭盒，而非以原点为中心的包络；上下界相差奇数毫米也直接保留，不产生半毫米中心舍入。三个下上偏移都相等时，结果为该固定偏移后的完整线段。这里提供表示 protected/gear 包线的底层能力，不证明其尺寸、接地或接触许可来源。

仅完成源码与静态/纸面审阅，未编译、执行几何函数、运行解析器或测试。

## 451：共享取消信号，取消不是几何结论

公开 `CancellationToken` 为 `std::shared_ptr<const std::atomic_bool>`。每个独立请求由拥有者创建一个 false 的 `std::atomic_bool`，计算持共享只读 token，取消方仅执行一次 false→true（建议 release store）；不能复位为 false、转用于新请求或用裸指针替代共享生命周期。`Budget` 新增 `cancellation` 和初始 false 的 `cancelled`，旧空 token 调用完全兼容。Budget 的计数及布尔状态仍只允许计算线程读写，取消线程只写其 atomic，不得并发修改 Budget 本身。

入口、每次 work／fragment、并集 volume／DFS 迭代，以及所有确定 True／False 结论返回前均检查 acquire load。观察到取消时锁存 `budget.cancelled=true`，返回 `Unknown`；它不同于 `exhausted`，不把人为取消标成资源耗尽或几何不满足。已经算出的分片不能成为取消后的成功前缀，严格面见证和全部并集逻辑不因取消而放宽。

这是协作式取消，不强杀线程、不清空共享数据、不触发 QObject／回调／I/O。固定尺寸整数单次运算不会被内部抢占；结论返回后仍可能发生取消，因此发布者必须在 actual worker finished 后复核同一请求身份和 token，不能仅凭已返回的 Verdict 发布旧结果。`sweptBox` 和 `sweptBoxOffsets` 仍是小型固定工作函数，无新增 token 参数；上层每段投影前后检查取消。

仅静态源码审阅，未编译、运行或测试取消时延／并发行为。下面449数学与范围合同保持不变。

`FleetMissionClearanceGeometry` 是纯 native、定长整数、三值几何模块，不是 QObject，不读取文件、时钟、Vehicle、地图或工程资料的可信状态，也不产生飞行许可。以下算法只对调用者提供的**毫米坐标数学集合**作判断。它不认证实际机体、场地、垂直基准、跟踪误差、起降接触或真实航迹。

## 固定公共接口与输入域

- `Plane {east,north,altitude,limit,strict}` 表示 `east*x+north*y+altitude*z <= limit`，strict 为 true 时改成严格 `<`。每个法线分量在 ±100000000，limit 在 ±20000000000000000；法线至少一项非零。全零法线不属于本接口，返回 Unknown，不能忽略或把它当成自动满足的面。
- `Bounds` 的 east/north/altitude 各 min/max 都在 ±51000000，且 min<=max，允许点、线、平面等退化闭盒。
- `Polytope` 保存 Bounds 和 0–64 个额外 Plane。求解时**始终**另加 Bounds 的六个轴向闭半空间，不把无界多面体的“找不到顶点”误判为空。
- `Verdict` 是 `ProvenFalse/ProvenTrue/Unknown`。`feasible(poly,budget)` 判断集合是否非空；`coveredByUnion(poly,volumes,budget)` 判断 query 全体是否包含于 volumes 的联合。它们的 True 都不是安全或准入结论。
- union 最多1024个体积，每体积必须有1–18个合法 **closed** 面；空 volume 不是全空间，整份拒绝。空 union 合法，非空 query 返回 False，空 query 返回 True。
- `Budget` 由调用者跨所有检查共享：`remainingWork` 初始2000000，`remainingFragments` 初始2048，`exhausted=false`。负值/超声明上限、已 exhausted 或入口无剩余 work 返回 Unknown；内部需要更多 work/fragments 时置 exhausted。非法几何、容量/额外面超限也返回 Unknown，但不伪称全部是预算耗尽。

所有公开输入先校界。不能通过 caller 预设“大预算”绕过资源上限。每个三面候选、每次顶点逐面代入及 bounds/union 分面工作都扣 work。DFS 根和每次 outside 分片尝试扣 fragment；一个 split 的 inside 前缀替换原片，不作为额外活跃片重复计数。不可行 outside 尝试也不退还计数。整个调用没有递归栈，也不取样、不丢弃困难片、不输出成功前缀。

## 精确扫掠盒，不仅是端点或 AABB

`sweptBox(from[3],to[3],extent[3],result)` 接收各坐标 ±50000000、各正半尺寸1–500001。result 存在时，无论成功失败都先初始化为默认 Polytope；失败必须按 bool false 处理，不能把该默认值当作计算结果。

构造的是平移盒沿完整线段的 Minkowski 和，即四生成元 zonotope：三个轴向盒生成元加线段方向。它的面法线来自所有非零生成元对的叉积，因此需要六个轴面，以及 `±cross(ex,dir)`、`±cross(ey,dir)`、`±cross(ez,dir)` 的非零方向。六轴面的支持值以 Bounds 保存，斜面支持值为：

`max(n·from,n·to) + |n.x|*extent.x + |n.y|*extent.y + |n.z|*extent.z`。

这是完整扫掠集合的半空间表示，不把轴包围盒冒充扫掠体。零长度线段自然退化为盒，没有额外斜面；平行方向导致的零叉积跳过，重复合法斜面不改变集合。旧接口的非零 extent 使原盒三维；452新增接口还支持退化偏移盒，见上节。方向分量最大1e8、端点最大5e7、每个偏移最大500001，支持值的绝对值保守小于1.52e16，处于 qint64 范围，仍再次核 Plane/Bounds 公共界限；超界整份失败。

## 256 位有符号整数与有理顶点

内部整数由固定8个 uint32 limb 表示256位无符号幅值，另有符号位。无 Boost、浮点、epsilon、除法或有理数舍入。零幅值符号规范为正；乘法把 qint64 幅值拆为两个32位块，单块乘积在 uint64 内，加法逐 limb 传播 carry；减法用 uint64 承载 `lesser+borrow`（最多2^32），任何越容量 carry/borrow 都拒绝。qint64 幅值取法对最小负整数也有定义，但公开值域远小于该点。

每组三个面使用六项展开计算 determinant；奇置换减，偶置换加。D=0 的三元组不提供独立顶点。其余用 Cramer 得到 X、Y、Z；若 D<0，则 D/X/Y/Z 同时取反。逐面计算整数残差 `nx*X+ny*Y+nz*Z-limit*D`，仅看符号，全程没有先除后比较。

在公开 n<=1e8、d<=2e16 的域内，D 的六项总幅值至多6e24，X/Y/Z 至多1.2e33，最终残差幅值保守上界4.8e41，小于2^139，远低于256位。Bounds 的单位法线不扩大此界。实现仍保留每个整数操作的溢出检查，不依赖纸面证明跳过错误处理。

可先用 Bounds 上的精确整数 min/max 支持值判定整个盒满足所有面，或某面排除整个盒。三项绝对值和至多1.53e16，安全使用 qint64。这只是充分条件快速路径；无法判定时继续完整有理顶点方法，不将整个盒不满足误当成 query 不可行。

## Strict 面不能只找一个严格顶点

先枚举所有面作为非严格闭约束时的 closure 顶点。无 strict 面时，任一完整通过所有面的顶点即证明可行。含 strict 面时，对**每一个** strict 面分别记录是否出现了某个 closure 顶点，其对应残差严格小于0。

这些见证可以来自不同顶点。因为每个见证满足其余所有 closure 面，取全部见证顶点的平均：对任一 strict 面，至少一项严格满足、其余项非严格满足，平均后仍严格满足；所有 closed 面也保持满足。因此全部 strict 面有见证即可提前 True，无需存在一个对全部 strict 面都严格的原顶点。

反过来，若非空严格集合中存在点，则每个 strict 线性函数在有界 closure 上的最小值也严格低于界限，并可在某个顶点取到。由此，**完整枚举结束**仍缺任一 strict 面见证，才可判空；预算中断只能 Unknown。Bounds 始终提供有界闭盒，退化 point/line/slab 也有足够独立活动边界面，不能直接因维度不足判空。

## 联合覆盖：有序严格差集

对当前非空片 P 和 closed 体积 `C=H1∩...∩Hm`，构造精确分解：

`P\C = (P∩outside(H1)) ∪ (P∩H1∩outside(H2)) ∪ ...`。

每个 outside 使用 `-n·p < -limit`，不是闭反面；每次 inside 前缀仍保留原 closed 面。这样邻接体积共享的边界不会形成假缝隙。每个非空 outside 在显式 DFS 任务中处理下一个 union 体积，完全 inside 部分丢弃。所有体积都处理后仍有已证非空片则 False；所有片都消耗完才 True。

先校验全部体积，避免一个早期覆盖结果掩盖后面的非法输入。在该完整遍历中同时核 query 的整个Bounds是否落于某个合法closed体积；若全部输入校验完成且已有此充分证明，可直接True，不耗根分片或枚举query顶点。不是要求所有联合都必须有单个体积包含Bounds：没有此快速证明时仍执行下面完整差集；输入后缀非法或遍历工作不足照样Unknown。

每个 outside 和新 inside prefix 都用同一个精确可行性检查；任一步 Unknown 立即向上传播，不把该片当空。额外面超过64、分片/工作耗尽都 Unknown。Bounds 能证明整个当前片在某面一侧时，可安全跳过冗余 inside 面或把整片继续交给下一个体积，仍不改变联合语义。

## 本批证据边界

以下仅为纸面解析用例，**未执行**，也不是算法实现已验证的测试结果：

- 斜段 `(0,0,100)→(100,100,100)`、三轴extent均1，其扫掠满足 `|x−y|<=2`。点 `(0,100,100)` 虽在整段AABB内，却不在扫掠体内；中心 `(50,50,100)` 则在扫掠体内。因此只用AABB可能误报，且只看端点会漏掉中段障碍。
- 该扫掠体的bounds为 `[-1,101]×[-1,101]×[99,101]`。两个闭盒只在x上分别取 `[-1,50]` 与 `[50,101]`、其余轴保持该bounds，联合覆盖整个扫掠，单个盒均不能独自覆盖。共享x=50面不应成为缝；若改成 `[-1,49]` 与 `[51,101]`，中心点就是明确未覆盖见证。
- 对bounds `0<=x<=1,y=0,z=0` 再加 `x<1` 与 `-x<0`，闭包顶点x=0／1各自不满足一个strict面，但二者平均x=1/2同时满足。此例说明低维集合和各strict面的不同见证必须保留，不能仅搜索一个同时严格的原顶点。

仅完成源码、纸面推导与静态文本审阅；未编译、执行数学函数、运行解析器/测试、打开UI或连接设备。工程师仍需独立验证整数实现、退化/接边/多体积联合覆盖、资源耗尽、调用侧投影与机体包线来源，以及完整产品接线。数学集合判定不能代替净空资料可信绑定、接触上下文、实际高度基准或最终 StartPermit。

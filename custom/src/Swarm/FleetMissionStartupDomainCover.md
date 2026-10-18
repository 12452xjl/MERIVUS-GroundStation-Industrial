# 启动完整理想球面域的保守整数外包（467；468更新）

## 537：逐 NAV 驻留的共享数值语义

完整覆盖模型的canonical NAV param1改用 [DwellEncoding](FleetMissionDwellEncoding.md)；原精确Float32、完整schema、非NAV参数、坐标及几何预算约束保留。正驻留塌为零等待不能由单独几何消费者通过。几何外包仍不是实际驻留轨迹、持续留在半径内或时间冲突豁免依据。

仅源码及限定直接静读，未运行helper／模型、编译、测试、仿真或修改PX4。实际工具链浮点语义、动态行为与最终飞行放行由工程师验证；下方536及更早为历史。



## 536：逐航点朝向编码与原生消费者接线

完整域的 wireCoordinate 同样调用 HeadingEncoding upload(false)，不再把所有地理 param4 强制要求 null。仅 NAV 精确 tick／absolute-degrees 增加接纳，起降／HOLD 的明确 yaw 仍拒。完整 canonical source 与各项身份保持，不能丢朝向字段后仅比较空间坐标。

整域外包与浮点误差预算、场景／接地政策、取消和资源预算不因朝向编码变化而放宽；真实转向和 StartPermit 未完成。

共享合同见 [HeadingEncoding](FleetMissionHeadingEncoding.md)。本批仅源码修改、直接静读与普通文本核对；未编译、运行实现／解析器／算法、测试、仿真、硬件操作、修改PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 481：原生整数域直接投影与旧理想模型分支

若原Motion有integerDomain()，Cover从该同一不可变Ptr读取全部Cell6和原整数闭slab，不从source Map重建。核source2／modelName／显式选择及完整domain描述对应后，逐cell用ContactModel整数E7/mm入口投影两角；正比例尺保证整个整数box的投影位于所得参考box。任何角点越frame／坐标域、取消或容量失败均无cells成功前缀，绝不裁到工程frame。

全部完成才置Result.nativeIntegerDomainCovered=true。该属性来自实际原对象和全量投影，不是输入认证位；旧无整数Ptr仍走原理想两帽算法，属性默认false。整个raw域保完整起始／航点／返程／终结链、反经线两片及独立AMSL范围，单模型2048腿／8192cells保持。

纸面包含关系：新实际contains=整数域membership AND 旧Qt／速度／航向等条件，因此接受集⊆原cells×slab；每个完整cell/slab的单调整数投影⊆本次参考box，所有box交原body／tracking／scene扩张进入环境审查。这个推导不要求旧Qt或libm误差界，也不证明真实动作或采样间连续轨迹。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 468：完整纬度范围决定经度下界，外包算法升级v2

当前外包算法名称为 `ideal-sphere-E7-two-cap-slab-outer-cover-v2`；理想球半径、完整路线并集、两帽半径B、原合法纬度域、完整AMSL带、API及容量上限保持。不是原运行source12模型升级：原 `qt-qgeocoordinate-startup-corridor-v1` 与 `contains()` 未改，numericContainmentProven仍false。

467统一用1毫米／E7的全球经度距离下界，在较低纬度会给出不必要地宽的经度矩形。本轮改为先完成原纬度外包，再令 `M=max(abs(minLatE7),abs(maxLatE7))`。M同时包含端点中心及所有允许query，不可只看中心纬度；经度半宽取纯整数 `ceil(B/k)`：

| 完整纬度外包的最大绝对值M | 距离下界k（毫米／E7） | 纸面cos下界 |
|---|---:|---|
| M≤30° | 6 | cos30°=sqrt(3)/2>6/7 |
| 30°<M≤45° | 4 | cos45°=sqrt(2)/2>7/10 |
| 45°<M≤60° | 3 | cos60°=1/2 |
| 60°<M≤65° | 2 | cos65°=sin25°>2/5 |
| 65°<M≤80° | 1 | cos80°>1/7（沿原短区间下界） |

证明：对两点纬度绝对值均不超过M，haversine给出 `sin(d/(2R)) >= cos(M)*sin(wrappedDeltaLongitude/2)`。经差角在[0,pi]，用 `asin(x)>=x` 及 `sin(x)>=2x/pi`，得到 `distanceMm >= Rmm*cos(M)*wrappedDeltaE7/900000000`，其中Rmm=6371000000。上述cos下界分别使该系数严格大于所选k（零经差只需距离非负），所以距离≤B必有经差≤B/k，整数向上外扩不漏点。

30°和45°的根式下界可直接平方比较；65°项令 `x=5*pi/36`，由 `3<pi<22/7` 有 `5/12<x<1`。`f(x)=x-x^3/6`在[0,1]递增，sin交错级数给出 `sin(x)>=f(x)>f(5/12)=4195/10368>2/5`；系列来源仍为 [NIST DLMF 4.19.1](https://dlmf.nist.gov/4.19.E1)。这些仅证明理想球面的纬度带，不提供Qt／libm误差合同。

`longitudeRadius=ceil(B/k)`不大于原B，全经度判断和跨／恰触反经线拆分均使用该新半径；`<=`／`>=`双闭编码仍保留。纬度外包、每腿至多四cells、qint64范围和完整frame失败边界均不扩大。两个端点可因其完整纬度外包不同而选择不同k；不能强行共用较小纬度端点的较大k。新报告模型名由DomainReview严格核验，旧v1报告不作为当前v2结果接纳。

源码直接静读及纸面界交叉审阅已进行；没有执行算法、构建、测试或性能测量，不声称测得CPU／能效改善或更少误报。工程师待验证新增±30／45／60／65°等号、中心在带内但cap跨带、南北对称、完整80°边界、每个k的向上取整、反经线零宽另一侧、整个cap越frame及末尾成员失败。下方467是历史说明，其中模型名v1、统一经度B半宽及第4／5步由本468替代；其未证明数值包含和非许可边界继续保持。

`FleetMissionStartupDomainCover` 为原启动模型的**全部理想球面水平走廊并集 × 独立 AMSL 高度带**准备整数 affine box cover。模型名称为 `ideal-sphere-E7-two-cap-slab-outer-cover-v1`，理想球半径为6371000米，地理端点按原整数 E7 的精确有理角度理解。它不是三维名义航线，不沿航点 p7 插值高度，不按当前任务序号截短完整路线。

**该理想模型不等于原 Qt 浮点 `contains()` 的接受域。** Cover 不调用 Qt 地理距离／方位角、三角函数或 `contains()`，没有 Qt／libm 的认证误差界，不把工程图表投影误差当成 Qt 数值误差。原[GeographicMotion](FleetMissionStartupGeographicMotion.md) source12 的 `numericContainmentProven=false`、`clearanceCertified=false` 和 `flightCommandReleased=false` 保持；外包准备成功不是原数值域包含证明、环境通过、机体认证、启动许可或物理路径保证。

外部源码核对：[Qt 官方5.15分支](https://raw.githubusercontent.com/qt/qtlocation/5.15/src/positioning/qgeocoordinate.cpp)的距离半径为6371.0072千米；本地原运行表达式在Qt距离后又用6371000米计算部分角度与距离。因此不能直接把后者的理想tube认作原混合表达式的接受域。该分支阅读不认证工程实际链接的Qt二进制、编译选项或libm误差；上述差异只是包含证明仍缺的具体依据，不是已经证明某次飞行越界。

## API 与来源

```cpp
static Result prepare(const FleetMissionStartupGeographicMotion::Ptr& originalModel,
                      const QVariantMap& normalizedClearanceInput,
                      const CancellationToken& cancellation = CancellationToken());
```

- 仅接受已由原生工厂准备的不可变 `Ptr`；不提供从外部 map 恢复来源的工厂，不重新 prepare 或改写原策略。实际 Witness／Txn／Review 的原生对象、完整 source JSON、payload hash、期限和权限仍由原调用者核对。
- 输入资料必须由上层为整组**完整规范化一次**。本 helper 复核根 schema1／2、scene13、frame7，以及实际消费的 frame 数值／范围；不重复遍历全部4 MiB资料，不声称重新校验未消费的 volumes／contacts／vehicles。
- source12、原 GPI9、规范 policy 和全部 wire row12 的结构／相关标量范围仍检查；完整编码 hash 与字节预算已由实际不可变模型及上层来源绑定检查，本 helper 不重新 hash，不用摘要认证飞机来源。
- 全部 waypoint／takeoff／hold／land 项参与原地理链，含返程和终结项。speed178 不生成段，也不断开连接。每机 wire 最多2048项，因此地理腿最多2048。

`Cell` 保存 `reference`（仅参考域的 `Geometry::Polytope`，无额外平面）、原 `segmentIndex`（0起）、`endpointIndex`（0为腿起点，1为腿终点）和原始闭 E7 box 的四个经纬边界。参考域尚未加机体、支撑、跟踪或安全余量，环境 solver 必须另行完整扩张与比较，不能将 reference box 当整机包线。

`Result` 为 `{ok,error,segmentCount,cellCount,cells}` 的原生结构；不是权限 DTO。成功时 error 为明确空串、cells 完整，cellCount 等于 cells.size。失败始终 cells 为空，不返回投影成功前缀。全 raw-box 规划完成前，两个计数保持0；完成后即使投影越界或取消，仍保留已知完整计划数量。因此失败的0不证明原路线为空，非零也不表示这些 cells 已成功投影或环境检查。

## 整数外包算法及证明边界

设两端点纬差绝对值为 `dLat`，经差取完整圆周上的较短差 `dLon=min(abs(lon2-lon1),3600000000-abs(lon2-lon1))`，均为 E7 整数。

1. 理想最短球面弧长不大于“先经线、后纬线”的一条连接路径。纬线单位长度不大于赤道；`R*pi/(180*1e7)` 每 E7 小于12毫米（用 `pi<22/7`）。因此整数 `L=12*(dLat+dLon)` 是完整理想弧长的上界，无需计算弧上中点或方位角。
2. 圆弧上的任意点到至少一个端点的沿弧距离不超过实际弧长的一半；到该弧距离不超过原 corridor 的任何点，由球面距离三角不等式可被某个端点半径 `B=ceil(L/2)+ceil(corridorMeters*1000)` 的球帽覆盖。两帽**联合覆盖整段 tube**，并非只检查端点；重合点、零长腿和非唯一的对跖最短弧也不省略。
3. 球面距离至少为绝对纬度角差乘 R。由 `pi>3`，每 E7 纬差严格超过10毫米。因此球帽的原合法纬度范围由 `centreLat ± ceil(B/10)` 保守覆盖。
4. 两点纬度都在原合法 `[-80°,80°]` 时，haversine 给出 `sin(distance/(2R)) >= cos(80°)*sin(wrappedLongitudeDifference/2)`。用 `asin(x)>=x`、`sin(x)>=2x/pi`（0至pi/2）及 `cos80°=sin(pi/18)>1/7`，距离毫米严格大于 `Rmm/6300000000 * wrappedDeltaE7`，又 `Rmm=6371000000>6300000000`，所以至少1毫米／E7。`sin(pi/18)>1/7` 可由 `sin x>=x-x^3/6` 与 `3<pi<22/7` 推得 `1/6-(11/63)^3/6>1/7`；sin 的交错幂级数可参见 [NIST DLMF 4.19.1](https://dlmf.nist.gov/4.19.E1)，此处只将其用于该短区间的纸面下界。故经度半宽直接取 B 个 E7 是保守外包。
5. 纬度仅与**原 position 合法域** `[-800000000,800000000]` 相交，不能裁到工程 frame。经度原编码范围为闭 `[-1800000000,1800000000]`，跨越或恰好接触接缝均拆两段并保留正负180°两种编码；恰好接触时另一段是零宽单点经线。B 不小于1800000000时取整个原经度域。每端帽最多两 box，每腿最多四 box，因此单机最多8192 cells。重复帽不静默去重或省略原腿身份。

第4步的严格距离下界用于非零经差；经差为零时使用距离非负性，该点仍在以B为半宽的闭经度区间内，不需要排除重合点。

以上证明只针对精确定义的理想球面集合，不是 Qt 实现的误差证明。两端帽和低精度经度界故意保守；外包可能比真实 tube 大很多。外包不落入已检查环境只能说明当前充分条件未成立，不能仅凭外包重叠宣称原域中必然存在不安全点。需要更紧的、有证明的完整外包或经原谓词确认的真实反例才能进一步判定；不得用名义段通过替代。

## 独立 AMSL slab 与整数 affine 映射

每个水平 box 均乘原 `postModeEnvelope.minAltitudeAmslMeters` 到 `min(postModeEnvelope.maxAltitudeAmslMeters, source.maximumAltitudeAmslMeters)` 的完整理想闭高度带；不由 p7、relativeAltitude、HOME 或名义起飞／LAND 路径收紧。这里对理想带做精确整数外包，不据此认证原AMSL浮点表达式到此带的数值包含；这仍属于numericContainmentProven=false的边界。不推断速度、航向或局部 NED 限制与该域的坐标关联，不把这些独立观察限制当作制动、姿态或未观察轨迹证明。

先用 [ContactModel::pointMillimetres](FleetMissionContactModel.md) 在原 frame 原点对 corridor 取精确二进制64高度缩放上界；该 helper 分解 double 位，并用整数精确求乘1000后的 floor／ceil，而不是对已舍入的 double 乘积补 `ceil`。corridor仍限原`.01–1000m`。每个 raw box 的最小经纬／最低AMSL角点和最大经纬／最高AMSL角点分别取得 lower／upper 毫米界。原 affine 两比例为正整数，因此这两个角点外包整个 box，原高度按同一精确方法外包。

两个完整 raw 角点都必须处于原 frame 声明范围；因各坐标独立闭区间，这也约束整个 box。任何点超 frame 都整份失败，不裁剪、不改原点、不删除跨经度接缝的另一半。毫米参考坐标另外限于 `±50000000`，为后续至多500001毫米的原包线扩张保留 Geometry 的 `±51000000` bounds 支持域；本模块不执行扩张或 solver，也不假定余量已经检查通过。

整数范围：纬差最多1600000000、短经差最多1800000000，和最多3400000000；L最多40800000000毫米，B最多20401000000毫米。所有 cap 原始边界、求和／相减均在 qint64 内。frame 只允许每轴 E7 delta 至多2000000、比例至多200000000纳米／E7，因此原 ContactModel 的整数乘积上界也远小于 qint64。几何预算与 exact union 比较由上层统一管理；Cover只有独立固定 cells／wire 资源上限，不为每个 cell 重置环境 work／fragment 预算。

## 失败与取消

固定原因包括 `startup-domain-cover-model-required`、`startup-domain-cover-model-invalid`、`startup-domain-cover-input-frame-invalid`、`startup-domain-cover-slab-invalid`、`startup-domain-cover-altitude-domain-unsupported`、`startup-domain-cover-cell-resource-limit`、`startup-domain-cover-affine-domain-unsupported` 和 `startup-domain-cover-cancelled`。空或不支持的高度带不自动成为“真空通过”证明。

共享 cancellation token 只读，沿原约定由 owner 单向置 true；入口、逐 wire、全 raw 规划结束、逐 cell 投影及最终发布前检查。取消不授予成功结果、不返回可用前缀，也不证明 owner 已过实际 finished。这里没有线程、信号、文件、SQL、时钟、真实遥测、执行命令或来源认证。

本批仅实现源码并进行静态读回／普通文本与空白核对，未编译、运行本 helper、解析业务资料、测试、仿真或连接设备。工程师后续需验证退化腿、speed连接、返程／终结项、对跖与反经线（含外包恰触正负180°时另一侧零宽闭经线 cell）、全纬经域、二进制64高度边界、完整 slab、frame 越界、预算与取消行为。最终原 Qt 数值包含、真实构型／datum／时效、完整 StartPermit 和任务结果仍未在本模块完成。

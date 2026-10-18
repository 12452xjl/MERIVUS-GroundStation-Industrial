# 原启动地理运动谓词的共享冻结模型（458）

## 537：逐 NAV 驻留的共享数值语义

完整canonical NAV param1改为共享 [DwellEncoding](FleetMissionDwellEncoding.md) 合法性校验，拒绝会在Float32或已声明PX4微秒转换中变成零等待的正值。已有exact Float32、完整12字段、模式、坐标、frame和非NAV参数限制不变；该运动模型不因新增驻留取得真实动力学、持续位置保持或StartPermit证明。

仅源码及限定直接静读，未运行helper／模型、编译、测试、仿真或修改PX4。实际工具链浮点语义、动态行为与最终飞行放行由工程师验证；下方536及更早为历史。



## 536：逐航点朝向编码与原生消费者接线

严格 canonical item 接纳改用共享 HeadingEncoding 的 upload(false) 分支：只允许 NAV_WAYPOINT 的既有 1/256° tick 与 absolute-degrees，不将原始回读的舍入窗口再次施加到模型输入。旧地理 null／system-default、速度保留零／not-applicable 保持。完整12字段、来源副本与指纹、原几何和政策仍完整保留；朝向字段不会被提取路径时丢弃。

这只接纳新的已编码朝向表示，不增加机头转向运动模型、实际路径认证、时序或启动许可。

共享合同见 [HeadingEncoding](FleetMissionHeadingEncoding.md)。本批仅源码修改、直接静读与普通文本核对；未编译、运行实现／解析器／算法、测试、仿真、硬件操作、修改PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 481：显式整数域与旧 Qt 谓词的实际 AND

actual payload 的 executionProfile 可明确选择 `startupSpatialGuard="integer-e7-amsl-mm-v1"`；存在时必须是真实非空精确字符串，并有与本次 normalizedStartupPolicy 完整相等的真实 Map startupTransitionPolicy。缺少 selector 保持原 source1/12 和 Qt 行为，不按 policy v1–v4、资料或历史记录默认升级；错误／typed-null selector 不当成缺失。

明确选择后，prepare 在原完整 GPI／policy／wire／hash及复制流程内创建唯一原生不可变 [IntegerDomain](FleetMissionStartupIntegerDomain.md)；`integerDomain() const` 只返回该同一 Ptr。它不从 source Map 恢复，也不通过 matchesSource 重建替换。新 source 恰14键：旧12基础字段全部保留，schemaVersion=2、modelName="qt-corridor-with-native-integer-domain-v2"，另加 startupSpatialGuard 和 integerDomain 的完整 source。source.numericContainmentProven 仍为 false，因为 source 不是计算结论或自认证许可。完整 source 总4MiB上限保持，不裁 cells／wire 后缀。

新实际 contains 在 position9 合法性之后、Qt／浮点高度／速度及航向累计之前，先检查原整数 E7／AMSL毫米通过该同一 IntegerDomain。失败直接返回false，不消费航向累计。成功才继续原 Qt 走廊和其余原条件，所以新实际接受域由代码保证落在完整 frozen integer cells×slab 内，而不是声称 Qt 半径／libm 接受域等于理想球面。新门可能拒绝旧 Qt 接受但位于整数外包之外的点，属于明确的新模型语义；模型版本、source、风险指纹与原人工审阅须反映该选择，不能替旧档补一个 numeric=true。

matchesSource 除原完整五类实参和 hash 外，须检查本次 profile 的 selector 是否同样明确有效、policy 是否相等，以及 selector 与当前对象是否有原 integer Ptr 一致。它保持原source14及完整 integerDomain 描述，只替换要比较的完整实参，不调用 IntegerDomain::prepare、不重算原 Qt 操作数，也不由仅相同域摘要放过不同完整输入。外部实际 profile 的完整绑定继续由上层承担。

此项真正接入实际运行门，不是仅新增报告 false 字段。Cover／DomainReview 必须完整消费原生同一 integer Ptr、精确投影所有 raw cells／slab且完整构型扩张，才可报告新受控谓词的空间数值包含；环境比较与违规另行独立核验，不能用包含结论代替环境通过。旧 source1 的 Qt 数值包含依旧未证明。来源、时效、动态净空、机载真实性、完整 StartPermit 及正式结果不由本工厂完成。仅静态源码和限定文本核对，未编译／执行模型／测试／仿真／设备。

下方471及更早是原无 selector 的 source1/12历史合同；其原 Qt 数值表达式和旧 policy1–4行为保持，不解释为新 source2 已认证。

## 471：真实不可变源与不重算Qt的完整参数对应

新增原生 `Models = QMap<int, Ptr>`，仅持有同一Review首次创建的不可变对象，不序列化指针、缓存段长或方位角。source12仍是完整值描述，不能由其相同推断另一对象具有同一次实际Qt运算数。

`prepare`在完整position／policy／canonicalValues／4MiB SHA校验之后，逐层独立复制baseline9、原规范policy、完整wire行及hash；map／list逐元素构造，QString值及keys按QChar数据和明确长度独立复制，不把外部既有QVariant元素引用或QString::data()缓冲继续共享给descriptor／baseline／heading。数值类型、正负零与原始UTF-16码元保留；地理param4的invalid仍只按原规则变为显式null。源码依据为[Qt5.15 QString构造实现](https://raw.githubusercontent.com/qt/qtbase/5.15/src/corelib/text/qstring.cpp)的独立分配／复制合同；不由该官方分支推断实际部署Qt或libm数值误差已获认证。

新增 `matchesSource(GPI9,payload,normalizedPolicy,ceiling) const`：复用原position、policy规范化、canonicalValues、完整载荷SHA及null yaw规范化，再核原source12对应的全部五类实参。它不调用QGeoCoordinate、不重算距离／方位、不重建缓存，也不采集Vehicle／时钟。保留完整wire中的speed和所有地理命令，不只看hash或成功前缀；typed-null坏yaw仍拒绝，不能因JSON同为null而扩大原合法输入。

有资料路径将本次原对象经两Review→Worker→实际DomainReview→Witness→Txn继续使用；无资料旧路径仍在Txn首次prepare。值型snapshot／resultMatches为结构核对可临时prepare，但该对象不得代替上述原模型或宣称证明原Qt运算数。

模型名、source12、contains原表达式／半径／运算顺序、策略v1–v4、航向累计和全部false不改。此项只固定未来数值包含计算的实际操作数与来源，不证明原Qt／libm接受域、实际净空、StartPermit或正式结果。仅源码及限定静读／文本核对，未编译、运行、测试或操作设备。下方459及更早为历史。

## 459：实际启动阶段复用同一个已绑定原生对象

本模型纯值API和source12不改。[Witness](FleetMissionStartupClearanceWitness.md)在原工厂校完整input2后逐机prepare并保存不可变Ptr，[Txn](FleetMissionStartupTransaction.md)新请求一次prepare核原beforeArm／完整payload／policy／ceiling；有资料须比完整source并持Witness原Ptr，无资料持本次Ptr。实际Transition只读原owner缓存，不在每次phase重建。

prepare成本被移出反复阶段重放，不声称已实测CPU／功耗改善。Ptr只保同一值模型，不包含Vehicle／权限／当前时效或可变heading累计；失效／阶段来源等仍经原完整门禁。source的numericContainmentProven及认证／释放false保持，原完整运动域包含尚未实现。下方458的数值与输入合同继续适用。

`FleetMissionStartupGeographicMotion` 只把原 `FleetMissionStartupTransition` 的球面走廊／高度／速度／航向判断提取为一次准备、重复读取的不可变纯值对象。它不创建 QObject，不采集 Vehicle，不使用文件或时钟，不修改策略、任务或原生来源，也不输出权限。

## 原生 API

- `Ptr = std::shared_ptr<const FleetMissionStartupGeographicMotion>`；构造私有、不可复制。共享指针只延续该纯值模型的生命周期，不代替实际启动 owner 或许可。
- `prepare(originalGlobalPosition9, actualPayload, normalizedStartupPolicy, ceiling)` 返回 Ptr；无效或超界返回空 Ptr，不发布部分路线。
- `contains(value9, previousHeading, observedHeadingTravel)` 复用同一模型。travel 仍由调用者针对同一条原生有序 journal 初始化为0并累计，不能逐叶、逐阶段重置。
- `source()` 返回有界完整的冻结描述副本，不提供从 map 恢复可信来源的 API。

策略使用 [ExecutionProfile](FleetMissionExecutionProfile.md) 共享 `normalizeStartupPolicy` 规则，要求规范结果与输入完整相等，支持既有 v1–v4，不添字段、不升级策略。初始位置仍为原九字段 GPI 风险值；高度上限仍为原计划的明确 `maxAltitudeMeters`。prepare 不代替 initial `contains`，不提前消费航向累计。

纯API另外明确拒绝 typed-null 数字／布尔，避免 Qt 将 null Int 等折叠为0；这是对非原生坏型输入的防御，合法原生值的数值模型与策略范围不变。

实际载荷只提取完整 `canonicalWireItems` 和 `contentHash`。每机1–2048项，每项必须为原规范12标量字段，分别支持 waypoint／takeoff／hold／land 或非地理 speed；不接受未知键、嵌套容器、opaque、typed-null 数字或非有限参数。param4 的地理默认 yaw 接受原 Builder invalid QVariant 和明确 JSON null，source 统一表达为明确 JSON null。其余编码参数完整保留，不反推或重新编码原上传项。

完整列表的紧凑 `QJsonDocument(QJsonArray::fromVariantList(items))` SHA256 必须等于实际 `contentHash`，与 [PayloadBuilder](FleetMissionPayloadBuilder.md) 原公式一致。不是只 hash 地理项或可用前缀；速度项虽不生成空间段，仍参与完整来源。描述独立上限4 MiB，先限制项目数／字段／数值再编码；来源哈希仅标识内容，不认证 payload 来自某架实际飞机。

## source 固定12键

`schemaVersion=1, modelName="qt-qgeocoordinate-startup-corridor-v1", sphereRadiusMeters=6371000, baselineGlobalPosition, canonicalWireItems, startupTransitionPolicy, maximumAltitudeAmslMeters, payloadContentHash, snapshotOnly=true, numericContainmentProven=false, clearanceCertified=false, flightCommandReleased=false`。

baseline 是完整原 GPI9；policy 是完整原规范策略；canonicalWireItems 是含全部速度项的完整原规范列表。它没有任务/审阅/Vehicle身份声明、当前来源、时间、批准或实际机载证明。调用者必须继续把它绑定原实际 payload、成员、审阅和来源代次。将来 worker 可用这份相同来源做全域计算，但本批的 `numericContainmentProven` 始终为 false。

## 原数值表达式保持

从原 GPI 经纬起点出发，按完整规范项顺序提取 command16／22／17／21 的地理坐标；speed178不生成段，也不断开前后连接。不按 missionCurrent 截取后缀，不省略返程或终结项，不把 Joint 的三种启动假设拼入这条地理链。

Qt `QGeoCoordinate::distanceTo`／`azimuthTo` 仍用于相同端点与观察点。原固定球半径6371000及 radians常数不变；仅缓存不变端点的 distance／azimuth 和声明限值。长度小于0.001米仍返回起点距离；沿程参数在段外仍取两端较近距离；sin值不有限或越出[-1,1]仍返回 infinity。内部 `atan2/sin/cos/asin`、运算次序及 `nextafter(closest,+infinity)>horizontalCorridorMeters` 拒绝规则保持，不换椭球／affine／epsilon算法。

高度继续使用原 `altitudeAmslMm/1000.0 ± 0.001`，独立核原最低／最高AMSL及计划ceiling；相对高度继续 `abs(relative/1000.0)+0.001`。水平速度继续 `hypot(abs(north)+1,abs(east)+1)/100`，升降速度继续 `(-down+1)/100` 与 `(down+1)/100`。没有新增或移除编码外包项，也没有把这些编码边界当实际定位误差证明。

v1继续原 headingCdeg／headingKnown完整相等；v2–v4仍委托 [StartupMotion](FleetMissionStartupMotion.md) 的 `headingMatches`，使用原基准、上一条有序 heading 和原 envelope。所有地理／高度／速度失败发生在航向累计之前；该 helper 保持失败不提交累计。v1仍不依赖 travel 指针。

## 明确保留的缺口

原允许范围是原球面水平走廊加独立AMSL区间，不是按每个航点的高度插值后得到的三维名义航线；Qt球面公式也不等于工程资料的 `engineer-declared-local-affine-geodetic-chart`。本批没有证明整个允许运动域加机体／跟踪包络包含于已检查净空空间，没有增加 affine 附加门槛，也没有默认误差预算。

后续全域求解必须使用本同一来源与实际数值模型，保留原构型／datum／时间／原生历史／Authority／发送门禁；LOCAL_POSITION_NED和ODOMETRY仍独立限制，不能由字段名称推断共同原点或与global相同采样。模型包含一个报告位置不证明未报告轨迹、物理制动、实际净空或最终 StartPermit。

仅完成源码与静态读回／文本检查；未编译、执行helper、运行解析器、测试、仿真、UI或设备。数值边界、Qt实现、整段顺序和实际调用链仍待工程师验证。

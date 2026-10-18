# FleetMissionExecutionProfile 显式执行意图契约

## 481：显式共享整数空间域选择，不升级旧 profile

profile 顶层仍 schema 1，只新增可选 `startupSpatialGuard`，支持值仅为实际非 null 字符串 `integer-e7-amsl-mm-v1`。必须同时提供由原校验器成功规范化的完整 `startupTransitionPolicy`；布尔、数值、null、空串、未知字符串及没有策略的 guard 整份拒绝，不删除坏字段或输出成功前缀。顶层白名单上限由 9 至 10；原策略 schema 1–4、字段、容差和规范化规则不变。

未声明该字段的旧 profile 输出仍不含它，不补默认、不自动升级，也不改原 12 项 `requiredProfileHash` 合同。选择通过逐机完整配置的新修订进入既有 Compiler、PayloadBuilder 和归档所保存的规范 profile，不增加执行命令或恢复旧批准。该选择使运行检查与审阅可使用同一个完整整数 E7／AMSL 毫米空间域，作为原 Qt 谓词的附加限制；真实动力学、实际路径、场景净空、数值全集证明和 StartPermit 仍是独立义务。

工程师待验证：旧无 guard 的规范化输出不变；新值完整保留；十字段 profile 正常进入下游；各类错误值／缺 policy 整份失败；policy 1–4 保原合同；编辑保存、历史／草稿往返和原内容身份随该字段改变。以上只列交接场景，未编译、运行模型／QML／helper、测试、仿真或操作硬件。

下方为历史记录。

## 480：profile不扩schema，独立动作参数进入当前证据

原profile／verticalPolicy／requiredProfileHash和公开19键观察仍只绑定原12参数，不把28项偷偷改成旧hash。新[ActionPhaseReview](FleetMissionActionPhaseReview.md)随当前native Evidence/schema4解释固定14动作参数，与原速率覆盖2项分开。

规范launchAction.type=takeoff及endAction.type=land实际消费兼容前置：外部setpoint注入使两者不兼容，RC辅助使明确LAND不兼容。报告ok只参数合同自洽；未新增默认动作阶段策略、精确纵向速度／时间承诺、当前内部状态或任何执行许可。原名义几何、接地条件与全部缺证阻断保持。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：旧12参数绑定保原义，总速覆盖证据另入当前审阅

`verticalPolicy.requiredProfileHash`仍精确表示原12个name/type/value内容，并未改成14集合，profile schema／adapter／字段／显式语义不变。原公开观察接口仍19键、parameterCount=12；在未绑定profile时也能独立读取这12项内容，避免循环前置。编辑器“当前12项”的文字保持真实合同。

当前VehicleEvidence schema3另观察两个ALL设置并按[纯覆盖模型](FleetMissionRateOverrideReview.md)核原8速率；guard来源、可用性及全部模型结果进入完整审阅与启动配置指纹，原12hash不能代替14项当前证据。新Refresh读14固定项，实际确认文案已明确扩展范围；不自动重绑旧profile／旧批准。

此增量不让垂直速度成为已编码命令或实际恒速／时序证明，完整起降相位／动态净空／StartPermit仍需实质开发。仅静读未编译、运行或测试；下方为历史。

下方为历史记录。

## 457：冻结启动政策复用同一纯形状校验

新增原生纯 `normalizeStartupPolicy(source, output)`，直接复用原 `normalize()` 所用的 v1–v4 `startupPolicy` 校验及原包线规则。失败清空 output，允许 source/output 为同一原 map 时先冻结原值；不构造虚拟完整 profile，不复制范围规则，不增加默认值或自动版本升级。

[整组启动内存 seal](../../../src/Vehicle/FleetStartupMemorySeal.md)只用它验证调用方原已冻结政策及自然过期预算；该入口不捕获 Vehicle、不编码任务、不构造许可或修改计划。原完整 normalize 输出／错误语义保持，不证明物理性能、原点、净空或最终 StartPermit。仅源码与直接静态阅读，未编译、运行或测试；下方为历史。

## 446：v4容差比较使用原报告的精确线性关系

策略版本、字段、单位、0–1000 m范围及规范化输出不变。实际v4高度检查改用[定长精确算术](FleetMissionAltitudeArithmetic.md)：直接组合原三个binary32高度／原整数毫米参考，不先把毫米除为Double米或把已舍入的三差值再相减。容差按配置中实际保存的binary64数值解释；不是无限精度十进制文本，也不增加epsilon、默认余量或参数。

原20键reference／sample与首次审阅、同scope连续性、身份和发送门槛不变。derived三差值仍按原decoder保留供显示及结构一致性核对，但不是门禁中间算术的输入。闭区间允许真正恰好等于容差，零容差只允许原数值关系真正为零。未编译、测试或运行；下方445原范围声明继续。

## 445：显式 schema 4 高度关系包线

新增精确九字段 `startupTransitionPolicy.schemaVersion=4`：继承 v2 全球/局部运动与 heading，HOME 仍为 `stop-and-review`，只增加完整 `altitudeReferenceEnvelope`，不含 v3 的 `homeUpdateEnvelope`。旧 v1/v2/v3 六/八/九字段及原语义不变；profile 顶层 schema1/adapter 不变，不自动升级旧批准或补默认容差。

新包线恰七字段，由 [FleetMissionStartupAltitude](FleetMissionStartupAltitude.md) 纯规范化：固定 `referenceBasis=reported-altitude-relations-not-origin-validity-or-same-sample-proof`，加 `maxAmslMinusLocalDriftMeters`、`maxAmslMinusRelativeDriftMeters`、`maxRelativeMinusLocalDriftMeters`、`maxAmslMinusLocalToOriginResidualMeters`、`maxAmslMinusRelativeToHomeResidualMeters`、`maxRelativeMinusLocalToHomeLocalZResidualMeters`。六个上限均为显式有限数字 0–1000 m，严格零保留；缺项、额外字段、bool/null/字符串数值拒绝。v4 不能带 v3 HOME 包线，v1–v3 不能夹带新高度包线。

原生被动 ALTITUDE 的同包高度关系、原 origin/HOME 残差与首次样本漂移是观察包线，不是内部 valid、同原点、同采样、传感器鲜度或最终许可。RAW 最大年龄/间隙沿所选 `maximumRawGapMs`，不从硬件能力猜容差。本轮不执行解析器、编译、测试、UI 或设备；实际完整消费者/发送 scope 的工程核查仍必要。下面438及更早段落保留历史版本范围。

## 438：启动策略全部字段的结构化输入

[策略表单](FleetStartupPolicyEditor.md)新增v1／v2／v3精确6／8／9字段的完整输入、原始JSON严格加载、范围／交叉约束反馈及明确采用；v3 HOME五个位移上限保留严格零。没有默认物理容差、隐式版本升级或原生schema更改。高级JSON沿原规范文本保留，未知字段／非法输入不能在转换中被静默删除。

[父编辑合同](FleetExecutionProfileEditor.md)保留完整profile保存、真实12参数重新观察和原revision CAS，新增实际草稿选择标识与callback前后真实backing Draft核对。来源变化保留编辑文字并禁提交，子表单未采用时不能保存旧策略。采用表单本身不保存Draft或授予启动权限。

此批补416“启动policy仍需手写JSON”的界面缺口。未编译、运行、解析、测试、认证、仿真、改PX4、刷机或打包；完整StartPermit、可信原点／净空、正式结果及原25目标其余实质工作继续。下方416及更早为历史，原native范围和未知态要求不变。

## 416：真实逐机编辑与参数内容绑定

[Draft](FleetMissionDraft.md)已为每个现有成员接入完整 profile 的精确 revision CAS。profile 顶层仍为原 `schemaVersion=1` / `adapter=px4-multirotor-mission-v1`，不改变本模块的合法规范化路径；新 UI 是该既有契约的输入入口，不是另一套执行架构。保留成员换角色保留 profile，reserve 不活动输出，删除重加不继承。

Planner 编辑器结构化提供巡航/返航段速度、自动爬升/下降要求、起飞/结束动作、明确 WGS84/AMSL 坐标及相对/绝对时序选择；范围只用于输入校验，不是安全推荐值。不从实时 Home、规划点或通用规划速度填执行值。完整 `startupTransitionPolicy` 仍是高级 JSON schema1、2 或3，包含该版本全部必填包线；不是每项政策已表单化。JSON 可留空以保存规划意图，但原受控启动门禁继续拒绝缺政策；不补默认物理容差。RTL/absolute-required 等可表达意图也不使原不支持的编码自动可执行。

高级 JSON 输入先核16384个 UTF-16代码单元上限，再 trim/解析；不是16 KiB字节文件上限。字段失焦与保存校验有可见错误，按钮保留键盘焦点；确认未预选，清除已保存配置另需两次明确点击并绑定原revision。隐藏时清确认/参数offer并换本次编辑token，换草稿或成员取消编辑；这只描述源码逻辑，不是已验收的界面行为。

`verticalPolicy.requiredProfileHash` 来自[只读实际12参数观察](FleetMissionReviewCoordinator.md)，不是手抄 cached Fact 或“哈希非空即可”。UI 先取得 `ok=true` 的参数 hash 与稳定 `observationFingerprint`，操作员明确确认，Guided 保存前重新读取并逐值匹配该指纹及 hash，才提交完整 profile。接口不会 refresh/写参数，也不把未绑定 hash 的 `parameterEvidenceUsable=false` 作为读取 hash 的循环前置。此后 Compiler、实际 native review、参数和目录门禁仍分别验证；保存配置不是批准或飞行许可。

参数 hash 绑定12项 name/type/value 内容，观察指纹另覆盖当前原生来源与语义 revision，二者用途不能互换。新 profile/政策通过计划与审阅 hash 传播，编辑使旧 review 失效；wire payload 未变也不允许复用旧批准。CAS 在通知后返回 false 可能已有本地内容提交，不应自动重试覆盖。

阶段仅416，未编译、测试、运行 helper/UI/认证/应用/设备、改 PX4、刷机、更新 PR 或打包。完整 StartPermit、固件目录操作入口、可信 HOME/local/净空与正式机载实例/结果仍未完成，原25目标/六类继续。以下旧批次保留为历史，不以旧“只有外部 profile 输入”覆盖当前入口，也不改变 schema1–3原约束。

## 320–324：显式 schema 3 HOME 观察政策

新增独立九字段 `startupTransitionPolicy.schemaVersion=3`：完整继承 schema 2 的局部运动、heading、RAW gap 和阶段时长契约，仅将 `homeChangeHandling` 明确写为 `bounded-observed-after-owned-arm-or-mode`，另加必填 `homeUpdateEnvelope`。顶层 profile 的 `schemaVersion=1` 和 adapter 不变。schema 1/2 仍各为原六/八字段和 `stop-and-review`，不自动升级、不补 HOME 包线、不接受额外 v3 字段。

`homeUpdateEnvelope` 由共享[HOME 观察 helper](FleetMissionStartupHome.md)规范化，恰有八字段，不接受缺项/未知字段、null、字符串或 bool 冒充数字：

- `referenceBasis` 恰为 `original-reviewed-home-observation-not-frame-rebase`。
- `maximumHomeUpdates` 为真实有限整数 1–128；`maximumPreModeRelativeAltitudeChanges` 为真实有限整数 1–256。
- `maxHorizontalDisplacementMeters`、`maxAltitudeDisplacementMeters`、`maxLocalHorizontalDisplacementMeters`、`maxLocalVerticalDisplacementMeters`、`maxPreModeRelativeAltitudeChangeMeters` 均为真实有限数字 0–1000 m。允许严格零，不造默认容差。

规范化结果完整保留该政策，沿原 Compiler、Builder、PlanStore、Readiness、审阅/审批/Authority 的完整 profile/record/sourceHash 链传播，不只比较可能不变的 canonical wire payload hash。新政策必须进入新的冻结计划和审批依据，不能给旧 v1/v2 批准扩权。实际有序 HOME/相对高度观察消费由[阶段契约](FleetMissionStartupTransition.md)负责；解析成功本身不放行任何过渡。

Readiness v3 前置要求真实 known local XYZ、已知正 HOME timestamp、两个 HOME 投影一致及规范 global 基线；缺口只阻断阶段候选，不伪造 unknown 为 known。v3 继续要求 v2 的实际 NED/NED、reset/estimator 与 ODOM 周期间隙。以上观察不证明 HOME valid_alt/z_valid、共原点/同样本、重新基准、未观察运动、障碍净空或最终 StartPermit。

本阶段仅源码与限定文本审阅，未执行解析器/helper、编译、测试、仿真或飞行。工程师待验证 schema 3 精确九字段/包线八字段、零边界/最大值/小数计数/多余字段拒绝、v1/v2 原规范化输出与 stop-and-review 不变、完整政策变更不能复用旧 record/审批/sourceHash。下方带旧批次号的进度说明是历史记录。

280–284 新增[显式局部运动与航向观察政策](FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

`static QVariantMap normalize(const QVariantMap& source)` 为无状态、有界纯静态输入规范化，返回 `{ok, profile, errorCode, errorDetail}`。失败没有部分 profile；错误文本不回显任意用户字段。它不读取 Vehicle/参数/权限，不分配可变 QObject，不联网、不上传、不解锁或启动。成功只证明输入契约有效，不证明固件支持、已编码或能执行。

profile 顶层最多九个已知字段，不接受未知字段、别名、null、数值字符串或布尔数值：

- `schemaVersion`：真实数值 1，规范化为整数 1。
- `adapter`：恰为 `px4-multirotor-mission-v1`。
- `cruiseSpeedMetersPerSecond`：必须明确，真实有限数 0.1–300 m/s。
- `returnSpeedMetersPerSecond`：可省略；提供时同范围，不补默认值。
- `timingPolicy`：恰为 `relative-dwell` 或 `absolute-required`。后者保留原要求，不能降级为估算时序。
- `verticalPolicy`：实际 Map，只允许 `mode=vehicle-parameters`、可选 `climbSpeedMetersPerSecond/descentSpeedMetersPerSecond`（真实有限数 0.1–20 m/s），可选 `requiredProfileHash`（64 字符小写十六进制）。未提供的速度/hash 不造默认值；模式声明本身不证明实时参数匹配。
- `launchAction`：只允许 `{type:takeoff, coordinate:...}` 或 `{type:none}`。
- `endAction`：只允许 `{type:hold|land, coordinate:...}` 或 `{type:rtl}`。
- `startupTransitionPolicy`：可省略且不补默认值。提供时只能是精确六字段 schema 1、精确八字段 schema 2，或精确九字段 schema 3/4（各自不同的必填包线）；原审阅与阶段消费见[阶段契约](FleetMissionStartupTransition.md)。缺政策可提案/暂存，但实际 Startup 拒绝；纯规范化结果不是飞行许可。policy 的版本不改变顶层 profile `schemaVersion=1` 或 adapter。

## 明确版本的启动阶段策略（280–284）

schema 1 保留原六字段和语义，不自动升级、不夹带新包络：`schemaVersion=1`、`homeChangeHandling=stop-and-review`、`headingHandling=preserve-reviewed`、`maximumRawGapMs`、`maximumPhaseDurationMs`、`postModeEnvelope`。

schema 2 恰有八字段：上述六字段中 `schemaVersion=2`、`headingHandling=bounded-observed`，并且显式提供 `localMotionEnvelope` 和 `headingEnvelope`。schema 3 在这八字段上按新节显式扩展。三种 policy 均要求真实有限的整数版本；字符串、bool、null、非整数、未知字段和另一版本的多余字段均拒绝，失败没有部分 profile。规范化输出完整保留所选策略，进入已有 profile 及下游原意图/载荷/审阅/sourceHash，不在启动时补默认或改变单位。

共有时间及全球运动包络不改变：

- `maximumRawGapMs`：真实整数 100–3000 ms；`maximumPhaseDurationMs`：真实整数 100–30000 ms。原 READ START、审阅和身份期限只能缩短，策略不能续期。
- `postModeEnvelope`：精确七字段。`minAltitudeAmslMeters` / `maxAltitudeAmslMeters` 为真实有限数 -1000–20000 m 且前者严格小于后者；`horizontalCorridorMeters` 为 0.01–1000 m；`maxRelativeAltitudeMagnitudeMeters` 为 0.01–21000 m；`maxHorizontalSpeedMetersPerSecond` 为 0.01–300 m/s；`maxClimbSpeedMetersPerSecond` / `maxDescentSpeedMetersPerSecond` 为 0.01–20 m/s。

schema 2 的 `localMotionEnvelope` 恰有七字段，不接受可选缺项：

- `referenceBasis`：恰为 `observed-native-odometry-ned-not-origin-proof`。
- `maxHorizontalDisplacementMeters`、`maxClimbDisplacementMeters`、`maxDescentDisplacementMeters`：各为真实有限数 0.01–1000 m。
- `maxHorizontalSpeedMetersPerSecond`：真实有限数 0.01–300 m/s；`maxClimbSpeedMetersPerSecond`、`maxDescentSpeedMetersPerSecond`：各为真实有限数 0.01–20 m/s。三个局部速度上限分别不得大于 `postModeEnvelope` 同名上限；相等允许。

schema 2 的 `headingEnvelope` 恰有三字段：`maxDeviationDegrees`、`maxAdjacentChangeDegrees` 各为真实有限数 0.01–180°；`maxObservedTravelDegrees` 为真实有限数 0.01–3600°。必须满足相邻变化上限不大于偏离上限、累计已观察变化上限不小于相邻变化上限；相等允许。不存在隐式偏航容差或角度/弧度自动换算。

这里的 referenceBasis 只是原生 ODOMETRY/NED 观察契约，不证明本地原点、Home/z 有效、真实北向、与 LOCAL_POSITION_NED 同一采样、认证或许可。schema 2 的运动/转向容差只能由阶段契约在真实 owned MODE 之后消费；MODE 前仍需维持原审阅基准，HOME 变化仍为 stop-and-review。缺失 ODOMETRY 不是自动通过，也不在本组件请求新流、安装新传感器或修改飞控。累计航向值是有界报文所能观察的变化，不证明包间没有更大运动。

数值解析范围不是机体性能承诺，更不是可靠性认证、完整任务闭环或最终 StartPermit。本组件不从包络反推出安全净空、不核实动力/续航、不放行 ARM/MODE、不认证固件分支，工程师仍需独立核查实际构型、数据流、采样间隙和执行约束。

动作最多两个字段；不需要坐标的 none/rtl 不允许夹带坐标。每个坐标恰有五个规范字段：`coordinateSystem=geodetic`、`altitudeReference=amsl`、真实有限 `latitude/longitude/altitude`，范围分别为 ±80 度、±180 度、-1000–20000 米。这里 geodetic 按本工程契约表示 WGS84；没有地图坐标、本地原点、相对 Home 或高度换算。解析范围不是飞行能力/飞行许可。

规范化保留完整策略要求，不补连接、起降点、返航速度或垂直速率，不把绝对时序、RTL、none 起飞策略伪装成可执行的相对驻留任务。任务编译器和载荷适配器需要进一步核查端点一致性、AMSL 上限、机型/实际固件与参数快照，并明确区分原意图、实际编码和执行许可。

下游端点语义：takeoff/HOLD 要求与对应出航第一点/返航最后点三维精确一致；LAND 坐标是显式地面目标，与返航最后进近点经纬度相同，但 AMSL 必须严格更低。二者均受同一高度上限；不把前一个 WAYPOINT 降到地面来伪造终点一致，也不推断地面高度。实际进近/降落净空、地面基准与 land_detected 仍需独立证据，规范化没有认证它们。

工程师后续核查（本轮未执行）：实际 Qt 编译；所有数值/字符串/字段数/未知与 null 边界；schema 1 六字段输出不变化且拒绝夹带新字段；schema 2 八字段和局部七字段/航向三字段缺项及额外字段拒绝；非整数版本拒绝；局部速度逐项超过全球上限拒绝；heading 两个交叉约束及相等/范围边界；完整新包络进入下游 hash 而不补默认；none/rtl 多余坐标拒绝；位置别名及混合基准拒绝；optional 字段不出现时不补值；成功输出不产生参数读写、命令、上传或执行权限。当前只做源码与限定文本静态检查，未编译、运行解析器或测试。

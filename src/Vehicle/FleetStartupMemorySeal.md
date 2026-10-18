# 457 / 460：原生启动阶段内存封条

## 490：必填原29参数的最终内存尾核

capture现在必填actualVehicle、actualPrimary、actualParameterManager、originalPrimaryOwner、originalExecutionParameters、originalStartupPolicy六参；没有缺省快照／旧三参旁路。Txn取原beforeArm.parameterSummary.originalExecutionParameters，入口固定PRIMARY弱控制块并初核，各stage复用同一原owner及原审阅值，不以新的参数读取或同值重捕替换。

原快照schemaVersion=1且exact6：legacyRows12、rateGuardRows2、actionPhaseRows14、arrivalRows1、schemaVersion和takeoffRampNegativeZero。既有codec先核每行14个标量、顺序、完整flag／类型／数值／manager epoch；Seal独立复制每个容器和QString buffer，无JSON往返，保留ramp signed-zero marker。

current()在原owners／time／完整状态相等之后，最后调用实际PM.parameterArrivalExecutionSnapshotMemoryMatch(...,30000)。它核原29值／contentRevision／当前Fact／manager epoch／实际原源与控制块、pending、类型／范围和原规则下的当前接收年龄；晚回调改变参数即使pending=false，也不能借其它状态未变过最终native queue尾栅。该核对无新读请求、Fact／Link虚回调、weak.lock、JSON或hash，沿既有Vehicle.commandContextCurrent→Txn整组seal链执行。

同值新PARAM_VALUE可按原ledger规则刷新年龄，不宣称任何新receipt都必失效；值变化／ABA修订／类型／source或RAMP正负零差异仍失败。原事务／审批30秒与phase期限不续期。只是本机已观察语义连续性，不认证实际二进制、内部控制器、真实轨迹、飞行许可或远端原子快照。原电池／EST／ODOM／ORIGIN／ALT等封条继续。见[事务](../../custom/src/Swarm/FleetMissionStartupTransaction.md)与[36项未执行交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

`FleetStartupMemorySeal` 是一次实际逐机阶段检查的不可移植内存封条，不是启动许可、身份认证、物理连续性证明或遥测解码器。仅 C++ 可取得 `shared_ptr<const FleetStartupMemorySeal>`；构造函数私有，没有 QVariant/QML/JSON 导入、导出、注册或恢复入口。

## 460：原生电池账本的末端覆盖

封条新增实际 `BATTERY_STATUS` 原生账本的内存身份：原源及版本 epoch、系统/组件与固件/构型、Link 弱控制块和四个来源 QPointer、接收深度、槽溢出/耗尽/移除标志、全局 receipt/semantic revision，以及每个槽的完整原生 value10、接收 token/receipt/revision、原接收时间和完整性标志。value10 包含 schema2 公开行的 charge/fault 已知位及完整 uint32 故障值；只有故障位变化也不能复用已检查的旧封条。即使尚未绑定或未观察，原空状态也固定保存；之后的新报告、清源或 A→B→A 接收不能更新或续用这份封条。

电池源存在、已绑定或已有新鲜报告**不是新增的默认先决条件**。捕获时仅对本机原 `_fleetBatteryTelemetryMemoryCurrent()` 为真、且已观察/完整/不在更新/有原 receipt 的 fresh 行固定其原 3000 ms 生命周期；主电池与副电池一视同仁，因为共享健康检查也读取副电池的明确负面状态。`current()` 在原电池时钟域检查这些原行没有自然过期。捕获时原本未绑定、未观察或已陈旧的行仍只参与完整状态比较，不被补成必需的新鲜来源；已有业务健康门禁继续决定其是否可用。

快照只保存时钟有效性、原 clock 起点和原接收时刻，不保存当前 `elapsed()`/动态 age，不借新包重采时间。电池 RAW 接收栈未退出不能捕获/通过封条。实现仅读私有内存与原 callback-free memory predicate，不调用公开电池 getter、Link 虚方法、`weak.lock()`、JSON 或 hash；没有命令、权限或 schema 变更。它补齐本次封条的账本覆盖，不宣称复制了 Registry 内部某次健康读取的原子快照，也不证明未观察到的物理故障不存在。

## 接口与原材料

```cpp
static Ptr capture(Vehicle*, LinkInterface*, ParameterManager*,
    const WeakLinkInterfacePtr& originalPrimaryOwner,
    const QVariantMap& originalExecutionParameters,
    const QVariantMap& originalStartupPolicy);
bool current() const;
```

策略必须是本次 `target.beforeArm.startupTransitionPolicy` 的完整原值。入口复用 `FleetMissionExecutionProfile::normalizeStartupPolicy` 的 v1–v4 规则并比较完整规范结果；不会补字段、选择策略、制造 profile 或解析另一份任务。490还必填实际PM、Target首次固定的PRIMARY弱控制块与原 `target.beforeArm.parameterSummary.originalExecutionParameters`，不提供旧三参旁路。原始动作、审批和阶段匹配仍由现有 Transition/Transaction 执行。

封条私有保存：

- 实际 Vehicle、PRIMARY、ParameterManager、MissionManager、GeoFenceManager、RallyPointManager、FTPManager、VehicleLinkManager、MAVLinkProtocol、MultiVehicleManager、初连状态机及已存在 sender owner/link 的 QPointer；PRIMARY 原弱控制块、系统/组件及固件/构型枚举。
- 490增加独立自持的原29参数快照（schemaVersion1／6键／每行14标量），另按原PM语义核原manager epoch、Fact、值／类型／contentRevision、实际来源和当前行龄；同值新回执可刷新参数行龄，但不延长事务／phase截止。
- VERSION、六路飞行状态、LOCAL/HOME、ODOMETRY、SYS_STATUS/ESTIMATOR_STATUS、BATTERY_STATUS、GLOBAL_ORIGIN 的原始 epoch、value/hash（账本原有字段）、接收 token/receipt、语义/参考修订、时钟不连续历史、完整性/更新状态和原接收时间。
- 当前 sender 的 owner/startup/step/operation、实际请求入队状态、原始计数、完整 retained step33/风险 journal、原队列快照、各通道 gap 起点和已结束时刻。原 armed/flying/landing/mode 缓存也一并保存。
- 只有 v4 增加 ALTITUDE 原始 ledger 与同一 scope monitor 的原参考、样本、违规/gap 历史。v1–v3 不读 ALT 来增加门槛；ODOMETRY 从未观察的 v1 路径仍兼容，v2–v4 保留原明确需求。

这些是本机已有有界私有结构的值副本，没有 JSON 往返、重新哈希、solver、文件 I/O、发送、通知或 Link 虚方法。`current()` 不调用可能回调的 flight/coordinate/ODOMETRY 公共快照；不执行 `weak.lock()`，也不要求 UDPLink 与 Vehicle 属于相同线程。对象消失、实际成员/PRIMARY 控制块变化、源 epoch 变化及原完整状态快照覆盖的RAW接收或修订变化，均不能靠最后一个相同hash恢复旧封条；490参数行单独沿原PM语义匹配，同值新receipt并非必失效。

## 自然过期及正常准备路径

动态 `elapsed()`、age、getter 派生最大 gap 不放入相等快照，避免单纯时间前进导致永远不相等。`current()` 另在各 ledger 自己的原始单调时钟域检查 VERSION 30 秒、原策略 periodic gap、LOCAL/HOME、EST、适用的 ODOM/ALT 鲜度，并逐条检查实际已入队 retained history 的持续时长与当前尾 gap；不把不同 clock 的 elapsed 直接相减。活动未关闭槽也保留原 expiresAt 边界。

BeforeSeq0 不要求已建立 sender owner、EST sender scope 或 ALT monitor。新 prepared/unissued 槽、发送者自身 `entryBusy=true`、已关闭但仍归本 scope 记录，都可按其真实原值捕获；不使用 `fleetStartupTransportQuiescent()` 一概拒绝发送栈。实际 RX 栈/正在更新的数据仍拒绝。旧已取消 scope 的历史保留作身份比较，不把其已过期 gap 或已退休 ALT monitor 当新审阅的先决条件。

新 sender slot 会实际改变封条。调用方必须在 slot 准备后通过完整 authorizationFence 重做全组阶段检查，才可使用该新组；不能仅重采一个 token 来绕过原 phase 检查。

## 全组使用约束

Transaction 在每架机所有回调型验证之前捕获封条，原 phase/read 检查后核该封条，最后再次核全组。只有 `_gate` 原来源、身份、权限及阶段检查全部成功，才发布本次完整同代/同阶段集合；失败不能留下可消费的成功前缀。

最终 `commandContextCurrent` 只核最近已完整检查的原集合，不捕获或重新批准。Vehicle 最后实际 write 前仍调用该原生检查。封条没有证书或授权含义，不能取代原 Review/Authority/Transaction、净空 witness、MAVLink 来源边界或执行后续风险日志。

## 验证边界

本批仅源码静态阅读和文本差异检查，未编译、运行 helper、测试、仿真或连接设备。工程师仍须验证正常 BeforeSeq0/新槽/闭合 barrier、后来一架的回调改变先前一架、A→B→A 原始接收、源换代、各 clock 的自然过期、v1 无 ODOM/ALT、v4 monitor、取消/销毁及实际 write 前拒绝路径。460 还需验证仅电池故障位/charge 已知性变化、主/副电池自然过期、原无源/陈旧兼容、嵌套电池接收及旧 seal 不因新包续期。封条只表示本机观察在检查窗口内未改变，不能证明没有未观察到的物理变化。

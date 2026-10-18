# Vehicle：ALTITUDE 原包账本与显式 v4 启动范围监视（444–445）

## 446：原scope的数值门槛统一使用精确比较

实际scope首次绑定的`capture/sampleWithinBaseline`，以及逐原包／当前观察／最终write前的`decodedMatches`，经同一[高度关系helper](../../custom/src/Swarm/FleetMissionStartupAltitude.md)改用[定长精确线性组合](../../custom/src/Swarm/FleetMissionAltitudeArithmetic.md)。直接使用原float32字段及整数毫米参考，避免小残差被Double中间运算舍入为零后误过零容差；不新增原生状态、回调或门禁旁路。

v4政策／reference20／sample20／monitor18及原packet/step33不变，原始位值、同receipt正负零不可改写、首次基准、违规锁存和计时域保持。只修改共享纯算术，v1–v3仍不依赖ALTITUDE；不改变认证、起飞许可或已write事实。未编译、测试、运行或连接设备；下方445及444为既有生命周期。

## 445：原已阅基准与固定大小连续监视

仅显式 `startupTransitionPolicy.schemaVersion=4` 使用本节功能；v1/v2/v3 仍不依赖 ALTITUDE 的接收、fresh、known 或状态版本。v4 沿用 v2 的运动/航向政策与 HOME `stop-and-review`，另由 [FleetMissionStartupAltitude](../../custom/src/Swarm/FleetMissionStartupAltitude.md) 归一化 `altitudeReferenceEnvelope`：固定 `referenceBasis` 与六个显式 0–1000 米容差，分别约束三个同报文差值相对**首次已阅 sample**的漂移，以及对原 origin/HOME/HOME localZ 的残差。数值满足容差不证明共同原点、PX4 内部 valid、同一估计器采样或最终启动许可。

实际 `FleetMissionStartupTransaction::reviewedAltitudeContext(actualVehicle,actualPrimary,startupId)` 只读取本实例目标保存的 `beforeArm` 六字段与完整政策，返回 `required/policy/reference/sample`。Vehicle 在该实际 owner/startup 的首次发送 scope 绑定一次 reference20/sample20；后续 Seq0/Arm/Mode 不重采基准。调用方不能用导入 map、QML 或当前最新值替换原基准。无效首次绑定仍保留失败的 v4 scope，不能重试成旧政策。新独立人工审阅可以保留原已观察的时钟倒退标志，但本 scope 内 flag 与 clock revision 必须不变。

固定大小 monitor 保存原 context、实际 PRIMARY 弱控制块、version/flight/ALT/coordinate epoch、原 HOME 风险版本、累计接收间隔与首个违规原因，不保存逐包无界列表。实际 PRIMARY ALTITUDE 包在 decode/state/clock 分类后、任何 Link 回调之前，直接使用原 decode18 检查六容差；不伪造一个 `quiescent=true` 的 getter 封套。坏包/弃帧、失源、新时钟倒退、必需三高度未知、数值越界或超间隔一旦观察即锁存，A→越界B→A 不能恢复本轮。无关 terrain/clearance/monotonic 字段的正常状态变化不单凭全局 `stateRevision` 拒绝 v4。

监视只覆盖首次本机 scope 绑定之后的实际收包，并核绑定时最新样本相对原已阅基准的容差；**不证明人工审阅至 scope 绑定之间没有未保留的中间越界，也不证明未收到的远端样本**。`maximumRawGapMs` 同时用于 ALT 的接收年龄/连续 gap，全部计算使用 `_startupAltitudeClock`，不与 step journal 的另一个 elapsed 时钟或远端 `timeUsec` 相减。

### 非 invokable 独立观察与排空

`fleetStartupAltitudeMonitor(actualOwner,actualPrimary,startupId,originalReference)` 只认原实际 owner/epoch/PRIMARY/scope 与完整原 reference/context，未绑定或不匹配返回空 map；成功返回固定 18 键：

| 类别 | 字段 |
| --- | --- |
| 类型及状态 | `schemaVersion=1`, `required=true`, `bound=true`, `sourceCurrent`, `referenceMatches`, `sampleWithinEnvelope`, `continuousGapValid`, `violationObserved`, `ok`, `reason` |
| 绑定与累计 | `startupId`, `transactionEpoch`, `referenceHash`, `maximumObservedGapMs`, `observedFrameCount`, `baselineReceiptSequence` |
| 固定否认 | `executionPermit=false`, `flightCommandReleased=false` |

`maximumObservedGapMs` 为本机 Double 整数，无法确定时 -1；两个计数是规范十进制字符串。`baselineReceiptSequence` 是首次 scope 绑定时实际最新 ALT receipt，不冒充人工审阅的 receipt；`observedFrameCount` 统计绑定后匹配 PRIMARY 的原包，包括随后拒绝的坏包。`ok` 仅为本地监视条件，不是许可。绑定后失败仍返回完整负结果；取消/退休仍保留原基准、计数和违规，`bound=true` 但 `sourceCurrent/ok=false`。

`startupAltitudeHomeReferenceMatches(actualPrimary,home9,maxAgeMs)` 是新增纯内存 v4 尾部检查：原 native coordinate ledger、PRIMARY 原弱控制块/实际 owner、version/flight、完整 HOME9、相关接收栈静止及 HOME 自身时钟年龄必须一致。它不调用 Link 虚方法、不请求 HOME、不从 Fact 补值，避免其它可回调 getter 只改变 HOME localZ/time 而未改变 geo3 后仍沿旧参考发送。

原 send 的完整身份/认证围栏保持；v4 再在排队前和最后实际 write 前进行无外部回调的 monitor 检查。原 flight/HOME 接收日志入口仍保留真实事实，不用要求静止栈的 monitor 阻断其自身接收。原 step schema3/33 键、origin24/native27 与无 owner 参数的通用 quiescence 合同不变。

`retireFleetStartupAltitudeMonitor(actualOwner,startupId)` 只明确终止匹配 scope，不清违规/基准/已 write 事实，不发送命令；覆盖首次绑定后尚无 step 即拒绝、事务 `_stop`、正常 scope cancel、owner 销毁及 `_detachDrain`。正常 Seq0 完成至 ARM/Mode 间不退休。专属 `fleetStartupAltitudeTransportQuiescent(startupId,transactionEpoch)` 只用于该已绑定 scope 的 ALT 接收栈排空，允许原 owner 已销毁；不看 fresh/违规/retired/ok，不会因失败状态永远无法排空。无此 scope 表示没有它拥有的 ALT 栈，不表示来源当前或可发送；原 v1–v3 不因此增加 ALT 依赖。

本批只完成源码与静态阅读，未编译、测试、运行应用/仿真、执行 API 或连接设备，未修改 PX4；工程师仍需承担完整构建与实际验证。下方 444 内容描述原独立诊断账本，不能覆盖本节显式 v4 限定。

本批为地面站源码实现与静态审阅，未编译、测试、运行应用/仿真或连接设备，未修改 PX4。ALTITUDE 只提供诊断，不接入现有 v1/v2/v3 启动政策、批准、发送门禁或最终许可。

## 原始来源与接收生命周期

`_mavlinkMessageReceived` 冻结 `originalMessage` 后，在任何 prepare、Link 虚方法、电池/健康/遥测/执行观察器或插件回调之前，分别建立 GPS_GLOBAL_ORIGIN 与 ALTITUDE 的独立接收深度。ALTITUDE prepare/commit 在入口早期完成，但其深度一直持有至整次 receive 返回。嵌套回调中不能将正在接收的 ALTITUDE 声称为 current/available。

只接收实际 MAVLinkProtocol、本 Vehicle SYSID/默认 COMPID、当前 PRIMARY 原弱控制块成员的消息 141。prepare/commit 核实际 Vehicle、Protocol、VehicleLinkManager、MultiVehicleManager、版本/flight 来源、PX4 多旋翼初连完成及非离线状态；每个 `isConnected/isMockLink/isLogReplay` 后检查原 QPointer、epoch、revision、receipt 和 PRIMARY 身份。其它系统/组件/次链路不污染本账本。

生命周期不依赖 Fact 或插件调整后的值。版本/flight 失源、链路移除/替换、原 Protocol/管理器/MVM 销毁、Vehicle 移除均退休该来源。失效函数只改本机内存；旧 Link hooks 以原 epoch 退休，下一次 bind 使用 `takeFirst → disconnect → QPointer/epoch/本次prepare尾核`，中断时保留未释放句柄。另有私有 `hooksReady` 防止连接期间同源坏帧中断后，把未完整安装的生命周期监听误认成完成绑定。

不存在新消息请求、自动采样、文件持久化、observer 替换或飞行命令。

## 同包值与未知语义

实际值由独立 `FleetStartupAltitude::decode` 解出并原样保存 native18；MAVLink1 为完整 32 字节，MAVLink2 合法 1–32 字节以明确零填充处理尾零裁剪。六字段严格来自同一原始报文：

- `altitudeMonotonicMeters`、`altitudeAmslMeters`、`altitudeLocalMeters`、`altitudeRelativeMeters`；
- `altitudeTerrainMeters`、`bottomClearanceMeters`。

每项保留 `value/known/state/rawBits`。有限 float32 精确提升为 Double；NaN/正负无穷保留原始位模式和状态，数值使用显式 JSON null。terrain 小于 -1000 或 clearance 小于 0 保留有限原值，但 `known=false`，不能被改成零或忽略。其它项可独立保持已知，不因一项未知而拼入旧包数据。

decoder 的三个 reported 高度差仅是本报文已知字段的数值差；未知仍是 null。`sameMessageFields=true` 不证明传感器新鲜、同一估计器采样、共同原点、PX4 内部 valid 分支或真实净空。`timeUsec` 是完整 uint64 规范十进制字符串，不经 JavaScript Number，不推断 UNIX/boot 时域或远端重启。

## 非 invokable 只读接口

`startupAltitudeObservation(qint64 maximumAgeMs = 3000) const`：同线程且 maxAge 在 1–60000 毫秒内返回固定 native27；错误线程/参数返回空 map，不写状态。完整键如下：

| 字段组 | 固定字段 |
| --- | --- |
| 身份 | `schemaVersion=1`, `vehicleId`, `componentId`, `sourceEpoch`, `flightSourceEpoch`, `altitudeSourceEpoch` |
| 观察 | `observed`, `everObservedRawFrame`, `available`, `fresh`, `receiveStackQuiescent` |
| 顺序/状态 | `receiptSequence`, `stateRevision`, `clockDiscontinuityRevision`, `clockDiscontinuityObserved` |
| 当前诊断 | `receivedAtMonotonicMs`, `ageMs`, `source`, `value`, `rawPayloadHash`, `rawPayloadLength` |
| 固定否认 | `authenticated=false`, `signatureVerified=false`, `px4BranchValidityProven=false`, `sameEstimatorSampleProven=false`, `executionPermit=false`, `flightCommandReleased=false` |

schema、ID、载荷长度为原生 Int；单调毫秒为 Double 整数；所有 receipt/revision 是不回绕的十进制字符串。缺失值 map/摘要为空，长度 0、receipt `"0"`、时间 -1。三个 epoch 是本机 UUID。初始 source 为 `"unknown"`，从未见过时三个 revision/receipt 为 0。成功完整观察 source 为 `"live-altitude"`。

`available` 只表示当前来源、完整合法封套和本地接收时间可用，不要求六个字段都 known，也不要求 clock flag 为 false。`fresh = available && ageMs <= maximumAgeMs` 仅为本机接收年龄；不是远端传感器采样年龄。过期数据仍保留真实值/年龄，但 fresh=false。

`startupAltitudeObservationSourceMatches(LinkInterface*) const` 是非 invokable、纯内存的本机来源/PRIMARY/静止栈检查，不调用 Link 虚方法，不证明远端此刻可达，不替任何高度值授予可用性或权限。getter 同样不触发外部回调。

## 状态版本与时间倒退

每个匹配的实际 PRIMARY 帧推进 receipt 计数，包括随后被拒绝的坏帧；只有完整观察才公开非零 `receiptSequence`。状态版本依据 decoder 的 state8（六字段 state 与 known 分类及固定非认证标志）：

- 正常有限高度变化、time_usec 递增/相同、接收时间及 age 增长不推进 `stateRevision`。
- 分类/known-mask 变化、坏封套、同源弃帧/嵌套接收、时钟数值倒退或来源失效在回调前退休旧状态；A→B→A 不能复活原版本。
- 远端 time_usec 只与上一个实际已解析的同源值比较。150→50 记一次倒退，随后 50→60 不重复记倒退；不与历史最高值比较。
- 同源坏/弃帧不清上次已解析时间和倒退历史。`clockDiscontinuityObserved` 是当前来源时间序列的 sticky 标志；真实来源失效、epoch 重建才清除该标志并重新建立时间序列。
- `clockDiscontinuityRevision` 是 Vehicle 生命周期内累计计数，换源也不清零。因此 false 标志与正累计 revision 可以同时出现。所有计数耗尽时明确不可用，不回绕。

`everObservedRawFrame` 见过匹配的原始帧后保持 true；坏帧或失源不能伪装为从未见过。新帧可恢复诊断观察，但不恢复任何旧批准；本批本身不提供批准接口。

## 保持原行为与后续边界

原六个 flight 通道、两个 coordinate 通道、ODOMETRY、origin24、step33 和 SYS/EST/电池合同不改；没有把 ALTITUDE 深度、时钟、fresh 或 stateRevision 加入现有 v1–v3 命令门槛。既有 `_handleAltitude` 显示 Fact 路径也不改。

配套审阅只冻结 `startupAltitudeDiagnostic` 供显示；动态 raw 值、receipt、age 必须排除在原阶段/风险身份比较之外。后续若引入显式高度参考政策，应独立限定所需字段、时间预算、数值域、容差和跨流参考依据；不能直接对整张动态原包作相等比较，更不能把同包差值或有限 float 当作 PX4 内部有效性证明。工程师仍需负责构建、仿真、真实固件发送与硬件验证。

# FleetCameraStreamObservation（507）

本模块是相机 `VIDEO_STREAM_INFORMATION`（269）／`VIDEO_STREAM_STATUS`（270）的**被动报告观察**，不是发送／停流事务。实际链为 Vehicle 原生 RAW 观察 → Registry 真实绑定只读 wrapper → FleetLinkDegradationPolicy → Guided 原五秒快照 → 指挥中心既有纯文本摘要。没有新按钮、授权、查询、配置或飞行动作。

原相机管理器的调整后消息缓存仍用于其既有功能，不被拿来填本模块的原生来源。URI／name 不进入公开 DTO；本地 receiver 正在解码也不能代替远端报告或证明远端停流。

## 1. 原生来源与资源边界

- [Vehicle](../../../src/Vehicle/Vehicle.h) 的 `fleetCameraStreamObservation(qint64 maximumAgeMs = 3000) const` 只可由 C++ 调用，不是可任意 QML 构造的服务。合法年龄参数为 1–30000 ms；Registry 固定读取 3000 ms，不接 caller 观察 Map 或自选时限。
- 原生准入只认本实际 Protocol、本车 sysid、实际 PRIMARY 成员及原弱所有权、相机 component 100–105 和 stream_id 1–255。相机 component 不是 autopilot 默认 component；多相机相同 stream_id 以 component＋stream 双键区分。
- 269／270 为 MAVLink2 消息，按实际生成定义的最大长度检查原包并对合法尾零截断进行零填充后解码；不把 MAVLink1 或任意直接 helper 调用当真实观察。原 P0 接收顺序／处理不因本模块观察失败而被放宽。
- 观察保存原 transport／version／来源代次及实际物理对象。接收阶段的三项 Link 实时检查后核原上下文；新 getter 为纯内存读取，不调用 Link 虚方法、weak.lock、账号／OS、发送器或信号。
- 全车最多保留 32 行，不是每相机各 32 行。新增不同双键的行将超过 32 行时才置 sticky `saturated`；恰好 32 行且未丢弃新行不因此饱和。饱和后保有界历史前缀，但全部 fresh／matches 为 false；后来的友好数据或 count 不能把该前缀自动变成完整目录。
- 真实源变更清空全部行及绑定、更新 sourceEpoch，并清 saturated；全局 receipt／语义 revision 不归零或回绕。耗尽停止新观察并清行，不能把新来源或相同数值接回旧依据。RAW／probe 临界中不可当前；时钟异常可保历史值而年龄为 −1。

这些只是本地来源过滤与续作限制，不证明硬件身份、消息签名、远端实例、实际视频传输或命令因果。`catalogComplete` 始终 false。

## 2. 原生值合同：22／4／15／15

原生顶层恰好 22 键：

```text
schemaVersion, reportType, vehicleId, sourceEpoch, transportEpoch,
versionEpoch, receiptSequence, sourceCurrent, observed, saturated,
maximumRows, maximumAgeMs, rows, reason, snapshotOnly, authenticated,
catalogComplete, remoteRunningVerified, remoteStoppedVerified,
configurationPermitted, executionPermit, flightCommandReleased
```

`schemaVersion` 为原生 Int 1，reportType 为非 null String `fleet-camera-stream-observation`；maximumRows 为 Int 32，maximumAgeMs 为 LongLong。顶层 observed 等于本线程下 rows 非空；sourceCurrent 只表示原提供者／绑定仍一致且读取时不在临界，不要求有任一新鲜／有效行。全陈旧、数值无效或 saturated 的历史都不能因此变成新鲜。

snapshotOnly 为 true；authenticated、catalogComplete、remoteRunningVerified、remoteStoppedVerified、configurationPermitted、executionPermit、flightCommandReleased 七项恒为 false。receiptSequence 为不经 JS number 的规范 uint64 十进制 String。源清后 observed=false 时仍可保历史非零全局 receipt；错线程返回空行／空来源代次／receipt `"0"` 等不可当前诊断。

rows 按 componentId、streamId 严格递增且唯一，每行恰好四键：

```text
componentId, streamId, information, status
```

每行至少一侧 observed。information 恰好 15 键：

```text
observed, fresh, ageMs, receiptSequence, contentRevision, declaredCount,
type, flags, framerate, resolutionH, resolutionV, bitrate, rotation,
hfov, valid
```

status 恰好 15 键：

```text
observed, fresh, ageMs, receiptSequence, contentRevision,
informationRevision, matchesCurrentInformation, flags, framerate,
resolutionH, resolutionV, bitrate, rotation, hfov, valid
```

原生类型与未知值：

- observed／fresh／valid／matchesCurrentInformation 为实际非 null Bool；ageMs 为 LongLong，−1 表示未知，否则非负。
- receiptSequence／contentRevision／informationRevision 为规范 uint64 十进制 String；未观测一侧的 receipt／contentRevision 为 `"0"`，已观测一侧的这两项为正。informationRevision 则按 Info 有无关联：Status 先到时即使 observed=true 仍可为 `"0"`，不能混作该侧 contentRevision。每行 receipt 不超过顶层 receipt。
- declaredCount 为 Int 1–255 且不小于该 stream_id；type 为 Int 0–255；flags／resolutionH／resolutionV／rotation／hfov 为 Int 0–65535。未观测为 −1。
- bitrate 为 LongLong，原 uint32 数值域 0–4294967295；未观测为 −1。它是相机报告字段，不是本地测量的发送率或可用带宽。
- framerate 为 Double，承接原有限非负 float 报告；未观测或原数值非有限／负值时公开 −1.0，并使 valid=false。不向外传 NaN／Infinity，也不凭其余 raw uint 字段替无效 framerate 补有效结论。
- 未观测一侧全部数值为 −1、Bool 为 false、counter 为 `"0"`；不是用零代替未知。一个合法无效／陈旧／饱和值不使 Registry 结构读取失败。

URI、name 仍可参与原生私有语义比较，避免相同公开数值却换了实际流元数据时沿用旧状态；它们不进入上述公开结构。

## 3. Info／Status 关联不能互借新鲜度

原生完整等式为：

```text
sample.fresh = top.sourceCurrent && !top.saturated && sample.observed
               && sample.valid && sample.ageMs >= 0
               && sample.ageMs <= maximumAgeMs

status.matchesCurrentInformation = top.sourceCurrent && !top.saturated
    && information.observed && status.observed
    && information.valid && status.valid
    && status.informationRevision != "0"
    && status.informationRevision == information.contentRevision
```

matches 不代替两侧独立 fresh，Status 的 fresh 也不依赖 Info 年龄。故“新 Status＋旧 Info”不能被策略当作同代新鲜状态。

- Status 先到允许保存，但 informationRevision 为 `"0"`，不匹配尚未观测的 Info。
- 首次 Info 或任意 Info 私有语义变化都会清该行旧 Status 的整个 15 键状态；重复等值 Info 只推进收到该侧报告的观察，不凭空改变其语义 revision。
- count 改变先删除同 component 的旧目录，再存本次行；同 component 已观测 Info 的 declaredCount 一致。不把两次目录前缀拼成完整枚举。
- 当 Info 与 Status 均有观察时，Status 的 informationRevision 必须等于当前 Info 的非零 contentRevision；新鲜度／来源条件再独立决定可否使用。源清、嵌套 RAW、counter 耗尽或饱和均不借旧 RUNNING 位恢复资格。

## 4. Registry 实际绑定 wrapper7

[FleetRegistry](FleetRegistry.h) 新增 `nativeCameraStreamReview(int systemId) const`。它只接受本车 ID，不接受观察、component、URI、原始 payload 或导入档作为来源。

实际 `_nativeBatterySources` 中的 Vehicle、vehicleRecordEpoch 和 bindingEpoch 必须仍精确属于同 Registry 记录；只对该实际 Vehicle 调用 getter 一次。完整原生值经闭键、精确原生 Int／LongLong／Double／Bool／String 类型、未知值、排序唯一行、counter 及关联等式校验后独立持有，返回前再核同原绑定，不用后来的同 ID Vehicle 接续旧读取。

wrapper 恰好七键：

```text
ok, error, vehicleId, vehicleRecordEpoch, bindingEpoch,
observation, snapshotOnly
```

成功时 ok=true、error 为空 String，保完整观察；sourceCurrent=false、单侧缺失、陈旧、valid=false 或饱和仍可为**成功读取的诊断**，不是控制许可。失败 ok=false、error 给出原因、observation 为空 Map，两个绑定 String 为空，snapshotOnly=true。没有副作用、通知或自动重试。

## 5. 策略与原界面真实消费

[FleetLinkDegradationPolicy](FleetLinkDegradationPolicy.h) 的 `evaluateNativeReception` 增加第五个可选 cameraReview；旧 evaluate 和三／四参数调用仍可用，缺相机观察合法未知。便携 Map 的值校验不认证来源，实际来源由上述 Registry 调用链建立。

新 localCamera 固定 18 键：

```text
sourceKind, state, reason, sourceBound, observed, saturated,
freshInformationCount, matchedFreshStatusCount, reportedRunningCount,
reportedNotRunningCount, unmatchedOrStaleRowCount, observation,
vehicleRecordEpoch, bindingEpoch, snapshotOnly, remoteRunningVerified,
remoteStoppedVerified, configurationPermitted
```

状态为 reported／incomplete／unavailable／invalid。仅同车、同 record／binding 且原相机来源当前、不饱和时计数；状态匹配须同时 Info.fresh、Status.fresh 和 matchesCurrentInformation。RUNNING 位只贡献 `reportedRunningCount`，未设置位只贡献“报告未运行”计数，不证明实际运行或停止。相关认证／配置旗标始终 false。

新原生策略结果为 24 字段，在原结构中增加 localCamera；相机报告不填 quality／bandwidth／loss，不让新路径 normal，不生成发送权限。policyChanged 只在完整结果构造后发布一次，返回局部值。

第五参数非空时，measurementSource 为 `native-local-reception-with-camera-review`，同时第四参数也非空则为 `native-local-reception-with-latency-and-camera-reviews`。缺第五参数保留旧 `native-local-reception-only`／`native-local-reception-with-latency-review` 值；这些来源标签也不认证 portable Map 或远端报告。

[GuidedActionsController](../../res/Merivus/GuidedActionsController.qml) 在原五秒周期从冻结的实际 Registry 读取 wrapper，调用前后及结果通知后核原 Root 存活／提供者，再交给同一策略快照。它不请求 Info／Status，不发送相机命令；已有可选遥测数值提案仍只是提案。

[CommandCenterOverlay](../../res/Merivus/CommandCenterOverlay.qml) 只扩展原 PlainText／WordWrap 摘要，显示该次快照匹配／不足／不可用架数，并明确不是实际送流、停流或带宽变化认证。没有新增控件、自动选择、焦点跳转或权限入口。UI/UX 仅采用静态参考，未运行检索、预览、渲染或辅助技术验证。

## 6. 相机模式入口与验证边界

本批还在 [CameraControl](../../../src/Camera/QGCCameraControl.cc) 的原 `setVideoMode`／`setPhotoMode` 入口接入共同本地模式意图：在虚 getter／Link 所有权释放前钉住原 CAM_MODE Fact，外调后核原对象、父对象、native map、参数完成状态、来源与不回绕 mode 代次，Fact 仍是目标值才继续本地 mode 更新。toggle、观察变化与 reset 撤销旧模式续作；同值显式意图也不承接旧操作。foreign／新建／重设父对象的 Fact 保守拒绝。

这只是修补原有 caller 的本地生命周期，不新增模式权限、命令 token 或远端执行认证；已经可能交接的命令不被宣称可撤回。全部旧相机／Fact API、SubtitleWriter、渲染线程与插件行为仍不能据此视为已完整审定。此前[506 合同](../../../docs/handoff/MERIVUS_VIDEO_LIFECYCLE.md)的本地启停／生命周期边界仍适用，已知模式 caller 缺口以本批限定修复为准，不外推整个相机类。

精确本批范围与全部**未执行**场景见[清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原 25 目标和六类实质剩余不缩减；约 40%／60% 仅源码工作量粗估。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；**未编译，未运行解析器、helper、业务、模型、QML、应用、认证、SQL 或 hash 实现，未测试／仿真／操作硬件，未读取或修改 PX4，未刷机／放行／提交／更新 PR 或打包**。本合同不是可部署、可飞或真实性证明。

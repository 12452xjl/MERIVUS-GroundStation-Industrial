# 受保护 Run 语义转换事件

## 541：提交后的任务模式退出只作观察

正常新 runObservation 为 schema2/exact29：保留以下旧28键并新增 `missionModeExitObservation`。旧 schema1/exact28 完整可读，capture-gap 仍只用 schema1。新增值不是 formal failed/abort，不覆盖原四种终点候选，也不改变 TaskRecord 的正式结果。观察对象只保留真实本地提交之后、同一尚未退休的本地来源中先任务模式后非任务模式的 HEARTBEAT 收件对；原 customMode/baseMode/systemStatus 不从调整后状态或调用者公开 map 重建。来源退休不拼跨 epoch 边沿，两次收件间仍可能存在缺口。

sidecar 精确9键：`schemaVersion=1/observed/count/candidate/first/last/requestCausalityVerified/remoteOutcomeVerified/automaticActionsIssued`。后三项恒 false。count 是0..UINT64_MAX规范十进制字符串；observed 等于 count>0，并与正常摘要 anomalyMask bit2 一致。零次 candidate=`not-observed`，first/last 为 JSON null；原生以无类型无效 QVariant 表示，不接受 typed-null 字符串或容器。非零 candidate=`mission-mode-exit-observed-not-outcome`，且原摘要 modeQueueCommitted 必须为 true。

每个 first/last 精确6键：`customMode/baseMode/systemStatus/priorMissionReceipt/exitReceipt/continuityUnknownAtExit`。模式和状态分别为真实数值 uint32/uint8/本仓 MAV_STATE 0..8（包括 UNINIT），拒绝 bool/string/非整值；连续性为严格 bool。每个 receipt 精确3键：`sourceEpoch/sourceSequence/receivedMonotonicMs`，分别为非零小写 UUID、正 uint64 十进制字符串、0..INT64_MAX十进制字符串。一次退出的两 receipt 同 epoch、序号严格增加、单调时间不回退，退出 raw 模式不得仍是带 CUSTOM_MODE_ENABLED 且 reserved=0 的 PX4 AUTO_MISSION。

共享 [FleetMissionModeExitObservation](FleetMissionModeExitObservation.md) 完整核 shape/类型/首末关系：一次时首末完全相同；多次时 first.exit.seq < last.prior.seq < last.exit.seq，首末时间不回退，并核 count-1 <= (last.exit.seq-first.exit.seq)/2。仅保存首末，不伪造中间心跳；同一模型提交前可合并多个退出边沿，提交检查点还受原轮询/源退休等流程影响，不声称每次退出都有完整独立历史。snapshotOnly 保 true、historyComplete 保 false。

持久 payload 外层仍 exact12：旧 marker `fixed-diagnostic-summary-v3` 只接 summary1，新 marker `fixed-diagnostic-summary-v4` 只接 summary2，混搭拒绝。v4 明确只为退出对保留上述本地随机 epoch/序号/单调毫秒，不是远端来源认证，不包含账号、经纬或飞行许可；旧 v3 的不留 RAW source epoch 规则仍适用于旧版。原事件 exact6、wrapper18/SQL schema、不完整历史与全部权限 false 不变。上游4096字节/512项FIFO不扩，超限走既有 capture-gap，不截取半个 sidecar。

ReportBuilder 与两个历史页显示计数、raw mode/status、首末 receipt、连续性及非结果说明；旧摘要显示“未采集”，不补为“未发生”。新摘要零次也不证明持续执行。这里只做源码/合同与静态交叉审阅，没有执行算法、解析器、hash、SQL、QML、编译、测试、仿真、刷机或硬件操作。以下510描述为旧版合同。

Qt 的 JSON-null/invalid QVariant 表示只在已经合法的 v4 未观察 first/last 解码处规范；两个历史页也仅对上述 exact9 的零次、双 own-key sidecar 将 null/undefined 两槽复制为显式 JS null，其余 undefined 拒绝。编码入口不接外来 typed-null 字符串或容器，不改旧投影空值政策。

## 510 当前边界

本链把实际 `SwarmMissionOrchestrator` 保留的原生 Run 关键语义转换，送入原 `FleetEventBlackBox` 新收据、持久事件归档、任务哈希历史下钻与只读时间线。它不是逐包遥测，也不因末任务项、到达报告或落地顺序而把任务改为 Completed/Failed。最终 StartPermit、连续动态轨迹与远端任务实例认证仍没有因此成立。

原生生产者只在已提交记录的关键语义变化后形成独立有界摘要；不把外来 JSON 或历史归档恢复为活 Run。Root 必须核实际生产者、当前临时收据和原对象生命周期，再显式送入 BlackBox。未开启录制、队列拒绝、投影拒绝和存储失败不能说成已经持久化。

## 原生事件合同

原事件 type 为 `task.observation.transition`，source 为 `SwarmMissionOrchestrator`。payload 恰六键：

- `taskId/startupId/deploymentId`：规范、非零、无大括号小写 UUID。
- `targetVehicleIds`：至多 32 个不重复的 1..255 整数。
- `vehicleId`：1..255，必须出现在原 target 列表中。
- `runObservation`：以下恰 28 键的小摘要。

这些飞行 system ID 不是 Windows SID、账号、设备序列号或硬件身份认证。Codec 的静态 `runObservationPayloadValid(payload)` 只做纯值形状和一致性检查，没有 QObject、文件访问、真实来源认证或授权能力。

| 字段 | 固定类型与含义 |
| --- | --- |
| `schemaVersion` | 整数 1 |
| `transitionKind` | `registered/changed/closed/owner-destroyed/capture-gap` |
| `transitionSourceHash` | 原生转换流标识的小写 SHA-256；不是远端身份认证 |
| `transitionSequence` | 1..UINT64_MAX 规范十进制字符串，不通过浮点数传计数 |
| `capturedAtUtcMs` | 1..9007199254740991 整值 UTC 毫秒；不证明跨机时钟同步 |
| `observationRevision` | 正规范 uint64 字符串；capture-gap 为 `0` |
| `revisionExhausted` | 严格 bool；为真时 revision 必须是 UINT64_MAX |
| `runIdHash` | 实际本地 Run ID 的 SHA-256；capture-gap 为明确空串 |
| `outcomeCandidate` | 原四种固定候选；capture-gap 为 `not-applicable` |
| `modeQueueBegan/modeQueueCommitted` | 严格 bool；committed 蕴含 began，只是原生本地队列事实 |
| `continuityUnknown` | 严格 bool；不把缺口补成连续执行 |
| `anomalyMask` | 整数 0..255，同 task/vehicle 曾观察到的分类位做历史 OR；后续普通报告不清旧位，不回写或改变原 TaskRecord |
| `reasonCode/sourceRetirementCode` | 非 null 的至多 128 字符小写 kebab 码或明确空串；生产者只取原生固定码，不复制自由理由 |
| `observerClosed/ownerDestroyed/reviewRequired` | 严格 bool；ownerDestroyed 蕴含 observerClosed |
| `droppedTransitionCount` | 0..UINT64_MAX 规范十进制字符串，计入已知本地转换缺口 |
| `snapshotOnly` | 恒 true |
| `historyComplete/remoteMissionInstanceVerified/missionCompleted/resultVerified/executionVerified/executionPermit/flightCommandReleased/permissionsRestored` | 八项恒 false |

四种候选仅为 `no-terminal-candidate-observed`、`terminal-item-observed-not-completion`、`indefinite-terminal-hold-observed`、`landed-after-terminal-observed-not-completion`。它们是本地 Run 观察窗口里的历史接收顺序候选，不证明任务成功、覆盖面积、事故原因或远端执行实例。

正常事件的 reviewRequired 等于 continuityUnknown、非空 reasonCode、非 bit6 的异常位或 ownerDestroyed 的逻辑或。单独 bit6 不强制人工复核。该字段只供只读复核建议，不放行自动动作、回滚、任务重新分配或飞行指令。

capture-gap 是整个转换流的缺口，不猜某架飞机或某个任务：前三 ID 明确空串、targets 为空列表、vehicleId=0、runIdHash 为空、observationRevision=`0`、revisionExhausted=false、候选 `not-applicable`、continuityUnknown/reviewRequired=true、anomalyMask=0、reasonCode=`capture-gap`、sourceRetirementCode 为空；mode 两项、observerClosed、ownerDestroyed 为 false。droppedTransitionCount 必须是正规范 uint64 字符串，拒绝 `0`；正常事件仍允许原累计计数为 `0`。其余固定声明仍按上表。缺口不伪装成某个真实任务的事件，也不抹去此前已观察事实。

## 固定持久投影 v3

`FleetEventArchiveCodec` 原 wrapper18、SQL schema、512 KiB 字节预算、深度 16 和 50000 节点上限保持不变。普通 v1 与工程审批 v2 的验证和编码仍保留；同一档案可混合三种记录。单条新 Run 事件不允许夹带 engineeringReview；不放宽旧工程事件 type/hash 对应关系，也不改变原工程 operation 搜索。

新 payloadProjection 为 `fixed-diagnostic-summary-v3`，投影恰 12 键：

`originalTypeHash/originalSourceHash/originalPayloadHash/payloadProjection/rawPayloadIncluded/omittedFieldCount/taskIdHash/startupIdHash/deploymentIdHash/targetVehicleIds/vehicleId/runObservation`。

原事件 type/source 必须精确匹配上文两字符串；解码还核对应的固定哈希及归档 event.type=`task`。外层 timestampMs 是 BlackBox 实际接纳时的本机 UTC，摘要 capturedAtUtcMs 保留原生捕获 UTC，两者可以不同，不用外层时间重写捕获时间或伪造跨机物理顺序。旧 Timeline 不因本链而排序或放宽时钟回退规则。rawPayloadIncluded 恒 false，omittedFieldCount 恒 0。三个 UUID 只保存 SHA-256；capture-gap 保三个明确空 hash，不保存 hash(空串)，vehicleId=0 与空 targets 仅此缺口分支允许。原 payload 本身不保存，原始内容哈希只是未认证的腐败检查和引用，不是签名或匿名化保证。

不保存任务 binding、完整 payload、账号/SID、scope token、原生 RAW source epoch、精确位置、轨迹、原始数据包或自由理由。现任务历史按保留的真实 taskIdHash 查询；没有 hash 的全局缺口不会凭时间、机号或遥测 run 猜入某个任务。SQL/HistoryReader 继续通过原 Codec 解码，没有新表、目录、索引、导入或重放授权。

## 工程交接与未执行项

本批仅源码及文档静态修改和交叉阅读；未执行编码器、SQL、解析器、辅助程序、QML、编译、测试、仿真、刷机或硬件操作。工程师需验证真实 Qt JSON 数字往返、v1/v2/v3 混档、规范计数及耗尽、非法类型/额外键/字段错配拒绝、gap 与正常事件、队列和磁盘缺口、源切换/销毁/通知重入以及真实历史与时间线接线。

历史读取不重新启动 Run，不恢复账号权限，不认证远端任务完成，不替代 ULog，也不改变仍为 false 的动态路径与最终飞行许可。

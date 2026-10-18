# 部署后独立只读对账：传输契约（426）

此文档描述 PlanManager / Vehicle 的源码实现，不代表已经编译、运行、仿真、实机验收或获准飞行。

## 调用与身份

`FleetMissionReconciliation` 是 Deployment 持有的实际 native 子对象。PlanManager 仅向该 friend 开放：

- `loadFleetMissionForDiagnostic(owner, primary, scopeId, maximumItemCount, fence)`：以新 UUID scope 发起独立 RAW INT 读取。`maximumItemCount` 必须为 1..2048，冻结于本次 scope；整组剩余预算为零时 workflow 不再发起读取。
- `cancelFleetMissionDiagnosticRead(owner, scopeId)`：只能取消原 owner 当前持有的诊断 scope；取消无需仍然有效的全授权，便于在撤销后排空。它不发送清空、返航、解锁等指令。
- `takeFleetMissionDiagnosticResult(owner, primary, scopeId, operationId)`：独立完成信号后排队调用，待原传输栈退出；实际来源仍一致才一次性取出完整结果，否则为空 map。再次读取、其他 owner、其他 scope/op 或更新的任务事务不能重用它。

完成信号为 `fleetMissionDiagnosticReadComplete(operationId, scopeId, success)`。它不是旧 `newMissionItemsAvailable` 信号，也不是任务内容一致性结论。失败或取消不发布部分 rawItems 结果。通知回调仍在 TransactionFinish 栈内，不能在直连回调中开始下一个读取或 take。

实际 owner 实现 `nativeReadContextCurrent(manager, vehicle, primary, scopeId, operationId)`，只核 frozen run / generation / target / 原对象；op=0 仅对应自己已准备的入门栈。每包和重试另外调用完整授权 fence，并前后重新核对实际所有权。调用方传入一个布尔值不能替代它。

## 传输与隔离

新增 `DiagnosticDisarmed` 是独立 admission。复用原来的真实来源、未解锁、已落地、状态新鲜、无外来传输策略，并拒绝正在启动的事务。原 StartDisarmed 和两种 POSCTL Review admission 不扩大；Review 判定使用显式枚举闭集。

只允许 `MISSION_REQUEST_LIST`、`MISSION_REQUEST_INT` 和完成读取时本地排队的 `MISSION_ACK(ACCEPTED)`。插件不能改包后绕过发送前后核对。读取采用 Vehicle 原始接收来源及 INT 项，不接受 FLOAT 回退或调整过的无来源信号。

只在诊断分支允许真实 `MISSION_COUNT=0`：实际收到 COUNT、完整 scope 有效、成功排队终止 ACK 后才形成结果。非零 COUNT 必须逐项完整读取；COUNT 超过冻结 cap 在创建索引/项之前拒绝。原启动和 Review 仍要求至少一项。

不同数量、不同有限字段、未知但可安全解码的命令不因“与期望不同”而拒绝传输；比较由 workflow 完成。非法布尔、非法非有限参数、不合法 DO_JUMP 目标、超限、错误来源、协议超时和 FLOAT 回退仍失败，不把这些情况转换为“空任务”。默认 yaw 的受支持 NaN 继续以无类型 null 保存。既有转换用的临时 MissionItem 仍有界，诊断结束后删除；保留的 rawItems 不使用显示缓存中的 DO_JUMP 重定位值。

成功诊断仅写入独立一次性结果槽；**不覆盖 `_missionItems`，不写 `_lastFleetReadReceipt`，不发旧 cache 完成信号**。新任务事务会失效旧诊断槽。诊断启动也使旧启动收据不可继续使用。不能从诊断 matched/count/hash 恢复 Deployment verified、StartReview permit 或执行授权。

## 启动返回

实际发起后沿用已有 RAW 读取的 9 字段返回：`ok`、`requestIssued`、`operationId`、`readScopeId`、`sourceEpoch`、`vehicleSourceEpoch`、`baselineReceiptSequence`、`vehicleId`、`componentId`。入口拒绝返回 `ok=false`、`requestIssued=false` 和固定 reason。

## 完整诊断结果（exact 35 字段）

- `schemaVersion`：Qt Int，1；`kind`：`diagnostic-raw-int-read`。
- `sourceEpoch`、`vehicleSourceEpoch`、`readScopeId`：原生 UUID 字符串。
- `operationId`、`receiptSequence`：正整数规范十进制字符串。
- `vehicleId`、`componentId`：原生整数；`itemCount`：Qt Int，0..2048 且不超过本 scope cap。
- `readStartedAtMonotonicMs`、`countReceivedAtMonotonicMs`、`firstItemReceivedAtMonotonicMs`、`lastItemReceivedAtMonotonicMs`、`completedAtMonotonicMs`、`durationMs`：有限、整数值 Qt Double。COUNT0 的 first/last 为 -1，其余时间仍有效；不是 UTC 时间。
- `countPayloadHash`、`rawItemsFingerprint`：小写 SHA-256。后者只是固定 raw 数组的本地摘要，**不是** canonical 任务 hash、来源认证或原子快照证明。
- `rawItems`：最多 2048 项。每项 exact 16 字段：`sequenceNumber`、`command`、`frame`、`param1`、`param2`、`param3`、`param4`、`x`、`y`、`z`、`current`、`autocontinue`、`missionType`、`targetSystem`、`targetComponent`、`rawPayloadHash`。有限参数为 Double，协议整数字段保持整数；默认 yaw 的 param4 为无类型 null。
- 以下 Bool 固定 true：`observed`、`requestIssued`、`countObserved`、`allItemsInt`、`readEndAckQueued`、`diagnosticOnly`、`historicalOnly`、`snapshotOnly`。COUNT0 时 allItemsInt 只表示没有非 INT 项，并不凭空证明存在航点。
- 以下 Bool 固定 false：`requestCausalityVerified`、`atomicSnapshotVerified`、`currentOnboardContentsProven`、`authenticated`、`signatureVerified`、`startupPermitGranted`、`executionVerified`、`flightCommandReleased`。

没有 available/fresh/age 的启动收据包装，没有可转移权限；原始接收、完整协议和本地 ACK 排队也不能证明远端当前内容、响应因果、消息认证或全队同时一致。多机逐台读取不是原子快照。

## 静态交接边界

本轮只编辑源码并进行文本读回 / diff 检查。Qt 编译、协议仿真、失败取消及重入时序、真实来源/空任务兼容、硬件和放行验证仍由工程师负责。

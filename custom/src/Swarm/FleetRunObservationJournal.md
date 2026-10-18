# 原生 Run 观察语义日志（510）

本模块说明原生受保护 Run 的低频观察变化如何进入现有黑匣子、持久事件归档、任务历史下钻、Timeline 和本地报告证据链。它不是新的执行器、正式任务结果裁决或自动任务交接。当前仅完成源码接线与静态阅读；编译、运行、测试、硬件、刷机、打包及 PR 均未执行。

## 1. 实际来源与消费者

1. [SwarmMissionOrchestrator](SwarmMissionOrchestrator.h) 只从已经提交到其私有 `TaskRecord` 的原生执行观察行提取固定语义。普通任务、导入摘要、QML Map、文件和外部调用者不能建立这个原生来源。
2. Orchestrator 实例持有独立 source epoch；每次捕获分配非回绕 transition sequence 和本机 UTC，先放入有界 FIFO。异步发布前取出队列项，发布期间才建立 `executionObservationEventReceiptMatches(epoch, sequence, exactPayload)` 的临时实际回执；它不是可持久化的授权凭据。
3. [GuidedActionsController.qml](../../res/Merivus/GuidedActionsController.qml) 只监听其实际私有 `_missionOrchestrator.executionObservationEventReady`。先冻结 lifetime、实际 Orchestrator、实际 BlackBox 和 plain 计数账本；闭合检查 6/28 键及实际发布回执，随后核同一来源与 lifetime，再调用现有 `FleetEventBlackBox.record("task.observation.transition", payload, "SwarmMissionOrchestrator")`。不 JSON 重建、不重新读取活 Run、不改发 `task.updated`。
4. 现有 BlackBox 的实际 native acceptance 回执继续进入显式启用的 `FleetPersistentEventStore`。Codec 将新事件投影为 v3 固定脱敏摘要；现有 `taskIdHash` 目录与 task-records 下钻保留该完整摘要。
5. 同一已接收的 BlackBox 事件进入原 Timeline、RuleEngine 和 `_appendMissionEvidence` 路径。只有 `reviewRequired === true` 的内置规则生成 `review_run_observation` 人工审阅建议；它不调用命令、`continuity.apply`、占用释放、恢复或交接。

原生 capture 的提交事实不依赖事件是否成功入队、转入 BlackBox 或写盘；日志丢失不回滚 Run，也不能把观察关闭提升为 Completed、Cancelled 或 Failed。

## 2. 语义变化与边界

每个目标成员只比较固定语义：outcome candidate、MODE queue began/committed、continuity unknown、已观察异常位、白名单 reason/source-retirement code，以及 observer closed/owner destroyed。首次为 `registered`；后续有变化才为 `changed`、`closed` 或 `owner-destroyed`。普通逐包当前序号、时间、年龄和计数递增不各自产生日志。

异常位记录“曾观察到”，不会被后续普通报告抹掉：

| bit | 含义 |
| --- | --- |
| 0 | MISSION_CURRENT 序号回退 |
| 1 | 序号跳变 |
| 2 | 离开观察到的 mission mode |
| 3 | HOME payload 变化 |
| 4 | 远端报告 no mission |
| 5 | 已知 total 与预期不一致 |
| 6 | 远端 complete 状态报告，未验证完成 |
| 7 | 来源退休已被观察到 |

`reviewRequired` 由 continuity unknown、非空 reason、除 bit 6 以外的异常位或 owner destroyed 得出。bit 6 单独出现不等于完成裁决，也不自动申请写动作。`closed` 本身只表示观察窗口关闭；没有异常时不能擅自补成失败。

正常候选只有：`no-terminal-candidate-observed`、`terminal-item-observed-not-completion`、`indefinite-terminal-hold-observed`、`landed-after-terminal-observed-not-completion`。均不证明可信远端任务实例、正式任务完成、结果归属或真实执行安全。

## 3. 原生值合同

发布 payload 固定 6 键：

```text
taskId, startupId, deploymentId, targetVehicleIds, vehicleId, runObservation
```

正常三个 ID 是原生 UUID；目标列表唯一且最多 32 项，`vehicleId` 在该原列表内。`runObservation` 固定 28 键：

```text
schemaVersion, transitionKind, transitionSourceHash, transitionSequence,
capturedAtUtcMs, observationRevision, revisionExhausted, runIdHash,
outcomeCandidate, modeQueueBegan, modeQueueCommitted, continuityUnknown,
anomalyMask, reasonCode, sourceRetirementCode, observerClosed,
ownerDestroyed, reviewRequired, droppedTransitionCount, snapshotOnly,
historyComplete, remoteMissionInstanceVerified, missionCompleted,
resultVerified, executionVerified, executionPermit,
flightCommandReleased, permissionsRestored
```

schema 为 1；UTC 为正的 JS 精确整数（最多 `2^53-1`），mask 为 0..255。sequence/revision/drop 是规范 uint64 十进制字符串，不能转 JS number；source/run hash 为小写 SHA-256。正常 observation revision 非零；耗尽时仍保已记录事实，不回绕为新来源。reason/retirement 是有界原生诊断代码；未知自由文本不外传。

`snapshotOnly=true`；`historyComplete`、`remoteMissionInstanceVerified`、`missionCompleted`、`resultVerified`、`executionVerified`、`executionPermit`、`flightCommandReleased`、`permissionsRestored` 恒为 false。

`capture-gap` 是独立丢失提示，不伪造某个任务：三个 ID、run hash 为空，targets 为空，vehicleId/revision 为 0，candidate 为 `not-applicable`，continuity unknown/review required 为 true，reason 为 `capture-gap`；其它原生事实标记保持 false。drop 是累计饱和计数，不是完整逐条丢失内容。

## 4. 有界捕获与可见丢失

原生 FIFO 最多 512 项、canonical payload 合计 2 MiB、单项 4096 bytes，每个异步 turn 最多发布 64 项。每个任务只新增固定成员语义缓存，不复制 128 份完整 observation。超出预算、不能形成合法摘要、时间无效或 sequence 耗尽都不能回滚已提交 TaskRecord；失败通过累计 dropped/pending capture-gap/sequence exhausted 诚实表示。

gap 只有在原时间与剩余容量等条件允许时才能发布；pending 不保证以后一定补出。未入队的语义不会在每次 poll 重试为重复事件。sequence/drop 不能回绕；sequence 永久耗尽后不再产生新序列，原状态和历史仍保留。

原生 `executionObservationEventStatus` 固定 12 键：

```text
schemaVersion, sourceEpoch, sourceHash, lastSequence,
queuedEventCount, queuedPayloadBytes, droppedTransitionCount,
pendingCaptureGap, sequenceExhausted, flushScheduled,
snapshotOnly, historyComplete
```

Root `runObservationJournal` 只公开该状态的值副本及本 Root 的 `acceptedCount`、`notRecordedCount`、`outcomeUnknown`、`counterExhausted`。计数最多 JS 安全整数，不回绕；耗尽或 record 调用结果不明时明确 unknown。BlackBox 未记录或拒绝与原生 FIFO 丢失是两个不同阶段，不互相抹掉。

record 返回后只写原已冻结 plain ledger；Root 仍存活且来源/账本仍相同时才发布新的计数副本。已接受事件即使同步通知销毁 Root，不能被改说成未记录。相反，`acceptedCount` 只表示内存 BlackBox 返回 true；公开 `persisted=false`、`historyComplete=false`，不能冒充 SQL commit、flush、持久化或完整历史证明。

## 5. 两个时间与现有回放

`runObservation.capturedAtUtcMs` 是原生语义快照捕获时的本机 UTC。事件需要异步排队，不能把这个早先时间重新当作日志入队时间插入现有 append Timeline；Root 使用 `BlackBox.record`，外层 `timestampMs` 是实际进入黑匣子的本机 UTC。

两个时间不相互替换、不要求相等，也不构成远端因果、时钟同步或实际飞行时间证明。保留原 BlackBox append 顺序和原 Timeline 接受域，不排序、不 clamp、不伪造单调 UTC。真实系统 UTC 回退仍可能使原 Timeline 的严格有序检查拒绝同步；这是未知/需人工核查边界，不被本批掩盖。

## 6. 归档、历史与隐私

Codec 的 `fixed-diagnostic-summary-v3` 固定 12 键：原 type/source/payload hash、projection marker、raw=false、omitted=0，task/startup/deployment 的 hash、原目标数字列表、成员 vehicleId 与原 `runObservation`。不存远端原始报文、坐标、凭据、自由文本或整份 observation。run/source hash 仅是本地诊断关联，不是身份认证或内容签名。

v3 不能与同一 payload 的 `engineeringReview` 混装；旧 v1/v2 编解码边界保持，归档数据库可以同时含旧记录与新记录。历史 `taskIdHash` 下钻只给本地摘要关联，不能恢复原 Run 权限、当前来源、自动重规划许可或任务控制权。旧档已被省略的 nested summary 不自动补回。

显式持久化默认开关、单 writer/reader、原预算/错误和真实提交状态保持。内存黑匣子仍受自身保留上限约束；源 FIFO、Root 转发计数与持久存储状态必须分别阅读。无日志、只有候选或缺最后事件，均不能推导“任务成功”或“没有异常”。

已有 [受保护 Run 报告](FleetProtectedRunReport.md) 的原生捕获、独立 report8 文件保存/重开与旧 full9 报告分型不变。本批仅使已接收的语义事件进入现有只读事件/报告证据路径，不新增报告真实性、自动归档所有 Run、跨站交接或恢复远端任务。

## 7. 静态交接与未执行验证

本批没有新面板、按钮、路由或资源注册；仅两个现有历史面板新增只读摘要文字，没有飞控修改、自动命令或授权通道。原 25 目标及六类实质剩余不缩减；原生历史候选仍不替代正式结果、完整执行闭环与工程验收。

工程师应按 [当前工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) 的 510 场景实际验证：低频语义变化/无逐包洪泛、begin 无 commit、部分 commit、来源退休、owner death/closure、原生预算与 gap/max、未记录/拒绝/结果未知计数、同步销毁/重入、v1/v2/v3 混合与 taskIdHash 下钻、日志 UTC 与捕获 UTC 分离、只审阅建议和零权限提升。

上述场景以及全部运行、解析、算法、编译、测试、硬件和打包验证均未执行；本合同不将静态阅读或普通文本核对称为运行通过。

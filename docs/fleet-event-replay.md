# Fleet OS 事件黑匣子与任务预演

## 295–299 封存历史独立读取

新增[Reader](../custom/src/Swarm/FleetEventArchiveHistoryReader.md)及[共用SQL](../custom/src/Swarm/FleetEventArchiveSql.md)，从Store native明确consent读取封存目录/页，不运行原TimelineReplay回调，不重建当前规则上下文，更不恢复任务/批准。catalogId/实际file+DBepoch+filter/next绑定是只读浏览作用域，不是权限；目录观察数/空列表不证明全集，basename非飞行序，原unclean/drop/retired逐行保留。Reader不读取current库或创建库，默认无目录/SQL操作，当前未运行/编译/测试。后面285–289描述为该历史批次边界。

## 285–289 独立连续归档边界

新增[持久化门面](../custom/src/Swarm/FleetPersistentEventStore.md)、[固定摘要](../custom/src/Swarm/FleetEventArchiveCodec.md)和[数据库线程](../custom/src/Swarm/FleetEventArchiveWorker.md)。原BlackBox内存/快照/TimelineReplay行为保留；新SQL归档默认关闭，仅显式native配置开启，捕获本实际BlackBox的新暂时收据，不接受import/clear或旧事件缓存作新来源。历史按数据库row_id读取且独立显示，不导入BlackBox、不触发当前规则运行态或TimelineReplay.eventReady，也不恢复任务/审批/控制。新库保存固定诊断摘要而非完整遥测或原payload，hash非签名/匿名化，clean不是事件完整性；长期轮换/完整采集仍待开发。本机未建库/执行SQL、编译、运行或测试。

`FleetEventBlackBox` 是只保存 QVariantMap 的传输无关事件日志。调用 `startRecording()` 后使用 `record(type, payload, source)` 追加事件；事件包含 `timestampMs`、`type`、`source` 和 `payload`。它不会连接或发送飞行链路命令。

事件日志默认最多保留 10000 条，可通过 `maxEvents` 调整，并可用 `exportJson()`/`importJson()` 处理旧版事件数组。用于跨进程交接时使用带 `format`/`schemaVersion` 的 `exportSnapshotJson()`/`importSnapshotJson()`；快照导入会先完整校验事件和 `maxEvents`，再原子替换当前内存日志与保留上限，但不会恢复录制开关。导入时会拒绝非对象、缺少 `timestampMs`/`type`/`source`/`payload` 的不完整记录、非字符串类型字段、非正数/超出小幅时钟容差的未来时间戳或非 map 的 `payload`，避免把无法排序或审计的损坏记录静默当成证据；`recordAt()` 也执行同一时间边界。导入的记录仍然只是诊断数据，不会重放成 MAVLink 命令。

本地文件交接可调用 `saveSnapshotToFile(filePath)` 和 `loadSnapshotFromFile(filePath)`。保存使用 Qt `QSaveFile` 原子提交，空路径、打开/写入/提交失败都会返回 `false`；加载拒绝空路径、不可读文件、超过 32 MiB 的文件或任何快照校验错误。加载沿用 `importSnapshotJson()` 的“先校验、后替换”语义，失败时保留当前内存日志。该接口只是本地文件 I/O，不负责签名、加密、访问控制或留存/清理周期。

`FleetTimelineReplay` 也支持版本化 `exportJson()`/`importJson()`，并提供 `saveToFile(filePath)`/`loadFromFile(filePath)` 本地交接入口。保存使用 `QSaveFile` 原子提交；空路径、读写失败和超过 32 MiB 的输入都会被拒绝，导入校验失败不会替换现有时间轴。它只把事件送入本地时间轴，不会调用 `Vehicle`、MAVLink 或任何飞行命令入口。文件签名、权限、加密和保留周期仍需工程师另行设计；当前模块不声称具备不可抵赖存储。

`FleetTimelineReplay` 接收与 `FleetEventBlackBox` 相同的事件列表，通过 `step()` 或 `eventReady` 信号逐项回放。每条事件必须包含真实有限整数 `timestampMs`、非空 `type`、字符串 `source` 和 Map `payload`；type/source 长度、payload 深度/节点/大小受限，时间戳必须保持非递减顺序。`play()`/`pause()` 控制内部可配置间隔的 `QTimer`；需要完全确定性的测试时可只调用 `step()`，避免依赖墙上时钟。

在 `GuidedActionsController` 中，黑匣子接受事件后会同步 `FleetTimelineReplay.events`；回放的 `eventReady` 只复用事件规则评估并更新人工复核建议。导入黑匣子快照后会重建规则运行态；暂停后继续播放不会把规则上下文重置，只有从位置 0 开始新的回放才重置。指挥中心当前只显示位置/播放状态，实际播放、步进和文件导入仍由后续 QML/工程师接入决定。

`FleetMissionSimulator` 接收描述性 `steps`，可用 `injectFault(kind, details)` 注入模拟故障；`preview()` 返回标记为 `simulated` 的步骤结果，并附带受影响飞机、降级建议和 `requiresReview`。三个后端类当前均注册到 QML 内部模块 `MerivusInternal 1.0`，公共界面仅接受窄facade/值，不直接持有可变后端；不会调用 SwarmController、GuidedActionsController 或任何真实飞行 API。

`FleetMissionDigitalTwin` 是更高层的纯数据预演入口：它同时接收 `fleet`、`mission`、`map`、`link` 和 `energy` 快照，使用 `schemaVersion=1` 的版本化 JSON 信封保存状态，并按稳定输入生成 `deterministic=true` 的时间线、摘要和 SHA-256 哈希。`injectFault()` 对失联、低电量、GNSS 下降、Leader 故障、链路下降和通信延迟生成 `faultSuggestions`，所有建议都带人工确认和 `flightCommandReleased=false`。它不能替代飞控仿真或安全模型；导入验证失败会保留原有内存快照。

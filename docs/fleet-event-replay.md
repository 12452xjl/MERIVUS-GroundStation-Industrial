# Fleet OS 事件黑匣子与任务预演

`FleetEventBlackBox` 是只保存 QVariantMap 的传输无关事件日志。调用 `startRecording()` 后使用 `record(type, payload, source)` 追加事件；事件包含 `timestampMs`、`type`、`source` 和 `payload`。它不会连接或发送飞行链路命令。

事件日志默认最多保留 10000 条，可通过 `maxEvents` 调整，并可用 `exportJson()` 导出、`importJson()` 导入，用于离线归档和回放。导入的记录仍然只是诊断数据，不会重放成 MAVLink 命令。

`FleetTimelineReplay` 接收 `events` 列表，通过 `step()` 或 `eventReady` 信号逐项回放。`play()`/`pause()` 控制内部可配置间隔的 `QTimer`；需要完全确定性的测试时可只调用 `step()`，避免依赖墙上时钟。

`FleetMissionSimulator` 接收描述性 `steps`，可用 `injectFault(kind, details)` 注入模拟故障；`preview()` 返回标记为 `simulated` 的步骤结果。三个类均注册到 QML 模块 `Merivus 1.0`，不会调用 SwarmController、GuidedActionsController 或任何真实飞行 API。

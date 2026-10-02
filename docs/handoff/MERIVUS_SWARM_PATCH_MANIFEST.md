# MERIVUS 集群地面站补丁清单

本清单对应 2026-10-02 当前工作树的源码改动。它不是 Git commit，也不是编译产物；工程师接手时应先在仓库中执行 `git status --short`，确认这些文件与交接包一致，再自行建立分支或提交。

## 新增文件

```text
custom/src/Swarm/CommandTransaction.cc
custom/src/Swarm/CommandTransaction.h
custom/src/Swarm/FaultToleranceManager.cc
custom/src/Swarm/FaultToleranceManager.h
custom/src/Swarm/FleetRegistry.cc
custom/src/Swarm/FleetRegistry.h
custom/src/Swarm/FormationPlanner.cc
custom/src/Swarm/FormationPlanner.h
custom/src/Swarm/FleetCapabilityMatcher.cc
custom/src/Swarm/FleetCapabilityMatcher.h
custom/src/Swarm/FleetIntentTask.cc
custom/src/Swarm/FleetIntentTask.h
custom/src/Swarm/FleetRiskRadar.cc
custom/src/Swarm/FleetRiskRadar.h
custom/src/Swarm/FleetEventBlackBox.cc
custom/src/Swarm/FleetEventBlackBox.h
custom/src/Swarm/FleetTimelineReplay.cc
custom/src/Swarm/FleetTimelineReplay.h
custom/src/Swarm/FleetMissionSimulator.cc
custom/src/Swarm/FleetMissionSimulator.h
custom/src/Swarm/FleetExtensionRegistry.cc
custom/src/Swarm/FleetExtensionRegistry.h
custom/src/Swarm/FleetRolePolicy.cc
custom/src/Swarm/FleetRolePolicy.h
custom/src/Swarm/FleetTaskTemplateRegistry.cc
custom/src/Swarm/FleetTaskTemplateRegistry.h
custom/src/Swarm/MissionHandoffManager.cc
custom/src/Swarm/MissionHandoffManager.h
custom/src/Swarm/SwarmMissionOrchestrator.cc
custom/src/Swarm/SwarmMissionOrchestrator.h
custom/src/Swarm/VehicleCapability.cc
custom/src/Swarm/VehicleCapability.h
docs/MERIVUS_SWARM_ARCHITECTURE.md
docs/MERIVUS_SWARM_TARGET_MATRIX.md
docs/MERIVUS_FLEET_OS_COLLABORATION.md
docs/fleet-event-replay.md
custom/src/Swarm/FleetIntentRisk.md
docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md
docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md
```

## 修改文件

```text
custom/custom.pri
custom/src/CustomPlugin.cc
custom/src/Swarm/SwarmController.cc
custom/src/Swarm/SwarmController.h
custom/res/Merivus/GuidedActionsController.qml
custom/res/Merivus/FlyViewMap.qml
custom/res/Merivus/CommandCenterOverlay.qml
docs/INDEX.md
docs/architecture/CURRENT_STATE.md
docs/development/SITL_MULTI_VEHICLE_MISSION_TEST.md
docs/development/SITL_SWARM_TASK_ISOLATION_FEATURE_BRIEF.md
docs/development/SITL_SWARM_TASK_ISOLATION_SAFETY_REVIEW.md
docs/handoff/README.md
```

## 变更分层

| 层 | 内容 | 是否直接控制飞行器 |
| --- | --- | --- |
| 数据层 | `VehicleCapability`、`FleetRegistry` | 否 |
| 产品规划层 | `FleetIntentTask`、`FleetCapabilityMatcher`、`FleetRiskRadar` | 否；只生成方案、角色和风险解释 |
| 事务层 | `CommandTransaction`、`SwarmController` ACK/遥测关联 | 仅经现有 QGC 人工入口发起；不自动决策 |
| 任务层 | `SwarmMissionOrchestrator`、`FaultToleranceManager` | 否；故障模块只产生建议 |
| 审计/预演层 | `FleetEventBlackBox`、`FleetTimelineReplay`、`FleetMissionSimulator` | 否；仅记录、回放和故障模拟 |
| 协同/扩展层 | `FleetRolePolicy`、`MissionHandoffManager`、`FleetTaskTemplateRegistry`、`FleetExtensionRegistry` | 否；权限和目录元数据不释放飞行命令 |
| 编队层 | `FormationPlanner`、编队阶段事务 | 需要工程师确认机载协议后才能用于真机 |
| 界面层 | Guided Actions、地图、指挥中心 | 否；只展示和转发人工操作 |

## 当前静态检查结果

- `git diff --check`：已通过。
- `custom/custom.pri` 中新增源文件和头文件路径：已逐项确认存在。
- 交接前静态审查发现的编队 ACK 监听缺口已修复：`_ensureFormationCommandConnection()` 现在同时接入通用事务处理器，编队事务可接收同一条 `mavCommandResult`。
- `FaultToleranceManager.cc` 已显式包含 `<QStringList>`，避免依赖传递包含。
- Leader 故障转移角色已对齐：UAV-1 的注册角色使用 `leader`，掉线时可生成 `reassign` 建议，不会被误判为普通成员而只执行 `hold`。
- `SwarmController::_refreshFleetRegistry()` 已把可读的电池平均值、健康检查摘要和 RADIO_STATUS RSSI 估算值持续写入 `FleetRegistry`；估算字段保留原始 dBm 与来源标记，未知值不转为健康。
- Qt/MSVC/qmake 编译：本机工具链不可用，未执行。
- Mock/SITL、日志回放、硬件测试、刷机：未执行。

## 工程师操作建议

1. 先阅读 [`MERIVUS_SWARM_ENGINEER_HANDOFF.md`](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
2. 在独立分支保存当前工作树后生成 qmake 并编译。
3. 只根据编译日志修复接口/Qt 兼容问题；不要删除事务、ACK、超时或安全前置检查来绕过错误。
4. 编译通过后按交接文档执行 Mock/SITL 和故障注入，再决定是否进入硬件流程。

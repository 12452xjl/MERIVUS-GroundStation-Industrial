# FleetAutonomyPolicy 契约

`FleetAutonomyPolicy` 是 Fleet OS 目标 24/25 的只读策略声明层。它把任务阶段固定为 `manual`（人工）、`assisted`（辅助）、`supervised`（监督）和 `automated`（自动化），同时声明 PX4、ArduPilot 与伴随计算机视觉插件的能力、权限范围、签名状态和沙箱占位。

## 只读边界

- 构造函数发布内置阶段与适配器声明；没有注册、启用、删除、加载插件或修改策略的接口。
- `evaluate(stage, action, role, adapterId)` 只返回审阅结果，包含阶段权限、角色范围、所需能力、人工/双人审批、签名检查和沙箱检查。结果中的 `allowed` 仅表示策略层允许继续审阅；`reviewOnly=true`、`executionAllowed=false` 和 `commandDispatch=not-released` 固定表明不可执行。
- `role` 为空时结果会标记 `roleCheckRequired=true` 并保持 `allowed=false`；调用方必须先用 `FleetRolePolicy` 解析当前角色。
- `signatureStatus=unverified`、`sandboxProfile=*_pending`、`enabled=false` 是当前占位状态。真实签名验证、版本兼容、权限审批和进程隔离由宿主产品接入。
- `flightCommandReleased` 和 `commandRelease` 始终为 `false`。模块不访问 `Vehicle`、`MAVLink`、`PX4`、`ArduPilot`、`SwarmController`，也不执行命令。

## 阶段策略

| 阶段 | 允许的策略范围 | 权限/审批占位 |
| --- | --- | --- |
| `manual` | 人工查看、任务预览和人工 UI 指令 | `operator/commander/administrator`；操作员确认；`human_ui_only` |
| `assisted` | AI 提案、风险解释和任务预览 | 规划员及以上；人工确认；Agent 沙箱待接入；`proposal_only` |
| `supervised` | 有界自动化建议与监督 | 指挥员/安全员/管理员；双人审批；签名适配器和沙箱待接入；`bounded_review_only` |
| `automated` | 策略允许的有界自动化规划 | 仅管理员；双人审批、签名适配器和沙箱待接入；当前版本禁用命令释放 |

阶段权限只表示“策略层可以继续审阅”。调用方仍须先通过 `FleetRolePolicy`，再执行车辆健康、链路、解锁、空域和任务安全门禁。

## 跨飞控与插件声明

`platformAdapters` 返回以下声明：

- `px4`：遥测、健康快照、任务预览、构型只读和命令预览能力。
- `ardupilot`：遥测、健康快照、任务预览、构型只读和参数元数据能力。
- `companion.vision`：视觉检测、遥测标注和任务上下文能力，飞控类型为 `agnostic`。

这些能力是适配器目录元数据，不是运行时探测结果。`adaptersForFlightController()` 只按飞控名称筛选声明；它不会实例化适配器或调用外部进程。扩展签名、权限范围、沙箱配置和飞控版本兼容必须由后续工程实现并记录在审计链中。

## QML 接入

`GuidedActionsController.qml` 暴露只读属性 `autonomyPolicy`。UI 可以读取 `stages`、`platformAdapters` 和 `evaluate()` 的结果来显示阶段门槛；任何结果都不能直接连接到 `SwarmController`、`Vehicle` 或 MAVLink 命令入口。

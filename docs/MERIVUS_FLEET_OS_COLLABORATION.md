# MERIVUS Fleet OS：协同、交接与扩展基础层

本批次在现有 QGroundControl Custom Build 的集群层旁边增加四个独立、传输无关的产品能力模块。它们只管理人工操作权限、任务元数据和扩展目录，不直接访问 `Vehicle`、MAVLink 或 PX4，也不会绕过现有的解锁、链路、估计器和任务安全检查。

## 模块

| 类 | 作用 | QML 类型 |
|---|---|---|
| `FleetRolePolicy` | 观察员、规划员、操作员、安全员、维修、总指挥、管理员的权限模型；返回允许/拒绝原因和二次确认要求 | `FleetRolePolicy` |
| `MissionHandoffManager` | 记录任务从一个操作员/控制中心交接给另一个操作员的请求、接受、拒绝、取消和过期 | `MissionHandoffManager` |
| `FleetTaskTemplateRegistry` | 注册巡逻、工业巡检、搜索救援、仓储盘点等行业任务模板和能力需求 | `FleetTaskTemplateRegistry` |
| `FleetExtensionRegistry` | 登记审计、报告、行业插件等可选扩展的元数据；默认关闭，不加载任意二进制 | `FleetExtensionRegistry` |

## 使用边界

这些类是“策略/目录层”，不是飞行控制器：

- `FleetRolePolicy::canPerform()` 只表示当前角色是否可以执行某个人工 UI/任务动作；即使返回 `true`，命令适配器仍必须重新做车辆健康、位置、链路、解锁和安全前置检查。
- `MissionHandoffManager::acceptHandoff()` 只确认任务所有权转移，不代表任务已上传、收到 MAVLink ACK 或飞行器已改变模式。
- `FleetTaskTemplateRegistry` 只提供意图模板和能力约束，不能替代航线规划、禁飞区校验、电量预测和人工确认。
- `FleetExtensionRegistry` 只存储扩展元数据。生产环境还需要签名校验、版本兼容检查、沙箱和权限审批，不能把 `entryPoint` 当作可执行路径直接加载。

## 推荐接入方式

1. 登录或控制中心切换时设置 `FleetRolePolicy.currentUserId/currentRole`。
2. 人工动作按钮先调用 `evaluate(action)`；被拒绝时向用户展示 `reason`，二次确认动作显示 `requiresSecondApproval`。
3. 任务交接由发送方调用 `requestHandoff()`，仅目标操作员可以 `acceptHandoff()` 或 `rejectHandoff()`；发送方可在接受前取消。
4. 任务向导从 `FleetTaskTemplateRegistry.templates()` 选择模板，再交给现有任务编排器生成预览；模板本身不触发执行。
5. 扩展启用前由宿主产品完成签名、权限和版本检查，再调用 `setExtensionEnabled()`。禁用/移除扩展不会影响在飞任务。

## 工程师验收重点

- C++/moc 能编译，Qt 5.15 QML 可实例化四个类型。
- 观察员不能创建/启动任务；规划员只能创建、编辑和预演；操作员的飞行相关动作应要求安全策略再次检查；安全员可审批/中止但不能凭角色绕过飞控安全。
- 交接只能由目标操作员接受或拒绝；重复解决、错误目标、自己交接给自己都必须失败。
- 模板 ID、扩展 ID 重复注册时应覆盖元数据但不自动启用，也不执行任何外部代码。
- 断开地面站或重启后，尚未持久化的内存记录不应被当作任务已完成；如需跨进程恢复，另行实现持久化和审计日志。

这些检查是接口验收，不构成 SITL、硬件或刷机放行。真实动作仍由现有 `SwarmController` 和工程师的测试流程负责。

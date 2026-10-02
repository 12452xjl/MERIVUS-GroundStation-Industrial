# MERIVUS 集群地面站工程师交接说明

> 交接日期：2026-10-02
> 交接对象：GroundStation 集群能力第一阶段源码改动
> 交接边界：本批次只修改地面站状态建模、事务编排和界面接入；不修改 PX4 姿态控制、混控、电机输出或飞控参数。

## 1. 先说结论

本批次代码已经整理到当前工作树，目标是让工程师先完成“能编译、能在 Mock/SITL 中观察事务和状态、能复现异常路径”的接手阶段。

本机没有 Qt/MSVC/qmake 构建工具链，因此本批次没有声称完成编译、仿真、硬件测试或刷机放行。工程师必须在受支持的 Windows Qt 环境中重新生成工程、编译并提供日志；真实机测试仍由工程师按现场安全流程负责。

## 2. 本批次新增/修改内容

### 新增 C++ 集群模块

- `custom/src/Swarm/VehicleCapability.h/.cc`
  - 统一车辆能力与基础健康字段的序列化。
  - 对未知字段保持未知，不把缺失遥测当作健康。
- `custom/src/Swarm/FleetRegistry.h/.cc`
  - 登记当前 QGC 车辆、连接/固件/机型/位置有效性摘要。
  - 为 QML、任务编排和故障管理提供统一车辆快照。
- `custom/src/Swarm/CommandTransaction.h/.cc`
  - 提供事务 ID、目标车辆集合、逐机结果、超时、重试、部分成功和终态。
- `custom/src/Swarm/SwarmMissionOrchestrator.h/.cc`
  - 提供任务创建、启动、暂停、恢复、取消、进度和成员重分配状态机。
- `custom/src/Swarm/FormationPlanner.h/.cc`
  - 提供编队/分裂/合并规划、ENU 偏移和最小间距校验。
- `custom/src/Swarm/FaultToleranceManager.h/.cc`
  - 根据已提供的车辆摘要生成掉线、低电量、Leader 候选和降级建议。
  - 只给出建议，不代替 PX4 安全逻辑，也不会自动发飞行命令。

### 修改 C++/QML 接入

- `custom/custom.pri`
  - 将上述新模块加入 qmake 工程。
- `custom/src/CustomPlugin.cc`
  - 注册 `FleetRegistry`、`CommandTransaction`、`SwarmMissionOrchestrator`、`FormationPlanner`、`FaultToleranceManager` 等 QML 类型。
  - 重新打开多机列表入口。
- `custom/src/Swarm/SwarmController.h/.cc`
  - 统一维护 `FleetRegistry` 和事务集合。
  - 起飞、降落、返航、直接指点和编队 PREPARE/COMMIT/RELEASE/ABORT 均返回事务 ID，并等待 ACK 或遥测结果。
  - 延迟发送前重新检查车辆就绪条件；失败车辆不会被伪装成成功。
  - 当前编队入口仍固定 UAV-1 为 Leader，支持 1–6 个选中成员；六机选择时要求 system ID 1–6 完整集合。
- `custom/res/Merivus/GuidedActionsController.qml`
  - 只创建一个共享 `SwarmController`，向任务编排和故障管理转发统一快照。
  - 任务状态会反映编队事务和故障建议；故障策略只暂停/重分配任务，不自动发送飞行命令。
- `custom/res/Merivus/FlyViewMap.qml`
  - 移除重复的本地 `SwarmController`，改为使用 GuidedActionsController 的共享实例。
- `custom/res/Merivus/CommandCenterOverlay.qml`
  - 展示舰队健康、任务摘要和故障建议数量；编队选择校验统一为 1–6。

### 文档同步

- `docs/MERIVUS_SWARM_ARCHITECTURE.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/architecture/CURRENT_STATE.md`
- `docs/development/SITL_MULTI_VEHICLE_MISSION_TEST.md`
- `docs/development/SITL_SWARM_TASK_ISOLATION_FEATURE_BRIEF.md`
- `docs/development/SITL_SWARM_TASK_ISOLATION_SAFETY_REVIEW.md`
- `docs/INDEX.md`

上述文档已把旧的“只允许 1/2/6 机”描述修正为当前入口的 1–6 机范围；这只是地面站入口约束，不代表协议已经支持任意规模集群。

## 3. 工程师接手步骤

### 3.1 固定当前工作树

在开始修编译前保存当前差异，不要直接覆盖或重置：

```powershell
cd E:\feikong\GroundStation
git status --short --branch
git diff --stat
git diff --check
```

确认所有新增 `custom/src/Swarm/*` 文件都已纳入工程后，再进行 qmake/构建。不要提交 `build/`、`agent/build/`、`agent/dist/` 或 staging 产物。

### 3.2 生成并编译

构建基线和 GStreamer 要求见 [`BUILD_WINDOWS.md`](../development/BUILD_WINDOWS.md)。Release 构建入口：

```powershell
cd E:\feikong\GroundStation
powershell -ExecutionPolicy Bypass -File tools/dev/build-merivus.ps1 -Configuration Release
```

工程师应优先处理以下类别的错误：

1. qmake 文件列表、include 路径和 Qt moc 生成错误；
2. `Q_OBJECT`/QML 注册、信号签名和 Qt 5.15 API 兼容问题；
3. `Vehicle` 实际版本中不存在的信号或属性；
4. MAVLink/MAV_CMD 枚举和 ACK 参数在当前 QGC 基线中的差异；
5. QML 绑定循环、对象生命周期和多实例重复连接。

每次修复后请保留首次错误和最终修复的构建日志，便于回溯；不要通过关闭 warning 或删除事务检查来“修到能编译”。

### 3.3 SITL/Mock 验收顺序

按下面顺序逐层增加风险，未通过上一层不得进入下一层：

1. 单机 Mock：检查界面能发现车辆，事务能进入 `Pending/Running/终态`，失败不会显示为完成。
2. 双机 Mock/SITL：检查选中集合、逐机 ACK、部分成功、拒绝和超时。
3. 3–6 机 Mock/SITL：检查编队 PREPARE → COMMIT → RELEASE；任一成员失败或超时必须进入 ABORT/回滚路径。
4. 断链/重连注入：检查 `FaultToleranceManager` 只生成 `reassign/split/return/hold` 建议，不能越权自动发飞行命令。
5. 任务注入：检查暂停、恢复、取消、成员重分配和进度状态不会绕过事务状态机。
6. 日志回放：核对界面显示的“已发送、已收到 ACK、已完成”与实际 MAVLink/遥测事件分别对应。

推荐从 [`SITL_MULTI_VEHICLE_MISSION_TEST.md`](../development/SITL_MULTI_VEHICLE_MISSION_TEST.md) 开始，并把新增事务/编队案例记录到工程师自己的测试报告中。

## 4. 进入硬件测试前必须补齐的内容

- 用当前 QGC 基线的 `Vehicle` 信号补齐电池、链路质量、估计器健康、任务进度字段；未知值必须继续保持未知。
- 确认实际 PX4/MAVLink 版本对 `MAV_CMD_NAV_TAKEOFF`、`MAV_CMD_DO_REPOSITION`、模式切换以及编队自定义命令的 ACK 语义。
- 确认真实机的 system ID、组件 ID、遥测链路和多机带宽；不能把 UAV-1/1–6 的临时限制当作通用协议。
- 明确 `FormationPlanner` 的 ENU 偏移如何转换到机体/全球坐标，以及偏移由地面站发送还是由机载编队控制器消费。
- 为临时 Mission 队列补齐“上传成功、启动 ACK、执行完成”的可观测闭环；在此之前只能显示“已发起/待核实”。

上述项目完成后，才进入台架、系留、限速和逐级放飞流程。地面站代码不能证明悬停不漂、断桨悬停、碰撞后自稳定或复杂环境避障；这些属于飞控/动力/传感器和真实飞行验证范围。

## 5. 已知限制和不可误读项

- 当前 FleetRegistry 的健康摘要是第一阶段基础字段，不是完整飞控健康判定。
- `CommandTransaction` 提供重试模型，但当前 `SwarmController` 没有把所有命令都自动重试；是否重试必须按命令和安全策略显式设计。
- 编队协议当前固定 UAV-1 为 Leader、最大 6 机；动态 Leader、超过 6 机和复杂网络拓扑需要单独做协议/带宽设计。
- `FaultToleranceManager` 的建议不会自动替飞机执行 RTL、降落或改航；最终故障保护仍由 PX4 机载逻辑承担。
- 本地环境没有完成 Qt/MSVC 编译，任何“编译通过”“SITL 通过”“可刷机”结论都必须由工程师重新给出证据。

## 6. 放行签字边界

本源码包交付到“工程师接手、编译修复、仿真验证”的阶段。只有工程师完成代码审查、构建、SITL/故障注入、台架/系留和现场飞行流程，并确认日志与风险评估后，才能决定是否允许刷机或放飞。

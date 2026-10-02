# 分阶段编队、多机动作与实时任务隔离功能说明

## Outcome

UAV-1 为当前协议主机，UAV-2～UAV-6 为可选从机；同一套协议支持包含 UAV-1 的 1～6 机验证。点击“编队”只触发配套 PX4 `swarm_node`，不读取或启动任意机载 Mission。启动采用 PREPARE、COMMIT、RELEASE、ABORT 事务，任一成员失败都会回滚整组。

## Operator workflow

1. 启动 QGC；六机编队和 Shift 临时 Mission 默认启用，不再依赖环境变量。
2. 启动 UAV-1：`make px4_sitl_default gazebo`。
3. 启动五架从机：`./Tools/simulation/gazebo-classic/sitl_multiple_run.sh -m iris -n 5`。
4. 在地图框选包含 UAV-1 的 1～6 机，确认界面核对成员后启动编队。
5. 框选两架或更多无人机后，起飞、降落和返航确认条冻结本次目标；返航统一使用标准 RTL。
6. 普通点击车辆会排他选择该机；普通右键立即更新当前选择的目标点。
7. Shift+右键第一个航点会冻结目标 ID 和几何关系；释放 Shift 后通过顶部滑动或长按空格确认。目标中已有任务时，确认内容会明确说明替换。
8. 临时任务到达末点并悬停后，QGC 请求清除该机临时 Mission；未收到清除 ACK 时标记为残留任务。

## Layer and owners

- Hardware: 配套 PX4 仓库的 `px4_fmu-v6c_default` 已包含 `swarm_node`，目标板为 Pixhawk 6C Mini；本文不执行真实飞机测试。
- LTE/RTK: 不在本次范围。
- QGC C++: `custom/src/Swarm/SwarmController.*` 负责成员校验、四阶段事务、逐机 ACK 聚合、失败回滚、主机位置租约、批量动作和临时任务状态。
- QML: `GuidedActionsController.qml`、`CommandCenterOverlay.qml`、`FlyViewMap.qml`、`PlanView.qml` 负责确认、选择冻结、任务显示和禁止跨机继承。
- MAVLink: `MAV_CMD_USER_1/2/3/4` 分别承载 PREPARE、COMMIT、RELEASE、ABORT。协议版本为 `2`，参数包含成员位图、主机 ID 和 24 位会话 ID。
- Position lease: 主机 `GPS_RAW_INT` 只作为位置来源；地面站向本次所有成员发送 `FOLLOW_TARGET`，其 `custom_state` 绑定会话 ID，并校验最初 PREPARE 的 MAVLink source。
- PX4: 配套源码位于 `E:\MERIVUS\FirmwarePX4`。`swarm_node` 负责预检、setpoint 预热、Offboard、解锁、起飞、Ready 屏障、Control，以及所有成员 3 秒租约超时转 AUTO_LOITER。
- Cloud/AI: 不接入。

## Safety impact

- Worst credible failure: 旧目标集合残留导致指令串机；错误启动旧 Mission；主机数据失效后从机继续盲飞。
- Required interlocks: 目标必须为包含 UAV-1 的 1～6 机；任一成员不合格、拒绝或超时则整组 ABORT；所有成员 Ready 后才 RELEASE；所有成员租约超时均进入悬停。
- Human confirmation: 编队启动、编队结束、Shift 临时任务和任务替换均需确认；普通右键单点按 MOBA 语义即时执行。
- Audit evidence: UI 结果区分已调度与已跳过；Mission 清理失败会明确提示残留。

## Scope

- Allowed files/components: `custom/src/Swarm/`、相关 MERIVUS Fly/Guided QML、`src/PlanView/PlanView.qml`、开发测试与说明。
- Prohibited files/components: `agent/`、AI Executor、真实飞行参数、云服务。
- Upstream QGC impact: 仅收紧 Plan View 切换车辆时的草稿处理；连接车辆之间不再允许把上一架的编辑草稿保留到新车辆上下文。

## States and failures

- Empty/loading/stale/offline: 无选择或坐标无效时拒绝；断链目标不发送；清除失败标记 stale。
- Timeout/cancel/retry: 地面站任一阶段等待逐机 ACK 超时后整组 ABORT；即使地面站失联，所有成员也会因机载租约超时转 AUTO_LOITER。
- Partial success: 普通多机指令可列出跳过目标；编队事务不允许部分成功。
- Recovery: 活动编队通过“结束编队”停止；残留临时 Mission 在下一次替换确认后覆盖。

## Acceptance criteria

- “编队”不再调用 `executeStartMissions`，只调用带冻结目标的 `sendStartCommand`。
- 编队和 Shift 临时任务默认启用，不再读取 `MERIVUS_DEV_ENABLE_SWARM_*` 环境变量。
- 编队允许包含 UAV-1 的 1～6 机；六机时必须为完整 UAV-1～UAV-6，并要求所有成员预检通过。
- 框选多机后，起飞、降落和标准返航冻结目标并逐机调度；单机失败不阻止其他合格目标。
- UAV-1 位置租约只发送给本次冻结成员，包括主机自身；不发送给其他连接车辆。
- 启动/停止不再伪装 `GPS_RAW_INT`；非法版本、成员位图、主机 ID 或会话 ID 由 PX4 拒绝。
- PREPARE 在机载预检后最终 ACK；COMMIT 在成员到达 Ready 后最终 ACK；全部 Ready 后才发送 RELEASE。
- 任一阶段拒绝、无响应、链路故障或人工结束都向整组发送 ABORT。
- 普通点击另一架机后，上一架的实时任务继续执行，但新右键指令只发给新焦点机。
- Shift 草稿在首点冻结目标和参考位置；目标切换会取消草稿。
- Shift 队列使用统一顶部滑动确认条，不再使用 Windows 平台原生白色弹窗。
- 切换无人机不会自动弹出“继续任务”；继续任务仅从动作列表手动触发。
- 同一车辆已有临时任务时，新的 Shift 队列必须确认替换；普通右键可即时抢占。
- 临时 Mission 完成后请求清除，旧路线不再作为当前任务恢复显示。

## Verification plan

- 执行 `tools/dev/test-sitl-swarm-task-isolation.ps1` 和 `git diff --check`。
- 环境具备时进行 QML/C++ 局部构建。
- 在配套 PX4 SITL 依次覆盖 1～6 机，并注入错误协议、错误成员位图、重复/过期会话、部分 ACK 丢失、PREPARE/COMMIT/RELEASE 失败、主机断链、地面站退出和从机断链。
- 不连接真实飞机。

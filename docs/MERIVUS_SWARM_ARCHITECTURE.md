# MERIVUS 集群地面站开发基线

本文档记录 MERIVUS GroundStation 集群能力的基础实现边界。目标是保留
QGroundControl/PX4/MAVLink 基础，把集群控制拆成可以独立验证的模块。

## 当前落地顺序

1. **FleetRegistry**：记录在线车辆、能力和健康摘要，不再让界面或任务模块自行猜测车辆状态。
2. **CommandTransaction**：统一批量命令的事务 ID、目标集合、超时、重试和部分成功结果。
3. **SwarmMissionOrchestrator**：任务创建、启动、暂停、恢复、取消、重分配和进度。
4. **FormationPlanner**：编队、分裂、合并和安全间距规划。
5. **FaultToleranceManager**：Leader 候选、掉线、低电量退出和降级建议。

以上五个模块已经加入 qmake 工程并注册到 `Merivus 1.0` QML 模块。`SwarmController`
现在为起飞、降落、返航、直接指点和编队阶段创建事务，并接收可用的
`Vehicle::mavCommandResult` 或飞行模式遥测；临时 Mission 队列仍保持“上传/启动已发起、
最终执行待核实”的语义。

标准 Mission 启动、继续和多机启动也统一经过 `SwarmController::executeStartMission()`：目标集合与任务项快照由 QML 冻结，C++ 在逐机发送前复核链路、MissionManager 缓存、任务项数量/当前索引和健康证据，并输出 MissionItem JSON 的 `snapshotFingerprint`；随后等待 `MAV_CMD_MISSION_START` ACK 或 Mission 模式遥测。ACK、模式和执行完成仍是不同证据，指纹当前用于审计，不是跨确认时刻的版本锁。

基础 FleetRegistry 先接入 QGroundControl 当前可可靠读取的车辆发现、连接、固件、
机型和有效全球坐标或本地位置 x/y/z 证据；电池、估计器、任务进度等字段在对应 MAVLink/Vehicle 信号接入后再
纳入健康评分，不能把“已注册”误认为“飞行安全”。

## 复用边界

- QGroundControl 的 Vehicle、MAVLink、Mission 和日志基础继续复用。
- MAVLink Router 适合作为独立链路进程，不直接替换 QGC 通信核心。
- MAVSDK 适合作为外部任务服务或自动化测试适配层，不在 QML 中重复建立第二套通信模型。
- Crazyswarm2、drone-swarm、SwarmPilot 和 swarm-autonomy 只提供架构/算法参考；
  集群核心按 MERIVUS 的 MAVLink 协议自行实现。
- 未经许可证确认的代码不复制进产品；每个第三方依赖都必须进入许可证清单和 NOTICE。

## 事务状态约定

集群操作的“发送成功”不等于“飞行器完成”。CommandTransaction 应至少区分：

- `Pending`：事务已创建，尚未发送；
- `Running`：正在发送或等待目标结果；
- `PartiallySucceeded`：部分目标成功；
- `Succeeded`：所有目标成功；
- `TimedOut`：仍有目标未在期限内返回；
- `Failed`：没有目标成功或事务被拒绝；
- `Cancelled`：操作员取消。

`CommandTransaction::resultSummary()` 同时输出 `terminal` 布尔字段；它必须与
`status` 的终态集合保持一致。`SwarmController` 的延迟 Mission 回调使用这个字段
阻止失败/超时事务继续推进；普通命令和临时 Mission 在实际发送前还会核对本车的
`vehicleResults[].state` 是否仍为 `pending/running`，因此多机事务中单车被 direct goto
替代时不会继续发送该车的排队回调。工程师不得只读取 `status` 而绕过整体/逐机终态门禁。

事务构造不会静默删除目标：任一目标 ID 非法、重复、字符串别名或超出 1–255 协议范围，
`targetInputValid` 会为 `false`，`start()` 在第一次 dispatch 前失败；`resultSummary()` 保留
原始 `targetVehicleIds`，并同时给出 `requestedTargetCount` 与有效内部目标数，防止调用方把被
截短的集合误当作整组目标。只有上游显式提供的真实整数/可接受的 QML 整数 double 才能进入
事务状态和 ACK 回填。

事务重试只允许发生在至少一次真实 attempt 已经启动、命令和目标集合有效、且终态仍有
失败/超时目标时；创建阶段的参数校验失败不能被 `beginRetry()` 重新变成 `Running`。
超时回调会重新核对当前 timeout policy 和 elapsed deadline，避免修改/停用定时器后残留的
队列事件误把活动事务标记为超时。

QML/操作员可通过 `SwarmController::cancelTransaction()` 取消仍在运行的事务；取消会让排队回调立即失去逐机发送资格，并让临时 Mission 进入已有清理路径。它不宣称已经发出的机载动作被逆转，仍需观察 ACK、任务清理和遥测结果。

临时 Mission 的替换不是同步操作。由于 `MissionManager::removeAllComplete` 不携带任务代号，
请求替换时先取消旧任务并等待清理回调；清理完成前新上传会被拒绝，操作员需在界面提示后重试，
以避免旧回调清除或完成新任务。

命令入口同时执行一层只读 FleetRegistry health preflight：起飞、Goto 和临时 Mission
在事务筛选、延迟发送以及 Mission 上传完成后再次阻断已经确认的离线、critical、故障、
临界电量或极差链路。批量降落/返航保留为安全逃生路径：它们仍检查实时 Vehicle/PX4
链路、模式和飞行前置条件，但 FleetRegistry 的 advisory block 只作为
`healthGate.warningVehicleIds` 返回，不抑制可达的安全动作。没有 Registry/车辆快照或
`discoveryState=invalid` 时普通命令 fail-closed；快照内部的可选字段缺失仍保持 unknown，
不会被当作健康。结果中的 `healthGate.reasons` 必须交给界面/审计层解释。该层不替代
Vehicle 自身前置条件或 PX4 机载故障保护。

编队阶段的 ACK 回调绑定本地 phase generation，旧会话中已经排队的 ACK 不会推进新会话；
`MAV_RESULT_IN_PROGRESS` 只表示仍在等待，不会被当作失败。最终 ACK/拒绝由当前阶段事务
记录，真实 PX4 自定义命令的 ACK 语义仍需工程师在 Mock/SITL 中确认。

## 交付原则

当前源码批次只改变地面站的状态建模和命令编排，不改变 PX4 姿态控制、混控或电机输出。
真机烧录和飞行验证由工程师在完成 SITL、日志回放和故障注入后执行。

## 分级自主与平台适配声明

`FleetAutonomyPolicy` 位于 `FleetRolePolicy`、`FleetExtensionRegistry` 和 AI 本地策略
（`AiSchemaValidator`/`AiCommandPolicy`）旁边，提供只读的人工、辅助、监督、自动化阶段
策略，以及 PX4、ArduPilot 和伴随插件的能力声明。它只返回角色范围、人工/双人审批、
签名检查和沙箱检查结果；适配器默认为未签名、未启用且沙箱待接入。该层不加载扩展、不
调用 Vehicle/MAVLink，也不释放飞行命令，阶段结果必须继续经过角色策略和车辆安全门禁。

# MERIVUS 集群地面站开发基线

本文档记录 MERIVUS GroundStation 集群能力的第一阶段实现边界。目标是保留
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

第一阶段的 FleetRegistry 先接入 QGroundControl 当前可可靠读取的车辆发现、连接、固件、
机型和位置有效性字段；电池、估计器、任务进度等字段在对应 MAVLink/Vehicle 信号接入后再
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

## 交付原则

第一阶段只改变地面站的状态建模和命令编排，不改变 PX4 姿态控制、混控或电机输出。
真机烧录和飞行验证由工程师在完成 SITL、日志回放和故障注入后执行。

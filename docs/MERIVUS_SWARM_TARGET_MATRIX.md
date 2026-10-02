# MERIVUS 集群目标落地矩阵

本文件对应“目标全部加入”的当前地面站实现边界。它把产品目标、代码入口和验收缺口放在同一张表里，避免把界面存在误认为飞行能力已经验证。

| 目标 | 当前代码入口 | 当前状态 |
| --- | --- | --- |
| 多机发现、能力档案、基础健康摘要 | `FleetRegistry`、`VehicleCapability`、`SwarmController::_refreshFleetRegistry` | 已接入 QGC 车辆发现、连接、固件、机型、位置有效性、电池平均值、健康检查摘要和 RSSI 估算链路质量；原始 EKF/完整链路质量仍需工程师按固件版本校验 |
| 批量指令的事务 ID、超时、重试、部分成功 | `CommandTransaction`、`SwarmController` | 起飞、降落、返航、直接指点和编队阶段已接入 ACK/遥测事务；临时 Mission 队列仍是待核实语义 |
| 任务编排、暂停、恢复、取消、重分配 | `SwarmMissionOrchestrator` | 已加入；故障策略会暂停任务或更新任务成员，不会自动发送飞行命令 |
| 意图式任务预览与人工审批 | `FleetIntentTask`、`FleetCapabilityMatcher` | 已加入方案状态机、能力/健康/电量/链路筛选、主组/备用组和角色建议；审批仍不释放飞行命令 |
| 任务级风险雷达 | `FleetRiskRadar` | 已加入意图关键词、调用方风险信号、分级解释和人工确认标记；禁飞区/航线/能源模型仍需接入工程数据 |
| 异构无人机能力匹配 | `FleetCapabilityMatcher`、`FleetTaskTemplateRegistry` | 已加入能力标签和行业模板目录；尚未接入完整机载能力发现和续航预测 |
| 编队规划、分裂、合并、最小间距检查 | `FormationPlanner` | 已加入，当前输出为本地 ENU 偏移；全球坐标转换和机载跟随协议仍需工程师确认 |
| 掉线、低电量、Leader 候选和降级建议 | `FaultToleranceManager` | 已加入；只生成 `reassign/split/return/hold` 建议，不替代 PX4 安全逻辑 |
| 任务交接和多人权限 | `MissionHandoffManager`、`FleetRolePolicy` | 已加入内存态交接、角色权限、二次审批标志；尚无持久化账号/服务端鉴权 |
| 黑匣子事件、时间线回放、任务预演 | `FleetEventBlackBox`、`FleetTimelineReplay`、`FleetMissionSimulator` | 已加入内存态记录、逐步回放和描述性故障注入；尚无磁盘/数据库持久化 |
| 行业任务模板、扩展目录 | `FleetTaskTemplateRegistry`、`FleetExtensionRegistry` | 已加入巡逻、巡检、救援、仓储模板和元数据目录；扩展默认不加载任意二进制 |
| 编队 PREPARE → COMMIT → RELEASE → ABORT | `SwarmController`、`GuidedActionsController.qml` | 已加入现有入口，成员失败/超时会整组回滚；当前协议仍固定 UAV-1 为 Leader、最大 6 机 |
| 地图、指挥中心共享状态 | `FlyViewMap.qml`、`CommandCenterOverlay.qml` | 已改为共享同一 `SwarmController`；多机列表开关已重新打开 |
| AI/自动化建议 | 既有 `Ai*` 模块 | 保持建议与人工执行隔离；本批次没有扩大 AI 的 MAVLink 执行权限 |
| 日志、回放、SITL/故障注入 | `docs/development/SITL_MULTI_VEHICLE_MISSION_TEST.md` 等 | 作为工程师接手验收项；当前环境没有 Qt 工具链，未执行构建或仿真 |

## 工程师接手顺序

1. 在受支持的 Qt 5.15/MSVC 环境编译 `custom/custom.pri`，先修复 moc、QML 和链接错误。
2. 补齐 `Vehicle` 的电池、链路、估计器、任务进度信号，并把未知值保持为未知，不能当作健康。
3. 完成普通批量指令与 `mavCommandResult` 的逐机 ACK 关联；没有 ACK 时只显示“已发送/待核实”，不显示“已完成”。
4. 使用 Mock、PX4 SITL、日志回放和故障注入验证超时、拒绝、部分成功、重连、Leader 替换和编队回滚。
5. 通过拆桨/系留/限速等现场安全流程后，才允许进入真机飞行验收；本代码包本身不构成飞行安全证明。

## 明确未承诺的能力

- 本地地面站不能保证断链后继续控制飞机，最终故障保护必须由 PX4 机载逻辑承担。
- “悬停不漂、断桨悬停、碰撞后自稳定、复杂避障”等飞行性能不能仅由地面站代码证明。
- 当前集群控制协议不是任意规模通用协议；要支持超过 6 机或动态网络，需要单独完成协议和带宽设计。

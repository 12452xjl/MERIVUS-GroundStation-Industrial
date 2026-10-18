# MERIVUS 当前状态

更新时间：2026-10-06

Fleet OS 目标的逐项源码 groundwork、当前缺口和验收边界以
[`MERIVUS_FLEET_OS_ROADMAP.md`](../MERIVUS_FLEET_OS_ROADMAP.md) 与
[`MERIVUS_SWARM_TARGET_MATRIX.md`](../MERIVUS_SWARM_TARGET_MATRIX.md) 为准；本页
是系统级总览，不替代这两份逐项契约。

当前版本：`0.1.0-dev.1`

版本性质：阶段性初版 / 开发测试版

## 当前结论

MERIVUS 已从早期界面原型进入“可构建、可本地联调、具备明确安全边界”的阶段性初版。仓库当前包含 QGroundControl 定制地面站、Python Local Agent、QGC C++ Agent 客户端和监管器、本地模型 Provider、结构化建议策略、自动化测试与完整开发文档。

本版本仍不属于生产版本。AI 链路只做问答、解释、状态展示和结构化建议，所有飞行动作类 proposal 均保持 `executable=false`，不会进入 Vehicle、MAVLink、PX4 或 Swarm 执行链路。

## 已实现能力

### 地面站与调度界面

- MERIVUS 品牌化 QGC Custom Build。
- 自定义主窗口、工具栏、地图、飞行控制和环境姿态视频区域。
- 多机列表、焦点飞行器、链路、遥测、电池、GPS、ESC 和任务信息展示。
- 飞行视图与设置页面共享的全局 AI 悬浮入口。
- MERIVUS 集群控制第一阶段模块：`FleetRegistry`、`CommandTransaction`、
  `SwarmMissionOrchestrator`、`FormationPlanner` 和 `FaultToleranceManager`。
- Fleet OS 产品层基础模块：`FleetIntentTask`、`FleetCapabilityDiscovery`、
  `FleetCapabilityMatcher`、`FleetHealthCenter`、`FleetMissionCompiler`、
  `FleetRoleAllocator`、`FleetEnergyScheduler`、`FleetAirspaceConflictChecker`、
  `FleetMissionRiskEvaluator`、`FleetLinkDegradationPolicy`、
  `FleetMissionContinuityPlanner`、`FleetEventRuleEngine`、`FleetEventBlackBox`、
  `FleetTimelineReplay`、`FleetMissionSimulator`、`FleetMissionDigitalTwin`、
  `FleetMissionEvidenceChain`、`FleetMissionReportBuilder`、`MissionHandoffManager`,
  `FleetMaintenancePassport`、`FleetOperationalMemory`、`FleetRolePolicy`、
  `FleetTaskTemplateRegistry`、`FleetExtensionRegistry`、只读
  `FleetAutonomyPolicy` 和 `FleetMissionReviewCoordinator`。
  这些模块只生成方案、风险、审计、维护和权限结果，不绕过人工确认或直接释放飞行命令。
- `FleetMissionReviewCoordinator` 将角色分配、任务编译、能量、空域、风险和报告组成一次
  可追溯的只读审阅；GuidedActionsController 的意图预览只记录该结果，缺失证据保持
  `needsReview`，不连接 `SwarmController`。
- `FleetAutonomyPolicy` 固定描述人工、辅助、监督和自动化阶段，以及 PX4/ArduPilot/伴随插件的
  能力、权限、签名和沙箱占位；当前签名状态为未验证、沙箱为待接入，所有结果保持
  `flightCommandReleased=false`。
- 人工批量起飞/降落/返航/直接指点与编队阶段使用事务 ID；完成状态分别由 MAVLink ACK
  或飞行模式遥测确认，不能把“已发送”当作“已完成”。
- 地图单机或框选 Goto（含普通右键指点）现在冻结确认时的目标 ID 集合与坐标，与批量/临时 Goto 共用
  FleetHealthCenter 双阶段门禁和 `SwarmController::executeGoto()` 事务路径；掉线/低电量回调只生成带任务
  revision 的连续性建议，不自动暂停或重分配任务。
- 连续性任务编排器只在 `GuidedActionsController` 内部保持可变对象；其他 QML 只能读取任务摘要。控制器提供
  `applyContinuitySuggestion(suggestionId, true)` 作为人工复核接缝，重新解析当前建议并核对来源 revision 后才允许
  成员重分配，且该入口未接入面板自动动作或飞行命令释放。
- 指挥中心以只读值快照显示模板、扩展和分级自主目录；事件规则支持版本化本地快照，导入不恢复冷却状态。
- 地图、指挥中心和 Guided Actions 共享同一个 `SwarmController` 实例；多机列表保持可见。

### AI Assistant

- 可缩放 AI 聊天面板，用户尺寸通过 Qt Settings 保存。
- 面板边缘独立悬浮和拖拽反馈。
- 用户与助手消息气泡、消息数量管理和清空历史。
- 本地规则模式、Agent 模式、Provider 配置和运行状态展示。
- 参数解释、基础问答和结构化建议展示。
- AI proposal 与本地规则模式待确认建议均可在操作员显式点击后生成 Fleet OS 只读审阅草稿；申请/批准仍受同一 `taskId/planId` 与 `needsReview=false` 约束，不上传任务或释放命令。
- QGC C++ `AiAgentClient` 负责 HTTP 通信，QML 不直接承担 Agent 网络请求。

### Local Agent

- Python FastAPI 服务与稳定 HTTP 契约。
- Mock Provider 和本机 Ollama Provider。
- 模型输出规范化、QA/指令分离、few-shot 和评估样例。
- QGC `AiServiceSupervisor` 负责 Agent 生命周期、健康检查和重启。
- 开发环境可自动发现仓库 Agent、Python 启动器或已打包 Agent。
- PyInstaller Windows 打包与 staging 集成脚本。

### 安全与质量

- `ActionProposal` C++ 数据结构和 JSON Schema。
- `AiSchemaValidator`、`AiCommandPolicy` 与审计事件。
- 未知命令、越权字段、原始 MAVLink 和参数写入请求由本地策略拒绝。
- Agent 测试、QGC AI 策略测试、模型评估和 GUI 烟测清单。
- 架构、构建、硬件、安全、风险和仓库维护文档。

### FTC 与植保方案

- FTC v2 遥测后端、状态面板、协议版本与过期通知已接入，契约见 [FTC 遥测](FTC_TELEMETRY_CONTRACT.md)。
- FTC 的局部测试和历史联调证据见 [测试矩阵](../testing/TEST_MATRIX.md)，不代表飞控主动控制或实飞已验收。
- [植保区域覆盖 ADR](../adr/0001-plant-protection-area-coverage.md) 是待实现、待验证的设计基线，不能列为已实现功能。

## 当前安全边界

- `flight_execution_enabled=false`。
- AI 不直接发送 MAVLink，不修改 PX4 参数。
- AI 不调用 Vehicle、SwarmController 或其他飞行动作入口。
- proposal 只用于显示和预览，不代表执行结果。
- Fleet OS 审阅草稿只生成风险/健康/能量/空域/报告快照，不能视为飞行批准。
- 真实飞行功能必须经过独立确认框架、审计、SITL 回归和人工安全评审。
- 真实凭据、生产坐标、飞行日志、模型文件和构建产物不得进入仓库。

## 验证状态

历史阶段记录（不代表当前 HEAD 已重新完成以下全部验证）：

- QML 语法与资源生成检查。
- Agent 自动化测试和 QGC AI 本地策略测试。
- Windows Qt 5.15.2 / MSVC Release 构建验证。
- Agent 打包与本机 HTTP 冒烟验证。

尚未完成：

- 真实无人机端到端测试。
- 真实 4G/RTK 多机长期链路测试。
- 生产网络安全、用户鉴权和权限系统。
- 安装包签名、升级机制与发布流水线验收。
- MCP、云端设备网关、数据库、Media Service 和 GIS Safety Service。
- AI 飞行动作确认器与命令执行器。

## 版本与分支基线

- 阶段版本：`0.1.0-dev.1`。
- 主线分支：`main`。
- 既有阶段标签：`v0.1.0-dev.1`；后续 FTC 与文档提交未形成新的正式发行版本。
- 后续开发从 `main` 创建短生命周期功能分支，完成验证后通过 PR 或受控集成回到主线。
- 历史功能分支在确认内容已进入主线并建立版本标签后清理。

## 下一阶段建议

下一阶段建议标记为 `0.1.0-dev.2`，优先完成：

- 干净环境下的可重复 Windows 构建。
- PX4 SITL 多机回归与异常链路测试。
- AI 会话持久化和消息记录管理策略。
- PX4/APM 参数说明数据源、缓存和离线回退。
- 低风险 UI 动作的受控确认框架与完整审计。
- Provider 配置凭据隔离和生产部署设计。

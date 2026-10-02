# MERIVUS 工程交接入口

MERIVUS 是基于 QGroundControl Custom Build 的多无人机调度地面站。当前仍是开发测试阶段，不是生产版本；不得把界面存在、代码存在或历史测试结果表述为当前版本已经验证。

## 接手顺序

1. 阅读仓库根目录 `AGENTS.md`（若本机提供）与 [`docs/INDEX.md`](../INDEX.md)。
2. 执行 `git status --short --branch`、`git log --oneline --decorate -15` 和 `git remote -v`，以当前代码、配置和 Git 历史为事实基线。
3. 阅读与任务直接相关的架构、开发或硬件文档；不要把整套文档当作必须逐篇阅读的交接快照。
4. 优先做静态检查和覆盖本次改动的局部验证；只有在环境具备或风险要求时执行完整构建、SITL 或集成测试。

## 当前安全边界

- AI 链路只回答、解释和展示 `ActionProposal`，不连接 Vehicle、MAVLink、PX4 或 Swarm 执行入口。
- 人工多机控制入口与 AI 建议链路必须分开审查。
- 不自动连接真实飞机、生产服务或云模型，不自动修改 PX4/RTK 参数。
- “已安排”“已上传”“收到 ACK”和“动作完成”是不同状态，不得混用。

## 单一真相源

- 当前能力与限制：[`CURRENT_STATE.md`](../architecture/CURRENT_STATE.md)
- 接口与安全契约：[`INTERFACE_CONTRACTS.md`](../architecture/INTERFACE_CONTRACTS.md)、[`SAFETY_BOUNDARIES.md`](../architecture/SAFETY_BOUNDARIES.md)
- 构建与 Agent 开发：[`BUILD_WINDOWS.md`](../development/BUILD_WINDOWS.md)、[`AGENT_DEVELOPMENT.md`](../development/AGENT_DEVELOPMENT.md)
- 可重复验证入口：[`docs/development`](../development/)
- 版本历史：[`CHANGELOG.md`](../releases/CHANGELOG.md) 与 Git/PR 历史

## 本批次交接入口

- 集群地面站第一阶段工程师交接：[`MERIVUS_SWARM_ENGINEER_HANDOFF.md`](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- 本批次源码文件与静态检查清单：[`MERIVUS_SWARM_PATCH_MANIFEST.md`](MERIVUS_SWARM_PATCH_MANIFEST.md)
- 集群目标与当前缺口：[`MERIVUS_SWARM_TARGET_MATRIX.md`](../MERIVUS_SWARM_TARGET_MATRIX.md)
- Fleet OS 协同、交接、行业模板和插件注册：[`MERIVUS_FLEET_OS_COLLABORATION.md`](../MERIVUS_FLEET_OS_COLLABORATION.md)
- Fleet OS 意图与风险方案：[`FleetIntentRisk.md`](../../custom/src/Swarm/FleetIntentRisk.md)
- Fleet OS 黑匣子、回放与预演：[`fleet-event-replay.md`](../fleet-event-replay.md)

交接文档只说明源码接手、编译、SITL/故障注入和放行边界，不替代工程师的真实硬件测试报告。

阶段性排障记录、旧环境说明、账号迁移清单、源码全文副本和二进制说明书不再作为仓库长期文档；需要时从 Git 历史或对应任务记录恢复。

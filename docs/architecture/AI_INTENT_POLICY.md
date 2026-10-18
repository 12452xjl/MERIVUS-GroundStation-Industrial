# MERIVUS AI 意图策略

本阶段在 QGC C++ 层建立本地 AI 意图策略。目标是让 Agent 可以返回结构化 `proposal`，但 QGC 必须先做本地 schema 校验、命令白名单、参数边界、风险重算和审计记录；QML 只显示结果，不执行任何飞行动作。

## 边界

- `AiAgentClient` 负责 HTTP、JSON 解析、proposal 本地校验和策略判定。
- `ActionProposal` 是 QGC 内部显示用的动作建议模型。
- `AiSchemaValidator` 只判断 Agent 返回结构是否安全、大小是否受控。
- `AiCommandPolicy` 只做本地风险和策略决策，不调用 Vehicle、Mission、MAVLink、Swarm 或 PX4。
- `FleetAutonomyPolicy` 只声明 AI 建议所处的人工/辅助/监督/自动化阶段，以及适配器能力、权限、签名和沙箱占位；它不替代 `AiCommandPolicy`，也不把 proposal 变成执行授权。
- `AiAuditEvent` 只记录脱敏审计摘要，不记录 Token、API Key、完整用户消息、RTSP 凭据或服务端账号。
- QML 只渲染建议卡片，不提供执行按钮；当操作员显式点击“生成舰队审阅草稿”时，允许把已校验 proposal 送入 Fleet OS 的只读 `prepareFleetIntent()` 审阅计算，但不请求审批、不上传任务，也不释放飞行命令。

## ActionProposal 字段

固定字段：

- `requestId`
- `command`
- `arguments`
- `summary`
- `source`
- `agentProvider`
- `agentModel`
- `validationStatus`
- `policyDecision`
- `localRisk`
- `requiresConfirmation`
- `reason`
- `executable`
- `hasProposal`
- `argumentsSummary`

`executable` 当前恒为 `false`。即便 Agent 伪造 `risk`、`requires_confirmation`、`executable` 或 `executed`，QGC 也会忽略并重新计算本地结果。

## Schema 规则

`proposal` 可以为 `null`；这代表没有结构化建议，不是错误。

当 `proposal` 为对象时：

- `command` 必须是非空字符串，最长 128 字符。
- `arguments` 必须是对象，最多 24 个 key。
- `summary` 必须是字符串，最长 1000 字符。
- 参数 key 最长 64 字符。
- 参数字符串最长 512 字符。
- JSON 深度最多 5 层。
- 拒绝参数数组，避免原始 MAVLink 参数数组绕过结构化契约。
- 拒绝 `execute`、`send`、`run`、`shell`、`script`、`QProcess`、PX4 批处理、Token/API Key/密码等危险字段或内容。
- 未知普通字段不会触发执行；未知危险字段会被安全拒绝。

## 命令白名单

只读：

- `vehicle.query_status`
- `vehicle.query_battery`
- `vehicle.query_position`
- `vehicle.query_rtk`
- `log.explain_error`

UI-only：

- `ui.select_vehicle`
- `ui.open_page`
- `map.focus_coordinate`

任务预览：

- `mission.create_draft`
- `mission.analyze`

高风险预览：

- `vehicle.arm`
- `vehicle.force_arm`
- `vehicle.takeoff`
- `vehicle.land`
- `vehicle.rtl`
- `vehicle.pause`
- `vehicle.goto`
- `mission.upload`
- `mission.start`

强制拒绝：

- `param.write`
- `mavlink.send_raw`

未知命令一律 `Deny`，风险为 `Critical`。

## 参数校验

- `vehicle_id` 如存在必须为正整数；`ui.select_vehicle` 必须提供正整数 `vehicle_id`。
- `ui.open_page` 必须提供短 `page` 字符串。
- `map.focus_coordinate` 必须提供合法 WGS84 `latitude` 和 `longitude`。
- `vehicle.goto` 必须提供合法 `latitude`、`longitude` 和 0 到 120 米的 `altitude_m`。
- `vehicle.takeoff` 如提供 `altitude_m`，必须大于 0 且不超过 120 米。
- `param.write` 即使参数形态合法也被本地策略强制拒绝。
- `mavlink.send_raw` 不接受原始参数数组，并被本地策略强制拒绝。

## 风险和策略

- `AllowReadOnly`：只读查询，风险 `Low` 或 `Informational`，不需要确认，不执行。
- `AllowUiOnly`：低风险 UI 建议，风险 `Low`，不需要确认，本阶段仍不自动操作 UI。
- `PreviewOnly`：任务草稿或飞行动作预览，风险 `Medium` 或 `High`，只显示。
- `Deny`：未知、非法、原始 MAVLink、参数写入或危险结构，风险 `Critical`。
- `RequiresConfirmation` 枚举预留给后续确认框架；本阶段不进入执行链路。

## QML 展示

AI 面板收到本地判定后的 proposal 后，只显示：动作、参数摘要、来源、provider/model、schema 状态、本地风险、策略决策、是否需要确认、当前状态、原因和 request_id。

对于 `vehicle.*` 或 `mission.*` 且未被本地策略拒绝的 proposal，AI 面板可以在操作员明确点击后生成一份 Fleet OS 审阅草稿；本地规则模式的起飞/降落/返航/暂停待确认建议也提供同样的显式“生成审阅”入口。该入口只携带安全的目标/位置/高度/时长字段，使用现有健康、能力、能量、空域和风险快照；它不会调用 `requestFleetIntentApproval()`、`approveFleetIntent()`，也不会连接 Vehicle、MAVLink、MissionManager 或 PX4。

FleetIntentTask 的申请/批准 API 还必须绑定同一轮 `taskId/planId` 的协调器结果，并要求 `reviewStatus=reviewable` 且 `needsReview=false`；否则仅记录 `intent.approval_blocked`，避免“界面显示待复核但意图已批准”的状态错觉。控制器同时监听协调器的 `reviewChanged`：外部重新审阅（即使 `planId` 未变化）会立即使审批凭据失效，必须重新经过 `prepareFleetIntent()`。

当前没有飞行动作执行按钮，也不会调用：

- `guidedModeTakeoff`
- `guidedModeLand`
- `guidedModeRTL`
- `pauseVehicle`
- `sendCommand`
- `startMission`
- `executeGoto`
- `executeQueuedGoto`

AI proposal 若需要映射到 Fleet OS 阶段，默认按 `assisted` 读取
`FleetAutonomyPolicy.evaluate()`，再由 `FleetRolePolicy` 和车辆安全门禁复核。
监督/自动化阶段需要外部签名适配器和沙箱审批；当前 `unverified`/`*_pending`
状态会保持审阅结果，不能释放命令。

## 审计

审计日志分类为 `merivus.ai.policy`，记录 request id、session id、command、validation、decision、risk、requires confirmation、provider/model 和截断后的 reason。

审计不得记录：

- Token
- API Key
- 完整用户消息
- RTSP 凭据
- 服务端账号密码
- 原始大段日志

## 已知限制

- 本阶段没有真实模型 Provider。
- 本阶段没有命令执行器。
- 本阶段没有确认弹窗。
- UI-only 命令仍只显示，不自动跳转或选机。
- 高风险命令只预览，不执行。
- 下一阶段建议进入确认/执行框架或继续安全策略细化；不建议直接进入真实模型 Provider。

## Release 构建稳定性记录

本阶段追加修复了 Qt 5.15.2 / MSVC Release 构建中的资源阶段假性崩溃。定位结果显示，最早失败点不是 `rcc`、`qmlcachegen` 或 AI 面板 QRC，而是生成 Makefile 后第一条 `cl.exe` 编译命令携带了过长的 `CXXFLAGS/CFLAGS + INCPATH`。`jom` 在该场景下返回 `-1073740791`，容易把最后打印的资源或 qmlcache 命令误判为根因；使用 `nmake` 可明确暴露 MSVC 命令行过长问题。

修复只作用于仓库构建脚本和生成目录中的 Makefile：`tools/dev/build-merivus.ps1` 在 qmake 后生成 MSVC 响应文件，并将编译规则中的公共 flags/include path 改为 `@merivus_cl_*_common.rsp`。Quick Compiler 没有被全局关闭，Qt 安装目录没有修改，AI 面板仍通过独立 `merivus_ai_panel.qrc` 进入普通资源编译，未进入 qmlcache 映射。

安全策略边界保持不变：`ActionProposal`、`AiSchemaValidator`、`AiCommandPolicy` 和 `AiAuditEvent` 未降低约束；高风险起飞类 proposal 仍只显示为未执行建议，`executable=false`，不调用 Vehicle、MAVLink、Swarm 或 PX4 执行入口，也不接入真实模型。

## 本地模型 Provider 输出处理

`feat/agent-model-providers` 后，真实本地模型只能通过 Python Agent 返回 `reply` 和可选 `proposal`。OllamaProvider 会拒绝模型输出中的 `executed`、`executable`、`risk`、`localRisk`、`policyDecision`、`requiresConfirmation`、`mavlink`、`shell`、`script`、`px4_parameters` 等越权字段。

这不是最终安全边界。QGC C++ 的 `AiSchemaValidator` 与 `AiCommandPolicy` 仍会重新校验所有 proposal，并继续保证当前阶段 `executable=false`。未知命令、`param.write`、`mavlink.send_raw` 和危险结构仍按本地策略拒绝。
## 本地模型输出稳定性补充

`feat/ai-model-stability` 增加的 `agent/app/proposal_normalizer.py` 只属于 Agent 侧输出整理层，不改变 QGC C++ 的安全职责。Normalizer 可以把常见别名整理为标准 command/arguments，也可以删除模型越权字段；但它不会授予执行能力，不会计算最终风险，也不会绕过 `AiSchemaValidator` 或 `AiCommandPolicy`。

安全回归要求保持不变：

- `executed`、`executable`、`risk`、`localRisk`、`policyDecision`、`requiresConfirmation` 等模型字段不能成为 QGC 信任来源。
- `param.write`、`mavlink.send_raw` 和未知 command 仍由本地策略拒绝。
- 高风险飞行动作仍只能显示为未执行建议。
- 当前阶段没有 `AiCommandExecutor`，没有确认弹窗，没有真实 Vehicle/MAVLink/PX4/Swarm 执行入口。

## few-shot / recovery 安全补充

`feat/ai-model-fewshot-eval` 增加的 few-shot 与 normalizer recovery 只提高本地模型输出稳定性，不改变本地策略权限。

- recovery 只能补全明确模板化文本中的 `reply/proposal`，不能执行。
- recovery 不生成 `param.write` 或 `mavlink.send_raw`，也不默认 `vehicle_id`、起飞高度或地名坐标。
- recovery 后的 proposal 与模型直接生成的 proposal 完全一样，仍必须经过 `AiSchemaValidator` 和 `AiCommandPolicy`。
- 高风险动作继续由 QGC 标记为 `PreviewOnly` / `executable=false`。
- `param.write`、`mavlink.send_raw`、未知 command、危险字段和伪造本地策略字段继续由 C++ 本地策略拒绝或忽略。

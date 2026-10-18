# MERIVUS 接口契约草案

本文档定义阶段 0 以来的接口边界。`feat/local-agent-http` 已实现独立 Python Mock Agent 的本机 HTTP 契约；`feat/qgc-agent-client` 已将 QGC AI 面板默认发送路径迁移到 C++ `AiAgentClient`；`feat/agent-supervisor` 增加 `AiServiceSupervisor` 管理本机 Agent 生命周期。

## QGC 到 Agent

### Endpoint

```text
GET  http://127.0.0.1:8765/health
GET  http://127.0.0.1:8765/merivus/info
POST http://127.0.0.1:8765/merivus/agent
Content-Type: application/json
X-Merivus-Token: <仅当QGC Supervisor启动Agent时携带>
```

### 请求 Schema

```json
{
  "request_id": "uuid",
  "session_id": "session-id",
  "message": "用户输入",
  "context": {
    "active_vehicle_id": 1,
    "connected": true,
    "armed": false,
    "flight_mode": "Position",
    "battery_percent": 80,
    "gps_valid": true,
    "rtk_fix_type": "none"
  },
  "allowed_capabilities": [
    "vehicle.query_status",
    "mission.analyze"
  ]
}
```

### 响应 Schema

```json
{
  "request_id": "uuid",
  "reply": "文本回答",
  "proposal": {
    "command": "vehicle.query_status",
    "arguments": {
      "vehicle_id": 1
    },
    "summary": "查询一号无人机状态"
  },
  "provider": "mock",
  "model": "mock-v1",
  "status": "ok"
}
```

### 契约规则

- `proposal` 可以为 `null`。
- 本阶段 Agent 响应不包含 `risk` 和 `requires_confirmation`；QGC 后续必须本地重算风险和确认策略。
- `command` 必须来自固定枚举。
- 未知字段默认拒绝，JSON Schema 中尽量设置 `additionalProperties: false`。
- 不允许返回原始 MAVLink 参数数组作为通用执行接口。
- Agent 超时、离线、返回无效 JSON 时，QGC 只显示错误，不影响飞控主功能。
- `proposal` 只是建议，不包含 `executed=true`、MAVLink 消息 ID、Shell 命令或 PX4 参数写入。
- Supervisor 启动的 Agent 可要求 `X-Merivus-Token`；Token 只存在于当前进程内存和子进程环境，不进入 QML 展示、配置文件、日志或 Git。
- health/info 继续不要求 Token，便于启动前检查和外部开发 Agent 兼容。

## 本机 Agent 已实现接口

`feat/local-agent-http` 新增独立 `agent/` 目录，当前只支持 `mock` Provider：

- `GET /health`：返回服务状态，不调用 Provider、模型或外部网络。
- `GET /merivus/info`：返回 `external_network_enabled=false`、`flight_execution_enabled=false`、`provider=mock`、`model=mock-v1`。
- `POST /merivus/agent`：接收 `request_id/session_id/message/context/allowed_capabilities`，返回 `reply` 和可选 `proposal`。

配置默认值：

- `MERIVUS_AGENT_HOST=127.0.0.1`
- `MERIVUS_AGENT_PORT=8765`
- `MERIVUS_AGENT_PROVIDER=mock`
- `MERIVUS_AGENT_LOG_LEVEL=INFO`
- `MERIVUS_AGENT_MAX_MESSAGE_LENGTH=8000`
- `MERIVUS_LOCAL_TOKEN`：可选，由 QGC Supervisor 启动子进程时生成并传入。

本阶段不接 OpenAI、DeepSeek、Gemini、Ollama、MCP、GIS 真实服务、云服务器、数据库、MAVLink、Vehicle 或 SwarmController。

## 第一批允许 command

低风险或只读：

- `vehicle.query_status`
- `vehicle.query_battery`
- `vehicle.query_position`
- `vehicle.query_rtk`
- `log.explain_error`

UI 操作：

- `ui.select_vehicle`
- `ui.open_page`
- `map.focus_coordinate`

任务草稿与分析：

- `mission.create_draft`
- `mission.analyze`

## 第一版禁止 command

- `vehicle.arm`
- `vehicle.force_arm`
- `vehicle.takeoff`
- `vehicle.land`
- `vehicle.rtl`
- `vehicle.pause`
- `vehicle.goto`
- `mission.upload`
- `mission.start`
- `param.write`
- `mavlink.send_raw`

说明：这些可以作为未来高风险能力预留，但阶段 0/第一版 AI 不应直接实现执行。

在 `feat/local-agent-http` 中，高风险 command 可作为 `proposal.command` 被 Mock Provider 识别，但前提是它出现在 `allowed_capabilities` 中；即便出现，也只返回结构化建议，不执行。

## QGC 本地策略接口

建议后续在 C++ 中实现：

- `AiAgentClient`
  - 负责 HTTP 请求、超时、取消、错误状态、JSON 解析。
- `ActionProposal`
  - 固定字段：`command`、`arguments`、`summary`、`source`、`request_id`。
- `SchemaValidator`
  - 负责请求/响应 schema 校验。
- `AiCommandPolicy`
  - 根据本地规则重算风险和是否需要确认。
- `AiCommandExecutor`
  - 第一版仅 mock 或只读/UI 操作。
  - 高风险命令后续必须接确认弹窗、前置检查和审计。

## QGC 到 Device Gateway

当前不建议第一阶段实现自建网关。未来草案：

- TCP MAVLink 设备入口：只允许认证设备接入。
- QGC 订阅入口：建议 WebSocket 或受控 TCP relay。
- 每个设备必须有稳定 `device_id`、`vehicle_id`、`sim_or_real` 标记。
- 必须区分遥测下行、命令上行、视频链路和管理 API。

## QGC 到视频服务

当前复用 QGC 视频设置和接收能力。未来草案：

- 输入：`vehicle_id`、`stream_url`、`protocol`、`timeout_ms`。
- 输出：连接状态、最后帧时间、错误码、重连状态。
- 视频失败不得影响 MAVLink Link。

## QGC 到 GIS Safety Service

未来草案：

```json
{
  "request_id": "uuid",
  "mission_draft_id": "draft-id",
  "waypoints": [
    { "lat": 0.0, "lon": 0.0, "alt_m": 80.0, "frame": "relative" }
  ],
  "checks": ["terrain", "building", "nofly_zone"],
  "unknown_area_policy": "warn"
}
```

响应只生成报告和建议，不自动修改任务：

```json
{
  "request_id": "uuid",
  "status": "warning",
  "findings": [
    {
      "severity": "warning",
      "type": "unknown_area",
      "message": "缺少该区域的建筑物计算数据"
    }
  ],
  "suggestions": []
}
```

## 从代码中确认

- 当前 AI 面板默认发送路径调用 QML 注册类型 `Merivus.AiAgentClient`。
- 当前 AI 面板通过 QML 注册类型 `Merivus.AiServiceSupervisor` 调用 `ensureRunning()` 管理本机 Agent。
- QML 不再直接创建活动 `XMLHttpRequest`。
- C++ 请求字段为 `request_id`、`session_id`、`message`、`context`、`allowed_capabilities`。
- C++ 响应字段校验为 `request_id`、`reply`、`proposal`、`provider`、`model`、`status`。
- `proposal` 只在 QML 中显示为未执行建议；经操作员显式触发时可进入 Fleet OS 只读审阅草稿，但不转换为 Vehicle、MAVLink、Swarm 或 PX4 操作。
- 独立 Python Agent 已按新契约实现 Mock HTTP 服务，并支持 `python -m app` 作为 Supervisor 的开发启动入口。
- Supervisor 发布启动路径为 `QCoreApplication::applicationDirPath()/agent/merivus-agent.exe`；开发启动路径仅由 `MERIVUS_AGENT_DEV_PYTHON` 和 `MERIVUS_AGENT_DEV_ROOT` 显式启用。
- Supervisor 不使用 `startDetached`，不使用阻塞式 `waitForFinished()`，也不自动 kill 端口上的未知进程。

## 待确认事项

- 是否保留兼容旧 `intent` 字段一段时间；当前 QGC 默认路径已切到 `proposal`。
- Agent 本机端口当前固定为 `8765`，普通用户暂不编辑。
- 本机 Agent 已支持 `MERIVUS_LOCAL_TOKEN`，QGC Supervisor 已实现当前进程内随机 Token 传递；后续发布包仍需确认日志和进程环境暴露边界。
- 多机场景下 `vehicle_id` 与 PX4 `sysid` 的映射规则。

## AI 本地意图策略契约

`feat/ai-intent-policy` 后，QGC 不再把 Agent `proposal` 原样交给 QML。`AiAgentClient` 会先调用 `AiSchemaValidator` 和 `AiCommandPolicy`，再把附带 `validationStatus`、`policyDecision`、`localRisk`、`requiresConfirmation`、`reason` 和 `executable=false` 的只读建议传给 QML。需要映射到 Fleet OS 阶段时，调用方可以把建议作为 `assisted` 输入 `FleetAutonomyPolicy`；该类只返回分级自主和平台适配器审阅结果，不改变 AI 策略，也不授予执行权限。

Agent 返回的 `risk`、`requires_confirmation`、`executable`、`executed` 等字段不具备权限含义，QGC 会忽略并本地重算。未知字段不会触发任何执行；危险字段、危险嵌套结构、原始 MAVLink 参数数组、shell/script/PX4 批处理内容会被拒绝。

当前策略结果仅用于显示和审计，不连接 Vehicle、Mission、MAVLink、Swarm 或 PX4。高风险命令如 `vehicle.takeoff`、`vehicle.land`、`vehicle.rtl`、`mission.start`、`param.write`、`mavlink.send_raw` 不会执行。
## Provider info 字段（feat/agent-model-providers）

`GET /merivus/info` 在原字段基础上增加：

```json
{
  "provider_ready": true,
  "provider_error": null,
  "available_models": ["qwen3:8b"],
  "external_network_enabled": false,
  "flight_execution_enabled": false
}
```

QGC 仅展示这些字段，不把它们作为飞行动作授权。Provider 失败时，`provider_error` 用于显示明确错误，例如 `Ollama service is not available` 或 `Model qwen3:8b is not installed`。

Agent 响应 schema 保持 `request_id/reply/proposal/provider/model/status`。Provider 不得返回或控制 `validationStatus`、`policyDecision`、`localRisk`、`requiresConfirmation`、`executable`；这些字段由 QGC 本地 C++ 策略层计算。

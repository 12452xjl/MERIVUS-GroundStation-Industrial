# QGC Agent Client 阶段记录

本文档记录 `feat/qgc-agent-client` 阶段的 QGC 到本机 Mock Agent 通信接入，并补充 `feat/agent-supervisor` 阶段后的生命周期边界。

## 范围

- 新增 `custom/src/Ai/AiAgentClient.h` 和 `custom/src/Ai/AiAgentClient.cc`。
- QML 面板只负责界面、消息展示和用户输入收集。
- C++ `AiAgentClient` 负责本机 HTTP 请求、超时、取消、错误状态和 JSON 解析。
- 默认本机地址固定收敛为 `http://127.0.0.1:8765`。
- 在 `feat/qgc-agent-client` 阶段，Agent 仍需要开发者手动启动，不使用 `QProcess`。
- 在 `feat/agent-supervisor` 阶段，`AiServiceSupervisor` 使用普通 `QProcess` 管理本机 Agent 生命周期；`AiAgentClient` 仍只负责 HTTP 通信。
- 不连接真实模型、云端服务、MCP、MAVLink、PX4 或真实无人机。
- `proposal` 只显示为未执行建议，不转换为飞行动作。操作员可显式将可审阅的 `vehicle.*`/`mission.*` proposal 送入 Fleet OS 只读审阅草稿，仍不会批准、上传或下发命令。

## 接口

```text
GET  http://127.0.0.1:8765/health
GET  http://127.0.0.1:8765/merivus/info
POST http://127.0.0.1:8765/merivus/agent
```

`AiAgentClient` 使用 `QNetworkAccessManager` 异步访问以上接口，所有请求都不阻塞 UI 线程。Supervisor 启动自己管理的 Agent 时，会把内存中的本机会话 Token 设置给 `AiAgentClient`；聊天 POST 请求携带 `X-Merivus-Token`，health/info 仍无需 Token。

聊天请求由 C++ 构造：

```json
{
  "request_id": "uuid",
  "session_id": "session-uuid",
  "message": "用户内容",
  "context": {
    "vehicle_count": 0,
    "active_vehicle_id": null,
    "connected": false,
    "armed": false
  },
  "allowed_capabilities": []
}
```

没有飞行器时，QML 明确传入 `vehicle_count=0`、`active_vehicle_id=null`、`connected=false`、`armed=false`。本阶段不伪造电量、模式、GPS 或 RTK 状态。

## 响应处理

C++ 客户端会检查：

- HTTP 状态码必须为 2xx。
- 响应必须是 JSON 对象。
- `request_id` 必须匹配当前请求。
- `reply` 必须是字符串。
- `proposal` 只能是对象或 `null`。
- `status` 必须为 `ok`。

未知字段不会触发任何执行逻辑。解析失败、超时、离线或 `request_id` 不匹配时，QGC 只显示错误并拒绝本次结果。

## UI 状态

AI 面板通过 `AiServiceSupervisor` 和 `AiAgentClient` 显示：

- `Agent未启动`
- `正在连接`
- `正在检查`
- `正在启动`
- `已连接`
- `Agent未安装`
- `端口被占用`
- `Agent已崩溃`
- `请求中`
- `请求失败`
- `请求超时`

设置区显示只读 endpoint、provider 和 model。普通用户本阶段不编辑 Agent 地址。

只有 `AiServiceSupervisor.healthReady=true` 后，QML 才允许通过 `AiAgentClient.sendMessage()` 发送聊天请求。

## 并发和超时

- health、info、chat 均为异步请求。
- 聊天请求同一时间只允许一个。
- 重复发送会被拒绝并显示提示。
- health 默认 2 秒超时。
- info 默认 3 秒超时。
- chat 默认 15 秒超时。
- `cancelCurrentRequest()` 只取消当前聊天请求。

## 安全边界

- `AiAgentClient` 不使用 `QProcess`，也不启动 Agent。
- `AiServiceSupervisor` 使用普通 `QProcess`，不使用 `startDetached`，不调用阻塞式 `waitForFinished()`。
- Supervisor 只关闭自己启动的 Agent；如果 `ownsProcess=false`，不关闭外部手动 Agent。
- 不接真实模型或 API Key。
- 不执行 MAVLink、Vehicle、Swarm 或 PX4 动作。
- `proposal` 只显示为“未执行建议”；可由操作员显式送入 Fleet OS 只读审阅草稿，但仍不转换为 Vehicle、MAVLink、Swarm 或 PX4 操作。
- 原生 QGC 手动飞控、地图和 Link 功能不因 Agent 离线而阻塞。

## 下一阶段

`feat/agent-supervisor` 已增加 Agent 进程监管。下一步不建议直接进入真实模型执行链路；应优先完成结构化意图、本地策略和确认框架，或进入发布打包验证 `agent/merivus-agent.exe` 放置路径。

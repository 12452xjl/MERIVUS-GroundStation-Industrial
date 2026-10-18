# MERIVUS Agent Supervisor 阶段记录

本文档记录 `feat/agent-supervisor` 阶段的本机 Agent 生命周期管理设计。

## 范围

- 新增 `custom/src/Ai/AiServiceSupervisor.h` 和 `custom/src/Ai/AiServiceSupervisor.cc`。
- `AiServiceSupervisor` 只负责本机 Agent 进程生命周期、health 轮询、失败降级和退出清理。
- `AiAgentClient` 继续只负责 HTTP 请求、JSON 解析、超时、取消和聊天响应处理。
- QML 只负责界面、状态展示和用户输入，不直接启动进程、不直接发起网络请求。
- 本阶段不接 OpenAI、DeepSeek、Gemini、Ollama、MCP、云端 Agent、数据库或真实模型。
- 本阶段不执行 MAVLink、Vehicle、Swarm、PX4 或真实无人机动作。

## 生命周期状态

`AiServiceSupervisor` 暴露以下状态：

- `Disabled`：用户未启用本机智能体。
- `Stopped`：Agent 未运行或当前不由 QGC 使用。
- `Checking`：启动前或手动 health 检查中。
- `Starting`：QProcess 已发起启动，正在等待 `/health` 就绪。
- `Healthy`：`/health` 返回兼容 `service=merivus-agent`。
- `Stopping`：QGC 正在异步关闭自己启动的 Agent。
- `Crashed`：自己启动的 Agent 异常退出。
- `NotInstalled`：未找到发布 Agent 程序，也未配置开发启动变量。
- `PortConflict`：`127.0.0.1:8765` 可连接，但不是兼容 MERIVUS Agent。
- `Error`：启动失败、health 超时或运行期连续失败达到阈值。

## 启动路径

禁止硬编码开发机绝对路径，也不扫描磁盘寻找 Python。

发布模式优先路径：

```text
QCoreApplication::applicationDirPath()/agent/merivus-agent.exe
```

开发模式按以下顺序解析：

1. 显式配置的开发环境变量；
2. 仓库内 `agent/.venv` 的 Python；
3. 当前 `PATH` 中可用的 Python。

Windows Store 的零字节 `WindowsApps\python.exe` 执行别名不会被当作可用解释器，避免未安装 Python 时出现“进程启动后立即崩溃”。

需要覆盖仓库虚拟环境时可显式配置：

```text
MERIVUS_AGENT_DEV_PYTHON=<agent/.venv/Scripts/python.exe>
MERIVUS_AGENT_DEV_ROOT=<repo>/agent
```

开发模式启动命令：

```text
<MERIVUS_AGENT_DEV_PYTHON> -m app
workingDirectory=<MERIVUS_AGENT_DEV_ROOT>
```

如果发布 EXE、仓库虚拟环境、显式开发解释器和 `PATH` Python 均不可用，状态进入 `NotInstalled`，界面显示“未找到本机Agent程序”，QGC 其他功能继续运行。

## 启动前检查和端口冲突

调用 `ensureRunning()` 后，Supervisor 先访问：

```text
GET http://127.0.0.1:8765/health
```

处理规则：

- health 成功且 `status=ok`、`service=merivus-agent`：复用已有 Agent，`state=Healthy`，`ownsProcess=false`，不启动第二个进程。
- health 连接失败或超时：按启动路径尝试启动 Agent。
- 端口返回 HTTP/JSON 但不是兼容 Agent：`state=PortConflict`，不启动、不 kill、不清理占用端口的未知程序。

## health 策略

启动后：

- QProcess `started` 只表示进程已启动，不代表 Agent 可用。
- 每 500ms 检查一次 `/health`。
- 最长等待约 10 秒。
- health 成功后进入 `Healthy`，并允许 QML 发送聊天请求。
- 启动超时后进入 `Error`；如果该进程由 QGC 启动，则异步关闭失败进程。

运行期间：

- 每 5 秒进行一次轻量 health 检查。
- 连续 3 次失败后才标记异常，避免瞬时网络失败触发重启。

## 自动重启

自动重启仅适用于 `ownsProcess=true` 的 Agent。

- 默认最多自动重启 2 次。
- 使用递增等待，避免紧密循环。
- 达到限制后停止自动重启，用户可通过 UI 的“重启Agent”再次触发。
- 外部手动启动的 Agent 不会被 QGC 重启或关闭。

## QGC 退出处理

如果 `ownsProcess=true`：

1. 调用 `terminate()`。
2. 使用异步 `QTimer` 等待约 3 秒。
3. 仍未退出时调用 `kill()`。

如果 `ownsProcess=false`，QGC 不关闭外部 Agent。

实现不使用 `startDetached`，不调用阻塞式 `waitForFinished()`，也不在 UI 线程进行长时间等待。

## 打包 Agent GUI 冒烟

`release/agent-packaging-poc` 阶段已完成一次 GUI 人工冒烟：

- `staging/MERIVUS.exe` 能够通过 `applicationDirPath()/agent/merivus-agent.exe` 找到打包 Agent。
- Supervisor 能够自动启动打包 Agent。
- health 正常并进入 `Healthy`。
- QML 显示 Agent 已连接。
- provider/model 显示 `mock/mock-v1`。
- 普通聊天请求能够返回 Mock 回复。
- 无 Vehicle 连接时未崩溃。
- proposal 只显示为未执行建议，不执行飞行动作。

退出残留只读检查未发现 `MERIVUS` 或 `merivus-agent` 进程，也未发现 8765 端口监听。由于本轮没有自动控制完整 GUI 关闭序列，后续仍建议重复人工验证 QGC 关闭时自己启动的 Agent 会退出并释放端口。

## 本地 Token

Supervisor 启动 Agent 时生成随机本机会话 Token，并通过子进程环境传递：

```text
MERIVUS_LOCAL_TOKEN=<memory-only-token>
```

同时通过 C++ 信号把 Token 交给 `AiAgentClient`，由聊天 POST 请求携带：

```text
X-Merivus-Token: <memory-only-token>
```

Token 只存在于当前进程内存和子进程环境，不写入配置文件、不写入日志、不显示在 QML、不提交 Git。

health 和 info 继续无需 Token；手动启动且未配置 Token 的开发 Agent 仍保持兼容。

## QML 行为

AI 面板实例化：

- `AiServiceSupervisor`：负责 `ensureRunning()`、状态显示和重启/停止。
- `AiAgentClient`：负责 `sendMessage()`、health/info/chat HTTP 请求。

用户打开 AI 面板不会无条件启动 Agent。启用“本机智能体”后才调用 `ensureRunning()`；关闭浮窗不立即停止 Agent；用户关闭“启用本机智能体”时只停止自己启动的 Agent。

只有 `healthReady=true` 后，QML 才允许发送聊天请求。Agent 离线、未安装、端口冲突、启动失败或崩溃时，界面显示清晰状态，地图、Link、Vehicle 和手动操作继续可用。

`proposal` 仍只显示为“未执行建议”，不会转换为飞行动作。对于通过本地策略校验的 `vehicle.*`/`mission.*` proposal，操作员可以在 AI 面板中显式点击“生成舰队审阅草稿”；该动作只调用 Fleet OS `prepareFleetIntent()` 做只读风险审阅，不请求批准、不上传任务、不释放命令。

## 失败降级

- Agent 缺失：显示未安装，不影响 QGC。
- 端口冲突：显示端口被占用，不 kill 未知进程。
- 启动超时：显示启动失败，异步关闭自己启动的失败进程。
- Agent 崩溃：状态进入 `Crashed`，可按限制自动重启；QGC 主界面不阻塞。
- 外部 Agent：复用但不关闭、不重启。

## 下一阶段

本阶段完成生命周期管理。后续可选择：

- `feat/agent-model-providers`：接入模型 Provider，但必须继续通过统一 schema 和本地安全策略。
- `release/windows-packaging`：发布包中放置 `agent/merivus-agent.exe`，并确认运行库、日志目录和升级策略。

进入模型 Provider 前，仍建议先完成结构化意图、本地策略和确认框架，避免模型输出直接靠近执行面。

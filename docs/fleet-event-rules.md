# FleetEventRuleEngine

## 546：可保留、可处理但不执行的人工收件箱

实际 `FleetEventBlackBox` 由私有owner以 `bindLiveSource(actualBlackBox)` 绑定；引擎只消费该对象正在发布且完整匹配的原生 `nativeEventAccepted(epoch, sequence, exactEvent)` 回执。独立 live 冷却／前序态不与旧 `evaluate()` 的日志导入、回放和 `resetRuntimeState()` 混用。下一条未命中事件或回放不会删除已保留的待审项；换源、源销毁、规则变化也不改写原待审详情。来源回执只是本地接收材料，不是远端身份认证、正式任务结果或授权。

待审箱仅本次会话：最多64条pending，每条完整JSON≤128 KiB、全箱≤4 MiB。一条live事件的全部候选要么完整入箱，要么整次拒入并更新overflow／固定原因，不截前缀、不自动trim旧pending；拒入不推进候选live冷却，原接收序号不能重放重试。完整值快照有深度／累计节点／文本／实际字节检查；不调用opaque QVariant转换器。自审前缀 `fleet.rule_review.` 不再生成新待办。

只读 `reviewRows` 是8键小摘要：`reviewId/revision/ruleId/eventType/actionType/generatedAtMs/sourceEpoch/sourceSequence`；`revision`与sourceSequence均保十进制String，不转Number。`reviewStatus` 为 `schemaVersion,pendingCount,overflowCount,lastError,sessionOnly=true,automaticActionsIssued=false,executionPermit=false,flightCommandReleased=false`。`reviewPage(offset)` 每页4条；`reviewDetail(reviewId,revision,offset)` 按2048 UTF-16单元提供完整预算内来源事件、原规则、原action与建议的JSON文本片，不割代理对。单片不是独立JSON，界面不整树重序列化。

操作员只可精确id／revision调用 `decideReview(...,"reviewed"|"rejected",reason)`，拒绝须写原因。成功原子移出这条pending并返回15键真实决定回执，不另建无限终态历史；Root将该实际回执写入现黑匣子与证据链，记录两者是否本地接纳但不保证磁盘持久化。标记已审阅不表示故障解决，拒绝建议不表示飞机命令撤销。界面“前往任务规划”只打开原规划入口，不从未知action代填坐标、建立任务、预演、批准或调用执行器；如需新草稿须另走原明确人工流程。

完整API、错误与通知边界见 [模块合同](../custom/src/Swarm/FleetEventRuleEngine.md)。本批仅源码修改与直接静读，未运行任何评估／JSON、文件、编译、测试、渲染或设备。以下描述保留旧接口与历史接线背景；它们不作为新live收件箱的输入或决定来源。

`FleetEventRuleEngine` 是一个只在地面站内运行的事件规则评估器。它把已解码的事件和操作员登记的规则进行匹配，返回“建议动作”供 QML 或任务编排层审阅；它不持有 `Vehicle`、MAVLink、Mission 或命令发送接口，因此不会自行控制飞行器。

## 生命周期接口

通过 `Merivus 1.0` 注册为 `FleetEventRuleEngine`：

- `registerRule(rule)`：登记规则。规则必须有唯一 `id`、`eventType`（或 `*`）和至少一个动作；重复 ID 被拒绝，避免无意覆盖已批准规则。
- `setRuleEnabled(id, enabled)`：只启停指定规则。
- `removeRule(id)` / `clearRules()`：删除规则。
- `enabled`：全局启停开关。
- `evaluate(event)`：返回建议列表，不发送任何命令。

## 规则和事件格式

```js
{
    id: "low-battery-hold",
    eventType: "battery.changed",
    priority: 100,
    enabled: true,
    cooldownMs: 30000,
    previousEventType: "link.degraded",
    conditions: {
        "batteryPercent": { operator: "lte", value: 30 },
        "vehicle.online": true
    },
    actions: [
        { type: "suggest_return_to_launch", reason: "battery_low" },
        { type: "notify_operator", severity: "warning" }
    ]
}
```

事件至少要有 `type`（兼容使用 `eventType`）；业务数据放在 `payload` 中，也可用顶层路径。条件按 AND 关系匹配，支持 `equals/eq`、`not_equals/ne`、`gt/gte/lt/lte`、`contains`、`in/not_in`、`exists`、`truthy/falsy`。路径使用点号，例如 `payload.linkQuality` 或 `vehicle.online`。事件类型 `*` 匹配所有事件。

规则注册会严格校验条件：路径必须是有限长度的点分段，单条规则最多 64 个条件、每个列表最多 128 项；比较数字必须是有限的真实数值，`exists` 的期望值必须是真实布尔。字符串 `"42"`/`"false"` 不会在规则评估中被隐式当成数字或布尔。

`evaluate()` 的每个结果包含 `ruleId`、事件类型、优先级、`recommendationOnly: true`、`manualConfirmationRequired: true`、`flightCommandReleased: false` 和一个 `action` 对象。若输入事件带 `timestampMs/timeMs`，建议沿用该时间戳，便于回放保持确定性；调用方必须继续执行人工确认、权限检查和现有事务流程，不要把返回结果直接接到 MAVLink 或 Vehicle API。规则目录最多登记 256 条，异常输入会被拒绝。

`cooldownMs` 可设置 0–24 小时的单规则冷却窗口；冷却只抑制重复建议，不执行任何自动动作。`previousEventType` 可要求当前事件必须紧跟指定的上一事件类型，用于表达“链路下降后再低电量”等跨事件顺序。引擎保存的是轻量运行时状态，不保存任务权限或飞行状态；可调用 `resetRuntimeState()` 开始新的日志回放，调用 `runtimeState(ruleId)` 查看上一事件、最近命中时间和命中次数。带时间戳的事件应按时间顺序输入，开始新的回放前必须重置状态。

## 安全边界和后续接入

本实现仍不执行自动动作；冷却和跨事件状态只用于减少重复建议、提高解释性。后续如需接入任务编排，应由上层把建议显示给操作员或转成待审批任务，并记录规则版本、输入事件和最终人工决定。真实链路接入、编译和 SITL/硬件验证由工程师负责。

当前 `GuidedActionsController.qml` 已把 `FleetEventBlackBox.eventRecorded` 接到规则引擎：每个被日志接受的实时事件都会得到一轮建议，结果通过只读属性 `eventRuleSuggestions` 和 `eventRuleSuggestionCount` 暴露给 QML。清空或导入空日志时会清除界面上的旧建议。内置的舰队降级、编队异常规则只产生“人工复核”提示，不会调用任何飞行命令。

同一控制器会把黑匣子事件同步到 `FleetTimelineReplay.events`；回放 `eventReady` 也复用规则引擎。导入非空日志时会在下一事件循环中重建规则运行态，避免旧建议残留；回放播放期间不会因为新实时事件重置当前回放位置。

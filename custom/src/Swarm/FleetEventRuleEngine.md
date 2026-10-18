# FleetEventRuleEngine 契约

## 546：原生日志接收回执驱动的待人工审阅收件箱

新增 `bindLiveSource(FleetEventBlackBox*)`，只允许同线程实际对象。引擎自己连接该对象的 `nativeEventAccepted`，并在原回调内调用该对象的 `nativeEventReceiptMatches(epoch, sequence, exactEvent)`；仅该尚在发布的实际回执可进入 live 评估，不能从 QML 提交事件 Map、导入日志或回放制造本次实时来源。epoch／sequence 只标识本地接收，不认证远端身份、任务实例、事件内容真实性或飞行状态。绑定换源重置独立 live 冷却／前序状态，已保留的 pending 不删除；源销毁同样保留其历史说明。

旧 `evaluate/resetRuntimeState/runtimeState` 继续原历史／回放行为，使用原 `_runtimeState`，不改变 live 冷却或收件箱。规则目录实际删除、停用或整份导入会清对应／整份 live 运行态，但不追溯修改已捕获的原规则。`fleet.rule_review.` 自审事件前缀在新 live 路径忽略，不再生成自审待办，也不推进 live 前序状态。旧 evaluate 接受域、规则文件格式及既有文件接口不变。

### 原子保留与预算

仅保留 pending，最多64条；每条完整紧凑 JSON 至多128 KiB，全箱累计至多4 MiB。一次真实 live 事件的全部匹配 action 先在临时运行态／候选中检查、生成，全部合格才一起提交。容量不足、节点／类型／字节预算或 revision 空间耗尽时整次事件拒入，不安装成功前缀、不挤掉旧 pending、不推进候选 live 冷却／匹配计数；原回执 sequence 仍被消费，不能后来重试成另一轮实时事件。无建议的合法事件仍推进 live 前序状态。`overflowCount` 是此次会话中拒入事件的饱和 uint64 计数，包含重入／容量／值或字节预算拒绝；`lastError` 给固定原因，不把拒入当作安全或任务结论。

每项私有详情是完整 `source{sourceEpoch,sourceSequence,event}`、原规则、原 action 和 suggestion 的有界 JSON 值快照及 recommendation-only／未放行声明，不执行、不推断或改写 action。先按深度24及累计节点／字符串／key／base64长度预算检查，再做规范化及实际字节检查；拒 opaque/custom 转换器（包括typed-null opaque）、非有限数及不能精确保留的64位整数。Map／Hash、List、StringList、String、ByteArray、Bool、数值和 Qt 日期时间按明确 JSON 投影保留；ByteArray为base64，日期时间为Qt JSON值，invalid／native JSON null与上述builtin typed-null保null，不承诺恢复原 QVariant 元类型。共享整数检查保原整型精度；Double在转qint64前排除2^63边界，再用准确整数上下界比较，避免时间戳窄化越域。预算是保留内容和工作量边界，不是总内存峰值或实时性测量。无文件、后台线程或新终态历史服务。

### API 与闭形 DTO

只读 `reviewRows` 与 `reviewStatus` 共用 `reviewsChanged`。row固定8键：`reviewId`（UUID）、`revision`（不回绕的正uint64十进制String）、`ruleId/eventType/actionType`（String）、`generatedAtMs`（原事件本地接收时间数值）、`sourceEpoch/sourceSequence`（原本地回执String）。不向模型暴露完整事件树或可调用对象。UUID与revision一起定位，revision不转JavaScript Number。

status固定8键：`schemaVersion=1,pendingCount,overflowCount,lastError,sessionOnly=true,automaticActionsIssued=false,executionPermit=false,flightCommandReleased=false`，其中overflowCount为规范uint64 String，lastError成功时显式空String。无建议且公开status不变的合法事件仍提交私有live运行态，但不发reviewsChanged，避免仅前序／冷却推进重复重建界面待审目录。

- `reviewPage(offset)`：实际整数offset范围0..pendingCount，正常返回8键 `ok=true,error="",offset,nextOffset,totalCount,hasMore,rows,sessionOnly=true`，每页最多4条。页是当次只读目录，界面仍须持原row id／revision，不能仅按旧索引决定。
- `reviewDetail(reviewId, revision, offset)`：精确原pending身份；offset为实际整数UTF-16偏移，正常8键 `ok=true,error="",reviewId,revision,offset,nextOffset,hasMore,text`。每片最多2048 UTF-16单元，末尾不割代理对，落在低代理项的offset拒绝。连续片拼成完整预算内JSON；单片不保证是完整JSON，不在QML parse/stringify整棵事件树。旧id／revision或错offset拒绝，不重基到后来条目。
- 上述读取失败仅返回 `ok=false,error`，不改收件箱或lastError；非对象线程读取同样拒绝。
- `decideReview(reviewId, revision, decision, reason)`：精确CAS，decision仅`reviewed/rejected`；reason为实际非null、已trim、无控制字符、至多512字符String，rejected必须非空。成功只移除该原pending并释放其预算，不执行action、不创建任务或声明已处理故障。readonly/detail不作决定。

决策回执固定15键：`ok,committed,error,reviewId,revision,decision,reason,ruleId,eventType,actionType,generatedAtMs,sourceEpoch,sourceSequence,decidedAtMs,flightCommandReleased=false`。成功回执包含实际被移出的原摘要，`committed=true`；通知期间销毁／换源或后续审计失败不能把已移出事实说成未发生。拒绝回执固定空身份、0时间和`committed=false`，不冒用此前决定。引擎不积累无限决策历史。

Root须把实际原生决策回执写到既有黑匣子与证据链，分别报告两者本地接纳结果，不能把接纳当持久化、签名或正式任务结果。界面可以明确前往已有任务规划入口，但只导航；这里没有将未知action转换为`prepareFleetIntent`、任务编排、Vehicle／Swarm或飞行命令的接口，也不自动覆盖当前草稿、填坐标、批准或启动。

工程场景全部待执行：live与回放冷却隔离、下一事件不清pending、实际回执／错源／重复序号、绑定通知销毁与换源、64条／128 KiB／4 MiB整批拒入、无成功前缀、2048代理对连续浏览、精确旧id／revision拒绝、拒绝原因、决定后通知重入及审计失败不回滚、本地自审前缀不反馈生成待办。仅apply_patch及直接静读，未执行评估器、JSON／hash／文件、线程、编译、测试、界面或设备。下方为旧接口合同。

`FleetEventRuleEngine` 是地面站内的事件规则建议器。它只读取黑匣子事件并返回可解释的人工复核建议，不访问 Vehicle、MAVLink、MissionManager 或任何命令释放接口。

## 规则快照

- `exportJson()` 输出 `merivus.fleet.event-rules`、`schemaVersion=1`、`engineVersion`、启用状态和规则定义。
- `importJson(json, replaceExisting)` 严格拒绝未知根字段、错误版本、重复规则 ID、错误字段类型、超过 256 条规则或 `flightCommandReleased=true`。
- `saveToFile()` 使用 `QSaveFile` 原子提交，`loadFromFile()` 限制 32 MiB 并拒绝非法 UTF-8/JSON。
- 导入只恢复规则定义和总开关；冷却、前序事件和匹配次数等运行时状态始终清空，避免旧快照制造虚假事件序列。
- 条件键和值采用严格 schema：路径仅允许有限长度的点分段，条件最多 64 项，列表最多 128 项，数字必须是有限的真实数值，`exists` 的期望值必须是真实布尔；字符串数字/字符串布尔不会被隐式转换。
- `replaceExisting=false` 时在现有规则上合并，重复 ID 拒绝；`true` 时以导入集合替换现有规则。

## 工程边界

快照导入不会启用任何任务，也不会改变编队、Vehicle、飞行模式或飞行命令。工程师仍需验证文件权限、版本迁移、坏文件原子性和规则建议的人工作业流。

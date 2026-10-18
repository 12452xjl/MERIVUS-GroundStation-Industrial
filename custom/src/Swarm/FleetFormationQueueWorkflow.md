# FleetFormationQueueWorkflow — 542

本工作流连接实际 `SwarmController::formationQueue` 和既有 Guided 编队人工确认/派发链，不建立另一命令服务。队列只保存本次选中机号，不保存账号批准、健康通过、飞行许可或可重放的确认令牌；没有自动 enqueue、自动复核、自动派发或持久恢复。

## 可见值与入口

Root 仅公开 JSON-detached 的 `formationQueueRows`、`formationQueueStatus`。实际 Queue QObject、reviewToken 和一次确认关联均在私有 lexical id 内。rows 为至多32项 exact11：`id/revision/vehicleIds/state/reason/createdAt/expiresAt/canReview/canCancel/canRemove/taskId`；revision 保正 uint64 十进制字符串，不经过 Number。时间为原生 UTC ISO 字符串，只供展示；30分钟待办与60秒复核期限由原生单调钟维护。status exact5 为 `count/limit/sourceCurrent/automaticDispatch/persistent`，后两项 false。ready 只表示当时本地资源可再审，不表示健康、账号、机载接受或执行可用。

四个公开方法只返回 exact2 `{ok,reason}`：

- `queueSelectedFormation()`：在方法入口冻结实际 selectedVehicleIds，显式保存1..6中含主机1的唯一机号。即使编队 busy 也允许保存；不把现有“结束编队”按钮误作排队操作。
- `reviewQueuedFormation(id,revision)`：只使用原 Queue 的 beginReview 回执，不读取共享 latest 猜归属。原生复核成功后，调用原 `confirmAction(actionSwarm, exactIds)` 并建立私有一次上下文与该次 fresh confirmation token 的身份关联。确认框未成功建立、源改变或后来的动作替换时不沿用旧批准。
- `cancelQueuedFormation(id,revision)`、`removeQueuedFormation(id,revision)`：只转原精确行身份。它们取消/删除本地待办，不取消飞机任务、不撤回已发命令；dispatching 由原生拒绝这些操作。

原生 beginReview 返回 exact6（exact5 基本回执加 reviewToken），其余相关操作 exact5。Root 只采用同原 Queue/Swarm/Orchestrator 的实际返回值；失败、异常或通知中换源不按同机号寻找替代 owner，不自动重试。原生复核令牌60秒后自然失效并改变revision；关闭后可明确按当前 revision 再复核，不能复活旧 token。

## 一次确认与实际派发

普通 confirmAction、关闭或取消确认框清理私有 opening/pending/claim，原生队列复核可自然超时；不借关闭操作取消整个待办。另一个普通编队动作不能认领队列回执。新方法的内部第四参数仅比较私有对象身份，不从 caller map 安装批准。

本次 issued confirmation token 同时保私有 queue-origin 标记。即使待办取消后 pending 已清除，旧 token 也只允许队列来源：缺失原精确私有回执即拒绝，不能把它降格成普通编队发送。token 仅在两个 lexical id 之间传递，签发方法不返回 capability。原对话框 hide/cancel 仍沿 confirmationCancelled/closeAll 烧掉令牌；再次普通发送必须另开人工确认。

原 executeAction 先验证/消费既有 Guided token，再走原执行时健康门；actionSwarm 分支才精确 consumeReview。consume 是原生同步的一次消费，先将行转 dispatching 并返回新的 dispatch revision。之后仍使用原 createTask/startTask、整组预约、同机号复核和原 `SwarmController.sendStartCommand`；没有直接包发送、跳过 native health/grounded 门或扩大机号范围。普通非队列编队也经过同一原派发实现，不会暗中排队。

共享派发 helper 额外要求 executeAction 在原 token/health 门之后创建的私有一次 plain grant 身份，并在 helper 外调前消费；QML 可公开调用 Root 函数并不意味着能直接调用该 helper 发包。grant 不在公开属性、返回值、队列行或事件中输出。

所有已取得实际 consume 回执的路径都有 detached finally：清理前先冻结原 Queue、id、dispatchRevision、实际 taskId 和本地提交事实；即使后续通知删 Root、换源或抛异常，仍只尝试原 owner 的 finishDispatch，绝不查询新 owner 补写。销毁的原 QObject wrapper 可以抛出，清理保守捕获；不能据清理异常称派发未发生。派发后队列行不再自动重排。

若本调用已创建的精确本地 formation task 仍 running 且从未进入 sendStartCommand，finally 可以只失败该未发本地任务，避免残留预约；核原taskId/type/ids，不碰后来任务。一旦进入 send，无论回执丢失或partial都不借此分支清掉可能已发的未知占用。

submitted 仅取原 sendStartCommand 回执的严格 `ok=true`，意思是本地请求已提交，不是 ACK、执行或成功。false 路径保留实际已创建 taskId，reason 明确 rejected/partial-or-unknown；它不证明零命令发送或原任务已停。取消晚于实际消费/提交不能撤回副作用，必须核对原编队状态和事务。

原 BlackBox 记录显式 enqueue/cancel/remove 和消费后的 dispatch-result；本地 record 接受也不是磁盘持久确认，不更改 ArchiveCodec 或恢复飞行权限。原生成确认/消费票据与 Qt NOTIFY 的寿命尾核保留，调用票据是私有 plain 状态，不把无通知的 ticket 作为公开按钮可用性的唯一依据。

本批仅 apply_patch 和直接源码/合同静读；未运行 QML、应用、队列/算法/解析器/helper、编译、测试、仿真、真实派发、设备、刷机或打包。工程师需验证：busy时保存、FIFO/TTL/替换复核、关闭/其它动作/健康变化、原确认令牌和行revision错配、consume通知重入/销毁、各创建/预约/派发失败与部分提交，以及原 owner 死亡时不认新 owner。

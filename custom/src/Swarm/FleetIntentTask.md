# FleetIntentTask 原生任务预览与人工审批

本类只保存本地任务意图、参数、预览和人工审批状态，不持有 Vehicle/MAVLink，不上传任务，不解锁、不起飞，也不授予任务部署或启动许可。`approved` 仍只是一层本地工作流状态；现有 Root、PlanStore、部署、双人审批及启动检查不因此省略。

## 441：显式禁飞区输入不能折叠为空列表

`parameters` 不含 `noFlyZones` 时，保留已有兼容路径。它只表示“未提供”，不表示禁飞区已加载、内容为空、数据新鲜或覆盖完整。

只要键存在（包括显式 null、false、空字符串），就调用共享原生 `FleetAirspaceConflictChecker::inspectNoFlyZoneInput()` 检查原 QVariant；不会先 `.toList()` 或用真假值将坏输入替换为空数组。共享接口要求真实非 null List、最多 1024 项及完整 JSON 资源预算；它只检查类型、资源和表示，不认证每项禁飞区几何。

每份新预览新增固定 5 字段 `noFlyZonesInputCheck`：

```text
valid: bool
state: list | invalid | not-provided
reason: string
zoneCount: integer | JSON null
fingerprint: SHA-256 | 空字符串
```

`list/invalid` 完整沿用共享检查结果。只有真正缺键才使用 `valid=true/state=not-provided/reason=""/zoneCount=null/fingerprint=""`；合法显式空列表则是 `list`、计数 0 和实际空数组指纹，不能与缺键混淆。

坏输入不丢弃本次参数，也不展示上一份可审批预览。当前预览保留原参数和共享诊断，同时强制 `feasible=false`、`parametersValid=false`、`needsReview=true`、`reviewStatus=blocked-invalid-airspace-input`，加入 `invalid_no_fly_zones_input` blocking 风险；风险评估明确 `inputValid=false`。仍允许操作员查看错误并修正，不能请求或完成审批。

`requestApproval()` 和 `approve()` 每次重新检查当前私有 `_parameters`，不只相信预览中的诊断字段或此前的一次检查。现有 preview/awaiting-approval 状态、非空意图、候选可行性和风险输入检查仍保持；合法列表也不跳过这些条件。

## 与外层审阅身份的关系

同值 `setParameters()` 仍不重复发参数变化通知，但 `preparePreview()` 每次都重新生成预览并发状态通知。Root 原有输入代次、审阅代次、先清绑定再 CAS 撤销旧 PlanStore 审阅的逻辑仍负责完整来源绑定。`matchesInputs()` 只是当前值比较，不是代次或授权证明，不能替代这些检查。

本类新增一个不公开的本地通知修订号；每次实际输入/状态变更，以及每次重新预览，均先推进修订号。输入 setter 先整体提交新参数及 draft/清预览状态，再通知；不会在参数通知后才让旧外层清除回调中刚创建的新预览。每个同步通知后使用原生 `QPointer` 与修订号核对，若对象已销毁、状态被回调替换，或输入发生 A→B→A，旧调用不再继续发剩余通知。布尔转换方法仅在本次修订和目标状态仍当前时返回 true；曾短暂进入某个本地状态但已同步失效不算当前批准，也不是任何飞行命令的回滚证明。修订号耗尽时拒绝新变更和新审批，不回绕复用；此私有数字不是新的外部许可或记录身份。

本批仅源码修改和静态阅读；未编译、测试、运行界面或访问设备。坏输入及同步状态切换的实际 Qt 行为仍由工程师验证，不将静态审阅写成运行通过。

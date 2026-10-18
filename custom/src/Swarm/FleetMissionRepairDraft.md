# FleetMissionRepairDraft — 从失败部署复制原规划输入

## 472：新保存草稿不放宽原失败部署取源

新增的[编辑草稿保存／恢复入口](FleetMissionSavedDraftPanel.md)和[固定本地单槽 Store](FleetMissionDraftStore.md)属于独立 `saved-draft` 来源，可保存结构合法但尚不满足构建条件的原生编辑内容。本模块仍只从原失败／取消 Deployment 及同源终态排空对账读取完整规划源；原 `validatePlanningSource` 与 `loadPlanningSource` 的完整可构建条件不变，不借编辑校验绕过修复来源门禁。新来源只经独立 `validateEditingSource`／`loadEditingSource` 恢复为人工继续编辑的副本，不成为原部署、历史审阅或当前机载状态证据。

Root 的来源版本预检补齐原 `clearanceInput` 内层 schema 2（含 `contacts`），同时保留 schema 1 与外层五键来源 schema 1／2 的对应关系。预检不是完整源认证；原生仍验证全部保留值、执行意图及工程声明，不能只以字段存在或版本号宣称有效净空。

`deployment-repair`、`archived-plan`、`saved-draft` 共用当前草稿和一份会话内备用、同一候选／ticket／销毁等待机制及旧备用替换确认。原 scope 的审批撤销、完整来源重核、成功后才发布新当前并保留旧当前的次序不变；恢复与切回仍清坐标基准确认，后续必须重新构建、原生审阅和审批。不恢复批准、账号、设备证据、原任务执行、控制权或其它飞行许可；内存备用也不被称为磁盘编辑槽。保存／重启读取及其失败事实详见上述两个新模块合同。

本增量只做源码／文档静态核对，未编译、执行 helper／解析器／业务代码／UI 或测试，未进行设备操作。它不表示本模块的实际修复、恢复或整机流程已全功能验收；下方原取源和输出合同继续有效。

## 448：失败部署的完整工程声明随原规划源保留

实际Deployment原只读取源继续调用[Draft中央验证](FleetMissionDraft.md)。外层成功schema1／十键、旧binding与对账收据不变；内层五键planningSource可为来源版本1（无clearanceInput）或版本2（settings完整包含该资料）。Root修复预检同步接受严格对应的两版，不以版本2为由丢弃资料或回退到原11项。

复制后仍清地图基准确认、启用独立本地revision，保留旧草稿供切回；不改变原record、对账unknown、旧批准或机载任务。成员／角色改变不会把旧逐机包络改成新机号，buildTask／Compiler要求资料与活动主机完整同序匹配。格式有效和修复副本创建不是净空通过，几何solver、实际启动风险固定消费与接地上下文尚未完成，当前新声明任务明确pending阻断。无新布局或自动操作；未编译、测试、运行或连接设备，下方427为历史。

## 427：实际 Deployment 的只读取源边界

`FleetMissionDeployment.repairDraftSource(expectedDeploymentId, expectedReconciliationRequestId)` 是纯值接口，不是新的飞行服务。它只接受此实例的当前失败或取消部署，以及同一部署、代次、binding、目标顺序下刚完成的实际426独立对账；不消费调用方传入的对账 JSON。

原部署必须为 failed/cancelled，仍通过 Episode 的 outbound-owner 检查；已归组对象只能来自允许发送意图的实际当前 Episode，未归组原生 legacy 保留既有兼容语义，不把此兼容解释为权限。实际 Root 入口另核当前 Episode。本地 begin/gate/inspection/dispatch、deferred 操作、占用、排空和原 Vehicle/MissionManager 发送栈均须退出，原 manager 也不能仍有任务事务。私有对账不得 busy，结果须为 completed/stopped/cancelled、pendingVehicleCount=0，且没有 active/reservationHeld/transportDraining。已经离线或销毁的原飞机无需重新上线，本接口不查找替代源。

对账中 matched、different、unknown 都可成为新的人工规划起点；结果仍保持 historicalOnly/snapshotOnly，所有当前机载/启动/执行许可旗标为 false。终态 stopped/cancelled 不被改写成成功，对账未知不被改写成无差异。

## 原始编辑值，不反推任务几何

来源仅为 Deployment 已冻结 `_recordSnapshot.plan.planningSource`。其五键为 `schemaVersion/sourceRevision/vertices/members/settings`，由 `FleetMissionDraft::validatePlanningSource()` 按既有编辑范围和 buildTask 规则验证。验证会规范化实际数值类型，但保留完整编辑内容，包括未启用端点、暂不参与编译的成员路线、reserve 成员 profile 及原 heading=360。

成功验证须返回固定四键 `ok/source/task/error`，其中 ok=true、source/task 为非空 map、error 为空字符串。不能从 plan.areaSummary 的顶点数量、生成的成员航线、普通回读缓存、426 RAW 观察或当前可变 Intent 中重建缺失源。旧记录没有 planningSource 明确返回 `original-planning-source-not-recorded-cannot-restore-draft`；原始输入非法或无法形成现有 Draft 任务时拒绝，不返回仅能展示的空摘要。

## 输出契约

成功固定10键：`ok/schemaVersion/deploymentId/deploymentGeneration/binding/reconciliation/planningSource/manualConfirmationRequired/flightCommandReleased/executable`。ok=true、schemaVersion为整型1、deploymentGeneration为十进制字符串；binding为原7或9字段值；reconciliation为实际当前30键完整有界摘要；planningSource为验证后的完整五键编辑源；manualConfirmationRequired=true，最后两个执行字段恒false。

拒绝固定4键：`ok/error/flightCommandReleased/executable`，ok=false，error为说明原因的字符串，其余两个字段恒false。接口不写 lastError、不发 changed 信号，不借错误回调改变原结果。

## 后续人工修复与交接边界

取源值只用于创建新的本地可编辑副本。UI需要明确人工确认、核原源身份和当前草稿代次，并保留或明确处理原编辑草稿；不得静默覆盖未保存输入。修改后必须沿新的 buildTask、Compiler、当前原生 Review、PlanStore 新版本及人工审批流程重新审阅。原 record、原机载状态、原426诊断均不因复制而被改写；不从旧 profile/参数摘要恢复当前证据，不自动重传、回滚或清空任务。

本接口不发布可编辑草稿，也不产生新 taskId；验证器内部只创建不发信号的局部临时 Draft，不新增持有服务。实际编辑器/Root采用何种独立实例和版本标识以其对应接线为准，不能把取源接口当成已上传修复。它没有上传、模式切换、ARM、任务启动或其它飞行出口。

仅源码开发及限定静态审阅，未编译、测试、执行验证 helper/认证/UI/仿真或连接设备。工程师仍需验证新源记录的正路径、旧记录拒绝、非法/超预算输入、同部署不同对账ID、取消后unknown保留、传输未排空、Episode切换、离线飞机、新草稿不污染旧记录，以及重新审阅后的独立人工审批。

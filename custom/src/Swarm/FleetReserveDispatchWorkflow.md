# FleetReserveDispatchWorkflow

## 520：本页备用机的显式原生证据刷新

519 的只读 offer 仍严格要求读取时十二项参数在 30 秒内；本节新增的是一次明确的真实采集请求，不是把旧 offer 改成 fresh，也不生成 profile、重审、批准或自动重试。继续复用当前 Episode 实际 `FleetMissionEvidenceRefresh`、原 `mission.evidence.refresh` 角色与 Session，以及原 Bridge 当前 scope，不新增 sender、权限用途或机载命令种类。

### 三个限定接口与十四字段回执

```text
requestFleetReserveDispatchEvidenceRefresh(captureToken, reserveVehicleId, true, cancellationState?)
fleetReserveDispatchEvidenceRefreshStatus(captureToken, reserveVehicleId, requestId)
cancelFleetReserveDispatchEvidenceRefresh(captureToken, reserveVehicleId, requestId)
```

start／status 严格十四键：

```text
schemaVersion, ok, error, captureToken, reserveVehicleId, requestId,
generation, current, active, status, snapshotOnly, revalidationRequired,
startupPermitGranted, flightCommandReleased
```

schemaVersion=1，四个末尾保证为 true／true／false／false。成功归属的 captureToken 和 ID 是原本页私有来源与 1..255 整数备用；requestId 是本次实际原生 UUID，generation 是原生非回绕 canonical uint64 字符串。未安装本次 attempt 的拒绝回执使用空 token／requestId／generation、ID 0、current／active=false 和空 status。cancel 返回 Bool，不接受 caller 的状态 Map 或新来源。

start.ok 只表示本次实际原生请求被接受、准确归属且返回时本页来源仍连续，不是已经得到新观测；原生正在采集时 `status.ok` 仍为 false。status.ok 表示能读到这一准确私有任务的实际或最后已观察快照，包括失败／取消的历史；是否仍是当前来源独立看 wrapper.current，采集是否仍在原 actual 对象上 active 独立看 wrapper.active。完整结果只按原 `status.state`／逐机收据解释，不能从 wrapper.ok 推断 fresh、成功响应归因或可上传。

status 原样保留当前刷新器的二十七字段：ok、requestId、generation、state、reason、active、actorId、vehicleIds、vehicles、completedVehicleCount、totalVehicleCount、currentVehicleId、currentParameter、remainingMs、requestLifetimeMs、stageTimeoutMs、requestIntervalMs、readOnlyRequestsMayRemainInflight、snapshotOnly、revalidationRequired、responseAttributionVerified、signatureVerified、hardwareIdentityAuthenticated、reviewRestored、uploadApprovalRestored、startupPermitGranted、flightCommandReleased。accepted 原生返回另有 requestAccepted=true，只在 start 私有接纳时消费，不并入 status。已知空 QString 的 reason／currentParameter 及每个原生 target 的 versionSourceEpoch／versionReceiptSequence／managerEpoch 规范为空字符串，保全部键；不补未知数值、观测或成功标志。

每次只在实际原刷新对象、预先冻结的“当前 generation 的唯一下一代”、准确请求 ID 与唯一目标 ID 全部匹配时保存脱离的 lastStatus。首次 ID 只可在原 Bridge work 的 starting 调用栈认领；该 work 的 finally 在 Bridge 结束／出版通知之前关闭窗口。尚无 ID 的早拒绝 attempt 退出后，不能再用 expected-next 从后来同 ID 的 generic 任务补认领或取消。后来的 generic 请求、同数字 ID 或另一 Episode 不能覆盖这份历史。原 actual.current 已被后来任务替换时只可展示旧 lastStatus，wrapper.current／active=false；缓存中原 status.active 是当时观察，不声称当前仍在采集或没有请求在途。lastStatus 不参与原生 gate、取消、fresh 或批准。

### 原来源、请求窗口与最终发送围栏

私有 attempt 在调用原生采集前冻结原 capture／source／task／plan、所选 ID、IntentTask／taskId、Reviewer／Store、实际 preparation 与输入内容 serial，以及原 scope／Session／角色对象和 actual refresh。scope metadata 本身不包含 captureToken，因此不能只比较 scope 或同数字 ID。换 capture、provider、输入内容代次、prepare 或 Episode 后，valid 永久 false；后来恢复同值不重认领原任务。

合法入场还要求原规划器没有当前 active／ready／运行／取消／完成中的请求、刷新器没有其它 active 任务。若存在 live review binding，必须是这个 capture 的准确 dispatch proposal／reviewContext，不能撤别人的新审阅。只有这些源值门通过后才退休此 capture 的旧 profileOffer、proposal 和 reviewContext 资格；capture 的完整规划来源和档案记录保留。后续认证失败或发送前取消不恢复旧资格；本页 canonical 拟值仍由 UI 保留供核对，不从旧完整 profileText 倒灌 hash、速度确认或原生资格。

私有 start 在原 `_fleetRemainderOperation` 票据内取得一次 Bridge 当前 lease，调用现有 private scope helper，不再调用 public start 嵌套 lease。额外 source fence 覆盖角色／Session 查询和原生当前性调用、旧上传审批撤销、Draft 本地失效的每个通知尾、Store／Intent 退休及实际采集调用前后。`_invalidateMissionDraftPreviewForScope` 将同一私有 fence 交给本地失效 helper；无私有 fence 的原 generic 调用保持旧语义。

原生每个实际请求的 authorizationFence 仍由刷新器在 outgoing plugin 之后、实际发送交接之前调用原 gate。其 SourcesMatch／Allowed 回调只对“本 attempt 正在 start 的准确下一代”或已绑定的原 requestId／generation 额外核原 source 与撤销状态；不要求当前 `_busy`／`_checking` 或 remainder ticket 为空，避免正常 native gate 自拒。后来其它 generic job 不套本页 fence；也不因本页 valid=false 获得发送权限。

可选 cancellationState 只接受本调用传入的同一个 plain `{cancelled:false}` 引用；原三参数调用兼容。true、错误容器／字段／值或读取异常均 sticky 退休本 attempt，false 不能授权。读取 caller 属性后再核原来源，防 getter 重入。UI 在还没拿到 requestId 的窗口可先置 true；该状态使原生下一次 gate／最终写 fence 拒绝，晚返回的准确 ID 仍被保留并精确取消。

cancel 只核私有 attempt 的原 captureToken／ID 和实际 requestId／generation／actual refresh，准备期 requestId 为空时只允许该原 starting attempt。它不要求已经退休的 capture 重新 current，也不为旧 Episode 取得新发送 scope：只能取消原 actual 对象上仍完全相同的一项任务，不查询共享 latest 后按机号猜 ID。取消、销毁或外层尾核失败都不是未发送证明；已有 accepted／在途／partial 收据不会写成未发生，原 readOnlyRequestsMayRemainInflight 保留。

### 原实际条件和后续人工流程

本入口保留原地面、未解锁／未飞行、真实 PX4 多旋翼、原 PRIMARY、完整参数 ready、无 pending writes、无 Mission 传输、成员占用及启动／部署冲突门。它不会为 airborne-hold 备用暗中放宽条件。实际采集仍请求版本与现有完整固定参数集合，不仅是向导的十二项；总期 120 秒、逐项 4 秒、最小请求间隔 100ms、来源检查、超时与取消均由原服务决定。协议没有为旧参数回包补 nonce，responseAttributionVerified／签名／硬件认证及所有恢复许可仍 false。

原刷新器的 observations-renewed 使用其原本证据条件，不保证结束时向导所需十二项都仍在 30 秒内。操作员必须另行明确 observe，再采用预算速度和完整新 profile、生成提案、当前审阅、采用与批准；不自动串接，不恢复旧批准、上传或 StartPermit。

本节只有源码与直接静读，未编译、运行项目／QML／JS／parser／helper／业务／认证／hash／SQL、测试、仿真、设备／PX4操作或打包。工程师仍须验证：正常地面刷新与 active／completed 区分；未授权／飞行／占用拒绝；准备期关闭／getter 重入／取消 token ABA；旧 offer／提案资格退休但本页拟值保留；同 ID 换 capture／Episode／provider；source fence 各通知尾；准确原 generation 与原对象取消；实际 accepted 后外层失效的 partial；后续 generic 覆盖只留历史；终态失败／取消显示，以及手动重读仍拒绝超 30 秒参数。

## 519：独立出勤向导的原生参数绑定

本节补实际备用独立出勤的新完整 profile 编写链，不是另一套规划器、性能声明、参数采集服务或启动许可。正常 Draft 的 editor 仍使用实际 Draft／revision；这里不创建假 Draft，不借它的成员列表或缓存，也不把备用旧 profile 自动继承到新任务。

### 显式读取本次备用机的原生参数

新入口 `observeFleetReserveDispatchProfile(captureToken, reserveVehicleId, true)` 只接受本页真实捕获 token、原冻结候选中明确选定的 1..255 整数 ID 和实际 Bool 确认。它进入原 `_fleetRemainderOperation` 票据／非回绕 serial；没有当前同 kind 的私有 capture、原 provider／Episode scope 或实际空闲门不通过时拒绝，不用 caller 的 source、参数或 observation Map 补源。

返回严格十键：

```text
ok, error, captureToken, reserveVehicleId, offerToken,
policy, observation, historicalOnly, executionPermit, flightCommandReleased
```

后三项固定 true/false/false。成功 token 为本次 `reserve-dispatch-profile-offer:<serial>`，不是权限；失败 token／captureToken 为空、ID 为 0、两个 Map 为空。合法确认通过实际来源入场后，重新读取先退休这个 capture 的旧私有 offer；新参数不可用或读取中来源变化也不能回领旧 token。非法／未确认／不属于该来源的请求不清他人的 offer。

观察操作只保私有 offer 并返回两份独立公开值副本。它不撤原 proposal、已归档 review 或实际提交事实，不修改 Draft、任务参数或机载参数。成功的 `sourceCurrent` 只来自该次原生读取时的核对，不是持续新鲜度保证。

`policy` 是实际 source.task.reservePlans 中所选 exact4 descriptor 的原 `reservePlan`，完整八键：schemaVersion、replacementVehicleId、standbyMode、standbyDurationSeconds、standbyConsumptionFactor、cruiseSpeedMetersPerSecond、climbSpeedMetersPerSecond、descentSpeedMetersPerSecond。它只展示原备用预算意图，不是当前机载参数或实测性能。向导不能为了通过门禁替换原速度、删除未知项或把原待机重演为新任务。

`observation` 直接复用 Coordinator 的 `executionProfileObservation(id)`。该原生函数从实际唯一 Vehicle、原 PRIMARY 和 ParameterManager 的原始 PARAM_VALUE 记录捕获两次完整十二项，逐外调尾核并在返回前核整组原生内存与年龄；这里不刷新、不请求、不写入任何参数。

成功 observation 精确十九键：

```text
schemaVersion, ok, vehicleId, componentId, parameterProfileHash,
observationFingerprint, managerEpoch, parameters, parameterCount,
maximumParameterAgeMs, source, sourceCurrent, allParametersUsable,
reason, reasons, hashPurpose, hardwareIdentityAuthenticated,
flightCommandReleased, executionPermitted
```

schemaVersion=1、ok=true，vehicleId 为所选 ID，componentId 为 1..255，两个 hash 为小写 SHA-256，managerEpoch 为规范 UUID。parameterCount=12、maximumParameterAgeMs=30000，source 固定 `live-param-value-receipts-only`，两个 current/usable 为 true，reason 为空字符串、reasons 为空数组。hashPurpose 固定 `operator-profile-binding-not-authorization`，末三项保证均 false。已知原生空 QString 的 Qt undefined/null 表示仅在这一成功的空 reason 字段规范为空串，不把坏数值或未知参数补成成功。

parameters 按原参数名称排序，十二项为 MIS_TAKEOFF_ALT、MPC_LAND_SPEED、MPC_TKO_SPEED、MPC_XY_CRUISE、MPC_XY_VEL_MAX、MPC_YAW_MODE、MPC_YAWRAUTO_MAX、MPC_Z_V_AUTO_UP、MPC_Z_V_AUTO_DN、MPC_Z_VEL_MAX_UP、MPC_Z_VEL_MAX_DN、NAV_ACC_RAD。实际顺序使用原名称集合的排序，而非上述文字书写顺序。

每行精确十五键：name、componentId、ageMs、source、mavParamType、contentRevision、managerEpoch、observed、available、fresh、cachedValueMatches、pendingWrite、rangeValid、usable、value。age 为 0..30000 整数，source 为 `live-param-value`，类型为 1..10 整数，revision 为正规范 uint64 字符串、epoch 与顶层相同；除 pendingWrite=false 外六个可用旗必须 true。value 必须为有限且在 JS exact 数域内的实际数值，缺值不变为 0。

### 预览重新读取并消费准确 offer

公开预览签名改为：

```text
previewFleetReserveDispatch(captureToken, request, expectedProfileOfferToken)
```

底层 request 仍严格三键 `reserveVehicleId/executionProfile/operatorConfirmed`；不向原 Planner 注入 caller observation、参数、fingerprint 或假原生标记。第三参数必须是本次明确读取实际返回的 offerToken；旧二参数调用、空 token、其它 capture／ID 或另一份新 offer 都明确拒绝。正常 single/group remainder 接口不改变。

Root 私有 offer 保实际 capture／source／task／plan／所选 descriptor 对象、原 reviewer／scope token、原预算八字段和完整原生 observation。返回给界面的 policy／observation 是另行复制，修改公开 Map 不能改私有 offer。新一次读取产生新 serial token；即使所有参数值相同也不能让旧按钮借后来 offer 续用。

合法完整请求与私有 offer 对应后，该 offer 先标为已使用，再读取同实际 reviewer 的原生 observation。当前完整十九／十五键合同必须再次成功，原 componentId／managerEpoch／parameterProfileHash／observationFingerprint 必须相同，输入完整 profile.verticalPolicy.requiredProfileHash 还必须等于该次实际参数 hash。原指纹包含源身份和参数内容代次，不只比数值 hash，因此参数 A→B→A、原 manager/PRIMARY 换代、过期、缺项、pending write、公开 Map 导入均不能替代。

一次合法预览尝试不论原生复核或后续 Planner 是否成功，都不恢复已经消费的 offer；修正后须明确重新读取。缺／旧 token 不会因为重审原 proposal 而自动出现新 offer。合法读取失败保原提案／历史结果；实际预览替换仍沿原规则退休旧 proposal／reviewContext，失败不能继续使用旧成功预览。

原生回调可通知或销毁对象，调用返回后先核 plain lifetime，再触及 Root；随后逐次核原 capture 槽、offer 对象、provider／scope 与原 policy。原 `_fleetRemainderOperation` 票据跨忙碌／出版通知保持，最终回执继续核同私有提案和 offer，源变化不伪造成功。

### 原完整消费者与安全边界不变

表单只创作完整新 executionProfile 的意图。原 `FleetMissionExecutionProfile::normalize`、备用策略速度一致性、独立单机新任务来源与完整路线、Compiler／PlanStore／PayloadBuilder／Coordinator 仍是实际消费者。新任务必须保持 relative-dwell，巡航与返航速度同原 reserve policy，爬升／下降同原 policy；与当前参数不一致仍由真实审阅拒绝，向导不替用户改预算或机载参数。

后续完整审阅仍重新取得当前能力、严格性能声明、构型基线、native 电池／SOC 窗、参数／固件、健康、空域、风险和归档依据。这个十二参数辅助 offer 不代替后续完整二十九参数／动作／动力学证据，不把声明当实测、不启用 StartPermit，不上传、不 ARM／MODE，也不接管旧组。

本节仅源码与直接静读，未编译、运行 QML／JS、parser／helper／业务／模型／认证／hash／SQL、测试、仿真、设备操作、刷机或打包。工程师仍须实际验证：19／15／8／10 形状与 Qt 空 QString；无 Draft 的真实候选读取；读中换源／销毁／scope ABA；同值新 read 的旧 token；参数同值 ABA、过期和 pending write；公开值改写隔离；一次消费失败后重读；三参预览至原完整能量／审批／StartPermit 消费，以及失败保历史事实。

## 515：保留原提案的显式重新审阅

514 的第一次审阅继续使用 `reviewFleetMissionRemainder(proposalToken, true)`；本 kind 已存在同一 capture／proposal 的实际 attempt 后，不能用该入口暗中重复提交。新入口 `rereviewFleetReserveDispatch(expected, true)` 的 expected 精确五键：`captureToken, proposalToken, reviewAttemptToken, reviewToken, requestId`，全部为实际字符串、各不超过128字符；同步或无记录失败的 reviewToken／requestId 可以为空。其余 kind 的旧入口和回执不增加 attempt 字段。

### 实际尝试与本页所有权

每次备用出勤审阅从不回绕的本地 operation serial 生成 `reserve-dispatch-review-attempt:<serial>`。真实返回收据、`fleetMissionRemainder.review` 和成功的 `fleetMissionRemainderReview` 保同一 `reviewAttemptToken`；异步同时保持实际 requestId，不把同 proposal 的后来请求或共享最新结果认作原尝试。token 仅是局部结果归属，不是角色、批准或设备证据。

Root 私有 attempt 保原 capture／proposal 对象、实际 IntentTask／taskId、完整输入值、原 prepare 对象、实际请求／审阅 token 和实际归档绑定。Intent／parameters 的内容代次不回绕；等值 A→B→A 或另一次同值 prepare 仍不算原尝试。普通批准退役造成的 state 变化不冒充参数编辑，因此旧 binding 失效后仍可保同一纯规划提案。安装自身输入时，只容许对应原 setter 的一次正常通知；嵌套内容编辑不由返回后的等值副本重新认领。

面板只能保自己真实返回且与状态一致的 attempt。隐藏／收起可以保纯 capture／proposal／attempt 锚，但须退休本页确认、采用资格和准确 pending；真正编辑、scope／controller 变化或销毁继续按原精确清理规则处理。保留值不是旧证据新鲜或批准当前的声明。

### 重审门与退休事实

重审先精确核五标识、私有原槽、原 provider／输入／prepare 归属及目标 scope；不得有当前规划 ticket、实际 active／ready／running／cancelling／completing 规划，或原部署／证据刷新／读回／启动 authority 的 busy／reservation／draining。只拒绝而不替用户取消他人工作。旧显示 planningPending 可能落后于已经取消的实际 native 请求；只有同一个真实 requestId 已终态且实际槽已不忙，才允许明确重新审阅，不把旧显示 flag 当永久忙或成功。

若当前存在 live binding，必须与本 attempt 私有保存的完整九键归档绑定完全相同；外来新 binding、另一 prepare、输入 ABA、替换 provider 或新 scope 一律拒绝，不能借旧 source 撤掉他人的新审阅。确已过期、没有 live binding 的原记录，以及没有新记录的原失败尝试可进入相同显式流程。

通过后只退休准确旧 context 的采用／批准资格，保原私有 capture 和原生生成的 proposal；然后复用同一个私有提交体进入正常 `prepareFleetIntent`，不是公开 operation 嵌套或另一套审批器。旧 context 在新 attempt 安装前精确清退，因此早期失效也不会留下旧 token 阻塞新失败尝试。原输入与 binding 在本地失效通知前后再核；原 capture／provider／prepare 的纯围栏也进入目标 scope 的每个 cancel／revoke／Store／Intent 外调前后。准确短收据覆盖三处：514 原来源捕获的 Store.revoke、515 attempt 的 Store.revoke，以及515最后 direct Intent.invalidateReview。每处只在原实际 Intent／provider 调用栈 active，finally 先退 active，再精确清同一私有槽；不是全局信号过滤。

收据把同围栏交给 archiveChanged 的实际失效处理器；仍 active 且原 IntentTask 同 target 时，stateChanged 也在推进输入代次前后核围栏，并传同收据给 invalidator，在退休 planning 前先核。514 原来源私有冻结 preparation（包括null）和内容serial；正常 approved／awaiting-approval 退休分为 before、仅在本次实际 invalidateReview 内的 invalidating，以及 after-state。中间只容原生已变preview但Root代次尚未推进的准确状态；state处理器只从invalidating推进一次，此后必须preview且原inputGeneration恰加一。完整参数、plan、provider、scope、原prepare和内容serial全程相同；同值新prepare不能借这一次正常状态过渡续用。不能等整个 Store 或 Intent 调用返回才发现本通知中已出现新prepare。不带这些准确收据的旧路径保持原行为，不增加权限或恢复旧批准。

回执的 `priorReviewRetired`／`priorRecordId` 只记录确有准确旧 context 被本次本地退休的事实；原无 context 的失败尝试不冒充退休提交。标准 committed 累积该本地退休或本次归档事实。`recordId`／`planArchiveRecorded` 和公开 `review.recordCommitted` 仍只描述本次新归档，不能用旧退休事实把新失败标成已归档。异步实际回调先保其私有原记录收据，再尝试正常出版；取消、通知重入或出版受阻不伪造回滚，也不让晚回调覆盖新 attempt。

### 新结果仍须重新采用、审批和部署

重审重新取得原正常链要求的当前能力／电量／参数／健康／空域／风险与完整新存档，不从旧档、旧 source 或 attempt 恢复。成功仍保新实际 requestId、reviewToken、archive 和独立 view17；失败保已经退休的事实及原纯规划提案，不自动采用、自动重试或恢复旧批准。新结果必须再次人工采用、申请／明确批准，才可进入原唯一共享部署区；上传、读回、启动和最终 StartPermit 原门不变。

本轮没有新增飞行命令、自动刷新／采集／文件 IO、Run 接管、设备证据认证或权限恢复。只作源码与合同静读；工程场景全部未执行，未编译、运行 QML／JS／parser／helper／模型／认证／hash／SQL、测试、仿真、设备／PX4操作、提交、PR或打包。下方514段保留为初始工作流历史，不能把其中“一次审阅”限制解释为515新显式重审入口不存在。

## 514：真实当前来源捕获与共享新任务审阅

本合同描述 [GuidedActionsController](../../res/Merivus/GuidedActionsController.qml) 的备用独立出勤工作流。它复用现有 `_fleetRemainderState` 的唯一捕获／提案／审阅槽和唯一共享部署区，不另建审批器、Run 或发送器。新 kind 固定 `reserve-dispatch`，不是 single／group remainder，也不把本机保留值标成原生 Run 来源。

前置纯值算法与完整消费者见 [ReserveDispatchPlanner](FleetReserveDispatchPlanner.md)。本工作流只把真实当前任务／计划／存档绑定形成明确保留来源，再经人工预览与一次全新普通审阅；不是自动补位、空中改派、已执行出勤、旧批准恢复或 StartPermit。

### 来源候选与按下时冻结

只读 `fleetReserveDispatchSourceCandidate` 固定九字段：`available, binding, reserveVehicleIds, sourceTaskId, planId, historicalOnly, sourceAuthenticated, flightCommandReleased, executable`。不可用时 binding／ID列表为空、两个标识为空；后三个认证／许可声明为 false，historicalOnly 为 true。available 只表示当前完整来源可读取，不表示能力、电量、位置或旧批准仍可用于新任务。

候选不接受 caller 的 task／plan。Root 从同一次私有 `_fleetIntentTask.parameters` 值副本补实际 taskId／intent，配同一个实际 `_fleetMissionReview.lastReview.plan`、完整 `_intentPlanBinding.value`，并读取实际 PlanStore 的准确 sourceTaskId／recordRevision。完整九键绑定、planId、内容摘要、证据摘要、storeEpoch、两个 generation 和实际记录的完整 plan 必须一致；不是用最新任意档案、区域草稿或 caller Map 补源。

读取前后固定实际 IntentTask／Reviewer／Store、目标 Episode scope、任务 ID／intent、原完整参数、完整 plan 和两个 generation。实际参数／plan 再读取与值比较，来源替身、输入 A→B→A、审阅替换、旧绑定、坏容器或缺完整备用列表均不能作为相同按下意图。候选仅公开完整绑定与备用 ID 摘要，不暴露这些原生 QObject 或完整私有 source。

### 明确捕获，先冻结再退休旧审阅

`captureFleetReserveDispatchSource(expectedBinding, true)` 只接当前显示的完整九键 binding 和明确确认。共享槽必须为空；不会抢占另一个 single／group／reserve-dispatch 编辑器。捕获先核本目标 scope 的原空闲门，再冻结完整实际来源，之后才退休旧草稿审阅／批准。若先由面板发编辑失效通知使原 binding 退休，则本次捕获必须拒绝，不能重新取新的绑定补救。

退休使用原本的本地失效与原目标 scope 的 cancel／revoke 路径。原 provider、任务 ID／intent、完整参数／plan、reviewGeneration 与 inputGeneration 在真正 revoke 前后再核。准确 Store.revokeReview 的 archiveChanged 正常处理器会调用 native invalidateReview；私有 plain 收据只记录这次调用确实成功，允许它带来的 approved／awaiting-approval→preview 那一次 inputGeneration 推进，不再重复要求第二次成功退休。其它输入／审阅变化、通知重入或 ABA 不借“这是撤旧”续用。失败不采用新来源，也不恢复被退休的旧批准。

成功返回原 operation 收据中的真实 captureToken，保存私有完整 `binding/task/plan` 副本。公共 `fleetMissionRemainder.capture` 显示 kind、token、来源绑定与备用 ID；其中 current 只表示本保留槽、provider 对象和目标 scope 没变。原 binding 已主动退休，因此不再要求它仍 live，也不把 current 解释为原账号批准、原设备证据、原电池或原任务执行状态当前。

原 operation 的 busy／ticket 跨所有 publication 保持，serial 不回绕。捕获不发送飞行命令。面板在隐藏／销毁期间失去本次操作，只可在真实返回后用原 controller 与准确 captureToken 精确退出，不能通过“清最新”处理他人新来源。

### 预览与完整只读核对

`previewFleetReserveDispatch(captureToken, request)` 的 request 精确三键：`reserveVehicleId, executionProfile, operatorConfirmed`，最后一项必须实际 true。来源必须是本槽的 `reserve-dispatch`；旧 single／group 入口同样明确核 kind，不能把这个 source 送进旧 single helper。提供坏值／undefined 不通过桥接删键回退；完整算法仍由实际 Coordinator 的 `buildReserveDispatchProposal` 检查。

失败替换先退休旧 proposal／reviewContext，不能继续使用旧成功预览。成功原生 result7 的完整新 task 私有保存，返回真实 proposalToken。公共 proposal 保 `token/captureToken/kind/summary/provenance`，仅该 kind 另有 `dispatchMember`，即实际 built.task.assignments 的唯一完整成员副本。面板据此分页显示自己的去返路线、完整 heading／hold、新 executionProfile 及 summary14；不从原备用简化 energyPlan 猜路线，不默认选择或自动审阅。

私有副本沿现 native JSON 边界保 Qt invalid QVariant 为显式 null；caller request 则拒 undefined，不静默删键。只用实际 Array 识别列表，不用任意 Map 的 length 键猜容器；未知有界政策不得变成空数组。副本有深度／节点／字串／容器边界，非有限或超过现精确数域的数值拒绝，原 native Planner 的整份字节与业务预算仍另行检查。复制及 Map 本身不认证来源或授予权限。

### 一次全新普通审阅与独立编码视图

第二次明确确认调用现有 `reviewFleetMissionRemainder(proposalToken, true)`。只将本私有原生生成 task 送 `prepareFleetIntent`，不加旧 fleet／证据／账号／批准。新意图标签与所有 data.review／reviewContext／公开审阅的 kind 都保 `reserve-dispatch`；当前真实能力、基线、参数、电池、健康、空域、角色、能量和风险仍由原正常审阅链取得。

异步规划只返回本次真实 requestId。pending 状态和完成的 data.review、公开 reviewContext 都保同一个 requestId／proposalToken，面板不能把同 proposal 的另一请求或共享最新结果认作自己的完成。同步完成则返回本次真实 reviewToken；预览成功也返回本次 proposalToken。超时、退出、来源或目标 scope 变化不得续期、自动重试或认领新回执。

归档成功仍先保 recordId／committed／planArchiveRecorded 的实际事实。视图从本次准确 Store.record.plan 和同一记录的 reviewEvidence 调用既有 `missionSpatialReview`，它对完整不变 plan 独立走实际 PayloadBuilder；不是调用 RemainderView 或借旧 Run 的 wire 序号。Root 核 exact3 `ok/error/view`、view17 的 planId／单机身份、snapshotOnly=true 及 onboardContentsProven／clearanceCertified／executable／flightCommandReleased=false，成功才保其独立 view。显示失败不抹已提交记录，不代表编码或批准通过。

公开审阅保实际新 plan／review／transportEncoding／独立 view。需要人工采用准确 reviewToken 后，才使用原请求审批／批准和唯一共享部署区。新 reviewContext 的绑定及 outbound scope 独立于原保留来源的编辑生命周期；正常当前证据和准确 archive binding 仍是后续门禁，不能由 capture.current、summary 或 display view 代替。

### 退出、边界与未执行验证

编辑退出按准确 captureToken 清本槽；审阅退出按准确 reviewToken 退休本审阅，保历史提交事实。取消异步只作用于本请求，不退出旧飞机 Run、不释放其占用、不回滚已发生本地归档。输入／确认／scope 变化后的旧按下、回调和对话框不能复活；面板不得自动选择、自动捕获、自动预览、自动审阅或自动批准。

所选备用生成的是独立单机新任务。数量／角色及备用0是明确新 scope，不宣称原组任务已被接替或原组保障仍成立。原共同安全／能力／能量 floor、未知政策、原边界及 clearanceInput 保持；旧净空 ID 不自动映射。原上传、独立授权、回读、启动准备和最后 StartPermit 不放宽，不发 ARM／MODE 或新增协议命令。

本轮已完整读取 ui-ux-pro-max 及 quick-reference／pro-rules，沿现 Qt/QGC 明确标签、可见反馈、可取消多步流程、键盘与纯文本规则作静态 fallback；未运行技能搜索脚本、QML／JS、应用或渲染。本合同不声称视觉、交互、构建、线程或设备验证通过。

后续工程师须实际验证：真实当前 binding 捕获与字段／provider／generation ABA；自己撤旧的一次 state 转换与重入换源；完整 Map 的 length 政策保留及 Qt null；三种 kind 隔离；新 profile 与完整路线预览；预览／同步／异步真实回执认领；取消／隐藏／销毁和同 proposal 重审；精确新记录的独立编码视图；新当前证据／审批／唯一部署区消费，以及所有原阻断继续有效。本文场景均未执行，没有编译、运行项目／parser／helper／模型／hash／认证／SQL／QML、测试、仿真、设备／PX4操作、刷机、放行、提交、PR或打包。

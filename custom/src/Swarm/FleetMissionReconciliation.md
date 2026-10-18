# FleetMissionReconciliation — 失败/取消后的独立整组只读对账

## 536：RAW对账保原NAV有限朝向并区分v3比较域

原完整raw16回执、来源、项数与Float32校验后，param4经共享 [HeadingEncoding](FleetMissionHeadingEncoding.md) 解码；不再把所有地理yaw强行置null/default。实际有限NAV原值保留，只有其唯一tick声明界内才能通过v3完整canonical；NaN默认、速度reserved-zero和非NAV限制原义保持。接纳期望载荷的v2／v3域，comparisonBasis对应canonical-v2／canonical-heading-tick-v3，不把语义相同说成原字节相同或飞行正确。

真实本次RAW读、原部署目标／binding、全组预算、取消／排空和无权限边界不变；对账不恢复上传／启动资格。仅源码静读，未执行对账／helper／算法、业务IO、编译、测试或仿真。下方为历史。

## 426：已实现的源码产品切片

操作员可在当前实际部署为 `failed` 或 `cancelled`、原传输完全排空后，明确发起一次新的整组对账。目标、顺序、期望载荷及档案绑定来自该 Deployment 的私有冻结值，不来自当前草稿、调用方 JSON、同 ID 替代飞机或旧全量 matched 收据。最多32架、每架期望2048项、全组期望8192项。本批只处理本次运行实际部署，不从重启后的离线档案恢复目标或权限。

这是 Deployment 唯一拥有的私有 QObject 子对象，只有 Deployment 可以构造和请求；没有独立 QML 注册、Episode 第15个服务或新控制权。Deployment 暴露只读 `reconciliation`，以及 `requestReconciliation(session,roles,store,swarm,authorizationOwner,expectedDeploymentId,confirmed)` / `cancelReconciliation(requestId)`。两个 Root 验源回调由真实 Deployment 成员亲自调用，实际接收者始终是原 Deployment，不把子对象伪装成其他服务。

## 新授权与原来源

使用实际 Session 当前身份及新的 `mission.reconcile.read` 角色快照；不是旧上传批准或 StartContentCheck 许可。READ 不要求第二账号批准，也不恢复任何批准。完整 fence 在每个实际发送/重试入口核身份、角色、Root 原生来源与上下文；其前后的纯原生范围检查不调用 OS/QML。身份核验可能可重入，返回后重新检查 owner、代次、源对象、角色、占用和原部署。

Session、Role、PlanStore、Swarm、authorizationOwner 必须就是原 Deployment 已捕获的实际对象。档案读取原绑定版本，不要求 `matchesBinding` 或 `liveReview=true`；只核原不可变 plan/reviewEvidence/hash 和绑定身份，再冻结本次撤销旧审批后的完整 record。运行中档案、角色或身份改变停止本次读取，不把动态 liveReview 旗标与旧上传快照直接相等而误拒正常撤销路径。

每架机固定原实际 Vehicle/MissionManager，并冻结 ParameterManager、VehicleLinkManager、component、来源 epoch 和 primary 弱指针控制块。原部署已经捕获主链路时，其原指针/控制块丢失或变化，该机记 `unknown`、不发请求，不用新链路接管。尚未开始过传输、没有捕获 primary 的 waiting 成员，可以在本次新的显式请求中冻结当时的实际主链路。运行中丢失/换源不重新绑定。

整个集合先取得本地只读传输占用。任一仍存在的原成员 ARM/flying、全局源/角色/档案改变、外国任务操作或总时限到期，停止后续读取并排空；对单机不可读、协议失败、超时或不支持的内容保留 `unknown`，不需要把其它可读机也伪装成失败。实际发包仍受 Vehicle 的真实源、已连接、地面且未解锁、无外国任务传输等原生准入保护。它不是跨站锁、远端身份认证或瞬时物理状态保证。

## 独立 RAW 结果，不借普通缓存或启动收据

顺序调用 PlanManager 的私有 `loadFleetMissionForDiagnostic`，每次冻结本地 scope、operation 和最大项数；实际 COUNT 上限为 `min(2048, 8192 - 已完整观察项数)`。达到全组观察预算后不再派发 READ，剩余机记未知。总期限120秒，每机期限按原期望项数估算且不超过总期限；重试和网络调度不保证全组能在期限内完成。

`operationStarted` 同步关联原读操作；独立完成信号只记成功/失败，排队后仍等原协议/完成栈空闲才一次性 `takeFleetMissionDiagnosticResult`。该结果槽只给本次真实子对象，不写 `_missionItems`，不发布可被 StartReview 消费的普通或启动收据。失败不保留半套 RAW 项；任何新任务操作或取消会使旧槽失效。

真正的成功 COUNT0 有完整 COUNT 与末尾 ACK 已本地排队的原生证据，显示已完整观察0项并与非空期望判为 `different/count-only`；协议失败/未观察则 `observedItemCount=-1`，绝不把空缓存或空 map 当0项。逐架成功读取不是整组原子快照，ACK 本地排队也不证明无线请求因果、远端当前内容或任务执行。

非空 RAW 原始 `MISSION_ITEM_INT` 仅按现有 v2 支持的命令/坐标/float32/default-yaw规则规范化，与原冻结完整 canonical items/hash 比较。规范化支持且完全一致为 `matched`，数量或可规范化内容不同为 `different`；同项数但有限 yaw、未知命令、未知 frame 或不支持参数则 `unknown`，不宣称所有字段变化都已判断。数量不同但无法规范化可报告 `different/count-only`，不冒称完成内容比较。没有复用425的 ordinary decoded diff，也没有逐字段差异明细或修复草稿。

## 只读显示契约

未发起时 `reconciliation={}`。发起后固定30键：`schemaVersion/requestId/runEpoch/generation/deploymentId/deploymentGeneration/binding/state/reason/active/reservationHeld/transportDraining/vehicleIds/vehicles/totalVehicleCount/matchedVehicleCount/differentVehicleCount/unknownVehicleCount/pendingVehicleCount/expectedTotalItemCount/observedTotalItemCount/remainingMs/sourceRepresentation/historicalOnly/snapshotOnly/currentOnboardContentsProven/startupPermitGranted/executionVerified/flightCommandReleased/executable`。两种 generation 分别属于子任务和原部署，均为十进制字符串；`sourceRepresentation=independent-diagnostic-raw-int-history`。接受请求额外返回 `ok=true/requestAccepted=true`，不代表对账相同。

每机固定16键：`vehicleId/state/reason/readScopeId/operationId/expectedItemCount/observedItemCount/expectedContentHash/observedContentHash/rawItemsFingerprint/countObserved/completeReadObserved/comparisonBasis/observedAtMonotonicMs/historicalOnly/currentOnboardContentsProven`。`operationId` 为十进制字符串；未观察的时间为空字符串、项数为-1。pending计数含 queued/reading；四种计数之和为总机数。全组 observedTotal 只累计完整 RAW 结果的实际项数，含完整但无法规范化的观察，不计失败的半套读取。

终态为 completed/stopped/cancelled；completed仅表示所有目标尝试结束，可以含不同或未知。已完成成员的历史结果不因后续成员失败或取消而抹掉。只保留当前一次有界结果摘要，不把完整 RAW 数组复制到部署历史；新任务或新部署不能借旧 request/代次取得权限。`historicalOnly/snapshotOnly=true`，全部当前内容/执行/起飞许可字段为 false。

## 生命周期与工程师边界

运行/检查/dispatch/占用/排空期间，Deployment 新上传、启动内容准备与启动历史入口互锁；Episode 切换检查该私有子对象忙状态，但原14服务与22字段范围合同不扩张。子任务冻结原部署终态并监听其变化：failed 变为 cancelled 即停止。原已 cancelled 时，旧 Deployment.cancel 仍按原幂等语义返回 false；活动对账使用新的 cancelReconciliation 精确取消，不把再次取消旧上传解释为新的动作。

取消先关本次原生准入，再取消准确 scope；等待原 MissionManager 和原 Vehicle 发送栈都空闲才释放占用。即使 MissionManager 先销毁也不能把仍活的 Vehicle 栈当空闲。父对象析构先销毁子对象；脱离后的清理 timer 只持有少量 Vehicle/manager QPointer 和 requestId，不持有8192项期望载荷。没有 clear、回滚、修复上传、ARM、模式切换或启动命令。

仅源码开发与限定静态读回，未编译、测试、执行 helper/OS认证/UI/仿真/设备或刷机。工程师仍需验证正常多机、不同/未知、COUNT0、失败清理、部分失败继续、8192总预算、同号对象/主链路控制块替换、完成栈嵌套事件循环、权限撤销、全组ARM变化、析构排空、其它读取/上传/启动互锁。本批不证明实际协议兼容性、飞行可靠性或可部署性。

# FleetOperationApproval：一次性双人上传审批

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestUploadApproval、approvalReceiptCurrent、consumeUploadApproval、grantCurrent及_sourcesCurrent的unchanged核实际Session同组。独立账号窗口、owner回调、Session lease消费前后及one-use提交前复核，不复活旧grant；revoke/_invalidate保留。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


本模块只审批一次**地面、未解锁的任务几何上传及独立回读**。它不审批启动，不发送 MAVLink，不解锁、不切模式、不执行任务、不清空任务，也不能撤销已写入机载端的内容。

## 实际身份与来源

`requestUploadApproval(session, role, store, owner, binding)` 只接收实际 `FleetOperatorSession`、`FleetRolePolicy`、`FleetMissionPlanStore` 与同线程控制器对象。主操作者必须由原生会话验证，其 SID 和角色必须与当前本地权限快照完全一致，角色仅为 `operator`、`commander` 或 `administrator`。本地权限必须允许 `mission.upload` 并仍要求额外审批；修改本地角色或把 `requiresSecondApproval` 设为 false 都不是身份来源，也不能绕过本模块。

主操作者身份不由本模块或 JSON 声明产生。本模块调用实际原生会话的 `authenticateApprover`，获得与主操作者不同 SID 的已认证审批者；审批者必须由会话核验为 `safety_officer`、`commander` 或 `administrator`。这是两项不同的操作系统账户身份，不单独证明确有两位不同自然人，账户分配、凭据保密与人员职责隔离仍需组织落实。本地角色模型中 `actorAuthenticated=false` 保持原义，它与实际会话的 `authenticated=true` 是两种不同来源，不得相互替代。

原生会话负责安全凭据处理、当地角色来源和身份有效性。本模块既不接收明文口令，也不保存身份凭据。Windows 原生凭据窗口只会在以后用户主动请求时由会话打开；本轮开发没有运行该窗口、任何身份 API 或软件。

## 绑定、有效期与重入

审批冻结七项身份：`sourceTaskId`、`recordId`、`recordRevision`、`planId`、`contentHash`、`reviewEvidenceHash`、`storeEpoch`；如输入含 `reviewGeneration` / `inputGeneration`，两项必须成对且一并冻结。字段类型、长度、哈希格式、数值范围均显式限制，不接受额外嵌套调用者元数据。

审批同时冻结实际来源 QObject 指针、主身份完整收据、本地权限完整收据、原生档案完整记录和目标机号。`bindingHash` 是规范绑定、该完整原生记录及 schema 2 授权条件的 SHA-256，包括计划、负载编码摘要、审阅来源证据、上传专属用途、最多 240 秒与必须保持当前原生操作者身份；哈希不是签名，不是文件来源认证或机载任务身份。

控制器必须通过 `fleetOperationApprovalSourcesMatch(actualUploadApproval, session, role, store)` 返回真实布尔 true，确认这些是自己的私有来源，并通过 `fleetDeploymentBindingAllowed(actualDeploymentOrUploadApproval, binding)` 重新核对当前计划、审批、占用等上下文。它们是同进程协作边界，不是对恶意原生插件、被篡改二进制或已被控制的进程的安全沙箱。

每次授权判定都在控制器回调前后复核原生身份、本地权限、档案、额外审批、指针、代数和有效期，防止回调或原生认证模态窗口内的变更漏过。Windows 原生会话新鲜度查询可能分派已发送的窗口消息，因此即便校验返回 true，也必须在之后重新核对 QPointer、代数、状态和冻结来源，防止锁屏/注销通知、撤销或对象删除发生在校验内部。消费过程中禁止嵌套替换请求。认证窗口中的会话 busy 通知暂缓处理，但认证返回后必须完整复核；注销、重登录、删除来源、修改角色、修改或恢复档案都不能保留旧审批。

只保留一项当前审批及其一项已消费授权。代数用不回绕十进制字符串，审批 UUID 和实例 epoch 不复用。**审批请求 60 秒**从请求冻结时开始，包括凭据输入、用户确认和消费。必须在该期限内一次性消费，未消费审批不能延期。

消费时才生成**同一部署专属、最多 240 秒的有限上传租约**，并由实际原生会话进一步限制为主操作者原始五分钟登录期限的剩余时间；不会重新登录、续签主身份或刷新登录截止时间。两个阶段是审批者明确看到并授权的条件，不是把过期审批重新恢复。原生审批凭据在消费时烧掉，旧 60 秒审批者凭据之后到期不再单独终止租约，但操作者原会话、Windows 活跃未锁定会话、两个 SID 的当前角色、计划与其全部原生来源仍必须持续有效。

原生会话和审批模块的单调时钟不能直接跨对象比较。模块用调用原生消费**之前**自己的时间加上原生返回的实际期限，扣除调用耗时、保守形成自己的固定截止时间；原生会话同时按它自己的原始会话截止时间验证。二者取严格交集，不发生延期。240 秒到达或主身份先到期时拒绝之后的步骤，由部署排空，不把未完成传输标成成功；慢链路/大机组能否在期限内完成仍需工程师验证。

公开 `approvalRequestRemainingMs`、`uploadLeaseRemainingMs` 分别展示两阶段；`remainingMs` 依据当前阶段。`uploadLease` 只公开部署 ID、grant ID、最大时长、实际时长、模块截止时间、剩余展示值、地面上传范围和 `renewed=false`，不公开完整原生身份租约。剩余时长不会每秒单独触发通知，界面需自己的只读刷新。只读 `current` 是状态快照，不是权限证明，消费者仍必须调用原生校验。

## 一次消费和继续验证

`approvalReceiptCurrent(session, role, store, owner, binding, roleReceipt, identityReceipt, approvalReceipt)` 可用于私有控制器确认前复核。JSON 复制的收据自身不产生审批，仅当它严格匹配此实际对象当前内存中的已批准收据且全部原生来源仍有效时才通过。布尔与字符串采用明确类型检查，不能用数字 1 或文本 `true` 替代。schema、时间和绑定修订等数值字段只接收真正的数值 QVariant（Int / UInt / LongLong / ULongLong / Double），必须是有限的精确整数、位于规定范围且不超过 `2^53-1`；拒绝 Bool、String、小数和非有限值。Qt 5 的 QML / JSON 数字往返可能把整数 Double 变为 Int，因此在显式类型与数值检查后允许两者的等值匹配，不要求仅为 Double。

原生 `consumeUploadApproval(..., deploymentId)` 只接受部署模块在本次上传之前生成的规范 UUID。它在任何发送前调用实际会话的 `consumeUploadIdentityLease`，烧掉新鲜的独立审批身份，将状态切成 `consumed` 并产生一个不可复用的 `grantId`。grant 固定绑定该部署 ID、实际原生身份租约、完整审批和固定截止时间，不能转给另一部署、计划、目标、传输内容或启动动作。

成功消费不发同步 `changed`，以避免在部署门禁调用栈内递归失效；它排队一次延后通知，只有实际代数、consumed 状态、有效期和完整 grant 仍匹配时才发布，防止界面停留在未消费状态。租约生成或最终收口失败同样排队发布失败。后续过期、撤销或来源变化也会通知。原生 `grantCurrent(grant, deploymentId)` 在每一步上传/回读前重新核对同一部署 ID、实际原生 `uploadIdentityLeaseCurrent`、双方实时角色、原始主身份和其他来源，不延期、不恢复审批、不调用传输。仅清理已消费的历史审批者凭据或暂时 busy 通知不会盲目撤销仍然有效的原生租约；注销、身份代数变化、锁屏、失活、角色丢失及实际租约失效仍必须拒绝。

尚有效的已消费 grant 禁止被新的审批请求暗中替换；必须明确撤销或等其过期。失效/撤销会按确切 leaseId 调用原生会话撤销租约，不撤销任何其他租约，不保留可被后续请求复用的旧原生租约。未消费审批仍可被新请求替换。

同一收据不能开始第二次上传。撤销已消费审批阻止之后的继续步骤，但已发出的原生传输仍按部署模块的排空规则处理，**不回滚、不自动清空任务**。本模块不持久化审批、会话或 grant，重启/档案恢复必须重新登录、审批及确认。

公开审批收据为 schemaVersion 2、固定 22 字段（包含 `uploadLeaseMaximumMs=240000` 和真正布尔 `uploadLeaseRequiresCurrentOperator=true`）；原生已消费 grant 为 schemaVersion 2、固定 13 字段。旧 schema 1 的 20 字段审批和 8 字段 grant 不再兼容、不能恢复授权。原生会话身份及原生身份租约各自保持其独立 schema 1，由实际会话核验。两阶段始终 `startupPermitGranted=false`、`flightCommandReleased=false`。用于显示的机号列表不证明链路、机型、健康或机载任务身份；部署模块仍独立核对这些条件。

## 工程师后续验证（本轮未执行）

- 验证 Windows 原生登录、角色组、不同 SID 审批、取消、超时、注销/角色撤销、被删来源和非 Windows 拒绝路径。
- 在凭据窗口、owner 回调和确认窗口期间变更计划、权限、会话及机体状态，检查旧收据拒绝。
- 验证请求 60 秒拒绝迟到消费、原生租约最多 240 秒且不超过原始五分钟主身份截止、凭据消费不可重放、不同部署 UUID 拒绝、数值严格类型、防重入、固定期限不延期及已有传输排空。
- 验证已消费的历史审批凭据超过 60 秒不误撤销有效租约，但主身份到期、锁屏/失活、双方角色变化、计划/来源变化、显式撤销及固定租约到期均阻断继续。
- 编译、运行及硬件验证由工程师另行完成。本模块的静态实现不是实飞放行，也不是任务启动或完成证明。

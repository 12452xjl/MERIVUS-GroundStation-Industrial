# 新地面再审独立账号审批（仅源码静态收口）

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestApproval、consumeApproval、grantCurrent、transferToGroundStartup及_sourcesCurrent核实际Session/previousTxn/Deployment/Review同组；pure authenticationConsumeContextCurrent和向实际Authority移交也检查，OS/owner回调与预burn/搬移后复核，revoke/清理保留。只移除primary Link的GUI线程误拒。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


这是一份新的条件启动意图审批，不是旧认证/旧启动恢复，不是飞行许可、上传租约或 ARM/MODE 发送器。没有执行认证窗口、业务 API、编译、测试、仿真、刷机、飞控修改、PR 或打包。后续实际模式消费者、完整 HOME/本地参考处理和飞行放行仍未由本模块完成。

## 实际来源与权限

`requestApproval(actualSources, reviewId, operatorConfirmed)` 要求 exact9 个真实 QObject：`session/roles/store/previousStartup/deployment/swarm/health/authorizationOwner/groundReview`；强类型、同线程、实际 Review 返回的私有原始对象及当前车辆 Vehicle/MM/PM/VLM/PRIMARY 全部核对，不接受 JSON 身份、同编号新飞机或恢复档案替代。

输入必须是 actual `FleetMissionArmedGroundReview` acknowledged 的完整20字段 context，原新 RAW 完整 INT 回读已匹配、全组不能缩小，armed/disarmed 相位逐机固定。完整非恢复最新 Store 记录字节、任务绑定、全 risk artifact、Catalog/Coordinator/Registry/FaultPolicy 和实际来源冻结；旧技术未验证项不筛掉、不改成 true。主身份必须真实 native Session 当前身份且与原 reviewed 主身份完全同；RolePolicy 用现有 `mission.start`，operator/commander/administrator、allowed 和 requiresSecondApproval 都必须为真实 Bool true，旧 role snapshot 的 actorAuthenticated=false 不是主身份认证证据。

私有 owner 需返回严格 actual Bool：`fleetGroundStartApprovalSourcesMatch(actualApproval, actualSources9)` 以及 `fleetGroundStartApprovalContextAllowed(actualGroundApproval, approvalId, reviewId, binding, orderedIds)`。每次有效门禁重新核完整 native Review20、原记录和实际9来源、完整新读来源以及各机 fresh on-ground/POSCTL/nonAUTO/nonHIL/not-flying、cached armed 与新 RAW 一致且与原 reviewed armed 相位相同。

## 新独立账号与期限

专用 Session action 是 `mission.start.ground.reassess.approve`。真实 native `authenticateApprover` 使用源派生摘要和绑定 hash，不接受调用者填写的“签字”、SID、身份声明或已保存审批。必须不同真实 SID，审批者为实际 safety_officer/commander/administrator；摘要明确已有 armed 数量、此次新完整回读，以及 HOME/local 分支、RC 中立、净空、硬件认证、远端 atomic snapshot 仍未验证。

已完成源事实在本人 secondary credential modal 内只以 actual native `identitySnapshot()==frozen` 校验主身份（仍查 SID/role/WTS/原登录期限）；不靠 busy scalar，也不盲目因该 modal 的 Session.changed 失效。modal 后重新捕获全部对象/Review/角色/记录/原期限；真实主身份、来源或风险变化仍拒绝。真正 consume/current proof 必须 modal 已结束。

期限来自新 Review 最早 READ START 的剩余 ≤30秒，审批提示、OS检查、读取和消费都消耗这份原预算，点击已阅、消费或领取 grant 不能续期。跨单调时钟只传剩余并保守扣全调用耗时，不比较绝对时间；主/次身份原5分钟/60秒期限若更早仍限制可用性。

## 一次消费与隐私

公开 `current` / schema1 的27字段 `approvalReceipt` 不含 SID、原始 RAW、完整 risk body 或 native authentication proof，只有审批/Review UUID、任务绑定、范围、hash、期限和 false 许可边界。它们是 `snapshotOnly/revalidationRequired` 状态，不是可执行证明。

审批成功后的同步 changed 仍在请求栈内，busy 为 true；另排一次严格 generation/approvalId/state=approved 且已 idle 的 queued changed，使 QML 不长期缓存旧忙碌状态。该通知不重新验证/延长权限，也不是发送门禁。

native-only `consumeApproval(actualSources9, exactReceipt, consumerIdUUID)` 只允许 approved 当前记忆状态一次消费。Session 专用 `consumeGroundStartApprovalAuthentication` 得到22字段 proof，并强制 actual ledger/approvalEpoch/reviewId/reviewEpoch/bindingHash。`authenticationConsumeContextCurrent(...)` 是纯内存/实际对象/固定消费预算回调，不递归查询 Session/Review/OS；Session 自己前后查真实主/次身份与 WTS，Ledger 在调用前后完整全源门禁和新 native proof current 复核。固定 `maximumRemainingMs` 必须等于本次 entry 参数，不与之后递减 remaining 比较造成自拒。

成功先提交 consumed 状态，再下一个 event-loop turn 按 generation/approvalId/grantId 发布通知；有效消费或门禁内不同步 emit。native-only14字段 grant 含私有 Session proof、consumer UUID、完整审批 scope、原期限；`grantCurrent(grant, consumerId)` 每次重核真实源和当前 proof，不续期。当前 expiry 额外保守扣整段 consume 耗时，可能重复扣前半 OS 成本而早拒，不是性能实测；未来消费者必须从本账本当前缩短后的剩余预算派生，不能用原公开 receipt 的较晚 expiry 续期。消费后仍有效的 grant 不被新的 request 静默替换，需精确撤销或原期限失效；撤销/替换/销毁清对应 actual owner/id/epoch proof，不清其它旧 Start/Authority/Txn 身份。

所有 `startupPermitGranted/executable/flightCommandReleased/previousAuthorizationRestored` 均 false；没有真实 Mode/ARM/起飞/回滚/自动 DISARM。新条件审批保留技术风险，并不证明城市巡逻安全、任务执行、任务完成或硬件真实性。工程师接手后仍需实际编译和协议/仿真/硬件验证，不能拿源码审阅当放行。

## 一次移交给独立地面授权对象（260+）

native-only `transferToGroundStartup(actualGroundAuthority, exactGrant14, startupId, sourceHash, maximumRemainingMs)` 只接受真实 `FleetMissionGroundStartupAuthority`、同线程、完全相同已 consumed 原生 grant、grant.consumerId==startupId、canonical UUID/hash 和 ≤30秒 entry 预算。先 `grantCurrent`，再完整旧 source/risk 门禁和 actual Authority 的纯四参数 owner-context；不递归调用新 Authority 的完整门禁。

Session `transferGroundStartApprovalAuthentication` 一次退休旧 ground22，转为强 actual Authority/epoch/startupId/sourceHash 的28字段 proof。Session 成功 wrapper6 的 `remainingMs` 是全部实际 native 检查后的剩余；28的历史 `durationMs` 不是新预算，不能拿它重新起时钟。Ledger 始终 cap 自己 entry 时的缩短期限，再保守扣整个调用耗时，最终 wrapper10 带 actual新proof/UUID/epoch/hash/remaining 和四个 false 边界。不发包、不创建远端 control lease或新审批期限。

真实 Session 移交成功后立即 `state=transferred`、清旧 ground22/grant、停止 timer 并断开旧 Review/源观察，再检查新 owner/native proof。之后旧 Review 自己过期不会盲撤新 owner；旧 grant 不能再 consume/current，新 request 不静默覆盖 transferred 状态。若 post-handoff 失败，精确撤新 owner/startupId/epoch proof，不恢复旧22。原对象销毁只撤自己的旧 proof，不触及已移交新实例。

显式 `revokeApproval(oldApprovalId)` 在 transferred 状态仅向保存的实际 owner 转发：actual pointer、startupId、authorityEpoch、sourceHash 和旧 generation/id 全部相同才调用 `revokeAuthority`。只有该 exact owner 已 revoked/expired/stale/failed，且 reservationHeld/transportDraining 都真实 false，或实际 owner 已销毁，才退休旧 reference；不覆盖 replacement，不释放别人的占用，不制造新 grant。新 GroundStartupAuthority 初期仍固定地面 armed 相位且没有模式续作；完整后续模式消费者与路线图仍未完成。

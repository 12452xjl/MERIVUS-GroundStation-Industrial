# 原生 owner 回调：实际调用对象契约

## 371–375：专用切换调用不改变原owner回调合同

[本地组管理](FleetMissionLifecyclePanel.md)新增两项产品入口和native `ForController`同步transition，原owner回调首参/22键scope合同不变。private stackpermit贯穿原owner/ID调用与实际变更；普通legacy/public end不能旁路解锁。变更前核旧scope，提交后核新scope，不能用旧scope自然失配否认已提交事实，也不能把普通调用scope改写为新组。

UI/Guided忙状态只用于新请求admission，pure来源回调不把正在执行的transition当自拒依据；现有fullgate/QQ/Episode/账号/来源与期限检查继续。prepare只分配新本地对象组，不切selected、不取消旧Run或释占用、不迁移身份批准。已发布返回先本地失效与身份协调，后续getter失败不抹原事实。selected改变的旧Close取消仍携issued原收据路由，不按UUID或新alias找替代。

实际8源码/19文档，限定源码交叉审阅不等于运行验证；未编译、测试、运行helper/应用/认证或设备、更新PR或打包。`outboundIntegrationComplete=false`、完整25目标和六类实质缺口继续。下方366–370及更早是历史背景，不覆盖当前已新增入口/API的事实。

## 366–370：24来源回调固定当前组或实际retained Close

24指来源/上下文callback，仍保留供原生调用的public函数签名并走原scope helper；private的是raw对象id及桥，不是所有函数。30指当前业务入口，另2个身份 `runCurrent` 与4个观察 `runObservation` 不并入此数。

nested draft后的finally仅撤原scope意图；不能核实则私有 `retiredCurrentScope`持续拒同组current入口/回调，不超时复活，selected观察仍可读。原生scope不包含该业务失效旗标或授权证明。Qt exact Int/QObjectStar往返/方法/parent/销毁尚未运行验证，不放宽合同；本批五源码须连累计完整源码/pri/注册/依赖交接。

[Guided](../../res/Merivus/GuidedActionsController.qml)已接私有 factory/14 actual 引用、30个 Fleet 入口和24个来源/上下文回调。原C++实际 producer 首参合同不变；QML从原生 scope 对应槽位核原对象，普通来源只认当前组，Close只按提供的实际 workflow 查原 retained 组，不按UUID、角色文本或同binding找替代。DeploymentBindingAllowed 两个合法实际 producer、GroundApproval 对真实 GroundReview peer 的复用保持。

回调保留协作来源/上下文检查，不另 acquire controller-call，也不以 scope替代 full gate；pure原生引用匹配本身不调用OS，外层 native call/prompt 正在运行不是其自拒原因。实际业务仍须在回调前后核QQ/Episode/原来源/身份与完整门禁，schema/TTL不变。嵌套草稿改动使 active current stale，后置失败不抹已 queue/可能发生或未知事实。plain lifetime/finally 只清原owner+ID，不靠owner死亡、超时或当前组替换解锁。

selected 四值同 scope、token 含选择代次；关闭取消携完整 issued receipt 走原 retained 实际对象，显式监视登录可撤当前未消费批准但不结束 Run。两类注册与初始构造已接，无自动 prepare/legacy fallback/公开rawrefs；新部署/选择产品入口及专用 transition API 仍缺，`outboundIntegrationComplete=false`。本批5源码/18文档，已完成限定文本审阅，不是调用绑定运行或安全通过；未编译、测试、执行业务或更新PR/包，完整25目标/六类继续。以下回调表保留历史合同背景。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 356–360 源码边界

本轮修改12个原生服务的20处 Direct 回调调用点，以及 `GuidedActionsController.qml` 对应19个回调函数签名。每次调用的首参数是当前原生 QPointer 所指的实际服务 QObject，不是调用者提供的系统 ID、部署 UUID、binding 或导入 map。

控制器用 `_fleetNativeCallbackReceiverMatches(actualReceiver, expectedReceiver)` 先拒绝空对象，再作严格实际对象相等检查；Context 回调在进入业务检查前和成功返回前核对应实际服务。新参数来自原服务的 `self` / `guard`，原生侧原有回调前后 QPointer、native Episode/同组、代次、冻结来源和完整授权检查保留。QML 中的相等检查不是类型认证、账号认证、机载身份认证或飞行许可，不能替代这些原生门禁。

## 本轮变更的私有回调签名

表中实参顺序必须与 C++ 和 QML 同步；同名函数不会回退接受旧的少参数形式。

| 回调 | 新参数顺序 |
| --- | --- |
| fleetDeploymentAuthorizationSourceMatches | deployment, roles |
| fleetDeploymentBindingAllowed | actualDeploymentOrUploadApproval, binding |
| fleetOperationApprovalSourcesMatch | uploadApproval, session, roles, store |
| fleetEvidenceRefreshSourcesMatch | refresh, session, roles |
| fleetEvidenceRefreshAllowed | refresh, ids |
| fleetStartContentSourcesMatch | contentCheck, session, roles, store, deployment, swarm |
| fleetStartContentContextAllowed | contentCheck, checkId, binding, ids |
| fleetStartReviewContentSourceMatches | startReview, contentCheck |
| fleetStartReviewAuthorizationSourceMatches | startReview, roles |
| fleetStartReviewContextAllowed | startReview, deploymentId, binding |
| fleetStartApprovalSourcesMatch | startApproval, session, roles, store, startReview, deployment |
| fleetStartApprovalContextAllowed | startApproval, context |
| fleetStartupAuthorityContextAllowed | startupAuthority, startupId, binding, ids |
| fleetArmedGroundReviewContextAllowed | groundReview, reviewId, previousId, binding, ids |
| fleetGroundStartApprovalContextAllowed | groundApproval, approvalId, reviewId, binding, ids |
| fleetGroundStartupAuthorityContextAllowed | groundAuthority, startupId, binding, ids |
| fleetStartupTransactionContextAllowed | initialTransaction, startupId, binding, ids |
| fleetGroundStartupTransactionContextAllowed | actualGroundTransaction, startupId, binding, ids |
| fleetObservationCloseContextAllowed | actualCloseWorkflow, context |

20处 C++ 调用对应19签名：Content 的 SourcesMatch 有两处；DeploymentBindingAllowed 有 Deployment 和 UploadApproval 两个真实生产者；Txn 同一条件调用点可选择普通/ground 两个方法名称。两类 Authority 以及 Review、GroundApproval、Txn、Close 已有的 `SourcesMatch(actualSelf, bundle)` 不重复加首参，现有 bundle 的字段数和内容不变。

## 本地预检查与原生回调分开

`fleetDeploymentBindingAllowed` 只接受当前实际 Deployment 或当前实际 UploadApproval 两种合法生产者，进入前固定是哪一种，返回前仍核同一实际生产者。不能用当前审批对象替代原 Deployment，或由另一任务提供相同 binding 来跳过来源核验。

控制器内部五处上传预检查改用 `_fleetDeploymentBindingCurrent(binding)`，五处刷新/其它工作流的共享目标预检查改用 `_fleetEvidenceRefreshTargetsAllowed(ids)`。它们只保留原业务检查，不伪装一个真实原生回调，不颁发 native scope、凭据或许可。原生回调才携真实生产者进入独立包装函数。

GroundApproval 复用 ground-review 的上下文检查时，显式传真实 GroundReview peer，而非 Approval 自己；之后再复核原 Approval。原独立 RAW、底层逐包 scope、批准凭据、超时、磁盘/receipt schema 均未改变。

Close 回调只新增实际 Workflow 核验，没有新增 outbound-current、在线/地面遥测或飞行许可要求。当前应用仍固定单链；将来保留旧 Episode 时，必须按实际 Close 找其原组并投影原 Session/Series，不能把这次实际对象核验误当该历史接线已经完成。

## 同批交接与工程验证

12个原生 `.cc` 和修改后的 [GuidedActionsController.qml](../../res/Merivus/GuidedActionsController.qml) 必须同批交接。仅更新一端会因 Qt 参数数目不匹配而拒绝，或因旧回调缺少真实调用者而拒绝；不增加旧合同 fallback 来假装兼容。源码清单见[补丁清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定审阅见[静态审计](../../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

留给工程师且本机未执行的场景：正常20调用点/两个 Txn 分支、缺首参/空调用者/另一实例、相同 binding 或账号的不同组、回调期间来源/代次变化、两个合法 upload 生产者、GroundApproval→真实 Review peer、旧 Close 仍可按原服务审批，以及 C++/QML 混版必须拒绝。需实际编译/Qt/QML/身份/生命周期验证，文本参数数量一致不证明运行调用成功。

完整 [Lifecycle 应用接线](FleetMissionDeploymentLifecycle.md) 仍未完成：Guided 的14个实际对象尚未改成 current Episode 引用，调用前后 Episode/epoch/服务冻结、共享 Role 明确身份意图、全 retained modal 忙锁、selected 旧 Run/Series/Close/Session 一组投影和新部署/历史观察入口继续开发。没有新类型登记、界面布局或交互变化，`outboundIntegrationComplete=false` 保留。

本机仅源码/文档及限定文本、人工源码审阅；不编译、测试、执行 helper/认证/应用/SQL/日志/仿真/设备或刷机，不改 PX4、更新 PR 或打包。原25目标/六类、最终 StartPermit/净空/正式远端结果与长期运营工作不因本轮完成。

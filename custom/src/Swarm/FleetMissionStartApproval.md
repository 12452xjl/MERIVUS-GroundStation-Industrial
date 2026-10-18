# FleetMissionStartApproval：条件性启动意图审批，不是启动许可

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestStartApproval、consumeStartApproval、grantCurrent、transferToStartup及_sourcesCurrent核实际Session/Review/Deployment同组；向实际StartupAuthority移交还单独pair，Session消费/搬移前后与transferred尾部再核。已烧或退休证明不恢复，revokeCurrent保留。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


该模块记录两项实际原生账户身份对一份**已确认阅读的独立启动风险快照**的条件性人工意图。它不是 dispatcher、启动许可或执行授权，不发送命令，不解锁、不切模式、不上传、不启动任务，也不产生上传租约。所有输出始终 `startupPermitGranted=false`、`executable=false`、`flightCommandReleased=false`，用途固定 `conditional-start-intent-approval-not-dispatch`。

## 实际来源与风险边界

`requestStartApproval(session, role, store, startReview, deployment, owner, reviewId)` 只接受实际同线程原生对象。主操作者必须是当前原生认证的 `operator` / `commander` / `administrator`，SID、角色与实际 `mission.start` 权限快照一致，并且本地策略仍要求额外审批。操作员改写昵称、角色、authenticated 或第二审批标志不能形成身份或绕过该链路。

独立原生会话的凭据窗口必须返回与主操作者不同 SID 的真实 `safety_officer` / `commander` / `administrator`。这是不同操作系统账户，不单独证明不同自然人；人员职责分离、账户分配和口令保密由组织保障。凭据不进入本模块或日志。账本不初始化管理员、不自动赋权，也不兼容上传动作的审批或租约。

实际 `FleetMissionStartReview::approvalContext` 必须是同一实际部署生成、尚有效、已明确 acknowledged 的不可变风险 artifact，阅读者 SID / 角色与当前主身份一致。复核 `approvalContextCurrent` 每次重新采集真实来源并比较原快照；不能从历史文件、JSON 声明或“已勾选”替代来源。审批主动作是 `mission.start`，被阅读快照的原角色动作仍是 `mission.start.review`，二者分别验证实际权限策略。

可以记录带有技术阻断、未知或未证实事项的**条件性意图**，不要求 `allPreconditionsMatch=true`。它保留原 artifact 的全部 blocked/unverified、原始 `secondApprovalEvidencePresent=false` 等捕获时事实，不把新账户审批写回 artifact、不移除技术缺口、不使旧 intent/planning/upload 审批复活。UI 需要同时展示原风险与“账户审批已记录，仍未允许启动”，不能显示“可飞”或“已放行”。

## 冻结与原始期限

冻结实际 session、role、PlanStore、StartReview、Deployment、控制器六个 QObject，完整不可变最新非恢复 PlanStore 记录，七项原记录绑定（及实际成对 input/review generations），部署 UUID / 代数，review UUID / 实例 epoch / 代数，整组有序真实目标 ID、原风险角色、artifact 和 source fingerprint。不能用同数字机号的新对象接替。

另外通过实际部署 getters 冻结每架 Vehicle、MissionManager、ParameterManager、VehicleLinkManager、有效 defaultComponentId，以及实际 PRIMARY Link 或原快照对应的明确缺失。Vehicle/MM/PM/VLM 必须存在且归属、线程一致，任务传输必须空闲。缺失 PRIMARY 是未解决风险，不是可飞；恢复、替换或移除链路需要新审阅。在凭据窗口前后、每个来源 gate 与消费时验证这些实际对象，不能只比较序列化数值。

StartReview 原生上下文固定 17 字段，其中只把 `remainingLifetimeMs` 排除在稳定身份哈希/精确比较之外；其他字段包含完整原 artifact，不能删去姿态、位置、参数、健康、角色或风险以让过期确认通过。`reviewContextHash` 是实际稳定上下文哈希；审批身份 `bindingHash` 还绑定完整实际档案记录的 SHA-256 和非执行用途。哈希不是签名，不证明硬件/固件认证。

人工审批最多 60 秒，包含捕获、凭据窗口、确认和消费；但本模块始终受**原风险审阅 30 秒截止时间和原主身份/审批身份期限**的更早限制。账本在读取原 Review 之前记录自己的单调时间，加上实际返回的剩余 Review 时间保守计算截止，不直接比较不同对象的时钟。凭据完成、published、消费或重复 current 查询都不能重开 30 秒或续签账户。

`approvalContextCurrent` 不只检查本地 nonce 尚未过期，还复核源证据年龄和实际状态；因此 30 秒不是保证所有风险源新鲜 30 秒。真实来源变化需要新审阅，不能静默替换新 pose 或恢复旧审批。

## 来源 fence 与生命周期

控制器必须用 DirectConnection 返回真正 bool true：

- `fleetStartApprovalSourcesMatch(actualStartApproval, actualSession, actualRole, actualStore, actualStartReview, actualDeployment)`：这些必须是此控制器的实际私有对象。
- `fleetStartApprovalContextAllowed(actualStartApproval, actualContext)`：同一 acknowledged Review、部署、最新非恢复档案及整组目标，不存在上传/排空/占用/其他任务冲突。允许记录条件性风险，但不提供执行权限。

在这些回调前后重新验证实际原生身份、角色、Review、档案、部署及全部目标。Windows 原生会话检查可能分派锁屏/注销窗口消息，返回 true 后仍要检查 QPointer、代数、阶段和冻结来源；控制器回调也可能撤销或销毁对象。凭据窗口的 session busy 通知仅在本次 pending 请求中暂缓，返回后必须完整复核；已批准/消费意图遇到新凭据窗口或身份失效不能保持有效批准。

会话无关的未使用审批身份清理不盲目破坏仍然相同的主身份，但实际 proof、主身份、Review 或角色过期必定在原生 gate 拒绝。source current 快照本身不是持续权限证明；消费者必须调用实际原生验证。此账本没有对恶意原生插件、已被控制进程或篡改二进制的沙箱保证。

## 一次消费（仍不 dispatch）

原生 `consumeStartApproval(session, role, store, startReview, deployment, owner, approvalReceipt, startupId)` 必须严格匹配此实际账本的当前 34 字段批准收据，且 startupId 是本次独立原生工作流生成的规范 UUID。完整来源 gate 通过后，实际 Session 的 `consumeStartApprovalAuthentication` 烧掉确切的新鲜 start 审批身份并产生它自己的 16 字段原生证明；不能把 pending 上传审批或 caller JSON 当 start 身份证明。

本模块生成条件性 13 字段 grant，固定 startup UUID、原批准收据、实际原生账户证明、原 Review 哈希与剩余原期限；期限只可能缩短，不续期。`grantCurrent(grant, startupId)` 还验证调用者提供的嵌套账户证明的真实类型及实际 Session 保留状态，避免 QVariant bool/数字等值转换冒充 schema/时间。双方当前 OS 角色、未锁定活跃主会话、原 Review 与全部 native 来源依然需要通过。

批准/消费都是人工意图记录，不是 “StartPermit”。本账本没有 ARM、起始序号执行绑定、模式切换、MISSION_START 或任何失败回滚代码。其后续实际 StartupAuthority 现在接管有限身份与独立新读/物理来源，但仍不发送指令；最终启动授权和执行事务仍要另外实现。

一个收据只消费一次。尚有效的已消费 grant 不允许新请求暗中覆盖，须明确撤销；原短 Review 已过期的替换、销毁或失败，会按确切 approvalId 撤销仍可能活到原 60 秒的 Session proof，不能孤立残留阻塞下一请求。原生消费/验证中若 WTS 回调删除账本，仍通过捕获的 Session QPointer 和已复制 approvalId 清理刚生成证明。成功消费只排队发布状态，不在门禁栈同步通知；过期、撤销或来源变化不恢复权限。所有批准、证明、grant 不持久化、不从文件恢复。

## 实际单次接管（仍不授予启动许可）

原生 `transferToStartup(actualAuthority, exactConditionalGrant, startupId, sourceHash, maximumRemainingMs)` 只接受真实强类型同线程 `FleetMissionStartupAuthority`，本账本实际未过期 consumed grant、原启动 UUID、实际 Authority full-source 哈希与 `1..30000` 毫秒剩余上限。所有借用字符串、完整 grant 及 native 弱指针先冻结，再进入可能分派 WTS/owner 回调的验证，返回后重新核 QPointer、代际、阶段、旧 grant、主身份、实例 epoch 和原期限。

转移前先核 `grantCurrent` 与完整 `_sourcesCurrent`，并调用 Authority 的原生 `authenticationTransferContextCurrent(startupId, sourceHash, actualThisLedger, exactGrant)`：实际 Authority 必须已处于此同一准备阶段、持有此实际账本的完整 grant、固定 full-source 哈希和自身原期限，且独立技术 gate 成立。不是由界面传一个 `allowed=true` 或 receipt JSON 取得接管。

在 OS 验证之前固定本账本 entry 时刻和 `min(旧 expiresAtMs, entry + maximumRemainingMs)`；Session 原生转移只可进一步缩短原主/审批身份及审阅期限。其成功生成 22 字段 owner-bound 身份依据并退休旧 proof 后，本账本立即置 `transferred`，停止原 timer、断开旧 Review/Content/Session 状态 watcher，清空旧私有 authentication 与 grant。随后再核实际新 Session proof 和实际 Authority transfer-context；任何回调撤销、销毁或预算耗尽都只按同一 actual owner/startupId/epoch 撤销刚转移的依据，返回失败，不复活旧 grant。

成功 wrapper 恰好九字段：`ok`、`authenticationReceipt`、`startupId`、`authorityEpoch`、`sourceHash`、`remainingMs`、`startupPermitGranted=false`、`executable=false`、`flightCommandReleased=false`。`remainingMs` 是原本地截止减去全部验证耗时，并受 Session duration 减整段调用耗时的更保守上限约束；Authority 从调用前时刻设自己的截止，不能直接比较不同对象单调钟基准或从返回重新计时。账本只排队发接管完成通知，不在当前门禁栈同步通知。

`current` 显示 `authenticationTransferred`、`transferredStartupId` 与 `oneUseConsumed`；转移后 `conditionalIntentApproved=false`，`grantCurrent` 必定拒绝旧 grant。旧 Review/Content 因后续预期 ARM 变化而失效不能撤掉新 owner 的账户依据；真正身份/角色/WTS/owner/原 TTL 仍由 Session 和实际 Authority 独立核验。**本批 Authority 仍 preARM-only，不是跨 ARM 派发实现**，不能把账户依据稳定误称为车辆可飞或启动许可。

存活的 transferred 状态拒绝新审批覆盖。取消优先直接调用实际 Authority 的 `revokeAuthority(exactStartupId)`。用户显式 `revokeCurrent(exactOldApprovalId)` 也可以转发到保存的实际 Authority，但先后都要求同一真实弱指针、实例 epoch、startupId、full-source 哈希与旧账本代际；只在同一 Authority 的确切 terminal 状态且 `reservationHeld=false`，或实际 owner 已销毁时退休旧账本。若 owner 已被复用为另一 startup，不发送取消、不清其新依据，报告需要实际 Startup 取消。此路径不恢复旧 grant；新审批仍必须从新 acknowledged Review 和完整当前来源重新开始。自动旧 Review 失效没有转发取消能力。

实际完整新读来源另有 `PlanManager::fleetMissionReadReceiptRetainedSourceMatches(primary, exactReceipt34)`，只对照私有实际 31 字段、新读 scope/operation/counter、同一 actual PRIMARY/Vehicle 版本 epoch、缓存 fingerprint 与原 read-start 时间；不以当前 armed/landed 判定内容身份，也不依赖完成后已失效的发送 fence。它严格递归核类型，排除仅有展示用途的 age/available/fresh，不从 caller age 得到续期；新的读/写/source/缓存变化或当前 RAW receive stack 均拒绝。原 grounded `fleetMissionReadReceiptSourceMatches` 保持不变；实际 Startup owner 必须另核阶段，不能把 passive receipt 当发送许可。顺序回读仍非原子、无请求因果/硬件认证、无执行/完成证明。

## 类型与资源

schema/时间/修订/机号只接受真正数值 QVariant（Int / UInt / LongLong / ULongLong / Double）的有限精确整数，位于各自范围且不超过 `2^53-1`；Bool、String、小数及非有限值拒绝。Qt 5 的整数 Int / Double 往返允许规范等值，但认证/许可布尔和字符串始终明确类型。UUID、正十进制 counter、lowercase SHA-256、最多 32 架唯一机号及字段集合均有界。

公开批准收据 schema 1 固定 34 字段，原生条件 grant schema 1 固定 13 字段；原生 Session 条件 proof 独立 schema 1 固定 16 字段，接管身份 proof 固定 22 字段，wrapper 固定九字段。内部 Review context 最多 3 MiB、完整原生档案最多 32 MiB，递归深度 18 / 两百万节点。UI current 只显示有界记录绑定、账户 SID、目标、期限、接管去向和最多 32 条各 256 字符的原风险代码，不输出完整计划、风险 artifact、身份口令或完整 proof。

## 工程师后续验证（本轮未执行）

- 独立不同账户认证、同 SID 拒绝、角色权限变化、凭据窗口取消/超时、锁屏/注销/失活及非 Windows 拒绝。
- 原 Review 30 秒、账户原期限、跨对象单调钟保守截止，批准/消费不续期，真实位置/姿态/参数/健康变化要求新审阅。
- 窗口和 owner 回调中的取消、替换对象/PRIMARY、重入/删除、原记录归档恢复、任务传输/占用冲突及旧 ID 拒绝。
- 条件性有技术风险时仍保留全部缺口、所有输出无 StartPermit/executable/dispatch，start 与 upload 审批不能互换。
- 一次消费、proof 原生状态、已过期本地意图的原生证明精确清理、析构/回调删除清理、严格类型与有界 UI 显示。
- 单次 owner-bound 接管，前后独立 native gates、借用参数/回调删除/替换、原时限只缩短、旧 grant 退休、显式确切 Startup 取消及 owner 复用拒绝；新/旧回读 validator 的阶段差异、原 read-start 年龄及非原子边界。
- 编译、运行、仿真、链路/硬件和发布验证由工程师另行执行，本轮未运行任何业务、身份 API、测试或构建。

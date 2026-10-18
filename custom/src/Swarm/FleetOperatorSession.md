# FleetOperatorSession：本地 Windows 身份与独立审批身份

## 509：单流启停专用 typed 双账号证明

封闭审批 purpose 新增 `fleet.video.stream.control.approve`，只供实际私有 FleetCameraStreamControl owner 消费。OS 提示明确仅所审阅单流的 STOP／START 与状态回读，可能在飞机飞行中移除操作员视频；无自动重试、恢复或回滚，不代表实际送流／停流、无线因果、带宽或飞行许可。申请 SID 和审批 SID 必须不同，审批账号仍须真实 safety_officer／commander／administrator 身份与当前角色。

新 family 的 consume／live-current／pure-consumed-current／revoke／clear 均为私有 typed owner 接口，与 interval、上传、启动、能力／构型审批分开。consume 在原 owner／Session／purpose／hash／主身份／待用独立批准两侧核验 OS 与原来源，先烧待用批准再核纯尾；失败不恢复。公开 Session 值合同不扩字段，不生成可序列化或可恢复 lease。

专用证明期限取原 Control Review 剩余时间、主身份和独立批准三个原期限的最早值，不因弹窗、消费或后续 native Run 重新计30秒。live-current复核实际账号／角色／工作站；pure方法只核原对象、身份、purpose、hash和期限，不调用 OS、Root或派新请求。Session换代／注销／工作站撤权经原 advanceSession 清理新 family；到期或 owner 销毁由原 expiry 路径清理，精确 revoke 只清原 owner／hash。

正常本次审批提示期间的角色显示保持由实际 Root／Control 原意图判定，不能把 busy 当新授权；真正换源／注销仍退休。不同 SID 不等于两名自然人的证明。详见[控制合同](FleetCameraStreamControl.md)和[工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本阶段只修改源码／文档与直接静态阅读，未执行 OS 认证、编译、测试、仿真或硬件操作。

## 502：可见面板沿现有真实认证

新[Panel](../../res/Merivus/FleetOptionalTelemetryPanel.md)仅显式调用Root.authenticateFleetOperator，配置独立批准仍经原501专用purpose与typed owner proof。界面仅在内存冻结必要身份5／actorId（可含SID）并只读显示，未持久化SID，未保存密码或可重用认证证明；勾选与本页receipt不替OS／角色／原TTL核验。Session源码与公开合同不改，实际提示、WTS、辅助技术及认证行为留工程师验证。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501：地面遥测配置专用typed双账号证明

精确新增fleet.telemetry.interval.configure.approve，专用OS提示展示机号、三原间隔、统一目标、原期限及不自动回滚。只准实际[Configuration](FleetOptionalTelemetryConfiguration.md)owner私有消费；绑定原primary／approver16／hash、当前Episode、不同SID与原角色、原TTL，不能借上传／启动／能力／基线purpose。

consume在OS核验两侧核原owner／原值，并先烧待用批准后作纯尾核；失败不恢复。私有proof期限取原服务剩余／主身份／二审最早值，审批不续Capture原30秒；live方法复核当前OS账户／SAM角色／WTS，pure尾方法只核原内存关联和期限，不调用Root或OS。Session换代／到期／精确revoke清proof，不生成公共lease。

Root在真正正常二审busy期间只保原身份的Role显示，不新授权或续期；真正退出／锁屏／变更仍退休。不同SID不是两名自然人证明，现有Session公开合同不扩字段。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 484：性能声明专用审批用途

实际Review的approve-performance只选 `fleet.performance.declaration.approve`。Session的封闭action集、专用OS提示、typed consume和capabilityReviewConsumedIdentityCurrent尾核均显式支持该独立用途，从实际owner读取，不接受caller自行改名的用途。原能力／基线凭据不可互用，也不生成上传／启动lease。

申请者maintenance／administrator、不同SID的合格独立审批者、原身份TTL、一次消费和原Review30秒不续期。失败不保证凭据尚未消耗，不恢复／重试已消费批准；最终Registry围栏仍只核原内存身份和owner，不查询OS或Root。提示明确两工程数字和完整caps一次替换、本机构型基线不变、非实测性能认证、无机载写入／飞行命令。

旧四操作与current30／result16保持，新preview3/20由Review管理；原8键approvalEvidence保存实际独立purpose。工程验证全部未执行，未运行OS认证／UI、编译或测试。下方为历史。

## 421：能力与构型审批的精确用途隔离

实际 [FleetCapabilityReview](FleetCapabilityReview.md)的冻结 operation 决定私有 `authenticationPurpose()`：能力两操作使用 `fleet.capability.review.approve`，基线两操作使用 `fleet.configuration.review.approve`。Session 仅接受这两个精确值，并从真实 owner 读取，不由 caller 自报 purpose。专用 OS 提示明确仅改变本机基线、不采纳能力、不写机载、不放行飞行。

typed consume 在每次 OS 检查两侧、一次消费后继续核原用途和 owner/Session 关联。最终 `capabilityReviewConsumedIdentityCurrent(primary, approver, actualOwner)` 只读实际内存身份和用途、不同 SID、原 TTL 与已消费状态；不再从固定能力字符串推导所有操作，不查询 OS、不调用 Root、不生成 lease。Review 原30秒仍额外限制，失败不自动重试或续期。

8键 approvalEvidence.action 保存实际批准用途；原 current30/result16/preview2/18 不扩字段。两种用途不可互用，旧 Review/Registry bool 兼容入口不是企业全入口身份隔离。下方418仅能力用途的说明由本节扩展；本轮未运行认证、UI、编译或测试。

## 418：专用能力采纳/撤销用途

[FleetCapabilityReview](FleetCapabilityReview.md)通过private friend typed consumeCapabilityReviewAuthentication消费精确fleet.capability.review.approve。每次OS检查两侧核原Session、Review、身份与binding；不同SID、原Episode和TTL不续期，一次清待用批准，不产生lease或changed。

capabilityReviewConsumedIdentityCurrent只核原内存身份、用途、账号关系和期限；最后Registry围栏不调用OS/Root。全局approver上限仍60秒，但本Review原30秒截止额外限制全流程。返回false不保证批准未被消耗，不自动重试。共享Review归Swarm而非Episode14个对象，实际Session仍须当前Episode允许。

账号关联以8键approvalEvidence保存为本地结果值，无密码/可复用凭据；不认证硬件/性能或放行飞行。未执行认证或测试。

## 417：独立目录重载用途

[目录工作流](FleetFirmwareCatalogReload.md)通过私有 friend typed `consumeFirmwareCatalogReloadAuthentication` 消费精确 `firmware.catalog.reload.approve`。每次 OS 身份检查两侧核原 Session／Reload／身份／政策／绑定；申请者与审批者 SID 不同，原 TTL 不续期。一次清除待用审批，不产生 lease、不发 changed、不复活失败凭据。

最后 `firmwareCatalogReloadConsumedIdentityCurrent` 仅核内存、当前 Session 所属 Episode、原账号身份和期限，不查询 OS 或调用 Root。共享 Reload 本身不被误要求归属于 Episode 的14个对象。实际请求角色 maintenance/admin 仍由 Reload 独立约束；用途不能冒充固件认证或飞行许可。未运行认证或测试。

## 411–415：恢复界面使用原Session，不扩权或续期

[恢复面板与Root合同](FleetOccupancyRecoveryPanel.md)固定当前actual Session/Role和独立fleet.occupancy.recover.approve；ForController模式保留实际Root，source callback不触发Session.current或OS查询，最终身份消费/原TTL尾栅不调用QML。身份重认证前精确取消原恢复意图，不把旧审阅带到新身份或Episode。

两个actor显示字段在尚未建立账号时可为invalid QVariant，Root只在固定27键显示shape把这两项规范为空；核心identity/审批/证据未知仍拒。source token只含稳定原身份，不含prompt busy/剩余年龄，防止自己的独立账号窗口因显示变化而自取消；窗口耗时仍计原生30秒与原Session TTL，失败不自动重试。

旧requestReview为可调用兼容入口，不称C++-only安全隔离；产品UI只用6参ForController，UI receipt8不授权限。仅源码审阅，未运行认证/界面、编译或测试；全25目标/六类继续。

## 406–410 原生恢复当前边界

[恢复复核合同](FleetOccupancyRecovery.md)：唯一新增fleet.occupancy.recover.approve，不享retained Close的旧Episode例外。实际Review私有fence一次烧身份，不生成lease；纯occupancyRecoveryConsumedIdentityCurrent在源回调后核原Session身份/epoch/revision和两原TTL，不重认证延期。最终Review再实时核当前Episode/Role。其它purpose不可互用，现场处置仅为人工声明。本批无新操作UI、远端空闲证明或完整自愈交付；仅源码和限定静态读回，未编译、测试或运行。

## 371–375：部署切换不迁移账号或批准

[本地组管理入口](FleetMissionLifecyclePanel.md)和两项专用原生transition已写入源码。prepare贯穿原生begin/私有栈permit/新scope/end，新组不继承旧Session身份、已消费proof或批准，不自动切走selected；旧Run和占用保持。Guided对已返回的发布/选择事实先作本地草稿失效及身份意图协调，再读后续scope；失败或未知尾部不恢复旧凭据，不从新alias取消旧工作。

选择历史监视只改selected，A→B→A也清旧界面确认；需要身份时仍显式针对该实际Session认证，不把同账号、UUID或任务binding视为跨代授权。全进程身份提示锁及原Session用途/schema/最早期限不变；普通pure来源回调不因transition busy自拒。`outboundIntegrationComplete=false`及完整25目标/六类仍保留。

本批8源码/19文档仅作源码与限定文本审阅，未运行OS认证、应用/helper、编译、测试、设备、PR更新或打包，不是身份系统或飞行安全验收。以下366–370及更早段为历史状态，旧“入口/API仍缺”不覆盖本批新增。

## 366–370：实际组身份意图与外层调用占用

selected四值投影先做同scope Session的OS查询，再配对纯Series/Run并末核，不声称原子快照。nested draft撤销不能核实时私有 `retiredCurrentScope`持续拒同组current入口/回调，但selected观察可读，不超时复活或结束Run。22键Int/QObjectStar往返、方法绑定与销毁尚未运行验证；交接须累计完整源码/pri/注册/原生依赖，不能只拿五源码增量。

Session 原生认证/凭据 schema、用途、TTL及全进程提示栈未在本批扩权。[Guided](../../res/Merivus/GuidedActionsController.qml)已由私有 Lifecycle/14引用取得实际 Session，并把 shared Role 身份意图绑定到选择的 actual Session+scope；OS返回后不自动采纳替代 current/selected Session，来源失效只撤界面 actor，不恢复批准或结束旧 Run。显式 selected 监视登录可能使当前组未消费批准失效，不能把同账号/同binding当跨代授权。

三个[原生 controller-call](FleetMissionControllerScope.md)保持整段同步调用的原 owner/ID，prompt 活着时不能提前 end；finally 只清原槽，owner死亡不自动放锁。pure scope/来源回调及原身份/fullgate不因 call/prompt busy 自拒，busy 也不是权限。nested draft 标记 active current stale；返回检查失败不回滚已 queue/未知事实。选中四值同 scope，完整 Close 收据取消走原 retained 实际对象，不按 UUID 搜索。

两类注册及现有工作流接线已写入源码，initial 由构造提供，无 legacy fallback 或公开 raw refs；新部署/历史选择入口及专用 transition 原生 API 仍缺，`outboundIntegrationComplete=false`，25目标/六类继续。5源码/18文档已完成限定文本审阅，不是身份系统运行或安全通过；未运行OS认证、helper、应用、编译或测试，未更新PR/包。下方是历史增量。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 345：Episode 的非 Close 身份用途边界（源码接线）

非Close的`authenticateApprover`/`approverSnapshotCurrent`及全部upload/start/ground凭据消费、移交、Current证明方法使用[Episode原生来源门禁](FleetMissionDeploymentLifecycle.md)。已归组Session必须是当前实际组，superseded、owner丢失、reparent、Series evidenceLost或未知来源拒绝；不得因parent改变退回legacy。带实际Approval/Authority/Txn的方法逐对调用`outboundOwnersCompatible`：双方真正legacy才沿用旧行为；任一managed则双方必须managed、属于同一实际owner且仍为current，不能跨工厂或靠同ID/binding混用。

OS身份/WTS/SAM和实际owner回调前后，及一次凭据烧毁/移交前、connect后的实际持有者与返回前，都重新核这一纯native边界。原有receipt shape、purpose、SID隔离、完整scope和最早期限不变；查询不续期，失败不恢复已烧或退休的proof，只精确清理本次已写proof而不清replacement。

不对Session整类封禁：`authenticateOperator`、`identitySnapshot`/`identitySnapshotCurrent`、公开current、普通身份事实、logout/WTS撤销、精确revoke/clear/expiry以及`task.observation.close.approve`与private即时Close消费保留原门禁，旧组可重新认证以结束自己真实监视。Close用途不能消费为upload/start；它仍须实际Close owner、两账号、原review/角色和截止时间，不是绕过身份校验。

本步仅接身份侧，其余12类owned新请求/审批/发送门禁与Guided、旧观察选择身份作用域仍待完整开发；Lifecycle仍未注册QML、没有新部署界面，不能宣称已实现跨部署全链隔离。只读源码、修改与限定文本/同行审阅；未执行登录、OS接口、业务/helper、编译、测试、应用、仿真或设备操作。

## 独立 purpose：结束本地执行监视（2026-10-15）

新增精确审批动作 `task.observation.close.approve`，与界面/角色权限 `task.observation.close` 对应。它只批准结束当前实际 Run 的本地监视并释放本次本地占用；飞机可能仍在飞行，不发送 STOP、LAND、DISARM，不停止机载任务，不设置 `Completed`，也不认证任务成功或源连续性。Windows 独立账号提示直接用英文重述这些边界，展示实际冻结操作摘要、绑定 SHA-256 和申请人 SID，并要求不同的审批账号。旧 upload/start/ground 提示文本和用途不变。

`authenticateApprover()` 的精确白名单现在为 `mission.upload`、`mission.start`、`mission.start.ground.reassess.approve`、`task.observation.close.approve`。仍只有一个待消费的真实审批身份，沿用既有 16 键 shape、不同 `TokenUser` SID、审批账号仅 safety_officer/commander/administrator、原主登录最多 5 分钟与原审批窗口最多 60 秒的约束；新用途不创建新的 Session 收据 schema。打开新审批窗口依然退休旧待消费身份，不能把四种动作的收据互换。

新增消费入口 `consumeObservationCloseAuthentication(primary11, approver16, actualOwner, bindingHash)` 是 **private**，只由 friend `FleetMissionObservationClose` 的当前即时流程调用，不是 QML 或通用认证入口。强类型实际 owner 和 Session 必须持续存活且在同一应用主线程。函数先独立持有全部传入值，冻结实际 Session epoch/revision、主身份、待消费审批身份和原申请人身份；在入口及原生身份/WTS/SAM 查询之后反复调用实际 owner 的 `authenticationBindingCurrent()`。这个 friend 检查器只比较 workflow 当前私有冻结来源/范围，不再查询 OS、不发通知、不递归认证；caller 的哈希、布尔值或导入 JSON 不能代替实际 owner。

函数使用现有 `identitySnapshotCurrent()` / `approverSnapshotCurrent()` 即时核验本对象实际身份、不同 SID、当前直接 SAM 角色、活动未锁定 WTS 和原期限；每次可能重入的系统查询后，重新核精确 owner、Session epoch/revision 和全部原身份状态，查询耗时不续期。精确 action 必须是 `task.observation.close.approve`，binding 必须是同一小写 SHA-256；旧上传/启动/ground 身份消费接口仍拒绝该动作。

核验通过后不可逆清空 `_approverReceipt` / `_approvedApplicantIdentity`，再完成只读 owner fence 和原截止时间核对；最终核对失败也不复活已烧掉的凭据。仅返回本次调用的 bool，不保留 close 租约、proof、grant、恢复记录或新身份副本，不修改旧 upload/start/ground 的已消费依据，不调用 `advanceSession()`，不发同步 `changed`。工作站撤销仍可能在烧掉前的 WTS 查询中发出独立通知；新 workflow 必须保护自己的重入与对象存活，并在消费后立即沿已冻结的原生结束监视流程处理，不能把 true 存起来用于后续操作。

结束监视的稳定 Run/任务/完整绑定/占用范围、第二账号策略、短期审阅窗口、一次确认 token 与实际排空门禁由独立 workflow 维护。Source 已失效不等于当前账号有效或可恢复旧启动批准；当前有效两账号只可结束自己实际 owner 绑定的监视，不可据此改写机载状态、历史来源证明或完成率。本批只修改源码并作限定文本核对，未执行系统认证、业务接口、编译、测试或设备操作。

新增 GroundStartupAuthority 到实际 StartupTransaction 的独立一次移交：原 ground28 与 original ground22、原两账号身份保存在专用私有仓；新 proof30 增加 `transactionEpoch` 和 `transactionAuthenticationId`，用途固定 `ground-startup-transaction-authentication-not-permit`，不覆盖旧非 ground proof24。Session 先冻结 entry+原剩余预算，再从实际 Authority 的 pure pending context getter 获取原生冻结 context27，并调实际 Transaction 的 pure4 ownership callback；身份和 WTS/SAM 查询后再次核这一实际待移交范围。成功仅退休旧 ground28，不改主会话或重启原 READ/审批期限；当前核验不再依赖已退休 Authority/Review 门禁。仅源码和文本静态审阅，未运行这些接口。

260–264 独立GroundStartup一次移交：actual GroundStartApproval/原22与actual GroundStartupAuthority/新UUID/epoch/sourceHash通过pure4arg核对，Session新28保存原22和原两身份；旧22退休与新owner分仓、不advanceSession。Current/revoke/锁定/退出/death/过期核实际SID/role/WTS和原更早期限，精准旧scope不清replacement。移交返回6字段的remainingMs在所有查询后计算；历史duration仍expiry-oldconsumedAt，不能当“现在剩余”重新续期。原预算先冻结，仅缩短，无飞行指令/最终StartPermit；仅源码，未调用系统或设备接口/编译/测试。

255–259 新增独立 ground-reassess 条件认证：专用 action `mission.start.ground.reassess.approve`、实际强 GroundStartApproval owner/approvalEpoch、reviewId/reviewEpoch/bindingHash 和一次 authenticationId。新 proof 22、wrapper 5，保存真实两账号/SID/原会话与独立认证期限，在第一次系统来源检查前固定原剩余预算，并只取更早截止；不续期、不覆盖或恢复旧 Start/Authority/Transaction 证明。提示未结束不能消费/current 验证；锁定/注销/断连、owner 死亡与过期按 exact owner/ID/epoch/authenticationId 撤销，不误清 replacement。仅源码实现与静态契约，不表示已运行 Windows 凭据/WTS 接口或获得启动/飞行许可。

本模块只提供地面站本进程中的操作员身份来源、一次独立审批身份收据、有限的上传身份租约、单次消费的启动意图认证依据，以及转移到真实原生 StartupAuthority/StartupTransaction 的有界身份依据，不发送 MAVLink，不上传任务，不解锁、不启动，也不创建或修改 Windows 用户、组或权限。启动意图认证与接管依据都不是启动许可、启动租约或飞行授权。代码尚未编译、运行或测试；以下为源码契约，不是认证系统已经通过验收的声明。

当前 244–249 批次：已增加真实强类型 Transaction 的一次性身份接管，24 字段私有来源证明、精确撤销及原期限/角色/WTS/owner 清理；Authority 的十字段接管结果与原 25 字段上下文、Vehicle 的 32 字段 RAW 传输事实是独立载体，不是可以互换的身份许可。没有续期、通用 JSON 恢复或隐式飞行授权。

## 身份来源与角色

默认无身份、无审批收据。只有用户显式调用 `authenticateOperator()` 才打开 Windows 安全桌面凭据窗口。使用 `CredUIPromptForWindowsCredentialsW` 收集凭据后，经 `LogonUserW(LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT)` 真正验证，取成功令牌的 `TokenUser` SID 作为 `actorId`。用户名和显示名不是授权依据。此过程不继承运行进程当前用户身份，也不把 Windows Administrators 自动映射为产品管理员。

角色由本机 SAM 中下列固定组的**直接 SID 成员关系**决定。应由组织的 Windows 管理员在部署阶段配置，本模块没有组创建、成员添加、提权或自助角色编辑接口。

| Windows 本地组 | 产品角色 |
| --- | --- |
| `MERIVUS_Fleet_Observer` | `observer` |
| `MERIVUS_Fleet_Planner` | `planner` |
| `MERIVUS_Fleet_Operator` | `operator` |
| `MERIVUS_Fleet_SafetyOfficer` | `safety_officer` |
| `MERIVUS_Fleet_Maintenance` | `maintenance` |
| `MERIVUS_Fleet_Commander` | `commander` |
| `MERIVUS_Fleet_Administrator` | `administrator` |

必须恰好命中一个角色组；命中多个组不选择“最高权限”，而是拒绝。没有直接命中、任何非“组不存在”的枚举错误、分页未完整返回、超过 4096 个成员或非法 SID 都拒绝。缺少某个角色组本身不授予权限。域组、其他本地组的嵌套成员不展开，因此仅加入域组并不能获得产品角色。

每次 `identitySnapshotCurrent()`、`approverSnapshotCurrent()`、上传租约与启动意图认证依据的原生核验，都重新读取本机固定组，以当前直接成员关系核对冻结角色。每组一次 `NetLocalGroupGetMembers(nullptr, group, 0, ..., 256 KiB, ..., nullptr)`；不持有可续读枚举句柄，不接受 `ERROR_MORE_DATA` 的前缀数据，所有返回缓冲区都释放。SAM 是本机管理员控制的信任根，不是地面站可编辑的 JSON 配置。

## 收据与接口

`current` 是只读状态，始终含 `identityReceipt`，未登录/失效时为 `{}`。还包含 `busy`、`platformSupported`、`workstationMonitorAvailable`、`authenticated`、`state`、`reason` 和 `uploadIdentityLease`。`platformSupported` 表示 Windows 构建分支存在，不能代替实际 Win10+ 监视窗口注册成功和实时会话检查；`workstationMonitorAvailable` 仅报告实际监视窗口是否保留并已注册，不声称会话当前未锁定。`uploadIdentityLease` 是本对象保留的租约来源副本，不是仅靠界面读到此字段就认定仍可继续上传的许可；原生消费者必须再次调用租约核验。所有结果的 `flightCommandReleased` 均为 `false`。

公开的原生/QML 接口：

- `authenticateOperator()`：显式认证；成功为 `{ok:true, identityReceipt:...}`，失败为 `{ok:false, reason:..., identityReceipt:{}}`。
- `logout()`：立即改变会话 epoch/revision，清除主身份、独立审批收据、上传身份租约、启动意图认证依据和已转移的 Authority/Transaction 身份依据；在凭据窗口期间注销也使窗口返回结果失效。
- `identitySnapshot()`：返回当前有效的原生收据副本；获取失败为 `{}`。独立审批窗口期间可供界面只读显示主身份，但不允许并发授权。
- `identitySnapshotCurrent(receipt)`：严格校验本对象实际收据、类型、epoch/revision、有效期及实时角色；凭据窗口期间始终拒绝授权。

主身份收据恰好 11 个字段：`schemaVersion`、`sessionId`、`sessionEpoch`、`sessionRevision`、`actorId`、`displayName`、`role`、`authenticationSource`、`authenticated`、`issuedAtMonotonicMs`、`expiresAtMonotonicMs`。三个数字字段原生发布为 Double 整数值，但 Qt 5 QML 可以把往返后的 JS 整数保存为 QVariant::Int；校验只接受实际 Int/UInt/LongLong/ULongLong/Double 数字类型，值必须有限、整型且处于 `0..2^53-1`。严格检查后才把这三个字段规范成 Double，与本对象的实际收据完整比较。Bool/String 不能转换成数字许可；字符串字段实际为 String，`authenticated` 实际为 Bool。`sessionRevision` 为不回绕的十进制字符串，`sessionId` 与 epoch 为随机 UUID。`authenticationSource` 固定为 `windows-logon-local-role-groups`。SID 最多 128 个 UTF-8 字节，以满足现有角色模型的身份字段约束；超长 SID 拒绝而非截断。显示名最多 256 字节，字符串拒绝控制/格式字符及不能无损 UTF-8 往返的 UTF-16。独立审批身份的三个数字字段采用同样规则。

主会话最多 5 分钟，从开始打开认证窗口前计时，窗口耗时不延长有效期。时间使用本对象单调时钟，不是 UTC、跨进程时间戳或可恢复会话。应用重启不恢复登录/审批状态。

仅供原生审批服务调用的接口：

- `authenticateApprover(action, bindingHash, applicantIdentity, operationSummary)`：精确白名单为上文列出的四种动作，不接受其他动作或通用命令范围。必须传入仍有效的实际主身份、小写 64 位十六进制 SHA-256 绑定值以及由原生审批服务从实际冻结操作派生的摘要，没有默认摘要或调用者凭空填写的界面选项。计划操作摘要至少包含记录/版本、计划与目标机范围；结束监视摘要需说明实际 Run/本地占用范围与“飞机仍可能飞行”的风险，用于帮助审批人理解具体操作。重新打开独立 Windows 凭据窗口，展示摘要、绑定哈希和申请人 SID。上传提示明确不授权解锁/启动，以及上传身份租约最多 4 分钟且受操作员原有登录到期时间限制。启动提示明确是“独立审阅后的启动意图批准；后续受控启动可能解锁/起飞，但本调用不发送任何飞行指令；不是上传授权/租约”，不声称存在 4 分钟启动租约。摘要最多 600 个 UTF-16 码元/1800 字节 UTF-8，拒绝不能无损往返的文本、NUL、格式字符和换行 LF 以外的控制字符；整段提示最多 1024 码元且不超过 SDK 的 `CREDUI_MAX_MESSAGE_LENGTH`。只有与申请人不同的 `TokenUser` SID 且唯一角色为安全员、总指挥或产品管理员才接受。
- `approverSnapshotCurrent(receipt)`：核对本对象保留的实际审批身份收据、仍有效的完整主身份、不同 SID、实时审批角色、同一 epoch/revision 和有效期；不修改状态、不发信号。

审批收据包含主收据相同的 11 个身份字段及 `action`、`bindingHash`、`applicantSessionId`、`applicantSessionRevision`、`applicantActorId`，恰好 16 个字段。审批身份的 `sessionId` 是独立 nonce，并非主会话 ID。四个独立动作共享最多一个待消费审批身份，后续任一审批提示开始时撤销旧的待消费收据。有效期最多 60 秒，包含凭据窗口耗时，且不得晚于主会话到期。各消费接口分别精确核对动作，不得交叉使用同一收据。它不是可复制、可恢复或跨动作的通用执行令牌。

所有认证入口限制在 QObject 所属的应用主线程；凭据窗口可产生嵌套事件，因而有忙状态、`QPointer`、会话 epoch/revision 和窗口返回后重新核验。审批认证在任何原生查询前复制操作字符串、绑定哈希、完整申请人收据和摘要；上传租约和启动意图消费同样复制 ID、绑定值和身份收据，防止调用方传入成员引用后在系统回调/模态窗口中销毁，而本身份对象仍存活时读取悬空引用。稳定的有效校验路径是 const，不主动发 `changed` 信号；独立 Windows 会话消息可以在系统调用期间撤销来源并发通知，因此入口、返回、调用方和定时器都核对对象存活及冻结来源。一秒定时器，在凭据窗口外重新读取主身份/待消费审批身份/已消费启动意图认证依据的直接角色成员关系，在窗口期间仍执行到期和已消费上传租约的实际角色/会话核验；主身份不再有效时改变 epoch/revision 并清空全部凭据来源，其他来源不再有效时只清空对应来源并通知。定时器不延长 TTL。主线程忙或系统调用延迟可能延后界面刷新，实际操作门禁仍须独立即时核验。凭据窗口期间的 `changed` 必须由审批服务按其冻结状态处理，窗口退出后必须重新采集计划、角色和主会话，不能靠“窗口成功返回”恢复旧许可。

## 单次消费的上传身份租约

仅原生审批服务可以调用 `consumeUploadIdentityLease(primary, approver, leaseId, maximumDurationMs)`，不是 QML 入口。消费时再次核验实际主身份、实际未过期独立审批身份、精确 `mission.upload` 动作、当前角色与工作站会话；`mission.start` 审批身份明确拒绝。`leaseId` 必须是规范 UUID，时长为 `1..240000` 毫秒。同一本对象只保留一个实际租约，仍有效的旧租约必须先由所属审批服务撤销，不能通过新消费偷偷替换。成功将本对象的待消费审批收据**不可逆地消费并清空**，返回 `{ok:true, identityLease:..., flightCommandReleased:false}`，不在消费函数里发同步 `changed`，避免正在创建实际授权的审批账本重入。

租约恰好 16 个字段：`schemaVersion`、`leaseId`、`sessionEpoch`、`sessionRevision`、`primarySessionId`、`primaryActorId`、`primaryRole`、`approverActorId`、`approverRole`、`approverAuthenticationId`、`action`、`bindingHash`、`issuedAtMonotonicMs`、`expiresAtMonotonicMs`、`approvalAuthenticationWindowStartedAtMonotonicMs`、`durationMs`。数字字段沿用实际数字类型/安全整型范围校验，严格检查后规范化全图比较。租约到期为 `min(消费时刻 + maximumDurationMs, 原主身份到期)`；`durationMs` 是本对象时钟中的实际到期减消费时刻，不是从后续调用再重新计时的时长。跨时钟域的账本必须扣除消费调用耗时后保守设自己的期限，不能直接比较另一个对象的单调时钟基准。

`approvalAuthenticationWindowStartedAtMonotonicMs` 来自审批身份的窗口开始时间，明确不是凭据验证结束时间。租约保留已认证的历史审批身份作为消费依据，但不把审批身份的 60 秒等待消费期限伪装成续期登录。租约期间仍需要原主身份的同一 session/epoch/revision、原 5 分钟认证期限、当前主角色、当前独立审批角色、不同 SID、活动且未锁定的同一工作站会话。原生 `uploadIdentityLeaseCurrent(receipt)` 严格核对本对象实际租约和这些来源；它允许不改变主会话的独立审批提示忙状态，但新操作员登录、注销和工作站撤销清除实际租约。60 秒待消费审批身份到期不会清除已经消费的租约，也不允许同一审批身份再次消费。

`revokeUploadIdentityLease(leaseId)` 仅原生、仅匹配实际租约 ID 时清除租约及其历史身份，返回 bool，不发同步通知。账本在实际授权撤销或正常结束时应调用；若匹配失败，不能清除其他操作的租约。最大 4 分钟并不保证任意大小任务上传能完成，超时仍应停止后续传输并按部署模块排空，不能自动延长、自动重新审批或滚回已写入数据。

## 单次消费的启动意图认证依据（不是启动许可）

`consumeStartApprovalAuthentication(primary, approver, bindingHash, approvalId)` 仅原生、不是 QML 接口。它再次核验本对象实际主身份和实际待消费独立审批收据、不同 SID、当前直接角色、活动且未锁定的实际工作站会话、精确 `mission.start` 动作、精确绑定 SHA-256 和规范 UUID。上传审批身份或上传租约不能传入这个消费接口。仍有效的已消费启动意图认证依据不能被新消费偷偷替换，须由其原生账本先明确撤销。

成功不可逆地清空唯一的待消费审批身份，保留完整主身份与审批身份作为私有来源，返回 `{ok:true, authenticationReceipt:proof, startupPermitGranted:false, executable:false, flightCommandReleased:false}`。失败也明确这三个许可/指令字段为 `false`。消费和精确撤销均不发同步 `changed`，防止账本建立或清除认证依据期间重入；系统 WTS 撤销通知仍可能在消费前的原生核验中发信号，调用方必须检查自身存活和实际冻结来源。

`proof` 恰好 16 个字段：`schemaVersion`、`approvalId`、`approverAuthenticationId`、`primarySessionId`、`primaryActorId`、`primaryRole`、`approverActorId`、`approverRole`、`sessionEpoch`、`sessionRevision`、`action`、`bindingHash`、`consumedAtMonotonicMs`、`expiresAtMonotonicMs`、`durationMs`、`approvalAuthenticationWindowStartedAtMonotonicMs`。五个数字字段只接受实际安全整型数字类型，严格形状核验后才规范化比较；字符串字段不能通过数字/布尔转换冒充。UUID 必须规范，动作固定 `mission.start`，哈希固定小写 64 位十六进制。`expiresAtMonotonicMs = min(原主身份到期, 原审批身份到期)`，`durationMs = 到期 - 消费时刻`；消费不会重新给出 60 秒窗口，更不会生成 4 分钟启动租约。审批窗口开始时间不代表验证完成时间。

`startApprovalAuthenticationCurrent(proof)` 仅核对本对象私有保留的精确实际认证依据及完整原身份来源；不调用已消费清空的 `approverSnapshotCurrent()`，也不接受 JSON 导入、应用重启恢复或外部“已审批”标志。核验要求同一主会话/epoch/revision、原主身份和原审批身份期限、当前主角色和独立审批角色、不同 SID 以及实时工作站状态。凭据窗口忙时拒绝新启动意图授权；定时器在窗口期间仍执行原期限到期，窗口外重新核验当前角色。它只能证明启动意图审批的身份来源仍有效，不能证明车辆可飞、计划已审阅、审批人一直在场或允许解锁。

`revokeStartApprovalAuthentication(approvalId)` 仅原生、只接受实际保留依据的精确 ID，清空依据与历史身份并返回 bool，不发同步通知。注销、新主身份认证、原身份到期、角色失效、工作站锁屏/注销/断开或监视窗口失效均撤销依据；解锁不恢复。启动审批账本必须另行约束原始审阅期限、计划/目标绑定和一次消费状态。本模块没有启动派发、飞行许可、解锁、起飞或任何 Vehicle 操作入口。

## 单次转移到实际 StartupAuthority（仍不是 StartPermit）

原生 `transferStartApprovalAuthentication(primary11, authentication16, authorityObject, startupId, authorityEpoch, sourceHash, maximumRemainingMs)` 只接收真实强类型 `FleetMissionStartupAuthority` 同线程对象、规范启动 UUID/实例 epoch、小写 SHA-256，以及 `1..30000` 毫秒原审阅预算。真正的唯一业务接管入口是 [FleetMissionStartApproval::transferToStartup](FleetMissionStartApproval.md)，该账本在转移前核实际当前条件 grant、完整 Review/档案/部署来源，并在转移前后调用实际 Authority 的 `authenticationTransferContextCurrent`，不把 caller 的哈希、时长或 JSON 当成技术授权。Session 自身再次核当前主身份、精确私有已消费认证依据、当前 OS 直接角色与 WTS 会话。

预算在任何 OS 查询前固定为此对象时钟的 entry + 最大剩余时间；实际到期取该预算、原主身份到期、原审批身份到期及旧认证依据到期的最小值。SAM/WTS 校验耗时消耗原预算，不能在函数返回重新启动 30 秒。仍有效的新接管依据不允许暗中替换；旧来源在回调期间发生变化时拒绝或只清理确切被冻结的旧来源，不能清空另一个接管对象。

成功不可逆地清除旧 16 字段依据及其私有身份副本，不改变主身份 epoch/revision；把完整原主身份/审批身份移入独立私有保留状态，返回 `{ok:true, authenticationReceipt:receipt22, startupPermitGranted:false, executable:false, flightCommandReleased:false}`，没有同步 `changed`。这个动作只转移身份依据，不转移飞机状态、审阅技术结论或执行许可。

新收据恰好 22 字段：`schemaVersion`、`startupId`、`authorityEpoch`、`authenticationId`、`sourceHash`、`approvalId`、`approverAuthenticationId`、`primarySessionId`、`sessionEpoch`、`sessionRevision`、`primaryActorId`、`primaryRole`、`approverActorId`、`approverRole`、`action`、`bindingHash`、`transferredAtMonotonicMs`、`expiresAtMonotonicMs`、`durationMs`、`approvalAuthenticationWindowStartedAtMonotonicMs`、`originalAuthenticationExpiresAtMonotonicMs`、`purpose`。六个数字字段实际数字/精确安全整数规则与主身份一致，严格形状检查后规范化比较；16 个字符串严格实际 String。用途固定 `startup-authentication-basis-not-permit`，动作精确 `mission.start`；新 `authenticationId` 是此 Session 实际生成的一次随机 nonce。审批窗口开始时间是历史窗口起点，不是认证完成时间。

`startupAuthenticationCurrent(receipt22, actualAuthority, startupId, authorityEpoch, sourceHash)` 只认本对象保留的完整精确收据、实际同一 Authority 弱指针/实例 epoch、主 session/revision、不同实际 SID、实时双方角色及活动未锁定 WTS，会在回调后重新核对这些冻结来源。凭据窗口忙时拒绝门禁；定时器不续期，窗口期间仍执行到期，窗口外核原生来源。Authority 销毁、角色/会话/原期限失效、新主登录、注销、锁屏/断开均清除接管依据。解锁不能恢复。

新身份依据不再读取已退休的旧条件 proof 或旧 Review/Content current，因而旧审阅页因后续合法 ARM 状态变化而自然失效，不会误清这个独立账户依据。**Authority 自身仍是 preARM-only，不发送指令**。后续实际 StartupTransaction 现在有专用有限阶段源码，须再次从该 Authority 一次接管并自己核合法阶段、完整机载新读/物理/参数/健康来源，不能只靠身份收据派发，也不能把账户证明当完整 StartPermit。

原生 `revokeStartupAuthentication(actualAuthority, exactStartupId, exactAuthorityEpoch)` 仅清同一实际 owner/scope，不同步通知。旧 `revokeStartApprovalAuthentication(oldApprovalId)` 只针对已退休的旧存储，不能清新的账户接管事实。主动取消优先调用实际 Authority 的确切启动 ID；旧账本的显式撤销可以安全转发到其保存的实际 Authority，自动旧 Review 失效则不能这样转发。所有收据、owner 和时钟基准只在当前进程保留，没有 JSON/文件恢复或通用授权消费接口。

## Authority 到实际 StartupTransaction 的一次接管

`transferStartupAuthenticationToTransaction(authentication22, actualAuthority, actualTransaction, startupId, authorityEpoch, transactionEpoch, sourceHash, maximumRemainingMs)` 仅原生，严格强转真实同线程 Authority 和 Transaction；前者必须是现存证明的实际 owner/epoch，后者必须匹配其实际实例 epoch。启动 UUID 与 full-source 哈希沿用，不重新生成任务范围或恢复 JSON。唯一业务入口由 Authority 转移函数先后核实际完整 context 和 Transaction 的原生 pending transfer-context；Session 再核自己保留的精确 22 字段依据、双方真实 SID/当前角色/WTS。

在 OS 查询前固定预算，实际到期是原 Authority 证明到期与 entry + 原剩余预算的最小值。成功清空旧 Authority 证明与其 destroyed 连接，仅移动完整原主/审批身份到独立 Transaction 私有状态，主 epoch/revision、5 分钟主登录、60 秒原审批窗口和更短原 Review/READ START 期限均不续期。新的 `authenticationId` 是一次随机 nonce，不是新认证；`authorityAuthenticationId` 保留原 nonce 作为历史转移来源。

新证明 schema 1 固定 24 字段，即原 22 字段增加 `transactionEpoch` 和 `authorityAuthenticationId`，`purpose` 改为 `startup-transaction-authentication-basis-not-permit`；`transferredAtMonotonicMs` 为此次原生移交时刻，`durationMs` 为旧 deadline 减此时刻，不是新的 30 秒。六个数字仍严格实际有限安全整数，18 字符串实际 String。成功/失败 wrapper 仍全部 `startupPermitGranted=false`、`executable=false`、`flightCommandReleased=false`，Session 没有传输入口。

`startupTransactionAuthenticationCurrent(proof24, actualTransaction, startupId, transactionEpoch, sourceHash)` 只对照私有实际证明、原 Authority 历史依据、完整原身份、同一实际 Transaction/epoch、不同真实 SID、当前双方直接角色、原期限和当前活动未锁定 WTS；不会调用已退休 Authority 的 preARM gate，因此期望的实际 ARM 不会自行撤掉身份来源。身份稳定并不批准 HOME 重置、物理风险变化或下一步命令，这些仍由 Transaction 的独立阶段/风险 gate 核验。

`revokeStartupTransactionAuthentication(actualTransaction, exactStartupId, exactTransactionEpoch)` 只清此确切 owner/scope，不同步通知。旧 Authority revoke、析构或旧 proof 清理不能误清新的存储。实际 Transaction 销毁、注销/换主身份、原期限、角色、锁屏/断开失效会撤销新依据，定时器在回调后核冻结 proof/owner 再清理，不清回调中新提交的其他依据。没有新凭据窗口、企业身份恢复、通用 lease、自动续期或飞行命令。

## GroundStartupAuthority 到实际 StartupTransaction 的独立移交

原生 `transferGroundStartupAuthenticationToTransaction(authentication28, actualGroundAuthority, actualTransaction, startupId, authorityEpoch, transactionEpoch, sourceHash, maximumRemainingMs)` 只接受真实强类型、同线程、同实例 epoch 的 GroundStartupAuthority 和 StartupTransaction。普通 Authority22/Transaction24 不可交叉消费；仍有效的其他 ground Transaction30 不能被默默替换。原 startup UUID、原 full-source hash、整组目标和原 review/read deadline 沿用，不恢复 JSON 或生成新任务范围。

Session 在第一个系统查询前固定 `entry + maximumRemainingMs`（1..30000 毫秒）。通过实际 Authority 的原生 `pendingStartupTransactionContext(actualTransaction, startupId, transactionEpoch, sourceHash)` 获取仅内存的冻结 context27，再通过真实 Transaction 的 `authenticationTransferContextCurrent(startupId, sourceHash, actualGroundAuthority, exactContext27)` 核对同一次 preparing/pending handoff。这两个回调不能递归调用完整 source/OS 门禁；完整技术门禁由外层 Authority 与 Transaction 在移交前后核对。Session 另外即时核实际 ground28、原两 SID/角色、主会话/工作站和原更短期限，所有系统及上下文回调后复核冻结来源、实际 owner 和 epoch。

成功返回恰好六字段 `{ok:true, authenticationReceipt:proof30, remainingMs:..., startupPermitGranted:false, executable:false, flightCommandReleased:false}`；`remainingMs` 在所有检查完成后计算。失败沿现有原生认证拒绝结构返回 `ok:false`、`reason`、空 `authenticationReceipt` 和三个 false，不把历史 duration 当剩余预算。成功清空实际 ground Authority28 并断开其销毁监听；保留原 ground28、原条件 ground22 及原两身份到独立 Transaction30 私有仓，不调用 `advanceSession`。移交后校验失败只清确切新 owner/epoch/nonce 的 proof30，不复活旧28、不误清旧非 ground24 或回调中新 replacement。

私有 proof30 = 原 ground28 的 28 字段加 `transactionEpoch` 和一次随机 `transactionAuthenticationId`，`purpose` 改成 `ground-startup-transaction-authentication-not-permit`，实际 `expiresAtMonotonicMs` 只缩短，`durationMs` 仍是更短 expiry 减原 `consumedAtMonotonicMs`。原 `authenticationId`、`authorityAuthenticationId`、`transferredAtMonotonicMs`、`transferBudgetExpiresAtMonotonicMs` 仍是历史来源，不能当新认证或重启时钟。此次移交时刻和 entry-budget deadline 单独保留为原生私有状态，不进入可反序列化接口。八个数字字段仍严格实际有限安全整数；两个新增字符串严格规范 UUID。

`groundStartupTransactionAuthenticationCurrent(proof30, actualTransaction, startupId, transactionEpoch, sourceHash)` 精确对照私有 full map、原28/22关联、同一真实 Transaction/epoch、原两 SID/当前直接角色/WTS、原主 session/revision 和此次私有原期限；不读取退休 Authority 的 ground-only gate，也不重新审批 HOME、地面净空、解锁或飞行许可。Transaction 必须独立检查实际阶段/完整机载读回/风险/链路/整组范围与真实 transport facts。Session 本身不发任何指令。

`revokeGroundStartupTransactionAuthentication(actualTransaction, exactStartupId, exactTransactionEpoch)` 只清这一实际 owner/scope，不发同步通知。真正的事务撤销/排空必须由 Transaction 持续保有原 UUID 的车辆预留直到 native transport quiet；不能把身份清理当作可提前放弃物理事务所有权的依据。旧 GroundAuthority/GroundApproval 的自动失效或析构不会误清新30。新事务销毁、注销/换主身份、锁屏/断开、角色或原期限失效会清新30；定时器在系统回调后比对冻结 owner/map，避免清另一个 scope；忙提示期间门禁拒绝，期限照常耗尽。

工程师仍需实际验证 Windows/Qt 编译链接、两独立真实账号、原28只能移交一次、purpose/字段/owner 错配拒绝、30与旧24分仓、失效后不恢复、查询耗时不续期、移交中 WTS/角色/对象销毁和新事务排空期间撤销。这一批只做源码和文本静态检查，不执行系统认证、测试或设备操作。

## 工作站锁屏、注销和断开

Windows 适配器要求实际内核版本 Windows 10 或更高，使用已加载的 `ntdll.dll` 中 `RtlGetVersion` 查询，不采用 Qt DEBUG 环境覆盖或依赖清单的旧版本推断；版本未知/过旧直接不可用。这样避免 Windows 7/Server 2008 R2 的 WTS LOCK/UNLOCK 标志反转问题。本版本没有旧 Windows 或非 Windows 的宽松替代身份来源。

本对象拥有不可见的 message-only HWND，并用 `WTSRegisterSessionNotification(NOTIFY_FOR_THIS_SESSION)` 注册；目标 session 通过 `ProcessIdToSessionId(GetCurrentProcessId())` 冻结为实际进程所在会话，不拿活动控制台会话冒充 RDP 会话。注册失败、窗口丢失、查询失败或未知状态均不能授权。每次认证/身份/上传租约/启动意图认证依据门禁都查询本机 `WTSQuerySessionInformationW(WTSSessionInfoEx)`，要求足够且有界的结构、Level 1、同一 session ID、`WTSActive` 和确切 `WTS_SESSIONSTATE_UNLOCK`，查询缓冲区清零并释放；注册窗口中尚未分派的 WTS 消息也阻止新授权。

实际同一会话的锁屏、注销、控制台/RDP 断开、终止和监视窗口销毁消息立即改变会话 epoch/revision，并清除主身份、待消费审批、实际上传租约、已消费启动意图认证依据和已接管的 Authority/Transaction 身份依据。其他会话消息忽略。解锁、重新连接或新系统登录消息不会恢复旧身份/租约/认证依据，必须重新完成产品登录及相应审批。窗口注销通知后才销毁，WndProc 在通知 Qt 前完成默认处理，不在会导致对象删除的信号之后再访问窗口/监视对象。

这不是人一直在场、账号未共享、RDP 未被接管或操作系统不可攻击的证明，也不是企业账号撤销服务。系统消息分派/主线程延迟仍需工程师验证；真实会话状态核验与有限租约并行提供拒绝路径，不能以历史“未锁屏”断言代替实时检查。

## 秘密与平台边界

密码及凭据 BLOB 只存在于原生缓冲区，不进入 QString/QVariant、QML、日志、磁盘、任务快照或历史。密码/凭据 BLOB 在登录成功后立即 `SecureZeroMemory`，所有退出路径也通过不可复制的 RAII 缓冲区清零并释放；令牌句柄关闭。账号拆分使用有界 Windows `CredUIParseUserNameW`，UPN 使用 null domain，显式本地域/域名不被默默改写。空密码、不能解包或不能供 `LogonUserW` 验证的提供者格式拒绝；不降级为 generic 明文提供者窗口。

本版本是**Windows 10+ 本地交互式凭据适配器**，不是跨平台账号系统、SSO、远程身份服务、硬件密钥/Windows Hello 完整支持，也不是进程级恶意插件隔离。非 Windows 构建拒绝认证，没有假身份替代。产品角色授权、二次审批、车辆健康和实际命令门禁仍独立存在；操作员登录或身份租约本身不表示允许上传或飞行，具体传输授权仍由审批/部署模块约束。两个 SID 证明两个独立账号完成了认证，不能证明现实中是两个人、没有共享密码，也不能抵御掌握本机管理权限/进程修改权限的攻击者。组织须管理账号、组的 ACL、职责分离和凭据安全。

Windows 的交互式登录可能由系统缓存登录信息；本模块不声称清除了 LSASS/凭据提供者内部缓存。会话 TTL 证明近期认证及当前角色，不证明账号当前未禁用、密码未改变、设备被加固或现实操作员一直在场。实时账号撤销、跨站身份/租约和企业鉴权仍需后续完善；本模块补的是本机 WTS 会话撤销、一次性上传身份租约和不续期的启动意图认证依据源码，不是这些更广义能力已经验收。

工程师应补充 Windows SDK/Qt 编译、链接 `Credui`/`Advapi32`/`Netapi32`/`Ole32`/`Wtsapi32`、安全桌面及密码提供者兼容性、取消/超时/对象销毁、角色组变化、相同 SID 拒绝、不同有效审批账号、四个精确动作的提示与拒绝交叉消费、监视结束只在原生实际 owner 当前流程消费且不产生长授权、一次消费重放拒绝、启动意图不续期及全部许可字段为 false、60 秒待消费超时与 4 分钟上传租约差异、原主登录到期限制、精确撤销、真实锁屏/解锁/注销/RDP 断开、监视窗口注册失败及非 Windows 拒绝路径验证。本轮不执行这些操作，不自动创建账号或角色组，不进行部署、刷机或放行。

## 采用的官方 API 文档

- [CredUIPromptForWindowsCredentialsW](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-creduipromptforwindowscredentialsw)：凭据提供者、安全桌面、BLOB 清零与释放。
- [CREDUI_INFOW](https://learn.microsoft.com/en-us/windows/win32/api/wincred/ns-wincred-credui_infow)：提示信息和标题的原生长度限制。
- [CredUnPackAuthenticationBufferW](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-credunpackauthenticationbufferw)：受保护凭据解包、字段大小及提供者格式。
- [CredUIParseUserNameW](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-creduiparseusernamew)：UPN 与域名/用户名的原生拆分。
- [LogonUserW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-logonuserw)：真实本机登录验证、令牌、UPN domain 规则及系统缓存边界。
- [GetTokenInformation](https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-gettokeninformation)、[ConvertSidToStringSidW](https://learn.microsoft.com/en-us/windows/win32/api/sddl/nf-sddl-convertsidtostringsidw)、[LookupAccountSidW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-lookupaccountsidw)：令牌主体 SID、显示转换和仅用于展示的账号名。
- [NetLocalGroupGetMembers](https://learn.microsoft.com/en-us/windows/win32/api/lmaccess/nf-lmaccess-netlocalgroupgetmembers)：本地 SAM 直接成员 SID、有界缓冲区和枚举句柄。
- [Qt 5.15 QML 数值转换源码](https://raw.githubusercontent.com/qt/qtdeclarative/5.15/src/qml/jsruntime/qv4engine.cpp)：QML JS 整数返回 QVariant::Int，而其他 Number 返回 Double；因此数字存储类型往返不得被误判为伪造收据。
- [RtlGetVersion](https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlgetversion)、[ProcessIdToSessionId](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-processidtosessionid)：实际系统版本与实际进程会话。
- [WTSQuerySessionInformationW](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsquerysessioninformationw)、[WTSINFOEXW](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/ns-wtsapi32-wtsinfoexw)、[WTSINFOEX_LEVEL1_W](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/ns-wtsapi32-wtsinfoex_level1_w)：活动/锁屏状态、结构和 Windows 7 反转标志。
- [WTSRegisterSessionNotification](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsregistersessionnotification)、[WTSUnRegisterSessionNotification](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsunregistersessionnotification)、[WM_WTSSESSION_CHANGE](https://learn.microsoft.com/en-us/windows/win32/termserv/wm-wtssession-change)、[Message-only windows](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features#message-only-windows)：定向消息、会话撤销与监视窗口生命周期。

# 新地面再审启动依据的一次接管

## 535 当前：完整异机包络报告随原生依据接管

绑定材料升级为 exact8，必需包含非空 `interVehicleEnvelopeReview`；旧七字段材料不能补空报告后继续。接管同时使用 [Worker](FleetMissionStartupClearanceWorker.md) 的共同结论门，要求完整异机包络比较完成且没有交叠候选；整份报告仍由原 artifact／sourceHash 和原生 [Witness](FleetMissionStartupClearanceWitness.md) 绑定。

新检查比较同次准备的启动整域与 NAV／起降参考组件，保原机体、支撑和误差扩张，不将接地 pad 用作异机豁免，不以不同计划时间推断安全错峰。交叠是声明外包模型的保守候选，不是实测轨迹碰撞；无交叠也不证明真实路径。原 minimumSeparation、来源、期限、审批和最终发送条件继续保留。

Authority 不创建几何、不重跑求解器、不从 JSON 安装凭证。原 `actualPathCertified=true` 必需条件未放宽，当前实际 producer 仍为 false；本批没有开放 StartPermit 或新增飞行指令。仅源码和静态审阅，未编译、测试、仿真或放行。下方 482 及更早为历史。

## 482 当前：地面依据只接完整result7与原源Witness

绑定检查当前要求exact7，新增非null、非空 `waypointSwitchReview`，保原名义／整域报告与整份artifact／sourceHash身份。仍从实际已确认ArmedGroundReview取得原Witness、读取其余来源后再核同Ptr，不接受JSON材料安装凭证或缺报告降级。

Witness保原29稳定参数、最初PRIMARY弱owner和READ／Review时钟；Authority不新采样、不换源、不续期。共用准入消费新NAV模型／环境比较完整及零violation，仍要求既有actualPathCertified=true。当前实际路径未证明，不能因停止旧事务或新报告无violation就放行；接地pad、双账号、一次移交及最终发送门禁不放宽。

只静态源码／文档修改，未编译、运行、测试、仿真或刷机放行；最终文本核对已记录于本批静态审计。下方exact5／6为历史，当前使用完整七字段结果。

## 467：接管材料必需包含完整整域报告

绑定检查从旧 result5 升级 exact6，额外要求 `startupDomainReview` 为非 null、非空 map，仍必须从实际已确认地面 Review 取得同一原生 Witness，核原 inputFingerprint 并在其它来源读取后再核同 Ptr。新增字段不是可安装凭证或复用历史权限的入口。

两 Review 和 Witness 工厂已共用 [Worker](FleetMissionStartupClearanceWorker.md) 的名义模型、完整整域与 numericContainmentProven 必需条件；当前该数值包含字段固定 false，因此地面 Review 保留完整两报告并进入 clearance-blocked，工厂不发布新 Witness，本 Authority 不把理想外包成功当作许可。原 source18／datum14、实际对象、原 READ／Review 截止、UTC 水位、双账号及一次移交逻辑不变，无资料旧路径不加依赖；Compiler pending 与完整 StartPermit 仍未完成。仅源码／静态核对，未编译、测试、运行、仿真或放行。下方 result5 为历史，现要求完整六字段结果。

## 456：保留新地面审阅的同一原生净空 witness

冻结计划含 `constraints.clearanceInput` 时，接管必须从实际、已确认的 [ArmedGroundReview](FleetMissionArmedGroundReview.md) 取得原 [startup clearance witness](FleetMissionStartupClearanceWitness.md)，核对原 `reviewArtifact.startupClearanceReview` result5 的 `inputFingerprint`，并在来源读取之后再次确认仍是同一 shared pointer。只读取报告或给出相同 hash 不能安装该原生对象；required witness 缺失、替换或失效直接拒绝，不能降级成旧无资料流程。明确未提供资料的旧计划仍使用空 pointer。

私有 pointer 贯穿本轮 Authority，保留实际 typed source18／datum14、原 Review 的单调时钟绝对期限和 UTC 水位，不新建时间窗口、不续期、不重跑 solver，不替换最初 Readiness／v4 reference sample。`_gate` 先执行 witness 的完整来源检查，再完成原账号、双人批准、占用、物理状态和 Root 门禁，最后以 callback-free `memoryCurrent` 封闭回调窗口。fence 只核原对象／generation／冻结内容／pointer，不递归进入 witness。

新增非 QML 的 `startupClearanceWitness(startupId)` 只在原 accepted、active、持有预约、未进入移交且完整门禁成功时返回同一 pointer。新事务在移交前取得并独立持有；Session 搬移前后、transferred 状态下和最终返回前均检查原 pointer 及原来源，不从 portable context 恢复。搬移失败仍按原规则撤销实际已搬移认证、保留提交事实并释放精确原 UUID，不取消另一轮作用域。

witness 只延续来源证据，不是许可；本模块未解除 Compiler pending、未新增命令、未证明运动包络与已求解净空包络包含关系或现场物理真实性。原显式撤销转发、事务排空、身份和截止规则保持。仅源码静态核对，未编译、测试或运行。

## 445：地面依据持续对原 v4 样本比较

从新地面审阅实际冻结材料接管 v4 reference/sample，不从当前遥测重建。后续稳定身份仅排 sample，但必须另对首次 sample 比显式容差；缺 map、错误指纹或 reference 不同不通过。完整 `_context()`／sourceHash 保持原样本，不因新包续期或重基。

原再审、双账号、来源和发送条件不放宽，v1–v3 不依赖 ALTITUDE；关系一致不认证共同原点、内部有效性或许可。未编译、测试或运行。

## 444：保留冻结高度诊断，当前比较不随周期抖动

地面 Authority 的 `stableReadiness` 显式排除 [startupAltitudeDiagnostic](FleetMissionStartReadiness.md)，与普通 Authority／GroundReview／Readiness phase identity 一致。原 `currentRisk`、`_context()`、sourceHash 和事务交接保留首次材料中获预算接纳的诊断；若原材料已省略则不补采，缺键仅为未提供。后续只比较去掉该动态附加展示字段后的真实风险；不更新旧材料、原期限或批准来源。

缺报文、未知字段、周期高度／receipt／age 变化仅为诊断，不新增 v1–v3 门槛，不放宽原实际来源、健康、物理状态、原点与启动政策条件。材料不是实时高度、传感器样本鲜度、共原点、PX4内部valid、净空或许可认证。未编译、测试或运行。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestAuthority、_gate、pure authenticationTransferContextCurrent/pendingStartupTransactionContext、authorityContext、actualSources/native供应者及transferToStartupTransaction核实际owned来源与Txn同组；搬移/预约变更前后使用same/transferred复核。撤销、销毁和排空保留；只移除primary Link的GUI线程误拒。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


270–274：`nativePreviousStartup(startupId)` 经当前完整门禁返回本轮已冻结的实际前代，供[事务序列](FleetMissionStartupSeries.md)核对，而非用数值ID或最新UI快照替换。每轮显式新Review先按旧UUID/epoch/hash退休Transferred链并确认terminal/drained，再冻结最新已初始化owner；新Authority的accepted gate持续只核本轮冻结来源，不因Series最新指针前进而回查旧Review.valid。原28/30一次移交、真实两身份与原最早期限规则不变；此getter不启动读取或飞行。

2026-10-14，修改记录 260–264 的独立依据基础；本轮增加 actual StartupTransaction 的一次移交及原 UUID 连续占用。仅地面站源码与有限静态契约；未编译、运行、测试、调用 Windows/MAVLink/设备接口、修改飞控、刷机或放行。

## 用途与未完成范围

`FleetMissionGroundStartupAuthority` 将 [GroundStartApproval](FleetMissionGroundStartApproval.md) 的一次新两账号条件意图移交到独立实际 owner/startup UUID/source hash。它不是旧启动恢复、最终 StartPermit、起飞净空认证或控制租约，不发读取、seq0、ARM、MODE、DISARM、删除任务或回滚。

此依据接受前及接受期间将已解锁/未解锁状态逐机冻结，实际变化仍撤销。本轮增加向 actual `FleetMissionStartupTransaction` 的强绑定移交；该消费者的 typed 分阶段动作、合法差异与完整 HOME/local frame/运动/转向策略由事务模块定义，不能用本模块快照代替动作门禁。净空、最终许可及完整任务执行完成尚未由本模块证明。原 25 项/六类范围保持，不能把“接受依据”或一次移交当作完整飞行功能完成。

## 实际源与新收据

- `requestAuthority(actualSources10, reviewId, operatorConfirmed)`：实际 Session/Role/Store/groundReview/groundApproval/previousStartup/Deployment/Swarm/Health/authorizationOwner，全强类型/同线程；另冻结 actual Catalog/Coordinator/Registry/FaultPolicy 与原整组 Vehicle/MM/PM/VLM/PRIMARY。
- 初始取实际已阅 Review20 和 public27 receipt。全记录 hash/字节、全 risk artifact、member/payload、实际完整 RAW/read scope/operation/epoch/cache、frozen armed、当前参数版本/坐标/健康及 Leader 均核对。
- 新 startup UUID 只消费一次 native14 grant。`authenticationTransferContextCurrent(startupId,sourceHash,actualApproval,exactBasis22)` 是 pure 内存所有权回调，绑定实际 pending owner/epoch/grant/source hash，不反调 OS/Root/full gate。
- Session 将旧22一次移交成独立28，保存原22/原两身份及新actual owner；Session返回6字段（含所有核对后的真实剩余时间），Ledger返回10字段。28 的历史 `durationMs` 不是“现在还剩多久”，不得用它续权。
- 成功后旧 Ledger `transferred`、原22及grant退休，停止旧 Review observers；新owner独立查当前 SAM/SID/role/WTS、原最早 READ START deadline、全部 native RAW/cache/参数/版本与 typed 风险，不使用 oldReview.valid 或 oldGrant.current 保持授权。
- 原始 private context 为27字段，包含完整新Review/风险、任务、原身份/角色与来源；public `current` 仅范围/ID/hash/期限/有界目标摘要，不公开身份或RAW。所有 `previousAuthorizationRestored/startupPermitGranted/executable/flightCommandReleased` 为 false。

## 期限、占用与撤销

预算在取 Review 前按本地调用起点冻结，并受新回读最早 READ START 的原 ≤30秒及原主/次身份更早期限约束。Session 在首系统查询前冻结 transfer budget，实际剩余在所有核对之后返回，双方只缩短、保守计完整调用时间，不比跨域绝对时钟，不以消费/接管完成重新计时。

只有移交完成、new28/current 与独立全源 gate 成功才取得整组本地预留。准备态允许这笔旧的**新**条件 grant 完成自己的完整门禁，不把自身当竞争工单；接受态仍每100毫秒及每次 native context 查询重验。预留不是跨站控制租约。

未移交时撤销先关闭当前 gate/状态，按 actual owner/startupId/epoch 撤销新 proof，再释放捕获的旧 startup UUID。Session 通知即使删除/替换对象，也不能遗留旧预留或释放 replacement。此类不发 native transport，没有它所拥有的待发包栈。正在移交但返回包装尚未提交的失败/销毁还按捕获的 actual pending Transaction/UUID/epoch 清理新30；该阶段不允许动作队列入站。

旧审批显式 revoke 精确转发保存的新 owner/UUID/epoch/hash；只在对应新owner真正终态、无active/预留/排空后退休旧转发引用。旧审批自然到期不能盲清新28，原期限和风险仍由新owner检查。

## 工程师验证项（未执行）

1. 受支持 Qt/MSVC 工程重新生成、moc/type/pri/跨类强cast接口及资源接线。
2. 旧22/新28互斥、重复移交、旧scope取消/销毁与新scope replacement、转移后旧Review自然失效、SID/角色/WTS实时撤销。
3. 控制器准备态/接受态占用、reserve回调销毁与代际变化、Session revoke回调销毁、精确旧UUID释放。
4. 30秒边界/耗时回调/异单调clock、旧duration误用、实际新RAW/参数/版本/目录变化、mixed armed 自动停机与物理ABA。
5. actual Transaction 的28→30一次消费、Session6/Authority10字段、保守剩余时间、pending context pure无重入、原Swarm UUID无释放重取、转移后旧审批显式取消链及实际事务被动排空。

## 向实际动作事务的单次移交

- `actualSourcesForStartupTransaction(startupId)` 只有在 accepted/active/held 且原全源门禁通过后返回 exact8：actual Session/Role/Store/authority=this/Deployment/Swarm/Health/authorizationOwner。实际 Catalog/Coordinator/Registry/FaultPolicy 和 `nativeGroundReview` getter 同样受当前全源门禁保护；转移后均不可再取作许可。
- `transferToStartupTransaction(actualTransaction,exactAuthorityContext27,startupId,sourceHash,maxRemaining)` 只接受实际强类型、同线程、不同于原 previous Transaction 的新消费者。全源/身份/完整context/current/原整组预留和 actual Transaction pure pending 回调在移交前核对；不接受任意 QObject、自述 bool 或替换 ID。
- Session 在系统核对时用 `pendingStartupTransactionContext(actualTransaction,startupId,transactionEpoch,sourceHash)` 取得本模块实际冻结27。此接口只在 exact native pending stack 的 busy/transfer/accepted/active/held 期间返回纯内存证据；不反调 OS/Root/full gate，不是启动许可。
- native Session 将原28退休成独立30，返回 exact6（包含全部核对之后的真实 remainingMs）；本模块先成为 `transferred`，清理原28/grant并停止旧 pre-ARM observers，然后独立核对 new30/current、actual Transaction pure callback 和原整组预留，成功返回 exact10。原 `authorityContext27` 形状不变。
- 成功包装为 `ok/authenticationReceipt/startupId/transactionEpoch/sourceHash/remainingMs/reservationTransferred=true/startupPermitGranted=false/executable=false/flightCommandReleased=false`；30不是24或28，不从旧 `durationMs` 推算现在的剩余时间。原最早READ START及两身份截止时间不变，返回的Session剩余再保守倒算到调用起点，只可能缩短期限。
- 原 Swarm startup UUID 在移交中始终保留整组占用；不 release/reserve 重取、不换子集、不恢复旧凭据。新事务收到成功包装后才取得发送门禁，并继续自行查全部实际源、当前身份、typed 风险、原预算与 transport ownership，不回查旧 Authority accepted/current 或旧审批valid保活。
- 移交成功后的显式 `revokeAuthority` 将 actual owner/UUID/Transaction epoch/source hash 精确转发到 actual Transaction 的取消；原 GroundApproval → GroundAuthority → Transaction 链一直保留至该事务 terminal 且无 active/reservation/transportDraining。旧依据销毁也只精确要求新事务取消与排空，不直接释放它的UUID或盲清替换proof。

本模块没有执行按钮，不直接发模式或解锁指令。工程师不能用 legacy `startMission` 代替 typed 事务，也不能从上述静态核对推导可放飞。

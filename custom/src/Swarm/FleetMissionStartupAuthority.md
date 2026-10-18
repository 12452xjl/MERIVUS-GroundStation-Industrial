# Finite native startup-authority basis (not a StartPermit)

## 535 当前：完整异机包络报告随原生依据接管

绑定材料升级为 exact8，必需包含非空 `interVehicleEnvelopeReview`；旧七字段材料不能补空报告后继续。接管同时使用 [Worker](FleetMissionStartupClearanceWorker.md) 的共同结论门，要求完整异机包络比较完成且没有交叠候选；整份报告仍由原 artifact／sourceHash 和原生 [Witness](FleetMissionStartupClearanceWitness.md) 绑定。

新检查比较同次准备的启动整域与 NAV／起降参考组件，保原机体、支撑和误差扩张，不将接地 pad 用作异机豁免，不以不同计划时间推断安全错峰。交叠是声明外包模型的保守候选，不是实测轨迹碰撞；无交叠也不证明真实路径。原 minimumSeparation、来源、期限、审批和最终发送条件继续保留。

Authority 不创建几何、不重跑求解器、不从 JSON 安装凭证。原 `actualPathCertified=true` 必需条件未放宽，当前实际 producer 仍为 false；本批没有开放 StartPermit 或新增飞行指令。仅源码和静态审阅，未编译、测试、仿真或放行。下方 482 及更早为历史。

## 482 当前：只接管完整result7及其同一原生Witness

`clearanceBindingMatches` 从历史exact6升级为exact7，额外要求非null、非空 `waypointSwitchReview`；原 `startupDomainReview`、inputFingerprint和整份artifact／sourceHash继续匹配。新字段不是安装Witness的入口，仍必须从实际已确认StartReview双取同一原生shared pointer，并在全部来源回调后做原memory尾核。

Witness继承本次最初29参数快照、原PRIMARY弱owner及原Review绝对期限，不由Authority新capture、换owner、重算solver或续期。真实共用准入新增静态NAV连接条件，但保actualPathCertified=true的必要条件；现producer为false，不能以新报告／数值包含打开ARM／MODE或StartPermit。

只静态修改和直接源码阅读，未编译、运行、测试或放行；最终文本核对已记录于本批静态审计。下方exact5／6仅历史，不是当前合同。

## 467：启动净空结果升级 exact6，不兼容缺整域报告的旧结果

`clearanceBindingMatches` 改为严格六键计数，要求 `startupDomainReview` 为非 null、非空 map，并继续核实际 Review 的同一原生 Witness 和 inputFingerprint。结果仍由完整原 artifact／sourceHash 绑定，不从新字段安装凭证，不扩大授权来源。两 Review 和 Witness 工厂现在先消费 [Worker](FleetMissionStartupClearanceWorker.md) 共用的名义＋整域＋原 Qt 数值域包含条件；当前 numericContainmentProven=false 时工厂不发布 Witness，不能让这个形状升级绕过阻断。

原同 Ptr 双取核验、sourceCurrent／callback-free memory 尾核、原时钟不续期、账号移交、撤销与最后发送边界不变。无资料旧任务不加新依赖，Compiler pending 和 StartPermit 保持未完成。本批只做源码和静态文本核对，未编译、测试、运行或放行。下方 result5 表述为历史，当前均为六字段完整结果。

## 456：接管同一个原生净空 witness，不复制或续期

原计划 `constraints` 明确含 `clearanceInput` 时，`requestAuthority` 必须从实际、已确认的 [StartReview](FleetMissionStartReview.md) 取得其原 [startup clearance witness](FleetMissionStartupClearanceWitness.md)。它必须与本次冻结 `reviewArtifact.startupClearanceReview` 的完整 result5 中 `inputFingerprint` 一致；在读取其余来源后，再从同一个 Review 取得并核对同一 shared pointer。缺失、替换或失效不能退回无资料路径。旧计划未提供该字段时保留空指针及原流程。

Authority 私有持有 `std::shared_ptr`，不接收可从 JSON、QML 或报告 map 安装的 witness。原 artifact 和 sourceHash 不新增平行身份字段：已有 result5 继续绑定报告，原生指针则保留实际 typed 构型／datum 来源和原 Review 单调时钟的绝对截止。Authority 不创建 witness，不重跑求解器，不重采并替换原姿态，不调用 deadline 延长或以自己的时钟重建期限。

每次 `_gate` 先对同一 witness 执行完整 `sourceCurrent`；传入的 fence 只检查原 QPointer、generation、状态、冻结材料及指针，不递归进入 witness。随后仍执行原身份、权限、占用、Readiness 和实际 Root 门禁；所有回调之后最终执行无回调 `memoryCurrent`。原 Session 一次移交前、移交返回以及 transferred 后，仍核对同一 pointer、来源与最终 memory 状态；发生失效沿原失败／撤销流程处理，不抹去已发生的认证搬移事实。

新增非 `Q_INVOKABLE` 的 `startupClearanceWitness(startupId)` 只在原 accepted、持有预约、未移交且完整 `_gate` 成功时返回同一 pointer。事务必须在移交前取得它，移交后自行持有；旧 Authority 或 Review 的自然退休不负责重新造一份依据。显式取消仍沿原事务转发和排空规则，不因 witness 存活而获得执行权限。

本模块仍不产生 StartPermit，不解除 Compiler 的净空 pending，不证明运动包络包含关系、现场资料真实性或最终飞行放行。仅源码与静态读回，未编译、测试或运行。

## 445：接管原已阅高度样本，不在启动时换基准

v4 接管前，把当前 Readiness 与原 StartReview 同机的必需 reference/sample 比较：稳定 reference/hash 相同，当前 sample 相对原样本在显式包线内。成功后 `target.readiness` 的 sample 明确取原已阅材料，不取刚重采值；phase 指纹不含周期 sample，完整 sourceHash／事务交接仍保留原样本。

后续实际源检查除原稳定 readiness 比较，还另对首次 sample 比容差；不是删去动态字段后忽略变化。v1–v3 无新 ALTITUDE 门槛，原身份、来源、期限与许可条件保留。未编译、测试或运行。

## 444：高度诊断随冻结材料保留，不参与动态门槛

`stablePhaseReadiness` 除原 RAW coordinate supplement 外，显式排除 [startupAltitudeDiagnostic](FleetMissionStartReadiness.md)。本次原生 ALTITUDE 高度、收包序号／年龄或诊断未知状态变化，不会仅因此使原阶段身份每周期失效；Readiness 的 phase fingerprint 同样排除此诊断。`beforeArmReadiness`、原 sourceHash 与交给事务的冻结材料仍保存首次实际快照，绝不以新诊断覆盖旧审阅或续期。

原 actual owner、PRIMARY、版本、参数、物理阶段、EST、GLOBAL_ORIGIN、LOCAL/HOME/ODOM、角色认证与所有许可门槛保持。缺少 ALTITUDE 不新增 v1–v3 门槛；诊断不能替代这些依据，也不证明传感器样本新鲜、共原点、PX4内部valid、净空或最终StartPermit。仅源码静态审阅，未编译、测试或运行。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestAuthority、_gate、authorityContext、actualSources/native供应者及transferToStartupTransaction核Session/Review/Approval/Content/Deployment与实际Txn同组。authenticationTransferContextCurrent现为pure preparing/busy/代次/QQ/精确grant/hash/未持proof与reservation/原期限比对，不再调用full _gate；完整source/OS门禁仍由request及StartApproval移交两侧执行。纯返回不是身份、风险检查或StartPermit。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


This module takes one actual consumed conditional intent/authentication basis into a separately owned native scope. `authorityAccepted` can genuinely become true after all native identity, full source, phase-fact and original-deadline checks. It is **not** the final startup permit or a command adapter: `startupPermitGranted`, `executable`, and `flightCommandReleased` remain false. No fake ARM/start dispatcher or generic consumer placeholder is provided.

Current batch 244–249 adds an actual once-only transfer to a strongly typed StartupTransaction: unchanged startup UUID/reservation/original deadline, 24-field Session proof, ten-field transfer wrapper, retained 25-field Authority context, and 32-field dedicated Vehicle RAW step receipts. These are separate native carriers, not deserialized permission claims or an assertion that the remaining safety/flight roadmap is complete.

## Request and exact actual sources

`requestAuthority(actualSources, reviewId, operatorConfirmed)` requires an exact ten-key map whose values are actual `QObject*` variants, not JSON, string handles, restored claims or signed assertions. Keys are `session`, `roles`, `store`, `review`, `approval`, `contentCheck`, `deployment`, `swarm`, `health`, and `authorizationOwner`. Every source is strictly cast to its native type and must share the owner thread. The private root callback `fleetStartupAuthoritySourcesMatch(authorityObject, bundle)` checks this actual Authority instance plus all ten actual sources; `fleetStartupAuthorityContextAllowed(actualStartupAuthority, startupId, binding, ids)` checks its exact frozen scope/latest record/targets without recursively requiring an old review, upload lease or intent to remain live. Callback return values must be actual bool true.

The request requires a current acknowledged native Review, the same current operator SID/role, a genuine independent-account conditional approval for that exact Review, full fresh ContentCheck evidence, complete runtime Deployment plan/payload, and latest non-restored Store record bytes. The primary is operator/commander/administrator and the native `mission.start` policy must allow the action and require the independent approval. Old local-policy `actorAuthenticated=false` remains distinct from actual native authenticated account evidence.

Actual Review-provided Catalog/Coordinator, Health Registry/Fault-policy sources and Leader binding are frozen as native pointers. Complete per-target Vehicle/MissionManager/ParameterManager/VehicleLinkManager/PRIMARY pointers are retained. Numeric vehicle IDs cannot replace dead instances. Groups remain whole and ordered: at most 32 targets, 2,048 mission items per target and 8,192 total. Full record/context/source serialization is bounded by 32 MiB, depth 20 and two million nodes.

## Typed before-ARM facts, not removed reasons

Admission requires native `allControlledArmPreconditionsMatch` and each target's typed `FleetMissionStartReadiness::captureForStartup(..., Phase::BeforeArm, actualCatalog)` result. The complete captured physical map and phase fingerprint are frozen and rechecked. The new capture's original source fingerprint must equal the acknowledged Review's readiness fingerprint, preventing silent pose/risk rebasing.

This is the explicit supported takeoff/PX4 POSCTL/grounded/disarmed admission, with all other computed prerequisites present; it is not inferred by filtering a manual-arming reason out of a blocked list. Original `preconditionsMatch`, manual-arming reasons, unverified requirements, lack of clearance/neutral-pilot-input proof, local firmware declaration limitations and no-permit flags are preserved. An accepted authority basis neither proves neutral RC input nor certifies motors, ground clearance, firmware binaries, actual flight paths or task execution.

The original full RAW INT read scope/counter/epoch/cache fingerprint is retained through native `fleetMissionReadReceiptRetainedSourceMatches`. The initial actual 34-field MissionManager receipt must match the ContentCheck's stable 33-field copy (only derived age omitted). The passive validator checks actual private RAW provenance/PRIMARY/cache/last-operation identity rather than the read-sender's disarmed admission gate; it does not send a packet. **This Authority still rejects ARM/flying/state changes while it owns the scope.** Only its separately implemented native StartupTransaction may take over once and apply the independent finite step gates below.

## One-way authentication handoff and original lifetime

Ordering is strict: consume the actual conditional approval, validate its native grant, transfer its actual authentication basis to this actual Authority instance/startup UUID/authority epoch/source hash, then reserve the whole local Swarm group, recheck every source/deadline, and publish. There is no reservation while the old approval's validation is still running, which would otherwise self-deny its existing idle-owner gate.

StartApproval's native `transferToStartup` and Session's native ownership-transfer receipt retire the old consumed authentication proof once. An old ledger's automatic Review/Content expiry or expected later physical-state invalidation cannot clear the newly owned basis merely by revoking the old approval ID. Actual primary/approver SIDs, current local roles, Windows unlocked state, original primary/secondary expiry and exact owner/scope remain mandatory. No new credential prompt, approval, restored identity or lease is created.

The Authority freezes its deadline from timestamps sampled **before** fetching the original Review and ContentCheck remaining windows. Both are at most the original 30-second Review/READ START windows. It caps the deadline again using the handoff's remaining budget sampled with a timestamp taken before transfer. Session additionally caps original identity/approver deadlines. Validation consumes time; neither transfer nor getters start a fresh 30-second window. Independent native clocks are not compared directly. The entire full native before-ARM map, raw provenance, immutable contexts, original risk artifact, conditional grant, account basis, Store hash, Health gate and Catalog snapshot are bound into a bounded source hash.

After the first handoff, Authority gates do **not** call old Review/Content/Approval validity methods. They independently recheck original native source pointers and complete Store bytes, actual parameter/version/phase fingerprints, Catalog provider/snapshot, Health/Leader gate, RAW/cache/source identity, actual roles/accounts, exact local reservation and original deadline. Any actual ARM, flying, fresh pose/parameter/version/risk change, source replacement, new mission operation, role/group/identity change, lock/logout or expiry invalidates the still-owned basis. Readiness comparison excludes ONLY the full `startupCoordinateObservation` transport metadata field; its original full RAW copy remains retained, and stable coordinate risk/phase fingerprint, current native source/freshness/epoch/revisions are independently enforced before/after capture. Same-value RAW receipt/age notifications do not renew it.

## Native context and lifecycle

`authorityContext(startupId)` performs a fresh full gate; `authorityContextCurrent(startupId, expectedContext)` performs another actual gate and compares exact stable content. There are 25 fields: `schemaVersion`, `ok`, `authorityAccepted`, `startupId`, `authorityEpoch`, `authorityGeneration`, `sourceHash`, `reviewId`, `deploymentId`, `binding`, `targetVehicleIds`, `reviewContext`, `contentContext`, `deploymentContext`, `conditionalGrant`, `authenticationReceipt`, `physicalTargets`, `healthGate`, `catalogSnapshot`, `allowedFutureActionScope`, `preARMOnly`, `startupPermitGranted`, `executable`, `flightCommandReleased`, `remainingLifetimeMs`. Only the last field is volatile/excluded from stable comparison. Public `current` is a retained notification snapshot, **not** a fresh executable proof; a future native consumer must use the full native APIs. `authorityVehicle`, `authorityManager` and `authorityPrimaryLink` return actual frozen objects after passive physical/source checks.

`authenticationTransferContextCurrent` is the native transfer callback for the actual StartApproval object, complete pending native grant, startup UUID and source hash. It is not a QML authority claim. WTS/owner callback checks fence QPointer lifetime, generation, immutable scope and deadline before/after each callback. Valid ownership checks do not emit mutation notifications or renew a permission.

The local reservation lasts only this original finite window. While the Authority still owns it, `revokeAuthority(startupId)`, failure, expiry or destruction revoke the exact newly owned Session basis and release only this startup UUID's local reservation. Aircraft mission caches/content are never written by this Authority. After transfer, it no longer releases the Transaction's reservation or clears its independent authentication. Legacy PX4 `Vehicle::startMission()` changes mission mode then arms, and `MAV_CMD_MISSION_START` can arm in the referenced firmware; neither is reused as the controlled adapter.

## One transfer to the actual StartupTransaction

`actualSourcesForStartupTransaction(startupId)` returns an admission-only native eight-object bundle: `session`, `roles`, `store`, `deployment`, `swarm`, `health`, `authorizationOwner`, `authority`. `nativeFirmwareCatalog`, `nativeReviewCoordinator`, `nativeRegistry`, and `nativeFaultPolicy` return the actual frozen providers, never objects reconstructed from JSON snapshots. These getters require this accepted/live owner and a fresh actual native gate; callers freeze them before handoff and do not recursively call the old owner during transfer or later ARM.

`transferToStartupTransaction(actualTransaction, exactAuthorityContext25, startupId, sourceHash, maximumRemainingMs)` strictly casts the actual native Transaction and checks its instance epoch, exact old context, all actual source/reservation gates, and the Transaction's native pending `authenticationTransferContextCurrent` before/after the move. No generic `takeGrant`, caller permit flag, extra approval, credential refresh or arbitrary command scope is accepted. Budget begins at entry and only shortens the existing Review/READ START/account deadline.

Session moves the exact actual 22-field proof once to its private 24-field Transaction-owner proof. Authority commits `transferred`, stops its preARM timer/watchers, retires its old authentication, and sets `reservationHeld=false` / `reservationTransferred=true`. The whole group keeps the **same startup UUID and original Swarm reservation**; it is not released and reacquired. The successful ten-field wrapper is `ok`, `authenticationReceipt`, `startupId`, `transactionEpoch`, `sourceHash`, `remainingMs`, `reservationTransferred`, `startupPermitGranted=false`, `executable=false`, `flightCommandReleased=false`. Remaining time conservatively subtracts native validation/call overhead; independent clock origins are not compared. Failed/deleted new-owner handoff revokes only its exact authentication/scope and releases the matching never-reused reservation, not a replacement operation.

Old Review expiry, old automatic physical invalidation or Authority destructor cannot revoke the moved account basis or release the Transaction's reservation. Explicit `revokeAuthority(exactStartupId)` forwards only to the saved actual Transaction with matching epoch/source hash/UUID, and retires the old Authority only after that same Transaction is terminal, inactive, reservation-free and not draining (or the actual owner has been destroyed). Pending transport drain is reported; the old owner cannot prematurely release a group. Cancellation does not undo queued packets or disarm aircraft.

## Dedicated native wire facts and safety boundary

Vehicle has a native-only fixed enum `Seq0`, `NormalArm`, `MissionModeOnly` and a mandatory by-value authorization fence. The only packets are raw `MISSION_SET_CURRENT` wire sequence 0 (no QGC home-index remap), normal `COMMAND_LONG` ARM (`param1=1`, `param2=0`, all other parameters zero), and mode-only `DO_SET_MODE` (`param1=1`, `param2=4`, `param3=4`, remaining zero). No `MISSION_START`, force ARM, arbitrary parameters, legacy shared waiter, retry or callback `void*` is used. Opcode/target/payload and actual owner/source are rechecked before/after outgoing plugin and immediately before queuing. Raw replies cannot create a new step while a receive stack is active.

The retained native step receipt now uses schema 3 and exactly 33 fields (381–385): `schemaVersion`, `ok`, `requestIssued`, `startupId`, `stepId`, `operationId`, `step`, `vehicleId`, `componentId`, `sourceEpoch`, `flightSourceEpoch`, `baselineRawReceiptSequence`, `issuedAtMonotonicMs`, `expiresAtMonotonicMs`, `wirePayloadHash`, `closed`, `closeReason`, `ack`, `heartbeat`, `missionCurrent`, `receiveStackQuiescent`, `requestCausalityVerified`, `authenticated`, `signatureVerified`, `executionVerified`, `flightCommandReleased`, `queueFlightObservation`, `queueCoordinateObservation`, `queueEstimatorObservation`, `riskObservationsJournal`, `journalExhausted`, `maximumObservedRawGapMs`, `historyDurationMs`. `requestIssued` is actual queue accounting. `flightCommandReleased` is true only for queued ARM/mode, not sequence-setting; it is not an execution/safety verdict. Other verification flags stay false.

One sender lane retains at most three actual step records for the same owner/scope. RAW ACK/heartbeat/current leaves have four fields (`receiptSequence`, `receivedAtMonotonicMs`, `rawPayloadHash`, `value`), with original physical PRIMARY/system/component provenance. ACK has no operation nonce; a fresh counter/quarantine is not wireless causal attribution or hardware authentication. Seq0 has no ACK. Terminal ACK/timeout/cancel enters finite command quarantine; IN_PROGRESS cannot extend the finite wait. An observed seq0 or ACK closes only the sender lane, not the continuous risk journal.

The maximum-256-entry RAW risk journal continues through whole-group barriers until the next actual queue or exact scope cancellation, including the final MODE wait. Eight-field journal entries retain channel, native journal/RAW receipt counters, receive time, original payload hash, risk hash, full value and source. ABA/out-of-envelope-then-return remains visible. Only GP/LOCAL boot transport clock is excluded from risk hashing. Invalid/overflow history is sticky unavailable. Independent periodic HB/GP/HOME/landed/current/LOCAL receive gaps include the queue's actual previous age and the current trailing gap; unknown required evidence yields -1. MISSION_REACHED remains risk/event evidence but is not a periodic gap heartbeat. Neither gap nor phase duration uses GPS/boot clock domains.

The supplementary coordinate root has ten fields and its own source epoch; LOCAL keeps x/y/z/vx/vy/vz/timeBootMs, HOME keeps original geodetic three plus local x/y/z (NaN explicit null/unknown), local-known bool and exact uint64 decimal `timeUsec` / known bool. HOME timestamp is reference-update risk, not removed as transport time. The original six flight channels are unchanged. These facts diagnose arithmetic consistency only: they do not prove internal estimator `home.valid_alt` / `lpos.z_valid`, airborne clearance, neutral pilot input or a permitted HOME reset. Actual HOME change currently stops before MODE for new review; automatic re-review/rebase remains unfinished.

These semantics were checked against the [official MAVLink message/command definitions](https://mavlink.io/en/messages/common.html) and the actual local PX4 source, not inferred from a generic APM restart parameter. A new RAW armed/AUTO Mission observation is still not mission execution, completion, verified request causality or a full StartPermit. Engineer validation and remaining safety/firmware/clearance work remain required.

## Static delivery boundary

Only source implementation and static inspection were performed. No compiler, tests, simulator, application/native authentication, real mission read/write, ARM, mode/start command, flight or flashing was run. Dedicated command adapters and ownership source code now exist; runtime/SDK validation, actual controlled transitions, partial-dispatch behavior, late-ACK limitations, neutral pilot inputs/clearance, HOME re-review and hardware release remain engineering work. Accepted authentication/source facts do not mean the full roadmap, StartPermit or a flight-ready startup path is complete.

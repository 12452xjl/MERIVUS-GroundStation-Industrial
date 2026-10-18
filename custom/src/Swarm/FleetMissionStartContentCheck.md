# Native startup-content read/check (static source delivery)

## 536：当次完整RAW内容核对支持显式NAV朝向v3

实际raw16的param4保原有限float32，使用共享 [HeadingEncoding](FleetMissionHeadingEncoding.md) 分辨NAV绝对朝向、地理NaN默认和速度reserved-zero；非NAV有限yaw不扩支持。原期望payload允许明确v2或v3，整份canonical items／contentHash／wireSchema仍须与同一冻结载荷一致；v3只按唯一tick声明界归一，不使用任意容差、不忽略param4，也不把语义hash冒充原RAW字节hash。

原真实新READ、原PRIMARY／对象／操作、全组数量／期限、角色／审批、原read-start年龄、实际回执及后续权限门保持。读回相同不认证固件、真实姿态、因果归属或StartPermit。仅源码及静态阅读，未发READ、执行helper／算法、编译、测试或仿真。下方为历史。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestCheck、_gate、reviewContext和原scoped READ fence绑定实际Session/Deployment同组；借用部署上下文和原生物理读取返回后复核原对象，不用binding相同替代来源，取消与排空仍处理原范围。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


`FleetMissionStartContentCheck` adds an explicit, opt-in, complete onboard mission **read** before a new startup review. It does not upload, restore any review or approval, arm, start, resume, reserve remote control, or grant a StartPermit. Every public result keeps `startupPermitGranted`, `executable`, and `flightCommandReleased` false.

## Native sources and permission

`requestCheck(session, roles, store, deployment, swarm, owner, expectedDeploymentId, operatorConfirmed)` requires six actual native sources, explicit confirmation, an actual current operator identity, and the native policy's `mission.start.content.read` snapshot matching that SID and role. It invokes the private owner's exact-source callback `fleetStartContentSourcesMatch(actualContentCheck, session, roles, store, deployment, swarm)` and `fleetStartContentContextAllowed(actualContentCheck, requestId, binding, ids)`. Callback values must be actual `QVariant::Bool` true; caller JSON cannot establish authority.

Before taking its immutable snapshot it calls the actual Deployment's `prepareStartContentCheck`: old live verified staging is revoked without writing or clearing the aircraft. It freezes that runtime's full plan/payload/historical context, the latest **non-restored** Store record bytes and seven-field binding (plus paired optional generation fields), the ordered whole target set, actual Vehicle/MissionManager/ParameterManager/VehicleLinkManager/PRIMARY pointers, and their source epochs. An old upload lease or conditional start-intent approval is not authority for this read.

Only actual, initially connected, disarmed/not-flying PX4 multirotors with connected non-replay/non-mock PRIMARY links and settled parameters are eligible. The native transport also requires fresh RAW grounded heartbeat/landed-state evidence and rejects foreign parameter reads/writes, FTP, geofence/rally transfers, commands and waiters. The whole Swarm group is locally reserved before the first read; its exact reservation is checked at every gate. This is not a remote lock.

## Complete RAW evidence, not cache or ACK

Targets are read sequentially through native `loadFleetMissionForStart(primary, callerGeneratedReadScopeUUID, mandatoryByValueFence)`. The scope is fixed before native operation entry. `ownsFleetMissionRead(scope, primary, operationId)` allows only this prepared/actual read while its own transport stack is busy; it never exempts an arbitrary READ. The fence rechecks actual sources, operator/policy, owner, target/scope/generation, original stage/total deadlines, and reservation after plugin callbacks and immediately before each queue, including retries and final read-end ACK. Cancellation closes the fence before native cancellation.

Completion signals are notifications only: their boolean means `removeAll`, not success. The service waits for native finish/receive stacks to unwind, then requires the retained successful **34-field** scoped RAW receipt, actual original PRIMARY source matches before/after the getter, matching native operation/scope/source epochs, a decimal successful RAW receipt counter above the request baseline, nonzero complete count, all INT rows, and actual queued read-end ACK. Native failed/canceled reads preserve old cache but cannot reuse an old successful proof.

Each retained **16-field** RAW item keeps original sequence, command, frame, float32 parameters, x/y INT coordinates, z, current/autocontinue bytes, mission type, payload target IDs and raw payload hash. The comparator accepts only declared supported commands and explicitly maps geographic AMSL `GLOBAL_INT` (5) to canonical AMSL `GLOBAL` (0); `MISSION` (2) remains the speed command's frame. Relative/local/unknown frames are rejected. Latitude/longitude use the original E7 integers. Geographic default yaw is native NaN represented by invalid QVariant/null with `system-default` semantics; other NaN/Inf is rejected. No display/cache conversion or JSON NaN coercion supplies RAW provenance. The current-item flag is retained and type checked but excluded from immutable canonical identity; it is not execution/completion/ownership evidence.

The full canonical `mission-item-int-plan-v2` items and SHA-256 identity must equal the frozen actual PayloadBuilder output for every target. Comparing only a prefix, waypoint count, hash from caller metadata, old cache or ACK is insufficient. RAW count/item source IDs and GCS payload addressing are checked in native transport before retention.

## Finite resource and freshness policy

- At most 32 vehicles, 2,048 items per vehicle, and 8,192 items per whole group; map/tree serialization is bounded to 32 MiB, depth 20 and two million nodes.
- One 120-second batch deadline; per-vehicle budget is `min(120 s, 4 s + 250 ms × itemCount)` capped by the same original batch deadline. Slots are not extended by callbacks or retries; polling/new requests are paced at 100 ms.
- New Review evidence lasts at most 30 seconds measured conservatively from **each native original READ START**, not completion. Native start/count/first-item/last-item/completion/duration metadata is retained. Ledger and native clock domains are not compared: the ledger uses the native elapsed age, sampled with its own earlier timestamp, to cap its deadline without renewal.
- A large or slow group can finish matching all contents yet be `matched-stale` and unavailable to a new Review. Limits are resource ceilings, not guaranteed performance. Smaller groups or a manual fresh attempt may be required; there is no silent retry/renewal.
- A complete MAVLink read is sequential, not an atomic remotely locked snapshot. No echoed request nonce, hardware authentication or signature proof exists: request-attribution, atomic-snapshot, current-onboard-certainty and hardware-authentication claims remain false. Matching content alone proves no mission safety or execution.

## New Review contract and lifecycle

Native-only `reviewContext(checkId, deploymentId)` / `reviewContextCurrent(checkId, deploymentId, context)` recheck actual sources, complete RAW receipts, exact current cache identity and the original deadlines. The context has exactly 18 keys: `schemaVersion`, `ok`, `checkId`, `checkEpoch`, `checkGeneration`, `deploymentId`, `deploymentGeneration`, `binding`, `plan`, `payloadBuild`, `sourceFingerprint`, `targetVehicleIds`, `readbacks`, `allFreshOnboardReadsMatched`, `startupPermitGranted`, `executable`, `flightCommandReleased`, `remainingLifetimeMs`. **Only** the last field is volatile/excluded from stable comparison. Readbacks retain original RAW evidence plus canonical matches and explicit false attribution/authentication/atomic claims. Native `reviewVehicle`, `reviewManager`, and `reviewPrimaryLink` return the actual frozen objects, not reconstructed handles.

Active read/prequeue authorization rejects a busy identity prompt. A completed readonly content fact may survive an unrelated secondary approver prompt using the actual Session's `identitySnapshot()` (which still validates original primary SID/epoch/role/deadline/OS unlocked state); it cannot survive a replacement primary login, logout, lock or changed role/source. This avoids invalidating the new Review merely because independent-account authentication opened a modal dialog. Source checks before and after native/WTS/owner callbacks guard deletion, cancellation and reentry.

Cancellation, source loss or deletion stops further native requests. Already queued readonly packets and late replies may remain inflight; no rollback is claimed. Local reservations remain until actual manager stacks/transactions unwind, including a weak native cleanup timer if the service is destroyed from a callback. Any subsequent foreign read/write/clear/guided operation invalidates the successful content context. No result is restored from an archive.

Root integration must revoke prior Review/StartApproval/upload approval and invalidate the old live draft **before** starting this check; after a fresh successful check it creates a new Review, obtains risk acknowledgement, and independently authenticates the two real accounts. Existing technical risks and no-permit flags are preserved. This module does not implement that future permit/dispatch boundary.

## Delivery boundary

Only source changes and static contract/lifecycle inspection were performed. No compilation, tests, simulator, app/native authentication, mission read request, aircraft connection, flight, flashing or release approval was run. Engineers still own compilation, integration validation, protocol fault/late-frame/cancellation/reentry validation and hardware release.

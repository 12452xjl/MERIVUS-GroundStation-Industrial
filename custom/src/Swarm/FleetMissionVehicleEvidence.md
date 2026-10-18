# FleetMissionVehicleEvidence — native read-only parameter evidence

## 482 当前：schema5、独立到点高度参数和原29行尾核

新增 `arrivalEvidence` 恰三键 `{parameterRows,usable,sourceCurrent}`，只装 `NAV_MC_ALT_RAD` 一行；要求真实 REAL32、原数精确 float32、有限、非负且 abs≤2^53−1。原 `parameters` 十二行、`parameterProfileHash` 和公开19键观察合同不变；原两行速率守卫、14行动作模型输入也不扩容。

完整稳定 arrival 投影只去 `ageMs`，进入 `evidenceFingerprint` 与 `startupConfigurationFingerprint`；可用度还要求此行与原12／2／14同时可用、当前。所有外部getter之后，以最初 Vehicle／PM／PRIMARY弱owner和四组原稳定行调用 `parameterArrivalExecutionObservationsMemoryMatch`，一次纯内存核29项，不以新capture替原源。缺项、类型／范围失配、ABA、pending、过期或来源替换均不能回退为旧28成功。

本项补的是普通 NAV 高度接受半径的真实材料来源，不把固定 `NAV_MC_ALT_RAD` 当动态控制器接受域。实际连接审阅见 [WaypointSwitchReview](FleetMissionWaypointSwitchReview.md)；动态高度接受、平滑器可达域、真实路径和 StartPermit 仍未证明。

本批仅源码及文档静态修改；未编译、执行任何模型／helper／业务／QML、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。工程场景全部未执行，最终普通文本核对已记录于本批静态审计。下方均为历史。

## 480：schema4动作证据与原28行最终尾核

原parameters十二行与parameterProfileHash内容完全不扩容，rateOverrideEvidence两行保留；独立actionPhaseEvidence恰四键：parameterRows（固定排序14）、modelReview（[模型](FleetMissionActionPhaseReview.md)8键）、usable、sourceCurrent。完整稳定投影只去行ageMs，保fresh／available／typed值／源／epoch／revision及模型和兼容结果，进入evidenceFingerprint和startupConfigurationFingerprint。

parameterEvidenceUsable实际要求原12绑定／范围、两项速率宏及新14动作参数均鲜度可用且模型一致，并按规范profile消费明确TAKEOFF／LAND的兼容前置。COM_OBS_AVOID!=0阻明确起飞／降落，MPC_LAND_RC_HELP!=0阻明确降落；不向没有LAND的profile虚构LAND动作，不写参数或用模型期望替raw。缺参数／类型／范围／响应宏失配均不能退回旧14即通过。

最终version／catalog／source getter后调用新三集合memory gate，对最初Vehicle／PM／PRIMARY弱owner和原28稳定行作一次检查，不重采样。早行ABA、过期、pending、Fact或PRIMARY控制块改变整组拒绝。旧PM12／14口与公开19键hash观察不改；来源当前性不是完整飞行期间连续历史。schema4完整map沿既有Coordinator消费者进入风险，不依赖调用方自报许可。

斜坡参数MPC_TKO_RAMP_T的+0／−0不归一化：PM内容revision、缓存对应和原行尾核均保其符号差异。模型负零仅使TAKEOFF兼容失败并以null＋有限性／负零标志报告非finite近地加速度，无关LAND不继承该失败；原12hash不含此新增项，不把JSON数值零的显示当来源同一证明。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：两个原生速率宏观测与原十四行内存尾核

当前原生 capture 为 schema 3。原 `parameters` 仍是按名称排序的十二行，`parameterProfileHash` 仍只绑定这十二行 name/type/value，旧 profile 的 `requiredProfileHash` 和公开观察接口不被改成另一种哈希。新增 `rateOverrideEvidence` 固定四键：`guardRows`、`modelReview`、`usable`、`sourceCurrent`。guardRows 按名称保留实际 `MPC_XY_VEL_ALL`、`MPC_Z_VEL_ALL` 两行，字段及鲜度/缓存相符/待写/来源/epoch/revision检查与原行相同；负值和零不被当作缺值或替换默认值。

[`FleetMissionRateOverrideReview`](FleetMissionRateOverrideReview.md) 只做本地 PX4 源码模型的一致性复核：将两个宏及八个相关速率的实际原始参数与其应有的 float32 override 结果比较。类型不符、不可精确表达为 float32、不完整或结果不一致均失败；不把计算结果写入原始参数行，不更改参数，也不把它当作飞控内部状态已经获得证明。`modelReview` 的固定六键和全部原 guard 行保留在证据中。

`parameterEvidenceUsable` 必须同时满足原十二行/明确 profile 绑定/实际请求限制、两行 guard 新鲜可用、模型一致，以及原十四行的最终内存检查。原 `MPC_XY_VEL_MAX`、AUTO/TKO/LAND/MAX 的约束没有被宏值替代绕开。失配通过既有不可用风险进入人工审阅和启动前置检查；缺失 guard 不会退回十二行即通过。

capture 在最初原 Vehicle/PM/link-manager 上冻结 PRIMARY 指针及 weak owner 控制块。所有版本、目录及参数 getter 返回后，以 `ParameterManager::parameterExecutionObservationsMemoryMatch` 对原十二行和原两行 guard 整组核验；该方法不调用 Link virtual、不重采样、不请求、不写入。早先参数在后续 getter 中变化、ABA、pending write、原 PRIMARY 变更或鲜度到期不能由后采样覆盖。此处只补 capture 结束时的一致性，不认证整个飞行阶段的连续参数历史。

PM 仅增加两个固定 guard 的观察和读取刷新，旧十二行 `parameterObservationsMemoryMatch` 合同保留。新十四行尾核以两个独立固定排序列表接收稳定行，不接调用方任意名称子集。相关参数必须与原实际 REAL32 观察对应；原生证据及新整组尾核另保持现有风险材料的 `abs(value)≤2^53−1` 支持域。纯模型可比较更大有限 float32，不能据此扩大材料域；例如固件中可能表示宏关闭的过大负数仍明确为本地证据不支持，原因 `outside-supported-evidence-numeric-domain`，不是推定硬件损坏或自动补零。

完整 `rateOverrideEvidence` 进入 `evidenceFingerprint` 与 `startupConfigurationFingerprint`；稳定投影只去掉 guard 的推进中 `ageMs`，保留 fresh/available、源和语义 revision、模型及匹配结果。实际年龄仍保留在原生诊断。两项指纹不能恢复批准或授予 StartPermit。

这不是完整固件语义认证、实际速度/加速度/起降动力学、已编码垂直命令、时序或净空证明。着陆分段、人工输入、TKO ramp 及其它运行期限制仍需后续独立工作。未编译、测试、执行 helper/脚本、启动应用或操作设备；不修改 PX4。下方为历史记录，旧“十二行 ledger”范围以本节新增的两行 guard 为准，旧十二行内容 hash 合同保持。

244–249 adds startupConfigurationFingerprint, leaving only context.armed/flying for the actual typed phase check. Complete parameter/version/source epochs and revisions, catalog bindings, profile and other context risks remain. The original evidenceFingerprint is unchanged. This is not a permit, reference-frame certificate or caller-controlled risk rebase; no compiler, runtime or test was executed.

`capture(Vehicle*, member, constraints, maximumParameterAgeMs=30000)` reads the actual native Vehicle and its exact ParameterManager. It is not QML registered, sends no parameter requests/writes, allocates no mission items, changes no mode/arm state and never grants flight permission. Calling capture does not make a stale parameter fresh.

## Sources and boundaries

The legacy Vehicle identity, firmware version, custom-version display, capability bits, handshake/link and parameter-load state remain explicit native-cache values. Missing cached UID is unknown and absent cached version stays null. PX4's 16-hex display is not necessarily pure Git: the local firmware uses a ten-hex Git prefix plus six vendor-version hex digits. `capabilitiesKnown` can arise from initial-connect fallback and `missionIntFlagSet` reports only that cache. Schema version 2 additionally separates the real version receipt below; no cache is substituted for it. All firmware/signature/hardware/flight certifications remain false.

The twelve fixed parameters are read only through the new native `parameterObservation()` accessor: MPC_XY_CRUISE, MPC_XY_VEL_MAX, MPC_Z_V_AUTO_UP/DN, MPC_Z_VEL_MAX_UP/DN, MPC_TKO_SPEED, MPC_LAND_SPEED, MIS_TAKEOFF_ALT, NAV_ACC_RAD, MPC_YAW_MODE and MPC_YAWRAUTO_MAX. Capture does **not** call `getParameter()` (including its missing-parameter popup), refresh, cache/FTP loaders or any transport API. ParametersReady alone does not supply actual receive evidence.

Each row retains only bounded known scalar fields: actual source name/component, observed/available/fresh, monotonic receipt age, `live-param-value` source, MAV_PARAM_TYPE, finite numeric value, decimal contentRevision, managerEpoch, cached-value match and pending-write state. Twelve unknown rows remain explicit when their sources are absent; no prefix is promoted into a complete profile. Capture requires source-thread affinity, QPointer liveness and exact manager/Vehicle ownership. Age policy is 1–120000 ms, aligned with the native observation accessor; default is 30000 ms.

## Content identity and intent consistency

### Native observation ledger

`ParameterManager::parameterObservation(componentId, name, maximumAgeMs)` is a
non-QML C++ accessor with no I/O and no missing-parameter popup. Its fixed-name
ledger only accepts the vehicle's default autopilot component. The actual
`Vehicle` receive edge passes the real Link; an omitted Link is untrusted.
Exact nonzero system ID, default component, attached connected non-replay
source Link and live native context are required. A live primary Link alone
cannot certify a packet arriving through a replay secondary Link.

Each accepted receipt retains the decoded finite numeric value/type, source
Link/Fact instance, monotonic receipt time and content revision. Native float
values normalize to double for JSON-safe evidence, not a new measurement.
Repeated identical receipts may update age without changing content revision;
provenance/content/type/Fact changes do change it. Revisions and per-receipt
update tokens never wrap; exported revisions are decimal strings, not lossy
JavaScript numbers. Cache/FTP/file/offline loaders, local setters/resets,
refresh, request timeout, Fact mutation/removal, vehicle model/handshake/load
changes and primary/secondary Link lifecycle events invalidate relevant rows
or the whole manager epoch. A pending write at receipt, unfinished load or
current Fact/type disagreement cannot become usable just because the locally
cached value looks plausible.

Receipt processing publishes availability only after native Fact/load callbacks
have settled and the same update token/epoch/instances still hold. The minimal
Fact notification guards stop continuation after direct deletion or a different
nested value; they are not a full-class same-value ABA or thread-safety proof.
This ledger is twelve parameter observations, not a signed full configuration,
physical modification detector, firmware attestation or flight-performance
certificate. Refresh is an invalidation, never implicit evidence renewal.

Vehicle's native parameter-manager reference is now a QPointer, with both
public getters defined where ParameterManager is complete. Deleted managers
become null rather than a stale capture source. The receiver freezes its
Vehicle/Link and FactGroup targets and checks liveness after direct callbacks;
the parameters-ready continuation also freezes its actual manager. Initial
connection does not advance when that parameter source disappears. This does
not certify every called handler internally or provide a new destruction
notification for the existing QML CONSTANT parameterManager property.

The existing local PARAM_SET transport has a final grounded/connected policy
recheck after progress callbacks, including timeout retry. Its private helper
returns whether a send was submitted; rejection removes waiting/progress and
is neither an ACK nor a successful resend. Initial load alone remains no fresh
parameter proof: engineers need later genuine settled PARAM_VALUE receipts.

`parameterProfileHash` is SHA-256 of a compact JSON array of all twelve name/type/value rows in alphabetical name order. Age and receipt timestamps are excluded. Incomplete, invalid or out-of-range content produces an empty hash, not a partial configuration identity. A complete stale hash may remain useful as historical content identity, but never makes evidence usable.

The member's explicit executionProfile is normalized by the shared strict helper; no missing profile/default/parameter state is inferred. `verticalPolicy.requiredProfileHash` must be supplied and match the complete observed hash. Missing binding is reported as `profile-binding-missing`; changing observed contents requires a newly reviewed profile rather than silently rebinding it.

Requested cruise/return speeds must not exceed observed MPC_XY_VEL_MAX. MPC_XY_CRUISE is observed but is not forced to equal an explicit DO_CHANGE_SPEED request. If requested climb/descent values exist they must exactly equal their respective observed AUTO_UP/DN values; no tolerance, rounding, parameter write or DO_CHANGE_SPEED type 2/3 is used to manufacture agreement. AUTO_UP/TKO must not exceed observed Z_MAX_UP, and AUTO_DN/LAND must not exceed Z_MAX_DN. Speed/limit values must be positive finite numbers; MIS_TAKEOFF_ALT/NAV_ACC_RAD may be zero; MPC_YAW_MODE must be an actual integral numeric observation, not certified yaw semantics.

`evidenceFingerprint` separately binds identity/firmware/native context (including known armed/flying and parameter-load flags), normalized requested profile, the declared validated AMSL ceiling, source/type/value/contentRevision/managerEpoch, availability/freshness/cache match/pending writes and hash binding. It excludes advancing receipt age, so same-value live receipts alone do not churn it; stale transitions, explicit refresh invalidation, epoch/source/content changes, writes or binding changes invalidate it. The footprint is twelve rows and at most 32 reasons, each at most 256 characters; truncation is explicitly marked. No raw arbitrary member metadata is retained.

## Real version receipts and local adapter declarations (schema version 2)

Capture reads the non-QML native `Vehicle::autopilotVersionObservation(60000)`.
The Vehicle freezes the actual original AUTOPILOT_VERSION frame before plugin,
core or initial-connect cache callbacks, checks actual protocol sender/system/
component/Link, zero-fills legitimate MAVLink 2 trimmed tails, and commits only
the same live token/epoch after callback processing. A version-only receive
depth guard keeps nested/outstanding version callbacks unavailable. Normal
initial-connect completion can retain the same-source real receipt; close,
removal, model or primary/secondary Link changes revoke it. Default age is
60 seconds, not indefinite cache validity; no version request is sent here.

The bounded `autopilotVersionObservation` retains all twelve decoded original
payload fields and rechecks their canonical compact-JSON SHA-256, actual types,
UID/UID2-known consistency, source epoch/revision and freshness. uint64 values
are decimal strings and byte arrays remain wire-order hex. Advancing age is
excluded from stable identity; expiry and raw source/content changes are not.
The final capture rechecks the same version and catalog after parameter reads.
First version anchoring, changed twelve-field contents (including UID/UID2) or
source Link replacement revoke the twelve parameter observations, without
requesting or writing values. Same-content/same-source version refresh does
not. This establishes local receipt-order consistency, not a protocol session
token or proof that delayed pre-change wire packets cannot arrive later.

The optional native [`FleetFirmwareAdapterCatalog`](FleetFirmwareAdapterCatalog.md)
only matches explicit local engineering declarations. It requires a fresh
actual version and exact version/board/vendor/product tuple, reported INT
capability, known received identifier and the declared Git5/Vendor3 layout.
`firmwareAdapterDeclarationMatched` and `autopilotVersionEvidenceUsable` are
separate from `parameterEvidenceUsable`; none is execution permission. Missing
or unmatched declarations remain explicit. The member keeps only small catalog
binding/match data, not a repeated full directory.

The private ReviewCoordinator owns the catalog and loads its AppData file only
at construction if present; native backend methods can explicitly load/update
it later. Physical JSON edits, source-tree changes or a flashed binary are not
automatically watched or authenticated. Source-commit prefix correspondence
cannot prove a clean worktree, submodules, build options or actual binary.
Catalog changes/expiry alter live source identity and invalidate old review.
All `firmwareAdapterCertified`, `missionIntSupportCertified`, signature,
takeoff/landing, startup and execution flags stay false.

## Not an execution permit

`parameterEvidenceUsable` means the captured native parameter source, freshness, cache/write/load state, complete profile hash binding and declared parameter requirements are consistent. It does not certify firmware authenticity, command execution semantics or actual flight performance. Unknown firmware/UID are reported separately, not fabricated as approved versions or known identities.

The supplied ceiling is explicitly AMSL and profile action altitudes are checked against it, but no cached position altitude is treated as fresh flight evidence. `takeoffAltitudeComputed=false`; actual current-position/MIS_TAKEOFF_ALT paths, landing ground datum/approach clearance, geometry, timing, land-detected completion and operator/firmware approval still need independent evidence. Every capture retains `executable=false`, `executionPermitted=false`, `flightCommandReleased=false`, `actualTakeoffPathCertified=false` and `landingClearanceCertified=false`.

This source was written and statically inspected only. No compiler, test, simulator, application or real parameter/flight operation has been run.

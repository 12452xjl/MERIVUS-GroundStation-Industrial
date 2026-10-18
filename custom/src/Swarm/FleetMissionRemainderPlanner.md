# FleetMissionRemainderPlanner

## 513：独立备用出勤来源与后续余段互斥

新增的 [备用出勤规划来源](FleetReserveDispatchPlanner.md) 不冒充运行余段来源。单成员task／compiled-plan匹配明确拒绝同时携带reserveDispatchPlanningSource，不能以双来源通过本模块直接匹配。由真实新出勤Run另行取得完整原生或历史余段材料时，生成新余段task的scope替换只移除旧reserveDispatchPlanningSource；不继承旧出勤批准、Run或控制权，也不删除共同安全／能力／能量政策。旧来源schema和本模块输出不变。

本批只接后台新任务生成、真实编译及归档匹配；备用出勤的Root捕获／可见人工操作入口仍待接，完整启动许可、正式执行及工程验证均未完成。未编译、运行规划器或测试，原25目标继续。

## 478：新provenance绑定完整原域，旧内容只复现

新生成native provenance为schema3／exact29，historical为schema4／exact31；相比原1／2各新增完整canonical `routeBoundaryPolicy`，历史两项archive身份仍独立。`validateProvenance`复用Planner严格policy validator；`matchesTask`从完整task.area与assignments重算，`matchesPlan`要求双方policy存在性与全部值相等，再核原路线／constraints哈希。政策内容摘要不认证来源或飞行权限。

原1／2继续其历史合同，不能在新计划加入policy却沿用未绑定旧provenance。新提案按保留的原area重建政策，不把sourcePlan摘要当原域；sourcePlan已有policy时，完整sourceTask.area必须重建相同policy，否则整源拒绝。原单成员native18／history20来源合同不改。

历史payload的自洽比较改用独立 `reproduceHistoricalPayloads`，旧无policy资料保builder2.0原形；实际新提案显示／审阅／上传仍normal build严格检查编码后完整链。由group Run再生成single时清掉旧group来源，task／plan存在相异或混合来源均拒绝；无自动恢复Run、占用、批准或执行。

仅源码与静态审阅，未编译、运行编码／几何或测试。下方473及更早版本计数为历史。

下方为历史记录。

## 473: shared complete source validation and explicit historical provenance

`static QVariantMap validateSource(const QVariantMap& source)` is a pure-value
validator shared by the remainder builder and the separate planning archive.
It returns exactly `{ok, reason, source}`. Success has an explicit empty reason
and the complete independently JSON-owned source, including explicit JSON null;
failure has a fixed nonempty reason and `source={}`. It does not construct a
dummy request, crop a route, estimate movement, access a Run or filesystem,
modify state, notify, authenticate a source or grant permission.

The complete original source checks have moved out of `build` into this single
function: bounded whole tree, exact schema/fields/claims, Run identities and
generation, full binding, full plan hash, exactly one matching original member,
full PayloadBuilder reconstruction and matching complete target payload, and
current/reached source-item mapping. Every retained source field survives; there
is no normalized subset, successful prefix or substitution from a current
Draft/PlanStore. JSON round-trip ownership changes only JSON-compatible scalar
storage widths, not source values or original explicit null. Whole-source
budgets stay 32 MiB/depth24/1,048,576 nodes with the existing smaller item limits.

Two closed source contracts are now accepted:

- Native: schema1, exactly the original 18 keys, with
  `sourceKind="native-run-planning-snapshot"`. No archive keys are permitted.
- Historical: schema2, exactly those 18 keys plus `sourceArchiveId` and
  `sourceArchiveContentHash`, with
  `sourceKind="historical-run-planning-snapshot"`. The added values must be a
  canonical nonzero UUID and a lowercase SHA-256 string. All remaining source
  fields, payloads, policies and progress checks are unchanged.

Version/kind/key mismatches, missing or extra archive identity, bad hashes,
payload or mapping mismatch all reject the complete source. This helper only
checks the *format* of an archive identity/hash: it cannot recompute or
authenticate an archive envelope it was not given. The separate Archive owns
the full file-envelope validation; Root retains the private selected file
source. A caller can construct internally valid values, so validation alone
does not prove either actual native ownership or historical authenticity.
An imported source must never be renamed native to pass an old code path.

`build(source, request)` first consumes this shared validation result and then
uses the same complete cropping, request, timing, retained-policy, energy-floor,
profile and item-budget algorithm as before. Native provenance remains schema1
and **exact28**. Historical provenance is schema2 and **exact30**: its kind is
historical and it additionally retains the original `sourceArchiveId` and
`sourceArchiveContentHash`. Central `validateProvenance`, `matchesTask` and
`matchesPlan` accept only these two corresponding schemas/kinds, retaining all
existing complete derived-task and route/constraints hashes and count checks.
Compiler and PlanStore use these same centralized consumers; no old approval
or native owner can be recovered from provenance.

The build result remains exact7. The native summary is now **exact23**, the old
22 fields plus `sourceKind`; historical summary is **exact25**, additionally
carrying the two archive fields. The complete `sourceProgress` remains a
historical hint. A stored `sourceCurrent` value does not become a current
aircraft observation after load, and neither terminal/reached observations nor
a complete payload prove completed work. `historicalOnly` remains true;
`progressUsedToSkip`, `completionProven`, `authorityRestored`,
`flightCommandReleased` and `executable` remain false where defined.
The operator still chooses an inclusive
waypoint and replays its entire hold, supplies new endpoints/speeds/profile,
and must pass a completely new current-evidence review. Absolute schedules are
still rejected, not silently erased or converted to estimates.

This is source implementation/static review only. No helper, parser, archive
read/write, build, test, application, simulation or device was executed.
Engineers must verify native18/history20 and provenance28/30 round trips,
explicit null, schema/kind/extra-field rejection, archive ID/hash propagation,
corrupt or mismatched full plan/payload/mapping, all resource limits, retained
timing/policy blocks and complete new-review handoff. This does not deliver
full startup, automatic resume, verified completion or an execution permit.

## 433: an independently reviewed, inclusive remainder proposal

This pure C++ helper constructs a **new local planning task** from one frozen
native Run target. It does not resume a flight, change the old plan, modify a
Run, cancel an aircraft, upload, send a command or restore approval. Native
ownership comes from the actual Startup source getter and Root's retained
source identity, not from caller-supplied maps or hashes.

The operator chooses an original **zero-based outbound waypoint index**. The
selected waypoint is included, with its **entire original hold**, followed by
every later outbound waypoint. There is no automatic `current + 1`,
`reached + 1`, nearest-point selection, completed-area inference or subtraction
of allegedly elapsed hold time. MAVLink item sequences include speed, takeoff
and terminal actions and are not original waypoint indices.

## API and source

`build(source, request)` returns exactly seven fields:

- `ok`, `reason`, `task`, `summary`, `provenance`;
- `flightCommandReleased=false`, `executable=false`.

On failure the three output maps are empty; no accepted prefix is published.
The success reason is an explicit empty string. This class is not a QObject
and has no invokable transport, Vehicle, timer, filesystem or permission API.

The source is the exact 18-field result of
`FleetMissionStartupTransaction::remainderPlanningSource(expectedRunId, id)`:
`schemaVersion`, `ok`, `reason`, `sourceKind`, `runId`, `startupId`,
`transactionEpoch`, `transactionGeneration`, `deploymentId`, `binding`,
`sourceVehicleId`, `sourceTask`, `sourcePlan`, `sourceMember`, `sourcePayload`,
`progress`, `readonly`, `flightCommandReleased`.

In the original 433 native branch, only successful
`native-run-planning-snapshot` values are accepted; 473 adds the separately
labelled historical branch above. The native
source contains the original captured intent parameters, not a task inferred
from the current area editor. The helper checks the exact historical binding,
plan content hash, unique original member, and deterministically rebuilds the
original PayloadBuilder output. The complete target payload, including its
source-item mapping, must agree with the frozen payload. This is a local
encoding comparison, not an aircraft mission read.

The actual progress row must belong to that target and committed MODE window.
If current/reached sequences are present, their source-item mappings must
match the frozen payload. Retired or source-unknown observations remain valid
**historical hints**; no current source or mission-completion claim is made.
Progress never determines the selected starting index.

The source tree is bounded to 32 MiB JSON, depth 24 and 1,048,576 nodes; maps
have at most 1024 keys, lists at most 32768 entries and strings at most 32768
UTF-16 code units. Opaque objects, non-finite values and invalid numeric/null
representations are rejected. Explicit JSON null remains usable in historical
progress and default-yaw payloads. These are source limits, not permission to
exceed the existing Builder's smaller mission-item limits.

## Explicit request

The request has exactly nine fields:

| Field | Required meaning |
| --- | --- |
| `sourceWaypointIndex` | Integer 0 through original outbound count minus one, inclusive |
| `targetVehicleId` | Integer 1..255; may equal the original member ID |
| `start`, `returnTo` | Each an exact five-field geodetic/WGS84, AMSL coordinate |
| `cruiseSpeedMetersPerSecond` | Finite, explicitly greater than zero and at most 1000 |
| `climbSpeedMetersPerSecond` | Same input bounds; not an aircraft capability certificate |
| `descentSpeedMetersPerSecond` | Same input bounds |
| `executionProfile` | Map: empty means geometry only; otherwise whole-profile normalization |
| `operatorConfirmed` | Actual boolean true |

Endpoint fields are `coordinateSystem=geodetic`, `altitudeReference=amsl`,
`latitude` (-80..80), `longitude` (-180..180), and `altitude` (-1000..20000).
They are explicit operator input, not Home, current position, ground height,
or automatically copied primary-aircraft endpoints.

A supplied new profile must use relative dwell, match the requested cruise
speed, and match explicitly declared vertical rates. An explicit return speed
is used for the new return estimate. The original aircraft's profile and
parameter hash are **not** adopted. Endpoint/profile inconsistencies continue
through the ordinary Compiler/Builder checks as blockers; this helper does not
repair takeoff, landing, HOLD, RTL or parameter requirements.

The whole request is bounded to 128 KiB. UI text-size limits are additional
limits, not a substitute for native value validation.

## Geometry, timing and scope

The generated outbound path is `new start + original [index..last]`. The
return path is exactly `outbound endpoint + new returnTo`. The return connector
has zero hold so the original final outbound hold is not repeated. Original
outbound heading requirements remain on retained outbound points; generated
connectors invent no yaw requirement.

Each leg uses sequential horizontal-distance/cruise plus positive-altitude/
climb or negative-altitude/descent allowance. This does not generate an
obstacle-free vertical corridor or certify how an aircraft travels the leg.
Point elapsed estimates and member `estimatedDurationSeconds` contain
**movement only**; the existing Coordinator adds explicit holds once.
The summary's outbound total includes ingress, retained movement and holds.

Old absolute times, authoritative timing evidence, deadlines/start windows,
or an `absolute-required` profile are explicitly rejected. Estimated timing,
including the actual `constant-speed-estimate` coverage producer, is replaced
by the new speed-based estimate. Absolute semantics are never silently erased.

The original source task is copied before recognized old scope, geometry,
reserve/role-count/dispatch fields and captured runtime review evidence are
removed. Safety, capability, energy, template, risk, no-fly and other bounded
policies remain. The original `area` remains as area/boundary context: it can
supply the original required geofence, but does not request a fresh coverage
allocation without `coverage`. Original compiled constraints and conservative
task duration floors remain. Every explicitly supplied original member
`durationSeconds`, `missionDurationSeconds`, `taskDurationSeconds` and
`timeLimitSeconds` must be valid and positive; their maximum is retained as a
new task duration floor. An explicit original `returnDurationSeconds` is
combined conservatively with both task and constraint return-reserve aliases,
and the resulting floor is also written into the new constraints before route
hashing. The existing 120-second default applies only when neither scope has
an explicit return-reserve floor; invalid values are not repaired.
`minimumSpeedMps` is raised, never lowered, to cover the new
horizontal speed assumptions so existing capability review can check them.

The new scope is one explicit assignment with the original role and selected
target ID, zero reserves, and a separate remainder target description. Old
coverage and `planningSource` are not reused: doing so would regenerate the
full area rather than the selected suffix. Retaining a policy can correctly
block the new single-member task, for example when its template requires a
larger fleet; it is not silently weakened to make the proposal pass.

Outbound, return, speed/takeoff/terminal actions together must fit **2048
items** for this single target; the existing whole-batch 8192 limit is therefore
also respected. Both output paths contain at least two points, and return
begins at the exact outbound endpoint. The whole derived task is bounded to
16 MiB. Direct ingress and return connectors have no obstacle/airspace or
ground-datum certification.

## Provenance and integration checks

`remainderPlanningSource` has exactly 28 fields:

`schemaVersion`, `sourceKind`, `runId`, `startupId`, `transactionEpoch`,
`transactionGeneration`, `deploymentId`, `binding`, `sourceVehicleId`,
`sourcePayloadHash`, `originalRouteHash`, `sourceWaypointIndex`,
`sourceWaypointCount`, `targetVehicleId`, `targetRole`, `outboundPointCount`,
`returnPointCount`, `sourceTaskPolicyHash`, `derivedTaskHash`, `newRouteHash`,
`selectionPolicy`, `timingBasis`, `historicalOnly`, `progressUsedToSkip`,
`completionProven`, `authorityRestored`, `flightCommandReleased`, `executable`.

`historicalOnly` is true. The final five booleans are false.
`selectionPolicy` is `inclusive-waypoint-full-dwell-replay`; `timingBasis` is
`sequential-horizontal-climb-descent-estimate`. The provenance contains no
recursive original plan or live QObject references. Its maximum size is 64 KiB.

`validateProvenance` returns exactly `{ok, provenance, reason}`. This proves
bounded shape and internal counts only, not native authenticity.
`matchesTask` checks the complete derived task hash after excluding
`fleet`, `intent`, `taskId`, `remainderPlanningSource` and freshly recomputed
`templateValidation`. No safety-policy fields are excluded from this hash.

`matchesPlan` independently requires one corresponding member, identical
canonical `assignments` and `members`, no reserves, and no coverage plan. Its
shared `newRouteHash` projection includes target ID/role, coordinate references,
return-required flag, both complete paths, the whole new profile if provided,
movement/distance estimates, and complete compiled constraints. Compiler-only
bookkeeping such as `memberIndex` does not cause a false mismatch. Thus the
Compiler and archive importer can check the actual generated plan rather than
accepting a provenance label unrelated to its routes.

`sourceTaskPolicyHash` identifies the retained original policy input before
the explicit new task scope is installed. Hashes provide consistency and
traceability, never remote authentication or restored permissions.

## Summary and actual product path

The original 433 summary had exactly 22 fields (473 adds the source-kind and,
for historical inputs only, archive metadata described above): `sourceVehicleId`, `targetVehicleId`,
`sourceWaypointIndex`, `sourceWaypointCount`, `retainedWaypointCount`,
`outboundPointCount`, `returnPointCount`, `estimatedIngressSeconds`,
`estimatedTaskSeconds`, `retainedHoldSeconds`, `estimatedOutboundSeconds`,
`estimatedReturnSeconds`, `sourcePayloadHash`, `originalRouteHash`,
`newRouteHash`, `executionProfileProvided`, `sourceProgress`, `selectionPolicy`,
`routeClearanceVerified=false`, `performanceCertified=false`,
`missionCompleted=false`, `flightCommandReleased=false`.

Root must capture the actual selected native source, bind local input and
confirmation to that source, and then explicitly submit the generated task to
the ordinary `prepareFleetIntent` / native review / Compiler / energy,
airspace, risk and PayloadBuilder / PlanStore review path. A helper result is
not a reviewed plan or an upload action. A new record never edits the original
immutable record. Current/native source ownership, fresh review evidence,
operator authority, occupancy and all later upload/start gates still apply.

Implementation and static source review only. No helper, parser, application,
compiler, test, simulation, hardware operation, upload or flight was executed
for this change. Engineer validation of the complete actual workflow remains
required.

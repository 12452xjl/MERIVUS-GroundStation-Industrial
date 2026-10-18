# Reserve standby and full-route replacement energy proposal — stage 432

## 483：备用独立预算的当前工程性能来源

`build(reserveDescriptor,primaryMembers,constraints,reserveVehicle,performancePolicy)` 新增第5个原QVariant参数，默认实际Bool false，四参旧调用保原结果shape；null／字符串／数字不修成false。Compiler原两处真实备用构建均传task策略原值，不把策略混进constraints或改变原约束。

true时要求[exact11当前声明](FleetCapabilityEvidence.md)属于该备用机且规范最大速度known，独立cruise不得超本机声明。仍算自身起点／返场点、待命、进入目标主机第一点、完整重做原出航路线和自身返航，不借主机速度／续航，也不变成中断点接续。严格结果增加 `requirePerformanceEvidence,declaredPerformanceEvidence,declaredPerformanceEvidenceUsable`；后者只表示helper声明有效，不认证完整预算可行。

false／absent保旧raw速度合同；true仅消费既有批准数字，没有新增批准／性能录入器、真实耗能模型、接替命令或飞行许可。工程验证全部未执行，未编译／执行／测试；下方432为原范围与公式历史。

## Scope

`FleetReserveEnergyPlan` is a pure value calculator registered in `custom.pri`.
It does not look up a live Vehicle, activate a reserve, change a primary,
command a standby mode, upload a route or grant/restore approval. A complete
result means the bounded **planning estimate can be calculated**, not that a
reserve has flown, can safely take over, or has a validated battery model.

The scenario is deliberately explicit: the reserve starts at its own declared
start, travels to the nominated primary's first compiled waypoint, redoes that
primary's **entire outbound waypoint route**, including the primary's original
ingress if present, then returns from the final target waypoint to the reserve's
own declared return endpoint. It is not a remaining-route or interruption-point
handover, does not infer current progress, and does not reuse the primary's
return route, launch/end actions, timing schedule or speed.

Ingress and return are direct geometry proposals. Being calculable or inside a
survey area does not prove obstacle clearance, geofence compliance, airspace
availability, collision separation, takeoff/landing clearance or command support.
Both paths remain independent reserve inspection geometry, never active mission
members. Existing primary budgets must remain untouched when one reserve fails.

## Input contracts

`normalize(policy)` returns exactly `{ok, policy, reason}`. Failure has an empty
policy and a non-empty stable reason. Success has a canonical eight-field policy
and an explicit non-null empty reason. Strings/booleans/null are not numbers;
finite JSON numeric forms are accepted and canonicalized.

The policy has exactly:

| Field | Required value |
| --- | --- |
| `schemaVersion` | Numeric 1; canonical output Int 1 |
| `replacementVehicleId` | Integer 1..255 |
| `standbyMode` | `ground-disarmed` or `airborne-hold` |
| `standbyDurationSeconds` | Explicit finite 0..86400 |
| `standbyConsumptionFactor` | Explicit finite greater than 0 and at most 10 |
| `cruiseSpeedMetersPerSecond` | Explicit finite greater than 0 and at most 1000 |
| `climbSpeedMetersPerSecond` | Explicit finite greater than 0 and at most 1000 |
| `descentSpeedMetersPerSecond` | Explicit finite greater than 0 and at most 1000 |

The broad speed domain is an input bound, not a recommended aircraft performance
range. No speed or standby factor is inherited from the primary. `ground-disarmed`
does not imply zero power; even a zero-duration standby retains the explicit
factor rather than manufacturing a reusable zero-cost ground policy.

`build(reserveDescriptor, primaryMembers, constraints, reserveVehicle)` accepts
descriptor keys `vehicleId`, `start`, `returnTo`, `reservePlan`. Missing keys
produce an explained incomplete plan; unknown descriptor keys are rejected.
Start/return endpoints are exact five-field geodetic/AMSL coordinates. The
helper requires a unique nominated primary among at most 255 primary members
with unique numeric IDs, and rejects a reserve also present as a primary.

The target must have an actual non-empty compiled `waypoints` list. Every point
must explicitly be geodetic/AMSL with canonical latitude, longitude, altitude
and consecutive zero-based sequence. Latitude is -90..90, longitude -180..180,
and AMSL altitude -1000..20000 m. Optional canonical heading/timing metadata is
type/size checked before hashing but never used as reserve timing. Only explicit
`holdSeconds` in 0..86400 is charged as dwell. Local frames, mixed/implicit
datums, unknown point fields, missing routes and global fallback routes are not
silently converted. No datum or current position is inferred from the fleet.

When an altitude ceiling is supplied, it must explicitly be an AMSL
`maxAltitudeMeters`/`maxAltitudeReference` pair, and all endpoints/target points
must fit it. Absence of a ceiling is not a clearance claim. Other constraints
remain the responsibility of the surrounding review/risk pipeline; this helper
does not interpret time limits as observed execution schedules.

A non-empty `reserveVehicle` must identify the same reserve through its supplied
`systemId`/`vehicleId`/`id` aliases. A known nonnegative `maxSpeedMps` declaration
must cover the requested cruise speed. Missing or exact -1 is unknown, allowing
geometry estimation with `speedCapabilityKnown=false`, never certification.
Malformed or conflicting identities/capabilities fail. No vertical capability
is invented from a horizontal maximum-speed declaration.

## Calculation

For every route leg, horizontal distance is the geodetic surface distance;
reported path distance combines this with the altitude difference. Time is the
sequential allowance:

`horizontalDistance / cruiseSpeed + positiveAltitudeChange / climbSpeed + negativeAltitudeChangeMagnitude / descentSpeed`

This sum is a conservative axis-sequential **timing model**, not newly generated
vertical corridors and not a guarantee about the actual autopilot trajectory.
Acceleration, turns, heading changes, wind, payload, battery ageing and actual
phase-dependent power are not modeled. Primary timestamps and estimated elapsed
times are ignored; a new aircraft cannot inherit the old route's schedule.

`standbyEquivalentFlightSeconds = standbyDurationSeconds * standbyConsumptionFactor`

The factor is an engineering declaration of standby consumption relative to
nominal flight consumption, not a measured power ratio. The assumption that a
reported remaining-time estimate uses the same nominal load is **unverified**.
The helper does not turn stage 431 autopilot reports into measured endurance.

`missionEquivalentFlightSeconds = standbyEquivalentFlightSeconds + estimatedIngressSeconds + estimatedTaskSeconds + targetHoldSeconds`

`estimatedTaskSeconds` includes only movement along the whole target route.
Every target hold is counted once, including a final hold. The copied final
coordinate starting `returnPath` has hold zero so it cannot charge that final
target hold twice. `estimatedReturnSeconds` is the reserve's own return leg.
Caller task-duration floors and reserve margins are applied later by the
Coordinator; they do not become route timing evidence.

## Fixed result

Build always returns exactly 29 keys:

- `schemaVersion` (Int 1), `complete` (Bool), `reason` (QString).
- `vehicleId`, `replacementVehicleId` (parsed IDs or -1), `policy` (canonical
  policy once valid, otherwise empty map).
- Double `standbyDurationSeconds`, `standbyEquivalentFlightSeconds`,
  `estimatedIngressSeconds`, `estimatedTaskSeconds`, `targetHoldSeconds`,
  `missionEquivalentFlightSeconds`, `estimatedReturnSeconds`,
  `outboundDistanceMeters`, `returnDistanceMeters`.
- `targetRouteHash` (lowercase SHA-256 of the checked original compiled target
  waypoint list's compact Qt JSON, or empty on failure).
- `outboundPath`, `returnPath` (lists of budget geometry).
- `targetWaypointCount` (Int actual target count, or -1).
- `speedCapabilityKnown` (Bool), `declaredMaxSpeedMetersPerSecond` (Double,
  declaration or -1).
- `timingBasis="sequential-horizontal-climb-descent-estimate"`.
- `loadBasis="declared-standby-to-nominal-flight-ratio"`.
- `targetRouteMode="full-primary-route-redo"`.
- `routeClearanceVerified`, `performanceCertified`,
  `nominalLoadAssumptionVerified`, `executable`, `flightCommandReleased`: all
  fixed Bool false, including complete results.

Each output point has exactly seven keys: `sequence`, `coordinateSystem`,
`altitudeReference`, `latitude`, `longitude`, `altitude`, `holdSeconds`. Paths are
not encoded flight instructions. The reserve start is appended before all
target points without dropping repeated coordinates/holds; return has two
points with zero hold. Original heading/timing remains bound by the target hash
but is not copied into the inspection geometry as an execution policy.

Failure has `complete=false`, non-empty reason, all calculated numeric metrics
and point count -1, empty paths and hash, and no successful prefix. Already
parsed identities and a valid policy can remain for explanation. Missing policy
uses `reserve_policy_missing`; malformed fields/values, missing endpoints or
target, invalid point/frame, altitude exceedance, incompatible speed, path
limits and non-finite budgets have their own `reserve_*` reason codes.

## Resource and integration boundaries

Each actual output path is at most 4096 points. The outbound reserve start counts
toward that limit: a 4096-point target would require 4097 outbound points and is
rejected without truncation, so accepted full-redo targets have at most 4095
points. Return contains two points. Target JSON has a 2 MiB limit after bounded
per-point canonical validation. Numeric sums must remain finite.

The Compiler additionally enforces an all-reserve 32768-point output budget.
If that group budget is exceeded, every reserve result is made incomplete with
empty paths/unknown metrics, not a successful prefix; the original primary
mission is retained. It may obtain the standard failure shape by calling
`build` with only the reserve ID, then substitute the group-level failure reason.
The surrounding plan content hash must include the independent reserve result.

The Coordinator/Scheduler/Risk consumers must treat missing/incomplete reserve
budgets as unavailable reserves, not fall back to the longest primary allowance.
A calculated reserve budget does not prove capability equivalence to its target
role or justify automatic reassignment. Those are separate review requirements.

## Verification status

Source implementation and static reading only. No compilation, test, helper
execution, application, simulator, firmware operation or aircraft operation was
performed. Engineering still must verify declarations, actual aircraft capability,
nominal/standby power relationship, route safety and behavior before operational
use. This proposal is not an installation, flight or reliability acceptance.

# Native battery observation — stages 431 / 460

## 516: independent bounded SOC-decline observation

`Vehicle::fleetBatteryDrainObservation(maximumAgeMs = 3000)` is a new native,
non-invokable, callback-free getter. It does not change the existing schema1/2
battery envelope or row contracts below. It observes only successful original
PRIMARY/default-component `BATTERY_STATUS` commits, never Facts, metadata,
caller maps, recorded files or a timer-generated percentage. No request or
configuration command is sent.

### Time, source and bounded history

The new channel freezes its own battery-clock timestamp at the earliest RAW
entry, before provider probes or firmware/plugin callbacks. The old battery
channel retains its existing later receipt-time contract. Their ages need not
match millisecond-for-millisecond. Both must independently be fresh; neither
age proves the remote sensor's measurement time. The new ingress also freezes
the actual PRIMARY weak control block and pointer, vehicle/component/model,
version epoch and original receipt baseline. Only the original completed
transaction's revalidated row/token can append history; nested or replaced
ingress cannot be rebased onto a later report.

A source/window clear never carries a prior successful window forward. An
original same-PRIMARY frame that is subsequently accepted by the old complete
transaction may seed only a new warm-up after that clear; this first point is
not a known decline. A new complete continuous window of at least30 seconds
and a decrease of at least2 percentage points is still required.

Each of the existing bounded battery slots privately retains at most256 SOC
points over120 seconds: approximately one point per second for flat reports,
and every SOC change. Every accepted RAW report, including omitted flat points,
checks the actual local gap. Unknown percentage, a rise, role/type change,
backwards/invalid time, a gap greater than3000ms, discarded ingress or source
retirement breaks the window. A sample-limit failure latches unknown for that
slot until source retirement; it does not publish a favourable prefix. Source
retirement clears the independent model in memory before disconnect callbacks.
Battery IDs still identify report slots, not physical pack identities; same-ID
pack replacement, charging, capacity, calibration or battery topology is not
certified.

A window is published only on an actual SOC decrease, with duration30000 through
120000ms and a decrease of at least2 percentage points. Its maximum gap is
computed from all accepted RAW reports between retained points. `sampleCount`
counts retained points, not every packet; duration must not be reconstructed
from that count times the maximum RAW gap. Only one unambiguous fresh ALL or
PROPULSION battery can supply the aggregate; old unknown-role/multiple-flight/
slot-overflow restrictions remain. Auxiliary batteries never become propulsion
estimates.

Flat packets refresh only the latest real battery receipt/age. They do not
extend a published window's endpoint or dilute its rate. Independent semantic
revision advances for a new published decline, or when a previously published
cache is cleared by a real source/window break. Warm-up without a published
cache, identical receipts and natural aging do not advance it. Counter
exhaustion makes this new channel permanently unknown, without upgrading or
changing the old battery schema.

### Fixed value contract

The top level has exactly25 keys:

- Int `schemaVersion=1`, `vehicleId`, `componentId`, `batteryId` (unknown ID=-1).
- Non-null QString `reportType="native-soc-decline-window"`, `sourceEpoch`,
  `state`, `reason`; canonical uint64 QString `contentRevision`,
  `batteryContentRevision`, `batteryReceiptSequence`.
- Bool `sourceCurrent`, `observed`, `drainKnown`.
- qint64 `ageMs` (new latest RAW age, or-1); QVariantMap `window`.
- Double `percentPerSecond`, `remainingFlightSeconds` (-1 when unknown).
- Bool `snapshotOnly=true`; `authenticated`, `performanceCertified`,
  `remoteMeasurementTimeVerified`, `configurationPermitted`, `executionPermit`,
  `flightCommandReleased` all false.

`batteryContentRevision` is the old whole-ledger revision, not its row revision.
`batteryReceiptSequence` is the selected row's latest real receipt, including
flat packets. Selection, component and source identity correspond to the old
native battery getter. `observed` means that getter supplied an actual selected
fresh flight row; it can be true with unknown SOC or an unavailable new RAW age.
Without selection, batteryId=-1, latest receipt="0", age=-1 and observed=false,
although a current but ambiguous source can still have sourceCurrent=true.
Wrong-thread/invalid-age calls return the unavailable default. The allowed
maximumAgeMs is1 through3000; getters do not call Link virtuals, lock a strong
link owner, emit, authenticate or probe the remote aircraft. Any active whole
RAW/battery receive makes the new model unavailable.

`window` has exactly8 keys: canonical uint64 QString `startReceiptSequence` and
`endReceiptSequence`; Double `startPercent` and `endPercent`; qint64 `durationMs`,
`maximumGapMs`, `endAgeMs`; Int `sampleCount`. A complete window has ordered
positive receipts with endReceipt<=latest selected receipt, percentages0..100,
startPercent-endPercent>=2, duration30000..120000, gap0..3000 and count2..256.
Its endPercent must equal the current selected SOC. The endpoint receipt stays
fixed across flat reports. Unknown windows use both sequences="0", both
percentages=-1, all three time/gap fields=-1 and count0; no warm-up prefix is
exposed. Only `stale-decline-window` may retain the complete original window as
history, with endAge>30000 and within the JS-exact integer range; its rate and
remaining value remain-1 and drainKnown=false.

Closed states are `source-unavailable`, `receive-in-progress`, `no-observation`,
`slot-limit-exceeded`, `unknown-battery-role`, `multiple-flight-batteries`,
`no-flight-battery`, `stale-flight-battery`, `unknown-soc`, `warming-window`,
`stale-decline-window`, `sample-limit-exceeded`, `counter-exhausted` and
`reported-decline`. Native reason equals state except successful
reported-decline, whose reason is the non-null empty string.

### Estimate and use boundary

Known requires both the current selected original battery report and the new
latest RAW receipt to be fresh within maximumAgeMs, a complete published window
whose endpoint age is at most30000ms, and no source/topology/capacity ambiguity.
The local estimate is:

`rate = (startPercent - endPercent) * 1000 / durationMs`

`remaining = max(0, currentSOC / rate - max(ageMs, window.endAgeMs) / 1000)`

Using the older effective age prevents repeated flat packets from restoring
time to an already published decline estimate. Flat/no-decline data does not
imply zero consumption or infinite endurance. Current SOC0 can yield a known
estimated0 only with a valid positive decline rate; unknown remains-1.

This is a local observation-based SOC model, not physical range/performance,
sensor timing, authentication, a complete pack topology or a flight permit.
Actual Registry/source binding and shared evidence validation must remain the
source of consumer inputs. Energy consumers may only lower their existing
valid estimate with this value; it cannot independently create known energy,
replace negative/fault evidence, raise declared endurance or restore approval.
No compilation, decoder/helper/parser/model/JS/QML execution, test, replay,
application, simulation, device/PX4 operation, command, PR or package was run.

## 460: explicitly observed charge state and fault mask

The native envelope remains exactly 20 fields and now reports `schemaVersion=2`.
Each battery row has 19 fields. The four new fields are part of the original
raw report's semantic value, not a Fact/display-cache interpretation:

- `chargeStateKnown` is Bool. It is true only for an original MAVLink2 frame
  whose length is greater than wire offset 40 and whose decoded charge state
  is not `MAV_BATTERY_CHARGE_STATE_UNDEFINED` (0).
- `chargeState` is Int, 1..255 when known, otherwise -1. Future nonzero wire
  states are retained as reported; this native getter does not classify them
  as healthy or decide their task eligibility.
- `faultBitmaskKnown` is Bool. It is true only for an original MAVLink2 frame
  whose length is greater than wire offset 50.
- `faultBitmask` is qint64, 0..4294967295 when known, otherwise -1. The signed
  64-bit container preserves every uint32 bit, including its highest bit,
  together with the explicit unknown sentinel. A present zero mask does not
  certify a healthy battery or supported fault detection.

MAVLink1 and unobserved extensions stay unknown. A MAVLink2 uint32 extension
with a partial zero-trimmed tail is decoded using the existing explicit
zero-filled payload; it is not rejected merely because all four bytes were
not transmitted. Original length and version still determine knownness.

The private report value grows from 6 to 10 fields. Charge/fault value or
knownness changes participate in the existing full-value comparison and
advance the row and whole-ledger semantic revision even when percent and
remaining time are unchanged. Identical packets only refresh receipt/age.
The getter does not change the closed aggregation states, selected battery,
percent/remaining-time estimates, source/depth checks, or permission fields.
Shared consumers must separately interpret explicit faults; no `healthy`
claim is inferred from unknown extensions or a zero mask. Old schema1 maps
cannot be silently treated as schema2 observations.

Only static source and text review was performed; no compilation, decoder
execution, replay, helper/parser/script, test, application, UI or device run.

## Implemented source path and limits

`Vehicle::fleetBatteryTelemetryObservation(maximumAgeMs = 3000)` is a native C++
read-only getter, not a `Q_INVOKABLE` operation. It observes original
`BATTERY_STATUS` frames at `Vehicle::_mavlinkMessageReceived`, before firmware,
core-plugin, Fact, or observer callbacks. It does not request telemetry, send a
command, modify PX4, certify endurance, or create/restore an approval.

The source is the current actual Vehicle instance, its original MAVLinkProtocol,
VehicleLinkManager and MultiVehicleManager, the actual PRIMARY link's weak
ownership identity, matching system/default-component IDs and version epoch.
Offline-editing, mock, replay, disconnected, removed, replaced or wrong-thread
sources cannot supply a current observation. Other links/components do not
refresh this ledger. A source break retires its rows and epoch; the original
native owners are not reconstructed by numeric ID or from an imported map.

The public getter performs only native memory continuity checks (the original
owners, threads, toolbox, unique Vehicle, version/model/component identity,
PRIMARY weak control block, owning link entry, communication-lost flag and
receive/lifecycle state). It does not call Link virtual methods, acquire a
temporary strong link owner, emit a signal or invoke a callback. Actual
connected/non-mock/non-replay checks remain at raw ingress and commit, with
source revalidation after each virtual call. Disconnect/destroy/source-change
hooks retire the ledger. `sourceCurrent` at read time therefore means local
source continuity, not a fresh network probe, physical-link certification or
proof that the remote vehicle is reachable at that instant.

Receipt sequences advance only on accepted original packets, even when their
values do not change. Semantic revisions advance on changed report values,
new slots, overflow and invalidation, not because a timer/getter was called.
An in-progress receive remains fenced across all downstream callbacks. A
nested same-source receive cannot publish a still-in-flight row; abandoned
pending observations are retired. Counters and receive depth do not reset on
nested invalidation. Counter exhaustion fails closed.

This is independent of the existing SYS_STATUS/ESTIMATOR_STATUS, flight-state,
ODOMETRY and startup contracts. Existing battery Facts remain display caches;
their `telemetryAvailable`, a periodic Registry refresh, HIGH_LATENCY reports,
and vehicle heartbeat timestamps are not battery freshness evidence.

## Report semantics and aggregation

The getter accepts a local reception-age limit of 1 through 120000 ms. The
default is 3000 ms. Age is measured with a process-local monotonic clock since
the actual packet entered Vehicle. BATTERY_STATUS provides no remote battery
measurement timestamp; local freshness does not prove remote sampling age,
truth, authentication, usable capacity, a physical pack serial number or a
known complete hardware topology.

MAVLink2 zero-tail trimming is decoded from an explicitly zero-filled payload.
MAVLink1 lacks the remaining-time extension. Missing extension, zero or negative
`time_remaining` means unknown remaining time; it is never a measured zero-second
capability. Percent is known only for a reported integer in 0..100. A positive
remaining-time report is an autopilot **estimate**, not measured endurance.

For a valid report, locally aged remaining seconds are
`max(0, reportedRemainingSeconds - ageMs / 1000.0)`. A positive original report
can therefore age to a known estimated zero. This arithmetic is not a battery
physics model, a remote countdown guarantee, a sensor calibration, or proof
that the intended mission can finish. Percent is the original reported percent;
no synthetic percent is derived from remaining seconds.

At most 16 battery-ID slots are retained. Seeing a seventeenth distinct slot
latches `slot-limit-exceeded` for that source incarnation and disables aggregate
values; it does not silently select 16 favourable batteries. A source retirement
clears the set and overflow latch. Battery IDs identify report slots, not
physical battery serial numbers; same-slot pack replacement is not certified.

Aggregation is available only for exactly one observed `ALL` (1) or
`PROPULSION` (2) battery and a fresh complete report from the current source.
Any UNKNOWN or undefined role blocks aggregation. Two flight-role slots block
aggregation even when one is old. `AVIONICS` (3) and `PAYLOAD` (4) slots are
displayed separately, are never averaged into the flight battery and do not
block the single flight-role estimate merely because their report is old.
No supported serial/parallel multi-pack topology or capacity-weighted model is
implied. The two aggregate Known flags are independent: a fresh flight report
may provide percent but no remaining-time estimate, or the reverse.

## Fixed value contract

Top level has exactly 20 keys:

| Keys | Native type and meaning |
| --- | --- |
| `schemaVersion` | Int 2 |
| `vehicleId`, `componentId` | Int actual IDs; 0 in unavailable default output |
| `sourceEpoch` | QString source UUID, or empty before a source is initialized |
| `sourceCurrent` | Bool, native memory source continuity; not authentication or remote reachability |
| `rows` | QVariantList, 0..16 rows in ascending battery ID order |
| `aggregationState` | QString from the closed set below |
| `batteryPercentKnown`, `remainingSecondsKnown` | Bool; aggregate flags require a fresh unambiguous flight-role report |
| `batteryPercent` | Double 0..100 or -1 when aggregate percent is unknown |
| `remainingFlightSeconds`, `reportedRemainingSeconds` | Double aged/original estimate or -1 when aggregate time is unknown |
| `ageMs` | qint64 selected report's local receipt age, or -1 |
| `batteryId` | Int selected slot 0..255, or -1 |
| `receiptSequence` | Canonical decimal QString selected report sequence, or `0` |
| `contentRevision` | Canonical decimal QString whole-ledger semantic revision, including topology changes; `0` before any revision |
| `readonly` | Bool true |
| `authenticated`, `performanceCertified`, `flightCommandReleased` | Bool false |

Only `reported-estimate` copies a selected row into the aggregate values. Every
other state retains false aggregate Known flags, -1 aggregate values/age/ID and
sequence `0`; whole-ledger revision is still retained. An invalid argument or
wrong-thread call returns the unavailable default envelope. An unavailable
source or active receive returns no rows, rather than borrowing old-source rows.

Closed aggregation states:

- `no-observation`
- `source-unavailable`
- `receive-in-progress`
- `slot-limit-exceeded`
- `unknown-battery-role`
- `multiple-flight-batteries`
- `no-flight-battery`
- `stale-flight-battery`
- `reported-estimate`

Each row has exactly 19 keys:

- Int `batteryId`, `batteryFunction`, `batteryType` (wire domains 0..255).
- Bool `observed`, `fresh`, `batteryPercentKnown`, `remainingSecondsKnown`,
  `chargeStateKnown`, `faultBitmaskKnown`, `sourceCurrent`.
- Int `chargeState` (1..255 when known, otherwise -1).
- qint64 `faultBitmask` (0..4294967295 when known, otherwise -1).
- qint64 `ageMs` (local reception age, otherwise -1).
- Double `batteryPercent`, `reportedRemainingSeconds`, `remainingFlightSeconds`
  (unknown values are -1).
- QString `receiptSequence`, `contentRevision` (decimal), `sourceEpoch` (UUID).

Row Known flags describe whether that complete observed report contains a
valid field; **they do not imply `fresh`**. Old rows retain their reported values
for explanation, while top-level aggregate Known flags stay false. Rows emitted
by this getter belong to the same current epoch and have `sourceCurrent=true`.

## Consumer and persistence boundary

Consumers must use the top-level aggregate flags and source/freshness result,
not select a favourable row themselves. Source epoch and semantic revision can
identify the report incarnation/content, while dynamic age and aged seconds
must not be confused with a new report. Identical new packets can refresh age
without changing the semantic revision.

No descriptor, passport, JSON archive, UI map, saved plan or historic receipt can
install this native ledger. Portable evidence is historical display material;
it cannot restore current battery availability. The stage 431 Registry adapter
must obtain a new actual Vehicle projection when taking a planning snapshot,
and must not make an imported old `online`/`lastSeenUtc`/percentage current.
Static declared endurance remains a separate model, not a certified property
created by the new report path.

## Verification status

Implemented and reviewed as source only. No compilation, tests, MAVLink replay,
helper execution, application launch, simulation or hardware operation was
performed in this stage. Engineering still needs to verify the actual firmware's
role/ID/remaining-time reporting, cadence, multi-battery configuration, clock
and reconnect behaviour, and the operational conservatism of any mission
energy model. This implementation is not a flight release or a reliability proof.

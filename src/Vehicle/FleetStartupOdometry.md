# Passive ODOMETRY source contract

280–284 新增[显式局部运动与航向观察政策](../../custom/src/Swarm/FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

## 275–279 native integration and handoff

280–284 adds a policy-specific periodic clock for explicitly reviewed startup
policy version 2. The actual transaction's pure frozen-policy lookup decides
whether it is required; no caller flag can choose it. Native queue-time receipt
age, every original ODOM packet (including equal content), and the closed-step
tail contribute to the existing maximumObservedRawGapMs. A later fresh packet
does not erase an earlier excessive gap. The clock uses GCS monotonic receipt
time, never timeUsec or a LOCAL_POSITION_NED sample-clock comparison. Version 1
does not acquire this extra periodic prerequisite, and step32/flight6 remain
unchanged. Pose is not appended as a seventh risk-journal channel.

Version 2 requires actual NED pose/NED velocity, explicit reset metadata,
AUTOPILOT8, valid primary observations and unchanged native reference history.
LOCAL_POSITION_NED uses vehicle_local_position, whereas ODOMETRY uses
vehicle_odometry: these checks do not prove the same origin, same sample,
North/reference validity, HOME branch or physical continuity between messages.
The independent observation envelopes authorize none of those claims or a
final StartPermit. No stream request, firmware edit or runtime work was done.

The helper is registered once in `qgroundcontrol.pro` (header and source) and
`src/Vehicle/CMakeLists.txt` (Vehicle target), not as a new custom QML type.
It does not enable/request an ODOMETRY stream or require a new sensor. The local
PX4 stream implementation exists, but that does not prove the configured rate,
the current aircraft binary, or that a message was received.

Native-only Vehicle APIs are `startupOdometryObservation(maximumAgeMs=3000)`,
`startupOdometryReferenceObservation(maximumAgeMs=3000)`, and
`startupOdometryObservationSourceMatches(expectedPrimary)`. The independent
ledger does not change the old flight six channels, coordinate10/row13,
mission read34 or startup step32 contracts. The full native observation has
exactly 27 fields:

```text
schemaVersion, vehicleId, componentId, sourceEpoch, flightSourceEpoch,
odometrySourceEpoch, observed, available, fresh, ageMs, source,
receiptSequence, contentRevision, contentHash, rawPayloadHash, rawPayloadLength,
referenceRevision, referenceHash, receivedAtMonotonicMs,
clockDiscontinuityObserved, clockDiscontinuityRevision, everObservedRawFrame,
value, authenticated, signatureVerified, executionPermit, flightCommandReleased
```

The reference getter removes receiptSequence/contentRevision/contentHash,
rawPayloadHash/rawPayloadLength/receivedAtMonotonicMs/value and adds
referenceProjection, giving 21 fields. Readiness alone removes its advancing
ageMs to freeze 20 fields; observed/available/fresh, source epochs, reference
revision/hash, clock-change history and sticky everObservedRawFrame remain.
The full getter preserves the complete bounded decode50 and original wire hash;
it is not a public QML payload or a startup permission.

Actual pre-plugin MAVLinkProtocol sender, nonmock/nonreplay live PRIMARY,
system/default component, PX4 multirotor, initial-connect and actual version/
flight epochs are checked independently of decoding. The whole ODOM receive
stack is guarded, including rejected/filtered frames. RAW prepare counts
reference changes before plugin callbacks; a shadow hash and nonwrapping
revision retain observed B-to-A changes during nested receipts. Completion and
discard use the exact token and epoch, never overwriting a newer nested row.
Source/context invalidation creates a new odometry epoch and clears the row,
but it does not reset monotonic counters or everObservedRawFrame.

everObservedRawFrame becomes sticky after actual source checks, before decode,
including an invalid frame. A previously seen stream that was invalidated,
filtered, discarded or lost cannot become an initial optional missing stream.
New admission needs a new actual fresh, valid observation; a file, same hash or
cleared row cannot restore the previous basis. Initial missing data stays
unknown and is not a valid local origin or same-sample claim.

Sample-time decrease records clockDiscontinuityObserved and a separate
nonwrapping revision in RAW prepare. It is neither a reboot proof nor a clock
mapping to GCS, LOCAL_POSITION_NED or HOME. Only observed metadata changes are
detected; unseen resets or a whole 8-bit counter cycle cannot be inferred.

[Readiness](../../custom/src/Swarm/FleetMissionStartReadiness.md) freezes the
stable supplement, [ground READ](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)
retains it throughout collection, and [Transition](../../custom/src/Swarm/FleetMissionStartupTransition.md)
rechecks the actual native reference before and after its existing checks.
No full pose/time/quality metric enters this stable identity. Existing strict
HOME/local branch restrictions stay; origin/validity, movement/turn policy,
clearance, final StartPermit and execution completion remain development work.
Only source and limited text/static review were performed, not compilation,
parsing, tests, business API calls, simulation or hardware work.

`FleetStartupOdometry` is a non-`QObject`, static helper. It only constructs
bounded `QVariantMap` evidence from a received message. It does not send a
message, modify PX4, access hardware, authenticate a source, establish a
navigation reference, or authorize arming/startup.

The input to `decode(original, accepted)` is the original MAVLink message before
firmware-plugin adjustment. `accepted` means only that the envelope is MAVLink
2 (`MAVLINK_STX`), message 331 and payload length 1 through 233. A rejected
envelope returns an empty map and sets `accepted=false`; a decodable but unknown
or physically invalid observation is retained with explicit status flags.
The caller owns receipt time, freshness, source identity, any clock-change
history, and the independent hash of the original wire payload. This helper
does not calculate `rawHash`.

The checked local definitions are:

- `libs/mavlink/include/mavlink/v2.0/common/mavlink_msg_odometry.h`: full length
  233, base/MIN length 230, quaternion w/x/y/z, and both 21-entry covariances.
- `libs/mavlink/include/mavlink/v2.0/message_definitions/common.xml`, ODOMETRY
  message 331: units, covariance sentinel, and extension-field meanings.
- `libs/mavlink/include/mavlink/v2.0/mavlink_helpers.h`: MAVLink 2 trailing-zero
  trimming may continue below base/MIN length, with at least one payload byte.
- Read-only primary source
  `E:/feikong/FirmwarePX4/src/modules/mavlink/streams/ODOMETRY.hpp`: sample time,
  frame selection, reset source and sparse covariance production.

## Wire presence and clocks

The decoder copies only `original.len` payload bytes into a zero-initialized
message and invokes the generated decoder with length 233. Both generated
decode branches therefore see zeroes rather than residual bytes after a legal
MAVLink 2 truncation. A missing trailing zero byte is a protocol default, not
proof that the producer populated or supports an extension.

| Extension | Payload offset | Wire-explicit presence |
| --- | ---: | --- |
| reset_counter | 230 | `original.len > 230` |
| estimator_type | 231 | `original.len > 231` |
| quality | 232 | `original.len > 232` |

Decoded numeric zero and wire-explicit presence are saved separately. An absent
reset counter has `resetCounterUnknown=true`; even an explicitly present zero
does not prove that no reset occurred, that the source supports reset tracking,
or that reset histories from different sources are comparable. An 8-bit
counter can wrap. No reset history is established by one message.

`timeUsec` is the exact unsigned 64-bit decimal `QString`, including values
above JavaScript's exact integer range. Its magnitude does not select epoch or
boot time: `timestampClockDomain="unknown"` and
`timestampRelation="unproven"`. The local PX4 stream assigns
`odom.timestamp_sample`, but generic received ODOMETRY does not prove that
producer implementation or a clock relation to another message. Timestamp
equality, sequence number, receipt time, or apparent clock continuity does not
prove the same sample as LOCAL_POSITION_NED, HOME_POSITION or another stream.

## Numeric semantics

Position is x/y/z in metres, quaternion is w/x/y/z as received, linear velocity
is vx/vy/vz in m/s, and angular velocity is rollspeed/pitchspeed/yawspeed in
rad/s. A finite float is retained as an exactly representable double. The
decoder neither normalizes nor repairs the quaternion. A wholly finite
quaternion outside `abs(sum(q_i*q_i)-1) <= 0.01` is marked invalid; this is a
documented observational sanity tolerance, not an attitude or health proof.
A finite component whose squared magnitude exceeds 1.01 is also invalid when
another quaternion component is unknown. All received components stay intact.

Every numeric group contains a fixed-length `values` list and matching `states`
list. NaN and either infinity are JSON-safe null (`QVariant()`) in `values`.
States are `finite`, `nan_unknown`, `positive_infinity_invalid`,
`negative_infinity_invalid`, or `negative_variance_invalid`. NaN remains
unknown; infinity is invalid. Negative finite covariance diagonal variances
are invalid and remain numerically visible. Negative off-diagonal covariance
is not rejected merely for its sign. No generic flight-envelope threshold is
invented for finite positions, speeds or covariance magnitudes.

Covariances retain all 21 raw upper-triangle elements in row order:
`(0,0)..(0,5), (1,1)..(1,5), (2,2)..(2,5), (3,3)..(3,5),
(4,4)..(4,5), (5,5)`. Pose states are x/y/z/roll/pitch/yaw; velocity
states are vx/vy/vz/rollspeed/pitchspeed/yawspeed. A NaN first element sets
`unknownByFirstElement` as specified by MAVLink; other NaNs are individual
unknown entries and are never filled with zero. `partial` means finite and
NaN entries coexist, even when the first-element sentinel is present.

The local PX4 producer first assigns NaN to both full arrays. Pose fills only
indices 0/6/11/15/18/20, and velocity fills only 0/6/11. Those sparse patterns
are retained and are not rejected for their NaNs. In particular, unknown
angular-rate covariance does not make all transmitted velocity components
invalid. NaN in a primary position/quaternion/velocity/rate component makes
`observationValid=false` but remains distinct from `observationInvalid`.

`frameName` and `childFrameName` label the exact protocol enum. LOCAL_NED,
LOCAL_FRD, BODY_FRD and LOCAL_ENU are not converted into each other. LOCAL_FRD
does not prove North alignment; BODY_FRD travels/rotates with the vehicle.
Deprecated BODY_NED semantics depend on whether the value is position or
velocity. `frameKnown`/`childFrameKnown` mean a defined, non-mission,
non-reserved coordinate-frame enum, not suitability for a startup branch.
Mission, reserved and unknown frame values are retained and marked invalid.

`qualityState` is `failed` for -1, `unknown` for omitted/zero, `metric` for
1..100, and `invalid` outside -1..100. Failure sets observation-invalid.
A metric, including 100, is not estimator health, usable reference, covariance
validity or an arming proof. Estimator zero/omission remains unknown; an
explicit out-of-enum estimator value is retained and marked invalid.

`observationValid` only means the primary numeric components are finite and
the documented type/range checks have not found an invalid condition.
`observationUnknown` can coexist with it (e.g. sparse covariance or omitted
quality). `observationNonFinite` includes both legal covariance NaNs and
invalid infinities, so it is not by itself a rejection rule. None of these
flags establishes flight readiness, PX4 `z_valid`, `home.valid_alt`, same
sample, coordinate origin, EKF branch, or reference validity.

## Exact bounded map shapes

A successful decode has exactly **50 top-level fields** and
`decodeMapShape="fleet-startup-odometry-v1"`:

```text
decodeMapShape, schemaVersion, messageId, protocol,
wireLength, zeroFilledBytes, basePayloadBytes, fullPayloadBytes,
timeUsec, timestampClockDomain, timestampRelation,
frameId, frameName, frameKnown, childFrameId, childFrameName, childFrameKnown,
resetCounter, resetCounterPresent, resetCounterUnknown,
estimatorType, estimatorTypeName, estimatorTypePresent, estimatorTypeKnown, estimatorTypeUnknown,
quality, qualityPresent, qualityState, qualityUnknown, qualityValidRange,
position, quaternion, linearVelocity, angularVelocity, poseCovariance, velocityCovariance,
observationValid, observationUnknown, observationNonFinite, observationInvalid,
authenticated, signatureVerified, referenceValidity, canArm, startupPermit,
startupComplete, flightReady, sameSampleProven, producerExtensionSupportProven, resetHistoryKnown
```

`schemaVersion=1`, `messageId=331`, `protocol="mavlink2"`;
`basePayloadBytes=230` and `fullPayloadBytes=233`.

| Nested group | Exact fields | List lengths |
| --- | --- | --- |
| position, linearVelocity, angularVelocity | values, states, anyUnknown, anyNonFinite, invalid (5) | 3 values + 3 states each |
| quaternion | values, states, anyUnknown, anyNonFinite, invalid, unitNormWithinTolerance, unitNormSquaredTolerance (7) | 4 + 4 |
| poseCovariance, velocityCovariance | values, states, anyUnknown, anyNonFinite, invalid, unknownByFirstElement, partial (7) | 21 + 21 each |

There are six nested maps, 36 nested fields, 12 lists and 110 list elements
per decoded observation. There is no unbounded source list, payload list,
covariance matrix expansion, or message history in this helper.

`referenceProjection(decoded)` returns empty for an unrecognized map shape.
The shape gate requires the exact 50-field key set, marker/version/message/protocol
and six bounded numeric-group shapes; it is a codec interface gate, not source
authentication. Its successful output has exactly **42 top-level fields** and
`referenceMapShape="fleet-startup-odometry-reference-v1"`:

```text
referenceMapShape, schemaVersion, messageId, protocol,
frameId, frameName, frameKnown, childFrameId, childFrameName, childFrameKnown,
resetCounter, resetCounterPresent, resetCounterUnknown,
estimatorType, estimatorTypeName, estimatorTypePresent, estimatorTypeKnown, estimatorTypeUnknown,
qualityPresent, qualityState, qualityUnknown, qualityValidRange,
position, quaternion, linearVelocity, angularVelocity, poseCovariance, velocityCovariance,
observationValid, observationUnknown, observationNonFinite, observationInvalid,
authenticated, signatureVerified, referenceValidity, canArm, startupPermit,
startupComplete, flightReady, sameSampleProven, producerExtensionSupportProven, resetHistoryKnown
```

The projected position/linearVelocity/angularVelocity maps each have exactly
`anyUnknown, anyNonFinite, invalid` (3); quaternion additionally has
`unitNormWithinTolerance` (4); the two covariance maps additionally have
`unknownByFirstElement, partial` (5 each). There are 23 nested fields and no
lists in the reference projection. The raw quality integer stays only in the
decoded observation. The projection retains quality classification and wire
presence, so changes among unknown/failed/invalid/metric remain metadata
changes, while normal 1..100 metric refreshes do not invalidate every sample.
Those classification changes, including A/B/A, require history in the caller.

Time, wire length, zero-fill count, numeric values, covariance contents and
quaternion norm/tolerance numbers are omitted from the projection. Ordinary
time advance and motion with unchanged metadata therefore do not themselves
change this projection. A caller may separately retain discontinuity history
and epochs; comparing two equal projections cannot detect an intervening
A/B/A source/reset/quality change that the caller did not retain.

In both maps, `authenticated`, `signatureVerified`, `referenceValidity`,
`canArm`, `startupPermit`, `startupComplete`, `flightReady`, `sameSampleProven`,
`producerExtensionSupportProven`, and `resetHistoryKnown` are always false.
The projection is metadata for invalidation/audit, not a permit or a branch
proof. It does not establish PX4 `z_valid` or `home.valid_alt`.

## Verification limits

This increment is source-only. The implementation and local primary sources
were reviewed with bounded text reads. No compilation, parser execution,
tests, business/system/hardware runtime calls or live flight interaction were
performed. No FirmwarePX4 source was changed. Any review of this helper is a
limited static review, not evidence of runtime correctness or flight readiness.

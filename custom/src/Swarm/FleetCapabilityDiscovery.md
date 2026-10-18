# Fleet capability discovery contract

This document describes the descriptor consumed by `FleetRegistry` and
`FleetCapabilityMatcher`. The discovery transport is intentionally separate
from the registry: a MAVLink handler, replay loader, or future MAVSDK adapter
can publish the same `QVariantMap` without coupling the product model to one
backend.

The C++ `FleetCapabilityDiscovery` class is the transport-neutral normalization
entry point. An adapter passes decoded maps for `heartbeat`,
`autopilotVersion`, `componentMetadata`, `parameterSnapshot`, and a local
`trustedManifest`; the class returns a descriptor and a separate validator
reports errors/warnings. It does not parse raw MAVLink frames or issue flight
commands.

## 386–390: current capability-review semantics

Discovery normalizes declarations; it does not approve capabilities.
`AUTOPILOT_VERSION`, component metadata and a map named `trustedManifest`
no longer automatically produce effective `verifiedCapabilities`. Their
capability strings, including legacy verified lists, become
`declaredVerifiedCapabilities` with a declaration-only diagnostic state.

In an actual Registry snapshot, `verifiedCapabilities` is derived only from
that Registry's current private, revision-bound local engineering review.
`capabilityEvidenceState` is diagnostic and cannot activate a review. The
[capability evidence contract](FleetCapabilityEvidence.md) defines the shared
six-key `capabilityReviewContext`, fourteen-key `capabilityApproval`, canonical
tokens and current-review checks used by planning consumers. A copied or
self-consistent caller map is not native provenance or authentication.

The earlier interpretation of protocol/manifest capability strings as
automatically verified is historical and is superseded by this section.

## Resource limits

Capability/evidence lists contain at most 128 entries of at most 128 raw
characters per entry. Decoded sources and descriptors are bounded to depth 8,
2048 nodes (including nulls), 128-character keys, and 128 KiB cumulative
key/string/byte-array data. Primitive QVariant values and QDateTime are
accepted; arbitrary QObject/custom types and non-finite doubles are rejected.
An over-limit source returns only a bounded invalid diagnostic and, when
unambiguous, a numeric system ID; no rejected configuration is hashed.
Merged configurations are rechecked before hashing. Registry also stages
partial merges and checks the stored aggregate, so repeated small updates
cannot bypass the retained metadata budget. These limits do not authenticate
an evidence source or prove physical performance.

`SwarmController::_refreshFleetRegistry()` now uses the same normalizer for
the basic QGC heartbeat descriptor before adding live battery/link/health
evidence. This keeps the ordinary live snapshot on the shared descriptor
contract, but it is intentionally only `partial` discovery: the controller
does not invent `AUTOPILOT_VERSION`, component metadata, parameter snapshots,
or a trusted baseline. An engineer adapter must still supply those decoded
sources when full capability discovery is required.

`buildDescriptor()` checks all identity aliases across heartbeat,
`AUTOPILOT_VERSION`, and component metadata. If any source supplies an
invalid or conflicting ID, the result is marked `discoveryState=invalid` and
does not retain a registrable identity instead of silently choosing the first
source. Static capability fields prefer protocol/metadata sources over the
low-trust heartbeat; dynamic health fields remain heartbeat-first.

`validateDescriptor()` rejects malformed map fields (including implicit string
or fractional identity conversions, non-boolean health flags, non-finite or
out-of-range numeric values, invalid timestamps, and inconsistent identity
aliases), invalid configuration fingerprints, a baseline source without
corresponding evidence, and a baseline configuration map whose hash disagrees
with `baselineConfigurationFingerprint`. It reports a supplied baseline map
without a fingerprint as a warning rather than silently treating it as
approved evidence.

Calling the ordinary `FleetRegistry.registerVehicle()` or
`updateVehicle()` path does not grant approval: baseline fields are ignored on
that telemetry-facing path. Explicit passport import and the native
`approveCurrentConfiguration()` method manage configuration baselines only;
neither approves capability tokens. Capability review uses the separate
native workflow below.

The registry ingress is also strict about identity and live evidence: all
present `systemId`/`vehicleId`/`id` aliases must resolve to the same integer in
1--255, booleans must be actual boolean values, numeric health/performance
fields must be finite and within their documented range, and a supplied
configuration fingerprint must be a 64-character SHA-256 hex value matching
the supplied configuration map. `metadata.configuration` is treated as the
same static evidence as top-level `configuration`; hash-only observations that
change the fingerprint clear a stale retained map so exported passports stay
internally consistent. Invalid partial updates are rejected without mutating
the previous vehicle snapshot. Explicit `lastSeenUtc`/`lastSeen`
values must be valid ISO timestamps; a timestamp more than the small allowed
clock-skew window in the future cannot keep a vehicle online and is treated as
stale by the registry health refresh. Capability-passport import additionally
rejects unknown root/approval fields and invalid export timestamps before the
atomic replacement.

Fault status has an explicit `faultHealthKnown` flag and
`faultEvidenceAgeMs` field. `hasFault=false` is only a cleared placeholder
when the flag is false; it is not a no-fault assertion. A non-negative age is
required for a health/leader/mission gate to treat the assessment as current,
and the registry bounds it by its offline timeout. The live QGC adapter now
uses the actual native `SYS_STATUS` and `ESTIMATOR_STATUS` observations from
`Vehicle::fleetHealthTelemetryObservation()`, with per-channel receipt records,
source/semantic identities and ages measured by the local monotonic receive
clock. A Registry timer does not synthesize another packet or reset its age.
A current fault-health assessment requires both streams and uses the older
age; missing, stale or changed sources do not establish current no-fault
evidence or receive a new age `0`. Explicit bad values remain conservative.
Independently fresh position evidence and the corresponding estimator bits
are also required for known position health. The
[native health telemetry contract](../../../src/Vehicle/VehicleHealthTelemetry.md)
defines the exact sources, shapes and existing startup EST integration.

These are local receipt ages, not proof of a remote sample time, hardware
identity, message signature, estimator correctness or flight clearance.
Older descriptions of this adapter publishing a fresh synthetic age `0`
from cached health checks are historical, not the current implementation.

The same pairing rule applies to `FaultToleranceManager` partial reports:
setting `faultHealthKnown=true` without a non-negative
`faultEvidenceAgeMs` is rejected, and a non-negative age without a matching
known flag is rejected. Clearing the known flag without an explicit age also
clears the retained age, so an old `0` cannot make a later assessment appear
fresh. `FleetHealthCenter` uses the configured `offlineTimeoutMs` rather than
duplicating a fixed three-second window.

`FleetRegistry.updateVehicle()` follows the same clearing rule for an explicit
`faultHealthKnown=false` partial update, keeping the stored age at `-1` unless
the adapter deliberately submits an explicit clearing age.

## Static capability evidence

The descriptor should include the normal identity/capability fields already
accepted by `VehicleCapability` and, when available, a static `configuration`
map. A suitable descriptor shape is:

```json
{
  "systemId": 7,
  "vehicleType": "multirotor",
  "autopilotType": "PX4",
  "firmwareVersion": "1.15.x",
  "capabilities": ["guided", "gnss", "relay"],
  "declaredVerifiedCapabilities": ["relay"],
  "configuration": {
    "board": {"vendor": "px4", "model": "pixhawk-6c-mini"},
    "airframe": {"type": "quad-x", "motorCount": 4},
    "esc": {"protocol": "dshot", "model": "unknown"},
    "battery": {"cells": 6},
    "parametersHash": "<source-provided-or-computed-value>"
  }
}
```

`configuration` is static evidence only. Do not put battery percentage,
RSSI, estimator status, timestamps, or other heartbeat data in it. The
registry computes `configurationFingerprint` as a deterministic SHA-256 over
the recursively key-sorted map. When the map is present, the registry and
normalizer recompute the hash locally. An explicit valid hash can be retained
as a hash-only observation when the map is unavailable; it is not a trusted
approval, evidence of a complete parameter read or a physical inspection.
The example's declared capability list does not satisfy verified-only policy.

The resulting state is:

| State | Meaning |
| --- | --- |
| `unknown` | No configuration evidence was received. |
| `unverified` | Evidence exists but no approved baseline was supplied. |
| `baseline` | Observed fingerprint equals the approved baseline. |
| `modified` | Observed fingerprint differs from the approved baseline. |

The baseline is an approval record, not a claim that the ground station can
physically inspect every part. A hardware/parameter change is therefore
reported as evidence requiring review, not silently declared safe or unsafe.
The normalizer can describe a locally supplied manifest baseline with
`baselineSource: "trusted_manifest"`; an existing ground-station baseline uses
`local_approval`. Vehicle telemetry cannot establish either through ordinary
Registry ingress: baseline claims are stripped there. Explicit passport import
checks its baseline/approval-record contract, but this remains local baseline
data, not capability approval or signature verification.

The native `approveCurrentConfiguration()` method creates a `local_approval`
baseline after the integrating backend's review. The
matching `rollbackConfigurationBaseline()` call only removes that ground
station approval; it does not restore hardware, parameters, or props.

## Native capability review, change and import

The positive backend path is:

1. Obtain the actual Registry's `capabilityReviewContext(vehicleId)`.
2. Review the current configuration and intended capability scope, then call
   `approveCurrentCapabilities(vehicleId, exactContext, capabilities, reason)`.
   The context must still match; the submitted canonical capability set must
   be non-empty and bounded, and the reason must be non-empty and at most
   1024 characters. Engineering declarations may include a token absent from
   observed telemetry, such as `thermal`; this does not certify installed
   hardware or measured performance.
3. Use `revokeCapabilityApproval(vehicleId, exactContext, reason)` for explicit
   revocation. Re-approval requires a new current context, not an old receipt.

These are C++ native APIs, not QML approval/write methods. This batch adds no
operator authorization UI, dedicated Session approval purpose or account
permission service. The eventual integrating backend must provide real actor
authorization; a native method or a reason string is not account identity.

The context binds the Registry epoch, per-record epoch, vehicle ID, monotonic
`capabilityRevision`, static identity fingerprint and configuration fingerprint.
A legal hash-only configuration can be reviewed; a present map must hash to
the current fingerprint. Normal health age, receipt/time progression and
approval date/reason are excluded from the stable identity.

Reported changes to configuration, static identity, observed/declared
capabilities, performance/canUse claims or configuration-baseline state advance
the revision and supersede an old current review. Approval/revocation also
advance it. A→B→A does not restore old approval, and counter exhaustion does not
wrap. `unregister`/re-registration use a new record epoch; `clear` changes the
Registry epoch. This detects changes in supplied evidence, not unreported
propeller, motor, battery or payload modifications, or every same-ID physical
replacement.

Ordinary telemetry cannot write the private review record. Incoming legacy
verified labels and imported approval capability sets remain declarations;
one bounded imported approval map is retained as `importedCapabilityApproval`
for diagnosis only. Passport import rebuilds the record epoch for each touched
vehicle and never activates imported approvals. Incremental import preserves
native reviews for vehicles not touched by the import.

Normal snapshot round-trips have one precise exception: the full read-only
context/approval/derived-verified group is ignored as an echo only when it
matches that vehicle's actual private projection. This prevents an unchanged
snapshot from inventing a new declaration and revoking itself. It cannot
restore an old review; import never uses this exception. Explicit static
changes still supersede the current review, and contradictory newly supplied
top/metadata evidence is rejected rather than silently merged.

## Recommended discovery sources

1. Identity and protocol: `HEARTBEAT`, `AUTOPILOT_VERSION`, and the vehicle's
   component IDs.
2. Firmware/build: autopilot version fields and component metadata when
   available.
3. Board and sensor topology: component metadata or an independently checked
   vehicle manifest. This normalizer does not verify manifest signatures.
4. Frame/ESC/battery/parameter evidence: a trusted vehicle manifest or a
   parameter snapshot. Use a source-specific `parametersHash` rather than
   treating a transient parameter read as a complete physical inspection.

If a source cannot provide a field, omit it. Missing evidence must remain
`unknown`/`unverified`; it must not be replaced with a nominal default.

## Mission gate

Existing planning tasks keep their compatibility defaults. A task can
explicitly require both local capability review and a configuration baseline:

```json
{"requireVerifiedCapabilities": true, "requireConfigurationBaseline": true}
```

Verified-only planning consumes only current revision-bound engineering review,
not a naked verified/declaration list, even when required capability tokens are
empty. Ordinary planning may combine observed tokens with current reviewed
tokens. A required baseline remains a separate check; `configurationState`
must be `baseline`. The shared checks also apply to roles, templates,
continuity suggestions and final compiler-resolved members. Compiler policy
mismatches are blocking; it does not repair them by silently selecting another
aircraft or changing routes. See
[the capability evidence contract](FleetCapabilityEvidence.md) for the actual
consumer and stable-review identity chain.

Neither policy is hardware, measured speed/endurance, account authentication
or a final flight permit. Review changes require renewed planning review;
already queued commands and retained Run/ownership facts are not rolled back
or automatically released. The original 25 development goals and six remaining
work categories continue, including real source/identity integration,
authorization UI and numeric performance validation. This batch is source and
documentation work only: no compilation, tests, helper/business/authentication
execution, application/UI, simulation, devices, PX4 edits or flashing were
performed.

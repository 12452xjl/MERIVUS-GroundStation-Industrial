# FleetMissionDraftStore — explicit local editing checkpoint (472)

## 521：编辑检查点保留性能下限来源6

存储代码继续复用中央Draft editing-source validator，不另建schema解析器。任意一个显式`minimumSpeedMps`／`minimumEnduranceSeconds`（包括0）使用planningSource schema6，完整保留其presence、数值和独立strict三态；没有下限的旧来源仍为1..5。旧schema带下限、6缺下限、错误类型或损坏完整源均由中央校验拒绝，不静默丢键或降级。

原保存预览与内容hash仍覆盖完整settings；Root版本预检及实际Compiler／Store／Payload消费者同步认识新来源。恢复仍是pristine本地副本、revision1与datum重确认，不恢复性能证据、原账号、批准或机载状态。新编辑器pending继续进入原待处理门，未提交拟值不会混进检查点。仅源码和静读，未执行保存／恢复、hash、编译或测试。

## 485：完整编辑来源保留三态性能政策

存储本身无需新实现：现有canonicalEditingSource经中央validateEditingSource，完整保留新schema5及实际Bool，未声明源仍原1..4。新增政策不会使不完整草稿自动可构建，也不把editing恢复口放宽成历史planning恢复口。

原私有保存／读取预览及完整内容hash覆盖settings新字段；Root集中版本门认识5及原三optional任意组合。新[政策编辑器](FleetPerformancePolicyEditor.md) pending并入Planner的inputSettled，因此未保存／取消政策时，不能保存或切换来源后偷偷用旧值。明确保存只保存已提交native编辑态，不回填未确认UI选择。

恢复仍建pristine副本、revision1、datum撤确认；保留政策不恢复审批／性能证据／机载状态。未执行文件保存／读取／hash／业务或测试；工程矩阵全未执行。下方472为原实现与边界，source1..4限制由本节扩展。

This single-slot store saves **editing state**, including a structurally valid
but incomplete `FleetMissionDraft`. It does not save an approved plan, Run,
Vehicle, account, occupancy, upload receipt, execution permission or flight
authority. The existing strict planning-source validator is unchanged; the
store uses `FleetMissionDraft::validateEditingSource`, which does not build a
task. There is no automatic save, read, recovery or restoration on construction,
property access, application startup or display.

The type is registered as `MerivusInternal 1.0 / FleetMissionDraftStore` for a
private Root instance. It has no arbitrary-path, import-JSON, compiled-plan,
authority or transport API. All public actions require the object's thread;
wrong-thread and synchronous reentrant actions fail closed. `busy` describes
this local synchronous operation, not a background worker or a disk deadline.

## APIs and exact values

`current` and every returned `preview` have exactly 15 fields:

`state`, `reason`, `previewKind`, `previewToken`, `checkpointId`,
`checkpointRevision`, `sourceRevision`, `vertexCount`, `memberCount`,
`hasClearanceInput`, `overwritesExisting`, `editingStateOnly`,
`manualConfirmationRequired`, `approvalRestored`, `flightCommandReleased`.

The last four are fixed `true`, `true`, `false`, `false`. Kinds are `none`,
`save` or `restore`; only a live save/restore preview has its canonical nonzero
UUID token. Checkpoint revision is a decimal string: `0` when unknown and
`1..18446744073709551615` for saved records. Increment at the upper limit is
rejected, never wrapped or converted through a QML Number. Historical source
revision is `-1` when unknown or a numeric integer `0..9007199254740992`, including
the exhausted but preservable Draft revision. Vertex/member counts retain the
Draft's complete bounds of 64/32; no prefix is displayed as a whole checkpoint.

Every map-returning action has exactly six fields:
`ok`, `error`, `committed`, `current`, `preview`, `source`. `source` is empty
except after a successful, still-current `restoreSource`. `committed` records
only an actual successful `QSaveFile::commit`; it is not the success of preview,
restore, Draft loading, a future file check or an atomic power-loss guarantee.
`ok` additionally requires no error and an unchanged operation after public
notification. A save may therefore return `committed=true`, `ok=false` and
`current=false`; the saved fact is not erased when readback or the source-tail
check fails. An unconfirmed commit error is not proof that disk was unchanged.

- `prepareSave(actualDraft, expectedRevision)` takes the actual typed Draft,
  freezes the supplied numeric revision before its first `changed` notification,
  requires that exact call-time revision still current, canonicalizes its full editing
  source, and reads the existing slot as a raw-byte baseline. It does not create
  a directory, lock or checkpoint. A bad existing file is an error, not an empty
  slot that can be overwritten. The private preview retains the same native
  Draft pointer, historical revision, independently copied canonical source,
  complete baseline bytes, and the complete proposed encoded checkpoint.
- `commitSave(previewToken, confirmed)` requires that precise save preview and
  explicit `true`. The token is consumed before IO. It rechecks the same Draft
  pointer/revision/entire source, takes a nonblocking advisory lock, checks the
  whole baseline again, writes with direct-write fallback disabled, and reads
  back and fully validates the exact committed bytes. No failed preview or
  commit is automatically retried, refreshed, repaired or rolled back.
- `inspectSaved()` only reads and fully validates the slot, then creates one
  restore preview. It does not construct or alter a Draft. An absent slot is
  explicitly reported rather than replaced by defaults.
- `restoreSource(restoreToken)` uses the precise restore preview, rereads
  and validates the slot, and compares its complete raw bytes and canonical
  source with the private baseline. Only then does it return the private
  canonical editing source5. Success retains that same restore preview/token
  so Root can independently repeat this complete baseline read before and after
  its confirmation/candidate steps; it is not a one-time permission token.
  Failure retires it rather than returning a cached source. It does not call a Draft loader or restore live
  permission. Root must separately and explicitly load it into a new pristine
  Draft through `loadEditingSource`, which sets local revision 1 and clears
  `mapDatumAcknowledged`; old sourceRevision is never live CAS.
- `cancelPreview(token)` cancels only that current token while idle. It does not
  cancel a disk operation, delete a checkpoint or modify an aircraft.

A new prepare/inspect/valid commit replaces the sole preview and retires its
old token. A successful restore read alone preserves the original token and
baseline without extending it to any new file. Invalid token/confirmation, wrong-thread and busy calls
cannot consume some other preview. `changed` handlers may synchronously edit or
destroy objects or begin a later operation; the returned `current` is checked
after notification against the same operation, summary and, for saves, actual
Draft/source. No callback can silently substitute a new Draft with the same
numeric revision. Returning a summary does not promise the file will remain
unchanged afterward.

## Fixed slot and complete format

The location is lazily resolved from Qt `AppDataLocation` and fixed for this
store instance as `fleet-os/editing-draft-v1.json`. There is no caller-supplied
path. Only explicit commit may create the dedicated directory. Ordinary
prepare/inspect/restore and properties never create it. The dedicated directory
and checkpoint reject observed symlinks and non-directory/non-regular-file
types. A `QLockFile` at the fixed slot plus `.lock` uses `tryLock(0)` and stale
time zero; there is no waiting or explicit stale-lock removal. The lock covers
baseline check, save and full readback. It coordinates cooperating writers;
it is not an adversarial filesystem lock or authentication mechanism.

The checkpoint root has exactly 12 fields:

`schemaVersion=1`, `documentType="merivus.fleet.editing-draft"`, `checkpointId`,
`checkpointRevision`, `savedAtUtc`, `source`, `sourceHash`, `contentHash`,
`editingStateOnly=true`, `manualConfirmationRequired=true`,
`approvalRestored=false`, `flightCommandReleased=false`.

`source` is complete canonical exact5/schema1-or2 editing state: schemaVersion,
sourceRevision, vertices, members, settings. Schema2 retains the whole optional
clearance declaration, not a net-clearance verdict. All member endpoints,
routes, profiles, reserve policies and settings survive; incomplete area or
missing assignments are not manufactured into a buildable mission.

`sourceHash` is SHA-256 over the full source compact JSON. `contentHash` covers
every root field except itself, including source, identity, revision, timestamp
and fixed claims. These are internal consistency checks, not signatures or
origin, current configuration, datum, safety, authenticity or approval proofs.
Checkpoint UUID remains stable across successfully prepared successive saves;
its independent decimal revision increments once in the proposed new file.
`savedAtUtc` is the local preview-creation timestamp in canonical UTC ISO with
milliseconds, not trusted time or proof of the final write time.

Whole encoded input/output/file size and cumulative key/string UTF-8 size are
bounded to 32 MiB; value trees to 24 levels and 1,048,576 nodes; maps to 1024
keys, lists to 32768 entries, keys to 1024 and strings to 32768 UTF-16 code
units. Only finite bounded JSON scalars/containers are accepted; custom and
typed-null opaque values are not JSON null. Source-specific smaller limits
still apply. There is no budget increase or accepted prefix.

Readers require valid UTF-8, a complete JSON object, exact known schema/fields,
canonical source, both complete hashes, strict identities/revisions/timestamp,
and **the entire compact canonical JSON byte string equal to the file bytes**.
This intentionally rejects whitespace/BOM/tails, duplicate-key collapse,
truncation and arbitrary external JSON formatting. It is this local checkpoint
format, not a generic JSON importer. Raw baseline CAS compares all original
bytes and existence, not merely a hash or filename.

Observed external change, corrupted file, unavailable lock, failed write or
readback remains an explicit error. No automatic overwrite of a bad slot,
deletion, directory cleanup, recovery, retry, backup or rollback is provided.
QSaveFile's atomic replacement is not advertised as fsync, crash/power-loss
durability, hostile path-replacement protection or all-process exclusivity.

## Handoff limitations

This batch is source implementation and static reading only. No filesystem
checkpoint operation, helper, parser, application, build, test, simulation or
device was executed. Engineers still need to verify Qt/QML typing, incomplete
and exhausted Drafts, canonical byte round trips, full clearance limits,
changed/corrupted/absent slots, lock contention, short writes and readback
errors, post-commit source changes, synchronous notification destruction/ABA,
and pristine restore with fresh local revision and datum confirmation. The
complete startup, measured performance and original product goals remain open.

The native C++ reentrancy handoff must also cover a caller-held mutable
`QVariant expectedRevision` lvalue: a synchronous first `changed` handler changes
both that lvalue and the Draft to the newer revision. The call must reject the
changed Draft using the already frozen call-time numeric revision; notification
must not redefine this request to mean the new value. An originally invalid
revision likewise cannot be repaired by changing the lvalue during notification.
These are source-level cases for engineer verification, not executed tests.

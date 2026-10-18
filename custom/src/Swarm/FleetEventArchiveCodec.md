# FleetEventArchiveCodec：有界历史诊断摘要

## 541：模式退出原值的独立 v4 投影

正常 Run 摘要新增 schema2/exact29，唯一新增键 `missionModeExitObservation` 由共享 [ModeExitObservation](FleetMissionModeExitObservation.md) 完整校验，并要求 observed 与 anomalyMask 的 bit2 一致；已观察退出还必须有本地 MODE 队列已提交事实。原 outcomeCandidate 四种候选不变，不把退出改为 Failed/Abort/Completed。旧 schema1/exact28 继续读取；全局 capture-gap 仍只允许 schema1。

持久投影仍是原 exact12，但新增 marker `fixed-diagnostic-summary-v4` 只接受 schema2；旧 v3 只接受 schema1，两者混搭整份拒绝。Codec 不从公开 map 创造真实来源，原生来源接纳仍由原 Root/Orchestrator 收据链负责。sidecar 的未观察 first/last 原生以无效、无类型 QVariant 表示 JSON null；原有 typed-null 编码拒绝政策不变。仅在解码已通过完整 v4 projectedEnvelope/shared helper 的未观察 sidecar 时，将 Qt 的 Nullptr JSON-null 表示规范回无类型无效值，再验证 canonical 字节相等；不在编码入口、通用字段或旧 v1-v3 投影放宽 typed-null。

两历史面板的有界复制仅对 exact9、schema1、observed=false/count=`0`/candidate=`not-observed`、三声明 false 且 first/last 都有 own key 的 sidecar，把这两项 Qt null/undefined 表示转为显式 JS null。其它 undefined 仍拒绝，不放宽整页或通用字段，也不通过 JSON stringify 省略未采集槽。

v4 的明确隐私扩展仅为已验证首末退出收件对保留本地随机 sourceEpoch、uint64 序号、非负 int64 单调毫秒，以及 raw customMode/baseMode/systemStatus 和该次连续性未知标记。它们不是账号、经纬、完整心跳流、远端身份认证、请求因果验证或结果证明；不保留其它 RAW 内容。旧 v3 不回填这些字段。计数与首末对只描述已保留观察，可在一次模型提交前合并多个边沿，不能据此重建每次退出原值。

原 wrapper18、SQL schema、512 KiB Codec 预算、深度16/50000节点、历史只读及全部权限 false 声明不变。上游单事件4096字节与512项转换FIFO也不扩张，超过原预算进入既有缺口政策，不截断 sidecar 以冒充完整记录。ReportBuilder 与两个历史面板显示原值、首末收件和“非结果”限制；旧版显示未采集，而不是补成未发生。本批仅静态源码/文本审阅，未执行编码、解析、hash、SQL、QML、编译或测试。下方为历史合同。

## 484：性能声明独立事件，不扩隐私摘要

闭合工程operation新增approve-performance，对应独立type `fleet.performance.declaration.result`；engineeringOperation、type/typehash映射、原事件编码允许集和engineeringSummary校验实际同步。旧四种映射不变，不靠改type hash把一类审批冒充另一类。

沿原engineeringReviewPayload、原receipt14/action一致性和exact21摘要。仅记录该工程操作的历史结果／提交事实及原构型比较摘要；不新增两数、原账号、自由理由、完整配置或审批凭据。旧档不回填，不恢复批准、任务或飞行许可；原nativeOk、stillCurrent和mutationCommitted仍相互独立。未执行codec／SQL／业务、编译或测试，工程场景全未执行。下方为历史。

## 422 当前增量：工程审批摘要历史，不是完整审批卷宗

新增纯值 `engineeringReviewPayload(vehicleId, operation, result, frozenComparison)`，成功仅返回 `vehicleId/engineeringReview` 两键；设备号或已提供的结果设备号/操作不匹配时返回空 map。它没有 QObject、文件、账号认证或注册表入口，不产生或恢复批准。新摘要须再经实际 BlackBox 新收据和持久事件入口接纳；函数返回及本地 append 均不是磁盘持久确认。

只有精确原事件 `fleet.capability.review.result` 与 `fleet.configuration.review.result` 可以携带 `engineeringReview`。encode 对它们验证设备号、四种 operation 与事件家族相符，输出 `payloadProjection=fixed-diagnostic-summary-v2`；其它事件夹带该字段直接拒绝。普通事件仍产生 v1，旧 v1 严格解码继续保留，不补造过去未保存的详情。v2 解码将 `originalTypeHash` 与操作对应的固定原事件类型哈希绑定，并要求归档 event.type 为 `fleet`。这些一致性校验不是对写入者或审阅账号的认证。

`engineeringReview` 精确 21 键：

- `schemaVersion=1`、`source=local-engineering-review-summary`、`vehicleId`（1..255 整数）、`operation`（`approve/revoke/approve-baseline/revoke-baseline`）。
- `mutationCommitted/nativeOk/stillCurrent/comparisonKnown/comparisonTruncated`：各为明确字符串 `yes/no/unknown`，不是通过 `toBool` 补齐的布尔值。
- `beforeTypedFingerprint/afterTypedFingerprint/baselineTypedFingerprint/reviewIdHash`：空字符串或小写 SHA-256。reviewId 仅在原值为规范 UUID 时保存其哈希，不保存 controller receipt 或 scopeToken。
- `reportedChangeCount`（-1..4096）取原 Diff report 的 `changeCount`，即总差异数量；`displayedChangeCount`（-1..128）取原 `reportedChangeCount`，即实际输出行数。未知/无效比较均为 -1，不把原报告占位 0 当作“无差异”。已知比较要求两个数量非负、显示数不大于总数；截断不会变成完整比较。
- `capabilityCount`（-1..128）仅在匹配的能力操作回执明确提交后，统计规范有效能力列表；其它情况为 -1，不把未知补成零。
- `historicalOnly=true`，`restoresAuthorization/hardwareIdentityAuthenticated/performanceCertified/flightCommandReleased=false`。

`nativeOk/stillCurrent` 仅来自设备号、操作、schema 匹配的 Registry 原回执，缺回执为 unknown；`mutationCommitted` 优先取该回执，缺回执才保留原工作流严格布尔事实，不从 `ok=false` 推断未提交。原生接受或仍为原快照不能同时宣称未提交；`nativeOk=no` 仍允许 `mutationCommitted=yes`，例如计数耗尽导致已发生保守失效。`stillCurrent=no` 也不抹已提交事实。

能力操作使用冻结 wrapper16 内 report13 的审阅比较：before 是其基准快照，after 是已观测快照；仅当 basis 为 `local-approved-baseline` 时，before 同时可作为当时基线指纹。基线操作优先取实际回执 `previousComparison` 的审阅差异与数量；明确提交后，after/基线指纹只取回执 `comparison/baselineEvidence`，缺失则保留为空，绝不将候选基线当作已批准基线。比较 known/counts 描述原审阅比较，并不保证提交后来源仍可读或审批成功。

摘要不保存 SID、账号名、审批自由理由、字段路径/值、原始或显示用配置、参数、完整回执、认证凭据或账号证据。因而可查看的是本地工程操作与差异数量/指纹的历史摘要，不是完整审批档案、实物改装结论、性能认证、签名或防回滚证明。wrapper18/schema1、SQL schema、历史页及目录 DTO 不变；本模块未运行、未编译/测试，也未更新 PR 或安装包。

以下原有数据边界与 v1 说明仍适用于其对应记录；当前 v2 的唯一白名单扩展如上。

本模块是非 QObject 的静态值编码器。实际记录来源、同线程对象身份及新收据的接纳由持有 `FleetEventBlackBox` 的归档入口负责；本模块不能把调用者传入的事件变成真实飞行证据，也不会恢复任务、审批、控制权或发送命令。

## 数据边界

`encode()` 接受原生黑匣子事件的精确四字段：`timestampMs/type/source/payload`。原 type/source 必须是已裁剪字符串，上限分别 128/256 个 UTF-16 单元，type 非空；payload 只能是严格 JSON 安全的 QVariantMap 树。只接受 map/list/string/bool、无类型 JSON null 和有限数值，整数必须在 ±9007199254740991 内。拒绝 QVariantHash、QStringList、QByteArray、日期、QObject/QJSValue、未知类型和非法 UTF-16，而不是隐式转换或置零。

只有无效且无类型的 QVariant 可代表 payload 内 JSON null。其它 valid-but-typed-null 值全部拒绝，包括原本受支持的 Int/Bool/QString/Map/List；不会调用默认转换把未知数值变为 0、未知布尔变为 false 或未知容器变为空。原事件四字段也不得 typed-null。明确的非 null 空 source 字符串允许，但默认构造的 null QString 不当作明确空字符串补齐。该严格边界可能使旧黑匣子已接纳的某些事件不能归档，入口须计入拒绝/缺口，而不是将其覆盖为已知值；payload 内 typed-null 不采用部分省略后再为原 payload 计算不同哈希。

原 payload 在内存中按深度 16、节点 50000、UTF-8 字符与标量预算 512 KiB、最终紧凑 JSON 512 KiB 检查后计算 SHA-256；不保存原 payload。字段名也计入标量预算。数字计 8 字节、bool 计 1 字节、null 计 4 字节。JSON canonical 指本程序 Qt `QJsonObject` 紧凑 UTF-8 表示，并非外部通用签名规范或 RFC 8785。

runId/sourceEventEpoch 是非零、小写、无大括号的标准 UUID。归档 sequence 是 `1..INT64_MAX` 的规范十进制字符串，原生 sourceEventSequence 是 `1..UINT64_MAX` 的规范十进制字符串；不通过 JSON 浮点数传输大计数。event/received UTC 毫秒均为正、JS 安全整数，事件不晚于接纳 UTC 的五分钟容差；接纳单调毫秒为非负 JS 安全整数。`clockDiscontinuityObserved` 仅记录本地时钟异常，不把 UTC 当作跨进程、跨飞机的同一采样证明。

## 固定隐私投影

归档 wrapper 精确 18 字段：`format/schemaVersion/runId/sequence/event/receivedAtUtcMs/receivedAtMonotonicMs/clockDiscontinuityObserved/sourceEventEpoch/sourceEventSequence/eventProvenance/historicalOnly/authenticated/signatureVerified/executionPermit/flightCommandReleased/completionVerified/restoresAuthorization`。format 为 `merivus.fleet.event-archive`，schemaVersion 为 1；event 仍是精确四字段。

event.type 只按原字符串前缀归入 `command/transaction/task/formation/fleet/intent/maintenance/other`，大小写敏感，不保存原 type；event.source 固定为 `native-blackbox-diagnostic`，不保存原 source。

payload 固定必选六字段：`originalTypeHash/originalSourceHash/originalPayloadHash/payloadProjection/rawPayloadIncluded/omittedFieldCount`。三类 SHA 只用于未认证的内容引用，不能证明原数据来源、真实性或权限。默认额外白名单为：

- vehicleId/componentId：1..255；targetVehicleIds：至多 32 个唯一的 1..255 整数，整个列表非法则省略，不部分截取。
- command：0..65535；result：0..255；queued/uncertain：严格 bool。这些是调用者的诊断陈述，不是原生命令真实性证明。
- taskId/planId/recordId/deploymentId/startupId/transactionId/reviewId/operationId：仅对非空且至多 256 单元的字符串保留对应 `...Hash`，不保存明文。

除当前 v2 专用 `engineeringReview` 白名单外，其它 payload 字段及不满足对应白名单类型/范围的字段被省略，并计入 `omittedFieldCount`。不复制任意 reason/state 自由文本、凭据、SID/UID、精确位置、原始数据包、图像或视频。摘要 SHA 不是匿名化或加密：低熵字段仍可能被猜测匹配，应由工程师结合部署隐私要求处理留存与文件访问权限。

## 历史读取不是授权恢复

`decodeCanonical()` 先检查字节上限、严格小写 64 位 SHA、哈希一致与有界容器深度，再验证精确 wrapper/event 结构、payload 白名单、类型、计数与时间关系，最后要求重新编码后的 canonical 字节逐字节相等。重复字段、其它 JSON 排序/空白表示、附加字段、错误格式、篡改或非规范值不会被当作有效记录返回。

历史包固定 `historicalOnly=true`，其余认证、签名、执行许可、飞行指令放行、完成验证、恢复授权六项标记全为 false。wrapper 的 `flightCommandReleased=false` 只表示归档/读取不放行新命令，不证明历史上未发过命令，也不覆盖摘要保留的 `queued/uncertain` 陈述。`eventProvenance` 固定为 `accepted-native-blackbox-summary-not-aircraft-authentication`，仅描述摘要格式预期；重新计算 SHA 的第三方仍能伪造整个文件，因此验证成功不能提升为签名、设备认证或业务授权。

## 工程师交接

本批仅做源码和文本边界审阅，未运行 codec、单元测试、真实编译、仿真或数据库/设备操作。工程师须验证 Qt 版本的 JSON 数字与 Unicode 表示、边界拒绝、规范字节往返、损坏/外来库读取、计数耗尽与时钟变化，不能据文档宣称运行验证完成。实际数据库文件的事务、页数、队列、锁定和保留策略由归档 worker 管理；本模块不承担其访问权限、崩溃持久性或存储介质可靠性。

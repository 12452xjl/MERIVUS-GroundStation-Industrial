# 509 当前补充：一次原生控制依据交接

Capture 的公开 READ 接口、action、exact20/current 与 exact5 回执不变。新增仅供同 Swarm/Registry 的真实私有 `FleetCameraStreamControl` 调用的 C++ `takeControlBasis`：从原 opaque Capture Run 原生 seal 一次依据，初取必须两份报告各 ≤3秒、完整同源，并且显式目标与原 RUNNING 相反；不从 public DTO/JSON 复建。handoff 前先 burn，一旦失败/取消/拒绝审批也不恢复，须重新采集。

`controlBasisSourceCurrent` 仅保同 Capture generation/ID/对象、Episode/Registry/PRIMARY 来源，不沿用旧8秒 READ 期限、旧 READ 角色或旧Status fresh，也不将自有启停之后已失效的旧 Capture Run 再认证为当前。控制服务有全新30秒 Review、独立action/purpose及双账号批准；READ不授予启停权限。详见 [FleetCameraStreamControl.md](FleetCameraStreamControl.md)。本批未编译、运行或测试。

服务 Capture ID 与原生 Capture Run ID 分别生成：Control.current.captureId 保原服务请求，Basis/Control Run.captureRequestId 保原生 Run；两者不互换或要求相等。初取及自有 action 写入前仍核完整 Info/Status；只有控制原生 Run 的精确 action 已实际写入后，才按私有配置 revision 容许 Info 的 RUNNING-only 更新，并要求实际新 Status 绑定当前完整 Info revision。其他配置变化及 ABA 仍失效；旧 Capture 的完整语义门与 READ 完成合同不因此放宽，不从旧缓存重建批准。

# FleetCameraStreamCapture — 508 显式相机流资料读取

本模块是地面站真实私有服务：对一架原生绑定的飞机、一个摄像头 component（100–105）、一个 stream（1–255），显式依次请求 `VIDEO_STREAM_INFORMATION` 与 `VIDEO_STREAM_STATUS`。它不发送 START/STOP、录制、参数写入或飞行动作，不实现自动降频、自动停流或自动恢复。

新原生读取采用共享命令 lane 的 `MAV_CMD_REQUEST_MESSAGE`（512）：param1 分别为 269/270，param2 为已明确选择的非零 stream ID，其余参数保持该原生请求合同；没有自动退回旧 2504/2505 或 stream 0 广播请求。本地 MAVLink 定义允许这两个消息被 REQUEST_MESSAGE 请求；具体摄像头固件的支持仍待工程验证，不因 ACK 或读取失败自动换协议。

两者索引合同不同：本地 `common.xml` 对 269 明确定义 param2 为 stream ID；270 只说明可被 512 请求，未明确 param2 的 stream 索引语义。因此 270/param2 是有界兼容尝试，只有实际精确同一 stream 的原始回包才接纳；不支持、错流或未回包均不能补成成功，资料保持失败/未知。

本批只修改源码并进行静态阅读；未编译、运行、测试、仿真或连接设备。以下合同不等于工程验收或协议兼容性证明。

## 实际入口与所有权

`FleetCameraStreamCapture` 由 `SwarmController` 私有构造并拥有，Registry 与服务必须同属该实际 Swarm。同一个 `cameraStreamCapture` 属性、`ownsCameraStreamCapture(actualCapture, actualRegistry)` 和非 QML 原生 `cameraStreamCaptureVehicle(...)` 桥接真实 Registry 绑定，冻结 `vehicleRecordEpoch`、`bindingEpoch`、Vehicle/VLM/PRIMARY 的 QPointer 与 PRIMARY 的原控制块。调用者不能用导入 Map、任意 Vehicle 指针或其他采集服务替代这个来源。

公开服务接口：

- `requestCapture(actualSession, actualRoles, actualRoot, scopeToken, vehicleId, componentId, streamId, confirmed)`。
- `current`：只读值快照。
- `cancel(requestId)`：只撤销本服务当前原 ID 的后续读取。
- `contextMatches(actualSession, actualRoles, expectedPrimary, expectedRole)`：原身份五字段与原 Role 九字段的纯匹配，不建立新权限。

独立 READ action 为 `fleet.video.stream.capture`。它必须由实际 Session 的当前身份与实际 Role 的该 action 共同允许，免二审，但不借用 RTT、遥测间隔读取、遥测配置或视频控制权限。Session 仍经 Episode 的 `outboundOwnerAllowed`；Root 原六参数 `_fleetCameraStreamCaptureSourcesCurrent` 负责原 Episode、意图、账号、服务和 scope 围栏。本模块不新增 Session lease、导入权限或可复用授权凭据。

这是 metadata READ，不附加仅在地面或未 armed/flying 的规则；原生协议、真实来源、期限及已有 P0 临界围栏仍须通过。实际 source 可用的飞行状态不会单因 armed/flying 变化而被该服务撤销。

## 原始期限与生命周期

- 总期从 `requestCapture` 入口、任何身份/来源外调之前起算，固定 8000 ms，授权准备也占用这段时间。
- 100 ms 定时推进；入口只接纳并发布真实服务 ID，不直接发包。原生 prepare 也只创建 ready Run；实际 wire 由后续 advance 进行。
- 原生每阶段最多 4000 ms，accepted ACK 后等待报告最多 1000 ms，均只能缩短原始期限，不续期。
- 原生 Run 只由 Vehicle 持有/建立。服务保存 opaque `shared_ptr<FleetCameraStreamRun>`，不从 JSON、历史 DTO 或 caller Map 重建。
- 原生接纳原 ACK、匹配 RAW 报告及双端完成前调用纯 `nativeResponseCurrent(run)`，再次核对原服务 Run、generation、最早 8 秒期限和 Role 当前性，避免剩余毫秒换时钟与轮询调度造成延期。该桥不调用 OS，也不作为终态历史资料的重新授权入口。
- RAW 暂忙时先由原生 advance 检查原 stage 期限，再等待下一次 tick；不在 RAW 中调用 Session/OS，不把暂不可读当作新 source，也不续任何期限。
- 即使 Run 已在 RAW 中完成双端，后续嵌套事件循环触发 tick 时也须等待整 RAW 退栈，再核当前来源和最终 gate；不能将 complete 的临界期暂 false 误判为失败，更不能将 complete 当作当前来源。失败/取消的原生终态仍可立即保留 partial 事实；服务最早 8 秒总期始终先检查。
- Session/Role/Root、实际 Registry 绑定、Vehicle/VLM/PRIMARY 生命周期及来源变化撤销原意图。A→B→A 不能恢复已经退休的原 generation。generation 不回绕；耗尽后拒绝新请求。
- pending 入场在原 source 冻结后、首个 Session/OS 外调前安装独立 sticky watcher，只置本次局部撤权标志；该 watcher 保持到正式 watcher 安装与请求返回，防止准备期间 PRIMARY/Role A→B→A 被最终值相等吸收。清理只处理原连接，不能重新捕获来源。
- 任何通知、外调、原生请求、连接清理之后，续作先检查原 QPointer、generation、request ID、opaque Run 与来源。`changed` 是最终通知或带完整尾核的发布点。
- 取消/失败只阻止后续请求，保留已经可能交接的 request、ACK 与报告事实，不宣称回滚或未发送。Vehicle 同步销毁后只允许原生静态历史投影保留原事实，不能恢复 active/sourceCurrent/权限。

## 请求回执 exact5

`ok`、`requestAccepted`、`requestId`、`reason`、`flightCommandReleased`。

前两项为 actual Bool；ID、原因是独立持有的非 null QString；最后恒为 false。接纳回执不是 ACK、资料已收到、摄像头已运行或已停止的证明。

## current exact20

`schemaVersion`、`requestId`、`generation`、`scopeToken`、`vehicleId`、`componentId`、`streamId`、`state`、`active`、`reason`、`remainingMs`、`observation`、`snapshotOnly`、`authenticated`、`configurationPermitted`、`executionPermit`、`flightCommandReleased`、`protocol`、`maximumAgeMs`、`requestAccepted`。

- schemaVersion 为 Int 1；三个目标 ID 为 Int，未入场时为 0。
- generation 为规范 UInt64 十进制字符串；scope/ID/reason 为独立 QString。
- state 为 `empty`、`preparing`、`capturing`、`observed`、`failed`、`stale` 或 `cancelled`。
- remainingMs 为 LongLong，active 时是原 8 秒总期的剩余值，终态为 0，不重新开启时钟。
- observation 为原生 exact30，尚未取得原生快照时为空 Map。
- protocol 固定 `video-stream-information-status`；maximumAgeMs 为 Int 3000，表示 Info/Status 各自的 fresh 年龄界，不是整个读取总期。
- snapshotOnly 恒 true；authenticated、configurationPermitted、executionPermit、flightCommandReleased 恒 false。
- requestAccepted 仅表示服务已经分配实际非空请求 ID；失败终态仍保留该接纳历史。

`current.observation` 是服务最后保存的资料快照，不是持续刷新的远端运行状态。快照内 sourceCurrent/fresh 描述该次原生投影；不能将缓存终态一直解释为后续实时状态或权限。

## 原生资料 exact30 / query exact12 / 信息状态各 exact15

原生 reportType 为 `fleet-camera-stream-capture`。exact30 字段为：

`schemaVersion`、`reportType`、`requestId`、`vehicleId`、`componentId`、`streamId`、`active`、`state`、`reason`、`sourceCurrent`、`remainingMs`、`stage`、`informationQuery`、`statusQuery`、`information`、`status`、`sourceEpoch`、`transportEpoch`、`versionEpoch`、`receiptSequence`、`snapshotOnly`、`authenticated`、`wireCausalityProven`、`catalogComplete`、`remoteRunningVerified`、`remoteStoppedVerified`、`availableBandwidthVerified`、`configurationPermitted`、`executionPermit`、`flightCommandReleased`。

原生状态为 `ready`、`waiting-info`、`waiting-status`、`complete`、`failed`、`cancelled`；stage 为 Int 0（Info）、1（Status）、2（两阶段完成）。上述所有认证、因果、完整目录、远端运行/停止、带宽、配置或飞行许可标志恒 false；snapshotOnly 恒 true。

每个 query exact12：

`requestId`、`messageId`、`requestIssued`、`ackObserved`、`ackResult`、`reportObserved`、`baselineReceiptSequence`、`receiptSequence`、`contentRevision`、`remainingMs`、`state`、`reason`。

messageId 分别是原生 269/270；ackResult 为 Int（-1 未观察，0–255 为原 wire 值），不能用 Bool/字符串代替。query state 为 `ready`、`waiting-ack-and-report`、`waiting-report`、`complete`、`failed`、`cancelled`。counter 都是规范 UInt64 字符串；已观察报告的 receipt 必须严格大于实际写入前原 baseline，contentRevision 不为 0。已可能发出、ACK 或报告事实不是远端因果认证。

information exact15：`observed`、`fresh`、`ageMs`、`receiptSequence`、`contentRevision`、`declaredCount`、`type`、`flags`、`framerate`、`resolutionH`、`resolutionV`、`bitrate`、`rotation`、`hfov`、`valid`。

status exact15：`observed`、`fresh`、`ageMs`、`receiptSequence`、`contentRevision`、`informationRevision`、`matchesCurrentInformation`、`flags`、`framerate`、`resolutionH`、`resolutionV`、`bitrate`、`rotation`、`hfov`、`valid`。

未观察时仍是完整 15 字段 unknown DTO，不补充为已见的零值。Info/Status 分别保存本 Run 首次匹配报告，不用后续重复包改写原 receipt。没有 URI、名称、完整摄像头目录或视频内容输出。

## 完成与边界

服务 observed 必须消费实际原生 complete/stage2/sourceCurrent，两个 query 都实际 issued、收到 accepted ACK 和匹配报告，并验证各 query 的 receipt/revision 与本 Run 保存的 Info/Status 一致，以及 Status 绑定同一 Info semantic revision。不能用单纯 ACK、已有被动缓存或一个自报 complete 字段完成读取。

两份资料的 fresh 独立按各自原年龄 ≤3000 ms 判断。整个读取可用 8 秒不等于两份资料同时 fresh；过期不会抹掉已经观察过的事实。完成也不表示远端正在传流/已经停流、真实传输率、可用带宽、完整目录、协议 nonce 因果或硬件身份认证。晚到同源 ACK/报告仍有 MAVLink 原协议无 nonce 的剩余歧义。

旧 CameraControl/Manager 的 2504/2505 读取、2502/2503 流控制、2500/2501 录制接口不改。此服务没有自动重试、自动启停、配置 Apply、恢复/回滚、跨源续传或飞行放行；目标14的受控视频降级仍需后续独立开发与工程验证。

## 工程交接：全部未执行

需验证空被动目录首次读取、100/105 边界组件与 1/255 流号、MAVLink 合法短尾、ACK/报告两种顺序、Info/Status 语义变化、重复/晚包、4秒阶段与入口8秒期限、RAW 嵌套事件循环、同步通知取消、账号/Episode/PRIMARY A→B→A、调用中销毁 Vehicle/Root/服务、已发 partial 保留，以及飞行状态下仅 metadata READ 不触发流控制。所有这些场景在本批均未执行；仍由工程师负责实际编译、测试与放行。

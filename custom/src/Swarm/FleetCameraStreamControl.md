# FleetCameraStreamControl — 509 独立审批的一次流启停

本模块使人工明确 Review → 不同账号 ApproveAndApply → 一次 START（2502）或 STOP（2503）→ 请求 STATUS（512/270）回读进入实际原生消费者。它不是自动视频降级、自动恢复、飞行许可或远端视频效果认证；允许来源有效时在飞行中操作，STOP 可能让操作员失去视频，必须由两名授权人员评估用途及后果。代码不能从现有元数据证明某流“不关键”。

本批仅写源码并静态阅读，未编译、执行、测试、仿真或连接设备。

## 实际私有服务与独立权限

服务由真实 `SwarmController` 私有构造并持有，只接受同 Swarm 的实际 Registry 与 `FleetCameraStreamCapture`。`cameraStreamControl`、`ownsCameraStreamControl(control, registry, capture)` 不把公开 Map 当成来源；目标 vehicle/component/stream 从原生 Capture 的 opaque Run 中取得，调用者不能重新指定三个 ID。

独立 action：`fleet.video.stream.control`；独立审批 purpose：`fleet.video.stream.control.approve`。READ、RTT、遥测间隔配置、旧视频入口或飞行批准均不能替代它。实际 Session 私有 typed consume/current/pure-current/revoke 只绑定本 Control 对象、两份身份、purpose 和本次 bindingHash。operator/maintenance/commander/administrator 的申请权限与不同账号审批方的资格由真实 Role/Session 控制，不从 UI 文本恢复。

公开方法：

- `requestReview(actualSession, actualRoles, actualRoot, scopeToken, captureId, targetRunning, confirmed)`。
- `approveAndApply(reviewId, confirmed)`、`cancel(reviewId)`。
- `contextMatches(actualSession, actualRoles, primary5, role9)` 与 `current` 值快照。

operation 只由 actual Bool targetRunning 派生为 `start`/`stop`，没有第二个可冲突的输入。Root 还须核 actual Bool，服务的 C++ bool 签名不声称能够识别 QML 隐式类型转换。原生目标必须与捕获 STATUS 中 RUNNING 位相反；同目标拒绝，不发命令或伪造已完成。

## 一次 opaque basis、原期与回调

`FleetCameraStreamCapture::takeControlBasis` 是只给真实私有 Control 的 C++ 交接。它从保留的同一次原生 Run 封装 `FleetCameraStreamControlBasis`，不接受报告 Map/JSON/用户 ID。初次 seal 必须两份完整资料各自 fresh ≤3000 ms、同原来源/metadata/info revision/已知本地视频 mutation；handoff 在可能外调之前置一次 transferred，失败、取消或拒绝审批不会恢复这次交接，需新采集。

后续依据是原 Capture 的 generation/服务 ID/实际对象、原 Registry 绑定、Episode/物理 PRIMARY、原 Info 配置与原生维护的可预期 own mutation。它不再要求旧 READ 角色、旧 Capture 8 秒窗口或两份资料持续 ≤3秒；自己的 action 实际写入之前仍要求原完整 Info/Status，写入之后才允许 Info 仅 RUNNING 位变化，并等待实际新 Status 与当前完整 Info revision 配对。该来源配对不是控制批准。新 Capture、换源、其他 Info 配置 A→B→A、外部本地已知启停等使原依据失效；不能随意重基。

Review 总期从 requestReview 最早入口起算 30000 ms，包含初始身份检查、资料交接、人工审批与全部 wire。100 ms 推进；每条 wire 最多 4000 ms，读取 accepted ACK 后报告等待最多 1000 ms，均只缩不续。Session 的 consumed scope 也只能缩到原 Review/两个账号的最早截止。

ApproveAndApply 只创建 ready 原生 Run，不发 wire；第一次实际 advance 在后续 tick，Root 须已经接纳原 reviewId/bindingHash 的批准意图。每次实际写入仍核独立两账号 final fence、原 UUID/source/期限。原生 ACK/RAW/双端终结还有纯 `nativeResponseCurrent(run)`，不因轮询或换本地时钟延长原服务 30 秒。

pending 原 source/Role watch 在首次 OS 调用前保持 sticky，正式 watch 安装前不留撤权空窗；PRIMARY/Role A→B→A 不能吸收旧意图。generation 不回绕。每个外调、通知或连接清理之后都先核原 QPointer/generation/review/run。忙 RAW 先检查原期，仅有界等待退栈；包括已 reported-target-match 但临界期 sourceCurrent 暂 false 的情况，不能提前当成功或误判失败。

审批弹窗只窄 hold：原 approving/_prompt 调用栈、原 Session epoch/revision/primary、Role、来源和原期限仍匹配，并且本次 prompt 正在进行，或成功通知带精确 purpose/hash/applicant 的 approver receipt。不是一概忽略 Session.changed；logout、WTS 撤权、失败或其他用途立即退休，弹窗结束后仍做真实 OS/身份检查与一次消费。传入 OS 审批的摘要列明实际三个目标 ID、原 Basis.originalRunning 的报告基线（不是物理认证）、目标 START/STOP 和 STOP 可能移除操作视频的风险，固定摘要保持在 600 字符限制内。

取消不恢复视频、不发送相反命令、不重试、不回滚。部分实际 requestIssued、ACK、Status 保留；同步销毁 Vehicle 时只从真实 opaque Run 的静态历史投影保留事实，sourceCurrent/reportedTargetMatch/权限不能复活。历史/缓存 JSON 不能重建 Run、basis 或批准。

## 回执与 current

请求/批准回执 exact5：`ok`、`requestAccepted`、`reviewId`、`reason`、`flightCommandReleased`。最后恒 false；接纳不是已发或回读成功。

current exact30：

`schemaVersion`、`reviewId`、`generation`、`scopeToken`、`captureId`、`vehicleId`、`componentId`、`streamId`、`operation`、`targetRunning`、`state`、`reason`、`active`、`remainingMs`、`bindingHash`、`basisSummary`、`run`、`authenticationConsumed`、`requestsMayRemainInflight`、`snapshotOnly`、`revalidationRequired`、`authenticated`、`wireCausalityProven`、`remoteRunningVerified`、`remoteStoppedVerified`、`availableBandwidthVerified`、`configurationPermitted`、`executionPermit`、`flightCommandReleased`、`permissionsRestored`。

schemaVersion 为 Int 1；目标三个 ID 为 Int；generation 为规范 UInt64 字符串；remainingMs 为 LongLong 0–30000，终态 0。state 为 `empty/preparing/review/approving/applying/reported-target-match/failed/stale/cancelled`。snapshotOnly、revalidationRequired 恒 true；所有认证、物理效果、带宽、配置/飞行许可及 permissionsRestored 恒 false。authenticationConsumed 只表示本次真实 typed 身份已消费；requestsMayRemainInflight 是保守历史已可能发送标记，取消不将其清零。

bindingHash 是原生 canonical compact JSON 的 SHA256 绑定，不是签名/身份认证。它绑定原 Review/Capture generation/ID、scope、immutable basis、目标及两账号申请依据，不包含动态剩余时间。basisSummary/run 只能是原生实际对象投影。终态 current 是历史快照，不是持续远端运行状态。

## Native 固定 DTO

Basis exact20，reportType=`fleet-camera-stream-control-basis`：

`schemaVersion/basisId/captureRequestId/vehicleId/componentId/streamId/sourceEpoch/transportEpoch/versionEpoch/mutationGeneration/informationRevision/statusRevision/originalRunning/targetRunning/remainingMs/snapshotOnly/configurationPermitted/executionPermit/flightCommandReleased`，以及上述 reportType。

其中 captureRequestId 是原生 Capture Run ID，current.captureId 则是服务 Capture ID；二者分别生成，不能要求相等。服务 captureId 绑定原 Capture 服务请求与 generation；Run 的 captureRequestId、basisId 分别匹配同次 Basis 的原生 ID，不能拿公开 ID 重建来源。原/目标 running 是 Bool 且相反；目标和 schema 为 Int；remainingMs 为 LongLong；mutation/revision 是规范 UInt64 字符串。snapshotOnly true，三个权限 false。

Run exact34，reportType=`fleet-camera-stream-control`：

`schemaVersion/reportType/requestId/basisId/captureRequestId/vehicleId/componentId/streamId/targetRunning/originalRunning/active/state/reason/sourceCurrent/remainingMs/stage/mutationGeneration/action/statusQuery/information/status/sourceEpoch/transportEpoch/versionEpoch/snapshotOnly/authenticated/wireCausalityProven/remoteRunningVerified/remoteStoppedVerified/availableBandwidthVerified/configurationPermitted/executionPermit/flightCommandReleased/reportedTargetMatch`。

state 为 `ready/waiting-action/waiting-status/reported-target-match/failed/cancelled`，stage Int 0/1/2。action exact8：`requestId/command/requestIssued/ackObserved/ackResult/state/reason/remainingMs`；command 为 2502 或2503；状态 `ready/waiting-ack/complete/failed/cancelled`；ackResult Int -1未知或0–255；remainingMs LongLong 0–4000。statusQuery 沿508 exact12（messageId270），Information/Status 沿各15字段；不输出 URI/name。

只有本 Run 精确 UUID 的第一次实际 action 写入能推进可预期 video mutation 一次；Service 核投影 mutation 为初值或确切初值+1，不借外部变化继续。各帧/ACK仍无协议 nonce，不能从这个本地代次宣称 wire 因果。

## 成功与兼容边界

成功只叫 reported-target-match：实际 action 已可能写入且 accepted ACK、实际 status query 已写入且 accepted ACK/原始同 stream 报告、receipt 严格大于实际写前 baseline、回包绑定同一 Info revision，RUNNING 位与冻结目标匹配，原来源和最终两账号 gate 当前。`reportedTargetMatch=false` 的死亡/失效历史仍保 terminal state 与已发事实，不丢为“从未发送”。

这不证明视频物理到达/停止、摄像头真伪、带宽改善或实际飞行安全。270 的本地定义只说明可512请求，未明确param2索引合同；本版仅作有界兼容尝试，错流、不支持、缺回包都失败/未知，无自动2505或广播回退。旧 CameraControl 正常同栈 STOP→START 不能被新的单 lane 意外吞掉；与 legacy 启停的互斥和已知本地 mutation 由 Vehicle 的实际发送入口处理，不声称全进程/外部设备的全部视频控制均已拦截。

初始 seal 和自有 action 实际写入前冻结完整 Info/Status。原生另保不可导入的配置 revision，仅在比较配置时忽略 flags 的 RUNNING 位；count、type、其余 flags、framerate、分辨率、bitrate、rotation、hfov 以及私有定长 name/URI 等仍参与配置变化判断。仅本 Run 精确 action UUID 已实际写入后，正常 RUNNING-only Info 更新不再自行阻断；其他配置变化或 A→B→A 仍使原依据失效，删除/重建行也不复用旧配置 token。公开完整 Info revision 和旧 Capture 的完整语义门不放宽。

新 Status 必须实际收到并绑定当时的当前完整 Info revision，Run 保留这对实际 Info/Status 各自原 receipt/时间，不把旧 Status 改挂到新 Info，也不刷新 Info 年龄。若先收的 Status 被后到 RUNNING-only Info 取代，保留 partial，但只在原剩余期（accepted ACK 后最多1秒）等待新的实际同对回包，不重发、不续期。RUNNING-only 翻转不证明命令因果；完成快照也只是所捕获配对的历史报告，不证明之后状态不再变化。

目标14的自动视频降级、自动恢复、链路容量认证和全Fleet网络控制仍未完成。工程师还需编译与场景验证，包括两人批准失败/超期、同目标、旧 basis 重用、服务/原生 ID 区分、源/Role ABA、弹窗锁屏注销、同步取消/销毁、自有写入前后 RUNNING-only Info、Info 配置 ABA、旧 Status 后遇新 Info 时的实际新配对、action后无法回读、RAW内终态事件循环、旧正常STOP→START、legacy外部改变和partial事实。此处所有场景均未执行。

# 原生固件适配器工程声明目录

## 536：逐航点朝向编码与原生消费者接线

supportedSemantics(bool headingTicks=false) 默认继续返回旧 v2 的原完整 map，不改变旧声明身份。显式 true 返回 mission-item-int-heading-tick-v3，并增加 waypointYawMode=absolute-degrees、waypointYawEncoding=binary-degree-tick-1-over-256、waypointYawAttainmentCertified=false；固定语义校验与规范化按原声明选择域，不静默提升旧档案。

新增 native-only acceptsWireSchema(matchedDeclaration,schema)：核当前 owner 线程、ok/matched、catalogEpoch／revision／fingerprint、descriptorId／当前 descriptorHash。新 v3 声明可覆盖不含朝向的 v2 子集，旧 v2 不覆盖 v3。matchObservation 的闭输出形状不改；该方法只是原生调用方已有匹配材料的当前声明一致性检查，不认证输入、机载固件、身份或飞行权限。

实际 ReviewCoordinator 以原 builder 逐成员编码 schema 检查声明，StartReadiness 在实际证据首尾再次检查；目录变更仍使原审阅失效。没有添加预认证条目，没有自动改写工程声明、加载新档案或进行设备操作。工程师须为实际固件重新审查并明确采用新语义，不能仅复制一个标签当认证。

共享合同见 [HeadingEncoding](FleetMissionHeadingEncoding.md)。本批仅源码修改、直接静读与普通文本核对；未编译、运行实现／解析器／算法、测试、仿真、硬件操作、修改PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 454：显式同高度基准、零偏移工程关系

新增 descriptor `schemaVersion=2`，恰为原八字段加 `datumRelations`；旧 descriptor 1 仍恰八字段，不自动升级、不补关系。档案外层仍是原 schema 1 四字段格式，可含显式版本 1 或 2 声明；原固定文件、整批预览／替换、工程审阅材料及受控重载流程继续使用，不新增独立审批服务。构造时自动加载仍只是恢复声明，不能当作已经经过运行中双账号重载。

版本 2 的 `datumRelations` 是 1–32 项实际列表。每项恰十二字段：

| 字段 | 严格要求 |
| --- | --- |
| `relationId` | 1–128 字符、首尾无空白／无控制字符；同 descriptor 内唯一 |
| `sceneId`、`sceneVerticalDatumId` | 各 1–128 字符，按原场景值精确匹配，不归一为通用 AMSL 名称 |
| `sceneRevision` | 规范正 uint64 十进制字符串，不接受数字、前导零或 0 |
| `sceneSourceDocumentHash` | 与场景原声明对应的 64 位小写 SHA-256 形状；不额外改写或补默认值 |
| `configurationFingerprint` | Registry 已冻结 JSON 构型的 64 位小写摘要，不是硬件身份认证 |
| `globalOriginReferenceHash` | 当前原生 GLOBAL_ORIGIN 参考投影的 64 位小写摘要，不是内部 origin 有效性或同估计样本证明 |
| `reportedAltitudeSource` | 固定 `global-position-int-alt`，不自动扩大至 HOME、ALTITUDE 或其他通道 |
| `reportedAltitudeDatumId` | 必须与本条 `sceneVerticalDatumId` 完全相同 |
| `offsetMm` | 显式、非 null 的真实数值整数 0；不接受 bool／字符串，也不把非零偏移夹为 0 |
| `relationModel` | 固定 `engineer-declared-same-datum-zero-offset` |
| `engineeringReview` | 与原 descriptor 相同的严格四字段材料引用，各关系独立保留，不认证审阅人或材料内容 |

选择器是 `sceneId/sceneRevision/sceneSourceDocumentHash/sceneVerticalDatumId/configurationFingerprint/globalOriginReferenceHash` 六字段。同 descriptor 内相同选择器也拒绝，不能借不同 relationId 形成歧义；原“一个固件元组只能有一份 descriptor”规则保持。所有关系进入完整 descriptor 摘要、归档与重载差异预览。原 64 条／1 MiB／深度 8／8192 节点总预算不增加；原生逐条新增也先检查候选完整档案预算，超限不部分提交。

新增 native-only `matchDatumRelation(actualVersion, queryExact6) const`。先执行原 `matchObservation` 严格报文／新鲜度／固件声明匹配，再验证六字段 query 并选择唯一关系。返回恰 `{ok, reason, evidence}`；拒绝时 evidence 为空，不能继承旧结果。成功 evidence 恰十字段：`schemaVersion=1`、`source=native-local-declared-datum-correspondence`、`firmwareBinding`（原完整版本匹配结果）、`relation`（完整十二字段）、`relationFingerprint`、`declaredCorrespondenceMatches=true`、`physicalDatumAlignmentCertified=false`、`firmwareAdapterCertified=false`、`startupPermitGranted=false`、`flightCommandReleased=false`。旧版本 1 缺关系明确返回 `datum-relation-not-declared`；场景／构型／原点不对应返回 `no-exact-local-datum-relation`。这只是声明精确对应，不是把声明变成实测基准；实际 Vehicle／PRIMARY／Catalog 来源和首次材料不替换仍由原生消费者约束。

新增 native-only `datumBindingMemory() const`，同线程返回恰 `{catalogEpoch, revision, catalogFingerprint}`，异线程为空。缓存采用与原 snapshot 完全相同的 entries／restored 标记和哈希公式，构造及每次实际新增、撤销、整批替换先更新缓存再发 `changed`；拒绝不改变缓存。此读口无序列化、回调、文件或设备操作，供最后来源围栏核当前目录身份，不用每次尾核重新编码全目录。

同时修正原报文形状漏项：实际 `Vehicle::autopilotVersionObservation` 带 `receiptSequence`，目录现在严格接收规范正十进制收据；缺失／非法仍拒绝。收据和年龄均不进入稳定 `observationBinding` 或匹配 fingerprint，合法同内容周期报文不会仅因收据推进变成另一份声明。

非零真实 datum 偏移需要未来一致地转换规划、环境、请求和实际观测，本接口明确不支持；`verticalDatumAlignmentErrorMm` 只是已有声明误差，不能替代偏移转换。GLOBAL_ORIGIN 哈希、JSON 构型哈希、固件短前缀和工程材料引用都不认证硬件、固件内部全球分支或物理高程对齐。本轮不计算完整实时净空、不解除既有 pending、不提供最终 StartPermit。这里只做源码和静态契约检查；编译、仿真、实际材料核验与硬件验证仍待工程师执行。

## 417：纯预览与最后提交围栏

新增 native-only `previewImport(json, expectedRevision) const`，复用严格档案 staging，返回原目录身份、完整规范声明／原 engineeringReview、规范与输入哈希、增删改和未变 ID／数量；不修改目录、不发 changed。三参数 `importJson(json, expectedRevision, finalCommitFence)` 在 staging 后复制围栏并核原实际对象／epoch／revision，再执行最后围栏与纯尾核；拒绝不提交，已提交回执先于 changed 保存。原双参数行为保持。

[固定文件工作流](FleetFirmwareCatalogReload.md)现在提供受控运行中重载及[两个界面入口](FleetFirmwareCatalogPanel.md)，不直接向 QML 开放 Catalog 的读写接口。空列表、旧档案版本和相同内容仍按整批替换处理；新 epoch/revision 与 `mutationCommitted` 不等于固件可信或授权。构造时旧自动加载不改为双账号，不能声称统一了全部导入途径。未编译、测试或运行。

`FleetFirmwareAdapterCatalog` 是有界、只读可观察的原生 C++ 后端目录。它把工程人员明确审阅的源码适配声明，与当前设备真实收到的 `AUTOPILOT_VERSION` 观测做精确对应。**对应关系不是固件认证、签名验证、真机放行或任务执行许可。** 构造时目录为空，没有预置的“已认证”机型或版本。

仅完成源码与静态契约检查，没有编译、测试、运行程序、读取实际设备、仿真或刷机。ReviewCoordinator 私有持有目录并将声明/真实版本观测接入审阅、归档与部署来源核对；直接目录 API 仍仅给原生 C++，417 的窄工作流另提供运行中显式重载，不提供固件认证或启动界面。

## 原生接口与状态

`snapshot` / `changed` 可供只读观察。以下方法均不是 `Q_INVOKABLE`，不开放 QML 参数修改、文件读写或手工认证接口：

- `registerReviewedDescriptor(descriptor, expectedRevision)`：新增一份严格声明；已有 ID 不隐式覆盖。
- `revokeDescriptor(descriptorId, expectedRevision)`：撤销本地声明，不清空无人机任务、写参数或发送飞行命令。
- `matchObservation(actualAutopilotVersionMap)`：只读匹配当前原生观测。
- `matchDatumRelation(actualAutopilotVersionMap, query)`：只读匹配显式场景／构型／原点的零偏移工程关系，见 454。
- `datumBindingMemory()`：仅内存读取最后实际目录提交身份，不重算序列化摘要。
- `supportedSemantics()`：返回当前源码适配器完整固定语义，工程配置应使用这个原生值，不自行扩展能力声明。
- `exportJson()` / `importJson(json, expectedRevision)`：显式交换工程声明。
- `saveToFile(path)` / `loadFromFile(path, expectedRevision)`：显式文件操作；保存使用 `QSaveFile`，不启用直接覆盖回退。目录不自动联网、不搜索仓库、不请求或刷新机载参数。

所有实例方法要求对象所属线程。快照提供 `catalogEpoch`（无括号规范 UUID）、`revision`（规范十进制字符串）、`catalogFingerprint`、`entryCount` 与按 ID 排序的有界 `entries`。快照没有时钟或年龄字段，重复读取不改变身份。每条快照含严格声明、声明 SHA-256、是否从归档恢复，以及始终为 false 的认证/审批标记；调用方应每组审阅归档一次，不把整个目录重复到每架机历史。

## 声明 schemaVersion 1

根对象必须恰含以下八个字段，额外字段拒绝：

| 字段 | 要求 |
| --- | --- |
| `schemaVersion` | 实际数值整数 1，不接受字符串或 bool 数字 |
| `descriptorId` | 1–64 字符、首尾无空白、无控制字符的实际字符串 |
| `adapterId` | `px4-multirotor-mission-v1` |
| `firmware` | 下述完整、固定字段的版本选择元组 |
| `sourceCommit` | 非零、小写 40 位十六进制工程源码提交声明 |
| `sourceRepository` | 1–512 字符的实际源码来源说明；本模块不联网验证 |
| `engineeringReview` | 下述四字段工程审阅声明，不是密码学签名 |
| `semantics` | 与 `supportedSemantics()` 严格类型一致的完整固定契约 |

`firmware` 必须恰含：`autopilot`（PX4=12）、`flightSoftwareVersion`（非零 uint32）、`flightCustomVersionWireHex`（非零、小写 16 位，按收到的字节顺序）、`boardVersion`（非零 uint32）、`vendorId` / `productId`（非零 uint16）、`customVersionEncoding`（仅 `px4-git5-vendor3-le`）、`vendorVersion`（uint24，可为零）。版本号只接收有限、范围内的真实数值整数；不接受 bool、字符串数值或未知零版本。

`engineeringReview` 必须恰含：`reviewId` / `reviewerId`（各 1–128 字符、有界无控制字符）、`reviewedAtUtc`（规范 UTC 毫秒时间，形如 `YYYY-MM-DDTHH:mm:ss.zzzZ`）、`evidenceDigest`（非零、小写 64 位十六进制）。这些是工程对应证据的引用，本目录不声明审阅人员已经实际执行验证，也不验证引用所指材料。

固定 `semantics` 包含：`wireSchema=mission-item-int-plan-v2`；按顺序恰有命令 `[16,17,21,22,178]`（WAYPOINT、LOITER_UNLIM、LAND、TAKEOFF、DO_CHANGE_SPEED）；`relativeDwell=waypoint-after-arrival`；`horizontalSpeedType=1`；`verticalPolicy=vehicle-parameters`；`defaultYawMode=system-default`；`takeoffEndpointPolicy=same-xyz-as-first-outbound`；`landingEndpointPolicy=same-xy-ground-below-approach`。`absoluteTiming`、`synchronizedStart`、`rtl`、`clearanceCertified`、`takeoffPathCertified`、`landingClearanceCertified` 必须都是真实 bool false。数值 0 不能替代 false；不能加任意未来命令或认证能力。

这描述的是请求编码，不是固件真实轨迹保证。PX4 多旋翼起飞可能按当前实际位置及 `MIS_TAKEOFF_ALT` 改变目标；无限盘旋不自动完成任务；落地完成依赖实际着陆检测。绝对时序、同步启动、独立垂直速度、起降实际路径、地面高度基准、净空、结束动作完成及执行审批仍需独立证据。

## Git5 + Vendor3 的本地源码边界

本地 `FirmwarePX4/src/modules/mavlink/mavlink_main.cpp` 保留 Git 二进制高五字节，再用 `vendorVersion >> 8` 覆盖低三个 wire 字节。QGC `InitialConnectStateMachine.cc` 把八字节倒序显示为 16 位十六进制；这个显示是 **10 位 Git 前缀 + 6 位 vendor 版本**，不是纯 Git16。

目录只支持显式声明的 `px4-git5-vendor3-le`：wire 的前三字节按 little-endian 解析，必须精确等于声明的 uint24 `vendorVersion`；wire 整体倒序后的前 10 位必须非零，并等于 `sourceCommit` 的前 10 位。完整八个 wire 字节仍用于精确元组匹配。未知布局拒绝，不能把这 40 位短前缀宣称为完整源码提交、可复现构建或机载二进制证明。

`boardVersion` 在本地 NuttX 源码中可来自 USB product ID，非 NuttX 可为常数 1。它与 vendor/product 字段仅是精确声明选择元组，不等于硬件型号、硬件修订、板卡认证或真实机载构建认证。UID 与 18 字节 UID2 的非零值也只提供收到的标识，不认证硬件或链路。

## 真实观测匹配

调用方必须从实际 `Vehicle::autopilotVersionObservation(60000)` 捕获当前原生对象结果；不能以缓存固件显示、初连能力回退、人工 options 或调用者自行构造的状态代替。此方法接收值对象，只检查其严格原生形状、内容一致性与声明对应，**不能独立证明调用者提供的 map 确实来自原生对象**；对象来源绑定必须由私有原生审阅路径维持。

匹配要求固定 schema 与字段集合、实际 bool `observed/available/fresh=true`、`source=live-autopilot-version`、规范 `sourceEpoch` UUID、正的十进制 `contentRevision` 与 `receiptSequence`、年龄 0–60000 ms，以及真实 PX4 多旋翼、非零 system/component ID、完整非零固件选择元组。必须由实际 uint64 十进制 capabilities 报告 MISSION_INT，不接受能力缓存回退；UID/UID2-known 必须与实际字段一致，且至少一个非零。收到的标识或能力仍不等于认证。

`contentHash` 必须精确等于这十二个公开原始 payload 字段形成的 `QVariantMap -> QJsonObject -> Compact JSON` SHA-256：

- 十进制字符串：`capabilitiesDecimal`、`uidDecimal`；
- uint32：`flightSoftwareVersion`、`middlewareSoftwareVersion`、`osSoftwareVersion`、`boardVersion`；
- uint16：`vendorId`、`productId`；
- 小写 wire-order hex：`flightCustomVersionWireHex`、`middlewareCustomVersionWireHex`、`osCustomVersionWireHex`（各 16 位），`uid2Hex`（36 位）。

中间件/OS 版本与 custom bytes 可为零；选择元组中的 flight/board/vendor/product 不可未知。哈希不包括 age、epoch、revision、display、派生 known 标记、模型 ID 或签名字段。MAVLink 2 合法尾零裁剪由原生接收层零填恢复，不能以消息不足 78 字节推断 UID2 已被认证或必然无效。

结果 `matched=true` 仅表示存在一份完整元组对应的本地声明，返回 `descriptorId`、`descriptorHash`、`sourceCommit`、有界 `engineeringReview`（同值 `declarationReview`）、目录身份、原生内容绑定与稳定 `fingerprint`。失败有明确 `errorCode/reasons`，没有时钟、推进中的 age 或整个目录副本。`ok=true, matched=false` 表示观测合法但无对应声明；未知、损坏、过期或来源不符则 `ok=false, matched=false`。

`firmwareAdapterCertified`、`signatureVerified`、`authenticated`、`sourceCommitProven`、`hardwareIdentityCertified`、`executionVerified`、`executionPermitted`、`startPermitted`、`executable`、`flightCommandReleased` 永远为 false。匹配、哈希、工程声明导入都不能解锁、启动任务或放行真机。

## CAS、导入与资源限制

变更要求 `expectedRevision` 是与当前值相同的规范 uint64 十进制字符串，不接受数字、前导零或溢出。成功新增/撤销递增一次；revision 达 uint64 上限后拒绝修改，不回绕。若 `changed` 消费者重入修改或删除对象，返回值仍是本次固定 receipt，`stillCurrent` 明确反映它是否仍有效，不覆盖嵌套修改的结果。

版本化文件根对象恰含：`schemaVersion=1`、`kind=local-firmware-adapter-declarations`、`catalogRevision`（十进制字符串）、`descriptors`（严格版本 1／2 声明列表）。外层不因新增显式关系自动升级。目录不保存命令审批、认证、实时观测、飞行状态或旧 epoch；关系中的参考哈希仍只是工程选择材料。导入逐项验证并暂存，全部成功才替换目录；重复 ID、重复完整固件元组或任一损坏声明整批拒绝，不部分提交。导入创建新 UUID epoch，revision 取本地与归档 revision 的最大值再加一；达到上限拒绝。归档恢复标记仅表示工程声明来源，`commandApprovalRestored=false`。

最多 64 条；UTF-8 输入与文件最大 1 MiB；JSON 解析前检查嵌套深度、节点预算与重复对象键（包括转义后相同的键）；深度最多 8、节点最多 8192，解析后再次检查预算与严格字段。文件读取有大小、完整 UTF-8 和读取前后长度检查；同长度并发替换、跨进程审批、文件内容签名与审阅证据存储不在此目录的认证范围内。`QSaveFile` 保证本次保存的原子替换，不等于跨地面站控制权租约。

工程师下一步仍需提供真实版本报文和可追溯审阅材料，完成编译/仿真/硬件验证与发布审批；本目录没有代替这些步骤。

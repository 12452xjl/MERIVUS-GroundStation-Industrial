# FleetTelemetryArchiveHistoryReader：独立只读 journal 历史

## 493：独立跨Run实际记录元数据检索

旧catalog1／page request11、Header19／Frame29／Footer31及页结构保持；search独立解析，不先读raw页再在QML滤。canonicalSearchManifest严格核≤128条原exact8 descriptor、basename升序／无重复，独立持有所有字符串，对完整ordered列表计算内容摘要；坏header描述也保留。摘要不是签名或全文件内容身份。

search request exact8：kind="search"、manifest、manifestHash、expectedDirectoryIdentity、afterFileIndex、vehicleId、messageId、limit。index/sysid/msgid/limit为真实非null Int；vehicle0为全部、1..255为单机；message=-1为全部、0仍是HEARTBEAT、上限16777215；limit≤64及原policy。首批index0且目录身份空，续批非零且用原native实际目录身份。caller不能由QML提供这些完整原生结构。

每批前后均实际枚举、核目录对象与全部filename/size/mtime/header8同原manifest；目录身份也须一致。完整单file按所有行核Codec规范、run／epoch→sysid／consent、archive与各source序递增、真实Footer数／字节／末序镜像，并重开第二hash。实际记录同时满足sysid/msgid才计数。发现后段完整坏行不能保先前命中；普通坏file为unavailable未知，已观察来源变化则整批stale，取消无结果前缀。合规最终无LF只标partial且不解析；缺Footer允许已验证完整前缀，非全遥测或永久封存。

独立search硬上限：单file扫描64MiB、每批≤64file行、结果≤1MiB减原生facade预留。总读预留130MiB（136314880）：两轮全manifest header各至多4098B／file，再为每个可扫goodheader file预留2×fileBytes；预算不足在下一file边界停。旧broad native政策保持，goodheader但大于64MiB仅给固定cap未知诊断，不整file读。每检查file都给一行，无命中也推进nextFileIndex；不自动续批、不承诺I/O时限。

native结果exact34=原claims13＋21字段：format／version／kind、manifestHash／directoryIdentity、vehicleId／messageId、afterFileIndex／nextFileIndex／totalFileCount、inspectedFileCount／verifiedFileCount／matchedFileCount／diagnosticFileCount、hasMore／budgetLimited、readByteBudget／reservedReadBytes、rows、readOnly／payloadPubliclyExposed。searchResultMatches独立重核完整请求、连续index／原descriptor、状态计数与精确预算，不能只信自报flags。

每row exact10：fileIndex、descriptor（原8）、fileHash、status、errorCode、matchingFrameCount、firstMatchingSequence、lastMatchingSequence、footerState、tailState。status为matched／no-match／unavailable；已验证零命中count及首末均"0"，未知则count／首末／hash全空，Footer／tail为unknown，不伪造0。首末是archive记录序号，不是时间、sourceSequence或跳转cursor。元数据无payload／帧时间／source epoch；目录identity只由Archive native续批持有。

跨批不认证已看过文件内容永久不变、全集或跨file原子快照；[Archive](FleetTelemetryArchive.md)只把当前成功hit原hash带入旧单file首页，UI不自动生成[运动报告](FleetTelemetryMotionReview.md)。[36工程场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

310–314 的 catalog 改接共享 `FleetArchiveDirectoryInventory` Native error-aware 枚举，不再有 QDirIterator fallback；前后都必须实际成功 EOF、通过 identity 核对和受限 metadata 检查，且两个实际 directoryIdentity 一致。成功 catalog 的 `directoryInventoryEnumerationCompleted=true` 仅说明这两次受限枚举均完成，不是全集/跨文件原子快照认证；`directoryEnumerationCompletenessVerified=false`、`fullRecordVerification=false` 和所有授权/执行禁止标志不变。entry8、request11、page29/raw rows 和盘内 header/frame/footer 精确格式未改，未运行目录/文件/应用或测试。

本模块读取[Telemetry Worker](FleetTelemetryArchiveWorker.md)产生的固定 namespace NDJSON，复用[Codec](FleetTelemetryArchiveCodec.md)的严格 header/frame/footer。它不连接 live Vehicle、Source、Writer、Registry、审批、Authority 或控制对象，不写入、建文件/锁、SQL、改名、删除、导入或回放。成功表示本次有限读取/校验通过，不认证飞机、CRC、签名、授权或永久关闭。

## 单次 native 生命周期

构造 `FleetTelemetryArchiveHistoryReader(canonicalAbsoluteDirectory, exactReadPolicy, queryId, exactRequest)` 是无 parent 的单 query QThread，构造只持值、校参数，不读磁盘。queryId 为 `1..128` ASCII 字母数字、`_`、`-`；runId/来源 epoch 仍为规范小写 UUID。根有独立 native nonce/对象/代次，不能用用户 queryId 本身代替实例身份。

requestCancel 设置 atomic flag；读取目录、每行/每 hash chunk、结果交付前合作核对，不中断正在执行的文件系统调用，没有 I/O 时间保证。current 为 mutex 保护的状态/禁止声明和 cancelRequested，不公开原始 rows 或目录。run 终结恰好发送一次 resultReady(queryId, historical) 或 failure(queryId, fixedCode)，取消也是 failure，不是静默返回。cancel 与最后 emit 仍可能交错，根必须检查实际 query 实例/nonce、ignore late result，且同时收到 terminalDelivered 与实际 finished 才复用槽位；状态 finished 或结果信号都不等于线程已经退出。

根接 actual finished→deleteLater；不能运行中删除、wait/join、terminate 或同步等磁盘。没有每 frame queued signal。源生命周期/当前 capture 与 query 的互斥、读取许可和隐私同意由根控制；Reader 不复用写入权限，不因看见历史就重开采集。

## 精确政策与请求

readPolicyValid 与 Codec 共用精确 4 个真实 Qt 整型字段，拒绝 extra/missing/bool/double/string/invalid/typed-null，无隐式调值：maximumFileMiB `1..1024`、maximumCatalogFiles `1..128`、maximumPageMiB `1..8`、maximumPageFrames `1..256`。

catalog request 恰好 `{kind:"catalog"}`。page request 恰好 **11** 键：kind="page"、basename、runId、fileBytes、modifiedUtcMs、headerHash、expectedFileHash、afterSequence、vehicleId、includePositionPayload、limit。字符串必须真实非 NULL QString；fileBytes 正规范十进制且在 reader cap 内，modifiedUtcMs 正规范十进制，headerHash 小写 64 位 SHA，expectedFileHash 首页可空、续页必须固定 64 位 SHA，afterSequence 为 `0..INT64_MAX` 规范十进制。vehicleId 是真实整数 `0..255`（0 为全部），includePositionPayload 是真实 bool，limit 为 `1..maximumPageFrames`。

basename 仅接受 `fleet-telemetry-<runUUID>.ndjson` 且 runUUID=请求 runId，不接受路径、任意文件或别的 namespace。目录须原值非空≤4096/无 NUL，规范绝对存在可读、本地非 UNC、无 symlink；候选须规范普通可读文件，不追 symlink。文件来源不是机载身份认证，私有目录 ACL/硬链接/未知外部 writer/恶意 TOCTOU 仍需部署隔离。

## cheap catalog 不是全库验证

catalog 共享 Native inventory 逐项枚举所有/隐藏目录条目，最多 4096，case-insensitive `fleet-telemetry-` namespace 文件最多 maximumCatalogFiles；枚举/关闭错误、超限或目录 identity 不稳定都拒绝，统一固定 `telemetry-history-directory-inventory-failed`，取消仍为 `telemetry-history-cancelled`，不把失败前缀当成功空列表或泄漏 OS 错误文本。随后继续原来的 Qt metadata、规范小写 UUID basename、普通文件/无 symlink/规范路径核验，同时要求 Native regularFile/non-symlink。basename 排序、不用 UTC 排序。每 entry 精确 8 键：basename、runId、fileBytes、modifiedUtcMs、headerHash、headerValid、positionTelemetryConsent、errorCode。已知空字符串使用非 NULL QString 空值；缺失/非法 metadata 为字符串 0，headerValid=false 时 consent=false/空 hash 只是不可用诊断，不是实际许可证明。

空 NewOnly 候选、超 reader cap、header 不完整/错误或不合 stored policy 的文件保留在诊断 entries，不隐藏失败创建文件，也不能 page。仅读取至多 4098 bytes header line；检查 QFile error、完整 LF、JSON≤4096、Codec 规范 header/run/source/policy。headerHash 为原 canonical header JSON（不含 LF）的 SHA-256；fileBytes/mtime 为实际文件 metadata。catalog 前后重新枚举 filename/size/mtime 列表，并比较两个 Native directoryIdentity；任一变化拒绝整体结果。identity 仅在私有读取栈比较，不进入 catalog/公开状态。

catalog 格式 `merivus-fleet-telemetry-catalog` version1，成功时新增 directoryInventoryEnumerationCompleted=true，fullRecordVerification=false。历史上 305–309 的 Qt iterator hasNext 路径无法区分正常 EOF 和枚举错误，已在 310–314 替换；当前通过 Native error-aware 成功 EOF 仍不宣称 directoryEnumerationCompletenessVerified，后者与 crossFileAtomicSnapshot 保持 false。cheap catalog 不扫描每条 frame/hash，不把各文件读出的 metadata 冒充同一跨文件原子快照，也不把 identity 对照说成恶意并发下的目录全集认证。

## 全文件 prefix 页校验

page 必须匹配实际 basename/run/size/mtime/headerHash，文件≤reader maximumFileMiB 且≤header 自己的 maximumFileMiB。只读新 QFile handle，不复用 live Writer。header sources 冻结来源 epoch→vehicleId；指定非 0 vehicleId 必须在 scope 中。

虽然只返回有限 rows，但每次流式扫描整个 file：每次 readLine≤4098，完整存储行（JSON+LF）≤4097，header/frame/footer JSON≤4096。每条 frame 经 Codec canonical/29 键检查，再核 run、sysid/epoch 和 position consent 对应 header。archive sequence 为正 signed64 严格递增，sourceSequence 为每 epoch 的正 UInt64 严格递增；过滤产生空档合法，不能凭空推丢包。所有完整 frame 都校验，不因 limit 满、位置隐藏或 vehicle filter 跳过剩余校验。

footer 若出现必须是最后一个完整 LF 行，并由 Codec 检查真实 scanned frame 数、含 header/每帧 LF 的 bytesBeforeFooter 和 lastSequence 镜像。footer 后任何数据、内部坏 canonical/重复序号/错误 scope 都使整页失败，不交付部分成功。不存在 footer 可读取有效前缀；最终无 LF 且≤4097 bytes 的尾部标 partialTail=true，**不解析、不返回该尾行**，其余完整前缀仍校验。更大尾行或内部截断拒绝。

所有实际 bytes（含 header/LF/footer/partial tail）加入 SHA-256；只在首次 handle 关闭且 metadata 未变后，再 readonly reopen，用最多 64 KiB chunk 重读完整 hash、核前后规范路径/size/mtime/EOF/QFile error，必须 hash 相同。expectedFileHash 非空还要匹配这次实际 digest。普通 append/替换/变化失效，不把稳定暂停的 writer 前缀说成永久不可变或恶意并发下的密码学身份保证。

afterSequence 仅 0 或实际完整、已验证 prefix 中出现的 archive sequence；猜想/过滤跳号不能当 cursor。nextSequence 是最后实际返回 row 的序号（无 row 保留原 after），hasMore 只表示还有符合 cursor/vehicle/位置输出条件的已验证 row。根另冻结实际返回 cursor/filter/hash 域，不能仅用 caller 自报 JSON 恢复续页。

## 返回预算与隐私边界

page 格式 `merivus-fleet-telemetry-history-page` version1，包含 basename/run/headerHash/fileHash/bytes/mtime、sources、after/next/hasMore、rows、totalValidatedFrames、lastValidatedSequence、positionWithheldFrames、footerPresent、partialTail 和 footerDiagnostic。positionWithheldFrames 只计 cursor 之后、符合当前 vehicle filter 且因 includePositionPayload=false 未返回的位置 frame；这些 frame 仍校验，且不被当作 hasMore 可输出 row。

rows 最多 limit 且同时限制实际紧凑 JSON envelope 为 maximumPageMiB−1024，给根 metadata 留空间；达限停止增加 rows、保留 hasMore，但继续全文件校验。首 row 无法放入则拒绝，不向上取整/静默超限。目录/文件/逐行/rows/hash buffer 有界，不宣传全进程恒定 RSS、低延迟或磁盘 I/O 截止保证；1 GiB 文件和多目录项仍可能耗时。

rows 是完整原始 frame map，仅 resultReady 的 native 根消费，**不是 QML 直接公开 API**；根必须固定投影剥除 payloadBase64/原始 header/footer 自由内容，实际 native detail 也需独立确认。includePositionPayload=false 不返回位置 frame，true 仍须根实际授权，不能把 bool 当账号/来源认证。source IDs/epoch 与历史 claimed parser/signature 标记也不成为可信飞机性能/控制许可。

两种结果均 historicalOnly=true；authenticated、crcVerified、permissionsRestored、executionPermit、executable、flightCommandReleased、fullTelemetryCaptured、closureVerified、crossFileAtomicSnapshot、directoryEnumerationCompletenessVerified、fsyncVerified、durableVerified 全 false。page 另示 completeRecordPrefixOnly=true；盘内 footer.clean=false/closeCandidate 不是最终 write/flush ack，也不证明全遥测、永久 closure、任务完成或授权恢复。不得将结果回灌 live BlackBox/Registry/Replay/任务/审批/Authority。

## 当前证明范围

仅源码实现、限定同行审阅和文本检查；未运行 Reader/Codec/readPolicy/inventory helper、目录枚举、QFile、hash、Qt 应用、编译、测试或设备。工程师仍需验证实际 Windows/POSIX 枚举 EOF/中途错误/权限/关闭/identity 替换、Qt 5.15 文件/EOF/部分尾、scope/UInt64、descriptor/hash续页、恶意文件/巨大目录、位置保密投影、预算、取消晚到、双终结和 source/query 互斥。当前不宣称真实历史运行验收、CRC认证、全遥测留存或永久关闭证明。

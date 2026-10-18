# FleetEventArchiveHistoryReader：封存段与离线未封存库只读历史

## 538：后台任务索引复用原搜索与完整来源复核

新增原生静态 `searchInWorker(directory,policy,request,cancelled,result,error)`，先冻结输入、清输出，仅在实际应用存在且当前不是应用owner线程时可调用。它复用原 `localDirectory`、严格七字段policy、searchRequestValid、完整readSearch和searchResultMatches，不另建Reader／连接服务、不放宽旧search域。失败不返回前缀。

`snapshotIndexSourcesInWorker(directory,policy,scope,inventoryFingerprint,cancelled,fileIdentities,error)` 在同一原inventory下冻结全部实际文件的exact6 `{fileIndex,basename,databaseEpoch,sealRunId,fileFingerprint,offlineCurrentOnly}`，严格按0..N−1排序、最多129项；失败不返回可用前缀。它与 `verifyIndexSourcesInWorker(directory,policy,scope,inventoryFingerprint,fileIdentities,cancelled,error)` 共用同一个内部文件探测器：前后完整枚举同原searchInventory并绑定原inventory fingerprint／目录身份及完整清单，对每个来源重新核原完整字节SHA、SQL schema／实际epoch，封存还核sealRunId／clean。verify接真实Int／String／Bool、index唯一的完整身份列表，TaskIndex保存全文件列表而非只保存已索引前缀，因此partial索引下钻到后面文件也有稳定旧hash可重核。

两API复用原前后文件hash与header、sidecar／普通文件／canonical和大小／mtime检查，不仅按元数据认旧检查点。未封存探测在原`.owner.lock`持有期间完成前后hash、SQLmetadata及一行只读页；verify内部从row0也明确绑定原epoch／hash，snapshot首次则冻结实际epoch／hash，不放宽公开offline首读合同。

新 `kind='task-index'` 的request校验、执行及结果值核对分别委托 [FleetArchiveTaskIndex](FleetArchiveTaskIndex.md)，沿原一个Reader与实际result／failure＋finished生命周期，不新增QML或飞行API。索引模块或Reader末尾在checkpoint已commit后才观察到取消时，仅原execute确已成功且checkpointCommitted为真实Bool true的成功结果保留提交事实，并将cancelObservedAfterCommit标true后再次闭合同核验；不由generic cancelled冒称未提交。提交前及未commit的open／page动作迟取消仍失败，全部结果保原最终有界门面字节预算。其它catalog／page／offline／search分支未改接受域或取消行为。

来源复核是逐文件观察，锁只覆盖实际离线库的该次读取；不证明跨文件原子快照、恶意同字节还原未发生、全集保留、文件／设备认证、正式任务完成或磁盘耐久。合作取消仍在枚举项／hash块／SQL阶段间检查，不声称可抢占SQLite或磁盘IO。仅源码编辑、完整直接静读及普通文本核对，未执行Reader／索引／SQL／枚举／hash／helper／业务IO、编译或测试。下方为历史。

## 484：跨库搜索识别性能声明操作

searchRequest闭合operation集合新增approve-performance；原逐记录工程摘要过滤使用相同operation，不增加新的Reader、目录、SQL、payload来源或重放路径。空operation仍表示原全部记录，旧四操作及既有当前页／封存／离线查询合同不改。

新操作的独立type/hash与原exact21摘要由[Codec](FleetEventArchiveCodec.md)核对，[Store](FleetPersistentEventStore.md)及[历史面板](FleetArchiveHistoryPanel.md)使用同新选择。原分页／预算／取消／文件身份及非授权边界保持；搜索不到不证明没有历史或从未提交。无数值／账号／理由／配置／凭据新增，旧档不回填。未编译、执行Reader／SQL／业务或测试，工程验证全未执行。下方为历史。

第 424 批新增跨文件历史检索：按已记录的 vehicleId 和工程审批 operation 搜索多个封存段，并可显式包含停止录制的未封存库。查询为逐批继续的只读浏览，不是全集证明、任务恢复或实时规则回放。原 catalog/page/offline 请求和单页 DTO 保持兼容；新增 `.h` 两个静态值型契约方法供 Store 复核。

第 461 批在同一 search 入口增加可选的精确 taskIdHash 过滤；原无此键的请求与结果保持原版，不扩展 SQL schema、文件范围、单批预算或授权能力。任务哈希仅来自已保留诊断行的真实 `record.event.payload.taskIdHash`，不能反推出任务名称、任务完成状态或未保存的完整任务。

本模块接在[持久化门面](FleetPersistentEventStore.md)后面，读取[存储线程](FleetEventArchiveWorker.md)明确封存的诊断摘要库，或显式读取已停止录制的固定未封存库。它只拥有目录、政策、queryId 和 request 的值副本；没有 Vehicle、BlackBox、Replay、Registry、MissionManager、审批或控制权对象。查询不创建目录/数据库，不改名、删除数据库、自动恢复 hot journal 或回灌控制状态。封存 catalog/page 路径仍不创建锁文件；第 423 批新增的 offline 路径会建立/释放与 Writer 相同的 `.owner.lock` 协作锁，但不启动采集或迁移、修复数据库。

第 423 批补齐的是跨重启、未封存但数据库本体可完整只读打开时的历史浏览；不是崩溃日志修复、完整审批档案、全集搜索或任务恢复。`.h` 接口在第 423 批未变；offline 请求由既有值型构造器接收。

## 请求与生命周期

构造 `FleetEventArchiveHistoryReader(canonicalAbsoluteDirectory, exactPolicy, queryId, exactRequest)` 不做文件或 SQL I/O。QThread 无 parent；门面只在明确 native 配置、操作者同意和查询容量允许后调用 start。结束通过 `finished -> deleteLater` 回收，不允许运行中析构、UI `wait()` 或 `terminate()`。`current()` 是 mutex 保护的诊断值，不包含路径。

queryId 为 1–128 位 ASCII 字母数字、`-`、`_`。政策与 Writer 一样必须恰好有 7 个实际 Qt 整型字段；拒绝 bool、double、字符串及隐式默认：maximumEvents 1000–1000000、maximumDatabaseMiB 32–4096、maximumQueueEvents 16–4096、maximumQueueMiB 1–64、batchEventLimit 1–128、busyTimeoutMs 1–1000、maximumHistoryMiB 1–8。Reader 不使用队列字段扩大任何查询容量。

request 为以下四种精确形状：

- Catalog：恰好 `{kind: "catalog"}`。
- Page：恰好 7 键 `kind="page"`、basename、databaseEpoch、sealRunId、runFilter、afterRowId、limit。前 6 键均实际 QString，limit 是实际 Qt 整型 1–256。runFilter 为空或规范非零小写 UUID；afterRowId 是 `0..INT64_MAX` 规范十进制字符串。
- Offline：恰好 6 键 `kind="offline"`、databaseEpoch、fileFingerprint、runFilter、afterRowId、limit；前 5 键均实际非 null QString，limit 是实际 Qt 整型 1–256。runFilter 必须为明确的空字符串，读取该库所有 run 的保留行。afterRowId 同上：首读必须为 `"0"` 且 epoch/fingerprint 均为明确的空字符串；后页 cursor 必须大于零且 epoch 为规范非零小写 UUID、fingerprint 为 64 位小写 SHA-256。无 basename/path/catalogId/sealRunId 输入。
- Search：原版恰好 5 键 `kind="search"`、scope、vehicleId、operation、cursor。第 461 批允许仅增加 taskIdHash 成为恰好 6 键；有该键时必须是实际非 null QString 的 64 位小写十六进制 SHA-256，空串、null、数字、大小写变体、缺位和附加字段均拒绝。三个原文本字段 kind/scope/operation 均实际非 null QString；vehicleId 为实际 Qt Int 0–255（0 表示全部）；cursor 为实际 QVariantMap。scope 只接受 `sealed` 或 `sealed-and-offline`；operation 只接受明确空字符串（不限操作）或 approve/revoke/approve-baseline/revoke-baseline。

Page 的 basename 仍仅接受 `fleet-events-sealed-<databaseEpoch>-<sealRunId>.sqlite`，两个 UUID 均为规范小写非零形式，且请求 UUID 必须与 basename 一致。不接受 caller 任意路径、current 库名、大小写变体、额外后缀或路径片段。门面还必须从实际成功 catalog 的冻结条目取 UUID，而不是信任调用者填写的 catalog/map。runFilter 可以指向同段内其它旧 run，不改变 sealRunId 的含义。Offline 使用独立 namespace 分支，只能读取 native 固定 `fleet-events-v1.sqlite`；封存调用不因该扩展而接受 current basename 或任意文件名。

每次查询只投递一次有界 `resultReady(queryId, result)` 或固定错误码 `failure(queryId, code)`，不为每段/行发送 queued 消息。门面将所有实时页、封存及离线查询合用一个待完成槽；Reader 查询必须等结果/失败实际送达 GUI 且真实 finished 送达后，才允许下一个请求，取消请求不能提前释放槽或复用 queryId。取消使用原子 flag，目录逐项扫描、文件 hash 的 64 KiB chunk、段间、SQL 页行间及最终交付前合作检查。SQLite 调用和磁盘读本身不能由该 flag 强行中断，busy timeout 不是 SQL CPU、磁盘 I/O 或线程完成的绝对期限。

## 封存身份和文件边界

目录必须已存在、规范绝对本地目录；拒绝 UNC、非目录、symlink 和 canonical 路径不一致。文件必须是同目录内的普通非 symlink 文件，512 字节以上且不超过本次 maximumDatabaseMiB；所有 `-journal`、`-wal`、`-shm` sidecar 必须不存在（包括 dangling symlink）。路径检查不是 Windows junction/硬链接、恶意置换或私有目录 ACL 的完整保证，部署仍需要工程师限制目录访问。

每段使用全新 UUID SQL connection，在 run 线程建立/使用/关闭，`QSQLITE_OPEN_READONLY` 加同政策的有界 `QSQLITE_BUSY_TIMEOUT`；读取核对实际 busy_timeout 和 journal_mode=delete，不写 pragma、不请求 QLockFile，不自动恢复 sidecar。descriptor 的只读事务先核[共享 SQL 身份](FleetEventArchiveSql.md)，要求实际 meta.database_epoch 等于 filename UUID，并且实际 filename 对应 sealRunId 的 typed clean_shutdown=1；其它旧 run 允许 unclean，不能把整个段当作“所有原始事件无损或完整”。descriptor 包含实际 typed retiredCount、retainedFromRowId、retainedThroughRowId，均以十进制字符串表示。

SQL open 前及所有查询/connection close/remove 后，取得有界完整文件 SHA-256，核对 SQLite 100-byte header、DELETE 格式、application_id=1296389702、user_version=1、page size/count、header change counter、文件大小/mtime/规范路径和 sidecar。前后 header/hash/大小/mtime 必须一致；hash 是变化诊断而不是签名、加密、匿名化或防恶意替换证明。所有 QSqlQuery 作用域均先结束，之后 close，再销毁数据库 handle，再 removeDatabase；无 SQL 对象进入 GUI。

## 第 423 批：离线未封存库

仅在显式 offline 请求中，Reader 在线程内对固定 currentPath + `.owner.lock` 做与 Writer 相同的预检查：拒绝 symlink、非普通文件、超过 16384 字节的既有锁文件，以及 canonical 路径不匹配。随后 `QLockFile::setStaleLockTime(0)`、`tryLock(0)` 非阻塞取得协作锁。零时间禁止按锁年龄抢占，不保证 Qt 永不处理死进程锁；本模块不手动删除锁文件或强制解锁。实际锁由 QLockFile 生命周期建立和释放，目录必须已存在且允许建立该锁；只读数据库并不意味着目录零写入。封存 catalog/page 不进入此锁路径。

协作锁从读取前的完整文件指纹之前持续持有，跨 SQL 建连、descriptor/readPage、close、所有 SQL handle 析构、removeDatabase 和读取后的完整文件指纹，直至本次函数退出。遵守同协议的 Writer 不能同时写入或封存；锁不抵御绕过协议的第三方写入、恶意路径置换或不合适的 ACL。`cooperativeOwnerLockHeldDuringRead=true` 只陈述本次读期间持锁，结果到达 GUI 时锁已经释放，也不代表外部文件已认证。

读取只使用固定 current basename，复用与封存段相同的文件大小/header/hash/canonical/sidecar 检查和 `QSQLITE_OPEN_READONLY` 配置。任何 `-journal`、`-wal`、`-shm`（含 dangling symlink）均拒绝，不尝试恢复、清理、迁移或创建数据库。先用只读 descriptor 事务检查原 schema/meta/typed 值、取得实际 databaseEpoch 并结束该事务；再以同实际 epoch 调用共享 readPage 的独立事务，不嵌套 BEGIN。Offline 不调用 sealedRunMatches，也不要求全部 run clean；每条历史行继续保留 `runCleanShutdown`、`uncleanRunMayHaveLostTail`、durable/drop/discard 信息。

首读将实际整库 SHA-256 返回为 `fileFingerprint`。后页要求请求携带相同整库 hash 和实际 epoch；原有 cursor 仅能在该库该文件版本继续使用，不按墙钟排序、不自动跳页。每次读取前后还要 header/hash/大小/mtime 一致。中途文件变化或后页 hash 不同返回 `archive-offline-file-changed-restart-required`，调用方应显式重读首页，不把旧页和新库拼为同一历史。即使 hash 相同，也不证明期间从未变化后还原，更不证明历史全集、消息来源或磁盘耐久性。

成功结果是原共享 history-page 的 20 键加固定 basename、`offlineCurrentOnly=true`、`sealedSegmentOnly=false`、fileFingerprint、`cooperativeOwnerLockHeldDuringRead=true`，以及 executionPermit/flightCommandReleased/restoresAuthorization=false，共恰好 28 键；不添加 catalogId 或 sealRunId。runFilter 始终是明确的空字符串；不是运行中 current 查询的当前 run 过滤。按最终紧凑 JSON 重算实际整型 `pageBytesWithoutThisField`，readPage 内层仍预留 1024 字节，Reader 最终仍留至少 512 字节给门面。读取结果不导入任何授权或实时任务状态。

本路径新增固定错误码以 `archive-offline-` 开头，共享 SQL 仍可返回既有 `archive-history-` 错误；取消终态为 cancelled。缺失库、锁不可用、未知 schema、sidecar、损坏页或指纹变化都只拒绝，不通过新建空库伪装成无历史。实际飞行和物理设备不参与此路径。

## 第 424 批：跨文件检索

### 范围、指纹与游标

Search 首次 cursor 必须为空 map。续查 cursor 恰好 5 键：inventoryFingerprint（64 位小写 SHA-256）、fileIndex（实际 Qt Int 0–129）、afterRowId（规范十进制字符串）、fileFingerprint、databaseEpoch（均实际非 null QString）。afterRowId 为 `"0"` 表示尚未开始的新文件，其 fileFingerprint/databaseEpoch 必须为明确空字符串；afterRowId 大于零才必须给出该实际文件的整库 hash 和规范 UUID。Reader 还会对照本次实际目录条目数拒绝越界 index；Store 只应使用上一次 native 返回的 cursor，而不是让 UI 自填跳跃位置。

先复用完整有界 native 目录枚举，封存段按 basename 排序，最多 128 个。`sealed-and-offline` 必须存在且通过安全检查的固定 `fleet-events-v1.sqlite`，追加为最后一项，总计最多 129；缺失、sidecar 或坏文件报错，不静默当成“无历史”。`sealed` 范围可有零个文件，但空目录结果不证明历史从未存在。不会把任意用户文件或路径加入检索。

inventoryFingerprint 绑定原 native directoryIdentity、scope、按序列出的 basename、文件大小、mtime，以及封存文件名中的 epoch/sealRunId；任何无效 mtime 均拒绝，不把不可观察时间转换为零。未封存项在该元数据清单中的 epoch/sealRunId 为空，只有实际打开其数据库时才取得并绑定真实 epoch。每批开始、结束都重新枚举并核对 inventory/hash、同序条目统计；跨批续查也绑定同一 inventoryFingerprint。它是元数据观察的一致性检查，不包含所有文件完整内容 hash，不证明已完成文件前缀没有同元数据替换，也不是跨库原子快照、历史全集、权限或来源认证。

对本批实际读取的文件，前置文件指纹的大小/mtime 必须与 inventory 项一致；前后完整 header/hash/大小/mtime 仍须一致。活动文件的续查还绑定整库 hash 和实际 epoch，改变即拒绝并要求从头开始；到下一文件时 cursor 的 hash/epoch 清空。未封存读取复用第 423 批 `.owner.lock` 协作锁，SQL close/removeDatabase 和后置 hash 期间均持锁；封存文件仍不创建锁。目录清单不持有覆盖所有文件的写锁，外部 Writer 是否遵守协议仍是部署边界。

### 有界扫描、过滤与缺口

每次最多调用 4 次文件页读取，每页请求上限是 `min(64, 64 - 本批已匹配行数)`，所以最多扫描 256 行、返回 64 个匹配。游标推进依据实际扫描并消费的行，而不是最后一条匹配：零匹配结果可以仍然 `scanComplete=false`，必须继续查询才能检查后面的记录。完成标记只说明本次所观察清单的保留行已经扫描到末端，不代表曾发生的事件全部存在。

过滤检查已由 SQL/codec 解码核验的 `row.record.event.payload`。vehicleId=0 不限制飞机，1–255 要求 payload 存在相同数值 ID；JSON 解码可能产生整值 Double，因此只接受数值类型精确等于该整数，不把 bool/string/null 转成已知飞机。operation 非空时必须为 `fixed-diagnostic-summary-v2`，带 21 键 engineeringReview/schemaVersion=1 和实际相同 operation。旧 v1 不记录审批详情，operation 过滤时是不匹配，不代表历史没有审批；不限 operation 时旧 v1 仍可返回。

taskIdHash 存在时，先检查 payload 中同名字段是实际非 null QString，且与原请求哈希精确相同，然后才允许 operation 为空的早返回。飞机、任务哈希和操作条件是相交关系；没有 taskIdHash 的历史行不是匹配，不能从 task/summary/name/planIdHash、车辆 ID、时间窗口或 runId 猜测补上。Codec 的 v1/v2 诊断投影都可能合法保留 taskIdHash；这里的 search-result schemaVersion=2 只标记新增筛选契约，不等同于 Codec 的 projection v2，也不因此排除合法的旧 projection v1。无该筛选键时仍按原逻辑返回旧行。

每批输出在原 maximumHistoryMiB 限制内，为最多 4 项覆盖摘要、cursor 和外层元数据预留 16 KiB。加入下一条匹配将超过行预算时，在消费该行之前停止，返回最后已消费 cursor；不会跳过未返回的匹配。即使已读到原 SQL 页的一部分，覆盖摘要也只统计实际已消费前缀，并将 hasMore 保持 true。单条合法 codec 记录最大 512 KiB，当前最小 1 MiB 历史预算可容纳首条和元数据；最终仍核对紧凑 JSON 完整字节数并为 Store 留至少 512 字节。任何读取/校验/取消错误只返回失败，不投递部分成功结果或暗示完整覆盖。

### 返回契约

Search result 恰好 28 键：format=`merivus-fleet-event-history-search`、schemaVersion=1、scope、vehicleId、operation、inventoryFingerprint、filesTotal、filesCompleted、scannedRows、matchedRows、scanComplete、nextCursor、rows、scannedPages、ordering=`basename-then-row-not-flight-time`、historicalOnly=true；permissionsRestored/authenticated/signatureVerified/executable/completionVerified/executionPermit/flightCommandReleased/restoresAuthorization/historicalCompletenessVerified/crossSegmentAtomicSnapshot/wallClockOrderingUsed 均为 false；最后为实际 Qt Int `resultBytesWithoutThisField`。计数为实际 Qt Int，scanComplete 为实际 bool；rows/scannedPages 是 list，nextCursor 是 map。scannedRows/matchedRows 是本批数量，filesCompleted 是全检索游标已经越过的文件数，不是本批新增文件数。scanComplete=true 时 nextCursor 为空；否则是上述严格 5 键 cursor。

第 461 批的有 taskIdHash 请求仅接受 schemaVersion=2、恰好 29 键的结果：其余 28 键不变，再加实际非 null QString 的同一原始 taskIdHash。无此键的请求仅接受原 schemaVersion=1/exact 28，结果不得夹带任务哈希；有此键的请求不得退为 v1、缺少哈希或替换哈希。`readSearch` 先加入该字段，再重算实际紧凑 JSON 字节数和整体预算；`searchResultMatches` 同时复核请求/结果版本对应、该字段和每条实际匹配行。有 taskIdHash 的 schema 2 结果在两处均须不大于 `maximumHistoryMiB * MiB - 2048`，为 Store 的 31 键 records 外壳预留 2048 字节；旧 schema 1 结果与其它读页仍为原 512 字节门面余量。原行预算减 16 KiB 不变，run 最后的共用 512 字节终检不会放松先已完成的新 2048 字节检查；Store 仍需在提交外层 DTO 前核完整紧凑 JSON 不超过原 maximumHistoryMiB，不扩大 UI 字符政策或数据库容量。游标仍原精确 5 键文件域，不内嵌或猜测任务身份；门面必须冻结原筛选并只以同一 native 查询范围续查，不能将一个查询的 cursor 作为另一个任务的完整扫描证据。

每条匹配是恰好 7 键：fileIndex、basename、databaseEpoch、sealRunId（未封存项为空）、fileFingerprint、offlineCurrentOnly、row。row 保留共享 readPage 的完整 8 键行，含原 record/contentHash、runCleanShutdown、uncleanRunMayHaveLostTail 和 durable/drop/discard 信息；本地 rowId 不升级为跨库全局 ID。身份前 6 键在对应覆盖摘要中完全相同。

scannedPages 最多 4 项，每项恰好 18 键：上述 6 个身份字段，afterRowId、nextRowId、hasMore、retainedFromRowId、retainedThroughRowId、retiredCount、cursorPrecedesRetainedHistory、scannedRows、matchedRows、uncleanRows、rowsWithDroppedEvents、rowsWithDiscardedEvents。后三项计数包含未匹配的已消费行；它们是本批行所引用 run 存在 unclean/drop/discard 的行数，不能相加当成独立丢失事件总数，也不覆盖没有保留事件行的 run。retiredCount 和保留行范围仍逐文件明确显示，不能因为匹配为零而隐去留存缺口。

`searchRequestValid(request)` 与 `searchResultMatches(result,request,policy)` 为 public static 值型契约：没有目录、文件、SQL 或线程执行。结果复核严格键数/类型、范围与过滤条件、权限标记、行与摘要身份、摘要计数、逐页游标推进及最终 nextCursor、非空批次推进和整体字节预算；完整数据库/schema/record hash 证据仍来自实际 Reader SQL 路径，不由这个包络复核函数冒充认证。Reader 投递前和 Store 接收时均使用该契约。搜索不导入 BlackBox、TimelineReplay、Registry、任务、审批或控制权。

## Catalog 和分页

目录观察改由共用 native [FleetArchiveDirectoryInventory](FleetArchiveDirectoryInventory.md) 执行，调用仍只位于 Reader 后台 `run()` 路径。它逐项计算全部 entries（包括 hidden/system/nonmatching），每次最多 4096，匹配封存 namespace 最多 128 项；使用 Windows/POSIX 明确的枚举结束和错误通道，无 Qt iterator fallback。取消、打开/遍历/关闭错误、平台不支持、目录身份变化或超限均不接受部分清单，Reader 返回固定错误码。原 Qt 5 QDirIterator 缺少独立枚举错误通道的旧限制已由本次 native 接线替代，但没有因此获得全集认证或目录原子性。

与封存 prefix 不区分大小写地匹配的条目仍必须通过原 basename 小写 UUID、普通非 symlink 文件、canonical 路径、大小和无 sidecar 检查；native entry 的 regularFile/symlink 结果是额外约束，不替代 QFileInfo 复核。任何坏 namespace/目录/sidecar、段身份不符或 cap 超限都整次拒绝。逐段核验前后各完成一次 native inventory，私有目录观察 identity 必须相同，再比较段文件名、大小和 mtime。identity 不公开到 result/current，entries 仍每项恰好 7 键。

只有前后完整枚举及其余核验均成功的 catalog result 才给出 `directoryInventoryEnumerationCompleted=true`；page 不添加此字段。该字段仅说明两次有界枚举观察到正常结束，不证明从未漏采、旧段未被删除、目录 ACL/路径安全、历史全量存在或所有事件均可信。catalog 仍为逐段观察，不是跨库原子快照，无法排除检查窗口后变化、创建后删除的短暂条目或恶意同元数据置换；`catalogCompletenessVerified=false`。segmentCount 仍是本次观察并核验的条目数，空 entries 不证明历史永不存在，也不是完整备份索引。后续全集证据链、保留策略与部署验证仍需继续实现。

Catalog format=`merivus-fleet-event-archive-catalog`，schemaVersion=1，entries 每项恰好 7 键：basename、databaseEpoch、sealRunId、retainedFromRowId、retainedThroughRowId、retiredCount、fileBytes。条目按 basename 排序，仅为确定浏览，不是飞行或墙钟时间顺序；跨库没有全局 rowId。`allRecordHashesVerified=false`：catalog 检查文件字节变化、schema/meta/run 和边界，不逐条验证所有事件证据；canonical/hash/行字段一致性由具体请求的页核验。

Page 使用共享 readPage 的独立读事务，完整复用固定诊断 codec/hash/typed row 检查；descriptor 事务已结束，不套嵌套 BEGIN。页只能查该 basename/databaseEpoch/run 组合：cursor 以该库 append row_id 为域，不按墙钟排序，不可直接续接另一库 cursor。页沿共享历史 envelope，另带 basename、sealRunId、sealedSegmentOnly、executionPermit/flightCommandReleased/restoresAuthorization=false。每行保留真实 runCleanShutdown、uncleanRunMayHaveLostTail、durable/drop/discard 摘要，封存操作 clean 不覆盖旧 run 的 unclean 或缺口。

整个紧凑 UTF-8 JSON 结果同时受 maximumHistoryMiB 限制，Reader 为门面 catalogId（仅封存）及值元数据预留至少 512 字节；页源先预留 1024 字节，再加对应历史域并检查。catalog 条目、页行不会独立排队；出错拒绝整个结果。`historicalOnly=true`、permissionsRestored/authenticated/signatureVerified/executable/completionVerified/executionPermit/flightCommandReleased/restoresAuthorization=false。记录内部 queued/uncertain 可以描述过去动作，外层 false 只说明本次查询不发布飞行指令，不代表历史从未发过命令。

结果不回灌实时 BlackBox/现有 TimelineReplay 的规则入口，不恢复任务、审批、FleetRegistry 或控制权。该历史诊断库仍不是完整遥测、完整证据链、企业备份/权限审计或真实飞行完成证明。

## 本阶段证明范围

只进行了源码实现、同行审阅及限定文本核对；未构造/启动 reader、未创建/读取真实数据库、未执行 native inventory/SQL/文件 hash/helper/应用/日志回放/编译/测试/设备操作/刷机。工程师需要验证 Windows/POSIX native 目录枚举的正常结束、中途错误、关闭错误、取消、空目录、大小写异常项、全 entry/匹配 cap 和目录 identity；还需验证 QtSql/SQLite 驱动实际只读行为及 busy 选项、未知 schema/损坏 typed 值和 hash/sidecar 拒绝、封存 UUID/clean 绑定、文件变化、页容量/cursor 绑定、取消/queued delivery/finished 生命周期及长时磁盘使用。当前 `directoryInventoryEnumerationCompleted` 的实现不等于上述工程验收已经通过。

第 423 批增量验收还包括：正常停止后及进程重启后读取同一未封存库、旧 unclean run 的逐行缺口提示、运行中 Writer/另一 Reader 持锁拒绝、取消后锁与 SQL 完整释放、只读目录拒绝、异常锁路径、Qt 死进程锁行为、sidecar 拒绝而不恢复、首读/后页字段严格约束、后页库变化必须重读首页、缺失库不创建、byte budget 与旧封存入口不回退。上述事项尚未运行验证，不可据源码说明宣称跨重启功能或 Qt/SQLite/平台锁行为已验收。

第 424 批还需工程师验证：零/多文件目录、required current 缺失、跨文件 rowId 重复、当前/封存混合次序、Qt JSON 整值数值过滤、旧 v1 无 operation、零匹配但未完成、未匹配行的缺口计数、4 页/256 扫描/64 匹配和大记录字节边界、同文件/下一文件 cursor、元数据/活动文件 hash 改变、取消与实际 finished、查询错误不发布局部结果，以及旧 catalog/page/offline 行为未回退。本批仅源码和文字核对；未执行搜索、SQL、文件 hash、目录枚举、编译、测试、应用或设备操作。

第 461 批新增的工程师待验收项：旧 exact 5/v1 exact 28 与新 exact 6/v2 exact 29 严格双版本；hash 空/null/类型/64 位小写约束及换 hash/换版本/额外键拒绝；operation 空仍必须按 taskIdHash 过滤；projection v1/v2 有同哈希可返回，无哈希/异哈希不能靠同车同时间补齐；车辆/operation/任务哈希相交；每行结果匹配与新完整 JSON 字节预算；跨段零匹配续查、原筛选冻结、取消及文件/清单变化；不恢复任何任务或授权。这些仅完成源码和限定文字自阅，未执行 Reader、SQL、helper、应用、编译或测试。

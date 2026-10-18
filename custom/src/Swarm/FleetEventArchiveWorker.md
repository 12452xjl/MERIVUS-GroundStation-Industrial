# FleetEventArchiveWorker：有界诊断事件存储线程

本模块是[持久化门面](FleetPersistentEventStore.md)的 value-only SQL 后端，使用[固定诊断投影](FleetEventArchiveCodec.md)和[共用 SQL 契约](FleetEventArchiveSql.md)。schema/constants/identity/规范事件镜像/历史页现仅由共享 helper 声明，Worker 保留队列、事务写入、合作 owner 锁和封存生命周期。它没有 BlackBox、Vehicle、MissionManager、Registry、Replay、审批或控制权对象指针，不发送飞行指令。存储成功不代表原始数据真实、完整、经过签名或获得执行许可。

## 生命周期和接口

`FleetEventArchiveWorker(absoluteDirectory, exactPolicy, runId)` 的 QThread 无 parent，构造不打开文件、不建立 SQL connection。门面显式 `start()` 后，`run()` 才执行磁盘工作。门面必须将 `finished` 接到 `deleteLater`，不得在运行中析构、`wait()`、`terminate()` 或在 UI 线程等待磁盘。析构只断言线程已经结束，不提供强行停止或同步排空。

接口仅接收值：

- `enqueue(Codec::Record)`：即时有界入队，返回值仅证明加入内存等待队列，不是持久化确认。
- `noteExternalGap(quint64 lostCount=1)`：线程安全记录门面已观察到的 codec 拒绝、来源跳序/丢失/重入缺口，lostCount 必须为 `1..INT64_MAX`，只在接收入口仍开放时接受。门面不得再为 enqueue 自身超限重复报告。该诊断不是 source 所有真实缺失事件的精确测量。
- `requestPage(queryId, runFilter, afterRowId, limit)`：提交独立只读历史查询；queryId 为 1–128 位 ASCII 字母数字、`-`、`_`，runFilter 为空或规范小写 UUID，cursor 为 `0..INT64_MAX` 规范十进制字符串，limit 为 `1..256`。
- `requestStop(true)`：不再接新工作，异步完成已接收的批次和查询；排空后才提交 clean 标记。
- `requestStop(false)`：显式丢弃仍在队列的事件并计数，正在提交的批次不能被伪称撤回。该停止不写 clean 标记。
- `requestStopAndSeal(int maximumSealedSegments=32)`：显式请求停止、排空并封存，范围 `1..128`；只有接收入口仍开放、没有旧 stop/seal 才接受。返回 true 只证明请求进入 native 状态，不证明封存、重开新库或权限已获准。
- `current()`：mutex 保护的值快照；门面可低频轮询，不为每条事件投递 Qt queued signal。
- signals：`ready(runId)`、`failure(runId, fixedCode)`、`pageReady(runId, queryId, historicalPage)`。异常不公开 SQL error 文本、文件路径或原始 payload。

`requestStop(false)` 在 worker 已取得“队列为空、已接收工作全部完成”的最终停止决定前，可将原 drain 请求降为 discard；任意普通 requestStop 在该决定之前都会取消 seal 请求。最终停止决定后，不再改变已完成 drain/封存的提交窗口。SQL I/O 和文件 hash/rename 期间不持有共享 mutex，因此 UI 不等待磁盘；前台也不能通过析构强行撤回进行中的封存。进程退出或失电可中断剩余排空，不能将 stop 请求本身当作完成确认。

## 规范政策

必须恰好有 7 个 Qt 整型字段；bool、字符串、double、不认识的键或缺键均拒绝，无隐式默认或调整：

| 字段 | 允许范围 |
| --- | --- |
| maximumEvents | 1,000–1,000,000 |
| maximumDatabaseMiB | 32–4,096 |
| maximumQueueEvents | 16–4,096 |
| maximumQueueMiB | 1–64 |
| batchEventLimit | 1–128 |
| busyTimeoutMs | 1–1,000 |
| maximumHistoryMiB | 1–8 |

队列同时计算 queued + in-flight 条数以及保守字节费用，费用含 canonicalJson 和 Record 字符串/节点开销。超限不阻塞 UI、不自动重试：`droppedCount`（入口超限加门面报告）、`externalGapCount`（其中门面报告数）、`gapObservationCount`（缺口观察次数，非去重缺失事件数）和 `gapObserved` 保留缺口。一个 run 的序号只增、不回绕，不接受重复/倒退重送；序号缺号会增加观察次数但不再次增加 droppedCount，避免重复计算门面已报告的拒绝。单条 canonicalJson 上限 512 KiB。查询队列最多 8 个（含执行中）；门面还必须保持最多 8 个“提交到收到结果”的待完成 query，防止异步结果排队越过 worker 的入口容量。

## 独立库和身份验证

固定文件 `fleet-events-v1.sqlite`，只接收显式存在、规范绝对本地目录；拒绝 UNC、目录/DB symlink、不可规范路径和异常 sidecar 类型。文件名不由 caller query 字符串拼接。路径检查不是恶意文件置换、Windows junction/硬链接、目录 ACL 或多用户访问安全的完整证明；部署仍需工程师配置私有本地目录与权限。

SQLite 身份为 `application_id=1296389702`（`0x4D455646` / MEVF），`user_version=1`。schema 为精确 3 张业务表（另外允许 SQLite 自有 `sqlite_sequence` 和两个自动唯一索引）：

- `meta`：singleton、database_epoch、retired_count、created_utc_ms。
- `runs`：run_id、started_utc_ms、last_sequence、clean_shutdown、dropped_count、discarded_count。
- `events`：row_id 自增整数、run_id 外键、sequence、三种时间、固定类别/来源、canonical_json BLOB、content_hash；`(run_id, sequence)` 唯一。

既有库先使用 worker 专属 UUID connection 的 READONLY mode 检查 application/version、sqlite_master 精确声明、table_info、unique index info、run 数据类型、meta、foreign_key_check 和 quick_check；关闭后才 RW reopen，再复核身份，随后写政策。未知现存空文件、其他表/视图/触发器/索引、损坏/不匹配 schema 都拒绝，不删库、不改名、不重建、不复位历史。READONLY 无法读取需 hot-journal recovery 的库也拒绝写入，应由工程师在离线副本上恢复；本模块没有宣称所有崩溃场景都可自动恢复。

新库由 QFile `NewOnly` 原子保留文件，设置 owner read/write 后，由一次 SQL transaction 创建 schema、owner epoch 和版本标记；失败可能留下不完整文件，下次按未知既有文件拒绝，绝不为了“自动修复”删除。SQLite connection 在 run 内建立/使用，所有 QSqlQuery/handle 作用域结束、关闭 connection 后才 removeDatabase，不复用地图缓存连接。

整个 run 在 worker 内持有固定 `fleet-events-v1.sqlite.owner.lock` 的 QLockFile，`setStaleLockTime(0)` 只关闭按年龄判 stale 的阈值，不关闭 PID/进程名等 stale 检测；`tryLock(0)` 仍可能自动移除被 Qt 判定过期的锁。不主动调用 removeStaleLockFile；正常析构会释放并删除本次 owner.lock，因此“保留数据库/旧段”不是“不移除任何锁文件”。依据目前 [Qt 官方 QLockFile 文档](https://doc.qt.io/qt-6/qlockfile.html)，目标工程的 Qt 5.15 平台行为仍需工程师核实。

锁路径拒绝 symlink、非普通文件、异常大或非规范既有锁文件。无法取得锁时明确停止，不等磁盘、不重试取锁。非阻塞 owner 锁从首次 SQL open 前保持到所有 SQL close/remove、可选封存和最终状态完成之后；根门面仍必须等待实际 finished，而不能见到 sealed 快照就抢先开启另一 writer。QLockFile 是合作本地进程锁，不是对所有 SQLite 工具、恶意外部 writer 或目录权限的强制隔离。

`journal_mode=DELETE`、`synchronous=FULL`、foreign_keys 和 busy_timeout 都读取核对；readonly/RW open 也携带同一有界 QSQLITE_BUSY_TIMEOUT connect option。依据真实 page_size 设置/读回 max_page_count。主 DB 页数受上限约束，但 rollback journal、文件系统缓存/块开销、备份和结果/队列内存另需预算，不能称整个磁盘使用的绝对上限或失电硬件保证。

## 提交、保留与缺口

Worker 交替执行有限事件 batch 和有限历史页，避免一方持续挤占另一方。每批先重新核 codec/hash/元数据一致性，使用 prepared INSERT，在一笔有限事务内先淘汰需要替换的最旧 row_id、同事务推进 retired_count，再插入新事件并更新当前 run durable 序号/已知丢弃摘要。只有实际 commit 成功后 `current.durableSequence`、`committedCount` 和 `retiredCount` 才推进；失败仅停止存储并公开 failedPendingCount，不操作飞行权限。

既有 events 超出本次 maximumEvents 政策则拒绝，不做大批隐式清理；正常每批淘汰最多 batchEventLimit 条。runs 元数据不自动删除，主库页上限最终仍可能触发停止；提供显式 clean 后封存，Worker 本身只读当前库页，跨封存段由独立[历史 Reader](FleetEventArchiveHistoryReader.md)复用同一 SQL helper。未实现自动空间救济或旧段清理；无自动 VACUUM、DB 删除、灾后队列恢复或从 JSON 恢复授权。

运行一开始提交 `clean_shutdown=0`；只有显式 drain 请求、接收入口已关闭、队列已空、所有 batch 成功后才提交 `clean_shutdown=1`，成功结果以 `cleanShutdownConfirmed` 表示。discard 停止也异步提交已知 dropped/discarded 摘要，但保持 clean=0，摘要提交失败则报告存储失败。队列丢包和已提交的序号缺口与 clean 标记独立：clean 只说明这次有序关闭，不说明原始事件无缺失。unclean 旧 run 只展示已提交历史，可能丢尾，不恢复旧 queue 或执行许可。门面外部缺口在下一笔 batch 或停止摘要事务后才进入 runs.dropped_count，未确认之前仍可能在崩溃时丢失。

## 显式闭库封存

仅在 requestStopAndSeal 被接受、有限工作已排空且真实 clean commit 成功后封存。worker 在既有 connection 上用有界 busy 的只读 `BEGIN EXCLUSIVE` 屏障，重新核对 identity/schema、实际 database_epoch 和本 run typed clean/durable/drop/discard；在该 SQL 屏障中取得文件 header、大小及 SHA-256，随后释放只读屏障，关闭 connection、销毁全部 SQL 对象并 removeDatabase。

仍持有 owner 锁时，取得 close 后 fingerprint，然后为实际 currentPath 建立一个新的专属 UUID READONLY probe connection（同一有界 busy timeout），在读事务中重新核 actual metaEpoch 和当前 run clean/durable/drop/discard 与封存预期一致；结束读事务、销毁所有查询、close/remove probe 后再取 fingerprint，必须与原 preclose/postclose 一致。这样不把旧 SQL handle 的缓存身份冒充实际候选文件的身份。只重开 currentPath，不 SQL 打开任何 sealed 目标，不恢复 hot journal。

关闭前后都限制 current 的规范本地路径、普通文件、当前政策的字节上限以及所有 `-journal`/`-wal`/`-shm` 必须不存在；残留 sidecar 不搬运、不删除。SQLite 100-byte header 必须具备正确 magic、DELETE 格式、page size/count、application_id、user_version 和有效 header change counter。hash 每次只读取最多 64 KiB chunk，全文件字节费用受当前 maximumDatabaseMiB 限制，不一次载入主 DB。close 后同样取得 fingerprint，必须与 close 前的完整 header/大小/hash 一致。

封存目标固定为同规范目录的 `fleet-events-sealed-<databaseEpoch>-<runId>.sqlite`，参数不能提供其他文件名。封存容量检查现复用 native [FleetArchiveDirectoryInventory](FleetArchiveDirectoryInventory.md)，在 worker 内逐项观察全部 entry（含隐藏、非匹配项），扫描最多 4,096，namespace 匹配上限使用本次 maximumSealedSegments（1–128）。Windows/POSIX 枚举以显式正常结束/错误通道区分结果，无 Qt iterator fallback；打开、读取、关闭、目录身份变化、不支持平台或超限等失败统一以 `archive-sealed-directory-inventory-failed` 拒绝封存，不能把部分清单当作剩余容量。旧 Qt 5 iterator 无独立错误通道的实现已替换；这不是目录原子快照或全集认证。

inventory 成功后仍要求严格 namespace：规范小写 UUID 文件名、native 与 QFileInfo 均观察为普通非 symlink 文件、canonical 路径一致，异常匹配条目拒绝；匹配条目数 `>= maximumSealedSegments` 拒绝，避免新增段突破上限。已存在目标、异常目录/路径、残留 sidecar 或文件变化也都不 rename。此接线不改变 SQL 清洁关闭、候选实际身份 probe、关闭前后 hash、合作 owner 锁、rename 后核验与不自动逆操作的要求。不覆盖、不自动删除/搬走旧段、不修改未知库、不创建替代空库来伪称成功。容量观察与实际 rename 之间仍有窗口，不约束不遵守 owner 锁的外部写入者。

QFile::rename 返回成功后，再核 current 路径已不存在、目标完整 header/大小/hash 与封存前一致，才置 `sealed=true` 并提供 `sealedFileName`（仅 basename）和 `sealedDatabaseEpoch`。`sealRequested`、`sealDecisionFinal` 只描述请求/最终窗口；`rolloverErrorCode` 是独立固定错误码，失败不将已 clean 的 DB 伪称丢包，也不改变 durable 事实。凡已尝试 rename 就保留 `sealMayHaveMoved=true`；即使返回 false 或后核失败，也不能推断 source 仍在原位置。不覆盖/删除任何一侧，不自动回滚改名，应交工程师定位现有文件。

依照目前 [Qt 官方 QFile::rename 文档](https://doc.qt.io/qt-6/qfile.html#rename)，rename 可能退为 copy/remove fallback，因此不称严格跨平台原子移动或目录 fsync/失电可靠性；该 Qt 6 说明也不替代目标 Qt 5.15 工程验证。hash 是字节变化诊断，不是签名、schema 全功能验收或恶意路径置换/外部写入防护。SQLite 屏障在 close 时必须释放，不遵守 owner 锁的外部 writer 仍可能在剩余窗口操作文件，因此需要私有目录 ACL、部署隔离和工程验证。段数上限不是整个磁盘配额：历史段可能来自不同 maximumDatabaseMiB 配置，加上 journal/备份/缓存，需要另行预算。

此 worker 不重开新 run、不恢复 live source、任务或审批。根门面只可在实际 finished、sealed 真成功并且原显式配置/同意和 source 仍有效时决定开始下一独立 run。轮换窗口不能假装连续采集；跨库 row_id/cursor 必须结合 databaseEpoch，不能混用旧页。独立历史 Reader 不取得写入/任务权限，自动满库轮换/失败重试和永久留存仍待后续开发。

## 历史页边界

历史按实际 SQLite append `row_id` 排序，不按可能回退的系统墙钟排序。请求/结果 cursor 都是规范十进制字符串。页包含 databaseEpoch、retainedFrom/ThroughRowId、retiredCount、cursorPrecedesRetainedHistory、hasMore、nextRowId、rows；每行含 codec 验证后的固定 18 字段 record、hash 和该 run 的 clean/durable/drop/discard 摘要。页同时有行数和整个紧凑 UTF-8 JSON envelope 字节上限，不能将大 blob 当成无限结果。

查询以一次读事务保持本页一致性；SQL CASE 先按实际 typeof/长度限制 blob 和文本投影，防止在 Qt driver 已分配超大损坏值之后才检查 size；不合法值映射 NULL 后拒绝，而非容错放行。SQL fetch error、非法 DB 整数/BLOB、canonical/hash/行元数据不一致均拒绝页并停止该存储实例，不能把部分 fetch 当成功 EOF。不同页之间发生保留淘汰时，边界/缺口明确显示，不许承诺整个分页扫描的永久快照。rows 只限读取，不回灌 live BlackBox、TimelineReplay、Registry、任务、审批或 authority；`historicalOnly=true`，`permissionsRestored=false`、`authenticated=false`、`signatureVerified=false`、`executable=false`、`completionVerified=false`。

共享 readPage 自己 BEGIN/COMMIT，页内重新核实际 schema/epoch，不能由调用者套未结束的外层事务或传 skipIdentity 布尔绕过。Worker 不传取消指针，保持原流程；独立 Reader 可传 atomic 合作取消，但不打断 SQL statement。共享 inspect 给 Reader 的实际 range 仅用于其读视图，Worker 初始化移除这些键，避免启动缓存范围冒充持续 current 数值。

## 本阶段证明范围

仅进行了源码实现、限定审阅和文本静态核对。未启动 worker、未创建数据库、未执行 native inventory/SQL/QLockFile/hash/rename、未编译、未运行测试/重放/应用、未访问设备、未刷机或放行。工程师需验证 native 枚举的正常 EOF、中途错误/关闭失败、大小写异常/目录/symlink 拒绝、全 entry cap、段数为 cap-1/cap/cap+1 的容量边界；还需验证 QtSql/SQLite 插件部署、实际 pragma 行为、锁库/磁盘满/热日志/崩溃/换路径、有界页/队列、长时 owner 锁、封存取消/rename fallback、header/hash、实际 finished 后重开及线程结束销毁。上述待验证项不应被当前文本检查替代。

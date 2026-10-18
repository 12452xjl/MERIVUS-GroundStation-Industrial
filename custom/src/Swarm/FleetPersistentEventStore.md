# FleetPersistentEventStore：默认关闭的连续诊断归档

## 546 A4：完整容量计划与顺序移动／备份

新增 `maintenanceCapacityPlan/maintenanceCapacityStatus/maintenanceCapacityChanged` 和 `requestLocalArchiveCapacity(intent, confirmed)`；Root 原 `persistentEventArchive` 仅增加 `capacityPlan/capacityStatus/capacity`。这是现有 MaintenanceWorker 的顺序编排，不是新服务、自动清理定时器、永久删除或空间释放认证。要求录制、Reader、原维护及清理均实际空闲；接纳 commit 后持有原 shared maintenance 门，直到本计划最后实际 worker 结束，不在步骤间借空槽开始捕获／其他维护。

Intent **exact7**：`action/expectedRunId/inventoryId/planId/maximumSealedCount/maximumSealedMiB/backupDirectory`。action 为 `prepare/commit/cancel/discard`，confirmed 必须 true；前四字段及备份目录为非 null QString。prepare 只用本 Store 私有原 inventoryId，planId 空，数量为 actual 整数0–32、逻辑 MiB 为0–131072，目录为旧备份流程的本地绝对目录语法；空白／字符串数字／Bool 不变成0。其余动作仅原 planId 非空，inventoryId/目录空、两数值为-1。回执沿原 **exact9** `schemaVersion/action/accepted/requestId/reason/requestOnly/historicalOnly/permissionsRestored/flightCommandReleased`，requestId 是 planId，不是某步线程 ID。accepted 不等于移动、备份或编排完成。

prepare 不读盘，冻结原全量 descriptor7 清单、目录原生身份、run、source binding 和原 BlackBox QPointer（原有来源销毁后即失效；原来无来源的历史维护不要求飞机）。按 sealed 的文件 mtime 降序、basename 升序保留连续前缀，同时满足数量和逻辑字节目标；不能装入的首项及之后全列入移动计划，不做背包挑选。mtime 不是任务结束时刻；0上限是明确全部移出。最多128个 sealed；现有 retired 加计划数量超过128则整份拒绝，须先另走525人工处理，没有自动删除退路。

Plan **exact24**：`schemaVersion/planId/runId/inventoryId/sourceDirectory/directoryIdentity/backupDirectory/maximumSealedCount/maximumSealedMiB/originalSealedCount/originalSealedBytes/plannedSealedCount/plannedSealedBytes/retireCount/requiredBackupBytes/rows/expiresAfterMs/canCommit/status/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。bytes 为规范十进制字符串；rows≤128，row **exact6** `index/source/retiredBasename/backupBasename/retireOperationId/backupOperationId`，source 为原 sealed descriptor7，两个 operationId 独立原生 UUID。status 为 prepared/expired/retired/consumed/discarded。30秒单调期限仅限制新 commit；PreciseTimer 只通知，不延长原期限，已提交计划不在30秒后被当作失败。

commit 一次消费原计划。每项依次 **新 inventory → 原 retire → 新 backup/readback**，全部步骤后再新 inventory；每个 inventory 的目录身份和全清单必须与私有 expectedEntries 相同。确认改名后仅将对应条目的 kind/basename 换为 retired，保原 bytes/hash/mtime；全清单仍按 sealed 再 retired、各 basename 排序。源改变、意外文件、未知结果、未确认改名或未核实副本都停止，不提交有效前缀为完整成功。复用 worker 自身的目录锁、源／目标检查、复制和读回，不把两次 worker 之间的观察当跨进程原子事务。

每步原 worker、jobId、planId 和请求固定；只有同实际 result/failure 与 `isFinished()==true` 的 finished 双尾齐备才释放该线程槽和推进。缺结果为未知失败，不补未改名／未写。cancel 只请求原计划停止后续步骤，准确合作取消本计划原 worker；必须等原实际终态，不抹已发生的移动、部分副本或读回事实。原 source 消失后仍收取在途 worker 的事实，但不再开始下一步；父销毁只请求取消、不 wait／delete 运行线程。

Status **exact20**：`schemaVersion/planId/runId/busy/stage/phase/jobId/currentIndex/totalCount/completedCount/cancelRequested/ok/error/lastWorkerResult/results/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。stage 为 idle/prepared/running/cancelling/complete/failed/cancelled，phase 为空或 inventory/retire/backup。lastWorkerResult 为完整原19/27/31诊断或空；results≤128，row **exact4** `index/source/move/backup`，不累计全部大树。move 为空未知或 **exact5** `known/moveConfirmed/mayHaveMoved/renameReportedSuccess/errorCode`；backup 为空未知或 **exact6** `known/backupVerified/mayHaveWritten/copyCompleted/bytesCopied/errorCode`。known=true 仅表示有原结果，并非所有事实真。completedCount 仅计已确认移动且已读回核实的项，ok=true 还要求最后全清单匹配。不存在原结果时未知不补false。

本计划每步先退休525 private basis，且不经普通备份的 basis 安装路径；**不会创建或继承永久删除资格**。若要删除，须另做当前 retired 原件的新显式备份，再走525独立预览和两项确认。移动不释放磁盘，副本反而增加占用；本计划不测 free space，不删除 current、sealed、retired 或副本，不恢复历史任务、账号、批准、权限或飞行命令。上述合同仅完成源码与静态审阅，未运行、编译或测试。

## 538：持久化任务索引与原共享读取槽

新增 `historyTaskIndex/historyTaskIndexChanged` 与窄 `requestLocalArchiveTaskIndex(intent, confirmed)`。公开 intent 恰七个非 null 字符串：`action/expectedRunId/scope/previousQueryId/indexId/taskIdHash/afterTask`。动作仅 create、open、resume、page、records、continue-records、rebuild；create/open/rebuild 不借旧 query/index，其他动作必须是本 Store 最近实际接纳的原生索引结果。页面不能输入文件路径、checkpoint hash 或原始游标。

实际文件读写由 [TaskIndex](FleetArchiveTaskIndex.md) 在原 [HistoryReader](FleetEventArchiveHistoryReader.md) 线程内执行，`kind=task-index` 复用已有读取槽、准确 query 取消和 actual finished 退出。要求录制、维护、清理和其他读取均已退出；没有第二套并行文件工作线程。原历史目录的 35/31 字段 DTO 和旧 8192/256 预算不变，新索引是独立持久工作区，不扩旧协议。

Worker exact16 只在实际 query/run/source binding 仍匹配且完整 matcher 接纳后发布；Store 添加原 `queryId/runId`，公开为 exact18。独立私有上下文保存原 request/result/hash，另保存同 index/hash/scope/run/binding 的最近实际 64 项任务页。下钻记录不抹原页，可继续选择同页另一任务或下一页；续记录只能用最近实际 records 的原 cursor。任何失败／取消均不能恢复已消费的上下文，须显式 open；更换／销毁 source、开始新录制或维护会使原资格失效。旧 UI 副本只能作历史阅读。

create/open/resume 不自动读任务页；用户明确 page 后才读取首页或下一页。rebuild 是独立二次确认动作：允许重启后没有旧 UI 上下文时重建固定派生文件，后台仍须读取合法旧 checkpoint、冻结完整文件基线并在原子替换前复核；不是任意覆盖或删除原归档。取消请求不是回滚，checkpoint 可能已提交，重新打开才能确认当前磁盘结果。

仅统计保留事件的任务哈希和行数，不还原真实任务 ID、正式任务结果、控制权或飞行授权。完整源码、容量限制及恢复边界见 [TaskIndex 合同](FleetArchiveTaskIndex.md) 和 [界面合同](FleetArchiveTaskIndexPanel.md)。未编译、运行、测试、仿真或修改 PX4；静态核对不等于工程验收。

## 525：原生已退休段永久清理与单槽事实交接

新增独立只读 `retiredPurgeState/retiredPurgePreview/retiredPurgeResult`、共同 `retiredPurgeChanged`，以及 `requestRetiredPurge(actualController,intent,confirmed)`。只接受当前 Store 实际 parent/controller 的原对象、同线程和明确确认；传入指针仅与原对象比较，不解引用任意 caller。Root 仅提供窄 `retiredArchivePurge`（及历史入口 `archive.retiredPurge`）门面，不把旧维护 intent6/7、预览、库存或公开结果扩成删除授权。旧维护 request/result/current 形状与动作保持；新动作仍复用原实际 MaintenanceWorker 单槽，不建立平行文件／SQL架构。

这是**明确永久删除一个已退休 SQLite 段**，不是原 retire 改名、不是任务恢复，也不是自动容量政策。只允许原完整读回核验备份对应的 `kind=retired` 源；current/sealed、缺少备份、未核验备份、目录清单或导入记录不能创建清理来源。GUI 不枚举、hash、开 SQL、删文件或计算实际释放空间；真正路径／文件身份、锁、hash、收据和删除由 [MaintenanceWorker](FleetEventArchiveMaintenanceWorker.md) 执行。当前仅 Windows 原文件句柄删除可用；其他平台 state.stage=unsupported，仍可诊断 basisReady，但 preview 拒绝，不影响原备份功能。确认不是身份认证或操作系统权限保证。

### 只有实际双终结备份能建立 private basis

在原 backup Worker 中独立捕获完整实际 **backup31**，必须原 kind=backup、source.kind=retired、backupVerified=true，且原 `resultMatches31` 全包通过；再等同一实际 Worker 的 resultReady **及 actual finished**。实际 Worker／原 query、request、固定目录、Store run、原 source binding 和不回绕 native generation 全部对应才一次安装 private basis 与全新 basis UUID。旧 maintenanceResult 的通知顺序不变，新 basis 不从 public last/result、diag、catalog、拷贝 JSON 或 caller map 重建；原 backup31 并没有 Windows file ID，不能倒推其曾认证文件对象。

resultReady 与 finished 任一先到均独立保留；旧 entry guard 尚未退栈时只存一个原冻结延期闭包，由同一原 entry guard 释放后核对安装，不开 retry／spin 工单。新普通维护请求、实际 source 替换／销毁、新 capture 或 archive Reader 启动先退休 basis、proof 及原 generation，晚旧 backup 不能复活。同路径／同值／同指针地址也不能恢复旧 UUID／代次。private basis 与旧 inventory/diagnostics 完全独立，原 `retireMaintenanceValues` 清旧值不会伪装两种来源同时存活。

### 闭合19/18/13/11与 intent5

State **exact19**：`schemaVersion=1/runId/basisId/basisReady/source/sourceDirectory/backupDirectory/backupBasename/busy/jobId/action/stage/cancelRequested/previewId/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。source 为原 descriptor7 或空 map；缺少来源不是零字节证明。busy 包含 shared maintenance／capture／Reader／pending 和 native entry 占用，jobId/action 却只属于最近本清理工单，不能拿 shared busy 推断本页持有其他工单。stage 只可 `idle/basis-ready/running/cancelling/prepared/expired/discarded/complete/failed/cancelled/unsupported/exhausted`；action 初始空，之后只保原业务 preview/commit。basis 可被消费而仍保原 source 供诊断，basisReady=false 不否认历史删除事实。

Intent **exact5**，全部非 null QString：`action/expectedBasisId/expectedJobId/previewId/expectedRunId`。所有动作必须精确匹配当刻 native state 的 basisId/jobId/runId；preview/cancel 的 previewId 为空，commit/discard 必须原 preview UUID。confirmed=true。独立 receipt **exact11**：`schemaVersion/action/accepted/jobId/reason/requestOnly/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`；requestOnly、historicalOnly=true，末四权限标志=false。accepted 仅是请求接纳，不能证明预览、文件删除、空间释放或收据写完。

| action | 只消费实际原生来源 |
| --- | --- |
| preview | 只从当前 private verified-retired basis 生成 worker request3 `kind=purge-preview/basis/operationId`。Worker 只读核源／备份并关闭原两个句柄，成功后才由原 native 双终结生成新 preview UUID 与30秒时钟；失败／取消可另行明确 preview，但不能复活旧 proof。 |
| commit | 只消费同 basis／原 preview、原 proof 与未过期单调时钟，生成 request4：request3 的 kind 改 purge-retired，另加 proof3。接纳时一次消费 basisReady/proof，失败或取消都不恢复破坏性意图；须重新真实备份才能再建 basis。 |
| discard | 同步只清原 preview 的 proof／期限，发布 discarded/canCommit=false，保留未消费 basis；过期 preview 也可明确废弃。不是撤销文件删除。 |
| cancel | 同步只请求当前原清理 busy job 合作取消，不提前释放槽，不抹决定后真实事实，也不假称已完成取消。旧维护 cancel 不能拿清理 jobId 绕过本入口。 |

Preview 初始 `{}`，有值 **exact18**：`schemaVersion=1/runId/basisId/previewId/expiresAfterMs=30000/status/canCommit/source/sourceDirectory/backupDirectory/backupBasename/sourceFileIdentity/backupFileIdentity/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。status 只可 `prepared/expired/discarded/consumed/retired`。proof3 保在 native private：`sourceFileIdentity/backupFileIdentity/backupDescriptor8`；两个实际 Windows 文件 ID 只能本次只读 preview 取得。公共18不能创建它，且不能用空 map／“没报错误”代替真实 proof。

QElapsedTimer 从双终结实际成功安装 preview 时开始，原 UUID timer、getter、commit 均重核原30000ms；早醒只重排原剩余时间，不续期。新请求、换源或清旧值不能借同值重基。30秒仅限制 native 接纳提交，不承诺 hash、SQL、收据或删除在30秒完成。每次 preview/commit 的 worker operationId **等于本次外层 jobId**（新 UUID），不同于旧 backup operationId，也不同于 previewId。

Result 初始 `{}`，有值 **exact13**：`schemaVersion=1/runId/basisId/jobId/action/ok/error/workerResult/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`；action 仍为原 preview/commit。只有完整 matcher 通过才保原 workerResult41；未知／缺失包保空 map与明确未确认，不把缺字段当 false 的删除保证。末五标志为 true/false/false/false/false。公开 state/result 永远仅诊断，不重建来源或工单持有权。

### 真共享槽、取消、原始事实与收据

新 Worker 继续放 `_maintenance` 原槽，另保存实际 equality-only 原指针、job/nonce/不回绕 serial 与原 controller/run/binding/generation。每次 native 入口及共享槽尾核均阻止新 capture、rollover、Reader/history、旧维护、工程 record 或 source 替换与其并行；即使实际 Worker QObject 先销毁造成 QPointer 清空，own busy 也保留到原 resultReady/failure **及 finished 双终结**。没有 GUI wait/terminate、finished 提前放行或公开取消代替 actual finished。`observeWorker` 不把新 current24 写进旧 maintenance snapshot，旧形状不漂移。

constructor/connect 后、真正 start 前再次核原 controller/source fence、原 private basis 与完整空闲证据；失效只把原复制 Worker 作为 cancelled 启动交回事实，仍保原槽至双终结。preview 终态还核原 source、原 basis 与未取消，才安装 proof；已退休／已取消的原成功只读结果不生成删除资格。源失效、界面换源或晚 cancel 不抹历史 result41，也不会从结果恢复 basis。

Worker41 保 common12 的否认保证，并独立保 `decisionFinal/deleteAttempted/deleteReportedSuccess/mayHaveDeleted/sourceAbsentObserved/backupAfterVerified/sourceHandleClosed/backupHandleClosed/cancelObserved` 及 receipt7：`receiptBasename/receiptCreateAttempted/receiptCreated/intentWritten/resultWritten/receiptClosed/receiptError`。commit 只在固定源目录 NewOnly 创建 `fleet-events-purge-<jobId>.jsonl`，完整意图行写入并 flush 成功后才允许原句柄删除。终态行／close 失败可能发生在删除之后；必须保实际删除事实，不能用 ok=false／previewVerified=false／取消文案把它改成“没删除”。本 Store 的 commit.ok 额外要求源缺失观察、备份后核、两个句柄关闭以及意图／终态完整写入与收据关闭；不是掉电持久化或物理空间证明。

`sourceLogicalBytes` 只是原文件逻辑长度字符串；`diskSpaceReclaimedVerified` 始终 false，不累计成真正磁盘已释放容量。这里不做自动批量删除、覆盖、回滚、失败重试、清未知文件或恢复任务／账号／权限，不解除起飞门禁，也不修改 PX4／发控制命令。退出／销毁仅对原 Worker 合作取消；parentless Worker 只有复制值，结果不自动恢复授权。

本批只进行源码修改与限定静态文本审查，未执行文件枚举／备份／删除／收据、helper／parser／SQL／UI、编译、测试或刷机。工程师仍须验证原 backup31／intent6/7 兼容、两种终结顺序及重入／销毁、当前／sealed 拒收、源／目标换代和同值ABA、30秒／取消／决定后晚取消、NewOnly／短写／flush／close／终态行失败事实、实际句柄删除／源缺失／备份后核与无并行入口；这些场景均未执行，源码静态收口不能替代真实环境验收。

## 484：独立性能声明摘要与本地搜索政策

recordLocalEngineeringReview接受approve-performance，经原Codec固定投影后记录独立 `fleet.performance.declaration.result`；本地search的闭合operation政策同步新增该操作。原enable／recording／busy／实际Reader／Worker及目录、分页和取消合同不变；新增审批不会自动开启归档或回录旧事件。

摘要仍exact21，不保存两项性能数字、原账号、自由理由、完整配置或凭据；实际原14 receipt、同operation/action及原比较摘要照常核对。memory accepted不是SQL持久化或防篡改证明，未接纳／失败不否认Registry已提交，不恢复批准或自动重试。现有原始事件／维护能力不扩大。未执行SQL／录制／文件／业务、编译或测试，32工程场景全部未执行。下方为历史。

## 466 原生可读诊断条目的历史导入

本批实际消费465可读条目的原生来源，新增 `prepare-import/commit-import/discard-import`。没有新公共属性：共享原 `maintenancePreview/maintenanceResult`、维护槽和实际finished生命周期。目标只固定本机 `_localArchiveDirectory`，先复制到新stage，完整读回后由后台非覆盖发布为retired。**这只是历史文件导入，不恢复任务、账号、身份、权限、审批或执行。** imported文件仍不进入普通sealed检索；之后必须另走原 inventory→prepare-restore→独立确认，才能恢复为普通历史段。上述“恢复”仅指历史检索，不是飞行任务恢复。

### 实际 private 诊断来源及 exact7

只有同实际Worker、原query/run、完整465 matcher成功的diag23回调，才原子建立私有 `diagnosticsId/runId/sourceDirectory/sourceDirectoryIdentity` 和 `entryId→backup descriptor8`。每个公开诊断行仍生成自己的native entryId，但只有原status=readable的完整descriptor入私有可选表；rejected行的ID不能准备导入。不从public26、复制JSON、UI字段、旧catalog24、caller文件名或任意路径重建该表；该私有状态在本批被真实prepare-import消费，不是未来占位授权。

三个新动作均为 **intent exact7**：原 `action/expectedRunId/inventoryId/entryId/previewId/queryId` 加 `diagnosticsId`，全部非null QString。inventoryId永远为空，diagnosticsId和entryId必须命中同一当前私有可读条目，expectedRunId仍为当前run，confirmed=true。prepare-import的previewId/queryId为空；commit-import/discard-import的previewId必须原生原预览，queryId为空。新动作不接受backupDirectory或目标路径；旧六字段动作及三个携原raw目录的七字段动作保持原契约，不宽松混收多余字段。

导入还要求固定本机目录字符串非空且有效、全部原writer／Reader／maintenance／rollover／capture／history pending槽和local busy空闲；不需要BlackBox或飞机。分支位于旧actual inventory检查之前，从私有诊断选源而不是伪装源段库存。GUI准备仅复制值，不检查文件／目录存在、容量、锁、SQL、hash或复制；真正源文件与两个目录的当前身份和全部操作由 [MaintenanceWorker](FleetEventArchiveMaintenanceWorker.md) 执行。只读诊断的可读标签不是永久可信来源，也不是目标存在／可写的保证。

### 独立原预览29与提交

新预览 **exact29**＝共同12＋业务17：`format="merivus-fleet-archive-import-preview"/schemaVersion=1/action="import"/previewId/diagnosticsId/entryId/runId/source/sourceDirectory/sourceDirectoryIdentity/destinationDirectory/stagingBasename/targetBasename/expiresAfterMs/status/canCommit/historicalImportOnly`。source为原backup8，sourceDirectory/ID来自原实际诊断，destinationDirectory只能是Store固定本机目标；historicalImportOnly=true，expiresAfterMs=Int30000，status=prepared、canCommit=true。其共同12继续否认认证／签名／权限／执行与跨段原子快照保证。

原previewId同时是唯一operationId：stage固定 `fleet-events-backup-<epoch>-<sealRunId>-<previewId>.sqlite`，发布目标固定 `fleet-events-retired-<epoch>-<sealRunId>.sqlite`，完整路径应在确认前展示。原backupOperationId只描述选中的源备份名，不能替换新的stage操作ID。Store私有预览同时冻结原diagId、entry、backup8、源目录对象身份、固定目标、两个basename和原run，public map不重建它。沿同一原生单调30秒计时器与身份timer；更改UI路径、不读时钟、旧通知或同值ABA都不能重基或续期。旧schema1 move25和schema2 backup26不改变，原timer仅按对应previewId作用。

commit-import再次核同私有diag／entry、原source与两个目录、原previewId/run及原单调期限后，只从私有值生成 **worker exact5**：`kind="import"/source(backup8)/sourceDirectory/expectedDirectoryIdentity/operationId`，其中expectedDirectoryIdentity为原来源目录ID，operationId为原previewId。worker constructor接原固定目标，不接caller目标。启动前完整复制request，一次退休原diag私有/公共、inventory、preview及全部history continuation；失败／取消不恢复旧ID。discard-import只退休同原未消费预览，发布discarded／canCommit=false，不取消或回滚已发工单。

### 完整38事实、原固定目标与最终线程

外层 `maintenanceResult` 仍exact20，action=import，完整已核 **worker38** 原样保存在workerResult；status优先级为 `import-verified`（importVerified）、`import-uncertain`（mayHavePublished）、`import-incomplete`（mayHaveWritten）、否则 `not-imported`。只有缺少完整合法事实才failed，不因某个复制／发布失败丢掉完整不确定性。

worker38＝共同12＋业务26：`format="merivus-fleet-archive-backup-import"/schemaVersion=1/kind/operationId/source/sourceDirectory/expectedDirectoryIdentity/observedDirectoryIdentity/destinationDirectory/destinationDirectoryIdentity/stagingBasename/targetBasename/targetDescriptor/decisionFinal/stagingCreateAttempted/stagingCreated/mayHaveWritten/bytesCopied/copyCompleted/stagingVerified/publishAttempted/publishReportedSuccess/mayHavePublished/importVerified/cancelObserved/errorCode`。targetDescriptor仅验证成功时为原retired descriptor7，否则空map；bytesCopied仍规范十进制QString。stage创建、实际写入、完整复制、stage读回、发布调用返回值、可能发布和最终核实互为独立事实，不能用accepted或取消回执代替。

纯值worker matcher不知道constructor目标，所以Store额外冻结原 `invocationDirectory`，用它构造实际worker并capture至回调；import结果的destinationDirectory必须严格等于该原目标，之后才接纳完整事实，不能用public DTO、request或回调时的新路径替换。只接受同实际QObject／原query／原run，resultReady／失败／取消都不清槽；actual QThread::finished才释放并保存最终snapshot。

import的current为 **exact27**＝共同12＋ `requestId/state/errorCode/cancelRequested/decisionFinal/stagingCreateAttempted/stagingCreated/mayHaveWritten/bytesCopied/copyCompleted/stagingVerified/publishAttempted/publishReportedSuccess/mayHavePublished/importVerified`；旧只读19／备份23不扩字段。后台先非覆盖创建stage，完整核验后native no-replace发布retired；未完成stage保留但不进入普通历史，不自动删除、覆盖、重试、回滚、创建当前DB或mkdir。发布后取消不能抹除可能已发布事实；后续必须重新inventory，不借旧私有diag继续。retireMaintenanceValues与任何begin都清新私有上下文，原各公开值通知／QPointer寿命门禁沿用；没有把导入结果回灌live任务、Registry或授权。

本批仅源码与限定静态文本核对，未编译、运行helper／parser／SQL／线程／UI、测试、备份导入恢复、刷机、提交或打包。工程师待实操：实际diag readable与rejected选择隔离、public伪造／旧run／旧ID；新exact7与旧契约、固定目标原capture；29预览完整路径与原30秒、同值ABA、丢弃与消费；源／目标换代、已存在stage/retired/对应sealed与容量，锁与sidecar；短写、flush/close、完整stage读回、发布前后取消和native无覆盖；38/27全事实与actualfinished、销毁和所有私有状态退休；retired导入不立即进入普通历史、不恢复任务或许可。上述均未执行。

## 465 独立备份逐文件只读诊断

新增只读 `maintenanceBackupDiagnostics`／`maintenanceBackupDiagnosticsChanged`，明确调用 `backup-diagnostics` 才在后台观察指定目录的各备份候选，分别报告 readable／rejected，而不是把某个失败文件隐藏后冒充464严格清单成功。**rejected 只表示本次未能完成该对象的规定检查，不证实文件损坏、不可恢复或可以删除。** readable也只是当前文件自洽，不证明原源对应、原始事件真实性、备份全集或任务／权限恢复。没有恢复、自动清理、修复、隔离、移动、重试或复制动作，旧 strict backup-catalog21／public24及copy／move语义不变。

### 显式范围与原线程槽

`action="backup-diagnostics"` 使用 **intent7**：原六字段加 `backupDirectory`，均非 null QString；四个 inventoryId／entryId／previewId／queryId 必须为空，expectedRunId仍等于当前Store run，confirmed=true。与prepare-backup和backup-catalog共用原路径纯词法政策，不trim／normalize、不借新路径延长预览、不在GUI查询目录／文件／SQL／hash；其余旧动作继续exact6。

独立分支在原source路径和actual inventory要求之前执行，不要求原库、BlackBox、飞机或账号。冻结原raw路径后native生成query UUID，worker **exact2** 为 `kind="backup-diagnostics"/destinationDirectory`。仍须原writer／Reader／maintenance实际对象、rollover／capture、全部pending与native local busy空闲。发起前退休原maintenance值／预览与全部历史continuation，但不会因此改变文件。独立cancel沿原exact6与原实际query，原库缺失也可请求；result／failure／cancel都不提前释放槽，实际finished才退槽并保留只读current19。所有真实目录、锁、文件和SQL IO都只在实际 [MaintenanceWorker](FleetEventArchiveMaintenanceWorker.md) 中进行。

### 完整观察结果23与public26

Worker **exact23**＝共同12＋业务11：`format="merivus-fleet-archive-backup-diagnostics"/schemaVersion=1/destinationDirectory/directoryIdentity/entries/candidateCount/readableCount/rejectedCount/enumerationCompleted/originalSourceVerified/backupSetCompletenessVerified`。candidateCount／readableCount／rejectedCount均严格Qt Int0–128，`candidateCount=entries.size=readableCount+rejectedCount`，enumerationCompleted=true，后两验证标志必须false。共同12仍否认身份、签名、权限、执行、全集记录哈希与跨段原子快照保证。

每个原row为 **exact4**：`basename/status/errorCode/descriptor`。basename为实际观察到的候选名，最长512字符，整个列表按名字严格升序且无重复；status只可readable或rejected。readable使用原完整backup descriptor8（含规范第三UUID backupOperationId），errorCode为空；rejected必须descriptor={}且errorCode为规定固定原因，不把部分descriptor、异常正文、SQL错误、源路径猜测或有效前缀塞进失败行。某些坏名、类型、旁路文件、权限／锁或变化也可导致拒收，不能把统一rejected理解为确定损坏诊断。

Store只在同实际Worker／原query／原run／原exact2完整 `resultMatches` 成功后，才一次性从原23生成 **public exact26**：新增native `diagnosticsId/queryId/runId`，每个原row4只追加native UUID键 **entryId**，变成row5，原descriptor不改写。即使rejected也可以有纯诊断ID，不是文件恢复句柄或批准；没有未来导入占位私有context，也没有从public DTO重建权限或读取任意文件的消费者。`maintenanceResult.workerResult` 保留完整原23，不反填公共ID。

外层result仍exact20，action=backup-diagnostics，成功status为 `backup-diagnostics-complete`，errorCode为空。**成功允许rejectedCount>0**：complete仅说明完整接纳这次有界候选观察，并非所有候选可读。候选名单枚举、目录对象或最终整包检查失败／取消仍只发布原failed和空workerResult，不发布已观察前缀为成功；单个受规定检查拒收的行在完整最终23中明确保留，所以不会污染或冒充464严格catalog。空候选也不证明其他文件、目录或全部历史为空。

任何新维护请求、copy／move提交或新录制begin都会退休本DTO；retireMaintenanceValues、notifyArchiveViews、实际结果通知及begin异步清值已接独立signal，所有多通知继续QPointer尾核。UI应按原request/run/path/来源和native值代次，在actualfinished之后才接纳本页结果；改路径后的原终态可作历史阅读，不能改回同值复活当前资格。关闭界面不自动读盘或把其他入口诊断当成本次成功。

本批只改源码并限定静态文本核对，未编译、运行helper／parser／SQL／UI、观察实际目录、测试、备份或恢复。工程师待实操：源失效独立请求及取消；旧6／新7与四空ID／原run；全readable、全rejected、混合及零候选计数；512字／128候选／目录容量边界、坏名／类型／symlink／sidecar／不可读／变化与固定原因；descriptor8／空map、严格升序和全包不接受前缀；目录与文件换代、实际finished/重入/销毁、路径同值ABA、新录制清DTO；诊断成功不升级strictcatalog或恢复授权，也不自动删除任何rejected对象。上述均未执行。

## 464 独立外部备份目录核验清单

新增只读 `maintenanceBackupCatalog`／`maintenanceBackupCatalogChanged`，复用原维护入口、固定本产品政策、actual Worker槽与取消流程。明确请求后后台枚举一个指定目录中的备份文件，并核各文件自身的 SQLite／封存元数据与读前后 SHA256 一致性。**这是文件自洽检查，不是与原源对应的证明，不是完整备份集合证明，也不是任务、数据或权限恢复。** 不要求原归档库、原 source BlackBox、飞机或账号存在；重启后原归档目录不可用也可查看独立外部备份。不会顺带创建或打开原库，不会把备份改名进普通归档、回灌 BlackBox、替换任务或补发命令。

### 显式 exact7 与来源独立性

新 `action="backup-catalog"` 与 prepare-backup 一样独占 **intent7**：原六个非 null QString 加 `backupDirectory`。inventoryId／entryId／previewId／queryId 四项必须为空；expectedRunId 必须仍等于当前 Store runId（新实例为空），confirmed=true。路径仍为原样明确输入，遵循463相同长度、Unicode、NUL、正斜线本机绝对clean、非根／UNC／设备／Windows ADS 纯词法政策；GUI不归一、不枚举、不查询路径是否存在、不计算hash或开SQL。其余旧动作仍exact6，旧两类预览及copy／move契约均不变。

同一 native动作 busy、实际 writer／Reader／maintenance、rollover／capture和全部历史pending槽必须空闲。backup-catalog分支位于原固定归档目录可用性及实际 inventory/source descriptor 要求之前，只从本次冻结 raw backupDirectory 生成 **worker exact2**：`kind="backup-catalog"/destinationDirectory`。原库缺失不成为隐藏门槛，也不从旧维护库存借文件或权限。生成新 native query UUID，启动前退休原 maintenance各值／预览和所有历史 continuation；这些只是旧来源与游标退休，不改变任何文件。

取消仍是原exact6／原实际query，不再依赖固定归档路径可用性；它必须能取消一个来源已失效情况下仍在进行的独立清单。cancel／resultReady／failure均不提前释放槽，actual finished才退槽。Store保留该 Worker最终current19快照（movefacts false）；清单不涉及复制或改名，不能用这些false去推断其他既有文件不存在。所有路径、SQL、hash、目录对象身份和协作锁IO仍只在实际 [MaintenanceWorker](FleetEventArchiveMaintenanceWorker.md) 后台进行。

### 完整实际结果与 public24

实际 Worker返回 **exact21**＝共同12＋业务9：`format="merivus-fleet-archive-backup-catalog"/schemaVersion=1/destinationDirectory/directoryIdentity/entries/fileCount/enumerationCompleted/originalSourceVerified/backupSetCompletenessVerified`。enumerationCompleted须true，originalSourceVerified和backupSetCompletenessVerified须false；fileCount为Qt Int0–128。每个 entry为 **descriptor8**：原 `basename/kind/databaseEpoch/sealRunId/fileBytes/fileFingerprint/modifiedUtcMs` 加 `backupOperationId`，kind固定backup，第三UUID严格等于 `fleet-events-backup-<epoch>-<sealRunId>-<backupOperationId>.sqlite` 中的原操作ID。

Store只接受同实际Worker、原query、原run及原exact2请求的完整 matcher通过结果，再一次性从该真实包生成 **public exact24**：原21＋native `catalogId/queryId/runId`。每个真实descriptor8只追加native UUID键 **entryId**，成为9字段；原Worker21在 `maintenanceResult.workerResult` 中完整保留，不向它反填公共ID。新公共ID只标识这次展示来源，不是恢复许可；没有从public map重建来源／路径的入口，也不增加未使用的“未来导入”私有库存或授权状态。

外层result仍exact20，action为backup-catalog，成功status为 `backup-catalog-complete`，errorCode为空；失败沿原failed语义，不提交合法前缀，不生成“部分已验证清单”。只接纳实际枚举的上限内完整集合；空目录清单也只说明该次固定命名空间观察为空，不证明其他目录或所有历史不存在。目标目录变化、文件损坏／变化、sidecar、锁、容量或取消均可能拒绝，不能因此自动修复或重试。完整枚举也不认证作者、backupOperationId来源，不证明备份来自当前原库或集合覆盖全部任务；共享共同12否认身份／签名／授权／执行与跨段原子快照保证。

retireMaintenanceValues及任何新录制begin清新catalog DTO；new inventory、copy、move或另一次catalog请求也退休它，旧public副本仅可历史展示，旧ID不能同值ABA复活。notifyArchiveViews、实际结果和begin异步清值通知均加入独立catalog signal，多信号尾部继续QPointer核同一Store寿命。UI只应在原回执query/run/path仍一致、完整原worker21核验且actualfinished后接纳为本页本次清单；关闭或改输入不自动扫描、复制、导入或取消其他入口工单。

本批只改源码并限定静态文本核对，未编译、运行helper／parser／SQL／UI、扫描实际目录、测试、备份或恢复。工程师待实操：无原库／空AppData独立读取与取消；旧6／新7和四空ID／原run；空目录／128与129文件／4096目录项／坏名／坏库／partial／sidecar整批拒收；全部descriptor类型、三UUID、SHA／SQL和目录换代；原query／path／UI代次与迟到结果；actualfinished互斥、销毁、多notify及begin清值；自洽与原源真实性／集合完整性／授权恢复否认。以上均未执行。

## 463 原单文件备份及后台读回核验

复用462同一维护槽、三属性、exact9 request-only 回执、原单调30秒预览与 actual-finished 退槽；不新增并行 Worker 或任意来源读取接口。原 inventory／retire／restore 全部请求、schema1预览25字段、worker19快照、move27事实与32段恢复政策保持不变。唯一扩展是明确选中原库存中一个 sealed 或 retired 文件，将其复制到另一个用户明确指定的本机既有目录，并在后台读回核验；不是重新导入任务、从备份目录启动应用、恢复许可或自动恢复数据。

### 唯一 intent7 与原路径绑定

仅 `action="prepare-backup"` 接受 exact7：原 `action/expectedRunId/inventoryId/entryId/previewId/queryId` 加 `backupDirectory`。所有字段为非 null QString；原 run／库存／entry 必须一致，previewId 和 queryId 为空，confirmed=true。其他所有旧动作，包括 backup 的 `commit/discard-preview/cancel`，仍只接受原 exact6；带第7字段（即使为空）也拒绝，不能在 commit 临时换目录。

GUI准备只做原字符串词法检查：1–4096字符、trim不变、无NUL或失配UTF-16 surrogate、正斜线本机绝对clean路径、非根目录、非UNC／设备命名空间；Windows还要求明确盘符且无额外冒号ADS。路径不自动规范化或改写，也不在准备阶段调用 QFileInfo／exists／canonicalFilePath／mkpath／hash／SQL。原明确输入完整冻结到私有 context 和公开预览；实际存在性、目录对象身份、可读写、别名与目标不存在等由后台 [MaintenanceWorker](FleetEventArchiveMaintenanceWorker.md)重核。词法接纳不保证实际文件操作可用，失败不自动修复目录或替换路径。

backup 原生预览为 **schemaVersion=2、exact26**：保留旧25字段，唯一新增 `backupDirectory`。action 为 backup，source 为原 descriptor7，targetBasename 固定为 `fleet-events-backup-<databaseEpoch>-<sealRunId>-<previewId>.sqlite`，原 preview UUID 同时冻结目标名和后续 operationId；不会在 commit 再生成另一个备份名。maximumSealedSegments 仍按继承 DTO 显示本产品 Int32，但不是备份容量预算，也不写入 backup worker 请求。完整目标目录和目标 basename 都应在人工确认前可见。

私有预览保存原 path、source／directoryIdentity／run／inventoryId／entryId、previewId 与原 QElapsedTimer。准备后改输入框不能改变该 context；UI须把改输入视为确认退休，用户应重新 prepare。timer按原 ID到期，getter及commit重核原30秒，不续期、不借新路径或新库存重算；30秒只约束提交接纳，不保证实际复制／读回30秒内完成。过期／discard仍保留schema2字段作历史展示，commit接纳后清预览并生成独立 query ID，不用 discarded 暗示磁盘没有改变。

### 固定工单、独立复制事实与线程槽

合法 commit 仅从私有 backup context 生成 worker **exact5**：`kind="backup"/source/expectedDirectoryIdentity/operationId/destinationDirectory`。operationId 必须是原 previewId，destinationDirectory 必须是原 backupDirectory；没有 caller指定文件名／路径修复／maximumSealedSegments，也不在提交时重新生 operationId。worker requestValid 纯值检查通过后才启动实际线程。每次已核提交尝试仍先一次消费原库存、预览及全部 history/catalog/page/search/task 私有游标和公开值；这是源代次退休，不表示源文件被移动，失败／取消也不恢复旧ID，必须重新 inventory。

外层 `maintenanceResult` 继续 **schema1 exact20**，action 新增 backup，完整 workerResult 保留原已核 **backup31**。status 独立按 `backupVerified`／`mayHaveWritten` 映射为 `backup-verified`／`backup-uncertain`／`not-backed-up`，不拿 moveConfirmed／mayHaveMoved 推断复制结果。valid 失败31也完整保留，通用 failure／未知包络只能 failed，不能凭缺少字段推断“没写盘”。

backup31＝原共同12＋业务19：`format="merivus-fleet-archive-maintenance-backup"/schemaVersion=1/kind/operationId/source/targetBasename/expectedDirectoryIdentity/observedDirectoryIdentity/destinationDirectory/destinationDirectoryIdentity/decisionFinal/targetCreateAttempted/targetCreated/mayHaveWritten/bytesCopied/copyCompleted/backupVerified/cancelObserved/errorCode`。bytesCopied 为规范十进制 QString，不通过 JS浮点数表示大字节数。targetCreated、mayHaveWritten、copyCompleted、backupVerified 是不同事实；目标存在、复制完成和读回核验不能互相代替，也不等于掉电持久化或所有事件语义真实完整。

backup 的 `current.archiveMaintenanceWorker` 独立 **exact23**＝共同12＋ `requestId/state/errorCode/decisionFinal/targetCreateAttempted/targetCreated/mayHaveWritten/bytesCopied/copyCompleted/backupVerified/cancelRequested`；旧kind的exact19不加字段或改语义。Store直接保存实际 current，不从public DTO重建；UI应按原动作解读不同形状，不能把backup缺失move字段当false的磁盘保证。

继续只在原实际 finished 后释放槽；resultReady／failure／cancel回执都不先释放，迟到的完整复制事实仍按原 worker/query/run 接纳。所有既有 native/local busy 门禁原样约束backup，begin新录制同样退休备份预览／库存。退出／销毁仅合作取消，不GUI wait／terminate。后台复制若已创建目标，取消或异常可能留下部分文件，不自动删除、覆盖、重试、回滚或把部分目标标为可恢复归档；完整事实交工程师处理。源始终保持封存／退休原名，目标不会自动加入原固定目录的历史或飞行控制。

本批只修改源码、限定源文本核对，不执行备份、编译、helper／parser／SQL／UI、测试或刷机。工程师仍须实操：旧exact6／schema1正路径不变；prepare-backup专属7字段及路径Unicode／ADS／根目录／UNC／别名；原输入改动、原30秒与同值ABA；原previewId目标名一致；源／目标目录换代、源hash变化、已有目标不覆盖；目标创建、短写、关闭失败、取消先后、部分文件保留；读回hash／SQL封存身份和来源不变；31事实与23快照、resultReady/actualfinished顺序和销毁；全部游标退休以及备份结果不恢复任务或授权。上述未执行。

## 462 单文件可恢复归档维护

新增只读 `maintenanceInventory/maintenancePreview/maintenanceResult` 和各自 changed 信号，唯一新产品入口 `requestLocalArchiveMaintenance(intent, confirmed)`。实际 IO 交给独立 parentless [MaintenanceWorker](FleetEventArchiveMaintenanceWorker.md)，本 Store 仅冻结值、原生身份、原单调期限和异步生命周期，不在 GUI 枚举／hash／开 SQL／改名。目标仍仅固定 AppData 目录和原 exact7 政策；不接收任意路径、文件描述符、容量、SQL、恢复数据或控制许可。不需要飞机或飞行账号；confirmed 是本机显式意愿，不是操作员认证、系统文件权限或远端授权。

### 请求、原生预览与期限

intent exact6，全部非 null QString：`action/expectedRunId/inventoryId/entryId/previewId/queryId`。expectedRunId 必须等于本 Store 当前 runId（尚无录制为空）。所有动作 confirmed=true，统一返回原 exact9 request-only 回执：`schemaVersion/action/accepted/requestId/reason/requestOnly/historicalOnly/permissionsRestored/flightCommandReleased`，固定后四标志 true／true／false／false；accepted 不表示文件已移动。

| action | 原生范围及回执 requestId |
| --- | --- |
| `inventory` | 其余四 ID 全空，启动固定目录真实库存工单；native 生成 query UUID。不是普通只读历史目录，包含原封存段和已退休段；新请求先退休旧库存、预览及全部历史游标／值。 |
| `prepare-retire` / `prepare-restore` | 仅选择本 Store 原真实库存 inventoryId／entryId，previewId／queryId 全空；sealed 才可 retire，retired 才可 restore。生成全新 native preview UUID，回执 requestId 为该 previewId。只准备值，不启动线程、不接触文件。 |
| `commit` | 原 inventoryId／entryId／previewId 必须全部一致，queryId 空；原私有 descriptor、directoryIdentity、run 和单调期限重核。接纳后一次消费预览并退休原库存和全部历史游标／值，生成新 query UUID，启动单文件真实工单。失败／取消也不能复活旧身份，须重新 inventory。 |
| `discard-preview` | 同原三 ID，queryId 空；只退休未消费预览，发布 discarded／canCommit=false，requestId 为原 previewId，不表示取消一个已发出的移动工单。 |
| `cancel` | 前三 ID 空，只准命中当前实际维护 query UUID，requestId 为该 query。请求合作取消，不提前清槽，也不承诺决定后文件没有移动。 |

私有库存保存完整实际 worker19、runId、inventoryId 以及每个 native entry UUID 对应的原 descriptor7。public map 不是恢复入口：caller 改写描述、指纹或 DTO 不能创建私有库存。准备时直接从该私有表取源，保存原源、原目录对象身份、原 run 和独立 preview UUID；不重新从公开列表查文件，不续借旧预览。每次新 prepare 是独立显式请求并替代旧预览，旧 timer 只按原 previewId 作用，不能清新预览。

预览提交有效期固定 **30000ms 原 QElapsedTimer**。原单次 timer 到期清私有 preview context，保留明确 expired／canCommit=false 公共值并通知；若 timer 因调度早醒，只重排原截止点剩余时间，不延长期限。getter 同时按原 clock 返回过期状态，commit 再真实重核。该期限只约束接纳 commit，不承诺后台 hash／SQL／改名在30秒内完成；工单接管后原预览已消费，没有默认运行超时、回滚、自动重试或自动清理。

### 三个公共 DTO

共同 exact12：`historicalOnly=true`；`authenticated/signatureVerified/permissionsRestored/executionPermit/flightCommandReleased/executable/completionVerified/restoresAuthorization/allRecordHashesVerified/catalogCompletenessVerified/crossSegmentAtomicSnapshot` 全 false。文件指纹不等于签名，库存完成不代表所有历史真实完整、所有行哈希已核或跨段原子快照。

- `maintenanceInventory` exact22：原 worker exact19 加 `inventoryId/queryId/runId`，原 entries 各 descriptor7 只追加一个 native `entryId`，变 exact8。worker业务7是 `format="merivus-fleet-archive-maintenance-inventory"/schemaVersion=1/directoryIdentity/entries/sealedCount/retiredCount/enumerationCompleted`，另加共同12。descriptor7 全为非 null QString：`basename/kind/databaseEpoch/sealRunId/fileBytes/fileFingerprint/modifiedUtcMs`；kind 为 sealed／retired，bytes／mtime 为规范十进制字符串，指纹为64位小写 SHA256。每类最多128文件，全部通过 worker 完整 matcher 才安装库存；public entry IDs 不写回 worker 原结果。
- `maintenancePreview` exact25：共同12加 `format="merivus-fleet-archive-maintenance-preview"/schemaVersion=1/previewId/inventoryId/entryId/runId/action/source/targetBasename/maximumSealedSegments/expiresAfterMs/status/canCommit`。action 为 retire／restore，source 为原 descriptor7；目标为同 epoch／sealRun 的另一 kind 规范 basename。maximumSealedSegments 为固定 Qt Int32，expiresAfterMs 为 Int30000，status 为 prepared／expired／discarded。prepared 不证明文件当前仍未变化；实际 commit worker 必须重核。commit 后该 DTO 清空，不用 discarded 暗示文件没有动。
- `maintenanceResult` exact20：共同12加 `format="merivus-fleet-archive-maintenance-result"/schemaVersion=1/queryId/runId/action/status/errorCode/workerResult`。action 为 inventory／retire／restore；status 为 running／inventory-complete／move-confirmed／move-uncertain／not-moved／failed。workerResult 起初为空 map，终态为完整已核原 inventory19 或 move27，不改写决定与不确定性标志；failure／包络拒收保持 failed 而不伪造成功。

move27 的业务15是 `format/schemaVersion/kind/operationId/source/targetBasename/expectedDirectoryIdentity/observedDirectoryIdentity/decisionFinal/renameAttempted/renameReportedSuccess/mayHaveMoved/moveConfirmed/cancelObserved/errorCode`，另加共同12。decisionFinal／mayHaveMoved／moveConfirmed 是独立事实；不能拿 accepted、cancelRequested 或系统 rename 返回值单独宣布“没动”或“已恢复”。原 valid move 的失败仍应保留 worker 的不确定性事实，不改成通用成功。

### 实际线程槽、互斥与可恢复边界

`current.archiveMaintenanceActive` 直接表示维护 worker 对象仍在槽内；独立 `archiveMaintenanceWorker` 是其实际 current19 快照，共同12加 `requestId/state/errorCode/decisionFinal/mayHaveMoved/moveConfirmed/cancelRequested`。不伪造 `archivedReaderActive`，不改变 writer 主状态。结果／failure 到达都不释放维护槽；只在这个原实际 QThread::finished 到达后清槽并保留最终 snapshot。析构／退出仅 requestCancel，parentless worker 独立持有复制值，finished 才 deleteLater；GUI 不 wait／terminate，也不依赖界面寿命完成改名。

maintenance 除 cancel 外要求 writer／Reader／rollover／capture／全部 pending 历史及 task 请求为空；本机动作 busy／记录 busy 也必须为空。原 source 绑定、native begin／stop／rollover／current 查询／catalog／page／Reader 启动及所有公开 local 动作／搜索／任务查询／工程记录都加维护互斥，原 native capture 也拒维护栈内接受。维护通知期间独立 busy 阻止同步重入，所有多通知用 QPointer 防同步销毁。维护完成后新查询须新原生身份，开始任意同路径新录制也退休维护库存／预览及原历史身份，原 DTO／cursor 源代次 ABA 不能恢复。

retire 只将一个完整 sealed 文件同目录无覆盖改名为 retired；restore 反向。不会永久删除、覆盖目标、修改数据库内容、修复坏库、自动清理、导入 live BlackBox、恢复任务或恢复权限。**retire 不释放磁盘空间**。restore 的实际 worker 必须持原协作 owner 锁，重新核当前 sealed 数量 **<32**，而不是拿128的诊断扫描上限绕过产品政策；目录／源指纹／mtime／schema／封存标志／目标不存在均在实际 worker 重核，失败要求重新查看库存。协作锁和 native no-replace 不认证其他进程，不能防所有不合作外部改写或电源丢失。

本批仅静态源文本读回，未编译、测试、执行 helper／SQL／应用／UI、刷机、提交 PR 或打包。工程师仍须实操：空／混合／超限库存、不可读／坏库／sidecar／锁冲突、全部 exact 类型／ID／原run、prepare 期限和旧 timer、新录制清身份、原文件或目录置换、同名目标无覆盖、restore32段上限、Windows／Linux native no-replace 和不支持平台、决定前／后取消、决定后不确定性、result 与 actual finished 先后及多通知销毁、所有 history/task 游标退休与不回灌控制。上述均尚未执行。

## 461 留存任务诊断目录和原始记录钻取

新增只读 `historyTasks/historyTaskRecords` 与各自 changed 通知，以及 `requestLocalArchiveTasks(intent, confirmed)`；复用同一实际 [HistoryReader](FleetEventArchiveHistoryReader.md)、固定本机目录／政策和原数据库 schema。不是另一个任务库，不从文件恢复任务、权限、审批或执行状态。构造、属性读取和界面出现均不自动查询；明确查看请求不需要飞机或账号，也不认证日志作者。

### 请求和原生游标

产品 intent exact6，全部为非 null QString：`action/expectedRunId/scope/previousQueryId/directoryId/taskIdHash`。expectedRunId 仍须等于当前 Store runId，尚未录制为空；scope 只准 `sealed` 或 `sealed-and-offline`。统一返回原 exact9 request-only 回执。所有查询均须 confirmed=true，实际 writer／Reader／rollover／capture／pending 槽均空闲；停止请求不是 writer 已退出的证明。

| action | 固定范围 |
| --- | --- |
| `tasks` | previousQueryId／directoryId／taskIdHash 全空；native 生成全新目录 UUID，从原未过滤 exact5 搜索开头扫描。替换旧目录同时清旧任务行、文件身份、records 值和两个 task continuation。 |
| `continue-tasks` | 当前实际目录 UUID、上一实际目录 queryId，taskIdHash 为空；只能消费本目录私有未完成游标。 |
| `task-records` | previousQueryId 为空；当前实际目录 UUID 及其已观察到的真实 taskIdHash，严格64位小写十六进制。以原目录 inventoryFingerprint 约束 fileIndex0／afterRowId0 原生起始游标；不是先查新目录再事后比 hash。 |
| `continue-task-records` | 同目录 UUID／任务 hash／上一实际 records queryId，消费其独立私有游标；不借用目录续扫 cursor。 |

底层四种请求的 pending kind 仍为 search，私有 purpose 按原 queryId 冻结进 result／finished 闭包，不按后来可变 action 猜测接收者。原 exact5 搜索用于目录；records 使用 Reader exact6／schema2，新增 taskIdHash，返回 exact29。原目录中的任务 hash 只能选择范围，不能指定路径、SQL、文件身份或任意偏移。每个批次仍受 Reader 的最多4页／256扫描行／64匹配行及原字节预算约束。

普通 historySearch、目录和 records 的上下文彼此独立。原普通续搜仍核本身保存的 actual query／run／原 DTO／过滤／完整 matcher，并保留原全局 latest／archivedResultAccepted 规则：其他查询更改全局身份后，旧普通搜索须明确从头开始。新增目录／钻取不写原普通 DTO／context、不借其 cursor；两类新续扫仅依各自原 native 成功上下文，不依全局最近查询身份。每次启动前仅消耗本类旧 cursor；失败／取消不会复活。新录制成功 begin 清全部搜索值／上下文并排队通知。

### exact35 目录值

业务23键：`format="merivus-fleet-retained-task-directory"/schemaVersion=1/directoryId/queryId/runId/scope/inventoryFingerprint/status/errorCode/scanComplete/canContinue/partial/budgetReached/scannedRows/unattributedRows/taskCount/batchCount/warningRows/clockDiscontinuityRows/retentionWarningPages/filesTotal/filesCompleted/tasks`。

共同12键：`historicalOnly=true`；`permissionsRestored/authenticated/signatureVerified/executable/completionVerified/executionPermit/flightCommandReleased/restoresAuthorization/historicalCompletenessVerified/crossSegmentAtomicSnapshot/wallClockOrderingUsed` 均 false。数字计数均 Qt Int；tasks 按 hash 排序，最多256。status 区分 scanning／partial／completed／budget-reached／failed／cancelled；completed 仅表示这个固定留存扫描到末尾，绝不表示任务完成或全集真实完整。

每个任务行 exact9：`taskIdHash/eventCount/vehicleIds/eventCategories/queuedTrueRows/uncertainTrueRows/clockDiscontinuityRows/warningRows/vehicleAssociationKnown`。eventCategories 只可能出现原 codec 八种类别 command／transaction／task／formation／fleet／intent／maintenance／other，值为明确观察行数；不能由 originalTypeHash 推断 task.updated 等原始子类型。task hash 来自原 row.record.event.payload.taskIdHash；没有合法值的行只计 unattributedRows，不拿 planId／runId／时间或机号猜任务关联。

vehicleIds 仅合并原固定 payload 中实际保留的 vehicleId／targetVehicleIds，范围1–255，排序去重。vehicleAssociationKnown 仅表示至少观察到一个明确关联，不代表已知完整参与机队；没有关联保空列表，而不是借实时 Registry 补齐。queuedTrueRows／uncertainTrueRows 只计实际 bool true，零不代表其余行明确 false，更不代表执行成功。原 task.updated 的 nested summary 已被投影丢弃，不能恢复完成、覆盖率、里程、能耗或 ROI。

eventCount 是物理留存行计数，不按原事件 ID 跨段去重；task 行之和加 unattributedRows 等于已提交 scannedRows。warningRows 是原行 uncleanRunMayHaveLostTail／非零 runDroppedCount／非零 runDiscardedCount 的并集行数；这些 run 级计数可能在多行重复，不是精确丢失事件总数。clockDiscontinuityRows 只计原 record 顶层明确 true。retentionWarningPages 计 scannedPages 中 retiredCount 非零或 cursorPrecedesRetainedHistory 的页观察次数，同一文件重复翻页可以多次计入，不是退休行数。

### exact31 记录值

业务19键：`format="merivus-fleet-retained-task-records"/schemaVersion=1/directoryId/taskIdHash/queryId/runId/scope/inventoryFingerprint/status/errorCode/scanComplete/canContinue/partial/budgetReached/scannedRows/cumulativeScannedRows/matchedRows/batchCount/searchResult`，另加上述共同12键。searchResult 是本批完整、已由 searchResultMatches 核验的原 exact29，不重写其中行／文件／扫描页／只读标志。scannedRows／matchedRows 只指本批，cumulativeScannedRows／batchCount 指该次固定任务钻取的累计范围；每页只保存当批原始行，不做无界 GUI 累积。

目录可为 partial、failed 或 budget-reached，但其中已经提交的真实任务 hash 仍可显式钻取。records 可以扫描同 inventory 的完整留存范围，而目录行数仍只覆盖其自身已提交前缀；records.scanComplete 不会升级目录的状态、计数或完整性。NDJSON runId 没有可信 task 关系，禁止按时间／机号把遥测联表归入该任务。

### 原子提交、上限和失败

结果必须来自同实际 Reader／原 query／kind／发起 run，完整 searchResultMatches 通过后再检查原 native 目录／任务关系。每个已见文件按原 fileIndex 保留 basename／epoch／sealRunId／完整 fileFingerprint／offline 标志，同 inventory 后续观察不一致拒绝整批。目录和 records 共用本目录已见文件身份，但不共用 cursor，目录 inventory 本身也不把文件内容 hash 认证为签名或跨段原子快照。

目录累计最多8192扫描行／256任务；每个 records 查询累计最多8192扫描行。行、任务、文件身份和计数全部先在局部值 staging，整个批次有效且不超限才一次提交，再发通知。遇任一超限不保留该批合法前缀，旧累计值保持，status=budget-reached、canContinue=false、partial=true、scanComplete=false，errorCode=`archive-local-tasks-batch-budget-reached`。records 启动已清当前页，因此该拒收值 searchResult 为空、当批 scannedRows／matchedRows 为0，累计数仍为此前完整批次；目录仍显示此前完整 tasks。恰好8192且扫描未完时提交完整本批，errorCode=`archive-local-tasks-row-budget-reached`、status=budget-reached，下一页不可继续；恰好8192且扫描已完仍 completed。无超限时 partial／completed 的 errorCode 为空。

内层 searchResult 仍受原固定2MiB政策；Reader 对旧schema1保留原512字节元数据预留，新 task-filter schema2专属预留2048。新增35／31键外层不是原 Reader 包络，其完整紧凑 UTF-8 JSON 在提交前仍独立核同一固定 `2MiB` 上限，不增加政策或截合法前缀；超限整批拒收并失败，不提交新 continuation。目录行数／机号集合／类别亦分别有界；JSON 字节上限不代表 QVariant 堆分配字节、零 GUI 开销或实际延迟保证。

scanning 和失败首批可以 batchCount0；正常零行末尾仍提交一批，batchCount1。queryId 表示这个实际发起请求，不是成功证明；failed／cancelled 保留该身份、明确错误和 canContinue=false。若取消发生在实际结果已交付后，已展示历史批次仍保留，仅退休 continuation；已读完的 scanComplete 可以仍为 true。失败或未核验结果不更新全局 lastHistoryQueryId；有效预算终止是已核验的诊断结果，但不宣称该批数据已加入累计。

取消仍使用原 requestLocalArchiveAction(cancel)／原请求 ID，不新增关闭或控制接口。原 Reader 实际 finished 之前全局槽始终占用；resultReady 不释放槽，失败／取消也不提前复用。finished 才清 private pending 并发布终态。任何新显示值在通知之前完整提交；重入、过期或已取消的 Reader 不能改写后来目录。历史始终不回灌 BlackBox、规则、任务、Registry、授权或 Vehicle。

本批只改源码并作限定文本／同行静态检查，未编译、运行应用／SQL／helper／UI／测试、刷机、提交 PR 或打包。工程师仍须实测空目录／零匹配、实际 schema1/2 与 Qt 数字类型、辅助机号／summary 缺失、跨段同 task／两个 task 游标独立与旧普通 latest 规则、原 inventory 和文件置换、256任务／8192行整批拒收及恰好上限、目录 partial 与 records completed 区分、取消先后序／actual finished／source 销毁／新 run 清值、损坏／锁冲突／unclean／退休／丢失标志，以及所有历史查询不回流控制。以上均尚未执行。

## 424 跨归档文件的有界搜索入口

新增独立只读 `historySearch` 值／`historySearchChanged` 通知与 `requestLocalArchiveSearch(intent, confirmed)`，复用原固定 AppData 目录、固定政策及 [HistoryReader](FleetEventArchiveHistoryReader.md)。搜索不改 `historyPage/historyCatalog`，不伪造 catalogId，也不让搜索游标成为 current／offline／sealed 页游标。下方“只有本页筛选／没有全库搜索”为历史状态；本批是显式、有界、分次扫描归档摘要，不是实时索引、完整遥测检索或已认证的全部历史。

外部 intent 恰好六个非 null QString 字段：`action/expectedRunId/scope/vehicleId/operation/previousQueryId`。action 为 `search` 或 `continue-search`；scope 为 `sealed` 或 `sealed-and-offline`；vehicleId 为空表示不过滤，否则须规范十进制 1–255；operation 为空或四种工程审批操作之一。空 vehicleId 在 native 请求中规范为 Qt Int 0。expectedRunId 须仍为本 Store 当前 runId（尚未录制为空）；不是历史 runFilter。首次 previousQueryId 必须为空，续搜必须原实际成功 query UUID。所有请求需 `confirmed=true`，只是显式查看意愿，不认证账号或材料作者。

不需要连接飞机、绑定 BlackBox 或登录飞行账号；两种 scope 都要求本实例 writer 对象已不存在，且没有 Reader、rollover、captureBusy 或待交付查询。点击 stop 或 captureEnabled=false 不替代 actual writer finished。Store 底层再次要求固定目录／政策、无 writer 及合法原生请求。搜索不创建目录、开启录制、修复／删除数据库，不触发飞机控制。

原生请求 exact5：`kind=search/scope/vehicleId(Qt Int)/operation/cursor(QVariantMap)`；首次 cursor 为空。Reader 的公开纯值 `searchRequestValid`／`searchResultMatches` 分别检查请求和返回包络。搜索遵守原单 pending 槽，原 queryId/kind 在启动 Reader 前预留；action 即便为 continue-search，pending kind 也固定 search。统一回执仍 exact9，action 保留 search／continue-search；accepted 只表示请求接纳，不是搜索完成或历史完整。

### 续搜、取消和结果隔离

Store 私有保存实际成功 queryId、发起 runId、原 exact5 请求及实际 nextCursor。续搜要求 previousQueryId 同时命中私有成功身份和 current.lastHistoryQueryId，原 scope／机号／操作过滤及 run 不变，当前实际 historySearch 仍是已核验的未扫描完结果，nextCursor 精确等于私有值。不接受 caller 提供文件路径、fileIndex、hash 或 rowId；普通目录／页／离线查询一旦成功替换最新历史身份，旧搜索也不能继续。原生 nextCursor 的目录指纹、文件序号、行号、文件指纹与库 epoch 由 Reader 继续核验。

每次合法搜索／续搜发起前消耗旧私有续搜上下文，只有新的实际成功结果才建立下一次上下文。失败或取消须显式从头搜索，不能恢复旧 cursor 冒充继续；保留的旧 historySearch 仅用于标明旧结果。取消使用原 `requestLocalArchiveAction(cancel)` 和原 requestId，原 Reader 实际 finished 才释放槽。即使结果先于取消到达，已展示的历史事实保留，但取消立即使续搜上下文失效。开始新录制清空搜索值／私有上下文，并异步通知，不恢复任何旧查询或权限。

结果来自原实际 Reader、原 queryId/kind 和原发起 run；独立 search 分支经 `searchResultMatches` 核验 exact28、过滤条件、续搜作用域、只读标志、扫描计数、行／扫描页身份及整包字节预算后，才更新 historySearch、lastHistoryQueryId 与 archivedResultAccepted。返回 row 保留原8键记录及文件身份；scannedPages 还保留未匹配行所在页的退休范围、unclean 和丢弃相关行数，不能把“本次未匹配”说成“历史不存在”。没有改变 historyPage/catalog 或导入 BlackBox、规则、Replay、Registry、审批、任务状态。

scanComplete 只表示本次冻结目录观察范围扫描到末尾；排序为文件名再行号，不是跨机飞行时间顺序。完整性、跨段原子快照、身份／签名／权限恢复和执行许可标志始终否认。读取可能因目录／文件变化、锁冲突、sidecar、坏库、字节或扫描边界而拒绝；完整语义和当前上限见 Reader 契约。遗失／退休／未曾录制数据不能通过搜索补回，仍缺持续完整遥测、完整构型档案、实时索引、长期自动留存和运营集成。

本批仅静态源码／文本核对，未编译、测试、运行 helper／SQL／认证／UI／应用、刷机或打包。工程师须验证首次／续搜同条件、跨文件／末尾空结果、目录或文件变化、不同面板竞争、旧 current/offline/sealed 游标隔离、取消先后序、Reader finished 与 writer 开始互斥、预算截断和未知历史提示；以上均未实操验证。

## 423 未封存主库的显式离线只读入口

在原 exact6 intent／exact9 回执中增加 `action=offline`，不新增任意文件路径接口、SQL schema、录制默认值或历史权限恢复。只读目标固定为 `localArchiveOptions.directoryPath/fleet-events-v1.sqlite`。下方 422 及更早“未封存主库没有离线读取”的叙述为历史状态，由本节覆盖；封存目录仍不列主库，离线读取也不把主库伪装成封存段。

- `expectedRunId` 仍须等于本 Store 当前 runId（新启动未录制时为空），只约束发起者所见本实例状态，不是历史 runFilter。`catalogId/basename/queryId` 必须为空，`afterRowId` 仅允许 `"0"` 或本产品最近实际成功离线页的 nextRowId。`confirmed=true` 仍仅为显式查看意愿，不是账号认证。
- 不需要已连接飞机、BlackBox source 或账号；不调用 beginPersistence、mkpath、修复、清理或封存。`current.writerActive` 直接表示当前 `_worker` 对象仍存在。offline 拒绝任何 writer（包括 opening/stopping/failed 但线程尚未收尾）、Reader、rollover、captureBusy 或待交付查询。`stop` 回执 accepted 不等于已经可离线读；须等实际 writer finished。
- 产品发起仍先预留 queryId/kind，再启动原 parentless Reader。底层 `startArchiveReader` 再次拒绝 offline 搭配现存 writer、非固定目录／政策或非法 exact6 Reader 请求。原 beginPersistence 同样拒绝 Reader 尚在，不会在离线读取过程中开始新写入。
- Reader 请求恰好 `kind=offline/databaseEpoch/fileFingerprint/runFilter=""/afterRowId/limit=64`。首读 after0 的 epoch/hash 都为空，由实际只读结果发现；下一页 epoch/hash 从本 Store 私有实际成功上下文取得，不采信 caller 字符串。上下文必须与当前 lastHistoryQueryId、historyPage 的 epoch/hash/next 一致，原页还须 hasMore=true；任何 current/sealed/catalog 成功替换了当前历史身份后，旧 offline 游标不能续用。可从 0 显式重开。
- offline 使用原 Reader 同一 pending／取消／finished 生命周期。`cancel` 仅能命中本产品原 queryId/kind；取消不立即释放原槽、不关闭录制线程，不自动在 UI 退出时执行。原 Reader finished 才退休该查询。失败／取消保留的旧页不代表新查询成功，仍应核 requestId 与 current.lastHistoryQueryId 以及 archivedResultAccepted。

### 结果绑定与历史边界

实际 Reader 结果为 exact28：共享 SQL 页原20字段，加 `basename="fleet-events-v1.sqlite"`、`offlineCurrentOnly=true`、`sealedSegmentOnly=false`、小写 SHA256 `fileFingerprint`、`cooperativeOwnerLockHeldDuringRead=true`、`executionPermit=false`、`flightCommandReleased=false`、`restoresAuthorization=false`。没有 catalogId／sealRunId；不会生成新 catalogId 或修改封存目录身份。

Store 在独立 offline 分支核实际 Reader、原 queryId/kind、取消状态，以及请求、格式、真实类型、UUID、hash、原 after、空 filter、三态之外的固定只读标志、保留范围、页数上限和整页字节预算。首读只接受实际有效 epoch/hash；续页必须与原私有冻结 epoch/hash 完全一致。`pageBytesWithoutThisField` 按不含自身字段的整页 UTF-8 JSON 实际长度核对。成功才发布 historyPage、lastHistoryQueryId、archivedResultAccepted，并更新私有 offline 游标；current／sealed 游标不能借用 offline 页。

读取依赖 [HistoryReader](FleetEventArchiveHistoryReader.md) 的同目录 owner 协作锁、只读 SQLite、schema／行摘要核验，以及读前后整个文件指纹检查。锁只约束遵守同一约定的进程，不认证作者、不防恶意／旧版外部 writer；全文件 hash 不等于签名、持久性或跨页原子快照。页间文件变化拒绝续页，需要明确重开；不冻结数据库到下一次人工翻页。hot-journal／未知 sidecar／坏库保持拒绝，不自动恢复。unclean、丢弃、退休范围仍来自历史记录，空页和 clean 标记都不证明全集完整。

读取结果只用于诊断展示，不回灌 BlackBox、规则、Replay、Registry、审批、任务或飞行指令。只读数据库会话仍需要协作锁的本机文件操作，不应承诺整个文件系统完全零写入。完整遥测、全局历史索引、完整审批档案／配置差异恢复、企业权限、自动清理、运营闭环仍待开发。

本批仅源码读回和限定文本检查，未执行编译、测试、SQL／helper／应用／UI、刷机、提交或打包。工程师应验证：冷启动无 source 的离线首读、stop 后线程真实收尾、writer/reader 双向互斥、锁冲突、空库／unclean／坏库／sidecar、64 行和字节边界、页间更换同 epoch 文件、过期 query／cancel／finished、current/sealed/offline 交叉游标拒绝，以及读取后显式 start 不复用旧离线身份。上述均未实操验证。

## 422 固定本机目录的显式产品入口与工程审阅摘要

新增受限 `Q_INVOKABLE` 产品请求，复用本实际 Store、[Worker](FleetEventArchiveWorker.md)、[HistoryReader](FleetEventArchiveHistoryReader.md)和[Codec](FleetEventArchiveCodec.md)，不另建数据库或把原任意目录/政策 native 方法公开给 QML。下方“没有运行中入口／只有值”的叙述属于对应历史；任意路径、导入、授权恢复及飞行控制仍未开放。

`localArchiveOptions` 是只读常量 exact7：`schemaVersion=1`、`directoryPath`、`policy`、`maxSealedSegments=32`、`pageSize=64`、`defaultEnabled=false`、`storesRawPayload=false`。目录仅由本机 `QStandardPaths::AppDataLocation/fleet-os/event-archive` 生成，既不读取 QML 路径，也不因构造、绑定或读取属性而创建目录或启用归档。政策 exact7 均为 Qt Int：`maximumEvents=100000`、`maximumDatabaseMiB=256`、`maximumQueueEvents=512`、`maximumQueueMiB=8`、`batchEventLimit=64`、`busyTimeoutMs=250`、`maximumHistoryMiB=2`。

### 动作与回执

`requestLocalArchiveAction(intent, confirmed)` 的 intent 恰好六个非 null QString 字段：`action/expectedRunId/catalogId/basename/afterRowId/queryId`。`expectedRunId` 必须仍等于本 Store 当前 runId；尚无 run 时使用空字符串。无关字段必须为空，非翻页动作 `afterRowId` 必须为 `"0"`。所有请求须明确 `confirmed=true`，只是用户意愿，不认证身份。

| action | 允许的本地请求 |
| --- | --- |
| `start` | 空闲且实际同线程 BlackBox 已绑定时，才检查／创建固定 AppData 及固定子目录，再开启同一 native begin 流程；不覆盖、删除、修复已有坏目标，不回录此前内存事件。 |
| `stop` | 只停止本实例固定目录、精确固定政策的当前 worker，异步 drain；不封存、不自动重新开启。 |
| `seal` | 同样只接当前固定 worker，复用显式轮换：排空、封存，然后仅在原完整后置条件成立时开启同源新 run；不是“封存并停止”。上限32段，失败不自动清理旧段。 |
| `catalog` | 只读固定目录中可验证的封存段，不创建目录或当前数据库，不接受 caller 提供目录／政策。 |
| `current` | 只查询固定 worker 当前正在录制的 run，`runFilter` 为该实际 runId、limit64；不是独立只读会话。 |
| `page` | 只读本 Store 当前 actual catalogId 中的实际 basename，`runFilter` 为空、limit64；不得合成段身份。 |
| `cancel` | 仅当 queryId 仍是本产品发起且与当前原 pending 槽精确一致时请求取消；旧页不能取消新请求。 |

`catalog/current/page` 的 intent.queryId 必须为空，由 native 生成全新 UUID requestId；只有 `cancel` 使用原回执 requestId。当前页／封存页只允许从 `"0"` 重开，或使用当前实际页匹配作用域的 nextRowId。当前页须同 product query、runId、实际 DBepoch；封存页须同 catalog、basename、空 filter，并继续通过原 native 游标检查。没有任意 rowId 跳页或跨段全局游标。

统一回执 exact9：`schemaVersion=1/action/accepted/requestId/reason/requestOnly=true/historicalOnly=true/permissionsRestored=false/flightCommandReleased=false`。接受查询只表示已提交请求；不是结果、SQL commit、完整历史或飞行许可。UI 应按回执 requestId 与 `current.lastHistoryQueryId` 识别新结果，不能把保留的旧 historyPage 当本次成功。queryId/kind 在发起前预留；嵌套事件循环即使提前交付 result/finished，返回时也不重新装入已经退休的 ID；拒绝只清自己那一项。

产品 current 取消与旧失源/discard 取消分开：只标记该原查询，不停止健康 recorder，也不立即释放槽；原实际 worker/run/query 的 pageReady 证明 SQL 请求已终结后，丢弃该页并清本产品原槽。失败无 pageReady 时由原 worker finished 兜底。原 native 失源/discard 策略仍保留到原 writer finished。Reader 的 catalog/page 取消仍等实际 QThread.finished，不能通过取消提前复用正在运行或待交付的槽。

### 审阅事件只记固定摘要

`recordLocalEngineeringReview(vehicleId, operation, result, frozenComparison)` 调用 Codec 的纯值 `engineeringReviewPayload`，只把 outer2/inner21 固定摘要送入本 Store 原实际同线程 `FleetEventBlackBox::record`，source 固定 `fleet-engineering-review`。基线两操作使用 `fleet.configuration.review.result`，能力两操作使用 `fleet.capability.review.result`。无效车辆／操作不给事件；不自动开始 BlackBox recording 或 SQL 归档，也不把 caller 的 result 值认证为真实批准。

记录同样返回 exact9，action 固定 `record-review`、requestId 为空。`accepted=true` 仅保实际 BlackBox 已返回的内存接纳事实；即使通知中 Store／source 被销毁，也保留该局部返回值，不用后置来源丢失伪装没记过。不据此宣布 SQL 已录制、持久提交或完整审计。后续归档仍走原 native epoch／接纳序号／在栈精确收据和原 queue/gap 规则。

摘要只保操作、车辆、三态提交／nativeOk／stillCurrent、有限变化计数／typed指纹／能力数量／reviewId哈希及固定否认标志；不保存原配置、完整比较13／16、完整审批回执、SID、自由理由、scopeToken 或凭据。原值 unknown 不补 false；旧 v1 摘要也不能补回此前未记录的详情。这是跨重启可查询的有限审阅摘要，不是可恢复的 typed 构型历史、签名或当前批准。

### 当前范围与工程师验证

本批仍仅封存段能通过独立 Reader 跨重启读取。普通 stop／退出未封存的 `fleet-events-v1.sqlite` 位于同一固定目录，但不纳入封存段目录清单；没有新增离线 current 读取、自动封存、后台自动启用或完整历史承诺。现主库有上限留存，退休／丢弃／unclean 信息必须一起显示；空目录、空页和 hash 匹配不证明记录从未存在、作者身份或全集完整。

显式 start 的固定目录初始化使用有限 Qt 文件系统检查和 mkdir，在调用线程发生；后续 SQL、数据库开关、封存及历史重读仍在原 worker／Reader。现有协作锁、规范路径／符号链接检查不宣称恶意 TOCTOU、目录 ACL、断电持久性或零 UI 延迟。不得删除锁、重建坏库或靠改政策接管其它 native 目录的 worker。

本模块已做源码及限定文本读回，未编译、运行、执行 SQL／helper／认证／UI、测试、刷机、更新 PR 或打包。工程师须验证默认关闭和固定目录初始化、Qt 类型往返／资源接线、两入口独立请求、过期 run／catalog／游标拒绝、pending／取消／嵌套 result／finished、内存接纳但 SQL 关闭／满盘／队列溢出、封存失败与新run、重启只见 sealed／旧 v1 摘要、source／Store 销毁后提交事实保留。原25目标与六类实质工作继续，不以该摘要入口视为长期运营或完整执行产品验收。

## 310–314 原生目录读取失败边界

封存 Worker 与 HistoryReader 改用 [FleetArchiveDirectoryInventory](FleetArchiveDirectoryInventory.md)，正常 EOF 与中途 I/O、取消和 close 失败明确分开，不再依赖 Qt iterator 结束表示成功。严格 UUID/普通文件/sidecar/规范路径/容量、SQL 身份/hash 及封存锁/rename 原有门禁保留。Reader catalog 的两次扫描私有 directoryIdentity 和段列表必须一致；成功结果顶层新增 `directoryInventoryEnumerationCompleted=true`，Store 还独立要求该字段为非 null 的真实 Qt bool true，再更新 actual catalogId/历史快照。current/page 不新增该目录声明。

exact entry7、SQL schema、页格式和权限/任务隔离不变；失败不生成新的成功目录，历史旧快照仍带旧身份。`catalogCompletenessVerified/allRecordHashesVerified/crossSegmentAtomicSnapshot` 仍 false，正常枚举不认证全集、原子状态、文件作者或无恶意并发。下面295–299对 Qt iterator 缺口的描述属于历史实现，本次只补 EOF/error 来源，全集索引、自动归档/留存和运营集成仍待开发。

未执行目录/SQL/hash/helper、编译或测试。工程师须验证目标平台正常空目录、中途失败、取消、close 失败、Unicode、目录替换及封存上限，并检查实际 Qt/native 头文件/文件系统语义；本次没有部署验证证据。

## 295–299 跨封存段浏览与分页

新增[只读HistoryReader](FleetEventArchiveHistoryReader.md)和[唯一SQL声明/读页](FleetEventArchiveSql.md)，复用原Writer schema而不迁移或恢复数据库。仅native C++显式调用，默认不枚举目录、不启动SQL；离线历史查询不依赖live BlackBox，不创建current采集库，也不改变实时采集、任务、审批或控制。公共QML只有historyCatalog/historyPage及current状态值，无文件/查询/取消对象入口。

- `requestArchiveCatalog(queryId, absoluteDirectory, exactPolicy, operatorConfirmed)`：规范绝对本地目录及原七整型政策、明确consent。若实时worker仍在，只允许同一明确配置目录；独立Reader只读扫描封存段，不接受current文件。成功后才生成本native catalogId并冻结目录/政策/实际entries，失败保留旧历史快照而非更新live状态。
- `requestArchivedPage(queryId, catalogId, basename, runFilter, afterRowId, limit, operatorConfirmed)`：必须当前实际catalogId和其中实际条目，epoch/sealRunId从冻结条目取得，不由caller map或任意路径生成。每段只有一个当前filter游标，最多128上下文；只允许0重开或该实际catalog/segment/epoch/filter上一成功页nextRowId，换段/换filter从0开始，替换catalog或改变记录目录清旧游标。不是跨库全局rowId，也不提供任意偏移跳页或统一飞行时间排序。
- `cancelArchivedHistory()`：仅合作取消Reader；不UI wait/terminate，不撤回已经交付的历史值，不提前释放仍运行/待交付的槽。

实时页/catalog/封存页共一个pending槽；Reader实际GUI结果交付后仍保留至actualfinished，晚到/取消结果必须同Reader QQ/查询/kind匹配才发布。current查询在discard/source丢失/失败时只标取消，槽保留到actualwriterfinished，避免旧查询未结束就复用。Writer与Reader各仅在另一线程对象已不存在时停止250ms状态轮询；source退出不会盲清独立Reader槽。开始新的实时采集须无Reader，维护轮换也拒绝Reader尚在，避免更换配置或目录时混合查询作用域。

页源复用严格类型、actual DBepoch、EOF/error、row codec/hash和whole JSON边界；内层预算允许1MiB-1024而外政策仍1–8MiB，Reader预留512字节供门面catalogId等metadata，门面加上下文后重算pageBytesWithoutThisField。native目录/文件/SQL耗时在parentlessReader线程；GUI值序列化仍有有限CPU开销，不称零延迟或实际性能已验证。

目录最多4096条目/128段，每段受主库字节政策限制。Catalog排序仅basename，catalogCompletenessVerified/allRecordHashesVerified/crossSegmentAtomicSnapshot均false；segmentCount只表示本次观察核验数，空entries不证明历史永不存在。平台目录I/O/EOF完整来源、全集索引、每条历史证据认证、恶意并发/ACL、自动归档清理、完整遥测和运营集成仍未完成。封存操作clean仅约束该sealRun，旧unclean/drop/retired事实逐行保留，历史不回灌rules/Replay、任务、机队或权限。本文后面290–294/285–289叙述为对应历史批次，当前已有native跨段浏览底座但完整产品目标不缩减。未运行Reader/SQL/hash/目录helper、编译或测试。

历史实现的目录限制：依据[Qt 5.15.2 QDirIterator公开接口](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/io/qdiriterator.h)缺少独立枚举错误获取接口，旧 hasNext/重扫不能推导 I/O 与 EOF 已区分，这是接口层推断而非平台测试。310–314 已由原生错误可区分清点替换该路径，但仍不证明全集/原子快照。

## 290–294 显式维护轮换

新增native `requestRollover(operatorConfirmed, maximumSealedSegments=32)`，最大封存段数1–128，无自动删旧段或未确认的阈值轮换。仅当前实际recording worker、实际同线程BlackBox/epoch、无待GUI历史页且非捕获重入时接纳；返回true是请求，不是成功。暂停基线取该实际BlackBox此刻接纳序号，封存和重启属于本次显式维护consent，不改变默认off。

暂停还要求实际BlackBox序号等于本消费者最后已观察序号，不能在更早的native signal槽里抢先停止、漏掉仍未递送到Store的同一事件而记0缺口。尚未观察完的发布窗口拒绝维护请求，需退出该回调栈后重新显式请求；不通过默认缺口或自动重试隐藏源顺序。

每次实际source绑定另有独立UUID binding epoch，destroyed回调核此epoch。即使旧parentless源移到别的线程后析构、其已排队destroyed迟到，也不会清除或停止后来绑定的新源/新run；disconnect本身不当作取消已排队回调的证明。

Worker协作锁覆盖SQL生命周期及关闭后的封存；实际clean commit、SQL关闭/句柄释放、库内容与安全路径复核后，才将固定current库不覆盖地保存为同目录 `fleet-events-sealed-<databaseEpoch>-<runId>.sqlite`。只有实际finished/clean/sealed以及精确DB epoch/固定文件名和同一源QQ/epoch仍有效时，门面沿用此次明确目录/政策开启全新run。新captureStart序号与暂停基线的差额作为rolloverUnrecordedSourceEvents/sourceMissingCount及externalGap进入新run；仍不是完整遥测缺失的精确证明。

普通stop、应用退出、析构及来源丢失先撤销续建意图。final停止决定后的取消不能承诺撤回已经发生的封存；lastRollover始终保留实际seal结果并分开标注resume cancelled，旧文件仍保留。失败、段数上限、未知sidecar、置换或新run开库失败不自动删除/重建/还原文件、不恢复权限，不伪称继续记录。lastRollover只暴露固定文件basename/DBepoch/源run/请求结果，不暴露目录或授权。新runRequested只表示opening接纳，非已经稳定记录。每次新请求清旧窗口数并标gapKnown/rolloverWindowKnown=false，未知不得当0；实际finished仍核同QQ/epoch和序号后才记录本窗口pause/end/unrecorded/observationOnly。该窗口止于finished时刻，取消/停止后更晚未捕获的事件不包含在此数里。pending期间保留stopping-for-rollover状态，不被worker尚在处理的recording字样覆盖。

QLockFile只是遵守同一约定的本产品实例锁，不认证文件或阻止恶意/旧版外部writer；流式hash是内容变化检查，不是签名。Qt/file-system rename、失电与目录fsync仍待工程师验证，不声称强原子/恶意TOCTOU安全。封存段保留会增加总磁盘占用，上限段数加每段主库政策只是部署预算的一部分，journal/sidecar/备份另计。当前没有跨段历史查询、离线恢复、自动外部归档/清理或完整遥测；原六类/25目标继续。本阶段未运行worker、rename、锁/SQL、编译或测试。

rename或后置验证失败可能已经改变文件位置，sealed=false不等于文件一定没动。lastRollover分开保留sealMayHaveMoved和精确candidateFileName/源DBepoch，不自动反向rename、不删候选文件，工程师须核原/候选路径。只有完整后置验证成功才sealed=true并允许续建。

sealErrorCode、resumeErrorCode/resumeCancelled分别保存，不让取消续建覆盖真正封存失败原因。新请求接纳时保存newRunId，后续新worker失败追加newRunErrorCode；手动再begin不清除这份最近维护摘要，但不是长期跨进程索引。完整newRun观察事实仍看当前worker，不能从newRunRequested=true推断已成功录制。

285–289 为路线图第18/20项目标和长期运营类补充实际诊断事件数据库入口。它使用[固定摘要编码器](FleetEventArchiveCodec.md)及[专属存储线程](FleetEventArchiveWorker.md)，不是完整遥测采集、飞行黑匣子认证、授权恢复或任务执行闭环。源码及有限文本/同行审阅，未编译、调用业务接口、运行应用、建立数据库、执行 SQL、测试或刷机。

## 接入和显式配置

`FleetPersistentEventStore` 在 `MerivusInternal 1.0` 注册；`GuidedActionsController` 创建 objectName 为 `merivusFleetEventArchive` 的实际对象，只绑定同线程实际 `FleetEventBlackBox`。默认 state 为 disabled，构造/绑定/QML初始化都不建库、不读取目录、不设置路径、不回录当前内存历史。

当前七个操作为 native C++ 方法，而非 Q_INVOKABLE；外部 QML facade 只有 current/historyPage/historyCatalog 值及 readOnly/historicalOnly 标记，没有文件操作、record/import、重放或控制对象；下列四项为原实时库操作，新增三个跨段操作见295–299节：

- `beginPersistence(absoluteDirectory, exactPolicy, operatorConfirmed)`：必须显式确认、本实际源及空闲实例；接受配置仅表示异步 opening。目录必须是规范绝对本地路径，实际存在性/规范路径/文件类型与库身份由 worker 检查，门面不自动创建目录。exactPolicy 恰好七个真实 Qt 整型字段，范围见 worker 契约，无默认值。
- `stopPersistence(drain)`：只请求异步停止。无 GUI wait、terminate、删除运行中的线程；应用退出请求不等于 durable 或 clean 确认。
- `requestRollover(operatorConfirmed, maximumSealedSegments)`：显式维护排空/封存/同源新run，不覆盖旧段；暂停窗口与失败/可能移动事实独立记录，完整边界见本文290–294节。
- `requestHistory(queryId, runFilter, afterRowId, limit, operatorConfirmed)`：当前实际 recording run 的独立历史 SQL 读事务；同时最多一个请求，直到 GUI 实际收到 pageReady 才释放槽位。上一 run、重复/过期请求和 discard-stop 已退休请求不发布。该阶段没有独立的只读离线库挂载；查询发生于显式开启的 RW 归档会话，不应称整个会话不写盘。

`operatorConfirmed` 仅表达显式配置/查看意愿，不认证账号、角色、两名操作员或企业 ACL。私有本地目录、磁盘配额、QtSql/SQLite 插件和部署权限仍由工程师配置；当前没有自动设置目录 ACL、加密、签名、恶意硬链接/文件置换完整防护或运行时 UI 配置入口。

## 原生接纳和缺口

BlackBox 在 append 后、旧 eventsChanged/eventRecorded 外部通知前提交并发布 `nativeEventAccepted`。每个实际对象有不可从文件恢复的 UUID epoch、nonwrap uint64 接纳序号及最多32个暂时在栈的帧；嵌套发布保留各自帧，scope guard 逐帧退休。clear/import/start-stop 清除在栈依据但不重置序号或把历史变成新事件。原有 setMaxEvents/importSnapshot 的通知也保留 QPointer 删除保护。

入口检查实际 QObject/同线程/固定 epoch、规范序号及本对象仍在发布的精确事件；递归比较要求同 QVariant userType/isNull、同键/元素，且有深度和节点上限，不依赖数值别名转换或自定义比较器。已知 Hash/字节/日期等 Qt 值和相同 NaN 位型可以匹配接纳身份，但仍不符合 codec 的严格 JSON 投影；自定义/对象类型不给该精确收据。默认空 source 被 BlackBox 明确表示为非 null 空字符串，payload typed-null 不补成已知0/false。

begin 冻结 `captureStartSequence`，只记录此后实际接纳窗口，不复制开库前事件。每次有效接纳先分配 `1..INT64_MAX` 归档序号再做 codec/queue：projectionRejectedCount、queueRejectedCount 不会通过“只给成功项编号”隐藏中间拒绝。来源跳序、重复/晚到/失去源、重入拒绝均保持诊断；源丢失停止后续接纳并请求排空已捕获值，不改变机队/控制权限。

sourceRejectedCount 是拒绝观察次数；sourceMissingCount 是相邻已接受序号差额。externalGapCount/droppedCount 包含明确入队丢弃及入口缺口标记，拒绝标记可能与后续跳序或晚到重叠，并非精确去重丢失事件数。未知尾部也无法从 counter 推断。gapAccountingAccepted 只说明 worker 接受了这些诊断计数，不说明它们已提交：下一笔 batch/停止摘要事务 commit 后才持久化，崩溃可能丢尾。`archiveCompletenessVerified=false` 始终保留，clean 也不证明全部源事件完整。

主线程只做有界值校验/固定摘要/哈希和短 mutex 入队；SQL I/O 全部在 worker 内。250ms 状态轮询合并通知，不为每条事件制造无界 Qt queued 队列。编码仍有有限 CPU/内存开销，不是零延迟保证，需工程师评估真实事件速率。queue + in-flight、batch、历史行数和紧凑 JSON 大小分别有界；JSON 页大小不等于整个 QVariant 堆内存的精确上限。

## 时间、隐私和历史隔离

原 event UTC、接收 UTC 及本 run 单调毫秒分开保存。墙钟相对单调增量偏离超过5秒或单调回退时，clockDiscontinuityObserved 黏性保留；历史按 append row_id，不以墙钟作为可靠全局顺序，更不推导机载时钟同源。

只持久化固定类别、诊断 ID/数值与原 type/source/payload 或任务 ID 的未验证哈希引用，不保存原自由文本、精确 GPS、完整任务、SID/凭据、视频或原始 payload。哈希不是加密/签名/匿名化，低熵值可被枚举推测；时间/车辆 ID 仍可能敏感，须部署级访问和留存政策。

历史页仅保留独立只读值。即使过去 summary.queued 或 uncertain 为 true，也不代表历史命令成功/任务完成；包裹 flightCommandReleased=false 表示归档不会下发命令，而非否认过去曾排队。新 run 清空旧页并异步通知；历史不进入 BlackBox import、TimelineReplay/rule callbacks、Registry、任务编排、审批、authority 或任何 Vehicle/MAVLink 接口。authenticated/signatureVerified/permissionsRestored/executable/executionPermit/flightCommandReleased/completionVerified 均保持 false。

## 尚未完成和工程师事项

当前已接连续诊断摘要归档、显式维护封存/新run轮换及native跨封存段浏览分页。仍缺持续完整遥测、已认证全集历史索引/自动归档清理、跨任务统计与维护集成、签名/企业权限和离线灾后恢复。未知/损坏/需 hot-journal 恢复的库拒绝，不自动删库或重建；硬退出不能保证异步线程完成或 deleteLater 被执行。完整25目标/六类工作继续，尤其最终 StartPermit、净空、执行进度/完成和跨站控制权未由此补齐。

工程师后续应验证默认off/明确consent、实际源与import隔离、嵌套/clear/源销毁、typed-null/NaN/超大payload、真实 QtSql 插件和 schema/pragmas、锁库/磁盘满/外部坏库/异常停机、queue/in-flight/单槽历史和drop计数、正常排空与discard、主库加rollback journal空间、不同页间留存变化及线程结束销毁。当前这些场景全部未执行。

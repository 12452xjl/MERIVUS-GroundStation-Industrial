# FleetEventArchiveMaintenanceWorker — 单文件归档维护与外部备份读回

## 批次 525：已核备份对应的 retired 文件显式清理

新增 `purge-preview` 与 `purge-retired`，不改变以下旧 inventory/move/backup/catalog/diagnostics/import 的请求、结果和平台域。新清理仅支持 Windows；其它平台在删除决定与收据创建之前返回明确 unsupported，不降级成路径删除、普通 remove 或其它平台的 unlink。它只处置原固定目录中的一个 retired 归档，不打开当前数据库，不清理 sealed、备份、partial 或旧收据，不恢复任务、账号、权限或执行。

### 来源与精确合同

- preview request exact3：`kind="purge-preview" / basis / operationId`；commit request exact4：`kind="purge-retired" / basis / operationId / proof`。operationId 是本次原生拥有方冻结的规范非零 UUID，不从 UI 显示的 jobId/previewId 猜出。
- basis 是完整旧 backup31；重建它原先的 backup request5，再用原 `resultMatches` 严格核验，必须 `backupVerified=true`、source descriptor7 的 kind 为 retired、原两目录 ID 非空且不同，大小仍在实际 policy 内。旧31没有原文件对象 ID；其本地字节核验不被改称原文件对象、签名或实物来源认证。实际拥有方只从原 backup 的同 worker/resultReady/actual finished 双终结保留 basis，外来 Map 通过纯值检查不创建删权。
- proof exact3：`sourceFileIdentity / backupFileIdentity / backupDescriptor`。前两者为本次 Windows 句柄实际信息中的 `windows-volume-file-index:<volumeSerial8hex>:<fileIndexHigh8hex>:<fileIndexLow8hex>`，小写且补足前导零，必须不同。backupDescriptor 是原完整8键，使用本次备份文件自己的 mtime；其 basename/第三 UUID、epoch/run/bytes/SHA 必须分别对应 basis 的目标名、旧备份 operationId 和 source 内容。文件 ID 可复用，不是永久身份，也不证明不同物理介质。
- result exact41 = common12 加以下29键：`format / schemaVersion / kind / operationId / basis / proof / previewVerified / observedDirectoryIdentity / destinationDirectoryIdentity / nativeDeleteSupported / decisionFinal / deleteAttempted / deleteReportedSuccess / mayHaveDeleted / sourceAbsentObserved / backupAfterVerified / sourceHandleClosed / backupHandleClosed / cancelObserved / errorCode / diskSpaceReclaimedVerified / sourceLogicalBytes / receiptBasename / receiptCreateAttempted / receiptCreated / intentWritten / resultWritten / receiptClosed / receiptError`。format 固定 `merivus-fleet-archive-retired-purge`，schemaVersion 为 Qt Int1；所有事实位是实际非 null Bool，其余字符串均非 null QString，basis/proof 是实际非 null Map。
- sourceLogicalBytes 是 basis.source.fileBytes 的原规范十进制字符串，只表示逻辑文件大小；`diskSpaceReclaimedVerified=false` 恒定，不能据此显示“已释放这些物理字节”。尚未观察到的两目录 ID 是空串；通过前置核验时必须对应 basis 原 ID，失败结果可以保留本次实际观察到的不匹配 ID 作诊断。preview 失败 proof 为空 Map，成功才完整3；commit 无论是否入场成功都保留请求的原 proof3，不生成替代删权。
- `decisionFinal == deleteAttempted == mayHaveDeleted`；deleteReportedSuccess 蕴含 attempted，只记录 `SetFileInformationByHandle` 的返回值。sourceAbsentObserved 和 backupAfterVerified 分别是删除决定后的只读观察，不能从 reportedSuccess 推导。close 两事实各自独立，false 表示未确认关闭，不是确认句柄从未打开。
- previewVerified：preview 必须全部前置/原目录尾核及两原生句柄关闭均无错，且无取消，才 true 并给 proof。commit 指本次原对象/字节检查通过且最终两句柄都确认关闭；最终 close 失败可以使它 false 而 deleteAttempted 仍 true。因此不要求 `decisionFinal => previewVerified`，更不能因它 false 抹掉已经发生的删除尝试。commit 成功的空 errorCode 还必须实际 reportedSuccess、正常后核确认源缺失、备份后核、两句柄关闭和完整收据结果/关闭全部成立；否则给固定错误并保各自事实。
- current exact24 = common12 + `requestId / state / errorCode / cancelRequested / decisionFinal / deleteAttempted / deleteReportedSuccess / mayHaveDeleted / sourceAbsentObserved / backupAfterVerified / sourceHandleClosed / backupHandleClosed`。state 为 created/opening/inspecting/deleting/confirming/result-submitted；它不是终态41结果，也不带收据的全量诊断。旧 current19/23/27 不改。
- 结果仍受原 `min(2 MiB - 2048, maximumHistoryMiB × MiB - 2048)` 预算。有效新操作的所有终态使用 resultReady41，不因已经尝试删除或创建收据后的错误改发“无事实”的 failure；实际拥有方仍须等 QThread::finished。纯值匹配不接触磁盘、不认证 Map 的实际来源。

### 后台检查和同句柄删除

1. 固定 ctor `_directory` 为原源，目的目录仅来自原 basis；两路径沿原 canonical 本地非根、无链接规则，文本上互不相等/包含，真实目录 ID 也不同。不 mkdir。先原 `fleet-events-v1.sqlite.owner.lock`，再备份目录 `fleet-events-backup.owner.lock`，同旧 nonblocking/大小/类型路径规则；合作锁不防恶意进程。
2. 原 sealed/retired 双 namespace 和备份 namespace 均使用完整、正常 EOF 的有界清单。新清理使用严格 backupListing，任何入选的异常名字/类型/sidecar 都拒绝，而不是从诊断 readable 前缀选出成功；仍是每 namespace 128、全目录4096的旧边界。选中 retired 必须仍在原清单；两完整目录身份对应旧 basis。
3. **先**用现 inspectFile 完成选中源和备份的完整 hash、SQLite header/schema/meta/seal clean、二次 hash/metadata。源 descriptor7 与旧 basis 全等，备份完整8与原内容对应。QSqlDatabase::close 的 void 返回不被新解释成底层关闭认证。
4. 再打开源 READ（preview）或 READ|DELETE（commit）、备份 READ 的 Windows 固定句柄，均 FILE_SHARE_READ、OPEN_EXISTING、OPEN_REPARSE_POINT，拒目录/reparse/非 disk 文件。SQL/QFile 检查已结束，不能在持 DELETE handle 时假设 Qt SQLite 可重新打开。由各自原句柄读取实际 volume/index、size/mtime，并从同句柄重置位置完整 SHA/明确 EOF/前后句柄信息核对应原已验证字节。相同对象 ID 拒绝；commit 必须完整对应原 proof，不能只比较路径或 SHA。
5. 收据意图写完后，重新正常两目录全表/原 ID/metadata 和两原句柄 SHA/ID，再在短 mutex 内最后检查取消并提交 decision/attempt/may/state。释放 mutex 后才调用原源句柄 `SetFileInformationByHandle(FileDispositionInfo)`；短锁记录返回值。GUI current/cancel 的 mutex 不跨原生删除 IO。取消在决定前可阻止删除，决定后只能记事实，不回滚、不重试、不再打开另一个路径执行删除。
6. 明确观察原 source CloseHandle；失败即未知，不重试可能不再属于自己的 handle 值。源缺失必须两次原目录正常 EOF、原目录 ID、原完整清单**只少这一个源**且其它 bytes/mtime 均保持；QFileInfo 的不存在值仅补名/sidecar检查，不能独立作为删除证明。保留备份原句柄，在后核时完整同句柄 SHA/ID/size/mtime与两次备份清单仍原样，然后观察 backup CloseHandle。两个目录并非跨目录原子快照。

### 不覆盖的本地意图与结果收据

preview 不写收据。commit 固定原源目录下 `fleet-events-purge-<operationId>.jsonl`，仅 `QFile NewOnly|WriteOnly|Unbuffered`，不覆盖、删除、截断、自动续写或重试。文件名非空不表示文件已创建。receiptCreateAttempted 表示可能已留下文件或部分字节；receiptCreated 只表示本次 open 成功。创建失败/短写/取消遗留均保留，后续不能从名字恢复许可。

每份最多两行 NDJSON：一行 intent、一行 result；每行含换行≤64 KiB、总≤128 KiB，不截断。intent 保完整原 basis/proof/operationId/固定源目录和两实际路径，不含账号；完整一次 write 接纳及 flush 成功后才 intentWritten=true，之前绝不删除。短写不重试；flush 失败也不伪装完整意图。result 行保真实删除/目录/备份/句柄观察以及当时完整41事实；它写入前自己的 resultWritten/receiptClosed 仍 false，不能自证自身持久化。返回值中的 resultWritten 表示这行完整 write+flush，receiptClosed 仅本次实际 close 后 Qt 无 error。

receiptError 独立固定码：`archive-purge-receipt-create-unconfirmed / archive-purge-receipt-write-failed / archive-purge-receipt-flush-failed / archive-purge-receipt-close-unconfirmed / archive-purge-receipt-budget-rejected` 或空；errorCode 保首个业务/收据错误。终态收据失败仍保已删除/可能删除事实，不能改成未发送/已撤回。flush/close 不是 fsync、目录同步、断电耐久性或签名；本地收据不是备份恢复许可或完整历史全集。新增 artifact 仍参与全目录原4096边界，没有无限自动清理。

### 未证明与工程师待验证

只做了源码/直接文本静读；没有编译、测试、执行 IO/线程/SQL/hash/helper/项目/UI 或真正删除文件。待验证包括：原实际 backup 双终结→retired31→preview→30秒原生一次性proof→commit；非 Windows 写前拒绝；0/128/4096名单边界及异常末项；两目录别名/ID、hardlink同对象、reparse、大小/mtime/SHA变化、fileID复用；Windows sharing/DELETE权限/杀毒与其它句柄、只读属性、实际文件系统的 FileDispositionInfo/CloseHandle/namespace缺失行为；取消在意图前/flush后/决定前/决定后；NewOnly同名、部分/短写、磁盘满、flush/close失败与意图后检查失败；删除报告失败/缺失未知/备份后核变化、两handle关闭失败；result41/current24/receipt7跨端和真正 finished。备份字节仍可读不等于已测试恢复，逻辑大小不等于物理释放；文件hash、对象ID、路径/合作锁不防恶意ABA/别名/挂载/非合作进程，也不认证原事件、任务完成或任何权限。

实现参考（文档阅读不是 Windows API 执行证据）：[CreateFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew)、[GetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle)、[SetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle)。

## 批次 466：显式历史导入，先暂存核验再无覆盖发布

新增 `import` 将**当前选中的一个 readable backup** 复制到构造时固定的本机归档目录，暂存文件完整读回后再用已有 native no-replace rename 发布为 retired 历史。不是把备份当当前库，不恢复原任务、账号、权限、执行状态或历史成功收据，也不自动进入普通 sealed 检索。是否随后 restore 到 sealed 必须是另一次原维护审阅；本次不串联恢复或放宽旧流程。

### 冻结合同

- request exact5：`kind="import" / source / sourceDirectory / expectedDirectoryIdentity / operationId`。source 是原诊断 readable 的 backup descriptor8，sourceDirectory 为原样严格 destinationSyntax 路径，expectedDirectoryIdentity 为该备份目录原观察 ID。operationId 为原本次预览 UUID。构造 `_directory` 是固定本机导入目标，request 不能替换；source 的 backupOperationId 是旧副本第三 UUID，不与本次 operationId 混淆。
- stagingBasename 严格 `fleet-events-backup-<epoch>-<sealrun>-<operationId>.sqlite`；targetBasename 严格 `fleet-events-retired-<epoch>-<sealrun>.sqlite`。暂存沿既有 backup 名字/受约束 inspect，不放宽旧 sealed/retired parseName，也不使用普通目标名直接写入。
- result exact38 = common12 + `format="merivus-fleet-archive-backup-import" / schemaVersion=Int1 / kind / operationId / source / sourceDirectory / expectedDirectoryIdentity / observedDirectoryIdentity / destinationDirectory / destinationDirectoryIdentity / stagingBasename / targetBasename / targetDescriptor / decisionFinal / stagingCreateAttempted / stagingCreated / mayHaveWritten / bytesCopied / copyCompleted / stagingVerified / publishAttempted / publishReportedSuccess / mayHavePublished / importVerified / cancelObserved / errorCode`。destinationDirectory 由 worker 填原 `_directory`；纯 resultMatches 没有 ctor 上下文，实际 Store 必须再核它逐值等于原固定 invocationDirectory。
- targetDescriptor 仅 importVerified 时为完整 retired descriptor7，否则必须空 Map；核新名字/kind、epoch/run/bytes/SHA 与原 source8 对应，新 mtime 可以不同。两个 observed directoryIdentity 以本次原生值填入，尚未观察时是非 null 空 QString；完整 source 与请求原值保留，不以新目标冒充原审批依据。
- `decisionFinal == stagingCreateAttempted == mayHaveWritten`；stagingCreated 蕴含 attempted，正 bytesCopied 蕴含 created，bytes 是规范十进制 QString且≤source.fileBytes；copyCompleted 蕴含 created、全部字节/EOF/流 SHA 成立。它仍早于 flush/close，不等于耐久性。stagingVerified 蕴含 complete 且原复制/flush/源目标 close、stage 读回及源/目标当前检查均无错误。
- `publishAttempted == mayHavePublished` 且蕴含 stagingVerified；publishReportedSuccess 只记录 native 返回值，蕴含 attempted；importVerified 蕴含 attempted、stagingVerified、完整对应 targetDescriptor7 和非 null 空 errorCode。native 返回 false 但完整后核实际确认时允许 verified；晚到取消也不抹已发布事实。未 verified 必须固定非空 errorCode，并保持空 targetDescriptor。
- current exact27 = common12 + `requestId / state / errorCode / cancelRequested / decisionFinal / stagingCreateAttempted / stagingCreated / mayHaveWritten / bytesCopied / copyCompleted / stagingVerified / publishAttempted / publishReportedSuccess / mayHavePublished / importVerified`。states 为 created/opening/inspecting/copying/verifying/publishing/result-submitted，失败/取消事实由终态结果保留；不混旧 move/copy 字段。
- 所有 valid import 终态 resultReady38，绝不通过 failure 丢弃 possible write/publish。完整 JSON 预算和 strict 类型、Map、UUID、命名、所有关系由原 pure requestValid/resultMatches 核；旧 catalog21、diagnostics23、copy31、move27、inventory19、preview/旧 current 均保持。

### 实际后台流程

1. 目标 `_directory` 和 sourceDirectory 必须是既有 canonical 本地非根目录，互不相同、互不包含，原生对象 ID 也必须不同；Windows 文本大小写不敏感，其余平台按当前词法/原生观察边界。不 mkdir，不打开当前库。nativeRenameSupported 只检查编译平台/头文件分支是否提供现有 no-replace primitive；该分支不存在时在写前拒绝。Linux 内核或实际文件系统是否支持 RENAME_NOREPLACE，可能到真实 publish syscall 才能观察失败，此时已写 stage 必须保留，不能宣称所有 unsupported 都在写前识别。
2. 固定顺序先取得目标 `fleet-events-v1.sqlite.owner.lock`，再取得来源 `fleet-events-backup.owner.lock`，均沿原路径/大小/类型检查和 nonblocking 合作锁；持有到全部后核结束。不等待、倒序重试、偷换 live owner 或把锁当恶意进程防护。
3. 来源用 EventBackups **raw** scan，允许目录有其他 rejected 候选，不使用“所有副本都必须好”的 strict backupListing。只选中原 source8 必须 regular/无链接并由完整 hash/只读 SQLite/double SHA 形成同一 descriptor8。首轮、复制前及最终的 raw 名单都必须正常 EOF、同目录 ID、同全体名字和原生类型；不打开其他候选 payload，也不借其他文件给本文件补身份。
4. 目标沿原 sealed+retired listing；retired 必须<128；目标 retired 名、其对应 sealed sibling 和 staging 都不存在，包含 sidecar 缺失。对应 epoch/run 已在任一普通 namespace 中就拒绝，不覆盖、不自动去重或替代。前置清单/ID需稳定。暂存 prefix 不在 sealed/retired listing 中，失败 partial 不会自动毒化原维护清单或伪装正式 retired。
5. 最终创建决定前合作取消可阻止目标写入。之后 NewOnly|WriteOnly|Unbuffered 创建新 stage，用原 64 KiB/短 write 循环复制，计真实接纳字节；copy 中取消停止写入，flush/close 后保留 partial，所有创建尝试都保 mayHaveWritten。不删除、resize、覆盖、自动回滚或重试。
6. 完整 copy 即使晚到取消仍完整读回 stage：header/SQLite epoch+seal clean/double SHA，stage bytes/hash 与 source 对应、使用 stage 自己的实际 mtime；来源重新完整核原8，目标清单/ID仍原样且 target/sibling absent。原复制/flush/任一 close 失败绝不 stagingVerified，虽然 copyCompleted 可能 true。
7. 只有 stagingVerified 后，mutex 中再次检查取消；已观察取消则不 publish，保留完整 stage/原事实并回 cancelled 结果。未取消才设 publishAttempted/mayHavePublished，调用已有 Windows MoveFileExW flags0 或 Linux renameat2 NOREPLACE，把 stage 发布为 retired。不使用 QFile rename/copy/remove fallback、普通可覆盖 rename 或自动撤销。
8. native 返回值或之后取消都不能代替后核：stage 及其 sidecar 缺失；目标 retired7 实际完整 hash/SQL 与 source8 bytes/hash/epoch/run 对应；目标旧清单**只新增这一 target**（原 sealed/retired顺序及 metadata 均保持）；来源 raw 全名单和选中 source8 仍对应。全部成立才 importVerified 并发布完整 targetDescriptor7。返回 false 但后核确认实际发布，也保留 reported=false/verified=true 两种不同事实。
9. 未确认时保 mayHavePublished、stage/target 名和已写字节，不自动移动回去、删除、补成功、自动重试或恢复账号/任务。结果提交后仍等 actual QThread::finished，不能提前复用原维护单槽。

### 未证明与工程师待办

导入验证仅本次字节与数据库身份相符，不证明最初原机来源/签名、全部事件 hash、全历史完整、fsync/目录同步、断电耐久性或业务成功。锁/路径/ID/metadata/SHA 仍不能保证恶意 ABA/TOCTOU、非合作进程、挂载/别名、ACL 或底层文件系统语义；partial/stage 名本身不是可恢复许可，retired 历史也不是当前执行状态。

本批未编译、测试、执行 helper/parser/业务脚本、SQL/hash/scan/线程/UI/应用或真实归档操作。工程师需验证：实际 diag readable8→预览→提交绑定；fixed target 与 source 祖孙/别名/native ID，锁顺序/冲突/ACL；缺编译分支时写前拒绝与 Linux 内核/文件系统运行期不支持的 publish 失败区别；target/sealed sibling/stage/sidecars 已存在；128 retired容量、全 raw 非选中坏文件保留且不打开；source descriptor/原名单/target清单变化；NewOnly/短写/磁盘满/读写/flush/源目标 close；取消在创建前、copy中、stage核后publish前及native调用后；late cancel和reported=false后核成立；失败保留partial或完整stage，publish不确定无rollback；结果38/current27/空 Map/非null空串/预算和实际 finished生命周期。以上均未执行，不是可恢复、实操或放行通过证据。

## 批次 465：完整候选列表的逐文件诊断

新增独立后台 `backup-diagnostics`，使用户能看见备份 namespace 中当前拒收的候选及原因；它不放宽原 strict `backup-catalog`，后者仍有任一坏项就整份失败。这里只诊断用户显式选择的目录，不隐式访问 ctor 的原源目录、不取源 owner 锁或打开当前库；允许用户显式把 destination 选为同一物理目录，不能把“不隐式访问”误解成已禁止该选择。

### 精确值合同

- request exact2：`{kind:"backup-diagnostics", destinationDirectory:<原样规范路径>}`，沿原严格路径词法；current 保持旧 exact19，只处于 created/opening/inspecting/result-submitted/failed/cancelled，decisionFinal/mayHaveMoved/moveConfirmed 全 false。原 catalog2/21、inventory19、move27、backup31 与各旧 current 不改。
- result exact23 = common12 + `format="merivus-fleet-archive-backup-diagnostics" / schemaVersion=Int1 / destinationDirectory / directoryIdentity / entries / candidateCount / readableCount / rejectedCount / enumerationCompleted=true / originalSourceVerified=false / backupSetCompletenessVerified=false`。计数严格 Qt Int 0–128，candidateCount 必须等于完整 rows 数及 readableCount+rejectedCount，路径逐值对应原请求。
- 每行 exact4：`{basename, status, errorCode, descriptor}`，descriptor 是 Map。basename 是本次原生 namespace 中真实候选名，严格有序且唯一；非 null QString 1–512、有效 UTF-16、无 NUL 或 `/`。POSIX 的原始反斜杠可以被原样表示，随后作为坏名拒收，绝不按它打开 payload。超过有界表示、不可逆/非法编码或整份字节预算不满足，整体 failure，不截断名字、不丢后缀或只发前缀。
- readable 行：status=`"readable"`，errorCode 为非 null 空串，descriptor 是原 backup descriptor8，basename 与其中名字一致且完整三 UUID 对应。SHA/bytes/mtime 来自本次文件检查，仍没有原始 source 或旧成功收据认证。
- rejected 行：status=`"rejected"`，descriptor 必须是空 Map，固定非空 errorCode 只能为 `archive-backup-name-invalid / archive-backup-type-rejected / archive-backup-size-rejected / archive-backup-sidecars-present / archive-backup-file-rejected / archive-backup-structure-unconfirmed`。拒收表示本次未接纳，不是证明文件永久损坏、恶意、从未有效或可删除。
- 原完整结果预算 `min(2 MiB - 2048, maximumHistoryMiB × MiB - 2048)` 保持；resultMatches 独立核所有行的精确字段/类型、namespace 前缀、完整 descriptor8/理由枚举、升序唯一、计数、原 path、两个 false 和全部 common 标志，不接纳成功前缀。

### 后台 IO 与全局失败边界

1. 只在目的地既有 canonical 本地非根目录取得同一个 `fleet-events-backup.owner.lock` nonblocking 合作锁，再直接进行 EventBackups **raw** namespace 扫描。候选最多 128，所有目录 entry 仍计 4096；必须明确正常 EOF 和原生句柄关闭，不能使用会提前过滤坏项的 strict backupListing。没有 mkdir、写副本、改名、删除、修复、自动重试或恢复。
2. 遍历全部 raw 候选，先核名字可表示；不符合三 UUID 完整名字给 name-invalid，不打开其 payload。名字合规但原生 symlink/非 regular 给 type-rejected，也不打开。规范 regular 按当前文件存在/类型、大小、sidecar、safeFile 分别给 file/size/sidecars 理由；其中 0 字节 partial 也保留为 size-rejected 行，而不是被过滤成不存在。
3. 其余文件经同一受约束 inspectFile：完整 hash/SQLite header/meta epoch+seal clean/二次 hash。普通内容或结构未通过为 structure-unconfirmed，清空 descriptor；通过才建立完整 readable descriptor8。helper 的可选 fileCloseFailed 输出只用于本新诊断，原调用默认合同不改；hash QFile 的显式 close 和早退清理都观察 Qt error，若观察到文件 close 失败则整份 failure，不能只给一行 rejected。QSqlDatabase::close 是 void，关闭/移除连接不构成底层 SQL close 成功的新证明。
4. 取消在每行、每个 hash 读取块和最终发布前检查。hash 中取消或已请求取消一律全局失败/取消，不能转成 structure-unconfirmed 并继续发“其余已成功”。原生目录错误/close、目录对象变化、完整候选名字/regular/symlink 集变化也必须整体 failure，无任何 entries 前缀。
5. 完成后再次 raw namespace 清点，同原目录 ID、同完整有序候选及类型；随后对所有 readable 行补最终 safeFile/bytes/mtime 与该行 descriptor 对应，变化使整份失败。rejected 行不含可信 metadata，也不要求其内容/mtime 静止；成功诊断不是跨文件原子快照。所有条件及完整 DTO 通过才一次 resultReady23，actual finished 后才能释放生命周期占位。

### 待验证与未证明

只读诊断不认证原 source、旧成功备份、全集、签名、每条事件 hash、权限、持久化或可恢复执行状态；也不能据 readable/rejected 数量给业务成功率或自动清理建议。路径/对象 ID/metadata/SHA 仍受前述恶意 ABA、TOCTOU、链接别名、非合作进程和真实文件系统语义边界限制。新旧诊断与严格 catalog 是不同请求，不能拿新诊断中的 readable 前缀恢复旧 catalog 成功。

本批没有编译、测试、执行 helper/parser/业务脚本、启动线程/UI/应用、执行 SQL/hash、扫描目录或实际读写副本。工程师待做：0/128/129 与 bad-last 全候选，混合 readable/坏名/目录/symlink/sidecar/0bytes partial、POSIX 反斜杠/Unicode/512/513；每个冻结 reason、empty Map/empty QString 与严格 row/全计数；原生 EOF/error/close、hash read/close 错误、SQL 未确认、取消各阶段；第二次 name/type/ID 变化、readable 最终 bytes/mtime 变化与 rejected metadata 可变的区分；原 source 缺失/显式同路径、预算失败无前缀、SDK/MOC 与实际 finished。所有这些是未执行事项，不是运行或数据恢复通过证据。

## 批次 464：不依赖原源目录的独立备份清点

`backup-catalog` 真实读取一个既有规范备份目录。即使原 source 目录、原库或原清单都已不存在，也可独立检查现有副本；不能从副本恢复数据、任务、权限、备份成功收据或原始 hash。旧 inventory19、move27、backup31、旧 current19 与 backup current23 的字段和行为均保留。

### 契约与来源边界

- 请求 exact2：`{kind:"backup-catalog", destinationDirectory:<原样规范路径>}`。路径沿批 463 的完整纯词法校验；不增加 GUI 文件/目录查询。ctor 中的 `_directory` 不参与该分支有效性判断，不隐式用它清点或取得源 owner 锁／打开当前库；读取范围只由用户明确指定的 destinationDirectory 决定，若两路径相同也不改写用户选择。
- 结果 exact21 = 原 common12 + `format="merivus-fleet-archive-backup-catalog" / schemaVersion=Int1 / destinationDirectory / directoryIdentity / entries / fileCount / enumerationCompleted=true / originalSourceVerified=false / backupSetCompletenessVerified=false`。destinationDirectory 必须逐值对应原请求，directoryIdentity 是本次观察到的原生目录对象值。
- 每个 entry 是 exact8：`basename / kind="backup" / databaseEpoch / sealRunId / fileBytes / fileFingerprint / modifiedUtcMs / backupOperationId`，全部为非 null QString。三个 UUID 必须完整对应 `fleet-events-backup-<epoch>-<sealrun>-<backupOperationId>.sqlite`；fileBytes 与 modifiedUtcMs 是规范正十进制（bytes ≥512），fingerprint 是本次完整读取算得的小写 SHA-256。它**不是**从旧 source 或旧成功备份结果取回的认证 hash。
- entries 必须严格按 basename 升序且唯一，fileCount 严格 Qt Int 0–128 并与全部 entries 数一致，每个文件不超过原 policy 的 maximumDatabaseMiB。整个结果沿原 `min(2 MiB - 2048, maximumHistoryMiB × MiB - 2048)` 预算。`resultMatches` 完整核验所有 entry，不接受合法前缀，也不会把 originalSourceVerified 或 backupSetCompletenessVerified 改为 true。
- current 保持旧 exact19：common12 + requestId/state/errorCode/decisionFinal/mayHaveMoved/moveConfirmed/cancelRequested；三项移动事实始终 false。state 为 created/opening/inspecting/result-submitted，失败/取消分别 failed/cancelled。结果或失败信号都不能提前释放实际 QThread::finished 生命周期占位。

### 真实读取流程

run 的第一分支先进入 runBackupCatalog，不走源目录 localDirectory 检查、源 owner 锁或当前库路径。后台仅检查目的地确实是既有 canonical 本地非根目录且无 symlink；同样受 Windows 非远程盘/非 reparse 与 POSIX 原生路径边界限制。唯一可能创建/释放的是目的地固定 `fleet-events-backup.owner.lock` 合作锁，取得它之后才首轮清点；不 mkdir、不写副本、不改名、不恢复、不修库。该目录锁也不构成恶意进程防篡改或只读磁盘权限承诺。

新增原生 `Namespace::EventBackups` 前缀固定 `fleet-events-backup-`，原前三个 enum 的数值、前缀与行为保持。所有非 dot entry 仍计入 4096 上限；备份前缀以大小写不敏感方式入选，最多 128，不按后缀偷偷过滤。消费者严格检查完整三 UUID 名字、普通文件、无链接、canonical path、三类 sidecar 缺失、真实 bytes/mtime。因此 uppercase 变体、sidecar、坏末项、目录、链接、部分副本、超限或枚举失败不能被当作“没有这个文件”而发成功清单。

每文件使用原显式 backup=true 的受约束 helper：100 字节 SQLite/header/application/schema/page/counter 核验，64 KiB 有界块全文件 SHA，独立 QSQLITE_OPEN_READONLY 连接与只读事务内真实 meta epoch/指定 seal run clean 身份，结束读视图、关闭/移除 SQL 连接，再全文件 SHA/metadata 核一致。旧 parseName 没有放宽；SQL 身份只核 epoch/sealrun，文件名第三 UUID 不是 DB 内操作收据认证。

全部文件完成后再完整 EventBackups 原生清点；前后同目录对象、同有序 basename/bytes/mtime，且原生明确 EOF/句柄关闭检查通过、未取消，才一次 resultReady21。任何异常整体 failure 固定错误码，无 entries 成功前缀。取消在原生调用之间与每个 hash 读取块检查，不能保证中断正在执行的 SQL/系统调用或实时截止。成功空目录必须有两次正常完整清点，不以枚举错误当空。

### 不证明与待验证

本次 hash/SQLite 自洽检查不能证明与最初源文件一致、每条事件内容 hash 正确、签名、数据全集、备份集完整、fsync/断电持久化或可恢复执行状态。损坏或截断的保留 partial 副本会使检查失败且整份清单失败，不自动删除或修复；反之，某个旧未确认副本若现今字节及 DB 自洽，也不能据此补造最初 source 对应或原备份成功收据。路径 canonical/native ID 只是可观察检查，不保证恶意 ABA、硬链接、重用 ID、不同大小写卷别名、挂载变化或非合作进程安全。既有只读目标文件的 kernel/SQLite 检查也不是完整应用数据恢复验证。

本批没有编译、测试、解析/运行项目 helper/业务脚本、启动线程/UI/应用、执行 SQL、目录扫描或实际读写副本。待工程师验证：原源目录缺失而备份目录仍可独立读取；empty/128/129、bad-last、大小写变体/sidecar/目录/链接/partial、实际 policy 大小和正 mtime 边界；枚举 EOF/error/close、锁冲突与 ACL；header/schema/epoch/seal/重复或坏 UUID、首后文件变化、SHA/mtime 变化、取消时序、结果字节预算、strict 全 entry 与新旧 DTO；所有失败不发布前缀、真实 finished 后退槽，以及目标 SDK/Qt/SQLite 与实际文件系统行为。以上均为未执行事项。没有新增 SDK 依赖或依据来源。

## 批次 463：真实单文件复制与读回验证

本批在相同单任务 parentless QThread、固定源目录、实际 Store 私有来源和 finished 生命周期上增加 `backup`。原 inventory19、move27、旧 current19 和原 sealed/retired 完整名字规则保持不变；新 backup 不是恢复任务、签名备份、全历史全集或断电恢复工具。源文件可以是 sealed 或 retired，但不打开当前库，不移动源，不修改源内容。

### 精确契约

- 请求 exact5：`kind="backup" / source / expectedDirectoryIdentity / operationId / destinationDirectory`。source 是原库存中完整 descriptor7，不是公共界面传回的文件路径；operationId 由 Store 在 prepare 时生成并冻结为原 previewId。destinationDirectory 是原样确认的既有规范目录：非 null QString 1–4096、无 NUL/孤立 surrogate、无首尾空白、只用规范正斜线、绝对且 cleanPath 与原文相同、非根、非 UNC/device/Windows ADS。`requestValid` 对这些只做纯值/词法核验，不在 GUI 上查目录。
- target 只由原 source 的两个 UUID 和原 operationId 推导：`fleet-events-backup-<epoch>-<sealrun>-<operationId>.sqlite`，没有调用方可指定的 basename。旧 parseName 仍只认 sealed/retired；新增 private parseBackupName 精确检查三个规范 UUID。只有目标读回 helper 明确传 `backup=true` 才可打开这个名字，默认旧路径不能打开 backup 或任意数据库。
- backup result exact31 = common12 + 19 项：`format="merivus-fleet-archive-maintenance-backup" / schemaVersion=1 / kind / operationId / source / targetBasename / expectedDirectoryIdentity / observedDirectoryIdentity / destinationDirectory / destinationDirectoryIdentity / decisionFinal / targetCreateAttempted / targetCreated / mayHaveWritten / bytesCopied / copyCompleted / backupVerified / cancelObserved / errorCode`。
- 两个 observed/native directoryIdentity 是本次真实目录观察值，未完成检查时为空的非 null QString；destinationDirectory 保留请求原文，source/operation/expectedSourceId 也必须逐值对应原请求。bytesCopied 是规范十进制 QString，可以为 `"0"`，只数实际 write 返回已接纳的字节，不是持久化字节或完整文件证明。
- `decisionFinal == targetCreateAttempted == mayHaveWritten`；`targetCreated` 只在 NewOnly open 返回成功后为真，created 蕴含 attempted；正 bytes 蕴含 created，bytes 不大于原文件；copyCompleted 蕴含 created、bytes 等于原 source 字节并完成读源 EOF/完整复制流 hash 对应。**copyCompleted 在 flush/close 前记录，只是全部字节的读取/write 接纳及流 hash 完成；后续 flush 或任一源/目标 close 失败时它仍可能为 true，而 backupVerified 必须为 false。** backupVerified 蕴含 copyCompleted、完整后核通过且 errorCode 是非 null 空串。未 verified 必须有固定非空错误码。create 失败也可能留下文件，故 attempted 后 mayHaveWritten 不被改回 false；不据 NewOnly 返回失败宣称没有创建。
- backup `current()` exact23 = common12 + `requestId / state / errorCode / decisionFinal / targetCreateAttempted / targetCreated / mayHaveWritten / bytesCopied / copyCompleted / backupVerified / cancelRequested`。状态 created、opening、inspecting、copying、verifying、result-submitted。每次短 write 后更新有界事实，由现有定时观察合并通知，不从复制循环调用 UI。旧操作仍是原 exact19。
- valid backup 所有终态都 resultReady31，不用 failure 信号丢失创建或部分写入事实。完整结果仍受原 `min(2 MiB - 2048, maximumHistoryMiB × MiB - 2048)` 限制。common12 的执行、认证、签名、完整记录 hash 与完整性标志仍全部为 false；真实写盘不标 readOnly。

### 真正复制与检查顺序

1. 在后台核既有源和目的地 canonical path/类型/可读/无 symlink，目的地非根，且与源既不相同也不互为祖先/子目录；Windows 文本比较大小写不敏感，其他平台按大小写敏感文本关系并再核真实目录对象 ID。目录别名、非合作 ABA 和恶意挂载不是由文本比较认证的。原生扫描保留 Windows 本地盘/非 reparse 检查及 POSIX 可逆路径/原生句柄/EOF 错误边界。
2. 持原 `fleet-events-v1.sqlite.owner.lock` 和目的地固定 `fleet-events-backup.owner.lock` 两把 nonblocking 合作锁；锁路径沿原检查，不能异常类型、symlink、超 16 KiB 或 canonical path 不一致。两锁都在各目录首扫描前取得，覆盖后续全部 IO；不 mkdir，不打开当前库。不能拿到第二把锁就整体退出，不等待或倒序重试。
3. 源双 namespace 原生 inventory 必须同原 expectedDirectoryIdentity，且 source 原 bytes/mtime 仍在原清单；目的地进行共享 native scanner 的全目录 error-aware EOF/目录对象观察并冻结 ID，拒与源同对象。该目的地观察只取目录对象，不是 backup 全集目录，不从目的地旧文件恢复任何身份；全目录 4096 bound 和入选 sealed namespace 的旧 128 bound 保持。
4. 原 source 完整文件 SHA/100 字节 SQLite/header/application/schema/真实 meta epoch 和 seal clean 身份、二次 hash 必须与原 descriptor7 全等；再次源清点及目的地 native ID 核对。推导的目标名及其 journal/wal/shm 必须不存在。预览只是原路径的值确认，不宣称 prepare 时已检查目的地物理对象；目标实际物理 ID 在本次 worker 冻结。
5. 最终创建决定前取消可阻止所有目标创建。之后调用 `QFile::open(WriteOnly | NewOnly | Unbuffered)`，不用 QFile::copy、QSaveFile、resize、rename 或任何删除/自动回滚。Qt 5.15.2 native Windows 对 NewOnly 使用 CREATE_NEW；Unix 使用 O_CREAT|O_EXCL。目标已存在不能覆盖；不能先 open 普通 WriteOnly 再补存在性判断。
6. 每次只读最多 64 KiB；每块按实际返回值循环短 write，0、负数或超出剩余长度整次失败，不假定一次 write 全写。复制中合作取消停止新增写入，flush/close 后保留已有部分文件。完整 source 字节读完还检查明确 EOF/error、原大小及复制流 SHA 等于原 source；只有这些满足才 copyCompleted。flush 成败独立检查，close 是 void，随后检查 error，先前失败不被 close 可能清除的错误覆盖。flush 不是 fsync/FlushFileBuffers/目录同步证明。
7. **完整复制后**无论晚到取消都继续只读核验：目标实际 mtime 单独捕获，可以与源不同；新名字严格校验后走同一文件完整 hash、真实只读 SQL seal 身份、二次 hash，目标 bytes/SHA/epoch/sealrun 对应原 source，而不是要求目标 basename/mtime 等于源。目的地 native ID 前后仍为原冻结对象。最后重新完整 inspect 源 descriptor7 并清点同原源清单；全部通过且没有 flush/close 等错误才 backupVerified。
8. 部分、未知、flush/close 失败或读回未确认一律不删除新文件、不自动重试、不覆盖，不回滚源。late cancel 可以与 backupVerified=true 同时存在，表示文件已核且取消晚到，不是“已撤销备份”。任何创建尝试后 mayHaveWritten=true 的终态都必须让工程师明确看到原目标；后续只能先独立检查再决定处置。

### 未证明与待工程师验证

备份验证只证明本次观察到的新文件与原封存文件字节及 DB 身份对应；不证明所有事件 payload hash、签名、权限、真实性能、任务结果、全历史或恢复可执行状态。不提供磁盘写入耐久性、断电后完整性、ACL 防篡改、恶意路径/硬链接/ABA/TOCTOU 防护；目的地名称空间也不是签名备份库或跨进程恶意写保护。POSIX 本地路径不证明没有网络挂载，目标卷实际持久性/剩余空间未认证。

本批未编译、测试、执行项目 helper/parser/业务脚本、线程、SQL、目录清点、复制、UI、应用或设备。工程师待验证：目标 Qt 5.15 NewOnly/Unbuffered/短 write/flush/close 行为；MOC/生命周期与实际 finished；拒根/UNC/device/ADS/目录上下级/大小写/Unicode/链接/alias；锁冲突与失败；目标同名/sidecar 已存在竞态；disk-full/short-write/0-write/read error/取消在创建前、块中及完整复制后；保留 0 字节/部分文件；目标 mtime 不同但有效与 SHA/epoch/run 不同的拒绝；源/目录代次变化、完整 readback、断电及真实文件系统差异。以上不是已执行证据。

静态 SDK 依据：[Qt 5.15.2 QIODevice enum](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/io/qiodevice.h)、[Windows native open](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/io/qfsfileengine_win.cpp)、[Unix native open](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/io/qfsfileengine_unix.cpp)、[QFileDevice flush/close](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/io/qfiledevice.cpp)。这些源码核对不是目标 SDK 构建或文件系统实测。

## 批次 462 原有维护合同（保留）

批次 462。该原生 parentless `QThread` 只接受值对象，既不持有 Vehicle、BlackBox、控制权、任务执行或回放引用，也不访问当前库内容。它支持同一规范本地目录中固定封存归档与退役归档的一对一改名；不删除、不覆盖、不复制、不改数据库内容、不自动修复、不自动回滚。**退役只让封存检索不再纳入该段，不释放磁盘空间；恢复不是恢复任务、权限或执行状态。**

## API 与精确值契约

构造 `(canonicalAbsoluteDirectory, exactPolicy, requestId, exactRequest)`；`requestCancel()` 是合作取消，`current()` 返回互斥保护的状态副本；一次 `resultReady(requestId, observedResult)` 或 inventory 的 `failure(requestId, fixedCode)` 后，拥有方仍须等实际 `QThread::finished` 才释放生命周期占位与对象。结果信号不是线程已经退出。析构断言线程未运行，不在 GUI 上等待。

`requestValid(request)` 与 `resultMatches(result, request, policy)` 仅做值、类型、精确形状、UUID、原始对应关系和 JSON 字节预算核验，不做文件、目录、SQL 或线程操作，也不认证一个来自外部的 Map。目录、policy、请求 ID 与原始请求由实际 Store 持有并绑定，不从显示字符串建立授权。

- policy 沿实际 ArchiveWorker/HistoryReader 的完整 7 项边界：`maximumEvents / maximumDatabaseMiB / maximumQueueEvents / maximumQueueMiB / batchEventLimit / busyTimeoutMs / maximumHistoryMiB`，范围不放宽。
- inventory 请求 exact1：`{kind: "inventory"}`。
- retire/restore 请求 exact5：`kind / source / expectedDirectoryIdentity / operationId / maximumSealedSegments`。`source` 是原始 descriptor7；`operationId` 为小写规范非零 UUID；`maximumSealedSegments` 严格 Qt Int 1–128，产品 Store 传当前实际 32，不接受调用方放宽产品容量。`expectedDirectoryIdentity` 是原清单目录对象观察标识，不是路径或认证标记。
- descriptor7：`basename / kind / databaseEpoch / sealRunId / fileBytes / fileFingerprint / modifiedUtcMs`。所有字段为非 null QString；kind 只能 sealed/retired，大小写敏感；文件字节为规范正十进制且至少 512；mtime 为规范正十进制毫秒；SHA-256 为小写 64 位十六进制。epoch/run 为完整规范 UUID。basename 必须恰好 `fleet-events-sealed-<epoch>-<run>.sqlite` 或 `fleet-events-retired-<epoch>-<run>.sqlite`，与其 kind/epoch/run 完整一致。target 只能由这个名字推导，不接受任何路径。
- common12：`historicalOnly=true`；`authenticated / signatureVerified / permissionsRestored / executionPermit / flightCommandReleased / executable / completionVerified / restoresAuthorization / allRecordHashesVerified / catalogCompletenessVerified / crossSegmentAtomicSnapshot` 全部为 false。真实改名不是只读，不添加 `readOnly=true`；historicalOnly 指归档诊断域而非零写入。
- inventory 结果 exact19 = common12 + `format="merivus-fleet-archive-maintenance-inventory" / schemaVersion=1 / directoryIdentity / entries[descriptor7] / sealedCount / retiredCount / enumerationCompleted=true`。每个 namespace 最多 128、总最多 256，结果核全部条目及唯一 basename，不发失败前缀。
- move 结果 exact27 = common12 + `format="merivus-fleet-archive-maintenance-move" / schemaVersion=1 / kind / operationId / source / targetBasename / expectedDirectoryIdentity / observedDirectoryIdentity / decisionFinal / renameAttempted / renameReportedSuccess / mayHaveMoved / moveConfirmed / cancelObserved / errorCode`。kind/operation/source/expectedIdentity 必须原样对应请求；所有六个事实位为严格 Bool。返回的 source 是原始 descriptor，不以目标新值代替原审批依据。
- `current()` exact19 = common12 + `requestId / state / errorCode / decisionFinal / mayHaveMoved / moveConfirmed / cancelRequested`。state 为 created、opening、inspecting、moving、confirming、result-submitted，inventory 失败也可为 failed/cancelled。state 仅生命周期状态，不代替 result 中的移动事实；result-submitted 可以带错误码。

完整 worker 结果 JSON 预算为 `min(2 MiB - 2048, maximumHistoryMiB × MiB - 2048)`；余量供实际 Store/Root 展示外层绑定使用，不把目录或操作 ID 拼入底层 errorCode。

## 原生检查与操作次序

1. 检查已有规范本地目录、固定请求与 policy。固定锁路径 `fleet-events-v1.sqlite.owner.lock` 沿原 ArchiveWorker 检查：不能为 symlink、异常类型、超 16 KiB 或 canonical path 不一致。`QLockFile::setStaleLockTime(0)` 后非阻塞取得相同产品合作 owner 锁；不创建、打开或修复 `fleet-events-v1.sqlite`。零 age 不是禁用 Qt 对已死进程的处理，也不是恶意进程互斥证明。
2. **先取锁再首清点**，避免锁文件本身创建使单次 native scan 自行失效。锁覆盖两种 namespace 全部枚举、完整文件 hash、SQL、决定、改名和后核。原生 scanner 区分 EOF/error，扫描包括无关项的 4096 目录上限不变。sealed 与 retired 各有独立 128 入选上限；大小写变体、sidecar、坏 UUID 名字、目录及链接进入所属 namespace 后整批拒绝，不静默过滤。原 sealed namespace 不纳入合法 retired 文件。
3. inventory：首轮两种 namespace 清点，同目录对象 ID；每文件普通文件、无 symlink/sidecar、canonical path、字节/mtime，100 字节 SQLite header、page size/count、application ID、schema version、rollback journal 格式、change counters；以 64 KiB 有界块做完整文件 SHA-256。独立 SQL 连接 `QSQLITE_OPEN_READONLY`，核实际 busy_timeout/delete journal，读事务内调用共享 `FleetEventArchiveSql::sealedRunMatches`，它检查真实 schema/meta epoch 和指定 seal run 的 clean_shutdown。退出读事务（rollback 仅结束读视图，不是撤销移动），关闭并移除该线程 SQL 连接；再次完整文件 hash/metadata 核一致。全部完成后再次两个 namespace 原生正常 EOF，清单/mtime/bytes/目录对象标识不变才发全部条目。
4. move：首清点必须原 expectedIdentity；source 仍在原清单且字节/mtime 相同；目标名字和三种 sidecar 都不存在；retire 的目标 retired <128；restore 的目标 sealed **小于请求中实际产品上限**，不是只按 128 放回。完整 hash + SQL 身份 + 二次 hash 必须与原 descriptor7 全等。再清点并核原清单与目标缺失。
5. 取消在最终决定前可阻止操作。决定处互斥保护，随后 `decisionFinal=renameAttempted=mayHaveMoved=true`；此后取消只记录，不跳过后置核、不自动回滚。native rename 即使报告失败，也保留 mayHaveMoved=true。没有高层 copy/remove fallback。
6. 后核忽略合作取消，以新的完整清点确认同目录对象、原名及其 sidecar 缺失；完整目标文件 hash/SQL 身份/二次 hash 必须与原 source 除 basename/kind 外全部一致；再清点稳定且原名仍缺失。仅这个观察整体通过才 `moveConfirmed=true`。native 返回 true 但后核失败为 `archive-maintenance-postcondition-unconfirmed`；native 返回 false 且后核未确认是 `archive-maintenance-rename-unconfirmed`。若 native 报失败但完整后核实际确认成功，保留 reportedSuccess=false、moveConfirmed=true 的两种独立事实。

所有有效 move 终态，包括未决定失败、取消、决定后不确定，均通过 resultReady 保留 target 与 mayHaveMoved，不以 failure 丢失事实。不确定时不推测已恢复、已退役、已取消或可重试；工程师先重新清点，不能盲目重试或自动移动回来。清单失败可 failure，且没有“已核验前几个”的成功列表。

## 平台实现、可信边界与未验证事项

Windows 使用 `MoveFileExW(..., 0)`，同一已检查本地目录，不传 REPLACE_EXISTING 或 COPY_ALLOWED。Linux 只有本地目标头提供 `SYS_renameat2` 与 `RENAME_NOREPLACE` 才调用 syscall；底层内核/文件系统不支持时失败，不退回普通覆盖式 POSIX rename。其他平台（包括当前未确认头文件/声明的 macOS）明确 unsupported；不编造原型，不使用 link/unlink 替代。旧 ArchiveWorker 的旧 seal 路径不属于本批修改，不能把新无复制实现反向宣称给旧路径。

目录 ID 是原 scanner 的 `windows-volume-file-index:<8hex>:<8hex>:<8hex>` 或 `posix-device-inode:<unsignedDecimal>:<unsignedDecimal>`，只是观察值。单次扫描核目录变化 metadata，跨扫描核对象和清单；没有锚定文件句柄级 rename，也没有抵御恶意 ABA/TOCTOU、ACL 被篡改、非合作进程、硬链接别名、对象 ID 重用的保证。文件 SHA 证明本次字节一致，不证明 payload 全部 hash 正确、签名、认证或完整历史。共享 SQL inspect 含结构/关系/quick_check/计数核验，不逐条加载并验证全部事件内容，也不能中断正在执行的 SQL/native 调用。

native rename 与后核不提供 fsync、目录 fsync、断电原子性/持久化或 crash 后自动恢复证明。合作取消没有实时 deadline；决定前后 race 的观察时点明确，不将晚到取消误报成未执行。最多 256 个有界文件逐块读取，大小上限沿原 policy，可能有大量 I/O，不能在 GUI 线程直接执行。

本批**未编译、测试、执行清点/SQL/改名、打开应用或操作设备**。后续工程师需验证目标 Qt/SQLite/native headers 兼容，MOC 登记和实际 finished 退役，锁冲突/死进程/ACL/Unicode，native unsupported 与目标已存在，源/目标 sidecar、mtime/hash/epoch/seal 变化，容量 32/128 边界，枚举 error 与明确 EOF，读/close/rename/后核失败、取消时序、断电不确定结果、不同真实文件系统行为。以上是待做项，不是已执行证据。

API/源码静态依据（不等于平台运行验证）：[Microsoft MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw)、[Linux man-pages rename/renameat2](https://man7.org/linux/man-pages/man2/rename.2.html)、[Qt 5.15.2 QFile 源码](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/io/qfile.cpp)。新实现避开 QFile rename 的 copy/remove 和失败回滚分支。

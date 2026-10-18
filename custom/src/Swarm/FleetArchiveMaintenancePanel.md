# 本地封存段维护面板

## 546 A4 完整容量编排区

在原维护面板内增加独立容量区，沿同一实际 `archive` facade 的 `capacityPlan/capacityStatus/capacity(intent,true)`，不增加组件资源、Store、目录服务或飞行入口。用户先显式读上方原 inventory 并填写原备份目录，再填 sealed 保留数量0–32与逻辑 MiB0–131072；空白不是0。生成完整预览不读盘，列全原件／副本路径、bytes/hash、目录身份、保留与移动数量。排序为文件 mtime 新到旧、同时间按 basename，保留同时满足两上限的连续前缀；mtime 不是任务完成时间。0表示本次全部移出，不自动选择永久删除。

预览后独立复选核对本次完整影响，30秒内才能提交同一原 planId；确认只允许 sealed→retired 后逐项新备份／读回，不允许永久删除。UI 使用原 count/MiB/目录输入版本、source/run、私有生命周期、原 plan 全值 stamp、独立 consent 版本与按下票据，来源／隐藏／禁用／输入变化撤旧确认，不借共享最新诊断恢复选择。输入和清理通知期间持 plain 围栏；取消／放弃闭包先冻结原 owner/run/planId，即使组件在通知中销毁仍只清准确原计划，不取消后来或别页工单。

Intent exact7 与 Plan24／row6、Status20／结果row4 严格闭形，见 [Store 合同](FleetPersistentEventStore.md)。prepare 接受后只采本次实际 plan；commit 接受只是原编排开始，不是成功。UI 不把各步 jobId 当 planId，也不从 lastWorkerResult 认领本页权限；本页准确 owned plan 才可请求取消。原生取消仅停止后续步骤并合作取消原 worker，已有移动／写入／部分副本不回滚，actual finished 才空闲。原终态未知时显示未知，不补未移动／未写false，不自动重试。

预览与步骤事实均用有界 ListView，长路径／危险按钮 PlainText 换行、主题颜色和最小目标；两个输入、四动作、独立确认与反馈具有键盘焦点及实际 Flickable contentY 有界滚动，危险动作不以 Enter 穿透默认按钮。步骤诊断独立显示 moveConfirmed/mayHaveMoved/renameReportedSuccess 与 backupVerified/mayHaveWritten/copyCompleted/bytesCopied；只有同原 native plan/status ID 时显示原冻结目录作为只读工程定位，不从后来目录猜旧副本路径。

本流程只编排旧移动与备份，没有自动清理／物理释放认证；retired不足时整份拒绝。下方525仍必须另做新备份、独立检查与双确认，本容量计划不生成删除 basis。旧462–466的隐藏不读盘／不取消仅限定旧区；本新容量区退场会准确请求取消原 owned plan、废弃原 prepared plan，不声称已撤回文件副作用。仅使用技能的静态通用指导；未执行技能脚本、界面、预览、编译或测试，不作为视觉／实机验证。

## 525 独立永久删除区与旧流程区分

本维护区新增默认收起的 [FleetRetiredArchivePurgePanel](FleetRetiredArchivePurgePanel.md)，只传同一真实 `archive.retiredPurge` facade；sourceActive 沿原维护组件的实际可见／启用。两个原宿主仍用现 archive，不接新 controller、原生服务、路径或 proof。新组件及合同是本批两条新路径，资源登记由主接线维护。

下面462–466是原移动／备份／恢复历史合同，其“不永久删除／不清理磁盘”只限定这些旧按钮。525另一个明确危险流程才可能永久删除 **一个已retired原件**：必须新做该原件外部备份，原 native backupVerified＋actual finished 后建立私有 basis，再独立只读检查、核对路径／UUID／hash／文件ID和两项确认。旧库存、独立目录诊断、公开Map、旧备份确认或任意路径均不变成删除权。

新面板隐藏／换源／销毁仅取消本页准确 job、作废准确旧 preview，不清别人；结果只读保存实际删除和收据事实，不因失败补“未删”，不自动重试、回滚、清当前库／sealed／副本／partial或恢复任务。仅Windows支持原生精确删除，物理磁盘空间回收认证恒false。旧可恢复移动和新永久删除的文案已明确分开，完整19／18／13／11／41合同及全部未执行边界见新模块。

## 462 实际入口与范围

[FleetArchiveHistoryPanel](../../res/Merivus/FleetArchiveHistoryPanel.qml) 内新增默认收起的独立 Loader，创建 [FleetArchiveMaintenancePanel](../../res/Merivus/FleetArchiveMaintenancePanel.qml)，并登记于 [qmldir](../../res/Merivus/qmldir) 和 [资源清单](../../qgroundcontrol.qrc)。展开、选择、勾选确认、展示 DTO、收起及销毁不读盘，也不发 stop/cancel；实际 inventory/commit 只由明确按钮点击发出。

462 为固定本地归档目录中单个封存段的 `sealed ↔ retired` 可恢复改名入口；463 在同一面板增加显式单文件备份及读回，不是磁盘清理。retire 仅使该段不再进入普通封存段历史检索，仍占用磁盘；restore 使其重新进入普通检索，不恢复实时任务、账号、批准、许可、执行状态或飞行命令。一次只操作一个文件，不永久删除、不覆盖已存在目标、不移动当前未封存库、不输入任意源文件路径/SQL/文件游标、不自动重试/覆盖/回滚/修复旁路文件。只有463的 prepare-backup 可显式提供受原生检查的现存目标目录，不能绕过原库存或原预览选择任意源文件。

所有维护动作要求实际录制线程退出、capture=false、普通历史读取及维护槽空闲。需要固定目录协作锁，Qt 可能回收死进程锁；协作锁不是抵御恶意并发改写、原生文件替换或密码学认证的保证。面板明确解释停止/取消 accepted 不等于实际线程退出，不要求连接飞机或登录。原 [历史面板](../../res/Merivus/FleetArchiveHistoryPanel.qml) 和 [留存任务面板](../../res/Merivus/FleetArchiveTaskHistoryPanel.qml) 的统一 busy 纳入 `archiveMaintenanceActive`，缺该 Bool 字段时不能误认为空闲；Store 仍独立把关，不依赖 UI 禁用。

## 窄 facade、请求及真实完成

组件仅接 `archive.current/options/maintenanceInventory/maintenancePreview/maintenanceResult`（464再加只读 `maintenanceBackupCatalog`）和 `maintenance(intent, confirmed)`；没有 Store 对象、文件系统、SQL、飞机、账号、任务恢复或权限 API。

旧动作 intent 仍恰六个非 null 字符串字段：`action/expectedRunId/inventoryId/entryId/previewId/queryId`，动作为 `inventory/prepare-retire/prepare-restore/commit/cancel/discard-preview`。463 新 `prepare-backup`、464新独立 `backup-catalog` 动作各加第七个 `backupDirectory` 字段，细节见后文；commit 不加路径。

- inventory：四个 ID 为空，显式启动原生读盘线程。
- prepare：只用本页原生 inventoryId 与该清单 entryId，preview/query 为空；纯值预览不读盘、不改名。
- commit/discard：沿用同一原清单、原 entryId、原 previewId，query 为空。commit 显式启动单文件改名线程；discard 只作废原预览。
- cancel：只携本页持有的实际 queryId，其余三个 ID 为空。仅当前实际维护线程 requestId 与其相等且尚未接纳终态时可发送，不取消其他面板请求。

9 键 schema1 回执只表示请求接受，不代表清单有效、改名成功、取消完成或持久化。prepare 接受时 requestId 是 previewId，不是线程 ID。inventory/commit 必须等待 `archiveMaintenanceActive=false`（Store 在实际 `QThread::finished` 后才释放）及同 query/run/action 的终态 result；不以 resultReady 或回执代替实际结束。取消后继续观察同一原请求终态，不能因为“已请求取消”而丢弃可能已移动的结果。

未知/拒绝回执不补成功、不自动重试；提交/新清单会提前消耗本地旧清单资格，commit/discard 会提前消耗旧预览及确认。这样未知回执、失败或旧值恢复不能重复使用原确认。实际文件与清单是否仍匹配，始终由 [原生 Worker](FleetEventArchiveMaintenanceWorker.cc) 再检查，UI 预览不是读盘证明。

## 原生 DTO 与完整有界接纳

值复制限制深度12、32768节点、1 MiB字符、单数组256、单 map64键；拒绝非有限数、超精确整数、危险对象键及不可复制对象，不执行 helper、SQL 或业务脚本。

共同12个字段为 `historicalOnly=true`，以及 `authenticated/signatureVerified/permissionsRestored/executionPermit/flightCommandReleased/executable/completionVerified/restoresAuthorization/allRecordHashesVerified/catalogCompletenessVerified/crossSegmentAtomicSnapshot=false`。不通过字符串或数字代替 Bool，不从归档提升权限。

- inventory 恰22键：原 Worker19键加 inventoryId/queryId/runId。最多 sealed128、retired128、总计256；完整枚举才接纳，不用有效前缀冒充成功。每项恰8键（原 descriptor7 加原生随机 entryId）；entryId/文件名不重复，各类计数与完整数组一致。公开清单去 entryId 后须逐项等于同原请求 workerResult 原清单。
- descriptor7 为 basename/kind/databaseEpoch/sealRunId/fileBytes/fileFingerprint/modifiedUtcMs。只允许规范 `fleet-events-{sealed|retired}-<UUID>-<UUID>.sqlite`；文件指纹小写 SHA256；bytes/mtime 保留规范正十进制字符串，bytes 为512至4096MiB内；不把大数或 mtime 当 JSON 浮点身份。目录身份是原生 Windows volume-file-index 或 POSIX device-inode 字符串，不误当内容 SHA256，不用它证明全目录内容不变。
- preview 恰25键，必须为本页本次 prepare 回执 previewId、原 inventory/entry/run、原 source descriptor、对应 targetBasename、status=prepared、canCommit=true、maximumSealedSegments=32、expiresAfterMs=30000。UI 不生成 ID，不用新时钟延长有效期；过期由原生计时，恢复时32个普通封存段容量可能拒绝，目标存在也不能覆盖。
- result 恰20键，匹配原 query/run/action。failed 必须有明确 errorCode 且 workerResult={}，不能据此断言文件未变。inventory-complete 需完整19键原清单。move-confirmed/move-uncertain/not-moved 需完整27键原改名事实，原 source/target/目录身份及 action 对应本页原预览；operationId 是另一个原生 UUID，不错误要求等于 queryId。
- move27 保留 decisionFinal/renameAttempted/renameReportedSuccess/mayHaveMoved/moveConfirmed/cancelObserved。前3个关键关系为 decisionFinal=renameAttempted=mayHaveMoved、reportedSuccess⇒attempted、confirmed⇒decisionFinal且error为空；已决定时 observedDirectoryIdentity 必须等于 expected。系统调用报告失败但完整后核确认时，允许 confirmed=true/reportedsuccess=false；晚到取消也可以 confirmed=true/cancelObserved=true，不能用取消冲掉实际事实。
- worker current19 为共同12加 requestId/state/errorCode/decisionFinal/mayHaveMoved/moveConfirmed/cancelRequested；只显示同本页原请求且完整形状核实的快照。created/opening/inspecting/moving/confirming/result-submitted/failed/cancelled 不等同线程已退出。

原生失败、枚举超限、结果缺失/拒绝、未确认移动与旧快照均明示，不凭空展示“部分成功清单”。已结束结果另显示完整改名事实；`mayHaveMoved=true`/move-uncertain 明确提示可能已改名、不能称失败未变，保留文件/回执/错误交工程师核查，不承诺掉电原子性、自动回滚或恢复真实任务。

## 归属、版本与独立确认

按下按钮时冻结原 facade、options 指纹、生命周期 epoch、run、原 savedInventory/savedPreview、原选择版本和确认对象；click 发送前再次核对，callBusy 不通过临时解锁获取可用性。确认复选框也有独立 pressed/click 快照，不能在按下后预览切换时确认新的预览。

每次来源/options/run 变化递增生命周期 epoch；改回相同值也不复活旧快照。每次原生 inventory/preview 通知、每次选择变化都递增独立版本并清除确认，旧值回同值不能恢复资格。保存值还固定对应原生对象身份；共享属性非空或其他入口完成不是本页成功。变更版本耗尽即拒绝新操作。

接纳 prepare 后不自动勾选。commit/discard 调用前消耗原 previewId 并清确认；只在本次已捕获旧确认的调用尾核中允许该消耗态，不误把安全消耗当正向流程自锁。同步原生通知可以先于回执；先固定 ownedOperation，再在原生 ID 回填后重做完整接纳。异步通知只接同原请求，失败和取消不得回灌或自动重试。源变化或销毁只撤掉本页资格，不取消其他所有者。

## UI/UX 与工程师待验证

本批使用 `ui-ux-pro-max` 及其 static quick-reference/pro-rules：复用 QGC palette/字体/按钮，不创建视觉系统、不重设计原页面、不运行搜索脚本。采用默认收起、显式四步流程、独立复选确认、危险动作文字说明、不以颜色独传状态、长期/旧结果标记、纯文本长 ID 换行、有界虚拟 ListView、按需只读 DTO、StrongFocus/可访问名称及焦点滚入。按钮禁止 Return/Enter 意外提交，保留标准 Space/点击；没有动画或悬停唯一操作。

未编译、解析、执行 UI/helper/SQL/业务脚本、测试、仿真、设备操作、刷机、认证、提交、外写或打包。静态阅读不能证明 QML 实际加载、键盘/读屏、主题对比度、改名行为、线程竞态或掉电语义已通过。

工程师须验证：展开不读盘、空/最大/超限目录、32段恢复容量、同源目标与旁路文件冲突、源/options/run/selection/preview 通知 ABA、按下到点击及确认之间的同步变更、未知回执/同步结果、30秒超期、单次消耗后重复点击、与录制/历史读取互斥、同 query 取消早晚两种时序、resultReady 先于 finished、native报告失败但后核确认、无法确认移动的醒目提示、旧结果无权限、窄屏/大字体/明暗主题/键盘与虚拟化焦点。所有项目均仍未执行，交由工程师实测。

## 463 显式备份与完整读回

仅扩展原 QML 和本文，不创建重复大面板、目录选择组件或资源登记。复用原库存、sealed/retired 原生 entryId、原选择、同一独立复选确认、commit/cancel/discard 和实际 finished 接纳。面板提供有可访问名称的目录文本输入，maximumLength=4096，初始为空、不保存默认目录、不读盘、不创建目录、不自动触发 prepare 或复制。

目标是原样的规范本地绝对目录，Windows 示例 `E:/FleetBackups`：使用正斜杠、无末尾斜杠，不写 URL、UNC/网络路径、文件名或根目录。不 trim、不替换反斜杠、不 cleanPath，也不通过路径同值重现续期。UI 只给有界基本语法提示；实际存在性、可写性、规范原路径、Unicode、链接/重解析点、源与目标是否互不包含、目录对象身份及目标新文件条件均由 [Store](FleetPersistentEventStore.cc) / [Worker](FleetEventArchiveMaintenanceWorker.cc) 检查，不能把预览当实际可写证明。

只读检查仓库 [QGCFileDialog](../../../src/QmlControls/QGCFileDialog.qml) 后没有嵌入 picker：其桌面 selectFolder 透传有效，但 mobile 自定义文件选择分支不处理目录选择且包含删除/覆盖入口；[LogDownloadPage](../../../src/AnalyzeView/LogDownloadPage.qml) 明确说明移动端不能选目录。本批不声称已验证跨平台 picker，不引入与单文件“不删不覆盖”产品边界不符的通用文件操作菜单。

prepare-backup 的 intent7 为旧六键加原样 `backupDirectory`；原六个 ID/动作字段仍各自为原字符串范围，不让长目录路径挤进128字符 ID 界限。commit/discard/cancel 仍 intent6，不重传可替换路径。目录唯一来源是原生私有 preview context，普通库存与旧移出/恢复的形状完全不变。

backup preview 为 schema2、恰26键（原25加 backupDirectory）、action=backup。原生 previewId 同时用于备份 operationId，目标文件名严格为 `fleet-events-backup-<sourceEpoch>-<sealRun>-<previewId>.sqlite`；在影响预览中完整显示原目录及拼接目标，并核对原 source descriptor。拼接仅用于展示，不作为后端输入或新的路径授权。`maximumSealedSegments=32` 是旧政策兼容标记，不限制备份数量；原30000ms仍只是原生单调提交窗口，不是复制超时，路径改变不会重新计时。

新增独立 `backupPathRevision/backupPathExhausted`。每次目录文本变化都单调前进，改回不复活；只退休备份的预览/确认，不清除未改变的移出/恢复预览或确认。备份路径版本耗尽仅拒绝新的备份资格，不停用非备份操作。prepare、同步预览接纳、复选确认及 commit/discard 按下与调用尾核均冻结/核对原路径与该版本；callBusy 期间不临时解锁。备份启动以后，目录输入仍可修改；取消和终态结果按原实际 query/preview/原目录接纳，不以当前新文本或新路径版本丢弃可能已经写入的结果。

外层 result 仍 schema1、恰20键；action=backup，终态只 `backup-verified/backup-uncertain/not-backed-up/failed`。前三种需内部完整31键 backup facts：共同12加 format/schemaVersion/kind/operationId/source/targetBasename/expectedDirectoryIdentity/observedDirectoryIdentity/destinationDirectory/destinationDirectoryIdentity/decisionFinal/targetCreateAttempted/targetCreated/mayHaveWritten/bytesCopied/copyCompleted/backupVerified/cancelObserved/errorCode。

核对31键完整值、原 previewId 操作ID、原源/目标/目录及原生目录身份形状。bytesCopied 为规范十进制字符串，可0且不大于 source.fileBytes。decisionFinal=targetCreateAttempted=mayHaveWritten；created⇒attempted、非0bytes⇒created、complete⇒created且bytes等源长度；已经决定时观察源目录身份等原 expected，目标目录身份必须已知。verified⇒complete且error为空、两目录观察已知匹配原约束；未 verified 则 error 非空。可以已核实后才观察到取消，不用 cancelObserved 抹掉读回事实。failed 且 workerResult={} 仅说明无可核实完整结果，不表示没有创建/写入文件。

备份实际 worker current 为恰23键：共同12加 requestId/state/errorCode/decisionFinal/targetCreateAttempted/targetCreated/mayHaveWritten/bytesCopied/copyCompleted/backupVerified/cancelRequested，状态为 created/opening/inspecting/copying/verifying/result-submitted/failed/cancelled。还核 bytes 不超原 preview 源长度、decision=attempted=mayWritten、created⇒attempted/非0bytes⇒created、complete⇒created且长度等源、verified⇒complete且error空；不显示自相矛盾快照为可信进度。旧库存/改名 worker19 与 move27 保持原路；根据本页原请求动作明确分支校验/显示，不能读不存在的 mayHaveMoved/moveConfirmed 当 false 而声称没写。取消只同实际 query，active 到实际 finished 才释放。

状态区逐项显示已尝试创建、是否创建、可能已写、已复制字节、复制完成、完整读回核实与取消事实。明确区分“复制完成”和“已完整读回”；即使 backup-verified 也不授予 authenticated/signatureVerified/allRecordHashesVerified/catalogCompletenessVerified 或任何飞行/任务权限，不证明每条原事件真实、所有历史完整、跨库原子性或掉电持久化。未核实/取消可能留下部分文件，界面醒目提示保留原目标路径、文件、回执及错误交工程师，不自动删除、覆盖、重试、回滚或从副本恢复任务。

原 copyValue 的总字符1MiB、32768节点、深度12、map64/array256边界不缩小；合法4096字符目录及31键事实中有限重复均可完整容纳，不误套旧 ID 的128字符上限。纯文本换行展示长目标，原样输入可键盘编辑；没有新动画/图标、自动读盘、目录扫描或 UI 脚本执行。技能沿用前述静态 UI/UX 规则；未运行 helper、解析器、QML、应用、备份、读回、编译或测试。

工程师新增待验证：正斜杠 Windows 路径、4096字符/非BMP/损坏代理项/ADS/链接/网络/根/源目标包含拒绝，sealed及retired两种源、源变化/目标已存在/磁盘不足、路径按下到click及复选到commit间变化/改回、改路径不影响旧改名确认、原30秒过期、同步预览通知先于回执、复制中改路径/晚取消仍显示原目标结果、partial/copyCompleted未verified、完整读回失败、同一原文件UUID不覆盖、actualfinished后才空闲、旧结果只历史、长路径大字体/键盘/读屏与主题。以上全部未执行。

## 464 独立读取备份目录与可读性诊断

同一面板、同一备份目录文本框中新增显式备份目录严格检查按钮；不要求先读原库存、选择源 entryId、建立源预览或连接飞机。展开面板、输入路径、改路径、列表呈现和展开 DTO 均不发请求，不读盘。此动作只检查明确指定目录中规范备份文件的当前可读性/结构自洽，不隐式访问原归档目录、不读取原当前库或取得源锁，不恢复任务、批准、账号或权限。用户明确选中的目录可以恰是原路径，不额外添加源目录互斥约束，也不声称物理上永不访问用户指定的原路径。

backup-catalog 为 intent7：原六个字段、action=backup-catalog、expectedRunId=原run、inventoryId/entryId/previewId/queryId全空，加原样 backupDirectory。仍最多4096字符、正斜杠、现存规范本地非根目录，不创建目录、不改写路径。其原生 work order 为 kind+destinationDirectory 两字段，不携原源或源库存。会创建/释放备份目录协作锁；不是完全无写盘痕迹，也不锁/读取原源目录。不会自动停录、修复坏库、删除部分副本或重试。

UI 为该独立读取新增 `catalogSourceReady/catalogOptionsStamp`：仍要求真实 facade、生命周期/版本有效、完整 options7/政策7形状、显式默认关闭标记及实际录制/历史读取/维护 Bool 状态，只允许固定源 directoryPath **字符串为空**，不补路径、政策或状态默认值，不查源路径是否存在。原 `sourceReady/optionsStamp` 仍要求源 directoryPath 非空，旧 inventory/prepare/move/copy 不因独立读取而默认放行。目录输入在独立读取来源就绪时可编辑，不能被旧 sourceReady 自锁。全局真实槽仍须空闲；cancel 根据原 ownedOperation.catalogRead 使用其独立来源和 options 指纹，因此源目录缺失/空路径不是同 query 取消的前置条件。

Worker catalog 为 schema1、恰21键：共同12加 format=`merivus-fleet-archive-backup-catalog`/schemaVersion/destinationDirectory/directoryIdentity/entries/fileCount/enumerationCompleted/originalSourceVerified/backupSetCompletenessVerified。entries最多128，fileCount为相等的原整数、enumerationCompleted=true，后两个验证标记必须false。没有部分成功目录；坏末项、目录改变、取消、超限或不可读时整批拒绝，不能展示有效前缀。

原 entry 恰8键：原 descriptor7 中 kind=backup，再加 backupOperationId。文件名严格为 `fleet-events-backup-<databaseEpoch>-<sealRunId>-<backupOperationId>.sqlite`，三个ID均规范UUID；bytes/mtime为规范正十进制、文件512至4096MiB内，内容指纹为小写SHA256。不能把文件名中的历史操作ID当作过去 backup-verified 回执或当前真实原源身份。

Store `maintenanceBackupCatalog` 为恰24键：原 Worker21加原生 catalogId/queryId/runId，每个公开 entry 又加随机原生 entryId 为恰9键。接纳时核同原 query/run/action、原查询目录、全部字段、文件名严格升序/entryId唯一、计数；去公开 entryId 后逐项等原 Worker descriptor8，去附加三个顶层字段后保持原21语义。不能凭共享属性非空、其他入口完成、空列表或成功前缀给本页补成功。

外层 result 仍 schema1、恰20键；action=backup-catalog，只有 backup-catalog-complete（完整原 Worker21）或 failed（有错误且workerResult={}）。实际 worker current 为原19键，不误套复制23；限定 created/opening/inspecting/result-submitted/failed/cancelled 读取状态、decisionFinal/mayHaveMoved/moveConfirmed全部为false，拒绝移动阶段冒充目录读取进度。状态文字按原 action 独立说明只读查询，不从 mayHaveMoved=false 声称复制/恢复已完成。与其他动作一致，必须 `archiveMaintenanceActive=false` 即 Store 实际 finished 后才接纳；resultReady先到或9键accepted回执都不代替完成。同原 request/query 才能取消，取消请求不提前释放槽。

按下/click与调用前尾核冻结同原 owner/options/run/epoch、原备份目录及路径版本，无库存或选择依赖。同步原生通知可先于回执，仍先固定原调用，回执回填真实 queryId 后再次接纳。同副本复制一样，已发目录查询后改路径不丢弃原路径的真实终态；独立 savedBackupCatalog 保留原目录、路径版本、原生对象与本地通知版本，只读展示。其“当前”资格须原路径/版本仍匹配；路径改回同值不复活。来源/options/run变化仍退休旧资格；归属指纹允许空目录的范围仅限该动作。

新目录读取、重新原 inventory 和 commit 都先消耗本地旧库存/预览/目录续用资格，再调用原生；未知回执、失败或恢复旧同值不能复活旧上下文。Store 同样独立退休原私有上下文。旧 inventory/move/copy 保持原 positive path，不通过临时解除 callBusy 获得资格。savedBackupCatalog 与 savedInventory/Preview/Result 分开，失败保留旧目录只作明显历史快照，不偷偷复用它确认新源或新备份。

新增有界128项只读虚拟列表、焦点可滚入视区、纯文本长ID换行和按需完整DTO。清楚显示原路径、目录身份、原 query/catalogId、接纳文件数及当前资格。空目录只说明本次没有匹配规范备份，不代表其他目录无备份或全部历史为空。`originalSourceVerified=false/backupSetCompletenessVerified=false` 与共同否定标记完整保留；目录成功不是原源比对、全备份、签名/认证、所有行哈希、权限恢复、可恢复性或飞行放行证明。文件物理变化以后不会自动刷新，读取时自洽也不保证未来仍可读。

本批仅修改现 QML 与本文，没有新增组件或资源；继续使用先前已全文读取的 UI/UX 技能及静态规则，不运行搜索/helper、解析、QML/UI、SQL、原生读取、备份、编译或测试。工程师须新增验证：无原源目录及source path空字符串仍可独立读/取消，而旧动作不放宽；首次无需库存/选择；空/128/超限、坏末项/部分副本整批失败、原query同步完成与取消、原路径变化及改回、通知/owner/options/run ABA、actualfinished门禁、公开24/entry9对原21/entry8完整接纳、坏列/替换/锁/旁路文件拒绝、旧结果历史标记、128项虚拟化/键盘/大字体/主题及读屏。全部仍未执行。

## 465 逐文件备份诊断（限定静态收口）

在同一目录输入和独立空源门禁上增加 `backup-diagnostics`，intent7仍为原六字段、四ID为空及原样 backupDirectory，不要求原库存/源选择。原严格目录操作不替换、不放宽，按钮明确“严格检查，任一坏项整次失败”；新增逐文件按钮才将被拒绝候选作为独立诊断行显示，不能把有坏项的完整观察冒充全部可读。两者只按显式点击读取，不自动轮询/继续/重试、删除或修复。

统一 `inspectionRead = catalogRead || diagnosticsRead` 决定独立 options 指纹、来源就绪、原工单取消和只读 worker19；旧改名/复制仍走原非空源门禁与 current19/23分支，不按不存在的移动字段推断写入结果。独立读取均不隐式访问原归档目录，不读取原当前库或取得源锁；但用户明确指定目录可恰为原路径，不能误称任何情况下都不物理接触原路径。

Worker诊断 schema1 恰23键：共同12加 format=`merivus-fleet-archive-backup-diagnostics`/schemaVersion/destinationDirectory/directoryIdentity/entries/candidateCount/readableCount/rejectedCount/enumerationCompleted/originalSourceVerified/backupSetCompletenessVerified。三计数为0..128真整数，候选数等完整数组、readable+rejected等候选、实际逐行可读计数相等；enumerationCompleted=true，后两个证明标记false。Store公开恰26键，另加 diagnosticsId/queryId/runId；每行新增原生entryId。仍要同 query/run/原目录、实际维护线程 finished 后才整体接纳，不靠共享属性非空或回执成功。

原 row 恰4键 basename/status/errorCode/descriptor，公开 row恰5键。名字严格升序唯一、有效UTF16、1..512码元、不含NUL或正斜杠；POSIX原始坏名可含反斜杠，只作拒绝诊断显示，不归一/打开其payload。readable行 errorCode空，descriptor恰原backup8、basename匹配；rejected行 descriptor={}，错误只能以下六种：archive-backup-name-invalid、archive-backup-type-rejected、archive-backup-size-rejected、archive-backup-sidecars-present、archive-backup-file-rejected、archive-backup-structure-unconfirmed。不能从空descriptor读取伪默认数据库字段或将拒绝项选为源/恢复输入。

公开26去顶层附加ID/原生entryId后须逐项等原Worker23/row4；严格核完整形状、每项状态/原因/descriptor、原内容指纹及统计，不发布有效前缀。外层result20 action=backup-diagnostics，成功状态 backup-diagnostics-complete 可以 rejectedCount>0，只表示本次完整观察；failed仍须有错误且workerResult={}。枚举/取消/目录变更或结果拒绝不接纳部分成功诊断，旧副本只能保留明显历史。

新增 savedBackupDiagnostics 固定原 owner/options/run/epoch、query、原路径/版本、原生对象与本地通知版本。调用中先冻结原上下文，回执回填原生ID后重做接纳，actualfinished前不放结果。已发后编辑路径不丢原工单终态；“当前”仍要求原路径版本，改回不复活。inventory/commit/catalog/diagnostics开始前同时消耗旧库存/预览/目录/诊断资格；未知回执或原值回放不能恢复旧上下文。取消仍只原真实query且不提前释放槽。

界面醒目展示候选/本次可读/拒绝三计数及原目录、原ID、当前资格；独立有界128项只读虚拟列表，拒绝项显示明确原因/诊断码，不出现源或恢复按钮。空候选不证明没有其他文件/目录或全部历史为空；“完成”不提升 originalSourceVerified、backupSetCompletenessVerified、签名/权限/真实性/可恢复性或飞行放行。

`escapedName` 只改变名字的显示：对原≤512码元有效名的U0000–001F、007F、2028–202E、2066–2069显示可见 `\\uXXXX`，反斜杠显示双反斜杠，防止换行/制表/bidi控制伪装状态行；原后端名称和比较值不变，转义串不作为路径回传。最长输出有界3072字符并完整换行展示，不截掉尾部；规范ASCII可读文件名不变。完整DTO仍只读历史值，不恢复权限。

面板及本文已与原生Worker／Store／Guided完成465批限定静态交叉阅读，沿用已读UI/UX技能静态规则；不等于实际加载或交互通过。未编译、解析、执行helper/UI/SQL/目录诊断/备份或测试。工程师须验证混合可读/坏名/链接/尺寸/旁路/坏结构、坏末项、有效UTF16长名及控制/bidi/POSIX反斜杠显示、未知原因和坏descriptor整包拒绝、128/超限/空、改路径后原终态与当前资格、source空目录独立cancel、旧严格操作仍整批拒绝、实际finished、完整26/23和5/4、同通知与版本ABA、虚拟化/键盘/主题/读屏。全部未执行。

## 466 可读备份显式导入retired保留区

只扩现面板与本文，不新增组件、文件选择器、可编辑目标路径或资源。当前逐文件诊断中 readable 行才有“选择此可读副本”按钮；rejected行仍纯诊断，无选择/导入按钮、不从空descriptor补字段。选择只是本页状态，不读盘；按下/点击冻结原diag对象、owner/options/run/epoch、原路径及版本，再核原生结果身份与readable行。单独 selectedDiagnosticEntryId/diagnosticSelectionRevision 与原库存选择分开，改选/改回不复活旧确认。

导入须原非空sourceReady、固定目标options.directoryPath和真实空闲，不能借独立空源读取放宽条件。只用同页当前savedBackupDiagnostics原native对象、通知版本、原路径版本及native readable entryId，不能凭用户填ID、历史JSON或旧同值进入；Store独立只用原私有readable依据。

新prepare-import/commit-import/discard-import intent均恰7键：旧action/expectedRunId/inventoryId/entryId/previewId/queryId加diagnosticsId。inventoryId永远空，prepare的previewId/queryId空、entryId为当前原可读行、diagnosticsId为原公开26 ID；commit/discard沿原diag/entry/preview且query空。没有额外sourceDirectory/destinationDirectory字段替换原生路径；cancel仍旧6键，仅同实际query。

prepare不IO。import preview共用原maintenancePreview槽，format=`merivus-fleet-archive-import-preview`/schema1/恰29键：共同12加format/schemaVersion/action=import/previewId/diagnosticsId/entryId/runId/source(backup8)/sourceDirectory/sourceDirectoryIdentity/destinationDirectory/stagingBasename/targetBasename/expiresAfterMs/status/canCommit/historicalImportOnly。最后一项true，status=prepared、canCommit=true、原单调提交窗口30000ms。完整核原diag/readable/source8、原目录及身份、固定目标；暂存名`fleet-events-backup-<epoch>-<run>-<previewId>.sqlite`，最终名`fleet-events-retired-<epoch>-<run>.sqlite`。

UI分别保存导入与旧改名/备份预览，不扩旧inventory gate；两者核同actualPreview原生对象与通知代次，不是两个同时执行的许可。任何新预览调用先消耗旧预览与两份确认，任何原生preview通知清两份确认，旧同值不复活。导入有独立复选框与pressed/click原预览快照，默认不勾；变路径/选择/预览不能确认新内容。不用UI时钟重基30秒。

预览完整展示原副本、固定目录、暂存及最终retired全路径，解释源副本不删除、不覆盖；目标容量、同身份sealed/retired已有文件、暂存/旁路冲突可能拒绝。不是任务恢复，不自动加入普通检索。commit-import先消费本地原导入预览、库存、目录及diag资格、清确认，再按原snapshot尾核；消耗态只允许原同次已捕获确认与原生对象/版本/路径，不再要求已消费变false的backupDiagnosticsCurrent，避免正向自锁。

调用先固定ownedOperation，回执后用真实requestId重装，兼容同步预览通知先于回执。prepare回执ID是previewId；commit实际线程query另有身份，Worker operationId才等原previewId。未知/拒绝回执不重试、不撤销或复用旧确认。discard仅作废原预览不IO。来源/选择/路径/通知版本变化后旧副本保历史无资格；已经发出的导入按原工单保终态，不因后改路径/选择丢掉可能已写/已发布事实。

result20 action=import，按原query/run/basis及actualfinished后archiveMaintenanceActive=false整体接纳；取消不提前释放槽、不丢晚到实际结果。Worker38为共同12加format=`merivus-fleet-archive-backup-import`/schemaVersion/kind=import/operationId/source8/sourceDirectory/expectedDirectoryIdentity/observedDirectoryIdentity/destinationDirectory/destinationDirectoryIdentity/stagingBasename/targetBasename/targetDescriptor/decisionFinal/stagingCreateAttempted/stagingCreated/mayHaveWritten/bytesCopied/copyCompleted/stagingVerified/publishAttempted/publishReportedSuccess/mayHavePublished/importVerified/cancelObserved/errorCode。

核全部字段与原29预览对应：decision=stageAttempt=mayWrite；created⇒attempt、非0bytes⇒created；copied规范十进制不超源，complete⇒created且字节等源；stagingVerified⇒complete；publishAttempted=mayHavePublished且⇒stagingVerified；reported⇒attempt；importVerified⇒publishAttempted+stagingVerified。决定后观察源身份等原expected且目标身份已知。未verified须error非空、targetDescriptor={}；verified须error空、retired descriptor7完整，basename/epoch/run/bytes/hash同原source8。允许native报告false但后核成立，以及latecancel=true但最终verified，不把系统调用报告成功当最终读回。

终态按实际事实分层：import-verified才核实已进retired；否则mayPublished为import-uncertain、仅mayWritten为import-incomplete、此前未尝试为not-imported。failed无完整38，不能推断没暂存/没发布。分别显示暂存创建/字节完成/暂存读回、发布尝试/报告/可能发布/最终核实。部分暂存、未确认目标交工程师；不自动删stage、回滚、重试、恢复任务或启用普通检索。

worker current27为共同12加requestId/state/errorCode/cancelRequested与上述11事实（含bytesCopied），只认原commit-import工单；states为created/opening/inspecting/copying/verifying/publishing/result-submitted/failed/cancelled。完整shape/布尔/规范字节/原源长度及暂存发布关系同样检查。只读目录current19、旧备份current23、旧改名move27和导入current27按原动作明确分支，不拿同键数冒充同类对象。

即使importVerified也只入retired；后续须显式重新inventory→选择retired→prepare-restore→独立确认后才纳入普通历史，不自动触发这些步骤、任务调度、命令事务、账号或批准恢复。共同否定权限/认证/真实性/全历史标记仍保留，不声称掉电原子性或恶意替换安全。copyValue深度12/map64/数组256/32768节点/1MiB字符容纳29/38及合法目录，不运行payload。复用诊断128列表的palette、焦点、纯文本安全名与Space/点击；Return/Enter不意外提交。

466仍作静态源审阅整理，未执行helper/parser/QML/UI/SQL/归档IO、编译、测试、刷机、提交或打包。工程师新增待验证：readable/rejected/旧ID与假公开map拒绝，selection/path/native通知ABA、sharedpreview相互替代与原30秒、按下到确认/commit变化、消费后正常commit不自锁、目标固定不可注入、源目标缺失/链接/旁路/容量/同身份冲突、不覆盖stage/target、复制部分/读回失败/取消/晚发布与nativefalse后核成立、actualfinished前不接纳、38/27严格形状、后改路径原结果保留、importVerified只retired且另inventory→restore、窄屏大字/键盘/虚拟列表及读屏。全部未执行。

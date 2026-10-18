# 留存任务诊断目录与原记录下钻面板

## 510：同任务哈希下的原生Run语义事件

原记录列表新增与[归档历史页](FleetArchiveHistoryPanel.md)一致的v3／runObservation只读摘要。真实原生事件的外层taskId经Codec保存为taskIdHash，因此沿既有原生任务目录→实际选定哈希→原记录查询就能看到本地MODE交接、连续性异常、源退休、结果候选及观察关闭事实；不依靠时间或相同机号猜关联，不直接拿执行lane引用去关联另一个NDJSON遥测source。

保留原35／31／29字段DTO、目录／记录两套续扫上下文、查询归属、8,192行累计预算、有限页、显式动作和只读副本规则。只增加每行已经收到的28字段事件摘要辨认／文字，不新增读取、控制或自动扫描入口。原生事件累计丢弃不等于本任务损失；全局capture-gap无taskIdHash，可能只在非任务过滤的归档历史里出现，不能把某任务列表没看到gap解释为完整。

显示始终保留historyComplete=false、结果候选非完成、监视关闭非停飞及不恢复权限的边界；遇未知／坏v3摘要显示未知。没有把ObservationClosed改成Completed或Failed。UI/UX仅静态可读性／PlainText换行，未运行UI、QML、SQL、helper、编译或测试。详见[事件合同](FleetRunObservationEvent.md)和[实际日志接线](FleetRunObservationJournal.md)。下方为历史。

## 462 维护槽与历史来源退休

原sourceReady新增对原生archiveMaintenanceActive真实Bool的要求，统一historyBusy包含维护线程；未知不能当空闲。维护实际finished前不允许发起目录／原记录查询，不用取消或结果accepted提前复用读盘槽。界面变化仅为这两处门禁，461两条预算、taskIdHash实际筛选、独立目录／记录续扫与旧副本语义保留。

同一[Store](FleetPersistentEventStore.md)在新的维护inventory／commit请求前退休原任务目录、原记录及它们的真实一次性上下文；副本仍可历史阅读，但原生DTO对象／来源失配后不能继续，不凭旧directoryId或改回同值重建。维护退役段不再进入普通sealed范围；恢复后必须明确重新查询，不恢复任务、事件、批准、账号或控制权。

维护UI独立默认收起，详见[面板](FleetArchiveMaintenancePanel.md)。未编译、解析、运行UI／SQL／线程／helper、测试或实际扫描／改名；静态核对不是运行／安全验收。下方为历史。

## 461 实际入口与范围

[FleetArchiveHistoryPanel](../../res/Merivus/FleetArchiveHistoryPanel.qml) 增加默认收起的独立 Loader；展开后创建 [FleetArchiveTaskHistoryPanel](../../res/Merivus/FleetArchiveTaskHistoryPanel.qml)。新组件登记于 [qmldir](../../res/Merivus/qmldir) 和 [资源清单](../../qgroundcontrol.qrc)。展开、切换范围、选择任务哈希和展开只读 DTO 均不读盘，不自动建立目录、继续查询、停录或取消其他入口的查询。

组件只接原窄 `archive` facade：`current/options/historyTasks/historyTaskRecords`、`tasks(intent, confirmed)` 及原 `request(intent, confirmed)`。实际任务筛选由 [原生 Reader](FleetEventArchiveHistoryReader.cc) 执行、由 [Store](FleetPersistentEventStore.cc) 保持独立原始续扫上下文；不是 QML 对已显示页面做过滤。没有原 Store、文件路径输入、SQL、原生游标输入、飞机、账号、活任务、回放导入、批准恢复或飞行执行入口。

范围仅 `sealed` 和 `sealed-and-offline`。两者均要求原生 `writerActive=false`、`captureEnabled=false`，以及 `historyRequestPending=false`、`archivedReaderActive=false`；不把停止/取消请求的 accepted 当作实际线程退出。包含停止后的未封存库时，固定库必须存在；界面提示创建/释放协作锁、Qt 可能回收死进程锁以及 journal/wal/shm 等异常应保留交工程师处理，不自动清理或重试。

## 请求、归属与取消

`tasks` intent 恰六个 nonnull 字符串字段（值按动作允许为空）：`action/expectedRunId/scope/previousQueryId/directoryId/taskIdHash`。动作仅 `tasks/continue-tasks/task-records/continue-task-records`。首次目录不携旧 ID；目录续批只能发本页原接纳的 queryId/directoryId；原记录首批只能选择本页原生目录明确存在的 taskIdHash；记录续批还须本页原记录 queryId、稳定的原目录身份、所选哈希及版本仍相同。UI 不拼文件游标或根据时间、机号猜 task。

按下时冻结原 facade、options 指纹、生命周期 epoch、run、范围版本、选中版本、原 savedDirectory/savedRecords/nativeResult 对象及 intent；click 调用前再次核对。callBusy 持有期间，不通过暂时释放锁来检查可用性；调用前冻结 ownedQuery，回执补入原生 queryId 后再尝试接纳，兼顾同步原生通知。9 键回执仅表示请求接受，不表示查询完成或结果有效。未知或拒绝回执不补成功、不自动重试，不复活本次已消耗的续批资格。

新结果需实际读取槽结束、`archivedResultAccepted=true`、`lastHistoryQueryId` 等于本页 ownedQuery、对应原生 DTO 的 queryId/范围/run/来源一致，再作完整有界形状核对。其他入口共享属性非空不是本页成功。保存副本带生命周期 epoch，owner/options/run 改动后即使改回，也不会复活旧副本的“当前”资格；范围和选择版本同样单调递增。旧副本保留只读，并明确显示是否仍属当前范围、选择及原生对象。

**目录与记录为两个独立的原生续扫上下文**：接纳新结果时使用全局本次完成 ID，但随后继续原目录或原记录不强加 `lastHistoryQueryId` 必须仍等旧目录 ID。记录下钻不能仅因改变全局 latest，就把未改变的目录续扫误锁死。各自仍须原 nativeResult 对象、独立 queryId、原 options/run/生命周期 epoch/范围版本、原生 canContinue 和真实空闲。继续目录更新已核实快照但 directoryId/inventoryFingerprint/scope/run 等稳定身份不变、所选哈希仍在实际目录时，原记录可继续，不因 JS 目录对象正常更新而永久失效。每次按下仍冻结当次当前目录对象和原记录对象，调用期间任一变化均拒绝；改变范围、来源或真实目录身份不能借此恢复旧续批。

取消复用原 `request` 的六键 intent：action=cancel、expectedRunId、catalogId/basename 空、afterRowId="0"、queryId=本页实际持有 ID。底层 pending kind 仍为 `search`。仅当原生读取器 queryId 与本页未接纳请求一致才开放；按下/click 再核原 ownedQuery、来源、run 和实际 busy。取消 accepted 不解除 busy，等真实读取器结束；收起/销毁/来源变化不调用全局 cancel 或 stop。查询状态中的失败/取消/预算码直接归属于同 queryId 的原生 DTO，不要求失败更新 lastHistoryQueryId。

## 有界 DTO、计数与预算

完整复制限深度 16、131072 节点、2 MiB 字符、单数组 256、单 map 64 键；拒绝非有限数、超过精确整数范围、危险对象键和不可复制对象，不执行其业务逻辑。节点上界覆盖合法 256 个任务各自累计 255 个关联机号的目录，不能用 65536 节点截掉合法完整机号列表。没有将容量阅读称作运行验证。

原生目录恰 35 键，格式 `merivus-fleet-retained-task-directory`、schemaVersion=1；任务行恰 9 键。核对任务哈希排序唯一、每行计数、最多 255 个排序机号、八种原生类别计数，以及 `unattributedRows + ΣeventCount == scannedRows`。目录最多 256 哈希、累计 8192 行。缺失明确任务哈希的保留行单独计数，不按其他字段补关联。

原记录 wrapper 恰 31 键，格式 `merivus-fleet-retained-task-records`、schemaVersion=1；正常接纳时内部原生 searchResult 恰 29 键、schemaVersion=2、taskIdHash 等于原选择、vehicleId=0、operation=""。核对文件身份、原行号顺序、原 coverage 范围、最多四页/256 扫描行/64 匹配行、各计数和 nextCursor 的边界。游标仅作只读展示，不发送回后端。只显示本批，不无限积累原始行。

两条预算路径分别处理，不将 budget 误报为普通空成功：

- `archive-local-tasks-row-budget-reached`：完整本批提交后累计恰为 8192；目录/记录 batchCount 递增，正常记录有完整 29 键 searchResult，但 canContinue=false、partial=true、scanComplete=false。
- `archive-local-tasks-batch-budget-reached`：整个新批不提交，不接纳有效前缀。目录保原任务、扫描/分类/警告等计数，batchCount 不递增；QML 逐字段与原副本核对，只有 queryId/status/error/相应状态标志允许变化。记录 wrapper 的 searchResult={}、本批 scannedRows/matchedRows=0，累计和 batchCount 保前批值，明确“没有发布新记录”。不能套正常 29 键要求或 previous+1。

`completed/scanComplete` 只表示该查询走到原冻结范围保留记录末尾；记录查询的 scanComplete 不提升目录完整性。partial、budget、未关联行、时钟异常行、留存警告行/分页均以文字显示；达到预算、取消或失败不允许沿用已消费续扫。空目录/空批不证明没有任务、没有事件或完整历史。

## 产品语义与安全边界

`eventCount` 是保留事件**行数**，不是去重任务次数、真实执行次数或飞行结果；跨录制/文件重复保留的同源行不作物理事件去重。`queuedTrueRows/uncertainTrueRows` 仅统计已保留且明确为 true 的行，零不表示所有行都明确为 false。`vehicleAssociationKnown` 只表示至少观察到一个明确关联机号；列表不证明任务成员完整或该任务仅由这些飞机执行。

仅显示任务哈希，不能从哈希还原真实任务名、原始 ID、任务内容或人的身份。warningRows 可重复引用同一个问题 run，并非独立丢失事件总数；原记录明确使用原 `discarded/dropped` 字段名。时钟异常保留原标志，不按墙上时间重排或用时间关联 NDJSON 遥测。

目录/记录共同 `historicalOnly=true`；`permissionsRestored/authenticated/signatureVerified/executable/completionVerified/executionPermit/flightCommandReleased/restoresAuthorization/historicalCompletenessVerified/crossSegmentAtomicSnapshot/wallClockOrderingUsed=false`。不会回灌活 BlackBox、任务、权限、规则、回放执行或命令事务。归档序列化成功不证明当前可靠性、物理真实性、任务成功率/覆盖率/收益或飞行放行。

## UI 与工程师待验证事项

本批使用 `ui-ux-pro-max` 的静态 quick-reference/pro-rules：复用 QGC palette/字体/按钮，不创建新视觉系统；显式首次/继续/取消、文字加载/失败/部分/旧结果和空态、长 ID 纯文本换行、按需展开 DTO、两级有界 ListView、StrongFocus/可访问名称及焦点滚入视区。查询按钮不以 Return/Enter 意外提交，保留标准按钮 Space/点击触发。没有动画、悬停唯一入口、颜色唯一状态或拖动唯一操作。

未执行技能搜索脚本、helper、解析器、编译、测试、UI/SQL/业务脚本、认证、仿真、硬件、刷机、提交或打包。静态阅读不能证明控件实际加载、按下/Space/取消/同步通知、焦点、读屏、主题对比度或实际读盘通过。

工程师须验证：空 sealed 首批、固定库缺失/变化、独立目录与记录交错续批、改范围/改回/切哈希/改回、来源/options/run ABA、未知回执、同步完成、其他入口占槽、取消/失败/线程实际结束、256 哈希与 8192 行两种完整批预算边界、最大关联机号目录、重复/未归属/时钟异常记录、窄屏/长字体/明暗主题、键盘 Space/Tab 和虚拟化焦点。全部仍为未执行项。

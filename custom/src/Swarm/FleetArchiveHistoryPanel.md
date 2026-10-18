# 本地事件归档与历史查询面板

## 525：旧移动维护与独立永久清理分别提示

原维护Loader、两个历史宿主及实际archive门面不变，仅更新入口文案以区分旧sealed／retired改名、外部备份和新增独立[退休原件清理](FleetRetiredArchivePurgePanel.md)。展开／收起不读取文件或删除档案；新子面板经原archive.retiredPurge窄门面显式请求预览，再独立确认删除风险。旧移动流程仍不释放磁盘，新永久清理不碰当前库、sealed段或备份。仅源码与静态设计，未编译、运行、预览、测试或操作真实文件；完整长期留存／自动清理策略及原25目标仍未完成。

## 510：运行观察事件的只读历史摘要

原当前页、封存页与跨库搜索的行摘要新增辨认 `fixed-diagnostic-summary-v3 / runObservation`，在现有PlainText换行区域显示原机号、本地MODE交接事实、低频转换、终点候选、异常位图／固定原因码、观察关闭／所有者销毁、原事件序号与累计丢弃。候选明确不等于任务完成，关闭不等于停飞；不把v3误标成旧v1工程资料缺失。原工程v2及其独立操作筛选保持。

UI仅消费已经由原生Codec／Reader校验并由本页接纳的有界历史页，再核28字段摘要、规范uint64字符串、实际Bool、闭合候选／类型与固定false边界。坏摘要显示未知，不推断否或成功；不把文件哈希／旧任务哈希变成当前Run、审批或权限。新字段不会启动录制、读盘、续扫、回放导入或调用飞机。

`capture-gap` 是整个原生事件流的缺口，空任务哈希／空目标和vehicle0有明确不同语义；它不伪装成单架任务事件，也可能不在某个taskIdHash或机号过滤结果内。累计丢弃是该原生流捕获时的计数，不是本任务损失数，更不是完整历史保证。物理时间、无线因果与正式结果不由该摘要认证。

同一只读呈现合同也用于[任务原记录](FleetArchiveTaskHistoryPanel.md)。原页面、主题、焦点和滚动不重做；UI/UX技能仅静态应用文字状态和换行，不运行检索脚本／QML／渲染／读屏／SQL或测试。详见[事件合同](FleetRunObservationEvent.md)、[实际日志接线](FleetRunObservationJournal.md)及[未执行工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 484：性能声明结果可在原历史入口辨认与筛选

原当前页操作筛选和独立跨库search下拉分别新增“工程性能声明更新”，数值索引仍显式校验，无效索引不退回全部。工程摘要判定的闭合操作集识别approve-performance；schema1/exact21、固定payloadProjection、未知三值、原source／query／continuation合同保持。

新摘要不显示或保存两数／原账号／理由／完整配置／凭据，只说明原历史操作和结果。它不是完整性能卷宗、实测证明或权限恢复；未开启归档的审批不会自动回录。展开／改筛选仍不读盘／开启录制，失败不自动查询或重试。UI/UX仅静态标签、语义状态与原键盘／主题边界，未搜索脚本、运行UI／SQL／业务、编译或测试。下方为历史。

## 463 维护入口明示单段备份

默认收起的维护按钮明示“移出／恢复／备份，不清理磁盘”，以便从原历史入口发现新增备份操作；仍只展开原[维护面板](FleetArchiveMaintenancePanel.md)，不因展开读盘、选择目录或发复制请求。备份源沿原生真实库存，显式输入既有目录、原预览和独立确认后才提交。源文件不移动／删除，备份额外占空间；副本不是任务或权限恢复。

本批仅调整这一入口文字，不改原历史搜索／任务查询／维护busy或续扫逻辑。备份接口与部分文件／实际读回语义见维护面板、[Store](FleetPersistentEventStore.md)与[Worker](FleetEventArchiveMaintenanceWorker.md)。未编译、解析、运行UI／SQL／线程／helper、实际备份或测试。下方为历史。

## 462 可恢复维护入口与统一互斥

原面板新增默认收起的[维护子面板](FleetArchiveMaintenancePanel.md)，只接同一个实际窄archive facade。展开不读盘；实际读取清单及单文件改名分别需要明确按钮，预览与独立确认不改文件。retire只是把sealed封存段改为retired，使普通历史检索不再包含；仍占磁盘。restore只恢复可检索性，不恢复任务、账号、批准或飞行许可。

sourceReady要求原生archiveMaintenanceActive为真实Bool，统一historyBusy包含该值；缺字段或实际维护线程未结束时不能读历史、开始录制或封存。Store仍独立把关。inventory／commit请求在后台启动前退休原生历史目录、页、search和两类任务续扫上下文，旧UI副本只能只读展示，不能借同值恢复续页资格。实际finished才释放维护槽，accepted／resultReady／取消请求不等于线程结束。

维护契约、单文件原生无覆盖改名、30秒提交窗口及不确定结果见[Worker](FleetEventArchiveMaintenanceWorker.md)、[Store](FleetPersistentEventStore.md)与[面板](FleetArchiveMaintenancePanel.md)。461保留任务查询的身份、预算和独立续扫语义不改变；本批未编译、解析、运行UI／SQL／线程／helper或测试，也没有实际扫描、改名、提交、打包或放行。原25目标与六类实质剩余保持。下方为历史。

## 461 留存任务目录与原记录子面板

原历史面板新增[任务历史子面板](FleetArchiveTaskHistoryPanel.md)，仍使用同一个实际窄archive facade。该facade新增只读historyTasks／historyTaskRecords与tasks(intent, confirmed)，实际调用原Store的requestLocalArchiveTasks；没有暴露Store、路径、SQL或可写任务对象。原当前页、封存浏览、独立工程操作search及其旧schema1结果不改作任务目录。

任务目录只能来自原生实际保留行累计；选择已有taskIdHash后，原始记录由原生Reader跨文件精确筛选，而非本页JS筛选。两类查询及原search保持各自作用域／实际查询身份与续扫资格，共用原真实读取线程槽。显式首次／继续／取消，展开和改变条件不读盘，失败／取消不自动重试或恢复旧cursor。

目录和记录都是历史诊断：完整任务状态／成员summary此前未保存就保持未知，不据数量补任务成功率、覆盖率、续航或ROI，不关联NDJSON runId。部分／预算／末尾不是全历史或跨库原子快照。具体字段和预算见[Store](FleetPersistentEventStore.md)及[Reader](FleetEventArchiveHistoryReader.md)。

UI/UX技能只用于现有QGC主题、文字状态、键盘／焦点、按需展开及有界列表；本批未执行脚本搜索、QML、UI、SQL、应用、编译或测试，视觉／读屏和真实交互留给工程师。本批不恢复任务、账号、批准或飞行权限；原25目标及六类实质开发继续。下方为历史。

## 424 独立跨库保留记录搜索

在原有当前页筛选旁增加可收起的独立搜索区；原 `vehicleFilter/operationFilterIndex` 仍只过滤收到的单页，不能替代跨库查询。新范围为 `sealed`（已封存）或 `sealed-and-offline`（加停止后的固定未封存库）。两种范围均要求原生 writerActive=false、captureEnabled=false、读取器与查询槽空闲；包含未封存时该库必须存在且满足只读条件，不悄悄忽略缺失库。展开面板、改变条件不会读盘、停录、清理或自动查询。

[GuidedActionsController](../../res/Merivus/GuidedActionsController.qml) 的原窄 facade 只增加只读 `historySearch` 和 `search(intent, confirmed)`，不暴露原 Store、SQL、路径或原生 cursor。搜索 intent 恰六字符串字段：action=`search|continue-search`、expectedRunId、scope、vehicleId、operation、previousQueryId；首次 previousQueryId 为空，继续只提交上一份本页已接纳结果的 UUID。机号输入为空或 1–255；**native 结果 vehicleId 为整数 0–255，0 表示全部**，UI 显式转换核对，不能把字符串 intent 与整数结果直接比较。操作为空或四种工程审阅操作。请求仍为原生生成 UUID 的 9 键 accepted 回执，pending kind 恒为 `search`；取消复用原 action=cancel 入口及该实际查询 ID。

按下时冻结实际 facade、界面 epoch、固定 options、当次 run、筛选版本与原 savedSearch/nativeResult 对象。调用前再次核对，并先冻结 ownedQuery；即使原生同步通知先到，仍须实际回执 ID 与 lastHistoryQueryId、archivedResultAccepted、独立 historySearch 作用域一致才接纳。未知回执不补成功、不自动重试。每次发起搜索消耗旧结果的本地续扫资格，失败/取消后保留旧展示但须明确从头搜索；原生独立消耗真实 continuation，UI 无权拼游标。筛选改动使用单调版本，改回原值也不会复活旧续扫；其他入口替换原生最新结果后同样不能沿用旧搜索。

搜索结果独立保存为 savedSearch，不覆盖 savedPage/catalog，不无限追加。校验 native 28 键信封、最多 64 个 7 键匹配封装、最多 4 个 18 键扫描覆盖项、本批扫描/匹配计数、固定文件身份、行号归属及历史/非授权字段。未完成的 nextCursor 为原生 5 键值，仅展示在只读 DTO，不发送回后端：afterRowId="0" 表示下一文件起点，epoch/hash 必须为空；非零才要求实际 UUID/小写 SHA256。完成时 nextCursor 为空且 filesCompleted=filesTotal。首范围无封存库可合法返回已完成空结果，组合范围不能以缺失未封存库伪装空结果。

界面展示本批扫描行/匹配行、已越过/范围文件数、scanComplete、本批覆盖区间、清理计数及游标是否早于保留区。异常退出/丢弃/拒收字段是**本批被扫描行引用相应问题 run 的行数**，不是独立丢失事件总数。每条匹配显示文件名、文件序号与实际 rowId，旧 v1 缺失操作继续标为未知，不补造审批详情。空匹配批次不等于整个范围无匹配，最后一批无匹配也不抹掉前面批次；本页不累计总命中数。

scanComplete 仅指本次冻结清单下保留记录到达末尾；非全历史证明、非当前新鲜度、非跨库原子快照，不恢复账号、批准或飞行权限。匹配列表按页虚拟化，覆盖项最多四条；纯文本输出，完整有界 DTO 按需展开。UI 技能用于独立条件标签、显式首次/继续/取消、文字状态和禁用原因、分批/缺失边界及主题控件复用，未以静态阅读声称视觉或交互通过。

424 静态阅读已对照实际 Reader 的 request/result/coverage/cursor 与 Store 的 6 字符串入口、9 键回执、latest/query 归属、native 续扫消耗及取消槽。只编辑源码/说明并做文本检查；未编译、测试、运行 helper/SQL/UI、认证、连接设备、提交或打包。工程师仍须验证 Qt QML 绑定/可访问性、实际读盘及失败/取消/重启/文件变更情况。

## 423 停止后的未封存库只读入口

在 422 显式录制/封存/历史查询入口之上增加 `offline`，不改变 facade 或 6 键 intent / 9 键回执。按钮明确为“读取停止录制后的未封存库（只读数据库）”，仅在原生 `writerActive === false`、`captureEnabled === false`、查询槽及读取器空闲和原 facade 可用时开放。停止请求的 accepted 或状态文本不代替实际录制线程退出；无需连接飞机或飞行账号。界面显示 writerActive 的是/否/未知，不以未知当作空闲。

读取固定 `fleet-events-v1.sqlite`，不接路径、库名、SQL、恢复、初始化或清理入口。点击即明确确认数据库只读和创建/释放协作锁；Qt 的锁管理可能回收死进程锁，不应解读为整个目录零写入。遗留 `-journal`/`-wal`/`-shm` 或占用/变更会拒绝，不删除旁路文件，请保留原文件交工程师处理。不会自动停止当前录制来满足查询条件，也不自动重试。

首游标 `"0"` 不预知磁盘 epoch/hash，结果必须给出实际规范 UUID 和小写 SHA256，不能因为本进程未启动录制而要求数据库 epoch 为空。成功页还要求固定 basename、`offlineCurrentOnly=true`、`sealedSegmentOnly=false`、`cooperativeOwnerLockHeldDuringRead=true`、空 runFilter、无 catalogId/sealRunId，以及历史/非授权标志；不会把当前批次页或封存页当成未封存页。runFilter 为空允许查询此库保留的各历史 run；`expectedRunId` 仍仅冻结发起动作时本 Store 的 run，不是历史 run 限制。

每页至多 64 行，筛选仅本页。后续页必须保持本页持有的原 facade/options、原 savedPage 对象、原生查询 ID、nextRowId、databaseEpoch 和整文件 fileFingerprint；原生独立保持其真实游标，UI 不向其传入任意 hash/epoch。按下到释放及同步通知尾核再次确认线程/捕获关闭和页身份。整文件变化时旧页保留为只读快照，提示重新读取第一页，不把旧游标自动用于另一库。

结果采用本次 reader ID、`lastHistoryQueryId` 和 `archivedResultAccepted` 三者归属；首读成功即可展示实际 epoch/hash。offline 的取消复用实际读取器 ID/类型，只有本页持有的尚未接纳查询可请求取消；原生槽继续等待真实 reader 结束。错误仅取匹配本页 ID 的 reader，不冒用当前录制器或另一入口错误。新分支没有更改 catalog/current/sealed 的正常入口；current 仍不要求封存读取器接受标志。

423 仅静态对照 Store 与 Reader 的实际分支、回执/页面字段、首读及续页身份、取消占用、错误归属和 Qt/QGC 按钮源码；没有运行代码、编译、测试、SQL、helper 或 UI。技能 `ui-ux-pro-max` 用于显式读盘与协作锁告知、空闲条件解释、纯文本错误/旧页提示、长按钮换行和复用调色板；视觉、焦点、读屏与真实持久化效果仍未验证。

## 范围与实际入口（422 基础，423 扩展）

[面板](../../res/Merivus/FleetArchiveHistoryPanel.qml)仅接收 `archive` 窄facade：current、historyCatalog、historyPage、historySearch、options及 request(intent, confirmed)/search(intent, confirmed)。Root负责资源、指挥中心与区域任务规划页接线；面板不接原Store、文件/SQL接口、飞行对象、账号或旧批准恢复接口。没有机队连接条件，默认收起、不读盘、不自动启用。

固定目录与策略由原生提供：AppData/fleet-os/event-archive；活动库100000条/256 MiB，队列512条/8 MiB，批64条、忙等250 ms，历史页2 MiB；每页请求64行、封存上限32段。限额不是完整性或可用磁盘空间保证；封存文件累积，不自动删除、清空或清理。

## 请求与确认

intent恰6个字符串键：action、expectedRunId、catalogId、basename、afterRowId、queryId。action仅start/stop/seal/catalog/current/offline/page/cancel；expectedRunId取按下时原生runId，无关字段保持空，afterRowId默认字符串0。offline 的 catalogId/basename/queryId 为空，只有cancel携带本页持有的queryId，其他请求ID由原生生成。回执schema1/action/accepted/requestId/reason/requestOnly/historicalOnly/permissionsRestored/flightCommandReleased中，requestOnly/historicalOnly为true，权限与飞行字段为false。

回执恰9键且按动作核对：start/stop/seal成功或拒绝都没有查询ID，要求requestId为空；catalog/current/offline/page接受须有原生规范UUID，拒绝可在分配前为空或在分配后带UUID；cancel接受须回原查询ID，早期拒绝可为空。不把控制动作成功的空ID误报成未知，也不把请求接受当作操作完成。

start必须先明确勾选默认关闭的持久化隐私/磁盘/丢失风险确认；seal另有默认不勾选的封存及继续录制风险确认。stop仅请求停止与排空，不删除文件。所有动作均显式点击，Enter不会直接提交；读目录或页面不启停录制，不需要飞行账号。accepted只表示请求接受，不是写盘、读页、封存、续录或持久化成功，不自动重试。

current 只在recording时请求，限定当前录制run；offline 在录制实际停止后读取未封存库保留的各run。封存目录按原生catalog条目选择，不接受手填文件名；首游标0，后续游标只来自本页已核实的原生页，不自动跨库/段。机号与四操作筛选只作用于本次已读页，明确不是全库搜索。

## 本页归属与结果

面板冻结原facade、选项身份、按下时run/databaseEpoch、目录/段/游标和原生requestId。本次成功必须原生lastHistoryQueryId匹配本页ID，目录/页的实际格式和作用域一致；封存查询还核archivedResultAccepted，当前库查询不误用该封存专用标记。共享historyPage非空不能代替本页成功；旧页只读保留并区分是否仍是当前原生结果，旧目录/游标不能当新请求依据。

按下到释放之间，以及清空确认框等同步属性通知之后，再核原目录、所选basename、databaseEpoch、sealRunId；下一页还要求仍是原savedPage对象及其nextRowId，不在选择变化后继续旧请求。尾核保持callBusy，避免为了查询可用条件而短暂解锁。当前页的“仍为最新”同时核实际DB epoch/runFilter；封存页再核sealRunId、sealedSegmentOnly和空runFilter。

busy由historyRequestPending和archivedReaderActive决定，不把取消请求当作原生槽已释放。只有明确点击才能请求取消本页已收到ID的查询；隐藏、收起、来源变化和销毁均不调用全局cancel或停止录制，不能误取消另一入口。原生仍决定实际槽与范围。生命周期plain alive/epoch/ticket保护跨同步通知返回，不对销毁后的界面写值。

取消按钮不面向已接纳结果，要求pending kind一致；封存查询还要实际archivedReader.queryId匹配本页ID，当前查询要求原run一致，原生继续核其实际pending ID。读取器的state/errorCode仅在该快照queryId等于本页ID时显示为本页状态；不使用录制器的全局errorCode冒充封存读取错误，不把另一入口的旧读取器错误归于本次查询。未取得匹配快照明确标未知，不自行重试。

## 工程摘要与边界

新v2的record.event.payload.engineeringReview恰21键。面板显示四操作、机号、mutationCommitted/nativeOk/stillCurrent的yes/no/unknown三态、比较已知/截断三态、三项有类型哈希、变更总数/已展示行数/能力数及审阅ID哈希。未知不转成no，旧v1无详情时明确未记录，不从哈希补造批准、理由或身份。自由理由、SID、原始构型与原始载荷均不存于该摘要。

列表使用ListView按页虚拟化，收到的有界DTO仅按需展开、PlainText且不额外截断；完整DTO不等于全库或全量飞行历史。历史丢失/清理、异常退出和游标早于保留区仍需阅读原生状态，不以空页证明没有事件。全部记录historicalOnly，不恢复权限、不认证硬件/性能、不发送飞行命令。

本批仅源码与静态阅读，未编译、测试、执行helper/SQL/认证、启动或渲染界面、连接设备。使用ui-ux-pro-max静态交互规范：显式确认、默认关闭、键盘焦点、文字状态和按需披露；布局、Qt运行、对比度、读屏和真实持久化/分页/取消仍由工程师验证。

静态复核对照了实际Store的9键回执、run过滤、目录/页作用域、读取器queryId与取消槽生命周期；只作源码阅读和diff空白检查，不构成上述路径已运行或可发布的证明。

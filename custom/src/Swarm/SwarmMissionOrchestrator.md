# SwarmMissionOrchestrator native execution observation bridge

## 546 当前：本机 Run 检查点与独立只读恢复

本功能只保存本实际 Orchestrator 已提交的全部受保护原生 Run 最新快照，不保存 generic task，不把低频事件重放、QML Map、报告文件或机载候选重新登记为任务。完整只指该冻结代次的全部 native record16；不是逐包／逐边沿完整历史、掉电零丢失、远端实例、任务完成或恢复继续执行证明。原保守占用恢复和既有闭部署归档不被替代。

### 显式开关与后台提交

默认关闭且不自动读取文件。固定文件为 `QStandardPaths::AppDataLocation/fleet-os/fleet-run-checkpoint-v1.json`，无 caller 路径、源 Map 或 SQL 输入。`setRunCheckpointEnabled(enabled, confirmed)` 必须明确确认；开启表示允许今后持续原子替换这个固定本地槽，包括原有旧检查点，可能以当前空 Run 集合替换旧材料。应先显式读旧历史再决定开启。关闭只停未来自动写，不取消或回滚已受理写；取消须另用原 job ID。

只有私有原生 record 已提交后的通知排队入口，以及已归档原生记录整批真实退休，才标 dirty／递增独立非回绕 UInt64 generation。generic 任务信号、Root 转发、审计重放及外来文件不能标记。原观察模型每次提交可能合并多个机载边沿；本功能不提升其采集频率或原真实性结论。

开启后500ms合并窗口捕获当时所有 record；GUI只复制 COW 的纯值、ID和小时间文本，不调用含 selection JSON/hash 的旧目录捕获函数，也不生成报告／JSON或做文件IO。cc 内 parentless、无 `Q_OBJECT` 的一次性 worker 从冻结值计算原 selection token、复用 `FleetMissionExecutionReport::buildProtectedRunObservation`，构造全部 protected8 后保存。原 TaskRecord 后续更新不改已冻结快照，也不能否认已经写入的旧代次；`sourceCurrent=false`／`dirty=true` 明示较新提交尚未保存。

每次仅一个 worker，500ms不重复排队任务，实际原 `finished` 前不释放槽。期间再 dirty 只保存一份最新待捕获意图，原 worker 结束后再合并。失败／取消的同一 generation 不自动自旋重试；新 native 提交可以产生下一代尝试，操作者也可显式重试。generation耗尽停止新自动保存，不回绕；保存结果不能恢复观察 revision 或权限。

保存／读取均持固定文件 `.lock` 协作锁，`setStaleLockTime(0)` 关闭按文件年龄接管，不提供强制解锁／显式删除其他锁的接口；失败须人工核查，不自动重试。Qt自己的进程存活锁判定不构成锁所有者认证。锁不是恶意非合作改写、跨进程所有者认证或原子全机队历史证明。worker保存可创建固定的应用数据子目录，不创建任意 caller 目录；拒最终目录 symlink、异常目标和共享文件层拒绝项。多实例只有逐操作锁，不宣称一个实例的集合是所有进程的全集。

### API 与31／15／8／9／6值合同

属性 `runCheckpointStatus`、`runCheckpointHistory` 共用 `runCheckpointChanged`，通知始终异步，不把文件工作外调嵌入原 Run commit 栈。所有新方法和 getter 仅在本 owner 线程可用；不向 QML 暴露 worker。

- `bool setRunCheckpointEnabled(bool enabled, bool confirmed)`；同值是 no-op，不发假写入。
- `QString requestRunCheckpoint(bool confirmed)`：要求 enabled、空闲、未耗尽，显式保存／重试；非空 UUID 仅表示受理。
- `QString loadRunCheckpointHistory(bool confirmed)`：空闲时显式读取同一固定文件，无需启用、飞机、账号或活 Run；成功后才原子替换独立历史槽。失败／取消保旧历史。
- `bool cancelRunCheckpoint(jobId)`：只请求实际当前原 job 合作取消，不保证已经终止、没有文件或已回滚。析构仅请求原 worker 取消，不 wait／delete 正在运行的线程；原 finished 独立连接 deleteLater 回收。
- `runCheckpointHistoryPage(historyId, offset)` 与 `runCheckpointHistoricalReport(historyId, index)`：必须原成功 load UUID；索引是真实整数数值，拒 bool／String／null，坏请求 `{}`。同文件、同ID／hash再载入也获新 historyId，旧页不能重基到新槽。

status 固定31键：`schemaVersion=1,enabled,dirty,busy,jobId,action,stage,cancelRequested,ok,error,filePath,sourceId,currentGeneration,capturedGeneration,savedGeneration,sourceCurrent,recordCount,checkpointId,contentHash,fileWriteAttempted,fileWrittenKnown,fileWritten,readbackVerified,readbackError,fsyncVerified=false,durable=false,historicalOnly=true,authenticated=false,permissionsRestored=false,executionPermit=false,flightCommandReleased=false`。字符串显式非null；generation为规范 UInt64 String，current初始1、captured/saved初始0；recordCount实际整数0..128。action为空／save／load，stage为idle/running/cancelling/complete/failed/cancelled。sourceId/currentGeneration始终属于本活 Orchestrator；capturedGeneration是该job受理时本地代次，load不据此声明文件当前。sourceCurrent仅对原save冻结代次与当前源代次仍相等时为true，不是durability或飞行许可。

保存复用 [FleetSnapshotFileIo](FleetSnapshotFileIo.md)：`ok=true` 表示实际 QSaveFile commit 已报告成功；readback 失败或迟取消仍保 `fileWritten=true`、单独 `readbackVerified=false/readbackError`，不能说回滚。savedGeneration记录已报告commit的冻结代次，不冒充fsync；commit不确定则fileWrittenKnown=false，不能由fileWritten=false补“没写”。读取不写输出。尚未收到准确终态的UI不能补结果；busy与异步结果通知先后由原job匹配，不认shared latest为本页成功。

history初始 `{}`；成功固定15键：`schemaVersion=1,historyId,filePath,checkpointId,sourceId,generation,capturedAtUtcMs,recordCount,contentHash,readOnly=true,historicalOnly=true,authenticated=false,permissionsRestored=false,executionPermit=false,flightCommandReleased=false`。historyId为本次load原生UUID，checkpointId/sourceId/generation/UTC来自文件声明，未认证；不是重启后本机活Run、当前权限或任务结束时间。

page 固定8键：`ok=true,error="",historyId,offset,nextOffset,totalCount,hasMore,records`，最多4条、offset允许终端count。row固定9键：`index,taskId,runId,observationRevision,observationClosed,observationOwnerDestroyed,modeQueueCommittedObserved,targetVehicleIds,observationUpdatedAt`。历史report固定6键：`ok=true,error="",historyId,index,report,jsonText`；report为原protected8（v1/v2解码兼容），完整JSON已在worker构好，getter不全树序列化。不能将该历史report传给原生注册／StartPermit／占用解除／恢复任务；数值／候选／null与现有protected8语义保持。

### 文件、预算与整份拒绝

检查点根固定10键：`format="merivus-fleet-run-checkpoint",schemaVersion=1,checkpointId,sourceId,generation,capturedAtUtcMs,recordCount,reports,contentHash,historicalOnly=true`。contentHash是去本字段后规范JSON的SHA-256；只检内容自洽，不是签名。读取严格根字段、真实数值／Bool、UUID／正代次、全部报告与同sourceId、taskId/runId唯一及count相等；原 protected8 codec 校验每份schema/hash/null/uint32及否定claims。所有报告通过才安装，不返回成功前缀。

整文件32MiB，最多128个native Run，单报告复用2MiB限制；GUI不先构造128×大JSON。worker先以深20和累计32Mi下界单位遍历冻结纯值，再逐份构报告并累计真实canonical字节（保留2KiB根开销），最后全文件实际字节再核32MiB；超界整个job失败，旧文件不被前缀替换。load按共享层普通非symlink文件、64KiB完整读／尾部／UTF-8检查，再做深度和累计值预算后才canonical/hash；解码报告的canonical JSON累计同样受32MiB减根开销限制，避免128×2MiB文本暗中另增无界槽。坏尾／坏末报告／重复／源不一致均整份拒绝。每步间合作取消，Qt JSON/单报告codec内部不声称可即时中断。最多保存有界报告树及对应完整JSON文本，不把32MiB文件上限称为全进程峰值或时间保证。

已完成实际commit后，即使来源更新、关闭开关、取消或owner销毁，仍不能说文件被撤回。进程崩溃可能丢失最后合并窗口或在途结果；QSaveFile／readback不等fsync或真实掉电持久化。历史窗口不重建observer/PRIMARY/任务/批准/租约，也不清保守restart holds。A4备份留存编排不在本功能中实现。

工程场景（全部未执行）：默认off不IO／不开机自动读；明确覆盖及空集合；native提交dirty与generic无影响；500ms合并／后台更新旧snapshot；128／32MiB／单2MiB／深度／坏末项整拒；同代失败无自旋与显式重试；开启/关闭/在途取消／latecommit+readback失败；原job finished前忙与析构；锁冲突、目录/目标symlink、磁盘满、generation耗尽；全record modeexit/null/uint32保留；重启load后无活任务／占用／权限恢复；四条目录、原historyId与同内容重载ABA、完整报告纯文本。仅源码编辑和直接静读；未执行实现、JSON/hash/IO/线程/算法、编译、测试、预览、设备或打包提交。

## 542 当前：编队待办的有界占用投影

新增仅 C++ 可调用的 `formationQueueAssignmentSnapshot()`，在原 Swarm/Orchestrator 双向关联仍有效且同线程时，直接读取 TaskRecord。generic running/paused 与 native `!observationClosed` 均保留占用；按创建顺序为每个 system ID 保首个阻塞任务，输出最多255条 `{vehicleId, taskId, taskPhase}`，不复制全任务元数据或完整 Run 观察。空返回表示来源未知，不能当作没有冲突。

该投影仅供[编队等待队列](FleetFormationQueue.md)开始重新审阅；不创建任务、不释放任何占用、不恢复机载控制权。实际派发仍新建/启动原 formation task 并重新走原 Swarm 健康与协议门。暂停、模式退出、已观察终点和队列取消不会通过此接口变成任务完成。

只做源码与静态阅读，未编译或测试。下方为此前模块历史。

## 541 当前：有界任务模式退出观察

实际 native TaskRecord 的目标结果升级为 schema2；语义快照增加 `missionModeExitObservation`，保计数及首/末退出的原 HEARTBEAT custom/base/status 和前后接收引用。首个异常位已置位后，后续计数/首末变化仍参与相等比较。采集仍在原模型提交检查点，多个边沿可合并；这不是逐包或完整逐边沿历史。

normal runObservation 升级 exact29/schema2，capture-gap 保原 exact28/schema1；原临时发布回执、4096字节单事件/512行/2MiB FIFO、丢弃计数和异步发布边界不变。保留的 sourceEpoch 是本地接收实例 UUID，不是远端身份认证。退出不改变四个终点候选、不释放占用、不发送指令。受保护 record16 当前 schema2，轻目录仍 schema1。

具体格式见[共享值合同](FleetMissionModeExitObservation.md)、[审计版本合同](FleetRunObservationEvent.md)和[保护报告](FleetProtectedRunReport.md)。本次仅源码与直接静读；未编译、执行或测试。下方旧版本描述保留为历史。

## 524 当前：原生归档后的精确protected记录退休

新增仅供实际Episode/Lifecycle使用的私有 `captureClosedExecutionRecord(actualRun, taskId)` 与 `retireArchivedExecutionRecords(exactRecords)`。前者只冻结本实际TaskRecord，要求原Run owning-thread/QQ相同、closed、非busy、owner未destroy；后者最多9条，先逐条核实际原owner、唯一taskId、完整record16和原死亡连接，全部相同后才整批移除。没有QML/generic removeTask/导入文件通道，不能按ID删未知任务，也不把任意文件或hash当退休授权。

实际退休同时移除同一 `_tasks/_taskOrder`、精确owner死亡连接、pending通知集合及逐task语义缓存，防长期循环残留无界缓存；已发布的值事件、既有FIFO/gap及历史文件不改。异步死亡/更新回调还须原完整identity或实际owner匹配、任务仍存在；已删记录不会被迟到回调重建，新UUID不复用。不释放Swarm占用、不停止Run/飞机、不制造Completed/Failed/Cancelled，也不改变原128条上限；容量仅由已完整归档且原生尾核的专用批次回收。

Lifecycle必须先原独立Close或严格never-began例外、全原生quiet与整张实际owner表无占用、worker完整写/readback及双终结，再按原全record/原对象尾核调用本方法。旧Episode全体实际后代真正销毁后才回收其8槽，与本记录容量是不同事实。见[Lifecycle 524](FleetMissionDeploymentLifecycle.md)及[归档worker](FleetDeploymentArchiveWorker.md)。本批仅源码和限定静态审阅，未编译、测试、运行实现、操作设备或放行；下方“闭历史永不移除”是旧政策历史，仅本专用已归档退休路径构成明确例外。

## 511 当前：begin真实提交与write后的commit分开

[Observation的511合同](FleetMissionExecutionObservation.md) 已在 `beginExecutionQueue` 安装本次原queue、queueBegan／sourceActive／tracking后，立即调用实际Orchestrator的受保护模型更新。只有该提交成功且原Observer QQ、安装后的ReadContext／mutation serial、原对象与queue均保持，且未退休／耗尽／commit、仍tracking／sourceActive，begin才true。它确认本地模型检查点，不证明已write、ACK、远端MODE或live来源一直有效。

完整 `TaskModelEvidence` 仍会经nativeTargetCurrent／nativeBindingCurrent调用Link虚函数，不是无回调pure getter。begin在调用前持已安装Target副本并离开内部引用作用域；回调后先查QQ／原ReadContext，再按原queue与对象重新定位。模型拒绝或原上下文在回调中变化会返回false，但不能reset已经安装或已经提交的事实，也不能覆盖回调推进的新状态。

实际Vehicle caller之后仍逐项核同observer／slot revision／epoch、BindingMatches、authorization和immutable，真正write之后才按原QueueContext记录queueCommitted。write内或之后的源退休、取消、销毁及late commit不撤销真实调用事实；模型更新失败不变成回滚证明。原其它updateTaskModel callers可以忽略新的bool，其历史／清理语义保持；无新发送、占用释放或完成状态。

因此510低频事件链现在可在两个实际提交点分别冻结began与commit，即使旧公共通知稍后合并，也不必从flush时最新TaskRecord倒推。原FIFO／gap／异步发布／临时回执／原schema与权限边界不变；缺口或未录制不保证两条事件一定交付或持久化。

下节510中“begin本身不提交、began与commit可能合并”的段落是当时限制的保留历史，**当前以本511节为准**；不补写过去未曾捕获的begin时间。该改动不是StartPermit、真实远端执行实例／正式结果认证或放飞。本批固定2源码＋13文档＝15路径，无新路径、UI、资源或依赖；[28项工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，原25目标／六类实质剩余及40%／60%源码工作量粗估保持。仅直接静读和普通文本核对，未编译、运行helper／解析器／业务／QML／认证／SQL／hash实现、测试、仿真、设备／PX4操作、刷机、放行、提交、更新PR或打包。

## 510 当前：真实受保护 Run 的低频语义事件

仅实际 native TaskRecord 在注册、更新、强制观察关闭或 owner 销毁完成私有提交后，冻结逐机语义变化到独立 FIFO。不是重新实现观察报告、正式结果或另一个任务模型；不把任何候选、源中断或本地关闭改为 Completed、Failed、Cancelled，也不释放占用或发飞行命令。旧 TaskRecord、完整观察／报告结构、原观察通知的 coalescing 均保持。

### 来源与临时回执

- `executionObservationEventReady(sourceEpoch, sequence, payload)` 仅从异步 flush 发出，不在原 native 更新栈同步通知 Root。
- `executionObservationEventReceiptMatches(sourceEpoch, sequence, payload)` 是纯临时匹配：同实际 Orchestrator、独立事件 epoch、正在发布的原序号及完整原 payload。信号结束后该 receipt 消失，历史 Map、导入值或相同 hash 不能重领。
- `executionObservationEventStatus` 为只读状态，NOTIFY 同样只在异步 flush 发出。没有新任务、账号、Run、结果或发送授权。

sourceEpoch 是本实例独立事件 UUID，不与保护报告的 sourceId 混用。序号是独立正 UInt64 十进制字符串，不靠可能耗尽的 observationRevision 作唯一事件身份。transitionSourceHash 是实际事件 epoch 的 SHA256，runIdHash 是原 native Run ID 的 SHA256；只是本地关联值，不是匿名化、签名或远端身份认证。

### 实际触发与历史语义

每个受保护 task／vehicle 首次产生 `registered`；此后只比较 mode queue began／committed、四类 outcome candidate、continuityUnknown、原 reason／source-retirement code、八位异常历史、observerClosed 和 ownerDestroyed。普通 sourceCurrent／window／age／鲜度、接收或重复包计数、observationRevision 和轮询时钟不参与差异，不将每250ms快照转为事件。

同次 native 提交发生的不同中间语义在各自提交时冻结，不到通知 flush 时从最新 TaskRecord 倒推。已存在成员变化用 `changed`；关闭成员用 `closed`；owner 销毁用 `owner-destroyed`，仍保原候选和已发生 queue 事实。owner 死亡可在此前已关闭之后补充自己的不同事实，不重开观察。

只记录实际 TaskRecord 已提交可见的状态：原 `beginExecutionQueue` 本身不提交任务模型，正常同栈的 began／commit 可以在后来同一次模型提交中同时为 true。此链不会补造一个单独的 began 时刻或其间每个原始帧，也不提前改变原 queue 栈的更新节奏。

anomalyMask 为 Int 0..255，同 task／vehicle 仅在新私有 lastSemantic 中对真实观察作历史 OR，不回写 TaskRecord／targetOutcome；后续普通值不能清掉曾观察到的位。

| 位 | 曾观察到的事实（不是原因或正式结果） |
| --- | --- |
| 0 | currentSequenceRegressions > 0 |
| 1 | currentSequenceJumps > 0 |
| 2 | missionModeDepartures > 0 |
| 3 | homePayloadChanges > 0 |
| 4 | noMissionReported |
| 5 | totalKnown 且 totalMatchesExpected=false |
| 6 | completeStateReportedNotVerified，即曾报告 COMPLETE 但未认证 |
| 7 | sourceRetirement.observed |

仅 bit6 不强制 reviewRequired；continuityUnknown、非空原 reason、其它异常位、owner 死亡或 capture-gap 要求人工复核。复核要求不是事故判定、远端中止或可接管认证。原 code 仅允许实现中的固定 `execution-*` 闭集；未知文字转 `native-diagnostic-code-unrecognized`，不把 arbitrary reason、startup 文本或用户文本搬进事件。原空 reason 规范为空 QString。

### 固定值合同

正常 payload exact6：`taskId/startupId/deploymentId/targetVehicleIds/vehicleId/runObservation`。前三是原实际 UUID；targetVehicleIds 保原1..32个唯一 Int 1..255，vehicleId 为其中一员。无原包、坐标、URI、账号凭据或完整观察树。

runObservation exact28：

`schemaVersion/transitionKind/transitionSourceHash/transitionSequence/capturedAtUtcMs/observationRevision/revisionExhausted/runIdHash/outcomeCandidate/modeQueueBegan/modeQueueCommitted/continuityUnknown/anomalyMask/reasonCode/sourceRetirementCode/observerClosed/ownerDestroyed/reviewRequired/droppedTransitionCount/snapshotOnly/historyComplete/remoteMissionInstanceVerified/missionCompleted/resultVerified/executionVerified/executionPermit/flightCommandReleased/permissionsRestored`。

schemaVersion=Int1，kind 为 `registered/changed/closed/owner-destroyed/capture-gap`。capturedAtUtcMs 是 native 冻结时的 qint64 UTC，范围1..2^53−1，不是远端事件时间，也不保证系统钟单调。BlackBox 入日志 UTC 与它分开，不能将延迟补记误说成严格时间线。普通 observationRevision 为原正 UInt64 字符串，revisionExhausted 保原耗尽或最大值；不据同 revision 猜事件唯一。counter／sequence 为规范十进制字符串，其余旗标是 actual Bool。snapshotOnly 恒 true，historyComplete 与后七认证／结果／许可字段恒 false。

`executionObservationEventStatus` exact12：`schemaVersion/sourceEpoch/sourceHash/lastSequence/queuedEventCount/queuedPayloadBytes/droppedTransitionCount/pendingCaptureGap/sequenceExhausted/flushScheduled/snapshotOnly/historyComplete`。schema/count 为 Int，queuedPayloadBytes 为 qint64，lastSequence／dropped 为可零 UInt64 字符串；lastSequence 是已分配最高序号，不承诺全部已交付。snapshotOnly=true、historyComplete=false，状态不会认证源、交付或 durable。

### 预算、缺口及生命周期

FIFO 最多512行，累计 canonical payload 最多2MiB，单 payload 最多4096字节，每异步 turn 至多发64行。该字节预算不是 Qt 真实堆内存上限。lastSemantic 只保最多128个原 native task 各32成员。每行通过共享纯值 `FleetEventArchiveCodec::runObservationPayloadValid` 后入队。

先弹出本行并减少排队字节，再发布该行的临时原生 receipt 和信号；通知返回先核 self QPointer，再只清原 receipt，不跨信号保 TaskRecord 或容器引用。新提交可在通知内追加，但不能覆盖正在发布的原行，也不能通过嵌套事件循环反复发布它。

超预算、无有效捕获时间或无法表示的事件只增加饱和不回绕 droppedTransitionCount，并保 pendingCaptureGap；原任务提交、发送和旧通知不回滚。不淘汰旧队列给新事件让位，也不在后续同值轮询反复补发已丢语义。排队空间可用时追加全局 `capture-gap`：三个 ID 明确空串、targets=[]、vehicleId=0、runIdHash空、observationRevision='0'、candidate=`not-applicable`、continuityUnknown/reviewRequired=true，其它 queue／closed／owner／异常事实保守 false／0。这个 gap 不冒充某一 Run 的异常或已恢复。

序号达到 UInt64 最大值后永久拒绝分配，不回绕；剩余原队列仍可排空，pending gap／sequenceExhausted 留在状态中，不能凭空制造可用新序号。无有效本机捕获时间时也不会无休止零延迟重试；等待后续真实提交再尝试。关页、停 BlackBox、未启持久化、入队／写盘失败或退出仍可丢失后续历史；本源不将发出信号说成 BlackBox 接纳、SQL commit、完整存档或正式任务结果。

真实 Root 接原 receipt 后进入已有 BlackBox 和默认关闭的持久事件链，固定投影及现有按 taskIdHash 下钻只保本地历史。完整任务远端实例、正式结果、任务与遥测档案的可信关联以及原25目标／六类实质剩余继续。所有本批工程场景均未执行；未编译、运行 helper／解析器／业务／QML／认证／SQL／hash 实现、测试、仿真、改飞控、设备操作、刷机、放行、提交、更新PR或打包。

## 492 当前：Evidence回调后重新定位原完整TaskRecord

register／executionObservationMatches／executionObservationRetirementMatches／updateExecutionObservation均在owner.taskModelEvidence前固定self与actualOwner QPointer；回调后先核两者存活和原线程。三处已有记录路径还在回调前取完整TaskRecord副本、让旧QHash指针离开scope；回来按值taskId重新find，再逐项比较原24字段，不能跨回调延用旧元素指针。

private sameExecutionTaskRecord包含任务id/type/phase、成员／metadata／reason／progress、普通revision及四时刻，native标志／owner、完整executionIdentity／observation、observation修订及两时刻、modeCommitted／busy／closed／ownerDestroyed／exhausted。比较不只靠身份或可能饱和的修订。无关任务插入导致QHash重排时，同一未变记录仍可匹配；原记录在回调中实际推进／关闭时，外层旧结果拒绝，不覆写内层更新。

register的destroyed连接先本地暂存，connectNotify后先QQ／线程，再复取完整六项Evidence并与初样逐值相同，随后重新查重、容量、id与占用冲突；全部通过才插入记录及采纳连接。失败只断开本地未采纳连接；nested已经注册的同Run可返回其原id，不覆盖或删除内层记录。register回调前没有持task指针；不会因同Run重复调用额外建任务。原最大修订强制关闭／owner销毁、闭合后late commit补历史、queued通知及历史保护报告合同保持；不新增发包、完成、删除记录或释放占用。该guard不是记录签名、授权或远端任务认证。

见[Run调用者](FleetMissionExecutionObservation.md)、[原历史报告](FleetProtectedRunReport.md)和[未执行工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前：同一私有保护记录的轻目录与单条报告来源

新增private/friend-Swarm的 `protectedExecutionReportCatalog()`、`captureProtectedExecutionRecord(request)`，不注册QML调用、不调用实际owner/current刷新，也不复用queue／retirement／assignment准入作历史读取门。每个Orchestrator构造独立sourceId，一次Swarm关联丢失不重绑。

目录exact6、每行13及原记录16详见[完整合同](FleetProtectedRunReport.md)。目录不复制executionObservation正文，selectionToken只哈希前12个元数据字段。按sourceId／taskId／规范observationRevision／token查原private TaskRecord；closed／ownerDestroyed／exhausted仍可历史读取，不从同runId新对象或所选Series补字段。

max时原强制闭合／销毁会在同修订下改变closed/death/exhausted等metadata；token不能仅用revision。Controller在报告构建和原生JSON之后重抓完整record并比原实例，报告仍标revisionUnambiguous=false。所有通用mutation保护、占用、真实queue事实及原通知节奏不变，没有新发包、自动完成或记录清理。

[36项交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 411–415 current: local recovery UI is not onboard reassignment

The [recovery panel contract](FleetOccupancyRecoveryPanel.md) now has source wiring through the actual Guided controller at the command-center and FlyView regional-planner entries. It only reviews local restart/history holds; the current real uncertain set, task map, owner/Run and existing final old∪new native occupancy CAS remain unchanged.

A returned local release is historical, not a remote lease, free vehicle or formally attributed onboard result. The original worker must commit the new actual identity before the existing getter/CAS can admit work. Full execution/self-healing and the original 25 goals/six categories remain incomplete. No compilation, test, helper/authentication/app/UI/SQL/simulation/device execution, PX4 edit, flashing, PR update or packaging occurred. Older no-UI statements below describe their historical batches.

## 406–410 原生恢复当前边界

[恢复复核合同](FleetOccupancyRecovery.md)：恢复先核实际busy任务map；运行/暂停或原生观察占用不由它释放。最终六参CAS仍核old∪new当前actual身份和真实commit，不把历史解除收据或文件恢复当控权。本批无新操作UI、远端空闲证明或完整自愈交付；仅源码和限定静态读回，未编译、测试或运行。

## 401–405 current: native restart-review history at the same admission

The bound actual Swarm now automatically owns a [native occupancy journal](FleetOccupancyJournal.md). The existing final admission still calls its public nine-key getter, which additionally requires the original live worker, accepted bootstrap, ready/atomic-replacement readback, and equality of actual/offered/committed full identity. Old file identity is not a current commit; a stale/missing/failed source cannot fall back to an empty Swarm or a legacy API.

Every new Swarm installs a separate restart-review hold for all 1–255 protocol addresses before any public projection. Disk observations only add real uncertainty; they never restore task ownership, Session, approval or control. Empty/missing/open history or a successful write cannot clear the holds. No dedicated recovery-review/resolve workflow exists yet, so generic continuity remains pending review rather than becoming automatically schedulable. This is a temporary source-integration state, not completion of self-healing or of the original 25 goals/six categories. Safety commands do not wait for journal IO or gain a new storage veto. No build, test, helper/business/FS/authentication/application/device execution was performed.

## 396–400 current: actual native Swarm occupancy at the CAS boundary

The product path is now `reassignTaskRolesWithOccupancyAtRevision(taskId, targetVehicleIds, expectedRevision, leaderVehicleId, roleAssignments, expectedOccupancyIdentity)`. It retains the complete atomic membership/leader/role contract described below, but also requires the exact original native Swarm source and its reviewed occupancy identity. A caller-supplied free list, task-summary reservation table or pure helper result cannot grant this admission.

The actual Guided controller owns its private Swarm and Orchestrator instances. Swarm's existing one-use `executionTaskOrchestrator` association installs the Orchestrator's private `_bindContinuitySwarm` reverse QPointer before public notification, without a callback between the two halves. Only the actual same-thread Swarm may install that source; a second source cannot replace it, and destruction does not clear the sticky assigned marker. The final admission verifies the reciprocal native association. This is a source reference boundary, not hardware/account authentication or a claim that arbitrary plugins cannot instantiate their own isolated objects.

After all local CAS, member, leader, role, metadata and task-model conflict checks pass, the common staging helper calls the pinned Swarm's private `_continuityAssignmentAllowed` immediately before committing. The helper checks the union of original and proposed members against the real current native occupancy, including revoked-but-retained deployment reservations and operational/uncertain blockers. It does not infer absence from `missionDeploymentOwns == false`, drop a still-busy member to bypass its state, exempt a supposed own formation by task-ID text, cancel anything or release an owner. Missing/lost/mismatched source or unknown occupancy refuses the change. There is no signal, credential prompt, virtual transport getter, caller callback or outbound command between this native read and the one local commit.

The reviewed occupancy identity has exactly four actual strings: `{swarmEpoch, occupancyRevision, fingerprint, scope}`. It comes from the actual Swarm's validated nine-key `continuityOccupancySnapshot`; [FleetNativeOccupancyEvidence](FleetNativeOccupancyEvidence.h) only checks value shape/hash consistency and extracts this identity, not provenance or freedom of a real aircraft. The native source independently compares it with its own current state at final admission. The snapshot is explicitly `local-swarm-occupancy-at-read-not-remote-lease`: it is neither a continuously authenticated history nor a flight permit, remote control transfer, safe-to-fly certificate or automatic lease acquisition.

Guided `refreshMissionContinuity` and manual `applyContinuitySuggestion` both freeze the original actual source references and pass `requireNativeOccupancyEvidence: true` with that source's snapshot. Apply requires the recomputed suggestion and plan to explicitly report `requiresNativeOccupancyReview === false`; its exact four-string `sourceNativeOccupancyIdentity` must equal both the original captured snapshot and the recheck result. It then submits that fixed identity through the six-argument API. The prior explicit confirmation, actual local role-policy recheck, exact task revision, complete one-to-one roles, complete replacement coverage, member count and resolved Leader gates remain. No new visual UI or automatic reallocation is introduced.

All three older reassignment entry points share the same final native-current occupancy check once this Orchestrator has ever been bound, even though they do not carry a reviewed identity. They cannot bypass a lost original source by reverting to standalone mode. Only a genuinely never-bound standalone instance retains its documented local-model compatibility behavior; the product apply does not call that path. Generic APIs still cannot mutate a sticky native execution-observation task, renew Startup authority, stop/complete a flight, release Swarm/Run occupancy or replace the original task/observation ownership.

The prior frozen commit-versus-notification distinction remains: later source/owner destruction cannot undo a completed local mutation or be reported as proof that nothing changed. This source-only increment does not complete the original 25 goals or six remaining work categories. No compiler, parser, tests, Qt/QML/helper/business/API/authentication/application/SQL/device/PX4 execution, PR update or packaging was performed. Engineers still need to verify source pairing, QML value round trips, occupancy changes between review and commit, revoked/stale/uncertain blockers, old-member removal attempts, owner deletion and non-bypass of legacy entry points. Earlier batch-specific sections below are historical and do not override the current product API/source requirements.

## Historical 391–395: atomic local membership, leader and role reassignment

`reassignTaskRolesAtRevision(taskId, targetVehicleIds, expectedRevision, leaderVehicleId, roleAssignments)` is a single native local-task compare-and-swap. It stages membership, both leader aliases and `metadata.roleAssignments` together, checks the actual current QObject thread, exact task revision, nonterminal/generic ownership, existing assignment conflicts, existing metadata resource bounds and a nonexhausted structural revision, then commits once before notification. It never calls a legacy membership update followed by a second metadata patch. A native execution-observation record remains forbidden at every generic reassignment entry; no Run, Swarm reservation, flight command, Session authorization or remote result is changed by this task-model mutation.

When a role mapping is present, it is the complete group: a list of exactly two-key rows `{vehicleId, role}` with unique actual numeric integral system IDs in 1–255, covering each target exactly once. Roles must already be one of `leader`, `scout`, `relay`, `responder`, `member`; text/boolean IDs, implicit role conversion, duplicate/missing/extra members and other row keys are rejected. Exactly one nonempty-group row must be `leader`, matching the resolved task leader. The existing explicit leader aliases must agree; where neither exists, the existing generic first-member leader is used, not a newly selected aircraft. An empty draft may retain an empty mapping, but cannot start without members. Accepted rows are stored in the actual target order with native integer IDs.

`createTask` validates an explicitly supplied `metadata.roleAssignments` by the same contract. Legacy tasks without a mapping retain their old behavior. Both legacy reassignment APIs also validate any existing mapping against the proposed group and leader: they cannot silently keep removed members, invent new roles, remove the map or change the leader while leaving its old role behind. Such a change requires the complete atomic API. Reassignment refuses an exhausted `qint64` revision rather than modifying a task under an unchanged CAS value; a no-op is not a new revision.

Guided's existing manual `applyContinuitySuggestion` path consumes only the currently recomputed native planner suggestion. It now requires `details.replacementCoverageComplete === true`, a complete numeric `proposedAssignments` group, an exact complete `proposedRoleAssignments` list, and an explicit matching `proposedLeaderVehicleId` for both `replace_leader` and `reallocate_task`. The proposed member count must match the current task's original count; malformed or partial compound proposals are refused. It rechecks the current structural revision and the same local role-policy identity/permission before calling the new native API. JavaScript accepts only positive exactly representable revisions through 2^53−1; the native CAS still checks the real stored revision. There is no new panel, automatic action, privileged raw task setter or onboard transport path.

The native state commit remains distinct from later signals/audit. New creation/reassignment publication uses frozen values and QPointer checks between synchronous notifications; an already committed mutation is not relabelled false because notification destroyed its owner. The manual path likewise keeps an actual returned commit true if its controller is destroyed, and checks its plain lifetime before subsequent controller access. This is bounded source handling, not proof of reentrant runtime behavior or a blanket hardening claim for every legacy generic method.

The simultaneous role-aware replacement planner and reserve-coverage matching are separately documented in [FleetMissionContinuityPlanner](FleetMissionContinuityPlanner.md) and [FleetRoleAllocator](FleetRoleAllocator.md). This local positive task-model path does not certify candidate hardware/performance, replace the future authorized capability-review UI, transfer a native Run, complete a remote mission or release occupied aircraft. The original 25 goals and remaining six categories are unchanged. No compilation, parser, tests, helper/business/API/Qt/QML/application/authentication/SQL/device/PX4 execution, PR update or packaging was performed; actual build, Qt container round trips, role-loss/compound-failure scenarios, revision races, owner destruction and hardware release remain engineer verification work. The batch-specific sections below are historical; their existing native outcome protections remain in force.

## Historical 376–380: observed outcomes in the same protected record

The actual `FleetMissionExecutionObservation` Run now supplies a bounded observed-outcome value model through its existing private `TaskModelEvidence`. No placeholder result service, second TaskRecord store, caller JSON setter, new task phase or result-permission interface is introduced. `executionObservation.observedOutcome` and the native summary's top-level `observedOutcome` are the same stored group value; each vehicle row carries its corresponding target value. Run `current()` and task-model evidence use the same private `outcomeSummary()` / `targetOutcome()` implementations.

The exact value contracts are documented in [FleetMissionExecutionObservation](FleetMissionExecutionObservation.md): target 26 keys, group 20 keys, `reportedMission` 11 keys, `sourceRetirement` 5 keys, and 11 fixed receipt roles whose present values each have exactly `{sourceEpoch, sourceSequence, receivedMonotonicMs}`. Missing references remain null. Depth and payload stay within the existing bounded JSON observation contract; no raw messages, coordinates, account credentials, full read receipts or per-poll host clock are added.

The two target axes are separate:

- `outcomeCandidate` records only no-terminal-candidate, terminal-item, indefinite-terminal-hold or landed-after-terminal **observations**, never a completed mission.
- `observationWindow` records waiting, provisional, committed/source-bound, source-ended/unknown or monitor-retired status. Ending a source or destroying/retiring its owner never erases an earlier candidate or its bounded receipt references.

Group `modeQueueCoverage` is `no-targets-prepared`, `none-committed`, `partial-committed` or `all-committed`. None-committed means no commit has been retained; it does **not** certify no send, no active write or rollback. The original fixed membership remains the denominator. Four mutually exclusive candidate counts partition that membership; there is no fleet-success aggregation, distance/time/area percentage or removal of uncommitted targets to manufacture success.

Every new receipt is written only in the actual post-commit branch and remains tied to the original source epoch, increasing ingress sequence and that lane's local monotonic receive clock. Provisional RX, including frames nested inside the write call, is not promoted when a later commit arrives. Repeated reached messages remain repeated observations, not new successful waypoints. A first Mission-mode receipt and later terminal-current can express local reception order, not command causality or a remote mission-instance identity.

Valid negative MISSION_CURRENT reports are retained before correlation ends: no-mission sentinel/state, a known total mismatch or an out-of-range sequence remain diagnostic evidence with a specific reason. Zero/UNKNOWN extensions do not become known by default. An actual reported COMPLETE state is explicitly `completeStateReportedNotVerified`; neither it nor a cache match/landing candidate can set `missionCompleted` or `resultVerified`.

Original Vehicle retirement codes use a bounded whitelist with explicit native-known origin; unrecognized codes keep an unknown-origin fixed fallback, not arbitrary text. A late same-epoch retirement invokes the existing passive task-model update after checking the original self/target identity, even if monitor retirement stopped the timer. This preserves the real tail code in this record without synchronous outward notification, replacement-source lookup, transport, automatic closure or release.

Owner destruction still closes only monitoring. The mandatory close handler preserves all candidates and receipt references, marks each target's source/continuity unknown and its window retired, and marks the existing group value retired/unknown-tail. It does not convert the sticky native record to a generic task or emit taskCompleted/taskFailed/taskCancelled. Generic mutation/remove protections, observation revision exhaustion handling, 128 native records and the existing total capacity remain unchanged. Observation-only changes continue through the independent queued channel, not legacy per-frame taskUpdated/black-box logging; no new timer or log writer is added.

All `resultVerified`, `missionCompleted`, `executionVerified` and `remoteMissionInstanceVerified` claims remain false. `automaticOutcomeActionsIssued=false` means the outcome model issues no actions; it does not negate retained MODE queue facts. No candidate triggers generic completion, STOP/LAND/DISARM, direct/forced Run/Swarm release, renewed authority or safe-reassignment certification.

The concurrent native PRIMARY SYS_STATUS / ESTIMATOR_STATUS freshness chain is independent health evidence, not a result certificate. Complete per-packet EST stage-gap continuity, the final StartPermit, authenticated remote mission-instance/result evidence and authorized long-term retention cleanup remain unfinished. The local firmware's sequence-only progress messages cannot be upgraded to its unsent `instance_count/finished/failure` fields or a Run token by ground-station logic alone.

376–380's cumulative increment covers 7 source paths and 19 documentation paths; this is a path scope, not completion of the original 25 goals or the remaining six work categories. The preceding 371–375 deployment/history-selection integration remains source-only. No compile, moc, tests, parser, helper/business/API/authentication/application/SQL/device/simulation/flashing execution, PR update or packaging was performed. Earlier batch sections below are historical, including their then-current no-QML/no-cross-episode-UI statements; they do not override this section.

## Historical 340–344：共享原任务模型，保留旧部署占用

[Deployment Lifecycle 原生核心](FleetMissionDeploymentLifecycle.md)每代新建14个实际 owned 对象（含 signed-out Session），但 Swarm/Orchestrator/Role/Store/Health 共享五个冻结 actual typed 来源，Swarm 的实际 backend 必须是同一 Orchestrator。旧实际 Run、受保护任务、未知状态与原 map 占用不删除/reset；新任务仍受原成员冲突与容量门禁。

全部 retained 组的 native 固定18键静默证据不把 unknown 当无任务/空占用，实际预约的 revoked 不等于 absent。8次对象组分配含初始/失败尝试，不驱逐历史、不归还旧任务容量，也不重建 Orchestrator 绕过原 native/总任务上限。monitor 关闭不是飞行完成/停止证明。

**尚未注册 QML、替换 Guided 或提供新部署入口，应用仍单链。** native sticky outbound helper 已定义但 owned 请求/继续点、QML epoch/source callbacks、shared Role 明确身份上下文及 selected Run/Series/Close/Session 一组投影仍待接线；不能称跨代隔离已完成。原25目标/六类和长期结果/容量清理缺口保留；限定检查结果已记录，未编译、测试、执行 API/helper/认证/应用/SQL/设备、更新 PR 或打包。

## 335–339: UI access does not change native task authority

The shared [FleetMissionObservationPanel](FleetMissionObservationPanel.md) now has fixed entries in CommandCenter's left column and below the Planner header, outside the draft scroll. Entry placement does not depend on draft/preview, upload permission, selected members or aircraft-online state; existing parent-surface visibility rules remain. Guided exposes only the actual Series `previousStartup` Run, with no initial-owner fallback when that source is lost. No panel can create a native task marker, restore a Run or invoke generic mutation on its sticky protected record.

Each panel owns its controller reference, lifetime epoch and exact review token (`reviewId`, workflow epoch, generation and complete context). It cannot adopt or cancel the other entry's review; dismissal, hiding, destruction or controller replacement attempts cancellation only of its own still-current token. Cancelling this page's review does not retire monitoring, stop flight or roll back a committed native effect. All three confirmations start unchecked and clear on controller/token changes or loss of the panel's own-review match; focus defaults to explicit close/cancel, there is no default accept action, and Enter/Return cannot activate the dangerous approval button.

The native two-account workflow, original 30-second budget, complete context18/binding7-or-9, actual owner-map evidence, quiet transport and one-use consumption remain authoritative. UI counters/labels neither authenticate nor renew them. Display keeps known release, known retained occupancy and unknown/null separate; `ObservationClosed` remains neither `Completed` nor STOP/LAND/DISARM. This source-only UI integration supersedes the historical 330–334 no-UI status below. No compile, tests, scripts, application/rendering or authentication/business/helper/device execution occurred; remote result attribution and long-term lifecycle remain unfinished.

## 330–334: independently reviewed local-monitor retirement

The separate [FleetMissionObservationClose](FleetMissionObservationClose.md) backend connects an explicit local-monitor closure workflow. It requires the current real primary account, an exact `task.observation.close` policy receipt with second approval still required, and a different qualified approver SID authenticated for the dedicated `task.observation.close.approve` purpose. Upload/start/ground proofs are not interchangeable. A fixed 30-second monotonic review budget includes every check and the credential modal, and final closure requires three separate confirmations: end monitoring, release only this scope's local reservations, and acknowledge that aircraft may keep flying. The 330–334 batch had no operation UI; the later 335–339 source integration above adds those entries, not verified execution.

The actual Transaction comes only from the actual Series' `previousStartup()`; no initial-owner fallback, ID lookup or imported summary can select it. The workflow privately freezes the actual Session, RolePolicy, Series, Transaction, authorization owner, Run, Swarm and Orchestrator QPointers. Its exact 18-key native context retains the complete seven/nine-key binding, original ordered members and all scope fields. `localReservationState.run` and `.startup` each have exactly `{owner, occupiedVehicleIds, authorityRevoked}`. These are actual owner-map occupancy snapshots: every entry for each reviewed UUID is scanned, occupied IDs must stay within the original group, and the two sets cannot overlap. Revoked authority is not absent occupancy. Both snapshots participate in the frozen review/hash; neither effective `missionDeploymentOwns` failure nor a caller bool proves release.

After the one-use native account consumption, only a final passive actual-owner/context fence precedes the original Transaction's monitoring-retirement/drain path. Post-disposition again reads actual map occupancy for both exact UUIDs. Missing/malformed data or destroyed owners leave release **unknown**, never a fabricated empty map or successful release. `ObservationClosed` remains distinct from `Completed`, STOP, LAND and DISARM; neither observation closure nor verified local map removal authenticates a remote result, stopped aircraft, transferred remote control or safe reassignment.

`executionObservationRetirementMatches` is a separate private owner/immutable-identity/member matcher, not an alias for MODE queue admission. It still requires the actual living owner and matching native record, but does not demand a fresh source, clear unrelated assignment conflicts or an unexhausted observation counter. Queue admission retains its stricter checks. At `observationRevision` maximum, mandatory native retirement can close the record with the existing maximum and sticky exhaustion flag; it never wraps, reopens an owner, creates another unique observation revision or manufactures a mission result. Long-term task/deployment/retention lifecycle and authenticated remote completion remain unfinished.

No compile, tests, authentication/business API/helper execution or device interaction was performed. The dedicated backend is connected; the later UI source integration is described above, while engineer verification remains separate work.

## Scope

Generic local tasks keep their existing explicit lifecycle, assignment and progress APIs. This module does not send Vehicle/MAVLink commands. A separate **private native bridge** registers a local observation record for an actual `FleetMissionExecutionObservation` Run before its MODE write. That record is not the original `binding.sourceTaskId` intent, a remote mission-instance identity, an execution permit, a flight-state certificate or a completed mission.

`registerExecutionObservation`, queue-admission `executionObservationMatches`, retirement-only `executionObservationRetirementMatches` and `updateExecutionObservation` are private native methods, accessible to actual native owners through friendship, not QML invokables. They require the actual current QObject thread and consume `TaskModelEvidence` directly from that Run's native getter; caller-provided QML maps cannot register or update these records. Retirement matching does not grant another queue admission or certify its diagnostic projection.

## Immutable identity and limits

The identity is exactly `runId`, `startupId`, `transactionEpoch`, `transactionGeneration`, `deploymentId`, `binding`. UUIDs are canonical lowercase nonzero UUID strings; transaction generation is a canonical positive uint64 decimal string. Binding follows the existing Deployment/Series contract: the complete seven keys `sourceTaskId`, `recordId`, `recordRevision`, `planId`, `contentHash`, `reviewEvidenceHash`, `storeEpoch`, or all nine including the paired nonnegative `inputGeneration` and `reviewGeneration`. No fields are silently discarded. Targets are 1–32 unique actual Qt Int MAVLink system IDs in 1–255, kept in their frozen order.

Registration creates a new Orchestrator UUID, freezes this identity and assignment, rejects existing assignment conflicts, and cannot create another record for the same actual Run/run ID. A repeat before commit may return its existing matching record. The native-record cap is 128 within the existing total 10,000-task cap; closed/unknown history is not evicted or automatically removed. A full native collection explicitly prevents another registration before MODE. Authorized retention/archive cleanup remains future work.

The native observation projection is bounded to depth 10, 32,768 nodes and a compact JSON payload below 512 KiB (1 KiB reserved for owner-destruction tail markers). Only JSON value types are accepted, not QObject/custom types, raw bytes or implicit date/hash conversion. The actual Run supplies a fixed projection, without raw MAVLink payloads, coordinates or per-poll clock fields such as `hostQuietMs`. This module validates bounds and provenance of the native caller; it does not make that projection a new authority source.

## Lifecycle and mutation protection

- Registration with no actual MODE queue commit creates `Draft`.
- The first actual commit changes phase to `Observing`. Partial or uncertain flight effects remain observations, not a generic `Running`/success claim.
- Actual observer retirement or owner destruction changes phase to `ObservationClosed`. This means only that local monitoring has closed, not that a vehicle stopped, landed or completed its mission.
- Late commit facts can supplement an already closed record without reopening its phase or restoring busy state.

The native marker is sticky for the record's entire lifetime, even after the owner is destroyed. Generic start/pause/resume/cancel/complete/fail/progress, every reassignment entry point, and remove all reject it. A matching metadata field or task type cannot create this marker. Native summary progress and leader are unknown, `progressKnown`, `resultVerified`, `missionCompleted`, `executionVerified`, `executable` and `flightCommandReleased` remain false. The generic `terminal` field retains its original completed/cancelled/failed meaning; observation closure is separately explicit and never promoted to a flight result.

Every unclosed native assignment blocks another task's active assignment, including the prepared Draft phase. Its own generic assignment availability is always denied. Legacy `busyVehicleIds`, however, includes it only when the actual Run evidence reports busy (startup retired, a MODE commit exists, observer not retired). This intentionally avoids the prepared task rejecting its own MODE full gate. Local assignment protection is not a remote lease or flight permit.

On QObject owner destruction the identity and last observations survive, owner/current-source claims are cleared (including every vehicle row's `sourceCurrent` and the aggregate source count), and an explicit unknown/owner-destroyed tail is retained. Existing explicit-retirement facts are preserved, never inferred from closure or replaced with false. No completion/failure/cancellation/started signal is synthesized.

An unused MODE preparation can also close only through the actual Run/Startup private path after terminal sender/fence/protocol drain and explicit native evidence that no member MODE was issued or uncertain. `unissuedModePreparationClosed` distinguishes this cleanup from an explicit retirement request or owner destruction; it does not undo ARM/sequence effects or assert that an aircraft stopped. Unknown or committed MODE contexts are never auto-closed as unused. This permits a valid later ground reassessment without leaving the old protected Draft as an artificial assignment deadlock.

## Revisions and notifications

The existing structural task revision changes only for the first commit or observation closure, not each raw counter/source observation. Busy lifecycle changes notify legacy readers without changing this control revision. A separate nonwrapping uint64 decimal `observationRevision` and `observationUpdatedAt` track native observation changes. Normal updates refuse exhausted observation revisions, mark the exhaustion sticky and notify only the independent observation channel without inventing another revision. Queue matching also rejects the maximum counter. Mandatory actual-owner retirement and owner-destruction closure may retain that maximum value with `observationRevisionExhausted=true`, never wrap or claim a new unique revision; the retirement matcher does not reuse the queue-counter gate.

All native notifications are deferred through one zero-delay, QObject-bound coalescer. Pending sets contain only the bounded native records. Registration and actual phase/busy changes may send legacy `taskCreated`/`taskUpdated`, collection and fleet summary signals. Observation-only changes use `taskObservationChanged` and `executionObservationTasksChanged` exclusively, avoiding the existing controller's full legacy task-update black-box path. The read-only `executionObservationTaskSummaries` property filters the **same** TaskRecord collection; it is not a second independent task model. New native observation signals do not write logs or automatically create audit events.

The notification drain holds copied summaries, never a TaskRecord reference across public signals, and checks a QPointer to itself after every emit. Registration, queue-stack updates and polling therefore perform no synchronous outward public notification. Generic task signal behavior is unchanged.

## Verification boundary

This change is source-only. No compile, tests, helper execution, business API, device interaction, flight simulation, flashing or release approval was performed. Engineers must separately verify Qt build integration, native Run lifecycle/partial writes and reentrant notification behavior. Nothing here proves mission completion, remote mission-instance authenticity, stable flight or safe hardware operation.

# FleetProtectedRunReport：本进程受保护Run缓存报告

## 541 当前：报告 v2 保留模式退出证据

新 protected report8、record16、observation20/21 均 schema2；逐机 row25 不变，其 observedOutcome 从 schema1/exact26 升为 schema2/exact27，新增闭9 `missionModeExitObservation`。组 outcome20 仍 schema1。旧 report1/record1/observation1/outcome1 保原完整读取，版本混搭拒绝；旧资料不补成“退出0次”。

完整 UInt32 customMode 不恢复成有符号 Int；共享[退出校验器](FleetMissionModeExitObservation.md)核原源、计数、首末与接收顺序。原保护目录、原记录尾核、文件导入隔离、2MiB报告预算与非认证边界不变。退出不是停止、失败、完成或释放资格。见[报告实现合同](FleetMissionExecutionReport.md)；本次未构建、运行或测试。下方旧尺寸说明为历史。

## 494 当前合同：独立文件历史，不恢复受保护记录

489原生目录／选择／完整record尾核／构建链保持，新两文件API仅存取同一report8值，旧full9／handoff不接纳此格式。文件打开不是 `captureProtectedExecutionRecord` 或当前catalog来源；不回填TaskRecord、Run、权限、批准或占用。

保存链：本页原生 `jsonText`＋预期 `reportId/contentHash`＋显式路径／确认→Root私有busy围栏→`saveProtectedRunToFile`→完整codec→QSaveFile。报告原ID、capturedAtUtc、record及hash不重新生成；compact格式可规范化，内容hash不因此成为原字节签名。加载链：显式文件与确认→`loadProtectedRunFromFile`→完整codec→exact4原生map／jsonText→本页 `origin=imported` 的独立snapshot；没有全局新结果槽。

codec不直接放宽nativeInt：仅已知schema小整数槽将JSON实际Int／LongLong／Double有限整值恢复Int，null只在原未知槽保留并要求键存在；字符串修订／UInt64序列和binding宽域不转换。恢复后完整复用record16、compact20／21、成员25、单机结果26／整组20、reportedMission11、引用11／ref3及retirement5校验，重建summary13；根exact8、UTC、claims22及canonical正文hash完整核对。没有以局部摘要替完整记录验证。

输入／输出2MiB，既有depth16／nodes32768／map键256预算、UTF8回环、JSON object和全量成功／失败边界不变。保存前核同ID/hash、受限绝对路径且禁direct fallback；加载核尺寸、完整读／尾部／error／close。失败不返回成功前缀，写入结果未知不自动重试。没有文件系统身份、跨文件原子／durable／签名或当前遥测真实性证明。

本区保存／打开均先选文件、显示待确认材料，再明确操作；选择／snapshot／路径／同意变化后旧按压不能复用。导入与captured来源分离，原目录变化不能把文件升级为原生新鲜证据；外部历史不要求原native目录在线。隐藏或换controller清本页，不删除文件或改变真实任务。旧完整报告UI／文件接口保留独立。

[36工程场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。此批只补报告文件能力，签名、跨站／控制租约、全历史索引、真实任务关联／结果与完整执行许可仍未完成。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方489为历史。

## 489 当前合同：与完整Run及遥测档案独立

此链报告的是同一Orchestrator保留的本地TaskRecord，不是原Run完整current()、磁盘档案、可信远端任务实例或正式执行结果。来源已经关闭、owner已销毁或观察修订耗尽时，真实保留历史仍可被明确选取；缺少的原始字段不补造。操作成功不认证停飞、落地、遥测新鲜度、权限或飞行安全。

### 真实调用链

[Orchestrator](SwarmMissionOrchestrator.md)私有轻目录／单条捕获 → 一次关联的SwarmController只读接口 → [ExecutionReport](FleetMissionExecutionReport.md)非invokable static构建器 → 原生JSON及完整记录尾核 → Guided窄转发 → [原报告面板](FleetMissionExecutionReportPanel.md)中的独立折叠区。

新链不通过原Episode选择、previousStartup、旧完整Run build或文件导入；不把caller JSON、目录显示字段、同runId新对象或账号批准当原生记录。没有新QObject／代码文件、资源登记、依赖、发包或控制入口。旧full report9、source3、handoff17及文件接口保持原类型，不直接接纳新报告。

### 目录与选择

SwarmController公开只读 `protectedExecutionReportCatalog`，新NOTIFY只来自初次一次关联、原保护观察集合通知和实际backend销毁；丢失后不重绑、不定时刷新owner、不读盘。Orchestrator构造一个独立sourceId UUID，仅表示本进程对象实例，不是签名或真机身份。

目录exact6：

`schemaVersion=1, catalogSource="merivus-protected-run-records", sourceId, records, readOnly=true, historicalOnly=true`。

records按原任务顺序，仅原native保护记录，最多128条；不复制完整executionObservation正文。每行exact13：

`taskId, runId, startupId, deploymentId, observationRevision, observationRevisionExhausted, targetVehicleIds, modeQueueCommittedObserved, observationClosed, observationOwnerDestroyed, observationUpdatedAt, observationClosedAt, selectionToken`。

observationRevision始终为正UInt64规范十进制字符串，不经QML Number转换，也不用普通taskRevision代替。成员为原有序1..32个真实Int系统ID。更新时间是本地记录UTC时间，不是飞机采样、新鲜度或跨站时钟证明。

selectionToken为该行**去掉selectionToken后的12字段**Qt compact JSON SHA256。它不是全观察hash或授权token；用途只是在同实际sourceId/taskId下避免目录旧选择被误用。正常观察变化增加revision；max修订时原mandatory close／ownerDestroyed仍可改历史，其closed／death／exhausted等元数据使token改变，不能只比revision。

捕获请求exact4：`{sourceId,taskId,observationRevision,selectionToken}`，四项都为实际非null QString。外部调用还需明确confirmed；它只是本次只读操作确认，不是账号审批。非法、来源不配或过期选择返回失败，不自动选第一条、不fallback或重试。

### 原生单条材料

Orchestrator私有 `captureProtectedExecutionRecord` 从原TaskRecord读一份，返回record exact16：

`schemaVersion=1, source="merivus-protected-run-record", sourceId, taskId, observationRevision, observationRevisionExhausted, selectionToken, identity, targetVehicleIds, observation, modeQueueCommittedObserved, observationBusy, observationClosed, observationOwnerDestroyed, observationUpdatedAt, observationClosedAt`。

identity恰原6：runId/startupId/transactionEpoch/transactionGeneration/deploymentId/binding；binding保全部原7字段或成对inputGeneration/reviewGeneration的9字段。观察为原compact20；ownerDestroyed后原生只增加ownerDestroyed=true成为21，不增加完整current字段。原成员25、单机结果26／整组20、reportedMission11、receiptReferences11／单ref3及sourceRetirement5全部保留。

读取不调用owner/current／taskModelEvidence补源，也不用queue admission／retirement／assignment门限制历史；不更新观察、关闭状态、占用、权限或记录版本。sourceCurrent等都是捕获的缓存字段，不在读报告时变成当前新鲜证明。ownerDestroyed取存储标志，不把短暂QPointer为空改写成真实销毁结论。

Controller先冻结请求四字符串，在同实际Orchestrator及sourceId上取record；static构建后重抓**完整record逐值相等**，原生JSON生成后再核一遍。不是仅比较hash、revision或身份。无法保持同源即失败，不发布旧报告冒充本次选择。

### 独立构建器与报告

`static FleetMissionExecutionReport::buildProtectedRunObservation(nativeRecord)` 非QML invokable，返回exact3 `{ok,error,report}`；失败report为空。它是值校验／构建器，不把任意caller map认证成原生来源。

入口先限完整JSON树和2MiB材料，再递归独立持有QString／map／list。原始typed-null QString及invalid未知序号保持原Qt类型；不JSON roundtrip归一化原record，避免与Controller源尾核不等。

完整消费校验包括实际Int／Bool／字符串修订、原身份／有序成员、20／21及25／26／20闭集合、11个同epoch引用和已知接收先后、reportedMission真实负报告与原seen分离、候选／汇总计数及原总8192项限制。未began空source/operationId与未知seq合法；provisional不能偷带正式引用。报告不补造commit baseline、periodic、raw、完整来源或物理结果。

report exact8：

`schemaVersion=1, reportType="fleet-protected-run-observation", reportId, capturedAtUtc, record, summary, claims, contentHash`。

reportId为本次UUID，capturedAtUtc为本地UTC捕获时间；record保完整16字段。summary exact13：

`targetVehicleCount, modeQueueBeganVehicleCount, modeCommittedVehicleCount, uncommittedVehicleCount, sourceUnknownVehicleCount, noTerminalCandidateVehicleCount, terminalCandidateVehicleCount, indefiniteHoldCandidateVehicleCount, landedCandidateVehicleCount, observerRetired, startupTerminalState, startupTerminalReason, revisionUnambiguous`。

revisionUnambiguous只在未exhausted且revision不是UInt64最大值时true；不代表当前内容／来源真实或执行身份。sourceUnknown可与候选重叠，四候选计数按原组相加，不是成功率。

claims复用原22字段：historicalOnly／snapshotOnly／reportOnly／observationOnly=true；currentOnboardContentsProven／remoteMissionInstanceVerified／resultVerified／missionCompleted／executionVerified／executionAttributionVerified／requestCausalityVerified／authenticated／signatureVerified／permissionsRestored／startupPermitGranted／executionPermit／executable／flightCommandReleased／fullTelemetryCaptured／tailObservationComplete／sourceProvenanceVerified／currentValuesFreshnessVerified=false。

contentHash对去contentHash的完整report正文作Qt compact JSON SHA256，完整输出仍受2MiB限制。不签名、不认证、无全局_lastReport或文件读写。旧build、save／load／export、handoff类型校验未放宽。

### 对外返回与实际UI

Controller `captureProtectedExecutionReport(request,confirmed)` 对外exact4 `{ok,error,report,jsonText}`；失败report={}且jsonText=""。成功jsonText是同一report的原生Qt compact JSON，≤2MiB；序列化后再做完整原记录尾核。Qt未知值在JSON中明确为null，不靠QML stringify可能丢键的map生成复制文本。

Root只转四项选择、不存全局新报告；返回后另核实际Swarm和同轻目录选择。Panel持本页controller/lifetime/historyEpoch与按压时选择，明确确认后才调用。返回时解析原生jsonText，核report8／record16／summary13／claims22和原选择、reportId／hash，单次快照同时保存parsed报告及原文本。前端显示检查不替代原生完整校验或认证。

选择、无票据清理和捕获都持本区ticket跨通知；选择要求只推进自己的一个historyEpoch，清理逐通知核同票据／代次，最终消息在票据内，释放后不写。失效可借现action票据但不擅自释放；busy=false通知期间仍持capture票据，防重入覆盖。

目录没有默认首项，不自动生成／认领别面板报告。选中记录推进撤销旧选择和确认，同源已捕获报告仍明确冻结历史，不悄悄更新。换controller、来源消失、隐藏或收起清本区快照与意图；不关闭原Run，不删记录或文件。JSON需显式展开，可人工选择复制；不自动保存、写剪贴板、上传或恢复权限。

UI/UX指导只用于明确来源／未知／失效、短标签、PlainText和长ID换行、主题及键盘焦点、最小44px控制高度。界面未运行，大小窗／字体／对比度／读屏和滚动仍待验证。

### 工程状态与仍未完成

[36项工程场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。真实构建／运行／源生命周期／JSON与hash实现、复杂场景内存和耗时均未验证；新类型的文件存储／导入、数字签名、跨站同步、正式任务实例／结果和完整执行许可仍未实现，不将这些缺口改称仅需工程师开关。

仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。

# FleetMissionExecutionReport — Run 观察复盘文件（429）

## 541：任务模式退出历史证据的 v2 完整消费

新实际 Run 的 observation 根为 schema2，每机 targetOutcome 为 exact27/schema2，新增 `missionModeExitObservation`；整组 terminal outcome 仍是原 exact20/schema1，四种终端候选、计数与全部否定权限声明不变。full9、protected8、protected record16 与 handoff17 沿对应原观察的版本生成 v1 或 v2，不把旧资料补升级。校验明确核外层／观察／单机版本一致；v1 不得夹带新 sidecar，v2 必须完整携带，不接受混搭或成功前缀。

sidecar exact9 与 exit6/ref3 直接复用 [FleetMissionModeExitObservation](FleetMissionModeExitObservation.md) 的纯值 `valid`。full／protected 路径同时绑定原成员 sourceEpoch 与规范完整 UInt64 `missionModeDepartures`；未提交 MODE 不得声称已观察退出。首／末次 raw customMode、baseMode、systemStatus、此前任务模式接收与退出接收、当时连续性未知均保留。原 customMode 为完整 UInt32，JSON Double 整值由共享校验器接纳；protected JSON codec 不把它截成 Int32，不把 UInt64 接收序号／单调时间字符串转 Number。原小整数恢复和其余 protected 字段仍按旧规范核验。

full／brief summary 顶层仍 exact13，每机由 v1 exact14 变为 v2 exact15，新增同名 sidecar；protected summary 仍原 exact13，证据保在 record 完整观察中。v2 可见摘要及 full 报告保留的 observation 副本中，sidecar 缺省 first/last 只在 hash 前规范化为明确 JSON null，避免 Root 投影的 QML stringify 丢键；不改原 native Run／TaskRecord，不改数值类型，不补零或伪造退出。旧 v1 摘要保持无此键，界面明确“旧版未采集”，不能解释为 count=0。brief v1 保深5／1024节点，v2 为深8／4096节点；两版仍 96KiB 全量字节上限，不裁剪 32 机前缀。父交接 metadata 的有界包络适配由原 Manager 负责，导入仍是无权限历史。

full／protected 文件读取、共享 protected codec 和部署 bundle 复用同一版本校验，保原报告 ID／时间／hash，不复活任何 Run。SwarmController 真实 protected capture 的隐藏 schema 门同步允许匹配原 record 的版本1或2，仍完整同源 record 尾核。新增证据证明的是本地接收模式变化，不证明中止请求因果、退出原因、远端任务实例、正式完成／失败或实际停飞；本模块不发送任何控制命令。

同一 v2 summary row15 中，尚未观察的 currentSequence/reachedSequence 也仅规范为明确 JSON null，确保 Root stringify 后保留闭键并让 begin／commit 尚无进度的报告可见；原 observation 未观察序号仍保持缺键，v1 不改。不是补成序号0、收到回包或完成进度。

本批仅 apply_patch 与直接静读；没有执行解析、hash、文件 IO、业务、QML、编译、测试或仿真。下方均为历史合同，其 v1 描述不用于限制本节明确的 v2。

## 524：已闭部署归档复用原pure protected8 codec

新增非invokable纯值static `protectedRunObservationValid(Map)`、`decodeProtectedRunObservation(QByteArray)`、`decodeProtectedRunObservationValue(Map)`。直接复用原private protected8完整校验／已知整数和null恢复／summary重算／正文hash与2MiB预算；Value入口先做原depth16／nodes32768和纯值预算再serialize，拒未知类型／额外字段。解码两入口均exact4 `{ok,error,report,jsonText}`，失败空report／文本；不读时钟、文件、native owner或权限，不产生新reportId。

旧protected file两API继续同一接受域、路径和回执，不接纳新 [DeploymentArchive document15](FleetDeploymentArchiveWorker.md) 或full9。新worker逐原closed record16调用既有buildProtectedRunObservation，保其真实候选／引用和claims；重开bundle时逐份复用上述codec，不能从自洽值证明原生来源或实际关闭。文件归档／回读成功与最后Lifecycle对象退休是分开的事实，不让此pure codec释放任务、占用、账号或飞行许可。

本轮仅源码及合同直接静读，未执行codec／hash／parser／IO、编译或测试。下方为历史合同。

## 494 当前：独立保护报告文件codec与两API

新增 `saveProtectedRunToFile(path,jsonText,expectedReportId,expectedContentHash)` 返回exact2 `{ok,error}`；`loadProtectedRunFromFile(path)` 返回exact4 `{ok,error,report,jsonText}`，失败report={}／jsonText=""。两者要求对象所属线程，不发送信号或缓存全局报告；不调用build，不生成新ID／时间，原buildProtectedRunObservation仍严格native Int。

文件为原 `fleet-protected-run-observation` exact8，不加wrapper或偷带当前来源；旧full9 `saveToFile/loadFromFile/exportJson` 及handoff接受域不变。save消费同份原生文本并完整解析后核预期ID／hash才开写，避免QML QVariant的undefined经stringify丢键。load返回完整已校验map及原生compact JSON，非当前来源认证。

codec先限2MiB输入及UTF8整回环、JSON object；树深16／节点32768／map键256／标量与输出2MiB沿既有预算。仅root／record／compact／group／summary／vehicle／row outcome／reportedMission的已知小整数槽恢复Int；接受实际Int／LongLong／Double的有限整值且核原范围，拒布尔、字符串及不允许的null。允许未知序号的null仍须键存在；UInt64修订／序列字符串不转数，binding原宽域由旧validator核，不窄化。

新完整report8 validator核UTC、ID、record16全部原嵌套字段与接收先后、selectionToken、重建summary13、claims22固定4 true／18 false以及去contentHash后的canonical正文hash。允许合法JSON空白，规范输出不要求原字节相同；hash不是签名或来源证明。绝对路径受限且不创建父目录，QSaveFile禁direct fallback；读完整长度、尾部、size/error和close失败不给报告。没有fsync／durability或文件身份认证。

[完整合同](FleetProtectedRunReport.md)、[36项未执行矩阵](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 489 当前：独立受保护缓存报告，不伪装full current

新增非invokable `static buildProtectedRunObservation(nativeRecord)`，内部返回exact3 `{ok,error,report}`。它只消费独立record16，完整核compact观察20／ownerDestroyed21、成员25、单机结果26／整组20、reportedMission11、引用11／ref3和retirement5；保真实空QString／invalid未知序号及负面任务报告，不用当前Run字段补缺。

新report exact8／type=`fleet-protected-run-observation`，摘要13含revisionUnambiguous；原reportClaims22（4 true／18 false）、正文内容hash及2MiB预算保留。独立拥有递归字符串，原record不经JSON roundtrip改类型；原生Controller另生成规范jsonText供人工复制并完整尾核。

[完整新合同](FleetProtectedRunReport.md)区别旧report9／source3和handoff17。旧build／saveToFile／loadFromFile／exportJson／handoff全部保持原类型校验，不直接接纳新格式；本新入口不自动读写文件或更新报告缓存，不提供来源认证或任何控制。

[36项交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行；仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 436：实际 Run 交接摘要

新增 `buildHandoffBrief(observation, source)`，返回 exact3 `{ok, error, brief}`。它复用原 `build` 和报告校验，在本次调用内临时构建完整报告，再只提取身份与摘要；不保存、导出或缓存完整报告。成功 `error` 是明确空字符串，失败 `brief={}`，不截断或返回有效前缀。原报告允许已准备但任务模型注册失败的空 task ID；交接摘要不允许，必须使用原观察中非空 UUID `executionObservationTaskId`，不能用规划会话 `binding.sourceTaskId` 或“最新任务”替代。已有注册 task 的全部未 MODE 提交、部分提交、来源未知或退休 Run 仍可以生成历史简报。

`public static bool handoffBriefValid(const QVariantMap& brief)` 是独立纯值校验，供交接记录创建及导入复用。它不访问 QObject 状态、文件、飞机、操作者或权限服务。`buildHandoffBrief` 仍经原 `build` 要求对象所属线程；新增方法不改变原四个报告/文件 API。

brief 根 exact17：

| 字段 | 合同 |
| --- | --- |
| `schemaVersion` / `briefType` | 数值 1 / `fleet-run-handoff` |
| `briefId` | 本次新建的小写非零 UUID |
| `capturedAtUtc` | 同本次临时报告的 UTC ISODateWithMs |
| `source` | 原 exact3：`lifecycleEpoch`、`selectionGeneration`、`episodeId` |
| `runId` / `taskId` / `startupId` / `deploymentId` / `transactionEpoch` | 原 Run 对应的小写非零 UUID；taskId 来自真实注册观察任务字段 |
| `transactionGeneration` | 原 Run 的规范正十进制字符串 |
| `binding` | 完整原 Run 7/9 键绑定，不换成当前最新计划 |
| `summary` | 原 exact13，`vehicles` 为 1–32 行原 exact14 摘要 |
| `reportId` / `reportContentHash` | 本次临时完整报告的 UUID / 小写 SHA-256；不代表完整报告另有文件已保存 |
| `claims` | 下述固定 exact8 声明 |
| `contentHash` | 去除此键后整个 brief 的 Qt 紧凑 JSON SHA-256 |

binding 的七个基础键为 `sourceTaskId`、`recordId`、`recordRevision`、`planId`、`contentHash`、`reviewEvidenceHash`、`storeEpoch`；九键版本增加 `inputGeneration`、`reviewGeneration`。ID、整数与摘要哈希的限制沿用原报告；sourceTaskId/recordId 不擅自升级为 UUID。数值校验接受真实整值 JSON 数字，拒绝字符串、布尔及 null 冒充整数。

claims exact8：`snapshotOnly`、`historicalOnly`、`receiptOnly` 固定 true；`controlOwnershipTransferred`、`approvalTransferred`、`missionCompleted`、`flightCommandReleased`、`executable` 固定 false。接受简报只表示本地信息回执，不能推导控制权、起飞许可、原批准转移或任务完成。

brief 的紧凑 JSON 和累计键/字符串 UTF-8 字节各不超过 96 KiB，值树最多 1024 节点、深度 5（brief 根为 0）。这为交接 `metadata={kind:'run-brief',brief}` 的深度 8、2048 节点、128 KiB 标量预算留出包络空间，不需要放宽旧预算。原始 observation/source 输入仍沿用 2 MiB 报告预算；简报不携带完整 observation、任务载荷或任意附加树。

发布前通过 `QJsonDocument::fromVariant(...).toVariant().toMap()` 统一显式 JSON null，避免 Qt 5 的 invalid QVariant 在 QML 变成 undefined 并丢键。摘要序号两键必须存在：观察计数为 0 时只接受 JSON `QMetaType::Nullptr`；非零时接受 0–2047 的真实整值，绝不以 null 当序号 0。校验器还核对唯一车辆、提交必须先排队、退休不得声称来源当前、未提交不得有进度、候选/窗口闭集及全组九个计数与逐行关系。由于摘要没有原 itemCount，只能检查观察数量/序号可证明的全组最少项数不超过 8192，不能反推原载荷长度。

摘要哈希是内部一致性检查，不是签名或来源认证。`reportContentHash` 在生成时对应本次完整报告；只持有 compact brief 的校验器无法重新计算被省略报告的哈希，也不证明完整报告可找回。真实选定 Run、操作者、目标接收者及前后来源复核由调用方负责；任意调用者能构造同样的合法值，不能因此获得本地可信来源或导入后的权限。

新增失败类别：`execution_handoff_report_invalid`、`execution_handoff_registered_task_required`、`execution_handoff_brief_budget_exceeded`、`execution_handoff_brief_invalid`；原 build 失败沿用其 `execution_report_*` 原因。当前仅源码与静态读回；原生/QML/JSON null 往返、32 行边界、元数据嵌套预算、任务注册缺失、退休及部分提交、错误类型/计数/哈希，仍需工程师构建后验证，本轮未执行。

## 实现范围与边界

本模块把已经准备的 `FleetMissionExecutionObservation.current()` 值快照生成可保存、重新打开的复盘报告。它不读取 Vehicle，不发送任务、解锁、模式或飞行命令，不修改 Run、任务档案、审批或控制权。报告可以在 Run 仍在观察、部分 MODE 入队、全部未入队、观察退休时生成；“历史快照”指捕获时刻的值，不表示任务已经结束。

Root 负责从实际选中的 Episode/Run 所有者取值，并在调用前后复核来源。这个 QObject 只接受值，不能证明任意调用者传入的数据来自真实飞机。导入文件不查询当前飞机，不绑定为活动 Run，不恢复权限或批准。

允许逐机遥测来源未知不等于允许丢失本地整个组／Run所有者后从缓存取源；Root仍要求实际选定组及previousStartup身份一致。报告的`flightCommandReleased=false`描述报告动作本身，不证明关联启动事务此前没有发过ARM、序号或模式命令。

已实现源码；本批只做静态读回。尚未编译、运行、仿真、文件读写测试或硬件验证，不构成可部署验收。

## API 与文件行为

所有 QObject 调用入口要求对象所属线程；纯静态 `handoffBriefValid` 不访问对象。模块无持久状态、属性或信号，不缓存旧报告。

- `build(observation, source)`：返回 exact3 `{ok, error, report}`。成功保留原始有界观察树并派生摘要；失败 `report={}`。未准备的初始观察实例即使已有随机 Run ID 也拒绝。真正已准备、但任务模型注册失败导致 task ID 为空的未入队 Run 可以报告，保留失败原因。
- `saveToFile(path, report)`：返回 exact2 `{ok, error}`。先完整验证报告，再使用 `QSaveFile`，关闭 direct-write fallback，检查完整写入及 `commit()`。不创建目录。允许替换用户明确选择的既有文件；文件选择后的第二次保存确认由 Root/UI 提供，不是本模块的权限证明。
- `loadFromFile(path)`：返回同 build 的 exact3。读取大小、UTF-8、JSON 对象、版本、身份、边界声明、派生摘要和摘要哈希全部验证后才返回原报告；失败不返回部分报告。加载不分配新 report ID、不刷新捕获时间，也不恢复活动状态。
- `exportJson(report)`：验证后返回紧凑 UTF-8 JSON 文本；失败返回空字符串。导出不写文件。

文件 API 只接收绝对本地文件系统路径，拒绝空路径、首尾空白、NUL、控制字符、无效 Unicode、URL 形式及超过 4096 字符的路径。不接受 `file://` URL；调用方须先转换为 local file path。读取要求普通文件。这里没有自动建目录、路径搜索、历史扫描或自动保存。

成功 `error` 为明确空字符串。稳定失败类别包括：`execution_report_wrong_thread`、`execution_report_input_budget_or_type_invalid`、`execution_report_observation_or_source_invalid`、`execution_report_output_budget_exceeded`、`execution_report_output_invalid`、`execution_report_invalid`、`execution_report_path_invalid`、`execution_report_open_failed`、`execution_report_write_failed`、`execution_report_commit_failed`、`execution_report_file_budget_exceeded`、`execution_report_read_failed`、`execution_report_utf8_invalid`、`execution_report_json_invalid`。

## 文件格式与完整性

报告根 exact9：

| 字段 | 约束与含义 |
| --- | --- |
| `schemaVersion` | 数值整型 1；读取兼容 JSON 数字类型，拒绝布尔、字符串和 null |
| `reportType` | `fleet-run-observation` |
| `reportId` | 新的小写非零 UUID |
| `capturedAtUtc` | 生成报告时的 UTC ISODateWithMs，不是飞机时间或任务完成时间 |
| `source` | exact3：`lifecycleEpoch`、`selectionGeneration`、`episodeId`；前后两项小写非零 UUID，generation 为规范正十进制字符串 |
| `observation` | 原实际 Run 的有界 JSON 值快照；额外有界观测字段保留，不复制整套上游字段白名单 |
| `summary` | 下述 exact13 及最多 32 个 exact14 逐机摘要 |
| `claims` | 下述固定 exact22 边界声明 |
| `contentHash` | 去除此字段后的整个根对象，以 Qt 紧凑 JSON 编码计算的小写 SHA-256 |

摘要在保存和加载时均由 `observation` 重新推导并做 JSON 值比较；不接受与原观察不一致的手写摘要。版本、根字段、声明和哈希不合格时拒绝，未知或旧版本不自动迁移。哈希只检查文件内部一致性，**不是签名、来源认证、防篡改证明或远端任务实例证明**；能够改文件的人也能够重算哈希。

输入、报告和读取文件均限 2 MiB。通用值树限深度 16、32768 个节点、每对象 256 个键、键 128 字符、单字符串 65536 字符，并限制累计字符串字节及最终 JSON 字节。数值必须有限且绝对值不超过 JavaScript 精确整数边界；计数字段另要求整值。只接受 JSON 类型，兼容原生 `QVariant()` 与 JSON `QMetaType::Nullptr` 空值；拒绝任意 QObject 指针、日期、二进制及自定义类型。缺失观察保留 null，绝不当作序号 0 或成功。

## 实际 Run 正向约束

要求 `prepared=true`，真实 Run/startup/transaction epoch/deployment 的 UUID、正 transaction generation、原任务 binding，以及 1–32 个按原顺序唯一的目标。binding 保留原 7/9 字段约定：record revision 为正数；可选 input/review generation 为非负整值；档案 ID 不强制为 UUID。每机任务 1–2048 项，全组最多 8192 项。

报告验证原观察的只读、未认证、未完成等核心布尔边界，及摘要依赖的排队、来源、序号、结果候选和计数相容性。没有把每个上游遥测子树重新实现为独立协议。未入队的 source epoch/operation/hash 允许原生空 QString；JSON 重新打开后的空字符串同样兼容。已退休的来源可以不可用，原历史候选仍保留，不要求 sourceCurrent=true。

## 摘要与解释

summary exact13：`targetVehicleCount`、`modeQueueBeganVehicleCount`、`modeCommittedVehicleCount`、`uncommittedVehicleCount`、`sourceUnknownVehicleCount`、`noTerminalCandidateVehicleCount`、`terminalCandidateVehicleCount`、`indefiniteHoldCandidateVehicleCount`、`landedCandidateVehicleCount`、`observerRetired`、`startupTerminalState`、`startupTerminalReason`、`vehicles`。

逐机 exact14：`vehicleId`、`componentId`、`phase`、`reason`、`modeQueueBegan`、`modeQueueCommitted`、`sourceCurrent`、`continuityUnknown`、`currentSequence`、`reachedSequence`、`observedCurrentItemCount`、`observedReachedItemCount`、`outcomeCandidate`、`observationWindow`。

目标数为 1–32；汇总计数为 0–目标数。四类候选计数之和等于目标数，包含未入队目标，不是成功率。`sourceUnknownVehicleCount` 按 `!sourceCurrent || continuityUnknown` 汇总，可以与候选重叠。observed item count 是观察到的不同序号数量，不是已完成航点、距离、面积、时间或任务进度百分比。序号未观察时为 null，观察到时为 0–itemCount-1；观察计数为 0–itemCount。

`outcomeCandidate` 直接取原生 `targetOutcome`，不能仅根据 phase 推导完成：

- `no-terminal-candidate-observed`：未观察到终端候选。
- `terminal-item-observed-not-completion`：观察到末项，不证明完成。
- `indefinite-terminal-hold-observed`：观察到无限期末项保持，不证明完成。
- `landed-after-terminal-observed-not-completion`：末项后观察到落地，不证明远端任务完成。

`observationWindow` 为原生优先顺序：退休 `observation-retired`；未开始排队 `awaiting-native-mode-queue`；来源不明 `source-ended-or-unknown`；尚未提交 `provisional-native-queue-window`；否则 `committed-native-source-bound`。phase/reason 仍保留原 Run 文本，不将 partial/none committed 概括为整组成功或失败。遥测 `hostQuietMs` 是捕获时主机观察间隔，不是飞行时长，也不用于制造新的证据版本。

## 固定声明

claims exact22 中仅以下四项为 true：`historicalOnly`、`snapshotOnly`、`reportOnly`、`observationOnly`。

以下十八项固定 false：`currentOnboardContentsProven`、`remoteMissionInstanceVerified`、`resultVerified`、`missionCompleted`、`executionVerified`、`executionAttributionVerified`、`requestCausalityVerified`、`authenticated`、`signatureVerified`、`permissionsRestored`、`startupPermitGranted`、`executionPermit`、`executable`、`flightCommandReleased`、`fullTelemetryCaptured`、`tailObservationComplete`、`sourceProvenanceVerified`、`currentValuesFreshnessVerified`。

这些声明不会因为摘要出现 landed、全部 MODE 已提交、哈希一致或文件重新打开而改变。

## 工程师后续验证（未执行）

需要实际构建后覆盖：原生/JSON null 往返；完整未入队、部分入队、全部入队、任务模型注册失败及退休快照；未知来源和历史候选保留；32 目标与预算边界；坏版本、坏哈希、错误摘要和非法类型拒绝；UTF-8/截断/超限文件；选择路径后取消、再次确认、覆盖既有文件、权限或磁盘失败时的 QSaveFile 行为；Root 切换 Episode/Run 时的实际来源归属。以上均不能由本次静态审查代替。

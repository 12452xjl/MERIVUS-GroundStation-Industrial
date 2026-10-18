# FleetOperationalMemory 运营记忆地图与指标契约

## 523：严格单页 GNSS 报告的跨任务人工留存和独立文件工作区

本类在原 loose observations 之外新增独立 private GNSS reports 集合；不把严格报告包装成 `recordObservation`、vehicle snapshot 或 legacy hotspot，不从当前 Registry 拼坐标。旧 `observations/mapPoints/metrics/maxObservations/clear/importJson/exportJson/saveToFile/loadFromFile` 的来源和 schema1 原样保留，既不读写也不清除新集合及其去重依据。这里的“跨任务”是本地历史材料留存，不认证任务归属、飞行完成或物理飞机。

### 实际追加与容量

`appendGnssHistory(actualArchive,reviewToken,true)` 只在 owning GUI、非退场／非当前mutation／非工作区入场通知中消费实际 Archive 的 native-only `nativeGnssReview(token,true)`；原 token/current/action-busy 和完整33值校验均通过，再以同一次原生读取核对后才提交。caller report、导入Map或坐标不能进入这个口。它没有任何飞行／参数／文件发送动作。

回执 exact7：`appendAccepted,reason,memoryInstanceEpoch,memoryRevision,reportIndex,reportCount,sampleCount`。新对象 epoch 为 UUID，revision 从正 uint64 `"1"` 起；每次实际成功追加增1且不回绕，耗尽后拒绝新写。成功 index 为原集合0..31，失败−1；非owner拒绝返回空epoch／revision `"0"`／0计数，不跨线程读集合。完整提交和固定回执先于通知，通知内删除 Memory 或 Archive 不回撤已接纳事实。

最多32完整报告／2048 GPS样本，满额整份拒、不trim、不替换、不默默淘汰。每份必须通过 [reportValid](FleetTelemetryGnssReview.md)，整个集合还须通过同一 `reportsValid`：同 `runId/headerHash/afterSequence/nextSequence` 页键拒重复（含0 GPS页），同 `runId/headerHash/sequence` 样本键拒重叠；不按增长时会变化的fileHash、query、catalog或token规避。页内原 epoch／sysid／component及原sourceSequence完整保留，不把数字ID当认证同机。除对象寿命结束外本批不新增active集合清除／导入；清文件槽、旧legacy clear及旧import都不删除上述依据。

### 异步原生文件与分页

`requestGnssWorkspace(intent,true)` exact8：`action,expectedJobId,snapshotId,contentHash,filePath,reportIndex,offset,overwriteConfirmed`。字符串均真实非null QString，index是实际整值−1..31、offset0..32；Bool不接受字符串。动作：

- `freeze`：仅实际私有typed集合COW冻结，产生新snapshotId，保原epoch／revision／capturedAtUtcMs；空集合也合法。没有任意Map freeze输入。
- `open`：手选绝对本地JSON，用独立file槽；失败保旧file槽，不导回active集合。
- `save`：只原session冻结snapshotId＋完整contentHash，另明确overwriteConfirmed；不会重新export live集合。
- `reports`：原snapshotId，offset作为afterIndex，每页最多8条小metadata，不公开全部报告。
- `report`：原snapshotId＋准确reportIndex，返回该一份原完整33（最多64样本）供显式审阅／另次选图。
- `cancel`：只当前expectedJobId合作取消；重复取消幂等，不提前释放线程槽。
- `clear-file/clear-frozen`：仅精确snapshotId清对应私有槽，不清active集合，也不按共享latest清别人。

除对应动作使用的字段外必须中性：非save hash空／overwritefalse，非open/save路径空，非report index−1，非reports offset0，freeze/open/cancel snapshotId空。save只能session槽，clear动作须符合对应槽。单槽busy、入场busy、原jobId、非回绕serial／nonce／真实worker identity共同约束，不在busy时创建第二job。

文件工作由一次性 [FleetGnssMemoryWorkspaceWorker](FleetGnssMemoryWorkspaceWorker.md) 执行，只有值型request、私有cache和原生typed报告进入线程，没有live Memory／Archive／权限对象。其 `resultReady(jobId,result5)` 与实际 `QThread::finished` 两端均到达且原对象／job／nonce／serial／worker仍一致后才接纳；取消的读取不安装cache或报告。aboutToQuit／析构先关闭新业务并退休selected，再只请求cancel，不wait／terminate或删除运行线程，独立finished→deleteLater仍在。保存的实际commit事实不会因迟到cancel改成未写；进程关闭不是可认证的durable保证。

worker cache exact3 `{descriptor,bytes,reports}` 只在原Memory成员保留，不Q_PROPERTY公开。文件独立document type／schema1，旧LegacyMemory格式的未知根字段规则仍会拒新文件；新文件32报告／2048样本／4MiB整份校验、无成功前缀。路径／QSaveFile／实际bytes hash／commit与readback分离沿该worker合同；本地文件和hash不认证作者、设备或权限。

descriptor exact17：`schemaVersion,snapshotId,origin,sourceInstanceEpoch,sourceRevision,capturedAtUtcMs,reportCount,sampleCount,locatedSampleCount,contentHash,sourceFileHash,sourceCurrent,historicalOnly,authenticated,permissionsRestored,executionPermit,flightCommandReleased`。origin为session或file；session是原冻结epoch／正revision，sourceFileHash空；file原来源epoch空、revision `"0"`、sourceFileHash等原文件实际bytes SHA。contentHash总是该快照实际bytes SHA。worker sourceCurrent恒false，由原Memory只按实际session epoch／revision且未耗尽投影当前提示；file恒false。自然新增使旧冻结提示false，不篡改原历史数据，也不剥夺它的只读审阅资格。所有授权旗false，historicalOnly true。

`gnssWorkspaceState` exact17：`schemaVersion,jobId,action,stage,busy,cancelRequested,frozen,file,selectedReportToken,memoryInstanceEpoch,memoryRevision,reportCount,sampleCount,historicalOnly,permissionsRestored,executionPermit,flightCommandReleased`。

`gnssWorkspaceResult` exact10：`jobId,action,ok,error,reply,historicalOnly,authenticated,permissionsRestored,executionPermit,flightCommandReleased`。请求回执 exact11沿既有workspace形状：`schemaVersion,action,accepted,jobId,reason,requestOnly,historicalOnly,authenticated,permissionsRestored,executionPermit,flightCommandReleased`；accepted仅表示接纳异步请求，不等于文件或报告成功。freeze/open reply为descriptor单键；reports reply6为snapshotId／afterIndex／nextIndex／totalCount／hasMore／rows，每row11保原sourceIdentity7和实际计数；report reply3为snapshotId／reportIndex／report。save reply11分别保fileWriteAttempted、fileWrittenKnown、fileWritten、readbackVerified，commitfalse时写入可能未知；committrue后written事实不因回读失败抹除，durable／fsyncVerified始终false。

### 明确选择，不恢复当前源

仅原 `report` 成功双终结后，Memory从实际cache相同snapshot／index／完整report安装 `selectedGnssReport`33、新的 `selectedGnssReportToken` UUID和 `selectedGnssReportBinding` exact5 `{snapshotId,contentHash,sourceFileHash,origin,reportIndex}`。getter检查实际cache、完整原报告、无异步busy和非退出；自身同步入场／发布fence不当异步busy，以免通知内嵌套事件循环完成的真实结果被自隐藏。不因后续active追加或sourceCurrent提示false使同一历史内容失效。新的有效业务请求先清选择／token，cancel、clear或退场也退休；失败不会自动恢复旧图。

Root仅从实际对象getter取得所选快照；公共小Map不能导回active或native authority。当前GNSS地图投影为exact5并明确origin `native-page/session-snapshot/file-history`；重开文件后仍需一次实际报告选择和一次地图同意，默认不显示，不自动居中／goto、不插值、不拼航迹、不授StartPermit。Archive／Memory请求桥均不预先清共享latest；请求被拒不获得清另一来源／别页后来图层的资格，只由本页准确lease及实际native token退休清自身选择。HDOP仍为同包显示启发而非米级精度；unknown、未定位、partialTail和hasMore不转成“正常”或完整历史。

本批仅源码和普通文本静态阅读／编辑；未编译、执行项目／parser／helper／hash／QML、测试、仿真或硬件。需工程师验证：空／32／33集合、重复空GPS页、跨fileHash重叠、真实same-source分组、JSON Double／全uint64、worker双终结次序、busy通知／删除、cancel前后commit事实、失败重开保旧槽、active新增时冻结不换值、选图隐藏／换源令牌退休及legacy clear/import绝不清GNSS去重。下方为legacy合同，不能将其宽松归一化或热点阈值套到本严格集合。

`FleetOperationalMemory` 是地面站侧的只读运营记忆层。它把已经解码的车辆快照、任务摘要和人工干预记录归一化为有限长度的观测历史，并从观测中聚合链路盲区、GNSS 不稳定、风场、高耗电航段、异常历史和运营指标。模块不访问 `Vehicle`、MAVLink 或 PX4，不修改任务，不改变车辆状态，也没有任何命令发送入口。

## 执行观察与完成率归因边界（2026-10-15）

`metrics()` 保留既有任务指标字段，但把顶层真实 `nativeExecutionObservation=true` 的最新任务摘要单独计数：`nativeExecutionObservationTaskCount` 为此类任务总数，`nativeExecutionObservationClosedCount` 为其中 phase=`observation-closed` 的数量。`taskCount` 仍是所有最新任务摘要数；上述观察任务全部排除在 `taskCompletedCount`、`taskFailedCount`、`taskCancelledCount`、`taskTerminalCount` 和 `taskCompletionRate` 的分子/分母之外。即使导入摘要同时写了 `nativeExecutionObservation=true` 和 `phase=completed`，也不会贡献完成数。关闭观察不是完成任务，正在观察也不代表飞机执行成功。该分组只接受顶层真实 bool，不接受 metadata 或字符串代替。

完成率只反映剩余普通任务的本地模型状态标签，不认证飞行结果。指标固定给出 `taskCompletionEvidenceScope=local-model-state-labels-not-authenticated-flight-results`、`taskCompletionAttributionVerified=false`；调用者或导入 payload 写入同名 true 标志不能改变这一输出。既有 `taskCompletionRateIsEstimate=false` 只表示对现有标签做确定性计数，不代表其来源或任务成功经过认证；没有普通终态分母时完成率仍为 `-1`。

导入历史依然可以影响本地标签统计，但不能重建实际 Run、原生观察槽、owner 绑定、控制权或飞行完成证据。模块没有把本地摘要绑定到连续原生执行证据/实际飞行结果的认证链；真实完成率、事故率和运营可靠性仍存在该统计缺口，不能把导出数据或界面比率用作放飞/验收证明。本批没有编译、测试、执行指标 API 或读取飞行日志，仅作源码和限定文本核对。

## 接口

模块注册为 `Merivus 1.0` 的 `FleetOperationalMemory`，主要属性和方法如下：

```qml
FleetOperationalMemory {
    id: memory
}

memory.recordVehicleSnapshot(snapshot)
memory.recordTaskSummary(summary)
memory.recordOperatorIntervention("manual_review", { taskId: taskId })
var handoffJson = memory.exportJson()
memory.importJson(handoffJson)
memory.saveToFile(filePath)
memory.loadFromFile(filePath)
```

`recordObservation()` 也接受通用 `{ kind, timestampMs, payload }` 记录。每条记录带 `observationOnly=true`、`commandPathConnected=false` 和 `flightCommandReleased=false`；`estimateFields` 列出这条观测中明确标记为估算的字段。车辆 ID 限制为 1–255，明显未来时间戳会被拒绝。乱序 `task_summary` 不会覆盖较新的任务快照。历史默认保留 5000 条，可通过 `maxObservations` 调整。

观测 payload 中的布尔证据采用严格类型：只有实际的 JSON/QVariant `bool` 才能触发 `online=false`、`gnssHealthy=false`、`energyHotspot`、`anomaly`、`hasFault`、`operatorIntervention` 或 `*IsEstimate=true`。字符串 `"false"`、`"true"` 和其他隐式值不会被当作证据，避免损坏交接数据被误计入运营热点或异常。

## 地图点与聚合指标

带有有效经纬度的记录可以产生以下地图点：

- `link_blind_spot`：链路质量低于 50%、丢包达到 10%、延迟达到 1000 ms、离线或断链状态；
- `gnss_instability`：无效/不稳状态、低于 3D fix、卫星数少于 6 或 HDOP 大于 2.5；
- `wind`：风速样本；
- `energy_hotspot`：显式高耗电、能耗/距离超过 300 Wh/km 或电量不高于 25%；
- `anomaly`：异常、故障、事故或高/严重等级记录。

`metrics` 提供任务完成率、覆盖率、响应时间、能耗、人工干预、利用率、维护成本和上述热点数量。任务完成率只对已记录的普通终态任务标签计算，原生执行观察另行计数且不进入分母；没有覆盖率、能耗、利用率或维护成本证据时返回 `-1`，并通过对应的 `<metric>IsEstimate` 与 `estimateFields` 明确表示未知/估算。由位置格网得到的 `coverageCellCount` 是观测范围，不是测绘面积或安全覆盖证明。

链路质量若来自 RSSI 或其他推导源，应在输入中带 `linkQualitySource` 或 `*IsEstimate=true`；模块会把 `linkQualityPercent`、RSSI、坐标或能耗等字段列入 `estimateFields`，不把估算值伪装成原始遥测。

## JSON 交接

`exportJson()` 返回 `schemaVersion=1`、模块名、观测数组和聚合指标。`importJson()` 只接受声明过的根字段，并要求完整对象、版本号、模块名、有效时间戳、非空类型和对象型 `payload`，先完成全部校验后再替换内存历史；失败时保留现有记录。未知根字段不会被静默丢弃，新增字段必须提升 schema 版本。`saveToFile()` 使用 `QSaveFile` 原子提交，`loadFromFile()` 拒绝空路径、读写失败和超过 32 MiB 的输入，并沿用导入校验语义。导出的指标是可重算的展示快照，不是签名证据链；本地文件接口也不提供加密、权限、清理周期或数据库保留策略。

## QML 接入与边界

`GuidedActionsController` 以较低频率采样现有 `FleetRegistry` 车辆快照和 `SwarmMissionOrchestrator` 任务摘要，并通过 `operationalMemory` 暴露共享实例。`FleetOperationalMemoryPanel` 只显示热点、指标和估算标记，不触发任务、交接、Vehicle 或飞行命令。真实链路盲区、风场、GNSS、能源模型、人工干预来源和维护成本仍需工程师接入并用日志回放、Mock/SITL 数据校准。

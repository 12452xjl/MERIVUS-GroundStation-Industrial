# GNSS 历史记忆异步文件工作区

## 523：独立文档、私有值缓存和一次性 Worker

[FleetGnssMemoryWorkspaceWorker.h/.cc](FleetGnssMemoryWorkspaceWorker.h) 复用维护工作区的一次性QThread／resultReady＋实际finished模式，但不复用维护记录schema或活动Passport。唯一typed owner为 [FleetOperationalMemory](FleetOperationalMemory.md) 的独立GNSS集合；legacy observations、KPI、同步保存接口和维护工作区不被文件导入覆盖。

构造为 `FleetGnssMemoryWorkspaceWorker(jobId, action, request, privateSnapshot={}, freezeReports={})`。仅值参数，不跨线程携带Memory、Archive、Vehicle或账号。原子取消是合作请求；内部Qt解析／hash单次有界操作不承诺即时打断。owner须按原jobId、nonce、非回绕serial与实际worker identity核对结果，并等resultReady和actual finished双终结才释放单槽；不得删除运行线程或在GUI等待。

Worker result exact5：`ok,error,action,cache,reply`。cache只在freeze/open成功时返回；是原生私有exact3 `{descriptor,bytes,reports}`，完整4 MiB字节及32报告不作为Q_PROPERTY／QML整表公开。其它成功动作cache={}，失败不清已有缓存，不返回成功样本前缀。

## 独立磁盘合同 exact11

```text
schemaVersion, documentType, sourceInstanceEpoch, sourceRevision,
capturedAtUtcMs, reports, historicalOnly, authenticated,
permissionsRestored, executionPermit, flightCommandReleased
```

schemaVersion=1，documentType=`merivus-fleet-gnss-memory`；sourceInstanceEpoch为原Memory规范非零UUID，sourceRevision为canonical正uint64十进制字符串（1..18446744073709551615，不转JS Number）。capturedAtUtcMs为1..9007199254740991；打开文件时只保留其声称的UTC，不认证来源或实际采集时间。historicalOnly=true，其余四个身份／权限／飞行旗标固定false。空reports合法，但revision仍正数。

reports是≤32个完整成功 [GNSS33](FleetTelemetryGnssReview.md)，总GNSS样本≤2048，实际UTF-8文件／冻结字节≤4 MiB。freeze先调用共享纯值`reportsValid`，不让QObject、typed-null／未知树或自定义转换进入JSON；open解析后完整调用相同校验。校验全部33／identity7／policy3／row18、整数／字符串域、同包位置和质量、未知哨兵、计数与序号。`runId/headerHash/afterSequence/nextSequence`页重复及`runId/headerHash/sequence`样本重复整份拒绝；不因fileHash增长、query/catalog/token不同逃过去重。空GPS页仍防重复页，不把重读算新证据。纯值一致不认证文件或真正native捕获。

bytes始终为实际冻结或读取字节；contentHash只计算这些字节的SHA-256，不用第二规范化hash冒充源文件hash。open不重写原bytes，也不恢复文件中的snapshotId／runtime token或控制权限。snapshotId由实际owner当次生成，不在磁盘根保存。

## descriptor exact17

```text
schemaVersion, snapshotId, origin, sourceInstanceEpoch, sourceRevision,
capturedAtUtcMs, reportCount, sampleCount, locatedSampleCount,
contentHash, sourceFileHash, sourceCurrent, historicalOnly,
authenticated, permissionsRestored, executionPermit, flightCommandReleased
```

origin=session|file，snapshotId为本次owner UUID；reportCount≤32、sampleCount≤2048、locatedSampleCount≤sampleCount。session保原epoch／正revision，sourceFileHash为空；file的descriptor epoch为空、revision="0"仅明确不认证当前来源，sourceFileHash等于实际bytes hash（磁盘根仍须正revision）。文件capturedAtUtcMs保其声称值。sourceCurrent在Worker始终false；仅owner可对session的原epoch/revision派生本地当前性，file永不变active。historicalOnly=true，其余四旗标false。

每个基于缓存的动作均在Worker从原bytes重新解析验证并重建cache对比，核原descriptor、报告和实际hash。此核验不是文件此刻未变或实物身份认证，不读活动来源补缺失。

## 请求与小页

| action | worker request精确键 | reply |
| --- | --- | --- |
| freeze | snapshotId,sourceInstanceEpoch,sourceRevision,capturedAtUtcMs | exact1 `{descriptor}` |
| open | path,snapshotId | exact1 `{descriptor}` |
| save | snapshotId,contentHash,path,overwriteConfirmed | 下述exact11 |
| reports | snapshotId,afterIndex | exact6摘要页 |
| report | snapshotId,reportIndex | exact3 `{snapshotId,reportIndex,report}` |

reports每页8，afterIndex范围0..实际reportCount，允许末页空。reply exact6为 `snapshotId,afterIndex,nextIndex,totalCount,hasMore,rows`；每row exact11为 `reportIndex,sourceIdentity,rowCount,gnssSampleCount,locatedSampleCount,unlocatedSampleCount,qualityCautionSampleCount,qualityUnknownSampleCount,withinThresholdSampleCount,groupCount,historicalOnly`，historicalOnly=true。不包含绝对坐标；reportIndex为本快照0基索引，不是来源认证。report动作仅准确索引，整份report33≤64样本才返回，不自动选择首份或显示地图；owner双终结接纳后另发原生token，UI仍须另次明确地图选择。

## 保存事实与共享IO

save仅允许origin=session的明确冻结缓存，核原snapshotId和actualbytes hash，路径及覆盖确认独立且必须true；file历史不能自动变成可保存的活动来源。使用 [FleetSnapshotFileIo](FleetSnapshotFileIo.md) 的4 MiB读写，沿原QSaveFile分块／取消／commit／回读边界，不新建目录。

save reply exact11：`snapshotId,contentHash,path,fileWriteAttempted,fileWrittenKnown,fileWritten,readbackVerified,readbackError,fsyncVerified,durable,authenticated`。后三旗标始终false。commit未知保fileWrittenKnown=false；commit成功保ok=true、fileWritten=true，即使之后取消或回读失败，error/readbackError非空也不抹实际写入事实。前置失败可返回空reply，不假称文件确定没写。不自动重试／回滚／删除文件。

## 未执行工程场景

本轮只做源码修改及直接静读／普通文本检查；未运行业务、parser、IO／hash／helper、编译、测试、仿真、硬件或刷机。工程师须覆盖：0／32／33报告、2048边界、4 MiB两侧、坏任一报告无成功前缀、重复页与跨页重复sample、uint64极值、文件伪造／未知旗标、两种双终结次序、取消及销毁、保存commit未知／成功后回读失败、跨snapshot旧回调、打开失败保旧槽、无自动live导入／任务恢复／地图／控制副作用。

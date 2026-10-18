# 已闭本地部署归档文件 Worker

## 524：归档材料与实际对象退休分离

`FleetDeploymentArchiveWorker` 是一次性 QThread，只接纯值，不携带活动 Lifecycle／Episode／事务／Run／账号／Vehicle。原生 Lifecycle 捕获实际完整材料、按jobId／nonce／serial／真实worker匹配 `resultReady` 与实际 `finished` 两端，再在文件已写且回读核对成功后复核原来源；**只有该owner可以最后退休原本地对象和受保护记录**。worker不删除对象、不清占用、不释放飞行许可、不把文件或hash当认证。

构造 `FleetDeploymentArchiveWorker(jobId,action,request,privateInput={},privateArchiveCache={})`。动作仅 `archive-retire/open/report`；`requestCancel()` 只置原子标志。结果exact5 `{ok,error,action,cache,reply}`，cache是native-private exact2 `{bytes,document}`，不向QML公开完整归档包。关闭／析构由owner请求cancel，不能wait、terminate或删除运行线程；取消不抹掉实际commit事实。

### 原生输入与独立文档

archive-retire request exact2 `{path,overwriteConfirmed:true}`；privateInput exact8：

`archiveId,lifecycleEpoch,episodeId,capturedAtUtcMs,published,retirementKind,quietEvidence,records`。

前三ID是规范非零UUID，UTC为1..9007199254740991的本地捕获值，published为真实Bool。retirementKind闭集 `closed-observations/unused-native/unpublished-pristine`：以quietEvidence中真实Run是否存在分支，不用report数猜无Run。有Run仅closed-observations；注册task失败但原生严格确认未发MODE并关闭的Run可以保在quiet而0份report。无Run为明确unused／unpublished分支；未published必须unpublished-pristine。0记录不表示任务完成、没有飞行或远端空闲。

quietEvidence保原Episode的exact18、512KiB上限：schemaVersion／episodeId／scope／deploymentId／deploymentGeneration／deploymentState／binding／sessionState／sessionRevision／componentStates／seriesRevision／transactions／reservations／runs／outboundQuiescent／flightStateVerified／missionResultVerified／flightCommandReleased。scope固定 `native-quiescent-outbound-not-flight-or-run-retirement`，outboundQuiescent=true，后三保证false。componentStates恰原14、事务≤9、占用行≤32、runs≤9；保原sourceRetained和localTransport未知null，不把缺对象补为idle。占用数组须空，retired Run是本地观察已退，不是飞机停止。

worker检查闭字段、有限JSON纯值、局部传输旗标与空占用的值一致性、事务／Run关联；不复制或冒充native对象检查、当前Link／Session／权限门。输入树先检查depth≤24／nodes≤524288／标量≤32MiB及UTF-8，再序列化；拒QObject、自定义值、typed-null数字／容器和超JSexact原生整数。counter保uint64字符串不转Double。quietEvidence另有512KiB检查。

records为0..9个原完整record16，逐项复用 [ExecutionReport](FleetMissionExecutionReport.md) 原 `buildProtectedRunObservation`，不是构造一个假Run。每个record必须closed=true、busy=false、ownerDestroyed=false；原完整成员／结果候选／引用／未知值和claims不改。每个有真实taskId的quiet Run与记录一一对应，原identity6、targets及modeQueueCommitted相同；task／run／报告ID不重复。未登记taskId的行只能是retired且未commit模式的原生未发MODE分支，真正允许条件仍由native owner证明。多事务不简化成只有最后一条。

磁盘document exact15：

`schemaVersion,documentType,archiveId,lifecycleEpoch,episodeId,capturedAtUtcMs,published,retirementKind,quietEvidence,reports,historicalOnly,authenticated,permissionsRestored,executionPermit,flightCommandReleased`。

schemaVersion=1，documentType=`merivus-fleet-deployment-archive`；reports为上述0..9份完整protected8，historicalOnly=true，四个认证／权限／飞行旗标false。独立文件不放宽旧report8／full9／handoff文件接受域。输入／文件／输出完整字节≤32MiB，超界整份拒绝，无成功前缀。每份protected报告仍沿原depth16／nodes32768／2MiB接受域。

### 保存事实、重开与浏览

保存只用这次完整冻结的实际材料，显式路径和overwrite=true；复用 [SnapshotFileIo](FleetSnapshotFileIo.md) 的本地绝对.json、末级符号链接拒绝、QSaveFile fallback=false、分块取消及完整commit／回读。源目录重解析点、恶意TOCTOU、fsync、断电持久化和文件真实性都没有因此认证。

archive-retire reply exact12：`archiveId,contentHash,path,fileWriteAttempted,fileWrittenKnown,fileWritten,readbackVerified,readbackError,fsyncVerified,durable,authenticated,summary`。最后三项false。commit失败可fileWrittenKnown=false，不能称确定未写；commit成功立即保ok=true／fileWritten=true，之后取消／回读失败只进error和readbackError。仅ok且readbackVerified=true才返回private cache供最后native退休复核；否则仍保完整保存事实，不自动重试或回滚。

contentHash是**实际整个文件bytes**的SHA-256，不是某一protected报告正文contentHash；后者保原report8内部一致性。cache.bytes保存原字节，cache.document是全量校验并恢复已知整数／null后的原文档值；不重新生成报告ID或捕获时间。

open request exact1 `{path}`。打开后完整校验独立document15及每report8，复用同pure codec，不恢复活跃Episode、task、Run、账号或权限，不改变任何active记录。失败无prefix并由owner保旧opened槽。open reply直接为summary exact9：`archiveId,episodeId,lifecycleEpoch,capturedAtUtcMs,published,retirementKind,reportCount,contentHash,historicalOnly:true`。文件UTC／quiet声明只是未认证原文值，不是本次打开时刻或物理状态。

report request exact2 `{archiveId,reportIndex}`，index为0..8且小于实际reportCount；仅owner的actual cache，按原bytes重新校验并与cache.document完全对应。reply exact3 `{archiveId,reportIndex,jsonText}`；jsonText为原codec完整校验的canonical report8文本≤2MiB，报告hash／ID／UTC不改。UI只逐2048个UTF-16单元显式浏览，不parse／stringify整包或重建来源。summary9没有报告目录或默认首项，0报告只显示明确空材料。

### 静态交接

本轮仅源码编辑、直接静读和普通文本检查；未执行IO／parser／helper／hash／业务／QML、编译、测试、仿真或设备。工程师须覆盖：0／1／9／10事务与记录、2MiB报告／32MiB包边界、任一坏记录整份拒、JSON整数／null／全uint64、闭观察／未commit例外、报告与quiet Run关联、部分占用、取消与commit前后、commit未知、成功后回读失败、两种双终结次序、源在最后复核变化不退休、退后文件独立重开与Unicode全文浏览，以及同ID／hash不能恢复实际对象或权限。

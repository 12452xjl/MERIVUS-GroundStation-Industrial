# FleetArchiveTaskIndex：固定派生任务历史索引

## 538：后台持久化、分窗续扫与真实记录下钻

本模块仅新增 [h](FleetArchiveTaskIndex.h)／[cc](FleetArchiveTaskIndex.cc)／本文档。`requestValid(request)` 和 `resultMatches(result,request,policy)` 是无I/O的值契约；`execute(directory,policy,request,cancelled,result,error)` 只准在已有 [HistoryReader](FleetEventArchiveHistoryReader.md) 后台线程执行。它没有QObject、Vehicle、任务控制、审批或回放来源，不自行创建线程、配置任意目录或恢复权限。

索引仅汇总原已保留诊断事件的完整 `record.event.payload.taskIdHash`。任务哈希是完整64字符小写十六进制值，不截短聚合、不从vehicle／run／plan／时间或任务名字补关联。缺字段及Codec v3 capture-gap合法的显式空hash都计未归属行，不计算hash(空串)。其它存在但坏型／坏hash的行整窗失败。`indexedEventCount` 为已消费总行数，恒等于所有任务 `eventCount` 之和加 `unassignedEventCount`；计数为无回绕的规范u64十进制字符串。行数不是任务完成、飞行次数、原始全集或无缺口证明。

### 固定请求与结果

请求恰8键：`kind='task-index'`、`action`、`scope`、`indexId`、`checkpointHash`、`afterTask`、`taskIdHash`、`cursor`。前7项均实际非null QString，cursor为实际非null Map。scope只收 `sealed`／`sealed-and-offline`。

- create／open：indexId／checkpointHash和所有业务游标中性为空；create拒绝已有固定索引文件，不覆盖它。
- resume：精确当前native indexId和checkpointHash，业务字段空；完整索引可无写入地返回，未完成者继续原私有游标。
- page：精确同一索引身份，afterTask为空或原页末完整hash；非空锚必须确在该checkpoint。每页最多64项，按完整hash字典序，不按墙钟或任务结果排序。
- records／continue-records：精确同一索引身份及当前索引内完整taskIdHash；首次cursor空，继续只用Store私有保留的原Reader cursor5，不能由UI重建。复用原任务过滤search，返回原记录与缺口摘要，不把索引聚合当事件原文。
- rebuild：只替换固定派生索引，可显式选新scope；允许旧id／hash同时为空（重启或旧来源失配后的恢复入口）或同时为精确旧值。两种情形都必须先实际读取并完整校验固定旧checkpoint，冻结其全字节／mtime基线，再扫真实新源；绝不因空输入盲覆盖。坏格式／未知schema仍拒绝，交工程师处理，不自动删除或修复原件。

成功结果恰16键：`format='merivus-fleet-archive-task-index'`、`schemaVersion=1`、action、scope、indexId、checkpointHash、checkpointCommitted、cancelObservedAfterCommit、checkpoint、page、records、historicalOnly=true、authenticated=false、permissionsRestored=false、executionPermit=false、flightCommandReleased=false。

checkpoint恰8键：`inventoryFingerprint`、`scanComplete`、`taskCount`、`indexedEventCount`、`unassignedEventCount`、`sourceFileCount`、`windowBatches`、`windowScannedRows`。sourceFileCount是**整份冻结源清单**的文件数，不是已扫描完成数。scanComplete仅说明本次冻结清单的保留行扫描到尾；部分checkpoint也可分页、下钻，不称历史完整。

仅page动作的page为恰5键 `{afterTask,nextAfterTask,hasMore,totalCount,rows}`；row恰2键 `{taskIdHash,eventCount}`，升序唯一、eventCount为正u64字符串。其余动作page={}。仅两记录动作records非空，严格为原Reader task-filtered schema2／exact29；其它动作records={}。checkpointHash是固定checkpoint紧凑JSON原字节的SHA-256，**不包含本次查询包装、page、records或cancel标志**。它不是签名、权限或可信来源认证。

### 扫描、完整源身份与容量

每个create／rebuild／未完成resume窗口最多调用128次原 `searchInWorker`；无筛选search每批至多64行，故窗口最多8192行。窗口结束提交可继续的新checkpoint，不存在终生8192行截断；后续显式resume从同一原cursor继续。最多32768个不同任务hash、索引文件最多16MiB，达到容量时整窗失败，不淘汰旧任务或发布成功前缀。UI仅收到checkpoint摘要、64项任务页或原有界记录页，不累计整份事件列表。

第一次窗口先保留全部实际已读文件身份；提交前通过 `snapshotIndexSourcesInWorker` 冻结同inventory的**全部**files6，核已读前缀逐项一致后才替换为完整清单。每个file6为fileIndex／basename／databaseEpoch／sealRunId／fileFingerprint／offlineCurrentOnly，索引0..N−1，封存最多128项，显式offline最多再加固定current库1项。新JSON不猜文件identity或目录来源。

open／resume／page／records／continue-records以及保存前均调用 `verifyIndexSourcesInWorker`：完整目录inventory相同，所有冻结文件的完整字节SHA及SQL epoch／seal等元数据重新核对；offline沿原owner协作锁和双SHA只读路径。部分索引的记录下钻也核全部源，不能仅记当前文件而遗忘前页已读文件。记录search自身仍执行原页／文件／游标与取消验证。任一缺失、变更、sidecar、坏SQL／元数据或取消整次拒绝；不删除原日志，不从旧结果恢复当前源有效性。rebuild不消费旧源有效性，只在真实新清单上重新建立派生状态。

### 文件、原子提交与取消事实

仅使用实际配置目录下固定 `fleet-task-index-v1.json` 和同名 `.lock`；不收caller文件名／path。目录须现存规范绝对本地目录，拒绝UNC／symlink／canonical不符；文件须普通非symlink、非空且≤16MiB。既有lock须普通非symlink、≤16384字节，`QLockFile` 使用stale=0和tryLock(0)，同一派生索引各请求全程协作互斥，不按年龄强抢锁。

固定checkpoint JSON恰12键：format／schemaVersion／indexId／scope／inventoryFingerprint／filesTotal／scanComplete／cursor／fileIdentities／indexedEventCount／unassignedEventCount／tasks。cursor沿Reader exact5；fileIdentities必须完整等于filesTotal，尚未扫描的文件也保真实身份。所有任务、计数、排序、总和、cursor及活动文件对应关系完整验证。JSON重开时，仅已核域内的结构整数Double规范成实际Int，再传严格Reader API；不把String／Bool变成index。

读取按64KiB有界循环、完整EOF／error／size／close与前后规范路径／mtime核；解析前词法深度≤12、容器／字符串token≤262144、单字符串原UTF-8表示≤8192字节。整份紧凑JSON重编码必须逐字节相同，拒绝重复键、坏UTF-8、额外键、非规范数值／顺序、尾部、BOM／空白和未知格式。它是本模块生成的严格派生格式，不是通用JSON导入器；预算不是Qt parser峰值内存或绝对CPU时间证明。

写入使用QSaveFile且禁direct-write fallback，64KiB分块完整写／flush。实际commit之前再核全部真实源及原索引文件存在性／全字节／mtime基线；create还须保持原不存在。所有最终结果形状／整体展示预算在commit前已验证。取消或失败在commit前不提交本窗口，不改旧checkpoint；临时文件由QSaveFile退场。原先成功窗口不回滚。

`checkpointCommitted=true` 仅在本次QSaveFile commit报告成功后成立。之后发现取消仍返回已提交事实并置 `cancelObservedAfterCommit=true`；Reader后续取消尾也只能保留此事实，不能改成“保存未发生”。未commit的open／page／records／no-op resume晚取消仍失败。commit／flush不是fsync、独立readback、断电耐久或磁盘备份保证。协作锁和路径核不防绕过协议的非合作进程、硬链接／junction／恶意置换或同元数据ABA；部署须限制目录访问。

重开验证文件结构与源文件身份，不重新从所有原始行重算聚合。因此checkpoint本身仍是可被本地编辑者改写的未认证派生诊断材料；hash只绑定本次内容，不能升级其任务归属、计数真实性、原日志可信性、全集、完成状态或任何执行许可。需要重新核算时显式rebuild。原日志retired／drop／discard／unclean缺口继续由真实记录search展示，索引没有补造丢失事件。

### 工程交接（全部未执行）

1. 首次create无文件、已有文件拒绝、空sealed清单及offline缺失拒绝。
2. 超8192行后checkpoint非完成，重复resume能到真实尾，无重计／跳行；空文件与零行批次仍前进。
3. 同hash跨库聚合、完整hash排序、缺hash／v3 gap空hash未归属、无跨vehicle／run推断。
4. 32768任务／16MiB边界、u64计数耗尽、坏最后行，整窗不提交或淘汰。
5. 64项首页／后页／最后页／空页、明确旧锚拒绝及原任务记录续页。
6. 部分checkpoint下钻后，前一批已越过文件发生同metadata字节变化，完整SHA重核仍拒绝。
7. 全源snapshot已读前缀变化、后续未扫描源变化、缺文件／sidecar／坏SQL／epoch／seal，拒绝不拼旧源。
8. 正常重启open的JSON整数恢复；重复键、坏尾、深度／token／字符串／字节预算和未知schema拒绝。
9. 锁占用／异常lock／symlink／路径绕过／读取中变化／保存前baseline变更拒绝。
10. rebuild空旧id恢复入口、显式旧id不符拒绝、新scope重扫、坏旧checkpoint不自动修复或删源。
11. 每个扫描／全源核／写入阶段取消，无本窗提交；实际commit后取消保committed事实且不自动重试。
12. result／finished双尾、原query／scope／index／checkpoint／任务身份及UI隐藏／换源处理，由原Store／Panel联审。

仅新增源码／合同并作直接静态阅读、普通文本核对；未执行JSON／SQL／hash／索引／helper／worker／文件I/O实现、编译、测试、预览、仿真、设备操作、原日志迁移／删除、提交或打包。原25项目标及总体未验证边界不由本模块完成。

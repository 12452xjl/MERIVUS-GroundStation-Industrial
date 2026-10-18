# 维护护照文件工作区（518）

## 523：仅提炼共享文件IO

原Worker的路径检查、32 MiB UTF-8分块读取及QSaveFile提交／回读移入 [FleetSnapshotFileIo](FleetSnapshotFileIo.md)。仍传原 `maintenance-workspace` 错误域，原request、result5、cache5、descriptor18、save11及commit事实边界不变；Passport单槽／双终结、预检、评估和UI未改。新的 [GNSS文件工作区](FleetGnssMemoryWorkspaceWorker.md) 使用独立schema与4 MiB，不把GNSS当维护记录或借用活动Passport。此提炼只经源码静读，未执行IO、编译或测试。

本模块为地面站中的人工维护审阅提供一次性原生后台任务：冻结当前进程中的维护记录、明确保存该固定快照、独立只读打开文件，以及有界浏览来源组、记录摘要和完整规范化详情。它不从文件恢复当前维护记录、任务、账户、审阅批准或任何飞行权限，也不把同一数值机号或文件 hash 当成物理设备身份认证。

## 1. 实际边界与线程

`FleetMaintenanceWorkspaceWorker.h/.cc` 是一次性 `QThread`，不是常驻服务或 QML 类型。构造参数为：

```cpp
FleetMaintenanceWorkspaceWorker(jobId, action, request,
                                privateSnapshot = {}, freezeRecords = {});
```

`jobId` 为非空规范 UUID；`action` 仅为 `freeze/open/save/groups/records/detail`。`requestCancel()` 只置原子取消位。线程终结发出 `resultReady(jobId, result)`；Passport 还必须等待同一实际线程的 `finished`，按原 jobId、nonce、代次、实际对象与取消状态接纳。结果到达与线程结束可能先后不同，单槽未双终结前不可重用。running worker 不得直接析构；其生存期和双终结由原 Passport 管理。

Worker 的 `run()` 不读取活动 Passport、Archive、Vehicle、Registry、账户或界面对象。本线程内创建一个无观察者的临时 Passport，复用现有 schema1、条件历史来源哨兵、记录规范化、历史区间去重与来源组评估规则。原活动实例不调用 `importJson/loadFromFile`，不被文件替换，也不因只读打开而发 `recordsChanged`。

冻结输入只能是 Passport 已分 turn 完整预检的实际记录值。允许 JSON 可保留的有限原生值，拒 QObject、自定义类型、日期/字节数组、typed-null 容器、无效 UTF-16、非有限数字，以及超出 ±9007199254740991 的原生 64 位整数；不能靠隐式转换静默丢失或取整。JSON null 保留为 null。文件解析得到的是该 JSON 文件自身的数值语义，不是原远端传感器精度认证。

纯值边界以 `records` List 为根，最大深度 24、节点数 262144、保守标量预算 32 MiB；完整文件/冻结 JSON 字节也不超过 32 MiB，记录数不超过 2000。超界整份拒绝，不截断成“完整”前缀。解析、规范化和评估在 worker 执行；原既有评估单次调用不被拆成可中断算法，取消在有界调用前后及显式循环检查，不承诺即时取消磁盘或 Qt 内部解析。

## 2. 私有缓存与不可变快照

结果严格为五键：

```text
ok, error, action, cache, reply
```

`ok` 为 Bool，`error/action` 为固定 String。`cache` 是原生私有值，不能作为 Q_PROPERTY、QML projection、JSON 复制或公共回执发布。仅 Passport 内部最多保留两个缓存槽：本次会话冻结快照与独立文件历史。完整记录、完整评估和文件字节不得整表交给界面。

缓存五键：

```text
descriptor, bytes, records, groups, detailCache
```

`bytes` 为实际固定 QByteArray，`records` 为完整规范化记录，`groups` 的每项为 `{row, assessment, recordIndices}`。`detailCache` 至多含一项 `{groupId, recordIndex, text, contentHash}`，不是累积所有巨型详情。Qt 值容器的隐式共享只用于原生线程间值交接，后续替换遵守值语义，不能跨线程携带活动 QObject。

`freeze/open` 成功返回新缓存；`detail` 成功仅更新同一快照的一条详情缓存。`groups/records/save` 不返回新缓存。失败保留原槽；打开失败不能抹掉已经打开的历史，保存失败不能重导出另一份活动记录。

### descriptor：严格 18 键

```text
schemaVersion, snapshotId, origin, sourceInstanceEpoch, sourceRevision,
capturedAtUtcMs, documentGeneratedAt, recordCount, groupCount,
contentHash, sourceFileHash, sourceCurrent, historicalOnly,
authenticated, physicalIdentityVerified, permissionsRestored,
executionPermit, flightCommandReleased
```

`schemaVersion=1`；`snapshotId` 为规范非空 UUID；`origin=session|file`。session 的 `sourceInstanceEpoch` 为原 Passport UUID，`sourceRevision` 为其非回绕原生 uint64 十进制字符串（可为 `"0"`）；file 分别为明确空串和 `"0"`。`capturedAtUtcMs` 是本地冻结/打开入场 UTC，正整数且不超过 JS exact 范围，不是远端测量时间。

`documentGeneratedAt` 保留该固定 JSON 的原生成时间字符串，最多 128 个 UTF-16 单元，旧格式缺省或空值为明确空字符串。旧导入器只要求它是字符串，但新工作区独立拒绝超过 128 的整份文件，不截断原文，也不改变旧 `importJson` 接受域；否则小 descriptor 可能把近 32 MiB 的根字符串带到界面。记录/组数为 0..2000。`contentHash` 是实际缓存 `bytes` 的小写 SHA-256；file 的 `sourceFileHash` 等于这个原文件字节 hash，session 的 `sourceFileHash` 为空。文件字节 hash 与下述规范化详情 JSON hash 是不同对象，不得互换。

Worker 总是输出 `sourceCurrent=false`；只有原 Passport 可依据 session 的原实例和修订关系计算本地缓存是否还对应当前记录。追加新记录只能让旧冻结快照不再当前，不能替换其内容。file 永远不是活动维护来源。`historicalOnly=true`，其余身份、权限和飞行保证均 false；缓存“当前”不证明文件仍未变、实物身份或当前维护健康。

## 3. 请求与公共分页

所有请求闭键校验，数字接受实际有限整数值，不接受数值字符串、Bool/null 或非整数 Double。对某快照的请求必须精确携带其原 `snapshotId`；详情索引还必须属于同一个完整来源组。不得把另一页、另一快照或任意公开 Map 当私有缓存。

| action | 请求精确键 | reply |
| --- | --- | --- |
| freeze | snapshotId, sourceInstanceEpoch, sourceRevision, capturedAtUtcMs | `{descriptor}` |
| open | path, snapshotId | `{descriptor}` |
| save | snapshotId, contentHash, path, overwriteConfirmed | 见第 4 节 |
| groups | snapshotId, afterIndex | 六键来源组页 |
| records | snapshotId, groupId, afterIndex | 七键记录摘要页 |
| detail | snapshotId, groupId, recordIndex, offset | 十键详情片 |

`afterIndex` 是下一项的零基数组索引，范围 0..totalCount，不是带权限的游标。分页每次最多 8 项，允许最终空页。来源组 ID 为完整 `{vehicleId, historyScopeId}` 规范 JSON 的 SHA-256；历史组保原 source7，不按数值机号把不同档案、epoch、component 或 battery 合为实物。legacy 的 scope 为空、source 为 `{}`，仍明确未认证。

groups reply 六键：

```text
snapshotId, afterIndex, nextIndex, hasMore, totalCount, rows
```

每个 row 精确 13 键：

```text
groupId, vehicleId, historyScopeId, historicalOnly, recordCount,
maintenanceStatus, predictiveRisk, estimatedRiskScore, confidence,
sourceSelectionRequired, manualReviewRequired,
historicalTelemetrySource, physicalIdentityVerified
```

`vehicleId` 为 1..255；组记录数为 1..2000；`physicalIdentityVerified=false`，`manualReviewRequired=true`。状态/评分复用当前 Passport 的启发式评估，不是适航结论：status 为 unknown/monitor/inspection_due/grounding_recommended，risk 为 unknown/low/medium/high/critical，score 为 0..100，confidence 为 low/medium/higher_but_not_validated。历史组的 `historicalOnly` 表示其原历史来源，legacy 组此位 false 也不代表实时认证。

records reply 为六键页外加 `groupId`，每个 row 精确八键：

```text
recordIndex, groupRecordIndex, timestamp, timestampMs,
fieldCount, historicalOnly, metrics, hasModifications
```

`recordIndex` 是私有完整 records 的实际下标；`groupRecordIndex` 是该组内的下标。`metrics` 仅列出已经规范保留的 currentAmps/temperatureC/vibrationRms/batteryDegradationPercent/flightCount；缺字段是未知，合法负温度不是未知。摘要不带整条 note/modificationRecords 或完整 latestRecord。原记录数组顺序不会伪装成跨机物理时间顺序；历史评估继续按源文件 sequence 选择最新页，禁用物理两点磨损趋势。

detail reply 十键：

```text
snapshotId, groupId, recordIndex, offset, nextOffset,
hasMore, totalCharacters, contentHash, text, canonicalJson
```

`recordIndex=-1` 为完整组 assessment，其他为属于该组的真实 records 下标。返回的是完整规范保留值的 compact JSON 连续片，不是未经 schema 规范化的原始输入对象，也不伪称原文件字节。`canonicalJson=true`，`contentHash` 绑定这一完整规范 JSON 的 UTF-8 字节。全文上限 64 MiB（允许原文件近 32 MiB 的单条记录加 assessment 固定说明），超界明确失败。

`offset/nextOffset/totalCharacters` 以 UTF-16 code unit 计，范围 0..67108864。单片最多 2048，片尾若为高代理项则退一单元；请求落低代理项明确拒绝，不能漏字或以替代字符冒充全文。`nextOffset` 是唯一续读位置。分页/折叠/尚未打开不等于已核完整，也不能根据已读片段推导维护结论。

## 4. 显式保存和文件事实

保存只允许 session 私有快照，精确核对其 snapshotId/contentHash，`overwriteConfirmed` 必须是实际 Bool true。保存不能读取或重新导出当前活动 Passport；即使活动记录后来变化，也不能偷偷改写本次固定快照。打开的独立历史不借此路径另存为活动来源。

路径必须为原样本地绝对 `.json` 路径，不做 trim 或 URL 到本地路径的宽松转换；长度最多 4096，UTF-8 可往返，无 NUL/control、UNC/device 路径。Windows 拒额外冒号、设备名与段末点/空格。拒目录和目标符号链接。存在检查不是跨进程路径/父目录竞态免疫，不认证文件所有者或防篡改。

必须在选择路径之后另行明确确认可能覆盖同名文件，不能只信桌面/移动 picker 各自的覆盖提示。`QSaveFile::setDirectWriteFallback(false)`，分块完整写入，短写/取消在 commit 前取消临时文件；不会自动重试。

save reply 精确 11 键：

```text
snapshotId, contentHash, path, fileWriteAttempted,
fileWrittenKnown, fileWritten, readbackVerified, readbackError,
fsyncVerified, durable, authenticated
```

pre-commit 拒绝/短写/取消保持 `fileWrittenKnown=true,fileWritten=false`。commit 返回失败时结果未知，`fileWrittenKnown=false`，不能仅凭 false 值声称目标没被替换。commit 成功后固定 `fileWrittenKnown=true,fileWritten=true`；随后取消、界面失效、来源变化或回读失败都不撤销该事实。

成功提交后独立重读相同路径，并且读取字节与固定快照完全一致才 `readbackVerified=true`。读回失败/变化保留已提交事实；外层 `ok` 仍表示实际 commit 成功，`error/readbackError` 可带诊断。`readbackVerified` 不是 fsync、断电耐久、跨进程锁、签名或物理身份保证；`fsyncVerified/durable/authenticated` 均恒 false。

读取固定 0 < size ≤32 MiB，分块读取、额外尾字节、错误、最终 size 和 close 后错误均核对，并要求 UTF-8 原字节往返。文件仅在一次读取时形成历史快照，不持续监视外部变化。完整复用旧根5以及含历史记录时条件根6的 `historySourceContract`；标记和实际历史记录存在性不符整份拒绝，旧 reader 不得静默丢来源再按同机号并组。

## 5. 工程交接（均未执行）

本轮仅源码实现与直接静读；没有编译、测试、解析器/业务 helper 执行、UI 启动、文件实际读写、仿真、刷机或硬件证明。后续工程验证至少覆盖：

- freeze 后活动 append/import/clear、A→B→A；固定 snapshot 字节不被新记录替换。
- 旧根5、历史根6、marker 缺失/多余/坏值、重叠历史页与完整来源组隔离。
- result/finished 两种次序、取消、对象销毁、失败保旧 opened、nonce/actual worker 不同不接纳。
- 空记录、2000 条、32 MiB、深度/节点/超界原生整数、巨型 note/modification 全值不静默裁剪。
- generatedAt 缺省/空串/恰好 128 单元保留；129 或近文件上限的字符串整份拒绝，保留旧打开快照与活动记录，不把截断文本作为原文显示。
- 8 条分页、错误组/跨组 index、assessment -1、UTF-16 emoji 片界、低代理 offset 拒绝和全文 hash。
- 路径 URL/UNC/device/控制字符/符号链接/目录；短写、commit 不确定、commit 后取消及回读不匹配。
- `fileWrittenKnown=false` 的 UI 不显示为“未写”；没有 durable、身份、维护健康或权限恢复的推断。

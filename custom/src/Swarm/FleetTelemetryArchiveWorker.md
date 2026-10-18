# FleetTelemetryArchiveWorker：选定入站观察的后台 journal

310–314 将目录 slot 的 Qt iterator 改为共享 `FleetArchiveDirectoryInventory` 的 Native error-aware 枚举：只有实际枚举到成功 EOF、目录 identity 稳定、namespace 数量/类型/规范路径均通过后，才继续 NewOnly 创建。无 QDirIterator fallback；Windows/POSIX 的实现仍需工程师在实际目标平台验证。`current.directoryInventoryEnumerationCompleted` 初始 false，仅这次受限检查成功后 true，不写入盘内 header/frame/footer，也不等于目录全集认证；原 `directoryEnumerationCompletenessVerified=false` 不变。未运行枚举、写盘、编译或测试。

305–309 新增[独立只读HistoryReader](FleetTelemetryArchiveHistoryReader.md)与Codec共享Header/Footer校验，writer格式和写入/flush/关闭事实不变。读取只交付完整验证前缀/固定public metadata，不把盘内clean=false候选提升为末端flush或权限证明；完整历史closure/全集索引/长期恢复仍未完成，未运行查询、写盘或测试。

本模块只把[遥测 Codec](FleetTelemetryArchiveCodec.md)的受限值写入独立 NDJSON，供工程诊断。它不是完整遥测记录器、真实 MAVLink wire dump、数据库、导入/回放器或飞行控制组件；无 Vehicle/source QObject 指针，不操作 live BlackBox、Registry、任务或审批。不宣称 source/CRC/签名认证、恢复授权、执行许可或完整采集。

## 原生接口和 frozen scope

构造 `FleetTelemetryArchiveWorker(directory, exactPolicy, runId, includePositionTelemetry, exactSources)` 是 parentless QThread，不创建文件；根门面实际 start 后才做磁盘操作。根负责强类型 native 来源和实际授权/同意，worker 只持值，不用 bool 代替账号/机载身份认证。

exactSources 是 `1..32` 个精确三键 Qt Map：vehicleId 为真实 Qt 整数 `1..255`、sourceEpoch 为规范小写 UUID、initialSequence 为真实 QString `"0"`。IDs/epochs 均唯一，构造冻结 epoch→vehicleId；header.sources 保存该实际选定范围，因此某架机从未发送白名单消息也不会在来源范围中消失。positionTelemetryConsent 保存构造时 native bool，不能由单条记录扩大。

- `static policyValid(exactPolicy)`：供根复用严格固定政策，不改 caller 值。
- `enqueue(Record)`：GUI native 直接有限入队，不逐帧 queued invoke/signal；Codec 一致性和最多 4,096 bytes JSON 解析在共享锁外，actual sysid/sourceEpoch/position consent 必须匹配冻源。写前再次核，不能把别的 run、机体或 epoch 数据混入。
- `noteCaptureSummary(exactSummary)`：以受限值更新最后已观察 prefix 诊断，根可 250 ms 合并一次、停止前最后一次，不逐帧 queued。
- `requestStop(drain=true)`：异步关闭接收入口，正常排空；false 弃队列和下一 frame 写入边界尚未写的 in-flight，不能撤回已经开始/完成的 write。最终空队列决定之后，后续 stop 不改结果；false 请求在最终决定前不可被后续 true 翻回。
- `current()`：mutex 保护的合并快照，只公开 basename，不公开目录、SQL、原始 payload 或源 QObject。
- signals：ready(runId)、failure(runId, fixedCode)，不是每包通知；根将 actual finished 连接 deleteLater，不得在运行中删除、wait/join 磁盘线程或 terminate。

valid 构造即接受有界 opening pending，ready 仅表示文件/header 已写，非 flush/durable 证明。在 QThread.run 前已经请求停止则不 NewOnly 创建空文件，把已接 pending 计 discard，cleanShutdownConfirmed=false。opening 中停止可能已经创建候选文件，保留其真实状态，不假称从未创建。

## 精确政策和容量

须恰好六个真实 Qt 整型字段；bool、double、string、invalid/typed-null、额外/缺失字段拒绝，不补默认或调值：

| 字段 | 允许范围 |
| --- | --- |
| maximumQueueFrames | 16–4,096 |
| maximumQueueMiB | 1–16 |
| batchFrameLimit | 1–128 |
| maximumFileMiB | 1–1,024 |
| maximumRunFiles | 1–128 |
| flushIntervalMs | 100–5,000 |

queue + in-flight 同时计条数/保守字节费用（JSON、QString 和节点开销）。写成/弃掉的 retained batch payload 在释放容量 credits 前清除，不把已写数据仍留在 batch 却让 GUI 补满新队列。空 batch 容器、单次写缓冲、Qt JSON/文件缓冲、其它生产者持值和线程开销另有界，不把政策宣传为全进程 RSS 硬上限。共享 mutex 内没有磁盘/目录/flush；互斥和有界分配不等于硬实时无延迟。

record sequence 必须严格递增、不回绕，不要求从 1 连续；有意隐私/白名单过滤会产生 source/record 空档，不能从差值凭空增加丢包计数。queueDroppedFrames 只记录本 worker 实际超限入队拒绝，discardedFrames/failedPendingFrames 分别描述明确弃尾与错误时未完整写出 pending。各计数是非绕回 signed64 十进制字符串，耗尽时拒绝并停止，不清零或重试旧 run。

## 文件、合作目录锁与 header

目录必须显式存在、可读、规范绝对本地路径，长度不超过 4,096，无 NUL、UNC 或 symlink。固定文件名 `fleet-telemetry-<runUUID>.ndjson`；NewOnly 防覆盖，owner read/write，worker 独占自己的 handle。不修改、删除、改名、自动轮换、修复或导入旧文件；创建/权限/header 失败保留真实候选文件，计入以后 namespace 费用。

目录创建 slot 使用固定 `.fleet-telemetry-catalog.lock` 的 QLockFile，worker 内非阻塞 tryLock(0)、staleTime=0；持锁贯穿 namespace scan、NewOnly reserve 和 header 完成或失败关闭，随后释放，允许不同 UUID 的记录 run 并行。合作创建者使用同一锁和观测枚举，避免同时通过“还剩一个 slot”的 race。锁文件路径拒绝 symlink/非普通文件/非规范或异常大文件，不主动 removeStaleLockFile；staleTime=0 只禁年龄阈值，Qt 仍可能按 PID/进程名检测 stale 并清理，正常释放会删除本次 owner 锁。[Qt 5.15 官方说明](https://doc.qt.io/archives/qt-5.15/qlockfile.html)不替代工程目标 actual release/平台验证。

共享 inventory 逐项枚举包括无关/隐藏条目，总最多 4,096；case-insensitive `fleet-telemetry-` 匹配都计入 namespace，最多 maximumRunFiles。Native API 的枚举失败、取消、超限或不稳定 identity 不能变成成功空列表，统一为固定 `telemetry-directory-inventory-failed`，不公开 OS/目录错误文本。返回成功后仍逐一复核规范小写 UUID `.ndjson` basename、Native regularFile/non-symlink、Qt 普通文件/非 symlink/规范路径，异常 namespace 拒绝；实际数量达到 maximumRunFiles 拒绝创建，不能删历史腾空间。目录锁仍贯穿 scan 到创建 slot，并非仅锁住一次枚举。`current.directoryInventoryEnumerationCompleted=true` 只记录这次已完成的受限检查，不表示之后不会发生变化。

历史上 300–309 曾仅凭 Qt iterator 的 hasNext 结束观察列表；此路径已在 310–314 替换，不再以它作为创建容量的成功依据。runtime/header/footer 的 `directoryEnumerationCompletenessVerified=false` 仍保留：成功 EOF/identity 对照不等于跨时刻全集、目录原子快照或恶意并发认证。目录 ACL/未知外部 writer/恶意 TOCTOU/硬链接/实际磁盘块占用仍是部署边界，不称全目录强硬 quota。旧文件可来自不同 maximumFileMiB，段数不等于整个磁盘预算。

header 固定 kind=header、format=merivus-fleet-telemetry-journal、version=1，含 runId、实际 sources、position consent、policy 和 openedUtcMs（真实 host 开文件近似时间，`1..9007199254740991`，不是机载采样或认证时间）。时钟不合法时保留 NewOnly 候选并报固定错误；所有认证/CRC/许可/fullTelemetry/fsync/durable/wire-reconstruction 标志 false。header.sources 只是 native scope，不认证飞机。header JSON 不含 newline，最多 4,096 bytes，写入再加一 newline；超限不放宽或删除来源来伪造成功。

## write、flush、footer 的不同事实

Record JSON 最多 4,096 bytes，无 CR/LF，写时追加 newline。主文件上限包括 header、frame 和 footer，始终为 footer 预留 `4,097 bytes`；容量不足就停止、保留已有前缀，不自动换文件/重送。实际 write 满行才增加 writtenFrames/lastWrittenSequence；正数部分 write 只增加实际 writtenBytes、标 partialWriteObserved，失败 pending 不能当成已写成功。

按 flushIntervalMs 和正常 stop 做 QFile.flush；只有实际 flush=true 才更新 flushConfirmedFrames/Bytes/Sequence。成功 write 不等于 flush，flush 也不等于 fsync、硬件失电持久性或 durable；fsyncVerified/durableVerified 永远 false。磁盘错误只停止观察存储，固定 errorCode 不泄漏目录或系统错误文本，不触发控制行为。

可正常终结时追加 kind=footer，同样最多 4,096 bytes + newline。footer 记录 writtenFrames/Bytes（bytes 明示不含本 footer）、已发生 flush level、最后 seq、queueDrop/discard/failedPending、最后 source summary 和固定 error；健康的容量停止可记录错误 footer。部分 write 或 I/O/flush 失败可能完全没有 footer/尾行不完整，保留实际文件，不补伪 clean；未来读者最多识别有效前缀，不能假定最后一行必为 footer。

盘内 footer.clean 固定 false；acceptedQueueDrained 和 closeCandidate 仅描述已处理 accepted subset/候选关闭，finalFlushAcknowledgementIncluded=false。因为最终 flush 在写 footer 后才能发生，同一待 flush footer 不能携带它自己的完成确认。只有 footer 实际满写、最终 flush=true、无错误且为 drain 决策时，运行态 current.cleanShutdownConfirmed 才 true。最终 flush/close 失败仍 false；离线看见候选 footer不能推出 runtime 确认。永久历史 closure/独立落盘完成证明留待后续实现，不能以此批代替。

## 来源摘要与未知窗口

exactSummary 必须精确 10 键：九个 `0..INT64_MAX` 规范 QString 十进制值（receivedFrames、filteredFrames、invalidFrames、queueRejectedFrames、lastOfferedSequence、sourceMissingFrames、sourceRetiredCount、captureStartUtcMs、captureEndUtcMs）及真实非 NULL bool sourceCoverageClosed。未提供则为 0/false，provided=false；摘要只能在最终冻结前提交。失败/未开文件路径会强制 sourceCoverageClosed=false。

这些由根实际 native prefix 观察产生，不是任意外部 JSON/import；worker 不认证 caller，不把 sourceMissing 的已知数推成所有网络丢包。sourceCoverageClosed=false 表示存在未观测尾/窗口，数量 unknown；为 true 也仅表示根定义的本次观察 prefix 闭合，不等于白名单之外消息或完整遥测都已保存。源断开可以排完已接受子集但 sourceCoverageClosed 仍 false。footer/runtime 的 worker queue 计数与 source filtered/invalid/missing 各层不同，不机械相加冒充去重总损失。

## 本批证明范围

仅源码实现、限定审阅、文本检查，未调用 Codec/worker/policy/inventory helper、锁/目录枚举/NewOnly/write/flush、应用、编译、测试或设备。工程师需验证实际 Windows/POSIX EOF/权限/中途枚举错误/identity 替换、Qt 5.15 文件/锁、并发 slot、容量/部分 write/flush错误、source取消/开文件前stop、状态销毁、隐私 scope 和计数/摘要时点。仍无真实采集验收、完整历史 closure、全原始遥测、自动轮换或控制权限恢复。

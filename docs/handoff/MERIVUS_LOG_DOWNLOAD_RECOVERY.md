# MERIVUS 机载日志有界缺块补读合同

## 505 范围与交付状态

本轮修改既有 [LogDownloadController.h](../../src/AnalyzeView/LogDownloadController.h) 和 [LogDownloadController.cc](../../src/AnalyzeView/LogDownloadController.cc)，继续由原 LogDownloadPage 的 Refresh／选择／Download／Cancel 操作实际使用。公开 model、requestingList、downloadingLogs 及原调用接口保留，没有新增 UI、工程登记、Session／Role 授权器或外部接收服务。

这是机载日志在**本进程、同一下载会话**中的有限缺块补读和本地文件成功语义修补，不是整个 Fleet 的断网恢复补传，也不是跨重启 checkpoint／resume。原 EraseAll 仍是单独的既有破坏性操作；不能把整个控制器称为只读服务。

本合同按冻结后的两份源码直接静读整理。[精确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)为 2 源码＋12 文档共 14 路径，仅本合同是新文件。[40 项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；静读、普通文本检查不代表构建、协议兼容或运行通过。

## 1. 入口、来源与退休

Refresh 固定原 Vehicle、MultiVehicleManager 与本地来源代次，先清旧目录，再按同一实际来源建立目录 Run；模型通知中换车不能被旧 Refresh 吸收为新的请求。下载只接受本控制器实际收到、完整且仍合格的目录，随后独立冻结整批选择的条目对象、归一化 id、声明 size 和 UTC 时间；UI 后续勾选变化不能替换该批目标。

实际目录／数据处理从原 MAVLinkProtocol 的 messageReceived 进入，不再以 UAS 的借用数据指针或调整后的 logEntry／logData 信号填充文件。每个 Run 固定并复核：

- 实际 Toolbox、Protocol、Manager、当前 Vehicle 和实际 VehicleLinkManager 对象；本车 systemId、默认 component、firmware／vehicle 类型。
- 原 PRIMARY QObject、弱控制块与实指针、仍为该管理器成员的关系、通道和本地 GCS system／component。
- 本地来源代次、原 Run 身份、原完整目录及文件条目值、原单调时钟和期限。

RAW 回包须来自同一 Protocol、同一 PRIMARY、本车 sysid 和默认 component，且只能进入当前目录或当前文件阶段。换车、PRIMARY 变化、断开、失联、相关对象销毁、固件／机型变化或本地 systemId 变化会退休原会话和目录资格；来源 A→B→A 不复活原资格。代码不再为下载关闭通信丢失检测。新的显式 Refresh 才能建立新目录依据。

QPointer、独立持有的 Run／文件状态、忙闩和通知后尾核用于限制同步回调销毁／换源／重入；原 Run 的晚回调不能接管后来的 Run。RAW 正在处理时，下一数据请求排到原栈之后，并再次核同一 Run／文件／来源／期限，不在当前 RAW 栈递归发下一 chunk。

这些是**本次本地观察来源**约束，不绑定远端 boot／version epoch，不是远端文件或身份认证。此 Protocol 槽可能排在部分 Vehicle 消费者之后，不构成“最早 RX 因果切点”。

## 2. 固定资源与期限

| 对象 | 当前固定上限／规则 |
| --- | --- |
| 目录 | 最多 4096 项；目录 Run 总期 60 秒 |
| 选择 | 一批最多 64 个实际目录项；size 必须为正 |
| 单文件 | 声明 size 最多 512 MiB；每文件原期限 15 分钟 |
| 整批 | 声明 size 总和最多 1 GiB；下载 Run 原总期 1 小时 |
| 当前工作集 | 每 chunk 最多 512 个 90 字节 bin，即 46080 字节；仅保当前 chunk 位图 |
| 缺口补读 | 500 毫秒检查周期；连续无新进度最多发 5 次补请求，然后停止 |
| 文件名冲突 | 同一目录最多 1000 个候选：原名及 `_1` 至 `_999` |

选择数和声明字节预算在创建文件前检查。1 GiB 是冻结所选目录公告 size 的合计，不是重试在内的网络总流量、实际剩余磁盘空间或长期文件留存上限。目录／批次时钟从各自 Run 创建开始；每文件起点在发布该文件状态前固定，包含随后扩展名读取和文件准备，不把后续文件误按批次零点计时，也不在打开文件后重基。共用的私有 _runCurrent 检查同时包含原文件期限，不能仅靠下一次 500ms timer 阻止过期 RAW 写入或完成。

回包、重试、重复或非法数据不延长上述绝对期限。目录只有新收到且完整填入的条目会重置缺口进度；数据只有**新完整块恰好写入成功**才重置重试计数和下次缺口时刻。已有 bin 的重复包不累计唯一字节、不续缺口钟。补读只请求当前 chunk 中第一个连续缺口，并夹到冻结文件尾；超过重试或时间预算停止本批，不自动跳过失败文件继续下载。

## 3. 目录与 RAW 编码支持边界

首个目录响应固定 num_logs 和 last_log_num；同一目录后续冲突或超 4096 项会失败。零项可形成完整空目录；尚未收到的条目不能用于下载。

保留旧 PX4 dense、ArduPilot 1-based 映射：模型 id 为归一化连续索引，APM wire id 加 1，APM 零 size 的目录占位响应不当作有效条目。last_log_num 在本次目录中必须一致，但没有据此实现任意 sparse／轮转 id 目录，也不承诺所有机载日志实现均兼容。重复目录项的声明 size／UTC 时间变化会终结目录，不能静默更换冻结条目。

LOG_ENTRY／LOG_DATA 的 MAVLink 1 消息须具固定完整 payload 长度；MAVLink 2 允许从 1 字节至该消息完整长度的合法尾零裁剪。代码先把完整目标 payload 清零，再复制实际字节并使用生成的解码器，避免把裁剪后的未提供字节当成借用缓冲区内容。该归一化不建立签名／CRC／身份认证。

## 4. 字节写入、位图与完成条件

当前 LOG_DATA 必须具有原 wire log id；count 为 1..90，offset 为 90 字节对齐、严格小于冻结 size，并属于当前 chunk。中间 bin 必须恰好 90 字节，最后 bin 必须恰好为冻结 size 的剩余长度；不接收越尾、短片段或重复 bin 作为新进度。

同一当前 chunk 内可以乱序补齐。先 seek，再要求 QFile::write 的返回值**恰好等于**本块预期长度；seek 失败、−1 或部分正写均停止批次，不能预先置位。完整写入且原来源／文件／期限仍匹配后，才置 bit、增加唯一字节和更新速率。旧 chunk 数据不能修改新 chunk。

只有当前及此前所有 chunk 完整，且唯一写入字节数等于冻结 size、本地 file.size() 等于该 size，才尝试 flush。随后 close 无错误、同目录 rename 成功，并在原来源／文件／期限尾核后，才把条目状态设为 Downloaded。预分配或文件表面大小、收到最后一包、已发完请求、进度条接近 100% 都不是完成依据。

Downloaded 仅表示“本次冻结公告 size 的全部字节已按本地规则收齐并 flush／rename”。flush 不等于 fsync／断电 durable；这一流程没有远端文件 hash、签名、ULog 内容有效性或端到端内容校验。日志协议的公告 size 可以近似：偏大时严格收齐可能保守拒绝正常文件；偏小时若 peer 按请求裁尾，也可能只得到完整的公告前缀。因此成功不证明真实远端文件已取尽，失败也不能据此断言远端文件损坏。

## 5. 路径、partial 与失败事实

目标必须是已有的绝对目录；本地 file URL 先转路径，再取该目录的 canonical 路径。不会自动创建下载目录。完成名沿原格式：

```text
<canonicalDir>/log_<normalizedID>_<yyyy-M-d-hh-mm-ss|UnknownDate>[_1.._999].<ulg|px4log|bin>
```

PX4 默认 `.ulg`，既有 SYS_LOGGER 参数存在且为 0 时用 `.px4log`；其它固件用 `.bin`。时间取目录 UTC；早于 2010 年显示 UnknownDate。实际写入路径是在该完成名后追加 `.partial`。先排除已占用完成名／partial／符号链接候选，再以 WriteOnly＋NewOnly 创建新的 partial，不以 Truncate 覆盖既有文件。

失败、超时、取消、换源或写盘错误停止整批，尝试关闭当前文件，但**不删除已有 partial**，不自动读取或续写它。状态根据实际事实区分 finalized file retained、partial file retained、no partial file created；排队未开始项标注 not started，不能把它们伪报成功。若 rename 已成功而随后原来源失效，保留已发布文件事实，不虚称仍有 partial，也不恢复下载权。

重试一次人工 Download 会重新建会话／新文件名，不恢复旧内存位图。partial 可能只有部分有效块或未持久化字节，不应当作完成日志；需由工程师另外辨识和处理。NewOnly、冲突检查和 rename 不构成对恶意并发文件系统或整机存储权限的完整安全认证。

## 6. LOG_REQUEST_END 与旧 EraseAll

LOG_REQUEST_LIST 在部分系统上可能暂停机载日志记录。正常终结、失败或取消时，仅在本 Run 已可能交接过 117／119 请求、原来源仍有效且尚未发 END 的条件下，最多一次 best-effort 发送 LOG_REQUEST_END（122）。在发送外调前固定 once 标记；不重试，不借后来连接或新 Vehicle 补发。尚未发读请求的准备失败不先发送 END，来源失效和析构不发 END。

END 没有 ACK 或恢复证明；进程退出、断链或多 GCS 并发时不能保证送达、机载记录恢复或与其它控制方互不干扰。它也不是分布式控制租约。取消仅停止本地接续，不宣称撤回已发 wire 请求。

既有 EraseAll 仅在空闲时按原实际目标发送 LOG_ERASE，再在同源条件下走原刷新流程，不把发送结果当擦除 ACK。下载和缺块补读不会自动调用 EraseAll，也不发送飞行命令、修改参数或借用可选遥测配置授权。本轮未新增身份审批，不能据此宣称该旧入口具企业级授权。

## 7. 仍未解决的协议与产品范围

- LOG_ENTRY／LOG_DATA 没有本次操作 nonce，远端文件没有绑定 hash 或 boot／version epoch。同源迟到的同 id／offset 数据、远端重启后同 id 重用仍可能歧义；本地来源代次不能消除该协议限制。
- 请求仍走普通 Vehicle sender；plugin 之后没有本下载 Run／PRIMARY／完整原 LOG 消息的最终写栅。发送前后来源检查约束本地消费者，不证明最终 wire 目标、字节未被插件改变或远端已执行。
- 没有持久化位图／checkpoint、跨重启断点续传、跨机接管、远端幂等 ACK、Fleet outbox／控制中心接收端，也没有重放任务／飞行命令来伪装补传。
- 本轮不修改 MAVLink 协议、PX4 固件、遥测频率、视频、任务执行许可或 P0 放行条件，不改变原 25 目标和六类实质剩余。约 40% 完成／60% 剩余仍只是源码工作量粗估，不由本批路径数或补请求数推算。

## 8. 工程师接手

请按[工程师交接 505 的 40 项场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)完成 Qt／MSVC／moc 构建、旧页面交互、真实协议来源与裁剪、缺块／重复／过期／换源、负写／短写／flush／rename 故障、通知同步重入及 END 行为验证后再决定使用。尤其需要验证机载 num_logs／last_log_num／id 和 size 实际语义；遇到支持边界应保持保守失败，不擅自填位图或放宽文件成功条件。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash 实现，未测试、仿真、操作硬件、修改 PX4、刷机、放行、提交、更新 PR 或打包。所有工程场景均为【未执行】，不得把本合同当作已可安装、已可飞或已验收证明。

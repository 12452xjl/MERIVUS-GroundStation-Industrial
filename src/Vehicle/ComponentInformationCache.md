# ComponentInformationCache：组件元数据原始文件缓存

## 526：传输字节 namespace 与准确坏项退休

本类仍是每个目录一个实例、非线程安全的本地有界文件缓存。默认实例的目录、最大50项、旧 `.meta/.cache` 格式和 access/insert 接受域不变；访问计数与 LRU 只管理本机留存，不证明文件正确、来源、当前构型或适航。

组件元数据主流程使用新 tag：`raw-v1_<旧crc_type_translation>`。这只是同缓存目录内的新 key namespace，不搬迁、不升级或读取旧 tag 下的解压 JSON 为原始传输件。`.cache` 后缀本身不代表原始、已校验或已解压；调用方必须区分 namespace。新 tag 的数据只能是原始下载／传输字节，CRC 按其原规范计算；解压／JSON 解析使用后台产生的独立快照，不能与原始缓存文件混用。

缓存命中只取得候选路径，不直接接纳业务内容。Manager 须由同请求的实际异步 worker 重新读原字节并核对应 CRC，再用本次独立 JSON 快照继续。缓存始终保存原始传输字节，但实际 PX4 GENERAL 可由 Manager 明确允许解压 JSON 与声明 CRC 比较，并标为 `decoded-px4-general` 兼容域；原始字节本身匹配仍优先标 `transport`。其它有 CRC 声明的路径只接受 transport；无声明仍仅 `not-compared`，不借 cache tag 冒称原始 CRC 相等。CRC 不是签名、远端身份、版本真实性或防恶意碰撞认证。不能用 LRU 时间、文件名中 CRC、旧命中或 insert 返回的已有路径代替本次校验。

## 新 native invalidate

`bool invalidate(const QString& fileTag)` 非 Q_INVOKABLE。在任何路径构造、文件操作或内存修改前，严格要求 tag 长度1–128、全部为 ASCII `[A-Za-z0-9_-]`；空、点、分隔符、冒号、通配符、非ASCII／NUL等均拒绝。不会从 tag 接收目录、路径或多个条目。

只对本实例固定目录下准确的 `<tag>.meta`、`<tag>.cache` 各调用一次 `QFile::remove`，两次均尝试，不因第一项失败就遗漏另一项。不删除缓存根、不递归、不清理其它 tag、不执行 LRU、不重试或修复目录。返回 true **仅代表两个实际 remove 调用都报告成功**；任一个失败（包括文件已经缺失）即 false，不用 exists=false 将权限或 IO 未知冒称为确认不存在。部分删除不回滚，false不能改称两个文件都仍在或本次没有任何效果。

无论两次删除是否全部成功，都遍历真实 `_cachedFiles` 移除**所有** value 精确等于该 tag 的项，而不只移除一次计数；`_numFiles` 随后设为实际 map.size。其它 tag／排序键及 `_nextAccessCounter` 不改，不按猜测减一，也不为失败项保留看似有效的内存身份。无效 tag 则完全不修改状态。

Manager 只在原实际 cache worker 校验失败后，对原冻结 raw tag 明确调用一次 invalidate；只有 true 才允许原URI重新下载一次。false应停止本次路径并保拒绝，不能反复下载／删除或把部分失败当缓存已空。此 API 不自动下载、迁移、解压、重算 CRC或创建文件；也不保证并发非合作进程不会随后改动文件。

旧 insert 在任一旧 meta/data 已存在时仍可能删掉传入文件并返回已有路径，该历史行为本批不扩大。新主流程不能消费该返回路径冒充本次已核 JSON：原始缓存与独立已核 JSON 必须分离，且之后每次命中仍重新核验。`invalidate` 不是允许任意同名文件覆盖、原始来源认证或文件系统原子事务。

## 静态交接边界

本批只修改源码与直接静读，未编译、测试、运行缓存、后台 worker、下载、CRC／解压／JSON实现或进行真实缓存IO。工程师需验证：新 namespace 不读旧解压件；合法 tag、128/129与路径字符拒绝；同tag重复 LRU记录全部退休且其它项不变；两文件分别删除失败／已缺失／部分成功时 false且不重下；原合法命中坏CRC→一次清理→一次原URI下载；新独立JSON不被旧 insert 路径替换；磁盘权限／并发／Qt文件系统语义和真实主流程生命周期。以上均未执行，不是缓存或组件元数据可靠性验收。

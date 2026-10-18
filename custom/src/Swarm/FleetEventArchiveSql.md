# FleetEventArchiveSql：共用 schema 与严格只读页

本模块从[存储 Worker](FleetEventArchiveWorker.md)抽出其原 schema、SQLite 身份检查、规范事件镜像检查和历史分页，供 Worker 与独立[封存历史 Reader](FleetEventArchiveHistoryReader.md)使用。它不是 QObject，不保存 Vehicle、BlackBox、Registry、Replay、审批、权限或控制对象，不开连接、不选择目录、不发命令、不恢复授权。

## 唯一声明来源

`applicationId()` 为 `1296389702`，`schemaVersion()` 为 `1`，`maximumRecordBytes()` 为 `512 KiB`。`metaSchema()`、`runsSchema()`、`eventsSchema()` 返回原有精确 CREATE 声明；Worker 创建/验库、文件 header 检查和 Reader 均复用这一来源，不再保存两个可漂移 schema 版本。抽取不迁移已有数据库、不改表、不减弱旧声明或唯一/外键约束。

`scalar(database, fixedSql, value)` 仅供受信 native 调用点的固定 SQL/PRAGMA 使用，不是用户 SQL 执行端点；它检查一个真实整型非 NULL 值和确切 EOF/no error。`recordConsistent(record)` 重用[Codec](FleetEventArchiveCodec.md)的 canonical/hash/18 字段检查，并核 SQL 行镜像与 Record 的 run、序号、时间、类别和来源一致。digest 只检查投影一致性，不认证写入者。

## 身份检查与 metadata

`inspect(database, metadata)` 检查实际 application_id/user_version、精确 sqlite_master 声明、table_info、unique index、实际 run 值型/UUID、foreign_key_check、quick_check 和 singleton meta。未知表/索引/触发器/视图、额外 meta、错误 schema/数据型都拒绝，不删除或重建。

保留原 driver 前限幅：sqlite_master type/name/table/sql 只接收限定长度真实 TEXT；sql 为 NULL 的自动索引另有实际 NULL/typed-length-valid 检查，异常巨大 sql 不会因 CASE 映射 NULL 被当正常自动索引。meta epoch 必须真实 TEXT 长度 36，各整数必须实际 INTEGER；无效和 typed-null Qt 值均不能作为 0/false。schema/index/fetch 到 EOF 时仍检查 lastError，不能把失败当正常完成。

成功时 metadata 恰好为 4 个字段：

- `databaseEpoch`：实际 meta 的规范小写 UUID。
- `retiredCount`：真实淘汰总数，规范十进制字符串。
- `retainedFromRowId`、`retainedThroughRowId`：真实 MIN/MAX append row_id，规范 `0..INT64_MAX` 十进制字符串；无事件时二者都为 `0`。

失败不发布部分新 metadata。inspect 不自行 BEGIN/COMMIT，调用者必须在自己的 SQL 线程选择实际 connection、只读模式、busy timeout、读取事务/快照和文件身份边界。Worker 初始化去除新增范围键，避免把开库时范围混入持续变化的 current 状态当实时范围。

`sealedRunMatches(database, expectedEpoch, sealRunId)` 自主核两个规范 UUID，再实际 inspect/metaEpoch，并 prepared SELECT 检查命名对应的 sealRun：typed clean=1、durable/drop/discard 真实非负整数和确切单行 EOF。它不接受 caller 自报 clean 标志，不要求所有旧 run clean；旧 unclean run 必须按真实状态展示。它不证明整个数据库所有事件的 hash 都通过，亦不证明 seal 操作的目录/文件来源，后者仍由 Reader 的实际文件策略负责。

## readPage 自有事务

`readPage(database, runFilter, cursor, limit, maximumBytes, databaseEpoch, page, fixedError, cancelled=nullptr)` 保留 Worker 的既有完整页契约。参数须为实际规范 epoch、空或规范 UUID filter、非负 cursor、`1..256` 行；page/error 指针必须存在。maximumBytes 是内部 JSON envelope 的实际预算，范围 `1 MiB - 1,024 bytes .. 8 MiB`，不是用户政策的整数 MiB 下限：Worker/Reader 的外层 exactpolicy 仍要求 `1..8 MiB`，Reader 可先预留 1,024 bytes 给 context，不能为通过门禁把实际预算隐式向上取整。最终外层完整结果还需 Reader 自己核总字节费用。BEGIN 后再次 inspect 并核实际 epoch，不能以 caller 缓存 metadata 代替本页实际读视图。

该方法自行 BEGIN/COMMIT，失败或取消 rollback，不可包在调用者已经开始但尚未结束的事务内。Reader descriptor 可在先前独立读事务核对应 sealRun，然后结束该事务，再调用 readPage；文件前/后 fingerprint 和合作封存不可变策略负责跨这两个读视图的边界，不能宣称恶意外部 writer 下同一原子快照。没有 skipIdentity 或借 caller 布尔值绕过的模式。

页按 row_id append 顺序，而非墙钟排序。SQL CASE 在 driver 分配前限制 BLOB/TEXT 大字段、保留真实整数型；每行完整 codec/hash/镜像校验，limit+1 明确 hasMore/nextRowId，整页 UTF-8 紧凑 JSON 也核最大字节费用。原格式、rows、retained/retired/cursor 边界、clean/durable/drop/discard 摘要和各权限禁止标志保持不变；只对请求页的记录做 canonical/hash 检查，不称全库内容都经验证。

可选 `const std::atomic_bool* cancelled` 必须在整个调用期间有效；调用前、逐行 fetch 前后、提交前和提交后交付前读取 acquire flag。取消返回固定 `archive-history-cancelled`，不交付部分 page；不能打断执行中的 SQLite statement、quick_check 或实际 I/O。Worker 默认 nullptr 继续原流程，Reader 可合作取消。有限 4 GiB 库、很多封存段仍可能产生大量磁盘工作，“有界”不等于低延迟或已测性能。

任何成功页都只用于独立历史观察，`historicalOnly=true`，permissionsRestored/authenticated/signatureVerified/executable/completionVerified 均 false；不得回灌 live BlackBox、TimelineReplay、任务、审批或 Authority。跨库 cursor 必须结合 databaseEpoch，不是跨段全局行号。

## 本批证明范围

仅静态抽取/实现、限定源码审阅和文本核对；未执行 helper、SQLite、Codec、Qt worker/应用、编译、测试或设备操作。工程师仍需确认 QtSql 插件、实际数据库/NULL 值型、嵌套事务拒绝、取消时点、忙锁/损坏/大值页及跨段来源/时间/权限边界。本说明不代表 SQL 行为或产品功能已运行验收。

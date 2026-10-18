# CompInfoParam — 527 原子参数定义消费

本模块消费 [ComponentParameterMetadata](ComponentParameterMetadata.md) 的实际 parameter-v1 定义，连接 [ComponentInformationManager](ComponentInformationManager.md) 与 ParameterManager 的三个新 Fact 创建点及参数缓存 CRC 快载。只改定义解释和本地所有权，不新增参数读取／写入命令、UI、许可或认证。没有编译、测试、运行解析器或执行下载／缓存业务验证。

## 整份候选与失败退役

`setJson` 在可能释放旧目录前复制文件名。实际 owner 线程先占本地调用围栏，再 `resetMetadata()`：旧 JSON、indexed 解析缓存和 XML opaque 资格先退休；任何失败都不能沿旧成功目录，也不发布新文件的成功前缀。空路径保留旧式 XML fallback 意图。

新 JSON 只接绝对路径的普通非符号链接文件，大小 1..32 MiB；64 KiB 分块有界读取，检查读取错误／EOF／长度，关闭后再核文件类型、大小、canonical 路径和 mtime，然后交严格 helper。该检查不是对抗任意非合作进程的文件系统认证。helper 的完整候选上限为 16,384 定义、262,144 节点、深度 32、累计 8 Mi UTF-16 字符、每文本 65,536 字符、每 object 最多 128 键／每键最多 128 字符、每 enum 最多 256 项／bitmask 最多 32 项；实际版本仍为 1、七种参数数型，具体字段域以 helper 合同为准，不宣称支持其它版本。

所有 Definition 先在独立私有 QObject owner 下创建完整模板，逐步核原对象、reset 代次和原来源，最后一次交换 live catalogue。旧目录由本地 `shared_ptr` 保到正在进行的虚调用／构造返回；reset 只撤成员引用，不在插件栈内删其旧 opaque。新目录不累积旧代次。

`resetMetadata()` 由实际 Manager 在新 GENERAL 批次前及来源退休时调用，也供本地显式重载使用。非回绕代次耗尽后永久拒绝此通道的新目录／查询。重入 load 可以退休正在处理的旧代次，但不能让外层续作覆盖较新的 reset。

## Fact-owned 快照与 indexed 规则

`createMetaDataForFact(Fact*)` 返回 `QPointer<FactMetaData>`，只接本 Vehicle 实际 ParameterManager 所有、component/name/type 同本次请求的 Fact；创建并最终 parent 到该 Fact，不在 callee 调 `Fact::setMetaData`。三个真实 caller 持 QQ，原生命周期／provider 尾核通过才安装；空返回保留 Fact 自带的通用定义，不能 `setMetaData(nullptr)`。

私有模板／opaque 的借用裸指针不再公开。已有 Fact 保留自己的旧快照，后续目录重载不能释放其元数据，也不声称自动刷新已有 Fact 的 CONSTANT 元数据属性。该策略不把旧快照升级为当前机载定义证明。

直接名称必须精确匹配并同类型。indexed 名称仅单个 `{n}`；其它字符按字面转义，使用 `\A…([0-9]+)…\z` 全名称匹配。两个 indexed 模式同时命中时返回未知，不以最后命中覆盖；短描述和原长描述分别替换原数字串。wire Fact 类型不同不能借错误模板；缺失／歧义／不匹配只生成 actual Fact 类型的通用副本，不伪造已知 volatile。

每目录正／负解析缓存合计最多 65,536 个 `(name,type)`，满后新查询为未知；APM XML lookup 不会因同一查询反复分配模板。该预算不宣称替代旧 XML 文件解析预算或原参数 Fact 总数限制。

## XML fallback 与原生来源

没有已安装 JSON 时，仅原 AUTOPILOT1 路径保留旧 FirmwarePlugin XML fallback。内置 PX4/APM `_loadParameterMetaData` 返回一个有效、parentless、caller-owned 对象；本目录仅接管同线程、仍无 parent 的该对象。已 parented／foreign 对象不重设 parent、不删除；custom 插件必须遵守原生有效返回和所有权转移契约，不能靠 QPointer 或 CRC 保护任意恶意 C++／已经悬空的 raw return。

插件虚调用前后核原目录和实际 provider；`_getMetaDataForFact` 的 raw return 先只比地址，在仍活的本 opaque 实际 direct children 中找到对应对象后才持 QQ／复制。模板名和类型必须符合；内置 fallback 的无名通用对象不算已知参数定义。

在线目录冻结实际 Manager／Vehicle／FirmwarePlugin、Vehicle id／default component／firmware／vehicle type、版本生命周期 epoch 及 PRIMARY weak 控制块和实际指针；释放短期 strong Link 引用后再尾核。原 Vehicle 早期关闭门也必须允许。构造时不绑定尚未创建的 VehicleLinkManager，不使用 `isInitialConnectComplete`、参数就绪或 fresh-version 门。

离线编辑明确允许本地 XML／通用副本，不要求 PRIMARY；ParameterManager 构造期也不要求 Vehicle 的 `_parameterManager` raw getter 已赋值。离线来源不冒充机载数据。旧 XML 主版本选择仍是原 wanted-major=1 兼容逻辑，不证明真实参数版本；旧 XML 接受域／完整 schema／大小预算不属于本次严格 JSON 验证。

## CRC 的真实消费与未知

`parameterIsVolatile(name,type,bool&)` 是只输出值的已知性查询；XML fallback 可能懒加载并跨虚调用，不是无回调 pure getter。返回 false 时不写输出值；缺定义、类型不符、来源／代次变化、预算耗尽均不得当成已知 `false`。

`_tryCacheHashLoad` 使用本次 hash 的实际 `componentId`，不再固定 AUTOPILOT1。磁盘类型整数先按明确参数数型表核验，再转换 enum；任一未知 volatile／失源退出整次快载，正常参数下载仍可继续。三个 Fact 创建入口分别为 live PARAM_VALUE、offline 编辑参数、FTP 参数文件；都只安装该 actual Fact-owned 的 QQ 副本。

CRC 快径在观察失效通知／provider 创建前先冻结原 Vehicle／实际 Manager、PRIMARY 控制块及实指针、版本 epoch／firmware／vehicle type／default component 和早期关闭门，释放 strong Link 后尾核；上述两个外调返回均先核原来源，不能将收到的旧 hash 重基到新连接。其后才在同原 Manager 取实际 provider 并冻结非 invokable 的 `metadataRevisionMemory()`。目录 reset／替换、失源或释放 strong Link 后尾核失败，均不得继续旧 hash；逐 Fact 装载和原 `_HASH_CHECK` 发出边界也核这组原值。该 revision 是本地目录代次，不是认证；不用会被本次正常参数就绪变化推进的通用 observation epoch 来误退正常快载。

原 `_HASH_CHECK` ACK 改走 Vehicle 私有 friend-only `_sendParameterCacheHashAck`：仍只有原 PARAM_SET／原 hash 消息，不增加请求；将本次 source fence 传到连接虚调用、原插件出站调整之后及实际 `writeBytesThreadSafe` 入口前核对，并拒绝调整后改变原 payload。caller 仍有返回／strong Link 释放后的尾核。这不证明驱动内部完成、字节已离机、远端接纳或当前参数已认证，也不宣称其它通用 sender 已获得同一围栏。

这不是整个 QDataStream 参数缓存的重新设计：原文件预算／全 schema、旧 CRC 内容解释、已经发生的 Fact 插入前缀以及原快载后续流程不因此成为事务或认证。JSON／XML／缓存 CRC 均不能证明远端身份、物理构型、当前参数值、性能、StartPermit、飞行放行或任务执行成功。

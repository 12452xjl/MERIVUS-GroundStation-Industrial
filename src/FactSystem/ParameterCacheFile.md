# ParameterCacheFile — 531 有界 typed v3 缓存 codec

本模块只有纯值 `Row`／`Document`、normalize、encode、decode、valueMatches 和 PX4 CRC 字节贡献接口。不 IO、不构造 QObject／Fact、不碰文件路径或参数写入。实际 ParameterManager 负责原 Vehicle／component／PRIMARY／版本和 metadata 目录代次、完整文件读写及正常下载回退；缓存不是当前硬件身份、签名、新鲜参数、工程证据或执行许可。

## API 和候选值

`Document{vehicleId,componentId,rows}` 的 ID 都须 1..255，行数须 1..16384。`Row{name,factType,value}` 的 name 为 1..16 个可打印非空格 ASCII（0x21..0x7e），拒绝伪参数 `_HASH_CHECK`。所有行必须按名字严格升序且唯一，既不静默排序，也不接受成功前缀。

`normalizeCandidate(name,factType,actual,row,error)` 才负责把真实候选值变为完整规范行。只有小 UINT8／UINT16 额外允许原 PARAM_VALUE C++ 整型提升产生的非负 Int，并先核对应位宽，再转 UInt。Int8／Int16／Int32 只收 Int；UInt32 只收 UInt；UInt64／Int64 只收各自 ULongLong／LongLong；Float 只收 Float，Double 只收 Double。拒 String／Bool／null／未知型、浮整 cast、Double→Float 或 Float 默认 Int(0) 的猜测。signed／unsigned 8、16 位在转换前检查范围，不绕回。

规范 QVariant 与 530 相同：unsigned 8／16／32 用 UInt，signed 8／16／32 用 Int，64-bit 各自原型，REAL32／REAL64 各自 Float／Double。`encode` 只接受已完整 canonical 的行，不替 caller 修复坏类型；`valueMatches` 可为实际小 unsigned 的 Int 提升做同一受限规范化，随后逐 primitive 字节比较，不依赖 QVariant 的跨型数值相等。

这里不沿用离线文本的 finite 接受域。真实 Float／Double 的 IEEE 全部对象位模式都保留，包括 NaN payload／sign、Infinity 和 ±0；不把 unknown 变 0，也不把所有 NaN 规范成一个值。只有实际非 null 元型和原生 IEEE 宽度已核后，才从该真实型的 `constData` 复制对象表示，不按声明 `Fact.type` 盲读 QVariant 存储。normalize 在清输出 Row 前冻结实际值的原表示并核最终副本仍一致，valueMatches 对实数直接比较两边原表示。非 IEEE／宽度不符平台保守拒绝实数；没有宣称跨任意 C++ ABI 的对象存储认证。

## 独立 v3 格式

最大完整文件 `MaximumBytes=4*1024*1024`，不调用 generic QDataStream QVariant／Map 解码。20-byte header：

| 字段 | 字节 |
| --- | --- |
| magic `MVPXC3\r\n` | 8 个固定原始字节 |
| version=3 | LE uint16 |
| flags=0 | LE uint16 |
| vehicleId / componentId | 各 LE uint16 |
| count | LE uint32 |

每行：u8 nameLength、u8 MAV type（1..10）、name 原 ASCII 字节、固定宽 primitive LE 值；无 NUL、padding、独立值长度或任意 QVariant payload。1／2／4／8 字节分别沿 MAV 数值原始位宽；signed 使用明确 two's-complement 文件表示；浮点使用原 IEEE bits，不做字符串或数值重编码。MAV byte 必须先过闭表才生成 Fact 枚举。count 先核上界及剩余最小空间才 reserve，全部行和精确 EOF 通过才交出 Document。

解码后再 encode 整份字节必须与原输入完全一致；如果 Qt／平台浮点复制改变了 NaN 等表示，整份 miss，不交出被偷偷 canonicalize 的成功值。失败清空输出 bytes／Document／Row 或 CRC contribution，固定 error 不回显输入内容。

本 codec 不认识旧 v2，错误 magic 不尝试 fallback 解析；真正 consumer 只查独立 `.v3`，旧 `.v2` 仅 miss、不解析也不删除。写盘原子性、QSaveFile commit 和读回不是本 codec 的职责。

## PX4 CRC 是另一个窄字节域

`px4CrcContribution(row,contribution,error)` 仅允许 INT32／FLOAT：输出 ASCII 名字（无 NUL），后接精确 LE4 primitive 原值。不加 type、字段长度、delimiter 或全文件 header，不能把十类型缓存文件宽度称为 PX4 实际 hash 支持域。

实际 PX4 `param_hash_check` 按 handle 顺序遍历 used 且非 volatile 参数；生成器按 name 排序，原生类型是 INT32／FLOAT，值 4 字节，hash 起始 0，无最终 xor。LE4 域依据本地当前 6C 源码／配置约定：NuttX endian.h 在未设 CONFIG_ENDIAN_BIG 时为 little-endian，v6c defconfig 使用 STM32H743VI 且未设 BIG；不泛称任意 PX4、未来 big-endian 平台或远端硬件已核。真实 consumer 仍须逐行从原 metadata provider 获得已知 volatile 状态；仅非 volatile 行的贡献按同顺序交原 QGC CRC32。codec 不把 caller Map 当已知 volatility，不包含 `_HASH_CHECK` 自身，也不证明远端 CRC／网络来源认证。

静读依据：本仓库 ParameterManager 的旧 CacheMap、PARAM_VALUE 整型提升／Fact 创建、主 CRC 和 debug-miss 的声明型 `constData` 旧用法；本地 PX4 parameters.cpp／parameters_common.cpp、生成器排序与 mavlink_parameters.cpp 的 INT32 memcpy／FLOAT little-endian 实际链。旧盲读及 generic 反序列化须由实际 consumer 一并替换，不能只新增无人使用的安全 codec。

## 状态与工程验证

仅源码编辑与直接静读，未运行 encode／decode／parser、项目、helper、编译或测试。需工程验证 header／count／name／type／宽度／排序／重复／坏尾及全部预算，10 型边界、small unsigned Int 提升与越界拒、±0、quiet／signaling NaN payload、Infinity、decode 再 encode 一字节差异拒；实际 writer 原子失败及旧 v2 miss，metadata unknown／reset／source 变更，CRC 不符回正常下载，以及 debug 不再盲 memcmp。IEEE 表示相等只表示本地值连续性，不证明参数可用于飞行、权限恢复或远端已经接纳 HASH ACK。

# OfflineParameterFile — 530 离线参数整份值 codec

本模块只接借用的 `QByteArray`，生成纯值 `Document`，不打开文件、不构造 QObject／Fact／metadata、不访问 Vehicle 或发送参数。实际 ParameterManager 负责有界读、原 offline 对象和插件／模型身份、候选 Fact、整批发布、失败清理及通知后尾核。codec 的成功既不是文件来源认证，也不是机载参数、工程性能或飞行许可。

## 原生接口

`OfflineParameterFile::decode(bytes, document, error)` 仅在整份成功后交出 `Document{sourceSystemId,rows}`。每 `Row` 包含 `componentId`、`name`、原 `mavType` 整数、对应 `FactMetaData::ValueType_t factType` 及规范化 `QVariant value`。失败清空 Document（sourceSystemId 为 -1、rows 为空），固定诊断带实际行号，不回显文件内的名字、值或注释。

`valueMatches(row, actual)` 只比较该行要求的规范类型和值；还核行内 MAV／Fact 类型一致、整数原始位宽范围、非 null 和有限实数。绝不通过 String／Bool／其它数值元型的转换把值当相同。Float／Double 的零必须 signbit 相同；因此 -0 和 +0 不可互相代替。本函数不证明 Fact、parent、组件、catalog 或实际来源归属，这些仍由真正 consumer 核验。

## 整份格式与预算

`MaximumBytes=4*1024*1024`，文件字节（含可选 BOM）不超过 4 MiB；总实际行最多 65536，数据行最多 16384，每行内容最多 4096 字节（不含 LF／CRLF 终止符），每个数字字段最多 128 字节。首部可有一次 UTF-8 BOM，支持 LF、CRLF、末行无换行、空行和仅 ASCII 空格／tab 的空白行。数据中的独立 CR、NUL、非 ASCII、DEL 或其它控制字符拒绝。

只有原始行首 `#` 为注释，先按不透明字节跳过，仍计行数及行长度预算；不会先对注释做全文件 NUL／UTF-8 拒绝。真实 `Plane3.9.OfflineEditing.params` 的 Git Revision 注释含 NUL，因此保留这条既有资源可达性。` #` 不是注释。只有空白／注释／BOM 或空文件整份拒绝。

每个数据行必须精确五个 tab 列：source-system、component、name、value、MAV-type。source-system 是 0..255 且全份相同的声明值；不要求等于 offline Vehicle 的实际 id 0，也不改变实际 Vehicle 身份。允许多 component，每个是 1..255；codec 不猜默认组件。name 为 1..16 个可打印且非空格 ASCII（0x21..0x7e）。同 `(componentId,name)` 重复，哪怕同类型同值，也整份拒绝，不覆盖、截前缀或按最后一行决定。

## 十种数值类型

| MAV type | Fact type | 规范 QVariant |
| --- | --- | --- |
| 1 UINT8 / 3 UINT16 / 5 UINT32 | Uint8 / Uint16 / Uint32 | UInt |
| 2 INT8 / 4 INT16 / 6 INT32 | Int8 / Int16 / Int32 | Int |
| 7 UINT64 | Uint64 | ULongLong |
| 8 INT64 | Int64 | LongLong |
| 9 REAL32 | Float | Float |
| 10 REAL64 | Double | Double |

MAV 整数 1..10 在转枚举前通过闭表，未知值不落入旧 INT32 fallback。映射沿现 ParameterManager 的两个双向映射；实际 writer 也可导出 64-bit／REAL64，因此本 codec 不只补旧 switch 的七种类型。值不受 metadata min／max、枚举或默认值二次改写。

所有整数用 C 十进制 digit 语法；有符号允许 `[+-]?digits`，无符号允许可选 `+`，拒绝 `-`（包括 -0）。逐 digit 先核准确位宽范围再转换，不发生先窄化或绕回。完整 UINT64_MAX 与 INT64_MIN 有专门不溢出的边界路径，不经 Double 或 JS 安全整数截限。

实数语法为可选符号、带至少一个 digit 的十进制 mantissa（允许 `.5`／`1.`），及可选 `e`／`E` 有符号整数 exponent。拒空白、locale 分隔符、十六进制、NaN／Inf 及类型后缀；用固定 C locale 转换到真实 Float／Double，须成功且有限，非零 mantissa 若下溢成 0 则拒绝。全零 mantissa 保显式正／负零，合法有限零不因很大的 exponent 猜成非零；正常浮点舍入是该数值类型的解析，不声称恢复原十进制词法精度。

## 兼容依据与待工程验证

静读实际资源和 qrc 别名：PX4 `V1.4.OfflineEditing.params` → `PX4.OfflineEditing.params`；APM `Copter3.6`／`Plane3.9`／`Rover3.5` → 现三个 OfflineEditing 别名。原文件来自与 `writeParametersToStream` 相同的五列历史导出格式，不改这四份资源，不把注释中的旧版本当当前硬件证据。实际 Copter／Rover 负 Int8（如 -1、-100）必须按有符号域保留，不能沿旧 `toUInt()` 把它们变 0。

仅直接源码／资源静读，未执行 decode、parser、helper、项目、编译或测试。需工程验证十类最小／最大／越界、数值语法／非有限／非零下溢、浮点 -0 及实际 Fact 安装类型、所有行／字节预算、BOM／LF／CRLF、注释 NUL、空文件／坏末行／重复／跨 source、失败不交成功前缀；consumer 还须验证离线构造早期身份、虚插件／metadata 回调重入、完整安装后单次发布、失败不误称 ready 及绝不触发线上参数写入。

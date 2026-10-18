# Fact：容器输入的实际类型和值

## 531：缓存浮点原值与 NaN 通知尾

实际 PX4 参数 CRC 对 Float 原始四字节计算，不规范化 NaN、Infinity 或正负零。容器入口改为实际 QVariant 元型相同、null 状态相同后，对非 null Float／Double 比较其确切 IEEE binary32／binary64 位模式；其它类型保持 QVariant 比较。只有在实际元型已确认且使用对应原生固定宽度时复制 `constData`，不按 Fact 声明宽度读取任意 QVariant。

原 530 的数值相等尾核仍会把“刚赋值的 NaN”判断为不同，导致后续 `rawValueChanged`／`vehicleUpdated` 被跳过。现在同位模式 NaN 可以通过自己的通知尾；不同 payload、不同零符号或不同实际类型仍会触发更新／使旧尾失效。没有将任意 NaN 当健康值，也不改变 530 离线文本 codec 的有限实数输入政策。

这只作用于 `_containerSetRawValue`，原 `setRawValue`／`forceSetRawValue` 不变，不新增 `_containerRawValueChanged` 或参数写回。平台须支持相应 IEEE 宽度；源内静态断言不是已执行编译或数值验证。531 同样只有源码和限定直接静读，NaN payload、Inf、正负零、通知删除／重入及在线接收仍须工程师验证。参见[参数缓存格式](ParameterCacheFile.md)与[实际加载](ParameterManager.md)。

## 530：离线参数实际消费所需的窄修复

`_containerSetRawValue()` 是容器接收数值的入口，不是操作员参数写入入口。旧 `QVariant` 数值相等判断可能把初始 `Int(0)` 和 `UInt(0)`、`Float(0)` 或 `Float(-0)` 当成无需赋值，留下错误的存储类型或丢失负零符号。

现在只有 QVariant 实际类型相同、值相等，并且浮点零符号相同时才跳过赋值。赋值后的 cooked 转换、`valueChanged` 与 `rawValueChanged` 通知尾部都重新核对实际对象和值；回调中删除 Fact、改变类型或改变零符号时，不继续发出旧值的后续通知。未改变的合法值仍可发出原 `vehicleUpdated`；NaN 不新增相等特例。

这会让“数值相同但存储类型／浮点零符号不同”的容器更新产生值通知，是有意的实际状态修正。该入口不负责替所有调用者检查 Fact 声明类型、范围或数值真实性；离线文件的完整类型检查由独立 codec 和 ParameterManager 完成。`setRawValue()`、`forceSetRawValue()` 及其它入口不在本批改动范围。

不新增 `_containerRawValueChanged`，不产生参数写回、重启消息、机载观察证明或执行许可。也不是跨线程同步或恶意 native 回调的防护承诺。

本批只修改源码并直接静态阅读，未执行 Fact、解析器、工程、编译或测试。工程师需复核类型变化、正负零、通知重入、删除对象及原在线参数接收链。参见[离线加载](ParameterManager.md)、[输入格式](OfflineParameterFile.md)及[工程交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

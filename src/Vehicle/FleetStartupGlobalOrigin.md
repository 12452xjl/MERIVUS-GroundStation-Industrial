# GPS_GLOBAL_ORIGIN 原始观察解码与参考投影

## 443 范围与边界

[FleetStartupGlobalOrigin](FleetStartupGlobalOrigin.h) 是纯 C++ 辅助类，只解码本仓库 MAVLink 定义的 `GPS_GLOBAL_ORIGIN`（消息 49），并从完整原生解码结果构造有界参考投影。

本类不持有 `Vehicle`、链路或账号，不请求消息、不设置消息频率、不发送命令、不读取文件、不判断观察新鲜度。实际来源归属、主链路、接收次序、版本和来源 epoch、观察失效与重复检查属于调用方原生接收账本及启动检查链；本类本身不能证明这些条件。

新增内容不修改 `startupTransitionPolicy` 的 v1/v2/v3，不赋予启动许可，也不改变飞控代码。坐标范围正常、结构合法或消息解码成功，都不是共同原点、PX4 内部有效分支、净空或同一估计器样本的证明。

## 接口

```cpp
static QVariantMap decode(const mavlink_message_t& original, bool* accepted);
static QVariantMap referenceProjection(const QVariantMap& decoded);
```

`original` 必须由接收方提供固件插件调整之前的原始消息。`decode` 一开始将非空 `accepted` 指针设为 `false`；仅合法封装返回解码结果并设置为 `true`。非法封装返回空 map。经纬度超界仍属于可解码的诊断值，返回原值并将 `coordinatesInRange` 设为 `false`，不能将 `accepted` 当成健康判断。

`referenceProjection` 对完整原生 18 字段执行键、类型、固定标记与一致性检查。失败返回空 map；成功返回 8 字段，不附加 `ok`，不计算哈希，不授予权限。严格 map 校验本身也不是来源认证：调用方不能用任意拼接 map 代替真实原生接收证据。

## 原始封装及数值

- 消息编号必须为本地定义的 `49`。
- MAVLink 1 必须匹配其 magic 且 payload 长度恰为 `12`。
- MAVLink 2 必须匹配其 magic 且 payload 长度为 `1..20`。允许协议规定的尾零截断，包括基础字段中的尾零；不能误用 `MIN_LEN=12` 拒绝合法 MAVLink 2 截断帧。
- 将本地 `mavlink_message_t` 全部初始化为零，只复制实际 payload 字节，再以完整 `20` 字节布局调用生成的解码函数。这样普通解码分支和字节交换分支都不会读取原消息有效范围以外的残留字节。
- 纬度、经度和 AMSL 高度保留完整有符号 32 位整数，单位分别为 `degE7`、`degE7` 和 `mm`。纬度范围检查为 `±90°`，经度为 `±180°`；高度不额外夹紧或补默认值。
- `timeUsec` 保留完整无符号 64 位值，使用规范 ASCII 十进制字符串 `0..18446744073709551615`，不转成可能丢精度的 JavaScript number。
- 本类不重新验证传输 CRC 或签名，不检查真实发送者或 primary link。接收封装可解码不等于传输认证。

## decode：精确 18 字段

```text
decodeMapShape, messageId, wireVersion, payloadLength,
latitudeE7, longitudeE7, altitudeAmslMm, timeUsec,
timeUsecExtensionObserved, timeUsecNonzero, coordinatesInRange,
authenticated, signatureVerified, sameOriginProven,
px4BranchValidityProven, sameEstimatorSampleProven,
executionPermit, flightCommandReleased
```

`decodeMapShape` 固定为 `fleet-startup-global-origin-v1`。`messageId`、`wireVersion`、`payloadLength` 和三个有符号坐标字段均为原生 `QVariant Int`；`wireVersion` 为 `1` 或 `2`。投影校验不将字符串、布尔或 JSON double 隐式转换为这些原生整数。

`timeUsecExtensionObserved` 仅在 MAVLink 2 且实际长度大于 `12` 时为 `true`；表示观察到扩展字段的一部分或全部字节，不代表已验证完整时间语义。`timeUsecNonzero` 等于解码后的值是否非零。未观察到扩展时，补零结果为字符串 `"0"`。

三个诊断布尔必须是原生 Bool，并与实际版本、长度、时间和坐标一致。经纬和高度分别按原布局字节偏移 `0/4/8` 核对：未收到的高字节必须为零，整字段未收到时值只能为零，不将缺失高字节符号扩展。部分时间戳扩展的高位也必须符合尾零补齐结果。剩余七个布尔必须是原生 Bool 且全部为 `false`；额外键、缺键、null、错误类型、非规范时间字符串或伪造为 `true` 的权限标记均被投影拒绝。

## referenceProjection：精确 8 字段

```text
referenceMapShape, latitudeE7, longitudeE7, altitudeAmslMm,
timeUsec, timeUsecExtensionObserved, timeUsecNonzero, coordinatesInRange
```

`referenceMapShape` 固定为 `fleet-startup-global-origin-reference-v1`。其余七项保留已校验原生值。无健康摘要、隐式坐标重投影或默认参考位置；超界坐标仍可留作诊断，不能用于实际准入。

参考投影包含完整 `timeUsec`。因此时间值变化也是“已观察参考身份变化”的输入，供原生账本计数；这不声明其一定是重启代数、系统启动时间、同一时钟或同一采样时刻。时间为零、未观察到扩展字节，均不能证明飞控不支持该扩展；也不按数值大小认证其时钟类型。

## 与启动门禁的关系

实际消费者需要同时核验真实 Vehicle/主链路/系统与组件/版本来源、观察年龄的诊断意义与完整接收状态，不能仅比较本类的 8 字段 map；此事件型声明本身不设置周期TTL。即使它与 HOME、LOCAL_POSITION_NED 或 ODOMETRY 的值表面一致，也不能推导共同原点、内部 `z_valid` / `home.valid_alt`、无未观察重置或同一估计器样本。

本类没有最终 `StartPermit`，没有 `canArm` 或自动放行结果。`sameOriginProven`、`px4BranchValidityProven`、`sameEstimatorSampleProven` 始终为 `false`。后续严格准入必须保留这些未证明边界，而不是新增一个声明布尔覆盖未知状态。

## 本次检查及工程验证边界

已静态对照仓库中的 [消息 49 生成定义](../../libs/mavlink/include/mavlink/v2.0/common/mavlink_msg_gps_global_origin.h) 和 [相邻 ODOMETRY 解码实现](FleetStartupOdometry.cc)，并读回本类全部源码，核对精确字段数、原生类型、负坐标、全范围时间字符串、截断扩展和非授权语义。

本次未编译、未运行解码器、未执行测试或脚本、未连接设备。工程验证仍需覆盖合法 MAVLink 1/2、尾零截断、错误长度/magic/msgid、经纬边界与越界、负高度、完整 uint64 时间、类型/标记篡改、实际接收来源失效及发送前检查链；不能以此静态检查替代验证或刷机放行。

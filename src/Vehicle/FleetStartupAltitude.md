# ALTITUDE 同包高度字段解码与状态投影

## 444 范围与限制

[FleetStartupAltitude](FleetStartupAltitude.h) 只对本地 MAVLink 消息 `ALTITUDE`（141）做纯值解码和严格原生投影。它不持有 Vehicle/链路/账号，不请求消息、不设置发送频率、不读写文件、不发送命令、不建立新的启动许可，也不修改 PX4 或 `startupTransitionPolicy` v1/v2/v3。

这是一份“收到的报文字段”诊断，不是 PX4 内部有效位、估计器原点或传感器样本新鲜度的证明。合法 NaN、无穷值及未知哨兵独立保留；不能因为其中一个字段未知而擦掉其余字段，也不能把未知改成零。

## 接口与封装

```cpp
static QVariantMap decode(const mavlink_message_t& original, bool* accepted);
static QVariantMap stateProjection(const QVariantMap& decoded);
```

接收者传入固件插件调整前的原始消息。`decode` 先将非空 `accepted` 设为 `false`；只有匹配消息 141、magic 及长度的封装才返回 decode18 并设置 `true`。MAVLink 1 长度必须恰为 32；MAVLink 2 接受协议尾零截断后的 `1..32`，先将本地 payload 补零至 32 字节。不会把 MAVLink 2 长度小于 `MIN_LEN` 直接当坏包。

本类不重做 CRC、签名或实际 primary-link 归属核验。`accepted` 仅表示上述封装可解码；不代表字段有效、发送者已认证或允许飞行。错误封装返回空 map。

## decode18 精确合同

```text
decodeMapShape, messageId, wireVersion, payloadLength, timeUsec, fields,
reportedAmslMinusLocalMeters, reportedAmslMinusRelativeMeters,
reportedRelativeMinusLocalMeters, sameMessageFields,
sensorSampleFreshnessProven, sameOriginProven, px4BranchValidityProven,
sameEstimatorSampleProven, authenticated, signatureVerified,
executionPermit, flightCommandReleased
```

`decodeMapShape` 固定为 `fleet-startup-altitude-v1`；消息、版本和长度均为原生 Int。`timeUsec` 是完整 uint64 的规范 ASCII 十进制 QString（包括零和最大值），不转成 JavaScript number，不按数量级推断或认证时钟类别。

`sameMessageFields=true`；其余八个证明/授权标记全部为原生 Bool `false`。同一报文不等于同一 uORB 样本、相同估计器时刻或内部有效分支。

`fields` 恰有以下六键。布局与本地 [消息 141 定义](../../libs/mavlink/include/mavlink/v2.0/common/mavlink_msg_altitude.h) 一致；每项恰有 `value / known / state / rawBits` 四键。

| 字段 | payload 字节偏移 | 有限值的未知规则 |
| --- | --- | --- |
| altitudeMonotonicMeters | 8 | 无额外哨兵 |
| altitudeAmslMeters | 12 | 无额外哨兵 |
| altitudeLocalMeters | 16 | 无额外哨兵 |
| altitudeRelativeMeters | 20 | 无额外哨兵 |
| altitudeTerrainMeters | 24 | 小于 -1000 m 为未知哨兵；恰好 -1000 仍是有限已报告值 |
| bottomClearanceMeters | 28 | 小于 0 m 为未知哨兵；零（包括负零）不是负值哨兵 |

- `rawBits` 是原 wire float32 的 uint32 数值，以恰好八位小写十六进制保存，包含符号零与 NaN payload；不是本机字节序字符串。
- 有限 binary32 精确提升为原生 Double，保留负零，不额外夹紧高度。普通有限值为 `state=finite / known=true`。
- 有限哨兵仍保留原 Double，但为 `state=unknown-sentinel / known=false`。
- NaN、正无穷、负无穷分别为 `nan / positive-infinity / negative-infinity`，`known=false`，`value` 为明确原生 `QMetaType::Nullptr`。不把这些数值写入 JSON 数字，也不将 NaN 转换成某个默认高度。
- 先按原始整数位分类，再只对有限位型进行浮点转换，避免读取 signaling NaN 的浮点值改变原始 NaN 位模式。

三个差值只在各自两侧 `known=true` 时计算：AMSL−local、AMSL−relative、relative−local。计算使用原 float32 精确提升的 Double，结果为 Double；否则输出同样的显式 Nullptr。差值仅是同包字段代数诊断，不是已测出的 HOME/原点偏移或新的控制输入。

## stateProjection8 严格校验

```text
stateMapShape, states, known, sameMessageFields,
sensorSampleFreshnessProven, px4BranchValidityProven,
sameOriginProven, sameEstimatorSampleProven
```

`stateMapShape=fleet-startup-altitude-state-v1`；`states` 和 `known` 均恰含上述六个字段名，分别保留通过校验的原状态字符串与 Bool。`sameMessageFields=true`，其余四个证明标记为 `false`。

投影前逐项校验完整 decode18 的键、原生类型、固定值、规范时间和原始位串，并从 rawBits 重建 value/known/state 逐项比较。有限值仅接受 Double；数值和符号零必须相同。未知数值仅接受明确 `QMetaType::Nullptr`，不接受 invalid QVariant、typed-null QObject/自定义指针或依靠 `isNull()` 的宽松转换。三个派生差值也重新计算、核对类型和符号零；缺键或额外键均拒绝。

MAVLink 2 的省略高字节必须与补零一致：短 timestamp 的高位为零；每个 float 原始位串在只收到部分或完全未收到字段时，其未接收高字节为零。不会进行 32/64 位满宽移位。合法尾零截断的零值是 wire 解释，不证明发送方确实拥有健康高度估计。

成功返回 state8，失败返回空 map。这个接口面向本机原生解码对象，不是 JSON 导入兼容入口；将 JSON 数字/空值重新拼成 map 不能替代实际原生接收证据。

state8 故意不包含高度数值、差值、rawBits 或时间，因此连续高度变化或同状态 NaN payload 变化不等于“状态分类变化”。调用方可以将它用于分类计数，但不能称为参考原点代数或同源身份证明。原始值与逐次观察仍在 decode18；实际来源、收到时间、来源失效和接收栈完整性属于 Vehicle 账本。

## 本地 PX4 发送实现的静态依据

以下依据来自当前工作区 [ALTITUDE.hpp](../../../FirmwarePX4/src/modules/mavlink/streams/ALTITUDE.hpp)，不是对任意飞控固件的推断，也未修改该文件。

- 初始化六个浮点字段为 NaN；`vehicle_air_data` 有非零 timestamp 时，将 `baro_alt_meter` 放入 monotonic，发送端没有额外年龄或有限数校验。因此字段名本身不能证明物理严格单调、不漂移或无气压基准变化。
- `vehicle_local_position.copy()` 成功后，在 `z_valid` 分支给 `local=-z`。`z_global` 时 `amsl=-z+ref_alt`；否则 AMSL 回退为前述气压高度。消息没有携带这个分支选择或对应有效位。
- `home.valid_alt` 为真时 `relative=-z+home.z`，否则 `relative=-z`。没有核 `home.valid_lpos`；[HomePosition.cpp](../../../FirmwarePX4/src/modules/commander/HomePosition.cpp) 的全局/本地 HOME 有独立填充分支，`valid_alt` 不能替代 `home.z` 的有效本地基准证明。
- `dist_bottom_valid` 时 `terrain=-z-dist_bottom`，`bottom_clearance=dist_bottom`。这里 terrain 是局部高度减地面距离的表达，不能按通用消息注释直接把它显示成已证明的离地高度。`local-terrain≈bottom` 也只是该发送公式的一致性诊断。
- 同包 AMSL−local 只有在 `z_global` 分支才近似 ref_alt；否则混合气压与局部估计。relative−local 只有 HOME 对应分支才近似 home.z；回退零与真实 home.z 为零不可区分。AMSL−relative 不能直接证明实际 home.alt。float32 舍入也不允许把每次数值微变自动解释为参考重置。
- [Subscription.hpp](../../../FirmwarePX4/platforms/common/uORB/Subscription.hpp) 的 `copy()` 与 `update()` 不同；[uORBManager.cpp](../../../FirmwarePX4/platforms/common/uORB/uORBManager.cpp) 仅在 `only_if_updated` 时要求新一代数据。此发送器采用 `copy`，可能重复旧样本；名为 `lpos_updated` 的局部变量表示复制成功，不等于刚更新。
- 发送条件中的 `10 ms` 只参与气压回退触发，不拒绝已经复制成功的旧 lpos。`time_usec=hrt_absolute_time()` 在发送时写入，不是 `timestamp_sample`、`ref_timestamp` 或 HOME 时间。刚收到报文或时间推进均不证明传感样本新鲜。
- air_data、local_position、home 是各自复制的主题，未携带同一次融合样本的关联保证。内部 `z_valid/z_global/home.valid_alt`、重置计数、ODOMETRY 原点等也没有被消息 141 完整传输。

原 [mavlink_stream.cpp](../../../FirmwarePX4/src/modules/mavlink/mavlink_stream.cpp) 按配置/速率调度发送；[mavlink_main.cpp](../../../FirmwarePX4/src/modules/mavlink/mavlink_main.cpp) 不同模式下可配置 0.5/1/10 Hz。它们不是本机当前实际频率或投递保证；本类不新增消息请求来弥补缺报。

## 静态检查与交接

本次只读回源码并对照本地消息布局、PX4 发送分支和 strict DTO 合同；未编译、未运行解码器、未执行测试或脚本、未连接设备。当前纯层也不能保证全链路实际行为。

后续工程验证应覆盖 MAVLink 1/2 合法长度与尾零截断、错误编号/magic/长度、完整 uint64 时间、六字段各自 NaN/正负无穷/有限哨兵、正负零、float32 极值、rawBits/type/差值篡改、重复旧样本与新发送时间、不同内部参考分支，以及实际主链路失源。不得以静态检查代替测试、飞行许可或刷机放行。

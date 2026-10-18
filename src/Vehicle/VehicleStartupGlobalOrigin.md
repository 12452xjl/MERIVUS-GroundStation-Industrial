# Vehicle：被动 GPS_GLOBAL_ORIGIN 参考账本（443）

本批为地面站源码实现与静态审阅，未编译、未运行测试或仿真、未连接设备、未修改 PX4。它补充本机原始来源与启动参考连续性，不证明飞行安全、坐标共同原点、PX4 分支有效性、同一估计器采样或远端来源认证。

## 来源与生命周期

`Vehicle::_mavlinkMessageReceived` 冻结收到的 `originalMessage` 后，立即为消息 49 建立独立接收深度，早于电池、SYS/EST、遥测/执行观察器和固件插件回调。只承认实际 MAVLinkProtocol 发出的、本 Vehicle SYSID/默认 COMPID、当前 PRIMARY 原控制块成员的原始帧。次链路、其它系统或组件不替换账本；即使外来原点帧仍处于完整接收栈，也不允许从其回调穿透启动发送边界。

入口和提交同时检查实际 Vehicle、Protocol、VehicleLinkManager、MultiVehicleManager、PRIMARY 弱控制块、已观察的版本/flight 来源、PX4 多旋翼及初连完成、本地单调时钟，并在每次 `isConnected/isMockLink/isLogReplay` 外部调用后重新检查 QPointer、epoch、计数与来源。prepare/commit 只读取原始帧，不从显示 Fact、调整后报文或 QML 值补证据。不绑定或抢占现有遥测/执行 observer。

完整 `referenceProjection` 的八字段（包括报告的 `time_usec`、扩展字段是否出现）参与 SHA-256。prepare 在任何 Link 回调之前推进内容变化的 `referenceRevision`。合法且八字段完全相同的重复帧只推进 receipt，不推进参考 revision；A→B→A、坏封套、未提交丢弃、来源失效及嵌套原点接收不能复活旧参考。计数达到 `quint64` 上限即不可用，不回绕。

实际版本或 flight 来源失效也轮换独立 `originSourceEpoch` 并清除观测；断链、PRIMARY 更换、原 Link 销毁、原管理器/协议销毁或 Vehicle 移除同样使来源失效。`everObservedRawFrame` 一旦观察到本机实际 PRIMARY/默认组件的原点帧即保持 true，不能通过失源把“见过但无效”变成“从未见过”。失效函数本身只改本机内存；旧的有界两条 Link hook 以旧 epoch 失效，在下一次 guarded bind 时释放，避免 disconnectNotify 在失效中复活旧状态。

接收深度持续到整次 receive 退出，包括已经完成早期提交之后。嵌套回调期间 getter 不会声称 `available=true`。不存在新消息请求、主动轮询、文件持久化或任何命令发送。

## C++ 只读接口与固定形状

`startupGlobalOriginObservation() const` 非 Q_INVOKABLE；同线程返回固定 24 键，错误线程返回空 map：

| 字段 | 含义/类型 |
| --- | --- |
| `schemaVersion`, `vehicleId`, `componentId` | 原生 Int；schema 为 1 |
| `sourceEpoch`, `flightSourceEpoch`, `originSourceEpoch` | 当前本机版本、flight 和独立原点 UUID |
| `observed`, `everObservedRawFrame`, `available`, `receiveStackQuiescent` | 原生 Bool；静止栈仅指本独立原点接收栈 |
| `referenceRevision`, `receiptSequence` | 完整无符号十进制字符串，避免 JavaScript 精度损失 |
| `referenceHash`, `referenceProjection` | 全八字段的本机摘要与原生 projection8；缺失为空字符串/map |
| `receivedAtMonotonicMs`, `ageMs` | Double 整数，本机单调毫秒；未知为 -1 |
| `source`, `value` | 来源原因字符串、原生 decoder18；缺失 value 为空 map |
| `rawPayloadHash`, `rawPayloadLength` | 原始有效载荷 SHA-256、原生 Int 字节数；未知为空字符串/0 |
| `authenticated`, `signatureVerified`, `executionPermit`, `flightCommandReleased` | 始终 false |

从未见过时：`observed/everObservedRawFrame/available=false`，revision/receipt 为 `"0"`，source 为 `"unknown"`，所有值/摘要为空，时间 -1。身份重建仍轮换 origin epoch，但不会凭空制造 seen 或参考 revision。

曾见失效时：ever 保持 true、revision 推进、observed/available false、清空原始值与摘要、receipt 0，source 保留具体失效原因。封套合法但坐标越界时：observed true，保留原始 decoder/projection/hash，available false。字段未知不使用 0 坐标填补。

`startupGlobalOriginObservationSourceMatches(LinkInterface*) const` 同样非 Q_INVOKABLE；只进行实际原生内存的来源/弱控制块/静止栈核对，不调用 Link 虚方法。正常 getter 也是纯内存连续性检查；它不证明此刻远端仍可达。对 never-seen 可核对当前实际来源，但不把缺失称为有效原点；ever-seen 后失效不能通过此接口取回旧来源。

## 事件来源与兼容性

原点是事件来源，不按 3 秒周期过期。`ageMs` 只供诊断；本机新接收不证明远端新采样。消息内的系统/组件、UID、timestamp 不等于认证、统一时钟或位置重基准许可。

从未收到原点消息时保留既有缺省兼容；其它启动条件并未取消。一旦见过无效/失效原点，则不得当作从未见过继续。

保守边界：若唯一一次原点在初连、版本或 flight 身份锚定完成前收到，不能跨后续身份变化沿用；必须再次实际收到当前来源的原点。此实现不自动请求 `GPS_GLOBAL_ORIGIN`。该边界及实际固件发送时序需要工程师验证，不能靠取消检查宣称部署可用。

## 实际启动发送边界

`sendFleetStartupStep` 在真实 transaction owner/startupId 首次绑定时冻结 stable18：从 native24 仅去除 `value/receiptSequence/receivedAtMonotonicMs/ageMs/rawPayloadHash/rawPayloadLength`。同一次 scope 的 Seq0、Arm、Mode 后续步骤不重新采样这个基线；旧参考失效后不能通过重新调用下一步刷新批准。

真实 transport-current、排空检查、步骤回执来源/捕获尾检查、EST 原始日志边界和每次 `writeBytesThreadSafe` 之前的最终纯原生检查都核对原点基线及独立接收深度。完全相同原点重复帧在接收退出后不因 receipt/年龄改变而破坏基线；内容、epoch、revision、ever/available 或来源变化则拒绝后续发送。

原步骤回执仍为原 33 键，没有把此参考包装为新许可。已调用真实 write 的 `requestIssued` 事实继续保留，即使 write 内同步回调导致原点失效；不伪装撤回、回滚或未发送。

原六个 flight 通道、两个 coordinate 通道及 ODOMETRY、SYS/EST、电池 DTO 不增键、不替换。配套 Readiness/Transition 原生消费者使用实际来源核对及 stable18，仅为参考连续性条件，不将 portable map 当作来源认证或新的飞行许可。

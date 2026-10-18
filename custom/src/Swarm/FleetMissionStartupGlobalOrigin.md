# FleetMissionStartupGlobalOrigin

## 443：被动原点声明的阶段参考

本模块是纯值校验，不读设备、不请求消息、不改参数或任务、不发送命令。实际来源仍由调用者持有的 Vehicle、PRIMARY link 及原生来源匹配器负责；传入 map、UUID、hash、校验成功都不能认证真实来源。

API：

- `capture(nativeObservation, vehicleId, componentId, reference)`：严格校验原生24键及目标 ID，成功输出稳定18键；结构正确的不可用／越界观察可保留，成功不等于匹配。
- `admissible(reference)`：检查该补充观察当前是否符合声明域规则；不是飞行许可。
- `same(reference, current)`：两份均 admissible 且完整稳定18键相等。保留原生单调 referenceRevision 与三个来源 epoch，不按相同坐标或 hash 忽略已观察的 A→B→A。

原始解析复用 [FleetStartupGlobalOrigin](../../../src/Vehicle/FleetStartupGlobalOrigin.md) 的严格 decoded18→projection8；不是重新解释显示 Fact 或外部 JSON。这里只允许有界固定字段、真实数值／bool／字符串，时间和序号使用规范 uint64 十进制字符串，未知不能从字符串、bool 或缺字段转换为零。

## 原生24与稳定18

原生输入固定24键：`schemaVersion/vehicleId/componentId/sourceEpoch/flightSourceEpoch/originSourceEpoch/observed/everObservedRawFrame/available/receiveStackQuiescent/referenceRevision/referenceHash/referenceProjection/receiptSequence/receivedAtMonotonicMs/ageMs/source/value/rawPayloadHash/rawPayloadLength/authenticated/signatureVerified/executionPermit/flightCommandReleased`。

稳定18仅删除 `value/receiptSequence/receivedAtMonotonicMs/ageMs/rawPayloadHash/rawPayloadLength` 六键。其余完整保留，包括 observed、sticky everObservedRawFrame、available、receiveStackQuiescent、referenceRevision、完整 referenceProjection 和 hash。

projection8 为 `referenceMapShape/latitudeE7/longitudeE7/altitudeAmslMm/timeUsec/timeUsecExtensionObserved/timeUsecNonzero/coordinatesInRange`。经纬原始 int32 越界可被 capture 保留作诊断，但 admissible=false；零经纬是合法数值，不默认认作无定位。时间戳零或没有观察到扩展不证明发送端不支持扩展，也不是机载时钟或重启认证。

observed=true 时必须有完整严格 decoded18、投影相等、原始 payload 长度对应、正 receipt/referenceRevision、可用 raw/ref hash 和接收时刻。observed=false 时 value／projection／hash 均为空、receipt为0、payloadLength为0、接收时刻／age均为-1；失效记录的 referenceRevision 可保留已推进值。

admissible 分两支：

1. **确实从未收到**：everObservedRawFrame=false、observed=false、available=false、referenceRevision=0、source=unknown，配合上述完整空值及接收栈静止。兼容原 v1/v2/v3 没有此报文的流程，不把缺报解释为原点有效。
2. **已有实际声明**：everObservedRawFrame=true、observed=true、available=true、receiveStackQuiescent=true、source=live-gps-global-origin、坐标在范围内。实际收到过坏包／失源之后不能退回第一支。

两支都必须由调用者另核实际 Vehicle / PRIMARY 原生来源连续性，缺席不能绕过原有初连、版本与 flight 来源门槛；该来源核对仍不把 absence 认证为有效原点。

GPS_GLOBAL_ORIGIN 是事件声明。当前本地 PX4 实现在首发、明确请求或参考变化时发送；配置流频率不代表每秒重复原点。故没有 age≤3000ms 的周期门槛，稳定快照不含推进中的收包年龄或 receipt。其 timestamp 仍完整参与参考 hash／revision；它不是其它消息的采样时钟。重复相同声明不必使稳定身份改变；原点、参考 timestamp、来源或原生 revision 改变必须重新取得依据。

## 实际消费链

[Readiness](FleetMissionStartReadiness.md) 仅在 `captureForStartup` 增加以下四项，普通 capture 及原六通道／LOCAL+HOME两通道／ODOM结构不改变：

- `startupGlobalOriginReferenceObservation`：稳定18或捕获失败时空 map。
- `startupGlobalOriginReferenceRiskFingerprint`：该稳定18的完整 SHA-256；捕获失败时空字符串。
- `startupGlobalOriginReferenceCaptured`：仅说明严格形状与实际目标 ID 捕获成功。
- `startupGlobalOriginReferenceMatches`：admissible、实际来源及所有先前回调取证后的原生尾核一致。

四项进入原 phase fingerprint；仅补充收紧原 startupPhasePreconditionsMatch/deferredStartupAction，保留其它原因和原许可 false。来源变动时不替换初始快照，保留原观察并令 Matches=false、增加有界原因。

[Transition](FleetMissionStartupTransition.md) 要求原 baseline 的四项完整，比较本次 Readiness、独立实际原生 getter 和所有回调／journal／owner尾核之后的稳定18及 PRIMARY。声明变化、第一次新报文、丢失或已观察 ABA 都使旧参考不匹配，不从 latest 重基；不把缺失的新字段当旧基线已拥有该证据。

[终态再审阅](FleetMissionStartupTransaction.md) 逐机13键保留相同四项，原外层20键和 step33不变。所有回调取证后，重新比对原实际目标的内存观察与 PRIMARY；仅最终仍匹配时发布原始冻结行，不把后取得的参考写回本次来源。GroundReview须消费完整13键并在新 READ 中继续冻结。

## 不证明的事项

不证明 HOME 与 LOCAL/ODOM 共原点、PX4 `z_valid/z_global/home.valid_alt` 当前内部状态、北向、跨消息同样本／时钟、认证、签名、实际轨迹、净空或最终 StartPermit。decoded 的 sameOriginProven、px4BranchValidityProven、sameEstimatorSampleProven、authenticated、signatureVerified、executionPermit、flightCommandReleased 均保持 false；稳定投影没有新增相反的认证声明。

本轮不接 ALTITUDE、不改变既有 flight／coordinate／ODOM 合同及 policy v1/v2/v3、不发 REQUEST_MESSAGE／SET_MESSAGE_INTERVAL；新增原点 native24／stable18 独立合同，终态逐机行变更为上述13键。ALTITUDE 同包高度关系、受信跨流参考、完整净空和最终启动许可仍为后续工作。只做源码与静态读回，未编译、测试、运行 helper／应用／UI、仿真或设备。

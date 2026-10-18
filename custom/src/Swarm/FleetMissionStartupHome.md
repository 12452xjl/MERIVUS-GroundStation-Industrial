# 显式审阅的 HOME 观测过渡数值契约

320–324 的非 QObject 纯值 helper，由实际启动过渡消费者调用。没有文件、SQL、权限、Qt 信号、调用者成功凭据、重放恢复或控制命令。`true` 只表示输入值符合明确的观测政策；不是来源认证、PX4 内部有效性、local origin、同采样、响应因果、净空或最终 StartPermit。

## 精确政策与初始化

`normalizeEnvelope(source, out)` 要求恰好8键，拒绝缺键、未知键、bool/字符串冒充数值、null、NaN/Infinity及隐式默认。规范输出 reference 为 QString、两个数量为 Qt Int、五个限幅为 Qt Double；失败不修改 out。

| 字段 | 规范 |
| --- | --- |
| referenceBasis | original-reviewed-home-observation-not-frame-rebase |
| maximumHomeUpdates | 1–128整数 |
| maximumPreModeRelativeAltitudeChanges | 1–256整数 |
| maxHorizontalDisplacementMeters | 0–1000有限数字 |
| maxAltitudeDisplacementMeters | 0–1000有限数字 |
| maxLocalHorizontalDisplacementMeters | 0–1000有限数字 |
| maxLocalVerticalDisplacementMeters | 0–1000有限数字 |
| maxPreModeRelativeAltitudeChangeMeters | 0–1000有限数字 |

五个0限幅是严格零变化，不是关闭检查。实际零距离可通过；非零指标比较保守向正无穷外包，精确落在非零边界的值可能被保守拒绝，不补米级容差。地理距离仅为 QGeoCoordinate 对编码E7点的球面距离，局部坐标只比较原HOME的native float32值；不换原点、不对齐ODOM、没有传感器/估计器/延迟/地形误差预算。

`initialize(originalGeo3, originalCoordinates9, originalGlobal9, envelope, State*)` 核精确3/9/9字段、整数编码范围、heading已知标记与sentinel一致性；HOME geo子集必须一致。HOME local XYZ须已知、有限且精确可表示为native float32；timeUsecKnown必须true，timeUsec为正uint64规范十进制字符串，不转Double。GP的未知heading不会在本helper变成已知；调用者原schema3 heading/ODOM/地理/高度前置继续独立检查。

成功以全新State初始化原始及expected HOME、原始GP和规范政策，计数只清零一次。失败不改已有State。`originalGeo/originalCoordinates/originalGlobal` 是本次冻结基线，消费者不可覆盖成滚动基线；public State不是可恢复授权或可信序列化证明。

## 原始双HOME事件

`consumeHome(State*, nativeJournalLeaf, ownedStage)` 只接受stage1实际已排队普通ARM或stage2实际已排队MODE的观察上下文；stage0及未知stage拒绝。helper只检查stage数值，实际原生owner、已排队事实和当前来源必须由调用者独立证明。

leaf恰有既有8键：journalSequence、receiptSequence、channel、receivedAtMonotonicMs、rawPayloadHash、riskContentHash、value、source。两计数是正uint64规范字符串，接收时刻是非负且可精确表示的整数；两个hash为小写SHA-256形状，风险hash还须与严格值map的规范JSON一致。原始payload hash仅用于同一原生事件配对，不是签名或认证。

- `homePosition/live-home-position` geo3实际改变时，先保存pending，expected不提前替换；还需紧接的`homeCoordinates/live-home-position-geo-local`完成事件。
- 两叶须同RAW receipt、同原payload hash、同地面接收时刻，journalSequence相邻，ownedStage相同；geo3等于coordinates9的geo子集。
- coordinates-only允许同expected geo的timestamp/local变化；不能凭单坐标叶接受未记账的geo改变。
- 每个完成的coordinates事件只计一次homeUpdates；timestamp必须严格大于上一次已接受HOME更新时间，不跨流对时，不据此证明机载因果。
- 新geo/local限幅始终相对immutable原HOME；每次接受后只推进expected，不刷新预算。RAW receipt须晚于前一已完成HOME事件；native整体顺序/跨step身份仍由调用者核验。

geo暂存的成功返回不代表事件已经完成。每个step结束/发布阶段匹配前必须`complete(state)`为true，不能将半对事件跨step继续。中间非HOME叶也必须由调用者按原生完整journal处理；后坐标叶必须保留严格相邻关系。失败均不发布部分State更改，pending或旧事实不会被失败调用清空。

## ARM阶段relative-only正向分支

`consumeRelative(state, observedGlobal9, previousGlobal9, motionEnvelope, ownedStage)` 仅stage1且没有pending HOME半对时适用。除relativeAltitudeMm外8字段必须与previous完全相同，relative必须实际改变。相对originalGlobal的偏移受maxPreModeRelativeAltitudeChangeMeters约束，绝对幅度同时遵守原motionEnvelope.maxRelativeAltitudeMagnitudeMeters；计数跨所有step累计，不在每个tick/阶段重置。

该helper只读取motionEnvelope中原绝对相对高度限值；完整motion政策及其它地理/AMSL/速度/heading字段仍由实际消费者验证。允许GP先于HOME到达，不把两个异步报文误说成同样本或因果配对。它不放宽pre-MODE的LOCAL XYZ、全球位置/AMSL、heading或速度，也不代替真实ARM ACK/新armed HB/地面POSCTL和整组屏障。

## 消费者必须保留的边界

本helper不持有Vehicle/Link/事务/账号/审批，也不判断原生risk revision。实际消费者必须从固定baseline按全部owned ordered receipts重建，两风险域分别推进，原始叶与两本账核验、raw gap/期限/源epoch、参数/版本/任务内容、实际账号/整组与最终current精确比较保持不变。无新传感器、流请求、飞控更改或自造内部有效位。

本批只写源码、做同行只读审阅和限定文本检查。未调用initialize/consume/helper，未编译、运行测试/应用/API、读取真实日志/设备或刷机。工程师需验证精确schema/zero caps、双叶/单叶/缺叶/错序/不同hash和时刻、uint64极值/推进、跨step预算/ABA、GP先于HOME、relative-only其它字段变化拒绝，以及真实ARM/MODE时序；这些是待验证项，不是已运行证据。

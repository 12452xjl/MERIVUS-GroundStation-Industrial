# 启动估计器证据与连续性（源码实现，未编译或运行）

## 417：全局航线健康门禁要求绝对位置

现有任务 payload 的地理航点使用 `MAV_FRAME_GLOBAL`（WGS84、MSL），因此 `healthyReference` 在原四个健康位／错误位／时钟核对之外，要求 `ESTIMATOR_POS_HORIZ_ABS` 和 `ESTIMATOR_POS_VERT_ABS` 同时存在，并拒绝 `ESTIMATOR_CONST_POS_MODE`。Vehicle 的启动 EST 投影在 `requireHealthy=true` 时使用相同条件，防止只凭相对定位健康通过全局启动门禁。

`referenceValid`、原始 `reference/capture` 和 `requireHealthy=false` 负面叶保留，11字段引用与 RAW schema3/33 回执不变；不修改通用 HealthCenter、不禁用室内功能、不新增硬件 GNSS 要求，也不宣称定位精度或空间净空已证明。相关调用链已限定静态读回，未编译、测试或运行；PX4 源码未修改。

## 381–385：独立 EST 风险域

[值校验器](FleetMissionStartupEstimator.h)只处理调用方从实际 Vehicle 获取的 `fleetHealthTelemetryObservation()`。它不接收设备控制权、不发包、不请求传感器流，也不证明传入 map 的真实性；真实对象、PRIMARY、版本、owner 和接收栈仍由原生生产者及各消费者核对。

`reference` 严格核原生健康 top9、双通道、row9 和 EST value8，以及来源当前、实际年龄和新鲜度；`capture` 另核冻结的实际 vehicle/component ID。输出固定11键：

| 字段 | 类型与含义 |
| --- | --- |
| schemaVersion | Int，固定1 |
| sourceEpoch | 原生 health 来源 UUID |
| contentRevision | EST 实际语义 revision，规范十进制 uint64 字符串 |
| flags | UInt，完整已知 EST flags |
| goodAttitudeEstimate / goodHorizVelEstimate / goodVertVelEstimate / goodHorizPosRelEstimate | 四个与 flags 对应的 Bool |
| gpsGlitch / accelError | 两个与 flags 对应的 Bool |
| clockDiscontinuityObserved | 原生已观察到的时钟回退标志 |

正常增长的 timeUsec、收包 receipt 和 age 不进入该风险身份。UInt flags 与六个 Bool 必须逐位一致；字符串布尔、未知位、错误键集合或非规范 revision 不会被宽松转换。

`referenceValid` 不把合法坏标志视为格式错误；`healthyReference` 才要求四好、无 GPS/加速度错误、无已观察时钟回退。`reference` 的 ok 不等于健康，坏时钟导致原生 row 不可用时也不能凭历史值恢复可用。`advance` 只接受同源、健康、flags 确有改变且实际 revision 严格增加的下一叶；revision 计数与 SYS 共享，不能要求恰好加一。拒绝不改输出，更不能删除原始坏叶或重新取最新好值当基线。

## 收据与消费范围

[启动收据](FleetMissionStartupTransaction.md)显式升级为 schema3/33键，新增 `queueEstimatorObservation`，不把 EST 塞入旧六 flight 通道。原 journal leaf8 形状保持，EST 使用独立 `channel=estimatorStatus` 与 `source=live-estimator-status`，value 就是上述11键。旧 schema2/32键不能声称新连续性覆盖；READ 收据、六通道 flight schema2 和 profile policy1–3 是不同合同，不随之升级。

[Readiness](FleetMissionStartReadiness.md)将稳定11、captured/matches 及其风险指纹纳入原有来源和阶段身份。[Transition](FleetMissionStartupTransition.md)从原冻结基线逐步核 queue、按原有有序 journal 推进 EST、再核真实当前尾部。始终健康的已知 optional flag 改变可以经真实叶推进；四好中任何一项变坏、GPS/加速度错误或时钟回退均停止后续步骤。坏后恢复不能只看最后一包而越过原坏叶。

## 时间和连续性起点

原生健康时效仍用自己的 health 单调时钟。启动间隙另在同一实际 EST 接收入口采样现有 startup 单调时钟，不通过 health age 换算、不互减两个时钟坐标。每个实际 EST 包，包括语义相同包，都推进启动 RAW receipt 和周期时效；仅语义改变追加风险叶。

首次实际 queue 前只复核当前真实新鲜 EST、冻结的语义和来源；**没有为更早的审阅、账号审批至首次 queue 区间建立完整收包间隙历史**。连续间隙覆盖从首次实际 queue 起的初始年龄、每包间隙以及阶段尾段；旧步骤关闭后仍覆盖全组屏障，直到下一实际 queue 或明确 scope 取消。恢复包、新步骤或较新的健康值不得清除旧步骤最大间隙。已解锁再审路径跳过 ARM 时，不制造 ARM 包或收据，Seq0 历史继续覆盖其间隔。

## 工程师交接与未完成范围

本轮仅源码与限定文本审阅；未编译、运行校验器、测试、仿真、认证、启动应用或操作设备。工程师需在目标环境验证普通同值包、健康 optional 位变化、坏好 ABA、断流恢复、queue 前后边界、旧阶段尾间隙、重入、来源替换及已解锁跳 ARM 时序。本文件列的是后续验证要求，不是已通过记录。

这不是估计器内部有效性、机体稳定性、HOME/local 同原点或同采样、净空、最终 StartPermit、正式机载任务结果或无线全程无遗漏的证明。地面站未观察到的错误不在这本被动账本的证明范围内；原25目标/六类路线图及 `outboundIntegrationComplete=false` 保持，不能凭新增证据模块宣称整个产品已完成。

最终尾核另重读最多三份同实际owner/step的原生收据：queue、完整journal和固定身份须与刚重放的一致，允许普通ACK/相同值收包推进，但真实gap/historyDuration仍按原政策重新核对。Vehicle在来源回调后才复制实际记录并计算尾gap，发生嵌套RAW/语义/来源变化即拒当前捕获；sender最后纯检查同时核原owner、deadline及实际已持久间隙。所有回调通过后才封旧段、安装新段并调用write；失败准备不能提前结束旧history，已经调用write的事实也不会被后续失效抹掉。

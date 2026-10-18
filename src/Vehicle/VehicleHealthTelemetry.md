# Vehicle 原生健康遥测证据

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：实际原包年龄，而非定时刷新缓存

本批整体增量为7个源码路径、19份文档；本页记录[Vehicle.h](Vehicle.h)/[Vehicle.cc](Vehicle.cc)的独立健康证据及其现有消费链，不是可单独构建或安装的包，交接仍需累计完整工程。只有源码编辑、限定人工交叉审阅和文本检查，未编译、测试、运行Qt/UI、认证、API/helper、应用或设备，未改飞控、刷机、更新PR或打包。原25目标/六类仍有实质工作，不能按文件或模块数量称完成。

新增非 `Q_INVOKABLE`、非QML属性的原生接口：

```cpp
QVariantMap Vehicle::fleetHealthTelemetryObservation(qint64 maximumAgeMs = 3000) const;
```

这是被动的本机接收观察。接口不请求新流、不写文件、不发控制、不调用OS/授权回调、不发同步public通知，也不从Fact缓存、导入描述或历史快照建立新证据。age上限参数只接受1..120000毫秒；错线程或非法参数返回同形unknown。正常查询不修改来源、不续时间或恢复观测。

## 固定值合同

top恰9键：`schemaVersion`为Qt Int 1；`sourceEpoch`为QString；`vehicleId/componentId`为Int；`sourceCurrent`为Bool；`channels`为Map；`readonly=true`、`authenticated=false`、`flightCommandReleased=false`均为Bool。channels固定 `systemStatus`、`estimatorStatus` 两行。

每行恰9键：`observed/available/fresh`为Bool；`ageMs`为qint64（-1表示未知）；`receiptSequence/contentRevision`为精确十进制QString；`value`为Map；`source`为QString；`clockDiscontinuityObserved`为Bool。source拒绝时不输出旧value或availability，但保留可读取的原生语义计数诊断；这些数字不是许可。

- system value恰4个UInt：`sensorsPresentBits/sensorsEnabledBits/sensorsHealthBits/sensorsUnhealthyBits`。最后一项为原包 `enabled & ~health`，沿既有Vehicle语义。来源为 `live-sys-status`。合法全零mask包照实记录；消费方不能把全零当作传感器健康已知。
- estimator value恰8键：`flags`为UInt；`goodAttitudeEstimate/goodHorizVelEstimate/goodVertVelEstimate/goodHorizPosRelEstimate/gpsGlitch/accelError`为Bool；`timeUsec`为精确十进制QString。来源为 `live-estimator-status`。未知flag位保守失效；没有新增创新比率/精度阈值，也不把ratio解释为新授权。

两行各自保存最后实际原包的receipt、value及本机单调接收时间。receipt来自独立健康账本的原包递增计数，行间可以跳号；SYS到达不更新EST年龄，反之亦然。age是真实经过时间，不被注册表timer或查询重置；available与fresh分开，旧而完整的值可以诊断性存在，但不能因此fresh。

## 实际来源、重入与语义历史

只从实际 `MAVLinkProtocol::messageReceived` 入口，在任何firmware/core plugin与被动observer回调之前冻结并解码原包。要求本GUI线程实际MVM所持Vehicle、原protocol/VehicleLinkManager、实际PRIMARY及其原shared控制块、默认component、当前版本来源，链路连通且非mock/replay。原protocol/MVM/manager的QPointer固定于该Vehicle实例；死亡或替代不从raw字段重建来源。其它configured link上相同system/component的包只忽略，不能替换PRIMARY账本。

Link可以属于工作线程，不加GUI affinity限制。查询实际Link虚getter后逐次复核原QPointer、owner、model、版本/健康epoch、原PRIMARY控制块及计数；不只依赖入站时的连通缓存。全源失效清value/当前证据并换epoch，close/prepareDelete、移出MVM、link/protocol/manager丢失、PRIMARY/通信/model/版本来源变化不沿用旧healthy。

原包先prepare/complete，健康receive-depth一直持有到整个接收函数退栈；其间查询不报当前健康源，嵌套实际健康接收使该轮来源失效，不让旧外层回调覆盖新状态。MAVLink2截零先零填充再解码，MAVLink1要求MIN长度；不以MIN长度误拒合法MAV2零尾。非法包、未知flag、重入或计数耗尽不能继续使用旧健康值；计数不wrap。

`contentRevision`只随mask/flag语义变化、失效或EST时间回退递增，已观察A→B→A仍留下递增历史。正常 `timeUsec`增长仅更新真实receipt/年龄，不使语义指纹每包抖动。回退置本源sticky `clockDiscontinuityObserved=true`，该EST行available/fresh为false，直到来源换epoch；这可能是重排或时钟断裂，不认证重启，也不把该时钟与GPS/ODOM或另一个消息采样时钟等同。

## 已接消费链与仍缺的门禁

[SwarmController](../../custom/src/Swarm/SwarmController.cc)消费严格类型的两行原包及真实年龄，不再以healthy arming report代替EST。定位known还需独立新鲜GLOBAL/LOCAL原包与相应EST位置位，有限显示坐标不够，也不证明同origin/同sample。故障known要求SYS和EST两路都可用，年龄取两者较旧值；可选ESC/FTC/旧Fact/健康检查只能保留坏值或unknown，不能为两路原包续新鲜。尾部再次核原来源/语义身份，不能拼接已变代的依据。

[HealthCenter](../../custom/src/Swarm/FleetHealthCenter.cc)以Registry为基线，旧FTC缓存只能更保守：known取配对交集，明确坏值保留，配对年龄取max；FTC-only或不完整配对不提升许可。现有健康gate由改后的descriptor继续实际消费，而不是仅增加一个展示类。

本批**没有**扩原六flight channels、ODOMETRY契约、startup RAW32收据或profile schema1–3；完整EST启动阶段逐包gap/journal、最终write前后连续依据仍未在本批接完。原生来源观察也不证明CRC/签名/发送者认证、PX4内部有效分支、HOME/local origin、净空、动力学或最终StartPermit。不得把本接口成功或健康状态变绿当作起飞/完成放行；这些剩余代码和工程师真实验证仍须继续。

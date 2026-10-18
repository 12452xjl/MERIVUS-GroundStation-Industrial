# FleetMissionProgressObservation 原生任务状态观测

## 536：当前原生任务缓存不再误标显式NAV朝向

实际普通MissionItem缓存逐项保原param4；共享 [HeadingEncoding](FleetMissionHeadingEncoding.md) 只把有限NAV识别为absolute-degrees，地理NaN保持defaultnull。原值必须是实际float32可表示值，不先窄化再声称观测一致；v3严格归一比较与原v2域由完整payload决定。起降／HOLD有限yaw不新增支持，异常不补零。

这仍只是同源当前任务缓存／遥测观察，不是RAW INT、请求归因、真实yaw达到或正式任务完成。原Run、缓存来源、身份、权限false和全部未知态保持。仅源码及静态阅读，未运行helper／观测／算法、编译、测试或仿真。下方为历史。

## 376–380：快照不补造本次Run的历史结果

[连续Run](FleetMissionExecutionObservation.md)已在actual MODE commit之后记录固定11个原生接收引用，复用同一private helper向current和受保护TaskRecord输出历史候选/观察窗口两轴；MISSION_CURRENT的state/mode/total原值及known、无任务负报告和source退休原因保留。末项、无限HOLD、有序LAND或COMPLETE扩展报告都不是本次机载任务实例/结果认证，不自动Completed或释放占用。

本类源码未在本批改动，仍是短时六通道/缓存对照，不能用它的旧快照填Run receipt、provisional前缀或事后历史。同期[原生SYS/EST健康证据](../../../src/Vehicle/VehicleHealthTelemetry.md)经Swarm/Registry/HealthCenter修正年龄与保守合并，但不扩本快照schema、profile1–3或startup32，不代表完整EST逐阶段gap、最终StartPermit、HOME/local origin/净空已完成。

本批累计增量7源码/19文档；完整25目标/六类及正式远端结果、长期授权/容量生命周期仍待实质开发。仅源码与限定文本审阅，未编译、测试、执行函数/helper/认证/应用/设备、修改飞控、更新PR或打包。以下为历史合同背景。

## 315–319：与连续 Run 的分工

本类继续作为无运行实例的短时缓存/六通道快照，所有原未归属/未完成声明保留。新增[连续运行观测](FleetMissionExecutionObservation.md)从 actual Transaction 和真实 MODE queue 建立独立本机 run、保留有界 post-commit序号/原始状态历史；不会把本类旧快照导入为新事件或正式执行依据。controller 公开独立 `fleetMissionExecutionObservation`，只读检查按同 deploymentId 附 `continuousExecutionObservation`，与此 unbound snapshot 明确分开。连续 Run 仍不认证远端 mission instance，不替代最终许可或正式完成结果；本轮没有编译/测试/API执行。

## 当前 229–233 契约增量

当前捕获使用原生 `Vehicle::flightStateRiskObservation()` v2。完整 raw value/hash/receipt 仍保留并重验；风险投影只从 GP `GLOBAL_POSITION_INT` value 中去掉 `timeBootMs`，在核对 raw hash 与原生 risk hash 后才采用原生 risk hash/revision，并去掉 `ageMs/receiptSequence`。不回绕的 risk revision 保留 A→B→A 变化，不能用相同恢复值伪装稳定来源。机位/高度、航向/速度、armed、任务内容与序号、未知及 fresh 状态仍参与；Raw v1、原 receipt 记录和同源 boot-time 回退撤销保护不变。

最终重捕获比较的是风险身份与实际 MissionManager 缓存，不把相同内容的例行收包当作新风险，也不以收包计数推进证明新到点。`MISSION_ITEM_REACHED` 始终 `attributionStatus=unbound-sequence-observation`、`reachedAttributionVerified=false`；没有这次计划 hash/启动事务归属时仍是 unbound/no-completion。`executionAttributionVerified/missionCompleted/executionVerified/startupPermitGranted/executable/flightCommandReleased` 始终 false，UI/文档不得将此模块称作 execution proof。

该观测可成为实际审阅原生 17 字段 `conditional-human-context` 的风险来源，但不能延长原审阅 30 秒；ack、独立凭据提示和核验不得自动续期。审阅只在真实健康 gate/具体 Leader 风险变化时因 HealthChanged 撤销；其他实际 source/epoch 或内容变化在原生核验时仍拒绝。独立 Startup、本次新鲜机载任务内容/起始序号和真实完成归属仍须后续独立链路，本模块不替代。本轮没有编译、测试或运行任何业务/API。

`capture(Vehicle*, MissionManager*, payload)` 只供 C++ backend 使用，没有 QML 类型注册或可写遥测入口，不请求、上传、改模式、解锁或启动。Deployment 使用自身冻结的实际对象与载荷调用；公开 controller 只能按当前 deploymentId 发起只读检查，不能提供替换的飞机、任务或遥测证据。

## 分开的证据

- 当前实际 MissionManager 不能处于传输/协议 callback 栈。只支持与地面暂存相同的 PX4 多旋翼、无 synthetic-home 的 INT 适配。
- 对有界原始 payload（最多2048项）重新计算完整 canonical wire 列表/hash/schema，核对保存的列表身份。实际缓存逐项独立转换并比对，而不是仅看 ACK、数量或当前索引；这仍是**当前本地缓存**比对，不是本次查询进行了新的机载回读。
- 原生 `Vehicle::flightStateRiskObservation()` v2 中保留的 raw fresh HEARTBEAT、EXTENDED_SYS_STATE、MISSION_CURRENT 和 MISSION_ITEM_REACHED 分别解释。原消息经实际 Link/source/单调年龄检查；稳定风险比较再核 raw/risk 双 hash 和原生 risk revision，旧缓存 `flying/currentIndex` 或高延迟替代数据不能填未知值。
- 当前序号必须在冻结载荷范围内；若 total 已知则必须等于列表长度，明确无任务不能相关。通过后只输出一个 `sourceItemMap` 行，将速度/起飞/出航/返航/终结项还原到原字段和点序号。它不是成功完成项数、距离、耗时、面积覆盖率或百分比。
- 最后重新捕获实际对象、源 epoch/风险 revision/hash、未知/freshness 与缓存内容；相同风险的例行 receipt 推进本身不改变投影，但物理/任务状态、风险 A→B→A 或源/边界改变仍不一致。`sourceCurrent` 仅指这次捕获没有检测到风险来源变化，**不等于所有通道新鲜或任务身份已获机载认证**；另输出 `usableChannelCount`。无原始证据时各 known/observed 标志保持 false。

## 状态和完成边界

新鲜原始 heartbeat 的 PX4 custom-mode 位可报告 AUTO_MISSION 被观测到；armed 与真实 landed-state 单独输出，区分地面未解锁、地面解锁、起飞、降落和空中状态。看到某种模式不证明它由本地这次部署启动，也不证明正在执行该冻结载荷。

MISSION_CURRENT 没有计划 hash/启动事务 token；配套 PX4 的 current/reached 仅携带序号，扩展零表示未知。reached 可周期重复，是“收到这条观测”，不是唯一到点事件。不能根据本地递增 receiptSequence 推断新的到点动作；`attributionStatus=unbound-sequence-observation`、`reachedAttributionVerified=false`，旧任务重复的序号也只能做显示对照。

最后 LOITER_UNLIM 明确输出 `indefinite-hold-does-not-complete`；最后 LAND 只输出任务项被观测到，不能以该项、ACK、地面状态或自动停机单独证明本任务成功完成。所有结果保持 `executionAttributionVerified/missionCompleted/startupPermitGranted/executionVerified/executable/flightCommandReleased=false`。

完整独立 Startup/受控启动、启动授权链、本次新鲜机载任务身份和起始序号绑定与完成归属仍不能由本模块或独立人工启动意图批准代替。捕获结果是短时只读快照；不得恢复为历史审批或执行令牌，不得从 reached/最后项/地面状态推导任务已经完成。

工程师后续需核查 Qt 构建、INT量化/NaN/默认yaw、不同内容同数量、忙传输/对象替换、未知/过期/重复序号、扩展 total/无任务、模式变化、最后 HOLD/LAND 与旧/迟到消息等路径。本轮没有运行函数、编译、测试或传输。

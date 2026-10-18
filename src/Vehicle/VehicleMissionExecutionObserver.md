# VehicleMissionExecutionObserver — 独立的原生任务执行观察入口

## 492：三项live检查保留，调用者逐外调核原上下文

Vehicle在原_commonInit创建VehicleLinkManager后、原版本观察初始化前，一次pin原LinkManager／Protocol／MVM／Toolbox四QPointer；后续sourceAllowed／bind先核这些原QQ，再比现raw身份，不从已失效的遗留raw重建，也不重pin新来源。离线构造未产生Protocol，原offline路径仍拒绝。

Vehicle执行观察lane的_sourceAllowed仍调用isConnected／isMockLink／isLogReplay，不能因修复重入而删去任一语义。新增私有probe忙保护与单调slot变更身份；捕获原QQ Vehicle／Link／LinkManager／Protocol／MVM、原PRIMARY弱控制块、version／组件／observer／epoch等，逐虚调前后仅在原对象和原上下文仍一致时继续。没有把含虚调的BindingMatches叫作pure getter。

bind在live调用前冻结原空槽与QueueContext，connectNotify连接先存本地且每次回调后核原空槽；所有原证据仍一致才整体安装新绑定和写out。失败只断开本地未采纳连接，不迭代已死成员；unbind／retire先清原slot再断开本地连接，不用disconnectNotify后的新slot作旧slot清理。BindingMatches先复制epoch，避免调用者直接传成员QString引用被回调清空。RAW和recheck先固定原observer／epoch，失败也必须在QQ与同原槽仍成立时才退休，不能退休重入新槽。原MODE发送前的多次外调分开检查，每次回来先核QQ与同observer／原上下文，最终write后的commit交付路径保留。

私有代次不回绕、不扩大原生命周期或授予新权限；实际已进入writeBytesThreadSafe的事实不能被后来live失败、slot耗尽或退休撤销。其它Vehicle telemetry／sensor lane不因本节自动宣称已完成同样修复。公开QueueContext和observer抽象接口不扩形，未改飞控或新增消息。见[Run](../../custom/src/Swarm/FleetMissionExecutionObservation.md)、[Txn](../../custom/src/Swarm/FleetMissionStartupTransaction.md)和[未执行交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 376–380：有界post-commit观察引用与健康账本分离

本批整体增量为7个源码路径、19份文档，需累计完整工程交接，不是独立包。原抽象observer和startup RAW32收据不扩形；新增[原生健康两通道](VehicleHealthTelemetry.md)独立于执行观察/原六flight/ODOM/profile，SYS/EST有自己的来源、实际接收年龄和语义ABA，不从此observer的帧数或定时poll推导健康新鲜度。原25目标/六类、最终许可及正式结果仍未完成。

上层实际[Run](../../custom/src/Swarm/FleetMissionExecutionObservation.h)/[实现](../../custom/src/Swarm/FleetMissionExecutionObservation.cc)为固定最多32个目标保留11种post-commit接收引用：missionMode、missionCurrent、missionItemReached、terminalCurrent、terminalAfterMissionMode、terminalReached、terminalReachedAfterCurrent、airborneState、terminalAfterAirborne、landingAfterTerminal、landedAfterTerminal。每个引用仅含原sourceEpoch、sourceSequence和本次绑定本机monotonic接收时间的精确字符串；缺失保持unknown，不能用后到commit回填provisional，也不比较不同车辆的时钟来推导同步。

`observedOutcome`分开历史candidate与当前observationWindow两轴。terminal/无限hold/terminal后落地候选不等于Completed，source结束/显式retire/owner销毁也不抹掉已观察候选。MISSION_CURRENT的total=0、65535及state/mode未知分别显示；合法no-mission、数量不符或越界报告先保留原收到事实，再结束关联。COMPLETE状态和相同item count都不认证本次远端任务实例。

上述引用只在真实MODE write commit之后按原sequence/接收时间形成。任务模式先于terminal、terminal后REACHED，以及非地面→terminal→landing/on-ground分别记录，只称本地接收顺序；没有机载因果、空地状态真实采样同步、净空或降落成功证明。`current()`和原生任务模型使用同一helper，避免分别计算出不同结论。[Orchestrator](../../custom/src/Swarm/SwarmMissionOrchestrator.cc)ownerDestroyed只关闭观察窗口/置unknown尾，保留candidate和引用；迟到的原epoch退休或commit事实只能补原历史，不重开任务或释放新的占用。没有新增自动Completed、STOP/LAND/DISARM或按候选自动释放Run/Swarm的路径。

本批健康消费已从原包年龄接到Swarm/HealthCenter，但完整EST启动阶段逐包gap仍未接完；不是最终StartPermit、CRC/签名/遥测认证或可信HOME/local origin。只有源码及限定人工/文本审阅，未编译/moc/测试/解析器、运行Qt/UI/认证/API/helper/应用/设备或飞行，未改飞控、刷机、更新PR或打包。下方315–319保留为core hook历史合同，不把新增观察摘要称为正式结果。

## 历史315–319：core观察入口

阶段 315–319 新增的非 Q_OBJECT C++ 抽象接口，只接实际 `Vehicle::sendFleetStartupStep` 的 `MissionModeOnly` 原生排队路径和其后原始 MAVLink 接收。它不占用 `VehicleTelemetryObserver` 的单槽，也不修改已有 startup receipt 32 字段或磁盘 schema。

## 排队与事实顺序

不存在 public bind、QML invokable、调用方可反序列化的绑定或快照恢复入口。公开 native 操作仅有 `fleetExecutionBindingMatches(observer, sourceEpoch)` 和 `unbindFleetExecutionObserver(observer, sourceEpoch)`；两者均核当前对象/线程/epoch，解绑不制造任务完成。

实际 `MissionModeOnly` 路径通过最终 authorization fence 和 immutable packet 检查后，向实际 startup owner 取得 `executionObserverForQueue(vehicle, primary, startupId, stepId)`。内部先安装独立观察槽，再以 `QueueContext` 调用 `beginExecutionQueue`；拒绝、对象销毁、source 失效或回调后 fence 失败都禁止此次 write 并退休新槽，不删除观察器的既有历史。

begin 之后的 `current/immutable` 复核还要求实际 owner 的纯 getter 仍返回捕获的同一 observer；因此最终 fence 回调若使执行观察失效（例如 MM operation ABA 或无效 provisional 帧），不能仅凭 core 槽的 source epoch 仍匹配就继续 write。getter 允许“已 begin 但仍 tracking/source-active 且未 commit”的同一排队窗口，不允许重复 begin 或已失效的观察。

`QueueContext` 只含 native `sourceEpoch / startupId / stepId / operationId / wirePayloadHash / vehicleId / componentId`。epoch 为本次内部绑定新建 UUID，source sequence 从零开始，时钟从本次 monotonic clock 启动；没有快照或日志恢复通道。wire hash 由实际不可变消息生成，不是由 caller 上报。

`beginExecutionQueue` 到实际 write 返回之间的 RX 只能作为 provisional 观察；包括 write 回调内嵌套收到的帧，不能提前标成请求因果、ACK 匹配或任务完成。实际 `writeBytesThreadSafe` 调用返回后，先置 `_fleetStartupRequestIssued = true` 并保存旧 receipt 的真实状态，再向捕获的 observer/context 调用 `executionQueueCommitted`，最后才做 public 通知。

若嵌套 write 回调已经退休该绑定，仍向存活的捕获 observer 交付实际 write 事实；如果 Vehicle 已删除，也只用捕获的 `QPointer` 与值类型 context 交付，不解引用旧 this、owner 或 link。commit 是实际排队调用事实，不是串口送达、机载接受、任务启动、结束、请求因果或放行证明。observer 在 commit 回调内销毁 Vehicle 时，后续也先核 `QPointer` 再继续。

## 原始来源与退休

执行观察有自己的 observer/link、epoch、sequence、monotonic clock、busy 和 removing 状态。绑定与帧要求实际 GUI-thread MVM 所有的当前 Vehicle、实际 protocol、PX4、冻结的 default component、当前 PRIMARY、已连接且不是 mock/replay。Link 本身可以属于 UDP/串口工作线程，不额外要求 GUI affinity；其 disconnected/destroyed 观察用 Auto connection 回到 Vehicle 所属线程，并保留旧 epoch 核验。只接受原 protocol signal entry 冻结的原始消息，位置在 firmware/core plugin 改写之前；不接受普通 getter、导入、回放或显示层缓存作为新来源。

sequence 为 native ingress 单调计数，非 MAVLink 8-bit wire sequence，不能当作 RF 丢包精确计数。回调重入、计数耗尽或时钟异常会退休该槽，不 wrap，不从历史恢复。observer 应有界复制/处理帧，不在此回调写文件、请求新数据流或发控制命令。

PRIMARY 任何变更、断链/失联、link/protocol/observer 销毁、移出 MVM、close/prepareDelete、机型或版本来源变化均使旧绑定失效。连接闭包核旧 epoch，不能退休后来新建的槽。版本 invalidation 在当场清槽/epoch，`matches` 立即失败，但使用 observer-context 的零延迟 queued 退休通知，以免给原来无同步回调的旧 version invalidation caller 引入 delete-this 风险。该通知携带旧 epoch/sequence，Vehicle 可能已为 null，消费者必须按 epoch 保留历史并拒绝把旧通知套到新绑定。

Vehicle 析构只清槽和断开连接，不从局部析构状态调用 virtual。执行观察器还须独立监听 Vehicle 的 `QObject::destroyed` 并记录未知尾部；这不应抹掉稍后交付的 captured commit 事实。普通解绑不重新认证、迁移或恢复授权，最终完成判定属于上层执行观察模块，不由 core hook 自动推断。

## 本阶段未做的验证

只有源码编辑、只读交叉审阅和限定文本检查，没有编译、moc、测试、运行 API、接设备、仿真或飞行。工程师后续需验证目标工具链注册、真实排队顺序、begin 拒绝、回调删除 Vehicle/observer、嵌套 RX、source retire 后 commit、异步旧 epoch 退休、PRIMARY/版本切换、计数耗尽和实际帧语义。不能把这些待办当作已运行证据。

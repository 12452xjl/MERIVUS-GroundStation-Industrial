# 普通部署上传与回读的原生发送范围

## 425：完成栈原生成功证据与差异诊断

普通 `newMissionItemsAvailableWithOperationId(op, false)` 没有 success 参数，失败清空缓存后也能发出，不能据此判定真实 COUNT0。PlanManager 新增仅 friend Deployment 可调用的 `fleetDeploymentReadSucceeded`：只在原 `TransactionFinish` 栈、原 closed scope、原实际 owner/Vehicle/primary 控制块及部署/代次/读操作匹配时读取成功事实；要求最终 success、已观察 COUNT 与缓存数量一致、末尾 ACK 已实际本地排队。完成栈排空后清除此事实，旧操作、普通 legacy 读和独立 RAW 读不能借用它。它不是可转移收据、远端认证或当前内容许可。

Deployment 在采集前和后置完整门禁后都重核该证据；失败/未确认保持空诊断，不将清空缓存传入比较器冒充已观察空任务。实际成功回读才生成 [有界 decoded-item 历史差异](FleetMissionReadbackDiff.md)，保留原完整匹配门槛和 StartReview 历史收据。原目标引用不跨可重入门禁；冻结范围、QPointer 项及字段在门禁后复核，变化/取消不能继续使用旧缓存。普通 scoped COUNT 的2048上限不放宽，也不额外派发一次 READ。

新增诊断只进入当前部署显示和压缩的终态历史，不修改发包许可、飞控、任务项或设备。没有把普通解码项升级为独立 RAW INT、原子机载快照或实际执行证据。只改源码并作限定静态读回，未运行任何编译、测试、helper、认证、UI 或设备；工程师须覆盖 success COUNT0 与失败清缓存、实际末 ACK queue、完成栈回调失效、原控制块变化和缓存修改等路径。以下351–355为既有传输范围，不能用旧“无公开显示字段”描述覆盖本批新诊断。

## 351–355 源码契约

本模块为实际 `FleetMissionDeployment` 的普通任务上传与内容回读增加私有原生传输入口，补齐调用方检查不能覆盖的异步逐包、重试及插件回调后发送边界。它不改变独立 [RAW 启动内容读取](FleetMissionStartContentCheck.md) 的来源或凭据语义，也不授予解锁、启动或飞行许可。

本轮实际源码为 [Deployment.h](FleetMissionDeployment.h)、[Deployment.cc](FleetMissionDeployment.cc)、[Episode.cc](FleetMissionDeploymentEpisode.cc)、[PlanManager.h](../../../src/MissionManager/PlanManager.h)、[PlanManager.cc](../../../src/MissionManager/PlanManager.cc)、[Vehicle.h](../../../src/Vehicle/Vehicle.h)、[Vehicle.cc](../../../src/Vehicle/Vehicle.cc)。没有新增 QML API、类型登记、磁盘格式或公开证明字段。

## 实际操作范围，而不是导入描述

- Deployment 在每次上传或回读入口冻结原实际 Vehicle、MissionManager、primary Link、Session、UploadApproval、部署 UUID、代次、目标索引和读写类型；不按相同系统 ID 查找替代对象。Target 单独保留已捕获 primary 的 QPointer 与 sticky captured 标记，丢失不改成从未捕获。
- PlanManager 的私有 `writeFleetDeploymentMissionItems` / `loadFleetDeploymentFromVehicle` 先取得原生 OperationEntry 栈保护，再安装 scope 和调用完整门禁。旧公开 `writeMissionItems` / `loadFromVehicle` 不接收、重建或继承该许可；独立 RAW 读取接口保持原路径。
- 原生 scope 固定实际 owner、Vehicle、Link、primary 弱指针控制块、UUID、代次、读写类型与本地操作号。op0 只允许当前真实 dispatch 中、双方实际操作号仍为零的入口；开始事务先绑定新 opN，再按原信号顺序通知，只有实际 `operationStarted` 已关联目标后才按严格 opN 检查。没有把 target.op0 扩成任意新操作的许可窗口。
- `_fleetDeploymentContextCurrent` 和 Deployment 的 `nativeTransportContextCurrent` 只作纯原生范围检查，不调用 OS、QML 或完整授权 closure。`_liveTransactionAllowed` 只引入这一纯检查，避免给未逐一保护的旧调用栈注入可删除对象的完整回调。
- 完整 fence 在自己的可重入保护内执行，前后复核原实际对象、范围与期限。nested fence 会关闭该 scope，不恢复旧许可。adoption、进度通知、Fact setter 和虚拟 HOME 策略返回后也要核原对象及范围；专用 PX4 路径拒绝插件要求发送 synthetic HOME，旧普通路径保留原策略。

## 逐包与插件后的最后边界

| 操作 | 专用 sender 允许的普通协议消息 |
| --- | --- |
| 新上传 | 原实际非空列表的 `MISSION_COUNT`、当前请求序号的 `MISSION_ITEM_INT` |
| 普通回读 | `MISSION_REQUEST_LIST`、当前待读序号的 `MISSION_REQUEST_INT` / `MISSION_REQUEST`、末尾 `MISSION_ACK(ACCEPTED)` |
| 不在本许可中 | `MISSION_CLEAR_ALL`、`MISSION_SET_CURRENT`、guided item、COMMAND、ARM、模式或启动 |

COUNT、ITEM、REQUEST_LIST、REQUEST_INT 和末尾 ACK 的实际调用以及对应重试都进入同一个专用 sender；不是仅第一次入口检查。scoped 分支在发送前设置正确 ACK 阶段，旧分支保留原时序。

Vehicle sender 冻结实际 manager/owner/primary、参数与版本来源、固件插件、协议对象、channel、本地操作号、ACK 阶段、序号及原列表；完整 fence 与纯原生/实际连接检查在插件前、插件后及最终 queue 前重复核对。原 primary 控制块必须相同；Link 可以合法运行在 worker 线程，不要求其 GUI 线程亲和性。

插件不得改变这次原始编码帧的消息类型、长度、源 ID、magic/flags 或 payload。unsigned 帧保留序列号/校验和处理；原 MAVLink2 SIGNED 帧另固定 seq、checksum 和 signature，不能带着旧签名改签名覆盖内容。只比较实际字段，不比较整个结构体 padding、unsigned 未初始化 signature 或仅用于入站缓存的 ck；不新增签名验证或重签。专用 sendBusy 保护覆盖 fence、插件及 queue，阻止同路径嵌套推进，并进入本地 quiet getter。

这些检查针对**实际本地操作与本次原始编码帧**。没有新增“adoption 后全部 MissionItem 字段都等于原批准载荷”的独立证书；完整内容仍由后续独立匹配检查。普通回读也没有因此升级为 RAW 来源认证、无线请求因果、原子机载快照或任务执行证明。

## 已排队事实、取消与排空

调用实际 `writeBytesThreadSafe` 后，先保存 `requestIssued=true`，再发 `messagesSentChanged`；即使通知中取消、退休或删除 owner，不能把已发生的本地 queue 事实改回未发送。queue 也不等于无线到达、飞控接受或实际执行。`writeMayHaveCommitted` 保持原操作开始时的保守“可能写入”语义，不包装成每包到达凭据。

scope 在失败或完成后先 sticky closed，直到原 OperationEntry、WriteWorker、ProtocolDispatch、Finish、fence 和发送栈都退出才清理。旧调用栈不能因 scope 提早清空而回落通用 legacy sender；新操作也不能继承旧许可。精确取消、撤销、排空与真实不确定结果不依新发送许可删除，不能清空、回滚或重试另一架同 ID 飞机来制造成功。

Deployment destructor / `_stackBusy` 同时检查原实际 Vehicle 的本地 quiet 状态：即使原 MissionManager 已失效，仍活着的 Vehicle sender 栈不被误判为空闲。该检查不加入纯范围检查或持续 full gate，避免 sender 自己使自己失效。Episode 的原固定诊断 schema 只补实际捕获 primary 的来源值；缺失保持 unknown，不认证远端空闲。

普通 read 的 count 从 -1 未知开始，真实 `COUNT(0)` 才是空列表；末尾 ACK 要求实际已观察 count、当前完整列表和阶段匹配。没有 Link、发送被拒或末尾 ACK 未真正 queue，不能 `_finishTransaction(true)`。合法空读即使 ACK 已 queue，仍须由 Deployment 与原期望内容比对，不能因空读协议结束就认为原任务相符。

## 工程师验证与仍未完成项

本机只改源码/文档及做限定文本、人工源码审阅；不编译、测试、执行 helper、认证、应用、SQL、仿真、设备或刷机，不更新 PR 或打包。限定检查结果见[静态审计](../../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)，它不是运行正确性证明。

工程师后续应覆盖：正常/重试上传与回读、真实空读与未知 count、插件改包/取消/嵌套请求、Fact/parent/进度回调失效、op0→opN 信号窗口、同 ID 对象替换、原 primary/插件/版本/参数来源变化、messagesSentChanged 中撤销、sender 中 manager 销毁、所有旧栈退出前无 legacy fallback；并检查独立 RAW、初连读取及旧普通调用未被误接此权限。本机未执行这些验证。

完整 [Lifecycle 应用接线](FleetMissionDeploymentLifecycle.md) 仍需 Guided actual Episode/epoch、共享 Role 身份意图、全部 retained modal 忙锁，以及 selected 旧 Run/Series/Close/Session 的一致投影和界面入口。`outboundIntegrationComplete=false` 保持；最终 StartPermit/净空/正式远端结果、跨站协作和其它原25目标/六类不因本轮完成。

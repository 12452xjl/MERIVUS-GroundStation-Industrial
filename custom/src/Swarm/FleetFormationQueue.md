# FleetFormationQueue — 542 编队等待意图与重新确认

本模块只保存本地编队待办，不保存健康批准、占用租约、远端身份或飞行许可，不发送任何命令。实际消费者仍须经 GuidedActions 的本次人工确认与原 `createTask/startTask/sendStartCommand` 流程；队列不能绕过原健康、任务冲突、准备／提交／释放或真实飞控门。

## 有界状态与真实来源

SwarmController 构造一个队列，原 C++ `bindOrchestrator` 只接受与该实际 Swarm 已配对、同线程的原 Orchestrator，一次绑定后不换源。构造期间不读取尚未完成初始化的 Swarm；绑定排队刷新，不在原来源 setter 内同步发布队列变化。

绑定先冻结原self／Swarm／Orchestrator的QPointer并占住本地入口；每次connect可能触发connectNotify后都重新核原配对／寿命／线程，再安装下一条连接。绑定栈中的同步更新仅请求稍后刷新，不在未完成连接安装时再审阅或消费。

最多32条，包括终态；必须显式移除终态才恢复本队列容量。ID 为新 canonical UUID，逐条 revision 为正 uint64 十进制 QString，比较原字串，不接受舍入数字或前导零；到最大值后闭锁，不回绕。只接受1..6个唯一整值、每个ID也必须是1..6且必须包括Leader 1；六机自然恰为1..6。实际非null Int／UInt／LongLong／ULongLong／Double 整值可用，String／Bool／Float／custom／非有限／小数均拒，规范保存按ID升序的Int列表。保存待办不要求当前空闲，不等于设备已连接、能力已证明或允许起飞。

每个API在读写前同步刷新，另有1秒定时器与原生变化连接；定时器不是正确性的唯一依据。刷新从原 Swarm `continuityOccupancySnapshot` 读取，并用 [FleetNativeOccupancyEvidence](FleetNativeOccupancyEvidence.md) 完整验证；空／无效／未落盘稳定的原生占用不是“空闲”。目标ID存在任何原生 held／uncertain 项即不能审阅；编队 busy／active 阻塞所有待办。原 Orchestrator 的 C++只读 `formationQueueAssignmentSnapshot` 直接从实际 TaskRecord 输出最多255个busy ID，不拷贝大观察／元数据：generic running／paused，以及 native `!observationClosed` 全部保留占用，不把未知远端结果、observer销毁或不busy当已关闭。任何无效投影或实际配对／线程／来源失效闭锁；公开map或导入文件不能替代原生来源。

阻塞原因保留具体目标：`vehicle_native_occupancy_held:<id>`，或 `vehicle_reserved_by_task:<id>:<原taskId>`，后者仅为原投影的首个阻塞任务，不声称已经列出全部owner。新UUID若恰与仍保留条目重复则整笔拒，不覆盖或把别条的回执当本次成功。公开空taskId明确是非null空QString，避免QML把未派发行误认为格式错误。

全队FIFO：仅首个非终态条目可开始重新审阅；其余 `waiting`。终态 `submitted/rejected/cancelled/expired` 不阻后项，`awaiting-confirmation` 与 `dispatching` 阻后项。`ready-to-review` 只表示可进入本次人工重审，绝不是健康／飞行许可。状态全域为 `waiting/ready-to-review/awaiting-confirmation/dispatching/submitted/rejected/cancelled/expired`。

未派发意图从入队起30分钟到期，包括审阅期间；审阅凭据60秒到期，按单调时钟判断，不能用系统时间回拨延长。`createdAt/expiresAt` 只是 UTC ISODateWithMs 字串诊断，不作期限证据。关闭确认窗口不取消整条待办；原队首 awaiting-confirmation 可用准确当前revision重新 `beginReview`，烧旧token并重新取源，也可等待60秒自然回到待重审。

`beginReview` 私有保存当前原生占用 stableIdentity（epoch／revision／fingerprint／scope），不在公开rows泄漏token。`consumeReview` 同步重新取实际来源／任务／占用，必须仍是原队首、原revision／token、原identity且没有冲突，然后**先烧token并进入dispatching、递增revision**，才返回。派发消费者必须使用 consume 返回的revision完成本地结果登记，不能沿begin版本。进入dispatching不形成任何原生预约；原派发链仍需自己的最新门。变化通知可同步重入／销毁，入口独占，结果在通知前冻结，通知后不访成员；源探测后的 QPointer／实际配对尾核保留，不把失败重入当成功。

dispatching 不因30分钟到期、来源丢失、取消按钮或进程窗口关闭而假装释放；若消费者在consume后销毁／异常未finish，该条明确保留 `dispatch_submission_result_unknown` 并阻后项，需人工复核。重启会丢失这个不持久队列，但**不会清除原生任务、连续性日志或真实飞机占用**；本模块没有借重启、移除或取消清飞机占用的API。

## API与闭形状

Q_PROPERTY `rows` 是最多32行的列表，每行exact11：`id/revision/vehicleIds/state/reason/createdAt/expiresAt/canReview/canCancel/canRemove/taskId`。`canReview` 仅ready或awaiting且源当前；`canCancel` 仅非终态且非dispatching；`canRemove` 仅终态。`status` exact5：`count/limit/sourceCurrent/automaticDispatch:false/persistent:false`。getter仅投影，不在QML属性读取中隐式做原生探测。

C++构造 `FleetFormationQueue(SwarmController*,QObject* parent=nullptr)`；C++一次性 `bindOrchestrator(SwarmMissionOrchestrator*)`，不暴露QML换源接口。

Q_INVOKABLE：`enqueue(vehicleIds)`；`beginReview(id,expectedRevision)`；`consumeReview(id,expectedRevision,reviewToken)`；`finishDispatch(id,expectedRevision,taskId,submitted,reason)`；`cancel(id,expectedRevision)`；`remove(id,expectedRevision)`。普通返回exact5 `ok/queueId/revision/vehicleIds/reason`，begin再加 `reviewToken` 为exact6；失败token永远空。失败行仍存在时可返回当前id／revision／ids用于诊断，但不能作为新确认。busy／错线程／通知销毁无可用行时返回空id／revision／ids与false。

finish只接受原dispatching版本；成功submitted须非空taskId，最长128字符，reason最长4096字符且都无NUL。submitted／rejected只记录消费者所报告的**本地派发请求提交／失败**，不是命令ACK、远端执行、任务完成或安全起飞证明。失败前创建的真实taskId可保留以便追溯。cancel只取消未派发意图，不发停止／返航；remove只移除终态意图，不移除 Orchestrator 任务或 Swarm 占用。所有旧revision、旧token、消费两次、重审前旧批准都拒绝。

本批只apply_patch与直接静读，未编译、测试、运行队列／QML／业务helper、仿真或设备。工程应覆盖ID边界／重复／六机、FIFO和终态容量32、ready非许可、generic running／paused与native未closed、所有held／uncertain／journal未知、编队busy／active、review60秒／pending30分钟及时间回拨、源销毁／旧token／替换重审、通知中销毁与重复consume、派发失源／consumer销毁未知保留、finish提交与ACK区分、取消与移除不清真实占用。实际构建、集成、时序和硬件验证仍未开展，不能据静读宣称可靠性已认证或原25目标全部完成。

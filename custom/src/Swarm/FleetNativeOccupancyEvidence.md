# 实际本地占用证据与连续性提交

## 411–415：恢复界面不改变占用证明范围

[恢复面板/Root](FleetOccupancyRecoveryPanel.md)已接实际原生工作流，但九键占用快照与四键稳定身份不变。Swarm持久化诊断现13键，新增currentUncertainVehicleIds来自真实unknown集合，不能从restart-review blocker反推。UI本地解除结果只作历史事实，当前选中待审与新actual身份提交另外报告。

只读currentOccupancyCommitted表示本次getter返回已匹配实际提交，不证明飞机或远端空闲；源变化仍由既有原生getter/最终CAS即时重加hold。当前真实unknown、owner、Run、任务与everObserved不清除。未编译、测试、运行或放行；完整25目标/六类继续，406无UI为历史。

## 406–410 原生恢复当前边界

[恢复复核合同](FleetOccupancyRecovery.md)：专用复核分离restart、restoredHistorical与本轮真实unknown，仅前两类可在在线地面/双账号/整组尾栅后移除；真实unknown、owner/task/quarantine不变。新actual身份须原worker实际commit，换源在getter/CAS即时重新hold；原9键快照/4键身份及本地模型边界不变。本批无新操作UI、远端空闲证明或完整自愈交付；仅源码和限定静态读回，未编译、测试或运行。

## 401–405 当前：重启待审与实际留存门禁

[原生留存](FleetOccupancyJournal.md)现由实际Swarm构造自动接线，不靠QML导入或手动save/load。公开getter仍是9键/entry5/identity4，但它必须等原worker bootstrap、当前完整actual==offered==committed身份及ready/原子替换读回；未提交变化、失源或粘性存储失败返回无效。私有capture用于后台观察，不授提交资格。

每个新Swarm先在独立集合为1–255协议地址加unresolved-outcome待审行；不代表255架飞机实际飞过。旧文件ID仅增加真实unknown，纯重启占位不伪装observed历史；缺/空/旧open或commit都不清这两种未知。当前没有专用授权再审解除入口，因此连续性暂待审，不是重启后已恢复可调度。安全指令不等IO、不新增存储失败否决；所有磁盘IO在worker，原子替换不称断电durable或完整capture。下节396–400无持久化的描述为历史，完整25目标/六类及专用解除流程继续。

## 396–400 当前范围

本模块区分三种不同结论：任务摘要中的本地成员占用、actual Swarm 对象所持本地运行占用，以及真正机载/跨站控制权。新增的是第二种证据及本地角色CAS前的实际来源检查，不是第三种许可。纯值[校验器](FleetNativeOccupancyEvidence.h)只核供入快照一致性，不能认证调用者或飞机；产品必须使用原实际对象和最终原生提交门禁。

## 实际来源与保留未知

Swarm 的 `continuityOccupancySnapshot()` 只读取本实例所持状态，不从Registry健康、任务phase、owner文本相等或 `missionDeploymentOwns=false` 推断空闲。阻断来源包括：

- `deployment-reservation`：原部署owner map；authority被撤销仍保留占用。
- `formation`：原编队成员、阶段及正在派发的栈；包括准备、中止和未结束阶段。
- `command-transaction`：已进入的创建/预抢占栈及pending/running事务，尚未派发也不当作空闲。
- `safety-escape`：实际safety owner map，不能靠关联事务终态先行省略。
- `temporary-mission`、`temporary-mission-stale`：上传、启动、执行、清理以及未排空的清理账。
- `mission-quarantine`：内容隔离；不因本地部署owner释放就恢复启动/接管资格。
- `unresolved-outcome`：已知但未解决的结果不明。中止失败/超时、原对象丢失或已进入发送的指令失败等，不能因为原记录清理而重新出现为“无占用”。

原事务/临时对象在实际创建点保留QPointer和原目标，不对已失活raw指针重新构造观察者，不按相同ID寻找替代对象。已知结果不明保留于本实例有界账本，不新增自动超时清除、健康转好清除或导入清除。这里没有完成可授权的未知解决流程或跨重启持久化；重启/空map不证明远端已停止，后续仍须真实来源、工程审阅与受控流程。

创建/预抢占栈的暂存占用独立于最终事务记录，用有界原生代次与每栈退出清理，避免旧事务被同步回调归档而新事务尚未入表的短窗。它只是当前栈的本地占用，不自动释放其它owner或撤回已发包。

诊断暂存容量/代次/取证失败只能令占用快照与连续性提交失效，不能新增对原Land/RTL/ABORT等安全动作的否决，也不替代原命令自身的门禁。已耗尽的诊断身份不回绕恢复；原安全路径继续不表示连续性仍获批准。

## 限定事务调用边界

本轮边界修复包括9处事务创建调用、直接包围编队发送的4处调用及停止入口、直接编队通知/Idle/暂停链、Goto前置清理与取消同步链，以及临时任务 `writeMissionItems()` / `removeAll()` 返回后的两处尾窗；不是全仓或全部历史回调的生命周期审计。创建前冻结原Controller、原Vehicle的QPointer及数值ID，返回后先查原Controller，再用原目标与实际事务配对；不能在回调后把旧raw指针重新包装，也不能按相同ID取得重连对象继续旧请求。编队内外两层另外冻结原session、phase和command generation，变代后不继续旧阶段或中止新阶段。前置清理须保留已请求的事实，原record消失/替换后不得续清新record。

正常起飞、启动、goto与临时路线保持原整组预检；创建后的初次原目标复核必须在该组任何派发安排之前进行。已经安排或可能发送的事实不因后续失败而回滚成“从未派发”。Land/RTL仍是明确的部分安全逃逸：一个原目标失效只跳过该目标并保留unknown，不新增对其它仍有效原目标的整组否决；整体事务失效、被替代或Controller失活则不得继续旧事务。

Land/RTL结果的 `dispatchedIds` 仅表示本次实际安排了原延迟派发，不是MAVLink已入队、ACK或模式确认。新增 `dispatchOutcomeUnknownVehicleIds` 单独标记回调边界上无法继续归属的目标，不能因未列入本次dispatchedIds推断它此前从未执行；`skippedIds` 同样不授空闲或解除占用。

底层 `CommandTransaction` 也在通知及可通知的私有调用返回后核QPointer和原attempt/status，避免调用者尚未拿回控制权时发送者已继续访问失活对象。旧attempt不能在取消/新attempt后继续写Running、重启timer或发布旧timeout/retry；短作用域transition标志只拒通知栈中的嵌套start/retry，不新增cancel或安全命令的busy否决。cancel仍遵原terminal规则，不能理解为重试准备每个瞬间都可取消。计数不回绕。bool成功保留已接受的本地状态/计数事实，不认证通知后仍Running、已派发、ACK或飞机成功；通知后销毁不撤回已经提交的事实。这不是其它QObject通知或全部旧临时回调的穷尽修复。

## 固定值合同

快照恰9键：`schemaVersion=1`、`swarmEpoch`、`occupancyRevision`、`fingerprint`、`sourceCurrent=true`、`entries`、`scope`、`authorityGranted=false`、`flightCommandReleased=false`。scope固定为 `local-swarm-occupancy-at-read-not-remote-lease`；epoch为每实际Swarm实例的UUID，revision为不经Double的规范正uint64十进制字符串。实际语义变化推进revision、不回绕；错线程、源失活、容量/代次耗尽或不可一致取证返回无效，而不是有效的空entries。

entries至多255行，按数值ID严格递增；每行恰5键：`vehicleId`、`blockers`、`owners`、`authorityRevoked`、`uncertain`。blockers是上述8种代码的非空、有序去重列表；owners是至多16个不含首尾空白、UTF-8可往返、非空且至多64字符的opaque字符串，有序去重，可为空。owner不是任务ID、账号或租约。两个状态位必须是真实bool；uncertain与unresolved-outcome一致，撤权位仅能与部署占用并存。快照没有“free证明”行。

fingerprint为去掉fingerprint自身后的其余8键规范map的compact JSON SHA-256。纯值校验器仅允许schema/ID的等值有限QML数值整数，先规范为Int，再核hash；不接受bool、字符串ID、小数、额外键、重复/乱序ID或冲突状态。字符串列表可跨Qt/QML容器形式，但原内容必须已经有序去重，不容许坏项变空列表。

`stableIdentity()` 只在完整快照有效时输出恰4个字符串：`swarmEpoch`、`occupancyRevision`、`fingerprint`、`scope`。普通时钟、心跳、健康age不在该身份里；实际占用变化和已观察到的反复变化不能只因最终标签相同恢复旧提案。hash与sourceCurrent都是数据一致性字段，不构成数字签名或远端认证。

## 实际消费与提交

[连续性规划器](FleetMissionContinuityPlanner.md)新增严格的 `requireNativeOccupancyEvidence` 和 `nativeOccupancySnapshot` 上下文。任何明确提供的坏快照都使输入无效；产品路径要求实际快照，缺失只hold。旧孤立值规划可保留诊断，但必须标记 `requiresNativeOccupancyReview=true`，不能应用。

候选排除每一个held/unknown条目，不因owner字符串与taskId相等豁免。原任务任一成员有条目时，整任务只hold，不把它标坏后换掉就视为原实际占用已解除。稳定身份进入建议ID和结果；先前的一对一全缺口、角色闭合、同批建议排他与native Run隔离继续保留。

Swarm与[编排器](SwarmMissionOrchestrator.md)以原生双向sticky QPointer绑定同一实际对象。源被销毁后不换一个空Swarm、清pin或回退旧无源路径。`reassignTaskRolesWithOccupancyAtRevision(taskId, ids, revision, leader, roles, expectedOccupancyIdentity)` 在所有本地验证后、单次角色提交前，直接向原源核对冻结身份及old∪new目标当前无held/unknown条目；中间没有OS/QML回调、通知、释放或重新分配实际占用。

旧本地重分配入口在从未绑定源的孤立模型保留兼容；一旦绑定，同样须核实际当前占用，不因调用旧接口跳过。Guided刷新和应用始终携带actual源快照，重审后逐4键比原快照/建议/结果，并调用新6参入口；仍保留明确确认、原角色权限、任务revision和完整角色图检查。结果只表示本地模型已提交，不表示机载改派、飞行完成或自动自愈成功。

## 未完成与工程师后续工作

本轮没有机载任务实例/控制租约、真实故障解除证明、未知状态授权解决界面、跨重启未知留存或最终StartPermit；没有普遍为所有规划器提供真实占用许可，RoleAllocator的静态覆盖仍只是声明约束下的建议。真实受控自愈、性能/能量模型、企业跨站及原25目标/六类实质开发继续。

仅源码与限定文本读回，未编译、测试、解析或执行任何helper/业务/OS认证/应用/SQL/仿真/设备。工程师需后续验证7类来源、撤权但held、预抢占重入暂存、原指针销毁/ID复用、未知保留、revision/代次溢出、同值ABA、双向pin及源死不重绑、provided坏值拒绝、同revision新旧入口、角色完整提案与old∪new末核。以上是待执行清单，不是通过记录或放飞许可。

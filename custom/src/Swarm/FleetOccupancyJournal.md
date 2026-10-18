# 原生占用留存与重启待审

## 411–415 当前：恢复面板接线与13键诊断

[恢复界面/Root合同](FleetOccupancyRecoveryPanel.md)已接实际Swarm唯一原生工作流，9键占用快照与11键磁盘文件不变。continuityOccupancyPersistence由12升13键，新增currentUncertainVehicleIds为实际本轮unknown有序地址列表；restart/history/currentunknown三集合明确分开，不把1–255协议地址holds当实际飞机数。

原生recoveryWorkflowImplemented仅表示对象存在；recoveryUiIntegrated仍false。Root另报uiWiringPresent=true、runtimeValidated=false，只说明源码已登记，不声称Qt运行/持久化/物理处置验收。UI本地解除历史事实、selectedReviewRequiredVehicleIds和实际新提交分别显示；persistenceConfirmed在原生审批返回中仍false，不乐观补真。

取消、日志失败或关闭不回滚已发生的本地解除，不清真实unknown、everObserved或owner/任务；下一Swarm仍重新待审。离线/报废及真实unknown解决仍未实现。未编译、测试或运行，原25目标/六类继续。下方406“无UI/12键诊断”为历史。

## 406–410 原生恢复当前边界

[恢复复核合同](FleetOccupancyRecovery.md)：bootstrap历史ID现进独立restoredHistorical集合，不伪装本轮真实unknown。原生专用双账号复核可解除选中restart/history，everObserved与11键文件不变，新actual必须真实commit，下一Swarm仍全部待审。诊断12键区分native入口存在与UI未接线；下方401–405“无任何再审入口”为历史，不覆盖本节。本批无新操作UI、远端空闲证明或完整自愈交付；仅源码和限定静态读回，未编译、测试或运行。

## 401–405 当前范围与临时限制

本轮将[纯值Codec](FleetOccupancyJournalCodec.h)与[原生后台worker](FleetOccupancyJournalWorker.h)接到实际 `SwarmController` 构造、原生定时捕获和最终连续性门禁。不是无人消费的save/load工具，也不借默认关闭的诊断SQL或遥测归档充当占用账本。

每一个新Swarm实例在任何公开占用投影之前，先为协议地址1–255建立独立 `restartReview` 集合。它代表“本次尚未完成恢复再审”，不是255架飞机实际飞过、故障或曾被本地占用。旧文件为空、缺失、`runOpen=true`、一次成功写入、对象析构或正常退出，都不能移除该集合。

当前没有专用授权再审/解除入口，公开只读状态明确 `recoveryWorkflowImplemented=false`。因此本阶段连续性改派保持待再审限制；不能称重启后自动恢复可调度、受控自愈已经落地，或把这个临时限制当作完整产品终态。专用purpose、明确现实处置依据、原账本/本机状态绑定和受控解除仍须继续开发。

## 实际接线与来源

构造使用实际 `QStandardPaths::AppDataLocation`，不接QML传入的路径；worker只处理固定 `fleet-os/native-occupancy-v1.json` 和 `.native-occupancy-v1.lock`。没有公开文件加载、导入、初始化为free、清除、resolve或路径setter。新类型仅在 `custom.pri` 登记，不建立第二个QML Swarm来源。

worker为parentless QThread，磁盘/目录/合作锁操作仅在 `run()`。GUI的offer/status/bootstrap-pop/stop只持短mutex，不在mutex内执行文件IO；等待条件会释放mutex。Swarm析构只请求协作停止，worker finished后才deleteLater；不wait、不terminate、不写clean、不恢复或释放飞机占用。停止不保证中断正在阻塞的系统调用，也不是实时退出期限。

每次实际占用代次变化只标dirty，不在 `_bumpContinuityOccupancyRevision()` 内读快照或落盘，因为它通常发生在原map修改之前。100ms原生timer在后续GUI栈捕获当前实际状态，提交不可变IDs与完整identity4。仅实际blocker或真实已知unknown计入单调 `everObserved`；纯restart占位不伪装历史飞机记录。过快建立又消失的状态可能未被异步捕获，因此不称完整覆盖或write-ahead。

## 固定磁盘合同

文档恰11键，最大16,384字节；unknown键、坏类型、乱序/重复ID、非规范counter和不匹配摘要全部拒绝。

| 字段 | 合同 |
| --- | --- |
| schemaVersion | 数值整数1，规范为Int，不接受bool/字符串/小数 |
| format | `merivus-native-occupancy-journal` |
| ledgerEpoch | 规范非零UUID字符串，已有账本跨writer保留 |
| ledgerRevision | 规范正uint64十进制字符串，不经Double、不回绕 |
| writerEpoch | 本次实际Swarm epoch，非账号或控制租约 |
| runOpen | 真实bool true，不能写clean |
| coverageComplete | 真实bool false |
| everObservedVehicleIds | 至多255个数值整数ID，1–255，严格递增唯一，可空 |
| lastObservationIdentity | 空map仅作历史初值，或恰4个严格字符串的native identity |
| recoveryReviewRequired | 真实bool true |
| fingerprint | 去自身后的其余10键规范compact JSON SHA-256，小写64hex |

identity恰 `{swarmEpoch, occupancyRevision, fingerprint, scope}`，scope仍为 `local-swarm-occupancy-at-read-not-remote-lease`。JSON/QML等值schema/ID整数先规范为Int再核摘要。列表不静默排序或去重，失败清输出，输入输出同对象不会先丢输入。数据不含位置、账号凭据、活owner、QObject、任务内容、审批或可恢复权限。

`lastObservationIdentity` 可以来自上一个writer；磁盘已有identity绝不直接成为当前Swarm的committed身份。hash只是内容一致性，不认证文件、硬件、账号、时间、全集或防回滚。

## 后台写入、并发与错误

启动先检查已有文件。坏schema、超长、截断、非普通/链接目标或基线冲突不删除、不重建、不空化。确实缺失时可建立新的quarantined档，初始空observed列表和空历史identity仍必须open/coverage=false/review=true；Swarm的255个独立待审holds仍保留，不会因此产生free证明。

固定合作锁贯穿整个worker run。文件和目录通过原生身份、大小、内容字节/hash及路径重开核对，拒已观察到的非预期替换；写temp前与commit紧前都核原baseline或仍Missing。使用QSaveFile且关闭direct-write fallback；commit后重新读取完整内容并核Codec，绑定替换后的真实新FileID，而不是强求沿用旧ID。

单槽offer可合并请求：全部已接受ID都并入有界单调union，包括in-flight旧批和更晚pending；新identity/request只标记下一次写，旧commit不清掉新pending。只有该批实际原子替换及读回后，才更新它自己的committedRequestId/committedIdentity。代次、容量、IO、源身份或基线错误粘性失败，不自动换一个空worker继续。

这不是rename时刻的原子file-CAS、恶意进程隔离或签名。合作锁只约束同路径约定的参与者，Qt仍可判断stale PID锁；未实现私有目录ACL部署、恶意TOCTOU、所有文件系统身份唯一性或防回滚证明。[Qt 5.15.2锁实现](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/io/qlockfile.cpp)不是企业控制权服务。

`atomicReplacementConfirmed` 仅记录当前原子替换/读回事实；`osSyncVerified=false`、`durableVerified=false`始终保持。Qt 5.15.2的commit不报告其syncToDisk调用的失败，不能据此认证断电后仍持久；参见[对应Qt源码](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/io/qsavefile.cpp)。完整捕获、平台flush/目录同步策略及失电验证仍未实现。

## Swarm恢复与最终门禁

bootstrap收据一次取用，固定原worker QObject、workerRunId、newWriterEpoch与单调generation。旧observed IDs只union到当前原生真实未知及历史观察集合；不重建部署owner、编队、事务、Run、Session、批准或租约，也不清当前真实unknown或独立restart holds。

私有capture只读当前原容器；公开 `continuityOccupancySnapshot()` 还必须满足：原worker仍活、bootstrap已接受、状态ready/无错误、原子提交事实存在，且当前完整实际identity == 本机最后offered identity == 该原worker实际committed identity。worker代次回退、失源、错writer、未提交新变化或存储错误均返回无效map；不能把旧ACK当新变化已写入。身份是本机at-read关系，不是持续磁盘认证或与文件系统/远端控制原子提交。

最终六参角色CAS仍通过同一私有Swarm admission读取这份当前证据并检查old∪new。存储门禁不进入原命令staging、普通预检或Land/RTL/ABORT路径；安全动作不等磁盘、不因账本错误新增否决。未落盘尾部或原记录清理不能借重启变成free，255个重启待审holds仍阻止连续性接管。

`continuityOccupancyPersistence` 只有10键只读诊断：nativeSwarmEpoch、worker、bootstrapAccepted、stickyFailure、errorCode、restartReviewVehicleIds、recoveryWorkflowImplemented、statusScope、authorityRestored、flightCommandReleased。statusScope明确last-poll，不把缓存状态当实时许可；后两项始终false。没有新的界面控件或自动解除操作。

## 工程师后续验证与完整范围

后续需实际验证：初用目录/缺文件、空历史与旧open、坏/截断/超限档不覆盖、单槽pending+inflight并发union、bootstrap只导旧ID、旧ACK不认新identity、原QObject/workerRun/writerEpoch丢失或代次回退、两进程合作锁、目录/文件变更、commit前基线变化、写后新FileID、写失败粘性、退出排空/已发生替换事实及所有平台Qt值往返。也须验证磁盘故障不影响原安全动作。上述未在本机执行。

本轮7源码/工程登记、15文档，共22路径；仅源码与限定文本读回，没有编译、测试、运行Codec/worker/文件系统/认证/应用/SQL/仿真/设备，没有改PX4、刷机、更新PR或打包。原25目标/六类继续：专用恢复再审与现实处置依据、正式受控执行/StartPermit/远端结果、可信性能来源、自愈执行、企业跨站、长期运营以及真实模型/生态均未因该账本而完成。

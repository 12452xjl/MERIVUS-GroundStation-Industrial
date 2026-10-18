# ParameterImportTransaction — 534 原生固定值导入事务

本批接 533 实际参数文件预览，不再借 Fact 普通 setter 乐观修改现场值。只保存本进程单槽原生 Run；不是飞行任务 Run、参数 ACK 协议、远端构型认证或持久归档。旧流导入、普通控件写入、参数读取、v3 缓存、530 离线初始化的接受域不由本事务替代。

## 原生接纳与公开状态

`beginParameterImport(const QVector<OfflineParameterFile::Row>& rows, LinkInterface* expectedPrimary, const WeakLinkInterfacePtr& expectedPrimaryOwner, std::function<bool()> admissionFence)` 非 QML invokable。输入是实际预览完整选中的 typed 原候选，不接受 caller QVariantMap。Controller 的 fence 只在同步接纳期间复核 533 原 token/来源/选择行基线；接受之后不持有它，关闭/销毁预览不自动取消已接受 Run。

接纳 EXACT8：`accepted,runId,state,reason,rowCount,enteredRowCount,writeEntryCount,active`。计数为 Int，`writeEntryCount` 为 canonical unsigned decimal QString。拒绝总是空 runId、0 计数、active=false，不拿旧 Run 冒充本次。accepted=true 只是本次事务已接纳；原 source 随后退休/取消不把接纳或既有 writer-entry 事实改写为未发生。

`parameterImportState` 只读 Q_PROPERTY、`parameterImportChanged()` 通知；`parameterImportStatus(runId)` 只返回精确原 Run，错 ID 返回空 Map。状态 EXACT21：

`schemaVersion,runId,state,reason,rowCount,pendingCount,enteredRowCount,writeEntryCount,matchingReportCount,supersededCount,timedOutCount,active,sourceCurrent,cancelled,partialSubmission,snapshotOnly,reportMatchingOnly,authenticated,configurationPermitted,executionPermit,flightCommandReleased`。

schemaVersion=1；runId 为本次 UUID（idle 为空）；所有行计数为有界 Int，writeEntryCount 为原生实际 ledger 汇总的 unsigned decimal String。snapshotOnly/reportMatchingOnly 固定 true；authenticated/configurationPermitted/executionPermit/flightCommandReleased 固定 false。partialSubmission 表示已有 writer entry 而并非所有行都有匹配报告，不能读成远端已部分保存。sourceCurrent 仅当前原生来源内存一致性提示，不是值仍相同或权限。

状态包括 `idle,queued,sending,waiting,reports-matched,superseded,finished-with-unmatched-rows,cancelled,source-retired,timed-out`；reason 保留实际固定原因。`cancelParameterImport(runId)` 只撤精确原槽，错误 ID 不操作；同一已终结 Run 的取消幂等返回 true，不重写终结事实、不取消后来 Run。状态通知通过 GUI queued 合并发布，未发布之前直接读取仍是实际原生状态；不把通知/界面更新当 wire 事实。

## 完整预检和来源

最多 16384 个非空唯一 `(componentId,name)` 候选，复用 OfflineParameterFile 的严格 canonical 数型和值域检查。仅支持实际 PARAM_SET 七型 MAV 1..6、9；拒 7/8/10、非法组件/名称、保留 `_HASH_CHECK`、非有限 Float、越界或重复。任何原普通 pending write 都先整批拒绝；没有迁移/清空旧等待表。

原实际 Vehicle/PM/ComponentInformationManager、Vehicle 早期 QPointer pins 的 plugin/links/protocol、原 PRIMARY 对象及 weak owner、原系统/默认组件/固件/机型、版本来源 epoch 和实际七版本值均独立冻结。不借会被正常回包/本次写入推进的 cache activity serial 或 PM observation epoch。PRIMARY 可以在 UDP/Mock Link 自己线程，不能错误要求 Link QObject affinity==GUI；调用者、Vehicle、PM、plugin、协议及 manager 保持实际拥有线程。

源同时保持原 GCS system/component、实际 link channel；Link connected/replay/channel 虚调用前后复核准确原 source。两个版本监听就位后的变化 sticky 退休；不声称第一条 connectNotify 内另一尚未监听信号隐藏 ABA、无信号原生 setter ABA、跨线程修改或实物身份的证明。

每行已知 Fact 保实际 parent/thread/name/type、原 owned metadata、raw 范围/只读/枚举/单位/翻译器等快照以及实际 provider 目录 revision。整批接纳尾全部再核。首次进入 writer 前仍要求原值基线；不创建替身现场 Fact、不调用 setRawValue/forceSetRawValue/_containerSetRawValue 改 live 值。原本缺失参数仍可显式导入；若后来先出现不匹配的新 Fact，不把它领为旧基线继续重试。精确同包匹配报告仍可在正常 live 处理后结束该行。

APM 的实际 per-component 编码模式由 Vehicle private bridge 查询；数值 float 模式整数必须 `double(float(original))==original`，整批先核可精确表示，不能把越界 float 窄 cast 回整数、默默舍入后才发现或退回 bytewise 假兼容。发送逻辑 union 与 expected wire 均完整零初始化，窄型不用未定义高字节。

## 独立等待、固定重试与实际 writer

独立 native QTimer 100ms；每轮最多扫描 256 行、最多尝试 64 行，重试间隔从原调用返回后算起至少 3s，慢虚调不能令下一次重试立刻发生。首次后最多沿原 `_maxReadWriteRetry`（5）次重试，原 disableAllRetries 保持生效。单 Run monotonic 总预算 120s，在驱动、实际 bridge fence、最早 RAW 取票和消费均核；Qt/驱动函数不是物理可抢占，预算只在实际可检查边界执行。

每次重试重建同一已冻结 logical 参数与 expected wire，不读当前 Fact.rawValue 重发。Vehicle private `_sendParameterImport` 在实际连接/plugin/模式外调后核固定 tuple/typed bytes 和原 sourceFence；最后 plain ledger `entered++` 后立即调用原 Link writer。每行 shared ledger 跨重试/取消/销毁保留事实：bridge 返回 true 表示已进入该本地 writer，非传达或机载接纳；晚取消不能把 entered 改回 0。

不向 `_waitingWriteParamNameMap` 登记导入，不用其“任意同名回包 remove”语义，不让普通 timeout 从当前 Fact 值重发本事务。pendingWrites、原生观察 pending 判定及进度聚合包含本事务实际等待数；原 index/name 读取与 timer、公平重试不因导入被全停/清空。普通同键写入只把该 Run 原 pending 行标为 superseded，然后继续原普通写路径；不同键普通写不被导入占用。显式 refreshParameter/refreshAll（包括非默认组件）、reset/旧 file import/FTP 重载和真实源变化退休本事务，不拿正常回包/普通 read timeout 当本次自动取消。

## RAW 匹配而不是 ACK

Vehicle 在原始 message 冻结后、最早任何通知/probe/plugin 之前，从当时实际原 PM 捕 native-only opaque `ParameterImportReceipt`。取票纯内存、无 Link virtual/weak lock/emit：须同原 Run/PRIMARY/sysid/compid/name/type、该行已经 ledger.entered>0，以及原始 PARAM_VALUE 值按已冻结 bytewise/numeric 模式准确相同。Float 保 ±0，窄整数只解其声明的实际值，不把未使用高位当有效值。时间来自本 Run monotonic clock，非远端测量时刻。

晚 PM 入口只消费该原票据，不能重新从 latest Run 造票；无票、次要源、回放/helper/cache、未 entered 或改值/改型回包不信用。先走原 live Fact 更新，再消费匹配票，避免把本来仍 pending-at-receipt 的包提前洗成无写入的观测；消费还核实际 per-component 编码模式未从原 bytewise/numeric 解释改变。中途取消/supersede/换源/新 Run/到期均不让旧票清新等待。驱动 while 条件里的 Link probe 同样可取消，退出尾不覆盖其既有终态。

MAVLink PARAM_VALUE 无本事务 nonce：在新 writer 已 entered 之后才到达的旧同源同值包，无法证明其远端生成时间或因果。这里只记“入场时已有 entered 的 Run 收到同源同类型同值 RAW 报告”，绝不称 ACK、配置已验证、flash 已保存、认证设备身份或正式成功；固定 assurance flags 始终 false，不恢复任何 StartPermit/审批/飞行权限。

## 退休与未执行交接

取消/源失效/冲突/超期只移除本 Run 自有 pending，保原行 ledger 和匹配报告，不回滚他人普通写、read waits、已进入 writer 或真实 live 值。容量为单 Run，终态保留至下一次完整接纳；应用退出仅停本 timer/退休原 Run，不后台发送或等待线程。

全部仅源码编辑/直接静读，未运行 parser/helper、业务 IO、QML、编译或测试。需后续工程验证：七型全量预检与 APM 大整数零写拒、UDP 不同线程、逐虚调换源/取消、plugin 改 payload、同步 RAW 在 writer 前/后、旧同名不同值/旧回调中新 Run、所有 retry 固定值、同键普通写精确 supersede、非默认 refresh、部分 entered 后取消/销毁、120s 取票与消费两端、pending/progress 与普通 read 并行、晚 ledger 事实及界面准确 Run 取消。

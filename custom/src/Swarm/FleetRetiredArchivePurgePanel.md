# Retired 原件独立永久删除面板

## 525 实际入口与范围

[FleetRetiredArchivePurgePanel.qml](../../res/Merivus/FleetRetiredArchivePurgePanel.qml) 默认收起，嵌入现 [FleetArchiveMaintenancePanel](FleetArchiveMaintenancePanel.md)，沿同一真实 `archive.retiredPurge` 窄 facade 取 `workspace`。两个原历史宿主不另接一套 controller／原生服务。`sourceActive` 来自实际父维护区的 `visible && enabled`，再与本组件的可见、启用及展开状态共同决定本页是否可操作；隐藏、失源、换 facade 或销毁退休本页旧资格。

本批是一个 **retired 原件的永久删除**，不是旧 `sealed ↔ retired` 可恢复移动，不是自动留存策略、全目录清理或物理磁盘空间释放认证。旧移动／备份按钮保持原合同，不永久删除。新流程必须先在旧维护流程选择 retired 原件，新做外部单文件备份，等待 `backupVerified` 和实际 Worker 结束双尾；只有原生实际完成路径才建立私有 basis。库存、目录诊断、共享公开 Map、已保存 JSON、同文件名或任意路径不能重建该依据。

可见流程：新备份及读回 → 明确请求只读删除前检查 → 阅读原件／保留副本全路径、UUID、SHA256和两份不同文件ID → 分别确认备份风险与永久删除风险 → 明确提交 → 等同原 job 双尾并逐项核对事实。展开、呈现、确认、隐藏不自动发 preview／commit；隐藏只可能精确取消本页原 job／作废本页准确旧预览。没有自动重试、自动恢复、删除 current／sealed／副本／partial、批量 purge、任务或账号恢复。

只支持 Windows 原生精确删除；其他平台显示 `unsupported` 并拒绝预览／提交，不把 unsupported 解释为没有备份。恢复须沿原备份诊断 → 导入 retired → 独立 restore 链，不能撤销已发生删除，也不能恢复旧飞行、任务或批准。

## Facade 与闭合值合同

仅消费 `workspace.state / preview / result / request(intent, true)`，不暴露 Store、Worker、文件系统或 native proof。intent 恰五个非 null String：`action / expectedBasisId / expectedJobId / previewId / expectedRunId`；动作只有 `preview / commit / discard / cancel`。所有动作冻结实际 facade、原 run／basis／job；preview 的 previewId 为空，commit／discard 只用本页已接纳准确 previewId，cancel 的 previewId 为空。不传源／副本路径、descriptor、proof 或调用者报告作为删除权限。

- state19：`schemaVersion / runId / basisId / basisReady / source / sourceDirectory / backupDirectory / backupBasename / busy / jobId / action / stage / cancelRequested / previewId / historicalOnly / authenticated / permissionsRestored / executionPermit / flightCommandReleased`。jobId 是最近原 purge job，不是任意维护线程；busy 包括共享槽占用，busy 不等于本页持有可取消请求。source 是原 retired descriptor7 或空 Map。stage 闭集为 idle、basis-ready、running、cancelling、prepared、expired、discarded、complete、failed、cancelled、unsupported、exhausted。
- preview18：`schemaVersion / runId / basisId / previewId / expiresAfterMs / status / canCommit / source / sourceDirectory / backupDirectory / backupBasename / sourceFileIdentity / backupFileIdentity / historicalOnly / authenticated / permissionsRestored / executionPermit / flightCommandReleased`；无预览为 `{}`。原生30000ms提交窗不从 UI 计时续期。status 为 prepared／expired／discarded／consumed／retired；只有本次 prepared 且 canCommit、原生当前同ID、源未变时才可新确认。
- result13：`schemaVersion / runId / basisId / jobId / action / ok / error / workerResult / historicalOnly / authenticated / permissionsRestored / executionPermit / flightCommandReleased`。初始 `{}`；action 为原 preview／commit。原生失败可以 `workerResult={}`，表示没有完整可接纳事实，不能补“没有删除”。
- receipt11：`schemaVersion / action / accepted / jobId / reason / requestOnly / historicalOnly / authenticated / permissionsRestored / executionPermit / flightCommandReleased`。accepted 仅表示请求接纳，不是检查成功、删除完成或取消完成。Root 异常的 accepted=null 作为未知回执，不能强转 false。

preview／commit 各生成新的实际 jobId，不能等于旧 expectedJobId；Worker operationId 正是这个新 jobId，不是旧 backup operationId，也不是成功检查后另生的 human previewId。discard／cancel 同步保原 jobId，不等新的异步 action=result；取消后继续等原 preview／commit 业务终态。

共同 `historicalOnly=true`，authenticated／permissionsRestored／executionPermit／flightCommandReleased=false。descriptor7 为 `basename / kind / databaseEpoch / sealRunId / fileBytes / fileFingerprint / modifiedUtcMs`，kind 只 retired；规范文件名、UUID、小写SHA256及 qint64 范围十进制字串完整核对，逻辑尺寸512字节至4096MiB接受域不作为物理释放量。实际固定产品政策仍由原生独立核对，面板不提高该政策。

state／preview 的 backupBasename 接受域为256码元，容纳真实三个UUID构成的完整备份名，不能误用旧128字符ID上限而阻断正常备份依据。规范名称及其完整源对应关系仍独立核对，不裁剪原名。

## Worker 事实与不可逆结果

Worker result41 为原共同12个否定／历史标记，加 `format / schemaVersion / kind / operationId / basis / proof / previewVerified / observedDirectoryIdentity / destinationDirectoryIdentity / nativeDeleteSupported / decisionFinal / deleteAttempted / deleteReportedSuccess / mayHaveDeleted / sourceAbsentObserved / backupAfterVerified / sourceHandleClosed / backupHandleClosed / cancelObserved / errorCode / diskSpaceReclaimedVerified / sourceLogicalBytes`，再加收据七字段 `receiptBasename / receiptCreateAttempted / receiptCreated / intentWritten / resultWritten / receiptClosed / receiptError`。format=`merivus-fleet-archive-retired-purge`，kind 对应 purge-preview／purge-retired。

完整接纳原 backup basis31，不把 basis 的公开 Map 当来源；它必须是本次真实已核实 retired 备份。proof3 为 `sourceFileIdentity / backupFileIdentity / backupDescriptor`；backupDescriptor 为原8键，两份 Windows volume-file-index 身份必须不同。预览结果还须同本页原 job、basis、run、原 state 源内容和副本名，再与实际 preview18 的两文件ID及完整源逐项对应，才保存本页预览。

分别显示删除前双文件核对、决定、进入删除调用、系统报告、可能已删、事后观察原件缺失、备份后核、两句柄关闭和取消。`previewVerified` 可因最后关闭失败变 false，而实际 deleteAttempted／mayHaveDeleted 已真；不能因前者失败丢掉后者。false 仅是该事实未确认，不是未发生文件效果的证明。`diskSpaceReclaimedVerified` 恒 false；`sourceLogicalBytes` 仅原文件逻辑尺寸，不能标实际空间已释放。

commit 的唯一新收据名为 `fleet-events-purge-<本次jobId>.jsonl`。创建尝试／创建、意图行完整 write+flush、结果行完整 write+flush、close、错误均独立显示；记录不能认证自己的落盘耐久，仍无 fsync／durable 保证。preview 不创建收据。没有完整41或真实系统结果未知时，保留副本及原回执供工程师核查，不补删前状态、不自动重试或回滚。

本页 ownResult 和另行标记的“原生上次结果诊断”都只读。后者可在弹窗销毁后重开帮助查看实际已删／收据事实，但绝不安装 pending、预览、确认、取消归属或权限。保存事实不被隐藏／失败擦成“未删”；也不因旧结果非空获得新的删除依据。

本页 commit 在原按下预览时冻结 sourceDirectory，只将该原字符串随本页 job／ownResult 保作诊断，可显示原收据全路径；不从最新 state 拼旧 job 路径。共享 nativeFacts 没有这份本页原来源，只显示收据文件名并说明位于原固定归档目录，绝不猜路径或恢复操作资格。

## 来源、票据与正常链

plain lifetime 保存非回绕 epoch、consent 代次及独立 ticket。按下到点击冻结实际 workspace、原完整 state、basis／run／job、原本页预览和两项确认；确认按下后源变化或确认 A→B→A 都拒旧 click。两项确认互相独立，不从旧移动／备份复选框继承，提交或作废前先消费，再仅容本同次消费状态。

ticket 覆盖调用、确认消费、回执及结果发布和 callBusy 通知；外部业务不能在清理／Qt通知内重入。按钮的可见可用性只依赖会通知的 Qt busy／pending／native 属性，不把无通知 plain ticket／retiring 放进只读绑定，防结果接纳后两个确认一直自锁；真正业务入口仍独立核 plain fence。清理在任何字段通知前冻结纯JS detached closure／原ID，finally 即使同步死亡仍分别尝试原 cancel／discard；嵌套隐藏／销毁只追加 sticky 原清理责任，不因字段已清空漏取消。lease.release 在 QObject 销毁后只操作已冻结 plain 对象，不重新读取已死 QML 属性。

state／result／preview 是同一原生通知的独立绑定：只在 state 实际空闲、同本页原 job／action／basis／run 及相应 result 已到齐后结算；旧 `{}` 或上个 job 结果不消耗 pending，preview18 尚未对应亦继续等。pending 未接纳期间禁止开启新 preview／commit。隐藏／收起可明确退休本页观察；再次操作必须重新预览和确认。

原 ticket 内的通知记 dirty，不直接丢弃；释放后才按实际回执已安装的 pending 判归属。外来新 basis／run／job 完整退休本页旧预览和确认；本页正常新 job 不因自己的身份推进自锁。来源／隐藏／销毁只用冻结原 job 精确 cancel 和原 preview 精确 discard；晚回真实 jobId 的收尾闭包只用原 facade／原ID，不在销毁后解析旧 QML helper，不会按 shared latest 清他人结果。

commit 一旦被原生接受即烧掉 basis/proof，即使失败／取消也不恢复。preview 失败或取消可在原 basis 仍有效时另行明确预览，discard 只作废准确旧 proof；新维护／采集／来源请求仍会退休依据。这里只是本地文件维护，不给操作员、飞机、任务或飞行任何新权。

## UI/UX 与未执行边界

本轮完整阅读 `ui-ux-pro-max` 及 quick-reference／pro-rules，按用户约束只用静态通用指导，不运行搜索脚本。沿原 QGC palette、字体与按钮；默认收起、危险动作独立、两个短复选标签配完整 PlainText 说明、长路径／hash换行、最小44目标、StrongFocus／Accessible.name、明确取消及失败恢复说明。全部七个交互控件（展开、preview、两项确认、commit、discard、cancel）及反馈区都有同一实际 Flickable 焦点钩子；按钮长标签均为主题配色 PlainText 换行。焦点滚动沿实际 contentItem／contentY／contentHeight 和 mapToItem 有界查找并夹在内容范围，不依赖不存在的 ensureVisible 方法；没有实际视觉／键盘测试。Return／Enter 不意外提交删除，保留正常 Space／点击，不新增动画、顶层页面、路径输入或文件选择器。

以下均为工程师待验证，**全部未执行**：retired新备份双尾才出 basis；sealed／旧诊断／伪Map／同路径不能造依据；非Windows明确 unsupported；独立 preview 不删；两份全路径／UUID／hash／不同文件ID；原30秒过期及确认ABA；正常两确认→commit消费不自锁；state/result/preview各绑定先后；pending期间新操作拒绝；ticket内外来job通知延后退休；隐藏／换源／销毁／晚回ID只精确清原请求；不取消他人；删除前／删除后取消；已删但关闭／收据失败事实保留；未知worker不补 false；旧上次结果仅诊断；恢复仍走原备份导入与restore；长文本、窄屏／大字／主题／键盘／读屏及实际Windows文件句柄行为。

未运行 QML／JS／parser／helper／项目／Worker／SQL／hash／归档IO／删除、预览、编译、测试、硬件、刷机、提交或打包。静态阅读不能证明实际交互、视觉、线程竞态、恶意文件系统竞争、物理空间释放或掉电恢复可靠性。

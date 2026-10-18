# 本地部署组管理：新建与历史监视选择

## 524：已闭旧部署归档与本地容量回收

原两个宿主仍各只嵌一个本面板；本次在原弹窗内嵌[归档面板](FleetDeploymentArchivePanel.md)，通过同一 Guided 的 `deploymentArchive` 窄门面选择旧组、另选文件路径、明确确认后异步归档，并在原生最后核对和实际对象销毁后回收本地槽。没有新建第二套 Lifecycle、顶部页面或飞行发送者。完整交互与值合同见该模块，不借旧新建 / 选择确认。

原 `fleetDeploymentLifecycle` 的 `allocationCount` 现在表示仍占用的 live / pending 本地槽；`capacityRemaining` 仍是 `capacity-allocationCount`。失败保留组和等待实际销毁的组仍占槽，不能在提交退休时提前减数。终身分配尝试另由归档状态的 `lifetimeAllocationAttempts` 无符号十进制字符串显示，不回绕、不因回收而重置；不能把它当当前槽数。原371–375“不回收容量”是当时限制，本批新增显式受控回收，不是取消最后来源、闭合、调用栈或实际销毁保护。

当前任务组、当前监视组、正在退休或失源的组不能在界面选为回收目标。目录行不是“已闭证明”；用户确认也不能替代原生保存、回读之后的最终核对。旧飞机仍可能继续飞行，本地退休不发送停止、落地、返航或解锁命令，不声明任务完成。历史文件可在无 live 飞机或账户时独立只读打开，但不恢复对象、监视、账号、批准或任何权限。

此处仅源码实现与静态审阅，未运行、预览、编译或测试；完整25目标及六类实质缺口没有缩减。

## 371–375 基础接口与524当前接线

[面板](../../res/Merivus/FleetMissionLifecyclePanel.qml)在[指挥中心](../../res/Merivus/CommandCenterOverlay.qml)和[任务规划页](../../res/Merivus/FleetMissionPlannerPanel.qml)各有一个固定入口，资源由 qrc/qmldir 各登记一次。复用原 Guided 和其唯一原生 Lifecycle，不创建第二套控制器、初始组或飞行发送者。这里的“组”是保留的本地对象组，不是空域编队、机载任务实例或跨站控制租约。

371–375当时的8源码/19文档是累计工程增量，不是8个文件即可独立构建的安装包。本批仍只改源码和文档并做限定文本审阅，未编译、测试、运行应用、渲染界面、认证、操作设备、更新PR或打包。`outboundIntegrationComplete=false`仍保留；完整25目标与执行闭环、可信能力、自愈执行、企业跨站、长期运营、真实模型/生态六类缺口没有缩减。

## 控制器接口与只读元数据

新建 / 监视选择仍使用原两个产品方法；524归档面板另使用同一个实际Lifecycle的窄归档门面：

```text
prepareNextFleetDeployment(expectedCurrentToken, operatorConfirmed)
selectFleetObservationEpisode(episodeId, expectedObservationToken, operatorConfirmed)
```

`fleetDeploymentLifecycle`是固定12键的纯值投影：`schemaVersion/sourceKnown/busy/sourceLost/currentEpisodeId/selectedObservationEpisodeId/capacity/allocationCount/capacityRemaining/episodes/currentToken/observationToken`。未知计数为null，不补零。最多8行，每行固定6键：`episodeId/published/sourceRetained/actualSourceUsable/current/selectedForObservation`。界面不持有或序列化原生22键scope、实际QObject、账号凭据或原生调用票。

currentToken只含工厂epoch、outboundGeneration和episodeId；observationToken只含工厂epoch、selectionGeneration和episodeId。它们是显示版本标记，不是权限；A→B→A也有新选择代次。当前组与监视组分别显示，不能用同一UUID或相同任务binding替换原始实际来源。

## 明确操作顺序

1. 打开“部署组 / 历史监视（本地管理）”只创建本地视图，不分配新组、不切换监视、不提示账号。初始焦点为关闭，Esc关闭；Enter/Return不会提交，最终动作须明确点击或聚焦后的空格。
2. “选择创建下一部署组”或某一行“选择本组监视”只选择本页动作，冻结按下时控制器、显示版本和界面代次。实际来源、容量或选择变化清除所有旧确认；未知来源或忙状态禁用操作并保留原因。
3. 创建需分别确认旧飞机可能继续飞行且旧Run占用不被释放，以及旧界面确认失效、新组需重新认证审批。历史选择需确认只改变本地监视、不改变机载任务或移交控制权。复选框默认未勾。
4. 最终提交先消费本页手势，再调用唯一对应产品方法；提交前仍核原视图、控制器和版本。按钮/Qt通知期间保留plain调用票，finally仅清原票，不超时解锁、不自动重试、不执行回滚。
5. 成功变更通常会改变显示版本，这本身不丢弃已返回事实。每次打开另有独立viewTicket；关闭重开、隐藏、控制器替换或销毁后，旧请求不得把结果写入新视图。关闭不撤销已发生的切换。

无Start、ARM、STOP、LAND、RTL、DISARM或紧急停机按钮。需停飞时使用既有独立飞行安全操作，不能把关闭视图或结束本地监视等同于停止飞机。

## 原生同步变更边界

[Lifecycle](FleetMissionDeploymentLifecycle.md)的两个`ForController`方法贯穿native begin、私有栈permit保护的变更、新scope核验和原owner/ID的end。普通legacy入口仍拒绝有调用占用的状态；不能先end再调用legacy，public end也不能提前解正在执行的transition。普通scope匹配不被改写为新scope，以免放宽旧调用的来源约束。

prepare仍要求全部保留组原生发送栈静默、实际shared5一致、候选完整且全新、原来源持续有效。初始组已由构造发布，新建不自动改selected、不迁移登录/批准、不取消旧Run或释放其占用。失败分配保留对象仍占用8个本地槽之一，不自动删除、驱逐或重试；只有524独立显式归档路径最终确认实际销毁才回收该槽，终身尝试不重置。`localObjectGroupAllocated=false`只是未发布别名，不能据此推断没有消耗尝试或容量。

select只选实际保留且可用的已发布组，不新增“必须停飞”的条件，不恢复旧组发送能力。选择同一组可接受但不增加代次，`mutationCommitted=true`、`observationSelectionChanged=false`；界面通常禁用当前已选行。

纯scope和原生来源回调不因普通call/transition busy自拒；权限、来源、账号、风险和时限仍由各原生业务门禁核验。忙状态显示不具原子性或权限含义，全进程认证提示锁仍由原生入口即时核验。

## 返回事实，不伪装回滚

固定12键结果为`ok/reason/transitionKind/mutationCommitted/newEpisodePublished/localObjectGroupAllocated/observationSelectionChanged/controllerCallReleased/currentEpisodeId/selectedObservationEpisodeId/authorityGranted/flightCommandReleased`。最后两项恒false。提交事实在实际发布/选择写点、通知前记录；之后源或owner丢失不抹掉已知true，未知尾部保留null。

`ok`要求原生接受、新scope后置核验和原调用结束均核实，不只是UUID看起来变化。已提交但end或后置核验失败可返回`ok=false`且提交事实true；不能自动重试。`controllerCallReleased=false`表示结束未获核实或本次未获得调用，不证明一定仍忙。界面只显示原请求/返回事实，不把它当当前飞行状态或任务完成。

Guided在已返回的发布事实为true时，先本地失效旧草稿确认并协调旧身份意图，再读后续原生scope；后续读取抛错不能跳过这一步。仅从实际后置观察补出的新事实也作对应本地处理，各只尝试一次；失败保留事实和诊断，不通过新alias撤旧对象。选择变化会使[原监视面板](FleetMissionObservationPanel.md)旧token失效，原issued Close收据仍按原retained owner取消，不接管或取消别人的审阅。

## 交给工程师验证的项目（本机未执行）

- 使用累计完整工程、依赖及注册资源验证Qt类型往返、实际parent、原生方法参数和纯值投影，不能只交早期8文件或当前面板文件。
- 验证两个宿主入口、窄屏/大字号/滚动焦点、亮暗主题及键盘路径；无默认提交、无色彩单独传义。使用现有QGC主题、原生控件和系统字号，未做渲染、对比度或可访问性实测。
- 验证成功prepare保留旧Run/占用与selected，新组无旧账号/批准；原组发送静默与shared5门禁仍逐次核验。
- 验证失败候选计费、容量满、源丢失、认证提示中、其它调用中和嵌套重入时拒绝，无自动重试、淘汰或超时复活。
- 验证选择A→B→A清旧确认、same-ID不增代次、旧Close收据精确路由；仍在飞行的历史组可以被只读观察。
- 验证提交期间关闭重开、隐藏、controller替换和销毁，以及发布已发生但end/返回/后续读取失败时，已知事实不回滚，旧回执不填入新视图。

以上是待验证交接项，不是已经通过的测试、安全证明、最终StartPermit或部署放行。

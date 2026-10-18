# 控制器原生引用作用域与身份提示栈

## 371–375：有原生占用的专用transition

本批在[Lifecycle](FleetMissionDeploymentLifecycle.md)新增两个 `Q_INVOKABLE`，并由[Guided](../../res/Merivus/GuidedActionsController.qml)和[本地组管理面板](FleetMissionLifecyclePanel.md)接真实新部署/保留观察选择入口；既有运行监视面板仍沿用原selected投影，本批没有修改。366–370所述“专用transition/API/产品入口仍缺”现为历史；源码接入仍不等于多部署完整验收，`outboundIntegrationComplete=false`和原25目标/六类保留。

```cpp
QVariantMap prepareNextEpisodeForController(QObject* actualController,
    const QVariantMap& expectedCurrentScope,
    const QVariantMap& actualSharedSources, bool operatorConfirmed);
QVariantMap selectObservationEpisodeForController(QObject* actualController,
    const QVariantMap& expectedSelectedScope,
    const QString& episodeId, bool operatorConfirmed);
```

expected严格使用原22键current/selected引用scope；传入controller仅与实际parent QQ地址比较，不解引用caller raw值。两个接口自己begin并持有原同步调用，内部私有栈permit以实际地址、owner QQ、callId、purpose和原scope关联，只授权本wrapper进入对应私有mutation及原ID end。没有public bypass；原legacy prepare/select仍拒普通active call，public end也不能提前清持permit的transition。普通begin固定六键与scope22均未改。

mutation之前匹配旧scope；实际publish或selection改变后，原call仍保存旧scope，后置改为核私有receipt中的新actual scope，不能用generic `controllerCallMatches(oldScope)`误拒自己，也不能把原scope改成replacement。清理核原owner+ID，与原scope失配独立；新scope后置、accepted及end全部核实才ok。end失败保持原active，不超时/自动解锁；QQ死亡不能触碰旧this，也不能回滚已publish/selection事实。QML外层transitionBusy用于同步入口重入，不把纯来源回调因本次busy机械自拒。

原生固定12键值DTO为 `ok/reason/transitionKind/mutationCommitted/newEpisodePublished/localObjectGroupAllocated/observationSelectionChanged/controllerCallReleased/currentEpisodeId/selectedObservationEpisodeId/authorityGranted/flightCommandReleased`。kind固定 `prepare-next-episode` / `select-observation-episode`；reason与两ID是QString，ok/released为Bool，末两Bool恒false。四个mutation事实为Bool或空QVariant未知，allocated对应published；admission尚未开始则false，owner丢失且无commit收据则未知，真实commit后即使后置/end失败仍保留true。没有raw引用、可复活权限或 `controllerCallEnded/Verified`别名；缺失/未知不得由UI补false。

选择相同组可accepted且committed=true、selectionChanged=false，不递增选择代次；发布current不自动选择观察组。prepare继续全部retained quiet/shared5/fresh/QQ/parent/UUID核验，失败分配尝试消耗既有容量，不自动prepare初始之外的新组、登录、复制批准或grant。两个transition都不停止Run/飞行、不释放旧Run/Swarm占用，不把观察选择或本地call结束当远端idle/Completed。

本轮仅源码和限定人工/文本审阅，native新增范围有独立只读交叉复核，未见确定新增静态阻断；没有编译、测试、解析器、运行UI/Qt/认证/API/helper/应用或设备、刷机、PR更新或打包。工程师仍须验证exact Int/QObjectStar往返、方法绑定、父级/销毁重入及真实post/end尾部；不能放宽原合同宣称通过。完整执行闭环、跨站和长期运营仍有实质代码及验证缺口，交接需累计完整工程而非本批增量单独构建。

## 历史366–370：真实控制器调用占用与私有引用桥

本批只列相对上一阶段的5源码增量，交接仍需累计完整源码、pri、注册和原生依赖。Qt目标环境尚未验证22键scope的 exact Int/QObjectStar往返、实际方法绑定、Root parent及模态销毁，不能通过宽松类型转换或跳过合同伪造通过。以下均为源码合同，不是运行证明。

最新sticky invalidation：nested draft在finally仅调用原scope cancel/revoke-only helper；无法核实撤销时私有 `retiredCurrentScope` 持续拒同组current入口/回调，不自动超时复活。selected观察仍可读，不结束Run、直接/强制释放Run/Swarm占用或撤回已queue。cancel true仅表示请求接受；各native自身仍按原drain规则退休/释放自己的占用，不把请求接受冒充排空完成。selected投影先查询同scope Session（可触OS），再获取纯Series/Run配对并末核；它仍不是原子业务快照。

本批5源码为 [Lifecycle.h](FleetMissionDeploymentLifecycle.h)/[Lifecycle.cc](FleetMissionDeploymentLifecycle.cc)、[类型注册](../CustomPlugin.cc)、[Guided](../../res/Merivus/GuidedActionsController.qml)、[监视面板](../../res/Merivus/FleetMissionObservationPanel.qml)，同步18文档。两类注册已接，Guided direct-child 工厂的构造已有 initial，不能自动额外 prepare。14个raw引用仅在private id内；公开业务入口通过private bridge，24个来源回调走原scope helper，并非把所有函数改成private。当前 `runCurrent` 覆盖30个业务入口加登录/退出2个身份入口，`runObservation` 为4个观察调用；不公开raw refs，也不在源失效后fallback到legacy实例。

原五个纯22键 scope 接口保留，另有三个原生调用接口。`beginControllerCall(QObject* actualController, const QString& purpose, QObject* actualCloseWorkflow)` 返回固定六键：`ok` Bool true、`callId` String、`purpose` String、`scope` Map22、`authorityGranted`/`flightCommandReleased` Bool false。purpose 只允许 current/selected-observation/retained-close，前两 Close 必须 null。原实际 parent QQ/线程、无 active call/switch/sourceLost/destroying/提示栈才可 begin；epoch+原生单调代次的 ID 不重用，计数耗尽拒绝。caller raw QObject 仍只作地址比较，不因传入值解引用。

`controllerCallMatches(actualController, callId)` 为纯 owner QQ/parent/ID/原 scope 关联，不查询 OS、公用 current、quiet、prompt 或 full gate；它不是认证或许可，也不冻结 shared5/任务/账户/风险。`endControllerCall(actualController, callId)` 只清原实际 owner+ID槽，允许 scope/source/parent 后变，提示栈仍活则拒；无自动超时或 owner 死亡解锁，不清新 call。prepare/select 在入口、回调后及写前拒 active call，pure scope/callback 不因此自拒；`controllerCallBusy` 只报告本地栈占用。

外层 plain JS lifetime 在同步模态销毁后仍可先判 alive，再对原 factory/owner/ID finally 收尾；nested draft 标记 active current stale。后置核验不是回滚，已 queue/可能发生或未知结果必须保留。selected 的 Series/Run/Close/Session 四值来自同一 scope，sourceToken 只序列化 epoch/选择代次/episodeId 元数据，不序列化22键 QObject map；完整 issued receipt 的取消回到原 retained Close，不按 UUID 搜索，显式监视登录可撤未消费批准但不结束 Run。

新部署/选择产品入口及专用 transition 原生 API 仍缺，不能在普通 call 中自行切代；`outboundIntegrationComplete=false`，原25目标/六类仍有实质源码缺口。已完成限定文本审阅和多组限定人工源码交叉复核，具体证据见本轮静态审计，不是运行或安全通过；未编译、测试、运行helper/认证/应用/设备或更新PR/包。以下361–365保留为历史范围，不再代表未注册/未迁移的当前状态。

## 历史361–365 源码范围

本轮修改 [Lifecycle.h](FleetMissionDeploymentLifecycle.h)、[Lifecycle.cc](FleetMissionDeploymentLifecycle.cc)、[Session.h](FleetOperatorSession.h)、[Session.cc](FleetOperatorSession.cc) 四个原生文件，为后续 current/历史观察控制器接线提供纯引用 scope，并补认证调用栈互斥。没有注册新 QML 类型、迁移 Guided 的14对象或加入新部署/历史入口。

引用匹配只表示**本工厂、原实际 Episode 与14个 owned 服务仍是同一组**。它不冻结 shared5、账号、凭据、任务/Run、风险、遥测或权限，不授予上传、解锁、启动、控制或监视关闭许可；所有原生 full gate 和实际业务来源检查仍须执行。

## 三种引用 scope、五个查询

| 纯原生接口 | 含义 |
| --- | --- |
| captureCurrentScope() | 当前 outbound Episode 的实际引用与独立代次；失败返回空 map |
| currentScopeMatches(scope) | 与当前原生快照逐键/类型/值精确比较 |
| captureSelectedObservationScope() | 当前明确选中的历史观察 Episode，而非 outbound 别名 |
| captureObservationCloseScope(actualWorkflow) | 按实际 Close 地址匹配本工厂保留原组，不按部署/机号查询 |
| observationScopeMatches(scope) | 按 selected 或 retained-close 的原生规则重建并精确比对，不能混入 current scope |

三个 kind 分别固定为 `current-outbound-reference-snapshot`、`selected-observation-reference-snapshot`、`retained-close-owner-reference-snapshot`。

每个非空 map 恰22键：

- `schemaVersion`：Qt Int 1；`scopeKind`、`lifecycleEpoch`、`scopeGeneration`、`episodeId`：QString。
- `episode` 和14个 owned 字段均为真实 `QObjectStar`：`deployment/session/uploadApproval/evidenceRefresh/contentCheck/startReview/startApproval/startupAuthority/initialTransaction/groundReview/groundApproval/groundAuthority/startupSeries/observationClose`。
- `authorityGranted=false`、`flightCommandReleased=false`：真实 Bool。

current 与 selected 的代次各从1开始，用 QString 保留精确整数。仅**成功发布**新组递增 outbound 代次；仅**实际改变选择**递增 observation 代次。失败分配、选相同组、查询和普通总 revision 变化不递增它们；溢出拒绝、不 wrap。新 current 不自动更换 selected；选择旧观察也不撤销同一 current 的引用匹配。

retained-close 的代次固定字符串 `"0"`，按实际 Workflow 对应的 retained QQ/UUID/owned provenance 验证，不要求该组仍为 outbound current 或 selected。它只是找到原对象组，不能把旧 Session/Series/Close 替成新组，也不授权这次关闭操作。

## 纯查询与 raw QObject 值

capture/matches 只读原生对象、父级、retained 记录、UUID、私有来源与代次，重新核14个 QQ；不调用 public current getter、OS、QML、switchEvidence 或完整授权，不混入 `_switching`、transport quiet 或全局提示 busy。瞬时忙碌不是对象被替换，不能让查询在自己的认证/传输 gate 中自拒。

逐键要求数量、键名、Qt 元类型和值精确一致，拒绝空/未知 scope、额外字段、数字/字符串或 Bool 混型、不同 factory/epoch、替代 Episode/服务。`QObjectStar` 只作地址比较，不通过调用者提供的 raw 指针调用 thread/parent、qobject_cast 或其它方法。原对象已毁时，由 retained QPointer/原生记录判失效，不按相同 ID 重建。既有 `episodeForObservationClose` 也去掉了对传入 raw 地址的直接解引用。

返回的 QVariant map 不持有 QObject，不延长对象寿命，不是 QPointer 容器。调用者不能直接把旧 map 的 raw 值当活对象；后续控制器必须在每个 OS/原生调用前后匹配原 scope，并由实际 C++ 业务 QPointer/full gate 保护回调。不能持久化、序列化/恢复、导入或发给日志/公开面板作为可恢复凭据；匹配成功也不锁住未来状态。

## 全进程 GUI 认证调用栈互斥

两个 Session 入口 `authenticateOperator` / `authenticateApprover` 先作纯 owner-thread 确认，然后立即获取唯一 `AuthenticationPromptStackGuard`。它覆盖之后所有工作站/身份 OS 检查、凭据提示、changed 回调、结果构建和最终身份校验；所有 Session、legacy 与不同 Episode/工厂共享同一 GUI 栈 token。

guard 是第一个函数局部、最后析构，不可复制/移动；只在全局 token 仍等于本 guard 时释放，不访问 Session 或 application。原 Session 在提示或 changed 中销毁，仍不能提前释放；`_promptActive=false` 也不代表原函数已退栈。nested 被拒不会释放外层 token，也不清除、烧掉或更新外层凭据。没有修改 Windows 提示实现、角色组、系统权限或原凭据期限。

Session 新增非 Q_INVOKABLE 的 static `authenticationPromptInProgress()`：无 app/错误线程保守 true，GUI 线程只读 token，不调用 OS。Lifecycle 在 prepare 的入口、回调后 same 和最终发布前，以及选择入口/实际写入前查询它，认证栈未退出就不切代/改选择。原 Run、占用和历史不取消、不删除。

该全局 busy **不进入** identityCurrent、full gate、Close 消费或纯 scope 查询；原本允许在本人提示阶段核主身份的逻辑保留，旧 Close 也没有增加 outbound-current 或在线/落地要求。历史361–365所列控制器冻结/finally已由366–375后续源码推进；所有UI模态、Qt/OS交互仍未运行验证，不据此称这些流程已验收。

`Lifecycle.current.identityPromptBusy` 只是读取时诊断快照。token 释放不自动广播全局 Lifecycle.changed，不保证界面缓存实时更新；真正切代/选择用原生实时 getter，禁止依 cached current.busy/identityPromptBusy 授权。

## 工程师交接与剩余工作

本机仅改源码/文档并作限定人工/文本审阅；未编译、测试、运行构造器/工厂/认证/API/helper/应用/SQL/日志/仿真/设备或刷机，未改 PX4、更新 PR 或打包。检查记录见[静态审计](../../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)，不是 Qt 或 OS 运行证明。

工程师后续需验证（本机未执行）：原五个scope查询、三个call和两个transition接口的实际方法绑定及QML→QVariantMap往返精确类型、22字段防替代/删源、跨工厂、A→B→A的selected旧代次拒绝、失败分配不改变current scope、同组选择不递增、commit后新scope核验与原ID end独立、后置失败仍保留publish/selection事实、旧Close在更换current/selected后仍只定位原组、stale raw QObjectStar不被解引用，以及提示中重入/删Session/锁屏/取消/尾部changed时锁保持至旧栈退出。非Windows仍不支持实际本地认证。

历史361–365的迁移清单——Guided14引用、原scope前后/finally、shared Role身份意图、selected四值投影、类型注册及新部署/历史选择入口——已由366–375写入后续源码，不能再写成全部未开发，也不能把存在这些模块当完整完成。`outboundIntegrationComplete=false`仍保留；原25目标/六类、正式StartPermit/净空/执行结果、权限和容量长期留存、跨站与长期运营继续。

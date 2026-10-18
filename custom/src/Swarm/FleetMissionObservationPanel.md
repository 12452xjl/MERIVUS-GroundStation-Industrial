# 运行监视面板：查看、审阅与本地结束

## 429：独立运行报告入口

入口区域增加默认折叠的[FleetMissionExecutionReportPanel](FleetMissionExecutionReportPanel.md)。报告生成只要求选定的实际prepared Run，不借用结束观察的startupRetired、账号或确认门槛；退休／来源未知的历史观察允许按原未知语义报告。外部文件打开不要求Run存在。

这里的来源未知指Run内逐机遥测来源未知；实际本地选定组及previousStartup仍须存在且身份一致。整个组／owner已经丢失时，不从旧UI缓存或另一事务补造报告，仍可另行打开已经导出的历史文件。

报告保存在Root的一份冻结值中，独立于当前监视与原关闭审阅。收起或切换来源不会自动保存、更新、重建或恢复权限；文件选择／确认意图有自己的失效边界。原关闭监视流程不变，结束监视和报告中的落地候选都不等于完成任务。未运行UI或验证效果。

## 371–375：独立历史选择入口已接源码

[本地部署组管理面板](FleetMissionLifecyclePanel.md)已在指挥中心和任务规划页各接一处。它只选择实际保留且可用的selected组，不移交远端控制、不停止任务或释放占用；新建current组不会自动把监视切走。选择通过专用native begin/私有permit/变更/新scope/end链，不用先释放普通call再调用legacy的空档。

本面板既有sourceToken/完整issued收据合同不变。选择A→B→A有新选择代次，清旧三项确认；取消仍走原retained Close实际对象，不在新selected上查旧review或接管另一入口。显式历史身份不迁移到新组，原Close权限和两账号期限保持。切换结果已提交与call结束分开，未知不作未发生或回滚，两个面板都不自动重试。

本批8源码/19文档仅源码与限定文本审阅，未编译、测试、渲染/运行UI/认证/helper/设备，未更新PR或打包；`outboundIntegrationComplete=false`和完整25目标/六类继续。下面366–370及更早为历史状态，旧“无历史选择入口/API”已由本批源码增量替代，不代表完整任务生命周期已完成。

## 366–370：selected来源代次与原关闭收据路由

选中监视投影先完成该scope Session的OS身份查询，再读取并末核纯Series/Run配对；同scope不保证原子采样。当前组sticky draft失效/`retiredCurrentScope`拒绝current工作不封禁selected只读监视，也不自动结束Run。Qt严格类型传参、方法/parent/销毁尚未运行验证；五源码增量须与累计完整工程/pri/注册/原生依赖一并交接。

[Guided](../../res/Merivus/GuidedActionsController.qml)现从同一 selected 原生22键 scope 获取 Series/Run/Close/Session 四个值快照，源失效不回退另一组。公开 `fleetObservationSourceToken` 只含 factory epoch、选择代次和 episodeId 的字符串元数据，不暴露实际 QObject；面板 runToken/ownToken 同时绑定它，选中来源或 ABA 选择变化会清旧确认并尝试精确取消原审阅。

申请调用携 expectedSourceToken；后端返回的完整收据含 reviewId/workflowEpoch/generation/context 和 observationSourceToken，面板按原 controller/epoch/来源保留。批准传完整原收据并仍要求原 selection 当前；取消或视图销毁也把原 issued 收据交给 Root，后者用原 retained Close 实际对象路由，不搜索 UUID、查询新 selected alias 或接管另一入口的 review。返回未知/抛错不证明未关闭、占用未变或已回滚，不自动重试。

新增选中监视的显式账号认证入口，绑定按下时的来源 token 和 plain lifetime；登录可使 shared Role 改变、撤当前未消费批准，但不结束 Run。原三项默认未勾、默认取消、危险 Enter 禁止、独立入口 token、30秒原生期限和三态 unknown 边界保留。外层同步 native call/finally 与全进程提示栈保护已写入源码，不代表 UI/模态时序已验证。

本批实际5源码/18文档，已完成限定文本审阅，不是UI运行或安全通过；没有新部署或历史选择的产品入口，也未实现专用 transition 原生 API，`outboundIntegrationComplete=false`，25目标/六类继续。未编译、测试、渲染/运行UI或操作设备/更新PR/包。以下335–339为初始面板的历史范围。

## 335–339 实现范围

源码是 [FleetMissionObservationPanel.qml](../../res/Merivus/FleetMissionObservationPanel.qml)。指挥中心及地图任务规划页各有一个固定入口，复用同一 GuidedActionsController 和 [Close 原生工作流](FleetMissionObservationClose.md)，没有第二个 backend/controller。qrc 与 qmldir 各登记一次。入口不依赖草稿有效、已生成提案、上传权限、飞机在线或地面状态；仍可能受既有宿主界面的整体显示/折叠规则约束。

本批只为既有本地监视关闭接 UI，不提供 Start、ARM、STOP、LAND、RTL、DISARM 或机载任务完成按钮。结束监视可能失去后续观测，飞机仍可能继续飞行；local 占用为空也不证明成员可以重新分配。需要停飞时使用原有独立飞行安全流程，不把这个界面当紧急停机。

## 来源与操作顺序

1. “运行监视 · 查看与结束审阅”只打开视图，未生成review或调用系统凭据窗口。默认焦点为关闭/取消，显示当前实际Run和原成员及可能的历史结果。
2. 当前可操作来源须为Series未丢证据且previousStartupId/previousTransactionEpoch与实际Run匹配，并且prepared、startupRetired、未retired。Guided读取最新actual owner，缺失返回空值，不回退到initial Run或按ID另找飞机。
3. “生成本次关闭审阅”才显式调用request(startupId,runId,true)。仅生成原生短期review，不结束监视、不提示第二账号。账号/角色/排空与context18的完整有效性仍由native决定，UI门禁不是认证。
4. 显示冻结review、Run/启动/部署/记录/版本/计划与原成员，以及两个所有者的actual occupancy列表/authorityRevoked；撤销权限不是占用消失。最多32个成员的固定11字段风险全部显示，包括MODE步骤/排队提交、不确定、连续性及原对象保留，不当当前遥测；完整epoch/内容与审阅hash/库及两代次可显式展开。每次新review三项默认未勾，分别承认失去监视、仅释放本次local占用、飞机可能继续飞行。
5. 最终按钮明确申请另一账号审批并结束本地monitor；它只传原backend的三个显式true，实际不同SID/WTS/权限/专用purpose/期限及唯一commit仍在native。危险确认不是默认Enter动作，支持明确点击或聚焦后的空格。
6. 关闭/Esc仅取消本页拥有的原review，不能停止飞行、回滚已发生的退休/释放或恢复凭据。再次尝试必须由人重新打开/申请，绝不自动重发或创建review。

## 每入口归属、期限与重入

每页私下持有actual controller、reviewId、workflowEpoch、generation和完整context组成的token。取消前核原controller当前完整token仍相同；另一入口不能消费/取消它。new token/controller/context变化、明确stale/failed/cancelled/expired/closed终态或显示期限耗尽清空确认；authenticating/consuming自然ok=false不触发自动取消。普通Run滚动报文/quiet计数不参与token，不因正常观测持续清空人工确认。

UI显示期限从最早request墙钟开始，并用native返回remaining再保守扣除请求耗时；通知、点击或凭据返回不重启。墙钟回退使本页禁批准，不能利用时间变化延长窗口；最终仍由native单调30秒决定，UI数字不是授权。过期仅清确认，不自动结束monitor。

公开current可能在系统modal内仍为awaiting-review/busy=false，不能据此解除UI锁。独立callTicket贯穿整个同步调用，仅实际返回、plain JS lifetime仍alive且ticket相同才清callBusy。Controller换代清旧ticket，旧调用不能清掉新调用。销毁前标记plain guard失效；请求返回后即使页面已销毁，放弃只用局部closure核exact receipt并尝试取消，不读已销毁QML对象。

## 结果与可访问交互

- 原生monitoringRetiredKnown/monitoringRetired均true且releaseKnown/released均true：仅称本地监视已结束、两个原所有者的实际local map已空。
- releaseKnown=true、released=false：已知仍有local占用，不称未知/成功释放，不自动补释放或恢复权限。
- post-state缺失/损坏：释放未知，null不补false/true；若monitor退休也未知则明确没有已核实效果。
- 每份结果显示其review/Run ID，不把旧运行或另一入口的结果归给当前飞行。取消/异常不证明“什么都没发生”。

采用现有QGC控件、主题色与屏幕字号。ui-ux-pro-max的静态规则影响默认取消、独立风险确认、语义警示文本、危险Enter拦截、焦点边框、换行/长ID、44px最低行高、错误摘要焦点及滚动可见。没有引入新字体、图标/动画、颜色库或插件；没有以颜色独自表示结果。原生凭据窗口接受系统已有输入能力，面板不采集密码或SID。

## 验证边界与交接

仅源码阅读与限定文本检查，不运行skill搜索脚本、应用、认证、业务/helper、QML解析器、编译/测试/仿真、日志/SQL或设备/刷机。视觉、明暗对比度、屏读/焦点、触控/键盘、缩放/小屏/横屏，以及真实modal内源销毁/换代均未实测，留给工程师。完整执行许可/净空/正式远端结果、长任务/新部署/授权容量与原六类/25目标仍继续。

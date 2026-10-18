# FleetReserveDispatchPanel：备用机独立出勤人工流程

## 520：独立确认刷新版本与参数，再手动读取配置依据

在既有第2步增加可选的显式刷新，不新增组件或资源。操作员须保持本页实际私有capture、明确选定其备用ID，单独确认后调用`requestFleetReserveDispatchEvidenceRefresh(captureToken,reserveId,true,cancellationState)`。它复用原`mission.evidence.refresh`权限及实际服务，不借519缓存读取确认；只请求版本和固定29项参数，仍限真实地面、未解锁／未飞行、已完成初连的PX4多旋翼，以及原身份、PRIMARY、参数和传输空闲门。总期120秒、单阶段4秒，不延长、不写参数或发飞行命令。

本页只保存这次返回的实际requestId／generation及原owner、scope、capture、备用ID；`fleetReserveDispatchEvidenceRefreshStatus(captureToken,reserveId,requestId)`闭14封装原actual current27值副本，`cancelFleetReserveDispatchEvidenceRefresh(captureToken,reserveId,requestId)`只清原请求。共享`fleetEvidenceRefresh`仅作为通知依赖和占用提示，不能用其latest ID认领或取消别页工作。显示原请求、目标、阶段、当前参数、剩余时间、完成数、原因和`readOnlyRequestsMayRemainInflight`；该警告是保守尝试事实，不是实际发送／ACK／响应因果证明。封装top.active表示实际仍匹配原job在途；status.active是该原job快照当时的值，后来generic请求替换原生槽时可保历史true而top.active／current均false，不改写旧status。来源脱离或状态查询失败的旧回执强制标为非当前历史，保留原status和active历史，不补造终态／取消成功；同scope仍在途或未知时保守阻断续作，可精确取消或明确退出恢复。

新的刷新先撤本页旧offer、hash、速度采用、确认、提案及可采用审阅锚；Root在原capture冻结后处理相应审阅／本地批准退休，不由Panel再发父级`reviewInvalidationRequested`打断本次bridge。归档事实与原capture保留。同一完整来源键下，纯值编辑器可保本页手填动作／坐标及已经明确采用的canonical启动策略纯拟值，不能保旧参数依据或把未采用子表单当采用。实际调用前也检查`startupPending`；有未采用子表单时刷新被阻断并说明，不能只依赖按钮disabled。真正换capture／备用ID不继承这些拟值；原刷新回执另作非当前历史显示，不把失效来源隐藏成“未发生”。

刷新在准备中、在途或状态未知时锁本页读取、配置采用、预览和审阅续作。完整调用与结果发布期间持plain票据；独立确认代次及按压快照拒旧pressed、确认ABA或新来源覆盖。第4参是本次同引用`{cancelled:false}`撤销信号，不能授权；准备期取消／隐藏／禁用／换controller或scope／销毁先置true，晚返回真实ID仍仅精确取消原请求。取消／失败停止后续读取，不撤回可能已经发出或收到的只读材料，不自动恢复原批准；已结束历史不会因普通配置编辑被重新取消或重写。

完成只表示该有限批次的观测采集结束，不保证所有12项配置参数在此刻仍满足519的30秒新鲜度。没有自动调用observe、采用速度、生成配置、审阅或批准；操作员须再单独点击“读取本次参数依据”，取得新的actual offer，再重新采用速度、确认和生成。无ID拒绝不遗留ID-less忙槽；有实际ID但最后来源检查失败仍保留原partial并精确清理，不声称未发送。

本轮沿现QGC主题、可换行PlainText、明确标签、键盘焦点及最小44高度，使用ui-ux-pro-max的静态通用指导，未运行技能搜索、QML、应用、预览、解析、编译、测试或设备验证。[本批40项工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原519以下合同保持，但“读取不请求刷新”只指519独立observe动作，不指本节另行确认的刷新动作。

## 519：原生参数offer与结构化新配置

第2步不再要求手填完整JSON或hash：本页明确选中实际capture的备用机后，单独调用`observeFleetReserveDispatchProfile(captureToken,reserveId,true)`，仅认领原capture／ID／本页代次对应的exact10回执、原native observation19与policy8。Root保留私有本次offer，不从返回Map恢复来源。选择／输入／隐藏变化使旧确认不可复用；原JSON仅作完整值核对，不自动作为新offer。

新[纯值配置编辑器](FleetReserveDispatchProfileEditor.md)复用原StartupPolicyEditor，操作员明确采用原备用预算速度并提供新动作、坐标、时序和完整策略。巡航和返航同原cruise，爬升／下降仍须匹配预算及当前参数；冲突时回原任务重做。未知或空白不填默认，AMSL不是离地高度，不从旧主机profile或GNSS补位置，也不伪造Draft来绕原编辑器绑定。

本页调用`previewFleetReserveDispatch(captureToken,request,offerToken)`；原request仍恰三键reserveVehicleId／executionProfile／operatorConfirmed。Root核原私有offer并重新采集原native参数fingerprint和profileHash，再进入原提案生成与完整审阅。旧二参数调用、另一机号或同值但不同观察代次均不能领用新offer。该读取不请求刷新或写参数，不授执行权。

合法预览尝试按一次使用消费原offer，面板仅在本次结果与上下文核对后精确退休对应的本地预览资格，保完整profileText供核对；失败后须重新明确读取，不能反复点击旧token。重新读取同capture／备用ID时可保本页用户填写的纯动作／坐标／策略文本，但须撤旧速度采用、确认及hash绑定；新依据下重新采用并生成完整配置。真正换capture／ID不跨来源继承，未采用的启动策略表单也不能因重读自动变为已采用。

原515重审、准确attempt、采用／批准与共享部署流程保持；重审仍审阅原完整已生成提案，不自动重新生成配置或恢复旧批准。源码与材料静态收口不表示构建、界面或飞行验证；[本批40项交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。下方旧‘手填JSON’步骤由本节替代，其他原门禁保持。

## 515：完整审阅明细与保留提案的显式重审

本页在准确新记录摘要之后、采用之前嵌入 [只读明细](FleetReserveDispatchReviewPanel.md)。只传本页 retainedReviewVisible 对应的实际新 reviewContext，不自动认领共享最新值，不把完整字段显示当作已批准。逐项协议编码、完整能量／风险／空域可在同记录内分页展开；其余审批和唯一共享部署不改。

本页只保存真实返回且与同proposal状态对应的reviewAttemptToken。新的重审按下冻结exact5：captureToken、proposalToken、reviewAttemptToken、reviewToken、requestId，勾选确认后才调用Root的rereviewFleetReserveDispatch。旧来源已过期只影响审批资格，不必删除私有规划来源；Root仍核准确旧尝试、原source／scope与空闲门，不抢外来新记录或在途工作。失败的已退休事实保留，不自动恢复原批准。

重审复用原完整新profile与全部路线，不重选、补零、重算前端任务或自动刷新。新同步reviewToken／异步requestId与同attempt、proposal、record相符才认领，新结果必须重新采用／本地批准。没有任何一次“展开”或“重审成功”直接上传、起飞。

隐藏／收起保留描述性的proposal／attempt锚，清原同意和未采用审阅的本页采用资格，精确取消本页pending；再次打开可明确重审，不自动恢复确认或批准。真实输入编辑则退休这些旧规划锚；controller／scope改变或销毁仍清本页exact capture，不撤已独立采用的reviewContext。显式退出仍保归档事实。

本轮仅源码及静态对读；ui-ux-pro-max用于现主题／PlainText／长字段／焦点规则，没有运行QML、解析、编译、测试或设备验证。下方514原流程描述为历史，515的重审和隐藏保规划锚规则以本节为准。

本模块为既有 `FleetMissionPlannerPanel` 内的 QML 子面板。它消费真实 Root 保留的完整任务／计划来源与 `FleetReserveDispatchPlanner` 提案，不创建 Run，不接管原组，不恢复旧审批或飞行权限。本文描述源码合同，未进行 QML 解析、编译、运行、仿真或真机验证。

## 1. 输入与父面板合同

- `controller`：当前实际 GuidedActionsController。
- `inputSettled`：其它编辑流程已收口；父级计算必须排除此子面板自己的 `pendingInput`，否则自锁。
- `expanded`：展开状态；`editingLocked`、`planningBusy`、`episodeToken` 和 `pendingInput` 为只读状态。
- `focusVisibilityRequested(item)`：将焦点反馈滚动到可见区域。
- `reviewInvalidationRequested()`：仅在已经持有来源、且旧提案／审阅存在时编辑输入发出。初次展开、勾选冻结确认、第一次填写备用 ID／profile 均不先撤原来源。

本页已明确采用其对应记录时，`pendingInput` 为 false。采用记录仍保留，不能因共享部署区读取本页状态而自动取消本地批准或形成循环门禁。

## 2. 分开的人工步骤

1. 明确确认冻结当前完整来源。按下时冻结候选的原 binding；点击时仍须同来源。Root 先读取实际原 task、plan、PlanStore binding9，再撤旧本地审阅。`candidate.available` 不是权限。
2. 从本次冻结来源的实际 `reserveVehicleIds` 明确选择一架；初始没有默认选择。填写非空完整新 `executionProfile` JSON 对象，最多 16,384 字符；没有旧 profile 或模板自动回填。
3. 独立确认这是单机新任务、不接管旧组、不重演旧待机、重审当前电池后生成预览。提案仅由返回的 `proposalToken` 认领。
4. 显示完整 summary14、实际新 member 的完整去程／自身返程和 profile。路线每页 8 点、完整字段以 PlainText JSON 显示；不以备用预算简化路径替代航路。summary、member ID、点数或 profile 缺失不允许提交审阅。
5. 独立确认发起一次完整当前审阅。同步仅认返回的 `reviewToken`；异步仅认实际返回的 `requestId` 加同 proposalToken、同来源代次及对应归档记录。共享区其它结果不自动领用。
6. 独立确认采用为唯一共享部署来源。采用可以保留阻断记录供核对，**不等于审阅通过或批准**。
7. 本页已采用且其真实当前 `approvalAllowed` 为 true 时，分别勾选确认并调用原 `requestFleetMissionRemainderApproval(reviewToken)`、`approveFleetMissionRemainder(reviewToken)`。前者只允许 `preview`，后者只允许 `awaiting-approval`；按钮按下冻结原 review/archive 身份和状态。没有新增权限路径，也不把采用改写为批准。
8. 只有共享部署区再经过其原独立上传与权限门禁才可继续。这里没有上传、解锁、起飞或飞控写入调用。

## 3. 身份、并发和生命周期

本页私有 JS lifetime 记录实际 controller、部署 scope、单调输入／确认代次、自己的 capture/proposal/review/request ID 和当前调用票据。显示快照不构成权限。按下到点击的来源、输入、同意代次必须一致；默认均不勾选，清同意期间发生额外 ABA 会拒绝本次调用。禁用按钮撤掉按压快照。

原 Root 操作有独立票据与最终来源检查；本页调用票据跨 `callBusy=false` 通知保留，阻止同步通知重入。任何通知后继续操作前先检查原局部 lifetime；销毁后的晚回执只用事先捕获的 owner 和精确 ID 清理，不访问已销毁面板。

输入变化、隐藏、收起或禁用会撤本页按压／同意、提案和未采用审阅的可采用资格，并精确取消本页已返回的后台 request ID。它不猜测共享最新 request ID、不清别人后来创建的记录，也不自动撤已经明确采用的唯一共享部署来源。再次打开不恢复旧同意；保留资料只读显示。

controller 或部署 scope 更换以及组件销毁时，仅清原 controller 上本页精确 capture 和 pending request；Root 的已采用 reviewContext／归档不因此删除。显式“退出／撤销本页采用”才按本页保留的精确 reviewToken 清对应本地上下文；没有删除归档、撤销飞机动作或回滚已发生的本地事实。

后台请求可显式精确取消。取消／关闭与成功通知竞态时保留已经生成的记录事实，晚结果不自动复活旧资格。冻结动作已实际返回 captureToken 而最终检查失败时，只精确释放该次返回的来源；失败审阅只取消该次实际返回的 requestId，不猜共享最新 ID、不撤归档 reviewContext。清理通知后仍核原局部 lifetime／代次／controller 才显示反馈。调用失败、返回时来源已变、异常或不完整回执均要求核对记录，不自动重试。

## 4. 展示与限制

沿现有 QGC 主题，说明与动态 JSON 使用 PlainText、可换行控件；确认和主要按钮最小高度 44，支持键盘聚焦；输入错误可请求父级滚动到反馈。本批仅在既有 qrc／qmldir 中登记此本地面板，没有新增外部依赖、计时器、权限服务或原生 QML C++ 类型注册。

`current` 只是保留槽／对应 scope 与实际归档绑定在检查时一致；不是认证来源、持续性能核验、真实轨迹或飞行许可。AMSL 路线和时间为提案／编译值及估算；不会把备用出勤称为原任务已接管、已完成或运行成功。

## 5. 工程师尚需执行

以下均未执行：Qt 5.12 控件／资源编译及窄宽滚动、焦点与主题检查；来源冻结正常流程；无默认候选与空／超长／无效 JSON；summary14 与完整 member 对应；同步和异步审阅真实 ID；其它请求同 proposal 的结果不能领用；按压期间修改输入、同意 ABA、隐藏／销毁／controller/scope A→B→A；取消与晚成功交叉；采用后隐藏不撤部署且 `pendingInput=false`；申请批准与明确批准确实沿原门禁进入 approved；审阅失败／能量不足／净空阻断不能批准；退出只清本页精确 token；其它 single/group/area 流程与部署共享槽互斥；没有自动上传、解锁或飞行命令。

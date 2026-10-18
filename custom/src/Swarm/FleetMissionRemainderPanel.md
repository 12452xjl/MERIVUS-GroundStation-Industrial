# 按真实运行观察准备单成员余段子提案

## 514：单成员余段不认领备用出勤的新记录

state.review 与共享 newReview 现在只接受显式 kind=single。foreignGroupContext 的既有内部名称保留，但同时覆盖 group 与 reserve-dispatch：其它当前来源存在时，旧单成员区显示准确提示，不采用、批准、取消或清除它。旧原生 single capture 可能不带 kind，不能简单将所有非 single capture 当外来；原单源捕获合同保持。

独立备用出勤有自己的 [操作面板](FleetReserveDispatchPanel.md) 与 [Root工作流](FleetReserveDispatchWorkflow.md)，仍共用唯一审阅槽和部署区；不会通过旧余段 helper 伪造 Run、按进度跳点或沿用旧批准。此隔离不改变 single 的完整停留、新起返点、执行配置或正常审批门禁。

仅源码与直接静读；未编译、运行、测试或设备验证，隐藏／回调／焦点及三 kind 实际隔离须工程师另验。下方为历史。

## 475：与联合编辑区的实际状态／动作隔离

Root共用当前任务审阅与唯一部署源，但新增kind明确single／group。此单成员区不显示group后台或审阅状态，不取消group requestId，不采用／批准其记录；共享来源为group时提示前往联合区并禁读写本历史单源。联合区通过[新面板](FleetMissionGroupRemainderPanel.md)及独立group provenance进入同一当前审阅／批准门禁，旧single数据形状及算法保持。

Planner将单／组pending互斥，各自排除自己的pending；正常退出仍只清精确本地关联，原Run／占用／归档保持。新组明确采用后唯一部署区展示全部plan.members，不回退旧区域草稿或第一机。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474：整组历史目录复用本单成员编辑／审阅链

嵌入的ArchivePanel新增完整原Run目录与明确成员选择；父面板仍只采用当前history20的精确capture token与sourceVehicleId，不把整包当成多成员联合提案。原索引、新目标／起返点／速度／profile输入保留并重新人工核对，完整停留、原policy下限和当前审阅门禁不变。

一次只保一份有界资料包及一个选中来源；收起／隐藏不等于退出。明确退出或重编辑清精确关联capture后可从同包选择另一成员，不恢复原Run／批准／占用。无需原飞机在线只适用于历史资料读取，不是远端恢复或接管。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：独立历史资料包接原余段编辑／新审阅

面板嵌入[完整规划资料区](FleetMissionRemainderArchivePanel.md)，经[Archive](FleetMissionRemainderArchive.md)显式冻结原生单成员来源并保存；重启后明确打开／选用历史资料。运行报告、brief或summary不够完整，不能作为这种来源。新capture.display包含sourceKind／包UUID／完整包hash／archiveToken及当前性；原native读源仍来自实际selected owner，历史则锁私有包及当前目标scope，不能从历史sourceCurrent推导live资格。

captureAdopted只消费本次Root成功回执的精确token并核历史schema种类／当前性；保留原文字，人工重核新目标、起返点、速度、完整profile及包含式原航点。历史来源使用独立planningSourceToken／planningRunId，不被“必须仍有原Run”错误阻断；点击新的原生读取仍使用实际选定Run。选用后共用原preview／异步review／归档／采用／批准链，新增provenance由中央校验保历史schema2与包关联。

资料区的路径选择／确认和调用计入pendingInput，但本面板inputSettled仍明确排除自己的pending状态，避免自锁。原生编辑在活动时可冻结／保存自己的资料；打开／选用／清资料须先退出当前编辑。历史capture未退出时Root也拒绝换包，只有单槽和单capture，不累积历史完整包。展开不自动IO，不注册Run、不恢复占用或批准，不据progress跳点；原完整停留、时序拒绝及安全政策保留。

收起／隐藏后若Root仍持历史capture，allowLoad／allowUse继续阻断并显示明确退出提示，而非仅凭active=false开放。退出按钮另冻结实际历史captureToken，丢失本地owned token也能精确清理，按下后换源则拒旧按钮。清新审阅时，Root只在同context.proposalToken、同proposal.captureToken及原capture／proposal对象仍一致时清本地来源；旧归档／退休审阅保留，不清不同新来源。由此覆盖“收起／隐藏→再展开→退出→开另一包”和“新审阅→重新编辑／退出→开另一包”，不以丢界面token冒充已退出。

这里只完成源码与限定静态接线；未编译、解析、运行、测试或连接设备。实际文件窗口、往返、来源变化／通知回调与新审阅仍由工程师验证，不代表自愈、正式任务结果或完整执行许可完成；下方为历史。

## 451：后台审阅等待、取消与原来源回调

带clearanceInput的精确私有task通过Root原[后台编译／显式完成链](FleetMissionReviewCoordinator.md)处理。初次操作可返回`planningPending=true/requestId`，`ok=true`仅表示请求已接受，不是最终审阅、归档或批准。`review.planningPending`及准确requestId用于只读显示；后台active期间不重复生成，明确文本说明计算中／取消中／过期。取消按钮可键盘聚焦，按下冻结controller与ID，点击仍同源才调用`cancelMissionPlanning(id)`，保留输入，不取消机载任务。

完成回调不能复用已经结束的原`_fleetRemainderOperation`票据；Root新建本地操作，复核同一私有capture／proposal对象、原requestId及实际部署scope，再沿原归档绑定建立reviewContext。来源已变只更新同一旧提案的过期诊断，不能生成新context或沿用批准。退出本次编辑时只可取消与该余段请求准确匹配的后台任务，不影响新请求。`planArchive.ok`真实提交事实与`planArchiveRecorded`当前绑定可用性继续分开。

UI技能仅用于静态等待反馈、保留文本、可见取消和键盘焦点，没有执行UI、helper、编译或测试。正常完成、忙态、取消／退出、换Run／部署源及归档通知重入仍由工程师验证；原正式自愈／空中接替、起降净空和启动许可不因此完成。

## 435：新记录的地图与逐点／编码核对

新审阅摘要后、采用按钮前增加[默认折叠的只读核对区](FleetMissionRemainderRoutePanel.md)，可在采用前查看新目标机的出航／返程点和实际编码序号。数据是Root由同一实际新计划一次构建的[显示投影](FleetMissionRemainderView.md)，公开审阅值新增第12字段 `view`；不从原Run进度、区域草稿或transportEncoding计数推测路径。

明确采用后，FlyViewMap的规划层显示该余段记录并隐藏旧区域提案层；记录过期仍明确显示历史来源，不自动换回旧草稿。只读地图核对保持规划模式，不开放Goto／Shift命令，也不修改顶点；逐点核对同样不产生批准。地图二维几何不能代替净空、高度／地形、避障或机载实际执行证明。

本批补充434的地图／新路径逐点缺口，不完成正式机载接替或受控启动。下方434“尚未接地图”是历史状态。仅源码／静态核对，未编译、运行、测试或放行；待办见[435工程师交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 434：新审阅记录的采用、批准与共用地面部署

433的捕获／预览／审阅之后，操作员核对**新目标机、record ID和版本**，另行勾选并点击“结束编辑并采用本次新审阅”。该动作只选择后续工作流来源，`needsReview=true`的当前记录也可采用以显示阻断原因；采用不是批准。成功后余段输入锁定、`active=false`，正常解除本面板未结束编辑造成的上传自锁。文本保留，不调用clear销毁新记录。

Root新增深拷贝只读值 `fleetMissionRemainderReview`，公开 `token, proposalToken, accepted, current, approvalAllowed, archive, plan, encoding, review, flightCommandReleased=false, executable=false`。私有上下文保存实际新归档／审阅、精确九字段binding（七个Store字段及inputGeneration、reviewGeneration）、生成审阅时的current-outbound原生scope及退休状态；不保存伪Draft revision，不使用原Run的审批。`current`是新记录与当前部署组／Store／代次一致，`approvalAllowed`还要求明确采用及原 `_reviewAllowsIntentApproval` 全部证据门禁。两者都不是飞行许可。

新增精确token接口：

- `acceptFleetMissionRemainderReview(token, true)`：明确采用当前新记录，不修改Intent批准状态。
- `requestFleetMissionRemainderApproval(token)`、`approveFleetMissionRemainder(token)`：分别复用原申请／批准状态机和证据检查；没有另建批准服务。
- `clearFleetMissionRemainderReview(token)`：取消该来源选择并退休该上下文；仅当仍为它自己的live review才撤旧审阅，不删除归档、不撤另一个新提案、不直接取消传输／飞机动作、不释放原Run占用。

接口沿用433回执形状；申请／批准的`committed`保存实际原生本地状态变化，后置失效不写成回滚。忙态或重入clear明确拒绝，界面保留锁定并提示重新明确操作，不自动重试。清理成功后保留历史token／记录，但accepted=false且current=false，不能再次采用已退休记录。

唯一 `FleetMissionDeploymentPanel` 由Planner传入 `sourceKind, contextToken, approvalAllowed, refreshVehicleIds`及该来源的archive／plan／encoding／current。accepted余段即使过期仍是显示来源，**不静默回退区域草稿**；区域批准按钮在此期间停用。余段刷新目标来自其实际plan.members，区域来源仍取区域draft.members。来源／完整记录身份变化清确认与勾选；区域修复功能仅在区域来源可用。

人工批准之后仍须原当前操作员认证、独立账号上传批准、明确覆盖确认以及原生实际链路／地面未解锁／占用／完整编码／读回门禁。上传会覆盖目标的机载任务；这一步不是空中接替，不启动／解锁／起飞。原机仍忙时不会因“余段”获得占用豁免。

证据刷新和人工输入改变撤旧live review；已采用但过期时保留来源与归档说明，必须明确重新编辑／重新审阅或退出。原观察选择变化可清编辑capture，但不能把已采用的新计划恢复为旧Draft或让它继承旧Run权限；新审阅独立核当前outbound组。关闭／隐藏面板不自动清已采用记录，确认本身不跨隐藏／换controller沿用。

剩余界面缺口：现有FlyViewMap航线叠加仍消费区域`missionDraftPreview`，尚未接独立余段记录的地图航线／新路径逐点审阅。不得把旧区域地图当余段上传预览。此项继续开发，本批仅接新记录的审批和共用地面部署来源，不宣称完整规划／执行产品已交付。

仅源码和限定静态阅读；未运行技能脚本、UI、编译、测试、helper、应用或设备。UI技能采用静态规则指导分步确认、退出、禁用原因、键盘焦点和长ID折行；视觉／交互效果仍需工程师验证。详见[434交接待办](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方433描述为历史，新增的批准／地面部署以本节为准，原取源／规划边界不变。

## 433：原提案入口（历史）

## 实际入口与边界

`FleetMissionRemainderPanel.qml` 是规划页现有主滚动体内的默认折叠区，位于历史草稿恢复入口之后，不是单独演示页。类型登记在 `custom/qgroundcontrol.qrc` 和 Merivus `qmldir`。先通过既有生命周期面板选择实际观察 Episode / Run，再选择原成员，明确读取冻结来源。

本入口只创建单成员独立子提案，并可明确送入现有实际审阅 / 归档链。它不改变原区域草稿、原不可变记录、其他原机、原 Run 或控制权；不做空中接管、不自动挑替补、不发任务写入、ARM、模式切换或起飞命令。沿用当前 Root 的任务身份建立新的记录版本，不承诺创建新 task UUID。

没有可报告的实际 Run、没有符合原生取源条件的目标或取源已过期时，界面显示不可取源 / 错误。导入的复盘 JSON 永远不是此入口的规划来源。历史观察不证明远端任务实例、实时位置、作业完成或当前飞行许可。

## 可操作步骤

1. 结束其他规划输入的编辑，明确点击读取所选成员。读取 / 重读会失效当前提案的旧审阅与批准；旧归档记录本身仍保留。
2. 查看原出航航点，每页最多实例化 8 行，原数组上限 2048。人工填写 **0 基原出航航点索引**；该点被包含，且重做其完整停留，不根据 current / reached 自动加 1。协议序号和原路径点索引分别显示，不混作航点编号。
3. 明确填写新目标 ID、起点和自身返回点各三坐标、水平 / 爬升 / 下降速度。坐标使用 geodetic + AMSL，AMSL 不是离地高度，不自动采用 Home 或实时机位。
4. 提供整份新 `executionProfile` JSON（最多 16384 字符），或留空 / `{}` 仅构建几何提案。不会复制原机执行配置。非空内容由原生整份配置规则校验；巡航及显式升降速率须与请求一致。
5. 明确确认人工裁剪和新输入，再生成几何 / 时间预览。预览说明原 / 新成员、裁剪范围、出航和返程点数、进场 / 移动 / 全部停留 / 返程估算、原 payload 与路线摘要、新路线摘要以及限制。
6. 另行确认，将**私有保存的同一份原生构建 task**送入 `prepareFleetIntent`。实际审阅重新读取当前 FleetRegistry、健康、原生参数 / 版本 / 能量证据，经过原有 Compiler、风险、空域、PayloadBuilder 和 PlanStore。显示真实返回的 record ID、版本、是否记录提交、原审阅回执归档标记（历史）、可行性、需审阅状态及有界原因摘要。原回执标记不代表最终来源尾核通过，更不代表现在仍有批准。

进场是新起点到选定原航点，余段保留到原出航末点，返程是从该末点到新返回点。并非断点续飞或“已经完成工作”的证明；直连接口没有障碍 / 净空认证。时间仅按明确速度及停留估算，不是物理性能保证。历史一致性哈希不认证来源或执行结果。

## 私有来源和回执

Root 新增只读投影 `fleetMissionRemainder`，只公开 `capture` 显示快照、`proposal` 摘要 / provenance、最近 `review` 历史回执、busy / error 与固定非执行标志。投影深拷贝，完整原始任务、计划、payload、实际 QObject owner / scope 和精确构建 task 只留 Root 私有状态。

公开方法为：

- `captureFleetMissionRemainder(expectedSourceToken, expectedRunId, sourceVehicleId)`。
- `previewFleetMissionRemainder(expectedCaptureToken, request)`；request 为原生 exact 9 项合同。
- `reviewFleetMissionRemainder(expectedProposalToken, operatorConfirmed)`。
- `clearFleetMissionRemainder(expectedCaptureToken)`。

每个操作返回 `ok`, `error`, `committed`, `current`, `planArchiveRecorded`, `recordId`, `flightCommandReleased=false`。`committed` 仅表示该步骤本地冻结 / 预览 / 清除的提交，或实际 review 记录已提交，不表示飞行执行。记录提交后若源、对象或新审阅身份失效，保留记录事实并降低 current / ok；异常回执不能被解释为“肯定未写入”。无自动重试或伪回滚。

捕获调用实际选定 `previousStartup.remainderPlanningSource`，复用现有 observation scope、实际 owner 及 Run 稳定身份的首尾核对。后续普通遥测进度前移不改冻结快照，也不强行要求计数永远不变。切换实际来源 / owner 则不能继续提交旧捕获。人工确认绑定不透明捕获 / 提案 token。明确重取来源或重新预览时先退休旧 token，失败也不恢复旧提案；最近已提交记录的历史摘要独立保留。

## 交互与生命周期

余段 active / 正在调用时，Planner 的原区域预览与审批、历史草稿切换按既有未提交输入门禁停用。余段自身使用**排除本面板 pending 的外部输入已结束条件**，避免自锁；Root 正在调用时使用共享草稿操作锁，自己的精确 `prepareFleetIntent` 正向通路不被此锁禁止。

任何人工字段 / profile 更改均丢弃本页提案确认，并经 Planner 既有 `invalidatePreview()` 撤旧全局审阅。此行为不在正常 native 审阅结果发布时自动触发。没有改变区域草稿 canonical 数据，也不将余段拼成虚构区域。

退出始终可达；保留文本但清除本地来源和确认，下次必须明确重取。隐藏、controller 或 observation source token 更换会使本页确认失效。操作持有独立 plain-JS lifetime / ticket，跨同步 busy 通知与结果发布检查原 owner / 选择；旧回执不得接入新面板选择。取消或清理回执未知时明确提示，不把它写成清理成功。退出后可保留最近 review 历史记录摘要，不恢复批准。

使用现有 QGC 颜色 / 控件，纯文本与长标识换行，显式标签 / 单位、禁用显示和键盘焦点。分页后及焦点变化复用 Planner 的现有滚动可见性逻辑，不建立额外轮询或自动执行流程。错误不清除人工文本；数值逐项有界校验，空 / 未知观察不补零。

## 本批检查与工程师验证

仅源码编辑、按真实 producer / consumer 合同的静态交叉阅读及差异空白检查。按照用户限制，未运行 UI 技能脚本、QML、解析器、helper、编译、测试、仿真或设备操作。UI 技能仅用于静态交互、可访问性、有界列表和反馈指导。

工程师后续需验证：真实 selected Run / 成员取源；current / reached 缺失及退休观察；索引 0 与最后点、完整停留；空 / 完整配置；错误重读与失败预览不复活旧 token；输入修改撤旧审阅；取消 / 隐藏 / 换源 / 同步回调；实际新记录提交后失源仍展示提交事实；区域与余段 pending 正向路径；窄屏长文本、键盘顺序、分页焦点可见、深浅主题及错误反馈。上述检查未由本批运行证明通过。

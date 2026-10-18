# FleetMissionDeployment — grounded staging and onboard content readback

## 536：实际NAV朝向参数进入原上传及完整回读

原MissionItem构造前通过共享 [HeadingEncoding](FleetMissionHeadingEncoding.md) 验证param4：NAV显式值必须已经是精确上传tick，原值直接交真实MissionItem／MISSION_ITEM_INT生产；只有明确默认null变NaN。DO_CHANGE_SPEED及起降／HOLD域不扩，不从计划扫描轴、当前heading或回读值生成请求。

普通MissionItem读回保原float32 param4；有限NAV标为absolute-degrees，地理NaN保null／system-default，非NAV有限yaw仍不被悄悄接受。非float32可表示或非有限异常不窄化伪装成合法数值。实际原操作／对象／generation和完整项数尾核不变，再按 [PayloadBuilder](FleetMissionPayloadBuilder.md) 的v2／v3域比较整份内容。

[ReadbackDiff](FleetMissionReadbackDiff.md) 对显式heading使用schema2／top29，有界保原actual p4、规范tick、明确界和逐项结论；语义matched也显示舍入。历史摘要仅保经严格校验的至多64条heading观察，避免新部署后丢掉这些原值；它不是RAW INT证据或当前机载内容／结果认证。全部上传审批、StartPermit、角色、地面和来源门保持，已发送／完成回读事实不被取消回滚。本批未运行、上传、执行helper／parser／算法、编译、测试或仿真；下方为历史。

## 515：新出勤明细已有可见入口，重审仍需重新批准

备用出勤来源提示现在指向其上方准确新记录的逐项协议编码和完整能量／风险明细，而不只显示编码总数。该界面消费Root已有真实Spatial17及同记录review，不从机载缓存、区域草稿或新遥测补历史账单；完整字段显示不是上传或回读收据。

下方共享部署仍再次核实际成员和编码总项数，所有原sourceKind／contextToken／archive、当前批准、角色／账号、独立上传审批、覆盖确认、native fence与原期限不变。用户显式保留提案重审会退休旧采用／本地批准；新的结果必须重新采用和批准，不能凭相同路线或同一来源沿用旧上传资格。

本轮没有修改Deployment C++、wire、权限或实际任务动作。仅源码／静态文字；未编译、运行、测试、仿真或操作设备，最终StartPermit与机载执行仍待开发。下方514“无逐项明细入口”描述是当时历史。

## 514：共享部署明确支持独立备用出勤来源

既有 FleetMissionDeploymentPanel 的 sourceKindKnown 闭集增加 reserve-dispatch；其身份与按下冻结、确认请求仍保实际 sourceKind、contextToken 与同一 archive 七字段，不拿旧区域草稿或其它审阅记录补全。目标列表读取实际新 plan.members，过期来源保持选中且禁用，不自动回退。

本地任务状态必须真实 approved，并同时满足当前审阅、完整编码、原账号／角色、独立上传审批和覆盖确认等原门禁。新子区只负责原本地申请批准／批准，不取代部署区的不同账号审批。采用不是批准，也不自动触发上传、读取、ARM 或 MODE。草稿修复入口仍仅 sourceKind=draft，不能把独立出勤任务改造成旧区域修复副本。

可见来源、只读证据刷新目标和上传确认文字准确标记“独立备用出勤新任务”。上方备用区核完整路线／profile，下方共享区核实际目标与编码总项数；本轮没有该新kind的逐项编码明细界面，不能把Root已有完整编码值说成逐项可见入口。原参数刷新仍撤销旧审阅；新来源不能用“曾采用”恢复批准。实际出勤、原组任务接替、机载改派和最终 StartPermit 均未完成。

本轮仅 QML 接线、文案与合同，不修改 Deployment C++、wire、权限或时限；没有编译、运行、测试、仿真或设备操作。工程场景见 [514交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 482 当前：手动只读刷新明确29项

原部署面板说明与确认勾选改为固定29项：原12＋速率守卫2＋动作14＋独立 `NAV_MC_ALT_RAD` 1；保%1／%2实际来源、PlainText、WordWrap、默认不勾选、原忙锁／确认回调和取消逻辑。profile hash编辑器仍只核旧12，未改成29。没有新增自动刷新、参数写入、起飞或机载命令入口。

Evidence/schema5及Readiness原29快照真实进入两Review、Worker/schema3和新NAV连接环境比较；紧凑UI投影不因此生成许可。旧批准继续失效，不因刷新成功自动恢复。120秒总／4秒阶段／100毫秒尝试间隔及busy不续期不变，多一项不保证容量或鲜度通过。

本轮UI/UX工作仅静态文案与既有语义控件保持；未运行技能搜索脚本、QML、预览、交互／焦点／朗读／翻译／布局验证，也未编译、测试、仿真或刷机。实际路径与StartPermit仍未完成，最终普通文本核对已记录于本批静态审计。下方为历史。

## 480：实际刷新确认28项与动作失效边界

部署面板原只读刷新说明及勾选同步28项：原12、总速覆盖2、起降动作14；保%1／%2实际来源、PlainText／WordWrap、默认不勾选、忙锁及原确认回调，不新增自动请求／写参数／启动入口。旧profile hash编辑器仍显示12，不能改成28。

新动作值经Evidence/schema4完整身份进入原Review／Readiness，缺证和明确动作不兼容阻断；compact投影继续只显示aggregate bool／fingerprint，不生成许可。更长固定采集集合不延长120秒总预算、阶段期限或批准寿命。完整动态净空／最终StartPermit／正式结果尚未完成。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：证据刷新确认明确14项，profile绑定仍是12项

原部署面板刷新范围说明及勾选文字同步为14固定参数，明确原12项加MPC_XY_VEL_ALL／MPC_Z_VEL_ALL；%1来源和%2冻结目标列表不改。仍说明只读取版本和参数、不写／上传／解锁、撤销旧审阅与审批且完成后重新审阅。没有新增自动请求、确认状态、忙锁、来源替代或飞行操作。

UI/UX技能只用于范围明确、PlainText／WordWrap及保留显式确认的静态文案复核；没有执行搜索脚本、启动QML或测量布局、焦点／朗读／翻译，工程师需另验。profile编辑器原12项内容hash文字保持，因为那是不同合同；不把本次14项刷新说成飞行许可或真实速度证明。实际Model／collector／Readiness边界见[原生证据](FleetMissionVehicleEvidence.md)。

本批未改Deployment C++、批准、ACK或实际任务命令；仅两处面板文案和模块说明。未编译、执行或测试；下方为历史。

下方为历史记录。

## 447：原评估与准确归档的阻断风险必须一致且为空

实际 `_gate()` 除原 `needsReview=false/reviewStatus=reviewable` 外，独立消费冻结的 `reviewSnapshot.riskAssessment` 和同 binding 的 `record.reviewEvidence.riskAssessment`。两者都必须是非 null map，通过 [RiskEvaluator 的原生无阻断检查](FleetMissionRiskEvaluator.md)，并具有相同的紧凑规范 JSON 内容。非空阻断原因、缺项／坏型、旧记录无风险评估或同源内容不同均拒绝，原因是 `archived-review-risk-blocked-invalid-or-mismatched`。

实际producer的空QStringList与JSON往返后的空QVariantList都合法，比较先严格核型再用JSON，不把坏类型 `.toList()` 成空列表，也不因原生列表包装差异误拒同一内容。`inputValid=true` 只说明评估输入语法，不能覆盖合法的业务阻断；人工确认、报告可见或标签 `reviewable` 也不能忽略它。

此共用gate覆盖初始准备、后续逐机传输与当前staging复核，不增加回调、重采、自动重试或发送动作，不更改原审批／来源／占用／final native fence。旧阻断记录仍可只读展示，若需重新部署必须生成新的完整原生审阅与归档，不能修改历史标签或清掉原因继续。仅源码与静态对读；未编译、测试、运行或连接设备。完整净空／最终StartPermit及正式机载结果仍待开发。

## 435：余段来源的核对入口提示

共用地面上传面板的旧“余段地图尚未接入”提示更新为实际[新记录地图／逐点核对](FleetMissionRemainderView.md)入口和二维几何限制。只读核对不改变现有 sourceKind/contextToken、精确归档身份、确认冻结、独立账号审批、实际目标或任何原生上传／读回协议；未新增飞行命令或放宽地面未解锁／占用门禁。

显示可用不是批准可用，记录过期保留历史来源；没有自动回退旧Draft或绕过新审阅。仅源码、静态核对；未编译、运行、测试或连接设备。

## 434：共用地面部署界面的显式规划来源

本批只修改QML来源适配，原生Deployment及上传协议不变。Planner保持唯一 `FleetMissionDeploymentPanel` 实例，显式传入区域草稿或已采用的[余段新记录](FleetMissionRemainderPanel.md)：`sourceKind`、本地`contextToken`、对应`approvalAllowed`、明确`refreshVehicleIds`与原archive／plan／encoding／previewCurrent／inputSettled。

面板不再硬编码区域`missionDraftApprovalAllowed`；余段审批资格来自Root对实际新记录的九字段binding、输入／审阅代次、当前outbound组及原证据门禁。已采用余段过期时继续显示该来源并禁用上传，不能自动换回旧区域记录。确认冻结来源／token／归档身份，改变即清旧确认与勾选；区域批准／修复入口在余段来源下不适用。

只读版本／参数刷新按明确来源成员：余段取其plan.members，区域取draft.members；无效或重复目标不退回其它来源。刷新仍撤旧审阅／审批，不自动重审或恢复批准。上传后内容检查和失败对账继续读取actual deployment的原冻结目标，不从新选机或当前草稿替换。

复用原 `requestFleetUploadApproval`、`requestFleetMissionUpload` 和 `confirmFleetMissionUpload`，不重复嵌套原生租约，不新增平行上传服务或飞行按钮。当前身份、独立账号审批、一次覆盖确认、实际地面未解锁／链路／占用／编码／读回要求保持；原Run占用不被余段计划释放。这是源码路径接通，不是已运行／已上传证明；未编译、测试、运行或接设备。完整启动许可、净空及正式机载结果仍未完成。

## 427：失败部署的原始规划输入取源

新增纯值接口 `repairDraftSource(expectedDeploymentId, expectedReconciliationRequestId)`，只读本实例已冻结 `_recordSnapshot.plan.planningSource`，不读取当前 Intent/Draft，不从生成航线倒推区域，不寻找同号替代飞机。实际 Episode 必须仍允许该 Deployment；原部署限 failed/cancelled、所有本地准备/检查/传输/占用/排空均已退出，426私有对账也必须终态且无 pending。

接口绑定原对账 requestId、deploymentId、deploymentGeneration、binding 和完整原目标顺序；unknown/different 不阻止创建新的人工编辑意图，但被原样保留在历史摘要中，不解释为已修复或允许重传。成功固定10键，包含完整规范化 planningSource、原 binding 和当前30键对账摘要；失败固定4键。旧记录没有规划源则明确不可无损恢复，不制造占位草稿。字段和后续人工边界见 [修复草稿说明](FleetMissionRepairDraft.md)。

此接口不创建新的持有服务或发布可编辑草稿，不改原记录、不修改现有草稿、不恢复批准、不发起 READ/上传/清除/回滚或飞行命令，也不要求飞机仍在线。纯值验证器内部的临时 Draft 不发布、不发信号。Episode 检查保留既有原生 legacy 兼容规则；实际产品 Root 入口仍核当前 Episode，不把该兼容规则当成权限。后续编辑与新审阅由独立产品入口完成；仅取源成功不是修复成功。只做源码与静态读回，未编译、测试或运行。

## 426：失败/取消后的新整组诊断 READ

新增 Deployment 私有唯一子对象 [FleetMissionReconciliation](FleetMissionReconciliation.md)，提供独立 `reconciliation` 显示值及显式 request/cancel 包装入口。仅在本实例 failed/cancelled 且原传输排空后，从原冻结 targets/payload 发起新的 `mission.reconcile.read`；不要求旧全量 matched 收据，不调用旧 StartContentCheck，不恢复上传或启动批准。

新 PlanManager 诊断 RAW 结果与普通缓存/StartReview 收据隔离，真正 COUNT0、different 与 unknown 分开；部分单机失败可继续其它可读机，全局来源/权限/飞行状态改变则停并排空。当前30键结果/每机16键摘要、32架/8192项预算与无许可语义见模块契约。本批不复用425普通 decoded diff，不提供修复上传或64行RAW差异明细。

Target 在原上传/回读冻结 primary 时同时保留弱指针控制块；独立对账不静默采用原已捕获链路的替代源。新上传、启动历史准备与检查入口在子任务忙/排空时拒绝；Episode 只增加私有忙检查，不扩张原14服务。原 `current()` 的上传状态/占用/排空字段不混入子任务自身标记，以免新门禁拒绝自己的占用。原部署由 failed 变为 cancelled 会使子任务冻结状态失效并停止；原已 cancelled 时不改变旧 cancel 幂等语义，使用独立 cancelReconciliation。销毁时先清理子对象。

仅改源码和限定静态审阅，未编译、测试或运行。独立对账的历史相同不表示当前机载相同或可启动；修复草稿、跨站控制权等剩余产品工作不由本批宣称完成。以下425仍为普通上传回读的诊断，不是新独立RAW对账。

## 425：实际普通回读的逐项差异诊断

实际 `_read` 完成路径已接入 [FleetMissionReadbackDiff](FleetMissionReadbackDiff.md)。只有原 PlanManager 私有 scoped 完成证据同时确认原 owner/Vehicle/primary、部署/代次/操作号、最终成功、实际 COUNT 和末尾 ACK 已本地排队，才采集普通 decoded `MissionItem` 缓存。失败后清空缓存仍可能发出的旧完成信号，不被当成成功的 `COUNT(0)`；没有可确认的观察时 `readbackDiagnostic={}`，不是“无差异”。

回调在完整门禁前冻结实际对象、目标索引、部署/代次、写/读操作号、载荷和原 primary 控制块，不把 `Target&` 带过回调。门禁后重核原生范围，再采集不超过2048项的 QPointer/字段快照；后置完整门禁后再次核范围、原成功证据、缓存大小、同一项对象和字段未变。取消、对象替换、缓存修改、失效或门禁拒绝不能继续写入 matched/verified，也不换用同 ID 新对象。普通 scoped 协议本身已拒绝超过2048的 COUNT；本层另保留不遍历超限列表的防守分支。

`current().generation` 使用十进制字符串。每个 `current().vehicles[]` 新增 `readbackDiagnostic`：有观察时为固定16键 `schemaVersion/deploymentId/generation/vehicleId/writeOperationId/readOperationId/observedAtMonotonicMs/observationScope/captureState/observedItemCount/comparison/historicalOnly/snapshotOnly/currentOnboardContentsProven/startupPermitGranted/flightCommandReleased`。代次、操作号和单调时间均为十进制字符串；时间来自本次本地捕获，不是远端飞行时间。`observationScope=ordinary-decoded-mission-items-history`；正常 `captureState=captured`，防守超限为 `item-limit-exceeded` 且 `comparison={}`，没有伪造比较行。

`comparison` 为 helper 的有界28键结果，显示最多64个字段差异；真实成功空读、数量不同和 null 项也先留诊断再沿原门槛失败。原非空/数量/完整 canonical items/hash 匹配才置 `readbackVerified`，不采用 helper 的 matched 字段作为授权或替代完整判定；`historicalReadbackReceipt` 和 StartReview 历史入口语义不变。后续失败/取消撤销 live verified，但不抹去已捕获历史诊断。新部署不继承旧目标诊断。

仅当前部署保留差异行；`_archive` 对每个 comparison 使用 `summary()`，终态最多32条历史各保留27键比较摘要，不复制64行明细。摘要中的 displayed/truncated 计数描述原捕获，不能解释为历史仍含这些行。历史快照不是独立 RAW INT 来源证据、当前机载内容证明、任务执行结果或起飞许可；上述三个许可/证明字段始终 false。没有增加 READ、修复写入或飞行命令。

只完成源码接线及限定文本/人工静态审阅，未编译、测试、运行 helper/认证/应用/设备。工程师需验证正常匹配、字段/数量差异、实际 COUNT0、失败清缓存、null 项、回调取消/删除/换源/改缓存、行截断及历史压缩。独立整组重新对账、修复草稿、真实跨站控制权交接及完整路线图仍待开发；本批不是可部署或可飞行认证。以下保留历史增量。

## 420：归档基线观察与实际审阅一致

准备/持续核对沿原实际审阅检查 reviewEvidence 中的 capabilityReviewEvidence，和 nativeVehicleEvidence、transportEncoding 一样必须为 map 且与冻结的实际审阅值相同；不匹配返回 archived-review-evidence-missing-or-mismatched。这替换了原只提 native/encoding 的理由代码。

[新基线归档](FleetConfigurationBaselineEvidence.md)是 Coordinator schema2 的历史观察，不是当前 Registry；本检查不要求所有飞机 currentConfigurationBaseline=true，也不把 captureComplete 当飞行许可。原当前 health、控制器实际快照/审批、来源、占用与最终发送门禁保持，不能从档案恢复旧授权。未编译、测试或运行。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

## 346–350：当前实际 Episode 的原生服务边界

beginUpload、prepareStartContentCheck、_gate的contextUnchanged及_uploadNext/_readback/_resumeDeferred的调用前检查绑定实际Session/UploadApproval同组；虚拟sendHomePositionToVehicle返回后复核原Vehicle/固件插件。普通writeMissionItems/loadFromVehicle保持旧API，不能据caller gate宣称core item adoption、插件后的最终发送或异步每包/重试受Episode fence覆盖；该专用core接口仍待开发。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


## 315–319: independent continuous Run value

The native `inspectExecutionState()` contract remains the existing unbound cache/flight snapshot; it does not manufacture a run or completion proof. `GuidedActionsController.inspectFleetMissionExecutionState()` separately appends `continuousExecutionObservation` only when the actual retained startup owner's new Run value has the same deploymentId. The Run is created from the real transaction/MODE queue, not from this inspection result or imported telemetry. Its private source/queue history, lastObserved sample semantics and local reservation are independent of the old staging approval. No new execution button, flight command, final StartPermit or formal mission result is introduced. Source/static review only; no build, API execution or tests.

## 2026-10-14 Separate fresh startup-content read (234–238)

Native `prepareStartContentCheck(deploymentId)` requires the same completed
runtime history and idle source/transport stacks; it revokes old live verified
staging BEFORE the separate [StartContentCheck](FleetMissionStartContentCheck.md)
freezes its historical context. It neither writes/clears/rolls back the mission
nor restores old intent/upload/start approvals. Own fresh READ operations then
cannot silently turn the old upload approval into current authority or replace
the frozen context. After all read stacks unwind, the new Review independently
binds full RAW INT evidence and caps its deadline at the earliest original READ
START plus 30 seconds. Sequential matching is not an atomic remote lock or
authenticated remote snapshot. No StartPermit, ARM/start, completion or release
is implemented by this step; no build, runtime or test was performed.

## 2026-10-14 Conditional start-intent approval (229–233)

[FleetMissionStartApproval](FleetMissionStartApproval.md) binds this actual
Deployment, its ordered targets and immutable historical full readback basis
to a separately acknowledged native risk review and independent real account.
It does NOT consume upload authority as start authority. The original 30-second
risk deadline covers both acknowledgement and credential entry; no renewal.
Its receipt and consumed conditional grant remain non-executable/no-start/no-ARM,
preserving unresolved risks and historical-versus-live onboard limitations.
No startup workflow consumes it yet. Fresh onboard content, final StartPermit,
authorized transition handling/dispatch and completion remain roadmap work.
Risk semantic revision separately excludes receive counters and only GP boot
clock ticks; full RAW evidence and boot-regression invalidation remain intact.
No source helper, OS authentication, build, runtime or test was executed.

## 2026-10-14 Explicit evidence acquisition (225–228)

[FleetMissionEvidenceRefresh](FleetMissionEvidenceRefresh.md) is a separate
bounded read-only service, not an automatic upload-lease renewal. Controller
copies exact draft target IDs and revokes old review/intent/upload approvals
BEFORE requesting raw version and twelve fixed parameters from frozen actual
Vehicle/ParameterManager/PRIMARY objects. Completion never restores them or
relaxes Deployment's independent live evidence/freshness/identity gates.
New local raw arrival counters do not prove request-response causality or
hardware authentication. Its 120-second historical collection policy does not
replace the stricter subsequent 30-second parameter review policy; large
batches may be stale by the end and must not silently reapprove themselves.
No parameter write, mission upload, startup, ARM, runtime or test was performed.

## 2026-10-14 Finite consumed upload lease (221–224)

The native caller generates the deployment UUID BEFORE consuming approval.
Schema-2 approval freezes an explicitly reviewed maximum 240000 ms upload lease;
schema-2 grant binds that exact deployment ID and the actual native identity
lease. Every gate validates the grant against THIS deployment, both current
account roles and the actual unlocked/active Windows process session. Unknown
desktop evidence denies. Request/confirmation remains 60 seconds; consumption
starts a separate finite upload/readback lease capped by the ORIGINAL primary
five-minute session, not a login renewal. Validators never extend either clock.
Native identity and ledger clock domains are not compared; an earlier ledger
timestamp plus the granted duration conservatively bounds its own deadline.

Different deployment, logout, lock/logoff/disconnect, role or source changes,
explicit revocation and expiry stop further work and preserve drain/no-rollback.
The schema-1 20/8-field receipt/grant cannot be used as schema-2 lease authority.
Neither lease permits startup or proves flight completion. Four minutes is not
an indefinitely running fleet/control lease; future approved renewal and full
startup/completion remain separate roadmap work. Older sections below preserve
their original historical lifetime boundaries. No APIs/build/run/tests executed.

Other live gates retain their independent freshness and transport deadlines;
the human authorization cap does not keep version/parameter/health evidence
fresh, refresh a review, expand the existing fleet protocol, or guarantee that
32 targets / 8192 items finish before it expires.

## 2026-10-14 Native local authentication and independent upload approval

`beginUpload` also takes the actual private `FleetOperatorSession` and
`FleetOperationApproval`, the frozen primary identity and approval receipt.
After complete payload/actual target validation it consumes the native receipt
exactly once; source fences, session/role/record identity and the SAME native
grant are validated at every continuation and final dispatch edge. Caller maps,
role-policy edits or a single overwrite checkbox cannot issue a grant. Native
context uses QPointer across authentication/approval callbacks.

The 60-second lifetime includes the independent credential prompt AND all
consumed upload continuations; validation never renews it. Slow/large batches
can stop and drain without rollback. This is a finite local workstation account
path, not enterprise identity, cross-station control leases, proof of two humans,
or startup/ARM authorization. See [identity](FleetOperatorSession.md),
[approval](FleetOperationApproval.md) and [workflow](FleetMissionOperatorWorkflow.md).
No authentication API, app, compiler, test or actual upload was run in this batch.
The older sections below describe their historical boundaries.

## 2026-10-14 Role receipt, operator UI and historical review source

`beginUpload` now additionally takes the actual local `FleetRolePolicy` and its
frozen action receipt. All nine fields use strict types before native equality;
the owner must confirm its actual private policy instance, and every continuation
rechecks identity/permission/epoch/revision. Missing actor or an outstanding
additional-approval requirement denies upload. A single overwrite confirmation
is not independent second approval; there is still no account authentication.

Only successful complete content comparison creates an immutable in-memory
`historicalReadbackReceipt`. Fail/cancel/ARM still revokes the live verification;
the historical observation is never current onboard certification or a start
permit. Native-only `startReviewContext`/target getters require this current
deployment, a whole matched group, original actual objects and idle/drained
transport. They provide provenance to a NEW independent risk review without
restoring planning approval, changing intent or lifting staging start isolation.

The Planner now includes an operator upload/confirmation/status/inspection
panel. Workflow and explicit missing account/additional approval boundaries are
described in [FleetMissionOperatorWorkflow.md](FleetMissionOperatorWorkflow.md);
the new risk acknowledgement is [FleetMissionStartReview.md](FleetMissionStartReview.md).
These are source-only integrations: no compile/app/render/test/vehicle access,
no start/ARM/mode dispatch or complete task execution/completion proof.

## 2026-10-14 原生启动前置条件与状态只读检查

新增 `inspectExecutionState(deploymentId)`，只读取本实例冻结的 plan/member/constraints/payload 与实际 Vehicle/MissionManager；调用方不能提供遥测、伪造审批或替换目标。检查拒绝上传/排空/协议栈忙、旧 ID、重复/缺失成员和对象/来源变化；通过原生 StartReadiness 与 ProgressObservation 分别输出前置证据和任务项观测。原有 staging 门禁仍必须当前有效，读取结果不刷新、恢复或授予审批。

`startReadiness` 与 `progressObservation` 是只读短时显示快照，最多保留1秒；状态/世代改变或到期返回过期标记并撤去成员事实，timer通知失效。即便在1秒内也必须作为 captured/snapshot-only 解释，实际 dispatcher 仍需重新抓取。没有自动轮询调用 helper；controller 只提供主动检查函数并记录有界状态/数量摘要，完整 snapshot 不复制进32条部署历史。

启动前置证据契约见 [FleetMissionStartReadiness.md](FleetMissionStartReadiness.md)，任务序号对照与未认证完成边界见 [FleetMissionProgressObservation.md](FleetMissionProgressObservation.md)。ARM/起飞会按原隔离撤销 staging；之后只允许查看仍冻结载荷的 raw-state/cache 对照，不证明它正在执行，不复用旧批准。所有启动/执行/完成许可保持 false，仍没有上传/启动 UI 按钮、独立启动事务或完成归属判定。

1秒是原生getter的显示有效窗口；定时器通知仍受事件循环调度延迟影响，不是硬实时物理保护。保存的返回值始终是captured/snapshot-only，不可因为UI还留有旧画面就将其作为当前飞行批准。

配套 PX4 `MISSION_START` 在 Commander 分支尝试解锁，Navigator 独立设置起始序号，接受 ACK 不证明序号已应用。`DO_SET_MODE` 忽略 ARM flag；不得误用此标志作为解锁/禁止解锁证明。本轮没有调用这些入口，仅仅读配套源码核对语义。

This native adapter stages a frozen, human-approved local plan through the existing PX4 `MissionManager`. It is not a mission-start adapter and does not arm, take off, change mode, clear an old mission, or roll back a write.

The v2 payload can include explicit speed, takeoff, waypoint dwell and terminal
hold/land requests. All five supported command types are read back and compared
using command-specific wire identity; a speed item's reserved numeric param4
is not replaced by the geographic default-yaw NaN sentinel. Unknown commands or
frames are rejected by canonical validation, not normalized from expectations.
`verificationScope=fresh-onboard-mission-items-readback` concerns stored items
only; `executionVerified=false` and `executable=false` remain unconditional.
Per-item source/semantic manifests remain only in the one current payload;
terminal history stores counts and bounded unencoded-requirement summaries.
Earlier geometry-only statements below describe the v1 stage, not proof that
v2 commands execute or that mission completion has been observed.

## Native review evidence binding

The immutable archived `reviewEvidence.nativeVehicleEvidence` and
`reviewEvidence.transportEncoding` must both be actual maps and must exactly
equal the full corresponding fields of the frozen native ReviewCoordinator
snapshot. Matching only `needsReview`, a caller-supplied options flag, an
aggregate Boolean or an isolated parameter-profile hash is insufficient.
Reviews without these fields require new native preparation and approval;
archive restoration or manually attaching metadata cannot create live source
ownership.

After version/approval, health, controller authorization, reservation and
actual frozen Vehicle/MissionManager checks, the final gate calls the exact
native coordinator's `nativeEvidenceCurrent()`. This rechecks its private
Vehicle/ParameterManager instances and current source fingerprint. Parameter
content/source/epoch, cache/write/freshness changes and replaced/destroyed
objects therefore invalidate continuation instead of being rebound to a new
source with the same system ID. The gate freezes all backend QPointers, the
binding/record/review, target IDs and deployment generation, then checks them
again after callbacks and native recapture. Reentrant gate evaluation is a
conservative failure; nested begin calls cannot replace a job while this gate
is on the stack.

Native capture completeness/currentness is **not** an assertion that all
parameters are usable. A geometry-only plan without executionProfile may
still be reviewed and staged with explicitly unknown/unusable parameter
evidence, provided the real capture remains current and matches the archive.
Profile-related readiness belongs to native review policy and is not invented
by this transport service. Parameter evidence never promotes cached firmware
metadata, MISSION_INT flags, takeoff paths, landing clearance or mission items
into certified execution.

`current().reviewSourceSummary` retains only the bounded aggregate digest and
capture/encoding status. It contains no twelve-parameter lists or per-item
manifests; terminal history does not duplicate native evidence across vehicles.
`executionVerified`, `executable` and `flightCommandReleased` remain false in
all phases and summaries.

The source summary additionally separates real firmware-observation usability
from local adapter-declaration correspondence. Native review hashes include
the catalog identity and actual version source, so update/revoke/destruction,
first/new raw-version anchoring or source expiry cannot borrow an old approval.
The transport does not authenticate a binary, load a directory, renew version
or parameter evidence, or turn a directory match into startup permission.

## Contract

- `beginUpload` requires the actual native plan store, intent, review, health and Swarm objects, the live controller authorization object, an exact archive binding and explicit overwrite confirmation. It validates the whole vehicle set before the first write and reserves the entire set locally.
- Bindings retain only the seven required scalar archive identity fields and an optional paired input/review generation. Unknown fields, strings above 128 characters, nested metadata and non-finite/non-integral or JavaScript-inexact counters are rejected before retention; terminal history does not accept an unbounded caller metadata envelope.
- Each frozen vehicle must still be the exact connected object, initially connected, PX4 multirotor, disarmed and not flying, with known MISSION_INT support, a valid finite-altitude home and an idle mission manager. Other firmware or local/relative geometry is not silently converted into this adapter.
- Native archive/version, approved intent, review snapshot, live health and controller authorization are checked again at dispatch, protocol callbacks and timer edges. A safety escape can revoke deployment permission without prematurely freeing transport occupancy.
- Continuation gates also require the exact archived native/encoding evidence and current native source binding, including the final native check after authorization callbacks. A source failure stops queued next uploads/readbacks, not a physical rollback of packets already sent.
- Sequential Write operation IDs identify upload ACKs. A queued, separate Read operation then obtains the full onboard list; count and canonical wire content/hash must match. The comparison uses the payload builder's float32/latitude-E7 rules, preserves the system-default yaw NaN sentinel, and excludes mutable current-item flags. An upload ACK alone is not readback proof.
- Dispatch-stack guards defer continuation until the outer `writeMissionItems`/`loadFromVehicle` and PlanManager completion stacks return, including when a direct-connected consumer enters a nested event loop. A queued callback consumed inside that loop is retained as a deferred step, not dropped or repeatedly rescheduled there. The regular 500 ms timer resumes it after `transportCallInProgress()` clears. A deferred continuation has a 120 s deadline; protocol operations have bounded, item-count-derived deadlines. Both use a monotonic clock.
- PX4's existing PlanManager drops the synthetic home and shifts real sequences. The adapter retains/deletes that home itself; route objects are handed over only when an identified Write begins. Rejected calls delete unadopted objects; QPointers avoid deleting objects already freed by synchronous failure.

## Failure and cancellation

Cancellation suppresses queued reads/next vehicles. It cannot reverse an onboard write already sent. `writeMayHaveCommitted` remains true after any identified Write begins. There is no implicit clear, retry-to-another-vehicle, rollback, start or auto-arm. Timeouts and invalidated bindings fail the job but retain its reservation while any frozen mission manager reports an in-progress transport **or** an active transport call/completion stack; the timer monitors this drain. Destroying this service during a transfer leaves local occupancy fail-closed rather than pretending the transport stopped.

Foreign mission-manager operations invalidate a previously verified receipt. Only the current job retains full payloads; history is at most 32 small terminal summaries. IDs are local ground-station correlation, not MAVLink transaction tokens, remote locks or cryptographic attestation.

The verified receipt also remains subject to live archive/intent/review/health/vehicle/native-evidence checks. Write and Read operation IDs are retained separately for audit. Staging writes notify SwarmController's mission-start quarantine: releasing or cancelling a staging reservation cannot make this incomplete payload startable through the legacy start action.

PlanManager establishes QObject ownership for the adopted route objects; deletion of the frozen manager frees them, while the skipped synthetic home remains adapter-owned. Native entry/protocol/finish guards and frozen local write-ACK provenance are source-level lifecycle/correlation defenses, not runtime leak or protocol-token proof. MissionController's separate legacy resume continuation now waits until these stacks unwind and remains bound to the original source/request/manager/content; generated resume writes have no fresh-upload provenance and cannot clear geometry quarantine.

## Deliberate execution boundary

`verified` means fresh onboard **stored Mission items** matched, not that the task executed faithfully. The v2 builder encodes some explicit requests, but actual temporal, vertical-profile, launch/landing and completion evidence remains incomplete. Every state reports `executable=false`, `executionVerified=false` and `flightCommandReleased=false`; the builder's start blockers remain visible. A separately reviewed execution adapter is required before this becomes a real mission workflow.

Source-only development: this addition has not been compiled, run, simulated, tested on hardware or authorized for flight. Engineers must inspect the real PX4 mission acceptance/readback behavior and all cancellation, stale-version, concurrent-operation and ownership paths before enabling a UI entry.

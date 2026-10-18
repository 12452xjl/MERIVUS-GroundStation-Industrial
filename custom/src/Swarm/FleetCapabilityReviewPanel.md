# 本地能力审阅面板与 Root 桥

## 484：第五操作录入工程性能声明

指挥中心和FlyView仍共用原面板，新增approve-performance；旧四操作顺序和签名保持。最大速度m/s、满电续航s分别有可见标签、未知选择和十进制输入，默认两项unknown，不自动继承旧数字／能力。空白、单位、指数、逗号和坏数不被Number转换成0；speed0合法，endurance须>0，至少一known。UI展示实际将提交的规范数字，再同冻结预览核对，能力清单仍必需完整非空。

Root新增专用requestFleetPerformanceReview值接口，经同原scope／quiet／controller-call链到实际native请求；旧requestFleetCapabilityReview仍闭合四操作。canonical两数独立重建，初次native request回调冻结拟context token；所有后续callback、idle／approve和最终请求返回核同来源。输入签名／press／cancel／返回后比对包含两文本及unknown状态；改任一项只取消本页原收据及确认，不接管其它入口。

新preview3/exact20严格分支保两proposal map，旧schema2/exact18及current30／收据5不放宽。界面并列原值和拟值、拟revision／identity、完整能力差异和当前完整基线依据；原生仍是权威。两项默认不勾选确认、默认取消、非Enter提交、原owner/view/ticket及30秒期限保持。普通未提交输入不再逐键向错误摘要抢焦点；已有审阅／调用被输入变化取消时仍明确提示。未知不是已认证0，新增说明不承诺实测性能。

UI/UX技能影响标签／单位／unknown、就地错误、焦点、文字换行、主题及默认取消；未运行搜索脚本、QML、预览、交互、读屏、对比度、翻译或布局验证。runtimeValidated=false／reviewUiIntegrated=false保持。32工程场景全部未执行；仅源码静读，不代表可以直接运行或飞行。下方为历史。

## 422 审批结果与历史摘要状态分开显示

审批结果增加 archiveSummaryRecordConfirmed／archiveSummaryOutcome 的只读展示，未知保持未知。文字明确区分本地登记提交、原证据链追加、独立摘要内存接纳及 SQL 落盘，不因日志失败显示回滚或自动重试审批。

[归档面板](FleetArchiveHistoryPanel.md)由两个宿主页独立挂载，不把录制开关并入能力批准或要求必须连接飞机。默认不持久化；操作员须明确启用，再用当前 run／已封存段查询核对。原账号、自由理由、完整配置不保存在工程摘要；历史读回不重新批准能力或基线。仅源码静态接线，未编译、运行、渲染或测试。

## 421 四操作入口与基线审阅

本节覆盖下方418/419历史中的两操作、非空context及“不改变基线”口径。指挥中心与FlyView区域任务规划页仍共用这一个面板和原Root值接口，不新增可写Registry/Session引用。

操作下拉明确列出 `approve`、`revoke`、`approve-baseline`、`revoke-baseline`；非法索引不会回退为approve。仅能力approve提交非空完整标签清单，其余三项固定 `[]`。基线操作单独显示原生观察epoch/revision、当前完整性、本机完整基线可用性、实际原基线批准的哈希/来源/理由/时间及本次工程理由，不把能力approval14当作基线记录。

Root的同一sourceToken冻结能力与配置两份政策9键；选择操作不会改变该令牌，具体intent只选对应的精确政策。能力操作使用 `fleet.capability.review(.approve)`，基线操作使用 `fleet.configuration.review(.approve)`，审批用途不可互用。修改操作仍作为输入变化立即取消本页原收据和确认，不能把一份已确认的能力审阅改成基线批准。

面板继续验证preview schema2/18键与原比较16键。批准基线必须有当前完整构型和原生after有类型指纹，但首次批准可没有before，不要求比较已知/完整或已有基线。仅撤销基线允许实际context为空、当前构型未知；仍核同一机号、比较身份及实际存储的2–5键本地批准。原记录缺失的ID、理由或时间不补造，界面以当前条目机号关联，缺失的历史理由/时间显示未知；不从报告哈希、旧合并配置或显示JSON补造依据。

合法旧批准的source可能带首尾空白、指纹可能带首尾空白或大写；界面仅在判定时使用source.trim和fingerprint.trim.lower与原生语义对齐，不改写原始DTO、冻结依据或提交参数。完整只读预览保留收到的原记录；界面规范化不是认证或新的原生批准。

默认取消、非Enter提交、强焦点、原30秒期限、两项默认不勾选的确认、原owner/view/ticket与exact收据取消规则不变。基线确认明确旧能力批准/任务依据会失效：同一基线重复批准也须重新审阅能力；撤销只移除本地批准，不恢复硬件/参数、不自动终止已有飞行，不放行新飞行。普通未知/截断比较不被说成“无变化”。

基线原生回执仍为14键，但与能力回执内容不同：含previousComparison、comparison、approval、baselineEvidence及独立stillCurrent。面板保留原生mutationCommitted真事实、原双方账号用途、完整提交回执和审计追加状态；即使ok=false、来源/页面状态变化或后置审计失败，也不清成“未发生”。stillCurrent=false不等于回滚，审计未确认/未知不等于日志不存在，不自动重试。

仅做源码和静态交叉核对，未编译、测试、运行/渲染界面、认证或设备验证。`uiWiringPresent=true` 仅表示源码接线，`runtimeValidated=false` 保留；本机完整有类型基线不保证跨重启恢复，也不是实物/性能认证或飞行许可。

## 419 字段差异展示

现接收schema2/18键预览，新增[原生比较](FleetConfigurationDiff.md)16/13/6/9键值合同。界面明确区分本机完整批准快照、前次不同完整观察和unknown；报告哈希状态不叫实物状态。Missing、invalid、null、类型和前后值分别显示；路径/值转义后只作PlainText。未知和截断保留醒目文字，不以空行列表称无变化。

差异区按需展开，未展开不实例化逐行控件；原生最多128行，每值512单元和整体64 Ki显示预算均有说明。完整DTO查看器仅保证不额外裁剪收到的DTO，不保证原生差异完整。沿用默认取消、两项确认、原收据/作用域归属和键盘焦点，没有运行布局或可访问性验证。合法未知比较不额外阻断本地能力声明审批，清空构型导致原context无效仍按原门禁拒绝。

## 418 实际接线

[FleetCapabilityReviewPanel.qml](../../res/Merivus/FleetCapabilityReviewPanel.qml)进入指挥中心和 FlyView 地图内区域任务规划页，均使用实际 custom Guided 的同一原生 Swarm/Registry。由 custom.pri、qrc、qmldir 交接，非原 PlanView 单独入口。

面板只接值投影 fleetCapabilityReview 和 requestFleetCapabilityReview / approveFleetCapabilityReview / cancelFleetCapabilityReview；不会接收可写 Registry、Session 或 Swarm 引用。原生合同见[双账号工作流](FleetCapabilityReview.md)。

## 操作与显示

输入明确单机 ID、approve/revoke、规范能力标签和理由；不从旧批准静默继承能力。ID是本地协议地址，不是物理身份证明。请求后完整显示当前构型/基线指纹、发现依据/错误、上报能力、原能力声明、当前有效采纳、旧批准和拟变更。

完整配置为 native configurationJson 只读文本，仅用于展示，不保证恢复原Qt类型；无完整基线原配置时不生成假字段差异。速度/续航显示声明值而非实测。审批不会顺便更新构型基线。

预览和提交分步，两项人工确认均默认不勾选；默认取消，危险 Enter 不直接批准。沿用 QGC palette、文字状态、键盘焦点、最小44像素控件及滚动焦点设计。没有运行布局、对比度或可访问性验证。

## 原审阅归属

每个面板只持有本页原5键收据和原 owner/view/ticket。输入、控制器、scope/身份或静态依据变化、隐藏和销毁都只取消原审阅，不接管另一个入口；在原生账号窗口期间仍保留调用忙锁。账号提示耗时计入原30秒；本地倒计时仅显示与保守禁止，不替代原生期限。

Root 的来源回调检查实际原 Swarm/Registry/Session/Role、冻结身份5字段/政策9字段、准确请求输入，以及实际 controllerCall：
- preparing 只能属于原 request；
- idle issued 只能处于 review；
- approving 只能属于原 approve。
申请在 active 清理和 controller-call 结束通知之后还要再次核原收据/state/期限，不能返回已取消的成功预览。

Root 能力值拷贝单集合上限2048，仍保留深度12、65536节点和2 Mi字符限制；原目录调用保持64-map/255-array边界。配置已由原生转为展示文本，不把任意 QObject 序列化成授权依据。

## 结果与审计

本地真实 mutationCommitted、ok、stillCurrent 和 auditRecordConfirmed 分开显示。即便后置来源失效、审计追加失败、窗口关闭或原对象销毁，也不得把已提交变更改写成“从未发生”。保留原双方账号 approvalEvidence，不用提交后另一个账号的显示值替换。

auth consumed=false 不证明批准凭据未消耗，不自动重试。批准、撤销只改变本地工程记录；不代表起飞许可、当前实物真实性或自动改装检测。

uiWiringPresent=true 仅说明源码有入口，runtimeValidated=false；工程师待执行事项见[交接说明](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

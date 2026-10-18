# 原生重启／历史占用恢复复核

## 411–415 覆盖说明：实际控制器来源与专用复核界面

新增 [专用复核面板说明](FleetOccupancyRecoveryPanel.md) 及 [Panel源码](../../res/Merivus/FleetOccupancyRecoveryPanel.qml)，配合 [Guided](../../res/Merivus/GuidedActionsController.qml) 的有界值投影与私有issued收据。本批主任务已接指挥中心和区域任务规划面板两处入口，qrc/qmldir各登记一次；第二处实际由 [FlyViewMap](../../res/Merivus/FlyViewMap.qml) 的区域任务popup承载，不是src/PlanView页面。不因模块存在或接线就宣称界面可运行、可安全审批或完整产品完成。下面406–410保留为当时历史范围，其中“没有QML/仍缺操作UI”及旧形状数量由本节覆盖。

原生新增六参 `requestReviewForController(actualSession, actualRoles, actualAuthorizationOwner, selectedVehicleIds, dispositionReference, operatorConfirmed)`，固定实际Swarm parent/Root授权所有者，并通过五个QVariant参数的 `fleetOccupancyRecoverySourcesMatch(actualReview, actualSession, actualRoles, context, reviewId)` Direct回调前后核实际来源、原current Episode、输入代次与角色。普通忙栈不使本次纯回调自拒，回调不另调OS身份；Root不在来源失效时回落旧五参兼容入口。旧requestReview仍Q_INVOKABLE、没有Root面板/草稿约束，并非C++-only隔离边界；其原生账号与地面证据门禁仍适用。

native current现为27键、常规result为13键，增加 `controllerManaged/authorizationOwnerRetained`，原生 `recoveryUiIntegrated` 仍false。Root对外 `uiWiringPresent=true/runtimeValidated=false` 只表示本批UI桥代码，不能伪装运行验证；workflow额外附加其原issued sourceToken，原生账号、两TTL、整组尾栅和无飞行命令边界不变。Swarm持久诊断现在13键，另显示 `currentUncertainVehicleIds`；不改变journal文件11键。

本页显式申请后再作两个默认未勾确认，并独立审批；按下到点击绑定controller/token/视图及输入代次。完整issued receipt恰8键：`workflowEpoch, reviewId, reviewGeneration, bindingHash, context, targetIdentities, targetVehicleIds, sourceToken`。编辑、换源/账号、隐藏、关闭或销毁精确取消原owner的原收据，cancel-only可在原模态调用忙时作废旧intent，不能接管另一入口或撤回已发生解除。

Root审计接纳结果独立报告 `auditRecordConfirmed/auditRecordOutcome`，区分accepted、not-accepted和acceptance-unknown-after-exception；记录回调失败不抹签发收据或解除事实，异常也不能假称肯定未记录。

仍只解除独立restart/history待审，不能清当前真实unknown、owner、Run、任务或隔离。`localHoldsReleased`保持历史事实，`selectedReviewRequiredVehicleIds`另报当前待审；返回 `persistenceConfirmed=false`，必须等原worker提交新actual占用身份。Root只读 `currentOccupancyCommitted` 也不是远端空闲/租约/起飞许可。后置来源变化或页面失效不得把已发生true抹成未发生或自动回滚。

具体值形状、UI操作与工程师待验证矩阵见 [Panel说明](FleetOccupancyRecoveryPanel.md)。本批仅源码/限定文本审阅，未编译、测试、运行UI/解析/helper/系统认证/设备，未更新PR或打包；完整25目标/六类、真实unknown处置和远端任务/租约等实质工作继续。

## 406–410 历史范围（后续UI与形状增量见上节）

实际 [SwarmController](SwarmController.h) 构造并唯一拥有 [FleetOccupancyRecoveryReview](FleetOccupancyRecoveryReview.h)，只读 `continuityRecovery` QObject属性提供实际原生入口。本批没有新增QML界面、自动调用、账号配置、远端协议或飞行命令；不是可恢复许可。后续仍须接明确选择、处置依据、风险展示和独立确认界面。

| 状态 | 来源 | 本工作流可否移除 |
| --- | --- | --- |
| restart review | 新Swarm的1–255协议地址保守占位；换源后重加 | 仅选中且所有条件满足时 |
| restored historical review | 原worker bootstrap的历史everObserved地址 | 同上；不删除历史ID |
| current uncertain outcome | 本轮发送失败、失联、结果不明或源丢失 | 不可；须另建受控处置流程 |
| live owner/task/quarantine | 实际部署、事务、编队、临时任务、隔离、Run或忙任务 | 不可；存在即拒绝 |

历史ID现在进入独立restoredHistorical集合，不再并入本轮真实unknown；原9键占用快照仍以unresolved-outcome展示，解除必须查实际集合，不能仅从显示行猜来源。

## 实际入口和期限

- `requestReview(actualSession, actualRoles, selectedVehicleIds, dispositionReference, operatorConfirmed)`：固定实际对象、1–32个唯一地址、处置编号和完整依据，不解除。
- `approveAndResolve(reviewId, groundInspectionConfirmed, localReuseOnlyConfirmed)`：专用独立账号认证、一次消费、最终整组门禁，仅解除本地restart/history。
- `cancelReview(reviewId)`：仅取消原审阅，不改飞机、不释放占用、不恢复已消费身份。
- `current`：有界纯值诊断，无QObject/路径/飞行许可；`localHoldsReleased`是历史动作事实，`releaseFactScope`明确含义，`selectedReviewRequiredVehicleIds`另报当前待审，不能用旧true当可分配。

选择地址严格检查后排序为原生Int；重复、小数、布尔、越界、空组和超过32个均拒绝。处置编号非空、无控制/格式字符、规范UTF-8且最多256字节，不裁切。只有一个活动审阅，generation不回绕；preparing阶段不让timer误读未完成字段。

30秒从request开始，包含采集、人工操作、独立认证及后续检查。不同对象单调时钟不直接相减；Session在自己的时钟域分别核原申请/审批身份期限。慢检查不续期，最终纯Session回查也不重新认证或生成长期凭据。

## 原生证据与严格合同

[Evidence](FleetOccupancyRecoveryEvidence.h) 固定原Vehicle、MissionManager、ParameterManager、VehicleLinkManager、MultiVehicleManager、实际model、primary QQ及shared-owner控制块。同ID多对象、替换、换主链路、失源或不在原model均不接受。

原始HEARTBEAT必须明确未解锁，EXTENDED_SYS_STATE必须明确已知ON_GROUND，两路各自年龄不超过2秒。语义revision/hash/value进入稳定身份，普通相同内容收包的receipt/age不进入；坏值恢复的ABA仍改revision。两次读取只作同栈一致性检查，不是同硬件采样、连续观测窗口或物理停机证明，不要求用SYS/EST健康冒充维修复核许可。

capture恰10键：`schemaVersion, vehicleId, componentId, sourceEpoch, versionSourceEpoch, channels, localTransportQuiescent, authenticated, remoteTaskAbsenceVerified, flightCommandReleased`；channels恰HB/landed两条原9键行。stableIdentity恰7键：前五个身份字段及 `heartbeat, landedState`，后二者各恰 `contentRevision, contentHash, value`。摘要只是值一致性，不是签名。

[Vehicle接口](../../../src/Vehicle/Vehicle.h)新增三个只读native方法：TransportQuiescent复用实际正常只读准入，含命令/等待、Mission/Fence/Rally、参数读取/写入及FTP，不发请求；SourceMatches仅核原对象/两源epoch/primary，不要求RX静默、地面或原语义不变；GroundMatches在最终整组尾栅纯核原两RAW语义、2秒年龄、完整非updating、未解锁地面及全部本机队列，不调Link虚函数、OS或通知。

## 专用账号与整组最终解除

[Session](FleetOperatorSession.h)封闭用途只新增 `fleet.occupancy.recover.approve`。operator/safety_officer/commander增加 `fleet.occupancy.recover`，管理员原通配不变；即使自定义角色关闭默认双人标记，此用途仍强制独立账号。未配置或不支持本地Windows认证时拒绝，不启用匿名/单人回退。

绑定摘要覆盖实际身份、角色、完整9键占用context、来源语义身份、workflow/review/generation及处置编号。不同真实SID与合格审批角色仍由原生Session核验；一次消费只能由实际Review私有纯fence接受，烧掉原审批存储，不生成upload/start/control lease。上传、启动、地面重审、ObservationClose身份不可互用；新用途不享retained Close的旧Episode例外。

所有可触发Link虚函数或OS的检查之后，整组调用纯 `finalCurrent`；再核原worker/run/ledger/revision/request/committed identity、原占用指纹、真实unknown/holds与本地忙任务map，最终当前Episode/Role和原双TTL尾核。随后同栈整组移除选中restart/history并bump，无逐机部分解除、中途通知或最后新回调。后机读取不能令先机旧ON_GROUND蒙混通过。

处置编号和地面检查是申请人的明确人工声明，不认证实物。不同SID不证明两名不同自然人。`physicalDispositionVerified`、`remoteTaskAbsenceVerified`、`remoteControlLeaseGranted`、`flightCommandReleased`始终false；不停止、解锁、起飞、取消远端任务、伪造完成或清本轮真实unknown/owner/任务/隔离。

## 提交后、换源与再次重启

本地解除事实先发生；原 [Journal](FleetOccupancyJournal.md)需实际提交新actual身份并读回，既有continuity getter/CAS才重新可用。offer/审批不当committed；本操作结果 `persistenceConfirmed=false`，`resolved-awaiting-current-journal`不宣称后续提交已被本操作确认。

最多255个地址留存原source bundle和两epoch。来源销毁、MVM同ID替换、primary控制块或epoch变化重新加restart hold，公开getter/最终CAS当场核，不只等100ms轮询；同源正常收包、后续合法ARM/飞行/MM工作不冒充换源。当前状态与已发生的解除事实分开，不抹历史来伪装未发生修改。

Swarm留存诊断现在12键：原10键外增加 `historicalReviewVehicleIds`、`recoveryUiIntegrated`；`recoveryWorkflowImplemented=true`仅指native入口存在，UI仍false。journal文件11键、everObserved单调历史、runOpen/reviewRequired和durable/osSync/完整覆盖false不变。不保存可恢复权限或永久free；每次新Swarm仍从全部地址待审开始。

原Land/RTL/ABORT不新增磁盘等待或恢复审批前置条件。atomicReplacement不当OS sync、durable、完整捕获或write-ahead证明。

## 未完成范围和工程师待验证项

仍缺操作UI、离线/报废旧机现实处置与控权回收、本轮真实unknown受控解除、远端任务实例/租约及正式机载自愈。永久失联旧成员不能凭此工作流绕过old∪new门禁。完整25目标/六类实质源码工作保持，不称全部恢复产品完成。

工程师后续须验证：Qt/moc与类型接线；账号取消/超时/锁屏/注销/撤权；最后源回调切Episode/角色；后机检查使先机HB/landed失效；源毁/同ID替换/PRIMARY ABA；正常RX/起飞/MM不误重hold；所有真实unknown/owner/task/quarantine拒绝；缺坏账本、锁竞争、写失败和延迟commit不先开放；旧review不可重放；原安全指令独立。以上仅为待执行清单。

本机只改源码、工程登记及文档，并作限定文本读回。未编译、测试、运行解析/helper/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包；不作可安装、可运行或飞行安全结论。

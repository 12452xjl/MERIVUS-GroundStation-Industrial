# 重启／历史本地占用专用复核面板

## 411–415：界面与实际原生所有者接线

[FleetOccupancyRecoveryPanel.qml](../../res/Merivus/FleetOccupancyRecoveryPanel.qml) 是独立的 `FocusScope` 和 Controls 2 `Dialog`，只消费 [GuidedActionsController](../../res/Merivus/GuidedActionsController.qml) 的有界值投影与专用方法。它不直接取得 Swarm、Review、Session 或原生 scope，不新增飞行命令，也不以草稿、上传权限或一个 caller 布尔值授权解除占用。

本批主任务已在 [指挥中心](../../res/Merivus/CommandCenterOverlay.qml) 和 [区域任务规划面板](../../res/Merivus/FleetMissionPlannerPanel.qml) 各接一个入口，并登记 [qrc](../../qgroundcontrol.qrc) alias 与 [qmldir](../../res/Merivus/qmldir)。第二入口的实际宿主链是 [FlyViewMap](../../res/Merivus/FlyViewMap.qml) 的 `missionPlannerPopup.contentItem: FleetMissionPlannerPanel`，不是 `src/PlanView/PlanView.qml`。两个入口不受草稿有效性、preview或提案结果控制，但仍受原宿主整体显示规则约束；接线不等于目标 Qt 环境的可加载、可用、可访问验收。

可解除范围仍是 [原生恢复复核](FleetOccupancyRecovery.md) 所述的独立 restart review 与 restored historical review。当前真实 uncertain outcome、所有者、Run、任务、编队、临时任务和隔离不由本页解除。1–255 的初始待审集合是协议地址限制，不是发现了255架飞机、255个故障或可全选的真实机队。

## 用户操作与界面门禁

1. 明示按钮只打开视图。默认焦点是取消／关闭，不自动登录、创建 review、重试或审批；本页不是紧急停机界面。
2. 读取在线地址、重启待审、历史待审、当前真实未知、所选仍需复核地址，以及原生原因与账本线程状态。未知不补为空列表；在线地址只是候选提示，不能代替实际 Vehicle/model/PRIMARY 的原生绑定。
3. 申请人可显式调用既有 `authenticateFleetOperator()`。返回后仍核原 controller、来源 token、页面代次和 lifetime；认证不自动触发下一步，也可能撤销当前尚未消费的批准。
4. 手动输入1–32个唯一地址，英文逗号分隔，每个为1–255整数；本页拒绝空项、重复、前导零和越界，并排序为数值列表。不能选择不在线或在当前真实 unknown 集合中的地址。实际唯一对象、地面遥测、队列与占用门禁仍在 native。
5. 输入非空人工处置／检查编号，最多256个UTF-8字节。不自动裁切或修剪；本页先拒首尾空白、控制字符、常见格式字符及无效代理对，原生再完整检查控制／格式类别和UTF-8往返。
6. “申请所选地址专用复核”只创建有界 review。创建后两个复选框均为 false：一项声明已完成现场地面检查但不是传感器或实物认证，另一项确认仅本地待审、不证明远端任务不存在、控制租约或起飞许可。
7. 只有本页拥有的完整原收据仍匹配、原期限未过且两项明确确认后，才可请求另一实际账号审批。不同SID、角色、专用用途、一次消费及最终整组解除全部由 native 决定；不同SID也不证明两名不同自然人。

危险按钮不接受默认 Return／Enter 提交；取消按钮支持明确键盘取消，视图内容提供 Escape 退出路径。控件有标签、焦点边框和换行文本，错误有字段提示及可聚焦摘要，焦点会尝试滚入可见区。这里描述源码设计，不宣称实际键盘、屏读、对比度或布局已验收。

## Root 公开值投影与方法

`fleetOccupancyRecovery` 公开值字段包括：`sourceKnown`、`sourceToken`、`busy`、`workflow`、`persistence`、`availableVehicleIds`、`operatorIdentity`、`currentOccupancyCommitted`、`uiWiringPresent`、`runtimeValidated`、`flightCommandReleased`。实际 QObject 与22键 current scope留在私有桥内，不作为面板数据导出。

- `workflow` 来自原实际 Review 的27键 current，并由 Root 为匹配的私有 issued receipt 附加其原 `sourceToken`；不是根据 reviewId／UUID 搜索另一个工作流。
- `persistence` 是当前实际 Swarm 的值诊断，包含 worker、bootstrapAccepted、stickyFailure/errorCode、restart/historical/currentUncertain 三组地址等。worker的旧 atomicReplacementConfirmed不当新当前提交。
- `operatorIdentity` 只展示 authenticated/actorId/role。来源 token 使用原 scope、Swarm/workflow epoch、输入代次和原身份的稳定 sessionId/sessionEpoch/sessionRevision/actorId/role，不把普通 busy、显示剩余时间或自然收包年龄变化当新身份。
- `currentOccupancyCommitted=true` 只描述本次读取的实际占用身份已被当前原生提交匹配，不认证远端空闲或实机处置，也不授控制权。
- 原生 Review 与 Swarm 的 `recoveryUiIntegrated` 仍为 false；Root 的 `uiWiringPresent=true`、`runtimeValidated=false` 只标识本批接线代码存在且未运行验证，不能改写原生安全结论。

公开方法合同如下；失败可能返回较短错误DTO，缺失事实不得补为 false 或 true：

```text
requestFleetOccupancyRecoveryReview(ids, dispositionReference,
    operatorConfirmed, expectedSourceToken)
approveFleetOccupancyRecovery(reviewId, groundInspectionConfirmed,
    localReuseOnlyConfirmed, expectedReceipt)
cancelFleetOccupancyRecoveryReview(reviewId, expectedReceipt)
authenticateFleetOperator()
```

成功申请返回平铺的 native current 字段，加 `ok=true`、`nativeRequestAccepted=true` 与 `sourceToken`；`remainingReviewMs` 本身属于 current，不是新期限。私有issued发布、审计通知后和外层调用真正结束后分别再核原完整收据、scope、输入、状态与剩余期限；若途中失效，保留已创建事实和完整收据但返回ok=false，不把旧review伪装为当前成功。最后通知销毁Root时仅修改已持有的纯值结果，不调用死对象。审批保留 native result 已发生事实并附加原 sourceToken。取消是原 private issued receipt 限定的 cancel-only 路由，可在原调用忙或来源 token 已变时取消原审阅，不进入另一组普通调用、不取得新权限、不撤回已发生解除。

Root另把审计记录接受状态与申请/解除结果分开：`auditRecordConfirmed`、`auditRecordOutcome` 分别描述明确接受与 `accepted / not-accepted / acceptance-unknown-after-exception`。记录观察者拒绝或抛错不得抹掉已签发完整收据或已发生localHoldsReleased；异常也不证明日志肯定没有记录。尚未发布到本页的native申请可保留 `nativeRequestAccepted/nativeReason`，但不能伪装成本页可消费的成功review。

## 六参原生入口、五 QVariant 回调与精确形状

[Review.h](FleetOccupancyRecoveryReview.h) 的控制器入口为：

```cpp
QVariantMap requestReviewForController(QObject* actualSession, QObject* actualRoles,
    QObject* actualAuthorizationOwner, const QVariantList& selectedVehicleIds,
    const QString& dispositionReference, bool operatorConfirmed);
```

Root 使用实际 current scope Session、实际 shared RolePolicy、Root自身和原输入调用；Review 固定实际 Swarm 与其原 parent/authorization owner。旧五参 `requestReview` 仍是Q_INVOKABLE兼容入口，没有Root的面板/草稿约束，并非只能从C++调用的隔离边界；原生账号、双人审批和地面证据门禁仍适用。产品Root不在控制器来源不满足时回退它。

[Review.cc](FleetOccupancyRecoveryReview.cc) 通过 `Qt::DirectConnection` 调用原 authorization owner 的 `fleetOccupancyRecoverySourcesMatch(actualReview, actualSession, actualRoles, context, reviewId)`。五项均为 `QVariant` 参数，前三项封装实际 QObject 指针，后两项是原 context map 和原 reviewId；返回必须是实际非空 Bool true。回调前后核原QQ、代次、状态、输入、当前Episode及角色；Root核私有 active/issued 与原 scope，不查询OS身份、不另取普通controller lease，也不因本次正常busy而自拒。

常规 native `current` 恰27键：

```text
schemaVersion, workflowEpoch, reviewId, reviewGeneration, state, reason,
targetVehicleIds, dispositionReference, context, targetIdentities, bindingHash,
remainingReviewMs, applicantActorId, approverActorId, groundInspectionDeclared,
localReuseOnlyConfirmed, localHoldsReleased, releaseFactScope,
selectedReviewRequiredVehicleIds, controllerManaged, authorizationOwnerRetained,
currentUncertainOutcomesCleared, physicalDispositionVerified,
remoteTaskAbsenceVerified, remoteControlLeaseGranted, flightCommandReleased,
recoveryUiIntegrated
```

常规 native `result` 恰13键：

```text
ok, reviewId, state, reason, localHoldsReleased, controllerManaged,
authorizationOwnerRetained, currentUncertainOutcomesCleared,
physicalDispositionVerified, remoteTaskAbsenceVerified,
remoteControlLeaseGranted, persistenceConfirmed, flightCommandReleased
```

原生错误／所有者销毁分支可能无法返回完整形状；不得把短错误当成功 current 或完整收据。`reviewGeneration` 为不回绕的十进制字符串，空工作流诊断可为"0"，已发收据必须为正数。`controllerManaged/authorizationOwnerRetained` 只是实际原生所有权诊断，不是新的授权凭据。

初始或尚未审批时，两个actor显示值可能是invalid QVariant，Root在固定27键shape核对后仅将这两个null/undefined显示值规范为空字符串。其它字段仍严格复制/校验，不以空actor建立身份或把未知证据转为空证据。

本页拥有并回传的 `expectedReceipt` 恰8键，不能用当前投影、reviewId单值或另一入口的review替代：

```text
workflowEpoch, reviewId, reviewGeneration, bindingHash,
context, targetIdentities, targetVehicleIds, sourceToken
```

`context` 仍恰9键，绑定当前原Swarm占用identity4、原worker/run、ledger epoch/revision、committedRequestId和本次目标；每个 `targetIdentities` 项仍为原生7键稳定来源身份，含原HB与landed的语义revision/hash/value。Root严格核完整形状与私有 issued token，普通JSON或缓存bool不能自行建立 issued/native owner，也不能恢复跨重启批准。

## 每页所有权、取消与模态重入

每页只保存自身 controller、完整8键收据、来源token、输入/视图代次和 plain JS lifetime。按下按钮时冻结这些值，点击时再次比较；来源A→B→A、地址／编号编辑、controller变化、隐藏、aboutToHide、视图或面板销毁都会立即丢弃本页拥有关系、取消原 owner 的原 issued receipt并清两项确认。不是等关闭动画结束或系统提示返回后才作废；另一入口不能接管、消费或取消本页以外的收据。

原生请求可能在返回前使页面失效；局部纯值 closure 在返回后只放弃原返回收据，不读取已毁 QML 对象。审批/认证用原 owner和完整同步调用ticket，`callBusy`贯穿系统modal及尾部通知；plain activeCall直到最后QML属性通知后才清，防重入启动第二次操作。busy不把旧收据换成当前alias，cancel-only仍可立即作废原 private intent。

Root 同时冻结原 current Episode scope、draft/input/review代次与实际Review/Session/Role来源；输入变更或current退役可使原intent失效，不依赖面板一个UI判断。native回调不把这些值当身份认证，实际账号与原双TTL仍由 Session在其时钟域检查，最终整组原生尾栅后才能解除本地集合。

30秒从原生request开始，包括采集、人工确认、系统审批和最终重查。UI用最早request墙钟及返回remainingReviewMs保守显示，扣除请求耗时，不在通知或凭据返回时续期；显示时钟回退或到期会取消本页旧审阅。最终许可不是这个倒计时数字。

## 解除事实、当前待审与持久提交分别展示

`localHoldsReleased=true` 与 `releaseFactScope=historical-local-mutation-not-current-availability` 只描述已经发生的某次本地修改。后置页面变化、source失效、日志回调或调用尾部失败不能把真事实抹成“从未发生”，也不自动回滚／补释放；本页保留返回结果用于历史提示，不将旧controller/旧review的true归给当前空闲。

`selectedReviewRequiredVehicleIds`另报所选目前仍需复核的地址。源被替换后原生可重新加restart hold，历史解除仍true；当前真实unknown集合始终不被此工作流清除。

审批结果的 `persistenceConfirmed` 始终 false。解除后新actual占用身份必须由原工作线程实际提交并读回，native getter/CAS才按原规则重新可用；offer、旧committed、旧atomicReplacement或UI成功提示都不能提前替代。界面另显示 `currentOccupancyCommitted` 的当前读取结果；即使true，仍不证明失电持久、完整捕获、远端任务为空或有飞行许可。参见 [Journal](FleetOccupancyJournal.md) 与 [本地占用证据](FleetNativeOccupancyEvidence.md)。

## 工程师待验证矩阵（本批均未执行）

| 范围 | 必须实际验证的场景与预期边界 |
| --- | --- |
| Qt集成 | 完整累计工程编译/moc、Controls2 Dialog与导入版本、两宿主入口、qrc/qmldir各一次、实际parent及销毁顺序；不能仅拷本批文件构建 |
| 原生方法绑定 | 六参ForController实际QObject与数值列表传参、五QVariant Direct回调、返回非空Bool、current27/result13/receipt8往返；缺字段/错误类型不得放宽通过 |
| 双入口归属 | 一页申请，另一页不可接管/审批/取消其review；关闭原页只精确取消原issued，不能取消新review或另一工作流 |
| 输入 | 空白、重复、0/256、33个地址、小数/布尔/字符串伪ID、前导零、UTF-8边界、无效代理对/控制及格式字符；错误不自动修剪、提交或重试 |
| 手势与键盘 | 按下后切源/编辑/隐藏再松开不得提交；默认Cancel，危险Enter不提交，Esc/明确取消、键盘空格、焦点可见且不被滚动遮挡 |
| 模态与销毁 | OS提示期间编辑/关页/Root销毁/原Session或Review销毁，原intent立即失效；旧返回不读死QObject、不清新call、不自动重试 |
| 账号与期限 | 显式登录、错误/取消、双SID、角色修改/撤权、注销/锁屏、主身份或审批TTL在尾部到期；不重用Close/upload/start凭据，不生成lease |
| 原始来源 | 原Vehicle/MM/参数管理器/model/PRIMARY换源、同ID替代、bad→good ABA和过期HB/landed；后机回调使先机失效仍整组拒绝 |
| 解除边界 | 只所选restart/history同栈整组解除；真实unknown/owner/Run/task/formation/temp/quarantine有任一不符即拒绝，不逐机部分放行 |
| 后置事实 | 本地解除后发生页面/源/记录回调变化，仍保留历史true，不声称未发生或当前free；selectedReviewRequired重新出现可独立显示 |
| 持久化 | 新身份延迟提交、锁竞争、缺坏账本、写失败、worker换代/停止；旧ACK不开放当前CAS，不把atomic replacement当durable/OS sync；安全Land/RTL/ABORT不等本审批或磁盘 |
| 可访问性与布局 | 实际明暗主题对比、字体/DPI/窄窗口、长ID/文本换行、Tab与屏读顺序、错误摘要焦点、scroll可见和触控；当前仅静态规则不作视觉验收 |

实现沿用项目QGC主题/控件与ui-ux-pro-max静态交互原则；没有运行skill搜索脚本、QML解析器、应用、OS认证、业务/helper、编译/测试、SQL、仿真或设备。未改PX4、刷机、更新PR或打包。

该面板没有补齐当前真实unknown受控处置、离线/报废旧机现实控权回收、远端任务实例/租约、完整长任务与正式机载自愈。完整 [25项目标及六类后续工作](../../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) 保留，不称全部完成或达到可飞安全放行。

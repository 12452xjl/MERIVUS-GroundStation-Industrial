# 有界实际启动事务序列

## 346–350：当前实际 Episode 的原生服务边界

acquireConsumer、consumerBasisCurrent及其前后owner集合核实际Authority/Txn同组，已分配child只在原实际basis下复用。setInitialTransaction保留一次严格managed构造绑定：actual Episode getter槽/owner/parent/pristine状态匹配，不要求尚未发布的组为current；legacy路径必须双legacy。child先继承sticky来源、登记为真实owner后才接受current pair；current/previousStartup/consumerIsCurrent仍是被动身份或历史，不授新请求。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


## 340–344：同部署 Series 与下一部署 Episode 分开

[Lifecycle 原生核心](FleetMissionDeploymentLifecycle.md)每代创建新的实际 Series、初始 Txn 和其余 owned 对象（共14个，含 signed-out Session）；Series 一次绑定本组初始 Txn，原同部署最多8个 ground child 约束不变。新部署不是把旧 Series/generation 清零，旧完整实际组/Run/占用继续保留。

全部 retained Series/Txn/Run 纳入固定18键纯 native 静默证据；Series 证据丢失拒绝而非 fallback，旧 Run 可继续观察，不要求关闭监视才构造不重叠新任务，也不据此宣称落地。工厂总8次分配含初始/失败尝试，不驱逐历史或新建 Orchestrator 绕容量。

**未注册 QML、未替换 Guided、无新部署入口，应用仍单链。** sticky outbound helper 只是核心，Series/owned 继续点门禁、QML actual Episode/epoch 和 selected retained Run/Series/Close/Session 同组投影尚待接线，不能将当前 Series 与历史 Close/Session 混用。限定检查结果已记录；未编译、测试、执行 API/helper/认证/应用或设备。

## 335–339：两个固定 UI 入口只认本 Series 当前 Run

[FleetMissionObservationPanel](FleetMissionObservationPanel.md) 已在 CommandCenter 左栏和 Planner 标题下、主草稿滚动区之外各接一个固定入口，不随草稿/预览、upload 权限、选机或飞机在线状态隐藏；原父界面尺寸/折叠规则不变。Guided 的当前 Run 值现在只取实际 `previousStartup`；返回空时保持无当前来源，不退回初始 Transaction、旧前代或同 ID 替代对象。UI 显示的 Run 与 review 不能恢复或改变 Series 实际 owner。

每个面板以 controller、lifetime epoch、review UUID/workflow epoch/generation/完整 context 组成自己的交互范围，不接管另一个入口建立的审阅。取消/关闭/隐藏/销毁或切换 controller 仅尝试取消本页仍精确匹配的 review token，不取消别人当前审阅，不结束监视、不停飞，也不声称回滚已提交本地效果。三项确认每次新审阅默认未勾，controller/token变化或本页审阅匹配失效后清空；弹窗默认关闭/取消焦点，无默认接受，危险批准按钮禁止 Enter/Return。

原生两账号/专用purpose、最初30秒期限、context18/完整binding7或9/原成员和实际占用、排空与一次消费门禁不因 UI 接入改变。UI 剩余时间只作保守展示，不能验证来源、恢复旧批准或续期；结果保持已知释放、已知仍占用、unknown/null 三态，不把 `ObservationClosed` 写成 `Completed`、STOP/LAND/DISARM、停飞或远端交接。此节覆盖330–334无UI的历史状态；没有编译、测试、执行脚本/认证/业务接口/helper、启动应用或渲染，长期生命周期和真实结果归因仍待开发。

## 330–334：只从当前实际前代结束本地监视

330–334接线的独立 [FleetMissionObservationClose](FleetMissionObservationClose.md) 后端当时没有操作 UI；后续335–339入口见上节，仍未编译、测试或执行认证/业务接口。它仅从本实际 Series 的 `previousStartup()` 获取当前 Transaction，绝不 fallback 到初始 Transaction、旧前代或相同 UUID 的其他对象；入口 ID 只是对当前实际对象的断言，不是 owner 选择器。最新来源丢失、series 证据粘性失效或实际对象替换时拒绝，不恢复历史权限。

流程私下冻结实际 Session、RolePolicy、Series、Transaction、authorization owner 和实际 Run/Swarm/Orchestrator 的 QPointer/线程；保留原 startup/run/task UUID、epoch/generation、deployment、完整 7/9 键 binding 与原有序整组成员。私有 context 恰好 18 键，额外包含 `localReservationState`：`run` / `startup` 各为精确三键 `{owner, occupiedVehicleIds, authorityRevoked}`。快照检查实际 Swarm map 中该 UUID 的全部条目，成员必须是原组子集且两 owner 不重叠；整个快照参与冻结审阅/hash。`authorityRevoked=true` 与 `occupiedVehicleIds=[]` 是不同事实；有效授权已拒绝时，本地占用仍可能保留，不能从 `missionDeploymentOwns=false` 宣称不存在占用或已释放。

结束监视采用独立 `task.observation.close` 权限与 `task.observation.close.approve` 审批 purpose，必须保留当前真实两账号/不同 SID 和第二审批策略。最初请求固定的 30 秒单调窗口覆盖人工审阅、原生来源检查和凭据弹窗，不因重试/回调续期；最终分别确认结束监视、仅释放本次本地占用、飞机可能仍继续飞行。旧 upload/start/ground 批准不能代替新流程。Source/遥测失效可保持 unknown 并允许结束自己实际 owner 的监视，但原启动必须终态且发送/native transport 栈排空；不认证或重建缺失来源。

Session 一次即时消费后，实际 Transaction 沿原 Run 退休及旧 owner 排空路线处理这两个确切 UUID，不触碰其他 owner。后置 disposition 从实际 map 再核占用；对象丢失、缺值或形状错误继续 unknown/null，不把监视已经退休误说成占用已全部释放。Orchestrator 的退休匹配独立于 MODE queue admission，观测计数到最大值仍可 mandatory close，保持最大/exhausted、不回绕或捏造新唯一版本。任务只进入 `ObservationClosed`，不是 `Completed`，没有 STOP/LAND/DISARM、飞机停飞或远端控制转移保证。

本节覆盖较早批次关于独立关闭审批尚未实现的状态：330–334完成后端源码接线，335–339补上UI入口但尚未验收；Series 本身仍不认证账号、不审批、不发飞行命令。真实结果归因、长任务/留存、新部署重置/容量管理、跨站租约及原路线图其它实质目标仍未完成。

## 325–329：同实际 Swarm 的任务模型来源

初始及后续每个真实 child Transaction 创建的Run，均在prepare从原实际Swarm取得其一次关联的Orchestrator，登记同一模型的独立protected观察任务。Series不接caller taskId、假Run摘要或新的任务JSON，不借编队activeSwarmTaskId；完整binding仍保留7或9键。MODE排队前缺真实模型/容量/成员依据则拒绝，不在已发送后补造成功任务。

Run在旧启动认证退休后继续更新观察任务；晚到commit保留历史，监视退休/析构只进入ObservationClosed，不能用Series清理宣称Completed/已停飞或复活旧认证。若MODE全部明确未发出，原Txn终态且发送/授权/协议排空后才私有关闭unused准备，释放其模型占用供新的地面重审；未知或部分MODE绝不自动走此路径，新重审也不忽略未退休旧Run。观测通知独立于结构revision，普通continuity/reassign/complete等不可改写这个private owner记录。后续授权清理/长任务/新部署生命周期与正式结果归属仍未完成，本轮未编译/运行/测试。

## 315–319：与连续运行实例的关系

每个实际 Transaction 现在明确一次使用，不能在旧终态上重置 generation/覆盖运行事实。后续地面再审继续使用本 Series 已有的最多8个独立 child，而不是再次调用旧对象。新增连续 Run 为各自 Transaction 的独立子对象；启动终态/旧 reservation 已释放不表示 Run 已结束，controller 从 `previousStartup` 取得它的只读 executionObservation。任何 MODE 已排队/不确定后的地面重审禁令不变，不能借新监视器恢复旧审批或在飞行中套用地面路线。

该 Run 的原子整组本地占用使用另一 UUID，旧 Transaction/Series 清理不能凭旧 UUID 删除新占用；运行对象销毁也不等于飞机任务结束。正式任务生命周期、容量/新部署切换和安全交接仍待原路线图后续，不把本轮源代码接线当部署验收。未编译、测试或运行。

`FleetMissionStartupSeries` 只保留实际 native owner 并为后续每轮地面再审分配独立 `FleetMissionStartupTransaction`。它不发 MAVLink、取证、审批、ARM/MODE/DISARM、重试或回滚，不产生最终启动许可。本批仅源码和限定静态契约，未编译、运行、测试、仿真、调用 Windows/设备接口或刷机。

## 实际 owner 与保留规则

- `initialTransaction` 仅一次接受同线程 actual Transaction；不可用任意 QObject、ID、JSON 或快照替代，不接管初始对象所有权。
- `consumer` 是最新实际 ground child，初始为空。`previousStartup` 是最新已有实际非空 startup UUID 的 owner；未初始化 child 不代替前代。任何 owner 来源丢失后不回退更早对象。
- 每个已初始化 owner 的 startup UUID、Transaction epoch、generation、source hash 固定；发现重用/清空/替换，粘性 `evidenceLost` 阻止后续分配。第一次分配冻结原记录 binding、deployment 与有序整组 ID；不能用新范围覆盖此序列。
- 最多保留初始 owner 加 **8 个 ground child**。容量满明确拒绝；不淘汰、删除、归档重建或自动换 deployment。当前没有“新部署重置”入口。
- 实际 owner 销毁会保留其最后脱敏快照并粘性阻止分配。即便看似空候选丢失也保守停用：初始化可能发生在首个通知之前。绝不凭旧 owner 或数值 ID 恢复授权。

## 显式消费者工厂

`acquireConsumer(actualGroundAuthority,startupId,operatorConfirmed)` 只在明确确认下读取 actual Authority context27 并调用实际 full gate。实际两身份、原回读/审批期限及来源门禁继续由 Authority 核对；工厂不创建新期限、凭据或占用。

工厂要求全部旧 owner 已终态、无 active/reservation/drain/preparing；必须以 actual `nativePreviousStartup` 对象和原 UUID/epoch 对上最新 owner。最新 owner 已有 MODE 入队或不确定提交事实时拒绝，不能跳过它选早期 owner。任务 binding、deployment、有序整组 ID 必须与初始系列一致。

尚未初始化的最新 child 只允许同一次 actual Authority/UUID/完整 stable context 的精确复用。原期限只缩短，不接受新账号、新审批或新 basis 替换它。另一真实新 basis 通过完整门禁后分配**新的 child**，旧空对象保留并继续计入8个容量；旧 basis 过期/撤销或首次初始化前拒绝不能导致永久覆盖旧事实。已初始化的 child 永不复用；后续一轮分配新的实际对象。

`consumerIsCurrent(actualTransaction)` 与 `consumerBasisCurrent(actualTransaction,actualAuthority,startupId)` 仅做纯内存实际对象/原分配 UUID/Authority epoch、generation、source hash 与 Transaction constructor epoch 匹配。后者只接受尚空的实际 generation0，或原计划 UUID 的首次 preparing/running generation1（首次 changed 尚未发也能核对）；终态/第二次 factory 均拒绝，同 scope 的 transferred Authority 不因为旧accepted状态退休而自拒。控制器的实际 source/context 回调必须同时核对它们，不能绕过工厂把另一新 basis 直接送进旧空 child。纯匹配不替代 Transaction/Authority 的完整动作门禁或原期限检查。

通知前先完整提交 child 与索引，再发 `changed`；通知后重查实际对象、序号、索引、原范围及 full context。重入、销毁、初始化或期限变化时不返回可用消费者，但已提交 child 保留，不回滚删除事实。`Transaction::changed` 仅观察原生快照/通知，不自动取证、审批或发送。

## 脱敏摘要与边界

公开 `current` 包含容量、来源丢失、错误、前代 UUID/epoch、脱敏 consumerSnapshot 和最多9轮摘要。没有账号 SID、凭据、RAW 字节、完整参数或传输 observations；只读快照不是命令门禁。实际 consumer/previous 对象应仅在控制器私有 backend 使用，不能向操作 UI 暴露任意发送接口。

来源丢失后 `historicalSnapshotsOnly/sourceLossMayHideCommittedCommands=true`：最后观测可能没包含销毁/通知窗口中的真实排队，不得把缓存的flightCommandReleased=false当“没有发包/未起飞”。保留诊断并阻断新分配，不恢复或猜测远端结果。

本模块没有证明完整 HOME/local frame/运动/转向策略、最终 StartPermit、净空、实际起飞执行、任务完成或跨站控制租约。多轮源码保留不是完整运营历史数据库。原完整目标继续保留。

## 留给工程师的验证项（未执行）

1. Qt/moc/pri/type 注册、QObject 绑定与 root 私有回调实例匹配；本文件不自行注册。
2. 第1至第8轮新 child、容量拒绝、初始 owner 复用拒绝、旧终态事实原样保留。
3. 未初始化候选同basis精确复用；另一真实新Authority/审批/basis通过完整门禁后分配新child，旧空owner不覆盖且占容量；旧同scope替换和过期复用拒绝。
4. 最新 owner MODE 已入队/不确定、active/drain/预留未释放拒绝，不能退回更早 owner。
5. changed 通知重入、删除 series/owner/authority、首通知前初始化与销毁、丢失来源永久拒绝。
6. 旧成员组/顺序/binding/deployment 替换、同 ID 不同实例、旧 startup UUID/epoch/generation 覆盖拒绝。

参考：[启动事务](FleetMissionStartupTransaction.md)、[地面再审启动依据](FleetMissionGroundStartupAuthority.md)。

# FleetRolePolicy：本地角色政策与监视结束权限

## 509：相机单流启停是独立强制二审写动作

精确新增 `fleet.video.stream.control`：默认 operator／maintenance／commander 可以申请，administrator 保持原 `*`；observer／planner／safety_officer 不因拥有相机 READ 而自动取得写权限。它强制二审，不受可编辑角色默认 requiresSecondApproval=false 影响，也不进入 READ 免二审闭集。合格独立审批账号仍由真实 Session 核验，必须与申请 SID 不同。

此动作只用于所审阅单流的 VIDEO_START_STREAMING／VIDEO_STOP_STREAMING 和状态回读，不授予参数、上传、ARM／起飞或飞行控制。读取成功、旧 control 回执或已有 `allowed=true` 不是它的批准；恢复／启动必须重新取得相机资料、审阅和独立批准，不自动复用旧暂停权限。见[控制合同](FleetCameraStreamControl.md)及[身份合同](FleetOperatorSession.md)。本阶段仅改源码与直接静读，没有编译、执行认证或测试。

## 508：独立相机流元数据 READ 权限

新增精确 action `fleet.video.stream.capture`：默认 planner／operator／safety_officer／maintenance／commander 允许申请，administrator 保持原 `*`，observer 不自动获得。它属于 `requiresSecondApproval=false` 的精确只读例外，不放宽其它动作或整个角色的二审要求。

此权限仅用于明确确认后，对同一飞机、相机 component 和非零 stream 的一次 Info／Status 读取；真实 Session 身份、原 Role9、Root 意图、Registry 绑定及原生来源仍须分别核对。不能借 RTT／interval capture 或本地 `allowed=true` 取得它；也不允许配置、远端启停流、恢复、降速或飞行控制。Session 不新增二审 purpose、批准凭据或 lease。

具体接口与完成语义见[主动查询合同](FleetCameraStreamCapture.md)，工程验证见[交接说明](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本阶段仅源码与直接静态阅读；未编译、测试、运行认证或应用，未仿真、操作硬件、修改 PX4、刷机或放行。

## 504：独立单次RTT READ权限

新增精确action fleet.telemetry.rtt.capture。默认planner／operator／safety_officer／maintenance／commander及administrator可执行，observer没有；requiresSecondApproval的精确READ例外不改其它action或全角色二审政策。仍须真实主账号／实时身份和Role9／原Root来源／明确确认。

它只允许当前受控地面分支最多单次定向111诊断；不是interval capture、configure、飞行命令或完整网络探测许可。Session批准purpose不改，没有新增二审证明或lease。见[采集合同](FleetLinkLatencyCapture.md)和[44项全未执行](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502：可见操作不扩大角色政策

新[Panel](../../res/Merivus/FleetOptionalTelemetryPanel.md)只按当前账号显示／启用操作，实际Root／Session／Role门禁仍决定许可。读取与配置分步，配置仍仅operator／maintenance／commander／administrator可申请且强制二审；不能从UI勾选或共享报告获得批准。Role源码和9键政策不改，不新增账户或企业权限。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501：独立地面配置action强制二审

精确新增fleet.telemetry.interval.configure；默认operator／maintenance／commander允许申请，administrator保原*，planner／observer／safety_officer不因能READ而能配置。此action强制two-person policy，不能通过普通needTwoPerson设置关闭。实际[Configuration](FleetOptionalTelemetryConfiguration.md)另核当前Session和不同SID批准。

capture action仍是单账号只读用途；Session配置purpose为fleet.telemetry.interval.configure.approve，与上传／启动／能力／构型凭据不通用。Role9只是政策值，不能独立形成实际配置许可；正常prompt保留旧显示不修改政策／角色，也不续期。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500：独立可选遥测间隔READ动作

默认planner、operator、safety_officer、maintenance、commander新增精确fleet.telemetry.interval.capture；administrator仍沿用*，observer不自动获得。本动作明确requiresSecondApproval=false，属于有界只读请求例外；不修改Session二审闭集，不借上传／启动／能力或构型批准凭据。

实际[Capture](FleetOptionalTelemetryCapture.md)仍核真实主账号、同一原政策九键、Root／Swarm／Registry／Vehicle原对象与固定期限，最终write前重新认证；local allowed旗标不是账号认证。它只查询510，不允许511、恢复原率、起飞或执行任务。政策或账号通知保守退休旧批次，无自动恢复。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

[24路径与40未执行交接](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 426：失败部署的独立只读重新核对

默认 planner、operator、safety_officer、maintenance、commander 增加精确 `mission.reconcile.read`；administrator 沿用 `*`，observer 不自动获得。此动作属于有界 Mission READ 的单账号例外，`requiresSecondApproval=false`；不增加 Session 第二账号用途，也不借用上传、启动或能力批准凭据。

[实际重新核对](FleetMissionReconciliation.md)仍要求同一真实 Session 的当前主账号、完整九键政策快照、原 Deployment/车辆/主链路及当前授权来源，在实际发送前持续检查。权限只允许从本实例失败或取消的部署发起独立诊断读取，不允许写任务、清空任务、恢复旧批准或启动飞机。不同、未知、真实 COUNT 0 都只是该次读取的诊断结果，不改变原 StartReview 门槛。

本地 policy 仍可由受信调用者修改，不是企业身份系统或恶意插件隔离；新增权限不认证物理停机或远端控制权。仅源码与文本审阅，未运行认证、编译或测试。

## 421：独立的构型基线动作

maintenance 新增精确 `fleet.configuration.review`，administrator 保持原 `*`。该动作与 `fleet.capability.review` 均强制第二账号，不能用可编辑角色默认 flag 放宽。实际 [Review](FleetCapabilityReview.md)按原冻结 operation 选 action，并独立核维护/管理员申请、不同合格 SID 审批以及 Session 精确用途。

Root 将两类九键政策一起冻结，切换操作不隐式生成新身份或继承旧批准。该新增动作只允许审阅本地参考基线，不认证实物、性能或授予机载控制。原政策收据 schema 不变，旧原生兼容接口仍是受信后端，不宣称所有入口均经过双账号。未编译、测试或运行认证。

## 418：本地能力审阅强制双账号

maintenance新增精确fleet.capability.review，administrator保原*；本动作强制requiresSecondApproval=true，不受默认角色flag放宽。[实际Review](FleetCapabilityReview.md)另核维护/管理员真实申请身份和不同合格SID审批，批准与撤销由原operation及bindingHash隔离。

本地政策本身不认证身份，不授飞行/上传/改参数或更新构型基线权限。原生旧bool后端仍非此账号路径，不宣称全入口统一授权。仅静态源码，未运行认证或测试。

## 417：目录重载强制双人审批

maintenance 新增精确 `firmware.catalog.reload`；administrator 仍沿用 `*`。此动作强制 `requiresSecondApproval=true`，不受本地角色默认 flag 改成 false 的影响。实际 [Reload](FleetFirmwareCatalogReload.md)另核 Windows SID／角色，只允许 maintenance/admin 发起，独立审批人仍须安全员、总指挥或管理员且 SID 不同。

可编辑 policy 只是一项本地约束，不认证身份、不恢复旧审批，也不授控制或飞行权限。既有动作规则不因本次权限增加而放宽；仅源码和静态核对，未执行认证或测试。

`FleetRolePolicy` 只回答本地角色政策是否允许某个精确操作，不认证账号、不建立车辆控制权，也不发送命令。`defineRole()`、`assignUserRole()`、`setCurrentUserId()` 等是可变本地模型接口；原生消费者必须冻结实际 policy 对象，并结合独立真实身份来源核验，不能把 caller 给出的角色或 `allowed=true` 当成认证。

## 独立监视结束动作（2026-10-15）

新增精确角色权限 `task.observation.close`：默认 operator、safety_officer、commander 显式拥有；administrator 沿用既有 `*`。observer、planner、maintenance 不自动获得此权限。上述允许角色的默认 `requiresSecondApproval=true` 保持不变；该动作不加入 READ 请求的单账号例外，不借用 upload/start/ground 的批准。

这个动作仅供独立原生监视结束 workflow 使用，含义是结束实际 Run 的本地监视/本次 local reservation，不是停止飞机、LAND、DISARM、任务 Completed 或验证机载执行成功。workflow 必须同时核同一真实 Session 的当前主账号、当前 policy、精确 Run/完整 binding/成员与占用范围，以及独立不同 SID 的当前审批账号。OS 提示/身份一次消费使用另一精确 purpose `task.observation.close.approve`，由 `FleetOperatorSession` 私有 friend 消费，不接受本地角色快照替代真实两账号。

## 政策收据

`actionSnapshot(action)` 仍恰好九键：`actorId`、`role`、`action`、`allowed`、`requiresSecondApproval`、`policyEpoch`、`policyRevision`、`actorAuthenticated`、`executionPermitted`。其中身份/动作/epoch/revision 是实际 String，四个旗标是实际 Bool；`actorAuthenticated=false`、`executionPermitted=false` 固定，不能覆盖为 true。要成为有效本地快照，当前 user/role、实际 per-user assignment、允许权限与有界文本必须一致。

`actionSnapshotCurrent(receipt)` 要求精确九键/类型、当前完整政策收据一致且 allowed 为 true。原生消费者还须核 receipt 的 actorId/role 等于 `FleetOperatorSession` 实际身份，且 action 恰为当前流程要求。不能只调用 `canPerform()` 或 `evaluate()`，也不能仅核 action 字符串或 policy 哈希。

用户切换、角色变更、分配/定义/删除政策递增不回绕的 revision，修改后即使恢复同样表面值，旧 receipt 也不会重新 current；policy 实例拥有随机 epoch，不能用另一实例替代。消费者须在原生账号/WTS 查询可能重入后再核同一实际对象和完整收据。Windows 直接角色成员关系、身份期限和工作站状态由独立 Session 即时核验，不由本地政策声明推导。

角色模型可由有权限的本地调用者改写，并非恶意插件/进程内攻击隔离或企业身份权限系统；运行时自定义政策也不能消除专用 workflow 对真实身份、第二账号和实际 owner 的独立门禁。导入摘要、旧审批或角色 JSON 不能恢复这些权限或改变飞行完成统计归因。

本批仅源码与限定文本检查，未编译、测试、执行系统认证/业务 helper 或操作设备。实际 Windows/Qt 行为与两账号拒绝/撤销路径仍待工程师验收。

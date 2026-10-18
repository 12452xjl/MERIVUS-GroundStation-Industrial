# FleetMissionEvidenceRefresh：手动原生只读证据刷新

## 482 当前：固定联合29项，期限与旧观察接口不变

联合读取原12项＋速率守卫2项＋动作14项＋独立到点项 `NAV_MC_ALT_RAD`，固定排序29项，不按profile选子集。新增项必须真实REAL32、有限、原值精确float32、非负且abs≤2^53−1；不改参数或用metadata显示上界代替完整受支持数值域。部署说明及确认文字同步29，原profile hash观察仍只12。

PM新增独立 `executionArrivalParameterNames()` 和四组 `parameterArrivalExecutionObservationsMemoryMatch`／快照版尾核；旧12／14／28三口保精确旧合同。新 `executionParameterSnapshotRows` 只做exact6快照的有界值恢复，JSON往返的精确整数元数据恢复Int、值保Double，RAMP负零由显式布尔marker恢复；它不采样、不写参数、不认证来源。

仍为120秒总期限、4秒单阶段、100毫秒尝试间隔，busy／rate拒绝不续期；取消／超时不补成功或恢复旧批准。29项增加材料量，不保证慢链路、多机容量或全组鲜度能够通过。

仅静态源码及文案修改，未运行刷新、QML、编译、测试、仿真或操作硬件；最终文本核对已记录于本批静态审计。没有新增命令权限、自动请求、PR或包。下方为历史。

## 480：联合28固定项，旧12／14合同保持

当前固定读取原12项、两个ALL守卫及ParameterManager::executionActionPhaseParameterNames()的14项动作参数，联合排序逐项收回执；不接调用方子集。动作集合详见[模型](FleetMissionActionPhaseReview.md)。COM_OBS_AVOID／MPC_LAND_RC_HELP须实际INT32且0／1，其余须REAL32、原值精确float32并在abs≤2^53−1材料域；0／负sentinel在观测层不自动改正，支持范围与SYS_RESP一致性由独立模型消费。

PM旧parameterObservationsMemoryMatch精确12和parameterExecutionObservationsMemoryMatch精确12＋2均不改；新增parameterActionExecutionObservationsMemoryMatch在同一次纯内存尾核核12＋2＋14各自固定排序行，不能混合全局排序28列表代替三组。不锁新link、不调用Link虚函数、不请求或写参数。

最大每架28项；120秒总期限、4秒单阶段、100毫秒尝试间隔、busy／rate不续期和撤销旧审阅／批准保持。更多参数不保证慢链路、全组30秒鲜度或多机容量可通过；失败／取消不补成功、不恢复批准。实际部署说明与勾选均28项，原目标来源／默认关闭／忙锁／确认门禁不变。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：固定14项只读范围，不改变旧12参数内容哈希

本批在原十二项后增加 `MPC_XY_VEL_ALL`、`MPC_Z_VEL_ALL` 两个总体速度设置守卫，按名称排序逐项读取，且每次仍由原操作者明确确认既有只读刷新入口。固定名称来源复用 `ParameterManager::executionRateGuardParameterNames()`，不接收任意 UI 参数列表。两个新增值必须是 live `MAV_PARAM_TYPE_REAL32`、有限且精确 float32 可表示的原始数值；负数表示关闭总体覆盖，非负表示活动设置。观察到0可以完成采集，但不把0说成合法飞行速率或符合原十二项正速率要求。

原 `parameterProfileHash` 及 `requiredProfileHash` 继续只代表原十二项，不把两项守卫悄悄并入旧哈希。PM 被动观察/明确只读请求白名单扩大为固定12+2；原 `parameterObservationsMemoryMatch` 仍只接收按名称排序的精确12行。新增 `parameterExecutionObservationsMemoryMatch(componentId, PRIMARY, weakOwner, legacyRows, guardRows, maxAge)` 在一次纯内存调用内核完整12+2行；两组分别使用各自固定顺序，均为原14字段无年龄 stable row，拒绝缺项、重复、任意子集、额外字段、源/组件/PM epoch/type/value/revision不符、待写、Fact或PRIMARY身份改变及过期。尾部再核原上下文、共享控制块、账本代次与整组最早回执年龄；不锁 weak link、不调用 Link 虚函数、不请求或写参数，不把顺序回执当作机载原子采样。

总预算仍固定120秒，单阶段4秒、尝试间隔100毫秒，不因为增加两项、等占用或慢链路而延长总期限。返回 DTO 字段、人工确认方式、撤销旧审阅/批准、取消及迟到包语义均不改变；结果最多每架14项。完成只证明本次有限读取和回执核对，不恢复批准、启动许可或持续新鲜度；扩展纵向相位参数和内部状态、着陆RC辅助、斜坡/动力学与动态净空仍是后续工作。实际部署面板的说明和勾选文案同步为14项，并列明两个新增ALL名称；原目标来源、失效及确认门禁不变。

本轮只修改源码和文档并静读审查；未编译、执行任何实现或测试、仿真、硬件操作、修改PX4、刷机或放行。工程师需另行覆盖两个守卫缺失、错误MAV类型、缓存/次链路替代、REAL32非有限或非精确值、负/零/活动正值、总体设置覆写与独立项不一致、12/14行混用、来源ABA与恰逢期限、慢链路14项未完成及审批不能自动恢复。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestRefresh、_gate及原参数/版本scoped请求的预发送fence绑定实际Session同组，OS/owner回调后反复核原QQ/代次/期限；不修改被动current、cancel或已有取消后排空语义。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


该模块在明确确认后，为当前实际 PX4 多旋翼请求新 AUTOPILOT_VERSION 与固定十四项 PARAM_VALUE 观测（原十二项加两项总体速度设置守卫）。只请求读取：没有参数写入、任务上传/清空、任务启动、解锁、模式切换、自动审批、自动恢复或执行。代码实现并不证明硬件身份、固件签名、机载执行许可或实飞安全。

## 入口与来源

`requestRefresh(session, role, owner, vehicleIds, operatorConfirmed)` 必须接收实际原生 `FleetOperatorSession`、`FleetRolePolicy` 和同线程控制器。主身份必须仍由实际会话认证，SID、角色与本地 `mission.evidence.refresh` 权限快照一致；调用者不能通过填写姓名、角色或 authenticated 字段代替它。

目标列表只允许真正数值类型的整数机号 1–255，不能重复，一次最多 32 架。冻结实际 QGC 模型里的唯一 Vehicle、它自己的 ParameterManager、MissionManager、VehicleLinkManager 与当前 PRIMARY Link。要求初始化连接完成、真实链路在线、不是回放/离线编辑/MockLink、PX4 多旋翼、未解锁/未飞行、参数加载完成且没有缺失和待写参数、任务传输空闲。原生请求 API 还独立核对当前原始 HEARTBEAT、ON_GROUND 状态、发送方与自己的传输占用。

控制器必须通过 `fleetEvidenceRefreshSourcesMatch(actualRefresh, session, role)` 确认实际私有来源，并通过 `fleetEvidenceRefreshAllowed(actualRefresh, vehicleIds)` 核对集群和地面站占用。严格要求真正布尔 true；缺少方法、替换对象或改变上下文均拒绝。控制器在任何刷新请求前撤销旧审阅、意图确认和上传审批；刷新服务不会替控制器恢复这些状态。

每个原生请求必须携带按值复制的 C++ `std::function<bool()>` 发送授权栅栏，不接受调用者 JSON、签名标志、虚构 nonce 或 QML 布尔作为该栅栏。它冻结本服务的实际 QPointer、代数、阶段、目标/参数索引、会话/角色/控制器、目标对象和两个截止时间。原生发送层在 outgoing plugin 回调之后以及真正进入链路队列之前调用它；栅栏在原生身份与控制器复核前后均检查同一冻结上下文及 4 秒阶段/120 秒总期限。因此 plugin 回调内取消、注销、撤销权限或销毁来源时不能再排队新的读取包，仅发送之后的复核不足以提供此保证。空栅栏拒绝，不续期，不恢复权限。

这些原生来源和角色模型仍是同进程协作边界，不是针对恶意原生插件或已控制进程的隔离沙箱。会话身份是操作者账户身份，不是无人机硬件身份。

## 有限状态与新 RAW 到达

每次生成新请求 UUID 和不回绕代数，只允许一个活跃请求，逐架执行：

1. 请求版本，只等待实际 RAW AUTOPILOT_VERSION 成功提交的 `receiptSequence` 严格大于原生请求前基线。
2. 明确核对收到该观测的实际 Link 就是冻结的 PRIMARY、Vehicle/组件未替换、来源 epoch 未改变、字段完整且在观测时间策略内。
3. 新版本首次确认可能正常清除旧参数观测。因此在版本 RAW 提交并完成回调后才捕获 ParameterManager epoch，不在请求最初冻结旧参数 epoch。
4. 按同一固定十四项名称依次请求参数。每项等待自己的 RAW 成功提交计数严格大于基线、同一实际 PRIMARY 来源与同一 manager epoch，且 cache 匹配、无待写、可用且新鲜。
5. 每阶段及最终重新核对身份、权限、控制器、实际对象、链路、epoch 和已观察计数不倒退；最终状态是 `observations-renewed`。

固定名称与 `FleetMissionVehicleEvidence` 一致：`MPC_XY_CRUISE`、`MPC_XY_VEL_MAX`、`MPC_Z_V_AUTO_UP`、`MPC_Z_V_AUTO_DN`、`MPC_Z_VEL_MAX_UP`、`MPC_Z_VEL_MAX_DN`、`MPC_TKO_SPEED`、`MPC_LAND_SPEED`、`MIS_TAKEOFF_ALT`、`NAV_ACC_RAD`、`MPC_YAW_MODE`、`MPC_YAWRAUTO_MAX`，以及守卫 `MPC_XY_VEL_ALL`、`MPC_Z_VEL_ALL`。不接收 UI 任意参数名称，不走 legacy `refreshParameter` 的身份失效/读等待重试逻辑。

计数由原生接收路径在成功提交 RAW 观测时产生，包括同值再次收到的包；不采用 prepare token、cache 时间、内容修订变化或发送 ACK。初次没有 RAW 观测时允许基线为字符串 `0`，不要求旧观测已有 PRIMARY 来源；新到达后才必须验证收到包的实际来源。计数是严格十进制字符串，不能通过 JSON 数字转换丢失 uint64 精度，不回绕。

这些无线报文没有本服务的请求关联 token。计数大于基线仅证明**本地请求之后有 RAW 到达**，也可能是同一链路上的异步/延迟报文，不证明报文确实回应本次请求；因此 `responseAttributionVerified=false`。ACK 本身永远不能完成观测阶段。

## 时限、占用和失效

整个请求最多 120 秒，每项阶段最多 4 秒（包括本地未发送的占用等待），所有尝试至少相隔 100 毫秒，只有当前一架的当前一项在等待，不发全参数列表、不广播刷新、不无限重试。

仅当原生请求明确 `requestIssued=false` 且原因为 `fleet-evidence-request-busy` / `fleet-evidence-rate-limited` / `fleet-evidence-status-wait`，才在原有阶段时限内等待下一次尝试；不把等待记为已发送，不重置期限。新版本观测可能先于对应命令队列 ACK 到达，下一参数请求仍需要等该队列排空；首次或改变的版本来源也可能合法清除旧 HEARTBEAT/landedState，因此原生 status-wait 只等待固定 PRIMARY 上重新到达当前地面状态，缓存或 RAW 表示已解锁/离地仍是立即拒绝，而非等待。其他拒绝、来源/epoch 改变、解锁、起飞、参数写入、任务占用、失联、替换对象、身份变化或总/阶段超时均停止进一步发送。

取消按确切请求 ID 操作，过期 ID 不能取消新请求。取消或失效仅停止之后的只读请求；已进入链路队列的只读包或回包可能继续到达并更新原生缓存，不回滚、不撤销收到的 RAW，不删除任务。公开 `readOnlyRequestsMayRemainInflight` 是发起原生请求尝试后的保守警告，不是 wire 发送/ACK/关联成功证明。

Windows 会话新鲜度查询可能分派锁屏/注销窗口消息，所以原生身份校验之后仍必须重新核对 QPointer、代数、冻结状态及来源。控制器回调前后进行同样复核；用户取消或来源销毁可以在发送调用栈中发生，不会接着发下一项。

## 结果边界

`current` 只包含最多 32 架、每架最多 14 项的机号、阶段、来源 epoch、RAW 计数和本服务接收记录时间；没有计划内容、Fact 对象、口令或完整身份收据。请求入口 `ok=true` 只表示接受刷新；current 的最终 ok=true 才表示有限批次内的观测采集完成。

结果只描述本次历史观测及复核，不提供持续新鲜度承诺。服务最终使用最多 120 秒的观测年龄策略；后续任务审阅仍要按自己的更严格参数/版本年龄、健康、配置、能力和真实源校验。特别是 32 架批量顺序读取的最早参数，可能在整个批次结束前已超过任务审阅的 30 秒参数年龄，需工程师评估机组大小和人工分批，不能自动恢复审批来掩盖失效。

120 秒总量、4 秒阶段、100 毫秒节流不保证所有 32 架都能完成。慢链路、共享队列和状态数据缺失会受限退出，UI 应说明原因和已经观察/尚未观察机号，不把部分结果标成完整成功。

所有结果始终 `signatureVerified=false`、`hardwareIdentityAuthenticated=false`、`responseAttributionVerified=false`、`reviewRestored=false`、`uploadApprovalRestored=false`、`startupPermitGranted=false`、`flightCommandReleased=false`。刷新结束后要重新编制、审阅、审批并人工确认；不会上传或起飞。

## 工程师后续验证（本轮未执行）

- 首次 RAW 缺失、同值重复 RAW、缓存/FTP/文件不能代替到达、版本 ACK 先后顺序、UNKNOWN/丢失字段和不支持固定参数的拒绝。
- 同 PRIMARY / 次级 Link、源 epoch 变化、PM 替换/复位、新版本导致旧参数失效、实际机号歧义和 uint64 计数耗尽。
- 发送期间取消/销毁/锁屏/注销/角色变化/任务占用、正好阶段/总时限、busy 等待不延期、节流与迟到回包。
- 多机组与慢链路资源预算、完成后的任务审阅新鲜度、旧审批在请求前撤销及不能自动恢复。
- 编译、运行、链路、仿真及硬件验证全部由工程师另行执行；本轮仅源码实现与静态审查。

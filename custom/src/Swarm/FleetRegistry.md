# FleetRegistry 原生动态投影契约

## 516：同实际绑定读取独立SOC下降窗

`_vehicleSnapshot`在同一原Vehicle／recordEpoch／bindingEpoch及ProjectionGuard中，依次读取原电池20字段与新`fleetBatteryDrainObservation`，每次读取后核原对象和绑定。新通道仅通过原生memory getter，不请求遥测、重探Link或改变持久记录。新`nativeBatteryDrainEvidence`从所有静态root／metadata入口剥离，快照也先剥同名值再覆盖实际结果，不可用返回共享unknown工厂；实际DTO违反共享值合同时保明确invalid空map，由消费者拒绝，不静默回退较宽模型。

源一致性、模型公式及未知／过期由[共享校验](FleetNativeBatteryEvidence.md)与[实际审阅链](FleetNativeBatteryBridge.md)消费；它们不认证调用方map、物理电池、远端采样时间或性能。旧nativeBatteryEvidence的schema1/2、标量别名和负面准入保持。仅源码／直接静读，未编译、执行、测试、仿真或操作设备。

## 507：实际绑定的一次相机流review

新增nativeCameraStreamReview(systemId)：固定wrapper7沿既有真实Vehicle／recordEpoch／bindingEpoch，调用fleetCameraStreamObservation(3000)一次，闭22／4／15／15及原生精确类型、排序唯一性、receipt／revision／count关系；拥有化拷贝后再次核原绑定。sourceCurrent=false／saturated／单行stale仍可作为合法历史诊断返回ok=true，不是控制依据。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

[完整合同](../../../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 503：实际绑定的一次本地接收review

新增nativeLocalReceptionReview(systemId)：固定wrapper7为ok、error、vehicleId、vehicleRecordEpoch、bindingEpoch、observation、snapshotOnly。只从现有actual Vehicle来源绑定读取native getter一次，前后及深拷贝结果后尾核原record／binding。失败observation={}；成功但sourceCurrent=false仍是合法历史诊断，不是认证或许可。

闭native22／window9严格原生类型与公式校验；global receipt清源后可非零，历史observed且age=-1、完整0尾窗、current但saturated均分别保留。没有metadata／护照／调用者Map来源入口。原15／21／6可选流合同不改。

[完整观察合同](FleetLocalReceptionObservation.md)、[40项全未执行](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502：可见面板不接收原生Registry

新[Panel](../../res/Merivus/FleetOptionalTelemetryPanel.md)仅通过实际Root的只读值和显式操作；不接收原Registry或从metadata／护照／历史重建Vehicle绑定。源码与15／21／6合同不改，后端actual绑定、opaque Basis及最终围栏仍依原500／501链路。界面未执行，不能把uiWiringPresent当运行验证。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501：配置服务仍只取原实际绑定

新[Configuration](FleetOptionalTelemetryConfiguration.md)复用Swarm私有原Registry绑定的Vehicle／recordEpoch／bindingEpoch及500 Capture。Registry源码与旧observation15／review21／comparison6合同不改；其Map、metadata、护照、历史报告不能重建native Basis或配置授权。

own511清原账本后，后续真实回包才可建立同原Run受控新绑定；现有Registry／Guided仍须新重采，不自动把旧值快照标当前。配置结果reported-values-match不等于实测率／带宽或能力认证。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500：新读取服务复用原生绑定，不增加配置许可

Registry.h/.cc不改。原Swarm friend增加C++窄getter，只在本实际Capture／Registry配对时返回现有私有Vehicle绑定及recordEpoch／bindingEpoch，服务不能用metadata／护照或调用者Map安装来源。复用NativeBatterySource仅为对象绑定，不以电池健康授予遥测读取权限。

新[Capture](FleetOptionalTelemetryCapture.md)显式查询固定三流，报告进入原498被动账本；optionalTelemetryRateReview仍一次原native getter、15／21／6完整校验和snapshotOnly／许可false。后续Guided五秒重采才读新值，旧快照不自动更新；不调用配置器。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

[24路径与40未执行交接](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499：原只读审阅消费实际write前的旧基线失效

本Registry.h/.cc不改，498观察15／review21／comparison6及实际Vehicle绑定／一次getter完整校验继续。Vehicle的ledgerEpoch从纯UUID变为UUID:不回绕十进制mutationGeneration，仍是不超过64字符的opaque本地身份；任何调用者不得解析后重建来源或当认证。

本车默认component的任意511只在最终actual write前，以同UUID一次预留推进不回绕mutation generation，并全清498三report及接收窗；ledgerEpoch为UUID:十进制generation。writeDepth贯穿write与通知，临界内原包准入／getter不可当前。未发送拒绝不冒称已配置；已可能write的代次／requestIssued不回滚，耗尽保守停止而非重用。498观察15／Registry review21／comparisons6保持，后续只读重采会看到旧基线失效，既有值快照不自动变成实时通知，须新被动资料与新完整5秒窗，不等于实际生效。

511的lane／ACK终态本身不改Registry健康、能力或批准。write后的新244即使同值也只是新本地报告；不能自动恢复旧capture／审批，未来真实Apply必须持独立原生引用、原期限和实际授权。原generation不经metadata／护照导入，也不因源清理归零。

Vehicle公共raw sender在plugin前后拒510／511／512旁路，正式请求走专属通道。此覆盖不包括MAVLinkProtocol转发、移动Taisync字节桥、外部GCS或固件内部调频；本批不改变转发策略，不认证实际发布率、丢包或带宽。

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

[18路径清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)及[42项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498：可选遥测真实绑定的独立只读数值审阅

新增Q_INVOKABLE optionalTelemetryRateReview(systemId,requestedIntervalUs)，不修改普通vehicle快照或持久健康／能力／批准。只用现有friend私有NativeBatterySource保存的实际Vehicle／记录epoch／bindingepoch；此处仅复用实际对象绑定，不消费电池DTO或把电池健康当遥测配置许可。前后核同绑定，原生getter一次，不调用带其它投影的_vehicleSnapshot。

### 原生观察合同

Vehicle.optionalTelemetryObservations(maximumAgeMs=30000)不是Q_INVOKABLE。顶层exact15：schemaVersion=1、reportType=fleet-optional-telemetry-observations、vehicleId、componentId、ledgerEpoch、versionEpoch、sourceCurrent、maximumAgeMs、rows，以及readonly=true／authenticated=false／actualTransmitRateVerified=false／availableBandwidthVerified=false／executionPermit=false／flightCommandReleased=false。

rows恰三条固定62／74／83，每行exact3(messageId,reported,receivedWindow)。reported exact8：observed、available、fresh、ageMs、receiptSequence、contentRevision、intervalUs、state；未知interval为原生null，已观测间隔为原Int，state按unobserved／disabled／not-available／positive／invalid派生。报告receipt／revision为不回绕canonical uint64文本；同值新包改receipt／年龄而不改contentRevision，清源不归零全局counter。

receivedWindow exact9：durationMs=5000、retainedFrameCount、complete、saturated、receivedFrameCount、receivedMeanHz、maximumInternalGapMs、newestAgeMs、receiptSequence。每流5秒尾窗、最多512个单调接收时刻；原来源绑定满5秒、该流曾收到且最近包不超过30秒、无相关接收栈／探测中状态和窗内容量丢失才可complete。完整窗可以0帧，仅表示本地该5秒没收到；从未收到过不能complete。均值是本地收包数／5秒，最大内部间隔不含窗两端空隙；不推算发送率、链路丢包或停流。

Vehicle只观察原始PRIMARY的MESSAGE_INTERVAL244及固定NAV_CONTROLLER_OUTPUT62、VFR_HUD74、ATTITUDE_TARGET83。244保原signed间隔：−1已报告disabled，0已报告not-available，正数为报告间隔，低于−1为invalid；未观测是null而非−1。报告fresh与available分开，所有值均非身份／实际发布率／带宽认证。

sourceCurrent必须原PRIMARY、持久QQ provider、同weak控制块和实际pointer、原版本来源epoch／component／固件／机型及全栈quiescent；中央invalidate直接清账本。Link虚调仅发生在原包准入并逐步尾核；getter无虚调／weak.lock／signals／新observer或请求。未绑定版本epoch为规范空QString，不是认证失败；整份严格校验不将未知补健康。

### Review合同及真实消费

返回exact3(ok,error,review)，失败review空。无提案(null／0)不是511默认请求；正提案须真实整数int32且可精确REAL32，拒Bool／String／小数／负／越界。对同次三条当前鲜正报告比较targetUs>=baselineUs；基线−1／0／invalid／未知或过期均不能参与已知正基线，不用本地窗口反推。

review exact21：schemaVersion=1、reportType=fleet-optional-telemetry-rate-review、registryRecordEpoch、bindingEpoch、vehicleId、ledgerEpoch、versionEpoch、sourceCurrent、requestedIntervalUs、proposalProvided、observation、comparisons、numericComparisonComplete、nonIncreasingReportedRate、reasonCodes、snapshotOnly=true、configurationPermitted=false、rateAppliedProven=false、wireCausalityProven=false、executionPermit=false、flightCommandReleased=false。

comparisons恰3，每条exact6：messageId、baselineKnown、baselineUs(Int/null)、baselineExactlyEncodable、nonIncreasingReportedRate、reason。整组数值比较完整需要实际正提案及全部当前正基线；精度标记只是诊断，不能舍入后自动restore。returned readonly值不建立后续write资格，不接受callerMap作为native来源。

现有custom Guided五秒策略快照把Policy正rateHz换成ceil(1000000/rateHz)的只读间隔提案，经实际私有_swarm.fleetRegistry调用并附optionalTelemetryRateReview；0／未知不变为停流／固件默认。Policy源码、P0流、旧PID／ESC与发送器不改，公开facade不暴露原生setter或新增apply。

本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

本批[18路径](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)及[36项未执行场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 484：规范性能声明的stage与一次CAS

新增native-only `previewPerformanceDeclaration(id,expectedContext,proposedPerformance)` 返回exact4 `{ok,reason,proposedPerformance,proposedContext}`；失败后两map为空。输入exact2必含maxSpeedMps／enduranceSeconds，实际有限数值且abs≤2^53−1；−1明确unknown，speed0合法、endurance0拒、至少一known，不收alias／partial／extra／字符串／null。

私有stage只读实际stored row、review、完整history及本机基线，不调用带原生电池投影的_vehicleSnapshot。须原context和当前完整匹配基线，保原typed观察／批准；规范数字替换并清metadata同名及enduranceSec旧别名，一次推进review revision，计算同原record的新identity/context。耗尽拒绝，无半提交。

`approveDeclaredPerformance(id,expectedContext,proposal,expectedProposedContext,caps,reason,fence)` 重新stage并精确比拟context，造原approval14，原角色／账号认证由真实Review另行负责。原record／review／history／基线批准及typed指纹围住独立持有的fence；最终纯CAS后一次写数字与新approval，再通知，原baseline／observation不重新批准或造新样。原receipt14先独立复制容器／文本，通知后stillCurrent可false，mutationCommitted真事实不撤回。

普通Swarm刷新不提供这两数字，partial merge不会清空；外部显式更新／护照新数字仍退役旧批准，不以本地override忽略新来源。同值新采纳仍推进revision。旧四操作保持；新声明供[483严格消费者](FleetCapabilityEvidence.md)，不是实测认证、硬件验证或飞行许可。工程验证全部未执行，未编译／运行／测试。下方为历史。

## 460：明确电池故障保留并进入健康快照

电池投影统一使用 [FleetNativeBatteryEvidence](FleetNativeBatteryEvidence.md) 的原生只读解析器，兼容原 schema 1（outer20 / row15）与 schema 2（outer20 / row19）。新行保留 `chargeStateKnown/chargeState` 和 `faultBitmaskKnown/faultBitmask`；注册表不另造扩展字段，也不凭显示 Fact 填未知值。

Registry 给原 DTO 配上本次标量别名后检查 `Read.valid`，而不是以 `Read.usable` 决定是否保留证据。这样结构合法但明确报告故障的电池不会被替换成空的未知 DTO。共享解析器只把实际当前来源、未过期电池行中的非零故障位或明确不安全充电状态作为负面；该负面可来自辅助电池，不受推进电池聚合是否成功限制。来源失效和过期行不被称为新鲜故障。同源曾明确负面的行过期后保留 `native_battery_fault_evidence_stale`，变为未解决的未知/不可用，不因辅助电池老化自动恢复健康。

已报告负面只修改当前返回副本：保留原其它故障摘要，追加具体电池原因并设置 `hasFault=true`，复用原评分逻辑；不改持久记录、原 SYS/EST 的 `faultHealthKnown` 或其年龄。无效/不可用的必需原生电池证据不能继续显示 healthy；原其它 critical/offline 负面仍保留。schema 1 或未携带扩展的旧硬件不会仅因扩展未知而新增“一律故障”规则，未知本身也不证明电池健康。

[FleetHealthCenter](FleetHealthCenter.md) 在 FTC 合并前验证实际 Registry 快照，再独立保守合并明确负面和未知；既有电量别名、当前来源和过期机制仍由同一解析器核对。该增量不认证电池真实性、物理构型或性能，不新增设备调用、审批或飞行命令。仅完成源码与静态读回，未编译、测试或运行。

## 453：实际构型来源与完整本地基线

新增独立于电池来源的私有 `bindNativeConfigurationSource(id, actualVehicle, submittedConfiguration)`，仅实际 Swarm friend 能在成功提交描述后调用；公开描述、metadata、护照或 QML 不能建立这个绑定。配置采集沿 [FleetNativeConfigurationObservation](FleetNativeConfigurationObservation.md) 的原完整配方，不以清单中的声明自动代替当前本机观察。

私有绑定保存原 Vehicle/component、ParameterManager、VehicleLinkManager、MVM/model、PRIMARY weak owner、原生 VERSION 来源 epoch、机型/固件原 enum、记录 epoch、完整 typed 配置、配置观察 epoch/revision 和当前能力上下文。稳定的同源/同配置/同记录心跳不改变 bindingEpoch；失联、解绑、同编号换源、PRIMARY/管理器/版本来源变化或记录/批准代次变化不能通过 A→B→A 恢复旧绑定。VERSION epoch 是生命周期标识，不是 VERSION 可用性或真实性认证。

三个接口均只供原生 C++，不新增 Q_PROPERTY/Q_INVOKABLE：

- `nativeConfigurationEvidence(Vehicle*)` 和 `nativeConfigurationEvidenceCurrent(Vehicle*, expectedEvidence)` 返回 exact3 `{ok, reason, evidence}`。失败 evidence 为空且 reason 非空；成功 reason 是明确空字符串。Current 会实际重采并完整 typed 比较原 evidence，不能将传入 JSON 当作实际来源。
- `nativeConfigurationEvidenceMemoryCurrent(Vehicle*, expectedEvidence)` 返回 Bool，仅用于全部可回调采集之后的整组原依据尾核。它不虚派发插件、不调用外部 fence、翻译或 Link 虚方法，不借用电池投影。源内存/记录/基线不再对应时返回 false。

成功 evidence 为 exact13：`schemaVersion=1`、`source='native-current-configuration-observation'`、`vehicleId`、`bindingEpoch`、原 exact6 `registryContext`、原 exact14 `configurationBaselineEvidence`、`currentTypedFingerprint`、`configurationFingerprint`、`bindingMatchesCurrentObservation=true`，以及四个 false 标志 `hardwareIdentityAuthenticated`、`physicalEnvelopeCertified`、`performanceCertified`、`flightCommandReleased`。计数/ID沿原 native Int，epoch/指纹/十进制代次沿原 QString。

只有当前完整配置与原提交值、Registry 完整 typed 当前观察及实际本地批准基线相等才成功。不是仅凭 fingerprint 字符串批准：本地基线为空、被撤销/重新批准、历史丢失完整性、导入恢复的声明都不能通过。即使旧字段文字恢复，原观察/批准代次也必须一致。JSON 构型指纹与 typed 指纹保留各自用途，不与启动配置指纹混同。

注册入口未改变原容量上限。当前绑定不序列化、不放入普通 capability 快照；clear/注销/成功导入触达记录会清除绑定。完整替换导入清全部，增量导入仅清触达行；失败导入不清。清理先提交移除原绑定/记录，再断开原连接句柄，避免 disconnectNotify 重入擦掉后来建立的新绑定；提交后通知不被解释成撤回已发生的操作。连接建立逐次检查实际对象及绑定 epoch，未完成 hook 建立的绑定不可读。

已知原配方的 APM `FRAME` / 潜航 `FRAME_CONFIG` 依赖另存原 Fact、名字、组件及 typed rawValue；原 Fact 变化/销毁或原组件添加 Fact 立即退休绑定，尾栅只读原 Fact，不重新查参数。添加通知不解引用未受保护的原始指针，采用原组件保守退休。缺依赖时不建立绑定；安装 hook 后才完整重采并公开 ready 状态。当前原生重采失败/不同也按原 epoch 退休绑定，避免随后恢复 A 值复活旧依据；只退休运行来源，不改旧配置或批准记录。

私有来源同时固定实际 FirmwarePlugin。APM Copter 的共享 coaxial bool 由原实际类的显式限定、非虚调用读取（当前实现仅返回成员值），防其它机回调改变共享状态后旧证据继续可用。禁用 APM 插件的构建不引用该实现且不给相关不受支持分支绑定；不扩大公开 evidence 字段。

此接口不检测未上报物理改装，不认证工程包线或真实动力性能。任意第三方插件额外内部状态不在已知固定依赖之外被自动认证。其余限制见观察模块说明。仅源码和静态检查，未编译、测试或运行。

## 431：当前 Vehicle 电池遥测与工程档案分离

Registry 的普通描述、metadata 或能力护照不能建立原生电池来源。只有 `SwarmController` friend 可以调用私有 `bindNativeBatterySource(systemId, Vehicle*)`；调用方须在实际 Swarm/MultiVehicleManager 所有权与对象身份复核后绑定。Registry 保存 `QPointer<Vehicle>`、当前 `vehicleRecordEpoch` 和新的绑定 epoch，不延长 Vehicle 生命周期，也不持久化这一关联。

重复绑定同一对象和同一记录不会重复通知；更换或解除绑定才通知 `registryChanged` / `healthChanged`。解绑再绑定相同对象仍形成新的绑定身份。注销和 clear 清除绑定；成功完整替换导入清空所有绑定，增量导入只清除触达记录（包括仅更新批准的记录）的绑定。失败导入不改变当前绑定，未触达行不因增量导入丢失原来源。

### 快照出口

`vehicles()`、`vehicle(id)`、`onlineVehicles()`、`vehiclesWithCapability(...)` 均通过同一只读投影出口。每行先复制静态记录，再读取实际绑定对象的 `fleetBatteryTelemetryObservation(3000)`。固定 3000 ms 是本机接收新鲜度上限，不随可配置的长心跳超时一起延长，也不是远端采样时间或可达性保证。

每个公开行都带 `nativeBatteryEvidenceRequired=true` 与 `nativeBatteryEvidence`。后者沿用 Vehicle 的 exact20 原生 DTO（逐槽 rows 最多16项；schema/行形状按上方 460 契约）；Registry 依据完整共享校验后的顶层 known 状态投影：

- `batteryPercentKnown=true`：覆盖行的 `batteryPercent`；否则固定 `-1`，不回退描述、历史档案或显示 Fact。
- `remainingSecondsKnown=true`：新增顶层 `remainingFlightSeconds`；未知则该顶层字段缺失，绝不把未知 `-1` 当作合法直接剩余秒数传入旧消费者。
- 静态 `enduranceSeconds` 声明仍保留，它不因本次绑定变成实测续航，也不替代原生电池身份/新鲜度门禁。

没有绑定、绑定对象已失效或返回 DTO 不适用时，提供同结构的未知 evidence：`sourceCurrent=false`、两个 known=false、数值未知 `-1`、空 rows、空 sourceEpoch、counter `"0"`、`readonly=true`，认证、性能认证与命令标志均为false。Registry 的受控 `aggregationState` 为 `registry-native-source-unbound`、`registry-native-source-unavailable`、`registry-native-observation-invalid` 或临时重入时的 `registry-native-read-reentrant`。这些不是飞行故障结论，而是该次本地投影的来源/可用性说明。

### 生命周期和只读性

原生 getter 当前是本机内存连续性与时间投影，不调用 Link 虚方法；Registry 仍在该边界保留防护：不跨调用持有 hash 行引用、指针或迭代器；返回后复核 Registry、源 Vehicle、记录 epoch 和绑定身份。若记录或绑定在调用中变化，不返回借用旧来源的部分行。多机出口还复核整组绑定 epoch，任一行失效时整组返回空列表，而不是混合新旧来源。

同步重入时仅本次电池投影为未知，不再次调用原生 getter；正常返回后解除 guard，不把只读重入永久记录成源失效。该防护不使快照成为跨机原子遥测，更不保证返回后来源永远不变。

健康评分只在副本上复用原来的评分算法重算；原带通知的 `_recomputeHealth` 与只读 `_recomputeHealthValue` 共用同一逻辑。健康/在线计数和 `fleetHealthSummary` 从动态快照聚合，避免将先前描述缓存的电量评分称为当前原生健康。460 在此基础上加入当前明确电池负面及必需证据不可用处理；未知不被当作已报告故障，也不能借其它正常缓存显示为电池已确认健康。

既有 `vehicleHealthChanged` 单行通知仍使用缓存行评分；它不是原始电池更新事件，订阅者不能仅凭该信号值判断原生电池当前可用。本批动态证据的消费者须读取上述公开快照；Registry通知与实际收包并非一一对应。

### 导入和导出

顶层和 metadata 中的 `nativeBatteryEvidenceRequired`、`nativeBatteryEvidence`、`remainingFlightSeconds`、`availableFlightSeconds` 为 Registry 保留投影字段；普通描述、合并和护照导入会剥离，导出也不保存。不会将坏 metadata 类型转换为空 map 来绕过原校验。其它允许的静态声明和历史 `batteryPercent` 可继续存档，但公开快照始终由原生投影覆盖，历史值不能恢复 live known 状态。

构型基线、能力工程采纳、当前账号与飞行审批沿原独立机制处理；电池来源绑定不批准构型，不检测所有实物改装，不认证电池真实性、真实性能或飞行许可，不发送任何命令。

本轮仅源码修改、静态读回与文本检查。未编译、测试、运行 helper、应用、仿真或设备；工程师仍需验证收包/过期、来源切换、解绑再绑、重复消息、导入触达与失源场景。

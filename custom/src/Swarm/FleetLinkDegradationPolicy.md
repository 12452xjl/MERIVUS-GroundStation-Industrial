# FleetLinkDegradationPolicy

## 507：第五相机review仅增加独立报告诊断

旧evaluate20不变；evaluateNativeReception三／四参兼容，第五可选cameraReview缺失时合法unknown。输出24=原23＋localCamera18；闭wrapper7／native22／row4／Info15／Status15，只在同vehicle／record／binding且Info与Status均fresh并本地revision匹配时计报告数。相机源当前性、年龄和饱和分别判断，不把单个新Status刷新旧Info。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

[完整合同](../../../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504：可选独立RTT review只补唯一latencyMs

旧evaluate20保持；evaluateNativeReception三参兼容，新增第四可选actual latencyReview。新结果23=原22＋localLatency10；无第四参数合法unknown，提供时measurementSource为native-local-reception-with-latency-review。闭wrapper7／native26，仅同车／record／binding、两源current、complete／issued／response／age≤30秒才填原roundTripMs为latencyMs。

本地交接→RAW耗时含排队、远端处理和投递，不是纯网络延迟或时钟同步。capacity／quality／loss仍未知，单次RTT不使normal。非法review／绑定矛盾不从metadata补齐，也不授权限；helpers不发布中间Policy，最终emit后只返回局部值。

实际Guided原五秒块只读取原Swarm私有Service，不启动或续期探测；source／lifetime在每次调用与发布后复核。完整[26／7／23／10合同](FleetLinkLatencyCapture.md)、[44项全未执行](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503：本地接收观察独立消费，不伪装四QoS

新增evaluateNativeReception(review, online, task)，只把实际online传原策略构造；四QoS保持unknown，完整接收窗不使新路径normal。旧evaluate接口与20字段不改；新结果多measurementSource和localReception固定10字段，完整原生观察22／window9保留供诊断。

闭wrapper7／22／9值形，允许QML桥接后的有限精确整数number，拒绝字符串／布尔数字与非有限值，receipt保留uint64十进制String。输入形状不认证来源；实际Guided原五秒块只从原Registry取得新review，不用metadata、RSSI、旧seq-loss或接收字节率填可用带宽／RTT／丢包。

新旧入口完整构造后仅一次发布，emit后只返回局部值；Root另持plain lifetime及原提供者，在调用／通知后复核。recommendationOnly、manualConfirmationRequired、flightCommandReleased=false保留，不接自动调频／视频／速度／补传。参见[完整合同](FleetLocalReceptionObservation.md)和[40项全未执行](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502：可见配置仍与自动策略分离

新[面板](../../res/Merivus/FleetOptionalTelemetryPanel.md)分三步人工操作，仅固定62／74／83统一正间隔，不自动把本Policy建议发成511。Policy源码不改，原生独立purpose、二审、最早30秒及partial不回滚合同不变。真实链路测量、视频／速度联动、补传和效果核验仍待开发；目标14未完成。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501：策略与独立配置分离

本Policy源码不改，旧五秒只读建议不自动发包。[Configuration](FleetOptionalTelemetryConfiguration.md)是另行显式确认／独立双账号批准的地面后台链，仅固定62／74／83、同一正int32精确REAL32且数值不增频。不能把Policy全telemetry.rateHz直接当全部MAVLink流setter，不降低P0证据／控制协议。

三次511逐项510回读并最终整轮510，只给报告值匹配；原30秒含审批，失败保partial不回滚。真实带宽／延迟／丢包测量、视频／速度联动、恢复补传和效果验证仍待开发，目标14未完成。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500：显式采集与五秒只读策略保持分离

Policy.h/.cc不改，原Guided五秒policy／Registry review消费不自动发510。新[Capture](FleetOptionalTelemetryCapture.md)需要独立主账号、精确capture action、原Root意图和明确确认，只读62／74／83；收到报告不授予降频、停流或配置权限。

本批没有新可见按钮或Apply。未来511配置必须另有实际action／必要二审、固定同源采集、逐步写与回读和部分已发结果；不能把capture、旧refresh/start凭据或全telemetry.rateHz提案借为写权限。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

[24路径与40未执行交接](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前前置：实际配置请求使旧只读基线失效，Policy仍不执行

Policy.h/.cc、Guided五秒消费与原recommendationOnly／manualConfirmationRequired／flightCommandReleased=false不改。现有正rateHz仍只产生数值提案；新共享lane没有把策略接成自动配置器。

本车默认component的任意511只在最终actual write前，以同UUID一次预留推进不回绕mutation generation，并全清498三report及接收窗；ledgerEpoch为UUID:十进制generation。writeDepth贯穿write与通知，临界内原包准入／getter不可当前。未发送拒绝不冒称已配置；已可能write的代次／requestIssued不回滚，耗尽保守停止而非重用。498观察15／Registry review21／comparisons6保持，后续只读重采会看到旧基线失效，既有值快照不自动变成实时通知，须新被动资料与新完整5秒窗，不等于实际生效。

510／511／512现走同一Vehicle级跨component专用lane：外调前预留UUID／不回绕serial，固定原提供者QQ、PRIMARY弱控制块与实指针、transport代次及原固定期限；原nativeVersion直接入队也接入，但保原地面／参数／实际授权fence和单次3秒，不借generic条件放宽。generic初连不要求MVM已登记或握手已完成；版本资料首次锚定与实际transport变化分开，native批准依据不自动rebase。

Vehicle公共raw sender在plugin前后拒510／511／512旁路，正式请求走专属通道。此覆盖不包括MAVLinkProtocol转发、移动Taisync字节桥、外部GCS或固件内部调频；本批不改变转发策略，不认证实际发布率、丢包或带宽。

本地common协议头822–827明确GET_MESSAGE_INTERVAL510用param1指定流并返回244；512参数2用途须由对应消息定义。不能因共享lane已支持510／512，就假定当前固件支持“512请求244、param2选择62／74／83”；新Fleet主动capture仍需独立窄scope和工程兼容验证。本批未读取或修改PX4飞控。

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

[18路径](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[42项全未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前消费：正策略值只进入实际原生基线的只读审阅

Policy.h/.cc不改，recommendationOnly／manualConfirmationRequired／flightCommandReleased=false仍保留；不是自动降频器。

现有custom Guided五秒策略快照把Policy正rateHz换成ceil(1000000/rateHz)的只读间隔提案，经实际私有_swarm.fleetRegistry调用并附optionalTelemetryRateReview；0／未知不变为停流／固件默认。Policy源码、P0流、旧PID／ESC与发送器不改，公开facade不暴露原生setter或新增apply。

Registry仅从现有friend建立的实际Vehicle绑定读取一次原生快照，前后核记录与绑定，不以metadata／护照／调用者Map替代。null／0仅无提案，正间隔须int32整数且精确REAL32；三条当前新鲜正基线都已知、提案间隔均不小于原报告间隔，才给数值不增频结论，仍snapshotOnly且所有许可false。

每流5秒尾窗、最多512个单调接收时刻；原来源绑定满5秒、该流曾收到且最近包不超过30秒、无相关接收栈／探测中状态和窗内容量丢失才可complete。完整窗可以0帧，仅表示本地该5秒没收到；从未收到过不能complete。均值是本地收包数／5秒，最大内部间隔不含窗两端空隙；不推算发送率、链路丢包或停流。

实际[Registry合同](FleetRegistry.md)说明完整观察15／review21／comparison6。报告可缺，尤其纯被动不保证收到244；缺资料只给未知，不暗发查询或用metadata当原生基线。旧链路四指标和策略阈值不被本地窗口替代，本批不认证带宽／延迟／丢包或实际降频。

本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

[18路径清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前前置：修现有interval消费者，Policy仍不执行

本Policy.h/.cc没有修改，evaluate输出及recommendationOnly／manualConfirmationRequired／flightCommandReleased=false原边界保持。当前rateHz不能直接变成所有流的511请求，metadata／Registry估计也不是当前原生带宽／延迟／丢包测量。

本批修现有511（SET_MESSAGE_INTERVAL）消费者，不接通Fleet自动降频。StreamConfig改非零不回绕requestId、唯一pending、先pop后callback和迭代pump；单次最多64实际callback，失败保possibly changed供后续显式选择／restore。callback副本在私有weak lifetime最终尾核前析构，禁copy/move，销毁后不再访问helper。interval=0仍仅固件默认，不是原配置回滚。

公共511 entry固定原VLM／Protocol／Plugin、PRIMARY QQ与weak控制块及实际pointer、原source epoch和本地UUID；复用构造期持久pin，不从已销毁raw重建。外调后入队前再核duplicate/quarantine，实际发送前核原UUID及完整payload，失败按原UUID重找。ACK只接同原物理来源，IN_PROGRESS不续期；511超时加入原隔离，不新增每个成功终态冷却。

PID与ESC各自一次handler/context消费自己的本地请求，删除原任意live511推进PID／ESC的分支。STATUS5Hz→INFO1Hz、原PID两消息100Hz及手动默认恢复入口保持；拒绝／超时不自动续链。same-source迟到重复wire ACK仍可能歧义：协议不回显messageId，本地ID／UUID／来源核对不是wire因果认证，accepted也不证明实际频率、带宽或安全效果。

真正的目标14仍需独立实际Session/角色scope与必要二审、当前原生链路证据／期限、闭合optional白名单、原MESSAGE_INTERVAL244基线/回读及收包频率窗口。不能借只读refresh或start/capability凭据，不能照搬Policy的全telemetry.rateHz，更不能降频P0证据或控制协议；本批没有新apply UI/入口／权限／自动飞行动作。

本批[6源码＋12文档=18路径](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。改到的PX4FirmwarePlugin是GroundStation适配器，未改PX4飞控；没有新增权限purpose、GUI或策略apply开关。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为原模块合同。

`FleetLinkDegradationPolicy` 是地面站侧的纯策略模块，用于把已归一化的链路指标转换为可审阅的降级建议。它不连接 `Vehicle`、MAVLink、Mission 或链路发送器，也不会自动修改视频、遥测、编队速度或任务。

## 输入

```qml
policy.evaluate({
    qualityPercent: 64,
    bandwidthKbps: 900,
    latencyMs: 180,
    packetLossPercent: 2.5,
    linkState: "connected"
}, {
    taskPriority: "high",
    videoRequired: true,
    telemetryRequired: true,
    formationRequired: true
})
```

链路字段可使用 `linkQualityPercent`、`availableBandwidthKbps`、`roundTripLatencyMs` 和 `lossPercent` 别名。缺少指标不会被当成健康，越界指标会被标为无效并进入保守模式。`linkState`/`state` 只接受已定义枚举，任务的 `videoRequired`、`telemetryRequired`、`formationRequired` 必须是布尔值；非法状态或隐式布尔转换会使 `inputValid=false` 并保持保守输出。任务优先级支持 `low`、`normal`、`high`、`critical`。

## 输出

- `level`：`normal`、`watch`、`reduced`、`critical`、`unknown` 或 `link_lost`。
- `video`：是否建议保留视频、码率、帧率、分辨率和降级动作。
- `telemetry`：实时遥测档位、速率和是否只保留关键数据。
- `formation`：编队速度比例和 hold/降速建议；`0` 表示不建议继续实时编队动作。
- `dataPriorities`：安全事件、ACK、遥测、编队状态、视频和批量日志的优先级与处置动作。
- `recovery`：稳定采样窗口、遥测/ACK 证据补传、过期视频丢弃和日志批传建议。
- `riskExplanations`/`riskCodes`：每项风险的可读解释和机器码。

所有结果固定包含 `recommendationOnly=true`、`manualConfirmationRequired=true` 和 `flightCommandReleased=false`。工程师接入 UI 或任务审批时必须保留这三个边界；策略输出不能直接接入命令发送路径。

## 阈值与限制

策略使用保守的质量、带宽、延迟和丢包阈值，仅用于地面站提示和预演。阈值不是 PX4 链路安全参数，也不能证明实际无线链路、编队控制或飞行安全。真实带宽测量、消息限流、视频编码切换、补传队列和机载安全行为仍需工程师在具体链路协议上实现并验证。

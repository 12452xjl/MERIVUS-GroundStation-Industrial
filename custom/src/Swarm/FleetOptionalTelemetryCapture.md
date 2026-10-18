# FleetOptionalTelemetryCapture：显式只读遥测间隔采集

## 502：显式只读采集已有可见消费者

新[Panel](../../res/Merivus/FleetOptionalTelemetryPanel.md)由指挥中心／地图规划页显式打开并确认，仅读actual Root的captureSnapshot，调用既有request／cancel。只真实返回5键及当前24同ID／代次／机号才归本页；成功terminal保原归属供另一步审阅，不从观察到的历史接管。打开和计时器不自动采集。原Capture源码、current24／rows4／query32／result5及只读action不改。

终态remainingMs=0不能用作新30秒；界面只用自己最早调用+30秒作保守显示／撤销，原native单调期与新配置fence仍限制。关闭／换源／改机号精确取消原ID，晚返回不复活。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501：给独立配置封存一次原生Basis

Capture本身仍只发510、不发511。新成功三项在精确释放原native ctx之前调用seal；只允许真实same owner三terminal query封存一次不可序列化Basis，旧500已release／历史Map不能补造。实际Swarm-owned [Configuration](FleetOptionalTelemetryConfiguration.md)可在原期限内一次接管；接管前即烧handoff，后续失败或取消不能重用，须新Capture。

纯sourceFence保原Capture generation／request／Registry绑定／PRIMARY／Root／当前Episode／Role及最早30秒，但不再要求Capture active或旧telemetry行在own511后仍current。配置Run自己核精确own mutation与新回读，不能据此放宽外部换源。handoff后仅忽略正常审批的Session.changed通知；Role／物理源改变仍sticky退休，配置服务独立核真实Session／两账号和期限。服务原current24／result5／rows4／query32不扩字段。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方500为Capture原合同；其中“尚无独立配置”是历史状态，现由新Configuration承担，不改变Capture只读边界。

## 500：代码链路与交接边界

本批新增单车主动读取流程，供现有地面站后台桥接显式调用；没有新增可见按钮、自动查询或配置 Apply。只读取固定三条可选流：NAV_CONTROLLER_OUTPUT（62）、VFR_HUD（74）、ATTITUDE_TARGET（83）。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

## 原生对象与授权

SwarmController 私有构造并持有本服务，ownsOptionalTelemetryCapture 核实际对象、Registry、parent 和线程；不能从 QML 新建任意服务或导入一个 Map 充当 Vehicle。C++ optionalTelemetryCaptureVehicle 只读取同 Registry 的原生 Vehicle 绑定、vehicleRecordEpoch 和 bindingEpoch；此处复用对象绑定，不把电池健康当读取许可。

requestCapture(actualSession, actualRoles, actualRoot, scopeToken, vehicleId, confirmed) 要求实际 FleetOperatorSession、FleetRolePolicy、Root、当前 Episode 和明确确认。精确 action 为 fleet.telemetry.interval.capture。默认 planner、operator、safety_officer、maintenance、commander 可申请，administrator 沿用 *；observer 不自动获得。此有界 READ 用途是单账号例外，不添加或消费 Session 的上传、启动、构型或能力二审凭据。实际配置以后必须另设 action 和独立批准。

Root 使用既有 controller-call bridge，冻结原意图、scope、五字段主身份和九键政策；native 每次实际发送前直接回到同一私有意图核验，并重新核实际 Session。scopeToken 和公开状态只是值，不是许可。Root 销毁、当前 Episode 更换、账号／政策或 Registry 绑定变化均不得自动恢复原意图。

## 时序与真实发送

总期限从请求最早有效入口的本服务单调时钟起固定30秒；每项4秒。接纳只启动100毫秒轮询，Root 先持有返回的 requestId，下一轮才调用 Vehicle，不在 RAW 回包栈里发下一项。

Vehicle 的非 invokable C++ 口为 beginFleetMessageIntervalQuery、fleetMessageIntervalQuerySnapshot、cancelFleetMessageIntervalQuery，并有独立纯内存的fleetMessageIntervalQueryTransportQuiescent。后者不依赖账本是否已绑定：最早RAW入口持有整栈深度，嵌套事件循环触发的tick先核原期限，再遇忙就等下一轮，不提前做OS核验、不误判来源失效或延长期限；真正来源失效仍终止。深度耗尽仅拒新采集，不丢原飞行状态包。每项仅 COMMAND_LONG / GET_MESSAGE_INTERVAL（510），param1 为当前固定 messageId，其余参数0；只发送一次。510、511、512 共用现有 Vehicle 排他通道。native 单项期限从原预留开始固定4秒；accepted ACK 后最多另等1秒，但只能缩短原期限。

严格准入保留原 PRIMARY 控制块与实指针、构造期 provider QPointer、transport、版本语义、mutation generation 和原参数上下文；仍要求已初连、未解锁／地面、当前地面原始证据、参数稳定及相关传输空闲。Link／plugin／账号回调后重核，最终 actual write 前再核授权及完整载荷。准备期间的报告不算本次结果，基线在最终 write 前冻结。

ACK accepted 和对应原始 MESSAGE_INTERVAL（244）必须都到，任意顺序均可；IN_PROGRESS 不重发、不延长原截止。RAW 入口在任何外调前冻结原 query／UUID／writeStarted，原 P0 围栏建立后才处理。报告必须同原 Protocol／物理 PRIMARY／sysid／默认 component、合法 wire 长度和精确 message_id，receiptSequence 晚于 write 前基线。

## 三项的一致性

- 整批固定原 Vehicle／Registry 绑定和原来源、版本、transport、mutation；所有已采 native query 在后续项及最终发布前仍须当前。
- 未绑定的被动账本只可在原同源初次绑定处分明地转移一次；坏 wire、未知清理或第二次换代不能被当正常初次绑定而重基。
- 已采报告的 contentRevision 和 interval 保留，A→B→A 不能令旧报告复活；同值新 receipt 不能延长整批期限。
- −1 保留 disabled，0 保留 not-available；低于−1是 invalid 诊断，不给整批成功。收到特殊值不等于获得正频率基线。
- native 最多保留三条原 owner 记录，终结时服务先保存历史快照，再精确释放；不得清除别的命令或后续新操作。

## 取消与历史事实

取消、来源失效或截止立即停止后续项。已实际 write／可能排队的事实不回滚；失败或取消保原隔离，不能立即把迟到 ACK 交给下一项。终态 native 记录的释放不触碰新 lane。

如果取消发生在 begin 的同步外调内部，返回后只能把同 generation／request／Vehicle 的结果补为原已退休批次的历史，不能恢复活动状态或污染新批次。readOnlyRequestsMayRemainInflight 是保守提示，不是已发数量或远端收到证明。

## 值合同与消费者

| 输出 | 形状与含义 |
| --- | --- |
| requestCapture 返回 | 恰好5键：ok、requestAccepted、requestId、reason、flightCommandReleased；ok只表示接纳 |
| service current | 恰好24键，包括原request／generation／scope、阶段、固定三项、rows、剩余时间及全false许可／认证旗标 |
| rows | 固定三行，每行4键：messageId、state、reason、query；未发的query为空Map |
| native query | 恰好32键；原source、写入事实、ACK及244、receipt／baseline／语义代次、期限和明确未证明旗标 |

service current 为纯值历史／进度，getter 不认证、不发包；contextMatches 也不进入 OS、Link 虚调或临时 strong ownership。Root外层投影另给workflowScopeMatches／workflowHistorical：切换Episode后保留的旧批次明确标为历史，其scopeToken不变；外层sourceToken仅供当前新采集准入，不能把旧成功领作当前Episode结果。准备阶段若先发布native ID再返回，只允许据该ID取消原意图，不借它接纳或恢复授权。新报告进入原被动账本，既有 Registry optionalTelemetryRateReview 与 Guided 五秒重采继续原15／21／6校验；既有值快照不自动变成实时通知，不新增配置许可。

## 未证明／未完成

510 是地面站随包 common 协议明确的查询方式，但静读不证明目标固件支持。拒绝、不回244或不完整必须失败；不自动假定512(244,param2)可索引查询。MAVLink ACK／244 无本地 operation nonce，本次接收仍不是无线因果、硬件身份或实际发布率认证；所有 responseAttributionVerified、actualTransmitRateVerified、availableBandwidthVerified、configurationPermitted、executionPermit、flightCommandReleased 等相应字段保持 false。

此模块不代表目标14完成，不修改视频、编队速度、P0证据、机载控制或飞控；进程转发出口、其它地面站及固件内部变化不由本读服务控制。新文件只登记 custom.pri，未新增依赖、qrc 或 QML 可构造类型。具体范围见[补丁清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

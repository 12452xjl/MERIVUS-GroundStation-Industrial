# FleetLinkLatencyCapture：人工单次定向往返耗时

## 504 已写源码与适用范围

504新增独立人工单次定向TIMESYNC往返耗时采集：实际主账号／Role／Root原意图→Swarm私有服务／原生Run→真实同车绑定review→Policy及可见面板。最早四秒包含授权核验和后续100ms轮询，不自动采集、重试或续期，不回退PING或广播。

当前入口只支持有新鲜原生版本、Heartbeat／landed资料的PX4多旋翼地面未解锁分支；不是任意固件、飞行中或所有异构飞机的RTT支持。原生还核完整初连及参数／Mission／FTP／startup／消息事务静默；条件不足明确拒绝，不请求新数据或放宽P0。

测量是本地writeBytesThreadSafe交接前至匹配响应RAW最早入口的单调耗时，包含I/O排队、通信、远端处理和本地主线程投递。不是纯网络延迟、单向延迟、时钟同步、可用带宽或丢包率，也不认证远端身份／wire因果。无响应不是丢包率。

## 为什么不使用单机定向PING

本仓库MAVLink PING定义中target_system／target_component=0才是请求；正值是回复地址。仅选择一条PRIMARY也可能由UDP fanout／路由触达多机，不能称定向探测。

采用本地TIMESYNC111定义允许的MAVLink2目标扩展：request tc1=0、ts1为本地正int64 timestamp、target为选中sysid／默认component。v1会截掉目标扩展，故输出MAV1、编码非v2 full18或plugin改变完整消息时拒绝；不修改共享通道版本来强制发送，不回退PING、广播或旧16字节回应。实际固件是否支持须工程师验证，源码存在不是兼容性证明。

## 原生Run与发送／接收

Run仅定义在Vehicle.cc，公开Map不能重建。prepare只建立ready，advance在后续timer才最多一次交接；snapshot／retiredSnapshot只读，cancel精确同owner／Run。服务由Swarm私有构造，无新可任意QML构造类型。

原提供者QPointer、PRIMARY实指针及weak控制块、sys/component、GCS地址、channel、版本／传输代次与参数来源冻结；最终授权fence在实际write前重核。标准编码与plugin处理后完整消息须不可变，目标扩展不能静默丢失；requestIssued在write前保留“可能已交接”，不宣称物理发送或远端收到。

nonce采用进程级锁保护的时间戳：首次墙钟纳秒锚定后只加单调时间与last+1，跨Vehicle不重置、不回绕。真实耗时另用Run单调时钟，不减远端tc1或墙钟；不称跨进程永久唯一或认证nonce。函数对象复制／析构也有QQ与原slot尾核，不覆盖后来请求。

RAW最早冻结原Run／nonce／issued事实和到达时间，在原P0围栏建立之后消费。只认原Protocol、本车默认component、原PRIMARY及原epoch；响应必须v2 full18、tc1>0、ts1逐位匹配、target精确原GCS。pre-send／同包后来建Run、旧nonce、异车或secondary不领成功。接纳前另以纯内存／单调时钟门复核原service的四秒期限及同一Run，不能因remainingMs取整或native准备入场偏差延长回包资格；不在RAW里做账号／OS核验。过任一原deadline不接纳，未回应保未知。

关闭、移除、版本／连接变化及真实armed／离地RAW退休来源，已complete也不因后来落地重新current。中央退休只改内存，不执行或析构外来fence；Vehicle死亡后的只读快照保留可能已发／已观察事实。独立RTT不全局阻断旧安全飞行动作，耗时不承诺隔离其它流竞争。

## 固定值合同

### 原生观察26

schemaVersion=1、reportType=fleet-link-rtt、requestId、vehicleId、componentId、active、state、reason、requestIssued、responseObserved、sourceCurrent、remainingMs、roundTripMs、nonce、versionEpoch、transportEpoch、snapshotOnly、authenticated、wireCausalityProven、availableBandwidthVerified、packetLossVerified、remoteClockOffsetVerified、configurationPermitted、executionPermit、flightCommandReleased、ageMs。

- 状态仅ready／waiting-response／complete／failed／cancelled；active仅前两者。
- 原生ID为String、nonce为正int64规范十进制String或空；不经JS number。remainingMs／ageMs为LongLong，前者0–4000，后者-1未知。
- roundTripMs为Double，未观察为-1.0，观察后0–4000；responseObserved必须有requestIssued。
- snapshotOnly=true；authenticated、wireCausalityProven、availableBandwidthVerified、packetLossVerified、remoteClockOffsetVerified、configurationPermitted、executionPermit、flightCommandReleased均false。
- active受最早剩余期限；complete历史同源且age≤30000可current，仅为诊断，不再允许发送或授权。

### Service current18／result5

current固定schemaVersion、requestId、generation、scopeToken、vehicleId、state、active、reason、remainingMs、observation、snapshotOnly、authenticated、configurationPermitted、executionPermit、flightCommandReleased、protocol、maximumAgeMs、requestAccepted。protocol=targeted-timesync-v2，maximumAgeMs=30000；state为empty／preparing／capturing／observed／failed／cancelled／stale。

requestCapture返回固定ok、requestAccepted、requestId、reason、flightCommandReleased:false。只有实际返回result5与同current18匹配才建立本页拥有关系；观察到共享current或preparing ID不授权。current中的requestAccepted仅表示已有本次尝试ID，不代替入口result5的接纳与后置核验。

### 实际读取wrapper7／Policy23

latencyReview(vehicleId)只从本service原Run及实际Swarm／Registry记录／binding读取一次native观察并尾核、深拷贝。wrapper固定ok、error、vehicleId、vehicleRecordEpoch、bindingEpoch、observation、snapshotOnly；失败observation={}，合法陈旧历史仍可读但不可作为已知当前RTT。ok不是认证或执行许可。

Policy保留旧evaluate20；evaluateNativeReception前三参兼容，第四可选latencyReview。新结果23=原22＋localLatency10；十字段为sourceKind、state、reason、sourceBound、roundTripKnown、ageMs、roundTripMs、observation、vehicleRecordEpoch、bindingEpoch。无第四参数合法unknown；提供第四时measurementSource=native-local-reception-with-latency-review，不提供则保持native-local-reception-only。

仅当两个review同vehicle／record／binding、两原生来源current、RTT complete／issued／responseObserved／age≤30秒，才把原roundTripMs填唯一latencyMs。闭7／26校验允许QML有限精确整数number，拒字符串／布尔数字、非有限值和来源矛盾，nonce仍String；portable Map不建立来源。quality、capacity、loss仍未知，所以绝不normal，不自动恢复视频、提高速度或发511。

## 独立授权、Root与可见入口

新增精确READ action fleet.telemetry.rtt.capture，默认planner／operator／safety_officer／maintenance／commander及administrator可执行，observer无权；无需二审，但仍必须实际主账号、原Role快照、当前Episode／Root意图、实时身份／WTS与明确确认。未改Session批准purpose或借用interval capture／configure证明。

Root分别持原scope／身份5／policy9、actual Swarm／Registry／服务及本次intent，入口经原runCurrent；preparing ID只供精确撤销。owner.active通知、native返回与controller-release后均核原寿命／意图；六参finalFence不做登录、不取得第二lease、不换来源。pending调用的同引用cancelState只允许sticky撤权，不从false授新权。

FleetLinkLatencyPanel在指挥中心和地图规划页各有入口。打开不采集；确认框按下／点击之间有独立代次，true→false→true、disabled／hide／换源均使旧按压失效。首清理只接受自身epoch+1及确认清除，之后仍核原full sourceToken，不能吸收外部新capture代次；晚返回只取消原owner／ID。关闭不撤回已发字节，公共历史不领权。

Guided原五秒周期只读取latencyReview，不能发起或续期；发布通知后仍核原提供者和Root寿命。Panel和Overlay明确“该次快照”与排队／处理含义，不把缓存当实时认证。UI/UX技能只以完整静态参考指导QGC控件、文字、换行、确认、取消和焦点可见；真实Qt布局、键盘、主题、辅助技术未运行验证。

## 交接与剩余

[精确31路径](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全部未执行](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](../../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

目标14仍缺完整链路容量／丢包来源、连续且有界的测量政策、飞行场景授权与支持、视频／速度降级联动、稳定恢复补传及真实效果验证。单次地面样本不是整个目标完成。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。

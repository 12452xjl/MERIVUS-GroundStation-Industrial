# FleetLocalReceptionObservation：本地主链路接收窗

## 504：独立RTT来源与接收窗分离

本原生接收22／window9及Registry wrapper7不变，不以收包内部间隔当RTT。新增[独立单次TIMESYNC采集](FleetLinkLatencyCapture.md)另走opaque Run26／实际服务review；只有同车／同record／binding的新鲜完整回显才给Policy唯一latencyMs。

Policy新结果23含localReception10和localLatency10，原三参调用兼容且RTT未知；capacity／quality／loss仍未知。原接收字节率始终不是可用带宽，接收零窗／缺回显也不计算丢包。两个模块均无自动511／视频／速度动作。[44项全部未执行](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503 实际源码链与边界

503将实际PRIMARY／本车默认component的原始解码帧接到独立五秒接收窗，经Registry实际绑定读取、Policy独立消费、Guided原五秒周期和指挥中心只读摘要。记录接收帧数、编码MAVLink字节率与最大内部间隔；不把它们当可用带宽、RTT、丢包率或远端发布率。

这是地面站的被动观察，不新增PING、流请求、配置或控制命令。原500／501／502显式采集／配置、独立授权、二审和期限不变。接收窗不是新的认证、配置依据或飞行许可；四项QoS仍未知，完整窗也不能使新策略路径变为normal。

## 原生来源与生命周期

- Vehicle非Q_INVOKABLE const getter为fleetLocalReceptionObservation()；Registry从实际Vehicle绑定调用一次，不从metadata、护照或调用者Map建立来源。
- 只接实际Protocol sender、本车sysid、默认component、同一PRIMARY实指针及weak控制块。原providers QPointer、transport／version epoch、机型与firmware type均须保持。
- RAW最早冻结原包、独立单调接收时刻、来源和全局receipt。此处只读内存；三次Link检查延迟到原P0围栏和prepare之后，不新增P0前Link虚调用，不因观察失败丢弃旧RAW。
- 整个原RAW未退栈、probe重入、来源失效、从未观察、最新接收超30秒或计数耗尽时sourceCurrent=false。早退未记录的原包按原epoch／receipt清窗，不能保留有缺口的完整前缀。
- PRIMARY、成员／连接、transport、version、机型、关闭、prepareDelete、析构触发清窗并换opaque epoch；原全局receipt不重置。旧栈尾核只清原epoch／receipt，不覆盖后来来源；达到uint64上限永久停用此观察，不回绕，不停止旧消息路径。
- getter只读内存／同线程单调计时与现有来源值，不调用Link虚方法、不weak.lock、不发信号，不发认证／文件／网络请求。跨线程返回未知诊断，Registry只在owner线程读取。

## native观察：固定22字段

| 字段 | 原生类型／含义 |
| --- | --- |
| schemaVersion／reportType | Int 1／String fleet-local-reception |
| vehicleId／componentId | Int，本车1–255／默认component或未知0 |
| sourceEpoch | String，本地opaque来源代次，不是认证 |
| receiptSequence | canonical uint64十进制String；跨来源不重置 |
| sourceCurrent／observed | Bool；来源当前性与是否有历史接收分别表达 |
| newestAgeMs／boundAgeMs | LongLong，-1未知，其余本地单调年龄 |
| maximumAgeMs | Int 30000 |
| window | 下表固定9字段 |
| snapshotOnly | Bool true |
| authenticated | Bool false |
| availableBandwidthVerified／latencyVerified／packetLossVerified | 三个Bool，均false |
| remoteTransmitRateVerified | Bool false |
| configurationPermitted／executionPermit／flightCommandReleased | 三个Bool，均false |
| reason | String，原生诊断原因 |

## window：固定9字段

| 字段 | 原生类型／含义 |
| --- | --- |
| durationMs／maximumRetainedFrames | Int 5000／4096 |
| retainedFrameCount | Int 0–4096，尾窗中实际保留条目数，仅诊断 |
| complete／saturated | Bool，完整覆盖／本地有界保留截断 |
| receivedFrameCount | Int；完整窗0–4096，不完整-1 |
| receivedFrameBytes | LongLong；完整窗编码帧字节总数，不完整-1 |
| observedReceiveKbps | Double；完整窗bytes×8／5000，不完整-1.0 |
| maximumInternalGapMs | LongLong；完整且至少两帧时相邻样本最大间隔，否则-1 |

同源绑定满五秒、曾观察过、最新样本不超过30秒、原RAW全栈静默、来源未变且无容量截断，才可complete。窗口采用(now−5000, now]；满五秒前的样本不属于尾窗，最大内部间隔不含窗两端空隙，不能命名为最大沉默时间。

完整窗可为0帧／0字节／0Kbps／gap=-1，只表示这个本地五秒尾窗没有目标帧，不证明停流或丢包。从未收到过的源不能给完整零窗。饱和期间sourceCurrent可true但complete=false，四项计算值仍未知；不把4096截断前缀当全窗，也不把本地容量截断称无线过载。

字节数使用本地MAVLink framing常量：v1含magic／header／checksum，v2再按signed标志计签名块。不是串口／UDP／TCP／IP开销、重传、物理吞吐、剩余容量或远端发送率。未知／被Protocol过滤的帧不在此观察范围，签名存在不等于已认证。

## Registry实际绑定与wrapper7

nativeLocalReceptionReview(systemId)返回固定ok、error、vehicleId、vehicleRecordEpoch、bindingEpoch、observation、snapshotOnly。失败observation={}；成功只表示真实绑定getter及值形读取成功，sourceCurrent=false的合法历史诊断仍ok=true。

来源沿现有原生绑定记录；getter前后及固定DTO深拷贝后复核同Vehicle／recordEpoch／bindingEpoch。闭键校验native22／window9，原生Int／LongLong／Double／Bool／String及公式、零／未知、饱和与年龄必须一致；不能导入Map来建立绑定。

## Policy与实际消费者

evaluateNativeReception(review, online, task)仍只给建议。在原20字段输出外增加measurementSource=native-local-reception-only和localReception固定10字段：

sourceKind、state、reason、sourceBound、windowComplete、saturated、observed、observation、vehicleRecordEpoch、bindingEpoch。

state只为complete／incomplete／unavailable；sourceBound仅该次值中的来源检查结果，不是身份认证。observation保留合法原22字段，失败则{}。Policy闭键核wrapper7／22／9，允许QML桥接后的有限精确整数number，拒绝字符串数字／布尔数字／非法比例；超出JS安全整数域的年龄保持不可用。receipt仍String，绝不经JS number。

Policy只把实际online转换为connected／offline传入原策略构造，四QoS不由metadata／RSSI／旧seq-loss／接收字节率补齐；新路径只能unknown或link_lost，不自动恢复视频、提高速度或降频。完整零窗、容量截断与非法审阅有独立解释，额外原因权重0，不假造新的概率模型。旧evaluate接口及20字段合同保留；两个入口都完整构造后一次发布，直接信号可能销毁对象，emit后不读取this。

Guided原五秒块逐机取实际Registry新review，再调用新Policy；既有三条可选遥测的只读数值提案仍另走optionalTelemetryRateReview，不发511。该块冻结plain lifetime及原Swarm／Registry／Policy，getter、Policy通知返回、可选流review及快照发布通知后均先核guard与同一提供者，销毁／换源即停止本周期续作。指挥中心只统计该次快照的完整／不足／不可用，PlainText换行，不把缓存标成实时认证。未新增Timer、可构造QML服务、pri／qrc依赖登记或新权限。

## 旧RSSI的有限修复

Swarm只在原legacy近似分支额外排除−1未知哨兵，避免被换算成约99%的链路质量。非3DR路径也可能经int8转换，其他负缓存不证明物理dBm单位、来源或新鲜度；本批不完成RSSI可信化。旧Protocol按channel／sysid／compid的序号loss和bytesSent也不进入新Policy，均不当可用带宽／RTT／丢包实测。

## 工程交接与未完成范围

[精确23路径](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](../../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

目标14仍需可信RTT／丢包／容量来源、视频／速度降级联动、稳定恢复与补传和真实效果验证；不是所有链路测量已完成。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。

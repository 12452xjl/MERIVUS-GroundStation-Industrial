# MERIVUS MAVLink ACK/操作身份审计

## 516 当前：近期原生SOC下降窗下调已有续航预算

新增独立native SOC下降窗：实际原PRIMARY收包、最早RAW本地时刻和原complete尾核后留有界采样；30–120秒、至少下降2个百分点、原报告gap≤3000ms、最多256点才可发布。flat不刷新已发布终点／速率，最新RAW须≤3000ms，下降终点须≤30000ms；模型统一扣两者较大年龄。上升、未知、坏时钟、用途／类型或来源断裂清窗，不从Fact缓存拼接。

实际Registry在同Vehicle／recordEpoch／bindingEpoch下读取旧电池20字段与新top25／window8，剥离静态root／metadata同名键。新共享校验不认证Map；坏新证据明确拒绝，合法warming／stale／unavailable不补模型。报告SOC不是实测电功率或物理电池身份。

Scheduler只下调已有known可用秒数及已有known电量投影，随后重算缺口、成员资格及替补建议。严格性能声明与原故障门保持；assignment18/21、编译备用预算和旧电池schema不改。可选nativeBatteryDrainAdjustments逐行14字段保完整窗口、应用前后与原因，并沿原完整energyPlan归档／报告／515明细；普通规划页只新增依据标签。

Root通过exact6共享值桥消费语义身份，排除自然age及重复receipt抖动；窗口失效／换源／重建使旧审阅不当前。批准期还核实际扣龄预算是否覆盖原所需秒数、是否新跨原替补阈值，不重写旧账单或自动批准。估算、上传、机载接纳与最终StartPermit不等价。

本批11源码／接线＋18文档=29既有路径，无新增文件、资源、wire、外部依赖、数据库schema、权限或PX4修改。48项工程场景全部未执行。UI/UX指导只用于原主题下的清楚只读依据标签，没有搜索脚本、预览或交互验收。

[原生观察](../../custom/src/Swarm/FleetNativeBatteryObservation.md)、[实际数据链](../../custom/src/Swarm/FleetNativeBatteryBridge.md)、[修改清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标／六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估，不按采样点或新字段数上调。仅编辑、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。下方为历史。

## 515 当前：新出勤逐项编码／完整账单与保留提案重审

备用出勤新记录新增默认折叠的只读明细：实际Spatial view17／逐机15／编码项9，分页核对每个真实seq、command／frame、原来源索引、raw／canonical和量化字段；完整能量、风险、空域及审阅值可逐层浏览。每层8项，长键和值分片，不把缺失／null／false／0或负余量混为未知，不用旧区域账单或合成启动捕获补源。编码拒绝保完整错误及未编码要求，不造成功前缀。

新增明确“保留本提案重新审阅”。只用本页实际capture／proposal、实际reviewAttemptToken、旧reviewToken／requestId精确认领；先退休旧采用／本地批准，保私有完整提案，再走原当前证据审阅／归档。失败和过期可人工恢复流程，不清原备用来源强迫重选；外来新记录、活动传输或规划不被抢占。新结果仍须重新采用、申请批准／明确批准与原独立上传确认，没有自动刷新、重试或旧权限恢复。

本批6源码／资源接线＋15文档=21路径，新路径仅ReviewPanel.qml／ReviewPanel.md两条。没有新C++、wire、外部依赖、权限或飞控修改。48项工程场景全部未执行。UI/UX技能用于现Qt/QGC主题、渐进展开、分页／长值／焦点的源码核对，未运行技能脚本、预览或视觉／交互验收。

[新明细合同](../../custom/src/Swarm/FleetReserveDispatchReviewPanel.md)、[重审工作流](../../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本轮只完成源码与静态接线，不表示机载已接纳、任务接替、实际轨迹／净空或最终StartPermit。原25目标／六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估，不按明细或按钮数上调。仅编辑、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 514 当前：备用独立出勤的可见人工流程与唯一共享部署

在513完整后台提案之上新增真实Root来源捕获和FleetReserveDispatchPanel：操作员先明确冻结当前完整task／plan／存档绑定，再选一架实际备用、填写完整新executionProfile、核对自身完整去返路线与summary14，重新提交正常审阅。捕获先冻结再撤旧，精确自身退休收据仅容一次真实批准→preview推进，其它输入／provider／代次或ABA变化仍拒绝。

新kind=reserve-dispatch贯穿捕获、提案、同步／异步回执、审阅归档与唯一共享部署区；旧single／group不能认领、批准或清理它。采用、本地申请批准／明确批准、不同账号上传审批与覆盖确认分别保留，采用不是批准。采用后自身pending不自锁部署，隐藏／退出／取消只处理自己的准确token与请求，不删除已提交事实或自动切回草稿。

独立视图消费本次准确Store记录与同份reviewEvidence，经既有missionSpatialReview和实际PayloadBuilder生成；地图只绘同一新记录的完整两条二维路线，不造Run、原序号或起降轨迹。原未知政策／共同floor／边界、全新profile、当前能力／电池、原生上传门禁与最后StartPermit不放宽。这不是实际备用出勤、原组接替或自动机载改派完成。

本批8源码／资源接线＋18文档=26路径，新路径仅Panel.qml／Panel.md／Workflow.md三条；没有新C++、wire、依赖或权限。56项工程场景全部未执行。UI/UX技能仅用于现Qt/QGC主题、确认／错误／键盘焦点的静态核对，没有运行搜索脚本、预览、交互或视觉验收。

[Root工作流](../../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[可见面板](../../custom/src/Swarm/FleetReserveDispatchPanel.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标／六类实质剩余保持，约40%源码完成／60%剩余仍仅工作量粗估，不按文件、入口、批次或防护数量上调。仅源码修改、直接静读与普通文本／路径／Markdown／git空白核对；未编译、执行项目解析器／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 513 当前：备用保障方案生成独立出勤任务的后台接线

新增FleetReserveDispatchPlanner：从完整原task／plan／binding和明确选中的备用机，生成一份独立单机新任务。新执行路线使用备用自身起返点，并保留原被替补成员全部航点、heading与完整停留；不能直接拿已简化的预算路径代替任务要求。新executionProfile必须人工重新提供且与预算速度一致，不沿用原主机的起降配置；历史待机不在新任务重复执行或扣时，当前电池仍须新审阅。

新reserveDispatchPlanningSource为闭17键schema1；结果7键、摘要14键。完整task和member／constraints／原边界／严格性能布尔分别绑定，Compiler前置任务核对与输出前真实plan核对、PlanStore归档及PayloadBuilder正常编码入口均消费同一来源；四种规划来源互斥。旧single／group再规划只清旧出勤来源并重新绑定，不继承Run、批准或控制权。

保留共同安全／能力／能量及所选成员政策和floor；显式单机用途重设成员／角色数量和备用0，不声称原组已接替或原组保障不变。原clearanceInput不自动换ID，不适用仍阻断。源32MiB、任务／匹配16MiB、请求128KiB、provenance64KiB及每机2048／全组8192编码项等预算不扩，未知要求不能为生成成功而静默丢弃。

本批仅完成后台规划与真实编译／编码／归档消费者接线，尚未新增Root真实来源捕获、可见按钮、确认流程或共享部署源选择；这些是同一备用出勤功能接下来的实质工作，不把Q_INVOKABLE当可操作产品或真实任务执行。正常审阅、独立审批、当前能力／电量及最终StartPermit仍不放宽。

[备用出勤合同](../../custom/src/Swarm/FleetReserveDispatchPlanner.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批10源码／工程登记＋18文档=28路径，新文件仅Planner h／cc／md三条。40项工程场景全部未执行。仅源码修改、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标／六类实质剩余继续，约40%／60%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 512 当前：联合余段任务重建新备用保障

联合余段任务现在支持显式重填完整新备用配置，解决原主／备用换机后旧reservePlans仍指旧ID而必须阻断的未开发边界。不是暗改数字或删除原拒绝：每个新备用恰一份自身起点／返点、替补的新执行机、待机与速度策略，完整新方案仍重新进入现Compiler→ReviewCoordinator→EnergyScheduler。

原三键请求继续旧保守合同；含reservePlans字段就要求新四键完整请求，坏容器不能降级。新provenance schema3／exact28保完整canonical reservePlans；原sourceTaskPolicyHash继续标改前政策，完整新task／routes摘要绑定新意图。新备用运行配置允许显式重新规划，不冒称旧待机／速度值全部未变；最低备用数量及其余共同安全／能力／能量／返航政策保持。旧clearanceInput不自动换ID，资料不适用仍阻断。

task与compiled-plan匹配分开。后者消费真实reserveMembers.energyPlan，以现备用生成器从实际新成员路线重算完整路径／待机／任务／返程成本及关联，不能仅比ID、首尾或hash；当前性能分支不得自行降为legacy。值重算不认证当前性能或来源，实际新能力／电池与原审批来源尾核仍必须成立，旧档案不恢复当前权。

现联合面板新增有界JSON输入及逐备用预览，沿原主题／PlainText／焦点反馈和两步人工确认。文字最多32768字符、最多32描述；原4MiB请求、64KiBprovenance、16MiB任务／匹配、32MiB源／结果及32768生成点预算不扩。没有新模块、资源、依赖、命令、数据库schema或自动改派。

[联合规划器](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)、[操作面板合同](../../custom/src/Swarm/FleetMissionGroupRemainderPanel.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批4源码／接线＋13文档=17既有路径，48项工程场景全部未执行。原25目标与六类实质剩余保持，约40%源码完成／60%剩余仍为粗估，不按本批路径或局部功能上调。

仅源码、直接静态阅读与普通文本／路径／Markdown／git空白核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。新备用预算不证明真实待命、已接替、机载改派、可信实测耗能或StartPermit；完整目标继续。下方为历史。

## 511 当前：MODE发送前的真实begin检查点

原生begin在安装本次真实queueBegan后，立即走同一保护模型提交；只有提交接纳成功、原ReadContext／serial与实际对象仍同、原queue仍处于未commit且可跟踪状态时才返回true。内部Target引用不跨完整Evidence的Link虚调，回调后先核QQ／原上下文再重新定位。失败不回滚已安装的开始事实，也不据日志失败抹掉后来实际write。

这使510事件链能在成功提交时分别捕获“开始准备”和“已调用实际发送”，而不是把两者合并猜测。不是额外控制器、远端ACK、网络送达或正式MODE执行。Vehicle原调用者仍再核原binding／授权／消息不变性；实际write后原commit始终先保真实调用事实，其旧生命周期语义不因新的bool返回值而回撤。事件队列仍可能有明确缺口，不能承诺每项已持久化。

完整Evidence会调用Link虚函数，可嵌套RAW、取消、退休或销毁；这里只保证新路径不直接emit／写文件／发控制，不能宣称纯内存无外部回调。没有新wire、公开API、报告schema、账号权限、资源、依赖或新增路径。

[执行观察合同](../../custom/src/Swarm/FleetMissionExecutionObservation.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批2源码＋13文档=15既有路径；工程场景28项全部未执行。原25目标和六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按检查点或文件数上调。

仅源码、直接静态阅读及普通文本／路径／Markdown／git空白核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。本检查点不授最终StartPermit，不认证远端任务实例、正式结果、占用释放或放飞。下方为历史。

## 510：本地观察事件不是命令ACK或完成认证

受保护原生TaskRecord提交后的注册、MODE本地交接、终点候选、连续性异常、源退休和观察关闭，新增独立低频事件通道进入实际BlackBox及其持久归档，不再只停留在内存观察通知。逐包计数／年龄／临时sourceCurrent不作为事件变化；同Run的异常位图保历史已观察事实。

独立FIFO最多512行／2MiB canonical payload、单条4096字节，每turn最多64条；不是堆内存硬上限。溢出保旧事件并累计缺口，原生临时收据只在实际发布期间有效；缺口不归入假任务。Root另记本地接纳／未记录／未知，BlackBox接纳不等于SQL提交或完整历史，持久录制仍默认关闭。

新v3固定12字段脱敏投影保28字段runObservation；原v1／v2、SQL及历史查询schema不变。实际按taskIdHash下钻和两历史面板新增只读摘要；现报告将reviewRequired=false普通转换作为info，异常只作warning人工复核，raw保原事件。日志timestamp是实际入日志的本机UTC，原捕获UTC另保，不对队列事件偷偷排序或改写时间。

[事件合同](../../custom/src/Swarm/FleetRunObservationEvent.md)、[实际日志接线](../../custom/src/Swarm/FleetRunObservationJournal.md)、[25路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[60项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批8源码／接线＋17文档，新路径仅两份模块合同，无新代码文件／资源／外部依赖。

这仍是本地Run观察，不是任务Completed／Failed、远端实例认证、正式结果、占用释放或飞行许可。原25目标与完整执行／可信性能／自愈机载改派／跨站协作／长期运营／真实模型插件六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按本批路径上调。

仅源码、直接静读及普通文本核对；未编译、执行解析器／helper／业务／QML／SQL／认证／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

本批无新MAVLink发送或审批purpose，原MODE交接事实只被记录。临时receipt匹配证明本actual Orchestrator正在发布这份值，不认证相机／飞控／远端任务实例；旧epoch／旧sequence／历史文件／重算hash都不能恢复该当前发布依据。原编排器对native观察记录的generic complete/fail/reassign禁令保留。

## 509：单次2502／2503及270回读，不将ACK认证为停流

control独立action与purpose为 `fleet.video.stream.control`／`fleet.video.stream.control.approve`。不同SID的实际两账号、原scope／hash／Role、一次typed消费及每次实际发送的最终fence均不可借READ／interval／任务启动批准；不同账号不证明两名自然人。

受控tagged启停与510／511／512共lane，legacy正常同栈STOP→START不整体改成共lane命令。两者忙状态互斥，实际发送推进非回绕video mutation；只有精确原action UUID可接纳自己的一次推进。转发／外部GCS／设备内部控制不在本进程归属证明内。

自己的action实际write之后允许Info的RUNNING-only变化；私有配置代次仍冻结，其他语义ABA不得重基。完整原生回读须原query实际写后新receipt、原PRIMARY／目标stream、accepted ACK及同一实际Info／Status pair。RUNNING变化与回包没有nonce，不能认证由本命令造成。

每阶段最多4秒，accepted query ACK后最多1秒只缩；原Review30秒含审批，不重试／续期／自动恢复／回滚。ACK和reported-target-match只表示该次本地匹配报告，非实际视频到达／停止、带宽或飞行许可。270 param2单流索引仍待兼容验证，不支持不回退0／all／广播或2505。

[控制合同](../../custom/src/Swarm/FleetCameraStreamControl.md)、[Session](../../custom/src/Swarm/FleetOperatorSession.md)、[30路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。只静态源码／普通文本核对，未编译、执行项目实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包；原25目标与六类实质剩余保持。下方为历史。

## 508：独立相机读取，不以512 ACK认证请求因果

新增的人工相机 Info269→Status270 使用独立 `fleet.video.stream.capture` READ。真正发送资格来自 actual Session／Role／Root 原意图、私有 Swarm／Registry 配对及 opaque Run，不来自导入 Map、公开 ID、角色显示、被动报告或 RTT／interval 权限。它没有启停流、配置、二审批准或飞行控制权。

两项读取复用原 Vehicle 510／511／512 消息通道，最终发送前核原授权、物理来源和完整载荷；每项至多一次，整体最早8秒、阶段最多4秒、accepted ACK 后报告窗口最多1秒且只缩短。取消／撤权保留已可能交接、ACK和报告事实，不声称撤回字节。安全飞行命令不应因读取通道被全局封锁。

ACK 只含512等有限字段，269／270也无本地 Run nonce。原 RAW 入口冻结、receipt 基线、精确 sys／component／PRIMARY／stream 和 Info 语义 revision 只能限制本地接纳，不能排除同源迟到包、重复 ACK 或证明无线因果。`wireCausalityProven`、`authenticated`、`catalogComplete`、`remoteRunningVerified`、`remoteStoppedVerified`、带宽及配置／执行／飞行许可旗标均不得据此转真。

269 的 param2 索引有当前本地 common.xml 说明；270 仅说明可由512请求，特定索引兼容性未证实。不支持或回错流就失败／未知，不回退2504／2505、stream0或广播。完整捕获也不认证实际视频送达、无线容量变化或两份报告现在同时新鲜。

[服务合同](../../custom/src/Swarm/FleetCameraStreamCapture.md)、[面板合同](../../custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[48项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 507：被动269／270不借通用ACK建立视频控制许可

新增原PRIMARY相机VIDEO_STREAM_INFORMATION269／STATUS270被动账本，经Registry实际绑定一次读取、Policy第五参数和Guided原五秒周期接入既有指挥中心摘要；不发新请求或控制命令。另修上一轮留下的setVideoMode／setPhotoMode实际caller，以及其toggle／reset模式意图衔接。

本批不新增2502／2503／2504／2505发送或专用lane，不将只读wrapper／本地Info revision作为停流事务、二审或恢复授权。269／270不带本批请求nonce，本地来源／顺序与matchesCurrentInformation不能证明无线请求因果或远端目录实例。原SET_CAMERA_MODE payload保持，只补限定caller生命期和模式意图尾；已可能写入不回滚，也不视ACK为硬件／远端执行认证。

[完整合同](../../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 506：本地播放意图不等于远端视频停流／恢复授权

相机仍沿既有Vehicle调整后MAVLink缓存及通用2502／2503／2504／2505命令，录像2500／2501保持分离。本地stopVideo只管理本地接收／解码，不能据此称无线带宽已下降。VideoReceiver异步完成只有STATUS无requestId，串行pending与代次只限制本地旧续作，不能认证具体异步完成的因果。

原生camera／stream选择代次与QObject身份核对防止本地A→B→A和销毁后继续；不是PRIMARY原始新鲜证据、硬件认证、远端catalog完整性或账号权限。本轮没有新增Fleet受控停流事务、专用wire lane、二审purpose、恢复租约或独立自动降级入口。legacy切车STOP／resume保持其旧用途，不得借作将来自动QoS Apply许可。

[合同](MERIVUS_VIDEO_LIFECYCLE.md)、[50项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505：LOG缺块补读不借MAV_CMD ACK或任务／配置授权

本轮继续使用既有日志下载入口与117／119，不新增Session／Role授权、企业身份或飞行动作；正常结束增加原有效来源一次122 best-effort。LOG_REQUEST_LIST在部分系统可能暂停机载记录，122无ACK，源失效／进程退出时不能保证送达或恢复。

实际Protocol／PRIMARY控制块／ptr／sys／component与本地原Run过滤限制读取来源；LOG_ENTRY／DATA没有请求nonce，不能证明迟到同ID／offset或重启后同ID属于本次远端文件。size官方可近似，严格匹配冻结size可能保守拒绝正常文件，不冒称远端损坏。

普通Vehicle sender仍可由plugin调整消息，插件后没有本下载Run／PRIMARY／完整原LOG等值尾栅；发送前后检查不证明最终wire目标或请求完成。完整本地unique字节＋flush＋rename也不认证远端hash／签名或断电durable，不释放任务执行权限。

[合同](MERIVUS_LOG_DOWNLOAD_RECOVERY.md)、[40项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504：定向TIMESYNC回显不借MAV_CMD／配置授权

新action=fleet.telemetry.rtt.capture，只读主账号／Role／原Root意图，READ免二审，不改Session批准用途，不借interval capture／configure或510／511／512 ACK通道。native单次111及精确ts1回显只支持本地交接→RAW耗时，不认证wire因果、远端身份或时钟偏移。

必须v2 full18和非零目标，plugin不得改完整消息；MAV1／16字节或零目标回应不能退广播。nonce跨Vehicle进程级不复用，不称密码学身份。实际write前标requestIssued只保可能交接；取消／超时不宣称撤回，也不算丢包率。

native26及服务／Policy的全部认证、配置、执行／飞行旗标保持false，snapshotOnly／recommendationOnly不解除。两actual review记录／绑定一致才填唯一latencyMs，capacity／quality／loss仍未知，不能normal。见[合同](../../custom/src/Swarm/FleetLinkLatencyCapture.md)、[44项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503：被动接收不是ACK因果、身份或配置授权

实际Protocol／PRIMARY／Vehicle／component与本地代次只限定本次观察来源，native22的authenticated、availableBandwidthVerified、latencyVerified、packetLossVerified、remoteTransmitRateVerified、configurationPermitted、executionPermit、flightCommandReleased均false，snapshotOnly=true。MAVLink signed帧计签名字节也不宣称验证签名。

Registry成功wrapper7只是实际绑定读取／形状校验；Policy收到portable Map不建立来源权限。新实际五秒消费只取Registry，旧metadata／RSSI／seq-loss不作四QoS；完整窗、完整零窗或接收字节率不证明远端率、停流、丢包或可用容量。

500–502显式Capture／独立配置purpose／二审／原30秒及partial事实不改；此批无新ACK、新查询／511、控制命令或授权。见[合同](../../custom/src/Swarm/FleetLocalReceptionObservation.md)与[40项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502：界面回执归属不是原生授权

[可见面板](../../custom/res/Merivus/FleetOptionalTelemetryPanel.md)只调用实际Root现有显式函数，不取得Swarm／Session／native setter。采集和审阅仅由真实result5接纳并核原当前24的ID／代次／scope／hash／目标；观察到的preparing ID、共享状态和历史不能领权。

stable context只供界面撤销，完整token仍在新请求准入前比较；二审由原Session独立purpose／SID／角色／原期完成。uiWiringPresent仅源码接线、runtimeValidated=false，所有native许可／认证旗标不变。取消原owner／原ID且晚返回不能复活；报告值匹配仍不是wire因果或实际率证明。[44项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501：独立配置身份，不把ACK／回读当认证

配置action精确fleet.telemetry.interval.configure，强制不同SID二审；批准purpose精确fleet.telemetry.interval.configure.approve。Session私有typed消费绑定actual Configuration、primary／approver、hash、原Episode与最早期限，消费前后核原值；实际write围栏重新核当前OS身份／角色／WTS，最后pure tail不授新身份或续期。登出／新代次／到期清私有proof，不能复用capture或上传／启动批准。

未来新Capture只可把3原native terminal query封存一次opaque Basis；公开15键摘要不能反序列化来源。Run精确own511 UUID才允许一次mutation推进和清ledger后的同源bind，外部变化失败；最后三条510同最后代次，不能拼接已失效中间值。wire ACK无本地nonce，同源迟到重复仍可能歧义；reported-values-match不是无线因果、发布率、带宽或硬件认证。

正常二审prompt的Root例外只保同一原身份的旧Role显示，不跳过native实际two-account write fence。所有对应认证／执行／飞行旗标false；不同SID也不证明自然人独立。[46项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

Vehicle在actual write／通知内销毁后，static历史Run投影只读同原actual owner持有的opaque事实；全部来源／匹配／许可false，不走OS／回调／发送，也不把历史state当当前资格。owned510先pin实际child，服务同代次三处回退保partial；此修复解决事实遗漏，不增加认证或写权限。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500：510原query归属与ACK／244双端

native独立原owner／UUID／不回绕lane serial、原PRIMARY弱控制块与实指针、version语义／transport／mutation及最终write前基线保留到两端终结。accepted ACK不能单独完成；244须精确目标且晚于write基线。RAW最早固定原ctx，P0后分派，不让外调中新query领旧包；IN_PROGRESS不重发、不续期。

初次未绑定ledger只能在原明确同源bind处分明转移一次，invalid wire／未知清理会打断旧token；已采内容A→B→A按contentRevision撤销。native最多三条原owner记录供service最后逐项复核。取消和失败保原隔离及已发事实，精确release不清新lane；终态输出是历史不是持续授权。

无nonce的同源迟到ACK／244仍不能证明无线因果，本地UUID不能替代协议认证。此覆盖不扩展进程转发出口、其它GCS或固件内部变化。[模块](../../custom/src/Swarm/FleetOptionalTelemetryCapture.md)与[40项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前：本地唯一lane、原包归属与实际改动代次

510／511／512现走同一Vehicle级跨component专用lane：外调前预留UUID／不回绕serial，固定原提供者QQ、PRIMARY弱控制块与实指针、transport代次及原固定期限；原nativeVersion直接入队也接入，但保原地面／参数／实际授权fence和单次3秒，不借generic条件放宽。generic初连不要求MVM已登记或握手已完成；版本资料首次锚定与实际transport变化分开，native批准依据不自动rebase。

通用512的RequestMessageInfo由Vehicle持有shared context，ACK与原始回包两端终结前保原lane。原500ms响应timer在queue空之前也核固定deadline，ACK后1秒回包窗口只缩原期限；静默、IN_PROGRESS或重试不使等待无限。原包入口冻结原请求归属，实际回调仍在P0原生围栏之后；消息0合法，不再凭0哨兵判无请求。可选callbackOwner仅是QObject寿命条件，不是角色／账号授权；InitialConnect与StandardModes实际传原owner，未传owner的旧borrowed-data调用者仍自行保证数据寿命。

本地UUID／serial、transport epoch、originalMessage入口归属与writeStarted只排除可识别的本地旧上下文／准备前旧包。same-source迟到或重复的无线ACK仍没有请求nonce／messageId回显，244也未证明来自本次查询或改频；不得将这些本地身份当物理设备／固件或实际生效认证。

本车默认component的任意511只在最终actual write前，以同UUID一次预留推进不回绕mutation generation，并全清498三report及接收窗；ledgerEpoch为UUID:十进制generation。writeDepth贯穿write与通知，临界内原包准入／getter不可当前。未发送拒绝不冒称已配置；已可能write的代次／requestIssued不回滚，耗尽保守停止而非重用。498观察15／Registry review21／comparisons6保持，后续只读重采会看到旧基线失效，既有值快照不自动变成实时通知，须新被动资料与新完整5秒窗，不等于实际生效。

Vehicle公共raw sender在plugin前后拒510／511／512旁路，正式请求走专属通道。此覆盖不包括MAVLinkProtocol转发、移动Taisync字节桥、外部GCS或固件内部调频；本批不改变转发策略，不认证实际发布率、丢包或带宽。

本地common协议头822–827明确GET_MESSAGE_INTERVAL510用param1指定流并返回244；512参数2用途须由对应消息定义。不能因共享lane已支持510／512，就假定当前固件支持“512请求244、param2选择62／74／83”；新Fleet主动capture仍需独立窄scope和工程兼容验证。本批未读取或修改PX4飞控。

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

[18路径](MERIVUS_SWARM_PATCH_MANIFEST.md)与[42项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前：被动报告／本地收包与ACK、应用及授权分离

244只作为原source同epoch下的被动报告：没有本地query token、wire nonce或本次511因果证明。62／74／83只计本地原始包的有限接收窗口，不由COMMAND_ACK补成已生效、不推送方频率或链路丢包。

Registry三条当前fresh正报告只给targetUs>=baselineUs的数值比较；receiptSequence／contentRevision是本地账本代次，不是账号、签名、远端query实例或配置许可。same-source旧244仍可能是非本次请求响应，source／component核对不消除该wire歧义。

Guided只附只读快照，不发送512／511，不重用refresh／start／capability审批。真正capture/apply须独立原生目的授权、同源期限及共享lane／变更代次，逐步保部分已发事实；不能将本批review当permit。

[18路径](MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前：本地511 handler归属不等于wire ACK归因

公共511 entry固定原VLM／Protocol／Plugin、PRIMARY QQ与weak控制块及实际pointer、原source epoch和本地UUID；复用构造期持久pin，不从已销毁raw重建。外调后入队前再核duplicate/quarantine，实际发送前核原UUID及完整payload，失败按原UUID重找。ACK只接同原物理来源，IN_PROGRESS不续期；511超时加入原隔离，不新增每个成功终态冷却。

StreamConfig的requestId只绑定一个native pending；Vehicle专属PID context及ESC一次STATUS context不再从全局command511信号认领他人本地结果。SID／审批／StartPermit没有被新ID替代，没有新增认证、权限或飞行动作。context删除前后核同原来源，Provider销毁后的下一入口也不能从raw重建QQ。

成功511终态保原连续串行，不新增每个terminal的隔离／冷却；同物理来源的旧重复ACK仍可能命中后续同command entry。COMMAND_ACK未回显目标messageId，故本地ID、UUID、component、PRIMARY与epoch都不能证明wire因果、远端应用／实际接收频率或带宽节省。timeout quarantine仅有限兼容保护，不是永久迟到排除或协议token。ESC正常accepted只续自身初始化，失败不推进；PID失败保possibly changed而非已回滚。

真正的目标14仍需独立实际Session/角色scope与必要二审、当前原生链路证据／期限、闭合optional白名单、原MESSAGE_INTERVAL244基线/回读及收包频率窗口。不能借只读refresh或start/capability凭据，不能照搬Policy的全telemetry.rateHz，更不能降频P0证据或控制协议；本批没有新apply UI/入口／权限／自动飞行动作。

[18路径](MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前：垂直参考的动作归属不成为实飞序号或权限

Contact仅保同次成功vertical/support的原from数组、原rest±tol、原bound contact实际22序号及有限pad。Waypoint复核同Joint.from投影、整数GPI点包含与完整current窗口；new detail的22→22只关联TAKEOFF动作，不声称捕获起点是wire22、已收到ACK、真实飞行序号或本次执行归属。

same-call native ref不是可移植凭证，值／hash不认证来源；原Models Ptr／PM／PRIMARY／期限／Witness不换源不续期。只有该vertical/support指定terrain完整交集可按同pad例外，不传播到body／obstacle／top→NAV／Domain。v5仍report31/schema1、七认证／许可false，原Worker actualPath真门保留，无发包／执行新入口。

[19路径](MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前：起飞参考几何不成为动作归因或许可

本批仅在Waypoint消费原GPI9／PM29／wire与Joint已绑定名义来源，重算地面分支条件编码top区间并检查首NAV完整接受域连接。takeoffSequenceVerified与名义点对应不认证当前landed、内部控制器状态、ACK因果或实际轨迹；独立值调用者仍可伪造材料，真实原生owner／来源／期限和Worker对象链不能用hash代替。

没有新增发包、权限或启动动作；报告仍为原exact31/schema1，七认证／许可false不升格，Worker actualPathCertified=true要求不删除。接地例外不转给新参考连接，结果满足声明环境也不等于StartPermit或正式任务结果。[16路径](MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前：报告文件身份不是飞行或交接授权

本批原生新save/load仅处理report8历史值。expectedReportId＋expectedContentHash只约束用户确认的同份材料，不是签名、账户、机载任务实例或存储来源认证；保存不新建reportId／捕获时间。原record selectionToken仍只说明内部元数据对应，导入不能注册为当前native目录记录。

Root两新窄口不访问Vehicle／Swarm执行或旧full9报告状态；本页imported来源独立，不恢复任务、占用、审批、StartPermit、跨站租约或发MAVLink。实际写入成功／未知事实和后续上下文失效分开记录，取消不宣称回滚。

[18路径](MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前：搜索来源绑定不是任务或飞行身份认证

search由原actual catalog／全ordered descriptor manifest／原native目录身份／真实batch index及sysid/msgid过滤器绑定；header声明的机号不代替实际记录命中。searchId仅本机搜索序列UUID，manifestHash／fileHash仅内容对应，不是签名、ACL或飞机身份。

原生终态与actual finished双条件后才公开当前search。新读／取消／新capture失效旧搜索和hit资格；打开只用该原成功hit的fileHash及原完整catalog descriptor进入单file首页，不能由caller提供hash或把首末seq减1伪造cursor。位置载荷仍要求原header同意与本次明确选择，运动报告另一次确认。

无payload／帧UTC／source epoch/sequence／签名公开，不关联任务Run、不发命令、不恢复控制权、认证或StartPermit。历史完整前缀不等于完整遥测／原子快照；旧页协议及权限不改。36工程场景全未执行。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前：live检查及commit事实不构成远端认证

三Link虚调保留，但每次前后绑定原Vehicle／Link／Manager／Protocol／MVM、弱PRIMARY、version和slot上下文；返回false也不等于this／原行还活。RAW／recheck只能退休其原绑定，不能把回调后来新建的epoch误退。Run私有变更代次与TaskRecord24字段比较只阻止混代、悬空引用或旧值覆盖，不成为新认证／权限。

MODE排队仍用同一原observer，ready回调后重核原command context。原actual write发生后，即使source退休／Vehicle销毁或本地serial耗尽，也保留捕获sameQueue的实际commit；这是调用事实，不是机载收到／接受／完成／安全，不能删除历史后冒充未发。纯报告／TaskModelEvidence不释放新命令或恢复权限。

没有新ACK因果、远端任务实例、跨站租约、签名或StartPermit；未知尾、候选非正式结果及动态轨迹未证明边界保持。36工程场景全未执行；本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前：LAND参考来源不是实际动作／接地身份

LandingReference仅在同次规范schema2＋完整Joint已成功绑定LAND支承组件时产生；vehicleId、原目标协议sequence、segmentId、载荷对应和E7／wire-ground容差沿旧校验，rest上下界与contact有限band直接复用原applied组件。它是本次原生值，不新增可导入token、签名、ACK因果或当前着陆状态认证。

Waypoint用原PM29的NAV半径与LAND兼容分支构造声明入口／下降体。下降体覆盖整个入口与目标XY凸包，不假设水平先结束；实际MOVE分支、动态接受高度和控制器内部状态仍不可由该值确定。只该下降support指定terrain的完整交集被原有限pad包含才可豁免，入口连接、普通NAV、body、障碍及StartupDomain不借此豁免。

无新命令、控制租约、权限或最终StartPermit；actualPath／numericAcceptance／动态高度／时间等认证标志不改true。PM原29尾核、Review／Worker／Witness原身份／期限继续，hash或native参考不恢复过期批准。36工程场景全未执行；本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前：原29语义连续性不是飞行权限或ACK因果认证

Txn从实际authority关联Vehicle／PM／PRIMARY及原beforeArm.parameterSummary.originalExecutionParameters取源，首次固定PRIMARY弱控制块，初次PM29核对后贯穿所有stage。MemorySeal.current末尾检查原manager epoch／Fact／源／类型／值／contentRevision／pending和年龄；原生Vehicle最终commandContextCurrent已经沿整组seal链调用，不靠一个caller布尔、新参数读、hash或后来同值重采样放行。

等值PARAM_VALUE可按原账本规则刷新接收年龄，不要求冻结每个receipt编号；实际值／类型／来源／ABA修订和MPC_TKO_RAMP_T正负零变化仍失败。原30秒事务、审批及phase截止不续期。该检查只证明本机已观察原参数语义仍对应，不证明真实固件内部状态、二进制身份、远端原子快照或无线因果。

索引保存自有原凸柱，以bounds严格分离粗筛后仍用完整query及原并集计算；terrain水平全Z与接触顶面独立。没有新增ACK归属、控制租约、任务实例认证、签名、许可或命令。actualPathCertified／executionPermit／flightCommandReleased等原未完成边界不改true，完整动作／动态净空、StartPermit及正式结果继续。36工程场景全未执行；本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前：私有保护记录的选择与报告不是飞行身份

新sourceId只标识本Orchestrator进程实例；taskId＋规范UInt64观察修订＋元数据selectionToken仅定位当前所选缓存。token来自轻量12字段，不是完整观察哈希、签名、操作者或真机身份认证。max修订后强制close/death由关闭／销毁／耗尽等元数据变化撤旧选择；仍明确该修订不唯一。

Controller不接受caller观察／结果正文，不调用owner/current补源；构建和原生JSON之后都核同实际Orchestrator／source及完整record。reportClaims保持原4个历史说明true和18个认证／新鲜／完成／权限等false。不新增ACK归属、lease、任务实例token、控制、关闭监视或释放占用。

新报告不套用当前selected Episode source3，旧full9／文件／handoff不放宽。Panel只采用本次确认返回的native JSON及对应reportId/hash/source，不自动领用其他面板结果；来源消失、隐藏、换controller及明确清理退役本区意图。36工程场景全未执行；仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：回退停止关联与几何模型不授予执行身份

post-commit `MISSION_CURRENT` 回退原包仍进入reportedMission及receipt，再累计回退并停止该target tracking；它可能是重排／重启／外来任务，地面站不认证原因。后续REACHED／LANDING／ON_GROUND不能与旧terminal／airborne组合新候选。已观察候选与真正MODE提交事实仍保留，sourceCurrent变false不表示停飞；本地占用不自动释放、其他成员不中止。

公开Run单机26／整组20、TaskModel与报告旧合同保持，轮询仍合并通知；没有新增ACK归属、远端实例token、结果认证、身份、授权用途或控制命令。几何模型名v2只区分声明算法版本，不认证真机固件、实际轨迹、场景或StartPermit；粗筛命中不是接触结论，declaredModelSatisfied不等于飞行许可。

[32项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)不是协议或运行验证。仅地面站源码／文档及普通文本核对；未编译、运行模型／helper、测试、仿真、改PX4、硬件／刷机／放行、提交、更新PR或打包。下方为历史。

## 487 当前：资料卷宗不继承任务或身份权限

原review操作的同一次二次位置确认生成31及dossier21，不新增动作或授权用途。Archive原page／serial／latest/last／双终结和六身份尾核后，双缓存一次发布；任一失配两个getter均空。采集run不是任务run，机号／时间／文件hash都不构成任务归属或真实飞机身份。

新格式没有旧任务reportId／boundaries组合；不写旧_lastReport、飞前missionReport、Run结果、任务事件或权限链。hash只作本地正文对应，非签名或认证。historicalOnly/readOnly=true，认证／签名／任务关联／完成／执行许可／飞行发送／权限恢复／运行验证=false；四视角不是服务端鉴权，不把客户子视图称为已交付成果。

本面板成功回执及原context尾核后才采用成对快照，不自动认领全局结果；隐藏、输入或query变化清显示。可复制文本不自动写文件、剪贴板、上传或发送。34工程场景全未执行；独立交叉静读及最终文本结果已记本批审计，不是身份、协议或运行验证。本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前：历史读请求与摘要不产生控制权

新闭合本地request只读原AppData遥测目录，不接调用方path、policy、query、afterSequence、raw或Vehicle。latest query＋native serial防同ID旧调用重入；first选真实目录，next取原最新完成页的私有cursor/filter/位置选择，review取实际Archive nativeHistoryPage，不使用公开history摘要重建。

位置读取及派生显示分别确认；旧query/options／输入代次／隐藏等撤旧意图，通知后再核原context，异步只CAS清同一pending。新query／cancel清native report；公开值不包含原raw／绝对位置／UTC／epoch。historicalOnly=true；认证／CRC／无reset／漂移原因／任务归属／轨迹安全／执行许可等证明旗标保持false；本地位置确认不是操作者身份认证或飞行批准。

32工程场景全部未执行；本批审计已记录独立交叉静读、14文档终审及最终普通文本核对，不是账号、协议或运行验证。仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前：规划政策不是性能证据或新飞行权限

三态编辑器只给原Draft写optional实际Bool或明确移除字段，不写Registry性能值、不采纳能力／基线、不使用或扩大484账号审批用途。开启严格政策让实际规划／预算要求当前工程声明及完整匹配基线，关闭／未声明只保旧规划规则，不能关闭原健康、来源、净空或执行门。

编辑冻结原draft/source/revision和明确选择；按压后换来源／修订／选择／确认则不提交旧意图到新草稿。Root先撤旧审阅再精确原源CAS，native changed继续使旧审阅／上传确认／后台结果失效。同值no-op不会自动增revision，但原主动失效仍发生；来源通知或提交收尾失败不自动重试或推断回滚。

schema5 requirePerformanceEvidence presence、实际非null Bool及值贯穿source／task／plan，Store严核完整源；旧源删版本或新源删flag都不能静默降级。保存／恢复及余段保完整政策，但不恢复账号、批准、Run、占用、机载状态或飞行许可。32工程场景全部未执行，本批审计已记录独立交叉静读及最终文本核对，不是账号、协议或运行验证。

原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。 本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前：性能声明独立用途与真实一次提交

新 `approve-performance` 复用能力工程角色政策，但认证用途严格为 `fleet.performance.declaration.approve`；旧capability／configuration审批不能用于本操作，也不产生上传／启动lease。真实Review决定purpose，Session闭表、OS提示、consume与已消费尾核均按同一实际owner；两账号SID不同，原30秒期限和Session TTL不续期。

Root冻结canonical两数与原账号／政策／scope；首次同原native请求回调保拟context token，后续回调及请求返回保持同两map。Review私有绑定原baseline14、原context、拟数值／context和完整caps；Registry在ownedFence前后核原record／review／history／基线批准及typed指纹，一次提交数字、新context和approval14，先固定独立receipt14再通知。通知删除／换源／审计失败不改写mutationCommitted事实。

数字是本地工程声明，performanceCertified／hardwareIdentityAuthenticated／executionPermitted／flightCommandReleased不被本操作授予。native C++ fenced API本身不是账号认证服务；实际产品双账号结论只来自受控Review／Root／Panel链。独立摘要不保存数值／账号／理由／配置／凭据，也不恢复批准；完整本地EvidenceChain追加与摘要接纳、SQL落盘分开。

32工程场景全部未执行；本批审计已记最终普通文本核对、独立静读与两处修正，不是认证或运行验收。原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。 本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前：当前工程声明数值不等于真实性或命令许可

新增pure `currentDeclaredPerformance` 复用原capabilityReviewContext／approval、identityFingerprint及完整configurationBaselineEvidence；旧批准和hash schema不变。最大速度／续航原本已进入工程身份，本轮不重造批准，而是使显式严格策略真实消费这些当前数字。exact11及其nested两身份是值一致性证据，不认证伪造的caller map或物理飞机。

完整canonical数字及当前工程批准／构型身份进入严格计划和Coordinator审阅fingerprint；变更、撤销、重批准或数值／配置替换需新的来源与审阅，不能沿用旧权限。Compiler0.8.0计划必须保true策略，历史档案不被回填或自动升级。实际产品来源仍依赖Coordinator既有原native快照／来源连续性检查，pure helper不创建原生凭证。

严格能量以声明续航和原电量估算；原生电池unsafe／过期独立拒绝，实际native剩余时间仅可进一步降低，不把metadata direct／drain当当前工程声明。performanceCertified／hardwareIdentityAuthenticated／executionPermitted／flightCommandReleased均false；没有ACK、MAVLink、参数写入、ARM／MODE权限或PX4变化。P0缺动态状态与真实路径证据不因本轮改变。

本轮仅源码／文档；30项工程场景全部未执行。本批静态审计已记最终普通文本核对和独立静读结论，不等于认证或运行验收。未编译、运行helper／业务／模型／QML、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。原25目标、六类剩余与约40%／60%粗估保持。下方为历史。

## 482 当前：原29参数来源不由JSON、同hash或新PRIMARY代替

新增 `originalExecutionParameters` 只是有界材料：schema1 exact6、固定12／2／14／1稳定行及RAMP负零marker。codec允许JSON整数元数据恢复Int，不放宽旧native12／14／28口，不产生原PM／Link引用或身份；marker只保符号不能认证来源。旧12profile hash、公开19键及纯动作模型输入保持。

两Review从同次Readiness冻结原快照，保Targets最初Vehicle／PM／links／PRIMARY弱owner；Worker/schema3 row4只带值，真实owner在入队／finished及全部来源回调尾部用原29memory口核实际epoch／revision／Fact／pending／年龄。Witness接原PRIMARY弱map并独立复制外层存储，不从完成时新PRIMARY换owner；原时钟与绝对期限不续期。相同裸指针、ABA同值、相同hash或重采新鲜值均不能替原源。

同步publication也是来源窗口：normal开始计算、finished及acknowledge通知返回后核原29，Ground finished的same／fence同样闭尾；通知内改PM／断链／换component使旧结果失效。独立worker即使owner在通知中取消／销毁仍启动完成清理，不以不启动掩盖来源变化；Witness原弱map复制前限1..32项。

Worker实际新增NAV接受／连接环境报告，结果exact7被两Review风险、Witness和两Authority完整绑定。resultMatches只核完整材料／重建shape与一致性，不独立认证solver verdict；合法公开map不能安装原Witness。原native Models与整数域同Ptr链及sourceFence仍必需。

没有新MAVLink消息、ACK处理、参数写入或ARM／MODE权限。新Report证明及权限字段false；既有actualPathCertified=false继续阻断共用准入，不借静态环境无violation释放StartPermit。Contact接地豁免与旧批准不扩大／恢复。

仅直接静读及静态源码／文档工作；限定独立静读及最终普通文本核对记录见本批静态审计。未编译、执行helper／模型／业务／QML、测试、仿真、硬件、PX4修改、刷机、放行、提交、更新PR或打包。原25目标、六类实质剩余与整体约40%粗估保持。下方为历史。

## 481：显式选择与同对象域，不产生新命令权限

startupSpatialGuard仅真实精确字符串且要求完整启动策略；缺失保旧，坏值拒绝，不由policy版本、资料或历史报告推断选择。完整profile进入既有计划／风险／配置身份，新source2另绑定完整raw cells/slab，切换需新审阅，旧批准不迁移。

实际Worker用原Models，Cover读原Motion的同IntegerDomain Ptr；只有值型matcher可重现临时对象作完整shape/source比较。新numeric由全部原域完整投影生成，不接受source自报true、JSON还原授权或同hash替原对象。实际准入额外消费已存在的actualPathCertified字段；producer仍false，所以没有借新包含结论释放ARM／MODE权限。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480：28项参数来源与动作前置，不改命令权限

PM被动观察／受控PARAM_REQUEST_READ扩为固定12＋2＋14，不能由UI传任意参数名。原12／14接口保持原精确集合，新三集合memory gate在最终回调后核原epoch／revision／Fact／PRIMARY控制块／pending和全组年龄，不以同ID或新capture换旧源。顺序回执不是机载原子快照。

schema4动作完整stable证据进入原审阅及启动身份；参数变化撤旧依据，明确LAND的RC辅助及起降外部setpoint不由旧路径假设放行。没有参数写入、新命令、ACK语义或占用／控制权变化；本地固件快照和目录声明不认证真机二进制、源作者或当前内部阶段。最终StartPermit与正式结果缺口保持。

MPC_TKO_RAMP_T的+0／−0在本地Auto除法语义不同，不能用数值相等隐藏符号翻转。PM仅对此固定参数的内容变化／缓存对应／最终原行比较补signbit敏感性，保留原值与revision链；模型负零分支不材料化非finite数值、拒TAKEOFF兼容，不把JSON的零表示当完整来源依据。旧12hash与行形状不扩容。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：14项观察不改命令权限，旧12绑定与当前全证据分离

新增观察MPC_XY_VEL_ALL／MPC_Z_VEL_ALL沿既有真实PARAM_VALUE源账本；显式Refresh沿原受控PARAM_REQUEST_READ入口，固定14而不请求任意参数、写参数或恢复批准。原sourceLink／Fact／epoch／revision／pending／PRIMARY弱owner检查保持，旧12hash用途及公开19键接口不改。

两个宏和实际覆写关系进入完整evidence／startupConfiguration身份，失配经原不可用风险与Readiness阻断。新整组memory gate核原12＋2固定集合，在最终可能重入回调之后执行，不通过刷新、同ID新对象、同值ABA或新capture代替旧源。它不认证报文作者、真机固件内部值或整段飞行连续参数历史，不改变ACK／命令释放／StartPermit。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478：边界策略身份与实际INT编码对应

原始outer／holes／margin／frame进入Compiler planId的独立policy和原完整task，输出完整plan再由原Store contentHash／binding／审批链冻结；这些摘要只核内容一致，不认证构型、现场或操作权限。余段新provenance完整policy同时核task与plan，资料包中的sourceTask／sourcePlan亦互核。

读取实际PlanManager发送器确认当前为MISSION_ITEM_INT：原double乘1e7到int32截断、参数／高度float32；旧MISSION_REQUEST仍进入同一INT发送器，真实owned raw回读拒MISSION_ITEM。无新消息、ACK语义、参数写入、占用或启动动作。保raw上传degree，检查用E7/1e7副本不回写，防二次截断。

独立reproduceHistoricalPayloads只供3处原资料自洽复现；Deployment／Review／Readiness保持正常build严格完整策略。最终StartPermit、动态净空和正式任务结果仍未完成，编码检查不授予执行权限。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477：退让策略不增机载权限或ACK语义

本地area.boundarySetbackMeters进入完整task/source版本身份并由共享checker消费，不新增MAVLink消息、飞控参数写入、占用、Run或启动动作。候选成功只生成提案，名义距离检查不认证传感器／构型、跟踪误差、编码量化后几何、动态轨迹或StartPermit。

source4恢复不恢复批准；编辑同次CAS保存孔与退让并冻实际owner／selection／revision。输入发布前撤确认，成功清理尾核两字段及原上下文，取消持ticket防新编辑被旧清理覆盖；本地可能已提交但上下文已变时保留当前文字，不自动回滚或重试。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476：二维排除区不成为机载避障或执行凭据

新generate与共享validator只消费本地值，不持有Vehicle、发送ACK或改原Run／任务占用。Compiler名义排除区失败加入blocking风险；通过只说明当前声明下的有界二维路线检查，不授予启动、机载租约、动力学或障碍净空认证。

schema3导入仍经完整原生Draft校验并清datum确认；Root完整settings替换及排除区setter均冻结原backend／selection／revision，通知期间换源不向新草稿提交。UI失败／隐藏撤销原确认，opaque上下文代防已开始动作的ABA，原文字保留供核对。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475：联合新提案不认证旧任务或释放飞行

新的groupRemainderPlanningSource精确绑定全部原／新成员、来源包、原payload／路线／成员hash、新联合政策／路线hash；它仍只是历史值一致性，不认证远端任务实例、ACK因果或正式完成。每个原成员必须原begin＋commit才可生成全组提案，不能把未提交者丢掉或按reached跳点。

一次联合当前审阅、显示／采用与本地批准仍不等于上传、ARM、MODE释放或接管许可；唯一共享部署源和原native来源／时效／权限／StartPermit边界保持。取消只针对冻结的本地后台requestId，不取消飞机任务，不回滚已发生的归档／批准事实。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474：完整原组归档不新增ACK或远端任务认证

新集合保留每个原target完整payload／MODE队列来源／原进度，包括未提交行。available只表示本地原native MODE begin＋commit的历史取源资格，不是任务成功、远端任务实例或接管许可；旧v1单源沿旧验证域，缺begin显示未知，不补证明。

source20绑定所选成员及整包hash，仍需全新当前任务审阅；不从目录、文件、模式、sequence／reached或本地保存成功推断正式完成。原迟到ACK、完整StartPermit／正式结果、机载改派、控制租约及企业跨站等实质工作保持。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：完整历史包身份只作追溯，不转成native／Run／权限

包内nativeSource是原捕获18字段的历史记录，不代表导入时仍有原owner或新鲜机载来源。Archive完整校原plan hash、唯一member、全部重建载荷及progress映射后，输出独立schema2／历史kind和包UUID／完整hash；Planner历史provenance30字段携原绑定，但不会重新认证被省略的旧reviewEvidence或恢复原native身份。

Root显式原生冻结仍锁实际selected owner／identity；历史capture只锁私有包槽、完整包ID／hash和当前目标scope，经过Bridge尾栅后才发布。公开摘要不包含完整task／plan／payload，当前新审阅重新采集证据。原sourceCurrent、观察序号、末项或落地候选不变成完成前缀、远端实例或正式结果。

保存真实commit事实先于Root生命周期尾核保留；界面按下／busy后核原意图，结果按独立context epoch归属。明确退出只清与同review/proposal/capture精确关联的本地来源，不清新来源，不改Run、归档或占用。原ACK、最终Vehicle gate、numericfalse／Compiler pending和完整StartPermit缺口不变。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：本机保存事实不变成机载或操作权限

[Store](../../custom/src/Swarm/FleetMissionDraftStore.md)只承认自己的实际QSaveFile成功为checkpoint committed；读取、完整hash、记录UUID／修订、磁盘内容一致均不是机载身份、请求因果、审批、Run或启动许可。Root新saved来源复用原Episode空闲、实际Draft／选择token及撤审批／发布流程，不新增任何MAVLink发送。

原生prepareSave在首次changed前冻结expectedRevision及合法性；不能由同步槽改caller QVariant与Draft来偷换调用时预期。restoreSource成功保同一预览令牌，每次仍全文件重读；不是一次性权限消费。Root不把历史sourceRevision恢复成活跃CAS。

Panel按下／busy通知后重核原请求，返回反馈绑定独立contextEpoch；控制器／Episode／输入／显隐变化含ABA后，已提交事实仍保留但不声称当前来源通过。普通新Draft token变化不是后验上下文失效。焦点及滚动请求均重新核同一原上下文。原ACK语义、Flight gate、numericfalse及Compiler pending完全不改。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：原Ptr固定运算依据，不新增任何身份或许可

两类Review原Models锁同次不可变对象，Worker完整核原input8各ID／source12、实际Domain直接用原Ptr，finished再核原集合，Witness const保存到Txn。模型输入独立持有防原容器／字符串旧可写引用事后改变源；source值相同不能替代原Ptr、实际finished或Owner／generation对应。

nativeModelsMatchInput只做已冻结值与原模型描述的完整对应，不认证哪个Review创建对象；public snapshot／resultMatches临时重建也不证明同次缓存。两真实Review私有来源与同集合尾栅、原实际Vehicle／PRIMARY、权限、deadline／UTC及最终write门禁继续，不从shared_ptr生命周期推断证据仍鲜活。

本批不发MAVLink、不改ACK／receipt／source12／input8／result6／report28，不把source hash变成Qt二进制／libm或机载实例认证。numericfalse、Compiler pending、完整动作范围／StartPermit／正式结果缺口保持，不能由原对象连续性恢复旧批准。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：索引原行是几何对应，不是任务或执行身份

PreparedPrismIndex仅由原生完整工厂构造并私有持有本次原凸柱值；逐元素独立外层容器防调用者既有可变引用替换来源，copy-only防有效空壳。其row只是同一次scene的原始行，Morton排序／树节点不成为资料ID、账号、审批、原生观测或任务实例认证。

DomainReview两个完整索引各自提供候选和同源indexed SAT，再按各自原terrainRows／obstacleRows取cellId／volumeId。完整候选只说明包围盒未分离，真实SATTrue也仅说明声明集合相交；不能替代实际碰撞观测或起降动作／阶段许可。

本批不改ACK、receipt、完整input／source12、v2／报告28／Worker6、原运行谓词、权限／期限或最后Vehicle write门禁，不发MAVLink。numericfalse、Compiler pending、StartPermit与正式远端实例结果缺口保持；纯几何缓存不能恢复任何旧批准。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469：原生几何缓存不具有飞行或任务身份

PreparedPrism的私有valid只表示该数值凸柱已经完成完整几何工厂检查；不能认证原资料、工程师、机体、垂直datum或时效。复制保几何，不生成新的源身份、账号、审批、动作绑定或启动许可；不能由JSON恢复有效对象。

DomainReview仍绑定同次完整input／Joint／motion／payload／geometryBasis和原生worker完成，报告28／Worker6及numericfalse保持。新SAT仅用于同一计算中声明数学集合交集，不发MAVLink、不修改ACK／receipt／原运行谓词／Compiler pending／最终write门禁，也不把接地候选当已批准起降动作。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468：外包版本和明细前缀不替代实际来源／执行身份

DomainReview新模型名v2与当前Cover算法对应，旧v1报告不可改字符串后复用；同次完整来源、原payload、geometryBasis及原生worker完成／时效链仍须保留。纯值matcher拒收尚未处理component的明细，反经线共享身份按最早实际出现保守核对，不从component索引推断机载动作。

本批不改ACK／receipt／source12／Worker result6，不新发MAVLink，不变更两Review、Witness、两Authority或最后write入口。numericContainmentProven仍false，原完整运动谓词／动作范围、StartPermit与正式任务实例结果均未由本轮完成；更紧的理想外包和更少冗余平面不授予权限。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467：完整理想域报告不是ACK、动作身份或启动许可

Worker完整input8指纹继续绑定原source12；新增startupDomainReview与原Joint／名义报告共同形成严格result6。两Review先核实际input／原生源与时限，再保存完整结果；两Authority不得接受旧result5。Witness工厂独立要求统一接纳门禁，numericfalse不能从JSON、理想模型满足或名义段成功恢复为许可。

动作身份不可由几何推断：原TAKEOFF／LAND pad绑定完整payload及指定组件，不能因为同机号、terrainCellId或cap的segmentIndex就扩成全域接地许可。新整域已移除此豁免；原名义规则不变。报告source hash仅内容绑定，既非原生来源认证，也非机载实际任务实例或正式执行结果。

本批不发MAVLink、不改ACK/receipt含义、原运动谓词、权限、期限、Compiler pending或最后Vehicle write门禁。完整数值／动作范围证明、StartPermit、正式远端实例和结果继续待开发。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466：导入确认仅限历史文件，不是飞行控制授权

diagnosticsId／entryId先与当前原生私有readable来源对应，previewId绑定原来源／固定目标及30秒单调期限，commit-import一次消费生成独立queryId；operationId保原previewId。public ID／备份文件UUID／新观察hash不能独立重建来源、批准、账号、StartPermit或正式机载结果。

Worker38保原来源与暂存／发布事实，Store核同实际worker／query／run及固定invocationDirectory，面板actualfinished后才接纳同原预览结果。importVerified只表示文件进入retired历史保留区，不解除任务或飞行门禁、不发MAVLink、不改ACK／机载控制路径。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465：本地诊断ID不参与MAVLink控制或恢复授权

backup-diagnostics的queryId／diagnosticsId／entryId只是当前本机维护观察身份。Store仅在原actual worker、query、run和完整23结果匹配后生成public26；面板须actualfinished后与原Worker23／row4逐项对应，原目录／原版本／通知不能被新值冒名替换。
本批不发MAVLink、不改机载ACK／启动／飞行控制路径、不从文件名UUID、观察hash或本地诊断ID恢复任务、权限或已批准源。所有共同非授权旗标及originalSourceVerified／backupSetCompletenessVerified保持false。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：独立副本UUID／hash不是原任务或作者认证

新的[备份目录](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)从文件名读取epoch／sealRun／backupOperation UUID，并与实际SQLite身份对应；第三UUID不能证明曾由本地463工单生成，测得SHA也不能与已丢失原源作真实性比较。Store随机catalog／entry／query ID只绑定当前本地观察。

本批不改MAVLink、Vehicle、ACK／receipt或任何启动／飞行许可。原源对应与备份集合完整性固定false，不将副本导入live任务／BlackBox／审批或控制权，不从结构自洽生成执行授权。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：备份工单不是机载任务或授权凭据

backup的operationId使用原预览UUID，queryId为实际维护工单UUID；两者只对应本地复制意图和观察，不是MAVLink token、机载执行实例或任务完成证明。[完整备份事实](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)中backupVerified仅指当前副本读回及源对应，SHA／epoch／seal clean均不认证作者或远端飞行事件。

本批不改Vehicle、MAVLink／ACK／receipt或任何飞行授权。不从备份文件回灌任务、账号、批准或控制权；复制accepted、字节计数、copyCompleted和实际读回通过分开，取消不能抹掉mayHaveWritten。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：维护工单不是飞行命令，恢复归档不恢复授权

[归档维护](../../custom/src/Swarm/FleetArchiveMaintenancePanel.md)的inventory／entry／preview／query ID只绑定固定本机文件及原生观察；request accepted不是文件移动成功，moveConfirmed也不是远端任务完成。Hash／SQLite epoch／seal run clean只能对应本次本地保留内容，不认证作者、飞机硬件或原始事件真实性。

本批不改MAVLink、Vehicle、ACK／operation／receipt／启动或飞行许可；退役与恢复只改归档命名空间，旧native历史游标退休，旧UI副本仍历史只读。不会把文件恢复回灌live BlackBox／规则／Replay／Registry／任务／账号／审批或控制权。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：任务哈希仅关联保留诊断，不是远端任务实例

[任务目录](../../custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)只消费真实归档payload.taskIdHash及原文件／行号／contentHash来源。相同哈希可组织本次观察到的诊断事件，但不认证硬件、原始任务内容、机载实例、ACK因果或任务完成；原始子事件类型与嵌套summary已省略时不能复原。

本批没有改MAVLink、Vehicle命令、ACK／operation／receipt或任何执行授权；历史只读，不回灌live任务／权限，不将NDJSON runId按机号或时间拼成同一任务。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：电池状态修订不是认证或命令因果

[原生电池](../../custom/src/Swarm/FleetNativeBatteryObservation.md)的charge／fault和known变化进入同原PRIMARY账本的semantic revision；Guided身份保健康内容／fresh，普通age／receipt增长不制造新批准。扩展不存在是未知，零mask／正常枚举不是物理认证，BATTERY_STATUS也不提供真实电池硬件身份或远端测量时刻。

[共享检查](../../custom/src/Swarm/FleetNativeBatteryEvidence.md)只将合法原负面用于阻断；最后[phase seal](../../src/Vehicle/FleetStartupMemorySeal.md)另保原typed电池账本，不借显示或一次JSON匹配替代跨机callback后的整组尾核。ACK／operation／journal／Receipt、远端任务实例及正式结果合同不改，不新增命令／授权。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：原生模型同对象不证明遥测因果或远端执行

[Witness](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)和[Txn](../../custom/src/Swarm/FleetMissionStartupTransaction.md)固定原source12模型同Ptr，Transition阶段复用，不改ACK／operation／journal／receipt或远端任务实例合同。完整模型匹配和源hash只能对应原本地内容，不能将ACK、观测、已阅或同对象升级为本次执行归属。

原普通Authority sourceFingerprint已包含GPI9，未证实旧路径静默换基准；本批是显式同模型和移除重复准备。模型自身无权限或时效，原Witness／phase seal／最后write仍独立核验。完整运动域、StartPermit和正式结果保持未完成。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：运动描述内容绑定不新增远端身份或命令因果

[共享模型](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)的完整载荷hash仅与原Builder内容、同序Joint.memberCoverage及原审阅input2摘要对应；不能认证远端机型、固件二进制、命令执行或物理轨迹。冻结原GPI9／策略／ceiling同样不证明相同远端采样或实际任务实例。

本批不改ACK、RAW operation／journal或Receipt合同；既有原生Witness与457 phase seal继续各自保源，旧包不能凭新模型恢复。source12保持snapshotOnly=true与numericContainmentProven／clearanceCertified／flightCommandReleased=false，原名义报告scope不扩张。最终StartPermit、正式机载实例／结果及全域包含仍未完成。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：原packet身份与阶段seal不是因果或远端实例

普通RAW入口在首回调前冻结原protocol／PRIMARY／owner／startup／原journal及其operation／原收到时刻。排队前包、回调后出现的新step或全局新slot.operation不能作为该原包身份；closed sender记录仍延续至下一实际queue。write内同步响应可记账，requestIssued仍等真实write返回，不由ACK／序列推导命令因果。

opaque Seal仅native构造，保存本次完整阶段检查所用原typed账本／receipt／token／来源，所有回调后核整组；commandContext不capture新值替旧判定。原DTO与step33保持，seal不包含可导入授权或正式任务完成证明。完整净空包含、StartPermit／正式机载结果仍未完成，原pending保持。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 456：原生Witness不等于ACK、任务实例或飞行许可

Worker result5的inputFingerprint仅绑定同次输入，不认证远端采用或结果。新增Witness只允许实际Review工厂在原typed source18／datum14、实际对象和完整满足报告下建立；普通／地面Authority及Txn共享原生ptr，QML／JSON／历史导入不能重建。公开context形状不加可伪造许可键。

原Review clock／absolute deadline、until及UTC水位贯穿移交，无新计时或新批准。每次sourceCurrent完整回调核先于旧身份／认证门禁，最终memory与实际Vehicle commandContextCurrent全组尾核没有插件／Link虚调／weak.lock；它不证明网络、物理datum、PX4当前内部valid、同采样或整个运动域净空。单个caller fence拒绝不撤销别的owner权力，但该owner命令仍须原权限门禁；真源／时间／期限失败永久退休。

普通RAW首回调窗和全组motion最终尾栅另须补，原pending与最终StartPermit／正式机载结果保持未完成。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：worker输入指纹不是机载实例或启动许可

前置geometryBasis是本次脱离来源的完整输入JSON摘要，含实际审阅UUID／epoch／generation、原计划、准确归档证据和同次几何；不含最终报告／artifact，不借用最终风险指纹或形成循环摘要。线程只持值，不能由其source18／datum14 JSON副本恢复原生绑定或账号权力。

queued finished仅表示本地计算结束，不是MAVLink ACK／机载采用／正式任务结果。结果必须经原owner重核、完整来源匹配与四真零违规判断，第一次v4样本不能因合法新样本或完成通知被替换；全报告进入原风险摘要。取消／失源／到期不能被旧成功结果复活，原TTL不续期。原pending和最终write消费者尚未完成。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：工程datum对应不是当前高度分支或远端身份认证

[Datum证据](../../custom/src/Swarm/FleetMissionClearanceDatum.md)绑定原场景／构型、实际VERSION稳定内容和PRIMARY、原点参考及[目录2](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)的明确关系；其engineeringReview只是材料引用，不证明审阅人身份、机载二进制或真实高程。目录匹配成功只表示本地工程声明对应，physical／currentGlobalAltitudeBranch／sameEstimatorSample／permit等仍false。

修复Catalog原exactKeys漏receiptSequence：该字段作为真实原生输入严格校验，但不加入稳定版本绑定或指纹，不把传输收据当ACK因果、任务实例或权限。最终整组memory尾核不调用Link虚方法或weak.lock，不证明实际网络可达性。Datum的实际最终UTC通过原生输出传回审阅owner，并核原已观察下界，避免只保留更早外Source时刻而漏掉部分回退；时刻不参与稳定指纹，原单调期限和资料窗口不续期。

原净空pending、实际启动solver与两Authority至最后write及正式结果仍有实质代码。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 原25目标保持，下方453为历史。

## 453：配置来源epoch与资料时间不是远端任务身份

[Registry证据](../../custom/src/Swarm/FleetRegistry.md)的bindingEpoch／record与本地批准基线用于隔离当前实际来源，不是硬件身份签名、机载任务实例或MAVLink ACK因果证明。[Source](../../custom/src/Swarm/FleetMissionClearanceSource.md)核原JSON配置hash与本机UTC窗口；typed指纹、startup配置指纹及文档hash各自语义保留，不混作认证。

普通／地面审阅把固定来源纳入原artifact前再hash，剩余期限仅缩短，不因ACK、已阅、重新登录或来源恢复续期。真实启动净空、最终Authority／write与正式结果仍未完成，不改变原协议限制或解除pending。本批只优化地面站源码及交接材料；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余不缩减。 以下452为历史。

## 452：接地声明的payload／序号不是机载接触或停止证明

[ContactModel](../../custom/src/Swarm/FleetMissionContactModel.md)以原完整payloadHash、实际segmentSources协议序号／路径及E7位置绑定声明；这些只约束内容关系，不证明本次机载任务实例、ACK因果、远端采用或真实触地。LAND声明参考点不写回原wire，不发送新目标，不把p7当本地多旋翼PX4硬停止高度。

[报告3](../../custom/src/Swarm/FleetMissionClearanceReview.md)保存双组件及有限support地形交集，certified／permit／flight字段仍false；旧资料／报告不自动升级，不改变原协议身份限制、人工／账号批准或最终write。原pending、实际构型／datum／时效、同次启动及正式结果继续。

9源码／18文档只源码／限定静态核对，未运行、编译、测试、修改PX4、刷机、更新PR或打包；原25目标保持，下方451为历史。

## 451：后台request身份不是无线operation token

[PlanningWorker／Coordinator](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)新增的request UUID、reviewEpoch、输入指纹、实际owner及Root preparation／generation只隔离本次本地计算和审阅续作。actual finished后才采用，取消／换源不能恢复旧票据；这些都不是MAVLink ACK因果、机载任务实例或跨站控制租约证明。

Worker不发送任何MAVLink、参数请求、上传或启动；实际只读原生采集仍GUI，既有协议／命令门禁与ACK限制不变。已提交归档的receipt保实际事实，planArchiveRecorded=false只表示当前审阅绑定失效，不是远端动作回滚。原净空pending／最终StartPermit及正式机载结果仍未完成。

本批15源码／17文档仅源码与限定静态核对，未运行／编译／测试、改飞控、刷机、更新PR或打包；原25目标及六类工作继续。下方450为历史。

## 450：规划来源与独立geometryBasis不等于命令授权

[新scope producer](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)／[独立startup投影](../../custom/src/Swarm/FleetMissionStartupSpatialReview.md)仅消费值，保留原最终reviewFingerprint显示合同。geometryBasis不是远端响应、账号、artifact批准或真实路径认证；不能将后续最终artifact hash回填为自己的来源。

[Compiler](../../custom/src/Swarm/FleetMissionCompiler.md)真实执行规划环境求解、[PlanStore](../../custom/src/Swarm/FleetMissionPlanStore.md)核完整原内容，只增加冻结诊断和原记录hash范围，不新发ACK／RAW INT／ARM／MODE，不新增权限。原pending和447实际风险阻断链保持，新report成功也不能授权启动。真实原生源／接地／构型与datum对应／资料截止及最后write固定核验继续开发；原25目标保持，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方449及更早为历史。

## 449：净空纯计算不引入协议或权限来源

[几何](../../custom/src/Swarm/FleetMissionClearanceGeometry.md)与[报告](../../custom/src/Swarm/FleetMissionClearanceReview.md)只消费值，不读Vehicle、当前账号／时钟、原始MAVLink或最新草稿，不写文件、不发信号／命令、不创建收据、租约或许可。完整成员／来源形状及hash只作内容与诊断绑定，不证明远端身份、配置、时效或实际路径。

Compiler几何pending和447真实风险阻断链保持，未改变ACK／RAW INT／ARM／MODE的目标、来源、重入或真实write门槛。本批尚无实际StartReview／GroundReview消费者，不能拿独立helper ok或声明模型满足代替最终StartPermit。后续必须在同准确记录／同次原生Readiness内生成有界结果再hash，并让实际批准／Authority／发送消费；不复用UI缓存或循环hash。原25目标保持，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方448及更早为历史。

## 448：工程场地声明与内容hash不是净空认证或ACK

资料schema1的来源／版本／UTC日期、配置指纹与datumAlignmentDeclared只作工程声明；normalize或同序ID检查不认证当前场地、实际机体、性能、坐标对齐或传感器真值。planId／contentHash保完整资料内容，不是签名或授权。区域来源1／2及两处恢复预检同步严格配对，外层schema1、原binding／TTL／账号／占用／机载收据不变。

合法新资料明确几何pending阻断，沿447实际风险链消费，不因为看到地图、资料完整或恢复副本成功便获得部署／启动许可。后续完整solver及实际StartReview／GroundReview固定结果必须另外实现；不能只删除原因。历史资料今天过期不改写旧档，重新操作仍需新证据和原生门禁。8源码／17文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)，原25目标保持；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方447及更早为历史。

## 447：业务阻断不能被reviewable标签或报告可见性覆盖

共享风险检查只判断原生语法标志真实true及严格空原因；它不认证输入来源、不产生ACK、批准或启动许可。严重未命名信号在明细／原因用同一fallback，非空阻断确实使Coordinator needsReview，Deployment及普通／地面准确记录检查独立拒绝，不以人工确认跳过。

Deployment比较实际冻结与准确归档的完整规范JSON，兼容合法QStringList／JSON List表示；启动审阅只消费原绑定记录，不拿当前其它任务报告换源。历史可读不等于可部署，原TTL、来源epoch、角色／独立账号、占用、逐阶段及write尾核保持，失败不能抹已发生的发送事实。

6源码＋16文档共22路径，见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)和[待工程师验证项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。不是净空认证／最终StartPermit或正式任务结果；原25目标继续，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方446及更早为历史。

## 446：精确数值边界不改变来源、ACK或许可

v4高度门槛按原高度与整数毫米的精确线性组合比较实际binary64工程容差，不用中间舍入差值或最终一格外扩代替真实残差／漂移。零容差非零残差拒绝、非零恰等阈值接受；仅修数值判定，不更换原始参考／首次样本，不改变reference20／sample20、ACK归属、PRIMARY／来源epoch、实际scope或最终发送门槛。

数值匹配不是同采样、共同原点、内部valid、报文认证、真实净空、最终StartPermit、命令接受或正式任务成功。未新增命令、消息请求、自动批准或权限；旧v1–v3不增加高度前置。三个静态解析反例和工程师未执行项目见[高度合同](../../custom/src/Swarm/FleetMissionStartupAltitude.md)与[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批4源码／登记、15文档共19路径；原25目标继续。仅源码和限定静态核对，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方445及更早为历史。

## 445：高度一致性政策不生成ACK或内部有效性证明

v4只对实际来源报告的AMSL／local／relative同包差值及冻结原点／HOME数值执行显式容差比较。报文残差匹配不认证相同估计器样本、共同原点、PX4内部valid、真实净空或最终StartPermit；三种消息的时间不混算。

首次完整审阅hash包含原sample；稳定风险比较隔离动态sample，但必须另外对原sample检查容差。原生只读取源接口允许读取本实例保留历史，不延长账号／批准、创建控制权或恢复已取消scope。明确的新地面READ／新风险审阅可以建立自身新的首次基准，期间不得重采替换；不能拿上一轮beforeArm基准永久禁止合法重新审阅，也不能因新审阅抹除旧违规。真实发送仍经原实际owner/startup/step门禁；独立高度监视不改变step33或ACK语义，write后出现失效也不能擦除requestIssued事实。

逐包监视从本次实际scope建立才开始，同scope已观察的越界／坏帧／失源／断档不能由后续正常包清除；此前人工审阅等待期不宣称连续无违规。原v1–v3、完整25目标与正式机载结果来源继续，未编译、测试、运行、认证、改PX4、刷机、提交或更新PR／包；下方444及更早为历史。

## 444：同包高度、接收鲜度与审阅身份

[ALTITUDE解码](../../src/Vehicle/FleetStartupAltitude.md)的sameMessageFields只表明六项来自本次相同payload，不证明原始uORB或传感器采样同时。timestamp是报文报告数值；本地PX4发送时间／重复copy不能认证样本鲜度、原点代次、HOME/local有效分支或北向。原始bit串和hash不是签名或ACK。

[原生账本](../../src/Vehicle/VehicleStartupAltitude.md)记录实际本机来源与接收鲜度；NaN／Inf／哨兵各自未知，不补0，不以某项known使其它项known。[普通审阅](../../custom/src/Swarm/FleetMissionStartReview.md)仍以首次完整artifact/hash作为历史材料，当前风险比较只排新增诊断，不恢复权限或扩大TTL。诊断也不替代443原点basis、旧坐标／ODOM／EST或实际发送检查。

本批不新增遥测请求、账号／批准用途、ACK语义或StartPermit；共同参考、净空、最终许可和正式结果及原25目标继续。未编译、测试、运行、认证、访问设备、改PX4、刷机、提交或更新PR／包。下方443及更早为历史。


## 443：原点声明身份不是共同参考、认证或ACK

[原点报文解码](../../src/Vehicle/FleetStartupGlobalOrigin.md)只说明收到的本地消息49结构与数值；[实际来源账本](../../src/Vehicle/VehicleStartupGlobalOrigin.md)只维护当前PRIMARY／版本／飞行epoch及原点声明连续性。timestamp大小不认证时钟类型、PX4 ref_timestamp、boot、原点代次或同一估计器采样；SHA-256不是签名。

[阶段依据](../../custom/src/Swarm/FleetMissionStartupGlobalOrigin.md)中的stable18、referenceRevision和epoch用于拒绝旧basis／已观察ABA，不是飞控内部有效性证明。原生实际send scope首次冻结，最终write前复核；消息从未收到兼容不等于原点已验证。已write后失效不擦除requestIssued或保证未飞；本批不新增ACK、传感器请求、policy版本或最终飞行许可。

HOME／local／ODOM／估计器共同参考、净空、最终StartPermit和正式结果仍待开发，原25目标保持。未编译、测试、运行、认证、访问设备、改PX4、刷机、提交或更新PR／包；下方442及更早为历史。


## 442：联合名义检查不产生ACK、机载事实或飞行许可

[新投影](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)仅消费同次完整本地canonical请求链和独立启动参考；量化后的E7／float32及协议序号表示生成请求，不是远端收到、回读一致、实际路径或完成事实。速度保留零不作地理点，RTL未编码不补HOME。

Root同一实际Episode／身份／风险指纹／精确归档binding／整组顺序首尾核对，公开九字段中的jointReview只用于显示。报告没有Session、控制租约、已阅、批准或StartPermit；旧原报告scope不被新结果改写。原3个启动分支独立，空间接近不证明同时到达；区域／阈值hash也不认证数据发布者或净空。

本批未编译、测试、运行、认证、访问设备、改PX4、刷机、提交或更新PR／包，原25目标保持。下方441及更早为历史。

## 441：禁飞区数据身份不是操作权限或ACK

[输入链](../../custom/src/Swarm/FleetMissionAirspaceInputGate.md)的SHA-256仅比较完整JSON值；不认证空域发布者、地图覆盖、无线来源或操作员。原生task／参数不一致时阻断，不从caller提供的内部标志获得豁免；原审阅epoch正常提交并发通知，旧审批依原代次／CAS失效。

归档只取原生采集前快照；有效审阅的当前私有Intent指纹在归档前后和审批首尾核对。安全坏JSON可以保存明确阻断诊断，绝不使needsReview通过。输入修订与QPointer只隔离本地同步通知，不代表跨站控制租约或已发生动作回滚。

本批无新Vehicle/MAVLink发送、ACK语义或飞行许可，原最终StartPermit／正式结果及25目标继续。未编译、测试、运行、认证、设备操作、刷机或更新PR／包；下方440及更早为历史。

## 440：全任务本地编码与归档报告不生成机载事实

完整任务／协议明细与439起飞几何绑定同一实际Episode、Session、review指纹和准确归档记录；全组targets／planId及首尾来源核对保持。新增显示不消费批准、不调用Vehicle、不发送报文；PayloadBuilder成功只证明本地生成，不能当ACK、真实回读、机载实例或正式结果。

新空域输入元数据是原调用参数的形状记录，不是数据加载／覆盖证明。原报告与禁飞区使用原reviewEvidence，不换最新地图；原实际阈值不被更小的plan字段覆盖。既有报告scope不因共同显示扩大到起飞／结束动作，旧空记录不补成安全。

本批未编译、运行parser／helper／认证／UI／应用／设备、测试、刷机、提交或更新PR／包；下方439及更早为历史。

## 439：空间捕获与地图选择不转移启动权限

[空间取源](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)绑定当前实际Episode、身份上下文、review ID／fingerprint和准确记录；只读捕获／选图不生成已阅、二次批准、StartPermit或飞行命令，也不恢复历史来源权限。当前性变化仍保留原冻结几何的历史身份。

E7／AMSL／名义编码界限和空间接近候选只表达本地观察模型，不能当无线因果、净空、实时轨迹或任务完成。未知时间不输出假秒数，缺阈值不借默认值；Map模式切换仍隔离指点／Shift／框选，不能将只读手势变成发送。

本批未编译、运行parser／helper／认证／UI／应用／设备、测试、刷机或更新PR／包。下方438及更早为历史。

## 438：本地策略输入与真实草稿身份

[策略表单](../../custom/src/Swarm/FleetStartupPolicyEditor.md)明确采用只更新父规范文本，[完整配置保存](../../custom/src/Swarm/FleetExecutionProfileEditor.md)仍走原实际参数双hash与native revision CAS，不新增认证、ACK、批准或MAVLink发送入口。policy版本切换不复活旧审批，不改变原StartPermit旗标。

实际草稿选择代次／backing对象与账号身份不同；两者不能互换。原机号和revision相同仍需同一实际草稿，换源旧输入只供查看。保存／清除返回未确认不证明未发生本地提交，不自动重试；旧计划与审批依原链失效。

未编译、测试、运行解析器／认证／UI／应用／设备、刷机或更新PR／包；下方437及更早为历史。

## 437：人工步骤、条件认证与真实发送事实

[启动工作台](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)的确认固定当前Episode、真实主身份收据及各阶段版本；原生独立账号审批继续要求不同SID、原用途和原期限。身份显示／已阅／条件审批不是最终StartPermit、飞机硬件身份认证或任务完成。

20个动作各明确一次调用，Seq0／普通ARM／MISSION沿原实际事务发送。逐机requestIssued、rawStateObserved、mayHaveCommitted分开；ACK、观察到模式和started-observed不等于因果证明或正式结果。布尔取消回执不能推出未提交，原生返回丢失保未知；关闭UI／取消不执行DISARM或回滚。

新地面riskSummary只公开原风险白名单，不含身份／角色凭据、原任务载荷或完整认证包；实际权限仍由原Session和用途服务检查。未编译、测试、运行认证／UI／应用／设备或刷机，未更新PR／包。下方436及更早为历史。

## 436：人员简报接收不是MAVLink ACK或飞机接管

新[交接工作流](../../custom/src/Swarm/FleetMissionHandoffWorkflow.md)复用实际selected Run的owner／事务来源，保存真实观察taskUUID和准确binding。简报只保留历史MODE入队／提交及序号线索，不把它们当任务成功、当前机载任务或命令因果认证。

发送人由实际FleetOperatorSession身份收据与task.handoff角色快照一致确定；接收人是准确SID，输入格式不证明该账户存在。接收／拒绝／撤销核准确recordRevision与brief contentHash，原发送人无需保持登录。导入来源不能恢复本实例当前请求，哈希不是身份签名。

本路径不新增MAVLink消息、ACK消费者、机载改派、占用释放、启动许可或跨站租约。Manager信号回调发生后仍保留已创建ID和状态变更事实，context失效不等于回滚；未知结果不自动重试。仅静态阅读，未运行认证、协议、UI、编译或测试。下方435及更早为历史。

## 435：只读显示序号不变成协议授权

共享显示后端只对同一实际新plan调用原PayloadBuilder，按真实sequenceNumber联结raw／canonical／sourceItemMap／量化；没有发送、ACK消费、MissionManager操作ID或原生租约变更。规划点索引各路线从0开始，动作另占协议序号；旧Run的CURRENT／REACHED不被用于猜新项、跳过任务或认定完成。

Root保存原九字段binding、实际outbound scope与新record，再缓存只读view；公开值增加第12字段，既有审批／上传／占用消费者不读取view作许可。过期仍可显示历史，但不能恢复旧授权；地图只读模式保持原规划点击隔离。只核源码，未运行协议、认证、UI、编译或测试。完整启动许可、机载接替和正式结果仍未完成。

## 434：新记录本地批准与原机载上传身份仍分层

新[余段审阅上下文](../../custom/src/Swarm/FleetMissionRemainderPanel.md)的token只标识本地新审阅；九字段绑定和current-outbound scope不成为MAVLink线上的任务实例或控制权。人工采用、意图申请／批准、独立账号上传批准和实际机载任务写入分别保持原步骤。

新入口复用原request／confirm上传与读回，不新建协议token、不更改ACK匹配或自动重试。唯一[部署面板](../../custom/src/Swarm/FleetMissionDeployment.md)冻结来源及归档身份；换源不沿用旧确认。余段刷新目标取新plan.members，而上传后内容读取／失败对账继续取actual Deployment原冻结目标。

本地批准已发生但尾核失效时保留committed，不宣称回滚；上传／回读／MODE／REACHED仍不能认证正式机载任务完成。原Run占用不因新提案解除，没有新增ARM／Start入口。未编译、测试、运行或实际验证协议；下方433及更早为历史。

## 433：原协议进度只映射来源，不成为余段完成证明

[余段取源](../../custom/src/Swarm/FleetMissionRemainderSource.md)核实际Startup/Run/Deployment、原完整payload及sourceItemMap；只能从该已提交MODE窗口的目标取得progress。当前/最后到达序号若存在，必须与原payload的那一项对应。速度、起飞和终态动作不是原航点，禁止固定减偏移或自动index+1。

模式提交、ACK、CURRENT或REACHED均不认证远端任务实例，也不证明连续完成了之前所有作业/停留。新[规划](../../custom/src/Swarm/FleetMissionRemainderPlanner.md)由人工明确选0基原索引，包含该点与完整后缀停留；只生成单成员新审阅提案。来源摘要/route hash保持内容一致性，不恢复旧审批、命令租约或跨机控制权。

本批不增加MAVLink发送、读取、ACK处理或自动重试；纯PayloadBuilder重建只比较历史编码，不是新机载回读。原Run/成员占用/未知结果保持；已发生的新本地归档提交与“返回时是否仍当前”分别报告。未编译、测试、运行或实际验证协议；下方432及更早为历史。

## 432：备用计划身份不成为机载接替或命令ACK

[备用策略](../../custom/src/Swarm/FleetReserveEnergyPlan.md)绑定原 reserve vehicleId 和明确 replacementVehicleId；targetRouteHash 覆盖目标原编译航点内容，仅用于本地规划身份，不是签名、远端任务实例、当前进度或已飞轨迹。输出几何剥离原主机时间／速度假设，不进入 active members／上传载荷。

原草稿 revision、完整source、reserveMembers内容及既有计划hash共同使编辑后的旧审阅失效；保存／清除使用同一实际Draft的版本比较，不自动重试可能已经提交的修改。旧source缺策略时不补默认，缺预算不能凭旧主机账单通过。Guided原主备身份、实际电池来源及扣龄门禁保持。

本批没有新增MAVLink请求、命令、ACK或控制权转移；预算完整、estimate-feasible、保存成功均不表示机载已接替或获得飞行许可。未编译、测试或通信验证；原执行／交接／结果归属工作继续。

## 431：电池来源身份不成为ACK、性能认证或控制权

[原生电池观察](../../custom/src/Swarm/FleetNativeBatteryObservation.md)绑定实际Vehicle／Protocol／MVM／VLM、PRIMARY共享所有者、版本代次和默认组件；原始收包／提交执行完整源检查，公开getter只检查本机内存连续性。sourceEpoch、receiptSequence、contentRevision分别标识本地来源、接收与语义变化，不是物理电池序列号、签名、远端测量时间或远端任务实例。

[Registry桥接](../../custom/src/Swarm/FleetNativeBatteryBridge.md)不接受描述／护照恢复live数据；Guided审阅冻结来源与语义，不把每次年龄推进当内容变更，但另核当前扣龄估算仍覆盖原预算。来源变化、过期或不明状态不能凭同机号／旧百分比沿用旧审阅。合法相同内容新包刷新接收年龄仍不证明远端新测量。

本批没有新增MAVLink请求、命令、ACK匹配、自动接替或审批权限；原有上传／启动和任务归属机制保持独立。estimate-feasible只供审阅，不构成起飞许可或任务结果。未编译、测试、运行或通信验证，未改PX4、刷机、提交、更新PR或打包。

## 430：能量成员身份不产生机载ACK或执行权限

[能量账单](../../custom/src/Swarm/FleetEnergyScheduler.md)绑定实际Compiler原主机和备用机；Risk核对完整成员行及合格子集，不能以同人数、较高电量备用或外部候选替代原主机。这里的vehicleId是现有规划协议地址，不是硬件认证或跨站控权凭证。

预算、eligible、feasible、estimate-feasible和百分比预测都只供审阅，不构成上传／启动收据、ACK、机载任务完成或允许起飞。没有新增命令、自动替补、审批权限或MAVLink事务；原命令身份与观察链不改变。仅源码静态阅读，未编译、测试、运行或放行。

## 429：运行报告不创造新的ACK或机载结果

Root报告取源绑定实际selected lifecycleEpoch／selectionGeneration／episodeId，再沿其startupSeries.previousStartup核startupId、transactionEpoch、generation、sourceHash与runId；不会用UI传来的Run值、外部文件或另一组initialTransaction代替。稳定身份复核只防止本地错配，不认证飞机。

MODE队列开始／提交覆盖、post-commit航点观察及历史落地候选均保留原含义，不升级为远端任务实例、完成回执或成功率。零MODE提交也不证明此前没有ARM／序号命令。reportId和contentHash只识别冻结文件；导入metadata不恢复账号、审批、控制权或任务状态，报告动作不发命令与关联启动事务可能已发命令分开表述。

本批无协议／ACK控制路径改动，无编译、运行或通信测试。详见[报告合同](../../custom/src/Swarm/FleetMissionExecutionReport.md)和[工程师待验证项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 428：历史规划来源与当前任务身份分离

[历史规划入口](../../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)消费[PlanStore](../../custom/src/Swarm/FleetMissionPlanStore.md)实际已加载记录的精确7字段身份，不以latest查询、旧liveReview或已恢复的record替代当前授权。storeEpoch绑定本实例内存来源；recordId／revision／planId／内容和审阅hash只作历史记录一致性，不是签名、远端ACK或执行证明。

历史提案会话sourceTaskId不写回当前IntentTask，也不是机载执行任务ID。新草稿从revision1开始并要求重新确认地图基准；原生审阅重新捕获当前fleet／设备依据。旧reviewEvidence和审批不随源导入，restoredFromArchive只标明本地记录加载状态。

此入口不要求历史Deployment或426结果存在；也不因为读到历史计划而生成对账匹配、机载内容收据或起飞许可。共享草稿切换仍先检查实际发送／排空并撤销旧审阅；没有新增MAVLink发送或ACK消费。目录与取源不写盘、不清隔离、不改旧记录，后续新归档仍受原写入门禁。未运行协议、UI、认证、应用或设备验证。

## 427：修复副本来源不是机载修复回执

[修复取源](../../custom/src/Swarm/FleetMissionRepairDraft.md)只读取实际Deployment冻结plan的完整编辑源，并核该实例原426 requestId、deploymentId/generation、binding、目标顺序与终态排空。不得把当前Intent、同号新飞机、生成路线或调用方提供的matched摘要当作原始草稿；旧记录缺源明确不可恢复。

Compiler在封存前核完整源与八个实际规划字段相符，planId包含原task，contentHash另含规范完整源。哈希与本地对象关系不是数字签名或机载认证。读取终态可以是completed/stopped/cancelled，different/unknown原样保留；建立新草稿不改变原机载任务、历史结果或旧record。

Draft sourceRevision是历史编辑代次，新实例固定起于revision1，并重置地图基准确认。Root的草稿对象／来源token与同一提案的新record版本分开，不产生新执行taskId。native load本地提交、Root发布为当前草稿和后置current状态分别报告；取消预览／切回旧草稿也不能恢复以前的上传或启动批准。

此批没有新MAVLink发送或ACK消费者，不增加重试／上传／ARM／MODE路径；后续上传仍须新审阅及独立批准。未编译、测试或运行协议／UI／认证／应用／设备。

## 426：独立诊断READ与启动收据隔离

[诊断传输](../../custom/src/Swarm/FleetMissionReconciliationTransport.md)新增独立DiagnosticDisarmed admission，只由Deployment私有实际Reconciliation调用。原StartDisarmed及两种POSCTL Review使用显式枚举判断；不能把新增诊断当作armed Review。原生owner／Vehicle／MM／主链路控制块、scope及本地operationId与每包完整授权检查分开核对。

本路径仅REQUEST_LIST、REQUEST_INT和读取终止ACK。真实COUNT0只在诊断分支放行，仍须收到COUNT、完整来源、末ACK实际排队；失败不发布部分rawItems，也不以空显示缓存伪装COUNT0。非零项数和预算在分配前检查，FLOAT回退及非法项保持拒绝。

诊断完成走独立signal；调用方排队等待原完成栈退出后，经原owner/source/scope/op一次性take取得35键结果。成功不交换普通_missionItems、不写_lastFleetReadReceipt、不发布普通缓存完成通知；新事务使旧诊断槽失效。原Start/Review不得通过诊断summary/hash或旧历史批准获得新资格。

工作流另外输出30键批次摘要和16键逐机摘要；接受回执为摘要加ok/requestAccepted共32键。COUNT0／数量差异可报告different，同数量但无法按支持的编码规范化仍unknown；matched要求原规范化items与hash一致。输出不保留完整RAW数组，不伪装425普通解码字段差异，也不认证当前机载状态。

本地scope/op、真实原始接收及末ACK排队仍不能证明远端响应因果、全队原子快照、消息签名、控制权或飞行执行。仅源码与文本审阅，未运行认证、协议、QML、应用或设备，未编译／测试／刷机。

## 425：普通READ完成结果与差异诊断身份

发现原PlanManager普通READ在success=false后会清空缓存，仍发送newMissionItemsAvailableWithOperationId(operationId,false)。第二个参数是removeAll，不是成功标识；仅凭它、idle状态或空列表不能证明收到有效COUNT(0)。

新增friend-only fleetDeploymentReadSucceeded，在原TransactionFinish栈、原closed deployment scope内核原owner／Vehicle／PRIMARY控制块、部署ID／generation／operationId、最终success、已queue末ACK与真实COUNT／cache数量。success和COUNT在旧通知前冻结，scope排空后清除；不生成可转移收据，不调用QML／认证，也不认证远端或实际执行。

Deployment在真实回读采集前后核该完成依据及原来源，不保留跨_gate的Target引用；有限诊断另附原部署、机号、读写操作号与本机观察时间。matched仅是旧严格规范化比较的历史结果；取消／失败保留诊断但撤掉live匹配，不能复活历史批准或启动许可。

比较helper不参与命令ACK匹配、重试、发送授权或最终StartReview门槛。普通解码路径可能涉及FLOAT回退，不能把新摘要称为独立RAW INT证据。失败后的独立整组READ／修复草稿仍待开发；仅静态源码核对，未运行相关信号、认证、传输或设备场景。

## 424：搜索游标、文件身份与飞行身份分离

跨库查询采用独立外部6字符串意图：action、expectedRunId、scope、vehicleId、operation、previousQueryId；原生内部5键search请求不接受调用方文件路径或任意SQL。后续扫描取本Store私有实际游标，核对同一成功结果ID、范围、筛选和调用run；不会借用其它面板查询或重新录制前的旧结果。

result的文件来源、databaseEpoch、整文件指纹和文件内rowId描述本机历史观察，不是MAVLink ACK、机载任务实例或控制租约。目录清单指纹仅绑定观察到的目录身份/文件元数据；跨库原子性、历史完整性、认证、权限恢复及飞行许可仍false。

查询失败/取消不发布部分批次成功、不创建新审批或任务；结果归属和实际线程finished分别处理。本批不修改ACK路由或飞行控制链。合同见[Reader](../../custom/src/Swarm/FleetEventArchiveHistoryReader.md)、[Store](../../custom/src/Swarm/FleetPersistentEventStore.md)；未编译或运行验证。


## 423：离线查询身份与飞行ACK完全分离

产品动作offline沿用intent6/return9和native生成queryId。expectedRunId只冻结本Store的调用范围，不筛选离线库的历史run；新离线页runFilter为空，页内各row仍保存其原runId。当前本机run或本页查询ID均不是机载任务实例、控制租约或飞行ACK。

后页使用本Store实际发布页的databaseEpoch、整文件fileFingerprint与nextRowId，不能用caller提供的哈希、封存catalog或实时页代替。文件指纹用于检测变化而非证明作者/硬件身份；协作锁只协调本地文件访问，不能恢复任何批准或授权。

离线读取不创建BlackBox事件、不回放控制/规则，不改MAVLink及原审批证据。historicalOnly保持，权限恢复/执行/飞行放行标志均false。未执行运行验证；静态和工程待办分别见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)与[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 422：归档请求ID不等于审批或机载ACK

本批未更改MAVLink ACK、飞行命令发送或机载完成归属。新归档intent6／return9是本地文件诊断请求：start/stop/seal无查询ID；catalog/current/page由native生成ID；cancel只对原产品pending ID。lastHistoryQueryId只识别实际发布历史结果，不能冒充飞机执行确认。

工程摘要固定两事件族，能力approve/revoke与基线approve-baseline/revoke-baseline不可混族；v2解码绑定原type哈希和fleet分类。摘要的mutationCommitted/nativeOk/stillCurrent保留yes/no/unknown；原生回执字段不可核实时不补出成功。reviewIdHash/typed指纹不是账号、硬件、性能认证或签名。

本次审批后摘要是独立best-effort BlackBox记录，不把EvidenceChain append当SQL写入，也不把未收到日志当已回滚；记录失败不恢复旧批准。历史页永不生成执行许可，不恢复权限或调用实时规则；新实时BlackBox摘要仍按原实时通路通知消费者。原跨站租约/正式机载结果等缺口保持。

限定检查见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)，所有运行场景见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)且均未执行。

## 421：构型基线审批用途与提交事实

新增角色动作 `fleet.configuration.review` 与账号用途 `fleet.configuration.review.approve`，不能复用能力 `fleet.capability.review` / `fleet.capability.review.approve`。真实Review冻结operation，Session typed消费和纯内存尾核均从实际owner取精确用途；独立SID、原30秒和身份TTL持续绑定，不生成lease/StartPermit或机载控权。Root冻结两类政策，同一界面的选项不是凭据。

未知当前配置仅可撤销实际存储的原批准，凭原条目独立静态指纹和比较16/观察epoch/revision绑定；不接受描述符fallback。新基线mutation回执14先于通知冻结，后置删源、变源或审计失败不抹真实commit。Root新增 `fleet.configuration.review.result` 与原能力事件分开，8键approvalEvidence保存实际purpose；这不是MAVLink ACK或远端完成结果。

未改MAVLink命令/ACK匹配、PX4或已有Run/占用，不认证硬件/性能、跨站授权或正式机载结果。仅源码交叉核对，未运行认证、协议、编译、测试或设备。下方418的固定能力purpose说明仅为旧路径。

## 420：构型观察身份加入现有审阅关联

[固定合同](../../custom/src/Swarm/FleetConfigurationBaselineEvidence.md)的 observationEpoch/revision、原类型当前/基线hash和原批准hash进入连续性建议、Guided旧/新快照比较、Coordinator归档members hash。新纯值桥只读传入原snapshot，不通过当前Registry(id)洗掉旧身份；实际审批仍核原当前来源。

Deployment 增加 capabilityReviewEvidence 与实际审阅同值检查；旧档案不能代替实时来源。没有新增 MAVLink 命令、ACK匹配方式或启动许可，已queue/未知结果、Run和占用不因此回滚。回到同内容可做新审阅，不复活旧观察身份。仅限定静态阅读，未编译、测试或执行协议/设备。

## 419：能力审批绑定新增构型观察身份

本批不改MAVLink ACK、命令发送或飞控。原能力预览改schema2/18键，冻结实际Registry的独立observationEpoch/comparisonRevision、basis及前后typed指纹；原map仍私有按类型核对，不以Qt JSON展示或截断字段行代替身份。

原context6/approval14、current30/result16/Root收据5和独立账号一次消费不扩字段。full丢失/恢复、typed-only变化及审批-only导入触及原条目会使旧批准/预览失效；普通健康与重复完整配置不制造新修改。Registry最终围栏另固定原history身份。工程声明、账号审批和本地比较均不证明实物/性能或远端任务执行；旧哈希基线政策尚未统一升级。限定静态阅读，未运行或验证ACK时序。

## 418：能力声明的账号决定不是 MAVLink ACK

[新工作流](../../custom/src/Swarm/FleetCapabilityReview.md)使用独立 fleet.capability.review.approve。原maintenance/admin申请与另一合格SID审批，实际Session/Episode/Role/Root/Swarm/Registry、原30秒和双身份TTL持续核对；typed一次消费不产生lease/StartPermit，不复用上传/启动/恢复或目录用途。

原生context6/approval14保持。新Registry回执14记本地mutation与stillCurrent，工作流result另保8键approvalEvidence，Root日志将其与可编辑角色审计上下文分开。提交后删源/重入/追加日志失败不抹真实commit；consume=false也不能证明批准凭据没消耗。

双账号只对实际工作流路径成立，旧native bool和原生fence不能被描述成全入口账号服务。硬件真实性、改装部件、实测性能、机载结果与跨站控权均不由此认证。仅源码交叉阅读，未运行认证/测试/设备或放行。

## 417：本地目录重载不是 MAVLink ACK 或固件认证

新[Reload](../../custom/src/Swarm/FleetFirmwareCatalogReload.h)独立用途 `firmware.catalog.reload.approve` 仅消费本次完整目录替换的第二账号决策；维护/管理员申请、不同合格账号审批，实际 Session/Episode/Role、Root/current scope、原 identity TTL 与30秒 Review窗口绑定。不复用 upload/start/ground/close/recovery receipt，不生成 lease、StartPermit 或任何 MAVLink 指令。

原目录 epoch/revision/fingerprint 的 CAS 与原始文件/规范内容两种哈希绑定完整旧/新声明；`engineeringReview` 仍是档案自身的原始声明，不是当前审批人的固件签名或审阅者真实性证明。最后固定文件复读后仅走纯内存原来源/角色/双TTL/原Review期限围栏。替换的是内存 catalog，不写档案；原 startup 自动 load 未改为两账号，不称全入口统一授权或防回滚。低 archiveRevision 仍可被明确审阅后重引入，不自动可信。

Catalog.changed 使旧任务 review 失效但不停止原 Run、释放占用或宣称 Completed。`mutationCommitted`/`reloadCommitted` 是本地历史事实，和 `stillCurrent`、外层后置失败及 audit append 结果分别保留；日志追加确认也不是断电 durable 证明。全球任务 ABS 水平/垂直及非 CONST_POS 补口不改变 RAW合同，也不能认证 HOME/local、净空或最终机载结果。

未编译、测试、运行解析/helper/UI/认证/应用/SQL/设备、改 PX4、刷机、更新 PR 或打包；原25目标/六类继续。仅限定源码交叉阅读，以下416及更早为历史，不外推为全链 ACK/身份或运行验收。

## 416：参数内容身份不是 ACK、账号授权或执行许可

[只读 Coordinator 观察](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)使用实际 MVM/model/Vehicle/ParameterManager/VLM 与原PRIMARY共享所有者；12项 live-param-value 观察均须当前可用、30秒内、缓存吻合且无待写。参数内容 hash 保持原排序 name/type/value 算法，稳定 `observationFingerprint` 另保留共同 PM epoch、逐项语义 revision、来源及可用状态，剔除不断增长的 age/receipt。失败不提供可绑定 hash/指纹；不 refresh 或写参数。

全部可回调来源读取后，原生PM整组 `parameterObservationsMemoryMatch` 再核实际12行语义内容、原PRIMARY共享身份、PMepoch与当前鲜度；尾栅不调用Link虚函数/OS，不从cached Fact单独恢复许可。保存桥的只读调用先结束，随后本地Draft CAS仍使旧review失效；不是为避免自拒而恢复旧批准。

操作员保存只把该内容 hash 写入本机 `verticalPolicy.requiredProfileHash`，Guided 先重新读取并精确匹配原指纹/hash及准确 Draft revision，不能凭 caller 的任意非空字符串通过。该本地意图不生成 ACK、Session proof、租约或权限；硬件认证/flightCommandReleased/executionPermitted 仍 false。完整 native review 之后仍重新核参数、来源、适配器、计划与审批绑定，不把观察 DTO 当12参数原子事务。

Draft CAS 内容提交在通知之前；false 可能是通知销毁/重入后的返回，不能宣称未提交或自动重试。角色变化保留本机 profile、reserve 不活动输出、删除重加不继承。旧 review 因编辑失效；高级 startup JSON 可空但不允许受控启动。阶段仅416，无编译/测试/UI/认证/设备或运行验收，未改 PX4、更新 PR 或打包；原25目标/六类及正式机载结果等缺口继续，下方历史记录保留。

## 411–415：恢复用途与原始来源身份

[面板/Root合同](../../custom/src/Swarm/FleetOccupancyRecoveryPanel.md)使用新6参ForController实际Root模式；native Direct回调5个QVariant实际Review/Session/Role/context/reviewId，Root回调只读scope/私有意图/输入代次，不调用Session.current、identitySnapshot、OS认证或再取控制器call。旧5参requestReview兼容入口仍可由QML调用，不声称语言/进程安全隔离，产品面板不回退它。

独立fleet.occupancy.recover.approve、原始Session TTL/当前Episode/Role与逐机源证据不因UI接线延期。27键current/13键result新增controllerManaged、authorizationOwnerRetained仅是持有关系，8键值收据本身不授权限；13键诊断明确当前真实unknown且不提供clear。两个未知actor仅显示规范为空，不参与空身份通过。

BlackBox接纳结果与操作结果分离：false是未接纳，异常是接纳未知，均不抹已返回localHoldsReleased。没有ACK伪造、飞行dispatch、远端空闲/控权证明或新lease。未编译、运行认证、测试、UI验证或放行；原25目标/六类继续。

## 406–410：原生重启／历史占用专用复核

[恢复复核合同](../../custom/src/Swarm/FleetOccupancyRecovery.md)新增实际Swarm唯一拥有的原生工作流，公开只读对象和request/独立审批/取消入口；未加QML操作界面。bootstrap历史待审与本轮真实unknown分集合，整组最多32个选中地址只可解除restart/history holds，真实unknown、实际owner、任务、临时任务、隔离和everObserved历史不清除。

原Vehicle/MM/参数/VLM/MVM/model及主链路共享所有者冻结，新鲜2秒内原始HB未解锁与已知ON_GROUND、完整本机队列排空进入30秒复核。专用fleet.occupancy.recover.approve强制不同真实账号，一次消费不生成lease；原账号TTL、当前Episode/角色在最终无回调尾栅再核。现场处置编号和地面检查是明确人工声明，不认证物理停机、远端无任务或控权。

所有可回调读取后，整组再以纯native原RAW语义revision/hash/value/鲜度/队列核对，并核原worker、实际占用和任务map；同栈整体移除本地待审后bump，新actual身份须由原worker实际提交才恢复既有continuity getter/CAS。留存原QQ、源epoch与primary控制块，换源在getter/最终CAS即时重加hold，不只等timer；正常收包、合法起飞和新MM工作不冒充换源。

本批12源码/工程登记、17文档，共29路径。原生recoveryWorkflowImplemented与UI集成分开；localHoldsReleased仅历史动作事实，selectedReviewRequiredVehicleIds另报当前待审。仍缺UI、离线/报废现实处置、本轮真实unknown受控解除和完整机载自愈；完整25目标/六类实质开发保持。未编译、测试、运行helper/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方401–405及更早是历史状态，不以旧“无任何再审入口”覆盖本批native入口，也不宣称全部恢复产品已完成。

### 406–410 恢复不是旧ACK或Close复用

新purpose只接受实际Review绑定、不同真实SID与原Session pending审批的真实一次消费，不消费upload/start/ground/ObservationClose receipt。consume后源getter仍可能切Episode、变角色或耗尽原TTL，所以最终pure fence再次核当前Episode/Role/原双期限；没有新lease或延期。

RAW地面状态、两次一致性读取、账号认证与现场声明分别标作用域；都不证明远端任务absence、控权回收或Completed。最终whole-set pure RAW/queue核之后才核原worker/占用/taskmap及原身份尾栅并同栈移除两类本地待审。新actual必须真正commit，不以offer/ACK/旧文件/原始true代替当前状态。真实unknown、临时/任务/owner隔离不在此用途消除范围内。

## 401–405：原生异步占用留存与重启待审

[占用留存合同](../../custom/src/Swarm/FleetOccupancyJournal.md)已把严格11键Codec和单槽合并worker接到实际Swarm构造/原生timer，不依赖无人调用的save/load或QML可写路径。固定AppData目录、本地合作锁、原文件基线/目录身份前后核验、无直接覆盖回退的原子替换及读回，与当前实际源身份分开；不是完整捕获、write-ahead或断电durable证明。

每个新Swarm先为1–255协议地址建立独立restart-review holds。历史ID只并入真实未知，不重建owner、任务、Run、账号或权限；空/缺失/旧open文件和写入成功都不能解除这些holds。纯重启占位不冒充已观察到的飞机记录。当前尚无专用再审解除流程，所以本阶段连续性改派保持待再审限制，不能称恢复后已可调度；此流程仍须继续开发。

公开native占用快照必须等原worker bootstrap、当前完整身份已实际提交并读回，且actual==offered==committed；存储失败/失源/坏文件/代次耗尽拒连续性提交。磁盘IO只在worker线程，原Land/RTL/ABORT不等IO、不因账本失败新增否决；析构只协作stop，不写clean或释放飞行占用。只读状态标明last-poll和recoveryWorkflowImplemented=false。

本批7源码/工程登记、15文档，共22路径；不是全部25产品目标、安装包或完整交付。QSaveFile结果仅记atomicReplacementConfirmed，osSyncVerified/durableVerified始终false。未知授权解决、正式受控执行/StartPermit、可信性能、企业跨站、长期运营及真实模型/生态等六类仍有实质源码工作。仅源码和限定文本读回；未编译、测试、运行解析/helper/业务/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方396–400及更早为历史，不覆盖本节新的重启待审门禁。

## 396–400：实际本地占用进入连续性审阅与提交

[原生占用合同](../../custom/src/Swarm/FleetNativeOccupancyEvidence.md)新增actual Swarm快照和最终本地角色CAS守门。部署撤权不删除raw占用；编队、创建/预抢占栈、活事务、安全owner、临时任务/清理及内容隔离分别保留。已知结果不明不会因原记录清理变成可用；当前账本没有自动清除或跨重启恢复证明。

Continuity排除所有held/unknown候选；原成员任一实际占用即整任务hold，不通过换掉成员来假装释放。Guided刷新/应用要求原实际快照，稳定原生身份进入建议；缺失或坏证据不能产生产品可应用的完整提案。旧孤立值诊断仍明确requiresNativeOccupancyReview，不授来源认证。

Swarm与Orchestrator双向sticky pin，源死不换空实例。新六参数角色CAS在同栈提交前核冻结身份与old∪new实际状态；绑定后的旧本地入口也不绕过当前占用检查。不改实际owner、不发命令、不解除native Run保护；只代表本地模型变更，不是飞机空闲、正式机载改派或控制租约。

本批12源码/工程登记触点、15份文档，是累计工程增量，不是完整25目标完成或安装包。包括直接相关的CommandTransaction通知后存活/原尝试检查及嵌套start/retry保护，不是全仓回调安全认证。未知状态的授权解决/跨重启留存、完整受控机载执行、性能/能量与企业跨站等六类仍继续。仅源码与限定文本审阅，未编译、测试、解析/运行helper、业务/认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅摘要占用/五参产品应用”的描述由本节覆盖。

### 占用证据与ACK/远端任务分离

原生occupancy epoch/revision/hash只绑定当前实际Swarm的本地状态，不是MAVLink ACK、远端任务实例、账户认证或控制租约。撤权但仍held与已知unresolved outcome不因terminal/cleanup消失；新六参接口没有发包或回滚原动作。纯值一致性和actual对象来源分开，最终由原生sticky源核对，不接受caller的free bool授权。

新的只读诊断账本失败/容量/代次耗尽不得新增安全逃逸否决：占用证据失效后连续性CAS关闭，原Land/RTL/ABORT仍按原命令/安全门禁处理，不因缺诊断token直接return拒绝。

事务创建返回后的原目标复核与实际ACK分开。限定9处创建调用、4处编队外层及停止入口、直接编队通知/Idle/暂停和Goto清理/取消同步链，以及2个MissionManager尾窗采用原QPointer、原ID及record/phase代次，不以同ID重连来源替换原源；这不覆盖其它历史或异步完成回调。Land/RTL的单目标失效不能整组挡住仍有效安全目标；dispatchedIds只代表本次派发安排，dispatchOutcomeUnknownVehicleIds保留不明事实，两者都不认证已发包、ACK、空闲、飞行结果或远端控制权。

底层CommandTransaction的通知后存活/attempt检查及嵌套start/retry保护只维护原生本地账本；计数、bool true、Running/terminal与callback返回均不是MAVLink相关性或执行结果证明。通知销毁不回滚已接受事实，旧timeout栈不沿新attempt发布retry；没有改ACK信号、远端任务归属、占用解除或任何飞行许可。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](../../custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

### 替补匹配与本地CAS不授机载控制权

slotId、replacementBindings、建议批内承诺和任务revision都不是MAVLink ACK、远端mission instance、硬件/账户认证或真实控制租约。新CAS没有发包或修改旧批准，native Run仍必须走其实际owner工作流；任务摘要显示空闲或观察closed不能证明Swarm实际未知/未排空占用已释放。本轮不更改ACK通道、启动收据或飞行来源合同。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](../../custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

### 能力审阅不是ACK或账户证明

`capabilityReviewContext`/批准UUID、记录epoch和revision仅标识本地工程记录；不能当作MAVLink ACK事务、机载任务实例、真实操作者身份、跨站租约或飞行许可。值校验器只验证供入快照的一致性，Registry普通遥测/护照入口不接受外部有效批准；真实来源与账户授权仍须由原生后端及后续接入分别保证。本轮无新增发包路径、不放宽原有账号/双人审批和最终queue门禁；`hardwareIdentityAuthenticated=false`、`flightCommandReleased=false`保留。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

原包receiptSequence是本机接收序号，semantic revision只记录本地语义变化；outcome receipt中的sourceEpoch/接收时间同样不是MAVLink任务实例令牌。completeStateReportedNotVerified只记录收到该扩展，不认证本Run结果。原ACK/RAW/账号合同和三个执行权限false不因本批改变。

## 371–375：本地切换 receipt 不升级 ACK 或远端执行归属

两个 `ForController` 接口连续执行实际 Root begin → 私有 permit mutation → 新 scope/原 owner-ID end，两个 Root fixed API 与双入口面板只选择本地对象组。公开12键 metadata、6键组行和12键 transition receipt 不包含 raw refs；native mutationCommitted/newEpisodePublished/selectionChanged 与 end 独立，它们不是 MAVLink operation token、签名认证、远端因果或任务完成证明。

prepare 不自动改 selected、迁移登录/批准、停止旧 Run 或释放占用；本地选择不转移远端控制。失败尝试也计8组容量，allocated=false 不证明未计费；end/后置失败不能抹已提交事实或解释为回滚。已返回发布事实先处理本地 token/身份意图，再作后置 source getter，不将未知尾部当“未发生”。原账户 proof/TTL、ACK、RAW/普通 mission 传输合同均未因该入口扩大。

本批8源码/19文档，`outboundIntegrationComplete=false`、完整25目标/六类仍有缺口；限定文本复核的实际记录见本轮静态审计。未编译、测试、解析/运行 helper、认证、应用或设备，未更新 PR/包。以下366–370及更早身份审计保留为历史，旧无入口/API口径由本节覆盖，不提升运行或飞行安全结论。

## 366–370：控制器调用归组不升级线上 ACK 证据

草稿失效后的cancel/revoke-only补偿不是ACK撤回；无法核实时 `retiredCurrentScope` 持续拒同组current入口/回调，不超时恢复，selected观察仍可读。Qt精确引用/类型往返未经目标环境验证，不能把本地作用域、5源码增量或原收据路由当协议级执行身份；交接必须保留累计源码依赖。

新增三 native controller-call 和私有 Guided factory/14引用/30入口/24来源回调，finally 只按原实际 owner+callId 清理；pure scope 只证本进程实际引用同组，不因同步忙自拒。nested draft 会使 active current stale，但后置失效不能抹已 queue、可能发生或未知结果。两类 QML 注册与 selected token/完整收据路由不增加 MAVLink operation token、签名认证、远端因果、原子任务快照或正式完成归属。

selected 四值同 scope、token 含选择代次；原 retained Close 的完整 issued 收据只确定本地路由，取消不按 UUID 搜索、不停止飞机，显式监视登录不结束 Run。新部署/选择入口及专用 transition API 仍缺，`outboundIntegrationComplete=false`，原25目标/六类继续。5源码/18文档已完成限定文本审阅，不升级ACK归属、运行或安全结论；未编译、测试或执行业务/设备/PR/包。以下保留历史身份审计。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](../../custom/src/Swarm/FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

新scope的authorityGranted/flightCommandReleased严格false。actual对象/代次相同不是身份或任务状态证明，也不是ACK因果token；全进程提示token只串行本进程认证入口，不产生新的account proof、flight grant或跨站控制权。RAW、普通mission传输、启动ACK与已有凭据schema不变。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](../../custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

actual QObject首参只关联本机服务生产者；不是MAVLink目标/ACK操作token、密码认证或执行归属。相同binding/账号/UUID不替代实际producer，原native sticky/final sender门禁保留；本轮未修改ACK解析/链路或RAW凭据。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](../../custom/src/Swarm/FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

本轮普通部署sender不是RAW证书：本地operationId只关联本实例，MISSION_ACK不携该token；实际queue与飞控接受、无线因果、机载执行分开。允许的普通消息按target/type/phase/seq/count核实际操作，插件不能换payload/header；原signed帧还冻结seq/checksum/signature，不新增密钥核验或认证来源主张。原独立RAW及启动ACK来源边界不变。

## 346–350：十二类原生服务门禁接线

[Episode服务接线](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)已覆盖另外12类服务的新请求、live依据、审批/身份移交及现有scoped发送fence：归组来源须仍为current且owned peer属于同一实际组；旧取消/revoke/drain、被动Run/Close和真实已queue记账不借此封禁。Authority/Txn认证移交上下文改为纯本地比对，完整身份/风险门禁仍在调用两侧；Series保留严格一次未发布构造绑定。

仍未形成新部署可用闭环：普通Deployment上传/回读的legacy PlanManager在插件后、异步每包/重试没有独立Episode fence；Guided actual Episode/epoch、共享Role身份上下文和selected旧观察整组投影尚未接入，无新QML注册/部署入口。只改源码及限定静态审阅，不编译、测试、运行认证/应用/设备、更新PR或打包；原25目标/六类继续。


## 345：非 Close 用途的原生身份门禁

[Session 身份接线](../../custom/src/Swarm/FleetOperatorSession.md)已加入上传/启动/ground用途的current Episode检查、实际owner配对及OS/owner回调后的再次核对；旧组普通身份事实、显式登录/退出、精确撤销和独立Close用途保留。相同账号/UUID或不同工厂的当前对象不能替代同一实际组。

这是340–344核心后的源码增量，不是全链隔离完成：其余owned发送/审批入口、Guided actual Episode/epoch和selected旧观察投影仍未完整接线；无QML注册/新部署入口，现有应用仍单链。只做源码与限定静态审阅，不编译、测试或运行认证/应用/设备，未更新PR或包，原25目标/六类继续。


## 340–344：Episode 是本地实际来源边界，不是新的 ACK 或身份凭据

[Lifecycle 核心](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)为每代创建14个实际 owned 对象，新 Session signed-out，不复制旧身份、receipt/proof、期限或批准；旧 actual Txn/Series/Run 及实际占用保留。Episode UUID/QQ/父级和固定18键 native 静默证据只识别本地来源，不认证远端执行、落地或任务完成，也不使旧 ACK/相同 binding 成为新代许可。

共享的五个实际 typed 来源在首次成功后固定；共享 Role 的当前任务身份与 retained 观察关闭身份仍须后续显式作用域接线，旧 Session changed 不得推定新 actor。容量8次含初始/失败尝试，不销毁旧实际对象来掩盖尾部或腾空占用。

**未注册 QML、未替换 Guided、无新部署入口，应用仍单链。** native sticky outbound helper 已定义，但 owned 请求/继续发送、QML actual Episode/epoch、源回调与 selected Run/Series/Close/Session 投影尚待完整接线，不能称新身份边界已在应用全路径生效。限定文本审计结果已记录；未编译、测试、运行身份/业务/API/helper/应用/设备，未更新 PR 或打包。

## 335–339：UI不成为身份或协议完成证明

[监视面板](../../custom/src/Swarm/FleetMissionObservationPanel.md)仅调用原有request/approve/cancel；角色、两真实账号/WTS及专用purpose仍由实际native Close/Session核验，UI不暴露或填写SID/密码，不借upload审批。每入口只持有controller+review完整token，三勾选不是认证，另一个入口也不能接管当前审阅。

原生modal期间公开busy可滞后，UI实际callTicket锁存至调用返回；不能用定时显示判断账号已验证或允许重发。native仍重核实际scope/身份/原期限，UI倒计时不续期；未知/已知仍占用/已释放分开，ACK/mode/reached/landed不提升为身份、远端控制交接或任务Completed。仅源码/限定静态审阅，未执行认证、应用/协议、编译/测试或设备；335–339新增UI覆盖330–334无UI的历史状态。

## 330–334：监视结束采用独立账号用途

[新Close](../../custom/src/Swarm/FleetMissionObservationClose.md)使用精确role动作 `task.observation.close`，系统审批用途 `task.observation.close.approve`；旧上传/启动/ground审批不可交叉消费。实际主账号/WTS/SAM角色与不同SID审批账号绑定完整reviewUUID/epoch/generation、context18及identity/role，原30秒包含提示全过程，三项确认均必需。两个SID并不证明两名自然人或企业认证。

Session private即时消费烧掉待审批收据，不存新grant/proof/lease；passive friend checker不递归OS/Session，不发同步changed，burn后仅原生owner/context/role核对后唯一monitor退休。旧MAVLink ACK、mode、reached、landed或本地source状态都不因此变成机载身份/完成认证。

实际map占用与safety revocation分开；释放范围只含本Run/旧startup UUID，异常/缺主源保持unknown，未核实不称已释放。此动作不发STOP/LAND/DISARM、不重放/恢复旧授权，不认证任务成功。仅源码/限定静态审阅，未实际执行系统认证、协议、测试或设备操作；真实账号撤销/同步销毁/时序行为待工程师验证。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](../../custom/src/Swarm/SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

320–324（2026-10-15）新增[显式 HOME 观测过渡](../../custom/src/Swarm/FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

315–319（2026-10-15）新增[本次启动的连续运行观测](../../custom/src/Swarm/FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

310–314（2026-10-15）新增[错误可区分的原生目录清点](../../custom/src/Swarm/FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

## 305–309 历史页不是线上事务身份

新增[Telemetry只读历史](../../custom/src/Swarm/FleetTelemetryArchive.md)的catalogId、native nonce、header/file hash与cursor只绑定本次地面站文件观察；原sys/component/sourceEpoch/RX序号不认证硬件或关联线上ACK。历史Frame/Foooter校验不执行parser、重建控制消息、恢复账号或发布live状态，页successful只表示本次有界读取格式/扫描镜像一致。

无Footer/partialTail、Footer clean=false以及只在runtime的末端flush事实分开，不能从磁盘candidate推演任务启动/完成、关闭证明或StartPermit。public固定row无原包，native detail需当前查询/GUI确认；失败/取消不推进未交付cursor，已交付不可撤回仍等actualfinished。旧ACK/启动门禁与25目标其余来源缺口不变，未运行测试或设备。

305–309 新增[遥测只读历史检索](../../custom/src/Swarm/FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

## 300–304 RX记录身份不构成ACK或执行证明

新增[被动遥测记录器](../../custom/src/Swarm/FleetTelemetryArchive.md)只绑定实际注册Vehicle/PRIMARY/observer与本地UUID/单调序号，不发送命令、获取账号、恢复审批或回灌任务。parser-accepted、message checksum及signaturePresent均非CRC/验签/硬件身份；本地rxSequence不能建立线上request token或准确无线loss，record/flush/窗口闭合也不能证明某次飞行命令已执行。

不改变旧ACK隔离、Mission scoped RAW读或StartPermit门禁；raw记录不作为实时权限/健康覆盖或任务完成输入。来源退休先清绑定，历史仅观察陈述；完整执行归属和协议端身份仍为原缺口。只有静态阅读，未调用解析器/账户/设备、编译/测试或放行。

300–304 新增[原生入站遥测记录器](../../custom/src/Swarm/FleetTelemetryArchive.md)：独立Vehicle pre-plugin被动入口、实际PRIMARY/Vehicle/observer绑定与退休、27消息固定Codec、parentless有界NDJSON worker及native确认/公开只读值已接线。新增默认关闭，不改旧.tlog，不请求数据流或控制；定位类另明确确认，原len payload与RX序号只观察，签名/CRC/身份不认证。文件NewOnly不覆盖或删除，入队/write/flush/durable分层，源失效停止整组并保留unknown尾部；fullTelemetryCaptured始终false，不把此子集当完整遥测或任务许可。运行/编译/测试/解析helper/设备均未执行，未改PX4/刷机/更新PR/打包；原25目标/六类不缩减。下方295–299及更早为历史批次。

## 295–299 历史目录/游标不是操作授权

native catalogId只是本次读取作用域，actual文件/DBepoch/sealRun/filter/nextRowId只约束只读浏览，不能作为账号租约、MAVLink ACK、执行进度或完成来源。封存操作clean不抹旧run unclean/drop，hash仅投影/文件一致性，目录全集/全row验证/恶意并发安全未认证。Reader与live采集独立，结果不回灌rules/Replay、任务、审批或authority；全部权限/飞行释放/完成标记false。实际QQ/query/kind、取消保槽及有限每段cursor防迟到/错域显示，不恢复旧操作批准；完整六类/25目标继续，未运行库/API/设备。

295–299 新增[跨封存段只读浏览](../../custom/src/Swarm/FleetPersistentEventStore.md)：Sql唯一schema/身份/严格页抽取、parentless单请求Reader与Store native目录/分页/取消及public值facade已接。actual目录条目生成本catalogId，页域绑定文件/DBepoch/sealRun/filter，最多128个每段单filter游标，只0或实际next；当前页/Reader共单槽，取消与错误不提前复用，历史不回灌rules/Replay/任务/权限。目录128段/4096条目与JSON页有界，合法1MiB政策的内部预留预算已修；目录全集、全库记录hash、跨段原子快照/飞行序都不宣称已验证。仍缺全集索引/自动归档清理、完整遥测/运营集成及最终StartPermit等原六类/25目标；未编译/测试、运行SQL/目录或hash helper、改飞控/刷机、更新PR或打包。下方290–294及更早说明为历史批次。

## 290–294 维护续建不恢复控制或来源认证

native显式维护consent只用于记录库排空/封存/建立全新run，不创建机载启动、操作员租约或飞行命令。actual sealed/clean/finished/DBepoch/固定basename与同实际源QQ/epoch核对是本地生命周期门禁，hash/QLockFile不认证飞机或恶意文件writer；原始出版顺序领先本消费者时拒绝轮换，binding nonce拒绝旧destroyed回调影响新源。窗口known/unknown和缺口不可当完整遥测，cancel/退出不抹可能已发生的封存。sealError/resumeError分开保存，历史与维护结果不进入live任务/审批/authority。完整25目标/六类和最终StartPermit/执行完成仍未完成，未运行接口/文件轮换/锁/SQL/设备。

290–294 新增[显式维护封存轮换](../../custom/src/Swarm/FleetPersistentEventStore.md)：实际worker协作锁贯穿排空/SQL关闭与同目录旧库保留，native确认后才请求，不自动删旧段；actual clean/sealed/finished、精确DBepoch/文件名和同源QQ/epoch核对后才沿本次明确配置建立新run。取消/退出/源丢失只退休续建，不抹掉可能已发生的封存事实；暂停序号窗口显式known/unknown及缺口摘要，不称连续无损。封存段数有界、未知文件不覆盖，hash/协作锁不认证或保证恶意并发/失电安全。仍缺跨段查询/自动归档清理、全遥测/运营集成及最终StartPermit等原六类/25完整目标；未编译/测试/运行库或锁/文件轮换、执行SQL、改飞控/刷机、更新PR或打包。下方285–289及更早记录为历史批次。

## 285–289 归档不是ACK或控制权恢复

actual BlackBox的epoch/sequence/在栈同类型receipt只证明本对象此次诊断接纳，不认证飞机、遥测、操作者或签名。Codec存固定诊断摘要及未验证hash引用；历史summary.queued/uncertain可以表示过去记录，但包装flightCommandReleased=false只声明归档不发送命令，不能代替MAVLink ACK、真正起飞或完成归属。历史页不进审批、authority、任务、实时rules或Replay，无Vehicle/MissionManager调用；native consent不是账号鉴权。clean/commit/durable仅存储事实，六类实质开发和最终StartPermit/执行完成继续；未运行任何API、数据库或设备。

285–289 新增[默认关闭的连续诊断归档](../../custom/src/Swarm/FleetPersistentEventStore.md)：实际BlackBox暂时收据/同类型事件身份、固定摘要Codec、专属QtSql线程与Store门面已接pri/type/controller；仅native显式配置/确认后异步开库，默认不读取或写盘，不回录开库前历史。queue+in-flight/批次/主库页/历史结果有界，实际commit后才durable，来源/投影/入队缺口保留诊断；历史单槽直到GUI交付，不进入实时回放、规则、机队、审批或控制链。无原payload/凭据/精确GPS，哈希非签名/匿名化；clean非完整性认证，未知库与需hot-journal恢复库拒绝不重建。仍缺全遥测/长期轮换/运营集成及最终StartPermit等原六类/25完整目标；未编译/运行/测试、建库/SQL执行、改飞控、刷机、更新PR或打包。下方280–284及更早批次为历史记录。

280–284 v2观察政策不改变ACK/身份语义：mode仍仅实际DO_SET_MODE普通纯切换包，Seq0/ARM/组屏障/partial queue/排空事实原样。native ODOM周期gap由实际owner冻结政策决定，不靠caller flag/相同payload或最后fresh包认证连续性；样本时间不拿来生成收到包或补过去缺口。heading累计来自完整实际journal的cdeg观察，不是角速度、实际总转角或无未观察旋转证明。两个uORB来源NED标签/显式reset/estimator8不能证明同origin/sample或内部HOME有效性；全StartPermit/认证/完成标志仍false，原签名/远端因果/执行归属缺口未删。

280–284 新增[显式局部运动与航向观察政策](../../custom/src/Swarm/FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

275–279身份边界：ODOMETRY native sender/PRIMARY/epoch、原始wire hash和本地referenceRevision只建立地面站接收来源及已观察历史，不认证机载身份、报文签名、请求因果、PX4二进制或local origin。reset_counter最多8位，省略的扩展默认0不能认证“无重置”；sample time与其它流/GCS钟不混比。sticky/raw prepare及READ冻结防本地清row和已观察ABA绕过，仍不证明未观察到的重置或物理连续性。无新请求/ACK/控制许可，原read34与step32及原审计缺口保留；未运行协议/业务/系统/硬件API。

275–279 新增[被动 ODOMETRY 原生账本与契约](../../src/Vehicle/FleetStartupOdometry.md)：原始 MAV2 解码50/reference42，Vehicle full27/reference21；实际 PRIMARY/PX4/版本与飞行来源、接收整栈、prepare前计数和token/epoch防嵌套覆盖。sticky everObservedRawFrame 不把曾断流/无效/被过滤的源当初始缺流；Readiness仅存去age的20字段参考投影，普通收包/时间/运动数字/quality分值不造成身份抖动，reset/frame/状态与已观察ABA撤销旧依据；地面READ冻结该投影，Transition前后重核实际来源。稀疏协方差NaN保留未知，扩展尾零/LOCAL_FRD/时钟不认证参考有效性或同采样。不增加传感器、不请求/启用流、不改原六通道/read34/startup32、不放宽HOME/local严格政策。源码接线与有限静态审阅，不是完整运动/转向/净空/最终StartPermit或执行完成；原25目标/六类继续，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方270–274及更早批次是历史记录。

270–274只有新READ/新已阅/新两账号审批/新Authority/独立new30的原生链可续作；Series历史不再用旧proof、ACK或modeSeen当许可。最新QQ/UUID/epoch/generation/hash与有序全组固定，历史6摘要sourceContextHash对应完整实际原context（仅外层remaining剔除），不是签名、认证或可导入证明；Terminal20保留本轮真实queue/uncertain与风险事实。MODE曾可能queue、active/reservation/drain、来源丢失都不沿更早scope回退；公有Series数据只脱敏状态，实际对象仅private backend。未调用身份/业务接口，不声明飞行因果/硬件/净空/完成。

270–274 新增[有界实际启动事务序列](../../custom/src/Swarm/FleetMissionStartupSeries.md)及多轮地面再审后端：最新已初始化owner不回退，逐轮冻结actual QQ/UUID/epoch/generation/hash，旧转交链精确退休后重新完整读取/已阅/两账号审批；每次新消费者保留前代，初始/终态对象不能重用覆盖。native固定历史摘要6防递归复制，完整当轮任务/新风险/真实queued事实仍保留；同一原组/记录/部署，最多8个ground child（空候选也计费），源丢失/容量满/MODE已可能排队/未排空均不淘汰或恢复授权。实际pri/type/controller已接，限定源码复核完成，未编译/运行/测试/API/飞控改动/刷机/PR或打包。容量/新部署重置、HOME有效分支/local frame/运动转向、净空/最终StartPermit/时序/执行完成及原25目标/六类仍待开发。下方265–269及更早批次是历史记录。

265–269用途隔离：独立GroundStartupTransaction proof30保留原28并添加实际transactionEpoch/一次transactionAuthenticationId，purpose为ground-startup-transaction-authentication-not-permit，旧24不覆盖。实际pending27与Txn纯回调只绑定owner/source，原两身份与SID/role/WTS/期限仍在Session独立重验；Session6真实remaining→Authority10同UUID移交→事务每包full fence。跳ARM是实际已审阅的armed状态，不是ACK，也不将ARM queued/committed置true；mode-only新RAW不证明任务归属/执行完成。没有调用身份/业务接口或发包，源码不认证远端因果/净空/硬件安全。

265–269 新增[地面再审后的分阶段启动事务](../../custom/src/Swarm/FleetMissionStartupTransaction.md)源码：actual8/context27、一次28→独立30（Session6/Authority10）、同UUID连续占用；独立第二实例不覆盖旧终态。已解锁成员不发ARM，但仍核实际Seq0完整journal与整组屏障；未解锁成员仅普通ARM(param2=0)，之后仅mode-only。首次Seq0前要求实际周期RAW基线/政策gap，错误实例在换generation前拒绝；后端接线且限定源码契约复核完成，无新执行按钮。本机未调用接口/编译/运行/测试、改飞控、刷机或更新PR/包。最终StartPermit/净空/完成、第二事务再次终止后的实际owner重审路线、HOME有效分支/local frame/运动转向及原25目标/六类仍待开发；旧第一终态不能代替第二事务事实。下方260–264及更早批次为历史记录。

260–264 actual GroundAuthority移交：旧22一次退休换独立28，原purpose改为ground-startup-authentication-not-permit，新增startupId/authorityEpoch/sourceHash/authorityAuthenticationId/transferAt/transferBudget；原两SID/身份窗口/预算只缩短。Session成功6给“核对后剩余”，Ledger10再保守计整个调用，Root不把28历史duration当新剩余，不比较跨域绝对clock。pure4只内存owner/原basis22/新scope/hash，Session查真实身份/WTS，Root前后完整RAW/cache/参数版本/风险；Context27无可执行证明/public不泄露SID。它不发送MAVLink，不认证远端因果/硬件/参考有效分支，不授最终飞行许可。

260–264 新增[地面再审启动依据一次接管](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)源码：实际10来源/原始context27、完整新RAW/全风险/记录/typed当前地面状态进入新owner，旧22一次移交Session独立28（6字段返回真实remaining）、Ledger10字段接管。旧账本退休不盲撤新owner，原最早READ START/两身份期限只缩短，独立实时查身份/WTS/源与整组占用；public不泄露SID/RAW，所有执行/启动/飞行许可false。实际后端/pri/type已接，本批限定源码契约静态收口已完成，无读取/ARM/MODE或UI执行按钮。真正分阶段模式消费者、HOME分支/local frame/运动转向、净空/最终StartPermit/完成及其它六类/原25项目标继续；未编译/运行/测试、调用接口、改飞控、更新PR/源码包/安装包。下方255–259及更早批次为历史记录。

255–259 新 ground条件意图用途隔离：角色action为现有mission.start，实际认证action为mission.start.ground.reassess.approve。新22 proof由实际强GroundStartApproval/approvalEpoch/reviewId/epoch/bindingHash和一次UUID绑定，与旧16 Start/22 Authority/24 Transaction分仓；不能交叉消费、导入恢复或拿public27 receipt当证明。私有14 grant包含真实proof并每次重新核当前源，不发MAVLink命令，不证明无线请求因果或远端身份。pure owner callback只做本次内存消费范围，OS/SAM/WTS和全风险另在实际Session/Ledger前后核对；原预算先固定，仅缩短，撤销精确实际owner/ID/epoch。

255–259 新增[地面再审独立账号条件意图](../../custom/src/Swarm/FleetMissionGroundStartApproval.md)源码：实际新 Review20/九对象/冻结整组与 armed 状态、完整最新记录/风险/新 RAW，真实两账号新提示和 Session 独立22证明，一次消费/撤销及 native14 grant；public27 receipt 无 SID/RAW。最早 READ START 原30秒不续，旧证明不恢复，实际提示期间主身份仍查 SID/role/WTS/旧期限；active READ与消费不放宽。实际 backend/pri/内部类型已接，本批已完成限定源码契约静态收口，没有新增执行按钮/新 startup authority/MODE续作。最终许可/净空/HOME有效分支/local运动转向/完成及其它六类/原25目标继续；未编译/运行/测试或调用接口、改飞控、更新PR/源码包/安装包。下方250–254及更早批次为历史记录。

250–254：新地面再审只读 factory/实际强 Review/固定 scope/armed-POSCTL admission/PRIMARY 与 version/flight epoch，新收据保存原 read-start、原计数和全部 INT/end ACK，旧 disarmed factory 与旧 SourceMatches 不接纳新例外。成功 factory 9、RAW 31+3=34、终态原引用 20、新风险 native context 20；所有旧授权恢复/启动/执行/飞行释放字段 false。READ 操作会改变远端传输状态且逐项读取不是原子快照，计数变新不证明无线请求因果/硬件身份；已阅不是 new authority 或 mode 续作。原生初始全栈静止与本 owner READ 接收栈续包分别核对，纯 owned getter 不反调完整 fence。仅源码文本审阅，未发送 MAVLink、编译或测试。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](../../custom/src/Swarm/FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

审计范围：`src/Vehicle/Vehicle.*`、`src/MissionManager/PlanManager.*`、`src/MissionManager/MissionManager.*` 以及 `custom/src/Swarm/SwarmController.*`。

本报告只做静态审计，不代表已编译、仿真、硬件测试或刷机放行。

244–249新增本地actual owner/startupUUID/stepUUID/operation/PRIMARY/epochs、固定三步packet/发送围栏、RAW ACK/状态与持续风险历史源码，有限源码静态收口完成。没有给无线协议增加nonce；新CURRENT或ARM/mode心跳不认证因果、远端任务/硬件或完成。取消保留partial queue/可能飞行，不回滚/DISARM。HOME/local参考有效性缺口保留，完整再审/执行归属待开发；未发送/运行接口、编译或测试。

239–243增量边界：新增native retained-read校验只核原真实私有scope/op/原始完整证据/实际PRIMARY/source epoch/成功计数/cache/READ START时效，不用原grounded读取admission；原主动读工厂与grounded SourceMatches未放宽，实际启动phase另核。账号接管只有固定owner/UUID/epoch/sourceHash/原TTL，不增加无线operation token或硬件认证。只读源码另确认旧PX4插件先Mission再ARM，配套Commander的MISSION_START也会ARM；DO_SET_MODE没有ARM但需固定main/sub参数，MISSION_SET_CURRENT没有COMMAND_ACK，未来必须独立观察新的seq0、正常ARM和mode-only状态。当前没有实现这些命令发送或实际完成归属，也没有运行任何接口/编译/测试。

234–238原生只读增量：新的启动前整组读取用真实callerUUID scope/本地op/source epochs与mandatory授权fence，读取原始PRIMARY INT报文而非已调整显示项；34字段成功收据保留原始16字段项、真实local endACK queued及原READ START年龄，失败/取消保留旧cache但不复用旧proof。入口接收栈阻止旧报文回调开启新scope，取消后的旧protocol栈在op/TransactionRead/scope核对后才能继续，不能转到legacy无fence ACK。新native scope/计数仍不是无线nonce：顺序读取非远端原子锁、迟到帧不能据此宣称因果或硬件身份认证；完整任务启动/完成归属仍缺独立事务。本批无协议/API调用、编译或测试。

2026-10-14 最新增量：PlanManager 在完成信号前冻结本地 Write ID、内容指纹和 fresh-upload 来源，entry/worker/finish/protocol 栈守卫拒绝重入；已接管 MissionItem 的 QObject 所有权与生命周期检查同步收口。MissionController 的 MERIVUS 恢复通知关联确认时的原 source 内容/manager epoch、实际目标、本次 generate Write ID 和请求，延迟至栈退出后再发，启动前重新检查 ACK/cache 身份。FleetMissionDeployment 另发独立完整 Read 并按 wire 项/哈希比对，但仍只是几何暂存，不允许启动。以下旧行号只作历史定位，当前实现应按符号与实际源码读取；这些本地相关性措施仍未给无线协议增加 token。

## 结论

当前最重要的工程师接手风险是：底层 `COMMAND_ACK` 和 Mission Protocol 消息没有地面站 operation/session token。Swarm 层已有 dispatch、generation 和 postcondition 门禁，能降低大部分误归属，但不能证明一个迟到的 MAVLink ACK 一定属于当前操作。

### 1. `Vehicle::mavCommandResult` 缺少操作身份（高风险）

- `src/Vehicle/Vehicle.h:1022` 的信号只有 `vehicleId、targetComponent、command、ackResult、failureCode`，没有 request/operation ID。
- `src/Vehicle/Vehicle.cc:3308` 通过 `(message.compid, ack.command)` 查找待处理命令；`3314/3326` 直接取出匹配项。
- 超时/重试失败在 `Vehicle.cc:3183-3190` 移除待处理项并发失败。若旧 ACK 随后到达，而同一机体/组件/命令已经开始新请求，旧 ACK 可能被新请求消费。
- MAVLink `COMMAND_ACK` 不回显地面站 transaction ID；Swarm 的 `dispatchEpoch` 只存在于本地，不能修复底层 ACK 的物理归属问题。

工程师建议：在 Vehicle 层引入每次发送的 operation ID/epoch，并使所有回调携带它；对超时失败的 `(vehicle, component, command)` 设置 ACK quarantine/cooldown，或扩展协议/命令封装使飞控回传可匹配 token。旧 token 的 ACK 必须进入 ignored/audit 轨迹，不能完成新事务。

### 2. MissionManager/PlanManager 已有本地 identity，无线消息仍无本次 request token（中高风险）

- `PlanManager.h` 的旧完成信号仍只有 `bool`，但现在新增 `operationStarted` 和 `*WithOperationId` 兼容信号；内部仍以 `_expectedAck`、`_transactionInProgress` 驱动协议状态。
- `PlanManager.cc:169` 的超时逻辑会重试或结束；`271` 仅按期望 ACK 类型判断；`555-631` 收到匹配 `MISSION_ACK` 即推进/结束当前事务。
- `MISSION_COUNT/ITEM/REQUEST/ACK` 只按 `mission_type`、序号和当前 ACK 阶段筛选，没有本次 read/write/removeAll 的 generation。上一事务的迟到消息可能在下一事务恰好处于相同期望阶段时被消费。
- 旧的 `sendComplete(bool)`、`removeAllComplete(bool)` 仍没有 request identity；新增信号可以让新调用方确认地面站本地操作归属，但不能确认无线消息的物理归属。
- 历史审计时 PlanManager handlers 缺少独立来源校验；当前 handlers 已共用来源/目标与 mission type 检查，工程师仍需验证多组件、异常 target 和陈旧消息路径。来源校验不能让迟到的同来源消息携带本次 request token。

工程师建议：继续用当前本地 operationId 关联 UI/Swarm 回调，同时为 Mission 消息增加协议侧 generation/token 或严格的超时消息 quarantine，使旧 generation 消息进入 ignored/audit 轨迹；至少保持 `message.sysid`、`message.compid` 校验，并验证 mission type、序号、方向和阶段。

### 3. 当前 Swarm 缓解措施的边界

`custom/src/Swarm/SwarmController.cc:1859,3057,3495` 已明确记录底层无 token，并通过临时 Mission 状态、dispatchStarted/epoch、formation generation-bound connection、Land/RTL owner 和 Mission list postcondition 做本地防护。这些措施是有价值的，但不能替代 Vehicle/PlanManager 的协议级 operation identity。

### 4. 本轮地面站补强后的边界

- `PlanManager` 现在为每次 read/write/remove-all（含 guided 和 resume upload）生成单调的本地 `quint64`，通过 `operationStarted`、`*WithOperationId` 完成信号和 `errorWithOperationId` 暴露；旧的 bool/无参数信号保留，避免破坏现有控制器和测试接口。
- `SwarmController` 临时 Mission 状态保存 upload/clear operationId，并只接受与当前状态相符的完成回调；机载 Mission 列表的后置校验仍保留。
- `PlanMasterController` 的 Mission 读写串联保存自己发起的 operationId，忽略另一个控制器/调用方的带 identity 完成回调；没有主动发起 operation 时仍保留初始被动 Mission 加载的兼容行为。
- 该 operationId 只在地面站进程内关联回调和日志，不会进入 MAVLink Mission/COMMAND_ACK，因此不能把迟到旧无线消息变成可证明的协议 token。Vehicle 的 quarantine 同样只是兼容性降低风险措施。
- 本轮进一步把 `PlanMasterController` 的 `operationStarted` 预锁存、GeoFence/Rally 带身份回调、Fact 参数发送总闸和 Vehicle 初始 Mission/GeoFence/Rally 读操作身份接入静态边界；这能阻止旧地面站回调和不安全入口继续推进状态，但仍不能替代 MAVLink 线上 token。
- ACK quarantine 现在不再豁免 `MAV_CMD_DO_SET_MODE` 与 `MAV_CMD_COMPONENT_ARM_DISARM`；模式/解锁相关的迟到 ACK 仍需工程师用 Mock/SITL 和真实链路验证，不得把静态隔离描述成成功证明。

因此原结论仍然成立：工程师必须在运行时验证迟到旧消息、并发 MissionController 和 read/write/removeAll 交叉时序，不能把本地 operationId 单独当作飞控已接受或物理归属证明。

## 工程师验收场景

1. 对同一机体、同一组件、同一 `MAV_CMD`：让第一次请求超时，随后启动第二次请求，再注入第一次的迟到 ACK；第二次不得因旧 ACK 成功。
2. 让 Mission `MISSION_COUNT`、`MISSION_ITEM`、`MISSION_REQUEST`、`MISSION_ACK` 在 read/write/removeAll 超时后跨事务迟到；旧消息不得完成新事务。
3. 发送来自非 autopilot component 的 Mission 消息，必须被忽略并留下可审计记录。
4. 验证重试、断链恢复、多机同命令、`MAV_RESULT_IN_PROGRESS` 和取消/替换任务路径。

验收标准：每个 ACK/完成回调均能关联唯一 operationId；无法关联时必须 fail-closed，不能静默视为成功。

## 本轮异步状态边界补充

- `PlanManager` 现在在任务 read/write/remove-all、请求/重试、Mission item 回传和 clear-all 发送前重查 live transport；普通写入/清空在 armed/flying 或断链后以失败结束，避免 timer retry 继续发旧任务。该检查仍不能把 MAVLink ACK 变成协议级 token。
- `MissionManager` 的 resume 上传复用 grounded write gate，ArduPilot guided item 保留显式 `AckGuidedItem` 飞行态例外；工程师必须验证两者不会互相误用。
- `Vehicle` 的 C++ transport/maintenance gate 和 Guided 输入校验覆盖维护命令及常见 Guided/模式入口；这只是发送前拒绝，不代表飞控接受、执行或安全完成。
- 参数写入 timeout retry 现在复用最终 Fact gate；旧 timer 不得在 armed/flying/断链状态下发 PARAM_SET。
- Swarm 临时 Mission 在延迟 dispatch 后、实际 `writeMissionItems()` 前重新验证目标健康、链路、飞行状态和冻结坐标；仍须用延迟 ACK/状态变化场景验证事务归属。
- 云台/相机/吊舱/Joystick/RC 映射与自动 Home 后台入口也不再把 `primaryLink()` 指针存在当作完整 transport 证明；它们在 C++ 入口复核初始连接和通信状态，RC/云台输入还经过范围检查。该防线仍不提供 ACK operation identity。

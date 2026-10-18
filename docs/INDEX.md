# MERIVUS 文档索引

## 546：最终源码交接索引

优先阅读[交付范围与外部未完成项](handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师待验证清单](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本轮操作合同：[规则待审箱](../custom/src/Swarm/FleetRuleReviewPanel.md)、[Run后台检查点](../custom/src/Swarm/FleetRunCheckpointPanel.md)、[用户模板](../custom/src/Swarm/FleetUserTaskTemplatesPanel.md)、[显式容量留存](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)、[四受众审阅报告](../custom/src/Swarm/FleetMissionReviewReportPanel.md)。源码封包不是编译、测试或工程放行；旧历史中的剩余项以最新范围表区分，不再按批次推算进度。


## 545：证据链文件操作

[原生后台合同](../custom/src/Swarm/FleetMissionEvidenceChain.md)、[指挥中心操作](../custom/src/Swarm/FleetEvidenceArchivePanel.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[12项未执行工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。当前资料区分受理、写入、读回及历史验证，不将哈希链称为身份签名或飞行结果。

## 544：公共航线下发链路修复

[编译合同](../custom/src/Swarm/FleetMissionCompiler.md)说明展开条件、原始要求检查和时序继承；[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步记录。只进行源码修改和直接静读，不是构建、运行或飞行结果；无阶段包。

## 543 当前：配置速度明确采纳与逐机路线预算

当前参数观察可提供四项配置速度建议，操作员查看并明确采纳到未保存输入；采用前与保存时都重查原来源。返程 NAV 建议显式沿用巡航值，不是独立RTL参数，不代表真实性能认证。普通路线现在由最终本机路线/profile生成出返航行估值，与驻留分开；只提高既有预算，不能把未知总任务时长补为可执行。

兼容原备用配置读取、载荷字段核对及任务重规划；不增加飞行指令、放宽启动权限或恢复旧批准。10源码＋21文档，共31路径，无新文件。原25目标尚未全部完成；不制作阶段包，继续面向统一源码交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR；不以文件数估算进度与交期。

见[配置操作](../custom/src/Swarm/FleetExecutionProfileEditor.md)、[路线模型](../custom/src/Swarm/FleetMissionPreviewTimeline.md)、[实际预算消费](../custom/src/Swarm/FleetMissionReviewCoordinator.md)。下方542及更早为历史。


## 542 当前：编队等待与重新确认（源码接线，工程验证未执行）

新增本次会话的编队待办：明确保存固定成员 → FIFO 等待实际占用释放 → 操作员重新审阅 → 原确认框与既有派发入口。资源就绪不自动派发；取消待办不停止飞机，已提交请求不等于飞控接受或任务完成。最多32条（含终态），待办30分钟、单次审阅60秒，以原生单调时钟判断；重启不恢复队列或批准。

实际 Swarm/Orchestrator 原对象提供占用信息，原生占用未知或未解决时不能进入复核；确认消费前重查同一来源、版本、FIFO 与占用。新增有界任务占用投影，避免定时复制完整 Run 观察；原协议仍仅接受 UAV-1～UAV-6、必须包含 UAV-1，不扩大飞机容量或最终启动许可。

本次12源码/工程登记＋16文档，共28路径，推进目标6/8/13的本地调度操作链；25项目标尚未全量完成。按最新要求不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR；不根据路径数推算完成率或交期。

见[队列原生合同](../custom/src/Swarm/FleetFormationQueue.md)、[实际确认与派发接线](../custom/src/Swarm/FleetFormationQueueWorkflow.md)、[操作界面](../custom/src/Swarm/FleetFormationQueuePanel.md)。下方541及更早为历史。


## 541 当前：任务模式退出证据贯通（源码实现，工程验证未执行）

原生提交后的任务模式退出现在保留计数、首/末 HEARTBEAT 原模式与状态、前后接收引用；已置异常位不会吞掉后续已提交快照的变化。完整/保护报告、交接简报和归档历史同步版本化，旧资料显示“未采集”，不补成零次。检查点之间多个退出可合并，不宣称完整逐边沿日志。

18源码/工程登记＋23文档，共41路径，新增共享校验器 h/cc/md。只推进运行结果、交接和审计链，不改变终点候选、不发飞行命令、不恢复控制权。原25目标与实质剩余保持；不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR，不据路径数推算完成百分比或交期。

见[退出证据合同](../custom/src/Swarm/FleetMissionModeExitObservation.md)、[报告合同](../custom/src/Swarm/FleetMissionExecutionReport.md)、[审计版本](../custom/src/Swarm/FleetRunObservationEvent.md)。下方540及更早为历史。

## 540 当前：完整计划驱动的逐机时间预演（源码实现，工程验证未执行）

预演不再仅接任务摘要：操作员明确捕获当前审阅记录，同一记录的完整计划与能量预算一起进入逐机路线计算。出程、独立返程及显式驻留分别计算；缺速度、垂直率或坐标基准时保留未知，不均摊全队航线、不借新电量补旧计划。原生结果保有界事件集合，现有界面四条分页；自动概览刷新不覆盖正在浏览的冻结计划。

相对时间以各机首规划点为零，不代表跨机同步；起降、转弯、风及实际飞控动态未由此认证。此项推进目标16/17，但不替代最终StartPermit、正式机载结果或完整数字孪生。原25目标不缩减；不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR。

见[计算合同](../custom/src/Swarm/FleetMissionPreviewTimeline.md)、[实际界面与来源](../custom/src/Swarm/FleetDigitalTwinPanel.md)。下方539及更早为历史；不按批次或路径数量推算完成百分比和交期。

## 539 当前：交接历史保存与恢复（源码实现，工程验证未执行）

原运行简报交接增加后台文件保存、独立加载预览、四条分页与详情，以及明确确认后的仅追加历史合并；不开放旧整库替换入口。同ID内容冲突整笔拒绝，已保存事实与当前工作资格分开。详细流程见[交接工作流](../custom/src/Swarm/FleetMissionHandoffWorkflow.md)、[原生合同](../custom/src/Swarm/MissionHandoffManager.md)和[界面合同](../custom/src/Swarm/FleetHandoffPanel.md)。

原25目标保持，本次推进目标15的本地资料交接，不代表跨站控制权交接或全部目标完成。无阶段源码包；完成约定源码后统一交付。未编译、运行、测试、仿真、操作硬件、刷机、修改PX4、提交或更新PR；不以文件数推算完成百分比或交期。下方538及更早为历史。

## 538 当前：持久化任务索引（源码接线，工程验证未执行）

建立固定派生索引，保存分窗扫描进度；重开核原来源后继续，任务分页与原记录下钻保持有界。不是提高旧目录上限或只新增建议界面。详细实现与范围集中见[索引合同](../custom/src/Swarm/FleetArchiveTaskIndex.md)、[界面合同](../custom/src/Swarm/FleetArchiveTaskIndexPanel.md)及[修改清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

原25目标与六类剩余保持。先前“约40%／60%”没有逐项验收权重，仅属历史粗估，不再作为当前完成度或交期依据。按用户最新要求，不制作阶段源码包；完成全部约定源码后统一交付。当前仍未全量完成，未编译、测试、仿真、运行项目、刷机、修改PX4、提交或更新PR；实际工程验证仍由工程师承担。下方537及更早为历史。


## 537 当前：逐 NAV 航点相对驻留（源码实现，工程验证未执行）

沿真实辅助路线点编辑器增加可选holdSeconds：独立出返程点可新增、编辑、取消或清除驻留；缺失与明确0分别保留，原秒数贯穿草稿、计划和来源身份。复用原点编辑的版本／来源保护，不把驻留解释为绝对到达时刻，也不扩成自动覆盖、起降或HOLD动作设置。

新增共享DwellEncoding，统一拒绝正请求在Float32或所述PX4微秒转换中变成零等待。上传与规范回读仍沿原param1规则，原请求不被量化回写。能量／时间审阅改用原请求、上传值及模型等待量的保守预算；仅确有下舍才上取，精确1秒和1+1不无故增加ULP。三个严格几何消费者与Joint使用相同接受域，明确驻留即使没有executionProfile也进入原实时版本／当前工程声明门。

模型仅对应已读本地旋翼NAV分支，计时从首次位置接受开始，yaw等待可能消耗它；不是精准悬停、持续留在半径内、真实调度上界或实际固件认证。没有新增payload/schema/权限字段，最终StartPermit、actualPath和正式结果仍未完成。

本批12源码／工程登记＋21文档=33路径，新增DwellEncoding.h／cc／md三条；40项工程场景全部未执行。限定直接静读修正了新点错误消息的局部变量重名问题，并交叉核对实际消费者与数值模型。UI/UX仅指导原Qt表单的独立勾选、标签、错误及取消恢复，没有运行技能脚本、界面预览或任何项目逻辑。

原25目标与六类实质剩余不缩减，约40%源码／60%剩余仅粗估。未编译、运行helper／算法／parser／QML／hash、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[驻留数值合同](../custom/src/Swarm/FleetMissionDwellEncoding.md)、[实际点编辑](../custom/src/Swarm/FleetMissionDraft.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方536及更早为历史。


## 536 当前：逐 NAV 航点机头方向（源码实现，工程验证未执行）

真实辅助路线编辑入口、Draft、Compiler、载荷及上传／回读消费者接入可选逐点 heading。规划界面支持 relay／responder 独立出返程点的新增、整点编辑、取消和删除；区域自动覆盖点仍沿飞控策略，不把扫描线方向、起降或 HOLD 请求当逐点机头朝向。缺省与显式0°区分，原连续请求保留在草稿／计划身份中。

上传采用声明的最近1/256°网格，最大圆周量化差1/512°；原请求、上传tick、实际回读原值及比较用规范值分别保留。实际有限Float32回读只有在明确推导界内才归同一tick；非法大有限角度也保原数诊断，不补0。编码步长及语义匹配不是实飞精度、逐位相同、真实固件认证或达向／转向动力学证明。

无heading单机保持原v2；明确heading使用独立v3语义域，混合机组逐成员区分。原严格几何／审阅消费者使用同一编码规则；实际目录声明在审阅与启动前置首尾核对应域，旧声明不自动升级。回读报告新schema2在匹配时也显示原值，最多64条并明确截断。最终StartPermit、actualPath及正式执行结果仍未完成。

本批28源码／工程登记＋32文档=60路径，新增HeadingEncoding.h／cc／md三条；48项工程场景全部未执行。限定源码交叉静读与普通文本／路径／Markdown／空白核对不等于构建或运行验证。UI/UX仅静态指导原Qt表单的独立勾选、字段错误、键盘／换源恢复与原值显示；未运行技能脚本或界面预览。

原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅粗估，不按文件／批次上调。未编译、执行项目／parser／helper／QML／算法／hash、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[朝向编码合同](../custom/src/Swarm/FleetMissionHeadingEncoding.md)、[真实编辑入口](../custom/src/Swarm/FleetMissionDraft.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方535及更早为历史。


## 535 当前：异机完整包络冲突检查（源码静态收口）

沿任务执行与安全主线新增异机声明包络比较：同次启动整域和 NAV／起降参考体的完整原生组件进入全异机组合，保机体、支撑及误差余量；不从报告重建名义线段或只取前缀。闭边界接触也属于交叠候选，地形接触 pad 不豁免另一架飞机，无可靠同步时序按全时段空间保守比较。

完整双方平面进入整数几何求解；新比较共用有界预算与原取消信号。缺失完整几何、求解未知或超预算不算通过。Worker 结果升级 exact8，普通／地面审阅、Witness 和两 Authority 接入同份报告及共同结论，不是只新增显示字段。

旧 minimumSeparation 检查不被替换；外包交叠不是实测碰撞，无交叠也不是实际路径认证。原 `actualPathCertified` 与最终 StartPermit 缺口保留，未新增飞行命令。原25目标／六类实质剩余继续，约40%源码／60%剩余仍仅粗估，不按本批路径数上调。

本批15源码／工程登记＋21文档=36路径，新增纯值模块h／cc／md三条；源码实现及限定静态交叉审阅已收口，48项工程场景全部未执行。仅源码、直接静读及普通文本／路径／Markdown／空白核对；未编译、运行项目／算法／parser／helper、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[异机包络合同](../custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.md)、[后台与准入](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方534及更早为历史。

## 534 当前：参数导入原生发送与固定重试（源码静态收口）

继续533实际导入入口，将私有typed选中值交给PM独立原生事务；接纳后不再经Fact通用setter乐观改当前值。事务固定原飞机／PRIMARY／参数值，独立有界调度与重试；不借旧同名等待队列或当前Fact值重发。正常关闭预览不取消已接纳事务，参数页增加原事务状态与准确ID停止剩余发送入口。

新Vehicle私有桥在插件调整后及writer入口前核原来源与固定payload，先记实际writer入口事实再调用；APM按真实组件保numeric-float转换，不可精确表示的整数整批拒绝。最早RAW接收冻结当时已有事务的票据，后来的新事务不能领取旧包；同值报告仅称匹配，不是因果ACK、配置认证或飞行许可。CMake同步实际编入APM与共享特性开关。

100ms独立调度，每轮最多扫描256行／尝试64行，每行至少3秒、首次后最多5次重试，整次120秒在发送与RAW取票／消费两端复核。UDP／Mock原Link可在工作线程；取消期间的旧终态不被尾部逻辑覆盖，页面销毁后不再续读旧对象。

本批11源码／工程登记＋16文档=27路径，新合同2份；源码实现及限定静态交叉审阅已完成，64项工程场景全部未执行。UI/UX仅静态指导原Qt控件的状态、准确ID取消、键盘焦点及窄窗换行，没有执行技能脚本或预览。未编译、运行项目／parser／QML／业务／定时器、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%源码／60%剩余仍仅粗估。

[原生事务](../src/FactSystem/ParameterImportTransaction.md)、[发送桥](../src/Vehicle/ParameterImportTransport.md)、[界面与接纳](../src/QmlControls/ParameterEditorController.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方533及更早为历史。


## 533 当前：参数文件完整预览与精确提交关联（源码静态收口）

真实产品入口是ParameterEditor载入文件和ParameterDiffDialog人工确认。旧预览会跳过坏行、未核完整字段和类型，并借用live metadata生成临时Fact；未知枚举及重启标志可能影响当前参数定义。本批复用现完整typed文件校验，建立独立预览及私有候选，并用原预览token关联提交、选择和关闭。旧页面不显示或改动后来模型，载入失败显示明确原因。

文件可表达十种数值类型，但现PARAM_SET发送器只正确支持七种；选中的64位整数／Double必须在第一项提交前明确拒绝。逐项写入前后及最终通知尾复核原对象、值和当前写入条件；中断保留已尝试项可能提交的事实，不宣称批量回滚或机载ACK。同一行旧sender内部的最终来源连续性仍需专用提交桥，不能用caller前后检查代替。

本批4源码／QML＋13文档=17路径，新增1份合同，48项工程场景全部未执行。源码实现及限定静态交叉审阅已完成；未编译、运行QML／parser／参数／业务IO、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。UI/UX技能仅静态指导原Qt控件的错误反馈、状态与恢复路径，没有执行技能脚本或预览。原25目标与六类实质剩余保持，约40%源码／60%剩余仅为粗估。

[实际导入合同](../src/QmlControls/ParameterEditorController.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方532及更早为历史。


## 532 当前：多机元数据后台调度（源码静态收口）

继续实际多机连接链：Artifact校验、Summary解析与Translate翻译接入同一进程FIFO预算，最多2项运行／32项排队，排队120秒并在获槽前复核期限。队列满、未启动取消／超期与运行失败分别处理；原取消后仍start的两个清理旁路移除，避免突破运行上限。

运行项保留原输入所有权及result／actual finished双尾，取消不提前归还运行槽。未启动失败不作为缓存损坏、CRC失败或重下载依据。这个预算只涵盖上述两类worker，不涵盖HTTP／FTP、所有Qt线程、旧显式同步翻译或整进程内存／CPU。

本批10源码／工程登记＋17文档=27路径，新增3条；40项工程场景全部未执行。源码实现及限定静态交叉审阅已完成。未编译、运行项目／线程／parser／helper／下载／业务IO、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍是粗估。

[调度合同](../src/Vehicle/ComponentMetadataWorkScheduler.md)、[实际元数据消费](../src/Vehicle/ComponentInformationManager.md)、[实际翻译消费](../src/Vehicle/ComponentInformationTranslation.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方531及更早为历史。


## 531 当前：参数缓存 v3 与真实 HASH 接线（源码静态收口）

替换实际参数缓存的 generic QVariant／QMap反序列化及按声明宽度盲读：新增4MiB／16384行的独立v3纯值格式，明确头、身份声明、十型primitive LE、排序唯一名称和完整EOF；坏文件整份miss。旧.v2保留，不解析、不删除或降级读取。Float／Double保实际IEEE原表示，Fact容器通知尾同步按真实位模式核NaN／Inf／正负零，不把非有限值当健康证据。

实际PX4 hash按本地6C源码核对，只覆盖已使用非volatile参数的名字与Int32／Float原4LE字节，初值0／无末xor；十型文件格式不等于十型PX4 hash。RAW HASH在PM最早可见入口绑定原PRIMARY及UInt32／原count，读取、metadata和发送尾持续核来源与真实参数对象，不借后来链路处理旧包。

CRC与总数都不能证明volatile名称／used-index未变化。零volatile才可按完整候选快载，全部actual Fact／metadata／目录／原来源及值通过后才无外调批量信用索引，再在无pending write及索引状态复核后，走受控完成与原固定HASH ACK桥；含volatile只预填非volatile模型，不清任何真实接收等待、不发HASH ACK，正常机载流／原重试读齐后才完成。自身回放不递归写缓存或逐行提前ready，真实接收／刷新／写入永久退休旧attempt，其独立普通完成不被旧cache卡住。写入改QSaveFile无direct fallback及完整块／commit检查，取消假进度动画；已发生的发布／writer事实不伪造回滚。

本批7源码／工程登记＋15文档=22路径，新增3条；64项工程场景全部未执行。仅源码修改、直接静读与普通文本／路径／Markdown／空白核对；未编译、执行工程／codec／parser／CRC／helper／参数／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标与六类实质开发保持，约40%源码／60%剩余仍仅粗估；后台全机队调度、其它factory内部寿命及完整执行／运营仍有源码工作。

[缓存格式](../src/FactSystem/ParameterCacheFile.md)、[实际消费](../src/FactSystem/ParameterManager.md)、[IEEE容器值](../src/FactSystem/Fact.md)、[22路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方530及更早为历史。


## 530 当前：离线参数真实值与整份发布（源码静态收口）

修复真实离线编辑入口：旧代码把文件数值解析到局部变量却从未赋给 Fact，非零参数仍为初始 0；负 Int8 又使用无符号转换。本批新增纯值 codec，完整接纳五列文件和十种数值类型，再建立实际 Fact-owned 元数据与参数值候选，全部通过才发布映射、组件计数、总数、默认组件和 ready 状态，不保坏尾的成功前缀。

输入最多4MiB／65536行／16384参数，行内容4096字节、数字字段128字节；整数先范围后转换，实数有限且拒非零下溢，保浮点负零。实际 Plane 注释含 NUL，先跳过有界原始注释而不误拒内置模板。文件 system ID 仅历史声明，不改离线 Vehicle ID；构造期间不要求尚未安装的 Vehicle.parameterManager 或在线握手。

候选保持原 owner／offline／插件／模型及实际版本／来源，通知和工厂回调后重核；两版本监听就位后也拒改回原值的旧候选，失败只清本批未发布对象。Fact 容器输入另修数值相等却类型不同或零符号不同的跳赋值，仍不发参数写回信号。离线值不成为实机观测、构型认证、性能证据或 StartPermit。

本批7源码／工程登记＋15文档=22路径，新增5条；源码实现及限定静态交叉审阅已收口，48项工程场景全部未执行。仅源码修改、直接静读与普通文本／路径／Markdown／空白核对；未编译、执行项目／parser／helper／参数或文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。二进制参数缓存校验、全机队后台调度仍有源码工作；原25目标及六类实质剩余保持，约40%源码／60%剩余仅粗估，不按批次数上调。

[纯值格式](../src/FactSystem/OfflineParameterFile.md)、[实际加载](../src/FactSystem/ParameterManager.md)、[类型／零值](../src/FactSystem/Fact.md)、[22路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方529及更早为历史。


## 529 当前：后台有界翻译与合法递归（源码静态收口）

沿真实元数据翻译链继续，不关闭本地化功能。新增纯值有界核心及两段后台作业：摘要读取／locale URL选择 → 主线程有界下载 → TS解压／XML／JSON／规则执行／唯一输出。生产入口绑定原生summary／metadata artifacts及独立请求ID，实际worker自己持输入；不是仅把QString传入线程。

原实际fixture包含随list／items下降数据的合法recursive-def，不能按def全局visited拒绝。新核心区分真实物理数据位置和TS显示context，仅拒无进展的活动规则／引用循环，另对JSON全树、XML文本及翻译工作分别计预算。数字list key对应实际PX4枚举／bitmask，修复旧toString变空context；字符串、数组、wildcard、globals及合法DOCTYPE TS保兼容。

协调器以actual worker／阶段／代次及result+actual finished两尾才采用结果；source退休／析构按原ID及两artifact准确取消，后台未结束不销毁线程或提前释放输入。新native完成信号与legacy路径调用者分离，不接管外国输出。同步translateJsonUsingTS仍保借用／qrc输入及原生唯一输出，复用同有界核心。

本批9源码／工程登记＋16文档=25路径，新增6条；源码实现与限定静态交叉审阅已收口，48项工程场景全部未执行，不代表运行或可靠性验收。未编译、执行项目／parser／helper／规则／线程／XML／JSON／下载／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍为粗估，不以路径或批次数上调。

[有界核心](../src/Vehicle/ComponentTranslationEngine.md)、[后台作业](../src/Vehicle/ComponentMetadataTranslationWorker.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。


## 528 当前：下载原件唯一所有权（源码静态收口）

继续真实组件元数据入口，处理同名远端文件共用临时路径的问题。新增原生 fresh 临时文件 owner，HTTP／MFTP 和 TS 下载使用独立 request ID／完成信号；不是下载后再从共享 basename 复制。原 legacy 固件、相机、参数包下载接口及其大文件／本地路径接受域保留。

新传输持 owner 到写句柄关闭，Manager 核实际 provider、原 ID／path／来源／阶段，worker 自己持输入到实际结束；TS 处理栈另持原件，summary／metadata artifact、解压输出和译后 JSON 所有权分开。受理 false／同步终结、准确取消、旧回调和通知中销毁均须限定静读，不借共享 cancel 清他人。

新 HTTP／FTP 原件上限32MiB并各有120秒总期，检查完整读写／关闭；HTTP 有限重定向、Cached 原任务最多一次 cache-only 回退；FTP新增模16序号回绕、4096缺块预算和零进展包拒绝。CRC／schema 的既有边界保留；唯一文件不认证 TLS、机载身份、PRIMARY wire 来源、真实性能或 StartPermit，亦不保护任意违规 native caller 并发复用同 lease。

本批15源码／工程登记＋19文档=34路径，新路径6条；源码实现与限定静态交叉审阅已收口，64项工程场景全部未执行，不代表可运行版本或完整可靠性验收。未编译、执行项目／parser／helper／CRC／hash／下载／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%源码／60%剩余仅粗估。

[输入所有权](../src/QGCOwnedDownloadFile.md)、[下载接线](../src/Vehicle/ComponentInformationManager.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。


## 527 当前：参数定义整份接纳与实际Fact副本（源码静态收口）

继续原组件元数据的真实消费链，而不是新增只读报告。已核旧CompInfoParam在验证前关闭fallback、逐条发布live map，通用factory错误又返回非空默认对象；坏尾、重复加载或未知类型会留下半批／旧数据。本批已实现参数专用整份校验、候选目录一次切换及明确失败回退，不改原通用FactMetaData factory的其它内部资源接受域。

本地PX4实际jsonout生成version1，虽vendored／上游schema已是不同版本，不能仅按最新schema替本机猜类型／版本。参数专用解析以实际七数型为依据，明确v1严格数字String兼容、最终min／max／default／increment、完整values与bitmask及安全移位；未知扩展保容忍但计资源预算，不冒称全draft-schema或设备真实性认证。

真正的三个Fact创建入口改为actual Fact-owned元数据副本，避免目录替换时删除被Fact借用的模板；缓存hash只查询已知volatile值，未知／类型冲突放弃快载而正常读参数。已存在Fact保自己的旧副本，不称热更新或当前构型重新认证；CRC副本／字段本身不证明构型、性能或执行许可。

Manager在准确旧source退休及新GENERAL前清实际各组件参数目录资格。独立候选／旧目录的回收、XML fallback所有权、实际source／代次与QObject通知重入已完成限定静态交叉审阅；不要求initialConnectComplete，不复活旧批准或新增参数写入／飞行命令。

CRC先冻结原来源再跨观察失效通知／provider创建，防旧HASH重基新源；不用会被正常参数就绪推进的观察epoch，另冻本地目录revision。原HASH ACK改走私有固定消息门，连接虚调及插件调整后再核来源／原payload，到write入口前最后复核；进入原writer是已调用事实，不冒称驱动完成、字节已发送或远端接纳。其它generic sender不改，也不增加控制协议。

本批12源码／工程登记＋13文档=25路径，新路径4条；58项工程师场景全部未执行。未编译、运行项目／解析／helper／CRC／hash／下载／缓存／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标和六类实质剩余保持，约40%源码／60%剩余仅工作量粗估。

[参数解析](../src/Vehicle/ComponentParameterMetadata.md)、[实际消费者](../src/Vehicle/CompInfoParam.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 526 当前：组件元数据字节校验与后台快照（源码静态收口）

回到能力证据的实际接入链：原组件Manager下载和缓存命中只把声明CRC作缓存标签，不比较实际文件；本批已写入统一后台原始字节核对及独立JSON快照，最终仍供原general／参数／事件／执行器消费者使用。本批源码实现及限定静态交叉审阅已收口，不是可运行版本或完整能力认证完成。

新raw-v1_缓存只保原始传输字节，每次命中重新校验，不把旧解压cache当原件；坏命中最多精确退休原tag一次，两文件移除确认成功才重下原URI一次。当前解析只消费本次独立JSON，不消费insert返回的旧路径；原raw／JSON快照与可转交缓存候选明确分开。

CRC复用现MAVLink算法，初态0且无终异或。当前配套PX4生成脚本给通用元数据计算未压缩JSON、URI却为xz，子元数据计算压缩文件；因此仅实际PX4 GENERAL允许明确decoded-px4-general兼容域，其它仍核传输件。无CRC保持not-compared，不补认证、真实性能、构型或执行权限。

后台固定raw32MiB／JSON32MiB／字典64MiB，最多三份独立唯一临时文件；分块、EOF／错误、取消及实际worker结果和finished双尾分别处理。原始输入不移除，快照由实际所有权释放；有界新解压入口与旧两参数入口分开。三份合计最多96MiB临时磁盘，不声称总解码内存仅64MiB或绝对IO／CPU时限。

通用目录在新batch先退休旧来源，候选类型／CRC／URI整份通过才一次安装；坏文件不保旧目录或成功前缀。参数定义工厂完整schema及半装问题仍待后续处理，不因文件CRC匹配宣称定义已验证。

Manager冻结本次来源、PRIMARY、请求／阶段和实际worker；请求回调及InitialConnect完成回调补实际QObject寿命。Vehicle关闭／移除／析构先退休本通道，避免部分析构中继续旧步骤；可选步骤失败终结不是文件成功。初始加载不要求initialConnectComplete，不借共享FTP.cancel清别人，不把文件一致当远端PRIMARY字节、HTTP端点或设备身份认证。

翻译改为本次唯一TS／译后JSON输出，完整读写及XZ各32MiB／字典64MiB；冻结原下载器／URL／代次、先退旧槽再通知，迟回只清本次唯一输出。译文不是原CRC核过内容；底层下载basename碰撞、递归引用图深度／CPU与全部临时内存仍是后续缺口，不称全部跨机输入隔离或完整图预算已解决。

本批18源码／工程登记＋15文档=33路径，新路径7条；48项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／CRC／hash／解压／下载／缓存实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、PR更新或打包；没有实际下载、读写或删除业务缓存／归档。原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估。
[加载接线](../src/Vehicle/ComponentInformationManager.md)、[后台快照](../src/Vehicle/ComponentMetadataArtifactWorker.md)、[原始缓存](../src/Vehicle/ComponentInformationCache.md)、[翻译输出](../src/Vehicle/ComponentInformationTranslation.md)、[33路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 525 当前：已核备份后的单个退休原件永久清理（源码静态收口）

补长期运营的真实磁盘退役路径：旧retire／restore仍只改名，备份仍另存；新增独立只读预览及明确永久清理一个retired原件的入口，不触当前库、sealed段、旁路或备份，不批量、递归或自动清理。本批源码实现及限定静态审阅已收口，不是可运行版本、放行或完整长期留存策略完成。

依据只来自原实际backup worker的完整backup31、backupVerified且source.kind=retired，并等原resultReady与actual finished双尾；公开last、文件名、hash、目录或导入材料不能造删权。新原生私有basis／原preview绑定本次操作；请求固定五个字符串，仅preview／commit／discard／cancel，不能携任意路径、文件Map或proof。

预览worker只读核原完整SQLite／double SHA、两目录与实际Windows文件对象；提交消费原30秒预览并重新核原两file ID及同handle字节，再以原源handle请求删除。非Windows明确unsupported，不用按路径删除或跨平台不安全回退。已核副本不等于异地介质、原始事件真实性或掉电耐久。

提交另以fresh操作UUID创建不可覆盖的小型意图／终态收据；意图完整写入及flush确认前不删除。删除调用、可能删除、原件事后缺失、备份后核、两handle关闭及收据错误各自保留；晚到取消或终态收据失败不伪造回滚。sourceLogicalBytes仅原件逻辑大小，diskSpaceReclaimedVerified仍false，不声称物理空间已释放。

原维护页内嵌独立小面板，经原archive.retiredPurge门面显示原件／副本与两项确认；旧History Loader不新增控制器或服务。交叉审阅修正真实137字符备份名误拒、结果接纳后按钮自锁和字段通知中销毁漏取消；焦点沿实际Flickable滚动。UI/UX只静态指导原Qt主题、独立确认、来源文字及键盘反馈，未运行搜索脚本、预览或交互验证。

本批10源码／资源＋15文档=25路径，新路径仅PurgePanel.qml及合同两条；64项工程场景全部未执行。未编译、执行项目／parser／helper／业务／QML／JS／模型／认证／SQL／hash／原生Windows API实现、测试、仿真、操作硬件／PX4、刷机、放行、提交、更新PR或打包；没有实际删除、创建或读写业务归档。原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估。

[原生合同](../custom/src/Swarm/FleetPersistentEventStore.md)、[worker](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)、[界面](../custom/src/Swarm/FleetRetiredArchivePurgePanel.md)、[25路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 524 当前：已闭部署归档与本地容量回收

回到任务执行生命周期，新增明确选择旧本地组、独立选路径和确认后的归档／退休流程。原最多8次终身分配限制改为最多8个live或等待实际销毁的组；终身分配尝试另以uint64字符串累计，不回绕、不重置。没有自动淘汰、增大容量、复用旧对象／批准，或重建Orchestrator绕过128条受保护记录限制。

只允许非current／非当前监视、原实际来源完整的旧组。原生检查全部初始及最多8个ground事务／Run、真正静默栈及实际Swarm整张owner表无原占用；有MODE准备的Run须原独立Close已提交，明确从未began的原unissued关闭例外不靠空值猜测。完整pristine未发布失败组可为0报告；注册task失败的真实未发Run仍完整保在quiet材料，不造任务。已began但未issued分支不再自动退休而堵死人工Close，仍走既有双账号关闭，不放宽归档资格。

后台复用原protected8完整报告codec，逐原record16生成0..9份报告，保完整quiet18和独立document15，总32MiB／单报告2MiB。保存、commit、回读与最后原生退休分开：commit不明不称未写，成功后取消／回读失败仍保文件已写，但不退休。文件回读匹配后还须原对象、完整quiet及全部record逐值尾核；不把hash、文件内容或操作员勾选当当前原生来源。

专用native批次只退休准确已闭记录，普通removeTask仍拒受保护记录；同批清其本地语义缓存，不删除排队历史事件或他人占用。旧组及全部受控QObject后代（最多256个）弱引用均实际为空，并在后续turn复核后才回收8槽；父destroyed信号和deleteLater不算全部销毁。原source-lost监听按准确组隔离，退休旧组不毒化后来current，也不把本地回收称停飞、落地或任务Completed。

原部署管理弹窗新增归档区；Root只给typed九字段窄请求及状态／结果，不公开Episode、账号或引用scope。独立打开文件可无live飞机／账号，不恢复任何对象、观察或权限；只显式选一份原生canonical报告文本，按2048UTF16分片查看，不在GUI解析／序列化整包。文件时间明确仅文档声称；弹窗重开可只读查看原生上次事实，但不能认领为本页新请求。UI/UX仅指导原Qt主题、分层确认、长标签／错误反馈和键盘操作，未执行技能脚本或预览。

外来job即使打开声称同archiveId的另一文件，也退休本页旧历史目录与报告；本页open受理同样先撤旧历史资格。界面忙碌期间被挡的job通知在原调用票释放后补核，不能用后来state将新文件报告拼在旧标题下。这里只撤界面资格，不删除磁盘文件或伪造取消别人的任务。

本批19源码／资源登记＋20文档=39路径，新路径6条；无新QML后端服务、wire、外部依赖、SQL schema、飞控或PX4修改。56项工程场景全部未执行，只做源码、直接静读和普通文本／路径／Markdown／空白核对；未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。完整25目标和六类实质剩余保持，约40%源码／60%剩余仍为工作量粗估；本批不证明最终StartPermit、正式远端任务结果或全部长期运营完成。

[生命周期合同](../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)、[归档worker](../custom/src/Swarm/FleetDeploymentArchiveWorker.md)、[界面合同](../custom/src/Swarm/FleetDeploymentArchivePanel.md)、[39路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[56项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 523 当前：GNSS 历史集合与独立文件工作区

将522的真实同包报告接到原FleetOperationalMemory的独立历史集合：只允许实际Archive及准确原生报告token追加，不接受QML报告／坐标作为来源。保每页完整33字段、原7身份及18字段样本；按run／headerHash／原sequence去重，不依会随文件增长变化的fileHash，也不把采集Run或数字机号当可信任务／实物身份。最多32报告／2048样本，重复或容量满整份拒绝，不自动裁剪去重依据。

旧运营观察、KPI、热区推断、5000条裁剪及旧schema1导入／清空完全不接这批记录。新增共享纯值报告与集合校验；它只验证文件内容自洽，不认证原始采集、设备、精度或飞行状态。无GPS和无fix历史仍保诊断，不补旧坐标、不计算任务完成率或环境风险。

新增显式冻结、选路径后另行确认保存、独立只读打开、每页8份目录及单份报告查看。文件root为新闭11字段，4MiB上限；冻结保原Memory instance／revision和不可变字节，打开文件不导入active记忆、不恢复权限。单槽worker只消费纯值，实际结果与线程结束双终结后接纳；取消不抹掉已commit文件，写入已知／未知与readback、fsync／durable分开。

提取共享FleetSnapshotFileIo，让维护与GNSS工作区使用同一路径检查、64KiB读写及QSaveFile提交／回读逻辑。原维护32MiB、错误码和回执边界保持，GNSS用4MiB；不是恶意并发／祖先reparse／失电安全认证。本轮只改实现，没有实际创建、读取或写出任何业务档案。

历史面板新增独立记忆区：追加、冻结、保存、打开、选报告与选地图均须明确动作，不自动读写／续页／选首条。原地图共用唯一选择，投影新增origin区分原生页、session冻结历史和文件历史；只绘当前选中的一份离散点，不把文件值冒充当前Archive或实时定位。UI/UX静态指导用于原Qt主题、分层确认、文字来源和键盘操作，未运行技能脚本、预览或交互验证。

本批18源码／资源登记＋18文档=36路径，新路径8条；无新QML后端服务、wire、外部依赖、数据库schema、权限或PX4修改。仅源码、直接静读及普通文本／路径／Markdown／空白核对；48项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。

原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估。本批是有界历史留存与重开链，不是全历史自动索引、自动留存清理、跨任务统计、链路／风场模型或最终StartPermit；active集合满时仍须明确拒绝，不通过清文件槽偷偷清空。

[运营记忆合同](../custom/src/Swarm/FleetOperationalMemory.md)、[文件工作区](../custom/src/Swarm/FleetGnssMemoryWorkspaceWorker.md)、[界面](../custom/src/Swarm/FleetGnssMemoryWorkspacePanel.md)、[36路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 522 当前：同包历史 GNSS 诊断与显式只读地图

为集群记忆地图接入第一条真实历史地理依据：使用现固定本机遥测档案的原生已完成页，另行确认后只解释同一GPS_RAW_INT帧的位置、fix、HDOP与卫星数。最多64帧，先复用现MotionReview完整页校验再解码；不从Registry、实时Fact、其它消息或分页补位置与质量。没有GPS的合法页明确为空，不等于区域安全或从未出现异常。

fix仅3／4／5／6／8且经纬度合法才发布该帧报告坐标；0／1／2／7及未知fix只计未定位，旧坐标不投点。原eph65535与卫星255保未知，不补0；HDOP无单位、不是米精度。HDOP>2.5或卫星<6只是固定显示提示，质量未知与阈值内分开，均不认证定位精度、漂移原因或环境风险。

Archive保实际页来源与独立新报告UUID。原六字段action与十一字段回执只新增gnss-review动作；原运动／四视角报告和维护流程保持。界面独立确认GNSS位置发布，再另行明确选择地图，不自动选图／续页／拟合视野。地图只绘离散历史点，含质量文字图例，点与标签吞点击，不接Goto、任务编辑或飞行控制。

地图选择只接受实际报告token，Root自行读取原生来源；原owner／完整报告／token与独立选择代次绑定。输入、隐藏、换源或新查询退休本页选择，旧页面不能关闭后来替换的选择。公共投影固定深度冻结；不支持冻结时不绘图，不把脱离副本当不可变保证。

本批8源码／工程登记＋13文档=21路径，新路径仅GnssReview h／cc／md三条；无新QML资源／服务、wire、依赖、数据库schema、权限或PX4修改。UI/UX技能仅指导原Qt主题、独立同意、文本图例和键盘焦点，未运行技能脚本、预览或交互验证。

仅源码修改、直接静读及普通文本／路径／Markdown／空白核对；40项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍为粗估。本页图层不是持久跨任务全集、风场／链路风险地图或最终StartPermit。

[原生报告合同](../custom/src/Swarm/FleetTelemetryGnssReview.md)、[历史工作流](../custom/src/Swarm/FleetTelemetryHistoryPanel.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 521 当前：普通区域任务的速度／续航准入下限

补通普通区域任务的两项性能要求：最低速度能力minimumSpeedMps（m/s）和最低续航能力minimumEnduranceSeconds（秒）。现有性能政策编辑器将原strict三态与两个可选下限一次提交给原草稿，不分次保存半套政策；空白不补0，显式0与缺省保持不同来源身份。这里不是巡航速度指令、当前剩余飞行时间或实测性能认证。

新增原生三键完整政策CAS：只替换requirePerformanceEvidence及两floor，保其它settings；任一下限存在（包括0）使用planningSource schema6。schema1..5不能夹带新字段，6缺全部floor也拒绝；旧无floor来源不自动升级。普通设置编辑、检查点保存／恢复及修复仍保完整来源、准确revision与原datum重确认，旧审批不恢复。

Matcher与角色分配在选择前、Compiler在固定主／备用成员上都执行正下限；缺值或Registry实际−1不能通过，不拿其它机的最大值抵扣。strict=true仍只取本机当前构型匹配的工程声明；false／未声明可用原规划描述符，但不能关闭明确下限或宣称可信实测。0／缺floor不新增正值证据依赖，当前电池与逐机能量门不变。

Compiler任务根、归档计划和PayloadBuilder按presence、类型和完整来源值绑定两个下限，不能在保存、修复或直接编码时丢字段／降级。界面复用原Qt主题和UI/UX静态指导，明确单位、未知／0、采用／取消及失败反馈；没有运行搜索脚本、预览或交互验收。

本批9源码＋18文档=27既有路径，无新类／资源、wire、依赖、数据库schema、权限或PX4修改。仅源码、直接静读与普通文本／路径／Markdown／空白核对；48项工程场景全部未执行。未编译、执行项目／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍为粗估，不按字段或控件数上调。

[草稿合同](../custom/src/Swarm/FleetMissionDraft.md)、[性能政策入口](../custom/src/Swarm/FleetPerformancePolicyEditor.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 520 当前：备用出勤显式只读刷新与本次请求归属

为519的被动参数读取补上独立、人工确认的刷新入口。复用现有FleetMissionEvidenceRefresh读取实际版本及固定29参数，不新增发包服务、参数写入或飞行命令；原账号／角色、真实地面未解锁、PRIMARY链路和占用门禁保持。

本次刷新绑定原私有capture、明确选中的备用机、实际scope／刷新对象及真实requestId／generation。开始前退休旧参数offer、提案和审阅资格，但保留来源与已归档事实；状态与取消只认本次原请求，不认同数字ID或共享latest。界面离开／换源先撤本次资格，取消仅停止后续读取，已排队请求和晚到回包可能仍在途。

刷新结束只显示实际结果，不等于12参数仍满足30秒新鲜度。操作员仍须单独重新读取本次依据、明确采用预算速度和新配置，再走原完整审阅及审批；不自动刷新、重试、observe、采用、批准或恢复旧权限。同来源只保本页手填的动作／坐标和已明确采用的启动策略纯拟值，未采用启动策略子表单须先处理。

本批2源码＋13文档=15既有路径，无新组件／资源、C++、wire、依赖、数据库schema、权限或PX4修改。仅源码编辑、直接静读和普通文本／路径／Markdown／空白核对；工程场景全部未执行。UI/UX静态指导用于原Qt主题、请求归属、状态区别、反馈与键盘焦点，没有运行搜索脚本、预览或交互验收。

未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍是粗估，不因刷新按钮或字段数上调。

[刷新工作流](../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[面板合同](../custom/src/Swarm/FleetReserveDispatchPanel.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 519 当前：备用独立出勤的真实参数绑定与结构化配置

原备用出勤要求人工填写完整executionProfile和参数hash。本批为实际冻结capture／选中reserveId接入现Coordinator原生12参数只读offer；Root保留私有本次offerToken、原对象与参数fingerprint，生成提案前重新读取并核对。外来Map、同值ABA、旧token或另一备用的参数不能代替本次真实来源；原生参数读取不刷新或写入参数。

新增纯值配置编辑器，不伪造Draft。操作员明确采用本次备用预算速度，再选择起飞／结束动作、AMSL坐标、时序和完整启动策略；requiredProfileHash只取本次实际offer，不再手填。复用原启动策略编辑器，完整JSON仅供核对；没有从旧主机、GNSS或模板补起降位置和安全默认值。巡航与返航仍须同原备用cruise，垂直速度仍须匹配原预算和实际参数；不兼容时回原计划重做，不静默修正。

仍走原reserve-dispatch Planner→Compiler／能力与能量／当前电池审阅→归档／采用／独立批准／共享部署流程。新三参数preview入口要求准确offerToken，底层原exact3请求、profile schema1与启动策略1–4不改。读取和生成配置都不是上传、接管原任务、性能认证、StartPermit或自动出勤。

本批5源码／资源接线＋13文档=18路径，新路径仅ProfileEditor.qml和对应合同md两条，无新C++、wire、外部依赖、数据库schema或权限。40项工程场景全部未执行；UI/UX技能只指导原Qt主题、分步确认、完整输入／错误反馈与静态焦点核对，未搜索、预览或交互验收。

仅源码、直接静读和普通文本／路径／Markdown／空白核对；未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估，不按表单或字段数上调。

[实际工作流](../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[配置编辑合同](../custom/src/Swarm/FleetReserveDispatchProfileEditor.md)、[修改清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行交接](../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 518 当前：维护护照显式保存与独立历史工作区

将517的本进程维护记录接到实际可见的文件工作区：显式冻结当前内容、另选路径并确认覆盖风险后保存；选择已有文件并另行确认读取，成为独立只读历史。当前护照不被历史load/import替换，文件、hash和来源组不认证实物、维护状态或权限。

冻结保原实际records／instance／revision，GUI分turn只验纯JSON类型和有界结构；文件读写、JSON规范化、评估与完整文本分页交独立单槽worker，原job结果与线程结束双尾后才接纳。517实际追加可继续，旧快照内容不会改成新记录；来源版本变化只使sourceCurrent失效，不悄悄重导出。

来源组、记录目录每页8项；完整记录／评估按2048 UTF16片浏览，长备注不整包交界面。保存分fileWrittenKnown／fileWritten与独立readbackVerified，commit失败不谎称未替换，commit成功不被取消或回读失败抹掉。QSaveFile禁直接写回退，fsync／durable／认证仍false。纯legacy根5、历史根6与原严格来源元数据保持。

本批10源码／资源接线＋13文档=23路径，新路径为worker h／cc、工作区Panel.qml和合同md四条。48项工程场景全部未执行；无新QML服务类型、wire、外部依赖、SQL schema、权限或PX4修改。UI/UX指导仅用于现Qt主题、分步文件同意、只读来源与错误反馈的静态设计；未搜索、预览或交互验收。

未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估；本功能不等于长期运营／维护预测整体完成。

[文件工作区合同](../custom/src/Swarm/FleetMaintenanceWorkspace.md)、[修改清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 517 当前：真实历史电池证据进入维护护照

从当前已完成的原生遥测历史页显式选择完整电池来源，预览本页报告温度／非负放电电流峰值和样本覆盖，再另行确认写入实际维护护照。未知、越界、充电与零值分开；不从实时Fact补历史，不新增遥测请求或采集流。

以Run／头摘要／文件摘要／sourceEpoch／机号／组件／电池槽七字段隔离历史来源组，不与同数字机号的手工记录混合。相同来源组的选中序号闭区间重叠拒绝；容量2000满时明确拒绝，不淘汰去重依据。schema1文件保完整历史元数据并原子拒绝重复／重叠；历史页峰值不计算物理两点趋势，不补振动RMS、寿命或飞行次数。

新增嵌入式维护页与现指挥中心来源组摘要；私有Archive／Passport由Root实际配对，公共接口不接任意record或raw payload。页面失效、换源、隐藏、选择／同意ABA不得沿用旧确认；已追加事实不因界面退场被当作回滚。只保本进程记录，本入口不自动落盘或恢复权限。

本批10源码／资源接线＋13文档=23路径，新路径仅Panel.qml与Import.md两条。40项工程场景全部未执行；未编译、执行项目／helper／QML／JS、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。仅源码、直接静读和普通文本／路径／Markdown／空白核对。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估；本功能不代表目标21整体完成或当前飞机适航。

[实际数据与API合同](../custom/src/Swarm/FleetMaintenanceHistoryImport.md)、[修改清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行交接](../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 516 当前：近期原生SOC下降窗下调已有续航预算

新增独立native SOC下降窗：实际原PRIMARY收包、最早RAW本地时刻和原complete尾核后留有界采样；30–120秒、至少下降2个百分点、原报告gap≤3000ms、最多256点才可发布。flat不刷新已发布终点／速率，最新RAW须≤3000ms，下降终点须≤30000ms；模型统一扣两者较大年龄。上升、未知、坏时钟、用途／类型或来源断裂清窗，不从Fact缓存拼接。

实际Registry在同Vehicle／recordEpoch／bindingEpoch下读取旧电池20字段与新top25／window8，剥离静态root／metadata同名键。新共享校验不认证Map；坏新证据明确拒绝，合法warming／stale／unavailable不补模型。报告SOC不是实测电功率或物理电池身份。

Scheduler只下调已有known可用秒数及已有known电量投影，随后重算缺口、成员资格及替补建议。严格性能声明与原故障门保持；assignment18/21、编译备用预算和旧电池schema不改。可选nativeBatteryDrainAdjustments逐行14字段保完整窗口、应用前后与原因，并沿原完整energyPlan归档／报告／515明细；普通规划页只新增依据标签。

Root通过exact6共享值桥消费语义身份，排除自然age及重复receipt抖动；窗口失效／换源／重建使旧审阅不当前。批准期还核实际扣龄预算是否覆盖原所需秒数、是否新跨原替补阈值，不重写旧账单或自动批准。估算、上传、机载接纳与最终StartPermit不等价。

本批11源码／接线＋18文档=29既有路径，无新增文件、资源、wire、外部依赖、数据库schema、权限或PX4修改。48项工程场景全部未执行。UI/UX指导只用于原主题下的清楚只读依据标签，没有搜索脚本、预览或交互验收。

[原生观察](../custom/src/Swarm/FleetNativeBatteryObservation.md)、[实际数据链](../custom/src/Swarm/FleetNativeBatteryBridge.md)、[修改清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标／六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估，不按采样点或新字段数上调。仅编辑、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。下方为历史。

## 515 当前：新出勤逐项编码／完整账单与保留提案重审

备用出勤新记录新增默认折叠的只读明细：实际Spatial view17／逐机15／编码项9，分页核对每个真实seq、command／frame、原来源索引、raw／canonical和量化字段；完整能量、风险、空域及审阅值可逐层浏览。每层8项，长键和值分片，不把缺失／null／false／0或负余量混为未知，不用旧区域账单或合成启动捕获补源。编码拒绝保完整错误及未编码要求，不造成功前缀。

新增明确“保留本提案重新审阅”。只用本页实际capture／proposal、实际reviewAttemptToken、旧reviewToken／requestId精确认领；先退休旧采用／本地批准，保私有完整提案，再走原当前证据审阅／归档。失败和过期可人工恢复流程，不清原备用来源强迫重选；外来新记录、活动传输或规划不被抢占。新结果仍须重新采用、申请批准／明确批准与原独立上传确认，没有自动刷新、重试或旧权限恢复。

本批6源码／资源接线＋15文档=21路径，新路径仅ReviewPanel.qml／ReviewPanel.md两条。没有新C++、wire、外部依赖、权限或飞控修改。48项工程场景全部未执行。UI/UX技能用于现Qt/QGC主题、渐进展开、分页／长值／焦点的源码核对，未运行技能脚本、预览或视觉／交互验收。

[新明细合同](../custom/src/Swarm/FleetReserveDispatchReviewPanel.md)、[重审工作流](../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本轮只完成源码与静态接线，不表示机载已接纳、任务接替、实际轨迹／净空或最终StartPermit。原25目标／六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估，不按明细或按钮数上调。仅编辑、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 514 当前：备用独立出勤的可见人工流程与唯一共享部署

在513完整后台提案之上新增真实Root来源捕获和FleetReserveDispatchPanel：操作员先明确冻结当前完整task／plan／存档绑定，再选一架实际备用、填写完整新executionProfile、核对自身完整去返路线与summary14，重新提交正常审阅。捕获先冻结再撤旧，精确自身退休收据仅容一次真实批准→preview推进，其它输入／provider／代次或ABA变化仍拒绝。

新kind=reserve-dispatch贯穿捕获、提案、同步／异步回执、审阅归档与唯一共享部署区；旧single／group不能认领、批准或清理它。采用、本地申请批准／明确批准、不同账号上传审批与覆盖确认分别保留，采用不是批准。采用后自身pending不自锁部署，隐藏／退出／取消只处理自己的准确token与请求，不删除已提交事实或自动切回草稿。

独立视图消费本次准确Store记录与同份reviewEvidence，经既有missionSpatialReview和实际PayloadBuilder生成；地图只绘同一新记录的完整两条二维路线，不造Run、原序号或起降轨迹。原未知政策／共同floor／边界、全新profile、当前能力／电池、原生上传门禁与最后StartPermit不放宽。这不是实际备用出勤、原组接替或自动机载改派完成。

本批8源码／资源接线＋18文档=26路径，新路径仅Panel.qml／Panel.md／Workflow.md三条；没有新C++、wire、依赖或权限。56项工程场景全部未执行。UI/UX技能仅用于现Qt/QGC主题、确认／错误／键盘焦点的静态核对，没有运行搜索脚本、预览、交互或视觉验收。

[Root工作流](../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[可见面板](../custom/src/Swarm/FleetReserveDispatchPanel.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标／六类实质剩余保持，约40%源码完成／60%剩余仍仅工作量粗估，不按文件、入口、批次或防护数量上调。仅源码修改、直接静读与普通文本／路径／Markdown／git空白核对；未编译、执行项目解析器／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 513 当前：备用保障方案生成独立出勤任务的后台接线

新增FleetReserveDispatchPlanner：从完整原task／plan／binding和明确选中的备用机，生成一份独立单机新任务。新执行路线使用备用自身起返点，并保留原被替补成员全部航点、heading与完整停留；不能直接拿已简化的预算路径代替任务要求。新executionProfile必须人工重新提供且与预算速度一致，不沿用原主机的起降配置；历史待机不在新任务重复执行或扣时，当前电池仍须新审阅。

新reserveDispatchPlanningSource为闭17键schema1；结果7键、摘要14键。完整task和member／constraints／原边界／严格性能布尔分别绑定，Compiler前置任务核对与输出前真实plan核对、PlanStore归档及PayloadBuilder正常编码入口均消费同一来源；四种规划来源互斥。旧single／group再规划只清旧出勤来源并重新绑定，不继承Run、批准或控制权。

保留共同安全／能力／能量及所选成员政策和floor；显式单机用途重设成员／角色数量和备用0，不声称原组已接替或原组保障不变。原clearanceInput不自动换ID，不适用仍阻断。源32MiB、任务／匹配16MiB、请求128KiB、provenance64KiB及每机2048／全组8192编码项等预算不扩，未知要求不能为生成成功而静默丢弃。

本批仅完成后台规划与真实编译／编码／归档消费者接线，尚未新增Root真实来源捕获、可见按钮、确认流程或共享部署源选择；这些是同一备用出勤功能接下来的实质工作，不把Q_INVOKABLE当可操作产品或真实任务执行。正常审阅、独立审批、当前能力／电量及最终StartPermit仍不放宽。

[备用出勤合同](../custom/src/Swarm/FleetReserveDispatchPlanner.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批10源码／工程登记＋18文档=28路径，新文件仅Planner h／cc／md三条。40项工程场景全部未执行。仅源码修改、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标／六类实质剩余继续，约40%／60%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 512 当前：联合余段任务重建新备用保障

联合余段任务现在支持显式重填完整新备用配置，解决原主／备用换机后旧reservePlans仍指旧ID而必须阻断的未开发边界。不是暗改数字或删除原拒绝：每个新备用恰一份自身起点／返点、替补的新执行机、待机与速度策略，完整新方案仍重新进入现Compiler→ReviewCoordinator→EnergyScheduler。

原三键请求继续旧保守合同；含reservePlans字段就要求新四键完整请求，坏容器不能降级。新provenance schema3／exact28保完整canonical reservePlans；原sourceTaskPolicyHash继续标改前政策，完整新task／routes摘要绑定新意图。新备用运行配置允许显式重新规划，不冒称旧待机／速度值全部未变；最低备用数量及其余共同安全／能力／能量／返航政策保持。旧clearanceInput不自动换ID，资料不适用仍阻断。

task与compiled-plan匹配分开。后者消费真实reserveMembers.energyPlan，以现备用生成器从实际新成员路线重算完整路径／待机／任务／返程成本及关联，不能仅比ID、首尾或hash；当前性能分支不得自行降为legacy。值重算不认证当前性能或来源，实际新能力／电池与原审批来源尾核仍必须成立，旧档案不恢复当前权。

现联合面板新增有界JSON输入及逐备用预览，沿原主题／PlainText／焦点反馈和两步人工确认。文字最多32768字符、最多32描述；原4MiB请求、64KiBprovenance、16MiB任务／匹配、32MiB源／结果及32768生成点预算不扩。没有新模块、资源、依赖、命令、数据库schema或自动改派。

[联合规划器](../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)、[操作面板合同](../custom/src/Swarm/FleetMissionGroupRemainderPanel.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批4源码／接线＋13文档=17既有路径，48项工程场景全部未执行。原25目标与六类实质剩余保持，约40%源码完成／60%剩余仍为粗估，不按本批路径或局部功能上调。

仅源码、直接静态阅读与普通文本／路径／Markdown／git空白核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。新备用预算不证明真实待命、已接替、机载改派、可信实测耗能或StartPermit；完整目标继续。下方为历史。

## 511 当前：MODE发送前的真实begin检查点

原生begin在安装本次真实queueBegan后，立即走同一保护模型提交；只有提交接纳成功、原ReadContext／serial与实际对象仍同、原queue仍处于未commit且可跟踪状态时才返回true。内部Target引用不跨完整Evidence的Link虚调，回调后先核QQ／原上下文再重新定位。失败不回滚已安装的开始事实，也不据日志失败抹掉后来实际write。

这使510事件链能在成功提交时分别捕获“开始准备”和“已调用实际发送”，而不是把两者合并猜测。不是额外控制器、远端ACK、网络送达或正式MODE执行。Vehicle原调用者仍再核原binding／授权／消息不变性；实际write后原commit始终先保真实调用事实，其旧生命周期语义不因新的bool返回值而回撤。事件队列仍可能有明确缺口，不能承诺每项已持久化。

完整Evidence会调用Link虚函数，可嵌套RAW、取消、退休或销毁；这里只保证新路径不直接emit／写文件／发控制，不能宣称纯内存无外部回调。没有新wire、公开API、报告schema、账号权限、资源、依赖或新增路径。

[执行观察合同](../custom/src/Swarm/FleetMissionExecutionObservation.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批2源码＋13文档=15既有路径；工程场景28项全部未执行。原25目标和六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按检查点或文件数上调。

仅源码、直接静态阅读及普通文本／路径／Markdown／git空白核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。本检查点不授最终StartPermit，不认证远端任务实例、正式结果、占用释放或放飞。下方为历史。

## 510 当前：原生Run语义事件接入任务历史

受保护原生TaskRecord提交后的注册、MODE本地交接、终点候选、连续性异常、源退休和观察关闭，新增独立低频事件通道进入实际BlackBox及其持久归档，不再只停留在内存观察通知。逐包计数／年龄／临时sourceCurrent不作为事件变化；同Run的异常位图保历史已观察事实。

独立FIFO最多512行／2MiB canonical payload、单条4096字节，每turn最多64条；不是堆内存硬上限。溢出保旧事件并累计缺口，原生临时收据只在实际发布期间有效；缺口不归入假任务。Root另记本地接纳／未记录／未知，BlackBox接纳不等于SQL提交或完整历史，持久录制仍默认关闭。

新v3固定12字段脱敏投影保28字段runObservation；原v1／v2、SQL及历史查询schema不变。实际按taskIdHash下钻和两历史面板新增只读摘要；现报告将reviewRequired=false普通转换作为info，异常只作warning人工复核，raw保原事件。日志timestamp是实际入日志的本机UTC，原捕获UTC另保，不对队列事件偷偷排序或改写时间。

[事件合同](../custom/src/Swarm/FleetRunObservationEvent.md)、[实际日志接线](../custom/src/Swarm/FleetRunObservationJournal.md)、[25路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[60项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批8源码／接线＋17文档，新路径仅两份模块合同，无新代码文件／资源／外部依赖。

这仍是本地Run观察，不是任务Completed／Failed、远端实例认证、正式结果、占用释放或飞行许可。原25目标与完整执行／可信性能／自愈机载改派／跨站协作／长期运营／真实模型插件六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按本批路径上调。

仅源码、直接静读及普通文本核对；未编译、执行解析器／helper／业务／QML／SQL／认证／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 509 当前：人工单流STOP／START

- [控制模块及原生值合同](../custom/src/Swarm/FleetCameraStreamControl.md)：一次基线、原30秒、独立二审、单发与回读。
- [账号审批合同](../custom/src/Swarm/FleetOperatorSession.md)、[角色政策](../custom/src/Swarm/FleetRolePolicy.md)：READ不授写权，专用typed证明。
- [读取交接](../custom/src/Swarm/FleetCameraStreamCapture.md)、[现有面板](../custom/res/Merivus/FleetCameraStreamCapturePanel.md)：原报告状态、相反目标、两确认和精确取消。
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[ACK审计](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)、[64项未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批14源码／接线＋16文档=30路径，新文件仅Control h／cc／md。自己的实际action之后仅RUNNING位Info更新可继续，配置ABA和来源变更仍失效；报告匹配不是实际停流／带宽认证。原25目标／六类实质剩余不缩减，整体约40%／60%仅源码工作量粗估。本批未编译、运行项目实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 508 当前：人工相机流信息／状态读取

新增人工单流相机 Info269→Status270 查询：独立 READ 权限与真实账号／角色／Root 意图，经 Swarm 私有服务、Registry 原绑定、Vehicle 原生 Run 和现有共享消息通道，接到指挥中心／规划页面板。打开页面或原五秒策略不会自动查询；不启停流、配置、降速或恢复。

原入口总8秒包含授权准备，每阶段最多4秒，accepted ACK 后报告窗口最多1秒且只缩短；每项至多一次实际发送。允许真实来源满足检查的飞行状态 metadata READ，不把 RTT 的地面限制直接套入。取消保留已可能发送／ACK／报告事实，不宣称撤回。

ACK 与269／270无nonce，本地匹配不证明无线因果、实际送流／停流、完整目录或带宽变化；270索引兼容性待工程师确认，不支持不回退0／all／广播。complete 不等于两份报告此刻同时新鲜。远端受控视频降级与恢复仍待开发。

[服务合同](../custom/src/Swarm/FleetCameraStreamCapture.md)、[面板合同](../custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[28路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批14源码／接线与14文档；新增 Service h／cc／md、Panel qml／md共5路径，没有新外部依赖。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅为源码工作量粗估，不按批次、路径、接口或局部保护数量上调。完整任务执行、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 507 当前：原生相机流报告进入实际只读消费者

新增原PRIMARY相机VIDEO_STREAM_INFORMATION269／STATUS270被动账本，经Registry实际绑定一次读取、Policy第五参数和Guided原五秒周期接入既有指挥中心摘要；不发新请求或控制命令。另修上一轮留下的setVideoMode／setPhotoMode实际caller，以及其toggle／reset模式意图衔接。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

[完整合同](../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 506 当前入口

- [相机／视频生命周期合同](handoff/MERIVUS_VIDEO_LIFECYCLE.md)：本地意图、真实选择、有限恢复、发现与模型通知尾核。
- [精确19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[50项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505 当前入口

- [机载日志有界缺块补读与partial合同](handoff/MERIVUS_LOG_DOWNLOAD_RECOVERY.md)：现有日志下载UI、原来源／选择冻结、精确字节写入、有限缺块重读和成功语义。
- [精确14路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[40项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

只是现有机载日志下载可靠性修补，不是完整Fleet网络补传；不新增UI、授权、外部接收端或自动恢复。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504 当前入口

- [人工单次定向TIMESYNC往返耗时](../custom/src/Swarm/FleetLinkLatencyCapture.md)：native26、Service18／5、实际wrapper7、Policy23／10及Panel。
- [Role精确READ权限](../custom/src/Swarm/FleetRolePolicy.md)与[Policy独立第四参](../custom/src/Swarm/FleetLinkDegradationPolicy.md)。
- [31路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[44项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

当前仅受控地面单次采集，包含排队／远端处理，不是纯网络QoS，不支持即未知，不回退PING或广播。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503 当前入口

- [本地主链路被动接收窗](../custom/src/Swarm/FleetLocalReceptionObservation.md)：native22／window9、实际Registry wrapper7、Policy及五秒消费者。
- [Registry合同](../custom/src/Swarm/FleetRegistry.md)与[Policy合同](../custom/src/Swarm/FleetLinkDegradationPolicy.md)。
- [23路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[40项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

完整本地接收窗不是带宽／延迟／丢包认证，也不触发自动配置或飞行动作。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502 当前入口

- [可选遥测可见面板](../custom/res/Merivus/FleetOptionalTelemetryPanel.md)：仅本页真实回执，三步显式操作、原30秒、取消与历史。
- [只读采集](../custom/src/Swarm/FleetOptionalTelemetryCapture.md)与[独立配置后端](../custom/src/Swarm/FleetOptionalTelemetryConfiguration.md)。
- [24路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[44项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501 当前入口

- [独立授权遥测配置合同](../custom/src/Swarm/FleetOptionalTelemetryConfiguration.md)：固定三流、opaque Basis、九阶段Run、原30秒、partial与未证明边界。
- [Session用途隔离](../custom/src/Swarm/FleetOperatorSession.md)与[Role强制二审](../custom/src/Swarm/FleetRolePolicy.md)。
- [精确30路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[46项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500 当前入口：单车显式可选遥测间隔采集

- [Capture原生对象／时序／32与24键合同](../custom/src/Swarm/FleetOptionalTelemetryCapture.md)
- [独立本地READ action](../custom/src/Swarm/FleetRolePolicy.md)
- [精确24路径修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [40项全未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [静态审计记录](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前入口：消息查询／配置的真实native排他

- [18路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [ACK／原包归属及本地代次边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [42项全部未执行工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [Registry只读基线失效](../custom/src/Swarm/FleetRegistry.md)

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前入口：真实被动遥测的只读审阅

- [精确18路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [Registry原生观察／review合同](../custom/src/Swarm/FleetRegistry.md)
- [Policy建议与实际消费边界](../custom/src/Swarm/FleetLinkDegradationPolicy.md)
- [36项全部未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)

本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前入口：现有511请求归属及生命周期

- [18路径精确清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [静态审计及已知ACK歧义](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [36项全未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [低带宽Policy仍仅建议](../custom/src/Swarm/FleetLinkDegradationPolicy.md)

本批未新增Fleet apply、权限scope或UI；旧PID／ESC链得到源码修复，不代表低带宽自动化产品完成。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前入口：完整起飞垂直编码参考

- [精确修改清单：19既有路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [静态审计范围与结果](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [36项全未执行工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [同次TAKEOFF原起点／窗口材料](../custom/src/Swarm/FleetMissionContactModel.md)
- [Waypoint v5完整垂直参考及有限pad](../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)

仅新vertical/support指定terrain完整交集可用同finitepad；模型仍非真实路径／启动许可。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前入口：起飞参考高度区间与首NAV连接

- [Waypoint模型v4](../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)：来源、完整top区间、帽分量、连接体及无豁免范围。
- [真实Worker消费](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)：原row3／结果7和matcher／实际路径门。
- [Joint名义边界](../custom/src/Swarm/FleetMissionJointSpatialReview.md)：名义两线与新第三报告不混同，不扩大planning。
- [16路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前入口：保护历史报告保存与重开

- [独立保护报告合同](../custom/src/Swarm/FleetProtectedRunReport.md)：report8／record16／summary13、文件整数槽与历史来源边界。
- [原生报告文件服务](../custom/src/Swarm/FleetMissionExecutionReport.md)：新独立两API及旧full9／handoff隔离。
- [现有报告面板](../custom/src/Swarm/FleetMissionExecutionReportPanel.md)：文件意图、二次确认、取消和只读来源。
- [18路径修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前：遥测历史跨Run元数据搜索

入口依次为[Reader原生合同](../custom/src/Swarm/FleetTelemetryArchiveHistoryReader.md)、[Archive同源与双终结](../custom/src/Swarm/FleetTelemetryArchive.md)、[实际QML面板](../custom/src/Swarm/FleetTelemetryHistoryPanel.md)。原manifest、目录身份、游标和命中hash由native持有；QML不能提供任意路径、SQL或cursor，不公开原始载荷。

[20路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步。检索不等于完整遥测、任务结果或认证；原25目标继续。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前：执行观察的live回调边界

先读[Vehicle observer lane](../src/Vehicle/VehicleMissionExecutionObserver.md)、[Run快照及生命周期](../custom/src/Swarm/FleetMissionExecutionObservation.md)、[Txn队列observer](../custom/src/Swarm/FleetMissionStartupTransaction.md)和[TaskRecord消费](../custom/src/Swarm/SwarmMissionOrchestrator.md)。三项live检查保留，不把QQ保护写成无回调证明或新权限。

[22路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步。实际write后的原事实、历史unknown及原动态许可缺口保持。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前：LAND原生参考与完整接受区域

先读[ContactModel](../custom/src/Swarm/FleetMissionContactModel.md)和[WaypointSwitchReview](../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)，再读[Worker实际消费](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)与[Joint名义范围](../custom/src/Swarm/FleetMissionJointSpatialReview.md)。新下降参考体不是单一目标竖柱，也不是实际轨迹／接地认证。

[19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步。无新文件、UI、工程／资源登记或依赖。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前：末端原参数及完整并集覆盖

先读[MemorySeal](../src/Vehicle/FleetStartupMemorySeal.md)与[StartupTransaction](../custom/src/Swarm/FleetMissionStartupTransaction.md)，再读[Geometry](../custom/src/Swarm/FleetMissionClearanceGeometry.md)、[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)及[WaypointReview](../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)。必填原29与原PRIMARY控制块，不引入新采样；候选索引不替代完整query、原并集差集或动态路径证明。

[8源码＋16文档=24路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步。无新文件、UI、资源、依赖或许可升级。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前：本进程保留Run的独立报告

先读[独立合同](../custom/src/Swarm/FleetProtectedRunReport.md)、[Orchestrator来源](../custom/src/Swarm/SwarmMissionOrchestrator.md)、[报告构建器](../custom/src/Swarm/FleetMissionExecutionReport.md)与[现有面板](../custom/src/Swarm/FleetMissionExecutionReportPanel.md)。新类型不进入旧full报告文件或交接接口，不恢复原任务与权限。

[23路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：P0观察与NAV几何

先读[Run回退关联边界](../custom/src/Swarm/FleetMissionExecutionObservation.md)及[NAV凸包棱柱合同](../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)。仅两个现有后端源码变化，没有新UI、消息／协议、类或登记；原真实Worker／TaskModel消费者保留。

[15路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步；全部工程验证未执行，不提供本批安装包或放行。下方为历史。

## 487 当前：历史观察资料报告

先读[观察卷宗](../custom/src/Swarm/FleetTelemetryObservationReport.md)、[ReportBuilder](../custom/src/Swarm/FleetMissionReportBuilder.md)、[Archive双缓存](../custom/src/Swarm/FleetTelemetryArchive.md)与[实际面板](../custom/src/Swarm/FleetTelemetryHistoryPanel.md)。新历史格式不进入旧任务报告、实际任务结果或权限链。

[21路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[34项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步。本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前：遥测历史与单页运动报告值

先读[Archive固定入口](../custom/src/Swarm/FleetTelemetryArchive.md)、[MotionReview](../custom/src/Swarm/FleetTelemetryMotionReview.md)和[历史面板](../custom/src/Swarm/FleetTelemetryHistoryPanel.md)。固定目录／原生游标→最新完成原页→独立位置派生确认→分段报告值，实际接原Root及两个宿主；不开放raw／录制／飞行权限。

[25路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)保留实现与运行证据区别。仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前：区域任务的显式性能政策

先读[新政策编辑器](../custom/src/Swarm/FleetPerformancePolicyEditor.md)和[Draft schema5](../custom/src/Swarm/FleetMissionDraft.md)，再看[Compiler](../custom/src/Swarm/FleetMissionCompiler.md)与[PlanStore](../custom/src/Swarm/FleetMissionPlanStore.md)的完整来源／任务／计划绑定。它实际使用483消费者和484录入值，不新增性能认证或许可来源。

[编辑Store](../custom/src/Swarm/FleetMissionDraftStore.md)、[保存面板](../custom/src/Swarm/FleetMissionSavedDraftPanel.md)、[历史恢复](../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)及[修复副本](../custom/src/Swarm/FleetMissionRepairDraftPanel.md)说明新源的完整保留。[28路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)为本轮入口。

原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。 本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前：工程性能声明的可操作入口

从[原面板与Root](../custom/src/Swarm/FleetCapabilityReviewPanel.md)进入[双账号Review](../custom/src/Swarm/FleetCapabilityReview.md)、[Session独立用途](../custom/src/Swarm/FleetOperatorSession.md)及[Registry一次CAS](../custom/src/Swarm/FleetRegistry.md)。新声明仍由[483共享证据](../custom/src/Swarm/FleetCapabilityEvidence.md)的既有消费者使用，无平行批准或性能认证器。

独立摘要见[Codec](../custom/src/Swarm/FleetEventArchiveCodec.md)、[Store](../custom/src/Swarm/FleetPersistentEventStore.md)、[Reader](../custom/src/Swarm/FleetEventArchiveHistoryReader.md)及[历史面板](../custom/src/Swarm/FleetArchiveHistoryPanel.md)。[31路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)为本轮入口。

原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。 本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前：工程声明性能的真实消费

[Evidence exact11](../custom/src/Swarm/FleetCapabilityEvidence.md)复用当前工程批准及完整构型基线，供[Matcher](../custom/src/Swarm/FleetCapabilityMatcher.md)、[RoleAllocator](../custom/src/Swarm/FleetRoleAllocator.md)、[Compiler](../custom/src/Swarm/FleetMissionCompiler.md)、[备用预算](../custom/src/Swarm/FleetReserveEnergyPlan.md)和[EnergyScheduler](../custom/src/Swarm/FleetEnergyScheduler.md)在显式严格策略下实际准入；[Coordinator](../custom/src/Swarm/FleetMissionReviewCoordinator.md)保schema3来源，[PlanStore](../custom/src/Swarm/FleetMissionPlanStore.md)不允许0.8.0计划去掉true策略。

[31路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[30项全部未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计范围](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)为本轮入口。没有新性能录入器／实测清单／标定或二进制认证；声明不产生飞行许可，P0原false仍保。整体约40%／剩余60%粗估及原25目标、六类实质工作保持。

仅源码和文档工作，无新文件／依赖／登记／UI，未编译、执行helper／业务／模型／QML、测试、仿真、硬件、PX4修改、刷机、放行、提交、更新PR或打包。下方为历史。

## 482 当前：原29源与普通NAV接受／连接模型

新增 [WaypointSwitchReview](../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)，由[Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)实际计算第三报告。[Evidence](../custom/src/Swarm/FleetMissionVehicleEvidence.md)／[Readiness](../custom/src/Swarm/FleetMissionStartReadiness.md)／[Refresh](../custom/src/Swarm/FleetMissionEvidenceRefresh.md)说明schema5、原29快照与固定29读取；[StartReview](../custom/src/Swarm/FleetMissionStartReview.md)／[ArmedGroundReview](../custom/src/Swarm/FleetMissionArmedGroundReview.md)／[Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)保原源回调尾核；[StartupAuthority](../custom/src/Swarm/FleetMissionStartupAuthority.md)／[GroundStartupAuthority](../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)只接完整result7及原Witness。[Deployment](../custom/src/Swarm/FleetMissionDeployment.md)只改29项静态说明。

[41路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[本批静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)为本批入口。原25目标、六类实质未完成与整体约40%粗估保持；新静态几何不证明真实路径或StartPermit。

仅静态源码／文档工作，未编译、模型／helper／业务／QML执行、测试、仿真、硬件、PX4修改、刷机、放行、提交、更新PR或打包，UI未脚本／界面验证。下方为历史。

## 481：受控整数空间域与原生执行链

新增[IntegerDomain](../custom/src/Swarm/FleetMissionStartupIntegerDomain.md)，接[Motion](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)、[精确投影](../custom/src/Swarm/FleetMissionContactModel.md)、[Cover](../custom/src/Swarm/FleetMissionStartupDomainCover.md)、[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)和[Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)。[Profile](../custom/src/Swarm/FleetMissionExecutionProfile.md)／[编辑器](../custom/src/Swarm/FleetExecutionProfileEditor.md)保显式选择及旧数据；[Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)／[Txn](../custom/src/Swarm/FleetMissionStartupTransaction.md)沿原Ptr。

本轮[39路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480：起降参数证据入口

新增[ActionPhaseReview](../custom/src/Swarm/FleetMissionActionPhaseReview.md)；配套[原生证据](../custom/src/Swarm/FleetMissionVehicleEvidence.md)、[Readiness](../custom/src/Swarm/FleetMissionStartReadiness.md)、[只读刷新](../custom/src/Swarm/FleetMissionEvidenceRefresh.md)、[执行意图](../custom/src/Swarm/FleetMissionExecutionProfile.md)、[真实审阅](../custom/src/Swarm/FleetMissionReviewCoordinator.md)和[部署确认](../custom/src/Swarm/FleetMissionDeployment.md)。

本批[27路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[30项未执行交接场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[来源／权限边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步；不按本节认定全路线图完成。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479 当前总速覆盖与原14项尾核入口

- [纯float32覆盖一致性模型](../custom/src/Swarm/FleetMissionRateOverrideReview.md)
- [PM固定14／原生证据与指纹](../custom/src/Swarm/FleetMissionVehicleEvidence.md)、[显式读取刷新](../custom/src/Swarm/FleetMissionEvidenceRefresh.md)
- [Readiness与phase末尾原源检查](../custom/src/Swarm/FleetMissionStartReadiness.md)
- [原12profile hash合同](../custom/src/Swarm/FleetMissionExecutionProfile.md)、[原公开19键接口与实际审阅消费者](../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [准确27路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478 当前完整原域与编码后检查入口

- [Planner规范策略与整组路线复核](../custom/src/Swarm/FleetAreaCoveragePlanner.md)
- [Compiler身份／原域保存](../custom/src/Swarm/FleetMissionCompiler.md)、[PlanStore严格归档](../custom/src/Swarm/FleetMissionPlanStore.md)
- [严格当前编码与独立历史复现](../custom/src/Swarm/FleetMissionPayloadBuilder.md)
- [single3／4](../custom/src/Swarm/FleetMissionRemainderPlanner.md)、[group2](../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)
- [历史资料来源](../custom/src/Swarm/FleetMissionRemainderSource.md)、[原生事务静态取源](../custom/src/Swarm/FleetMissionStartupTransaction.md)、[Readiness原策略重建](../custom/src/Swarm/FleetMissionStartReadiness.md)
- [准确31路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477 当前名义退让入口

- [退让候选、实际原域检查与分量分配](../custom/src/Swarm/FleetAreaCoveragePlanner.md)
- [source4和一次性区域策略提交](../custom/src/Swarm/FleetMissionDraft.md)
- [编辑区两字段确认、取消及地图边界说明](../custom/src/Swarm/FleetCoverageExclusionsEditor.md)
- [编译消费者及未证明的编码／动态轨迹边界](../custom/src/Swarm/FleetMissionCompiler.md)
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476 当前排除区规划入口

- [覆盖算法与共享全部名义路线检查](../custom/src/Swarm/FleetAreaCoveragePlanner.md)
- [草稿source3与保存／恢复边界](../custom/src/Swarm/FleetMissionDraft.md)
- [坐标编辑、确认、地图和来源隔离](../custom/src/Swarm/FleetCoverageExclusionsEditor.md)
- [准确修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475 当前联合新任务入口

[联合算法与来源](../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)、[实际联合编辑区](../custom/src/Swarm/FleetMissionGroupRemainderPanel.md)、[当前审阅消费者](../custom/src/Swarm/FleetMissionReviewCoordinator.md)、[全组显示](../custom/src/Swarm/FleetMissionRemainderView.md)、[准确33路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474 当前源码与交接入口

[完整Run取源](../custom/src/Swarm/FleetMissionStartupTransaction.md)、[双版本资料包](../custom/src/Swarm/FleetMissionRemainderArchive.md)、[离线成员选择](../custom/src/Swarm/FleetMissionRemainderArchivePanel.md)、[准确22路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师验证项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。新资料只用于新人工规划，不恢复旧Run或许可。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：完整历史重规划资料入口

- [Archive](../custom/src/Swarm/FleetMissionRemainderArchive.md)：exact8文件、真实提交／读回、只历史转换。
- [Planner](../custom/src/Swarm/FleetMissionRemainderPlanner.md)：共享完整源校验，原生18／历史20和provenance28／30。
- [ArchivePanel／Root](../custom/src/Swarm/FleetMissionRemainderArchivePanel.md)：实际冻结／保存／打开／明确选用与精确退出。
- [原余段编辑](../custom/src/Swarm/FleetMissionRemainderPanel.md)、[原Run取源](../custom/src/Swarm/FleetMissionRemainderSource.md)。
- [Compiler](../custom/src/Swarm/FleetMissionCompiler.md)、[PlanStore](../custom/src/Swarm/FleetMissionPlanStore.md)、[View](../custom/src/Swarm/FleetMissionRemainderView.md)：中央历史provenance消费，不放宽外层格式。
- [30路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静审](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[未执行工程师案例](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：未完成编辑态本机保存与恢复入口

- [Draft](../custom/src/Swarm/FleetMissionDraft.md)：严格可构建源与完整编辑源分开。
- [DraftStore](../custom/src/Swarm/FleetMissionDraftStore.md)：exact6／15／12、固定单槽、完整原字节CAS及提交事实。
- [SavedPanel／Root](../custom/src/Swarm/FleetMissionSavedDraftPanel.md)：实际操作、输入缓冲门禁、共享备用及上下文归属。
- [历史计划恢复](../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)／[修复副本](../custom/src/Swarm/FleetMissionRepairDraft.md)：旧严格来源与新saved来源并存。
- [28路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静审](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[未执行工程师案例](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：原运动对象与源数据连续性交接入口

- [GeographicMotion](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)：Models、完整matchesSource及有界源数据／字符串独立持有。
- [Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)／[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)：实际原Ptr与值型结构重建分路。
- [普通Review](../custom/src/Swarm/FleetMissionStartReview.md)／[地面Review](../custom/src/Swarm/FleetMissionArmedGroundReview.md)：原集合、实际finished、取消与重入边界。
- [Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)／[Txn](../custom/src/Swarm/FleetMissionStartupTransaction.md)：原对象持有及资料分支完整参数核对。
- [准确清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审阅](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师未执行项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[身份边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：完整候选索引与同源SAT交接入口

- [Geometry合同](../custom/src/Swarm/FleetMissionClearanceGeometry.md)：自持原凸柱、Morton闭树、可取消稳定排序、完整原行候选和精确indexed SAT。
- [DomainReview合同](../custom/src/Swarm/FleetMissionStartupDomainReview.md)：两个scene索引先完整构造、同一预算、失败无候选前缀、实际原行身份消费。
- [准确修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审阅](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师未执行项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[来源／权限边界](handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469 原生闭凸柱与实际整域求交入口

- [Geometry PreparedPrism／精确SAT／整数范围及纸面证明](../custom/src/Swarm/FleetMissionClearanceGeometry.md)。
- [整域场景一次准备、共享预算及实际接入](../custom/src/Swarm/FleetMissionStartupDomainReview.md)。
- [准确16路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[全部未执行验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

通用扫掠体／并集算法不变；PreparedPrism不是地图JSON、飞行凭证或来源认证。原数值包含未证和启动门禁保持。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468 五纬度带外包与报告一致性入口

- [Cover v2整数界及纸面证明](../custom/src/Swarm/FleetMissionStartupDomainCover.md)。
- [DomainReview v2、地形覆盖去冗余及处理前缀](../custom/src/Swarm/FleetMissionStartupDomainReview.md)。
- [准确15路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[全部未执行验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

API、原source12及Worker result6不变；当前模型名v2拒旧v1结果。原数值包含与启动放行仍未完成。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467 完整理想运动域与启动门禁入口

- [整数两端帽／反经线／完整高度带](../custom/src/Swarm/FleetMissionStartupDomainCover.md)。
- [整组环境计算与report28](../custom/src/Swarm/FleetMissionStartupDomainReview.md)。
- [Worker result6和统一接纳](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)、[Witness工厂](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)。
- [普通Review](../custom/src/Swarm/FleetMissionStartReview.md)、[地面再审](../custom/src/Swarm/FleetMissionArmedGroundReview.md)、[共享来源检查](../custom/src/Swarm/FleetMissionClearanceReview.md)。
- [准确34路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[未执行验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原Qt数值包含未证、整域动作豁免未建立，不能据理想外包或名义段通过释放启动。下方旧result5描述由467消费者说明升级，不把历史合同当当前接口。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466 历史备份导入交接入口

- [Worker导入5／38／27与暂存发布](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)。
- [Store私有来源与预览29](../custom/src/Swarm/FleetPersistentEventStore.md)。
- [当前可读项选择、独立确认和状态展示](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)。
- [准确20路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师待验证事项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465 逐文件备份诊断入口

- [后台诊断契约](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)：完整候选、拒收原因、整包失败边界。
- [原生只读状态与工单](../custom/src/Swarm/FleetPersistentEventStore.md)：request2、public26、实际finished。
- [维护面板说明](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)：独立路径、三计数、原名安全显示。
- [本批准确修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：独立备份读取与来源边界

- [Worker独立目录／文件检查及21键结果](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)
- [EventBackups原生枚举命名空间](../custom/src/Swarm/FleetArchiveDirectoryInventory.md)
- [Store原生24键目录、9键条目和actualfinished](../custom/src/Swarm/FleetPersistentEventStore.md)
- [UI显式读取、原目录结果及未知来源提示](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

独立读取不是恢复，不从完整列举或本次SHA推断原源对应、历史完整性或执行状态。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：显式备份、部分文件与读回事实

- [备份Worker的exact5／31／23及源目标检查](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)
- [Store独占路径预览、30秒提交窗口与实际槽](../custom/src/Swarm/FleetPersistentEventStore.md)
- [同一维护面板的路径确认与复制结果](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师未执行项目](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

本批没有实际备份或恢复数据；备份核对不是飞行任务完成、作者认证或许可恢复。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：归档维护与恢复

- [归档维护面板、预览和结果语义](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)
- [后台原生改名／前后源核对](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)
- [sealed／retired独立原生清点](../custom/src/Swarm/FleetArchiveDirectoryInventory.md)
- [Store私有预览、期限、互斥与游标退休](../custom/src/Swarm/FleetPersistentEventStore.md)
- [准确修改范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师未执行验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

本批仅归档分类维护，不认证日志作者、完整历史、任务执行结果或飞行许可。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：保留任务诊断目录

- [任务目录和原始记录面板](../custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)
- [原生累计与独立查询状态](../custom/src/Swarm/FleetPersistentEventStore.md)
- [Reader旧搜索／任务哈希筛选双版本](../custom/src/Swarm/FleetEventArchiveHistoryReader.md)
- [原历史面板与既有搜索边界](../custom/src/Swarm/FleetArchiveHistoryPanel.md)
- [准确修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

本批索引不证明完整历史、正式任务结果、运营指标或任何执行许可。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：电池明确异常的原生来源及实际门禁

- [原始电池账本](../custom/src/Swarm/FleetNativeBatteryObservation.md)：schema2 outer20／row19、扩展known与semantic revision。
- [共享值检查](../custom/src/Swarm/FleetNativeBatteryEvidence.md)：schema1兼容、fresh负面／过期负面、完整成功提交。
- [Registry](../custom/src/Swarm/FleetRegistry.md)／[HealthCenter](../custom/src/Swarm/FleetHealthCenter.md)：原生动态来源、FTC合并前检查与实际阻断。
- [主备能量账单](../custom/src/Swarm/FleetEnergyScheduler.md)和[发送前整组seal](../src/Vehicle/FleetStartupMemorySeal.md)。
- [清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标、约40%源码工作量粗估和未编译／测试边界保持；下方为历史。

## 459：同一原生运动模型进入实际阶段

- [Witness模型缓存](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)：完整原source12、同ID、getter原memoryCurrent，无续期。
- [Txn目标绑定](../custom/src/Swarm/FleetMissionStartupTransaction.md)：新请求一次prepare，资料路径比完整原source后持原Ptr。
- [Transition实际复用](../custom/src/Swarm/FleetMissionStartupTransition.md)：只取实际owner缓存，缺失不重造。
- [模型边界](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)／[Worker输入合同](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)。
- [准确清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标及约40%源码工作量粗估保持；完整域和最终许可未完成，未编译、运行、测试或打包。下方为历史。

## 458：共享实际运动模型与原审阅输入

- [实际地理运动模型](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)：不可变纯值对象、顺序地理链及原数值表达式。
- [实际阶段消费者](../custom/src/Swarm/FleetMissionStartupTransition.md)：同阶段一次准备、原有序GPI和累计航向复用。
- [Worker输入2](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)：必需row3完整来源、全计划payload配对及显式row2投影。
- [普通审阅](../custom/src/Swarm/FleetMissionStartReview.md)／[地面审阅](../custom/src/Swarm/FleetMissionArmedGroundReview.md)：同次capture／原完成READ，不重采替换。
- [补丁清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师后续验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

完整运动域净空与最终许可未完成，名义段报告不扩大scope；原25目标和约40%源码工作量粗估不变。未编译、运行、测试、更新PR或打包。下方为历史。

## 457：原包入口及完整阶段的整组memory栅

- [普通RAW原包入口绑定](../src/Vehicle/VehicleStartupRawIngress.md)：首回调前原scope／operation／时间及整栈depth，嵌套／丢弃不洗掉历史。
- [原生整组memory seal](../src/Vehicle/FleetStartupMemorySeal.md)：原typed账本、receipt、来源与自然时限，非JSON恢复的许可。
- [StartupTransaction](../custom/src/Swarm/FleetMissionStartupTransaction.md)：完整阶段检查后的同组候选、所有回调后提交与最终命令只读消费。
- [共享启动政策形状校验](../custom/src/Swarm/FleetMissionExecutionProfile.md)：直接复用原validator，不用虚拟完整profile。
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)／[限定审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)／[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原空间包含／最终StartPermit／正式结果仍未完成，Compiler pending保持。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 456：原生启动净空Witness及移交消费者

- [共享原生Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)：原typed材料、原Review时钟、共享UTC水位、source／memory分层，不可从JSON恢复。
- [普通审阅](../custom/src/Swarm/FleetMissionStartReview.md)／[地面审阅](../custom/src/Swarm/FleetMissionArmedGroundReview.md)：只从本次实际满足报告生成。
- [普通Authority](../custom/src/Swarm/FleetMissionStartupAuthority.md)／[地面Authority](../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)：同ptr准入与移交前后核验。
- [StartupTransaction](../custom/src/Swarm/FleetMissionStartupTransaction.md)：独立保源到既有末写切点，不借旧Review活性。
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)／[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)／[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

这不是完整运动连续性、最终StartPermit或可飞认证，原pending保持。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：同次启动报告的真实消费者

- [独立StartupClearanceWorker及exact8／5合同](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)。
- [完整Joint启动提取](../custom/src/Swarm/FleetMissionJointSpatialReview.md)、[共用严格报告来源匹配](../custom/src/Swarm/FleetMissionClearanceReview.md)。
- [普通审阅](../custom/src/Swarm/FleetMissionStartReview.md)、[地面续审](../custom/src/Swarm/FleetMissionArmedGroundReview.md)，以及原[Source](../custom/src/Swarm/FleetMissionClearanceSource.md)／[Datum](../custom/src/Swarm/FleetMissionClearanceDatum.md)的原生来源责任。
- [29路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原pending、Authority至最后write和完整许可仍待开发。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：明确工程高程关系，不从报文字段名推断

- [目录descriptor1／2与关系合同](../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)。
- [新Datum原生capture／recapture／memory尾核](../custom/src/Swarm/FleetMissionClearanceDatum.md)。
- [原Source18职责不变](../custom/src/Swarm/FleetMissionClearanceSource.md)、[原固定文件重载](../custom/src/Swarm/FleetFirmwareCatalogReload.md)。
- [普通审阅](../custom/src/Swarm/FleetMissionStartReview.md)、[地面新READ审阅](../custom/src/Swarm/FleetMissionArmedGroundReview.md)。
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原实际启动solver和最终许可仍未完成，原pending保留。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 原25目标继续，以下453为历史。

## 453：实际配置来源与资料窗口

- [共享实际配置提取／无物理认证边界](../custom/src/Swarm/FleetNativeConfigurationObservation.md)。
- [Registry私有来源绑定和本地基线](../custom/src/Swarm/FleetRegistry.md)。
- [净空来源七字段返回与稳定证据](../custom/src/Swarm/FleetMissionClearanceSource.md)。
- [普通审阅冻结](../custom/src/Swarm/FleetMissionStartReview.md)、[地面READ起点及持续复核](../custom/src/Swarm/FleetMissionArmedGroundReview.md)。
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原datum／实际启动净空与最终许可尚未完成，不解除pending。本批只优化地面站源码及交接材料；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余不缩减。 以下452为历史。

## 452：显式接地与双组件全扫掠

- [完整输入schema2](../custom/src/Swarm/FleetMissionClearanceInput.md)。
- [非对称偏移盒](../custom/src/Swarm/FleetMissionClearanceGeometry.md)。
- [原动作绑定、唯一投影与protected／support模型](../custom/src/Swarm/FleetMissionContactModel.md)。
- [报告3及新旧来源匹配](../custom/src/Swarm/FleetMissionClearanceReview.md)。
- [Compiler实际消费](../custom/src/Swarm/FleetMissionCompiler.md)、[Store完整归档](../custom/src/Swarm/FleetMissionPlanStore.md)、[Draft版本分层](../custom/src/Swarm/FleetMissionDraft.md)。
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅声明模型与源码，不认证真实触地、净空或许可。原25目标继续，未编译、运行、测试或放行；以下451为历史。

## 451：后台规划、取消和真实consumer

- [Worker输入、线程与清理](../custom/src/Swarm/FleetMissionPlanningWorker.md)。
- [Coordinator／Root、来源尾核及三入口](../custom/src/Swarm/FleetMissionReviewCoordinator.md)。
- [可取消编译](../custom/src/Swarm/FleetMissionCompiler.md)、[报告取消](../custom/src/Swarm/FleetMissionClearanceReview.md)、[几何取消](../custom/src/Swarm/FleetMissionClearanceGeometry.md)。
- [余段等待／取消](../custom/src/Swarm/FleetMissionRemainderPanel.md)。
- [准确修改范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师实际验证待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅源码／限定静态审阅，未编译、运行、测试、放行或打包；原25目标／六类实质代码仍继续。以下450为历史。

## 450：规划环境计算、独立basis和归档来源

- [完整段新scope入口](../custom/src/Swarm/FleetMissionJointSpatialReview.md)与[启动独立basis入口](../custom/src/Swarm/FleetMissionStartupSpatialReview.md)。
- [报告schema2及源匹配](../custom/src/Swarm/FleetMissionClearanceReview.md)。
- [Compiler真实调用](../custom/src/Swarm/FleetMissionCompiler.md)、[PlanStore一致性](../custom/src/Swarm/FleetMissionPlanStore.md)与[Coordinator投影](../custom/src/Swarm/FleetMissionReviewCoordinator.md)。
- [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批不解除原pending，不增加启动权限；未编译、测试或运行。下方449及更早为历史。

## 449：完整扫掠净空计算源码入口

- [精确整数几何／闭并集](../custom/src/Swarm/FleetMissionClearanceGeometry.md)。
- [声明模型净空报告](../custom/src/Swarm/FleetMissionClearanceReview.md)。
- [原工程输入合同](../custom/src/Swarm/FleetMissionClearanceInput.md)与[原联合段范围](../custom/src/Swarm/FleetMissionJointSpatialReview.md)。
- [准确修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师待执行说明](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅计算核心／独立报告，实际启动消费和接地政策未完成，pending不解除。未编译、测试、运行或放行；下方448及更早为历史。

## 448：净空工程资料和来源版本

- [输入schema1、毫米区域与逐机包络合同](../custom/src/Swarm/FleetMissionClearanceInput.md)。
- [Draft来源1／2、CAS与完整重建](../custom/src/Swarm/FleetMissionDraft.md)、[编译与pending阻断](../custom/src/Swarm/FleetMissionCompiler.md)、[准确归档校验](../custom/src/Swarm/FleetMissionPlanStore.md)。
- [历史草稿恢复](../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)与[失败部署修复复制](../custom/src/Swarm/FleetMissionRepairDraft.md)的新来源版本边界。
- [8源码／工程＋17文档清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师未来执行事项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

资料合法不代表当前场地、实际构型、净空或许可；新资料任务仍几何待审阻断。完整求解与实际启动消费、原25目标继续，未编译、测试或运行，下方447及更早为历史。

## 447：风险原因到实际审批／部署／启动审阅

- [风险汇总与共享严格检查](../custom/src/Swarm/FleetMissionRiskEvaluator.md)、[Coordinator消费](../custom/src/Swarm/FleetMissionReviewCoordinator.md)。
- [实际Deployment冻结／归档核对](../custom/src/Swarm/FleetMissionDeployment.md)、[普通StartReview](../custom/src/Swarm/FleetMissionStartReview.md)及[地面再审](../custom/src/Swarm/FleetMissionArmedGroundReview.md)。
- [6源码＋16文档清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师未执行清单](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

只说明实际原生门禁不再忽略业务阻断，合法空列表也不等于许可。完整净空／最终StartPermit和原25目标继续；未编译、测试、运行或放行，下方446及更早为历史。

## 446：高度关系门禁的精确数值比较

- [定长精确比较器](../custom/src/Swarm/FleetMissionAltitudeArithmetic.md)：原 binary64 位表示、36 个 uint32 limb、支持域和闭区间比较。
- [实际高度门禁与三个解析反例](../custom/src/Swarm/FleetMissionStartupAltitude.md)：原 A／L／R 高度、整数毫米参考及原始四项漂移，不用已舍入差值作门禁算术。
- [4 源码／登记＋15 文档的精确清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[工程师待执行验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

保留实际 binary64 配置容差及 `<=` 闭区间；真实零可满足零容差，非零残差不得因中间舍入变零而通过。不改 DTO、schema、权限或原基线；三个反例是未运行的数学推导。当前仅源码与静态核对，不是编译／测试通过或完整 StartPermit，未来执行验证由工程师负责，原25目标继续。下方445及更早为历史。

## 445：显式高度关系政策与启动scope连续性

- [纯关系契约](../custom/src/Swarm/FleetMissionStartupAltitude.md)、[执行配置v4](../custom/src/Swarm/FleetMissionExecutionProfile.md)及[策略表单](../custom/src/Swarm/FleetStartupPolicyEditor.md)。
- [实际高度账本／scope监视](../src/Vehicle/VehicleStartupAltitude.md)、[原生Transaction取源](../custom/src/Swarm/FleetMissionStartupTransaction.md)、[Readiness](../custom/src/Swarm/FleetMissionStartReadiness.md)和[Transition](../custom/src/Swarm/FleetMissionStartupTransition.md)。
- [普通审阅](../custom/src/Swarm/FleetMissionStartReview.md)、[初始Authority](../custom/src/Swarm/FleetMissionStartupAuthority.md)、[地面风险](../custom/src/Swarm/FleetMissionArmedGroundReview.md)与[地面Authority](../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)。
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

首次样本保留、当前值按原容差复核；scope内正常恢复不能洗掉已观察违规。不是内部参考／净空／最终许可认证，未编译、测试或运行，原25目标继续；下方444及更早为历史。

## 444：同包高度与冻结风险诊断

- [ALTITUDE纯解码](../src/Vehicle/FleetStartupAltitude.md)：decode18、六个row4、state8、原始float32与三个报告差值。
- [Vehicle高度账本](../src/Vehicle/VehicleStartupAltitude.md)：native27、实际来源、整栈隔离和接收／状态／时钟语义。
- [Readiness诊断](../custom/src/Swarm/FleetMissionStartReadiness.md)、[普通审阅](../custom/src/Swarm/FleetMissionStartReview.md)、[地面风险](../custom/src/Swarm/FleetMissionArmedGroundReview.md)、[初次Authority](../custom/src/Swarm/FleetMissionStartupAuthority.md)与[地面Authority](../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)。
- [精确清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

诊断不是实时仪表、传感样本鲜度或放行依据；未编译、测试或运行，原25目标继续。下方443及更早为历史。


## 443：原点声明与实际启动连续性

- [GPS_GLOBAL_ORIGIN纯解码](../src/Vehicle/FleetStartupGlobalOrigin.md)：decode18／reference8、MAV1／MAV2零扩展与完整时间文本。
- [Vehicle独立原点账本](../src/Vehicle/VehicleStartupGlobalOrigin.md)：native24、实际PRIMARY来源、整栈隔离和首次发送scope冻结。
- [稳定阶段参考](../custom/src/Swarm/FleetMissionStartupGlobalOrigin.md)、[Readiness](../custom/src/Swarm/FleetMissionStartReadiness.md)、[Transition](../custom/src/Swarm/FleetMissionStartupTransition.md)、[终态再审](../custom/src/Swarm/FleetMissionStartupTransaction.md)及[地面READ](../custom/src/Swarm/FleetMissionArmedGroundReview.md)。
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批不认证共同原点／内部有效分支／净空，不新增最终许可；只改源码和文档，未编译、测试或运行，原25目标继续。下方442及更早为历史。


## 442：联合名义空间报告

- [全任务／启动联合投影](../custom/src/Swarm/FleetMissionJointSpatialReview.md)：完整canonical请求链、独立假设、逐机覆盖及来源映射。
- [纯静态几何检查](../custom/src/Swarm/FleetAirspaceConflictChecker.md)：完整段、严格区域、原实际间距与不完整／截断边界。
- [同源捕获](../custom/src/Swarm/FleetMissionStartupWorkbench.md)、[空间入口](../custom/src/Swarm/FleetMissionStartupSpatialPanel.md)、[联合明细](../custom/src/Swarm/FleetMissionSpatialPanel.md)和[Coordinator](../custom/src/Swarm/FleetMissionReviewCoordinator.md)。
- [清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅源码与静态核对，未编译、测试或运行；不认证净空／时序，不释放指令，原25目标继续。下方441及更早为历史。

## 441：原始空域输入、同源归档与审批

- [全链合同](../custom/src/Swarm/FleetMissionAirspaceInputGate.md)：Root原值、原生快照、当前私有输入核对及阻断诊断边界。
- [Checker](../custom/src/Swarm/FleetAirspaceConflictChecker.md)：固定5字段检查、预算、坏间距与不完整报告。
- [Coordinator](../custom/src/Swarm/FleetMissionReviewCoordinator.md)：固定7字段来源检查、4字段快照及完整新审阅发布。
- [Intent](../custom/src/Swarm/FleetIntentTask.md)：私有参数审批门禁、修订号与同步通知重入。
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅源码及限定静态核对，未编译、测试或运行；原25目标保持，下方440及更早为历史。

## 440：完整任务与归档空域共同视图

- [完整任务投影](../custom/src/Swarm/FleetMissionSpatialReview.md)：全部成员、规划路线／返程、动作和原整组协议项。
- [完整任务明细与地图](../custom/src/Swarm/FleetMissionSpatialPanel.md)：逐机分页、来源区分与本地请求标记。
- [归档空域](../custom/src/Swarm/FleetAirspaceConflictChecker.md)：保原报告scope／实际阈值和未知覆盖，不重新认证。
- [归档输入形状](../custom/src/Swarm/FleetMissionPlanStore.md)：新四字段元数据，旧无字段记录保留未知。
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批未编译、测试或运行；下方439及更早为历史。

## 439：冻结启动空间

- [原生几何投影](../custom/src/Swarm/FleetMissionStartupSpatialReview.md)：完整目标、E7／AMSL、三种独立名义几何与未知项。
- [空间面板与地图](../custom/src/Swarm/FleetMissionStartupSpatialPanel.md)：明确选源、逐机查看、历史标签和地图隔离。
- [名义间距候选](../custom/src/Swarm/FleetAirspaceConflictChecker.md)：新静态入口、局部投影域与预算；旧check保持。
- [Root来源合同](../custom/src/Swarm/FleetMissionStartupWorkbench.md)、[Coordinator入口](../custom/src/Swarm/FleetMissionReviewCoordinator.md)、[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅源码与限定静态核对，未编译、运行、测试或放行；下方438及更早为历史。

## 438：结构化启动策略

- [完整政策表单](../custom/src/Swarm/FleetStartupPolicyEditor.md)：v1／v2／v3原字段、加载、版本变化与明确采用。
- [父执行配置编辑](../custom/src/Swarm/FleetExecutionProfileEditor.md)：唯一规范文本、待处理输入、真实草稿绑定及保存CAS。
- [原生profile契约](../custom/src/Swarm/FleetMissionExecutionProfile.md)：原范围与校验链保持。
- [范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅源码／限定静态核对，未编译、运行、测试或放行；下方437及更早为历史。

## 437：实际启动工作台

- [当前部署启动工作流](../custom/src/Swarm/FleetMissionStartupWorkbench.md)：来源、20个单步动作、提交事实和取消语义。
- [启动操作界面](../custom/src/Swarm/FleetMissionStartupPanel.md)：初次／地面再审、确认和分页查看。
- [地面新风险展示](../custom/src/Swarm/FleetMissionArmedGroundReview.md)：只读有界风险投影，不暴露凭据或载荷。
- [本批范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅源码接线；未编译、运行、测试或放行，下方436及更早为历史。

## 436：实际Run简报交接

- [取源、原生身份与明确确认工作流](../custom/src/Swarm/FleetMissionHandoffWorkflow.md)
- [有限简报结构与内容校验](../custom/src/Swarm/FleetMissionExecutionReport.md)
- [Manager分页、容量和导入来源](../custom/src/Swarm/MissionHandoffManager.md)
- [交接面板及历史查看](../custom/src/Swarm/FleetHandoffPanel.md)
- [精确修改路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待执行项目](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

仅源码接线，不是跨站控制权移交或运行验证。下方435及更早为历史。

## 435：余段新记录地图与逐项核对

- [共享只读投影、索引和编码边界](../custom/src/Swarm/FleetMissionRemainderView.md)
- [分页核对区及地图交互说明](../custom/src/Swarm/FleetMissionRemainderRoutePanel.md)
- [新记录采用／审批工作流](../custom/src/Swarm/FleetMissionRemainderPanel.md)
- [精确修改路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待执行验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

仅源码阶段，未编译／运行／测试；不新增飞行授权，下方为历史记录。

## 434：新余段记录的采用、批准与共用地面部署

- [新审阅上下文、明确采用与退出](../custom/src/Swarm/FleetMissionRemainderPanel.md)
- [唯一部署面板的来源与刷新范围](../custom/src/Swarm/FleetMissionDeployment.md)
- [17路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证项目](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

仅源码和静态检查，不是运行验收或新的安装包；旧Run审批／占用不因新提案改变，完整25目标继续。下方433及更早为历史。

## 433：从实际Run生成独立余段重做提案

- [实际原任务取源与身份边界](../custom/src/Swarm/FleetMissionRemainderSource.md)
- [纯值余段构建、政策与来源摘要](../custom/src/Swarm/FleetMissionRemainderPlanner.md)
- [捕获、预览、确认审阅、退出入口](../custom/src/Swarm/FleetMissionRemainderPanel.md)
- [Compiler一致性](../custom/src/Swarm/FleetMissionCompiler.md)、[Coordinator](../custom/src/Swarm/FleetMissionReviewCoordinator.md)、[PlanStore归档](../custom/src/Swarm/FleetMissionPlanStore.md)
- [完整修改路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师验证待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

这些是源码与交接文档，不是运行/测试证明、自动接替或已编译软件。下方432及更早为历史。

## 432：备用待命与接替路线能量预算

- [严格策略、独立几何与29字段结果](../custom/src/Swarm/FleetReserveEnergyPlan.md)
- [草稿保存／清除与旧编辑源兼容](../custom/src/Swarm/FleetMissionDraft.md)
- [编译成员、备用路径和总量边界](../custom/src/Swarm/FleetMissionCompiler.md)
- [共同下限与独立待命开销](../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [未知备用账单与原成员资格](../custom/src/Swarm/FleetEnergyScheduler.md)
- [备用编辑／取消／清除与自身端点](../custom/src/Swarm/FleetReserveEnergyPlanPanel.md)
- [逐机预算分项展示](../custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)
- [29路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

仅源码和静态核对；无编译、测试、运行或放行。原25目标继续。

## 431：实际电池报告与动态预算时效

- [原始收包、20／15字段合同与接收年龄](../custom/src/Swarm/FleetNativeBatteryObservation.md)
- [实际Swarm绑定与完整消费路径](../custom/src/Swarm/FleetNativeBatteryBridge.md)
- [Registry动态快照与护照隔离](../custom/src/Swarm/FleetRegistry.md)
- [能量预算和原生报告边界](../custom/src/Swarm/FleetEnergyScheduler.md)
- [只读来源展示与审阅时效](../custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

仅源码阶段；飞控上报估算不等于实测续航，未编译、测试或运行验收。

## 430：原成员逐机能量账单

- [预算推导与真实编译成员绑定](../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [能量账单、百分比公式与未知口径](../custom/src/Swarm/FleetEnergyScheduler.md)
- [风险消费者的主／备用身份核对](../custom/src/Swarm/FleetMissionRiskEvaluator.md)
- [规划页只读分页明细](../custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

仅源码阶段；真实电池模型与完整25目标仍未完成，未编译或运行验收。

## 429：实际运行复盘报告

- [报告数据、边界和文件接口](../custom/src/Swarm/FleetMissionExecutionReport.md)：与[飞前计划报告](../custom/src/Swarm/FleetMissionReportBuilder.md)分开，保留未知与候选语义。
- [报告界面和冻结操作](../custom/src/Swarm/FleetMissionExecutionReportPanel.md)：生成、再次确认保存、外部只读打开，不恢复飞行权限。
- [变更路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证项目](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 428：从本地历史版本恢复规划草稿

- [版本目录与精确取源](../custom/src/Swarm/FleetMissionPlanStore.md)：每个提案会话最多128个明确历史版本，不用revision=0替代已选项。
- [历史规划操作界面](../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)：读取来源、明确恢复、保留当前草稿及重新审阅。
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅恢复规划编辑内容，不恢复账号、审批、部署或执行。原25目标继续，未编译或运行。下方为历史。

## 427：完整原草稿与人工修复副本

- [完整编辑源与一次性导入](../custom/src/Swarm/FleetMissionDraft.md)、[编译对应关系](../custom/src/Swarm/FleetMissionCompiler.md)、[源归档](../custom/src/Swarm/FleetMissionPlanStore.md)。
- [实际失败部署取源](../custom/src/Swarm/FleetMissionRepairDraft.md)、[创建副本与切回界面](../custom/src/Swarm/FleetMissionRepairDraftPanel.md)。
- [25路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证项目](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅本地编辑副本，不是自动机载修复；旧记录无源时拒绝，不从生成路线猜回区域。原25目标保持未完成。下方为历史。

## 426：失败部署的独立整组核对

- [原生工作流](../custom/src/Swarm/FleetMissionReconciliation.md)：原失败／取消部署、逐机三态结果与排空。
- [独立RAW INT传输](../custom/src/Swarm/FleetMissionReconciliationTransport.md)：真实COUNT0、一次性结果，不污染普通缓存和Start收据。
- [只读操作界面](../custom/src/Swarm/FleetMissionReconciliationPanel.md)及[角色政策](../custom/src/Swarm/FleetRolePolicy.md)：明确确认、独立权限、取消与来源摘要。
- [本批清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师待验证事项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅源码阶段，未编译或运行；不是自动修复、启动许可或完整25目标交付。以下为历史入口。

## 425：部署回读差异诊断

- [纯值比较与有界摘要](../custom/src/Swarm/FleetMissionReadbackDiff.md)：规范化比较、原始字段诊断与未知分开。
- [实际回读接线](../custom/src/Swarm/FleetMissionDeployment.md)及[传输来源边界](../custom/src/Swarm/FleetMissionDeploymentTransport.md)：保留原部署与操作身份，不改变发送许可。
- [逐机差异展示](../custom/src/Swarm/FleetMissionReadbackDiagnosticsPanel.md)：摘要、按需明细及失败后的历史解释。
- [修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师验证清单](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅源码阶段；失败后的整组独立重新对账和修复草稿仍待开发，不是起飞许可或完整25项目标完成。以下为历史入口。

## 424：跨日志库的有界历史检索

[归档面板](../custom/src/Swarm/FleetArchiveHistoryPanel.md)增加独立检索入口；范围为本机诊断事件库的全部封存段，或封存段加固定的已停止录制未封存库。按机号与四种工程审批操作查询，不再仅筛选当前展示页。选择包含未封存库时，文件缺失属于失败，不静默缩小范围；查询不会自动停止录制。

每批最多读取4页、消费256行、返回64条匹配，并有字节上限；续扫游标来自原生实际结果，空匹配不等于扫描完。界面区分本批计数、已扫完文件和所选范围完成，保留来源文件、退休记录与异常退出等覆盖信息；不自动累积无限结果。完整合同见[Reader](../custom/src/Swarm/FleetEventArchiveHistoryReader.md)和[Store](../custom/src/Swarm/FleetPersistentEventStore.md)。

目录清单元数据在批次前后及续扫时核对，当前正在分页的文件另绑定实际epoch和整文件指纹；这不是跨文件原子快照、已读文件的持续哈希复核或全部飞行历史完整性证明。历史不恢复账号、批准、控制权或飞行权限。旧v1未记录的审批操作不会推测补齐。

本批源码与待验证范围见[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／SQL／UI／应用／设备，未修改PX4、刷机、更新PR或打包。以下423及更早为历史；本批部分补足其“全库检索”缺口，但不完成永久全集索引、遥测全集、留存治理或恢复工具；原25目标和六类实质开发继续。


## 423：停止录制后的未封存库离线查询

[归档面板](../custom/src/Swarm/FleetArchiveHistoryPanel.md)新增明确的离线读取入口，复用原[读取线程](../custom/src/Swarm/FleetEventArchiveHistoryReader.md)，只读固定本机未封存的 fleet-events-v1.sqlite。无需先恢复录制、连接飞机或登录飞行账号；本实例录制线程必须已结束，不能用点击停止或captureEnabled=false代替实际退出。

数据库以只读方式打开，不创建、修复、迁移或删除库；读取期间使用与录制器共用的协作锁，因此会创建／释放临时锁文件，不宣称完全零文件写入。遗留journal/wal/shm、坏库、锁冲突或身份变化均拒绝，不自动清理。每页64行，后页绑定原实际库epoch与整文件指纹；两页之间库变更须重新读取第一页。

该入口沿用历史只读边界，不恢复账号、能力批准、任务或飞行权限；异常退出、退休行及丢失尾部信息保留，查询成功不等于历史完整。下方422的“离线只支持封存段”由本批有条件的未封存读取补充；全库搜索、完整构型历史、恢复工具和长期留存仍未完成。

本批范围见[修改清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，运行待办见[工程师交接](../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码及静态文本核对，未编译、测试、执行helper／SQL／UI／应用／仿真／设备，未改PX4、刷机、更新PR或打包。原25目标及六类实质开发继续，不能据本批声称整体落地或承诺完成日期。

## 422：显式本地归档与工程审批摘要（源码阶段）

[归档入口](../custom/src/Swarm/FleetArchiveHistoryPanel.md)接入指挥中心与任务规划页：默认关闭，明确开始／停止／封存并继续录制，查询当前录制批次或已封存段，按原生游标分页和取消。复用原 Store、BlackBox、Worker 与 Reader，不新增数据库，不依赖飞机在线或飞行账号；只读历史不会恢复权限或触发飞行命令。

四种能力／基线操作新增独立的[有限工程摘要](../custom/src/Swarm/FleetEventArchiveCodec.md)：操作、机号、提交与结果三态、指纹和差异计数。不保存账号、自由理由或原始构型；内存接纳不等于 SQL 提交，原审计链追加也不等于持久化。旧 v1 未记录的详情不能补回，摘要失败不改变已发生的登记提交。

本批11源码／资源、16文档见[清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，待执行验证见[工程师交接](../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅静态阅读与限定文本检查，未编译、测试、运行 helper／UI／认证／SQL／应用、修改 PX4、刷机、更新 PR 或打包。

仍缺停止后未封存活动库的离线读取、全库检索、完整构型历史及长期留存管理；本页筛选仅当前页，封存数量不是总磁盘硬配额。原25目标与完整受控执行、可信性能、自愈、企业跨站、长期运营、真实模型／生态六类工作保持未完成，不能按阶段号计算完成率或可靠工期。下方421及更早为历史，“只有值／无归档UI”仅由本批固定本机入口部分覆盖。

## 421：构型基线批准／撤销接入实际操作链

[基线合同](../custom/src/Swarm/FleetConfigurationBaselineEvidence.md)、[原生工作流](../custom/src/Swarm/FleetCapabilityReview.md)和[共用面板](../custom/src/Swarm/FleetCapabilityReviewPanel.md)已接四种独立操作。维护/管理员申请、不同合格账号审批；能力采纳与构型基线使用不同精确用途，不互借凭据。首次完整配置无旧基线也可批准；当前配置未知时，只能撤销真实存在的本地批准，不能建立新基线。

重复批准同一基线也退役旧能力批准，须另行审阅能力。原生先冻结真实提交回执再通知，后置失效或审计失败不抹提交事实。修复界面对合法旧记录大写哈希/首尾空白的误拒；原记录不改写。完整有类型基线仍仅在本进程保存，不认证硬件、改装或性能。

本批9源码、17文档见[精确清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，待执行项目见[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码与限定静态审阅，未编译、测试、运行UI/认证/helper、修改PX4、刷机、更新PR或打包。原25目标/六类仍有实质开发，不能据此计整体验收或完成率；下方420及更早“无基线批准界面”为历史状态。

## 420：完整有类型基线接入任务消费者

[构型基线合同](../custom/src/Swarm/FleetConfigurationBaselineEvidence.md)将实际 Registry 当前完整配置与本机批准快照统一接到 Matcher、RoleAllocator、Compiler 和 Continuity。旧 configurationState 只保留哈希级报告语义，不再单独满足 requireConfigurationBaseline 或获得基线排序优待。

新观察身份进入 Guided 旧/新机队指纹和 Coordinator schema2 审阅归档；Deployment 核归档与实际审阅一致，仍须当前来源检查。hash-only、未知或导入旧批准不能通过；恢复原配置可以重新审阅，但不能复活旧批准。可选基线政策不变成所有任务的强制门槛。

本批12源码、20文档见[精确清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。没有新增基线批准界面/持久化或硬件、性能认证；原25目标/六类仍有实质开发。仅源码和限定静态检查，未编译、测试、运行、修改PX4、刷机、更新PR或打包。下方419及更早为历史，其“任务仍仅按旧标签判断”由本批限定消费路径覆盖。

## 419：构型快照和字段差异已接源码，未运行验收

[构型比较](../custom/src/Swarm/FleetConfigurationDiff.md)将实际Registry的本机完整批准快照或前次不同完整观察接入原双账号能力审阅。界面区分来源、类型、Missing/null和截断；缺前/后值不报告无变化。只按显式非空map视作完整上报，不认证每机型字段完整、实物或性能。preview独立升schema2/18键，原context6/approval14不变。

typed变化与完整观察丢失/恢复会退役旧能力批准；审批-only导入同样退役触达记录。新构型基线批准要求当前完整观察，不能批准hash-only之后残留的旧map。普通健康/重复同配置不滚动历史；私有历史仅本进程，无新增持久化或基线批准UI。

本批8源码/工程登记、15文档见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。原25目标/六类仍继续；下一源码切点是统一可信基线证据与Matcher/RoleAllocator/Compiler消费。旧configurationState/requireConfigurationBaseline仍为哈希级，不能按本批差异展示宣布完整任务基线门禁完成。没有编译、测试、运行helper/UI/认证/应用/设备、修改PX4、刷机、更新PR或打包。下方418及更早是历史。

## 418：单机本地能力审查

- [原生双账号工作流、值合同与限制](../custom/src/Swarm/FleetCapabilityReview.md)
- [面板与Root桥、取消/超时/提交事实](../custom/src/Swarm/FleetCapabilityReviewPanel.md)
- [原context6/approval14与下游消费](../custom/src/Swarm/FleetCapabilityEvidence.md)
- [16源码/资源及17文档精确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [工程师待执行矩阵](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [限定静态审阅](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)

上述仅源码状态，未编译、测试、运行、刷机或放行；不是全部25目标完成。

## 417：固定文件工程声明重载与交接

- [目录声明、纯预览与带围栏导入源码](../custom/src/Swarm/FleetFirmwareAdapterCatalog.h)
- [实际 Coordinator 拥有的运行中重载工作流](../custom/src/Swarm/FleetFirmwareCatalogReload.h)
- [完整旧/新声明、双入口面板源码](../custom/res/Merivus/FleetFirmwareCatalogPanel.qml)
- [现有全球任务估计器参考合同](../custom/src/Swarm/FleetMissionStartupEstimator.md)
- [19个源码/资源路径与累计工程交接范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [工程师待执行矩阵](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [限定交叉阅读与未执行事项](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)

运行中固定文件只读、两账号一用审批后替换的是内存工程目录，不是固件刷写/签名或飞行许可；原启动自动 load 不在双账号改造范围。旧任务 review 失效，不停止 Run/清占用。原25目标/六类继续，未编译、测试、运行或打包/更新 PR；下方旧阶段保留历史。

## 416：逐机执行配置与真实参数绑定

- [Draft 完整逐机 profile、revision CAS 与生命周期](../custom/src/Swarm/FleetMissionDraft.md)
- [ExecutionProfile schema 与高级 startup JSON 1–3](../custom/src/Swarm/FleetMissionExecutionProfile.md)
- [只读12参数观察、19键 DTO 与稳定指纹](../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [操作员编辑、重读、旧审阅失效与后续流程](../custom/src/Swarm/FleetMissionOperatorWorkflow.md)
- [本阶段源码/资源及文档同步清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [工程师待执行验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

阶段名称仅416；累计源码不是安装包，没有编译、测试、运行或放行。原25目标/六类继续，以下旧批次为历史。

## 411–415：恢复复核入口与交接索引

- [恢复界面与Root合同、工程师待验证矩阵](../custom/src/Swarm/FleetOccupancyRecoveryPanel.md)
- [原生恢复复核合同（含当前Root模式覆盖）](../custom/src/Swarm/FleetOccupancyRecovery.md)
- [9源码/资源与18文档精确清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [工程师接手顺序及未执行验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)

源码入口是指挥中心和FlyView地图内区域任务规划页，不是独立安装包，也不是完整25目标完成。未编译、测试、运行或放行；下方早期无UI描述为历史。

## 406–410：原生重启／历史占用专用复核

[恢复复核合同](../custom/src/Swarm/FleetOccupancyRecovery.md)新增实际Swarm唯一拥有的原生工作流，公开只读对象和request/独立审批/取消入口；未加QML操作界面。bootstrap历史待审与本轮真实unknown分集合，整组最多32个选中地址只可解除restart/history holds，真实unknown、实际owner、任务、临时任务、隔离和everObserved历史不清除。

原Vehicle/MM/参数/VLM/MVM/model及主链路共享所有者冻结，新鲜2秒内原始HB未解锁与已知ON_GROUND、完整本机队列排空进入30秒复核。专用fleet.occupancy.recover.approve强制不同真实账号，一次消费不生成lease；原账号TTL、当前Episode/角色在最终无回调尾栅再核。现场处置编号和地面检查是明确人工声明，不认证物理停机、远端无任务或控权。

所有可回调读取后，整组再以纯native原RAW语义revision/hash/value/鲜度/队列核对，并核原worker、实际占用和任务map；同栈整体移除本地待审后bump，新actual身份须由原worker实际提交才恢复既有continuity getter/CAS。留存原QQ、源epoch与primary控制块，换源在getter/最终CAS即时重加hold，不只等timer；正常收包、合法起飞和新MM工作不冒充换源。

本批12源码/工程登记、17文档，共29路径。原生recoveryWorkflowImplemented与UI集成分开；localHoldsReleased仅历史动作事实，selectedReviewRequiredVehicleIds另报当前待审。仍缺UI、离线/报废现实处置、本轮真实unknown受控解除和完整机载自愈；完整25目标/六类实质开发保持。未编译、测试、运行helper/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方401–405及更早是历史状态，不以旧“无任何再审入口”覆盖本批native入口，也不宣称全部恢复产品已完成。

## 401–405：原生异步占用留存与重启待审

[占用留存合同](../custom/src/Swarm/FleetOccupancyJournal.md)已把严格11键Codec和单槽合并worker接到实际Swarm构造/原生timer，不依赖无人调用的save/load或QML可写路径。固定AppData目录、本地合作锁、原文件基线/目录身份前后核验、无直接覆盖回退的原子替换及读回，与当前实际源身份分开；不是完整捕获、write-ahead或断电durable证明。

每个新Swarm先为1–255协议地址建立独立restart-review holds。历史ID只并入真实未知，不重建owner、任务、Run、账号或权限；空/缺失/旧open文件和写入成功都不能解除这些holds。纯重启占位不冒充已观察到的飞机记录。当前尚无专用再审解除流程，所以本阶段连续性改派保持待再审限制，不能称恢复后已可调度；此流程仍须继续开发。

公开native占用快照必须等原worker bootstrap、当前完整身份已实际提交并读回，且actual==offered==committed；存储失败/失源/坏文件/代次耗尽拒连续性提交。磁盘IO只在worker线程，原Land/RTL/ABORT不等IO、不因账本失败新增否决；析构只协作stop，不写clean或释放飞行占用。只读状态标明last-poll和recoveryWorkflowImplemented=false。

本批7源码/工程登记、15文档，共22路径；不是全部25产品目标、安装包或完整交付。QSaveFile结果仅记atomicReplacementConfirmed，osSyncVerified/durableVerified始终false。未知授权解决、正式受控执行/StartPermit、可信性能、企业跨站、长期运营及真实模型/生态等六类仍有实质源码工作。仅源码和限定文本读回；未编译、测试、运行解析/helper/业务/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方396–400及更早为历史，不覆盖本节新的重启待审门禁。

## 396–400：实际本地占用进入连续性审阅与提交

[原生占用合同](../custom/src/Swarm/FleetNativeOccupancyEvidence.md)新增actual Swarm快照和最终本地角色CAS守门。部署撤权不删除raw占用；编队、创建/预抢占栈、活事务、安全owner、临时任务/清理及内容隔离分别保留。已知结果不明不会因原记录清理变成可用；当前账本没有自动清除或跨重启恢复证明。

Continuity排除所有held/unknown候选；原成员任一实际占用即整任务hold，不通过换掉成员来假装释放。Guided刷新/应用要求原实际快照，稳定原生身份进入建议；缺失或坏证据不能产生产品可应用的完整提案。旧孤立值诊断仍明确requiresNativeOccupancyReview，不授来源认证。

Swarm与Orchestrator双向sticky pin，源死不换空实例。新六参数角色CAS在同栈提交前核冻结身份与old∪new实际状态；绑定后的旧本地入口也不绕过当前占用检查。不改实际owner、不发命令、不解除native Run保护；只代表本地模型变更，不是飞机空闲、正式机载改派或控制租约。

本批12源码/工程登记触点、15份文档，是累计工程增量，不是完整25目标完成或安装包。包括直接相关的CommandTransaction通知后存活/原尝试检查及嵌套start/retry保护，不是全仓回调安全认证。未知状态的授权解决/跨重启留存、完整受控机载执行、性能/能量与企业跨站等六类仍继续。仅源码与限定文本审阅，未编译、测试、解析/运行helper、业务/认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅摘要占用/五参产品应用”的描述由本节覆盖。

当前入口：[实际本地占用](../custom/src/Swarm/FleetNativeOccupancyEvidence.md)、[连续性快照消费](../custom/src/Swarm/FleetMissionContinuityPlanner.md)、[绑定及六参CAS](../custom/src/Swarm/SwarmMissionOrchestrator.md)、[纯一对一匹配](../custom/src/Swarm/FleetReplacementMatcher.md)。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](../custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

当前入口：[匹配器](../custom/src/Swarm/FleetReplacementMatcher.md)、[角色备用](../custom/src/Swarm/FleetRoleAllocator.md)、[整组连续性提案](../custom/src/Swarm/FleetMissionContinuityPlanner.md)、[本地角色CAS](../custom/src/Swarm/SwarmMissionOrchestrator.md)。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](../custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

当前能力链入口：[本地工程能力审阅](../custom/src/Swarm/FleetCapabilityEvidence.md)、[能力发现](../custom/src/Swarm/FleetCapabilityDiscovery.md)、[角色建议](../custom/src/Swarm/FleetRoleAllocator.md)、[最终编译门禁](../custom/src/Swarm/FleetMissionCompiler.md)、[连续性建议](../custom/src/Swarm/FleetMissionContinuityPlanner.md)、[审阅证据归档](../custom/src/Swarm/FleetMissionReviewCoordinator.md)。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

## 371–375：下一部署组与历史监视选择

最新入口：[本地部署组面板](../custom/src/Swarm/FleetMissionLifecyclePanel.md)、[Lifecycle 原生切换](../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)、[控制器作用域](../custom/src/Swarm/FleetMissionControllerScope.md)、[操作工作流](../custom/src/Swarm/FleetMissionOperatorWorkflow.md)和[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批8源码/19文档，新增两个 ForController native begin → 私有 permit mutation → 新 scope/原 ID end、两个固定 Root public API、12键 metadata/6键组行，以及指挥中心和规划页两个双步确认入口；raw refs 不出值 DTO。

prepare 不自动切 selected、登录/授予批准、停止旧 Run 或释放占用；已提交事实不因 end/后置核验失败被抹掉。失败分配尝试仍计8组上限，allocated=false 不证明未计费。`outboundIntegrationComplete=false`、完整25目标/六类缺口保留；限定文本检查记录已同步本轮静态审计，未编译、测试或运行，未更新 PR/包。以下366–370及更早索引是历史增量，旧未实现入口的说明由本节覆盖。

## 366–370：当前组接线与观察收据路线

本批入口见[控制器作用域](../custom/src/Swarm/FleetMissionControllerScope.md)、[Lifecycle](../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)、[操作工作流](../custom/src/Swarm/FleetMissionOperatorWorkflow.md)、[监视面板](../custom/src/Swarm/FleetMissionObservationPanel.md)及[原生回调](../custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md)。涵盖5源码/18文档、三 native call、两类注册、私有14引用/30入口/24回调、同 selected scope 的四值/token 和原 retained Close 完整收据；没有公开 raw refs 或 legacy fallback，构造 initial 不自动再 prepare。

新部署/选择产品入口及专用 transition API 未实现，`outboundIntegrationComplete=false`；完整25目标/六类仍有源码缺口。已完成限定文本审阅，具体证据见本轮静态审计，不是运行或安全通过；未编译、测试、运行、更新PR或打包。下方保留历史批次；旧未注册/未迁移状态不覆盖本批。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](../custom/src/Swarm/FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](../custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](../custom/src/Swarm/FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

## 346–350：十二类原生服务门禁接线

[Episode服务接线](../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)已覆盖另外12类服务的新请求、live依据、审批/身份移交及现有scoped发送fence：归组来源须仍为current且owned peer属于同一实际组；旧取消/revoke/drain、被动Run/Close和真实已queue记账不借此封禁。Authority/Txn认证移交上下文改为纯本地比对，完整身份/风险门禁仍在调用两侧；Series保留严格一次未发布构造绑定。

仍未形成新部署可用闭环：普通Deployment上传/回读的legacy PlanManager在插件后、异步每包/重试没有独立Episode fence；Guided actual Episode/epoch、共享Role身份上下文和selected旧观察整组投影尚未接入，无新QML注册/部署入口。只改源码及限定静态审阅，不编译、测试、运行认证/应用/设备、更新PR或打包；原25目标/六类继续。


## 345：非 Close 用途的原生身份门禁

[Session 身份接线](../custom/src/Swarm/FleetOperatorSession.md)已加入上传/启动/ground用途的current Episode检查、实际owner配对及OS/owner回调后的再次核对；旧组普通身份事实、显式登录/退出、精确撤销和独立Close用途保留。相同账号/UUID或不同工厂的当前对象不能替代同一实际组。

这是340–344核心后的源码增量，不是全链隔离完成：其余owned发送/审批入口、Guided actual Episode/epoch和selected旧观察投影仍未完整接线；无QML注册/新部署入口，现有应用仍单链。只做源码与限定静态审阅，不编译、测试或运行认证/应用/设备，未更新PR或包，原25目标/六类继续。


## 340–344：新部署生命周期原生核心

- [实际 Episode 与 Lifecycle](../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)：14个全新 owned 对象、signed-out Session、保留旧 Run/占用、五个实际共享来源、全部 retained 原生静默证据及8次分配上限（含初始/失败尝试）。
- [本批补丁清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)：本轮23路径/18文档限定文本检查已记录，不是测试通过记录。

本批只登记原生新4文件；**未注册 QML、未替换 Guided、无新部署入口，应用仍单链**。sticky outbound helper 不等于 owned 门禁、QML epoch/source callback 或历史 Close 身份已接线。原25目标/六类不缩减；未编译、测试、运行业务/认证/应用/设备、更新 PR 或打包。

## 335–339 当前入口

- [运行监视面板与操作契约](../custom/src/Swarm/FleetMissionObservationPanel.md)：两个入口、完整review token、三项确认、默认取消/键盘、原生期限与三态结果。
- [原生关闭工作流](../custom/src/Swarm/FleetMissionObservationClose.md)：实际owner/完整context18、不同账号purpose与一次原生commit，权限不由UI决定。
- [修改清单](../MODIFIED_FILES.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批没有运行UI/认证、编译/测试或设备操作，原25目标/六类继续。

## 330–334 当前入口

- [独立监视结束工作流](../custom/src/Swarm/FleetMissionObservationClose.md)：context18、真实map占用、三项确认及原30秒双账号审批；无新UI，不是STOP/LAND/DISARM/Completed。
- [专用系统身份消费](../custom/src/Swarm/FleetOperatorSession.md)和[本地角色动作](../custom/src/Swarm/FleetRolePolicy.md)：不同用途、当前实际两账号与一次消费，非企业/跨站身份系统。
- [本批修改清单](../MODIFIED_FILES.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码/限定静态审阅，未编译、测试、运行认证/业务/helper或操作设备；原25目标/六类仍继续。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](../custom/src/Swarm/SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

320–324（2026-10-15）新增[显式 HOME 观测过渡](../custom/src/Swarm/FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

315–319（2026-10-15）新增[本次启动的连续运行观测](../custom/src/Swarm/FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

310–314（2026-10-15）新增[错误可区分的原生目录清点](../custom/src/Swarm/FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

305–309 新增[遥测只读历史检索](../custom/src/Swarm/FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

300–304 新增[原生入站遥测记录器](../custom/src/Swarm/FleetTelemetryArchive.md)：独立Vehicle pre-plugin被动入口、实际PRIMARY/Vehicle/observer绑定与退休、27消息固定Codec、parentless有界NDJSON worker及native确认/公开只读值已接线。新增默认关闭，不改旧.tlog，不请求数据流或控制；定位类另明确确认，原len payload与RX序号只观察，签名/CRC/身份不认证。文件NewOnly不覆盖或删除，入队/write/flush/durable分层，源失效停止整组并保留unknown尾部；fullTelemetryCaptured始终false，不把此子集当完整遥测或任务许可。运行/编译/测试/解析helper/设备均未执行，未改PX4/刷机/更新PR/打包；原25目标/六类不缩减。下方295–299及更早为历史批次。

295–299 新增[跨封存段只读浏览](../custom/src/Swarm/FleetPersistentEventStore.md)：Sql唯一schema/身份/严格页抽取、parentless单请求Reader与Store native目录/分页/取消及public值facade已接。actual目录条目生成本catalogId，页域绑定文件/DBepoch/sealRun/filter，最多128个每段单filter游标，只0或实际next；当前页/Reader共单槽，取消与错误不提前复用，历史不回灌rules/Replay/任务/权限。目录128段/4096条目与JSON页有界，合法1MiB政策的内部预留预算已修；目录全集、全库记录hash、跨段原子快照/飞行序都不宣称已验证。仍缺全集索引/自动归档清理、完整遥测/运营集成及最终StartPermit等原六类/25目标；未编译/测试、运行SQL/目录或hash helper、改飞控/刷机、更新PR或打包。下方290–294及更早说明为历史批次。

290–294 新增[显式维护封存轮换](../custom/src/Swarm/FleetPersistentEventStore.md)：实际worker协作锁贯穿排空/SQL关闭与同目录旧库保留，native确认后才请求，不自动删旧段；actual clean/sealed/finished、精确DBepoch/文件名和同源QQ/epoch核对后才沿本次明确配置建立新run。取消/退出/源丢失只退休续建，不抹掉可能已发生的封存事实；暂停序号窗口显式known/unknown及缺口摘要，不称连续无损。封存段数有界、未知文件不覆盖，hash/协作锁不认证或保证恶意并发/失电安全。仍缺跨段查询/自动归档清理、全遥测/运营集成及最终StartPermit等原六类/25完整目标；未编译/测试/运行库或锁/文件轮换、执行SQL、改飞控/刷机、更新PR或打包。下方285–289及更早记录为历史批次。

285–289 新增[默认关闭的连续诊断归档](../custom/src/Swarm/FleetPersistentEventStore.md)：实际BlackBox暂时收据/同类型事件身份、固定摘要Codec、专属QtSql线程与Store门面已接pri/type/controller；仅native显式配置/确认后异步开库，默认不读取或写盘，不回录开库前历史。queue+in-flight/批次/主库页/历史结果有界，实际commit后才durable，来源/投影/入队缺口保留诊断；历史单槽直到GUI交付，不进入实时回放、规则、机队、审批或控制链。无原payload/凭据/精确GPS，哈希非签名/匿名化；clean非完整性认证，未知库与需hot-journal恢复库拒绝不重建。仍缺全遥测/长期轮换/运营集成及最终StartPermit等原六类/25完整目标；未编译/运行/测试、建库/SQL执行、改飞控、刷机、更新PR或打包。下方280–284及更早批次为历史记录。

280–284 新增[显式局部运动与航向观察政策](../custom/src/Swarm/FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

275–279 新增[被动 ODOMETRY 原生账本与契约](../src/Vehicle/FleetStartupOdometry.md)：原始 MAV2 解码50/reference42，Vehicle full27/reference21；实际 PRIMARY/PX4/版本与飞行来源、接收整栈、prepare前计数和token/epoch防嵌套覆盖。sticky everObservedRawFrame 不把曾断流/无效/被过滤的源当初始缺流；Readiness仅存去age的20字段参考投影，普通收包/时间/运动数字/quality分值不造成身份抖动，reset/frame/状态与已观察ABA撤销旧依据；地面READ冻结该投影，Transition前后重核实际来源。稀疏协方差NaN保留未知，扩展尾零/LOCAL_FRD/时钟不认证参考有效性或同采样。不增加传感器、不请求/启用流、不改原六通道/read34/startup32、不放宽HOME/local严格政策。源码接线与有限静态审阅，不是完整运动/转向/净空/最终StartPermit或执行完成；原25目标/六类继续，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方270–274及更早批次是历史记录。

270–274 新增[有界实际启动事务序列](../custom/src/Swarm/FleetMissionStartupSeries.md)及多轮地面再审后端：最新已初始化owner不回退，逐轮冻结actual QQ/UUID/epoch/generation/hash，旧转交链精确退休后重新完整读取/已阅/两账号审批；每次新消费者保留前代，初始/终态对象不能重用覆盖。native固定历史摘要6防递归复制，完整当轮任务/新风险/真实queued事实仍保留；同一原组/记录/部署，最多8个ground child（空候选也计费），源丢失/容量满/MODE已可能排队/未排空均不淘汰或恢复授权。实际pri/type/controller已接，限定源码复核完成，未编译/运行/测试/API/飞控改动/刷机/PR或打包。容量/新部署重置、HOME有效分支/local frame/运动转向、净空/最终StartPermit/时序/执行完成及原25目标/六类仍待开发。下方265–269及更早批次是历史记录。

265–269 新增[地面再审后的分阶段启动事务](../custom/src/Swarm/FleetMissionStartupTransaction.md)源码：actual8/context27、一次28→独立30（Session6/Authority10）、同UUID连续占用；独立第二实例不覆盖旧终态。已解锁成员不发ARM，但仍核实际Seq0完整journal与整组屏障；未解锁成员仅普通ARM(param2=0)，之后仅mode-only。首次Seq0前要求实际周期RAW基线/政策gap，错误实例在换generation前拒绝；后端接线且限定源码契约复核完成，无新执行按钮。本机未调用接口/编译/运行/测试、改飞控、刷机或更新PR/包。最终StartPermit/净空/完成、第二事务再次终止后的实际owner重审路线、HOME有效分支/local frame/运动转向及原25目标/六类仍待开发；旧第一终态不能代替第二事务事实。下方260–264及更早批次为历史记录。

260–264 新增[地面再审启动依据一次接管](../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)源码：实际10来源/原始context27、完整新RAW/全风险/记录/typed当前地面状态进入新owner，旧22一次移交Session独立28（6字段返回真实remaining）、Ledger10字段接管。旧账本退休不盲撤新owner，原最早READ START/两身份期限只缩短，独立实时查身份/WTS/源与整组占用；public不泄露SID/RAW，所有执行/启动/飞行许可false。实际后端/pri/type已接，本批限定源码契约静态收口已完成，无读取/ARM/MODE或UI执行按钮。真正分阶段模式消费者、HOME分支/local frame/运动转向、净空/最终StartPermit/完成及其它六类/原25项目标继续；未编译/运行/测试、调用接口、改飞控、更新PR/源码包/安装包。下方255–259及更早批次为历史记录。

255–259 新增[地面再审独立账号条件意图](../custom/src/Swarm/FleetMissionGroundStartApproval.md)源码：实际新 Review20/九对象/冻结整组与 armed 状态、完整最新记录/风险/新 RAW，真实两账号新提示和 Session 独立22证明，一次消费/撤销及 native14 grant；public27 receipt 无 SID/RAW。最早 READ START 原30秒不续，旧证明不恢复，实际提示期间主身份仍查 SID/role/WTS/旧期限；active READ与消费不放宽。实际 backend/pri/内部类型已接，本批已完成限定源码契约静态收口，没有新增执行按钮/新 startup authority/MODE续作。最终许可/净空/HOME有效分支/local运动转向/完成及其它六类/原25目标继续；未编译/运行/测试或调用接口、改飞控、更新PR/源码包/安装包。下方250–254及更早批次为历史记录。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](../custom/src/Swarm/FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

文档只维护当前实现、稳定契约和可重复操作。版本历史由 Git、发布记录和 PR 保存，不在仓库中复制源码、构建产物或阶段性排障快照。

- [系统化文档入口](README.md)
- [项目状态](PROJECT_STATUS.md)
- [文档审计](DOCUMENT_AUDIT.md)

## 接手入口

- [工程交接入口](handoff/README.md)
- [本批次变更文件快速索引](../MODIFIED_FILES.md)
- [Fleet OS 静态审计记录](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [项目总览](architecture/PROJECT_OVERVIEW.md)
- [当前状态](architecture/CURRENT_STATE.md)
- [仓库结构](architecture/REPO_STRUCTURE.md)
- [风险登记](architecture/RISK_REGISTER.md)

## 架构与安全契约

- [系统图](architecture/SYSTEM_MAP.md)
- [代码责任边界](architecture/CODE_OWNERSHIP_MAP.md)
- [数据流](architecture/DATA_FLOWS.md)
- [关键调用链](architecture/CALL_CHAINS.md)
- [MAVLink 到 UI 数据流](architecture/MAVLINK_UI_DATA_FLOW.md)
- [Fly View 数据流](architecture/FLIGHT_VIEW_DATA_FLOW.md)
- [FTC 遥测契约](architecture/FTC_TELEMETRY_CONTRACT.md)
- [接口边界](architecture/INTERFACE_CONTRACTS.md)
- [安全边界](architecture/SAFETY_BOUNDARIES.md)
- [硬件集成边界](architecture/HARDWARE_INTEGRATION.md)
- [当前链路基线](architecture/CURRENT_LINK_BASELINE.md)
- [Local Agent HTTP](architecture/LOCAL_AGENT_HTTP.md)
- [QGC Agent Client](architecture/QGC_AGENT_CLIENT.md)
- [Agent Supervisor](architecture/AGENT_SUPERVISOR.md)
- [Agent Provider 设置](architecture/AGENT_PROVIDER_SETTINGS.md)
- [Agent 模型 Provider](architecture/AGENT_MODEL_PROVIDERS.md)
- [AI Intent Policy](architecture/AI_INTENT_POLICY.md)
- [AI 问答与指令分离](architecture/AI_QA_INTENT_SEPARATION.md)
- [AI 模型稳定性](architecture/AI_MODEL_STABILITY.md)
- [MERIVUS 集群目标落地矩阵](MERIVUS_SWARM_TARGET_MATRIX.md)
- [Fleet OS 协同、交接与扩展基础层](MERIVUS_FLEET_OS_COLLABORATION.md)
- [本地系统身份与角色来源](../custom/src/Swarm/FleetOperatorSession.md)
- [任务绑定的独立上传审批](../custom/src/Swarm/FleetOperationApproval.md)
- [条件启动意图的独立账号审批](../custom/src/Swarm/FleetMissionStartApproval.md)
- [原生三步启动事务](../custom/src/Swarm/FleetMissionStartupTransaction.md)
- [原生阶段观测及未完成边界](../custom/src/Swarm/FleetMissionStartupTransition.md)
- [受控原生只读版本与参数刷新](../custom/src/Swarm/FleetMissionEvidenceRefresh.md)
- [启动前整组机载任务只读核对（非启动许可）](../custom/src/Swarm/FleetMissionStartContentCheck.md)
- [启动依据的一次原生接管（非最终StartPermit）](../custom/src/Swarm/FleetMissionStartupAuthority.md)
- [Fleet OS 意图、风险、事件与预演基础层](../custom/src/Swarm/FleetIntentRisk.md)
- [Fleet OS 事件回放与任务预演](fleet-event-replay.md)
- [Fleet OS 任务数字孪生契约](../custom/src/Swarm/FleetMissionDigitalTwin.md)
- [Fleet OS 事件触发规则评估](fleet-event-rules.md)
- [Fleet OS 事件规则快照契约](../custom/src/Swarm/FleetEventRuleEngine.md)
- [Fleet OS 后期开发目标路线图](MERIVUS_FLEET_OS_ROADMAP.md)
- [Fleet OS 能力发现与构型指纹契约](../custom/src/Swarm/FleetCapabilityDiscovery.md)
- [Fleet OS 健康中心与任务门禁契约](../custom/src/Swarm/FleetHealthCenter.md)
- [Fleet OS 能量与备用调度契约](../custom/src/Swarm/FleetEnergyScheduler.md)
- [Fleet OS 链路降级策略契约](../custom/src/Swarm/FleetLinkDegradationPolicy.md)
- [Fleet OS 动态角色分配契约](../custom/src/Swarm/FleetRoleAllocator.md)
- [Fleet OS 任务编译器契约](../custom/src/Swarm/FleetMissionCompiler.md)
- [Fleet OS 显式执行 profile 契约](../custom/src/Swarm/FleetMissionExecutionProfile.md)
- [Fleet OS 区域覆盖航线提案契约](../custom/src/Swarm/FleetAreaCoveragePlanner.md)
- [Fleet OS 本地计划版本与审阅存档契约](../custom/src/Swarm/FleetMissionPlanStore.md)
- [Fleet OS 地图任务草稿与交互契约](../custom/src/Swarm/FleetMissionDraft.md)
- [Fleet OS Mission 载荷 v2 与 wire 指纹契约](../custom/src/Swarm/FleetMissionPayloadBuilder.md)
- [Fleet OS 地面上传与独立机载回读契约](../custom/src/Swarm/FleetMissionDeployment.md)
- [Fleet OS 原生参数来源与执行配置证据契约](../custom/src/Swarm/FleetMissionVehicleEvidence.md)
- [Fleet OS 原生启动前置条件与起飞段推导](../custom/src/Swarm/FleetMissionStartReadiness.md)
- [Fleet OS 原始任务序号与实际缓存状态对照](../custom/src/Swarm/FleetMissionProgressObservation.md)
- [Fleet OS 独立启动风险审阅契约（已阅非启动许可）](../custom/src/Swarm/FleetMissionStartReview.md)
- [Fleet OS 操作员上传确认、角色与状态界面](../custom/src/Swarm/FleetMissionOperatorWorkflow.md)
- [Fleet OS 原生固件适配工程声明目录契约](../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)
- [Fleet OS 4D 空域冲突检查契约](../custom/src/Swarm/FleetAirspaceConflictChecker.md)
- [Fleet OS 任务风险评估契约](../custom/src/Swarm/FleetMissionRiskEvaluator.md)
- [Fleet OS 任务报告构建契约](../custom/src/Swarm/FleetMissionReportBuilder.md)
- [Fleet OS 任务审阅协调器契约](../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [Fleet OS 任务证据链契约](../custom/src/Swarm/FleetMissionEvidenceChain.md)
- [Fleet OS 维护护照与预测性维护契约](../custom/src/Swarm/FleetMaintenancePassport.md)
- [Fleet OS 任务连续性建议契约](../custom/src/Swarm/FleetMissionContinuityPlanner.md)
- [Fleet OS 运营记忆地图与指标契约](../custom/src/Swarm/FleetOperationalMemory.md)
- [Fleet OS 行业任务模板市场契约](../custom/src/Swarm/FleetTaskTemplateRegistry.md)
- [Fleet OS 分级自主与平台适配器契约](../custom/src/Swarm/FleetAutonomyPolicy.md)
- [Fleet OS 扩展元数据与本地快照契约](../custom/src/Swarm/FleetExtensionRegistry.md)

## 设计决策

- [ADR-0001：植保区域覆盖飞行初版方案（待实现与验证）](adr/0001-plant-protection-area-coverage.md)

## 开发与验证

- [构建与生成入口](development/BUILD.md)
- [变更影响指南](development/CHANGE_IMPACT_GUIDE.md)
- [重构候选](development/REFACTOR_CANDIDATES.md)
- [测试矩阵](testing/TEST_MATRIX.md)
- [Windows 构建](development/BUILD_WINDOWS.md)
- [Agent 开发](development/AGENT_DEVELOPMENT.md)
- [AI GUI 手工烟测清单](development/AI_GUI_SMOKE_CHECKLIST.md)
- [PX4 起飞健康检查与自动解锁契约](development/PX4_TAKEOFF_HEALTH_CHECK.md)
- [SITL 多机任务测试](development/SITL_MULTI_VEHICLE_MISSION_TEST.md)
- [SITL 编队任务说明](development/SITL_SWARM_TASK_ISOLATION_FEATURE_BRIEF.md)
- [SITL 编队安全评审](development/SITL_SWARM_TASK_ISOLATION_SAFETY_REVIEW.md)

## 硬件与版本

- [硬件目录说明](hardware/README.md)
- [7 寸原型机](hardware/AIRFRAME_7INCH_PROTOTYPE.md)
- [重量与电源预算](hardware/WEIGHT_AND_POWER_BUDGET.md)
- [版本变更记录](releases/CHANGELOG.md)
- [`0.1.0-dev.1` 阶段说明](releases/0.1.0-dev.1.md)

## 代码索引

- [MAVLink 索引](reference/MAVLINK_INDEX.md)
- [Vehicle 模型索引](reference/VEHICLE_MODEL_INDEX.md)
- [QML 组件索引](reference/QML_COMPONENT_INDEX.md)
- [术语表](reference/GLOSSARY.md)

## 仓库卫生

不要提交 DOCX/PDF、源码全文存档、厂商资料、真实凭据、飞行日志、模型文件、缓存或构建产物。历史资料需要回看时使用 Git 历史；尚未落地的设计进入 Issue 或 ADR，并明确责任边界与验收条件。

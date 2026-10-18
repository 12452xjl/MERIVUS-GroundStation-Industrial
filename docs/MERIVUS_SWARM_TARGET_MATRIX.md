# MERIVUS 集群目标落地矩阵

## 546：本地最终交接增量

| 原目标 | 本轮真实操作链 | 明确保留的边界 |
|---|---|---|
| 7 | 原生事件→有界待审→说明/审阅或拒绝→审计 | 不把规则action自动转成飞行命令 |
| 18 | 实际Run→合并后台检查点→独立历史→完整报告 | 不恢复Run、占用或批准，不保证断电持久 |
| 10 | 实际草案→不可变模板版本→文件→新草案 | 内建目录不改写，旧许可不复用 |
| 18 | 真实归档清单→明确留存计划→串行退休/验证备份 | 不自动永久删除，不宣称物理释放空间 |
| 19 | 原实际审阅→完整冻结→四受众→文件与历史 | 不是正式任务结果、事故定论或脱敏导出 |

完整25目标按[最终范围表](handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md)逐项区分本地源码与外部条件。用户已接受此源码交付边界；不将这五条链或52个路径换算完成百分比。未编译、运行、测试或刷机。


## 545：目标20文件操作链已接通，仍未工程验证

完整证据链可从现指挥中心明确冻结保存，并将旧文件后台验证后作为独立只读历史查看；哈希、操作者身份、实际任务结果和控制权限仍是不同概念。7源码/资源登记＋14文档，21路径，新2路径。不编译、测试或打阶段包；原25目标不按文件数折算进度，外部证据/服务依赖与源码缺口分别见[当前路线图](MERIVUS_FLEET_OS_ROADMAP.md)。

## 544：目标6的路线编译断点修复

公共去程展开到正式members/assignments，解决审阅可见但载荷缺成员路线的问题；各机原返程、时间声明及参考保持完整。仅推进已有任务编排链，不代表所有普通输入均可上传，更不认证真实执行、控制权或StartPermit。25项目标未宣称全部完成；无阶段包、编译或测试。

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

## 509 当前：目标14局部受控写链，不增加整目标验收数

| 本批源码增量 | 实际消费者 | 未完成／未证明 |
| --- | --- | --- |
| 独立control写权及精确typed二审 | Role／Session→私有Control | 非企业权限、硬件身份或飞行许可 |
| 真实Capture一次基线／原30秒 | Review→独立ApproveAndApply | 不是导入Map、旧批准或自动resume |
| 单次2502／2503与512／270回读 | Vehicle原sender／共享lane／原RAW | 无nonce；非物理视频／带宽认证，270索引待核 |
| 配置代次与自己的RUNNING-only变化 | 原生Basis／Run／真实Info与Status | 不允许其它配置ABA，也不认证变化由本命令造成 |
| 独立Root意图与现有面板 | 明确目标、两确认、精确取消／partial | 真实Qt／OS／摄像头工作流未验证 |

[30路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[合同](../custom/src/Swarm/FleetCameraStreamControl.md)。目标14的自动联动、恢复、容量与全Fleet调度继续待开发。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。未编译、执行项目实现、测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 508 当前：人工相机流信息／状态读取

新增人工单流相机 Info269→Status270 查询：独立 READ 权限与真实账号／角色／Root 意图，经 Swarm 私有服务、Registry 原绑定、Vehicle 原生 Run 和现有共享消息通道，接到指挥中心／规划页面板。打开页面或原五秒策略不会自动查询；不启停流、配置、降速或恢复。

原入口总8秒包含授权准备，每阶段最多4秒，accepted ACK 后报告窗口最多1秒且只缩短；每项至多一次实际发送。允许真实来源满足检查的飞行状态 metadata READ，不把 RTT 的地面限制直接套入。取消保留已可能发送／ACK／报告事实，不宣称撤回。

ACK 与269／270无nonce，本地匹配不证明无线因果、实际送流／停流、完整目录或带宽变化；270索引兼容性待工程师确认，不支持不回退0／all／广播。complete 不等于两份报告此刻同时新鲜。远端受控视频降级与恢复仍待开发。

[服务合同](../custom/src/Swarm/FleetCameraStreamCapture.md)、[面板合同](../custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[28路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批14源码／接线与14文档；新增 Service h／cc／md、Panel qml／md共5路径，没有新外部依赖。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅为源码工作量粗估，不按批次、路径、接口或局部保护数量上调。完整任务执行、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 507 当前：目标14新增实际相机报告，不新增整目标验收数

| 本批实际增量 | 消费者 | 仍未证明 |
| --- | --- | --- |
| 原PRIMARY相机269／270被动账本22／4／15／15 | Vehicle原RAW与Registry wrapper7 | 非身份／完整目录／命令因果 |
| 两fresh且本地revision匹配的状态计数 | Policy第五参数／localCamera18 | 非实际送流／停流或带宽 |
| 原五秒快照和既有PlainText摘要 | Guided→CommandCenterOverlay | 非自动配置／恢复许可或运行验证 |
| 模式实际caller与独立mode意图代次 | CameraControl原公开入口 | 非全部旧相机API认证 |

[完整合同](../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 506 当前：目标14前置链收口，不新增整目标验收数

| 本批源码 | 实际消费者 | 不证明／剩余 |
| --- | --- | --- |
| 本地播放意图、pending与有限恢复 | 原VideoManager／双receiver | 非远端停流、wire因果或带宽认证 |
| 原生选择代次与真实QObject归属 | 原CameraManager／Control／VideoManager | 非PRIMARY原始新鲜证据或配置权限 |
| 发现／选择／旧回调保护与有界字符串 | 原相机发现、操纵杆及流Info／Status | 非全部旧相机API／插件认证 |
| 共享模型通知后QQ与匹配reset | 原相机／流模型与既有共享消费者 | 非任意外部raw列表突变或Qt内部线程认证 |

[合同](handoff/MERIVUS_VIDEO_LIFECYCLE.md)、[19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[50项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505 当前：目标14／18修真实下载链，不新增整目标验收数

| 本轮源码范围 | 实际消费者 | 仍不证明 |
| --- | --- | --- |
| 原来源与冻结目录／选择 | 原LogDownloadPage与Controller | 无LOG nonce／远端文件身份认证 |
| 有限缺块重读与完整写入去重 | LOG_DATA当前chunk及本地partial | 非跨重启checkpoint或自动续传 |
| 冻结公告字节全收齐／flush／改名后成功 | 现有状态列与标准完成文件 | 非远端取尽／签名／断电durable／整个Fleet补传 |

[合同](handoff/MERIVUS_LOG_DOWNLOAD_RECOVERY.md)、[14路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或重试次数上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504 当前：目标14增加实际RTT观察，不新增整目标验收数

| 源码增量 | 实际消费者 | 未证明／剩余 |
| --- | --- | --- |
| 独立action／主账号／原Root意图 | Swarm私有Service，current18／result5 | 非二审、配置或飞行许可 |
| 单次定向TIMESYNC v2／原4秒 | Vehicle opaqueRun与原RAW回显26 | 非纯网络／身份／时钟同步；旧端无回包即未知 |
| 原记录／绑定wrapper7 | Policy第四参及localLatency10 | 容量／质量／丢包仍未知，不得normal |
| 人工单次Panel与原五秒只读策略 | 两宿主及Overlay该次快照 | 无周期探测、视频／速度联动或恢复补传 |

[31路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[合同](../custom/src/Swarm/FleetLinkLatencyCapture.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503 当前：目标14新增真实接收观察，不增加整目标验收数

| 已写源码 | 实际消费者 | 未完成／未证明 |
| --- | --- | --- |
| PRIMARY／默认component原帧五秒窗4096 | Vehicle→Registry实际binding一次getter | 非可用带宽／RTT／丢包／远端率 |
| native22／window9与wrapper7 | 新Policy固定消费；QML整数桥校验 | Map不认证；四QoS未知，不能normal |
| 原五秒块接真实review | Guided→Overlay该次快照完整／不足／不可用 | 不是实时许可、自动降频或视频／速度联动 |
| RSSI排除−1未知哨兵 | 旧Registry健康近似 | 其余负缓存单位／来源／新鲜度仍未认证 |

[23路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[合同](../custom/src/Swarm/FleetLocalReceptionObservation.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502 当前：目标14补可见消费者，不新增整目标验收数

| 已写源码 | 实际消费者 | 未证明／剩余 |
| --- | --- | --- |
| FleetOptionalTelemetryPanel | 指挥中心／地图规划页，同一Root | 真实Qt／布局／键盘／OS／协议未验证 |
| 显式采集→审阅→独立批准 | 原500／501后台和result5／workflow24 | 共享历史不能领权，无自动Apply |
| 稳定来源＋本页真实回执 | 原scope／identity5／双policy9与ID/gen/hash | 不随自有状态误取消，不等于认证 |
| 原30秒显示／精确取消／partial展示 | 原native期限与历史 | 不续期、重试、撤回或回滚 |

[24路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[面板合同](../custom/res/Merivus/FleetOptionalTelemetryPanel.md)。真实测量、视频／速度／补传和目标14效果验证继续待开发。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501 当前：目标14新增限定配置链，不新增整目标验收数

| 源码增量 | 实际消费者 | 未完成／未证明 |
| --- | --- | --- |
| Capture三原上下文seal一次／opaque Basis | 私有Configuration requestReview | Map／历史ID不能复原；须未来新Capture |
| 独立action／purpose／双账号typed proof | 实际Session＋Role＋Root原意图 | 非企业鉴权、硬件认证或飞行许可 |
| 3×511→510＋最终3×510 | 原Vehicle共享lane／九阶段Run | ACK／值匹配不是实际频率／带宽证明 |
| 原30秒、取消、partial与历史投影 | 后台request／approve／cancel | 无新可见按钮、重试、回滚或自动降级 |

start仅认领ready、不写包；100ms后推进九个wire阶段。每条至多4秒，510 accepted ACK后至多1秒且只缩，所有阶段与审批共用原Capture最早30秒。慢链路可能无法完成；截止／失败／取消保留已发事实，不重试、不自动回滚。三项最终回读只给reported-values-match，不认证无线因果、实际发布率、带宽或飞行许可。

[30路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[46项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[合同](../custom/src/Swarm/FleetOptionalTelemetryConfiguration.md)。目标14的真实测量、视频／速度／补传及效果验证继续待开发。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500 当前：目标14新增真正只读查询，不新增整目标验收

| 源码增量 | 实际消费者 | 未完成／未证明 |
| --- | --- | --- |
| 独立capture action与主账号 | 私有Swarm服务＋Root原意图 | 非Apply、二审或飞行许可 |
| 原生510／ACK＋244双终结 | 固定单车62→74→83 | 固件兼容、无线因果与真实发布率未验证 |
| 原来源／mutation／语义与固定期限 | 三条原native query全量尾核 | 不接受混合代次或unknown clear重基 |
| 原批历史和精确取消／释放 | Root只读投影与既有Registry重采 | 无新可见按钮、无自动降频或已发撤回 |

[24路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；详见[模块](../custom/src/Swarm/FleetOptionalTelemetryCapture.md)。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前：目标14修真正发送链，不新增整目标验收

| 源码增量 | 实际消费 | 仍不证明 |
| --- | --- | --- |
| 510／511／512共享原native lane | generic命令＋原版本refresh | 无wire nonce，非请求因果／远端应用认证 |
| 512 shared双端context及固定deadline | 原raw ACK／消息＋原500ms timer | 本地有界，不是链路可靠性已验证 |
| 511实际write前mutation／全清旧ledger | 498原Registry与Guided只读链 | 不是配置后实际率／带宽／许可 |
| owner及整列表代次尾核 | 实际InitialConnect／StandardModes | 非所有legacy callback或握手通过证明 |

Vehicle公共raw sender在plugin前后拒510／511／512旁路，正式请求走专属通道。此覆盖不包括MAVLinkProtocol转发、移动Taisync字节桥、外部GCS或固件内部调频；本批不改变转发策略，不认证实际发布率、丢包或带宽。

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

[18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[42项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前：目标14的原生观察和实际只读消费，不新增整目标验收

| 已写源码 | 实际消费者 | 未完成边界 |
| --- | --- | --- |
| 原244 signed报告及固定62／74／83 | Vehicle原包账本→Registry实际绑定getter | 被动接收，不主动查询，不认证发布率 |
| 5秒／512本地窗 | 同次review完整observation | 不把收包均值当带宽、丢包或停流 |
| 正int32精确REAL32／三条基线比较 | Registry review21／comparisons6 | 数值不增频不是配置许可或实际应用 |
| 私有Registry真实调用 | 原Guided五秒linkPolicies附review | 不新增UI、授权、发送或自动降频 |

本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

[18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)及[36项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前：目标14前置修现有请求归属，不新增整目标验收

| 实质增量 | 已接消费者 | 仍未完成／未验证 |
| --- | --- | --- |
| Stream requestId／单pending／pump64 | 原PID三组高频与显式restore | 0仅固件默认，非原实测速率回滚 |
| 专属结果context | 原PID与ESC STATUS→INFO | 非wire因果／实际频率／带宽节省证明 |
| 公共511原source／UUID／timeout隔离 | 原所有generic511入口 | 成功终态无新冷却，同source重复ACK仍有歧义 |
| provider／QObject生命周期尾核 | 原Link/plugin/Fact/错误通知／实例设置 | 未执行生命周期或业务验证 |
| 工程交接 | 6源码＋12文档=18路径，36项全未执行 | 不是构建／运行／工程验收通过 |

真正的目标14仍需独立实际Session/角色scope与必要二审、当前原生链路证据／期限、闭合optional白名单、原MESSAGE_INTERVAL244基线/回读及收包频率窗口。不能借只读refresh或start/capability凭据，不能照搬Policy的全telemetry.rateHz，更不能降频P0证据或控制协议；本批没有新apply UI/入口／权限／自动飞行动作。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前：目标6／12补垂直编码参考，不新增整目标验收

| 已落增量 | 实际消费者 | 未完成边界 |
| --- | --- | --- |
| 同次TakeoffReference | 原成功vertical/support→Waypoint一次Contact.prepare | 非ground状态、可移植凭证或来源认证 |
| 完整current/top盒 | 原connectionHull／外扩／三覆盖＋两接触 | 非实际上升、动态接受或平滑轨迹 |
| 原窗口与有限pad | full current整体落原窗口；指定terrain完整交集包含 | 不clip／放宽，不扩body／obstacle／top→NAV／Domain |
| v5及实际总计数 | 原31/schema1／3／Worker7及同prepare matcher | NAV计数不变，七false和actualPath真门保持 |
| 静态工程交接 | 4源码＋15文档=19路径，36项全部未执行 | 不代表构建、运行、仿真或放行通过 |


原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前：目标6／12补起飞参考连接，不新增整目标验收数

| 增量 | 真实消费者 | 未完成／未验证 |
| --- | --- | --- |
| 原GPI9＋PM29＋wire重算 | Waypoint独立重建原float32完整top区间及Joint名义点／序号对应 | 不是当前landed／ground-valid或内部状态证明 |
| top到首NAV完整连接体 | 全split帽XY凸包×完整Z、原protected/support与五类环境 | 非真实起飞垂直域／平滑轨迹或永久HOLD |
| 模型v4／总计数 | 原report31/schema1/result3、Worker7和同prepare matcher | 原七认证／许可false保持，实际路径真门不删 |
| 静态交接 | 2源码＋14文档=16路径，36项全部未执行 | 不代表构建／模型／工程验收通过 |

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前：目标18／19／20新增文件历史，不新增整目标验收数

| 增量 | 实际消费者 | 仍未完成／未验证 |
| --- | --- | --- |
| 独立保护报告codec | 原record16→report8，保存／加载复用完整校验与摘要重建 | hash不是签名、来源或远端结果认证 |
| 新窄文件转发 | 原生JSON＋预期ID/hash、两文件动作共享围栏 | 不进入旧full9槽，不恢复Run或权限 |
| 当前报告面板 | 另选路径、二次确认、取消、captured/imported历史 | 未加载UI或执行文件往返，非当前状态证明 |
| 交接与审计 | 4源码／UI＋14文档=18路径；36项全未执行 | 静态阅读不是构建或工程验收 |

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前：目标18／19的遥测跨Run检索，不新增整目标验收

| 已落增量 | 实际消费者 | 未完成边界 |
| --- | --- | --- |
| ordered manifest＋实际逐file search | Reader新增8键请求／34键结果／10键行，原Archive窄入口 | 非全集索引、任务关联或跨file原子快照 |
| matched／no-match／unavailable | 同一原batch全部已检查file，包含零命中和未知诊断 | 坏行、坏源序／Footer不得用合法前缀报命中 |
| 原生cursor及hit hash打开 | facade／Panel显式续批与打开整个原file首页 | 页仍只按机号，不继承msgid筛选或自动报告 |

[6源码／UI＋14文档=20路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36项工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。原25目标和六类实质剩余继续，约40%完成／60%剩余只是源码工作量管理粗估，不由入口／文件／查询条数上调验收率。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前：目标6／18／20的原生观察生命周期

保留三项live Link语义，以私有source／slot和Run变更代次、原QQ／weak owner及回调后原行重定位补执行观察链。const报告也不得跨外调持成员引用或混代；serial不回绕，耗尽不能删除已捕获实际commit或伪造新的可用source。

原TaskRecord在Evidence前保存完整副本，回调后按同id重新find并核24字段；容器正常重排不影响同一未变记录，记录真实推进不能被外层旧值覆盖。原历史报告／max关闭／late commit和全部认证／执行false边界不改，未新增完成／释放占用／控制指令。

[7源码＋15文档=22路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。原25目标及六类实质剩余继续，约40%完成／60%剩余仍仅源码工作量管理粗估，不由本批路径或guard数量推验收率。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前：目标6／12的LAND声明参考体

新增的是前NAV完整接受区域到已绑定支撑参考面的有界几何，不是实际着陆控制器。原生LandingReference沿同次成功LAND support提供vehicle／sequence／segment／精确rest上下界／contactId／指定terrain与有限pad；不从QML、p7或最新参数补参考。

Waypoint模型升级v3，入口连接和完整XY下降参考体均实际送原三类覆盖与地形／障碍接触检查，原NAV计数仅计NAV，全部参考体纳region／component及matcher。只下降support可按原有限交集包含规则豁免，原Joint／名义Contact组件×2及原Worker合同不改；planning仍无PM29接受域审阅。

[4源码＋15文档=19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。动态接受扩大、真实入口／平滑／地面／时序与最终许可未完成，原25目标及六类实质开发保持；约40%完成／60%剩余仅源码工作量粗估，不能从局部参考体或文件数增加验收率。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前：目标6／12的原参数末端及完整索引覆盖

目标6：原beforeArm.parameterSummary的6键／29行快照进入实际MemorySeal，不使用新capture结果替原值；Target首次固定PRIMARY控制块贯穿stage，唯一调用更新为必填actual PM／owner／snapshot。最终Vehicle已有commandContextCurrent链实际再核PM29，无需新增另一授权器。等值新receipt与值／来源／类型／contentRevision变化分开，旧截止不续期。

目标6／12：Geometry新增coveredByIndexedPrisms，Domain和Waypoint六处coverage／corridor／terrainHorizontal接入。全部原场景先准备、严格分离裁去无关凸柱，保留原完整斜／严格面、闭边界、空query语义及统一预算；terrain水平与接触索引分别建。旧数学模型／报告schema及许可不升级。

[24路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)同步。原25目标、六类实质剩余保持，约40%完成／60%剩余只为源码工作量粗估，不由本批路径／接口推分项验收比例。LAND真实移动／下降域、其它动作／动态轨迹、完整StartPermit、正式结果与其它五类继续开发。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前：保护记录历史报告，不新增整目标验收数

| 增量 | 实际消费者 | 未完成／未验证 |
| --- | --- | --- |
| 轻目录6／行13 | 原Orchestrator→Swarm→Root→现报告面板 | 非磁盘档案／完整历史全集或身份认证 |
| 原记录16捕获 | private lookup、原sourceId／四字段选择、完整记录双尾核 | 缓存非当前遥测／控制权或正式实例 |
| 独立report8／summary13 | 新static构建器核compact20／21及原候选引用 | 不补full current，不认证完成／停飞或原因 |
| 本页单份报告 | 明确选择／确认、原生JSON、同票据清理和失效 | 不自动保存／发送；未加载UI或运行代码 |
| 工程交接 | 8源码／UI＋15文档=23路径；36项全未执行 | 静态核对不是编译或验收 |

原25目标及六类实质剩余不缩减，约40%完成／60%剩余只是源码工作量粗估。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍需开发。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：P0关联隔离与普通NAV凸域，不新增整目标验收

| 增量 | 实际消费者 | 仍未完成／未验证 |
| --- | --- | --- |
| 序号回退结束关联 | 原Run tracking门、poll、同记录TaskModel与报告 | 不判原因、不认证远端实例或正式完成，不释放占用 |
| XY凸包连接棱柱 | 原prepare／Worker／resultMatches及覆盖并集 | 非真实平滑器、动态高度或采样间轨迹认证 |
| 带平面接触检查 | 原索引AABB粗筛后同scene完整凸域可行性 | 不承诺性能改善，预算耗尽／未知仍阻断 |
| 工程交接 | 2源码＋13文档=15路径；32项全未执行 | 静态阅读与文本检查不等于编译或运行验收 |

原25目标及六类实质剩余保持；整体约40%完成／60%剩余仅源码工作量粗估，不由两个修复或路径数推导新百分比。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍需继续。未编译、执行helper／模型／应用、测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 487 当前：历史资料报告实际接线，不新增整目标验收数

| 增量 | 实际消费者 | 未完成／未验证 |
| --- | --- | --- |
| 新static报告入口 | 原page→一次MotionReview→dossier21含原31、来源6、覆盖8 | 不关联任务、不认证事故／漂移／性能或正式结果 |
| 双缓存共同来源 | Archive原review一次消费、原页/hash/结构尾核、双getter共同当前性 | 未运行生命周期／业务／hash验证 |
| 四视角UI | 原确认、一次快照、实际聚合／质量限制、可选择复制结构化文本 | 视角非权限；不自动保存／发送，未加载UI |
| 工程交接 | 6源码／UI＋15文档=21路径；34项全未执行 | 独立静读／最终文本统计已记本批审计，不是运行验收 |

原25目标及六类实质剩余保持；整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批路径或视图数上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。 本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前：遥测历史实际消费，不增加整目标验收数

| 源码增量 | 实际接线 | 仍未完成／未验证 |
| --- | --- | --- |
| 固定历史动作 | Archive exact6→原生catalog／first／next／review／cancel，Root窄facade | 不开放capture／任意路径／raw，不认证操作者 |
| 运动摘要 | 原最新双终结page36／row29→MotionReview→固定report31 | 不是漂移／原因／真实性能／完整轨迹或无reset证明 |
| 实际UI | 指挥中心和Planner、位置两次明确同意、原query/代次尾核 | 未加载／运行布局、键盘或读屏 |
| 工程交接 | 11源码／UI／登记＋14文档=25路径；32项全未执行 | 独立静读及最终文本结果已记本批审计，不是运行验收 |

原25目标及六类实质剩余保持；整体约40%／剩余60%仍仅源码工作量粗估，不按本批文件或入口上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。 仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前：严格性能政策接入区域任务，不新增整目标验收数

| 源码增量 | 已接实际消费者 | 未完成／未验证 |
| --- | --- | --- |
| 三态政策编辑器 | 原Planner／Root set/clear、pending统一门 | 未运行UI／可访问性，非性能证据 |
| Draft schema5 | 完整editing/planning来源与实际task top Bool | 不补默认、不恢复批准／基线真实性 |
| Compiler／Store | source完整重建，显式false保留，存在性＋值绑定 | 不代表预算／轨迹或启动许可已通过 |
| 保存与恢复 | shared源校验／原完整源传递，余段保政策 | 没有文件／历史往返或执行验证 |
| 工程交接 | 9源码／UI／资源＋19文档=28路径，32场景全未执行 | 本批审计已记录独立静读及最终文本核对，不是运行验收 |

原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。 本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前：性能声明录入／审批纵切，不增加整目标验收数

| 已落源码 | 作用边界 | 仍未完成／未验证 |
| --- | --- | --- |
| 原面板第五操作与Root专用请求 | 两canonical数值＋完整caps＋理由真实输入、冻结、取消及两确认 | 未运行布局／认证／交互；非实测录入自动认证 |
| Registry／Review／Session | 当前完整基线、独立purpose双账号、同拟context一次CAS后通知 | 不改变基线、不认证实物／性能，不恢复飞行许可 |
| 摘要与历史 | 新独立type/hash、原21键隐私摘要、当前页／跨库操作筛选 | 不保存完整性能卷宗，不恢复批准或证明SQL落盘 |
| 工程交接 | 12源码／UI＋19文档=31路径，32场景全未执行；本批审计已记独立静读及最终文本核对 | 不是运行验收 |

新值进入483实际消费者，但原区域Draft严格开关未开发。原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。 本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前：工程性能数字进入准入，不宣称性能认证完成

| 本轮增量 | 已落源码边界 | 仍未完成／未验证 |
| --- | --- | --- |
| 可信声明消费 | 原能力批准＋完整当前构型基线，共享exact11返回绑定数值 | 不认证调用者、实物、实测性能或未上报改装 |
| 实际任务准入 | 严格选机／角色、Compiler主备阈值／执行速度／coverage、备用独立预算 | 新性能录入器、可信实测清单、载荷／环境／老化标定 |
| 能量与身份 | 声明续航×原电量，native direct只取小；strict planId、Store标记、Coordinator/schema3完整来源 | 不是实际电池模型、保证续航或动态轨迹／StartPermit |
| 工程交接 | 12源码＋19文档=31路径，30项全部未执行场景；本批审计已记独立静读和最终文本核对 | 未编译、运行、测试、仿真、硬件、刷机或放行；不是验收通过 |

既有 `requirePerformanceEvidence=false`／缺失不自动升级旧规划或历史批准。原25目标及六类实质工作完整保留，整体约40%完成／60%剩余是源码工作量粗估，不是验收率；P0缺动态状态消息证据及原false阻断继续。下方为历史。

## 482 当前：真实NAV接受／连接审阅，不新增整目标验收

| 本轮增量 | 已落源码边界 | 仍未完成／未验证 |
| --- | --- | --- |
| 原参数来源 | schema5独立arrival、Readiness原12／2／14／1快照、原29尾核贯穿两Review／Witness | 不是实际控制器动态状态或真机二进制认证 |
| 普通NAV几何 | 静态接受帽、独立精确高度界、邻接完整AABB、全量环境比较 | 动态高度接受、平滑器／真实路径、独立动作与采样间连续性 |
| 实际消费者 | Worker/schema3／row4／result7，风险、共用准入和两Authority完整绑定 | actualPathCertified=false仍阻断，完整StartPermit及正式任务结果未收口 |
| 工程交接 | 精确41路径、32项未执行场景、刷新29项静态文案 | 未编译／运行／测试／仿真／硬件／刷机，最终文本核对已记录于本批静态审计 |

旧12hash、公开19键及PM12／14／28精确旧合同保留，不靠扩大旧集合或参数自报许可。原25目标和六类实质开发不缩减，整体约40%完成／60%剩余仍是源码工作量粗估，不是运行通过率或目标验收率。

只静态源码／文档修改；未执行helper／模型／业务／QML、未改PX4、无提交／新PR／包或放行，UI/UX未脚本或界面验证。下方为历史。

## 481：实际整数门与同域外包，不升级整目标验收

[IntegerDomain](../custom/src/Swarm/FleetMissionStartupIntegerDomain.md)完整接Motion／Cover／DomainReview真实链；profile和编辑器提供显式选择、保旧配置并撤旧确认。原完整源与native对象流经Worker／Witness／Txn，值型重建不替换真实原Ptr。

新全组受控模型可产生针对整数采样接受域的包含结论；旧／混组或不完整投影不能true。实际动作、采样间路径及完整启动／任务执行仍未完成；actualPathCertified=false由实际准入消费，Contact豁免不扩大。39路径是源码增量，不是25项完成数或运行通过率，整体约40%不据此提高。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480：起降参数来源及兼容前置，不新增整目标验收

新增[动作分支模型](../custom/src/Swarm/FleetMissionActionPhaseReview.md)及固定28项实际读取／原源尾核，schema4独立动作证据进入原审阅与Readiness。自动LAND的RC辅助／外部setpoint和TAKEOFF的外部注入不被原名义路径假设接纳，旧12hash／19键及PM12／14合同保留。

这是执行主链的有限源码增量，不是实际起降轨迹、全阶段数值／动态净空或启动许可认证；Contact接地例外和原起飞几何合同不扩大。原25目标及六类实质剩余仍在，整体约40%只源码工作量粗估，不由本批27路径或模型ok上调。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：目标6的参数覆写一致性，不新增整目标验收

[RateOverrideReview](../custom/src/Swarm/FleetMissionRateOverrideReview.md)有真实8速率比较和失败消费者；PM14固定来源、Refresh显式14读取、原12hash不变、Evidence／Readiness／phase最终纯内存核原14行全部接入源码。

同源guard参数与可用性进入完整证据／启动配置指纹，不是只新增目录标签。该合同不证明内部运行分支或实际跟踪；完整TKO／LAND动态、时序、StartPermit、正式结果和原25目标／六类实质剩余保持。没有编译、运行或验收统计。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478：目标6／12及余段来源的编码一致性增量

Planner三API提供canonical原域、strict验证和实际路线消费者；Compiler0.7／plan1.1保存完整策略，Store核当前源与覆盖摘要。Builder2.1严格当前构建，整组一次查编码后坐标链，原raw上传与wire hash不重写。

新single3／4、group2保完整policy；旧来源／计划仅惰性历史，独立复现口不进入实际Deployment／Review／Readiness。单机前置重建绑定同一fullpolicy，资料源task与plan相互一致。原25项目标数、未验收状态与六类实质剩余保持。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477：目标6／12的名义退让，不新增整目标验收

[Planner](../custom/src/Swarm/FleetAreaCoveragePlanner.md)新增0.4正退让候选与全部原边整段欧氏距离复核；共享checker无洞正退让同样消费，不因没holes而no-op。原64／8／128输入、1024／8192／32768预算保持，派生32环／256点／2048路径元素额外有界。

[source4](../custom/src/Swarm/FleetMissionDraft.md)可含独立退让、孔及clearance组合，旧1/2/3不变；实际source恢复、同次UI保存与版本身份保留。physicalClearanceVerified／numericalErrorCertified仍false，不把名义距离当真实起降或编码轨迹净空。原25项及六类实质剩余继续。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476：目标6／12的名义排除区，不新增整目标验收

[覆盖算法](../custom/src/Swarm/FleetAreaCoveragePlanner.md)支持8孔、总128顶点、有界扫描和全环连接；[source3草稿](../custom/src/Swarm/FleetMissionDraft.md)及[编辑器](../custom/src/Swarm/FleetCoverageExclusionsEditor.md)接到原审阅链。Compiler检查全部实际名义路线（含去返桥接与完整备用预算），新单机／联合余段保留原area后重新检查。

它不证明真实3D轨迹、工程障碍净空、转弯／半径／地理误差或飞行成功；旧numericfalse、Compiler pending和默认飞行许可保持。原25项及六类实质开发不因本批改变。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475：目标6／13接联合新任务，不新增整目标验收

GroupPlanner、Compiler／Store、ReviewCoordinator与全组View使用独立groupRemainderPlanningSource，和旧两源严格互斥。完整原组每行明确请求，新目标／备用唯一，未提交／不支持行不能默默省掉；每机预算与速度进入真实当前审阅。

Root／联合编辑区／唯一共享部署源已接，不循环覆盖单成员审阅、不从第一机推全组，也不恢复旧Run或执行许可。旧区域及single合同保持；原25目标、六类实质范围和工程师验证均继续。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474：目标6／13增加完整原组目录，不新增整目标验收

新增原生整组集合与中央纯值校验、全包v2/common单副本、双版本窄目录和明确成员选择，接原single余段预览／新审阅。未提交行、未知旧begin和无独立航点均明确显示，不以其它成员路线伪造执行资料。

原25目标保持；本批不提供联合多成员新计划、自动机载自愈、跨站控制租约、完整启动许可或正式任务结果。源码范围6、文档16，不是完成目标数。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：目标6／13的显式完整历史资料来源，不新增整目标验收

[RemainderArchive](../custom/src/Swarm/FleetMissionRemainderArchive.md)和[实际面板](../custom/src/Swarm/FleetMissionRemainderArchivePanel.md)保原Run单成员的完整政策、计划、载荷映射及观察；新的历史schema2及包关联进入原余段裁剪／新审阅。打开仅预览，选用、保存覆盖分别确认，不借旧报告或最新计划补缺项。

单份资料槽／capture、精确退出及文件提交事实已写源码；没有编译／UI／文件往返证据。原25目标与六类范围保持：自动机载自愈、完整启动许可／正式结果及企业跨站等不因此完成。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：目标6新增独立未完成编辑态来源，不新增完整目标验收

新增[Store](../custom/src/Swarm/FleetMissionDraftStore.md)及[实际面板](../custom/src/Swarm/FleetMissionSavedDraftPanel.md)，经Planner／Root／原generic copy流程接线。只保存完整规范的已应用编辑态，可不满足buildTask；恢复新Draft revision 1、重新确认地图基准、撤销旧批准、保原草稿为共享会话备用。旧history／repair strict校验保持。

覆盖／恢复均需独立确认，不从文件hash、成功替换或历史内容生成机载证据或权限。完整任务执行和六类实质工作继续，原25项目标不缩减；整体约40%仍只是源码工作量粗估。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：目标6／12保同次原运动对象，不新增整目标验收

[Models及深冻结](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)实际贯穿两类Review／Worker／DomainReview／Witness／Txn，后台计算不从source12重建替身。实际finished比原Ptr集合，取消／代际替换不能以同描述新对象继续；原组完整性、原Owner、时钟与最终门禁保持。

source值匹配只核对应，不能认证对象实际来自哪个Review；原生私有持有链仍是必要依据。原contains、报告与数值未证状态不变，完整StartPermit／动作范围／正式结果及其它六类代码继续。13源码18文档31路径是本批范围，不是完成目标数。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：目标6／12增加同源完整候选索引，不增加整目标验收数

[Geometry](../custom/src/Swarm/FleetMissionClearanceGeometry.md)在既有完整凸柱上新增自持闭BVH；[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)将地形／障碍全量逐对SAT改为完整包围盒候选后的同源indexed SAT。输出原行顺序及真实cell／volume身份不变，接触不丢，失败不消费部分候选。

原三种并集覆盖、完整高度带、全组唯一预算、v2／报告28／Worker6和numericfalse门禁保持。索引是有界数学计算缓存，不是资料认证、碰撞观测、起降许可或25目标完成证明；原六类实质工作仍须开发。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469：目标6／12接精确凸柱交集，整目标验收数不增加

[整域报告](../custom/src/Swarm/FleetMissionStartupDomainReview.md)的地形／障碍交集已实际改用[准备型精确SAT](../custom/src/Swarm/FleetMissionClearanceGeometry.md)。完整资料先规范化、全部原外包准备完成，再用同组预算准备全部凸柱后查询；坏型／取消不交付成功前缀，资源不足保真实incomplete。

三种并集覆盖、完整高度带、接触不豁免、模型v2／报告28／Worker6及原数值未证阻断保持。新True只说明声明数学集合非空，不是碰撞观测、净空认证或启动许可；P0和原25目标仍有实质代码工作。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468：目标6／12的实际外包与求解改进

[整域模块](../custom/src/Swarm/FleetMissionStartupDomainReview.md)接新保守纬度分段外包v2，去掉地形XY覆盖的重复query-Z面，并校不完整报告只能引用实际可能处理的component前缀。完整输入、全部原腿／高度带、原source12和result6消费者不变，旧v1报告须重新计算。

本批不增加25整目标验收数；原Qt完整数值包含、动作／阶段净空和最终启动许可仍未完成，numericfalse、Compiler pending及整域无动作接地豁免保持。更小外包不等于更窄飞行允许域，删除冗余平面不等于删除地形高度检查。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467：目标6／12增加完整理想域计算，不新增整目标验收

[DomainCover](../custom/src/Swarm/FleetMissionStartupDomainCover.md)与[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)已接实际启动Worker及两条原生消费者链：完整有序source12、原payload、同次Joint与资料绑定，整组覆盖／走廊／地形／障碍计算共享200万work／2048fragments。合法但不支持外包是完整incomplete诊断，取消或坏源为空报告失败。

理想外包完整／声明模型满足与原Qt数值包含是不同字段；后者固定false，实际共同门禁要求true，故两Review保持clearance-blocked、Witness不得构造。原指定起降pad不能豁免整域任意support接地，旧名义规则不变。没有增加正式启动许可、任务完成或25目标验收数。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466：目标18的历史导入增量，不新增整目标验收

[维护面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)只允许当前原生诊断的readable行进入导入预览，rejected仍仅诊断。原生固定目标、同一私有预览和实际线程结果对应，源目录／目标目录／暂存名／最终名均明确，不把partial当正常历史段。

importVerified只说明本次副本发布并读回自洽，不验证原源作者、事件真实性、所有记录hash或历史全集，也不生成任务／账号／许可。原25目标、完整执行与运营六类实质范围保持。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465：目标18的逐文件诊断，不新增整目标验收

[维护面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)消费同一原生结果，明确候选／可读／拒收及固定原因，保留原工单、原目录、实际finished及路径版本。原始坏名只做安全纯文本显示，不成为任意源路径或恢复输入。
新Worker23／Store26、row4／5仅诊断；originalSourceVerified与backupSetCompletenessVerified保持false，不借旧backup31的backupVerified授信。正式备份导入恢复、长期运营和其它实质开发仍未完成。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：长期运营增加独立备份观察，不新增整目标验收

[维护链](../custom/src/Swarm/FleetPersistentEventStore.md)的backup-catalog只用明确目录，原source不存在也不读取或创建它；新Worker21／Store24和entry8／9只表示本次副本结构观察。originalSourceVerified／backupSetCompletenessVerified严格false，旧backup31的backupVerified含义不被借用或放宽。

用户明确读取、同query取消、实际finished后显示与路径版本退休已接[面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)。本批不是备份导入／恢复、任务完成／许可，完整备份运营、原25目标和六类实质开发继续。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：长期运营的单段备份增量，不新增整目标验收计数

备份source只来自同一[原生库存](../custom/src/Swarm/FleetPersistentEventStore.md)，旧6字段动作不变；仅prepare-backup为7字段，backup预览schema2／26键。原维护槽接独立backup31事实与current23，不把旧move27的“未移动”解释成“没有写入副本”。

[面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)明确路径、目标名和独立确认，实际finished后接纳同原工单。原目标18／长期运营只增加这一可读回单段副本能力；备份全覆盖、签名／权限、灾后恢复及其余目标继续，不按新增分支或文件数提高完成率。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：长期运营的可恢复归档维护增量

[固定本机维护](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)与原封存／任务历史共用实际原生互斥，库存→私有预览→提交→真实线程结束及移动事实已接源码。单文件retire／restore只改变归档命名空间，不能从恢复文件推导账号、任务实例、执行成功或当前能力。

退役不会降低磁盘用量，未知移动结果不补成功／未发生。原25目标验收数不因本批增加；完整留存清理／备份／灾后恢复、长期运营及其它六类实质剩余保持。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：长期历史按任务下钻，整目标验收数不增加

[目录及原记录入口](../custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)已接固定本地归档通路；原生累计来自完整接纳的实际search批次，task筛选由Reader执行，而不是只过滤屏幕当前页。现有工程操作搜索保持独立。

目录是可重建的本机内存诊断索引；只数保留事件、已保留的车辆关联与异常标记，不能还原被省略的任务结果／成员／能耗。部分、末尾、预算和取消／失败必须区分；不新增完整任务、长期运营或原25项的验收完成声明。原六类实质剩余代码保持。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：目标5／9的明确电池负面与实际准入消费者

[原生电池](../custom/src/Swarm/FleetNativeBatteryObservation.md)新增实际扩展状态，[共享检查](../custom/src/Swarm/FleetNativeBatteryEvidence.md)统一完整合同。主／副电池明确故障或非正常charge阻断主备账单和任务健康门禁；过期负面保unknown，原合法evidence与来源保留。Guided身份／原semantic revision和发送前phase seal覆盖变化，不只做UI显示。

这不是认证电池性能、预测未来耗电、物理身份或完整飞行许可。无扩展旧硬件不被默认叫故障；原时限、权限、源检查及全部25目标保持，完整运动域、StartPermit和正式结果继续。准确范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：目标6／12的原生同模型持有和实际阶段复用

[Witness缓存](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)与[Txn私有目标](../custom/src/Swarm/FleetMissionStartupTransaction.md)固定完整原模型，实际Transition只读同Ptr；新请求一次准备和完整source配对，不在每次phase／诊断重放时重建。无资料旧政策同样保一次绑定，不加默认或新schema。

这是来源明确化及移除重复准备，不是完整域覆盖或许可验收；原普通源指纹已含GPI，不虚报已证明漏洞。数值外包、整个走廊×高度区间、完整StartPermit和正式结果仍未完成，Compiler pending不删，原25目标不缩。准确范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：目标6／12固定同一实际地理运动模型，未新增验收目标

[模型](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)已接实际Transition初始及有序GPI判断；[Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)新增内部input2 row3原运动描述，和完整原计划载荷／策略／上限／原GPI几何对应。普通／地面来源分别是同次capture和原completed READ风险，原期限不续期。

Joint仍只检查原名义段和独立启动假设。整个走廊并集×独立高度区间的净空包含、完整StartPermit及正式结果未完成，source认证／许可标志仍false，Compiler pending保持。原25目标和其它实质开发不缩减；8源码／18文档准确范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：目标6／12的原scope接收历史与整组阶段尾核

[RAW](../src/Vehicle/VehicleStartupRawIngress.md)原身份在首回调前锁存，坏帧／嵌套／丢弃原记录失效，prequeue不成为新step事实。[Seal](../src/Vehicle/FleetStartupMemorySeal.md)不可携带导入，Txn在完整逐机phase及所有回调后核同一原整组，成功才供最后commandContextCurrent消费；prepared身份变动先经原完整gate，不跳过seal。

这不是整目标完成：空间包络包含、最终StartPermit／正式结果与其余五类仍须开发；Compiler pending保持。10源码／17文档不计为完成的功能数量。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 456：目标6／12的独立原生依据移交，不新增整目标验收

两类真实Review仅对完整同次满足报告创建[Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)；Authority从实际已确认Review取得同ptr并在移交前后重核，Txn独立持有到原生最终write前memory栅。原材料／原Review截止及UTC水位不因换owner续期，portable map／digest不是原生来源证明。

原pending保留：运动包络包含关系、普通RAW首回调前scope锁存及全组motion最终尾栅尚待实质开发；完整StartPermit／正式结果、可信性能、自愈、跨站、长期运营和真实模型生态仍在。13源码／19文档不能增加整目标完成计数。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：目标6／12的同次启动后台报告，不新增整目标验收

[启动Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)通过实际普通／地面审阅消费，原冻结来源和首份风险保留，完整报告先形成再进入最终摘要。旧规划结果匹配仍限原范围，失败／未知／违规不成为可已阅状态；取消、原期限及地面预约排空均接源码。

原pending仍阻断带资料任务贯通部署；两Authority独立移交后到最后write、已审运动包络、完整StartPermit和正式结果未完成。原25目标及六类实质工作不缩减，不能将本批11源码／18文档当功能完成数。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：目标1／6／12增加显式工程datum对应消费

[目录关系12字段](../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)与[原生Datum14](../custom/src/Swarm/FleetMissionClearanceDatum.md)接到原普通／地面审阅，固定原scene／配置／版本／原点来源；小证据重捕不换原基准，必需材料纳入artifact。动态receipt只校验不hash，修复真实VERSION被旧exactKeys拒绝的错配。

这不是实测高程／当前PX4内部valid认证，不能据AMSL字样、曾收原点或有限值开放启动。原净空pending及真实启动solver／两Authority至最后write／StartPermit／正式结果继续，原25目标和六类实质剩余保持；整体约40%仍只源码工作量粗估。11源码／17文档是局部范围，非实操通过率。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 下方453为历史。

## 453：目标1／3／6／12的实际构型与资料时间来源

[Registry实际来源证据](../custom/src/Swarm/FleetRegistry.md)与[Source](../custom/src/Swarm/FleetMissionClearanceSource.md)连到原普通／地面审阅，不从输入配置hash、导入护照或旧lastReview恢复实际来源。当前本地批准基线、实际对象及来源身份和资料窗口均须对应；动态剩余时间不参与稳定风险hash，但只缩短原审阅／READ期限。

这不增加实物包络认证、datum对齐、同次实际启动净空结果或最终发送许可，原pending和六类实质代码保持。未观察的物理改装仍需工程检查，不宣称软件可识别所有换桨／载荷变化。15源码／16文档只本批范围，整体约40%仍为源码工作量管理粗估，非通过率；本批只优化地面站源码及交接材料；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余不缩减。 以下452为历史。

## 452：目标6／12增加真实规划接触模型，不是启动净空完成

[输入2／接触组件](../custom/src/Swarm/FleetMissionContactModel.md)与[报告3](../custom/src/Swarm/FleetMissionClearanceReview.md)已由原后台Compiler实际生成，PlanStore同源匹配支持新旧明确配对；不是只加允许接地布尔值。protected／support全段检查，有限pad例外只覆盖指定support∩terrain整个交集，旧input1不补尺寸或新默认。

原wire与声明静止参考分开，PX4 LAND停止语义不能由p7证明。真实构型／datum／资料当前有效、同次原生启动报告及最终write与StartPermit／正式结果仍待完成；原pending继续阻断，原25目标和六类实质工作不减少。9源码／18文档仅本批范围，非验收／实操／工时完成率；未编译、运行、测试、放行、更新PR或打包，下方451为历史。

## 451：目标6／12后台编译与取消进入实际入口

[原生Worker与完成边界](../custom/src/Swarm/FleetMissionReviewCoordinator.md)及三个现有consumer已接源码。重型规划计算不再由该Coordinator同步分支执行，取消整份丢弃、实际finished后核来源才完成／归档；旧结果不能覆盖新输入。不是即时取消或全GUI响应性能证明。

原净空pending仍真实阻断；接地、构型／datum／资料时效、同次启动结果和最终write及正式机载结果未完成。原25目标及六类剩余保持，整体约40%是源码工作量的管理粗估，非验收／测试通过率；本批15源码／17文档不能提高整目标计数。未编译、测试、运行、放行、更新PR或打包，下方450及更早为历史。

## 450：目标6／12的规划结果开始由真实编译消费者生成

[范围明确的完整段源](../custom/src/Swarm/FleetMissionJointSpatialReview.md)与[报告schema2](../custom/src/Swarm/FleetMissionClearanceReview.md)进入Compiler完整plan，环境计算不再仅为未调用helper；归档严格核完整原来源，新报告随contentHash留存。规划未包含startup显式说明，不以假启动／最终reviewFingerprint借壳。

原pending仍阻断，当前不是上传／启动净空准入完成：接地上下文、实际构型／datum／时间、同次启动报告与最终write固定消费待完成。原25目标、六类剩余代码保持；9源码／17文档仅源码增量，未编译、测试、运行、更新PR或打包；下方449及更早为历史。

## 449：目标6／12增加真实几何核心，实际净空准入尚未完成

[精确扫掠／闭并集](../custom/src/Swarm/FleetMissionClearanceGeometry.md)与[声明模型报告](../custom/src/Swarm/FleetMissionClearanceReview.md)已写源码并登记，覆盖整个名义段及其机体／误差外包，不以航点、独立cell或AABB代替一般检查。范围不全、计算Unknown或起降接地上下文缺失不能成为模型满足。

Compiler几何pending仍阻断，实际规划／启动同源消费者、接地政策、场地时效／真实构型及最终许可仍待实现；并非目标6／12或原25项完成。其余六类工作保持，5源码／15文档仅静态源码增量，未编译、测试、运行、更新PR或打包；下方448及更早为历史。

## 448：目标6／12完成本批资料通路，不是整目标净空完成

[版本化场地／逐机包络](../custom/src/Swarm/FleetMissionClearanceInput.md)经原生严格校验、[区域来源2](../custom/src/Swarm/FleetMissionDraft.md)、原Compiler约束及PlanStore完整内容进入准确记录；活动成员同序匹配，余段换机不能借旧包络冒配。两处历史／修复恢复同步版本预检，仍需原生校整份来源。

当前合法新声明仍有几何审阅pending的真实阻断；无solver、现场认证或新增许可。覆盖／障碍／地形／走廊整扫掠、起降接地、实际启动固定消费、最终StartPermit／正式结果及其余五类代码保持。原25目标不缩减，不按25路径或批次增加验收数；未编译、测试、运行或放行，下方447及更早为历史。

## 447：目标6／11的业务阻断进入真实门禁，不新增整目标验收

[风险原因](../custom/src/Swarm/FleetMissionRiskEvaluator.md)不再只显示：Coordinator真实needsReview消费共享严格检查，Deployment同时核当前冻结与准确归档，普通／地面启动审阅独立核原记录。空code严重信号采用同一fallback进入riskItems与blockingReasons，不因空字符串被去重函数忽略。

当前只闭合既有风险判断与原生消费者，不认证工程场景、净空、最终StartPermit或正式机载结果。完整场地／机体包络净空和其它五类实质代码、原25目标保持；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方446及更早为历史。

## 446：目标6补原报告数值的精确比较，不新增整目标验收

v4高度关系门槛使用[原字段精确线性组合](../custom/src/Swarm/FleetMissionAltitudeArithmetic.md)，不再先舍入差值或毫米换算后比较；保留原六容差、reference20／sample20及实际scope／write消费，无新权限或DTO。修复了静态可推导的零／贴边误通过，不表示实际工程测试已做。

仍需版本化工程场地与机体误差资料、真实启动风险内的完整扫掠净空结果、最终StartPermit与正式机载结果；其余五类实质开发及原25目标保持。当前Joint几何只在空间视图，不能当已接启动许可。未编译、测试、运行或放行；下方445及更早为历史。

## 445：目标6增加显式高度关系门槛，仍非内部参考认证

新增明确选择的v4：原始ALTITUDE的三差值相对首次审阅基准检查漂移，对原点海拔／HOME海拔／HOME localZ检查残差，六项容差由工程师明确给出。普通／地面已阅材料保存原样本，当前身份只隔离正常动态值且仍执行原基准容差比较；真实启动scope内逐包保留越界／失源／坏帧／断档，后续正常包不能自行恢复该scope。

原v1–v3保持，v4沿v2 motion／heading和固定HOME，不混用v3 HOME迁移。此条件不证明共同原点、内部valid、同采样、样本鲜度或净空；scope建立前等待期不算连续覆盖。最终许可、正式结果和其余五类实质源码继续，原25目标不缩减。本批只做源码与限定静态核对，未编译、测试、运行或放行；下方444及更早为历史。

## 444：目标6的高度观察材料，不新增整目标验收计数

[decode18／state8](../src/Vehicle/FleetStartupAltitude.md)与[独立native27](../src/Vehicle/VehicleStartupAltitude.md)接入启动风险的固定11字段诊断；逐字段finite／NaN／Infinity／哨兵和三项同包差值明确保留。协议报文字段不认证PX4分支有效性、传感样本鲜度、HOME／local／ODOM共同原点或净空。

首次普通／地面风险材料冻结；当前比较排除新动态诊断而保留所有原门槛与首次完整artifact/hash。原25目标／六类实质剩余工作保持，未来显式参考政策／最终StartPermit仍待开发；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，不按批次／文件数计完成率。下方443及更早为历史。


## 443：目标6的原生原点连续性，不新增整目标验收计数

[消息49纯解码](../src/Vehicle/FleetStartupGlobalOrigin.md)保留经纬E7、AMSL毫米和完整uint64时间文本，[独立Vehicle账本](../src/Vehicle/VehicleStartupGlobalOrigin.md)区分从未收到、已观察、不可用与接收栈内。stable18进入Readiness／地面READ物理冻结／Transition；实际发送scope冻结原参考并在最终write前检查，重复合法同值报文不因receipt／age使审批抖动。

这不证明HOME/local/ODOM/估计器同原点、同采样、内部有效状态、真实净空或最终StartPermit；未自动请求消息，不改变旧6flight／2coordinate／ODOM schema或policy v1–v3。原25目标、六类实质剩余工作和工程师验证均保持，不能按批次或文件数上调完成率。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方442及更早为历史。


## 442：目标6／12的联合名义请求几何，不新增许可或验收计数

[纯值联合视图](../custom/src/Swarm/FleetMissionJointSpatialReview.md)以完整canonical请求顺序与独立启动参考对同归档区域进行静态检查。每机几何／动作缺口、全部目标顺序、原实际间距、区域来源状态与有界计算结果独立显示；没有按成功子集冒充完整任务。

原归档报告仍只覆盖原scope；[现有明细](../custom/src/Swarm/FleetMissionSpatialPanel.md)新增只读摘要与分页，新capture信封为9字段。零候选、合法空区域、计算完成或覆盖完整均不是可飞／净空／4D认证。最终StartPermit、正式机载结果及其余六类实质源码继续，原25目标保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方441及更早为历史。

## 441：目标6／11／12的真实输入阻断，不新增验收计数

[空域输入链](../custom/src/Swarm/FleetMissionAirspaceInputGate.md)已经接到原生报告、风险评估、needsReview、归档与审批，而不只是显示字段。坏原值／双源冲突／损坏间距不会在替代空列表或默认5米上继续几何；坏审阅替换旧审阅。原生冻结值形成新归档，有效来源还核私有Intent当前指纹；任务通知重入不续用旧批准。

合法空列表、未提供与坏值继续区分；列表合法不是几何、完整性／时效或权威空域认证。仍无全路线＋启动＋终结联合审查、最终StartPermit或正式机载结果，其余六类实质源码与原25目标保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方440及更早为历史。

## 440：目标6／12完整任务显示，不新增安全验收计数

[共同视图](../custom/src/Swarm/FleetMissionSpatialReview.md)从同一准确归档计划／reviewEvidence取源，保留所有目标、规划路线、返程、起止动作与原完整Builder生成的协议项；[界面](../custom/src/Swarm/FleetMissionSpatialPanel.md)不借单成员合成plan凑编码成功，不把生成请求称已上传／已执行。

原归档zone列表／报告和实际使用间距保留，空列表／缺输入／坏类型／旧未知分开；原报告scope仍是规划出航及返程，不覆盖新增起飞假设或终结下降。真实空域联合检查、可信原点／净空／最终StartPermit与正式结果仍待完成；原25目标及其余实质源码保持。仅源码与静态审阅，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方439及更早为历史。

## 439：目标6／12的冻结启动几何与独立候选

当前实际normal／ground风险经[纯值helper](../custom/src/Swarm/FleetMissionStartupSpatialReview.md)和[空间面板／地图](../custom/src/Swarm/FleetMissionStartupSpatialPanel.md)显示完整目标集与三种独立名义线段，原AMSL编码区间、缺口及过期历史保留。Root核准确记录而非最新草稿，不从实时Vehicle补点。

[静态间距候选](../custom/src/Swarm/FleetAirspaceConflictChecker.md)要求明确声明阈值，并单列投影支持域、全对数及有界明细；无时间不造时刻，候选少不等于安全。真实地形／禁飞区、完整原任务空间及最终许可未完成，原25目标和六类实质工作保持。未编译、运行、测试、刷机或更新PR／包；下方438及更早为历史。

## 438：目标6完整启动政策输入，不新增执行许可

[v1／v2／v3表单](../custom/src/Swarm/FleetStartupPolicyEditor.md)提供全部必填值／单位／范围、局部速度与全局速度关系、航向累计约束和HOME零位移边界；明确采用进入原完整profile／参数指纹／CAS路径。原始JSON与未采用表单不能互相静默覆盖，旧策略不能绕过屏幕上的待处理输入保存。

[实际编辑来源](../custom/src/Swarm/FleetExecutionProfileEditor.md)核草稿选择代次，Root冻结真实backing Draft防同facade同机号同revision误写。没有更改原生schema、启动门禁、净空或最终许可；原25目标及六类实质工作继续。未编译、测试、运行、刷机或更新PR／包；下方437及更早为历史。

## 437：目标6／13的当前部署启动工作台

初次与地面续审的[人工单步入口](../custom/src/Swarm/FleetMissionStartupWorkbench.md)消费原实际会话、审阅、双账号条件批准、认证移交和三步事务；[Panel](../custom/src/Swarm/FleetMissionStartupPanel.md)明确展示冻结目标、原生原因、逐机请求／观察／可能提交及排空。地面风险新增只读白名单，不从读取匹配推导安全。

真正启动会请求Seq0／普通ARM／MISSION；startupPermitGranted=false仍不能解读为不会发送。没有独立最终许可／净空认证或正式任务结果，原25目标及六类实质工作继续。本批未编译、测试、运行或放行，未更新PR／包。下方436及更早为历史，“无启动消费者”只由本批有限UI接线补充。

## 436：目标15的运行简报、本地收件箱和接收回执

[真实Run交接](../custom/src/Swarm/FleetMissionHandoffWorkflow.md)从实际选定owner捕获简报，绑定真实taskUUID和原完整记录版本，不采用任意输入taskId或当时最新提案。发起／接收／拒绝／撤销核实际Session身份与角色收据，候选一次消费；分页与历史详情已接源码，未知观察不升级为成功。

只覆盖本实例的历史简报人员确认，不释放占用、不改变机载任务或控制权，导入不恢复来源。跨站／企业身份／实际交接提交／中途加入，以及完整执行、可信性能、自愈、长期运营、真实模型／生态继续；原25目标不变。本批未编译、测试、运行或放行，未更新PR／包。下方435及更早为历史。

## 435：目标6／13的余段地图与逐项只读入口

地图与[分页核对区](../custom/src/Swarm/FleetMissionRemainderRoutePanel.md)共用[同一新记录投影](../custom/src/Swarm/FleetMissionRemainderView.md)，不从原Run序号或旧区域图推导新路线。完整规划几何与完整编码分别可用，失败不绘制／返回成功前缀；记录过期保留历史说明且不能换源续用批准。

只是实际源码接线，不代表目标6／13或原25目标完成；净空／StartPermit／正式接替和其余六类实质工作仍在。未编译、运行、测试、刷机、更新PR或打包。下方434的地图缺口由此有限覆盖，其他边界不变。

## 434：目标6／13的新余段审阅来源接通现有地面部署界面

[新审阅上下文与采用入口](../custom/src/Swarm/FleetMissionRemainderPanel.md)分离历史Run来源、未结束编辑与当前新记录。原意图审批和唯一[地面部署面板](../custom/src/Swarm/FleetMissionDeployment.md)复用，不伪造区域Draft身份；刷新指向所选计划成员，过期不回退旧来源。

仅新增源码可操作路径，不增加整目标验收计数。上传仍须当前证据、实际身份及独立批准、地面未解锁／无占用／读回检查；不新增启动入口、空中接管或旧占用豁免。原25目标及六类实质开发继续，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方433及更早为历史。

## 433：目标6／9／13的独立余段提案源码增量

实际Run只读取源、人工包含式原航点后缀、目标自身起返点和新执行profile已接[规划/审阅链](../custom/src/Swarm/FleetMissionRemainderPlanner.md)。新来源进入Compiler与PlanStore一致性检查，保留原安全政策，当前能力/健康/模板重新审阅。

不增加整项目标验收计数：CURRENT/REACHED不证明已完成前缀，后缀包含完整停留；既有Run/成员占用不变，新提案不是已执行的替补。正式机载改派/租约、真实能耗/净空、完整执行与结果、跨站、长期运营和真实生态仍未完成。未编译、测试、运行、修改PX4或打包；下方432及更早为历史。

## 432：目标9备用独立预算，不新增整目标验收计数

原[草稿](../custom/src/Swarm/FleetMissionDraft.md)新增可选完整备用策略，旧编辑源不注入默认值；[编译器](../custom/src/Swarm/FleetMissionCompiler.md)分离备用路径，原[调度器](../custom/src/Swarm/FleetEnergyScheduler.md)解释不完整备用而保留主机账单。新增[编辑入口](../custom/src/Swarm/FleetReserveEnergyPlanPanel.md)以原对象和版本保存，不授予执行权限。

覆盖“待命＋自身进场＋完整目标航点重做＋自身返程”估算，不等于剩余任务续接、实际自愈、可信性能或真实电池模型。目标8／11的成员资格和风险门禁保持；所有原25目标及六类剩余实质源码范围不变。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

## 431：目标5／9的来源与鲜度接线，不新增整目标完成计数

目标5公开健康快照的电量改用实际原生投影；目标9逐机账单增加[飞控报告估算及来源](../custom/src/Swarm/FleetEnergyScheduler.md)，[现有规划页](../custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)解释报告年龄与未知原因。Guided审阅既核稳定来源／内容，也在原预算上复核实时扣龄剩余量；不借旧报告恢复批准。

只支持唯一明确的推进用途报告；用途未知、多推进、过期和失源不由缓存替代。实际接收不是远端测量或性能认证；实物改装、真实性能、多电池与耗能模型仍未完成。其余受控执行、自愈、跨站、长期运营和模型／生态工作保持，原25目标未缩减。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

## 430：目标9的原身份账单，不新增整目标完成计数

目标9新增[逐机任务／返程预算](../custom/src/Swarm/FleetEnergyScheduler.md)和[规划界面展示](../custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)；目标8的核心性能未知口径、目标11的原成员身份风险同步。每架拒绝成员仍留账单，备用不得替换原主机；组级瓶颈与累计缺口显式说明未知，不将有效子集当全组。

真实电池遥测／耗能模型、备用待命与接替路线、可信性能来源尚未完成；没有新增执行或放行权限。其余完整受控执行、自愈、跨站、长期运营及真实模型／生态继续。仅源码及静态阅读，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；25目标完整范围不变。

## 429：目标19的运行观察报告，不新增整项目标验收计数

目标19新增[独立Run报告](../custom/src/Swarm/FleetMissionExecutionReport.md)及[操作入口](../custom/src/Swarm/FleetMissionExecutionReportPanel.md)，覆盖选定实际来源、冻结摘要、逐机明细、显式JSON导出和离线只读打开。目标6可据此交接“观察到了什么／哪些未知”，不能据此获得起飞许可、恢复任务或认定执行成功。

报告hash不是签名，source元数据不是身份认证，MODE提交覆盖不是成功率；本地无MODE提交也不证明此前未ARM或未改航点。仍缺正式机载结果、完整遥测关联及其余六类实质开发。原25目标保持，未进行编译、运行、测试或放行，不能按429批次号计算完成率。

## 428：历史归档规划恢复为独立草稿（源码阶段）

[本地版本库](../custom/src/Swarm/FleetMissionPlanStore.md)增加指定提案会话的全部历史版本摘要与精确记录取源；[历史规划入口](../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)用于在现有启动加载的档案中选择具体版本、预览来源并明确创建可编辑副本。不以“最新版本”替代已选版本，不依赖原失败部署或426对账；旧记录没有完整编辑源时明确不可恢复。

恢复仅复制规划内容，重置地图基准确认，并复用427的独立草稿、上一份备份及替换确认。历史sourceTaskId只作来源；重新审阅使用当前提案会话身份及当前设备证据，不恢复旧审批、机载状态、任务执行或控制权。档案只读恢复不解除写入隔离，后续归档仍可能因冲突或容量被拒绝。

本批接线与待验证边界见[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／应用／设备，未修改PX4、刷机、提交、更新PR或打包。只覆盖已加载本地有效档案的规划编辑恢复，不是损坏档案修复、通用任务编辑或机载任务续飞；原25目标及六类实质工作继续。下方427及更早为历史增量。

## 427：保留完整原草稿并建立人工修复副本（源码阶段）

[区域草稿](../custom/src/Swarm/FleetMissionDraft.md)现在随编译计划保存有界完整编辑源，不再尝试从生成航线反推原多边形；[编译器](../custom/src/Swarm/FleetMissionCompiler.md)核对源与实际规划字段一致，[版本库](../custom/src/Swarm/FleetMissionPlanStore.md)把源纳入原内容摘要和历史存档。旧记录缺源时明确不可恢复，通用任务不会被伪装为区域编辑器草稿。

[失败部署取源](../custom/src/Swarm/FleetMissionRepairDraft.md)基于原失败／取消部署与已排空的426核对结果，保留different／unknown，不将其称为已修复。新[操作入口](../custom/src/Swarm/FleetMissionRepairDraftPanel.md)只创建独立本地编辑副本，保留上一份草稿供明确切回；新副本重新确认WGS84／AMSL基准，再走原新预览、原生审阅、新记录版本及人工审批。不是新taskId，不恢复旧审批，不自动上传、清空、回滚或启动。

本批范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定源码核对见[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)，待实际验证事项见[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／应用／设备，未修改PX4、刷机、提交、更新PR或打包。跨重启历史草稿恢复入口、逐项修复建议及完整受控执行、可信性能、自愈、跨站、长期运营、真实模型／生态仍待开发；原25目标保持，以下426及更早是历史增量。

## 426：失败／取消后的独立整组只读核对（源码阶段）

目标6继续补任务部署失败后的恢复操作链：[独立核对工作流](../custom/src/Swarm/FleetMissionReconciliation.md)从本实例原失败／取消部署冻结期望任务，由当前有权操作员明确发起；不要求旧回读全部匹配或旧批准仍有效。[传输](../custom/src/Swarm/FleetMissionReconciliationTransport.md)使用独立RAW INT读取和一次性结果，[界面](../custom/src/Swarm/FleetMissionReconciliationPanel.md)逐机区分相同、不同、未知与尚待处理。

实际成功COUNT0与协议失败分开；诊断不覆盖普通任务显示缓存、不生成启动收据、不自动修复／清空／上传或启动。全组逐机读取不是原子快照；相同仅是该次受支持规范化内容的比较，未知不能当相同。不支持的机型／协议、失效来源和无法规范化内容保留限制，不能据此声称所有异构机已支持。

本批源码／文档范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，静态审阅与待执行项目分别见[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／认证／应用／设备，未改PX4、刷机、提交、更新PR或打包。修复草稿及完整受控执行、可信性能、自愈、跨站、长期运营、真实模型／生态仍待开发；原25目标保持，以下425及更早为历史增量。

## 425：普通任务回读差异诊断（源码阶段）

目标6的普通任务部署增加逐航点回读差异诊断，是失败部署重新对账的前置，不代表整组重新读取、修复草稿或完整执行闭环完成。 [比较合同](../custom/src/Swarm/FleetMissionReadbackDiff.md)、[实际部署接线](../custom/src/Swarm/FleetMissionDeployment.md)与[界面说明](../custom/src/Swarm/FleetMissionReadbackDiagnosticsPanel.md)分别说明数值、来源和展示边界。

界面先显示逐机历史摘要，再按需展开有限明细；缺项、null、未知、规范化失败和截断不能显示为“没有差异”。原回读匹配与启动审阅门槛保持；诊断来自普通解码后的MissionItem，不证明独立RAW INT来源、此刻机载内容、控制权或起飞许可。内存部署历史仅保留摘要，不保存无限差异列表。

本批仍不提供失败后整组独立重新读取、修复草稿、自动覆盖或回滚。原25目标和六类实质开发继续，不能按批次号或文件数计算完成率。源码范围见[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，工程师待验证项目见[交接说明](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／应用／仿真／设备，未改PX4、刷机、提交、更新PR或打包。以下424及更早是历史增量。

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

## 418：新增局部可操作路径，不增加完整目标验收计数

目标1–4已有的原生能力批准/撤销现已接[实际工作流与UI](../custom/src/Swarm/FleetCapabilityReview.md)，当前目标6/8/13继续消费本地有效声明；目标20增加原双账号、操作与版本绑定的结果审计。作用仅限本地工程采纳，不认证硬件/数值性能，不自动改变正在执行任务或授飞行许可。

本阶段16源码/资源、17文档；context6/approval14未变，旧native bool仍为受信后端。完整受控执行、可信能力性能、完整自愈、企业跨站、长期运营和真实模型生态六类仍有实质缺口。没有编译/运行/验证结果，不能给出整体完成率或用接线代替25项目标验收；下方旧无UI描述仅由本节限定覆盖。

## 417：工程目录显式 reload 的局部进展

目标6的工程前置新增[纯完整预览与最终 CAS 导入](../custom/src/Swarm/FleetFirmwareAdapterCatalog.h)、[固定文件独立审批工作流](../custom/src/Swarm/FleetFirmwareCatalogReload.h)及双入口[目录面板](../custom/res/Merivus/FleetFirmwareCatalogPanel.qml)。完整旧/新声明与原 engineeringReview、差异和双哈希进入同一次实际 scope/目录身份绑定；30秒、原账号 TTL、最终文件复读及纯围栏后只替换内存目录，不写档案。旧任务 review 失效，不停止 Run、不释放占用。

它不是固件签名/硬件认证、飞行许可或机载执行结果；原启动自动 load 也未改成强制双账号。现有全球任务启动另补 ABS 水平/垂直且非 CONST_POS，不扩 RAW、通用 Health、室内/GNSS 硬件。19个源码/资源路径仅是[增量范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不是完成计数。

原25目标及受控执行、可信能力/性能、完整自愈、企业跨站、长期运营、真实模型/生态六类仍有实质源码缺口；可信 HOME/local/净空、最终 StartPermit、正式机载实例/结果没有被目录重载完成。未编译、测试、运行解析/helper/UI/认证/应用/SQL/设备或放行，未改 PX4、刷机、更新 PR、打包；不能给出有依据的整体百分比/日历工期。以下416及更早保留为历史，旧目录入口缺口由本节运行中部分覆盖。

## 416：逐机配置正向输入，不增加整体完成计数

Draft/Planner 增加[每成员完整 profile CAS](../custom/src/Swarm/FleetMissionDraft.md)，Guided 从[真实12参数只读接口](../custom/src/Swarm/FleetMissionReviewCoordinator.md)取得待绑定哈希，保存前按实际来源稳定指纹再次读取。角色 reserve 保留但不参与活动 assignments；删除再加同号不继承。错误返回不证明未提交，禁止自动重试覆盖新版本。

核心字段结构化、高级完整 startup JSON schema1–3 为本轮产品边界；不声称所有政策已表单化。缺政策可保存意图，原启动门禁仍拒绝；不 refresh 或写参数。本阶段范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，只是局部源码接线，无编译/UI/设备或运行验收。

原25项目标及六类剩余范围不变，不按文件数量计完成率：完整 StartPermit、可信 HOME/local/净空、固件目录真实操作入口、正式机载任务/结果及长期运营等仍有实质开发。下方旧批次作为历史，不以旧“不带 profile”覆盖当前源码，也不以本节抹掉其他未完成项。

## 411–415：局部接线状态，不增加完整完成计数

目标13增加[本地重启/历史恢复面板](../custom/src/Swarm/FleetOccupancyRecoveryPanel.md)，与目标6当前原生来源/角色/身份边界衔接。原生current27、result13和持久化诊断13键由Root值投影消费；8键收据只指向私有原审阅，既不选择替代后端也不授飞行许可。

目标13仍缺本轮真实unknown的受控处置、离线/报废现实控权回收与正式机载自愈；目标6完整许可/机载实例/结果仍未完成。原25项/六类继续，不按文件或局部模块数量估算完成百分比。两个源码入口已接，原生recoveryUiIntegrated=false与Root runtimeValidated=false明确未验收。

本阶段9源码/资源、18文档；只读审阅及限定文本检查，不编译、测试、运行或放行。下方旧无UI状态由本节接线覆盖，不覆盖其它未完成项。

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

目标6/13：当前只为连续性产品路径加实际本地占用与本地CAS约束，不宣称所有规划器已获真实占用许可；目标15：实际源sticky并非跨站控制交接；目标18/20：本实例未知保留不是跨重启证据全集/签名。完整受控执行和长期运营仍有实质代码缺口。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](../custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

目标8：全局及四类角色政策进入同一候选池；目标9：备用角色覆盖与同时覆盖缺口分开；目标13：只对全部缺口完整匹配生成可应用的本地提案，同批不重复承诺候选。目标13的真实受控任务改派仍未完成；角色图/本地CAS不是飞行许可、机载改派或跨站租约。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](../custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

目标1–4：新增原生明确能力批准/撤销与构型绑定，但不是可信硬件或二进制认证；目标6/8/13：当前成员、候选及显式最终角色政策统一核当前审阅，角色等价替补仍待；目标20：归档固定能力审阅证据，不是签名/账户权限证明。以上均不使原25项目标自动验收完成。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

## 371–375：局部部署组生命周期推进，不升级25目标验收

新增两个 `ForController` native 切换、Guided 固定 prepare/select API 与指挥中心/规划页共用的 [LifecyclePanel](../custom/src/Swarm/FleetMissionLifecyclePanel.md)。实际 Root 的 begin → 私有 permit mutation → 新 scope/原 ID end 连续持占用；只读公开12键 metadata、每组6键，不输出原生引用。面板双步确认、默认关闭/取消、禁止 Enter 提交并冻结原 view ticket。这些是8源码/19文档的实际增量，不是运行验收或全部任务功能完成。

新 current 不自动改变 selected、不继承登录/批准、不停止旧 Run 或释放其占用；原 quiet/shared5/8组上限保留。提交事实与 end 独立，失败尝试仍消耗容量，allocated=false 不证明未计费，后置失败也不回滚已经发布的组。`outboundIntegrationComplete=false`；目标6/13/15/18/20等不能因新增界面或工厂 API 自动记为完成，六类及完整25目标仍有实质源码缺口。

已完成限定文本审阅，具体数量见本轮静态审计，本机未编译、测试、解析/运行 helper、认证、应用或设备，未更新 PR/包。以下366–370及更早矩阵说明保留为历史，旧“入口/API未实现”已由本批源码增量覆盖，但验证状态不变。

## 366–370：应用引用接线推进，不上调目标验收状态

当前草稿失效后只撤销原scope意图；不能核实撤销则私有 `retiredCurrentScope` 持续拒同组 current 入口/回调，selected观察仍可读，不按超时复活。5源码不是独立交付包，Qt exact Int/QObjectStar传参与销毁时序未经运行验证，不能据此上调任何目标状态。

现有业务已接私有 Lifecycle/14个原生服务引用和30个 Fleet 入口/24个来源回调，Lifecycle/Episode 类型已登记，initial 由构造提供。三个 native call 接口在同步栈内阻切代，pure scope/回调不自拒；finally 用原 owner+ID 清理，失效或未知/已 queue 事实不作回滚。监视四值同 selected scope、token 含选择代次、完整 issued 收据路由原 retained Close，不按 UUID 搜索或回退；显式监视登录不结束 Run。

仍缺新部署/历史选择实际产品入口及专用 transition 原生 API，`outboundIntegrationComplete=false`；不能把注册、引用迁移或模块存在当目标6/13/15/18/20或全部25目标完成。执行闭环、可信能力、自愈执行、企业跨站、长期运营、真实模型/生态六类实质开发继续。实际5源码/18文档，已完成限定文本审阅，不是目标验收或运行/安全通过；未编译、测试、运行、更新PR或打包。下方旧未注册/未迁移说明仅为历史状态。

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


## 340–344 状态边界：多部署基础核心，未成为可操作功能

新增 [Episode / Lifecycle 原生核心](../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)，每组14个新 owned 对象和 signed-out Session，旧实际对象/Run/占用保留，五个实际共享来源冻结；8次分配含初始和失败尝试，不驱逐历史。全部 retained 组用固定18键 native 静默证据，不以 OS/public getter 或未知占用替代。

**矩阵中的既有目标不因本批自动升级为完成：未注册 QML、未替换 Guided、无新部署入口，现有应用仍单链。** sticky outbound 状态/helper 已定义，但 owned C++ 门禁、QML epoch/source callbacks、共享 Role 身份意图和 selected retained Run/Series/Close/Session 一致投影仍需真实接线；不可称多部署隔离已完成。

原25目标/六类不缩减；本批限定检查结果已记录，未编译、测试、运行 API/helper/认证/应用/设备，未更新 PR 或打包。

## 335–339：关闭监视UI接线，不改变目标验收边界

[运行监视面板](../custom/src/Swarm/FleetMissionObservationPanel.md)在CommandCenter与Planner始终提供入口，不要求有效草稿、原上传权限或在线/地面遥测。view只读，request不结束监视，三个确认与另一账号专用审批后才沿原生关闭；每入口token隔离，unknown保留，local关闭不等于停飞、远端移交、可重新分配或Completed。

这是任务生命周期/操作审阅入口增量，不代表目标6/13/15/18/20或全部25项目标已验收。335–339后当前已有关闭UI源码；330–334无UI是历史状态。未编译、测试、启动应用/认证、操作设备、更新PR或打包；其余六类缺口不缩减。

## 330–334 状态补充：本地监视结束不是完整任务完成

[关闭监视后端](../custom/src/Swarm/FleetMissionObservationClose.md)已经接入实际Session/Role/Series及唯一当前Txn/Run/Swarm/Orchestrator，专用不同账号审批与三项确认。18键范围包含完整binding与实际占用/撤销快照，安全撤销不冒充map已空；关闭/实际占用仍在/结果未知分开，不认证停飞或允许成员重新分配。

此增量推进任务生命周期与操作审计，不使目标6/13/15/18/20或其余25项自动验收通过。仍无新关闭UI、正式机载结果、跨站控制移交和完整自动执行；仅源码/限定静态审阅，未编译、测试、运行系统认证/应用/helper/设备，也未更新PR/包。下方325–329及更早说明为历史批次。

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

本文件对应“目标全部加入”的当前地面站实现边界。它把产品目标、代码入口和验收缺口放在同一张表里，避免把界面存在误认为飞行能力已经验证。

244–249已有actual StartupTransaction/固定三步sender/持续RAW风险与typed模型、实际八对象后端及原profile政策源码；本批有限源码静态收口已完成。取消不回滚或保证未起飞，mode观测不认证任务执行。HOME有效分支/重置再审、local frame/运动/转向、最终StartPermit/净空/完成与其它六类未完成；未编译/运行/测试、改飞控或更新PR/安装包。下方为历史批次。

最新239–243：新StartupAuthority一次接管实际已消费独立账号与全记录/RAW/健康/目录/BeforeArm事实，原身份与最早30秒期限不新起算；旧Review/Content自然失效不能替代独立owner的当前源校验。typed普通地面POSCTL候选不是机载canArm/中立遥控/净空或StartPermit，原manual/未认证风险不改；当前仅preARM且三个飞行标志false，无发送/UI，未来实际seq0/正常ARM/mode-only和完成归属仍待开发。原六类全目标保持，未编译/运行/测试或更新PR/安装包；下方为历史批次。

最新234–238：新StartContentCheck接真实原生有作用域的完整RAW INT回读、全规范载荷比对及34字段成功依据；现有Qt面板显式撤销旧审批后申请当前部署整组只读核对，显示匹配/过期/排空。九参数StartReview绑定新18字段上下文，30秒受最早READ START截止，不由完成/已阅/独立账号提示续期。默认无Start/ARM、执行与许可false，顺序读取不证明远端原子/无线归属/硬件认证；完整启动/完成及其它六类源码工作继续，未编译/运行/测试或更新PR/安装包。下面为历史批次。

最新229–233：独立账号的条件启动意图审批已绑定actualReview/Deployment/Store/Session/Role/private owner与全目标来源，原30秒风险期限包含系统输入，上传依据不交叉消费。native风险v2保留RAW v1与boot回退，语义单调revision区分纯到达/GPclock和真实物理ABA；健康年龄通知只有实际风险gate/Leader变才撤销。审批/消费保留未解锁/未认证等全部原风险，三个startup/executable/flight字段始终false；后端接线没有启动UI或调用Vehicle。原25完整目标/六类剩余范围未完成，未编译/运行/认证/测试或更新PR/安装包。

最新225–228：FleetMissionEvidenceRefresh/原生读取/controller/UI接线，显式当前账号只读授权与旧审阅先撤销，最多32实际目标按版本→十二参数顺序采集；实际PRIMARY、epoch、成功RAW计数大于请求前基线，不把ACK/cache或字符串flags当新观测。120秒总/4秒阶段/100毫秒间隔，明确未发送busy/rate/缺状态才有限等待，不恢复批准或扩大协议；服务历史年龄不替代后续30秒参数审阅。硬件/固件/请求因果认证与启动/完成许可仍false，原六类完整功能未完成；未编译/运行/测试或实际调用接口。

最新221–224：有限上传租约绑定实际deploymentId，与60秒审批申请分开；消费后最多4分钟且不越过原操作员5分钟期限，双方实时角色与实际WTS桌面继续核对，锁定/注销/断连/未知拒绝，验证不延期。schema2/22审批和13grant不接受旧20/8授权；只用于本次地面上传/回读，不是跨站租约、持续任务控制或Start/ARM许可。未编译/运行/系统接口调用/测试，原25项完整功能仍未完成。

最新217–220：实际Windows本地系统认证、固定专用本地组直接SID角色来源、任务绑定的独立授权账号收据及消费grant已接controller/上传每步gate/UI。默认无配置仍拒绝，60秒包含系统输入和全部传输续作，不续期不回滚；不同SID不是两名人员证明，系统组不是企业权限服务，本地receipt不是跨站租约。风险已阅与上传审批均不授权Start/ARM，完整启动/完成及原25目标仍待开发；仅源码/静态复核，未编译/运行/认证/测试、更新PR/安装包。

最新212–216（2026-10-14）：任务Planner新增地面上传覆盖确认/目标版本及实际项数/逐机传输/取消排空/只读检查UI；当前操作者及策略快照进入原生gate，缺账号/额外批准明确拒绝。独立StartReview使用本runtime全组历史回读和当前原生来源，30秒风险已阅不恢复旧liveReview/ARM后的staging、不生成启动许可；新Start/ARM/模式/完成事务仍未实现。类型与资源接线只经静态源复核，没有编译/运行/渲染/测试；原25项目标未全部完成。

最新208–211（2026-10-14）：原始六通道flight-state账本、启动前置证据和任务序号/缓存wire相关已接线；未知扩展不填状态、重复reached不当新事件，boot-time回退保守撤销可用证据而不认证重启。真实起飞段/最低起飞高度提升与连接及AMSL上限显式推导，未解锁不当前置条件匹配。Deployment主动只读检查冻结目标，facade只给1秒snapshot-only值与有界摘要；没有独立启动许可/完成归属或UI按钮，原暂存隔离仍保持。未编译/运行/测试，完整25项目标仍在进行中。

最新 204–207（2026-10-14）：原始真实版本 ledger 与十二字段 canonical hash、源/年龄/回调 depth、首次/变化撤销旧参数已接线；新增 FleetFirmwareAdapterCatalog 原生工程声明、CAS/有界原子 JSON 交换与实际版本精确对应。显式 profile 要求鲜版本与声明匹配，目录/版本变化进归档和上传 source gate；事件只保存有界摘要/精确归档引用。Git5/Vendor3 短前缀、board/UID 与工程声明都不是固件/二进制/硬件认证，不自动监控物理文件，不生成启动许可；部署仍不可执行，完整受控启动/完成/UI及其它原目标仍缺，未编译/运行/测试。

最新 199–203（2026-10-14）：固定十二项真实参数的单调年龄、内容 revision、manager epoch 与实际接收链路观测已写入 ParameterManager；缓存/FTP/文件/本地写/刷新和连接/主次链路变化撤销来源。新增 VehicleEvidence，原生审阅/编码摘要→归档 hash→批准/部署最终来源门禁接齐；Fact direct callback 补生命周期保护。参数可用不等于可信固件/起降/执行许可，几何提案不造 profile；所有部署不可执行，完整启动/完成/UI、跨站和长期运营等缺口仍保留。未编译/运行/测试或更新 PR/安装包。

最新 195–198（2026-10-14）：`FleetMissionExecutionProfile` 严格保留显式执行要求并进入编译/覆盖/计划身份与归档；载荷 v2 支持速度、起飞、驻留及终点 HOLD/LAND 请求，五类命令完整 wire 比对和来源映射。LAND 保留进近高度与较低显式地面目标，量化后也不得重合；驻留纳入独立出航/返航能量预算，与明确 duration/窗口/相邻权威到达间隔矛盾则阻断。全部仍不可执行、未获得完成/起降净空/垂直参数/绝对时序证据，没有新增启动或 UI 上传，未编译/运行/测试。

最新 187–194（2026-10-14）：新增 `FleetMissionPayloadBuilder` / `FleetMissionDeployment`，地面几何上传、独立回读、完整批准身份、一次性确认、共享占用/排空和几何启动隔离已写入源码；底层协议重入/对象接管与原恢复任务的延迟、原请求/操作/内容关联同步收口。当前没有上传 UI，所有 Fleet 部署均 `executable=false`；速度/时序/起止动作和受控启动仍需开发。仅源码及静态审查，未编译/运行/测试；以下历史说明以本段为最新增量，不计作全部目标完成。

最新 183–186：`FleetMissionDraft` / `FleetMissionPlannerPanel` 接通区域选点与键盘输入、固定成员/角色、AMSL 参数、端点和辅助机路线编辑、逐机只读预览及显示版本审批。规划与原 GoTo/Shift 任务按交互 epoch/最终调用门禁隔离，暂停自动居中；没有上传或飞行释放。原生资源上限、声明坐标基准和 GUI 状态不能证明机体能力/净空/现有协议扩容。仅源码与静态审查，未编译/渲染/测试；下文批次说明属于历史状态。

最新 179–182：覆盖端点模式生成只读 3D 进场/独立返回，返航估算提高储备，完整路径接受高度上限审查；`FleetMissionPlanStore` 加入不可变计划/审阅版本和本地原子档案，批准回执/历史交接引用有精确版本身份。恢复不能恢复批准，重入/ABA 不得撤销新版本。仍无自动上传/飞行、地图编辑、跨站控制租约或长期数据库完成证明。

2026-10-13 区域规划增量：`FleetAreaCoveragePlanner` 经任务编译器 opt-in 接线，生成简单区域扫描航段并按机分配；编译保留点时间/成员返航/基准和独立时长，审阅保持估算时序未知并纳入能量预算。路线仍是纯数据提案，未运行/编译/测试，不代表地图编辑、感知覆盖、进场/返航生成、上传或真实执行完成。契约见 [`FleetAreaCoveragePlanner.md`](../custom/src/Swarm/FleetAreaCoveragePlanner.md)。

| 目标 | 当前代码入口 | 当前状态 |
| --- | --- | --- |
| 实际 Run 与受保护任务记录 | `SwarmMissionOrchestrator`、`FleetMissionExecutionObservation`、`SwarmController`、`FleetMissionContinuityPlanner`、`FleetOperationalMemory` | 325–329在同一模型MODE前登记独立taskUUID、原生身份/完整binding及private owner，generic mutation不可改；独立observationRevision/NOTIFY，Observing/ObservationClosed不冒充正式完成。Planner保守占用、专用原生工作流诊断，Memory关闭不计飞行成功；只有源码/有限静态审阅，正式结果认证/独立关闭审批/长期容量与原六类仍继续 |
| 明确 HOME 观察更新与逐机预算审阅 | `FleetMissionStartupHome`、`FleetMissionExecutionProfile`、`FleetMissionStartReadiness`、`FleetMissionStartupTransition`、`FleetMissionStartReview`、`FleetMissionDeploymentPanel` | 320–324新增schema3九字段/八字段HOME包线，原基线和跨步骤次数，实际ARM/MODE双叶/coord-only与ARM relative-only消费；逐机完整冻结范围、单位和未知态只读显示，旧v1/v2不升级，ODOM/ACK/HB/组/gap/两risk域与final比较不删。仅源码/限定静态审阅，不认证内部HOME有效性、共原点/同样本/因果/净空/最终许可，没有新增启动按钮；六类/25目标继续 |
| 本次启动后的连续运行观测 | `VehicleMissionExecutionObserver`、`FleetMissionExecutionObservation`、`FleetMissionStartupTransaction`、`SwarmController` | 315–319已接真实MODE write前切点和captured commit、独立pre-plugin来源、post-commit序号/重复reached/跳号/模式/HOME改变与LAND候选；原启动认证退休后仍保留有界观察，协议排空后原子移交整组local占用，不抢遥测槽。QML只读值及同deployment检查结果，lastObserved不当fresh/健康；不是可信远端实例、最终StartPermit、正式任务完成或飞行停止证明，未编译/运行/测试 |
| 多机发现、能力档案、基础健康摘要 | `FleetCapabilityDiscovery`、`FleetRegistry`、`VehicleCapability`、`FleetHealthCenter`、`GuidedActionsController.qml`、`SwarmController::_refreshFleetRegistry` | 已接入 QGC 车辆发现、连接、固件/UID、机型与电机布局、摄像头/Obstacle Distance/测距能力、传感器位图、有效全球坐标或本地位置 x/y/z 证据、电池平均值、健康检查摘要、Estimator/SYS_STATUS/ESC/FTC 当前摘要和保守 RSSI 估算链路质量（未知/未初始化 RSSI 保持未知，不伪造高分）；能力发现入口与 Registry 均拒绝隐式 ID/布尔/数值转换、冲突身份别名和未来心跳；配置只作为 observed evidence，trusted baseline 仍需本地批准；FactGroup `telemetryAvailable` 不等于带时间戳的新鲜度；能力护照支持版本化 JSON、本地审批元数据和本地原子文件交接（32 MiB 上限），增量导入合并审批并要求 `local_approval` 基线与同指纹审批记录一致；普通注册/更新/心跳入口增加 identity、有限数值、配置 map/指纹和时间戳门禁，历史 `lastSeen` 不得把显式离线快照提升为在线；Registry 写入/批准/清空方法收回 QML `Q_INVOKABLE`，只保留 C++ 发现后端调用，FaultToleranceManager 也只以报告/建议快照对外；指挥中心通过 `FleetHealthCenter` 统一读取 Registry/FaultToleranceManager；签名、权限、原始 EKF/完整链路质量仍需工程师按固件版本校验；本地位置证据不等价于全球 Mission 坐标可用 |
| 批量指令的事务 ID、超时、重试、部分成功 | `CommandTransaction`、`SwarmController` | 起飞、降落、返航、直接指点、标准任务启动、恢复任务和编队阶段已接入 ACK/遥测事务；标准任务启动冻结目标集合并等待 `MAV_CMD_MISSION_START` 接受或 Mission 模式遥测，恢复任务上传完成后也必须进入同一启动事务，临时 Mission 上传完成后先进入 `Starting`，不会把上传 ACK 当作任务启动或完成；普通/临时 Mission 延迟回调发送前会校验事务整体、本车逐机状态和 `_vehicleMissionStartReady()`；同一车辆的普通任务/飞行事务互斥，RTL/Land/formation ABORT 是明确安全例外；新增命令级 health preflight：起飞/Goto/临时 Mission/任务启动对确认离线、严重健康、故障、临界电量或极差链路在创建/延迟发送前阻断，没有 Registry/车辆快照或 `discoveryState=invalid` 时普通命令 fail-closed；批量降落/返航保留安全逃生路径并只把 FleetRegistry advisory block 写入 `healthGate.warningVehicleIds`；未知字段不把缺失数据当健康；`CommandTransaction` 对任一非法/重复/别名目标 ID 整笔 fail-closed，保留原始请求列表并输出 `targetInputValid/requestedTargetCount`；新增 `cancelTransaction()` 让操作员取消活跃事务并触发临时 Mission 清理，但不伪称已发出的机载动作已逆转；真实 ACK/模式时序仍需工程师核实 |
| 任务编排、暂停、恢复、取消、重分配 | `SwarmMissionOrchestrator`、`GuidedActionsController.qml` | 已加入；每次状态、进度或成员变更递增 `revision/taskRevision`，成员 ID 只保存 1--255 的有限整数；创建/重分配中的非法或重复 ID 整体拒绝，不静默缩小集合，带非法/冲突 Leader 元数据的重分配也 fail-closed；连续性建议 ID 还绑定稳定 Fleet 快照指纹；恢复任务确认时冻结 `resumeMissionIndex`，异步上传完成后仍使用该索引并在两阶段做范围校验；终态任务不生成连续性变更建议，显式 Leader 必须属于成员；`replace_leader` 通过版本保护入口原子更新成员与 Leader 字段；原始编排器只在控制器内部使用，普通 QML 只得到任务摘要；掉线/低电量回调只记录建议并刷新只读规划。`applyContinuitySuggestion(suggestionId, true)` 只接受当前规划器中仍存在、需要人工确认且带有效来源版本的替补/重分配建议，并在 `reassignTaskAtRevision()` 前再次核对任务版本；未提供人工确认、建议过期或动作类型不允许时拒绝，不会自动发送飞行命令 |
| 意图式任务预览与人工审批 | `FleetIntentTask`、`FleetCapabilityMatcher`、`FleetMissionReviewCoordinator` | 已加入方案状态机、能力/健康/电量/链路筛选、主组/备用组和角色建议；审阅协调器串联角色、任务、能量、空域、风险和报告快照，缺失或与当前 FleetHealthCenter 不一致的证据保持 `needsReview`；`degraded/warning/limited` 在能力、角色、模板和能量候选阶段统一进入 `health_degraded` 人工复核，不直接放行；稳定健康指纹变化会使旧审批失效，审批仍不释放飞行命令 |
| 任务级风险雷达 | `FleetRiskRadar` | 已加入意图关键词、调用方风险信号、分级解释和人工确认标记；任务参数/风险信号的布尔、Map、有限数值和百分比范围现在严格校验，坏输入返回 `inputValid=false` 并阻断评估；禁飞区/航线/能源模型仍需接入工程数据 |
| 任务编译与审阅 | `FleetMissionDraft`、`FleetMissionPlannerPanel`、`FlyViewMap`、`FleetMissionCompiler`、`FleetAreaCoveragePlanner`、`FleetMissionExecutionProfile`、`FleetMissionReviewCoordinator`、`FleetMissionPlanStore`、`FleetMissionPayloadBuilder`、`FleetMissionDeployment`、`FleetMissionEvidenceRefresh` | 地图提案、精确版本审批及默认/整份成员执行 profile 已接线；v2 上传编码速度/起飞/驻留/HOLD/LAND 请求，完整 INT/float32 项比对、来源映射和驻留最低预算有明确边界。现有面板已有实际账号/独立审批/覆盖确认的地面上传与回读入口，以及撤销旧批准后的受控只读版本/参数刷新，不从规划自动发送。绝对时序、垂直参数/版本认证、真实起降净空、受控启动/完成、孔洞/退让/动力学、草稿重启恢复及长期轮换仍待完成。上传租约和刷新都不授予执行许可，所有 Fleet 部署不可执行，未编译/运行/测试 |
| 事件触发任务规则 | `FleetEventRuleEngine`、`docs/fleet-event-rules.md`、`FleetEventRuleEngine.md` | 已加入严格规则字段、条件 schema（路径/数量/列表/文本边界、真实数值和布尔）、AND 条件、优先级、单规则冷却、前一事件类型门槛和版本化 JSON/`QSaveFile` 原子交接；导入只恢复规则定义并清除运行时状态，结果仍只生成建议，不释放命令；跨站同步、人工审批和工程师验证待完成 |
| 异构无人机能力匹配 | `FleetCapabilityMatcher`、`FleetTaskTemplateRegistry` | 已加入能力标签和行业模板目录；能力发现区分 observed-unverified 与静态/受信 `verifiedCapabilities`，任务可用 `requireVerifiedCapabilities=true` 拒绝低信任标签；尚未接入完整机载能力发现和续航预测 |
| 编队规划、分裂、合并、最小间距检查 | `FormationPlanner` | 已加入，当前输出为本地 ENU 偏移；全球坐标转换和机载跟随协议仍需工程师确认 |
| 掉线、低电量、Leader 候选和降级建议 | `FaultToleranceManager`、`FleetMissionContinuityPlanner` | 已加入；连续性规划器读取任务/机队纯数据快照，生成 Leader 接替、重分配、拆分、恢复和 hold 建议；只对 pending/running/paused 任务生成变更建议，校验 Leader 成员关系，支持任务级中途加入、可信能力和构型 baseline 门禁；故障 freshness 以离散 fresh/stale/unknown 进入建议指纹；故障回调只记录 `continuity.review_required` 并刷新建议，不再直接暂停/重分配活动任务；FleetRegistry/FaultTolerance/连续性层对 ID、有限数值、布尔值、时间戳和重复成员做门禁，输入不一致时仅输出阻断建议；只读且不替代 PX4 安全逻辑 |
| 任务交接和多人权限、中途加入 | `MissionHandoffManager`、`FleetRolePolicy`、`FleetMissionContinuityPlanner`、`SwarmMissionOrchestrator` | 已加入版本化交接快照、本地 `saveToFile()`/`loadFromFile()` 原子文件交接、内存态交接、角色权限、二次审批标志、中途加入候选和任务来源 revision；交接导入严格拒绝未知字段并要求完整全局/记录 revision，避免旧快照降级版本门禁；尚无持久化账号/服务端鉴权、跨站同步或自动执行 |
| 黑匣子事件、时间线回放、任务预演 | `FleetEventBlackBox`、`FleetEventArchiveCodec/Sql/Worker/HistoryReader`、`FleetPersistentEventStore`、`FleetTelemetryArchive`、`FleetTelemetryArchiveCodec/Worker/HistoryReader`、`VehicleTelemetryObserver`、`FleetTimelineReplay`、`FleetMissionSimulator` | 原诊断SQL/分段浏览及27批准入站NDJSON保留。305–309新增共享Header19/Footer31、只读目录/整file前缀page、source/序号/3计数镜像与重开hash，actualcatalog/每file filter+定位+hash cursor及终态交付/actualfinished单槽。public仅固定metadata，native最新原包另确认；invalid候选保留诊断，缺Footer/partialTail不冒充clean/closure；完整坏行/镜像错拒整页。不写档案/建锁/改.tlog/请求流或回灌live状态。CRC/身份/许可/fullTelemetry/closure false，未运行读取/写盘/helper/测试。真实覆盖/error-aware全集索引/归档清理/统计维护/灾后恢复、签名/企业权限及工程师验证继续 |
| 任务证据链 | `FleetMissionEvidenceChain` | 已加入追加式 SHA-256 链、原子 JSON 导入，并支持 `saveToFile()`/`loadFromFile()` 本地原子交接；文件大小和坏链拒绝，数字签名、权限、长期留存和工程师验证仍待接入 |
| 自动事故与任务报告 | `FleetMissionReportBuilder` | 已加入四类受众视图、事件输入完整性标记、JSON 与 `saveToFile()`/`loadFromFile()` 本地原子交接；权限、报告模板、长期归档和工程师验证仍待接入 |
| 任务数字孪生、故障注入与确定性预演 | `FleetMissionDigitalTwin`、`FleetDigitalTwinPanel.qml` | 已加入 fleet/mission/map/link/energy 版本化快照、稳定哈希、确定性时间线、`saveToFile()`/`loadFromFile()` 本地原子交接和失联/低电量/GNSS/Leader/链路故障建议；只读且不发命令，真实飞控/碰撞/风场模型和 Mock/SITL 验证待完成 |
| 集群记忆地图 | `FleetOperationalMemory`、`FleetOperationalMemoryPanel` | 已加入只读观测历史、链路/GNSS/风场/高耗电/异常位置格网和估算标记，并支持 `saveToFile()`/`loadFromFile()` 本地原子快照；真实长期采集、数据库、地理校准和工程师验证仍待接入 |
| 运营指标与 ROI | `FleetOperationalMemory::metrics()`、`exportJson()`、`saveToFile()`/`loadFromFile()` | 已加入任务完成率、覆盖率、响应时间、能耗、人工干预、利用率、维护成本和 JSON/本地文件交接；成本口径、样本完整性和工程师验证仍待完成 |
| 维护护照与预测性维护 | `FleetMaintenancePassport` | 已加入维护记录标准化、时间排序、证据缺口、阈值原因和估算风险，并支持 `saveToFile()`/`loadFromFile()` 本地原子交接；阈值校准、长期数据库、真实维护数据和工程师验证仍待完成 |
| 行业任务模板、扩展目录 | `FleetTaskTemplateRegistry`、`FleetExtensionRegistry`、`FleetCatalogPanel.qml` | 模板目录已支持三段式版本、能力/任务约束校验、JSON 与 `saveToFile()`/`loadFromFile()` 本地原子交接；默认模板/扩展由 C++ 后端注册，注册、启用、删除和导入/加载不再暴露给 QML；扩展目录对 metadata 增加递归深度、节点数、键长度和累计大小边界，并固定 `flightCommandRelease=false`，默认不加载任意二进制；指挥中心以值快照只读展示模板、扩展和签名/沙箱待验证状态；签名服务、审批、长期存储和结果执行仍待接入 |
| 分级自主与跨飞控/插件能力声明 | `FleetAutonomyPolicy`、`FleetRolePolicy`、`FleetExtensionRegistry`、`AiCommandPolicy` | 已加入人工/辅助/监督/自动化阶段、PX4/ArduPilot/伴随插件能力、角色与审批范围、签名/沙箱占位；只读、无命令释放，真实适配器签名、沙箱、版本兼容和执行闭环仍待接入 |
| 编队 PREPARE → COMMIT → RELEASE → ABORT | `SwarmController`、`GuidedActionsController.qml` | 已加入现有入口，成员失败/超时会整组回滚；操作员停止与故障 ABORT 分开记录，停止启动时不立即冒充 `formationFault`，只有回滚拒绝/超时才报告具体故障；ACK 回调按 phase generation 隔离旧会话，`MAV_RESULT_IN_PROGRESS` 保留 pending；当前协议仍固定 UAV-1 为 Leader、最大 6 机，机载协议/真实 ACK 语义仍需工程师核实 |
| 地图、指挥中心共享状态 | `FlyViewMap.qml`、`CommandCenterOverlay.qml`、`GuidedActionsController.qml` | 已改为共享同一 `SwarmController`；指挥中心显示选中目标健康门禁、容错建议和活动事务取消入口；地图左键菜单和普通右键 Goto 都冻结单机或框选目标 ID 与坐标，确认/执行两侧复核最新 FleetHealthCenter，并统一调用 `SwarmController::executeGoto()` 事务路径；标准启动/继续任务也冻结目标集合、任务项快照并调用 `executeStartMission()`，不再直接调用 `Vehicle.startMission()`；指挥中心面板的高度/速度/起飞速度卡由 `GuidedActionsController.requestControlCommand()` 统一进入冻结目标、二次健康/状态预检和人工确认，并记录 `command.requested/confirmed/sent/rejected`；对 `MPC_TKO_SPEED` 成功提交后会在 5 秒内轮询参数回读，记录 `command.acknowledged` 或 `command.ack_timeout`；当前范围是保守的 UI/机型通用上限，不代替机体能力、地理围栏、任务空域或 C++ 权限边界；发送后仍需遥测核实；起飞/编队/任务启动在确认与执行边界复核门禁，临时 Mission 在地图适配器真正调用 `executeQueuedGoto()` 前再次复核；所有 Guided/批量动作执行入口另有一次性 confirmation token 与冻结 `actionData` 双重核验，缺失/重复/复制回调不会触达 Vehicle/SwarmController；多机列表的启动任务/单机暂停/单机 RTL 现已转入统一确认入口，直接解锁与接管模式按钮暂禁以避免绕过门禁。RTL/降落/急停保留安全逃生路径，不被健康门禁阻断，但仍需人工确认 |
| AI/自动化建议 | 既有 `Ai*` 模块 | 保持建议与人工执行隔离；本批次没有扩大 AI 的 MAVLink 执行权限 |
| 日志、回放、SITL/故障注入 | `docs/development/SITL_MULTI_VEHICLE_MISSION_TEST.md` 等 | 作为工程师接手验收项；当前环境没有 Qt 工具链，未执行构建或仿真 |

## 工程师接手顺序

2026-10-13 续作：本地任务模型新增活动成员排他占用（running/paused 保留、Draft 不占用、终态释放），`assignmentAvailability()` 提供冲突机/任务/phase，`fleetSummary.busyVehicleIds` 提供忙机快照；启动/恢复/活动重分配在后端拒绝冲突，编队先预留再发送。连续性应用前重审全部任务，不以单任务输入抹掉其它任务占用。它不是跨站锁或机载执行授权，完整产品目标仍未验收。

资源边界也补齐实际控制流：发现超限直接返回 invalid，不进入递归 hash；Registry 对累积 metadata 暂存验证后提交；无效 Matcher 任务不进入候选循环；任务记录类型/原因/数量/metadata 有界，拒绝请求不局部修改既有任务。

### 本轮输入边界补强（2026-10-07 续作）

- `FleetHealthCenter` 的健康聚合/门禁现在严格读取布尔和有限数值，电量/链路越界、混合目标中的非法 ID、无安全 Leader 和来源 `healthState=healthy` 覆盖风险都会阻断；结果保留 `invalidSelection`、`globalBlocking` 和 `reportedHealthState` 诊断字段。
- `FleetMissionCompiler` 的任务身份、assignment role、reserve 列表、闭合 fleet membership 和 `geofenceRequired` 几何证据已在编译入口校验；非法输入形成 blocking unmet/risk，只有明确不提供 fleet 时才是 planning-only 预览。
- `FleetCapabilityDiscovery` 的规范化输出与 QGC live descriptor 均经过严格 `validateDescriptor()` 自审计；发现错误会保留 `discoveryErrors`/`discoveryState=invalid`，不能把未经验证的能力/构型数据静默当作可信护照。最终是否写入仍由 FleetRegistry 原子 ingress 决定。
- 能力/构型 descriptor 现在有共同资源上限：能力证据最多 128 项且单项最多 128 字符；递归 Map/List/Hash/StringList 最多 8 层、2048 节点、累计字符串/字节最多 128 KiB，非 JSON-safe 自定义 QVariant 直接阻断；发现构建、Registry 注册/更新/心跳均在 fingerprint/metadata 合并前执行该边界。
- Guided 单机动作和事务证据继续收口：Takeoff/Land/RTL 的确认目标不可被 active vehicle 切换改写，确认数组带指纹；Formation timeout 有 phase/deadline 门禁；Mission/Land/RTL 目标模式需晚于 dispatch 的证据；queued goto 不接受混合非法航点。临时 Mission 共享回调仍需要工程师验证真实归属。
- 能力发现首次注册路径也已对齐：`VehicleCapability` 保留并重新输出顶层 verified/证据状态、discovery 状态/错误、发现时间和位置证据源，Registry 在 ingress 前校验列表元素及时间戳；工程师需用首次接入、往返序列化和 invalid discovery 门禁回归确认无数据丢失。
- 能力匹配可信度也已对齐：`requireVerifiedCapabilities=true` 只消费显式 verified 标签，不能由 `canUse*` 运行时观测绕过；工程师需回归 verified-only 与普通规划两条策略路径。
- 连续性替补也沿用 verified-only 规则，且 fleet fingerprint 追踪 verified/discovery 证据变化；工程师需验证旧建议在能力信任变化后失效。
- 能力标签输入对 `capabilities/verifiedCapabilities` 执行严格列表类型校验，损坏条目进入拒绝原因；工程师需验证 JSON/QML 数组映射后的类型兼容性。
- 能力 Matcher 与发现/Registry 使用同源资源边界：任务/车辆能力最多 128 项且单项最多 128 字符，Fleet 候选最多 2048 架；超限输入保持 `parametersValid=false`，不生成可行候选。
- 连续性候选入口使用同一能力列表类型门禁，坏快照标记 invalid 且不进入替补候选；工程师需验证输入错误不会被建议层吞掉。
- Matcher 的任务能力要求同样拒绝非字符串元素，工程师需覆盖模板/JSON/QML 的数组类型映射。
- RoleAllocator 现在支持 `requireVerifiedCapabilities` 并沿用同一信任边界；工程师需验证角色任务模板的策略字段传递。
- TaskTemplateRegistry 的模板/任务验证也支持 `requireVerifiedCapabilities`，能力列表错误进入阻断；工程师需验证模板约束和任务覆盖的实际类型。
- Matcher 参数不合法时选择/分配结果为空，工程师需覆盖 UI 对 `parametersValid=false` 的展示和禁止执行逻辑。
- `CommandTransaction` 目标身份键现在与 Registry/Health 契约一致，只接受真实有限整数 1–255；工程师需回归 QML double、字符串/布尔别名和 ACK 回填。
- 普通多机 Takeoff、Goto/临时 Mission 和 Mission Start 现在要求冻结目标集合全量通过预检，任一机体失败即整批 fail-closed；Land/RTL 继续作为明确安全逃生部分执行路径。
- PlanView 顶部 Upload 入口增加 controller 存在性和 `upload()` 方法检查，面板/工具栏均进入同一任务写入预检；工程师需验证 PlanView 生命周期和 active vehicle 切换。
- MERIVUS Guided legacy 单目标动作在执行边界重查连接/链路；PID 调参自动模式与 FlightModeDropdown 通过 `requestFlightModeChange()` 进入统一模式入口。上述均未执行 QML/Mock 回归。
- RCToParamDialog 的维护参数写入现在执行连接、链路、armed/flying、有限数值和 MERIVUS FleetHealth 门禁；工程师需验证参数回读与 ACK。
- Guided/CommandCenter/PlanView 对空 `vehicleLinkManager` 统一显示链路未知并禁止发送；工程师需验证缺失链路管理器、恢复链路和 UI 可用性状态转换。
- PX4 CameraComponent 重启按钮现在执行连接/链路/armed/flying 门禁；工程师需验证参数回读、重启 ACK 和页面恢复。
- 兼容版 Guided fallback 的 `executeAction()` 现在也在实际发送前检查初始连接、`vehicleLinkManager` 和通信状态；多机 Start Mission/Pause 先预检全部目标，工程师需验证 QML vehicle list 的 `count/get` 映射、部分掉线不产生部分执行以及 ACK/超时语义。
- 兼容版 Guided 的 Mission resume 现在通过正常确认框冻结目标，并在执行边界复核活动机/链路；工程师需验证 MissionManager 恢复上传、operation identity、ACK/超时和旧回调不会误触发下一次恢复。
- PX4 `SensorsSetup` 校准后重启和 Mission complete 任务清理入口增加连接/链路/飞行态二次门禁；工程师需验证重启、清理回执和 QML 生命周期，不把按钮可用当作飞控接受证明。
- Mission resume 生成入口增加空/短任务防越界，Formation ABORT 对 skipped 成员 fail-closed 且重复停止不覆盖进行中的回滚事务；工程师需验证异常任务、ABORT ACK/拒绝/超时和 operation identity。APM 三个维护重启组件同样增加连接/链路/飞行态门禁，MotorTest/Fact 写入仍待后续统一。
- Formation 通用取消入口现在拒绝 `formation.*` 地面-only cancel，统一要求显式 ABORT 回滚；工程师需验证 UI 取消、重复停止和 ABORT ACK/超时路径。
- 意图式任务审批已绑定 FleetRegistry 稳定快照指纹，改装/替换/能力证据变化会使旧审批失效；工程师需验证 stale fleet、registryChanged 和构型证据更新路径。
- Mission resume 地面站上下文增加 request identity、任务快照和 120 秒过期门禁；工程师仍需补 MissionManager operation token 与 ACK/超时归属验证。
- 通用/APM MotorTest 入口增加连接、链路、armed/flying 双重门禁；工程师需补 C++ 权限、ACK/超时和断链回归。
- 底层 ACK 归属专项审计确认：`Vehicle::mavCommandResult` 与 Mission Protocol 消息没有协议级 operation/session generation；`PlanManager` 已补地面站本地 identity 及带 identity 的回调，但 Swarm 本地 epoch/identity 仍只能缓解，不能证明迟到旧 ACK 属于当前发送。工程师必须继续补协议侧 token 或 quarantine，再把事务终态用于放行判断。
- `PlanManager` 现在为 read/write/remove-all、guided 和 resume upload 生成本地单调 `operationId`，保留旧完成信号并提供带 identity 的新信号；Swarm 临时 Mission 上传/清理已按保存的 identity 归属回调，并继续做机载任务列表后置校验。该 ID 不在 MAVLink 线上，工程师仍需验证并发 MissionController、迟到 ACK 和 read/write/removeAll 交叉时序。

- `FleetRegistry`/`FaultToleranceManager` 现在对成对 fault evidence 保存地面站观测锚点并随刷新老化；只发心跳不能让 `age=0` 永久保持新鲜。FTC 删除成员后重建建议、拒绝未注册 Leader，Registry 配置 clear 与新 evidence 的矛盾组合会被原子拒绝。

1. 在受支持的 Qt 5.15/MSVC 环境编译 `custom/custom.pri`，先修复 moc、QML 和链接错误。
2. 补齐 `Vehicle` 的电池、链路、估计器、任务进度信号，并把未知值保持为未知，不能当作健康。
3. 完成普通批量指令与 `mavCommandResult` 的逐机 ACK 关联；没有 ACK 时只显示“已发送/待核实”，不显示“已完成”。
4. 使用 Mock、PX4 SITL、日志回放和故障注入验证超时、拒绝、部分成功、重连、Leader 替换和编队回滚。
5. 通过拆桨/系留/限速等现场安全流程后，才允许进入真机飞行验收；本代码包本身不构成飞行安全证明。

## 明确未承诺的能力

- 本地地面站不能保证断链后继续控制飞机，最终故障保护必须由 PX4 机载逻辑承担。
- “悬停不漂、断桨悬停、碰撞后自稳定、复杂避障”等飞行性能不能仅由地面站代码证明。
- 当前集群控制协议不是任意规模通用协议；要支持超过 6 机或动态网络，需要单独完成协议和带宽设计。

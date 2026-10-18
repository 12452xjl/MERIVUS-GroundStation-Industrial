# MERIVUS GroundStation 本批次变更入口

## 546：五条本地操作链与最终交接范围

本轮26条源码/工程登记＋26条文档，共52条去重路径；其中公共交接文档12条，另新增25目标范围核对。此数字只描述本轮，不是全项目完成率，也不表示仅需应用这些文件即可还原全部历史开发。

- 规则建议：实际黑匣子接收、独立实时冷却、有界人工待审和处理审计。
- Run检查点：明确启用后的合并后台保存、独立历史重开和完整保护报告查看。
- 用户模板：完整草案捕获、不可变版本、独立库文件、精确版本复制新草案并重新审批。
- 日志留存：真实清单预览、原维护槽逐项退休/验证备份，保留部分结果，不自动永久删除。
- 审阅报告：修复原摘要域，实际审阅冻结、四受众详情和完整文件保存/独立重开。

逐路径见[本轮清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。用户已确认[本地源码最终范围](docs/handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md)，外部未完成项不假称完成。未编译、运行或测试，不含新的已编译安装程序。下方为历史。


## 545：完整证据链文件交付链路

7源码/资源登记＋14文档，共21路径，新增面板qml与说明md两文件；[逐文件清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。复用原证据链和文件层，新增后台冻结保存及独立历史浏览，不新造任务/控制权系统。未编译、测试、打包、提交或更新PR。

## 544：公共航线逐机展开

本次2源码＋14文档，共16路径，无新文件；明细见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。实际修改集中于[Compiler](custom/src/Swarm/FleetMissionCompiler.cc)，统一实际成员、预算、预演和下发的路线取源，保留时间、返程及风险约束。不制作阶段源码包，未编译或测试。

## 543 当前：配置速度明确采纳与逐机路线预算

当前参数观察可提供四项配置速度建议，操作员查看并明确采纳到未保存输入；采用前与保存时都重查原来源。返程 NAV 建议显式沿用巡航值，不是独立RTL参数，不代表真实性能认证。普通路线现在由最终本机路线/profile生成出返航行估值，与驻留分开；只提高既有预算，不能把未知总任务时长补为可执行。

兼容原备用配置读取、载荷字段核对及任务重规划；不增加飞行指令、放宽启动权限或恢复旧批准。10源码＋21文档，共31路径，无新文件。原25目标尚未全部完成；不制作阶段包，继续面向统一源码交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR；不以文件数估算进度与交期。

见[配置操作](custom/src/Swarm/FleetExecutionProfileEditor.md)、[路线模型](custom/src/Swarm/FleetMissionPreviewTimeline.md)、[实际预算消费](custom/src/Swarm/FleetMissionReviewCoordinator.md)。下方542及更早为历史。


## 542 当前：编队等待与重新确认（源码接线，工程验证未执行）

新增本次会话的编队待办：明确保存固定成员 → FIFO 等待实际占用释放 → 操作员重新审阅 → 原确认框与既有派发入口。资源就绪不自动派发；取消待办不停止飞机，已提交请求不等于飞控接受或任务完成。最多32条（含终态），待办30分钟、单次审阅60秒，以原生单调时钟判断；重启不恢复队列或批准。

实际 Swarm/Orchestrator 原对象提供占用信息，原生占用未知或未解决时不能进入复核；确认消费前重查同一来源、版本、FIFO 与占用。新增有界任务占用投影，避免定时复制完整 Run 观察；原协议仍仅接受 UAV-1～UAV-6、必须包含 UAV-1，不扩大飞机容量或最终启动许可。

本次12源码/工程登记＋16文档，共28路径，推进目标6/8/13的本地调度操作链；25项目标尚未全量完成。按最新要求不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR；不根据路径数推算完成率或交期。

见[队列原生合同](custom/src/Swarm/FleetFormationQueue.md)、[实际确认与派发接线](custom/src/Swarm/FleetFormationQueueWorkflow.md)、[操作界面](custom/src/Swarm/FleetFormationQueuePanel.md)。下方541及更早为历史。


## 541 当前：任务模式退出证据贯通（源码实现，工程验证未执行）

原生提交后的任务模式退出现在保留计数、首/末 HEARTBEAT 原模式与状态、前后接收引用；已置异常位不会吞掉后续已提交快照的变化。完整/保护报告、交接简报和归档历史同步版本化，旧资料显示“未采集”，不补成零次。检查点之间多个退出可合并，不宣称完整逐边沿日志。

18源码/工程登记＋23文档，共41路径，新增共享校验器 h/cc/md。只推进运行结果、交接和审计链，不改变终点候选、不发飞行命令、不恢复控制权。原25目标与实质剩余保持；不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR，不据路径数推算完成百分比或交期。

见[退出证据合同](custom/src/Swarm/FleetMissionModeExitObservation.md)、[报告合同](custom/src/Swarm/FleetMissionExecutionReport.md)、[审计版本](custom/src/Swarm/FleetRunObservationEvent.md)。下方540及更早为历史。

## 540 当前：完整计划驱动的逐机时间预演（源码实现，工程验证未执行）

预演不再仅接任务摘要：操作员明确捕获当前审阅记录，同一记录的完整计划与能量预算一起进入逐机路线计算。出程、独立返程及显式驻留分别计算；缺速度、垂直率或坐标基准时保留未知，不均摊全队航线、不借新电量补旧计划。原生结果保有界事件集合，现有界面四条分页；自动概览刷新不覆盖正在浏览的冻结计划。

相对时间以各机首规划点为零，不代表跨机同步；起降、转弯、风及实际飞控动态未由此认证。此项推进目标16/17，但不替代最终StartPermit、正式机载结果或完整数字孪生。原25目标不缩减；不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR。

见[计算合同](custom/src/Swarm/FleetMissionPreviewTimeline.md)、[实际界面与来源](custom/src/Swarm/FleetDigitalTwinPanel.md)。下方539及更早为历史；不按批次或路径数量推算完成百分比和交期。

## 539 当前：交接历史保存与恢复（源码实现，工程验证未执行）

原运行简报交接增加后台文件保存、独立加载预览、四条分页与详情，以及明确确认后的仅追加历史合并；不开放旧整库替换入口。同ID内容冲突整笔拒绝，已保存事实与当前工作资格分开。详细流程见[交接工作流](custom/src/Swarm/FleetMissionHandoffWorkflow.md)、[原生合同](custom/src/Swarm/MissionHandoffManager.md)和[界面合同](custom/src/Swarm/FleetHandoffPanel.md)。

原25目标保持，本次推进目标15的本地资料交接，不代表跨站控制权交接或全部目标完成。无阶段源码包；完成约定源码后统一交付。未编译、运行、测试、仿真、操作硬件、刷机、修改PX4、提交或更新PR；不以文件数推算完成百分比或交期。下方538及更早为历史。

## 538 当前：持久化任务索引（源码接线，工程验证未执行）

建立固定派生索引，保存分窗扫描进度；重开核原来源后继续，任务分页与原记录下钻保持有界。不是提高旧目录上限或只新增建议界面。详细实现与范围集中见[索引合同](custom/src/Swarm/FleetArchiveTaskIndex.md)、[界面合同](custom/src/Swarm/FleetArchiveTaskIndexPanel.md)及[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

原25目标与六类剩余保持。先前“约40%／60%”没有逐项验收权重，仅属历史粗估，不再作为当前完成度或交期依据。按用户最新要求，不制作阶段源码包；完成全部约定源码后统一交付。当前仍未全量完成，未编译、测试、仿真、运行项目、刷机、修改PX4、提交或更新PR；实际工程验证仍由工程师承担。下方537及更早为历史。


## 537 当前：逐 NAV 航点相对驻留（源码实现，工程验证未执行）

沿真实辅助路线点编辑器增加可选holdSeconds：独立出返程点可新增、编辑、取消或清除驻留；缺失与明确0分别保留，原秒数贯穿草稿、计划和来源身份。复用原点编辑的版本／来源保护，不把驻留解释为绝对到达时刻，也不扩成自动覆盖、起降或HOLD动作设置。

新增共享DwellEncoding，统一拒绝正请求在Float32或所述PX4微秒转换中变成零等待。上传与规范回读仍沿原param1规则，原请求不被量化回写。能量／时间审阅改用原请求、上传值及模型等待量的保守预算；仅确有下舍才上取，精确1秒和1+1不无故增加ULP。三个严格几何消费者与Joint使用相同接受域，明确驻留即使没有executionProfile也进入原实时版本／当前工程声明门。

模型仅对应已读本地旋翼NAV分支，计时从首次位置接受开始，yaw等待可能消耗它；不是精准悬停、持续留在半径内、真实调度上界或实际固件认证。没有新增payload/schema/权限字段，最终StartPermit、actualPath和正式结果仍未完成。

本批12源码／工程登记＋21文档=33路径，新增DwellEncoding.h／cc／md三条；40项工程场景全部未执行。限定直接静读修正了新点错误消息的局部变量重名问题，并交叉核对实际消费者与数值模型。UI/UX仅指导原Qt表单的独立勾选、标签、错误及取消恢复，没有运行技能脚本、界面预览或任何项目逻辑。

原25目标与六类实质剩余不缩减，约40%源码／60%剩余仅粗估。未编译、运行helper／算法／parser／QML／hash、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[驻留数值合同](custom/src/Swarm/FleetMissionDwellEncoding.md)、[实际点编辑](custom/src/Swarm/FleetMissionDraft.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方536及更早为历史。


## 536 当前：逐 NAV 航点机头方向（源码实现，工程验证未执行）

真实辅助路线编辑入口、Draft、Compiler、载荷及上传／回读消费者接入可选逐点 heading。规划界面支持 relay／responder 独立出返程点的新增、整点编辑、取消和删除；区域自动覆盖点仍沿飞控策略，不把扫描线方向、起降或 HOLD 请求当逐点机头朝向。缺省与显式0°区分，原连续请求保留在草稿／计划身份中。

上传采用声明的最近1/256°网格，最大圆周量化差1/512°；原请求、上传tick、实际回读原值及比较用规范值分别保留。实际有限Float32回读只有在明确推导界内才归同一tick；非法大有限角度也保原数诊断，不补0。编码步长及语义匹配不是实飞精度、逐位相同、真实固件认证或达向／转向动力学证明。

无heading单机保持原v2；明确heading使用独立v3语义域，混合机组逐成员区分。原严格几何／审阅消费者使用同一编码规则；实际目录声明在审阅与启动前置首尾核对应域，旧声明不自动升级。回读报告新schema2在匹配时也显示原值，最多64条并明确截断。最终StartPermit、actualPath及正式执行结果仍未完成。

本批28源码／工程登记＋32文档=60路径，新增HeadingEncoding.h／cc／md三条；48项工程场景全部未执行。限定源码交叉静读与普通文本／路径／Markdown／空白核对不等于构建或运行验证。UI/UX仅静态指导原Qt表单的独立勾选、字段错误、键盘／换源恢复与原值显示；未运行技能脚本或界面预览。

原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅粗估，不按文件／批次上调。未编译、执行项目／parser／helper／QML／算法／hash、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[朝向编码合同](custom/src/Swarm/FleetMissionHeadingEncoding.md)、[真实编辑入口](custom/src/Swarm/FleetMissionDraft.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方535及更早为历史。


## 535 当前：异机完整包络冲突检查（源码静态收口）

沿任务执行与安全主线新增异机声明包络比较：同次启动整域和 NAV／起降参考体的完整原生组件进入全异机组合，保机体、支撑及误差余量；不从报告重建名义线段或只取前缀。闭边界接触也属于交叠候选，地形接触 pad 不豁免另一架飞机，无可靠同步时序按全时段空间保守比较。

完整双方平面进入整数几何求解；新比较共用有界预算与原取消信号。缺失完整几何、求解未知或超预算不算通过。Worker 结果升级 exact8，普通／地面审阅、Witness 和两 Authority 接入同份报告及共同结论，不是只新增显示字段。

旧 minimumSeparation 检查不被替换；外包交叠不是实测碰撞，无交叠也不是实际路径认证。原 `actualPathCertified` 与最终 StartPermit 缺口保留，未新增飞行命令。原25目标／六类实质剩余继续，约40%源码／60%剩余仍仅粗估，不按本批路径数上调。

本批15源码／工程登记＋21文档=36路径，新增纯值模块h／cc／md三条；源码实现及限定静态交叉审阅已收口，48项工程场景全部未执行。仅源码、直接静读及普通文本／路径／Markdown／空白核对；未编译、运行项目／算法／parser／helper、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[异机包络合同](custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.md)、[后台与准入](custom/src/Swarm/FleetMissionStartupClearanceWorker.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方534及更早为历史。

## 534 当前：参数导入原生发送与固定重试（源码静态收口）

继续533实际导入入口，将私有typed选中值交给PM独立原生事务；接纳后不再经Fact通用setter乐观改当前值。事务固定原飞机／PRIMARY／参数值，独立有界调度与重试；不借旧同名等待队列或当前Fact值重发。正常关闭预览不取消已接纳事务，参数页增加原事务状态与准确ID停止剩余发送入口。

新Vehicle私有桥在插件调整后及writer入口前核原来源与固定payload，先记实际writer入口事实再调用；APM按真实组件保numeric-float转换，不可精确表示的整数整批拒绝。最早RAW接收冻结当时已有事务的票据，后来的新事务不能领取旧包；同值报告仅称匹配，不是因果ACK、配置认证或飞行许可。CMake同步实际编入APM与共享特性开关。

100ms独立调度，每轮最多扫描256行／尝试64行，每行至少3秒、首次后最多5次重试，整次120秒在发送与RAW取票／消费两端复核。UDP／Mock原Link可在工作线程；取消期间的旧终态不被尾部逻辑覆盖，页面销毁后不再续读旧对象。

本批11源码／工程登记＋16文档=27路径，新合同2份；源码实现及限定静态交叉审阅已完成，64项工程场景全部未执行。UI/UX仅静态指导原Qt控件的状态、准确ID取消、键盘焦点及窄窗换行，没有执行技能脚本或预览。未编译、运行项目／parser／QML／业务／定时器、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%源码／60%剩余仍仅粗估。

[原生事务](src/FactSystem/ParameterImportTransaction.md)、[发送桥](src/Vehicle/ParameterImportTransport.md)、[界面与接纳](src/QmlControls/ParameterEditorController.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方533及更早为历史。


## 533 当前：参数文件完整预览与精确提交关联（源码静态收口）

真实产品入口是ParameterEditor载入文件和ParameterDiffDialog人工确认。旧预览会跳过坏行、未核完整字段和类型，并借用live metadata生成临时Fact；未知枚举及重启标志可能影响当前参数定义。本批复用现完整typed文件校验，建立独立预览及私有候选，并用原预览token关联提交、选择和关闭。旧页面不显示或改动后来模型，载入失败显示明确原因。

文件可表达十种数值类型，但现PARAM_SET发送器只正确支持七种；选中的64位整数／Double必须在第一项提交前明确拒绝。逐项写入前后及最终通知尾复核原对象、值和当前写入条件；中断保留已尝试项可能提交的事实，不宣称批量回滚或机载ACK。同一行旧sender内部的最终来源连续性仍需专用提交桥，不能用caller前后检查代替。

本批4源码／QML＋13文档=17路径，新增1份合同，48项工程场景全部未执行。源码实现及限定静态交叉审阅已完成；未编译、运行QML／parser／参数／业务IO、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。UI/UX技能仅静态指导原Qt控件的错误反馈、状态与恢复路径，没有执行技能脚本或预览。原25目标与六类实质剩余保持，约40%源码／60%剩余仅为粗估。

[实际导入合同](src/QmlControls/ParameterEditorController.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方532及更早为历史。


## 532 当前：多机元数据后台调度（源码静态收口）

继续实际多机连接链：Artifact校验、Summary解析与Translate翻译接入同一进程FIFO预算，最多2项运行／32项排队，排队120秒并在获槽前复核期限。队列满、未启动取消／超期与运行失败分别处理；原取消后仍start的两个清理旁路移除，避免突破运行上限。

运行项保留原输入所有权及result／actual finished双尾，取消不提前归还运行槽。未启动失败不作为缓存损坏、CRC失败或重下载依据。这个预算只涵盖上述两类worker，不涵盖HTTP／FTP、所有Qt线程、旧显式同步翻译或整进程内存／CPU。

本批10源码／工程登记＋17文档=27路径，新增3条；40项工程场景全部未执行。源码实现及限定静态交叉审阅已完成。未编译、运行项目／线程／parser／helper／下载／业务IO、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍是粗估。

[调度合同](src/Vehicle/ComponentMetadataWorkScheduler.md)、[实际元数据消费](src/Vehicle/ComponentInformationManager.md)、[实际翻译消费](src/Vehicle/ComponentInformationTranslation.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方531及更早为历史。


## 531 当前：参数缓存 v3 与真实 HASH 接线（源码静态收口）

替换实际参数缓存的 generic QVariant／QMap反序列化及按声明宽度盲读：新增4MiB／16384行的独立v3纯值格式，明确头、身份声明、十型primitive LE、排序唯一名称和完整EOF；坏文件整份miss。旧.v2保留，不解析、不删除或降级读取。Float／Double保实际IEEE原表示，Fact容器通知尾同步按真实位模式核NaN／Inf／正负零，不把非有限值当健康证据。

实际PX4 hash按本地6C源码核对，只覆盖已使用非volatile参数的名字与Int32／Float原4LE字节，初值0／无末xor；十型文件格式不等于十型PX4 hash。RAW HASH在PM最早可见入口绑定原PRIMARY及UInt32／原count，读取、metadata和发送尾持续核来源与真实参数对象，不借后来链路处理旧包。

CRC与总数都不能证明volatile名称／used-index未变化。零volatile才可按完整候选快载，全部actual Fact／metadata／目录／原来源及值通过后才无外调批量信用索引，再在无pending write及索引状态复核后，走受控完成与原固定HASH ACK桥；含volatile只预填非volatile模型，不清任何真实接收等待、不发HASH ACK，正常机载流／原重试读齐后才完成。自身回放不递归写缓存或逐行提前ready，真实接收／刷新／写入永久退休旧attempt，其独立普通完成不被旧cache卡住。写入改QSaveFile无direct fallback及完整块／commit检查，取消假进度动画；已发生的发布／writer事实不伪造回滚。

本批7源码／工程登记＋15文档=22路径，新增3条；64项工程场景全部未执行。仅源码修改、直接静读与普通文本／路径／Markdown／空白核对；未编译、执行工程／codec／parser／CRC／helper／参数／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标与六类实质开发保持，约40%源码／60%剩余仍仅粗估；后台全机队调度、其它factory内部寿命及完整执行／运营仍有源码工作。

[缓存格式](src/FactSystem/ParameterCacheFile.md)、[实际消费](src/FactSystem/ParameterManager.md)、[IEEE容器值](src/FactSystem/Fact.md)、[22路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方530及更早为历史。


## 530 当前：离线参数真实值与整份发布（源码静态收口）

修复真实离线编辑入口：旧代码把文件数值解析到局部变量却从未赋给 Fact，非零参数仍为初始 0；负 Int8 又使用无符号转换。本批新增纯值 codec，完整接纳五列文件和十种数值类型，再建立实际 Fact-owned 元数据与参数值候选，全部通过才发布映射、组件计数、总数、默认组件和 ready 状态，不保坏尾的成功前缀。

输入最多4MiB／65536行／16384参数，行内容4096字节、数字字段128字节；整数先范围后转换，实数有限且拒非零下溢，保浮点负零。实际 Plane 注释含 NUL，先跳过有界原始注释而不误拒内置模板。文件 system ID 仅历史声明，不改离线 Vehicle ID；构造期间不要求尚未安装的 Vehicle.parameterManager 或在线握手。

候选保持原 owner／offline／插件／模型及实际版本／来源，通知和工厂回调后重核；两版本监听就位后也拒改回原值的旧候选，失败只清本批未发布对象。Fact 容器输入另修数值相等却类型不同或零符号不同的跳赋值，仍不发参数写回信号。离线值不成为实机观测、构型认证、性能证据或 StartPermit。

本批7源码／工程登记＋15文档=22路径，新增5条；源码实现及限定静态交叉审阅已收口，48项工程场景全部未执行。仅源码修改、直接静读与普通文本／路径／Markdown／空白核对；未编译、执行项目／parser／helper／参数或文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。二进制参数缓存校验、全机队后台调度仍有源码工作；原25目标及六类实质剩余保持，约40%源码／60%剩余仅粗估，不按批次数上调。

[纯值格式](src/FactSystem/OfflineParameterFile.md)、[实际加载](src/FactSystem/ParameterManager.md)、[类型／零值](src/FactSystem/Fact.md)、[22路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方529及更早为历史。


## 529 当前：后台有界翻译与合法递归（源码静态收口）

沿真实元数据翻译链继续，不关闭本地化功能。新增纯值有界核心及两段后台作业：摘要读取／locale URL选择 → 主线程有界下载 → TS解压／XML／JSON／规则执行／唯一输出。生产入口绑定原生summary／metadata artifacts及独立请求ID，实际worker自己持输入；不是仅把QString传入线程。

原实际fixture包含随list／items下降数据的合法recursive-def，不能按def全局visited拒绝。新核心区分真实物理数据位置和TS显示context，仅拒无进展的活动规则／引用循环，另对JSON全树、XML文本及翻译工作分别计预算。数字list key对应实际PX4枚举／bitmask，修复旧toString变空context；字符串、数组、wildcard、globals及合法DOCTYPE TS保兼容。

协调器以actual worker／阶段／代次及result+actual finished两尾才采用结果；source退休／析构按原ID及两artifact准确取消，后台未结束不销毁线程或提前释放输入。新native完成信号与legacy路径调用者分离，不接管外国输出。同步translateJsonUsingTS仍保借用／qrc输入及原生唯一输出，复用同有界核心。

本批9源码／工程登记＋16文档=25路径，新增6条；源码实现与限定静态交叉审阅已收口，48项工程场景全部未执行，不代表运行或可靠性验收。未编译、执行项目／parser／helper／规则／线程／XML／JSON／下载／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍为粗估，不以路径或批次数上调。

[有界核心](src/Vehicle/ComponentTranslationEngine.md)、[后台作业](src/Vehicle/ComponentMetadataTranslationWorker.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。


## 528 当前：下载原件唯一所有权（源码静态收口）

继续真实组件元数据入口，处理同名远端文件共用临时路径的问题。新增原生 fresh 临时文件 owner，HTTP／MFTP 和 TS 下载使用独立 request ID／完成信号；不是下载后再从共享 basename 复制。原 legacy 固件、相机、参数包下载接口及其大文件／本地路径接受域保留。

新传输持 owner 到写句柄关闭，Manager 核实际 provider、原 ID／path／来源／阶段，worker 自己持输入到实际结束；TS 处理栈另持原件，summary／metadata artifact、解压输出和译后 JSON 所有权分开。受理 false／同步终结、准确取消、旧回调和通知中销毁均须限定静读，不借共享 cancel 清他人。

新 HTTP／FTP 原件上限32MiB并各有120秒总期，检查完整读写／关闭；HTTP 有限重定向、Cached 原任务最多一次 cache-only 回退；FTP新增模16序号回绕、4096缺块预算和零进展包拒绝。CRC／schema 的既有边界保留；唯一文件不认证 TLS、机载身份、PRIMARY wire 来源、真实性能或 StartPermit，亦不保护任意违规 native caller 并发复用同 lease。

本批15源码／工程登记＋19文档=34路径，新路径6条；源码实现与限定静态交叉审阅已收口，64项工程场景全部未执行，不代表可运行版本或完整可靠性验收。未编译、执行项目／parser／helper／CRC／hash／下载／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%源码／60%剩余仅粗估。

[输入所有权](src/QGCOwnedDownloadFile.md)、[下载接线](src/Vehicle/ComponentInformationManager.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。


## 527 当前：参数定义整份接纳与实际Fact副本（源码静态收口）

继续原组件元数据的真实消费链，而不是新增只读报告。已核旧CompInfoParam在验证前关闭fallback、逐条发布live map，通用factory错误又返回非空默认对象；坏尾、重复加载或未知类型会留下半批／旧数据。本批已实现参数专用整份校验、候选目录一次切换及明确失败回退，不改原通用FactMetaData factory的其它内部资源接受域。

本地PX4实际jsonout生成version1，虽vendored／上游schema已是不同版本，不能仅按最新schema替本机猜类型／版本。参数专用解析以实际七数型为依据，明确v1严格数字String兼容、最终min／max／default／increment、完整values与bitmask及安全移位；未知扩展保容忍但计资源预算，不冒称全draft-schema或设备真实性认证。

真正的三个Fact创建入口改为actual Fact-owned元数据副本，避免目录替换时删除被Fact借用的模板；缓存hash只查询已知volatile值，未知／类型冲突放弃快载而正常读参数。已存在Fact保自己的旧副本，不称热更新或当前构型重新认证；CRC副本／字段本身不证明构型、性能或执行许可。

Manager在准确旧source退休及新GENERAL前清实际各组件参数目录资格。独立候选／旧目录的回收、XML fallback所有权、实际source／代次与QObject通知重入已完成限定静态交叉审阅；不要求initialConnectComplete，不复活旧批准或新增参数写入／飞行命令。

CRC先冻结原来源再跨观察失效通知／provider创建，防旧HASH重基新源；不用会被正常参数就绪推进的观察epoch，另冻本地目录revision。原HASH ACK改走私有固定消息门，连接虚调及插件调整后再核来源／原payload，到write入口前最后复核；进入原writer是已调用事实，不冒称驱动完成、字节已发送或远端接纳。其它generic sender不改，也不增加控制协议。

本批12源码／工程登记＋13文档=25路径，新路径4条；58项工程师场景全部未执行。未编译、运行项目／解析／helper／CRC／hash／下载／缓存／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标和六类实质剩余保持，约40%源码／60%剩余仅工作量粗估。

[参数解析](src/Vehicle/ComponentParameterMetadata.md)、[实际消费者](src/Vehicle/CompInfoParam.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 526 当前：组件元数据字节校验与后台快照（源码静态收口）

回到能力证据的实际接入链：原组件Manager下载和缓存命中只把声明CRC作缓存标签，不比较实际文件；本批已写入统一后台原始字节核对及独立JSON快照，最终仍供原general／参数／事件／执行器消费者使用。本批源码实现及限定静态交叉审阅已收口，不是可运行版本或完整能力认证完成。

新raw-v1_缓存只保原始传输字节，每次命中重新校验，不把旧解压cache当原件；坏命中最多精确退休原tag一次，两文件移除确认成功才重下原URI一次。当前解析只消费本次独立JSON，不消费insert返回的旧路径；原raw／JSON快照与可转交缓存候选明确分开。

CRC复用现MAVLink算法，初态0且无终异或。当前配套PX4生成脚本给通用元数据计算未压缩JSON、URI却为xz，子元数据计算压缩文件；因此仅实际PX4 GENERAL允许明确decoded-px4-general兼容域，其它仍核传输件。无CRC保持not-compared，不补认证、真实性能、构型或执行权限。

后台固定raw32MiB／JSON32MiB／字典64MiB，最多三份独立唯一临时文件；分块、EOF／错误、取消及实际worker结果和finished双尾分别处理。原始输入不移除，快照由实际所有权释放；有界新解压入口与旧两参数入口分开。三份合计最多96MiB临时磁盘，不声称总解码内存仅64MiB或绝对IO／CPU时限。

通用目录在新batch先退休旧来源，候选类型／CRC／URI整份通过才一次安装；坏文件不保旧目录或成功前缀。参数定义工厂完整schema及半装问题仍待后续处理，不因文件CRC匹配宣称定义已验证。

Manager冻结本次来源、PRIMARY、请求／阶段和实际worker；请求回调及InitialConnect完成回调补实际QObject寿命。Vehicle关闭／移除／析构先退休本通道，避免部分析构中继续旧步骤；可选步骤失败终结不是文件成功。初始加载不要求initialConnectComplete，不借共享FTP.cancel清别人，不把文件一致当远端PRIMARY字节、HTTP端点或设备身份认证。

翻译改为本次唯一TS／译后JSON输出，完整读写及XZ各32MiB／字典64MiB；冻结原下载器／URL／代次、先退旧槽再通知，迟回只清本次唯一输出。译文不是原CRC核过内容；底层下载basename碰撞、递归引用图深度／CPU与全部临时内存仍是后续缺口，不称全部跨机输入隔离或完整图预算已解决。

本批18源码／工程登记＋15文档=33路径，新路径7条；48项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／CRC／hash／解压／下载／缓存实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、PR更新或打包；没有实际下载、读写或删除业务缓存／归档。原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估。
[加载接线](src/Vehicle/ComponentInformationManager.md)、[后台快照](src/Vehicle/ComponentMetadataArtifactWorker.md)、[原始缓存](src/Vehicle/ComponentInformationCache.md)、[翻译输出](src/Vehicle/ComponentInformationTranslation.md)、[33路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 525 当前：已核备份后的单个退休原件永久清理（源码静态收口）

补长期运营的真实磁盘退役路径：旧retire／restore仍只改名，备份仍另存；新增独立只读预览及明确永久清理一个retired原件的入口，不触当前库、sealed段、旁路或备份，不批量、递归或自动清理。本批源码实现及限定静态审阅已收口，不是可运行版本、放行或完整长期留存策略完成。

依据只来自原实际backup worker的完整backup31、backupVerified且source.kind=retired，并等原resultReady与actual finished双尾；公开last、文件名、hash、目录或导入材料不能造删权。新原生私有basis／原preview绑定本次操作；请求固定五个字符串，仅preview／commit／discard／cancel，不能携任意路径、文件Map或proof。

预览worker只读核原完整SQLite／double SHA、两目录与实际Windows文件对象；提交消费原30秒预览并重新核原两file ID及同handle字节，再以原源handle请求删除。非Windows明确unsupported，不用按路径删除或跨平台不安全回退。已核副本不等于异地介质、原始事件真实性或掉电耐久。

提交另以fresh操作UUID创建不可覆盖的小型意图／终态收据；意图完整写入及flush确认前不删除。删除调用、可能删除、原件事后缺失、备份后核、两handle关闭及收据错误各自保留；晚到取消或终态收据失败不伪造回滚。sourceLogicalBytes仅原件逻辑大小，diskSpaceReclaimedVerified仍false，不声称物理空间已释放。

原维护页内嵌独立小面板，经原archive.retiredPurge门面显示原件／副本与两项确认；旧History Loader不新增控制器或服务。交叉审阅修正真实137字符备份名误拒、结果接纳后按钮自锁和字段通知中销毁漏取消；焦点沿实际Flickable滚动。UI/UX只静态指导原Qt主题、独立确认、来源文字及键盘反馈，未运行搜索脚本、预览或交互验证。

本批10源码／资源＋15文档=25路径，新路径仅PurgePanel.qml及合同两条；64项工程场景全部未执行。未编译、执行项目／parser／helper／业务／QML／JS／模型／认证／SQL／hash／原生Windows API实现、测试、仿真、操作硬件／PX4、刷机、放行、提交、更新PR或打包；没有实际删除、创建或读写业务归档。原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估。

[原生合同](custom/src/Swarm/FleetPersistentEventStore.md)、[worker](custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)、[界面](custom/src/Swarm/FleetRetiredArchivePurgePanel.md)、[25路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 524 当前：已闭部署归档与本地容量回收

回到任务执行生命周期，新增明确选择旧本地组、独立选路径和确认后的归档／退休流程。原最多8次终身分配限制改为最多8个live或等待实际销毁的组；终身分配尝试另以uint64字符串累计，不回绕、不重置。没有自动淘汰、增大容量、复用旧对象／批准，或重建Orchestrator绕过128条受保护记录限制。

只允许非current／非当前监视、原实际来源完整的旧组。原生检查全部初始及最多8个ground事务／Run、真正静默栈及实际Swarm整张owner表无原占用；有MODE准备的Run须原独立Close已提交，明确从未began的原unissued关闭例外不靠空值猜测。完整pristine未发布失败组可为0报告；注册task失败的真实未发Run仍完整保在quiet材料，不造任务。已began但未issued分支不再自动退休而堵死人工Close，仍走既有双账号关闭，不放宽归档资格。

后台复用原protected8完整报告codec，逐原record16生成0..9份报告，保完整quiet18和独立document15，总32MiB／单报告2MiB。保存、commit、回读与最后原生退休分开：commit不明不称未写，成功后取消／回读失败仍保文件已写，但不退休。文件回读匹配后还须原对象、完整quiet及全部record逐值尾核；不把hash、文件内容或操作员勾选当当前原生来源。

专用native批次只退休准确已闭记录，普通removeTask仍拒受保护记录；同批清其本地语义缓存，不删除排队历史事件或他人占用。旧组及全部受控QObject后代（最多256个）弱引用均实际为空，并在后续turn复核后才回收8槽；父destroyed信号和deleteLater不算全部销毁。原source-lost监听按准确组隔离，退休旧组不毒化后来current，也不把本地回收称停飞、落地或任务Completed。

原部署管理弹窗新增归档区；Root只给typed九字段窄请求及状态／结果，不公开Episode、账号或引用scope。独立打开文件可无live飞机／账号，不恢复任何对象、观察或权限；只显式选一份原生canonical报告文本，按2048UTF16分片查看，不在GUI解析／序列化整包。文件时间明确仅文档声称；弹窗重开可只读查看原生上次事实，但不能认领为本页新请求。UI/UX仅指导原Qt主题、分层确认、长标签／错误反馈和键盘操作，未执行技能脚本或预览。

外来job即使打开声称同archiveId的另一文件，也退休本页旧历史目录与报告；本页open受理同样先撤旧历史资格。界面忙碌期间被挡的job通知在原调用票释放后补核，不能用后来state将新文件报告拼在旧标题下。这里只撤界面资格，不删除磁盘文件或伪造取消别人的任务。

本批19源码／资源登记＋20文档=39路径，新路径6条；无新QML后端服务、wire、外部依赖、SQL schema、飞控或PX4修改。56项工程场景全部未执行，只做源码、直接静读和普通文本／路径／Markdown／空白核对；未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。完整25目标和六类实质剩余保持，约40%源码／60%剩余仍为工作量粗估；本批不证明最终StartPermit、正式远端任务结果或全部长期运营完成。

[生命周期合同](custom/src/Swarm/FleetMissionDeploymentLifecycle.md)、[归档worker](custom/src/Swarm/FleetDeploymentArchiveWorker.md)、[界面合同](custom/src/Swarm/FleetDeploymentArchivePanel.md)、[39路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[56项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 523 当前：GNSS 历史集合与独立文件工作区

将522的真实同包报告接到原FleetOperationalMemory的独立历史集合：只允许实际Archive及准确原生报告token追加，不接受QML报告／坐标作为来源。保每页完整33字段、原7身份及18字段样本；按run／headerHash／原sequence去重，不依会随文件增长变化的fileHash，也不把采集Run或数字机号当可信任务／实物身份。最多32报告／2048样本，重复或容量满整份拒绝，不自动裁剪去重依据。

旧运营观察、KPI、热区推断、5000条裁剪及旧schema1导入／清空完全不接这批记录。新增共享纯值报告与集合校验；它只验证文件内容自洽，不认证原始采集、设备、精度或飞行状态。无GPS和无fix历史仍保诊断，不补旧坐标、不计算任务完成率或环境风险。

新增显式冻结、选路径后另行确认保存、独立只读打开、每页8份目录及单份报告查看。文件root为新闭11字段，4MiB上限；冻结保原Memory instance／revision和不可变字节，打开文件不导入active记忆、不恢复权限。单槽worker只消费纯值，实际结果与线程结束双终结后接纳；取消不抹掉已commit文件，写入已知／未知与readback、fsync／durable分开。

提取共享FleetSnapshotFileIo，让维护与GNSS工作区使用同一路径检查、64KiB读写及QSaveFile提交／回读逻辑。原维护32MiB、错误码和回执边界保持，GNSS用4MiB；不是恶意并发／祖先reparse／失电安全认证。本轮只改实现，没有实际创建、读取或写出任何业务档案。

历史面板新增独立记忆区：追加、冻结、保存、打开、选报告与选地图均须明确动作，不自动读写／续页／选首条。原地图共用唯一选择，投影新增origin区分原生页、session冻结历史和文件历史；只绘当前选中的一份离散点，不把文件值冒充当前Archive或实时定位。UI/UX静态指导用于原Qt主题、分层确认、文字来源和键盘操作，未运行技能脚本、预览或交互验证。

本批18源码／资源登记＋18文档=36路径，新路径8条；无新QML后端服务、wire、外部依赖、数据库schema、权限或PX4修改。仅源码、直接静读及普通文本／路径／Markdown／空白核对；48项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。

原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估。本批是有界历史留存与重开链，不是全历史自动索引、自动留存清理、跨任务统计、链路／风场模型或最终StartPermit；active集合满时仍须明确拒绝，不通过清文件槽偷偷清空。

[运营记忆合同](custom/src/Swarm/FleetOperationalMemory.md)、[文件工作区](custom/src/Swarm/FleetGnssMemoryWorkspaceWorker.md)、[界面](custom/src/Swarm/FleetGnssMemoryWorkspacePanel.md)、[36路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 522 当前：同包历史 GNSS 诊断与显式只读地图

为集群记忆地图接入第一条真实历史地理依据：使用现固定本机遥测档案的原生已完成页，另行确认后只解释同一GPS_RAW_INT帧的位置、fix、HDOP与卫星数。最多64帧，先复用现MotionReview完整页校验再解码；不从Registry、实时Fact、其它消息或分页补位置与质量。没有GPS的合法页明确为空，不等于区域安全或从未出现异常。

fix仅3／4／5／6／8且经纬度合法才发布该帧报告坐标；0／1／2／7及未知fix只计未定位，旧坐标不投点。原eph65535与卫星255保未知，不补0；HDOP无单位、不是米精度。HDOP>2.5或卫星<6只是固定显示提示，质量未知与阈值内分开，均不认证定位精度、漂移原因或环境风险。

Archive保实际页来源与独立新报告UUID。原六字段action与十一字段回执只新增gnss-review动作；原运动／四视角报告和维护流程保持。界面独立确认GNSS位置发布，再另行明确选择地图，不自动选图／续页／拟合视野。地图只绘离散历史点，含质量文字图例，点与标签吞点击，不接Goto、任务编辑或飞行控制。

地图选择只接受实际报告token，Root自行读取原生来源；原owner／完整报告／token与独立选择代次绑定。输入、隐藏、换源或新查询退休本页选择，旧页面不能关闭后来替换的选择。公共投影固定深度冻结；不支持冻结时不绘图，不把脱离副本当不可变保证。

本批8源码／工程登记＋13文档=21路径，新路径仅GnssReview h／cc／md三条；无新QML资源／服务、wire、依赖、数据库schema、权限或PX4修改。UI/UX技能仅指导原Qt主题、独立同意、文本图例和键盘焦点，未运行技能脚本、预览或交互验证。

仅源码修改、直接静读及普通文本／路径／Markdown／空白核对；40项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍为粗估。本页图层不是持久跨任务全集、风场／链路风险地图或最终StartPermit。

[原生报告合同](custom/src/Swarm/FleetTelemetryGnssReview.md)、[历史工作流](custom/src/Swarm/FleetTelemetryHistoryPanel.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 521 当前：普通区域任务的速度／续航准入下限

补通普通区域任务的两项性能要求：最低速度能力minimumSpeedMps（m/s）和最低续航能力minimumEnduranceSeconds（秒）。现有性能政策编辑器将原strict三态与两个可选下限一次提交给原草稿，不分次保存半套政策；空白不补0，显式0与缺省保持不同来源身份。这里不是巡航速度指令、当前剩余飞行时间或实测性能认证。

新增原生三键完整政策CAS：只替换requirePerformanceEvidence及两floor，保其它settings；任一下限存在（包括0）使用planningSource schema6。schema1..5不能夹带新字段，6缺全部floor也拒绝；旧无floor来源不自动升级。普通设置编辑、检查点保存／恢复及修复仍保完整来源、准确revision与原datum重确认，旧审批不恢复。

Matcher与角色分配在选择前、Compiler在固定主／备用成员上都执行正下限；缺值或Registry实际−1不能通过，不拿其它机的最大值抵扣。strict=true仍只取本机当前构型匹配的工程声明；false／未声明可用原规划描述符，但不能关闭明确下限或宣称可信实测。0／缺floor不新增正值证据依赖，当前电池与逐机能量门不变。

Compiler任务根、归档计划和PayloadBuilder按presence、类型和完整来源值绑定两个下限，不能在保存、修复或直接编码时丢字段／降级。界面复用原Qt主题和UI/UX静态指导，明确单位、未知／0、采用／取消及失败反馈；没有运行搜索脚本、预览或交互验收。

本批9源码＋18文档=27既有路径，无新类／资源、wire、依赖、数据库schema、权限或PX4修改。仅源码、直接静读与普通文本／路径／Markdown／空白核对；48项工程场景全部未执行。未编译、执行项目／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍为粗估，不按字段或控件数上调。

[草稿合同](custom/src/Swarm/FleetMissionDraft.md)、[性能政策入口](custom/src/Swarm/FleetPerformancePolicyEditor.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 520 当前：备用出勤显式只读刷新与本次请求归属

为519的被动参数读取补上独立、人工确认的刷新入口。复用现有FleetMissionEvidenceRefresh读取实际版本及固定29参数，不新增发包服务、参数写入或飞行命令；原账号／角色、真实地面未解锁、PRIMARY链路和占用门禁保持。

本次刷新绑定原私有capture、明确选中的备用机、实际scope／刷新对象及真实requestId／generation。开始前退休旧参数offer、提案和审阅资格，但保留来源与已归档事实；状态与取消只认本次原请求，不认同数字ID或共享latest。界面离开／换源先撤本次资格，取消仅停止后续读取，已排队请求和晚到回包可能仍在途。

刷新结束只显示实际结果，不等于12参数仍满足30秒新鲜度。操作员仍须单独重新读取本次依据、明确采用预算速度和新配置，再走原完整审阅及审批；不自动刷新、重试、observe、采用、批准或恢复旧权限。同来源只保本页手填的动作／坐标和已明确采用的启动策略纯拟值，未采用启动策略子表单须先处理。

本批2源码＋13文档=15既有路径，无新组件／资源、C++、wire、依赖、数据库schema、权限或PX4修改。仅源码编辑、直接静读和普通文本／路径／Markdown／空白核对；工程场景全部未执行。UI/UX静态指导用于原Qt主题、请求归属、状态区别、反馈与键盘焦点，没有运行搜索脚本、预览或交互验收。

未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍是粗估，不因刷新按钮或字段数上调。

[刷新工作流](custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[面板合同](custom/src/Swarm/FleetReserveDispatchPanel.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 519 当前：备用独立出勤的真实参数绑定与结构化配置

原备用出勤要求人工填写完整executionProfile和参数hash。本批为实际冻结capture／选中reserveId接入现Coordinator原生12参数只读offer；Root保留私有本次offerToken、原对象与参数fingerprint，生成提案前重新读取并核对。外来Map、同值ABA、旧token或另一备用的参数不能代替本次真实来源；原生参数读取不刷新或写入参数。

新增纯值配置编辑器，不伪造Draft。操作员明确采用本次备用预算速度，再选择起飞／结束动作、AMSL坐标、时序和完整启动策略；requiredProfileHash只取本次实际offer，不再手填。复用原启动策略编辑器，完整JSON仅供核对；没有从旧主机、GNSS或模板补起降位置和安全默认值。巡航与返航仍须同原备用cruise，垂直速度仍须匹配原预算和实际参数；不兼容时回原计划重做，不静默修正。

仍走原reserve-dispatch Planner→Compiler／能力与能量／当前电池审阅→归档／采用／独立批准／共享部署流程。新三参数preview入口要求准确offerToken，底层原exact3请求、profile schema1与启动策略1–4不改。读取和生成配置都不是上传、接管原任务、性能认证、StartPermit或自动出勤。

本批5源码／资源接线＋13文档=18路径，新路径仅ProfileEditor.qml和对应合同md两条，无新C++、wire、外部依赖、数据库schema或权限。40项工程场景全部未执行；UI/UX技能只指导原Qt主题、分步确认、完整输入／错误反馈与静态焦点核对，未搜索、预览或交互验收。

仅源码、直接静读和普通文本／路径／Markdown／空白核对；未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估，不按表单或字段数上调。

[实际工作流](custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[配置编辑合同](custom/src/Swarm/FleetReserveDispatchProfileEditor.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 518 当前：维护护照显式保存与独立历史工作区

将517的本进程维护记录接到实际可见的文件工作区：显式冻结当前内容、另选路径并确认覆盖风险后保存；选择已有文件并另行确认读取，成为独立只读历史。当前护照不被历史load/import替换，文件、hash和来源组不认证实物、维护状态或权限。

冻结保原实际records／instance／revision，GUI分turn只验纯JSON类型和有界结构；文件读写、JSON规范化、评估与完整文本分页交独立单槽worker，原job结果与线程结束双尾后才接纳。517实际追加可继续，旧快照内容不会改成新记录；来源版本变化只使sourceCurrent失效，不悄悄重导出。

来源组、记录目录每页8项；完整记录／评估按2048 UTF16片浏览，长备注不整包交界面。保存分fileWrittenKnown／fileWritten与独立readbackVerified，commit失败不谎称未替换，commit成功不被取消或回读失败抹掉。QSaveFile禁直接写回退，fsync／durable／认证仍false。纯legacy根5、历史根6与原严格来源元数据保持。

本批10源码／资源接线＋13文档=23路径，新路径为worker h／cc、工作区Panel.qml和合同md四条。48项工程场景全部未执行；无新QML服务类型、wire、外部依赖、SQL schema、权限或PX4修改。UI/UX指导仅用于现Qt主题、分步文件同意、只读来源与错误反馈的静态设计；未搜索、预览或交互验收。

未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估；本功能不等于长期运营／维护预测整体完成。

[文件工作区合同](custom/src/Swarm/FleetMaintenanceWorkspace.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 517 当前：真实历史电池证据进入维护护照

从当前已完成的原生遥测历史页显式选择完整电池来源，预览本页报告温度／非负放电电流峰值和样本覆盖，再另行确认写入实际维护护照。未知、越界、充电与零值分开；不从实时Fact补历史，不新增遥测请求或采集流。

以Run／头摘要／文件摘要／sourceEpoch／机号／组件／电池槽七字段隔离历史来源组，不与同数字机号的手工记录混合。相同来源组的选中序号闭区间重叠拒绝；容量2000满时明确拒绝，不淘汰去重依据。schema1文件保完整历史元数据并原子拒绝重复／重叠；历史页峰值不计算物理两点趋势，不补振动RMS、寿命或飞行次数。

新增嵌入式维护页与现指挥中心来源组摘要；私有Archive／Passport由Root实际配对，公共接口不接任意record或raw payload。页面失效、换源、隐藏、选择／同意ABA不得沿用旧确认；已追加事实不因界面退场被当作回滚。只保本进程记录，本入口不自动落盘或恢复权限。

本批10源码／资源接线＋13文档=23路径，新路径仅Panel.qml与Import.md两条。40项工程场景全部未执行；未编译、执行项目／helper／QML／JS、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。仅源码、直接静读和普通文本／路径／Markdown／空白核对。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估；本功能不代表目标21整体完成或当前飞机适航。

[实际数据与API合同](custom/src/Swarm/FleetMaintenanceHistoryImport.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 516 当前：近期原生SOC下降窗下调已有续航预算

新增独立native SOC下降窗：实际原PRIMARY收包、最早RAW本地时刻和原complete尾核后留有界采样；30–120秒、至少下降2个百分点、原报告gap≤3000ms、最多256点才可发布。flat不刷新已发布终点／速率，最新RAW须≤3000ms，下降终点须≤30000ms；模型统一扣两者较大年龄。上升、未知、坏时钟、用途／类型或来源断裂清窗，不从Fact缓存拼接。

实际Registry在同Vehicle／recordEpoch／bindingEpoch下读取旧电池20字段与新top25／window8，剥离静态root／metadata同名键。新共享校验不认证Map；坏新证据明确拒绝，合法warming／stale／unavailable不补模型。报告SOC不是实测电功率或物理电池身份。

Scheduler只下调已有known可用秒数及已有known电量投影，随后重算缺口、成员资格及替补建议。严格性能声明与原故障门保持；assignment18/21、编译备用预算和旧电池schema不改。可选nativeBatteryDrainAdjustments逐行14字段保完整窗口、应用前后与原因，并沿原完整energyPlan归档／报告／515明细；普通规划页只新增依据标签。

Root通过exact6共享值桥消费语义身份，排除自然age及重复receipt抖动；窗口失效／换源／重建使旧审阅不当前。批准期还核实际扣龄预算是否覆盖原所需秒数、是否新跨原替补阈值，不重写旧账单或自动批准。估算、上传、机载接纳与最终StartPermit不等价。

本批11源码／接线＋18文档=29既有路径，无新增文件、资源、wire、外部依赖、数据库schema、权限或PX4修改。48项工程场景全部未执行。UI/UX指导只用于原主题下的清楚只读依据标签，没有搜索脚本、预览或交互验收。

[原生观察](custom/src/Swarm/FleetNativeBatteryObservation.md)、[实际数据链](custom/src/Swarm/FleetNativeBatteryBridge.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标／六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估，不按采样点或新字段数上调。仅编辑、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。下方为历史。

## 515 当前：新出勤逐项编码／完整账单与保留提案重审

备用出勤新记录新增默认折叠的只读明细：实际Spatial view17／逐机15／编码项9，分页核对每个真实seq、command／frame、原来源索引、raw／canonical和量化字段；完整能量、风险、空域及审阅值可逐层浏览。每层8项，长键和值分片，不把缺失／null／false／0或负余量混为未知，不用旧区域账单或合成启动捕获补源。编码拒绝保完整错误及未编码要求，不造成功前缀。

新增明确“保留本提案重新审阅”。只用本页实际capture／proposal、实际reviewAttemptToken、旧reviewToken／requestId精确认领；先退休旧采用／本地批准，保私有完整提案，再走原当前证据审阅／归档。失败和过期可人工恢复流程，不清原备用来源强迫重选；外来新记录、活动传输或规划不被抢占。新结果仍须重新采用、申请批准／明确批准与原独立上传确认，没有自动刷新、重试或旧权限恢复。

撤旧通知的来源隔离覆盖初次捕获Store、重审Store及末尾直接Intent失效三处；同值新prepare不再借旧通知被退休。初次捕获仍只容本次实际批准→preview的一次代次推进，原preparation（含null）、内容与provider／scope不得替换。详见本批重审合同和静态审计，实际重入行为仍待工程师验证。

本批6源码／资源接线＋15文档=21路径，新路径仅ReviewPanel.qml／ReviewPanel.md两条。没有新C++、wire、外部依赖、权限或飞控修改。48项工程场景全部未执行。UI/UX技能用于现Qt/QGC主题、渐进展开、分页／长值／焦点的源码核对，未运行技能脚本、预览或视觉／交互验收。

[新明细合同](custom/src/Swarm/FleetReserveDispatchReviewPanel.md)、[重审工作流](custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本轮只完成源码与静态接线，不表示机载已接纳、任务接替、实际轨迹／净空或最终StartPermit。原25目标／六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估，不按明细或按钮数上调。仅编辑、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 514 当前：备用独立出勤的可见人工流程与唯一共享部署

在513完整后台提案之上新增真实Root来源捕获和FleetReserveDispatchPanel：操作员先明确冻结当前完整task／plan／存档绑定，再选一架实际备用、填写完整新executionProfile、核对自身完整去返路线与summary14，重新提交正常审阅。捕获先冻结再撤旧，精确自身退休收据仅容一次真实批准→preview推进，其它输入／provider／代次或ABA变化仍拒绝。

新kind=reserve-dispatch贯穿捕获、提案、同步／异步回执、审阅归档与唯一共享部署区；旧single／group不能认领、批准或清理它。采用、本地申请批准／明确批准、不同账号上传审批与覆盖确认分别保留，采用不是批准。采用后自身pending不自锁部署，隐藏／退出／取消只处理自己的准确token与请求，不删除已提交事实或自动切回草稿。

独立视图消费本次准确Store记录与同份reviewEvidence，经既有missionSpatialReview和实际PayloadBuilder生成；地图只绘同一新记录的完整两条二维路线，不造Run、原序号或起降轨迹。原未知政策／共同floor／边界、全新profile、当前能力／电池、原生上传门禁与最后StartPermit不放宽。这不是实际备用出勤、原组接替或自动机载改派完成。

本批8源码／资源接线＋18文档=26路径，新路径仅Panel.qml／Panel.md／Workflow.md三条；没有新C++、wire、依赖或权限。56项工程场景全部未执行。UI/UX技能仅用于现Qt/QGC主题、确认／错误／键盘焦点的静态核对，没有运行搜索脚本、预览、交互或视觉验收。

[Root工作流](custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[可见面板](custom/src/Swarm/FleetReserveDispatchPanel.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标／六类实质剩余保持，约40%源码完成／60%剩余仍仅工作量粗估，不按文件、入口、批次或防护数量上调。仅源码修改、直接静读与普通文本／路径／Markdown／git空白核对；未编译、执行项目解析器／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 513 当前：备用保障方案生成独立出勤任务的后台接线

新增FleetReserveDispatchPlanner：从完整原task／plan／binding和明确选中的备用机，生成一份独立单机新任务。新执行路线使用备用自身起返点，并保留原被替补成员全部航点、heading与完整停留；不能直接拿已简化的预算路径代替任务要求。新executionProfile必须人工重新提供且与预算速度一致，不沿用原主机的起降配置；历史待机不在新任务重复执行或扣时，当前电池仍须新审阅。

新reserveDispatchPlanningSource为闭17键schema1；结果7键、摘要14键。完整task和member／constraints／原边界／严格性能布尔分别绑定，Compiler前置任务核对与输出前真实plan核对、PlanStore归档及PayloadBuilder正常编码入口均消费同一来源；四种规划来源互斥。旧single／group再规划只清旧出勤来源并重新绑定，不继承Run、批准或控制权。

保留共同安全／能力／能量及所选成员政策和floor；显式单机用途重设成员／角色数量和备用0，不声称原组已接替或原组保障不变。原clearanceInput不自动换ID，不适用仍阻断。源32MiB、任务／匹配16MiB、请求128KiB、provenance64KiB及每机2048／全组8192编码项等预算不扩，未知要求不能为生成成功而静默丢弃。

本批仅完成后台规划与真实编译／编码／归档消费者接线，尚未新增Root真实来源捕获、可见按钮、确认流程或共享部署源选择；这些是同一备用出勤功能接下来的实质工作，不把Q_INVOKABLE当可操作产品或真实任务执行。正常审阅、独立审批、当前能力／电量及最终StartPermit仍不放宽。

[备用出勤合同](custom/src/Swarm/FleetReserveDispatchPlanner.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批10源码／工程登记＋18文档=28路径，新文件仅Planner h／cc／md三条。40项工程场景全部未执行。仅源码修改、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标／六类实质剩余继续，约40%／60%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 512 当前：联合余段任务重建新备用保障

联合余段任务现在支持显式重填完整新备用配置，解决原主／备用换机后旧reservePlans仍指旧ID而必须阻断的未开发边界。不是暗改数字或删除原拒绝：每个新备用恰一份自身起点／返点、替补的新执行机、待机与速度策略，完整新方案仍重新进入现Compiler→ReviewCoordinator→EnergyScheduler。

原三键请求继续旧保守合同；含reservePlans字段就要求新四键完整请求，坏容器不能降级。新provenance schema3／exact28保完整canonical reservePlans；原sourceTaskPolicyHash继续标改前政策，完整新task／routes摘要绑定新意图。新备用运行配置允许显式重新规划，不冒称旧待机／速度值全部未变；最低备用数量及其余共同安全／能力／能量／返航政策保持。旧clearanceInput不自动换ID，资料不适用仍阻断。

task与compiled-plan匹配分开。后者消费真实reserveMembers.energyPlan，以现备用生成器从实际新成员路线重算完整路径／待机／任务／返程成本及关联，不能仅比ID、首尾或hash；当前性能分支不得自行降为legacy。值重算不认证当前性能或来源，实际新能力／电池与原审批来源尾核仍必须成立，旧档案不恢复当前权。

现联合面板新增有界JSON输入及逐备用预览，沿原主题／PlainText／焦点反馈和两步人工确认。文字最多32768字符、最多32描述；原4MiB请求、64KiBprovenance、16MiB任务／匹配、32MiB源／结果及32768生成点预算不扩。没有新模块、资源、依赖、命令、数据库schema或自动改派。

[联合规划器](custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)、[操作面板合同](custom/src/Swarm/FleetMissionGroupRemainderPanel.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批4源码／接线＋13文档=17既有路径，48项工程场景全部未执行。原25目标与六类实质剩余保持，约40%源码完成／60%剩余仍为粗估，不按本批路径或局部功能上调。

仅源码、直接静态阅读与普通文本／路径／Markdown／git空白核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。新备用预算不证明真实待命、已接替、机载改派、可信实测耗能或StartPermit；完整目标继续。下方为历史。

## 511 当前：MODE发送前的真实begin检查点

原生begin在安装本次真实queueBegan后，立即走同一保护模型提交；只有提交接纳成功、原ReadContext／serial与实际对象仍同、原queue仍处于未commit且可跟踪状态时才返回true。内部Target引用不跨完整Evidence的Link虚调，回调后先核QQ／原上下文再重新定位。失败不回滚已安装的开始事实，也不据日志失败抹掉后来实际write。

这使510事件链能在成功提交时分别捕获“开始准备”和“已调用实际发送”，而不是把两者合并猜测。不是额外控制器、远端ACK、网络送达或正式MODE执行。Vehicle原调用者仍再核原binding／授权／消息不变性；实际write后原commit始终先保真实调用事实，其旧生命周期语义不因新的bool返回值而回撤。事件队列仍可能有明确缺口，不能承诺每项已持久化。

完整Evidence会调用Link虚函数，可嵌套RAW、取消、退休或销毁；这里只保证新路径不直接emit／写文件／发控制，不能宣称纯内存无外部回调。没有新wire、公开API、报告schema、账号权限、资源、依赖或新增路径。

[执行观察合同](custom/src/Swarm/FleetMissionExecutionObservation.md)、[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批2源码＋13文档=15既有路径；工程场景28项全部未执行。原25目标和六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按检查点或文件数上调。

仅源码、直接静态阅读及普通文本／路径／Markdown／git空白核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。本检查点不授最终StartPermit，不认证远端任务实例、正式结果、占用释放或放飞。下方为历史。

## 510 当前修改范围：8源码＋17文档=25路径

受保护原生TaskRecord提交后的注册、MODE本地交接、终点候选、连续性异常、源退休和观察关闭，新增独立低频事件通道进入实际BlackBox及其持久归档，不再只停留在内存观察通知。逐包计数／年龄／临时sourceCurrent不作为事件变化；同Run的异常位图保历史已观察事实。

独立FIFO最多512行／2MiB canonical payload、单条4096字节，每turn最多64条；不是堆内存硬上限。溢出保旧事件并累计缺口，原生临时收据只在实际发布期间有效；缺口不归入假任务。Root另记本地接纳／未记录／未知，BlackBox接纳不等于SQL提交或完整历史，持久录制仍默认关闭。

新v3固定12字段脱敏投影保28字段runObservation；原v1／v2、SQL及历史查询schema不变。实际按taskIdHash下钻和两历史面板新增只读摘要；现报告将reviewRequired=false普通转换作为info，异常只作warning人工复核，raw保原事件。日志timestamp是实际入日志的本机UTC，原捕获UTC另保，不对队列事件偷偷排序或改写时间。

[事件合同](custom/src/Swarm/FleetRunObservationEvent.md)、[实际日志接线](custom/src/Swarm/FleetRunObservationJournal.md)、[25路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[60项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批8源码／接线＋17文档，新路径仅两份模块合同，无新代码文件／资源／外部依赖。

这仍是本地Run观察，不是任务Completed／Failed、远端实例认证、正式结果、占用释放或飞行许可。原25目标与完整执行／可信性能／自愈机载改派／跨站协作／长期运营／真实模型插件六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按本批路径上调。

以上是本批人工范围，不是全脏工作区统计、已导出patch或安装包。仅源码、直接静读及普通文本核对；未编译、执行解析器／helper／业务／QML／SQL／认证／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 509 当前：人工单流控制链

本批涉及14源码／实际接线与16文档，共30路径；新增仅FleetCameraStreamControl.h／.cc／.md。不是整个脏工作区统计，也不是已导出补丁、更新PR或安装包。

主要变更：独立写权限／双账号typed审批；真实Capture一次基线交接；固定原30秒的单次2502／2503→512／270原生事务；与legacy切流双向互斥；私有配置代次区分RUNNING状态变化与配置ABA；现有Root／面板的独立审阅、确认、取消和partial事实展示。

[精确修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[64项工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。所有工程场景未执行；已有ACK／回报不等于实际视频控制效果。

本轮仅源码／文档及直接静态阅读、普通文本核对，不编译、执行项目实现、测试、仿真、硬件／PX4操作、刷机或放行。原25目标和六类实质剩余保持，约40%／60%仍为源码工作量粗估。下方为历史。

## 508 当前：人工相机流信息／状态读取

新增人工单流相机 Info269→Status270 查询：独立 READ 权限与真实账号／角色／Root 意图，经 Swarm 私有服务、Registry 原绑定、Vehicle 原生 Run 和现有共享消息通道，接到指挥中心／规划页面板。打开页面或原五秒策略不会自动查询；不启停流、配置、降速或恢复。

原入口总8秒包含授权准备，每阶段最多4秒，accepted ACK 后报告窗口最多1秒且只缩短；每项至多一次实际发送。允许真实来源满足检查的飞行状态 metadata READ，不把 RTT 的地面限制直接套入。取消保留已可能发送／ACK／报告事实，不宣称撤回。

ACK 与269／270无nonce，本地匹配不证明无线因果、实际送流／停流、完整目录或带宽变化；270索引兼容性待工程师确认，不支持不回退0／all／广播。complete 不等于两份报告此刻同时新鲜。远端受控视频降级与恢复仍待开发。

[服务合同](custom/src/Swarm/FleetCameraStreamCapture.md)、[面板合同](custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[28路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批14源码／接线与14文档；新增 Service h／cc／md、Panel qml／md共5路径，没有新外部依赖。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅为源码工作量粗估，不按批次、路径、接口或局部保护数量上调。完整任务执行、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 507：10源码＋15文档=25路径

新增原PRIMARY相机VIDEO_STREAM_INFORMATION269／STATUS270被动账本，经Registry实际绑定一次读取、Policy第五参数和Guided原五秒周期接入既有指挥中心摘要；不发新请求或控制命令。另修上一轮留下的setVideoMode／setPhotoMode实际caller，以及其toggle／reset模式意图衔接。

本批新文件仅custom/src/Swarm/FleetCameraStreamObservation.md；这是一份新合同，不是新独立helper／服务或已生成安装包。既有工程／资源入口保留，未新增权限、依赖或wire消息。

[完整合同](custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 506：7源码＋12文档=19路径

源码为CameraControl h／cc、CameraManager h／cc、VideoManager h／cc及共享QmlObjectListModel.cc；新增文件仅[生命周期合同](docs/handoff/MERIVUS_VIDEO_LIFECYCLE.md)。

本轮修改现有CameraManager／CameraControl→VideoManager真实调用链：本地播放意图、原相机／流选择代次、有限接收器恢复、活动飞机切换和通知回调后的对象存活检查；共享QmlObjectListModel补通知后的自身存活尾核与匹配reset。不是新增只读helper，也不是完整Fleet视频降级／受控停流事务。

[精确19路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[50项工程场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。范围是人工本批清单，不是累计脏树计数、25项目标的完成数或已生成补丁／安装包。保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505：2源码＋12文档=14路径

实际改动源码仅LogDownloadController.h／.cc；现有LogDownloadPage接口／资源登记不变。新增文件仅[机载日志补读合同](docs/handoff/MERIVUS_LOG_DOWNLOAD_RECOVERY.md)。

本轮修现有LogDownloadPage→LogDownloadController→LOG_REQUEST_LIST／DATA→本地文件的真实消费者：同一目录／批次来源、冻结选择、有界缺块补读、完整写入后位图与去重计数、NewOnly partial及完整刷新／改名后才显示Downloaded。不是另建只读建议helper，也不是完整Fleet控制中心补传。

[精确14路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项工程场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。清单是本批人工范围，不是累计脏树计数、功能验收数量或已生成patch／安装包。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504：16源码／接线＋15文档=31路径

504新增独立人工单次定向TIMESYNC往返耗时采集：实际主账号／Role／Root原意图→Swarm私有服务／原生Run→真实同车绑定review→Policy及可见面板。最早四秒包含授权核验和后续100ms轮询，不自动采集、重试或续期，不回退PING或广播。

新文件仅FleetLinkLatencyCapture.h／.cc／.md和FleetLinkLatencyPanel.qml四条；没有新可任意QML构造服务或Session二审purpose。独立action／私有Swarm绑定、原生六API、Policy第四参数、实际五秒只读消费、两宿主与qrc／qmldir／pri一并交接。

[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[44项全未执行场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。人工范围表不是累计脏树计数、已生成patch或安装包。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503：9源码＋14文档=23路径

503将实际PRIMARY／本车默认component的原始解码帧接到独立五秒接收窗，经Registry实际绑定读取、Policy独立消费、Guided原五秒周期和指挥中心只读摘要。记录接收帧数、编码MAVLink字节率与最大内部间隔；不把它们当可用带宽、RTT、丢包率或远端发布率。

新文件仅[本地接收观察合同](custom/src/Swarm/FleetLocalReceptionObservation.md)一条。Vehicle两个源、Registry两个源、SwarmController、Policy两个源及Guided／Overlay两个QML一起交接；未新增pri／qrc／服务注册。计数是人工登记的本批范围，不是累计脏树、功能完成数或已生成补丁包。

[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502：6源码／UI资源接线＋18文档=24路径

502新增FleetOptionalTelemetryPanel可见界面，在指挥中心和地图规划页复用同一actual Root。操作员显式只读采集→另行审阅统一目标→两项确认后独立账号审批→逐项配置与回读；固定62／74／83，原501实际后端和授权合同不变。不会自动采集、Apply、续期、重试或回滚。

新文件仅FleetOptionalTelemetryPanel.qml／.md；Root四个投影return补接线旗标与配置账号值视图，两request／source围栏新增可选的本次调用撤销态，原生服务、Session、Role和实际授权来源不改。详见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[面板合同](custom/res/Merivus/FleetOptionalTelemetryPanel.md)。[44项交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501：13源码／接线＋17文档=30路径

501新增独立双账号的地面可选遥测配置：从未来新成功Capture的三条真实native上下文封存不可序列化Basis，在同一最早30秒内独立审阅／批准，再逐项511→510并最终三项510复核。仅固定62／74／83、同一正int32且精确REAL32间隔，目标不得比任一原正基线更短；不启停流、不降频P0、不改视频或编队速度。

新文件仅FleetOptionalTelemetryConfiguration.h／.cc／.md；Session专用typed消费和Role二审政策、Capture opaque接管、Vehicle九阶段、Root意图及Swarm/pri接线一起列入。本表不是累计脏树统计或产品目标完成数；详见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

start仅认领ready、不写包；100ms后推进九个wire阶段。每条至多4秒，510 accepted ACK后至多1秒且只缩，所有阶段与审批共用原Capture最早30秒。慢链路可能无法完成；截止／失败／取消保留已发事实，不重试、不自动回滚。三项最终回读只给reported-values-match，不认证无线因果、实际发布率、带宽或飞行许可。

[46项工程场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500：主动只读间隔采集，未接配置Apply

9源码／后台桥接与工程登记＋15文档=24路径；新文件仅Capture.h／.cc／.md三条。原FleetRegistry／FleetLinkDegradationPolicy源码与15／21／6合同不改，新报告沿原账本进入既有只读重采。没有新可见按钮、qrc、依赖或可任意QML构造类型。

新增Swarm私有持有的FleetOptionalTelemetryCapture，显式单车62→74→83，实际Session主账号＋独立fleet.telemetry.interval.capture政策＋原Root/controller意图。接纳后100ms轮询才发；原入口固定30s总期，native每项一次510／4s，accepted ACK后至多1s且只缩，必须原ACK与目标244双端齐全。最终write前复核授权、物理来源、完整载荷并冻结receipt基线；三项同原version／transport／mutation／已绑定ledger，已采语义A→B→A不复活。取消保已发／partial事实，先存历史快照再精确释放原query，不宣称撤回或实际频率已生效。

精确路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，原生对象／值合同见[Capture](custom/src/Swarm/FleetOptionalTelemetryCapture.md)，[40项交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。本清单不是已生成补丁文件或安装包。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499：共享发送lane、有限512等待及旧基线撤销

源码5份：Vehicle.h/.cc、InitialConnectStateMachine.cc、StandardModes.h/.cc。改到真实generic／nativeVersion发送、原ACK与回包、定时终结及两个实际调用者，不改QML、Registry／Policy源码或飞控。

通用512的RequestMessageInfo由Vehicle持有shared context，ACK与原始回包两端终结前保原lane。原500ms响应timer在queue空之前也核固定deadline，ACK后1秒回包窗口只缩原期限；静默、IN_PROGRESS或重试不使等待无限。原包入口冻结原请求归属，实际回调仍在P0原生围栏之后；消息0合法，不再凭0哨兵判无请求。可选callbackOwner仅是QObject寿命条件，不是角色／账号授权；InitialConnect与StandardModes实际传原owner，未传owner的旧borrowed-data调用者仍自行保证数据寿命。

本车默认component的任意511只在最终actual write前，以同UUID一次预留推进不回绕mutation generation，并全清498三report及接收窗；ledgerEpoch为UUID:十进制generation。writeDepth贯穿write与通知，临界内原包准入／getter不可当前。未发送拒绝不冒称已配置；已可能write的代次／requestIssued不回滚，耗尽保守停止而非重用。498观察15／Registry review21／comparisons6保持，后续只读重采会看到旧基线失效，既有值快照不自动变成实时通知，须新被动资料与新完整5秒窗，不等于实际生效。

[18路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[42项全未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498：被动原包账本与实际Registry／Guided消费者

源码5份：Vehicle.h/.cc、FleetRegistry.h/.cc和custom实际GuidedActionsController.qml。原244报告与三条可选流的5秒本地窗口分开，实际绑定Vehicle的一次只读审阅附到现有五秒策略快照；没有新消息发送／observer槽／信号／配置UI或权限。

Registry仅从现有friend建立的实际Vehicle绑定读取一次原生快照，前后核记录与绑定，不以metadata／护照／调用者Map替代。null／0仅无提案，正间隔须int32整数且精确REAL32；三条当前新鲜正基线都已知、提案间隔均不小于原报告间隔，才给数值不增频结论，仍snapshotOnly且所有许可false。

[18路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[36项全未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497：现有511结果归属、重入及来源前置

源码6份：MAVLinkStreamConfig、Vehicle、GroundStation PX4FirmwarePlugin各h/cc。requestId／单pending／pump64和生命周期尾核进入原PID真实消费者；ESC只自己的STATUS正常结果续INFO，公共511不再无差别推进任一链，补源QQ／weak／epoch／UUID及超时隔离。

[18路径逐项清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[36项全未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。没有新文件／UI／权限或Fleet apply；成功ACK无messageId回显，不能由本地归属声称远端因果或实际频率已应用。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496：同次起点参考与完整垂直编码体

源码只改ContactModel／WaypointSwitchReview各h/cc，共4份。新增同次TakeoffReference，保实际22动作身份、原from包围及rest±tol；Waypoint full current整体落原窗口后用两完整端点盒建vertical。仅该support指定地形完整交集可用原finitepad，旧名义组件／LAND／NAV计数、原预算和七false不改，模型v5。

[19路径逐项清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[36项未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。没有新文件、登记、UI、PR或包。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495：起飞参考top到首NAV的完整声明连接

本批源码仅 `FleetMissionWaypointSwitchReview.h/.cc`：有TAKEOFF才检查MIS实际REAL32、原编码／Joint对应，重算完整预测top上下界；每个首NAV帽分量追加完整XY凸包×Z连接，沿原body/support外扩、五类环境和预算，不带接地豁免。NAV／LAND旧域与NAV计数原义保持。

模型名升级v4，原返回3／report31／schema1和Worker7保持；旧v3不能被新matcher接受，所有真实轨迹／许可false不变。没有新文件、QObject、UI、依赖或登记。[16路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[36项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494：保护缓存报告的文件闭环

- `FleetMissionExecutionReport.h/.cc`：新增独立report8完整验证、已知JSON整数槽恢复、原生文本保存及历史重开。旧native严格Int、full9文件／导出及handoff接受域不变。
- 实际custom `GuidedActionsController.qml`：共享纯值busy围栏，两新窄函数只转文本／固定身份及确认，不发布全局新报告、不恢复权限。
- 实际custom `FleetMissionExecutionReportPanel.qml`：独立历史来源、选择文件后二次确认／取消、同snapshot与代次绑定，导入不被目录冒认。

本批4源码／UI＋14文档=18既有路径，无新QObject、依赖或工程资源登记。[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[36项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前准确范围：6源码／UI、14文档，共20路径

Reader h/.cc增加独立search8／result34／row10及完整来源、逐文件记录和预算校验；Archive h/.cc增加独立public search36、closed intent8和同hash首页打开，保旧目录／page request11／action6／options7／receipt11。custom实际Guided facade及Panel两QML接显式首批／续批／选中／同摘要打开，不改默认src副本或资源登记。

见[20路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。无新文件或依赖，不把累计脏树视为本批改动量。UI/UX影响限明确筛选／状态／确认与原主题控件，未渲染；完整动态轨迹／最终许可与其余原目标不因此完成。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前准确范围：7源码、15文档，共22路径

Vehicle.h/.cc仅执行观察lane的私有代次／原source保留及live回调尾核；ExecutionObservation.h/.cc覆盖source探测、owned组快照、原队列重定位、变更代次／耗尽、注册更新及退休清理；Txn.cc固定同observer并在ready回调后重核原command context，也补观察关闭上下文的外调尾核。Orchestrator.h/.cc增加私有完整TaskRecord比较和四处Evidence调用者QQ／重定位及注册连接的本地暂存、复取同证据后发布，不改公开任务协议。

[22路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)以本节为当前批次。原实际write后commit、历史候选和TaskRecord最大修订闭合规则保持；没有把动态净空／真实任务结果或来源认证改为已完成。无新文件、QObject、工程／资源登记、依赖或UI，不把累计脏树视为本轮新增。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前准确范围：4源码、15文档，共19路径

ContactModel.h/.cc新增纯原生LandingReference及Result.landingReferences，材料仅来自原成功应用的LAND support，原components仍每段两份。WaypointSwitchReview.h/.cc扩展实际LAND参考几何、有限接地交集检查与模型v3；旧NAV计数、报告31键及Worker结果7键保持，不新增报告槽或预算。

[19路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)以本节为当前批次，不把累计脏树／未跟踪文件算作新增文件。Joint／Worker源码未改，既有真实调用和严格matcher消费本次模型；planning没有PM29来源，不获得接受域审阅。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前准确范围：8源码、16文档，共24路径

Geometry.h/.cc增加coveredByIndexedPrisms；DomainReview.cc与WaypointSwitchReview.cc各接coverage／corridor／terrainHorizontal三项。MemorySeal.h/.cc的capture新增实际PM、原PRIMARY控制块和原29快照必填参数；StartupTransaction.h存Target.primaryOwner，.cc在初次来源固定时核原29并更新唯一capture调用。没有保留绕过新原材料的旧capture重载。

快照为schemaVersion=1、6键、12＋2＋14＋1行；不是schemaVersion=6。完整semantic content／Fact／manager epoch／类型／来源／pending／年龄尾核不等于每个同值新receipt都作废。原事务／phase期限不因新回执重置。覆盖原模型与报告schema不变，workUnits及资源Unknown分布可以改变。

[24路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)以本节为当前批次，非累计脏树统计。无新文件、QObject、工程／资源登记、UI或依赖。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前准确范围：8源码／UI、15文档，共23路径

Orchestrator和SwarmController各.h/.cc新增轻目录、原生私有单条捕获与窄只读调用；ExecutionReport .h/.cc新增独立static构建器；Guided与现ExecutionReportPanel接入明确选择／确认／单份冻结及人工复制。仅新增模块说明 `FleetProtectedRunReport.md`，没有新源码文件、QObject、工程／资源登记或依赖。

目录6／行13、请求4、原记录16、内部返回3、独立报告8／摘要13／原claims22、对外返回4含原生jsonText。原compact20／销毁21、成员25与候选26／20完整保留，不伪装旧full current报告；旧报告／文件／交接合同不放宽。

[23路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)以本节为当前批次；不是累计脏树统计。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前准确范围：2源码、13文档，共15路径

仅修改 `FleetMissionExecutionObservation.cc` 与 `FleetMissionWaypointSwitchReview.cc`；不新增头文件、类、QObject、UI、工程／资源登记或依赖。前者结束已知任务序号回退后的关联，后者把相邻NAV连接改为有界整数XY凸包棱柱并让原覆盖／地形／障碍消费者完整使用它。

没有扩大旧公开shape：运行单机26／整组20、几何返回3／report31继续原合同；几何模型标识升v2，旧v1不冒充新模型。所有完成认证／实际路径／执行与飞行许可边界保持，不新增控制或占用释放。

[完整15路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项未执行场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)以本节为当前批次，不将累计脏树当本批diff。仅静态阅读／文档和普通文本核对；没有编译、运行代码／模型／helper、测试、仿真、改PX4、硬件、刷机、放行、提交、更新PR或打包。下方为历史。

## 487 当前准确范围：6源码／UI、15文档，共21路径

新增一个观察卷宗模块说明，无新C++／QML文件、资源登记或依赖。新链不调用旧build，旧view/export/save/load不直接接纳独立dossier；旧build的通用Map合同保持；Archive一次调用后从同dossier取31，双缓存共同最新页门禁和一次通知；Root只添readonly属性，现有Panel原子采用两份对应资料并显示四视角。

完整dossier21含sourceIdentity6／coverage8／原31／≤16个资料质量发现、4×view6／section3、确定性内容hash与未验证边界。四视角真实消费本页计数和缺口，不只是静态模板；不按机号／文件采集run／时间匹配任务，不产生事故或风险通过结论。

[34项工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；本批审计已记录三路独立交叉静读、15文档终审和最终普通文本核对；旧build拒绝范围的文案已缩窄，不是运行验收。[准确路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)不是累计脏树计数。原25目标及六类实质剩余保持；整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批路径或视图数上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。 本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前准确范围：11源码／UI／登记、14文档，共25路径

新增MotionReview.h/.cc、TelemetryHistoryPanel.qml及两模块md共5文件。custom.pri登记新C++源／头，qrc／qmldir登记新组件；两个实际宿主分别有入口。Archive新固定本地只读动作、位置双确认／当前实际完成页消费及Root窄facade形成真实纵切，不是未调用helper。

输入exact6、options7、receipt11，原生历史page36／row29只由纯helper消费；公开report31／group5／segment16不含raw、绝对坐标、UTC或epoch。新query／输入确认失效、通知返回后来源尾核和异步同pending CAS已补。32工程场景全部未执行；本批审计已记录三路独立交叉静读、14文档终审与最终普通文本核对；确认来源及通知收尾问题、历史限定文案均已修正，不是运行验收。准确路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

原25目标及六类实质剩余保持；整体约40%／剩余60%仍仅源码工作量粗估，不按本批文件或入口上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。 仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前：9源码／UI／资源、19文档，共28路径

新增 `FleetPerformancePolicyEditor.qml/.md` 两文件；qgroundcontrol.qrc／qmldir各登记一次。其它编辑为Draft set/clear与schema5、Compiler source重建／显式false保留、PlanStore presence＋Bool绑定，以及Root原源CAS facade、集中版本门和Planner pending互斥。准确路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不是累计脏树统计。

旧1..4／未声明输出保持；true／false显式存在才是schema5。严格true仍compiler0.8.0；合法区域来源明确false时plan保false且仍0.7.0；无source explicit false旧任务不改输出shape。来源与计划不允许删／改／偷加政策。UI/UX影响三态／默认不升级、明确确认、原source/revision、隐藏输入及焦点，未运行技能搜索脚本或界面。

[32项交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；本批审计已记录三路独立交叉静读、19文档终审及最终普通文本核对；仅一处新模块说明末尾多余空行已修，不是运行验收。原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。

本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前：12源码／UI、19文档，共31路径

实际竖切为Registry原生preview／一次CAS、Review专用请求与Session独立用途、原Root值桥及面板第五操作，再到独立摘要／历史筛选。全部编辑已有文件，无新文件、依赖、工程登记、QML类型或MAVLink变化。准确路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不是累计脏树统计。

旧四操作、current30／result16／收据5／approval14、旧preview2/18保持；仅性能操作使用preview3/20，专用 `fleet.performance.declaration.approve` 不与旧凭据互换。两数及完整caps一次提交；基线观察／批准不更新。新数据供483真实严格消费者使用，但不证明实测性能或飞行许可。

[32项交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；本批审计已记录三路独立静读、两处修正及最终普通文本核对，不是运行验收。UI/UX用于明确单位／未知、错误与焦点、默认取消及两确认，未执行技能搜索脚本或界面验证。原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。

本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前：12源码、19文档，共31路径

新增纯值 `FleetCapabilityEvidence::currentDeclaredPerformance()`，不新增文件、依赖、登记或UI。复用原能力批准／构型身份，明确输出已绑定的工程数值而非真实性认证；严格策略在Matcher、RoleAllocator、Compiler、ReserveEnergyPlan、EnergyScheduler及Coordinator形成真实消费链，PlanStore保新策略标记。准确路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不是累计脏树统计。

既有 `requirePerformanceEvidence=true` 强化为当前声明＋完整基线；false／absent不升级旧规划。严格Compiler0.8.0、完整来源进入planId；原批准及identity/hash schema不改。Energy严格估算不靠metadata drain、untrusted direct或投影fallback通过；原生电池故障／鲜度仍独立阻断。

[30项交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；本批审计已记录三路独立静读及最终普通文本核对，不等于运行验收。无性能录入器／实测认证、PX4修改、编译、运行、测试、刷机、放行、新PR或安装包。原25目标、六类实质剩余和约40%完成／60%剩余粗估保持。下方为历史。

## 482 当前：19源码／登记／UI、22文档，共41路径

新增 `FleetMissionWaypointSwitchReview.h/.cc/.md`，源／头登记于既有 `custom/custom.pri`；真实Worker计算静态NAV接受区及邻接连接域的环境比较。其余改动连通PM原29尾核、Evidence/schema5、Readiness原快照、两Review／Witness／两Authority和部署29项说明，准确逐路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不统计整个脏树。

旧12hash、公开19键、旧PM12／14／28合同及纯动作14输入不扩容；没有PX4、依赖、MAVLink、QML类型或命令权限新增。新报告result3／report31进入Worker/result7及实际风险和准入，但actualPathCertified=false仍阻断；不宣称完整路径、动力学或StartPermit完成。

[32项交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，审计已记录直接静读及最终普通文本核对。未编译、执行helper／模型／业务／QML、测试、仿真、操作硬件、刷机、放行、提交、更新PR或打包；无本批安装包。原25目标、六类实质剩余和整体约40%粗估保持。下方为历史。

## 481：16源码／登记、23文档，共39路径

新增StartupIntegerDomain.h／cc／md；一个无QObject原生不可变类，仅custom.pri登记源／头。更新Motion、Contact精确整数API、Cover／DomainReview／Worker、Profile及原编辑器。无新资源、依赖、MAVLink命令或PX4改动。[准确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)只统计本批，不计整个脏树。

新模型显式选择及同对象运行门／环境外包是真实源码，报告包含不等于许可。[32项工程场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；无构建／测试产物、新PR或安装包。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480：9源码／登记、18文档，共27路径

新增ActionPhaseReview.h／cc／md三个文件，一个无QObject纯值类；custom.pri登记源／头，无新依赖或资源。其余修改PM固定28观察及独立三集合尾核、Refresh固定28、Evidence/schema4、Readiness原快照消费者及部署面板两处确认文案。[逐路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)仅列本轮，不统计整个脏树。

[30项未执行工程场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)已同步。没有编译／测试产物、PR更新或新安装包。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：9源码／登记、18文档，共27路径

新增RateOverrideReview.h／cc／md三个文件，一个无QObject纯值类；只在既有custom.pri登记SOURCES／HEADERS，无新依赖／QML类型／资源；部署面板只同步刷新范围说明和勾选文字。其余修改PM固定观察及原14尾核、Refresh固定14、VehicleEvidence和Readiness真实消费者。[逐路径范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)不统计整个脏树或按文件计功能完成。

[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[26项未执行工程场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)已同步。没有编译／测试产物、新PR或安装包。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478：10源码、21文档，共31路径

没有新增文件、依赖或工程／资源登记。更新Planner三接口、Compiler／PlanStore完整策略保存与源对应、Builder严格当前口／独立旧档重现口，以及single／group新provenance、历史来源互核和Readiness完整策略重建。[逐文件范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)只计本批31路径，不统计整个脏树。

[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师待执行清单](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。没有编译／测试产物、新PR或安装包。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477：8源码、15文档，共23路径

没有新增文件或外部依赖。更新[区域算法](custom/src/Swarm/FleetAreaCoveragePlanner.md)、[source4草稿](custom/src/Swarm/FleetMissionDraft.md)、[编译消费说明](custom/src/Swarm/FleetMissionCompiler.md)与[双字段编辑入口](custom/src/Swarm/FleetCoverageExclusionsEditor.md)。原QtGui已在工程登记，Qt路径布尔仅生成搜索候选，原域完整距离检查独立负责名义约束。

[逐文件范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)不统计整个脏树或按文件算功能；[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)均待工程师运行验证。没有编译／测试产物、PR或安装包。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476：11源码／资源、15文档，共26路径

实际新增一个排除区坐标编辑QML及其模块说明，其余为既有路径增量。核心是[真实区域算法](custom/src/Swarm/FleetAreaCoveragePlanner.md)、[草稿来源](custom/src/Swarm/FleetMissionDraft.md)和Compiler最终全路线消费，而不是仅增加声明字段。原无孔任务保持旧planner版本，带孔规划为0.3.0。

[逐文件清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)保留历史范围，不把整个脏树算成本批；[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)中的运行场景均未执行。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475：15源码、18文档，共33路径

新增GroupRemainderPlanner.h／cc／md及GroupRemainderPanel.qml／md五文件；修改Compiler／PlanStore／ReviewCoordinator／RemainderView的实际消费者，Root和两种编辑区／共享Planner及工程资源登记。完整列表见[本批清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

联合来源与原区域／单成员来源互斥，每原成员同序一次，新目标及备用不冲突；不静默排除未提交成员。原Run、占用、PX4和StartPermit不改，未生成工具／构建／测试产物或新PR／安装包。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474：6源码、16文档，共22路径

实际范围是[StartupTransaction](custom/src/Swarm/FleetMissionStartupTransaction.md)整组原生取源、[Archive](custom/src/Swarm/FleetMissionRemainderArchive.md)双版本全包校验与成员目录、[资料面板](custom/src/Swarm/FleetMissionRemainderArchivePanel.md)及Root实际选择链。没有新文件／工程登记／构建产物；未修改PX4、执行许可、原Run注册或占用逻辑。

完整逐路径列表见[本批清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本批不计为任一原25整目标完成。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：完整历史重规划来源，准确30路径

11源码／登记资源：RemainderPlanner h／cc抽共用完整源校验及历史provenance；新增RemainderArchive h／cc；custom.pri／CustomPlugin登记；Root私有资料槽、文件／历史capture接口和精确退出；原RemainderPanel接历史输入；新增ArchivePanel.qml；qrc／qmldir登记。19文档逐路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。新文件为Archive.h／cc／md和ArchivePanel.qml／md五份。

保存完整原政策副本／计划／单成员载荷映射及原观察，不改原Run、报告或占用格式，不从latest／summary补缺项。打开后只本页历史预览，选用再独立确认；schema2／包身份保到新任务审阅与归档。文件已提交和后置读回／当前性分开，输入文字不被旧包自动填入。

静审补齐收起／隐藏及退出审阅后的后台历史capture清理；只清本提案的精确关联，不清新来源或删除归档。无工具、测试、构建产物，未改PX4或执行模型许可。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：编辑态单槽保存恢复，准确28路径

11源码／登记资源：Draft h／cc 新 editing validator／loader；新增 Store h／cc；custom.pri／CustomPlugin 注册；Root 新 saved 来源及同旧复制流程；Planner 挂新 SavedPanel；新增 SavedPanel.qml；qrc／qmldir 登记。17文档同步，逐路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本次新增 Store.h／cc／md 与 SavedPanel.qml／md 五文件，其余为本批既有路径。

旧严格可构建源规则不改；新编辑源完整验证所有保留字段。Root 补净空资料内层2／contacts预检；saved恢复用新loader并重置本地revision／基准确认，沿原审批撤销与共享备用。UI按下／busy后核原请求、返回按独立上下文epoch显示结果，已提交事实不被后续换站抹去。

无新工具、测试或构建产物，不改Compiler、Vehicle或PX4。单槽不是自动保存未提交文字、多版本备份、跨站同步或运行任务恢复。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：原模型真实持有链与输入侧深冻结

13份既有源码涉及GeographicMotion h／cc、Worker h／cc、DomainReview h／cc、普通／地面Review各h／cc、Witness h／cc及Txn.cc。新增Models和不重算Qt的matchesSource；实际计算分支必传原Ptr；两个Review保完整原集合与finished／重入同一性；Witness const保存原集合；Txn有资料不再prepare替代对象。

同步18份文档，无新增文件、工程／资源登记、QML、工具、测试或构建产物，准确31路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。纯值快照／matcher仍可结构重建，但不代替实际原对象；数值／动作／执行许可缺口和全部原false保持。470索引本身不在本471改动范围。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：自持空间索引、完整候选与原行SAT消费

只改Geometry.h／cc和DomainReview.cc三份既有源码：新增copy-only PreparedPrismIndex、完整准备／候选／indexed SAT三个API，可取消稳定归并、整数Morton叶序和闭XYZ层级；实际消费者完整构建两个索引，成功枚举后才逐原行做真实SAT。不借外部可变凸柱数组，不用树节点号代替cellId／volumeId。

同步13份文档，准确16路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。无新文件、工程／资源登记、工具或测试文件；原Motion／ContactModel／Input／Compiler／Vehicle／QML／PX4不改。模型v2、报告28／Worker6、同组预算和numericfalse均保持，完整启动许可及其余原目标未完成。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469：原生准备凸柱与整域实际SAT接线

Geometry.h／cc新增Point2、私有PreparedPrism及prepareConvexPrism／axisAlignedBoxIntersectsPrism。DomainReview.cc在单一预算内一次准备完整terrain／obstacle，再将原逐对通用三维可行性计算替换为适用范围明确的精确SAT；三种并集覆盖仍用原solver。

只改3份源码并同步13文档，无新文件、工程登记、工具或测试文件，准确16路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。原Motion／ContactModel／ClearanceInput／Compiler／Vehicle／QML／PX4不改；模型名v2及报告形状不变，不能据新几何True恢复启动许可。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468：两份后端源码的外包与求解改进

只改DomainCover.cc和DomainReview.cc：完整纬度区间选k=6／4／3／2／1，整数ceil(B/k)经度半宽；报告算法名v2；不完整明细核真实处理前缀；地形覆盖删重复Z面并复用完整XY竖柱。API、result3／report28／limits8／Worker result6及预算不变，无新类／登记／资源／测试文件。

同步13份文档，准确15路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。原Motion／ContactModel／Geometry／Compiler／Vehicle／QML／PX4均无本批修改，原实际接纳仍要求未成立的数值包含，不以优化扩大许可。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467：完整理想域计算、消费者升级及接触豁免修正

新增DomainCover／DomainReview两组h／cc／md，并登记custom.pri。ClearanceReview抽共享geometrySourceValid，不复制Joint解释器、不重跑旧环境solver；Worker原input8／schema2／source12保持，result从5升级为严格6，新增startupDomainReview。两Review、Witness和两Authority同步消费；失败清空Joint及两份报告。

整组外包准备后才开始共享预算的真实环境比较，原Qt数值包含固定未证。交叉审阅后删除新整域层对vehicle／terrain的pad聚合及豁免，保留旧名义层的动作限定接触规则。没有修改GeographicMotion、ContactModel、Compiler、Vehicle、QML或PX4。

准确14源码／登记、20文档共34路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。没有更新PR、源码包或安装包。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466：备份诊断来源接入真实历史导入链

原Worker增加import5／result38／current27；Store从原生诊断readable行建立本批实际消费的私有来源，新增三种import intent7、纯值preview29及原固定目标尾核；Guided仅增加诊断ID字段，面板接选择／预览／独立确认与原actualfinished。

复用现有类、资源及线程槽，无新Inventory实现、工程登记、恢复工具或测试文件。准确6源码、14文档共20路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。未提交、更新PR或打包。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465：逐文件候选诊断，保留旧严格目录语义

复用原Worker、Store、Guided窄门面及维护面板，新增backup-diagnostics请求2／结果23、只读public26和row4／5；可读项携原backup8，拒收项descriptor为空并给固定原因。不增加资源或工程登记，不修改原目录Inventory类、原strict catalog21／24或移动／复制契约。
准确6源码、14文档共20路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。仅补源码及静态交接，没有更新PR、源码包或安装包。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：独立备份目录→原生检查→实际完成→只读展示

原目录清点追加EventBackups命名空间；原维护Worker新增backup-catalog请求／结果，Store新增只读maintenanceBackupCatalog，Guided与维护UI消费同一原生结果。旧录制／历史、移动和复制契约不变，不新建数据库或重复后台槽。

准确8源码、15文档共23路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本批不执行盘点、读回、恢复、SQL或应用，不更新PR、源码包或安装包。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：原维护链增加显式备份，不新建平行归档服务

只扩原Worker／Store／维护面板与Guided窄入口，复用已登记类和组件；不增加新的数据库、任意源路径、飞行接口或资源注册。旧inventory、retire、restore契约保持；prepare-backup才带明确目标目录，新备份预览与复制事实采用独立形状，commit只用原私有预览。

本批准确7源码、15文档共22路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。原历史折叠入口补“备份”文字，不改查询逻辑；没有对本机归档执行复制、读回、SQL、扫描或文件清理，未更新PR／源码包／安装包。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：维护库存→原生预览→单段退役／恢复

本批新增后台维护Worker及面板，原目录清点新增独立retired namespace，Store负责固定目录／原生身份和一次性预览、实际线程槽；Guided窄门面及已有历史／任务历史面板接互斥状态。新Worker登记custom.pri，组件登记原qrc／qmldir。完整路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

只写源码，不对本机归档执行任何读取／移动／删除。新代码没有永久删除、覆盖、自动回滚／重试或当前录制库改名入口；退役保留数据且不回收磁盘。旧历史副本仍可只读，但movement尝试退休原生游标与批准性上下文，不能借旧目录复活。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：实际留存任务目录，不是运营结果认证

实际改动分为Reader任务哈希双版本筛选、Store原生有界累计／独立原记录查询、Guided窄facade、已有历史面板嵌入新任务诊断组件及资源登记。精确文件范围与工程师待验证项见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

既有search1保持，task筛选不从自由输入路径／SQL／任意原生cursor构造，不给历史恢复或飞行入口。目录计数是保留诊断记录，不代表真实任务次数、成功或完整历史；本批不更新PR、源码包或安装包。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：原生电池故障／状态与实际健康、能量和尾栅

准确11源码／工程登记、19文档共30路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不是全脏树增删或验收计数。新增共享检查器h／cc／md；修改GroundStation Vehicle、原phase seal、Registry、HealthCenter、EnergyScheduler、Swarm发现、Guided审阅身份并登记custom.pri。不是修改PX4飞控。

Schema2仍outer20、逐行19，schema1原15行字段兼容。明确fault／不安全charge进入拒绝账单和原blocking消费；过期负面保unknown，FTC不能覆盖。合法负面原evidence保留，坏型不发布成功前缀；发送前原整组seal覆盖新账本变化。源码与未执行验证边界见[工程师说明](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：原审阅模型进入事务私有缓存及实际阶段消费者

准确5源码、16文档共21路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)：Witness h／cc、StartupTransaction h／cc、StartupTransition cc及配套文档；不含Vehicle、飞控、工程登记变更或全脏树差异。

Witness逐原source12准备完整同源模型并按ID保留，native getter先原memoryCurrent。Txn一次准备与完整来源配对后保存原Ptr，Transition只从实际owner取已绑定缓存，无fallback prepare。没有新DTO／策略schema、QML恢复、默认值或TTL；原权限、原始账本及整组seal继续独立消费。详见[工程师说明](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：实际地理运动规则共享及原审阅来源冻结

准确8源码／工程登记、18文档共26路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不是整个脏工作树差异。新增[GeographicMotion](custom/src/Swarm/FleetMissionStartupGeographicMotion.md)三文件；实际Transition改一次准备／重复判断，Worker及两类Review固定完整同源描述并登记custom.pri。

普通Readiness::capture不输出startupTransitionPolicy，已改为提前复用原冻结member的规范化reviewedProfile，让模型与原审阅行共用reviewedPolicy；地面仍用原completed READ currentRisk。Worker全量核policy／ceiling／原GPI几何及完整payloadHash，不采用可用前缀、可选诊断或默认模型。源描述的numericContainmentProven／clearanceCertified／flightCommandReleased全部false；不是完整运动域检查完成。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：普通RAW整栈锁存与阶段整组memory seal

准确范围为10源码／工程登记和17文档共27路径，见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。新增原生Seal三文件及RAW说明，修改Vehicle和StartupTransaction实际消费者；ExecutionProfile新增原共享纯政策校验入口，并登记qmake／CMake。整个脏工作树和历史修改不是本批范围。

原step33及公开DTO不变，无新命令／UI／权限；原scope事件、原阶段判定和原材料不能被重捕替换。原Compiler pending、整个运动域净空包含、最终StartPermit及正式结果仍未完成。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 456：共享原生Witness到真实命令末写栅

本批新增Witness h／cc／md并登记，修改两类Review、两类Authority及StartupTransaction的h／cc与配套说明，准确13源码／登记＋19文档共32路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。既有Vehicle末写切点通过实际Txn消费，不把整个脏工作树或历史改动计为本批。

保同一原生指针、原报告及原始Review单调时钟；移交无新TTL，来源／UTC／期限最终全组memory核验。不是完整运动连续性或净空放行，Compiler pending、StartPermit及正式结果仍未完成。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：同次启动净空报告的后台实际审阅接线

本批准确范围11源码／登记、18文档共29路径。新建StartupClearanceWorker三文件，其余既有文件增量，不将整个累计dirty diff当本批改动。

源码／登记：

- [custom/src/Swarm/FleetMissionJointSpatialReview.h](custom/src/Swarm/FleetMissionJointSpatialReview.h)
- [custom/src/Swarm/FleetMissionJointSpatialReview.cc](custom/src/Swarm/FleetMissionJointSpatialReview.cc)
- [custom/src/Swarm/FleetMissionClearanceReview.h](custom/src/Swarm/FleetMissionClearanceReview.h)
- [custom/src/Swarm/FleetMissionClearanceReview.cc](custom/src/Swarm/FleetMissionClearanceReview.cc)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.h](custom/src/Swarm/FleetMissionStartupClearanceWorker.h)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.cc](custom/src/Swarm/FleetMissionStartupClearanceWorker.cc)
- [custom/src/Swarm/FleetMissionStartReview.h](custom/src/Swarm/FleetMissionStartReview.h)
- [custom/src/Swarm/FleetMissionStartReview.cc](custom/src/Swarm/FleetMissionStartReview.cc)
- [custom/src/Swarm/FleetMissionArmedGroundReview.h](custom/src/Swarm/FleetMissionArmedGroundReview.h)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [custom/custom.pri](custom/custom.pri)

文档：

- [README.md](README.md)
- [MODIFIED_FILES.md](MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](docs/INDEX.md)
- [docs/handoff/README.md](docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionJointSpatialReview.md](custom/src/Swarm/FleetMissionJointSpatialReview.md)
- [custom/src/Swarm/FleetMissionClearanceReview.md](custom/src/Swarm/FleetMissionClearanceReview.md)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.md](custom/src/Swarm/FleetMissionStartupClearanceWorker.md)
- [custom/src/Swarm/FleetMissionStartReview.md](custom/src/Swarm/FleetMissionStartReview.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](custom/src/Swarm/FleetMissionArmedGroundReview.md)
- [custom/src/Swarm/FleetMissionClearanceSource.md](custom/src/Swarm/FleetMissionClearanceSource.md)
- [custom/src/Swarm/FleetMissionClearanceDatum.md](custom/src/Swarm/FleetMissionClearanceDatum.md)

主要改动：独立纯值可取消worker、完整Joint启动源提取／共用报告匹配、原普通／地面审阅实际接线、首风险和v4样本不重基、原期限及占用排空。完整报告必需prehash，未知／失败／违规不能成为可批准状态。两Authority到最后write和运动包络仍待开发，原pending保持；见[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：显式工程高程关系与实际审阅来源

准确范围11源码／工程登记、17文档，共28路径；是本批局部范围，不是累计dirty diff或功能验收计数。新建Datum h／cc／md，其余均既有文件增量，没有二进制、PR或安装包。

源码／登记：

- [custom/src/Swarm/FleetFirmwareAdapterCatalog.h](custom/src/Swarm/FleetFirmwareAdapterCatalog.h)
- [custom/src/Swarm/FleetFirmwareAdapterCatalog.cc](custom/src/Swarm/FleetFirmwareAdapterCatalog.cc)
- [custom/src/Swarm/FleetMissionClearanceDatum.h](custom/src/Swarm/FleetMissionClearanceDatum.h)
- [custom/src/Swarm/FleetMissionClearanceDatum.cc](custom/src/Swarm/FleetMissionClearanceDatum.cc)
- [custom/src/Swarm/FleetMissionStartReview.h](custom/src/Swarm/FleetMissionStartReview.h)
- [custom/src/Swarm/FleetMissionStartReview.cc](custom/src/Swarm/FleetMissionStartReview.cc)
- [custom/src/Swarm/FleetMissionArmedGroundReview.h](custom/src/Swarm/FleetMissionArmedGroundReview.h)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [src/Vehicle/Vehicle.h](src/Vehicle/Vehicle.h)
- [src/Vehicle/Vehicle.cc](src/Vehicle/Vehicle.cc)
- [custom/custom.pri](custom/custom.pri)

文档：

- [README.md](README.md)
- [MODIFIED_FILES.md](MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](docs/INDEX.md)
- [docs/handoff/README.md](docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetFirmwareAdapterCatalog.md](custom/src/Swarm/FleetFirmwareAdapterCatalog.md)
- [custom/src/Swarm/FleetMissionClearanceDatum.md](custom/src/Swarm/FleetMissionClearanceDatum.md)
- [custom/src/Swarm/FleetMissionStartReview.md](custom/src/Swarm/FleetMissionStartReview.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](custom/src/Swarm/FleetMissionArmedGroundReview.md)
- [custom/src/Swarm/FleetMissionClearanceSource.md](custom/src/Swarm/FleetMissionClearanceSource.md)
- [custom/src/Swarm/FleetFirmwareCatalogReload.md](custom/src/Swarm/FleetFirmwareCatalogReload.md)

目录descriptor2保存有场景／配置／原点绑定的显式工程同datum零offset关系，原outer1／descriptor1保持。新原生Datum实际进入两Review prehash和整组尾核；Catalog漏接native receiptSequence的形状错配修复，动态收据不污染稳定指纹。新增Vehicle纯内存版本末核／非拥有PRIMARY读取，消除Datum的weak.lock；跨层原生UTC下界／最终观察输出补齐两Review水位，资料和原审阅期限不续期。

这不完成实际启动solver／两Authority至最后write／StartPermit或物理基准认证，原pending保留；原25目标和六类实质剩余保持。见[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)及[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 以下453为历史。

## 453：实际配置来源与资料有效期的原生审阅接线

准确范围：15源码／工程登记、16文档，共31路径；是本批局部范围，不是累计dirty diff、31个功能或验收计数。新建两组h／cc／md，其余均为既有文件增量；没有新二进制、PR或安装包。

源码／登记：

- [custom/src/Swarm/FleetRegistry.h](custom/src/Swarm/FleetRegistry.h)
- [custom/src/Swarm/FleetRegistry.cc](custom/src/Swarm/FleetRegistry.cc)
- [custom/src/Swarm/SwarmController.cc](custom/src/Swarm/SwarmController.cc)
- [custom/src/Swarm/FleetNativeConfigurationObservation.h](custom/src/Swarm/FleetNativeConfigurationObservation.h)
- [custom/src/Swarm/FleetNativeConfigurationObservation.cc](custom/src/Swarm/FleetNativeConfigurationObservation.cc)
- [custom/src/Swarm/FleetMissionClearanceSource.h](custom/src/Swarm/FleetMissionClearanceSource.h)
- [custom/src/Swarm/FleetMissionClearanceSource.cc](custom/src/Swarm/FleetMissionClearanceSource.cc)
- [custom/src/Swarm/FleetMissionStartReview.h](custom/src/Swarm/FleetMissionStartReview.h)
- [custom/src/Swarm/FleetMissionStartReview.cc](custom/src/Swarm/FleetMissionStartReview.cc)
- [custom/src/Swarm/FleetMissionArmedGroundReview.h](custom/src/Swarm/FleetMissionArmedGroundReview.h)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [custom/custom.pri](custom/custom.pri)
- [src/Vehicle/Vehicle.h](src/Vehicle/Vehicle.h)
- [src/Vehicle/Vehicle.cc](src/Vehicle/Vehicle.cc)
- [qgroundcontrol.pro](qgroundcontrol.pro)

文档：

- [README.md](README.md)
- [MODIFIED_FILES.md](MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](docs/INDEX.md)
- [docs/handoff/README.md](docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetRegistry.md](custom/src/Swarm/FleetRegistry.md)
- [custom/src/Swarm/FleetNativeConfigurationObservation.md](custom/src/Swarm/FleetNativeConfigurationObservation.md)
- [custom/src/Swarm/FleetMissionClearanceSource.md](custom/src/Swarm/FleetMissionClearanceSource.md)
- [custom/src/Swarm/FleetMissionStartReview.md](custom/src/Swarm/FleetMissionStartReview.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](custom/src/Swarm/FleetMissionArmedGroundReview.md)

共享配置提取与Swarm实际提交使用同一字段；Registry保存独立实际来源与本地完整批准基线，新Vehicle epoch只读接口不借原点DTO。Source严格绑定原完整输入／成员及本机UTC窗口，两Review冻结同源依据，原单调期限只缩短。固定证据进入原artifact摘要，不将资料hash升级为物理／高程认证。

原Compiler pending保持；显式datum对应、同次实际启动异步报告、两Authority到最后write、完整StartPermit及正式结果仍须开发，其余六类和原25目标不缩减。限定核对见[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)，工程师待执行见[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、运行、测试、改PX4、刷机、放行、提交、更新PR或打包；以下452为历史。

## 452：输入2、精确非对称扫掠与接地报告3

准确范围：9源码／工程登记文件、18文档，共27路径，不是累计dirty diff或27项目标完成。新ContactModel h／cc／md，其余为既有文件增量。

源码／登记：

- [custom/src/Swarm/FleetMissionClearanceInput.h](custom/src/Swarm/FleetMissionClearanceInput.h)
- [custom/src/Swarm/FleetMissionClearanceInput.cc](custom/src/Swarm/FleetMissionClearanceInput.cc)
- [custom/src/Swarm/FleetMissionClearanceGeometry.h](custom/src/Swarm/FleetMissionClearanceGeometry.h)
- [custom/src/Swarm/FleetMissionClearanceGeometry.cc](custom/src/Swarm/FleetMissionClearanceGeometry.cc)
- [custom/src/Swarm/FleetMissionContactModel.h](custom/src/Swarm/FleetMissionContactModel.h)
- [custom/src/Swarm/FleetMissionContactModel.cc](custom/src/Swarm/FleetMissionContactModel.cc)
- [custom/src/Swarm/FleetMissionClearanceReview.h](custom/src/Swarm/FleetMissionClearanceReview.h)
- [custom/src/Swarm/FleetMissionClearanceReview.cc](custom/src/Swarm/FleetMissionClearanceReview.cc)
- [custom/custom.pri](custom/custom.pri)

7份模块说明：[输入2](custom/src/Swarm/FleetMissionClearanceInput.md)、[非对称几何](custom/src/Swarm/FleetMissionClearanceGeometry.md)、[接触来源／组件](custom/src/Swarm/FleetMissionContactModel.md)、[报告3与匹配](custom/src/Swarm/FleetMissionClearanceReview.md)、[Compiler](custom/src/Swarm/FleetMissionCompiler.md)、[PlanStore](custom/src/Swarm/FleetMissionPlanStore.md)、[Draft版本分层](custom/src/Swarm/FleetMissionDraft.md)。再加11份既有共同状态／修改／路线图／索引／交接／审计文档，共18份，完整路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

实际planning consumer使用新版模型，旧input1/report2不补默认，input2/report3同源归档。只允许指定support／terrain整交集在有限pad band内，protected和障碍不豁免。原pending／真实来源／最终write等未完成，不是可飞行交付。仅源码与限定静态核对，未编译、运行、测试、改PX4、刷机、提交、更新PR或打包；下方451为上一批。

## 451：可取消后台规划与实际UI消费链

本批准确范围为15个源码／工程登记文件、17个文档，共32条路径，不是整个累计dirty diff，也不是32项目标完成。没有更新PR或安装包。

源码／登记：

- [custom/src/Swarm/FleetMissionClearanceGeometry.h](custom/src/Swarm/FleetMissionClearanceGeometry.h)
- [custom/src/Swarm/FleetMissionClearanceGeometry.cc](custom/src/Swarm/FleetMissionClearanceGeometry.cc)
- [custom/src/Swarm/FleetMissionClearanceReview.h](custom/src/Swarm/FleetMissionClearanceReview.h)
- [custom/src/Swarm/FleetMissionClearanceReview.cc](custom/src/Swarm/FleetMissionClearanceReview.cc)
- [custom/src/Swarm/FleetMissionCompiler.h](custom/src/Swarm/FleetMissionCompiler.h)
- [custom/src/Swarm/FleetMissionCompiler.cc](custom/src/Swarm/FleetMissionCompiler.cc)
- [custom/src/Swarm/FleetMissionPlanningWorker.h](custom/src/Swarm/FleetMissionPlanningWorker.h)
- [custom/src/Swarm/FleetMissionPlanningWorker.cc](custom/src/Swarm/FleetMissionPlanningWorker.cc)
- [custom/src/Swarm/FleetMissionReviewCoordinator.h](custom/src/Swarm/FleetMissionReviewCoordinator.h)
- [custom/src/Swarm/FleetMissionReviewCoordinator.cc](custom/src/Swarm/FleetMissionReviewCoordinator.cc)
- [custom/res/Merivus/GuidedActionsController.qml](custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetMissionPlannerPanel.qml](custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [custom/res/Merivus/MerivusAIAssistantPanel.qml](custom/res/Merivus/MerivusAIAssistantPanel.qml)
- [custom/res/Merivus/FleetMissionRemainderPanel.qml](custom/res/Merivus/FleetMissionRemainderPanel.qml)
- [custom/custom.pri](custom/custom.pri)

模块说明：

- [取消几何](custom/src/Swarm/FleetMissionClearanceGeometry.md)、[取消报告](custom/src/Swarm/FleetMissionClearanceReview.md)、[静态编译入口](custom/src/Swarm/FleetMissionCompiler.md)。
- [Worker生命周期](custom/src/Swarm/FleetMissionPlanningWorker.md)、[Coordinator／Root及三个consumer](custom/src/Swarm/FleetMissionReviewCoordinator.md)、[余段等待与取消](custom/src/Swarm/FleetMissionRemainderPanel.md)。
- 上述6份模块文档与README、本清单、PROJECT_STATUS、ROADMAP、TARGET_MATRIX、INDEX及handoff目录5份交接／审计文档，共17份。

重型净空编译走纯值worker；原生观察仍GUI，旧来源／取消不采用旧结果，完成回调逐入口核实际来源。输入保留、取消不发飞机命令；归档真实提交不冒称回滚。仅限定静态审阅，工程师实际验证未做；完整净空／启动／结果及原25目标仍继续。下方450为上一批。

## 450：明确范围的规划计算与原记录来源检查

本批触及9源码／17文档共26路径，无新文件、工程登记、UI／资源、QML类型或飞行入口。准确文件见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不以累计dirty diff统计本批。

Joint／StartupSpatial新增geometryBasis纯源入口，复用完整canonical编码提取；旧显示合同保持。ClearanceReview新增独立schema2报告及原规划来源匹配检查；Compiler真实调用求解并冻结planningClearanceReview，PlanStore严格核成功报告同源／失败诊断形状，Coordinator原样投影且继续实际阻断。

规划不包含startup不等于缺失被伪造补齐，独立basis也不是最终artifact指纹。归档不重跑旧空间checker或净空solver；内容匹配不是报告真实性或通过证明。原pending保持，接地与完整native启动准入仍待开发。原25目标不缩减，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方449及更早为历史。

## 449：整段扫掠与区域并集的真实计算源码

新增FleetMissionClearanceGeometry和FleetMissionClearanceReview的h／cc／md，custom.pri登记头／源；输入合同与联合空间文档补消费边界，共同状态、审计和交接同步。准确范围为5源码／工程＋15文档共20路径，详见[逐文件清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不使用累计dirty diff统计本批改动。

算法以整数平面表示整段扩张机体包络，使用精确有理顶点、严格补集和有界完整并集差集。报告覆盖场地、走廊、地形范围／最高地形与障碍；Unknown不当作无命中，起降接地缺口不当作模型满足。只计算明确声明的名义模型，不认证真实路径、传感器、配置或当前场地。

原几何pending阻断和所有审批／发送门槛不变。实际规划／启动消费者、接地上下文与最终许可继续开发，原25目标保持；本批无UI、飞控改动、编译、测试、运行、刷机、提交、PR或安装包更新。下方448及更早为历史。

## 448：工程场景和逐机包络的完整资料通路

新增FleetMissionClearanceInput h／cc／md并登记custom.pri；Draft新增CAS设置／清除资料，来源版本1／2严格配对11／12项settings；Compiler保原约束并验证实际活动主机；PlanStore独立核完整规范值与编辑源同资料；Guided两处历史／修复预检兼容明确新版本，不新增布局或自动操作。

本批8源码／工程、17文档，共25路径，逐项见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。坏声明不保存成功前缀，合法声明仍几何审阅pending，不能从校验成功推导净空或许可。[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)保留实际未执行项目。原25目标及完整solver／实际启动消费继续，不按文件／批次计完成率；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方447及更早为历史。

## 447：业务阻断从风险报告贯通实际原生门禁

本批6源码、16文档，共22路径，无新文件／类型／资源登记。RiskEvaluator新增native-only纯 `hasNoBlockingRisks`，并统一未命名严重信号的有效code；Coordinator用同一结果决定needsReview。Deployment核实际冻结评估与准确归档；普通／地面启动审阅在原记录绑定与hash后独立核原风险，不消费其它任务lastReview。

精确逐文件范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)保留未执行的正常空列表、合法阻断、坏型／缺项、旧记录与同源比较验证。不以累计diff或批次算完成率；原25目标、完整净空与最终许可等实质代码保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方446及更早为历史。

## 446：同一v4门槛的精确算术修正

新增FleetMissionAltitudeArithmetic的h／cc／md，登记custom.pri；FleetMissionStartupAltitude统一更换残差及原样本漂移比较，不改变API或20／20、六字段Readiness、monitor18及step33。原始derived差值不删，门槛直接组合原始三个高度／整数毫米参考。固定36个uint32 limb，输入／容量不支持明确拒绝，不引入库、eps或默认容差。

本批4源码／工程登记、15文档共19路径，逐文件见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)保留未执行的数学反例、类型／边界和实际链路验证；静态推导不冒充测试通过。原25目标与工程场地净空等实质工作继续；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方445及更早为历史。

## 445：显式高度关系政策及不可自行清除的启动监视

新增[纯高度关系helper](custom/src/Swarm/FleetMissionStartupAltitude.md)、明确startupTransitionPolicy v4、[策略表单](custom/src/Swarm/FleetStartupPolicyEditor.md)、普通／地面风险首次基准、真实Transaction／Vehicle scope监视与发送前核对。稳定reference与首次sample分开；当前复核仍对原sample检查容差，不以删除动态字段绕过风险。原v1–v3、step33及外层profile schema1保持。

本批精确触及范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不以累计dirty diff、文件或批次计完成率。scope监视不覆盖此前审阅等待期，不认证共同原点、内部valid、同采样、净空或最终StartPermit；原25目标继续。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方444及更早为历史。

## 444：原始高度观察与启动风险的诊断接线

本批11源码／工程登记、18文档，共29路径。新增FleetStartupAltitude的h／cc／md及VehicleStartupAltitude.md，登记qmake／Vehicle CMake；Vehicle新增被动native27，Readiness增加固定11字段诊断，普通／地面风险摘要接入，同步排除动态诊断对阶段／审批当前身份的影响。精确文件见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

这是冻结观察材料，不是可信共同原点、PX4内部valid、传感样本新鲜度、净空或StartPermit。首次完整artifact及其hash仍绑定原诊断；只在当前风险重捕比较时排除该独立诊断，不以新值覆盖原材料。原25目标保持；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方443及更早为历史。


## 443：原生原点观察、冻结参考和发送前核对

本批13源码／工程登记、18文档，共31路径。新增纯消息解码h／cc／md、Vehicle账本文档、Swarm纯参考helper的h／cc／md；修改实际Vehicle收包／发送、Readiness／Transition、终态再审上下文及地面READ物理投影，分别登记qmake、Vehicle CMake与custom.pri。精确文件见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

原点为事件型报告，年龄只诊断；从未观察保持兼容，但观察后损坏／丢源不降级成缺消息豁免。实际发送scope不逐步重采原点依据；报文、hash和账号均不认证共同原点、净空或最终许可。原25目标保持，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，不按累计diff／批次计完成率。下方442及更早为历史。


## 442：全任务请求链／启动／终结的联合名义空间报告

本批10源码、17文档，共27路径；新增[FleetMissionJointSpatialReview](custom/src/Swarm/FleetMissionJointSpatialReview.md)的h／cc／md，工程登记新helper，不新增QML组件或飞行入口。Checker增加纯静态完整段／区域检查，Root九字段capture冻结独立jointReview，原明细复用分页字段浏览。精确文件见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

这是本地名义几何重审，不是正式4D、权威空域、净空、StartPermit或机载结果。原25目标和剩余实质代码保持；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，不由累计diff／文件／批次计完成率。下方441及更早为历史。

## 441：原始空域输入的实际阻断及同源归档

本批7源码、15文档，共22路径。变更原[Checker](custom/src/Swarm/FleetAirspaceConflictChecker.md)、[Coordinator](custom/src/Swarm/FleetMissionReviewCoordinator.md)、[Intent](custom/src/Swarm/FleetIntentTask.md)的h／cc与Root QML；新增两份任务层／全链说明，不新建服务或资源登记。逐路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

坏输入、双源冲突及坏间距进入原生报告／风险／needsReview，归档取冻结原值，审批重核私有当前参数；同步通知不再让旧外层覆盖新状态。本批不是全路线联合审查或最终许可。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；不按批次／文件数计完成率，下方440及更早为历史。

## 440：原计划路线／返程／动作／协议项与归档空域

本批14源码／资源、18文档，共32路径；新建完整任务helper的h／cc／md和明细Panel的qml／md。实际源、API、全组编码和显示边界见[任务投影](custom/src/Swarm/FleetMissionSpatialReview.md)、[明细入口](custom/src/Swarm/FleetMissionSpatialPanel.md)及[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

新空域输入形状元数据随原reviewEvidence哈希归档，旧记录兼容但不补加载事实；本地协议生成不是上传／回读／执行证明，原空域报告不升级覆盖范围。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包，不按累计diff／批次估算完成率。下方439及更早为历史。

## 439：实际启动风险的空间捕获、明细与地图

本批13源码／资源、17文档，共30路径；新增纯值helper的h／cc／md和空间Panel的qml／md。精确范围见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，原生来源及两个明确接口见[工作台合同](custom/src/Swarm/FleetMissionStartupWorkbench.md)。不按累计diff或批次计完成率。

独立静态名义线段候选不改旧空域check()或原启动门禁，不增加飞行指令。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包；下方438及更早为历史。

## 438：结构化启动策略与实际草稿绑定

本批6源码／资源、14文档，合计20路径；新增策略QML组件及两份模块文档。精确路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，输入合同见[策略表单](custom/src/Swarm/FleetStartupPolicyEditor.md)和[父编辑器](custom/src/Swarm/FleetExecutionProfileEditor.md)。不以累计diff、文件数或批次号计算完成率。

保持原生profile／policy schema、参数观察、完整保存CAS与所有发送门禁；只新增表单输入及草稿来源核对。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包。下方437及更早为历史。

## 437：当前部署的启动与地面续审操作链

本批7源码／资源、14文档，合计21路径；其中新增启动Panel、其文档和工作流文档。实际路径及工程登记见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，行为见[工作流](custom/src/Swarm/FleetMissionStartupWorkbench.md)。不按累计工作树或批次号计算完成率。

真实三步发送、独立认证和原生门禁沿用；新地面riskSummary只供显示，不授予许可。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包。下方436及更早为历史。

## 436：实际Run简报、本地收件箱和接收回执

本轮7个源码路径、15个文档路径，合计22路径；完整列表见[436补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。复用ExecutionReport生成有限简报，扩展Manager的记录／分页，接通Guided原生取源与身份检查，并替换指挥中心交接面板。

只统计本批触及路径，不把工作树此前累计修改当本批新增，也不据路径数计算完成率。没有编译、测试、运行、改飞控、提交、更新PR或打包。交接是本地只读简报的人员确认，不是机载接管或跨站产品完成。下方435及更早为历史。

## 435：同一新余段记录的只读地图和逐项明细

本轮12个源码／工程资源路径、16个文档路径；精确路径见[435补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。新增纯显示后端、Coordinator入口与Root一次缓存，新增分页QML，接入FlyViewMap和既有余段面板；更新地面部署提示及工程／资源登记。

不改变原生上传／启动协议，不恢复任何历史批准；当前工作树此前累积变化不属于本轮新增计数。仅源码与限定静态核对，未编译、测试、运行或打包；未提交／更新PR。以下434及更早为历史。

## 434：独立余段审阅来源与原地面上传界面复用

本批4个QML源码、13份文档，共17路径，见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。不新增C++类、资源、协议、飞行入口或独立上传服务；使用433已登记的面板和既有原生后端。

Guided增加当前新审阅上下文与精确token的采用／申请／批准／退出；余段面板区分编辑和采用；Planner与唯一DeploymentPanel显式选择来源、审批资格和刷新成员。历史归档和已经发生的本地动作不因后置失效被抹掉。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；这不是完整25目标验收。下方433及更早为历史。

## 433：余段重做提案的取源、审阅与归档

本轮路径及各文件职责见[精确补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。新增[取源合同](custom/src/Swarm/FleetMissionRemainderSource.md)、[规划helper](custom/src/Swarm/FleetMissionRemainderPlanner.md)与[界面合同](custom/src/Swarm/FleetMissionRemainderPanel.md)，并接原Compiler/Coordinator/PlanStore，不另建飞行执行通道。

实际原任务纯值快照→选定Run/成员→人工包含式后缀→目标自身端点/速度/新profile→新审阅记录。来源hash只作一致性，不认证飞机、任务完成或控制权；旧Run和其他成员保持原状态。缺旧完整来源不能借最新计划补造，绝对时序须另行重规划。

这是累计工作树源码增量，不是已编译安装包或已更新PR；未编译、测试、运行或刷机。全25目标仍有实质开发，不能按批次号计算完成率。以下432及更早为历史。

## 432：备用独立路线预算及编辑入口

本批11个源码／工程登记路径、18份文档，共29条路径，见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。新增 FleetReserveEnergyPlan h/cc/md 和备用编辑说明；Draft、Compiler、Coordinator、Scheduler、现有Planner及Guided贯通同一份显式策略。custom.pri 登记新纯计算类，无新QML类型或资源文件。

备用缺少独立预算时保留未知／拒绝行，不借另一架主机的时长；共同任务下限之外另加待命开销。输入清除、角色／成员变化和历史草稿恢复均保留版本边界，不恢复旧审批。当前是累计源码增量，不是独立可构建补丁、安装包或新PR。未编译、测试、运行或放行；下方431及更早为历史增量。

## 431：原始电池来源、动态快照与预算时效

Vehicle→Swarm→Registry→Scheduler→Guided／Planner 的电池报告链已写入源码。新增3份模块文档，修改既有后端、界面和交接说明；本批9个源码路径、16份文档，共25条路径，见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。无新QML类型或资源注册；既有未跟踪文件也必须纳入累计源码交接。

动态时间只来自实际原生绑定，不借护照、普通metadata或Fact缓存；没有真实耗能标定、多推进电池模型或物理换电认证。本批不是独立可构建补丁、安装包或新PR。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方430及更早为历史增量。

## 430：逐机预算、原分组约束及未知性能口径

Coordinator→Scheduler→RiskEvaluator→现有 Planner 的逐机能量账单链已写入源码；Matcher／RoleAllocator 同步兼容原 Registry 的精确数值 -1 未知标记，保留原证据要求。主机不足不能由另一架备用机补人数隐藏，缺失数据不当作零缺口。

本批7个源码路径、17份文档，共24条路径，见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。没有新增QML类型或资源登记。不是独立可构建补丁、安装包或新PR，既有未跟踪文件也必须纳入交接。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

## 429：运行观察报告的实际取源、导出与打开

新增独立[报告后端](custom/src/Swarm/FleetMissionExecutionReport.md)和[QGC面板](custom/src/Swarm/FleetMissionExecutionReportPanel.md)，接入原运行观察入口。Root只从选定scope的实际previousStartup取源，结束原生调用后再核来源，保留一份冻结报告；公开值是副本，导出不取界面可改的报告。原FleetMissionReportBuilder仍负责飞前计划／风险报告。

本批9个源码／资源登记路径、15份文档，共24条路径，详见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。不是独立可构建补丁或安装包；未跟踪的新文件同样必须交接。未编译、测试、运行helper／UI／应用／设备，未改PX4、刷机、提交、更新PR或打包。

## 428：历史归档规划恢复为独立草稿（源码阶段）

[本地版本库](custom/src/Swarm/FleetMissionPlanStore.md)增加指定提案会话的全部历史版本摘要与精确记录取源；[历史规划入口](custom/src/Swarm/FleetMissionArchiveDraftPanel.md)用于在现有启动加载的档案中选择具体版本、预览来源并明确创建可编辑副本。不以“最新版本”替代已选版本，不依赖原失败部署或426对账；旧记录没有完整编辑源时明确不可恢复。

恢复仅复制规划内容，重置地图基准确认，并复用427的独立草稿、上一份备份及替换确认。历史sourceTaskId只作来源；重新审阅使用当前提案会话身份及当前设备证据，不恢复旧审批、机载状态、任务执行或控制权。档案只读恢复不解除写入隔离，后续归档仍可能因冲突或容量被拒绝。

本批接线与待验证边界见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／应用／设备，未修改PX4、刷机、提交、更新PR或打包。只覆盖已加载本地有效档案的规划编辑恢复，不是损坏档案修复、通用任务编辑或机载任务续飞；原25目标及六类实质工作继续。下方427及更早为历史增量。

## 427：保留完整原草稿并建立人工修复副本（源码阶段）

[区域草稿](custom/src/Swarm/FleetMissionDraft.md)现在随编译计划保存有界完整编辑源，不再尝试从生成航线反推原多边形；[编译器](custom/src/Swarm/FleetMissionCompiler.md)核对源与实际规划字段一致，[版本库](custom/src/Swarm/FleetMissionPlanStore.md)把源纳入原内容摘要和历史存档。旧记录缺源时明确不可恢复，通用任务不会被伪装为区域编辑器草稿。

[失败部署取源](custom/src/Swarm/FleetMissionRepairDraft.md)基于原失败／取消部署与已排空的426核对结果，保留different／unknown，不将其称为已修复。新[操作入口](custom/src/Swarm/FleetMissionRepairDraftPanel.md)只创建独立本地编辑副本，保留上一份草稿供明确切回；新副本重新确认WGS84／AMSL基准，再走原新预览、原生审阅、新记录版本及人工审批。不是新taskId，不恢复旧审批，不自动上传、清空、回滚或启动。

本批范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定源码核对见[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)，待实际验证事项见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／应用／设备，未修改PX4、刷机、提交、更新PR或打包。跨重启历史草稿恢复入口、逐项修复建议及完整受控执行、可信性能、自愈、跨站、长期运营、真实模型／生态仍待开发；原25目标保持，以下426及更早是历史增量。

## 426：失败／取消后的独立整组只读核对（源码阶段）

[独立核对工作流](custom/src/Swarm/FleetMissionReconciliation.md)从本实例原失败／取消部署冻结期望任务，由当前有权操作员明确发起；不要求旧回读全部匹配或旧批准仍有效。[传输](custom/src/Swarm/FleetMissionReconciliationTransport.md)使用独立RAW INT读取和一次性结果，[界面](custom/src/Swarm/FleetMissionReconciliationPanel.md)逐机区分相同、不同、未知与尚待处理。

实际成功COUNT0与协议失败分开；诊断不覆盖普通任务显示缓存、不生成启动收据、不自动修复／清空／上传或启动。全组逐机读取不是原子快照；相同仅是该次受支持规范化内容的比较，未知不能当相同。不支持的机型／协议、失效来源和无法规范化内容保留限制，不能据此声称所有异构机已支持。

本批源码／文档范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，静态审阅与待执行项目分别见[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／认证／应用／设备，未改PX4、刷机、提交、更新PR或打包。修复草稿及完整受控执行、可信性能、自愈、跨站、长期运营、真实模型／生态仍待开发；原25目标保持，以下425及更早为历史增量。

## 425：普通任务回读差异诊断（源码阶段）

普通任务部署增加逐航点回读差异诊断：在既有回读结果中保留有限字段差异和来源操作标识，失败／取消后不再只剩通用失败码。 [比较合同](custom/src/Swarm/FleetMissionReadbackDiff.md)、[实际部署接线](custom/src/Swarm/FleetMissionDeployment.md)与[界面说明](custom/src/Swarm/FleetMissionReadbackDiagnosticsPanel.md)分别说明数值、来源和展示边界。

界面先显示逐机历史摘要，再按需展开有限明细；缺项、null、未知、规范化失败和截断不能显示为“没有差异”。原回读匹配与启动审阅门槛保持；诊断来自普通解码后的MissionItem，不证明独立RAW INT来源、此刻机载内容、控制权或起飞许可。内存部署历史仅保留摘要，不保存无限差异列表。

本批仍不提供失败后整组独立重新读取、修复草稿、自动覆盖或回滚。原25目标和六类实质开发继续，不能按批次号或文件数计算完成率。源码范围见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，工程师待验证项目见[交接说明](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／应用／仿真／设备，未改PX4、刷机、提交、更新PR或打包。以下424及更早是历史增量。

## 424：跨日志库的有界历史检索

[归档面板](custom/src/Swarm/FleetArchiveHistoryPanel.md)增加独立检索入口；范围为本机诊断事件库的全部封存段，或封存段加固定的已停止录制未封存库。按机号与四种工程审批操作查询，不再仅筛选当前展示页。选择包含未封存库时，文件缺失属于失败，不静默缩小范围；查询不会自动停止录制。

每批最多读取4页、消费256行、返回64条匹配，并有字节上限；续扫游标来自原生实际结果，空匹配不等于扫描完。界面区分本批计数、已扫完文件和所选范围完成，保留来源文件、退休记录与异常退出等覆盖信息；不自动累积无限结果。完整合同见[Reader](custom/src/Swarm/FleetEventArchiveHistoryReader.md)和[Store](custom/src/Swarm/FleetPersistentEventStore.md)。

目录清单元数据在批次前后及续扫时核对，当前正在分页的文件另绑定实际epoch和整文件指纹；这不是跨文件原子快照、已读文件的持续哈希复核或全部飞行历史完整性证明。历史不恢复账号、批准、控制权或飞行权限。旧v1未记录的审批操作不会推测补齐。

本批源码与待验证范围见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／SQL／UI／应用／设备，未修改PX4、刷机、更新PR或打包。以下423及更早为历史；本批部分补足其“全库检索”缺口，但不完成永久全集索引、遥测全集、留存治理或恢复工具；原25目标和六类实质开发继续。


## 423：停止录制后的未封存库离线查询

[归档面板](custom/src/Swarm/FleetArchiveHistoryPanel.md)新增明确的离线读取入口，复用原[读取线程](custom/src/Swarm/FleetEventArchiveHistoryReader.md)，只读固定本机未封存的 fleet-events-v1.sqlite。无需先恢复录制、连接飞机或登录飞行账号；本实例录制线程必须已结束，不能用点击停止或captureEnabled=false代替实际退出。

数据库以只读方式打开，不创建、修复、迁移或删除库；读取期间使用与录制器共用的协作锁，因此会创建／释放临时锁文件，不宣称完全零文件写入。遗留journal/wal/shm、坏库、锁冲突或身份变化均拒绝，不自动清理。每页64行，后页绑定原实际库epoch与整文件指纹；两页之间库变更须重新读取第一页。

该入口沿用历史只读边界，不恢复账号、能力批准、任务或飞行权限；异常退出、退休行及丢失尾部信息保留，查询成功不等于历史完整。下方422的“离线只支持封存段”由本批有条件的未封存读取补充；全库搜索、完整构型历史、恢复工具和长期留存仍未完成。

本批范围见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，运行待办见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码及静态文本核对，未编译、测试、执行helper／SQL／UI／应用／仿真／设备，未改PX4、刷机、更新PR或打包。原25目标及六类实质开发继续，不能据本批声称整体落地或承诺完成日期。

## 422：显式本地归档与工程审批摘要（源码阶段）

[归档入口](custom/src/Swarm/FleetArchiveHistoryPanel.md)接入指挥中心与任务规划页：默认关闭，明确开始／停止／封存并继续录制，查询当前录制批次或已封存段，按原生游标分页和取消。复用原 Store、BlackBox、Worker 与 Reader，不新增数据库，不依赖飞机在线或飞行账号；只读历史不会恢复权限或触发飞行命令。

四种能力／基线操作新增独立的[有限工程摘要](custom/src/Swarm/FleetEventArchiveCodec.md)：操作、机号、提交与结果三态、指纹和差异计数。不保存账号、自由理由或原始构型；内存接纳不等于 SQL 提交，原审计链追加也不等于持久化。旧 v1 未记录的详情不能补回，摘要失败不改变已发生的登记提交。

本批11源码／资源、16文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，待执行验证见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅静态阅读与限定文本检查，未编译、测试、运行 helper／UI／认证／SQL／应用、修改 PX4、刷机、更新 PR 或打包。

仍缺停止后未封存活动库的离线读取、全库检索、完整构型历史及长期留存管理；本页筛选仅当前页，封存数量不是总磁盘硬配额。原25目标与完整受控执行、可信性能、自愈、企业跨站、长期运营、真实模型／生态六类工作保持未完成，不能按阶段号计算完成率或可靠工期。下方421及更早为历史，“只有值／无归档UI”仅由本批固定本机入口部分覆盖。

## 421：构型基线批准／撤销接入实际操作链

[基线合同](custom/src/Swarm/FleetConfigurationBaselineEvidence.md)、[原生工作流](custom/src/Swarm/FleetCapabilityReview.md)和[共用面板](custom/src/Swarm/FleetCapabilityReviewPanel.md)已接四种独立操作。维护/管理员申请、不同合格账号审批；能力采纳与构型基线使用不同精确用途，不互借凭据。首次完整配置无旧基线也可批准；当前配置未知时，只能撤销真实存在的本地批准，不能建立新基线。

重复批准同一基线也退役旧能力批准，须另行审阅能力。原生先冻结真实提交回执再通知，后置失效或审计失败不抹提交事实。修复界面对合法旧记录大写哈希/首尾空白的误拒；原记录不改写。完整有类型基线仍仅在本进程保存，不认证硬件、改装或性能。

本批9源码、17文档见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，待执行项目见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码与限定静态审阅，未编译、测试、运行UI/认证/helper、修改PX4、刷机、更新PR或打包。原25目标/六类仍有实质开发，不能据此计整体验收或完成率；下方420及更早“无基线批准界面”为历史状态。

## 420：完整有类型基线接入任务消费者

[构型基线合同](custom/src/Swarm/FleetConfigurationBaselineEvidence.md)将实际 Registry 当前完整配置与本机批准快照统一接到 Matcher、RoleAllocator、Compiler 和 Continuity。旧 configurationState 只保留哈希级报告语义，不再单独满足 requireConfigurationBaseline 或获得基线排序优待。

新观察身份进入 Guided 旧/新机队指纹和 Coordinator schema2 审阅归档；Deployment 核归档与实际审阅一致，仍须当前来源检查。hash-only、未知或导入旧批准不能通过；恢复原配置可以重新审阅，但不能复活旧批准。可选基线政策不变成所有任务的强制门槛。

本批12源码、20文档见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。没有新增基线批准界面/持久化或硬件、性能认证；原25目标/六类仍有实质开发。仅源码和限定静态检查，未编译、测试、运行、修改PX4、刷机、更新PR或打包。下方419及更早为历史，其“任务仍仅按旧标签判断”由本批限定消费路径覆盖。

## 419：构型快照和字段差异已接源码，未运行验收

[构型比较](custom/src/Swarm/FleetConfigurationDiff.md)将实际Registry的本机完整批准快照或前次不同完整观察接入原双账号能力审阅。界面区分来源、类型、Missing/null和截断；缺前/后值不报告无变化。只按显式非空map视作完整上报，不认证每机型字段完整、实物或性能。preview独立升schema2/18键，原context6/approval14不变。

typed变化与完整观察丢失/恢复会退役旧能力批准；审批-only导入同样退役触达记录。新构型基线批准要求当前完整观察，不能批准hash-only之后残留的旧map。普通健康/重复同配置不滚动历史；私有历史仅本进程，无新增持久化或基线批准UI。

本批8源码/工程登记、15文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。原25目标/六类仍继续；下一源码切点是统一可信基线证据与Matcher/RoleAllocator/Compiler消费。旧configurationState/requireConfigurationBaseline仍为哈希级，不能按本批差异展示宣布完整任务基线门禁完成。没有编译、测试、运行helper/UI/认证/应用/设备、修改PX4、刷机、更新PR或打包。下方418及更早是历史。

## 418：能力采纳与撤销的实际操作流程

本阶段16源码/资源、17文档（共33路径）见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。新增 Review h/cc、能力面板；修改 Registry、Session、Role、Swarm、实际 custom Guided、两个宿主及资源登记。Swarm 同时补当前 Registry 来源销毁/刷新回调的直接相关保护，不改变健康/动力策略。

成果为本地单机工程声明的完整预览、独立账号审批、最终版本核对、采纳/撤销与事实回执，不是硬件认证、自动检测改装或完整性能认证。审计和工程师待执行矩阵同步；未编译、测试、运行、更新 PR 或打包，原25目标仍未完成。

## 417：固定文件工程目录重载与全球任务估计器门禁

本阶段19个源码/资源路径及19份文档（共38路径）见[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)：StartupEstimator h/cc 与 Vehicle.cc；Catalog h/cc；新 Reload h/cc；OperatorSession h/cc 与 RolePolicy.cc；Coordinator h/cc；Guided、新目录面板及两个宿主；custom.pri、qrc、qmldir。不能把源码数量当完成率或独立可构建包。

实际增量为纯完整替换预览、保留原旧/新工程声明、差异及双哈希、固定 AppData 文件原生有界读、独立两账号一次审批、原目录 CAS/实际 scope/期限/最后复读及纯围栏。替换只落在内存 catalog，不写档案；目录改变使旧任务 review 失效，但不停止 Run 或清占用。真实 commit、外层后置失败和日志追加结果分开，不能按失败响应自动重试覆盖。

全球坐标启动参考另要求 ABS 水平/垂直且非 CONST_POS；原 RAW/通用 Health/室内/GNSS 硬件合同不扩展。原启动自动 load 不在本次双账号改造范围。限定审阅记录见[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)；未编译、测试、运行、认证、设备操作、改 PX4、刷机、更新 PR 或打包。原25目标/六类继续，以下旧阶段保留为历史。

## 416：每机 executionProfile 的实际草稿入口

本阶段11个源码/资源触点是 Draft h/cc、ReviewCoordinator h/cc、ParameterManager h/cc、Guided、Planner、新 `FleetExecutionProfileEditor.qml`、qmldir 与 qrc；另同步15份文档，共26路径。精确路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定文本检查见[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。这些数量不是产品完成率。

实际增量为逐机完整 profile 的 revision CAS、成员生命周期保留/删除规则、活动 assignments 透传、只读真实 12 参数哈希/指纹、保存前重读及原 review 失效。结构化核心编辑加完整 startup JSON 1–3，不补默认容差；留空不准受控启动。旧的规划设置不是机载执行配置。返回 false 可能发生在本地内容已提交后的通知尾窗，不能按“未提交”重试。

保留全部历史和累计依赖；没有编译、测试、运行、PX4 修改、设备操作、PR 更新或安装包。原25目标/六类实质开发不缩减。

## 411–415：恢复复核实际接线增量

本阶段精确范围为9个源码/资源触点、18份文档，共27路径，见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。包括新[恢复复核面板](custom/res/Merivus/FleetOccupancyRecoveryPanel.qml)、Guided私有原审阅持有与值投影、双入口及qrc/qmldir登记、Review实际Root模式和Swarm当前真实unknown诊断。不是只交这9个文件就可运行的独立包。

[界面合同](custom/src/Swarm/FleetOccupancyRecoveryPanel.md)说明6参ForController、5个QVariant来源回调、27键current/13键result、8键原收据及13键持久化诊断。取消已接原scope/账号/输入失效；日志false或异常不抹已发布收据/已返回解除事实，审计接纳与持久化另报。

只做源码与限定文本审阅，没有编译、测试、运行、PX4修改、刷机、PR更新或打包。原25目标/六类实质开发未完成；下方旧阶段无UI的描述不覆盖本节。

## 406–410：原生重启／历史占用专用复核

[恢复复核合同](custom/src/Swarm/FleetOccupancyRecovery.md)新增实际Swarm唯一拥有的原生工作流，公开只读对象和request/独立审批/取消入口；未加QML操作界面。bootstrap历史待审与本轮真实unknown分集合，整组最多32个选中地址只可解除restart/history holds，真实unknown、实际owner、任务、临时任务、隔离和everObserved历史不清除。

原Vehicle/MM/参数/VLM/MVM/model及主链路共享所有者冻结，新鲜2秒内原始HB未解锁与已知ON_GROUND、完整本机队列排空进入30秒复核。专用fleet.occupancy.recover.approve强制不同真实账号，一次消费不生成lease；原账号TTL、当前Episode/角色在最终无回调尾栅再核。现场处置编号和地面检查是明确人工声明，不认证物理停机、远端无任务或控权。

所有可回调读取后，整组再以纯native原RAW语义revision/hash/value/鲜度/队列核对，并核原worker、实际占用和任务map；同栈整体移除本地待审后bump，新actual身份须由原worker实际提交才恢复既有continuity getter/CAS。留存原QQ、源epoch与primary控制块，换源在getter/最终CAS即时重加hold，不只等timer；正常收包、合法起飞和新MM工作不冒充换源。

本批12源码/工程登记、17文档，共29路径。原生recoveryWorkflowImplemented与UI集成分开；localHoldsReleased仅历史动作事实，selectedReviewRequiredVehicleIds另报当前待审。仍缺UI、离线/报废现实处置、本轮真实unknown受控解除和完整机载自愈；完整25目标/六类实质开发保持。未编译、测试、运行helper/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方401–405及更早是历史状态，不以旧“无任何再审入口”覆盖本批native入口，也不宣称全部恢复产品已完成。

本批12源码/17文档的[精确路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)已同步。新增Evidence与Review两组h/cc，Swarm持有/门禁/历史来源分离，Session专用审批及原TTL尾核，RolePolicy权限，Vehicle三个只读native尾栅及custom.pri登记须整体交接；不是12文件独立包。

## 401–405：原生异步占用留存与重启待审

[占用留存合同](custom/src/Swarm/FleetOccupancyJournal.md)已把严格11键Codec和单槽合并worker接到实际Swarm构造/原生timer，不依赖无人调用的save/load或QML可写路径。固定AppData目录、本地合作锁、原文件基线/目录身份前后核验、无直接覆盖回退的原子替换及读回，与当前实际源身份分开；不是完整捕获、write-ahead或断电durable证明。

每个新Swarm先为1–255协议地址建立独立restart-review holds。历史ID只并入真实未知，不重建owner、任务、Run、账号或权限；空/缺失/旧open文件和写入成功都不能解除这些holds。纯重启占位不冒充已观察到的飞机记录。当前尚无专用再审解除流程，所以本阶段连续性改派保持待再审限制，不能称恢复后已可调度；此流程仍须继续开发。

公开native占用快照必须等原worker bootstrap、当前完整身份已实际提交并读回，且actual==offered==committed；存储失败/失源/坏文件/代次耗尽拒连续性提交。磁盘IO只在worker线程，原Land/RTL/ABORT不等IO、不因账本失败新增否决；析构只协作stop，不写clean或释放飞行占用。只读状态标明last-poll和recoveryWorkflowImplemented=false。

本批7源码/工程登记、15文档，共22路径；不是全部25产品目标、安装包或完整交付。QSaveFile结果仅记atomicReplacementConfirmed，osSyncVerified/durableVerified始终false。未知授权解决、正式受控执行/StartPermit、可信性能、企业跨站、长期运营及真实模型/生态等六类仍有实质源码工作。仅源码和限定文本读回；未编译、测试、运行解析/helper/业务/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方396–400及更早为历史，不覆盖本节新的重启待审门禁。

## 396–400：实际本地占用进入连续性审阅与提交

[原生占用合同](custom/src/Swarm/FleetNativeOccupancyEvidence.md)新增actual Swarm快照和最终本地角色CAS守门。部署撤权不删除raw占用；编队、创建/预抢占栈、活事务、安全owner、临时任务/清理及内容隔离分别保留。已知结果不明不会因原记录清理变成可用；当前账本没有自动清除或跨重启恢复证明。

Continuity排除所有held/unknown候选；原成员任一实际占用即整任务hold，不通过换掉成员来假装释放。Guided刷新/应用要求原实际快照，稳定原生身份进入建议；缺失或坏证据不能产生产品可应用的完整提案。旧孤立值诊断仍明确requiresNativeOccupancyReview，不授来源认证。

Swarm与Orchestrator双向sticky pin，源死不换空实例。新六参数角色CAS在同栈提交前核冻结身份与old∪new实际状态；绑定后的旧本地入口也不绕过当前占用检查。不改实际owner、不发命令、不解除native Run保护；只代表本地模型变更，不是飞机空闲、正式机载改派或控制租约。

本批12源码/工程登记触点、15份文档，是累计工程增量，不是完整25目标完成或安装包。包括直接相关的CommandTransaction通知后存活/原尝试检查及嵌套start/retry保护，不是全仓回调安全认证。未知状态的授权解决/跨重启留存、完整受控机载执行、性能/能量与企业跨站等六类仍继续。仅源码与限定文本审阅，未编译、测试、解析/运行helper、业务/认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅摘要占用/五参产品应用”的描述由本节覆盖。

本批27路径为12源码/工程登记与15文档；实际路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不是25项产品完成计数，也不能单拿这12文件替代累计完整工程。

本轮静态收尾已补9处事务创建调用、直接编队通知/停止链、Goto前置清理/取消和上传/清理尾窗。底层事务通知后核原存活/attempt；Land/RTL保留单目标失败后继续其余原有效目标，派发安排与unknown事实分开。两组限定交叉读回与最终文本清点已记审计；未扩审全部历史异步回调，未证明编译/运行或机载自愈。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

当前26路径是累计工程增量，不是26项目标完成，也不是独立安装包；实际文件列于[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

实际14路径以 [本轮清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) 为准；18文档包含新能力审阅合同和六个发现/规划/审阅模块说明。不是14项产品已完成，也不是仅拷这14文件即可构建。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

实际7源码为 `src/Vehicle/Vehicle.h/.cc`、`custom/src/Swarm/SwarmController.cc`、`FleetHealthCenter.cc`、`FleetMissionExecutionObservation.h/.cc` 和 `SwarmMissionOrchestrator.cc`（后三组在同一Swarm目录）。19文档为13份公共/工作流说明，加新VehicleHealthTelemetry与ExecutionObservation、Orchestrator、HealthCenter、VehicleMissionExecutionObserver、ProgressObservation六模块说明；合计26路径，是累计工程增量，不是七文件独立构建包。

## 371–375：8源码 + 19文档的增量范围

本批8个实际源码/资源路径：`custom/src/Swarm/FleetMissionDeploymentLifecycle.h/.cc`、`custom/res/Merivus/GuidedActionsController.qml`、新增 `custom/res/Merivus/FleetMissionLifecyclePanel.qml`、同目录 `CommandCenterOverlay.qml` / `FleetMissionPlannerPanel.qml` / `qmldir`，以及 `custom/qgroundcontrol.qrc`。19文档为本入口等13份公共/工作流说明，加 Lifecycle、ControllerScope、Session、NativeOwnerCallbacks、ObservationPanel 与新 [LifecyclePanel 说明](custom/src/Swarm/FleetMissionLifecyclePanel.md)，合计27路径；限定检查记录见本轮静态审计，不按编号计算目标完成率。

两个 `ForController` 接口在同一 native call 中完成 begin → 私有栈 permit mutation → 新 scope 核验 → 原 owner/ID end，不释放后转调 legacy。Guided 两个固定 public API 只接受显示 token/目标组/明确确认，公开12键只读 metadata、最多8条6键行；raw refs 不进入值 DTO。独立 transitionBusy 覆盖整个 JS 栈，纯 scope/回调不以此自拒。已返回的发布事实先作本地草稿失效/身份意图协调，再进行可能失败的后置读取，既有事实不因尾窗丢失而改成未发生。

新面板在指挥中心和规划页各有一处入口，双步确认、默认关闭/取消、禁止 Enter 提交，并固定原 view ticket。prepare 不自动切 selected、登录、继承 grant、停止旧 Run 或释放占用。mutation 已提交与 end 是否成功分开；失败尝试仍消耗8组容量，`localObjectGroupAllocated=false` 不是“没有分配尝试”的证明。

这是累计完整工程上的增量，不是8文件独立包；`outboundIntegrationComplete=false`、完整25目标/六类缺口保留。本机未编译、测试、解析/运行 helper、认证、应用或设备，未更新 PR/包。以下366–370及更早清单保留为历史，旧“尚无入口/专用 transition”口径由本节覆盖。

## 366–370：控制器真实接线与同步调用占用

入口计数口径：30指当前组业务入口，另有当前账号登录/退出2个 `runCurrent` 与4个 `runObservation`；24指来源/上下文回调。private的是raw引用id与调用桥，public entry经过桥、callback经过scope helper，不是所有函数都改成private。

5源码是累计工程上的增量，不是独立可构建包；交接要带完整源码/pri/注册及原生依赖。nested draft 的 finally 仅对原scope cancel/revoke，不能核实撤销时私有 `retiredCurrentScope` 持续拒绝同组 current 入口/回调，selected 监视仍可读，不超时复活或回滚已 queue。Qt Int/QObjectStar往返/方法绑定/parent/销毁尚待工程师实际验证，不放宽原合同。

本批实际修改5个源码：`FleetMissionDeploymentLifecycle.h/.cc` 新增 begin/matches/end 三个 controller-call 接口及 mutation 忙门禁；`custom/src/CustomPlugin.cc` 注册可创建 Lifecycle/不可创建 Episode；`GuidedActionsController.qml` 接私有工厂、14服务引用、30个 Fleet 入口/24个原生来源回调、明确身份意图与 retained Close；`FleetMissionObservationPanel.qml` 接 selected token、完整收据及原组取消路由。另同步18份既有文档，合计23路径；不把改动编号当产品完成计数。

原构造已提供 initial，不自动 prepare；无 legacy fallback/公开 raw refs。外层 finally 只结束原 owner+callId，pure 回调不因调用忙而自拒，嵌套草稿改动使 active current 调用 stale，真实已 queue/未知结果不回滚。selected 四值同 scope，token 含选择代次，取消不按 UUID 搜索其它组；显式登录可撤当前未消费批准，不结束 Run。新部署/选择产品入口及专用 transition API 仍缺，`outboundIntegrationComplete=false`，原25目标/六类继续。已完成限定文本审阅，具体证据见本轮静态审计，不是运行或安全通过；未编译、测试、运行、更新PR或打包。以下为历史批次。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](custom/src/Swarm/FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

| 编号 | 实际增量 |
| --- | --- |
| 361 | current引用捕获/精确比对、独立outbound代次，不混普通revision |
| 362 | selected观察与actualClose retained原组scope，独立观察代次、旧raw地址不解引用 |
| 363 | Session全进程GUI认证调用栈token，原对象被毁仍持锁至旧栈退出 |
| 364 | Lifecycle切代/选择接实际提示busy，不阻纯查询或本人身份复核 |
| 365 | 跨组源码审阅、限定文本检查、清单/审计/工程师说明同步 |

编号不是25项目标完成数量。4源码均仍未跟踪，须随累计源码交接；路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，检查见[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

| 编号 | 实际增量 |
| --- | --- |
| 356 | Deployment/EvidenceRefresh/ContentCheck/StartReview共10处Direct首传实际调用者 |
| 357 | UploadApproval/StartApproval/StartupAuthority/GroundAuthority共6处Direct首传实际调用者 |
| 358 | GroundReview/GroundApproval/Txn/Close共4处Direct首传实际调用者，旧Close不变权限 |
| 359 | Guided19个签名、非空实际receiver检查、双upload生产者及本地预检查分离 |
| 360 | 双侧合同/调用交叉源码审阅，模块旧签名与清单/审计/工程师说明同步 |

数字是增量记录，不是25项目标完成数量；本轮13实际源码、26文档，限定源码/工程文本范围36路径。路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，结果见[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](custom/src/Swarm/FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

| 编号 | 本批实际增量 |
| --- | --- |
| 351 | PlanManager私有scoped入口、op0→opN关联、adoption后复核及closed至原栈排空 |
| 352 | Vehicle专用typed消息发送、插件前后/最终queue门禁和原signed帧保护 |
| 353 | Deployment冻结原实际来源，普通上传/回读切换私有原生接口 |
| 354 | sender忙纳入本地quiet，旧MM丢失仍排空，Episode记录原primary |
| 355 | 分组与交叉源码审阅、限定文本检查、修改清单/审计/工程师说明同步 |

编号是增量记录，不是25项产品目标的完成数。实际路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定检查见[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

## 346–350：十二类服务的新请求与移交门禁

| 编号 | 本批实际增量 |
| --- | --- |
| 346 | Deployment、EvidenceRefresh、ContentCheck、ArmedGroundReview入口/live上下文、原scoped fence及caller发送边界核current实际组 |
| 347 | UploadApproval、StartReview、StartApproval、StartupAuthority的审批/上下文/消费/移交核owned同组，前后复核 |
| 348 | GroundApproval、GroundAuthority、Txn、Series同组门禁及未发布一次bootstrap，不封旧观测或取消排空 |
| 349 | Authority/Txn认证移交比对改pure native、保留外层完整门禁；真实已queue/uncertain事实先保存；修3处primary Link线程误拒 |
| 350 | 分组交叉源码复读、限定文本检查、修改清单/审计/工程师交接同步 |

实际源码增量12个既有`.cc`，无头/API、磁盘或receipt schema变更；完整路径见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。限定检查35个源码/工程路径与25份文档，最终结果见[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)，不是编译/测试结论或25项目标完成数。

普通Deployment的writeMissionItems/loadFromVehicle仍是legacy core API，其插件后最终发包与异步每包/重试缺Episode scoped fence，caller gate不冒充该覆盖。Guided实际Episode/epoch、共享Role身份作用域和selected旧观察整组投影仍待开发；无新类型/界面注册，outboundIntegrationComplete=false。未编译、测试、运行业务/认证/应用/设备、改PX4、刷机、更新PR或打包，原25目标/六类不缩减。

## 345：Session 的非 Close 原生用途门禁

`FleetOperatorSession.cc`已为非Close独立审批、upload/start/ground凭据消费/移交及其Current证明检查接入actual current Episode门禁。显式Approval/Authority/Txn须和Session属于同一个实际原生组；只有双方均为真正legacy才沿用旧行为。系统身份/owner回调后、烧凭据或移交前及返回前再次核对，失败不恢复旧proof。

普通登录/退出、只读身份事实、工作站撤销、精确revoke/clear/expiry以及专用Close审批/即时消费保留，不对Session整类封禁。Episode配对helper和14类sticky provenance位于340–344已列文件中；本步另增加Session.cc及Session.md增量，与核心合并为23个源码/工程路径、18份文档。限定检查结论见[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

尚未接完其它12类新请求/发送/审批门禁或Guided Episode/epoch/旧观察身份投影；未注册QML或增加新部署入口。未编译、测试、运行身份/应用/设备、更新PR或打包，原25目标/六类继续。

## 340–344：实际 Episode 与新部署工厂（未接 QML）

| 阶段 | 本批范围 |
| --- | --- |
| 340 | 每组14个全新实际 owned 对象及只读检查所需 friend 声明；新 Session signed-out，旧完整组/Run/占用保留 |
| 341 | 全部 retained 组的原生静默证据，固定18键；不调用 OS/身份或 public current getter，不把未知当空占用 |
| 342 | 有界8次分配工厂与独立 retained 观察选择；初始/失败尝试均计数，不驱逐旧组 |
| 343 | 四个新 h/cc 在 `custom.pri` 登记及候选、父级事件、QQ/UUID/计数的发布前后复核 |
| 344 | 文档、交接与限定文本检查；限定检查结果已写入静态审计 |

核心源码/登记22路径（新4、既有 friend/provenance headers 16、`FleetMissionStartupSeries.cc` 1、`custom.pri` 1），文档17份（本页等16份既有文档及新增 [Lifecycle.md](custom/src/Swarm/FleetMissionDeploymentLifecycle.md)）。Series新建ground child在公开前继承原实际Episode的sticky原生来源；不使用可写动态属性或重新parent推断授权。后续345的Session实现另列。

**尚未注册 QML、替换 Guided 或提供新部署入口，现有应用仍单链。** native sticky outbound helper 不是已接线门禁；owned 请求/传输继续点、QML epoch/source callback 和 selected Run/Series/Close/Session 同组投影仍待开发。未编译、测试、运行业务/认证/应用/设备，未更新 PR 或打包。

## 335–339 本次修改记录

| 编号 | 实际增量 | 主要路径 |
| --- | --- | --- |
| 335 | 实际当前Run查看与独立监视审阅视图；去掉旧初始Run回退 | 新 `FleetMissionObservationPanel.qml/.md`、`GuidedActionsController.qml` |
| 336 | 每入口controller/review完整token归属、精确取消、plain JS生命周期与调用ticket，modal全过程锁存 | 新面板 |
| 337 | 三项默认未勾确认、默认取消/危险Enter拦截、状态失效清空、文本三态结果及可滚动键盘错误摘要 | 新面板 |
| 338 | 指挥中心及Planner固定入口，qrc/qmldir登记，不创建第二controller/backend | `CommandCenterOverlay.qml`、`FleetMissionPlannerPanel.qml`、`custom/qgroundcontrol.qrc`、`qmldir` |
| 339 | 限定源码交叉复读、文本检查及修改清单/审计/交接说明同步 | 本文件及相关说明 |

本批6个实际源码/资源/模块路径、18份文档有增量，无C++后端协议/权限/持久化格式变更。完整交互契约见[面板说明](custom/src/Swarm/FleetMissionObservationPanel.md)；不是Start/ARM、停飞或Completed界面。ui-ux-pro-max用于取消优先、语义警示、换行及焦点交互，不运行skill脚本、应用或视觉验证。未编译/测试/认证/设备/刷机/更新PR/打包，不能因此宣布全目标完成。

## 330–334 本次修改记录

| 编号 | 实际增量 | 主要路径 |
| --- | --- | --- |
| 330 | 独立短期审阅、真实owner冻结、两账号审批与一次原生关闭 | 新 `FleetMissionObservationClose.h/.cc/.md` |
| 331 | 专用系统审批purpose、private一次消费及独立本地角色动作 | `FleetOperatorSession.h/.cc/.md`、`FleetRolePolicy.cc/.md` |
| 332 | actual Run/Swarm/Orchestrator来源、18键关闭范围和真实占用/撤销快照；结束监视不借MODE准入 | `FleetMissionStartupTransaction.h/.cc`、`SwarmController.h/.cc`、`SwarmMissionOrchestrator.h/.cc` |
| 333 | 新类型/工程注册、readonly backend门面及显式三项确认函数，无新UI按钮 | `custom/custom.pri`、`CustomPlugin.cc`、`GuidedActionsController.qml` |
| 334 | 限定源码交叉审阅及修改清单、审计、路线图、工程交接同步 | 本文件、`docs/`及相关模块契约 |

本批实际14个源码/工程路径、19份文档有增量；不能用旧已跟踪diff遗漏新未跟踪文件。完整契约见[关闭监视说明](custom/src/Swarm/FleetMissionObservationClose.md)。独立动作只结束本地monitor；安全撤销不等于实际占用消失，释放占用也不证明可重新分配、停止飞行或Completed。未编译、测试、执行认证/业务/helper/设备、改PX4、更新PR或打包，原25目标及六类未完成范围不缩减。

## 325–329 本次修改记录

| 编号 | 实际增量 | 主要路径 |
| --- | --- | --- |
| 325 | 同一Orchestrator内新增原生受保护记录、独立观察状态/revision/通知与全部generic mutation门禁 | `SwarmMissionOrchestrator.h/.cc/.md` |
| 326 | 实际Swarm一次绑定、Run在MODE前登记及逐次source match，commit/退休/析构保留事实，无MODE准备排空后收尾 | `SwarmController.h/.cc`、`FleetMissionExecutionObservation.h/.cc/.md`、`FleetMissionStartupTransaction.cc` |
| 327 | 连续性合法新phase/保守成员占用和原生工作流诊断；运营观察关闭不计成功 | `FleetMissionContinuityPlanner.cc/.md`、`FleetOperationalMemory.cc/.md` |
| 328 | 实际内部对象关联、独立readonly观察任务列表，通用continuity apply提前拒绝 | `GuidedActionsController.qml` |
| 329 | 独立源码复读、限定文本检查、审计/修改清单/工程交接同步 | 本文件、`docs/`及 `FleetMissionStartupSeries.md` 等模块契约 |

本轮实际10个源码/工程路径有增量；限定范围和检查结果以审计本批节为准。没有新增QObject/QML注册类型或工程源路径，也没有新的完成/停机/启动按钮；protected task的UUID与意图sourceTaskId不同，ObservationClosed不是Completed。只实现源码与限定静态审阅，未编译、测试、运行应用/helper、改飞控、刷机、更新PR或打包；原25目标/六类未全部闭环。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](custom/src/Swarm/SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

## 320–324 本次修改记录

| 编号 | 实际增量 | 主要路径 |
| --- | --- | --- |
| 320 | 非 QObject 的 HOME 精确八字段规范化、原始基线与有界数值消费 | `FleetMissionStartupHome.h/.cc/.md` |
| 321 | 新 schema3 九字段政策、真实 HOME 基线前置与 Txn 九字段/ODOM 门禁 | `FleetMissionExecutionProfile.cc/.md`、`FleetMissionStartReadiness.cc/.md`、`FleetMissionStartupTransaction.cc` |
| 322 | 真实 owned ARM/MODE 双 HOME 叶配对及 ARM relative-only、有界累计/独立 risk revision | `FleetMissionStartupTransition.cc/.md` |
| 323 | 本次完整冻结政策进入逐机审阅 artifact，并全文显示单位与预算 | `FleetMissionStartReview.cc/.md`、`FleetMissionDeploymentPanel.qml`、`custom.pri` |
| 324 | 独立源码复读、限定文本检查和修改/审计/工程交接同步 | 本文件、`docs/`、模块契约及 `FleetMissionStartupMotion.md` |

实际9个源码/工程路径有增量，限定检查18个源码/工程路径、19份文档；仅本批统计，不是整个工作树或目标完成数。旧v1/v2不升级，新v3仍不认证HOME有效分支/共原点/同样本/因果/净空/最终StartPermit。没有新增控制按钮或流请求；未编译/运行/测试/刷机/更新PR或打包，原25目标/六类未全部闭环。

320–324（2026-10-15）新增[显式 HOME 观测过渡](custom/src/Swarm/FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

## 315–319 本次修改记录

| 编号 | 实际增量 | 主要路径 |
| --- | --- | --- |
| 315 | 独立core execution槽、真实MODE write前begin与write返回后captured commit | `src/Vehicle/VehicleMissionExecutionObserver.h/.md`、`Vehicle.h/.cc`、core qmake/CMake |
| 316 | actual Txn 来源冻结、provisional/post-commit分离及有界连续运行观测 | `FleetMissionExecutionObservation.h/.cc/.md` |
| 317 | 一次Txn、独立监视生命周期、协议排空与原子整组本地占用转交 | `FleetMissionStartupTransaction.h/.cc/.md`、`SwarmController.h/.cc` |
| 318 | private native模块工程登记、独立readonly controller值及同部署检查结果 | `custom.pri`、`GuidedActionsController.qml`及模块说明 |
| 319 | 三方源码交叉复读、限定文本检查和修改/审计/工程交接同步 | 本文件、`docs/`及模块契约 |

本轮实际13个源码/工程路径有增量；限定检查范围20个源码/工程路径、18份文档。新观察器不请求流、不发命令、不抢占遥测归档槽；lastObserved/HOLD/LAND候选不表示当前健康或正式完成。原25目标/六类仍未完成，没有编译、测试、运行API/helper、改PX4、更新PR或打包。

315–319（2026-10-15）新增[本次启动的连续运行观测](custom/src/Swarm/FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

## 310–314 本次修改记录

| 编号 | 实际增量 | 主要路径 |
| --- | --- | --- |
| 310 | Windows/POSIX 原生 EOF/error/close、目录对象观察与有界 namespace 清点 | `FleetArchiveDirectoryInventory.h/.cc/.md` |
| 311 | 遥测 Writer 容量检查及 Reader 前后目录身份/文件视图接线 | `FleetTelemetryArchiveWorker.cc/.md`、`FleetTelemetryArchiveHistoryReader.cc/.md` |
| 312 | 封存事件 Writer 容量检查及 Reader 前后目录身份/段视图接线 | `FleetEventArchiveWorker.cc/.md`、`FleetEventArchiveHistoryReader.cc/.md` |
| 313 | 两个 native catalog 门面核真实完成旗标、Codec 固定23错误与工程登记 | `FleetTelemetryArchive.cc/.md`、`FleetPersistentEventStore.cc/.md`、`FleetTelemetryArchiveCodec.cc/.md`、`custom.pri` |
| 314 | 三方源码复读、限定文本检查、路线图/审计/工程师交接同步 | 本文件、`docs/`及模块契约 |

实际源码/工程增量10个路径；限定检查覆盖19个源码/工程路径与21份文档，包含未跟踪文件的直接文本检查。编号不是25个产品目标的完成数；原完整目标继续，尤其 P0 最终启动许可、完整执行/完成归属仍未收口。本批未编译/测试/运行 helper、更新 PR 或打包。

310–314（2026-10-15）新增[错误可区分的原生目录清点](custom/src/Swarm/FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

## 305–309 本次修改记录

| 编号 | 实际增量 | 主要路径 |
| --- | --- | --- |
| 305 | 共享严格Header19/Footer31、实际扫描镜像与readPolicy4 | `FleetTelemetryArchiveCodec.h/.cc/.md` |
| 306 | parentless只读目录/整文件prefix页、source/序号校验和重开hash | `FleetTelemetryArchiveHistoryReader.h/.cc/.md` |
| 307 | Native actualcatalog/每文件filter+位置+hash游标及双终结取消寿命 | `FleetTelemetryArchive.h/.cc` |
| 308 | 最新raw页native-only、固定公开6键row与pri/controller接线 | `FleetTelemetryArchive.h/.cc/.md`、`custom.pri`、`GuidedActionsController.qml` |
| 309 | 三方限定静态复读、修改清单/路线图/审计/交接同步 | 本文件、`docs/`与模块契约 |

实际业务/工程增量8个路径；限定检查另含原writer/core登记与来源契约。只读源码/文本，不运行任何日志/helper/编译/测试，不改PX4/PR/包；编号不表示25目标已完成。

305–309 新增[遥测只读历史检索](custom/src/Swarm/FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

## 300–304 本次修改记录

| 编号 | 实际增量 | 对应源码 |
| --- | --- | --- |
| 300 | 真实Vehicle/PRIMARY被动接收桥、UUID/接收序号与来源退休；保留原控制/日志 | `src/Vehicle/VehicleTelemetryObserver.h`、`Vehicle.h/.cc`、core qmake/CMake头注册 |
| 301 | 固定27消息白名单/定位确认、原len payload与精确29字段JSON | `FleetTelemetryArchiveCodec.h/.cc/.md` |
| 302 | parentless有界队列/独占NewOnly NDJSON、来源scope与写入/flush分层 | `FleetTelemetryArchiveWorker.h/.cc/.md` |
| 303 | native整组明确绑定/确认、源失效停止、250ms只读状态接线 | `FleetTelemetryArchive.h/.cc/.md`、`custom.pri`、`CustomPlugin.cc`、`GuidedActionsController.qml` |
| 304 | 限定源码复读、修改清单/路线图/静态审计/工程师交接同步 | 本文件、`docs/`及模块契约 |

编号为修改记录，不是目标完成数量；本批没有修改PX4或真实构建/测试，不提交/更新PR或包。

300–304 新增[原生入站遥测记录器](custom/src/Swarm/FleetTelemetryArchive.md)：独立Vehicle pre-plugin被动入口、实际PRIMARY/Vehicle/observer绑定与退休、27消息固定Codec、parentless有界NDJSON worker及native确认/公开只读值已接线。新增默认关闭，不改旧.tlog，不请求数据流或控制；定位类另明确确认，原len payload与RX序号只观察，签名/CRC/身份不认证。文件NewOnly不覆盖或删除，入队/write/flush/durable分层，源失效停止整组并保留unknown尾部；fullTelemetryCaptured始终false，不把此子集当完整遥测或任务许可。运行/编译/测试/解析helper/设备均未执行，未改PX4/刷机/更新PR/打包；原25目标/六类不缩减。下方295–299及更早为历史批次。

295–299 新增[跨封存段只读浏览](custom/src/Swarm/FleetPersistentEventStore.md)：Sql唯一schema/身份/严格页抽取、parentless单请求Reader与Store native目录/分页/取消及public值facade已接。actual目录条目生成本catalogId，页域绑定文件/DBepoch/sealRun/filter，最多128个每段单filter游标，只0或实际next；当前页/Reader共单槽，取消与错误不提前复用，历史不回灌rules/Replay/任务/权限。目录128段/4096条目与JSON页有界，合法1MiB政策的内部预留预算已修；目录全集、全库记录hash、跨段原子快照/飞行序都不宣称已验证。仍缺全集索引/自动归档清理、完整遥测/运营集成及最终StartPermit等原六类/25目标；未编译/测试、运行SQL/目录或hash helper、改飞控/刷机、更新PR或打包。下方290–294及更早说明为历史批次。

290–294 新增[显式维护封存轮换](custom/src/Swarm/FleetPersistentEventStore.md)：实际worker协作锁贯穿排空/SQL关闭与同目录旧库保留，native确认后才请求，不自动删旧段；actual clean/sealed/finished、精确DBepoch/文件名和同源QQ/epoch核对后才沿本次明确配置建立新run。取消/退出/源丢失只退休续建，不抹掉可能已发生的封存事实；暂停序号窗口显式known/unknown及缺口摘要，不称连续无损。封存段数有界、未知文件不覆盖，hash/协作锁不认证或保证恶意并发/失电安全。仍缺跨段查询/自动归档清理、全遥测/运营集成及最终StartPermit等原六类/25完整目标；未编译/测试/运行库或锁/文件轮换、执行SQL、改飞控/刷机、更新PR或打包。下方285–289及更早记录为历史批次。

285–289 新增[默认关闭的连续诊断归档](custom/src/Swarm/FleetPersistentEventStore.md)：实际BlackBox暂时收据/同类型事件身份、固定摘要Codec、专属QtSql线程与Store门面已接pri/type/controller；仅native显式配置/确认后异步开库，默认不读取或写盘，不回录开库前历史。queue+in-flight/批次/主库页/历史结果有界，实际commit后才durable，来源/投影/入队缺口保留诊断；历史单槽直到GUI交付，不进入实时回放、规则、机队、审批或控制链。无原payload/凭据/精确GPS，哈希非签名/匿名化；clean非完整性认证，未知库与需hot-journal恢复库拒绝不重建。仍缺全遥测/长期轮换/运营集成及最终StartPermit等原六类/25完整目标；未编译/运行/测试、建库/SQL执行、改飞控、刷机、更新PR或打包。下方280–284及更早批次为历史记录。

280–284 新增[显式局部运动与航向观察政策](custom/src/Swarm/FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

275–279 新增[被动 ODOMETRY 原生账本与契约](src/Vehicle/FleetStartupOdometry.md)：原始 MAV2 解码50/reference42，Vehicle full27/reference21；实际 PRIMARY/PX4/版本与飞行来源、接收整栈、prepare前计数和token/epoch防嵌套覆盖。sticky everObservedRawFrame 不把曾断流/无效/被过滤的源当初始缺流；Readiness仅存去age的20字段参考投影，普通收包/时间/运动数字/quality分值不造成身份抖动，reset/frame/状态与已观察ABA撤销旧依据；地面READ冻结该投影，Transition前后重核实际来源。稀疏协方差NaN保留未知，扩展尾零/LOCAL_FRD/时钟不认证参考有效性或同采样。不增加传感器、不请求/启用流、不改原六通道/read34/startup32、不放宽HOME/local严格政策。源码接线与有限静态审阅，不是完整运动/转向/净空/最终StartPermit或执行完成；原25目标/六类继续，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方270–274及更早批次是历史记录。

270–274 新增[有界实际启动事务序列](custom/src/Swarm/FleetMissionStartupSeries.md)及多轮地面再审后端：最新已初始化owner不回退，逐轮冻结actual QQ/UUID/epoch/generation/hash，旧转交链精确退休后重新完整读取/已阅/两账号审批；每次新消费者保留前代，初始/终态对象不能重用覆盖。native固定历史摘要6防递归复制，完整当轮任务/新风险/真实queued事实仍保留；同一原组/记录/部署，最多8个ground child（空候选也计费），源丢失/容量满/MODE已可能排队/未排空均不淘汰或恢复授权。实际pri/type/controller已接，限定源码复核完成，未编译/运行/测试/API/飞控改动/刷机/PR或打包。容量/新部署重置、HOME有效分支/local frame/运动转向、净空/最终StartPermit/时序/执行完成及原25目标/六类仍待开发。下方265–269及更早批次是历史记录。

265–269 新增[地面再审后的分阶段启动事务](custom/src/Swarm/FleetMissionStartupTransaction.md)源码：actual8/context27、一次28→独立30（Session6/Authority10）、同UUID连续占用；独立第二实例不覆盖旧终态。已解锁成员不发ARM，但仍核实际Seq0完整journal与整组屏障；未解锁成员仅普通ARM(param2=0)，之后仅mode-only。首次Seq0前要求实际周期RAW基线/政策gap，错误实例在换generation前拒绝；后端接线且限定源码契约复核完成，无新执行按钮。本机未调用接口/编译/运行/测试、改飞控、刷机或更新PR/包。最终StartPermit/净空/完成、第二事务再次终止后的实际owner重审路线、HOME有效分支/local frame/运动转向及原25目标/六类仍待开发；旧第一终态不能代替第二事务事实。下方260–264及更早批次为历史记录。

260–264 新增[地面再审启动依据一次接管](custom/src/Swarm/FleetMissionGroundStartupAuthority.md)源码：实际10来源/原始context27、完整新RAW/全风险/记录/typed当前地面状态进入新owner，旧22一次移交Session独立28（6字段返回真实remaining）、Ledger10字段接管。旧账本退休不盲撤新owner，原最早READ START/两身份期限只缩短，独立实时查身份/WTS/源与整组占用；public不泄露SID/RAW，所有执行/启动/飞行许可false。实际后端/pri/type已接，本批限定源码契约静态收口已完成，无读取/ARM/MODE或UI执行按钮。真正分阶段模式消费者、HOME分支/local frame/运动转向、净空/最终StartPermit/完成及其它六类/原25项目标继续；未编译/运行/测试、调用接口、改飞控、更新PR/源码包/安装包。下方255–259及更早批次为历史记录。

255–259 新增[地面再审独立账号条件意图](custom/src/Swarm/FleetMissionGroundStartApproval.md)源码：实际新 Review20/九对象/冻结整组与 armed 状态、完整最新记录/风险/新 RAW，真实两账号新提示和 Session 独立22证明，一次消费/撤销及 native14 grant；public27 receipt 无 SID/RAW。最早 READ START 原30秒不续，旧证明不恢复，实际提示期间主身份仍查 SID/role/WTS/旧期限；active READ与消费不放宽。实际 backend/pri/内部类型已接，本批已完成限定源码契约静态收口，没有新增执行按钮/新 startup authority/MODE续作。最终许可/净空/HOME有效分支/local运动转向/完成及其它六类/原25目标继续；未编译/运行/测试或调用接口、改飞控、更新PR/源码包/安装包。下方250–254及更早批次为历史记录。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](custom/src/Swarm/FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

日期：2026-10-14（最新源码增量；未构建/测试）

本文件是工程师交接时的快速索引，不是独立的发布说明，也不代表当前代码已经编译、仿真、刷机或放飞验收。完整清单见 [`docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，静态审计见 [`docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。本批次没有修改 PX4 飞控仓库。

最新 234–238：新增启动前整组机载任务的显式 RAW INT 只读核对及后端/权限/面板接线。原生真实 scope/操作/PRIMARY、完整规范载荷和最早 READ START 期限进入新风险审阅，旧审批先撤销；全组匹配不等于远端原子快照、启动许可或任务完成。下方“最新增量”旧标题为对应历史批次，不代表当前实现仍只有 212–216；完整 25 项/六类剩余工作未收口，未编译/运行/测试或更新 PR/安装包。

## 变更范围

- `custom/src/Swarm/`：事务、健康门禁、能力发现/护照、角色与能量调度、任务编译/审阅、空域冲突、连续性、交接、数字孪生、故障预演、黑匣子/回放、证据链、报告、维护护照、运营记忆、事件规则、模板和扩展目录等地面站模块。
- `custom/res/Merivus/`：Fleet OS 面板、地图/指挥中心共享状态、意图审阅、连续性/交接/数字孪生/目录/运营记忆展示，以及统一人工控制确认链路。
- `custom/custom.pri`、`custom/qgroundcontrol.qrc`、`custom/res/Merivus/qmldir`、`custom/src/CustomPlugin.cc`：源码注册、QML 资源和类型注册；`custom.pri` 同时恢复 Custom PX4 Firmware/AutoPilot 插件工厂的源码与头文件注册。
- `src/Vehicle/Vehicle.cc/.h`、`src/MissionManager/PlanManager.cc`、`src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc`：MAVLink ACK 超时隔离、Mission 消息来源/目标校验和兼容错误映射；不改变 PX4 飞控源码。
- `src/FlightDisplay/`、`src/Vehicle/`、`src/MissionManager/`、`src/FactSystem/`、`src/Joystick/`、`src/QmlControls/`、`src/AutoPilotPlugins/` 等 QGC 通用入口：补充确认快照、链路/维护态门禁、参数/命令输入校验和异步事务边界；不改变 PX4 飞控源码。完整范围以工作树 `git status --short` 和补丁清单代表性路径为准。
- `src/FlightDisplay/FlyViewMissionCompleteDialog.qml`：恢复任务按钮改走 `confirmAction(actionResumeMission)`，避免直接调用缺少冻结目标/确认上下文的 `executeAction()`。
- `docs/`：路线图、目标矩阵、架构/安全边界、风险登记、静态审计和工程师交接说明。

## 最新增量（2026-10-14；212–216）

新增地面上传二次确认/逐机状态/只读检查面板，冻结实际编码项数、目标和
操作者/策略快照；原生上传每步也核身份/权限变化，未实现的额外审批不造
默认放行。完整历史回读仅提供新独立风险审阅来源，30秒“已阅”不恢复旧
批准、不解锁或启动。所有变更未编译/运行/测试，不更新PR或安装包。

## 上轮增量（2026-10-14；208–211）

新增六通道真实飞行状态观测、原生启动前置条件检查及任务序号/当前缓存对照。
实际起飞高度提升、当前位置垂直段与后续连接显式显示，未解锁/未知/过期
证据不能显示为启动前置条件已满足；遥测序号、任务模式、LAND/HOLD均不当
本任务启动/完成证明。部署按当前冻结实例提供主动只读检查与短时值快照，
不复用旧审批，不新增启动/解锁/UI按钮。未编译、运行、测试或更新PR/安装包。

## 上轮增量（2026-10-14；204–207）

新增真实原始 AUTOPILOT_VERSION 观测与有界固件适配工程声明目录；按本地
Git5/Vendor3 wire 布局匹配，不将 16 位显示误当纯 Git 哈希。版本源首次锚定、
内容/UID/来源变化同步撤销旧参数证据；原生审阅/目录/归档/上传门禁与有界
黑匣子摘要已接线。声明匹配不是固件或二进制认证，不恢复启动审批；所有部署
仍不可执行，未编译/运行/测试、提交或更新 PR/安装包。

## 上轮增量（2026-10-14；199–203）

固定十二项参数的真实接收来源、单调年龄、内容 revision 和 manager epoch
进入原生只读观测；缓存、FTP、文件、写入、刷新和链路变化不得冒充实时值。
新增 `FleetMissionVehicleEvidence`，将完整参数内容哈希与来源连续性指纹分开，
明确区分参数可用、固件缓存信息与未获得的执行认证。审阅、归档、人工批准和
部署继续步骤已接入原生证据核对；Fact direct signal 的生命周期保护同步补齐。
所有部署仍不可执行，没有新增启动/解锁或上传 UI，未编译/运行/测试，也未
更新 PR、源码包或安装包。编号是修改记录，不是产品目标完成计数。

## 本轮重点

1. 指挥中心高度、速度、`MPC_TKO_SPEED` 参数卡统一进入 `GuidedActionsController.requestControlCommand()`。
2. 标准 Guided 高度/速度滑块冻结目标 UAV，并在执行前重新检查链路、飞行状态、健康门禁和数值范围。
3. 手动参数请求使用唯一 `requestId`、人工确认和审计事件；`sent` 与真实飞控 ACK 明确分离。
4. 起飞速度 setter 接受后最多轮询 5 秒参数回读，记录 `command.acknowledged` 或 `command.ack_timeout`。
5. 确认取消、过期快照、复制确认对象和动作码/参数类型不一致时，均清除或拒绝，不触达 `Vehicle API`。
6. 标准任务启动/继续/多机启动统一进入 C++ `mission_start` 事务；临时 Mission 上传完成后必须经过 `Starting` 阶段，等待启动 ACK 或 Mission 模式遥测。
7. FleetRegistry 的位置健康证据兼容有效全球坐标和已收到的本地位置 x/y/z 遥测，并记录证据来源；不把本地证据误报成全球 Mission 坐标能力。
8. 标准任务启动逐机核对机载任务项数量/当前索引（按固件 Home mission item 规则与 QML 序号对齐），并输出 `snapshotFingerprint` 审计字段；创建事务和延迟发送前均复核，冻结快照不一致的目标跳过。
9. 标准/临时任务延迟与上传完成回调复用 `_vehicleMissionStartReady()`；同一车辆的普通任务/飞行事务建立互斥，RTL/Land/formation ABORT 保留安全逃生例外。
10. `ManagedTransaction` 增加逐机 `dispatchStarted/dispatchEpoch`：未真正交给 Vehicle/MissionManager 前，ACK/模式遥测不能完成事务；Land/RTL 同机 safety escape 使用唯一 owner，避免重复安全事务和 ACK 广播。
11. 临时 Mission 的上传与 `MAV_CMD_MISSION_START` 使用分离的逐机 dispatch 标记/epoch；上传阶段的 ACK 不再能冒充后续启动 ACK，启动前失去事务所有权会进入清理。
12. 审阅、风险、能力匹配、时间线、预演和数字孪生 QML 属性改为受控 facade/值快照；外部 QML 只能读取结果或播放时间线，不能导入、注入或替换审计/预演输入。
13. 临时 Mission 的共享 `sendComplete/removeAllComplete` 增加本地 upload/clear pending 与上传结果证据核对；无法归属或不匹配的回调只失败并脱离本地状态，同时写入 stale 栅栏阻止后续任务覆盖现场，不主动 `removeAll` 外部航线。
14. 恢复任务上传完成后由 `resumeMissionReady` 进入统一 `mission_start` 事务；数字孪生快照与时间线事件属性改为只读，并通过原子/严格入口更新。
15. Registry 能力护照补充固件/UID/电机布局/传感器位图与 camera/obstacle/range 证据；Estimator、SYS_STATUS、ESC、FTC 当前证据写入健康摘要，仍明确 observed 与 trusted baseline 的区别。
16. 收口 QML 写入口：`GuidedActionsController` 对黑匣子、事件规则、证据链、运营记忆、维护护照、链路策略、模板/扩展目录和自主策略只暴露值快照；任务交接改走以 `FleetRolePolicy.currentUserId` 为唯一身份来源的受控 facade，固定二次确认和 300 秒有效期。原始 QObject 继续由控制器/受信任后端内部使用。
17. 连续性规划器要求 estimator/position/fault evidence 明确且 `faultEvidenceAgeMs` 新鲜；未知证据只能生成 `health_unknown` 人工复核建议，不能被选为替补或中途加入机。
18. 运营记忆的布尔观测证据改为严格类型：只有真实 JSON/QVariant `bool` 才能触发链路/GNSS/能耗/异常/人工干预或估算标记；字符串 `"false"`/`"true"` 不会被隐式转换为证据。
19. 连续性规划器对车辆快照的在线/链路/健康/能力/故障布尔字段改为严格类型；字符串布尔值不会改变替补、Leader 或失联判断，显式类型错误会阻断本次规划。
20. 证据链增加单条 payload 的深度、节点数、类型长度和 512 KiB 大小上限；运营记忆/维护护照拒绝未知根字段，维护护照同时校验 `riskModel`，避免导入时静默丢字段或造成资源消耗。
21. 空域冲突检查器对全局/任务级 `returnRequired` 采用严格布尔解析；类型错误按 blocking 违规处理，不让字符串布尔值伪造返航通道要求。
22. 任务风险评估器收紧航点/覆盖/备用机计数与风险信号输入：计数关系异常、备用列表不足、未知 severity 或越界 weight 会阻断评估；非法覆盖率不会再保留为可用百分比。
23. 故障健康证据改为成对提交：`FaultToleranceManager` 拒绝仅设置 `faultHealthKnown=true` 而缺少新鲜年龄，或带非负年龄却未声明 known 的部分报告；清除 known 时同步清除旧年龄，`FleetHealthCenter` 的年龄窗口跟随可配置的 `offlineTimeoutMs`，避免旧零年龄或硬编码超时伪造当前健康。
24. `FleetCapabilityMatcher` 的车辆身份解析拒绝数字字符串、非法/小数 ID 和冲突的 `systemId/vehicleId/id` 别名，避免能力编组把用户输入静默映射到错误机体。
25. `FleetEventBlackBox` 对录入和导入的事件统一限制 type/source 长度，以及单条 payload 的递归深度（16）、节点数（50000）和大小（512 KiB）；超限事件会被拒绝，避免高频黑匣子被单条恶意/损坏 QVariant 树耗尽内存或 UI 线程时间。
26. `FleetMissionCompiler` 对 `constraints.geofenceRequired` 增加严格布尔门禁；字符串或数值类型错误会生成 `invalid_geofence_required` 阻断风险并使计划不可行，避免 `QVariant::toBool()` 将字符串 `"false"` 隐式当成开启。
27. `FleetRiskRadar` 对任务意图/参数及插件风险信号增加严格 Map、布尔、有限数值、百分比和备用机计数校验；非法输入返回 `inputValid=false`、`needsReview=true` 和阻断分数，不再依赖 `QVariant::toBool()/toInt()` 静默转换。
28. 收紧 QML 控制面：`GuidedActionsController` 对外以只读 facade 替代 raw `FleetHealthCenter`、`FleetRegistry`、`FormationPlanner`；地图的 `swarmController` 仅保留临时航点查询/下发与完成信号，原始 SwarmController 不再从公共属性泄露。恢复任务经统一确认和冻结目标路径进入上传/启动流程。
29. 收紧 `FleetRoleAllocator`：车辆 `systemId/vehicleId/id` 逐字段严格校验并要求别名一致，拒绝数字字符串、空值、分数/非有限 ID；`leaderRequired` 与角色计数拒绝隐式布尔/整数转换。非法任务参数只返回诊断和拒绝项，不生成 assignments/reserve，避免把损坏输入显示成可用编组。
30. `FleetIntentTask::approve()` 现在要求预演 `feasible` 是真实 `bool` 且为 `true`，并检查内嵌 `FleetRiskRadar` 的 `inputValid`；风险输入类型/范围非法时即使能力匹配可行也不能进入批准状态，避免损坏风险证据被人工审批流程静默放行。
31. `FleetTaskTemplateRegistry::validateTask()` 收紧 `allowUnverified`、车辆在线/能力布尔值、身份别名和任务时长输入；数字字符串、冲突 ID、字符串布尔或非有限数值进入可解释拒绝原因，不再依赖隐式 QVariant 转换。
32. `FleetEventRuleEngine` 收紧条件 schema：路径、条件数量、列表长度和文本长度有上限；比较数字必须是有限真实数值，`exists` 期望值必须是真实布尔，字符串数字/布尔不会改变规则匹配结果。
33. `FleetCapabilityDiscovery` 不再把非字符串能力静默转成标签，增加 observed/verified 能力区分、构型类型错误和发现时间校验；`FleetCapabilityMatcher` 增加 `requireVerifiedCapabilities` 门槛，安全任务可拒绝只有 heartbeat 证据的能力标签。
34. Guided 动作确认链增加一次性 `confirmationToken`：`confirmAction()` 在动作专属校验后冻结 token 与 `actionData` 身份，通用 `GuidedActionConfirm` 回传原 token；期望 token 保存在控制器/确认组件的 lexical child-object 状态中，不再作为公开 property 提供读取。`executeAction()` 在任何 Vehicle/SwarmController 调用前严格核对 token、actionCode 和冻结对象，并在通过后立即消费。缺 token、复制 payload、过期或重复回调均记录拒绝并阻断；RTL/Land/急停仍保留原有安全逃生门禁例外，但不能绕过人工确认。
35. Fleet OS 意图审批绑定稳定的健康快照指纹：排除 `lastSeenUtc`/连续增长的年龄字段，仍纳入在线、健康证据、故障、评分和状态变化；健康轮询只在风险相关快照真正变化时失效旧审阅，避免每秒刷新导致无法点击批准。调用方传入的外部 `healthReports` 不再成为隐藏审批输入，和当前 FleetHealthCenter 快照不一致会被标记为阻断复核。
36. `FleetRegistry::_mergeDescriptor()` 支持独立的 `configurationClear=true` 更新：即使传输不重复旧 configuration map，也会清除配置图、观察指纹和 metadata 别名；`VehicleCapability` 注释同步说明受控的 `trusted_manifest`/`local_approval` 两类审批来源。
37. `SwarmMissionOrchestrator` 将成员 ID 规范化为 1--255 的有限整数；创建或重分配时只要出现字符串、布尔、小数、越界、非有限或重复成员，整次操作即拒绝，不再静默丢弃成员或把无效 Leader 替换成首个成员。空成员列表仅允许用于未分配的草稿任务。
38. `CustomPlugin` 将可变任务/健康/命令 QObject 注册到 `MerivusInternal 1.0`，`GuidedActionsController` 显式导入该内部 URI；公开 `Merivus 1.0` 面板只消费 facade/值快照。该 QML 模块边界用于减少旁路，不替代 C++ 权限、签名或进程隔离。
39. 连续性建议 ID 绑定任务 revision 与稳定 Fleet 快照 SHA-256（成员/在线/健康/链路/电量/能力/构型等风险字段），重新规划后机队证据变化会使旧 UI 建议无法按同 ID 重新取回，避免操作员确认旧快照却写入新分配。
40. 恢复任务启动改用确认时冻结的 `resumeMissionIndex`：索引保存在控制器 lexical child-object 状态中，上传完成后的实时 `currentMissionIndex` 不再覆盖确认索引；索引在上传前和启动前均做有限整数/任务范围校验，非法索引记录 `mission.resume_start_blocked` 并阻止进入 Vehicle/Swarm 命令路径。
41. 标准任务启动新增确认时的逐机 `missionStartSnapshot()` 与 fingerprint 锁：`executeStartMission()` 在创建事务和延迟下发前均核对锁定指纹，MissionManager 在确认后被替换、改写或清空时阻断启动并保留审计证据。
42. 连续性规划器只对 pending/running/paused 任务生成变更建议，校验显式 Leader 必须属于任务成员，并支持任务级 `allowMidMissionJoin`、`requireVerifiedCapabilities` 与 `requireConfigurationBaseline` 门禁；Leader 接替应用会以 revision 保护同步更新任务 Leader 字段，故障证据仅在 fresh/stale 离散边界变化时刷新建议指纹。
43. `FleetContinuityPanel.qml` 仍保持只读，但按严重度稳定排序显示最多六条建议，补充 message/reason、任务 revision、Fleet 指纹前缀、受影响/候选 ID，并提示剩余未展开数量；没有新增任务或飞行命令写入口。
44. 连续性候选不再依赖车辆发现顺序：按构型 baseline、verified 能力、已知电量/较高余量、已知链路/较高质量和稳定 ID 做确定性排序，结果仍只是人工复核建议。
45. 多机标准任务启动不再复用当前活动机的任务项数量/当前索引；多机入口改由确认时逐机 `snapshotFingerprint` 锁定完整 MissionManager 内容，单机入口仍保留数量/索引与指纹双重校验。
46. `CommandTransaction` 的重试只允许真实发起过且仍有可重试成员的命令；运行中调整超时策略会同步重置计时器，过期队列回调会重新核对当前 deadline，序列化结果补充 `terminal` 字段。
47. `FleetHealthCenter` 聚合与任务门禁改用严格布尔/有限数值读取；字符串布尔、字符串电量/链路、非法故障年龄或损坏健康分数会进入 unknown/block，不再由 `QVariant` 隐式转换成健康证据。
48. 健康状态改为证据优先的保守合并：来源的 `critical/degraded/offline` 只能把状态升级，来源的 `healthy` 不能覆盖严格字段推导出的 unknown/critical/offline；原始来源状态保留在 `reportedHealthState`。
49. `FleetMissionCompiler` 的车辆/备用机 ID 统一只接受真实 1–255 有限整数，拒绝字符串 ID、数字字符串、分数、越界值和 `vehicleId/systemId/id` 冲突，使编译计划与 Registry、能力匹配和任务编排器的身份契约一致。
50. `FleetHealthCenter::_evaluateGate()` 不再静默丢弃混合目标中的非法 ID；任何字符串、分数、越界或别名冲突都会生成 `invalid_vehicle_id` 阻断并返回 `invalidSelection`；没有安全 Leader 时通过 `globalBlocking` 强制拒绝，不把无效 Leader 当作可放行状态。
51. `FleetMissionCompiler` 在提供 fleet 快照时强制校验所有 fleet/assignment/reserve ID；未知机体和非法 fleet 条目进入 blocking unmet/risk，只有未提供 fleet 的 planning-only 预览才跳过成员存在性检查。
52. 备用机列表不再静默丢弃非法项；`reserveVehicleIds/reserve` 中任何字符串、分数、布尔、越界或冲突条目都会生成 `invalid_reserve_vehicle_id` blocking unmet/risk，即使未设置 `reserveCount` 也不能伪装成可行计划。
53. `FleetMissionCompiler` 对 `geofenceRequired=true` 现在验证实际非空边界几何（至少三点或有效 bounds），空 Map、null、字符串或不完整边界进入 `geofence_missing` blocking，而不是仅因字段存在就放行。
54. assignment 角色现在只接受真实字符串且必须属于 `leader/scout/relay/responder`；数字、布尔、空角色和未知角色进入 blocking unmet/risk，不再生成看似可行的未知角色计划。
55. `FleetHealthCenter` 在自身门禁边界再次限制电量/链路为 `-1..100`；越界有限数值进入 `health_input_range_invalid` blocking 或 unknown，不依赖上游 Registry 校验才能安全。
56. 编译器契约文档同步闭合 fleet、严格角色/Leader 唯一性、reserve 非法项、重复 fleet ID 和真实 geofence 几何边界，避免文档继续暗示 fleet 仅作报告用途。
57. 编译计划显式输出 `fleetProvided`/`planningOnly`，让 UI/审阅层区分闭合机队校验和未提供 fleet 的 planning-only 结果。
58. `FleetMissionCompiler.h` 的接口注释同步说明非空 fleet 是闭合成员快照，避免调用方继续按“仅报告用途”使用编译器。
59. `FleetRegistry` 与 `FaultToleranceManager` 为 fault evidence 保存地面站观测锚点，并在每次健康刷新时推进 `faultEvidenceAgeMs`；一次 `age=0` 报告在只有心跳、没有新故障评估时会自然过期，不再永久充当新鲜证据。
60. `FaultToleranceManager` 删除成员后立即重建 Leader/拆分/降级建议，`setLeaderVehicleId()` 拒绝未注册的幽灵 ID，`clear()` 即使报告集合已空也会清理残留 Leader 和建议状态。
61. `FleetRegistry`/`FleetCapabilityDiscovery` 对 `faultHealthKnown=false` 的 null 年龄不再依赖 QVariant 的 null-to-zero 转换；`configurationClear=true` 与同时携带新 configuration/fingerprint 的矛盾更新会被原子拒绝。
62. 故障年龄现在必须与 `faultHealthKnown` 成对出现；age-only（包括 `-1`）更新会被拒绝，避免产生 `known=true + age=-1` 的矛盾快照；FTC 新证据在缺少时间戳时以本次报告接收时刻为锚点。
63. `FaultToleranceManager` 在自动选择安全候选替代不健康 Leader 时，先通过统一 `degradationRecommended("reassign")` 发出旧 Leader/候选 ID 和人工复核原因，再更新只读 Leader 属性，避免 UI 只收到属性变化却没有连续性事件。
64. `FleetHealthCenter` 将实际生效的 `offlineTimeoutMs` 带入逐机值快照，连续性/风险下游不再因自定义健康窗口缺少字段而回退到硬编码 3000 ms。
65. `FleetMissionContinuityPlanner` 的候选快照解析也读取每机 `offlineTimeoutMs`，fresh/stale 判断与 FleetHealthCenter/FTC 使用同一配置，不再在第二个解析路径硬编码 3000 ms。
66. 健康审阅 fingerprint 纳入有效 `offlineTimeoutMs`；操作员改变故障证据窗口时，即使当前 healthState 尚未跨阈值，也会让旧意图审阅失效并要求重新确认。
67. FleetHealthCenter 汇总快照也公开实际 `offlineTimeoutMs`，无车辆或仅有摘要的审阅路径仍能感知健康策略窗口变化。
68. Registry 健康刷新在 fault evidence 年龄推进时发布值快照更新，操作员不会看到停在旧 age 的诊断；删除当前 Leader 时无安全候选会明确发出 `hold/no_safe_leader_candidate`，不只改变 Leader ID。
69. `FaultToleranceManager` 的全局恢复判断从“仅 Leader 离线”扩展为“Leader 不满足完整安全候选条件”；在线但低电量、估计器/定位异常、故障或故障证据过期且无替补时，也会发出 `hold/no_safe_leader_candidate`。
70. Registry/FTC 对“离线后只带在线心跳重连”按证据通道清除缺失的旧电量、链路、估计器、定位和 fault-health 样本；明确 `hasFault=true` 仍保守保留，避免重连活性被误读为健康证明。
71. `FleetCapabilityDiscovery::buildDescriptor()` 现在对自身输出执行一次完整 `validateDescriptor()` 自审计；`SwarmController::_refreshFleetRegistry()` 在加入 QGC 实时健康字段后再次校验完整 live descriptor，发现错误会保留 `discoveryErrors` 并标记 `discoveryState=invalid`，随后仍交给 Registry 的原子 ingress 校验，避免能力发现结果只定义不落地或被静默当成可信护照。
72. Guided 单机 Takeoff/Land/RTL 在确认时也冻结为单元素目标列表，执行侧不再回落到可变的 `activeVehicle` 直调；确认 payload 增加 JSON 指纹，数组原地修改会使一次性 token 失效。
73. `executeQueuedGoto()` 遇到混合有效/无效航点时整笔拒绝，不再静默缩短路线；Formation command timer 增加 phase/deadline 门禁，陈旧 queued timeout 或同步 ACK 推进后的旧 start 不会直接中止新阶段。
74. 任务/降落/返航完成遥测保存 dispatch 前飞行模式与时间戳，已处于目标模式的旧遥测不能冒充本次事务完成；临时 Mission 清理成功回调还要求机载 MissionManager 为空，否则保持 stale 并失败关闭。共享 MissionManager 信号没有操作 token，最终归属仍需工程师验证。
75. `VehicleCapability::fromVariantMap()` 在首次注册时保留能力发现的 `verifiedCapabilities`、证据状态、构型证据来源、发现状态/错误、发现时间和位置证据来源，并由 `toVariantMap()` 重新暴露这些字段保证往返无损；`FleetRegistry` 同时严格校验这些顶层证据字段的类型和时间戳，避免初次接入丢失 `discoveryState=invalid` 或把损坏护照转成可用能力。
76. `FleetCapabilityMatcher::capabilities(..., requireVerifiedCapabilities=true)` 现在只返回显式 verified 能力，不再把运行时 `canUse*` 观测布尔追加成“已验证”能力；普通规划模式仍可使用观测能力，避免 verified-only 任务被低信任遥测绕过。
77. `FleetMissionContinuityPlanner` 的 verified-only 替补筛选与 `FleetCapabilityMatcher` 对齐：只读取显式/metadata verified 标签，不再用 `canUse*` 观测绕过；连续性 fleet 指纹同步纳入 verified/discovery 证据字段，证据变化会使旧建议失效。
78. `FleetCapabilityMatcher::plan()` 对车辆 `capabilities/verifiedCapabilities`（含 metadata fallback）执行严格 String/StringList/List<String> 校验；损坏能力证据进入 `invalid_capability_evidence` 拒绝列表，不再依赖 QVariant 的宽松转换。
79. `FleetMissionContinuityPlanner` 的 fleet 快照索引也拒绝损坏的能力列表/metadata verified evidence，避免坏能力快照进入替补候选或仅以 `inputValid` 旁路继续规划。
80. `FleetCapabilityMatcher::plan()` 对任务 `requiredCapabilities/capabilities` 也执行元素级字符串校验，数字/布尔/混合列表使 `parametersValid=false`，不再被自动转换成可匹配标签。
81. `FleetRoleAllocator` 新增 `requireVerifiedCapabilities` 策略并与 Matcher/ContinuityPlanner 共用可信边界；角色能力筛选只用 verified 标签，车辆/角色能力列表类型错误进入 `invalid_capability_evidence` 或无效任务参数。
82. `FleetTaskTemplateRegistry::validateTask()` 支持模板/任务级 `requireVerifiedCapabilities`，模板能力审查只使用 verified 标签；任务/车辆能力列表和策略布尔严格校验，坏证据进入阻断原因。
83. `FleetCapabilityMatcher` 在任务参数无效时不再填充候选/分配列表；`selectedVehicleIds/reserveVehicleIds/assignments` 保持为空，避免 `feasible=false` 之外仍残留可被误用的选择结果。
84. 多机列表的启动任务、单机暂停和单机 RTL 改走 `GuidedActionsController` 确认入口；直接写 `Vehicle` 的解锁和接管模式按钮改为禁用并提示使用舰队控制面板，避免 UI 旁路健康/目标冻结策略。单机目标对象现在可被控制器冻结并在执行时重新解析。
85. `actionMVPause` 在确认时冻结完整车辆 ID 快照，执行时只遍历该快照并拒绝缺失目标；不再按确认后的实时车辆列表暂停后来加入的机体。
86. 多机列表的单机启动/继续任务在显式目标快照下不再依赖当前活动机的 `showStartMission/showContinueMission` 可见性，确认文案也显示冻结的 UAV 列表；无目标时仍保留原有单机门禁。
87. 静态入口审计补充记录：`GuidedActionList` 的自定义 MAV_CMD、Joystick 可配置 MAV_CMD、Mavlink Console、FlightModeMenu、PlanView 上传/清空和 `Vehicle` 的 Q_INVOKABLE 仍是维护/兼容路径，尚未伪装成 FleetHealth/CommandTransaction 权限边界；交接文档已列为工程师后续隔离项。
88. `PlanView.qml` 的任务上传/清空现在冻结 active Vehicle 对象和 ID，并在确认前/延迟回调前重新检查目标、链路和 FleetHealthCenter 门禁；飞行完成清理对话框同样拒绝活动机切换、链路断开或健康门禁未就绪的直接清理。
89. Mission resume 的无 token 异步信号增加地面站侧 request identity：失败重试只接受仍存活的 `requestId/vehicleId/resumeIndex/itemCount` 上下文；无上下文的 ready/fail 信号被忽略，ready 阶段还要求当前任务项数量与确认快照一致。底层 MissionManager 仍需工程师补充真正 operation token。
90. `GuidedActionList.qml` 的标准自定义动作按钮不再直接调用 `CustomAction.sendTo(activeVehicle)`；现在经 `GuidedActionsController.confirmCustomAction()` 进行目标/链路/健康检查、人工确认、一次性请求身份和发送审计，再调用自定义动作。底层 `CustomAction::sendTo()` 仍是可被其它 QML 调用的维护接口，尚未成为 C++ 权限边界。
91. `FlightModeMenu.qml`、`ModeIndicator.qml` 和 `FlightModeMenuIndicator.qml` 在 MERIVUS 控制器存在时改走 `requestFlightModeChange()`；普通模式需健康门禁与人工确认，RTL/SmartRTL/Land 保留安全逃生例外并仍冻结目标/记录审计。无 MERIVUS 控制器的通用 QGC 场景保留原兼容回退。
92. `CustomAction` 增加 `safetyClass`（`inflight_safe`、`disarmed_only`、`maintenance_only`）元数据和 C++ 发送侧飞行态/链路检查；未知命令默认 `disarmed_only`，内置相机捕获动作显式标为 `inflight_safe`。控制器确认与执行阶段再次校验安全等级，发送返回值失败时记录拒绝而不是伪造 `command.sent`。
93. 自定义 Joystick MAV 命令配置增加 `safetyClass`（现有 `MAV_CMD_USER_1/2` 默认 `disarmed_only`），`JoystickMavCommand::send()` 在未连接、链路丢失、已解锁或飞行中拒绝发送并返回失败；Mavlink Console 的 C++ `commandAllowed()`/`sendCommand()` 同样只允许已连接、未解锁且未飞行时发送，QML 键盘、粘贴和发送按钮统一拦截失败并提示操作员。该策略仍不等于 MAV_CMD 白名单、角色权限或飞控 ACK。
94. `CommandTransaction` 车辆身份键不再使用 `QVariant::toString()` 宽松转换；事务目标和 ACK 回填只接受真实有限整数 1–255，并以规范化十进制键去重，拒绝 `"01"`、布尔值和任意文本别名，和 Registry/Health/任务编排的身份契约保持一致。
95. PlanView 的顶部工具栏 Upload 增加 fail-closed 入口检查，统一调用已有 `globals.planMasterControllerPlanView.upload()` QML 包装；PlanView 面板按钮继续调用同一 `PlanMasterController` QML 包装，该包装复用目标冻结、FleetHealth 门禁、任务完整性和 `sendToVehiclePreCheck()`。PlanView 销毁时清理全局 controller 引用，避免工具栏保留旧对象。
96. MERIVUS Guided legacy 单目标动作增加执行时链路边界检查：确认 token 通过后再次要求目标仍在机队、初始连接完成且 `vehicleLinkManager.communicationLost=false`；RTL/Land/急停仍可绕过健康评分，但不能绕过目标/传输有效性。
97. PID 调参页的自动模式切换和 `FlightModeDropdown` 在 MERIVUS 控制器存在时改走 `requestFlightModeChange(mode, vehicleId)`，统一经过确认/目标/健康门禁；无控制器的通用 QGC 场景保留原兼容回退。
98. `RCToParamDialog` 的 `sendParamMapRC` 写入口增加维护态门禁：目标必须完成初始连接、链路正常、未解锁且未飞行，输入数值必须有限；MERIVUS 上下文还必须通过 FleetHealth 门禁，失败只提示并不发送参数。
99. Guided 控制器与指挥中心统一把缺失 `vehicleLinkManager` 视为链路未知/不可发送，而不是仅检查 `communicationLost`；按钮可用性、链路文案和执行前门禁保持一致，避免“无链路管理器却显示已连接”。
100. PX4 CameraComponent 的 Apply and Restart 增加重启写入口门禁：必须完成初始连接、存在正常链路管理器、未解锁且未飞行；按钮禁用或提示阻止后不调用 `rebootVehicle()`。
101. 参数维护入口收口：`ParameterEditorController::sendDiff()`/两个 reset API 增加 C++ 侧连接、链路、未解锁/未飞行和活动目标一致性门禁；参数文件 vehicleId 缺失、混合或与当前机体不一致时 fail-closed。`ParameterDiffDialog` 显示阻止原因并在写入失败时保持对话框；`ParameterEditor` 的加载、重置、RC 映射清理和重启入口增加同一维护态检查，单参数编辑/恢复默认也在提交前检查。该层仍未执行真实编译或 MAVLink ACK 验证。
102. 兼容版 `src/FlightDisplay/GuidedActionsController.qml` 的 `executeAction()` 增加执行时传输门禁：目标必须完成初始连接、存在 `vehicleLinkManager` 且未丢链路；多机启动/暂停在发送第一条命令前预检全部车辆，避免一个目标掉线后部分执行。失败显示阻止原因并返回 `false`，未知动作也显式失败；未改变 MERIVUS 自定义控制器路径。
103. 兼容版 Guided 恢复任务入口补齐目标冻结后的确认链：`actionResumeMission` 不再在 `confirmAction()` 中无条件早退，而是显示正常确认框并复用执行时链路/活动目标检查；单机动作成功、未知动作或多机预检失败后清理一次性 `_confirmedVehicle`，避免旧确认状态残留。Mission complete 对话框说明同步修正；仍需工程师验证 MissionManager 恢复上传/ACK 和 QML 生命周期。
104. PX4 `SensorsSetup.qml` 的校准完成后“Reboot Vehicle”快捷入口增加维护态门禁：目标必须完成初始连接、存在正常链路、未丢链路、未解锁且未飞行；按钮禁用并在竞态点击时再次拒绝，避免旧校准弹窗重启错误或飞行中的机体。Mission complete 的任务清理同样补齐 `initialConnectComplete`/`vehicleLinkManager` 二次检查；仍需工程师验证重启/清理回执和页面生命周期。
105. `MissionManager::generateResumeMission()` 增加空任务/空生成结果和当前项索引边界保护，避免陈旧或直接 QML 恢复请求在空列表上解引用 `-1`/越界；短任务仅含一个条目时不再假定存在第二个 Home 项。编队回滚 ABORT 还把被跳过的成员纳入失败集合，未实际下发 ABORT 的成员不会被误报为本地完成；重复停止在 ABORT 进行中返回原事务，不覆盖 generation-bound ACK handler；仍需工程师验证空任务、DO_JUMP、ABORT 拒绝/超时和 ACK 时序。
106. APM 维护重启入口（`APMSensorsComponent.qml`、`APMSafetyComponent.qml`、`APMPowerComponent.qml`）统一增加初始连接、链路管理器、通信状态、未解锁/未飞行门禁，按钮与点击回调双重检查；通用 MotorTest 与 ParameterManager 已有 C++ 维护门禁，工程师仍需验证角色权限、ACK/回执和其它旁路。
107. 编队事务取消旁路关闭：`SwarmController::cancelTransaction()` 和 MERIVUS `GuidedActionsController.cancelTransaction()` 拒绝直接取消 `formation.*` 地面事务，要求走确认后的 `endFormationSession()`/MAV_CMD_USER_4 ABORT，避免只取消地面状态而留下机载编队状态；拒绝事件写入黑匣子。
108. 意图式任务审批增加机队快照指纹：预演记录车辆 ID、在线/健康状态、构型指纹/证据来源、发现状态、能力/已验证能力、传感器/电机布局和角色等稳定证据；审批时必须与当前 FleetRegistry 快照一致，外部传入的旧/改装后机队列表不能继续沿用旧审批。Registry 变化会使审批 token 失效；仍需工程师验证异步 registryChanged 与真实能力证据更新时序。
109. Mission resume 异步链继续收口：恢复上下文保存 `issuedAtMs` 并限制 120 秒有效期，ready/fail/执行阶段都要求 request identity、目标和任务项快照一致；`confirmAction()` 对恢复条件不可用、索引无效或缺少身份的 direct QML 调用 fail-closed，不建立确认上下文。该层仍不能替代 MissionManager 的协议 operation token。
110. 通用与 APM MotorTest 入口增加维护态门禁：电机滑杆只有在初始连接完成、存在正常链路、未丢链路、未解锁且未飞行且 API 存在时可用；滑杆回调再次检查，竞态失效会归零而不调用 `motorTest()`。Vehicle C++ `motorTest()` 也复核维护态与参数边界；工程师仍需验证角色权限、ACK、回执和真实硬件保护。
111. 恢复任务生成器释放被过滤的临时 `MissionItem`：重复相机/ROI 指令和非时间触发项从中间列表移除时同步删除对象，避免长时间重复恢复任务造成内存泄漏；不改变任务筛选语义，仍需工程师验证 `DO_JUMP`、异常任务和真实上传回执。
112. 交接审计明确底层 ACK 归属缺口：`Vehicle::mavCommandResult` 和 `PlanManager` 的共享 MAVLink 回调没有协议级 operation/session token；本轮只保留地面站 epoch、状态互斥和后置证据，未声称能隔离迟到旧 ACK。工程师必须在 Vehicle/PlanManager 层补 generation/token 或 ACK quarantine 后再放行。
113. `Vehicle` 对无响应终态增加同组件/命令的单调时钟 ACK quarantine：短时间内拒绝重发并忽略迟到 ACK，保留旧 API/信号；`REQUEST_MESSAGE`、`SET_MESSAGE_INTERVAL`、`DO_SET_MODE`、`DO_MOTOR_TEST` 和 `COMPONENT_ARM_DISARM` 作为同步链路/安全逃生例外不隔离；孤立的 `SET_MESSAGE_INTERVAL` ACK 不再推进流配置。该机制只降低迟到 ACK 串线风险，不构成协议级 operation token。
114. `PlanManager` 增加兼容的本地 mission operation identity：读、写、清空和 ArduPilot guided 写入在开始时生成单调 `quint64`，完成/错误时同时保留旧 bool/错误信号并发出带 `operationId` 的新信号；`MissionManager::generateResumeMission()` 的上传也因此可被识别。`PlanMasterController` 的 Mission 读写串联改用带 identity 的完成信号，并只接受自己发起的 operation；`SwarmController` 的临时 Mission 上传/清理改用该 identity 归属回调，并继续要求机载任务快照后置证据；该 ID 只存在于地面站，不是 MAVLink 线上的 token，仍需工程师做延迟旧 ACK/并发 read-write-removeAll 回归。
115. Mission 规划控制链继续使用 operation identity：`MissionController` 不再由旧的无身份 Mission 完成/恢复信号直接刷新 QML 状态，而是校验 `operationStarted` 与对应 `*WithOperationId` 回调；`PlanMasterController` 对 Mission、GeoFence、Rally 的读写串联均保存各自 operation ID，拒绝迟到或其它控制器的回调，并在 Mission 写入失败时停止继续上传围栏/备降点。旧信号仍保留供兼容组件使用；仍需工程师验证初始自动加载、被动同步、恢复任务和 fence/rally 真实 ACK 时序。
116. `PlanMasterController`、`GeoFenceController`、`RallyPointController` 增加 `operationStarted` 先行锁存和同步完成竞态保护；任务主控的 load/send/remove-all 入口统一检查初始连接、链路、通信状态以及写操作的解锁/飞行状态，并在 transient controller 失败或未创建事务时释放自身。
117. `ParameterManager::_factRawValueUpdated()` 成为 Fact 参数写入的最终 fail-closed 边界：离线、未完成初始连接、无主链路、通信丢失、已解锁或飞行中均不发送 PARAM_SET；有可用链路时回读权威值。APM 子电机维护页移除打开页面自动 DISARM，电机方向、自动检测和手动测试入口增加维护态复核与二次确认。
118. Vehicle 初始 Mission/GeoFence/Rally 读取改用带 operation ID 的完成信号；旧无参槽只保留给初始连接状态机的显式跳过路径，迟到的旧读取完成不得提前推进 `initialConnectComplete`/`initialPlanRequestComplete`。
119. `CustomAction`、Joystick 自定义 MAV_CMD 配置拒绝非法命令/组件和非有限参数；显式 `inflight_safe` 仅对白名单相机捕获命令 2000/2001 有效，其余强制 disarmed-only。MAV_CMD 的无响应隔离不再豁免模式切换与解锁/停机命令，降低迟到 ACK 串线风险。
120. `Vehicle::flashBootloader()` 增加 C++ 连接、初始握手、链路、落地/解锁状态总闸；Firmware Upgrade 高级按钮增加同一执行时复核和二次确认，避免旧页面或误点击在飞行/断链状态触发刷写。
121. Guided Orbit 的中心、半径、方向和目标改为确认时的原语快照，执行时重建坐标并校验有限地理范围；Set Home/ROI 同步冻结经纬高，Set Waypoint 校验确认时与执行时任务项数量，避免地图对象或任务内容在确认窗口内变化后仍发送旧意图。
122. 兼容版与 MERIVUS Guided 的多机 Pause 均按确认时冻结的车辆 ID 做全量链路/API 预检，再按冻结集合发送；执行过程中不再重新读取会吸收新加入机体的实时列表，任何目标失效时整批拒绝。
123. Vehicle C++ 命令入口增加统一 transport/maintenance gate：重启、标定、UAVCAN 配置、电机测试、Bootloader 刷写必须完成初始连接、存在主链路、未断链、未解锁且未飞行；模式切换、任务/Guided/RTL/Land/Takeoff、急停、相机日志、VTOL 等允许飞行的入口至少拒绝离线/未握手/断链状态。
124. Vehicle Guided C++ API 增加有限坐标、半径、高度、速度和爬升量校验；`doSetHome` 的异步地形回调再次检查 transport、坐标和地形数组，避免确认后的旧回调发送 NaN/过期 home 命令。
125. PlanManager/MissionManager 在 read/write/remove-all、Mission 请求/重试、Mission item 回传和 resume/guided 写入边界重新检查 live transport；常规任务/围栏/备降点写入与清空在解锁/飞行后终止，ArduPilot guided item 保留其明确的飞行态例外。参数写入重试也复用最终 Fact gate，不再由定时器绕过 armed/link 检查。
126. Swarm 临时 Mission 延迟上传在真正 `writeMissionItems()` 前重新检查目标、链路、健康、武装/飞行状态和所有冻结航点坐标，防止确认到 timer dispatch 之间状态变化仍写入机载任务。
127. `Vehicle` 的相机触发、云台、吊舱抓取、Joystick 手动控制和 RC 参数映射入口补充 C++ transport/维护态边界；RC 映射拒绝非有限数值、非法 tuning ID 或过长参数名，云台拒绝越界角度，Joystick 高频路径在未握手/断链时静默丢弃。
128. 自动更新 Home 与地形高度回调增加 transport、有限坐标和空数组检查，避免后台 GCS 位置更新或 terrain callback 在断链/无效数据下直接发 `MAV_CMD_DO_SET_HOME` 或解引用空结果。
129. `PlanManager::loadFromVehicle(bool allowInitialConnectRead)` 为初始连接状态机提供显式、仅限被动读取的早期链路；普通 UI 读取仍需 `initialConnectComplete`。同时阻止无 Home 项时把 Mission sequence 0 减成 `uint16_t` 的 65535。
130. Mission resume 生成成功后释放用于筛选的临时 `MissionItem` 克隆；PlanManager 继续持有独立写入副本，避免重复恢复任务累积本地对象。
131. 初始连接状态机先连接 Mission/GeoFence/Rally 的进度信号再启动显式 early-read，避免 fail-closed 的同步失败在回调连接前丢失。
132. `MissionManager::generateResumeMission()` 对 direct QML/plugin 提供的 `resumeIndex` 改为越界直接拒绝，不再把非法索引静默夹到 0，避免恢复错误航点。
133. 临时 Mission 清理在 `SwarmController::_requestTemporaryMissionClear()` 发送 `removeAll()` 前复核初始连接、主链路、通信状态及 grounded/disarmed；断链或飞行中只保留待清理状态，不改写当前机载任务。并修复 `TemporaryMissionState` 重复成员声明。
134. `Vehicle::_sendMavCommandWorker()` 对通信管理器已明确报告丢链路的通用 MAVLink 命令队列 fail-closed 并回调失败；不要求 `initialConnectComplete`，保留初始握手所需的 benign 命令路径，高风险用户入口仍由更严格的 C++ gate 保护。
135. 通用命令 worker 对空 `VehicleLinkManager` 也 fail-closed 并完成失败回调，避免维护/插件调用在 stale 目标上解引用空链路管理器。
136. 初始读取白名单仅在 `TransactionRead` 仍处于活动状态时生效；完成回调期间启动的新写事务不会继承 `_allowInitialConnectRead`，避免早期读取例外泄漏到任务写入。
137. `FleetAirspaceConflictChecker` 的禁飞区检查从固定 13 点采样改为包含边界的二维线段—多边形相交，并对路径段高度范围与禁飞区高度窗做保守相交；禁飞区声明高度限制但路径缺少有限高度时产生 blocking 的 `path_altitude_missing`，不再把缺失高度默认为 0 放行。`timeStepSeconds` 仍仅作输入格式门禁，未宣称为安全证明。
138. `Vehicle::motorTest()` 保留现有滑杆语义：`timeoutSecs=0` 用于停止/禁用电机测试，只有负数才被判为非法；连接、初始握手、主链路、落地和解锁状态门禁不变。
139. `FleetMissionReviewCoordinator::missionPlansFor()` 将任务/约束中的持续时间或有限 `startAt`/`deadline` 派生时长传给空域审查器；错误或反向时间保留为 blocking 输入，避免能量阶段有时长而空域阶段静默降级为未知时间。
140. `Vehicle::sendJoystickDataThreadSafe()` 在高频手动控制打包前拒绝 NaN/Infinity，并将上游轻微越界值限制到 `[-1, 1]`，避免浮点到 MAVLink `int16` 转换的未定义行为；离线/断链静默门禁保留。
141. `FleetAirspaceConflictChecker` 明确区分地理 `bounds.west/east/south/north` 与本地 `bounds.minX/maxX/minY/maxY`，对有限性和正向边界顺序做校验，避免把经纬度围栏错误投影为米制坐标。
142. Vehicle 的流配置与关闭入口增加空链路/断链保护：`requestDataStream()` 保留初始握手可用但拒绝 stale manager，批量重试队列遇到失效链路会清空，PID 遥测流切换不再访问空主链路，`closeVehicle()` 也不再对空链路管理器解引用。
143. `Vehicle::_sendMavCommandWorker()` 在主链路于入队前消失时也完成失败回调/错误信号，不再静默留下带 handler 的悬挂命令；`requestMessage()` 拒绝空回调、重复等待器和不可用传输，并修复失败/超时消息请求上下文的释放，避免旧消息等待状态覆盖新请求或持续泄漏。
144. `Vehicle::closeVehicle()` 关闭前停止命令/批量消息定时器、清空待重试队列，并把未完成命令和 `requestMessage()` waiter 以失败终态回调后释放；`_handleCommandAck()` 仅允许匹配活动命令的终态 ACK 改变 ROI、参数重置或 Bootloader 成功提示，孤立/迟到 ACK 不再制造成功状态。
145. `FleetMissionRiskEvaluator` 统一健康状态分类：`degraded/warning/limited` 进入需人工复核的 major 风险，`fault/unhealthy/emergency/lost/disconnected/maintenance` 进入不可用，未知状态不再默认健康；健康报告必须显式提供布尔 `online`，不得含计划外机体。能量计划必须显式提供布尔 `feasible/parametersValid`，缺失、类型错误或无效参数 fail-closed。
146. `FleetMissionReportBuilder` 与风险评估器对齐健康 taxonomy：严格读取布尔 `online`，缺失/非法在线证据、故障类状态、降级状态和未知状态分别生成 critical/major 健康问题，不再把字符串 `"false"` 或未知状态默认为在线健康。
147. `FleetAirspaceConflictChecker` 的车辆身份解析与 MissionCompiler/ContinuityPlanner 对齐：只接受有限的真实 MAVLink system ID 1–255，拒绝数值字符串（包括 `"01"`）和显示名称别名，避免同一任务载荷在不同审查阶段得到不同身份。
148. `FleetMissionDigitalTwin::setSnapshot()` 对外部 `snapshotHash` 执行与文件导入相同的 SHA-256 完整性校验；预演 Fleet 行的 `online` 只接受真实布尔，缺失或非法值进入未知统计，不再把字符串 `"false"` 当成在线。
149. `FleetMissionCompiler` 的 geofence bounds 语义与 `FleetAirspaceConflictChecker` 对齐：地理 `west/east/south/north` 检查经纬度范围及 `west<east`、`south<north`，本地 `minX/maxX/minY/maxY` 检查有限性和正向顺序；无坐标系/轴顺序声明的四数字 bounds 列表不再作为围栏证据。
150. 连续性建议应用入口增加 `FleetRolePolicy.currentUserId/currentRole` 与 `task.continuity.apply` 权限门禁；成功/拒绝审计携带操作者身份，仍要求显式人工确认并固定 `flightCommandReleased=false`。
151. `SwarmMissionOrchestrator` 的任务创建与重分配改为严格成员身份契约：任一字符串、布尔、小数、越界、非有限或重复 ID 使整次操作失败，不再静默丢弃合法子集；创建和通用重分配同时拒绝非法/冲突的显式 Leader 元数据，不自动替换成首个成员。
152. `FleetCapabilityMatcher`、`FleetRoleAllocator`、`FleetTaskTemplateRegistry`、`FleetEnergyScheduler` 统一将 `degraded/warning/limited` 作为 `health_degraded` 人工复核阻断，不再把降级机体直接纳入能力候选、角色分配、模板放行或能量候选；故障类 taxonomy 也与风险评估器保持一致。
153. `FleetMissionReviewCoordinator` 将编译器/角色分配得到的成员与备用机 ID 作为 `candidateVehicleIds` 传给 `FleetEnergyScheduler`；能量调度不再从整支 Fleet 另选一组机，候选身份列表类型/范围/重复项错误会阻断并保留 `invalid_authoritative_vehicle_ids` 风险。
154. `FleetTimelineReplay` 的 `replaceEvents()`/JSON 导入现在复用严格黑匣子事件 schema：必须有有限整数时间戳、type/source/payload，限制 payload 深度/节点/大小并拒绝乱序事件；`FleetAirspaceConflictChecker` 对部分时间戳只保留保守的空间审查，不再把插值时间当作权威 4D 时间证据。
155. `FleetMissionEvidenceChain` 拒绝早于链尾的时间戳，导入校验也要求记录时间单调不回退；文档明确 SHA-256 链不等于操作者身份或数字签名，主体认证必须由上层提供。
156. `GuidedActionsController` 的链路策略采样会从 Registry descriptor/metadata/linkMetrics 读取带宽、延迟、丢包和状态证据，保留真实类型交给 `FleetLinkDegradationPolicy` 严格判断；数值 `-1` 仅作为未观测哨兵，字符串/越界值不会被伪装成正常链路，策略仍只读且不自动改链路或飞行行为。
157. `FleetRegistry::vehiclesWithCapability()` 增加显式 `requireVerified` 过滤；普通查询保持 observed 能力兼容，安全任务可只匹配 `metadata.verifiedCapabilities`，QML facade 同步传递该门槛，避免把 heartbeat/observed 标签误当可信能力。
158. `CommandCenterOverlay` 的维护护照摘要新增统一的待复核计数；当记录存在但状态为 `unknown/monitor/inspection_due/grounding_recommended` 等非明确通过状态时，顶部状态条保持警示色，而不是仅因“有记录”显示为正常。该改动只影响只读可解释性，不改变维护评估、健康门禁或任何命令路径。
159. `CommandCenterOverlay` 增加能力证据摘要，将 Registry 车辆区分为显式 `verifiedCapabilities`、仅 observed 能力和 `discoveryState=invalid` 三类，并在指挥中心只读显示；发现无效的车辆不会被误显示为已验证能力。该摘要不改变匹配、审批或命令门禁。
160. `GuidedActionsController._appendMissionEvidence()` 为控制器生成的每条证据附加 `groundStationAudit` 操作者/角色快照，并明确标记本地策略来源、未认证、未签名；原始业务 payload 不被修改，证据链仍只提供完整性链接，不能被误读为权限或不可抵赖证明。
161. `CommandTransaction` 保留原始目标列表并新增 `targetInputValid`；构造时遇到非法、重复或别名身份会使整笔事务失效，`start()` 在任何 dispatch 前阻断，结果摘要同时报告 `requestedTargetCount` 与去重后的内部计数，避免静默缩小批量目标集合。
162. `FleetExtensionRegistry` 对扩展快照增加条目数上限，并对 `metadata` 增加递归深度、节点数、键长度和累计字符串/字节大小上限，在注册与导入两条路径共用资源门禁；扩展仍只保存元数据、不加载二进制、不启用命令释放。
163. `MissionHandoffManager::requestHandoff()` 在后端再次强制二次确认并限制交接有效期为 1–86400 秒；即使绕过 QML facade 的固定 300 秒策略，raw QObject 调用也不能创建单方确认或超长期 pending 记录。
164. `MissionHandoffManager` 进一步收紧交接记录边界：任务/操作者/原因/备注和 metadata 有长度、类型、递归深度、节点数及累计字节限制；解析动作对备注/操作者同样执行上限检查，交互式接受/拒绝/撤销必须带 `recordRevision`，只有内部过期路径可无 token 结算；导入要求记录有效期、解决时间顺序和二次确认字段保持真实布尔 `true`，完整快照仍在全部校验后原子替换，避免超大或时间/策略异常的审计记录进入内存。
165. `SwarmController` 编队命令发送增加实时链路、`isConnected()` 和同组件/同命令 pending 预检；处理 `Vehicle::sendMavCommand()` 可能同步返回的本地失败/成功 ACK 时，延迟 phase 推进或 ABORT，直到当前成员发送循环结束，并在 ABORT 前预置 pending 集合保留同步拒绝/成功结果，避免回滚重入后继续发送旧阶段命令或清掉失败证据。
166. `FleetCapabilityDiscovery` 与 `FleetRegistry` 为能力/构型 descriptor 增加共同资源边界：能力证据列表最多 128 项、单项最多 128 字符；Map/List/Hash/StringList 递归最多 8 层、2048 节点、累计字符串/字节最多 128 KiB，并拒绝非 JSON-safe 自定义值。发现构建、Registry register/update/heartbeat 三条入口均在计算指纹或合并 metadata 前 fail-closed，避免异常能力/构型输入拖垮规划层或污染可追溯档案。
167. `FleetCapabilityMatcher` 对任务能力列表、车辆能力证据和 Fleet 候选数量增加同源资源边界（能力最多 128 项/单项 128 字符，候选最多 2048 架）；超限输入在排序、能力匹配和候选分配前进入 `parametersValid=false`/明确拒绝原因，不再让异常任务或超大 Fleet 消耗规划循环。
168. `SwarmController` 将普通多机 Takeoff/Goto/临时 Mission/标准 Mission Start 改为创建时冻结目标集合的全量预检：任一选中机体缺失或前置门禁失败则整批不入发送队列；Land/RTL 仍保留明确的安全逃生部分执行语义。此检查不构成多机原子执行：错峰发送期间逐机失败不能撤销此前已发的动作，必须保留逐机 ACK/超时结果。Goto 清理/抢占顺序的后续修复另列增量项。
169. `SwarmMissionOrchestrator` 增加任务类型（128 字符）、原因（1024 字符）、任务数量（10000）与 metadata 资源上限（8 层/2048 节点/128 KiB），拒绝非有限数值、自定义值及超限记录；被拒绝的创建/状态请求不局部修改已有任务。上限是本地资源契约，不是可安全同时执行的任务规模。
170. 能力资源边界复核修正：发现输入/验证超限后实际早退出，不再继续递归计算 fingerprint；多来源构型合并后再次验证预算，任何发现错误保持 `invalid`。Registry 对累积合并结果进行暂存校验后提交，防止多次小更新累积越界；Matcher 无效参数/超限 Fleet 不再遍历并排序前缀候选；null 也计入节点预算，能力长度按原文计算，不能用空白填充绕过。
171. 任务成员占用：编排器对 `running/paused` 任务建立本地排他占用语义，启动、恢复、活动任务重分配前检查其它活动任务成员，并提供 `assignmentAvailability()` 冲突诊断及 `busyVehicleIds` 摘要；Draft 可提出重叠方案，终态释放本地占用。编队 UI 必须先成功预留成员再发送，连续性应用前保留完整任务上下文；此锁只作用于本地任务模型，不是跨站控制权或机载命令锁。
172. 连续性规划新增任务集合占用索引（running/paused/pending），排除其它活动任务成员作为替补；冲突/重复/损坏任务身份或成员使全集合只输出阻断 hold。建议 ID 绑定稳定 `sourceTaskOwnershipFingerprint`，别的任务启停/重分配使旧建议失效；单任务查看明确缺少占用证据，控制器拒绝应用这类建议，并由编排器在写入前二次验证占用。
173. Goto/临时任务抢占顺序修复：完整组预检后才进行旧任务清理，普通 Goto 可先预留继任事务再取消旧临时任务；等待 Pause 的 DO_REPOSITION pending 清空时，回调持续复核事务截止/目标/健康/连接，避免旧 ACK 被新 Goto 消费及重复模式请求。Clearing 不再周期性重复 Pause，取消旧事务前快照 QPointer 集合；同步 ABORT 已终态时不重复结束/覆盖回滚结果。仍不是协议级 ACK token 或多机原子执行。

174. 区域覆盖规划：新增纯 C++ `FleetAreaCoveragePlanner` 并加入 `custom.pri`；显式 `task.coverage` 生成简单凹/凸多边形扫描航段，按机体 ID 稳定分配不同航段，通过区内可视图连接凹形断段。坐标/高度基准、辅助角色路线、输入及几何资源有严格边界；输出距离/时长仅为估算，不生成进场/返航、不证明真实覆盖或碰撞安全。
175. 任务路线贯通：编译器允许完整逐机路线而不强制全局路线，拒绝全局与覆盖生成冲突；严格保留点时间、基准、成员返航和起止/时长字段，任一坏航点阻断整条路线而不保留合法前缀。任务级返航回退先归一化后进入覆盖基准门禁，覆盖提案及逐机路线进入计划指纹，仍固定人工审阅且不释放命令。
176. 审阅时序与能量接线：覆盖任务不向所有角色复制全局路线；能量预算纳入最长逐机估算/显式时长及任务上界预算，明确来源。空域把匀速估算保持为未知权威时序，未知时序不跳过时间受限禁飞区；返航使用独立时间表，不继承出航时间表。本轮只静态收口源码，未运行规划器、编译、测试或上传。

177. 覆盖审阅事件使用有界摘要：`GuidedActionsController` 为覆盖提案的 `intent.preview` 记录任务/计划 ID、状态、参与机体与航段/航点计数，不重复保存巨大 plan/report 路线树而触发黑匣子资源拒绝；显式标记 `fullSnapshotIncluded=false`，证据链另记实际 `previewEventRecorded`。完整审阅仍在 facade 内，摘要不是可恢复任务快照。

178. 空域跨层约束补强：任务最小间距传给 checker，损坏输入阻断且 options 不能降低任务要求；路径/禁飞区核对显式本地原点、高度及时间基准，未知/不兼容时不凭错误数值排除冲突。缺高比较统一降为双方 XY，稀疏时间戳改为线性间隙处理。检查增加输入、比较和诊断预算，超限标记未完成/不可行，不能把已检查前缀放行。生成成员及包装路线拒绝矛盾基准，高度上限需同一显式 `maxAltitudeReference`。仍未运行/编译/测试。

179. 区域端点提案：`FleetAreaCoveragePlanner` 新增严格 `vehicleEndpoints` 和显式爬升速度，生成逐机 3D 进场/独立返回、巡测视图和航段索引；区内复用可视图，区外/垂直净空明确未验证，不推断起飞或降落。所有路径视图及辅助路径纳入共享数量/可视性预算，失败不保留前缀。
180. 全路径能量与高度收口：协调器独立保留进场/巡测/返航估算，返航储备至少覆盖最长返回估算，不将其计作权威时间；编译器核对完整出航、支援及返回点的高度上限与同一基准，不能只检查巡测高度。
181. 新增 `FleetMissionPlanStore`：按本地提案 source 保存完整计划及独立审阅身份，每次准备新增不可变版本，以 recordId/revision/两个 hash 关联；默认 AppData 原子持久化、本地文件锁+磁盘基线 CAS、损坏档案写入隔离及严格恢复验证。恢复只读历史，清空 live 状态并更换 epoch；有明确容量上限，不是跨站控制锁或完整长期数据库。
182. 版本批准与审计接线：私有版本库/只读 facade、调用前输入/审阅世代及存档 CAS；恢复/失败/陈旧或重入替换不能沿用批准，拒绝/取消仅撤销旧版本，不损伤新提案。批准请求/结果/拒绝/取消回执携带精确版本，过期批准视图回到待审阅；交接只附精确同 source 的历史引用，不转移批准或机载控制权。

183. 新增 `FleetMissionDraft`：原生、有界的本地地图草稿保存区域、固定成员/角色、成对端点和辅助机独立路线；完整参数替换、实际数值/布尔类型、AMSL/WGS84 声明和 revision CAS 先校验再提交。32 成员/64 顶点/每路径 256 点/保留路径合计 4096 点是编辑资源上限，不是可安全执行的机队规模；无 Vehicle、文件或命令接口。
184. 地图规划界面：指挥中心新增“区域任务规划（仅提案）”，支持右键选点和键盘经纬度、顶点编辑/删除/末点撤销、明确角色、覆盖参数、逐机端点及辅助出航/返回路线；辅助路线每页 12 点，选点模式缩小面板。地图仅绘制 `missionReview.plan.members` 的出航/返回，不把自动匹配建议当作手工成员或执行路线。
185. 草稿审阅身份：每次编辑（含未提交/拒绝/无变化输入）立即失效旧审阅，生成前提交完整参数并阻断未保存坐标；生成前后核对 draft revision/edit generation，审批只接受面板显示的 revision/recordId，再复用现有健康、构型和版本门禁。计划确认不上传、不解锁、不起飞，未知时序/净空仍不能放行；草稿仅在当前界面会话保留，未增加跨站或重启草稿恢复。
186. 规划与旧地图指令隔离：左右键/Shift 补确认、菜单、队列 adapter 和最终发送前均检查规划模式/交互 epoch；按下后跨模式的手势直接丢弃，避免关闭规划后变成 GoTo。打开规划清旧队列/确认令牌，暂停首次 GCS/机位居中、追踪动画和新任务自动缩放；独立人工 Land/RTL 安全入口未全局禁用。QRC/qmldir/内部类型/qmake 注册已接线，仍未编译、启动、渲染、测试或更新 PR/安装包。

187. 新增 `FleetMissionPayloadBuilder`：完整批次校验后构造 WGS84/AMSL 出航和返回的 NAV_WAYPOINT 几何载荷，拒绝不连续返航、未支持 heading、坏基准/高度和超限；默认 yaw 显式 null→NaN，不误设为零度。原 double 与 MISSION_ITEM_INT/float32 规范 wire 指纹分开，回读逐项/全哈希核对，量化后高度也不得超过上限；原时序、速度/爬升要求和差值保留但明确未编码，不是执行许可。
188. 新增 `FleetMissionDeployment`：冻结实际审批/归档身份、Vehicle/MissionManager 实例和整组目标，重复复核健康、版本和本地任务忙机；仅在支持已知 MISSION_INT 的地面未解锁 PX4 多旋翼上顺序上传，捕获 Write operationId，另发独立 Read 并核对完整机载内容。ACK 不等于回读，回读一致也不等于可执行；当前没有自动启动、解锁、模式切换、清空或回滚。
189. 共享占用与排空：Swarm 普通命令遵守整组部署占用；RTL/Land/ABORT 撤销部署权限但保留仍活跃的传输占用。取消/超时阻止下一机/读回，不伪称已经撤销机载写入；在 `inProgress` 或 transport 调用栈未结束前不释放，占用/历史均有边界。外来 Mission 操作、源对象替换、版本/健康变化使凭据失效。
190. 一次性确认与审计：控制器提供请求/确认/取消的源码入口和只读部署状态，确认冻结显示 recordId、版本及两项 hash，60 秒一次性凭据在传输前消费；实时 owner gate 核对当前批准及 running/paused 忙机。阶段事件通过 `snapshot.binding` 关联真实版本，记录可能已写入而不称飞行成功；尚未提供可点击的上传/启动 UI 或更新 PR/安装包。
191. 任务协议重入与本地 ACK 证据：PlanManager 入口、写 worker、完成及协议回调调用栈隔离，并覆盖 MissionManager 的 guided/resume 绕行路径；完成前冻结本地 ACK 的 operationId/内容指纹及是否来自明确的新上传，再整理缓存/发布信号。服务在嵌套事件循环下保存后续步骤，待栈退出再推进/排空，不能吞掉下一步或接管旧缓存。仍不是线上的 MAVLink token 或机载身份认证。
192. 几何暂存启动隔离：Swarm 在精确 Write 开始时记录冻结的 MissionManager 并按系统 ID 隔离旧启动入口，重连/被动读回/恢复旧任务不能解除。只有占用释放后的明确新上传、精确本地 ACK 来源及当前缓存指纹一致才恢复原手动门禁；没有把几何读回提升为受控启动。覆盖计划保留原巡航/爬升输入，完整速度/时序/终止动作编码和受控启动仍待开发；本轮仍未编译、运行或测试。

193. MissionItem 对象接管：PlanManager 对完整无别名输入建立实际 QObject parent 所有权，并在接管、Fact 回调、协议进度/错误/发送和清理后核对 QPointer；manager 在 direct callback 中被删除时不写旧对象，已接管航点由 manager 析构释放，synthetic Home 保持调用方所有权。只完成源码生命周期防护，不构成运行时泄漏或线程安全证明。
194. 恢复任务延迟与原请求回执：MissionController 100 ms 有界续作等待原生栈退出，冻结原 Vehicle/MissionManager、requestId、Write operationId 和本地 ACK 内容；新尝试/新操作/换机作废旧续作。原源任务 content/manager epoch/数量在确认时冻结，native 生成前复核，重试仍沿用原 source，拒绝同数量替换路线。MERIVUS ready/fail 使用带 request 的信号，私有不回绕序列、uploading 阶段、精确实例及最终 `resumeUploadMatches` 防旧回调消费新确认；生成后按新 itemCount/内容快照启动，不将旧源索引当重编号任务的 current-index。仅在有效恢复上传期间抑制自动普通 Start 弹窗，主动新动作仍可取消旧确认；保留原人工恢复流程，不新增 Fleet 任务执行许可，未编译/运行/测试。

195. 显式执行 profile：新增纯静态 `FleetMissionExecutionProfile`，严格八字段和实际数值/规范 WGS84/AMSL；编译器 0.3.0 保留任务默认、成员整份覆盖、显式 holdSeconds 及计划身份，坏覆盖不回退。覆盖参与者巡航/爬升冲突直接阻断，辅助成员不继承覆盖速度；none/RTL/absolute-required 仍保留待适配，不造执行许可。
196. Mission 载荷 v2：按显式 profile 编码巡航/返航速度、TAKEOFF、WP 相对驻留、终点 HOLD/LAND；不从 ETA/估算推导驻留。TAKEOFF/HOLD 精确匹配对应路线点，LAND 同经纬度且明确地面 AMSL 低于返航进近高度，raw/float32/canonical 邻接和高度分离双重核对；五类命令/frame/NaN yaw/保留参数严格白名单，来源序号映射及全部浮点/E7 差值进入有界完整载荷。新增动作计入每机 2048/整批 8192 item 预算，错误无前缀。
197. 原生上传适配：Deployment 按实际命令处理 param4，地理默认 yaw=null→NaN，速度保留值=数值零；读回按实际 command/frame 交由 v2 核对，未知命令不借用期望元数据。状态明确为完整 Mission 项回读而非实际执行验证，终态历史只保存语义数量/未编码摘要，不重复逐项 manifest；全部仍 executable=false，不调用启动/解锁/清空/回滚。
198. 存档与驻留审阅：PlanStore 接受规范 executionProfile 根字段并复核根/成员/assignments 的完整规范值，旧无 profile 档案兼容，恢复仍不恢复批准。审阅按出航/独立返航累加显式驻留，加入同机原移动估算和最低预算；驻留不能单独补齐未知总时长，显式 duration/时间窗或相邻权威到达间隔短于驻留时阻断，估算标签不当权威。512 成员/32768 点是审阅资源上限，不是实机队规模；绝对时序、垂直参数认证、实际起降轨迹/净空与完成判定仍待开发。

199. 原生参数观测账本：`ParameterManager.h/.cc` 提供非 QML 的 `parameterObservation()`，仅跟踪默认 autopilot 的十二个已知参数；Vehicle 传入实际接收 Link，要求精确非零 system ID、组件与真实非回放所属链路。值/类型/来源内容 revision 用不回绕序列和十进制字符串保存，年龄用单调时钟；缓存/FTP/导入/离线、本地写与刷新、模型/连接/主次链路变化或超时撤销来源。参数 getter 不刷新、不写入，不将 Fact/parametersReady 当作新鲜接收证据；当前值/类型及 pending write 需一致，外层 token/epoch 防旧同步回调覆盖新观测。
200. 新增 `FleetMissionVehicleEvidence.h/.cc/.md`：从精确实际 Vehicle/ParameterManager 只读采集十二项观测，完整排序内容生成 parameterProfileHash，稳定 evidenceFingerprint 另绑定来源、epoch、fresh/stale、native context 和请求。明确 requiredProfileHash、AMSL 上限及水平/垂直请求一致性，未知值/不完整 profile 不伪造性能。UID/固件/能力位仅标记 native-cache，参数可用不等于适配器、起降净空、MISSION_INT 或执行认证；所有执行字段 false。
201. 审阅与版本绑定：协调器增加 opt-in `reviewWithNativeEvidence()` 和 bounded 编码摘要，保留原 value-only 规划 API；实际成员源捕获最多 32 架，显式 profile 的不可编码/参数证据不足进入 blocking 风险。QML 准备使用原生模式，归档原样保存 transportEncoding/nativeVehicleEvidence 并进入 reviewEvidenceHash；批准复核 actual sources、稳定指纹、输入/审阅代次、store binding、健康与 fleet。coordinator 不回绕 invocation epoch、QPointer 和返回本次局部结果防嵌套审阅借用，过期需新审阅，不自动请求参数。
202. 部署来源最终门禁：归档两个证据 Map 必须与冻结审阅完整一致，旧/缺失原生证据要求重新准备；每次继续上传/独立读回的门禁在 owner/健康/目标检查后调用 nativeEvidenceCurrent，尾部复核实际 backend、版本、状态及占用，并拒绝门禁重入替换。失败只停止后续步骤、保留活跃传输排空，不清空/回滚；历史仅保留组指纹和有界来源摘要。无 profile 的几何暂存不制造参数可用或执行许可，所有部署仍不可执行。
203. Fact 与接收来源生命周期：`forceSetRawValue`、`setRawValue`、`_containerSetRawValue` 拷贝 signal 参数，direct callback 后核对 QPointer/异值，deferred 标记在 valueChanged 前提交。Vehicle 用 QPointer 保存 PM，两个 getter 移至 cc；接收链路冻结 self/Link/FactGroup 来源并在回调返回后守卫，参数 ready 续作冻结实际 PM，初连缺 PM 不推进。`InitialConnectStateMachine.cc` 的 connect/disconnect 同步改用 public getter，避免 QPointer 模板契约回归。此为局部源码保护，不是所有 callee 内部、全 Fact/Vehicle、同值 ABA、QML CONSTANT 销毁通知、运行时线程/泄漏或飞行认证。

204. 真实版本观测：Vehicle 的非 QML `autopilotVersionObservation(60000)` 从实际 MAVLinkProtocol/Link 的原始 AUTOPILOT_VERSION 帧冻结解码，精确 system/component 与非回放来源、单调年龄/sourceEpoch/revision、token/QPointer 及有界版本接收 depth 防缓存回退与嵌套借用。十二个原始 payload 字段使用 JSON-safe 数值/uint64 十进制/完整 wire hex 做 canonical hash，UID2 尾零正确恢复；正常初连完成可保留同源实际回报，断链/换源/model/移除撤销。初次/新内容或来源提交同步撤销 PM 十二项参数，不请求/写入；同内容同源刷新不撤销。
205. 新增 `FleetFirmwareAdapterCatalog.h/.cc/.md`：最多 64 个原生工程声明、完整 firmware tuple/sourceCommit/审阅引用与固定五命令语义；严格 CAS、不回绕十进制 revision、新 epoch、1 MiB/depth8/8192node JSON 预预算及转义重复键检查、原子整批导入和 QSaveFile。匹配实际 version/INT/UID 观测与全部十二字段 hash，Git5+Vendor3 显式布局与 fullCommit 前十位对应；声明匹配/材料 digest 不是签名、干净源码/子模块/构建选项或烧录二进制证明，所有权限/认证 false。
206. 原生版本/目录接线：VehicleEvidence schema2 保留缓存显示与真实版本两个来源，完整类型/hash及参数读取后的版本/目录二验；ReviewCoordinator 0.3.0 私有持有目录，启动时可读 AppData/fleet-os/firmware-adapters-v1.json，公开仅只读值快照与非 QML C++ 配置 getter。显式 profile 缺真实鲜版本或对应声明阻断；目录改动/销毁、版本/参数来源变化失效旧审批及上传继续步骤，不依赖物理文件自动watch。每组完整目录归档一次，成员只保存 small binding/match，不冒充执行认证。
207. 来源摘要与交接：部署摘要区分 firmwareObservationUsable/adapterDeclarationMatched/parameterEvidenceUsable；黑匣子对所有原生审阅（不止覆盖路线）保存有界 source hash、状态、成员数量及精确归档记录/两个hash，不重复巨大证据树，显式 fullSnapshotIncluded=false。没有覆盖输入时为 null 而非假零覆盖结论；完整证据只在成功归档中保留。同步状态/路线图/模块/审计/工程师说明，独立启动/完成/UI、真实净空与其它产品目标仍待开发，未编译/运行/测试。

208. 真实飞行状态：Vehicle 新增非QML `flightStateObservation()`，固定 HEARTBEAT/GLOBAL_POSITION_INT/HOME_POSITION/EXTENDED_SYS_STATE/MISSION_CURRENT/MISSION_ITEM_REACHED 六通道；原始消息/真实Link/精确ID、单调年龄/源epoch、value内容hash和decimal revision/receiptSequence分开。插件/Core/缓存callback前冻结、之后token/QPointer/depth守卫提交；版本/UID/源与既有生命周期变化撤销账本。相同flight源GP的timeBootMs小差回退保守撤销版本/参数/flight，仅为可用性撤销，不能认证重启；自然wrap、恒0与迟到仍有边界。未知扩展字段和重复reached不提升为执行/完成，位置零坐标不统一拒绝；GPS质量和真实净空仍未认证。
209. 新增 `FleetMissionStartReadiness.h/.cc/.md`：从实际Vehicle/参数/版本/声明和真实flight观测计算前置证据，复用规范profile与完整载荷一致性；地面起飞推导MIS_TAKEOFF_ALT抬高后的当前位置垂直段与到指定第一点连接，核对声明AMSL上限。未解锁仅能提供预审证据，仍需独立人工解锁和新启动审批；none必须fresh armed/IN_AIR。绝对时序/同步/RTL、起降净空、机载身份和完成缺口保留，所有启动/执行/认证false。
210. 新增 `FleetMissionProgressObservation.h/.cc/.md`：完整规范payload与当前实际MissionManager缓存独立对照，只将fresh raw current/reached序号映射到一个sourceItemMap行；armed/landed/PX4 AUTO_MISSION分别观测，不以索引比例伪造完成率。reached可为旧任务重复，明确unbound/reachedAttributionVerified=false；最后HOLD不会自行完成，LAND/地面/停机不能单独证明本任务成功。尾部复核实际源/缓存，所有完成归属与执行false。
211. 部署检查接线：`inspectExecutionState(deploymentId)`只用冻结原生计划/目标，拒绝忙传输/旧ID/替换对象，并复核当前staging门禁；ARM后隔离不解除，只允许显示未归属的raw状态对照。公开facade新增startReadiness/progressObservation短时snapshot-only值，1秒到期/状态与世代变化撤去成员事实；controller主动检查写有界计数/状态摘要，不把全snapshot放入历史。新helper纳入pri，修改清单/路线图/审计/工程师文档同步，仍无独立启动事务或上传/启动UI，未编译/运行/测试。

212. 本地角色快照：FleetRolePolicy新增mission.upload/mission.start.review和原生actionSnapshot/current，九字段严格String/Bool、唯一policyEpoch及不回绕十进制revision，权限/身份A→B→A变化不沿用旧确认。未知/退出身份回退observer且实际用户分配必须与currentRole一致，防继承上个用户高权限；Unicode/控制字符与bool混型拒绝。仍无账号认证/第二批准服务，额外审批要求不由单actor bool充当。
213. 上传角色与历史回读接线：Deployment beginUpload收实际私有policy及冻结角色收据，每次gate复核精确对象/收据/owner来源，角色或策略变更撤销继续步骤并排空；缺额外批准明确拒绝。完整独立回读成功时一次保存本runtime历史receipt，fail/cancel/ARM仍清live verified。NONQML startReviewContext/getters仅整组历史成功、原实例/最新来源且传输idle可读，不接受部分集合、文件历史或伪造readID，不恢复旧批准/隔离。
214. 新增FleetMissionStartReview.h/.cc/.md：新UUID/代际与单调30秒本地独立风险审阅，实际角色/owner、非恢复最新record/plan/hash、完整历史回执、原生参数/版本/目录/飞行状态/健康及当前缓存一致性捕获；ack复核同一不可变来源，变化须重审。已阅仅记录风险，不授予启动、身份认证、第二人批准或任务完成；没有命令dispatch、stage/intent状态回写或档案IO。
215. 新增FleetMissionDeploymentPanel.qml并嵌入既有Planner/资源/qmldir：明确覆盖机载任务的二次确认，默认取消/非默认Enter、press→release版本/角色锁、实际members.itemCount总数及目标冻结、匹配请求取消/一次消费、逐机ACK/回读/排空/停止不回滚、snapshot-only阻断展开和新独立风险审阅展示。默认无actor/权限/额外审批源时明确禁用，不提供自填身份快捷放行或Start/ARM按钮。
216. Controller/类型注册与交接：新增matching-only取消请求、角色快照变化撤销、实际private policy来源回调及独立风险审阅窄函数；新backend纳入pri和MerivusInternal注册，公开仅只读facade/值。同步操作契约、路线图/状态/矩阵/修改清单/静态审计与工程师说明。完整启动事务/新鲜机载身份/完成归属、账号/第二批准及其它原目标仍待开发；未编译/运行/渲染/测试、提交或更新PR/安装包。

217. 新增 FleetOperatorSession：显式 Windows 系统凭据认证与实际 TokenUser SID，本地固定专用角色组的直接 SID 成员关系作为角色来源；无预置身份/默认管理员、不创建账号或修改组。原生凭据不进入 QML/日志/持久化，有限会话和独立账号收据；非 Windows、未知/不支持凭据、角色冲突或源错误拒绝。这是本工作站账号证据，不是企业登录或两名不同人员证明。
218. 新增 FleetOperationApproval：实际 session/policy/store/private owner 和完整任务版本冻结，独立授权 SID 确认后再复核全部来源；短期一次性上传收据，原生消费后每步验证同一 grant，不恢复档案批准、不续期、不以单人勾选或可变角色策略替代第二来源。撤销/到期只停止后续传输，不回滚已写内容。
219. 上传接线：Deployment 接实际 session/approval 与冻结身份/审批、全来源 QPointer 及返回后的生命周期复核，载荷/目标校验后原生消费、每步和末门禁重核。Controller 私有实例/只读值、专用源匹配回调与显式认证/独立审批/匹配撤销；身份同步不重复改角色 revision，原确认期限不超过审批剩余寿命。新增类型/路径/Windows 链接库写入源码工程。
220. 操作界面与交接：现有上传面板补系统身份、绑定任务的独立账号审批及撤销/退出，确认冻结十一字段身份和二十字段审批，按下/松开到确认复核；额外审批与覆盖确认分开，无密码输入或 Start/ARM。同步源码契约/状态/路线图/矩阵/静态审计/工程交接；未编译、运行、登录、测试、真实上传、提交或更新 PR/安装包。完整启动/完成和其它原目标仍待开发。

221. 原生身份上传租约：短期独立审批只在实际消费时转成原生单实例、一次性的上传身份租约；最多240000ms且不越过原操作员5分钟期限，独立审批的60秒认证只作为消费时的历史批准依据，不被改写/续期。每步继续检查原epoch、双方SID/当前角色与实际桌面来源，退出/换身份/撤权/到期撤销，不恢复JSON授权。
222. 审批与部署租约接线：审批receipt升级schema2/22字段，显式同意最长4分钟和持续操作员要求；grant升级schema2/13字段，冻结实际deployment UUID、完整归档/身份与原生租约。请求60秒与消费后的租约分开，使用消费前时刻与实际获准duration形成保守ledger期限，不混比较时钟域、不给验证延期；有效已消费授权不被新请求暗换。
223. 工作站会话撤权：Windows原生WTS会话来源、实际process session与锁定/活动状态加入认证和上传租约门禁，并处理锁定/注销/断连通知；旧系统反向flag/未知/注册失败拒绝，不以最后一次未锁定缓存充作当前事实。写入Wtsapi32链接契约，没有调用系统接口、锁定工作站或运行应用。
224. 有限租约操作说明与静态收口：上传面板和系统审批明确申请期限/本次部署租约/原操作员上限，22字段含租约政策一起冻结，不提供自动续期或飞行授权；同步修改清单、状态、路线图、矩阵、补丁/审计及工程师交接。4分钟不是长任务持续控制权，真正续授权、跨站租约、独立启动/完成和其它原目标仍待开发；未编译/运行/登录/测试、真实上传、提交或更新PR/安装包。

225. 原生只读请求：Vehicle 将 AUTOPILOT_VERSION 请求纳入冻结 PRIMARY Link/来源 epoch 的单次命令队列，不借用 legacy void* 消息等待者；ParameterManager 仅对固定十二项发送 PARAM_REQUEST_READ，支持完整16字节名称，不修改缓存、参数等待表、写入或 FTP。成功提交 RAW 的十进制 receiptSequence 与内容 revision 分开；接收来源须是实际冻结链路。mandatory原生C++授权fence在插件前/后及prequeue核实际请求/账号/原期限，随后复核真实地面状态、对象和不可变报文；只读ACK只清队列不当证据，有限隔离不改变其它命令的全局豁免政策。
226. 新增 FleetMissionEvidenceRefresh：显式当前原生账号/角色/私有控制器授权，冻结最多32架实际 Vehicle/PM/任务管理器/PRIMARY，逐机版本后十二项读取；120秒总期限、4秒每阶段和100毫秒间隔，初次版本后再捕获 PM epoch。仅原生明确未发送的 busy/rate/status-wait 在原阶段内等待，不延期或重发已发送请求；已知解锁、空中状态、替换或来源变化停止。要求新 RAW 计数大于基线，但无线协议无请求关联 token，不认证因果、硬件或固件。
227. 刷新接线和操作界面：MerivusInternal/pri 注册新服务与 mission.evidence.refresh 只读权限，controller 先复制严格机号列表，再撤销旧审阅/意图/上传批准；公开仅只读值与申请/匹配取消。现有上传面板显示草稿目标、默认未勾选的撤销同意、按下冻结控制器/身份/权限/目标、逐机阶段和取消在途边界，无参数写入、任务上传、Start/ARM 或自动恢复审批。
228. 刷新契约与交接：同步模块、修改清单、状态、路线图、矩阵、索引、补丁/静态审计及工程师说明。完成只表示有限历史观测采集；120秒服务年龄不能替代后续30秒参数审阅策略，32成员容量不保证批次完成或突破六机协议。源码审阅不替代工程师构建/交互/链路/仿真/硬件验证；未调用业务或系统 API、运行/测试、修改飞控、提交或更新 PR/安装包。独立启动/完成及其余六类源码目标继续保留。

229. 独立启动意图账号依据：OperatorSession精确区分mission.upload与mission.start，启动提示明确后续可能涉及解锁/飞行但本调用不发指令；启动专用16字段原生依据一次消费、当前复核与匹配撤销，严格保留原主身份/独立审批截止、不创建4分钟启动租约，不接受上传依据交叉消费。注销、组/桌面变化和到期失效，不恢复文件或JSON授权。
230. 风险内容与报文到达分离：Vehicle保留完整RAW v1、receiptSequence与原boot-time回退撤销，新增native风险v2/独立单调riskContentRevision/hash，仅GLOBAL_POSITION_INT去timeBootMs。风险投影先核完整raw/risk hash，再排除纯接收计数/age/GP输送时钟；位置/高度/速度/heading/armed/mission/unknown/fresh/source变化与物理A→B→A仍改变风险身份，未增加容差或执行证明。
231. 原生已阅上下文：StartReview新增17字段native approvalContext/Current，冻结实际不可变artifact/source、Review实例epoch/generation、Deployment版本、全组目标及归档身份，只排除显示剩余毫秒。原30秒涵盖已阅、系统输入和后验，不重新计时；健康通知只按原实际gate/Leader比较，不因年龄推进盲撤销，也不静默更新原artifact，真实来源/健康风险变化仍拒绝。
232. 新增FleetMissionStartApproval：实际Session/Role/Store/Review/Deployment/private owner、全记录字节和实际Vehicle/MM/PM/VLM/PRIMARY（或原明确缺失）冻结；不同真实SID独立批准条件启动意图，34字段receipt/13字段native conditional grant，一消费及匹配撤销，过期替换/销毁清理对应原生依据，WTS前重入/后生命周期守卫。当前风险和未解锁原因原样保留，审批/消费均startupPermitGranted/executable/flightCommandReleased=false，不接Vehicle发命令。
233. 启动意图接线与交接：注册内部native类型/pri和独立mission.start权限，controller公开只读值及申请/匹配撤销，草稿编辑/只读刷新先撤销旧风险及启动意图；精确私有来源/归档/忙机门禁，无新Start/ARM按钮或启动消费入口。同步模块、状态、路线图、矩阵、清单、审计及工程交接。完整鲜机载内容核对、启动许可/事务/UI、受控状态过渡和任务完成仍待源码开发；未编译/运行/认证/测试、修改飞控、提交或更新PR/安装包。

234. 原生整表读取作用域：PlanManager/Vehicle 新增 caller UUID、mandatory by-value fence、真实 prepared/active own-read 判定与匹配取消；发送前后插件及 prequeue 核实际账号/对象/来源/原期限，READ COUNT/INT 项来自未调整的原始 PRIMARY 报文。独立 pending cache 仅完整读取及本地 read-end ACK 实际排队成功后提交，失败/取消保留旧缓存但不给新成功依据；原始接收入口栈防旧报文回调创建新读作用域，取消后已结束栈不能落入 legacy 无 fence 发送。
235. 新增 FleetMissionStartContentCheck.h/.cc/.md：实际 Session/Role/Store/Deployment/Swarm/private owner、最新非恢复全记录/七项绑定、冻结整组源及自身原生 scope；最多32架/2048项每机/8192整组、120秒总期限及原单机期限，逐机完整v2 canonical比对。原始 GLOBAL_INT5仅显式映射同义AMSL GLOBAL0，保留 raw/current/原报文hash，不把current当执行证据；必须成功实际34字段RAW收据和源核验，旧ACK/缓存不能替代。
236. 新读取到独立审阅：Deployment prepareStartContentCheck在冻结新上下文前撤销旧live staging，不写/清空/回滚；StartReview九参数接实际ContentCheck与UUID，18字段native上下文只排除remainingLifetimeMs，绑定新全组raw来源并将原30秒审阅期限缩短到最早READ START截止。授权回调在最终物理捕获前结束，尾部仅被动原生源/操作证明，防嵌套回调悄悄替换风险；保留原风险/历史上传/缓存及远端原子/无线归属/硬件认证false，不恢复批准、不续期、不产生StartPermit/完成。
237. 权限/控制器/现有Qt面板：新mission.start.content.read仅授权只读，不用第二账号勾选替代执行审批；controller先撤销旧Review/StartApproval/uploadapproval/草稿批准，公开只读值和精确申请/匹配停止。默认未确认、按下冻结实际部署/绑定/目标/身份/权限、返回换源尝试取消原请求，展示逐机/排空/匹配但过期及保守倒计时；上传/参数刷新/审阅避免重叠，没有Start/ARM按钮。已完成事实可在仍有效主身份的独立审批窗口中保持，但锁屏/换账号/撤权/原期限不豁免。
238. 本批工程接线与交接：新h/cc纳入pri和MerivusInternal，更新模块、清单、状态、路线图、矩阵、索引、原生身份审计与工程师说明；源码交叉核调用契约、局部生命周期/重入及资源/期限边界。仅源码/文本静态检查，不调用业务或系统接口、编译/测试/软件/仿真，不修改飞控或提交/更新PR/安装包。真正受控启动/状态过渡/完成归属和其它六类实质开发继续，不能以新增类数或改动记录推导完成率。

239. 原生一次授权依据移交：OperatorSession将实际已消费的条件账号依据一次移交到强cast的FleetMissionStartupAuthority/固定startupUUID/owner epoch/完整sourceHash，保留原双方SID/角色/主会话与独立审批截止，原剩余只能缩短不新起算。StartApproval进入transferred终态并停止旧Review/Content失效联动，不再有可重复consume/grant；新依据仍实时核真实账号/角色/WTS/owner/原TTL，精确Startup取消，不给旧审批无条件撤销新依据的权限。
240. 新增FleetMissionStartupAuthority.h/.cc/.md：十个实际原生来源、实际目录及提供者、最新非恢复全记录/不可变风险/完整RAW读取/缓存/健康与Leader/真实BeforeArm候选冻结，实际consume→认证移交后才整组本地占用，成功也不是最终StartPermit。接管后独立重验完整源/物理/身份/原最早期限，不继续用旧Review/Content/Approval.valid给跨ARM续权；当前只支持preARM依据，实际ARM仍撤销，尚无发送或受控过渡执行。
241. 分阶段真实前置事实：Readiness新增typed Phase::BeforeArm/ArmedGroundReady与captureForStartup，controlledArm候选用实际armingDeferred/全部其它conditions推导，不过滤blockedReasons；仅普通PX4 POSCTL/nonAUTO/nonHIL/known status/ON_GROUND/not-flying、完整参数版本与起飞推导相符，未知或截断拒绝。原preconditionsMatch/manual-arm风险false仍保留；Review单独汇总全组候选并提供实际目录/提供者native getter，不称机载canArm、遥控中立或净空认证。
242. 原生只读机载身份与接线：PlanManager保留原grounded SourceMatches，另加仅验证私有完整RAW/原操作scope/epoch/计数/实际PRIMARY/cache/READ START时效的passive retained getter；不放宽普通读工厂或允许飞行读取，phase条件由新实际owner独立检查。内部类型/pri、controller真实十对象来源和当前绑定/目标门禁、显式申请/匹配Startup撤销/只读值已接入，草稿编辑先精确撤销新依据，普通上传/刷新/新审阅避免占用冲突；本批未新增UI或发命令入口。
243. 契约审计与工程交接：源码核对本地PX4旧插件先Mission再ARM、MISSION_START自身也可能ARM，未来必须单独正常ARM/seq0原始观测/mode-only适配；本批不调用legacy startMission或假装完成命令事务。同步模块/清单/状态/路线图/矩阵/索引/身份审计与工程交接，只源码和静态检查，未编译/运行/系统或业务API/测试/界面/仿真/刷机，不改飞控、不提交/更新PR/安装包。最终StartPermit、精确受控状态过渡、命令ACK与执行完成及其它六类目标仍继续开发。

## 当前边界

295. 新FleetEventArchiveSql.h/.cc/.md：唯一DB ID/schema/精确表声明、inspect四字段、封存实际run clean核验及strict readPage供Writer/Reader共享，不迁库不降旧门禁；driver前限幅、row codec/hash、EOF/error、页自身txn和合作取消保留。
296. 新FleetEventArchiveHistoryReader.h/.cc/.md：parentless单请求READONLY线程，actual sealed basename/DBepoch/sealRun绑定、前后文件header/hash、目录128段/4096entry/JSON行数及字节有界；一次结果，非global rowId/飞行时间序，catalog全集/全rowhash/跨段原子证明false。
297. Store native catalog/page/cancel与historyCatalog接线：显式配置/consent可离线但不建current库，实际entries生成catalogId，epoch/run私有冻结；每段一filter游标最多128，只0/实际next，换catalog/目录清旧域；错误/取消不回灌live状态、任务或授权。
298. current查询与Reader共单pending槽，discard/源丢失/失败不提前释放，Reader交付后仍等actualfinished；原writer/reader低频状态timer互不误停，late结果同实际QQ/query/kind与inert flags/scope核，1MiB预留预算和上下文字节数对应。
299. pri登记新Sql/Reader各h/cc一次，Reader/Sql不注册QMLtype，无新视觉UI；controller只增historyCatalog值。路线图/矩阵/修改/补丁/静态与ACK审计、模块契约和工程交接同步。完整25目标/六类继续，未编译/测试/运行SQL或目录/hash/helper、改飞控、提交PR/打包。

290. FleetEventArchiveWorker为实际current库全run增协作QLockFile占用，专属线程处理，非阻塞申请、无GUI等待，不当企业权限或恶意外部writer防护。
291. Worker native requestStopAndSeal(1–128段，默认32)冻结维护决定，实际clean/SQL关闭后限定路径/sidecar/段数/库内容复核，旧库同目录不覆盖保留，段满/失败不删或重建。hash仅变化检查，无强原子/失电保证。
292. Store native requestRollover显式确认冻结实际源QQ/epoch/DBepoch/暂停原生序号，无待交付历史或capture重入；actual finished/sealed/clean及固定文件名匹配后沿明确配置新run。停止/退出/源丢失撤销续建，但保留已发生的封存事实。
293. 每次窗口known/unknown、pause/end、序号差额与新run外部缺口显式保留；新请求不混入上一轮数，pending state不被recording覆盖。新runRequested非已正常记录，不恢复权限或完整遥测，未接跨段历史/自动清理。
294. 路线图/矩阵、修改/补丁清单、静态与ACK审计、Store/Worker契约和工程师说明同步。原25目标/六类不收缩；本机不运行SQL/锁/文件轮换、编译或测试，不改飞控/刷机、提交PR或打包。

285. FleetEventBlackBox新增真实native epoch/uint64序号/暂时在栈收据，append后先于旧通知发布；嵌套帧独立退休，clear/import/start-stop不恢复收据、不重置序号。同类型递归身份、QObject删除保护和同线程修改门禁；默认空source明确非null，修复importSnapshot局部变量遮蔽signal。
286. 新FleetEventArchiveCodec.h/.cc/.md：精确18字段包裹/4字段事件/固定诊断摘要，严格JSON/UTF-16/typed-null/数量大小边界，只存未验证哈希引用而非原payload、凭据或精确GPS；历史canonical/hash只作损坏检查，权限/签名/执行/完成均false。
287. 新FleetEventArchiveWorker.h/.cc/.md：parentless专属线程SQL连接，已有库先只读验证精确schema与数据型，未知库不删/重建。七整型政策、queue+in-flight/batch/主库页/历史页有界，事务提交后durable；有限row淘汰、缺口摘要和clean/discard区别，run元数据长期轮换仍未实现。
288. 新FleetPersistentEventStore.h/.cc/.md及pri/type/controller接线：默认off，native显式确认/配置才异步开库；实际BlackBox绑定/enable窗口新收据、encode前序号、250ms合并状态、source丢失停止。历史最多一个待GUI交付请求，结果只读值，无审批/实时规则/replay/控制恢复；线程结束才deleteLater，不wait/terminate。
289. README、路线图/矩阵、索引、修改清单、事件回放边界、静态审计/ACK审计与工程交接同步。事件落盘仍是诊断摘要且未运行，哈希非认证/匿名化、clean非完整性。原25目标/六类和最终启动/完成未完成；未编译/测试/建库/SQL执行、改飞控/刷机、提交PR或打包。

280. ExecutionProfile新增startupPolicy v2/8字段：原v1/6严格兼容，local7/heading3必须显式真实有限数及精确键，局部三速度不大于原全球上限，adjacent≤deviation/travel≥adjacent；outer schema1/adapter不变，null数值明确拒绝，不补默认或伪造飞控指令。
281. 新FleetMissionStartupMotion.h/.cc/.md及pri各一次：LOCAL精确6/binary32独立NED位移/速度，GP精确9的已知heading圆周差、原基线/相邻/累计budget；双编码0.02度保守外包、同heading不累计，失败不改累计指针，无来源或许可逻辑。Transition仅stage2消费，全ordered journal重建保留两个风险revision域与queue/final精确投影。
282. Readiness/READ/实际Txn接线：v2明确actual ODOM NED/NED、resetPresent/autopilot8/source/valid及age≤policygap，原native21→stable20参考不认证两流同origin/sample；两个motion旗标加入READ物理冻结，Txn真实6/8门禁修复，原完整normalize/profile/冻结风险与新审批保持，pure实际目标政策getter不反调OS/fullgate或接caller bool。
283. Vehicle v2原始ODOM periodic clock：final queue实际owner政策冻结，真实row初始化、每包含同值、closed-step tail均入既有maximumObservedRawGapMs；非法/缺基线/时间倒退/溢出拒绝，fresh末包不清旧gap。v1不增加周期前置，不将ODOM pose作为第七risk叶、不改step32/read34/coordinate10/13或新请求/控制命令。
284. 同步原/新政策模块、路线图/矩阵/索引/清单/有限静态与身份审计/工程师说明，历史schema1固定XYZ/heading语义明确限定；独立源码串审及有限文本/链接/注册/资源核对，不编译/解析器/测试/接口/飞控/刷机/PR或打包。合法HOME/受信origin/内部有效分支、净空/最终StartPermit/完整时序/完成及原六类/25目标继续。

275. 新增 src/Vehicle/FleetStartupOdometry.h/.cc/.md：实际原始 MAV2 ODOMETRY 被动解码，合法尾零省略1–233/显式扩展presence、uint64精确字符串、原始有限数字及NaN/null未知状态；decode50/reference42固定资源，稀疏协方差不误拒，quality分值不进入稳定参考身份，所有认证/许可标志false。
276. Vehicle独立原生账本：full27/reference21/非回绕receipt、content/reference/clock revision；actual PRIMARY/pre-plugin/sender/ID/PX4/version/flight/init绑定。prepare记录reference shadow及降序，不混sample/GCS/其它消息时钟；token/epoch提交与精确discard不覆盖nested新行。sticky everObservedRawFrame在实际来源核验后decode前置true，跨失效保留，不把invalid/discard/lost伪装成初始缺流。
277. Readiness与地面READ：仅冻结去age后的20字段参考投影/风险hash，常规收包/time/pose/速度/quality数值不反复失效；source/epoch/reset/frame/状态/observed ABA仍变化。已出现的流须actual available/fresh且无显式无效，真正初始缺流仍未知且不认证参考；READ采集物理基线新增四个ODOM稳定键，禁止中途隐式rebase。
278. Transition与传输守卫：前后读取实际native21→20，核原baseline hash/ref revision与source；旧six-channel/coordinate及journal严格限制保留，不用ODOM证明z_valid/home.valid_alt/same sample或放开运动转向。Vehicle整ODO栈及PlanManager retained直接quiescent拒绝接收中借用；原read34/startup32/coordinate10/13不扩schema；helper只在qgroundcontrol.pro与Vehicle CMake登记。
279. 同步模块/路线图/矩阵/索引/清单/有限静态与身份审计/工程师说明；限定独立只读审阅关闭sticky缺口，文本/路径/构建登记检查，不运行解析器/编译/业务API/测试/仿真/飞控/刷机/PR或打包。合法HOME分支/local origin/运动转向、净空/最终StartPermit/时序/执行完成及原六类/25目标继续。

270. 新增FleetMissionStartupSeries.h/.cc/.md：actual初始事务一次绑定、每轮独立child/最新已初始化owner不回退，同原任务组/记录/部署；8个ground容量（空候选也计），无淘汰/自动reset/restore。来源丢失/旧owner重用保持粘性拒绝，脱敏摘要不放行动作。
271. Terminal原生摘要6：factory实际完整authority context仅去remaining后sha256，换generation前生成、commit一次冻结；terminal20不递归复制旧auth，而完整部署/成员/payload/当前风险/真实steps保留。显式fresh native捕获可更新新引用，比较capture不覆写、retained要求exact native_saved，不恢复旧证明。
272. 多轮controller冻结路由：最新QQ/UUID/epoch/generation/hash前后核对，精确退休旧转交链→新READ/已阅/账号审批→新Authority/新child；root SourceMatch/Context分别用冻结QQ和当前consumer，不动态换风险源。初始化/终态对象禁止再次factory覆盖。
273. 非执行入口与回调边界：actualPreviousStartup getter/pri/内部type注册；null-safe脱敏snapshot、pureconsumerBasisCurrent、空候选同basis精确复用/不同新basis新child、准备态不自拒；编辑/登录/退出捕获旧QQ+UUID再取消，重入入口busy不加入自身门禁。
274. 模块/路线图/矩阵/索引/清单/静态与身份审计/工程师说明同步，独立只读串审及文本/路径/注册/资源检查；未编译/运行/测试/API/飞控/PR/打包。容量/新部署reset、HOME/local frame/运动转向、净空/最终StartPermit/时序/完成及原六类/25目标继续。

265. GroundAuthority一次事务移交：actual8/context27、nativeGroundReview/目录提供者与冻结pending27，Session6→Authority10；同UUID连续占用，旧28/watcher退休，显式revoke链精确至实际Txn终态排空，不释放replacement。
266. Session新独立30仓：原28+transactionEpoch+transactionAuthenticationId，保留原22/28/两身份及首查询前budget；历史duration不当remaining，新真实remaining在全查后返回。旧24不覆盖，logout/WTS/expiry/ownerdeath精确清理。
267. StartupTransaction新增强Ground入口与实际第二实例：换generation前先核actual消费者，不覆盖旧终态；真实33→34读取，整组固定seq0→normalARM-only-unarmed→mode-only，已armed阶段不造queued/receipt/ACK，纯阶段依据绑定完整私有参数/风险/UUID。
268. Transition/controller前置和边界：已armed实际UUID/opcode0/2映射与持续Seq0 journal；首次Seq0要求真实周期基线/age≤policygap，不先发后补。准备态同ID移交豁免、身份/编辑精确取消；第二事务已使用后禁止旧第一owner重审，实际多轮owner路线仍待开发。
269. 模块/状态/路线图/矩阵/索引/清单/源码静态与身份审计/工程师说明同步；限定只读交叉审、文本/路径/注册/资源核对，不编译/运行/测试/API/飞控改动/PR/打包。完整HOME/local frame/运动转向、净空/最终许可/时序/执行完成及原六类/25目标继续。

260. GroundStartApproval一次移交：exact14/native已消费新22与实际GroundAuthority/startup/epoch/sourceHash绑定，Session一次退休22换新28；Ledger transferred停止旧watchers/不再给旧grant，显式旧revoke精准转发对应新owner，真正终态无占用才退休引用。审批完成queued idle通知补齐，不用busy标量续权。
261. Session独立GroundStartup仓：新28保留原22/两身份，actual新owner/UUID/epoch/hash、原始与transfer budget期限；成功6返回所有核对后的真实remaining，旧历史duration不能续期。Current/撤销/ownerDeath/锁定/退出/过期精确，不清replacement或无关旧证明。
262. 新GroundStartupAuthority.h/.cc/.md：actual10与private27/full record/新RAW/完整风险/typed冻结ground armed与目录/健康提供者/sourceHash，pure4 callback、消费/移交前后独立全源核对，最早READ START与原双方期限只缩短。移交后不问oldReview.valid，取得整组本地占用但不发读取/ARM/MODE/回滚，无最终StartPermit。
263. Backend/pri/type接线与精确cleanup：current/request/revoke、实际10来源和完整绑定目标回调；准备态不过早自拒，接受态不靠旧ack。编辑/身份变更/退出先撤新Authority；撤proof通知即使删除/替换owner仍释放捕获的旧Swarm UUID，不误释放replacement。
264. 模块、状态/路线图/矩阵/索引/清单/静态及身份审计/工程师说明同步；限定独立只读串审10/27/28/6/10/pure4、文本括号/空白/路径资源，无编译、测试、运行API、飞控修改、PR/打包。真正受控模式/分阶段动作、完整HOME/local frame/运动转向、净空/最终许可/执行完成及其它六类/25目标未完成。

255. 新 GroundStartApproval.h/.cc/.md：新已阅 Review20、actual九对象、完整最新非恢复记录/全风险/新RAW与 frozen armed整组进入新的两账号条件意图；真实提示使用独立新action，不复活旧审批，技术未验证项保留。
256. OperatorSession新增独立 ground认证仓/proof22/wrapper5：actual强Approval/epoch/reviewID/epoch/hash、一消费身份和不同SID，首OS检查前固定原budget，仅缩短原主/次/Review deadline；精确撤销/ownerDeath/锁定/退出/过期，不清无关旧证明或replacement。
257. Native consume/grant14与public receipt27分离：consume前后full九来源/Review20/全记录/真实目标/目录健康提供者门禁；pure callback不反调Review/Session/OS，Session自己重核真实身份，成功先commit consumed再按generation/approvalId/grantId异步通知。无SID/RAW public泄露、命令租约或飞行包。
258. Review acknowledged native九对象/target/provider getters、inactive真实credential modal主身份支持与后端request/revoke/current/source callbacks/pri/type注册已接；active READ和真正消费不放宽，编辑/身份切换/退出精确撤销新ledger，忙态隔离竞争请求，无新增按钮。
259. 同步模块、清单、状态/路线图/矩阵/索引与工程师/审计文档，限定只读交叉核 actual9/20/27/22/14 与原期限/pure callback/modal/lifecycle；文本括号/空白和路径资源复核，不运行接口、编译/测试/刷机或更新PR/源码包/安装包。GroundStartupAuthority/模式续作、HOME分支/local运动转向、净空/最终StartPermit/完成及原六类/25目标仍未完成。

250. 原生终态新引用：实际旧 Transaction/最新完整非恢复记录/原载荷/全组与提供者冻结，仅从未可能排队 MODE 且已排空的地面 POSCTL 机体可入场。原生保存完整 20 字段引用，风险投影前精确比对原文，current 捕获不覆盖引用；新 generation 清空引用，旧身份/批准/期限不恢复。
251. 专用强 owner READ：PlanManager/Vehicle 新 armed/disarmed-POSCTL 三态 admission、实际 Review/epoch/scope/PRIMARY、逐包 mandatoryFence 和 completed retained 收据。成功 9 字段与 RAW 34 字段保持，原未解锁工厂/来源校验不放宽；纯所有权查询不反调 full gate。仅本 READ 接收栈可续下一包，不能容忍 ARM/飞行/版本/坐标接收栈。
252. FleetMissionArmedGroundReview.h/.cc/.md：完整原始 INT 全组读取/载荷比对、当前风险与原基线/参数/版本/健康/目录核对、120 秒采集与最早 READ START 新 30 秒截止、已阅/取消/排空。逐机 armed 冻结、不删成员或自动重 ARM，新风险完整保留 CURRENT/REACHED 与未知项，无旧证明消费/恢复或飞行指令。
253. RolePolicy/Controller/工程接线：专用只读 reassess 角色、内部类型/pri、实际八对象回调/全绑定全组、backend request/ack/cancel及只读摘要；编辑/登录/退出精确取消，其他部署/刷新/启动避免占用冲突。没有新增界面按钮，public/事件不泄露完整身份或私有原始依据。
254. 同步模块、清单、路线图/矩阵/状态/索引、有限静态与身份审计、工程师交接。修复已发现的 RAW 投影伪造风险、own READ 第二包自拒绝和不存在的 ParameterManager 信号名称；仅文本/路径/签名/资源/差异复核，不做编译、解析器、测试或业务/系统接口调用。新独立授权与模式续作、HOME 有效分支/local frame/motion/转向、最终 StartPermit/净空/完整执行完成及其它六类保留未完成。

244–249为上轮受控启动源码增量，上轮有限源码静态收口已完成；未编译/运行/测试/仿真/刷机、改飞控、提交或更新PR/安装包，编号不是功能验收数量。

244. 原生Vehicle固定三步：直接wire seq0、普通ARM(param2=0)、mode-only AUTO_MISSION；实际owner/PRIMARY/epochs/by-value发送围栏，单lane/3保留scope，无legacy启动/force/自动retry。schema2/32实际queue及RAW来源、持续256叶风险journal、逐通道同值/首尾gap与historyDuration；lane关闭不终止历史，seq0新CURRENT关闭等待但不是无线因果认证。
245. 独立LOCAL/HOME RAW与source epoch/raw及risk hash/非回绕计数，HOME geo/local xyz及time_usec更新风险保留，不改原六通道schema。Readiness保存完整原RAW和stable risk身份；配置指纹仅将armed/flying交由typed独立核，不删除参数/版本/目录/其它原风险。
246. 原profile可选startupTransitionPolicy六字段/七字段postModeEnvelope，保留进原计划/载荷/归档/审阅身份；缺政策不补默认，不续原期限。当前HOME/参考未知变化停止、航向和local XYZ保持原依据，完整再审/local运动/转向仍待开发。
247. 新StartupTransaction/Transition：实际八来源强cast、原Authority/Session一次身份与同UUID占用移交、原最早预算；全组seq0→ARM→MODE屏障，100ms节奏/6秒阶段，typed逐叶历史/原生计数核对，ACK与状态分开。取消保留partial queue/可能已飞行事实，仅本地栈排空后释放，不回滚或DISARM。
248. Controller/工程接线：新内部类型/pri、实际八对象与全绑定/目标回调、显式backend request/cancel及只读状态；编辑/身份切换/退出先cancel实际Txn，旧Authority显式撤销匹配转发，不沿旧Review.valid续授权。没有Start/ARM按钮或自动启动，未在本机调用这些函数。
249. 模块、状态/路线图/矩阵/清单/审计/工程师说明同步。仍非最终StartPermit、机载因果/身份认证、净空、完整HOME再审/local motion/转向或执行完成闭环，原25项/六类范围继续；严格拒绝路径或源码存在不是完整产品完成。

- 这是源码级 groundwork 和静态审计结果，不能证明悬停稳定、断桨能力、避障能力、集群自愈或真机安全。
- 当前没有执行 Qt/MSVC 编译、Mock/SITL、日志回放、硬件测试、刷机或放行。
- QML 函数/属性不是不可信插件的 C++ 权限边界；工程师仍需补齐真实参数/命令 ACK、权限、签名、沙箱和运行时验证。
- 多机列表已收口部分入口，但模式菜单、任务上传/清空和 legacy Guided API 仍存在直接 Vehicle/PlanMasterController 路径；本轮未假装它们已经成为 C++ 权限边界，需工程师按交接清单继续分层。

## 工程师入口

1. 阅读 [`docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
2. 按 [`docs/MERIVUS_SWARM_TARGET_MATRIX.md`](docs/MERIVUS_SWARM_TARGET_MATRIX.md) 对照每个目标的当前状态和缺口。
3. 在受支持的 Qt/MSVC 环境重新生成工程并编译，再按交接文档执行 Mock/SITL、故障注入和硬件放行流程。

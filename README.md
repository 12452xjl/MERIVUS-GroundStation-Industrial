# MERIVUS GroundStation

## 546：最终本地源码交接

按用户确认，本次交付限定为本地地面站源码与工程交接文档；外部接口、跨站服务、动态插件运行时及实测验证明确保留为未完成项，不宣称原25项完整产品能力全部落地。

最后补齐五条真实操作链：[规则待审](custom/src/Swarm/FleetRuleReviewPanel.md)、[Run检查点](custom/src/Swarm/FleetRunCheckpointPanel.md)、[用户模板](custom/src/Swarm/FleetUserTaskTemplatesPanel.md)、[日志留存](custom/src/Swarm/FleetArchiveMaintenancePanel.md)、[四受众报告](custom/src/Swarm/FleetMissionReviewReportPanel.md)。同时修正检查点时间戳窄化、规则整数/纯值边界、报告生成后摘要失配、模板/报告视图状态命名及失败后旧副本可读性。限定作者与交叉源码静读已完成，未编译、运行项目、测试、仿真、刷机或修改PX4。

先看[最终范围与外部依赖](docs/handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md)，再看[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。不制作阶段包；最终只封装当前源码，不是安装程序或放行版本，未提交、推送、合并或更新PR。下方545及更早均为历史，不代表当前剩余工作。


## 545 当前：证据链文件操作已接通

现指挥中心增加完整证据链的明确冻结保存、后台验证打开、独立只读历史与四条分页；不会替换活动链或恢复旧批准。[操作说明](custom/src/Swarm/FleetEvidenceArchivePanel.md)与[原生合同](custom/src/Swarm/FleetMissionEvidenceChain.md)说明路径确认、写入/读回状态和资源限制。

与544公共航线修复一起继续面向最终源码交付。无阶段包、构建、运行或测试；源码不是已验证安装包。

## 544 当前：公共航线进入实际逐机任务

修复普通任务公共航线只用于审阅副本、实际成员缺路线而无法构建载荷的问题。合法且缺省的成员去程现在在编译时展开；显式坏路线、覆盖任务、未支持要求及未知参考不被默认补齐。详见[编译合同](custom/src/Swarm/FleetMissionCompiler.md)。

继续面向一次完整源码交付，不制作阶段包。未编译、运行、测试、仿真、刷机或更新PR；不称为已验证安装包。原25项目标尚未全部完成，下方为历史记录。

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

## 510 当前：原生Run语义事件接入任务历史

受保护原生TaskRecord提交后的注册、MODE本地交接、终点候选、连续性异常、源退休和观察关闭，新增独立低频事件通道进入实际BlackBox及其持久归档，不再只停留在内存观察通知。逐包计数／年龄／临时sourceCurrent不作为事件变化；同Run的异常位图保历史已观察事实。

独立FIFO最多512行／2MiB canonical payload、单条4096字节，每turn最多64条；不是堆内存硬上限。溢出保旧事件并累计缺口，原生临时收据只在实际发布期间有效；缺口不归入假任务。Root另记本地接纳／未记录／未知，BlackBox接纳不等于SQL提交或完整历史，持久录制仍默认关闭。

新v3固定12字段脱敏投影保28字段runObservation；原v1／v2、SQL及历史查询schema不变。实际按taskIdHash下钻和两历史面板新增只读摘要；现报告将reviewRequired=false普通转换作为info，异常只作warning人工复核，raw保原事件。日志timestamp是实际入日志的本机UTC，原捕获UTC另保，不对队列事件偷偷排序或改写时间。

[事件合同](custom/src/Swarm/FleetRunObservationEvent.md)、[实际日志接线](custom/src/Swarm/FleetRunObservationJournal.md)、[25路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[60项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批8源码／接线＋17文档，新路径仅两份模块合同，无新代码文件／资源／外部依赖。

这仍是本地Run观察，不是任务Completed／Failed、远端实例认证、正式结果、占用释放或飞行许可。原25目标与完整执行／可信性能／自愈机载改派／跨站协作／长期运营／真实模型插件六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按本批路径上调。

仅源码、直接静读及普通文本核对；未编译、执行解析器／helper／业务／QML／SQL／认证／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 509 当前：人工单流受控启停与状态回读

新增独立视频控制动作：从本页真实完整读取中一次封存基线，人工选择相反目标，另行确认并由不同账号批准，再执行单流 STOP／START 和状态回读。READ 权限不兼作写权；停止后恢复必须新读取、新审阅、新批准，不自动重试或反向回滚。

Review 最早30秒包含准备、审批和执行；每个发送阶段最多4秒。只报告本次状态值匹配，不认证实际视频送达／停止、带宽改善或飞行安全。自己的实际动作之后仅 RUNNING 位的 Info 更新可沿原配置代次继续；其它配置变化或 ABA 仍使依据失效，不从新报告重建权限。

[本批30路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师64项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[控制合同](custom/src/Swarm/FleetCameraStreamControl.md)。14源码／接线＋16文档；只新增Control h／cc／md三路径，无新增依赖。

原25目标和六类实质剩余不缩减，整体约40%完成／60%剩余仍仅源码工作量粗估。本阶段未编译、运行项目实现、测试、仿真、连接硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 508 当前：人工相机流信息／状态读取

新增人工单流相机 Info269→Status270 查询：独立 READ 权限与真实账号／角色／Root 意图，经 Swarm 私有服务、Registry 原绑定、Vehicle 原生 Run 和现有共享消息通道，接到指挥中心／规划页面板。打开页面或原五秒策略不会自动查询；不启停流、配置、降速或恢复。

原入口总8秒包含授权准备，每阶段最多4秒，accepted ACK 后报告窗口最多1秒且只缩短；每项至多一次实际发送。允许真实来源满足检查的飞行状态 metadata READ，不把 RTT 的地面限制直接套入。取消保留已可能发送／ACK／报告事实，不宣称撤回。

ACK 与269／270无nonce，本地匹配不证明无线因果、实际送流／停流、完整目录或带宽变化；270索引兼容性待工程师确认，不支持不回退0／all／广播。complete 不等于两份报告此刻同时新鲜。远端受控视频降级与恢复仍待开发。

[服务合同](custom/src/Swarm/FleetCameraStreamCapture.md)、[面板合同](custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[28路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批14源码／接线与14文档；新增 Service h／cc／md、Panel qml／md共5路径，没有新外部依赖。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅为源码工作量粗估，不按批次、路径、接口或局部保护数量上调。完整任务执行、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 507 当前：原生相机流报告进入实际只读消费者

新增原PRIMARY相机VIDEO_STREAM_INFORMATION269／STATUS270被动账本，经Registry实际绑定一次读取、Policy第五参数和Guided原五秒周期接入既有指挥中心摘要；不发新请求或控制命令。另修上一轮留下的setVideoMode／setPhotoMode实际caller，以及其toggle／reset模式意图衔接。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

[完整合同](custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 506 当前：相机／视频生命周期与本地启停意图

本轮修改现有CameraManager／CameraControl→VideoManager真实调用链：本地播放意图、原相机／流选择代次、有限接收器恢复、活动飞机切换和通知回调后的对象存活检查；共享QmlObjectListModel补通知后的自身存活尾核与匹配reset。不是新增只读helper，也不是完整Fleet视频降级／受控停流事务。

手动停止不会被接收器完成回调或切车直接恢复；新飞机无热流时清空旧热流地址，修Taisync地址解析自递归。原始异步完成只有STATUS，没有请求ID，本地串行保护不等于wire／远端因果认证。

[生命周期合同](docs/handoff/MERIVUS_VIDEO_LIFECYCLE.md)、[19路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[50项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505 当前：修现有机载日志缺块补读链

本轮修现有LogDownloadPage→LogDownloadController→LOG_REQUEST_LIST／DATA→本地文件的真实消费者：同一目录／批次来源、冻结选择、有界缺块补读、完整写入后位图与去重计数、NewOnly partial及完整刷新／改名后才显示Downloaded。不是另建只读建议helper，也不是完整Fleet控制中心补传。

换车／换PRIMARY／断链退休原下载和目录资格，不禁用原通信丢失检测；失败、取消与超时保留.partial，不自动重启或续传。LOG_REQUEST_END仅同原有效来源一次best-effort，没有ACK或恢复认证。

[下载合同](docs/handoff/MERIVUS_LOG_DOWNLOAD_RECOVERY.md)、[14路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或重试次数上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504 当前：独立人工单次定向往返耗时采集

504新增独立人工单次定向TIMESYNC往返耗时采集：实际主账号／Role／Root原意图→Swarm私有服务／原生Run→真实同车绑定review→Policy及可见面板。最早四秒包含授权核验和后续100ms轮询，不自动采集、重试或续期，不回退PING或广播。

当前仅有新鲜原生证据的PX4多旋翼地面分支；收到同源匹配回显后，可给本地交接→RAW入口耗时。包含排队、通信和处理，不是纯网络延迟。容量／质量／丢包仍未知，单次样本不使策略normal，也不自动调视频／速度／遥测。

[合同](custom/src/Swarm/FleetLinkLatencyCapture.md)、[31路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全部未执行](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503 当前：真实被动接收窗接入只读策略

503将实际PRIMARY／本车默认component的原始解码帧接到独立五秒接收窗，经Registry实际绑定读取、Policy独立消费、Guided原五秒周期和指挥中心只读摘要。记录接收帧数、编码MAVLink字节率与最大内部间隔；不把它们当可用带宽、RTT、丢包率或远端发布率。

固定native22／window9与Registry wrapper7；新Policy在原20字段外增加来源与localReception10，四QoS仍未知，完整窗不能变normal。旧RSSI仅排除−1未知哨兵，不认证其余负缓存。已有显式遥测配置权限与期限不变。

见[完整合同](custom/src/Swarm/FleetLocalReceptionObservation.md)、[23路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[40项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502 当前：可选遥测配置的可见操作入口

502新增FleetOptionalTelemetryPanel可见界面，在指挥中心和地图规划页复用同一actual Root。操作员显式只读采集→另行审阅统一目标→两项确认后独立账号审批→逐项配置与回读；固定62／74／83，原501实际后端和授权合同不变。不会自动采集、Apply、续期、重试或回滚。

本页仅由真实返回result5及同一当前workflow24的ID／代次／hash建立拥有关系；共享状态或历史不领权。稳定来源只取原scope／主身份5／双政策9，自有ID／代次／状态推进不误取消；完整sourceToken仍在新请求前精确比较。审批不借全局busy自拒，关闭／换源／输入变更退休本页资格并精确取消原ID，晚返回不能复活。

新增的是源码UI接线，不代表界面已运行或实际频率已生效。见[面板合同](custom/res/Merivus/FleetOptionalTelemetryPanel.md)、[24路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501 当前：独立授权的地面遥测配置与回读

501新增独立双账号的地面可选遥测配置：从未来新成功Capture的三条真实native上下文封存不可序列化Basis，在同一最早30秒内独立审阅／批准，再逐项511→510并最终三项510复核。仅固定62／74／83、同一正int32且精确REAL32间隔，目标不得比任一原正基线更短；不启停流、不降频P0、不改视频或编队速度。

start仅认领ready、不写包；100ms后推进九个wire阶段。每条至多4秒，510 accepted ACK后至多1秒且只缩，所有阶段与审批共用原Capture最早30秒。慢链路可能无法完成；截止／失败／取消保留已发事实，不重试、不自动回滚。三项最终回读只给reported-values-match，不认证无线因果、实际发布率、带宽或飞行许可。

新增的是后台显式request／approve／cancel与只读状态，尚无新可见操作按钮。见[模块合同](custom/src/Swarm/FleetOptionalTelemetryConfiguration.md)、[30路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)及[46项全未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500 当前：显式只读采集三条可选遥测间隔

新增Swarm私有持有的FleetOptionalTelemetryCapture，显式单车62→74→83，实际Session主账号＋独立fleet.telemetry.interval.capture政策＋原Root/controller意图。接纳后100ms轮询才发；原入口固定30s总期，native每项一次510／4s，accepted ACK后至多1s且只缩，必须原ACK与目标244双端齐全。最终write前复核授权、物理来源、完整载荷并冻结receipt基线；三项同原version／transport／mutation／已绑定ledger，已采语义A→B→A不复活。取消保已发／partial事实，先存历史快照再精确释放原query，不宣称撤回或实际频率已生效。

实现与未证明边界见[模块契约](custom/src/Swarm/FleetOptionalTelemetryCapture.md)、[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[40项工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)，工程场景全未执行。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前：实际消息查询／配置排他与有限等待

510／511／512现走同一Vehicle级跨component专用lane：外调前预留UUID／不回绕serial，固定原提供者QQ、PRIMARY弱控制块与实指针、transport代次及原固定期限；原nativeVersion直接入队也接入，但保原地面／参数／实际授权fence和单次3秒，不借generic条件放宽。generic初连不要求MVM已登记或握手已完成；版本资料首次锚定与实际transport变化分开，native批准依据不自动rebase。

本车默认component的任意511只在最终actual write前，以同UUID一次预留推进不回绕mutation generation，并全清498三report及接收窗；ledgerEpoch为UUID:十进制generation。writeDepth贯穿write与通知，临界内原包准入／getter不可当前。未发送拒绝不冒称已配置；已可能write的代次／requestIssued不回滚，耗尽保守停止而非重用。498观察15／Registry review21／comparisons6保持，后续只读重采会看到旧基线失效，既有值快照不自动变成实时通知，须新被动资料与新完整5秒窗，不等于实际生效。

[5源码＋13文档=18路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[42项未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前：可选遥测原生观察进入只读数值审阅

Vehicle只观察原始PRIMARY的MESSAGE_INTERVAL244及固定NAV_CONTROLLER_OUTPUT62、VFR_HUD74、ATTITUDE_TARGET83。244保原signed间隔：−1已报告disabled，0已报告not-available，正数为报告间隔，低于−1为invalid；未观测是null而非−1。报告fresh与available分开，所有值均非身份／实际发布率／带宽认证。

Registry仅从现有friend建立的实际Vehicle绑定读取一次原生快照，前后核记录与绑定，不以metadata／护照／调用者Map替代。null／0仅无提案，正间隔须int32整数且精确REAL32；三条当前新鲜正基线都已知、提案间隔均不小于原报告间隔，才给数值不增频结论，仍snapshotOnly且所有许可false。

现有custom Guided五秒策略快照把Policy正rateHz换成ceil(1000000/rateHz)的只读间隔提案，经实际私有_swarm.fleetRegistry调用并附optionalTelemetryRateReview；0／未知不变为停流／固件默认。Policy源码、P0流、旧PID／ESC与发送器不改，公开facade不暴露原生setter或新增apply。

[5源码＋13文档=18路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前：修现有遥测频率链的本地结果归属

发现任意live511 ACK可推进PID／ESC后，现把两链改为专属一次handler，补StreamConfig requestId／迭代重入、公共511原PRIMARY及持久provider来源核对和超时隔离。保原PID档位、ESC5Hz／1Hz及手动默认恢复；不把此修复说成新的Fleet自动降频。

[6源码＋12文档=18路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36项工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。同source重复ACK仍无wire归因证明，实际降频授权／244回读／收包测量及完整启动／任务闭环继续。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前：P0补完整起飞垂直编码参考

在495 top→首NAV连接基础上，Contact保同次成功支承绑定的原起点／原高度窗口，Waypoint用full current与full top两端编码盒补垂直参考体，实际进入原环境检查。新支承接触只限指定地形完整交集位于同有限pad，不把声明几何当真实轨迹／许可。

[精确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)4源码＋15文档=19既有路径；[36项工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前：P0起飞参考高度区间接入首NAV接受域

原Readiness已有编码上下界，但Joint只用名义top，Waypoint遇TAKEOFF会断开普通NAV连接。现在由同次原GPI9／PM29／wire独立重算完整top区间，连首NAV全接受帽，protected／support实际进入原五类环境检查。不是新增真实起飞轨迹或飞行许可。

[范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)2源码＋14文档=16既有路径；[36项工程场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前：保护Run历史报告可显式保存与重开

现有保护记录report8增加独立文件codec及原报告面板操作：保存消费同份原生JSON并绑定原报告ID／hash，重新打开返回同一历史材料，不重新捕获或生成身份。路径选择后还须明确确认；导入标记为外部只读历史，不进入旧full9报告槽或原生任务目录。

[修改范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)为4源码／UI＋14文档=18路径；[36项工程场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前：遥测跨Run元数据检索与同文件摘要打开

新增实际记录驱动的跨文件检索：机号0／消息号-1分别表示全部，消息0仍是HEARTBEAT；逐文件给出命中、无命中或不可验证，坏文件不当零命中。每批最多64文件，130MiB读取预留，按最后检查文件推进，不自动续读。

[原生Reader](custom/src/Swarm/FleetTelemetryArchiveHistoryReader.md)、[Archive](custom/src/Swarm/FleetTelemetryArchive.md)与[实际面板](custom/src/Swarm/FleetTelemetryHistoryPanel.md)已接独立search；沿原完整catalog／ordered manifest及私有目录身份续批。打开只用当前成功hit原fileHash进入旧单文件首页，首页仅按机号筛选、可能没有该消息命中，不恢复任务或权限。

范围[6源码／UI＋14文档=20路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，无新文件／资源／依赖；[36项后续场景](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。UI/UX仅静读通用交互规则，不运行技能脚本或界面验证。原25目标与六类剩余继续，整体约40%／60%仍仅源码工作量粗估；本项不是全历史持久索引或完整运营完成。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前：执行观察链的同步回调与对象生命周期

保留原isConnected／isMockLink／isLogReplay三项live检查，补逐外调的原对象／PRIMARY控制块／版本／槽代次尾核及探测重入保护；不通过删检查或只信缓存消除回调。Vehicle构造期一次保留原Manager／Protocol／MVM／Toolbox的QPointer，Run同样保留原app／Toolbox／MVM，失效后不从旧raw重新构造或重绑新源；绑定／RAW／recheck／发包前和Run／Txn各调用者均核同一原上下文。

[Run](custom/src/Swarm/FleetMissionExecutionObservation.md)新增私有变更代次，回调前固定值快照、返回后先核QPointer和代次再重新定位原队列行；[任务记录](custom/src/Swarm/SwarmMissionOrchestrator.md)不跨owner回调持QHash元素指针，返回后按原id重找并比完整24字段。已发生实际write的commit事实不因后来源失效／退休而删除，修复不新增命令或完成判定。

范围[7源码＋15文档=22路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36项全未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。无新文件／UI／资源或依赖；完整动态路径、StartPermit及原25目标六类实质开发继续，整体约40%完成／60%剩余仅源码工作量粗估。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前：LAND接受区域的完整声明参考体

[航点／LAND审阅](custom/src/Swarm/FleetMissionWaypointSwitchReview.md)在原NAV接受帽和连接之外，增加着陆入口参考与完整XY下降参考体，实际进入原覆盖／走廊／地形／障碍比较。不是只看中心线，也不假定PX4一定先平移后垂降；XY尚未收敛时下降的声明连接也在新参考体内。

[接地模型](custom/src/Swarm/FleetMissionContactModel.md)只从同次已成功绑定的LAND支撑组件提供原生参考点／有限pad。缺原来源、分支不兼容或完整入口高度不支持时保持不完整；只有下降support的指定地形交集完全在原有限pad内才可豁免，机体、障碍及入口连接没有豁免。

[4源码＋15文档=19路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。无新文件／UI／资源或依赖。实际动态入口、可扩大的高度接受半径、平滑器、真实地面／时序及StartPermit仍未证明；原25目标和六类实质开发保持，整体约40%完成／60%剩余仅源码工作量粗估。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前：原29参数末端封条与完整覆盖索引

[原生启动封条](src/Vehicle/FleetStartupMemorySeal.md)现在必带实际ParameterManager、首次固定的PRIMARY弱控制块及原审阅29参数；[事务](custom/src/Swarm/FleetMissionStartupTransaction.md)入口先核原来源，各stage和最终native queue复用，不在最后回调后漏掉参数变化。等值原始回执可按原规则刷新行龄，但不延长原事务／阶段／审批截止。

[几何核心](custom/src/Swarm/FleetMissionClearanceGeometry.md)新增完整索引覆盖口，已接[启动整域](custom/src/Swarm/FleetMissionStartupDomainReview.md)和[NAV连接](custom/src/Swarm/FleetMissionWaypointSwitchReview.md)。索引只去掉严格分离的原凸柱，完整查询面仍交原并集算法；地形水平覆盖用独立全Z索引，不借接触高度删候选。共用原2M工作／2048片段预算，可能因准备成本更早Unknown，不承诺普遍提速。

范围8源码＋16文档=24路径，无新文件、UI、资源或依赖。见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项全未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。真实动作／动态轨迹、完整StartPermit和正式任务结果仍未完成，原25目标及六类实质剩余保持，整体约40%完成／60%剩余仅源码工作量粗估。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前：受保护旧Run记录的独立报告入口

[独立保护缓存报告](custom/src/Swarm/FleetProtectedRunReport.md)已从同一Orchestrator私有TaskRecord接到现有报告面板。轻量目录不复制全部观察正文；操作员明确选择任务／观察修订并确认后，才捕获一份报告。关闭、owner销毁及修订耗尽保留原历史边界，不套用当前Episode来源或补造完整Run字段。

新报告可人工选择复制原生完整JSON，不进入原完整Run报告槽／文件读写／交接接口，不自动保存、发送或恢复权限。选择、捕获、被动失效及清理均核本区票据／代次；目录推进不自动更新已冻结报告。

范围8源码／UI＋15文档=23路径，仅新模块说明1份，无新源码文件、资源或依赖。见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：执行观察回退隔离与斜向NAV连接几何

[运行观察](custom/src/Swarm/FleetMissionExecutionObservation.md)在正式观察到任务序号回退时保留原报告、计数和既有历史，结束该机的后续关联，避免用回退前终结引用拼接新结果候选；不自动停机、释放占用或判任务完成。

[普通NAV审阅](custom/src/Swarm/FleetMissionWaypointSwitchReview.md)将相邻接受矩形的大AABB换为XY凸包×完整高度区间，机体／支承余量同时扩bounds和边平面；环境接触经原索引粗筛后核完整凸域，减少斜向无关角区的假阻断。它仍是声明静态模型，不是实际飞行轨迹证明。

本批2源码＋13文档=15路径，无新文件、UI、协议或登记。见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。未编译、运行模型／helper、测试、仿真、改PX4、操作硬件、刷机、放行、提交、更新PR或打包。下方为历史。

## 487 当前：历史观察资料进入实际四视角报告

现有[报告构建器](custom/src/Swarm/FleetMissionReportBuilder.md)新增独立native static入口；原一次历史review从实际原页生成[观察卷宗](custom/src/Swarm/FleetTelemetryObservationReport.md)及其完整31字段运动摘要，Archive成对保存并共用最新完成页有效性。不把观察资料灌入旧任务报告、任务结果或飞行授权。

[历史面板](custom/src/Swarm/FleetTelemetryHistoryPanel.md)沿原二次位置确认显示实际聚合数、来源／覆盖、资料质量发现与指挥／客户／工程／安全审阅四视角，可人工选择复制结构化文本；不自动保存或发送。视角不是权限，内容hash不是签名，有限报值不是事故原因或任务完成。

范围6源码／UI＋15文档=21路径，仅新模块说明1文件，无新注册／依赖。详见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[34项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。原25目标及六类实质剩余保持；整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批路径或视图数上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。 本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前：遥测档案有实际只读入口和单页运动摘要

新[历史审阅面板](custom/src/Swarm/FleetTelemetryHistoryPanel.md)进入指挥中心和Planner，原生端固定本机目录、读取预算、query及分页游标；不开放录制、任意路径、raw或飞行接口。读取位置载荷及显示派生数值分别明确确认，展开不读盘、不建目录。

原实际完成页接[单页运动报告值审阅](custom/src/Swarm/FleetTelemetryMotionReview.md)，只统计同源分段的相对报告值和速度，不把数值变化当飘移原因、真实性能或实际轨迹安全。新请求／取消／输入来源变化退役旧摘要。范围11源码／UI／登记＋14文档=25路径，详见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

原25目标及六类实质剩余保持；整体约40%／剩余60%仍仅源码工作量粗估，不按本批文件或入口上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。 仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前：区域任务可明确选择严格性能准入

新增[性能政策编辑器](custom/src/Swarm/FleetPerformancePolicyEditor.md)，实际接入原区域Planner：准备编辑→选择“未声明／明确关闭／严格开启”→明确确认→保存。未处理的政策编辑阻断旧值预览、审批、部署及草稿保存／切换；隐藏只撤确认、不清掉待处理输入。更改保存到原Draft修订后，必须重新生成和审阅，不发送飞机指令。

新optional Bool进入schema5来源、真实任务top、Compiler及PlanStore来源绑定；显式false不会丢失，旧缺省不会自动补false或变true。完整编辑保存、历史恢复、部署修复及单／组余段保留政策，经现有严格消费者使用当前工程声明，而非自动认证实测性能。

[28路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[本批静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)同步。原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。

本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前：工程性能声明有实际录入与独立审批入口

[单机工程面板](custom/src/Swarm/FleetCapabilityReviewPanel.md)新增第五操作“性能 · 更新工程数值及完整能力清单”。最大速度以m/s、满电续航以秒录入，两项默认未知且至少一项明确；不继承旧数字或旧能力清单。原生要求已有当前完整匹配构型基线，独立账号按专用用途批准后，一次提交规范数值、新审核context和完整能力采纳，不更改基线、不写飞机。

新声明直接进入483已接好的严格选机、角色、Compiler主备速度和能量消费者。它是工程声明，不是实测性能、二进制或改装真实性认证。普通心跳不清空录入值；以后显式外部数值更新仍会使旧采纳／任务依据失效，不能靠本地覆盖忽略新源。

[31路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行工程交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)同步。独立摘要事件和历史筛选区分性能声明；摘要不存两数、账号、自由理由、配置或凭据，也不恢复批准。未新增Draft.settings的严格策略开关；区域草稿extra仍不支持。原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。

本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前：已批准工程性能声明进入真实规划和能量准入

既有 `task.requirePerformanceEvidence=true` 现在通过[共享声明证据](custom/src/Swarm/FleetCapabilityEvidence.md)要求当前本地工程能力批准和完整匹配构型基线，取其已绑定的规范最大速度／满电续航；真正接入选机、角色分配、Compiler主备速度、备用独立预算及逐机能量审阅。未提供／false保留旧规划行为，不把旧审批或raw数值升级成实测认证。

策略入口沿既有显式task／Intent.parameters；未增加UI开关或Draft.settings字段，区域草稿不支持这个extra键。Coordinator来源材料总是升级schema3，不表示非严格路径的全系统输出schema逐字不变。

严格Compiler为0.8.0，计划保true策略及来源身份；Coordinator schema3保完整逐机证据。严格能量估算只用声明续航乘原电量比例，实际原生剩余时间只能进一步取小，不能由metadata消耗率或不可信直接时间独立通过。[31路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[30项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[本批审计范围](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)同步。

本轮仅消费既有批准数字，没有新的性能录入器、可信实测清单、标定或机载二进制认证。P0动态状态／真实路径缺证和StartPermit阻断仍保留；原25目标及六类实质剩余不缩减，整体约40%／剩余60%仅源码工作量粗估，不按31路径上调。未编译、运行helper／模型／业务／QML、测试、仿真、操作硬件、改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 482 当前：原29参数来源与普通NAV接受／连接域接入真实审阅

新增 [WaypointSwitchReview](custom/src/Swarm/FleetMissionWaypointSwitchReview.md)，实际比较普通NAV的静态接受区及相邻完整连接AABB与coverage／corridor／terrain／obstacle。独立 `NAV_MC_ALT_RAD` 补入Evidence/schema5，Readiness同次原29快照进入两Review和Worker/schema3，原PM／PRIMARY弱owner尾核贯穿异步完成及Witness，不靠自报半径或换样。

旧12参数hash、公开19键观察和PM12／14／28旧口不变；部署手动刷新明确29项且不延长期限。新报告是实际第三报告，不只是字段guard，但动态高度接受、平滑器可达域和真实路径仍未证明，既有actualPathCertified=false继续阻断实际准入，不能当StartPermit或可飞版本。

[本批41路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[本批静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[身份边界](docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)同步。仅静态源码／文档修改，未编译、运行helper／模型／业务／QML、测试、仿真、硬件、修改PX4、刷机、放行、提交、更新PR或打包；UI/UX仅静态文案，未搜索脚本或界面验证。

原25目标及六类实质剩余不缩减；整体约40%完成／60%剩余仅源码工作量粗估，不按41路径或新报告上调。下方为历史。

## 481：运行谓词与环境审查可显式共享整数空间域

新增[原生整数域](custom/src/Swarm/FleetMissionStartupIntegerDomain.md)和完整实际消费者。只有profile显式选择startupSpatialGuard时，运行contains先过同一原生E7／AMSL毫米域，再保旧Qt额外条件；外包不再独立重算另一套允许域。旧无选择模型、旧策略及旧批准不自动升级。

新报告只在全组原域完整精确投影后给新受控采样谓词的数值包含；旧／混组仍false。真实动作及采样间路径未证明，实际准入继续消费actualPathCertified并阻断。[39路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[32项未执行交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480：起降动作参数已接真实证据与审阅

新增[动作参数模型](custom/src/Swarm/FleetMissionActionPhaseReview.md)，固定采集28项（原12＋速率覆盖2＋起降14），按当前profile明确TAKEOFF／LAND消费兼容前置。降落RC辅助或外部setpoint注入不能沿用原动作假设；失配使实际参数证据不可用，不写参数。

完整原28行来源贯穿Evidence与Readiness最后回调，旧12hash、公开19键及PM旧12／14接口不改。[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。本轮不是完整起降动力学、StartPermit或全部路线图完成。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：总速覆盖一致性及原14项参数尾核

新增[纯参数覆盖模型](custom/src/Swarm/FleetMissionRateOverrideReview.md)，检查两个ALL设置与原八个速率的float32覆写规则是否一致，不替换原观察。PM／显式刷新观察14项，但原12参数hash和公开绑定接口不变；完整guard来源进入当前审阅和启动配置指纹。

[原生证据](custom/src/Swarm/FleetMissionVehicleEvidence.md)与[Readiness](custom/src/Swarm/FleetMissionStartReadiness.md)在所有后续getter／诊断／phase回调之后，以原PM／PRIMARY控制块和原14行作纯内存尾核。旧样本被改变不能靠新采样补过。[修改入口](MODIFIED_FILES.md)及[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478：完整原域策略贯通到真实编码后路线

[区域策略](custom/src/Swarm/FleetAreaCoveragePlanner.md)保留原外环／孔／退让／参考系，Compiler 0.7／plan1.1与PlanStore实际保存并核对；[编码器](custom/src/Swarm/FleetMissionPayloadBuilder.md)按当前INT发送链逐项取E7坐标与float32高度，全组一次检查全部坐标命令连接。raw上传点不重写。

新提案／余段来源携同策略；旧档只能在独立内容复现口保持原形，正常上传缺完整策略必须从完整源重新编译和审阅。单机Readiness同样绑定原完整策略，不从摘要补none。[修改入口](MODIFIED_FILES.md)与[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)已同步。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477：名义边界退让、子区域分配与source4

区域可明确声明退让距离：完整外环内缩／孔扩张保守候选、全部子轮廓扫描与连接、按可达分量／逐机起返点分配。每条实际名义段另外按原域排孔、包含关系和原边欧氏距离核对，不从候选布尔成功推断安全。原省略／精确0路径保持。

草稿source4及同次编辑保存、历史／保存源认识和地图原输入标识已接线。[准确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)列8源码、15文档共23路径；[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)列未执行场景。不是实物净空、量化后轨迹、动态执行或放飞证明。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476：区域巡检可声明排除区，全部名义路线实际检查

新增区域内部多边形排除区的扫描裁剪与绕行；同一共享检查实际接入Compiler的主／辅助、共同路线回退、完整备用预算及无coverage的余段新计划。草稿schema3、坐标编辑和地图只读标识贯通保存／恢复，不恢复旧审批。

这是名义二维规划：边界可沿行，没有安全退让、机体半径、障碍／地形认证或飞行许可。准确范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，工程师后续操作见[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475：完整原组进入一个联合新任务与一次当前审阅（源码静态交接）

[联合规划器](custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)从v2完整组资料建立闭合的原成员→新目标映射；每机人工输入索引、起返点、速度与新配置。共同政策、独立备用计划及每机任务／返航下限保留，按各机当前速度证据阻断，不拿最快一机覆盖整组。

[联合编辑区](custom/src/Swarm/FleetMissionGroupRemainderPanel.md)接一次后台联合审阅、完整组路线／协议显示、明确采用及原本地批准，后续仍只走唯一共享部署区。不是原Run恢复、自动接管或完整启动许可。准确15源码／18文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474：整组历史资料保存与离线逐成员选用（源码静态交接）

同一实际Run的一次原生观察现在可保留全部原成员的完整载荷与进度，共同任务／计划只存一次；未MODE提交成员完整保留但不可选用。新版资料包与旧单成员包并存，目录按原顺序显示，打开后必须明确选成员，再进入原单成员新提案／新审阅。

不恢复Run、批准、占用或控制权，不是多成员联合改派、自动自愈或自动留存。准确6源码／16文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，工程师验证项见[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：完整历史规划资料包接入人工余段重规划

新增[Archive](custom/src/Swarm/FleetMissionRemainderArchive.md)和[实际资料入口](custom/src/Swarm/FleetMissionRemainderArchivePanel.md)，显式保存原Run单成员的完整任务政策／原计划／原载荷映射／观察快照。运行报告和旧summary缺少这些内容，不能据它们补造。重启后打开只预览，另行确认选用历史来源，再走原余段输入、预览和新审阅。

原生18字段／schema1保持，新增历史20字段／schema2及包ID／完整hash关联；中央provenance区分原生28字段和历史30字段。保原包含式航点、完整停留、政策及时序门槛，不自动跳过“已完成”航段。旧来源收起后仍可明确退出，再打开另一包；不会恢复原任务、占用或批准。

本批11源码／登记资源、19文档共30路径，新增5文件，见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。只有源码／静读；文件／界面／当前新任务验证见[工程师清单](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。不是自动自愈、跨站控制权交接或完整启动许可完成。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：未完成编辑草稿可显式保存并在重启后恢复为新副本

新增 [DraftStore](custom/src/Swarm/FleetMissionDraftStore.md) 和 [本机保存面板](custom/src/Swarm/FleetMissionSavedDraftPanel.md)，接入原 Planner／Root。保存已应用的完整原生编辑态，不要求区域、成员或角色路线已经能编译；未提交输入框文字不保存，先应用或取消才能操作。固定本机单槽，构造／展开／重启不自动读写。

保存经原 Draft／修订／完整源与文件原字节预览、单独确认、锁内比较、替换和完整读回。恢复重新读取同文件，创建 revision 1／地图基准未确认的新草稿，沿原撤销审批／独立备用／发布流程；不会恢复飞行许可。旧历史计划与失败部署修复仍保持严格可构建校验。

本批11源码／登记资源、17文档共28路径，含5个新文件，见[准确清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。静审修复重复取源 token 错配、调用时预期修订偷换及界面回调归属问题；工程师未执行案例见[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：同次审阅的原运动对象贯穿后台计算和启动事务

[GeographicMotion](custom/src/Swarm/FleetMissionStartupGeographicMotion.md)增加原生Models、完整参数对应及源数据独立持有；[Worker](custom/src/Swarm/FleetMissionStartupClearanceWorker.md)与[DomainReview](custom/src/Swarm/FleetMissionStartupDomainReview.md)实际接收同一个不可变对象，不仅凭source12重建Qt段长／方位缓存。两类Review锁原Ptr集合至实际finished；[Witness](custom/src/Swarm/FleetMissionStartupClearanceWitness.md)和[Txn](custom/src/Swarm/FleetMissionStartupTransaction.md)继续原对象。

静读同时修正输入侧隐式共享：预先保留的容器元素引用或字符串缓冲不再能改变已创建模型的baseline／descriptor，而缓存仍旧。Txn有资料分支严格核原完整参数／SHA／null yaw后持原Ptr，无资料旧路径仍一次prepare。value-only matcher的临时结构重建不被当作实际计算对象或来源认证。

本批13源码／18文档共31路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。原contains、source12／input8／result6／report28、预算、时限和numericfalse不变；完整Qt数值包含、动作范围、StartPermit与正式结果仍未完成。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：地形／障碍完整候选筛选与同源精确求交

[Geometry](custom/src/Swarm/FleetMissionClearanceGeometry.md)新增原生自持凸柱空间索引，按整数Morton顺序构建有界闭包围树；[DomainReview](custom/src/Swarm/FleetMissionStartupDomainReview.md)先完整准备地形和障碍两个索引，再逐组件枚举完整候选，并对同一索引持有的原凸柱执行精确SAT。候选不等于相交，原资料行身份不随树重排改变。

索引、候选及SAT共用原200万work／2048 fragments预算；取消或失败不交付候选前缀，接触边界不被剪掉。根节点严格分离或被查询完整包含可直接得到确定候选集合；密集局部重叠仍可能增加工作量，没有实测提速或节能结论。原并集覆盖、完整高度带、numericfalse及启动阻断保持。

准确3源码／13文档共16路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，工程师未执行项见[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469：精确整数凸柱求交接入完整运动域检查

[Geometry](custom/src/Swarm/FleetMissionClearanceGeometry.md)新增一次准备的原生闭凸柱与轴对齐盒SAT，[DomainReview](custom/src/Swarm/FleetMissionStartupDomainReview.md)实际用于全部地形／障碍逐对求交。所有凸柱先完整检查，再供整组component复用；不是只测AABB重叠或端点，不忽略额外斜面。

准备、并集覆盖及新SAT共享原2M／2048预算和取消。边、点接触仍算相交，真实地形顶面及障碍上下高度保持；原并集覆盖算法、原Qt运行规则、numericfalse和启动阻断不变。没有运行测速，不能声称已测得提速、省电或飞行可靠性提高。

准确3源码／13文档共16路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，工程师待验证项见[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468：收紧完整理想域外包，减少冗余求解并核报告前缀

[Cover](custom/src/Swarm/FleetMissionStartupDomainCover.md)按整个cap的纬度外包选择保守整数经度距离下界，不只取端点中心；原完整路线、两帽、独立高度带和原运行策略保持。[DomainReview](custom/src/Swarm/FleetMissionStartupDomainReview.md)同步模型名v2，地形覆盖复用同一XY竖柱并集，移除被查询Bounds蕴含的重复高度面。

不完整报告的明细只能引用已处理前缀或当前component，不能提前引用尚未处理的末尾成员；反经线重复身份仍可合法出现。旧v1报告不被当作当前v2结果，原Qt数值包含仍未证、numericfalse和实际启动阻断不变。

准确2源码／13文档共15路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行验证见[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。没有测得CPU、功耗或飞行可靠性改善。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467：整运动域的理想外包检查接入启动审阅

新增[整域外包](custom/src/Swarm/FleetMissionStartupDomainCover.md)和[环境计算](custom/src/Swarm/FleetMissionStartupDomainReview.md)，在原名义起降／编码航段检查之外，实际检查完整理想水平走廊并集乘独立AMSL高度带；原机体、支撑与工程误差完整扩张。两份报告经原后台线程、两类Review、Witness工厂和Authority共同消费，不从名义航线通过推断整域通过。

本批修正交叉审阅发现的接触例外扩散：原指定起降动作的pad不能变成整域接地豁免。原Qt数值接受域尚未证明被该理想模型包含，numericContainmentProven仍false，因此有资料路径明确clearance-blocked，不发布Witness，不解除Compiler pending。完整启动许可与正式任务结果仍未完成。

准确14源码／登记、20文档共34路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)；全部未执行验证见[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466：备份可显式导入本地历史保留区

沿原[维护面板](custom/src/Swarm/FleetArchiveMaintenancePanel.md)新增“选择本次可读备份→导入预览→独立确认→暂存复制／读回→无覆盖发布”的源码流程。仅完整核实的单段副本进入retired历史保留区，再由既有维护恢复流程单独确认纳入普通查询；不替换当前库，不恢复任务、账号、批准或飞行许可。

准确6源码、14文档共20路径见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本批不执行实际导入；未知发布和部分暂存交工程师保留核查，不自动重试、删除或回滚。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465：备份目录增加逐文件诊断

在已有[归档维护面板](custom/src/Swarm/FleetArchiveMaintenancePanel.md)增加独立逐文件检查：同一次完整观察内列出可读项、拒收项和原因，保留旧“任一坏项整次失败”的严格目录检查。新诊断不删文件、不修库、不恢复任务，也不把可读副本当原源认证或恢复许可。
当前准确范围为6源码、14文档共20路径，见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)；限制和未执行验证见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：重启后独立读取备份目录

[维护面板](custom/src/Swarm/FleetArchiveMaintenancePanel.md)新增显式备份清点：填写已有目录并点击读取后，由原生后台重新枚举、完整hash和检查SQLite封存身份，不要求原归档目录存在或先有本地库存。结果与原备份复制工单分开，实际线程结束后才由本页接纳。

这只是当前副本的可读性／结构自洽观察。originalSourceVerified和backupSetCompletenessVerified固定false，不冒充463“同原源复制并读回”的backupVerified；不证明作者、全集、独立介质、数据恢复或任务授权。坏文件／部分副本／超限／取消使整次清单失败，不以成功前缀掩盖问题，不自动删除或修复。范围与未执行验证见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：显式单段备份与读回核对

[归档维护](custom/src/Swarm/FleetArchiveMaintenancePanel.md)增加备份操作：沿原真实库存选一个sealed／retired段，明确填写已有本地目录，生成带完整目标名的原生预览，独立确认后后台无覆盖创建。源文件不移动、不删除；复制完成后重新打开目标核完整文件与数据库身份，再核原源，才可显示backup-verified。

备份沿原维护单槽和实际finished，不因accepted或取消提前结束。可能写入、目标已创建、已复制字节、复制完成与读回核对是不同事实；部分文件保留，不自动删除／重试／覆盖。路径变化使旧备份确认失效，已发工单的原结果仍应保留。准确范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[工程师待验证](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

这不是全历史备份、独立介质证明、加密签名、掉电持久性或灾后恢复完成。旧退役／恢复请求和结果保持，恢复不释放任务权限。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：单段归档退役与恢复的源码闭环

新增[归档维护](custom/src/Swarm/FleetArchiveMaintenancePanel.md)：固定本机目录中 sealed 与 retired 单文件一对一改名，真实库存、30秒原生预览提交窗口、重新确认、实际后台工单与结果观察分开。退役只移出普通历史检索，不释放磁盘、不永久删除或修复库；恢复不恢复任务／账号／批准或控制权。

实际源和目标只由原生库存／私有预览决定，不接受任意路径、SQL或旧导入DTO。写入前后核文件／目录及数据库对应，禁止覆盖和复制回退，异常保“可能已移动”。记录／查询／维护共用互斥门禁，取消不提前释放线程或保证回滚。准确范围和未执行验收见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：留存任务诊断目录与原始记录下钻

[任务历史面板](custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)沿现有固定目录、只读Reader和实际保留事件新增有界任务目录，按真实payload.taskIdHash累计诊断行，再显式按同一哈希读取原记录。旧全库搜索保持独立；不靠机号、时间或NDJSON runId猜任务关联。

目录只表示本次观察范围的保留事件数量／类别／明确车辆与缺口警告，不恢复任务、账号或控制权，不从已丢弃summary推完成率、覆盖率、真实能耗或ROI。“原记录”仅指原归档保留的诊断行，不是原始载荷或完整飞行日志。实际来源、逐查询身份、原生游标和真实线程结束继续核对；界面以文字区分等待／部分／末尾／失败，默认不读盘、不自动续扫。完整范围见[补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：原生电池明确异常进入健康／能量准入链

[原生电池账本](custom/src/Swarm/FleetNativeBatteryObservation.md)新增charge_state／fault_bitmask和实际扩展known，纯故障变化也推进原内容版本。[共享检查器](custom/src/Swarm/FleetNativeBatteryEvidence.md)统一Registry、HealthCenter、EnergyScheduler的完整值合同；当前明确负面阻断主机／备用机，过期负面保持unknown，不靠静态满电续航或FTC旧正常值绕过。

Swarm保合法负面源用于真实绑定，Guided原审阅身份包含健康变化；发送前[整组phase seal](src/Vehicle/FleetStartupMemorySeal.md)补入原生电池账本，保原来源／期限及所有回调后尾核。未知扩展、0mask或正常枚举不认证物理健康，不新增飞行命令。准确范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：原生运动模型只绑定一次，实际阶段不再反复重建

[Witness](custom/src/Swarm/FleetMissionStartupClearanceWitness.md)从原input2缓存逐机不可变模型；[Txn](custom/src/Swarm/FleetMissionStartupTransaction.md)在新请求时核完整来源后持同一个原Ptr，无资料路径也只准备一次。[Transition](custom/src/Swarm/FleetMissionStartupTransition.md)只取实际owner的原缓存，不从JSON／最新样本或失败回退重造，原初始及逐GPI判定保持。

这是明确原生所有权和移除反复路线解析／端点准备的源码优化，不是宣称修复已证实的GPI重基漏洞；原Authority的源指纹已含原GPI9。完整运动域净空、Qt数值包络、完整StartPermit和正式结果仍未完成，原pending保持。准确5源码／16文档共21路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：共享实际启动运动模型，冻结同源审阅输入

[GeographicMotion](custom/src/Swarm/FleetMissionStartupGeographicMotion.md)提取原Transition球面判断，一次准备原完整载荷的顺序地理链，阶段内复用端点／限值而不逐GPI重新解析。原走廊、高度、速度、航向及累计判断保持，不加未经证明的坐标转换门槛。

[Worker](custom/src/Swarm/FleetMissionStartupClearanceWorker.md)内部input2固定每机row3，必需冻结原GPI9、完整canonical载荷／hash、原策略和高度上限；普通／地面Review分别取同次原生capture／首次完成READ风险，不取最新位置替换。Joint仍只接明确row2名义几何投影；完整描述进入输入hash，但不冒充已检查的运动域。

完整运动域净空包含、最终StartPermit和正式机载结果尚未完成，Compiler pending保持。准确8源码／登记＋18文档共26路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：原包入口绑定与完整阶段检查的原生整组尾栅

[普通启动RAW](src/Vehicle/VehicleStartupRawIngress.md)在任何observer／Link／插件回调前冻结原scope／journal／operation／收到时刻并持整栈接收栅；排队前包不归新步骤，原scope坏帧／嵌套／丢弃锁存失效，closed sender继续组屏障期间记录。[原生memory seal](src/Vehicle/FleetStartupMemorySeal.md)固定原typed账本／收据，Txn完整阶段检查及所有回调结束后核整组，最终命令上下文只读已完整检查的原集合，不临时重捕替旧判定。

新prepared slot先走完整授权fence再查新整组seal，避免旧seal必拒正常步骤；原政策校验共享，不构造虚拟profile。运动允许运动域与实际已检查三维空间包含仍未完成，原Compiler pending保持；完整StartPermit和正式机载结果继续。准确10源码／工程登记、17文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 456：原生启动净空依据贯穿授权移交及最终发送

[同一原生Witness](custom/src/Swarm/FleetMissionStartupClearanceWitness.md)由实际普通／地面Review创建，两类Authority及StartupTransaction共享原对象，而非复制一个通过标志或从JSON恢复。保原Review时钟／截止时间、原配置／datum／报告及UTC水位；来源或期限失效后不能由新采样复活。最终Vehicle已有commandContextCurrent切点消费全组无回调memory核验，本批不改Vehicle文件。

原Compiler pending保持：尚未证明Transition整个允许运动包络包含于已审空间；普通RAW首回调前锁存和全组运动journal最终尾栅仍须补。完整StartPermit、正式机载结果及其他五类实质开发继续。[准确范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)为13源码／登记、19文档。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：同次启动净空后台计算接入两类实际审阅

[StartupClearanceWorker](custom/src/Swarm/FleetMissionStartupClearanceWorker.md)只持有纯值，通过[完整启动几何提取](custom/src/Swarm/FleetMissionJointSpatialReview.md)和原可取消环境模型生成同次报告。[普通审阅](custom/src/Swarm/FleetMissionStartReview.md)／[地面续审](custom/src/Swarm/FleetMissionArmedGroundReview.md)在实际finished后核原来源，完整result5作为必需风险材料；原报告匹配抽共用，不在GUI重跑solver。

首次风险／v4高度样本不被完成时重采替换，原UTC水位／期限不续期；取消不冒充线程已结束，地面预约等线程与传输均排空。构造成功不等于通过，四项完整／满足和零违规才成为可已阅状态。旧无资料路径保持。

尚未接两Authority移交后的Transaction／Transition到最后write，运动政策包络与已求解范围关系未完成，原Compiler pending保持；所以不是完整净空部署准入或StartPermit完成。11源码／登记、18文档共29路径见[清单](MODIFIED_FILES.md)和[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：工程高程关系接入原生审阅，不按AMSL字段名默认同基准

[固件目录descriptor2](custom/src/Swarm/FleetFirmwareAdapterCatalog.md)增加绑定场地版本／文档、构型与原点参考的显式同datum／零偏移工程关系；旧descriptor1和档案外层1保持。[Datum原生取源](custom/src/Swarm/FleetMissionClearanceDatum.md)核当前实际VERSION／PRIMARY／配置与原点，两类启动审阅将固定证据纳入原artifact摘要并在最后做整组内存尾核，不从页面或历史恢复。

同时修复目录未接收原生VERSION的receiptSequence而拒绝合法报文的源码错配；该动态序号只验证，不参与稳定指纹。两Review保留Datum内部实际观察到的最新UTC，回退到较早外Source之后也拒绝；该时间不入稳定材料，资料窗口及原单调期限不续期。工程对应不是实际高程／PX4当前内部高度分支认证，原净空pending、同次实际启动solver到最后write与完整StartPermit／正式结果仍须开发。11源码／登记、17文档见[修改清单](MODIFIED_FILES.md)与[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 以下453为历史。

## 453：实际构型来源与资料有效期接入两类启动审阅

[共享原生配置观察](custom/src/Swarm/FleetNativeConfigurationObservation.md)复用实际发现链的完整配置字段；[Registry](custom/src/Swarm/FleetRegistry.md)用私有实际对象、来源及本地批准基线建立配置证据，不把输入中的hash当作实机证明。[净空来源](custom/src/Swarm/FleetMissionClearanceSource.md)核全部原成员的配置对应与本机UTC窗口；[普通启动审阅](custom/src/Swarm/FleetMissionStartReview.md)和[地面新READ审阅](custom/src/Swarm/FleetMissionArmedGroundReview.md)冻结同一依据、原期限只缩短，已观察失效不因恢复旧值复活。

这不是实物尺寸／改装认证或高程基准对齐。真实启动净空计算、两Authority到最终write统一消费、完整StartPermit和正式机载结果仍待开发；原Compiler pending保留，不以新来源检查开放飞行。范围15源码／登记、16文档见[修改清单](MODIFIED_FILES.md)和[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批只优化地面站源码及交接材料；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余不缩减。 以下452为历史。

## 452：显式支承接触模型接入实际规划，不将LAND地面高度当机体中心

[输入schema2](custom/src/Swarm/FleetMissionClearanceInput.md)分开保护机体／旋翼和起落架，绑定原完整payload／序号、声明平面、有限pad及容差。[ContactModel](custom/src/Swarm/FleetMissionContactModel.md)和[报告3](custom/src/Swarm/FleetMissionClearanceReview.md)进入原后台Compiler及PlanStore，不只是未调用的数据类。原wire保留，声明静止参考点另算；两组件全段检查，仅起落架与指定地形的完整交集可使用有限接触带，机体和障碍不豁免。

本机配套多旋翼PX4 LAND以下降速度和landed检测工作，p7不保证停止高度。声明模型不认证真实触地／构型／高程／资料时效，原pending和最终StartPermit缺口保留。原25目标与六类实质代码继续，不增加整目标验收数。本批9源码／登记、18文档见[修改清单](MODIFIED_FILES.md)及[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；以下451为历史。

## 451：规划编译转后台，三个实际入口接等待与取消

带净空资料的任务通过[单任务Worker](custom/src/Swarm/FleetMissionPlanningWorker.md)运行完整可取消编译；[协调器与Root](custom/src/Swarm/FleetMissionReviewCoordinator.md)在实际finished后核原输入／真实对象，再完成原生审阅和准确版本归档。地图Draft、余段子提案和AI建议接上pending／最终回调、可见取消和过期反馈，不以等待当通过，也不自动批准或下发。

旧任务取消后不能因来源恢复而复活；已经提交的归档事实与当前批准资格分开。不是所有GUI计算都已迁移，也不保证立即取消或固定响应时间。原净空pending／接地与构型高程时效、最终StartPermit及其余六类代码仍未完成。原25目标保持，整体仍为约40%的管理粗估，不由批次或文件数提高。

本批15源码／登记、17文档见[修改清单](MODIFIED_FILES.md)和[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码和限定静态审阅；未编译、测试、运行、修改PX4、刷机、提交、更新PR或打包。下方450及更早为历史。

## 450：规划净空进入编译与归档，独立启动几何身份接线

[完整段producer](custom/src/Swarm/FleetMissionJointSpatialReview.md)现在明确区分encoded-planning与startup-and-encoded-planning，不伪造当前位置或启动视图；[启动投影](custom/src/Swarm/FleetMissionStartupSpatialReview.md)新增独立geometryBasis入口，保留旧最终审阅指纹接口。[净空报告schema2](custom/src/Swarm/FleetMissionClearanceReview.md)保留声明范围与basis，不能把规划成功当作实际启动已审。

[Compiler](custom/src/Swarm/FleetMissionCompiler.md)实际计算并冻结planningClearanceReview，失败增加真实阻断；[PlanStore](custom/src/Swarm/FleetMissionPlanStore.md)核输入、同序成员和完整几何来源一致，归档检查只重建来源，不重复重型净空求解。原Coordinator风险／空域门禁保持，旧报告／历史也不改写。

**原pending暂不解除。** 接地参考点语义、实际构型／高程对应、资料有效期及真实启动／最终write固定消费仍待完成，不能为推进流程删阻断。9源码／17文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标保持，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方449及更早为历史。

## 449：整段声明机体包络的精确净空计算核心

新增[有界整数几何核心](custom/src/Swarm/FleetMissionClearanceGeometry.md)和[完整联合段报告](custom/src/Swarm/FleetMissionClearanceReview.md)：机体／跟踪／场地／投影／高程误差沿整个名义段扫掠，对覆盖区与允许走廊做闭区域并集包含，对地形覆盖、保守地形上界与障碍做整段检查。不是只检查航点，也不要求一个区域独自包含整段。

有理交点使用固定256位整数和严格补集；共享边界不制造假缝，资源超限明确未知。工程输入与原联合视图先完整验证，报告分开表示范围完整、计算完成和声明模型满足。起降接地上下文仍缺失，不能删除相应段或自动通过。

本批是实际算法源码，不是实际启动净空已接通：Compiler的 `clearance_geometry_review_pending` 保留，未新增UI、QML入口或许可。规划阶段／真实StartReview与GroundReview的固定来源消费、时效／构型核验、最终StartPermit及原25目标继续。5源码／工程＋15文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方448及更早为历史。

## 448：净空工程资料进入真实草稿、编译与归档链

新增[版本化净空输入合同](custom/src/Swarm/FleetMissionClearanceInput.md)：场地来源／版本／UTC时间、高程与局部图表、毫米凸柱体覆盖／障碍／地形上界／允许走廊，以及逐机尺寸、配置声明与跟踪误差。完整资料有界校验、规范JSON及内容指纹，不补物理默认值。

[Draft](custom/src/Swarm/FleetMissionDraft.md)的旧来源1／11项settings保持；含资料为明确来源2／12项，保存、恢复与[编译](custom/src/Swarm/FleetMissionCompiler.md)保留完整资料，活动主机必须完整同序匹配。[版本库](custom/src/Swarm/FleetMissionPlanStore.md)独立核完整内容及原编辑源一致性；两处历史／修复恢复预检同步兼容版本，保持原草稿与确认流程。

本批仅完成资料竖切，**没有实现净空求解**：合法新资料仍产生 `clearance_geometry_review_pending`，沿实际风险链阻断批准；不是看见地图或格式有效就可启动。下一步必须用真实几何结果替换待审，而非删原因。8源码／工程、17文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标保持，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方447及更早为历史。

## 447：阻断风险接入实际审批、部署与启动审阅

[风险汇总](custom/src/Swarm/FleetMissionRiskEvaluator.md)的 `inputValid=true` 不是无业务阻断；[协调器](custom/src/Swarm/FleetMissionReviewCoordinator.md)现在消费同一原生严格检查，非空 `blockingReasons` 不再只出现在报告中。未命名严重风险以相同 `caller_risk_signal` 写入明细与原因，避免空code丢失阻断。

[实际部署](custom/src/Swarm/FleetMissionDeployment.md)核冻结评估与准确归档内容；[普通启动审阅](custom/src/Swarm/FleetMissionStartReview.md)和[地面再审](custom/src/Swarm/FleetMissionArmedGroundReview.md)各自核确切记录，不借另一任务的最新报告。合法空列表仍继续原全部门槛，不形成许可；旧阻断历史只能只读或新审阅。

本批6源码、16文档共22路径，见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。这是净空实际风险链的必要前置，不是工程场地／机体包络净空已实现。原25目标继续；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方446及更早为历史。

## 446：高度容差的中间舍入误通过修正（源码阶段）

[v4高度门槛](custom/src/Swarm/FleetMissionStartupAltitude.md)改为对原报文高度及整数毫米参考作[定长精确线性比较](custom/src/Swarm/FleetMissionAltitudeArithmetic.md)，不先算Double差值再用最后一格外扩掩盖中间丢失。固定大小整数覆盖原支持域，六项容差、原20／20字段、scope监视及旧v1–v3不变；三差值仍保留显示。

这是既有实际启动门禁的算法修正，不是新增许可或安全认证。完整净空仍需版本化场地／机体误差资料并进入真实启动风险链；当前联合空间报告仍是名义几何显示，不能据其零命中宣称净空。原25目标继续，范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，静态推导和未执行事项见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方445及更早为历史。

## 445：显式高度关系容差与启动连续性（源码阶段）

新增人工明确选择的[第四版启动策略](custom/src/Swarm/FleetMissionExecutionProfile.md)：沿用局部运动／航向观察、固定HOME，显式填写六项高度差值／参考残差容差；[表单](custom/src/Swarm/FleetStartupPolicyEditor.md)不填默认值、不隐式升级。[原生高度契约](custom/src/Swarm/FleetMissionStartupAltitude.md)与实际Vehicle／Transaction接线用于冻结首次审阅基准，并在本次实际启动scope内保留越界、坏帧、失源和断档事实，后来的正常数值不能清除。

这是显式报文一致性约束，不证明共同原点、PX4内部valid、同采样、真实净空或最终StartPermit；scope建立前的审阅等待期不冒充连续覆盖。原v1–v3保持，原25目标和其余实质代码继续。范围及静态核对见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行项见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方444及更早为历史。

## 444：同包高度与参考差值的冻结诊断（源码阶段）

[ALTITUDE原包解码](src/Vehicle/FleetStartupAltitude.md)保留同一消息的六个float32字段、逐字段未知原因／原始位值和三个报告差值；[独立Vehicle账本](src/Vehicle/VehicleStartupAltitude.md)记录实际PRIMARY来源、接收年龄、状态类别与报告时钟倒退。新收包或发送timestamp推进不证明传感样本新鲜，差值不直接命名为已验证原点／HOME。

诊断进入现有普通／地面启动风险快照，不从高度Fact或不同报文拼字段。首次审阅材料保持冻结，周期高度／age不进入原启动阶段或审批当前风险身份；原v1–v3门槛不变，无新消息请求或放行。11源码／工程登记、18文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行项见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标继续，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方443及更早为历史。


## 443：被动原点账本与实际启动连续性（源码阶段）

[原始报文解码](src/Vehicle/FleetStartupGlobalOrigin.md)和[Vehicle独立账本](src/Vehicle/VehicleStartupGlobalOrigin.md)记录实际PRIMARY收到的GPS_GLOBAL_ORIGIN，不请求新数据流，不使用周期三秒TTL。原点数值／报告时间或来源变化、坏帧及已观察后失源不能被当成从未收到，也不能恢复旧启动依据。

[稳定参考契约](custom/src/Swarm/FleetMissionStartupGlobalOrigin.md)接入原生Readiness、终态再审上下文、地面新READ和Transition；实际发送scope首次冻结一次，后续步骤及最终write前核对，不重基。仍不证明HOME／local／估计器同原点、同采样或内部有效分支，不新增StartPermit。原25目标继续，13源码／工程登记、18文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)；未执行事项见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方442及更早为历史。


## 442：完整请求链与独立启动参考的联合名义空间检查（源码阶段）

[联合报告](custom/src/Swarm/FleetMissionJointSpatialReview.md)复用同一次准确记录捕获，按完整本地canonical请求顺序检查出航／返程连接、TAKEOFF连接、HOLD锚点和LAND下降；速度项不当坐标。独立启动假设不拼成实际航迹，使用原归档区域及原实际应用间距，不重新加载当前地图。

[既有明细](custom/src/Swarm/FleetMissionSpatialPanel.md)增加联合摘要与逐层分页，分别显示几何覆盖、计算完成、接近候选、区域相交和截断；原归档报告不改写。报告始终不认证时序／实际路径／净空或授权执行。10源码、17文档的范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行事项见[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标继续，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方441及更早为历史。

## 441：禁飞区原值进入审阅、归档和审批门禁（源码阶段）

[输入保真链](custom/src/Swarm/FleetMissionAirspaceInputGate.md)不再用真假值将坏禁飞区折叠为空列表；[原生检查](custom/src/Swarm/FleetAirspaceConflictChecker.md)拒绝坏类型／JSON／预算、双源冲突和损坏间距，并发布新的阻断报告覆盖旧审阅。归档只取采集前冻结副本，有效来源还核当前私有Intent指纹；[任务层](custom/src/Swarm/FleetIntentTask.md)重新检查审批输入并隔离同步重入。

本批7源码、15文档，共22路径；不新增飞行入口，不认证真实空域或净空。完整联合空间检查、最终StartPermit／正式结果及原25目标继续；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)；下方440及更早为历史。

## 440：原完整任务与归档空域共同冻结显示（源码阶段）

[完整任务投影](custom/src/Swarm/FleetMissionSpatialReview.md)对同一准确归档plan构造全成员路线、返程与起止动作，并只调用一次原完整PayloadBuilder生成本地协议明细；失败不补路、不拆单机凑成功。[新明细](custom/src/Swarm/FleetMissionSpatialPanel.md)与原起飞空间共用一次捕获／地图选择，分别标注规划、已生成请求、未知和历史。

归档禁飞区与原空域报告按原版本显示，不换当前地图；新输入元数据区分未提供／显式空／坏型，旧空列表保持未知。原报告未覆盖起飞独立假设和结束动作，不能冒充联合净空检查。原25目标和最终StartPermit、正式结果、真实性能／自愈／跨站／长期运营／真实模型生态继续。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包；见[本批范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。下方439及更早为历史。

## 439：冻结起飞空间审阅（源码阶段）

[空间面板](custom/src/Swarm/FleetMissionStartupSpatialPanel.md)从当前实际normal／ground风险审阅明确捕获名义垂直段、起飞请求点连接和当前位置到首航点提示；逐机高度／未知项与只读地图共用同一冻结来源，过期不自动换图。

[几何投影](custom/src/Swarm/FleetMissionStartupSpatialReview.md)保留完整目标集，[间距候选](custom/src/Swarm/FleetAirspaceConflictChecker.md)只比较名义线段，明确缺时间／地形／禁飞区／净空。没有新飞行指令、默认间距或最终许可。原25目标及剩余实质代码工作保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；见[范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。下方438及更早为历史。

## 438：完整启动策略表单（源码阶段）

[结构化表单](custom/src/Swarm/FleetStartupPolicyEditor.md)覆盖现有v1／v2／v3全部高度、速度、时间、局部运动、航向与HOME字段；明确采用后进入同一完整执行配置保存链，不再只能手写JSON。高级JSON保留，未采用输入会阻止旧策略被保存；没有默认容差或隐式升级。

[父编辑器](custom/src/Swarm/FleetExecutionProfileEditor.md)增加真实草稿选择绑定与换源文字保留，Root核实际backing Draft防同机号同修订号误写。原25目标和完整许可／可信性能／自愈／跨站／长期运营／真实模型生态继续。未编译、运行、测试、改飞控、刷机、提交、更新PR或打包。见[范围](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)；下方437及更早为历史。

## 437：实际部署启动工作台（源码阶段）

[启动工作台](custom/src/Swarm/FleetMissionStartupWorkbench.md)接入当前部署的整组内容核对、风险已阅、独立账号条件审批、原生认证移交和真实三步事务，并提供地面新读取／新审批／独立新事务路径。每次明确执行一步；逐机区分已请求、已观察和可能提交，取消后保留排空与历史事实。

[地面风险展示](custom/src/Swarm/FleetMissionArmedGroundReview.md)增加只读白名单，操作员可查看原风险后确认。仍缺完整StartPermit、可信原点／净空、正式任务结果，以及可信性能、自愈改派、跨站、长期运营和真实模型／生态。原25目标保持。本批未编译、测试、运行、改飞控、刷机、提交、更新PR或打包；范围见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。下方436及更早为历史。

## 436：实际运行简报交接与本地收件箱

[交接工作流](custom/src/Swarm/FleetMissionHandoffWorkflow.md)从实际选定Run冻结任务、准确规划版本及逐机历史摘要，操作员明确确认后交给指定认证账户。接收只代表收到简报，不移交飞机控制权、不释放占用或恢复批准。指挥中心增加分页收件箱和终态详情，不再手填任务ID或只显示两条待处理记录。

原生身份与角色收据在变更前复核，已提交记录不因随后换账号／换来源而冒充回滚；导入历史不能恢复本实例交接来源。没有跨站同步或新持久化界面。原25目标及六类实质开发继续。仅源码与限定静态核对，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方435及更早为历史。

## 435：余段新记录的地图与逐点／协议核对

新增[共享只读显示投影](custom/src/Swarm/FleetMissionRemainderView.md)和[分页明细](custom/src/Swarm/FleetMissionRemainderRoutePanel.md)。采用余段后地图规划层切到该新记录，过期仍标为历史，不以旧区域草稿替代；只读地图模式保留规划隔离，不发指令。

规划点索引与实际协议序号分开，完整编码来自原 PayloadBuilder；未知值、编码失败、量化和未编码要求明确保留。这不是避障／净空证明、启动许可或正式接替。原25目标和六类实质开发继续；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师待办](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。以下434及更早为历史。

## 434：余段新审阅接入人工批准与共用地面部署界面

[余段操作入口](custom/src/Swarm/FleetMissionRemainderPanel.md)增加明确“结束编辑并采用本次新审阅”。Root 保存新记录的完整绑定与当前部署组；申请／批准复用原意图状态机，唯一[地面部署面板](custom/src/Swarm/FleetMissionDeployment.md)显示明确的区域或余段来源，并按该来源选择证据刷新目标。过期余段保持可见且不可上传，不静默回退区域草稿。

不继承原Run审批，不解除原成员占用，不增加起飞／空中接管。现有身份、独立账号审批、覆盖确认、地面未解锁和读回门禁保持。范围见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，待验证项见[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码与限定静态检查，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；原25目标和六类实质开发继续。下方433及更早为历史。

## 433：从实际运行来源生成独立余段重做提案（源码阶段）

新增[原运行只读取源](custom/src/Swarm/FleetMissionRemainderSource.md)、[余段规划](custom/src/Swarm/FleetMissionRemainderPlanner.md)和[操作入口](custom/src/Swarm/FleetMissionRemainderPanel.md)。操作员明确选择原成员与包含该点的航点后缀，提供目标机自身起返点、速度与新profile，再生成并确认新的审阅记录。原安全/能力/能量/模板政策保留；原Run、其他成员和占用不被改派。

进度遥测不是连续完成证明，不自动跳过“已到达”点；原点的完整停留保留。该增量不是空中接管、自愈完成、旧批准恢复或可放飞证明。范围、限定静态审阅与待实际验证事项见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[审计](docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师交接](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。原25目标与六类剩余开发继续；下方432及更早为历史增量。

## 432：备用机待命与接替路线独立预算（源码阶段）

规划页增加[备用预算编辑](custom/src/Swarm/FleetReserveEnergyPlanPanel.md)：明确待命方式、时长、耗能系数、三轴速度及接替对象，并单独填写自身起点／返程点。[估算模块](custom/src/Swarm/FleetReserveEnergyPlan.md)按自身进场、目标完整航点路线和自身返程计算，进入原逐机审阅账单；缺少输入的备用显示不完整，不借用主机最长预算。

这是“完整目标航线重做”的规划估算，不是中断点续飞、实际接替、避障路线或已标定电池模型。新路径不进入实际执行成员；无新增飞行命令或许可。本批11源码／18文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[交接说明](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)列出工程师待验证项。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；原25目标仍有六类实质开发。

## 431：实际电池报告进入能量审阅（源码阶段）

新增[原生电池观察](custom/src/Swarm/FleetNativeBatteryObservation.md)，经实际 Swarm 绑定和 [Registry 动态投影](custom/src/Swarm/FleetRegistry.md)，进入[逐机能量预算](custom/src/Swarm/FleetEnergyScheduler.md)与现有规划页。仅使用唯一明确推进电池的新鲜报告；辅助电池不平均，失源、过期和拓扑不明保持未知。普通描述或护照不能恢复实时来源。

飞控报告的剩余秒数是估算，本地收包年龄不是远端测量时间。审阅绑定来源和内容，并再次核对扣龄后时间能否覆盖原预算；不授予启动权限。完整链路及限制见[桥接说明](custom/src/Swarm/FleetNativeBatteryBridge.md)，交接范围见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；原25目标和六类实质开发仍未完成。

## 430：原任务成员的逐机能量账单（源码阶段）

[审阅协调器](custom/src/Swarm/FleetMissionReviewCoordinator.md)按实际编译主机和备用机生成预算，[能量调度](custom/src/Swarm/FleetEnergyScheduler.md)保留每架原成员及拒绝原因，不再让高电量备用机通过排序掩盖主机不足。规划页增加默认折叠、每页8架的[只读账单](custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)，区分未知、估算余量、缺口与预测依据。

同时修正满电续航的剩余百分比公式，兼容 Registry 的数值 -1 未知标记；未知不是已验证性能。多种有效续航依据取较小值，仅直接剩余秒数不推导剩余百分比。备用预算仍不是实际接替路线或待命模型，本批没有新增真实电池遥测。见[修改清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)及[工程师待验证项](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码和限定静态审阅，未编译、测试、运行、修改PX4、刷机、提交、更新PR或打包；原25目标未完成。

## 429：真实运行观察报告与离线查阅（源码阶段）

[运行报告](custom/src/Swarm/FleetMissionExecutionReport.md)从选定部署组的实际最新启动事务采集Run快照，与原飞前计划报告分开。[报告面板](custom/src/Swarm/FleetMissionExecutionReportPanel.md)提供明确生成、冻结摘要／逐机明细、选择路径后再次确认保存，以及外部JSON只读打开。没有prepared观察时不生成；已退休、零MODE提交及失源的历史观察仍保留其未知语义。

报告不计算任务成功率，不把队列提交、航点观察或落地候选当作执行完成。文件摘要仅检查内容一致性，不认证来源；导入不恢复账号、审批、任务状态或控制权。参见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师待验证项](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码及限定静态审阅，未编译、测试、运行应用、修改PX4、刷机、更新PR或打包；原25目标及六类实质工作继续。

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

## 418：单机能力声明的实际双账号审阅入口

[能力审阅工作流](custom/src/Swarm/FleetCapabilityReview.md)及[面板](custom/src/Swarm/FleetCapabilityReviewPanel.md)已接实际 Swarm/Registry、专用 Session/Role、Root 与指挥中心/地图规划页。可预览当前本地条目、完整替换采纳能力或撤销原批准；两项操作都不改构型基线、不改参数、不发飞行指令。

维护/管理员申请、不同合格账号审批，原30秒与身份期限、原对象和条目版本持续约束；Registry 通知前固定提交回执，后置失败、销毁或审计失败不抹变更事实。原 context6/approval14 保留；旧 bool 原生后端不是全入口双账号服务。界面不把能力声明叫硬件验证，构型 JSON 仅供只读展示，具体改装与实测性能仍未知。

本阶段16源码/资源及17文档见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，须连同累计工程接收；未编译、测试、运行 UI/认证/helper/应用/设备，未改 PX4、刷机、更新 PR 或打包。原25目标和六类实质工作仍继续，不能以此称已可部署；下方417及更早是历史。

## 417：运行中显式重载本地固件工程声明目录

新增[固定文件重载工作流](custom/src/Swarm/FleetFirmwareCatalogReload.h)和[完整预览界面](custom/res/Merivus/FleetFirmwareCatalogPanel.qml)，入口位于指挥中心及 FlyView 地图内的区域任务规划页。只读取 AppData 下固定的 `fleet-os/firmware-adapters-v1.json`，不接受界面路径或任意 JSON；有界读取、完整旧/新工程声明及增删改清单、原始/规范哈希进入同一次审阅。原 `engineeringReview` 是档案声明，不会被当前审批账号替换。

维护人员或管理员申请，另一个合格实际账号以独立 `firmware.catalog.reload.approve` 一次审批；原生目录 epoch/revision/CAS、实际 Root/current scope、原账号期限和30秒窗口持续约束，最后固定文件复读及纯提交围栏后才整体替换**内存**目录，不写原文件。变更使旧任务 review 失效，不停止已有 Run、不释放占用，也不是固件签名验证、刷写或飞行许可。提交事实、外层后置失败和审计追加结果分别保留。

另补现有全球坐标启动链的估计器要求：必须有绝对水平及绝对垂直位置，且不在 constant-position 模式；不改 RAW schema、通用健康定义、室内能力或 GNSS 硬件。19个源码/资源路径见[清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，须连同累计工程交接，不是独立包。启动时原有自动 load 路径没有改成强制双账号；本节只覆盖运行中显式 reload，不能宣称所有入口统一授权。

仅源码及限定交叉审阅；未编译、测试、运行解析器/helper/UI/认证/应用/SQL/设备，未改 PX4、刷机、更新 PR 或打包。原25目标/六类仍有实质源码工作，无整体完成率或交期结论。以下416及更早为历史，本节覆盖其中“固件目录操作入口尚缺”的运行中入口部分，不提升其他完成或验收状态。

## 416：逐机执行配置与真实参数哈希绑定

地图草稿现在可为每个成员保存完整、独立的 `executionProfile`，经准确草稿 revision 的 CAS 规范化替换；不把规划速度、实时 Home 或别机配置补成执行参数。角色变化保留本机配置，reserve 只保留而不进入活动 assignments；删除后重加同号成员不继承旧配置。通知重入后返回 false 可能已有本地提交，须读回当前 revision，不可自动重试。

规划页新增[逐机配置编辑器](custom/res/Merivus/FleetExecutionProfileEditor.qml)：核心动作、AMSL 坐标、速度与时序有结构化输入，完整启动政策仍由高级 JSON 提供 schema 1–3，不是所有包线的表单化配置。政策可留空保存规划意图，但不能完成受控启动。[只读原生参数观察](custom/src/Swarm/FleetMissionReviewCoordinator.md)仅在实际同源 12 项均可用时提供哈希/稳定指纹；不 refresh、不写参数，保存前再次精确比对，再进入 Compiler 原有合法配置路径。

本阶段源码/资源及文档同步属于累计工程，范围见[当前清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，不是安装包或运行验收。UI 技能影响标签、明确确认、可见错误及现有 QGC palette 的沿用，不代表界面已经验证。完整25目标/六类仍继续；最终 StartPermit、固件目录操作入口、可信 HOME/local、净空及正式机载结果未完成。未编译、测试、运行 helper/UI/认证/应用/设备、修改 PX4、刷机、更新 PR 或打包。以下旧批次为历史；旧“草稿不带 profile”描述由本节覆盖。

## 411–415：恢复复核面板与实际控制器接线

[专用复核界面合同](custom/src/Swarm/FleetOccupancyRecoveryPanel.md)已加入指挥中心及地图内区域任务规划页，复用原私有Swarm、当前实际Session/Role和原生恢复工作流；不是第二套集群架构。申请与独立账号审批分开，默认取消，两项人工确认不预勾选，不以Enter批准；关闭、改输入或换来源取消原审阅。

本阶段9个源码/资源触点、18份文档，共27路径，是累计源码增量，不是独立安装包。已接线不等于Qt界面验证：Root标明uiWiringPresent=true、runtimeValidated=false，原生recoveryUiIntegrated仍false。只解除本地重启/历史待审，不清当前真实unknown、owner或任务，不授飞行许可。

完整25目标和六类实质开发继续。仅源码及限定文本审阅，未编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，未改PX4、刷机、更新PR或打包。下方406–410“未加UI”为历史，由本节源码接线覆盖，不提升运行或放行结论。

## 406–410：原生重启／历史占用专用复核

[恢复复核合同](custom/src/Swarm/FleetOccupancyRecovery.md)新增实际Swarm唯一拥有的原生工作流，公开只读对象和request/独立审批/取消入口；未加QML操作界面。bootstrap历史待审与本轮真实unknown分集合，整组最多32个选中地址只可解除restart/history holds，真实unknown、实际owner、任务、临时任务、隔离和everObserved历史不清除。

原Vehicle/MM/参数/VLM/MVM/model及主链路共享所有者冻结，新鲜2秒内原始HB未解锁与已知ON_GROUND、完整本机队列排空进入30秒复核。专用fleet.occupancy.recover.approve强制不同真实账号，一次消费不生成lease；原账号TTL、当前Episode/角色在最终无回调尾栅再核。现场处置编号和地面检查是明确人工声明，不认证物理停机、远端无任务或控权。

所有可回调读取后，整组再以纯native原RAW语义revision/hash/value/鲜度/队列核对，并核原worker、实际占用和任务map；同栈整体移除本地待审后bump，新actual身份须由原worker实际提交才恢复既有continuity getter/CAS。留存原QQ、源epoch与primary控制块，换源在getter/最终CAS即时重加hold，不只等timer；正常收包、合法起飞和新MM工作不冒充换源。

本批12源码/工程登记、17文档，共29路径。原生recoveryWorkflowImplemented与UI集成分开；localHoldsReleased仅历史动作事实，selectedReviewRequiredVehicleIds另报当前待审。仍缺UI、离线/报废现实处置、本轮真实unknown受控解除和完整机载自愈；完整25目标/六类实质开发保持。未编译、测试、运行helper/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方401–405及更早是历史状态，不以旧“无任何再审入口”覆盖本批native入口，也不宣称全部恢复产品已完成。

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

当前交接：[修改入口](MODIFIED_FILES.md)、[路径清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师说明](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

本轮交接入口：[修改清单](MODIFIED_FILES.md)、[实际路径](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师说明](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

本轮具体路径及工程师验证要求见 [补丁清单](docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) 和 [交接说明](docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

## 371–375：下一部署组与历史监视选择入口（源码接线，未验证）

已增加两个专用 `ForController` 原生切换接口及 Guided 的 `prepareNextFleetDeployment(currentToken, true)` / `selectFleetObservationEpisode(id, observationToken, true)`。原生调用连续持有实际 Root 的调用占用，经私有栈 permit 完成 mutation，再核新 scope 并结束原调用；不是先释放占用再调用 legacy 接口。公开只读元数据为12键、每组6键，不输出 raw QObject 引用。

[本地部署组面板](custom/src/Swarm/FleetMissionLifecyclePanel.md)已接指挥中心和规划页两个入口，先选择动作、再明确确认，默认关闭/取消，不以 Enter 提交；原视图 ticket、来源代次和 plain lifetime 防止旧视图提交。创建新组不自动更换 selected 监视、不登录或继承批准、不停止旧 Run、不释放旧占用。已提交的发布/选择事实与调用收尾分开，失败不回滚；失败分配尝试也计入8组上限，`localObjectGroupAllocated=false` 不证明未计费。

本批实际8源码、19文档，是累计工程增量而非独立安装包。`outboundIntegrationComplete=false` 保留，完整25目标/六类仍有实质源码缺口；已完成限定文本审阅，具体数量见本轮静态审计。本机未编译、测试、解析/运行 helper、认证、应用或设备，未刷机、更新 PR 或打包。以下366–370及更早批次均为历史状态，其“尚无产品入口/专用切换 API”说明由本节覆盖，不表示新入口已运行验收。

## 366–370：实际部署组接入控制器（源码接线，未验证）

这5个源码是相对前批的增量，交接必须包含累计完整源码、pri、注册与原生依赖，不能只拷5文件构建；未打包/更新PR。Qt目标环境的22键 scope Int/QObjectStar往返、方法绑定、parent和销毁时序尚未运行验证，不能放宽合同冒充通过。

已注册原生 Lifecycle 和不可直接创建的 Episode；Guided 私有工厂的构造函数已有 initial 组，不自动再分配。14 个服务引用、30 个 Fleet 入口与24个原生来源回调改为实际组范围，外层同步调用以原 owner/callId 在 finally 清理；没有 legacy fallback 或公开原始对象引用。[作用域契约](custom/src/Swarm/FleetMissionControllerScope.md)只核引用身份，不授予权限；未知或已 queue 的事实不因返回检查失败而回滚。

监视四个值快照来自同一 selected scope，来源 token 含选择代次；关闭审阅携完整收据路由至原 retained Close，选中监视可显式登录，但不结束 Run。用户可操作的新部署/选择入口及专用 transition 原生 API 仍未开发，`outboundIntegrationComplete=false`；25目标/六类实质源码缺口仍保留。本批实际5个源码、18份文档，已完成限定文本审阅，不是运行或安全通过；未编译、测试、运行、更新PR或打包。下方批次是历史状态，其中未注册/未迁移说明由本节覆盖。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](custom/src/Swarm/FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](custom/src/Swarm/FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

## 346–350：十二类原生服务门禁接线

[Episode服务接线](custom/src/Swarm/FleetMissionDeploymentLifecycle.md)已覆盖另外12类服务的新请求、live依据、审批/身份移交及现有scoped发送fence：归组来源须仍为current且owned peer属于同一实际组；旧取消/revoke/drain、被动Run/Close和真实已queue记账不借此封禁。Authority/Txn认证移交上下文改为纯本地比对，完整身份/风险门禁仍在调用两侧；Series保留严格一次未发布构造绑定。

仍未形成新部署可用闭环：普通Deployment上传/回读的legacy PlanManager在插件后、异步每包/重试没有独立Episode fence；Guided actual Episode/epoch、共享Role身份上下文和selected旧观察整组投影尚未接入，无新QML注册/部署入口。只改源码及限定静态审阅，不编译、测试、运行认证/应用/设备、更新PR或打包；原25目标/六类继续。


## 345：非 Close 用途的原生身份门禁

[Session 身份接线](custom/src/Swarm/FleetOperatorSession.md)已加入上传/启动/ground用途的current Episode检查、实际owner配对及OS/owner回调后的再次核对；旧组普通身份事实、显式登录/退出、精确撤销和独立Close用途保留。相同账号/UUID或不同工厂的当前对象不能替代同一实际组。

这是340–344核心后的源码增量，不是全链隔离完成：其余owned发送/审批入口、Guided actual Episode/epoch和selected旧观察投影仍未完整接线；无QML注册/新部署入口，现有应用仍单链。只做源码与限定静态审阅，不编译、测试或运行认证/应用/设备，未更新PR或包，原25目标/六类继续。


## 340–344：新部署生命周期原生核心，尚未接入应用

新增 [Deployment Lifecycle](custom/src/Swarm/FleetMissionDeploymentLifecycle.md) 的实际对象组与有界工厂，并在 `custom.pri` 登记四个新源码文件。每个 Episode 新建14个 owned 对象（含 signed-out Session 和 EvidenceRefresh），旧实际 Deployment/Txn/Series/Run 与占用保留；共享五个冻结的实际来源。容量为8次分配，含初始组及失败尝试，不驱逐历史。

**本批没有 QML 类型注册、没有替换 Guided 原对象、没有新部署入口；现有应用仍为原单链。** sticky outbound 状态和 native helper 已定义，但 owned C++ 请求/继续点、QML 调用前后 epoch、源回调与 retained Close 身份选择尚未完整接入，不能称跨部署隔离完成或新部署已可操作。

原25目标/六类不缩减；后续仍有实质源码工作。未编译、测试、运行应用/认证/helper、仿真或刷机，未更新 PR 或重新打包；本批审计限定检查结果已记录。

## 335–339：运行监视操作界面（2026-10-15）

[运行监视面板](custom/src/Swarm/FleetMissionObservationPanel.md)已接入指挥中心与任务规划页，独立于草稿/上传权限。查看不执行；申请后每次重新确认三项风险，并由另一个账号审批。各入口只拥有自己的review，取消不接管其它入口；已释放/仍占用/未知分别显示。此节覆盖330–334“无UI”的历史状态，但仍未编译、测试、启动界面或执行认证/设备；原25目标/六类仍继续，没有更新PR或打包。

## 330–334：独立结束本地运行监视（2026-10-15）

新增[本地监视结束工作流](custom/src/Swarm/FleetMissionObservationClose.md)：实际当前 Run/启动/任务模型绑定、两个真实独立账号、独立用途、原30秒审阅期限及三项确认已接后端。实际占用表与安全撤销分开核对，未知不报已释放；没有新增界面按钮，不发送停飞/降落/上锁，不宣布任务完成。当前仍仅源码与限定静态审阅，未编译、测试、运行认证/应用/设备，未更新PR或打包；原25目标/六类仍未全部完成。下方325–329及更早是历史批次。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](custom/src/Swarm/SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

320–324（2026-10-15）新增[显式 HOME 观测过渡](custom/src/Swarm/FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

315–319（2026-10-15）新增[本次启动的连续运行观测](custom/src/Swarm/FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

310–314（2026-10-15）新增[错误可区分的原生目录清点](custom/src/Swarm/FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

305–309 新增[遥测只读历史检索](custom/src/Swarm/FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

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

基于 QGroundControl 的多无人机调度地面站，由 **[Ale-xl](https://github.com/Ale-xl)** 维护。配套飞控为 [MERIVUS-FirmwarePX4](https://github.com/Ale-xl/MERIVUS-FirmwarePX4)。本仓库包含界面、人工多机调度、FTC 遥测展示，以及只提供建议的本机 AI Agent。

> 当前为开发测试版本 `0.1.0-dev.1`，不是生产发行版。已有代码、历史验证和本次验证是不同状态；FTC 主动控制与植保覆盖作业均不能因界面或方案存在而视为已经验收。

## 当前能力与边界

244–249新增[原生三步启动事务](custom/src/Swarm/FleetMissionStartupTransaction.md)和[阶段观测](custom/src/Swarm/FleetMissionStartupTransition.md)源码，当前已完成有限源码静态收口，未编译/运行/测试。实际seq0/normalARM/mode-only后端不使用隐式解锁，取消保留可能提交/飞行事实；无新Start/ARM按钮。HOME重置再审、local参考/运动/转向、最终StartPermit/净空及完整执行完成仍待开发，原六类/25项目标保持。下方239–243为历史批次。

239–243源码增量加入[Startup依据一次接管](custom/src/Swarm/FleetMissionStartupAuthority.md)：真实账号/完整来源/原最早期限移交到固定实际owner，并增加typed地面阶段候选。它不是最终StartPermit，当前实际ARM仍使依据失效；尚无受控命令事务或新执行UI，未编译/运行/测试或更新PR/安装包。完整路线图继续，不能把依据接管称作已经实现起飞。

| 范围 | 当前内容 | 限制 |
| --- | --- | --- |
| 地面站 | QGC Custom Build、地图、飞行器选择、链路与状态展示 | 沿用 QGC/PX4 原生安全流程 |
| 人工多机调度 | 明确目标的任务交互与编队协议 | 需要 Mock/SITL 和现场分阶段验证 |
| FTC 遥测 | 四类自定义 MAVLink 消息、类型化后端、状态面板、协议和过期判断 | 只展示飞控报告，不代表主动控制已验证 |
| 本机 AI Agent | FastAPI、Mock/Ollama、C++ 客户端与服务监管器、Schema/Policy 校验 | `ActionProposal` 保持 `executable=false`，不进入飞行执行链 |
| 植保区域覆盖 | [ADR-0001 设计方案](docs/adr/0001-plant-protection-area-coverage.md) | 待实现与验证，不包含喷洒设备控制 |

AI/LLM 不发送 MAVLink、不修改 PX4 参数，也不调用 Vehicle 或 Swarm 的飞行动作入口。云设备网关、生产认证、GIS Safety Service、真实命令执行器及安装包签名尚未完成。

2026-10-14 源码增量234–238加入[启动前整组任务只读核对](custom/src/Swarm/FleetMissionStartContentCheck.md)：实际原始INT全读/完整载荷核对与最早READ START时效接入现有面板及独立风险审阅，旧批准先撤销，不自动恢复或续期。它不是StartPermit、原子机载快照或任务完成证明；未编译、运行、测试、修改飞控或更新PR/安装包。完整25目标/六类剩余源码范围见[路线图](docs/MERIVUS_FLEET_OS_ROADMAP.md)。

## 获取源码

```sh
git clone https://github.com/Ale-xl/MERIVUS-GroundStation.git GroundStation
git clone --recursive https://github.com/Ale-xl/MERIVUS-FirmwarePX4.git FirmwarePX4
```

两个仓库建议放在同一父目录，便于交叉检查 FTC 协议和复现 MAVLink 生成。GroundStation 的当前 Git 树将第三方依赖作为源码快照跟踪，没有 Git 子模块；FirmwarePX4 使用锁定提交的子模块。依赖来源和上游版权按各目录许可证保留。

## 开发与验证

Windows 产品基线为 Qt 5.15.2 / MSVC2019_64 Qt Kit、Visual Studio 2022 x64 工具链和 Python 3.11。环境与构建入口见 [Windows 构建说明](docs/development/BUILD_WINDOWS.md) 和 [构建与生成](docs/development/BUILD.md)。

```powershell
# 静态契约检查，不连接真实飞机
pwsh -File tools/dev/test-ftc-telemetry-contract.ps1
pwsh -File tools/dev/test-field-telemetry-contract.ps1
pwsh -File tools/dev/test-takeoff-health-contract.ps1
pwsh -File tools/dev/test-sitl-swarm-task-isolation.ps1
pwsh -File tools/dev/test-version-contract.ps1

# 配置好 Python 测试环境后执行 Agent 测试
cd agent
python -m pytest
```

完整构建入口：`tools/dev/build-merivus.ps1 -Configuration Release`。FTC 后端单元测试见 [test/FTC](test/FTC/README.md)。真实模型评估需显式选择 Provider，不由默认验证自动调用。

## 仓库结构

| 路径 | 职责 |
| --- | --- |
| `src/`、`libs/` | QGC 主体与第三方依赖源码 |
| `custom/` | MERIVUS 界面、C++ 扩展、人工调度与策略测试 |
| `agent/` | 本机 AI 服务、Provider、测试与打包 |
| `schemas/`、`configs/` | 协议源文件、JSON Schema 和配置模板 |
| `tools/dev/`、`test/FTC/` | 构建、协议生成、静态检查与后端测试 |
| `docs/` | 当前架构、接口、设计决策、开发和验证说明 |

## 文档与贡献

- [文档索引](docs/INDEX.md) · [工程交接入口](docs/handoff/README.md)
- [本批次变更文件快速索引](MODIFIED_FILES.md)
- [当前状态](docs/architecture/CURRENT_STATE.md) · [模块状态](docs/PROJECT_STATUS.md)
- [FTC 遥测契约](docs/architecture/FTC_TELEMETRY_CONTRACT.md) · [测试矩阵](docs/testing/TEST_MATRIX.md)
- [贡献指南](CONTRIBUTING.md) · [版本记录](docs/releases/CHANGELOG.md)

本仓库延续原 `Ale-xl/MERIVUS` 历史与 PR 记录，产品导入起点为 `b3d6659`。`.mailmap` 统一维护者旧身份的显示名称，保留原提交 SHA；具体归属边界见贡献指南。禁止提交真实凭据、生产坐标、飞行日志、模型权重、厂商原始资料和构建产物。

## 许可证与上游

基于 [QGroundControl](https://github.com/mavlink/qgroundcontrol) 二次开发，保留上游和第三方版权、许可证约束。产品历史始于源码快照，不能把快照导入作者当作全部源码作者。见 [COPYING.md](COPYING.md) 与各源文件声明。

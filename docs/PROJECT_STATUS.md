# 项目状态

## 2026-10-18 组织仓库审阅分支

完整本地交付与 `Merivus-Industrial/MERIVUS-GroundStation` 的 `ffe42d8` 主分支已在独立发布工作区整合，准备提交中文 PR，等待人工审阅；没有合并主分支。当前整合版本未编译、测试或仿真，下面保留的旧 FTC 构建/链路结果不是对本次整合的验证。参见[本次整合记录](handoff/MERIVUS_INDUSTRIAL_SOURCE_SUBMISSION_20261018.md)。

## 546 当前：本地源码最终交接收口

用户已明确最终范围为“本地地面站源码＋交接文档，外部未完成项单列”。最后五条本地操作链已接线：规则待审、Run后台检查点、用户模板、显式日志留存、四受众审阅报告；作者与非作者限定静读及本轮文本/路径核对已完成。本地交接内容收口，统一使用最终完整源码包，不再另发阶段包。

本轮26源码/登记＋26文档，共52路径。数字不代表完成率；整个25项产品能力并非全部落地，外部协议/服务/实测模型及工程验证见[当前范围表](handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md)。未编译、运行项目、测试、仿真、刷机、修改PX4或提交/更新/合并PR。最终包是源码交接，不是可安装版或飞行放行版。

下方545及更早为历史记录，旧“还有四条缺口”已由本节五条收口更新，旧百分比不再采用。


## 545 当前：证据链保存和独立历史查看已接线

现指挥中心可明确冻结保存完整证据链、后台验证打开历史、四条分页并查看有界纯文本详情；不替换活动链或恢复权限。保存受理、文件写入和读回复核分别显示，取消/关闭不撤销已写事实。原生后台与窄门面、QML及资源登记已接通，工程验证未执行。

本次7源码/资源登记＋14文档，共21路径，新面板qml/md两文件。544公共航线修复已静态收口。本次还将实际源码缺口与真实性能、远端服务等外部依赖分开记录，避免把工程师的实机验证无限计作尚未编写的地面站代码。原25目标尚未全部完成，无阶段包、编译、运行、测试、仿真、刷机、PX4修改、提交或PR更新。

## 544 当前：公共航线进入实际成员

已修改普通公共路线的编译链：合法路线展开到缺省成员，预算、预演和载荷使用同一份实际路线；显式坏成员路线不回退，coverage不继承公共路线，时间别名、返程要求与参考不被静默抹去或推断。

本次2源码＋14文档，共16路径，无新文件。原25目标尚未全部完成；不制作阶段源码包，继续面向一次完整源码交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR；没有新的完成百分比或交期承诺。下方为历史。

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

[输入所有权](../src/QGCOwnedDownloadFile.md)、[接线](../src/Vehicle/ComponentInformationManager.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。


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

## 509 当前：人工单流启停，不是自动视频降级完成

已接独立control action／typed双账号purpose、真实Capture一次opaque Basis、最早30秒Review、原生单次STOP／START→Status回读，以及现有Root／面板消费者。原READ公开结构不变，READ成功不授控制权；START恢复也须重新读取、审阅和批准。

正常legacy同栈STOP旧流→START新流保留旧路径；新的受控事务只将自己的tagged action接入共享lane，双方忙状态互斥。实际启停推进非回绕本地mutation。自己的精确action写入后只允许Info的RUNNING位变化，私有配置语义代次仍冻结并防其余配置ABA；真实回读保连贯Info／Status及原年龄。

成功只表示该次reported-target-match，非实时视频状态、无线因果、物理停流或带宽认证。270索引兼容性仍待工程师确认；不支持无广播／stream0／2505回退，失败保已发事实而不自动恢复。

[控制合同](../custom/src/Swarm/FleetCameraStreamControl.md)、[30路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项全部未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批14源码／接线＋16文档，新路径仅Control h／cc／md三条。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次／路径／局部保护上调。完整任务执行、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态继续待开发。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、执行项目／解析器／helper／模型／QML／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

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

## 506 当前：现有相机／视频启停与选择生命周期

本轮修改现有CameraManager／CameraControl→VideoManager真实调用链：本地播放意图、原相机／流选择代次、有限接收器恢复、活动飞机切换和通知回调后的对象存活检查；共享QmlObjectListModel补通知后的自身存活尾核与匹配reset。不是新增只读helper，也不是完整Fleet视频降级／受控停流事务。

本地wanted意图与来源／选择快照分离，start／stop完成有单独pending，恢复按稳定意图有限预算。Manager发现失败不误记成功，重复ready不叠操纵杆连接；Control重复Info不反复使解码器重启，固定数组字符串有界读取。

[合同](handoff/MERIVUS_VIDEO_LIFECYCLE.md)、[19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[50项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505 当前：机载日志有界缺块补读与文件成功语义

本轮修现有LogDownloadPage→LogDownloadController→LOG_REQUEST_LIST／DATA→本地文件的真实消费者：同一目录／批次来源、冻结选择、有界缺块补读、完整写入后位图与去重计数、NewOnly partial及完整刷新／改名后才显示Downloaded。不是另建只读建议helper，也不是完整Fleet控制中心补传。

500ms只由新完整块推进；最多5次无新块补请求。目录最多4096项／60秒，选择最多64项，单文件512MiB／15分钟，整批1GiB／1小时；重复／非法包不刷新进度。整批选择与id／size／UTC冻结，不从新Vehicle或新目录继续旧文件。

仅本进程同会话缺块补读；无checkpoint、跨重启续传、Fleet outbox或远端幂等ACK。LOG无nonce、size可近似；Downloaded只认公告字节域，不能证明真实远端文件已取尽、ULog有效或身份认证。

[合同](handoff/MERIVUS_LOG_DOWNLOAD_RECOVERY.md)、[14路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或重试次数上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504 当前：目标14增加单次地面定向往返耗时来源

504新增独立人工单次定向TIMESYNC往返耗时采集：实际主账号／Role／Root原意图→Swarm私有服务／原生Run→真实同车绑定review→Policy及可见面板。最早四秒包含授权核验和后续100ms轮询，不自动采集、重试或续期，不回退PING或广播。

独立fleet.telemetry.rtt.capture实际主账号、READ免二审，不能借interval capture／configure权限。native26、current18、result5、wrapper7和Policy23／localLatency10已接实际消费者；只有同车／同record／binding、两源current、完整回显及≤30秒年龄才补唯一latencyMs。四秒是原采集／回包接纳期限，终态30秒仅供诊断，不恢复发送权。

当前仅受控PX4多旋翼地面。MAV1、无目标扩展、旧回包、不支持或超时都未知，不能回退广播；未回应不算丢包。目标14仍缺容量／丢包来源、连续测量与飞行场景、视频／速度联动、稳定恢复补传和效果验证。[完整合同](../custom/src/Swarm/FleetLinkLatencyCapture.md)、[31路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503 当前：目标14新增本地主链路接收证据竖向链

503将实际PRIMARY／本车默认component的原始解码帧接到独立五秒接收窗，经Registry实际绑定读取、Policy独立消费、Guided原五秒周期和指挥中心只读摘要。记录接收帧数、编码MAVLink字节率与最大内部间隔；不把它们当可用带宽、RTT、丢包率或远端发布率。

满五秒且来源新鲜／原RAW静默、未饱和才给计算值；从未接收／陈旧／容量截断保持未知。完整零接收只说明本地尾窗没收到帧；最大内部间隔不包括窗两端。来源失效换epoch，global receipt不归零，失败不改变旧RAW／P0流程。

实际五秒消费者不再把metadata／旧RSSI／seq-loss当实测QoS。Policy保持unknown／link_lost和建议边界，指挥中心显示“该次快照”。[合同](../custom/src/Swarm/FleetLocalReceptionObservation.md)、[23路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。目标14仍缺可信RTT／丢包／容量来源、视频／速度联动、恢复补传和效果核验。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502 当前：目标14已有人工可见采集／审阅／审批入口

502新增FleetOptionalTelemetryPanel可见界面，在指挥中心和地图规划页复用同一actual Root。操作员显式只读采集→另行审阅统一目标→两项确认后独立账号审批→逐项配置与回读；固定62／74／83，原501实际后端和授权合同不变。不会自动采集、Apply、续期、重试或回滚。

本页仅由真实返回result5及同一当前workflow24的ID／代次／hash建立拥有关系；共享状态或历史不领权。稳定来源只取原scope／主身份5／双政策9，自有ID／代次／状态推进不误取消；完整sourceToken仍在新请求前精确比较。审批不借全局busy自拒，关闭／换源／输入变更退休本页资格并精确取消原ID，晚返回不能复活。

本页显示原30秒的保守剩余、固定三流、原生原因、已发／ACK／中间及最终回读；报告值匹配明确不是实际率／带宽认证。Qt／键盘／主题／布局／屏幕阅读器与真实时序未执行验证。uiWiringPresent=true仅表示源码已接线，runtimeValidated=false保持。

[面板](../custom/res/Merivus/FleetOptionalTelemetryPanel.md)、[24路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。目标14仍缺真实链路测量、视频／速度降级联动、恢复补传与实际效果核验；完整目标和原六类不缩。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501 当前：目标14接独立配置授权与逐项回读

501新增独立双账号的地面可选遥测配置：从未来新成功Capture的三条真实native上下文封存不可序列化Basis，在同一最早30秒内独立审阅／批准，再逐项511→510并最终三项510复核。仅固定62／74／83、同一正int32且精确REAL32间隔，目标不得比任一原正基线更短；不启停流、不降频P0、不改视频或编队速度。

start仅认领ready、不写包；100ms后推进九个wire阶段。每条至多4秒，510 accepted ACK后至多1秒且只缩，所有阶段与审批共用原Capture最早30秒。慢链路可能无法完成；截止／失败／取消保留已发事实，不重试、不自动回滚。三项最终回读只给reported-values-match，不认证无线因果、实际发布率、带宽或飞行许可。

Role新增fleet.telemetry.interval.configure强制二审；Session精确purpose为fleet.telemetry.interval.configure.approve，私有typed凭据只交实际配置owner。原Capture action只读、不兼作配置许可。正常二审prompt仅保原身份的Role显示；真正撤权仍退休。

[模块合同](../custom/src/Swarm/FleetOptionalTelemetryConfiguration.md)、[30路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[46项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。源码尚不包含新可见Apply按钮、真实带宽／延迟／丢包测量、视频／速度联动或补传队列；目标14和整条路线图未完成。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500 当前：目标14接显式主动读取，不接自动调频

新增Swarm私有持有的FleetOptionalTelemetryCapture，显式单车62→74→83，实际Session主账号＋独立fleet.telemetry.interval.capture政策＋原Root/controller意图。接纳后100ms轮询才发；原入口固定30s总期，native每项一次510／4s，accepted ACK后至多1s且只缩，必须原ACK与目标244双端齐全。最终write前复核授权、物理来源、完整载荷并冻结receipt基线；三项同原version／transport／mutation／已绑定ledger，已采语义A→B→A不复活。取消保已发／partial事实，先存历史快照再精确释放原query，不宣称撤回或实际频率已生效。

当前有Root值投影和显式request／cancel后台入口，无新可见操作按钮；实际service不能从导入Map／护照建立来源或配置权限。首次ledger绑定只在原明确同源bind处一次转移，unknown clear不重基；最终成功仍是该批历史观测，不认证硬件身份、无线因果、真实发布率或带宽。

9源码／后台桥接与工程登记＋15文档=24路径；新文件仅Capture.h／.cc／.md三条。原FleetRegistry／FleetLinkDegradationPolicy源码与15／21／6合同不改，新报告沿原账本进入既有只读重采。没有新可见按钮、qrc、依赖或可任意QML构造类型。

[模块契约](../custom/src/Swarm/FleetOptionalTelemetryCapture.md)、[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前：目标14修真实查询／配置通道及静默终结

510／511／512现走同一Vehicle级跨component专用lane：外调前预留UUID／不回绕serial，固定原提供者QQ、PRIMARY弱控制块与实指针、transport代次及原固定期限；原nativeVersion直接入队也接入，但保原地面／参数／实际授权fence和单次3秒，不借generic条件放宽。generic初连不要求MVM已登记或握手已完成；版本资料首次锚定与实际transport变化分开，native批准依据不自动rebase。

通用512的RequestMessageInfo由Vehicle持有shared context，ACK与原始回包两端终结前保原lane。原500ms响应timer在queue空之前也核固定deadline，ACK后1秒回包窗口只缩原期限；静默、IN_PROGRESS或重试不使等待无限。原包入口冻结原请求归属，实际回调仍在P0原生围栏之后；消息0合法，不再凭0哨兵判无请求。可选callbackOwner仅是QObject寿命条件，不是角色／账号授权；InitialConnect与StandardModes实际传原owner，未传owner的旧borrowed-data调用者仍自行保证数据寿命。

本车默认component的任意511只在最终actual write前，以同UUID一次预留推进不回绕mutation generation，并全清498三report及接收窗；ledgerEpoch为UUID:十进制generation。writeDepth贯穿write与通知，临界内原包准入／getter不可当前。未发送拒绝不冒称已配置；已可能write的代次／requestIssued不回滚，耗尽保守停止而非重用。498观察15／Registry review21／comparisons6保持，后续只读重采会看到旧基线失效，既有值快照不自动变成实时通知，须新被动资料与新完整5秒窗，不等于实际生效。

InitialConnect的UID／版本／Hash／能力、provider／Link虚调及同步状态续作增加原QObject尾核，Vehicle能力通知之间亦防删除；StandardModes按整份列表推进不回绕read generation，阻止modesUpdated里新读同步失败后旧completion再次发布。原模式内容、DAILY_BUILD和握手步骤语义保持；没有宣称所有legacy调用者整体安全已验收。

Vehicle公共raw sender在plugin前后拒510／511／512旁路，正式请求走专属通道。此覆盖不包括MAVLinkProtocol转发、移动Taisync字节桥、外部GCS或固件内部调频；本批不改变转发策略，不认证实际发布率、丢包或带宽。

本批[18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[42项工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前：目标14补可选遥测的真实被动基线与只读比较

Vehicle只观察原始PRIMARY的MESSAGE_INTERVAL244及固定NAV_CONTROLLER_OUTPUT62、VFR_HUD74、ATTITUDE_TARGET83。244保原signed间隔：−1已报告disabled，0已报告not-available，正数为报告间隔，低于−1为invalid；未观测是null而非−1。报告fresh与available分开，所有值均非身份／实际发布率／带宽认证。

每流5秒尾窗、最多512个单调接收时刻；原来源绑定满5秒、该流曾收到且最近包不超过30秒、无相关接收栈／探测中状态和窗内容量丢失才可complete。完整窗可以0帧，仅表示本地该5秒没收到；从未收到过不能complete。均值是本地收包数／5秒，最大内部间隔不含窗两端空隙；不推算发送率、链路丢包或停流。

Registry仅从现有friend建立的实际Vehicle绑定读取一次原生快照，前后核记录与绑定，不以metadata／护照／调用者Map替代。null／0仅无提案，正间隔须int32整数且精确REAL32；三条当前新鲜正基线都已知、提案间隔均不小于原报告间隔，才给数值不增频结论，仍snapshotOnly且所有许可false。

现有custom Guided五秒策略快照把Policy正rateHz换成ceil(1000000/rateHz)的只读间隔提案，经实际私有_swarm.fleetRegistry调用并附optionalTelemetryRateReview；0／未知不变为停流／固件默认。Policy源码、P0流、旧PID／ESC与发送器不改，公开facade不暴露原生setter或新增apply。

本批[5源码＋13文档=18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)；[36项工程验证](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前：目标14前置修现有PID／ESC的511消费链

本批修现有511（SET_MESSAGE_INTERVAL）消费者，不接通Fleet自动降频。StreamConfig改非零不回绕requestId、唯一pending、先pop后callback和迭代pump；单次最多64实际callback，失败保possibly changed供后续显式选择／restore。callback副本在私有weak lifetime最终尾核前析构，禁copy/move，销毁后不再访问helper。interval=0仍仅固件默认，不是原配置回滚。

公共511 entry固定原VLM／Protocol／Plugin、PRIMARY QQ与weak控制块及实际pointer、原source epoch和本地UUID；复用构造期持久pin，不从已销毁raw重建。外调后入队前再核duplicate/quarantine，实际发送前核原UUID及完整payload，失败按原UUID重找。ACK只接同原物理来源，IN_PROGRESS不续期；511超时加入原隔离，不新增每个成功终态冷却。

PID与ESC各自一次handler/context消费自己的本地请求，删除原任意live511推进PID／ESC的分支。STATUS5Hz→INFO1Hz、原PID两消息100Hz及手动默认恢复入口保持；拒绝／超时不自动续链。same-source迟到重复wire ACK仍可能歧义：协议不回显messageId，本地ID／UUID／来源核对不是wire因果认证，accepted也不证明实际频率、带宽或安全效果。

[18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。真正的目标14仍需独立实际Session/角色scope与必要二审、当前原生链路证据／期限、闭合optional白名单、原MESSAGE_INTERVAL244基线/回读及收包频率窗口。不能借只读refresh或start/capability凭据，不能照搬Policy的全telemetry.rateHz，更不能降频P0证据或控制协议；本批没有新apply UI/入口／权限／自动飞行动作。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前：P0垂直参考保完整两端高度编码范围

495只补top→首NAV全帽，旧Contact垂直段仍是名义端点。本批新增同次成功TAKEOFF support的原生起点／原窗口参考；Waypoint重算full current/full top，以原connectionHull保两端全Z，并按原protected/support误差送原环境比较。旧名义组件不替换，不宣称新体每输入更大或真实严格升高。

full current必须整体落原rest±容差，schema1／缺成功ref／窗口不足整组不完整／失败且无几何前缀。只有新vertical/support指定terrain的完整交集可按同finitepad判例外，body、obstacle、top→NAV及Domain不借；旧LAND、NAV计数、公开合同及七false保留，模型v5。

[4源码＋15文档=19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。真实ground状态、动态接受／平滑／连续轨迹、StartPermit及正式结果仍未完成。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前：P0补完整起飞编码参考区间到首航点的连接

之前Joint保两条名义中心线，Waypoint仅16→16相连；现在从完整原Motion source的GPI9、同次PM29及wire重算ground-branch条件编码top区间。新连接按原baseline E7而非请求XY起步，连接首NAV全部接受帽和完整Z，依原构型／误差外扩后实际送原环境比较，不再只核名义中心。

缺来源／顺序／类型或任一完整投影不支持时整份不完整／失败，无可用几何前缀；新体不借起飞或LAND接地pad。v4沿旧31键／schema1和原NAV计数，Worker输入输出不改、原生来源／期限与最终actualPath真要求不删。此模型不自行认证landed/内部状态，也未覆盖真实起飞接受域、垂直整域、动态平滑或持续HOLD。

[16路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前：保护记录报告从本页快照扩到显式文件历史

489的独立report8不再只能本页复制JSON：新文件口完整核record／summary／claims／正文hash，保原ID、捕获时间和内容；只在已知schema槽恢复JSON小整数，不放宽原生来源Int。保存前核预期ID与hash，读取有UTF8、预算、完整长度／尾部与关闭检查。

原报告面板增加与旧full9分开的待写／待读确认、取消和历史origin。导入不注册回Run、不恢复占用／批准或把sourceId当当前原生来源；目录变化不冒充刷新导入材料。没有数字签名、存储身份认证、全历史索引或完整结果闭环。

[18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前：长期运营增加真实遥测跨文件检索

原遥测目录只查header、页只读单basename；现在新增独立search，从同一原完整catalog冻结ordered descriptor manifest，按实际已校验记录的sysid／msgid统计。各file完整前缀／源序／footer／重开hash通过才给已知计数；坏文件是未知诊断，后段坏行不能保留先前命中当成功。

新增闭合操作入口和实际现面板：显式首批、同条件下一批、人工选命中、锁原fileHash打开单文件首页。索引按最后已检查file推进；最多128目录文件、64文件一批、单文件扫描64MiB、预留读字节130MiB及1MiB结果不放大。每批前后核完整manifest及原目录身份，跨批不认证旧文件内容永不变或跨文件原子快照。旧单页位置同意／报告不被搜索替代。

[6源码／UI＋14文档=20路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。本项不是全历史持久索引、任务Run关联或完整运营；原25目标与六类实质剩余保留，约40%完成／60%剩余仍仅源码工作量粗估。完整动态接受／动作／连续轨迹、StartPermit和正式远端结果，以及可信性能、自愈机载改派、企业跨站、长期运营、真实模型生态仍需继续。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前：P0闭合执行观察的同步外调生命周期

已确定原执行观察lane的三项Link虚调用可同步退休／销毁对象，RAW、poll和只读投影等调用者却仍持原Target引用；同一外调还可使Orchestrator QHash元素重排。此次保留全部live检查，补原native对象／弱PRIMARY／版本／槽代次逐外调尾核与重入限制，Run用独立快照和单调变更代次，原行必须在回调后重新定位才可写。

Vehicle与Run在构造期一次保留原来源QPointer，原对象失效后下一次入口也不能从遗留raw重新构造QPointer或替换来源。注册／更新／报告及退休清理的二级调用者同样先核原QQ／上下文。Orchestrator四Evidence入口释放旧task指针、返回后重找同id核完整24字段，不以identity或可能饱和的revision替完整状态。注册的destroyed连接先本地暂存，connectNotify后复核原完整六项Evidence并重新查重／容量／冲突，再发布。计数耗尽停止新的可用source探测／推进，但已实际write产生的原sameQueue commit仍须保留为历史，不变成撤回／未发送或正式完成。

[7源码＋15文档=22路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36项工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25目标和六类实质剩余完整保留，约40%完成／60%剩余仅源码工作量粗估。动态高度／实际动作／平滑器／连续轨迹、StartPermit与正式远端实例结果仍未完成，遥测跨Run检索等运营项亦仍待开发；不能用本轮重入修复上调验收率。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前：P0补LAND非零接受区域到支撑面的参考几何

当前支持的LAND与前NAV编码E7 XY本来相同，因此原名义中心线已经垂直；真正漏项是前NAV非零XY／高度接受区到声明支撑面的完整参考体。本批沿原PM29与同次Joint／schema2 contact来源构模，不把LAND p7当实际停止高度。

新模型分别检查无接地豁免的入口XY凸包×完整入口Z区间，以及同完整XY凸包×支撑参考面至入口顶的下降参考体。后者保留水平收敛与下降可同时进行的声明连接，而非仅目标XY竖柱。机体／support分别膨胀后进入原五类环境比较；仅下降support指定地形的完整交集在原有限pad内可豁免。缺来源／不支持保持整份不完整或失败，无可用几何前缀。

[4源码＋15文档=19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36项工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。动态高度接受可能由内部状态扩大，真实MOVE入口／分支、平滑内部状态、真实地面／时间及actualPath／StartPermit仍未完成；原25目标与六类实质剩余保持，整体约40%完成／60%剩余仍仅源码工作量粗估，不按参考体或路径数上调。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前：P0参数末端连续性与覆盖算法接线

原启动封条只固定PM对象和其它状态，未将原29参数内容带到最后native queue。本批入口严格读取beforeArm.parameterSummary.originalExecutionParameters，固定actual PRIMARY弱控制块，初核PM29；后续stage不更换控制块或重新读参数，封条独立持有完整快照，current()末尾按原PM语义核原29。补上无clearanceInput路径的同一缺口，有资料路径仍保原Witness独立检查。

完整并集覆盖新增原生索引候选口，两个真实消费者的coverage／corridor／terrainHorizontal共六处调用已接。所有scene行先准备；只剔除与query bounds严格分离者，原完整query继续求解，空候选不抢判。terrainHorizontal用独立±51000000mm全Z，原接触索引保持。工作2M／片段2048与取消机制不增额；稀疏场景有减少重复面准备的机会，稠密及小场景可能更贵，无实测性能结论。

[8源码＋16文档=24路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36项工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；静读与普通文本核对范围见[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。原25目标与六类实质开发保持，约40%完成／60%剩余仍仅源码工作量粗估，不按接口／路径数上调。真实LAND移动到着陆点与下降动作域、起降／HOLD／平滑器及采样间动态路径、完整StartPermit和正式结果等未由本批解决。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前：同一受保护TaskRecord可显式捕获历史报告

此前执行报告只能从所选Series的actual previousStartup取得完整Run；保留在Orchestrator的旧／owner已销毁记录只有compact投影，不能伪补字段后喂旧build。本批新增独立原生来源链：轻目录→四字段选择→同一private TaskRecord16→static独立report8→本页单份快照／原生JSON。

目录只哈希轻量12字段，row13的selectionToken同时绑定修订和关闭／销毁／耗尽等元数据；不是全观察哈希或身份认证。修订耗尽后的强制闭合／销毁会改变选择标记，report仍明确revisionUnambiguous=false。Controller在构建及原生JSON之后两次核原实例／来源和完整record相等，不从当前owner或缓存显示补新鲜度。

原面板新增独立默认折叠区，不自动选择首条、不认领全局结果、不自动读写文件；失效／清理与选择票据防通知重入。目录变化可撤旧选择，但既有同源报告明确为冻结历史，不悄悄更新。原full报告、文件及handoff保持原类型。

[8源码／UI＋15文档=23路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，[36项工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。本批审计已记录独立源码／文档交叉静读、两处UI通知重入修订及最终普通文本核对；没有编译／运行证据。原25目标及六类实质剩余继续；约40%完成／60%剩余仅源码工作量粗估，不按本批接口／路径数上调。正式任务实例与完成、动态轨迹及StartPermit、可信性能、自愈机载改派、企业跨站、长期运营和真实模型生态仍待继续。

仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：P0真实观察关联和NAV空间模型修复

任务执行观察现已对post-commit `MISSION_CURRENT` 序号回退停止该机tracking：先保留实际负面／回退报告与receipt，再安全累计回退数并失效。后续REACHED／落地包不能借旧terminal／airborne引用新建候选；已有历史、真实MODE提交、其他成员和本地占用不受此修复删除。原250ms轮询负责解绑和任务投影通知，不在接收栈同步通知公共UI。

普通NAV连接改为两个端接受矩形的XY整数凸包×完整Z闭区间，而非覆盖无关角区的大AABB。机体／支承分别扩张，原完整coverage／corridor／terrain覆盖保留全部平面；接触索引只以AABB粗筛，再由原完整查询与同scene行做精确可行性检查。原共享预算／Unknown／取消保留，不承诺性能提升。

[15路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)含2源码／13文档；[32项工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。本批静态审计已记录独立交叉静读、源码／文档复核和最终普通文本核对；尚无编译、模型执行或运行证据。原25目标及六类实质剩余继续；整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批修复数上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍需继续。

仅地面站源码／文档；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。下方为历史。

## 487 当前：真实历史页形成配套资料报告

目标19推进到原始资料实际消费：现有ReportBuilder新增独立static历史入口，内部只调用MotionReview一次。完整dossier21保原31、六页身份、覆盖8、资料质量发现与四视角；视图显示实际聚合帧数／样本／断段与分页等状态，非首页另行说明。不借旧mission verdict／safety gate生成结论。

Archive先完整核结构、原页／31对应、body hash与1MiB预算；同次来源尾核后双缓存连续赋值、一次通知，两个getter共用双终结／最新页／六身份／嵌套31相同门禁。新读、取消和实际构建失败两者失效。Panel沿原确认与通知尾核，以一次savedReport保存两者，视角切换不读盘，可复制文本不自动保存／发送。

准确范围6源码／UI＋15文档=21路径，新文档1、无新注册；34工程场景全部未执行。本批审计已记录三路独立交叉静读、15文档终审及最终普通文本核对；文案范围已修正，但没有运行验证。采集run不是任务run，hash不是签名，资料质量不是事故原因／漂移或性能诊断；真实任务关联／结果、报告签名／存储与权限仍未完成。原25目标及六类实质剩余保持；整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批路径或视图数上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。

本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前：实际历史遥测进入只读运动审阅

指挥中心／Planner可显式读取固定本机目录下已存在的遥测NDJSON。原生query、目录、策略和实际next游标不交给QML；最多128文件、单文件64MiB、每页64帧。默认不录制、不建目录、不读盘；该入口不读取ULog或任意用户路径。

已读取位置载荷的最新双终结成功页，可经第二次明确确认生成同页报告值摘要。完整page36／row29先校验再decode；按源／组件及boot重复回退、接收时钟回退、mask变化和坏样本分段，不插值、不跨页。report31无raw／绝对坐标／UTC／epoch；未知-1不填0。它不是悬停误差、飘移原因、真实位移、实测最大能力、完整遥测或轨迹安全证明。

准确范围11源码／UI／登记＋14文档=25路径，新文件5；32工程场景全部未执行。本批审计已记录三路独立交叉静读、14文档终审和最终普通文本核对；确定问题已修正，但不代替运行验证。原25目标及六类实质剩余保持；整体约40%／剩余60%仍仅源码工作量粗估，不按本批文件或入口上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。

仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前：严格性能政策进入区域任务实际操作链

现有区域规划页新增可操作编辑器，冻结原Draft／selection来源／revision，显式保存三态性能政策。新草稿和旧档不会因展开、普通数值保存或预览自动产生false／true；待确认政策不被旧值预览或保存／换源绕过，隐藏不清待处理标记。原Root先撤旧预览／审批，再原源CAS；同值native no-op与实际变更＋1均区分。

可选requirePerformanceEvidence实际非null Bool存在时为schema5；完整来源仍exact5，旧1..4严拒偷带该键。实际task只在provided时输出top flag；Compiler完整重建并逐字段匹配，合法area false保plan false／0.7，true仍0.8，Store精确绑定存在性和值。完整编辑保存、恢复、修复和余段沿原完整源／政策复制链继续，无新增平行授权器。

准确范围9源码／UI／资源＋19文档=28路径，两新文件；32工程场景全部未执行。本批审计已记录三路独立交叉静读、19文档终审和最终普通文本核对，未报告确定剩余缺陷；这不代替运行验证。性能数据仍是工程声明，未实测／标定；P0动态状态、真实路径与最终许可等未因此完成。原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。

本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前：性能数字从只读声明变为受控录入

既有单机工程面板增加明确的第五操作：规范两数、两项unknown选择、完整能力清单和理由进入同一次原生审阅。当前完整匹配基线是前置；独立用途双账号一次消费后，Registry重stage核原源及拟context，一次CAS提交数字＋新context／approval，再通知。相同数字重新采纳也推进一次revision，不延用旧任务依据。

当前普通Swarm刷新不提供这两数字，不会每秒清空人工声明；显式外部描述／护照更新有新数字时仍覆盖并退役旧采纳，不静默忽略。原构型基线与观察历史不被性能操作伪造／重新批准；过程及返回值不产生飞行许可。旧四操作合同保持；新preview3/20及事件类型和两个历史筛选有真实消费者。

12源码／UI＋19文档=31路径，无新文件／依赖／登记；32工程场景全部未执行。实测清单、环境／载荷／电池老化模型、跨重启可信审批恢复、区域Draft严格政策入口和P0动态状态仍未完成。原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。

本批审计已记录三路独立静读、两处修正及最终普通文本核对，不是运行验收。本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前：可信性能类推进到已批准工程数值的实际准入

本轮有限源码增量是消费既有工程批准数字：共享exact11只在能力批准当前、完整构型基线匹配且同快照身份时返回canonical最大速度／满电续航，明确保未知。既有task严格策略进入选机和角色排序前过滤、Compiler全任务及逐成员速度／实际profile／coverage主备检查、备用独立预算和逐机能量准入；不是只增加显示标签。

strict Compiler0.8.0的策略和完整来源纳planId，PlanStore阻止该版本去flag；Coordinator capabilityEvidence/schema3保存逐成员完整声明及usable进入原fingerprint。严格能量只用声明续航和原电量比例，原生实际剩余时间仅额外取小，不能以metadata drain、untrusted direct或投影fallback替代。false／absent路径保旧规划行为，旧审批不自动变为性能认证。

新的性能录入器、可信实测性能清单、环境／载荷／老化标定、二进制及物理改装真实性认证仍未完成。P0 controller动态状态、真实路径和完整StartPermit缺证保false；另外受控自愈机载改派、企业跨站、长期运营、真实模型／插件生态等六类实质工作继续，原25目标不缩减。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径上调。

准确范围12源码＋19文档=31路径；30项工程场景全部未执行。本批审计已记录三路独立静读及最终普通文本核对，不等于运行验收。未编译、运行helper／模型／业务／QML、测试、仿真、硬件、PX4修改、刷机、放行、提交、更新PR或打包；没有本批安装包。下方为历史。

## 482 当前：P0普通NAV几何增加真实参数来源与实际第三报告

本轮完成的是有限源码增量：独立arrival参数 `NAV_MC_ALT_RAD` 与原12／2／14组成29项同源观察；Evidence/schema5和Readiness同次稳定快照进入Worker/schema3成员row4。原Vehicle／PM／PRIMARY弱owner、epoch／revision、Fact、pending和鲜度在两Review及Witness回调尾部核原29行，不重capture续鲜度。

新WaypointSwitchReview实际构造command16静态接受帽和相邻NAV完整AABB连接，扩张protected／support并送全组统一预算的环境比较；Worker结果exact7、两Review风险、Witness和两Authority完整消费。旧12hash／19键／PM12／14／28口与动作纯模型14输入不改。

这不是完整PX4动态接受或轨迹模型：controller动态高度接受、PositionSmoothing可达域、独立起降／HOLD与采样间运动仍缺证。新报告证明及权限字段固定false；既有actualPathCertified=false被实际准入消费，所以Compiler pending、完整StartPermit及正式任务结果尚未收口。可信能力性能、自愈机载改派、企业跨站、长期运营、真实模型生态等六类实质工作继续，原25目标保持。

准确范围19源码／登记／UI＋22文档=41路径；32项工程场景全部未执行。仅静态修改及直接源码阅读，最终普通文本核对见本批审计记录；未编译、运行helper／模型／业务／QML、测试、仿真、硬件、PX4修改、刷机、放行、提交、更新PR或打包。UI/UX只静态文案，未脚本／界面验证。整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批路径数上调。下方为历史。

## 481：新受控采样域的数值包含接入完整原生链

新显式profile选择不改变policy1–4结构；源由旧source1/12分为新source2/14。实际Motion和Cover共享唯一原生整数域，保完整地理链／反经线／整数AMSL slab，以原E7／毫米直接投影，不给未知Qt误差补一个任意epsilon。

DomainReview/schema2／model v3按全组原native域的完整投影计算numericContainmentProven；旧模型、混组、任何域／扩张／预算失败仍false。这个结论只说明实际受控谓词接纳的采样空间被外包包含，不认证Qt本身或真实飞行路径；环境比较结果另核。Worker实际准入还要求actualPathCertified，而现producer明确false，故仍clearance-blocked。

原Contact接地例外、Compiler pending、动作／采样间动态范围、最终StartPermit与正式结果未收口，六类仍有实质源码。UI需显式选择和重新确认，不沿用旧审批；整体40%不按本轮39路径上调。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480：起降分支由参数声明接入实际审阅

新增14项独立动作参数，结合原12项描述近地起飞加速度、降落分段／测距crawl／RC辅助及独立电机ramp参数；SYS_RESP三项覆盖按本地float32规则核原始值，不改raw。ALT1内部max、相等阈值和ramp正零合法分支保留，负零另作不兼容TAKEOFF分支且来源链识别符号变化。

schema4独立actionPhaseEvidence进入完整evidence／startupConfiguration／Readiness身份。明确TAKEOFF遇外部setpoint注入、明确LAND遇RC辅助或外部注入时，实际parameterEvidenceUsable与启动前置阻断，不静默改参数；非LAND不把RC开关单独误作LAND动作。原28行最终尾核及过龄／来源变化失效，不重采样替原行。

旧12hash／公开19键与PM12／14口保持，刷新联合28项及真实确认文字同步。本轮仍不完成数值包含、完整动作运行域、动态净空、最终StartPermit或正式结果；六类仍有实质源码，40%不按本批上调。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：原始速度参数与总速覆盖规则真正交叉消费

当前capture schema3单独保留两项ALL回执、模型、当前性与可用性，旧12参数和hash不改。实际模型核XY宏的CRUISE／MAX及Z宏的6项float32关系，失配不替换raw，而使真实parameterEvidenceUsable失败；原有profile意图／限速检查继续。

新增固定14整组memory gate补在最终版本／目录读取后，以及Readiness的末尾诊断和启动phase回调后；以原快照而非后采样核源、revision、PRIMARY控制块、pending及全组年龄。独立静读当前范围未见剩余确定缺陷，但没有运行证据。

本轮不完成真实纵向相位、RC辅助／TKO ramp、动态到点半径、实际速度／时间、StartPermit或正式结果；原六类仍有实质源码，40%不按本批上调。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478：完整边界来源与编码后实际坐标链已接源码

完整canonical原域从Compiler身份／计划进Store和PayloadBuilder；严格源对应覆盖区域草稿、覆盖三声明以及新single3／4、group2 provenance。所有可新构建计划必须有明确active／inactive策略，不用版本删除或areaSummary补无约束。

编码副本按当前INT发包取E7 XY与float32高度，保实际动作／去返连接和全组共享预算；失败无payload前缀。旧资料3处自洽重放走独立历史口，sourceTask.area与sourcePlan互核；Readiness当前口保完整策略。尚未证明真实路径／数值误差／动态净空或启动许可，六类实质开发继续。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477：正退让真实消费，不再只有零退让占位

area.boundarySetbackMeters明确保留0..500真实数，正值生成保守内缩／外扩候选，全部候选分量保留并按成员端点或负载分配。共享显式路线检查不生成候选，仅按原域和请求距离核全部路线／返程／桥接；已有欧氏可行路线不因保守方形候选的复杂度被误拒。

source4、一次CAS同时替换孔与退让、全部保存／修复源及原边界地图说明已接入。静审修复两字段修改沿用旧确认、保存清理隐藏新文字和取消重入覆盖新编辑。仍非编码后位置或实际轨迹证明；完整数值域／动态净空／启动许可／正式结果及六类实质工作保持。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476：排除区域已从编辑声明接到真实名义路线检查

区域覆盖支持最多8个内部排除环：扫描线减去孔内部，区内连接与区域外起返点连接分别构图，后者不放宽排除区。辅助路线、返程及去返桥接也必须符合声明；最终Compiler另核当前完整成员和备用预算路线，余段任务保留area后同样实际消费。

草稿完整替换／保存／历史修复认识source3，旧source1/2不升级；新增编辑、明确空列表确认及地图待审标记。未实现安全退让、真正动态净空或执行许可，也未运行验证。完整启动／正式结果、可信性能、自愈机载改派、企业跨站、长期运营及真实模型生态仍须开发。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475：完整原组可以生成一次联合人工新任务审阅

新增独立group来源及编译／归档／显示消费者，完整原成员闭合映射到唯一新目标；原绝对时序不降级，原净空输入不自动裁剪或重绑，共同政策与独立reservePlans保留。逐机最低速度、任务时长和返航下限留各自assignment，并由真实Compiler／能量派生分别消费。

实际联合编辑区支持逐成员参数、同组预览、一次后台审阅、全组路线／编码分页、明确采用和原本地批准；旧单机面板隔离group状态及取消。修正了取消取live请求、普通JS ticket造成ready不更新及input／Episode变化旧意图续作的问题。已有退出只撤本地上下文，不改原Run或飞机动作。

本次15源码、18文档共33路径，仅源码与静态交接。完整StartPermit／动态净空／正式结果、可信性能来源、机载改派／控制租约、企业跨站、长期运营和真实模型生态仍需实质开发。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474：同Run完整成员资料可以进入离线目录与人工选用链

新原生getter一次冻结原owner、完整有序targets与共同任务／计划，再取一次完整观察；各成员保原payload／映射／进度。合法未提交成员不丢失、但无选用资格；其它来源／预算／映射失配整份拒绝，不返回成功前缀。

新版包共同值只存一次，保旧单成员格式兼容；打开仅预览目录，换包清选择，明确选一个有支持路线的成员后才进入原包含式余段／完整停留／新任务审阅。未知旧格式begin不补false。仍非联合多成员新计划、自动持续留存、Run／控制权恢复或机载改派。

本次6源码、16文档共22路径。完整启动／时序／动态净空／正式结果、可信性能、自愈机载改派、企业跨站、长期运营和真实模型生态仍需实质开发。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：完整资料跨重启进入人工新任务，仍非原Run恢复

已接[资料包](../custom/src/Swarm/FleetMissionRemainderArchive.md)的显式冻结、保存、打开预览、单独选用及[原余段编辑](../custom/src/Swarm/FleetMissionRemainderArchivePanel.md)。原TaskRecord／报告／占用历史不包含完整规划来源；本包必须在实际原对象仍存在、目标真实MODE提交后明确捕获，旧缺项不能恢复。

新历史来源与原生来源严格分型，全部中央源／provenance消费者继续完整校验；新任务仍重填目标、起返点、速度和profile、重做所选航点完整停留并重新当前审阅。收起／隐藏不是退出，明确退出精确清本地旧capture后才允许换包；旧归档及实际Run不变。

当前仅11源码／登记资源、19文档的源码增量，未编译或运行验证。完整启动／时序／动态净空／正式结果、可信性能、自愈机载改派、企业跨站、长期运营和真实模型生态仍有实质代码；整体约40%不按本批编号／文件数上调。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：不完整编辑态的保存／重启恢复源码链已接入

[Store](../custom/src/Swarm/FleetMissionDraftStore.md)保存完整 canonical 编辑源，而不是要求先生成可编译任务；[面板](../custom/src/Swarm/FleetMissionSavedDraftPanel.md)提供明确预览、确认保存、读取来源、恢复新副本及会话切回。未提交文字先处理，构造／展开／启动无自动IO。

Root接原本地candidate／撤审批／来源重读／共享备用流；磁盘完整字节与原Draft来源始终分开。已提交但尾核失效保持事实；contextEpoch阻止旧控制器／Episode结果冒充新上下文通过。旧strict／历史计划不放宽。

这是11源码／登记资源、17文档的有限增量。尚未编译／运行／测试，不证明实际保存恢复可用；完整任务执行／StartPermit／正式结果、可信性能、自愈机载改派、企业跨站、长期运营和真实模型生态仍需开发。整体约40%不因本批编号或文件数上调。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：固定原Qt缓存操作数及完整源数据

普通／地面Review首次创建[原运动对象](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)后，源码链继续同Ptr到[Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)和实际[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)，实际finished核原集合，再将同对象交Witness／Txn。此前相同source12可在几个环节重新计算Qt缓存，不能据值相同声称是同次原运算数。

本批增加完整参数对应，保全部canonical输入规则与SHA；factory在完整有界校验后独立持有各层容器和文本，修外部既有元素引用／字符串缓冲改变已创建源数据的风险。无资料旧任务仍在Txn一次prepare，不新增资料依赖。

这是13源码／18文档的来源连续性增量，未证明完整数值接受域或动作净空。numericfalse、Compiler pending、最终StartPermit及正式结果缺口保持；原25目标及六类实质开发继续。准确范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：空间筛选实际接入整域环境计算

[Geometry](../custom/src/Swarm/FleetMissionClearanceGeometry.md)已实现原生自持索引、完整候选遍历及同索引原行SAT；[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)实际同时准备全部地形／障碍后使用。索引外层逐值独立保存，避免调用者事先持有的可变元素引用改变缓存来源；同中心不同形状或高度不合并。

闭节点仅以严格XYZ间隙排除，边／点接触保留。完整查询校验先于空索引及根节点快路径；候选有界、唯一且按原行升序，失败无前缀。稀疏场景可减少最终SAT对数，但准备／排序／密集遍历也有成本，未测CPU、延迟或功耗。

本批仍是3源码／13文档的算法与接线增量。原Qt数值接受域的包含、动作／阶段接触、完整StartPermit／正式远端实例结果及六类实质代码继续；未把40%粗估变成验收率。范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469：完整理想域的地形／障碍求交有专用精确算法

[Geometry](../custom/src/Swarm/FleetMissionClearanceGeometry.md)新增私有PreparedPrism：先完整校3–16严格凸CCW顶点、整数坐标和闭高度，缓存各边轴双向投影。查询必须是真正无额外面的轴对齐盒；完整XYZ区间及所有polygon边法线均不分离才证明交集非空，含点／线／面退化与相触，不以粗包围盒相交冒充结果。

[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)使用唯一预算一次准备全部地形和障碍，随后实际SAT取代每对通用3D顶点枚举。地形截于全局支持下界−51000000毫米，因query本在此域内，与原z<=terrainTop交集等价；不抬地形、不改高度带或机体包线。准备取消／资源／坏型分流，不查询前缀或默认对象。

当前为3源码／13文档的实际算法增量，不是完整Startup许可完成。原Qt数值包含、动作／阶段范围、StartPermit与正式远端实例结果及其它六类开发继续；没有编译或运行证据。准确范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468：理想域外包紧化与真实报告进度一致性

[Cover](../custom/src/Swarm/FleetMissionStartupDomainCover.md)将统一1毫米／E7经度界改为基于完整cap纬度范围的五带整数下界，经度外包不大于旧版且仍证明覆盖整个理想cap；原纬度、路线、完整高度带及工程frame不变。[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)模型名升v2，拒收旧v1报告及不完整计算引用未来component的矛盾明细。

地形XY覆盖直接使用原closed polygon竖柱；query的Bounds已蕴含原来逐component追加的上下Z面，删除复制和冗余平面不改变集合判断，实际地形接触最高界及障碍高度未删。共享预算及取消保持，workUnits可能改变；没有性能实测。

新代码仍只证明理想外包及声明环境计算，不证明原Qt混合数值谓词接受域。仓库默认／CI的Qt版本不是实际链接库或浮点编译契约；不新增自报数学许可。完整原数值域、动作范围、StartPermit、正式远端实例结果及其它六类实质工作继续。准确15路径见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467：整运动域检查已有实际算法和启动消费者，尚非完整准入

[Cover](../custom/src/Swarm/FleetMissionStartupDomainCover.md)以整数两端球帽外包完整理想走廊，再乘原独立高度带；[DomainReview](../custom/src/Swarm/FleetMissionStartupDomainReview.md)对全组完整保护／支撑组件检查场地和走廊并集、地形XY覆盖、地形及障碍相交。不裁到资料frame，不沿航点高度插值；超域或容量不足整份incomplete，不返回成功前缀。

[Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)实际生成两份报告并升级result6，两Review和Witness工厂共用同一接纳谓词，两Authority同步完整信封。旧起降动作pad在全域没有对应动作／阶段证明，因此本批取消其整域豁免，非空支撑接地候选仍保留；原名义接触语义不变。

原Qt／libm数值接受域到理想外包的包含仍未证明，numericContainmentProven固定false。有资料路径保存完整两报告后clearance-blocked，不生成启动凭证；Compiler pending、完整StartPermit、正式远端实例／结果及其余六类实质开发保持。不是只等工程师开按钮或测试。当前34路径见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466：从副本观察推进到历史保留区导入

[Worker](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)新增分段暂存复制、独立读回及原生无覆盖发布；[Store](../custom/src/Swarm/FleetPersistentEventStore.md)将同一次真实诊断的可读项绑定到私有预览，固定本地目标不由UI路径替换。[面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)区分已写暂存、读回通过、发布尝试及实际导入确认，不从请求接受推断文件效果。

此为单段历史副本导入retired保留区，不是全系统灾后恢复。原源真实性／完整备份集合仍未认证，不创建缺失目标目录，不替换当前库；部分暂存和未知发布不自动修复。完整任务执行／正式结果、可信性能、自愈机载接替、企业跨站、持续全遥测及真实模型生态仍有实质开发。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465：坏候选不再只能得到整目录失败

[Worker](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)新增独立诊断通路，在完整原生候选枚举后逐文件分类；[Store](../custom/src/Swarm/FleetPersistentEventStore.md)与[面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)接同一次23／26原结果及三计数。拒收候选不消失、无descriptor，不推断可删除或不可恢复；旧严格目录检查保持。
取消、目录枚举／关闭失败、观察目录或原名／类型变化、可读项尾核变化及结果超预算均整包失败，不交付成功前缀。原源对应和完整备份集合仍未验证；实际导入恢复、留存清理、完整遥测运营及其它五类工作仍须开发。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：原归档不可用时也能显式检查既有副本

[Worker](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)独立读用户指定备份目录，新的原生EventBackups枚举与实际hash／SQLite身份检查不隐式访问构造传入的原源目录，不打开原当前库或取得源锁；[Store](../custom/src/Swarm/FleetPersistentEventStore.md)和[面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)接新catalog24／entry9，沿原单槽和actualfinished。用户明确选中的目的目录不因与原路径相同而自动改写。

只把完整检查后的清单作为本次观察，不把新测得hash当已知原hash，不把filename里的三个UUID当身份认证。原源对应和完整备份集合均未验证；目前partial或坏候选使整次失败，不自动跳过、删除或修库。正式恢复、错误条目处置、留存回收、完整遥测运营及原其它六类实质工作继续。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：长期归档增加可核对的单段备份

原[维护Worker](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)、[Store](../custom/src/Swarm/FleetPersistentEventStore.md)和[面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)新增显式目标目录、原生预览、无覆盖复制及重新打开核对的实际源码链。复制完成不是读回通过；原源仍需对应。取消后可能留下部分文件，事实保留到实际线程结束，不自动清理。

这只推进单个保留事件库的备份，不含全遥测／全历史／自动留存回收、独立备份介质认证、长期备份索引或灾后恢复。完整运动域及StartPermit／正式任务结果、可信性能能源、自愈机载接替、企业跨站与真实模型生态仍有实质代码。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：长期归档增加可恢复的分类维护流程

[维护Worker](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)与[面板](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)接同一实际Store：先查看真实库存，再选单文件、准备原生预览、单独确认和提交后台工单。移动实际结果与请求accepted分离，未知／可能已移动不自动重试或回滚。

sealed与retired是不同命名空间；普通历史查询不会自动读取retired，恢复只恢复归档检索，不恢复飞行任务。退役不释放磁盘，尚不是完整留存／外部备份／永久清理或灾后恢复平台。完整启动准入／运动域／正式任务结果、可信性能能源、自愈机载接替、企业跨站、长期运营和真实模型生态继续。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：长期运营增加可追查的保留任务目录

[Store](../custom/src/Swarm/FleetPersistentEventStore.md)复用实际Reader的无筛选保留行，新增原生有界任务诊断累计；[Reader](../custom/src/Swarm/FleetEventArchiveHistoryReader.md)只在明确提供taskIdHash时使用新版本，旧五字段搜索与原28字段结果保持。新[任务面板](../custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)接真实窄facade，可显式继续目录及按选中任务读原记录。

taskIdHash来自原实际event.payload，嵌套summary没有保留下来的任务状态／成员不能猜回。目录总计与单任务统计仅为已观察保留行；扫描到末尾不等于历史完整、任务成功或跨库原子快照。完整运行任务／StartPermit／正式结果、可信性能与能源、自愈机载接替、企业跨站、长期运营和真实模型生态仍有实质代码工作。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：电池原始负面闭环，不冒充可信续航模型

[Vehicle原始账本](../custom/src/Swarm/FleetNativeBatteryObservation.md)原6值扩为10，outer20 schema2／row19保charge_state与fault_bitmask及known。MAV1／没到扩展为未知，MAV2零尾截断沿原zero-fill；32位最高位保为qint64。两字段变化进入原semantic revision，不再只看电量／时间是否变。

[共享检查](../custom/src/Swarm/FleetNativeBatteryEvidence.md)被Registry、HealthCenter、Scheduler实际消费；当前fresh主／副电池负面均阻断，合法证据不被丢成unknown后回退，过期负面保未解决unknown。FTC合并前核实际Registry aliases、合并后生成check4；任务门禁独立保blocked IDs。原主／备用exact18账单、Risk／Coordinator／Deployment及启动健康门禁继续消费，不新开许可。

Swarm和Guided已适配原生新版本／故障身份；[phase seal](../src/Vehicle/FleetStartupMemorySeal.md)补原电池typed账本／来源和原fresh窗口，最后write原整组内存尾核继续消费。无源／无旧fresh行不新增“必须安装电池扩展”的默认条件，已有原值变化仍不能给旧判定续期。

可信性能／耗电标定未完成；整个运动域、完整StartPermit、正式结果、自愈、跨站、长期运营和模型生态继续。浮点高度range研究未据不完整FE／FTZ合同改写原算法。准确11源码／19文档见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：同一原生模型从Witness到实际启动阶段

[Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)在原工厂内缓存完整原input2逐机source12的不可变模型，getter经原内存／时效门禁才返回。[Txn](../custom/src/Swarm/FleetMissionStartupTransaction.md)新请求只准备一次当前原beforeArm来源；有资料须与Witness原模型完整source规范JSON相同后持原Ptr，无资料持本次原Ptr。全部绑定成功才提交请求，不用逐阶段新模型替旧依据。

[Transition](../custom/src/Swarm/FleetMissionStartupTransition.md)初始contains及有序GPI重放只复用实际owner缓存，缺失即拒绝，无JSON或新prepare回退。原策略、航向累计、LOCAL／ODOM／HOME／EST／ALT、权限、时钟和整组phase seal保持。源码去除重复准备，不声称测得CPU、延迟或功耗改善。

本次追查确认旧普通Authority源指纹已绑定GPI9，未证实静默重基漏洞。现有工程projection error也不是Qt/libm数值误差证明，资料maxDelta、LOCAL／ODOM及speed×phase不能充当旧全域上界；完整球面走廊并集×独立AMSL slab的包含关系仍须开发。Compiler pending、完整StartPermit、正式结果及其余六类保持。5源码／16文档共21路径见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：实际运动谓词共用，完整运动域求解仍在后续

[共享模型](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)由实际Transition消费，原GPI到全部地理编码项按顺序连接，速度项不断链；缓存不变端点和限值，不改变原球面走廊、独立AMSL区间、速度／航向累计表达式。不是原点至每项的星形辐射，也不是名义三维路线。

[Worker输入2](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)必须保留每机原GPI9、完整载荷／hash、规范策略和原ceiling。两类Review从同一次原生capture／首次READ风险取源；普通策略从同原member的规范化profile取，不误读不存在的capture字段。完整source12进入同一输入hash，既有原生Witness继续保存原input／result，原时钟／UTC／source18／datum14不变。

实际环境求解仍只覆盖原Joint名义段；row3显式投影到旧row2，不增加scope或认证。原球面走廊并集×完整高度区间及机体／误差包络包含尚未实现，Compiler pending、完整StartPermit、正式远端结果及其它五类代码继续。8源码／登记＋18文档共26路径见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：修普通RAW回调窗，新增原阶段检查的整组内存尾栅

[RAW入口](../src/Vehicle/VehicleStartupRawIngress.md)在任何可能回调之前冻结原owner／PRIMARY／journal及其operation／原时刻，整栈持depth；原包不能附到回调中新步骤，嵌套乱序／坏帧／丢弃令原journal失效，closed组屏障继续记账。

[Seal](../src/Vehicle/FleetStartupMemorySeal.md)是不可从QML／JSON构造的原生只读对象，固定原typed账本／raw token／receipt和原来源；[Txn](../custom/src/Swarm/FleetMissionStartupTransaction.md)逐机完整phase检查后与所有回调结束再核原整组，成功才提交本generation／stage／startupId集合；命令入口不另取新seal替旧判定。诊断回调、阶段屏障和发送前补完整gate，新prepared slot先重核完整授权再读seal，避免正常步骤必拒。原政策形状复用同一纯validator，不伪造完整profile。

这补456明确的RAW首回调和全组motion记录尾栅缺口，不认证物理连续轨迹、同采样或整个运动域净空。Transition现有球面走廊并集＋独立AMSL slab与已求解空间包含仍未证明，Compiler pending保持；完整StartPermit、正式远端结果及其它五类开发继续。范围10源码／登记、17文档见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 456：原依据移交到实际启动事务，不等于完整准入完成

[Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)原生私有工厂核同次input8／result5和原typed source18／datum14，实际Registry／Catalog／Vehicle／PRIMARY来源贯穿普通及地面Review、独立Authority、最终Transaction。各层保同一shared ptr及报告指纹，不从导入JSON／QML票据恢复。

原Review QElapsedTimer副本与绝对elapsed截止保持，后续只能缩短；资料until及共享UTC水位不续期。完整sourceCurrent先于原身份／权限链，所有回调后memoryCurrent核全组；实际Vehicle最后write前的既有commandContextCurrent再次消费它。旧Review自然退休不作为新owner的依据，owner fence拒绝与真实源退休分开。

原pending仍阻断带资料任务贯通部署。已定位下一实质缺口：Transition球面走廊并集＋独立高度slab不是已证明包含的三维扫掠管道；普通RAW在早期observer回调后才建startup depth，逐机motion核后尚缺全组最终memory尾栅。完整StartPermit、正式机载结果与其他五类仍须开发。13源码／登记、19文档见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：实际启动报告进入原审阅，最终发送消费仍继续

[独立纯值线程](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)冻结exact8，geometryBasis绑定原计划／归档证据、同次实际几何、审阅UUID与source18／datum14副本，不含最终artifact。Joint新增完整一次提取入口，ClearanceReview抽严格geometryResultMatches支持明确规划／启动范围，旧planning入口仍只核planning报告。只值校验不重solver，不证明结果真值。

[普通](../custom/src/Swarm/FleetMissionStartReview.md)和[地面](../custom/src/Swarm/FleetMissionArmedGroundReview.md)仅在真实来源／期限仍对应时附完整result5后hash。最新采样只能验证，不能替换首次v4基准；不完整、未知、违规或失败不进入可批准状态。原期限计入计算时间，取消／销毁不wait，槽与地面占用等实际finished和原运输排空。必需报告不能被可选诊断挤掉或省略。

当前原Compiler pending仍使带资料任务无法贯通部署，未调整该阶段语义；两Authority／Transaction独立保源与最后write、运动包络包含关系、完整StartPermit和正式机载结果仍需实质开发。11源码／18文档范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：显式工程datum对应进入两类实际启动审阅

[Catalog](../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)严格区分descriptor1原8字段和descriptor2新增datumRelations；关系绑定原scene版本／来源文档／垂直datum、JSON构型指纹及GLOBAL_ORIGIN参考hash，必须明确GPI.alt、同datum和offsetMm=0，不补默认或伪做非零转换。目录整体预算保持，轻量catalog3缓存沿原完全相同fingerprint公式，所有实际提交先更新再通知。

[Datum helper](../custom/src/Swarm/FleetMissionClearanceDatum.md)从原453 Source18取源，核真实当前VERSION、PRIMARY、完整本地构型与已观察原点。普通／地面审阅冻结同一14字段声明对应材料，首capture、后续小证据recapture及最终memory核分开；资料／READ／审批原期限只缩短，动态receipt／age不使稳定依据抖动。必需材料在原artifact hash前进入，不作为可选诊断省略。无资料旧任务不加默认关系。

实质修复：原Catalog严格字段少了Vehicle实际getter的receiptSequence，合法native包会被拒；现接收且核规范正十进制，不纳入稳定版本绑定。跨层UTC核对另补原生下界／最终观察输出，两Review不再只保存较早外Source时刻，timer同样保水位且不续期。PRIMARY读取仅非拥有指针，不临时weak.lock。配套PX4源码只读确认GPI.alt与ALTITUDE AMSL可回退baro，HOME／原点报文没有具体geoid或当前内部valid证明。新工程对应并不改变这些物理证据边界。

同次真实启动异步净空报告、两Authority／Transaction／Transition至最终write、完整StartPermit和正式机载结果仍需开发，原pending保持；可信性能、自愈机载执行、企业跨站、长期运营及真实模型生态继续。准确范围11源码／登记、17文档共28路径见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。原25目标保持，整体约40%仍是源码工作量粗估，不按本批文件数提高验收率。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 下方453为历史。

## 453：当前飞机／本地构型基线／资料期限的原生来源切片

[配置观察](../custom/src/Swarm/FleetNativeConfigurationObservation.md)与Swarm实际发现使用同一提取，独立于原电池来源绑定。[Registry原生证据](../custom/src/Swarm/FleetRegistry.md)要求同实际来源和完整当前本地批准基线，JSON配置指纹与typed指纹分别使用；重复同源观察不刷新binding身份，换源／失效／触及导入退休旧绑定。

[Source](../custom/src/Swarm/FleetMissionClearanceSource.md)核原完整净空输入、原成员顺序、实际Vehicle和场景资料有效窗口；稳定证据不包含当前时间／剩余秒数。[StartReview](../custom/src/Swarm/FleetMissionStartReview.md)与[GroundReview](../custom/src/Swarm/FleetMissionArmedGroundReview.md)加入原生审阅摘要前冻结资料，地面在新READ开始绑定；原单调期限与资料寿命取最小值，重捕／已阅／审批不续期，已观察UTC回退或来源失效保持退休。

本批不证明实测机体／桨／起落架尺寸、未上报改装、硬件身份、真实性能或共同高程参考；系统UTC不是可信时间认证。原净空pending保持，仍须显式datum对应、同次真实启动异步报告、两Authority／Transaction／Transition及最后write消费，以及最终StartPermit／正式结果。15源码／登记、16文档准确范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定核对见[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。本批只优化地面站源码及交接材料；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余不缩减。 整体约40%仍只是源码工作量粗估；下方452为历史。

## 452：规划阶段的接地模型已接通，真实启动准入仍未完成

原统一normalize新增内部schema2，原Draft外层source2保持；工程声明明确protected与support尺寸、完整接触动作／payload／seq、pad与terrain引用及显式容差。[非对称扫掠](../custom/src/Swarm/FleetMissionClearanceGeometry.md)保原精确全段表示，[接触模型](../custom/src/Swarm/FleetMissionContactModel.md)由[Review报告3](../custom/src/Swarm/FleetMissionClearanceReview.md)实际消费，进入原后台Compiler／准确归档来源匹配。

LAND静止参考终点来自独立声明支承面＋显式支承偏移，原wire请求不改，不用编码高度量化或大容差截短到表面之前；起飞保原实际vertical.from并核原TAKEOFF，current-first独立假设不删。两组件全部检查场地／走廊／地形覆盖／障碍，只有指定support∩terrain完整包含于有限contact band时允许接触。

这是声明几何，不是PX4真实停止高度、实际构型／datum／时效或触地认证。仍无同次实际启动报告、最终write统一消费／完整StartPermit或正式结果；原25目标和六类实质剩余保持。9源码／登记、18文档见[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。仅限定源／文本静态核对，未编译、运行、测试、改PX4、刷机、提交、更新PR或打包；下方451为历史。

## 451：后台规划、取消传播与三入口源码接线

[Compiler](../custom/src/Swarm/FleetMissionCompiler.md)提取纯值static入口，沿[Review／Geometry](../custom/src/Swarm/FleetMissionClearanceReview.md)传同一shared atomic取消信号。Worker只持冻结JSON值，不访问Vehicle／账号／Registry；线程实际结束才释放唯一槽，cancel后不堆新工作。

[Coordinator／Root](../custom/src/Swarm/FleetMissionReviewCoordinator.md)核原目录／Vehicle／ParameterManager对象及输入代次后显式完成。Draft、余段、AI三个现有入口均接pending与完成回调；来源A→B→A不复活旧票据，completing退休保实际归档receipt。等待／取消／错误明确显示，不增加审批、上传或飞行命令。

这只补450的异步计算缺口，不完成净空准入：起降接地、实际构型／datum／资料时效、同次startup报告与最终write消费及正式结果继续。旧complete的其他纯计算与原生采集仍GUI，单次JSON／Joint等只边界取消，响应时间未验证。原25目标／六类实质剩余保持，整体约40%仅管理粗估。

15源码／登记、17文档见[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。本机未编译、运行、测试、改PX4、刷机、提交、更新PR或打包；下方450及更早为历史。

## 450：真实规划计算已进入编译输出，尚未开放净空启动准入

[Joint纯源](../custom/src/Swarm/FleetMissionJointSpatialReview.md)复用原完整编码／source映射提取，显式区分规划范围与包含启动的范围；[StartupSpatial独立basis](../custom/src/Swarm/FleetMissionStartupSpatialReview.md)不借最终artifact指纹，避免后续嵌入自引用。[新报告](../custom/src/Swarm/FleetMissionClearanceReview.md)明确scope／startupIncluded，不冒称实际启动已审。

[Compiler](../custom/src/Swarm/FleetMissionCompiler.md)在完整非释放plan成形后实际执行规划环境检查，完整结果进入planningClearanceReview和原contentHash；无法构造或模型不满足再增加具体blocking。[版本库](../custom/src/Swarm/FleetMissionPlanStore.md)严格核新结果的形状与原计划几何／输入一致性，仅复做源提取，不重复2M solver或旧1M空间checker。无报告旧历史兼容，不倒推已检查或复活批准。

原geometry pending继续阻断；真实启动／两Authority／最终write、接地语义、当前构型／场地高程对应与时效尚未完成，不能称完整净空接线或最终StartPermit。原25目标及其余实质源码保持，本批9源码／17文档见[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方449及更早为历史。

本批实际调用目前仍由显式QML准备同步进入GUI线程，未有worker／取消点；这是已确认待继续修的源码粗边，不是只等运行测量。下一步先做纯值单任务异步、取消与回GUI来源代次复核，再推进启动准入，不把Vehicle捕获整段搬后台。

## 449：完整扫掠净空算法与独立报告源码

[几何核心](../custom/src/Swarm/FleetMissionClearanceGeometry.md)用整个box＋segment的支持面，而非端点或AABB近似替代扫掠；精确256位整数比较有理交点。闭区域并集通过有序严格补集差集，覆盖多个相邻区域的共同包含；有界工作／分片／平面超限返回Unknown。

[报告helper](../custom/src/Swarm/FleetMissionClearanceReview.md)消费448完整输入与442联合段，全量核成员／来源／计数及坐标适用域，检查覆盖、允许走廊、地形覆盖／上界和障碍。范围、计算、模型满足分开；显式接地上下文缺失阻止模型满足，不删除起飞／LAND段。报告ok仅为构造成功，全部实飞／许可标志仍false。

尚未接实际StartReview／GroundReview或规划放行；Compiler pending保留，不新增UI或命令。完整同源消费者、接地政策、场地时效／实际构型／高程对应及最终StartPermit、正式结果和其余五类工作继续。原25目标保持，本批5源码／15文档见[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方448及更早为历史。

## 448：净空资料从草稿到准确计划记录已接源码

[新纯值合同](../custom/src/Swarm/FleetMissionClearanceInput.md)接受明确场地版本、来源hash、UTC时间、高程／局部图表、毫米凸区域与地形上界，以及全部活动机的尺寸／配置指纹声明／跟踪误差。整数凸性判断不使用浮点epsilon，不修复或补默认资料；hash和声明均不认证实测真值。

[区域来源版本2](../custom/src/Swarm/FleetMissionDraft.md)与原来源1分离，完整重建constraints并保持原planId／contentHash。[Compiler](../custom/src/Swarm/FleetMissionCompiler.md)拒绝坏型与成员错配；[PlanStore](../custom/src/Swarm/FleetMissionPlanStore.md)独立核规范内容及编辑源／计划同资料。Root两处恢复版本预检同步，不丢原输入或跳过原生整体验证。

仅资料通路，不是净空计算完成。合法新资料也明确 `clearance_geometry_review_pending`，447实际阻断链继续拒绝，下一实质工作是完整名义段扫掠／覆盖／障碍／地形／走廊与接地语义、真实StartReview／GroundReview结果消费。最终StartPermit、正式结果及其它五类代码仍在，原25目标保持。8源码／17文档见[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方447及更早为历史。

## 447：合法业务阻断不再只停留在报告

原评估可能 `inputValid=true` 但存在业务 `blockingReasons`；协调器过去只读前者，报告的blocked标签又不被实际审批消费。本批新增[共享原生检查](../custom/src/Swarm/FleetMissionRiskEvaluator.md)，严格接受真实true及实际空QStringList／QVariantList；缺项、null、坏型或非空均不能成为可审阅状态。严重信号空code同时写明细／原因时采用相同fallback，避免原因列表漏项。

[Deployment](../custom/src/Swarm/FleetMissionDeployment.md)再次检查冻结与归档风险，并用规范JSON保同一内容；[StartReview](../custom/src/Swarm/FleetMissionStartReview.md)／[GroundReview](../custom/src/Swarm/FleetMissionArmedGroundReview.md)在原准确记录上独立消费，覆盖确认和审批重捕。不更换任务来源、不剥离历史原因、不新增命令或许可。

6源码／16文档见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。完整版本化场地、逐机包络与实际启动几何净空仍须开发，最终StartPermit／正式机载结果及其它五类实质代码保持，原25目标继续。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方446及更早为历史。

## 446：v4高度关系判断不再丢失中间残差

静态数值审阅发现：先算Double的AMSL-local再与毫米换算值比较，会把非零残差舍为零；最后一次nextafter不能恢复前面丢失的数值。[新增算术](../custom/src/Swarm/FleetMissionAltitudeArithmetic.md)按原binary64的确切位值做定长整数线性组合，[既有高度helper](../custom/src/Swarm/FleetMissionStartupAltitude.md)的残差、sample漂移和逐包比较统一使用它。配置容差和DTO保持，不加默认安全余量。

原actual scope／最终write门槛继续消费同一helper，不另造显示专用结果或许可。此修正不认证传感真值、内部valid、净空或完整StartPermit。下阶段净空必须补工程资料来源／版本／覆盖、逐机机体及跟踪误差，并让同次完整启动几何的结果进入StartReview／GroundReview实际风险，而非只显示Joint报告；早期Coordinator不具备实际起点不能伪造覆盖完成。正式机载结果及其余原25目标继续。

本批4源码／登记、15文档见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)，仅源码／限定静态核对。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方445及更早为历史。

## 445：显式报文高度关系政策进入实际启动链

[startupTransitionPolicy v4](../custom/src/Swarm/FleetMissionExecutionProfile.md)保留v2的运动／航向观察及固定HOME，增加六个工程师明确填写的[高度差值与参考残差容差](../custom/src/Swarm/FleetMissionStartupAltitude.md)；不套默认阈值或混入v3 HOME更新。原生基准来自实际PRIMARY的ALTITUDE／原点／HOME，不从444可选展示诊断补缺。

首次样本进入完整审阅材料，当前稳定身份不因正常高度／时间／receipt推进变化，但每次仍对原样本检查容差。实际Vehicle启动scope逐包保留越界、坏帧、失源及断档，正常恢复不洗掉历史；最后write前仍核同一scope。连续性只从该scope建立开始，不证明此前等待期、内部valid、共同参考、同采样或净空。原v1–v3及step33保持。

最终StartPermit、可信内部参考／真实净空、正式机载结果，以及真实性能能耗、自愈改派、企业跨站、长期运营和真实模型生态仍有实质代码；原25目标保持。本批仅源码／限定静态审阅，范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，待执行项见[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方444及更早为历史。

## 444：实际ALTITUDE高度诊断进入启动风险材料

[纯解码](../src/Vehicle/FleetStartupAltitude.md)完整保留六个同包float32字段的有限值／NaN／Infinity／协议未知哨兵及rawBits，派生AMSL-local、AMSL-relative和relative-local三个报告差值。[原生账本](../src/Vehicle/VehicleStartupAltitude.md)以实际PRIMARY／版本／flight来源记录raw接收、鲜度、状态类别revision和已观察的time_usec数值倒退；不从Fact、旧样本或其它消息填缺。

[Readiness](../custom/src/Swarm/FleetMissionStartReadiness.md)输出独立冻结诊断，[普通审阅](../custom/src/Swarm/FleetMissionStartReview.md)及[地面风险](../custom/src/Swarm/FleetMissionArmedGroundReview.md)沿现有逐机JSON展示。首次完整artifact及hash保持，当前风险比较仅排除新增诊断；phase fingerprint、三类stableReadiness和异常fallback同样隔离动态值。缺报／坏字段只形成未知诊断，不给旧v1–v3另加门槛，也不赋予许可。

本地PX4的ALTITUDE可以复制旧local样本、用baro回退AMSL或用-local.z回退relative；其timestamp是发送时间。原始terrain字段的本地公式不能直接当AGL净距，差值不能证明HOME/local同参考或内部valid。显式容差／参考政策、净空、最终StartPermit、正式结果及可信性能、自愈、跨站、长期运营、真实模型生态仍有实质代码，原25目标保持。本批11源码／登记、18文档见[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方443及更早为历史。


## 443：被动原点变更接入实际启动依据

[Vehicle账本](../src/Vehicle/VehicleStartupGlobalOrigin.md)从原始GPS_GLOBAL_ORIGIN冻结消息、实际来源及单调参考revision；收包入口先建立整栈隔离，重复相同参考不因年龄／receipt更新换身份。原点／报告timestamp改变、坏帧和来源退休使旧依据失效，A→B→A不复活。此消息非周期性，未设置三秒鲜度门槛或自动请求。

[纯参考helper](../custom/src/Swarm/FleetMissionStartupGlobalOrigin.md)将native24投影为stable18；Readiness／地面新READ冻结／Transition首尾与真实发送scope使用原依据，而非QML可改flag。第一次send scope绑定后不在ARM／MODE步骤重采，最后write前仍核实际来源和原点参考；已发生write事实不被后置失效擦除。

终态新再审上下文也冻结原点四字段，逐机row由9变13；外层20字段及step33不变。新READ开始时核与该原上下文同一参考，期间继续物理投影核对。本批13源码／登记、18文档，见[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。这只是原点声明的连续性，不证明HOME/local/估计器共享原点、PX4内部有效分支、同采样、净空、最终StartPermit或正式结果；ALTITUDE分支证据仍待独立开发。可信性能能耗、自愈改派、企业跨站、长期运营、真实模型生态与原25目标继续。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方442及更早为历史。


## 442：全任务请求链与独立启动参考已接联合名义报告

[联合helper](../custom/src/Swarm/FleetMissionJointSpatialReview.md)消费同一次capture的完整任务投影和启动快照，不重新生成载荷或从当前草稿补几何。canonical请求链含出航／返程连接和LAND下降；启动参考保持独立，逐机缺口、RTL未编码和未知来源均保留。原归档区域及实际阈值用于新纯静态检查，不改写原报告scope。

[明细入口](../custom/src/Swarm/FleetMissionSpatialPanel.md)分别呈现覆盖完整性、计算完成、候选／区域相交计数和截断；缺联合结果不借旧报告补成功。Root同一record／有序整组目标／来源指纹首尾核对，缓存按同capture token发布，Map仅适配九字段信封，不新增自动绘图或命令。

仍无权威时序／实际轨迹／动力学／净空认证、最终StartPermit、正式机载结果；真实性能耗能、自愈改派、企业跨站、长期运营、真实模型生态及原25目标保持。10源码、17文档，见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方441及更早为历史。

## 441：原始空域输入已接实际风险／归档／审批链

[全链合同](../custom/src/Swarm/FleetMissionAirspaceInputGate.md)保留真实缺字段、显式空列表与坏值；原生类型／完整JSON预算、task与参数指纹、无效间距均明确阻断，不保留上一份可批准报告。归档使用原生采集前快照，当前有效来源须与私有Intent匹配；安全坏JSON只能形成阻断诊断，超限／不可表示原值不归档。

[任务状态机](../custom/src/Swarm/FleetIntentTask.md)在请求和批准时检查当前私有输入，输入和工作流先整体提交再通知，私有修订与销毁检查防同步重入覆盖。7源码、15文档的本批范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，工程师待办见[交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仍缺原完整路线／独立启动假设／终结动作的联合空间重审，以及可信原点／净空、最终StartPermit、正式机载结果、真实性能耗能、自愈改派、企业跨站、长期运营与真实模型生态；原25目标保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方440及更早为历史。

## 440：同一原记录的完整任务空间显示已接源码

[完整投影](../custom/src/Swarm/FleetMissionSpatialReview.md)保留全组身份及几何／编码各自错误；原plan只整体调用一次Builder，出航、返程、起飞／HOLD／LAND请求和未编码RTL分别解释。[逐机明细](../custom/src/Swarm/FleetMissionSpatialPanel.md)与地图使用439同一次捕获，不能从最新草稿或当前Vehicle补缺。

[归档空域](../custom/src/Swarm/FleetAirspaceConflictChecker.md)只展示准确record的原报告及zone列表，保留更严格的原实际间距，不重新只按plan约束覆盖。新[输入形状证据](../custom/src/Swarm/FleetMissionPlanStore.md)区分缺失、空列表和坏型；旧记录不倒推已加载或覆盖完整。

仍缺完整路线／起飞参考／终结下降／真实空域的联合重审与权威时间、可信净空和最终StartPermit、正式机载结果。真实性能／能耗、自愈改派、企业跨站、长期运营及真实模型生态保持，原25目标不缩减。本批14源码／资源、18文档，未编译、运行、测试、改PX4、刷机、提交、更新PR或打包。见[范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)；下方439及更早为历史。

## 439：实际启动空间审阅已接源码

[空间捕获／地图入口](../custom/src/Swarm/FleetMissionStartupSpatialPanel.md)消费同一冻结原生风险，保留有序整组目标、名义AMSL垂直段、起飞后到TAKEOFF请求点的连接，以及独立的当前位置到首NAV_WAYPOINT提示。三种提示不拼成已证明的航迹；二维垂直段有位置标记和高度明细。

Root按当前实际Episode／身份／完整binding／review指纹与准确归档版本取源；新捕获不自动换地图，过期历史仍明确标识。[名义接近候选](../custom/src/Swarm/FleetAirspaceConflictChecker.md)使用明确声明间距，保留投影域、未知时间、地形／禁飞区／净空和明细预算。旧checker与原发送门禁保持。

尚未覆盖原任务全部路线／返程／结束动作和真实空域的共同冻结报告，亦未完成最终StartPermit、可信原点／净空、正式结果、真实性能能耗、自愈改派、跨站、长期运营和真实模型生态；原25目标不缩减。本批13源码／资源、17文档，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)；下方438及更早为历史。

## 438：启动策略结构化输入已接源码

[新表单](../custom/src/Swarm/FleetStartupPolicyEditor.md)覆盖原v1／v2／v3完整政策，按版本呈现全部必填字段及原生范围／交叉约束。只在明确采用后更新原规范JSON文本，再走[整份配置保存](../custom/src/Swarm/FleetExecutionProfileEditor.md)；未采用表单保留并阻断旧策略保存，非法JSON／未知字段不静默丢弃。

编辑与回调增加实际草稿选择代次和真实backing对象检查，防长期facade下相同机号／revision误接新草稿；换源保留文字并标失效，隐藏保输入、撤确认与参数依据。没有改变原生容差、policy版本或启动权限。

本批6源码／资源、14文档；未编译、运行、解析、测试、认证、改PX4、刷机、提交、更新PR或打包。最终StartPermit、可信原点／净空、正式结果、可信性能与能耗、自愈改派、跨站、长期运营及真实模型生态仍有实质工作，原25目标保持。见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)；下方437及更早为历史。

## 437：实际启动和地面续审工作台已接源码

[工作台](../custom/src/Swarm/FleetMissionStartupWorkbench.md)以当前outbound Episode统一投影与确认，不采用历史Run选中源；接通原内容读取、风险已阅、独立账号审批、认证移交、初次三步事务，以及最新actual owner的新地面复核链。操作员每次明确选择一步，真实目标、阶段和逐机已queue／观察／可能提交分别显示；取消不抹已发生事实。

[地面风险](../custom/src/Swarm/FleetMissionArmedGroundReview.md)新增有界只读展示，保留当前风险与未证明项，隐藏认证／原载荷。此批补“后端已有、启动UI缺失”，不是完整启动安全认证。最终StartPermit、可信HOME／原点／净空、正式结果、真实性能能耗、实际自愈改派、跨站、长期运营及真实模型／生态继续，原25目标不缩减。

本批未编译、测试、运行UI／认证／应用／设备、改PX4、刷机、提交、更新PR或打包。见[精确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)；下方436及更早为历史。

## 436：实际Run简报交接操作链已接源码

[交接工作流](../custom/src/Swarm/FleetMissionHandoffWorkflow.md)将原自由taskId审计入口改为实际选定Run取源、冻结有限简报、明确发送、收件人确认及历史查看。[Manager](../custom/src/Swarm/MissionHandoffManager.md)按四类筛选、每页4条摘要；界面显示真实账户SID，不把姓名当身份。

新简报包含实际观察任务UUID、Run／部署／事务身份、准确7/9字段记录绑定及逐机历史摘要。原生会话和用途角色快照均复核；发送候选一次消费，已提交事实和后续上下文有效性分开。导入历史只读，不恢复本实例来源。仅本地接收回执，不改变机载任务、成员占用、批准或控制租约。

本批推进目标15，但跨站同步、企业账号、真实控制权交接与中途加入仍未完成；完整受控执行、可信性能、自愈、长期运营和真实模型／生态继续，原25目标不缩减。未编译、测试、运行helper／UI／认证／应用／设备，未改PX4、刷机、提交、更新PR或打包。见[精确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[待验证事项](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)；下方435及更早为历史。

## 435：余段新记录地图与逐点核对已接源码

[同一计划的显示投影](../custom/src/Swarm/FleetMissionRemainderView.md)缓存到新审阅上下文，供地图和[分页核对区](../custom/src/Swarm/FleetMissionRemainderRoutePanel.md)共用。地图明确切换已采用来源、隐藏旧区域提案层；过期／无效不自动回退。新增只读地图核对入口，保持规划隔离和编辑文本，不发送飞行指令。

规划点与协议项分别展示，速度／起飞／结束动作不冒充路线点；原编码失败、未知数值及E7／float32量化保留。此批补434地图／逐点入口缺口，仍不完成净空、最终StartPermit、机载接替或正式结果。完整受控执行、可信性能能耗、自愈、跨站、长期运营及真实模型生态继续，原25目标不缩减。

未编译、测试、运行helper／UI／应用／设备、改PX4、刷机、提交、更新PR或打包。[精确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师待办](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步；下方434地图缺口及更早为历史。

## 434：余段新记录的批准／地面部署操作链已接源码

433只生成新审阅记录，原区域草稿专用按钮无法消费该记录；本批新增[独立审阅来源](../custom/src/Swarm/FleetMissionRemainderPanel.md)，明确结束编辑后复用原意图审批与唯一地面上传面板。余段目标来自自身plan，不借用区域草稿成员；其记录过期后仍显示所选来源，需明确退出或重新审阅。

下一可见缺口：FlyViewMap当前仍只叠加区域草稿预览；独立余段路线的地图展示与新路径逐点审阅尚未接入，旧区域图不能作为新余段载荷的核对依据。

本批是目标6／13正向操作链的源码增量，不是受控启动、正式机载接替或任务完成认证。最终StartPermit、可信原点／净空、真实能力能耗、自愈执行、跨站、长期运营和真实模型生态仍未完成；原25目标保持。未编译、测试、执行应用／helper／设备、改PX4、刷机、提交、更新PR或打包。范围和待办见[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)；下方433及更早为历史。

## 433：实际Run余段可形成独立审阅提案，尚无机载接替

新增[原生取源](../custom/src/Swarm/FleetMissionRemainderSource.md)、[单成员余段规划](../custom/src/Swarm/FleetMissionRemainderPlanner.md)及[操作面板](../custom/src/Swarm/FleetMissionRemainderPanel.md)。冻结原输入政策与载荷映射；操作员选原航点索引及目标机自己的端点/速度，完整保留所选点和后缀停留。原CURRENT/REACHED仅是历史参考，不推导连续完成前缀。

提案进入原编译/当前证据审阅/新归档记录；不是新执行UUID，不恢复旧审批，不自动上传、起飞或修改原Run/其它成员。剩余工作仍包括实际接替事务、可信性能与耗能、完整受控执行、跨站、长期运营和真实模型生态。原25目标保持，未因本轮增加整项验收数。

仅源码与限定静态核对，未编译、测试、执行helper/应用/设备或放行；没有更新PR或包。下方432及更早为历史。

## 432：备用待命／进场／完整路线／返程预算已接源码

[备用策略与几何](../custom/src/Swarm/FleetReserveEnergyPlan.md)从原草稿进入编译计划、[协调器](../custom/src/Swarm/FleetMissionReviewCoordinator.md)和原能量账单，[现有规划页](../custom/src/Swarm/FleetReserveEnergyPlanPanel.md)提供明确编辑／取消／清除及自身端点入口。有效主机账单不会因合法未知备用而消失；缺预算备用仍受编译和风险阻断，关闭能量证据要求也不能使其合格。

估算只覆盖显式待命系数、自身进场、目标完整航点重做及自身返程；没有中断点续接、自动接替、目标起降／终态动作复制或真实耗能标定。非覆盖任务的明确共享路线只用于预算专用副本，不改变实际上传条件。原电池来源／鲜度及审批扣龄核对保持。

本批11源码／18文档；仅静态阅读及文本检查，不代表整个目标9或25目标完成。完整受控执行、可信性能、实际自愈、跨站协作、长期运营和真实模型／生态继续。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方旧备用最长预算由本批替代，其他历史边界仍需按当前实现判断。

## 431：原生电池估算与时效进入实际预算链

[Vehicle](../custom/src/Swarm/FleetNativeBatteryObservation.md)在原始PRIMARY收包路径保存独立电池观察；[Swarm桥接](../custom/src/Swarm/FleetNativeBatteryBridge.md)只绑定实际对象，[Registry](../custom/src/Swarm/FleetRegistry.md)在快照读取时投影固定3秒接收鲜度。唯一明确推进电池可供预算，辅助电池不参与平均；多推进、用途未知、过期或失源不借历史值。

Scheduler保留原主／备用身份，并将来源带到逐机账单；Guided的稳定审阅身份包含来源与报告内容，批准检查另核扣龄后时间不小于原预算。规划页解释报告估算、来源和年龄，不将其标成实测续航或性能认证。

本批推进目标5／9及审阅消费，不完成真实能量模型、备用待命／接替模型或整个目标9。完整受控执行、可信性能、自愈、跨站、长期运营和真实模型／生态六类仍有实质源码工作。仅源码及限定静态阅读，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方430“无新增实际电池遥测”为历史状态，由本批有限接入补充。

## 430：逐机能量账单接线，真实电池模型仍缺

[协调器](../custom/src/Swarm/FleetMissionReviewCoordinator.md)按实际编译成员推导独立预算，[调度器](../custom/src/Swarm/FleetEnergyScheduler.md)固定原主／备用身份，[风险评估](../custom/src/Swarm/FleetMissionRiskEvaluator.md)检查账单与实际计划一致。原成员拒绝仍展示，不以备用排序替换主机。规划页增加[分页只读明细](../custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)。数值 -1 未知口径和满电续航百分比公式同步修正。

推进目标8／9／11的实际消费链，不代表完成目标9或25项目标。共同时间下限保留；备用仍用最坏主机预算，不含真实待命与接替路线。实际剩余续航遥测、耗能标定、风／载荷／老化模型仍待接入。完整受控执行、可信性能、自愈、跨站、长期运营、真实模型／生态六类仍有实质源码工作。未编译、测试、运行或放行，未改PX4、提交、更新PR或打包。

## 429：运行报告接线，不代表机载结果闭环完成

已有飞前计划报告与实际Run观察之间增加[独立报告服务](../custom/src/Swarm/FleetMissionExecutionReport.md)及[显式界面](../custom/src/Swarm/FleetMissionExecutionReportPanel.md)。报告记录采集时的逐机模式队列、序号观察、历史候选和未知状态；冻结后不随当前选机或遥测更新。用户可以显式保存JSON，或不连接飞机、不登录飞行账号地打开历史报告。

摘要与原始有界观察一起保存，导入重算摘要并核对内容哈希；这些检查不证明文件可信或任务成功。外部文件永远不成为当前Run。原25目标中本批推进目标19，也支持目标6的诊断交接；完整受控执行／正式机载结果、可信性能、自愈、企业跨站、长期运营及真实模型／生态仍有实质源码工作。未编译、测试、运行或放行，未更新PR或打包。

## 428：历史归档规划恢复为独立草稿（源码阶段）

[本地版本库](../custom/src/Swarm/FleetMissionPlanStore.md)增加指定提案会话的全部历史版本摘要与精确记录取源；[历史规划入口](../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)用于在现有启动加载的档案中选择具体版本、预览来源并明确创建可编辑副本。不以“最新版本”替代已选版本，不依赖原失败部署或426对账；旧记录没有完整编辑源时明确不可恢复。

恢复仅复制规划内容，重置地图基准确认，并复用427的独立草稿、上一份备份及替换确认。历史sourceTaskId只作来源；重新审阅使用当前提案会话身份及当前设备证据，不恢复旧审批、机载状态、任务执行或控制权。档案只读恢复不解除写入隔离，后续归档仍可能因冲突或容量被拒绝。

本批接线与待验证边界见[修改清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／应用／设备，未修改PX4、刷机、提交、更新PR或打包。只覆盖已加载本地有效档案的规划编辑恢复，不是损坏档案修复、通用任务编辑或机载任务续飞；原25目标及六类实质工作继续。下方427及更早为历史增量。

## 427：保留完整原草稿并建立人工修复副本（源码阶段）

[区域草稿](../custom/src/Swarm/FleetMissionDraft.md)现在随编译计划保存有界完整编辑源，不再尝试从生成航线反推原多边形；[编译器](../custom/src/Swarm/FleetMissionCompiler.md)核对源与实际规划字段一致，[版本库](../custom/src/Swarm/FleetMissionPlanStore.md)把源纳入原内容摘要和历史存档。旧记录缺源时明确不可恢复，通用任务不会被伪装为区域编辑器草稿。

[失败部署取源](../custom/src/Swarm/FleetMissionRepairDraft.md)基于原失败／取消部署与已排空的426核对结果，保留different／unknown，不将其称为已修复。新[操作入口](../custom/src/Swarm/FleetMissionRepairDraftPanel.md)只创建独立本地编辑副本，保留上一份草稿供明确切回；新副本重新确认WGS84／AMSL基准，再走原新预览、原生审阅、新记录版本及人工审批。不是新taskId，不恢复旧审批，不自动上传、清空、回滚或启动。

本批范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定源码核对见[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)，待实际验证事项见[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／应用／设备，未修改PX4、刷机、提交、更新PR或打包。跨重启历史草稿恢复入口、逐项修复建议及完整受控执行、可信性能、自愈、跨站、长期运营、真实模型／生态仍待开发；原25目标保持，以下426及更早是历史增量。

## 426：失败／取消后的独立整组只读核对（源码阶段）

[独立核对工作流](../custom/src/Swarm/FleetMissionReconciliation.md)从本实例原失败／取消部署冻结期望任务，由当前有权操作员明确发起；不要求旧回读全部匹配或旧批准仍有效。[传输](../custom/src/Swarm/FleetMissionReconciliationTransport.md)使用独立RAW INT读取和一次性结果，[界面](../custom/src/Swarm/FleetMissionReconciliationPanel.md)逐机区分相同、不同、未知与尚待处理。

实际成功COUNT0与协议失败分开；诊断不覆盖普通任务显示缓存、不生成启动收据、不自动修复／清空／上传或启动。全组逐机读取不是原子快照；相同仅是该次受支持规范化内容的比较，未知不能当相同。不支持的机型／协议、失效来源和无法规范化内容保留限制，不能据此声称所有异构机已支持。

本批源码／文档范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，静态审阅与待执行项目分别见[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／UI／认证／应用／设备，未改PX4、刷机、提交、更新PR或打包。修复草稿及完整受控执行、可信性能、自愈、跨站、长期运营、真实模型／生态仍待开发；原25目标保持，以下425及更早为历史增量。

## 425：普通任务回读差异诊断（源码阶段）

普通任务部署增加逐航点回读差异诊断：在既有回读结果中保留有限字段差异和来源操作标识，失败／取消后不再只剩通用失败码。 [比较合同](../custom/src/Swarm/FleetMissionReadbackDiff.md)、[实际部署接线](../custom/src/Swarm/FleetMissionDeployment.md)与[界面说明](../custom/src/Swarm/FleetMissionReadbackDiagnosticsPanel.md)分别说明数值、来源和展示边界。

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

## 418：本地能力声明审批已接源码，尚未运行验收

[工作流](../custom/src/Swarm/FleetCapabilityReview.md)连接原 Swarm/Registry 和专用账号审批；[界面](../custom/src/Swarm/FleetCapabilityReviewPanel.md)在指挥中心与地图规划页均有入口。approve 完整替换本地能力采纳，revoke 明确撤销原批准；不会顺便批准构型基线。原6/14证据合同不扩字段。

普通收包/健康更新不要求重审；静态构型、发现依据、原条目/账号/政策或scope改变则拒旧审批。完整配置只作Qt JSON展示，原类型map在原生私有保存并核对。真实提交、当前仍有效、独立账号依据和本地审计追加分开报告，不能按ok=false推断回滚。

本批16源码/资源、17文档见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。未编译、测试、运行解析/helper/UI/认证/应用/SQL/设备、修改PX4、刷机、更新PR或打包。原25目标/六类仍有实质源码缺口：完整受控执行、可信能力/性能、完整自愈、企业跨站、长期运营、真实模型/生态。下方旧“无能力审批界面”由本节限定路径覆盖，不代表全部交付。

## 417：本地工程目录的运行中操作链已写入，未运行验收

现有[目录](../custom/src/Swarm/FleetFirmwareAdapterCatalog.h)增加不改变状态的完整预览与带最终围栏的 native 导入；[重载工作流](../custom/src/Swarm/FleetFirmwareCatalogReload.h)接实际 Coordinator，Guided 私有原来源及[面板](../custom/res/Merivus/FleetFirmwareCatalogPanel.qml)接指挥中心和 FlyView 地图区域规划页。旧/新完整声明、原工程审阅者及材料摘要、增删改 ID/数量、原始文件与规范内容哈希均保留，不以当前账号伪造档案 reviewer。

固定 AppData `fleet-os/firmware-adapters-v1.json` 只读，最多1 MiB/64声明；维护/管理员申请与不同合格账号的一次审批分离。原生 epoch/revision/CAS、实际 current scope/源、30秒及原身份 TTL、最后文件复读和纯提交围栏共同约束整批内存替换；不写原文件、不刷固件、不授飞行许可。旧任务 review 失效，不停止已有 Run或释放占用；commit、后置来源失败和审计追加分别报告。原启动自动 load 仍保留，不能据此宣称全入口强制两账号。

另外，现有全球任务的启动估计器要求 ABS 水平与 ABS 垂直、且非 CONST_POS；不改 RAW、通用健康、室内或 GNSS 硬件能力。本阶段19个源码/资源路径和累计依赖见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定交叉阅读见[审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。未编译、测试、运行解析/helper/UI/认证/应用/SQL/设备，未修改 PX4、刷机、更新 PR 或打包。

原25目标/六类实质工作继续：完整受控执行/StartPermit、可信能力与性能、完整自愈、企业跨站、长期运营、真实模型/生态仍非完成；可信 HOME/local、净空、正式机载实例/结果不能用本次目录入口替代。无可信整体完成百分比或日历工期。以下416及更早为历史，旧“目录入口未开发”由本节的显式 reload 部分覆盖，不覆盖其他缺口。

## 416：逐机执行配置源码链已接，未运行验收

[草稿](../custom/src/Swarm/FleetMissionDraft.md)已支持每个保留成员的完整规范化 `executionProfile`，精确 revision CAS，不做局部字段合并或别机回退。换角色保留配置；reserve 不进入活动任务 assignments；成员删除再加入不继承。通知之后 false 不能证明本地提交未发生，界面必须读回而非自动重试。

[参数观察](../custom/src/Swarm/FleetMissionReviewCoordinator.md)复用实际 Vehicle/PM 的12项原生观察：同一 MVM/model/Vehicle/PM/VLM/PRIMARY 及共享所有者前后核，30秒内每行 usable，双读稳定身份一致，全部来源回调后再做PM纯内存整组尾核，才返回可绑定 `parameterProfileHash` / `observationFingerprint`。缺 `requiredProfileHash` 或固件工程目录不匹配不造成取哈希的循环前置；成功也不是原子参数事务、硬件认证或飞行许可。Guided 保存前再次读取并精确匹配指纹和哈希，改草稿先使旧 review 失效。

Planner 已接新逐机编辑器：核心字段有结构化输入，完整 startupTransitionPolicy 仍是高级 JSON schema1–3；可不填并保存规划意图，但原启动链继续拒绝缺失政策。本阶段11个源码/资源及15份文档范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定静态结果已记录在本批审计，不是运行验收。未编译、测试、运行 helper/UI/认证/应用/设备，未改 PX4、刷机、更新 PR 或打包。

完整25目标及六类实质工作继续：受控执行、可信能力/性能、完整自愈、企业跨站、长期运营、真实模型/生态。完整 StartPermit、固件目录操作入口、可信 HOME/local 与净空、正式机载实例/结果不能按本批编辑入口宣布完成。下方旧批次为历史，旧“draft 不带 profile”的状态不覆盖本节。

## 411–415：恢复复核源码入口已接，未运行验收

[专用面板合同](../custom/src/Swarm/FleetOccupancyRecoveryPanel.md)对应的源码已接指挥中心和FlyView地图内区域任务规划页；使用同一个Guided私有实际来源，不新增Swarm或可替换原生对象别名。当前申请/独立审批/精确取消、默认取消与两次人工确认均已写入源码。

实际Root模式在账号核验及来源读取前后复核原scope、输入代次和原审阅；最终原生提交尾栅仍无QML/OS回调。作用域清理、重认证前、生命周期来源变化和UI关闭只退役原审阅。初始空申请/审批显示actor仅在固定27键Root显示层规范为空，核心证据不降级。日志false/抛异常另报auditRecordConfirmed/auditRecordOutcome，不抹已发生的本地解除。

本阶段9源码/资源、18文档，共27路径。Root的uiWiringPresent=true仅表示源码有接线，runtimeValidated=false；原生recoveryUiIntegrated保持false。当地解除事实、新实际占用身份提交和当前可用性分别显示；当前真实unknown仍不允许本路径解除。未编译、测试、运行helper/认证/应用/UI/SQL/仿真、设备、刷机、修改PX4、更新PR或打包。

完整25目标/六类仍在开发：受控执行、可信能力/性能、完整自愈、企业跨站、长期运营、真实模型/生态。下一源码切点仍需完善完整执行许可与正式任务/结果归属，不能把恢复面板接线当作全部目标完成。下方406–410无UI为历史。

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

本轮限定静态收尾已完成：9处事务创建调用均补原self/目标/事务检查，直接编队通知/停止及临时任务清理链不沿新会话或新record继续旧操作；Land/RTL单目标失效不挡住其它原有效目标。底层事务通知不复活已取消/新attempt；清理批首冻结全部未处理记录，上传op0失败通知/pause后再核原记录。仅限定读回与文本清点，结果详见静态审计，不是全部旧异步路径或全部产品目标完成。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](../custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](../custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

下一P0源码切点：把新增EST原包来源/语义revision/真实年龄明确纳入实际启动前冻结与逐阶段连续性契约，处理真实中途缺流恢复，而非仅看注册表最后一拍。原SYS/EST age已经由当前门禁实际消费，但这不等同于全阶段journal已具备该通道；正式结果仍需真正机载实例/关联结果来源，不能把本批历史候选改名为认证成功。其余六类完整路线图继续，未缩减为只等工程师验收。

## 371–375：已接下一部署/历史选择局部生命周期，未完成全产品

源码新增两项专用 `ForController` 切换：实际 Root begin 后，只允许私有栈 permit 执行 mutation，再核新 scope 并结束原 owner/ID；普通 active call 和 legacy 切换仍拒绝，未采用“先释放再调用旧接口”。Guided 固定公开 prepare/select 两入口，值投影仅12键 metadata、最多8条6键组记录，不公开 raw refs。独立 JS transitionBusy 与 plain lifetime 覆盖后置处理及 finally，pure scope/来源回调不因此自拒。

指挥中心与规划页已加入同一 [本地部署组面板](../custom/src/Swarm/FleetMissionLifecyclePanel.md)，动作选择后另行确认，默认关闭/取消、非 Enter 提交、固定原 view ticket。新 current 不自动切换 selected；新组不继承登录/批准，不停止旧 Run 或释放它的占用。发布/选择的实际提交事实与原调用 end 分开；失败分配尝试也计入8组容量，allocated=false 不能解释为未计费。

本批实际8源码/19文档；已返回 publication=true 时先进行本地 token 失效及身份意图协调，不让后续 source getter 尾窗抹掉事实。`outboundIntegrationComplete=false` 不变，完整执行闭环、可信能力、自愈执行、企业跨站、长期运营、真实模型/生态六类及25目标仍有实质源码缺口。已完成限定文本审阅，具体数量见本轮静态审计；本机未编译、测试、解析/运行 helper、认证、应用或设备，未更新 PR/包。以下366–370及更早为历史状态，旧入口未实现的描述不覆盖本节。

下一源码阶段仍按P0任务执行闭环推进：最终许可需要的真实HOME/local/净空依据、正式远端执行与结果归属、长任务授权及容量生命周期均不能由本地对象组切换或monitor关闭替代。继续完善这些实现，不把后续工作缩减为仅等工程师验证，也不因本批入口收口而结束完整目标。

## 366–370：当前实际组应用接线，非多部署产品完成

嵌套草稿失效后的 finally 只调用原scope cancel/revoke-only清理；无法核实则私有 `retiredCurrentScope` 继续拒同组 current 入口/回调，selected 监视可读，没有超时复活。5源码是累计工程增量，不是五文件独立安装/构建包；Qt类型往返、parent和模态销毁均未实际验证。

Lifecycle 三个原生 controller-call 接口、两类 QML 注册、Guided 私有工厂/14服务引用和30入口/24来源回调、监视面板 selected token/完整收据路由已写入源码。初始组由构造提供，不自动再 prepare；没有 legacy fallback 或公开 raw QObject。plain lifetime/finally 只清原 owner+ID，pure scope/回调不因忙而自拒；嵌套草稿更改使 active current 调用 stale，已 queue/可能发生或未知事实不作回滚。

selected 的 Series/Run/Close/Session 四个值快照同源，token 包含选择代次；旧审阅取消只走 issued 收据的原 retained Close，显式监视登录不结束 Run。新部署/历史选择的实际产品入口及专用 transition 原生 API 尚未实现，`outboundIntegrationComplete=false`。完整执行闭环、可信能力、自愈执行、企业跨站、长期运营、真实模型/生态六类源码缺口与25目标仍保留。实际5源码/18文档；已完成限定文本审阅，不是运行或安全通过，未编译、测试或运行。下方是历史状态，不以旧未注册说明覆盖本批。

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


## 340–344 当前：新部署原生核心，应用仍单链

已增加 [实际 Episode / Lifecycle 核心](../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)：14个全新 owned 对象、新 signed-out Session、五个 typed 共享来源、全部 retained 组的 native 静默证据及8次分配上限（初始与失败尝试计入）。旧实际对象、Run 与占用不 reset、销毁或自动释放，静默不等于落地/任务完成。

本批只在 `custom.pri` 登记新4文件，**未注册 QML、未替换 Guided、无新部署入口**。现有软件仍走原固定单链。sticky outbound flags/helper 已定义，但 native owned 请求/继续发送门禁、QML 前后 actual Episode/epoch、源回调及历史 Close 身份作用域未完整接线，不能宣称已能多次部署或跨代隔离完成。

下一步仍是源码接线，不是仅等待工程师试飞；原25目标/六类保持不变。限定文本审计结果已记录；未编译、测试、运行 API/helper/认证/应用、仿真或刷机，未更新 PR/包。

## 335–339 当前源码增量：可操作的本地监视审阅界面

[运行监视面板](../custom/src/Swarm/FleetMissionObservationPanel.md)已接指挥中心和Planner独立入口。严格只显示实际Series当前Run，不再回退旧初始Run；查看、生成审阅、三项确认与另一账号审批分步。两个入口只取消自己的controller+review完整token；modal调用ticket保持整段忙锁，不拿滞后的公开busy快照解锁，原30秒不从界面重算。已释放/仍占用/未知分开，不宣称停飞或任务成功。

覆盖330–334无UI的历史状态；本批仍仅源码/限定静态审阅，未编译、测试、启动应用/认证或操作设备，未更新PR/包。完整StartPermit/净空/正式结果、长任务/新部署/容量及其余原25目标/六类仍有实质代码工作，不是只等验证。

## 330–334 当前源码增量（2026-10-15）

[独立关闭本地监视](../custom/src/Swarm/FleetMissionObservationClose.md)已接真实Session/Role/Series当前启动owner与原Run/Swarm/任务模型。原30秒预算覆盖账号提示，三项确认和不同SID专用审批不可复用旧启动依据；消费后只走原生核对及唯一监视退休。18键范围冻结实际map占用成员/撤销状态，旧源丢失不替换同ID对象，释放结果未知保持未知。无新UI按钮、飞行停止或正式完成认证。

这不是全部产品完成或“只等测试”：StartPermit/净空/正式远端结果、生命周期/容量、可信能力与构型、自愈提交、跨站权限交接、完整归档/运营及实际模型等仍有代码缺口。本批仅源码/限定静态审阅，未编译、测试、运行认证/应用/helper、设备/刷机或更新PR/包；以下325–329及更早为历史记录。

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

2026-10-14 当前244–249：实际三步sender/32字段RAW依据与持续风险历史、补充LOCAL/HOME更新风险、原profile显式政策及Transaction/typed阶段模型已写入源码，controller后端和内部工程注册已接。本批有限源码静态收口已完成，未编译/运行/测试或调用接口，无Start/ARM按钮。合法HOME重置再审、local frame/运动/转向、净空/最终StartPermit、执行完成和其它六类目标仍待开发，不能把严格停止称完整飞行功能。以下为历史批次状态。

> 本页开头的 FTC 验证记录是 2026-09-17 的历史快照，不是当前工作树的构建或
> 放行证明。当前 Fleet OS/集群地面站源码状态以
> [`MERIVUS_FLEET_OS_ROADMAP.md`](MERIVUS_FLEET_OS_ROADMAP.md)、
> [`MERIVUS_SWARM_TARGET_MATRIX.md`](MERIVUS_SWARM_TARGET_MATRIX.md) 和工程师交接文档为准。

历史快照日期：2026-09-17；对应分支 `codex/ftc-full-validation`，收口前源码提交 `e7444f32b2f81c699e8b881a1262ca8f1945f5ee`。

当前工作树的 Fleet OS 改动尚未在本机重新完成 Qt/MSVC 编译、Mock/SITL、硬件测试或刷机放行；工程师必须按交接文档重新验证。

2026-10-14 启动依据一次接管（239–243）：新增实际StartupAuthority冻结十来源/全记录/风险/完整RAW及BeforeArm候选，真实账号依据从条件账本一次移交到固定owner/startupUUID/sourceHash，原最早Review/READ START与双方身份期限只缩短不续期；旧Review自然失效不无条件清新owner依据，新owner仍严格核真实源/身份/WTS/原TTL。typed阶段候选不删manual-arm阻断，不称遥控中立/净空/机载canArm；passive retained proof不放宽原grounded读工厂。controller内部申请/匹配撤销接线但未加UI/发指令；当前preARM，真实ARM仍失效，三个飞行许可/执行标志false。最终StartPermit/受控命令过渡/完成与原六类源码目标继续，未编译/运行/测试或更新PR/安装包。下方为历史批次。

2026-10-14 新鲜整组机载任务核对（234–238）：原生 caller scope/mandatory发送fence、原始PRIMARY完整INT读取及34字段成功依据，新StartContentCheck实际来源/全记录/整组规范载荷比对与120秒采集/最早READ START30秒时效，已接权限/controller/默认未确认面板及九参数StartReview。新读前撤销旧live staging和审批，不续期或恢复批准；已匹配但过期不供新审阅，顺序读取不冒充原子快照/硬件认证/无线归属。所有许可/执行false，无Start/ARM按钮，完整受控启动/完成及其它六类工作继续；未编译/运行/测试、修改飞控或更新PR/安装包。下方为历史增量。

2026-10-14 条件启动意图的独立账号审批（229–233）：新增FleetMissionStartApproval、Session专用一次认证依据及StartReview原生已阅上下文，绑定全记录、实际来源/目标、不同SID、不可变风险与原30秒截止。RAW v1保留，风险v2用独立单调语义revision区分纯收包/GP输送时间与真正风险变化；health年龄通知不盲撤销，但门槛/Leader/来源和物理ABA仍失效。所有审批/消费仍不是StartPermit，不移除未解锁/未知或原artifact第二审批缺失字段，不发ARM/模式/启动。Controller后端接线但无启动按钮/实际消费入口，完整受控启动/鲜机载内容/完成判定及其它六类源码工作继续；只做源码静态审阅，未编译/运行/认证/测试、修改飞控或更新PR/安装包。以下是历史增量。

2026-10-14 受控只读证据刷新（225–228）：新 FleetMissionEvidenceRefresh 与实际 Vehicle/PM 读取 API、私有 controller 和现有面板已接线。明确当前账号/角色确认，先撤销旧审阅与上传批准，再逐机读取原始版本及固定十二项；最多32目标、120秒总期限/4秒阶段/100毫秒间隔，不写参数/上传/解锁或启动。新 RAW 计数大于基线与实际 PRIMARY 来源分开核验，没有无线请求因果或身份认证；首次版本清旧状态后的缺失等待有界，已知飞行/解锁或来源异常立即拒绝。完成不恢复批准，服务120秒年龄不是审阅30秒参数寿命承诺。只做源码与静态审阅，未调用实际接口/运行/编译/测试、修改飞控或更新PR/安装包；完整启动/完成与其它六类工作仍待开发。以下记录属于历史增量。

2026-10-14 有限上传租约与桌面撤权（221–224）：短期审批申请60秒与实际消费后最多4分钟的本次部署上传/回读租约分开，仍不越过原操作员会话5分钟期限；schema2 receipt/grant冻结明确租约政策与deploymentId，验证不续期、不同clock只取duration作保守期限。实际Windows WTS工作站来源加入认证/每步gate，未知/旧系统/注册错误拒绝，锁定/注销/断连撤销。新lease不是保持60秒审批“仍新鲜”的伪造认证，也不是跨站/任务飞行许可。源码与UI说明已同步，完整独立启动/完成等六类任务仍未完成；未编译/运行/系统API调用/测试、修改飞控或更新PR/安装包。下面是历史批次。

2026-10-14 本地认证与独立上传审批（217–220）：新增实际 Windows 凭据/SID 和固定本地角色组来源，原生会话与独立授权账号、任务绑定的一次性审批接入私有 controller/上传 gate/UI；没有预置账号、创建角色组或改系统权限。默认未配置拒绝，非 Windows 不支持；账号不同不证明人员不同，本地认证不等于企业/跨站控制权。上传续作不续期，过期/撤销排空不回滚，无启动/解锁许可。完整执行/组件可信度/自愈/跨站/长期运营/模型生态仍待源码开发；未编译/运行/登录/测试、修改飞控、真实上传或更新 PR/安装包。下方是历史批次。

2026-10-14 操作入口与独立风险审阅（212–216）：新增地面上传二次确认/逐机传输/取消排空/只读检查面板；实际members项数、计划版本、目标、当前actor/策略epoch及revision冻结，原生每步gate也核实际来源，未知/退出不继承旧高权限、混型bool拒绝。历史全内容回读只供新StartReview的本runtime来源，30秒风险已阅不复活旧批准/授予启动；ARM后不能靠改verified让入口可用。默认observer/空身份、缺独立额外批准明确禁用，账号/审批、真实启动/完成仍待源码开发及工程接入。未编译/运行/渲染/测试、修改飞控或更新PR/安装包；下方为历史批次。

2026-10-14 飞行状态与启动前置证据增量（208–211）：真实六通道raw flight ledger保留来源/单调时效/epoch/hash/receipt；新增StartReadiness推导实际固件起飞高度规则、当前位置垂直段及后续连接，ProgressObservation独立核缓存wire并映射原始序号。Deployment主动只读检查冻结本实例计划/实际对象，facade只给1秒显示快照和有界审计摘要；未解锁/未知证据不显示前置条件已满足。MISSION_START会尝试解锁，current/reached没有计划身份，HOLD无限且LAND高度非地面/完成认证，均按源码边界保留。仍无启动许可/执行归属/完成判定、上传/启动按钮；未编译/运行/测试或更新PR/安装包，不修改飞控。

2026-10-14 真实版本与工程声明增量（204–207）：Vehicle 新增原始 AUTOPILOT_VERSION 观测/单调年龄/源 epoch/revision 与 callback-depth 保护，完整十二字段 canonical hash 不由缓存/默认能力生成；初次/版本/UID/来源变化同步撤销旧参数。新增有界原生 FleetFirmwareAdapterCatalog，按显式 Git5/Vendor3 布局及精确 tuple 对应工程声明，不能把16hex显示当纯Git或把十位前缀当完整构建证明。原生审阅、目录身份、版本归档与上传最终门禁已接线，黑匣子只记录有界来源摘要和归档引用。所有认证/启动/执行许可 false；组件元数据/实际二进制认证、真实起降/完成、独立受控启动/UI 等仍待开发。未编译/运行/测试、更新 PR 或安装包；下文是历史增量。

2026-10-14 原生来源与版本门禁增量（199–203）：ParameterManager 新增固定十二项参数的只读观测账本，Vehicle 保留实际接收链路；仅真实非回放 PARAM_VALUE 作为观测，缓存/FTP/文件、本地写/刷新、连接及主次链路变化使旧来源失效。新增 FleetMissionVehicleEvidence，将完整参数内容哈希与来源连续性指纹分开；固件/UID/能力位明确仅 native-cache，未知不当认证。原生审阅与编码摘要进入归档证据身份，审批与上传继续步骤重捕获实际源；Fact 同步通知补生命周期保护。仅源码/静态收口，没有参数请求/写入或新增飞行启动，所有部署仍不可执行；可信固件适配、真实起降/完成、独立启动许可、UI 和其它产品缺口仍待开发。未编译/运行/测试、更新 PR 或安装包；下文为历史增量。

2026-10-14 执行语义增量（195–198）：新增严格执行 profile，编译/覆盖/归档/身份保留默认及整份成员覆盖、显式驻留和原策略；载荷 v2 编码速度、起飞请求、驻留及 HOLD/LAND 请求，按五类命令完成 INT/float32 内容核对和来源映射。LAND 保留返航进近高度，显式较低地面目标另由 LAND 表达，并阻断编码高度重合；驻留纳入出航/返航最低预算，明显短于驻留的明确时序阻断，不将估算补作到达时刻。Deployment 按 command 处理 yaw/速度保留零，回读范围改为完整 Mission 项，终态仅保留有界摘要。所有状态仍 `executable=false`/`executionVerified=false`；没有上传 UI、完整启动许可、绝对时序、机载垂直参数认证、起降净空或实际任务完成证明。未编译/运行/测试，也未更新 PR 或安装包；下文为历史增量。

2026-10-14 静态收口续作（193–194）：上传航点接管增加 QObject parent 与 direct callback 生命周期核对；修复新协议 guard 使原恢复任务同步启动被拒绝的衔接，MissionController 以 100 ms/120 s 有界原生续作等调用栈退出，ready/fail 关联原请求、实际 Vehicle/MissionManager、操作和内容。确认还冻结原 source 内容/manager epoch，生成前核对，失败重试不改用新源任务。MERIVUS 不沿用无身份 ready，不以原任务索引/数量要求恢复生成的新列表；最终锁定新任务指纹并再次核对原上传证据。有效上传期间抑制重载引发的自动 Start 弹窗，主动操作仍可取消旧请求。没有扩大自动启动/解锁权限或把 Fleet 几何暂存变成可执行任务；完整执行闭环与其它产品缺口仍在路线图中，未编译/运行/测试。

2026-10-14 最新传输增量（187–192）：新增逐机几何载荷及地面上传/独立回读服务，接入完整归档/审批身份、一次性确认、实时健康/忙机及共享 Swarm 占用；取消和安全抢占保留未结束传输占用，不清空/回滚。几何暂存按系统 ID 阻断原任务启动，重连/被动回读/resume 不解除；PlanManager/MissionManager 调用栈保护、不可变本地 ACK 来源和延迟后续步骤防止直接信号重入替换缓存。覆盖计划保存原高度/巡航/爬升，wire 编码与量化差值独立保留；时序/速度/终止等语义未完整编码，因此 `verified` 只表示程序观察到几何回读一致，所有状态仍 `executable=false`。本轮只改代码/静态审阅，未编译/运行/测试，没有上传/启动按钮、真实设备操作、完整受控启动或 25 项目标完成证明。下方是历史批次状态。

2026-10-13 最新地图入口增量（183–186）：新增 `FleetMissionDraft` 和 `FleetMissionPlannerPanel.qml`，接入指挥中心、地图、资源与内部类型。区域可地图右键/键盘选点，固定成员并编辑五类角色、AMSL 参数、覆盖机端点及辅助机独立出航/返回；只绘制当前审阅计划，不使用另行自动匹配建议。未提交/拒绝输入失效旧审阅，审批校验显示 revision/recordId。规划模式与旧指令/Shift 回调按交互 epoch 隔离，暂停自动移图；没有任务上传或飞行释放。32 成员是草稿容量而非现有六机协议突破。仅静态源审阅，未编译/启动/渲染/测试；完整执行闭环、动力学/感知净空、跨站权限和长期存储仍待开发。此前下方段落是对应历史批次的状态，不代表当前地图入口仍未写。

2026-10-13 最新增量（179–182）：覆盖任务可显式提供每机起点/返回点，生成 3D 进场/返回提案；完整路线高度上限、返航能量估算及航段视图已接线。新增 `FleetMissionPlanStore` 保存不可变计划/审阅版本，默认 AppData 原子持久化并使用本地文件锁和磁盘基线 CAS，损坏档案隔离；恢复不恢复批准。批准请求/结果/拒绝/取消关联精确版本，输入/审阅世代及完整绑定防信号重入和恢复 ABA，交接仅附同 source 的历史引用。仍未编译/运行/测试；没有地图绘图、自动上传/启动、感知净空、跨站控制租约或完整长期数据库。

2026-10-13 区域任务增量：新增 `FleetAreaCoveragePlanner` 生成有界的简单凹/凸区域覆盖提案，按机分配独立扫描航段并连接区内断段；编译器保留逐机航点时间、基准、返航和时长，完整逐机路线不再要求一条全局路线。审阅/能量层接入距离与匀速时长估算，空域不将估算升级为权威 4D 时序，未知时序仍检查时间受限禁飞区；覆盖审阅事件保存有界摘要和计划 ID，避免完整路线重复树超过黑匣子限制；任务安全间距与空域基准/资源门禁同步收口。对应修改清单第 174–178 项，纯数据调用路径已接线但未运行/编译/测试；尚无地图绘图、真实感知覆盖、进场/返航生成、上传或执行闭环，也不代表 ADR-0001 植保功能完成。

2026-10-13 本轮续作：补齐任务记录资源上限、能力资源门禁的实际早退出与 Registry 累积 metadata 原子校验；新增本地活动任务成员排他占用、冲突诊断和忙机摘要，编队先取得占用再发送；连续性规划排除其它活动任务成员，建议绑定占用指纹，应用前重审全任务集合并拒绝无占用证据的单任务建议；修复 Goto 抢占/旧 Pause 等待与同步 ABORT 终态覆盖。对应修改清单第 169–173 项；仍是待构建/验证的源码，不代表全部 25 项产品目标已完成，也不等于跨站锁、机载执行授权或多机原子命令。

截至 2026-10-13 的增量静态审计还收紧了 Vehicle 关闭/迟到 ACK 生命周期、风险评估器健康状态 taxonomy、能量证据契约、报告健康 taxonomy、空域/数字孪生身份证据、任务重分配成员/Leader 身份契约、降级健康候选门禁、能量计划与编译成员的候选集合一致性、时间线/空域时间证据、证据链时间顺序、证据主体上下文、链路指标采样接线、Registry verified 能力查询旁路、维护护照状态条的待复核可解释性、事务目标集合完整性、扩展快照资源边界、交接后端二次确认/有效期、记录资源和交互版本门禁、编队同步发送/回滚重入边界、能力/构型 descriptor 资源边界、能力匹配候选资源边界以及普通批量动作的全量目标集合门禁；这些变更已列入 `MODIFIED_FILES.md` 第 143–168 项，仍不代表运行时或飞行验证完成。

当前源码已补齐基础运行时接线：实时 Registry 使用 `FleetCapabilityDiscovery` 归一化基础心跳，意图预览默认带入健康中心快照和黑匣子事件，事件规则与时间轴回放共享事件输入，事件规则支持版本化本地快照但不恢复冷却状态，意图生命周期摘要会追加到 `FleetMissionEvidenceChain`；AI 面板可由操作员显式把通过本地策略的 proposal 转为 Fleet OS 只读审阅草稿，申请/批准还会核对同一 `taskId/planId` 与 `needsReview=false`；控制器按较低频率生成链路降级快照，并只读显示维护护照评估/证据不足状态。模板/扩展/机队目录的写入口仅保留给 C++ backend，QML 只读消费。维护、链路和事件规则策略不会自动改任务、改链路或放行命令。这些仍属于待工程师构建/验证的 groundwork，不代表任务自动执行、飞行安全或硬件能力已被证明。

本轮继续收紧手动控制旁路：指挥中心的高度、速度和 `MPC_TKO_SPEED` 参数卡统一进入 `GuidedActionsController.requestControlCommand()`，标准 Guided 高度/速度滑块也冻结目标并在执行前复核链路、状态和健康门禁；起飞速度 setter 接受后会在 5 秒内轮询参数回读，记录已回读确认或超时。所有结果明确区分“已提交 Vehicle API”与“等待遥测/参数 ACK”。本轮仍只做源码和静态审计，未执行 Qt 编译、仿真、实机测试或刷机放行。

随后收口标准任务启动旁路：启动/继续/多机启动任务统一调用 `SwarmController.executeStartMission()`，冻结目标集合和任务项快照，并等待 `MAV_CMD_MISSION_START` 接受或 Mission 模式遥测；临时 Mission 上传完成后进入 `Starting`，启动失败或超时会清理任务并结束事务，不再把上传完成直接当作任务已启动。当前实现仍待工程师编译、ACK/模式时序和故障注入验证。

标准任务启动还会在每机下发前重新核对机载任务项数量/当前索引，并按固件是否发送 Home mission item 对齐 QML 序号，同时输出 MissionItem JSON 的 `snapshotFingerprint`；延迟发送窗口内任务项、序号或指纹发生变化时不会调用 `startMission()`。该指纹尚未与外部任务版本服务形成跨站版本锁定，仍需工程师补齐任务版本/哈希链路验证。

本轮目标模式继续收紧三类边界：故障证据显式携带 `faultHealthKnown/faultEvidenceAgeMs`，未知或陈旧故障状态不会被 `hasFault=false` 伪装成健康；Land/RTL 同机同操作保留唯一 safety-escape owner，事务在真实交给 Vehicle/MissionManager 前不会消费 ACK/模式遥测；临时 Mission 上传与 `MAV_CMD_MISSION_START` 分离记录 dispatch epoch。审阅、风险、匹配、回放、模拟和数字孪生的公共 QML 属性改为受控 facade/值快照。上述均为源码 groundwork，未执行 Qt/MSVC 编译、仿真、日志回放、硬件测试或刷机。

2026-10-07 目标模式续作：连续性规划器现在只接受真实 QVariant `bool` 作为在线/链路/健康/能力/故障证据，字符串布尔值或显式类型错误不会改变候选与 Leader 判断；证据链对单条 payload 增加深度、节点数、类型长度和大小边界；运营记忆与维护护照拒绝未知根字段，维护护照校验 `riskModel`。Custom PX4 插件工厂的现有源码/头文件也已恢复到 `custom.pri` 的构建契约。以上仍未编译或进行任何飞行/刷机操作，且本地 SHA-256 证据链不等于数字签名。

同一轮还收紧空域返航要求与任务风险输入：`returnRequired` 仅接受真实布尔值，风险评估的航点/覆盖/备用机计数、备用列表、severity 和 weight 均在进入概率估算前严格校验；覆盖计数关系异常时保持覆盖率未知并阻断评估，不会用表面有效的百分比掩盖矛盾证据。仍需工程师在目标 Qt/MSVC 环境编译和验证。

任务编译器继续收紧围栏约束：`constraints.geofenceRequired` 只接受实际 QVariant `bool`；字符串、数值或空值会生成 `invalid_geofence_required` 阻断风险并使计划不可行，避免隐式布尔转换改变地理围栏要求。此项仍只完成源码和静态审计，未执行 Qt 编译、仿真或硬件验证。

同一轮继续收紧风险与控制面：`FleetRiskRadar` 对任务参数和插件风险信号执行严格类型/范围校验，损坏输入返回 `inputValid=false` 并强制人工复核；`FleetEventBlackBox` 对单条事件的 type/source、payload 深度、节点数和大小设置统一上限；QML 公共属性改为 FleetHealth/Registry/Formation/Swarm 窄 facade，恢复任务按钮重新进入冻结目标和健康门禁流程。以上仍属于源码 groundwork，未执行 Qt 编译、仿真、硬件测试或刷机放行。

角色编组输入也已收紧：`FleetRoleAllocator` 逐字段核对机体身份别名，拒绝数字字符串/冲突 ID，并拒绝字符串布尔或非整数角色计数；参数错误不会生成看似可执行的 assignments/reserve。仍需工程师编译和验证真实能力快照。

意图审批还增加了严格证据闸门：`FleetIntentTask::approve()` 只接受真实布尔 `feasible=true`，并检查风险评估的 `inputValid`；风险评估明确无效或类型错误时会拒绝批准。该闸门仍是地面站源码约束，不替代工程师的权限和运行时验证。

事件规则输入也已收紧：条件路径、条件数量、列表长度和文本长度有界，数值比较只接受有限真实数值，`exists` 只接受真实布尔期望值；字符串数字/布尔不会被规则引擎隐式转换。规则仍只生成建议，不释放命令，待工程师编译和回放验证。

能力可信度继续收紧：能力发现拒绝非字符串标签和非 Map 构型证据，区分 heartbeat 的 observed-unverified 与静态协议/受信清单的 `verifiedCapabilities`，并校验 `discoveredAtUtc`；能力匹配新增 `requireVerifiedCapabilities=true` 门槛，允许安全任务只使用可追溯静态能力证据。仍需工程师接入真实 AUTOPILOT_VERSION/组件元数据并验证来源时序。

Guided 动作确认继续收口：`confirmAction()` 在动作专属校验后签发一次性 token，`GuidedActionConfirm` 原样回传；`executeAction()` 在触达 Vehicle/SwarmController 前同时核对 token、动作码和冻结 `actionData` 身份，成功后立即消费。缺失、复制、过期或重复回调会记录 `command.rejected` 并阻断；RTL/Land/急停仍只保留健康门禁的安全逃生例外，不能绕过人工确认。该改动仍未经过 Qt 编译、仿真、硬件测试或刷机放行。

本轮继续做地面站侧闭环：意图审批 token 绑定稳定的 FleetHealthCenter 健康快照指纹（忽略心跳时间戳和连续增长的年龄字段，只在在线/健康/故障/评分/状态变化时失效），外部传入的陈旧 `healthReports` 不再覆盖当前审阅证据；FleetRegistry 支持独立 `configurationClear=true` 清除旧构型证据；任务编排器只保存 1--255 的有限整数成员 ID；CustomPlugin 将可变后端类型移入 `MerivusInternal 1.0`，公开 QML 继续只得到受控 facade/值快照。以上仍未进行 Qt/MSVC 编译、仿真、日志回放、硬件测试或刷机放行。

连续性建议进一步绑定任务 revision 与稳定 Fleet 快照指纹（成员/在线/健康/链路/电量/能力/构型），在机队证据变化后旧建议 ID 不会被刷新规划重新解释为新分配；心跳时间戳和连续年龄不会造成无意义的每秒 ID 抖动。仍需工程师编译并验证不同回调时序。

恢复任务启动继续收紧：确认时冻结的 `resumeMissionIndex` 会跨越异步上传回调传入启动事务，并在上传前/启动前做整数与任务范围校验；不再使用可能仍指向旧计划的实时 `currentMissionIndex`。本项只完成地面站源码与静态检查，仍需工程师验证不同 MissionController 信号时序。

随后继续收口任务与连续性一致性：标准任务确认阶段通过 `missionStartSnapshot()` 锁定每个目标的 MissionManager 指纹，事务创建和延迟下发前都会重新核对；连续性规划器只对活动/待执行阶段生成变更建议，校验显式 Leader 成员关系并尊重任务级中途加入、能力可信和构型基线策略；Leader 接替应用现在在同一 revision 保护下同步更新成员与 Leader 字段，故障证据只以 fresh/stale 离散边界参与建议指纹。仍未执行 Qt 编译、仿真或硬件验证。

连续性面板也已补强为只读审阅视图：按严重度稳定排序显示最多六条，展示原因、任务 revision、Fleet 指纹前缀及受影响/候选机，并明确提示未展开建议数量；仍不提供自动重分配、任务写入或飞行命令入口。

替补候选选择也改为稳定的证据质量排序（构型 baseline、verified 能力、电量、链路、车辆 ID），不再受车辆发现顺序影响；这仍是人工审阅排序，不是自主调度或命令释放。

事务层还收紧了重试与 deadline：未真正开始 dispatch 的失败事务不可重试，运行中调整 timeout 会同步计时器，旧队列回调会重新校验当前 deadline，结果摘要明确给出 `terminal`。这仍是源码静态改动，未验证 Qt 事件循环时序。

健康聚合层也已收紧类型边界：`FleetHealthCenter` 不再把字符串布尔、字符串电量/链路、非法故障年龄或损坏健康分数隐式当作有效证据；这类输入进入 unknown 或门禁阻断。该项仍待工程师用真实 Registry/FaultTolerance 合并时序验证。

健康状态合并进一步改为证据优先：来源的 `healthState=healthy` 不能覆盖严格字段推导出的 unknown/critical/offline；来源的 critical/degraded/offline 只能保守升级，原始来源值保留为 `reportedHealthState` 便于诊断。

任务编译器的车辆身份也已与下游统一：assignments/members/reserve 只接受真实且别名一致的 1–255 有限整数，数字字符串、文本别名和冲突的 `vehicleId/systemId/id` 不会再被静默转成可执行成员。

健康门禁的目标集合也改为全量校验：混合列表中的非法/冲突 ID 不会被静默丢弃，`FleetHealthCenter` 返回 `invalid_vehicle_id` 阻断和 `invalidSelection=true`；任务门禁找不到安全 Leader 时通过 `globalBlocking` 拒绝，避免无 Leader 情况误报允许。

任务编译器在收到非空 fleet 快照时也执行闭合成员校验：非法 fleet 条目、未知 assignment/reserve 都会使计划进入 blocking review；只有明确不提供 fleet 的 planning-only 预览才跳过成员存在性比对。

备用机输入不再静默过滤：reserve 列表中的非法 ID 会生成 `invalid_reserve_vehicle_id` blocking 证据，避免无效条目被删掉后错误满足备用机条件。

任务编译器的 geofenceRequired 门禁也改为验证真实边界几何；空/错误 geofence 字段不再因为“字段存在”而绕过围栏缺失阻断。

assignment 角色也已对齐角色分配器：只接受真实字符串 `leader/scout/relay/responder`，未知、空值和非字符串角色进入阻断。

健康门禁在聚合层再次限制电量/链路证据范围为 `-1..100`，防止未来绕过上游校验的越界数值伪装成健康。

任务编译器文档已同步最新闭合契约：非空 fleet 必须是唯一合法 ID 快照，角色/Leader 关系和 geofence 顶点均严格校验；空 fleet 才代表 planning-only 预览。

编译结果现在显式带有 `fleetProvided`/`planningOnly`，审阅层可以区分未提供机队的纯规划与已提供但校验失败的闭合机队输入。

本轮继续补强健康证据生命周期：`FleetRegistry` 和 `FaultToleranceManager` 保存 fault evidence 的地面站观测锚点，并随刷新推进年龄；单次 `age=0` 报告不会因为后续只有心跳而永久保持新鲜。删除成员、设置 Leader 和清空状态的建议集合也已收口，未注册 Leader 会被拒绝。

配置清除入口拒绝与新 configuration/fingerprint 同时出现的矛盾更新；能力发现和 Registry 对 null fault age 按未知处理，避免 QVariant 隐式转换伪造故障新鲜度。

多机标准任务启动不再复用当前活动机的任务项数量/当前索引；多机确认快照按每架机的 MissionManager 指纹复核，避免异构任务被单机 UI 快照误拦截。单机启动仍保留数量、当前索引和指纹三重一致性检查。
## 目标仓库保留的 FTC 历史验证记录（本次未复跑）

更新：2026-09-17。最终分支 `codex/ftc-full-validation`，收口前源码提交 `e7444f32b2f81c699e8b881a1262ca8f1945f5ee`。

FTC 已同步 v2 extensions，typed backend 区分模型无效和消息过期，主 H/E 不显示无效历史估计。主界面保持简洁，估计内部诊断放在详情；ACTIVE 仅根据固件实际反馈显示。参见 [协议契约](architecture/FTC_TELEMETRY_CONTRACT.md)。

Qt Test 共 8 个结果项通过，Windows Qt 5.15.2/MSVC Release 构建通过。实际 VM—Windows 链路解码 26092 个包且 CRC 错误为 0，已验证 3 秒过期与恢复；这不构成绝对零丢包证明。FTC 详情面板的最终视觉样式、多机布局和目标硬件链路仍需复核。跨仓结论见 `E:/MERIVUS/FirmwarePX4/docs/testing/FTC_VALIDATION_SUMMARY.md`。

本次 2026-10-07 续作没有重新执行上述构建/测试：新增能力发现自审计和 QGC live descriptor 二次校验只完成源码与静态边界收口，工程师仍需在目标环境重新编译并按交接文档复核。

同一续作还收口了几条地面站事务边界：单机 Takeoff/Land/RTL 冻结目标并取消 activeVehicle 直落，确认数组增加数据指纹；Formation timeout 绑定 phase/deadline；Mission/Land/RTL 完成证据记录 dispatch 前模式；queued goto 拒绝混合非法航点；事务列表按单调创建序返回。临时 Mission 的共享 MissionManager 回调仍缺少底层 operation token，只增加了机载任务为空的后置核验，必须由工程师在 Mock/SITL 验证。

本轮又修复了能力发现首次注册的数据保留缺口：顶层 verified/证据状态、构型证据来源、discovery 状态/错误、发现时间和位置证据源会进入 `VehicleCapability.metadata`，并由 `toVariantMap()` 重新暴露以保证往返无损；同时由 FleetRegistry 做严格类型/时间戳入口校验。仍未重新编译或执行运行时回归。

能力匹配的可信边界也已收口：verified-only 请求不再把运行时 `canUse*` 布尔观测当作已验证能力；普通规划/观测模式保持原行为。仍需工程师用能力任务样例确认策略位传递正确。

任务连续性替补路径已同步相同可信边界，并把 verified/discovery 证据纳入 fleet fingerprint；仍未编译或运行替补重分配回归。

能力规划器的车辆能力列表入口也已严格类型化；坏能力证据现在拒绝候选，而不是被 QVariant 宽松转换。仍未执行运行时输入回归。

连续性规划器的机队快照入口也同步拒绝坏能力列表，避免输入无任务能力要求时仍选入畸形成员。仍未执行运行时回归。

Matcher 的任务能力要求也改为元素级严格类型；坏任务参数会直接标记不可行。仍未执行运行时回归。

角色分配器已同步 verified-only 能力策略并严格校验能力列表；仍未执行角色分配运行时回归。

任务模板审查入口也已支持同一 verified-only 策略并拒绝坏能力证据；仍未执行模板验证运行时回归。

能力 Matcher 的非法参数路径现在不会残留选择列表；仍未执行运行时回归。

本轮又收口了多机列表的命令入口：启动任务、单机暂停和单机 RTL 进入 GuidedActionsController 的确认/目标冻结路径；直接解锁和接管模式按钮暂禁。批量暂停改为只执行确认时冻结的车辆集合，不会把确认后加入的机体纳入指令。模式菜单、任务上传/清空和 Mission resume 的底层 operation token 仍是后续工程师工作项；本轮未编译或运行验证。

多机列表的单机启动/继续任务确认文案现在显示冻结的 UAV 目标，不再因当前活动机的显示属性而误隐藏显式目标动作；仍需工程师在多机 MissionManager/ACK 时序下复核。

PlanView 任务上传/清空和飞行完成清理现在也冻结活动机对象，并在延迟回调前再次检查链路和舰队健康门禁；仍未接入统一任务写事务或真实 ACK，不能据此宣称任务写入可靠性已验证。

Mission resume 异步回调现在要求地面站侧 request identity，并冻结/复核任务项数量；无活动上下文的 ready/fail 信号不再打开重试或启动任务。底层 MissionManager signal 仍没有 operation token，需工程师继续接线验证。

自定义 Guided Action 按钮已从直接 `sendTo(activeVehicle)` 改为控制器确认/健康门禁/目标冻结/审计路径；底层自由 MAV_CMD 接口和 ACK 仍未完成 C++ 权限隔离。

三个飞行模式菜单在 MERIVUS 资源中也已统一经过控制器确认和目标/健康检查，安全返航/降落模式保留逃生例外；通用资源回退和真实模式 ACK 仍需工程师验证。

自定义 MAV_CMD 现在有显式 `safetyClass`，未知命令默认 disarmed-only，C++ `sendTo()` 与控制器两侧都检查连接和飞行态；这仍不是命令白名单或 ACK 闭环。

本轮继续收口自由维护命令入口：Joystick MAV 命令读取 `safetyClass`，现有 `MAV_CMD_USER_1/2` 默认 `disarmed_only`，C++ 发送侧拒绝未连接、链路丢失、已解锁或飞行中的调用；Mavlink Console 增加相同的已连接/未解锁/未飞行门禁，QML 键盘回车、粘贴和发送辅助函数在被拒绝时提示操作员。以上仅为源码和静态边界收口，仍未执行 Qt/MSVC 编译、仿真、硬件测试或刷机；命令白名单、角色权限、维护模式及 MAVLink ACK/超时闭环仍由工程师补齐。

事务身份边界继续收紧：`CommandTransaction` 不再把任意 `QVariant` 转成字符串键，目标与 ACK 回填只接受真实有限整数 1–255，并使用规范化十进制键去重，避免 `"01"`、布尔值或文本别名混入事务状态。该改动仍需工程师通过 Qt/Mock 验证 QML 数字到 `QVariant` 的实际类型映射。

Plan Upload 入口已收口：PlanView 面板按钮继续调用已有的 `PlanMasterController` QML `upload()` 包装，顶部工具栏增加 `globals.planMasterControllerPlanView` 存在性/方法检查后再调用同一包装；该包装保留任务完整性、目标冻结、FleetHealth 门禁和预检。PlanView 销毁时清理全局 controller 引用，仍需工程师验证 QML 生命周期。

MERIVUS Guided 的单目标 legacy 动作在确认后增加统一执行时链路检查；即使 RTL/Land/急停保留健康逃生例外，确认期间掉线或初始连接未完成也会 fail-closed，不再继续调用目标 Vehicle API。尚未执行 QML/Mock 时序验证。

PID 调参页的自动模式切换和 `FlightModeDropdown` 也已接入 MERIVUS 的 `requestFlightModeChange()`，不再从这些控件直接改写飞行模式；普通 QGC 无控制器时继续使用兼容回退。仍需工程师验证模式确认、active vehicle 切换和真实 ACK。

`RCToParamDialog` 的维护参数写入现在要求目标已连接、链路正常、未解锁/未飞行和有限数值；MERIVUS 上下文还要通过 FleetHealth 门禁，失败时不触达 `sendParamMapRC`。仍需工程师验证参数回读、拒绝提示和实际 MAVLink ACK。

MERIVUS Guided/CommandCenter 对链路管理器缺失的语义已统一为“链路未知/不可发送”：执行门禁、按钮可用性和状态颜色不再把 `vehicleLinkManager=null` 当作正常连接。仍需工程师用断链/初始化异常快照验证 UI 与控制器一致。

PX4 CameraComponent 的 Apply and Restart 现在要求连接、链路、未解锁和未飞行状态，避免设置页在不安全状态直接调用 `rebootVehicle()`；仍需工程师验证重启 ACK、参数写入回读和页面状态恢复。

参数维护批量写入本轮完成一处 P0 收口：`ParameterEditorController` 的参数文件 `sendDiff()` 和两个 reset 路径现在在 C++ 侧要求活动目标仍是创建时的同一 Vehicle、初始连接完成、链路管理器存在且未丢链路、车辆未解锁且未飞行。参数文件每行的 vehicleId 解析改为严格检查，缺失/非法/混合/跨机文件会被 `diffOtherVehicle` 标记并拒绝应用；`ParameterDiffDialog` 展示原因、禁用确认并在竞态失败时保持弹窗。ParameterEditor 的加载、重置、RC 映射清理、重启以及单参数编辑/恢复默认均增加维护态提示。该改动仍只完成源码与静态边界，未执行 Qt/MSVC 编译、仿真、MAVLink ACK 或硬件验证。

兼容 QGC Guided fallback 也完成一次 P0 传输边界收口：`src/FlightDisplay/GuidedActionsController.qml` 在 `executeAction()` 的实际发送前重新检查初始连接、`vehicleLinkManager` 和通信状态；多机 Start Mission/Pause 在发送首条命令前预检所有车辆，任何一个目标失效即整批拒绝，避免部分执行。该补丁不改变 MERIVUS 自定义 controller，仍需工程师验证 QML vehicle list 类型、ACK/超时和运行时确认流程。

兼容 Guided 的 Mission resume 入口已与目标冻结确认链对齐：`actionResumeMission` 不再在 `confirmAction()` 中无条件早退，而是显示确认框并在执行时复核目标与传输状态；单机成功、未知动作或多机预检失败会清理一次性确认目标，避免旧状态复用。仍未验证 MissionManager 的恢复上传、operation identity、ACK/超时和 QML 生命周期。

PX4 传感器校准完成后的重启快捷入口也已收口：`SensorsSetup.qml` 要求初始连接、链路管理器、无通信丢失、未解锁且未飞行，按钮和点击时都执行检查。Mission complete 的任务清理二次检查同样拒绝未完成初始连接或缺失链路管理器。两处仍需工程师验证实际重启/任务清理回执及页面生命周期。

Mission resume 的底层生成入口也补了 P0 防崩边界：空任务、过滤后空任务和不存在的当前项均 fail-closed，不再可能在 `_missionItems[-1]` 或短任务的第二个 Home 索引上越界。编队 ABORT 对被跳过成员保留失败证据，避免没有实际下发回滚却报告本地完成；ABORT 进行中重复停止返回原事务，不覆盖已有 ACK 处理器。仍未验证真实 ACK/超时和并发回调。

APM 的三个维护重启组件（传感器校准、安全、电源）现在与 PX4 重启入口采用相同连接/链路/飞行态门禁。通用 MotorTest 和 Fact 参数控件尚未统一到 C++ 维护权限层，已列为工程师后续审查范围。

编队事务的通用取消入口已 fail-closed：`formation.*` 不允许只取消地面事务，必须经过确认的 Formation ABORT；拒绝会记录 `transaction.cancel_rejected`，避免 ACK 归属被切断后机载成员仍停留在编队状态。仍需工程师验证 ABORT ACK/超时和 UI 重复点击时序。

意图式任务审批现在同时绑定健康快照和机队能力/构型快照指纹。`prepareFleetIntent()` 若收到旧的外部 fleet 副本，审批阶段会因与当前 FleetRegistry 不一致而失效；Registry 变化也会主动清理审批身份，避免改装、替换或能力证据变化后继续使用旧方案。仍需工程师验证 QVariant/QML 列表映射、registryChanged 生命周期和真实发现证据更新。

Mission resume 的地面站异步上下文增加 120 秒有效期和确认入口门禁：ready、upload-fail 与执行前均要求 requestId、目标 Vehicle、恢复索引/任务项数量快照一致；不满足恢复条件的 direct `confirmAction()` 不再创建上传上下文。底层 MissionManager 信号仍没有协议 operation token，工程师必须继续验证并发恢复、旧回调和 ACK/超时。

通用和 APM MotorTest 滑杆也已加上连接、链路、armed/flying 双重门禁；链路或飞行态在拖动期间变化时，滑杆归零且不触达 `motorTest()`。仍需工程师审查 PX4/APM 的 C++ MotorTest 权限、MAVLink ACK 和 UI 断链时序。

恢复任务生成器在过滤重复相机/ROI 指令和非时间触发项时同步释放被移除的临时 `MissionItem`，避免反复恢复任务造成内存泄漏；此项只修复本地对象生命周期，不改变任务语义或替代真实上传/ACK 验证。

ACK/超时静态审计进一步确认：`Vehicle::mavCommandResult` 和 Mission Protocol 消息没有协议级地面站 operation/session token；`PlanManager` 现在补了本地 operation identity 及带 identity 的完成/错误信号，但同一 Vehicle/命令或跨 Mission 事务的迟到旧无线消息理论上仍可能被新事务观察。本轮 Swarm 的 dispatch epoch、互斥、PlanManager 本地 identity 和后置任务快照只能降低风险，不能替代协议级 token 或 ACK quarantine；工程师必须先补齐并实测，再把事务结果当作已完成。

本轮在 `Vehicle` 侧增加了兼容性 ACK quarantine：只对无响应终态记录同组件/命令的单调时钟隔离，重发在窗口内 fail-closed，迟到 ACK 只记录并忽略；连接初始化/流配置使用的 `REQUEST_MESSAGE`、`SET_MESSAGE_INTERVAL`、模式切换 `DO_SET_MODE`、MotorTest 和 `COMPONENT_ARM_DISARM` 不隔离，以免破坏同步链路、安全逃生或受控解锁/停机。孤立的 `SET_MESSAGE_INTERVAL` ACK 也不再推进下一条流配置。它仍不能证明 MAVLink ACK 的物理归属，协议 operation token、Mission generation 和运行时回归仍由工程师完成。

`PlanManager` 现为每次 mission read/write/remove-all（包括 ArduPilot guided 写入和 resume 上传）生成单调的地面站 `quint64` operation identity，并在保留旧信号的同时提供带 identity 的完成/错误信号。`PlanMasterController` 的 Mission 读写串联只接受自己发起的 operation identity；Swarm 临时 Mission 上传与清理回调也按该 identity 归属，再叠加机载任务列表后置校验，避免其它控制器的完成事件推进本事务。该 identity 不会写入 MAVLink，不能替代协议级 ACK token；仍未进行 Qt/MSVC 编译、Mock/SITL 或硬件验证。

本轮进一步把 identity 接到 `MissionController` 与 `PlanMasterController` 的全计划串联：MissionController 通过 `operationStarted` 与带 identity 的读/写/清空/恢复回调拒绝旧事务刷新 UI；PlanMasterController 对 Mission、GeoFence、Rally 的分阶段读写分别保存 operation ID，Mission 写失败不再继续提交后续围栏/备降点。旧无身份信号仍保留给兼容层；该改动仍只完成源码和静态审计。

指挥中心新增能力证据摘要：按显式 `verifiedCapabilities`、observed-only 和 `discoveryState=invalid` 分类 Registry 车辆，帮助操作员识别能力可信度；该只读提示不替代后端 verified-only 匹配门禁。

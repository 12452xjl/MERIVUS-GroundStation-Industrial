# MERIVUS Fleet OS 后期开发目标

## 546：按确认范围收口，不再无限扩展本地任务

用户明确选择“按本地源码范围最终交付，明确列出外部未完成项”。545确认的四条本地缺口均已接入真实操作链：完整Run检查点、人工规则待审、用户模板、显式留存编排。对原25项再核源码时，补齐目标19四受众报告的完整查看/后台文件链，并修复已哈希报告被后加根字段导致校验失败的问题。

当前25项的本地证据与完整产品外部依赖集中在[最终范围表](handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md)。这是交付边界，不是把跨站控制、动态插件、正式机载结果、实测模型等改称已完成，也不再把它们无期限作为本仓库待补小功能。没有新增产品目标。

本地源码及交接资料完成限定静态收口后统一封包；构建/仿真/硬件/发布由工程师负责，未执行。原任务边界、人工确认和最终StartPermit限制保留。下方545及更早为历史，不能据旧百分比或旧missing列表解释当前范围。


## 545：证据链落盘工作流与交付边界校正

目标20已有真实计划/审批/事件生产者，现在补上明确冻结保存、独立历史重开与有界详情；不再把“原生类有save/load、界面只有数量”称为完整工作流。此项代码接线完成，仍待工程验证，不是Run灾后自动检查点。

本次从最新源码核实的后续优先工作流还有：实际完整Run后台检查点与重启只读复盘；规则建议进入可审阅/拒绝的人工待办；本地模板编辑/版本/实例化；基于已验证备份的显式留存编排。这是四条已确认缺口，不是全项目穷尽统计或四个产品目标，不据此计算百分比。

以下须与“尚未写代码”分开记录：真实机体/控制包络、内部有效状态、可归属的机载任务实例及正式结果、实测性能/电池/维护模型与数据、企业账号/撤销/控制租约/跨站提交服务、插件ABI/签名信任根/实际适配器、权威地理资料与远端补传来源。这些当前没有足够实际输入或外部契约，不能靠本地布尔字段认证，也不授权擅自改PX4或搭建远端生产系统。

已存在真实源码操作链的元数据、能力工程审阅、本机双账号审批、草稿保存恢复、上传/回读与受限启动、余段/备用出勤、占用/待办、时间预演/预算、Run观察报告、SQL索引归档、维护与GNSS历史工作区，不应继续仅按旧历史段列成“未开发”；它们仍不等于整个对应目标或工程验收完成。原25目标不缩减、不增加新创意需求；最终统一交付，不制作阶段包。

## 544：推进任务编排的实际路线一致性

修复普通公共路线没有进入实际成员的断点：缺省成员路线经原始要求检查后展开，后续预算、预演及上传使用同一规范成员。原成员路线优先、coverage不回退、时间和返程参考保真；不新增目标、不把该修复称为完整4D执行或飞行验证。原25目标保持；不按修改路径或历史批次推算完成率。无阶段包，继续最终源码交付。

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

### 下一源码切片：近期掉电速率进入能量预算（未实现）

沿原目标9的续航调度继续接真实消费者，而不是新增孤立报告。当前Vehicle原PRIMARY电池账本只有单次百分比／报告剩余秒数，Registry投影当前值；EnergyScheduler的耗电速率仍来自静态元数据，严格性能分支不消费这种未绑定值。

后续从同一原生电池来源建立有界、单调时钟的SOC下降观测窗，经Registry实际绑定进入现调度器，并由现有重新审阅及完整energyPlan明细消费。缺样、间隙、上升、换源等须保持未知；只允许进一步下调已有续航预算或预计剩余电量，不补齐未知、不满足严格性能声明、不上调能力。SOC是飞控报告／估计，不称电功率实测、实物电池身份或未来工况认证。本段仅确定下一开发切片，没有新增该算法、执行或验证，也不改变原25目标／六类实质剩余。

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

## 509 当前：目标14新增受控单流操作

在508真实读取之上，加入独立权限和不同账号审批的人工STOP／START→状态回读。一次基线、相反目标、最早30秒、每项单发、失败保partial；不能从停止记录、旧批准或历史快照自动恢复视频。

本轮保留原相机手动切流；自己的实际启停之后允许RUNNING-only Info更新，但其它配置语义、来源、角色或期限变化不能重基。匹配报告不是物理效果或带宽保证。

目标14仍需容量／丢包可信来源、自动视频与速度降级联动、多机优先级、稳定恢复补传及效果验证；本批不是自动QoS或目标14整体完成。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈受控执行、企业跨站、长期运营及真实模型／插件生态六类均继续，原25目标不删。

[30路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[控制合同](../custom/src/Swarm/FleetCameraStreamControl.md)。整体约40%／剩余60%仅源码工作量粗估。本轮未编译、执行项目实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

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

## 506：目标14前置收口真实相机／视频链，不完成整个视频降级

本轮修改现有CameraManager／CameraControl→VideoManager真实调用链：本地播放意图、原相机／流选择代次、有限接收器恢复、活动飞机切换和通知回调后的对象存活检查；共享QmlObjectListModel补通知后的自身存活尾核与匹配reset。不是新增只读helper，也不是完整Fleet视频降级／受控停流事务。

后续仍需独立受控远端视频配置／停流授权、原生同源事务及回读、真实容量／丢包来源、视频与速度降级联动、稳定恢复补传与效果核验；不能把本地decoder停止当带宽已下降。完整任务执行、可信性能、自愈改派、跨站协作、长期运营及真实模型／插件生态仍按原路线图继续。

[合同](handoff/MERIVUS_VIDEO_LIFECYCLE.md)、[19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[50项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505：目标14／18接真实日志缺块补读，不缩成完整补传已交付

本轮修现有LogDownloadPage→LogDownloadController→LOG_REQUEST_LIST／DATA→本地文件的真实消费者：同一目录／批次来源、冻结选择、有界缺块补读、完整写入后位图与去重计数、NewOnly partial及完整刷新／改名后才显示Downloaded。不是另建只读建议helper，也不是完整Fleet控制中心补传。

本轮落实的是现有机载日志拉取的正确性与资源／期限边界；失败保partial，不承诺跨重启续传。目标14仍须视频／速度降级联动、可信容量／丢包来源、真正Fleet恢复补传及效果核验；目标18仍须完整历史索引、长期留存／维护集成和灾后恢复。

[合同](handoff/MERIVUS_LOG_DOWNLOAD_RECOVERY.md)、[14路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或重试次数上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504：目标14增加实际单次定向耗时链，不替换完整降级目标

504新增独立人工单次定向TIMESYNC往返耗时采集：实际主账号／Role／Root原意图→Swarm私有服务／原生Run→真实同车绑定review→Policy及可见面板。最早四秒包含授权核验和后续100ms轮询，不自动采集、重试或续期，不回退PING或广播。

使用v2 TIMESYNC而非把PING正target误当请求；旧v1会截扩展，严格拒绝不广播回退。地面单次样本包含队列和远端处理，不能当完整实时网络QoS。后续仍需有界连续测量政策、飞行授权场景、真实容量／丢包来源、视频／速度联动与恢复补传。

[模块与适用边界](../custom/src/Swarm/FleetLinkLatencyCapture.md)、[31路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503：目标14从声明指标推进到实际被动接收观察

503将实际PRIMARY／本车默认component的原始解码帧接到独立五秒接收窗，经Registry实际绑定读取、Policy独立消费、Guided原五秒周期和指挥中心只读摘要。记录接收帧数、编码MAVLink字节率与最大内部间隔；不把它们当可用带宽、RTT、丢包率或远端发布率。

只完成其中本地接收证据；不以编码接收字节率认证可用容量，不以最大内部间隔认证RTT或最大沉默，不用seq差推丢包。真实RTT／容量／丢包来源、视频／速度受控降级、稳定恢复／补传仍须继续；不更改原25目标或缩成“只等工程师开按钮”。

[合同](../custom/src/Swarm/FleetLocalReceptionObservation.md)、[精确23路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502：目标14接显式操作员界面，不把接线当运行验收

502新增FleetOptionalTelemetryPanel可见界面，在指挥中心和地图规划页复用同一actual Root。操作员显式只读采集→另行审阅统一目标→两项确认后独立账号审批→逐项配置与回读；固定62／74／83，原501实际后端和授权合同不变。不会自动采集、Apply、续期、重试或回滚。

本页仅由真实返回result5及同一当前workflow24的ID／代次／hash建立拥有关系；共享状态或历史不领权。稳定来源只取原scope／主身份5／双政策9，自有ID／代次／状态推进不误取消；完整sourceToken仍在新请求前精确比较。审批不借全局busy自拒，关闭／换源／输入变更退休本页资格并精确取消原ID，晚返回不能复活。

源代码沿现有QGC控件／主题，显式取消、错误摘要、可滚入焦点、长按钮换行和只读详情。按用户禁执行约束，UI/UX技能采用完整静态参考，不运行检索／预览／界面脚本，实际体验留工程师。仍需真实带宽／延迟／丢包测量、视频／速度联动和恢复补传。[面板](../custom/res/Merivus/FleetOptionalTelemetryPanel.md)、[44项全未执行](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501：目标14的独立人工配置竖向链路

501新增独立双账号的地面可选遥测配置：从未来新成功Capture的三条真实native上下文封存不可序列化Basis，在同一最早30秒内独立审阅／批准，再逐项511→510并最终三项510复核。仅固定62／74／83、同一正int32且精确REAL32间隔，目标不得比任一原正基线更短；不启停流、不降频P0、不改视频或编队速度。

start仅认领ready、不写包；100ms后推进九个wire阶段。每条至多4秒，510 accepted ACK后至多1秒且只缩，所有阶段与审批共用原Capture最早30秒。慢链路可能无法完成；截止／失败／取消保留已发事实，不重试、不自动回滚。三项最终回读只给reported-values-match，不认证无线因果、实际发布率、带宽或飞行许可。

此步把原只读策略推进到限定地面后台写入／回读，不以Basis／Run或ACK证明实际频率生效。尚需显式操作界面、真实链路测量／效果核验、视频和速度降级联动、恢复补传等；不把本批替换成目标14或25项产品全部完成。[模块](../custom/src/Swarm/FleetOptionalTelemetryConfiguration.md)、[46项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500：目标14从被动报告推进到独立只读采集

新增Swarm私有持有的FleetOptionalTelemetryCapture，显式单车62→74→83，实际Session主账号＋独立fleet.telemetry.interval.capture政策＋原Root/controller意图。接纳后100ms轮询才发；原入口固定30s总期，native每项一次510／4s，accepted ACK后至多1s且只缩，必须原ACK与目标244双端齐全。最终write前复核授权、物理来源、完整载荷并冻结receipt基线；三项同原version／transport／mutation／已绑定ledger，已采语义A→B→A不复活。取消保已发／partial事实，先存历史快照再精确释放原query，不宣称撤回或实际频率已生效。

顺序是独立人工确认／实际账号政策→原生510与目标244采集→整批同源复核与历史展示。既有五秒Policy review不是自动查询入口。下一实质切点仍是独立配置权限及必要二审、以当前采集为依据的逐步511 Apply、同源回读和部分已发结果；不把采集action或snapshot借为写权限。

[完整契约](../custom/src/Swarm/FleetOptionalTelemetryCapture.md)、[24路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前：目标14的共享native发送与有限等待，不是capture／Apply交付

510／511／512现走同一Vehicle级跨component专用lane：外调前预留UUID／不回绕serial，固定原提供者QQ、PRIMARY弱控制块与实指针、transport代次及原固定期限；原nativeVersion直接入队也接入，但保原地面／参数／实际授权fence和单次3秒，不借generic条件放宽。generic初连不要求MVM已登记或握手已完成；版本资料首次锚定与实际transport变化分开，native批准依据不自动rebase。

通用512的RequestMessageInfo由Vehicle持有shared context，ACK与原始回包两端终结前保原lane。原500ms响应timer在queue空之前也核固定deadline，ACK后1秒回包窗口只缩原期限；静默、IN_PROGRESS或重试不使等待无限。原包入口冻结原请求归属，实际回调仍在P0原生围栏之后；消息0合法，不再凭0哨兵判无请求。可选callbackOwner仅是QObject寿命条件，不是角色／账号授权；InitialConnect与StandardModes实际传原owner，未传owner的旧borrowed-data调用者仍自行保证数据寿命。

本车默认component的任意511只在最终actual write前，以同UUID一次预留推进不回绕mutation generation，并全清498三report及接收窗；ledgerEpoch为UUID:十进制generation。writeDepth贯穿write与通知，临界内原包准入／getter不可当前。未发送拒绝不冒称已配置；已可能write的代次／requestIssued不回滚，耗尽保守停止而非重用。498观察15／Registry review21／comparisons6保持，后续只读重采会看到旧基线失效，既有值快照不自动变成实时通知，须新被动资料与新完整5秒窗，不等于实际生效。

本地common协议头822–827明确GET_MESSAGE_INTERVAL510用param1指定流并返回244；512参数2用途须由对应消息定义。不能因共享lane已支持510／512，就假定当前固件支持“512请求244、param2选择62／74／83”；新Fleet主动capture仍需独立窄scope和工程兼容验证。本批未读取或修改PX4飞控。

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

[5源码＋13文档=18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[42项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。真实任务执行／动态轨迹与正式结果等原范围不缩减。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前：目标14从策略值接到实际被动资料，仍非apply

原244报告和可选62／74／83本地收包窗已进入原生账本，Registry从其实际Vehicle绑定读取一次，现有Guided五秒策略快照有真正只读消费者；不是再添加一个无消费的helper。报告间隔、接收频率、数值比较与授权／实际生效严格分开。

每流5秒尾窗、最多512个单调接收时刻；原来源绑定满5秒、该流曾收到且最近包不超过30秒、无相关接收栈／探测中状态和窗内容量丢失才可complete。完整窗可以0帧，仅表示本地该5秒没收到；从未收到过不能complete。均值是本地收包数／5秒，最大内部间隔不含窗两端空隙；不推算发送率、链路丢包或停流。

本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

[18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。完整任务执行／动态轨迹与正式结果等原范围不缩减。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前：目标14的真实传输前置，不把已有建议冒充执行

本轮先修已有511链的确定缺陷：任意终态推进PID／ESC、容器元素跨callback、同步重入及provider销毁后的旧指针。新增native请求归属／迭代pump／来源尾核实际进入旧PID和ESC发送链，公开档位及原5Hz／1Hz不变，失败停链且保可能改动，不自动恢复。

公共511 entry固定原VLM／Protocol／Plugin、PRIMARY QQ与weak控制块及实际pointer、原source epoch和本地UUID；复用构造期持久pin，不从已销毁raw重建。外调后入队前再核duplicate/quarantine，实际发送前核原UUID及完整payload，失败按原UUID重找。ACK只接同原物理来源，IN_PROGRESS不续期；511超时加入原隔离，不新增每个成功终态冷却。

PID与ESC各自一次handler/context消费自己的本地请求，删除原任意live511推进PID／ESC的分支。STATUS5Hz→INFO1Hz、原PID两消息100Hz及手动默认恢复入口保持；拒绝／超时不自动续链。same-source迟到重复wire ACK仍可能歧义：协议不回显messageId，本地ID／UUID／来源核对不是wire因果认证，accepted也不证明实际频率、带宽或安全效果。

真正的目标14仍需独立实际Session/角色scope与必要二审、当前原生链路证据／期限、闭合optional白名单、原MESSAGE_INTERVAL244基线/回读及收包频率窗口。不能借只读refresh或start/capability凭据，不能照搬Policy的全telemetry.rateHz，更不能降频P0证据或控制协议；本批没有新apply UI/入口／权限／自动飞行动作。

[6源码＋12文档=18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项未执行工程场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步；完整执行许可／动态轨迹和正式结果等原目标仍继续。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前：目标6／12补完整垂直编码参考，不完成实际起飞轨迹

新增同次Contact原起点／窗口材料，Waypoint将full current与full predicted top完整投影盒构成参考棱柱；旧名义组件保留，旧495 top→首NAV连接继续。完整起点需容纳于原rest±tol，不以裁剪或扩大pad换支持；实际TAKEOFF缺schema2／ref使整组不完整。

只有本vertical/support的指定地形完整交集可以同finitepad判例外，其它body／障碍／后续航段／Domain不借。v5、原31/schema1／Worker7／预算／七false不变；动作22序号仅作归属，不认证current端在wire或实际飞行时序。

[19路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项全未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原真实动态起降／HOLD／平滑器、受信地面与连续物理轨迹、最终StartPermit和正式结果继续开发。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前：目标6／12的起飞参考连接进入实际环境消费者

沿P0真实缺口推进，不再增加只有显示的flag。Waypoint从原GPI9／PM29／编码与Joint对应重建完整top高度编码区间，对首NAV所有完整接受分量逐一构造连接，并复用原覆盖／走廊／地形／障碍求解。旧NAV与LAND范围不替换，新区域计入原真实总计数与详情，v4报告由原matcher和Worker链消费。

这只是ground-branch条件声明参考体，不是对实际控制器阶段、动态高度扩大、平滑器、连续物理轨迹或StartPermit的认证。独立动作未认证与原七false保持，HOLD动态/漂移仍缺内部证据，不以同点重复帽冒充新进展。[16路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前：沿目标18／19／20补独立历史报告文件入口

原生保护TaskRecord捕获仍是唯一当前本地选择来源；新文件重开只产生独立历史值，不反向恢复任务。保存／打开不重新build，保持原报告ID／时间／hash。UI显式选择路径并再确认，报告或选择变化后旧意图不能复用。

这补的是既有报告的持久交接能力，不替代签名服务、企业权限、控制租约、跨任务遥测关联、留存治理或正式任务结果。[18路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前：沿原目标18／19推进遥测历史实际检索

新增跨采集Run的实际记录元数据搜索与原摘要锁定打开，补掉只有轻header目录／单file页的缺口；不是重复事件归档已有的跨封存search。由原native完整catalog取所有descriptor，包括坏header诊断；新search独立合同、预算与cursor，不修改旧页语义。

当前仍仅有限目录／文件批次，不是长期全集索引、自动留存清理、全遥测、任务Run映射、维护／运营全统计或灾后恢复。首末记录序号不是时间排序或跳转位置；打开的是锁原hash的文件首页，仅按机号过滤，可能无该msgid。

[20路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)及[36项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。P0完整动态轨迹与最终StartPermit／正式结果不造证据；可信性能、自愈受控执行、企业跨站、长期运营和真实模型生态原六类与25目标不缩减，整体约40%／剩余60%仅源码粗估。UI沿原QGC控件，技能仅静态参考。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前：先修真实P0生命周期缺陷，动态域不造证据

执行观察、Vehicle live Link检查和原TaskRecord消费的同步外调链改为原对象／原代次尾核；完整source检查仍执行，旧QHash／Target指针不跨回调，实际write后原commit事实保留。不是再加一个只显示的报告flag，也不把来源缓存当新的live许可。

只读配套PX4进一步确认动态altitude_acceptance由避障模块的实时垂直误差、历史hysteresis及内部时间戳发布，PM29／现有MAVLink未提供完整内部状态，不能单由参数造有限真实上界。TAKEOFF当前名义竖段／连接之外仍需完整声明动作域，更不能把起飞目标高度上界当实际切换／轨迹上界。本轮因此优先修可确定的P0对象重入问题，不改PX4或把actualPath等置true。

下一步保留真实动态接受／动作入口与TAKEOFF／HOLD、连续轨迹和完整StartPermit；可信性能、自愈机载改派、企业跨站、长期运营与真实模型／插件生态原六类不缩减。遥测跨Run只读检索是已识别的后续运营竖切，未在本轮实现。整体约40%／剩余60%仅源码粗估。[22路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前：LAND声明参考域落入真实消费者

目标6／12从名义中心线扩到前NAV接受区域的着陆参考体。复用原29参数、ActionPhaseReview的LAND兼容分支、同次Joint与已应用contact，不增传感器／遥测／默认地面；新几何由原Waypoint报告和Worker／Review／Witness链实际消费，没有平行报告槽。

只读配套PX4可见MOVE_TO_LAND有条件插入，且其高度取现有POSITION目标或当前global而非LAND p7；直接LAND时XY收敛与下降可同时发生。因此新参考体保完整XY hull×下降高度域，不能称必然的“先水平再垂直”。当前Joint还强制LAND与前NAV同XY；本批解决接受范围漏项，不是修正一条原本异点的编码对角线。

下一步仍须实际动态接受域／真实动作入口与平滑可达域、TAKEOFF／HOLD、采样间连续性、真实地面／执行时序、最终StartPermit与正式远端结果。能力性能、自愈机载改派、企业跨站、长期运营、真实模型／插件生态及原25目标全部保留。整体约40%／剩余60%仅源码工作量粗估。[19路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前：继续P0，不把索引或参数封条当完整执行许可

本轮让原29参数真正贯穿请求入口、逐机阶段检查和最后原生发包内存尾栅；同一actual PM／PRIMARY弱控制块与原snapshot保持，晚回调改参数、ABA或过龄不靠pending=false绕过。原等值回执年龄政策保留，原事务／审批／phase截止不续期。

覆盖求解在完整scene准备后增加严格分离粗筛，两个消费者继续用原完整查询与并集差集。terrain水平覆盖另建全Z索引，不复用有顶面的接触索引；预算／取消与Unknown保持，不宣称功耗或性能已验证。

下一P0仍须忠实的LAND先水平MOVE_TO_LAND再下降等真实动作参考模型、TAKEOFF／HOLD与平滑器／动态接受域、采样间连续性、最终StartPermit及正式机载任务实例／结果。不能用LAND p7的一条名义对角线当完整着陆路径，也不能因原参数当前而改actualPathCertified为true。原25目标及能力性能、自愈机载改派、企业跨站、长期运营、真实模型／插件生态六类仍有实质源码；整体约40%／剩余60%仅管理粗估。[24路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)与[36项未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前：P0观察归属接目标19／20的独立历史报告

同一Orchestrator受保护旧Run已有真实报告消费者，不再仅依赖当前所选启动对象；原compact缺少的完整Run／原生命周期来源不补造。轻目录、私有单记录捕获、原实例双尾核、新报告分型及现面板独立确认／单份复制链已落源码。

这只是本进程缓存历史报告：不等于磁盘归档、长期留存、正式任务结果、远端实例认证或跨站交接。旧full报告／交接／文件接口不放宽，新hash/token也不提供签名、身份或执行权限。原25目标与六类实质剩余保持，约40%／60%仍是源码工作量粗估。

[23路径](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)同步。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：继续P0关联及普通NAV几何，不增加整目标验收数

目标6／12的原普通NAV环境审阅改用XY凸包棱柱连接域；两端完整接受区、全部前后分量组合与独立高度范围保留，减少斜向AABB无关角区。全部实际覆盖与接触消费者沿原Worker链接入，不新增独立未消费helper；真实平滑器／动态高度／采样间运动仍未证明。

原生Run的已知任务序号回退现在结束该机后续关联，保真实原报告、计数、旧候选和MODE事实，不把回退解释为新一轮本任务。其余任务实例认证、正式完成与完整StartPermit仍须开发；不以更严格停止代替这些原目标。

[15路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[32项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[静态审计](handoff/MERIVUS_SWARM_STATIC_AUDIT.md)同步。原25目标及六类实质剩余不缩减；整体约40%／剩余60%仍是源码工作量粗估。未编译、运行模型／helper、测试、仿真、改PX4、硬件／刷机／放行、提交、更新PR或打包。下方为历史。

## 487 当前：目标18→19的实际资料报告消费者

486原页读取与派生摘要进入现有ReportBuilder的独立历史static入口，Archive原review一次生成完整dossier及原31，原UI显示实际聚合数、来源范围和资料缺口。四视角仅本地内容组织，客户可复制子视图不含源文件身份或逐机数值；工程完整卷宗仍含已确认派生数据。

这不是完整事故／任务成果报告：没有任务归属、正式执行结果、事故原因、签名、企业权限或长期报告存储；不写飞前missionReport／实际Run报告，新链不调用旧build；旧view/export/save/load不直接接纳新格式，旧build仍保通用Map输入而非dossier类型拒绝器。[21路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[34项未执行矩阵](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)保这些边界。

原25目标及六类实质剩余保持；整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批路径或视图数上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。 本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前：目标18的实际遥测历史审阅纵切

既有Archive记录格式／Reader不改，增加固定本地读取动作和两个实际面板入口；原最新native完成页经另一次位置派生确认进入MotionReview，公开固定数值摘要而不是raw。只读目录／页、原生游标、单槽双终结及取消保留；历史不回灌live／Replay／任务／权限。

本批补真实历史资料消费，不把相对报值变化解释为漂移、风或故障原因，不认证原点／reset、任务归属、真实性能或完整执行。全历史索引、留存与统计／维护集成、真实预演模型等仍要开发。[25路径清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[32项未执行矩阵](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)给工程师接手。

原25目标及六类实质剩余保持；整体约40%／剩余60%仍仅源码工作量粗估，不按本批文件或入口上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。 仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前：区域规划具备严格性能策略入口

483实际消费者与484声明录入之后，本轮补原区域任务的明确政策选择：新编辑组件→原源set/clear CAS→schema5完整来源／实际task→Compiler／Store一致绑定→现有审阅、能量和规划链。未声明／false／true分别保存，不默认升级旧档。

保存和恢复不只保显示开关：完整editingSource、编译来源、历史／部署恢复、单／组余段政策均不删该flag；新副本仍重新确认datum、重新当前审阅／审批，不恢复机载状态或权限。政策开关不是证据／实测清单／性能认证，也不是关闭其他安全门的开关。

原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。 [本批清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[32项全部未执行场景](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)保留这些边界。本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前：可信性能类补到工程人员可操作的声明入口

483已使严格规划真实消费当前批准数字；本轮继续接实际录入而非再建平行性能helper。原单机面板的第五操作→Root专用值请求→Review独立审批purpose→Registry原生一次CAS→原严格选机／主备／能量链；独立历史摘要和查询筛选也识别该操作。

该功能只采纳已声明的数值，不自行测量、学习或认证。完整当前基线必需，unknown不填默认。后续仍需可信实测性能清单、载荷／气象／温度／老化标定与实物配置真实性；已有局部审批不补成这些证明。Draft.settings的区域严格开关未新增，现有显式task／Intent策略保持。

原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。 [准确范围](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)和[32项全部未执行交接](handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)保留这些边界。本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前：可信能力／性能先打通原工程声明的消费链

复用已存在的原生能力工程审批及完整有类型构型基线，不重造目录／审批。显式 `requirePerformanceEvidence=true` 时，共享exact11为Matcher／RoleAllocator提供当前数字，为Compiler实际主备速度、备用完整路线预算和EnergyScheduler逐成员准入提供同源依据；来源进入计划及审阅身份，不靠标签或任意metadata通过。

这只是“当前已批准的工程声明数字”落到真实消费者，性能录入器、可信实测清单、载荷／电池／环境标定和真实性认证仍要继续。strict估算不是保证续航；旧false／absent规划不升级，旧档和旧批准不恢复权限。P0内部动态状态消息、实际轨迹、完整StartPermit和正式结果仍缺证保false。

原25目标和六类实质剩余不缩减：实际执行闭环、可信能力性能、受控自愈、跨站权限／控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍为粗估，31路径不是完成度。30项工程交接全部未执行；本轮无编译／运行／测试／仿真／硬件／PX4修改／刷机／放行／新PR或包。下方为历史。

## 482 当前：P0从名义航点折线补到静态接受／连接环境模型

新增真实WaypointSwitchReview，以原29参数快照中的水平／垂直接受半径及动作分支，构建普通NAV帽和完整邻接连接外包，真正进入coverage／corridor／terrain／obstacle比较、风险与准入。原29源链贯穿Readiness、两Review、Worker和Witness，JSON codec只恢复材料不授予来源。旧12hash、19键和PM旧口不变。

下一阶段仍须当前controller动态高度接受、实际平滑器／转弯可达域、独立起降与HOLD动作及采样间连续性，再收完整StartPermit和正式结果；不能以静态帽／AABB、新report或COM_OBS_AVOID=0替代这些证据。既有actualPathCertified=false仍阻断。原25目标及可信性能、自愈机载改派、企业跨站、长期运营、真实模型生态等六类实质范围保持。

本批41路径只为源码增量，整体约40%／剩余60%粗估不变。32项工程交接全部未执行；仅静态源码／文档修改与直接阅读，最终文本核对已记录于本批静态审计，无编译、模型／helper／QML／业务执行、测试、仿真、硬件、PX4修改、刷机、放行、新PR或包。下方为历史。

## 481：P0从理想外包转向同一原生整数运行域

新增显式startupSpatialGuard选择；实际运行contains先检查完整整数域，Cover同对象完整投影，原Qt只作额外AND意图条件。完整source、审阅、Worker、Witness与Txn保持原Ptr链，新受控谓词的数值包含不再依赖未知libm误差；旧模型及旧审批不自动升级。

接下来仍须真实起降／航点切换／平滑器的动作运动域、采样间连续性、构型／资料来源、完整StartPermit及正式任务结果。不能把新numeric=true当这些工作已经完成；实际准入仍被actualPathCertified=false阻断。能力性能、自愈机载改派、企业跨站、长期运营和真实模型生态等原25目标全部保留。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480：P0起降参数分支进入真实消费链

固定原12＋2＋14观察及原28行尾核，新增动作参数模型对近地起飞、LAND分段与RC／外部注入形成明确受支持前置。相位报告进入当前审阅与Readiness，不是只多显示参数；变化会使旧身份／审批失效。旧profile内容hash不改。

下一P0仍须完整数值接受域包含、实际动作／阶段与动态净空、最终StartPermit及正式远端结果，不能改true或删pending收口。可信能力／性能、机载受控改派与租约、企业跨站、长期全遥测运营、真实仿真与插件生态六类及原25目标全部保留。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：P0参数合同从12项内容扩到独立覆盖规则消费

原目标6执行链增加两个速率宏的真实原生来源与固定float32覆写比对，原观察不被期望值重写；原12绑定hash继续原合同，新增guard进入完整审阅／阶段配置身份。新PM14内存尾核封在本次所有后续回调之后，不以双capture比较当原源最终仍一致。

这只完成总速覆盖一致性，不把PX4的AUTO／TKO／LAND参数称恒速轨迹。源码已查到TKO ramp、LAND分段／测距／RC辅助、emergency braking、动态acceptance及精降等独立分支，完整动作／动力学／时序与最终许可继续开发。可信性能、自愈机载改派、企业跨站、长期运营及真实模型生态范围不缩减。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478：目标6／12推进编码后边界消费及来源连续性

不再仅保存区域摘要：受约束原外环／孔／退让／参考系进入完整计划，实际编码点按当前INT发送合同整组复核。新single／group提案把完整政策同时绑定到task与plan，已编码起飞／HOLD／LAND及去返连接不漏查。

历史仅保原内容复现，不能借旧无policy记录直接当前上传；新提案仍从完整源重建并重新当前审阅。原整运动域数值包含、动作／阶段／三维动态净空、最终StartPermit、正式远端实例／结果及其它可信性能、自愈执行、跨站、长期运营和真实模型生态仍未完成，不将本批计成整目标验收。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477：原目标6／12新增名义退让的算法与完整消费者

原边界／孔洞的方形支撑边缓冲与Qt布尔生成候选，扫描／图保全部轮廓；原域欧氏点／整段距边检查独立核主／辅助／备用预算与新余段，正值不允许借原outside路径穿外边界。分裂候选按分量与明确每机起返点真实分配，不选最大块丢任务；不足或不可达整份拒绝。

这补的是实际规划算法，不是最终许可。编码E7／float32可能改几何，实际转弯、跟踪包络和动态净空尚未证明；后续P0仍须保完整声明到真实编码／原生门禁、数值域、StartPermit及正式任务结果。可信性能、机载改派、企业跨站、长期运营与真实模型生态仍在原25目标中。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476：原目标6／12补实际带孔覆盖算法及完整路线消费

排除区不再只是一个被拒绝的输入；原扫描、全环可视图、区内／区外独立连接、辅助和返航路线及编译末端检查已接源码。原area仍进入余段新任务，故非覆盖计划也不能绕过本次声明。完整源和几何进入原任务／coveragePlan版本身份。

本项不完成安全退让、动力学、动态障碍或StartPermit。原25目标不缩减，六类实质剩余继续；下一步仍需完整运动／动作许可与时序、正式任务结果和机载控制权／改派，不把名义二维排除区称为自动避障。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475：原目标6／13新增忠实政策的联合人工新计划

完整v2资料包→全原组闭合映射／独立联合provenance→一次多成员当前审阅→显式采用／本地批准已写源码。逐机预算／最低速度各自保留并消费，备用要求与reservePlans不丢；改目标导致原净空声明不对应时仍应阻断，不把旧材料当新证据。

下一步仍是实际机载任务实例／控制权租约／受控改派和正式结果，不能将人工联合新提案称为自动自愈；完整启动许可、原运动数值／动态净空、可信性能、企业跨站、长期运营与真实模型生态继续。原25目标不缩减。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474：原目标6／13的完整组资料来源先接通

完整成员取源、双版本资料包、离线目录和明确选一成员的新人工重规划已写入源码。共同政策／计划不从摘要重建，未MODE提交行原样保留但不可用；旧单成员输入和严格新审阅门禁保持。

下一实质连续性阶段仍需多成员新任务的闭合映射／联合provenance／逐机政策与预算合并及一次联合当前审阅，随后才涉及受控机载改派。不能拼接几个单成员提案冒充全组新任务，也不能借归档或本地审批恢复旧控制权。原25目标及六类剩余完整保留。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：目标6／13补完整历史资料驱动的人工余段提案

[新实际链](../custom/src/Swarm/FleetMissionRemainderArchivePanel.md)让操作员在原Run对象仍在时显式保存完整单成员规划来源，重启后明确选用为新人工重规划；原生18／历史20及provenance28／30不冒名。原政策、包含式航点、完整停留和绝对时序拒绝保持。

这是任务连续性的真实源码增量，不是自动保留全部Run、自动接替／续飞、跨站控制权或正式任务结果。缺完整原来源的旧历史不能从summary猜回，单成员包也不是整组原子观察。原25目标及六类实质开发不缩减，完整StartPermit／数值与动作净空／正式结果等继续；不删除原pending或false。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：目标6补显式编辑保存，不把恢复内容当恢复执行

[新保存链](../custom/src/Swarm/FleetMissionSavedDraftPanel.md)覆盖尚不能buildTask的原生编辑状态；固定本机单槽、完整保留路线／profile／备用／净空声明，重启后必须人工检查并恢复为新草稿，基准与审批重新办理。旧428的已审历史计划恢复保留，两者不是同一功能。

不含未提交文字自动保存、后台持续保存、多版本回溯、跨设备同步或Run／控制权恢复。11源码／登记资源与17文档共28路径是增量，不完成整个目标6或原25目标。完整启动／时序／动态净空／正式结果及其它六类实质代码继续；不删除Compiler pending或numericfalse。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：先固定实际数值模型，不用重新创建的同值对象替代原依据

原Qt完整域研究需要保留同次实际段长／方位角缓存。本批将[原生Models](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)接入两Review→Worker→实际DomainReview→Witness→Txn，并对输入侧容器／文本独立持有；有资料Txn仅核完整原参数与SHA，不重新计算Qt端点距离／方位。

此项消除实际对象对应的不确定来源，不新增浮点误差保证。snapshot／resultMatches的临时值型模型仅用于结构／计数，不能当本次后台实际操作数。原Qt／libm及编译浮点契约、完整数值外包、动作／阶段合法接触、StartPermit／时序／界面与正式机载结果仍需继续；不将对象同一改称数学证明。

原25目标和能力性能、自愈受控执行、企业跨站、长期运营、真实模型与生态六类实质范围保持，整体约40%不按对象、文件或补丁记录编号上调。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：P0环境候选有界化，不削减全量安全判断

[原生空间索引](../custom/src/Swarm/FleetMissionClearanceGeometry.md)与[实际Domain消费者](../custom/src/Swarm/FleetMissionStartupDomainReview.md)已接：两份完整scene索引先建成，完整候选后对同源原凸柱做真实SAT。树只用于证明包围盒分离，不以近邻数量、抽样、命中率或固定前若干项替代完整场景，也不提高原预算。

当前数学场景及报告形状不变，workUnits可因索引准备／排序改变。根节点全包含只给出完整候选，仍不跳过SAT；稠密部分重叠可能更贵，无普遍加速或真实可靠性结论。

继续原Qt数值完整包含、动作／阶段合法接触、完整StartPermit／时序／界面和正式机载结果；可信性能能源、自愈受控执行、企业跨站、长期运营与真实模型／插件生态仍保留原25目标，不缩成只等工程师测试。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469：P0精确求交专用化，不缩小任务目标

[原生闭凸柱SAT](../custom/src/Swarm/FleetMissionClearanceGeometry.md)已进入[实际整域环境计算](../custom/src/Swarm/FleetMissionStartupDomainReview.md)：一次O(n²)凸柱验证／投影准备，随后每对O(n)整数SAT，无采样、epsilon或额外预算。此处只对现有完整轴盒×声明竖柱使用等价算法，不把一般名义扫掠体替成AABB，联合覆盖仍保原精确差集。

这是实际算法／接线推进，不是删除难题：原Qt混合谓词的数值包含、动作范围及合法接触、完整StartPermit／时序／界面与正式任务结果仍须继续；可信能力能源、自愈受控执行、企业跨站、长期运营及真实模型／生态保持原25目标。不因本批整数数学True或理论工作量界限宣称真实可靠性验收。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468：P0整域算法紧化，不替代完整运行谓词证明

新增[五纬度带整数外包](../custom/src/Swarm/FleetMissionStartupDomainCover.md)及[地形覆盖去冗余／报告前缀校验](../custom/src/Swarm/FleetMissionStartupDomainReview.md)，是实际算法和结果接纳修改。没有把完整路线改成当前短段，没有收紧原contains、提高工程余量、删接触候选或增加求解预算来让结果通过。

当前v2仍是理想球面域算法；原运行表达式混用Qt距离与本地半径，即使讨论精确数学也需先处理模型对应，再处理数值误差。默认Qt5.15／CI5.15.2不能证明实际部署库及浮点契约。后续仍须真实数值外包、动作／阶段范围、完整StartPermit和正式机载结果，不能靠改flag或删pending结束P0。

原25目标、可信性能能源、自愈受控执行、企业跨站、长期运营和真实模型／插件生态继续，未缩成只等工程师测试。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467：先补整域真实计算，数值与动作范围证明仍继续

原目标6／12新增[完整理想域环境计算](../custom/src/Swarm/FleetMissionStartupDomainReview.md)，不是增加一个未消费的标志。完整source12与同次Joint／geometryBasis／资料指纹绑定，全组外包、保护及支撑扩张后统一有限求解；实际Worker／Review／Witness／Authority已接入严格result6。

本批交叉审阅修正“同vehicle／terrain的有限起降pad并集”扩大动作豁免的问题。原contains不按当前动作缩小全路线域，不能仅靠segmentIndex关联名义LAND。整域所有确定非空接地交集保留候选；合法起降仍须真正动作／阶段范围证明，旧名义检查保持原豁免。

下一实质工作仍是原Qt混合数值谓词的可靠完整外包及适用域、动作／阶段范围、真实净空与最终StartPermit、完整时序／启动界面及正式机载实例结果，不能删除Compiler pending。可信性能能源、自愈机载接替、企业跨站、长期运营和真实模型／插件生态继续；不缩减原25目标。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466：长期运营增加可追查的单段历史导入

从当前原生诊断readable候选，经[同一维护链](../custom/src/Swarm/FleetPersistentEventStore.md)明确预览和确认后导入既有本地retired区；先暂存、完整读回，再无覆盖发布，保留取消及不确定事实。新文件不会自动进入普通查询或live任务，需再走既有库存／恢复确认。

这是原目标18的真实源码增量，仍非完整备份集、灾后恢复、授权恢复或整目标验收。暂存处置／恢复、全遥测与运营维护、完整启动准入和正式结果及其余六类工作继续，不按批次或文件数上调进度。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465：长期运营增加混合候选的可解释诊断

[同一维护链](../custom/src/Swarm/FleetPersistentEventStore.md)新增显式backup-diagnostics，可在原归档不可用时检查明确目录，逐项显示本次可读或被拒绝。完成只表示本次完整候选观察，不要求所有项可读，也不等于全备份或灾后恢复；旧strict catalog遇坏项仍整次失败。
此增量推进原目标18的运营诊断，不新增整目标完成数。完整运动域／启动许可和正式结果、可信性能能源、自愈机载接替、企业跨站、长期归档恢复及真实模型生态继续，不能缩成只等工程师测试。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：独立副本清点为后续恢复取证，不执行恢复

新增[backup-catalog](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)在原源不可用时，仍可对明确现存目录重新枚举／hash／SQL身份检查并只读显示。该目录既不恢复原生库存，也不恢复preview／任务／批准；当前观察副本不等于463同源copy工单的已验证副本。

后续仍须带逐文件错误诊断的备份管理、独立取证／受控导入、恢复冲突政策、完整保留／清理／长期统计及灾后恢复链。当前坏末项／partial整体拒收的保守清点还不是完整备份运营平台；原25目标及完整运动域数值包含／StartPermit／正式结果等六类实质范围不变。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：先做可核对单段副本，完整备份与恢复仍继续

新增[备份工单](../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)与原维护UI／Store来源绑定，允许用户明确把原封存段写到已有目标目录；源保持不变。完整目标名来自原预览UUID，不接任意文件名，NewOnly拒绝覆盖；有界复制后重新打开完整SHA和只读SQL身份核对，再复核原源。

单个读回一致的副本不等于备份介质独立、全部历史完整、签名、长久保存或灾后恢复成功。后续仍须备份目录管理、独立验证／恢复演练源码、留存回收与运营统计，不自动删原段或partial。原25目标和六类实质开发完整保留，尤其全运动域数值包含／最终StartPermit与正式机载实例结果不能被本批替代。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：归档单段退役／恢复，不是长期留存全功能完成

[实际维护链](../custom/src/Swarm/FleetArchiveMaintenancePanel.md)增加固定目录真实库存与原生预览，单段退出／回到普通封存检索范围；移动不覆盖、不永久删除，完成与不确定结果分开。原库存、目录和任务记录续扫在movement尝试前退休，不能把旧快照当当前文件状态。

后续仍需外部可验证备份、部署留存策略、明确授权的磁盘回收／永久清理、断电与恶意并发恢复及长期容量统计；本次不以改名冒充空间释放或全历史完整。原25目标保持：完整运动域及StartPermit／正式任务结果、可信性能能源、自愈、企业跨站与真实模型／插件生态仍有实质代码工作。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：先把原保留任务诊断做成可追查目录

[任务历史](../custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)消费真实留存taskIdHash；同固定清单下累计有界诊断目录，再按实际存在的哈希查看原记录。只推进长期运营中的历史组织，不把它改称正式任务结果、全集数据库或长期运营平台完成。

后续仍须完整运动域净空与最终StartPermit、正式机载实例／结果、可信性能能源、实际自愈接替、企业跨站、长期保留／清理／灾后恢复及真实模型／插件生态。历史summary已丢失的完成／ROI不能由新索引补造，NDJSON runId也不能按时间或机号强行关联。原25目标不缩减。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：先让明确电池异常贯穿实际准入，性能认证仍保留

[原生电池](../custom/src/Swarm/FleetNativeBatteryObservation.md)与[共享检查器](../custom/src/Swarm/FleetNativeBatteryEvidence.md)把故障位和charge state接入真实来源、主备能量账单、健康门禁、原审批身份和整组发送尾核。正常值／缺扩展不是物理健康证明，静态endurance不是实测续航；新规则仅负面降级／阻断，不新增返航或停机命令。

原25目标仍完整。下一步继续可信性能及模型来源、完整运行任务／StartPermit／正式结果、全域数值外包／真实净空、自愈机载改派和企业跨站。长期运营已有taskIdHash留存链，但缺累计任务目录／按任务下钻，可继续只读本机固定范围诊断索引，不能据丢失summary的旧记录猜完成率／ROI，也不能把NDJSON runId按机号或时间强行JOIN。

原Qt全运动域外包仍待可靠数值合同；FE_TONEAREST不足以排除FTZ／DAZ／fast-math，不能对当前高度表达式用不等价代数替代或默加epsilon。11源码／19文档是本批增量，不增加验收计数。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：先让后续全域报告对应同一个实际运行模型

[Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)／[Txn](../custom/src/Swarm/FleetMissionStartupTransaction.md)原生传递完整同源不可变模型，实际Transition不在每次phase重新prepare。原策略与全域范围不缩，旧sourceFingerprint已绑定GPI，不将本优化描述为已证明漏洞修复。

完整运动域下一步仍须可靠数值外包：现横向projection误差只声明工程映射，不是Qt／libm误差合同；资料maximumDelta不是runtime geofence，LOCAL／ODOM／HOME无可信共同XY，速度与短阶段时限也不能从原走廊中截掉远路段。实际全水平域必须独立挤出原完整AMSL区间，不能伪作带插值高度的名义段或向sweptBox塞超限corridor。全域protected／support不得继承名义TAKEOFF／LAND有限pad的接触豁免。

继续完整域报告与native消费者、完整StartPermit、正式机载实例／结果、可信性能能源、自愈执行、企业跨站、长期运营及真实模型／插件生态。原25目标不缩减，本批5源码／16文档只是增量。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：先固定真实运动规则和原输入，不替代全域包含证明

[GeographicMotion](../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)共享实际Transition数值规则并一次准备顺序地理链。两类Review→[Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)→既有原生Witness保完整原source12及输入hash；源描述明确numericContainmentProven=false，不与名义段报告混同。

下一实质竖切仍是整个原球面水平走廊并集×独立AMSL区间，加完整保护机体／跟踪／资料／投影误差，保守包含于实际检查场景；必须证明Qt球面表达式到工程affine图表的数值包络及适用域，不能用端点AABB、未经证明的角度误差、LOCAL／ODOM或单次nextafter冒充证明。起飞垂直段、起飞连接和当前位置到首航点三个独立假设仍分开，TAKEOFF／LAND的指定支承接触例外不能扩散到整个运动域。

随后补完整StartPermit、正式机载实例／结果、可信性能能源、实际自愈执行、企业跨站、长期运营和真实模型／插件生态；不删除原Compiler pending。原25目标不缩减，本批8源码／18文档只是增量。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：原包历史与完整阶段判定先封同一整组内存依据

[RAW入口修复](../src/Vehicle/VehicleStartupRawIngress.md)先原包／收到时刻／原journal operation／depth，后任何观察回调；不将原包归新步骤，不丢closed屏障历史。新[原生seal](../src/Vehicle/FleetStartupMemorySeal.md)和Txn整组尾栅覆盖原typed账本／receipt／raw序列／来源及自然过期，不从最新健康包恢复旧阶段判定。所有回调后查原集合，最终命令入口不新capture；新prepared步骤必须完整重核后才能使用新seal。

下一实质竖切仍须证明整个Transition允许运动域＋保护机体／跟踪误差包络包含于实际已检查空间，保持起飞垂直段、起飞连接及当前位置到首航点三个独立假设；不能用瞬时GPI点、独立高度slab或未证明坐标关系的LOCAL界限代替。Compiler pending不删；随后是完整StartPermit、正式机载实例／结果、可信性能能源、真实自愈执行、企业跨站、长期运营及真实模型／生态。原25目标不缩减，10源码／17文档只是本批增量。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 456：独立持有原审阅依据，下一步补完整运动连续性与包含关系

[原生Witness](../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)进入两类Review→Authority→StartupTransaction，同ptr／原typed材料／原Review时钟和共享UTC水位直至既有Vehicle最终commandContextCurrent；无callback memory尾核不重solver，移交不续期，不依赖旧Review后续活性。caller fence失效不是物理源失效，实际配置／目录／VERSION／PRIMARY／原点／时效变化则永久退休。

不删除Compiler pending，不把原报告成功当瞬时或连续运动许可。下一完整竖切须先在普通RAW任何可回调观察前冻结原scope／step／收到时刻，确保原包不被归入回调中新步骤；随后补全组motion／receipt／EST／原点／ALT最终无回调尾栅，并证明整个Transition允许运动域及保护机体／误差包络包含于实际已检查空间。球面走廊并集与独立AMSL slab不能凭瞬时GPI检查或无坐标关系的LOCAL界限替代。

之后继续完整StartPermit、正式远端任务实例／结果、可信性能能源、实际自愈机载改派、企业跨站、长期运营及真实模型生态。原25目标不缩减；13源码／19文档只是本批增量。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：同次启动报告先接实际审阅，完整准入竖切未结束

[Worker](../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)及两类原Review已接冻结输入、后台计算、原生完成复核与必需报告摘要，不仅是独立helper。首份风险及v4样本不重基，原期限不续期，取消后实际finished才复用；报告构造／范围／计算／接地／模型结论分开。

下一步仍须同一原报告／source18／datum14及UTC水位从两Authority独立移交Transaction，贯穿Transition和最后Vehicle write。现有允许运动的球面走廊／高程区间与solved trackingError／工程localFrame未证明包含，必须约束或保守证明；不能仅增加report布尔门槛。只有这些实际消费者齐备，才区分planning／upload与startup义务，不能直接删Compiler pending或放宽通用risk gate。正式机载结果、可信性能能源、自愈、企业跨站、长期运营及真实模型／插件生态继续。

11源码／18文档只是本批局部范围。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：高程对应的工程声明与实际来源绑定

[目录2](../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)与[Datum取源](../custom/src/Swarm/FleetMissionClearanceDatum.md)进入原普通／地面审阅；不是只增加一个未消费的flag。关系必须明确场地与文档版本、垂直datum、原JSON配置hash、原点参考和固件tuple。当前只实现同datum／显式零偏移工程关系，非零需真实转换，不因填写offset字段就按零算；未认证具体geoid、当前PX4内部全球高度分支或共同传感器采样。

下一完整竖切：同原记录／完整payload／同次Readiness冻结pre-artifact geometry basis，独立单任务可取消worker计算真实启动净空；保持原READ／审阅／账号期限，实际finished后核同源才hash并发布。区分规划义务与启动义务，让合法规划可以进入受控地面部署，但启动必须等本次结果，不能直接删pending、借旧planning报告或GUI缓存。原两Authority、Transaction／Transition及最后Vehicle write要消费原报告／来源和已审运动范围，不能在每次fence重跑solver。

正式机载结果、可信性能能源、真实自愈改派、企业跨站、长期运营与模型／插件生态仍有实质代码；原25目标不缩减。11源码／17文档不是验收计数。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 以下453为历史。

## 453：真实配置对应与当前资料期限进入原审阅来源

[原生配置绑定](../custom/src/Swarm/FleetRegistry.md)和[共享观察](../custom/src/Swarm/FleetNativeConfigurationObservation.md)把输入中的配置hash对应到实际原Vehicle／来源、本地完整构型和批准基线，不混用startupConfigurationFingerprint。新的[净空来源检查](../custom/src/Swarm/FleetMissionClearanceSource.md)由普通／地面审阅实际消费；固定资料身份与原单调期限，不接受过期资料或通过重捕延期，未带资料的原任务不自动补造输入。

下一完整竖切继续：明确工程datum与实际高度参考关系；同记录／完整payload／同次原生Readiness冻结独立basis，单任务可取消worker计算真实启动报告，完成后不续原时限；两条Authority、Transaction／Transition到最后Vehicle write消费原依据及已审运动包线。不能在重复capture／审批／发送fence重跑solver，不能因本批来源检查删除clearance pending。随后仍有正式结果、可信性能能源、真实自愈执行、企业跨站、长期运营和模型／插件生态代码。

15源码／16文档不是产品验收计数；本批只优化地面站源码及交接材料；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余不缩减。 以下452为历史。

## 452：明确接地来源与双组件模型进入原规划链

[ContactModel](../custom/src/Swarm/FleetMissionContactModel.md)绑定完整payload／真实序号与声明场地，区分不可接触body／rotor及support，保原wire。LAND用独立声明面＋支承偏移的参考终点，p7只核显式请求一致性；起飞不移动原vertical.from，current-first假设独立检查。完整模型不是跳过最后若干米，support与某地形的整个交集只有完全包含于有限pad高度带才可接触，障碍和body不豁免。原实际Compiler／Store消费者已使用报告3，旧资料／报告不自动升级。

下一实质竖切仍须实际原生构型对应（不是startupConfigurationFingerprint冒充Registry配置hash）、工程datum与实际高度参考的明确对应、当前UTC和不可续期的单调资料期限；同记录／payload／同次Readiness产生独立basis后计算报告，纳入最终artifact再hash，两条Authority／Transaction／Transition及最终Vehicle write消费同一依据，允许运动包线不得越已检查空间。实际启动新重计算还必须保持451线程隔离／取消／旧源退休，不把2M solver重新塞GUI。

配套多旋翼PX4 LAND使用速度下降与landed，不承诺按p7停止；纯接触声明不能解除原pending。完整正式结果、可信性能、自愈机载改派、企业跨站、长期运营及真实模型／生态继续。原25目标保持，9源码／18文档不是验收计数；未编译、运行、测试、放行、更新PR或打包。以下451为历史。

## 451：先完成真实规划入口的单任务后台与取消

[新Worker](../custom/src/Swarm/FleetMissionPlanningWorker.md)承载完整纯值compileCancelable；Geometry／Review协作取消，Coordinator真实finished后才ready，Root重新核原代次并显式完成。旧同步Coordinator不能借clearanceInput重跑重型solver。Draft、余段、AI都接最终回调，输入保留，取消和来源过期不恢复批准。原生参数／版本只在GUI完成阶段捕获，不搬线程，不借processEvents。

仍须继续下一完整净空竖切：版本化接地上下文，LAND地面请求与机体参考点分开；起落架允许接触与保护机体／旋翼包络分开；核实际构型、datum对应和UTC／单调资料期限；同记录／同次Readiness报告进入artifact再hash，两条Authority／Transaction／Transition与最终write消费同一依据，原允许运动不能越已检查空间。不能只删pending或把规划报告当启动许可。

451不代表所有GUI工作均非阻塞，原complete中能量／空域／风险仍在GUI，取消无固定时延保证。完整正式结果、可信性能、自愈机载改派、企业跨站、长期运营和真实模型／生态继续，原25项目标不缩减。15源码／17文档只是本批范围，未编译、运行、测试、放行、更新PR或打包；下方450为历史。

## 450：规划范围的真实结果与无循环几何来源

[新scope producer](../custom/src/Swarm/FleetMissionJointSpatialReview.md)和[独立startup basis](../custom/src/Swarm/FleetMissionStartupSpatialReview.md)保原最终审阅显示合同；规划范围不构造任何启动替身，来源提取不执行额外旧空间checker。规划basis来自已验证几何投影而非含报告的整个plan，避免报告保存后改变自身来源。[报告2](../custom/src/Swarm/FleetMissionClearanceReview.md)明确此范围，Compiler实际求解并冻结，PlanStore独立核完整来源而不重算几何。

本轮没有解除原pending或新增启动许可。下一完整竖切仍须：明确LAND原地面请求与落稳机体参考点，区分可接触起落架和受保护机体／旋翼；实际构型／高程对应与资料期限；同次原生Readiness的报告进入artifact再hash，两条Authority／Transaction／Transition和最终write消费同一依据，且原允许运动包线不能越已检查空间。不能只把schema2报告放进界面或把纯basis当认证。

在新增启动准入之前，优先把当前显式QML准备路径的同步2M纯计算迁到有取消点的单任务worker；GUI只冻结原生源、接收后核原代次再发布，不能把Vehicle／认证等原生捕获整段搬线程或processEvents。当前无异步／取消源码，不应称界面响应已解决。

原25目标和其余六类实质工作保持，9源码／17文档不是整目标验收计数。未编译、测试、运行或放行；下方449及更早为历史。

## 449：实现真实整段／并集计算，不把独立helper当完整净空竖切

[有界精确几何](../custom/src/Swarm/FleetMissionClearanceGeometry.md)与[联合段声明模型报告](../custom/src/Swarm/FleetMissionClearanceReview.md)覆盖整个机体／误差箱扫掠，包含场地和走廊的完整闭并集、地形范围／上界及障碍；保留共享边界、非零毫米量化外包和未知资源结果。不是仅逐点、仅AABB或仅单个缩小区域的替代算法。

449不删448的pending，也未接审批或UI。下一实质竖切仍须区分规划与实际启动范围，增加明确接地上下文；同记录／同次原生Readiness生成报告后进入有界artifact再hash，审批／Authority／实际发送明确消费，不借UI缓存或循环hash。还要核当前资料时效、实际构型和高程对应，不能把纯值报告当作这些证据。

特别保留两个实际设计约束：规划pending先挡住Coordinator和StartReview原记录，故不能等启动阶段才首次消除它；现Joint.reviewFingerprint又来自最终artifact，不能原样塞回自身。下一阶段须有独立geometryBasis身份、明确规划／实际启动两种范围与原生强制消费者，并将支承接触和不可接触机体／旋翼分开建模，不能假造startup源或忽略整段地形。

最终StartPermit、正式机载结果、可信性能能耗、自愈、企业跨站、长期运营与真实模型生态继续；原25目标保持，5源码／15文档不增加整目标验收计数。未编译、测试、运行或放行；下方448及更早为历史。

## 448：净空资料协议及无损来源接线，不缩减下一完整求解

[场地与逐机资料](../custom/src/Swarm/FleetMissionClearanceInput.md)采用有界毫米凸区域／明确高程／工程局部图表，进入[Draft来源2](../custom/src/Swarm/FleetMissionDraft.md)、完整Compiler task／plan和准确归档；来源1不补默认。当前配置指纹、日期、datum声明只是资料，未认证实际构型、时效或现场。新资料任务明确几何待审阻断，不把输入合同当净空产品完成。

下一竖切必须实际实现：完整canonical任务／返程／启动／终结段的机体与跟踪误差扫掠范围，对覆盖、障碍、保守最高地形和允许走廊作全段／并集检查；起飞接地和LAND接触需独立绑定的明确上下文，RTL缺路径不能补猜。先区分规划阶段与真实启动阶段所具备的几何，使规划检查可支持地面部署而不冒称启动净空已通过；后续实际StartReview／GroundReview以同记录／同次Readiness产生新有界结果再hash，原审批／Authority／实际发送链明确消费，不能永久保pending、只删原因或借UI缓存。

仍需真实构型对应、资料时效／高程对齐和工程适用域；最终StartPermit、正式机载结果、可信性能能源、自愈执行、企业跨站、长期运营和真实模型生态继续，原25目标不变。8源码／17文档仅源码及限定静态核对，未编译、测试、运行或放行，下方447及更早为历史。

## 447：先补齐实际风险阻断消费者，净空完整竖切继续

[共享无阻断检查](../custom/src/Swarm/FleetMissionRiskEvaluator.md)进入Coordinator的needsReview、Deployment当前及归档评估核对、普通／地面启动准确记录门禁。输入语法正确不再掩盖业务阻断；未命名严重风险以统一code进入原因列表。空QStringList和JSON空List兼容，非空／缺项／坏型不能由人工标签清除。

这解决446指出的“仅附blocking信号不足以阻断”的既有漏接，但没有实现工程场景与机体扫掠净空。下一完整竖切仍包括版本化WGS84／明确AMSL基准场景、覆盖／障碍／保守最高地形／允许走廊、逐机体积与跟踪／资料误差、整段走廊并集覆盖和LAND接地边界；同次StartReview／GroundReview生成的结果必须进入原artifact再hash，并被实际审批／Authority消费，不能复用UI点击缓存或循环hash。

原25目标、完整执行与最终许可／正式结果、可信性能能源、机载自愈、企业跨站、长期运营与真实模型生态继续。仅6源码／16文档及限定静态核对，不按文件数上调验收率；未编译、测试、运行或放行，下方446及更早为历史。

## 446：目标6的精确容差门槛与下一净空竖切

[精确算术](../custom/src/Swarm/FleetMissionAltitudeArithmetic.md)接入原v4各实际消费者，消除Double中间差值／毫米除法的信息丢失，不改政策、原基准和原生scope。派生显示值继续保留，闭区间和0按真实保存的binary64容差与原报告数值关系判断；数学推导不是运行验证。

净空正向实现仍需完整竖切：版本化工程场地覆盖／障碍／地形／允许走廊资料，逐机机体尺寸与定位／跟踪误差，完整名义段扫掠范围，及同次StartReview／GroundReview材料到批准／实际发送的固定来源消费。不能只扩noFlyZones或再添false占位；Coordinator的needsReview必须显式消费required结果，不能仅附blocking风险信号。LAND接地政策、RTL缺路径和未编码启动假设保留缺口，不把受控静态场地模型称任意城市环境保证。

原25目标、完整最终许可／正式结果、可信性能能源、机载自愈、企业跨站、长期运营和真实模型生态继续；不按本批文件数增加验收率。未编译、测试、运行或放行，下方445及更早为历史。

## 445：目标6新增明确选择的高度关系连续约束

[新纯契约](../custom/src/Swarm/FleetMissionStartupAltitude.md)与[第四版政策](../custom/src/Swarm/FleetMissionExecutionProfile.md)把“原包诊断”推进为明确容差下的报文一致性条件；[输入表单](../custom/src/Swarm/FleetStartupPolicyEditor.md)不补默认值。第一次审阅冻结的基准与实际发送scope内的不可自行清除违规记录相配合，不能拿最新恢复值掩盖越界，也不能把全部动态高度加入审批身份导致正常更新反复失效。

本批不完成可信内部参考、真实净空、最终StartPermit、权威4D时序或正式远端结果；scope建立前无连续覆盖承诺。后续仍按完整受控执行、可信性能能源、机载自愈、跨站控权、长期运营、真实模型与生态推进，原25目标不缩减、不因新增策略或文件数上调整体验收率。未编译、测试、运行或放行；下方444及更早为历史。

## 444：目标6补原始高度诊断，尚非参考有效性或启动许可

[ALTITUDE原始观察](../src/Vehicle/VehicleStartupAltitude.md)补足现有显示Fact缺少的同包六字段、原始位值、接收来源和时效；[启动风险](../custom/src/Swarm/FleetMissionStartReadiness.md)保留当时快照和明确未证明项。接收鲜度、发送时钟推进、数值有限与真实估计器样本新鲜是不同概念；不能据其完成共同参考证明。

原v1–v3实际准入和发送门槛不变，动态诊断不污染阶段／审批风险身份，初次完整材料不被重捕值覆盖。下一实质切点仍是显式可信参考／偏差容差政策、净空与最终StartPermit，正式机载结果及原25目标其他五类工作保持；不因新账本或文档数量上调验收状态。仅源码／静态核对，未编译、测试、运行或放行。下方443及更早为历史。


## 443：目标6补原点声明连续性，不替代共同参考证明

[被动原点账本](../src/Vehicle/VehicleStartupGlobalOrigin.md)与[阶段参考契约](../custom/src/Swarm/FleetMissionStartupGlobalOrigin.md)接入真实启动路径：原点声明／timestamp／来源变化和已观察后失源退休旧依据，地面READ期间不能偷偷重基；原生send scope第一次冻结后贯穿后续步骤及最终write。真正从未观察保持原兼容路径，不对事件型消息加周期TTL。

仍需实现可信HOME／local／估计器参考和有效分支证据、净空政策、完整最终StartPermit及正式机载结果。ALTITUDE有限数字不能代替其隐藏的内部有效分支；本批没有把这些未知标成已认证。真实性能能耗、实际自愈接替、跨站控权、长期运营和真实模型／生态仍有实质代码，原25目标不缩减。仅源码与静态核对，未编译、测试、运行或放行；下方442及更早为历史。


## 442：目标6／12补全请求链、启动参考与终结的共同名义几何

[联合报告](../custom/src/Swarm/FleetMissionJointSpatialReview.md)以原整plan已生成的canonical协议项为主干，保出航／返程连接、TAKEOFF连接、HOLD锚点和LAND下降；三种启动参考独立、不拼接实际航迹。只对同记录的区域与原实际间距重做有界静态比较，无法表示的来源、参考、动作或预算明确不完整。

此批补441之后的联合名义几何缺口，不完成正式4D时序、真实空域数据、地形／障碍／动力学净空、可信原点或最终StartPermit。后续继续可信参考与动态／净空来源、受控启动最终许可、正式远端实例与结果闭环；能力性能、耗能、自愈、企业跨站、长期运营和真实模型生态仍须实质开发。原25目标不缩减，未编译、测试、运行或放行，不新增整目标验收计数；下方441及更早为历史。

## 441：目标6／11／12的原始空域输入门禁

440记录输入形状但尚未成为独立风险门禁的缺口由[本批实际接线](../custom/src/Swarm/FleetMissionAirspaceInputGate.md)补上：Root不折叠坏值，原生比较task／参数来源，Checker坏输入和间距发布不完整阻断；Intent当前私有输入及归档前／后有效来源指纹一起核对。安全坏JSON仍可保存明确needsReview诊断，不能获得审批；超限／不可表示原值无新归档。

下一主线是完整原路线／返程、三类独立启动参考及终结动作对同一冻结区域的有界联合空间报告，不拼接假设、不补造时序或认证净空。最终许可／正式结果、真实性能能源、自愈、跨站、长期运营与真实模型生态仍有实质代码；原25目标不缩减。本批未编译、测试、运行或放行，不增加整目标验收计数；下方440及更早为历史。

## 440：目标6／12增加完整任务请求与原空域报告共同查看

原记录全成员出航、返程、profile起止动作和完整本地协议列表进入[共同冻结视图](../custom/src/Swarm/FleetMissionSpatialReview.md)，[界面](../custom/src/Swarm/FleetMissionSpatialPanel.md)明确区分规划来源、生成请求和未编码语义。一次原整plan生成失败不以逐机成功掩盖，旧启动假设仍独立。

这一批补原计划显示与归档来源解释，不将旧空域报告扩大成含起飞／终结动作的新联合检查；新形状证据也不认证真实空域来源，尚未独立成为风险／执行门禁。下一源码工作须将原始坏型空域输入明确阻断而非由旧归一化折叠为空，再接全路线／独立启动段／终结动作的联合审查。最终许可／净空、正式结果、真实性能、自愈、跨站、长期运营及真实模型生态仍有实质代码。原25目标保持，未编译、测试、运行或放行，不按文件／批次上调完成率；下方439及更早为历史。

## 439：目标6／12补冻结起飞空间与名义间距候选

[实际风险空间投影](../custom/src/Swarm/FleetMissionStartupSpatialReview.md)与[显式查看入口](../custom/src/Swarm/FleetMissionStartupSpatialPanel.md)覆盖三种独立启动几何，原生来源／记录与地图共用冻结token。新增无状态名义间距候选，不修改旧checker，不补造时间或使用默认间距。

本批不是完整4D、障碍净空或实际飞行路径认证；原完整路线／返程／结束动作与空域共同报告、最终StartPermit／可信原点和正式结果继续。可信性能能耗、自愈改派、跨站、长期运营、真实模型生态仍有实质代码，原25目标保持。未编译、运行、测试或放行，不按文件／批次上调完成率；下方438及更早为历史。

## 438：目标6补完整启动策略表单与真实编辑来源

原416完整profile编辑器增加[结构化启动政策](../custom/src/Swarm/FleetStartupPolicyEditor.md)：原schema1／2／3全部字段，不默认补数，不隐式升级；原JSON严格加载、明确采用与单一规范文本保存。父编辑器和Root补真实草稿选择／backing Draft核对，保留未提交输入而阻断错源保存。

这是规划输入到原生校验链的源码增量，不增加整目标验收计数。最终StartPermit、可信HOME／原点／净空、正式任务结果、真实性能能耗、自愈、跨站、长期运营与真实模型生态继续，原25目标不缩减。未编译、测试、运行或放行；下方437及更早为历史。

## 437：目标6／13补实际启动与地面再审阅界面

[启动工作流](../custom/src/Swarm/FleetMissionStartupWorkbench.md)串联原有实际后端的人工单步入口；新的统一投影固定当前Episode、身份、部署和逐机步骤，原倒计时只显示不续期。地面新风险有完整只读展示，后续新审批／新authority／独立consumer仍由原生服务决定。

此批补齐局部UI消费，不完成最终StartPermit、可信原点／起降净空、机载结果认证或正式自愈改派。可信性能／能耗、企业跨站、长期运营和真实模型／平台生态仍有实质代码工作，原25目标保持，不据文件／批次号上调总体完成率。未编译、测试、运行或放行；下方436及更早为历史。

## 436：目标15增加真实Run的本地人员简报交接

[实际取源与冻结确认](../custom/src/Swarm/FleetMissionHandoffWorkflow.md)替换自由任务ID和最新提案引用，接通有限运行简报、接收SID、每页4条收件箱及终态查看。接收表示读到该历史简报，不表示任务成功或接管飞机；旧Run可以成为历史材料，但不是新执行许可。

该批不是跨地面站控制权交接。原目标15中的企业身份／同步／控制租约／中途加入和其他六类实质工作仍需开发，原25目标不缩减；没有整项目标验收计数增加。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包。下方435及更早为历史。

## 435：目标6／13补新余段计划的空间与逐项审阅

[新记录显示链](../custom/src/Swarm/FleetMissionRemainderView.md)把实际单成员计划及原编码器明细接到地图和分页只读界面；只有明确采用才切换地图规划来源。地图、旧Run观察和真实机载任务仍是不同对象，过期记录不自动复活审批。

本批不新增整项目标验收计数，不按文件／批次数推完成率。下一实质工作仍含可信HOME／原点与净空、完整受控启动及正式结果、性能能耗模型、实际自愈接替、跨站、长期运营和模型／插件生态。原25目标保持；未编译、测试、运行或更新PR／包，下方434及更早为历史。

## 434：目标6／13补独立余段记录的人工批准与地面部署入口

[余段新审阅](../custom/src/Swarm/FleetMissionRemainderPanel.md)不再依赖区域Draft revision；采用记录后，精确绑定新归档、输入／审阅代次和当前部署组，复用原意图审批、独立账号上传批准与地面上传／读回界面。唯一部署面板不会因余段过期自动回退旧区域计划，证据刷新目标也跟随明确来源。

这是地面重新准备新任务的源码通路，不是空中接管、自愈完成、原Run取消或旧占用释放。原25目标和六类实质剩余工作保持；完整启动许可／净空／正式结果等继续开发。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，不按批次号上调完成率。下方433及更早为历史。

## 433：目标6／9／13增加人工余段重做提案

[取源](../custom/src/Swarm/FleetMissionRemainderSource.md)冻结原实际任务政策、计划和机载载荷映射；[规划](../custom/src/Swarm/FleetMissionRemainderPlanner.md)从人工明确的原航点索引开始，完整重做所选点及后缀，并使用目标机自身端点/速度与新profile。新[入口](../custom/src/Swarm/FleetMissionRemainderPanel.md)复用当前证据审阅和版本归档，不恢复旧区域编辑源或旧批准。

只补“人工选段→新提案”这条代码链，不以遥测序号认定连续完成，不自动省略停留、不改变旧Run或其他成员。实际自愈/机载替补、可信耗能与净空、完整时序/执行结果等仍属六类实质开发。原25目标不缩减，不按批次或路径数增加完成率；未编译、测试、运行或放行。下方432及更早为历史，原“没有余段提案”仅由本批限定能力补充。

## 432：目标9加入备用独立待命与完整航线重做估算

[独立预算](../custom/src/Swarm/FleetReserveEnergyPlan.md)与[操作入口](../custom/src/Swarm/FleetReserveEnergyPlanPanel.md)补入原实际草稿／编译／审阅链。原备用ID不再借最长主机路线；自身起返点、明确接替对象、待命时长／系数及自身速度成为可审阅输入。未知预算拒绝该备用，保留主机账单；几何容量超限不接受完整组的有利前缀。

这是目标9的源码增量，不完成实际接替或能耗模型。后续仍须补中断点／剩余航段语义、真实待命／风／载荷／老化耗能、返航／起降净空和正式机载改派；原25目标及六类实质工作保持，不按批次或文件数计算完成率。未编译、测试、运行或放行。下方431及更早为历史，备用预算是否已提供以本节和当前源码为准。

## 431：目标5／9接入实际电池报告，仍非性能模型完成

现有能量账单新增[原始BATTERY_STATUS→Registry→审阅链](../custom/src/Swarm/FleetNativeBatteryBridge.md)。本地接收年龄、实际来源、单推进电池用途与未知状态进入动态快照；飞控剩余时间报告扣除本地已过时间后用于预算。辅助电池不平均；多推进或用途未知不套用单电池模型。当前百分比与原静态满电续航仍可组合估算，但声明不因此变成实测性能。

本批不包括耗能标定、风／载荷／温度／老化、真实多电池拓扑、备用待命与接替路线，也不增加自动调度或飞行许可。原25目标范围及六类实质开发不变；不能按批次号、文件数或基础类数计算完成率。未编译、测试、运行或放行。以下430及更早是历史，旧“未接实际电池遥测”由本批有限来源链覆盖。

## 430：目标9从组级建议推进到原成员逐机预算

[逐机账单](../custom/src/Swarm/FleetEnergyScheduler.md)从实际编译主／备用身份生成并进入风险审阅及[规划页明细](../custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)。按各自航线／停留／返程资料核算，共同任务时间下限仍保留；原主机不合格不能靠备用机排序掩盖。可用时间按有效模型最小值，百分比预测按较大耗电率；仅直接剩余秒数时预测百分比未知。

这不是可信实测性能、电池标定或接替执行：备用缺独立路线和待命耗能，真实遥测新鲜度及风／载荷／老化模型仍需开发。目标8／11的消费与原生 unknown 标记同步，但不增加整项目标验收数。原25目标、六类实质剩余工作保持；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方旧状态按本节限定增量补充。

## 429：目标19接入实际运行观察

本阶段交付源码层的[运行报告](../custom/src/Swarm/FleetMissionExecutionReport.md)：实际选定Run采集、冻结、逐机展示、确认导出及外部文件只读打开。它不是把原计划报告重新命名，也不把结束观察当作完成任务。

目标19仍缺正式机载任务实例／结果来源、跨日志和视频的完整关联以及面向业务事件的完整报告。原25目标不缩减；目标6最终执行条件、可信能力／能耗、实际自愈改派、跨站控制交接、长期运营治理和真实适配生态继续开发。当前只作源码与限定静态核对，不代表编译、运行、测试或工程放行。

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

## 418：能力声明的工程操作闭环，不替代真实性认证

目标1–4/6/8/13/20的前置增加[原生双账号能力审阅](../custom/src/Swarm/FleetCapabilityReview.md)和[实际界面](../custom/src/Swarm/FleetCapabilityReviewPanel.md)：冻结单机条目、明确采纳或撤销、独立账号及最后原生版本核对，提交事实与后置审计分离。下游继续消费原有效本地声明，不新增自动飞行或基线批准。

机号/UID/指纹和账号批准都不证明实物或实测速度/续航；基线原配置不完整时不能生成具体改装差异。企业身份/跨站控权、真实硬件与性能来源、完整StartPermit/可信原点净空/正式机载结果、自愈、长期运营与真实模型生态仍需开发。25目标未缩减，不按本批文件或阶段号算完成率。仅源码及限定静态阅读，未编译/测试/运行或放行。

## 417：补运行中工程声明目录入口，不缩减整体路线图

[固定文件重载](../custom/src/Swarm/FleetFirmwareCatalogReload.h)把完整预览、原旧/新工程声明、差异/双哈希及独立两账号一次审批接入真实 Coordinator/Guided/双入口面板。固定 AppData 档案只读；实际目录 epoch/revision/CAS、原 scope、30秒与原身份期限、最终文件复读及纯围栏后整批替换内存目录。档案 reviewer 是原声明，不是这次审批人的固件认证；旧任务 review 因变更失效，不停止 Run 或释放占用。

这是目标6规划/审阅前置的一段正向操作链，不是固件签名、硬件/性能认证、上传/启动许可或完整执行闭环。原 startup 自动 load 仍未统一为双账号；本批只增加运行中显式 reload。全球坐标启动另要求 ABS 水平及 ABS 垂直且非 CONST_POS，不改变 RAW、通用 Health、室内或 GNSS 硬件范围。

19源码/资源路径见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，源码存在和局部接线不等于25项目标完成。受控执行/StartPermit与可信原点净空/正式结果、可信能力性能、完整自愈、企业跨站、长期运营、真实模型生态等六类仍需实质开发。未编译、测试、运行解析/helper/UI/认证/应用/SQL/设备，未改 PX4、刷机、更新 PR 或打包；不据阶段号或路径数估算工期。下方旧阶段为历史，其“缺目录入口”仅由本节运行中入口部分覆盖。

## 416：推进真实规划输入到执行配置的正向路径

本批将[逐机完整执行配置](../custom/src/Swarm/FleetMissionExecutionProfile.md)接到真实 Draft/Planner/Guided，不再只有外部 JSON 具备合法 profile 路径。每机准确 revision CAS、reserve 非活动保留、删除重加清空、真实参数观察后明确确认并在保存前重读，最后进入 Compiler 已有规范化路径；不自动填入任务全局配置、当前 Home、物理容差或安全推荐值。

这是规划到审阅的局部可实施增量，不等于完成目标6的全部受控执行。高级启动约束仍须完整 JSON schema1–3；空政策不能启动；完整 StartPermit、机载实例/正式结果、可信原点与净空、固件工程目录操作入口及其余25目标/六类继续。真实参数哈希只绑定观察内容，不认证硬件、性能、机载控制租约或空间安全。

阶段仅称416，11个源码/资源及15份文档范围见[清单](handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，限定静态结果已记录在本批审计；没有运行验收。UI 技能仅影响清晰标签、明确确认、错误反馈和现有 palette 的沿用。未编译、测试、运行 helper/UI/认证/应用/设备、改 PX4、刷机、更新 PR 或打包；旧批次描述保留为历史。

## 411–415：目标13的本地恢复操作入口

[恢复面板合同](../custom/src/Swarm/FleetOccupancyRecoveryPanel.md)补入在线、已知原生地面来源的本地restart/history再审界面，实际Root/current scope与专用双账号purpose绑定；不是自动自愈、机载改派或离线处置。当前真实unknown、原owner/任务及远端控权仍不清除。

原25项目标及六类剩余实质源码不减少为“只待工程师测试”。本阶段9源码/资源与18文档的接线并未完成最终StartPermit/可信HOME与local及净空、正式机载实例/结果、可信性能、跨站授权/租约、长期运营或真实预演生态。UI技能只影响静态确认、焦点与错误反馈设计；运行/布局/可访问性验证仍未做。

未编译、测试、运行helper/认证/应用/UI/SQL/仿真、设备、修改PX4、刷机、更新PR或打包。下方旧阶段无UI描述不覆盖本节源码接线。

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

目标13补上连续性的实际本地占用过滤、已知未知保留、源粘性与最终本地提交。仍需正式机载任务版本/实例、真实控制租约、路径/性能/净空与授权事务，以及未知状态的审阅解决流程；不能把当前snapshot无条目或原生CAS成功改称真实可接管。其余原25目标/六类继续。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](../custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

目标8/9/13补入静态角色备用资格、同时缺口最大一对一覆盖和本地完整角色变更；尚不是经过真实占用、路径/性能与批准事务的机载自愈执行。目标6最终许可/时序/正式结果、目标15企业交接以及其余长期运营/真实模型/生态继续，不能因本轮源API存在而把完整路线图记为完成。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](../custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

当前推进目标1–4/6/8/13/20的工程能力声明一致性，不把剩余工作缩减为测试。下一代码切点是按具体角色核备用/接替资格，而不是“有一架健康备用机”就视为可以接任 Leader/Relay/Scout；真实数值性能和完整受控执行也仍待开发。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

## 371–375 P0：下一部署/历史选择局部入口已写入源码

本轮8源码/19文档把上一批缺少的专用 transition 和产品入口接起来：两个 `ForController` native 接口以实际 Root begin → 私有栈 permit mutation → 新 scope/原 ID end 连续持占用；Guided 固定 prepare/select API 只接显示 token、目标 ID 与明确确认，公开12键 metadata/6键组行，不公开 raw refs。指挥中心与规划页复用 [LifecyclePanel](../custom/src/Swarm/FleetMissionLifecyclePanel.md)，先选动作再确认，默认关闭/取消、不用 Enter 提交、原 view ticket 防过期。

这只推进本地有界部署组生命周期，不是完整任务执行许可：prepare 不自动换 selected、登录、继承 grant、停止旧 Run 或释放占用；原 all-retained quiet、实际来源、shared5 和8组容量规则保留。已提交发布/选择事实与调用 end 独立，失败不回滚；失败尝试也计费，allocated=false 不证明未消耗容量。后置 source 读取失败不能丢弃已返回的发布事实及本地 token/身份协调。

`outboundIntegrationComplete=false` 保留。后续仍须可信 HOME/local/净空、最终 StartPermit、正式远端执行/完成归属、长任务续授权与容量长期生命周期等实质代码；六类及全部25目标不因这批局部入口上调为完成。限定文本检查记录已同步本轮静态审计；未编译、测试、运行 helper/认证/应用/设备，未更新 PR 或打包。以下366–370及更早路线图为历史，旧“下一步才做入口/专用 API”由本节覆盖。

## 366–370 P0：当前组控制器接线，不是新部署/选择入口完成

草稿失效的原scope只做cancel/revoke补偿；无法核实撤销时 `retiredCurrentScope` 持续阻断同组current入口/回调，selected监视保留可读，不设超时复活。这不是完整执行或运行结果回滚。5源码仅为累计工程增量，类型往返/方法绑定/parent/销毁仍需工程师按严格原合同验证。

已把现有 Guided 业务接到私有 Lifecycle 工厂和14个实际服务引用，并为30个 Fleet 入口/24个来源回调固定原组；两类 QML 已登记，构造已有 initial，无自动额外 prepare。新增原生同步 call 占用、plain lifetime/finally 原 owner+ID 收尾及 nested draft stale；scope 是引用身份而非许可，pure 回调不查瞬时忙，已 queue/未知结果不回滚。selected 四个观察值同 scope，选择代次 token 与完整 issued 收据把取消送回原 retained Close；显式监视登录可撤未消费批准但不结束 Run。

下阶段 P0 仍须面向用户的新部署/历史选择入口和与普通 call 占用兼容的专用 transition 原生 API；不能在普通 call 内调用切代而绕开自阻。`outboundIntegrationComplete=false`，其后仍有可信 HOME/local/净空、最终 StartPermit、真实远端执行/正式结果、长任务续授权及容量生命周期等实质源码工作。25目标/六类不缩减，企业跨站、长期运营、自愈和生态不因本批完成。实际5源码/18文档；已完成限定文本审阅，不是运行或安全通过，未编译、测试、运行或更新PR/包。以下保留历史增量，旧未注册/未迁移口径由本节覆盖。

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


## 340–344 P0 更新：独立部署生命周期基础，不是新部署已上线

[Lifecycle 原生核心](../custom/src/Swarm/FleetMissionDeploymentLifecycle.md) 新增每代14个实际 owned 对象（含独立 signed-out Session）、保留旧实际 Run/占用、共享五个冻结来源，以及全部 retained 原生静默检查和8次分配工厂；初始与失败尝试均占额度，不驱逐历史。它不重置旧 Txn/Series，不复活批准，也不要求 monitor 关闭才允许构造不重叠的新任务。

**应用尚未接入：无 QML 注册、无 Guided 对象替换、无新部署入口，当前仍单链。** 后续 P0 顺序为 native owned outbound 门禁 → QML 调用/OS 返回 actual Episode 与 epoch 冻结 → source callbacks 与共享 Role 显式身份上下文 → selected retained Run/Series/Close/Session 同组投影 → 完整入口与工程师验证。相同 binding 不能跨代充当授权。

原25目标和六类范围不缩减；本批不能视为长期任务、正式飞行结果、容量清理或完整运营生命周期完成。审计限定检查结果已记录；没有编译、测试、执行业务/认证/应用/设备或打包更新 PR。

## P0 335–339：监视结束界面已接线，执行生命周期仍继续

[新面板](../custom/src/Swarm/FleetMissionObservationPanel.md)补上330–334后端的两个固定入口，支持查看实际Run、独立申请审阅、当前两账号审批/三项确认及精确取消。UI不借草稿/upload权限、不回退旧Run、不接管另一入口review，原生仍决定来源/权限/期限/实际关闭，界面显示不认证可执行或安全。

下一阶段继续新部署/长任务/授权容量生命周期、可信HOME/local/净空及最终StartPermit、正式远端执行结果与授权提交；其余能力可信度、受控自愈、企业跨站交接、完整归档运营和真实模型/生态仍按原范围推进。此节只补关闭monitor界面，不把原25目标缩成观察功能；未编译/测试/运行UI、认证/helper或设备。下方330–334“无新UI”仅描述历史批次。

## P0 330–334：独立监视结束后端已接线，完整执行仍继续

[新的关闭工作流](../custom/src/Swarm/FleetMissionObservationClose.md)只从实际Series当前owner获取真实Run，绑定完整计划/本地占用和当前双账号；独立30秒审阅、专用purpose一次消费、三项确认、原生排空及实际map释放结果均已接线。没有新增UI按钮，不把local关闭/释放当停飞、正式Completed或可重新分配证明。

后续P0仍须可信HOME/local有效性/净空、最终StartPermit、完整时序和正式远端执行实例/结果、长任务/新部署/授权容量清理及必要执行UI。其余五类能力可信度、受控自愈、企业跨站权限交接、完整归档运营和真实模型/生态执行继续；原25目标不缩减，不能以本批后端收口称全部落地。本批未编译、测试、执行认证/业务/helper或操作设备；下方325–329及更早为历史记录。

## 325–329 历史增量：原生观察任务归属，正式飞行结果仍未认证

同一Orchestrator现在能记录本实际Run来源的protected任务及本地观察状态，不能再由通用complete/progress/reassign把它改为成功。结构/观察版本、成员占用和公开通知分开，连续性/运营也兼容其明确语义，不另建可被caller恢复的第二套任务权限。

这不是正式Completed、飞行开始/成功或安全停止证明。独立监视关闭审批后端已由330–334补入；仍需可信远端执行实例/正式结果及授权提交、最终StartPermit/净空/完整执行时序、长任务/新部署/容量清理，以及原25目标和六类其它功能。不能用更多观察记录或待工程师验证清单替代这些源码缺口。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](../custom/src/Swarm/SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

## P0 当前增量：明确 HOME 观察政策已接线，内部有效性与最终许可仍继续

新增schema3使真实owned普通ARM/MODE后、经原计划明确审阅预算内的HOME观测更新有正向消费路线；原始基线、全步骤计数、双原始叶配对、relative-only及两套risk revision均保留。逐机风险视图展示实际冻结限制，不新增发送入口，不把观察符合范围当作安全/完成认证。

后续P0仍须推进可信HOME/local有效分支与参考、真实净空/动力学、最终StartPermit、完整执行时序/正式任务结果归属、新部署与容量/长任务生命周期。原25目标和六类范围未缩减；不能把本批只读展示、bounded HOME或旧Run观测当作完整任务执行。

320–324（2026-10-15）新增[显式 HOME 观测过渡](../custom/src/Swarm/FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

315–319（2026-10-15）新增[本次启动的连续运行观测](../custom/src/Swarm/FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

## P0 当前增量：连续运行观测已接线，正式结果归属继续

315–319 已回到任务执行主线：真实 MODE 排队前的 provisional 切点、实际 write 后 captured commit、独立 Run 连续原始进度、启动发送认证退休与运行观测分离、协议排空后的原子整组本地占用移交，以及来源退休/未知尾部均已写入。独立 core slot 不抢占遥测记录器，不依赖归档开关；partial MODE/取消也保存事实。原 `FleetMissionProgressObservation::capture()` 仍是无 run 的快照，不能冒充新连续账本。只读 controller 按同 deploymentId 分开展示连续 Run 与旧 unbound snapshot，没有新增起飞/停止飞行按钮，也没有编译/测试。

进度已区分序号/重复 reached/跳号/模式偏离、post-commit 空中→terminal→落地候选与正式任务结果，陈旧样本只称 lastObserved，不以 binding current 认证通道 fresh。本地 PX4 源码快照发送路径不带 mission instance/finished，不能从最后序号/reached/landed 制造正式完成证明；该目录无 Git 身份，也不认证真机二进制。后续仍需可信远端实例/正式结果及授权提交、最终 StartPermit/净空/完整时序、合法 HOME 过渡政策与有效分支来源、新部署/容量和完整生命周期，不能用本次本地观察代替。原25目标和六类范围继续。

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

255–259 当前源码增量：新的 GroundStartApproval 独立账号条件意图申请、一次消费/撤销及 actual 9 来源/controller 已写入；public receipt 27 / native grant 14 / Session 独立 proof 22，对应新已阅风险 context 20 与原最早 READ START 30 秒。已完成风险阶段支持真实次账号提示且仍核 SID/role/WTS/旧登录期限，active READ 与实际证明消费没有放宽。本批已完成限定源码契约静态收口，不称新 startup authority/命令租约/MODE 续作/StartPermit；原完整 25 项及六类剩余范围保留，未编译/运行/测试、调用接口、改飞控或更新 PR/源码包/安装包。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](../custom/src/Swarm/FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

本路线图只统计地面站产品线的独立验收目标。已有代码基础不等于目标已经完成；目标需要经过编译、Mock/SITL、日志回放和工程师验收后才能标记完成。

## 当前批次进度（2026-10-14；增量静态审计）

244–249已写入原生启动lane、原账号/同UUID占用的一次Transaction接管、全组seq0→normalARM→mode-only屏障及持续RAW风险/typed核对，原审阅政策、逐通道gap与partial queue取消事实保留；本批有限源码静态收口已完成，未编译/运行/测试。没有执行UI或最终StartPermit，HOME合法重置再取证/再审/授权、local frame/运动/转向、净空/完整时序及完成继续原目标，不能用当前严格停止代替这些能力。原25项/六类范围保持，以下为历史进度。

最新239–243：独立StartupAuthority一次接管实际条件账号与固定全组来源依据，十个native来源/目录提供者/全记录/不可变风险/完整RAW/cache/health及typed BeforeArm候选；原deadline和SID/WTS保留，旧Review/Content/Approval不作为接管后的当前认证。原grounded读不放宽，新增passive retained proof与严格自身phase分开。controller内部真实来源/申请/匹配撤销/占用接线，无UI/实际命令。当前preARM依据不是最终StartPermit，真实ARM仍撤销；独立受控seq0/正常ARM/mode-only事务、允许差异及执行完成仍需继续。原25项/六类目标未缩减，未编译/运行/测试或更新PR/安装包。

最新234–238：启动前机载内容核对不再只用旧缓存/上传历史。新原生实际scope/原始PRIMARY完整INT读取、成功34字段收据与全规范载荷逐项比对已接到实际StartContentCheck/权限/controller/默认未确认面板；九参数StartReview绑定18字段新读取context且原30秒受最早READ START更早截止。120秒采集不保证整组最后仍新鲜，不续期、不恢复旧批准，顺序读取不证明远端原子快照/无线因果/硬件认证。没有真正StartPermit/受控启动/完成归属；原25项和六类剩余范围保持，未编译/运行/测试或更新PR/安装包。后面的“最新”段为历史批次。

本批次完成的是可交给工程师继续接入和验证的源码 groundwork，不等于对应产品目标已经验收：

最新229–233：真实不同账号的条件启动意图审批与原生已阅上下文已接线，上传凭据不能交叉消费为启动依据；Session专用16字段一次依据、34字段审批/13字段conditional grant保留原30秒审阅截止及全部风险，所有执行许可仍false。风险v2仅排除GP输送时钟/纯接收计数，独立risk revision保留真实物理/任务ABA；实际健康gate/Leader而非年龄通知决定撤销，不自动rebase或延期。controller仅内部来源/只读状态/申请撤销，没有Start/ARM或实际启动消费。完整鲜机载内容、受控启动事务/状态过渡、启动界面和完成归属仍待开发，不缩减原25目标/六类剩余范围，未编译/运行/测试或更新PR/安装包。

最新225–228：版本与固定十二项参数的显式只读刷新已接原生实际对象、操作者权限及现有面板；旧审阅/上传审批先撤销，顺序采集新 RAW/PRIMARY/epoch，不能从 ACK 或同值缓存生成新证据。120秒总/4秒阶段/100毫秒间隔，仅未发送的明确占用/限流/缺状态在原期限等待；首次版本清旧状态后允许等待新地面证据，实际飞行和来源异常拒绝。完成仅历史观测，不自动恢复审批、持续新鲜度、认证或Start/ARM；后续审阅仍独立30秒参数限制。六类剩余源码目标与25项验收范围不缩减；未运行/编译/测试或更新PR/安装包。

最新221–224：上传审批60秒申请窗口与已消费后最多4分钟的明确部署租约分开，原操作员5分钟会话仍为硬上限；schema2 receipt22/grant13冻结政策/实际deploymentId和原生租约，源二验不延期。Windows实际WTS桌面/会话加入认证与gate，锁定/注销/断连撤销，未知/不支持来源拒绝。UI分开说明期限/非飞行范围，继续使用默认取消。没有长期续授权、跨站租约或新的启动/完成闭环，原六类剩余源码工作保持；未编译/运行/系统API调用/测试或更新PR/安装包。

最新217–220：Windows本地系统认证/TokenUser SID与固定专用组角色、同一任务独立授权账号的短期一次性审批已写入并接到私有controller、上传gate与UI。系统窗口显示原生记录/版本/计划/目标摘要，消费后排队更新界面；有效已消费grant不被新请求暗换。默认未配置/非Windows拒绝，单人勾选不绕过、消费不续期，60秒包含输入及全部续作且慢批次可能停止。该本地路径不证明企业身份、两名人员或跨站租约，也不允许解锁/启动；完整执行闭环等六类源码目标仍未完成。未编译/运行/认证/测试或更新PR/安装包，下方是历史批次。

最新212–216：地面上传二次确认/实际编码项数/逐机传输/只读检查UI已接到原controller；默认取消，角色与策略快照贯通原生gate，未知身份及缺额外批准保持拒绝。完整历史回读来源与live批准分开，新独立StartReview生成30秒不可变风险快照，已阅不授予启动或第二人证明，不恢复ARM撤销的staging。界面不含Start/ARM；完整独立启动事务、新鲜机载整表身份和完成归属/账号审批服务仍待开发。未编译、运行、渲染或测试，原25目标及六类实质剩余工作不缩减。

最新208–211：六通道真实飞行状态/来源时效账本、原生StartReadiness和ProgressObservation已接线；显式推导当前位置起飞段/最低起飞高度提升及后续连接，独立缓存wire比较并映射raw序号，未解锁/未知/过期不当启动前置条件匹配。Deployment只读主动查询当前冻结实例，输出1秒snapshot-only显示与有界摘要；ARM不解除原暂存隔离。所有启动/完成归属许可false，尚无独立受控启动、上传/启动UI、完整完成判定；没有编译/运行/测试。本轮只是任务执行闭环中的来源与前置证据增量，原六类剩余源码目标不缩减。

最新 204–207：真实原始版本观测、十二字段内容身份与 callback/source 生命周期、版本/UID变化撤销旧参数、有界原生固件工程声明目录和精确匹配已接线。按实际 Git5/Vendor3 布局处理，十位源码前缀/板卡字段/声明材料均非二进制或身份认证；Review 私有目录、原生 profile 门禁、归档/上传源绑定与有界事件摘要同步。目录仅构造/显式原生加载，不物理 watch；没有预置已认证版本、启动许可或自动解锁，部署仍不可执行，未编译/运行/测试。

最新 199–203：固定十二项真实参数观测及接收 Link 来源、单调新鲜度、内容 revision/manager epoch 已接线；新增 VehicleEvidence 内容哈希/稳定来源指纹及明确请求一致性，固件元数据保持缓存而非认证。原生审阅和有界编码摘要进入版本存档，批准/上传继续步骤复核实际来源与当前身份，Fact 同步通知补生命周期保护。没有请求/写入参数或新增启动/解锁，部署仍不可执行。可信固件适配、起降路径/净空、绝对时序/同步、独立启动与完成/UI 等原缺口保留；未编译/运行/测试或更新 PR/安装包。

最新 195–198：显式执行 profile 进入编译/成员/覆盖/版本存档，holdSeconds 不与到达时间混用；v2 载荷加入巡航/返航速度、起飞、驻留和 HOLD/LAND 请求、来源映射以及 command-specific 完整 wire 身份。LAND 显式地面目标低于返航进近高度，float32 后仍需严格分离；驻留预算及与明确时序的矛盾检查已接审阅。尚无完整执行许可、绝对时序、独立垂直速率/参数认证、真实起降轨迹和完成判定；新增命令不解除暂存启动隔离，未编译/运行/测试。

最新 187–194：在地图/版本/批准基础上新增几何载荷构建、地面逐机上传与独立完整回读的源码服务；共享 Swarm 占用、取消排空、按系统 ID 的几何暂存启动隔离，以及 PlanManager/MissionManager 的重入和对象所有权防护已接线。恢复任务完成通知延迟到原生栈退出，并关联原请求、实际 Vehicle/MissionManager、操作 ID 和内容指纹；生成后的任务用新列表快照锁定，不把原源索引冒充新任务遥测索引。当前上传没有 UI 按钮，所有 Fleet 部署状态仍为 `executable=false`，没有受控启动或完整任务执行证明；仍未编译、运行或测试。

10 月 11 日增量补丁继续收紧 Vehicle 命令关闭/迟到 ACK、风险健康状态分类和能量计划输入；这些属于 P0 事务/状态可信度的源码 groundwork，未改变“待构建、待 Mock/SITL、待工程师放行”的状态。

| 目标 | 当前状态 | 说明 |
| --- | --- | --- |
| 1 能力发现协议 | 心跳/参数/版本来源与声明、当前工程审阅分离已接线 | 原始观测/失效账本、受控只读刷新和原生适配声明对应已接线；协议或manifest标签不再直接成为verified，当前能力只来自构型绑定的原生本地工程记录。组件元数据、可信构型/性能来源及真实构建/二进制身份仍待开发，不把本地声明当认证 |
| 2 能力护照 | 本地原子交接与导入能力重审已接线 | FleetRegistry版本化JSON/原子文件与32MiB上限保留，构型基线仍核同指纹本地记录；导入触及车辆更换能力record epoch，旧能力批准只作声明/诊断，不激活当前审阅。未触及记录保留；签名、权限、审批服务和长期历史仍待补齐 |
| 3 构型指纹/改装检测 | 本地指纹与四态判定已完成 | 只能识别提供的证据变化，不能替代实物检查；需验证参数/清单来源 |
| 4 能力验证与回滚 | 原生能力批准/撤销与失效已接线，尚非完整授权产品 | 构型基线与能力审阅分开；能力CAS上下文、revision/epoch/构型绑定、撤销/替代及A→B→A不复活已写入源码。只保留最新能力记录，无新增操作者授权UI/Session用途；硬件/性能认证、历史版本、签名与权限服务仍待开发 |
| 9 能量与备用调度 | 432备用独立待命／完整路线重做预算接入原431电池来源链，尚非真实电池模型 | 主机保持逐成员账单；备用使用自身起返点、明确目标、待命系数及自身速度，未知独立预算不借最长主机也不抹主机账单。Planner可编辑／取消／清除，原审批来源与扣龄检查保持。尚无中断点续接、实际替补、真实耗能／净空模型、多电池拓扑或性能认证；工程验证未开展 |
| 10 行业任务模板市场 | 本地原子目录交接 groundwork 已完成 | `FleetTaskTemplateRegistry` 为模板提供三段式版本、能力/任务约束校验、稳定 JSON 导入/导出及 `saveToFile()`/`loadFromFile()`；默认模板由 C++ backend 注册，目录写入不再暴露给 QML；签名服务、模板审批、长期存储和结果格式化执行仍待产品/工程师接入 |
| P0 事务与健康门禁 | 源码补丁已完成待构建 | ACK/超时/临时 Mission/标准任务启动/恢复任务/未知健康路径已收紧；标准/临时启动延迟窗口复用 `_vehicleMissionStartReady()`，同一车辆普通事务互斥（RTL/Land/ABORT 为安全例外）；未编译、未做 SITL |
| 7 事件触发任务规则 | 只读规则评估与本地快照持久化 groundwork 已完成 | `FleetEventRuleEngine` 支持严格规则/条件 schema、单规则冷却、前一事件类型门槛、可重置运行时状态和版本化 JSON/`QSaveFile` 原子交接；导入不恢复冷却/事件序列，也不释放命令；跨站规则同步、人工审批和工程师验证待完成 |
| 8 动态角色与能力市场 | 当前工程能力、全局/角色门槛与静态备用资格已接线 | 原生当前能力记录进入候选池；显式全局及四角色要求有界校验，备用按实际主角色资格选择并分列单角色/同时覆盖。Compiler最终成员门禁保留；真实性能等价、可信性能来源、任务协议和工程师验证仍待完成 |
| 5 集群健康中心完善 | 只读聚合及原生电池动态投影已接线 | `FleetHealthCenter` 合并 `FleetRegistry`/`FaultToleranceManager`；Registry接Vehicle健康、ESC、FTC及能力／构型证据。431电池不再平均旧Fact缓存，公开快照／汇总使用实际原生单推进报告的3秒接收鲜度；旧单行健康通知不等于原始电池事件。其它证据鲜度、固件语义及整套工程验证仍待完成 |
| 6 任务编译器 | 地图/版本库/profile、地面上传UI/回读及独立风险审阅已接线 | 显式速度/起飞/驻留/HOLD/LAND请求和完整wire比对；新增冻结版本/角色/实际项数的覆盖确认、逐机状态/只读检查，以及历史回读到新30秒风险审阅的独立来源。默认无账号/额外批准不得上传，已阅不当启动许可。绝对时序、垂直参数/固件认证、起降净空、完整受控启动/完成及启动界面、孔洞/退让/动力学、重启草稿恢复和工程师验证仍待完成 |
| 12 4D 空域冲突管家 | 只读冲突检查 groundwork 已完成 | `FleetAirspaceConflictChecker` 检查时空最小间距、禁飞区和返航通道，输出严重度、原因与替代方案；真实空域数据、坐标基准、法规和工程师验证待完成 |
| 13 自愈式任务连续性 | 本地角色/占用CAS、保守历史留存及在线原生专用复核已接线 | 全缺口匹配、批内排他、old∪new实际占用与sticky来源保留。实际Swarm异步留存历史，选中在线地面未解锁机可经专用双账号/整组尾栅解除restart/history；换源再hold，新actual须真实commit。不清本轮真实unknown、任务或owner；操作UI、离线/报废现实处置、真实unknown解决、正式机载改派/控制租约和工程验证仍待完成 |
| 14 低带宽与降级运行 | 只读策略快照接线 groundwork 已完成 | `FleetLinkDegradationPolicy` 按较低频率读取 Registry 在线/链路质量快照，并在指挥中心显示正常、降级或证据不足数量；根据质量/带宽/延迟/丢包和任务优先级输出视频、遥测、编队速度、数据优先级与恢复补传建议；真实带宽/延迟/丢包测量、限流执行、补传队列和工程师验证待完成 |
| 15 任务交接与中途加入 | 本地原子快照交接与受控操作入口 groundwork 已完成 | `MissionHandoffManager` 保持人工交接/二次审批记录并支持 JSON 与 `saveToFile()`/`loadFromFile()` 原子文件交接；指挥中心交接面板要求本地操作员身份，可发起固定二次确认的交接请求，并用 `recordRevision` 接受/拒绝/撤销；`FleetMissionContinuityPlanner` 输出满足门槛的中途加入候选；无账号持久化、跨站同步、任务版本提交或自动执行，工程师验证待完成 |
| 16 任务数字孪生 | 原子快照与 QML 只读边界 groundwork 已完成 | `FleetMissionDigitalTwin` 接收 fleet/mission/map/link/energy 快照，新增只能走 `captureSnapshot()`/`setSnapshot()`；输出稳定哈希、时间线和风险摘要，并支持 `saveToFile()`/`loadFromFile()`；没有飞控/碰撞/风场执行模型 |
| 17 故障注入与“如果这样会怎样” | 故障覆盖与人工建议 groundwork 已完成 | `FleetMissionDigitalTwin::injectFault()` 支持失联、低电量、GNSS、Leader、链路退化/延迟并生成人工复核建议；不修改任务、不发命令，真实故障注入和安全语义待完成 |
| 11 任务风险与完成概率 | 只读风险评估 groundwork 已完成 | `FleetMissionRiskEvaluator` 汇总计划、健康、能量和风险信号，输出最坏结果、启发式完成概率、覆盖证据状态和替代方案；真实概率模型、空域/碰撞模型和工程师验证待完成 |
| 19 自动事故与任务报告 | 报告构建、事件快照接线与只读摘要展示 groundwork 已完成 | `FleetMissionReportBuilder` 输出指挥员、客户、工程师、安全员四种结构化视图、异常列表、关键时间线、未验证项和 JSON；意图预览默认传入黑匣子事件快照，控制器/指挥中心只读显示 verdict、异常和未验证计数，并支持 `saveToFile()`/`loadFromFile()`；权限、模板审阅、长期归档和工程师验证待完成 |
| 20 任务证据链 | 本地原子快照交接 groundwork 已完成 | `FleetMissionEvidenceChain` 串联计划摘要、能力/构型指纹、审批、风险、关键事件和最终结果，支持 `saveToFile()`/`loadFromFile()`；导入前逐条校验 SHA-256 链并原子拒绝无效快照；数字签名、权限、长期留存和工程师验收待完成 |
| 21 维护护照与预测性维护 | 本地原子快照交接、控制器入口和只读状态展示 groundwork 已完成 | `FleetMaintenancePassport` 接收电流、温度、振动、电池衰减、飞行次数和改装记录，输出证据缺口、阈值原因、检查建议和估算风险，并支持 `saveToFile()`/`loadFromFile()`；控制器/指挥中心只读显示已有记录的评估，空记录明确显示证据不足，不自动写入；阈值校准、长期数据库、真实维护数据接入和工程师验证待完成 |
| 22 集群记忆地图 | 本地原子快照交接 groundwork 已完成 | `FleetOperationalMemory` 记录链路盲区、GNSS 不稳定、风场、高耗电航段和异常历史，输出位置格网与显式估算标记，并支持 `saveToFile()`/`loadFromFile()`；真实地理/遥测采集、长期数据库、格网校准和工程师验证待完成 |
| 23 运营指标与 ROI | 只读指标与本地快照交接 groundwork 已完成 | `FleetOperationalMemory` 聚合任务完成率、覆盖率、响应时间、能耗、人工干预、利用率和维护成本，并支持版本化 JSON 与本地原子文件交接；成本口径、样本完整性、跨任务统计和工程师验证待完成 |
| 24 渐进式界面与分级自主 | 只读阶段策略与目录展示 groundwork 已完成 | `FleetAutonomyPolicy` 声明人工、辅助、监督和自动化阶段的权限、人工/双人审批与命令释放边界；`FleetCatalogPanel` 只读展示阶段和适配器签名/沙箱状态；AI proposal→Fleet review 仅在操作员显式点击后运行，审批 API 额外核对同一 `taskId/planId` 且 `needsReview=false`；真实确认器、持久化策略和自动化执行仍待接入 |
| 25 平台生态层 | 扩展元数据本地快照与目录展示 groundwork 已完成 | `FleetAutonomyPolicy` 声明 PX4、ArduPilot 和伴随视觉插件的能力、权限范围、签名状态与沙箱占位；`FleetExtensionRegistry` 增加严格校验、原子 JSON 文件交接和固定命令释放边界，但不加载二进制，默认目录由 C++ backend 注册；`FleetCatalogPanel` 只读取扩展值快照；真实签名验证、沙箱、版本兼容和适配器执行仍待工程师接入 |
| 18 黑匣子与时间轴持久化 | 连续诊断SQL/显式轮换与跨封存浏览已接线 | 默认off/native明确配置；actual BlackBox收据、固定摘要、有限SQL/轮换与inert历史保留。共用Sql schema/strict页、单请求READONLY Reader核实际文件/DBepoch/sealRun，catalog native ID与每段filter/实际next cursor，全history单槽，合法1MiB预留预算。历史不进rules/replay/权限；clean/hash/目录不是完整性、签名或全集证明。全集索引/自动归档清理、完整遥测/离线恢复/企业权限及工程师验证仍待完成 |

其余目标保持“未验收”，按本路线图的依赖顺序继续实现；任何源码状态都不能直接推导出可刷机或可放飞结论。

### 剩余源码工作口径

280–284补上显式v2运动/转向观察政策及真实阶段消费：不再统一要求MODE后LPN XYZ/heading恒等，且政策必须随新计划/审阅冻结，NED参考元数据与ODOM周期gap独立核验。原严格schema1仍保留。该观察匹配不完成受信local origin/内部有效分支、动力学/障碍净空、最终StartPermit、完整时序或任务执行完成；这些代码缺口仍在第1类，不把未完成项改成“仅等工程师开按钮”。下方275–279为历史诊断增量。

275–279为第1类补上实际被动ODOMETRY参考/重置状态与时钟断裂的可观测性、READ冻结和阶段源复核，未打开原运动/转向分支。它只捕获看见的参考变化，不建立local origin、PX4 z_valid/home.valid_alt、同样本或内部参考有效性证明；完整HOME分支、运动/转向、净空/最终许可/时序/完成仍须继续实现，不能把这一诊断补丁等同任务执行闭环完成。

25 项均已有基础实现，不代表 25 项全功能完成。修改清单的编号是改动记录，不是产品功能完成计数；不能由类数、文件数或静态检查推导百分比。即使不计工程师的构建/实操验证，仍有六类实质开发工作，横跨原 25 项：

1. 任务执行闭环：补齐忠实的时序/起止语义、独立启动许可、完整受控状态过渡和启动界面；已有三步事务、混合armed不重复ARM、多轮最新actual terminal owner取证/审批与有界独立consumer序列、ODOM参考/时钟断裂账本、明确审阅的NED运动/航向观察包线及原始周期gap。315–319再接真实MODE切点/commit和独立连续Run、有界post-commit进度与原子整组local占用，认证退休不抹partial事实；仍不是最终启动许可或正式结果归属。还需容量/新部署生命周期和长期运行归档、合法HOME有效分支、受信local origin/内部参考有效性、完整动态/净空与StartPermit、长批次受控续授权、可信远端实例/正式任务结果及授权提交和企业来源。raw状态/序号和本地Run不能把观察/已阅/批准/接管/模式或LAND候选变成正式完成；新读取也不证明远端原子快照或本次执行归属。
2. 机载能力与性能证据：参数/原始AUTOPILOT_VERSION生命周期、受控只读重采集、工程声明对应和构型绑定本地能力审阅已接线，仍需组件元数据、真实构建/二进制认证、可信构型与数值性能清单来源、实际操作者授权界面/审批服务及历史。短前缀、RAW到达或本地能力token批准均不代替认证与实测性能。
3. 自愈受控执行：角色资格、完整一对一提案/本地角色CAS、actual Swarm占用、已知unknown保留与六参门禁已接；401–410增加保守原生历史留存及在线地面未解锁机的专用双账号复核，只解除独立restart/history，换源重新hold且新actual须真实commit。占用复核操作UI现已接入指挥中心；542再补人工编队待办与重新确认，不重复列为未开发。离线/报废现实处置、本轮真实unknown受控解决、完整durable/WAL覆盖、机载任务实例与控制租约、路径/性能和授权事务仍须开发，才能把替补/Leader接替/拆分/重分配/恢复落实为正式机载变更。本地解除或CAS不证明实际可接管、机载已改派或故障恢复成功。
4. 跨站协作：本地认证及Windows桌面撤权不替代企业/服务端账号鉴权、跨平台与账号实时撤销、真实跨站控制租约、交接提交、同步及安全中途加入。
5. 长期运营：已有默认off连续诊断SQL/封存轮换与分段浏览、27类批准入站消息NDJSON writer及独立只读catalog/prefix分页。305–309加入共享Header/Footer、source/序号/3计数镜像、全file重开hash和actualcursor/filter/定位/双终结槽；310–314再接 Windows/POSIX error-aware目录清点，明确EOF/失败/关闭及观察目录对象，不把正常枚举当全集认证。public metadata与native最新原包分开，旧段不自动删除，部分尾/缺Footer与非法记录分开；fullTelemetryCaptured/closure/身份/权限仍false，不改.tlog或请求新增流。仍需批准消息/覆盖证据扩展、完整历史全集索引、自动外部归档/留存清理、持续完整遥测、跨任务统计/维护集成及离线灾后恢复；历史条目/hash/native游标/维护续建或journal状态不等于完整证据、权限或执行状态恢复。
6. 高级模型和平台生态：真实预演/能量/动力学模型、插件签名/沙箱及跨飞控/伴随适配器执行。

下阶段以本地实际固件语义为证据，不只依据地面站命令列表。当前配套 PX4 的 `DO_CHANGE_SPEED` 不区分 speedType 而改变水平巡航速度，不能用 type=2/3 假装编码独立爬升/下降速度；NAV_DELAY 的本地实现不提供绝对到达时刻或跨机同步。后续要显式保留执行契约、已编码/未编码要求和源字段到 Mission 序号映射，绑定垂直参数配置证据；绝对时序、同步和完整起止动作仍属于原目标，不能删除或改称已实现。

### 2026-10-13 任务调度增量

最新 183–186：地图规划入口已写入源码并完成 native/controller/UI/资源接线，支持固定成员和五类角色、区域顶点/参数/端点/辅助路线编辑及逐机提案预览。输入编辑失效旧审阅，审批绑定显示的草稿 revision 和归档 recordId；按下至松开的交互 epoch 与最终发送门禁隔离旧地图指令，暂停自动移图。草稿 32 成员容量不突破既有六机协议，声明 WGS84/AMSL 不证明净空或坐标正确，估算不补齐权威 4D 时序。本轮仍未编译/运行/渲染/测试，25 项完整目标未收口；后面的批次说明为历史边界，地图入口已有基础源码但并未交付完整执行产品。

新增 179–182：端点模式补齐进场/独立返回的几何提案、3D 估算和全路径高度门禁；版本库保存完整计划与独立审阅身份，批准/交接审计可引用具体不可变版本。文件恢复不恢复批准，本地锁/基线 CAS 不等于跨站控制权。地图编辑、孔洞与退让、感知/动力学模型、真实上传执行、账号/跨站服务和长期存储轮换仍待开发；工程师验证仍未执行。

区域任务代码增量（目标 6/9/12）：通过显式 `task.coverage` 调用 `FleetAreaCoveragePlanner`，生成简单凹/凸多边形扫描航段并按机分配；辅助角色自带路线，区内连接用可视图，坐标/高度基准及资源有界。逐机路线/时间/返航信息贯通编译和审阅，能量预算纳入最长路线估算，空域保持估算时序未知。仍需完成地图编辑、孔洞/安全退让/转弯模型、进场和返航生成、真实执行闭环；本轮不运行规划器或任何编译/测试，不能把该增量当成完整任务编译器或植保功能验收。

本地成员占用已接入编排器启动/恢复/活动重分配门禁，暂停保留成员、终态释放，提供冲突诊断和忙机摘要；编队必须先预留再发送，连续性重审保留完整任务上下文。能力/构型资源门禁补齐实际早退出及累积合并预算，任务记录也增加资源上限。这些是目标 6/8/13 的本地代码增量，不是跨站租约、真实任务调度服务或工程放行；25 项仍不能用基础类数量直接计算完成率。

## 2026-10-07 续作边界加固

本轮继续围绕 P0 输入可信度收口：FleetHealthCenter 对健康证据、目标集合和 Leader 门禁执行严格类型/范围/全量校验；FleetMissionCompiler 对车辆身份、角色、备用机、闭合机队成员和真实围栏几何执行 blocking 级编译校验。上述仍是地面站源码 groundwork，尚未经过 Qt/MSVC 构建、Mock/SITL 或真机验证。

按当前产品规划，后期共有 **25 个开发目标**，分为五组。

## 一、能力、构型与可信度

1. **能力发现协议**：通过 `AUTOPILOT_VERSION`、Component Metadata、MAVLink FTP 和旧固件回退机制读取无人机能力。
2. **无人机能力护照**：保存构型、性能、传感器、任务能力、数据来源和验证时间。
3. **构型指纹与改装检测**：比较固件、板卡、参数、清单哈希、ESC 和传感器拓扑，识别换电池、换桨、换动力和参数变化。
4. **能力验证与版本回滚**：支持工程师确认、签名、版本差异、冲突处理和重新验证。
5. **集群健康中心完善**：补齐 EKF、链路、ESC、电池、任务状态和起飞条件，并给出可解释原因。

## 二、任务规划与调度

6. **任务编译器**：把区域、目标、时间和限制条件编译成可审阅的飞机、角色、航线和备用方案。
7. **事件触发任务规则**：支持“发现目标、低电量、链路下降、区域告警后执行下一步动作”的规则编排。
8. **动态角色与能力市场**：按能力、距离、余量和风险选择 Leader、Scout、Relay、Responder 和 Reserve。
9. **能量与备用调度**：计算任务最低机数、返航余量、替补时机和任务完成所需电量。
10. **行业任务模板市场**：提供巡逻、巡检、搜救、仓储、测绘和通信中继模板，并支持版本化、签名和结果格式化。

## 三、安全与任务连续性

11. **任务风险与完成概率**：把风险来源、最坏结果、完成概率、覆盖率和替代方案一起展示。
12. **4D 空域冲突管家**：按空间、时间、返航通道、禁飞区和最小间距检查冲突。
13. **自愈式任务连续性**：处理 Leader 故障、失联、低电量、编队拆分、任务重分配和恢复。
14. **低带宽与降级运行**：自动调整视频、遥测、编队速度、数据优先级和恢复后的补传策略。
15. **任务交接与中途加入**：支持跨操作员、跨地面站交接，以及新飞机安全加入正在执行的任务。

## 四、预演、审计与结果交付

16. **任务数字孪生**：使用真实机队构型、地图、链路和电量模型预演任务。
17. **故障注入与“如果这样会怎样”**：预演失联、低电量、GNSS 下降、Leader 故障和通信延迟。
18. **持久化黑匣子与时间轴回放**：保存任务版本、构型、审批、指令、ACK、遥测和异常；当前 groundwork 提供带 32 MiB 输入上限的本地原子文件交接，完整签名、权限和留存策略仍需工程化。
19. **自动事故与任务报告**：面向指挥员、客户、工程师和安全员生成不同报告。
20. **任务证据链**：保存能力快照、审批人、构型哈希、风险结果和最终任务结果，支持追责和复盘。

## 五、运营、维护与平台扩展

21. **维护护照与预测性维护**：根据电流、温度、振动、电池衰减、飞行次数和改装记录预测维护需求。
22. **集群记忆地图**：长期记录链路盲区、风场、GNSS 不稳定区域、耗电高的航段和历史异常。
23. **运营指标与 ROI**：统计任务完成率、覆盖率、响应时间、能耗、人工干预次数、利用率和维护成本。
24. **渐进式界面与分级自主**：按角色和风险显示信息，支持人工、辅助、监督和自动化等不同任务阶段。
25. **平台生态层**：包括插件签名与沙箱、PX4/ArduPilot 等跨飞控适配、伴随计算机/视觉结果接入，以及能力模型扩展。

## 产品演示主线

登记两种不同构型的无人机，系统识别能力差异；输入“园区巡逻”目标，自动生成编组和方案；在预演中注入低电量和失联；系统给出接管建议；任务结束后生成时间轴回放和客户报告。

## 计数说明

25 项是产品目标数量，不代表 25 个都需要新建独立类。当前代码已经为意图任务、能力匹配、风险雷达、黑匣子、回放、预演、交接、模板和插件提供基础，后续重点是能力发现、可信构型、任务执行闭环、持久化、验证和运营化。

## 2026-10-07 续作边界

本轮 P0 继续围绕“证据不会永久新鲜”收口：故障评估年龄由 Registry/FTC 以地面站观测锚点推进，Leader 替换/删除/无候选路径都输出可审计建议；健康窗口通过每机快照传给连续性规划和审阅指纹。以上仍是源码 groundwork，未执行 Qt/MSVC 编译、Mock/SITL、硬件测试或刷机放行。

本轮同时把能力发现接到实时路径：`FleetCapabilityDiscovery::buildDescriptor()` 对规范化结果自审计，`SwarmController::_refreshFleetRegistry()` 在追加 QGC 健康字段后再次校验；非法能力/构型/健康证据会保留诊断并标记 `discoveryState=invalid`，不会被误读为已验证护照。该项仍属于源码与静态边界，工程师需在目标 Qt 环境确认编译和真实数据时序。

事务边界续作：单机 Takeoff/Land/RTL 目标冻结与数据指纹、Formation phase/deadline timeout、Mission/Land/RTL dispatch 前模式证据、queued goto 全量坐标校验和创建顺序事务列表已补入源码；临时 Mission 的共享完成信号仍没有底层 operation token，只能作为工程师 Mock/SITL 的明确风险项。

能力可信度续作：修复首次 FleetRegistry 注册时发现证据只在顶层、进入 `VehicleCapability` 后丢失的问题；verified capabilities、证据状态、构型证据来源、discovery 状态/错误、发现时间和位置证据源现在被保留、重新输出并做入口类型校验。该项仍需工程师完成 Qt 编译、适配器 round-trip 和 invalid discovery 运行时回归。

能力匹配续作：verified-only 任务现在只接受显式 `verifiedCapabilities`，不再把运行时 `canUse*` 观测提升成可信能力；普通观测/规划路径仍单独保留，后续需工程师验证策略字段从任务模板到匹配器的传递。

连续性续作：替补候选的 verified-only 过滤与能力匹配器统一，fleet fingerprint 纳入 verified/discovery 证据，可信度变化不会继续沿用旧重分配建议；仍需 Mock/SITL 验证异步建议时序。

能力输入续作：Matcher 对能力标签和 verified 标签的列表类型做严格校验，坏证据不会进入候选；仍需工程师验证不同 MAVLink/JSON/QML 适配器的数组类型映射。

连续性输入续作：替补规划的 fleet snapshot 入口也拒绝畸形能力列表，输入错误不会因任务没有能力要求而漏进候选。

任务能力输入续作：required capability 列表采用同一严格字符串契约，错误任务不会被自动转型后继续生成候选。

角色分配续作：角色能力筛选支持 verified-only 策略，避免角色规划与能力匹配器的信任级别分叉。

任务模板续作：模板验证入口支持 verified-only 能力门禁并严格拒绝坏能力列表，保持模板审查与角色/匹配器语义一致。

候选失败续作：能力 Matcher 在参数无效时不再产生可被误用的选择列表，审阅层只接收阻断证据。

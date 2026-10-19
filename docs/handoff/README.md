# MERIVUS 工程交接入口

## 2026-10-19：PR #6 构建反馈修复

先阅读[本次修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定核验结果](MERIVUS_SWARM_STATIC_AUDIT.md)和[Ubuntu / Qt 5.15.2 复测说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本次修复18个源码文件中的 Qt 类型注册、QML 分号、保留字和按钮属性冲突，只更新同一个 PR #6，不合并主分支。

本机已执行离线语法核验，未完成地面站整体编译、启动或 PX4 + Gazebo SITL 集成验证。历史批次中的“未执行解析或编译”只描述当时状态，不替代本次明确列出的有限检查，也不能用本次语法检查覆盖历史待验项。

## 546：最终本地源码交接阅读顺序

1. [最终范围与外部未完成项](MERIVUS_FLEET_DELIVERY_SCOPE.md)：用户已确认本地源码交付，不等于25项完整产品验收。
2. [修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)：本轮52路径及历史修改记录。
3. [限定静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)：实际检查范围、已修问题和未验证事项。
4. [工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)：本轮38项待验证场景，全部未执行。

五条新增入口的模块合同均由上述文件链接。最终包应使用当前完整源码，而不是旧阶段ZIP或只复制本轮52路径。只交付源码；目标工具链构建、仿真、硬件与发布仍由工程师决定。没有新安装程序，没有提交、推送、更新或合并PR。


## 545：完整证据链文件操作

[后台合同](../../custom/src/Swarm/FleetMissionEvidenceChain.md)、[现指挥中心操作](../../custom/src/Swarm/FleetEvidenceArchivePanel.md)、[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)及[12项未执行验证](MERIVUS_SWARM_ENGINEER_HANDOFF.md)已同步。关键是明确覆盖确认、冻结一致性、活动链隔离、写入/读回事实和准确历史ID。完整代码仍待工程师构建与验证；不制作阶段包，不冒充安装程序。

## 544：公共航线实际下发准备

本次修复缺省成员路线没有展开的问题，详见[编译合同](../../custom/src/Swarm/FleetMissionCompiler.md)、[16路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)和[10项未执行工程验证](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。请特别核对公共时序与成员声明混合、无效成员不回退、完整出返程参考及旧归档重新审阅。没有阶段包；源码不是已验证安装程序。

## 543 当前：配置速度明确采纳与逐机路线预算

当前参数观察可提供四项配置速度建议，操作员查看并明确采纳到未保存输入；采用前与保存时都重查原来源。返程 NAV 建议显式沿用巡航值，不是独立RTL参数，不代表真实性能认证。普通路线现在由最终本机路线/profile生成出返航行估值，与驻留分开；只提高既有预算，不能把未知总任务时长补为可执行。

兼容原备用配置读取、载荷字段核对及任务重规划；不增加飞行指令、放宽启动权限或恢复旧批准。10源码＋21文档，共31路径，无新文件。原25目标尚未全部完成；不制作阶段包，继续面向统一源码交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR；不以文件数估算进度与交期。

见[配置操作](../../custom/src/Swarm/FleetExecutionProfileEditor.md)、[路线模型](../../custom/src/Swarm/FleetMissionPreviewTimeline.md)、[实际预算消费](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)。下方542及更早为历史。


## 542 当前：编队等待与重新确认（源码接线，工程验证未执行）

新增本次会话的编队待办：明确保存固定成员 → FIFO 等待实际占用释放 → 操作员重新审阅 → 原确认框与既有派发入口。资源就绪不自动派发；取消待办不停止飞机，已提交请求不等于飞控接受或任务完成。最多32条（含终态），待办30分钟、单次审阅60秒，以原生单调时钟判断；重启不恢复队列或批准。

实际 Swarm/Orchestrator 原对象提供占用信息，原生占用未知或未解决时不能进入复核；确认消费前重查同一来源、版本、FIFO 与占用。新增有界任务占用投影，避免定时复制完整 Run 观察；原协议仍仅接受 UAV-1～UAV-6、必须包含 UAV-1，不扩大飞机容量或最终启动许可。

本次12源码/工程登记＋16文档，共28路径，推进目标6/8/13的本地调度操作链；25项目标尚未全量完成。按最新要求不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR；不根据路径数推算完成率或交期。

见[队列原生合同](../../custom/src/Swarm/FleetFormationQueue.md)、[实际确认与派发接线](../../custom/src/Swarm/FleetFormationQueueWorkflow.md)、[操作界面](../../custom/src/Swarm/FleetFormationQueuePanel.md)。下方541及更早为历史。


## 541 当前：任务模式退出证据贯通（源码实现，工程验证未执行）

原生提交后的任务模式退出现在保留计数、首/末 HEARTBEAT 原模式与状态、前后接收引用；已置异常位不会吞掉后续已提交快照的变化。完整/保护报告、交接简报和归档历史同步版本化，旧资料显示“未采集”，不补成零次。检查点之间多个退出可合并，不宣称完整逐边沿日志。

18源码/工程登记＋23文档，共41路径，新增共享校验器 h/cc/md。只推进运行结果、交接和审计链，不改变终点候选、不发飞行命令、不恢复控制权。原25目标与实质剩余保持；不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR，不据路径数推算完成百分比或交期。

见[退出证据合同](../../custom/src/Swarm/FleetMissionModeExitObservation.md)、[报告合同](../../custom/src/Swarm/FleetMissionExecutionReport.md)、[审计版本](../../custom/src/Swarm/FleetRunObservationEvent.md)。下方540及更早为历史。

## 540 当前：完整计划驱动的逐机时间预演（源码实现，工程验证未执行）

预演不再仅接任务摘要：操作员明确捕获当前审阅记录，同一记录的完整计划与能量预算一起进入逐机路线计算。出程、独立返程及显式驻留分别计算；缺速度、垂直率或坐标基准时保留未知，不均摊全队航线、不借新电量补旧计划。原生结果保有界事件集合，现有界面四条分页；自动概览刷新不覆盖正在浏览的冻结计划。

相对时间以各机首规划点为零，不代表跨机同步；起降、转弯、风及实际飞控动态未由此认证。此项推进目标16/17，但不替代最终StartPermit、正式机载结果或完整数字孪生。原25目标不缩减；不制作阶段源码包，完成约定源码后统一交付。未编译、运行、测试、仿真、刷机、修改PX4、提交或更新PR。

见[计算合同](../../custom/src/Swarm/FleetMissionPreviewTimeline.md)、[实际界面与来源](../../custom/src/Swarm/FleetDigitalTwinPanel.md)。下方539及更早为历史；不按批次或路径数量推算完成百分比和交期。

## 539 当前：交接历史保存与恢复（源码实现，工程验证未执行）

原运行简报交接增加后台文件保存、独立加载预览、四条分页与详情，以及明确确认后的仅追加历史合并；不开放旧整库替换入口。同ID内容冲突整笔拒绝，已保存事实与当前工作资格分开。详细流程见[交接工作流](../../custom/src/Swarm/FleetMissionHandoffWorkflow.md)、[原生合同](../../custom/src/Swarm/MissionHandoffManager.md)和[界面合同](../../custom/src/Swarm/FleetHandoffPanel.md)。

原25目标保持，本次推进目标15的本地资料交接，不代表跨站控制权交接或全部目标完成。无阶段源码包；完成约定源码后统一交付。未编译、运行、测试、仿真、操作硬件、刷机、修改PX4、提交或更新PR；不以文件数推算完成百分比或交期。下方538及更早为历史。

## 538 当前：持久化任务索引（源码接线，工程验证未执行）

建立固定派生索引，保存分窗扫描进度；重开核原来源后继续，任务分页与原记录下钻保持有界。不是提高旧目录上限或只新增建议界面。详细实现与范围集中见[索引合同](../../custom/src/Swarm/FleetArchiveTaskIndex.md)、[界面合同](../../custom/src/Swarm/FleetArchiveTaskIndexPanel.md)及[修改清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

原25目标与六类剩余保持。先前“约40%／60%”没有逐项验收权重，仅属历史粗估，不再作为当前完成度或交期依据。按用户最新要求，不制作阶段源码包；完成全部约定源码后统一交付。当前仍未全量完成，未编译、测试、仿真、运行项目、刷机、修改PX4、提交或更新PR；实际工程验证仍由工程师承担。下方537及更早为历史。


## 537 当前：逐 NAV 航点相对驻留（源码实现，工程验证未执行）

沿真实辅助路线点编辑器增加可选holdSeconds：独立出返程点可新增、编辑、取消或清除驻留；缺失与明确0分别保留，原秒数贯穿草稿、计划和来源身份。复用原点编辑的版本／来源保护，不把驻留解释为绝对到达时刻，也不扩成自动覆盖、起降或HOLD动作设置。

新增共享DwellEncoding，统一拒绝正请求在Float32或所述PX4微秒转换中变成零等待。上传与规范回读仍沿原param1规则，原请求不被量化回写。能量／时间审阅改用原请求、上传值及模型等待量的保守预算；仅确有下舍才上取，精确1秒和1+1不无故增加ULP。三个严格几何消费者与Joint使用相同接受域，明确驻留即使没有executionProfile也进入原实时版本／当前工程声明门。

模型仅对应已读本地旋翼NAV分支，计时从首次位置接受开始，yaw等待可能消耗它；不是精准悬停、持续留在半径内、真实调度上界或实际固件认证。没有新增payload/schema/权限字段，最终StartPermit、actualPath和正式结果仍未完成。

本批12源码／工程登记＋21文档=33路径，新增DwellEncoding.h／cc／md三条；40项工程场景全部未执行。限定直接静读修正了新点错误消息的局部变量重名问题，并交叉核对实际消费者与数值模型。UI/UX仅指导原Qt表单的独立勾选、标签、错误及取消恢复，没有运行技能脚本、界面预览或任何项目逻辑。

原25目标与六类实质剩余不缩减，约40%源码／60%剩余仅粗估。未编译、运行helper／算法／parser／QML／hash、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[驻留数值合同](../../custom/src/Swarm/FleetMissionDwellEncoding.md)、[实际点编辑](../../custom/src/Swarm/FleetMissionDraft.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方536及更早为历史。


## 536 当前：逐 NAV 航点机头方向（源码实现，工程验证未执行）

真实辅助路线编辑入口、Draft、Compiler、载荷及上传／回读消费者接入可选逐点 heading。规划界面支持 relay／responder 独立出返程点的新增、整点编辑、取消和删除；区域自动覆盖点仍沿飞控策略，不把扫描线方向、起降或 HOLD 请求当逐点机头朝向。缺省与显式0°区分，原连续请求保留在草稿／计划身份中。

上传采用声明的最近1/256°网格，最大圆周量化差1/512°；原请求、上传tick、实际回读原值及比较用规范值分别保留。实际有限Float32回读只有在明确推导界内才归同一tick；非法大有限角度也保原数诊断，不补0。编码步长及语义匹配不是实飞精度、逐位相同、真实固件认证或达向／转向动力学证明。

无heading单机保持原v2；明确heading使用独立v3语义域，混合机组逐成员区分。原严格几何／审阅消费者使用同一编码规则；实际目录声明在审阅与启动前置首尾核对应域，旧声明不自动升级。回读报告新schema2在匹配时也显示原值，最多64条并明确截断。最终StartPermit、actualPath及正式执行结果仍未完成。

本批28源码／工程登记＋32文档=60路径，新增HeadingEncoding.h／cc／md三条；48项工程场景全部未执行。限定源码交叉静读与普通文本／路径／Markdown／空白核对不等于构建或运行验证。UI/UX仅静态指导原Qt表单的独立勾选、字段错误、键盘／换源恢复与原值显示；未运行技能脚本或界面预览。

原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅粗估，不按文件／批次上调。未编译、执行项目／parser／helper／QML／算法／hash、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[朝向编码合同](../../custom/src/Swarm/FleetMissionHeadingEncoding.md)、[真实编辑入口](../../custom/src/Swarm/FleetMissionDraft.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方535及更早为历史。


## 535 当前：异机完整包络冲突检查（源码静态收口）

沿任务执行与安全主线新增异机声明包络比较：同次启动整域和 NAV／起降参考体的完整原生组件进入全异机组合，保机体、支撑及误差余量；不从报告重建名义线段或只取前缀。闭边界接触也属于交叠候选，地形接触 pad 不豁免另一架飞机，无可靠同步时序按全时段空间保守比较。

完整双方平面进入整数几何求解；新比较共用有界预算与原取消信号。缺失完整几何、求解未知或超预算不算通过。Worker 结果升级 exact8，普通／地面审阅、Witness 和两 Authority 接入同份报告及共同结论，不是只新增显示字段。

旧 minimumSeparation 检查不被替换；外包交叠不是实测碰撞，无交叠也不是实际路径认证。原 `actualPathCertified` 与最终 StartPermit 缺口保留，未新增飞行命令。原25目标／六类实质剩余继续，约40%源码／60%剩余仍仅粗估，不按本批路径数上调。

本批15源码／工程登记＋21文档=36路径，新增纯值模块h／cc／md三条；源码实现及限定静态交叉审阅已收口，48项工程场景全部未执行。仅源码、直接静读及普通文本／路径／Markdown／空白核对；未编译、运行项目／算法／parser／helper、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。

[异机包络合同](../../custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.md)、[后台与准入](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方534及更早为历史。

## 534 当前：参数导入原生发送与固定重试（源码静态收口）

继续533实际导入入口，将私有typed选中值交给PM独立原生事务；接纳后不再经Fact通用setter乐观改当前值。事务固定原飞机／PRIMARY／参数值，独立有界调度与重试；不借旧同名等待队列或当前Fact值重发。正常关闭预览不取消已接纳事务，参数页增加原事务状态与准确ID停止剩余发送入口。

新Vehicle私有桥在插件调整后及writer入口前核原来源与固定payload，先记实际writer入口事实再调用；APM按真实组件保numeric-float转换，不可精确表示的整数整批拒绝。最早RAW接收冻结当时已有事务的票据，后来的新事务不能领取旧包；同值报告仅称匹配，不是因果ACK、配置认证或飞行许可。CMake同步实际编入APM与共享特性开关。

100ms独立调度，每轮最多扫描256行／尝试64行，每行至少3秒、首次后最多5次重试，整次120秒在发送与RAW取票／消费两端复核。UDP／Mock原Link可在工作线程；取消期间的旧终态不被尾部逻辑覆盖，页面销毁后不再续读旧对象。

本批11源码／工程登记＋16文档=27路径，新合同2份；源码实现及限定静态交叉审阅已完成，64项工程场景全部未执行。UI/UX仅静态指导原Qt控件的状态、准确ID取消、键盘焦点及窄窗换行，没有执行技能脚本或预览。未编译、运行项目／parser／QML／业务／定时器、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%源码／60%剩余仍仅粗估。

[原生事务](../../src/FactSystem/ParameterImportTransaction.md)、[发送桥](../../src/Vehicle/ParameterImportTransport.md)、[界面与接纳](../../src/QmlControls/ParameterEditorController.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方533及更早为历史。


## 533 当前：参数文件完整预览与精确提交关联（源码静态收口）

真实产品入口是ParameterEditor载入文件和ParameterDiffDialog人工确认。旧预览会跳过坏行、未核完整字段和类型，并借用live metadata生成临时Fact；未知枚举及重启标志可能影响当前参数定义。本批复用现完整typed文件校验，建立独立预览及私有候选，并用原预览token关联提交、选择和关闭。旧页面不显示或改动后来模型，载入失败显示明确原因。

文件可表达十种数值类型，但现PARAM_SET发送器只正确支持七种；选中的64位整数／Double必须在第一项提交前明确拒绝。逐项写入前后及最终通知尾复核原对象、值和当前写入条件；中断保留已尝试项可能提交的事实，不宣称批量回滚或机载ACK。同一行旧sender内部的最终来源连续性仍需专用提交桥，不能用caller前后检查代替。

本批4源码／QML＋13文档=17路径，新增1份合同，48项工程场景全部未执行。源码实现及限定静态交叉审阅已完成；未编译、运行QML／parser／参数／业务IO、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。UI/UX技能仅静态指导原Qt控件的错误反馈、状态与恢复路径，没有执行技能脚本或预览。原25目标与六类实质剩余保持，约40%源码／60%剩余仅为粗估。

[实际导入合同](../../src/QmlControls/ParameterEditorController.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方532及更早为历史。


## 532 当前：多机元数据后台调度（源码静态收口）

继续实际多机连接链：Artifact校验、Summary解析与Translate翻译接入同一进程FIFO预算，最多2项运行／32项排队，排队120秒并在获槽前复核期限。队列满、未启动取消／超期与运行失败分别处理；原取消后仍start的两个清理旁路移除，避免突破运行上限。

运行项保留原输入所有权及result／actual finished双尾，取消不提前归还运行槽。未启动失败不作为缓存损坏、CRC失败或重下载依据。这个预算只涵盖上述两类worker，不涵盖HTTP／FTP、所有Qt线程、旧显式同步翻译或整进程内存／CPU。

本批10源码／工程登记＋17文档=27路径，新增3条；40项工程场景全部未执行。源码实现及限定静态交叉审阅已完成。未编译、运行项目／线程／parser／helper／下载／业务IO、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍是粗估。

[调度合同](../../src/Vehicle/ComponentMetadataWorkScheduler.md)、[实际元数据消费](../../src/Vehicle/ComponentInformationManager.md)、[实际翻译消费](../../src/Vehicle/ComponentInformationTranslation.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方531及更早为历史。


## 531 当前：参数缓存 v3 与真实 HASH 接线（源码静态收口）

替换实际参数缓存的 generic QVariant／QMap反序列化及按声明宽度盲读：新增4MiB／16384行的独立v3纯值格式，明确头、身份声明、十型primitive LE、排序唯一名称和完整EOF；坏文件整份miss。旧.v2保留，不解析、不删除或降级读取。Float／Double保实际IEEE原表示，Fact容器通知尾同步按真实位模式核NaN／Inf／正负零，不把非有限值当健康证据。

实际PX4 hash按本地6C源码核对，只覆盖已使用非volatile参数的名字与Int32／Float原4LE字节，初值0／无末xor；十型文件格式不等于十型PX4 hash。RAW HASH在PM最早可见入口绑定原PRIMARY及UInt32／原count，读取、metadata和发送尾持续核来源与真实参数对象，不借后来链路处理旧包。

CRC与总数都不能证明volatile名称／used-index未变化。零volatile才可按完整候选快载，全部actual Fact／metadata／目录／原来源及值通过后才无外调批量信用索引，再在无pending write及索引状态复核后，走受控完成与原固定HASH ACK桥；含volatile只预填非volatile模型，不清任何真实接收等待、不发HASH ACK，正常机载流／原重试读齐后才完成。自身回放不递归写缓存或逐行提前ready，真实接收／刷新／写入永久退休旧attempt，其独立普通完成不被旧cache卡住。写入改QSaveFile无direct fallback及完整块／commit检查，取消假进度动画；已发生的发布／writer事实不伪造回滚。

本批7源码／工程登记＋15文档=22路径，新增3条；64项工程场景全部未执行。仅源码修改、直接静读与普通文本／路径／Markdown／空白核对；未编译、执行工程／codec／parser／CRC／helper／参数／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标与六类实质开发保持，约40%源码／60%剩余仍仅粗估；后台全机队调度、其它factory内部寿命及完整执行／运营仍有源码工作。

[缓存格式](../../src/FactSystem/ParameterCacheFile.md)、[实际消费](../../src/FactSystem/ParameterManager.md)、[IEEE容器值](../../src/FactSystem/Fact.md)、[22路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方530及更早为历史。


## 530 当前：离线参数真实值与整份发布（源码静态收口）

修复真实离线编辑入口：旧代码把文件数值解析到局部变量却从未赋给 Fact，非零参数仍为初始 0；负 Int8 又使用无符号转换。本批新增纯值 codec，完整接纳五列文件和十种数值类型，再建立实际 Fact-owned 元数据与参数值候选，全部通过才发布映射、组件计数、总数、默认组件和 ready 状态，不保坏尾的成功前缀。

输入最多4MiB／65536行／16384参数，行内容4096字节、数字字段128字节；整数先范围后转换，实数有限且拒非零下溢，保浮点负零。实际 Plane 注释含 NUL，先跳过有界原始注释而不误拒内置模板。文件 system ID 仅历史声明，不改离线 Vehicle ID；构造期间不要求尚未安装的 Vehicle.parameterManager 或在线握手。

候选保持原 owner／offline／插件／模型及实际版本／来源，通知和工厂回调后重核；两版本监听就位后也拒改回原值的旧候选，失败只清本批未发布对象。Fact 容器输入另修数值相等却类型不同或零符号不同的跳赋值，仍不发参数写回信号。离线值不成为实机观测、构型认证、性能证据或 StartPermit。

本批7源码／工程登记＋15文档=22路径，新增5条；源码实现及限定静态交叉审阅已收口，48项工程场景全部未执行。仅源码修改、直接静读与普通文本／路径／Markdown／空白核对；未编译、执行项目／parser／helper／参数或文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。二进制参数缓存校验、全机队后台调度仍有源码工作；原25目标及六类实质剩余保持，约40%源码／60%剩余仅粗估，不按批次数上调。

[纯值格式](../../src/FactSystem/OfflineParameterFile.md)、[实际加载](../../src/FactSystem/ParameterManager.md)、[类型／零值](../../src/FactSystem/Fact.md)、[22路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方529及更早为历史。


## 529 当前：后台有界翻译与合法递归（源码静态收口）

沿真实元数据翻译链继续，不关闭本地化功能。新增纯值有界核心及两段后台作业：摘要读取／locale URL选择 → 主线程有界下载 → TS解压／XML／JSON／规则执行／唯一输出。生产入口绑定原生summary／metadata artifacts及独立请求ID，实际worker自己持输入；不是仅把QString传入线程。

原实际fixture包含随list／items下降数据的合法recursive-def，不能按def全局visited拒绝。新核心区分真实物理数据位置和TS显示context，仅拒无进展的活动规则／引用循环，另对JSON全树、XML文本及翻译工作分别计预算。数字list key对应实际PX4枚举／bitmask，修复旧toString变空context；字符串、数组、wildcard、globals及合法DOCTYPE TS保兼容。

协调器以actual worker／阶段／代次及result+actual finished两尾才采用结果；source退休／析构按原ID及两artifact准确取消，后台未结束不销毁线程或提前释放输入。新native完成信号与legacy路径调用者分离，不接管外国输出。同步translateJsonUsingTS仍保借用／qrc输入及原生唯一输出，复用同有界核心。

本批9源码／工程登记＋16文档=25路径，新增6条；源码实现与限定静态交叉审阅已收口，48项工程场景全部未执行，不代表运行或可靠性验收。未编译、执行项目／parser／helper／规则／线程／XML／JSON／下载／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍为粗估，不以路径或批次数上调。

[有界核心](../../src/Vehicle/ComponentTranslationEngine.md)、[后台作业](../../src/Vehicle/ComponentMetadataTranslationWorker.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。


## 528 当前：下载原件唯一所有权（源码静态收口）

继续真实组件元数据入口，处理同名远端文件共用临时路径的问题。新增原生 fresh 临时文件 owner，HTTP／MFTP 和 TS 下载使用独立 request ID／完成信号；不是下载后再从共享 basename 复制。原 legacy 固件、相机、参数包下载接口及其大文件／本地路径接受域保留。

新传输持 owner 到写句柄关闭，Manager 核实际 provider、原 ID／path／来源／阶段，worker 自己持输入到实际结束；TS 处理栈另持原件，summary／metadata artifact、解压输出和译后 JSON 所有权分开。受理 false／同步终结、准确取消、旧回调和通知中销毁均须限定静读，不借共享 cancel 清他人。

新 HTTP／FTP 原件上限32MiB并各有120秒总期，检查完整读写／关闭；HTTP 有限重定向、Cached 原任务最多一次 cache-only 回退；FTP新增模16序号回绕、4096缺块预算和零进展包拒绝。CRC／schema 的既有边界保留；唯一文件不认证 TLS、机载身份、PRIMARY wire 来源、真实性能或 StartPermit，亦不保护任意违规 native caller 并发复用同 lease。

本批15源码／工程登记＋19文档=34路径，新路径6条；源码实现与限定静态交叉审阅已收口，64项工程场景全部未执行，不代表可运行版本或完整可靠性验收。未编译、执行项目／parser／helper／CRC／hash／下载／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%源码／60%剩余仅粗估。

[输入所有权](../../src/QGCOwnedDownloadFile.md)、[接线](../../src/Vehicle/ComponentInformationManager.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。


## 527 当前：参数定义整份接纳与实际Fact副本（源码静态收口）

继续原组件元数据的真实消费链，而不是新增只读报告。已核旧CompInfoParam在验证前关闭fallback、逐条发布live map，通用factory错误又返回非空默认对象；坏尾、重复加载或未知类型会留下半批／旧数据。本批已实现参数专用整份校验、候选目录一次切换及明确失败回退，不改原通用FactMetaData factory的其它内部资源接受域。

本地PX4实际jsonout生成version1，虽vendored／上游schema已是不同版本，不能仅按最新schema替本机猜类型／版本。参数专用解析以实际七数型为依据，明确v1严格数字String兼容、最终min／max／default／increment、完整values与bitmask及安全移位；未知扩展保容忍但计资源预算，不冒称全draft-schema或设备真实性认证。

真正的三个Fact创建入口改为actual Fact-owned元数据副本，避免目录替换时删除被Fact借用的模板；缓存hash只查询已知volatile值，未知／类型冲突放弃快载而正常读参数。已存在Fact保自己的旧副本，不称热更新或当前构型重新认证；CRC副本／字段本身不证明构型、性能或执行许可。

Manager在准确旧source退休及新GENERAL前清实际各组件参数目录资格。独立候选／旧目录的回收、XML fallback所有权、实际source／代次与QObject通知重入已完成限定静态交叉审阅；不要求initialConnectComplete，不复活旧批准或新增参数写入／飞行命令。

CRC先冻结原来源再跨观察失效通知／provider创建，防旧HASH重基新源；不用会被正常参数就绪推进的观察epoch，另冻本地目录revision。原HASH ACK改走私有固定消息门，连接虚调及插件调整后再核来源／原payload，到write入口前最后复核；进入原writer是已调用事实，不冒称驱动完成、字节已发送或远端接纳。其它generic sender不改，也不增加控制协议。

本批12源码／工程登记＋13文档=25路径，新路径4条；58项工程师场景全部未执行。未编译、运行项目／解析／helper／CRC／hash／下载／缓存／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标和六类实质剩余保持，约40%源码／60%剩余仅工作量粗估。

[参数解析](../../src/Vehicle/ComponentParameterMetadata.md)、[实际消费者](../../src/Vehicle/CompInfoParam.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 526 当前：组件元数据字节校验与后台快照（源码静态收口）

回到能力证据的实际接入链：原组件Manager下载和缓存命中只把声明CRC作缓存标签，不比较实际文件；本批已写入统一后台原始字节核对及独立JSON快照，最终仍供原general／参数／事件／执行器消费者使用。本批源码实现及限定静态交叉审阅已收口，不是可运行版本或完整能力认证完成。

新raw-v1_缓存只保原始传输字节，每次命中重新校验，不把旧解压cache当原件；坏命中最多精确退休原tag一次，两文件移除确认成功才重下原URI一次。当前解析只消费本次独立JSON，不消费insert返回的旧路径；原raw／JSON快照与可转交缓存候选明确分开。

CRC复用现MAVLink算法，初态0且无终异或。当前配套PX4生成脚本给通用元数据计算未压缩JSON、URI却为xz，子元数据计算压缩文件；因此仅实际PX4 GENERAL允许明确decoded-px4-general兼容域，其它仍核传输件。无CRC保持not-compared，不补认证、真实性能、构型或执行权限。

后台固定raw32MiB／JSON32MiB／字典64MiB，最多三份独立唯一临时文件；分块、EOF／错误、取消及实际worker结果和finished双尾分别处理。原始输入不移除，快照由实际所有权释放；有界新解压入口与旧两参数入口分开。三份合计最多96MiB临时磁盘，不声称总解码内存仅64MiB或绝对IO／CPU时限。

通用目录在新batch先退休旧来源，候选类型／CRC／URI整份通过才一次安装；坏文件不保旧目录或成功前缀。参数定义工厂完整schema及半装问题仍待后续处理，不因文件CRC匹配宣称定义已验证。

Manager冻结本次来源、PRIMARY、请求／阶段和实际worker；请求回调及InitialConnect完成回调补实际QObject寿命。Vehicle关闭／移除／析构先退休本通道，避免部分析构中继续旧步骤；可选步骤失败终结不是文件成功。初始加载不要求initialConnectComplete，不借共享FTP.cancel清别人，不把文件一致当远端PRIMARY字节、HTTP端点或设备身份认证。

翻译改为本次唯一TS／译后JSON输出，完整读写及XZ各32MiB／字典64MiB；冻结原下载器／URL／代次、先退旧槽再通知，迟回只清本次唯一输出。译文不是原CRC核过内容；底层下载basename碰撞、递归引用图深度／CPU与全部临时内存仍是后续缺口，不称全部跨机输入隔离或完整图预算已解决。

本批18源码／工程登记＋15文档=33路径，新路径7条；48项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／CRC／hash／解压／下载／缓存实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、PR更新或打包；没有实际下载、读写或删除业务缓存／归档。原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估。
[加载接线](../../src/Vehicle/ComponentInformationManager.md)、[后台快照](../../src/Vehicle/ComponentMetadataArtifactWorker.md)、[原始缓存](../../src/Vehicle/ComponentInformationCache.md)、[翻译输出](../../src/Vehicle/ComponentInformationTranslation.md)、[33路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 525 当前：已核备份后的单个退休原件永久清理（源码静态收口）

补长期运营的真实磁盘退役路径：旧retire／restore仍只改名，备份仍另存；新增独立只读预览及明确永久清理一个retired原件的入口，不触当前库、sealed段、旁路或备份，不批量、递归或自动清理。本批源码实现及限定静态审阅已收口，不是可运行版本、放行或完整长期留存策略完成。

依据只来自原实际backup worker的完整backup31、backupVerified且source.kind=retired，并等原resultReady与actual finished双尾；公开last、文件名、hash、目录或导入材料不能造删权。新原生私有basis／原preview绑定本次操作；请求固定五个字符串，仅preview／commit／discard／cancel，不能携任意路径、文件Map或proof。

预览worker只读核原完整SQLite／double SHA、两目录与实际Windows文件对象；提交消费原30秒预览并重新核原两file ID及同handle字节，再以原源handle请求删除。非Windows明确unsupported，不用按路径删除或跨平台不安全回退。已核副本不等于异地介质、原始事件真实性或掉电耐久。

提交另以fresh操作UUID创建不可覆盖的小型意图／终态收据；意图完整写入及flush确认前不删除。删除调用、可能删除、原件事后缺失、备份后核、两handle关闭及收据错误各自保留；晚到取消或终态收据失败不伪造回滚。sourceLogicalBytes仅原件逻辑大小，diskSpaceReclaimedVerified仍false，不声称物理空间已释放。

原维护页内嵌独立小面板，经原archive.retiredPurge门面显示原件／副本与两项确认；旧History Loader不新增控制器或服务。交叉审阅修正真实137字符备份名误拒、结果接纳后按钮自锁和字段通知中销毁漏取消；焦点沿实际Flickable滚动。UI/UX只静态指导原Qt主题、独立确认、来源文字及键盘反馈，未运行搜索脚本、预览或交互验证。

本批10源码／资源＋15文档=25路径，新路径仅PurgePanel.qml及合同两条；64项工程场景全部未执行。未编译、执行项目／parser／helper／业务／QML／JS／模型／认证／SQL／hash／原生Windows API实现、测试、仿真、操作硬件／PX4、刷机、放行、提交、更新PR或打包；没有实际删除、创建或读写业务归档。原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估。

[原生合同](../../custom/src/Swarm/FleetPersistentEventStore.md)、[worker](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)、[界面](../../custom/src/Swarm/FleetRetiredArchivePurgePanel.md)、[25路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 524 当前：已闭部署归档与本地容量回收

回到任务执行生命周期，新增明确选择旧本地组、独立选路径和确认后的归档／退休流程。原最多8次终身分配限制改为最多8个live或等待实际销毁的组；终身分配尝试另以uint64字符串累计，不回绕、不重置。没有自动淘汰、增大容量、复用旧对象／批准，或重建Orchestrator绕过128条受保护记录限制。

只允许非current／非当前监视、原实际来源完整的旧组。原生检查全部初始及最多8个ground事务／Run、真正静默栈及实际Swarm整张owner表无原占用；有MODE准备的Run须原独立Close已提交，明确从未began的原unissued关闭例外不靠空值猜测。完整pristine未发布失败组可为0报告；注册task失败的真实未发Run仍完整保在quiet材料，不造任务。已began但未issued分支不再自动退休而堵死人工Close，仍走既有双账号关闭，不放宽归档资格。

后台复用原protected8完整报告codec，逐原record16生成0..9份报告，保完整quiet18和独立document15，总32MiB／单报告2MiB。保存、commit、回读与最后原生退休分开：commit不明不称未写，成功后取消／回读失败仍保文件已写，但不退休。文件回读匹配后还须原对象、完整quiet及全部record逐值尾核；不把hash、文件内容或操作员勾选当当前原生来源。

专用native批次只退休准确已闭记录，普通removeTask仍拒受保护记录；同批清其本地语义缓存，不删除排队历史事件或他人占用。旧组及全部受控QObject后代（最多256个）弱引用均实际为空，并在后续turn复核后才回收8槽；父destroyed信号和deleteLater不算全部销毁。原source-lost监听按准确组隔离，退休旧组不毒化后来current，也不把本地回收称停飞、落地或任务Completed。

原部署管理弹窗新增归档区；Root只给typed九字段窄请求及状态／结果，不公开Episode、账号或引用scope。独立打开文件可无live飞机／账号，不恢复任何对象、观察或权限；只显式选一份原生canonical报告文本，按2048UTF16分片查看，不在GUI解析／序列化整包。文件时间明确仅文档声称；弹窗重开可只读查看原生上次事实，但不能认领为本页新请求。UI/UX仅指导原Qt主题、分层确认、长标签／错误反馈和键盘操作，未执行技能脚本或预览。

外来job即使打开声称同archiveId的另一文件，也退休本页旧历史目录与报告；本页open受理同样先撤旧历史资格。界面忙碌期间被挡的job通知在原调用票释放后补核，不能用后来state将新文件报告拼在旧标题下。这里只撤界面资格，不删除磁盘文件或伪造取消别人的任务。

本批19源码／资源登记＋20文档=39路径，新路径6条；无新QML后端服务、wire、外部依赖、SQL schema、飞控或PX4修改。56项工程场景全部未执行，只做源码、直接静读和普通文本／路径／Markdown／空白核对；未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。完整25目标和六类实质剩余保持，约40%源码／60%剩余仍为工作量粗估；本批不证明最终StartPermit、正式远端任务结果或全部长期运营完成。

[生命周期合同](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)、[归档worker](../../custom/src/Swarm/FleetDeploymentArchiveWorker.md)、[界面合同](../../custom/src/Swarm/FleetDeploymentArchivePanel.md)、[39路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[56项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 523 当前：GNSS 历史集合与独立文件工作区

将522的真实同包报告接到原FleetOperationalMemory的独立历史集合：只允许实际Archive及准确原生报告token追加，不接受QML报告／坐标作为来源。保每页完整33字段、原7身份及18字段样本；按run／headerHash／原sequence去重，不依会随文件增长变化的fileHash，也不把采集Run或数字机号当可信任务／实物身份。最多32报告／2048样本，重复或容量满整份拒绝，不自动裁剪去重依据。

旧运营观察、KPI、热区推断、5000条裁剪及旧schema1导入／清空完全不接这批记录。新增共享纯值报告与集合校验；它只验证文件内容自洽，不认证原始采集、设备、精度或飞行状态。无GPS和无fix历史仍保诊断，不补旧坐标、不计算任务完成率或环境风险。

新增显式冻结、选路径后另行确认保存、独立只读打开、每页8份目录及单份报告查看。文件root为新闭11字段，4MiB上限；冻结保原Memory instance／revision和不可变字节，打开文件不导入active记忆、不恢复权限。单槽worker只消费纯值，实际结果与线程结束双终结后接纳；取消不抹掉已commit文件，写入已知／未知与readback、fsync／durable分开。

提取共享FleetSnapshotFileIo，让维护与GNSS工作区使用同一路径检查、64KiB读写及QSaveFile提交／回读逻辑。原维护32MiB、错误码和回执边界保持，GNSS用4MiB；不是恶意并发／祖先reparse／失电安全认证。本轮只改实现，没有实际创建、读取或写出任何业务档案。

历史面板新增独立记忆区：追加、冻结、保存、打开、选报告与选地图均须明确动作，不自动读写／续页／选首条。原地图共用唯一选择，投影新增origin区分原生页、session冻结历史和文件历史；只绘当前选中的一份离散点，不把文件值冒充当前Archive或实时定位。UI/UX静态指导用于原Qt主题、分层确认、文字来源和键盘操作，未运行技能脚本、预览或交互验证。

本批18源码／资源登记＋18文档=36路径，新路径8条；无新QML后端服务、wire、外部依赖、数据库schema、权限或PX4修改。仅源码、直接静读及普通文本／路径／Markdown／空白核对；48项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。

原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估。本批是有界历史留存与重开链，不是全历史自动索引、自动留存清理、跨任务统计、链路／风场模型或最终StartPermit；active集合满时仍须明确拒绝，不通过清文件槽偷偷清空。

[运营记忆合同](../../custom/src/Swarm/FleetOperationalMemory.md)、[文件工作区](../../custom/src/Swarm/FleetGnssMemoryWorkspaceWorker.md)、[界面](../../custom/src/Swarm/FleetGnssMemoryWorkspacePanel.md)、[36路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 522 当前：同包历史 GNSS 诊断与显式只读地图

为集群记忆地图接入第一条真实历史地理依据：使用现固定本机遥测档案的原生已完成页，另行确认后只解释同一GPS_RAW_INT帧的位置、fix、HDOP与卫星数。最多64帧，先复用现MotionReview完整页校验再解码；不从Registry、实时Fact、其它消息或分页补位置与质量。没有GPS的合法页明确为空，不等于区域安全或从未出现异常。

fix仅3／4／5／6／8且经纬度合法才发布该帧报告坐标；0／1／2／7及未知fix只计未定位，旧坐标不投点。原eph65535与卫星255保未知，不补0；HDOP无单位、不是米精度。HDOP>2.5或卫星<6只是固定显示提示，质量未知与阈值内分开，均不认证定位精度、漂移原因或环境风险。

Archive保实际页来源与独立新报告UUID。原六字段action与十一字段回执只新增gnss-review动作；原运动／四视角报告和维护流程保持。界面独立确认GNSS位置发布，再另行明确选择地图，不自动选图／续页／拟合视野。地图只绘离散历史点，含质量文字图例，点与标签吞点击，不接Goto、任务编辑或飞行控制。

地图选择只接受实际报告token，Root自行读取原生来源；原owner／完整报告／token与独立选择代次绑定。输入、隐藏、换源或新查询退休本页选择，旧页面不能关闭后来替换的选择。公共投影固定深度冻结；不支持冻结时不绘图，不把脱离副本当不可变保证。

本批8源码／工程登记＋13文档=21路径，新路径仅GnssReview h／cc／md三条；无新QML资源／服务、wire、依赖、数据库schema、权限或PX4修改。UI/UX技能仅指导原Qt主题、独立同意、文本图例和键盘焦点，未运行技能脚本、预览或交互验证。

仅源码修改、直接静读及普通文本／路径／Markdown／空白核对；40项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍为粗估。本页图层不是持久跨任务全集、风场／链路风险地图或最终StartPermit。

[原生报告合同](../../custom/src/Swarm/FleetTelemetryGnssReview.md)、[历史工作流](../../custom/src/Swarm/FleetTelemetryHistoryPanel.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 521 当前：普通区域任务的速度／续航准入下限

补通普通区域任务的两项性能要求：最低速度能力minimumSpeedMps（m/s）和最低续航能力minimumEnduranceSeconds（秒）。现有性能政策编辑器将原strict三态与两个可选下限一次提交给原草稿，不分次保存半套政策；空白不补0，显式0与缺省保持不同来源身份。这里不是巡航速度指令、当前剩余飞行时间或实测性能认证。

新增原生三键完整政策CAS：只替换requirePerformanceEvidence及两floor，保其它settings；任一下限存在（包括0）使用planningSource schema6。schema1..5不能夹带新字段，6缺全部floor也拒绝；旧无floor来源不自动升级。普通设置编辑、检查点保存／恢复及修复仍保完整来源、准确revision与原datum重确认，旧审批不恢复。

Matcher与角色分配在选择前、Compiler在固定主／备用成员上都执行正下限；缺值或Registry实际−1不能通过，不拿其它机的最大值抵扣。strict=true仍只取本机当前构型匹配的工程声明；false／未声明可用原规划描述符，但不能关闭明确下限或宣称可信实测。0／缺floor不新增正值证据依赖，当前电池与逐机能量门不变。

Compiler任务根、归档计划和PayloadBuilder按presence、类型和完整来源值绑定两个下限，不能在保存、修复或直接编码时丢字段／降级。界面复用原Qt主题和UI/UX静态指导，明确单位、未知／0、采用／取消及失败反馈；没有运行搜索脚本、预览或交互验收。

本批9源码＋18文档=27既有路径，无新类／资源、wire、依赖、数据库schema、权限或PX4修改。仅源码、直接静读与普通文本／路径／Markdown／空白核对；48项工程场景全部未执行。未编译、执行项目／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍为粗估，不按字段或控件数上调。

[草稿合同](../../custom/src/Swarm/FleetMissionDraft.md)、[性能政策入口](../../custom/src/Swarm/FleetPerformancePolicyEditor.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 520 当前：备用出勤显式只读刷新与本次请求归属

为519的被动参数读取补上独立、人工确认的刷新入口。复用现有FleetMissionEvidenceRefresh读取实际版本及固定29参数，不新增发包服务、参数写入或飞行命令；原账号／角色、真实地面未解锁、PRIMARY链路和占用门禁保持。

本次刷新绑定原私有capture、明确选中的备用机、实际scope／刷新对象及真实requestId／generation。开始前退休旧参数offer、提案和审阅资格，但保留来源与已归档事实；状态与取消只认本次原请求，不认同数字ID或共享latest。界面离开／换源先撤本次资格，取消仅停止后续读取，已排队请求和晚到回包可能仍在途。

刷新结束只显示实际结果，不等于12参数仍满足30秒新鲜度。操作员仍须单独重新读取本次依据、明确采用预算速度和新配置，再走原完整审阅及审批；不自动刷新、重试、observe、采用、批准或恢复旧权限。同来源只保本页手填的动作／坐标和已明确采用的启动策略纯拟值，未采用启动策略子表单须先处理。

本批2源码＋13文档=15既有路径，无新组件／资源、C++、wire、依赖、数据库schema、权限或PX4修改。仅源码编辑、直接静读和普通文本／路径／Markdown／空白核对；工程场景全部未执行。UI/UX静态指导用于原Qt主题、请求归属、状态区别、反馈与键盘焦点，没有运行搜索脚本、预览或交互验收。

未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持；约40%源码／60%剩余仍是粗估，不因刷新按钮或字段数上调。

[刷新工作流](../../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[面板合同](../../custom/src/Swarm/FleetReserveDispatchPanel.md)、[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[未执行工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 519 当前：备用独立出勤的真实参数绑定与结构化配置

原备用出勤要求人工填写完整executionProfile和参数hash。本批为实际冻结capture／选中reserveId接入现Coordinator原生12参数只读offer；Root保留私有本次offerToken、原对象与参数fingerprint，生成提案前重新读取并核对。外来Map、同值ABA、旧token或另一备用的参数不能代替本次真实来源；原生参数读取不刷新或写入参数。

新增纯值配置编辑器，不伪造Draft。操作员明确采用本次备用预算速度，再选择起飞／结束动作、AMSL坐标、时序和完整启动策略；requiredProfileHash只取本次实际offer，不再手填。复用原启动策略编辑器，完整JSON仅供核对；没有从旧主机、GNSS或模板补起降位置和安全默认值。巡航与返航仍须同原备用cruise，垂直速度仍须匹配原预算和实际参数；不兼容时回原计划重做，不静默修正。

仍走原reserve-dispatch Planner→Compiler／能力与能量／当前电池审阅→归档／采用／独立批准／共享部署流程。新三参数preview入口要求准确offerToken，底层原exact3请求、profile schema1与启动策略1–4不改。读取和生成配置都不是上传、接管原任务、性能认证、StartPermit或自动出勤。

本批5源码／资源接线＋13文档=18路径，新路径仅ProfileEditor.qml和对应合同md两条，无新C++、wire、外部依赖、数据库schema或权限。40项工程场景全部未执行；UI/UX技能只指导原Qt主题、分步确认、完整输入／错误反馈与静态焦点核对，未搜索、预览或交互验收。

仅源码、直接静读和普通文本／路径／Markdown／空白核对；未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估，不按表单或字段数上调。

[实际工作流](../../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[配置编辑合同](../../custom/src/Swarm/FleetReserveDispatchProfileEditor.md)、[修改清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 518 当前：维护护照显式保存与独立历史工作区

将517的本进程维护记录接到实际可见的文件工作区：显式冻结当前内容、另选路径并确认覆盖风险后保存；选择已有文件并另行确认读取，成为独立只读历史。当前护照不被历史load/import替换，文件、hash和来源组不认证实物、维护状态或权限。

冻结保原实际records／instance／revision，GUI分turn只验纯JSON类型和有界结构；文件读写、JSON规范化、评估与完整文本分页交独立单槽worker，原job结果与线程结束双尾后才接纳。517实际追加可继续，旧快照内容不会改成新记录；来源版本变化只使sourceCurrent失效，不悄悄重导出。

来源组、记录目录每页8项；完整记录／评估按2048 UTF16片浏览，长备注不整包交界面。保存分fileWrittenKnown／fileWritten与独立readbackVerified，commit失败不谎称未替换，commit成功不被取消或回读失败抹掉。QSaveFile禁直接写回退，fsync／durable／认证仍false。纯legacy根5、历史根6与原严格来源元数据保持。

本批10源码／资源接线＋13文档=23路径，新路径为worker h／cc、工作区Panel.qml和合同md四条。48项工程场景全部未执行；无新QML服务类型、wire、外部依赖、SQL schema、权限或PX4修改。UI/UX指导仅用于现Qt主题、分步文件同意、只读来源与错误反馈的静态设计；未搜索、预览或交互验收。

未编译、执行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估；本功能不等于长期运营／维护预测整体完成。

[文件工作区合同](../../custom/src/Swarm/FleetMaintenanceWorkspace.md)、[修改清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

## 517 当前：真实历史电池证据进入维护护照

从当前已完成的原生遥测历史页显式选择完整电池来源，预览本页报告温度／非负放电电流峰值和样本覆盖，再另行确认写入实际维护护照。未知、越界、充电与零值分开；不从实时Fact补历史，不新增遥测请求或采集流。

以Run／头摘要／文件摘要／sourceEpoch／机号／组件／电池槽七字段隔离历史来源组，不与同数字机号的手工记录混合。相同来源组的选中序号闭区间重叠拒绝；容量2000满时明确拒绝，不淘汰去重依据。schema1文件保完整历史元数据并原子拒绝重复／重叠；历史页峰值不计算物理两点趋势，不补振动RMS、寿命或飞行次数。

新增嵌入式维护页与现指挥中心来源组摘要；私有Archive／Passport由Root实际配对，公共接口不接任意record或raw payload。页面失效、换源、隐藏、选择／同意ABA不得沿用旧确认；已追加事实不因界面退场被当作回滚。只保本进程记录，本入口不自动落盘或恢复权限。

本批10源码／资源接线＋13文档=23路径，新路径仅Panel.qml与Import.md两条。40项工程场景全部未执行；未编译、执行项目／helper／QML／JS、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。仅源码、直接静读和普通文本／路径／Markdown／空白核对。原25目标与六类实质剩余保持，约40%源码／60%剩余仍是工作量粗估；本功能不代表目标21整体完成或当前飞机适航。

[实际数据与API合同](../../custom/src/Swarm/FleetMaintenanceHistoryImport.md)、[修改清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方为历史。

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

## 510 当前：原生Run语义事件接入任务历史

受保护原生TaskRecord提交后的注册、MODE本地交接、终点候选、连续性异常、源退休和观察关闭，新增独立低频事件通道进入实际BlackBox及其持久归档，不再只停留在内存观察通知。逐包计数／年龄／临时sourceCurrent不作为事件变化；同Run的异常位图保历史已观察事实。

独立FIFO最多512行／2MiB canonical payload、单条4096字节，每turn最多64条；不是堆内存硬上限。溢出保旧事件并累计缺口，原生临时收据只在实际发布期间有效；缺口不归入假任务。Root另记本地接纳／未记录／未知，BlackBox接纳不等于SQL提交或完整历史，持久录制仍默认关闭。

新v3固定12字段脱敏投影保28字段runObservation；原v1／v2、SQL及历史查询schema不变。实际按taskIdHash下钻和两历史面板新增只读摘要；现报告将reviewRequired=false普通转换作为info，异常只作warning人工复核，raw保原事件。日志timestamp是实际入日志的本机UTC，原捕获UTC另保，不对队列事件偷偷排序或改写时间。

[事件合同](../../custom/src/Swarm/FleetRunObservationEvent.md)、[实际日志接线](../../custom/src/Swarm/FleetRunObservationJournal.md)、[25路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[60项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批8源码／接线＋17文档，新路径仅两份模块合同，无新代码文件／资源／外部依赖。

这仍是本地Run观察，不是任务Completed／Failed、远端实例认证、正式结果、占用释放或飞行许可。原25目标与完整执行／可信性能／自愈机载改派／跨站协作／长期运营／真实模型插件六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按本批路径上调。

仅源码、直接静读及普通文本核对；未编译、执行解析器／helper／业务／QML／SQL／认证／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 509 当前交接：独立审批的视频启停

工程师应从[30路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[64项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[ACK／身份审计](MERIVUS_ACK_IDENTITY_AUDIT.md)接手。不是已编译的安装包或已更新PR。

重点核：真实Capture一次交接、原30秒及双账号专用审批、正常legacy STOP→START兼容、受控单次启停与270回读、Info仅RUNNING变化与其它配置ABA区分、各阶段取消／销毁保partial。摄像头270单流索引与真实视频效果未验证，ACK／reported-target-match不等于物理停流或带宽改善。

原25目标和六类实质剩余继续；约40%源码完成／60%剩余只是工作量粗估。完整自动QoS、任务执行、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态并未完成。所有编译、运行、仿真、硬件及放行工作留给工程师，本轮未执行；未改PX4、刷机、提交、更新PR或打包。下方为历史。

## 508 当前：人工相机流信息／状态读取

新增人工单流相机 Info269→Status270 查询：独立 READ 权限与真实账号／角色／Root 意图，经 Swarm 私有服务、Registry 原绑定、Vehicle 原生 Run 和现有共享消息通道，接到指挥中心／规划页面板。打开页面或原五秒策略不会自动查询；不启停流、配置、降速或恢复。

原入口总8秒包含授权准备，每阶段最多4秒，accepted ACK 后报告窗口最多1秒且只缩短；每项至多一次实际发送。允许真实来源满足检查的飞行状态 metadata READ，不把 RTT 的地面限制直接套入。取消保留已可能发送／ACK／报告事实，不宣称撤回。

ACK 与269／270无nonce，本地匹配不证明无线因果、实际送流／停流、完整目录或带宽变化；270索引兼容性待工程师确认，不支持不回退0／all／广播。complete 不等于两份报告此刻同时新鲜。远端受控视频降级与恢复仍待开发。

[服务合同](../../custom/src/Swarm/FleetCameraStreamCapture.md)、[面板合同](../../custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[28路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批14源码／接线与14文档；新增 Service h／cc／md、Panel qml／md共5路径，没有新外部依赖。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅为源码工作量粗估，不按批次、路径、接口或局部保护数量上调。完整任务执行、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 507 当前：原生相机流报告进入实际只读消费者

新增原PRIMARY相机VIDEO_STREAM_INFORMATION269／STATUS270被动账本，经Registry实际绑定一次读取、Policy第五参数和Guided原五秒周期接入既有指挥中心摘要；不发新请求或控制命令。另修上一轮留下的setVideoMode／setPhotoMode实际caller，以及其toggle／reset模式意图衔接。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

[完整合同](../../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 506 当前交接：相机／视频实际生命周期

本轮修改现有CameraManager／CameraControl→VideoManager真实调用链：本地播放意图、原相机／流选择代次、有限接收器恢复、活动飞机切换和通知回调后的对象存活检查；共享QmlObjectListModel补通知后的自身存活尾核与匹配reset。不是新增只读helper，也不是完整Fleet视频降级／受控停流事务。

[完整合同](MERIVUS_VIDEO_LIFECYCLE.md)、[精确19路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[50项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。先由工程师构建并核对真实Qt／GStreamer时序与原功能兼容；本轮没有更新PR或安装包。保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505 当前交接：现有日志下载的有界缺块补读

本轮修现有LogDownloadPage→LogDownloadController→LOG_REQUEST_LIST／DATA→本地文件的真实消费者：同一目录／批次来源、冻结选择、有界缺块补读、完整写入后位图与去重计数、NewOnly partial及完整刷新／改名后才显示Downloaded。不是另建只读建议helper，也不是完整Fleet控制中心补传。

[完整合同](MERIVUS_LOG_DOWNLOAD_RECOVERY.md)、[精确14路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。失败保partial；实际源过滤不等于最终wire绑定／远端文件认证。END只有best-effort，无自动恢复或新外网上传。未更新PR或安装包。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504 当前交接：独立单次地面往返耗时

504新增独立人工单次定向TIMESYNC往返耗时采集：实际主账号／Role／Root原意图→Swarm私有服务／原生Run→真实同车绑定review→Policy及可见面板。最早四秒包含授权核验和后续100ms轮询，不自动采集、重试或续期，不回退PING或广播。

[26／18／5／7／23／10合同](../../custom/src/Swarm/FleetLinkLatencyCapture.md)、[精确31路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅实际同源匹配回显供诊断；无回应不是丢包率，没有新二审／配置／飞行许可。未更新PR或生成安装包。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503 当前交接：实际接收窗与保守策略消费

503将实际PRIMARY／本车默认component的原始解码帧接到独立五秒接收窗，经Registry实际绑定读取、Policy独立消费、Guided原五秒周期和指挥中心只读摘要。记录接收帧数、编码MAVLink字节率与最大内部间隔；不把它们当可用带宽、RTT、丢包率或远端发布率。

[完整22／9／7与Policy合同](../../custom/src/Swarm/FleetLocalReceptionObservation.md)、[23路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。不新增发送／权限／Timer；已有501配置链与二审不因本地窗口而放宽。没有更新PR或生成安装包。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502 当前交接：人工可见可选遥测面板

502新增FleetOptionalTelemetryPanel可见界面，在指挥中心和地图规划页复用同一actual Root。操作员显式只读采集→另行审阅统一目标→两项确认后独立账号审批→逐项配置与回读；固定62／74／83，原501实际后端和授权合同不变。不会自动采集、Apply、续期、重试或回滚。

[面板合同](../../custom/res/Merivus/FleetOptionalTelemetryPanel.md)、[24路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。既有501后端不因本UI接线增加权限；uiWiringPresent不是运行验证。没有更新PR或生成安装包。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501 当前交接：地面可选遥测配置

[配置模块合同](../../custom/src/Swarm/FleetOptionalTelemetryConfiguration.md)、[Session](../../custom/src/Swarm/FleetOperatorSession.md)、[30路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[46项全未执行工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

501新增独立双账号的地面可选遥测配置：从未来新成功Capture的三条真实native上下文封存不可序列化Basis，在同一最早30秒内独立审阅／批准，再逐项511→510并最终三项510复核。仅固定62／74／83、同一正int32且精确REAL32间隔，目标不得比任一原正基线更短；不启停流、不降频P0、不改视频或编队速度。

start仅认领ready、不写包；100ms后推进九个wire阶段。每条至多4秒，510 accepted ACK后至多1秒且只缩，所有阶段与审批共用原Capture最早30秒。慢链路可能无法完成；截止／失败／取消保留已发事实，不重试、不自动回滚。三项最终回读只给reported-values-match，不认证无线因果、实际发布率、带宽或飞行许可。

没有生成新补丁包／安装包，也未更新PR；本地源码不能直接当作已构建可安装交付物。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500 当前交接：显式510读取，不配置遥测

新增Swarm私有持有的FleetOptionalTelemetryCapture，显式单车62→74→83，实际Session主账号＋独立fleet.telemetry.interval.capture政策＋原Root/controller意图。接纳后100ms轮询才发；原入口固定30s总期，native每项一次510／4s，accepted ACK后至多1s且只缩，必须原ACK与目标244双端齐全。最终write前复核授权、物理来源、完整载荷并冻结receipt基线；三项同原version／transport／mutation／已绑定ledger，已采语义A→B→A不复活。取消保已发／partial事实，先存历史快照再精确释放原query，不宣称撤回或实际频率已生效。

[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)共24路径；[40项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。[模块契约](../../custom/src/Swarm/FleetOptionalTelemetryCapture.md)说明真实对象、期限、来源和部分已发事实。工程师须另验证目标固件510支持、两端顺序与失效／重入；本机没有新可见按钮或可安装产物。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前：真实native lane与有限512等待

先读[18路径精确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[归属边界](MERIVUS_ACK_IDENTITY_AUDIT.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)，再按[42项未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)接手。5源码／13文档，只有源码与文档，没有二进制、安装包或PR更新。

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前：被动报告、接收窗口及只读数值消费者

先读[精确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[原生Registry合同](../../custom/src/Swarm/FleetRegistry.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)，再按[36项未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)接手。只有5源码＋13文档=18既有路径；没有二进制、安装包、提交或PR更新。

本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前交接：6源码／12文档，仍无二进制与安装包

请以[18路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)定位本批，按[36项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)安排后续构建及验证；[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)明确本地ID不消除同source重复wire ACK歧义。修改的是GroundStation现有PID／ESC与公共511来源链，不是PX4飞控。

没有新增Fleet降频执行器／操作UI／权限scope；interval=0仍仅请求固件默认，失败不自动续链或回滚。本工作树也不代表远端PR或包已经更新。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前交接：4源码＋15文档，无二进制／安装包

请以[19路径精确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)定位本批，按[36项未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)安排后续工程验证，并核[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。源码增量是同次Contact起点／原窗口和完整current/top垂直参考，不是飞控修改、实际起飞控制器或StartPermit。

原名义组件／LAND／NAV计数保留；只有新vertical/support指定terrain完整交集可按同原有限pad判例外，模型v5及七false保持。不能从当前工作树推导已更新远端PR、已打包或可放飞。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前交接：P0起飞参考top至首NAV接受域

先核[16路径修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项待执行矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。本批没有编译产物、安装包或PR更新。

关键不是名义线更粗，而是原GPI9／实际REAL32 MIS／wire独立重算完整编码区间，对首NAV完整帽逐分量取XY凸包×Z，并真实走原环境／预算。新体不带contact豁免；地面/内部动态未由这条参考模型认证，原许可false保持。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前交接：保护报告的独立文件历史

先读[18路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[36项未执行矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。此批不是安装包，没有更新PR或编译产物。

重点：同一原生JSON和ID/hash保存，旧full9文件口不扩类型；文件导入只读且不恢复原生选择／任务／占用或权限。Qt JSON数值表示需在已知schema槽处理，不能为兼容而放宽原生记录。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前：接手遥测跨Run元数据搜索

接收[6源码／UI＋14文档=20路径](MERIVUS_SWARM_PATCH_MANIFEST.md)，无新文件／资源／依赖。先核[Reader](../../custom/src/Swarm/FleetTelemetryArchiveHistoryReader.md)的完整file校验与130MiB预留，再核[Archive](../../custom/src/Swarm/FleetTelemetryArchive.md)原catalog／同hash打开和单槽双终结，最后核[Panel](../../custom/src/Swarm/FleetTelemetryHistoryPanel.md)显式筛选、选择及确认。

[36工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。UI/UX只做静态参考，没有渲染、读屏、键盘或对比度实测；[审计](MERIVUS_SWARM_STATIC_AUDIT.md)不等于运行或放行。打开的是原file首页，不保证首页有该消息命中；不得用元数据结果自动生成运动报告或恢复任务。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前：接手执行观察外调生命周期修复

接收[7源码＋15文档=22路径](MERIVUS_SWARM_PATCH_MANIFEST.md)，无新文件／UI／资源或依赖。先核[Vehicle绑定／RAW／queue](../../src/Vehicle/VehicleMissionExecutionObserver.md)、[Run快照与变更代次](../../custom/src/Swarm/FleetMissionExecutionObservation.md)，再核[Txn](../../custom/src/Swarm/FleetMissionStartupTransaction.md)和[TaskRecord重定位](../../custom/src/Swarm/SwarmMissionOrchestrator.md)。

[36项场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；[审计](MERIVUS_SWARM_STATIC_AUDIT.md)不是对象生命周期、Qt线程、重入、编译或飞行放行证明。原live三查不得删，实际write事实不得回滚，源失效不认证任务完成。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前：接手LAND接受区域参考体

接收[4源码＋15文档=19路径](MERIVUS_SWARM_PATCH_MANIFEST.md)，先核[原生已绑定参考](../../custom/src/Swarm/FleetMissionContactModel.md)、[两类新参考体与有限pad](../../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)，再核原[Worker](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)实际消费者。

[36项场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)不等于构建、模型执行、净空／性能或飞行放行。缺contact或不支持分支不拿p7、默认地面或局部成功前缀补完；入口连接／body／障碍不借下降support豁免。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前：接手原参数封条及覆盖索引

接收[8源码＋16文档=24路径](MERIVUS_SWARM_PATCH_MANIFEST.md)，没有新文件／UI／资源登记。先核[原29封条](../../src/Vehicle/FleetStartupMemorySeal.md)及[事务来源／唯一调用](../../custom/src/Swarm/FleetMissionStartupTransaction.md)，再核[索引接口](../../custom/src/Swarm/FleetMissionClearanceGeometry.md)和Domain／Waypoint真实消费者。

[36项场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，需在工程师工具链及隔离环境另行验证；[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)与[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)不作构建、性能、净空或飞行放行证明。原29变化阻断不能解释为完整StartPermit；完整query的声明几何计算不等于真实动作路径。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前：接手受保护旧Run报告

接收[8源码／UI＋15文档=23路径](MERIVUS_SWARM_PATCH_MANIFEST.md)，仅新增[独立报告合同](../../custom/src/Swarm/FleetProtectedRunReport.md)一份模块说明，无新代码／资源登记。先核light catalog/private record、Controller双尾栅、Builder独立类型，再核原Panel明确选择和原生JSON单份采用。

[36项场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)不是放行依据。旧full报告／保存／打开／handoff不接纳新格式；人工复制新JSON不代表可恢复任务、权限或原Run。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前：接手两个P0后端修复

整批接收[2源码＋13文档=15路径](MERIVUS_SWARM_PATCH_MANIFEST.md)。先读[Run序号回退](../../custom/src/Swarm/FleetMissionExecutionObservation.md)和[NAV凸包几何](../../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)，再按[32项全部未执行矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)在工程师环境验证。

没有新UI、API／协议／资源登记，也没有本批提交、PR更新或安装包。[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)不是编译／运行／放行证据；动态轨迹、完整启动许可及正式机载结果仍未完成。下方为历史。

## 487 当前：接手历史观察卷宗

整批接收[6源码／UI＋15文档=21路径](MERIVUS_SWARM_PATCH_MANIFEST.md)，无新源码文件／注册，仅新增[观察卷宗说明](../../custom/src/Swarm/FleetTelemetryObservationReport.md)。依次核ReportBuilder新static、Archive一次调用与双缓存、原Panel的同一快照采用；[34项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

四视角只是展示，不是登录权限；手动复制不是自动导出、签名或文件传输。工程师应核对完整卷宗中的原页六身份和覆盖范围，不把客户子视图当完整原始证据，不把采集run当任务run。[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)不是放行依据。本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前：接手固定遥测历史与运动摘要

整批接收[11源码／UI／登记＋14文档=25路径](MERIVUS_SWARM_PATCH_MANIFEST.md)。阅读[Archive](../../custom/src/Swarm/FleetTelemetryArchive.md)、[纯摘要](../../custom/src/Swarm/FleetTelemetryMotionReview.md)及[面板](../../custom/src/Swarm/FleetTelemetryHistoryPanel.md)，再按[32项未执行矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证；[审计](MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)不是验收报告。

本地入口只认固定AppData/fleet-os/telemetry-archive中已存在且合规的NDJSON，不创建／复制／迁移文件，不开启录制。工程师需在受支持构建／隔离环境准备合法档案并验证；不能把该入口当ULog导入、漂移原因诊断或飞行放行。仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前：接手区域任务严格性能政策

整批接收[9源码／UI／资源＋19文档=28路径](MERIVUS_SWARM_PATCH_MANIFEST.md)。依次读[新政策组件](../../custom/src/Swarm/FleetPerformancePolicyEditor.md)、[Draft](../../custom/src/Swarm/FleetMissionDraft.md)、[Compiler](../../custom/src/Swarm/FleetMissionCompiler.md)、[Store](../../custom/src/Swarm/FleetMissionPlanStore.md)，再按[32项未执行矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证；[审计](MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)区分静读与运行证据。

新增两文件、两处QML资源登记，无依赖／C++类型／MAVLink／PX4变化。未声明／false／true三态是本地规划意图，不是证明性能达标或恢复审批。工程师负责真实编译、仿真、UI／硬件验证及放行；不能靠改true或删pending来放飞。

原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。 本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前：接手工程性能声明的录入／独立审批

整批接收[12源码／UI＋19文档=31路径](MERIVUS_SWARM_PATCH_MANIFEST.md)。依次读[原面板与Root](../../custom/src/Swarm/FleetCapabilityReviewPanel.md)、[Review](../../custom/src/Swarm/FleetCapabilityReview.md)、[Registry](../../custom/src/Swarm/FleetRegistry.md)、[Session](../../custom/src/Swarm/FleetOperatorSession.md)，再看[32项全部未执行矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[本批审计](MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)。

新增第五操作只批准本地工程数字及完整能力清单；基线不变、旧任务依据失效，未实测性能／自动检测改装／认证硬件。现有两宿主页继续共用同一个面板和actual Root／Swarm，不新增后端引用或新窗口体系。工程师负责真实编译、仿真、硬件与最终放行，不能把静读结果当安全证明。

原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。 本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前：交接已批准工程性能声明的实际消费

整批接收[12源码＋19文档，共31路径](MERIVUS_SWARM_PATCH_MANIFEST.md)。先读[共享声明证据](../../custom/src/Swarm/FleetCapabilityEvidence.md)、[Compiler](../../custom/src/Swarm/FleetMissionCompiler.md)、[EnergyScheduler](../../custom/src/Swarm/FleetEnergyScheduler.md)及[备用预算](../../custom/src/Swarm/FleetReserveEnergyPlan.md)，再核[30项全部未执行矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[审计范围](MERIVUS_SWARM_STATIC_AUDIT.md)和[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)。

本轮强化既有true严格策略；false／absent的旧规划不升级。它只消费既有工程批准数字，不提供新性能录入器／实测清单／标定／二进制认证；性能及权限认证字段保持false，P0动态状态和路径缺证仍阻断StartPermit。没有本批构建／测试产物、新PR、提交或安装包。

工程师后续在匹配工具链、隔离环境中编译、仿真、硬件验证并决定放行；本轮未执行任何这些动作，也未运行helper／业务／模型／QML或改PX4。原25目标、六类剩余及约40%／60%粗估不按31路径改变。下方为历史。

## 482 当前：交接原29证据链与真实普通NAV连接报告

整批范围[19源码／登记／UI＋22文档，共41路径](MERIVUS_SWARM_PATCH_MANIFEST.md)。先读 [WaypointSwitchReview](../../custom/src/Swarm/FleetMissionWaypointSwitchReview.md)、[Worker/schema3](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)、[原29Readiness](../../custom/src/Swarm/FleetMissionStartReadiness.md)和[Witness](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)，再看[32项全部未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[本批静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)。

本批无可运行安装包、构建／测试产物、新PR或提交。工程师后续在匹配工具链和隔离环境实际编译、仿真、硬件及最终刷机放行；本轮未执行这些动作。动态高度接受、平滑器、实际路径和完整StartPermit仍未开发收口，不能仅切开关或放宽false字段来放飞。

旧12hash、公开19键和PM12／14／28旧口保持；新Refresh／UI固定29不续期。仅静态源码／文档及文案工作，未运行helper／模型／业务／QML或UI验证、未改PX4。原25目标、六类实质剩余和整体约40%源码工作量粗估保持。下方为历史。

## 481：交接同原生整数域的运行门与环境计算

请整批接收[16源码／23文档清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，阅读[整数域](../../custom/src/Swarm/FleetMissionStartupIntegerDomain.md)、[审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[32项未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)。

新显式选择只给新模型，不改旧policy1–4或恢复旧审批；新numeric包含与环境、动力学、真实路径及最终许可分开。实际准入仍因actualPathCertified=false阻断，不是工程师只要切开关即可放飞。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480：交接固定28项起降参数证据链

请按[准确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)整批接收9源码／登记及18文档，先读[动作模型](../../custom/src/Swarm/FleetMissionActionPhaseReview.md)、[审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[30项未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[身份边界](MERIVUS_ACK_IDENTITY_AUDIT.md)。

旧12hash与PM12／14接口不变，新28证据不是命令或许可。模型一致、采集完成不证明当前控制器阶段、真实轨迹或飞行成功，完整数值域／动态净空／StartPermit／正式结果等仍须开发。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479：总速覆盖模型与原14参数尾核交接

本次9源码／登记、18文档共27路径，新增一个纯C++类的h／cc／md，无新依赖或新界面；原面板同步两处14项范围文案。先看[准确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)和[26项待执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

旧12参数hash及公开绑定接口保留；实际当前采证增加两个guard的来源、模型与整组尾核。工程师必须独立验证native回调／来源生命周期、实际构建、正／零／负宏及float32差异；本批不能授予部署或飞行许可。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478：完整原域与编码后坐标链交接

本次10源码、21文档，共31路径，无新增文件／工程登记。[准确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[待执行30场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)为接手入口。

正常构建缺完整策略一律拒绝；旧资料仅独立原形复现，3处source互核与当前Readiness策略输入已补。工程师需验证旧档byte形状、来源版本、E7边缘／整段／动作链、预算与实际工具链，不以本文替代实际验证。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477：名义边界退让与source4交接

本次8源码、15文档共23路径，无新类型／文件／依赖／资源登记。读取[准确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[工程师待执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

正值约束全部名义路径，但不证明编码量化后位置或物理轨迹；不得把source4、nominalBoundaryDistanceChecked或本次静读当作启动许可／飞行放行。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476：排除区名义规划源码交接

本次11源码／资源、15文档共26路径。新增坐标编辑→schema3→task.area.holes→实际生成／最终Compiler检查→当前审阅的源码链；地图标注仅待审输入。边界可沿、零退让及运行验证未开展均须保留。

请从[逐文件清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[待执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)接手；没有新PR、安装包、编译或测试产物。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475：联合人工新任务源码与静态交接

先读[33路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师接手项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[联合算法](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)和[实际操作链](../../custom/src/Swarm/FleetMissionGroupRemainderPanel.md)。本次沿用原独立运行／部署和批准边界，不恢复原Run、占用或控制权，也未新增启动放行。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474：整组历史资料与明确成员选择

请先读[22路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师接手项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态核对范围](MERIVUS_SWARM_STATIC_AUDIT.md)及[资料面板合同](../../custom/src/Swarm/FleetMissionRemainderArchivePanel.md)。新旧包互相不猜测升级；未提交成员保留不可选，旧原生对象已消失且资料未保存时仍不能补造。

当前仅提供源码及静态交接，不提供已编译安装包或实机可用证明。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：完整历史规划资料与人工新提案的交接

按[30路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)查看11源码／登记资源、19文档。先读[包合同](../../custom/src/Swarm/FleetMissionRemainderArchive.md)与[实际UI／Root链](../../custom/src/Swarm/FleetMissionRemainderArchivePanel.md)，再按[工程师清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证完整源／载荷映射、两种schema、新任务审阅、保存不确定性以及收起／退出／换包。

本批只源码与静态证据，无安装包或运行结论。导入资料始终历史且未认证，不恢复Run、占用、批准或正式结果。完整目标继续。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：编辑草稿保存恢复的静态交接

本批[28路径](MERIVUS_SWARM_PATCH_MANIFEST.md)包含11源码／登记资源、17文档。重点阅读[Store合同](../../custom/src/Swarm/FleetMissionDraftStore.md)和[实际面板／Root链](../../custom/src/Swarm/FleetMissionSavedDraftPanel.md)，按[工程师清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证未完成源、坏文件／外部变更、提交不确定性、同步回调归属及审批隔离。

当前只有源码及限定静读，没有可运行结论或新安装包。恢复为独立可编辑副本，不恢复启动许可或旧Run；单槽与一份会话备用不是多版本备份。原完整路线图保持。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：同次原运动对象的实际跨层持有

请按[31路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)查看13源码／18文档：原模型深冻结、两Review原Ptr集合、Worker／Domain实际消费、Witness保存及Txn参数对应。value-only结构重建和实际数值对象必须区分，不能由相同JSON、哈希或仍存活shared_ptr认证当前许可。

本轮只有限定静读和普通文本检查；工程师后续对象身份、COW逃逸、完整源、取消／finished及旧无资料路径验证见[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。numericfalse和全部启动阻断保持，原完整目标未完成；静态证据范围见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：接手完整候选索引与真实SAT链

本批交接为[Geometry](../../custom/src/Swarm/FleetMissionClearanceGeometry.md)原生自持索引及[DomainReview](../../custom/src/Swarm/FleetMissionStartupDomainReview.md)的实际消费，不是启动放行。请沿[16路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)查看3份源码／13份文档，并按[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证完整候选、闭接触、原行身份、取消和预算。

候选数不代表碰撞数；根节点全包含也仍须最终SAT。密集部分重叠可增加成本，未做性能或稳定性验证；numericfalse、Compiler pending、完整StartPermit与正式结果缺口保持。静态证据范围见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469 凸柱SAT与整域接线交接

先读[16路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[工程师待验证项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。这是两种新纯几何API和真实消费者，不是安装包或可飞放行。

关键区别：完整凸柱准备≠只检查顶点数量；精确SAT≠AABB重叠；数学非空≠实际碰撞；原生私有缓存≠场景来源认证。不得把一般带斜面扫掠体送入新接口后丢掉斜面，也不得以新的数学True绕过原numericfalse及pending。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468 外包紧化及报告前缀交接

先读[15路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[未执行事项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本轮不新增安装包或设备操作，只改两份既有后端实现。

重点区分：缩小仍然保守的计算外包，不是缩小原运行允许域；去掉query已蕴含的地形覆盖Z面，不是去掉接触／障碍高度；纯值前缀校验不是重算solver或认证结果真值。当前numericfalse与所有实际许可边界保持。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467 完整理想运动域交接

先读[准确34路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[工程师未执行验证项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批新增后端算法和实际消费者，不提供安装包，不操作飞机。

必须分别理解：外包构造完成、环境比较完成、声明理想模型满足、原Qt数值包含和最终启动许可。当前第四项明确未证，不产生第五项。指定TAKEOFF／LAND接触豁免仍只属于原名义动作，不适用于整个运动域；不得通过改布尔值、删pending或按机号聚合pad“打通启动”。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466 历史备份导入交接

阅读[本批修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[工程师未执行验证项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。这批只新增单段副本进入历史保留区的源码，不是可安装发布包或真机放行。

先核实原诊断与预览来源，再区分暂存写入、读回、发布和实际确认。任何failed／cancel／unknown都不自动删除、覆盖、恢复任务或重发。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465 逐文件诊断交接

先看[准确20路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[限定静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)及[未执行验证事项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批增加备份目录混合可读／拒收清单，原strict catalog继续整次拒绝坏候选。
没有任何实际文件扫描、hash、SQL、恢复或UI验证结果；诊断清单、原名和随机ID不是恢复授权。未更新远程PR或安装包。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：原源缺失时的独立备份清点

先核[准确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)和[未执行验证](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批只写读取既有备份的代码，不执行扫描、SQL、hash或恢复。原目录不必存在，目标目录和合作锁仍须工程师核真实环境。

清点成功只表示本次候选副本通过结构检查，originalSourceVerified／backupSetCompletenessVerified仍false。坏文件或部分副本会整次拒绝，没有自动修库／删文件入口；不应称灾后恢复平台已完成，也不是可运行包更新。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：单段备份与读回

本批仍只是地面站源码和文档，不是更新后的可运行安装包。先看[准确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)和[未执行验证](MERIVUS_SWARM_ENGINEER_HANDOFF.md)，再核原生目录／锁、NewOnly无覆盖创建、短写与部分文件保留、完整读回及原源对应。

源文件不删除、不移动；备份已核对不代表全部历史或独立介质。复核构建、实际SQL／文件系统行为、UI和故障场景全部留给工程师；本机不执行备份、回灌或恢复。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：单段归档退役与恢复

本批[维护入口](../../custom/src/Swarm/FleetArchiveMaintenancePanel.md)操作对象为固定本机归档文件，而非无人机或任务；实际执行代码必须经工程师构建／审查后验证。先看[准确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)和[待验证项目](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本机尚未执行任何归档盘点或移动。实现要求明确库存、原生短期预览、重新确认、不覆盖、不永久删除、不复制回退，异常保可能已移动；退役不释放磁盘。不要把这部分源码看成现有安装包已经具有或验证通过的功能。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：保留任务目录与精确哈希原记录

本批[任务历史组件](../../custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)实际接Reader／Store／Guided，工程师先核[准确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)与[未执行验证项目](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。需要本机已有明确录制的归档，默认不开始录制、停录、自动扫库或加载旧任务。

本机目录只是有限保留诊断行的累计，不认证任务成功、真实性能、完整遥测、跨站接替或真实控制权。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：明确电池异常贯穿原始来源与实际准入

[共享契约](../../custom/src/Swarm/FleetNativeBatteryEvidence.md)与[原生生产端](../../custom/src/Swarm/FleetNativeBatteryObservation.md)列新schema2及旧schema1边界；[HealthCenter](../../custom/src/Swarm/FleetHealthCenter.md)／[Scheduler](../../custom/src/Swarm/FleetEnergyScheduler.md)与原风险／审批链实际消费，最后原整组seal补电池账本，未新增飞行命令。

[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)列准确11源码／登记、19文档；[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列MAV1、零尾、最高故障位、纯故障变化、过期副电池、FTC合并和最后callback等未执行验证。不是可信续航／电池认证或刷机放行。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：原生运动模型一次绑定与阶段复用

[Witness](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)保原source12模型，[Txn](../../custom/src/Swarm/FleetMissionStartupTransaction.md)逐原目标核完整source后保同Ptr，[Transition](../../custom/src/Swarm/FleetMissionStartupTransition.md)不在phase新prepare。模型指针不是授权、时效或全域安全证书，原权限／时钟／Witness连续性和phase seal仍独立检查。

[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)列5源码／16文档；[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列合法旧值、错位成员／模型、JSON数字和null yaw、跨线程／过期及新请求等未执行验证。原普通源指纹已约束GPI，不把优化当已证实rebase漏洞；完整域／StartPermit仍须开发。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：实际运动规则统一和原审阅输入冻结

[共享模型](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)已供Transition复用；[Worker](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)及两类Review冻结原同源source12，不再只凭名义几何描述未来运动约束。它仍不是全域净空报告，原numericContainmentProven及认证／释放字段false。

[准确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)列8源码／登记、18文档；[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列合法旧数值等价、完整载荷绑定、普通profile取源、地面原READ、资源边界及拒绝路径等未执行事项。完整运动域包含、Compiler pending、完整StartPermit和正式结果仍须开发，不是只等测试。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：原始接收顺序与完整阶段判定的整组尾核

[RAW说明](../../src/Vehicle/VehicleStartupRawIngress.md)和[Seal说明](../../src/Vehicle/FleetStartupMemorySeal.md)覆盖首回调前绑定及实际Txn消费者；[工程师待验证](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列正常步骤、prequeue、同步write回包、嵌套／坏包／取消及跨机callback反例。本机没有执行这些场景。

范围10源码／工程登记、17文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。原Compiler pending保持，不把整组内存相同当物理运动轨迹／净空认证或最终StartPermit；完整产品和其他原25目标继续。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 456：原时钟与原来源随实际授权移交

[Witness说明](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)与[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)明确：原Review创建后，普通／地面Authority和Txn持同ptr；只缩短原期限，UTC水位共享。末写前核全组原source／datum，不用JSON标志恢复，也不因旧Review退休失去实际来源材料。

[范围](MERIVUS_SWARM_PATCH_MANIFEST.md)13源码／19文档；既有Vehicle钩子未修改。运动包含与连续性缺口仍使原pending保持，不可据本批认定可执行、编译通过或只剩工程师测试。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：启动计算的生命周期与首基准交接

先读[Worker](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)及[两类实际审阅](../../custom/src/Swarm/FleetMissionStartReview.md)，再按[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证来源／取消／期限／样本不重基。准确范围11源码／18文档共29路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

并非完整启动准入：原pending不删，仍需两Authority移交后的独立原生来源、最后write和运动包络包含关系。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：工程高程对应与实际原生来源交接

先读[目录2](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)和[Datum合同](../../custom/src/Swarm/FleetMissionClearanceDatum.md)，再核[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。准确范围11源码／登记、17文档共28路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

资料必须由工程人员明确同datum／零偏移及原scene／配置／原点对应；软件不自动识别geoid或当前PX4 AMSL分支。新来源进入真实审阅，不表示净空／最终write许可已完成，不能通过工程师开按钮绕过原pending。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 原25目标和六类实质代码保持，下方453为历史。

## 453：当前构型来源／资料有效期交接

先读[原生配置观察](../../custom/src/Swarm/FleetNativeConfigurationObservation.md)、[Registry证据](../../custom/src/Swarm/FleetRegistry.md)和[Source合同](../../custom/src/Swarm/FleetMissionClearanceSource.md)，再看[工程师待验证](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。准确范围15源码／登记、16文档共31路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批接普通／地面审阅的真实配置对应和期限，不完成实物包络／未上报改装认证、高程对齐或实际启动净空／最终write许可；原pending保持，仍须继续开发，不能只让工程师开按钮。本批只优化地面站源码及交接材料；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余不缩减。 以下452为历史。

## 452：接地模型与实际规划／归档交接

先读[完整合同](../../custom/src/Swarm/FleetMissionClearanceInput.md)和[模型／PX4语义边界](../../custom/src/Swarm/FleetMissionContactModel.md)，再看[报告3](../../custom/src/Swarm/FleetMissionClearanceReview.md)及[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。准确范围9源码／登记、18文档共27路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

声明pad与起落架不证明真实接触，p7不是本地多旋翼PX4下降硬停止点。原pending／实际构型datum时效／同次启动及最终write仍须开发，不能只由工程师开按钮放行。未编译、运行、测试、修改PX4、更新PR或打包；原25目标保持，下方451为历史。

## 451：可取消后台规划的三入口交接

先读[准确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)，再看[Worker生命周期](../../custom/src/Swarm/FleetMissionPlanningWorker.md)和[Coordinator／Root合同](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)。最后按[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证正常完成、取消排空、换源／关闭和归档重入。

本批只完成代码及限定静态核对，不是构建通过、界面性能或可飞放行。原净空／接地／真实构型与最终启动许可等仍须开发，不能只由工程师“开按钮”。没有生成安装包、提交或更新PR；原25目标保持。下方450为历史。

## 450：规划结果与独立启动几何身份

先读[新scope完整源](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)、[独立startup basis](../../custom/src/Swarm/FleetMissionStartupSpatialReview.md)及[报告schema2](../../custom/src/Swarm/FleetMissionClearanceReview.md)，再看[Compiler](../../custom/src/Swarm/FleetMissionCompiler.md)／[PlanStore](../../custom/src/Swarm/FleetMissionPlanStore.md)。9源码／17文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行项见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

实际规划求解及归档内容一致性不等于实际启动准入；原pending保留，接地／构型高程对应／时效及最后write仍需代码。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方449及更早为历史。

## 449：声明模型净空算法交接

从[几何合同](../../custom/src/Swarm/FleetMissionClearanceGeometry.md)及[报告合同](../../custom/src/Swarm/FleetMissionClearanceReview.md)开始，再读[精确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态推导](MERIVUS_SWARM_STATIC_AUDIT.md)和[待执行清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。5源码／15文档共20路径，无新UI、资源、后台服务或飞行入口。

完整扫掠／并集计算源码不是实际净空审批集成；原pending阻断、来源／身份门禁保持，接地上下文、规划／启动实际消费和最终许可继续开发。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方448及更早为历史。

## 448：版本化净空工程资料交接

先读[输入完整合同](../../custom/src/Swarm/FleetMissionClearanceInput.md)，再核[Draft](../../custom/src/Swarm/FleetMissionDraft.md)、[Compiler](../../custom/src/Swarm/FleetMissionCompiler.md)、[版本库](../../custom/src/Swarm/FleetMissionPlanStore.md)和历史／修复来源1／2配对。8源码／工程＋17文档共25路径，见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，未来执行项见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

资料采用显式毫米与高程／图表／误差声明，不猜机体尺寸或更换成员身份；无资料的旧来源不增加默认。合法新资料仍pending阻断，完整solver与实际启动固定消费尚未完成，不是可用净空产品。UI按现有输入保留与恢复确认边界静态修改，无新布局；未编译、测试、解析、运行界面或设备，原25目标继续，下方447及更早为历史。

## 447：从风险报告到实际门禁的交接

先读[RiskEvaluator共享检查](../../custom/src/Swarm/FleetMissionRiskEvaluator.md)、[Coordinator](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)，再核[Deployment](../../custom/src/Swarm/FleetMissionDeployment.md)和[普通](../../custom/src/Swarm/FleetMissionStartReview.md)／[地面启动审阅](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)。本批6源码＋16文档共22路径，见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)与[工程师未来执行事项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

`inputValid` 是语法，不是无业务风险；原生检查拒绝非空／缺失／null／坏型原因。当前评估和准确归档内容相同才可继续其余门槛，不能拿另一任务最新报告补旧记录。严重空code统一fallback，旧历史不回写。未编译、测试、运行或操作设备；完整净空／最终许可与原25目标继续，下方446及更早为历史。

## 446：原始高度与整数毫米的精确门禁

先读[精确比较器](../../custom/src/Swarm/FleetMissionAltitudeArithmetic.md)和[高度门禁及三个解析反例](../../custom/src/Swarm/FleetMissionStartupAltitude.md)。本批4源码／登记＋15文档，共19路径，见[精确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)；未来执行事项见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

残差直接比较 `1000*A-1000*L-originMm`、`1000*A-1000*R-homeMm` 和 `R-L-homeLocalZ`，漂移直接用当前／基线四个原始高度项。系数与容差比例在整数域计算，保留原实际 binary64 配置容差的 `<=` 闭区间，不先浮点换算毫米或相减后再补一个 ulp。零容差仅接受真实零，非零残差不能因舍入丢失而通过。

reference20／sample20、策略schema、原来源／收据／scope监视及权限不变；诊断差值仍保留原显示语义。三个解析反例尚未执行，工程师须验证数值边界及真实消费链，不能把本次静态修正当作编译／测试通过或完整StartPermit。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；原25目标继续，下方445及更早为历史。

## 445：显式高度关系容差与真实发送前连续性

先读[高度契约](../../custom/src/Swarm/FleetMissionStartupAltitude.md)、[政策v4](../../custom/src/Swarm/FleetMissionExecutionProfile.md)、[原生scope](../../src/Vehicle/VehicleStartupAltitude.md)和[Transaction](../../custom/src/Swarm/FleetMissionStartupTransaction.md)，再核普通／地面首次材料与逐阶段消费。[策略表单](../../custom/src/Swarm/FleetStartupPolicyEditor.md)提供明确版本和六项容差输入，不补安全默认。

当前样本不会替换原已阅基准；越界／坏帧／失源／gap在同scope不可自行清除，最终write前仍核原basis。scope建立前不承诺连续覆盖，不认证内部valid／同原点／同采样／净空／最终许可。精确范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行事项见[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标继续；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方444及更早为历史。

## 444：原始高度与风险快照

先读[decoder](../../src/Vehicle/FleetStartupAltitude.md)、[native账本](../../src/Vehicle/VehicleStartupAltitude.md)、[Readiness](../../custom/src/Swarm/FleetMissionStartReadiness.md)和[普通审阅身份](../../custom/src/Swarm/FleetMissionStartReview.md)。本批11源码／登记、18文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，实际验证待办见[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

注意首次完整审阅材料及hash不更新，当前重捕风险身份仅隔离新独立诊断；原门槛、账号、TTL、原点与step合同不变。接收fresh不等于传感样本fresh，三差值不认证原点／HOME。原25目标及最终许可继续；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方443及更早为历史。


## 443：原生原点账本与启动参考连续性

先读[纯解码](../../src/Vehicle/FleetStartupGlobalOrigin.md)、[Vehicle账本](../../src/Vehicle/VehicleStartupGlobalOrigin.md)、[阶段参考](../../custom/src/Swarm/FleetMissionStartupGlobalOrigin.md)，再核Readiness／Transition／终态再审／地面新READ。13源码／工程登记与18文档见[精确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行项见[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

此消息事件型、不自动请求、不套周期TTL；首次scope原点依据不得在后续步骤重基。已有write事实与后来失效分别记录，不声称回滚。共同原点／内部有效分支／净空／最终许可仍待开发，原25目标保持；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方442及更早为历史。


## 442：全请求链与独立启动参考的共同名义检查

先读[联合helper](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)、[Checker](../../custom/src/Swarm/FleetAirspaceConflictChecker.md)、[捕获来源](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)及[现有明细](../../custom/src/Swarm/FleetMissionSpatialPanel.md)。本批10源码、17文档，精确路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行核验见[待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

新检查只对冻结的名义请求和假设计算，不能代替真实空域、4D、净空、StartPermit或正式结果。原归档报告不改scope，历史查看不复活批准。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；原25目标继续，下方441及更早为历史。

## 441：原值审阅、冻结归档和当前审批输入

先读[全链合同](../../custom/src/Swarm/FleetMissionAirspaceInputGate.md)、[原生检查](../../custom/src/Swarm/FleetAirspaceConflictChecker.md)、[协调器](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)及[任务状态机](../../custom/src/Swarm/FleetIntentTask.md)。本批7源码、15文档的准确范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行核验见[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

阻断诊断可读／可归档不等于可批准；真实空域、联合净空和最终启动许可尚未完成。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方440及更早为历史。

## 440：完整任务／原空域记录的源码交接

先读[完整任务合同](../../custom/src/Swarm/FleetMissionSpatialReview.md)、[明细与地图](../../custom/src/Swarm/FleetMissionSpatialPanel.md)、[原空域报告范围](../../custom/src/Swarm/FleetAirspaceConflictChecker.md)及[四字段归档证据](../../custom/src/Swarm/FleetMissionPlanStore.md)。本批14源码／资源、18文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，未执行项目见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

全部内容仍是源码阶段；原报告不能证明启动参考或终结下降安全，完整目标未完成。没有编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方439及更早为历史。

## 439：启动空间捕获、逐机明细与只读地图

先读[工作台来源](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)、[空间helper](../../custom/src/Swarm/FleetMissionStartupSpatialReview.md)、[空间界面](../../custom/src/Swarm/FleetMissionStartupSpatialPanel.md)和[名义间距候选](../../custom/src/Swarm/FleetAirspaceConflictChecker.md)。本批13源码／资源、17文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，实际待验证见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

捕获／看图不已阅、批准或发飞行命令。时间／地形／空域／净空等仍未知，不能用candidateCount=0放行。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包；下方438及更早为历史。

## 438：完整政策表单与草稿来源

新增[策略表单](../../custom/src/Swarm/FleetStartupPolicyEditor.md)和[父编辑器合同](../../custom/src/Swarm/FleetExecutionProfileEditor.md)，沿用[原生profile规范化](../../custom/src/Swarm/FleetMissionExecutionProfile.md)。6源码／资源和14文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，限定检查见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)，实际待验证项见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

明确采用只修改本地编辑文本；最终保存仍须原参数重采集和完整revision CAS。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包，原25目标及剩余实质工作保持。下方437及更早为历史。

## 437：实际部署启动与地面再审工作台

先读[启动工作流](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)、[操作界面](../../custom/src/Swarm/FleetMissionStartupPanel.md)及[地面风险合同](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)，再按[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证。实际范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，源码审阅边界见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。

本批7源码／资源和14文档；新按钮可请求真实三步发送，取消不执行DISARM或回滚。最终许可、净空和正式结果尚未完成；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方436及更早为历史。

## 436：运行简报与本地交接回执

先读[工作流与边界](../../custom/src/Swarm/FleetMissionHandoffWorkflow.md)、[Manager合同](../../custom/src/Swarm/MissionHandoffManager.md)及[面板说明](../../custom/src/Swarm/FleetHandoffPanel.md)，再按[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)安排验证。新入口使用实际Run和准确记录版本，不再自由输入任务ID。

本批7源码／15文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，只做源码和限定静态阅读；未编译、运行、认证、测试或放行，没有新PR／安装包。跨站、控制租约与其余原25目标仍有实质开发。下方435及更早为历史。

## 435：新余段记录的空间和协议核对

新增[共享投影](../../custom/src/Swarm/FleetMissionRemainderView.md)和[分页界面](../../custom/src/Swarm/FleetMissionRemainderRoutePanel.md)，并接到现有规划地图；旧区域提案不能替代新余段来源。

先读[435待执行项目](MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[精确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，再读[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。本批没有编译、测试、运行或放行，没有新PR／安装包；原25目标仍有实质开发。下方434及更早为历史。

## 434：余段新记录复用原审批和地面部署界面

先读[17路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[待验证项目](MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)，再核[余段采用／退出合同](../../custom/src/Swarm/FleetMissionRemainderPanel.md)和[共用部署面板](../../custom/src/Swarm/FleetMissionDeployment.md)。须交接累计依赖，四个QML文件不能独立替换一个不含前序后端的旧仓库。

明确采用新记录后才进入其审批和地面上传界面；不会恢复原Run批准、释放旧占用或允许空中接管。未编译、测试、运行、刷机、提交、更新PR或打包；原25目标继续。下方433及更早为历史。

## 433：人工余段重做提案交接

先读[精确路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，再读[工程师操作与验证待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[限定静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。合同分为[原生取源](../../custom/src/Swarm/FleetMissionRemainderSource.md)、[纯值规划](../../custom/src/Swarm/FleetMissionRemainderPlanner.md)、[界面操作](../../custom/src/Swarm/FleetMissionRemainderPanel.md)。

须接收累计源码依赖与工程/资源登记，不只复制新helper。无新安装包、提交或PR更新；本轮未编译、测试、运行或放行。新提案需重新审阅；历史进度/来源hash不等于已完成任务、旧批准或跨机接管。原25目标仍继续开发，下面432及更早为历史。

## 432：备用独立待命与完整路线重做预算

本批[29路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)包含11源码／工程登记及18文档，须连同累计未跟踪依赖一起交接；不是独立补丁包或安装包。先读[估算合同](../../custom/src/Swarm/FleetReserveEnergyPlan.md)与[操作入口](../../custom/src/Swarm/FleetReserveEnergyPlanPanel.md)，再核[工程师待验证项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。custom.pri 已登记新类，无新QML资源。

备用预算不再借最长主机；缺输入保留不完整，不自动取消备用要求。这里只重做目标完整航点，不恢复中断进度，也不发送待命／接替／起飞指令。待命系数与飞控剩余时间的负载等价未认证。[审计](MERIVUS_SWARM_STATIC_AUDIT.md)仅记录源码静态工作；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。原25目标与六类实质开发继续。

## 431：原生电池来源与能量预算时效

按[25路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)接收累计源码，先读[原生观察](../../custom/src/Swarm/FleetNativeBatteryObservation.md)、[桥接](../../custom/src/Swarm/FleetNativeBatteryBridge.md)和[Registry](../../custom/src/Swarm/FleetRegistry.md)，再核对[工程师待验证项](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批无新工程资源登记，原Vehicle、Swarm和Scheduler等依赖必须一并交接。

仅单个明确推进电池的新鲜报告进入预算；普通护照和缓存不恢复实时来源。源连续性不是远端认证，time_remaining是估算，3秒为本地接收门槛，实际固件报告用途／频率仍待验证。[审计](MERIVUS_SWARM_STATIC_AUDIT.md)只记录静态工作；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，不是安装包或已放行版本。原25目标与六类实质开发继续。

## 430：逐机能量预算与原编译身份

本批把实际编译成员、能量账单、风险判断和现有规划页明细连起来，同时修正满电续航公式及原Registry未知标记。请按[24路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)接收累计源码，结合[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)中的正向与边界待验证项；[审计记录](MERIVUS_SWARM_STATIC_AUDIT.md)仅反映静态阅读。

没有新增真实电池遥测，备用预算也不是实际接替／待命模型。未编译、测试、运行、修改PX4、刷机、提交、更新PR或打包，不是可安装或已放行版本。原25目标与六类实质开发继续。

## 429：真实Run的只读报告

本批交接包含[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)中的新后端、Root取源／冻结与文件操作、观察面板入口和工程登记。先读[报告合同](../../custom/src/Swarm/FleetMissionExecutionReport.md)及[界面合同](../../custom/src/Swarm/FleetMissionExecutionReportPanel.md)，再执行[工程师待验证清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原飞前报告不变；本报告只说明已记录观察。外部JSON可离线打开，但不认证来源、不续飞、不恢复任何批准。保存操作可能替换同名报告，界面必须在选路径后再次确认。未编译、测试、运行、修改PX4、刷机、更新PR或打包；原25目标仍未全部完成。

## 428：历史归档规划的编辑恢复

交接包含[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)中的PlanStore只读接口、Root共享草稿切换、Planner历史面板及QML资源登记。历史档案仍沿用原schema和固定默认文件，无新增数据库、磁盘重载按钮或损坏修复操作。

先看[版本库](../../custom/src/Swarm/FleetMissionPlanStore.md)与[界面合同](../../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)，再按[工程师清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证跨重启选择旧版本、缺源拒绝、备份切回和全新审阅。恢复草稿不要求历史Deployment存活，也不清除归档写入隔离。

本机只做源码与限定文本核对，[审计](MERIVUS_SWARM_STATIC_AUDIT.md)不作编译／运行通过结论；没有刷机、提交、更新PR或打包。原25目标继续，以下为历史。

## 427：原始草稿封存与人工修复入口

本批同时交付[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)中的Draft完整源、Compiler对应校验、PlanStore归档、Deployment冻结取源与Root／Panel接线。缺少任一消费方可能导致新档案拒绝或修复不可用；原无源档案不被自动升级。

请先看[取源合同](../../custom/src/Swarm/FleetMissionRepairDraft.md)、[界面合同](../../custom/src/Swarm/FleetMissionRepairDraftPanel.md)，再执行[工程师验证清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。新草稿仍需明确基准确认、全新原生审阅和人工审批；之前的unknown或matched都不等于机载修复成功。原草稿保留供切回，较早备份替换须明确确认。

本机只改源码与文档，限定文本检查见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)；没有编译、测试、运行应用、修改PX4、刷机、提交、PR更新或安装包。原25目标继续，下方426及更早为历史。

## 426：独立读取失败部署的整组机载任务

交接时包含[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)中的新原生工作流、PlanManager／Vehicle读取接线、Deployment私有子对象、生命周期互锁、角色政策、两个QML入口与custom.pri。未跟踪的新文件不能由普通git diff代替交付。

读取成功、内容相同、当前仍一致和允许启动是不同事实。新诊断只证明本机在该次读操作观察到什么；不会自动改机载任务或恢复旧批准。[工程师清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列的是待执行验证，不是测试通过记录；[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)记录本轮源码审阅范围。

没有新安装包、提交或PR更新；没有编译、测试、运行、刷机或修改PX4。修复草稿和原25目标其余工作继续。以下425及更早为历史。

## 425：先看普通回读差异，不恢复旧批准

本批接入真实普通任务回读的逐机、逐航点差异诊断和只读展开界面。接收时必须带上[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)中的新helper、部署接线、面板及custom.pri；仅复制已有受跟踪文件的diff会漏新文件。

[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列出待实际验证的正常匹配、字段差异、未知／空读、计数截断、取消／来源变化与界面项目。[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)只记录源码／文本核对，不代表这些场景已经运行。

没有新安装包、提交或PR更新，没有运行、编译、测试、刷机或修改PX4。历史诊断不授权上传／启动，不证明当前机载内容；失败后独立整组重新对账、修复草稿和原25项完整路线图仍未完成。

## 424：跨日志库的有界历史检索

[归档面板](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)增加独立检索入口；范围为本机诊断事件库的全部封存段，或封存段加固定的已停止录制未封存库。按机号与四种工程审批操作查询，不再仅筛选当前展示页。选择包含未封存库时，文件缺失属于失败，不静默缩小范围；查询不会自动停止录制。

每批最多读取4页、消费256行、返回64条匹配，并有字节上限；续扫游标来自原生实际结果，空匹配不等于扫描完。界面区分本批计数、已扫完文件和所选范围完成，保留来源文件、退休记录与异常退出等覆盖信息；不自动累积无限结果。完整合同见[Reader](../../custom/src/Swarm/FleetEventArchiveHistoryReader.md)和[Store](../../custom/src/Swarm/FleetPersistentEventStore.md)。

目录清单元数据在批次前后及续扫时核对，当前正在分页的文件另绑定实际epoch和整文件指纹；这不是跨文件原子快照、已读文件的持续哈希复核或全部飞行历史完整性证明。历史不恢复账号、批准、控制权或飞行权限。旧v1未记录的审批操作不会推测补齐。

本批源码与待验证范围见[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper／SQL／UI／应用／设备，未修改PX4、刷机、更新PR或打包。以下423及更早为历史；本批部分补足其“全库检索”缺口，但不完成永久全集索引、遥测全集、留存治理或恢复工具；原25目标和六类实质开发继续。


## 423：停止录制后的未封存库离线查询

[归档面板](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)新增明确的离线读取入口，复用原[读取线程](../../custom/src/Swarm/FleetEventArchiveHistoryReader.md)，只读固定本机未封存的 fleet-events-v1.sqlite。无需先恢复录制、连接飞机或登录飞行账号；本实例录制线程必须已结束，不能用点击停止或captureEnabled=false代替实际退出。

数据库以只读方式打开，不创建、修复、迁移或删除库；读取期间使用与录制器共用的协作锁，因此会创建／释放临时锁文件，不宣称完全零文件写入。遗留journal/wal/shm、坏库、锁冲突或身份变化均拒绝，不自动清理。每页64行，后页绑定原实际库epoch与整文件指纹；两页之间库变更须重新读取第一页。

该入口沿用历史只读边界，不恢复账号、能力批准、任务或飞行权限；异常退出、退休行及丢失尾部信息保留，查询成功不等于历史完整。下方422的“离线只支持封存段”由本批有条件的未封存读取补充；全库搜索、完整构型历史、恢复工具和长期留存仍未完成。

本批范围见[修改清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，运行待办见[工程师交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码及静态文本核对，未编译、测试、执行helper／SQL／UI／应用／仿真／设备，未改PX4、刷机、更新PR或打包。原25目标及六类实质开发继续，不能据本批声称整体落地或承诺完成日期。

## 422：显式本地归档与工程审批摘要（源码阶段）

[归档入口](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)接入指挥中心与任务规划页：默认关闭，明确开始／停止／封存并继续录制，查询当前录制批次或已封存段，按原生游标分页和取消。复用原 Store、BlackBox、Worker 与 Reader，不新增数据库，不依赖飞机在线或飞行账号；只读历史不会恢复权限或触发飞行命令。

四种能力／基线操作新增独立的[有限工程摘要](../../custom/src/Swarm/FleetEventArchiveCodec.md)：操作、机号、提交与结果三态、指纹和差异计数。不保存账号、自由理由或原始构型；内存接纳不等于 SQL 提交，原审计链追加也不等于持久化。旧 v1 未记录的详情不能补回，摘要失败不改变已发生的登记提交。

本批11源码／资源、16文档见[清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)，待执行验证见[工程师交接](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅静态阅读与限定文本检查，未编译、测试、运行 helper／UI／认证／SQL／应用、修改 PX4、刷机、更新 PR 或打包。

仍缺停止后未封存活动库的离线读取、全库检索、完整构型历史及长期留存管理；本页筛选仅当前页，封存数量不是总磁盘硬配额。原25目标与完整受控执行、可信性能、自愈、企业跨站、长期运营、真实模型／生态六类工作保持未完成，不能按阶段号计算完成率或可靠工期。下方421及更早为历史，“只有值／无归档UI”仅由本批固定本机入口部分覆盖。

## 421：构型基线批准／撤销接入实际操作链

[基线合同](../../custom/src/Swarm/FleetConfigurationBaselineEvidence.md)、[原生工作流](../../custom/src/Swarm/FleetCapabilityReview.md)和[共用面板](../../custom/src/Swarm/FleetCapabilityReviewPanel.md)已接四种独立操作。维护/管理员申请、不同合格账号审批；能力采纳与构型基线使用不同精确用途，不互借凭据。首次完整配置无旧基线也可批准；当前配置未知时，只能撤销真实存在的本地批准，不能建立新基线。

重复批准同一基线也退役旧能力批准，须另行审阅能力。原生先冻结真实提交回执再通知，后置失效或审计失败不抹提交事实。修复界面对合法旧记录大写哈希/首尾空白的误拒；原记录不改写。完整有类型基线仍仅在本进程保存，不认证硬件、改装或性能。

本批9源码、17文档见[精确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，待执行项目见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码与限定静态审阅，未编译、测试、运行UI/认证/helper、修改PX4、刷机、更新PR或打包。原25目标/六类仍有实质开发，不能据此计整体验收或完成率；下方420及更早“无基线批准界面”为历史状态。

## 420：完整有类型基线接入任务消费者

[构型基线合同](../../custom/src/Swarm/FleetConfigurationBaselineEvidence.md)将实际 Registry 当前完整配置与本机批准快照统一接到 Matcher、RoleAllocator、Compiler 和 Continuity。旧 configurationState 只保留哈希级报告语义，不再单独满足 requireConfigurationBaseline 或获得基线排序优待。

新观察身份进入 Guided 旧/新机队指纹和 Coordinator schema2 审阅归档；Deployment 核归档与实际审阅一致，仍须当前来源检查。hash-only、未知或导入旧批准不能通过；恢复原配置可以重新审阅，但不能复活旧批准。可选基线政策不变成所有任务的强制门槛。

本批12源码、20文档见[精确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。没有新增基线批准界面/持久化或硬件、性能认证；原25目标/六类仍有实质开发。仅源码和限定静态检查，未编译、测试、运行、修改PX4、刷机、更新PR或打包。下方419及更早为历史，其“任务仍仅按旧标签判断”由本批限定消费路径覆盖。

## 419：构型快照和字段差异已接源码，未运行验收

[构型比较](../../custom/src/Swarm/FleetConfigurationDiff.md)将实际Registry的本机完整批准快照或前次不同完整观察接入原双账号能力审阅。界面区分来源、类型、Missing/null和截断；缺前/后值不报告无变化。只按显式非空map视作完整上报，不认证每机型字段完整、实物或性能。preview独立升schema2/18键，原context6/approval14不变。

typed变化与完整观察丢失/恢复会退役旧能力批准；审批-only导入同样退役触达记录。新构型基线批准要求当前完整观察，不能批准hash-only之后残留的旧map。普通健康/重复同配置不滚动历史；私有历史仅本进程，无新增持久化或基线批准UI。

本批8源码/工程登记、15文档见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。原25目标/六类仍继续；下一源码切点是统一可信基线证据与Matcher/RoleAllocator/Compiler消费。旧configurationState/requireConfigurationBaseline仍为哈希级，不能按本批差异展示宣布完整任务基线门禁完成。没有编译、测试、运行helper/UI/认证/应用/设备、修改PX4、刷机、更新PR或打包。下方418及更早是历史。

## 418：本地能力审批源码交接

接收[16源码/资源及17文档清单](MERIVUS_SWARM_PATCH_MANIFEST.md)与全部累计依赖，先读[工程师待验证矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[双账号工作流](../../custom/src/Swarm/FleetCapabilityReview.md)。这不是已编译安装包，也没有更新后的PR。

新路径只批准/撤销当前本地条目的工程能力声明。原生提交、当前有效性与日志追加是不同结果；同账号/过期/换源不可沿用确认，失败也不可盲目重试。构型JSON不是类型化备份或实物证明，旧C++ bool入口未统一改为双账号。完整路线图继续，本机不编译、测试、运行认证/UI/设备或放行。

## 417：固定文件工程目录的运行中重载交接

先读[19源码/资源清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[工程师矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)，再对照[Catalog](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.h)、[Reload](../../custom/src/Swarm/FleetFirmwareCatalogReload.h)及[面板](../../custom/res/Merivus/FleetFirmwareCatalogPanel.qml)。接收累计完整工程与依赖，不只复制本批文件；没有安装包或更新后的 PR。

AppData 固定现有档案只读，完整旧/新声明和差异/哈希被审阅，独立两账号一次确认、原目录 CAS/实际 scope/原期限及最后复读/纯围栏后替换内存 catalog。提交事实不因后置失败或审计追加异常被抹掉；旧任务 review 失效，不停止 Run、不释放占用。原启动自动 load 未改为双账号，不宣称全入口统一授权或固件认证。

全球坐标启动另补 ABS 水平/垂直且非 CONST_POS，不扩 RAW/通用健康/室内/GNSS 硬件。本机未编译、测试、运行解析/helper/UI/认证/应用/SQL/设备、改 PX4或刷机；工程师须实际验证两个入口、原TTL/文件变更/空替换/旧档案重引入/销毁重入及资源绑定。原25目标/六类仍有实质源码工作，旧章节保留历史。

## 416：逐机配置编辑与参数依据交接

先读[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[增量清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，再对照[Draft](../../custom/src/Swarm/FleetMissionDraft.md)、[Profile](../../custom/src/Swarm/FleetMissionExecutionProfile.md)、[只读参数接口](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)及[操作流程](../../custom/src/Swarm/FleetMissionOperatorWorkflow.md)。本阶段11个源码/资源及15份文档须连同累计依赖接收，不是独立安装包。

本地完整 profile 的保存、真实参数哈希绑定与后续飞行放行是不同环节。reserve 不活动输出，删除重加不继承；通知后 false 可能已有本地提交。保存前重读源指纹，改输入使旧 review 失效；高级启动 JSON 可空但不能因此启动。无参数 refresh/写入，不新增飞行权限。

本机未编译、测试、运行 helper/UI/认证/应用/设备或更新 PR、打包；工程师后续验证与真实放行另行负责。原25目标/六类继续，完整许可、固件目录入口、净空与正式机载结果未由本批完成。旧章节保留为历史。

## 411–415：专用恢复面板接线交接

先读[当前工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[精确增量清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，再读[界面/Root合同](../../custom/src/Swarm/FleetOccupancyRecoveryPanel.md)及[原生恢复合同](../../custom/src/Swarm/FleetOccupancyRecovery.md)。接收累计当前工程，不只复制新面板或9个触点。

指挥中心、FlyView地图内区域规划共享原Guided控制器；恢复只解除本地restart/history，不清当前真实unknown/owner/任务。9源码/资源、18文档是源码增量；Root uiWiringPresent=true但runtimeValidated=false，原生recoveryUiIntegrated仍false，不能解释为已有可安装的验证版本。

工程师负责后续真实编译、Qt绑定/系统认证交互、仿真、硬件验证与放行；本机均未做，也未更新PR、打包或修改PX4。原25目标/六类继续，旧无UI段为历史。

## 406–410：原生重启／历史占用专用复核

[恢复复核合同](../../custom/src/Swarm/FleetOccupancyRecovery.md)新增实际Swarm唯一拥有的原生工作流，公开只读对象和request/独立审批/取消入口；未加QML操作界面。bootstrap历史待审与本轮真实unknown分集合，整组最多32个选中地址只可解除restart/history holds，真实unknown、实际owner、任务、临时任务、隔离和everObserved历史不清除。

原Vehicle/MM/参数/VLM/MVM/model及主链路共享所有者冻结，新鲜2秒内原始HB未解锁与已知ON_GROUND、完整本机队列排空进入30秒复核。专用fleet.occupancy.recover.approve强制不同真实账号，一次消费不生成lease；原账号TTL、当前Episode/角色在最终无回调尾栅再核。现场处置编号和地面检查是明确人工声明，不认证物理停机、远端无任务或控权。

所有可回调读取后，整组再以纯native原RAW语义revision/hash/value/鲜度/队列核对，并核原worker、实际占用和任务map；同栈整体移除本地待审后bump，新actual身份须由原worker实际提交才恢复既有continuity getter/CAS。留存原QQ、源epoch与primary控制块，换源在getter/最终CAS即时重加hold，不只等timer；正常收包、合法起飞和新MM工作不冒充换源。

本批12源码/工程登记、17文档，共29路径。原生recoveryWorkflowImplemented与UI集成分开；localHoldsReleased仅历史动作事实，selectedReviewRequiredVehicleIds另报当前待审。仍缺UI、离线/报废现实处置、本轮真实unknown受控解除和完整机载自愈；完整25目标/六类实质开发保持。未编译、测试、运行helper/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方401–405及更早是历史状态，不以旧“无任何再审入口”覆盖本批native入口，也不宣称全部恢复产品已完成。

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

随累计完整工程交接新证据helper和Swarm/Orchestrator/Guided/Continuity整组依赖；[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列出撤权rawmap、旧对象丢失、未知保留、重入暂存、pin与末次提交的待验证项，不能当作实机通过记录。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](../../custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

交接须带累计完整工程及新matcher h/cc和pri，不是只拷单个角色模块；[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列出角色来源、复合故障完整性、批内排他和CAS待验证要求。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](../../custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

请随累计完整源码、工程登记及 [本轮路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md) 一起交付；[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)列出当前正向审批与失效/导入/Qt往返的待验证要求，不能将其当作测试通过记录。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

## 371–375：下一部署/历史选择的累计源码交接

最新说明见 [工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[补丁清单](MERIVUS_SWARM_PATCH_MANIFEST.md)与 [LifecyclePanel](../../custom/src/Swarm/FleetMissionLifecyclePanel.md)。本批8源码/19文档必须连同累计完整源码、pri、注册和原生依赖交接，不是8文件独立包；当前没有更新 PR 或生成安装包。

两个专用 `ForController` 接口保持 native begin → 私有栈 permit mutation → 新 scope/原 ID end，不释放后走 legacy。两个 Root fixed API 公开12键 metadata/6键组行，不公开 raw refs；指挥中心/规划页两入口的面板双步确认、默认关闭/取消、不用 Enter 提交并固定原 view ticket。提交事实与 end 分开，allocated=false 不证明失败尝试没消耗8组容量；prepare 不自动换 selected、登录/继承 grant、停止旧 Run 或释放占用。

工程师仍需实际构建与验证 Qt 类型/方法绑定、模态/销毁、ABA、失败计费和原占用保留；本机未编译、测试、解析/运行 helper、认证、应用或设备，限定文本检查记录已同步本轮静态审计。`outboundIntegrationComplete=false`，25目标/六类继续。以下366–370及更早交接范围是历史，旧入口/API尚未实现的状态由本节覆盖。

## 366–370：五源码同批交接，产品入口仍未闭环

“5源码”只指相对前阶段的增量，必须交接累计完整源码、pri、类型注册及原生依赖，不是单拷5文件编译。Qt目标环境的22键 Int/QObjectStar往返、方法绑定、工厂actual parent和模态销毁须工程师实核，不放宽严格合同；本批未打包或更新PR。

本批涉及 Lifecycle.h/.cc 的三 native call、CustomPlugin 两类注册、Guided 私有工厂/14引用/30入口/24回调与明确身份意图、ObservationPanel 的 selected token/完整收据路由；18文档同步，详见[工程师交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[作用域契约](../../custom/src/Swarm/FleetMissionControllerScope.md)。不要混用旧独立14实例或少参数调用，也不要自动 prepare 已由构造提供的 initial。

原 owner+ID finally、纯引用 scope、不因忙自拒的回调、未知/已 queue 不回滚和原 retained Close 取消必须一起保留。新部署/选择产品入口及专用 transition API 仍未实现，`outboundIntegrationComplete=false`，不声称完整多部署产品完成。25目标/六类继续；已完成限定文本审阅，具体证据见本轮静态审计，不是运行或安全通过，未编译、测试、运行、更新PR或打包。下方是历史批次。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](../../custom/src/Swarm/FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](../../custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](../../custom/src/Swarm/FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

## 346–350：十二类原生服务门禁接线

[Episode服务接线](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)已覆盖另外12类服务的新请求、live依据、审批/身份移交及现有scoped发送fence：归组来源须仍为current且owned peer属于同一实际组；旧取消/revoke/drain、被动Run/Close和真实已queue记账不借此封禁。Authority/Txn认证移交上下文改为纯本地比对，完整身份/风险门禁仍在调用两侧；Series保留严格一次未发布构造绑定。

仍未形成新部署可用闭环：普通Deployment上传/回读的legacy PlanManager在插件后、异步每包/重试没有独立Episode fence；Guided actual Episode/epoch、共享Role身份上下文和selected旧观察整组投影尚未接入，无新QML注册/部署入口。只改源码及限定静态审阅，不编译、测试、运行认证/应用/设备、更新PR或打包；原25目标/六类继续。


## 345：非 Close 用途的原生身份门禁

[Session 身份接线](../../custom/src/Swarm/FleetOperatorSession.md)已加入上传/启动/ground用途的current Episode检查、实际owner配对及OS/owner回调后的再次核对；旧组普通身份事实、显式登录/退出、精确撤销和独立Close用途保留。相同账号/UUID或不同工厂的当前对象不能替代同一实际组。

这是340–344核心后的源码增量，不是全链隔离完成：其余owned发送/审批入口、Guided actual Episode/epoch和selected旧观察投影仍未完整接线；无QML注册/新部署入口，现有应用仍单链。只做源码与限定静态审阅，不编译、测试或运行认证/应用/设备，未更新PR或包，原25目标/六类继续。


## 340–344 本批交接：原生核心，尚非可操作的新部署

先读 [Lifecycle](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)、[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)和[补丁清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。新核心保留旧实际组/Run/占用，每代新建14个 owned 对象和 signed-out Session，固定五个实际共享来源；全部 retained 原生静默后才可构造候选，8次分配含初始/失败尝试，不驱逐历史。

**当前未注册 QML、未替换 Guided、无新部署入口，现有应用仍单链。** native sticky outbound helper 已定义，但完整 native 门禁、QML actual Episode/epoch/source callbacks、共享 Role 身份意图及 selected Run/Series/Close/Session 同组投影仍为后续源码工作，不可拿相同 binding 跨代复用。

限定文本审计结果已记录；未编译、测试、运行 API/helper/认证/应用/SQL/设备/刷机，未更新 PR 或打包。原25目标/六类不缩减，不能将此批作为工程放行证明。

## 335–339 当前交接范围

本批在330–334后端基础上新增[监视操作面板](../../custom/src/Swarm/FleetMissionObservationPanel.md)，包括6个源码/资源/模块路径和18份说明。新QML及md可能未跟踪，qrc/qmldir、两个实际入口和controller无回退增量须同批；旧PR/旧包不含本批。

先阅读[工程师条目](MERIVUS_SWARM_ENGINEER_HANDOFF.md)与[限定审计](MERIVUS_SWARM_STATIC_AUDIT.md)。未编译/测试/启动界面或认证/操作设备，没有制作安装包或更新PR；监视结束与飞行停止/Completed/跨站移交分开，完整路线图继续。

## 330–334 当前交接范围

新增[独立关闭本地监视](../../custom/src/Swarm/FleetMissionObservationClose.md)及专用两账号身份消费、实际owner/占用检查。完整交接须包含新Close三文件、Role说明，以及manifest列出的14个源码/工程路径和19份配套文档；旧PR/旧源码包不是本批。当前不生成安装包、更新PR、编译/测试或操作设备。

先阅读[本批工程师条目](MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[审计边界](MERIVUS_SWARM_STATIC_AUDIT.md)。没有新增UI按钮；结束monitor和释放本地占用不意味着停飞、任务Completed或成员可重新分配，正式任务结果/StartPermit及其余原25目标/六类仍有开发缺口。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](../../custom/src/Swarm/SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

320–324（2026-10-15）新增[显式 HOME 观测过渡](../../custom/src/Swarm/FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

315–319（2026-10-15）新增[本次启动的连续运行观测](../../custom/src/Swarm/FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

310–314（2026-10-15）新增[错误可区分的原生目录清点](../../custom/src/Swarm/FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

305–309 新增[遥测只读历史检索](../../custom/src/Swarm/FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

300–304 新增[原生入站遥测记录器](../../custom/src/Swarm/FleetTelemetryArchive.md)：独立Vehicle pre-plugin被动入口、实际PRIMARY/Vehicle/observer绑定与退休、27消息固定Codec、parentless有界NDJSON worker及native确认/公开只读值已接线。新增默认关闭，不改旧.tlog，不请求数据流或控制；定位类另明确确认，原len payload与RX序号只观察，签名/CRC/身份不认证。文件NewOnly不覆盖或删除，入队/write/flush/durable分层，源失效停止整组并保留unknown尾部；fullTelemetryCaptured始终false，不把此子集当完整遥测或任务许可。运行/编译/测试/解析helper/设备均未执行，未改PX4/刷机/更新PR/打包；原25目标/六类不缩减。下方295–299及更早为历史批次。

295–299 新增[跨封存段只读浏览](../../custom/src/Swarm/FleetPersistentEventStore.md)：Sql唯一schema/身份/严格页抽取、parentless单请求Reader与Store native目录/分页/取消及public值facade已接。actual目录条目生成本catalogId，页域绑定文件/DBepoch/sealRun/filter，最多128个每段单filter游标，只0或实际next；当前页/Reader共单槽，取消与错误不提前复用，历史不回灌rules/Replay/任务/权限。目录128段/4096条目与JSON页有界，合法1MiB政策的内部预留预算已修；目录全集、全库记录hash、跨段原子快照/飞行序都不宣称已验证。仍缺全集索引/自动归档清理、完整遥测/运营集成及最终StartPermit等原六类/25目标；未编译/测试、运行SQL/目录或hash helper、改飞控/刷机、更新PR或打包。下方290–294及更早说明为历史批次。

290–294 新增[显式维护封存轮换](../../custom/src/Swarm/FleetPersistentEventStore.md)：实际worker协作锁贯穿排空/SQL关闭与同目录旧库保留，native确认后才请求，不自动删旧段；actual clean/sealed/finished、精确DBepoch/文件名和同源QQ/epoch核对后才沿本次明确配置建立新run。取消/退出/源丢失只退休续建，不抹掉可能已发生的封存事实；暂停序号窗口显式known/unknown及缺口摘要，不称连续无损。封存段数有界、未知文件不覆盖，hash/协作锁不认证或保证恶意并发/失电安全。仍缺跨段查询/自动归档清理、全遥测/运营集成及最终StartPermit等原六类/25完整目标；未编译/测试/运行库或锁/文件轮换、执行SQL、改飞控/刷机、更新PR或打包。下方285–289及更早记录为历史批次。

285–289 新增[默认关闭的连续诊断归档](../../custom/src/Swarm/FleetPersistentEventStore.md)：实际BlackBox暂时收据/同类型事件身份、固定摘要Codec、专属QtSql线程与Store门面已接pri/type/controller；仅native显式配置/确认后异步开库，默认不读取或写盘，不回录开库前历史。queue+in-flight/批次/主库页/历史结果有界，实际commit后才durable，来源/投影/入队缺口保留诊断；历史单槽直到GUI交付，不进入实时回放、规则、机队、审批或控制链。无原payload/凭据/精确GPS，哈希非签名/匿名化；clean非完整性认证，未知库与需hot-journal恢复库拒绝不重建。仍缺全遥测/长期轮换/运营集成及最终StartPermit等原六类/25完整目标；未编译/运行/测试、建库/SQL执行、改飞控、刷机、更新PR或打包。下方280–284及更早批次为历史记录。

280–284 新增[显式局部运动与航向观察政策](../../custom/src/Swarm/FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

275–279 新增[被动 ODOMETRY 原生账本与契约](../../src/Vehicle/FleetStartupOdometry.md)：原始 MAV2 解码50/reference42，Vehicle full27/reference21；实际 PRIMARY/PX4/版本与飞行来源、接收整栈、prepare前计数和token/epoch防嵌套覆盖。sticky everObservedRawFrame 不把曾断流/无效/被过滤的源当初始缺流；Readiness仅存去age的20字段参考投影，普通收包/时间/运动数字/quality分值不造成身份抖动，reset/frame/状态与已观察ABA撤销旧依据；地面READ冻结该投影，Transition前后重核实际来源。稀疏协方差NaN保留未知，扩展尾零/LOCAL_FRD/时钟不认证参考有效性或同采样。不增加传感器、不请求/启用流、不改原六通道/read34/startup32、不放宽HOME/local严格政策。源码接线与有限静态审阅，不是完整运动/转向/净空/最终StartPermit或执行完成；原25目标/六类继续，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方270–274及更早批次是历史记录。

270–274 新增[有界实际启动事务序列](../../custom/src/Swarm/FleetMissionStartupSeries.md)及多轮地面再审后端：最新已初始化owner不回退，逐轮冻结actual QQ/UUID/epoch/generation/hash，旧转交链精确退休后重新完整读取/已阅/两账号审批；每次新消费者保留前代，初始/终态对象不能重用覆盖。native固定历史摘要6防递归复制，完整当轮任务/新风险/真实queued事实仍保留；同一原组/记录/部署，最多8个ground child（空候选也计费），源丢失/容量满/MODE已可能排队/未排空均不淘汰或恢复授权。实际pri/type/controller已接，限定源码复核完成，未编译/运行/测试/API/飞控改动/刷机/PR或打包。容量/新部署重置、HOME有效分支/local frame/运动转向、净空/最终StartPermit/时序/执行完成及原25目标/六类仍待开发。下方265–269及更早批次是历史记录。

265–269 新增[地面再审后的分阶段启动事务](../../custom/src/Swarm/FleetMissionStartupTransaction.md)源码：actual8/context27、一次28→独立30（Session6/Authority10）、同UUID连续占用；独立第二实例不覆盖旧终态。已解锁成员不发ARM，但仍核实际Seq0完整journal与整组屏障；未解锁成员仅普通ARM(param2=0)，之后仅mode-only。首次Seq0前要求实际周期RAW基线/政策gap，错误实例在换generation前拒绝；后端接线且限定源码契约复核完成，无新执行按钮。本机未调用接口/编译/运行/测试、改飞控、刷机或更新PR/包。最终StartPermit/净空/完成、第二事务再次终止后的实际owner重审路线、HOME有效分支/local frame/运动转向及原25目标/六类仍待开发；旧第一终态不能代替第二事务事实。下方260–264及更早批次为历史记录。

260–264 新增[地面再审启动依据一次接管](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)源码：实际10来源/原始context27、完整新RAW/全风险/记录/typed当前地面状态进入新owner，旧22一次移交Session独立28（6字段返回真实remaining）、Ledger10字段接管。旧账本退休不盲撤新owner，原最早READ START/两身份期限只缩短，独立实时查身份/WTS/源与整组占用；public不泄露SID/RAW，所有执行/启动/飞行许可false。实际后端/pri/type已接，本批限定源码契约静态收口已完成，无读取/ARM/MODE或UI执行按钮。真正分阶段模式消费者、HOME分支/local frame/运动转向、净空/最终StartPermit/完成及其它六类/原25项目标继续；未编译/运行/测试、调用接口、改飞控、更新PR/源码包/安装包。下方255–259及更早批次为历史记录。

255–259 新增[地面再审独立账号条件意图](../../custom/src/Swarm/FleetMissionGroundStartApproval.md)源码：实际新 Review20/九对象/冻结整组与 armed 状态、完整最新记录/风险/新 RAW，真实两账号新提示和 Session 独立22证明，一次消费/撤销及 native14 grant；public27 receipt 无 SID/RAW。最早 READ START 原30秒不续，旧证明不恢复，实际提示期间主身份仍查 SID/role/WTS/旧期限；active READ与消费不放宽。实际 backend/pri/内部类型已接，本批已完成限定源码契约静态收口，没有新增执行按钮/新 startup authority/MODE续作。最终许可/净空/HOME有效分支/local运动转向/完成及其它六类/原25目标继续；未编译/运行/测试或调用接口、改飞控、更新PR/源码包/安装包。下方250–254及更早批次为历史记录。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](../../custom/src/Swarm/FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

MERIVUS 是基于 QGroundControl Custom Build 的多无人机调度地面站。当前仍是开发测试阶段，不是生产版本；不得把界面存在、代码存在或历史测试结果表述为当前版本已经验证。

## 接手顺序

1. 阅读仓库根目录 `AGENTS.md`（若本机提供）与 [`docs/INDEX.md`](../INDEX.md)。
2. 执行 `git status --short --branch`、`git log --oneline --decorate -15` 和 `git remote -v`，以当前代码、配置和 Git 历史为事实基线。
3. 阅读与任务直接相关的架构、开发或硬件文档；不要把整套文档当作必须逐篇阅读的交接快照。
4. 优先做静态检查和覆盖本次改动的局部验证；只有在环境具备或风险要求时执行完整构建、SITL 或集成测试。

## 当前安全边界

- AI 链路只回答、解释和展示 `ActionProposal`，不连接 Vehicle、MAVLink、PX4 或 Swarm 执行入口。
- 人工多机控制入口与 AI 建议链路必须分开审查。
- 不自动连接真实飞机、生产服务或云模型，不自动修改 PX4/RTK 参数。
- “已安排”“已上传”“收到 ACK”和“动作完成”是不同状态，不得混用。

## 单一真相源

- 当前能力与限制：[`CURRENT_STATE.md`](../architecture/CURRENT_STATE.md)
- 接口与安全契约：[`INTERFACE_CONTRACTS.md`](../architecture/INTERFACE_CONTRACTS.md)、[`SAFETY_BOUNDARIES.md`](../architecture/SAFETY_BOUNDARIES.md)
- 构建与 Agent 开发：[`BUILD_WINDOWS.md`](../development/BUILD_WINDOWS.md)、[`AGENT_DEVELOPMENT.md`](../development/AGENT_DEVELOPMENT.md)
- 可重复验证入口：[`docs/development`](../development/)
- 版本历史：[`CHANGELOG.md`](../releases/CHANGELOG.md) 与 Git/PR 历史

## 本批次交接入口

当前244–249已完成有限源码静态收口。按下列契约区分已写入、仍缺失与待工程师实操；源码/ACK/mode观测不是放飞许可。合法HOME再审/local参考/运动/转向及最终执行完成仍需代码开发；未编译/运行/测试或更新PR/安装包。

- 集群地面站与 Fleet OS 本批次工程师交接：[`MERIVUS_SWARM_ENGINEER_HANDOFF.md`](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- 本批次源码文件与静态检查清单：[`MERIVUS_SWARM_PATCH_MANIFEST.md`](MERIVUS_SWARM_PATCH_MANIFEST.md)
- 本批次静态审计记录（不含编译/仿真/硬件证明）：[`MERIVUS_SWARM_STATIC_AUDIT.md`](MERIVUS_SWARM_STATIC_AUDIT.md)
- MAVLink ACK/PlanManager 操作身份专项审计：[`MERIVUS_ACK_IDENTITY_AUDIT.md`](MERIVUS_ACK_IDENTITY_AUDIT.md)
- 集群目标与当前缺口：[`MERIVUS_SWARM_TARGET_MATRIX.md`](../MERIVUS_SWARM_TARGET_MATRIX.md)
- Fleet OS 协同、交接、行业模板和插件注册：[`MERIVUS_FLEET_OS_COLLABORATION.md`](../MERIVUS_FLEET_OS_COLLABORATION.md)
- Fleet OS 意图与风险方案：[`FleetIntentRisk.md`](../../custom/src/Swarm/FleetIntentRisk.md)
- 受控只读版本/参数刷新及工程验证边界：[`FleetMissionEvidenceRefresh.md`](../../custom/src/Swarm/FleetMissionEvidenceRefresh.md)
- 条件启动意图与独立账号依据（非StartPermit）：[`FleetMissionStartApproval.md`](../../custom/src/Swarm/FleetMissionStartApproval.md)
- 启动前整组机载任务读取/最早原读取时效（非StartPermit）：[`FleetMissionStartContentCheck.md`](../../custom/src/Swarm/FleetMissionStartContentCheck.md)
- Startup实际账号/完整来源依据一次接管及精确取消：[`FleetMissionStartupAuthority.md`](../../custom/src/Swarm/FleetMissionStartupAuthority.md)
- Fleet OS 黑匣子、回放与预演：[`fleet-event-replay.md`](../fleet-event-replay.md)

交接文档只说明源码接手、编译、SITL/故障注入和放行边界，不替代工程师的真实硬件测试报告。

阶段性排障记录、旧环境说明、账号迁移清单、源码全文副本和二进制说明书不再作为仓库长期文档；需要时从 Git 历史或对应任务记录恢复。

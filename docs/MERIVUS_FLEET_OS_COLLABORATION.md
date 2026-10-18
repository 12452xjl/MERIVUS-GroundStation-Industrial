# MERIVUS Fleet OS：协同、交接与扩展基础层

## 546：本地操作与外部权限明确分离

五条新接线均沿现有来源与状态：规则处理只审阅建议；检查点只保存/读回保留Run；模板复制新草案并退休原批准；留存沿原维护槽保留逐项事实；四受众报告只冻结已有审阅资料。文件保存受理、实际写入和回读分别处理，取消不回滚已提交事实，历史文件不恢复控制权。

报告完整文件包含工程原值；customer视角仅显示裁剪，不是脱敏或权限控制。四受众报告与protected实际Run报告格式分离。留存的可恢复退休不等于永久清理，原独立危险清理确认不被旁路。

依据用户确认，跨站账号/撤销/控制租约、正式机载任务结果及动态插件执行系统仍列[外部未完成项](handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md)，本轮不搭建服务、不改PX4、不合并PR。源码静读不是工程验证。


## 545：证据链文件流进入现有指挥中心

现有生产链经窄门面连接后台冻结保存和独立历史槽；不用旧import替换活动链，也不恢复旧操作者、批准或任务。原生单工作槽、完整链验证、明确历史ID及分页/预览上限与界面保持一致。UI按路径确认、原对象寿命和操作状态处理；关闭页面不伪称撤销已写文件。详见[工作流](../custom/src/Swarm/FleetEvidenceArchivePanel.md)。

## 544：编译成员与下游路线统一

普通公共路线在Compiler一次展开到缺省成员，不再仅存预算或名义时间计算的临时副本。原始别名和未编码要求在新执行路径上保守拒绝；公共时序只填缺失字段族，原成员声明优先并复核合并结果。原PlanStore、Timeline及余程消费者沿现有成员合同，不另开权限入口。见[合同](../custom/src/Swarm/FleetMissionCompiler.md)。

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


本批次在现有 QGroundControl Custom Build 的集群层旁边增加五个独立、传输无关的产品能力模块。它们只管理人工操作权限、任务元数据和扩展目录，不直接访问 `Vehicle`、MAVLink 或 PX4，也不会绕过现有的解锁、链路、估计器和任务安全检查。

## 模块

| 类 | 作用 | QML 类型 |
|---|---|---|
| `FleetRolePolicy` | 观察员、规划员、操作员、安全员、维修、总指挥、管理员的权限模型；返回允许/拒绝原因和二次确认要求 | `FleetRolePolicy` |
| `MissionHandoffManager` | 记录任务从一个操作员/控制中心交接给另一个操作员的请求、接受、拒绝、取消和过期 | `MissionHandoffManager` |
| `FleetTaskTemplateRegistry` | 注册巡逻、工业巡检、搜索救援、仓储盘点等行业任务模板；提供三段式版本、能力/任务约束校验和 JSON 交接 | `FleetTaskTemplateRegistry` |
| `FleetExtensionRegistry` | 登记审计、报告、行业插件等可选扩展的元数据；默认关闭，不加载任意二进制 | `FleetExtensionRegistry` |
| `FleetAutonomyPolicy` | 只读声明人工/辅助/监督/自动化阶段，以及 PX4、ArduPilot 和伴随插件的能力、权限、签名/沙箱占位 | `FleetAutonomyPolicy` |

## 使用边界

这些类是“策略/目录层”，不是飞行控制器：

- `FleetRolePolicy::canPerform()` 只表示当前角色是否可以执行某个人工 UI/任务动作；即使返回 `true`，命令适配器仍必须重新做车辆健康、位置、链路、解锁和安全前置检查。`evaluate()` 返回的 `reviewOnly=true`、`executionAllowed=false` 和 `commandDispatch=not-released` 固定表明它不是执行令牌。
- `MissionHandoffManager::acceptHandoff()` 只确认任务所有权转移，不代表任务已上传、收到 MAVLink ACK 或飞行器已改变模式；摘要带有 `recordRevision`，接受/拒绝/取消可传入 `expectedRevision`，版本不一致时拒绝写入，避免旧界面覆盖新状态。
- 交接接受/拒绝/取消会在解析动作内再次检查 `expiresAt`；即使 1 秒周期过期定时器尚未运行，已过期请求也会先转为 `expired` 并拒绝该动作，不能利用定时器间隙完成交接。
- `MissionHandoffManager::exportJson()` / `importJson()` 只保存和恢复版本化的交接记录；导入采用全量校验后原子替换，严格拒绝未声明的根/记录字段，并要求快照携带全局 `sequence`/`revision` 与每条记录的 `recordRevision`，避免恢复后把旧视图误当成当前版本；不恢复控制会话、不释放飞行命令。
- `SwarmMissionOrchestrator` 的任务摘要带单调 `revision/taskRevision`；连续性建议会回显生成时的来源版本，执行适配器必须在人工确认前重新比对，不能把过期建议当作当前任务版本。
- `requiresSecondApproval` 当前只是交接记录中的审批要求声明；此类源码没有第二审批人账户/服务端鉴权接口，工程师接入前不得把 `acceptHandoff()` 当作双人放行。
- `FleetTaskTemplateRegistry` 只提供带版本的意图模板、能力/任务约束校验和结果格式提示，不能替代航线规划、禁飞区校验、电量预测和人工确认。`exportJson()`/`importJson()` 是目录交接接口；签名字段只是未验证占位，不能据此启用模板。
- `FleetExtensionRegistry` 只存储扩展元数据。生产环境还需要签名校验、版本兼容检查、沙箱和权限审批，不能把 `entryPoint` 当作可执行路径直接加载。
- `FleetAutonomyPolicy` 只返回阶段与适配器策略结果。`signatureStatus=unverified`、`sandboxProfile=*_pending` 和 `enabled=false` 不代表适配器已获准运行；签名、版本、权限和沙箱由宿主产品完成，所有结果固定 `flightCommandReleased=false`。

## 推荐接入方式

1. 登录或控制中心切换时设置 `FleetRolePolicy.currentUserId/currentRole`。
2. 人工动作按钮先调用 `evaluate(action)`；被拒绝时向用户展示 `reason`，二次确认动作显示 `requiresSecondApproval`。
3. 任务交接由发送方调用 `requestHandoff()`，仅目标操作员可以 `acceptHandoff()` 或 `rejectHandoff()`；发送方可在接受前取消。
4. 任务向导从 `FleetTaskTemplateRegistry.templates()` 选择模板，先调用 `validateTask()` 检查任务和舰队快照，再交给现有任务编排器生成预览；模板本身不触发执行。
5. 扩展启用前由宿主产品/C++ backend 完成签名、权限和版本检查，再调用 `setExtensionEnabled()`；QML 目录面板不具备该写入口。禁用/移除扩展不会影响在飞任务。
6. 渐进式自主流程先读取 `autonomyPolicy.evaluate(stage, action, role, adapterId)`，再由 `FleetRolePolicy` 和车辆安全门禁复核；当前版本没有自动化命令释放路径。

## 工程师验收重点

- C++/moc 能编译，Qt 5.15 QML 可实例化五个类型。
- 观察员不能创建/启动任务；规划员只能创建、编辑和预演；操作员的飞行相关动作应要求安全策略再次检查；安全员可审批/中止但不能凭角色绕过飞控安全。
- 交接只能由目标操作员接受或拒绝；重复解决、错误目标、自己交接给自己都必须失败。
- 模板 ID、扩展 ID 重复注册时应覆盖元数据但不自动启用，也不执行任何外部代码；模板版本必须为三段式数字版本。
- `FleetAutonomyPolicy` 的四阶段权限、适配器筛选、未知阶段/适配器拒绝、未验证签名和待建沙箱状态应保持可解释；无论输入如何都不得生成或发送飞行命令。
- 模板 JSON 导入必须先完整校验后原子替换；损坏 JSON、重复 ID、无效版本或反转的约束范围不得改变当前目录。
- 断开地面站或重启后，尚未持久化的内存记录不应被当作任务已完成；如需跨进程恢复，另行实现持久化和审计日志。

这些检查是接口验收，不构成 SITL、硬件或刷机放行。真实动作仍由现有 `SwarmController` 和工程师的测试流程负责。

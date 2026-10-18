# MERIVUS Fleet OS 静态审计记录

## 546：最终本地源码限定静态审计

范围为本轮26源码/工程登记及26文档，共52路径，五条真实用户链。作者静读与非作者分工：规则原生由486实现、Root核对，UI由Root实现并由486核对；Run由468实现、Root和486核对；模板由522实现、486核原生、Root核UI/facade；留存由468实现、Root核原生、486核UI/facade；报告由522实现、486核工作区、468核Builder/Coordinator、Root核UI/facade。该描述是有限源码阅读，不是安全认证、全库无缺陷证明或运行测试。

已关闭的确定问题：检查点宽毫秒时间戳误用qFloor(int)；规则Double到qint64的2^63边界及opaque typed-null；模板在owner线程门之前读私态；已哈希report再追加根字段导致digest失配；capture/open失败使UI丢旧副本；冻结历史保存被无关live来源变化撤确认；新Item视图使用state命名与内建属性冲突。留存仅actual worker isFinished后消费终态，30秒预览用PreciseTimer且原生再核时钟；捕获来源销毁不复活原计划。无新候选/公开状态不变时不重发规则待审通知。

正常链核对：四条/有界分片→精确ID/版本→明确确认；后台文件单槽、实际结束、取消不抹写入事实；模板生成新草案、datum与批准重新取得；Run/报告只读历史不恢复权力；留存串行保部分移动/备份结果但不自动永久删除。四受众完整JSON明确含工程资料。

封包前普通文本核对：52条路径全部存在，无冲突标记、无行尾空白；26文档中7267个本地Markdown目标无缺失；本节清单52行、工程交接38项全部标未执行。已跟踪文件git diff --check退出0；本轮34条相对HEAD未跟踪路径的no-index空白核对无诊断（未跟踪不等于本轮新增）。这些仅是文本/路径检查。

未编译、执行项目/QML/helper/解析/hash/SQL/模型/业务文件IO、测试或仿真。最终源码封包不是对这些实现的运行验证。[外部未完成项](MERIVUS_FLEET_DELIVERY_SCOPE.md)按用户确认单列，不冒称完成。


## 545：证据链文件工作流——直接静读

已沿原事件生产者→Guided窄门面→原EvidenceChain后台→共享SnapshotFileIo→独立历史页/详情核对；没有建立新活动链或权限旁路。原始version1保留，保存先累计预算再逐条编码，后台单槽只在真实finished后释放；已提交写入不因取消/读回错误而抹去。末审将原共享索引核对从toInt窄化改为整数门后的宽域精确比较，避免大整数别名合法小索引；合法原记录格式不变。

界面静读发现并修正三处：覆盖勾选的旧按下可确认新路径、通知后未核销毁/换源、文件类型可能被按钮当富文本显示。现采用路径/epoch冻结、通知尾核、PlainText；原固定指挥面板入口改用有限高弹窗与滚动，键盘焦点和主题遵循UI/UX静态指引。原生后台与UI分别完成非作者交叉静读，限定新增范围未见其它确定缺陷。没有运行技能脚本、QML解析器、界面、文件IO、hash、模型或测试。

普通文本核对：21路径均存在、清单21行、工程12项全部未执行、14文档7052个本地链接目标有效；无冲突标记/行尾空白，tracked diff --check为0，7个untracked文件无空白诊断。实际新QML类型/qrc登记、Overlay实例及Guided对原生5接口调用逐文本核对存在。它们不能作为构建、运行或视觉验收；没有阶段包。

## 544：公共航线展开——直接源码审查

实际断点是Compiler允许普通公共路线，而只在预算/名义时间临时副本上补齐；PayloadBuilder和Timeline读取的真实成员没有路线。本次将规范展开前置到成员形成，后续只读同一实际成员。新路径核原raw去返程、未知成员要求、别名整族优先、合并后时序、参考和明确Bool；不放宽原上传点数、坐标、几何、性能、人工确认和StartPermit检查。

交叉静读覆盖Compiler、Coordinator、PayloadBuilder、PlanStore、Timeline及单机余程；没有运行任何编译器/helper/模型/解析器或项目逻辑。发现并补上成员/公共混合后反向时间，以及任务根已有时序/参考声明在新执行路径中丢失的问题；对应Store保25项原声明，未扩预算或权限。普通文本核对：16路径均存在、清单16行、工程10项全部未执行、14文档7051个本地链接目标有效；无冲突标记/行尾空白，tracked diff --check为0，6个untracked文件无空白诊断。这些不证明可编译或可运行；无阶段包。

## 543：配置速度与逐机路线预算——限定静态审查

本次真实链：原12项参数的最终稳定接收集合 → 有限四项配置建议 → 操作员查看/明确采纳 → 原草稿CAS保存与参数重新核验；以及最终每机路线/profile → 共享几何估值 → 原驻留预算 → 原逐机能量账单 → 原调度和明细。派生字段在载荷构建时按同一路线重新核对；组余程和备用出勤从政策匹配中仅剥离该派生字段并在新编译重算。

界面按UI技能的明确动作、文本原因、保留输入和键盘焦点原则修改；未运行技能脚本、项目、界面或模型。交叉静读发现的兼容点已纳入：旧观察闭19字段、载荷成员白名单及重规划政策投影；采用前先清确认并退休旧预演，再整笔替换四速度。实际工程验证18项全部未执行，见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

10源码＋21文档，共31路径，无新文件。普通文本核对：31路径存在，清单31行，工程场景18项全部标未执行；21文档7079个本地Markdown目标存在，无冲突标记及行尾空白。tracked范围git diff --check返回0，20个untracked文件的no-index空白检查无诊断。这不是编译、解析、模型或业务测试。原25目标尚未全量完成，不制作阶段包；未编译、测试、仿真、刷机、修改PX4、提交或更新PR。


## 542：编队等待与重新确认——限定静态审阅

实际链路：指挥中心明确保存选中机号 → Swarm原生会话队列 → 原Swarm占用与原Orchestrator轻投影 → FIFO重新审阅 → 新Guided确认令牌 → 原本地任务预约与原sendStartCommand。队列不自行发送，不把就绪/请求提交映射成健康许可、ACK或完成；原协议与任务执行权限保持。新增投影只保最多255个机号的首个阻塞任务，避免定时搬运完整Run观察。

队列上限32（含终态）、待办30分钟/审阅60秒、字符串uint64修订、一次token消费、实际来源固定、dispatching不可取消及原回执收尾均由原生代码控制；界面只给原ID/revision，不回传可编辑任务/批准。机号范围沿原UAV-1～UAV-6且必须含1。派发失败或结果未知不自动重试，取消待办不是撤销机载动作。

源码与消费入口按作者/非作者交叉静读。新接线中发现并修正：取消待办后旧确认可能降格成普通发送、提取到公开QML helper后缺私有一次派发凭据、签发函数不能返回确认令牌、未进入发送的本地预约尾清理，以及机号范围/原因长度与面板消费不一致。原生bind逐连接核原对象寿命；新面板在原裁剪宿主内使用有界滚动，避免后行/分页无法到达。最终代码又直接读回，限定范围未见另一明确问题；这不是运行验证。

UI技能仅用于复用QGC控件、明确状态/中文原因、按需展开、四条分页、PlainText和键盘焦点设计；未执行搜索脚本、界面预览或视觉验证。20项工程要求全部未执行，见[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

12源码/工程登记＋16文档，共28路径。普通文本核对：28路径均存在，清单28行，工程场景20项全部标未执行；16文档的6946个本地Markdown目标存在，未见冲突标记或行尾空白。tracked范围git diff --check返回0，9个untracked文件的no-index空白检查无诊断。这不是编译、解析、业务执行或测试。原25目标尚未全量完成；没有阶段源码包、提交、PR更新或刷机。


## 541：任务模式退出证据贯通——限定静态审查

实际链路是 post-commit HEARTBEAT 原生观察 → 受保护 TaskRecord → 完整/保护报告和交接简报 → 原生语义快照/发布回执 → BlackBox/归档v4 → 原报告与历史界面。新增共享校验器使用完整 UInt32 原模式、严格类型、同源前后引用及首末计数关系；既有终点候选、源退休、预约与飞行控制语义不变。多个边沿可合并，完整性仍明确为false。

作者与非作者直接交叉静读生产者、共享校验器、报告版本门、原控制器接线、归档及四个原界面。发现并修复了交接父元数据预算不足、Qt invalid/JSON null在新零次槽中的表示不一致，以及 full v2 摘要未观察序号在 QML stringify 后丢闭键的问题。修复后又直接读回；不是运行验证。隐藏消费者静读覆盖 Episode/Lifecycle、部署归档Worker、SQL/HistoryReader/TaskIndex，未发现另一个本次版本升级硬门。

v1资料不补造新证据；normal event2只用v4投影，旧event1/v3与gap1保留。新本地随机epoch/序号/单调毫秒的留存是显式版本扩展，不作远端身份或因果认证。报告保原2MiB限额；v2 brief仅深8/4096节点、96KiB，精确父wrapper深9/4098节点/128KiB，旧generic限制不变。

UI/UX技能仅影响原页面的按需展开、PlainText、长引用换行，以及“未采集/未观察/非结果”文字区分；未执行技能搜索、界面预览或交互检查。26项工程场景全部未执行，见[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本次不宣称最终StartPermit、正式机载结果或完整25目标已完成；无阶段包，继续面向统一交付。

普通文本材料核对：41路径存在，清单41行，工程场景26项且全部标未执行；23份文档中6929个本地Markdown目标均存在，未见冲突标记或行尾空白。tracked范围git diff --check返回0，26个untracked逐一no-index空白检查无诊断；这不代表编译、解析或运行通过。

仅修改源码、文档并直接静读/普通文本检查；未编译、测试、仿真、执行解析器/校验器/算法/QML/SQL/hash/文件业务，未改PX4、操作硬件、刷机、提交或更新PR。

## 540：完整计划驱动的逐机时间预演——限定静读

实际链路为原PlanStore的同一审阅记录 → 完整plan/energy快照 → 原生逐机航段/返程/驻留计算 → 私有事件集合 → 现有面板四条分页。7源码／工程登记与15文档，共22路径；不是阶段包或可运行安装程序。

模型严格区分相对规划时间、完整任务时间和真实执行：以各机首点为零，不推断跨机同步；不把AGL/terrain差当AMSL高度差；合法profile未设置返程速度时按原PayloadBuilder持续cruise语义，不给未知机载参数补默认。能量只关联原记录primary组的同机预算，备用机不借入；缺失与-1保持未知。

限定交叉静读修正了捕获busy通知前未冻结原绑定、新来源过早标记旧预演、旧概览通知尾覆盖新明确预演、同源能量未在实际面板显示等问题。原生通知使用对象/代次检查，页面和控制器尾使用plain lifetime；相同版本导入、clear及达到版本上限不能复活旧分页身份。作者及非作者已直接静读新增模型、Twin接口、Controller与Panel，所报问题均修正并读回。地图摘要另保留显式mapEvidenceAvailable=false，不因描述键存在误标有图；未显式声明时保旧摘要兼容行为，不当作地图认证。

普通文本材料核对：22路径存在，清单22行、工程交接22项且全部标未执行；15份文档的6789个本地Markdown目标均存在，未见冲突标记或行尾空白。tracked范围git diff --check返回0，10个untracked逐一no-index空白检查无诊断。此类文本检查不是构建或模型测试。

含clearanceInput的启动门仍要求actualPathCertified，原Domain报告仍不提供此认证；本批没有翻标志、去掉输入或发送新飞行命令。完整25目标继续，不把规划估算等同全量交付。

只修改源码并直接静读，未编译、执行模型／解析器／QML／文件业务逻辑、测试、仿真、修改PX4、操作硬件、刷机、提交、更新PR或打包。22项工程场景全部未执行，见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 539：交接历史保存与恢复——限定静读

本批为原Manager、受限Facade和原Panel的实际保存／预览／仅追加合并流程，涉及4源码与15文档，共19现有路径。不是新增孤立格式或只提供报告；不暴露旧整库替换方法作为恢复入口，也不增加飞行、网络投递或账户恢复动作。

后台捕获实际原生值集合，编解码、文件读写与合并候选计算不由QML执行；载入先生成独立预览。仅明确确认且原集合独立代次仍匹配，才提交整份追加候选。同ID内容相同去重，冲突整拒；新导入记录隔离为历史，不改变本机原记录来源。旧schema1接口兼容行为与新工作区隔离分别记录。

保存复用原FileIo的原子提交与回读事实区分；取消后不能抹除已提交文件，也不能提前归还后台槽。Facade外来intent getter可导致同步销毁的窄边界已补原Manager／plain lifetime捕获与调用前检查；原生调用后的脱离对象回执不再读旧控制器。

作者与非作者已直接静读原生三路径、新门面和新增历史界面，当前限定新增链未见确定剩余问题。界面覆盖确认按下后换路径的旧释放问题、候选delegate包装对象不保证同引用的问题，以及文件声称的Run资料容易被误解为当前真实来源的提示均已窄修并读回。UI/UX静态指引仅用于可见路径标签、确认、错误恢复、焦点与只读区别，没有执行技能搜索或界面预览。

本地QGCCheckBox实际继承Controls1.2，新覆盖确认不能假定它有Controls2的pressed/canceled信号；仅该新控件改用已导入的Controls.CheckBox2.4，沿用原palette与图标，未改全局QGCCheckBox。另对照Qt官方v5.15.2的[Controls1基类](https://raw.githubusercontent.com/qt/qtquickcontrols/v5.15.2/src/controls/Private/AbstractCheckable.qml)与[Controls2按钮声明](https://raw.githubusercontent.com/qt/qtquickcontrols2/v5.15.2/src/quicktemplates2/qquickabstractbutton_p.h)确认这一接口区别；上游源码对照不是实际安装Qt或构建验证。

普通文本材料核对：19路径存在，清单19行逐项对应且无重复；24项编号连续并全部标未执行。15份文档中的6752个本地Markdown目标均存在，无冲突标记或行尾空白；tracked范围git diff --check返回0，6个untracked逐一no-index空白检查无诊断（相对空文件的内容差异返回1，不是测试结果）。

所有源码结论均限于直接阅读，不来自运行。未执行Qt线程、文件保存／加载、JSON解析、哈希、QML、编译、测试、仿真或真实硬件；24项工程场景全部未执行，见[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。文件完整性不是身份认证，保存回读不是fsync／断电持久化或跨站控制权转移。原25目标继续，不制作阶段包。

## 538：持久化任务索引——限定静态审阅

### 实际功能与范围

[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)列12源码／工程登记、16文档，共28路径；新增索引h/cc/md和独立界面qml/md。实际入口为原归档面板内默认收起的索引工作区，经GuidedActions窄门面、Store原读取槽、HistoryReader进入后台checkpoint实现；不是独立演示或只读策略。功能覆盖建立、原子保存、重开、分窗续扫、有界任务页、部分索引的原记录下钻及显式重建。

### 静读发现与修正

- 原目录8192行／256哈希累计预算会消耗续扫资格。新索引将窗口和持久总量分离，保存原扫描位置，不扩大旧目录DTO，也不把全部任务或原记录塞入QML。
- JSON恢复后的结构整数必须规范成native Int后才能走严格原游标验证；合法事件的空taskIdHash计未归属，不以空串或机号猜任务。
- 索引尚未扫完时也允许查已发现任务：首次保存前冻结完整源清单的文件SHA／实际SQL身份，后续逐文件复核。没有用“只能完成后下钻”掩盖未消费文件的来源缺口。
- 源变化后open会拒绝；重启用户仍可二次确认rebuild。后台必须读取固定旧合法checkpoint并保存前比较完整基线，可明确选择新范围；不改原日志。
- Reader晚取消不能抹已commit事实；Store取消后的已提交结果仅发布历史回执，不重装native上下文。UI另核本页取消ID，不能靠公开DTO恢复资格。
- 重开清旧页资格，记录下钻则保同index/hash的实际native页；防止正常交错操作被误锁死，也不让失败后旧页复活。
- 最终交叉静读已覆盖索引、Reader、Store和六条界面接线。界面通知后的对象寿命、重建确认按下／释放间换对象，以及把合法记录页误限为1MiB三项已修正并读回；当前限定新增链未见确定剩余问题，不等于构建或运行验证通过。

### 验证边界

仅修改源码、作者与非作者直接静读，以及普通文本／路径／Markdown链接／git空白核对；没有运行索引、Qt、QML、JSON解析器、SQL、SHA算法、业务文件IO、构建、测试、仿真或真实设备。32项工程场景全部未执行，详见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。普通文本核对确认28路径齐全、清单28行无缺漏／重复、32项均为未执行且连续编号；16文档中的6726个本地Markdown目标均存在，未发现冲突标记或行尾空白。tracked git diff --check返回0，14个untracked逐一no-index空白检查无诊断（内容不同返回1不作为测试失败）。这些不是编译或功能验收。

固定JSON最多16MiB／32768任务，源范围仍受原128封存段（加可选停止库）限制；每操作完整源复核可能耗时。没有性能实测、全历史全集证明、跨文件原子快照或对恶意本机进程的文件系统隔离保证。派生计数不是任务成功、可信身份或飞行许可。长期运营其余保留政策、跨任务统计、灾后恢复，以及原25目标的其他五类实质代码仍继续开发。


## 537：逐 NAV 航点相对驻留——限定静态审计

### 实际范围与证据

[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)包含12源码／工程登记、21文档，共33路径；新增DwellEncoding.h／cc／md。实际功能入口是既有Planner辅助去返路线点，不是独立演示或只读策略。Draft、Compiler、载荷、时间／能量审阅、Joint及三个严格几何消费者接入相同合法性规则。没有新增payload／schema／命令类型或PX4改动。

直接读取本地FirmwarePX4文本：mavlink_mission.cpp:1372／1694直接保存／回传Float32 param1，navigation.h:160为float time_inside；mission_block.cpp:453–454有epsilon即时满足和float乘1e6转整数等待，609–620只对旋翼普通NAV返回该驻留；396–402首次位置接受锁存计时，449后yaw接受与计时联合检查。该源码快照没有可确认Git HEAD，不能证明烧录二进制或运行状态。计时不是持续在半径内、精确悬停或达向后再完整等待。

### 源码交叉审阅

- 真实编辑：可选hold字段只在独立路线开启，stored／恢复／完整source保原请求；heading与hold各自presence保留。整点更新、来源／revision核对、正常commit＋1、取消和过期保输入沿原链，UI只做普通输入检查，不复制浮点算法。端点、自动覆盖及起降／HOLD动作范围不扩。
- 数值helper：实际数型及0..86400，正值不能在Float32／声明微秒模型中归零；乘积先界限后转整。请求、上传、整数微秒与预算四值分开。IEEE32／64、denorm结构及当前RN门不认证硬件flush或编译器重结合行为；这些仍需工程验证。
- 保守而不随意扩时：modelUs可精确转Double，fma残差只在商下舍时上取；完整TwoSum只在sum下舍时上取，0相加／精确1秒／1+1不无故扩ULP。实际源级数值推理已独立静读，但没有执行算式或工具链验证。
- 实际消费者：Compiler与Builder原items只保原秒数，canonical使用上传Float32，原p1 delta和RAW身份不改。Coordinator逐点、两段合计及estimate加驻留均用同预算／累加，明确duration和相邻权威到达窗口核同值；未知航行或真实性能不因该预算变已知。
- 准入：actual relativeDwellEncoded即使无executionProfile也要求原实时版本、参数和当前目录语义匹配；Catalog原有PX4多旋翼观测限制保持。Joint原source p1对应和三strict完整字段／exact Float32不放宽。没有StartPermit、actualPath或正式结果完成标志升级。
- UI/UX：独立勾选、秒单位／范围、原值回填、就地错误及取消恢复沿原Qt控件；未运行技能脚本、QML、渲染或可访问性验证。

作者及独立审阅者已对上述新增链直接静读，当前限定范围未见剩余确定问题。根代理发现并修正Draft内部QString局部变量遮蔽QString*错误输出的确定源码错误；这不是编译发现，没有运行编译。预算由无条件多加ULP改为仅实际下舍上取，以免精确时间窗口被额外误拒。以上不表示无缺陷或业务验收完成。

### 材料核对与剩余

普通文本材料核对已完成：33路径存在且清单逐项对应，无重复；40项编号连续并逐项标未执行。21份文档共6741个本地Markdown目标均存在，无冲突标记或行末空白；tracked范围git diff --check无诊断，22个untracked文件逐一与NUL比较无空白诊断。NUL比较exit1仅说明有内容差异，不是项目测试失败或通过。本检查不执行任何项目逻辑。40项场景见[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)，全部未执行。未编译、执行helper／算法／parser／QML／hash／业务IO、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标与六类实质源码剩余保持，约40%源码／60%剩余仅粗估。下方536及更早为历史。



## 536：逐 NAV 航点机头方向——限定静态审计

### 范围与证据

[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)为28源码／工程登记及32文档，共60路径；新增HeadingEncoding.h／cc／md。当前证据来自直接读取实际源码、实际消费者、现有Qt接口声明及本地PX4文本，不来自运行结果。只编辑源码／文档并做普通文本、路径、Markdown、Git空白核对；未执行编码器、算法、parser、QML、hash、项目、构建、测试、仿真或设备操作。FirmwarePX4只读且无可确认Git HEAD，不认定已安装固件身份。

### 交叉静读结论

- 原请求链：Draft对完整独立路线保连续heading；界面只为辅助出返程点编辑。原扫描方向不混用，缺省不补0；整点更新、正常revision+1、取消和换源保输入路径已逐段静读，未验证实际Qt交互。
- 共享规则：HeadingEncoding只接受有限合法请求；两个别名须量化前原值相同。最近tick、半格向上、末格圆周归0、全部上传tick精确Float32及回读界在独立合同中列推导。静读推导不能代替目标工具链数值验证或真实性能。
- 实际消费者：Compiler／Builder、上传桥、普通／RAW回读、Reconciliation、StartContentCheck、ArmedGroundReview及ProgressObservation使用同规则；传输原值不被规范代表替换。三个严格几何消费者和Joint审阅保持完整原始字段及来源对应，不因允许heading放宽其它字段。
- 语义与许可：单机默认仍v2，明确NAV为v3；实际目录当前记录与逐成员payload域进入审阅和Readiness首尾检查。不是升级历史声明、硬件／固件签名验证、当前实际机头认证、StartPermit或正式执行结果。
- 回读与界面：新Diff29键／摘要28键保最多64条heading观察，即使匹配也显示actualRaw；complete只指有界候选已计数。Qt null／undefined显示兼容仍要求原字段存在；有限坏期望可显示，只有semanticMatch才能信用合法tick相同。非法大有限Double保存原数但canonical拒绝；原普通scalar整数域不改。
- 物理边界：仅明确heading成员增加达向／转向动态未认证要求。exactGeometryPreserved只涵XYZ／E7，不包括yaw网格量化。未证明实时转弯轨迹、动态避碰、朝向达成或飞行精度。
- 工程登记：新增cc／h登记custom.pri；没有把它等同全部CMake或目标构建配置已接入。UI/UX仅静态指导原控件、标注和错误／恢复路径，无渲染结果。

实现者与独立审阅者分别覆盖了共享helper、Draft／Facade／Planner、8个编码／回读消费者、严格模型／声明门禁及三个显示消费者。审阅发现并已在本批修正：空值跨Qt表示不一致、坏有限期望被显示层误拒、非法大有限raw丢值，以及坐标精确性／回读用语易被误解。有限静读范围未见剩余确定问题，不等于不存在缺陷或已通过运行验收。

### 材料核对与后续

普通文本材料核对已完成：60路径均存在且清单逐项一致，无重复；48项编号连续且逐项标为未执行。32份文档共6740个本地Markdown目标均存在，未发现冲突标记或行末空白；tracked范围git diff --check无诊断，untracked范围逐文件NUL比较无空白诊断。后者exit1仅表示相对空文件有内容，不是测试失败或业务正确性证明。没有执行项目逻辑。所有48项工程场景见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)，全部未执行。原25目标及六类实质剩余不缩减，约40%源码／60%剩余仅粗估。未提交、更新PR或打包；下方535及更早为历史。



## 535：异机完整声明包络比较（源码限定静态收口）

### 范围与判断口径

本批范围为 [清单](MERIVUS_SWARM_PATCH_MANIFEST.md) 中15源码／工程登记及21文档，共36路径；新增 InterVehicleEnvelopeReview.h／cc／md。仅直接静读、源码编辑及普通文本材料核对，不执行项目、求解器、parser、helper、线程、QML、SQL、hash、编译、测试、仿真或设备动作。不能从静态检查推出算法运行正确、构建成功、真实飞行安全或工程验收完成。

### 已检查的实际消费链

- Worker 真实结果由 exact7 升为 exact8，原三报告之外必需完整异机报告；失败整体返回空报告，不保可用成功前缀。原同次原生 Model 对象、原来源、actual finished、取消和期限仍独立核验。
- 共同结论独立读取异机几何完整、比较完整、声明模型满意及零冲突，再保留原名义／整域／Waypoint和 actualPathCertified 条件，不以新模型覆盖旧门槛。
- 普通和地面 Review 保存完整原 artifact；地面 risk 原因同步新报告；Witness 和两 Authority 同样消费共同结论及原来源，不接受旧 exact7、空新报告或从 JSON 安装原生依据。
- 新结果不替代原 minimumSeparation，不使用计划时刻或地形 pad 豁免异机，不产生 StartPermit、飞行命令或实际路径认证。

### 完整几何算法与材料收口

已直接阅读全文新 Inter h／cc／合同及两个生产者完整新增导出段、消费者和两 Authority 修改段，并进行作者外独立静态交叉审阅。当前新增范围未发现确定未修复缺陷，不等于编译或运行验证通过。

- 原生封装只在成功构造时交付完整数组，非法／取消先清输出；matcher-only 不可作为实际计算来源。共同 Motion 指纹另核 Domain row2 与 Waypoint row3 的共同描述，原完整参数绑定保留。
- 按完整成员顺序把每机 Domain＋Waypoint 组件做两两笛卡尔积，覆盖 DD／DW／WD／WW 与 protected／support。原数组索引保留相同显示身份的 split cells，不吞后成员、同形异机或跨来源组合。
- 闭 Bounds 只作严格分离排除；需窄相交时保双方全部平面与 strict 属性后交现整数 feasible，超过支持域为 Unknown。接触面／边／点及零宽轴不按 epsilon 删除；环境接地豁免不导出。
- 新比较同一2M工作／2048片段预算覆盖完整输入、平面校验、枚举及求交；旧环境预算不变。qint64组合计数、只计明确完成的pair、最多512明细、真实总数与截断标记保持。未完成不能称 clear；报告 work 不低于已发生固定收费，resource原因只对应预算耗尽。
- 值匹配器不运行求解器，也不枚举全部潜在pair；只在最多512条明细上复核原组件、递增次序和已处理前缀。它不能认证报告结论，实际 Worker／原对象／来源链仍独立核验。

普通材料核对：36路径全部存在，15／21分组与清单逐项一致且无重复；48条工程场景序号连续并全部标为未执行。6549个本地 Markdown 目标（含历史段）无断链；无冲突标记或尾空白，git diff --check无诊断。范围内25个未跟踪文件另以NUL作空白核对，均无诊断；退出1仅表示有文件内容差异。上述只是文本材料检查，没有执行项目或几何算法。

[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)48项场景全部未执行；原25目标与六类实质剩余保持，约40%源码／60%剩余仅粗估。没有提交、更新PR或打包。下方534及更早为历史。

## 534：参数导入固定来源与重试（源码限定静态收口）

本批11源码／工程登记＋16文档=27路径，新合同2份。真实旧路径的初次发送经Fact与PM普通通知链，重试按当前Fact值编码，同名PARAM_VALUE可清旧waiting；本批实际参数编辑器改接独立原生事务，不以533 caller前后检查代替writer保护。普通单项编辑和旧流导入不冒称整体重写。

PM接纳私有typed候选、原PRIMARY weak控制块、实际Vehicle／PM／plugin／protocol／目录与值基线；保原GCS身份及channel。整批七型与APM精度预检通过后才接纳，不乐观改现场Fact。独立100ms timer，每轮最多扫描256／尝试64，每行至少3秒、首次后最多5次重试、总120秒；重试固定原logical值，普通读取timer不停止。同键普通写准确supersede，显式刷新、重载和源变更退休原run；取消保writer-entry账本，不称回滚。

实际Vehicle私有桥独立核payload与APM per-component numeric-float；大整数必须可精确表示。插件和Link虚调后的原来源、目标、类型、name及值再次吻合才记录entered并调用原writer。最早RAW在公共通知和plugin前冻结原run／行票据，正常Fact更新后才消费；旧回调中新run不能领取旧包。120秒在驱动、writer fence及取票／消费均核，同值报告不是因果ACK、配置保存或身份认证。

主代理、作者和独立审阅者已直接读取新增PM、Vehicle桥／RAW、APM查询、Controller实际接纳、QML及构建条件。三处确定缺口已修并读回：533来源误要求UDP／Mock Link与GUI同线程；while条件的连接虚调内取消后终态被覆盖；页面外调后寿命尾与停止按钮键盘／窄窗问题。最终另核慢调用后的重试间隔、sourceCurrent前后准确run、消费时APM模式仍一致。限定新增未发现另一确定缺陷，不等于全仓或运行正确性证明。

当前Controller文档已改为534原生接纳语义，旧533 setter／attempted语义只保留于历史审计。UI显示精确原run状态及两类未全匹配终态，确认只停原run；plain lifetime在关闭／构建／提交／取消外调后检查，按钮StrongFocus、主题焦点框及PlainText换行。UI/UX仅静态指导交互，没有运行技能搜索、QML或预览。

普通材料核对：27路径全部存在，11源码／16文档清单无漏项或重复；所选文档含历史6362个本地Markdown目标无断链，64工程行连续且全部未执行。无冲突标记，tracked git diff --check退出0；6个本范围untracked文件的NUL差异空白核对无诊断（exit1仅内容差异）。Vehicle h／cc与APM h／cc合计25处既有行尾空白保留，数量与HEAD相同，未顺带格式化；这些检查仅限材料，不是构建或业务验证。

未编译、执行项目／业务／parser／timer／QML／收发、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%源码／60%剩余仍仅粗估。下方533及更早为历史。


## 533：参数文件实际导入链（源码限定静态收口）

范围4源码／QML＋13文档=17路径，新文件仅ParameterEditorController.md。主代理、作者及非作者直接读取实际ParameterEditor载入→Controller预览→ParameterDiffDialog确认→Controller提交，并追踪FactMetaData复制、Fact setter、实际PM writer和等待／观察来源。旧readParametersFromStream仅声明／定义，不是本UI入口；APM机架CSV不属本文件格式。

确认旧路径会略过错误列、按未知类型落Int32、忽略已有Fact与文件类型冲突；临时Fact借用live metadata会切换重启提示标志，enum格式化可把未知枚举加回实际定义，失败转换可能留下默认0。新路径限4MiB／64KiB块读，复用现完整五列／十型codec，全部候选与基线通过才一次发布；独立Fact和深metadata副本承担所有格式化，临时对象有准确清理。不用文件system ID修改当前飞机身份。

私有预览冻结实际Vehicle／PM／active机／PRIMARY弱owner／版本值与epoch、目录revision、实际Fact-owned metadata及原值；原缺失参数与后来删除Fact分开。公开行只读取load选择，不作为提交值或来源。正常单行PM invalidate不改变全表epoch，已静读确认多行不会因自己的普通写而自锁；刷新／换源仍退休原资格，不重捕新epoch续权。

实际PARAM_SET旧发送器只正确编码MAV 1..6、9，不是文件十型。选中Uint64／Int64／Double在任何写调用前整批拒绝，未选中者可保预览。已知且确有差异行使用原forceSetRawValue以保显式负零，缺失参数保原typed worker路径；全选择先核、逐项前后核、最后整批已尝试已知值及通知尾再核。预检失败仍可调整选择，实际尝试开始即一次消费；attempted是调用计数，不是wire／ACK／原子完成。

QML创建时复制准确token；旧页不再显示后来模型，确认、单项及全选均限定原预览，全选逐项通知后复核，关闭只清原token。载入失败现明确显示原因，过时页面保Cancel及重新载入提示。UI/UX技能只静态指导现Qt控件的错误反馈、状态清晰和恢复路径，没有执行技能脚本、QML或预览。独审发现最终diffErrorChanged后遗漏writeAllowed；已在最终检查中补写门和其后source复核，读回核实，不掩盖先前可能提交事实。

同一行旧通用sender内部以及后续重试尚无原导入sourceFence，本批未改PM／Vehicle发送器。PARAM_SET没有本次导入nonce；即使后续出现同值回包，也不能仅由同名或pending归零称本次因果ACK。下一步须原生导入事务、冻结payload和原PRIMARY重试、实际writer入口事实及同源匹配报告；不能把caller前后检查写成最终发送认证。APM合法numeric-float转换也必须保留，不照搬固定HASH ACK的字节不变规则。

普通材料核对：17路径全部存在，4源码／13文档清单无漏项或重复；所选文档含历史6277个本地Markdown目标无断链，48工程行连续且全部未执行。本范围无冲突标记或行尾空白，tracked git diff --check退出0；3个本范围untracked文件的NUL差异空白核对无诊断（exit1仅内容差异）。这些仅文本／路径核对。

此次只有源码、直接静读和普通文本／路径／Markdown／空白核对，没有编译、运行项目／parser／helper／QML／业务IO、测试、仿真、硬件／PX4操作、刷机、放行、提交、PR更新或打包。源码静态收口不等于运行或全仓可靠性验收；原25目标及六类实质剩余保持，约40%源码／60%剩余仍仅粗估。下方532及更早为历史。


## 532：元数据后台并发与排队（源码限定静态收口）

范围10源码／工程登记＋17文档=27路径，新增scheduler三文件。直接读取两个worker的实际构造、取消、结果、finished和输入所有权，以及Manager／Translation的四个原start入口。原每机各自QThread没有跨机预算，两个失源／早退清理路径仍cancel后start；本批统一受理并移除旁路。

新调度器仅在实际app主线程建立，Artifact、Summary、Translate共用2个运行槽／32个FIFO排队项。100ms巡检，排队从原受理开始120秒，派发前重核取消和elapsed；每轮扫描／派发最多32项。准入先入队后返回，不同步执行任务；一次worker只能尝试schedule一次。正常派发只有一个start调用点。

运行槽依据worker构造时先连接的actual finished原子记录归还；该DirectConnection仅写shared atomic，不读GUI或worker成员。resultReady、取消、源退休和QObject销毁不归还运行槽；未有finished而对象消失时停止新受理。槽与QObject回收分开，原结果／finished消费后deleteLater关系保持。此范围是实际run并发，不是所有OS线程／TLS结束或全进程内存上限。

未受理false由caller清准确未启动对象；已受理queued取消／超期先移除原队列项再发独立neverStarted。Manager清原worker等待后退休准确可选metadata批次，沿既有InitialConnect可选步骤终结，不经_finishArtifact、缓存失效或备用URI。Translation清原worker并独立失败终结，释放准确busy；正常运行保原双尾。没有假result／finished、cancel后start、提前删除运行线程或以排队拥塞删除好缓存。

主代理、作者及非作者已直接复核新scheduler全h/cc/md、两worker新增h／构造／schedule与run寿命链、两个消费者新增及实际退休／终态链与合同。最后admit改为持finished原子记录的值副本，并在singleton受控parenting通知后重核实际worker；已读回。当前限定新增未见另一确定剩余缺陷；源码实现及限定静态交叉审阅完成，不代表编译、运行或全仓可靠性验收。

普通材料核对：27路径全部存在，10源码／17文档清单无漏项或重复；所选文档含历史6247个本地Markdown目标无断链，40工程行连续且全部未执行。本范围无冲突标记或行尾空白，tracked git diff --check退出0；13个本范围untracked文件的NUL差异空白核对无诊断（exit1仅内容差异）。这些仅文本／路径核对。

本轮不执行项目、线程、parser、helper、下载、文件业务IO、编译、测试、仿真或PX4／硬件操作；没有提交、PR更新或打包。HTTP／FTP、其它线程、显式同步翻译和整个进程资源不在本调度预算内；原25目标及六类实质剩余继续，约40%源码／60%剩余仅粗估。下方531及更早为历史。


## 531：参数缓存 v3 与真实 HASH 消费（源码限定静态收口）

本批7源码／工程登记＋15文档=22路径，新增3条。直接读取原缓存读写、RAW HASH入口、等待表与完成通知、Fact真实存储和元数据所有权链。旧QDataStream直接恢复QMap/QVariant无完整资源和EOF检查，CRC／debug比较按声明宽度读取variant存储，均由源码确认；未运行复现。

新ParameterCacheFile为独立纯值格式：4MiB／16384行、20字节显式头、身份声明、排序唯一ASCII名称、十种数值primitive LE和完整EOF。decode后重编码核完整表示；真实metatype、宽度和非null先于primitive复制，保IEEE NaN payload、Inf及正负零。Fact容器的实际值与通知尾改按真实浮点位比较，避免NaN自不等中断自身通知；setRawValue/force和参数写回信号不扩展。

实际PX4参数实现、生成排序及MAVLink参数流只读核对：HASH覆盖used且非volatile参数名（无NUL）和Int32／Float原4字节，初值0／无末xor，原count仍包括volatile。当前6C NuttX配置为little endian；不推广为所有固件或硬件认证。十型文件不等于十型PX4 CRC支持。旧v2保留且不解析，坏v3整份miss后走正常参数流。

主代理与独立审阅者已读codec三文件、Fact窄修、工程登记及PM新增来源／读写／消费／完成链。缓存绑定原PRIMARY实际对象及weak owner、早期native QObject pins、原版本与目录revision、独立非回绕activity serial；真实接收／刷新／写入退休旧尝试。新Fact及owned metadata跨工厂和通知后复核，实际writer用QSaveFile关闭direct fallback、完整块和提交检查，原动画删除。

静读确认三项交互缺口并已修补：一是非volatile CRC和相同count不证明volatile名称／used-index不变，因此任volatile仅预填非volatile模型，不清真实等待或发送HASH ACK；二是precheck可processEvents，缓存initialComplete延后到precheck返回且全表仍有效，退休旧尝试不能阻断后来真实流正常完成；三是逐行安装失败前不得先清索引。最终所有cache行只改模型，整表实际值／QQ／metadata／来源均核过后，零volatile才在无外调区一次信用本组件合法索引与候选names，并保其他组件同号queue和外国等待。真实进度、precheck、ready及原固定HASH ACK桥各尾再核，已发布或已进入writer的事实不伪造回滚。

作者、主代理和非作者已读回最终后置信用delta及合同；限定当前新增未见另一确定缺陷。第一connectNotify内另一尚未监听信号的隐藏ABA、通用factory内部寿命、更早Vehicle入站来源和全机队后台调度未由本批证明；有界同步缓存也不代表GUI时延或峰值内存验收。

普通材料核对：22路径全部存在，7源码／15文档清单无漏项或重复；所选15文档含历史6158个本地Markdown目标无断链，64工程行连续且全部未执行，冲突标记无。Fact.cc原6处行尾空白与HEAD计数相同；本范围tracked git diff --check退出0，7个untracked文件的NUL差异空白核对均无诊断（exit1仅内容差异）。仅普通文本／路径／Markdown／空白检查，不是语法、构建或业务验证。

64工程场景全部未执行；未编译、运行项目／codec／parser／CRC／helper／参数／业务文件IO、测试、仿真、硬件／PX4修改、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仅为粗估。下方530及更早为历史。


## 530：离线参数实际消费（源码限定静态收口）

本批7源码／工程登记＋15文档=22路径，新增5条。主代理直接读取原 ParameterManager 构造／offline loader、writer及十型双向映射、Fact存储与容器更新、metadata实际owner链、Vehicle离线默认组件setter、四份qrc资源及别名。旧loader的paramValue从未安装，Int8使用toUInt，逐行插live map且打开失败仍可ready，均为实际源码缺陷，不是运行复现。

新增纯值codec完整候选：4MiB／4096字节行正文／65536行／16384项／128字节数字，精确五列、同source、组件域和名称唯一；十数型canonical、整数先限幅、C locale有限实数和非零下溢拒、正负零分开。Plane实际注释NUL先跳过；不猜默认组件或改变实际Vehicle id。作者与非作者已全文静读codec3与Fact窄修；只说明当前新增限定范围未见另一确定缺陷，不代表语法、构建或数值／运行正确性证明。

Fact容器更新按userType和值及浮点零signbit核实际存储，不能以跨型数值相等跳赋值；cooked转换及两通知尾同核actual Fact。仍不发_containerRawValueChanged，不改变set／force入口，不把valueChanged当参数写入或遥测证据。

ParameterManager消费者由作者、主代理和非作者完整读新增helper／loader／h私有状态与合同。pristine构造入口先冻原Vehicle／plugin／实际组件Manager／offline／模型／默认组件／观察和原表状态，provider第一次合格生成后冻目录revision；受控parent后批量安装实际typed值及Fact-owned元数据，后行之后再次复核全部对象和目录。无外调commit同时安装map／counts／total／默认组件及ready，不调用在线完成或写入。失败清准确新候选，不删除外国parent或回滚他人状态。

独立静读发现版本epoch并不覆盖公开setFirmwareVersion：原setter直接赋major/minor/patch/type而APM实际按versionCompare选XML。已补实际七版本值与epoch逐尾并行；另以共享atomic本次变化标志和两个准确 scoped DirectConnection，拒两监听就位后插件／metadata／值通知内A→B→A。每次connect之前已冻原来源，之后立即guard和current尾核；不捕栈引用，退出断开不回滚已commit。第一connectNotify内另一尚未监听信号的隐蔽ABA不由这条本地监听证明，MD和工程44明确保此边界，不称全局setter代次认证。原factory内部parented构造／删除风险亦不由caller尾核认证，仍可继续源码审查。

普通材料核对：22路径全部存在，准确7源码／15文档清单无漏项或重复；所选15文档含历史6082个本地Markdown目标无断链，48工程行连续且全部未执行，冲突标记无。Fact.cc原6处行尾空白与HEAD计数相同，不擅改历史；本范围tracked git diff --check退出0。7个本范围untracked文件的NUL差异空白核对均无诊断，exit1仅内容差异。此仅普通文本／路径／Markdown／空白核对，非C++语法解析、项目运行或业务验证。

限定新增静读未见另一确定问题，源码实现及限定静态交叉审阅已收口；不是全仓、qmake／CMake／moc、数值／性能／运行或可靠性验收。

48工程项全部未执行；未编译、执行项目／parser／helper／参数／Fact／metadata／文件业务IO、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。缓存v3与全机队调度仍有具体源码工作，不作为已实现或仅待测试。原25目标及六类实质剩余保持，约40%源码／60%剩余仍仅粗估。下方529及更早为历史。


## 529：两段后台翻译（源码限定静态收口）

本批9源码／工程登记＋16文档=25路径，新增6条。主代理直接读取旧Translation全h/cc、真实Manager／528输入、QGC TranslationTest JSON/TS、本地PX4 parameter.jsonout与events翻译声明，以及新增Engine／Worker／协调器全源码和合同；不是执行fixture或源代码语法解析。旧链在GUI整份读取summary、解压TS、XML／JSON递归及写译文，本批实际改为Summary worker→GUI唯一下载→Translate worker，旧显式sync接口不冒称异步。

作者和非作者独立全文复读Engine3、Worker／协调器6及root Manager／登记。现实际生产用caller UUID、准确两artifact和原producer，只听native独立3String信号；source退先inactive，starting栈迟取消即使self已死也只撤原请求。worker自己持原summary／metadata与TS lease，result和actual finished同原worker／phase／generation双尾后才取结果；未take输出由worker清，拒绝后只清准确本次输出。取消排空期间不清busy或删除运行线程，不借旧信号领养外国译文。

真实fixture的keyed／unkeyed list、字符串数组、wildcard、global<>转义和三层recursive-def已逐路径静推；不是测试通过。静态先核全部规则／ref存在但不沿引用图误拒自环，执行以actual(ruleID,physicalNodeID) active检测无数据进展，不能用重复list.key的显示前缀作身份。numeric index／value改用明确Qt JSON十进制，不恢复Float原始词法；root ref统一rule语义修旧不一致；未知规则操作拒，TS单message／非复数与无子集DOCTYPE域明确，不冒称全TS或schema兼容。

文件32MiB／64KiB、JSON64层／262144节点／16MiUTF16、XML64层／524288tokens／65536contexts／8MiUTF16、解释256层／2Mwork／16384路径及替换16MiUTF16分别核；PI target/data和属性名／值亦计费，输出树累计字符再核。解压32/32/64MiB沿原bounded helper，单worker最早clock120秒不重置，取消／期限不物理抢占Qt parser、序列化或阻塞IO。rebuild原会保每层完整子树中间值，已在父insert／append后清单parent的child值并计work；反向拓扑保证后续只需完整parent，非作者读回末delta未见确定缺陷。这只减少副本，不证明全机队线程池、峰值内存、总临时磁盘／CPU或全部GUI时延上界。

另核对[Qt5.15线程文档](https://doc.qt.io/archives/qt-5.15/qthread.html#finished)及[Qt5.15.2共同实现](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/thread/qthread.cpp)、[Windows实现](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/thread/qthread_win.cpp)、[Unix实现](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/thread/qthread_unix.cpp)：怀疑finished时isRunning门误拒的推测被源码否定，finishing位在发signal前建立，getter语义纳入该位；无需据猜测删门。此仅官方源码对照，不是调用本机SDK，不把finished夸成所有TLS／OS join或wait等价。

普通材料核对：25路径全部存在，准确清单9源码／16文档无漏项或重复；所选16文档含历史6018个本地Markdown目标无断链，冲突标记和本范围行尾空白无；48工程行连续且全部未执行。tracked git diff --check退出0；10个本范围untracked文件的NUL差异空白核对均无诊断，exit1只是内容不同。此检查仅普通文本／路径／Markdown／空白，不执行项目、C++语法解析或业务实现。

限定新增静读未见另一确定问题，不是编译、运行、性能、可靠性或全仓验收。未执行XML／JSON／规则／线程／helper／parser／下载／文件业务IO、编译、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持，约40%源码／60%剩余仍仅粗估。下方528及更早为历史。


## 528：独立下载原件（源码限定静态收口）

本批15源码／工程登记＋19文档=34路径，新增6条。旧HTTP与MFTP按remote basename写公共临时文件，worker后续快照不能追溯保证下载原件未被另一车覆盖；TS即使译后输出独有，原TS输入同样存在碰撞。本批从实际下载开始使用fresh原生owner，新API与legacy入口／信号分别保留，不靠下载后复制共享basename补救。

主代理直接读取共同owner、HTTP／Cached四源码和两合同、FTP完整h／cc／md、Manager真实接线、worker输入成员、Translation处理栈及两个工程登记。非作者另独立全文复读HTTP／Cached／FTP及root生命周期链。限定当前新增静读未见另一确定缺陷，不是语法／编译／协议／性能验证或全仓可靠性验收。

独立复读发现受理栈内Manager先析构、_retire因starting跳过取消而返回尾又因self空漏cancel；已用原local actualprovider/id在真实true返回后兜底，并读回同类Translation尾。另发现parented QTimer构造的ChildAdded通知可删除parent／timer后继续裸访问；已改parentless候选、QQ及仅局部清理，setParent／配置／connect／start后均核原job和actualtimer，未受理guard关闭原writer并清准确槽，修补后直接读回。

真实producer／consumer核对：所有accepted终态都回原id/path且先关writer，false不留下自身活动作业或发布自身成功；Cached原job只一次AlwaysCache且原writer关闭后清零，120秒外层总期不因fallback续。HTTP累计包括redirect body32MiB、8次redirect及实体／声明长度域分开。FTP unique独立component1..255前置解析、包长／64bit offset+size／seek／完整write与最终flush/close/size、模16 sequence wrap、4096缺块预算、零进展数据ACK拒及单调120秒；legacy不施加这些新域限制。原packet、实际request／phase／sendSerial与QObject／Link释放尾已直接追读。

worker自身const shared原输入覆盖Manager连接断开后的真实run；cache空owner不领养。原summary／metadata artifacts、TS原件、解压输出与译文各自保寿命。唯一文件和本地ID不是TLS、远端wire来源、固件／构型／性能或StartPermit认证；single-transfer原生契约不防任意违规C++跨实例复用，也不认证恶意本地路径替换、崩溃耐久或完整临时磁盘／CPU／网络内存上界。

普通材料核对：34路径全部存在；清单准确15源码／19文档，无漏项或重复；所选19文档（含历史）5951个本地Markdown目标无断链，冲突标记无。64项工程行连续且全部未执行。原行尾空白计数与HEAD相同：QGCFileDownload.h 2、.cc 4，QGCCachedFileDownload.h 1、FTPManager.h 7、Translation.h 1；不擅改历史。tracked git diff --check退出0；13个本范围untracked文件的NUL差异空白核对均无诊断，exit1仅内容差异。初次材料检查发现本批7个入口新空白行含空格，已仅清本次空格后复核通过。上述仅普通文本／路径／Markdown／空白，不执行项目／源代码语法解析或业务实现。

64项工程场景全部未执行；未编译、执行项目／parser／helper／业务／CRC／hash／文件业务IO／下载／缓存、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%源码／60%剩余仅粗估。下方527及更早是历史。


## 527：参数定义实际消费者（源码限定静态收口）

本批12源码／工程登记＋13文档=25路径，新路径4条；源码实现与限定静态交叉审阅已收口，不代表运行版本或全仓可靠性验收。

主代理已直接读取CompInfoParam、FactMetaData实际createFromJsonObject／解析器／数值setter、ParameterManager三个新Fact与CRC路径、Fact::setMetaData原raw存储及本地PX4 jsonout；非作者另独立追了相同生成/所有权与初连来源。确定原问题：校验前_noJsonMetadata=false、factory失败仍非空fallback、逐行发布坏尾保前缀、旧map/list未退休、Fact借用raw不copy。不能用qDeleteAll清旧map造成悬空来假修半装。

本机实际generator写version1，七数型中只有Int32／Float主动转数，其余可保数字String，values与bitmask可以同条存在。当前上游[参数schema](https://raw.githubusercontent.com/mavlink/mavlink/master/component_metadata/parameter.schema.json)与[元数据协议](https://mavlink.io/en/services/component_metadata.html)只作参考：它们的更新版本不等于本机文件已迁移。本批明确保实际v1，不静默用其它版本缺类型默认或把本地严格解析说成完整JSON Schema draft验证。

新入口只给参数消费者，原通用factory其它caller不变。整份纯值候选校类型/实际型域/最终区间/default/完整枚举和安全位移后才造模板；数字String兼容、未知default、资源预算均须按新合同直接审阅，不因非空QObject推schema成功。actual Fact-owned copy替换三个raw借用点；CRC按实际component已知volatile值，未知不能当false。Manager在准确source退及新GENERAL前清所有实际组件lookup资格；已有Fact保独立旧快照，不伪称热更新或当前构型重认。

原XML fallback为现有本repo有效新建parentless QObject转移契约，实际provider/owner/source与借用栈需保；不能仅按parentless认证插件，也不防任意违规第三方C++返回已死raw。旧XML解析及wantedMajorVersion1不是机载真实版本／完整schema或能力证据。本批不改飞控，不授予参数写、任务执行或StartPermit。

普通材料核对结果：25路径全部存在，准确清单12源码／13文档且无漏项或重复；所选13文档（含历史）5865个本地Markdown目标无断链。工程师表58行连续、全部未执行；冲突标记无。FirmwarePlugin.h2处、Vehicle.h9处及Vehicle.cc13处原行尾空白均与HEAD计数相同，未擅改历史；本范围tracked git diff --check退出0。7个scoped untracked文件作NUL内容差异空白核对均无诊断，exit1仅有内容差异。该检查仅普通文本／路径／Markdown／空白，不执行项目、源代码语法解析、CRC／hash、参数构造或文件业务逻辑。

58项工程场景全部未执行；未编译、执行项目／parser／helper／业务／CRC／hash／文件业务IO／下载／缓存、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标／六类实质剩余及约40%源码／60%剩余粗估保持。

## 526：组件元数据字节接入（源码限定静态收口）

范围18源码／工程登记与15文档，共33路径，新路径7条；源码实现及限定静态交叉审阅已收口。此节不是编译、运行、字节CRC／文件IO／解压验证或全仓可靠性认证。

已直接读取原Manager／Cache／CompInfo／StateMachine及当前配套PX4元数据生成脚本和streams：原流程声明CRC只作标签，未对实际字节比较；旧insert可能返回已有路径。新路径改取raw-v1原始缓存候选，每次经实际worker，当前消费者只用独立JSON，不拿旧返回值回填为本次已核内容。

协议参考仅用于设计：[组件元数据协议](https://mavlink.io/en/services/component_metadata.html)、[MAVLink CRC算法](https://mavlink.io/en/guide/crc.html)。现算法初态零、无终异或；本地CMake为general传未压缩JSON而URI指xz，子元数据传xz。因此只在实际PX4 GENERAL显式允许decoded-px4-general兼容，不将其标为transport比较，也不对全部文件猜CRC域。

后台raw／JSON各32MiB、字典64MiB兼容本机preset9、最多三份唯一临时文件；完整EOF、负read／短write／flush／close、CRC域、SHA及取消分别核对，实际结果和线程结束双尾才交原Manager。原raw／JSON不移除；可转交第三cache candidate与两固定快照分开，三文件最多96MiB临时磁盘不等于总解码内存64MiB。旧两参数inflate接受域不变，本批新增有界入口；译后JSON不是原CRC核过字节。

Manager／InitialConnect原QObject寿命、来源／PRIMARY及阶段、缓存失败一次准确invalidate／一次原URI下载均纳入直接审阅。Vehicle三个关闭入口第一句先退休本通道，sourceCurrent与所有外部terminal回调均核纯native早关门；不使用initialConnectComplete，正常初连不依赖自身已完成。source退或无来源时只一次结束可选步骤，不冒称文件成功；缺worker任一尾不推坏缓存／重试。先退旧槽再通知，实际worker parentless排空，不用共享FTP.cancel清他人。

General新batch清旧目录，version1、≤128条、唯一非负type、真实uint32CRC及有界String候选整份接纳；坏文件不传播旧catalog或成功前缀。参数工厂完整schema／半装缺口仍待后续，文件字节成功不提升为参数定义、真实性能或构型证明。

翻译TS／JSON输出改每次唯一，完整读取／写入与32／32MiB、字典64MiB边界；本对象busy、原下载器／URL／非回绕代次、先清槽再emit及实际唯一译文RAII接管均在审阅范围。较早接收者销毁parent时仅局部orphan guard清其本次输出；不在emit后读this或清新请求。底层共享下载basename、原TS/JSON递归图深度／CPU／总临时内存尚未解决，不能宣称全链路输入隔离、严格完整图预算、TLS或远端身份认证。

作者与非作者直接全文阅读覆盖Manager／Worker／Cache／General／Translation合同及新实现；Vehicle与InitialConnect只对本批新增接线作逐处核对，不宣称全文全仓认证。交叉审阅实际修正两处归属窗口：三个Vehicle关闭入口首句退休且sourceCurrent亦立即拒续作；每文件最多两个worker且三种终结闭包冻结原worker generation，旧deleteLater的destroyed不再读取新重试已重置的flags。其它修正包括固定MAV URI有界NUL／真实HTTP分派前缀、同步受理buffer、第三raw候选、译文跨通知RAII与parent先销毁orphan清理。两位独立审阅者已直接读回末delta，限定新增链未见另一确定缺陷；这是源代码推理，不是运行结果。

普通材料核对：33路径全部存在；所选15文档（含历史）5829个本地Markdown目标无断链；本批清单33条恰好覆盖18源码／15文档且无重复；工程师表48行连续、全部标未执行。未发现冲突标记；Vehicle.h9处、Vehicle.cc13处与Translation.h1处行尾空白均与原HEAD计数相同，未擅自清理历史。累计tracked范围git diff --check退出0；本范围9个untracked文件作NUL文本差异空白核对均无诊断，exit1仅表示有内容差异。该检查不执行项目、语法解析、CRC／hash或业务IO，也不能证明构建兼容。

48项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／CRC／hash／解压／下载／缓存实现、测试、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包，没有实际业务缓存／归档IO。原25目标与六类实质剩余保持，约40%源码／60%剩余仅工作量粗估。
## 525：单个退休原件永久清理源码静态审计

范围为10源码／资源与15文档，共25路径，新路径仅PurgePanel.qml及合同两条。作者、主代理与非作者已直接静读本批原生来源／共享槽／双终结、Worker完整新请求与结果、Root窄门面、UI及登记受影响链；限定源码合同静态收口，不是整仓无缺陷、构建／测试通过或原生API／删除行为的证据。

依据来自原实际backup worker的完整backup31及backupVerified/source.retired、resultReady与actual finished双尾；新增私有basis和原preview，不接受QML文件Map或公开last重建。预览只读；提交重新核当前完整源／副本、SQL／SHA、原目录及实际Windows file IDs，消耗30秒原预览。原件和副本实际ID相同拒绝，ID不同也不认证独立介质、来源或永久身份。

精确删除只在原已核源handle请求FileDispositionInfo，不按路径remove、递归或跨平台fallback；两handle关闭、原件实际缺失、备份后核分开表达。删除决定在短锁中提交，原生IO不持GUI会读取的状态互斥锁；决定后取消不伪造回滚。先以freshUUID／NewOnly创建意图收据，完整行write＋flush前不删除；最多两行、各64KiB、总128KiB，失败partial保留，终态收据失败不能抹已发生删除事实。flush／close不是掉电耐久证明，sourceLogicalBytes不当物理空间释放量，diskSpaceReclaimedVerified保持false。

Microsoft原生API参考仅用于源码设计，未调用这些实现：[SetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle)、[CreateFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew)、[GetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle)。删除需要DELETE访问；分享约束双向，不能假设DELETE handle下Qt／SQLite重开兼容；file index只用于当前本地对象对应。实际SDK／文件系统／分享／close及错误语义均待工程师验证。

新合同保原旧维护域不变：Worker preview请求3／commit4及proof3、result41／current24；原生state19／preview18／result13／receipt11。新job、nonce、非回绕serial与实际worker配对，双尾后才交付；取消、失源或收据错误不丢合法实际删除事实。原实际备份双尾若嵌套早于旧入口guard释放，只保一次原冻结续接，原栈释放后才安装basis，不轮询、重试或借公开last安装。

交叉审阅关闭确定缺陷：两个备份名接受域改256，容纳实际137字符而不截断；按钮显示门禁移除无通知plain票依赖，真正入口仍持票／retiring围栏，防正常检查后确认自锁；清字段通知前冻结detached原owner／原ID闭包，finally保留嵌套隐藏／销毁的sticky取消与作废责任。提交的本页收据路径只随原preview冻结，不从后来state拼旧路径；共享上次诊断仅显示原文件名。焦点使用实际Flickable contentItem／contentY／contentHeight有界滚动，取消、作废及反馈亦同；长危险按钮使用PlainText换行，不依赖不存在的ensureVisible。

Root typed五字段plain gate及同一archive.retiredPurge别名保持原Loader链，不新增控制器或后端服务。UI使用原Qt主题、来源标识、两项独立风险确认和显式键盘反馈；技能仅静态应用，没有执行搜索、预览或交互验证。64工程场景全部未执行。完整原25目标及六类实质剩余保持，不把单段显式清理缩成自动保留策略或长期运营全集完成；未实际读写／删除业务档案、编译、执行业务／parser／helper／QML／JS／SQL／hash／原生API、测试、仿真、硬件、PX4、刷机、放行、提交、PR更新或打包。

### 525 材料核对（非项目执行）

本批25路径均存在，10源码／资源＋15文档与清单25条逐项相等，无重复；仅两条为本批新路径，14条范围内untracked包含既有累计工作，不能把它们全算本批新增。15份文档含历史正文共5828个本地Markdown目标无缺失；64条工程场景1–64连续且全部未执行。无冲突标记或行尾空白；tracked范围git diff --check退出0，14个范围内untracked逐一NUL no-index空白检查无诊断，退出1仅表示非空文件相对NUL有内容。这些只检查普通材料，不检查C++／QML语法、Qt链接、业务执行、文件系统竞争、Windows句柄、真实交互、耐久或飞行安全。

## 524：已闭旧部署归档、原生退休及实际销毁回收静态审计

限定范围为19源码／资源登记及20文档，共39路径，新路径6条。沿原Lifecycle、Episode、Orchestrator、StartupTransaction和Run观察生命周期实现；worker及Panel通过既有工程／资源登记接入，不新增公开QML后端服务、wire、外部依赖、SQL schema、权限或PX4修改。本节仅记录直接源码审阅与材料核对，不是构建、运行、整仓安全或飞行可靠性认证。

live容量固定8，失败保留组与等待销毁组仍占槽；终身attempt独立uint64只增不回绕，回收不复用旧对象、UUID或批准。首次实际分配mutation前固定真实shared5，包括后来失败候选，文件不能pin来源。只允许非current／非所选观察的真实旧组，完整14服务、初始及最多8个ground事务／Run、真正静默和实际Swarm整张owner表均须核对；target子集外原owner占用也拒，不释放预约腾位。

有准备／began的真实观察须原独立Close提交sticky事实，明确never-began原unissued关闭是窄例外；不从空map、空taskId或0report猜未使用。began未issued分支保原观察与人工Close，不再自动退休造成关闭与归档均不可达。未注册TaskRecord的真实闭Run完整保在quiet，closed-observations按quiet.runs非空判断；完整pristine未发布失败组可0报告。

worker只持纯值副本，复用protected8完整codec，构造document15／quiet18及0..9报告；整包32MiB、quiet512KiB、单报告2MiB及树预算不扩。根与报告、record及Run对应、重复和未知字段整份检查；uint64保字符串，null／缺失／false／0分开。原full9／protected8接受域不放宽，提取纯解码未引入当前时钟、账号或live owner。原job／nonce／非回绕serial及actual worker身份配对，resultReady与finished双尾后才接纳。

共享IO保本地路径、末级symlink／普通文件检查、64KiB读写、QSaveFile禁direct fallback及回读。写入已知／未知、commit、回读、fsync和durable分别表达；commit后取消／回读失败保已写但不退休。不认证祖先reparse、恶意并发或失电持久性；没有实际读写业务归档。

实际回读缓存的完整原输入、quiet和所有原record与当前native再全值核对，前后再核来源、取消及全部原对象。专用native批次只移除准确已闭受保护记录及其语义缓存；普通remove仍拒，排队历史事件和他人占用不抹。退休后须原Episode及最多256全部原后代QPointer均空，后续turn才回收live槽。父destroyed、deleteLater或仅14服务空均不足；重parent仍存活后代继续占槽，不超时强回收。原lost监听按准确旧组隔离，不污染后来current。

Root仅公开typed九字段请求、state17及result15，plain gate覆盖调用者getter与通知，不泄漏Episode／账号／raw scope。UI明确选组、路径返回后另同意风险及覆盖；不自动选首条、重试或读报告，state/result分绑定等待同job同action。清理持ticket／retiring fence；按压、确认及来源ABA不复活旧意图。未认领job即使文件声称同archiveId也须撤旧history／reportView，ticket挡住的变化须随后处理；不拼旧summary与新报告。原生上次结果只读诊断，不恢复pending／路径／确认／文件槽或权限。

独立文件open不依live飞机／账号，不恢复原对象、观察或批准。cache实际bytes与完整document先核后读单份canonical文本；GUI不整包parse／stringify，仅2048UTF16显式分页保代理对。文件UTC仅文档声称，hash仅内容对应。UI/UX技能只指导原Qt主题、来源／确认层级、文字反馈和键盘操作，未运行搜索、预览或交互验收。

新增native、worker／codec与UI分别有作者直接静读及非作者交叉审阅，覆盖本批新增／受影响链段而非全仓认证；“未见确定剩余缺陷”不等于测试通过。56项工程场景全部未执行。未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、PR更新或打包。原25目标／六类实质剩余保持，约40%源码／60%剩余只是工作量粗估；不是最终StartPermit、可信远端正式结果或长期运营全集完成。

### 524 材料核对（非项目执行）

按本批39路径直接检查文件存在、普通文本、Markdown目标及清单：19源码／资源＋20文档全部存在；20文档含历史正文共5858个本地链接目标，未发现缺失目标。56条工程场景按1–56连续编号且全为未执行；补丁清单39条与范围逐项相等，无重复。未发现冲突标记或行尾空白；tracked范围git diff --check退出0，25个范围内既有／本批untracked路径逐一NUL no-index空白检查无诊断（退出1仅表示非空文件与NUL不同）。这不检查C++／QML语法、Qt链接、线程行为、业务结果、文件IO或飞行安全，不能当编译或测试通过。

## 523：GNSS 历史集合、独立文件工作区与来源地图静态审计

范围限定18源码／资源登记及18文档，共36路径；新增8路径为共享文件IO h／cc／md、GNSS一次性worker h／cc／md和Panel.qml／合同md。原FleetOperationalMemory、Archive、Root和两个History宿主继续使用；没有第二运营服务、公开QML后端、wire、外部依赖、数据库schema、权限或PX4改动。本节不是全仓认证或构建／运行通过声明。

真实追加只收actual Archive及本次原生GNSS token，从native-only getter取完整33报告并再次同源核对；GUI／owner先行，非GUI固定空拒绝不读私态。完整33／identity7／policy3／row18共享纯值校验兼容原生整数与JSON有限整值Double，sourceSequence保正uint64字符串；footerPresent与partialTail不得同时true。值自洽不能认证作者、文件、设备、位置精度、任务归属或飞行状态。

Memory独立private集合最多32报告／2048样本，同run／header／页范围或原GPS sequence重复整份拒绝，不以增长会变的fileHash、query或token逃过去重。容量满不trim或淘汰来源依据；旧宽松观察、KPI、热区、maxObservations、clear及schema1导入／导出不读写新集合。完整追加及非回绕revision先提交再通知；通知内换源／销毁不把已接纳事实变为未提交。初始epoch、revision1和stage idle与界面合同对应。

文件root独立exact11、4MiB整份校验，空集合也须正revision。显式freeze只复制真实typed集合值；worker持原不可变bytes，后续append不改旧快照。open只占独立file槽，不import active、不复活原Archive页或权限；descriptor17明确session与file，file来源epoch空／revision0仅标不认证当前来源。contentHash对应实际字节，sourceFileHash在file同实际字节hash；不拿规范化hash代替源文件身份。

单槽原job／nonce／非回绕serial／actual worker identity，resultReady和实际finished双终结后才接纳；整份文件JSON解析／序列化、hash及读写留在worker，完整集合不公开给GUI，界面仅核有界报告副本。取消不能提前复用槽；aboutToQuit／析构先关业务及退selected，再原子cancel，不wait、terminate或删除运行线程。cache原生私有exact3；每次缓存动作在worker重验实际bytes和完整cache，不借活动来源补缺失。

共享FleetSnapshotFileIo由原维护路径／分块读写提取，维护仍32MiB与原错误域、save11，GNSS为4MiB。保末级普通文件／symlink检查、64KiB读写、UTF8及QSaveFile禁direct-write fallback；不认证祖先reparse、恶意TOCTOU或失电durability。未尝试、已尝试、commit不明、已commit与回读分别表达；commit成功后取消／回读失败仍保已写，不假称回滚，fsync／durable固定false。此次只修改实现，没有实际业务文件读写。

Panel沿原QGC主题及UI/UX静态通用指导，追加／freeze／路径返回后的保存或打开各独立确认；不自动追加、读首项、续页或选图。目录每页8、report一次最多64；本页pending只认原job/action以及selectedReport／token／binding5和实际目录内容，等通知各值就绪才采用，不能用共享latest或任意Map补来源。隐藏／换owner／换槽先退本页资格，保存未知回执另保，不自动重试；纯ticket覆盖确认、调用、结果采用及finally。

Root共用唯一地图选择，exact5投影以native-page／session-snapshot／file-history区分；Memory分支另核实际owner、selected token、完整report及binding全文，不能借Archive token。新append使旧冻结sourceCurrent=false仅改变提示，不剥夺历史审阅资格或篡改内容。固定深度冻结、plain撤销引用、原槽和lifetime尾核保准确旧clear；地图仍只画离散点，极区明确省略、不连线／自动移图／Goto／任务或飞行控制。外审发现请求接纳前清共享latest会误清另一来源的新图层，已同时移除Archive及Memory请求桥预清，保留调用前lifetime尾核，由本页精确lease及各自实际native token退休清自身选择。file时间标签另修为文档声称UTC，不能称本次读取或可信本地采时。

作者自读、主代理对原生集合／getter／值校验／单槽双终结／共享IO与维护映射／Root／Panel和History／Map新增链及工程登记的直接阅读，以及作者外的原生／IO／Root／UI交叉静读已完成。最后读回footer矛盾拒绝、初始idle／正revision、selected getter同步fence与实际异步busy区分、退出cancel、文件时间措辞及两向错清修补；限定523新增链未见确定剩余源码问题，不是全仓无缺陷、构建成功或实际Qt事件／视觉／线程／文件系统验收。

普通材料核对：36路径均存在，18份文档5751个本地Markdown目标无缺失；清单36条与本批范围一一对应，48项工程场景连续且全部未执行。无冲突标记、尾随空白或tracked diff诊断；23条范围内untracked路径的NUL差异空白核对均无诊断，其中仅8条为本批新增。差异退出码1只表示相对NUL有内容，不是测试或构建通过。

[36路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项未执行工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[Memory合同](../../custom/src/Swarm/FleetOperationalMemory.md)、[worker合同](../../custom/src/Swarm/FleetGnssMemoryWorkspaceWorker.md)、[界面合同](../../custom/src/Swarm/FleetGnssMemoryWorkspacePanel.md)。全部工程场景未执行；未编译、执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、预览、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持，约40%／60%仍仅源码工作量粗估。本批不是全历史索引／自动留存／跨任务统计／真实环境风险模型／正式任务结果或最终StartPermit。下方为历史。

## 522：同包历史 GNSS 诊断与显式地图静态审计

范围为8源码／工程登记及13文档，共21路径；新路径只有GnssReview h／cc／md。新helper无QObject，不新增QML资源／服务、wire、依赖、数据库schema、权限或PX4改动。复用现实际Archive／Reader／Codec／MotionReview，未创建平行采集器、存储器或飞行控制链。

直接对照原完整native page、Motion inspectPage／review及Codec recordConsistent与本地MAVLink GPS_RAW_INT decode／enum。先非null rows≤64，再恰一次复用完整页校验后单pass解释消息24；其它坏行不允许成功前缀。MAVLink2短尾只在本地零初始化消息中作标准解码，原payload不改。lat/lon、fix、eph和卫星始终同帧，不借其它消息／当前Fact／分页；fix3／4／5／6／8加合法E7才提供坐标，(0,0)仍为合法报告，0／1／2／7／未知fix与越界不投点。

原eph65535与sat255才未知，分别为false／−1；已报告0不补成未知或达标。HDOP为eph/100，无单位、非米精度。2.5／6只为公开显示启发式，提示／未知／阈值内仅对定位行闭合计数，不认证正常、安全或漂移。top33／identity7／policy3／row18保本页来源和有界计数；sourceSequence保完整正uint64字符串，UTC仅本机接收时刻，不重建统一时钟、跨包轨迹、精度圆或风险区域。

Archive新gnss-review保exact6／receipt11，用实际固定catalog／cursor／vehicle／includePosition与当前完整原页。独立成功UUID即使同页同值也换代，getter保固定目录／策略、原双终结及单槽、无采集和原7身份；newquery／cancel／capture／catalog／退出清二者，旧运动与维护通道不借此认证。发布后QQ／serial／页全文／token尾核不把通知内新状态回滚。

Root只接准确reviewToken及纯撤销引用，自取实际原生报告；map选择保owner／query／完整JSON与不回绕serial，旧clear只匹配自身selectionToken。发布后核原引用／原token／value／lifetime，调用期间撤lease不能复活。独立外审修正了公开readonly var仍可改子对象的问题：固定深度冻结coordinate／sample／list／identity／policy／report／wrapper，冻结缺失或异常返回null，Map保守判空不绘。副本与冻结不等于来源认证或执行授权。

History在原位置记录同意／本页读盘之上独立确认GNSS坐标发布，完整值核对后只显示；地图仍需另一明确动作。按压／ticket／consent／input代次与原savedPage核对，换输入、隐藏／禁用／换owner或销毁先撤自身lease，晚回只清原选择。外审修正selectionToken误当UUID；实际是reviewUUID:正serial。Map沿同Root facade，仅离散历史点，AllButtons含label吞点击；极区原值留报告、底图省略数明确。没有Goto／center／fit／任务动作或全图控制权限变更。

作者自读、主代理对原生6路径／Root／两个界面新增完整链及工程登记的直接阅读、非作者交叉静读均已完成。原生另追实际Motion／Codec／GPS解码链；Root及UI外审读回了令牌格式、公共投影冻结／null兜底、撤销对象getter／setter重入后的lifetime与原选择槽身份、两个界面cleanup处理器的通知尾核及saved DTO双侧同值。限定522新增链未见确定剩余源码问题，不是全仓认证、构建通过或真实Qt事件／视觉已验证。

普通材料核对：21条路径齐全；13份文档5687个本地Markdown目标无缺失；清单21条与本批范围一一对应，40条工程场景连续且全部未执行。无冲突标记、尾随空白或tracked diff诊断；10条范围内untracked路径的NUL差异空白核对均无诊断，其中只有3条是本批新增路径。差异退出码1不是测试通过或构建成功。

40项工程场景全部未执行；仅源码、直接静读和普通文本／路径／Markdown／空白核对，未执行项目／parser／helper／QML／JS／业务／模型／认证／SQL／hash实现、编译、测试、预览、仿真、硬件／PX4、刷机、放行、提交、PR或打包。

[21路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[完整原生与Root合同](../../custom/src/Swarm/FleetTelemetryGnssReview.md)。原25目标与六类实质剩余保持；这是长期地图真实单页数据基础，不是持久跨任务全集或最终StartPermit。下方为历史。

## 521：普通区域任务性能下限静态审计

本批把后端已有但普通区域任务无法输入的minimumSpeedMps／minimumEnduranceSeconds接成实际流程，不另建性能采集器、审批器、飞行发送器或能力认证。限定9份源码和18份文档，全部为既有路径；未修改PX4、权限、wire、资源或依赖。

直接对照Draft完整settings、原commit与两source validator。新atomic setter只收实际非null Map的三可选键，错误类型／未知键或坏数值不能被转换为空Map；空actualMap明确移除三政策，其余内容保留。两floor域0..9007199254740991，显式0保presence；原strict setter／clear继续只改Bool。等值沿原no-op，通知后false不宣称回滚已提交内容。

任意floor使用planningSource schema6且外层仍exact5；旧schema1..5夹带新键拒，6缺全部floor拒，strict与旧其它optional可独立组合。真实buildTask按presence发根三政策。Compiler预检完整重建任务，数量计入两floor并逐原key比值，源有而task缺、外加、改型或改值均阻断，不以新表单绕过原完整源与几何条件。

主／备用候选在Matcher和角色打分之前执行正下限；显式固定成员由Compiler再逐机核对。不严格也不能把未知／Registry−1当达标，不能拿组内最高或别机数字补本机。strict继续只取当前工程批准和完整匹配基线的canonical声明；非strict仍为原规划描述符／原别名规则，不升级为实测可信度。缺／0不新增对应positive证据依赖，原能力、健康、逐机能量和电池门保持。

actualplan保两个floor含0，完整task仍进入原plan身份。PlanStore白名单及实际原area-source task对照、PayloadBuilder正常build独立源对照都核两floor的presence、类型、数域和值，并核strict Bool的presence／实际类型／同值，不能绕Store改false或删政策，不以归档摘要代替。原无policy历史复现只用于已有冻结payload／provenance自洽，不是当前build的用户开关；source6的完整policy走正常链，不从历史重建批准。

界面沿原窄Draft facade和既有pending门，同次编辑strict三态和两可选下限。按压身份保原source、revision、完整原／拟政策、表单对象及非回绕确认代次；任何变更撤旧确认，不默认填数值或保存半套。关闭、来源替换、失败或通知中销毁只退资格／说明已提交可能性，不自动重试、回滚或恢复批准。单位说明把能力门槛与巡航指令／当前可用时间分开；UI/UX只作静态通用指导。

作者自读、主代理对原生Draft／候选与角色消费者／Compiler／Store／Builder／Root／最终UI的直接阅读及作者外交叉静读完成。Draft三路径有非作者完整源恢复／commit尾核；两个筛选器及七个消费者有独立外审，并读回最后Builder strict Bool配对增量；UI全文与原父pending、窄facade和nativeCAS有两位作者外对照。限定521新增链未见确定剩余源码缺陷，不是全仓认证、构建通过或运行可用性证明。

普通材料核对：27条路径均存在，18份文档5684个本地Markdown目标均对应现有路径；清单27条与本批范围一一对应；48条工程场景连续且全部未执行；无冲突标记、尾随空白或tracked diff诊断。17条范围内既有untracked文件的NUL差异空白核对也无诊断；退出码1仅表差异，既非测试通过也非本批新增路径。

本批仅源码、直接静读及普通文本／路径／Markdown／空白核对。[48项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，[27路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)不是整个dirty树差异。未编译、执行QML／JS／项目／parser／helper／业务／模型／认证／SQL／hash实现、测试、仿真、预览、设备／PX4、刷机、放行、提交、PR更新或打包。原25目标与六类实质剩余保持；这不证明可信实测性能、完整执行、净空或StartPermit。

## 520：备用出勤显式只读刷新静态审计

范围限定为两份QML源码及对应13份文档。新增本页start／status／cancel适配，底层仍用原FleetMissionEvidenceRefresh；没有新C++、资源、协议、权限、数据库、硬件或PX4改动。本节不是全仓审计、构建通过或可飞行声明。

直接对照实际原生request／gate／current／cancel、原Root scope／Bridge／Draft退休路径与本页生命周期。start／status闭14字段分别表达请求接纳、当前归属、实际活动和原27字段结果；accepted原生28字段中的requestAccepted不混成新观测成功。原始拒绝5字段及Root早拒不被强套为成功。原对象的唯一下一代、私有capture／所选ID及准确requestId／generation阻同ID或共享latest认领。

额外私有撤权围栏覆盖身份／角色、旧审批撤销、本地Draft各通知尾与原native最终gate，复用既有SourcesMatch／Allowed，不以remainder自身busy或native checking阻正常调用。可选同引用cancellationState只撤本次意图，不能授权；准备期尚无ID也能先失效，晚回仍仅清原请求。取消原对象不为旧Episode取得新发送scope，不能撤回已排队包或删除档案。

外审另核“本次早拒后下一代被generic使用”的边界：首次绑定原生ID只许在本次实际starting栈内，Bridge work的finally在释放lease及最后通知之前关闭首认领窗口。退出时仍无原ID的attempt不能凭同对象／下一代／同机号首认领后来任务；晚cleanup、status或历史缓存也不得越过这个门。已绑定的原ID／代次才继续精确观察、留史或取消。

原current被后来generic任务替换后，只保本次最后实际观察的脱离status；历史status.active可能仍为true，wrapper.current／active必须false，不能用缓存通过gate、精确取消、新鲜度或批准。UI的owner／Episode／查询异常fallback也须强制非当前，不续用旧start回执的current=true。历史与当前active分层显示，不能把合法历史错拒成永久忙槽。

刷新前退休旧offer、提案和审阅资格；同来源只保本页手填动作／坐标和已明确采用的canonical启动策略纯拟值，不保hash、速度采用或确认。未采用启动策略表单在按钮、入口及最后调用前均阻断。刷新准备／活动与本页其它操作互斥，但精确取消／退出保留；实际结果发布持plain票据，自己的通知不被误判为新用户授权。

严格保留原真实地面未解锁／未飞行、Session及mission.evidence.refresh角色、PRIMARY、参数ready／无写、成员占用和启动／部署条件。原版本＋固定29参数、120秒全批、4秒阶段及100ms节流不扩。observations-renewed不保证519所需12项仍≤30秒；必须再手动observe、采用新profile及原完整审阅／审批，无自动读取、批准、上传、接替或StartPermit。

作者自读、主代理对实际原生生产者／Root／UI的直接阅读，以及作者外交叉静读已完成。独立外审读回了首次认领窗口修补、两处native回调、privateFence与本地通知尾；另一作者对实际Panel闭14／27、取消、同意与拟值保留作交叉对照。上述限定新增链未见确定剩余源码缺陷，不是全仓认证、构建通过或运行验收。

材料核对：15条路径均存在，13份文档的5596个本地Markdown目标均可解析到现有路径；清单15条与本批范围一一对应；40条工程场景连续且全部未执行；无冲突标记或尾随空白，tracked diff检查无诊断，6条范围内既有untracked文件的NUL对照空白检查无诊断。这里的untracked不是本批新增路径，且NUL差异退出码1不表示测试通过或构建成功。

本批仅直接静读和普通文本／路径／Markdown／空白核对；[40项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。[15路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)不是整个dirty工作树差异。未编译、执行QML／JS／项目／parser／helper／业务／模型／认证／SQL／hash实现、测试、仿真、预览、设备／PX4、刷机、放行、提交、PR更新或打包。原25目标及六类实质剩余保持。

## 519：备用出勤真实参数绑定与结构配置静态审计

本批复用既有Coordinator.executionProfileObservation，不新增自报证据、证书或另一参数采集器。原正常Draft入口硬需Draft成员，不能拿它为备用新出勤伪造Draft；新Root仅按其实际私有冻结capture与明确reserveId进入同一原生12参数只读API。

Root保留原native offer、实际reviewer／capture／scope和独立offerToken；合法入场的新read退休旧token。preview第三参数核准确本次私有offer，再重新读取原生fingerprint与profileHash；同内容hash不是同观察代次。坏来源、其它备用、同值ABA、失鲜或换实际对象均不得用caller Map／旧UI JSON恢复来源。读取不刷新或写参数。

新增纯值Editor复用原StartupPolicyEditor，不接controller／假Draft。操作员明确采用原备用预算速度并输入新动作／AMSL位置／策略；requiredProfileHash仅来自本次真实offer。原Planner仍强制relative-dwell、巡航／返航同cruise及预算垂直速度，实际review仍核当前参数／能力／能量／电池。新编辑器不修改这些门禁或生成权限。

作者自读及作者外交叉直接静读已完成。Root／Workflow对照实际Coordinator的19字段／12行15字段、原policy8和Planner速度／时序门；新Editor／Panel全文对照Root与原共享StartupPolicyEditor。限定新增链未见确定剩余源码缺陷，不是全仓审定、构建通过、界面可用性验收或运行认证。

正常链按实际来源逐段核对：capture→明确候选→读取原参数offer→明确采用预算速度／输入新动作与策略→生成本页profile→原offer重新取证／一次消费→原完整提案及审阅。纯编辑只退休旧提案而保同offer session，自己的adopt不会被误判换源；实际preview返回并核结果后才精确清原本地offer，既不自阻也不抹后来新offer。独立sourceIdentity不依赖offer是否null；同来源重读只保本页canonical拟值，速度采用／确认／adopted及hash均重新建立。未采用策略表单在按钮、真实入口和调用前阻重读，关闭／退休不能冒充采用。

静读中补齐失败／通知尾：尚未成功返回的半offer只按原对象CAS清理；公共值另复制不改私有offer；one-use预览失败需重新读取，界面不反复提交旧token；新read失败与成功后退休旧提案的文案分开。依据年龄仍是原PARAM_VALUE接收年龄，读取getter不使它刷新。以上为源码阅读和修正，不是运行复现。

最终普通材料文本核对：18路径齐全且与manifest一致，5源码／资源＋13文档；13文档5559个本地Markdown目标存在，40项场景编号连续且全部未执行；无冲突标记／行尾空白，限定git diff --check无诊断／exit0。范围内7条未跟踪路径的NUL空白核对无诊断，不将7当本批新增数；实际新增仅Editor.qml与合同md两条。此处不执行项目解析器、业务、QML或工程场景。

[18路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[工作流](../../custom/src/Swarm/FleetReserveDispatchWorkflow.md)。原single／group入口和exact3底层请求不加offer字段，无新C++、wire、依赖、schema或权限。

UI/UX技能仅指导原Qt/QGC主题、分步确认、完整输入、PlainText长值、错误反馈和焦点；未执行搜索脚本、预览或视觉／交互验收。未编译、运行项目／helper／业务／QML／JS／模型／认证／SQL／hash实现、测试、仿真、硬件／PX4、刷机、放行、提交、更新PR或打包。原25目标／六类与约40%／60%源码粗估不因本批上调。下方为历史。

## 518：维护文件工作区的静态审计

本批沿517真实Passport记录推进显式文件保存和独立历史浏览，不新建无人消费的导出helper。Root窄门面实际配对Passport，Overlay嵌入新Panel；qrc／qmldir及native worker工程登记同时接线。新文件业务从不调用active load/import，不改变当前维护记录、517原生页、任务、审阅批准或飞行权限。

记录源instance／非回绕revision和实际owned values在入口冻结，GUI分turn仅做纯JSON类型与有界结构预检；QObject／自定义类型、有损64位整数、非法null与非有限值不能偷偷转换后交worker。实际I/O、规范化、评估和完整canonical详情准备在一次性QThread完成，私有完整cache仅原生值交接，公开state11／result11不含整表bytes或记录。原jobId／nonce／serial／actual worker身份在result+finished双尾核后接纳，busy不长期锁517追加。

来源组13和记录8逐页最多8项，完整record／assessment用detail10的2048 UTF16连续片；raw file hash与规范化详情hash分别对应实际对象。分片末尾不割代理对，nextOffset是真实后继，未打开／未续读不冒充已核完整。session和file两私有槽分离，旧冻结来源版本变旧只使sourceCurrent失效，不把内容换成新records；文件history不能变当前维护认证。32MiB/2000记录、depth24／262144节点与详情64MiB为数据预算，不是进程RAM或瞬时取消硬保证。

保存绑定原session snapshotId/hash和原绝对路径，选路径后另确认可能覆盖。QSaveFile禁直接写回退；commit失败保fileWrittenKnown=false，commit成功的written事实不被取消、回读失败或UI退场抹去。readbackVerified独立核实际原bytes，fsync／durable／认证仍false。路径检查不认证文件所有者或跨进程／父目录竞态免疫。同步clear只清准确slot，不删文件、不清active记录、不制造异步完成，最后文件回执保留。

作者自读和作者外交叉直接静读已完成：Passport、Worker及Root两向对读，新Panel由两位非作者全文对照实际原生合同与既有QGCFileDialog入口。按当前实际来源对应冻结／独立重开→来源组→记录／完整评估→显式保存，以及同步清槽、原job取消和文件事实显示；在限定所读新增链未见确定剩余源码缺陷，不是全仓无缺陷、构建通过、交互验收或安全认证。

静态阅读中修复的边界包括：Qt JSON null的Invalid／Nullptr两种表示与其它typed-null分开；拒有损64位整数／非有限值；无known指标不报low风险；新工作区拒generatedAt超过128 UTF16单元的整份文件，不把巨串公开或截断为原文。界面先等同一jobId及原action的结果，不能因state绑定先更新而把旧结果当本次畸形回执清pending；finally使用入口冻结的plain释放闭包，通知销毁后不再读死Panel，退场／重入也不复活旧确认资格。这些来自源码阅读，不是运行复现。

最终普通材料文本核对：23路径齐全且与manifest一致，10源码／资源接线＋13文档；13文档5519个本地Markdown目标存在，48项场景编号连续且全部未执行；无冲突标记或行尾空白，限定范围git diff --check无诊断／exit0。范围内10条未跟踪路径的NUL空白检查均无诊断，不把10当本批新增数；本批实际新路径仅worker h/cc、Panel.qml及合同md四条。此为交付材料核对，不执行项目解析器、业务、QML或工程场景。

[23路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[完整合同](../../custom/src/Swarm/FleetMaintenanceWorkspace.md)。无新QML服务类型、wire、外部依赖、SQL schema、权限或PX4修改。

UI/UX技能仅用于现Qt主题、分步文件确认、只读来源区分、PlainText长值／焦点与错误恢复的静态通用指导；未运行搜索脚本、预览或视觉／交互验收。仅编辑、直接静读与普通文本／路径／Markdown／空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。原25目标／六类实质剩余保持，整体约40%／60%仅工作量粗估。下方为历史。

## 517：历史电池证据进入实际维护护照的静态审计

直接对照当前Archive的私有page36/frame29，固定本机目录／策略、双终结释放和真实query／serial绑定。metadata候选6／来源5只由原BATTERY_STATUS产生；prepare先退休旧私有预览，两次通知后核QQ、非回绕代次与完整原页，native-only record再次核当前与busy。新读取、取消、实际新capture／退出退休旧资格，public summary／原Codec／遥测流不扩。

预览14保source7/evidence21及实际可用的两指标，温度INT16_MAX、电流-1和其余负充电分开计数，负温度与零电流不被当未知。来源只表示本档案观察组；同页最大报告值不是整航次、实物身份或当前健康。原本地UTC首末／极值及回退保留，不改成导入时刻。不能表示的时间、全未知或未来时钟仍可能拒绝追加。

Passport实际消费者只取actualArchive私有record，完整source独立scope与selected序号闭区间去重，历史与legacy不混合。schema1完整归一化历史字段并原子拒坏记录／重叠，满2000不淘汰。纯legacy根5不变；含历史时根6必须带historySourceContract固定字符串且实际记录同在，旧版根白名单拒该新字段，防未修改文件被静默剥metadata／按同机号混组。它不是签名或防篡改证明。历史按原序号选最新页且禁物理两点趋势，不补振动RMS、衰减、飞行次数或当前许可。实际append先冻结结果、records与assessment，再通知；外调销毁不抹追加事实，同栈嵌套mutation被挡。

Root固定原Archive／Passport配对，以不发Qt通知的plain gate覆盖native调用，recordsChanged消费前后核实际passport／寿命。新增嵌入式维护面板把选择、预览、追加分成独立人工动作；当前页、来源、选择、同意代次与plain ticket贯穿调用和结果发布，旧press不能认领新的预览。历史追加回执只表示本进程记录，不自动保存；指挥中心摘要改按来源组而非架数，不能认证当前维护健康。

静态对读中确认并修复新链会触及的两处数值归一化问题：JSON Double毫秒UTC不能用返回int的qFloor判断整数；温度合法负范围不能让非法类型的-1哨兵通过。另关闭Root Qt busy setter外调尾与UI过早解busy／同意消费通知尾缺口。这些是源码审阅发现，不是运行复现或测试通过。

作者自读及作者外对Archive／Passport／Root／Panel／现有指挥中心的交叉直接静读已完成，末次再核条件根5／6哨兵与四处按下票据清理通知后的寿命尾。正常读来源→选择→预览→另确认→追加，以及合法未知／时间诊断与已commit事实的消费者对应；在限定所读新增范围未见确定剩余源码缺陷，不是全仓无缺陷、运行通过或安全认证。

最终普通文本核对：23路径齐全且与manifest一致；13文档5503个本地Markdown目标存在，40项场景编号连续且全部未执行；无冲突标记或行尾空白，scoped git diff --check无诊断／exit0。范围内11条未跟踪路径的NUL空白检查无诊断，不把11当本批新增；实际新增文件仅2条。仅交付材料核对，不执行项目解析器、业务、模型、QML或工程场景。

UI/UX技能仅用于现Qt/QGC主题、分步同意、长值换行、键盘焦点和错误恢复的静态通用指导；未运行搜索脚本、预览或视觉／交互验收。[23路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[新链合同](../../custom/src/Swarm/FleetMaintenanceHistoryImport.md)。本批10源码／资源接线＋13文档，新文件仅Panel.qml／Import.md；无新C++类型、wire、依赖、SQL schema、权限或PX4修改。原25目标／六类实质剩余保持，整体约40%／60%仅工作量粗估，不按本批字段上调。

仅编辑、直接静读和普通文本／路径／Markdown／空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。下方为历史。

## 516：近期原生SOC下降窗实际接线的静态审计

本批不从Fact、导入百分比或静态耗率拼原生窗口。已直接对照Vehicle原RAW冻结与原complete最终QQ／epoch／token重新定位，独立有界留点、每原报告回升／gap检查、源退休清空、发布／失效语义版本及只读getter。新top25/window8不改旧电池20字段；flat保旧终点和速率，预算统一扣max(RAW age,下降终点age)，源/窗口鲜度与历史stale诊断分开。此为收到SOC的局部估算，不是实测功率或未来耗电上界。

Registry沿原实际Vehicle／recordEpoch／bindingEpoch在同投影guard读取两证据，每次保绑定尾核，静态root／metadata与返回快照的同名原生键先剥再覆。producer坏值保明确invalid，不当合法unknown。共享Reader核完整25/8与旧电池身份、最新receipt／global版本、历史下降端点及公式；纯值一致不认证调用方map或硬件。

Scheduler在已有模型完成后只min已known时间／已known投影，之后原margin／eligible／score／替补及汇总消费较低值；不调用会补known的includeEstimate、不替代严格声明。新可选top列表逐行14字段保前后值、原因和完整证据；assignment18/21及编译预算保持。完整energyPlan自然进入原Coordinator、存档证据hash、工程报告及515只读明细，未扩旧archive schema、ReserveEnergyPlan闭形状或执行权限。

Root复用exact6 native值桥；稳定identity保来源／状态／窗口语义，排重复receipt与自然age。批准期实际预算尾核覆盖原required与原替补阈值，新失窗或换窗不能凭旧approved回退更宽基线。桥ageMs是模型扣龄而非RAW年龄，动态值只在usable时给出。普通规划页仅新增清楚的只读依据标签，不把它说成全Fleet窗口看板或已验证UI。

静态对读已修两处真实producer／Reader边界：延迟提交使最早RAW已老而旧late报告仍fresh时，observed未知诊断不能误判坏值；wholeRAW忙但旧电池通道尚未busy时，新receive-in-progress的精确全unknown形状允许sourceCurrent差异，不因此授usable。坏known／错源或任意旗标变化没有宽免。

坏新drain拒绝之前仍先保合法旧电池20字段，使原bound拒绝账单不因新诊断丢掉既有来源。这不接受坏窗口，也不借旧电池数据恢复新窗口模型。

作者自读及作者外对最终原生窗口、共享Reader／Scheduler／Coordinator、Registry／Root消费链与本批材料的直接静读已完成；实际producer与consumer、未知诊断、公式及旧shape逐段对照。限定所读新增范围未见确定剩余源码问题，不代表运行无缺陷、工程验收或安全认证。

冻结后普通文本核对：29路径齐全并与manifest一致，48项场景连续且全部未执行；18文档5682个本地Markdown目标存在，无冲突标记，scoped git diff --check无诊断。Vehicle.h／cc原有9／13条行尾空白与HEAD的完整行文本比较不变，不顺手清理无关旧内容；本批未新增行尾空白。范围内17条未跟踪路径的NUL空白检查无诊断，不能把17当本批新增；本批新增文件为0。这些只核交付材料，不执行项目解析器、业务、模型、QML或工程场景。

[29路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[实际数据链](../../custom/src/Swarm/FleetNativeBatteryBridge.md)、[共享校验](../../custom/src/Swarm/FleetNativeBatteryEvidence.md)。11源码／接线＋18文档均为既有路径，无新文件、资源、wire、依赖、数据库schema、权限或PX4改动。原25目标／六类实质剩余保持，约40%源码／60%剩余仅工作量粗估，不按窗口或字段上调。

仅源码与文档编辑、直接静读及普通文本／路径／Markdown／git空白核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。下方为历史。

## 515：逐项协议／完整账单与精确重审的静态审计

本批消费514真实独立出勤新记录，新增默认折叠的FleetReserveDispatchReviewPanel，不创建启动capture或第二任务来源。已直接对照实际Spatial producer：top17、vehicle15、encoded item9、encoding error5；记录task／plan／storeEpoch／hash／revision、唯一成员／role和固定false边界对应。原生编码值经同次准确存档记录产生，前端不重跑Builder、不重编码、不给成功前缀或把seq当点索引。

完整energyPlan／riskAssessment／airspaceReport／输入检查／transportEncoding与完整review均有同身份层级入口；每层8项，长键和值分片，null／undefined／false／0／空容器及负margin原样区分。折叠不生成明细委托，稳定token／archive改变才重置页，current／approval变化不恢复许可。最大24层／32768数组／1024对象键的显示界限只明确未展示／未核验，不伪装完整空结果。所有字段是原审阅值，不认证性能、动态轨迹、净空或执行。

Root新增精确five-key显式重审，正常新审阅和重审各有实际本地reviewAttemptToken；异步完成沿同attempt／request／proposal及实际归档收据。原完整capture和原生built task保留，撤旧采用／本地批准后重新prepare当前证据，不嵌套public operation、不借旧fleet或权限。忙、外来live binding、变更provider／输入内容代次及ABA拒绝；取消必须真实原请求终态才可人工再次确认，不自动重试。

主Panel只保存真实返回且同status／proposal的attempt，按下冻结capture／proposal／attempt／review／request五身份。新结果必须重新采用、本地申请批准／明确批准，并继续原独立上传门禁。隐藏保描述性规划锚而清同意与未采用资格；真实编辑退休旧锚，scope／controller换源仍清本页exact source。失败退出若旧context已退，只清原capture，不猜共享最新reviewToken。

静态对读已推动关闭两处重审实际缺口：标准committed不能被后续归档失败抹掉已退休事实，而新recordCommitted仍只取本次真实归档；新attempt安装时不能让旧context token留住、永久挡住失败后的再试。新input内容代次与原prepare对象区分参数ABA／新同值prepare和单纯批准失效，不把当前旧内容相同视为同一输入过程。

原输入／prepare的来源检查也进入本地失效及原Scope每次cancel／revoke调用前后。准确短收据覆盖初次来源捕获的Store.revoke、重审attempt的Store.revoke及Scope末尾直接Intent.invalidateReview；archiveChanged及其触发的stateChanged实际处理器在退休planning前使用同一来源检查，不等外层调用返回才识别通知中新建的P1。初次来源私有冻结原preparation（包括null）与内容代次，正常approved／awaiting到preview分开before、仅实际失效调用内invalidating、after-state三阶段，只容原输入代次恰加一。短收据finally退active并只清自己的槽，不作为权限或全局信号过滤器。

作者自读及作者外对新明细、主面板／部署接线与最终Root三处退休链的直接静读已完成；完整合同与真实producer、attempt归属及正常一次状态推进分别对照。限定所读新增范围未见确定剩余源码问题，不代表运行无缺陷、工程验收或安全认证。

冻结后普通文本核对：21路径齐全并与manifest一致，48项场景连续且全部未执行；15文档5604个本地Markdown目标存在，无冲突标记或行尾空白，scoped git diff --check无诊断。新ReviewPanel的qrc别名和qmldir本地类型各恰一条。本范围10条未跟踪路径的NUL空白检查无诊断，不能把10当本批新增文件数；本批新增仅2条。这些只核交付材料，不执行项目解析器、业务、QML或工程场景。

[21路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[新明细合同](../../custom/src/Swarm/FleetReserveDispatchReviewPanel.md)、[Root合同](../../custom/src/Swarm/FleetReserveDispatchWorkflow.md)。6源码／资源＋15文档；新路径仅ReviewPanel.qml／ReviewPanel.md两条，无新C++、wire、外部依赖、权限或PX4改动。

仅源码／文档编辑、直接静读和普通文本核对；未编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。原25目标／六类实质剩余保持，约40%源码／60%剩余仍仅工作量粗估，不按本批显示字段或重审按钮数上调。下方为历史。

## 514：真实备用独立出勤入口与共享部署的静态收口

本批在513后台Planner之上接真实当前来源，不创建另一套任务发送器。已直接静读Root候选／完整冻结／精确原Store.record、bounded copy、三kind、预览／新审阅／同步及异步真实回执；父Planner互斥、唯一共享部署sourceKind、旧Single隔离、资源登记及Map全量view17适配分别与实际消费者对照。

来源冻结在撤旧之前，固定实际Intent／Reviewer／Store、任务ID／intent、完整参数与plan、输入及审阅代次。精确原Store.revoke的同步通知用私有plain收据记录实际native invalidateReview成功；只容自身approved／awaiting→preview带来的一次input推进，不以新值重基。直接对读还修正了两处实际问题：普通政策Map的length键不得转换成List；正常已批准来源撤旧时不可被旧state／input比较误拒。失败不恢复原批准或覆盖他人新来源。

新Panel按实际私有capture／proposal／review token及确切requestId认领；完整summary14和实际dispatchMember供人工核全部路线／profile。来源捕获前不发父级编辑失效，避免先清原binding；隐藏／取消仅处理本页准确请求。采用后的本地申请批准／明确批准必须走原API及真实状态门禁，不能把采用当批准；这关闭了初稿中采用后停在preview、无法进入共享上传的确定源码缺口。通知前保存plain life／owner，通知后核对，实际Qt回调行为仍待工程师验证。

唯一共享部署区显式draft／remainder／reserve-dispatch，不回退过期来源；当前批准、编码、账户／角色、独立账号审批、覆盖确认与native fence保持。新kind不放开草稿修复。旧Single只接受single记录，不能误显示／采用／批准／取消备用出勤；Group原显式kind隔离保持。采用、本地批准、上传、机载执行与最终StartPermit彼此不等价。

新独立显示由准确新Store记录和同份reviewEvidence经既有missionSpatialReview产生。Map复用原全量Spatial17读法并核唯一实际ID／角色、空reserve、两条完整路线及总点预算；不画部分前缀、action marker起降轨迹或旧区域替身。路线标签另标独立出勤、规划／过期历史与原record版本，图例保完整身份。新Panel目前显示完整路线／profile而非逐项协议编码，共享区只显示实际目标／编码总数，提示已准确区分；已有完整编码值不能冒充逐项可见入口完成。二维坐标显示不认证动力学、障碍或净空。

作者自读及作者外对Root／Panel、父级互斥、共享部署、旧Single、Map与资源的直接静读均已完成；两份新合同与五份同步模块合同亦对照实际源码。最后还补精确失败回执清理与销毁清本页capture，防止已返回token却无法退出的孤儿来源；清理不猜最新ID、不撤已提交reviewContext，通知后先核局部寿命。限定所读新增范围未发现确定剩余源码缺陷；这不是运行无缺陷或工程验收证明。

冻结后普通文本核对：26路径齐全且与manifest一致，56场景连续并全部未执行；18文档5621个本地Markdown目标存在，无冲突标记或行尾空白，scoped git diff --check无诊断。qrc别名与qmldir本地类型各恰一条。本范围14条未跟踪路径的NUL空白检查无诊断，不能把14当本批新增文件数；本批新增仅3条。这些只核交付材料，不执行项目解析器、业务、QML或工程场景。

[26路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[56项全未执行工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[Root合同](../../custom/src/Swarm/FleetReserveDispatchWorkflow.md)、[Panel合同](../../custom/src/Swarm/FleetReserveDispatchPanel.md)。8源码／资源接线＋18文档；新路径仅Panel.qml／Panel.md／Workflow.md三条。原25目标／六类实质剩余保持，约40%／60%仍仅源码工作量粗估。

本批只有地面站源码／文档编辑、直接静读及普通文本／路径核对；没有编译、执行项目parser／helper／业务／模型／QML／JS／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。完整任务执行、可信性能、自愈机载改派、跨站控制、长期运营及真实模型／插件生态仍未完成。下方为历史。

## 513：独立备用出勤后台与实际编译／编码／归档来源

本批新增FleetReserveDispatchPlanner纯值任务生成，不把备用预算或旧Run来源改成执行权限。直接静读source3／request3、原binding7／9及完整plan摘要、全部原主／备用ID及描述、同一ReserveEnergyPlan重建的选中完整预算、新profile和整份原主路线。原heading／hold从完整主路线保留，不取已简化预算path；新任务不重演旧待机，当前电池和性能仍须普通新审阅。

来源约束按真实Compiler的八项root别名仅补缺失规则重建，再与原plan.constraints完整比较；原task未知约束不能被plan副本覆盖丢掉。原task.area和实际全部members重新生成已有边界政策并比较，不只信旧摘要。raw assignment按原三种一致数值ID别名，冲突不改号；共同安全／能力／能量和所选member未知要求／floor保留。新单机用途显式重设roster／角色／备用0，不宣称原组已完成接替或原组保障仍不变。

新provenance闭17字段，task完整hash与真实compiled member／constraints／boundary／strict bool的route hash分开；matchesPlan只认actual members=assignments及明确空reserve集合，不走task投影。严格源的原task／plan布尔一致，保留声明及嵌套review／baseline闭形状只是值自洽，不提供当前认证。旧single／group的scope清理识别新来源，两个直接matcher均拒混用；旧schema／hash规则不因新来源改写。

已直接对照Compiler前置validate＋matchesTask与输出前matchesPlan、PlanStore白名单及共用validPlan、PayloadBuilder正常编码前完整来源检查、Coordinator owning-thread薄口和pri登记。四来源互斥；失败记录blocking或空payload，不删除坏来源再冒充普通可用任务。原编码命令、INT／float32后边界、所有资源／净空／权限及最终StartPermit门保持。没有Root来源捕获、可见操作流程、共享部署选择或机载接管，本批不能当完整备用出勤产品。

作者自读及作者外对上述新增源码链的独立静读已完成，限定所读新增范围未见确定剩余源码缺陷；不是运行无缺陷或工程通过认证。显式returnDurationSeconds仍独立要求严格正数，不能与允许0的returnReserve合并后被默认120秒修掉。[28路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全部未执行工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[新模块合同](../../custom/src/Swarm/FleetReserveDispatchPlanner.md)。10源码／工程登记＋18文档，新路径仅Planner h／cc／md三条，不从git未跟踪数推算新增文件。

冻结后普通文本核对：28路径齐全并与manifest一致，40场景连续且全部未执行；18文档5580个本地Markdown目标存在，无冲突标记或行尾空白，scoped git diff --check无诊断。本范围19条未跟踪路径的NUL空白检查无诊断，不能将19当本批新增路径数。上述仅核交付材料，未执行任何项目实现或工程场景。

仅源码及文档编辑、直接静读和普通文本／路径／Markdown／git空白核对；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、操作硬件／PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，约40%／60%仍只是源码工作量粗估。下方为历史。

## 512：联合余段任务新备用保障的静态审计

本批补齐原主／备用换机后的显式新规划路径，不删除旧描述仍指旧ID时的原Compiler拒绝。直接静读GroupRemainderPlanner的旧exact3／新exact4入口、完整新描述规范化、替换前原policy hash、schema3／exact28，以及真实Compiler、PlanStore、RemainderView、ReserveEnergyPlan和ReviewCoordinator／EnergyScheduler消费者。只有明确提供完整自身起返点、替补对象和运行配置才生成新任务；原最低备用数、共同安全／能力／能量floor不因换机消失，旧clearanceInput不自动换ID。

新task完整匹配与compiled-plan匹配分开：后者核真实reserveMembers.energyPlan，复用原生成器结合实际新主机完整路线重算完整路径、停留、待机、任务和返程成本，不接受顶层reservePlans替身或只比首尾。新schema3 routes hash同时绑定canonical严格性能布尔，实际行声明存在性由该布尔决定；不能同时删去严格标志和三项声明证据来降级。scalar比较使用单元素List封装，避免QJsonDocument对不同scalar均返回空文档；旧container比较和schema1／2 hash合同不变。所有值匹配仍不证明实物、当前能力或正式执行权。

直接复读Guided真实入口与Panel的闭4／5／8输入、字段存在性、原序规范化、完整预览、原capture／ticket／source尾核及独立当前审阅。perform在验证产生反馈通知前保存lifetime，新增TextArea在属性通知后先核存活再使旧审阅失效；不新增自动请求或飞行命令。原ID如果被明确保留在新执行集合中仍是合法替补目标，交接说明已作集合限定，不把所有旧机号一概拒绝。

作者自读与作者外对上述新增链的独立静读均已完成，限定所读新增范围未见确定剩余源码缺陷；不是构建或运行无缺陷认证。[17既有路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[Planner合同](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)、[Panel合同](../../custom/src/Swarm/FleetMissionGroupRemainderPanel.md)。本批4源码／接线＋13文档，无新增文件、模块、资源或依赖。原32架、4096点每路径、32768全备用生成点及原字节预算保持。

冻结后普通文本核对：17路径齐全且与manifest一致，48场景连续并全部未执行；13文档5481个本地Markdown目标存在，无冲突标记或行尾空白，scoped git diff --check无诊断。本范围8条未跟踪路径的NUL空白检查无诊断，不能将8当本批新增文件数；本批无新增路径。这些只检查交付材料，不执行规划器、解析器、业务、QML、认证、SQL或hash实现，不代替工程验证。仅地面站源码及文档修改；未编译、测试、仿真、操作硬件／PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，约40%／60%仍只是源码工作量粗估，不按本批路径上调。下方为历史。

## 511：实际begin提交与已发送事实的静态审计

直接静读原生Observation的begin／完整TaskModelEvidence／private updateTaskModel、原ReadContext与所有更新调用者，并对照Vehicle真实begin caller、发送前围栏及actualwrite后commit路径。新begin只在原模型接纳且安装后的上下文、serial、对象与同queue仍匹配时true；内部Target引用不跨Link虚调。失败保已installed began，不恢复旧队列或重试。

原8处更新调用保持忽略返回，只有新增begin消费bool；actualwrite后commit先保本地调用事实再更新模型，更新失败或嵌套退休不能回撤。没有修改Vehicle发送器、三步事务、公共观察／报告shape、权限或占用释放。不是纯内存无回调；完整Evidence仍能触发Link虚调及嵌套事件。

作者静读与作者外对完整相关链的复读均已完成，限定所读新增路径未见确定剩余缺陷；这是静态结论，不是运行无缺陷认证。[15既有路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[28项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[执行观察合同](../../custom/src/Swarm/FleetMissionExecutionObservation.md)。

冻结后普通文本核对：15既有路径齐全且与manifest一致，28场景连续并全部未执行；13文档5455个本地Markdown目标存在，无冲突标记或行尾空白，scoped git diff --check无诊断。7条本范围未跟踪路径的NUL空白检查无诊断，不能将7当本批新文件数。没有新增路径。以上不是构建、解析、运行或工程验收证据。

只修改地面站源码／文档，未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余不缩减；约40%／60%只是源码工作量粗估，不由本检查点上调。下方为历史。

## 510：原生语义提交到持久历史的静态审计

受保护原生TaskRecord提交后的注册、MODE本地交接、终点候选、连续性异常、源退休和观察关闭，新增独立低频事件通道进入实际BlackBox及其持久归档，不再只停留在内存观察通知。逐包计数／年龄／临时sourceCurrent不作为事件变化；同Run的异常位图保历史已观察事实。

独立FIFO最多512行／2MiB canonical payload、单条4096字节，每turn最多64条；不是堆内存硬上限。溢出保旧事件并累计缺口，原生临时收据只在实际发布期间有效；缺口不归入假任务。Root另记本地接纳／未记录／未知，BlackBox接纳不等于SQL提交或完整历史，持久录制仍默认关闭。

新v3固定12字段脱敏投影保28字段runObservation；原v1／v2、SQL及历史查询schema不变。实际按taskIdHash下钻和两历史面板新增只读摘要；现报告将reviewRequired=false普通转换作为info，异常只作warning人工复核，raw保原事件。日志timestamp是实际入日志的本机UTC，原捕获UTC另保，不对队列事件偷偷排序或改写时间。

[事件合同](../../custom/src/Swarm/FleetRunObservationEvent.md)、[实际日志接线](../../custom/src/Swarm/FleetRunObservationJournal.md)、[25路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[60项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批8源码／接线＋17文档，新路径仅两份模块合同，无新代码文件／资源／外部依赖。

这仍是本地Run观察，不是任务Completed／Failed、远端实例认证、正式结果、占用释放或飞行许可。原25目标与完整执行／可信性能／自愈机载改派／跨站协作／长期运营／真实模型插件六类实质剩余不缩减，约40%源码完成／60%剩余仍为粗估，不按本批路径上调。

仅源码、直接静读及普通文本核对；未编译、执行解析器／helper／业务／QML／SQL／认证／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

本段审计限实际源／暂时发布收据、固定摘要、FIFO／缺口与异步通知、旧Codec兼容、实际Root／报告／历史呈现消费者。作者外阅读覆盖Orchestrator捕获与FIFO、Codec值域与投影、Root临时收据／接纳／通知尾、两个现历史摘要及Report消费者；所报确定缺口均已按当前文本修复并复读。具体包括type／source必须是非null真实QString、gap必须带正累计丢弃，以及精确type但坏来源／摘要只增unverified和无推定机号的info条目，不沿旧generic缺省warning或caller事故旗标制造incident；raw原输入仍保留。

冻结后普通文本核对：25路径齐全且与manifest一致，60场景连续并全部未执行；17文档5443个本地Markdown目标存在，无冲突标记及行尾空白；scoped git diff --check无诊断。14条本范围未跟踪路径的NUL空白检查无诊断，不能把14当本批新增文件数。以上检查没有执行源码、QML、SQL、Codec、hash、解析器或任何运行场景，不是构建、交付可用或安全验收证据。

## 509：受控单流启停与正常状态变化

直接静态阅读范围：Role强制二审action、Session精确purpose和私有typed consume/current/pure/revoke；Control实际Swarm／Registry／Capture来源、一次基线、原期和prompt；Vehicle tagged action共享lane／legacy互斥、实际mutation、RAW／ACK／timer；Root独立意图与Panel两确认／取消／partial投影。

正常运行语义：实际own action写入之前仍冻结原完整Info／Status；之后仅Info.RUNNING位变化可沿原私有配置代次继续。其它字段／有效性／name／URI等配置变化与ABA均退休原依据；回读复制实际连贯Info／Status，不把旧数据重绑或变新鲜。此处代次是本地证据，不认证无线因果。

本轮交叉静读中已修的确定问题：Root不再把分别生成的Service Capture ID与native Capture Run ID强求相等，Run绑定本Basis的原生ID与来源；审批和面板补原报告状态；active通知后及controller调用返回先核原lifetime／意图／bridge，避免退页或同步销毁后继续审批；Panel销毁后的晚返回清理只用冻结owner／返回ID和局部校验，不再解析死Panel方法。作者自读与作者外阅读分别记录，不以自读代替独审。native增量、服务／Session以及Root／Panel的跨作者末读均已完成已报问题的文本复核，限定所读509链未见确定剩余缺陷，不代表运行无缺陷。

[14源码＋16文档清单](MERIVUS_SWARM_PATCH_MANIFEST.md)与[64项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)对应。冻结后普通文本复核：30路径齐全且与manifest一致，64场景连续并全为未执行，16文档5381个本地Markdown目标存在，无冲突标记；scoped git diff --check无诊断。Vehicle.h／cc的9／13行尾空白计数分别与HEAD一致；15条本范围未跟踪路径的NUL空白检查无诊断，不能将15当本批新文件数。以上不是构建、解析或运行证据。

未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。270索引、Qt／OS／摄像头兼容和物理效果均未证明；完整目标14与原25目标六类实质剩余保持，约40%／60%只为源码工作量粗估。下方为历史。

## 508：实际相机读取入口与原生回包链静态收口

本批是实际人工消费者链：双宿主 Panel→Guided 原意图→独立 fleet.video.stream.capture／实际 Session 与 Role→Swarm 私有服务／Registry 原绑定→Vehicle opaque Run／共享512通道→原始 ACK 与269／270→服务／页面历史快照。不是仅新增建议 helper，也不是远端启停流、视频降级或飞行许可。

主审直接阅读了新 Service h／cc、Swarm 构造／属性／实际归属桥／pri，Vehicle 新 Run／六入口／样本投影、共享通道及 final writer／ACK／RAW／期限／失效与销毁钩子，Root 新投影／六参数来源门／request／cancel／三退休口，以及 Panel 全文／两宿主和资源登记。Service 与 Vehicle 跨作者分别复读；Role 六处独立 action 与本批当前文档亦由非作者核对。静态阅读不能证明构建、协议兼容、线程／Qt实际时序或运行无缺陷。

已识别并修正的跨端问题：

- 已完成 Run 的轮询／同步完成尾不再因 active=false 被上层误报失败；真正失败或取消仍保 partial，不授新发送权。
- RAW 暂忙先核原总期／阶段期，再等待；已 complete 但 RAW 尚未退栈时也不因临时 sourceCurrent=false 误终结，不在 RAW 内调用 OS 或续期。
- ACK、匹配 RAW 与双端终结均通过原 Service 的纯原8秒门，避免剩余毫秒换时钟把接纳窗口向后取整。
- 跨阶段取消即使 Status 尚未发出，也按任一已发 query 保持有界隔离；删除与 release 只作用原当前 UUID。
- Service 首个 Session／OS 外调前安装仅撤权的 pending sticky watcher，保持至正式 watcher 安装与返回，防止准备期 PRIMARY／Role A→B→A 被最终值相等吸收。
- Root 局部 closed20／30／12／15 复制保留有限 REAL32 framerate，不借旧安全整数复制器误拒合法诊断，也不放宽其它模块数域。

UI/UX 技能采用完整 SKILL／quick-reference／pro-rules 的静态规范对照：沿 QGC 主题、PlainText、换行和滚动模态框，保取消、键盘焦点与目标输入错误说明；长确认改短标签加独立完整说明，不修改共享 QGCCheckBox。没有执行技能检索脚本、渲染、预览或辅助技术实测。

普通文本核对覆盖本批28路径、14份文档、manifest逐项范围、48条连续全未执行工程场景、本地 Markdown 目标、冲突标记及 scoped 空白诊断。它只核文本／路径，不执行代码、AST／解析器、业务 helper 或测试；不将累计脏树或历史未跟踪数当本批新增数。本批仅新 Service h／cc／md 与 Panel qml／md 五路径。

最终文本结果：28路径全部存在且与manifest一致，14文档全文5320个本地Markdown目标均存在，48场景连续并全标未执行，冲突标记0，scoped git diff --check退出0无诊断。Vehicle h／cc分别9／13处尾空白数量与HEAD相同；范围10条未跟踪路径的no-index空白核对均退出1且无诊断，1仅表示与NUL不同，10不是本批新增数。这些结果不代表编译或测试通过。

270 的 param2 索引只是兼容尝试；无匹配目标就失败／未知，不回退0／all／广播。ACK与269／270无nonce，本地基线、UUID、物理来源和语义匹配不认证无线因果。complete也不等于两侧此刻同时fresh、实际送流／停流、完整目录、容量变化或控制许可。metadata READ不单因armed／flying拒绝，真实P0／来源／期限仍须有效。

[服务合同](../../custom/src/Swarm/FleetCameraStreamCapture.md)、[面板合同](../../custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[28路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅为源码工作量粗估，不按批次、路径、接口或局部保护数量上调。完整任务执行、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 507 当前：实际RAW相机报告链的静态收口

新增原PRIMARY相机VIDEO_STREAM_INFORMATION269／STATUS270被动账本，经Registry实际绑定一次读取、Policy第五参数和Guided原五秒周期接入既有指挥中心摘要；不发新请求或控制命令。另修上一轮留下的setVideoMode／setPhotoMode实际caller，以及其toggle／reset模式意图衔接。

Vehicle两源的原RAW入口、P0后observer、初始化／中央失效／关闭／移除／析构及纯内存getter已由非作者全文静读；主审另直接阅读新增方法／字段、三probe及discard／wholeRAW围栏。Registry两源和Policy／Guided／Overlay四消费者已跨作者核固定22／4／15／15、wrapper7、localCamera18及Policy24，模式两源由非作者按实际入口／Fact归属／原native map／独立mode代次复读。当前限定新增链未见剩余确定缺陷，不等于构建或运行通过。

静读中修正了Policy把age=-1合法历史错拒为invalid的问题；fresh公式明确age>=0才可新鲜。原sourceCurrent可与单行陈旧／无效／饱和并存，不能偷作双fresh。Info语义／count变化退休旧Status，私存URI／name不外传；matchesCurrentInformation仍只是本地关联，不认证晚到同源Status的远端目录／命令归属。measurementSource保旧缺第五参字符串，新第五参有独立camera-review命名。

模式新入口先持原活Fact与本地模式意图，虚getter／Fact写／Vehicle发送后重核；noop、toggle、实际模式观察和reset入场均撤旧意图。已可能发生写入不回滚；不外推全部旧Fact／相机／参数／存储／网络或渲染生命周期。旧506已知两个模式caller缺口以本批限定源修改为准，非工程验证。

新合同全文与15份文档当前增量按实际源交叉阅读；UI/UX技能的完整SKILL／quick-reference／pro-rules用于静态规范对照，沿既有PlainText、WordWrap、语义主题和原五秒快照，仅补“该次报告”文字，不新增控件／焦点／颜色判据。未运行检索、界面、渲染或辅助技术验证。

普通文本核对：25路径齐全且与本批manifest逐条一致，15文档全文5322个本地Markdown目标存在，52场景连续且全标未执行，无冲突标记。Vehicle.h／.cc分别9／13处尾空白与HEAD原行相同，没有新增；scoped git diff --check退出0无诊断。范围9条未跟踪路径no-index --check均无空白诊断、退出1仅表示与NUL不同；9不是本批新增数，本批仅新合同1file。原10源码／接线的pro／pri及QML qrc／qmldir登记仍在，无新增登记。这些只是普通文本／路径核对，不是程序或测试执行。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

[完整合同](../../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 506 当前：实际相机／视频生命周期静态收口

本轮修改现有CameraManager／CameraControl→VideoManager真实调用链：本地播放意图、原相机／流选择代次、有限接收器恢复、活动飞机切换和通知回调后的对象存活检查；共享QmlObjectListModel补通知后的自身存活尾核与匹配reset。不是新增只读helper，也不是完整Fleet视频降级／受控停流事务。

直接静读旧源确认无条件onStopComplete重启、无热流的新车遗留旧URI、Taisync自递归、重复Info推进选择代次，以及模型通知后已销毁对象仍被访问等路径。本批只修真实现有消费者，不借新helper、epoch或QQ声称飞行／配置安全认证。

CameraControl／CameraManager／VideoManager两源及共享模型均由非作者按最终增量复读；主审另阅读最终消费者与原接收器完成信号／GStreamer停止路径、本地common消息定义和既有工程登记。已定位并修补正常resume推进自身epoch后被旧选择快照误拒、工厂失败误记成功、无心跳的失败发现不退休、普通流切换把thermal计入上界、同名流按标签误选、重复Info反复启动、Link最后强引用释放后未重核来源、QQ变空误作manual源，以及binding耗尽后显式start沿旧车复活等问题。

模型回调中相机／stream独立销毁现先按原发布地址安全化实际raw行，再在owner结构作用域外精确清理；同时覆盖移除A的beginRemoveRows内销毁B而native平行索引已提前移动的过渡。待清理旧标签不能供setCurrentStream领用；同index但实际流对象改变仍通知。独立复读当前限定新增链未见剩余确定缺陷，不等于构建／运行通过，也不认证任意外部raw列表突变或Qt内部线程。

新合同全文与11份全局506顶节、50项工程场景已按最终源码交叉复读。已知旧setVideoMode／setPhotoMode的Fact／Vehicle外调后caller尾仍待后续修补；完整SubtitleWriter／render job／跨线程状态、通用ACK因果和远端受控停流／恢复均未在本批关闭。

普通文本核对：19路径齐全且与manifest逐条一致；12文档全文5242个本地Markdown目标存在，50场景连续且全标未执行，无冲突标记。共享模型全文8处尾空白均可匹配HEAD的15处原行，没有新增尾空白；scoped git diff --check退出0无诊断，范围4条未跟踪文档no-index --check无诊断、退出1仅表示与NUL不同。4不等于本批新文件数，本批仅新增合同1file；原7源码工程登记仍在，无新增UI／资源登记。这些仅为文本／路径核对，不是程序执行或工程验证。

[19路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[50项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[合同](MERIVUS_VIDEO_LIFECYCLE.md)。保留原QML／相机公开入口，不新增UI、资源登记、权限或依赖。地面本地停止、录像停止与远端VIDEO_STOP_STREAMING不同；通用ACK、调整后缓存和本地代次不认证远端停流／带宽下降／恢复许可。完整Qt渲染job／线程、SubtitleWriter和所有旧相机工作流仍未审定。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505 当前：真实日志下载正确性与有限恢复静态收口

本轮修现有LogDownloadPage→LogDownloadController→LOG_REQUEST_LIST／DATA→本地文件的真实消费者：同一目录／批次来源、冻结选择、有界缺块补读、完整写入后位图与去重计数、NewOnly partial及完整刷新／改名后才显示Downloaded。不是另建只读建议helper，也不是完整Fleet控制中心补传。

旧代码直接阅读确认写前置位、负／短写当真、重复包增量、无上限重试、切机未退旧下载及冲突文件名丢目录等问题；本轮限定两源修补，不以新本地token伪造LOG请求因果或远端文件身份。

两源码已由非作者完整静读并按最终增量复核；主审另完整阅读旧／新消费者、现有页面和本地协议定义。已定位并修补：15分钟文件期遗漏RAW准入、文件起点错用批次零点、模型begin／end通知中条目删除及raw指针悬空、从旧VLM字段重建QQ、未发117／119先发END、Refresh清表通知后吸收新飞机、Erase后换源仍自动Refresh、Link虚调后缺读期限尾核，以及rename后仍误称partial的事实丢失。Protocol systemIdChanged亦推进不回绕来源代次；源失效通知不被busy忽略。

最终独立复读确认上述修处和queued nextchunk无递归自排；两源当前未见剩余确定缺陷，不等于构建或运行通过。generic sender缺最终原Run／完整LOG尾栅、无LOG nonce／boot实例绑定、近似size、END无ACK及多GCS并发范围仍明确保留，不以本地令牌／完整文件假称认证。文档及普通文本最终核对另记于下。

新合同全文、11份全局文档505顶节和40项工程场景由非作者按最终两源逐份复读，未见确定矛盾。Downloaded只认本次冻结公告size的本地字节闭合；公告偏小且peer裁尾时可能仅得到完整公告前缀，不证明远端文件取尽或ULog有效。1GiB为所选公告size合计，不是重试流量或长期磁盘留存上限。

最终普通文本核对：14路径全部存在、无冲突标记或尾空白；12文档全文5206个本地Markdown目标存在，40工程场景连续且全标未执行。scoped git diff --check退出0且无诊断；当前范围4条未跟踪文档no-index --check均无空白诊断、退出1仅表示与NUL内容不同，4不等于本批新文件数（本批仅新合同1file）。原Controller h／cc的pro登记、原页面qrc及QML类型／AnalyzeView实例入口仍在，无新增工程／UI资源登记。这些仅为文本和原接线核对，不是程序／测试执行。

[14路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[合同](MERIVUS_LOG_DOWNLOAD_RECOVERY.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或重试次数上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504 当前：单次定向耗时全链静态收口

504新增独立人工单次定向TIMESYNC往返耗时采集：实际主账号／Role／Root原意图→Swarm私有服务／原生Run→真实同车绑定review→Policy及可见面板。最早四秒包含授权核验和后续100ms轮询，不自动采集、重试或续期，不回退PING或广播。

原生两源、Service／Swarm／Role／pri六源、Policy两源和Root／Panel／两host／资源六源已分工直接静读。非作者对native／service逐字段核26／18／5／7与RAW顺序、4秒、nonce、issued和retired事实；Policy由另一非作者阅读新旧入口、同binding与四参桥。最后通知与确认修补均经原独立审阅者按最终文本复读关闭；本次16源新增链路未见剩余确定缺陷，不以作者自读替代，也不称构建或运行通过。

已定位并修补：Panel首retire后重读sourceToken会把旧按压吸收到外部新代次，现固定原full token并重复尾核；确认框true→false→true缺独立代次，现press冻结consentEpoch与true，仅自有清除可恰好+1，额外ABA／disabled／hide退休。Root owner.active通知后立即核寿命／原意图，且拒绝位于try/finally内，防止源变仍活时漏清owner.active自锁。native另补std::function复制／析构后QQ及原slot尾核，单调时钟和不可回绕nonce明确。

主审另定位原Service期限跨层取整／入场桥：native只拿remainingMs另起clock，不能因此在原service四秒后接纳响应。新增private纯nativeResponseCurrent原Run／active／entry期限／Role值门，仅在匹配回显最终写入事实前调用，之后再核QQ／slot／nonce／physical source；无OS或RAW静默要求，不放入终态来源getter。原独审已复读确认不自锁、不误拒complete30秒历史。PROJECT_STATUS措辞同步改为采集／回包接纳期限，不再称只是写授权。

UI/UX技能以本轮完整静态参考影响原生QGC控件、单次确认、取消、文字换行／焦点可见及“该次快照／非纯网络延迟”表述；没有运行技能检索／预览／UI或实际辅助技术。独审与文本核对不是Qt加载、真实协议兼容或实机行为验证。

[31路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[完整合同](../../custom/src/Swarm/FleetLinkLatencyCapture.md)。15份最新文档顶节、新模块全文及44工程场景经非作者直接阅读，除上述期限措辞已修外未见确定矛盾。原25目标仍未完成。

普通文本核对：31路径全部存在、无冲突标记；15文档全文5203个本地Markdown链接目标存在，44工程场景连续且全标未执行。除Vehicle.h／.cc原9／13条尾空白与HEAD逐条多重集一致外，其余范围无尾空白；scoped git diff --check无输出。当前范围13条未跟踪路径的no-index --check均无空白诊断，退出1仅表示与NUL内容不同，不称程序执行成功；13是累计脏树状态，本批仍只新4file。Service h／cc的pri登记各一次，Panel qrc／qmldir及两个宿主使用各一次，文件均存在。以上只是文本与接线核对，不证明构建、可加载或协议兼容。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503 当前：原生接收观察与实际只读消费的静态收口

503将实际PRIMARY／本车默认component的原始解码帧接到独立五秒接收窗，经Registry实际绑定读取、Policy独立消费、Guided原五秒周期和指挥中心只读摘要。记录接收帧数、编码MAVLink字节率与最大内部间隔；不把它们当可用带宽、RTT、丢包率或远端发布率。

原生Vehicle两个源由非作者直接静读，Registry两个源／Swarm由另非作者审阅；主代理Policy／Guided／Overlay也经非作者交叉阅读。核对内容含原生来源／RAW顺序、22／9／7值形、未知与完整零窗、global receipt不归零、容量截断、QML数值桥、四QoS不被接收量替代。静读不代表Qt构建、协议／物理测量或UI运行正确。

审阅发现并修正新Guided调用方在Policy通知之后未复核Root寿命／原提供者的缺口：现冻结plain lifetime与原Swarm／Registry／Policy，native getter、Policy返回、原可选流review、最终快照发布通知后均再核，guard先短路，不能从已销毁Root或新来源续旧周期。原独立审阅者已按最终文本复读关闭；冻结9源本次新增竖链未见剩余确定缺陷，这只是静态阅读范围内结论。

RSSI行为只排−1未知值；同步纠正旧注释，不再声称非3DR unsigned不会变负或停消息会自动过期。其余负缓存只是legacy近似。UI/UX技能采用已完整读取的静态参考，仅影响“该次快照”文字、PlainText及换行；没有技能脚本／预览／布局／键盘／对比度／屏幕阅读器验证。

[23路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[40项全未执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[原生合同](../../custom/src/Swarm/FleetLocalReceptionObservation.md)。14文档最新顶节、新模块全文及40场景已经非作者直接阅读，合同与范围未见确定矛盾；未把任何未执行项列成通过。

普通文本核对：9源码＋14文档共23路径均存在，无冲突标记；14文档全文5169个本地Markdown链接目标存在，40场景连续编号且全未执行。除Vehicle.h／.cc原有9／13条尾空白与HEAD逐条多重集一致外，其余范围没有尾空白；scoped git diff --check无输出。当前范围8条未跟踪路径的no-index --check均无空白诊断（退出1表示与NUL有内容差异，不称执行成功）；8是累计脏树状态，本批新文件仍仅模块.md一条。旧Policy.h／.cc的pri登记和MerivusInternal类型登记仍存在，没有新增资源或注册。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502 当前：可见面板源码静态收口记录

502新增FleetOptionalTelemetryPanel可见界面，在指挥中心和地图规划页复用同一actual Root。操作员显式只读采集→另行审阅统一目标→两项确认后独立账号审批→逐项配置与回读；固定62／74／83，原501实际后端和授权合同不变。不会自动采集、Apply、续期、重试或回滚。

直接静读新Panel、原Root实际projection／request／approve／cancel、同类QGC控件与两个宿主资源接线。非作者已覆盖最终6源码／资源接线，并在pending修补后复读；本次可见源码范围未发现剩余确定缺陷。不宣称Qt类型、布局、焦点、OS提示、引用传递或运行通过。

静读已修：full sourceToken含自身ID／状态不能变化就取消，stable context只取原scope／身份5／双政策9；Config busy含本审阅active，approve／cancel不借它自锁；can*不依赖无通知JS嵌套activeCall，方法入口仍持票据；空Run用安全{}。长按钮覆受限宽PlainText标签，不靠单独wrapMode。capture在首清理通知前持ticket／原epoch，只允许自身一次+1，额外取消／输入ABA停止。retire独立finally持两原ID取消责任，不因通知／首cancel销毁Panel而漏第二个。

另定位pending请求还未返回ID时本地关闭仅等晚返回取消的窗口。已补本次调用同引用cancelState，界面retire／销毁最早置true，Root入场及纯来源围栏只将它用于sticky撤权；不按公开token找他人、不从false授新权或复活。capture自身初次清旧操作仅豁免精确新ticket，late-return原ID取消仍保留。原独立审阅者按最终文本复读关闭此问题。

配置终态Run是缓存快照，不能持续声称sourceCurrent=true；界面已将“当前来源”改为“该次快照采集时来源检查／该次报告值匹配（均非持续核验）”。独立审阅定位的五项问题（按钮宽度、首次清理票据、detached取消责任、pending即时撤销、缓存措辞）均按最终源复读关闭。文档另明确只在内存保存来源identity5／actorId（可含SID），不持久化SID或存密码／可重用认证证明。

[24路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[面板](../../custom/res/Merivus/FleetOptionalTelemetryPanel.md)。18文档顶节、新模块全文与44表已非作者静读，未把未执行项勾为通过。

最终普通文本核对：6源码／资源接线＋18文档共24路径全部存在，无冲突标记或尾空白；18文档全文5167个本地Markdown链接目标存在，502入口齐全，44场景连续编号且全未执行。qrc、qmldir及两个实际宿主的Panel登记／使用各一次，新QML文件存在；scoped git diff --check无输出，当前12条本批未跟踪路径的no-index空白检查无诊断。12是累计脏树状态，本批新文件仍只Panel.qml／.md两条。

UI/UX技能通过完整静态参考影响了原生控件、分步确认、取消、错误、焦点、长文本换行与主题用法；未运行技能检索／预览、解析器、Qt类型检查或UI，未做实际尺寸／主题对比／键盘／屏幕阅读器／OS提示／协议验证。以上文本登记和独审不能证明界面可加载、实际行为正确或软件已通过工程验收。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501 当前：配置链源码静态收口记录

501新增独立双账号的地面可选遥测配置：从未来新成功Capture的三条真实native上下文封存不可序列化Basis，在同一最早30秒内独立审阅／批准，再逐项511→510并最终三项510复核。仅固定62／74／83、同一正int32且精确REAL32间隔，目标不得比任一原正基线更短；不启停流、不降频P0、不改视频或编队速度。

独立审阅分开覆盖Root／Capture／Swarm桥接和Session／Role／Configuration服务；Vehicle Basis／Run九阶段与实际sender／ACK／mutation／timer／RAW／销毁及真实消费者已在最终修复后交叉复读。非作者分别覆盖本批13源码／接线，当前已审可见链路未发现剩余确定缺陷；这不是构建、运行、协议因果、实际频率或安全验收通过。

已修正常二审提示导致Root错误清Role、进而退休原Basis的自锁：只在actual approve owner、原bridge／scope／review／hash、相同primary5和原Role9仍当前且实际account.busy时保留旧Role显示，不赋权、不续期。实际写入仍走独立two-account原生围栏；登出／锁屏／真实身份或政策变化不享例外。

独立审阅另定位并修复write／messagesSentChanged同步销毁Vehicle后，配置服务因原Vehicle QPointer已空无法投影opaque Run、可能遗留ready的requestIssued=false。新增static纯历史投影仅允许dead Vehicle的原actual owner／线程；固定29／9／32保issued／ACK／report／计数，所有来源／匹配／许可false、remaining0，不调用OS／fence／Link或发送。owned510在任何外调／write前先pin真实query；Config start／tick／finish三处fallback同generation／reviewId／Run保partial并OR累积mayRemainInflight，不复活活动状态。原缺陷已由原独立审阅者按最终文本复读关闭。

start仅认领ready、不写包；100ms后推进九个wire阶段。每条至多4秒，510 accepted ACK后至多1秒且只缩，所有阶段与审批共用原Capture最早30秒。慢链路可能无法完成；截止／失败／取消保留已发事实，不重试、不自动回滚。三项最终回读只给reported-values-match，不认证无线因果、实际发布率、带宽或飞行许可。

[30路径](MERIVUS_SWARM_PATCH_MANIFEST.md)、[46项全未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[模块合同](../../custom/src/Swarm/FleetOptionalTelemetryConfiguration.md)。

最终普通文本核对：13源码／接线＋17文档共30路径存在、无冲突标记；17文档全文5125条本地Markdown链接目标均存在，501入口齐全，46场景连续编号且全为未执行。除Vehicle两源旧尾空白外，其余本批路径无尾空白；Vehicle.h／.cc保原9／13处，逐行文本多集与HEAD相同，未清理无关格式。Configuration新h／cc在custom.pri各一次且路径存在；scoped git diff --check无输出，当前15条本批未跟踪路径的no-index空白检查无诊断。15是脏树状态，不是本批新增数；本批新文件仍仅Configuration.h／.cc／.md三条。

以上只核文本、路径、登记和人工可见合同，不使用编译器／moc／qmake／语法解析器，不执行QML／业务／认证／hash／SQL或任何测试，不证明类型检查、OS提示、并发／协议时序或实际硬件效果。static历史修复与正常prompt修复都只按源码可见路径收口，工程场景继续交工程师验证。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500 当前静态范围：独立授权的510／244采集链

[精确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)9源码／后台桥接及工程登记＋15文档=24路径，新增3路径。主审直接读取实际native query／最终sender／RAW顺序与失效、Capture完整服务和实际Session／Role合同、Swarm私有Registry绑定、Root原意图／controller桥接与取消；不执行任何实现。

新增Swarm私有持有的FleetOptionalTelemetryCapture，显式单车62→74→83，实际Session主账号＋独立fleet.telemetry.interval.capture政策＋原Root/controller意图。接纳后100ms轮询才发；原入口固定30s总期，native每项一次510／4s，accepted ACK后至多1s且只缩，必须原ACK与目标244双端齐全。最终write前复核授权、物理来源、完整载荷并冻结receipt基线；三项同原version／transport／mutation／已绑定ledger，已采语义A→B→A不复活。取消保已发／partial事实，先存历史快照再精确释放原query，不宣称撤回或实际频率已生效。

已发现并修补的确定问题包括：未绑定ledger忽略unknown clear而错误重基、基线只在begin而非最终write前冻结、pure context中的weak.lock临时所有权、OS耗时后才开始总期限、begin内部取消导致已发partial事实丢失、Root退役首cancel后缺存活尾核、旧scope历史与新scope准入token混淆，以及准备阶段已有native ID但Root尚未接纳时无法显式取消。最后另补确定可达的RAW嵌套timer：最早入口整栈depth与不依ledger的纯quiescent口，service在任何OS／source snapshot前先核原total/item期限和明确busy，忙只等下一轮，不把暂时不可读当来源失效、不延期；未知／真正失效仍停止。

冻结后独立复读已完成：486非作者阅读全部9源码新链及最终whole-RAW／service重排／Root三处修补；468交叉阅读非本人Swarm两源／Root后台桥接／pri登记、15文档500完整顶节和40行工程交接，并复读准备ID取消与历史scope标记；480另读非本人Capture两源、Role新增分支与Swarm原生绑定桥。主审直接复读本限定实际生产／消费、最终write前fence、ACK＋244双端／timer、首bind／失效、服务全链和原controller归属。未报告本限定范围的剩余确定缺陷，不证明全产品无缺陷、可编译或运行正确。

最终普通文本核对：24路径均存在，15文档500顶节齐全；15文档全文5073处本地Markdown目标均存在（未验证锚点、页面或语义）。无冲突标记；全文22处尾空白仍为Vehicle.h的9处与Vehicle.cc的13处，两文件逐条尾空白原文多重集与Git HEAD一致；其余7源码／接线路径均无尾空白，未清理既有无关代码。不把其它既有脏树与HEAD的历史差异算成本轮改动。限定路径git diff --check退出0；9个本批范围内未跟踪路径另作no-index空白核对，零诊断／异常退出。工程交接40行、序号1..40且全未执行。工程登记普通读取确认custom.pri各一条h／cc，既有qrc仍指实际Guided；没有新qmlRegisterType或qrc。上述只为直接静读与普通文本／路径／空白核对，未执行任何实现、编译、认证、模型或测试。

[40项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行；[完整模块边界](../../custom/src/Swarm/FleetOptionalTelemetryCapture.md)。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前静态范围：真实查询／配置lane与有界旧512生命周期

[精确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)5源码＋13文档=18既有路径。直接读取Vehicle新dispatcher／reserve／sender／raw归属／timer／清理／mutation、InitialConnect与StandardModes实际调用链及原StateMachine同步续作合同；只读当前地面站协议头和已知sender。不执行实现，不读取或修改PX4飞控。

510／511／512现走同一Vehicle级跨component专用lane：外调前预留UUID／不回绕serial，固定原提供者QQ、PRIMARY弱控制块与实指针、transport代次及原固定期限；原nativeVersion直接入队也接入，但保原地面／参数／实际授权fence和单次3秒，不借generic条件放宽。generic初连不要求MVM已登记或握手已完成；版本资料首次锚定与实际transport变化分开，native批准依据不自动rebase。

通用512的RequestMessageInfo由Vehicle持有shared context，ACK与原始回包两端终结前保原lane。原500ms响应timer在queue空之前也核固定deadline，ACK后1秒回包窗口只缩原期限；静默、IN_PROGRESS或重试不使等待无限。原包入口冻结原请求归属，实际回调仍在P0原生围栏之后；消息0合法，不再凭0哨兵判无请求。可选callbackOwner仅是QObject寿命条件，不是角色／账号授权；InitialConnect与StandardModes实际传原owner，未传owner的旧borrowed-data调用者仍自行保证数据寿命。

本车默认component的任意511只在最终actual write前，以同UUID一次预留推进不回绕mutation generation，并全清498三report及接收窗；ledgerEpoch为UUID:十进制generation。writeDepth贯穿write与通知，临界内原包准入／getter不可当前。未发送拒绝不冒称已配置；已可能write的代次／requestIssued不回滚，耗尽保守停止而非重用。498观察15／Registry review21／comparisons6保持，后续只读重采会看到旧基线失效，既有值快照不自动变成实时通知，须新被动资料与新完整5秒窗，不等于实际生效。

InitialConnect的UID／版本／Hash／能力、provider／Link虚调及同步状态续作增加原QObject尾核，Vehicle能力通知之间亦防删除；StandardModes按整份列表推进不回绕read generation，阻止modesUpdated里新读同步失败后旧completion再次发布。原模式内容、DAILY_BUILD和握手步骤语义保持；没有宣称所有legacy调用者整体安全已验收。

交叉静读已修正的确定问题包括：静默waiter无限等待、ACK后提前释放／raw context悬空、回调新请求被旧清理影响、首次VERSION锚定误退generic握手、准备过程中旧244被新context认领、早于P0围栏发布回调、modesUpdated同步新读失败的Bool ABA、IN_PROGRESS后仍重试，以及新结果回调后的命名provider续作寿命。最终冻结后，一条非作者独立复读全部5源码及最后provider／进度／错误通知／清理修补；另一条交叉阅读非本人Vehicle两源、13份499完整顶节与42行交接。主代理直接复读原包最早context冻结、P0后分派、持久provider pin及对应续作、固定期限和progress、showError与prepareDelete。未报告本限定范围的剩余确定缺陷，不是无缺陷或可运行证明；ComponentInformation等未传callbackOwner的借用数据及进程转发出口仍不在本轮完成范围。

最终普通文本核对：18路径均存在，13文档499顶节齐全；13文档全文5030处本地Markdown目标均存在（未验证锚点、页面或语义）。无冲突标记；全文22处尾空白是Vehicle.h的9处与Vehicle.cc的13处，逐条原文多重集与Git HEAD一致，其余3源码无尾空白；未清理既有无关代码。限定路径git diff --check退出0；5个既有未跟踪路径另作no-index空白核对，零诊断／异常退出。工程交接42行、序号1..42且全未执行。只读消费者后续重采才看到新基线，不以旧值快照声称实时失效通知。以上只为直接静读与普通文本／路径／空白核对，未执行任何实现、编译或测试。

Vehicle公共raw sender在plugin前后拒510／511／512旁路，正式请求走专属通道。此覆盖不包括MAVLinkProtocol转发、移动Taisync字节桥、外部GCS或固件内部调频；本批不改变转发策略，不认证实际发布率、丢包或带宽。

本地common协议头822–827明确GET_MESSAGE_INTERVAL510用param1指定流并返回244；512参数2用途须由对应消息定义。不能因共享lane已支持510／512，就假定当前固件支持“512请求244、param2选择62／74／83”；新Fleet主动capture仍需独立窄scope和工程兼容验证。本批未读取或修改PX4飞控。

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

[42项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前静态范围：原244／可选收包窗进入实际只读消费者

[精确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)5源码＋13文档=18既有路径。主代理直接静读新增Vehicle原始入口／完整producer／中央失效、Registry完整DTO校验／一次实际绑定读取及review、custom Guided真实对象／五秒循环；只读原资源alias与既有源码登记，没有执行任何实现。

Vehicle只观察原始PRIMARY的MESSAGE_INTERVAL244及固定NAV_CONTROLLER_OUTPUT62、VFR_HUD74、ATTITUDE_TARGET83。244保原signed间隔：−1已报告disabled，0已报告not-available，正数为报告间隔，低于−1为invalid；未观测是null而非−1。报告fresh与available分开，所有值均非身份／实际发布率／带宽认证。

每流5秒尾窗、最多512个单调接收时刻；原来源绑定满5秒、该流曾收到且最近包不超过30秒、无相关接收栈／探测中状态和窗内容量丢失才可complete。完整窗可以0帧，仅表示本地该5秒没收到；从未收到过不能complete。均值是本地收包数／5秒，最大内部间隔不含窗两端空隙；不推算发送率、链路丢包或停流。

Registry仅从现有friend建立的实际Vehicle绑定读取一次原生快照，前后核记录与绑定，不以metadata／护照／调用者Map替代。null／0仅无提案，正间隔须int32整数且精确REAL32；三条当前新鲜正基线都已知、提案间隔均不小于原报告间隔，才给数值不增频结论，仍snapshotOnly且所有许可false。

已在源码阶段修正：Guided未定义_fleetRegistry改实际私有_swarm.fleetRegistry；完整布尔拒typed-null，计数为canonical uint64十进制文本，reported状态／fresh／available和窗口count／均值／gap／年龄互相一致；原生未绑定epoch采用规范非null空文本，合法未知不误作坏DTO。冻结后两路独立静读分别覆盖完整producer／最早四ID接收围栏／中央失效及Registry完整校验／实际绑定／Guided消费，均复核空epoch修复；另一条由producer作者交叉阅读非本人Registry／Guided及13份498完整顶节、36行交接。发现的交接第7项泛称“嵌套四类包”已收窄为受观察流或指向集合的244，其它合法目标只持depth；源码保原忽略行为。未报告本限定范围的剩余确定缺陷，不证明程序无缺陷或可运行。

最终普通文本核对：18路径均存在、13文档498顶节齐全；13文档全文4998处本地Markdown目标均存在（未验证锚点、页面或语义）。无冲突标记；全文22处尾空白为Vehicle.h的9处与Vehicle.cc的13处，逐条原文多重集与Git HEAD一致，未清理既有无关代码。限定路径git diff --check退出0；5个既有未跟踪路径另作no-index空白核对，零诊断／异常退出。工程交接36行、序号1..36且全未执行。普通调用名核对仅Registry一次native getter、custom Guided一次实际review及现有公开只读linkDegradationSnapshots；custom/qgroundcontrol.qrc257已指实际Guided、custom/custom.pri53已含Registry，本批无新登记。以上仅直接静读与普通文本／路径／空白检查，没有执行编译器、parser、helper、业务、模型、hash、认证或线程。

本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

[36项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前静态范围：现有511的本地结果归属及生命周期

[精确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)6源码＋12文档=18既有路径。本批只有GroundStation，含其PX4地面适配器；没有读取或修改PX4飞控仓库。主代理直接静读StreamConfig全源、Vehicle全部新增sender/context/ACK/timeout及原真实调用点、ESC专属context和初始化／handler；没有执行任何实现。

本批修现有511（SET_MESSAGE_INTERVAL）消费者，不接通Fleet自动降频。StreamConfig改非零不回绕requestId、唯一pending、先pop后callback和迭代pump；单次最多64实际callback，失败保possibly changed供后续显式选择／restore。callback副本在私有weak lifetime最终尾核前析构，禁copy/move，销毁后不再访问helper。interval=0仍仅固件默认，不是原配置回滚。

公共511 entry固定原VLM／Protocol／Plugin、PRIMARY QQ与weak控制块及实际pointer、原source epoch和本地UUID；复用构造期持久pin，不从已销毁raw重建。外调后入队前再核duplicate/quarantine，实际发送前核原UUID及完整payload，失败按原UUID重找。ACK只接同原物理来源，IN_PROGRESS不续期；511超时加入原隔离，不新增每个成功终态冷却。

PID与ESC各自一次handler/context消费自己的本地请求，删除原任意live511推进PID／ESC的分支。STATUS5Hz→INFO1Hz、原PID两消息100Hz及手动默认恢复入口保持；拒绝／超时不自动续链。same-source迟到重复wire ACK仍可能歧义：协议不回显messageId，本地ID／UUID／来源核对不是wire因果认证，accepted也不证明实际频率、带宽或安全效果。

交叉静读发现并修正的确定问题包括：持久VLM／Protocol／Plugin pin、编码前后provider寿命、511固定诊断名免rawToolbox依赖、callback后原UUID重定位、PID／ESC context parentless构造及parent尾核、ESC原epoch贯穿、实例setter与PID Fact／错误通知返回的Vehicle存活检查。冻结后独立复核已完成：一条逐段复读全部6源码及所有已报缺陷修补；另一条交叉读非本人作者的Vehicle／ESC来源与parent尾核，并逐份读12文档497完整顶节和36行工程交接；StreamConfig另获非作者全文交叉阅读。交接表第6项已按源码澄清仅正常accepted后续原下一意图，失败按第8项停链，不写成失败自动恢复。未报告本限定范围的剩余确定缺陷，不证明程序无缺陷或可运行；同source重复wire ACK及实际频率应用仍是明确未完成证据。

最终普通文本核对：18路径均存在、12文档497顶节齐全；12文档全文4961处本地Markdown目标均存在（未验证锚点、页面或语义）。无冲突标记；全文检出22处尾空白，分别为Vehicle.h的9处和Vehicle.cc的13处，逐条原文多重集与Git HEAD一致，未清理无关既有代码。限定路径git diff --check退出0；4个既有未跟踪路径另作no-index空白核对，零诊断／异常退出。交接表36行、序号1..36且全未执行。全仓源码普通调用名核对无旧gotSetMessageIntervalAck使用，只有本次requestId结果口及两构造器实际三参绑定；原文件仍在既有CMake登记。以上不是编译、parser、helper／算法／业务／hash、认证或线程执行。

真正的目标14仍需独立实际Session/角色scope与必要二审、当前原生链路证据／期限、闭合optional白名单、原MESSAGE_INTERVAL244基线/回读及收包频率窗口。不能借只读refresh或start/capability凭据，不能照搬Policy的全telemetry.rateHz，更不能降频P0证据或控制协议；本批没有新apply UI/入口／权限／自动飞行动作。

[36项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前静态范围：同次接地参考→完整起飞垂直编码体

[精确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)4源码＋15文档=19既有路径，无新登记／UI。主代理直接静读Contact原绑定、原投影／窗口／两组件、Waypoint原float运算、完整源／Joint对应、凸包外扩、append／环境／matcher；Worker与Domain只核真实消费者及原许可边界，未读取或修改PX4。

TakeoffReference只在原成功applied vertical/support产出，保原from数组、bound contact实际22动作序号、整数rest±tol及同有限pad材料。Waypoint同次Contact.prepare兼取LAND／TAKEOFF：复核原Joint.from投影数组；原GPI整数点只核被旧binary64 enclosure包含，不误作数组强等。full current整体落原窗口才建新体，不clip、不加epsilon；两完整端点盒用原connectionHull取所有Z界，允许编码区间重叠，不假称严格上升或对每输入更大。

仅新vertical/support指定terrain的整个交集可在原finitepad band内例外，使用同原Budget；前三类完整覆盖、protected/body、障碍、top→NAV和Domain不借。旧LAND及名义组件不改；v5／原31/schema1／3／Worker7、NAV计数、2M/2048预算和七false保持，原actualPath真门不删除。

冻结后独立交叉静读已完成：一条复核Contact与Waypoint四源码、Result全库命名字段消费者、Worker原row3／三报告与actualPath真门，以及Domain不转移pad；另一条独立对照Waypoint完整新增链，并逐份读完15文档496顶节和36行工程交接。主代理复读原Contact绑定／投影／窗口、full current/top重算、原Joint投影与整数点包含、完整hull／外扩、append／terrain例外和完整matcher。未报告本限定范围的剩余确定缺陷；这不证明程序无缺陷、数学模型已执行或可运行。新22→22详情只关联动作，不改原Joint未知序号或证明实飞归属。

最终普通文本核对：19路径均存在，15文档496顶节齐全；15文档全文4987处本地Markdown目标均存在（未验证锚点、页面或语义）。冲突标记／尾空白诊断0，限定路径git diff --check退出0；11个既有未跟踪路径另做no-index空白核对，零诊断／异常退出。工程交接表36行，顺序恰为1..36且全部未执行。当前调用名核对Contact.prepare在Waypoint仅一处，原Worker457 matcher／520 build及487 actualPath真门保留；Domain303–312不消费新ref，558–564地形命中仍报违例。这些仅为直接静读和普通文本检查，没有执行编译器、parser、helper、solver／数学、业务、hash或线程。

[36项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前静态范围：原编码top区间→首NAV全帽→原环境比较

[范围](MERIVUS_SWARM_PATCH_MANIFEST.md)2源码＋14文档=16既有路径。主代理直接读当前Waypoint完整源／头、原Readiness公式、Joint名义段／来源校验、Worker实际输入／run／matcher／准入和Contact整数投影；本轮未读或修改PX4。

源限定同次Motion GPI9／wire／policy和PM29；仅TAKEOFF成员额外核MIS REAL32=9＋exactfloat及起飞兼容，独立重算原float32/nextafter编码区间，不信derived上下界。新参考点固定原baseline E7，连接首NAV每个完整帽分量，复用原完整XY hull×全Z及body/support外扩；所有新体走原五类环境和共享预算，没有contact元数据。

模型v4保持31字段／schema1、返回3／Worker7和NAV计数原义；新组件真实计入原总计数／详情，同prepare matcher完整重建。全部七认证／许可false及Worker实际路径真要求保留。此为地面分支条件参考几何，不证明当前landed、动态接受、真正起飞垂直域、平滑器或实际连续轨迹。

独立交叉静读已完成：一条完整复核冻结的Waypoint两源码、原Readiness运算次序、Contact投影／凸包界、Worker实际消费／匹配和接地例外分支；另一条独立查原GPI9／PM29 codec、REAL32前置、Joint同点／序号／路径与原生owner／期限，并逐份对照14文档495顶节和36项交接。主代理直接复读最终新增helper、整组准备和全部首NAV分量追加。未报告本限定范围的剩余确定缺陷；这不证明程序无缺陷或可运行。合法未获原takeoffSequenceVerified的成员只使整组参考不完整，源矛盾／序号错配才失败；两者均无可用几何前缀，不靠旗标伪造地面状态。

最终普通文本核对：16路径均存在、14文档495顶节齐全；14文档全文4946处本地Markdown目标均存在（未验证锚点、页面或语义）。冲突标记／尾空白诊断0，限定路径git diff --check退出0；8个既有未跟踪路径另做no-index空白核对，零诊断／异常退出。工程交接表36行，顺序恰为1..36且全未执行。普通调用名核对原Worker457 matcher／520 build仍实际消费新模型，terrain豁免仍只给LAND descent/support；这只是文本及直接静读，不是编译器、解析器、数学／环境solver、hash、线程或业务执行。

[36工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前静态范围：保护report8→显式文件→独立历史

[范围](MERIVUS_SWARM_PATCH_MANIFEST.md)4源码／UI＋14文档=18既有路径；无新QObject、依赖、构建／QRC登记。实际custom面板与Guided由原qrc 233／257别名消费，src默认副本未改。

新文件codec从有界UTF8 JSON读取，只在schema已知小整数槽恢复实际数值类型，再完整复用record16及引用／候选／组计数校验，重建summary13、固定claims22和正文hash。保存绑定原ID/hash，不重新build；旧full9文件口与原native Int保持。读取检查完整长度／尾部／error／close，不能由此认证存储身份、当前遥测或远端任务。

主代理查Qt官方5.15 [QJsonValue源码](https://raw.githubusercontent.com/qt/qtbase/5.15/src/corelib/serialization/qjsonvalue.cpp)：toVariant的整数分支可返回qint64；初版只收Int／Double会误拒，现已仅在独立codec的已知整数槽增加LongLong兼容，原生nativeInt保持。此为源码证据，不是本项目实际Qt构建或往返运行结果。

独立交叉静读已完成：一条复核新native codec与Guided窄口及其旧接口隔离，随后完整核四源码实际接线、Panel的两步文件确认／取消、snapshot与各代次绑定、单view存活及导入来源；14文档494顶节和36项交接另逐份对照。主代理亦直接读新增codec／文件方法、两层调用及最终Panel动作／控件／四dialog与原QGCFileDialog本地路径信号。初读发现并修正JSON LongLong表示兼容、取消回调冻结意图变量遗漏，以及清旧同意时的通知重入可能被新路径吸收；后者现只接纳本操作自有一次清false增量，发布／返回／暂存路径均要求原consent代次和默认false，复核未报告剩余确定缺陷。此为限定静读，不证明程序无缺陷或可运行。

最终普通文本核对：18路径均存在、14文档494顶节齐全；14文档全文4900处本地Markdown目标均存在（未验证锚点、页面或内容语义）。冲突标记／尾空白诊断0，限定路径git diff --check退出0；9个既有未跟踪路径另做no-index空白核对，零诊断／异常退出。工程交接表36行，顺序恰为1..36且全未执行。普通调用名核对对应Panel→Root五参保存／两参打开→native四参保存／单参打开；这不是编译器、moc、解析器、业务、hash、Qt或界面执行。

主代理完整读取ui-ux-pro-max及两份参考，仅静态应用明确确认／取消、来源／未知说明、QGC原控件／主题与焦点规则；未运行技能搜索或界面，非数据库匹配、布局／读屏／对比度验证。[36工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前静态范围：实际记录→跨Run元数据→同hash首页

范围[6源码／UI＋14文档=20路径](MERIVUS_SWARM_PATCH_MANIFEST.md)，均既有路径，无新QObject／工程资源登记／依赖／PX4修改；实际QML在custom qrc既有alias中，未改src默认副本。

直接静读的来源链为原完整catalog→全ordered descriptor manifest→新Reader独立search8／result34／row10→Archive移除私有目录身份的public36→facade／Panel显式首批／续批／选命中／原hash打开整个单file首页。旧action6／options7／receipt11、page request11、原页和位置／报告结构保持；搜索msgid不悄改页筛选。

130MiB读取预留由两轮全manifest header及可扫file两次size组成，≤64file行／批、≤64MiB可扫file、≤1MiB结果；next按最后检查file推进，零命中不会停在原index。普通坏file未知，后段坏行不能返回前段命中，已知来源变化整批stale；取消无部分成功。current getter不读盘／hash；跨批不认证旧内容永恒不变或跨file原子。

独立交叉静读已完成：Reader两源码与Archive实际接线分别复核，另一条完整覆盖六源码／UI的来源、游标、预算、双终结和通知尾核；14文档493顶节与36项交接另逐份对照。初读指出的二次hash已观察大小变化、读出原范围外字节及header来源变化三类路径均已改为整批stale并复核，不再降为单file未知；UI普通picker A→B→A和隐藏后的选择复活亦已修复。限定静读未报告剩余确定缺陷，不证明程序无缺陷或可运行。

最终普通文本核对：20路径均存在，14文档493顶节齐全；14文档全文4891处本地Markdown目标均存在（不验证锚点、页面或内容语义），冲突标记及尾空白诊断0，限定路径git diff --check退出0。其中11个既有未跟踪路径另做no-index空白核对，零诊断／异常退出。当前工程交接表36行、编号1..36齐全且全未执行。以上只为普通文本／路径／空白核对，不是编译、解析器、业务、hash、Qt或UI执行。

主代理完整读ui-ux-pro-max技能及必要参考；用户禁止运行优先，未执行技能搜索、QML／JS、渲染或辅助实现。使用通用交互规则与现QGC控件／theme，不宣称数据库匹配、视觉／可访问性或性能验证。[36工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；原25目标／六类实质剩余保持，约40%仍只源码粗估。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史审计。

## 492 当前静态范围：原live检查→对象尾核→同代快照／记录

范围[7源码＋15文档=22路径](MERIVUS_SWARM_PATCH_MANIFEST.md)，无新文件／UI／资源／工程登记／依赖，仅限定的执行观察lane；其它Vehicle telemetry／sensor lane不因此自动被称为完成同类修复。

直接静读确认的修改：

- Vehicle原_commonInit在VLM创建后一次pin原Links／Protocol／MVM／Toolbox四QQ；后续先核原QQ再比raw，不在下一次入口从失效raw重构。三项live虚调各自尾核原对象／线程／PRIMARY控制块及实际指针／版本／slot；bind接线本地stage，失败只清局部，旧slot先清后disconnect；RAW／recheck只退休其冻结旧epoch。实际write后captured commit不加新的live门。
- Run构造一次pin原app／Toolbox／MVM；native source与Binding两层用owned Target、ReadContext及共同busy保护，回调后先QQ／同代再定位原queue行。整组current／Evidence的行和summary来自同份targets；serial不回绕，耗尽仍保历史commit。prepare连接本地stage并核每个Connection；析构使用本地连接副本。detach不以任意serial推进提前放弃后续旧slot，允许合法late commit但不清新绑定。
- Txn queue lookup固定原observer与owned targets；ready返回后核原QQ／generation／epoch／期限及commandContext。关闭context分离RetirementMatches外调，返回后核原Txn／Run／Orchestrator和原targets，wrapper及stop／drain相关调用者同样先QQ。
- Orchestrator三处已有记录路径在Evidence前复制全TaskRecord并退出旧QHash指针scope，返回后核QQ／线程、重找同id、比全24字段。register本地暂存destroyed连接，connectNotify后复取六项同值Evidence，再重查registry查重／容量／id／占用并发布；原max修订闭合／owner销毁及closed后晚commit不变。

本批独立交叉静读已完成：一条覆盖Vehicle／Run／Txn／Orchestrator七源整链，另一条分别复核Orchestrator完整24字段／接线注册与Txn／Run生命周期，另有15文档492顶节与工程交接对照。当前未报告剩余确定缺陷；这只代表该限定静读结果，不证明程序无缺陷。初读指出的detach合法同queue commit推进serial后漏清后续旧slot已修复并复核；主代理核出的PRIMARY实际指针及staged Connection有效性也已补回。工程场景33另修正文案：私有代次耗尽但未关闭的Run仍按真实commit派生busy以保占用，不要求一律清busy。

最终普通文本核对：22路径均存在，15文档492顶节齐全；15文档全文4913处本地Markdown目标引用均存在（未验证锚点、页面或语义）。冲突标记0；全文件扫描的22处尾空白均为Vehicle.h原9处和Vehicle.cc原13处，与HEAD对应文本及数量相同，未做无关格式改动。限定路径git diff --check退出0；其中10个既有未跟踪路径另做no-index空白检查，零诊断／异常退出。工程交接当前表36行，编号1..36齐全且全为未执行。以上只是文本／路径／空白核对，不是编译器、解析器、业务、模型或Qt执行。

[36工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。QPointer不能证明整个程序不存在use-after-free，文本无错误不证明Qt类型／moc／线程或回调行为正确；本批不认证物理轨迹、远端实例、控制权、StartPermit或正式完成，不改原25目标及六类实质剩余。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史审计。

## 491 当前静态范围：LAND原生已绑定参考→完整接受体→环境消费者

范围4源码＋15文档=19路径，无新文件、UI、工程／资源登记、依赖或PX4修改。主审已读配套PX4的适用AGENTS及三份必读门户／架构／来源文档，再只读mission.cpp、FlightTaskAuto.cpp和navigator动态接受分支；该目录git status报无Git仓库，故只将当前文件视作本地源码参考，不声明精确构建／二进制身份。没有运行其索引工具、解析器、构建或实现。

实际支持合同的LAND与前NAV同E7 XY、前高后低，原名义中心线已经垂直；真实漏项是非零接受区。PX4 MOVE_TO_LAND按实际水平距离条件插入，高度取有效POSITION目标或当前global，不按LAND p7；直接LAND保XY目标且Z为NaN并给下降速度，不能据此强加先水平后纯垂直的顺序。高度接受还可能被内部pos_ctrl_status放大，因此仅作声明静态入口参考，不认证真实可达域。

ContactModel新LandingReference只在同次已成功应用的LAND support组件生成，保原vehicle／sequence／segment／精确rest上下界及同有限pad／terrain；components仍segments×2、旧身份／应用／诊断和JSON不改，failure返回空引用向量。Waypoint仅实际LAND才核原ActionPhaseReview分支，一次同输入Contact.prepare取完整绑定，独立核Joint来源／目标三轴。缺schema2／contact／兼容分支或入口slab不全在rest之上，整份不完整或失败、无几何前缀，不拿p7／默认ground或clip补齐。

每个原cap分量构造entry完整XY hull×完整入口Z，以及同FULLXY hull×rest到原入口顶的descent，分别膨胀body／support，实际进入原三类覆盖和地形／障碍接触。仅descent/support在原指定terrain且完整query∩terrain被原有限pad包含时可豁免，Unknown／部分包含不行；entry／普通NAV／body／障碍／StartupDomain无借用。旧索引完整面／原序／terrain全Z、work2M／fragments2048／取消和2MiB保持；Contact.prepare有额外有界成本，无性能结论。

源码实现者互相核原native生产／消费，独立审阅者再读四源和原Worker／matcher。主审另核新模型名、完整cap／Z、旧组件计数、有限交集及原动态许可边界；直接静读不是编译、类型／链接或模型执行验证。新v3保report31与Worker result7，matcher沿同prepare重建完整几何计数和来源，旧v2不能冒本版；actualPath等原false保持。

文档终审覆盖共同11份当前段与4模块当前段至历史分界，另核Contact Result说明及交接1–36／manifest全部19路径。模型范围、完整原来源、无fallback／无前缀、有限接地、原预算／prepare成本及原false边界均与当前代码对应，未报告确定不一致。一处modelsFingerprint遗漏疑问经当前源码精确比较条件复核已撤回，没有新增重复条件；这不是完成运行验证或消除全部潜在缺陷。

最终普通文本核对覆盖4源码＋15文档=19路径：缺失0、冲突标记／行尾空白问题0；15文档4825个本地Markdown目标存在，断链0，顶部491缺失0。限定19路径git diff --check无诊断；本批11个未跟踪路径逐一git diff --no-index --check无诊断或异常退出（差异exit1且无诊断不视失败）。未跟踪不等于本轮新文件。该核对只证明普通文本／目标路径存在及空白，不证明链接锚点、语法、类型、moc、线程、数学实现／性能、运行或飞行安全。

交接顶部1–36全部保持【未执行】；原25目标与六类实质剩余保持，整体约40%仅源码工作量粗估，不据参考体／路径数抬验收率。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前静态范围：原29参数贯穿末端与完整索引覆盖

范围8源码＋16文档=24路径，均现有文件，无新UI／资源／依赖／PX4改动。直接静读实际Txn取源及唯一capture、MemorySeal私有state／copy／current、PM codec／29行memory gate与Vehicle最终queue链；不是调用任何这些实现进行验证。

原快照schemaVersion=1、exact6、12＋2＋14＋1行且每行14个标量，Seal重建map／list及QString buffer，无JSON往返。Target的原PRIMARY控制块只在最初固定，入口即核原参数，各stage及最终commandContextCurrent再核同seal，不能在晚回调后漏过值／来源／Fact／contentRevision／pending／类型／±0或年龄变化。等值新receipt允许原语义年龄更新，但原事务／审批及phase截止不续期。实际write内部后发变化不构成可撤回已commit或写后仍获证明；后续步骤仍需新一轮完整gate。

Geometry先核完整query与私有有效index，再以bounds取唯一原序候选，按index-owned原CCW轴maximum及上下Z恢复闭面，原完整query交旧coveredByUnion。严格分离成员与query无交，剔除不改变集合覆盖关系；触边／低维／strict及空query语义保留。Domain与Waypoint各三处真实消费者，所有scene先准备，terrainHorizontal独立±51M全Z，原contact索引／窄相保持。准备／候选／重建／旧union共原work2M／fragments2048及token，新增开销可能更早Unknown，没有性能或能效证明。

Geometry实现者独立读两个消费者；消费者实现者与另一独立审阅者分别读MemorySeal／PM29／Txn／Vehicle最终queue链。当前直接静读未报告确定新增缺陷；主审另核原codec6／29、唯一caller、全组尾链及完整面／地形索引。此结论不是语法／类型／链接、数学执行、差分、线程或飞行安全验证。

文档独立终审覆盖当前几何三模块、24路径manifest及36项交接，发现Waypoint说明将跨两消费者的六处调用误称单component六类操作；已改为“全部环境操作”，实际为三类覆盖与两类接触。另一审阅者核MemorySeal全文、Txn当前节、36项及共同入口，六参／原source／schemaVersion1／same receipt与TTL／写前边界与源码一致；交接第16项进一步明确RAW深度由Seal核，不归给PM29单方法。封条旧接口代码块同步为必填六参，参数快照始终写schemaVersion1／6键而非版本6；原历史正文的RAW失效说明已区分参数同值receipt语义。

最终普通文本核对覆盖8源码＋16文档=24路径：缺失0、冲突标记／行尾空白问题0；16文档4826个本地Markdown目标存在，断链0，顶部490缺失0。限定24路径git diff --check无诊断；本批16个未跟踪路径逐一git diff --no-index --check无诊断或异常退出（差异exit1且无诊断不视失败）。这是存在性／普通文本／空白检查，不验证链接锚点、语法、类型、moc、线程、数值实现、性能或运行安全，未跟踪不等于本轮新增文件。

交接顶部1–36全部保持【未执行】；无新文件、UI或工程／资源登记。不把hash／候选／参数匹配当身份／真实路径／许可认证。原25目标和六类实质剩余保持，约40%仅源码工作量粗估。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前静态范围：私有保护记录→独立报告→实际面板

8源码／UI＋15文档=23路径，仅新模块说明1份，无新代码／资源登记。直接对照原TaskModelEvidence20／销毁21、成员25、候选26／20及原完整Run报告来源后，新增独立catalog6／row13、请求4、record16、static构建器返回3／report8／summary13及对外reply4+原生jsonText；不伪补full current字段、不放宽旧文件／handoff合同。

原生来源核同一次关联的实际Orchestrator/sourceId，轻元数据token含修订／closed／death／exhausted，构建和nativeJSON后两次重抓完整record；不读owner/current刷新、不把max修订当唯一版本，不新增控制、占用释放或身份认证。完整payload成员／引用顺序及负MISSION_CURRENT原报告与旧seen分离均静读核对。

实际UI只按本次明确选择／确认采用一份native JSON解析值与原文本。独立审阅发现并已修复：选择通知可重入后旧row贴新epoch；无现票据清理可重入新选择再外写旧消息。现选择与reset先持票据、冻结代次并逐通知尾核，reset消息在票据内、释放后不写；capture收尾持票据跨busy通知。旧full报告槽和文件接口不接新type；Loader空引用和只读／PlainText边界保持。

Orchestrator／Controller与Report两组实现者互相静读真实producer和新消费分支；独立审阅者再核六后端源／头、Root及Panel。已知两处UI通知重入项修后复读未报告确定剩余缺陷；原生文档终审对新合同读到EOF并核全部shape／空值／max／token／双尾栅／旧type边界。主审另核真实代码、Q_PROPERTY／调用链、原none／partial队列及reportJSON对齐。UI/UX指导用于明确空选择／确认、来源与unknown、短标签、PlainText／长ID换行、主题、最小控制高度及键盘焦点；没有运行技能搜索、UI、布局、键盘／读屏或对比度验证。这些不是运行、性能或安全证明。

最终普通文本核对覆盖8源码／UI＋15文档=23路径：缺失0、冲突标记／行尾空白问题0；15文档4741个本地Markdown目标存在，断链0，顶部489缺失0。限定23路径git diff --check无诊断；10个本批未跟踪路径逐一git diff --no-index --check亦无诊断或异常退出（差异exit1且无诊断不视失败）。新增仅模块md1份，无新C++／QML文件、资源或依赖登记。交接顶部编号1–36全部保留【未执行】。文本检查只覆盖路径／内容与空白，不证明链接锚点、语法、类型、线程、JSON／hash实现、资源加载、布局或运行安全。

[36项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25目标／六类实质剩余与约40%源码工作量粗估保持，不按接口／路径数提高验收比例。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前静态范围：序号回退隔离及NAV凸包棱柱

本批2源码＋13文档=15路径，无新文件或登记。对照实际payload受控命令集合（speed／waypoint／takeoff／hold／land、无jump），核任务序号回退的原reportedMission保存顺序、安全计数、tracking门、terminal／airborne引用及同记录投影／报告。回退只结束该target后续关联，原候选、真实queue commit、占用与其他target保留；不推断回退原因，不新增控制或完成认证。

几何核对范围为最多8个端矩形角点、整数monotone hull／退化点线、gcd精确法向、完整Z、body／support扩bounds及所有非轴向面。三个union消费完整query；索引AABB只粗筛，带非轴向面的接触追加同份规范scene行的完整polygon／altitude平面，由原feasible检查。原共享2M work／2048 fragments、Unknown／取消和报告来源重建均保留；模型名v2区分旧AABB结果，不宣称真实平滑器轨迹或性能提升。

两项源码分别由实际实现者与独立审阅者静读；主审另核真实payload命令集合、回退前报告保存／原失效链／poll及getter、整数凸包朝向与退化、面外扩界限和原场景行绑定。复审建议保留无额外面时的原indexedSAT快路，并省略与implicit bounds同等的纯轴向边；已落实，非轴向面仍完整参与feasible。最终源码和对应交接文档的独立静读未报告确定剩余缺陷；这不是数值执行或性能通过证据。

最终普通文本核对覆盖2源码＋13文档=15路径：缺失0、冲突标记／行尾空白问题0；13文档4701个本地Markdown目标存在，断链0，顶部488缺失0。限定15路径git diff --check无诊断；7个本批未跟踪路径逐一git diff --no-index --check亦无诊断或异常退出（差异exit1且无诊断不视失败）。Waypoint头文件与本批前文本相同；无新文件、登记、UI或依赖。交接顶部编号1–32均保留【未执行】。这些仅为文本／路径及限定直接静读，不证明语法、类型、锚点、线程、模型／数值实现、运行安全或复杂场景性能。

[32项交接场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25目标／六类实质剩余与约40%源码工作量粗估保持，不按本批路径或两个修复提高验收比例。未编译、运行模型／helper／解析器／业务／应用／认证／SQL、测试、仿真、改PX4、硬件操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 487 当前静态范围：历史观察资料报告实际消费者

6源码／UI＋15文档=21路径，新文档1；没有新源／资源登记。直接阅读原任务Builder及旧shape/view/export/load、486 MotionReview／Archive真实原页和实际两个宿主入口后，新static只生成独立dossier21；旧任务reportId/boundaries不进入新类型，本链不调用旧build，旧view/export/save/load不直接接纳新资料；旧build通用Map合同未新加类型门。不覆盖飞前missionReport或实际Run结果。

重点静读：内部只一次MotionReview，完整31／来源6／覆盖8，真实聚合样本和资料质量≤16，四视角6／section3；canonical正文去dossierId/contentHash后SHA256，不读时钟／更新_lastReport；整体1MiB不截31。Archive严格shape／hash／原页与来源尾核、双缓存一次通知、共同getter和成对清理；UI一个快照持两者并沿原确认／代次，Loader空来源保护、只读可复制文本与视角非权限说明。

主审要求四视角消费实际计数／状态，不只静态说明；已补actual聚合及non-first-page发现。独立交叉静读未报告确定剩余缺陷；15文档及34项矩阵终审指出旧build泛Map并非类型拒绝器，六份说明已缩窄为新链不调旧build、旧view/export/save/load不直接接新dossier；修订后主审核对当前范围，未见确定剩余矛盾。这不是运行证据。UI/UX用于PlainText换行、主题、短确认标签、明确位置派生及分享边界、无自动读取／保存／发送；未运行技能搜索／QML／布局／对比度／键盘／读屏。

最终普通文本核对：6源码／UI＋15文档=21路径，缺失0、冲突标记／行尾空白问题0；15文档4680个本地Markdown目标均存在，断链0，顶部487缺失0。限定21路径git diff --check无诊断；12个未跟踪文件逐项git diff --no-index --check亦无诊断或异常退出（仅差异exit1且无诊断不当失败）。普通文本另核唯一static Builder调用和内部唯一MotionReview调用、新readonly属性及成对清理位置，矩阵34行全部保留【未执行】。没有新源码／资源登记，新增仅模块md一份。文本、路径和限定静读不能证明语法、锚点、类型、数值／hash实现、资源加载或运行安全。

[34项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25目标及六类实质剩余保持；整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批路径或视图数上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。

本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前静态范围：实际遥测历史与报告值摘要

本批11源码／UI／登记＋14文档=25路径，新文件5。直接读取原Codec／Reader／Archive及本地MAVLink32/85定义后，新增纯MotionReview并由真实最新双终结native页消费；Root窄facade和指挥中心／Planner实际入口完整接线。旧磁盘格式／Reader／observer不改。

重点静读：exact6输入、options7、receipt11；固定本地目录／预算和native query/serial/cursor；双终结和新query/cancel退役；page36／row29先完整原recordConsistent再zero-fill decode；来源／组件／boot重复回退／mask／坏值分段；report31／group5／segment16无raw／UTC／坐标／epoch。未知-1不是0，不作原因、未reset、性能、任务归属或路径安全证明。

UI/UX技能影响可见单位、PlainText换行、无自动读取／默认无确认、分层位置同意、文字错误／未知、44像素控件、分页和键盘防误操作。独立静读指出并已修正：跨query/options旧确认复用；native通知中隐藏／换输入后仍采报告；提交及异步settle的通知重入收尾。两路原生核心／实际消费者交叉静读及独立UI合同审阅，加14文档顶部和32项矩阵终审，修订后未报告确定剩余缺陷；ACK文档历史限定键名／true与false混写亦已修正。这不是语法、链接或运行证明；未运行技能搜索、QML、布局、键盘／读屏／对比度验证。

最终普通文本核对：11源码／UI／登记＋14文档=25路径，缺失0、冲突标记／行尾空白问题0；14文档4643个本地Markdown目标均存在，断链0，顶部486缺失0。限定25路径git diff --check无诊断；12个未跟踪文件逐项git diff --no-index --check无诊断或异常退出（差异exit1且无诊断不当失败）。普通文本核对新纯helper的pri源／头各一次，新组件qrc／qmldir各一次、指挥中心／Planner各一处；486矩阵32项全保留【未执行】。这些结果只覆盖文本和路径，不证明语法、锚点、类型、资源加载、文件读取、数值实现或运行安全。

[32项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25目标及六类实质剩余保持；整体约40%／剩余60%仍仅源码工作量粗估，不按本批文件或入口上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。

仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前静态范围：区域任务三态性能政策

本批9源码／UI／资源＋19文档共28路径，新组件qml／md两文件，qrc／qmldir两处登记。直接阅读范围：Draft新optional actual Bool／schema5 iff presence／set与clear CAS／load与真实task；Compiler中央source重建计数及逐键匹配、合法area false保plan；Store sourceTask／plan presence和值精确绑定；Root原owner/revision／selection与集中source版本门；新编辑器ticket／epoch／选择确认／收尾；Planner pending／互斥／预览／审批／部署门；保存／修复／历史及余段完整政策传递。

旧1..4和未声明不自动补false，source5允许原optional组合但须actual Bool；true仍0.8，合法area false保false且0.7，无source explicit false旧输出不变。政策不是性能证据，原健康／当前源／几何／动态净空／StartPermit边界不改；没有新增授权或飞行发送者。

UI/UX技能用于三态明确说明、默认不升级、独立确认、PlainText／换行、原主题、44像素控件及取消／焦点。未执行技能搜索脚本、QML、预览、布局／对比度／读屏／翻译或键盘验证。新来源预检仅格式对应，中央native校验和原源连续性仍是必要依据。

本批原生Draft链、Compiler／Store链与独立UI／Root合同三路交叉静读，加本次19文档顶部及32项交接终审，均未报告确定剩余缺陷；不是编译、模型执行、UI或运行证明。主审直接读当前源及注册；普通空白检查发现新模块说明末尾一处额外空行，已删除，复核未再产生诊断。

最终普通文本核对：9源码／UI／资源＋19文档=28路径，缺失0、冲突标记／行尾空白问题0；19份文档的4668个本地Markdown目标均存在，断链0，顶部485缺失0。限定28路径的git diff --check无诊断；17个未跟踪文件逐项git diff --no-index --check亦无诊断或异常退出（差异exit1且无诊断不当失败）。普通文本核对新组件在qrc／qmldir／实际Planner各一处，485交接编号32项全部保留【未执行】。这些计数只证明检查覆盖的文本和路径，不证明语法、链接锚点、类型、业务、资源可加载或运行安全。

[32项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。

本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前静态范围：性能声明真实录入、独立审批与一次CAS

本批12源码／UI＋19文档共31路径，无新文件、依赖、登记或资源。对照当前源直接阅读：Registry规范提案／纯stage／拟context与原源typed指纹／一次CAS及独立receipt；Review私有提案和原baseline14、两preview分支／hash／原owner期限；Session closed purpose、OS提示、consume及已消费尾核；Root两入口／intent两map／通知后尾核；Panel输入未知、源／收据归属、预览／确认／普通编辑焦点；Codec／Store／Reader及HistoryPanel新operation/type/hash的完整来源链。

旧四操作、current30/result16／receipt5／approval14和旧preview2/18保持。仅新操作preview3/20；完整基线是硬前置，工程声明不是性能或硬件认证。新数字由483既有实际消费者读取，不新增平行helper或授权来源。普通心跳不清数值，显式外部更新仍保守退役旧批准。

UI/UX技能只影响可见单位／unknown、就地错误、普通编辑不逐键抢焦点、默认取消／两确认、原主题／换行／键盘边界。未执行技能搜索脚本、QML、预览、焦点／对比度／读屏／翻译／布局或实际认证验证。

三路独立交叉静读分别覆盖原子stage／CAS与全层源码接线、Registry最终输入／回执及19文档／32场景、Root／两Panel与真实Review／摘要接口对应；主审另读新规范数域、原owner与source尾核、提交事实、summary预算及实际新旧preview分支。原生第一次来源回调发生在新preview填全之后，Root不会以preparing空预览捕获替代context。当前限定静读范围未发现剩余确定新增缺陷，不等于语法、类型、链接、线程、数值、账号认证或运行行为正确性得到验证。

本轮修正两处实际源码边界：普通未提交输入不再逐键向错误摘要抢焦点，但已有review／call变化仍取消并提示；新OS性能summary固定正文缩短，在ID／两double／caps数／revision／hash的保守最大显示宽度下不超过440字符，低于Session原600上限，不靠放宽原预算接受提案。合法极值和焦点仍列入未执行矩阵。另静读确认普通Swarm刷新不携带这两数字，后续显式外部更新仍会退役旧批准；不声称实测或持久恢复。

最终普通文本核对覆盖12源码／UI＋19文档共31路径：缺失0、冲突标记／尾空白问题0；4627个本地Markdown目标无失效，19份文档顶部均标484。限定git diff --check无诊断；20个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（exit1仅表示存在差异）。仅核路径目标，不验证链接锚点或外部网址，不覆盖业务／数值／UI／认证或实际归档实现；未沿用旧批统计。

[32项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。

本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前审计范围：当前工程性能声明的严格消费

本批限定12源码和19文档，共31路径，无新文件、依赖、工程登记或UI。已对照当前源码直接静读：共享exact11的当前cap批准／完整基线／同context、canonical数值和独立nested存储；Matcher／Role候选过滤；Compiler全任务阈值、成员floor、实际profile及coverage主备消费；Reserve第5原QVariant策略及两实际调用；Energy声明续航×原电量、native direct仅取小及原故障／鲜度；strict planId／0.8标记、Store bool与Coordinator schema3完整source指纹。

原maxSpeedMps／enduranceSeconds已经纳能力工程身份，现只新增严格消费者，不改原approval／identity/hash schema，不把工程声明、构型基线值或本地hash当物理性能认证。`requirePerformanceEvidence=true` 是既有显式task／Intent.parameters政策；false／absent Matcher／Energy／Reserve保旧结果合同。Coordinator材料升级为schema3并总是保新source，不承诺全系统非严格输出逐字不变。未新增性能录入器／UI／Draft.settings字段，区域草稿extra仍不支持。

共享证据失败空map，usable只表示函数得到有效声明，不等于某个数值known或预算可行。Energy还要正续航及原电量；原native battery不安全／过期独立拒绝，不凭metadata drain、untrusted direct或投影fallback补证。P0内部动态状态消息与真实路径缺证保false，未改PX4、MAVLink、ARM／MODE权限或StartPermit。

三路独立交叉静读分别覆盖新helper和Compiler／Coordinator／Store、Role与其余消费者及审批／历史留存、全量严格消费者与交接一致性。主审另核规范数域、原两处备用调用、候选过滤、严格预算与逐机速度、完整身份进入planId及归档。Role严格重复ID已补为连第一份候选一起剔除，空来源只作诊断；false／absent沿旧行为。实际产品归档保完整capabilityReviewEvidence与energyPlan，不靠行摘要代替来源；当前限定阅读范围未发现剩余确定新增缺陷。这不是语法、类型、链接、线程、数值或运行正确性证明。

最终普通文本核对覆盖12源码＋19文档共31路径：缺失0、冲突标记／尾空白问题0；4626个本地Markdown目标无失效，19份文档顶部均标483。限定git diff --check无诊断；21个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（exit1仅表示存在差异）。仅核路径目标，不验证链接锚点或外部网址；没有执行任何业务、模型、数值、UI或文件归档实现，旧批次计数不作本批证据。

[30项工程矩阵](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；未编译、执行helper／parser／AST／模型／业务／QML／应用、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。原25目标、六类实质剩余与约40%完成／60%剩余粗估保持。下方为历史。

## 482 当前静态范围：原29参数尾核与真实NAV接受／连接环境模型

本批对照当前源码直接静读：PM独立arrival集合及exact6／四组稳定行codec、RAMP符号、旧12／14／28合同；Evidence/schema5双稳定身份；Readiness同次原snapshot及三嵌套失效；两Review冻结row4与异步原源fence；Witness原PRIMARY弱map；Worker/schema3 exact8、result7、第三报告及实际准入；新Review的精确接受界、完整邻接AABB、独立动作边界、protected／support和全组共享预算环境比较。这里记录阅读范围，不等于编译、执行或证明无缺陷。

本地PX4仅作为只读源码参照：普通MC高度接受可受controller状态扩大，PositionSmoothing还依赖实时状态及内部航点，固定NAV参数并非完整实际运动域。新报告因此保持dynamicAltitudeAcceptanceProven／smoothingReachableSetProven／numericAcceptanceContainmentProven／actualPathCertified及三权限字段false；原整域actualPathCertified=true仍是共用准入必要条件。静态环境候选、值型matcher一致性、原生来源及最终StartPermit分别处理，不合并认证。

最后补边界已按当前源码直接静读并列入未执行交接：完整NAV纬度帽越±80°整份unsupported而不clip；normal及Ground begin publication后仍启动独立worker以确保finished清理，但PM29变化立即失效／cancel；normal finished／acknowledge publication后及Ground finished的same／fence继续原29tail；Witness弱map复制前先限非空1..32项。同步通知改PM／断链／换component不能保留通知前成功。

两路独立交叉静读分别覆盖新Review／Worker的完整源、整数接受界、连接、扩张、环境求解与报告匹配，以及PM／Evidence／Readiness、两Review／Witness／两Authority的原29来源及回调尾核。独立接线审查发现Ground入队通知后遗漏参数尾核，已补纯memory检查、失配stale／retire且保槽位／reservation至实际finished及drain，并经独立复读。主审同时复核纬度不裁剪、ordinary NAV不继承Contact豁免及通知后的原源检查；当前所审限定范围未见剩余确定缺陷，不等同语法、类型、链接或运行正确性。

Witness的通用native capture不作为任意调用者可变别名环境的认证：当前实际两处私有Review来源均为新建返回证据和私有整份赋值／清理，未见跨fence暴露可变元素引用；其Vehicle／原weak／Model集合在首外部fence前独立持有。本轮结论不外推为通用API防御或物理安全证明。

UI/UX只影响原刷新说明／确认的29项静态文案并保PlainText／换行、默认不选、实际来源和原busy／确认门禁；未运行技能搜索脚本、QML、预览、交互／焦点／朗读／翻译或布局验证。

最终普通文本核对覆盖19源码／登记／UI＋22文档共41路径：缺失0、冲突标记／尾空白问题0；4602个本地Markdown目标无失效，22份文档顶部均标482。限定git diff --check无诊断；30个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（exit1表示存在差异，不是测试失败）。仅核路径目标，不验证链接锚点或外部网址；这些结果不覆盖语法、类型、链接、线程／数值、UI、业务或运行行为。[32项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，旧批次检查不充当本批证据。

未编译、执行helper／parser／AST／模型／业务／QML／应用、测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。没有本批运行产物或安装包。原25目标、六类实质未完成和整体约40%源码工作量粗估保持，不按41路径或新模型报告升级。下方为历史，历史统计不是本批证据。

## 481 静态审计范围：真实整数谓词与同对象域投影

直接静读原Qt/libm混合走廊、理想两帽域、精确binary64毫米拆分、实际Review／Worker／Witness／Txn／Transition消费者。原理想几何不能仅凭末尾nextafter证明未知Qt误差；本轮不是给旧模型补epsilon或改source自报true，而是显式新模型真正加原整数membership后再保旧Qt额外AND。

主审及两路独立交叉静读核完整raw cells／slab、int64边界、反经线／编码padding、失败无前缀、source预算与原Ptr链；新schema2报告的numeric只来自每机同原域完整精确投影和全量扩张，旧／混组仍false。value-only matcher重建与实际native计算分离，guard同member双向绑定。现报告actualPathCertified固定false并由真实准入要求true，未借包含结论打开StartPermit或扩大Contact豁免。当前已审后端范围未见剩余确定缺陷，未运行任何实现。

UI/UX技能影响显式勾选、PlainText可换行说明、默认不选／非法值保留、撤旧确认及同步回调互斥。主代理与独立审阅均直接静读原来源／会话／revision、12项offer及CAS链和busy边界，当前所审新增控制流未见剩余确定缺陷；plain闩在通知解锁返回后才释放，旧读／保存尾清理不能重入覆盖新选择。未执行技能搜索脚本、QML、预览、焦点／朗读／翻译／布局验证。32工程场景全部未执行，不引用旧批次作本批证据。

最终普通文本核对覆盖16源码／登记、23文档共39路径：缺失0、冲突标记／尾空白问题0；4592个本地Markdown目标无失效，23份文档顶部均标481。限定git diff --check无诊断；30个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（exit1有差异不是测试失败）。精确文本确认新增源／头登记各一次、整数API及原Ptr Cover消费者、实际准入的路径条件和32项未执行场景。以上不覆盖语法、类型／链接、模型执行、QML通知或线程／数值运行行为。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480 静态审计范围：起降分支、真实消费与原28行尾核

直接静读本地PX4 Auto／位置控制器／mathlib插值及Takeoff参数语义；源码目录无Git元数据，不认证实际二进制，未运行其工具。新增pure模型按原float32响应宏核原参数、解释ALT1局部max、等阈值台阶、近地加速度／零ramp、LAND插值／crawl／RC以及独立控制器ramp，未知实际状态保持未知。

三集合原28来源检查保旧PM12／14接口、旧12hash及公开19键。独立静读确认Evidence/schema4真实影响parameterEvidenceUsable／Readiness，而不是只挂字段；完整stable动作模型进入身份，所有回调后核原快照，失败同步嵌套usable／sourceCurrent。Contact范围与原起飞exact形状不扩。

独立静读发现派生LAND系数直接套原整数材料上限会误拒合法参数并殃及起飞任务，已改为原操作数＋公式／可材料化状态：大有限系数不clamp或整体拒绝，非finite只阻LAND兼容。近地加速度下溢0也只使TAKEOFF不兼容，不让无关LAND失败；root ok仍只参数合同与宏一致，真实动态未知。

最后补齐MPC_TKO_RAMP_T正负零差别：负零在本地Auto除法产生非finite近地加速度，公开数值为null并显式flag，TAKEOFF不兼容而LAND独立。PM仅对此固定name的账本内容revision、提交／读取缓存对应及原行memory相等检查增加signbit敏感性，不normalize、不改其它参数或旧12hash。独立最终静读已复核三个派生／符号修处及实际消费，当前所审范围未见剩余确定缺陷，不代表运行行为得到验证。

最终普通文本核对覆盖9源码／登记、18文档共27路径：缺失0、冲突标记／尾空白问题0；4506个本地Markdown目标无失效，18份文档顶部均标480。限定git diff --check无诊断；16个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（有差异exit1不作测试失败）。精确文本另核模型源／头各一次登记、新28接口真实消费者及UI28／原profile编辑器12说明；没有运行语法／类型／链接／业务实现。

UI/UX技能只用于真实28项范围说明和确认文字，保原PlainText／WordWrap、目标来源、默认不勾选、忙锁及处理器；没有运行技能搜索脚本、QML、预览、布局／焦点／朗读或翻译验证。30工程场景全部未执行，以上不是语法、类型、链接、线程／数值或运行验收。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479 静态审计范围：覆盖规则、真实14项消费与最终原源检查

直接静读本地FirmwarePX4的参数更新／Auto任务／速度平滑与navigator分支，对读现有12项capture／hash／真实审阅／Readiness／阶段消费者；飞控目录无Git元数据，未运行GitNexus或以目录文档推定实际二进制。主代理新增实际float32逐值比对，不用期望值替换原参数。

独立交叉静读核固定12＋2集合、负／零／正宏、8项原速率、REAL32操作数、完整失败无前缀；发现现有双capture不足以封住后续getter改变早行，已补Evidence最终Version／catalog之后以及Readiness普通finish诊断／phase所有回调之后的原14内存尾核。原PM／Vehicle／PRIMARY弱共享身份与原行不被更新采样替代。

原12hash／UI19键不改，guard材料域维持abs≤2^53−1；纯模型／Refresh观察不扩大准入数值域或认证当前内部状态。当前所审范围独立静读未发现剩余确定缺陷；实际异常回调／线程／序列化和数值行为尚无运行证据。

最终普通文本核对覆盖9源码／登记、18文档共27路径：缺失0、冲突标记／尾空白问题0；4475个本地Markdown目标无失效，18份文档顶部均标479。限定git diff --check无诊断；16个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（有差异exit1不作测试失败）。普通精确文本核对确认模型源／头登记各一次、PM公开签名、guard白名单及collector／Readiness／Refresh真实消费者，不据此推断调用能运行。

UI/UX技能仅影响实际刷新范围说明和勾选文案：静态确认原PlainText／WordWrap、%1／%2来源、默认不勾选和忙锁未变，未执行技能搜索脚本、QML、渲染、焦点／朗读／翻译或布局测量。以上核对不是语法、类型、链接或运行验收。26工程场景全部未执行，下方旧批次结果不是本批证据。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478 静态审计范围：完整原域、编码后几何与历史隔离

直接静读Compiler／Store／Planner／PayloadBuilder和实际PlanManager INT sender及owned部署口。独立交叉审阅核同一原double、INT截断与float32高度、完整坐标命令链、无坐标speed不造点／不断链及全组预算。未运行编码或几何。

静读发现并修复：旧资料strictpayload重放会误用当前严格构建、Readiness单机合成计划缺原域、资料sourceTask可与sourcePlan的policy矛盾、group Run再生single遗留旧group provenance，以及canonical顶点夹带未知树会先序列化才拒绝的资源缺口。最终源码分开3处历史复现与当前normal口、以完整policy逐值对应，并先检查所有规范坐标叶／环预算；不是只加自报checked标签。

最终普通文本核对覆盖10源码、21文档共31修改路径：缺失0、冲突标记／尾空白问题0；4473个本地Markdown目标无失效，21份文档顶部均标478。限定git diff --check无诊断；23个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（有差异exit1不作测试失败）。精确文本还核到Planner三个接口定义、3个历史复现消费者和Deployment／Review／Readiness正常构建入口，以及3处资料policy互核；这不证明调用可运行。独立静读复核已确认最终深树前置预算修复，当前所审范围未见剩余确定缺陷。

以上仅普通文本和源码读取，不覆盖语法、类型、链接、数值正确性、实际IO／线程／通知或安全验收。30工程场景全部未执行。下方477及更早结果是历史，不套作本批通过证据。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477 静态审计范围：退让候选、原域独立距离与source4

直接静读原QGC简单offset与现QtGui登记，依据Qt官方路径API语义选择全部子轮廓，不将fill转换的rewinding桥线当边界。新原域拓扑／全部原边段距、零XY退化、所有分量／端点分配、发射坐标重投影以及共享显式checker不构造候选的主链均独立交叉静读；没有运行Qt或规划函数。

Draft source4、Root前检、原子策略setter及原存储／历史／修复消费者一致。UI静审修复文字发布前旧同意窗口、保存cleanup通知隐藏新输入与取消重入清空新编辑，保留原上下文和可能已提交事实。独立短读已确认修补，无遗留确定问题；不是运行通过。

最终普通文本核对覆盖8源码、15文档共23修改路径：缺失路径0、冲突标记／尾部空白问题0；4372个本地Markdown目标无失效。限定git diff --check无诊断；13个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（有差异exit1不当测试失败）。15文档顶部均标477；12项新增API／实际调用／原存储消费者的精确文本各出现一次，后两项只读既有Store而非新增修改范围。以上仅普通文本和路径核对，不是语法、类型、链接或行为测试。

[准确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)列8源码、15文档共23路径；36项[工程师待执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)无运行结果。正距离仅名义二维；Qt候选、局部投影及后验检查不构成数值误差、量化后轨迹或物理净空认证，原许可false和pending保持。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476 静态审计范围：完整带孔名义域到实际编译消费

直接阅读共享Domain解析／全环拓扑／扫描／两图／全部名义段及桥接、Draft source3与全部旧源消费者、Compiler最终原area及全部主／辅助／备用预算路线。审查不是运行算法或数值证明：边界可沿、无安全退让，原numericfalse／pending／执行许可不变。

独立交叉静读发现并修正：只分段而漏去返桥接、已知声明误置被no-op吞掉、无孔旧显式area不应被新coverage64点限制；UI失败／隐藏残留确认、同步通知销毁与ABA续作及完整settings换源提交。修补后的独立短读未发现遗留的确定缺陷，不代表运行通过。

最终普通文本核对覆盖11源码／资源、15文档共26路径：缺失路径0、冲突标记／尾部空白问题0；4322个本地Markdown目标无失效。限定git diff --check无诊断；14个本批未跟踪路径逐一对NUL做git diff --no-index --check，无空白诊断或异常退出（有差异exit1不当测试失败）。15文档顶部均标476；12项API声明／实际调用／实例／资源登记的精确文本各出现一次。以上是普通文本与路径检查，不是语法、类型、链接或行为测试。

[准确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)列11源码／资源、15文档共26路径；30项[工程师待执行场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)无运行结果。UI/UX技能仅用于静态标签／输入／焦点／同意策略，未运行脚本或界面测量。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475 静态审计范围：联合政策到一次真实新审阅

直接阅读GroupPlanner、Compiler／Store／Coordinator／View及Root／新旧Panel实际链；交叉核共同与逐机政策、标准Compiler投影、备用独立来源、三源互斥及全组显示。已修明确Leader布尔语义、跨面板取消串线、取消取live请求、普通JS ticket无通知导致ready停滞、input／Episode变化旧意图续作；静读无确定缺陷不是运行证明。

最终普通文本核对覆盖15源码／登记资源、18文档共33路径：缺失路径0、冲突标记／尾部空白问题0；4324个本地Markdown目标无失效。限定git diff --check无诊断；21个本批未跟踪路径分别以git diff --no-index --check对NUL核空白，已去除一份新模块说明末尾的多余空行，复核无诊断或异常退出。18份文档顶部均标475；12项工程／资源／实例登记及新增API／调用的精确文本各出现一次。以上仅普通文本核对，不是语法、链接、类型或运行验证。

没有运行parser、AST、qmake、moc、QML加载或任何业务、几何、文件及认证逻辑。[准确范围](MERIVUS_SWARM_PATCH_MANIFEST.md)与28项[待执行交接场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)分别记录源码和运行验证边界。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474 静态审计范围：完整原组来源到实际成员选用

直接阅读新增native整组getter、纯值集合校验、Archive双版本及Root／Panel；核对完整成员与payload／progress映射、partial MODE、整包累计预算、旧格式未知值、原所有权尾核、选择ABA及已有退出链。静态未发现确定缺陷不等于实际功能可用。

最终普通文本核对覆盖6源码、16文档共22路径：缺失路径0、冲突标记／尾部空白问题0；4255个本地Markdown目标无失效。限定git diff --check无诊断；13个本批未跟踪路径分别以git diff --no-index --check对NUL核空白，无诊断或异常退出（有差异时exit1不当测试失败）。16份文档顶部均标474；9项既有工程／资源／实例登记与7项新增API／调用的精确文本各出现一次。以上均为普通文本检查，不是语法／链接／类型／调用行为验证。

未调用parser、AST、qmake、moc、QML加载或任何业务实现，也没有运行文件往返、几何、认证或哈希逻辑。准确范围见[22路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，21项运行场景见[待执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：完整历史源／中央消费者／实际入口的限定静审

主审直接读旧Run取源和报告／summary缺失范围、新Planner完整source抽取与原裁剪后续、新Archive全部序列化／文件／历史转换，以及Root私有slot、原生与历史capture分支、原preview/review和父子UI／文件窗口／退出链。交叉只读审阅确认Compiler、PlanStore、View复用中央provenance，没有另加旧kind硬编码拒历史2；没有用伪request替完整源校验。

静读发现正常流程缺口：收起／隐藏或退出新审阅清掉界面token后，后台历史capture仍保留，换包被阻断却无法真正退出。当前修正保后台retained事实，显式退出冻结实际token并精确clear；清审阅只清同review/proposal/capture关联，回调换新对象不清，尾核检查真实清空结果。退休review和原归档／Run／占用均保持。

修正后的限定交叉静读未发现剩余确定问题；这不是编译、QML解析、文件往返、线程或UI运行证明。本批11源码／登记资源、19文档共30路径，含5新文件。最终普通文本检查：缺失路径0、冲突标记／尾随空白0；本地Markdown目标4199个、缺失目标0；限定git diff空白无诊断，17个未跟踪路径逐一NUL空白检查无诊断。9处工程／类型／资源／实际使用登记均按精确文本出现1次；不是qmake／moc／QML／XML解析或实际模块加载证明。全部未执行案例见[工程师清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472：编辑源／文件效果／回调归属的限定静审

直接阅读Draft新editing与旧strict双入口、Store全源规范化／信封／文件读回／锁内CAS／实际提交、Root私有来源和原generic copy链、Panel完整按下／通知／结果／焦点路径及工程／资源登记。三来源权限边界仍使用原流程，旧可构建要求没有变成任意编辑源接受。

静读发现并修正：restoreSource原成功消费token导致Root重复取源失配，现每次重读全baseline并保同预览；prepareSave在首次通知前冻结调用时expectedRevision；Panel不再把旧owner／Episode结果显示成当前通过，独立contextEpoch含ABA且反馈与原上下文同次发布，已提交事实仍保留。forceActiveFocus后的滚动请求也重新核同原上下文。新增Panel列入qrc与qmldir，不只在文件系统存在。

交叉只读审阅覆盖Draft完整字段／旧strict保持、Store取源／尾核、Root实际重复读源／候选／撤审批／备用发布和Panel归属；修正后的限定静读未发现剩余确定问题。这不是编译、运行、线程、磁盘或UI行为证明。

本批范围：11源码／登记资源、17文档共28路径。最终普通文本／路径检查：缺失路径0、冲突标记／尾随空白0；本地Markdown目标4114个、缺失目标0；限定git diff空白无诊断，15个未跟踪路径逐一NUL空白检查无诊断。9处工程／类型／资源／实际使用登记均按精确文本出现1次。这些只是普通文本检查，不是qmake／moc／QML／XML解析或实际类型加载证明。工程师所有未执行案例见[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471：原对象、源数据独立持有及actual／value分路

本批主审已直接读原Motion factory／canonical验证／contains、两类Review原持有路径、Worker输入与Domain prepare、Witness工厂与Txn接纳。改动要求实际计算持同次原Ptr，值型snapshot／matcher保留临时结构重建但不替实际对象；原模式／schema／预算／许可不扩大。

静读发现并修正输入侧COW逃逸：外部在prepare前持有的元素引用可在浅copy后改变baseline／descriptor，而Qt端点缓存不变。新factory完整校有界schema之后逐层复制容器及文本，原descriptor／baseline／heading均使用独立值。matchesSource复用原完整参数／SHA／null yaw校验而不调用Qt距离或方位；Txn资料分支不prepare替身，无资料旧分支保持。

主审及独立同行已读实际最终API／调用、首次完整提交、后续原Ptr复用、actual finished、地面pending→tick及Witness返回端同集合尾核，未发现仍需修正的确定错配；同descriptor新实例不能由两Review的实际Ptr比较当成原组。源码静读不代替编译或运行验证。

本批实际核对13源码／18文档共31路径，无缺失、冲突标记或尾空白；4140个本地Markdown目标均存在，限定git diff --check无诊断。另对23个本批未跟踪路径逐项git diff --no-index --check -- NUL，无空白诊断或exit>1；exit1只是正常内容差异。只核本批，不为全脏树补成功，准确范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。未执行项目的模型、快照、资料hash、matcher、线程或业务资料；numericfalse及全部原启动阻断保持。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470：自持闭树、完整原行候选与同源SAT

主审重新直接阅读Geometry.h三个新API、完整index factory／稳定归并／候选／indexed SAT及DomainReview实际准备、两candidate循环、失败分流和processed提交。独立同行再次只读核全量验证、整数界、非2幂填充、COW外层独立持有、copy-only状态、根节点快路与同源原行映射，未发现确定性新错配；静读不是编译或运行证明。

Morton仅排叶序，parent保完整XYZ闭包；完整原始行排序不改变明细顺序，同中心项不合并。query全校先于空集／root快路，严格gap不剪触点；候选失败清out且消费者不读前缀。两个index全部成功后才开始component，完整候选后仍调用同一index原凸柱SAT；准备、归并、遍历及SAT均付原同组预算。未将密集场景额外成本掩成通用加速。

本批实际核对3源码／13文档共16路径，无缺失、冲突标记或尾空白；3983个本地Markdown目标均存在，限定git diff --check无诊断。另对8个本批未跟踪路径逐项git diff --no-index --check -- NUL，无空白诊断或exit>1；exit1仅正常内容差异。只核本批，不为全脏树补成功，准确范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。无运行几何、parser、helper、matcher或业务资料；numericfalse和全部启动阻断保持。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469：准备型凸柱、完整闭SAT与整域真实消费

主审直接阅读新Geometry h／factory／SAT全部实现，以及原Budget／整数范围／generic solver、原Input strictCCW来源及DomainReview整段实际准备／比较／失败分流。厂端完整校所有坐标后才算边／投影，私有valid和显式copy不生成move；query拒额外plane并核全Bounds，所有轴双向严格gap与每个确定结论尾核取消。补强复制输出后的取消观察与清out，主审已读回。

Geometry与DomainReview双向同行静读，另由独立只读审阅核qint64支持值、退化／闭接触、terrain下界等价、一次准备及同组budget。实现借鉴公开分离轴数学原理，未复制外部代码；纸面方法、复杂度／work上界不是执行结果。主审已阅读模块所引官方原理文档。

实际接入只替代轴盒×地形／障碍逐对求交，不将一般扫掠体降为AABB，不删三种并集、地形／障碍高度或原接触／数值门禁。准确3源码／13文档共16路径：普通文本／路径无缺失、冲突标记或尾空白，3941个本地Markdown目标均存在，限定git diff --check无诊断。另对8个本批未跟踪路径逐项git diff --no-index --check -- NUL，无空白诊断或exit>1；exit1仅正常内容差异。只核本批路径，不给全脏树补成功，范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。未执行任何几何、matcher、helper或业务资料。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468：五纬度带整数界、地形冗余平面与结果前缀

主审直接阅读Cover选择k／ceil／完整cap纬度／闭接缝实现和纸面haversine下界；同行只读核对五带系数、65°多项式下界及qint64／容量不扩大。DomainReview主审读完整report matcher新前缀条件，保反经线重复身份的最早出现；这只拒绝顺序producer不可能产生的未来明细，不把matcher当环境solver。

已直接阅读Geometry::coveredByUnion完整输入校验／closed member／query Bounds／差集／取消分支，确认只要求query有界；据此DomainReview直接复用全部terrainHorizontal竖柱，删除与query Bounds相同的上下Z面。地形接触及障碍等其它高度面仍保留；实际预算计数可以变化，不声称已测性能。

准确2源码／13文档共15路径：普通文本／路径核对无缺失、冲突标记或尾空白；3903个本地Markdown目标均存在，限定git diff --check无诊断。另对7个本批未跟踪路径逐项git diff --no-index --check -- NUL，无空白诊断或exit>1；exit1只是正常内容差异。同行最后已只读核对实际落盘两源码／两模块文档，未发现确定新接口或合法生产者误拒；这仍非编译或运行结论。范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，算法、线程、报告matcher或业务资料均未执行。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467：完整理想域算法、实际接纳链及接触权限边界

主审直接阅读新Cover完整整数外包／接缝／投影、DomainReview准备／展开／全组环境计算／report matcher、共享geometrySourceValid、Worker生产及匹配、两Review的结果接纳／保留／阻断、Witness工厂及两Authority信封。另完成Cover／solver接口与动作接触语义、solver／消费者合同的交叉静态审阅；不将同行阅读当作编译或运行验证。

确定发现并已修正：初版把原TAKEOFF／LAND限定pad按vehicle／terrain聚合，并给任意整域support box豁免。原contains并不绑定当前动作，即使cell关联LAND腿也不足。已删除该聚合及豁免，保ContactModel上下文诊断，所有确定非空整域support∩terrain交集仍记录候选；旧名义层不改。还修正反经线恰触边界须保另一侧零宽闭cell的外包条件。

当前硬合同为Cover原生Result、DomainReview result3／report28／limits8／违反row6，Worker input8不变、result6。原Qt数值包含固定false，实际接纳要求true；理想模型成功不构造Witness或解除pending。原Qt5.15官方源码半径与本地混合表达式的差异已在[模块说明](../../custom/src/Swarm/FleetMissionStartupDomainCover.md)给来源，不据此认证本机Qt或物理行为。

本批准确14源码／登记、20文档共34路径：普通文本／路径核对无缺失、冲突标记或尾空白；3922个本地Markdown目标均存在，限定git diff --check无诊断。另对25个本批未跟踪路径逐项git diff --no-index --check -- NUL，均无空白诊断或exit>1；exit1只是正常内容差异。核对限本批路径，不执行parser、helper、环境solver、线程或应用，不给全脏树补成功。准确范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466：历史备份导入的限定静态审阅

本批审阅范围为原生真实diag来源、三动作intent7、preview29、Worker import5／38／current27、固定构造目标及回调invocationDirectory、分块暂存／读回／NOREPLACE发布、完整尾核和原实际finished。旧catalog21／24、diag23／26、backup31、move27、preview25／26保持。

主审直接阅读新requestValid／resultMatches、runImport全部前置／分块写／flush-close／读回／publish／后核及current生产，Store私有来源／预览／提交／原目标回调与清值、Guided新七字段和Panel选择／确认／消费后尾核／结果接纳。Worker与Store双向同行静读、Store对Panel／Guided独立交叉均已完成；已读范围未发现尚未修正的确定新接口或正常路径错配，不代表线程、文件系统或UI实际通过。

准确6源码／14文档共20路径，普通文本／路径检查无缺失、冲突标记或新尾空白；3816个本地Markdown目标均存在，限定git diff --check无诊断。另对11个本批未跟踪路径逐项git diff --no-index --check -- NUL，无空白诊断或exit>1；exit1仅正常内容差异。只核本批路径，不对全脏树补成功。

编译、moc、helper／parser／AST／业务脚本／QML／线程／UI／认证、实际目录／hash／SQL／文件导入／恢复／测试均未执行。nativeRenameSupported只提供编译分支判断，Linux kernel／文件系统运行期不支持可能在publish后才观察失败，文档已明确此时保暂存、不做覆盖或copy-delete fallback；未把普通文本检查写成恢复或放行证据。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465：逐文件备份诊断的限定静态审阅

主代理直接阅读Worker候选／matcher／hash-close、Store独立请求与实际线程结束、Guided和面板接纳／显示源码；Worker与Store双向同行静读，Store另只读核对面板／Guided。修正初版row键名分歧后，统一canonical descriptor，不引入双键兼容或有效前缀。
静态核对覆盖request2、Worker23／row4、public26／row5、旧current19只读状态和移动位false；原名／计数／原因／descriptor全量对应，source空分支、原工单取消、实际finished、路径／通知ABA及新操作消耗旧资格。原strict catalog21／24、move27、copy31和preview25／26不变，原Inventory类及资源登记无本批修改。
本批准确6源码／14文档共20路径；普通文本／存在路径检查无缺失、冲突标记或新尾空白，3778个本地Markdown目标均存在，限定git diff --check无诊断。另对11个本批未跟踪路径逐个git diff --no-index --check -- NUL检查，无空白诊断；内容差异exit1不算失败，未发现exit>1。以上只是文本／路径核对，不是编译、解析或运行验证。

在已读范围未发现尚未修正的确定新接口错配；此结论仅限本批静态契约。未直接执行native scanner、实际hash／SQL、QThread、QML、UI、认证、文件复制／恢复或项目helper，全部工程师待验证项仍未执行。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：独立备份目录的限定静态范围

审阅重点为新EventBackups命名空间、backup-catalog请求2／结果21与entry8、Store公开24／entry9／只读新信号及原actualfinished、不隐式访问原源与无需库存的请求分支、UI完整结果对应／原path版本／取消和旧值退休。准确路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

文件结构检查、当前SHA、正常目录EOF及三个UUID不证明原源hash、作者／签名、全部备份完整、灾后恢复或可执行状态。实际SQL／扫描／UI与全部故障反例均未运行，不能据普通静读宣称通过。

本轮主审直接静读新namespace追加、backupListing／descriptor8／request2／result21／runBackupCatalog全部独立分支，Store无源目录／无库存路径、取消前置、完整公开24生成及新通知，Guided原生属性与exact6／7，UI独立sourceReady／basis／按下路径／完整结果／actualfinished与只读列表。Worker和Store双向同行审阅、Store对Panel／Guided的独立交叉均已完成，未在已读范围发现尚未修正的确定新接口错配。此判断只限本批静态接口，不是成功执行证据。

实际实现不预先创建原源目录，读取分支在旧源路径与库存门槛前，目标合作锁／真实枚举／双SHA／只读SQL和后清单才在后台执行；public只从已核actual21生成catalog／entry ID，未添加没有消费者的未来导入占位。新结果两个来源／全集标志固定false，不将filename或本次hash充当463原复制工单。UI补齐与Worker一致的严格basename升序及catalog current19只读状态／三个移动事实false；空源路径另持独立options依据，旧动作仍要求原来源。原query取消和终态接纳不因之后编辑目标路径而丢失，旧catalog的current资格则按原路径版本退休，改回不复活。

最终普通文本读回：8源码、15文档共23路径均存在；15文档的3,740个本地Markdown目标均存在，无冲突标记或行尾空白，限定git diff --check无诊断。范围内14个未跟踪文件逐一no-index空白核对均无诊断，exit1仅表示相对NUL有内容；本批复用已登记类／组件，无新pri／QRC／qmldir项。没有执行目录、文件、SQL、取消、UI或业务反例，不能据此宣称构建、性能、文件系统、恢复／飞行验证或25目标完成。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 463：显式备份的限定静态审阅范围

审阅目标是原库存及source descriptor、备份私有路径预览与不可续期时钟、NewOnly复制／短写／flush-close／完整重开读回、实际源再核、取消部分文件及不确定结果、同一维护线程actual finished、Guided exact6／7与UI路径版本。准确路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

普通文本和API阅读不证明目标目录可写／容量、实际新建无覆盖、Qt／SQL行为、物理备份独立、掉电持久性或QML交互。完整备份／灾后恢复与原25目标仍须继续开发。

本轮主审直接静读新backup请求／结果及状态分支、runBackup全部操作次序、共享helper的专门backup名字开关、Store完整prepare／commit／result消费和UI的路径版本／按下确认／取消／结果接纳；独立Worker↔Store双向静态交叉及UI对Store／Guided契约核对均已回执。旧inventory／move和current19保持，新的backup31/current23不从旧move字段推断写盘。源descriptor和真实目录／全文件hash／SQL seal对应、目标NewOnly、短写循环／字节累计、完整后重开双hash+只读SQL及原源再核都在实际后台分支，不以UI布尔或独立未消费helper代替。

静态审阅补正两处：源读取句柄close后在无先前error时记录独立关闭错误，路径纯语法统一拒反斜线；UI还与Worker对齐已决定时原源／目标目录身份和current23事实关系，不接纳自相矛盾进度。copyCompleted在flush／close前记录，后续失败可保true但backupVerified必false；晚到取消不抹写入或读回事实，已发原工单不会因之后编辑目录丢失。原历史折叠按钮补“备份”以使功能可发现，不改变原查询／busy逻辑。

最终普通文本读回：7源码、15文档共22路径均存在；15文档3,700个本地Markdown目标均存在，无冲突标记或行尾空白，限定git diff --check无诊断。范围内13个未跟踪文件另逐一no-index空白核对均无诊断；exit1仅表示相对NUL有内容。复用原已登记类和组件，本批无新增pri／QRC／qmldir登记。未在本批已读范围发现尚未修正的确定新接口错配；未执行任何复制／读回／取消／路径／UI反例，不能据此证明构建、行为、完整性、性能、持久化或飞行验收通过。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：归档分类维护的限定静态范围

本批审阅目标为实际Worker文件／SQL身份和改名前后观察、namespace不混用、Store私有预览／原期限／单次消费／真实线程占用、全部原生入口互斥、历史源退休、Guided窄门面和维护UI／既有历史busy接线。准确路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

普通源码／API阅读不证明实际改名、断电持久性、不可覆盖的目标平台实现、恶意并发安全、数据库恢复、UI或构建通过。退役不释放磁盘，恢复归档不是恢复任务／许可，完整留存／备份／永久回收及原25目标仍待实质开发。

本轮主审直接阅读维护Worker全源、UI完整请求／确认／结果消费、Guided窄门面、Store关键生命周期及实际资源登记；独立交叉审阅覆盖Worker→Store和Store→Panel／Guided两条链。核对了原生库存／entry、exact1／5请求、worker19／27与Store22／25／20，原30秒单调提交窗口、原private上下文单次消费、完整历史游标退休、所有旧native入口互斥、结果与实际finished分离及多通知销毁保护。合法move-uncertain保完整worker27，系统报告失败但完整后核确认允许reported=false／confirmed=true；cancel不清原工单或丢迟到事实。

本批静态发现并修正Worker正常成功／早期失败中的null QString：成功errorCode与未观察目录明确为非null空QString，current状态同样规范，避免严格text契约把合法结果拒收。UI接纳回执后安装新ownedOperation对象以通知绑定，并在同步通知后补原来源／结果尾核。原生改名避开Qt复制／删除回退；本审阅没有调用这些系统接口或执行任何反例。

最终普通文本读回：13源码／工程及资源登记、17文档共30路径均存在；17文档的3,653个本地Markdown目标均存在，无冲突标记或行尾空白，限定git diff --check无诊断。范围内18个未跟踪文件另逐一做相对NUL的no-index空白核对，均无诊断；exit1只表示相对NUL有内容。custom.pri对新Worker的h／cc各一处登记，QRC／qmldir对新面板各一处登记。未在本批已读范围发现尚未修正的确定新接口错配；这不是编译、线程竞态、跨平台改名、持久化、QML交互或飞行验收证明。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：实际保留任务目录与精确哈希筛选的限定审计

本批审阅范围为Reader旧／新搜索契约、实际payload来源、Store原生批次累计／目录与记录各自身份及游标、固定目录和原真实finished单槽、Guided窄门面与新任务历史面板、资源登记及交接说明。精确路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

检查只限直接静态源阅读、普通文本／路径／本地Markdown目标及git空白；没有运行搜索、SQL、线程、UI或业务反例。静态对照无法证明构建、取消时序、持久化、性能或实际交互。目录内存可重建不等于长期运营或全历史完成；完整任务／StartPermit／正式结果及其余原六类开发仍保留。

本轮已直接核对：真实Codec envelope18／payload中的taskIdHash／category8；Reader request5→result28与task request6→result29严格配对、每条筛选及完整字节核；Store目录35／task row9／记录31的局部staging、固定inventory起点、已见file6身份、独立一次性continuation和actual finished单槽。原普通search仍保全局latest规则。目录／records可按同稳定来源交错续扫，不能仅因JS目录快照更新而自锁，也不能用改回来源／scope／选择复活旧票据。

静态发现并修正了预算UI与原生两条路径的错配：整批拒收保原累计和空当前记录，不按previous+1或正常29键页接纳；恰好8192则保完整已提交批次。新外层完整JSON仍核原2MiB，task-filter Reader单独预留2048，旧512不放宽；UI有限复制节点131072覆盖256×255合法关联集合，而不截成员。queued／uncertain零计数及已观察机号不升级为已知false或完整成员；保留行不充当去重事件或任务成功率。

最终普通文本读回：9源码／资源登记、15文档共24路径均存在；15文档的3,588个本地Markdown目标均存在，无冲突标记、行尾空白或坏链接，指定git diff --check无诊断。QRC／qmldir对新组件各有一处登记；两个新增组件／契约文件另做no-index空白核对，exit1仅表示相对NUL有内容。主审及独立静态交叉覆盖Reader／Store与UI／Guided实际消费者，未在本批已读范围发现尚未修正的确定新接口错配。没有执行反例或宣称编译、逻辑、交互、性能、完整历史、任务结果或飞行验收通过。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：电池明确负面与原始账本末端覆盖的限定静态审计

准确11源码／工程登记、19文档共30路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。限定直接阅读重点：

- 本地MAVLink原头／XML给charge_state offset40、fault_bitmask offset50及枚举含义；Vehicle沿原zero-fill记录真实MAV2扩展known，MAV1／未到字段及charge0 unknown/-1。fault以qint64保uint32最高位，schema2 outer20／row19、原value10，不改聚合估算／源身份／接收深度。
- 共享纯值检查完整旧schema1和新schema2、全部行／类型／known哨兵／来源／ID／序号／别名／推进聚合。用std::floor而非返回int的qFloor校uint32与大年龄。freshUnsafe／staleUnsafe仅全量校验后发布，坏后缀没有成功前缀或虚构evidence。
- 任意fresh同源主／副电池非零fault或非正常charge负面使usable=false；stale负面保未解决unknown而非新鲜故障，不因老化恢复健康。缺扩展／正常状态／0mask都不认证物理健康，schema1旧硬件不因缺扩展统一禁用。
- Registry以valid保完整合法负面，仅改返回副本；HealthCenter在FTC合并前查实际原aliases，再生成check4，不能借旧FTC百分比／metadata清故障。原生阻断ID独立去重并入最终gate，原其它在线／SYS／EST／位置／电量／链路／Leader不跳过。
- Scheduler删除旧匿名解析器，调用共享inspect；主机／指定备用拒绝行仍exact18且保合法来源。原RiskEvaluator／Coordinator／部署／启动健康门禁继续消费真实blocking，不用静态endurance绕过。
- Swarm仍投影合法负面percent以保实际Registry绑定，不以!usable丢掉故障。Guided接schema2健康行和原revision；纯fault／known／fresh变化影响原身份，普通age／同值receipt不抖动。
- 原phase seal此前没有独立Battery账本；现补全部typed值／receipt／revision／token／原接收时刻与epoch／owner／weak／原clock起点。只对capture时已有current/fresh完整行保原3000ms，未绑定／未观察／旧stale不新增必需源；elapsed当前采样不进入快照，不调用getter／JSON／hash／Link虚／weak.lock。新包／ABA不能更新旧seal，最后write原整组内存消费者保持。

最终普通文本读回：11源码／工程登记、19文档共30路径均存在；19文档的3,558个本地Markdown目标均存在，无冲突标记或新增行尾空白，Vehicle.cc的13处与HEAD逐字相同历史空白行保留，普通git diff --check无诊断。新共享检查器三文件另行no-index空白核对无诊断（exit1仅表示相对NUL有内容）。主审和独立静态交叉覆盖原生producer／shared完整成功提交／Swarm绑定／Registry／FTC前后健康聚合／最终阻断ID／主备账单及既有风险消费者／Guided稳定身份／原phase seal与prepared正向路径，未在本批已审范围发现新的确定接口错配。未执行数值／消息／回调反例或业务场景，不据此证明构建、逻辑、性能、实时性或飞行验证通过。可信性能／耗电标定、整个运动域、完整StartPermit、正式结果及原25目标／六类实质工作仍未完成。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：原生同模型传递与阶段复用的限定静态范围

准确5源码、16文档共21路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。限定直接阅读的要点：

- 旧Readiness::capture源指纹本来包含完整stableFlight／GPI9，普通Authority重捕严格核原指纹，v4仅包原hash；没有确认旧链允许静默换GPI，不把明确化所有权夸成已证明漏洞修复。
- Witness原工厂在原input2／result5和native source18／datum14校验后，逐完整source12一次prepare并比较完整规范JSON，按原ID缓存不可变模型。全组成功才发布；getter先memoryCurrent，失源／跨线程／到期／退休／未知ID空，不新采集、重准备或续期。
- Txn新请求用同原beforeArm GPI9／原member规范policy／完整payload／ceiling一次prepare；有资料与Witness原Ptr完整source对应，无资料持本次Ptr。实际owner getter只读原私有Target，不接受QML恢复或latest回退。
- Transition初始与原有序journal的GPI均取实际owner缓存，phase不再prepare；原contains表达式及跨步heading累计保持。原phaseBasis、权限、Witness连续性、整组phase seal与最后命令消费仍独立。
- 缓存模型只是纯值来源，不认证Qt／libm误差或整个域。现工程projection error不是数值库证书，资料maxDelta不是runtime限制，LOCAL／ODOM／HOME无已证共同XY，speed×phase不足以裁旧全路线；不能用这些替代完整域包含证明。

最终普通文本读回：5源码、16文档共21路径均存在，16文档的3,510个本地Markdown目标均存在，无冲突标记或行尾空白，普通git diff --check无诊断。5份本批源码均为既有untracked文件，另行no-index空白核对无诊断（exit1只表示相对NUL存在内容，不是测试）。主审与独立阅读对照普通／地面、有／无资料、v4和JSON数值／null表示，以及prepare一次→整组提交→preparing gate→两条Transition消费者；确认getter不因preparing／checking而自锁，未在本批已审范围发现新的确定接口错配。未执行业务、编译、测试或任何数值反例，不据此证明逻辑、构建、性能、实时性或飞行验证通过。整个运动域、Compiler pending、完整StartPermit、正式远端结果和原25目标／六类实质剩余保持。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：原运动模型共用与原审阅描述的限定静态范围

准确8源码／登记、18文档共26路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。只直接阅读源码和生产／消费合同；限定检查包括：

- 新不可变纯值模型完整校原GPI9、规范政策、原ceiling及canonical12／完整hash，按顺序连接全部地理项，speed不截断路线；缓存不变端点／限值，原Qt球面公式、高度／速度／航向累计及失败不提交语义保持，不引入未经证明的affine门槛。
- 实际Transition初始及每个原有序GPI消费同一prepare对象，不重置原journal航向累计；不是只新增未使用helper。额外typed-null／坏canonical拒绝不当作合法旧值语义变更证明。
- 普通capture不发布startupTransitionPolicy的确定错配已修，从同一原member提前normalize profile，模型与原审阅行共用reviewedPolicy。原同次capture的GPI及原payload／ceiling保留，地面只用原completed READ currentRisk和同一target.payload。
- Worker输入仅schema2 exact8、每机row3必需模型；完整source12重建一致，policy／ceiling／GPI几何对应，完整payloadHash在run和resultMatches两端与Joint完整同序memberCoverage配对，不采用子集／前缀或默认值。
- row3显式投影为Joint原row2，只构造原名义段。完整source进入外层输入hash并由原生Witness保留，但source的numericContainmentProven／clearanceCertified／flightCommandReleased为false，既有环境solver范围不扩大。
- 原Review clock／截止／UTC／source18／datum14、首次基准、取消／actualfinished和各自预算未改。普通完整artifact 2 MiB；地面内部artifact 32 MiB、公开riskSummary 2 MiB，不能统一称内部2 MiB。
- 独立直接阅读覆盖实际Builder规范值、普通policy修正、地面首次READ、Worker配对和Transition调用，未发现本批已审范围新的确定接口错配。没有执行这些反例、公式或业务场景，不证明逐位运行等价、可编译、性能、实时性或飞行可靠性。

最终普通文本读回：8源码／工程登记、18文档共26路径均存在；18文档的3,445个本地Markdown目标均存在，无冲突标记或行尾空白，普通git diff --check无诊断。新增GeographicMotion三文件另行no-index空白核对无诊断（exit1仅表示新增内容差异）。主审和独立阅读完成上述限定合同对照，未执行反例或业务。整个允许的球面水平走廊并集×独立AMSL区间及机体／跟踪误差的空间包含仍未实现，起降接触例外不能豁免全域。Compiler pending、完整StartPermit、正式机载结果及其余六类实质工作保持。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：RAW首入口与整组phase尾栅的限定静态审阅

准确10源码／工程登记、17文档共27路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。本批源码审阅覆盖：

- 普通RAW在首回调前冻结原包／收到时刻／原scope／journal receipt operation并持RAII depth，handler可回调检查后重取record。prequeue／foreign不采用新step，closed sender保留组屏障记录；嵌套／坏帧／丢弃原同scope锁存journal失效，不写新scope。
- 整栈guard越过handler之后的公共notify／firmware／core回调仍有效；后续nested RAW depth>1锁存原journal，外层handled=true不清它。原depth也在新scope绑定／新step进入之前阻断重入。
- opaque Seal不可从JSON／QML重建，原typed ledger／epoch／update token／raw receipt／revision／retained history及实际对象／PRIMARY身份冻结。自然时限在原各自clock域检查，不把elapsed采样本身放静态快照制造必变，不用新包重基旧判定。
- 完整Txn消费者逐机phase后与原READ尾核后查原seal，最后所有callback后查整组，成功才发布generation／stage／startupId集合；commandContext只读原已发布集合。失败／新request／阶段切换／停止退休旧集合。
- 已发现并修正正向自锁风险：Vehicle准备新slot后旧seal必变，故先完整authorizationFence，再current核新seal；不是跳过seal。首native发送入口仍以原集合核原pre-bind状态；gate路径不递归commandContext。
- tick诊断和observer.prepare含callback，阶段屏障／实际发送前补完整gate，不以陈旧allObserved越过新数据。旧接收栈defer、真源／历史失效与已write部分提交语义不放宽。
- ExecutionProfile只抽同一原startupPolicy validator供原生消费者复用；没有虚构carrier、默认值、自动升级或计划改写。qmake／CMake登记，不代表构建已验证。

最终普通文本读回：10源码／工程登记、17文档共27路径均存在，17文档的3,396个本地Markdown目标均存在；无冲突标记或新增行尾空白，Vehicle h／cc的22处逐字相同历史HEAD空白行保留；普通git diff --check无诊断。新Seal三文件另行no-index空白核对无诊断（exit1只表示新增文件差异）。主审及独立源码阅读对照原包入口／handler全栈、Seal实际memory调用、Txn整组候选／最终消费、新prepared正向顺序、自然过期与旧政策兼容；补入flying／landing缓存和实际Mission／GeoFence／Rally／FTP来源对象尾核后，未在本批已审范围发现新的确定接口错配。未执行上述反例，不能据限定静态审阅声称编译、逻辑、性能、实时性或飞行验证通过。整个允许运动域净空包含、Compiler pending、完整StartPermit与正式机载结果仍未完成。

本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方456为历史。

## 456：原生Witness移交及末写来源栅的限定静态范围

准确13源码／登记、19文档共32路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。本批直接静态阅读实际生产者与消费者，不执行helper、parser／AST、Qt业务、线程、UI、认证、SQL或设备。限定要点：

- Witness非QObject、私有工厂／不可复制，完整冻结原typed source18／datum14与纯值input8／result5；只对完整同次满足报告建立。原QPointer＋PRIMARY weak身份，不以weak.lock临时提升所有权。
- 原Review QElapsedTimer复制与原绝对elapsed deadline保持，所有后续只qMin缩短；原资料until及跨owner UTC水位不续期。memory floor／成功out继续跨层传递，不进入稳定hash。
- sourceCurrent持shared_from_this防回调释放最后外部引用；fence只核scope身份不递归。单个caller fence=false仅本次拒绝且仍纯memory尾核；真实源／时间／期限异常及真实重入退休，避免Review取消误退役其他owner原材料。
- 两类Review仅真实finished完整模型通过后创建；两Authority从真实acknowledged getter取同ptr并在搬移前后核；Txn独立持有，不从QML／JSON恢复或依赖已退休Review批准活性。原公开DTO与旧无资料路径保持。
- 可能回调的sourceCurrent安排在旧身份／认证／Readiness链之前；各层所有回调之后用memoryCurrent核全组。既有Vehicle最后commandContextCurrent已在write前消费Txn整组memory，之后至write只有本地journal／收据赋值；本批不改Vehicle。
- 独立阅读明确保留两处后续实质缺口：普通RAW首observer前未锁存startup depth及原step；逐机motion callback后尚无全组motion／receipt／EST／原点／ALT最终尾栅。Witness这里只封原配置／datum／时间来源，不能代替运动连续性证明。
- Transition允许运动是原点至全部地理项的球面走廊并集＋独立AMSL slab，未证明包含于已求解空间。Compiler pending、完整StartPermit与正式机载结果仍未完成，不把独立helper或末写源栅当完整净空产品。

最终普通文本读回：13源码／登记、19文档共32路径均存在；19文档的3,368个本地Markdown目标均存在，无冲突标记或行尾空白，普通git diff --check无诊断。新Witness三文件另行no-index空白核对无诊断（exit1仅为新增文件差异）。主审与独立源码审阅对照真实工厂／两类Review／两类Authority／Txn／Vehicle既有末写钩子、原时钟与UTC、caller fence及移交控制流，未在本批已审范围发现新的确定接口错配。未执行这些业务场景，不由上述核对推导编译、逻辑、实时性或飞行验证通过。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方455为历史。

## 455：后台启动报告与真实审阅的限定静态范围

准确范围11源码／登记、18文档共29路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。限定核对：

- parentless纯值Worker复用原JSON预算；实际输入exact8和结果5不含活QObject，原source／datum仍owner持有；取消协作式且不声明固定实时上限。
- Joint共享完整源提取，不复制编码器；报告匹配抽共享严格形状／引用／组件／计数／预算，明确planning和startup范围。没有GUI重solver。
- 普通与地面消费者在真实finished后核本次来源并将完整报告纳入artifact，原Deadline不续期，槽及地面预约不提前释放。
- 独立审阅发现完成时新样本替换首v4基准的确定问题，已改为向首次完整artifact副本追加报告；重捕只验证不替换。模型构造／满足及失败、违规分离，原无资料路径保持。
- 实际两Authority→Transaction独立持有、最终write、允许运动包络包含关系尚未接；Compiler pending仍阻断，不得解读为整目标可飞。

最终普通文本核对：11源码／登记与18文档共29路径均存在，18文档的3,272个本地Markdown目标均存在，无冲突标记或行尾空白，普通git diff --check无诊断；新Worker三文件no-index空白检查亦无诊断（exit1只表示新增文件差异）。主审及独立审阅对照真实生产者／消费者、旧规划合同、首基准修正、取消／期限及地面排空链，未发现本批已检查范围内新的确定接口错配。未执行这些业务场景；这不是编译、逻辑、线程实时性或飞行验证。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：显式工程datum与原生来源闭合范围

准确范围11源码／登记、17文档共28路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。仅直接源码阅读和普通文本／路径／Markdown目标／git空白核对；不执行业务、helper、parser／AST、脚本、QtJSON、线程、UI、认证、SQL、应用、设备、编译或测试。

限定源审阅重点：

- descriptor1原exact8与outer1保持；descriptor2明确exact9／relation12及所有原预算，query6唯一、显式GPI.alt同datum及真实0偏移，不默认补齐或虚做非零转换。
- 原native VERSION实际带receiptSequence而Catalog漏exact字段的确定错配已修；序号只核规范正十进制，不进入稳定内容绑定，原完整payload hash、版本／能力／来源校验保持。
- Catalog fingerprint缓存沿原snapshot的epoch／revision／完整entries公式；全目录候选受预算限制，所有真实提交先更新cache再changed，失败无部分变更。
- Datum先原Source18完整对应，再实际VERSION／PRIMARY／原点与目录关系；原配置13／原点18／目录10进入member4和整体14，所有回调后整组memory核，失败无前缀，recapture不更换原材料。
- Vehicle新增纯内存VERSION来源／内容／年龄检查及非拥有PRIMARY指针读取，不调用Link虚、插件、外部fence或发请求，不以weak.lock临时提升Link所有权。仍须原实际owner／QPointer／PRIMARY weak owner；持续网络连接或具体geoid不由此认证。当前Origin SourceMatches本来就直转MemoryCurrent，不将它误报为调用SourceAllowed的确定缺陷。
- 修复跨层UTC水位遗漏：Datum将实际最终观察时刻通过原生输出传给两Review，并接受调用方已观察下界；后续内存核同样不得回退到较早外Source时刻。动态时刻不进入exact5／稳定14或风险hash，原until和单调截止仍只缩短。
- 两类真实审阅在原artifact摘要前加入必需证据；原timer／Registry通知及审批尾栅消费同一冻结材料。旧无资料行为保持，原期限只缩短，非新TTL或新批准。
- 本地只读PX4流源码确有AMSL的baro回退，HOME／GLOBAL_ORIGIN不等于内部valid持续证据。工程对应不是物理高程／当前全球分支／同采样认证，原pending及实际启动到最终write仍未完成。

最终限定文本核对：11源码／登记＋17文档共28路径均存在；17文档的3,174个本地Markdown目标均存在，无新增冲突标记或行尾空白。Vehicle h／cc共22处与HEAD逐字相同的历史空白行保留；普通git diff --check无诊断，新Datum三文件的no-index空白检查亦无诊断（exit1只表示存在文件差异）。主审与独立审阅对照了真实类型／来源、纯内存链及UTC跨层反例，未发现本批已检查范围内新的确定接口错配。没有执行这些反例或任何业务代码；这不是编译、逻辑、实时性或飞行验证通过。原25目标和六类实质剩余保持，不按文件／批次提高总体完成率。以下453为历史。

## 453：原生配置来源与不可续期的资料窗口

准确范围15源码／登记、16文档共31路径见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。本批限定直接读源码与普通文本／文件路径／Markdown目标／git空白核对，不执行helper、QtJSON业务、parser／AST、脚本、UI、认证、SQL、应用、线程或设备，不编译或测试。

源代码核对重点：

- 共享观察保Swarm原完整配置提取，不发请求／写参数；唯一实际MVM对象、PM／链接owner／PRIMARY和组件身份前后一致，motor插件边界后重核，失败无成功前缀。
- Registry配置绑定与电池绑定独立；同源／同完整配置连续刷新保epoch，换源／失效／记录／构型历史改变退役。成功13字段关联context6和baseline14；当前本地批准不认证物理构型或性能。
- Vehicle新增callback-free版本来源epoch只读接口，不借原点／HOME有效性或报文freshness；Registry末尾的memory检查不重进motor插件。
- Source exact7／稳定evidence18，原输入指纹与原JSON配置hash明确，不混typed或startup指纹。完整原成员首轮与重捕后先结束所有回调，再整组纯内存核，非远端原子快照；冻结后recapture不重复场地规范化／hash，普通timer与Registry通知只做小型内存／时间检查。
- 已知APM／SUB布局参数绑定实际FRAME／FRAME_CONFIG Fact与typed原值，缺失不建立来源；变更／更换须退休原epoch，末核不重新getParameter。未上报硬件变化与任意第三方插件内部状态仍不是本地证据可证明项。
- 原APM共享coaxial状态额外绑定实际Copter对象并限定调用已读的非虚本机bool实现；宏只随pro实际APM模块登记，裁剪构建不引用该类。factAdded不解引用裸Fact实参，同组件新增保守退休。
- acquired<=本机UTC<until；捕获中或跨捕获已观察回退拒绝。首次单调期限与原内容读取／审阅期限取最小，重捕只缩短。未观察回退／未上报物理改装和系统时钟真实性不能由本机证明。
- 普通／地面审阅固定同源资料，地面从新READ起绑定；必需依据在原artifact摘要前进入，不能像可选ALTITUDE诊断被省略，旧无资料任务不补默认输入。
- 原clearance pending保持；本批不完成datum映射、同次实际启动solver／报告、两Authority至最终write、最终StartPermit或正式机载结果。

主读和独立交叉源审阅已完成；限定文本核对覆盖31准确路径，缺失／冲突标记／新增尾空白均无诊断，3074个本地Markdown目标均存在，git diff --check无诊断。Vehicle中22条逐字匹配HEAD的历史尾空白保持，不据此宣称全文件无历史空白。没有运行任何上述业务代码或工程验证，静态对读不是构建／逻辑／实时性通过证明。原25目标和六类实质剩余保持，不按文件或批次上调整体验收率。以下452为历史。

## 452：接触来源、双组件和整交集豁免范围

范围9源码／登记、18文档共27路径，见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。只直接读源码、做纸面整数／集合关系推导及文本／文件／Markdown目标／git空白核对；没有运行solver、helper、parser／AST、QtJSON业务、UI、编译或测试。配套PX4只读，不修改。

限定对读重点：

- Input1保持exact4／vehicle6；Input2明确root5／vehicle9／contact12及全部预算，不补缺项或自动升级历史。
- 非对称扫掠使用原axes及±cross支持面、准确sign选择上下offset；旧对称域结果数学表达保持，退化／奇数毫米不做浮点中心舍入。
- ContactModel保完整wire／序号／payload，LAND仅声明模型参考点另由支承面＋偏移产生；p7非MC下降stop，takeoff原起点不改，current-first独立假设不删。
- 仅指定support∩terrain整个闭交集包含于有限pad band才豁免；protected／obstacles／其他cell不豁免，完整coverage／corridor／terrainCoverage仍检查。
- Report3/limits9及原报告2明确配对，真实Compiler／Store消费者已接；源matcher重建应用／诊断／组件身份，不执行环境solver、认证结论或解除pending。
- 取消／Unknown／资源上限继续保原失败边界，原25目标与六类实质工作未缩减。

最终直接文本／路径／空白核对：9源码／登记、18文档共27路径均存在，无冲突标记或新增尾空白；18份文档中2985个本地Markdown目标存在，未检查锚点语义或外部网址；git diff --check无诊断。主代理及独立限定静态对读未发现确定缺陷，不证明实际接地、构型、坐标、时效、飞行路径或Qt运行正确；[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)明确未执行项。以下451为历史。

## 451：线程隔离、取消传播与三个consumer

审阅范围15源码／登记、17文档，见[准确清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。本批仅直接读源码与契约、文本／路径／Markdown目标和git空白核对，不执行Qt／QML解析器、AST、helper／技能脚本、线程、业务算法、认证、SQL、UI、应用或设备。

限定静态复核重点：

- Geometry所有确定结论在取消尾核后返回，取消锁存与资源耗尽区分；Review观察取消丢整份report，Compiler取消marker不能当plan。
- static编译不触实例_lastPlan／emit，Worker只持有界纯JSON深拷贝及shared atomic；无实时QObject／GUI／账号／Registry或文件对象跨线程。
- finished后才读输出／释放槽，parentless Worker cleanup不依赖Coordinator；析构只请求取消／解绑，不wait、不强杀或删除运行线程。
- 原生begin冻结实际Vehicle／ParameterManager身份，complete首次采参数值并首尾核原目录／原对象；ready不授予权限。
- Root只豁免自己一次准确reviewGeneration通知；pending健康／fleet及草稿编辑永久退票，原生、归档与公开binding均尾核，归档提交事实和当前批准分开。
- Draft、余段、AI各保原来源回调，取消按钮冻结原controller／requestId；不自动审批、上传、启动、恢复占用或发送命令。

直接文本／路径／空白核对：15源码／登记、17文档共32路径均存在，无冲突标记或新增尾空白；17份文档中2917个本地Markdown目标存在，未检查锚点语义或外部网址；git diff --check无诊断。限定源阅读未发现确定缺陷，不证明线程、时延、Qt类型映射、界面或运行正确。原净空／接地／构型高程时效、最终StartPermit／正式结果及其余六类代码未完成；原25目标保持，下方450为历史。

## 450：明确规划范围与无循环独立几何依据

本批准确9源码／17文档，见[范围](MERIVUS_SWARM_PATCH_MANIFEST.md)。仅源码静读和限定文本核对，未运行helper或测试。核心合同见[Joint](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)、[StartupSpatial](../../custom/src/Swarm/FleetMissionStartupSpatialReview.md)和[报告schema2](../../custom/src/Swarm/FleetMissionClearanceReview.md)。

- 旧显示reviewFingerprint保持其最终artifact含义；新geometryBasis是独立内容依据，不能换个名字借用旧最终指纹或伪造startup。规划只含encoded范围，startupIncluded与actualStartupReviewed分开，不冒称完整启动。
- 规划basis取已验证的完整几何／编码投影，不包含保存后的派生报告，不与plan报告形成循环。源提取不重复旧空间checker；归档源匹配不重复净空solver，也不能由来源一致推断计算真的通过。
- Compiler在完整非释放plan成形后真实调用，失败诊断与blocked风险保留，结果进入原记录完整内容。PlanStore对成功结果核完整来源，对失败仅允许精确空报告诊断；旧无报告历史不改写、不倒推已审。
- 原pending、Coordinator原风险／空域以及实际审批／write门槛保持。本批仍缺接地、当前构型／高程对应、期限和实际启动固定消费，不把代码入口当StartPermit。
- 已确认新增2M solver随显式QML准备→Coordinator::_review→栈Compiler同步运行于GUI线程；当前无worker／取消点，未发现定时重跑。异步纯值计算、原子取消和回GUI源代次核对是下一待开发代码，不以尚未测耗时否定风险，也不把完整原生Vehicle采集迁入worker。
- 主审与独立审阅已对读新旧身份分支、首次／归档同源、完整与预算Unknown的processed／checks计数、六类违反行与实际volume对应，未发现新的确定正向矛盾。该结论仅为限定源码阅读，不是测试通过或算法／运行安全认证。

本批限定26路径（9源码／17文档）检查：缺文件0、冲突标记／新尾随空白0、本地Markdown目标2836个、失效目标0；`git diff --check` 无诊断。只核文件／文字和目标存在，不解析锚点、不运行算法或证明构建／运行安全。原25目标和其余实质源码继续；未改PX4、刷机、提交、更新PR或打包，下方449及更早为历史。

## 449：整段声明模型净空核心的静态推导

本批准确5源码／15文档，见[范围](MERIVUS_SWARM_PATCH_MANIFEST.md)。仅阅读和纸面数学检查，未执行helper、解析器或测试；几何／报告合同分别见[Geometry](../../custom/src/Swarm/FleetMissionClearanceGeometry.md)和[Review](../../custom/src/Swarm/FleetMissionClearanceReview.md)。

- 定长符号整数先提升再乘，不在qint64中做Cramer三乘积；法向≤1e8、limit≤2e16下，分母量级≤6e24、分子≤1.2e33、代入残差＜1e42，远小于256位容量。支持界三乘和≤1.53e16仍在qint64内；溢出／非法公共输入返回Unknown。
- 有界闭包非空即存在三个独立活动面的顶点，包含低维情形。每个strict面各有一个严格见证时，见证顶点平均满足全部strict面；不能只检查单顶点。只有完整枚举或充分支持界证明才能返回空；预算不足未知。
- 三维box＋线段是四生成元zonotope，面法向来自生成元两两叉积：三轴与cross(axis,direction)；六轴bounds加其支持斜面表示整段，不以AABB代替一般扫掠。
- 闭区域有序差集用closed inside前缀和strict outside，覆盖共享边界不生假缝。任何非空剩余片段穿过全部体积即证明未覆盖；全部片段被覆盖才成功。原Bounds完全落于一个closed体积时允许充分证明快速True，不耗根分片；仍先验完整union后缀，坏值／预算不足不能被早期证明掩盖。没有单体快速证明则继续完整并集，不缩小支持范围。显式栈／面／片段／工作预算不截断为成功。
- E7及工程整数图表用有符号floor／ceil；原binary64高度按位精确放大取整，额外最多1毫米外包只包数值量化。报告保完整同序成员和独立假设，范围完整／计算完成／声明模型满足分离；接地缺口或Unknown不能变为通过。
- 主审／独立审阅已修正并复读：公共全零法向量拒绝；大map键先限长再UTF8分配；binary64完整布局守卫；Joint独立16MiB边界。最终Bounds充分证明新增段另作追加审阅，不能以其添加前的旧结论代替。全部仅静态阅读／推导，不是运行通过。

本批限定20路径（5源码／15文档）检查：缺文件0、冲突标记／新尾随空白0、本地Markdown目标2725个、失效目标0；`git diff --check` 无诊断。它只核文件、文字和目标存在，不解析锚点、不执行算法或证明构建／现实安全；Geometry另列三个未运行纸面用例。Compiler pending保留，无live消费者或新权限；原25目标、完整规划／实际启动消费及其余实质源码工作继续。未改PX4、刷机、提交、更新PR或打包；下方448及更早为历史。

## 448：净空工程资料与真实来源链的限定静态核对

新增exact4 normalize／exact7 inspect，输入schema1根4／scene13／frame7／vehicle6／policy9，4MiB、深16、131072节点。原始值先限纯JSON，再逐字段严格校验；毫米凸多边形以qint64精确叉积判断CCW／凸性，不用epsilon或修复。成员须为原全部活动ID同序，配置指纹只是声明，不认证实际物体。日期仅格式和先后校验，不检查当前时刻。

Draft缺资料仍来源1／11settings，含资料来源2／12settings；全替换configure、CAS设置／清除、中央source验证／加载及buildTask保留完整声明。Compiler保显式坏constraints与新资料原值，真实成员构造后检查；合法也产生几何pending阻断。PlanStore独立核原／规范内容与准确编辑源一致，不改历史值；坏新声明不能归档成有效前缀。Root两处恢复预检版本配对，不借新source恢复审批或机载事实。

主代理及独立生产／消费交叉静读覆盖严格类型／null、各字段数与资源预算、数值规范JSON、日期声明、全局区域ID、qint64凸性边界、逐机同序及schema1／2正负路径。原生CAS／commit通知尾核、source保存／完整加载、Compiler源重建及真实pending风险、447实际风险门禁和两处Root恢复都已对读；普通无资料不增加新字段或pending。补正版本库中区域编辑源与plan可各自合法但场景不同的缺口，现要求两边存在性／完整内容一致；版本库仅用一次inspect内置normalize，避免对同一资料重复多边形／树遍历。修正后未见确定的新生产消费错配；这是限定源码推导，不认证Qt类型、运行几何、时序或性能。

限定全文核对覆盖8源码／工程＋17文档共25路径，无缺文件、冲突标记或行尾空白，2685个本地Markdown目标路径无缺失（不验证页内锚点）；全工作树`git diff --check`无诊断。新增未跟踪文件同样按全文检查，不以累计diff统计本批行数。

UI/UX技能仅用静态输入保留、明确错误／确认和来源一致性规则，未运行技能脚本、解析器、UI或视觉验证。本批不认证场地、几何净空或最终StartPermit。原25目标及完整solver／真实启动固定消费和其它实质代码继续；未编译、测试、运行helper／UI／认证／SQL／应用／设备、仿真、修改PX4、刷机、提交、更新PR或打包，下方447及更早为历史。

## 447：风险业务阻断的实际消费者静态核对

静读发现两个同链问题：Coordinator只核riskAssessment.inputValid而未消费blockingReasons，合法严重业务风险仍可能标为reviewable；RiskEvaluator空code在riskItems使用fallback，但原因去重仍用原空code导致原因缺失。报告自身blocked／safety gate标签不被实际审批替代消费，不能以显示正确推断门禁已闭合。

新增native-only纯static检查，要求真实非nullinputValid=true与实际非null空QStringList／QVariantList，不作坏型宽松列表转换。生成风险时共用effectiveCode。Coordinator决定needsReview，Deployment再次核实际冻结评估与准确归档并比较完整规范JSON；普通／地面启动审阅分别核准确原记录，不取另一任务lastReview。合法空列表仍继续其它原门槛；requiresOperatorReview恒true不是业务阻断开关。

主代理及独立生产／消费交叉静读覆盖共享方法的真实类型／null检查、原生QStringList与JSON List正向表示、严重信号统一code、Coordinator结果提交及needsReview、Deployment冻结／归档完整规范JSON、普通原binding／完整hash、地面准确原记录／evidenceHash和成功才赋出bytes。请求、持续READ、捕获、确认、审批重捕与reviewContext沿原共用门禁消费；没有新增回调、重采、另一任务latest结果、权限或历史回写。修正后未见确定的新接线问题；这是限定源码推导，不认证Qt容器行为、真实协议时序或运行结果。

本批6源码／16文档共22路径。限定全文检查无缺路径、冲突标记或行尾空白，2661个本地Markdown目标路径无缺失（不验证页内锚点）；全工作树`git diff --check`无诊断。未跟踪文档同样按全文检查，不用累计diff计本批行数。本节不是编译／测试通过记录，原25目标与完整净空／最终许可／正式结果等实质代码保持。未编译、qmake／moc／QML解析、运行helper／UI／认证／SQL／应用、测试、仿真、访问设备、修改PX4、刷机、提交、更新PR或打包，下方446及更早为历史。

## 446：中间舍入误放行的精确数值门槛修正

静态数学推导确认旧路径存在三类误通过：原高度相减丢失微小非零残差、整数毫米先除以1000的误差在近消时放大，以及两个已舍入差值之间的漂移低估。最终nextafter无法恢复前面丢失的量。三个明确原始数值及阈值见[高度合同](../../custom/src/Swarm/FleetMissionStartupAltitude.md)，均为未运行解析反例，不能写成测试通过记录。

新增[定长算术](../../custom/src/Swarm/FleetMissionAltitudeArithmetic.md)按传入binary64的确切位值、以2^-1074为共同单位做整数线性组合。最多四个原值、整数系数±1000、固定36个uint32 limb；容量上界1139位小于1152位，运行时仍逐项拒绝非法输入／整数溢出。阈值保留实际binary64（含次正规）并精确乘整数scale，最后执行闭区间绝对值比较。没有先浮点乘1000、相减、除法或隐含epsilon。

主代理及独立生产／消费交叉静读覆盖宿主布局拒绝、指数与次正规映射、系数零仍校值、分块移位0–31、进借位、符号累加、绝对差、零／非零恰等边界和容量。原高度helper三残差直接使用A/L/R、原整数毫米及HOME localZ；三漂移直接使用四个原高度操作数。普通／地面Review、Authority、Readiness、Transition和Vehicle绑定／逐包／write前monitor仍共用同一helper。显示差值只保原解码一致性检查，不作为容差操作数；同receipt正负零、来源／时钟／首次基准及全部权限合同不变。修正后未见确定的新接线缺陷；这是限定静读，不认证数值环境、Qt行为、协议或运行结果。

本批4源码／登记、15文档，共19路径。限定全文核对无缺路径、冲突标记或行尾空白，2556个本地Markdown目标路径无缺失（不核页内锚点）；全工作树`git diff --check`无诊断。新增未跟踪文件同样按全文检查，不以累计diff计本批行数。工程师仍须构建与执行数值及真实调用链验证，静态推导不能替代该工作。

原25目标、实际净空风险链／最终StartPermit、正式机载结果和其余实质代码保持。未编译、qmake／moc／QML解析、运行helper／UI／认证／SQL／应用、测试、仿真、访问设备、修改PX4、刷机、提交、更新PR或打包；下方445及更早为历史。

## 445：首次高度基准、显式容差与原生scope的限定源码核对

本批新增纯reference20／sample20契约和明确startupTransitionPolicy v4，固定HOME、沿v2运动／航向观察，新七字段包线含六项显式0–1000 m容差。必要三个已知有限float32及三差值采用明确2^53−1安全JSON支持域，不夹紧；原始native27及旧v1–v3不因此丢值或增加依赖。无关monotonic／terrain／bottom不要求known，不将其大值塞入必要样本。

普通／地面首次审阅保存原样本，当前风险身份隔离正常动态sample但仍按原基准复核；真实scope一次绑定与逐原包固定大小监视共同约束Seq0／ARM／MODE和实际write。scope内曾越界、坏帧、失源或超gap不能因最新正常值清除；ALT计时保持自身本机单调域。终态原target只读取源不是权限续期，原step33及terminal row13／outer20保持。

独立生产／消费交叉静读覆盖pure20／20与native27、v4 exact9／envelope7、原已阅材料至Authority／Transaction、逐帧粘性监视、正常RAW日志、步骤／真实write尾核，以及取消／析构／未建step即失败后的独立退休与排空。修正了HOME localZ／time变化而geo3不变的尾窗：新增纯内存HOME9／自身单调年龄检查，所有新coordinate回调均放在原EST／origin最终核对之前；末尾不再调用Link虚方法。纯取源不再错误要求实际UDP Link与Vehicle同线程，合法IO线程PRIMARY仍沿真实对象／来源门禁处理。历史clock flag可经明确新审阅冻结，其后flag／revision变化拒绝；无关terrain状态不成为必要参考变化。

正向路径同时核对：v1–v3不继承残留v4监视或额外ALT接收依赖；原被动RAW日志的非quiescent分支不被其自身接收栈误拒。完整hash与全部风险精确不变的blocked材料仅可由原只读ACK流程确认，不获得条件批准。明确的新地面READ／风险审阅建立该轮首次基准并保持至新审批／Authority，不永久绑上一轮HOME，也不复活旧scope或抹旧违规。最后补正同receipt数值不可改写检查，使用严格数值／零符号比较，不让普通QVariant相等把+0与-0合并。修正后未见确定的新生产消费不匹配；这是限定源码阅读，不认证Qt类型、浮点数值、协议、线程时序或真实运行结果。

限定文本核对覆盖17源码／登记、24文档共41路径：无缺文件、冲突标记或新增行尾空白，2591个本地Markdown目标路径无缺失（不验证页内锚点）；全工作树`git diff --check`无诊断。Vehicle.h／cc的22处历史行尾空白与原HEAD整行一致，保留不作无关格式化；新增未跟踪文件同样按完整文本检查，不以累计diff计本批改动。

表单及现有只读摘要按ui-ux-pro-max的分组、显式标签、原输入保留、错误／焦点、完整预览及人工采用规则修改；未执行技能脚本、UI渲染或可访问性验证。

本批17源码／登记、24文档共41路径。只证明限定源码与文本层的工作，不认证共同原点、内部valid、同采样、样本鲜度、真实净空或最终StartPermit，亦不覆盖scope建立前审阅等待期的连续历史。原25目标及其余实质代码继续；未编译、qmake／moc／QML解析、运行helper／UI／认证／SQL／应用、测试、仿真、访问设备、修改PX4、刷机、提交、更新PR或打包。下方444及更早为历史。

## 444：原始高度与不可变诊断材料的限定源码核对

本批对照本地消息141与PX4 ALTITUDE发送实现，新增严格decode18／row4／state8和独立Vehicle native27。六字段保留同一原包的原始位值与逐项已知语义，三个报告差值不跨包补数。实际来源、整栈接收、类别revision和时钟倒退与推进中的高度／receipt／age分开；接收fresh不认证样本fresh。

诊断在Readiness及普通／地面风险中冻结展示；不改变原门槛，不进入phase或当前风险稳定身份。普通审阅首次完整artifact及hash继续保存，重捕比较先核完整原hash，再只排除新独立诊断和其派生hash，不用新观察覆盖原材料。原风险材料先按原预算完整检查，新增诊断只用剩余额度；容不下时整份省略该诊断，不删原风险字段或目标。缺key表示未提供，不表示零、无传感器或健康。

独立生产／消费交叉静读覆盖decode18／row4／state8、native27真实来源及整栈接收隔离、状态revision与数值变化分离、相邻实际报告时间倒退、失源后的当前序列标记和历史计数，以及普通／地面风险的首次冻结、动态身份隔离与预算回退。补正了未知值必须为显式Nullptr而非任意typed-null、Link回调后弱对象检查和部分hook绑定的就绪状态；普通Readiness整体风险预算明确先排除可选诊断。原float32完整有限值保留在原生观察，超出展示安全数值或独立深度／节点／字节预算只使该附加诊断不可提供，不截断数值或放宽原风险预算。修正后未发现确定的新生产消费不匹配；这是限定源码阅读，不是Qt类型、数值、协议、并发或运行行为验证。

限定文本核对覆盖11源码／登记、18文档共29路径：无缺文件、冲突标记或新增行尾空白，2453个本地Markdown目标路径无缺失（不验证页内锚点）；全工作树`git diff --check`无诊断。Vehicle.h／cc仍保留22处与原HEAD整行相同的历史行尾空白，不作无关格式化，不称为新增问题或声称全文件无空白。新增未跟踪文件同样检查完整文本，不以累计diff代替本批改动统计。

本批11源码／登记、18文档共29路径；原25目标、显式可信参考政策／净空／最终StartPermit及正式结果仍未完成。未编译、qmake／moc／QML解析、运行helper／UI／认证／SQL／应用、测试、仿真、访问设备、修改PX4、刷机、提交、更新PR或打包。下方443及更早为历史。


## 443：原点账本与实际发送连续性的限定源码核对

本批新增消息49纯decode18／reference8和独立Vehicle native24账本，stable18用于阶段依据。按本地MAVLink头定义保留完整时间文本、MAV2尾零与未认证边界；实际PRIMARY／版本／飞行来源及sticky everSeen与可用性分离。消息是事件型，age只诊断，不引入周期TTL或自动请求。

审阅范围包括：原始收包最早prepare／commit与整栈隔离、真实参考变化／坏帧／失源／ABA、Readiness和Transition原basis首尾、地面新READ物理投影、首次真实send scope冻结及最终write纯native检查。相同参考重复接收不因receipt／age更换稳定身份；真实write事实不被后置失效抹掉。

独立生产／消费交叉静读覆盖decode18／projection8、native24／stable18、两个absence／observed分支的实际PRIMARY核对、首次scope／实际write／receipt33，以及terminal row13到新READ全过程。补正了MAV2基础字段截断高字节一致性、connectNotify后Link销毁的QPointer检查、原点失效保持纯内存且旧hook逐条受控释放、地面READ固定白名单与终态再审逐机四字段遗漏；旧hook清理若中途重入仍保留未处理句柄，不丢失并累积。修正后未发现确定的新生产消费不匹配；这是限定源码审阅，不是Qt类型、协议数值、时序或真实运行验证。

限定文本核对覆盖13源码／登记、18文档共31路径：无缺文件、冲突标记或新增行尾空白，2404个本地Markdown目标路径无缺失（不验证页内锚点）；全工作树`git diff --check`无诊断。Vehicle.h／cc完整文本仍有22处原HEAD已存在的行尾空白，已逐行比对原HEAD原文保留，不做无关格式化，不将其称为本批新增或声称全文件无空白。新未跟踪helper及文档同样检查完整文本；累计diff不作为本批行数。

本批13源码／工程登记、18文档共31路径；终态再审逐机row9→13、外层20和step33不变。原25目标、共同原点／内部有效分支／净空／最终StartPermit及正式结果仍未完成。未编译、qmake／moc／QML解析、运行helper／UI／认证／SQL／应用、测试、仿真、访问设备、修改PX4、刷机、提交、更新PR或打包。下方442及更早为历史。


## 442：联合名义请求链与同源显示的限定源码核对

本批新联合helper消费原完整missionReview及startupView，不重跑Builder或拼单机计划；canonical协议顺序、E7／float32地理项与速度项分开，保出航／返程连接和终结请求。独立启动参考不串成真实航迹，逐机覆盖与比较完成分开。

新纯Checker对全部已表示跨机段及同一归档区域作保守静态分析；严格投影／区域／高度／时间语法和预算，不按未知权威时间排除区域，不支持输入或计算失败不发布成功前缀。原归档报告scope不被改写，全部许可／净空／实际路径／机载事实仍为false。

Root保持准确record／actual来源／完整目标顺序首尾核对，同capture token缓存并发布9字段信封。现有UI仅新增只读摘要及8项分页字段类别；沿既有Palette／纯文本／焦点和长值分段规则，不新建自动动作或图层。

独立生产／消费静态对读覆盖原Builder的raw13／canonical12／source映射与完整hash、新helper的22字段／逐机12字段／来源6字段、全部地理链与启动对应、Checker严格区域／高度slab／零长／边界／全工作预算及UI21字段分析／未知／截断含义；Root准确来源、同token缓存和Map九字段信封也已交叉核对。未见修正后确定的新增正常路径阻断或漏接；修正了coverageComplete不应受计算预算完成状态影响的语义耦合，输入表示范围与comparisonComplete现分别解释；新helper对明显超限字符串／键先做长度预检，再生成UTF-8副本，原预算未放宽。静态源码阅读不认证真实几何数值、Qt类型行为或运行性能。

限定文本检查覆盖10源码、17文档共27路径：无缺文件、冲突标记或行尾空白，2307个本地Markdown目标路径无缺失（不验证页内锚点），全工作树`git diff --check`无诊断。新未跟踪helper与文档同样按完整文本检查，不以累计diff统计冒充本批行数；本节不作为运行通过记录。

原25目标及可信动态／净空、最终StartPermit、正式结果等实质代码继续。未编译、qmake／moc／QML解析、运行helper／UI／认证／SQL／应用、测试、仿真、访问设备、修改PX4、刷机、提交、更新PR或打包。下方441及更早为历史。

## 441：原值风险门禁、冻结归档和同步通知的限定源码核对

静态阅读追踪原始输入经Root、Intent、Coordinator、Checker、RiskEvaluator／ReportBuilder到批准／部署门槛：坏值不会借真假值／toList变为空列表，坏间距不继续默认5米，source矛盾不任选一份。固定blocking code保留在风险／报告中；inputValid仅描述风险输入schema，不能替代critical／blockingReasons及needsReview门禁。

Root归档只用原生采集前snapshot，合法来源首尾核当前私有Intent指纹／提供性、原输入／审阅代次和完整binding。坏但安全JSON可保阻断诊断，超限／不可表示输入无新归档。Intent先整体提交新状态再通知，QPointer／私有revision隔离销毁和同步A→B→A，保留原Root的代次与CAS撤销。

独立生产／消费静态对读覆盖正常缺省／空列表、坏scalar／map／null、双源冲突、无效间距、回调期间换值、归档与审批首尾及状态通知正向路径。发现并修复了超限List可借单项包装获得快照的问题；包装现只用于非List安全坏JSON，原List必须通过完整检查。未见修正后确定新增的正常路径阻断；这不是运行／数值／性能或Qt类型行为验证。

本批限定文本检查覆盖7源码、15文档共22路径：无缺文件、冲突标记或行尾空白，2235个本地Markdown目标路径无缺失（不验证页内锚点），全工作树`git diff --check`无诊断。新增未跟踪文档也按完整文本检查，不将累计diff当本批行数。

未编译、qmake／moc／QML解析、运行helper／UI／认证／SQL／应用、测试、仿真、访问设备、修改PX4、刷机、提交、更新PR或打包。全路线联合空间报告、最终StartPermit／正式结果及其余原25目标的实质源码继续；下方440及更早为历史。

## 440：原完整任务与归档空域的限定源码核对

对照原PayloadBuilder实际出航／返程编码、sourceItemMap及起止动作，新增全成员只读投影；原整plan生成失败不能以显示fallback改变输入后继续编码。Root沿439准确record首尾核对，完整任务／原归档报告不借当前草稿／地图或独立历史Run。

空域输入四字段只记录形状，保存／导入与显示消费者分别验证；旧空列表不倒推加载。归档报告保留原实际阈值及原scope，不运行checker重演或补造终结／起飞检查。完整值缓存与分页避免在每次当前性通知中复制全部协议项，但尚无实际性能验证。

限定文本检查覆盖14源码／资源、18文档共32路径：无缺文件、冲突标记或行尾空白，2188个本地Markdown目标路径无缺失（不验证页内锚点），`git diff --check`无诊断。新文件同样按完整文本直接检查，不将累计git差异当本批行数。

独立静态对读覆盖原Builder、全组视图／错误、归档空域、Root精确record／成员顺序与cache token、PlanStore新四字段校验、明细分页与Map独立层，未见修正后确定的正向合同阻断。发现并修正了原生列表包装不应依赖JS Array.isArray、全视图缓存不得错配新捕获，以及通用estimatedElapsedSeconds不能一律称不含停留的文案问题。源码确认默认profile由Compiler解析到member，Builder本身拒绝未解析成员；显示没有引入默认补猜。界面按既有规范保分页、焦点与同XY合并／偏移标记，未做渲染或性能验证。

源码阅读不替代Qt编译、QML解析、协议／几何数值测试、UI性能、真实空域来源或放行。原25目标及联合重审、最终许可／正式结果等实质开发保持。本批未编译、运行、测试、认证、仿真、改PX4、刷机、提交、更新PR或打包。下方439及更早为历史。

## 439：冻结启动几何与地图的限定静态核对

对照实际Readiness／StartReview／ground riskSummary，新增共同几何helper与Coordinator纯值入口；Root捕获实际当前风险和准确归档record，前后核Episode／身份／binding／指纹／版本，地图与明细共用冻结值。无最新草稿或实时位置补缺，无捕获时空图而不是旧路线。

新静态间距入口复用既有线段最短距离算法，旧check行为保持；分别比较三类独立线段，明确未知时间、局部投影域、原声明阈值与明细预算，不伪造连续轨迹或4D秒数。几何可显示、已表示段比较完成、完整目标覆盖、实际安全许可是不同状态。

限定文本检查覆盖13源码／资源、17文档共30路径：无缺文件、冲突标记或行尾空白，2116个本地Markdown目标路径无缺失（不验证页内锚点），`git diff --check`无诊断。新增未跟踪文件同样按完整文本直接核对；不使用累计git统计冒充本批改动行数。

原生Readiness／两类审阅producer、helper、Root精确record和有序整组取源、Panel正向确认及静态checker另经独立生产／消费交叉阅读，未见确定新增合同阻断。地图复核修正同XY三卡遮挡为每机合并高度卡；初始化延后至组件完成，按压冻结原分支／来源key／交互代次，防旧按压落入另一分支或ABA后执行。进入／退出规划的可回调步骤后另核私有同步ticket、实际controller与来源key，防旧外层覆盖嵌套换源后的新模式。界面沿用仓库调色板、明确历史／未知文字、分页、键盘焦点和有界滚动，不宣称已渲染验证。

源码和文本无诊断不能替代Qt编译、QML解析、UI地图行为、数值测试、实际取证或放行。原25目标／六类实质代码保持未完成。未编译、运行、测试、认证、仿真、改PX4、刷机、提交、更新PR或打包。下方438及更早为历史。

## 438：完整政策表单与来源绑定静态核对

本批对照原ExecutionProfile policy规范化与StartupHome原范围，新增[表单](../../custom/src/Swarm/FleetStartupPolicyEditor.md)，沿[父完整保存链](../../custom/src/Swarm/FleetExecutionProfileEditor.md)消费，不运行解析器／helper或新旧业务代码。检查重点为所有原字段／常量／范围、零值、版本切换、未采用buffer／高级JSON互斥及原生CAS。

实际草稿代次经原publication公开，父冻结facade／代次／ID／revision；Root另在invalidate与参数只读回调两侧冻结真实backing对象。来源变化保留输入而撤销确认，不将相同机号／revision的新草稿当作旧对象；隐藏保留策略会话与字段但退休在途参数offer。

限定文本检查覆盖6源码／资源、14文档共20路径：无缺文件、冲突标记或行尾空白，2041个本地Markdown目标路径无缺失（不验证页内锚点），`git diff --check`无诊断。新增文件同时直接逐行核对，不以累计git统计说明本批行数。父子同步采用、来源／版本绑定与原生字段合同另作独立静态交叉阅读；发现的Controls1／Controls2混用及替换表单callback尾核问题已修正。

源文阅读与文本无诊断不替代Qt编译、QML解析、表单运行、实际参数观察或工程放行。原25目标／六类工作继续，未编译、运行、测试、认证、仿真、改PX4、刷机、提交、更新PR或打包。下方437及更早为历史。

## 437：实际启动工作台源码审阅

本批限定审阅[工作流](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)、[界面](../../custom/src/Swarm/FleetMissionStartupPanel.md)、[地面风险](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)及现有原生操作链。Root按准确20动作映射原ForScope，不嵌套runCurrent；公开仅JSON值。UI与操作前后分别核当前Episode／实际身份，原生每步继续核来源、期限与实际发送范围。

生产／消费对读修正真实身份role字段、部署vehicles取目标，以及Series脱敏摘要不含逐机步骤的问题；地面显示改取同一实际consumer并尾核原对象。动作接纳、后置上下文失效和真实queue事实分开；无原生返回则未知，不自动重试。地面风险展示不输出原身份、角色收据或载荷。

限定文本核对覆盖7源码／资源、14文档共21路径：无缺文件、冲突标记或行尾空白，1987个本地Markdown目标路径无缺失（不验证页内锚点），`git diff --check`无诊断。新增未跟踪文件另作逐行文本核对，累计git统计不作为本批规模。原生风险生产／消费与界面确认正向路径已独立交叉阅读；未见确定新增阻断，不等于运行通过。

以上不替代Qt编译、QML解析、UI布局、认证或运行验证。原25目标与六类实质工作仍未完成。未编译、测试、运行helper／UI／认证／应用／设备、改PX4、刷机、提交、更新PR或打包。下方436及更早为历史。

## 436：实际Run简报与本地人员回执的限定源码审阅

范围见[7源码／15文档清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。分别检查Report的完整观察→有限简报、Manager保存／分页／导入、Guided实际来源／身份／提交结果及Panel入口；并交叉阅读原生会话、角色快照、实际Run取源与指挥中心接线。没有执行编译、moc/qmake、解析器、helper、测试、UI、认证、SQL、应用、仿真或设备动作。

静态核对内容：

- 简报必须取真实注册taskUUID，保留原Run／事务／部署及原7/9字段binding；root不是从导入报告、任意taskId或移动的最新提案取源。
- 完整报告复用原校验后只抽有限摘要；17根键／13摘要键／14逐机键／8固定claims，null桥接、96 KiB／1024节点／深度5、不截断。
- 原生身份收据与用途角色快照在变更前重核，接收用当前接收人而非已离线的发送人；面板角色标签不单独授权。
- 候选一次消费；native signals之前保存提交事实，返回后上下文变化不抹commit。接收过期可能写expired并返回false，不能把false一律当无状态变更。
- Manager页摘要不携带整份metadata，每页4条；导入来源不序列化为可信local，Run最多256份且不自动删历史。
- UI分离冻结／明确发送与详情已阅／处理，身份／来源／版本变化废弃旧确认；当前已提交事实与历史记录单独保留。具体交互仍待工程师运行验证。

7源码／15文档的存在性、冲突标记及行尾空白检查无异常；15文档中的1929个本地Markdown链接目标路径存在，未核对锚点或据此认证文档内容。`git diff --check` 无诊断。上述文本检查也覆盖本批未跟踪文件，不能代替QML／C++解析或构建。

上述只说明静态审阅范围，不证明编译、运行、系统身份、远端任务或飞行可靠性。跨站控制权、真实机载接替及原25目标仍未完成；无PR或包更新。下方435及更早为历史。

## 435：只读新记录显示链的源码审阅范围

审阅范围为[12源码／资源路径及16文档](MERIVUS_SWARM_PATCH_MANIFEST.md)。主代理核实际Compiler／RemainderPlanner／PayloadBuilder字段、Root新记录缓存及页面消费；后端、地图和明细分别实现并交叉静态阅读。没有执行编译、moc/qmake、解析器、测试、helper、UI、应用、SQL、认证、仿真或设备动作。

重点核对：

- 同一actual新review.plan产生显示view；创建前后仍核原capture/proposal、outbound scope及精确binding。显示不供权限消费，不撤已提交事实。
- 规划点与实际协议序号、原点映射和动作映射分开，几何／编码独立可用；所有数量有限，不输出成功前缀，不伪造默认数值。
- 原编码器的default-yaw invalid QVariant存在QML丢字段风险，本批只在显示边界转成显式JSON null；保留原Builder、编码Hash和UI严格null判定。
- 采用来源、过期保留、无效不回退；只读地图模式保持规划开关，编辑实例不被隐藏销毁，按压代次与旧取点／飞行门禁仍核。
- 新QML只分页显示8项，record身份变化清旧页；资源、工程登记和新增入口对应。没有第二上传入口或新增启动许可。

直接PowerShell文本／路径核对覆盖28路径（包含未跟踪新文件）：无缺失、冲突标记或行尾空白；16文档中1920个本地Markdown目标均存在，`git diff --check`无诊断。这些是文本和路径检查，不是业务测试、QML解析或构建证明。

静态阅读只能说明所审源码关系，不能证明QML加载、数值桥接、交互、性能或飞行行为已经运行通过。工程师待执行项目见[435交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类剩余源码工作保持，以下434及更早为历史。

## 434：独立余段审阅到共用地面部署的限定源码审阅

范围为[4个QML及13文档](MERIVUS_SWARM_PATCH_MANIFEST.md)。本轮没有编译、解析器、测试、helper、UI／认证／SQL／应用或设备运行；静态阅读不能证明QML装载、实际认证、上传时序或飞行安全。

- 正向来源：实际prepareFleetIntent新记录→私有reviewContext→人工采用→精确token申请／批准→唯一DeploymentPanel；没有填写区域Draft identity，原区域按钮不被宽松OR放开。
- 身份：七个Store绑定字段加输入／审阅代次全部比较，保存生成审阅时的current-outbound scope，原证据审批门禁复用。旧selected Run只是历史规划来源，不能成为新批准；普通观察选择变化不凭空选择另一计划。
- 选择与失效：accepted与current分离，过期不自动回退区域；新捕获／预览尝试退休旧上下文，字段编辑与证据刷新仍撤live review。明确退出只退休匹配上下文，历史记录／已发生本地动作保留；原生批准回执先记录实际本地提交，再做尾核。
- 组件：传入明确sourceKind／contextToken／approvalAllowed／refreshVehicleIds，余段刷新取其plan.members，区域取draft.members；来源／完整归档identity变化清旧确认，仍用单个部署面板和原后端。
- 权限：原当前身份、不同账号审批、一次覆盖确认、实际地面未解锁／占用／链路／编码／读回门禁不改；不新增机载指令协议、起飞按钮或旧Run占用豁免。

UI技能仅采用静态交互规则：分步确认、保留输入、明确退出／过期／禁用解释、QGC主题和键盘焦点；未运行其脚本或视觉检查。工程师待执行场景见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方433及更早为历史；本批不增加全25目标验收计数。

限定文本检查：17路径（包括未跟踪源码）无缺失文件、冲突标记或尾随空白；13文档共1838个本地Markdown目标均能解析到现有路径；`git diff --check`无输出。另从实际源码确认部署面板仅有一个实例。上述检查不验证QML语法／装载、行为、文档所有陈述或真实设备安全。

## 433：原Run取源到独立余段提案的限定静态审阅

本轮范围为[清单中的16源码/资源和17文档](MERIVUS_SWARM_PATCH_MANIFEST.md)，共33路径；这不是整体25项目标验收、可编译证明或飞行安全结论。

### 已阅读并对应的生产/消费路径

- 原输入冻结：实际Deployment读取intent完整参数，整份纯值/预算核对后剥除明确运行快照，在原准入成功后尾核并冻结；失败只令余段来源不可用，不改变上传/Startup门槛。
- 原生取源：[Startup](../../custom/src/Swarm/FleetMissionRemainderSource.md)核实际Run/member/已提交MODE窗口、原plan/payload/sourceItemMap及progress映射，首尾检查owner/generation；不调用重新取证许可入口，不把retired/失联历史伪装成实时许可。
- 纯值规划：[helper](../../custom/src/Swarm/FleetMissionRemainderPlanner.md)仅接受明确0基原索引，包含所选点和后缀全部hold；重新计算自身起返点路径与移动估算。原area作为围栏/区域上下文保留，删除coverage和旧编辑源避免重跑覆盖；原成员时长/返程下限并入新政策，新水平速度要求不得更低。
- 正常消费：[Compiler](../../custom/src/Swarm/FleetMissionCompiler.md)共享validateProvenance/matchesTask/matchesPlan，实际规范geometry/hold/profile/estimate/constraints须一致；合法当前fleet/intent/taskId/templateValidation注入不作为内容篡改。PlanStore保存/加载复核规范来源与单成员实际plan，旧档案不恢复权限。
- 实际操作：[Guided/Panel](../../custom/src/Swarm/FleetMissionRemainderPanel.md)只用选定actual Run冻结源，预览与确认审阅分开；私有原生task送入原prepareFleetIntent。新捕获/预览失败不保留旧可确认token，字段编辑撤旧审阅；保留历史record提交事实，不借尾核失败断言未提交。
- 交互边界：自身pending不回灌到自身inputSettled造成自锁；对原区域预览/批准仍按未保存输入处理。退出保留文本清确认、来源变化不自动重读；实际渲染、焦点、主题与资源成本没有运行验证。

### 本轮静态修正

1. 保留原area边界上下文，避免删旧coverage时一并丢掉原围栏证据；未重新生成全区路线。
2. 新成员的estimatedDuration只算移动，显式hold由原Coordinator加一次，返程首点hold置0；不继承原成员速度/新目标旧profile。
3. 保留原成员四种明确时长字段的最大下限以及返程下限，同步constraints后生成新路线摘要；原坏值不被修复为默认值。
4. 重新取源/预览的失败路径退役旧token；输入更改失效旧全局审阅；历史归档事实与当前可用性分开表述。

### 直接文本检查与限制

已执行`git diff --check`，无诊断。另对全部33路径（包括未跟踪文件）直接检查存在性、冲突标记和尾随空白，未发现问题；17份文档的本地Markdown链接目标存在性检查未发现失效路径。后者只核路径，不证明段落语义、外部网站或运行结果；空diff也不被用来代替未跟踪文件检查。

未编译、运行qmake/moc/解析器/AST/helper/测试/SQL/UI/应用/认证/仿真/设备，未修改PX4、刷机、提交、更新PR或打包。工程师待执行项目见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仍缺正式机载接替与执行结果、可信性能/耗能/净空、跨站和长期运营等原范围；本轮不增加整项目标验收数。下方432及更早为历史增量。

## 432：独立备用能量链的限定源码审阅

依据为原Draft在reserve分支只保留ID、Compiler的reserveMembers没有自身路线，而Coordinator用最长主机任务／返程代替备用。新链为可选reservePlan／自身端点→Compiler独立reserveMembers.energyPlan→Coordinator逐机账单→Scheduler原身份资格→现有Planner解释，Guided只编辑本地草稿并撤销旧审阅。

跨模块静态读回核对：policy exact8、normalize exact3、helper exact29；源schema1仍exact5，原task的8字段与可选reservePlans的第9字段分别对齐，旧源不注入默认策略。角色变化保留编辑意图，非备用不发出策略；PlanStore沿用canonical source校验和原版本身份。正常Draft／覆盖点的sequence／geodetic／AMSL与helper相容；非覆盖共享route仅用于预算专用副本，显式空／坏成员路线不回退，active members／上传不变。

helper用自身速度重新估算水平及升降顺序时长，目标驻留只计一次，待命名义等价没有默认零开销。已知巡航上限不足拒绝、未知性能标为未知；实际outbound包含自身起点，单路4096／全备用32768超限不留成功前缀。直接几何不证明净空；完整目标航点重做不等于中断点续飞，也不复制原主机起降／终态动作。

Coordinator完整结果使用待命额外开销＋共同任务下限，自己的返程与原保留量取大；不完整为-1/-1和明确标记。Scheduler合法unknown只拒绝备用而保留主机账单，required=-1、组级瓶颈未知，requireEnergyEvidence=false不绕过；坏标记／半未知仍拒绝整份结构。Risk原备用身份阻断保持，431原生电池与审批扣龄门禁未放宽。

UI静态核对实际Draft／revision／selection绑定、整份保存、两步清除、独立端点、输入未保存阻断及来源变化后的取消出口；预算字段与真实details键一致。UI/UX技能影响明确标签、取消／错误反馈及焦点可见性设计；没有运行界面、测量对比度或验收无障碍。

范围为11源码／18文档，见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。最终直接文本检查覆盖29条路径（包括未跟踪文件），全部存在；无冲突标记或行尾空白诊断。18份文档的1722个本地Markdown路径目标均存在；此检查不验证外部URL、标题锚点或内容正确性。全工作树 `git diff --check` 无输出；未跟踪文件另由上述直接文本读取覆盖。

本轮只做源码交叉阅读和直接文本／路径检查，不作语法、编译或行为通过声明。未编译、测试、运行parser／helper／UI／应用／SQL／仿真／设备，未改PX4、刷机、提交、更新PR或打包。其余25目标及六类实质源码工作继续。

## 431：原始电池来源到动态预算的限定静态审阅

本批9源码／16文档。修改依据是原Swarm平均全部BatteryFactGroup百分比而未区分用途／接收鲜度，以及Fact的旧time_remaining不能证明新报告。实际新链为原始PRIMARY BATTERY_STATUS→Vehicle独立账本→Swarm私有绑定→Registry动态快照→Scheduler原成员账单→Guided再次预算核对与Planner只读说明。

独立交叉阅读覆盖Vehicle exact20／rows15与Registry／Scheduler实际消费：单个ALL／PROPULSION新报告配明确辅助槽可保留；只有百分比、只有剩余时间、协议0／负／缺扩展和有效正报告扣龄至0保持不同含义。3秒是固定本地接收鲜度，不是远端采样年龄；相同内容重复包允许刷新年龄，不证明新测量。多推进、未知用途及第17槽不选择有利子集。原始入站／提交核完整真实源，getter改成纯内存连续性，不通过Link虚调用给原Registry读路径引入回调；不同线程UDP Link不被误加主线程限制。

Registry四个快照出口、健康计数／汇总、普通描述／metadata剥离和成功导入触达清绑定已静态核对；单行vehicleHealthChanged仍是缓存评分，不称原始电池事件。Swarm前后来源比较不按自然年龄或重复receipt变化误拒；已修复良好数据恢复后旧异常metadata残留。Scheduler保留拒绝行的合法来源，不让备用替代原主机；Guided核原plan的完整主／备用身份、稳定源／内容和两次fresh快照的原需求下限，不重写原账单。

界面按ui-ux-pro-max的状态文字、只读区别和长标识可读性指导静态复核：新增中文原因保留原码，显示审阅采样年龄而非实时倒计时；UUID长词可断行。未增加业务按钮、轮询、权限或飞行指令。独立审阅未发现确定的新生产／消费不匹配，但没有运行证据，不能宣称编译成功、UI效果、飞行安全或完整25目标完成。

本批直接文本核对：25条范围路径全部存在，16份文档共1636个本地Markdown目标无缺失，冲突标记为0，范围内git diff --check无输出。全文扫描另外发现Vehicle.h／cc共22处尾随空白，各行相同文本均已存在于HEAD；未为本批清理这些无关旧行。未跟踪文件同样纳入逐文件检查，不能仅凭git diff无输出声称已覆盖它们。未编译、解析／AST、测试、执行helper／UI／应用／SQL／仿真／设备，未修改PX4、刷机、提交、更新PR或打包。[工程师清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)中的实际固件、报文、生命周期、时效、预算与界面验证全部待执行。

## 430：逐机能量预算生产／消费的限定静态审阅

本批7源码／17文档。原Coordinator将组最长路径给每机、Scheduler可排序改变主／备用身份、原满电续航百分比式低估耗电，以及Registry的-1未知被消费者当坏值，均以实际源码路径为修改依据。新增链为实际编译成员→逐机预算→固定身份账单→Risk身份检查→规划页只读明细。

限定审阅关注：exact5输入与17字段输出、全部拒绝成员保留、同数不同机／主备互换不能通过、有效模型最小剩余秒数与独立百分比预测、未知不伪造证据、旧独立调用保留、用户原数量要求仍由Compiler保留。备用最坏主机预算明确未提供真实路线／待命模型；共同任务下限保留，不承诺所有不同路线必得不同预算。

独立生产／消费交叉阅读未发现确定的新接口阻断；这不是运行通过结论。直接文本核对24条范围路径均存在，17份文档的1617个本地Markdown目标无缺失，尾随空白及冲突标记均为0；范围内git diff --check无输出。未跟踪文件已包含在逐文件文本检查中，不能仅凭git diff无输出宣称已覆盖它们。界面按现有QGC控件与调色板核对字段、8行分页、旧预览标识与键盘滚动；补齐翻页后几何变化时的焦点可见和独立禁用调色板，未运行视觉／交互验证。

未编译、解析／AST、测试、执行helper／UI／应用／设备、仿真、改PX4、刷机、提交、更新PR或打包。源码推理不证明编译成功、UI效果或飞行安全；后续实际验证见[工程师清单](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质工作继续。

## 429：运行报告生产／消费的限定静态审阅

[报告后端](../../custom/src/Swarm/FleetMissionExecutionReport.md)与原飞前ReportBuilder分离。Root使用selected-observation调用scope中的actual previousStartup，核实际Series、Startup和Run身份；原生调用结束后再次取稳定身份，才发布冻结报告。计数与quiet时间可以推进，不比较两个完整遥测快照；这不是原子远端采样或实时新鲜性证明。

报告schema1有9个顶层字段，source3键、summary13键、每机摘要14键及claims22键。摘要由原观察重算，候选只取actual observedOutcome，不由phase猜成功；原有有界观察一并保存。build／load结果3键，save结果2键；Root四操作均返回7键，提交事实与上下文是否仍有效分离。公开报告为深拷贝，保存消费Root持有的原冻结报告。

文件最多2MiB、JSON树深度16／32768节点；内容hash按Qt紧凑JSON排除contentHash计算，不是签名或来源认证。QSaveFile关闭直接写回退并核完整写入／commit，不创建目录；load只读且保留旧报告直到完整验证成功。没有磁盘身份CAS、异步IO或断电持久化证明，相关故障与响应性留给工程师。

界面文件选择、保存确认与报告身份分开，外部打开不需要在线Run或飞行账号；导入文件绝不写回任务／审批。路径意图额外冻结reportId及contentHash，不复用同ID不同内容的文件；自身生成／打开报告不会误丢已提交回执。展开体上限18默认字体行，按需最多32机，焦点只调整滚动位置。

限定静态交叉发现并修正一处条件括号缺失，补齐Qt JSON Nullptr与原生无效QVariant空值兼容。最终直接文本核对：24条范围路径均存在，15份文档的1580个本地Markdown目标无缺失；尾随空白、合并冲突标记均为0，累计工作树git diff --check无输出。pri／Plugin／qrc／qmldir及Observation入口已逐项对照。上述证据只覆盖源码阅读和文本检查，不是编译、解析器、真实JSON往返、UI效果、性能、设备测试或放行；未运行任何此类步骤。

## 428：历史版本目录与草稿恢复的限定静态审阅

范围见[本批清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。既有PlanStore构造已经读取固定默认档案，但summaries仅返回各提案会话最新版；本批补单会话全部版本目录与精确取源，不新建数据库、磁盘加载流程或历史Deployment。原25目标和六类实质开发保持，不把本地编辑恢复等同机载恢复。

新增目录结果10键、每行10键，精确选中身份7键；取源成功10键／拒绝4键。历史identifier沿用原非控制字符且长度至多128的合同，不强制全部UUID；版本必须为明确正整数，不能以旧record(task,0)的latest语义恢复。导入／重载换storeEpoch后旧选择失效；同一epoch新增其它不可变版本不使所选旧版变成最新版。

档案是本实例已加载的历史内存快照，不宣称持续重读磁盘或全局最新。读取不要求liveReview／matchesBinding，也不清writeBlocked或修改原记录；后续重新审阅仍受磁盘基线、隔离及容量限制。完整planningSource只作编辑意图，不携带历史fleet、reviewEvidence或审批；实际新任务审阅使用当前Root的IntentTask身份，不能把历史sourceTaskId当执行任务ID。

共享427独立候选、当前／备用对象和操作ticket，历史与失败部署保留各自来源检查。新面板登记于原qrc／qmldir，置于Planner并沿用未提交输入门禁；不新增上传、启动、清空或机载恢复动作。源码与资源存在不证明QML加载、对象切换或历史JSON运行正确。

交叉静态审阅按目录10／行10／绑定7／取源10／装入7的实际生产与消费逐段核对，覆盖初始Episode且无账号／无飞机的本地恢复、选择非最新版、同epoch追加与换epoch失效、旧无源及writeBlocked保留。没有发现该限定正向路径的确定阻断，不等于运行通过。历史来源标识的去重集合改用无原型字典，避免合法constructor／toString／__proto__被误判重复；公开来源绑定改为值复制，避免摘要与私有preview共享可变对象。来源下拉和版本／错误文本显式PlainText，不把归档标识当富文本。

另定位Planner的显示缓存顺序依赖：当前Draft指针切换会同时更新成员facade和选择投影，两条独立binding不能保证原立即刷新handler读到新成员。改为本轮同步发布结束后的Qt.callLater，冻结控制器／facade／选择代次并复核busy与inputSettled；旧请求及销毁回调失效。暂时忙或有未提交输入时保留请求，仅随busy解除或输入稳定事件重排，不轮询、不覆盖未提交文本。独立只读复核覆盖了这些正常与延后路径；这是显示层修正，没有新增native任务或权限修改。

最终限定文本核对：7个源码／资源及14份文档共21条路径全部存在，整文件未见尾空白或冲突标记；14份文档正则识别的1548个本地Markdown目标全部存在。未核锚点、渲染或完整Markdown语法。整个工作树git diff --check退出0且无输出；7个源码／资源路径中4个未跟踪，另作直接全文或本批对应源码段读回，不用空diff代表检查。实际资源为custom/qgroundcontrol.qrc和custom/res/Merivus/qmldir，已核新面板路径；不是编译／QML加载证明。

界面沿用ui-ux-pro-max的明确历史选择、来源说明、覆盖确认、可取消与文字错误指导，复用QGC控件。未运行技能搜索、视觉渲染、键盘／主题／交互验证；未编译、测试、执行parser／helper／UI／认证／应用／SQL／仿真／设备，未修改PX4、刷机、提交、更新PR或打包。工程师待执行项目另列，不作为本机验证通过记录。

## 427：原草稿封存与人工修复副本的限定静态审阅

范围见[25条路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。源码检查发现旧Compiler只把task用于planId哈希、areaSummary不保留多边形，且Draft.buildTask省略备用profile／停用端点等编辑值；所以本批保存完整有界编辑源，不从生成路线反推。原25目标不因这一入口标记完成。

生产／消费合同分别为Draft source5、validator4、裸规划task8／带源task9、load7、Deployment成功取源10／拒绝4。Compiler按validator的有界预期树核八字段和允许附加项，PlanStore保持原schema并验证可选源规范JSON；两者已交叉静态核对实际Root加入fleet／intent／taskId的正常路径，未见确定阻断，但没有运行该路径。

Draft保存所有保留成员内容，sourceRevision只作历史；新实例原子提交revision1并清地图基准确认。load在通知前提交，通知后失源仍保留committed事实，current另报。Deployment仅读冻结record的源与实际426结果，partial different／unknown不冒充相同，不要求失联飞机重连；历史档案存在不等于有可恢复的本次Deployment。

Root先在原Episode调用中取源，退出调用后才装入未连接的新Draft；再次撤销旧审阅后，退出原生调用才发布当前对象。切回只清备用的地图基准确认，其余内容保留。新副本成功发布前不丢旧备用；当前／备用之外仅容许一个临时候选或待销毁对象，稳态最多两个动态对象、峰值三个动态对象加初始对象，实际销毁通知前阻止再次创建。

交叉静态审阅定位并修正完成通知的重入窗口：私有ticket贯穿publication与busy通知，普通编辑和创建／切回在该窗口拒绝；失败候选先请求释放再公布结果。发布期间来源销毁或最终对象／修订不再匹配时，ok／current降为false，已经发生的committed等事实不抹掉。创建／切回追加有界来源审计，重新审阅另关联新旧归档；历史来源不注入当前fleet／设备证据。创建／切回显示审计追加是否确认，reviewed事件没有单独界面回执，也不代表磁盘持久化。

沿实际消费者静态追读，Coordinator保留规划源并维持显式成员；PayloadBuilder不将源当航点、成员或飞行载荷，原每机2048／整组8192任务项上限不变。源会增加本地计划和归档大小，仍受原16 MiB、512000节点及32 MiB归档等相应预算约束，临近容量时可能拒绝，不能把“没有新增航点”说成没有资源影响。

界面遵循ui-ux-pro-max的明确确认、覆盖警告、文字状态、原生控件及可返回原则。技能脚本、渲染、主题／焦点／键盘／交互验证未执行；仅源码和限定文本核对，不能证明Qt类型往返、正常操作、动态对象销毁或协议行为。

最终限定文本核对：8个源码与17份文档共25条路径全部存在；直接整文件检查未见尾空白或冲突标记。17份文档正则识别的1512个本地Markdown目标全部存在，未核锚点、渲染或完整Markdown语法。整个工作树git diff --check退出0且无输出；8个源码中7个未跟踪，另作直接源码读回，不能用空diff替代。界面静态补上隐藏、换控制器、换Episode时清除三项确认及四个按下意图，隐藏时不接受本地修复；这些检查不验证运行行为。

未编译、测试、执行parser／helper／认证／UI／应用／SQL／仿真／设备，未修改PX4、刷机、提交、更新PR或打包。新草稿不授飞行权限；完整受控重新部署、逐项修复建议、跨重启历史源操作入口以及其余六类／25目标仍有实质工作。

## 426：失败部署独立整组核对的限定静态审阅

范围见[29条路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。实际Deployment私有唯一Reconciliation复用原Episode来源，新增明确request/cancel与只读属性；Root接独立角色快照和原来源回调，Panel只显示有界摘要。Episode静默切换和新发布门槛纳入子对象忙碌／排空，不扩大原服务集合或QML可替换对象。

本轮按生产／消费源码对照：独立传输35键结果、批次30键摘要、32键接受回执、逐机16键；普通缓存、旧Start收据、425解码差异和新RAW摘要分离。重点阅读op=0准备、operationStarted绑定、完成栈退出后的take、COUNT0、数量差异、unknown、分配前资源上界、取消及原作用域清理。没有运行这些路径，不能据静态拒绝分支声称正常流程已验证通过。

交叉审阅指出并跟进：撤销旧批准会改变PlanStore.record的动态liveReview，故原不可变内容核对与本次post-revocation记录冻结分开；原已捕获PRIMARY不能被同号／新链路隐式替代；已cancelled的Deployment仍需明确取消活动诊断；界面观察总数上界与原生8192保持一致。取消只停止后续读取，最终占用释放还须原协议／发送栈排空，不是远端停机证明。

界面遵循ui-ux-pro-max的明确确认、可取消、文字状态、原生控件和按需来源详情。未运行技能脚本、渲染、主题／键盘／无障碍／交互测试。只允许源码读回和限定路径／空白／Markdown目标检查；这些不验证语法、构建、QVariant映射、运行时重入或设备。

最终限定文本核对：29条路径全部存在，16份文档正则识别的1421个本地Markdown目标全部存在；git diff --check退出0、无错误。整文件直接扫描仍见PlanManager.cc的7行、Vehicle.h的9行、Vehicle.cc的13行尾空白，未改动这些无关行。13个源码／工程路径中6个未跟踪，已另作直接源码读回，不能用空diff代替它们的检查。独立复核另覆盖op=0准备至完成栈退出后take、原PRIMARY控制块、双发送栈排空、独立NOTIFY、旧审批撤销与UI取消；这些仍不是运行结论。文档目标存在不验证锚点、渲染或所有Markdown语法。

未编译、测试、执行parser／helper／UI／SQL／认证／应用／仿真／设备，未修改PX4、刷机、提交、更新PR或打包。修复草稿、完整执行闭环及原25目标其余实质工作仍未完成，不按批次号、文件数或本次摘要验收整体。

## 425：回读诊断与真实完成依据的限定审阅

范围见[本批清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。新helper只输出有界纯值差异；Deployment在实际回读回调采集并保留来源，界面只读显示，内存history使用摘要。既有Root原生值投影无需变更，custom.pri登记新h／cc。

源码审阅发现：PlanManager普通READ失败也发布availability信号并留下空cache，因此新增私有完成栈查询，以原作用域、最终success、末ACK及COUNT排除“失败空缓存＝真实空任务”。这不是机载真实性或完整性认证，旧通知之前的任意缓存值变更也不能据此当独立RAW来源证明。

本批检查范围为有界采集、原来源／操作号、回调后失效、历史保留与摘要、typed差异／未知／截断、QML按需展开和工程登记。比较报告28键、差异行9键、历史摘要27键和原生捕获包装16键已按生产／消费源码逐段对照，不以源码中有检查分支代替实际运行通过。

同行只读复核完成栈时序：普通READ的COUNT含真实零，最终success与COUNT在通知前冻结；operation-id回调仍处于finish栈，原scope未被quiet清除，公开transaction已为None／activeOperationId为0。正常读取不因该次finish栈自行被拒；失败false／-1与下一scope初始化／排空复位分开。没有执行这些时序或据此认定运行正确。

主审阅另纠正界面措辞：observed-empty不是双方规范化成功；invalid-number也包括超出安全显示范围的有限数；itemIndex是列表比较位置而非观测到的sequenceNumber。新功能无Guided接线改动、无新读取／飞行按钮；原StartContentCheck不能用于证明失败后的独立对账已经实现。

最终限定文本核对：23条路径全部存在，15份文档正则识别的1353个本地Markdown目标全部存在；git diff --check退出0、无错误。直接整文件扫描保留PlanManager.cc中的7行既有尾空白，其他本批路径没有尾空白；没有为此改动无关旧行。8个源码／工程路径中5个未跟踪文件另作直接读回，不能用空diff冒充检查。路径、链接和空白结果不验证语法、QVariant往返、构建、协议、UI或设备行为。

界面采用ui-ux-pro-max的逐机错误反馈、文字状态、原生控件与按需明细指导；没有运行技能搜索、界面渲染、无障碍／主题或交互测试。未编译、测试、执行parser／helper／SQL／认证／应用／仿真／设备；未改PX4、刷机、提交、更新PR或打包。完整25目标及失败后独立对账／修复草稿继续，不标记整体验收。

## 424：跨库扫描的限定静态审阅

范围见[20条路径清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。新增独立search值合同、复用原只读读取器和真实全局查询槽、Root最小只读facade及明确操作面板；不改数据库schema、消息Codec、PX4或飞行命令路径。

本批静态核对重点：外部6字符串意图与内部5键请求分离；原生私有续扫游标；scope/筛选/实际queryId绑定；最多4次页读取、256行消费和64条匹配；按实际消费位置推进且不跳过字节超限的未返回匹配；scanComplete与空匹配分离；文件目录前后元数据复核、正在分页文件的整文件指纹和epoch；未匹配行的留存/unclean/drop/discard覆盖摘要；取消等待真实线程结束与独立结果属性。

目录/哈希只是限定观察，不证明跨库原子性、历史全集、签名、物理机型或控制权。覆盖计数不是去重后的丢失事件数量；文件名/文件内rowId不当飞行时序/全局ID。UI遵循明确范围、可取消、原生控件、换行及文字状态，不据此宣称视觉或无障碍实测通过。

Reader与Store已相互对照原生5键请求、28键结果、18键扫描摘要、7键来源包装和原8键行；检查首批空目录、续扫、无匹配前进、4页上限、字节边界和取消的源码分支，没有执行这些场景。主审阅另核Root窄facade与Panel结果归属。

静态对照发现并修正UI把原生整型vehicleId与请求字符串直接比较的问题：空字符串只在请求层映射为0，结果仍验证0–255整数。跨文件续扫的afterRowId=0允许空epoch/文件指纹；不能按文件内后页的非空要求误拒合法边界。Codec从JSON还原的整值Double只在已校验历史载荷筛选中处理，不放宽原生请求类型。字节预算在消费超限匹配前停止，保留其后续读取位置。

最终直接文本核对：本批20条路径全部存在，14份文档中的1278个本地Markdown目标均存在，尾空白0项；git diff --check返回0、无输出。6个源码路径中5个仍为未跟踪文件，另有已跟踪的GuidedActionsController.qml，因此直接读回和路径清点不能由空diff替代。以上只证明限定路径/文本结果，不是语法、构建、数据库或运行验证。未执行编译、测试、helper、SQL、UI、认证、应用、仿真或设备操作；未修改PX4、刷机、更新PR或打包。


## 423：离线未封存库的限定源码审阅

本批复用Reader、Store和既有Panel，增加固定未封存库的有条件离线查询，不修改SQL/schema、记录Codec、飞行固件或飞行命令路径。范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

限定审阅覆盖：Reader精确offline请求、固定filename、同Writer协作锁、只读连接与无sidecar、actual epoch/整文件指纹/后页绑定；Store实际writer退出与全局查询槽、首读/后页来源、取消等待finished和结果隔离；Panel默认无IO、三种页类型、独立按钮、禁用原因、原页游标和ID匹配。不能以这些源码条件证明运行行为已通过。

交叉阅读发现并修正离线readPage参数使用默认QString导致typed-null的兼容风险：现传明确非null空字符串，契合Store对空runFilter的严格验证；新增offline请求也拒typed-null。手工对照原SQL页20字段与离线新增8字段，Store接收合同为exact28，最终页字节字段仍按去掉自身字段后的JSON长度计算。未执行代码往返或数据库实验。

数据库无写入口，但QLockFile会创建/释放锁文件。setStaleLockTime(0)不按年龄判陈旧，不等于证明Qt绝不处理死进程锁。锁、规范路径和前后hash不证明恶意TOCTOU/ACL防护；SQLite调用和读盘本身不能被原子取消标记强行中断。

沿用UI技能的显式操作、可见状态、键盘焦点与按需详情规范，不运行技能搜索helper或渲染。未编译、测试、解析器/helper/SQL/认证/UI/应用/仿真/设备，未刷机、提交、更新PR或打包。工程师实际验证项目见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

冻结后限定文本核对：4源码、14文档共18条路径存在；直接逐行检查尾空白0；14份文档正则识别1211个本地Markdown目标，缺失0，不核锚点或渲染。git diff --check退出0且无输出；4个源码均未跟踪，因此另有直接源码读回，不用空diff证明它们通过。Reader与Store交叉核对6/28字段、明确空串、固定目录与锁、字节预算/游标及取消生命周期；Panel再按实际源核首读/续页条件，均仅静态结论。

本批不覆盖热日志恢复、未知schema迁移、全库检索、完整历史证明、自动留存/配额管理或原25目标整体验收。

## 422：显式归档入口的限定源码审阅

范围为[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)中的11源码／资源、16文档。交叉阅读当前Store产品动作、Codec工程摘要、Root实际连接、Panel请求/结果归属和两个宿主资源。无新飞行指令、权限恢复或数据库schema变更；不是整个仓库的正确性或安全认证。

本批审查定位并修正开始／停止／封存的空requestId回执被UI误判的问题；查询与动作ID分合同。原生在launch前预留本产品query ID，避免嵌套result/finished先到而返回后重装已退休槽。产品current取消与旧失源取消分开，仍等原实际请求结束；Reader取消仍等实际线程结束，不用UI取消冒充完成。

面板另补按下后的目录/选段/epoch/sealRun与原页游标尾核，检查时保持callBusy，不短暂解锁；读取器state/errorCode只按本页queryId解释，避免目录失败被旧录制错误覆盖。Root既有初始化仍启动内存BlackBox，新磁盘归档默认关闭且必须明确启用，两者没有混为同一录制状态。

工程审批前比较与实际提交回执分开传入固定摘要，基线审阅计数不取提交后的零差异。Record返回只证明实际BlackBox内存接纳；没有把原EvidenceChain接到SQL的假设。摘要内容/类型和新v2解码受限，旧v1不能凭空补详情；任一日志失败不改写原已冻结提交事实。

UI按ui-ux-pro-max静态规范复用QGC控件与调色板，默认确认关闭、明确取消、文字状态、虚拟列表与按需只读详情；未执行技能搜索helper、渲染、焦点/对比度/读屏或尺寸验证。Qt类型、原生线程、SQLite、文件系统与真实回调时序均仅源码推断。

限定文本检查：27条路径均存在、直接逐行读取的尾空白为0；16份文档正则识别1162个本地Markdown目标，缺失0（不验证锚点、渲染或所有Markdown语法）。git diff --check 返回0且无输出；新Panel在qrc/qmldir及两个宿主页的文本登记均存在，不等于资源编译通过。未跟踪文件另作直接读取，空git diff不能证明其正确性。未编译、测试、运行helper/解析器/认证/UI/SQL/应用/仿真/设备，未修改PX4、刷机、更新PR或打包。

未覆盖：离线未封存活动库、完整审计/构型历史、自动留存/总配额、恶意目录TOCTOU/ACL、断电持久性、全库索引和全部25目标。工程师后续场景见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 421：基线操作链的限定静态审阅

范围为[本批9源码/17文档](MERIVUS_SWARM_PATCH_MANIFEST.md)。Registry、Review/Session/Role 与 Root/Panel 已交叉阅读：首次无旧基线可批准当前完整值；unknown当前只允许实际原批准撤销；原生保存raw批准2–5键不补造历史，独立条目指纹与比较16仍持续冻结。完整配置、比较DTO和Root展示复制使用各自预算，不把原raw域2048节点限制误套大报告。

已发现并修改一个正向兼容问题：Panel原来只接受字面local_approval和原样小写hash，误拒原生允许的旧source空白/hash大写或空白记录。现在先核字符串类型，界面验证/关联时trim及lower；原冻结map和显示原值不赋新值，最终native按原记录严格核对。

独立核对四操作与三种空能力清单、精确purpose、两policy稳定sourceToken、原30秒/TTL、最后纯围栏、samebaseline也退役能力批准以及先冻结receipt后notify。记录原生mutation、stillCurrent、工作流ok和审计追加的区别；跨对象删除/耗尽后的真实提交事实不被错误返回抹掉。限定范围未见其它已确认阻断，不代表全仓无缺陷或可构建。

旧bool原生接口仍受信兼容，并非所有入口都有双账号；完整typed快照未持久化。语义路径仅做源代码阅读，没有执行场景。未编译/moc/QML解析/AST、测试、helper、OS认证、UI/应用、SQL、仿真或设备；未改PX4、刷机、更新PR或打包。原25目标/六类仍有实质开发。

### 本批文本检查记录

9个源码与17份文档共26路径逐一直接读取，缺失0、尾随空白命中0；整仓 git diff --check 无输出、退出0。17份文档的Markdown行内链接正则匹配1120个本地目标，缺失0、路径解析异常0。只验证匹配到的目标存在，不验证锚点、所有Markdown语法或内容真伪。未跟踪文件按清单直接读取，不宣称git diff覆盖新增文件；这些是文本核对，不是语法解析、编译或行为测试。

## 420：完整基线消费与审阅身份，限定静态审计

审阅范围为[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)本批12源码与20文档，不是全仓安全审计。实际源→快照→值校验→四类消费者→旧/新指纹→审阅归档→Deployment关联均已读回；多人独立交叉阅读未发现限定范围内新增的确定阻断。这不证明可编译、Qt/QML绑定正确、运行无误或安全放行。

具体核对：健康/重复同配置不滚动历史；hash-only/clear/恢复推进观察身份；批准/撤销/导入触达更新或退役私有记录；入站/导出不把只读字段变成metadata权限；最终成员/备用机/原成员/替补和排序共用共享函数。旧configurationState保留报告意义，不能再单独提供正向基线资格。

Coordinator schema2/member9整体归档，PlanStore既有有界树与reviewEvidence hash保留新内容；Deployment不把captureComplete或baseline bool强制加给可选政策。Root纯值桥对原输入取身份，不做当前ID查找，缺桥/异常/换reviewer返回空指纹。桥无Registry访问/信号/授权，保留原实际源检查。

已知边界：Discovery.validateDescriptor仍校整个输入的2048节点，不能假设接近原上限的完整Registry输出投影再回送该外部验证接口必然通过；当前实际Swarm buildDescriptor链并不这样回送。新证据只在Registry读出口添加，登记/更新/导入先strip，原存储预算未放宽。没有新增持久化或基线操作者界面，静态源/值一致性不认证实物与性能。

限定文本/本地链接检查结果在本节末另记。未编译、测试、运行解析/AST/helper、应用/UI、认证、SQL、仿真、设备；未改PX4、刷机、更新PR或打包。原25目标/六类仍未完成。

### 本批文本检查记录

12个源码与20份文档均直接读取并检查尾随空白，32路径存在且尾随空白命中0；整仓 git diff --check 无输出、退出0。20份文档按 Markdown 行内链接正则匹配1069个本地目标，缺失0；只证明这些匹配目标存在，不验证锚点、内容正确性或所有Markdown语法。未跟踪文件已按清单直接读取，不声称 git diff 自动覆盖新文件。上述都是文本核对，不是语法解析、编译或行为测试。

## 419：构型比较端到端的限定静态审阅

实际读取Registry观察/merge/register/基线/导入/clear/能力提交，新增typed helper，Review冻结/账号前后/最终fence，Root现有值复制预算及Panel schema2/18与懒加载差异区。未执行编译器、moc、QML解析、AST、helper、认证、应用、SQL、设备、仿真或测试。

静态审阅发现并修正两项：hash-only同hash后残留map仍可被新基线批准；approval-only导入只换观察而未退役旧能力记录。当前前者提交前拒绝，后者在staged中同时退役能力record/history；full丢失/恢复也失效原能力批准。最终能力围栏另核原history epoch/revision/exhausted，失败合并不提前写live。比较缓存不在每次审阅getter重复遍历。

限定文本检查：本批8源码/登记尾随空白0；7个C++/QML文件在粗略移除注释/字面量后的三类括号字符数量配对；git diff --check无输出。15份文档尾随空白0，正则匹配的982个本地链接目标均存在。字符计数不是语法解析、构建或运行证明，新未跟踪文件另行全文阅读。文档链接只检查匹配到的本地目标存在，不认证内容、锚点或完整链接语法。

遗留明确：旧requireConfigurationBaseline仍只看旧哈希状态，需下一阶段统一实际有类型证据消费；没有持久化构型史、字段完整性认证或基线批准UI。Qt映射、显示性能、内存/生命周期和真实工程场景均未验收。原25目标和六类实质代码缺口继续；不把8文件/419编号当完成比例。

## 418：能力声明审批限定静态交叉阅读

[本阶段范围](MERIVUS_SWARM_PATCH_MANIFEST.md)为16源码/资源、17文档。实际调用链已逐段阅读，Registry与Review/Session/Root/Panel进行独立交叉阅读；没有执行这些故障场景，也不是全仓安全或可构建证明。

限定阅读核对：实际Swarm拥有原Registry/Review；专用账号用途、角色、原TTL、原scope及准确输入；普通健康更新不改静态依据；最终纯fence后Registry比较原私有状态并先固定回执再通知。原context6/approval14没有加字段。旧bool不是全入口账号授权，新fence重载也不自行认证账号。

审阅中实际修正：合法配置可能超出目录显示集合边界或包含Qt字节/日期类型，故原配置留native并逐层type/null/value比较，展示改只读configurationJson，能力集合拷贝使用2048上限而目录默认不变；同时冻结发现来源/状态/错误。Root保存真实账号证据与审计状态，不从后置角色显示反推批准者。

Swarm刷新链冻结原Registry/QPointer及self/MVM/model/Vehicle；在直接相关原生观察、插件能力/构型getter、注册/更新/通知后再核，原对象丢失即停止或跳过，不按同ID找替代对象。只覆盖本次已读直接路径，不宣称全部历史回调安全。

限定文本检查：13个C++/QML文件去注释与字符串后的三类括号字符计数配对（不是解析或编译）；16源码/资源均无尾空白。17份文档中989个匹配的本地链接目标存在，包括历史目录链接，无文档尾空白；git diff --check无输出。新未跟踪文件另行全文读回，不能用空Git检查冒充覆盖。两个宿主入口及pri/qrc/qmldir登记已直接核对。源码静态核对不能证明Qt绑定、moc/链接、账号窗口、倒计时或实际部署。工程师矩阵见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

未编译、测试、运行解析/helper/UI/认证/应用/SQL/设备，未改PX4、刷机、更新PR或打包；原25目标/六类仍有实质源码缺口。

## 417：工程目录重载限定交叉阅读（非运行验收）

当前[19源码/资源及19文档范围](MERIVUS_SWARM_PATCH_MANIFEST.md)包括全局坐标航线估计器门禁、Catalog 纯预览/围栏导入、Reload、Session/Role、Coordinator、实际 custom Guided、完整声明面板及两宿主/资源登记。原生调用链与界面/Root合同已做限定人工独立交叉阅读，不是执行故障场景。

限定文本清点：16个C++/QML文件去注释与字符串后的三类括号字符数量配对，不是语法解析或编译证明。15份源码无尾空白；Vehicle.cc发现13处，与HEAD原有尾空白一致，未为本任务整理无关代码。19份文档所匹配的本地文件链接目标存在，无文档尾空白；git diff --check无输出。新未跟踪文件另行完整读回；空Git差异不代表覆盖新文件。两个宿主、qrc/qmldir和pri新增h/cc登记均在实际custom路径直接核对。

最终交叉阅读还修正两处调用生命周期：Root idle issued仅允许review，preparing须原request调用、approving须原approve调用；active须真实factory controllerCallMatches和current purpose。Reload在进入approving后、弹OS提示之前重新核此来源。request在active清理及endControllerCall通知后再次核原issued/state/期限/receipt/context；Panel也只接受当前review，不能把已取消同收据装成成功。无运行验收结论。

已实际发现并修正、读回的本批点：Qt 5 下新 Reload BusyGuard 改同型 QPointer 以消除跨类型聚合转换疑点；Review30秒窗口在目录 snapshot/hash 之后及最终认证/提交围栏末尾复核，不只依赖早期检查；新增 Root 使用的实际 Session/Role 与冻结 primary5/role9 纯 contextMatches，防请求源尾窗。Catalog 预览与导入共用严格 stageArchive，保留最大64/1 MiB、重复键预拒、整批重复声明拒绝、revision耗尽不回绕；最终 fence 之后先核原 QQ/epoch/revision，再不可逆替换。

限定读回看到完整旧/新 engineeringReview、差异/双哈希进入绑定；Session 专用 purpose 的一次消费及原 TTL 末核与 Reload friend 接口一致。Catalog 已提交 receipt 在 changed 同步重入/销毁后仍保留，Reload 同样保留 native mutation 事实；旧任务 review 的失效、外层后置状态及 audit append 不能抹掉它。固定文件的原生读取、目录/文件身份与最后同 rawSHA 复读仅证明有限观察，不是签名、恶意 TOCTOU 防护或账号授权的替代。

原启动自动 load 未改为强制两账号，所以本批只声称运行中显式 reload。全球任务 healthyReference 与 Vehicle 私有启动投影另要求 ABS 水平/垂直且非 CONST_POS，未改变 RAW/通用 Health/室内/GNSS 硬件范围。UI 静态说明和接线不能证明 Qt 绑定、布局、可访问性或 OS modal 的实际行为；验收矩阵见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

未编译、测试、运行解析器/helper/UI/认证/应用/SQL/设备，未修改 PX4、刷机、更新 PR 或打包。此记录不是全仓安全审计/可构建证明或全部25目标完成；六类实质源码工作继续，下方历史保留。

## 416：逐机执行配置链与限定静态收口

本阶段范围见[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)：11个源码/资源路径、15份文档，共26路径。实际调用链已人工读回，并进行独立只读交叉审阅；不是可运行或可放行证明。

限定文本结果：9个C++/QML文件去注释/字符串后的花括号、圆括号、方括号字符计数均配对，尾空白均0；这不是语法解析。15份文档扫描到843个本地链接目标，缺失0、尾空白0。`git diff --check`无输出；原有未跟踪文件另做直接读回，不能仅靠Git差异检查覆盖。qrc/qmldir登记与实际Planner实例已直接检查。

交叉审阅发现并修正本地Draft保存通知使自身原生调用失效的问题：现在只读观察调用结束后才提交本地CAS，旧审批仍全部失效。新参数观察在所有Link虚调用后以原生whole12内存/语义revision/值/来源/Fact/末尾整组年龄再核；两个被调用观察getter也保留原对象、行副本和更新代次，虚调用后先检查存活。未修改参数接收、刷新或写入流程。编辑器另补键盘StrongFocus、清除二步确认、输入错误和原页面/成员/草稿归属检查。以上仅限定源码核查，不是全仓回调或安全认证。

实际待核链包括 Draft 准确 revision 的完整 profile 规范化 CAS、提交先于通知且 false 不保证未提交、保留成员/角色/reserve/删除重加语义；Guided 原作用域读取 Coordinator 的19键纯值观察，保存前再读相同指纹与参数哈希，再使原 review 失效并交 native Draft。观察须12行真实 fresh/usable、原 native 对象及PRIMARY共享来源一致；不能用仅内容可知的非空旧 hash、缺 profile 的循环 `parameterEvidenceUsable` 或适配器匹配替代这一检查。

本批同时限定硬化 ParameterManager 单项 getter/source matcher 的原QQ/来源复核，并增加 `parameterObservationsMemoryMatch`。Coordinator 全部来源虚回调后以该整组纯内存尾栅核12行semantic revision/type/value、原PM/PRIMARY共享身份及年龄，再构造成功DTO；它不调用Link虚函数/OS/参数请求或写入。保存桥先结束只读调用才做本地Draft CAS，仍保留changed使旧review失效。这些是实际源码合同，未执行相关故障场景，不表示测试通过。

Editor 仅核心字段结构化，高级完整 startup JSON 1–3 仍由操作员提供；空政策不准启动，无默认容差或隐式 Home。UI 技能影响标签、明确确认、错误反馈与原 palette，不是界面可用性证明。只读观察不是原子参数事务、硬件/机载认证或最终 StartPermit；后续原生审阅、净空、固件目录入口与正式结果仍有实质工作。

本机没有编译、测试、运行解析器/helper/UI/认证/应用/SQL/仿真或设备，没有改 PX4、刷机、更新 PR 或打包；原25目标/六类继续。以下按旧批次保留，不能覆盖本节“draft 已带逐机 profile”的当前合同。

## 411–415：恢复面板接线限定静态审计

范围为[清单](MERIVUS_SWARM_PATCH_MANIFEST.md)9源码/资源、18文档。已人工读回实际Panel、Guided恢复方法、Review controller模式及Swarm诊断；独立只读复核报告并推动修补：初始invalid QVariant显示actor使严格复制器拒绝的正向阻断、取消未接原scope/身份清理、后置BlackBox异常丢原生返回事实，以及缺少实际资源/界面入口。

当前实现只在固定27键显示层规范两个未知actor，不把核心未知证据改为空；审计bool/异常与原操作结果分离，原收据/历史解除事实不被抹掉。原生5 QVariant来源回调在source读取和OS核验前后，最后整组ground/worker/occupancy/身份TTL提交尾栅仍纯native，不在该尾段调用QML或OS。旧兼容入口非C++-only隔离，产品面板仅用ForController。

UI/UX技能仅用于静态默认取消、确认不预选、危险Enter不提交、焦点与错误文案审查，没有运行技能脚本或界面验证。9源码/资源接线不认证运行成功、可访问性、物理安全或25目标完成。未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真或设备，未改PX4、刷机、更新PR或打包。

### 411–415 限定文本清点结果

- 本批4份QML与3份C++源码/头文件逐份读回：尾空白0；移除注释与字符串后的三种括号字符数量分别配对。这只是字符清点，不是QML/C++解析、类型检查或可构建证明。
- 常规native current/result与Swarm诊断文本项分别27/13/13；ForController声明/定义/Root调用各1，旧入口Root调用0，Direct调用5个QVariant且要求非空Bool true。Root来源回调中账号查询/认证/控制器占用获取文本匹配0，Panel直接原生方法调用0。
- qrc alias、qmldir与两个宿主入口各1，文件目标存在；两个人工确认默认false。原scope草稿清理、身份变更前与lifecycle来源清理各有接线。实际第二宿主链已读回FlyViewMap popup，而不是误称原PlanView。
- 复用原生尾栅限定清点：current-unknown remove/erase/clear调用0，restart/history remove各1；原whole-set final调用1、mutation尾核2。原Vehicle source/ground及Session TTL尾段未发现被检索的transport/OS/notify调用；此结论不外推到全仓或运行时。
- 本阶段18份文档793个本地文件链接目标存在，无新增尾空白。指定路径git diff --check无输出；新未跟踪文件另行全文与文本清点，不能用空diff代替对它们的验证。
- 独立只读审阅还推动补齐发布、日志及调用结束通知后的最后原issued复核；最后通知销毁Root时只改纯值结果。已返回localHoldsReleased继续保留，不以尾部失败宣称未发生；创建但失效的review返回ok=false且保留原收据/nativeRequestAccepted。没有执行这些故障场景，仍须工程师按矩阵验证。

## 406–410：原生重启／历史占用专用复核

[恢复复核合同](../../custom/src/Swarm/FleetOccupancyRecovery.md)新增实际Swarm唯一拥有的原生工作流，公开只读对象和request/独立审批/取消入口；未加QML操作界面。bootstrap历史待审与本轮真实unknown分集合，整组最多32个选中地址只可解除restart/history holds，真实unknown、实际owner、任务、临时任务、隔离和everObserved历史不清除。

原Vehicle/MM/参数/VLM/MVM/model及主链路共享所有者冻结，新鲜2秒内原始HB未解锁与已知ON_GROUND、完整本机队列排空进入30秒复核。专用fleet.occupancy.recover.approve强制不同真实账号，一次消费不生成lease；原账号TTL、当前Episode/角色在最终无回调尾栅再核。现场处置编号和地面检查是明确人工声明，不认证物理停机、远端无任务或控权。

所有可回调读取后，整组再以纯native原RAW语义revision/hash/value/鲜度/队列核对，并核原worker、实际占用和任务map；同栈整体移除本地待审后bump，新actual身份须由原worker实际提交才恢复既有continuity getter/CAS。留存原QQ、源epoch与primary控制块，换源在getter/最终CAS即时重加hold，不只等timer；正常收包、合法起飞和新MM工作不冒充换源。

本批12源码/工程登记、17文档，共29路径。原生recoveryWorkflowImplemented与UI集成分开；localHoldsReleased仅历史动作事实，selectedReviewRequiredVehicleIds另报当前待审。仍缺UI、离线/报废现实处置、本轮真实unknown受控解除和完整机载自愈；完整25目标/六类实质开发保持。未编译、测试、运行helper/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方401–405及更早是历史状态，不以旧“无任何再审入口”覆盖本批native入口，也不宣称全部恢复产品已完成。

### 406–410 限定静态收口结果

本批源码/工程登记12路径、文档17路径，共29；精确范围见补丁清单。文本读回覆盖24个C++ h/cc和1个关联QML，共25路径，工程pri另清点；注释/字符串剔除后的括号仅作字符计数，不是语法解析、编译或测试。全部计数配对。Vehicle.h/cc原有行尾空白9/13处与HEAD数量相同，未作无关清理；其余本次清点源文件和pri行尾空白0。git diff --check无输出。

实际文本清点：Swarm创建原Recovery对象1处，request/approve/cancel各1定义；Session专用consume及调用各1、纯原TTL回查定义/调用各1；最终整组finalCurrent入口1处，Swarm原生mutationBinding尾核2处；真实_continuityUncertainVehicleIds的remove/erase/clear全文件0处，restart和restoredHistorical各仅1处专用remove。两组新h/cc各在pri登记1次。Ground/Source/SessionTTL限定纯尾栅中所清点的Link虚/OS/emit/发送调用0；这只是已明确模式的文本清点，不证明不存在一切间接调用或未来改动。

17份文档的723个本地文件链接目标均存在，无缺失和行尾空白；不核远端URL、Markdown锚点正确性或内容运行效果。三个并行参与者对实际来源、Session用途/TTL、整组最终尾窗及Swarm提交作限定交叉读回；发现的历史来源混用、瞬时RX误重hold、队列准入不足、整组尾RAW、原TTL/Episode/Role和回调存活问题已修改并读回。限定范围未发现尚未解决的确定静态阻断，不是全仓认证或运行通过。

原生入口存在但操作UI、离线/报废现实处置、真实本轮unknown解除及完整机载自愈未完成。localHoldsReleased为历史动作事实，新actual仍须真正commit，文件提交不称durable/WAL覆盖。原25目标与六类缺口不缩减。未编译、测试、执行helper/解析器/OS认证/应用/SQL/仿真/设备、修改PX4、刷机、更新PR或打包。

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

### 本轮限定审阅范围

限定读回覆盖actual源容器与非wrap revision、最早通知前更新、原指针生命周期及已知unknown不被清理掩掉、重入创建栈暂存、helper9/entry5/hash8/identity4、双向pin和旧新入口最终old∪new检查，以及实际Guided两条消费路径。

同因回调读回另外覆盖9处事务创建调用、4处直接编队发送调用及停止入口、直接编队通知/Idle/暂停链、Goto前置清理/取消同步链，以及临时任务上传/清理两个返回尾窗；包括CommandTransaction发送者自身的通知后存活/原attempt检查，不只检查调用者。发现并修复了批首未冻结后续清理目标原record、上传op0失败通知/pause后未复核原record两处换代窗口。其它历史异步timer/完成handler未扩大审计，不能据此称全Swarm或全仓回调安全。

本轮最终限定文本记录：

- 实际12个源码/工程登记路径、15份文档，共27路径。另读6个关联头/源；16个C++头/源与1个纯控制逻辑QML共17路径，正则剥注释/字符串后三类括号左右字符数相等，尾空白0；pri尾空白0。这只是字符清点，不是语法解析、编译、测试或逻辑证明。
- getter声明/定义各1，基础map8键另加fingerprint1键，entry5键、稳定identity4键；getter内新增占用mutation调用0。新helper h/cc在pri各1；仅值一致性，不认证来源或真实空闲。
- private admission声明/定义/Orchestrator调用各1；reverse bind声明/定义/Swarm调用各1。新六参CAS声明/定义/Guided调用各1；Guided旧五参调用0、实际快照读取2、requireNativeOccupancyEvidence=true两处。最终实际old∪new检查及原生提交之间无通知或外部调用，仍只是本地模型提交。
- 9个事务创建调用均有返回首self检查；原目标冻结helper、实际原事务helper声明/定义各1。直接编队发送调用4处；其外层/停止入口和直接通知链经实际读回核原self/session/phase/generation/members，不能由正则数量推导全部Qt重入正确。
- DispatchFacts纯值helper1处，unknown字段定义1处；Land/RTL失效单目标不整组阻断，其dispatchedIds仅本次安排timer。清理批未处理来源复核循环1处；上传失败原来源guard定义1、通知与pause后调用2处，变代不继续清新record。原事务失效/中断不抹已安排或可能issued事实，也不认证ACK/远端结果。
- 最早progress/attempt/status占用观察器各1；unknown remove/erase/clear调用0，暂存token失败直接否决命令的旧模式0。诊断失效只关闭占用快照/连续性提交，不新增Land/RTL/ABORT否决。
- CommandTransaction transition标志1、start/retry重入入口检查2、固定原nextAttempt1，旧live-attempt+1通知模式0。逐通知/私有可通知调用后核原QPointer/attempt/status，timeout另核retryCount；计数上界不回绕，不新增cancel的busy否决，原terminal规则保留。true只保留接受的本地事实，不认证后续仍Running或飞机成功。
- 15份文档654个本地Markdown文件目标均存在、尾空白0；只查文件目标，不认证锚点/网页。git diff --check无输出；未跟踪源码另经直接读取/上述文本清点，不借tracked空diff代替检查。

两组独立限定源码读回与根代理复核未留下已确定的新增静态阻断；不把这个结论当作穷尽审计、可构建/可运行或飞行安全证明。本机没有编译、测试、解析/执行helper、业务/OS认证/Qt/QML/应用/SQL/仿真/设备，没有修改PX4、刷机、更新PR或打包。完整25目标/六类仍有实质源码缺口，目标保持未完成。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](../../custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

### 本轮限定审阅范围

限定读回覆盖资格图与当前能力记录一致性、增广一对一/资源边界、原生任务角色闭合、全缺口完整提案、批内建议承诺与实际占用分离、稳定建议身份、角色CAS完整提交及Guided实际消费。复核中统一了显式role原串必须canonical、候选已知链路质量与同任务阈值一致，以及实际消费的政策bool/阈值进入建议身份；未把裸角色/健康标签当认证。

本轮最终限定文本记录：

- 实际10源码/工程登记触点，另读6个关联头源。14个C++头/源和1个纯控制逻辑QML共15路径，正则剥注释/字符串后三类括号左右字符计数相等、尾空白0；pri尾空白0。这不是语法解析、编译、执行或逻辑证明。
- Matcher声明/定义各1、输出初始化7键；新h/cc在pri各1。Role和Continuity各有1处实际match调用；匹配器不检查健康/能力来源也不授真实占用。限定所读代码没有Qt slots变量误用。
- 新角色CAS声明/定义/Guided调用各1、同为五参；角色规范器声明/定义各1、create/重分配共同调用2。Guided完整覆盖检查1、旧ID-only CAS调用0；旧remainingCandidates.first替补模式0。这些清点仅辅助已读路径，不推导所有Qt运行绑定正确。
- 建议身份中4个策略bool、3个阈值及rolePolicyPresent各插入1次；角色/完整候选与映射/批内排除亦已读回绑定。跨任务claims仅限当前建议批次，不当实际Swarm/机载租约。
- 16份文档640个本地Markdown文件目标均存在、尾空白0；只核文件目标，不认证锚点/网页。git diff --check无输出；未跟踪源码另经直接文本读取，不用tracked空diff替代其检查。

多组限定源码交叉复核未留下已确定的新增静态阻断；这不证明可构建、可运行、真实自愈或飞行安全。未执行匹配器、Qt/QML、解析器、编译、测试、业务/认证/helper/SQL/应用/仿真/设备，没有修改PX4、刷机、更新PR或打包。完整25目标/六类仍有实质源码缺口，目标保持未完成。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](../../custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

### 本轮限定审阅边界

本轮限定人工式源码读回关注：原生私有批准与值校验不混为来源认证；6键上下文/14键审批的revision/epoch/构型一致性；非空批准、导入声明化与严格回显；静态依据变化失效、撤销、ABA及溢出；旧metadata别名不阻止合法新静态更新；最终成员显式全局/角色政策与审阅归档真实消费。复核中发现型号/固件字段可能先经toString规范化的问题，现已在Registry统一入口对六个canonical/alias静态文本键、两层输入核实际QString/512字符/UTF8往返，再进入正式解析与写回。空QString仍为未知，不作为认证。

本轮最终限定文本记录：

- 实际14源码/工程登记触点；另读8个关联头文件。20个C++头/源和1个纯数据控制器QML共21路径，经正则剥注释/字符串后三类括号左右字符计数相等，尾空白均0；pri尾空白0。只是字符清点，不是语法解析、编译或逻辑证明。
- Registry三个能力API声明/定义各1，原生上下文初始化6键/审批14键；helper h/cc在pri各1。静态文本前置校验定义/统一调用各1。Matcher/Role/Template/Continuity/Compiler的currentCapabilities调用分别2/2/2/4/1，仅辅助实际路径读回，不推导全部分支正确。
- Compiler最终成员及备用机门禁各1条循环；Guided稳定能力指纹函数和实际reviewEvidence归档字段各1。Coordinator实际捕获1、报告/结果插入2；能力证据不是新增飞行/账户许可。
- 18份文档617个本地Markdown文件目标均存在、文档尾空白0；只查文件存在，不核锚点、网页或内容真实性。git diff --check无输出；未跟踪源码另经实际读取/文本清点，不借tracked diff空输出代替检查。

独立限定源码交叉复核后，所审增量没有遗留已确定的新阻断；这不是穷尽审计、可构建/可运行或飞行安全证明。上述数字仅是本批范围，不能用类数/文件数/编号推导25目标完成率。

未调用解析器、编译器、qmake/moc、测试或任何业务/helper/Qt/QML执行；不执行OS认证/SQL/应用/设备。StartPermit、可信HOME/local/净空、正式远端实例/结果、长期授权/容量及其余五类均仍有源码缺口，`outboundIntegrationComplete=false` 保持。

### 381–385 限定文本核对范围

本轮触及12个源码/工程登记文件，另同步21份文档。对11个实际C++触点及5个关联头文件，仅作去注释/字符串后的括号字符计数和尾空白检查；这不是C++/QML解析、编译、测试或逻辑证明。16个文件的三种括号字符数量均成对相等；14个尾空白为0，Vehicle.h/cc分别9/13处，与只读HEAD的9/13一致，未整理用户原有无关空白。跟踪文件的git diff --check无输出；未跟踪文件另由上述文本检查覆盖，不能用git空输出替代它们的检查。

文本契约清点：原生snapshot初始化33键、EST journal叶8键；producer声明/定义/实际调用各1，原health complete定义/调用各1；新Estimator h/cc在pri各登记1；原生冻结gap getter声明/定义各1；已定位的Transaction/Transition/Run硬消费者没有旧32-key比较。原sixflight投影仍schema2，profile1–3及独立READ合同未机械升级。Transition真实逐叶advance和最终actualreceipt replay投影尾核各1条实际消费路径，不把存在函数名当作已验证运行结果。

21份文档的687个本地Markdown文件目标均存在、尾空白0；只核文件目标，不认证锚点、内容正确性或网页。读回审阅覆盖稳定EST11、实际ID/来源、两个时钟分别采样、firstactualqueue连续性起点、同值包时效、坏好ABA、健康optional位逐叶推进、旧阶段尾段、最终同源journal/gap重读，以及最后回调后的原生持久gap和deadline/owner检查。沿实际startup链的旧源getter也作必要QQ重核，不推导全仓回调安全。

未运行代码校验器、编译器、moc/qmake/AST/QML解析器、测试、SQL、业务/认证/helper、应用/UI、仿真、设备或刷机；未改PX4、提交/更新PR或生成包。以上只支持‘本轮源码与限定文本审阅已记录’，不支持完整25目标完成、StartPermit、净空、正式机载结果或真机放行。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

限定源码交叉审阅覆盖Vehicle原始来源/栈/时间、Swarm descriptor→Registry→HealthCenter，以及Run→同TaskRecord的结果投影；已确认的未知占位误判、缓存覆写和退休尾窗问题在所审范围修正。实际文本清点见本节后续记录，不把diff空输出当作未跟踪文件、解析、编译或运行证明。

本轮实际限定文本记录：

- 23个关联C++头/源路径经正则剥注释/字符串后三类括号左右计数相等，只是字符清点，不是语法解析。21路径尾空白0；Vehicle.h为9处、Vehicle.cc为13处，HEAD对应文件同为9/13，未为清零改动无关行；`git diff --check`无输出。未跟踪源码另行实际读取/文本清点，不能由tracked diff空推出覆盖。
- native健康getter声明/定义各1、实际RX prepare/complete调用各1、初始化调用1。两次Swarm native捕获为原捕获和末核；旧timer故障age=0模式、arming替代EST模式、FTC同名覆盖表及vehicleHealth的_mergeMissing调用均为0。这些数量只辅助所读源码合同，不证明实际Qt时序或传输行为。
- Run固定ReceiptRef成员11、引用角色11；target/group helper声明/定义各1，两种投影各调用同helper一次（各2处调用），同TaskRecord整组直投1处。没有用这些计数证明顺序、来源认证或任务完成；语义由限定逐段源码复读支持，仍未经运行。
- 19份文档656个本地Markdown链接目标存在、尾空白0；清单为7源码/19文档、26路径的本轮累计增量，不是独立安装包或验收进度百分比。

完整EST阶段逐包gap/最终write连续性、正式远端任务实例/结果、StartPermit/净空及长期生命周期仍缺，故完整目标保持未完成；未执行解析器、编译、测试、业务/helper、应用、OS认证、设备、刷机、PR更新或打包。

## 371–375：专用切换与双入口的源码审阅范围

状态：已完成限定源码及文本审阅，19份文档的648个本地链接目标存在、文档尾空白0；本节不借用366–370的旧数字，也不宣称编译、解析、运行或安全通过。本批实际8源码/19文档（27路径），实际路径见 [补丁清单](MERIVUS_SWARM_PATCH_MANIFEST.md)。

限定人工源码交叉审阅关注：两个 `ForController` 在同一 native call 中 begin → 私有栈 permit mutation → 新 scope/原 ID end；public end/legacy 不绕过活的 permit，旧 scope 只用于 mutation 前核验。新 current/selected 的实际提交事实保留在独立 receipt，不因 end 或 source-loss 尾窗失败而写成未发生。Guided 两个 fixed public API 使用原生 scope生成的独立 metadata token，12键 metadata/6键组行不输出 raw refs；JS transition ticket 覆盖通知及 finally，pure callbacks 不查其 busy。

指挥中心/规划页两处新面板为本地双步确认、默认关闭/取消、不以 Enter 提交、view ticket 防过期；这些是源码规则，不是视觉/焦点/模态实测。已返回 publication=true 先作本地 token 失效/身份意图协调，后置 getter 失败保留事实。prepare 不自动切 selected、搬登录/批准、停旧 Run 或释放占用；失败尝试也计8组容量，allocated=false 不证明未计费。原18键 switchEvidence/22键 scope/实际 quiet 和累计原生门禁保留。

本轮限定文本记录（由 Root 提供，范围包括累计关联源码，不等于8个实际改动路径）：

- 28个源码/工程路径：20个C++头源、5个QML及pri/qrc/qmldir，尾空白均为0；正则剥注释/字符串后三类括号左右计数相等。这只是字符计数，不是 parser、moc 或语法证明。
- Lifecycle 原8项加新2项 scope/call方法声明/定义各1；类型 include/注册及pri的h/cc条目各1。Guided实际factory为1、旧fixed14 ids为0、current调用32、observation调用4。
- 新Panel在qrc/qmldir各登记1次、两个宿主入口各1、两个product API声明各1；旧 `guard.epoch < epoch` 比较为0、`outboundIntegrationComplete=false` 保留1处。
- 既有12个cc的20处Direct调用、21分支/19方法的参数数量及首actual-self文本不匹配为0；限定 `git diff --check` 当前无输出。文本匹配不能证明 Qt 运行绑定或全部回调行为。
- 19份文档的648个本地Markdown链接目标存在、尾空白0。未跟踪源另行读取与计数，不以tracked diff空输出代表其已经受审；本轮8源码/19文档为累计工程增量，不是独立安装包。

`outboundIntegrationComplete=false`、完整25目标/六类缺口保持。本机没有执行 helper/业务/OS认证/应用、QML解析、编译/测试、SQL/日志/仿真/设备或刷机，未更新 PR/包。以下366–370及更早审计是历史记录，旧静态数字不转算为本轮覆盖，旧“未有入口/专用 API”状态由本节覆盖。

## 366–370：接线源码范围与限定文本审阅结果

状态：**已完成限定文本审阅**。本批仍是累计工程上的5源码/18文档增量，不是独立五文件包；交接须带完整源码/pri/注册及原生依赖。initial由构造提供，无自动prepare、legacy fallback或公开raw refs；原18键switchEvidence和22键引用scope不扩形。以下是文本及限定源码证据，不是解析、编译、运行或安全通过。

最终限定检查记录：

- 23个源码/工程路径（20个C++头/源、2个QML、1个pri）尾空白均为0；正则去注释/字符串后的三类括号左右计数成对。这只是字符计数，不是语法解析或编译证明。
- Lifecycle的8个scope/call方法声明、定义各1；Lifecycle/Episode的include、类型注册，以及pri各h/cc条目分别各1。
- 另对既有12个cc的20处Direct调用作参数文本复核：21个命名分支、19个不同QML方法的参数数量及首actual-self均对应，文本不匹配0；内部旧public Binding/Refresh伪调用0。此有限清点不覆盖所有回调的Qt实际绑定或运行行为。
- Guided实际factory为1，旧14个fixed-id为0；`runCurrent`为32（30业务+登录/退出2身份），`runObservation`为4。24个来源/上下文callback走原scope helper，private raw引用不表示所有public函数私有化。
- Panel在qrc/qmldir各登记1次，qrc的XML根为RCC；不代表QML加载、资源运行或实际方法绑定已验证。
- 18份文档的612个本地链接目标存在，文档尾空白均为0；限定`git diff --check`无输出。未跟踪源另行读取/文本核验，不能用diff无输出证明它们无问题。

多组限定源码交叉复核已完成（人工式逐段阅读，由助手完成，不执行程序）；在所审增量中没有留下已确认且未解决的新增静态问题。StartReview合法空对象、未知state拒绝、local setter plain lifetime及尾部stale原scope取消均已在此范围复核；这不是穷尽性审计、运行验收或飞行安全证明。

nested draft finally只撤原scope意图，无法核实时私有`retiredCurrentScope`持续拒同组current入口/回调，不超时复活，selected保持可读。cancel true仅表示请求被接受；各native自身仍按原drain规则退休/释放自己的占用，不直接/强制释放Run或Swarm占用，也不撤回已queue或把未知写成rollback。Qt exact Int/QObjectStar往返、实际方法绑定、Root parent、OS后Series/Run配对和模态销毁均未运行验证，不能放宽合同记作通过。

新部署/历史选择产品入口及专用transition原生API仍未实现，`outboundIntegrationComplete=false`；25目标/六类仍有实质源码缺口。未执行Qt/应用、OS认证/身份接口、编译、测试、解析/运行helper、SQL、仿真或设备操作，未改飞控/刷机/打包/更新PR。下方为历史审计，旧未接线状态不覆盖本批。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](../../custom/src/Swarm/FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

本轮限定审阅实际4原生源码及其关联20源码/工程路径、17文档。两组作者冻结后由另一组及独立只读代理跨核，根任务另读strict22、raw地址比较、独立代次、global token/acquire/lastdtor与mutation gate；限定新增范围未发现尚未解决的确定静态问题，不以未跟踪文件的空diff证明完成或运行正确。

- 限定20路径：实际Lifecycle/Session两组h/cc，加Episode/Series/Close/StartupTransaction/Deployment五组h/cc、RolePolicy/PlanStore/Health/Swarm/Orchestrator五个header及custom.pri。19个C++路径经正则剥注释/字符串后三类定界符左右计数相等，20路径尾空白0；不是C++/moc/AST/语法或运行检查。
- 新5个Q_INVOKABLE接口声明/定义各1；Session static提示getter声明/定义各1，两个auth各1个native栈guard。scope构造文本8个基键+14owned名称=22，15个QObjectStar只比较地址；独立outbound/observation递增文本各1，pure scope区内提示getter调用0，caller actualWorkflow直接成员解引用0。数量只辅助人工源码核验，不证明语义或Qt行为。
- Episode/Lifecycle/Session在pri的h/cc各1，未新增工程路径；新五scope方法在Guided调用0，CustomPlugin/Guided对Episode/Lifecycle类型提及仍0，qrc只核XML根RCC。明确未注册/迁移当前应用，不能由新增Q_INVOKABLE说新部署/历史观察已能操作。
- 17文档583处本地Markdown目标存在、尾空白0；限定git diff --check无输出。实际4源码均未跟踪，已逐文件读取/文本检查；没有提交、更新PR或生成包。独立文档复核确认22型/三个kind/纯查询/全局栈与at-read诊断边界和实际实现一致。

本scope不含shared5/账号/凭据/任务/Run/风险，不是授权。current.identityPromptBusy仅本次读取，token释放没有自动全局NOTIFY；任何切代/选择必须查实时native getter。原Session被毁/changed嵌套的真实Qt生命周期、QML精确类型往返、Windows工作站/凭据和跨对象调用均未运行验证。

下一阶段仍须Guided14实际对象引用迁移、各OS/native调用前后原scope/epoch及finally、共享Role明确身份意图、selected Run/Series/Close/Session整组投影与外层UI模态/入口注册。outboundIntegrationComplete=false、原25目标/六类/最终许可/净空/正式结果/跨站/运营继续；本机未编译、测试或执行任何helper/认证/API/应用/SQL/日志/仿真/设备/刷机，未改PX4。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](../../custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

本轮实际13源码（12cc+Guided）；12cc各只增现存QQ的首参数，QML同步19签名、已有7类SourcesMatch非空判断及内部helper分离。三组冻结后交叉配对原调用/新签名和仍保留的native前后门禁，未发现限定范围尚未解决的新增确定静态问题，不把参数文本一致或空diff当Qt运行证明。

- 限定源码/工程36路径：此前35路径（12类服务h/cc、Episode/Lifecycle/Session/Close/Run五组h/cc及custom.pri）加Guided；共34个C++、1个QML、1个pri。所有尾空白计数0；正则剥注释/字符串后的C++和QML三类定界符左右计数相等，非moc/QML/C++/AST/语法解析。
- 文本合同核对12个实际cc中20处Direct调用、Txn条件名称展开后21行/19个不同方法。各方法QML声明恰1，Q_ARG数量与声明参数数一致，首参文本为原self/guard QPointer的QObject包装。StartReview使用显式fromValue<QObject*>形式，其余static_cast形式均已按实际内容读取；这不是执行invokeMethod或证明Qt包装相等。
- Guided旧BindingAllowed和EvidenceRefreshAllowed内部直接调用均0；两个私有预检查helper各7处文本提及（定义1、原生包装1、原内部调用5）。GroundApproval调用ArmedReview明确传真peer，Close不引outbound-current。字词数量不替代native来源/授权检查或真实回调运行证明。
- 实际12类服务在pri的h/cc登记各1，未新增登记；CustomPlugin/Guided中的Episode/Lifecycle类型提及仍0，qrc XML根RCC。Close没有新增Episode outbound调用，旧Close观察权限边界保留。26文档601处本地Markdown目标存在、尾空白0；9份模块正文11行旧签名已同步，限定git diff --check无输出。
- 实际12cc全部未跟踪，已直接逐文件读取/文本检查，不能以tracked diff覆盖。QML是已跟踪累计改动，本轮没有把Git总行数当增量行数；12cc与Guided必须同批交接，旧少参数混版不能fallback为成功。

仍需源码开发：Guided14对象迁移实际current Episode，所有OS/原生调用前后Episode/epoch/实际服务冻结及finally只写原作用域，共享Role明确身份意图、全retained modal锁、selected旧Run/Series/Close/Session整组投影与人工新部署/历史入口。outboundIntegrationComplete=false、原25目标/六类与最终许可/净空/正式结果/跨站/运营范围均保留。

未编译、moc/语法解析、测试、执行helper/认证/API/应用/SQL/日志/仿真/设备/刷机，未改PX4、更新PR或打包；没有运行QML、打开软件或调用系统认证验证本结论。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](../../custom/src/Swarm/FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

本轮审阅范围：实际7源码及其关联源码/工程共22路径、16文档。三组作者冻结后交叉读取，根任务另读原生scope/ACK阶段、完整gate与pure分离、queue事实、signed字段和quiet用途；本批限定范围未发现尚未解决的新增确定静态问题，不是完整多部署应用隔离结论。

- 22路径为实际7源码，加Episode.h、Lifecycle/Session/Series/ObservationClose四组h/cc、MissionManager.h、LinkInterface.h、VehicleLinkManager.h、ParameterManager.h、FirmwarePlugin.h和custom.pri。其中21个C++路径经正则剥注释/字符串后三类定界符左右计数相等；这不是C++/moc/AST/语法解析。
- 尾空白既有5文件：PlanManager.cc=7、Vehicle.h=9、Vehicle.cc=13、MissionManager.h=2、FirmwarePlugin.h=2，各自与HEAD计数相同；其它17路径为0。未顺手重排旧源码。限定git diff --check无输出；实际7源码中的Deployment.h/.cc、Episode.cc三份仍未跟踪，已逐文件直接读取/文本检查，不能由tracked diff覆盖推断。
- 三个私有native API（PM write、PM read、Vehicle send）的声明/定义文本各1；Deployment/Episode/Lifecycle在pri的h/cc登记各1。Deployment旧普通write/load调用0；pure PM Context中的临时.lock文本0。旧Vehicle通用sender与独立RAW sender定义各1，但词频不是行为兼容性或授权正确性证明。
- CustomPlugin/Guided对新Episode/Lifecycle的提及仍0；qrc只核XML根RCC，没有QML或Qt资源构建。16文档560处本地Markdown目标存在、尾空白0；本段与后续剩余列表均已同步，不是把目录/文档存在当功能落地。
- 复核重点包括严格op0/N与真实operationStarted关联、每包/重试完整fence、adoption后QQ与pure、closed至旧栈退出、COUNT(0)与未知-1、实际末尾ACK queue、messagesSentChanged前记issued、原MM毁而Vehicle sender仍busy、signed原seq/checksum/signature固定。只读核过本仓MAVLink serializer实际使用checksum/signature、不重算/重签；没有运行serializer或增加密码认证结论。

仍需实际源码开发：Guided actual Episode/epoch与原receiver回调、共享Role明确身份意图、全部retained modal忙锁、selected旧Run/Series/Close/Session整组投影及人工新部署/历史入口；outboundIntegrationComplete=false保留。普通sender没有给adoption后全部item字段发原批准载荷再认证证书，也不证明RAW因果/机载原子快照/StartPermit/远端执行。原25目标/六类未整体完成。

限定文本/人工复读不证明Qt构造/销毁/重入、C++编译/链接、身份API、实际传输、兼容性或飞行行为；上述工程验证留给工程师。本机没有编译、测试、执行任何业务/helper/认证/应用/SQL/日志/仿真/设备/刷机，未改PX4、更新PR或打包。

## 346–350：十二类服务原生门禁与调用/发送边界（2026-10-15）

- 实际增量12个既有cc，无头/API、receipt或磁盘schema变更。限定读取/文本检查35个源码/工程路径：本12组h/cc、Episode/Lifecycle/Session/Close/Run五组h/cc和custom.pri；25份文档。12个实际cc均未跟踪，空git diff本身不覆盖它们，本轮已直接逐文件读取和文本检查。
- 四个sender/读取服务在request、live上下文、OS/Direct owner返回、原scoped READ/evidence fence及caller实际调用前核current Episode/owned peer同组；Deployment虚拟sendHome返回后核原Vehicle/插件QQ。四个审批/Authority服务在receipt/context/grant/消费/移交前后及same/transferred尾部同样核原实际组；不把Role/Store/Health/Swarm等共享源误传为owned pair。
- GroundApproval/GroundAuthority/Txn/Series的新请求、pure admission、授权实际供应者和向actual Txn的移交都核同组。Series managed bootstrap不要求尚未发布组current，而核实际Episode getter槽/parent/thread/private provenance/pristine/单次初值；child继承原来源且加入实际owner集合之后才调用current pair，legacy不得绑定managed Txn。consumerIsCurrent、previousStartup/current与历史保留。
- 两个既有非pure回调已收口：StartupAuthority认证移交不再调用full _gate，只比较preparing/busy/代次/QQ/精确grant/hash/未持proof或预约/原期限；Txn两种移交均只比对实际Authority UUID/constructor epoch/normal authorityGeneration或ground generation/hash/deployment/binding/原成员、准备中handoff及原期限。外层request、StartApproval/Authority移交两侧仍完整source/OS/风险gate；pure true不是身份检查、风险许可或StartPermit，OS耗时不续期。
- 已queue/uncertain保存先于返回后的新发送资格判定；_stop/_drain/_detachDrain、cancel、Close context/disposition/retire与被动Run不依当前outbound许可删除或回滚。Txn的主动授权_same含同组检查，但这些终态/清理方法仍直接核原generation/UUID/QQ，而不借_same吞旧事实。exact revoke不受current gate阻断。只删除ArmedGroundReview/GroundApproval/GroundAuthority共3处primary Link必须GUI线程误拒，其余实际线程/归属与Auto/Direct线程保护不放宽。
- 三组各自源码复读并循环交叉审其它4cc，根任务另外核pure回调/外层门禁、构造bootstrap、事实记账/终态清理和普通core发送路径；本批服务增量未发现尚未修复的新增确定静态问题。已知实质未完成项仍是Deployment legacy core每包/重试/插件后prequeue fence、完整控制器Epoch/来源/共享身份与旧观察整组投影，不能把前一结论说成全链隔离已通过。
- 限定文本结果：35路径尾空白0，其中34个C++路径剥注释/字符串后三类定界符左右计数相等；12个cc各1个Episode include，既有12组h/cc在pri各1，无新增类型/工程路径。CustomPlugin/Guided中的新Episode/Lifecycle提及仍0，qrc XML根RCC；12cc已核的primaryGUI约束文本为0，不能由词频证明所有线程行为正确。
- 25份文档558处本地Markdown链接目标无缺失、尾空白0；限定git diff --check无输出，未跟踪源码已另直接检查。定界符、登记、链接和源码复读不证明C++/moc/QML语法、链接、Qt构造/销毁/重入、Windows身份、实际传输或飞行行为。

普通Deployment仍调用writeMissionItems/loadFromVehicle，core的item adoption/progress/operationStarted/插件后与异步发送重试未持Episode scoped fence；本轮只加caller门禁，不修改Vehicle/PlanManager，也不把factory静默或UI隐藏当逐包保护。Lifecycle保持outboundIntegrationComplete=false、未注册QML/未迁移Guided/无新部署入口。

未编译、moc/语法解析、测试、运行任何业务/helper/API/认证/应用、SQL/日志/仿真/设备/刷机，未改PX4、更新PR或打包。原25目标/六类、最终许可/净空/远端正式结果和运营等完整目标继续，不是工程放行结论。

## 340–345：新部署原生核心与非 Close 身份门禁（2026-10-15）

核心审计对象为新Episode/Lifecycle四个h/cc、16个既有friend/provenance headers、`FleetMissionStartupSeries.cc`、`custom.pri`（源码/登记22路径）及17份文档；345另含Session.cc及Session说明，合并限定检查为23个源码/工程路径与18份文档。新/未跟踪文件已直接读取，不以tracked diff代表覆盖。设计和实际接线范围见[Lifecycle](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)。

逐源码交叉复读的契约：14个全新owned对象和signed-out Session；五个actual typed共享来源；全部retained纯native固定18键静默证据；候选自身fresh/QQ/父级/UUID唯一性及前后完整快照复核；8次分配含初始/失败尝试且不驱逐；旧实际对象、Run、未知状态与占用保留。不以public current getter、OS、空map或monitor关闭证明静默、落地或完成。

- 已修实际来源缺口：current/selected getter、isCurrent、Close查找与选择共用纯native record/parent/UUID/14provenance门禁；sourceRetained等历史指向另配actualSourceUsable。mandatory owned死亡sticky拒绝，Series失证据不复活，未知类型/owner丢失/reparent不降格为legacy。构造期已失owned不直接解引用或重建，候选后置拒；这不证明Qt构造/销毁运行安全。
- 外部原Vehicle/MM/PM/Links/primary缺失保持false/null及unknown，不用同ID替代。存活原对象仍独立检查真实native栈/协议/待写与实际归属；仅原Links/primary都存活才比较当前primary，变化只记未知，不清旧map/Run/Orch。9组件和最多9Txn各<=32目标，固定target4/source5/transport9；初始Txn不双列，所有物理/正式结果/指令释放claim仍false。
- Episode与Factory快照均限512KiB，Factory另限depth12/32768节点。按固定schema手工保守推算最多576个target行、单行<512B/19节点，完整bindings、32个reservation和固定元数据合计<416KiB/16384节点；此为源码预算推算，没有执行构造、序列化或helper测量，不是实测性能结论。超过预算拒绝而不截断历史/成员。
- Session实际16个目标方法（审批申请/验证、7个消费/移交及7个proofCurrent）加入current原生门禁或同实际owner配对；OS/owner回调后、预burn/移交及final核对，不续期或恢复旧proof。normal两次move也在connect后核QQ/pair/精确新proof。只有exact Close目的绕过outbound helper，原普通身份事实、登录/退出、Close即时消费、WTS、revoke/clear/expiry未整类封禁。双方真正legacy或同一managed actual组分开，跨组/工厂/混配拒绝。
- 限定文本检查：23个实际源码/工程路径全部读取，尾空白0；其中22个C++路径剥注释/字符串后三类定界符左右计数相等。14个owned头各有1个Episode forward/friend/managed/owner字段；Series child的两项原生继承各1。新四个h/cc在pri各1且目标存在，CustomPlugin/Guided中的新Episode/Lifecycle文本提及仍0，qrc XML根RCC。
- 18份文档527处本地Markdown链接目标无缺失、尾空白0；限定`git diff --check`无输出，未跟踪文件已另直接检查。两位负责人与独立只读复读已对最终来源/配对/Session增量核对，限定范围内无尚未解决的确定静态阻断；定界符、登记、链接及源码审阅不证明C++/moc/QML语法、链接、Qt重入、Windows身份或业务行为正确。

**未注册QML、未替换Guided、无新部署入口，现有应用仍单链。** Session只完成身份侧；其余12类owned新请求/审批/发送继续点、QML actual Episode/epoch/source、共享Role身份上下文及selected旧Run/Series/Close/Session同组投影尚未完整接线，`outboundIntegrationComplete=false`保持，不能称跨部署全链隔离通过。

未编译、moc、解析器、测试、运行构造器/工厂/API/helper/认证/应用、SQL/日志/仿真/设备/刷机，未更新 PR 或打包。原25目标/六类不缩减；本节不是工程验收、功能可用或飞行安全结论。

## 335–339：监视面板与来源/交互边界（2026-10-15）

- 实际6个源码/资源/模块路径及18份文档有增量，新QML/md直接读取，不以tracked diff替代未跟踪文件检查；C++后端与权限/磁盘格式未改。
- 两个入口复用实际controller，位于CommandCenter与Planner固定区域，不依赖草稿/上传/在线/grounded证据。Guided当前Run值不再fallback初始对象；UI还核Series evidenceLost=false、previousStartupId/epoch与实际Run对应，滚动观测计数不用于review token。
- 查看/申请/最终确认分离。每页冻结controller+reviewId/workflowEpoch/generation/context，取消精确原owner/token，不取消另一页review；三项默认false，新token/源换代/明确失效终态清空，authenticating/consuming的ok=false不触发取消。
- 原生modal内current/busy可能滞后，已删除Timer依快照解锁，callTicket锁存整个实际调用，只有返回且plain lifetime alive、当前ticket相同才清。原生回调期间面板销毁后，request放弃仅用局部closure与exact receipt，不访问dead QObject、不误清新调用。公开结果仍从原生current读取，不从JS返回猜成功。
- UI从最早request起按返回remaining再保守扣费，固定窗口不由通知/点击重启；墙钟回退清确认/禁批准，native单调期限仍为权威。固定context中最多32个成员的11字段风险全部显示，完整binding/epoch/hash/两代次可展开，不拿滚动报文冒充当前事实。默认取消、危险Enter拦截、长文本换行、语义颜色/可见焦点与错误摘要/滚动属于ui-ux-pro-max静态交互设计，未运行skill脚本/界面或测量视觉结果。
- released/known-held/unknown及monitor effect unknown分开，结果保留其review/Run标识；只有最终native真正效果有效，不宣称飞行停止、Completed、远端移交或成员可调度。
- 限定检查18个实际源码/资源路径（本批6、Close/Session/Txn/Series/Run h/cc及QGCPopupDialog/QGCButton），尾空白全部0；其中16个C++/QML路径剥注释/字符串后三类定界符左右计数相等。两个入口、qmldir类型及qrc别名各1，新资源目标存在、XML根RCC，旧initial Run fallback文本为0。
- 18份文档493个本地Markdown链接目标无缺失、尾空白0，限定git diff --check无输出，新未跟踪QML/md已直接读取。已修QObject/继承属性命名、deadthis放弃、modal忙锁、明确终态清确认等确定问题；这不是QML解析、moc/C++编译、渲染、系统modal/屏读、运行时或飞行验证。
- 未编译、测试、执行应用/认证/业务/helper/SQL/日志/仿真、改PX4、设备/刷机/放行、更新PR或打包。完整StartPermit/净空/正式机载结果、生命周期/授权容量及其它六类/原25目标未全部完成。

## 330–334：独立关闭监视源码审阅（2026-10-15）

- 本批实际14个源码/工程路径与19份文档有增量；新Close h/cc/md和Role md未跟踪文件均直接读取，不以空git diff代表已核查。
- 新Close仅从actual typed Series.previousStartup取得本次实际Txn，经private context18与sourceObjects3冻结原Run/Swarm/Orch QQ；完整binding7/9、有序native Int成员及原MODE-step身份不裁剪。旧Vehicle/MM失效保持unknown，不查同ID替代源；实际主owner及静默传输门禁不可旁路。
- 专用role action与系统approver purpose分开；Session实际主身份11、Role9、不同SID审批16、原30秒和三项确认进入整个review hash。OS/QML回调后重核同源，consume一次burn且不发同步changed、不存proof/lease；burn后只有passive/native核对及一次原生commit，失败不恢复。
- 已修确定问题：旧missionDeploymentOwns会因safety revoke返回false但实际map仍有占用；新private helper扫描该owner全部实际map项、要求原Run成员子集并排序，撤销与实际占用分别冻结。context18包含两份snapshot exact3，最终4bool来自实际map，失效/异常不能假空；范围变动使旧review失效，不把本地释放当成员可重新分配。
- Orch退休match与queue admission分开：失去旧source、无关assignment conflict或uint64观察revision耗尽不阻止结束原monitor；MAX仅实际closed能mandatory close，保留exhausted且不回绕。generic native记录门禁不放松，ObservationClosed不是Completed。
- commit前标记consumed/committing；QQ与局部实际返回事实保护同步销毁，缺post disposition保留release unknown；已知map仍有占用单独诊断，不自动补释放/重试或恢复授权。
- 限定检查19个实际存在的源码/工程路径（本批14及Run h/cc、Series h/cc、Role h），尾随空白全部0；其中18个C++/QML路径剥注释/字符串后三类定界符左右计数相等。最初检查清单误列只有md而没有同名h/cc的OperatorWorkflow，已删除错误检查项并对全部19个真实路径重查，不借失败命令的exit code报通过。
- 八组相关模块h/cc在pri各1，新Close MerivusInternal类型、内部实例、readonly值及request/approve/cancel函数各1，Run QML注册仍0。19份文档470个本地Markdown链接目标无缺失、尾空白0，限定git diff --check无输出；新未跟踪文件另已直接读取。交叉复读后actual occupancy、原输入合法数值类型与known-held诊断三项确定问题均修复，负责范围未见尚存确定静态问题。这不是编译、moc/QML解析、运行时或飞行证明。
- 未编译、测试、执行应用/认证/业务/helper/SQL/日志/仿真、改PX4、设备/刷机/放行、更新PR或打包。仍缺正式远端实例/结果与提交、最终StartPermit/净空/完整时序、授权容量/长任务/新部署及原25目标/其它六类完整功能，不能宣称全目标完成。

## 325–329 原生观察任务源码收口（2026-10-15）

- 实际增量10个源码路径；限定检查18个源码/工程路径与18份文档。新Orchestrator说明及未跟踪的Run/Txn/Planner等相关源码已直接读取，空git diff不代替未跟踪文件检查。未新增QObject/QML注册类型或工程源路径。
- actual Swarm仅一次接受typed同线程Orchestrator，Run在MODE前经actual owner登记同一模型的另一taskUUID；identity6与完整binding7/9、1–32个原生Qt Int有序成员冻结，128个native总量和512KiB以内固定投影有界。私有注册/更新/匹配不接QML来源map；typed-null代次、UUID碰撞覆盖及计数耗尽已明确拒绝，不回绕/驱逐旧历史。
- 所有generic生命周期/完成/进度/改派/删除入口对sticky native记录拒绝，owner销毁/closed也不变普通任务。Draft→Observing只记录实际MODE commit；ObservationClosed只结束监视，不触发通用taskCompleted/Failed/Cancelled/Started或成功比例。缺失leader/progress保持unknown。
- assignmentConflicts从prepared起保护全组，legacy busy直到startup认证退休且确有commit才计，首机commit后actual match允许其它机继续queue；Swarm旧lease及原原子移交不删除。独立observationRevision/通知不污染结构revision/普通taskUpdated黑匣子；原生事实同步保存，公开信号singleShot合并，逐emit检查QQ，不跨通知持TaskRecord引用。
- explicit native retirement、unused准备关闭、owner析构三事实分开。晚commit仅补历史不重开；销毁保留immutable身份和unknown tail，清所有sourceCurrent。已修实际接续问题：无MODE旧Draft不会在新地面链末段死锁；Txn先等发送/授权及native栈排空，Run严格核全部未issued/无uncertain后才关闭unused准备。未知或任何commit不自动清理，未退休旧Run提前阻断地面重审资格，不用字符串owner豁免。
- Planner新增合法观察phase且保守占用未closed成员，native自身没有generic建议/候选、缺Leader不造firstmember，坏标记拒绝；其它任务不会因合法新phase全失效。Memory观察总/关闭数独立，native=true伪completed也不计旧成功率，固定local-model标签scope/attribution=false，导入不恢复owner或认证。
- 已执行的仅文本检查：18路径存在、尾随空白全0；其中17个C++/QML路径剥注释/字符串后三类定界符左右计数相等，pri只核文本/登记。六组本批相关现有模块h/cc在pri各1；Orchestrator仍原MerivusInternal类型1，Run无QML注册，controller一次关联/独立readonly列表各1；qrc XML根RCC。
- 18文档尾空白0，445个本地Markdown链接目标无缺失，限定git diff --check无输出。独立源码交叉复读后，本批已报问题均修复并复读，未发现剩余确定阻断。计数/链接/形状不证明C++/moc/QML语法、链接、时序或飞行行为。
- 未编译、测试、运行应用/业务/helper/SQL/日志/仿真、调用认证/设备、改PX4、刷机、放行、更新PR或打包。正式远端任务实例/成功结果与授权提交、最终StartPermit/净空/完整时序、独立关闭审批、长期容量/清理及原25目标/六类仍继续，不把本地ObservationClosed称完整落地。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](../../custom/src/Swarm/SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

## 320–324 HOME 观察过渡源码收口（2026-10-15）

- 实际9个源码/工程路径有增量，限定18个源码/工程路径及19份文档；新未跟踪helper和本批未跟踪的既有模块已直接读取，不能用git diff覆盖范围冒充全覆盖。
- 精确schema1/2/3的6/8/9字段、原v1/v2输出与HOME stop-and-review不默认升级；v3八字段HOME包线经唯一纯值normalizer，真实已知float32/uint64 HOME/GP基线只阻断阶段候选，不造内部有效位。v3继承v2实际ODOM NED、参考metadata、reset/revision和周期gap。
- 原始HOME/GP只在capture开始初始化，全部实际ordered receipts重建共享计数/原参考；geo改变先pending、同RAW receipt/hash/时刻/相邻journal和stage的coords完成后计一次。coords-only要求geo不变；缺叶/夹叶/跨step半对拒绝。stage1 relative-only保持GP其余八字段，stage2继续旧motion/heading约束，两risk revision域分别推进，完整原生队列/final current精确核验保留。
- 来源为真实owner/PRIMARY/完整原计划和原生journal，不接受调用者成功map代替。原ACK/HB/地面/整组/账号/期限/任务依据不删；受审阅的观察范围不是HOME有效性、origin/同样本/因果、动态/净空或最终StartPermit。原风险算术诊断不变成许可。
- StartReview从本次冻结member取规范政策并进入完整artifact；审批/Authority/Txn保留full record身份，wirehash相同不复用旧政策批准。QML只读逐机显示范围/单位/原参考/跨步骤次数/严格零和未知，不折叠关键预算、不补默认/四舍五入、不加发送或自动已阅按钮。UI/UX规范仅影响文字分组、单位/未知态和全文换行；未运行视觉/键盘/辅助技术或布局验收。
- 已执行的仅文本检查：18路径存在；其中17个C++/QML路径剥注释/字符串后的三类定界符左右计数相等，pri只看登记/文本。18路径除原Vehicle.h9/cc13处尾空白外均0，这9/13与读取HEAD基线相同，未改无关行。HOME h/cc在pri各1，无QObject/QMLtype/可调用控制注册，qrc XML根RCC。
- 19文档尾空白0，442个本地Markdown链接目标无缺失；限定git diff --check无输出，未跟踪源码已另外逐个读取及检查。独立交叉源码复读未发现本批确定阻断，但括号/链接/hash形状一致不是C++/QML/moc/链接或行为正确证明。
- 未编译、测试、运行应用/API/helper/SQL/日志/仿真、调用系统认证/设备、改PX4、刷机、放行、更新PR或打包。原25目标/六类仍未全部闭环；HOME内部有效分支、最终许可/净空/完整时序与正式结果、新部署/容量及长期生命周期仍继续。

320–324（2026-10-15）新增[显式 HOME 观测过渡](../../custom/src/Swarm/FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

## 315–319 连续 Run 源码收口（2026-10-15）

- 实际增量13个源码/工程路径；限定检查20路径与18文档。新/未跟踪的 core observer 和 Run 文件已完整直接读取，不能用 `git diff --check` 代替其检查。
- 真实来源：独立Vehicle execution槽，不抢占telemetry；actual MODE-only write前私有begin，实际write调用返回后先原receipt snapshot、再captured commit、后public通知。Vehicle删除/嵌套退休仅用captured QQ/context交付仍存活observer，不反查已死对象；源/版本退休保留原epoch与未知尾部。
- 修复静态首稿发现：移除合法工作线程Link被错误要求GUI的限制；Link使用Auto通知，Direct闭包先线程guard。版本invalidation立即清槽而延后旧epoch虚通知，避免旧caller新增delete-this回调风险。provisional不回填post-commit、LAND必须空中后terminal再地面；lastObserved不当当前健康。
- MM operationStarted保留两tick间同值ABA。最后授权回调若使Run失效，core current/immutable还会重新通过actual owner getter核captured observer；ready支持有效provisional而begin另拒重复，最终write前拒绝坏依据。实际commit之后旧receipt查询不走该ready，因此不因已commit自拒。
- 启动发送认证/旧lane退休与Run监视分开；协议排空后对完整原组进行另UUID原子本地map移交，无release/reserve空窗。partial MODE与晚commit保留，已安全撤销/丢失不重建；旧析构只按旧UUID清理，不影响新owner。过去采用、实际释放调用和当前owns分别记录。显式结束监视不发送停机/降落，也不声称任务完成。
- 已执行的仅文本检查：17个C++/QML路径去注释/字符串后的三类定界符左右计数相等；20路径均存在，除原Vehicle.h9/cc13处尾随空白外均0，这9/13与HEAD读取基线一致，未清理用户旧代码。C++剥离方法不适用于qmake语法，qgroundcontrol.pro花括号28/26与HEAD相同，不拿该计数作语法结论；三个工程文件只核登记与文本。
- Run h/cc在custom.pri各1，新core header在qmake/CMake各1；Run无QML类型注册、Q_PROPERTY或Q_INVOKABLE，controller独立readonly属性1，qrc XML根RCC。18文档尾随空白0，421个本地Markdown链接目标无缺失。限定 `git diff --check` 无输出；此命令不覆盖未跟踪文件，已另直接检查。
- 已完成三方独立限定源码复读，未发现本批确定阻断问题。不证明C++/moc/QML/链接、真实通信时序、性能或飞行可用。未编译、测试、运行API/helper/SQL/日志/应用、仿真、操作设备、修改PX4、刷机、放行、更新PR或打包。
- 原目标未完成：最终StartPermit/净空/完整时序、可信远端mission instance/正式结果及授权提交、合法HOME有效分支、新部署/容量与全生命周期，以及原25目标/六类其它工作继续；不能以本地Run或本次静态收口替代。

315–319（2026-10-15）新增[本次启动的连续运行观测](../../custom/src/Swarm/FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

## 310–314 限定源码审计（2026-10-15）

- 范围：19个源码/工程路径、21份文档；其中10个源码/工程路径有本轮增量。新/未跟踪文件已直接读取，不把 `git diff --check` 当作覆盖它们的证据。
- Native inventory：成功必须明确正常 EOF、全部成功路径 close、未取消且目录对象 before/after/reopen 检查通过；失败 entries/identity 清空、ok/completed 保持 false。固定 namespace、全部非 dot 项4096和入选项最多128、可逆 Unicode、普通文件/无链接分类由两组消费者二次校验。没有 payload/SQL/写入/删除/控制接口。
- 接线：四个消费者不再使用 QDirIterator；两个 Reader catalog 前后私有目录观察身份与文件列表一致，两个根门面要求非 null 真 Qt Bool true 才接纳枚举完成。完成枚举不是全集认证或原子快照，ReFS 64-bit index 等对象观察限制仍在模块文档。
- 磁盘契约：Telemetry Header19/Frame29/Footer31、entry8/request11，事件 SQL schema/entry7 未改；Codec Worker 固定错误23项且23个唯一值。所有原认证/完整覆盖/权限/执行禁止旗标保留。
- 已执行的仅文本检查：19路径均存在、尾随空白0，去注释/字符串后的括号/花括号/方括号左右计数相等；21文档尾随空白0、417个本地 Markdown 链接目标无缺失；8组模块 h/cc 在 custom.pri 各一次，qrc XML根为RCC。限定 `git diff --check` 无输出。计数平衡不证明 C++/QML 语法、链接或 ABI 正确。
- 已完成 helper/两组消费者及根门面的独立源码交叉复读，未发现本轮确定阻断问题。这不是编译、运行或测试通过；未调用文件枚举/hash/SQL/新 API/helper、系统认证、仿真、设备、刷机、放行、更新 PR 或打包。
- 未完成的原范围：完整 P0 StartPermit/执行进度与完成归属、可信能力、自愈实际执行、跨站权限同步、完整遥测/索引留存/恢复、动力学/插件和跨平台执行仍继续，不能以本子模块收口缩减原25目标。

310–314（2026-10-15）新增[错误可区分的原生目录清点](../../custom/src/Swarm/FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

## 305–309 遥测历史源码与限定文本收口（2026-10-14）

实际业务/工程增量8路径：Codec2/Archive2/Reader2及pri/controller值接线；限定检查16源码/工程路径、18文档，另含原writer/core来源与登记契约。三方独立限定复读核对Header19/Frame29/Footer31、readPolicy4、request11/descriptor8/publicrow6、实际扫描帧数/字节/末序号、run/source/定位与序号、完整行前缀/partialTail/两次hash、native实际catalog/cursor及QQ/nonce双终结寿命。已修源码首稿的catalog format、crossFileAtomicSnapshot旗标、queryId规则、QString非NULL空值和header实际handle-size错位；不以接口类存在代替可运行证明。

有限文本检查实际391个本地Markdown链接无缺失，34个检查路径无缺失。新增/其它检查文件尾空白0；Vehicle.h/.cc历史9/13与前批和git HEAD同数，未改无关行。限定git diff --check无输出；untracked新增文件另逐个读取，空diff不冒充覆盖。4个Telemetry模块h/cc在custom.pri各一次，Archive type/objectName/public facade各一次，Reader没有QMLtype；core头qmake/CMake各一次、qrc根RCC。以下仅是剔除注释/字符串后的词法分隔符计数，不是C++/QML语法、编译或运行证明：

| 源码 | 花括号左右 | 圆括号左右 | 方括号左右 |
| --- | --- | --- | --- |
| FleetTelemetryArchiveCodec.h | 3/3 | 5/5 | 0/0 |
| FleetTelemetryArchiveCodec.cc | 89/89 | 683/683 | 5/5 |
| FleetTelemetryArchive.h | 2/2 | 28/28 | 0/0 |
| FleetTelemetryArchive.cc | 174/174 | 1078/1078 | 10/10 |
| FleetTelemetryArchiveHistoryReader.h | 2/2 | 7/7 | 0/0 |
| FleetTelemetryArchiveHistoryReader.cc | 109/109 | 770/770 | 1/1 |

Reader只读existing文件，bad/empty候选仍固定诊断；逐整file验证与重开hash不认证全集/恶意TOCTOU/硬件身份或永久closure。无Footer/partialTail只保留完整已验证prefix，完整坏行或Footer镜像错拒整页，v1磁盘没有lastFlushSeq/末端flush ack。请求capture互斥，结果与actualfinished双到才复用槽，取消未交付结果不推进cursor；结果已交付cancel返回false，已发布历史不回滚。Actualcatalog/nativeNonce/perfile唯一filter+位置+实际next/hash冻结，不让queryID重用/猜想游标恢复来源。

公开页按固定允许字段重建，不复制payload/frame时间/checksum/epoch/nativeSeq/signature/任意Footer文本；native detail只有最新实际成功page+同GUI+新确认且槽释放后，接纳新查询先清旧raw，失败/取消不拿旧page当成功。所有身份/CRC/closure/fullTelemetry/权限/执行认证false，不回灌Registry/Replay/规则/任务/审批/Authority或命令。

只有源码读取、限定文本/登记/路径与同行审阅，未调用读文件/目录/hash/Codec/Reader/业务helper/SQL/锁、语法解析器、编译器/moc/qmake、应用/UI、测试/仿真/设备/刷机；未改PX4/更新PR/包。平台error-aware全集索引、真实覆盖/更全批准数据、长期归档统计维护/灾后恢复及其它五类代码目标仍待开发，完整25目标不由309记录号推算完成率。

305–309 新增[遥测只读历史检索](../../custom/src/Swarm/FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

## 300–304 原生RX观察源码与限定文本收口（2026-10-14）

本批14个源码/工程路径、17文档：core Observer/Vehicle桥、Codec/Worker/Archive三模块及qmake/CMake/pri/type/controller非视觉登记。限定同行复读实际工具箱协议/MVM/VLM与PRIMARY来源、pre-plugin原len复制、source epoch/不回绕计数、clear-before-retire与派生析构解绑、Root实际worker直到finished寿命，以及Codec29字段/27白名单和Worker冻结scope/同意。关闭了真实UDP错误GUI-affinity拒绝、close/prepareDelete回调重入、typed-null target、并发目录slot创建和已写batch仍留payload却归还容量等源码边界。

文本检查实际369个本地Markdown链接无缺失，31个检查路径无缺失。新增及其它本批检查文件尾空白0；完整Vehicle.h/.cc历史尾空白分别9/13，与git HEAD同数，保留未改，本批新增行没有尾空白。限定git diff --check无输出，未跟踪新文件逐个另读，不以空diff冒充覆盖。三模块h/cc在custom.pri各1；Archive QMLtype/objectName/public值facade各1、Codec/Worker类型登记0，core头qmake/CMake各1、qrc根RCC。以下为剔除注释/字符串后的有限词法计数，不是语法/编译或运行证明：

| 源码 | 花括号左右 | 圆括号左右 | 方括号左右 |
| --- | --- | --- | --- |
| VehicleTelemetryObserver.h | 2/2 | 5/5 | 0/0 |
| Vehicle.h | 167/167 | 759/759 | 1/1 |
| Vehicle.cc | 1355/1355 | 5666/5666 | 197/197 |
| FleetTelemetryArchiveCodec.h | 2/2 | 2/2 | 0/0 |
| FleetTelemetryArchiveCodec.cc | 49/49 | 376/376 | 4/4 |
| FleetTelemetryArchiveWorker.h | 1/1 | 15/15 | 0/0 |
| FleetTelemetryArchiveWorker.cc | 102/102 | 745/745 | 3/3 |
| FleetTelemetryArchive.h | 2/2 | 15/15 | 0/0 |
| FleetTelemetryArchive.cc | 78/78 | 368/368 | 6/6 |

scope header明确实际选中机/源，即使没有approved frame也不隐去该成员；强类型10字段来源摘要独立于writer计数，最后采样可非原子。queue+in-flight/字节、扫描/段数/文件上限和footer reserve是有限源码约束，不宣称全进程RSS硬cap、全目录错误EOF认证或恶意外部writer强quota。目录协作锁不堵GUI，不强删旧段；新记录器不替代旧.tlog隐私控制。原parser签名/CRC静态风险未暗改，crcVerified/authenticated/fullTelemetryCaptured/许可均false。

盘内clean=false/关闭candidate不当末端flush确认，runtime clean只在实际满footer/flush/noerror/drain才真；fsync/durable始终false，源丢失尾部unknown，不自动追新机或续run。新记录不进入BlackBox/Replay规则/Registry/审批/任务/StartPermit或命令，旧日志和SQL归档未改行为。

仅源码读取、有限文本/路径/登记和官方Qt5文档核对；未调用Codec/worker/helper/锁/文件枚举/创建/write/flush/SQL、语法解析器、编译器/moc/qmake、应用/UI、测试/仿真/设备或刷机。未改PX4/PR/包。完整遥测覆盖、独立journal历史检索/全集索引、长期归档/维护/灾后恢复和其它五类仍待开发；全25目标继续，不由304修改记录推算完成率。

300–304 新增[原生入站遥测记录器](../../custom/src/Swarm/FleetTelemetryArchive.md)：独立Vehicle pre-plugin被动入口、实际PRIMARY/Vehicle/observer绑定与退休、27消息固定Codec、parentless有界NDJSON worker及native确认/公开只读值已接线。新增默认关闭，不改旧.tlog，不请求数据流或控制；定位类另明确确认，原len payload与RX序号只观察，签名/CRC/身份不认证。文件NewOnly不覆盖或删除，入队/write/flush/durable分层，源失效停止整组并保留unknown尾部；fullTelemetryCaptured始终false，不把此子集当完整遥测或任务许可。运行/编译/测试/解析helper/设备均未执行，未改PX4/刷机/更新PR/打包；原25目标/六类不缩减。下方295–299及更早为历史批次。

## 295–299 限定跨封存浏览源码与文本收口（2026-10-14）

实际业务增量为Worker/Store/新Sql/新Reader八个h/cc，外加pri/controller非视觉接线；五个模块检查范围合计15源码/工程文件、19文档。独立同行限定审阅复核共享schema/类型/实际epoch/EOF/事务和Reader只读/前后hash/目录容量、Store actualcatalog与分段filter游标/全history槽/取消及Qt线程寿命。关闭合法1MiB预算被内层最小值误拒绝、current discard提前复用查询槽、Writer结束误停Reader状态刷新等边界；按每basename只一个filter上下文限制最多128，不让caller任意UUID累积无界缓存。最后已读限定范围未发现新确定阻断，不证明SQL或运行正确。

有限文本核对：362个本地Markdown链接无缺失，全部检查文件尾随空白0；五模块h/cc在custom.pri各一次，Store QMLtype/objectName/actualBlackBox绑定/public值facade各一次，qrc根RCC。新Sql/Reader没有QMLtype登记，不迁库、不加资源/视觉界面。限定git diff --check无输出；新未跟踪文件均单独读取，不用该命令空输出冒充覆盖。注释/字符串剔除后的本批源码分隔符计数：

| 源码 | 花括号左右 | 圆括号左右 | 方括号左右 |
| --- | --- | --- | --- |
| FleetEventArchiveWorker.h | 2/2 | 15/15 | 0/0 |
| FleetEventArchiveWorker.cc | 157/157 | 1074/1074 | 1/1 |
| FleetPersistentEventStore.h | 1/1 | 28/28 | 0/0 |
| FleetPersistentEventStore.cc | 123/123 | 855/855 | 12/12 |
| FleetEventArchiveSql.h | 1/1 | 11/11 | 0/0 |
| FleetEventArchiveSql.cc | 91/91 | 524/524 | 0/0 |
| FleetEventArchiveHistoryReader.h | 2/2 | 9/9 | 0/0 |
| FleetEventArchiveHistoryReader.cc | 106/106 | 613/613 | 2/2 |

SQL inspect精确四字段/唯一定义、Reader READONLY/current文件排除/命名sealRun clean及旧unclean保留、元数据预留与whole页、有限一次结果、actualQQ/query/kind和保槽/前后scope由源码接线证明。Catalog全集/所有记录hash/跨段原子视图/飞行时间排序仍未证明：catalogCompletenessVerified/allRecordHashesVerified/crossSegmentAtomicSnapshot/globalRowIdAvailable=false，不把空列表或 basename序当完整历史证据。hash与catalogId不是认证或执行许可，历史不回灌live rules/Replay/任务/审批/authority。

只有源码/接口/有限文本/路径/登记核对，没有调用SQL、目录/文件hash/业务helper、Qt线程/应用、编译器/语法解析器/moc/qmake、测试、仿真、设备或刷机。没有修改PX4、提交PR或打包。完整25目标/六类继续：平台error-aware全集索引/自动留存归档、持续完整遥测/跨任务运营维护、签名/企业权限/灾后恢复，以及最终StartPermit/净空/完整时序/执行完成仍为代码缺口。

295–299 新增[跨封存段只读浏览](../../custom/src/Swarm/FleetPersistentEventStore.md)：Sql唯一schema/身份/严格页抽取、parentless单请求Reader与Store native目录/分页/取消及public值facade已接。actual目录条目生成本catalogId，页域绑定文件/DBepoch/sealRun/filter，最多128个每段单filter游标，只0或实际next；当前页/Reader共单槽，取消与错误不提前复用，历史不回灌rules/Replay/任务/权限。目录128段/4096条目与JSON页有界，合法1MiB政策的内部预留预算已修；目录全集、全库记录hash、跨段原子快照/飞行序都不宣称已验证。仍缺全集索引/自动归档清理、完整遥测/运营集成及最终StartPermit等原六类/25目标；未编译/测试、运行SQL/目录或hash helper、改飞控/刷机、更新PR或打包。下方290–294及更早说明为历史批次。

## 290–294 限定维护轮换源码与文本收口（2026-10-14）

本批实际业务源码增量为Worker/Store四个h/cc，另同步两模块契约和全局交接。独立同行复核关闭了旧SQL连接身份与待封存路径混淆、尚未递送到消费者的发布槽被抢先轮换、旧源queued destroyed清新源、取消抹封存物理事实、窗口known混上轮数以及封存/续建失败诊断相互覆盖的源码缺口。末端实际currentPath只读probe、前后hash与SQL句柄释放、native明确维护请求/同实例与epoch、实际clean/sealed/finished及精确basename门禁已接；最后限定已读范围未发现新的确定阻断问题，不证明运行正确。

identity的sqlite_master type/name/table/SQL及meta字段也在SQLite CASE中限制实际类型/长度，自动index允许的NULL另核有效flag，避免将异常大TEXT先materialize到Qt再比schema；初始库、末端原连接屏障和currentPath新probe共用此边界。源码字符串/控制流审阅不证明SQLite引擎内存、文件并发或实际driver行为，需工程师验证。

复核范围沿用11源码/工程文件与17文档：337个本地Markdown链接无缺失，各检查文件尾随空白0；三模块h/cc在pri各一次、Store类型/objectName/实际source绑定/public值facade各一次，qrc根RCC不变。限定git diff --check无输出，新未跟踪的Worker/Store及其它已纳范围文件均独立读取核查，不以git diff空输出代替覆盖。字符串/注释剔除后的本批源码分隔符数：

| 源码 | 花括号左右 | 圆括号左右 | 方括号左右 |
| --- | --- | --- | --- |
| FleetEventArchiveWorker.h | 2/2 | 15/15 | 0/0 |
| FleetEventArchiveWorker.cc | 226/226 | 1477/1477 | 1/1 |
| FleetPersistentEventStore.h | 1/1 | 21/21 | 0/0 |
| FleetPersistentEventStore.cc | 88/88 | 580/580 | 9/9 |

仅源码同行及有限文本/路径/登记核对，未调用SQL、QLockFile、文件hash/rename或业务helper，没有编译器/moc/qmake/语法解析、测试、运行应用、仿真、硬件或刷机。没有提交PR、打包或修改PX4。QLockFile属于同约定产品协作锁，Qt可按进程信息处理stale锁；不是ACL/恶意writer安全。hash只比较内容，rename及其后验证失败仍可能移动文件，sealMayHaveMoved/独立sealError和resumeError/候选路径保留，不自动删除/逆向恢复。

旧库封存和新run是显式维护流程，不是无缝完整遥测或权限恢复；暂停窗口仅已观察序号差且known/unknown明确，clean只表示存储关闭。封存段数有界不代表总磁盘绝对配额，无自动阈值轮换/跨段历史查询/永久留存清理或离线恢复；完整遥测/运营集成以及最终StartPermit/净空/时序/执行完成等原六类/25目标继续。

290–294 新增[显式维护封存轮换](../../custom/src/Swarm/FleetPersistentEventStore.md)：实际worker协作锁贯穿排空/SQL关闭与同目录旧库保留，native确认后才请求，不自动删旧段；actual clean/sealed/finished、精确DBepoch/文件名和同源QQ/epoch核对后才沿本次明确配置建立新run。取消/退出/源丢失只退休续建，不抹掉可能已发生的封存事实；暂停序号窗口显式known/unknown及缺口摘要，不称连续无损。封存段数有界、未知文件不覆盖，hash/协作锁不认证或保证恶意并发/失电安全。仍缺跨段查询/自动归档清理、全遥测/运营集成及最终StartPermit等原六类/25完整目标；未编译/测试/运行库或锁/文件轮换、执行SQL、改飞控/刷机、更新PR或打包。下方285–289及更早记录为历史批次。

## 285–289 限定源码与文本收口（2026-10-14）

实际同行只读范围覆盖Codec固定18/4/6–21投影、Worker建库身份/队列/提交/读页/停止、BlackBox真实在栈收据和Store生命周期/接线。关闭了取行错误被当EOF、SQLite坏类型默认转换、stop-final竞态、driver前坏BLOB/文本资源、typed-null补已知0/false、同类型receipt与NaN身份/投影混淆、新run清页通知等源码缺口；把诊断缺口重叠、hash非匿名化、hot-journal与长期runs轮换不足明确入文档。最后已读限定范围未发现新的确定阻断问题，不证明编译/SQL/运行正确。

有限文本检查为11源码/工程文件、17文档：所有检查文件尾随空白0；324个本地Markdown链接无缺失；Codec/Worker/Store的h/cc在custom.pri各一次，Store内部QML类型、私有objectName、实际BlackBox绑定和public值facade各一次；现有qrc根为RCC，无新增资源。限定git diff --check无输出；新未跟踪文件已显式读取检查，不能靠git diff覆盖。只在剔除字符串/注释后计分隔符，不运行源码/语法解析器：

| 源码 | 花括号左右 | 圆括号左右 | 方括号左右 |
| --- | --- | --- | --- |
| FleetEventBlackBox.h | 5/5 | 34/34 | 0/0 |
| FleetEventBlackBox.cc | 74/74 | 443/443 | 5/5 |
| FleetEventArchiveCodec.h | 2/2 | 2/2 | 0/0 |
| FleetEventArchiveCodec.cc | 102/102 | 555/555 | 0/0 |
| FleetEventArchiveWorker.h | 2/2 | 14/14 | 0/0 |
| FleetEventArchiveWorker.cc | 179/179 | 1107/1107 | 0/0 |
| FleetPersistentEventStore.h | 1/1 | 20/20 | 0/0 |
| FleetPersistentEventStore.cc | 54/54 | 389/389 | 9/9 |
| CustomPlugin.cc | 83/83 | 388/388 | 257/257 |

默认off和native consent、captureStart窗口、encode前nonwrap序号、250ms合并通知、parentless worker及finished后deleteLater、单槽历史直到GUI交付均由源码接线证明，非运行证据。没有调用业务API/helper、编译器/moc/qmake、测试/回放/仿真/应用、SQL或数据库操作，没有修改飞控、刷机、提交PR或打包。

本批只有诊断摘要连续存储，不保存原payload/精确GPS/凭据，不能认证数据源、匿名化、恢复权限或补齐完整遥测。clean只是存储有序关闭，pending/drop摘要仍可能在崩溃前未提交；历史不进入实时rules/Replay/任务/审批/authority/Vehicle。完整25目标/六类继续：长期runs/数据库轮换、完整遥测/统计维护/灾后恢复，以及最终StartPermit/净空/完整时序和执行完成仍为代码缺口。

285–289 新增[默认关闭的连续诊断归档](../../custom/src/Swarm/FleetPersistentEventStore.md)：实际BlackBox暂时收据/同类型事件身份、固定摘要Codec、专属QtSql线程与Store门面已接pri/type/controller；仅native显式配置/确认后异步开库，默认不读取或写盘，不回录开库前历史。queue+in-flight/批次/主库页/历史结果有界，实际commit后才durable，来源/投影/入队缺口保留诊断；历史单槽直到GUI交付，不进入实时回放、规则、机队、审批或控制链。无原payload/凭据/精确GPS，哈希非签名/匿名化；clean非完整性认证，未知库与需hot-journal恢复库拒绝不重建。仍缺全遥测/长期轮换/运营集成及最终StartPermit等原六类/25完整目标；未编译/运行/测试、建库/SQL执行、改飞控、刷机、更新PR或打包。下方280–284及更早批次为历史记录。

## 280–284 限定源码与文本收口（2026-10-14）

独立审阅输入schema、观察数学、实际Txn门禁、Readiness/READ冻结、Transition完整历史重构与Vehicle ODOM周期gap。关闭原真实工厂统一size6拒绝新政策的集成缺口，并将文档旧XYZ/heading恒等语义限定为schema1；最后已读范围无新的确定阻塞。没有运行解析器、编译、业务helper/API、测试、仿真或硬件操作，不能以此证明运行正确、origin/净空或最终许可。

实际有限检查范围为12源码/工程文件和19文档：322个本地Markdown链接无缺失；motion helper h/cc在custom.pri各一次，前批ODOM helper在qmake/CMake各一次保持，现有qrc根RCC。19文档和所有新/本批custom源码无尾空白，Vehicle.h/cc仍保留9/13行HEAD原有尾空白，不改无关格式。限定git diff --check无输出；新增未跟踪文件独立做文本检查，不依赖该命令覆盖它们。只做字符串/注释剔除后的分隔符计数，不是语法/链接或业务测试：

| 源码 | 花括号左右 | 圆括号左右 | 方括号左右 |
| --- | --- | --- | --- |
| FleetMissionExecutionProfile.h | 1/1 | 1/1 | 0/0 |
| FleetMissionExecutionProfile.cc | 91/91 | 417/417 | 0/0 |
| FleetMissionStartupMotion.h | 1/1 | 2/2 | 0/0 |
| FleetMissionStartupMotion.cc | 21/21 | 170/170 | 0/0 |
| FleetMissionStartupTransaction.h | 6/6 | 44/44 | 3/3 |
| FleetMissionStartupTransaction.cc | 276/276 | 2310/2310 | 95/95 |
| FleetMissionStartReadiness.cc | 245/245 | 1926/1926 | 6/6 |
| FleetMissionStartupTransition.cc | 93/93 | 1353/1353 | 4/4 |
| FleetMissionArmedGroundReview.cc | 253/253 | 1811/1811 | 29/29 |
| Vehicle.h | 167/167 | 753/753 | 1/1 |
| Vehicle.cc | 1331/1331 | 5475/5475 | 188/188 |

检查确认schema1/6与schema2/8、local7/heading3和actual native reference21→stable20对应，两个revision域/queue/final精确投影未删除；ODOM gap覆盖初始化/同值包/旧closed-step tail，旧step32/read34/coordinate10/13/flight6保持形状。LOCAL_POSITION_NED和ODOMETRY来自不同uORB流，只新增独立观察包线而无同origin/sample认证。累计重建不每步清零，同heading不算观察转动，0.02度编码外包可能使0.01度合法政策不匹配；既不是yaw-rate/真实总转角也不排除未观察旋转。

原HOME有效分支、受信local origin/内部参考有效性、全动态/净空、最终StartPermit/完整时序/执行完成和其它原六类/25目标继续。源码登记、路径和括号计数不能代替产品完成证明，旧PR/源码包/安装包没有更新。

280–284 新增[显式局部运动与航向观察政策](../../custom/src/Swarm/FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

## 275–279 限定静态收口（2026-10-14）

只读交叉审阅本地MAVLink header/XML/helpers、配套PX4 ODOMETRY stream及本批地面站接线。发现并关闭“失效clear row被当从未提供optional流”的具体缺口：actual source通过后decode前的sticky everObservedRawFrame保留；失效/过滤/源丢失不能凭空map得到新候选。最后已读范围未发现新的确定阻塞，不等于运行正确或参考有效性被证明。

实际执行的检查仅为文本读取、注释/字符串剔除后的分隔符计数、行尾空白、Markdown路径、工程条目计数、qrc XML根名与git diff --check；没有解析C++/QML、调用业务helper、系统/协议/设备API、编译或测试。限定10源码/工程文件及17文档：286个本地Markdown链接无缺失；17文档和两新helper文件无行尾空白；qmake与Vehicle CMake的helper h/cc各一次，现有qrc根为RCC。git diff --check无输出，新增未跟踪文件另做文本检查，不能依靠git该命令覆盖它们。

保留原历史空白：Vehicle.h 9行、Vehicle.cc 13行、PlanManager.cc 7行，与HEAD的全文统计分别一致，未为本批修改无关格式。这些不是新增diff空白，也不将全文说成零空白。下列数字只是词法计数，不是语法/链接/运行证明：

| 源码 | 花括号左右 | 圆括号左右 | 方括号左右 |
| --- | --- | --- | --- |
| FleetStartupOdometry.h | 1/1 | 3/3 | 0/0 |
| FleetStartupOdometry.cc | 42/42 | 348/348 | 12/12 |
| Vehicle.h | 167/167 | 753/753 | 1/1 |
| Vehicle.cc | 1325/1325 | 5390/5390 | 182/182 |
| PlanManager.cc | 314/314 | 1853/1853 | 13/13 |
| FleetMissionStartReadiness.cc | 245/245 | 1879/1879 | 6/6 |
| FleetMissionStartupTransition.cc | 91/91 | 1284/1284 | 4/4 |
| FleetMissionArmedGroundReview.cc | 253/253 | 1809/1809 | 29/29 |

确认helper50/reference42、Vehicle27/reference21、Readiness去age20及Transition/READ稳定身份对应；普通quality metric/time/运动值不进入参考身份，source/reset/frame/明确状态/已观测ABA仍保留。旧schema与HOME/local严格政策未放宽，没有ODOM参考origin/同样本/内部有效分支认证。最终StartPermit/净空/完整时序/执行完成、其它原六类/25目标未完成，不能因这组文本检查标记产品完成或直接打开执行按钮。旧PR、压缩包、安装包没有更新。

275–279 新增[被动 ODOMETRY 原生账本与契约](../../src/Vehicle/FleetStartupOdometry.md)：原始 MAV2 解码50/reference42，Vehicle full27/reference21；实际 PRIMARY/PX4/版本与飞行来源、接收整栈、prepare前计数和token/epoch防嵌套覆盖。sticky everObservedRawFrame 不把曾断流/无效/被过滤的源当初始缺流；Readiness仅存去age的20字段参考投影，普通收包/时间/运动数字/quality分值不造成身份抖动，reset/frame/状态与已观察ABA撤销旧依据；地面READ冻结该投影，Transition前后重核实际来源。稀疏协方差NaN保留未知，扩展尾零/LOCAL_FRD/时钟不认证参考有效性或同采样。不增加传感器、不请求/启用流、不改原六通道/read34/startup32、不放宽HOME/local严格政策。源码接线与有限静态审阅，不是完整运动/转向/净空/最终StartPermit或执行完成；原25目标/六类继续，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方270–274及更早批次是历史记录。

270–274限定只读源码串审：Series实际对象保留/容量/空候选/new basis/首changed前初始化、Terminal固定摘要6/sourceContextHash及native exact20引用、GroundAuthority实际previous getter、controller源冻结/旧链退休/新consumer及首/终态不重用、pri/type。修复空候选过期后永久卡死和错误首/终态factory先覆写的问题；第2/3轮源码契约及用途分离未报告新确定阻断。没有跑控制器、读取业务/系统API、编译、解析C++/QML语法、测试或硬件。容量生命周期/HOME/local frame/运动转向、最终许可/净空/时序/完成与原六类仍待开发，有限审阅不是完整验收。

9个本批C++/QML/pri文件去字符串/注释后的文本分隔符数量匹配、尾空白0；git diff --check无输出，untracked新源码另做逐文件检查，不把空diff当验证。16个Markdown的258条本地链接存在、尾空白0；Series新头/源的pri各一次且文件存在、内部type一次，qrc XML根RCC。这是文本/路径/注册元数据，不是parser/moc/qmake/编译或测试。

270–274 新增[有界实际启动事务序列](../../custom/src/Swarm/FleetMissionStartupSeries.md)及多轮地面再审后端：最新已初始化owner不回退，逐轮冻结actual QQ/UUID/epoch/generation/hash，旧转交链精确退休后重新完整读取/已阅/两账号审批；每次新消费者保留前代，初始/终态对象不能重用覆盖。native固定历史摘要6防递归复制，完整当轮任务/新风险/真实queued事实仍保留；同一原组/记录/部署，最多8个ground child（空候选也计费），源丢失/容量满/MODE已可能排队/未排空均不淘汰或恢复授权。实际pri/type/controller已接，限定源码复核完成，未编译/运行/测试/API/飞控改动/刷机/PR或打包。容量/新部署重置、HOME有效分支/local frame/运动转向、净空/最终StartPermit/时序/执行完成及原25目标/六类仍待开发。下方265–269及更早批次是历史记录。

265–269有限源码交叉审：actual8/27、ground28→独立30/Session6/Authority10、pending固定27/pure callback、actual第二Txn和完整冻结phase basis、真实33→34、跳ARM opcode0/2与整组journal屏障。修复审阅发现的两项断点：首次Seq0前周期RAW缺失/age超policygap即拒绝，不先发后补；实际消费者SourcesMatch提前到generation/目标覆盖前。限定复核上述修复未报告新具体错误，不代表完整审计或运行验证。

本批8个C++/QML文件去字符串/注释后仅文本分隔符计数匹配、尾空白0；git diff --check无输出，但untracked新文件不被它覆盖，已另做逐文件文本检查。并未调用parser/moc/qmake/编译/测试/API。HOME/local frame/运动转向、多轮真实终态owner重审、最终StartPermit/净空/时序/完成与原六类目标仍未交付。

本批16个Markdown中240条本地链接存在、尾空白0；StartupTransaction/GroundStartupAuthority的pri头/源各一次且文件存在、内部type各一次，qrc XML根RCC。只核文本/路径与注册声明，没有生成工程、moc或运行QML。下一批实际新终态owner取证不能绕过当前第二事务事实隔离。

265–269 新增[地面再审后的分阶段启动事务](../../custom/src/Swarm/FleetMissionStartupTransaction.md)源码：actual8/context27、一次28→独立30（Session6/Authority10）、同UUID连续占用；独立第二实例不覆盖旧终态。已解锁成员不发ARM，但仍核实际Seq0完整journal与整组屏障；未解锁成员仅普通ARM(param2=0)，之后仅mode-only。首次Seq0前要求实际周期RAW基线/政策gap，错误实例在换generation前拒绝；后端接线且限定源码契约复核完成，无新执行按钮。本机未调用接口/编译/运行/测试、改飞控、刷机或更新PR/包。最终StartPermit/净空/完成、第二事务再次终止后的实际owner重审路线、HOME有效分支/local frame/运动转向及原25目标/六类仍待开发；旧第一终态不能代替第二事务事实。下方260–264及更早批次为历史记录。

260–264 限定独立只读串审：实际10来源/context27、Session独立28、移交wrapper6→Ledger10与pure4 callback；移交前后完整native源、原新READ/完整记录/typed冻结地面状态及原期限，旧22退休不盲撤新28。Root后端准备态不自拒原新grant，接受态不问oldReview.ack；撤销通知前捕获actual QQP/旧UUID，通知删除或replacement后仍精确释放旧占用。另一个审阅者曾误计QStringLiteral/value/uuid三层括号，重读后撤回，未错误删合法右括号；有限审阅未发现本链新具体阻断，不推导编译/运行安全。

8 个本批 C++/QML 去字符串/注释后的文本括号计数匹配，gitdiffcheck无输出；不是parser/moc/qmake/编译或测试。真正模式消费者/分阶段许可、完整参考系处理/运动转向、净空/最终StartPermit/完成与原六类仍待代码开发。

260–264 新增[地面再审启动依据一次接管](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)源码：实际10来源/原始context27、完整新RAW/全风险/记录/typed当前地面状态进入新owner，旧22一次移交Session独立28（6字段返回真实remaining）、Ledger10字段接管。旧账本退休不盲撤新owner，原最早READ START/两身份期限只缩短，独立实时查身份/WTS/源与整组占用；public不泄露SID/RAW，所有执行/启动/飞行许可false。实际后端/pri/type已接，本批限定源码契约静态收口已完成，无读取/ARM/MODE或UI执行按钮。真正分阶段模式消费者、HOME分支/local frame/运动转向、净空/最终StartPermit/完成及其它六类/原25项目标继续；未编译/运行/测试、调用接口、改飞控、更新PR/源码包/安装包。下方255–259及更早批次为历史记录。

255–259 限定源码契约核对：actual Sources9/ReviewContext20/public receipt27/Session proof22/private grant14；真实次账号提示与已完成Review仍活的native身份源、pure consume callback和Session OS前后核对、Ledger consume前后完整风险与原记录/新RAW、异步消费通知/精确撤销。独立审阅者读取最新完整方法后未报告本链路具体新增阻断；另一个审阅者中途容量失败未计作完成。修复已完成Review在真实modal开始时误撤销的路径，active READ 与实际消费仍拒绝未完成提示，不续原最早READ START期限。

8 个本批 C++/QML 文件去字符串/注释后文本括号计数匹配，git diff --check无输出。16 个本批 Markdown 的 215 条本地链接存在、无尾随空白，新h/cc的pri各一次且文件存在，内部类型注册一次，qrc XML根RCC；不是parser/moc/qmake/编译或功能测试。源码有限收口不证明新startup authority、模式续作、完整参考系处理、净空或任务完成；上述及原六类产品范围继续开发。

255–259 新增[地面再审独立账号条件意图](../../custom/src/Swarm/FleetMissionGroundStartApproval.md)源码：实际新 Review20/九对象/冻结整组与 armed 状态、完整最新记录/风险/新 RAW，真实两账号新提示和 Session 独立22证明，一次消费/撤销及 native14 grant；public27 receipt 无 SID/RAW。最早 READ START 原30秒不续，旧证明不恢复，实际提示期间主身份仍查 SID/role/WTS/旧期限；active READ与消费不放宽。实际 backend/pri/内部类型已接，本批已完成限定源码契约静态收口，没有新增执行按钮/新 startup authority/MODE续作。最终许可/净空/HOME有效分支/local运动转向/完成及其它六类/原25目标继续；未编译/运行/测试或调用接口、改飞控、更新PR/源码包/安装包。下方250–254及更早批次为历史记录。

250–254 审阅口径：只做源码文本契约复核，初次发现终态风险投影会删除 caller RAW 哈希/计数/boot 字段而未比对实际原文，已改为原生保存完整引用后 exact compare，当前捕获不覆盖引用；新增读取必须允许本实际 scope 的 mission reply 栈继续下一 INT 包，初始入场仍全栈排空。还按本地 ParameterManager 头文件修复不存在的 observation source 信号，改用真实 ready/missing/pending 与 Link 生命周期通知，版本/参数内容每个 fence 另重验。

本批 11 个 C++/QML 文件去掉字符串/注释后的 `{}`/`()`/`[]` 文本计数匹配，`git diff --check` 无输出。14 个本批 Markdown 的 202 条本地链接均有对应路径、无尾随空白；新 h/cc 在 pri 中各一次且存在，内部类型注册一次，qrc XML 根为 RCC。此项不是语法解析、moc/qmake/编译或测试；不将独立审阅者的中途容量失败当作“全部审计通过”。真实新授权/MODE 续作和原六类产品范围仍未完成。

另一审阅者随后完成限定只读契约核对：实际初始终态与自有 READ 接收栈例外、pure owner/full gate 分离、实际参数 bool 信号、controller Sources8/ContextAllowed 和注册路径，未报告具体新增阻断；没有编辑文件或编译/测试。此结论仅覆盖上述源码范围，不认证完整产品、运行时安全、机载语义或未实现的后续新授权/执行。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](../../custom/src/Swarm/FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

244–249（2026-10-14）本批有限独立源码审阅已收口。新actual Transaction/32字段收据/持续journal/typed模型、原身份占用一次移交、policy和RAW/稳定风险区分已落盘。已定位修正字段/计数域不一致、seq0无ACK的lane关闭、请求接收与成功混淆、同值坐标年龄误失效、未批准local XYZ/heading及着陆枚举往返；有限独立审阅未发现本批已审路径的新具体阻断，不代表编译或完整动态启动验收。仅源码/文本检查，未使用parser/moc/编译/业务接口/软件/测试或硬件；HOME再审/frame/运动/转向/净空/最终StartPermit/完成和其它原目标继续。

底层 MAVLink ACK/PlanManager operation identity 的专项证据见 [`MERIVUS_ACK_IDENTITY_AUDIT.md`](MERIVUS_ACK_IDENTITY_AUDIT.md)。

244–249最终文本证据：15份本批C++ source/header/QML去注释和字符串后三类分隔符计数一致，13份非legacy全文尾空白为0，Vehicle.h/.cc保留旧9/13处（与HEAD一致），未清理无关格式。19份Markdown的192个本地链接存在且全文无尾空白；四个新h/cc在pri各一次且文件存在，新Txn内部注册一次，原qrc XML根RCC可读取，git diff --check无报错。文本检查曾定位Transition未观察分支缺一右括号，已修并全量重核；SHA文本比较另改为明确QString/digest形状。独立只读终审未发现当前已审链路的新具体阻断，范围仅代码可见契约，不是语法解析、Qt/moc构建、业务/身份/WTS/MAVLink/API或安全测试。没有发包、运行软件/界面/仿真、修改飞控、提交或更新PR/安装包；原六类/25项目标及完整动态启动/再审/净空/执行完成仍未完成。

日期：2026-10-11

增量复核：2026-10-14；最新补丁的证据仅来自源码路径和静态差异检查，未执行应用、编译或测试。旧条目所列检查不自动代表覆盖本轮所有新路径。

本记录只描述源码级审计，不构成构建、仿真、硬件测试、刷机或放行证明。当前工作树的改动范围主要在 `GroundStation/custom/`、`GroundStation/docs/`，以及 QGC 通用 `src/FlightDisplay/`、`src/Vehicle/Vehicle.*`、`src/MissionManager/PlanManager.cc` 和 PX4 地面站插件文件；PX4 飞控仓库未修改。

## 本轮检查

239–243一次Startup依据接管：actual十对象bundle/私有实际Authority source回调、最新非恢复全记录/完整RAW33稳定拷贝与native34/private31对照、目录及其提供者/health源与Leader/每机完整BeforeArm物理map/hash，已跨native Session/StartApproval/Authority/Readiness/Review/controller源审。原Conditional consume→完整grantCurrent→actual强castAuthority一次移交→本地整组reserve→首尾完整再核，原Review/READ START/双方SID角色/WTS与身份期限只缩短不renew；handoff后不再调用旧Review/Content/Approval.valid。旧ledger转transferred/断watchers/退休旧proof及grant；新22字段认证/9字段wrapper与25字段native context保持实际owner/startupUUID/epoch/sourceHash，三飞行许可/执行字段false，没有任何ARM/start/send/write调用或UI执行入口。

审阅发现并修复的三个具体契约点：Authority.current未给controller依赖的deploymentId，准备阶段owner callback全拒，已补实际冻结字段；Session对旧startup proof expiry/current的WTS查询可重入提交新依据，现每次查询冻结旧receipt/owner/SIDbasis并后验同一原scope再清，不能把新代际清掉；passive retained-read绕开旧groundadmission也遗漏native接收栈边界，已加mission/version receive-depth/exhaustion检查，不重新引入ARM/grounded要求、不接管尚未交到RAW ledger的旧报文。Authority与ledger失效/显式转发取消均冻结generation/startupUUID/ownerEpoch/sourceHash，emit/回调后CAS，借用QString参数先复制；self在conditional consume中销毁会精确清对应孤立旧proof，移交后owner销毁清确切新proof，不能留占用或误清新scope。

typed候选用独立armingDeferred/真实其它conditions，不删除manual-arm blocked或改变原preconditionsMatch；仅普通RAW PX4 POSCTL/nonAUTO/nonHIL/knownstatus/ON_GROUND及实际not-flying和全部声明域依据通过，BeforeArm/ArmedGroundReady及未知/截断分开。它不证明机载canArm/中立遥控/电机或地面净空，也不认证固件/路径。源码另核本地插件先Mission后ARM、MISSION_START自身也会ARM、DO_SET_MODE只模式但有main/sub参数语义、MISSION_SET_CURRENT无COMMAND_ACK；这些尚待专用native adapter/允许差异/完整事务，当前实际ARM仍使Authority失效，不能称跨ARM已完成。

最终独立只读审阅未发现本批已审路径的剩余具体阻断，仅限可见源码契约。14份C++ source/header/QML去注释与字符串后三类分隔符计数一致（不是语法解析），13份全文尾空白0，PlanManager.cc旧7处与HEAD一致；19份Markdown全文尾空白0、194本地链接存在，git diff --check无报错，新Authority h/cc pri路径和MerivusInternal注册各一次且文件存在。6/10实际来源、3参数factory、5参数ledger移交/7参数Session移交、22/9/25/34字段契约按源码对齐。未使用Qt/moc/qmake/编译器、语法解析器、业务/认证/WTS/MAVLink API、软件/界面/仿真/测试，未改飞控、提交或更新PR/安装包。完整最终StartPermit/受控命令过渡/执行完成及原25项/六类实质源码范围继续保留。

234–238启动前整组机载内容核对：原生factory三参数（实际PRIMARY、callerUUID scope、mandatory by-value授权fence）、prepared/active ownREAD、原始未调整INT来源/target/count边界、独立pending cache和失败取消旧cache保留、实际local read-endACK排队后才成功，已按源码与新服务交叉核对。服务actual六源+owner、全记录/七绑定、固定全组、34字段native收据/16字段RAW项、GLOBAL_INT5→AMSL GLOBAL0显式规范比对、原120秒/单机期限及最早READ START30秒；18字段native context只排除remaining进入九参数Review，source/prequeue/撤销/排空均保留原范围，不认证无线因果/原子快照/硬件，也无StartPermit/飞行或完成。

独立静态审阅发现并修复：progress直回调取消后只检查QObject仍活而继续进入legacy无fence ACK路径，现核frozen operation/TransactionRead/scope且list/next/readComplete都拒绝已结束事务；旧COUNT/ITEM已进入Vehicle接收栈时，插件或通知可创建新scope导致旧观察归入新读，现从原消息冻结处建立mission receive-depth入口guard，栈内不允许新scope工厂（既有实际ownedREAD的回复与后续请求仍可继续）。这些是本地原始观察时序保护，不声称修复无线协议无request token。

Review最后fullcontentCurrent本身含WTS/ownerQML，放在物理捕获尾部会在回调改变机位后保留旧风险，已移到全部最终物理capture之前；Target另冻结实际PRIMARY和原readScope/op，尾部仅被动原生target/receipt/cache/source证明和原期限检查，不再调用身份/owner回调。完成只读事实使用actualSession.identitySnapshot同原身份，允许独立次账号prompt busy但继续核WTS/SID/role/原TTL；active/prequeue仍拒绝busy，新主身份、锁屏、退出/撤权仍失效。Controller先撤销旧批准、ownREAD reservation不自拒；UI默认未勾选、press/返回后实际部署/全绑定/目标/身份/权限核对与匹配取消，无Start/ARM。最终独立只读审阅在当前已审源码路径未发现剩余具体阻断，结论不外推SDK/现场/协议时序。

本批文本级证据：14个C++ source/header/QML去字符串/注释后三类分隔符计数相等（不是语法解析）；11份全文尾空白0，PlanManager.cc旧7处、Vehicle.h/cc旧9/13处与HEAD相同。15份Markdown全文尾空白0、184个本地链接存在；git diff --check无报错，新h/cc的pri路径及MerivusInternal注册各一次且文件存在，旧qrc XML根RCC结构可解析。新3/8/9参数原生/服务/Review调用与34/16/18字段契约按源码对齐；未使用Qt/moc/qmake/编译器、语法解析器、业务helper、认证/WTS/MAVLink/API、软件/界面、仿真或测试，未修改飞控/提交/更新PR/安装包。完整受控启动/状态过渡/执行完成及其它六类源码工作仍在原路线图中，不以本批静态检查或新增文件数量称产品完成。

229–233独立条件启动意图：Session精确mission.upload/mission.start分离、16字段原生依据原期限/一消费/current/精确无emit撤销；StartReview native17字段实际已阅上下文、完整不可变artifact/source及原30秒，StartApproval actual六来源/全记录/34字段receipt/13字段conditional grant/实际startupId和全Vehicle/MM/PM/VLM/PRIMARY或明确原缺失；callback/WTS/modal/原clock剩余的前后来源、账号/角色/目标检查已交叉源审。不接受上传lease、callerJSON flags、原风险自动rebase/新起算期限或条件收据作StartPermit，三个permit/executable/flight字段始终false，未新增Vehicle命令。

静态审查发现的可用性问题及源码修复：原指纹包含同值receipt、GP raw boot clock，而healthChanged又因age推进全撤销，人工已阅/独立输入会无意义失效。Vehicle保留RAW v1/完整内容身份及原boot回退保护，新native风险v2在成功commit时独立单调risk hash/rev，仅GP去timeBootMs，风险变化/失效/ABA保留revision；Readiness重验完整raw/risk hash后才构造独立schema2风险投影并去age/receipt，Progress风险比较同义但仍不归属reached。健康通知比较冻结实际health gate/具体Leader与Registry/Fault来源，纯通知不延期/更新artifact，真实风险/门槛/源变化仍撤销。小幅真pose/速度噪声仍需重审，没有偷偷加容差。

另修复较短Review期限过期替换/析构留下较长Session消费依据，以及消费factory/WTS回调删除账本时还未赋给成员的孤立proof；用原copied approvalId及实际QPointer session匹配无emit撤销，不能清新授权。grantCurrent在首次native/WTS前置busy防直接重入。Review借用参数先复制、每个readiness/progress/getter/native callback后验实际QP/代数，源失效emit后不再使用this；当前健康检查也拒绝重入。独立最终只读复核未见本批新增具体阻断，结论仅限可见源码，不认证SDK/系统/协议/现场时序。

本批文本级检查：13个C++ source/header/QML去注释/字符串后三类分隔符计数一致，11份全文尾空白为0，Vehicle.h/cc保留旧9/13处（与HEAD相同）。16个Markdown全文无尾空白、162个本地链接存在，git diff --check无错误；新StartApproval h/cc的pri路径和MerivusInternal注册各一次、文件存在，旧qrc XML根RCC结构可解析；Review17/Session16/approval34/grant13字段数、native接口与实际调用按源码对齐。未使用语法解析器、Qt/moc/qmake/编译器、业务helper、Windows认证/WTS/MAVLink、界面、仿真或测试，也未修改飞控/提交/更新PR或安装包。

完整鲜机载内容、真正StartPermit/受控ARM及启动事务/界面、执行/完成归属与其它六类源码目标仍待开发，不能以“只剩工程师测试”交付。原30秒包含账号窗口，32目标资源上限不保证现场完成，账号不同不证明两名人员；这些窄静态结果不证明当前软件能编译、可实飞或整个产品完成。

225–228只读证据刷新：原生实际 PRIMARY/PM/Vehicle 与成功 RAW receiptSequence、同值新到达而非 cache/prepare/content revision、固定十二项名称/完整16字节字段、单次版本命令队列及有限ACK隔离、参数不碰旧cache/wait maps/FTP/写入；实际Session/Role/private owner、唯一整数目标/资源上限、版本后捕获PM epoch、逐阶段与最终来源二验、有界取消与原期限，已交叉按源码复核。计数不进入稳定内容指纹，始终不认证请求因果/硬件/签名，不恢复旧批准或授予Start/ARM。

独立源审发现并修复三个实际问题：首次/变化版本同步清旧 raw HB/landed 后，下一参数误作永久context失败；允许原生明确缺状态的status-wait在原4秒阶段等新地面证据，已知解锁/非地面或真实源错误仍拒绝。固定字段错误地拒绝长度16的合法参数名，改为完整16字节无NUL尾。发送插件可取消/退出/撤权/销毁或耗尽阶段，而仅post-send服务复核不能阻止未发送包；改为 mandatory 按值 C++ closure，在plugin前/后与真正prequeue重核实际请求/原身份权限/owner/对象/4秒与120秒，回调后再核物理来源、PM epoch、自己的entry和不可变报文，空fence拒绝。

WTS/getter可能同步通知或销毁对象：服务来源 getter包装按QPointer/代数在SourceMatches/getter/SourceMatches各调用后检查，身份/owner gate首尾保留原上下文，callback后重新取单调时间，不用旧tick时间滑到新阶段。初次版本后PM epoch重新捕获；与主身份无关的未使用approver到期不误撤销同一primary读取。控制器先复制严格目标列表再查询/撤销；UI冻结按下的实际控制器/身份/权限/机号，返回后换源尝试匹配取消旧请求且不在新面板报旧成功。接口和值提示均不含自动上传或批准恢复。

独立最终只读审阅未发现本批原生/service新增具体阻断；最后轻微UI换源成功提示由root补尾验。本批文本级检查：10个C++ source/header/QML去字符串和注释后三类分隔符计数一致；8个全文尾空白为0，Vehicle.h/cc保留与HEAD相同的9/13处旧空白，不清理无关改动。12个Markdown全文无尾空白、157个本地链接存在，git diff --check无错误；新h/cc的pri路径和MerivusInternal注册各一次/文件存在，旧qrc XML根RCC结构可解析，Version2参/PM4参的mandatory fence声明与实际调用、十二项名称与证据helper按源码对齐。

没有语法解析器、Qt/moc/qmake/编译器、任何业务helper、身份/Windows/MAVLink API、应用或界面运行、测试、仿真、上传/写入/刷机；未修改飞控或更新PR/安装包。这些窄静态检查不证明编译、界面或协议时序/安全正常。服务120秒历史年龄不替代审阅30秒参数时效，32目标不保证采集完成/六机协议扩容，完整独立启动/完成及其它六类实质源码目标仍未完成。

221–224源级审阅：请求60秒与明确消费后的有限upload lease分开，原操作员5分钟上限保持；schema2 receipt22/grant13、同deploymentId/本地原生lease/完整记录和actual来源、一次消费、不更新截止、有界政策/作用域进入实际摘要与UI。ledger取原生调用前时间+获准duration，不混比较clock；旧已消费授权须明确撤销/到期才可新申请。独立60秒request-approver失效不能错误撤销仍有效的consumed lease，但主身份/双方角色、实际desktop或lease过期必拒绝。

Windows工作站来源检查包括actual process/session、WTSInfoEx Level1/足够长度/明确active/unlocked、Windows10+真实版本与原生通知注册/资源生命周期；旧反向flag/未知/失败不推断可用，锁定/注销/断连清epoch与lease。WTS原生消息查询可能同步调用窗口过程并删除来源，新增current/timer/入口及ledger调用点QPointer/代数/实际源尾验；不是简单把const查询当无重入。未运行Windows API、窗口、认证、应用、编译、测试或真实上传。

独立交叉审阅最后修复调用方alias生命周期：系统审批入口先复制Ledger成员引用传入的action/hash/summary与收据，再作原生查询/emit/modal，调用方被删除不会在返回后读取悬空QString；候选grant的嵌套identityLease也先按实际原生严格类型复核，不能只比retained Map绕过Qt兼容相等。源码复核未见这批新增具体门禁旁路，结论只覆盖可见源码范围，不认证整个应用/Windows资源或现场时序。

本批文件级检查：5个C++ source/header与1个QML去注释/字符串后{}()[]计数一致，6份全文无尾空白；12个Markdown的136个本地链接存在且无尾空白，git diff --check无错误。新consume9参/grantCurrent2参/Sessionlease API与实际调用按源码对齐，pri中Wtsapi32登记一次。没有语法解析、编译、运行、认证、WTS调用或测试；4分钟授权上限不延长其它版本/参数/健康/传输deadline，不保证32目标/8192项完成，也不替代正式续授权、启动/完成或全目标开发。

217–220源码交叉审阅：显式Windows secure credential prompt/受保护unpack/INTERACTIVE LogonUser/actualTokenUser SID、原生secret缓冲清理/非复制RAII/令牌对齐及资源释放、固定SAM直接SID角色组的有界完整读且无resumehandle、严格11/16身份字段与GUI/modal/QPointer/TTL、窗口外角色撤权通知。独立审批冻结实际session/policy/store/private owner/完整record与bindinghash，显示源派生record/revision/plan/target摘要，postmodal及每步尾验、严格20receipt/8grant、一消费不续期、排队按精确grant发布状态，有效consumed不得暗中替换。系统账号不同不是两名人员证明，仍无企业鉴权/锁屏实时撤销/跨站租约。

交叉源审修复：Deployment消费原始grant却期待ok字段而全部拒绝；认证回调前后raw来源生命周期；审批按钮只比sessionId没有全identity/policy；TokenUser对齐、secretRAII复制及emit后读取销毁对象；组撤权未通知；消费后UI滞留approved状态；系统窗口只有hash缺可读摘要。当前gate明确复核原生grant，不以callerflags/本地role编辑产生授权。60秒覆盖prompt和整段传输，不悄悄延期，停止/排空不回滚；没有认证/上传/Start/ARM接口调用。

本批文件级静态检查只核文本分隔符、全文尾空白、工程路径/注册/调用签名、Markdown本地链接与git差异空白；不是语法解析、Qt/moc/编译或业务/安全测试。独立只读审查结论以源码可见范围为限，真实凭据提供者/系统账户权限、modal/主题键盘、延迟组查询与多机传输由工程师后续验证。源码目标整体未完成，未更新PR/安装包。

Qt5接口交叉复核另修正实际数值往返：官方[QV4 toVariant源码](https://raw.githubusercontent.com/qt/qtdeclarative/5.15/src/qml/jsruntime/qv4engine.cpp)与[JSON数值解析源码](https://raw.githubusercontent.com/qt/qtdeclarative/5.15/src/qml/jsruntime/qv4jsonobject.cpp)表明合法JS整数可变为QVariant::Int，不能强制所有schema/time为Double。身份/审批数值接受实际Int/UInt/LongLong/ULongLong/Double且有限、整值、0..2^53−1；Bool/String/小数/非有限/越界仍拒绝，原生完整收据核对前校验/规范化。这不放宽actor/epoch/版本/来源/一次性或三类boolflag。

最终文件级检查：7个本批C++ source/header与2个QML去注释/字符串后的{}()[]计数一致且全文无尾空白；12个Markdown的136个本地链接存在、无尾空白，git diff --check无错误。新两类的四条pri路径和两个MerivusInternal注册各一次，实际文件存在，原qrc XML结构可解析；声明/四参原生审批摘要、14参Deployment调用与状态回执按源码对齐。没有调用业务helper、认证/组配置、QML/应用、语法检查器、编译或测试；这些检查不证明安全认证、登录/界面/多机传输正常或整产品完成。

212–216静态交叉审阅：upload UI与原controller/private backend/资源接线、真实encoding.members.itemCount统计、版本/操作者/策略冻结、默认取消和非默认危险提交、matching请求取消/一次消费及失败/排空非回滚；RolePolicy九字段实际String/Bool、epoch/不回绕revision、未知/退出角色回退与实际分配一致，原生begin/每步gate核实际owner策略来源和收据，第二批准缺口不能靠checkbox绕过。历史全内容读回一次保存、liveflag仍失败/ARM撤销，NONQML整组/精确对象/idle context只作来源；新独立30秒风险审阅重新捕获最新非恢复plan/hash、actor/健康/原生证据/完整cache，已阅非授权，原store.matchesBinding未放宽。

本轮源审发现并修正：role receipt仅Map==导致bool可被整数兼容相等混用；未知新用户继承上个高权限角色；临时QVariantMap的悬空迭代；历史context漏掉异步manager.inProgress；UI把编码摘要误读为payloads/items、flat新风险artifact误读为nested、人工ARM后staging.failed被前端拒绝新历史审阅，以及health gate不应无条件新增Leader需求。没有从默认缺账号/额外批准造放行、恢复old verified、自动ack或新增Startup dispatch；本轮仍不证明登录、第二人批准、真实执行/完成或全25目标实现。

212–216文件级静态核对：7个本轮C++ source/header和3个QML去注释/字符串后的三类分隔符计数一致；6个未跟踪代码全文尾空白、11个Markdown的126个本地链接/尾空白检查通过，git diff --check通过。新StartReview两文件的pri路径各一次、实际存在，MerivusInternal注册/调用声明按源码对齐；新panel的qrc XML结构可解析且alias/qmldir各一次，实际路径存在。Popup/按钮/主题属性与当前QGC源码逐项核对，未运行UI或语法/Qt编译。独立只读终审确认以上指摘已关闭，无新增本批可见阻断；这些窄检查不证明编译、真实布局/键盘/主题、登录/审批、机载传输或任务执行完成。

208–211源码交叉复核：六通道原始flight-state来源/合法zero-tail、单一真实Link/精确ID、value hash/单调age/decimal revision与receipt、epoch/token/QPointer、flight/version callback-depth以及版本/链接生命周期撤销；同源GP timeBootMs小差回退撤销版本/参数/flight可用证据，末操作signal与caller owner检查闭合。自然wrap/恒0/重排不认证boot。StartReadiness严格原始profile/payload/实际参数版本声明、源二验和当前位置起飞/最低高度/连接/AMSL检查；ProgressObservation完整cache-wire比对/源二验、raw序号映射、unbound reached、未知与无限HOLD/非认证LAND完成边界。

源码复核指出并收口未解锁但preconditionsMatch仍为true的字段矛盾、none起始机位超声明高度未共用阻断，以及GLOBAL_POSITION_INT毫米量化不能精确重建PX4内部float的推导边界；名义值与保守量化上界分开，不认证真实航迹/定位误差/净空。Deployment主动只读检查冻结context/binding/对象与原staging gate，1秒snapshot-only显示、忙传输/源变拒绝、ARM不复活审批；QML仅有界摘要而非完整证据，未新增启动dispatch或UI按钮。配套固件只读核对MISSION_START自动arm、Navigator独立设index、DO_SET_MODE忽略ARMflag，以及current/reached没有完整任务身份或终态。所有执行/完成归属许可false；原六类剩余源码目标未缩减。

208–211文件级检查：8个本轮C++ source/header及1个QML去注释/字符串后的三类分隔符计数一致；检查发现StartReadiness一处缺失右括号并修复后全量重查。6个未跟踪C++全文尾空白、11个Markdown的119个本地链接/尾空白检查通过；两个新helper的四个source/header在pri各一次且实际路径存在，`git diff --check`通过。独立只读审阅未发现本批可见新阻断；没有语法解析、moc/qmake/编译器、helper/账本/档案IO/应用运行、测试或传输。这些有限静态检查不证明构建、任务执行、完成归属或全产品实现。

204–207 交叉源码复核：原始 AUTOPILOT_VERSION 在插件/Core/初连缓存处理前冻结，实际协议 sender/精确 ID/真实 Link、MAV1/MAV2 zero-tail、单调年龄/sourceEpoch/revision、token/QPointer/版本专用 depth 和最后提交维持来源；十二字段 JSON-safe canonical hash 与 UID/UID2-known 一致。首次锚定、内容/UID或来源变化同步撤销旧 PM 参数，修正新版本与仍新鲜旧参数混配；same raw/same source 不撤销，信号后不继续使用旧成员。仍只是本地到达顺序，不是线上的 packet generation 或身份认证。

目录严格 schema/type/semantics、Git5/Vendor3 与 sourceCommit 十位对应、完整 version tuple、hash/fresh/epoch/revision/capability/identifier、CAS/不回绕与 changed 重入 receipt、有界预解析/转义重复键/整批原子导入和原生 QSaveFile 已独立核对。依据本地配套固件发现并纠正“16hex纯Git”的错误假定；它是十位前缀加厂商版本，boardVersion 可是 product ID 或常数，均不认证真实构建、干净工作树、子模块、二进制或硬件。

helper schema2 以真实 API 与十二字段重算 hash，移除推进年龄但保留 fresh/stale/source/context，参数读取后再核版本/目录；Review 私有目录、changed/destroy清审阅、精确实例/源/版本尾验、显式 profile 三类证据门禁、归档身份/部署续作与 QML只读 facade 已交叉审阅。每组目录一次，成员 small match；所有原生审阅事件使用有界 source/归档摘要，未提供覆盖为 null，不重复巨大证据树。外部 JSON/源树/二进制不自动 watch，目录只反映构造/显式原生加载后的本地声明。

独立最终只读复核未发现本批新增可见阻断。声明对应不是认证或启动许可；所有签名/固件/硬件/起降/启动/执行字段仍 false，完整受控启动/完成/UI与其它路线图缺口未完成。未编译、测试、运行 helper/app、请求参数、上传或刷机，未修改飞控。

204–207 文件级检查：10 个本轮 C++ source/header 与 1 个 QML 的去字符串/注释词法分隔符计数一致，未作语法解析/编译；7 个未跟踪 C++ 文件全文无尾空白，`git diff --check` 通过。12 个 Markdown 的 116 个本地链接和尾空白检查通过；新 Catalog source/header 在 `.pri` 各一次且实际路径存在，Vehicle API/PM source-change/Review Catalog 的声明和接线按源码核对。没有调用 normalizer/matcher/ledger、档案 IO 或任何应用接口；这些窄检查不支持整产品完成、构建成功或实飞结论。

199–203 交叉源码复核：ParameterManager 仅跟踪默认 autopilot 的固定十二项参数，Vehicle 传递实际接收 Link，精确非零 system ID/default component/非回放附加链路用于 live 来源。缓存/FTP/文件与 local write/reset/refresh/超时、model/handshake/load/主次 Link 生命周期撤销旧源；单调年龄、内容 revision、manager epoch、token 和 Fact/Link 当前实例保留。完整参数哈希与稳定来源指纹分开，既有 PARAM_SET 最终策略二验及 bool 提交结果不冒充 ACK。

新增 helper 从实际 Vehicle/ParameterManager 只读采集参数与请求一致性，UID/固件/能力位明确只是 native-cache。Review 原生捕获、有界编码摘要与 blocking 风险，QML 原生准备/审批尾验、归档两个完整证据 Map、Deployment 最终 nativeEvidenceCurrent/版本/状态/健康/占用复核已交叉核对。value-only 历史不能作 live 来源；无 profile 的几何暂存不制造参数或执行认证，过期/来源变化停止后续步骤并排空，不能逆转已发内容。

审查发现并修正：共用 cache handler 的来源混淆、仅检查主链路不能排除次链路回放、次链路失联/恢复缺失效点；进度回调后 PARAM_SET 状态撤销，以及拒绝 retry 后重建 pending/误记 resend；Fact cooked/raw 通知后继续读取被删除成员与 deferred 标记覆盖嵌套更新；Vehicle raw PM getter/接收/ready 续作悬空来源，并同步 InitialConnect 的 QPointer connect 契约。受监控 Fact 被删除后只移除精确旧缓存键，避免下一次同名收包从旧 raw 构造 QPointer，不误删替代对象。

独立只读最终复核未发现本批新来源/上传接线的可见阻断。此为限定边界源码证据，未认证所有 callee 内部、全 Fact/Vehicle、同值 ABA、QML CONSTANT 销毁通知或运行时线程/泄漏；无可信固件适配、真实起降/净空、独立启动或完成证明，所有执行许可仍 false。没有编译、测试、运行 helper/app、请求参数、上传或刷机。

199–203 文件级检查：12 个本轮 C++ source/header 去注释/字符串后的分隔符计数一致；1 个 QML 的词法分隔符计数一致，未作 QML 语法解析。6 个未跟踪 C++ 文件全文无尾空白，其余改动由 `git diff --check` 核对并通过；11 个 Markdown 的 112 个本地链接及尾空白检查通过。新 helper source/header 在 `.pri` 各登记一次且路径存在，PM 方法/真实接收 caller、Vehicle getter 与 InitialConnect 友元使用按源码核对；没有调用 qmake/moc/编译器或运行测试。以上检查不能推导编译成功、函数已运行或整个产品完成。

195–198 源码交叉审阅：严格执行 profile 与整份成员覆盖、覆盖复制和巡航/明确爬升冲突、planId/存档保真、显式驻留与时序分离、五类命令/frames/NaN/保留零、INT/float32 来源及来源序号映射；新增 DO/TAKEOFF/terminal 项计入完整预算。Deployment 按实际读回命令分类，不从期望结果补造参数，完整 manifest 只保留在当前载荷，终态仅计数/有界摘要。

审查发现并修正 profile 根字段被旧档案白名单拒绝、Compiler/Builder 对覆盖速度冲突的可行性分叉、驻留未计入预算或被 max 掩盖明确时序矛盾、零跨度相邻权威到达时间跳过驻留检查，以及终点 LAND 前的 WP 指向地面和 float32 高度重合。LAND 现保留返航进近高度并使用显式更低地面目标，raw/wire 邻接、E7 与高度分离均检查；相邻间隔审阅允许零跨度，但能量正时长契约未放宽。估算标记、末点驻留与绝对时序没有被混同。

独立最终源码复核未发现本批次新增的可见阻断；仍只有静态证据，不代表输入/量化/时序算法已运行或真实 PX4 支持已获验证。所有状态仍不可执行；绝对时序、实时参数/机载版本、实际起降路径/净空及完成判定、独立启动许可和 UI 尚需开发。未编译/运行/测试、仿真、传输、刷机或放行。

195–198 文件级检查：9 个本轮 C++ 文件（含未跟踪新增文件）去注释/字符串后分隔符计数一致且全文无尾空白；14 个 Markdown 文件的 112 个本地链接及尾空白检查通过；新纯静态 helper 的 source/header 路径存在且在 `.pri` 各登记一次；`git diff --check` 无错误。没有执行 normalizer/compiler/builder/review、档案读写或命令接口，也没有进行语法解析、Qt/moc 编译、运行或测试。

193–194 交叉审阅补齐上传航点实际 parent 所有权与直接回调后的生命周期检查；修正原恢复流程同步 ready 在 finish guard 栈内启动的回归。原生续作冻结原 Vehicle/MissionManager、请求和本次生成操作，100 ms 周期有界等待栈退出；新的尝试/操作/换机取消旧续作。MERIVUS 以请求信号和最终 ACK/完整缓存比对消费确认，QML operationId 保持字符串，序列不回绕。

同轮又发现并修补两条真实衔接缺口：FlyView 重建任务引发自动普通 Start 弹窗清掉 uploading 请求，以及只冻结索引/数量导致确认期间同数量新航线被恢复。自动入口仅抑制有效同目标上传请求，主动取消/新动作仍有效；原 source content/manager epoch 在确认时冻结，重试沿用且 native 生成前复核。恢复后的列表缩短/重编号，以新 itemCount 和完整快照启动，不将源索引当新任务 current-index；这没有给 Fleet 几何暂存授予执行许可。

187–194 文件级静态核对：14 个 C++/QML 文件去注释/字符串后的分隔符计数一致；5 个未跟踪源码文件全文无尾空白，已跟踪新增行由 `git diff --check` 检查通过。原有 PlanManager.cc 的 7 行、MissionManager.cc 的 2 行尾空白未作无关格式清理，不宣称全文件无尾空白。12 个 Markdown 文件的 107 个本地链接/尾空白检查通过；两个新增模块的 source/header 路径及内部 Deployment 注册存在。独立源码交叉审阅未发现这批新增恢复路径的可见阻断；以上不是语法解析、Qt/moc 编译、业务测试、机载传输或飞行验证。

2026-10-14（187–192）源码审查覆盖几何载荷/INT 规范指纹、完整出航与返航、原参数保真、地面逐机 Write→独立 Read、完整归档身份、运行/暂停忙机门禁、共享占用、取消/失败排空以及阶段审计版本。审查发现并修补零度 yaw 默认误用、wire 量化/高度上限、无界 binding 附加字段、同步完成吞掉后续步骤、verified 自我失效、阶段日志读取错误层级，以及 PlanManager 准备/完成信号重入替换缓存的底层缺口。

隔离审查还修补按 ID 重找 MissionManager、断线重连解除隔离、旧完成信号误解除和 resume 克隆绕过；几何 Write 记录实际冻结 manager，系统 ID 隔离保留到明确的新普通上传，按精确本地 ACK 来源和缓存指纹解除。任务协议新增调用栈护栏，并覆盖 guided/resume 绕行路径和协议/计时回调；只能称源码边界与本地相关性增强，不是线上的 MAVLink token、跨站锁、账号认证、机载回读成功或飞行性能证明。

本阶段仍未编译、运行、调用上传接口、生成/读取真实机载任务或进行任何测试；没有启动/解锁/清空/回滚代码调用，也未接 UI 上传按钮。完整时序/速度/终止动作及受控启动继续待开发，25 项完整目标保持进行中。

183–186 新增原生草稿和地图规划面板，源码交叉审阅覆盖 native→coverage/compiler 的键/角色/坐标基准、UI 参数范围与类型、未保存文本、显示 revision/recordId 审批、当前计划成员渲染以及规划/旧指令隔离。发现并修正 Controls 1 `QGCTextField` 无 `textEdited` 的接线风险（新字段用 Controls 2）、UI 范围与 native 不一致、拒绝 checkbox 后绑定丢失、规划关闭后旧手势可能进入 GoTo、属性/健康调用重入后的最终发送门禁、自动定位/任务缩放和紧凑面板高度。辅助路线每页 12 点，地图完整路线视图总点数上限 8192，超限不绘制合法前缀；界面使用 QGC 主题、明确文本/字段标签和键盘输入，不引入新字体或网络资产。

本轮只审查源码及资源/构建引用，不运行 UI、生成器、归档读写或任何业务测试；静态检查不证明 Popup 焦点/布局/颜色对比度、实际几何或 Qt 类型检查。代码不包含新的任务上传/执行路径，也不把 32 草稿成员当作机载协议能力。完整 25 项目标保持未收口/未验收，目标模式仍继续。

183–186 静态文件检查结果：7 个本轮 C++/QML 文件（包含未跟踪新增文件）去字符串/注释后的分隔符数量一致且无尾空白；9 个 Markdown 文件的本地链接和尾空白检查通过；QRC XML 可解析，新面板 alias 唯一且路径存在，qmldir/native 注册和 `.pri` source/header 引用一致；`git diff --check` 无错误。仅是词法计数和文件契约核对，不是语法解析、Qt/moc 编译、业务测试或视觉验收。

179–182 静态追踪新增端点/3D 估算/全路径高度、不可变计划版本和精确批准回执。独立审查发现并修正 JSON 大整数精度、UTF-8/读尺寸、陈旧多实例写档案、损坏档案覆盖、载入信号重入解除隔离、审阅世代混用、旧取消/拒绝影响新版本以及 revision 恢复 ABA 撤销问题。新库使用完整 recordId/revision/hash/epoch 撤销绑定；历史恢复无 live/批准，交接仅有审计引用。以上仅源码静态推理，未运行档案读写、规划器、应用、编译或测试。

本批次独立尾空白/词法分隔符检查覆盖 10 个 C++/QML 文件（包含未跟踪文件），工程引用覆盖 5 个模块的 `.cc/.h`，相对链接/空白检查覆盖 12 个 Markdown 文件；`git diff --check` 通过。QML 四处撤销调用与 backend 完整绑定签名一致，新类型仅注册到内部模块。检查不是 Qt/moc 编译或业务测试，未检查实际文件锁、磁盘失败或几何运行结果。

区域规划增量（2026-10-13）：源码路径复核覆盖扫描线逐对求交、稳定逐机航段分配、凹形区内连接、输入资源/基准门禁及失败不保留前缀；编译/审阅核对逐机时间、返航回退、估算标记与能量预算。静态审查发现的混合 bounds、区域/顶点 `localFrameId` 冲突、别名深层比较和返航基准旁路已收紧；这不是对几何正确性的运行证明，未运行规划器/编译/测试。工程师需要按新模块契约执行几何和时序验证。

交叉审查还发现并收紧任务最小间距未接线、checker 基准声明未消费、生成成员元数据矛盾以及空域比较/诊断无界路径。覆盖预览日志采用有界摘要并记录实际记录结果。静态检查只验证词法分隔符、行尾空白、构建文件引用与 Markdown 路径，不证明 Qt 类型检查、算法正确性、界面响应或实际运行。

空域预算静态契约：512 个 plans / 1024 zones / 16384 raw points / 100 万潜在比较 / 2048 合计诊断；任一失败输出 `analysisComplete=false`、`feasible=false`，协调器据此保持待复核。基准门禁的不兼容结果不会凭高度/时间排除空间风险。该契约未经过运行时边界测试。

上述修补后的独立源码复核未发现新增阻断缺陷；7 个本轮 C++/QML 文件（包括未跟踪文件）的词法分隔符计数和尾空白检查、4 个模块的 source/header 工程引用、12 个 Markdown 文件的相对链接与尾空白检查及 `git diff --check` 已完成。没有将词法检查当成编译，也没有运行区域生成器或空域检查器。

- `git diff --check`：通过。
- `custom/src/Swarm/*.cc/.h` 与 `custom/res/Merivus/*.qml`：括号平衡检查通过。
- `custom/custom.pri`：Swarm 源码和头文件均有 qmake 注册；同时恢复了现有 `CustomAutoPilotPlugin`、`CustomFirmwarePlugin` 和 `CustomFirmwarePluginFactory` 的源码/头文件与 include path 注册，避免 custom PX4 插件选择依赖未列入工程的源码。
- `custom/qgroundcontrol.qrc`：XML 可解析；新增 Fleet 面板路径存在。
- `custom/merivus_ai_panel.qrc`：XML 可解析；`MerivusAIAssistantPanel.qml` 仅在专用资源中注册，并由 `QTQUICK_COMPILER_SKIPPED_RESOURCES` 保持解释执行，避免与主 QML 资源重复注册。
- `custom/res/Merivus/qmldir`：新增面板条目与文件名一致。
- `docs/` 和 `custom/src/Swarm/*.md`：相对 Markdown 链接检查通过。
- 工作树范围检查：本轮地面站修改覆盖 `custom/`、`docs/`，以及 QGC 通用的 Vehicle、MissionManager、FactSystem、Joystick、QmlControls、FlightDisplay、AutoPilotPlugins、AnalyzeView 和相关资源；准确文件集合以 `git status --short`/实际 diff 为准，PX4 飞控仓库仍未改动。
- 本轮增量复核：链路/故障/连续性/报告/数字孪生的安全数值拒绝 numeric string；Registry 心跳、交接记录和黑匣子事件拒绝明显未来时间；数字孪生持久化导入强制校验 `snapshotHash`；报告布尔字段拒绝隐式转换；黑匣子导入要求完整的 `timestampMs/type/source/payload`；MissionHandoff 导入拒绝未知字段并要求完整的全局 `sequence/revision` 与记录 `recordRevision`。
- RSSI 未知值复核：`Vehicle` 的 `RADIO_STATUS` 未知/未初始化值不会再被换算为高链路质量；只有负 dBm 证据进入 Registry，其他值保持 `linkQualityPercent=-1` 未知。
- 离线证据清理复核：`SwarmController::_refreshFleetRegistry()` 在车辆离线后会清掉仍残留的电量、链路、估计器和定位健康证据，已清空的离线快照不会被每秒重复写入。
- 标准任务启动复核：`SwarmController::executeStartMission()` 对目标 ID 做严格整数校验，重新检查 MissionManager 缓存、链路、模式和严格任务启动健康证据；事务使用 `MAV_CMD_MISSION_START`，ACK 只表示接受，`missionFlightMode` 遥测才完成事务。QML 的启动/继续/多机启动动作已无直接 `Vehicle.startMission()`。
- 临时 Mission 启动复核：上传回调从 `Uploading` 进入 `Starting`，收到启动 ACK 或 Mission 模式遥测后才进入 `Executing`；启动拒绝/超时设置失败原因并进入清理，清理成功不会把失败事务改写为成功。
- 公共批量目标复核：起飞、降落、返航、Goto、编队和任务启动入口拒绝字符串、小数、非有限值和重复 Vehicle ID，避免 `QVariant::toInt()` 截断到错误目标。
- Registry 导入状态复核：新注册描述中的历史 `lastSeen` 不再把显式 `online=false`（或缺失在线声明）提升为在线；只有显式在线且缺少时间时才补当前心跳时间。
- 能力发现入口复核：`FleetCapabilityDiscovery::validateDescriptor()` 现在在进入 Registry 前拒绝字符串/小数 ID、冲突别名、非布尔健康字段、越界/非有限数值、未来时间戳和非字符串静态字段，避免把隐式 QVariant 转换当成可信证据。
- 能力发现运行时接线复核：`SwarmController::_refreshFleetRegistry()` 使用共享 `FleetCapabilityDiscovery` 归一化基础心跳后再补充实时健康字段，`GuidedActionsController.capabilityDiscovery` 仅暴露该无传输依赖的入口；未宣称真实 MAVLink 能力证据已经完整接入。
- 模板目录导入复核：`FleetTaskTemplateRegistry::importJson()` 拒绝未知 root/template/signature 字段后才替换目录，损坏或扩展字段不会静默改变模板语义。
- 能力护照导入复核：`FleetRegistry::importCapabilityPassport()` 拒绝未知根字段、未知审批字段和非法导出时间戳，并在全部车辆/审批校验后原子替换。
- 能力身份复核：发现描述的 `discoveryState` 只在严格 1--255 整数 ID 下标记为 partial；护照审批中的 `systemId` 拒绝小数/字符串截断后再与车辆 ID 比较。
- QVariant 列表边界复核：审阅、任务编译、模板、空域、数字孪生、报告、风险和角色分配入口对 `QStringList` 显式展开，避免依赖 Qt 版本的隐式 `toList()` 转换而把合法角色/能力/信号/路径误当成空列表；扩展元数据的重复释放权限校验已去重。
- 健康中心目标 ID 复核：`FleetHealthCenter` 门禁入口只接受 1--255 的有限整数，并拒绝 `systemId/vehicleId/id` 冲突或隐式截断，避免 QML 输入把错误目标映射成另一架机。
- 事务重试/定时器复核：`CommandTransaction::canRetry()` 不允许从未启动的参数校验失败进入重试；`_handleTimeout()` 重新核对当前 timeout policy 和 elapsed deadline，停用或调整定时器后的残留队列回调不会误结束事务。
- 事务实际发送边界复核：`SwarmController::ManagedTransaction` 为每机记录 `dispatchStarted` 与单调 `dispatchEpoch`；错峰事务在真正调用 Vehicle/MissionManager 前只保持 pending，ACK 和 Land/RTL/Mission 模式遥测在发送标记之前不能完成事务。Land/RTL 的同机 safety escape owner 唯一，重复安全逃生请求被拒绝，ACK 不再广播给多个 safety transaction。
- 临时 Mission 替换复核：`MissionManager::removeAllComplete` 不带任务代号，替换请求会先进入旧任务清理并锁定车辆；清理完成前新事务拒绝创建，避免旧回调覆盖新 `_temporaryMissions` 状态。
- 临时 Mission 共享信号复核：`sendComplete/removeAllComplete` 只在本地 upload/clear pending 状态和结果航线证据匹配时消费；Clearing 阶段收到无 token 的外部 `sendComplete` 不再触发 `removeAll()`，非本控制器的 `removeAllComplete` 被忽略并保留现场。上传失败或结果无法归属时会保留 `staleTemporaryMissionIds` 栅栏，使后续临时任务/替换先进入显式清理流程，但不会在无法归属的回调中暂停或删除可能属于其他 MissionController 的航线。该护栏不能凭空给底层 bool 信号增加跨模块 operation token，仍需工程师注入并发 MissionController 场景验证。
- 指挥中心参数卡复核：`CommandCenterOverlay.applyControlCommand()` 只调用 `GuidedActionsController.requestControlCommand()`，不再直接访问 `Vehicle` 的高度/速度/起飞速度 API；控制器为 `{vehicleId, kind, value, requestId}` 生成冻结快照，请求与确认执行前均进行链路/健康/飞行状态/范围检查，并记录 `command.requested/confirmed/sent/rejected`；`sent` 仍标明为待遥测核实，不作为飞控 ACK。
- 标准 Guided 滑块复核：`actionChangeAlt/actionChangeSpeed` 在确认时保存 `_actionTargetVehicleId`，执行时重新解析车辆并复核链路、Guided/飞行模式和健康门禁；不再以当前 `activeVehicle` 作为无条件目标，滑块数值不是有效数字时拒绝。
- 参数动作类型复核：手动参数执行辅助函数再次核对 `actionCode` 与冻结快照的 `kind`，标准 Guided 辅助函数只接受高度/速度两种动作码；动作码、参数类型或确认对象不一致时记录拒绝，不进入 `Vehicle` API。
- 标准任务启动复核：`actionStartMission/actionContinueMission/actionMVStartMission` 当前只把冻结目标列表交给 `SwarmController.executeStartMission()`；C++ 重新检查连接、任务缓存、健康证据和模式，事务等待 `MAV_CMD_MISSION_START` ACK/`missionFlightMode` 遥测，QML 不再直接调用 `Vehicle.startMission()`。
- 任务快照证据复核：标准启动入口逐机重新读取 `MissionManager` 任务项数量/当前索引，并按 `FirmwarePlugin::sendHomePositionToVehicle()` 转换到 QML 任务序号（同时保留 `rawCurrentIndex`）；创建事务和延迟发送前都比较数量/转换后索引/指纹，窗口内变化时不调用 `startMission()`；结果及成功事务 `vehicleResults[].details.missionEvidence` 输出 MissionItem JSON 的 SHA-256 `snapshotFingerprint`。它仍不是外部任务版本服务的跨站锁定。
- 室内位置证据复核：FleetRegistry 不把缺失全球坐标伪装成故障；若 `Vehicle.localPosition` 已收到有限的 x/y/z 遥测，则记录 `positionEvidenceSource=local_position` 并允许健康证据继续流转。该证据不等价于全球 Mission 坐标可用，需由 PX4 任务检查和工程师实机验证确认。
- 本地位置新鲜度边界：当前 `FactGroup::telemetryAvailable` 只证明曾收到该组遥测，QGC 基线没有独立的本地位置样本时间戳；不能把它宣传为新鲜度证明。若产品需要严格时间窗，工程师必须接入带时间戳的 MAVLink/Vehicle 适配层并在此门禁中增加年龄检查。
- 临时 Mission 启动阶段复核：`MissionManager::sendComplete` 后状态为 `Starting` 而非 `Executing`；只有启动 ACK 或 Mission 模式遥测才进入执行阶段，启动超时/拒绝会标记失败并清理，避免上传成功被误报为任务已启动。
- 临时 Mission 启动归属复核：上传 dispatch 与后续 `MAV_CMD_MISSION_START` 记录分离的逐机标记/epoch；上传期或外部调用产生的 ACK 不得终结启动阶段事务，启动前失去所有权会进入清理。
- 健康门禁分层复核：QML `FleetHealthCenter` 对起飞/任务入口采取未知即复核或阻断；C++ `SwarmController::_healthGateBlocks()` 对直接 Goto/起飞保留“仅确认不安全才阻断”的兼容策略，避免把室内或部分解码车辆静默当成安全/不可用。标准 Mission start 使用更严格的 `_missionStartHealthGateBlocks()`；这不是权限隔离，工程师需在真实机型和定位源上确认最终策略。
- 任务启动竞态复核：标准 Mission 和临时 Mission 的延迟/上传完成回调都重新调用 `_vehicleMissionStartReady()`；同一车辆的普通任务/飞行事务在 `_createCommandTransaction()` 建立逐机互斥，RTL/Land/formation ABORT 为明确的安全逃生例外。取消事务仍不逆转已发送给飞控的动作。
- 恢复任务旁路复核：`resumeMission()` 只负责生成并上传恢复任务；`resumeMissionReady` 之后由 QML 冻结目标、复核健康和机载任务快照，再调用 `SwarmController.executeStartMission()`，不再在恢复路径直接启动任务。
- 能力/健康证据复核：`_refreshFleetRegistry()` 记录固件/UID/电机布局/传感器位图和 camera/obstacle/range 能力，并合并 estimator/SYS_STATUS/ESC/FTC 当前证据；证据只形成 observed configuration，不伪造 trusted baseline。FTC/FactGroup 的历史样本新鲜度仍需工程师用带时间戳适配层验证。
- 故障证据 known/freshness 复核：`VehicleCapability`、`FleetRegistry`、`FaultToleranceManager` 和 `FleetHealthCenter` 现在显式传递 `faultHealthKnown` 与 `faultEvidenceAgeMs`。无当前故障证据时会清除旧 `hasFault/faultSummary`，但 `hasFault=false` 不再单独证明无故障；健康评分、Leader 候选和严格任务门禁要求 known 且 age 非负并在离线超时内。`HealthAndArmingCheckReport` 现在按本地 `updateSequence` 观察并在 metadata 中记录观察时刻/年龄，但 QGC 基线仍未提供源 MAVLink 样本时间戳；该年龄不是飞控消息新鲜度证明，工程师仍需接入带时间戳适配层后再放行。
- 故障证据配对/窗口复核：`FaultToleranceManager` 部分更新若设置 `faultHealthKnown=true`，必须同时带非负 `faultEvidenceAgeMs`；非负年龄缺少 known 或 known=false 也会拒绝，清除 known 且不带年龄会清除旧年龄。`FleetHealthCenter` 读取 `FaultToleranceManager.offlineTimeoutMs` 作为统一新鲜度窗口，修改策略超时后不会因聚合层仍硬编码 3000 ms 而产生不一致健康状态。
- QML 预演/审阅边界复核：审阅协调器、风险雷达、能力匹配、时间线、任务模拟器和数字孪生均通过只读 facade 暴露；原始 `review/assess/plan/import/inject/clear/setSnapshot` 等写入口不再由公共控制器属性泄露。
- 连续性候选门禁复核：`FleetMissionContinuityPlanner` 现在要求 estimator、position、fault evidence 以及 `faultEvidenceAgeMs` 都明确且新鲜；未知健康证据生成 `health_unknown` 建议，并不能被当作 Leader/替补/中途加入候选。
- 任务风险输入复核：`FleetMissionRiskEvaluator` 对航点/覆盖计数和 `reserveCount` 只接受有限非负整数，对 `reserveVehicleIds` 要求列表；空备用列表不再绕过 `reserve_shortfall`。风险信号的显式严重级别和权重也必须可识别且在约定范围内，类型错误生成阻断输入风险，不会静默按默认值放行；覆盖计数关系错误即使同时带有表面上有效的覆盖率，也会保持 `coverageEvidenceAvailable=false`、覆盖率为 `-1` 并阻断评估。
- 任务编译围栏约束复核：`FleetMissionCompiler` 的 `constraints.geofenceRequired` 现在只接受实际 QVariant `bool`；字符串/数值类型错误生成 `invalid_geofence_required` 阻断风险并使 `feasible=false`，不会由 `QVariant::toBool()` 隐式改变地理围栏要求。
- QML 写入边界复核：数字孪生五类快照属性与时间线 `events` 改为只读；时间线只能使用 `replaceEvents()` 或严格 JSON 导入，未知根字段拒绝，避免外部 QML 混写审计时间线。
- 角色策略暴露复核：`GuidedActionsController.rolePolicy` 现在只提供用户/当前角色/定义的值快照，不再把 `FleetRolePolicy` 可变 QObject 交给普通 QML；权限定义和身份切换仍需后端认证接入，当前策略本身不是 C++ 权限隔离。
- QML 控制/健康源暴露复核：`GuidedActionsController` 不再把可替换 source 的 `FleetHealthCenter`、可写超时的 `FleetRegistry` 或带 WRITE/merge 入口的 `FormationPlanner` raw QObject 暴露给普通 QML；公开属性改为只读查询 facade。地图仅通过窄 `swarmController` facade 使用临时航点查询/下发，原始 `SwarmController` 的其它命令入口仍留在控制器内部。健康 facade 的 `requireLeader` 非布尔输入会阻断而不会被当成 false。恢复任务完成面板改走 `confirmAction(actionResumeMission)`，不再直接调用无冻结目标的 `executeAction(..., null, ...)`。
- Guided 动作确认旁路复核：`confirmAction()` 在动作专属校验完成后签发一次性 `confirmationToken`，并把冻结 `actionData`/动作码写入 token；期望 token 保存在控制器与通用 `GuidedActionConfirm` 的 lexical child-object 状态中，不作为公开 property 提供读取，只由确认手势原样回传给 `executeAction()`。执行入口在任何 Vehicle/SwarmController 调用前同时核对 token 身份、动作码和冻结对象，成功后立即消费；缺失/复制/过期/重复回调只记录 `command.rejected`。RTL/Land/急停仍可绕过 FleetHealth 的普通任务门禁作为安全逃生，但不绕过人工确认。该 QML token 仍不是不可信插件的 C++ 权限边界。
- 意图审阅新鲜度复核：`GuidedActionsController` 对审批使用的 FleetHealthCenter 快照生成稳定指纹，排除 `lastSeenUtc`/连续增长的证据年龄，只有在线、健康/故障证据、评分或状态变化才使 token 失效；调用方提供的外部 `healthReports` 不替代当前快照，不一致会令 `FleetMissionReviewCoordinator` 保持 `needsReview=true`。仍需工程师验证实际健康字段是否覆盖产品所需的全部风险维度。
- 编队停止语义复核：`SwarmController::endFormationSession()` 将“操作员请求停止”传入 ABORT 状态机，不在 ABORT 启动瞬间冒充 `formationFault`；只有后续 ABORT 拒绝/超时等具体回滚失败才报告故障。QML 任务可记录为取消，工程师仍需验证异步 ACK/超时和旧回调。
- QML 模块边界复核：可变命令/任务/健康 QObject 已注册到 `MerivusInternal 1.0`，`GuidedActionsController` 显式导入；公开 `Merivus 1.0` 面板未再获得这些类型的直接注册。该 URI 分层只能减少误用和旁路，不能替代 C++ 权限、签名或进程隔离。
- 编排器输入复核：`SwarmMissionOrchestrator` 只保存 1--255 的有限整数成员 ID；创建/重分配遇到字符串、布尔、小数、越界、非有限或重复成员时整体拒绝，不把合法子集静默留下；通用重分配遇到非法或冲突的显式 Leader 元数据也拒绝，不自动换成首个成员。`FleetRegistry` 的独立 `configurationClear=true` 更新会清除旧 configuration map、指纹和 metadata 别名，避免改装证据残留。
- 连续性建议身份复核：建议 ID 现在绑定任务 revision 与稳定 Fleet 快照 SHA-256（不含心跳时间戳/连续年龄），规划器刷新后若成员健康、链路、电量、能力或构型证据改变，旧 UI 建议不会被同 ID 重新解析为新建议。

## 已收敛的源码边界

- 事务、编队阶段、临时 Mission、交接和连续性建议均保留事务/版本门禁；新增 `cancelTransaction()` 只取消地面站事务并触发已有清理路径，不声称逆转已发出的机载动作。
- 能力、角色、任务编译、能量、空域、风险、链路和报告模块对关键 ID、布尔、有限数值、时间戳、重复成员和 JSON schema 做保守拒绝；坏输入不会被当作健康或可执行。
- 扩展目录、模板、证据链、数字孪生、运营记忆、黑匣子和时间轴提供本地原子文件交接；交接数据不会加载任意二进制、恢复控制会话或释放飞行命令。
- QML 面板只展示状态和建议；数字孪生面板的“预演”按钮只运行本地数据预演，不接入任务或飞行命令。结果持续带 `manualConfirmationRequired`、`recommendationOnly` 或 `flightCommandReleased=false` 等边界字段。
- `SwarmController` 的命令级 health preflight 对起飞、Goto 和临时 Mission 阻断已确认的离线/严重/故障/临界电量/极差链路；批量降落和 RTL 保留安全逃生路径，FleetRegistry advisory block 只在 `healthGate` 中作为 warning 暴露，实时 Vehicle/PX4 前置条件仍然有效。
- 指挥中心显式选中的单架 UAV 也沿用 `SwarmController` 的 Land/RTL/Takeoff 事务路径；不会因为只有一个选中目标而退回 `activeVehicle` 直控，确认文案与冻结目标保持一致。未提供显式选择时才保留原有 active-vehicle Guided 路径。
- Land/RTL 的安全逃生例外不会吞掉健康告警：批量结果现在在成功下发时仍显示 `healthGate.warningVehicleIds`，区分“可达的逃生指令”与“FleetRegistry 仍有风险证据”。
- `FleetMissionReviewCoordinator` 将角色分配、任务编译、能量、空域、风险和报告阶段串成单次只读审阅；`GuidedActionsController.prepareFleetIntent()` 只记录/返回该审阅结果，不连接 `SwarmController`。缺失证据保持 `needsReview`，并固定 `manualConfirmationRequired=true`、`flightCommandReleased=false`、`missionMutated=false`。
- 意图健康证据复核：`prepareFleetIntent()` 缺省使用 `FleetHealthCenter.vehicleHealth`，不会把空健康报告伪装成通过；调用方显式传入空数组仍按缺失证据处理。
- 能力匹配状态复核：`prepareFleetIntent()` 用预览同源输入刷新 `GuidedActionsController.capabilityMatcher.lastPlan`，避免 QML 能力面板读取旧计划；该调用没有任务/命令副作用。
- 共享服务接线复核：`GuidedActionsController` 实例化这些服务，但对普通 QML 只暴露 `missionEvidenceChain`、`maintenancePassport`、`linkDegradationPolicy` 的值快照；原始 QObject 写入口只留在控制器/后端内部，不连接任何命令释放入口。
- 报告事件证据复核：`prepareFleetIntent()` 将当前黑匣子事件作为审阅 options 快照，协调器转交报告构建器生成 timeline/incidents；事件仅进入报告值对象，不会反向写任务或命令。
- 报告展示复核：`missionReport` 只读暴露最近一次报告的 verdict、异常数和未验证项；指挥中心不提供批准、上传或命令释放操作。
- 链路降级接线复核：控制器只读评估 Registry 在线/链路质量快照；缺失带宽/延迟/丢包保持 unknown，指挥中心只显示统计，不修改传输、任务或编队。
- 维护护照接线复核：`recordsChanged` 只更新只读 `assessAll()` 结果；普通 QML 只得到记录/评估快照，append/import/load/clear 仍由受控维护适配器调用；空记录显示未知/证据不足，维护评估不进入 FleetHealthCenter 或命令门禁。
- 交接操作复核：`FleetHandoffPanel` 通过 `GuidedActionsController` 的身份绑定 facade 操作，身份唯一来自 `FleetRolePolicy.currentUserId`，不再使用可编辑/持久化的 QML Settings；facade 固定 `requiresSecondApproval=true`、300 秒过期并校验当前身份/版本，创建/接受/拒绝/撤销只修改 `MissionHandoffManager` 审计记录，不调用任务、Vehicle、MAVLink 或命令入口。
- 交接后端边界复核：`MissionHandoffManager::requestHandoff()` 即使被非 facade 调用，也拒绝关闭二次确认或超出 1–86400 秒的有效期；固定 facade 的 300 秒策略因此有后端上限兜底，交接仍只产生审计记录。
- 交接记录边界复核：`MissionHandoffManager` 后端限制任务/操作者/原因/备注文本和 metadata 的递归深度、节点数、键长及累计字节；解析动作也拒绝超长操作者/备注，接受/拒绝/撤销必须携带非零 `recordRevision`，只有内部过期路径可无 token 结算。导入要求真实 `requiresSecondApproval=true`、正且不超过 24 小时的有效期，以及 `resolvedAt` 不早于 `createdAt`，完整校验后才原子替换记录集合，仍不恢复控制会话或释放命令。
- 交接布局复核：无待办时面板默认收起为紧凑行，操作员点击“发起”才展开表单；有待办时自动展开。待办列表仍只显示前两条，完整记录以管理器快照为准。
- 编队同步发送复核：`SwarmController::_sendFormationCommand()` 在每架机下发前检查主链路已连接且同组件/同命令没有 pending；Vehicle 可能同步发出本地失败/终态 ACK 时，phase 推进或 ABORT 会延迟到当前发送循环结束，避免回滚重入后外层继续发送旧阶段命令；同步成功/拒绝仍保留在当前阶段的 pending/failed 集合中。
- 能力 descriptor 资源复核：`FleetCapabilityDiscovery` 与 `FleetRegistry` 对能力列表和递归 descriptor/metadata 采用同一数量、深度、节点、键长和累计字节上限，并拒绝自定义 QVariant 类型；发现构建和 Registry register/update/heartbeat 会在构型指纹计算、metadata 合并前拒绝超限输入，避免未受控的能力证据进入匹配/连续性规划。
- 能力匹配资源复核：`FleetCapabilityMatcher` 对 task/vehicle 能力列表执行 128 项/128 字符上限，并将 Fleet 候选数限制为 2048；超限 Fleet 在遍历前留下 `fleet_resource_bounds` 拒绝证据、`parametersValid=false`，不会继续排序或生成候选分配。
- 资源门禁实际控制流复核：Discovery 的超限输入/验证现在立即返回有界的 invalid 诊断，多来源构型在合并后、hash 前再检查；已有 discoveryErrors 不会被最终 partial/complete 状态覆盖。Registry 暂存完整合并、验证累计预算后提交，防止小更新累积越界；Matcher 任务参数无效即不进入候选循环。null 节点与未 trim 文本也计入限制。
- 任务记录资源复核：`SwarmMissionOrchestrator` 在创建/状态写入前限制类型、原因、metadata 和记录数量；错误请求不新增任务、不更新时间/revision。限制仅是本地资源预算，不宣称支持 10000 个并行飞行任务。
- 任务占用复核：编排器的启动/恢复/活动重分配在状态写入前拒绝其它 running/paused 任务的成员冲突；Draft 不占用，终态释放；查询仅返回本地冲突诊断。编队 facade 先预留再发命令，并提前绑定 task ID 接收同步失败；连续性应用重审全部 taskSummaries，避免丢失其它任务占用上下文。跨站锁、协议级授权不在本补丁内。
- 普通批量目标集合复核：Takeoff、Goto/临时 Mission 和标准 Mission Start 的创建前全量预检失败时不进入发送队列，Land/RTL 保留安全逃生部分执行。错峰发送期间只有逐机复核/失败/ACK/超时，不是多机原子提交；已发动作不能因另一成员后来失败而被地面站追溯撤销。
- 连续性占用复核：完整任务集合构建 running/paused/pending 的占用索引，别的活动任务成员不能作本任务替补；集合身份/成员冲突整体阻断。建议 ID 绑定稳定任务占用指纹，单任务查看缺少占用证据会显式标记，控制器应用前要求完整占用证据并重审全集合，后端编排器再拒绝抢占。
- Goto 抢占复核：完整预检/事务冲突检查后才清理旧临时任务，Goto 先预留继任事务再取消旧路；旧 Pause 的 DO_REPOSITION pending 清空前不设新 dispatch 标记，回调复核 deadline/健康/链路/所有权。Clearing 前置处理避免重复 Pause；同步 ABORT 后 phase 已改变则不覆盖终态结果。
- 维护温度复核：`FleetMaintenancePassport` 的温度归一化和 `metric()` 使用 -100 至 300 摄氏度范围，缺失哨兵低于该范围；负温度不会被当作 `temperature_missing`。
- AI/本地意图接线复核：`MerivusAIAssistantPanel` 只有在操作员显式点击“生成舰队审阅草稿”后才调用 `prepareFleetIntent()`；AI proposal 和本地规则模式待确认建议都走该只读入口，不调用申请审批、批准、上传或任何飞行命令入口。
- 模板约束接线复核：显式提供 `templateId` 时，`prepareFleetIntent()` 会调用 `FleetTaskTemplateRegistry::validateTask()`；结果进入 compiler identity/审阅 plan，模板缺失或无效追加阻断风险并保持 `needsReview`，未提供模板时不做隐式推断。
- 模板唯一来源复核：`industrial_inspection` 仅由 C++ 构造函数注册，QML 初始化不再以同 ID 覆盖，避免校验约束与默认值在启动后悄悄改变。
- 审批一致性复核：`requestFleetIntentApproval()`/`approveFleetIntent()` 会核对最近一次审阅的 `taskId`、`planId`、`reviewStatus` 和 `needsReview`；审阅缺失、过期或仍需复核时只记录 `intent.approval_blocked`，不改变意图状态、不释放命令。
- 审批新鲜度复核：`FleetIntentTask.intentChanged`、`parametersChanged` 或重新进入 `preview` 会清除控制器缓存的审阅身份，防止外部直接 `preparePreview()` 后复用旧协调器结果。
- 协调器重审复核：`GuidedActionsController` 监听 `FleetMissionReviewCoordinator.reviewChanged`；外部重审即使保持同一 `planId` 也会清除审批身份，避免健康/空域/风险输入变化后误用旧审阅。
- 原始意图对象暴露复核：控制器不再公开 `FleetIntentTask` 实例，只暴露状态/ID；普通 QML 无法直接调用 `requestApproval()`/`approve()`，必须经过带审阅 token 的控制器包装。
- 单机 Goto 门禁复核：地图确认/执行路径冻结活动机 ID，确认与执行两侧均用同一 ID 调用 `_commandGate()`，并统一调用 `SwarmController.executeGoto()`；不再直接绕过 FleetHealthCenter 调用 `guidedModeGotoLocation()`，健康/链路变化会在实际下发前再次检查。
- 右键 Goto 旁路复核：`FlyViewMap.executeRightClickFly()` 不再直接执行 `SwarmController.executeGoto()`；单机或框选目标均通过 `GuidedActionsController.confirmAction(actionGoto, ...)`，由控制器冻结目标集合与坐标、复核 FleetHealthCenter 并走事务执行，普通右键不会成为无确认的第二条指令路径。
- 指挥中心参数卡旁路复核：`CommandCenterOverlay.applyControlCommand()` 不再直接调用 `Vehicle.guidedModeChange*()` 或 `setGuidedTakeoffSpeed()`；高度/速度/起飞速度当前面板统一经 `GuidedActionsController.requestControlCommand()`。控制器冻结 `{vehicleId, kind, value, requestId}`，确认前后重查目标、健康、链路、飞行模式和范围，并记录 `command.requested/confirmed/sent/rejected`。`sent` 不代表 MAVLink/参数 ACK；待工程师用遥测回读补齐。该约束是当前 QML 面板协定，不代替 C++ 权限隔离。
- 取消回收复核：`GuidedActionConfirm.confirmCancelled()` 只在弹窗/地图指示器真正活跃时通知 `GuidedActionsController.confirmationCancelled()`；控制器清除手动参数快照、`_actionData` 和目标 ID，下一个请求在设置新快照前先回收旧弹窗，防止取消后重放。
- `MPC_TKO_SPEED` 回读复核：本地 setter 接受后加入最多 5 秒的待确认列表，控制器周期读取 `guidedTakeoffSpeed()`；值匹配记录 `command.acknowledged`，超时记录 `command.ack_timeout`。这是静态接线检查，仍需工程师用真实参数回读和 ACK 时序验证。
- 临时航点队列旁路复核：`executeBatchQueuedFly()` 必须消费 `GuidedActionConfirm` adapter 设置的一次性 `_shiftMissionApproved`，未确认或已消费时不下发；取消/草稿清理会清除令牌，确认后的最终门禁仍在调用 `executeQueuedGoto()` 前复核。
- 连续性回调复核：`FaultToleranceManager` 的掉线/低电量信号现在只追加 `continuity.review_required` 事件并刷新 `FleetMissionContinuityPlanner`，不直接调用 `pauseTask()`/`reassignTask()`；任务变更必须由人工审阅入口携带来源 `taskRevision` 后执行。
- 连续性写入口复核：`GuidedActionsController` 不再公开原始 `SwarmMissionOrchestrator` 或规划器的 `plan()/clear()` 写接口；普通 QML 只能读取任务/建议快照。唯一的控制器接缝 `applyContinuitySuggestion(suggestionId, true)` 现在还要求 `FleetRolePolicy.currentUserId/currentRole` 和 `task.continuity.apply` 权限，再按 ID 取回建议、用当前 Registry 快照重新规划，拒绝伪造载荷、非替补/重分配动作、缺失人工确认、无效/过期 `sourceTaskRevision` 或不再健康的候选；规划器输出的 `proposedAssignments` 含保留成员和替补的完整集合，通过后才调用 `reassignTaskAtRevision()`；成功/拒绝事件带操作者身份，面板没有自动调用该入口，事件记录仍标记 `flightCommandReleased=false`。
- Fleet OS 目录展示复核：`FleetCatalogPanel` 使用 `taskTemplateCatalog/extensionCatalog/autonomyPolicySnapshot` 值快照，不接收可变 registry QObject；面板仅显示声明和未验证状态，不提供注册、导入、启用、删除、插件加载或命令入口。
- FleetRegistry 写边界复核：`registerVehicle()`、`updateVehicle()`、`markSeen()`、能力护照导入/批准和 `clear()` 已移除 `Q_INVOKABLE`，仍可由 C++ `SwarmController`/后端调用，但普通 QML 只能读取车辆/健康/能力快照，不能直接伪造或清空实时机队。
- 故障策略写边界复核：`GuidedActionsController` 不再向普通 QML 暴露可调用 `updateVehicle()/reportBattery()/clear()` 的 `FaultToleranceManager` 原对象，只提供 `vehicleReports`、`recommendations`、`fleetSummary` 值快照；健康中心和命令门禁继续使用控制器内部实例，外部界面不能注入健康状态。
- 模板/扩展目录写边界复核：`FleetTaskTemplateRegistry` 与 `FleetExtensionRegistry` 的注册、启用、删除、导入/加载入口已移除 `Q_INVOKABLE`；默认目录在 C++ 构造阶段建立，普通 QML 只能读取目录快照和执行只读任务校验。
- 证据链生命周期复核：意图预览/审批/批准/拒绝会追加带人工审批边界的摘要记录；普通 QML 只读 `records/count/headHash` 快照，append/import/load 仅由控制器内部入口调用；证据链只写地面站本地对象，追加结果不参与命令门禁或命令发送。
- 审计持久化输入复核：证据链追加与导入均拒绝超过 128 字符的类型、超过 16 层/50000 节点或 512 KiB 的单条 payload；运营记忆和维护护照导入均拒绝未声明根字段，避免未知字段被旧版本静默丢弃。证据链仍仅是未签名 SHA-256 链，不能被当作数字签名或不可抵赖凭据。
- 事件证据复核：事务、编队异常、舰队降级和任务状态事件以原始快照追加到证据链；该回调无任务/命令写入，证据链拒绝或达到上限时不会反向阻塞事件源。
- 证据链 UI 复核：指挥中心只显示记录数和头哈希前缀，不把 `count/headHash` 当成签名或放行证明。
- 事件规则状态复核：`FleetEventRuleEngine` 的冷却窗口、`previousEventType` 和运行时状态只抑制/解释建议；普通 QML 只读规则/启用状态快照，注册、启用、导入、删除和清理仍由控制器内部初始化/回放流程执行；规则输入字段严格校验，`resetRuntimeState()` 可清理回放状态，结果仍固定不释放命令。
- 事件规则持久化复核：规则快照限制 32 MiB、严格校验根字段/版本/重复 ID/布尔值和 `flightCommandReleased=false`，文件使用 `QSaveFile` 原子提交；导入后运行时状态清空，不会伪造旧事件序列或释放命令。
- 能力护照审批复核：增量护照导入合并而不是覆盖旧审批；带 `local_approval` 的车辆必须同时存在同 ID、同指纹的本地审批记录，切换/清除基线会移除旧审批，不匹配时整份导入原子拒绝。
- 能力发现来源复核：`FleetCapabilityDiscovery` 对 heartbeat、AUTOPILOT_VERSION 和组件元数据的所有身份别名做一致性检查；跨来源冲突返回 `discoveryState=invalid` 且不保留可注册身份，静态能力优先高可信来源，动态健康保持心跳优先。
- 能力匹配身份复核：`FleetCapabilityMatcher` 现在对每架输入逐一检查 `systemId`、`vehicleId`、`id`，只接受真实有限整数且别名一致；数字字符串、分数和别名冲突会逐机拒绝，不再选择第一个别名。
- 角色分配身份/计数复核：`FleetRoleAllocator` 现在逐一检查 `systemId`、`vehicleId`、`id`，拒绝数字字符串、空值、非有限/非整数 ID 和冲突别名；`leaderRequired` 与角色计数只接受真实布尔或有限非负整数。任务参数一旦非法，规划保留诊断和拒绝项但不生成 assignments/reserve，避免 Qt 隐式 `toBool()`/`toInt()` 产生看似可用的编组结果。
- 构型证据合并复核：`metadata.configuration` 与顶层 configuration 使用同一状态机；配置别名更新会刷新 observed fingerprint，hash-only 变化会清理旧配置图，避免导出护照出现 map/fingerprint 不一致。
- 事件运行时接线复核：`FleetEventBlackBox.eventRecorded` 只更新 `GuidedActionsController.eventRuleSuggestions` UI 状态；清空空日志会清除旧建议，未发现进入事务、任务、Vehicle 或 MAVLink 写入口的路径。
- 时间轴接线复核：`FleetEventBlackBox.events` 现在同步到 `FleetTimelineReplay.events`；回放 `eventReady` 仅复用规则评估，导入非空日志会重建运行态，未增加命令或任务写入口。播放期间新日志事件不会重置回放位置。
- 运营记忆布尔证据复核：`FleetOperationalMemory` 的热点、异常、人工干预和估算标记只接受真实 JSON/QVariant `bool`；非空字符串不会通过 `toBool()` 被当作 true，避免损坏或伪造的交接 payload 改变运营指标。该模块仍是只读观察层，不构成签名证据或任务安全证明。
- 空域输入布尔证据复核：`FleetAirspaceConflictChecker` 的全局和任务级 `returnRequired` 只接受真实 QVariant `bool`；字符串布尔值不会改变返航通道判断，类型错误会进入 blocking 违规并要求人工修正。
- 风险雷达输入复核：`FleetRiskRadar` 对任务 `intent/parameters`、布尔门禁、百分比/备用机计数和风险信号 Map/weight/reason 做严格类型与范围校验；字符串布尔/数值或越界输入会返回 `inputValid=false`、`needsReview=true` 并将评估提升为阻断分数，不再被隐式转换后静默计算。
- 意图批准风险证据复核：`FleetIntentTask::approve()` 只在预览 `feasible` 为真实 `bool=true` 且嵌入的 `riskAssessment.inputValid` 缺失或为真实 `true` 时继续；显式 `false`、字符串或其它类型的可行性/风险输入不会被审批流程忽略。
- 模板校验输入复核：`FleetTaskTemplateRegistry::validateTask()` 对 `allowUnverified`、车辆 `online/canUse*` 布尔值、`systemId/vehicleId/id` 别名和任务时长执行严格类型/范围校验；字符串布尔/数值、冲突 ID 或非法时长进入拒绝原因，不会被 `toBool()`/`toDouble()` 静默转换。
- 事件规则条件复核：`FleetEventRuleEngine` 对条件路径、条件数量/列表长度/文本长度、真实数值与 `exists` 布尔期望值执行严格校验；字符串数字/布尔不会改变规则匹配，条件错误会在注册/导入时拒绝，规则输出仍不释放命令。
- 能力可信度复核：`FleetCapabilityDiscovery` 只接受 String/StringList/List<String> 能力标签，非 Map 构型和非法 `discoveredAtUtc` 会进入发现错误；heartbeat 能力保留为 observed-unverified，静态协议/受信清单才进入 `verifiedCapabilities`，`FleetCapabilityMatcher.requireVerifiedCapabilities=true` 时不会选用低信任标签。
- 黑匣子 payload 边界复核：`FleetEventBlackBox::recordAt()` 与 `importJson()` 共用 type/source 长度、payload 递归深度（16）、节点数（50000）和单条 payload 大小（512 KiB）限制；超限数据不会进入内存日志，导入失败保持原日志不变。整份快照仍受 32 MiB 限制；该模块仍是未签名本地审计记录。
- 恢复任务索引复核：恢复确认阶段保存在 lexical child-object 状态的 `resumeMissionIndex` 跨越异步 Mission 上传回调继续使用；`_startResumedMission()` 不再读取可能尚未更新的 `currentMissionIndex`，并在两个阶段拒绝非法/越界索引，阻止错误航点启动。
- 标准任务快照复核：确认后由 `missionStartSnapshot()` 形成逐机 SHA-256 指纹；`executeStartMission()` 在预检和延迟发送两个边界重新核对锁定指纹，MissionManager 任务发生替换/清空时不触达 `Vehicle::startMission()`。
- 连续性复核：终态任务不再进入变更建议分支；显式 Leader 不在成员集合会进入 `leader_not_assigned`；任务级可信能力/构型基线/中途加入策略会影响候选；`replace_leader` 应用通过 `reassignTaskAtRevision(..., leaderVehicleId)` 同步更新 `leaderVehicleId/leaderId`，fresh/stale 离散桶在跨阈值时使旧建议身份失效。
- 多机 Mission 启动复核：`actionMVStartMission` 不再把 active vehicle 的任务项数量/索引套用到所有目标，确认时逐机锁定的指纹仍在预检和延迟发送前复核；单机路径继续保留数量/索引门禁。
- 连续性面板复核：面板仍为只读，但按 critical/high/medium 等严重度排序展示最多六条，显示原因、任务 revision、Fleet 指纹前缀、受影响/候选 ID，并在超出显示上限时提示未展开数量；没有新增编排、任务或飞行命令入口。
- 连续性候选排序复核：planner 在过滤后按构型/能力可信、电量、链路和稳定 ID 排序，建议 ID 仍绑定 Fleet fingerprint；排序不会触发任务或飞行写入。
- `CommandTransaction` 静态复核：未进入第一次 dispatch 的空命令/空目标事务不能通过 `canRetry()`；运行中 timeout 变化会重启 timer，已排队的旧 timeout 回调会先重读当前 deadline；摘要显式含 `terminal`，仍需 Qt/Mock 验证边界时序。
- `FleetHealthCenter` 静态复核：健康状态、在线/故障门禁、电量/链路阈值和健康分数均经过严格 QVariant 类型读取；字符串布尔/数值与非法故障年龄不会在聚合层被隐式接受，仍需工程师验证实际 Registry/FaultTolerance 合并时序。
- `FleetHealthCenter` 状态合并复核：来源 `healthState=healthy` 不能覆盖证据推导的 unknown/critical/offline；来源 critical/degraded/offline 只会保守升级，原始声明单独保留，仍需 Mock 验证合并时序。
- `FleetMissionCompiler` 静态复核：车辆 ID 解析不再调用字符串 `toInt()` 或保留文本别名，并逐字段核对 `vehicleId/systemId/id`；只有真实数字类型、整数、1–255 且别名一致的值能进入编译成员和备用列表。
- `FleetHealthCenter` 门禁静态复核：`_evaluateGate()` 对每个输入项设置完整性标志；任一非法 ID 都追加 `invalid_vehicle_id`、`invalidSelection=true`，即使其他 ID 健康也不会允许部分集合下发；无安全 Leader 时 `globalBlocking` 仍阻断最终 `allowed`。
- `FleetMissionCompiler` membership 静态复核：`fleetProvided` 与 `fleetIds` 分离，非空但全非法的 fleet 不会退化成“未提供”；assignment/reserve 不在闭合 fleet 中会追加 blocking unmet/risk，planning-only 仅在 fleet 为空时成立。
- `FleetMissionCompiler` reserve 静态复核：`idList()` 通过 invalid count 保留非法输入证据；非法/混合 reserve 列表会阻断，而不是过滤成空列表后继续。
- `FleetMissionCompiler` 围栏静态复核：`geofenceRequired=true` 不再使用 `task.contains("geofence")` 作为存在性证明，实际检查 area vertexCount、bounds 或 geofence polygon 几何。
- `FleetMissionCompiler` 与 `FleetAirspaceConflictChecker` 的边界语义已对齐：地理 `west/east/south/north` 必须有限、在经纬度范围内且正向有序；本地 `minX/maxX/minY/maxY` 必须有限且正向有序；无坐标系/轴顺序声明的四数字 bounds 列表不再作为围栏证据。
- `FleetMissionCompiler` role 静态复核：assignment role 不再通过 `toString()` 接受任意 Variant；真实字符串必须规范化到四个受支持角色，否则进入 blocking unmet/risk。
- `FleetHealthCenter` 数值范围静态复核：battery/link 在严格 finite 检查后再次限制 `-1..100`；越界值进入 unknown 或 `health_input_range_invalid` blocking。
- `FleetMissionCompiler` 闭合静态复核：重复 fleet ID、role 别名冲突、多个/不匹配 Leader 和无效 geofence 顶点均会形成 unmet/risk；areaSummary 的 `boundaryValid` 不再只由 `vertexCount` 计算。

## 仍需工程师完成

- 故障证据新鲜度静态复核：Registry/FTC 为成对 `faultHealthKnown + faultEvidenceAgeMs` 保存观测锚点，并在健康刷新中推进年龄；仅心跳不会永久复用 age=0，超过 offline timeout 后不能作为健康/Leader 证据。
- FaultToleranceManager 集合状态静态复核：删除成员后重建 per-vehicle、Leader recovery 与 split 建议；Leader setter 拒绝未注册 ID，clear 不再因空报告集合提前返回。
- Registry 配置清除静态复核：`configurationClear=true` 与 configuration/fingerprint 同时出现会在 merge 前拒绝；null fault age 只表示 unknown，不经 QVariant 的隐式数值转换。
- Fault evidence 配对静态复核：`faultEvidenceAgeMs` 没有同报告的 `faultHealthKnown` 时被拒绝（包括 `-1` 清除值），避免 `known=true + age=-1` 的矛盾状态；缺时间戳的新证据按接收时刻锚定。
- Leader 替换静态复核：不健康 Leader 有安全候选时，`_rebuildRecommendations()` 先发 `degradationRecommended("reassign", [previous,candidate])` 再改 Leader 属性；UI/连续性不依赖只读属性变化来推断事件，仍不会发送飞行命令。
- 新鲜度配置静态复核：FleetHealthCenter 将 FTC 的实际 `offlineTimeoutMs` 写入每机快照，ContinuityPlanner 可读取同一窗口，避免自定义超时只在门禁层生效而在建议层误用 3000 ms。
- ContinuityPlanner 候选构建静态复核：snapshot 解析路径与 fingerprint 路径均读取 `offlineTimeoutMs`；只有缺少配置时才回退 3000 ms，避免同一任务在指纹与候选过滤中得出不同 freshness。
- QML 健康审阅静态复核：稳定 health fingerprint 纳入 `offlineTimeoutMs`，防止仅更改证据窗口而状态尚未翻转时仍复用旧批准。
- FleetHealthCenter summary 静态复核：汇总对象含实际新鲜度窗口，空机队时也不丢失策略变化证据。
- 删除/刷新静态复核：remove 当前 Leader 无候选会发 hold/no-safe-candidate；Registry refresh 将 fault age 变化纳入 changed 条件，单独 Registry 观察者可收到老化后的快照。
- Leader 安全候选静态复核：全局 recovery 不再只看 `online`，而是复用 `_isSafeLeaderCandidate()`；在线但不安全且无候选时也能稳定输出 hold/no-safe-leader-candidate。
- 离线重连静态复核：Registry/FTC 对 online 从 false→true 的 bare heartbeat 逐通道清除缺失旧证据，保留显式 fault assertion 的保守语义；不会因重新在线自动成为健康 Leader。
- 能力发现接入静态复核：`FleetCapabilityDiscovery::buildDescriptor()` 输出经过 `validateDescriptor()` 自审计；`SwarmController::_refreshFleetRegistry()` 在追加 QGC 健康字段后再次校验，错误只追加 `discoveryErrors`/`discoveryState=invalid`，实际写入仍由 FleetRegistry 原子 ingress 决定。未执行 Qt/MSVC 编译，不能据此宣称运行时验证完成。
- Guided 目标静态复核：无显式框选的 Takeoff/Land/RTL 也会冻结单元素目标并走 SwarmController 事务；普通单机 Vehicle 旁路在确认目标与 active vehicle 不一致时阻断。确认 token 同时比较数据指纹，目标数组原地修改不能复用 token。
- 事务完成静态复核：dispatch 记录基线飞行模式/时间；Mission/Land/RTL 只有 dispatch 后目标模式证据才可完成，旧模式不满足。`transactions()` 增加单调 `createdSequence` 并按创建顺序输出，QML 取消不再依赖 UUID 字典序。
- Formation/临时航点静态复核：阶段 timeout 有 phase/deadline 门禁，混合有效/非法 queued goto 坐标整笔拒绝；临时 Mission cleanup 成功还检查机载任务为空，但共享 MissionManager 回调缺少 operation token，仍需 Mock/SITL 验证并发归属。
- legacy Guided 静态边界：Arm/ForceArm/Disarm/EmergencyStop、暂停/航点序号/Orbit/ROI/Gripper/SetHome/VTOL 仍直接触达 Vehicle API；除确认 token、冻结目标和 active 切换阻断外，Vehicle 侧已补充 transport/有限输入 gate，但没有统一 ACK/超时事务、角色权限和审计闭环；Formation readiness 与普通命令 health gate 也未合并成一个 C++ 策略入口。
- 初次能力注册静态复核：`FleetRegistry::_registerVehicle()` 经过 `VehicleCapability::fromVariantMap()` 时，顶层 verified capabilities、discovery state/errors、configuration evidence sources、discovered timestamp 和 position evidence source 被保留到 metadata，`toVariantMap()` 再输出顶层兼容字段；Registry ingress 在 merge 前再次严格校验列表元素类型和发现时间戳，避免首次注册/往返路径与后续 merge 路径语义分叉。仍需 Qt/Mock 验证 round-trip 和 invalid 状态门禁。
- Verified-only 匹配静态复核：`FleetCapabilityMatcher::capabilities()` 在 `requireVerifiedCapabilities=true` 分支于读取显式 verified 标签后立即返回，不再把 `canUseGnss/canUseRtk/canUseVision/canUseRangeSensor/canRelay` 运行时观测追加到可信集合；仍需工程师验证任务编译器和能力匹配器的调用方是否正确设置该策略位。
- ContinuityPlanner verified-only 静态复核：`capabilityMatch()` 复用顶层/metadata verified 标签并在可信策略下提前返回，候选排序同样读取 verified evidence；fingerprint keys 包含 verified/discovery 字段，能力信任变化会使建议身份变化。仍需 Mock 验证旧建议失效时序。
- Matcher 能力输入静态复核：候选循环在健康/性能筛选前拒绝非 String/StringList/List<String> 的 capabilities 与 verifiedCapabilities（含 metadata fallback），并保留 `invalid_capability_evidence` 原因；仍需工程师确认 QML/JSON 到 QVariantList 的实际类型形态。
- ContinuityPlanner 能力快照静态复核：`vehicleIndex()` 对顶层 capabilities/verifiedCapabilities 及 metadata verified fallback 做同样的字符串列表门禁，坏条目使整体输入无效且不进入候选；仍需 Mock 验证 `inputValid` 与建议状态的联动。
- Matcher 任务能力静态复核：required capability 参数与车辆能力共享严格字符串列表检查；任一数字/布尔元素使 `parametersValid` 失败，防止自动文本化改变任务语义。
- RoleAllocator 可信能力静态复核：新增 `requireVerifiedCapabilities` 后，`hasCapability/hasAllCapabilities` 在可信模式不读取 `canUse*`，并校验车辆/角色能力列表类型；仍需工程师确认所有角色分配调用方显式设置策略。
- TaskTemplateRegistry 可信能力静态复核：`validateTask()` 解析模板/任务 verified-only 策略，`vehicleCapabilities()` 在可信模式只读取顶层/metadata verified，且任务/车辆列表类型错误进入 reasons/rejected；仍需工程师验证模板 constraint 与任务覆盖优先级。
- Matcher 参数失败静态复核：候选选择循环受 `parametersValid` 门禁保护；非法任务参数不会留下 selected/reserve/assignments，仍需工程师检查 QML 是否同时读取 `parametersValid` 与 `feasible`。
- 多机列表入口静态复核：`MultiVehicleList.qml` 的启动任务、单机暂停和单机 RTL 改走 `GuidedActionsController.confirmAction()`；直接解锁/接管模式按钮暂禁。显式目标启动/继续任务的确认文案显示冻结 UAV ID，不再依赖当前活动机的可见性属性。
- 批量暂停目标静态复核：`actionMVPause` 在确认阶段复制全量 vehicle ID，执行阶段只按 `_actionData` 快照解析并对缺失成员失败关闭；不会把确认之后新加入的 vehicle 纳入暂停。暂停 ACK/超时仍未进入 `CommandTransaction`。
- 命令入口审计补充：`FlightModeMenu`/`ModeIndicator`、PlanView/Vehicle 任务上传清空、`GuidedActionList`/`CustomAction`、Joystick 自定义 MAV_CMD、Mavlink Console 仍存在直接写入或自由格式命令路径；本轮已补其中的调用侧确认、目标/链路或维护态门禁，但尚未形成统一 C++ 权限、ACK/事务和角色边界。
- PlanView 写入静态复核：任务上传/清空在确认前保存 active Vehicle 对象/ID，并在延迟回调再次检查对象身份、连接状态和 `fleetHealthCenter.evaluateTaskGate()`；Mission complete 的机载任务清理也使用同一活动目标和健康门禁。该改动仍不是 ACK/事务闭环，工程师需验证 PlanMasterController 实际目标和 MAVLink 清理回执。
- Mission resume identity 静态复核：`onResumeMissionUploadFail` 不再无条件重新读取 active vehicle；只有当前 resume context 带 `requestId/vehicleId` 时才允许打开重试确认。`onResumeMissionReady` 没有活动 request 时被忽略，ready 阶段校验确认的 itemCount 与当前 MissionManager 数量一致；由于底层 signal 仍无 token，不能替代协议层 operation identity。
- Custom action 静态复核：`GuidedActionList` 按钮现在走 `confirmCustomAction()`，冻结 active Vehicle ID、重查链路/FleetHealth、显示命令标签并记录 `command.sent`（ACK 仍未观测）；直接 `CustomAction::sendTo()`、Joystick/Mavlink Console 等其它自由 MAV_CMD 入口仍需 C++ 白名单/维护模式隔离。
- 模式入口静态复核：`FlightModeMenu`、`ModeIndicator`、`FlightModeMenuIndicator` 在 MERIVUS 上下文中调用 `requestFlightModeChange()`，确认目标/链路/模式列表并对普通模式重查 FleetHealth；RTL/SmartRTL/Land 保留安全逃生例外。通用 QGC 无该控制器时仍回退直接写入，不能视作全局 C++ 权限边界。
- CustomAction C++ 策略静态复核：配置可声明 `safetyClass`；未知命令默认 `disarmed_only`，`CustomAction::sendTo()` 自身检查连接和 armed/flying 状态，控制器确认/执行再次核对等级并消费 bool 结果。该策略不是 MAV_CMD 白名单，也没有 ACK 回调；Joystick/Mavlink Console/其它直接 `sendCommand` 仍需独立隔离。
- Joystick MAV 命令静态复核：`JoystickMavCommand` 配置读取可选 `safetyClass`，未知命令和现有 `MAV_CMD_USER_1/2` 默认 `disarmed_only`；`send()` 在未完成初始连接、链路丢失、已解锁或飞行中返回失败，不再把配置动作直接提交给 Vehicle。`Joystick.cc` 记录策略阻断，但不产生 `command.sent`。
- Mavlink Console 静态复核：`MavlinkConsoleController::commandAllowed()` 与 `sendCommand()` 共同要求车辆已连接、链路未丢失、未解锁且未飞行；Console QML 的键盘回车、粘贴和发送辅助函数在调用前/失败后统一显示阻止提示。该维护通道仍不是命令白名单、角色认证或 ACK/超时闭环，工程师必须继续验证既有其它调用方。
- CommandTransaction 身份静态复核：`_vehicleKey()` 只接受真实 `Int/UInt/LongLong/ULongLong/Double` 类型、有限整数和 1–255 范围，并返回规范化十进制键；字符串、布尔、分数、越界和文本 ID 会被拒绝，事务去重与 `vehicleResult()` ACK 回填不再依赖 QVariant 的宽松 `toString()`。仍需 Qt/Mock 验证 QML 数字及 MAVLink ACK 回填的 QVariant 类型。
- Plan Upload 静态复核：`PlanView.qml` 面板 Upload 调用已有 `PlanMasterController` QML `upload()` 包装，顶部 `PlanToolBarIndicators.qml` 先检查 `globals.planMasterControllerPlanView` 和 `upload` 方法再调用同一包装；PlanView 销毁时按对象身份清理全局 controller。包装包含 ready/terrain、目标冻结、FleetHealth 和 `sendToVehiclePreCheck()`；仍需 QML 实例化验证工具栏加载时序。
- Guided legacy 目标静态复核：`executeAction()` 在确认 token、冻结目标和通用 gate 之后，对所有 single-target Vehicle API 再检查 `initialConnectComplete`、`vehicleLinkManager` 和 `communicationLost`；RTL/Land/急停的健康逃生例外仅绕过评分门禁，不再绕过失联/未初始化目标检查。仍需 Mock 验证掉线发生在确认与执行之间的时序。
- 飞行模式旁路静态复核：`PIDTuning.qml` 的自动 Stabilized/暂停模式切换和 `FlightModeDropdown.qml` 的下拉选择，在存在 MERIVUS Guided controller 时均调用 `requestFlightModeChange(mode, vehicleId)`；无控制器时保留通用 QGC 回退。仍需 QML/Mock 验证确认 token、active vehicle 切换和模式 ACK，回退路径不构成 MERIVUS 安全边界。
- RC 参数维护写入静态复核：`RCToParamDialog.qml` 在调用 `sendParamMapRC` 前检查目标初始连接、`vehicleLinkManager.communicationLost`、armed/flying 状态和四个数值有限性；MERIVUS Guided controller 存在时还要求 `FleetHealthCenter.evaluateTaskGate()` 允许。失败路径只显示阻止提示，不触达 Vehicle API；仍需 QML/Mock 验证参数回读与 ACK。
- 链路缺失静态复核：`GuidedActionsController`、`CommandCenterOverlay` 和 `PlanView` 均把缺少 `vehicleLinkManager` 视为不可发送/链路未知；CommandCenter 的 focus/takeoff/land/RTL 可用性和状态颜色与执行门禁一致，避免 UI 先放行再被底层拒绝。仍需运行时快照验证。
- PX4 CameraComponent 重启静态复核：Apply and Restart 按钮及执行函数共同检查初始连接、非空 link manager、未丢链路、未 armed/flying；失败只禁用/提示，不调用 `rebootVehicle()`。仍需工程师验证参数变更后的重启回执与页面恢复。
- 参数维护批量写入静态复核：`ParameterEditorController::sendDiff()` 在调用 `_factRawValueUpdateWorker()`/`Fact::setRawValue()` 前检查创建时 Vehicle 仍是当前活动机、初始连接、link manager、链路和 disarmed/on-ground 状态；`resetAllToDefaults()`/`resetAllToVehicleConfiguration()` 使用同一 C++ 门禁。`buildDiffFromFile()` 严格解析每行 vehicleId，缺失/非法/混合/跨机 ID 设置 `diffOtherVehicle`，`ParameterDiffDialog` 禁用确认并在竞态失败时保持弹窗。`ParameterEditor` 的参数加载、重置、RC 映射清理和重启入口，以及 `ParameterEditorDialog` 单参数/恢复默认提交前均有维护态提示。该检查没有证明 FactSystem 其它通用控件的所有 `Fact.value` 写入都已进入统一 C++ 门禁；工程师仍需编译并验证参数回写/拒绝 ACK。
- 兼容 Guided 控制器静态复核：`src/FlightDisplay/GuidedActionsController.qml::executeAction()` 在任何单机 Vehicle API 之前重新检查 `initialConnectComplete`、非空 `vehicleLinkManager` 和 `communicationLost=false`；`actionMVStartMission`/`actionMVPause` 在首条命令前遍历全量车辆，任一目标无效则整批拒绝，避免部分执行。阻断会提示并返回 `false`，未知 action 也显式失败。该改动仅覆盖兼容 QML fallback，未形成 C++ 权限/ACK/事务边界，仍需 Qt/Mock 验证 QML vehicle list 的 `count/get` 类型和 UI 行为。
- 兼容 Guided 恢复任务静态复核：`actionResumeMission` 现在进入正常确认框，复用 `_confirmedVehicle` 的活动目标一致性和执行时链路门禁，不再因 Mission complete 对话框调用 `confirmAction()` 后直接早退而丢失恢复动作；成功、未知动作和多机预检失败均清理一次性目标。该检查仍不能替代 MissionManager 的 operation token、上传 ACK/超时或恢复完成遥测。
- PX4 维护入口静态复核：`SensorsSetup.qml` 的校准后重启按钮与点击处理共同检查初始连接、非空 `vehicleLinkManager`、链路正常、未解锁且未飞行；Mission complete 的任务清理回调也增加初始连接和 link manager 检查。两处只完成 QML fail-closed 边界，未证明重启/清理 ACK 或完成回读。
- Mission resume/编队回滚静态复核：`MissionManager::generateResumeMission()` 在空原任务、生成后空列表和短任务当前项索引上 fail-closed；`SwarmController::_beginFormationAbort()` 将 `_sendFormationCommand()` 的 skipped IDs 纳入失败集合，无 pending 且有 skipped 时报告 rollback incomplete，不冒充本地完成；`endFormationSession()` 在 ABORT 阶段返回原事务，不重复注册回滚 ACK handler。仍需工程师验证 DO_JUMP、ABORT 拒绝/超时和异步 ACK 归属。
- APM 维护入口静态复核：`APMSensorsComponent.qml`、`APMSafetyComponent.qml`、`APMPowerComponent.qml` 的所有直接 `rebootVehicle()` 入口均在按钮和点击回调检查初始连接、link manager、通信状态、armed/flying；通用 MotorTest 与 ParameterManager Fact 写入已有 C++ 维护门禁，但角色权限、ACK/回执和其它旁路仍未形成完整闭环。
- 编队取消静态复核：`SwarmController::cancelTransaction()` 与 MERIVUS `GuidedActionsController.cancelTransaction()` 对 `formation.*` 事务拒绝地面-only cancel，要求 `endFormationSession()` 发送 ABORT；拒绝写入事件审计，避免取消事务后丢失机载回滚 ACK 关联。仍需运行时验证重复点击和 ABORT 终态。
- 意图审批机队身份静态复核：`prepareFleetIntent()` 记录稳定的 fleet fingerprint（ID、online/health、configuration/discovery、capabilities/verifiedCapabilities、vehicleType/firmware/role），`_reviewAllowsIntentApproval()` 同时比较当前 FleetRegistry 快照；外部 stale/改装 fleet 不能绕过审批。Registry 变化触发 token 清理；仍需 Mock/QML 验证列表类型与信号生命周期。
- Mission resume 异步静态复核：恢复上下文记录 `requestId/vehicleId/resumeIndex/itemCount/issuedAtMs`，ready、upload-fail 和 execute 阶段拒绝缺失或超过 120 秒的身份；`confirmAction()` 在 `showResumeMission`、索引和身份不满足时不建上下文。仍受底层 MissionManager 无 operation token 限制，不能单凭本层证明 ACK 归属。
- MotorTest 维护静态复核：`Common/MotorComponent.qml` 与 `APM/APMMotorComponent.qml` 的滑杆启用和 `onValueChanged` 均检查初始连接、非空 link manager、链路正常、未 armed/flying 和 `motorTest` API；Vehicle C++ `motorTest()` 继续执行维护态、参数和链路检查，失效时归零并不发送。角色权限与 MAVLink ACK/硬件回执仍未覆盖。
- Mission resume 对象生命周期静态复核：`MissionManager::generateResumeMission()` 在去重/过滤前缀命令时对被移除的临时 `MissionItem` 同步 `delete`，避免重复恢复任务积累泄漏；这是本地内存生命周期修复，不构成 MissionManager operation token、上传 ACK 或任务语义运行时证明。
- ACK 归属静态审计：`Vehicle::mavCommandResult` 仅携带 vehicle/component/command/result，Mission Protocol 消息也不携带地面站 operation token；`PlanManager` 虽已补本地 operation identity 和带 identity 的完成/错误信号，仍按当前 expected ACK/事务状态消费无线 Mission 消息。同一命令或跨 Mission 事务超时后重发时，迟到旧 ACK 仍可能落入新请求。`SwarmController` 的 dispatch epoch、事务互斥、临时 Mission identity/结果匹配和 formation generation 只能做地面站缓解，不能创造底层 token。工程师必须在 Vehicle/PlanManager 层继续完善 per-send generation/token 或 ACK quarantine，并用延迟旧 ACK、不同 component、read/write/removeAll 交叉时序验证。
- Vehicle timeout quarantine 静态复核：无响应终态才写入同组件/命令的单调时钟 quarantine，窗口内重发返回本地失败且迟到 ACK 在处理入口被忽略；`REQUEST_MESSAGE`、`SET_MESSAGE_INTERVAL`、`DO_SET_MODE`、`DO_MOTOR_TEST`、`COMPONENT_ARM_DISARM` 明确不隔离，避免破坏连接/流配置顺序、模式/解锁逃生和电机测试连续发送；只有命中的 terminal `SET_MESSAGE_INTERVAL` ACK 才推进下一条流配置。该实现保留旧信号/API，只是降低误归属概率；仍没有协议级 token，未编译、未验证高延迟链路和异常 ACK。

 - PlanManager operation identity 静态复核：每个 mission read/write/remove-all（含 guided/resume 写入）在开始处生成单调本地 `quint64`，结束/错误时保留旧信号并同步发出带 `operationId` 的新信号；`PlanMasterController` 和 Swarm 临时 Mission 状态均保存自身 operation identity，回调先做 identity 匹配再做 Mission 列表后置校验。该字段不在 MAVLink 线上，无法单独证明迟到旧 ACK 的物理归属；必须由工程师在 Qt/Mock/SITL 中覆盖旧回调、并发 MissionController 和 read/write/removeAll 交叉时序。
- MissionController/PlanMaster 全计划静态复核：`MissionController` 的 UI 刷新和 resume 转发现在由 `operationStarted`/带 identity 回调驱动；`PlanMasterController` 的 Mission、GeoFence、Rally 分阶段串联各自核对 operation ID，Mission 写失败会停止后续 fence/rally 阶段。初始自动加载在控制器晚于 Vehicle 连接时通过当前 active operation ID 兼容接管；仍需运行时验证自动同步、组件不支持、迟到回调和真实 ACK。
- Sync identity race follow-up：`PlanMasterController` captures `operationStarted` before invoking a manager, so synchronous completion cannot erase the ID before it is recorded. GeoFence/Rally controllers now consume `PlanManager::*WithOperationId` callbacks and reject stale UI refreshes. Plan load/send/remove-all entrypoints also fail closed on initial-connect/link/communication/armed/flying state, and transient upload controllers clean up on failed or missing transactions.
- Parameter transport boundary: `ParameterManager::_factRawValueUpdated()` rejects direct Fact writes while offline, initial-connect incomplete, disconnected, armed, or flying; a live link triggers an authoritative refresh instead of PARAM_SET. This closes remaining generic setup Fact widgets without claiming runtime ACK proof.
- APM maintenance hardening: opening the APM sub-motor page no longer sends an automatic DISARM. Direction changes, manual motor-test arming and automatic direction detection require grounded maintenance state and re-check that state in the delayed confirmation callback.
- Initial-connect plan loads: Vehicle no longer advances the initial connection state machine from legacy Mission/GeoFence/Rally no-argument completion signals. It tracks read operation IDs and accepts only matching `*WithOperationId` completions; the old no-argument slots remain explicit skip paths invoked by `InitialConnectStateMachine`.

### 空域检查器补充审计

`FleetAirspaceConflictChecker` 已改为二维线段—多边形精确相交（含边界）并对高度/时间窗采取保守相交；路径缺高而禁飞区有高度上下限时阻断。该改动只降低固定采样漏报和缺高默认 0 的风险，仍未证明坐标基准、时间证据或法规数据正确。`timeStepSeconds` 当前是格式校验字段，不参与几何步长；工程师不得把它写入放行说明或当成运行时避障能力。

电机测试边界保留 `timeoutSecs=0` 作为现有滑杆的停止/禁用语义；C++ 入口只拒绝负数，同时仍要求连接、初始握手、主链路、落地和解锁状态满足维护条件。

任务审查时间证据现在由 `FleetMissionReviewCoordinator` 统一传给空域检查器：任务/约束时长优先，只有有限 `startAt`/`deadline` 时才派生时长；无效或反向值保留为阻断输入，避免能量调度和空域冲突审查看到不同时间模型。

高频 Joystick 发送路径现在先拒绝非有限轴值，并把有限越界值限制到 `[-1,1]` 后再转换为 MAVLink `int16`；静态检查只证明入口防护存在，不能替代工程师对 Joystick 断链、飞行态和真实链路的回归。

空域 `bounds` 输入现在按字段语义分为地理经纬度矩形和本地米制矩形，并要求有限、正向边界；这消除了此前 `west/east/south/north` 被当作本地 `x/y` 的静态歧义。跨日期变更线的地理矩形仍需工程师补充专门几何回归。

Vehicle 流配置路径也已做生命周期审计：初始握手需要的 `requestDataStream()` 不要求完整握手，但要求 manager/通信/主链路存在；批量重试队列、PID 流切换和 `closeVehicle()` 均对失效链路 fail-closed。该静态检查不证明流配置 ACK 或断线重连策略。

Vehicle 关闭与 ACK 副作用边界也已收口：关闭时停止命令/批量消息定时器并失败化待处理命令、消息 waiter；ROI、参数重置和 Bootloader 成功提示只在 ACK 匹配活动命令且为终态时更新。静态检查不替代链路切换、关闭竞态和迟到 ACK 的运行时回归。

风险评估器的健康/能量输入也已改为 fail-closed：`degraded/warning/limited` 只能生成 major 人工复核风险，故障/失联/维护状态归为不可用，未知状态不能落入 healthy；健康报告必须显式声明布尔 `online` 且不能包含计划外机体。非空能量计划必须提供布尔 `feasible` 与 `parametersValid`，缺失或无效证据进入输入阻断。该 taxonomy 仍需与 FleetHealthCenter、EnergyScheduler、ReportBuilder 的真实调用数据在工程师环境中对齐验证。

`FleetMissionReportBuilder` 已同步同一 taxonomy，并拒绝把缺失/字符串在线标志当成在线；报告中会把降级、不可用和未知证据分别记为 major/critical 问题。静态一致性不等于真实报告发布权限或运行时输入可信度，仍需工程师验证所有调用方字段类型。

空域审查器的 `vehicleId/systemId/id` 也已统一为真实有限整数 1–255；数值字符串、`"01"` 和显示名称不再被转换为 MAVLink 身份，避免编译器、连续性规划器和空域审查器对同一载荷产生身份分叉。

数字孪生的内存 `setSnapshot()` 现在与 JSON 导入共用 `snapshotHash` 校验，外部调用不能只改哈希字段就伪造完整快照；预演对 Fleet 行的在线字段只接受真实布尔，缺失或非法类型会落入未知统计。仍需工程师验证旧快照兼容策略和 QML QVariant 类型。

`requestMessage()` 及通用命令队列的异步生命周期也已补强：主链路在命令入队前消失时必须回调失败而不是静默返回；空回调、重复的单一消息等待器和不可用传输均 fail-closed；命令失败、消息到达或超时终态会释放请求上下文。该修复只解决地面站回调归属/内存生命周期，仍需工程师用 ACK/消息乱序和链路切换回归验证。
- Custom command policy: invalid MAV_CMD/component IDs and non-finite parameters are rejected at JSON load; only camera capture 2000/2001 can be explicitly `inflight_safe`, while unknown or arbitrary commands remain disarmed-only. ACK quarantine now includes mode and arm/disarm commands; only benign request/stream setup commands remain exempt.
- Bootloader entry: `Vehicle::flashBootloader()` now enforces initial-connect, primary-link, communication, grounded and disarmed state in C++; Firmware Upgrade adds a frozen-target second confirmation. This is still static-only and requires engineer verification of the actual flash protocol and recovery path.
- Guided action payloads: Orbit/Set Home/ROI now use primitive coordinate snapshots with finite range checks and execution-time reconstruction; Set Waypoint compares the confirmed mission item count to the live count. Compatibility and MERIVUS multi-vehicle Pause freeze IDs and preflight all transport/API targets before dispatch.

工程师必须在目标 Qt/MSVC 环境重新生成 qmake/moc 并编译，随后按交接文档执行 QML 实例化、Mock/SITL、MAVLink ACK/遥测时序、异常注入、链路中断、权限边界、真实硬件测试和刷机放行。静态审计没有证明飞行安全、悬停稳定、集群自愈或任何实际飞行行为。

## 本轮后端入口与异步边界补强

- `Vehicle` 的 C++ transport/maintenance gate 覆盖重启、标定、UAVCAN 配置、电机测试、Bootloader 刷写，以及模式/Guided/任务/RTL/Land/Takeoff/急停/VTOL/日志等命令入口；维护命令要求初始连接、主链路、无通信丢失、未解锁且未飞行，飞行允许类至少拒绝离线、未握手和断链。
- Guided C++ API 对坐标、半径、高度、速度和爬升量执行有限值/范围检查；`doSetHome` 的异步地形回调重新检查 transport、缓存坐标和地形数组，避免旧回调发送无效或过期命令。
- `PlanManager`/`MissionManager` 在任务读写、clear-all、请求/重试、item 回传、resume/guided 写入各异步边界重新校验 live transport；常规任务写入/清空在解锁或飞行后终止，guided item 保留明确例外。参数定时重试不能绕过 Fact 写入总闸。
- Swarm 临时 Mission 在定时 dispatch 即将 `writeMissionItems()` 前再次复核目标、链路、健康、武装/飞行状态及冻结航点。

以上均为静态 fail-closed 设计，仍需工程师编译并用 Mock/SITL 验证延迟回调、ACK、guided 例外和真实链路行为。

- 其它直接 Vehicle 控制入口也补上了 C++ transport 边界：云台、相机触发、吊舱、Joystick 手动控制和 RC 参数映射；RC 映射增加有限数值、tuning ID 与参数名边界，云台增加有限角度/范围校验。
- 自动 Home 更新和 terrain altitude 回调增加连接状态、有限坐标及空结果保护，后台位置变化或异步 terrain 结果不会在断链/坏数据下直接发送 `MAV_CMD_DO_SET_HOME` 或访问空数组。
- 初始连接兼容性：`InitialConnectStateMachine` 的三次被动 Mission/GeoFence/Rally 读取显式传入 `loadFromVehicle(true)`；PlanManager 只对该标记放宽“初始连接未完成”这一项，仍检查 offline/link/communication，普通 UI 读取没有该例外。Mission sequence 0 在不发送 Home 项时不再下溢为 65535。
- Mission resume 的成功路径在复制到 PlanManager 写入列表后释放筛选用的临时 `MissionItem` 克隆；空/无效路径仍清理原列表，避免重复生成造成地面站内存累积。
- `MissionManager::generateResumeMission()` 对 direct resume index 执行严格范围检查，非法负值/越界不会被静默夹到 0；QML 确认链仍负责冻结目标和任务项快照。
- 临时 Mission 清理在 `SwarmController` 发送 `removeAll()` 前再次检查初始连接、主链路、通信状态和 grounded/disarmed；空中或断链只延迟清理，不改写当前机载任务。`TemporaryMissionState` 的重复成员声明也已移除。
- 通用 `Vehicle::_sendMavCommandWorker()` 在 VehicleLinkManager 已报告通信丢失时拒绝新命令并返回失败回调；它保留初始连接阶段 benign 命令需要的路径，不把全局 worker 错误地收紧成“必须已完成初始连接”。
- 同一 worker 对空 VehicleLinkManager 也直接失败并回调，避免 stale/plugin 入口在链路对象不存在时解引用。
- Early-read 例外只在活动 `TransactionRead` 中生效；`_finishTransaction()` 清除事务后，直接连接的 completion callback 启动的写操作不会继承该例外。
- 候选健康 taxonomy 复核：`FleetCapabilityMatcher`、`FleetRoleAllocator`、`FleetTaskTemplateRegistry`、`FleetEnergyScheduler` 现统一拒绝 `degraded/warning/limited` 状态并返回 `health_degraded`，故障/失联类仍返回 critical，unknown 仍按各自 `allowUnverified`/证据策略处理；降级机体不会被直接当作能力、角色、模板或能量可用候选。
- 能量集合一致性复核：`FleetMissionReviewCoordinator::deriveEnergyTask()` 将 compiler/role allocator 的成员与备用机 ID 传为 `candidateVehicleIds`；`FleetEnergyScheduler` 在存在该约束时只从这组权威身份计算，列表类型/范围/重复项错误 fail-closed，避免能量报告用另一组机体掩盖实际任务成员不足。
- 时间线/空域时间复核：`FleetTimelineReplay` 的内存替换与 JSON 导入现在要求与黑匣子一致的严格事件字段、资源上限和非递减时间戳；空域审查器对部分时间戳仍可填充估计值用于空间保守检查，但 `timingKnown=false`，不会把插值结果当成权威 4D 冲突时间窗。
- 证据链时间/身份复核：`FleetMissionEvidenceChain` 追加与导入均拒绝早于链尾的时间戳，避免哈希有效但审计顺序倒退；该链仍只提供完整性链接，不把本地 `headHash` 当作操作者认证或数字签名，主体身份必须由上层权限/签名系统提供。
- 链路指标接线复核：`GuidedActionsController` 采样会从 descriptor、metadata 和 linkMetrics 读取 quality/bandwidth/latency/packet-loss/state；真实数值 `-1` 只表示未观测并保持 unknown，字符串、越界和非法状态原样进入策略的严格输入门禁，不会被 `Number()` 隐式转换或误报为 normal。策略输出仍是 recommendation-only，不调用传输或飞行接口。
- Registry 能力查询复核：`FleetRegistry::vehiclesWithCapability(capability, requireVerified)` 默认保留 observed 能力兼容查询，但 `requireVerified=true` 只读取显式 `metadata.verifiedCapabilities` 且拒绝坏类型；QML facade 同步传递布尔门槛，安全任务不应把普通 observed 查询当作能力放行证明。
- 维护护照展示复核：`CommandCenterOverlay.maintenanceReviewCount()` 与维护摘要共用同一状态分类；只要评估为 `unknown/monitor/inspection_due/grounding_recommended` 等非明确通过状态，状态条保持警示色。该层只读显示人工复核需求，不改变维护评估、FleetHealth、任务门禁或命令释放。
- 能力证据展示复核：指挥中心新增 `capabilityTrustSummary()`，区分 verified、observed-only 与 discovery invalid 车辆；这是操作员可见的证据状态提示，不能替代后端 verified-only 匹配门禁。
- 证据主体上下文复核：`GuidedActionsController._appendMissionEvidence()` 为本控制器生成的记录附加 `groundStationAudit`，包含当前 `operatorId/operatorRole`、`identitySource`、`identityPresent`，并固定写入 `authenticated=false/signatureVerified=false`；这让缺失身份可见，但不把本地策略值伪装成认证、签名或权限证明。
- 事务目标集合完整性复核：`CommandTransaction` 保存原始 `targetVehicleIds`，任何非法、重复或别名 ID 都设置 `targetInputValid=false`；`start()` 在首个 dispatch 前失败，`resultSummary()` 同时给出 `requestedTargetCount` 和有效内部目标数，避免将部分目标误报为整组事务。
- 扩展元数据资源边界复核：`FleetExtensionRegistry` 的注册和快照导入共用最多 2048 条扩展、递归深度、节点数、键长度及累计字符串/字节上限；超限元数据或条目数整笔拒绝，且该目录仍不加载二进制、不启用命令释放。

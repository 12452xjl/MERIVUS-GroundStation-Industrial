# 连续原生入站遥测记录器

## 523：当前原生 GNSS 报告的仅原生留存读取

新增非 Q_PROPERTY／非 Q_INVOKABLE 的 `nativeGnssReview(token,confirmed)`。仅在实际 GUI 线程、确认true、原 `gnssReviewCurrent()` 当前、实际 UUID token 精确相同且 `_localHistoryActionBusy=false` 时返回原完整成功33报告；还须通过 [GnssReview::reportValid](FleetTelemetryGnssReview.md) 的33／7／3／18完整纯值检查，否则返回空Map。先做 owner-thread 当前性检查再读私态；不接 caller 报告、脱敏 page 或坐标。同步 `gnss-review` 自身 changed 通知期间 public getter 仍可读，但不能在该 action 尚未释放时借通知重入追加。

[OperationalMemory](FleetOperationalMemory.md) 只从这个实际入口追加，独立保留最多32报告／2048样本；页／样本完整重叠则拒，capture增长改变fileHash不重置去重。追加不是 capture控制、任务完成、native live重建或权限恢复；旧motion／dossier／维护页／Reader／Codec格式不变。Memory私有冻结／独立文件重开各有自己的报告选择token，不能充当本 Archive 的新token。Root现在以明确origin区分 `native-page`／`session-snapshot`／`file-history`，地图仍只读、默认不自动启用。

本轮未编译、执行项目／parser／helper／hash／QML／测试或设备，仅源码和合同静读。后续工程场景须覆盖同页新token、action发布内追加拒绝、真实追加后通知销毁保commit事实、旧query／token／页变化不能追加，以及文件中的自洽33不能绕过实际 Archive getter。下方为历史合同。

## 522：同一原生历史页的 GNSS 同包报告

原 `requestLocalHistoryAction` 的 exact6／receipt11 新增独立 `gnss-review` 动作，既有 `review` 继续生成 Motion31／dossier21，不改旧格式或派生通道。GNSS 只在固定本地 catalog、真实最新成功且 reader 双终结已释放的原页、cursor／机号／位置同意完全相同时调用 [GnssReview](FleetTelemetryGnssReview.md) 一次；cap64、整页原 schema／Codec 验证后才解同包 GPS_RAW_INT24。不能由脱敏 public page、caller coordinates、当前 Registry 或相邻消息重建位置。

只读 `gnssReview` 是该完整报告33，`gnssReviewToken` 是本次成功原生发布的独立 UUID；同页同值重算仍换 token，失败或退休清二者。新读、取消、新 catalog、实际新 capture、退出均退休；getter还核原 serial链形成的latest／last、实际页7身份、固定目录／策略、无capture／reader忙以及成功双终结。自身同步 action 发布 fence 不冒充 reader忙，以免 changed 通知期间刚生成的报告不可读。report/token 一并安装后发一次 changed，通知尾核原 QPointer／serial／完整页和二者，旧请求不回撤回调的新结果。

仅 fix3／4／5／6／8与同包合法E7坐标可投点；无fix／2D／STATIC／未知型不沿旧坐标投点。eph 为HDOP×100，不是米；65535及sat255为未知−1，0不被改成未知。HDOP>2.5或sat<6仅本显示启发，unknown不变阈值内。页内质量、计数、原sourceEpoch/sourceSequence、局部UTC与完整覆盖状态均按固定 [33／7／3／18合同](FleetTelemetryGnssReview.md) 解释。位置值显式确认不认证飞机、文件、真实精度、漂移、任务结果、动态轨迹或 StartPermit。

原Reader／Worker／Codec格式、motion/dossier、搜索、517维护与518工作区保持；不是持久跨任务全集。后续可见地图须另次显式选择准确原token，不能自动居中、goto、回放飞行或从旧同内容恢复资格。仅静态修改／对读，未编译、执行实现／helper／解析器／QML／测试、仿真或硬件操作。下方为历史合同。

## 517：实际当前历史页的维护证据预览

新增受限metadata候选6／来源5和私有preview14。仅固定本地策略下当前成功完成的原native页，扫描其中BATTERY_STATUS；不改旧page36、frame29、public summary、recording或motion报告。公开QML不获raw包，不从public行重建payload。

prepare冻结原页、query和完整七字段来源，先退旧再发通知，通知后复核原代次与页；native-only record再次核实际当前预览。新读、取消、录制等使旧预览不可操作。discard只处理准确ID；无自动追加或撤他人新预览。

页内温度／非负电流峰值、样本覆盖、收包UTC回退及Footer/尾行/下一页状态沿evidence21保留。全未知不能追加，sourceEpoch/数字机号/文件hash均不认证实物。

[完整合同](FleetMaintenanceHistoryImport.md)、[实际护照消费者](FleetMaintenancePassport.md)、[未执行工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批未运行或测试；下方为历史。

## 493：独立search值与原完整catalog驱动的窄操作链

新增只读historySearch及独立historySearchChanged；requestLocalHistorySearch exact8为action、expectedQueryId、catalogId、searchId、basename、vehicleId、messageId七个真实字符串和includePositionValues真实Bool。动作仅search-first／search-next／search-open；固定本机目录与policy由native决定，不借宽native口的任意目录。旧local action6／options7／receipt11和所有旧page／位置／报告合同不变。

首批从当前原完整catalog字典按basename冻结全部descriptor（含坏header），生成新searchId UUID；续批保该ID、原manifest／policy／catalog及过滤条件，使用私有实际DirectoryIdentity和nextFileIndex，queryId每批新生。不接受caller path／manifest／after／hash。searchSourceCurrent只比保留的原生目录／policy／完整descriptor来源，不在getter读盘或hash，也不认证磁盘此刻未变。

[Reader](FleetTelemetryArchiveHistoryReader.md)原生34严格匹配请求后，移除directoryIdentity并添加queryId／catalogId／searchId成为public exact36。每行仍exact10固定元数据，私有cursor不输出；当前getter还要求最新query、actual accepted／未取消、terminalDelivered和finishedObserved双条件且单槽已释放。两终结任意次序均再发所需notify。新read／cancel／新capture清search/source/cursor/last；capture结束不能恢复旧资格。

search-open仅当前matched且count>0原hit，核同一完整catalog descriptor及原fileHash；私有page starter把此hash放入旧request11的expectedFileHash，即使after=0也必须重验，不沿旧first清空hash。不新造单hit目录、不给任意hash setter，也不猜首序号-1。打开整个file首页，仍只有vehicle／position过滤；搜索msgid不变成page筛选，首页可无该消息。后续next／review沿旧页、原hash与原位置同意，不把多Run元数据交给MotionReview／dossier。

本功能复用原单Reader／nonce／serial／终态＋actualfinished槽，合作取消不提前复用，不接live／任务恢复／权限。目录对象身份用于续批对应，fileHash用于打开同材料；均不认证作者、介质身份、恶意TOCTOU、全遥测、任务Run或正式结果。[实际UI](FleetTelemetryHistoryPanel.md)与[36未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)同步。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 487：同一原页一次生成31与观察卷宗

review不再直接调用纯MotionReview，而只调用[ReportBuilder新static](FleetMissionReportBuilder.md)一次，从[返回卷宗21](FleetTelemetryObservationReport.md)内取得原31；输入action／intent6／options7／receipt11及读取格式不改。新motionDossier只读属性与原motionReview共用motionReviewCurrent，检查真实latest/last、accepted、未取消、实际双终结完成、六身份和嵌套31相等，任一缺失两者均空。

合法实际review前双清，整份21／来源6／覆盖8／发现3／四视角6／section3／限制和1MiB预算核后，另核正文hash以及原page/serial对应；通过才双赋值一次changed，通知后再核两缓存和原源。失败两者空；通知中换源只返回结局不明，不无条件清回调的新状态。新start、cancel和catalog接纳点双清，两个getter不互递归。

Root只多公开固定dossier值，不开放Builder实例、raw页、文件保存或任务API。[面板](FleetTelemetryHistoryPanel.md)原确认及代次链采用同一快照，不按采集run自动关联任务。34工程场景全未执行；本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486：固定历史只读动作及真实单页摘要消费者

新增localHistoryOptions exact7，固定QStandardPaths::AppDataLocation/fleet-os/telemetry-archive；读取policy exact4为64MiB／128文件／1MiB页／64帧，每次页64。构造与getter只形成路径，不建目录、不读盘、不录制。原Reader要求实际存在且合规；QML没有任意路径或原生capture引用。

requestLocalHistoryAction exact6：action、expectedQueryId、catalogId、basename、vehicleId实际string，includePositionValues实际Bool；只接受catalog／first／next／review／cancel。catalog／cancel的未用选择字段严格空，全部锁真实latestID；first按实际字典与header同意，next／review锁真实最新双终结页及私有cursor/filter/位置选择。query与serial原生生成，receipt exact11只说明请求接受，不代表结果成功。旧native broad口保留但不能把任意目录／策略借给本窄口。

[MotionReview](FleetTelemetryMotionReview.md)只在另一次review明确确认且原页含位置时调用；结果31、六身份和false边界吻合后才保存。motionReview getter只在readerNonce已释放、accepted／未取消、latest==lastPage且六身份仍同原页时返回；新读／取消清旧report。终态与actualfinished双条件释放单槽不改。非法前置请求不改旧状态，真正review失败清旧摘要；不回滚或自动重试。

[实际UI](FleetTelemetryHistoryPanel.md)经Root窄facade进入两个宿主；原Frame29／Header19／Footer31、Reader页34加两个原生身份成为36、公开catalog／page摘要格式不改。32工程场景全部未执行；仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 310–314 原生目录读取失败边界

Writer 与 HistoryReader 改用 [FleetArchiveDirectoryInventory](FleetArchiveDirectoryInventory.md)。仅原生枚举正常 EOF、目录对象前后/重开检查及显式 close 成功才接纳；取消、I/O、坏名、超限或不支持平台整批拒绝。Reader 前后扫描私有 directoryIdentity 与文件视图都须一致。实际 catalog 顶层 `directoryInventoryEnumerationCompleted` 必须为非 null 的真实 Qt bool true，门面独立核对后才生成新 catalogId，并复制该固定公开旗标；page 不据此新增目录声明。

Worker 当前态同名旗标只说明该次 namespace 容量/metadata 检查完成；不是全历史认证或目录实时状态。Header19/Frame29/Footer31、entry8/request11、页/定位同意和线程生命周期不变，Codec 接纳新增固定 Worker 错误码后白名单23项。`directoryEnumerationCompletenessVerified`、认证、原子快照和 fullTelemetryCaptured 仍为 false。旧章节关于 Qt iterator 的缺口为历史实现，本次补的是 EOF/error 来源，不是全集索引、自动留存或完整遥测。

未实际调用枚举/日志/hash/helper、编译或测试。工程师须在目标平台验证空目录、读到一半失败、取消、close 失败、Unicode、符号链接/重解析点、容量边界与目录替换；本地对象 ID/metadata 不防所有恶意置换或跨文件系统别名。

## 305–309 独立历史目录、分页与脱敏接线

新增[只读 HistoryReader](FleetTelemetryArchiveHistoryReader.md)及Codec严格Header19/
Footer31校验，保留原frame29/27白名单和writer行为。native
`requestHistoryCatalog(queryId, absoluteDirectory, readPolicy4, operatorConfirmed)`
可在没有live源时读已存在的固定namespace；不会新建、修复、删除、轮换或锁写
档案。readPolicy精确真实Qt整数：maximumFileMiB 1..1024、maximumCatalogFiles
1..128、maximumPageMiB 1..8、maximumPageFrames 1..256；不补默认值。

queryId 是1..128字符ASCII字母/数字/下划线/连字符，内部每次另生独立UUID nonce；
重用外部queryId不能让旧queued结果污染新请求。目录成功后生成actual catalogId，
冻结实际directory/policy/8字段条目；invalid/空/超限候选保留固定原因但不可分页，
不隐去失败NewOnly文件。目录全集/全记录验证/跨文件原子均不认证。

`requestHistoryPage(queryId, catalogId, basename, afterSequence, vehicleId, limit,
includePositionPayload, operatorConfirmed)` 不接caller任意descriptor：只用本实际
catalog条目。vehicleId=0为全部或1..255单机，具体必须在文件header.scope；序号从
0开始，非零必须本basename上次actual next，且catalog/run/filter/定位读取确认和
上次actual fileHash均匹配。最多128个文件各一个筛选上下文，换filter/定位确认
需从0重读，换catalog清旧游标，失败/取消不推进。目录元数据改变需重取catalog。

历史单槽与本对象capture互斥；拒绝/busy请求不改旧结果，被接纳的新查询先清私有
raw页/成功token。`cancelHistory()`仅请求合作取消并忽略late结果，槽直到终态
结果或失败已交付 **且** actual QThread finished，两种顺序都不提前复用。
结果已交付后cancel返回false（不能撤回已发布历史）；仍等待actualfinished才可
开始下一查询，不把这段等待称仍可取消的读取，也不回滚已观察到的游标。
关闭/退出/析构请求cancel，不wait/terminate或删除运行线程；源码不调用这些路径。

public `historyCatalog/historyPage` 是固定投影；每页row仅6键：sequence、vehicleId、
componentId、messageId、positionTelemetry、payloadPubliclyExposed=false。不复制原
payload、frame UTC/mono、checksum、source epoch/sequence、signature或任意Footer
文本；文件basename/hash/mtime仅用于历史目录观察，不是飞行时序或认证。
不是对raw map简单remove几键。`nativeHistoryPage(queryId,true)`不是Q_PROPERTY/
Q_INVOKABLE，只有同实际GUI、最新成功page、非cancel且历史槽实际释放后可取私有
raw值；本次定位payload另需显式includePositionPayload。磁盘头的定位同意不能
替代本次读取同意，bool本身也不是账号鉴权。新查询即使ID重用且失败也不会取回
旧raw页冒充成功；公开旧页只是带原queryId的历史摘要，不是当前查询结果。

Reader整文件逐完整行校验与hash，校验所有frame的本run/source scope、递增archive/
各epoch序号及Footer实际计数/字节/末序号，关闭后再只读重开hash复核。尾部未终止
最后一行只标partialTail，不解析或公开；缺Footer允许已校验完整前缀，但完整非法
Footer、内部坏行、超长行/重复序号或Footer后数据拒绝整页。paused其它writer可在
一个稳定观察窗口形成前缀，不称封存/不可变/全集；变化由meta/hash检查拒绝，hash
不是验签或恶意TOCTOU完整防御，closure/fullTelemetry/权限/执行认证始终false。

本次只有源码、限定文本和同行复读，没有运行文件枚举/打开/hash/Codec/Reader/
锁/SQL/helper、编译/测试/仿真/设备或刷机，不更新PR/包。仍需真实覆盖/更多批准
消息、error-aware全集索引、长期归档留存/统计维护/恢复和其它五类代码目标。

300–304 新增源码。只有源码、登记和限定文本审阅；未编译、运行、测试、打开
记录文件/数据库、仿真、接设备、改 PX4 或刷机，也未更新 PR/源码包/安装包。

## 实际接线与默认状态

[`FleetTelemetryArchive.h/.cc`](FleetTelemetryArchive.h) 派生
[`VehicleTelemetryObserver`](../../../src/Vehicle/VehicleTelemetryObserver.md)，从实际
Vehicle pre-plugin 原生入站消息接收有界值；不是 BlackBox 的诊断摘要，也不
借旧 `.tlog` 或机载 ULog API 证明完整采集。新对象在内部 controller 以
`merivusFleetTelemetryArchive` 登记，公开 `telemetryArchive.current` 仅复制状态，
没有配置/目录、start/stop、导入、回灌、任务、审批或发指令方法。

新增记录器默认 disabled；构造/QML 实例不会访问文件、绑定飞机或请求数据流。
这不改变软件原有 MAVLinkProtocol 临时 `.tlog` 行为：旧日志可能仍按原策略
记录收发内容，不能把新增默认关闭/过滤当作整个软件的隐私关闭或旧日志净化。

## Native 启动与停止

`beginCapture(absoluteDirectory, exactPolicy, exactVehicleIds,
includePositionTelemetry, operatorConfirmed)` 不是 Q_INVOKABLE。由受信任 C++ 后端
提供现存绝对本地目录和真实整数的 1..255 目标列表（不重复，1..32 架），必须
取得当前实际注册 Vehicle/PRIMARY 绑定；32 是记录目标上限，不扩大原六机编队
协议。任何一架拒绝则只撤销本次观察者绑定，不发指令或拆他人的观察者。

exactPolicy 精确六个真实 Qt 整数字段，不接受字符串、double、bool 或未知字段：

| 字段 | 含边界范围 |
| --- | --- |
| maximumQueueFrames | 16..4096 |
| maximumQueueMiB | 1..16 |
| batchFrameLimit | 1..128 |
| maximumFileMiB | 1..1024 |
| maximumRunFiles | 1..128 |
| flushIntervalMs | 100..5000 |

operatorConfirmed 只表示本次 native 明确确认，不是身份/权限认证。
includePositionTelemetry 是独立定位数据确认：LOCAL/ODOMETRY 同样包含轨迹，
并非“只需授权 GNSS”；具体固定白名单和 payload 隐私见
[Codec](FleetTelemetryArchiveCodec.md)。每次新 run 重新明确配置，不恢复旧批准。

begin 返回 opening-pending，开始接收有界待写队列，不代表文件创建/写入成功；
worker ready 才显示 recording。`stopCapture(drain)` 返回停止请求，不等于完成
或断电持久化。停止先清捕获并解绑，保留实际 worker 直到 finished；退出和
派生析构只请求后台排空，不 wait/terminate/delete 运行线程，不保证进程退出时
尾部已落盘。[Worker](FleetTelemetryArchiveWorker.md) 独占新 NDJSON 文件，旧文件
不覆盖、不删除，没有导入/回放入口。

Header冻结本次实际1..32来源的 vehicleId/sourceEpoch/initialSequence=0及定位确认，
worker只接受匹配该来源scope/本run/consent的Codec记录。没有发出白名单包的选中
成员也保留为本次意图范围，但该表不是硬件或账号身份认证，不自动加入新来源。

## 连续性、缺口与证据强度

native source UUID/uint64 序号和本 run int64 offered 序号是不同域，均不回绕；
计数以十进制字符串公开。固定10字段 captureSummary 保存 received、filtered、
invalid、queueRejected、sourceMissing、retired、offered、开始/结束 UTC 及
sourceCoverageClosed。received 包括未批准消息，filtered 不是网络丢失；invalid、
队列拒绝、写入/flush 错误独立保留。源切换/删除/失联停止整组本次采集，尾部
unknown，不自动跟随新机；轮询仅补救通知，不把未观察窗口推算成精确丢包数。

sourceCoverageClosed=true 仅指本地明确停止时所绑定来源仍匹配并已关闭观察窗口，
**不是**消息全集、无线连续性、无过滤/队列丢失或真实飞行完成认证；sourceMissing
只能数本地序号已知空档。后台 captureSummary 是最近接受的固定状态采样，异常
停止时可能落后于写入；不能假定它是遥测和文件的原子全集快照。

每条 receivedMonotonicMs 来自对应 Vehicle 绑定时启动的 QElapsedTimer，以
sourceEpoch 划分时钟域，跨飞机不能直接比较；UTC 只是同步接收近似时间，不能
作机载采样/绝对同步证明。控制器另用本地单调时钟标记超过5秒的 wall jump，
不修正历史样本或恢复执行许可。MAVLink rxSequence 只观察，不推算精确网络 loss。

真实 write、QFile flush 与断电 durable 分层；新状态固定 durableVerified=false，
crcVerified/authenticated/permissionsRestored/executionPermit/flightCommandReleased
均 false。parser-accepted 原 len payload 不是线缆原始字节，签名 presence 不当验签。
fullTelemetryCaptured 始终 false：只存当前实际已收到、固定批准的 RX 子集，不
采 TX、字节噪声、被 parser 丢弃/根本未上报的消息、其它链路或任意私有扩展。

## 后续与工程师接手

仍需批准的新消息分类/实际遥测覆盖证据、独立只读检索与索引、长期归档/留存、
跨任务统计和维护接入、平台权限/灾后恢复；本增量不能替代完整25目标/六类。
真实写盘错误/容量/部分写入、stop-before-run/取消排空、不同链路与源 ABA、进程
退出/断电、定位确认及旧日志独立隐私设置、Qt JSON/线程开销和 parser 签名/CRC
专项场景均留给工程师。本批没有执行任何上述场景。

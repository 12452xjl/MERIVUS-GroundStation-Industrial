# ComponentInformationManager — 原生组件元数据文件接纳

## 532：进程级有界调度的实际文件消费者

原 artifact worker 的正常启动只走`schedule()`；失源后的旧`cancel+start`清理入口
删除，未受理对象直接准确清理，不再直接`QThread::start()`。所有实际结果／finished／neverStarted／
destroyed连接先安装再受理；每个FileRequest仍固定原worker QObject、UUID、私有不复用
generation、原source/serial/phase，不能靠共享latest或后来同tag的结果领养。

`schedule=false` 表示从未受理，不期待worker自己的终结信号；caller只清自己的原等待，
对准确未启动对象deleteLater。`true`才交给进程级
[ComponentMetadataWorkScheduler](ComponentMetadataWorkScheduler.md)：queued取消、排队超期
等在从未运行时发独立`neverStarted(id,reason)`一次并由worker安排自身删除，既无
resultReady也无QThread::finished。requestCancel对queued只是准确sticky撤销，等待
scheduler原检查，不假设立即清pin。正常running取消仍仅requestCancel，保留实际
resultReady＋finished双尾和独立finished回收；不wait／terminate／删除运行线程。
实际进程配额为这两类worker合计running最多2、queued最多32，排队单调120秒上限；
100ms检查及获槽前重核，不把排队期限当OS／parser物理抢占或整个下载流水线时限。

neverStarted及拒受理不进入`_finishArtifact`、不读取artifact、不冒充CRC／JSON校验失败，
不invalidate好cache、不重下同URI，也不因advance而另发fallbackURI。只清该准确worker
等待并退休原可选metadata批次，沿既有optional-step合同对仍活原owner终结一次；不称
文件成功。已经失源的请求只清原等待，不续原处理，新source可在实际idle后正常进入。
排队终结发生于schedule返回前时先冻结plain reason，等实际受理返回再接纳；running
的两真实终结仍保持原样。旧destroyed不能把已知never-started误作新worker缺尾，不能
跨代次失效缓存。元数据／summary／下载输入仍由原native owner持有至准确排空。

只有这两个production worker类走本调度，不增加下载／压缩／参数／飞行sender，不将
进程配额或排队顺序当远端身份、可信构型、性能实测或最终StartPermit。以下529及更早
章节保留当时实现说明；532为当前queued/running终结区别。仅源码修改与直接静读，
未执行线程、排队、下载、解析、业务IO、编译或测试。工程需覆盖global满/超期/queued
cancel、入场通知内销毁、schedule返回前终结、好cache不被误删重下、失源后新source、
running实际双尾/缺尾及输入owner清理。

## 529：两段后台翻译的实际原生调用者

生产入口改为 `downloadAndTranslateArtifacts(requestId,summaryArtifact,metadataArtifact,maxAge)`，不是把两个 QString 放进线程就放弃其来源寿命。原 TranslationRequest 自有 UUID、实际 producer 和两个原生 artifact；只听独立 artifactTranslationComplete(id,path,error)，核原 producer／ID 后沿原 starting buffer 处理。legacy downloadComplete 不可让本请求接管外国译文路径。

source 退休先提交 inactive，再以原 ID 和准确两 artifact 调用 cancelArtifacts；受理栈内退休／析构则在真实返回 true 后由局部原 producer／ID／artifact 兜底，不能取消后来复用同路径／artifact 的任务。取消不删除仍运行 worker 或提前清翻译busy，实际双尾排空后才释放当前槽。

摘要解析／locale URL选择和 TS解压／XML／JSON／规则执行／输出均由各自 parentless worker 完成。每段 actual worker 自持原 native 输入，结果和 actual finished 双尾匹配后才能推进；主线程只接 URL／路径、发有界下载与原生终态。失败仍使用原未译元数据，不以部分译文或空目录替代原消费者。

有界 core 保合法随数据下降的递归定义，拒同一物理数据位置无进展的规则循环；数字 list key 修复为明确数字 context，不再借旧 toString 得空串。上述不认证语言作者、设备、真实性能、原CRC后的译文内容或StartPermit。见[核心](ComponentTranslationEngine.md)、[后台作业](ComponentMetadataTranslationWorker.md)、[协调器](ComponentInformationTranslation.md)。本轮仅源码与限定静读，未执行解析、线程、下载、文件业务IO、编译或测试；以下为历史。

## 528：唯一下载输入与准确终结

元数据实际 HTTP／MFTP 入口均改用独立 `downloadUnique`。每个原 FileRequest 在受理前 create 新 `QGCOwnedDownloadFile`；actual provider、原 request UUID、source／serial／phase 及准确原 owner.path 一起绑定。成功、错误和取消都必须由准确 provider 的独立三字段完成信号返回原 ID／原路径，不能拿同 URL、basename 或 legacy 全局 downloadComplete 清掉本次占用。

原同步 starting buffer 保留：真实 return=true 后才接纳中途终结；false 只完成本次未受理失败，不采同步成功。source 退休在 native inactive 已提交后只 cancelUnique 原 actual provider／ID，受理栈内退休则等真实返回后准确取消。传输自身持 owner 至关闭，取消不删除别人的文件或调用 legacy 共享 cancel。

Worker 新末参持原下载 owner 到实际线程结束／对象回收；cacheHit 明确空 owner，原 cache 不被领养。worker 两尾及原代次围栏保持。Translation 的 summary／metadata 原 artifact 也被原 TranslationRequest 持有，不能只存 QString；TS 输入、解压输出、译后 JSON 仍分属不同所有权。

新下载层的 32MiB／完整 IO 和唯一原件解决本机跨机 basename 覆盖，不认证远端设备、PRIMARY wire 响应、服务器、TLS、固件或当前性能；CRC 域／只读 fallback 和最终 StartPermit 边界不变。见[原输入所有权](../QGCOwnedDownloadFile.md)、[HTTP](../QGCFileDownload.md)、[缓存回退](../QGCCachedFileDownload.md)、[FTP](FTPManager.md)。本节以下保留历史，旧“底层 basename 尚未隔离”仅为526状态。

## 527 参数目录生命周期接线（源码静态收口）

本批在原来源退休（Manager自身析构除外）及真正新GENERAL请求前，遍历当时实际组件表的全部CompInfoParam，调用各自native resetMetadata。原GENERAL即使失败、为空或不再声明PARAMETER，也不能把旧JSON目录留作后来参数查询；额外component的旧lookup资格同样退休，不只清AUTOPILOT1。

reset只退休定义模板／fallback查询资格，不删除已交给实际Fact的独立定义副本，也不伪造已有Fact被更新。CompInfoParam与ParameterManager的真实消费者在同一批切换为Fact-owned copy；不能仅清原map后继续外借旧指针。旧模板及旧XML opaque的释放须按该模块实际所有权，不能借来源退休删其它plugin对象。

遍历冻结原组件表弱引用与原source，逐次reset后重核Manager QPointer与同source；不在同步析构／重入后继续清后来批次。该方法不要求source仍未退休，因为它同时服务准确旧来源的退休清理；它本身不发下载、写参数、飞行命令或赋予能力认证。

参数v1已知字段、候选安装及副本边界见[参数消费者](CompInfoParam.md)、[参数专用解析](ComponentParameterMetadata.md)。本批源码与限定静态交叉审阅已收口，工程场景全部未执行；以下526为此前文件字节接纳的历史边界，不因新参数接线自动升级为完整draft-schema、真实性认证或StartPermit。

## 526 范围与实际调用链

本批接通既有 `InitialConnectStateMachine → ComponentInformationManager → COMP_METADATA_TYPE_GENERAL / PARAMETER / EVENTS / ACTUATORS → CompInfo::setJson` 实际路径，不增加 UI、参数写入、网络命令、Fleet 能力认证或飞行许可。先尝试原 `COMPONENT_METADATA`，再按原流程尝试 deprecated `COMPONENT_INFORMATION`；GENERAL 目录决定其余类型是否受支持。新文件字节检查不是新的默认通过条件。

每个真正的新 GENERAL 批次在请求前调用 `CompInfoGeneral::resetMetadata()`，先退休上批目录及 URI；GENERAL 文件空、CRC 失败或 schema 拒绝不能再把旧 `_supportedTypes` 传播为本批目录。General 自身完整候选解析由其模块负责。其它 CompInfo 仍是原解析器；尤其参数定义接受域、旧 FactMetaData／本地 fallback、失败后的旧数据及半装行为不是本批全面重写或真实性认证。不能把 worker 成功当成所有参数定义 schema 已通过。

## 原批次、来源与完成回调

Manager 正常 parent 到实际 Vehicle；Vehicle 销毁会进入 Manager／子对象清理，而不是留下无主的整条下载链。持久 Vehicle、LinkManager、PRIMARY、CompInfo 与可选回调所有者均由 QPointer 追踪。每批冻结非回绕 serial、实际 Vehicle ID／default component／固件类型／本地 version source epoch、原 PRIMARY weak control block 与实际 QObject 地址。

PRIMARY 切换、连接／断开、通信丢失状态变化、全部链接移除、固件／version source epoch 变化及 Vehicle／callbackOwner 销毁会退休原批次；同值 A→B→A 不复活。续作检查同时核原批次、原 provider 与 weak owner／实际 PRIMARY 指针。弱引用临时提升后的释放也可能触发生命周期回调，所以释放后再核 QPointer 与完整原内存身份。此围栏不调用 Link 虚函数或 OS 探针；入场 `isConnected`／`isLogReplay` 则明确是虚调用边界，逐次核原来源。它不要求 `isInitialConnectComplete`、参数已就绪或新鲜版本内容，避免初始连接自身被未完成条件锁住。

`requestAllComponentInformation(fn, data, QObject* callbackOwner=nullptr)` 保留原两参数兼容，实际初始连接 caller 显式传原 connect machine。内部每次 `Vehicle::requestMessage` 另有一个不可变 QObject callback context，保存原 machine／CompInfo／serial／state；该 context 同时作为 opaque data 与 callbackOwner，不借用可被下一批复用的 raw machine。退休先清本次槽，已排队旧响应即使还未实际销毁也不能推进新 phase。

正常总完成先提交 `_active=false`、清 fn／data／owner 并退入场门，再调用冻结回调；新合法批次明确 `_stateIndex=-1`。回调中重入、销毁或开始新批次后，原 outer `requestAll`／`advance` 只在原 serial／source 仍同且存活时发布进度，不抹新回调。legacy 空 callbackOwner 只保持旧 opaque data 兼容，不证明 data 的外部寿命。

元数据是原初始连接中的可选步骤。运行来源退休或入场没有可用来源时，可对仍属本 Manager 的实际活 Vehicle／原 callbackOwner **一次**终结旧步骤，先清旧槽、日志说明未采用后来来源；这不是文件下载、CRC、schema 或 setJson 成功回执。原来源续作与任何外部 terminal 回调前均核实际 `Vehicle::componentInformationCallbackAllowed()`；Vehicle 析构、closeVehicle、prepareDelete 的第一句先设置本通道专用退休标志，随后才有其它回调，防止部分析构中 QPointer 尚活却继续启动文件 phase／setJson 或 InitialConnect。getter 同时核实际 owner／GUI 线程、原 removing 与 all-links-removed 门，不要求初始连接完成。Manager 析构、owner 已死均不回调；后续连接步骤仍各自执行其原真实来源门。

## 冻结 URI、CRC 与 raw-v1 缓存

固定 MAVLink URI 数组须在其长度内存在 NUL；无 NUL 的声明拒绝，不把整块数组当正常 URI。实际文件请求复制原 URI／CRC／类型／phase 后才跨外调。URI 最多2048 UTF16，拒原始控制字符／NUL、相对／本地 `file` 等协议，只允许 `mftp`、`http`、`https`。HTTP(S)还要求旧 QGCFileDownload 实际识别的精确小写 `http://`／`https://` 前缀、严格合法非相对 URL、有 host、无 userInfo；不能仅 lower scheme 后让 `HTTP://` 意外走旧本地文件分支。MFTP 保留原 FTPManager 的 `[;comp=N]` 前缀语法，不把它误作 RFC IPv6 authority 拒绝；移除 `mftp://` 后为其原远端文件 path，须非空、无嵌套 `://`／query／fragment。这个协议范围限制不认证远端站点或来源。

`_requestFile` 明确接收 `expectedCrc`；压缩属性从冻结 URI 的 HTTP path／MFTP 原远端 path 是否以 `.xz`／`.lzma` 结尾决定，不从临时名或 `.cache` 后缀猜。实际解码器支持的容器沿 worker 合同，不能仅凭 `.lzma` 后缀宣称支持所有裸 LZMA 格式。

新标签为 `raw-v1_` 加原 `_getFileCacheTag` 的 CRC／类型／translation 形式。绝不访问旧无 namespace 的解压 `.cache`。所有命中与新下载均先进入同一 artifact worker；缓存名字、存在与命中不是字节已验证或来源认证。

CRC 比较域只有三种：

- `transport`：声明 CRC 与原始传输快照字节一致，优先采用。
- `decoded-px4-general`：只在原实际 PX4 固件且 GENERAL 类型的显式兼容分支，压缩传输 CRC 不同但独立解码 JSON CRC 一致；不能扩大到参数、其它固件、其它类型或翻译。
- `not-compared`：无 CRC 声明，仍走有界快照，供原旧显示／解析，不冒称匹配、认证或执行证据。

缓存命中 worker 失败时，本次文件 phase 最多调用一次 `invalidate(准确 raw-v1 tag)`。只有准确数据文件与 metadata 文件两次删除都确认成功，才重新下载冻结原 URI 一次；任一失败／已缺都不假推删除成功，不循环。原 primary／phase 已退休时不 invalidation 或重下。配置中的原 fallback phase 仍可按其独立声明执行，不把坏 prefix 当成功。

只有新下载且存在声明 CRC 才要求第三份 `cacheCopyPath`。Cache::insert 可以移动或删除此候选，但其返回的旧缓存路径不作本批输入；本批始终消费原 shared artifact 的 JSON 快照。无 CRC 不插入此 raw-v1 文件缓存。既有 HTTP downloader 自己的网络缓存行为仍存在；其命中同样必须通过新 worker，不能冒称完全禁用了所有其它缓存。

## Worker 双终结与不可变工件

每次文件请求同时只有一个实际 parentless `ComponentMetadataArtifactWorker`，冻结唯一 request ID；cache 失败后的重下不会复用旧 worker 对象。每文件最多两次实际 worker，另有私有不复用的 worker generation，三种终结闭包都核原 generation；旧 worker 的迟到 destroyed 不能把新重试尚未齐全的 flags 当作旧缺尾。`resultReady(id, ok, error)` 与实际 `QThread::finished` 均属于同一实际 worker／ID／generation且双尾到齐，才能读 artifact、处理 CRC 失败、失效缓存或推进。任一尾单独不能取结果。实际 worker 被销毁但缺尾属于该可选步骤的失败退休，不能推断是 CRC 失败并重下；不在 GUI wait／terminate／删除运行线程。

来源变更／Manager 销毁只对本 worker `requestCancel()`；actual finished 后安排删除 parentless worker，未被采用的 shared artifact 随最后引用释放。请求状态不在 live worker／原下载／翻译仍未终结时重置成下一批。原 FTP／HTTP 没有可核对的 transfer-ID cancel API，因此本 Manager 不再以旧 slow-progress 或失源理由盲调全局 `FTPManager::cancel()`；保留自己的终结 listener 收尾，旧结果不跨源应用。这不保证已发请求已撤回，原共享传输 API 也不是经认证的 wire 归属证明。

同步 `download()`／翻译回调先暂存本次值，等对应方法真实受理返回后才采用；false 不因中途收到其它终结信号而被当作已受理。原 listener 在清 busy／槽后精确断开，续作再核 QPointer、原 source／phase／serial。连接、下载、缓存 access／insert／invalidate、worker 启动、翻译与虚 `setJson` 后均不凭最新对象或最新字段续作。

工件的十个 const 字段及字节检查细节见 [ComponentMetadataArtifactWorker.md](ComponentMetadataArtifactWorker.md)。raw／JSON 各32 MiB、XZ dictionary64 MiB、流式64 KiB；可选第三 raw 缓存副本32 MiB，三份最多96 MiB临时磁盘，未把解码内存总量声称为64 MiB，也没有绝对 IO／CPU 时限。旧本地下载输入文件不是工件拥有的唯一临时文件，不能由 artifact 析构盲删它。

`rawSnapshotPath` 与 `jsonSnapshotPath` 在 shared pointer 存续期间不可 rename／move／remove。Manager 原 metadata／translation summary 各持有实际 shared artifact，并在原 parser／translation 调用栈跨通知时另持局部副本。传给虚 `setJson` 的文件名也是冻结值副本，不传一个可能随 Manager 销毁而悬空的成员 QString 引用。最后 shared owner 负责其本次唯一 raw／JSON／可选候选路径的尽力清理。

## 翻译与已知边界

Translation summary 的 JSON 是原 worker 字节工件；随后 locale TS 下载、TS 解析与译后 JSON 是独立显示变换，**不在原声明 CRC 的比较域中**，不被 raw SHA 或 JSON SHA 认证。TS／译文改为各自实际唯一临时输出以免多机共享固定文件。Translation 本批另给输入完整读取／译后输出32 MiB及 TS XZ 输入／输出32 MiB、字典64 MiB边界；不扩大旧两参数 LZMA helper，也不把这些文件字节界限称为所有 JSON 引用图／递归算法的深度、CPU或总临时内存上界。其实际变更见 [ComponentInformationTranslation.md](ComponentInformationTranslation.md)。

译文返回时先冻结原唯一输出路径到纯 native RAII 责任、先清准确 busy／listener，再核旧 request／source／phase；即使该通知内 Manager 销毁或来源退休，也只清其原返回的唯一文件。成功译文保持到 setJson 结束；错误继续使用原 JSON。没有按新 source 或最新 filename 猜删路径，也不将译文插入原始 CRC 缓存。

CRC32、SHA、缓存及本地来源围栏不证明文件签名、远端身份、真实硬件构型、真实性能、参数单位／范围正确、任务归属、StartPermit 或放飞。没有新增 public 可导入认证 Map、批准、参数写入或飞行命令。

## 未执行与工程师待验证

本批仅 apply_patch 和直接静读；未执行任何下载、缓存、worker、文件业务 IO、解压／CRC／SHA、项目实现、parser/helper、编译、测试、硬件、刷机、PR 或打包。需工程师实际覆盖：正常首次初始连接和同 Manager 重批；COMPONENT_METADATA／deprecated fallback；raw／decoded GENERAL／无 CRC 三域；新 namespace 不读旧缓存；命中好／坏、两删除部分失败、一次重下与原 fallback；URI 无 NUL／file／坏 scheme；URI 压缩后缀与缓存名无关；所有双尾顺序与缺尾；source／PRIMARY／版本／通信 A→B→A；owner 销毁／Vehicle 部分析构不回调；terminal callback 重入新批、旧尾不抹新 callback；虚 setJson 销毁／重入；cache 候选被移动仍能解析 JSON；多机翻译唯一文件及退休迟回清理；旧 FTP 已发不可撤回与不 cancel 他人请求；尺寸／字典上限、真实 Qt／SDK 线程、临时文件清理失败。以上全未执行，不能称运行通过或完整可信能力工作流完成。

# ComponentInformationTranslation：每次调用独有的翻译文件

## 532：两个生产阶段共用有界工作调度

Summary与Translate正常启动均只调用worker的`schedule()`；旧的未启动排空入口删除，
未受理对象直接准确清理，不再用`requestCancel()+start()`另起线程。该编排仍一次仅一个请求／至多两个
阶段对象；各车、artifact和翻译阶段的进程级并发／排队由
[ComponentMetadataWorkScheduler](ComponentMetadataWorkScheduler.md)统一约束，输入native
shared owner在queued与running期间都不提前释放。
实际为两类worker合计最多2个running、32个queued，排队单调120秒上限；100ms检查及
获槽前重核，不把该期限当物理抢占或整条Summary／下载／Translate统一总期。

所有连接先装再schedule。返回false是从未受理，无该worker自身neverStarted；原caller
沿独立拒受理路径清准确worker并deleteLater未启动对象，终结原翻译请求。返回true后
queued取消／超期发独立`neverStarted(id,reason)`一次并由worker自行deleteLater；既不
伪造resultReady也不伪造finished，更不调用getSummaryUrl/takeOutputFile。准确
worker地址／requestId／generation／stage全核后，只清原pin并按失败终结、解除原busy。
拒受理或queued取消因此不会把Summary/Translate的busy留到一个永不发生的finished。

neverStarted在schedule受理栈内到达时只缓冲本次plain reason；真实返回之后才接纳。
排队requestCancel是sticky，原busy保持到实际never-started处理，不声称同步撤回。
running取消仍只requestCancel，busy直到原实际result＋finished两端齐全；不能以取消
或QObject销毁当成功输出。原独立finished→deleteLater保留；早退、owner销毁、迟回
只清／取消原实际对象，不删除运行线程，不启动额外drain线程，不自动重试或翻译。

终态仍先清原槽再发对应native/legacy信号；实际unique输出与输入owner责任不变，
不发参数写入或飞行命令。仅源码和静读，未执行排队/线程/parser/下载/IO/编译/测试。
工程需验证queued满／120秒排队超期／cancel后解除busy、真实running双尾、两阶段交接、
旧destroyed与原ID/phase隔离、终态回调中启动后来请求，以及所有原输入／唯一输出清理。

## 529 当前生产链：两段后台 worker

新增原生 `downloadAndTranslateArtifacts(requestId,summaryArtifact,metadataArtifact,maxCacheAgeSec)`
及 `cancelArtifacts(requestId,summaryArtifact,metadataArtifact)`。实际 Manager 传原生不可变
artifact，而不是只借其文件名；native 请求仅发独立
`artifactTranslationComplete(requestId,uniqueOutput,error)`，不发 legacy `downloadComplete`。
cancel 必须原 requestId 和两枚实际 shared owner 全部匹配，不能用同文件名、后来同一
artifact pair 或共享 latest 领取消资格。调用者必须每次生成不复用的 requestId。

生产线程只编排：实际 Summary worker 完成 summary 有界读取/JSON/locale URL 选择，
主线程收到该实际 worker 的 resultReady 与 finished 两端后，才创建本次原生 TS lease
并调用原 Cached.downloadUnique；下载结束再启动 Translate worker。
GUI 不再同步读取 summary、解压 TS、解析 XML/JSON 或递归翻译/写译文。
最后明确调用 `CompInfo::setJson` 的具体消费者接受域不因此认证或改写。

每对象只有一个编排请求，没有本对象额外的请求队列；532的进程级worker排队独立，
busy 覆盖 Summary、下载和 Translate。
每请求最多两个一次性 parentless worker，第二段只在第一段实际 finished 后启动。
编排核原 generation、阶段、actual worker、阶段代次、requestId 和 result/finished；
任一端单独到达不可读结果/安装输出。独立 finished→deleteLater 不以 Manager/
编排对象存活为条件，不 wait、terminate 或删除仍运行线程。缺终结只报失败，
不将 QObject 销毁当成功完成。

实际 worker 自己持 summary/metadata artifact；Translate worker 还持原 TS lease。
因此 Manager/Translation 析构或清成员不会提前删除后台正在读取的输入。
取消先 sticky 撤权：worker 阶段只 requestCancel，原 busy 保持到双终结排空；
下载准备栈先记撤权，实际受理返回后仅向原 provider/id 精确取消，包含 owner
已经销毁的晚回尾。已下载 writer 关闭、输入仍有 owner 才能进入后台读取。

每 worker 自己启动一个 120 秒单调 clock；Translate 的同一 clock 覆盖 XZ 和 engine，
不能在解压后重开预算。Summary、HTTP 和 Translate 各自有自己的阶段总期，并非
整个流水线只有一个 120 秒。取消/期限在可检查边界保守失败，不声称物理抢占 Qt
parser/OS/解压调用。engine 的深度/节点/XML/字符串/引用规则见其独立合同；
TS XZ 仍为输入/输出 32 MiB、字典 64 MiB，后缀来自本次原 URL，而不是缓存文件名。

公开旧 `downloadAndTranslate(summaryPath,metadataPath,maxAge)` 签名和旧信号保留：
路径为借用，不清输入；`true` 现在表示处理受理，不表示 summary 已合法或网络已起调。
合法受理后 summary 错误可异步上报；`false` 无该次自身完成。旧调用者负责使借用路径
活到异步完成，生产 Manager 不再使用此弱所有权入口。
公开同步 `translateJsonUsingTS` 也保留，直接复用同一 bounded engine，支持原 qrc
借用输入，不下载、不删除输入、不禁用翻译；成功唯一输出仍由调用者清理。
同步入口须 owner thread，且与本对象异步 busy/销毁/其它同步调用互斥；不会在生产后台
请求活动时再另起一份 GUI 解析。原实际 unittest 的无活动请求 qrc 调用不受此门影响。

输出仅在 worker 成功且实际 finished 后由 takeOutputFile 移交一次；未领取输出归
worker 析构尽力删除。编排拒绝/取消时清本次实际输出。终态先清原槽，再发对应 mode
的信号；emit 后不再访问 this/新请求，原 orphan guard 只清销毁时未交付的原唯一文件。
这是本地字节/对象生命周期，不认证 locale 作者、设备、签名、缓存、构型或性能，
不发参数写入、执行许可或飞行命令。

以下 528/526 是历史实现说明；其中生产主线程同步读取/解压/递归、固定 false
局部取消与“递归图尚无界”由 529 当前 worker/engine 合同替代，不能当作本次状态。
本次只编辑源码/直接静读，未运行 parser/解压/下载/项目/文件业务 IO/编译/测试。

## 528：TS 输入也独有，不能只隔离译后输出

原公开翻译签名保留，实际 TS 下载改为新 Cached.downloadUnique。每次原生 create 新输入 owner，冻结实际 downloader、原 generation／UUID／URL及准确 owner.path；新独立完成信号必须 ID／path 都匹配，不借 legacy 同 URL 信号归属。同步终结先缓冲，只有真实 return=true 才接纳；false 无本对象完成，留原 Manager 处理未开始失败。

原对象成员、传输及实际处理栈各自持输入 owner，解压／XML读取退栈前不能释放原件；析构先退休本对象占用与连接，再只 cancelUnique 原 ID。缓存命中、summary／原 JSON、TS 下载原件、TS解压输出和译后 JSON 全部区分。Manager 原 TranslationRequest 另持两输入 artifact，异步与同步旧栈不能因成员清理提前删除它们。

新 HTTP／Cached 的完整 IO、32MiB、120秒任务期限、有限 redirect 和一次 cache-only 回退按其各自合同；不是 TLS、设备、翻译作者认证或递归算法总CPU预算。以下526“类不删除下载输入／底层 basename 未隔离”是历史：当前只有原生 fresh owner 的原件会由最后共享所有者尽力清理，借用输入／缓存仍不删除。见[输入所有权](../QGCOwnedDownloadFile.md)、[新缓存链](../QGCCachedFileDownload.md)。全部场景尚未运行或测试。

## 526 实际用途与 API

保留 `downloadAndTranslate(summaryJsonFile,toTranslateJsonFile,maxCacheAgeSec)`、`translateJsonUsingTS(toTranslateJsonFile,tsFile)` 和 `downloadComplete(translatedJsonTempFile,errorMsg)`。真实 ComponentInformationManager 仍下载当前语言的 TS 并把译后 JSON 交给原 CompInfo，不禁用本地化，也不新建权限、参数写入或飞行接口。公开签名不变；头文件只增加本对象的私有占用、原 downloader QPointer、连接、URL和非回绕代次。

每个 Translation 对象只允许一项异步翻译，解析／写译文期间也拒绝重入。达到代次最大值后拒绝新异步请求，不回绕。新连接闭包冻结原 downloader、代次和 URL，迟到或不同 URL 的信号不能改写后来调用的输入。连接／断开／下载返回后检查实际 QPointer 和原代次；终态先清本次占用和输入，再把完成信号作为最后外调，不能在 emit 后清掉接收者同步启动的新请求。不调用共享下载器的无 token cancel。

此处来源检查只表示本地原对象／请求连续性，不证明 FTP 原 PRIMARY、HTTP响应身份、缓存新鲜、翻译作者或真实设备构型。Manager 另持自己的原批次／来源与实际回调围栏，来源退休后不能继续安装译文。

## 两份唯一输出与移交

旧的全局 `qgc_translation_file_decompressed.ts` 和 `qgc_translated_metadata.json` 不再使用。每次真正需要解压时创建唯一 `QTemporaryFile` TS 输出；它只属于当前调用，任何成功／失败／销毁分支均由局部 RAII 尝试清理。压缩判定来自当前原 URL 的 path 后缀 `.xz`／`.lzma`，不从缓存或下载后本地名字推断。

TS 调用新增 bounded XZ overload：输入和输出各32 MiB，字典最多64 MiB，局部原子取消值固定 false。本同步调用不承诺运行中可取消；拒超过预算、负 read、截断、尾随／拼接、unsupported check、坏压缩流和读写失败。后缀 `.lzma` 不表示支持另一种裸 LZMA 容器；其内容仍须是该 helper 支持的 XZ。

译后 JSON 另建唯一 `QTemporaryFile`。只有完整输出不超过32 MiB、每次 write 长度精确、flush成功、实际大小一致、close无错误且原对象仍存活，才 `setAutoRemove(false)` 返回其实际名字。失败由本地 RAII 清理；成功名字由调用者接管。异步完成尾若原对象／来源已失效，只清本次实际译后输出，不安装给后来来源。emit 前还冻结仅局部的 QPointer／路径清理 guard：若较早接收者同步销毁 parent／Translation，导致 Manager 消费槽不再运行，局部析构只尝试清本次实际输出，emit 后不访问 this 或新请求。Manager 须清它实际接收的这份唯一输出，即使接收时原来源已退休或消费对象已销毁。

类自身不删除输入 TS、JSON、网络下载文件或原缓存。没有原固定路径之间的覆盖、移动或共享 QFile 跨对象行为。临时文件的 remove 为尽力清理，不是删除／崩溃耐久性认证。

## 完整读取、URI 与解析责任

摘要 JSON、原待译 JSON和 TS 在解析前均要求实际可读普通非 symlink 文件、大小不超过32 MiB。64 KiB 读取明确检查负值、预算、正常 EOF、QFile error、实际字节数／大小、canonical路径及 mtime 前后连续性，close亦须无错误。不是 `readAll()` 后把错误／前缀当完整数据。JSON须正常解析为object；TS在整个 XML遍历后再检查解析错误，不发布坏尾的成功前缀。

摘要选当前系统 locale 的 `url` 必须是真实 String、1至4096 UTF16 字符，不含原始控制字符，使用 QGCFileDownload 实际支持的 `http://`／`https://` 前缀；QUrl还须严格合法、有host、非relative且userInfo为空。拒 file、本地路径、mftp、其它协议及带凭据URL；不把这一语法门称为 TLS、服务器身份或网络证书已认证。仍沿原共享下载器的缓存／网络行为，本类不擅自重试下载或刷新参数。

原 `$defs`／`$ref`、items／list／translate／translate-global 的递归翻译算法不重写。本批的文件／输出字节预算不等于已证明整张 JSON／引用图的深度、节点、递归CPU或全部临时内存上界；循环／过深定义仍是后续需独立处理的已知边界。General目录本批另行整份校验，参数等具体消费者仍独立负责完整定义 schema，译文不是能力、性能、签名或 StartPermit。

## 未执行交接

只做 apply_patch 和源码直接静读，未运行翻译、XML／JSON解析器、下载、解压、文件 IO、项目／helper、编译或测试。工程师需验证：两车同语言同时翻译及相同下载basename不会共享输出；同对象重入拒绝、旧 queued 回调／同URL后来代次、connect／disconnect／download通知内销毁；Manager退休／销毁后的实际唯一译文清理；合法 plain／XZ、URL后缀与缓存扩展不同、裸LZMA拒绝；32 MiB边界／超界、负读／短写／flush／close失败、坏XML尾、空翻译；http/https及本地协议／凭据拒绝；成功移交后原输入保留、终态 emit 内启动新请求。上述均未执行，不是运行或跨机可靠性验收。

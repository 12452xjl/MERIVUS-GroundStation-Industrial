# QGCCachedFileDownload：528 原生唯一缓存下载

## 1. 接口与所有权

新增原生 `downloadUnique(requestId, url, output, maxCacheAgeSec)`、
`cancelUnique(requestId)` 和独立
`uniqueDownloadComplete(requestId, localFile, errorMsg)`；不发旧完成/进度信号。
原 `download(url, maxCacheAgeSec)` 与其旧信号、旧缓存回退接受域保留。
唯一请求活动期间拒绝旧请求，底层有旧活动 reply 时新请求不受理。

复用原实例的同一个 QGCFileDownload、同一个 QNetworkDiskCache；不创建新的
QNetworkAccessManager 来转移 diskCache 的 QObject 所有权。新任务以弱 QObject
pin 绑定这两个实际对象，私有持原生唯一 output lease。lease 规则与
[QGCFileDownload](QGCFileDownload.md) 一致：原生调用者每请求新建、不得并发复用，
自己保留到实际读取结束。

主线程/对象线程、非空 requestId（最多 128 字符且无 NUL）、非负 maxCacheAgeSec
以及底层 HTTP(S)/空普通输出等新入口检查不改变旧调用者的接受域。
返回 false 不发该次请求自身完成；实际受理后的同步完成可以先于 true 返回。
所有实际终态 localFile 都是准确原 output.path，失败/取消也不改为空或别的文件。

## 2. 缓存策略与一次回退

沿用原本 User attribute 本地 creationTime 年龄政策：有该 metadata 且未过期
使用 PreferCache；已过期先 AlwaysNetwork；其它情形使用原默认请求政策。
这只是 Qt/GCS 缓存时间，不是机载测量时间、下载来源认证或 CRC 核验。

**新链的唯一差异：**仅因过期而强制网络的那次尝试失败时，可以回退一次
AlwaysCache；不是旧实现的默认模式再联网。回退不改变原外层 requestId/URL/
lease，不再作网络重试，不因取消、总期届满或非此策略失败自动回退。
外层 120 秒单调总期覆盖 metadata 选择、两次底层尝试和全部 redirect，
回退不会得到新的外层 120 秒。

底层原尝试终态保证其 writer 已关闭。缓存包装才可打开同一私有 output 文件，
准确截到 0、核 flush/close/零大小，再调用要求空文件的新 HTTP 入口。
不删除或接收任意 caller path，不在仍有 writer 时 reset，不把失败 prefix
冒充成功结果。实际连接/phase/代次均绑定原上下文，旧回执不得续用后来任务。

新链仅成功且非回退时补缺失的 User creationTime；不在错误上写新时间，
也不将 stale cache fallback 的 creationTime 刷为现在。旧链原逻辑保持。
回退仅保证 Qt AlwaysCache 政策，不认证 diskCache 内容；后续 artifact worker
仍须核自己的原声明 CRC/大小/独立快照。

## 3. 生命周期与范围

准备栈中的同步底层回执先存本次局部结果，底层返回 true 才认领并发布。
原生 false 不得被包装成一次自身完成。cancelUnique 只撤准确原 requestId，
在准备窗口先 sticky 撤权，再交原底层任务；不会取消别人的后来请求。
通知前关闭/取消实际 transport，清准确原槽，终态 emit 后不续访 this。
两个 QObject 被销毁时只清该原任务；不要求已销毁包装对象发完成。
期限 timer 采用无 parent 候选、局部弱 pin/清理围栏及 setParent 后原任务尾核，
不在 ChildAdded 通知后直接续用裸 timer；未受理失败清准确原入场资源。

本批不是任意原生恶意调用/文件替换隔离、网络身份、签名、可信性能、执行许可、
飞行权限、缓存永久持久性或当前固件兼容认证。其它旧下载 caller 不自动迁移。
下载文件的删除由原共享 lease 管理，缓存文件仍由原 diskCache 管理。

本批仅静态源码编辑/阅读，未执行下载、文件/网络 IO、解析器、项目、编译或测试。
后续工程需覆盖 fresh/expired/missing metadata、强制网络失败的单次 cache-only
回退、回退缺失/损坏、原输出清零失败、两级同步回执、取消前尚未有 reply、
回退和原期限相撞、同 basename 多实例、通知内销毁/重入、底层实际销毁、
旧/new 混用拒绝及所有 legacy-only caller 行为。

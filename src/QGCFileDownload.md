# QGCFileDownload：528 原生唯一输出路径

## 1. 本批边界

新增原生 `downloadUnique(requestId, url, output, attributes={})`、
`cancelUnique(requestId)` 和独立信号
`uniqueDownloadComplete(requestId, localFile, errorMsg)`。
旧 `download`、`downloadComplete`、`downloadProgress` 和原本支持本地路径的
接受域保留；不把新 HTTP 大小/时限政策施加给旧固件下载等调用者。
唯一任务活动时拒绝旧入口，旧 reply 或旧入口准备栈活动时拒绝新入口。
旧接口内部的并发共享状态、basename 临时文件及旧重定向机制不是本批认证范围。

新接口仅在应用主线程且对象所属线程调用，不是 QML invokable。
requestId 为非空、无 NUL、最多 128 UTF-16 字符的原生关联值；私有任务同时
绑定不可回绕代次、原 URL、原 attributes、实际 reply 和输出 lease。
requestId 本身不是权限或网络关联证明。

## 2. 输出与接受结果

`output` 只能来自实际原生 `QGCOwnedDownloadFile::create`，每个调用者请求
新建一个、不得并发复用或改写路径。入场核对应路径为已存在的空普通文件、
绝对路径且非符号链接，然后打开该准确路径，不从 URL basename 生成输出。
transport 私有任务及调用者均持 shared lease；transport 在成功、失败、取消
通知前关闭 writer，调用者须保留 lease 到自己实际的同步/后台读取完成。
这不是任意 C++ 调用者/外部进程篡改路径的对象身份认证，也没有通用 path-adopt API。

返回 `false` 表示未受理，不为该次调用发自身完成信号。实际 reply 受理后，
对象存活时成功、失败、取消均走一次独立终态，`localFile` 始终是原 output.path，
`errorMsg` 非空即不可消费为成功完整文件。同步完成可以在返回 `true` 前发信号；
消费者必须先冻结原请求、lease 与身份，并缓冲自己的准备栈回执。
析构仅撤销实际原 reply、关闭 writer、释放 lease，不要求已销毁对象发终态。

## 3. HTTP 与预算

- 仅严格 HTTP(S) URL，最多 4096 字符，无控制/NUL、无凭据、host 非空。
  本地路径、file URL 和其它 scheme 不属于新接口。
- 强制手动重定向，最多 8 次，目标仍须满足同一 HTTP(S) 域。
  原 requestId、URL、attributes、输出和单一总期限不随 redirect 重基。
- 每次 read 最多 64 KiB，reply read buffer 设 64 KiB。实际交付实体字节累计
  最多 32 MiB（包含被丢弃的 redirect body），声明 Content-Length 必须为
  非负十进制整数且不超过 32 MiB。负 read、短 write、非法长度、非 2xx 最终状态、
  未完整 EOF、flush/close 错误均失败，不发布可用 prefix。
- 最终无 Content-Encoding 或 identity 时，存在的 Content-Length 必须等于
  实际写入字节数。非 identity 可能被 Qt 透明解码，声明和交付数据各自有界，
  不虚称两者是同一字节域；不宣称 TLS/HTTP framing 等全部网络内存的硬界。
- 单任务 120 秒总期，QTimer 加单调 elapsed 检查覆盖 redirect 与每块读取；
  不自动重试。事件循环阻塞时不能承诺物理实时中断。

cancel 只撤销准确原活动 requestId，不取消其它旧任务或后来新任务。
旧 reply 的销毁只可终结仍绑定该原实际 reply 的任务；旧 redirect reply
迟到销毁不能归因到新 reply。readyRead 中的嵌套 finished 延迟至原 pump 退出，
避免把尚未计数/写完的数据当作完整终态。
期限 timer 先无 parent 创建并以局部弱 pin/清理围栏持有，setParent 后核
原 owner/任务/实际 timer 才继续配置；ChildAdded 中销毁 owner/timer 不续用裸指针。
未受理的入场失败关闭文件并撤准确原槽，无 parent 的候选由局部清理释放。

## 4. 信任与待工程验证

本次完成只证明本地 transport 的有界交付/文件关闭结果。HTTP 缓存、URI、
SHA/CRC 后续结果或 TLS 兼容策略都不是签名、设备身份、可信构型、性能实测、
执行许可或飞行授权。原 `setIgnoreSSLErrorsIfNeeded` 兼容行为保留，不能声称
所有环境均验证 TLS peer。JSON/schema 和声明 CRC 由后续独立消费者处理。

本批仅源码编辑和静态阅读；未下载、未运行文件 IO/网络/解析器/项目/测试或编译。
后续需验证：多机同 basename、同步/异步缓存完成、redirect 旧销毁、redirect
循环/跨 scheme、32 MiB 边界与短读写、透明编码长度域、取消准备窗口、120 秒总期、
通知内销毁/新请求、消费者后台读取期间 lease、混用旧/新入口及旧调用者兼容。

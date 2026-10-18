# ComponentMetadataTranslationWorker：529 两阶段原生翻译

## 532 当前：两段共用进程 FIFO

原生新增 `bool schedule()` 及独立 `neverStarted(const QString& requestId,const QString& reason)`，隐藏正常 `QThread::start`。Summary与Translate各用自己的真实worker/阶段，经 [ComponentMetadataWorkScheduler](ComponentMetadataWorkScheduler.md) 共用2 running／32 queued；排队原120秒不续，获槽前再次检查。schedule=false无受理、无假终态，caller回收准确未启动对象；true后排队取消／到期只发neverStarted一次再deleteLater，不跑空取消线程或伪造双尾。running取消仅原atomic，必须实际finished才还槽，原result+finished取URL／输出不变；scheduler不先删除running worker而丢客户端queued消费。worker构造器的首个finished latch只写原生shared atomic。自身120秒运行clock仍从实际run开始，包含Translate解压而不含等待队列；不能据队列/运行时钟强删活线程。未运行、预览或编译测试。

## 原生 API

parentless 一次性 QThread，构造 `ComponentMetadataTranslationWorker(Phase,const Inputs&)`；
Phase 为 Summary 或 Translate。Inputs 冻结 id、summaryPath、metadataPath、tsPath、
locale、compressed，以及原生 summaryOwner、metadataOwner、tsOwner。
`requestCancel()` 仅设原子撤销位；`resultReady(id,ok,error)` 与 actual finished
是两个独立事实。`getSummaryUrl()` 返回小字符串；`takeOutputFile()` 只在实际
finished 后由原编排一次领取唯一输出。未领取文件归 worker 析构尽力清理。

Inputs 是纯值/原生 RAII shared owner，没有 Vehicle、Manager、QObject、QFile 或
权限对象跨线程。生产两枚 artifact 的路径必须准确等于各自 jsonSnapshotPath；
Translate 阶段要求实际 TS lease 与 tsPath 一致。兼容字符串入口的 summary/JSON
可以借用，包括 qrc；这不是路径接管，worker 不删除输入/缓存。

## 执行与预算

Summary 仅调用同一 ComponentTranslationEngine.resolveSummary：完整有界读摘要、
解析和选择冻结 locale 的合法 HTTP(S) URL；它不做下载、不读 GUI 状态。
Translate 使用冻结原 URL 所决定的 compressed 位。需要时创建本 worker 唯一
TS 临时输出，核创建/flush/close，再调用 bounded XZ：输入和输出各 32 MiB、字典
最多 64 MiB。`.lzma` 后缀不表示接受裸 LZMA；仍由 XZ helper 的实际接受域决定。
解压局部临时文件始终 RAII 清理。

每次 run 从最早执行开始启动 120 秒单调 clock。Translate 将同一 clock 交给
engine.translate，不在解压后重置。解压内部按其原接口检查原子取消，期限在该
调用返回及 engine 可检查边界核验；无法保证在阻塞 IO/Qt parser 内物理准时中断。
engine 负责有界 XML、JSON/引用图与完整唯一译文写出，成功输出不是原 CRC 字节域。

结果安装前再核取消与期限；失败/cancel 不发布 usable output。worker 成功后若
编排已退休，原输出仍由未 take 的 worker 或领取后的局部清理负责。
所有输出均独立唯一文件，既不移动原输入，也不把共享缓存路径当作新临时原件。

## 双终结与生命周期

Translation 编排将 actual worker、requestId、阶段、非回绕请求 generation 和
本请求最多两次的 worker generation 一起冻结。结果与 finished 都匹配才消费。
worker 独立安装 finished→deleteLater，Manager/Translation 析构只 requestCancel，
不 wait/terminate/删除运行线程。Inputs 自己持 owner 到该 worker 退栈/销毁，不能
仅靠 GUI 成员或 queued lambda 保原输入寿命。

原生 artifacts 请求独立 3 String 信号；旧借用路径请求仍发旧 2 String 信号。
旧 caller、旧 queued 结果、同 artifact pair 后来的不同 requestId 不能相互领养。
此关系只证明地面站原对象/本地字节连续性，不证明机载身份、网络/翻译作者、签名、
性能、可信构型、StartPermit 或已执行飞行。

## 未执行场景

本次仅源码编辑和直接静读，未运行线程、parser/XML/JSON、XZ、下载、业务 IO、
项目、helper、编译或测试。需工程师验证：Summary/Translate 各阶段销毁或取消；
真实 result/finished 两种到达序及缺端；同 owner pair 新 requestId 与旧 cancel；
同步下载完成/false/晚 true 后精确取消；两机同语言/同 basename；TS XZ plain、
坏尾/超32 MiB/字典超64 MiB；引擎深度/循环/节点/Unicode/时间预算；成功输出
take 一次与未 take 清理；旧 qrc 同步入口和 legacy 借用路径入口兼容。

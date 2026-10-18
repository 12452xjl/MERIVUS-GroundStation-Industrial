# ComponentMetadataWorkScheduler — 532 进程级元数据后台准入

本模块仅管理实际 `ComponentMetadataArtifactWorker` 与 `ComponentMetadataTranslationWorker` 的线程启动预算，非网络／文件／设备／账号或飞行调度器。没有 QML、公开 Map 授权、参数请求或新网络命令。进程内 GUI 线程 singleton 归实际 QCoreApplication 所有；没有 app、关机、错误线程、已用 worker、异常线程终结或队满均保守拒绝。

## 固定预算与公平性

实际运行槽最多 2，FIFO 等待队列最多 32；同一实际 worker 只尝试 schedule 一次。准入不立即运行或发终态，100ms 有界巡检处理取消／到期及空槽，队列时限是原受理时刻起 120000ms 单调时间。获槽前再次核原取消位和 elapsed，GUI 延迟不会令已超期任务起跑；排队不重开或续期。队列项删除后才通知客户端，可重入但不能再次领取旧项；每次 pump 的队列扫描／grant 都有 32 次界限，不用零毫秒忙重排。

Artifact、Summary 和 Translate 共用这一个 FIFO。一个阶段真实 finished 后，新阶段／坏 cache 后的新 worker 仍重新入队，不持着槽跨网络下载或优先无限重试。queued worker 仅持原生输入 owner／值，不读文件或创建运行线程。等待期间输入 owner 继续保活，但本预算不等于所有下载、缓存／catalog、工件临时磁盘、Qt parser 或进程总内存上限。

## worker 合同与两个终态

两个 worker 新增 `bool schedule()` 和独立 `neverStarted(const QString& requestId,const QString& reason)`；保留原 `requestCancel` 原子位及 resultReady／finished。schedule=false 表示从未受理，不发该次自身信号；caller 清其准确未启动 worker／请求，不能等待不存在的 finished。schedule=true 表示已归本队列／运行槽管理，不表示文件已通过或 thread body 已开始。

已受理 queued 取消／120秒超期，原队列项先退休，然后仅发一次 neverStarted，worker 随后 deleteLater；不发假 resultReady／finished、不经坏工件路径清 cache。`requestCancel()` 保持线程安全的原子撤销，下一次巡检及最终获槽检查准确识别该 worker；实际客户端都在来源退休／析构中撤原 worker，而不是取消共享 latest。取消已线性进入 Running 的任务只设其原子位，不能承诺 run body 物理上尚未进入或撤回 IO。

每个 worker 的构造器先安装一个 DirectConnection finished latch，闭包只写独立 shared atomic，不读 GUI／worker 成员；这先于客户端连接，避免早期 GUI deleteLater／嵌套处理丢失原线程完成事实。进程槽只在该实际 finished latch 后归还，取消、resultReady、来源退休、owner死亡或 QObject destroyed 均不提前释放；未见 finished 却 QObject 消失则调度器 unavailable，不猜线程已停。释放槽和 QObject 回收分开：运行 worker 仍由客户端预先安装的独立 finished→deleteLater 回收，不能在 scheduler 中先删而丢掉 queued result／finished 消费尾。

`QThread::start` 在两个派生 worker 的正常接口中隐藏，唯一调度 start 位于本模块；不是抗恶意 C++ 强转到 QThread 的能力边界。原 Manager 两处 normal／stale-cancel start、Translation normal 和 drainUnstarted-cancel start 四个入口都须实际改为 schedule 或明确回收未受理对象，不能保留“取消仍直接 start”旁路。消费者先安装独立 neverStarted 和原双尾连接，再受理；不把队满／排队取消误当 CRC 文件校验失败，也不因取消可选元数据步骤挂住 InitialConnect。

## 关闭和限制

应用销毁只撤 queued／running 的原取消位；queued 为已知未启动，可安排 deleteLater，running 不 wait／terminate／删除，原生输入 owner 保留到真实 run／worker 结束。部分 app 析构中不新发可选元数据 continuation，停 event loop 后的删除只是尽力回收，不声称崩溃耐久。槽计的是两类 worker 的实际 run／finished，不声称已经 OS join 或全部 thread-local 析构完成。worker 自己的运行预算保持独立：Translate 原 120秒 clock 不含队列等待；Artifact 原字节／解压界限及原子取消也不被改成可物理抢占 OS IO 的保证。并发槽绝不靠超时强行归还。

仅直接源码静读与 apply_patch，未运行线程、项目、解析器、helper、测试或编译。工程验证仍须覆盖多车 FIFO／2+32界、四真实入口、同worker复用拒、false受理、queued取消／等待超期、获槽前晚时钟、运行取消不提前还槽、实际双尾两种顺序、缺端／销毁、neverStarted清忙不误失效cache、输入owner、app关闭，以及无可选元数据时正常参数回退与初始连接步骤终结。未作吞吐、实时、GUI或跨平台运行验收。

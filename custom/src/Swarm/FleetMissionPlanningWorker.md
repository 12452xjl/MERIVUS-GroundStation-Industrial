# FleetMissionPlanningWorker（451）

本模块把一次固定任务／机队值的编译放到独立 `QThread`。只由原生协调器创建，输入先通过完整有界纯JSON快照，再按不可变值持有；线程不捕获Coordinator、Registry、Vehicle、ParameterManager、账号、GUI、路径或命令对象。不进行认证、文件读写、上传或飞行控制。

## 值与资源边界

`snapshot(input,output)` 只做完整值检查和JSON深拷贝，不执行规划。外层必须是实际map，值仅map/list/string/bool/null及可无损传递的有限数值。Double／Float保留其有限原表示值，不在快照层另加2^53−1绝对值上限而缩小业务合法域；每个实际业务schema仍独立限制其允许范围。Int／UInt均可精确转为binary64；LongLong／ULongLong必须能够经binary64精确往返，先检查[-2^63,2^63)／[0,2^64)的回转域，再转换回原整数比较，避免边界溢出或未定义转换。并非任意大整数都允许，也不是将舍入后的整数当作原值。

typed-null容器、opaque对象、坏Unicode、非有限浮点数、不可无损转换的64位整数及超预算整份拒绝，不保有效前缀。限制32 MiB紧凑JSON、24层、1000000节点，key最多256字符；具体Compiler／几何模型原有更小预算仍分别生效，不被这里放大。空QString保持空文本，未知值以JSON null保留。以上仅源码推导，未执行数值或边界用例。

工作线程只调用 `FleetMissionCompiler::compileCancelable(task,fleet,token)`，不调用会更新实例 `_lastPlan` 和发 `planChanged` 的原实例接口。`token` 由本次Coordinator私有持有，`shared_ptr<atomic_bool>` 仅允许false→true；编译器以共享只读atomic token持有同一生命期。取消标记不是计划，不可当正常结果审阅。工作开始前、编译返回后及输出快照后均核取消；取消／坏输出不保可采用计划。

取消在Compiler及几何内部协作检查，并非抢占线程或固定时延保证。部分有界编码／JSON提取只在阶段边界检查，不能宣称立即停止CPU工作。没有轮询、线程强杀、无限队列或重试。

## 生命周期

线程无parent，Coordinator不会把运行中的QThread作为子对象析构。实际 `QThread::finished` 以queued连接交给Coordinator，只有此后GUI线程才读取并冻结结果，再通知 `planningReady`。准备好不发布 `lastReview`，也没有审批／执行权限。

自清理 `finished -> deleteLater` 不依赖Coordinator存活。Coordinator析构仅置取消并解绑自身回调，不调用 `wait`、不阻塞GUI、也不删除仍运行线程。旧线程尚未实际结束时，新开始请求取消旧请求并明确拒绝busy；必须由用户／调用方之后重新明确发起，不后台排队。

## 静态交付

本轮只修改和静态读取源码，未编译、运行线程、执行helper／测试、打开UI或连接设备。线程结束与取消竞态、析构、自清理、资源预算、正常／取消／失败结果及主线程响应性仍需工程师后续验证。后台编译不完成净空认证、场景时效、实际机体／datum、接地模型或最终StartPermit。

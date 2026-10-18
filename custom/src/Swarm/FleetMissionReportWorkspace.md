# 飞前审阅报告工作区（546）

本地用户链是：当前实际审阅＋同一PlanStore绑定 → 后台完整冻结 → 四类视角分页 → 明确保存完整JSON → 独立历史重开。它不是[FleetMissionExecutionReport](FleetMissionExecutionReport.md)实际Run报告，不产生事故定论、正式任务结果、飞行许可或权限恢复。

## 来源和格式

`capture(actualCoordinator,actualStore,binding9,true)`不接受调用者report。只从实际同线程Coordinator的lastReview读取完整plan/report，从实际PlanStore读取对应record.plan；核原绑定7字段，后台全计划JSON语义相等及report工程视角的计划相等，复用Builder原v1摘要。连接后任何reviewChanged使本次捕获失效（包括A→B→A）；结束仍核实际源、原绑定、原report。Root另以原binding9和输入/审阅代次撤销未完成捕获。捕获是历史副本，不刷新参数/健康或延长批准。

PlanStore默认reviewEvidence并未保存report，`reportIncluded=false`保持，不为本功能改变默认历史库。Coordinator不再在Builder已计算摘要后添加两个根字段；报告自身`boundaries`和外层review边界保持。工作区保存原v1完整报告，不新封装、不把protected Run格式混入。

## API与小值

- `state` exact7：schemaVersion/busy/jobId/action/stage/cancelRequested/snapshotId；stage为idle/running/cancelling/complete/failed/cancelled。
- `summary` inactive为{}，active exact16：snapshotId/reportId/sourceKind/sourceTaskId/planId/generatedAt/verdict/incidentCount/unverifiedCount/audiences/readOnly/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased。sourceKind为captured-review/file-review，后者全部身份与时间仅文件声称；readOnly/historicalOnly为true，四个权限/认证位false。
- capture及`requestFile(action,path,expectedJobId,expectedSnapshotId,overwriteConfirmed,true)`回exact11：schemaVersion/action/accepted/jobId/reason/requestOnly/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased。accepted只表示受理。
- `result`初始{}；其后exact11：schemaVersion/action/jobId/ok/error/reply/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased。capture/open成功reply={summary}；失败reply={}。
- save reply沿共享exact11：snapshotId/contentHash/path/fileWriteAttempted/fileWrittenKnown/fileWritten/readbackVerified/readbackError/fsyncVerified/durable/authenticated。snapshotId是原冻结副本，不是save job；最后三位固定false。实际提交后取消/回读失败仍保fileWritten及ok事实，error可非空。
- `cancel(jobId)`只取消准确运行任务，busy保留到actual finished。`clear(snapshotId)`仅idle清准确本地副本，不删文件/改变原review，历史result保留诊断。
- `viewPage(snapshotId,audience,offset)`仅四种commander/customer/engineer/safety；返回exact9：ok/error/snapshotId/audience/offset/nextOffset/totalLength/hasMore/text。offset按UTF16计，每片≤2048且页尾不切代理对。坏来源/参数返{}，无IO。

capture/save/open均各自新UUID后台一次性QThread，输入仅冻结纯值，worker没有live QObject。实际finished后原owner才读结果并释放单槽，独立deleteLater存活于owner销毁；销毁仅原子取消，不删除运行线程。新capture/open失败不替换旧native副本；UI不得凭共享旧结果自动领用。

## 资源和权限

32MiB整份UTF8/保存输出，后台树深32、节点1,048,576、字符/工作计数32Mi；报告须完整通过，不交付成功前缀。四视角预排文本总≤32MiB，GUI只取小摘要与2048字符片，文件读写使用共享[SnapshotIO](FleetSnapshotFileIo.md)的本地JSON路径、QSaveFile和回读。摘要不验证文件签名，回读相同不证明fsync或耐久性。

完整JSON始终包含工程原值。客户视角仅显示裁剪，不是脱敏文件，不是权限隔离；UI明确保存的是完整资料。四视角中的gate/clear/recommendedActions都是原报告声明，不作命令门。代码开发阶段未编译、运行、解析、hash、测试、仿真、硬件或刷机；工程师承担构建及实操验证，不将未完成源码转交为配置工作。最终源码交接封包不代表运行上述业务链或通过工程验证。

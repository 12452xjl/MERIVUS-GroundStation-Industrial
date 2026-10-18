# GNSS 历史记忆工作区

## 523：原生页明确追加、不可变文件快照和独立历史浏览

[Workspace QML](../../res/Merivus/FleetGnssMemoryWorkspacePanel.qml)嵌入既有[TelemetryHistoryPanel](../../res/Merivus/FleetTelemetryHistoryPanel.qml)的独立GNSS区，由两个既有宿主沿同实际Archive窄facade取得 `archive.gnssMemory`。不放入维护页，不新增顶层地面站或飞行入口。父级仅在自己实际页／报告仍当前时给出原 `reportToken` 和 `reportPreview`；预览只供计数、来源和确认核对，不作为原生追加输入。

### 追加与来源

必须先按522分层同意读取并生成当前页GNSS报告，再独立勾选本页追加确认，调用 `workspace.append(actualReportToken,true)`。不接caller报告Map、位置数组或任意记录。exact7回执为 `appendAccepted,reason,memoryInstanceEpoch,memoryRevision,reportIndex,reportCount,sampleCount`；只表示实际本地接纳，不是保存文件、认证或任务完成。异常／晚失源不自动重试，也不把false当回滚。

原生最多32份报告／2048样本，满额整次拒绝，不trim／淘汰。跨页、多次采集或文件报告均保完整原页sourceIdentity7和原sample18；采集runId、数字机号或地理接近不等于真实任务归属或同一物理飞机。工作区不把这些数据并入Legacy运营KPI、当前飞机状态或跨任务完成率。

### 不可变快照与文件

用户独立确认freeze actual本进程记忆；正常异步双终结回执后只安装本面板实际取得的session快照，不自动选槽／读目录。文件对话框仅暂存本地路径，不发IO；用户返回后另勾本次文件确认，保存必须明确允许覆盖，不能用文件窗口自身的覆盖提示替代。保存只传原session snapshotId／contentHash，后来append使sourceCurrent=false也不改变这些固定bytes，不能在save时重导出live内容。

open仅产生独立file-history槽，不调用旧 `loadFromFile/importJson` 覆盖active记忆。文件内容、hash或导入声明都不恢复Archive当前页资格、物理身份、任务归属、权限或批准。初始两个槽均未选；open／freeze不会自动读取首报告或启用地图。clear-file／clear-frozen为同步精确清槽，不等待不存在的新result，不删除文件、不清active记录，保留原文件写入回执。

文件操作展示真实 `fileWriteAttempted,fileWrittenKnown,fileWritten,readbackVerified,readbackError,fsyncVerified,durable`。未尝试可明确known未写；commit失败也可为未知而不是未写；commit成功但readback失败仍保文件已写，外层ok不要求error为空。fsync／durable保持false，不把普通QSaveFile回执称持久化认证。隐藏或退出不自动重试／撤回可能已提交的文件写入。

### 固定窄值面与浏览

state exact17：`schemaVersion,jobId,action,stage,busy,cancelRequested,frozen,file,selectedReportToken,memoryInstanceEpoch,memoryRevision,reportCount,sampleCount,historicalOnly,permissionsRestored,executionPermit,flightCommandReleased`。

result exact10：`jobId,action,ok,error,reply,historicalOnly,authenticated,permissionsRestored,executionPermit,flightCommandReleased`；初始可为空Map。request exact8：`action,expectedJobId,snapshotId,contentHash,filePath,reportIndex,offset,overwriteConfirmed`，无用字段必须中性。动作闭集freeze／open／save／reports／report／cancel／clear-file／clear-frozen；receipt11沿原异步工作区合同。cancel只认本页真实jobId，终结result仍保原动作，不把pending改为cancel。

descriptor exact17：`schemaVersion,snapshotId,origin,sourceInstanceEpoch,sourceRevision,capturedAtUtcMs,reportCount,sampleCount,locatedSampleCount,contentHash,sourceFileHash,sourceCurrent,historicalOnly,authenticated,permissionsRestored,executionPermit,flightCommandReleased`。session和file是两个不同来源槽，文件固定sourceCurrent=false；sourceCurrent仅是冻结来源revision提示，不是设备证据或许可。session的capturedAtUtcMs显示为“本地冻结UTC”；file仅显示“文档声称UTC（未认证）”，不当成本次打开或当前本地采时。

文件／冻结文档的原生字节预算为4MiB，不把全库JSON交给QML。用户明确选槽后，另按目录读取；reports reply6 `snapshotId,afterIndex,nextIndex,totalCount,hasMore,rows`，每页≤8。row11为原报告序号、sourceIdentity7、rowCount／GNSS／定位／未定位／三项质量计数、groupCount、historicalOnly。上一页保实际offset栈，下一页只用实际nextIndex，不自动续读。明确点击某行才请求report，reply exact3 `snapshotId,reportIndex,report`（report为完整33）；不自动选择首项，不认领共享latest。

完整报告仍按522的33／7／3／18闭形状及固定false字段核对，与本次实际目录行逐字段对应；sourceSequence全uint64保持十进制字符串，不转JS Number。原记录的unknown、真实0、无fix、不定位原因及阈值提示均完整显示，最多64条虚拟列表。HDOP无单位，不是米；“显示阈值内”不是精度或正常认证。

采用还要求实际 `selectedReportToken` 新UUID、原生 `selectedReportBinding` exact5 `{snapshotId,contentHash,sourceFileHash,origin,reportIndex}`，且公开selectedReport、result.report与本地保留stamp全值相同。state／result／selectedReport／token／binding的QML更新顺序不是合同：等待同job／原action及原selected绑定就绪才接纳，不拿旧空值提前消费pending。终态绑定无法接纳时可明确退出本页待接纳回执，不撤回已发生操作或自动重试。

### 地图与生命周期

读取报告并不选地图。必须另按明确按钮，才用准确selectedReportToken和同引用plain `{cancelled:false}` 调用 `workspace.selectMap`；通过同workspace `clearMap` 只清实际返回的原selectionToken。默认off，多个面板不能清除别人后来选择；隐藏／disabled／收起／换workspace及明确换槽／目录／报告会退休原本地地图lease。原生selected token变化也撤原选择，晚回token仅做精确清理。

Root的Archive和Memory请求桥均不预先清共享latest图层。被拒／陈旧／占用请求不能清另一个面板后来选择的另一来源；本页精确lease清理与实际原生report token退休分别负责自身选择，不把一次请求当清理全图的授权。

Root共享地图投影升级exact5：`active,selectionToken,reviewToken,origin,report`。origin闭集native-page／session-snapshot／file-history，inactive为空。原522原生页入口不变；文件历史不借Archive.gnssReviewToken变成当前页。[FlyViewMap](../../res/Merivus/FlyViewMap.qml)共用原离散点renderer，图例明确区分三来源及未认证／未恢复active或权限，仍无连线、精度圆、自动center、goto或飞行命令，极区仅明确省略底图点并保报告原值。标记及label吞点击／双击，防穿透原地图动作。

plain ticket早于公开busy／确认通知，跨整个调用、结果接纳、原状态发布与finally；释放使用预先冻结的plain closure，销毁后不重新查QObject helper。路径、dialog身份、原snapshot/hash、选择、confirmation和epoch均冻结并尾核；false→true／路径A→B→A不会恢复旧pressed。正常自有job推进只容恰一次，不把其它面板新job当本页结果。保存回执独立保留，文件路径和确认不因历史事实复活。

### 静态边界

ui-ux-pro-max和quick-reference／pro-rules本轮完整静读，依原QGC主题、PlainText换行、可见标签／unknown、44像素最小动作和键盘焦点，采用通用静态fallback。未运行技能搜索、QML／JS／项目／helper／解析器／hash／SQL、编译或测试，未预览／渲染／测量对比度、读屏、触控、小窗或大字。所有工程场景仍待执行；本模块不是原25目标验收完成、任务归属／定位真实性／完整历史／飞行安全或权限恢复证明。

# FleetArchiveTaskIndexPanel — 持久化任务诊断索引入口

## 538：源码接线与明确边界

本页为 [FleetArchiveTaskIndex](FleetArchiveTaskIndex.md) 增加真实用户入口：在既有归档历史面板中默认收起，展开不读盘；停止采集、写入及共享原生读取／维护槽空闲后，操作员可明确创建、重开、续扫、分页和查看原保留记录。原有任务目录与其预算、页面保留，不由新页替换。

创建、续扫和重建会写入派生检查点，不是只读建议。重开、任务分页与原记录查询为显式读盘。所有文件、SHA、锁、SQL、原生游标、检查点和原始日志核对均在原生后台完成；本页不接收或构造路径、SQL、检查点哈希、文件来源声明或 Reader 游标。展开、选范围、选任务、滚动和显示已有结果不会启动查询。

只做了源代码修改与静态阅读，没有执行 QML／JavaScript、解析器、索引或归档 IO，没有编译、测试、仿真或硬件验证。这里不是已验证安装包、可运行证明或上线放行。

## 页面与门面

- 页面：`custom/res/Merivus/FleetArchiveTaskIndexPanel.qml`。
- 既有 `FleetArchiveHistoryPanel.qml` 增加默认收起 Loader；收起销毁页面，不自动取消已经接纳的原生查询。提示明确说明该行为。
- `GuidedActionsController.qml` 既有归档门面新增只读 `taskIndex`，直接投影 `FleetPersistentEventStore.historyTaskIndex`；`index(intent, true)` 只复制约定的七个字符串后调用 `requestLocalArchiveTaskIndex`。
- `custom/res/Merivus/qmldir` 与 `custom/qgroundcontrol.qrc` 登记页面；不新增独立数据库、线程或任务执行架构。

界面遵循现有 Qt/QGC 主题、可换行明文标签、就地失败说明、独立确认与键盘焦点。UI/UX 技能仅用于静态表单、反馈、状态清晰度和键盘建议，没有运行其搜索或渲染脚本。没有引入新布局系统或主题。

## 七字段请求

公开 intent 精确为七个非空 QVariant 类型的字符串键（字符串内容可按动作要求为空）：

`action, expectedRunId, scope, previousQueryId, indexId, taskIdHash, afterTask`。

`scope` 为 `sealed` 或 `sealed-and-offline`。所有动作精确携带本页原 Store 的当前 `expectedRunId`；没有运行时源或车辆账户也不凭空制造任务／飞行身份。

| 动作 | 本页提交约束 |
| --- | --- |
| `create` / `open` | 首动作；`previousQueryId/indexId/taskIdHash/afterTask` 全空。 |
| `rebuild` | 独立首动作，四字段全空；只须原生槽空闲，不依赖已重开成功的旧结果。先准备，再单独确认，确认仍绑定原 owner／run／范围／选项／本页代次。会替换派生索引，不删除原归档；旧文件缺失、损坏或原生核验失败由后端明确拒绝。 |
| `resume` | 原已接纳的最新查询与索引 ID；冻结范围尚未扫描到末尾；其余选择字段空。 |
| `page` | 原已接纳的最新查询与索引 ID；`afterTask` 只能空（首页）或原实际任务页的 `nextAfterTask`。 |
| `records` | 原实际最多 64 条任务页中明确选中的任务哈希；最新原查询与索引 ID；没有 caller 游标。 |
| `continue-records` | 原最新记录结果同一任务，尚未到冻结末尾；原生后端取它自己的 Reader 游标；不使用旧页后新查询的过时记录结果续读。 |

创建、重开、续扫、重建结果的 `page` 可以且按当前合同必须为 `{}`。界面不会误判失败或伪造首页；有当前检查点后，操作员再明确读取任务首页。任务页与记录页均最多 64 条，不无限累积 GUI 数组。

回执沿原 `localDecision` 精确九键：`schemaVersion/action/accepted/requestId/reason/requestOnly/historicalOnly/permissionsRestored/flightCommandReleased`。`accepted=true` 只代表原生受理，不代表读盘、提交或线程结束。拒绝、未知或异常不自动重试，旧副本只读，需明确重开后再继续。

## 成功结果与接纳

Store 公开精确十八键：

`format, schemaVersion, action, scope, indexId, checkpointHash, checkpointCommitted, cancelObservedAfterCommit, checkpoint, page, records, historicalOnly, authenticated, permissionsRestored, executionPermit, flightCommandReleased, queryId, runId`。

其中 `format=merivus-fleet-archive-task-index`，`schemaVersion=1`；历史标记为真，认证、权限、执行许可和飞行命令放行标记恒假。页面只核对和有界复制真实门面结果，不把自构造 map 或其它查询的共享最新结果当成成功。

嵌套接受域：

- `checkpoint` 精确八键：`inventoryFingerprint/scanComplete/taskCount/indexedEventCount/unassignedEventCount/sourceFileCount/windowBatches/windowScannedRows`。任务上限 32768；两个事件计数保留完整十进制 uint64 字符串，不转 JS Number；单窗最多 128 批、8192 行，没有把单窗 8192 再当终身限制。`sourceFileCount` 是已冻结源文件数，不是已扫描完成文件数。
- 仅 `page` 动作为精确五键页：`afterTask/nextAfterTask/hasMore/totalCount/rows`。最多 64 个精确两键行 `taskIdHash/eventCount`；哈希严格升序，计数为正 uint64 字符串。其它动作页为 `{}`。
- 仅 `records/continue-records` 含原 Reader 的精确二十九键任务过滤结果；原 `rows/scannedPages/nextCursor`、覆盖、缺失和认证假标记保留。任务哈希、范围与库存指纹须对应当前实际检查点。`resultBytesWithoutThisField` 沿请求时冻结的实际 `policy.maximumHistoryMiB`（1–8 MiB，默认 2），不硬缩为 1 MiB；本页独立显示复制预算仍可能全拒超大结果，并就地说明。其它动作 `records={}`。
- 本页有界复制最多深度 16、131072 个值、2097152 UTF-16 字符、单数组 256 项、单 map 64 键；超过预算全拒，不显示成功前缀。原生后端仍负责完整 Reader 与检查点值校验。

本页记录受理的原 owner、run、选项、范围、页面代次和实际 requestId，等待 `historyRequestPending=false`、`archivedReaderActive=false` 且维护空闲；随后仅在 `historyRequestKind` 的实际 job、`lastHistoryQueryId`、`archivedResultAccepted` 与门面原对象相符时接纳。原线程未实际结束前不释放本页的等待语义，不以 resultReady 冒充线程已结束。

## 页保留、换源与取消

正常记录查询返回 `page={}` 时不抹掉原实际任务页。只在同 owner／run／范围／选项／索引 ID／检查点哈希／本页 indexSession 下保留它；下一次记录或下一任务页仍提交最新已接纳 queryId，具体页哈希和 `nextAfterTask` 由原生私有上下文再核。索引变化后旧页只读。

每次 `create/open/rebuild` 开始新的本页 indexSession，即使重开同一 ID／SHA，也不让取消、失败或旧打开周期的页副本复活。先明确首页查询，取得新的真实页才能选择任务。`resume` 提交新检查点使原哈希不同时，旧页也不可再选。记录续读只认最新记录查询本身；任务页或其它动作之后旧记录仍可阅读，但不再作为续读资格。

原门面、run、选项、范围变化或观察到外来新 job，会退休本页继续操作资格与重建确认，保留旧副本并标注只读。通知在本页调用栈内到来时先标 dirty，释放本页调用后排队处理，先安装实际回执 queryId，再判断归属；不会把本页同步受理通知误当外来查询。页面代次不回绕，调用与赋值通知后逐尾复核原来源。确认与放弃重建也在按下时冻结原确认对象；旧按钮释放不能提交或清除后来的新确认。

取消仍用原归档 `request` 精确六键：`action=cancel/expectedRunId/catalogId=""/basename=""/afterRowId="0"/queryId=原本页未结束查询`，不提供新的取消后门。取消尝试即退休本页该查询的继续资格；取消受理或结果未知都要等待实际线程结束。

取消不能回滚已实际提交的派生检查点。若同一原查询仍交付 `checkpointCommitted=true`，本页可接纳其落盘事实并只读显示；即使 worker 在取消前已生成 `cancelObservedAfterCommit=false`，本页自己的取消 ID 仍禁止 page／resume／records。必须明确重开后再操作。共享最近诊断与其它查询不能恢复该资格。

## 工程师应覆盖（本次均未执行）

1. 默认收起与展开不读盘；旧任务目录仍可用；资源注册与实际 Loader 创建。
2. 停止／录制／写入／读取／维护共享占用的按钮状态，以及原生拒绝与明确恢复文案。
3. 创建、重开、续扫空页成功，随后显式首页；跨进程重开，不从 GUI 缓存恢复 native cursor。
4. 多于 8192 行与多于 256 任务的后续扫描窗；32768 任务和计数边界由原生明确拒绝而非发布部分成功。
5. 首页／下一页最多 64 任务，记录最多 64 行；同页另一个任务、记录后下一任务页、记录后继续同任务。
6. 原文件变化使 open 拒绝后，无当前结果仍可二次确认重建；只替换派生文件，不删除原归档。
7. 页面、任务、源、范围、选项、run 和索引 SHA 变化；按下与释放之间变化；外来 job 与本页 job 的通知先后。
8. 原生失败／取消／未知回执后旧副本只读；相同 ID／SHA 重开不复活旧页选择；先显式首页。
9. 在原生提交之前与之后取消；结果已生成而取消稍后到来；保存事实可见但继续资格不恢复。
10. 原 resultReady／finished 顺序、页面销毁／收起、键盘操作、窄宽度、长哈希、长错误和大型有界记录显示。

此模块推进长期历史诊断工作流，不证明任务执行闭环、轨迹认证、机载能力、自愈控制、跨站授权、物理飞行成功或六类产品剩余全部完成。

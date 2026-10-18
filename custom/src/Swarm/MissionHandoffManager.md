# 本地任务 / Run 简报交接

## 541 当前：v2 Run 简报的有限嵌套兼容

原创建、历史编码和历史解码共同使用 `boundedHandoffMetadata`：仅 exact2 的 `{kind:run-brief,brief}` 且 brief schema2 时委托完整简报校验（深度8/4096节点/96KiB）；父层只多一层与两个节点，并限制完整 metadata JSON 为128KiB。旧简报与通用 metadata 仍用原深度8/2048节点/128KiB，不按任意声明放宽接受域。

父文件16层、32MiB、总记录/Run记录上限不变；超预算整份拒绝，不删退出证据以适配。保存、预览和仅追加历史合并不恢复当前待办、权限或控制权。见[报告合同](FleetMissionExecutionReport.md)和[退出观察合同](FleetMissionModeExitObservation.md)。本次未执行编解码、文件操作业务、构建或测试；以下539为历史。

## 539：显式后台保存、文件预览与只读历史合并

实际 Run 仍从原 `requestRunHandoff` 进入本地交接库。本批在同一个管理器增加独立文件工作区：
用户明确选择路径并确认覆盖后保存当前原生集合；明确读取现有文件只安装私有候选预览；
查看四条一页的目录和逐条完整详情后，另行确认才追加合并为只读历史。没有自动保存、
自动加载、自动合并或恢复 live Run；不调用旧 `loadFromFile` 覆盖正在使用的集合。

### 原生接口及精确载体

`requestHistory(exactIntent, confirmed)` 要求 `confirmed=true`，intent 恰好五键：
`action / expectedJobId / filePath / previewId` 为实际非 null String，
`overwriteConfirmed` 为实际非 null Bool。所有动作都精确匹配当前 `historyState.jobId`
（初始为空串），无用业务字段必须为空串或 false：

| action | 唯一业务输入 | 终结方式 |
| --- | --- | --- |
| save | 本地绝对 `.json` 路径、独立 `overwriteConfirmed=true` | 新 UUID，后台 |
| load | 本地绝对 `.json` 路径 | 新 UUID，后台，仅预览 |
| merge | 当前私有 `previewId` | 新 UUID，后台候选、GUI 代次 CAS 提交 |
| discard | 当前私有 `previewId` | 同步清预览，不产生新 job/result |
| cancel | 无；仅 `expectedJobId` 选原在途工作 | 同步请求取消，仍等 actual finished |

请求回执沿用 exact11：`schemaVersion / action / accepted / jobId / reason /
requestOnly / historicalOnly / authenticated / permissionsRestored / executionPermit /
flightCommandReleased`；schemaVersion 为 1，requestOnly/historicalOnly 为 true，其余权限
和认证均 false。accepted 只表示请求受理，尤其不代表保存、合并、取消撤回或远端交接完成。

`historyState` 恰好十一键：`schemaVersion / busy / jobId / action / stage /
cancelRequested / previewId / historicalOnly / permissionsRestored / executionPermit /
flightCommandReleased`。stage 为 `idle / running / cancelling / complete / failed / cancelled`。
busy 在原实际 worker 的 finished 处理完成前保持；action 保原 save/load/merge，不改为 cancel。

`historyResult` 初始 `{}`，终态 exact11：`schemaVersion / jobId / action / ok / error /
reply / historicalOnly / authenticated / permissionsRestored / executionPermit /
flightCommandReleased`。`historyWorkspaceChanged` 通知三份属性；外层绑定顺序不保证，UI 只能
凭自己真实回执的 jobId/action 接纳，不从共享最新结果重新领取操作资格。

`historyPreview` 无预览时 `{}`，成功 load 后 exact12：`schemaVersion / previewId / jobId /
path / contentHash / recordCount / runRecordCount / historicalOnly / authenticated /
permissionsRestored / executionPermit / flightCommandReleased`。previewId 为成功后新 UUID，
jobId 是原 load UUID，contentHash 是实际读入文件字节 SHA-256 的 64 字符小写十六进制；
不是签名、账号或真实性。load reply 仅 `{preview: <同份12键>}`。新的已受理 load 先退休旧
预览；读取失败不复活它。save 不读取、更改或重建原预览。

`historyPreviewPage(previewId,page)` 成功复用下文 page4/row14，按文件插入顺序倒序、每页四条；
失败为零值 page4 加固定 error（wrong-thread、invalid-page、history-preview-not-current 或
history-preview-invalid）。`historyPreviewRecord(previewId,handoffId)` 成功复用旧完整 detail16，
失败 `{}`。它们只读原私有候选，不从当前集合或 Registry 补字段；预览行 origin 均 imported。

save reply 是原文件事实 exact11：`snapshotId / contentHash / path / fileWriteAttempted /
fileWrittenKnown / fileWritten / readbackVerified / readbackError / fsyncVerified / durable /
authenticated`。snapshotId 是本次 save job UUID，hash 是冻结集合实际编码字节，路径是原选路径；
后三个保证恒 false。编码前失败可为 `{}`，不据此猜“从未写入”。使用共享
`FleetSnapshotFileIo::saveAndReadback`，commit 不确认时保 unknown；commit 已成功则 ok=true，
即使 error/readbackError 非空或后来取消也保 fileWritten=true。readback 失败不回滚文件；
不声称 fsync、异地备份、物理耐久或竞态外部进程保护。

merge 成功 reply 恰好四键：`previewId / addedCount / duplicateCount / recordCount`；保原
预览 ID 作为事实关联，提交后清预览。失败或取消不改集合、保原预览供用户另行明确重试。
discard 只清准确预览，保历史 result 和原 state job/action/stage；不会清后来预览或自动取消别人。

### 后台、原子候选及只读隔离

私有 cc worker 是 parentless QThread，无 Q_OBJECT，不访问 Manager、账号、Registry、飞机、
传输或 QML。save 在受理时只冻结 `_records/_order` 的 typed COW 值及原计数，所有完整 JSON
编码和文件 I/O 在后台；不在 GUI 先 exportJson。load 后台完整 UTF-8 读取、解码和校验，
不发布合法前缀。merge 后台拿原当前库和原预览的两份不可变快照，完成所有 ID 比较、
容量检查及新集合候选，GUI 只提交同原 collectionGeneration 的结果。

collectionGeneration 是每次实际新建、处理、旧 import 替换及新 merge 提交时更新的独立
本地 UUID，不用可被旧 import 重设的 recordRevision/sequence 判断来源。后台期间当前库
即使等值变化后恢复，也不能用旧候选覆盖它；失败明确重试，不重基到新集合。save 保存的
仍是按下时冻结的旧内容，不因集合后来变化而换成新内容。

同 ID 比较完整 schema1 持久字段的规范 JSON 表示，不比较运行时 origin/historicalOnly；
完全相同只计 duplicate 并保原本地记录，不把已有 local 记录降成 imported；任何内容冲突
使整笔 merge 失败。无冲突时仅追加新 ID，不删除或替换任何当前记录。实际记录上限
100000、Run 简报上限 256、完整编码 32MiB 均先检查，满额拒绝，不淘汰或截断。

新入口的所有追加记录私有 historicalOnly=true、origin=imported，包含通用 pending；
不能接受、拒绝、取消或手动到期，不进入 pendingCount/inbox/outbox。定时器不改其文件
原 status；history（相应参与 actor）和 all 仍可浏览。这使重复导入同一未改文件不会因
本地自动到期而制造冲突。已有本地重复记录完全保留原行为。旧 importJson/loadFromFile
及 generic imported 的 legacy 语义不扩改，新流程不调用它们；原 imported Run 的旧到期
政策也保持。任何历史时间只是文件声称，不是本机采时、任务结束或真实接管证明。

worker 取消只原 atomic，不提前释放 busy；析构只请求本 worker 取消，不等待或删除运行线程。
actual finished 的消费连接先于 worker 的 deleteLater，原 worker/UUID 精确核对后才读输出。
load/merge 的末尾取消不安装资格；save 已提交事实仍可发布。merge 在无外调的 CAS 提交及
结果提交后才通知，信号内后来关闭/取消不会把已追加记录伪称回滚。

复用 schema1 的元数据深度8／2048节点／128KiB标量和 Run brief 校验；文件在 Qt JSON 构造前
另限嵌套16。取消在有界读取、扫描和每记录处理间检查，不保证正在进行的单次 Qt JSON/hash
调用可中断，也不承诺全进程峰值内存或响应时间。旧 generic 元数据含 Hash/StringList/
ByteArray 或大整数时，schema1 JSON 的原有转换限制仍在；不能把规范持久表示相同宣称为
所有 QVariant 元型／整数位模式无损往返。原 Run 简报仍单独通过其 JSON-safe 合同。

### 工程交接（仅列场景，全部未执行）

1. save 捕获后当前 Run 变化，保存仍为原冻结集合。
2. 32MiB、100000 条、256 份 Run 边界及整份坏尾拒绝，不导入合法前缀。
3. load 成功只出现独立预览，旧 live 收件箱和待处理状态不变。
4. 四行分页及完整通用／Run 详情，缺失序列不补零。
5. 同 ID 同持久内容 dedup，origin 不制造冲突，local 身份不降级。
6. 同 ID 不同内容整笔拒绝，合并前后当前库无前缀追加。
7. 后台 merge 期间 legacy import/新建/处理造成代次改变，整笔候选拒绝。
8. 新通用 pending 历史不可处理、无待办徽标、不被定时器修改；all/history 可浏览。
9. cancel 精确原 job，重复取消不重复通知，不表示撤回已 commit 文件。
10. save commit 成功但 readback 失败／取消，保 Written=true 与独立诊断。
11. discard/换预览及旧回调不能清后来原生 job／预览。
12. 页面或 Manager 销毁后运行线程只取消并等 actual finished 回收，无运行线程 delete。

## 436 增量与边界

`MissionHandoffManager` 保留原 `pending / accepted / rejected / cancelled / expired`
本地记录状态机，新增实际 Run 的不可变摘要入口和有界收件箱分页。它不改变飞机模式、
不发送 MAVLink、不转移跨站控制租约、不恢复启动批准，也不认证收发操作员或远端任务。
真实会话、角色以及当前选择的原生 Run 来源仍由调用方重新核验；本类校验的是值合同。
本轮仅修改源码并静态读回，未编译、运行、测试、仿真、刷机或执行文件 API。

## Run 记录

`requestRunHandoff(brief, fromOperator, toOperator, reason)` 返回已提交记录的 ID，
拒绝返回空字符串。它调用 `FleetMissionExecutionReport::handoffBriefValid`，并由简报
自身的 `taskId` 取任务身份，不能让调用者另外提供另一个任务号。记录元数据恰好为：

```
{ kind: "run-brief", brief: <经过校验的完整简报> }
```

固定 `requiresSecondApproval=true`、`expirySeconds=300`；两个操作员必须不同。
原通用 `requestHandoff` 如使用保留的 `kind=run-brief` 也必须满足同样的简报校验、
任务绑定、有效期与数量约束，不能绕过专用入口。除状态、处理备注和处理时间外没有
修改既有简报的入口；记录内容以 Qt 值拷贝保存，详情返回值也不是可写原生记录。

全库最多 100000 条记录，其中 Run 简报最多 256 条（含终态和导入历史）。到限明确
拒绝新建，不淘汰旧历史。简报还受报告模块自己的 96 KiB / 1024 节点 / 深度 5 限制，
嵌入元数据后继续受原深度 8 / 2048 节点 / 128 KiB 标量预算约束。快照文件继续以
32 MiB 为上限；记录数上限不保证所有一般元数据的组合一定能导出到这个文件预算内。

`sequence` 和 `recordRevision` 的新建 / 变更不超过 JavaScript 精确整数上限
9007199254740991；到限不再变更。新的 ID 如与导入记录冲突则拒绝，不覆盖原记录。
接受、拒绝和取消继续要求原记录 revision 的 CAS；接受 / 拒绝者必须是接收人，
取消者必须是发出人。每次处理同步核到期时间，不把一秒定时器当成有效期授权。

请求与处理先提交原生记录，再发事件。信号中如发生重新导入、处理另一记录或销毁
管理器，已提交的 ID / 处理返回事实不倒退为“从未提交”；后续集合信号仅在对象仍存活
时发出。事件携带提交时冻结的 ID 和摘要，不持有可被回调替换的 QHash 元素引用。

## 分页合同

`handoffPage(actorId, filter, page)` 只读当前实际记录，不缓存全库摘要。
`page` 是 0 基的非负 JavaScript 精确整数（允许 QVariant 整数或 Double 整数，
拒绝字符串、Bool、null、小数及超限值），每页固定 4 条，按插入顺序倒序展示。
合法但超出末页的索引夹到最后一页；空集合为 `page=0 / pageCount=0`。

| filter | 范围 |
| --- | --- |
| inbox | 当前 actor 是接收人的 pending 记录 |
| outbox | 当前 actor 是发出人的 pending 记录 |
| history | 当前 actor 参与的终态记录 |
| all | 全部本地记录，可无 actor，用于离线历史浏览 |

其余三种过滤器必须提供非空、最多 128 字符的 actor。过滤身份只是选择条件，不是
登录证明。成功返回恰好 4 键：`page / pageCount / total / records`。失败返回这 4 键
的 `0 / 0 / 0 / []`，另加 `error`，闭集为 `wrong-thread / invalid-filter /
invalid-actor / invalid-page`。读取不更新状态、不触发到期变更、无信号或文件操作；
到期待处理的 pending 行仍以真实记录状态显示，处理入口会再次拒绝过期动作。

每行恰好 14 键：

```
handoffId, taskId, fromOperator, toOperator, status, recordRevision,
createdAt, expiresAt, resolvedAt, resolvedBy,
briefAvailable, briefHash, runId, origin
```

不返回元数据全文。`briefHash` 来自简报 `contentHash`；通用记录的 `briefAvailable`
为 false，两个简报身份字符串为空。`handoff(id)` 继续按需读取完整原详情，新增
`origin` 字段，原 `metadata` 中保存完整简报。`origin` 为 `local` 或 `imported`，
仅表示本进程创建还是快照导入，不是认证或签名状态。

## 导入兼容和历史隔离

本节描述旧 `importJson/loadFromFile` 兼容行为；539 新入口的不可操作、保原状态与待办
隔离规则以上方专节为准，不借旧替换 API 恢复这些新历史记录的当前资格。

原 schema 1 的 JSON 根和记录字段保持不变；运行时 `origin` 不写入快照，也不接受
文件声称自己是 local。成功导入的每一条记录都在内存标为 imported。Run 元数据
必须完整通过简报校验且 `brief.taskId == record.taskId`，有效期必须恰好 300 秒；
缺字段、损坏哈希、错任务号或超出 Run 数量上限使整次导入拒绝，不导入合法前缀。

导入 Run 的 pending 记录不能接受、拒绝或取消为当前本机交接；允许按原到期机制
变成 expired 历史。导入的已有终态只作查看。重新保存并导入也不会恢复本机来源。
原无 Run 元数据的通用 schema 1 记录仍按既有行为兼容；这不新增任何控制权语义。

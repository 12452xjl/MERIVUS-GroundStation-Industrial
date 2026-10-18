# FleetUserTaskTemplates 用户草案模板工作区

独立命名的用户编辑预设库，不是 `FleetTaskTemplateRegistry` 的可写副本。内建受信目录及其启停／校验合同不变；用户文件、名称、UTC 时间、UUID 和版本只是本地或文件声明，不获得目录签名、能力、审批、占用或飞行权限。没有任意 defaults 合并、动态代码或自动生成机载任务。

## 实际操作链与边界

`capture(actualDraft, expectedDraftRevision, templateId, expectedTemplateVersion, metadata, expectedLibraryRevision, confirmed)` 只捕获真正同线程 `FleetMissionDraft` 的完整 `planningSource()`，用共享 `validateEditingSource` 整份规范校验，检查原对象、原 revision 和原完整值仍相同。允许尚未能构建任务的合法编辑状态；不会调用 buildTask 或审批。空 ID／空预期版本生成新 UUID，已有 ID 必须匹配当前最新版本；每次保存内容或改元数据追加不可变的正 uint64 十进制字符串版本，不覆盖旧版本。`editMetadata(id, version, metadata, libraryRevision, true)` 只复用该最新版本的完整 source 追加版本。

元数据严格为 `name/category/description` 三个 String；名称非空，名称／分类最多128个UTF-16单元，说明最多4096，无NUL或非法UTF-8往返。不接受任意附加权限、模板默认值或执行字段。最多32个独立模板、64个总版本，完整 JSON 最多32 MiB；树深最多24、整库至多1048576节点，追加也核累计节点而不留到save才发现超量。达到容量或本地／模板版本耗尽明确拒绝，不回绕、不自动删历史。一次追加只编码该条有界记录，用累计精确 JSON 记录长度加固定 envelope／逗号长度核整库预算，不在GUI重复序列化整库。坏本地UTC毫秒值拒绝捕获，不造0时间；文件时间仍是未认证声明。属性读取没有文件IO。

`previewTemplate(id, version)` 为实际库中的精确版本生成新原生 token；`restoreSource(token)` 仅返回该 token 仍绑定的完整规范编辑状态。任何库变化或文件请求退休 token；失败请求不会换出另一份 source。旧预览、文件描述或可修改的公开 Map 不能构造原生私有 source。`cancelPreview(token)` 仅清匹配 token。返回 source 不恢复任何确认：Root 必须复用现有 pristine 新 Draft 的 `loadEditingSource`、原稿备份／替换确认、旧审阅撤销链；该加载会把本地 revision 置1，并清除 `mapDatumAcknowledged`。用户仍须补全当前坐标、机型／成员和参数证据并重新审阅／审批。用户库 ID 不当作内建 `templateId`。

## 文件工作与真实事实

`requestFile(action, path, expectedJobId, expectedLibraryRevision, overwriteConfirmed, confirmed)` 只接受 `save/open`。save要求明确覆盖确认；open要求明确替换独立用户库确认且覆盖参数为false。都检查原job及本地库revision、绝对本地JSON路径；属性或展开不自动读盘。一个原生无父 one-shot QThread 持有冻结的纯值及原身份，后台完成完整 JSON／共享 Draft 校验和 `FleetSnapshotFileIo` 读写。真正 finished 后才消费原 worker 结果并释放槽；析构只请求取消，独立 finished 清理不删除运行线程。不存在伪造 resultReady、提前归还槽、自动重试或覆盖当前机载任务。

save保存冻结完整库，返回原共享 SaveResult 的写入／读回事实；实际 commit 成功后取消或读回失败不会改写为“未写入”，`ok=true` 可同时带非空读回错误，fsyncVerified／durable始终false。open先完整校验每条／每个版本／全部源，拒绝未知字段、坏类型、重复或断裂版本、超量及任何坏尾；成功前原库不变。open仅在原worker、job、动作、本地revision仍匹配且未取消时一次安装新库，使用新的本地递增revision，不恢复文件版本为本地CAS。`cancelFile(jobId)` 只请求当前精确原任务取消，busy保持到真实终态。文件操作不阻断或控制无人机。

文件根 exact8：`schemaVersion:1, documentType:'merivus-user-task-templates', records, historicalOnly:true, authenticated:false, permissionsRestored:false, executionPermit:false, flightCommandReleased:false`。record exact7：`templateId/version/name/category/description/createdAtUtcMs/editingSource`；每个ID版本从1连续递增，可与其他ID的记录交错。单条源完整沿 Draft schema1–6，不引入新源 schema。open接收的时间及源内容均未认证，不补默认性能或高度参考。

## 只读 DTO 与回执

- state exact15：`schemaVersion/instanceEpoch/libraryRevision/templateCount/versionCount/busy/jobId/action/stage/cancelRequested/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。本地库revision初值`'1'`；job初值空串，stage为idle／running／cancelling／complete／failed／cancelled。同步capture和edit-metadata也发布独立UUID及冻结的提交事实。
- rows只列各ID的最新版本；`versions(id)` 按本库原顺序列该ID全部版本。每行 exact9：`templateId/version/name/category/description/createdAtUtcMs/sourceRevision/memberCount/vertexCount`。
- capture／editMetadata／requestFile回执 exact13：`schemaVersion/action/accepted/jobId/reason/committed/templateId/version/libraryRevision/historicalOnly/authenticated/permissionsRestored/flightCommandReleased`。同步追加accepted＋committed表示本地库已追加；异步文件accepted不表示文件完成，committed为false。拒绝不删旧库或旧文件事实。
- preview exact17：`schemaVersion/previewToken/instanceEpoch/libraryRevision/templateId/version/name/category/description/sourceRevision/memberCount/vertexCount/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`；inactive为空Map。
- restore exact9：`ok/error/previewToken/templateId/version/source/manualConfirmationRequired/permissionsRestored/flightCommandReleased`；坏token返回false、空身份和空source。
- result exact12：`schemaVersion/action/jobId/ok/error/reply/libraryCommitted/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`；初始为空Map。成功capture／edit-metadata reply为`templateId/version/libraryRevision`；成功open reply为`templateCount/versionCount/libraryRevision`。save reply沿共享exact11：`snapshotId/contentHash/path/fileWriteAttempted/fileWrittenKnown/fileWritten/readbackVerified/readbackError/fsyncVerified/durable/authenticated`；编码／校验前失败可reply为空。contentHash只是实际拟保存完整bytes的SHA-256，不是签名或文件持久性证明。

## 静态交接

所有新增入口先核owner线程，再读实例私态。capture／editMetadata／requestFile在该前置门失败时返回固定空身份、libraryRevision='0'拒绝回执，不越线程读取当前库代次／job；owner线程内正常拒绝和已提交事实合同不变。

本文件及源码只直接静读、apply_patch，没有运行实现、解析器、hash、文件IO、编译或测试。工程师需覆盖：完整／未完成草案捕获；空与显式0、heading／驻留／执行profile／备用完整源保留；原Draft或预期库／模板版本变化；32／64／32MiB边界；同名元数据不可变新版本；旧版本明确生成新Draft并清datum确认；坏文件整份拒绝不前缀；open取消保原库；save提交后取消／读回失败保事实；同job／原对象尾核；析构期间后台线程独立清理；公开DTO修改不改变私有库；用户ID不进入内建信任／旧审批复用。

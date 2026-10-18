# FleetMissionEvidenceChain

## 545：后台冻结保存与独立只读历史

新文件工作流只扩展本类，不建立另一个任务、Run、权限或证据真实性服务。旧 `appendEvidence/exportJson/importJson/saveToFile/loadFromFile/verifyChain/recordAt` 接口及原 version1 格式保留；新 UI 不暴露旧 `importJson/loadFromFile` 整链替换。新 load 永远只替换独立历史槽，不写当前 `_records/_headHash`。本批不是 Run 自动检查点，也不自动保存或读取任何文件。

### 请求及单槽

新增只读属性 `fileOperationBusy`、`fileOperationStatus`、`historicalSummary`，统一 `fileOperationChanged` 通知。新增接口：

- `requestSaveSnapshot(path, expectedCount, expectedHead, overwriteConfirmed)`：要求本对象线程、空闲、合法本地绝对 `.json` 路径、真实有限整数 count 精确等于当前数量、head 精确等于当前原生链头，且明确 `overwriteConfirmed=true`，不以查询文件不存在替代覆盖确认。只捕获当时 COW records/head/path 值，不在 GUI 先 export/verify/serialize。之后当前链继续追加或旧 API 更换当前链，不影响已冻结保存。
- `requestLoadHistoricalSnapshot(path)`：同样仅在本对象线程且空闲受理，后台完整读取和验证后才候选安装。成功 load 的新 UUID 同时是历史槽身份；旧文件若同数量、同链头或同路径，也不能复用上一次 UUID。
- 两种请求返回非空 UUID 只表示受理；拒绝返回显式空串，不发布自己的虚假终态。
- `cancelFileOperation(requestId)` 只请求准确当前 busy job 的合作取消，返回 true 不表示已结束、没有写入或已回滚；实际 `finished` 前不释放槽，不启动第二 worker。
- `historyPage(expectedRequestId, offset)`、`historicalRecord(expectedRequestId, index)` 均要求精确当前历史 UUID，offset/index 仅接受范围内实际整数数值，拒布尔、字符串和 null。坏请求返回 `{}`，不能把旧索引重基到新文件。

工作线程为本 cc 内私有、parentless、无 `Q_OBJECT` 的一次性 QThread。其构造输入只有冻结值和私有 atomic cancel，不持 GUI 对象；不通过 QML 暴露 worker。结果与候选只在准确原线程实际 `finished` 送达后读取，释放与 QObject 的 deleteLater 分开。父销毁仅取消，不 wait、不删除运行线程；原 finished 连接仍负责最终回收。开始、终结及通知期间有本地入口闩，旧栈不采用后来 job 或清除后来状态。

### 精确 DTO

`fileOperationStatus` 固定23键：`schemaVersion=1,requestId,action,stage,busy,cancelRequested,ok,error,filePath,snapshotCount,snapshotHeadHash,fileWriteAttempted,fileWrittenKnown,fileWritten,readbackVerified,readbackError,fsyncVerified=false,durable=false,historicalOnly=true,authenticated=false,permissionsRestored=false,executionPermit=false,flightCommandReleased=false`。字符串为非 null String，count 为0..100000实际整数，布尔为实际 Bool。`action` 为空／`save`／`load`，stage 为 `idle/running/cancelling/complete/failed/cancelled`。running 的 `ok=false` 不是终态失败，写入事实尚未知；操作结束后 `busy=false`。

save 复用 [FleetSnapshotFileIo](FleetSnapshotFileIo.md) 的独立事实。commit 成功后 `ok=true`，即使 readback 失败或迟到取消令 `error/readbackError` 非空，也不能说未写或回滚；`readbackVerified` 与 `fileWritten` 分开。commit 未确认时 `fileWrittenKnown=false`，不把 `fileWritten=false` 解释为证实未发生。所有 fsync／durability 声明仍 false。load 不产生输出文件；成功 load 的 fileWrittenKnown=true/fileWritten=false 仅说明该动作没有写入，不是对输入文件的真实性证明。

`historicalSummary` 初始 `{}`；成功后固定11键：`schemaVersion=1,requestId,filePath,recordCount,headHash,readOnly=true,historicalOnly=true,authenticated=false,permissionsRestored=false,executionPermit=false,flightCommandReleased=false`。请求或失败不清旧槽；新 load 必须完成全量校验且未被取消才一次替换。save 不改变历史槽。

正常 `historyPage` 固定8键：`ok=true,error="",requestId,offset,nextOffset,totalCount,hasMore,records`，每页最多4条；offset 允许0..totalCount，终页可为空。每条摘要固定5键：`index,timestampMs,type,hash,previousHash`。正常 `historicalRecord` 固定10键：`ok=true,error="",requestId,index,timestampMs,type,hash,previousHash,payloadPreview,truncated`；不返回 payload 树。hash 是原完整 record 的链哈希，不是仅预览文本的哈希，也不是签名。新 API 的成功空串显式保留，不让 QVariant 的未知值变成显示序号0。

### 后台整份预算及兼容边界

沿用原文件32MiB、100000记录、单payload深16／50000节点／估算512KiB、类型128和完整时间／index／链哈希检查。共享校验器先核实际整数类型，再以宽域数值与0..100000内的期望index直接比较，拒绝巨大整数经窄化后冒用合法索引；合法旧格式不变。读取复用共享文件层的普通非符号链接文件、64KiB分块、完整尾部及UTF-8回环检查。新 loader 不先安装成功前缀，也不调用活链 import。原未来5分钟容差及时间非递减接受域保持，不认证文件声称时间。

保存会在任何整链序列化之前遍历冻结值：节点与字符串／key／base64长度形成累计下界单位，最多32Mi单位，超过即整份失败；不先生成可能100000×512KiB的JSON。随后逐条原 canonical 编码并累计实际文件字节，超过32MiB整份拒绝，才交 QSaveFile。递归始终有深度及单payload限制；取消在遍历／逐条hash／逐条编码间检查。小的单条规范化和Qt JSON解析本身不是可中断系统调用，不把这些预算称为全进程峰值内存或实时性证明。

新后台仅接收可纯值规范化的 Map／Hash／List／StringList／String／ByteArray／Bool／Int／UInt／LongLong／ULongLong／Float／Double和Qt日期时间类型；未知 opaque/custom QVariant 拒绝，不在worker调用任意转换器或读取GUI QObject。null仍按原canonical语义。Map/Hash顺序、ByteArray的base64、数值及日期的原JSON投影不变；重开得到的是规范JSON值，不承诺恢复原始QVariant元类型。新工作流还要求记录type为实际非null String且原长度≤128，以便摘要始终有界；正常原生append产生的旧文件兼容，旧同步接口接受域不改。

load 在后台为每条 payload 生成最多8192 UTF-16字符的 canonical JSON纯文本预览，末端不割开代理对，超出明确 `truncated=true`；所有预览累计最多32MiB **UTF-16字节**，不是100000×8192的隐含无限分配。超出则整次load失败、保旧历史。GUI页和详情只取已冻结摘要／预览，不重新serialize大payload；预览不是完整导出或执行内容，完整材料仍在原文件和已验证的私有候选内。

工程场景（全部未执行）：空链与100000条边界、错误count/head/类型、32MiB与单payload预算、巨大当前链在序列化前拒绝、保存期间继续append、覆盖确认／路径／symlink／坏尾、半写与commit未知、已写后readback失败／取消、load坏末条／错hash／未来时间／未知值、不替换活链或失败保旧历史、父销毁／单槽忙／迟到finished、旧历史UUID与同路径/同hash ABA、四条分页、8192代理对边界、累计预览预算、8KiB以上payload不在GUI全文序列化。仅源码及直接静读；未执行实现、文件IO、JSON／hash、线程、QML、编译、测试或预览。

下方为原接口合同。

`FleetMissionEvidenceChain` 是地面站侧的任务证据链。它只保存已经由地面站产生的证据，不访问车辆、任务管理器或 PX4，也不会发送任何 MAVLink/控制指令。

`GuidedActionsController` 会在意图预览、申请审批、批准和拒绝这四个明确的操作员流程节点追加摘要证据。摘要只包含任务 ID、计划/风险状态、事件数量和审批边界，并附带 `groundStationAudit` 的当前本地操作者/角色快照；其中 `identitySource=local_fleet_role_policy`、`authenticated=false`、`signatureVerified=false` 明确表示它不是数字签名或权限证明。证据链结果不接回命令路径；工程师仍需显式保存或导出链条，并在交接时核对 `verifyChain()`。

对已写入黑匣子的事务更新、编队异常、舰队降级和任务状态事件，控制器也会追加带原始事件快照的审计记录；普通意图事件不重复追加。证据链写入失败不会改变黑匣子、任务或命令状态。

## 记录内容

调用方可以按任务生命周期追加以下类别的记录：

- `mission_plan`：任务计划或编译结果摘要；
- `capability_snapshot`：能力、构型指纹和证据来源摘要；
- `approval`：人工审批、撤销或门禁决定；
- `risk_assessment`：风险雷达/任务预演结果；
- `event`：失联、低电量、Leader 切换等关键事件；
- `final_result`：任务结束结果和未完成原因。

每条记录包含序号、时间戳、类型、payload、前一条 SHA-256 和当前 SHA-256。时间戳必须保持非递减顺序；系统时钟回拨或调用方提交早于链尾的时间戳会拒绝追加/导入。payload 由调用方提供，建议只放摘要和引用，不放原始视频或大日志。为避免不受信任交接文件在递归规范化/哈希时耗尽资源，类型长度、payload 递归深度（16 层）、节点数（50000）和单条 payload 估算大小（512 KiB）均有硬上限；追加和导入共用这组限制。

## 接口与完整性

- `appendEvidence(type, payload, timestampMs)` 只允许追加，已有记录没有编辑/删除接口；
- `exportJson()` 导出带版本号、记录数组和 `headHash` 的快照；
- `importJson(json)` 先验证格式、严格整数版本、字段、序号、时间戳、前后哈希和每条 SHA-256，明显未来时间戳、未知字段或非对象 payload 都会拒绝；任何失败都不改动当前内存链；
- `saveToFile(filePath)` 使用 `QSaveFile` 原子提交，`loadFromFile(filePath)` 拒绝空路径、读写失败和超过 32 MiB 的输入，并在完整校验后才替换内存链；
- `verifyChain()` 可在交接或落盘前重新验证当前链；
- 当前实现限定单个快照最多 100000 条记录，防止误导入耗尽内存。

这不是防篡改存储或数字签名：SHA-256 链只能发现快照内容被改动，知道规则的写入者仍可重新计算整条链；当前链也不自行证明操作者身份，操作者/系统主体应由上层认证后写入 payload 并在交接时单独验证。本地文件接口只提供原子写入和大小边界，不提供加密、权限或留存/清理策略。工程师如需跨设备不可抵赖，应在上层增加签名、权限和持久化策略，并保留原始导出文件。

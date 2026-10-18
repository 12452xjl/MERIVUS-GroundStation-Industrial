# 原 Run 剩余路线规划取源（433，源码静态实现）

## 478：保旧source形状，新提案政策有明确来源

native单成员18／历史20、native整组collection合同不改。旧policyless原计划及payload只能经独立历史编码复现进行内容自洽核对，不回填原外环或恢复Run／批准。新policy计划则必须与同份sourceTask.area重建值完整相同；native getter、整组validator及single共享validator均执行此配对。

新生成single provenance改为native3／29键、history4／31键，group为2／27键，均保存完整canonical policy，并由task／plan两端消费者核对；原1／2和group1仍仅历史。实际当前编码／上传缺policy要求完整源重编译和重新审阅，不能从摘要、进度或latest资料猜政策。未编译、运行或测试；下方计数为历史。

下方为历史记录。

## 474：新增整组取源，原单成员18字段不变

StartupTransaction.remainderPlanningCollection(runId)冻结原共同task／plan与全部实际targets，一次observer.current后逐行核完整payload／源映射／队列身份／progress并尾核。common只存一次，原有序vehicleIds／targets闭合；任何坏行整份拒绝，合法未提交行保留available=false及固定原因。

新validateCollection及sourceFromCollection先完整校所有行和全组累计预算，只有真实begin＋commit的指定成员可投原native18；Archive v2保存完整16字段集合，明确选成员后转history20并绑定整包hash。原remainderPlanningSource及旧v1文件保持；旧缺项摘要不能重建本集合。单次本地读取不认证远端同时采样，available不认证任务完成。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473：原生取源不变，完整资料显式进入历史分支

本类 `remainderPlanningSource(expectedRunId, sourceVehicleId)` 的原 native18／schema1／`native-run-planning-snapshot` 合同不变，仍由同一实际 Run 取原任务、完整计划、唯一成员、完整载荷和对应观察。新[余段规划资料包](FleetMissionRemainderArchive.md)经[显式保存／打开入口](FleetMissionRemainderArchivePanel.md)保存单 Run、单原成员的整份材料；不是从复盘报告摘要、当前草稿、最新计划或 current/reached 序号补造原政策和路径。

文件整包校验后另行生成 historical20／schema2／`historical-run-planning-snapshot`，携带 `sourceArchiveId` 与 `sourceArchiveContentHash`。导入不调用本 getter 恢复实际对象，不把历史来源改名 native。原源与历史源共同使用 [Planner 的完整 validateSource](FleetMissionRemainderPlanner.md)；这只证明有界值的一致性，不认证历史资料、当前飞机或远端任务实例。原生 provenance28／schema1 与历史 provenance30／schema2 由同一中央校验分别接受，Compiler／PlanStore／View 已实际复用，原外层格式和门禁不放宽。

原完整任务数据与安全政策继续保留；人工选择仍包含原航点并重做完整停留，绝对时序要求仍拒绝而不静默删改，目标机／起返点／速度／执行配置需重新明确。后续必须重新取得当前证据并审阅；观察不等于完成，所有权限恢复、执行和命令放行声明仍为 false，不恢复 Run、控制权、占用或正式结果。本批仅源码与静态读回，未编译、执行或测试。下方为原生取源历史说明。

本模块只提供真实本地 Run 关联的历史规划值。它不裁剪航线、不证明已完成航段、不修改原任务、不读取新机载任务，也不发送任何飞行或任务协议消息。人工确认、独立新任务规划与新审阅由后续消费者承担。

## 原输入冻结

`FleetMissionDeployment::beginUpload` 从传入且通过原有身份/批准链检查的实际 `FleetIntentTask::parameters()` 读取完整副本；在可能回调的目标捕获、批准消费和原 `_gate` 之前计算本地 SHA-256，原 `_gate` 成功后复核原实际 intent、完整参数值及哈希仍一致，再保存规划用副本。哈希仅用于本地输入一致性，不是签名、认证、许可或远端内容证明。

整份原参数首先通过有界纯值检查，之后才剥除明确不可继承的运行采集结果：

- 顶层：`fleet`、`healthReports`、`nativeVehicleEvidence`、`capabilityReviewEvidence`、`templateValidation`。
- `reviewOptions` / `options` 内：`events`、`healthSnapshotMismatch`；原有其它政策保持。

`noFlyZones`、`riskSignals`、`constraints`、能力需求、模板选择、速度/时限/返程约束及其它纯值原输入均保留；不能假定 `riskSignals` 都是临时遥测后把显式风险输入静默丢弃。`planningSource` 如原本存在也保留，但它只是当时编辑输入，不是新的实时草稿、审阅或授权。

原参数和剥除后 task 各限 compact JSON 16 MiB、深度 20、节点 524288、单 map 1024 键、单 list 32768 项、单字符串 32768 个 UTF-16 单元，键限 1024。只接受 map/list/string-list、字符串、布尔、有限数值及明确 JSON 空值；QVariant 整数类型必须在可精确 JSON 数值范围内。QObject、其它 opaque 值、包括 typed-null 指针均不接受。即使问题值位于拟剥除字段，也不接受有效前缀；整份来源不可用。

捕获失败仅保留内部不可用原因及空规划来源，**不会增加或改变既有上传、启动准入条件**。每次新的 Deployment generation 重置旧来源。`planningTaskForRun(binding)` 是非 QML 的 native 值访问接口，仅匹配该实例原冻结 binding；不访问最新 intent/PlanStore、不从历史档案补默认来源、不要求当前健康或刷新任何证据。

`FleetMissionStartupTransaction` 在原 request 路径中从同一个实际 Deployment、相同 deploymentId 和 binding 取得上述副本，再由本 Startup generation 独立保存。旧实例没有保存原 task、输入不合规或身份不匹配时，只使新规划来源不可用，不阻断原 Startup 流程。

## remainderPlanningSource 契约

`Q_INVOKABLE QVariantMap remainderPlanningSource(const QString& expectedRunId, int sourceVehicleId) const` 返回固定 18 键：

| 字段 | 成功值 |
| --- | --- |
| schemaVersion / ok / reason | Int `1` / Bool `true` / 明确空 QString |
| sourceKind | `native-run-planning-snapshot` |
| runId / startupId / transactionEpoch / deploymentId | 原实际实例 UUID QString |
| transactionGeneration | 原 generation 的规范正十进制 QString |
| binding | 原冻结 7 或 9 键归档/审阅绑定 |
| sourceVehicleId | 原主任务成员 Int ID，1–255 |
| sourceTask | 原 Deployment 捕获的完整规划政策副本（上述剥除规则） |
| sourcePlan | 原 Startup 冻结的 `deploymentContext.plan` |
| sourceMember | `sourcePlan.members` 中唯一原成员，与 Startup target.member 相同 |
| sourcePayload | 该 Startup target 的完整原 PayloadBuilder 结果，包括 items、canonicalWireItems、contentHash、sourceItemMap |
| progress | 原 ExecutionObservation 当前一次值快照中对应车辆行，不是 UI 回传或导入报告 |
| readonly / flightCommandReleased | Bool `true` / Bool `false` |

失败仍为上述 18 键：`ok=false`、非空固定原因，其余身份字符串为空、sourceVehicleId 为 0、六个 map 为空，固定 sourceKind/schemaVersion 和惰性标志不变，不返回可误用的部分来源。空来源、未准备 Run、目标未真实 MODE 入队提交、非唯一成员、原 payload 或映射不相符、取源重入/生命周期变化以及资源超限均明确失败。

最终完整 18 键信封限 compact JSON 32 MiB、深度 24、节点 1048576；单 map/list/string/key 上限同原参数。初步纯值检查在纯 PayloadBuilder 重建之前进行，最终加入 progress 后再次检查。所有资源失败均整份拒绝，不截断路线或成员。

## 来源核对与历史语义

原实际 `_executionObservation` 必须 prepared，绑定本 Startup generation/runId/transactionEpoch/deploymentId/binding，且所选目标有真实 `modeQueueBegan && modeQueueCommitted`。目标原 payload 的 hash、itemCount、完整 sourceItemMap、mode step、原 READ 引用和 observer 的冻结值一致；从 sourcePlan 纯值重建 payload 必须与原 target.payload 完整相符。重建只核对冻结值的编码一致性，不是机载新 READ，也不是本轮已执行测试的证明。

取 progress 前后持有 QPointer 并核对真实 owner/generation/冻结来源，复制 target 值，不在可外调 `current()` 期间持有目标容器引用；专用只读重入 guard 不永久改变原 Run。原 sourceCurrent=false、失联、retired 均可保留历史提示；不会为取得草案要求新的飞行健康、恢复旧许可或调用 terminalReassessmentContext。

`progress.currentSequence/currentMissionItem`、`reachedSequence/reachedMissionItem` 若存在，必须逐条匹配原 payload.sourceItemMap。`sourcePath=waypoints/returnPath` 的 sourcePointIndex 是原 compiled member 路线 0 基位置；速度、起飞和终点动作没有航点索引，不能对 wire sequence 减固定偏移。未观察值保持缺失，不能补 0。

后续进度推进不修改已返回的值快照；新的草案应绑定该快照与人工选择。`sourceCurrent` 只表示原本地观察来源当次关联，不证明远端实例、位置或已完成任务。重复/跳号/倒退、连续性缺口、未知尾段和原 `currentValuesFreshnessVerified=false` 等信息均完整保留。最后 reached 不代表连续完成前缀，当前航点之前的区域也不能自动宣称已完成。

## 验证与交接边界

本轮仅源码修改、静态生产消费核对与文本检查；没有在本机编译、执行 helper、运行应用、仿真、硬件测试或刷机。工程师仍须验证正常与退休 Run、部分 MODE 提交、原参数在回调期间变化、旧缺来源、原任务策略保留、资源边界、重入/对象销毁和 UI 取源切换。不得将取源成功称为剩余航段安全、当前飞行位置可信、实飞验证或新任务可直接执行。

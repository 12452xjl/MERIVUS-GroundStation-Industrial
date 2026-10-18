# 实际 Run 简报交接工作流

## 539：文件保存、历史预览和仅追加合并

在436的真实Run简报和本地收件箱之上，受限门面增加`historyState`、`historyPreview`、`historyResult`三个脱离原生对象的值投影。新的文件工作区不暴露旧`loadFromFile()`整库替换入口，不接管当前运行、权限或控制租约。

明确操作链为：指定本地JSON文件 → 确认保存当前完整记录快照；或读取文件 → 查看数量、摘要、四条一页的候选记录及独立详情 → 再确认仅追加历史。读取本身不改当前收件箱；同ID的规范持久内容相同则跳过，内容不同整次拒绝，不覆盖当前记录。成功合并的新增记录保持历史隔离，不能处理为当前交接。

`requestHistory(intent, confirmed)`只接收恰好五字段：四个String `action / expectedJobId / filePath / previewId`和Bool `overwriteConfirmed`。必须明确确认；原jobId必须仍当前，未使用字段必须为空／false。保存另需明确允许覆盖指定目标；加载不写目标文件。save/load/merge都返回独立原生作业的受理回执，**受理不是完成**；cancel/discard绑定准确旧作业／候选，不自动重试。

后台工作使用冻结的原生值快照，不把100000条记录或32MiB文件交给QML编解码。加载候选私有保存，`historyPreviewPage(previewId,page)`与`historyPreviewRecord(previewId,id)`只按需返回小页和单条详情。合并后台生成整份候选，提交前核原集合独立代次，防旧文件导入重设revision后误接受旧结果；实际线程结束前不归还工作槽。

保存沿共享原子文件写入／字节回读接口；已commit后取消或回读失败仍保留已写事实，不宣称回滚、断电持久化或身份认证。公开结果、候选和回执均固定`historicalOnly=true`，权限恢复、执行许可、飞行命令释放均为false。导入文件的哈希是完整性标识，不是收发账号签名。

门面保实际Manager与控制器生命周期，外来intent取值后再核原对象；原生通知后的返回值仅作为脱离对象的事实回执处理。界面确认、候选来源、作业状态和失败恢复见[界面说明](FleetHandoffPanel.md)，完整DTO、预算、合并和兼容边界见[原生说明](MissionHandoffManager.md)。原有schema1文件与旧C++导入接口保留；新历史流程的隔离承诺不反向改写旧接口行为。

仅源码开发和限定静读，未执行保存、加载、合并、解析、哈希、线程、QML、编译或测试。跨站账号／同步、控制权移交及正式机载接替仍未由此完成；不制作阶段源码包，按原全部约定目标继续到统一源码交付。下方436为历史说明。

## 436：本地人员确认，不是飞机控制权转移

`GuidedActionsController.qml` 的 `missionHandoffManager` 现在公开受限门面；指挥中心的 [FleetHandoffPanel](FleetHandoffPanel.md) 使用它，不再自由填写任务 ID 或附带当时最新的提案记录。后端由 [ExecutionReport](FleetMissionExecutionReport.md) 生成有界简报，[MissionHandoffManager](MissionHandoffManager.md) 保存请求、版本和接收结果。

本批只改源码与文档。未编译、运行、解析、测试、认证、仿真或刷机，不能据此声明运行可用、跨站交接完成或飞行放行。

## 实际来源与冻结

1. 操作员使用现有原生账号入口登录并选定运行来源。发起人取实际 `FleetOperatorSession` 的身份收据；角色标签仅供展示，不能单独授权操作。
2. 界面传入所选 `fleetObservationSourceToken`、实际 `runId`、接收 SID 和原因。`prepareRunHandoff()` 在实际 selected-observation 引用范围内复用 `_fleetExecutionReportSample()`，核对最新实际 owner、Run、startup、deployment、transaction epoch/generation 与 sourceHash。
3. 简报必须有真实注册的 `executionObservationTaskId`。原规划 `sourceTaskId` 和本地观察任务 UUID 是不同字段，不能互换。没有注册任务时拒绝，不制造 ID，也不回退到手填任务或最新提案。
4. 简报冻结准确 binding、最多32架的历史摘要、观察未知态及内容哈希。已经退休、尚未提交 MODE 或部分提交的真实注册 Run 可以成为历史简报；它们不因交接变成成功或当前机载状态。
5. 捕获前后检查实际来源和账号收据。合法遥测进度变化不要求整份观察逐字相等；实际 owner／事务身份改变则使旧候选失效。公开 preview 是脱离私有状态的值副本，不包含原生 QObject 或认证收据。

## 发送与处理合同

门面提供：

- `prepareRunHandoff(sourceToken, runId, recipientActorId, reason)`：生成独立 preview token，接收 SID、原因和发起身份一起冻结。
- `sendRunHandoff(previewToken, confirmed)`：必须明确 `confirmed === true`，复核同一原生身份收据、用途角色快照及 Run 来源；进入原生请求前一次消费候选。不能修改被冻结的接收人再沿用确认。
- `clearRunHandoff(previewToken)`：只清同一候选，不删除已发送记录。
- `handoffPage(actorId, filter, page)`、`handoff(id)`：有界分页与按需详情；全部本地历史可无飞行账号只读浏览。
- `resolveRunHandoff(id, revision, briefHash, action, note, confirmed)`：对准确记录版本及简报哈希作明确确认。仅当前真实接收 SID 可接收／拒绝，原发送 SID 可撤销；接收不要求原发送人仍登录，也不要求原 Run 仍在当前选择中。

本地请求固定300秒并要求不同接收账户确认。SID 格式检查不证明账户存在、拥有权限或对应另一名自然人；本批没有已验证接收人目录。原生接收时仍核实真正登录的 SID 和 `task.handoff` 权限。没有传输到另一台地面站的动作。

门面将页和详情转换成脱离原生包装的JSON值副本，页面的数组／字段检查不依赖Qt QVariantList包装类型。原生生成简报已将未知序号规范成显式null，因此转换不应省略未知字段；此往返仍列入工程师验证。

每次变更返回六字段 `{ok,error,committed,current,handoffId,token}`。`committed` 是 true／false／null，null 表示调用中断后结果未知，不能当作未发生。准备／清候选不是记录提交，保持 false。原生创建返回的 ID、已完成状态变更即使随后上下文失效也保留；过期检查可能写入 expired 并使接收返回失败。不要自动重试。`current` 仅表示本次调用仍匹配捕获的上下文，不代表飞机状态新鲜或执行许可。

原始 Manager 的通用 C++ 接口仍保留，但门面不再提供自由任务请求或旧的未确认 accept/reject/cancel 方法。旧本地无简报记录只能以“旧审计记录”明确查看和处理；不能冒充实际 Run 简报。

## 状态、容量与边界

简报根17字段、摘要13字段、逐机14字段；不把2 MiB完整报告塞入旧128 KiB metadata。独立简报最大96 KiB、1024节点、深度5，未知序号显式JSON null。最多256份Run简报，不自动淘汰历史；通用记录和导出文件仍各有原独立预算。达到预算应明确拒绝，不能默默删历史或缩小目标组。

本批新入口只操作当前 Manager 实例的本地记录。Manager已有文件API不是跨站同步服务；导入记录标记 `origin=imported`，该来源标记不能从文件恢复为local。门面拒绝对任何导入记录执行交互状态变更，原生后端特别拒绝导入Run简报的接收／拒绝／撤销；历史仅供查看。没有新增自动持久化、导出／导入界面或崩溃恢复承诺。

简报哈希用于内容一致性，不是数字签名。接收不释放成员占用，不改变原任务、飞行模式、机载任务、启动批准或控制租约，不发送MAVLink命令。跨站账号／同步、真实控制权交接、中途加入及正式机载接替仍属原25目标的后续开发。

工程验证待办见 [工程师交接说明](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)，本批实际修改路径见 [补丁清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

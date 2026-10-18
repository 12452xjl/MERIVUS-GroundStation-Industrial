# FleetMissionReportBuilder 契约

## 546：四受众审阅资料的真实用户交付

新增native-only纯值`validateReport/encodeReport/decodeReport`，保持原v1摘要及boundaries接受域，供独立[报告工作区](FleetMissionReportWorkspace.md)后台复用。原build/view/exportJson/saveToFile/loadFromFile接口及旧有效格式不变。Coordinator停止在已哈希的report根追加两字段，报告内部固定边界与review外层边界保留；原先事后变更会导致报告自身view/export/load摘要拒绝，现在当前真实report可被复用。

四类视角及明确冻结/保存/独立重开由[新界面](FleetMissionReviewReportPanel.md)实际消费，不改PlanStore默认reportIncluded=false，不重build而篡改原事件/生成时间。完整文件含工程原值，客户视角不是脱敏文件或权限。仍为飞前审阅资料，不是protected Run报告、事故结论或正式完成。没有运行/解析/hash/文件往返、编译、测试或硬件操作。

## 541：退出模式观察不替代终点或结果候选

原实际 Run 事件消费者沿 Codec 纯值校验接纳正常 schema2/exact29；读原 schema1/exact28 仍兼容，capture-gap 保旧版。没有新增报告格式，也不从任何退出值推断 Failed、Abort、Completed 或事故原因。原 outcomeCandidate 及 reviewRequired 的只读等级解释保持。

新版可读 detail 保留模式退出计数、首末 raw customMode/baseMode/systemStatus、各自 priorMissionReceipt/exitReceipt 的本地 epoch/十进制序号/单调毫秒与 continuityUnknownAtExit。大序号不经浮点转换，不把单调时间改为 UTC。原事件仍在既有 raw 中完整保留；仅首末收件对而非全心跳流，模型提交前多个边沿可以合并，不能宣称每次退出原值都已归档。旧版明确“未采集”，新版零计数也只表示未观察到，不证明继续执行或完成。

这些值由已核原事件提供，并非远端身份认证、请求因果验证、任务结果认证或自动动作依据；报告构建器仍无 Vehicle/命令入口。v4 持久化仅为上述退出对允许本地 source epoch/seq/ms，旧 v3 隐私投影不回填。4096字节上游事件/原FIFO与本报告预算均不扩张。仅源码静读，未执行模型、编码/解析、QML、编译、测试或设备操作；以下历史说明适用于相应旧版本。

## 510：实际Run语义事件进入现任务报告

原build的events消费者新增辨认精确type/source和Codec纯值校验通过的Run观察事件，不新建报告格式或替代完整[FleetMissionExecutionReport](FleetMissionExecutionReport.md)。原始事件完整保存在既有timeline／issue的raw；只在本地副本中派生可读原因、候选、历史异常位图和实际单机ID。全局capture-gap不猜机号／任务，累计丢弃明确属于原生事件流。

正常注册、MODE队列事实或单独未认证COMPLETE报告，按真实reviewRequired=false仅入info时间线，不因旧severity缺省warning而误列为事故。真实reviewRequired=true事件仅作为warning人工复核项；不升级critical、事故原因、飞行停止或正式任务完成。精确type的Run事件若摘要／来源文本不合格，仅增加unverified原因，时间线为info、无推定机号／动作；不沿generic缺省warning或caller的事故旗标另造incident，raw仍保原值。type／source须为非null的真实QString，不用QVariant转换比较认定。静态值校验／来源字符串不是原生来源、签名或远端认证。

日志timestamp按BlackBox实际接纳时的本机UTC解释，runObservation.capturedAtUtcMs继续保原生语义捕获UTC；不为排队延迟改写捕获时间，也不凭UTC排序宣称物理因果。报告原有排序／坏时间标记和文件预算保持。没有读取飞机、恢复Run、自动完成／失败、重分配或任何控制调用。

原build/view/export/save/load及独立历史观察21卷宗合同不变。事件来源、28字段与v3持久摘要见[事件合同](FleetRunObservationEvent.md)和[实际日志接线](FleetRunObservationJournal.md)。仅源码与静态阅读，未执行ReportBuilder／Codec／hash／QML／文件往返、编译、测试、仿真或硬件操作。下方为历史。

## 487：独立历史观察static入口，不混旧任务报告

新增static buildTelemetryReview(nativePage,positionValuesConfirmed)，非invokable、不更新_lastReport、不emit reportChanged、不读当前时钟或文件；只一次调用MotionReview后构建[观察卷宗21](FleetTelemetryObservationReport.md)。原build/view/exportJson/saveToFile/loadFromFile和飞前任务报告格式保持。新链不调用旧build，旧build仍接受通用Map、没有新增dossier类型门；新dossier没有旧reportId/boundaries组合，旧view/exportJson/saveToFile/loadFromFile不直接接纳它。本链不写原任务报告或正式运行结果。

保完整31及原页六身份、覆盖8、最多16条资料质量发现；按真实页的聚合帧／样本／拒绝／分组／断段和分页等状态填四种展示视角，不只输出模板。customer子视图无实际ID／文件hash／逐机数值，完整工程卷宗仍含已确认派生资料；展示视角不是权限服务。

body移除dossierId/contentHash后以Qt compact JSON SHA256生成内容标识，整体≤1MiB，超限整份失败，不截原31。hash不认证来源／签名，不将资料缺口当事故或性能诊断。原[Archive](FleetTelemetryArchive.md)review实际一次消费，原[UI](FleetTelemetryHistoryPanel.md)可人工选择复制文本，不新增文件API。34工程场景全未执行；本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为旧任务报告合同，新历史格式仅本节扩展。

## 429：与实际运行报告分工

本类仍用于飞前计划／风险等已有快照的报告。实际selected Run的模式队列、航点观察和历史候选由独立[FleetMissionExecutionReport](FleetMissionExecutionReport.md)及其[面板](FleetMissionExecutionReportPanel.md)承担；两者schema不可互换，运行报告不会写回本类lastReport或恢复任务状态。

`FleetMissionReportBuilder` 是地面站侧的只读报告构建器，用于把已经采集的任务计划、风险评估、逐机健康、能量计划、证据包和事件快照整理成可交接的报告。它不重新读取 Vehicle，不解析原始 MAVLink，不修改任务，也不释放飞行命令。

## 输入与输出

```text
build(missionPlan, riskAssessment, healthReports, energyPlan, evidence, events)
```

输出包含：

- `reportId`、`schemaVersion`、`generatedAt`：报告身份和版本；`reportId` 是正文摘要，加载和视图接口会重新校验；
- `missionSummary`、`verdict`：任务摘要、阻断/复核/清晰状态和最严重等级；
- `incidents`：由事件、风险项、健康异常和能量不足归一化的事故/异常列表；
- `timeline`：事件时间戳可解析时按时间排序，未知时间保留输入顺序；事件项不是对象、时间戳缺失/非法/非正数或明显未来时不会被悄悄当作可信输入，报告会加入 `unverifiedItems` 的 `events_input_invalid`（并在 `inputs.invalidEventCount` 计数）；
- `unverifiedItems`：缺失健康、能量、覆盖、构型或证据的明确清单；
- `evidenceSummary`：证据包是否提供、是否显式验证，并保留证据链摘要和追溯状态；缺失或非法 `evidenceHeadHash` 会进入未验证项；
- `recommendedActions`：给操作员/工程师的人工复核建议；
- `views.commander`：指挥员的决策摘要、关键异常和短时间线；
- `views.customer`：客户可读的结果、服务证据状态和限制说明；
- `views.engineer`：原始输入快照、完整风险、健康、能量、证据和时间线；
- `views.safety`：保守门禁、关键异常、未验证项和审计字段；
- `boundaries`：固定包含 `reportOnly=true`、`manualConfirmationRequired=true`、`flightCommandReleased=false`。

`view(audience, report)` 只允许 `commander`、`customer`、`engineer`、`safety` 四个受支持视图；未知 audience 或摘要校验失败返回空 map，不会把其他视图的数据误当成目标视图。`exportJson(report)` 输出缩进 JSON，适合交接、审计和存档；`saveToFile(filePath, report)` 使用 `QSaveFile` 原子提交，`loadFromFile(filePath)` 返回已解析且摘要校验通过的报告对象。文件接口拒绝空路径、读写失败、非法 UTF-8 和超过 32 MiB 的输入，不会把加载结果自动当成任务批准。

## 保守判定

- 任务计划不可行或存在 critical 异常时，报告状态为 `blocked`；
- 存在 warning/major 异常或未验证项时，报告状态为 `review`；
- 没有异常且输入证据齐全时才输出 `clear`，这仍不是飞行放行结论；
- 缺少任务成员的健康快照、能量计划或证据包会进入 `unverifiedItems`；
- 完成概率、覆盖率和客户结果只能沿用调用方提供的证据，不能由报告构建器臆测；
- 事件没有可解析时间戳时不会伪造时间，按输入顺序保留并标记为 `events_input_invalid`；明显未来时间同样视为不可验证；非对象或空事件项直接跳过，但同样计入未验证项，避免损坏输入生成 `clear` 报告。

## 安全边界

本类不得持有 `Vehicle`、`MissionManager`、MAVLink 发送接口或自动任务调度器。报告中的 `recommendedActions` 和安全门禁都只是人工复核建议，工程师接入 UI、持久化或导出时仍需保留原始输入和报告版本，不能把报告状态直接映射为解锁、起飞、返航或降落命令。

编译、Mock/SITL、日志回放、硬件测试和刷机放行由工程师负责；此契约本身不能证明任务完成率、避障能力或飞行安全性。

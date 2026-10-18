# 524 旧部署归档与本地容量回收面板

[FleetDeploymentArchivePanel.qml](../../res/Merivus/FleetDeploymentArchivePanel.qml)嵌在原[部署生命周期弹窗](FleetMissionLifecyclePanel.md)内，沿原指挥中心、普通任务规划两个入口到达。只接实际 Guided 的 `deploymentArchive` 窄门面；不创建另一 Lifecycle，不公开原scope、Episode、QObject或批准凭据。原两个宿主无需另加顶层入口。

## 明确操作链

1. 展开归档区只读当前目录，不自动选第一组。用户明确选择非当前任务组、非当前监视组、未处于退休过程且原来源仍保留的旧组。最多8行，未发布的失败保留组也可列出；行值不证明已闭或可回收，原生仍核完整实际状态。
2. “只选择归档路径”只打开一个 `QGCFileDialog` 并暂存路径，不读写文件。目标、当前生命周期代次、控制器或可见性变化后旧路径与确认失效。
3. 用户另确认“已核对旧本地观察已闭、旧飞机可能仍飞、只回收本地”及准确路径 / 覆盖。无论文件是否已存在都需覆盖确认，不从文件存在性推断同意。按钮随后只发一次 `archive-retire`。
4. 保存、commit、回读、原生最后来源 / 闭合核对、提交退休、实际对象销毁与回收容量是不同事实。正在销毁仍占槽，实际销毁后才显示回收；退休提交后取消不能回撤。取消、失败或隐藏不证明文件未写，不自动重试或伪造回滚。
5. 独立打开历史是另一条显式选路径、另确认的只读操作，无需 live 飞机、账号或新部署。打开成功只显示小目录，不自动读取第一份报告。0份报告合法：可以是未使用 / 未发布组，也可能曾有Run但未注册任务；类型按原生quiet证据判断，不能从reportCount猜类型或任务完成。
6. 用户按序号明确读取一份原保护缓存报告。原生验证并提供完整canonical JSON文本；界面不对整份报告 `parse` / `stringify`，只按最多2048 UTF16字符的原文片段前后浏览，保留代理对边界和实际offset栈，不裁剪、执行或恢复live对象。每份文本最多2MiB。

没有 STOP、START、LAND、RTL、ARM、DISARM、任务完成确认或权限恢复。旧飞机可能继续飞；组销毁及本地容量回收不证明远端已停、任务归属、遥测完整或正式结果。需要飞行安全处置时仍用既有独立安全操作。

## 公共值合同与真实来源

门面仅 `state`、`result` 和 `request(intent, true)`。界面不能通过目录、UUID、hash或文件Map重建原生来源。

`state`固定17键：

```text
schemaVersion / lifecycleEpoch / lifecycleRevision / busy / jobId / action /
stage / cancelRequested / lifetimeAllocationAttempts / liveAllocationCount /
capacity / capacityRemaining / candidates / historicalOnly /
permissionsRestored / executionPermit / flightCommandReleased
```

schema为1，epoch是原生命周期UUID；revision和终身尝试为正canonical uint64字符串。容量固定8，live占用含保留及待销毁对象，余量为8减占用。stage闭集为 `idle/running/cancelling/destroying/complete/failed/cancelled`。候选最多8个、UUID唯一，每行固定6键 `episodeId/published/current/selectedForObservation/retiring/sourceRetained`。历史标记为true，恢复 / 执行 / 飞行权限标记恒false。新建及选择本地监视仍使用原接口，不由此门面恢复。

`intent`固定9键：

```text
action / expectedJobId / lifecycleEpoch / lifecycleRevision / episodeId /
filePath / archiveId / reportIndex / overwriteConfirmed
```

所有动作均带当次实际state的epoch、revision及原job。`archive-retire`只额外带旧episode、路径、覆盖true；`open`只带路径；`report`只带自己打开所得archiveId及0..reportCount-1序号；`clear-file`只带准确archiveId；`cancel`只取消原job。未用业务字段为 `''/-1/false`，不靠truthiness降级或补新来源。

受理回执固定11键 `schemaVersion/action/accepted/jobId/reason/requestOnly/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。`archive-retire/open/report`各有新的唯一job；`cancel/clear-file`同步，不创建新job，不等待不存在的clear完成结果。cancel最终业务结果仍保持原动作，不被改名为cancel。

业务结果固定15键：

```text
schemaVersion / jobId / action / ok / error / episodeId / reply /
retirementCommitted / episodeDestroyed / capacityReclaimed / retiredTaskCount /
historicalOnly / permissionsRestored / executionPermit / flightCommandReleased
```

三个退休事实都是actual Bool，退休任务数0..9。界面缺失或异常时保未知，不能补成false。`ok=false`不抹已写入 / 已退休事实。门面state/result是两条QML绑定，刷新先后不作假设；仅在原job及原action结果同时到达后消费本页pending票。

保存reply固定12键 `archiveId/contentHash/path/fileWriteAttempted/fileWrittenKnown/fileWritten/readbackVerified/readbackError/fsyncVerified/durable/authenticated/summary`。写入已知和已写分开；unknown不能显示“未写”。原生前置失败可为完整空reply，此时文件事实未知而不是半份summary。fsync、durable、authenticated恒false；回读匹配不是来源认证。

`open`直接返回summary9，保存reply内也为同形：`archiveId/episodeId/lifecycleEpoch/capturedAtUtcMs/published/retirementKind/reportCount/contentHash/historicalOnly`。类型为 `closed-observations/unused-native/unpublished-pristine`，报告数0..9。保存时捕获UTC是本机归档时刻，不是任务结束时间；打开文件中的UTC仅文档声称，未认证，也不是本次打开时间。hash只是内容对应，不是签名或物理身份。

`report`返回固定3键 `archiveId/reportIndex/jsonText`。本页只认自己实际open回执及后续同archive/index的实际job，不从全局latest自动认领文件、报告或权限。归档UUID是文件声称值，不能当原生打开槽身份：任何本页未认领的新job变化都退休原history、reportView、路径及确认，即使后来文件声称相同archiveId也不能沿旧summary读取新报告。只有原owner及实际pendingJob精确匹配当前job时可保同一报告来源；本页新的open一经受理也先清旧历史显示，避免隐藏后未采用新结果却仍沿旧目录。即使原生打开失败而保留旧cache，界面也不恢复旧历史操作资格，须核对后另行明确打开；文件事实不回滚。清槽只清准确的本地打开缓存，不删除文件，不清live，不抹已写历史事实。

## 生命周期、确认与延迟回执

界面持独立plain lifetime、全调用ticket、界面代次和确认代次。每个按钮冻结按下时来源；选路径的动态对话框另有实际对象票。调用前消费本次确认，逐Qt通知核原票和来源；确认A→B→A不复活。ticket保持到结果发布及最后busy通知之后，销毁后release只清原plain票。

隐藏、收起、控制器替换、销毁或选源变化退休旧路径和确认。整段清理持plain retiring / ticket围栏，包括换控制器时清本地选择、历史和pending；外来动作不能在清理通知中插入新job再被旧尾部清掉。已有本页job只用原owner、job、epoch/revision精确请求取消，不按共享latest取消别人的操作；请求返回前退场则对实际返回的新ID作精确late取消。可能已写文件或已提交退休的事实不被取消或关闭伪装成未发生。新生命周期revision不会被当旧source沿用；原job返回的文件及回收事实仍独立保存为历史，不复活旧确认。

job通知在调用或清理ticket内到达时只置plain dirty标记，不丢通知、不在旧ticket内放行新动作。原ticket释放后，先以本页实际返回并安装的pending回执判定归属，再按原owner / 门面 / job身份受控drain；未认领变化在同一清理围栏中清旧历史显示，保已知归档事实。自己的report正常延迟绑定不推进界面代次、不自锁；clear同步回执不制造不存在的新job。

弹窗销毁后重开可另看“原生上次归档结果”纯诊断文字，闭合15键及保存12键后分别显示已写、回读、退休、销毁和回收事实。该显示明确不是本页认领、不是当前飞行状态或操作依据，不据此恢复pending、选中组、文件路径、确认、历史槽、账号或权限。共享last-result仅供解释旧事实，与本页按真实返回job采取操作严格分开。

采用现QGC主题和系统字号，短复选标签配完整PlainText说明、最小44高按钮、键盘Tab及显式错误焦点；沿原弹窗的限高滚动，不再叠一套长列表滚动器。JSON始终PlainText，长ID及文本可换行，不依赖颜色表达结局。

## 工程验证边界（全部未执行）

当前仅源码和文档静读。未执行技能搜索脚本、QML / JS / 解析器 / helper、应用、预览、编译、测试、硬件操作、认证、提交、PR或打包。须在完整工程验证两入口、空 / 满槽、失败保留组、未闭组、当前及监视组拒绝、保存成功而最后来源变更、取消各阶段、实际destroyed才回收、文件写入未知 / 回读失败、同job双绑定先后、两宿主竞争、同archiveId不同hash的外来open、ticket内被挡的job通知、own open隐藏后不留旧目录、自己的report延迟绑定不自锁、隐藏 / 销毁 / 换源迟回、确认ABA、清准确历史槽、0 / 9份报告、全文代理对边界、窄屏 / 大字号 / 亮暗主题 / 键盘与辅助技术。

本批服务于连续部署时保全旧材料及释放有限本地槽，不是完整任务执行闭环、最终StartPermit、飞控停止、可信正式结果或原25目标完成；原六类实质剩余保持。

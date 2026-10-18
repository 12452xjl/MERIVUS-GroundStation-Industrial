# 原规划修复副本入口（427，静态交接）

## 485：修复／保存／历史三来源均识别性能政策

Root共用_fleetPlanningSourceVersionKnown支持schema5 iff新实际Bool存在，允许原clearance／exclusions／setback任意组合；旧1..4仍按各自精确settings键集拒绝新键，不在修复时改为默认false。原生完整source validator和load继续是权威，不靠此预检认证内容。

现有deployment-repair、archived-plan仍要求完整可构建来源；saved-draft仍只要求结构合法editing来源。三者保新flag，候选revision1／datum撤确认；切回备用草稿也不丢政策或恢复审批。新政策编辑器pending已进入共享inputSettled，未处理原输入时不能从这些入口替换草稿。

无本面板或修复后端额外源码改动，复用完整源传递链；原唯一会话备用／原ticket／来源重读与通知后事实保留继续。未编译／运行恢复或UI／测试，32工程场景全未执行。下方为历史。

## 472：三来源共享提交与唯一会话备用

新增[编辑草稿保存／恢复面板](FleetMissionSavedDraftPanel.md)及[固定本地单槽 Store](FleetMissionDraftStore.md)，提供独立 `saved-draft` 来源，保留结构合法但尚未完成的原生编辑状态。它与原 `deployment-repair`、`archived-plan` 分开取源：原失败部署和历史记录仍走严格 `validatePlanningSource`／`loadPlanningSource`，不因新编辑保存能力而接受不可构建的历史规划；只有新保存来源调用 `loadEditingSource`，得到的只是待继续编辑的副本。

Root 的来源版本预检补齐 `clearanceInput` 内层 schema 2 的 `contacts` 字段，原 schema 1 及外层五键来源 schema 1／2 保持。预检仅校版本和字段对应，仍由原生完整验证全部值，不丢非活动路线／profile／reserve 声明或完整工程资料，也不把版本通过当成净空、实物或许可认证。

三个入口复用 `_createFleetMissionDraftCopy`、同一操作 ticket／候选对象池、当前草稿和**唯一一份会话内备用**。已有备用时仍须明确确认替换；旧当前先保留，只有新副本成功发布后才处理获准替换的更早备用。原 scope 撤销旧上传批准和旧审阅／启动相关批准、重核完整原源、退出桥接后再发布的次序保留；恢复或切回均清地图基准确认，重新构建／审阅／审批。不恢复批准、账号、当前设备证据、机载任务、控制权或飞行权限。固定磁盘编辑槽不等于这份会话备用，也不为三种来源各扩一份备份；新保存入口的确认、完整重读和提交／当前性事实以链接合同为准。

本增量仅源码与直接静态文档核对，未编译、执行 helper／解析器／业务代码／UI 或测试，未进行设备操作；不是原修复面板、草稿恢复或完整产品的全功能验收。下方历史正文保留，其“两入口”范围由本节明确扩为三来源共用，而原修复来源门禁不变。

## 实际入口与边界

`FleetMissionDeploymentPanel.qml` 在独立整组对账之后提供“读取原规划来源”“创建并切换到可编辑修复副本”“取消来源预览”和“切回上一个草稿内容”。打开面板不会读取来源或自动创建。创建、替换旧备用、切回的确认默认不勾选；按下与执行间核对控制器、当前 Episode、本地草稿选择/修订、来源令牌和对账请求。面板隐藏、控制器替换或 Episode 令牌改变时清除三个确认和四个按钮的按下意图；隐藏状态不接受本地修复操作，不因正常同步调用置 busy 就清除已捕获的动作。

只支持原失败/取消部署的独立对账已经进入终态、待处理数为零且双方传输/占用排空的来源。不同或未知对账结果允许人工继续编辑，不代表已经修复；本入口不要求飞机在线或借用历史身份、设备证据。其它真实读取、刷新、上传或启动链未结束时不能切换。原生后端仍决定后续实际审阅与执行门禁。

来源是原部署归档记录中的完整 `planningSource`，不是由编译航线反推的区域。旧归档没有保存该字段时明确返回 `original-planning-source-not-recorded-cannot-restore-draft`，不能猜测或补造。完整区域、成员（含非激活成员）、端点、保留路线、配置及执行意图仅保留在 Root 私有状态；公开 facade 只含有界摘要和本地结果。原规划有效上限为 64 个区域顶点、32 个成员、4096 个保留路线点；完整源交给原生校验，不套摘要用的小节点复制预算。

## 来源与提交次序

428 历史归档恢复入口复用本节的候选提交、ticket、当前/备用和销毁等待机制，由 `_createFleetMissionDraftCopy` 统一执行；来源 kind 分支保持隔离。427 仍只从原实际失败/取消部署及终态排空对账取得来源，不因 428 支持独立档案恢复而放宽该门禁。两个入口共用一份本地备用，不分别保留两份；详见 [历史版本恢复说明](FleetMissionArchiveDraftPanel.md)。

1. `inspectFleetMissionRepairDraft(deploymentId, requestId, draftToken)` 在原 Episode 桥接中调用只读 `Deployment.repairDraftSource`，保存当前实际 Draft、原修订/选择令牌和原 scope；退出 lease 后发布有界摘要。
2. `createFleetMissionRepairDraft(sourceToken, replaceBackupConfirmed, confirmed)` 再次核对原源，退出 lease 后创建尚未接入当前 Draft `Connections` 的新对象，调用 `loadPlanningSource`。加载只可进入全新 revision 0 实例，成功本地 revision 1；不沿用历史修订号。
3. 新对象成功装入后，再在同一原 scope 撤销旧上传批准和旧预览/审阅/启动相关批准，重新核对原源。退出 lease 后才切换当前 Draft 指针，保留之前的当前草稿作为唯一可切回备用。不会因新 Draft 的 `changed` 信号使仍持有的 native 调用自我失效。
4. `restorePreviousFleetMissionDraft(draftToken, confirmed)` 只清除未选中备用的 `mapDatumAcknowledged`，保留其余内容；捕获配置后的新修订，再撤销旧预览/审批并退出桥接，最后互换当前与备用。配置部分成功而后续切换失败时返回 `backupChanged`，不声称备用完全未变。

创建与切回均要求重新确认地图坐标基准、重新生成审阅和审批，不自动 `prepare`。仍沿用现有意图任务 ID，新审阅建立新归档版本，不宣称新任务 ID。原部署、原计划、机载任务和历史对账不会被改写；没有自动上传、修复指令、解锁、启动或许可恢复。

Planner 的显示缓存刷新与历史恢复共用：通过 `Qt.callLater` 等本轮同步发布结束，复核原控制器、Draft facade、选择代次、busy 已解除及 `inputSettled` 后，才刷新成员端点/分页并清除已结束的执行配置回执。暂时忙或有未提交输入时保留刷新请求，仅由 busy 解除或输入重新稳定触发，不轮询、不清除未提交文本；换源使旧请求失效，销毁后不再访问面板。

## 本地对象、取消与失败

初始 Draft 永不销毁；稳态至多保留当前/备用两个动态对象，允许一次临时候选或待销毁对象，峰值三个动态对象加初始对象。已有备用时必须明确确认替换；只有新副本成功发布后才请求销毁不再选用、且确为本入口创建的旧备用。失败不会先丢旧备用。`Component.onDestruction` 确认释放之前拒绝下一次创建，仍可明确切回。所有备用仅在应用内存中，不是磁盘备份或跨重启恢复。

取消来源预览只清除本地来源，不回滚已发布草稿，也不取消协议。新候选装入不等于已发布：`candidateInitialized`、`committed` 和 `current` 分开报告。候选装入但切换失败会释放未发布候选，仍保留旧当前与备用。发布后的同步通知/后置失败不能回报为“未发生修改”。发布结果过程中保持私有操作 ticket；普通 facade 编辑和新建/切回均检查 ticket，直到 busy 通知返回后才解开，防止同步重入覆盖外层提交事实。

## 有界审计与交接

追加 `mission.repair_draft_created` / `mission.repair_draft_restored`，记录原部署/代次、原绑定、对账请求/运行/代次、本地选择与修订、保留/替换事实，不记录完整规划源。这两项的审计追加未确认时，创建/切回结果提示该事实。新草稿成功审阅归档后追加 `mission.repair_draft_reviewed`，关联原 recordId 与新 recordId/recordRevision；此审计追加没有单独的界面回执，不宣称其失败会另行显示。重新审阅仍捕获当前实际 fleet/evidence；原规划来源不充当实时证据。任何审计追加失败都不撤销或否认已经提交的草稿/归档。

本批仅源码静态读回与定向 diff 空白检查。未编译、未运行 QML/业务/解析器/技能 helper、未测试、未连接设备或刷机。工程师需验证正常创建/切回、旧来源缺失、备用替换、离线本地编辑、同步通知重入、销毁等待、地图基准重新确认、新版本审阅和审计失败等实际路径；本文不作运行成功或飞行放行声明。UI 静态采用 `ui-ux-pro-max` 的明确确认、可取消、文字状态、键盘焦点、长文换行和渐进摘要指导，复用 QGC 控件/调色板；未执行技能的视觉验证步骤。

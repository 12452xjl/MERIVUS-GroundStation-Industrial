# 历史版本恢复为独立本地草稿（428，静态交接）

## 485：历史完整source5保留性能政策

具体历史版本中记录的完整planningSource经过中央严格validator、PlanStore源／计划策略presence和值绑定、Root新版集中预检后才可创建独立副本。schema5 false／true完整保留，旧1..4仍absent，不能从plan摘要、compiler标记或历史批准补出政策。

本面板／恢复后端无需机械新增字段拷贝，实际沿原完整source；新副本revision1、重新datum确认、重新当前审阅，不能恢复旧审批／账号／机载Run。新[政策编辑器](FleetPerformancePolicyEditor.md)待处理输入会阻断恢复／换源，隐藏控件不清pending。

旧档无完整来源仍不可反推区域；新field合法也不是性能认证或飞行许可。未运行历史读取／恢复／UI、编译或测试，工程场景全未执行。下方为历史。

## 472：独立编辑保存来源与历史恢复保持隔离

新增的[编辑草稿保存／恢复入口](FleetMissionSavedDraftPanel.md)采用独立 `saved-draft` 来源，由[固定本地单槽 Store](FleetMissionDraftStore.md)保存完整、结构合法但可能尚未完成的原生编辑内容。它不是把未编译草稿塞入 PlanStore 历史记录，也不代替本面板的具体历史版本选择。`archived-plan` 仍经原生严格 `validatePlanningSource` 和 `loadPlanningSource`，要求原完整可构建规划源；只有新 `saved-draft` 使用独立编辑校验／`loadEditingSource`，不能凭可恢复编辑内容取得历史规划资格。

Root 的共用来源版本预检补齐 `clearanceInput` 内层 schema 2（含 `contacts`）与原 schema 1 的对应字段；外层仍为原五键来源 schema 1／2。该预检只核版本／形状，完整保留值及其合法性仍交原生验证，不证明工程声明真实、当前或净空已通过。

历史、部署修复和新保存草稿三种来源，共用同一个当前草稿、一份**会话内备用**、候选工厂、操作 ticket 和明确替换备用确认；不是每个入口各有一份备份。创建新副本前仍在原 scope 撤销旧审批／预览并重新取源，之后才发布新当前、保留旧当前；恢复或切回均须重新确认坐标基准、重新构建／审阅／审批。磁盘编辑槽独立于这份内存备用，不恢复历史批准、账号、机载状态、部署、控制权或飞行权限。新入口的显式保存、完整字节重读及失败事实合同以其面板与 Store 文档为准。

本增量仅源码及直接静态文档核对，未编译、执行 helper／解析器／业务代码／UI 或测试，未进行设备操作。它不是历史恢复、修复入口或草稿产品的全功能验收；下方历史正文保留，其“两来源”叙述以本节的三来源共用机制补充。

## 448：有净空工程声明的来源版本2

历史接口外层schema1与七字段binding不变；内层planningSource保持五键，按[Draft合同](FleetMissionDraft.md)接受来源版本1（旧11项settings、无clearanceInput）或版本2（12项settings、明确含完整clearanceInput）。Root的历史来源预检同步核版本／字段存在性，随后仍由原生validatePlanningSource校整份资料，不能只改版本号绕过校验。

候选加载、重新确认坐标基准、明确替换／上一份草稿备用及原来源首尾检查保持；资料完整复制，不恢复批准、机载状态或旧任务执行。其它settings编辑复制完整map，不静默丢弃资料。资料值被读取／恢复不等于场地有效、物理构型已核或净空通过，后续编译仍保明确几何审阅pending阻断。本批无新界面布局，按UI/UX静态原则保持原输入、错误与恢复确认；未编译、解析、运行UI或测试，下方428为历史。

## 用户路径

`FleetMissionPlannerPanel.qml` 的编辑滚动区顶部新增 `FleetMissionArchiveDraftPanel`，默认折叠。展开不会查询版本、检查来源或恢复草稿。用户先选择一个已归档来源，再明确列出其全部保留版本（原生单来源最多 128 版，界面每页最多 32 条），然后选择具体正数修订版本；不是只显示最新版本，也不以 revision 0 代替用户选择。

每行展示版本、记录 ID、历史记录时间、是否从档案载入以及规划源字段是否存在。没有 `planningSource` 的旧版仍列出并标明不可恢复；字段存在不等于通过校验。检查所选版本时，原生 getter 再核对具体绑定、记录完整性与规划源。不能从编译航线反推原区域、补造源或把旧 fleet / 审阅证据带回当前任务。

此入口读的是**本实例已经加载的本地档案历史快照**，不是实时重读磁盘。跨重启由现有 PlanStore 构造加载固定档案；本批不添加文件选择、磁盘重载、自动刷新、定时查询或新 IO。后续外部文件变化不会仅靠内存 `storeEpoch` 被自动发现。`writeBlocked` 不阻止历史内容的本地编辑，但后续审阅可能无法新建归档。

## 来源绑定与公开接口

原有 `missionPlanArchive` 只读 facade 增加 `storeEpoch` 和显式 `historyVersions(sourceTaskId, expectedStoreEpoch)`。目录 exact10、每行 exact10；来源/记录标识采用不含控制字符且不超过 128 字符的 identifier，不限定 UUID。界面集合使用无原型字典，合法 `constructor` 等标识不会被当成重复。

选中值冻结原生绑定七字段：`storeEpoch/sourceTaskId/recordId/recordRevision/planId/contentHash/reviewEvidenceHash`。`inspectFleetMissionArchiveDraft(binding, expectedDraftToken)` 调用原生 `historicalDraftSource(binding)`；完整源只留 Root 私有 preview。`fleetMissionArchiveDraft` 对外仅投影有界摘要，绑定及结果复制后公开，不共享私有选择对象引用。更新追加较新版本不使旧版选择失效；更换/import Store 导致 epoch 改变则清除旧选择和确认。目录仅表示该来源当前保留范围，不证明全部历史完整性或来源认证。

`createFleetMissionArchiveDraft(sourceToken, replaceBackupConfirmed, confirmed)` 使用 `archived-plan` 来源种类；`cancelFleetMissionArchiveDraftPreview(sourceToken)` 只忘记本地来源预览。新面板沿用 `restorePreviousFleetMissionDraft` 切回共享备用。历史源不要求存在失败部署、整组对账、在线飞机或账号；初始实际 Episode 即可完成本地桥接。它不使用 `matchesBinding/liveReview` 或原部署成功证据。实际上传、读取、排空或启动链忙时仍互锁，不能借恢复清理未知占用。

## 与 427 共用提交和备用

427 的部署修复和 428 的历史恢复共用 `_createFleetMissionDraftCopy`、同一 ticket、工厂、当前/备用对象池、销毁确认、提交/当前性回执及审批失效逻辑。两者仅取源/重核源条件不同：427 保留原失败/取消 Deployment + 终态且排空 reconciliation 门禁，428 使用具体不可变归档版本。并未新增一套平行的草稿对象管理。

共享次序保持：桥接中捕获原源 → 退出 lease → 新建未接当前 Connections 的候选并原生加载 → 同一 scope 撤销旧预览/批准并重核源 → 退出 lease → 发布新当前、保留旧当前为备用 → 成功后才销毁已明确同意替换的更早备用。失败不先丢原备用。稳态最多两个动态对象加初始对象；临时候选/待销毁最多多一个，等待实际销毁时拒绝下一次创建，仍可切回。

加载后的本地修订为 1，地图基准确认强制清除。切回也只清此确认、保留其余内容，并要求重新审阅/审批。共享备用只在应用内存中，不保证跨重启保留未归档编辑。界面默认不勾确认；隐藏、折叠、换控制器/epoch/Episode、换草稿或来源后失效旧确认和按下意图。Planner 未提交输入时不允许本入口恢复；正常切换后的成员/端点、分页及已结束执行配置回执刷新通过 `Qt.callLater` 延后到本轮同步发布之后，重新核对控制器、Draft facade 和选择代次，并要求 busy 已解除且输入已提交或放弃。若暂时忙或有未提交输入，仅在 busy 解除或 `inputSettled` 变为 true 时再安排刷新，不轮询；换源后的旧请求和销毁后的回调无效，未提交文本不被静默丢弃。

## 身份、审计和许可边界

历史 `sourceTaskId` 仅作为追溯信息，不装入 Draft 或 buildTask。现有 `prepareFleetIntent` 仍使用当前 Root 生命周期中 `FleetIntentTask` 自己的 UUID；跨重启通常为新 UUID，本批不承诺同进程每次恢复都会生成新任务 ID。恢复不是审批、部署、任务续飞或机载状态恢复；仍需当前实际 fleet/evidence、地图基准确认、全新审阅和审批，不自动 prepare、上传、启动或解锁。

追加有界 `mission.archive_draft_created`、恢复到历史来源备用时的 `mission.archive_draft_restored`，以及成功重新归档后的 `mission.archive_draft_reviewed`，关联原七字段绑定、本地选择/修订及新归档 ID/修订。恢复到其他来源备用仍使用原 `mission.repair_draft_restored`。创建/切回结果说明审计是否确认；reviewed 无独立界面回执。追加失败不能撤销或否认已经提交的本地草稿/归档，不记录完整规划源。

## 本批检查与工程师验证

本批只做源码静态读回、跨层合同核对和定向 `git diff --check`；未编译、测试、运行 QML/技能脚本/解析器、连接设备或刷机。工程师需验证：旧版而非最新版本选择；合法非 UUID 标识；旧无源记录；重复追加版本不失效旧选择；更换 epoch 失效；空初始 Deployment 的恢复；离线/写入受阻编辑；创建/切回/替换备用/销毁等待；隐藏与换源废弃确认；当前任务 ID 与新证据审阅；审计失败及同步通知销毁。不得据本文宣称运行或飞行放行成功。

静态采用 `ui-ux-pro-max` 的明确选择、可取消、文字状态、键盘焦点、长文换行及有界渐进列表指导，复用 QGC 控件和调色板；未执行技能视觉验证步骤。

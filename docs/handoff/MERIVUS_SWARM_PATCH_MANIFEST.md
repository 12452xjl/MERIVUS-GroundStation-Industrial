# MERIVUS 集群地面站补丁清单

## 2026-10-19：PR #6 Qt 5.15 兼容性修复（23路径）

18源码＋5说明文件。基线 `7c01743fe0b94e65e3ac5a535a076f9ebfece381`；只修用户构建反馈中已确定的问题，不改业务条件、接口或飞行权限，不扩大交付范围。

| 源码路径 | 本次改动 |
|---|---|
| `custom/src/Swarm/FleetUserTaskTemplates.h` | 删除 class 的 `final`，允许 Qt 5 的可创建类型包装类继承 |
| `custom/src/Swarm/FleetMissionReportWorkspace.h` | 同上；原构造与 `qmlRegisterType` 不变 |
| `custom/res/Merivus/GuidedActionsController.qml` | 局部 `synchronized` 改 `identitySynchronized`，`native` 改 `nativeState`，同步全部局部引用 |
| `custom/res/Merivus/FleetArchiveHistoryPanel.qml` | 删除12处块外多余分号 |
| `custom/res/Merivus/FleetArchiveMaintenancePanel.qml` | 删除16处块外多余分号 |
| `custom/res/Merivus/FleetArchiveTaskHistoryPanel.qml` | 删除5处块外多余分号 |
| `custom/res/Merivus/FleetDeploymentArchivePanel.qml` | 删除14处块外多余分号，保留合法对象字面量赋值分号 |
| `custom/res/Merivus/FleetGnssMemoryWorkspacePanel.qml` | 删除1处滚动条对象绑定后的多余分号 |
| `custom/res/Merivus/FleetHandoffPanel.qml` | 删除2处块外多余分号 |
| `custom/res/Merivus/FleetMaintenanceHistoryPanel.qml` | 删除4处块外多余分号 |
| `custom/res/Merivus/FleetMissionGroupRemainderPanel.qml` | 删除1处块外多余分号 |
| `custom/res/Merivus/FleetMissionRemainderPanel.qml` | 删除2处块外多余分号 |
| `custom/res/Merivus/FleetMissionSpatialPanel.qml` | 删除7处块外多余分号 |
| `custom/res/Merivus/FleetMissionStartupSpatialPanel.qml` | 删除5处块外多余分号 |
| `custom/res/Merivus/FleetRetiredArchivePurgePanel.qml` | 删除6处块外多余分号 |
| `custom/res/Merivus/FleetTelemetryHistoryPanel.qml` | 删除9处块外多余分号 |
| `custom/res/Merivus/FleetReserveDispatchPanel.qml` | 删除1处块外多余分号；3个按钮快照属性及局部读写改为 `pressedIntent` |
| `custom/res/Merivus/FleetReserveDispatchProfileEditor.qml` | 删除3处块外多余分号；2个按钮快照属性及局部读写改为 `pressedIntent` |

说明文件为 `MODIFIED_FILES.md`、`docs/handoff/README.md`、本清单、`MERIVUS_SWARM_STATIC_AUDIT.md`、`MERIVUS_SWARM_ENGINEER_HANDOFF.md`。本次源码差异为105行删除及105行新增，行数包括原长行的最小替换，不代表新增算法或功能。

实际检查及未完成验证见[本次审计](MERIVUS_SWARM_STATIC_AUDIT.md)。以下546及更早内容保留为历史，不能视为本次版本的构建结果。

## 546：最终本地源码收口清单

本轮去重52路径：26源码/工程登记＋26文档。是545之后五条操作链及最终范围说明的增量，不是此前全部开发总数，不用于推算进度。完整交接以最终源码树为准，不仅本表列出的文件；外部条件见[最终范围](MERIVUS_FLEET_DELIVERY_SCOPE.md)。

| 类别 | 路径 | 本轮内容 |
|---|---|---|
| 源码/登记 | [custom/src/Swarm/FleetEventRuleEngine.h](../../custom/src/Swarm/FleetEventRuleEngine.h) | 实时待审箱API、状态与独立冷却存储 |
| 源码/登记 | [custom/src/Swarm/FleetEventRuleEngine.cc](../../custom/src/Swarm/FleetEventRuleEngine.cc) | 原生事件绑定、原子有界入箱、精确处理及数值/类型边界 |
| 源码/登记 | [custom/src/Swarm/SwarmMissionOrchestrator.h](../../custom/src/Swarm/SwarmMissionOrchestrator.h) | Run检查点后台状态、历史分页和窄操作接口 |
| 源码/登记 | [custom/src/Swarm/SwarmMissionOrchestrator.cc](../../custom/src/Swarm/SwarmMissionOrchestrator.cc) | 原生Run合并捕获、固定文件后台保存/历史重开与宽时间戳校验 |
| 源码/登记 | [custom/src/Swarm/FleetUserTaskTemplates.h](../../custom/src/Swarm/FleetUserTaskTemplates.h) | 独立用户库、不可变版本和文件/预览API |
| 源码/登记 | [custom/src/Swarm/FleetUserTaskTemplates.cc](../../custom/src/Swarm/FleetUserTaskTemplates.cc) | 实际Draft完整捕获、版本追加、后台整库读写与原来源核对 |
| 源码/登记 | [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h) | 显式留存计划/状态与原维护槽编排声明 |
| 源码/登记 | [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc) | 原清单保留计划、逐项退休/备份/复核及部分事实 |
| 源码/登记 | [custom/res/Merivus/FleetRuleReviewPanel.qml](../../custom/res/Merivus/FleetRuleReviewPanel.qml) | 规则建议四条分页、详情、明确审阅/拒绝及只导航入口 |
| 源码/登记 | [custom/res/Merivus/FleetRunCheckpointPanel.qml](../../custom/res/Merivus/FleetRunCheckpointPanel.qml) | 检查点明确启用、保存、独立历史和选中报告 |
| 源码/登记 | [custom/res/Merivus/FleetUserTaskTemplatesPanel.qml](../../custom/res/Merivus/FleetUserTaskTemplatesPanel.qml) | 用户库编辑/版本/文件与精确预览复制确认 |
| 源码/登记 | [custom/res/Merivus/FleetArchiveMaintenancePanel.qml](../../custom/res/Merivus/FleetArchiveMaintenancePanel.qml) | 现维护区留存计划、逐项结果与取消提示 |
| 源码/登记 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 五条真实来源窄门面、原生接线及新草案审批退休链 |
| 源码/登记 | [custom/res/Merivus/CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml) | 待审箱和完整审阅报告的实际指挥中心入口 |
| 源码/登记 | [custom/res/Merivus/FleetMissionExecutionReportPanel.qml](../../custom/res/Merivus/FleetMissionExecutionReportPanel.qml) | 检查点独立历史导入原保护报告只读显示 |
| 源码/登记 | [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 现规划器接入用户模板，并沿原输入提交与滚动 |
| 源码/登记 | [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | 登记新增QML面板 |
| 源码/登记 | [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 登记新增面板真实资源路径 |
| 源码/登记 | [custom/src/CustomPlugin.cc](../../custom/src/CustomPlugin.cc) | 登记用户库和审阅报告工作区原生QML类型 |
| 源码/登记 | [custom/custom.pri](../../custom/custom.pri) | 登记新增原生源文件和头文件 |
| 源码/登记 | [custom/src/Swarm/FleetMissionReportWorkspace.h](../../custom/src/Swarm/FleetMissionReportWorkspace.h) | 原生审阅报告工作区和受众详情/文件窄接口 |
| 源码/登记 | [custom/src/Swarm/FleetMissionReportWorkspace.cc](../../custom/src/Swarm/FleetMissionReportWorkspace.cc) | 实际来源冻结、后台完整验证/文件与独立只读槽 |
| 源码/登记 | [custom/res/Merivus/FleetMissionReviewReportPanel.qml](../../custom/res/Merivus/FleetMissionReviewReportPanel.qml) | 四受众审阅资料查看、完整文件警示及明确文件操作 |
| 源码/登记 | [custom/src/Swarm/FleetMissionReportBuilder.h](../../custom/src/Swarm/FleetMissionReportBuilder.h) | 复用旧格式的原生纯值校验/编码/解码声明 |
| 源码/登记 | [custom/src/Swarm/FleetMissionReportBuilder.cc](../../custom/src/Swarm/FleetMissionReportBuilder.cc) | 共享旧摘要和报告接受域，不另造事故格式 |
| 源码/登记 | [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc) | 避免给已哈希报告追加根字段导致原摘要失配 |
| 文档 | [custom/src/Swarm/FleetEventRuleEngine.md](../../custom/src/Swarm/FleetEventRuleEngine.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [docs/fleet-event-rules.md](../../docs/fleet-event-rules.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/SwarmMissionOrchestrator.md](../../custom/src/Swarm/SwarmMissionOrchestrator.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetRuleReviewPanel.md](../../custom/src/Swarm/FleetRuleReviewPanel.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetRunCheckpointPanel.md](../../custom/src/Swarm/FleetRunCheckpointPanel.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetUserTaskTemplates.md](../../custom/src/Swarm/FleetUserTaskTemplates.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetUserTaskTemplatesPanel.md](../../custom/src/Swarm/FleetUserTaskTemplatesPanel.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetArchiveMaintenancePanel.md](../../custom/src/Swarm/FleetArchiveMaintenancePanel.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetMissionReportWorkspace.md](../../custom/src/Swarm/FleetMissionReportWorkspace.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetMissionReviewReportPanel.md](../../custom/src/Swarm/FleetMissionReviewReportPanel.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetMissionReportBuilder.md](../../custom/src/Swarm/FleetMissionReportBuilder.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md) | 本轮实现合同、资源/来源/取消边界及未执行验证说明 |
| 文档 | [README.md](../../README.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 同步当前范围、修改、审计与工程交接；保留历史 |
| 文档 | [docs/handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md](../../docs/handoff/MERIVUS_FLEET_DELIVERY_SCOPE.md) | 原25目标本地证据、用户确认范围与外部未完成项 |

仅普通文本/路径/空白核对，不是编译测试结果。未修改PX4，未提交、推送、合并或更新PR；最终封装不含新编译安装程序。


## 545：完整证据链保存与独立历史（21路径）

7源码/资源登记＋14文档；新增面板qml与说明md两路径，其余为现有文件。没有阶段包、编译、测试、提交或PR更新。

| 类别 | 路径 | 用途 |
| --- | --- | --- |
| 源码/登记 | [custom/src/Swarm/FleetMissionEvidenceChain.h](../../custom/src/Swarm/FleetMissionEvidenceChain.h) | 后台操作、历史摘要及有界查询接口 |
| 源码/登记 | [custom/src/Swarm/FleetMissionEvidenceChain.cc](../../custom/src/Swarm/FleetMissionEvidenceChain.cc) | 冻结COW、单worker、整份预算及独立历史 |
| 源码/登记 | [custom/res/Merivus/FleetEvidenceArchivePanel.qml](../../custom/res/Merivus/FleetEvidenceArchivePanel.qml) | 明确保存/打开、状态、历史分页详情 |
| 源码/登记 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 不暴露活链替换的窄门面 |
| 源码/登记 | [custom/res/Merivus/CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml) | 原指挥中心实际入口 |
| 源码/登记 | [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | 登记QML类型和资源 |
| 源码/登记 | [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 登记QML类型和资源 |
| 文档 | [custom/src/Swarm/FleetMissionEvidenceChain.md](../../custom/src/Swarm/FleetMissionEvidenceChain.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [custom/src/Swarm/FleetEvidenceArchivePanel.md](../../custom/src/Swarm/FleetEvidenceArchivePanel.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [README.md](../../README.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 当前合同、清单、审计或未执行交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 当前合同、清单、审计或未执行交接 |

## 544：公共航线实际成员展开（16路径）

2源码＋14文档，无新文件；未编译、测试、打包、提交或更新PR。只修复现有普通路线编排链，不扩大飞行权限。

| 类别 | 路径 | 用途 |
| --- | --- | --- |
| 源码 | [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 公共路线逐机展开、原始要求及时间参考保真 |
| 源码 | [custom/src/Swarm/FleetMissionPlanStore.cc](../../custom/src/Swarm/FleetMissionPlanStore.cc) | 原任务根25项已有时间/参考声明随完整计划保存 |
| 文档 | [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [README.md](../../README.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 当前合同、修改清单、审计或未执行工程交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 当前合同、修改清单、审计或未执行工程交接 |

## 543：配置速度明确采纳与逐机路线预算（31路径）

10源码＋21文档，没有本批新文件。只修改地面站；无阶段包、提交或PR更新。源码静读不代表构建或真机验证。派生估值不进入飞行指令，也不补缺失任务时长或授予启动权限。

| 类别 | 路径 | 用途 |
| --- | --- | --- |
| 源码 | [custom/src/Swarm/FleetMissionPreviewTimeline.h](../../custom/src/Swarm/FleetMissionPreviewTimeline.h) | 共享名义路线计算接口 |
| 源码 | [custom/src/Swarm/FleetMissionPreviewTimeline.cc](../../custom/src/Swarm/FleetMissionPreviewTimeline.cc) | 复用几何内核的出返travel-only估值 |
| 源码 | [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 最终实际路线派生元数据 |
| 源码 | [custom/src/Swarm/FleetMissionGroupRemainderPlanner.cc](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.cc) | 组余程不继承旧派生估值 |
| 源码 | [custom/src/Swarm/FleetReserveDispatchPlanner.cc](../../custom/src/Swarm/FleetReserveDispatchPlanner.cc) | 备用新任务不继承旧派生估值 |
| 源码 | [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc) | 原生速度建议与逐机预算消费 |
| 源码 | [custom/src/Swarm/FleetMissionPayloadBuilder.cc](../../custom/src/Swarm/FleetMissionPayloadBuilder.cc) | 完整派生元数据核对 |
| 源码 | [custom/res/Merivus/FleetExecutionProfileEditor.qml](../../custom/res/Merivus/FleetExecutionProfileEditor.qml) | 明确查看、采纳与来源尾检 |
| 源码 | [custom/res/Merivus/FleetReserveDispatchProfileEditor.qml](../../custom/res/Merivus/FleetReserveDispatchProfileEditor.qml) | 旧19/新20观察兼容 |
| 源码 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 原备用消费者观察闭形兼容 |
| 文档 | [custom/src/Swarm/FleetMissionPreviewTimeline.md](../../custom/src/Swarm/FleetMissionPreviewTimeline.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetMissionGroupRemainderPlanner.md](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetReserveDispatchPlanner.md](../../custom/src/Swarm/FleetReserveDispatchPlanner.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetMissionPayloadBuilder.md](../../custom/src/Swarm/FleetMissionPayloadBuilder.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetExecutionProfileEditor.md](../../custom/src/Swarm/FleetExecutionProfileEditor.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetReserveDispatchProfileEditor.md](../../custom/src/Swarm/FleetReserveDispatchProfileEditor.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [custom/src/Swarm/FleetEnergyScheduler.md](../../custom/src/Swarm/FleetEnergyScheduler.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [README.md](../../README.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 合同、清单、审计或未执行工程交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 合同、清单、审计或未执行工程交接 |


## 542：编队等待与重新确认（28路径）

12源码/工程登记＋16文档，新增6路径（队列h/cc/md、界面qml/md、确认工作流md）。只增加明确人工操作的本地等待流程，复用原编队协议和派发入口；不修改PX4、放宽StartPermit、恢复未知占用或替代工程验证。没有阶段包、提交或PR更新。

| 类别 | 路径 | 用途 | 新增 |
| --- | --- | --- | --- |
| 源码/登记 | [custom/src/Swarm/FleetFormationQueue.h](../../custom/src/Swarm/FleetFormationQueue.h) | 有界本机会话队列接口 | 是 |
| 源码/登记 | [custom/src/Swarm/FleetFormationQueue.cc](../../custom/src/Swarm/FleetFormationQueue.cc) | FIFO/原生占用/期限/一次复核消费 | 是 |
| 源码/登记 | [custom/custom.pri](../../custom/custom.pri) | 登记新队列h/cc | 否 |
| 源码/登记 | [custom/src/Swarm/SwarmController.h](../../custom/src/Swarm/SwarmController.h) | 只读原生队列属性与持有 | 否 |
| 源码/登记 | [custom/src/Swarm/SwarmController.cc](../../custom/src/Swarm/SwarmController.cc) | 原Swarm创建及一次Orchestrator绑定 | 否 |
| 源码/登记 | [custom/src/Swarm/SwarmMissionOrchestrator.h](../../custom/src/Swarm/SwarmMissionOrchestrator.h) | C++轻量占用投影接口 | 否 |
| 源码/登记 | [custom/src/Swarm/SwarmMissionOrchestrator.cc](../../custom/src/Swarm/SwarmMissionOrchestrator.cc) | 直接TaskRecord最多255机阻塞投影 | 否 |
| 源码/登记 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 显式入队、原确认令牌绑定和原派发消费者 | 否 |
| 源码/登记 | [custom/res/Merivus/FleetFormationQueuePanel.qml](../../custom/res/Merivus/FleetFormationQueuePanel.qml) | 本机待办四条分页及操作 | 是 |
| 源码/登记 | [custom/res/Merivus/CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml) | 现指挥中心选机后接入 | 否 |
| 源码/登记 | [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | 登记新QML类型 | 否 |
| 源码/登记 | [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 登记新QML资源 | 否 |
| 文档 | [custom/src/Swarm/FleetFormationQueue.md](../../custom/src/Swarm/FleetFormationQueue.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 是 |
| 文档 | [custom/src/Swarm/FleetFormationQueueWorkflow.md](../../custom/src/Swarm/FleetFormationQueueWorkflow.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 是 |
| 文档 | [custom/src/Swarm/FleetFormationQueuePanel.md](../../custom/src/Swarm/FleetFormationQueuePanel.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 是 |
| 文档 | [custom/src/Swarm/SwarmMissionOrchestrator.md](../../custom/src/Swarm/SwarmMissionOrchestrator.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [README.md](../../README.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 当前实现合同、修改清单、静态审计或未执行工程交接 | 否 |


## 541：任务模式退出证据贯通（41路径）

18源码/工程登记＋23文档，新增3路径（共享校验器 h/cc/md）；其余是当前增量接线，不按工作区untracked状态计新建。保原生数据来源、有限队列和旧记录兼容，未改PX4、SQL表、飞行命令或控制许可。无阶段包、提交或PR更新；未编译、运行、测试或刷机。

| 类别 | 路径 | 本次用途 | 新增 |
| --- | --- | --- | --- |
| 源码/登记 | [custom/src/Swarm/FleetMissionExecutionObservation.h](../../custom/src/Swarm/FleetMissionExecutionObservation.h) | 原生首末退出与最近任务模式引用状态 | 否 |
| 源码/登记 | [custom/src/Swarm/FleetMissionExecutionObservation.cc](../../custom/src/Swarm/FleetMissionExecutionObservation.cc) | 实际post-commit边沿捕获与schema2投影 | 否 |
| 源码/登记 | [custom/src/Swarm/FleetMissionModeExitObservation.h](../../custom/src/Swarm/FleetMissionModeExitObservation.h) | 共享闭合同接口 | 是 |
| 源码/登记 | [custom/src/Swarm/FleetMissionModeExitObservation.cc](../../custom/src/Swarm/FleetMissionModeExitObservation.cc) | 完整UInt32、计数与接收顺序校验 | 是 |
| 源码/登记 | [custom/custom.pri](../../custom/custom.pri) | 登记新校验器h/cc | 否 |
| 源码/登记 | [custom/src/Swarm/FleetMissionExecutionReport.h](../../custom/src/Swarm/FleetMissionExecutionReport.h) | 报告v1/v2兼容说明 | 否 |
| 源码/登记 | [custom/src/Swarm/FleetMissionExecutionReport.cc](../../custom/src/Swarm/FleetMissionExecutionReport.cc) | 完整/保护报告与brief版本化、空值及有限预算 | 否 |
| 源码/登记 | [custom/src/Swarm/SwarmController.cc](../../custom/src/Swarm/SwarmController.cc) | 实际保护报告原记录版本门 | 否 |
| 源码/登记 | [custom/res/Merivus/FleetMissionExecutionReportPanel.qml](../../custom/res/Merivus/FleetMissionExecutionReportPanel.qml) | 完整及保护报告首末只读展示 | 否 |
| 源码/登记 | [custom/res/Merivus/FleetHandoffPanel.qml](../../custom/res/Merivus/FleetHandoffPanel.qml) | 交接简报版本门与首末只读展示 | 否 |
| 源码/登记 | [custom/src/Swarm/FleetEventArchiveCodec.h](../../custom/src/Swarm/FleetEventArchiveCodec.h) | 归档v3/v4接口说明 | 否 |
| 源码/登记 | [custom/src/Swarm/FleetEventArchiveCodec.cc](../../custom/src/Swarm/FleetEventArchiveCodec.cc) | 新投影版本、严格解码与零次空值兼容 | 否 |
| 源码/登记 | [custom/src/Swarm/FleetMissionReportBuilder.cc](../../custom/src/Swarm/FleetMissionReportBuilder.cc) | 实际事件报告保留退出细节 | 否 |
| 源码/登记 | [custom/res/Merivus/FleetArchiveHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveHistoryPanel.qml) | 归档页v4和首末显示、窄空值复制 | 否 |
| 源码/登记 | [custom/res/Merivus/FleetArchiveTaskHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveTaskHistoryPanel.qml) | 任务下钻v4和首末显示、窄空值复制 | 否 |
| 源码/登记 | [custom/src/Swarm/SwarmMissionOrchestrator.cc](../../custom/src/Swarm/SwarmMissionOrchestrator.cc) | 语义比较包含首末/计数、normal2与record2 | 否 |
| 源码/登记 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 实际事件版本与值形状镜像、仍核原生回执 | 否 |
| 源码/登记 | [custom/src/Swarm/MissionHandoffManager.cc](../../custom/src/Swarm/MissionHandoffManager.cc) | 精确v2 Run简报父metadata预算 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionExecutionObservation.md](../../custom/src/Swarm/FleetMissionExecutionObservation.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionModeExitObservation.md](../../custom/src/Swarm/FleetMissionModeExitObservation.md) | 当前模块合同与版本边界 | 是 |
| 文档 | [custom/src/Swarm/FleetMissionExecutionReport.md](../../custom/src/Swarm/FleetMissionExecutionReport.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionExecutionReportPanel.md](../../custom/src/Swarm/FleetMissionExecutionReportPanel.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/FleetHandoffPanel.md](../../custom/src/Swarm/FleetHandoffPanel.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/FleetEventArchiveCodec.md](../../custom/src/Swarm/FleetEventArchiveCodec.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionReportBuilder.md](../../custom/src/Swarm/FleetMissionReportBuilder.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/FleetRunObservationEvent.md](../../custom/src/Swarm/FleetRunObservationEvent.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/SwarmMissionOrchestrator.md](../../custom/src/Swarm/SwarmMissionOrchestrator.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/MissionHandoffManager.md](../../custom/src/Swarm/MissionHandoffManager.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [custom/src/Swarm/FleetProtectedRunReport.md](../../custom/src/Swarm/FleetProtectedRunReport.md) | 当前模块合同与版本边界 | 否 |
| 文档 | [README.md](../../README.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 当前进展、清单、审计或未执行工程交接 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 当前进展、清单、审计或未执行工程交接 | 否 |

## 540：完整计划驱动的逐机时间预演（22路径）

7源码／工程登记＋15文档，新增4路径（Timeline h/cc/md与面板合同）。不重写原任务架构或扩大飞控权限，不创建阶段包；未编译、运行、测试、仿真、改PX4、提交或更新PR。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionPreviewTimeline.h](../../custom/src/Swarm/FleetMissionPreviewTimeline.h) | 有界纯值时间线结果与输入接口 | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionPreviewTimeline.cc](../../custom/src/Swarm/FleetMissionPreviewTimeline.cc) | 逐机真实路线、返程连接、驻留和未知时间传播 | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionDigitalTwin.h](../../custom/src/Swarm/FleetMissionDigitalTwin.h) | 原生私有事件缓存与按原预演分页 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionDigitalTwin.cc](../../custom/src/Swarm/FleetMissionDigitalTwin.cc) | 完整计划接入、摘要与原生页身份 | 否 |
| 源码／工程登记 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 同一实际审阅记录捕获与来源版本绑定 | 否 |
| 源码／工程登记 | [custom/res/Merivus/FleetDigitalTwinPanel.qml](../../custom/res/Merivus/FleetDigitalTwinPanel.qml) | 实际捕获、历史说明、四条分页和未知时间显示 | 否 |
| 源码／工程登记 | [custom/custom.pri](../../custom/custom.pri) | 登记新纯值模块 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionPreviewTimeline.md](../../custom/src/Swarm/FleetMissionPreviewTimeline.md) | 计算输入、相对时间与模型边界 | 是 |
| 文档 | [custom/src/Swarm/FleetMissionDigitalTwin.md](../../custom/src/Swarm/FleetMissionDigitalTwin.md) | 原Twin新计算及分页接口 | 否 |
| 文档 | [custom/src/Swarm/FleetDigitalTwinPanel.md](../../custom/src/Swarm/FleetDigitalTwinPanel.md) | 实际捕获到只读显示工作流 | 是 |
| 文档 | [README.md](../../README.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 同步当前增量、审计和统一交付边界 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 同步当前增量、审计和统一交付边界 | 否 |

## 539：交接历史保存与恢复（19路径）

4源码＋15文档，全部为现有路径，无新增文件。保留原schema1与既有工程登记，后台worker在原Manager.cc中；复用已登记的FleetSnapshotFileIo，不新增库或依赖。此为源码增量，不是全项目交付或安装程序。未编译、运行、测试、仿真、改PX4、提交、更新PR或打包。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码 | [custom/src/Swarm/MissionHandoffManager.h](../../custom/src/Swarm/MissionHandoffManager.h) | 后台历史工作区API、私有原生候选和集合代次 | 否 |
| 源码 | [custom/src/Swarm/MissionHandoffManager.cc](../../custom/src/Swarm/MissionHandoffManager.cc) | 冻结快照、后台保存/读取/合并、冲突整拒和历史隔离 | 否 |
| 源码 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 受限五字段请求、脱离对象的状态及有界候选读取 | 否 |
| 源码 | [custom/res/Merivus/FleetHandoffPanel.qml](../../custom/res/Merivus/FleetHandoffPanel.qml) | 实际文件工作区、预览分页、确认合并及取消恢复 | 否 |
| 文档 | [custom/src/Swarm/MissionHandoffManager.md](../../custom/src/Swarm/MissionHandoffManager.md) | 原生接口、线程、文件事实及兼容边界 | 否 |
| 文档 | [custom/src/Swarm/FleetHandoffPanel.md](../../custom/src/Swarm/FleetHandoffPanel.md) | 界面流程、确认、状态和只读边界 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionHandoffWorkflow.md](../../custom/src/Swarm/FleetMissionHandoffWorkflow.md) | 保存到历史合并的端到端流程 | 否 |
| 文档 | [README.md](../../README.md) | 当前功能入口与交付边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前变更入口 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 交接功能文档入口 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 目标15源码进展、不缩减总目标 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 本地资料交接与跨站控制边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 局部流程与完整目标分开 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 材料同步与未执行范围 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前源码状态与统一交付要求 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 工程师接手入口 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 限定静读和材料检查 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 待工程验证场景 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 19路径实际清单 | 否 |

## 538：持久化任务索引（28路径）

12源码／工程登记＋16文档，新增5路径（索引h/cc/md、界面qml/md）。已有untracked不按本批新建计。原任务目录协议与预算保留；派生索引复用原HistoryReader，未另开并行读盘体系。没有PX4修改、新飞行命令、外部依赖、提交、PR更新或打包。以下是增量清单，不是全项目完成比例。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [custom/src/Swarm/FleetArchiveTaskIndex.h](../../custom/src/Swarm/FleetArchiveTaskIndex.h) | 纯原生后台API与闭合请求/结果合同 | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetArchiveTaskIndex.cc](../../custom/src/Swarm/FleetArchiveTaskIndex.cc) | 有界checkpoint、分窗扫描、全源冻结、原子保存及分页 | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetEventArchiveHistoryReader.h](../../custom/src/Swarm/FleetEventArchiveHistoryReader.h) | 搜索复用与全源采集/复核API | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetEventArchiveHistoryReader.cc](../../custom/src/Swarm/FleetEventArchiveHistoryReader.cc) | 接原读取线程、完整身份核对及晚取消提交事实 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h) | 窄索引属性、请求及私有上下文 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc) | 原共享槽、准确query、页面/记录连续性与取消 | 否 |
| 源码／工程登记 | [custom/custom.pri](../../custom/custom.pri) | 新索引源/头登记 | 否 |
| 源码／工程登记 | [custom/res/Merivus/FleetArchiveTaskIndexPanel.qml](../../custom/res/Merivus/FleetArchiveTaskIndexPanel.qml) | 创建/重开/续扫/重建/分页/下钻实际界面 | 是 |
| 源码／工程登记 | [custom/res/Merivus/FleetArchiveHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveHistoryPanel.qml) | 默认收起的独立索引入口 | 否 |
| 源码／工程登记 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 窄门面接原Store，不传文件路径或原游标 | 否 |
| 源码／工程登记 | [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | QML类型登记 | 否 |
| 源码／工程登记 | [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | QML资源登记 | 否 |
| 文档 | [custom/src/Swarm/FleetArchiveTaskIndex.md](../../custom/src/Swarm/FleetArchiveTaskIndex.md) | 合同、范围、静态审阅或工程交接同步 | 是 |
| 文档 | [custom/src/Swarm/FleetEventArchiveHistoryReader.md](../../custom/src/Swarm/FleetEventArchiveHistoryReader.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [custom/src/Swarm/FleetArchiveTaskIndexPanel.md](../../custom/src/Swarm/FleetArchiveTaskIndexPanel.md) | 合同、范围、静态审阅或工程交接同步 | 是 |
| 文档 | [README.md](../../README.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 合同、范围、静态审阅或工程交接同步 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 合同、范围、静态审阅或工程交接同步 | 否 |


## 537：逐 NAV 航点相对驻留（33路径）

12源码／工程登记＋21文档=33路径，新增仅DwellEncoding.h／cc／md三条。已有untracked文件不是本批新建。本批沿实际点编辑和原param1命令链补功能，无新外部依赖、PX4改动、payload/schema或飞行命令类型；实际工程登记仅custom.pri，全部构建入口未验证。其余累计本地修改保留，未提交、更新PR或打包。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc) | 逐点／去返／估算加驻留保守预算及明确窗口；无profile驻留仍核当前声明 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupGeographicMotion.cc](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.cc) | canonical NAV p1共享非零编码门，原exact Float32不放宽 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupIntegerDomain.cc](../../custom/src/Swarm/FleetMissionStartupIntegerDomain.cc) | 整数域NAV p1接受域与实际编码一致 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupDomainCover.cc](../../custom/src/Swarm/FleetMissionStartupDomainCover.cc) | 完整覆盖NAV p1接受域与实际编码一致 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionJointSpatialReview.cc](../../custom/src/Swarm/FleetMissionJointSpatialReview.cc) | raw p1共享合法性且原source／canonical／delta绑定保持 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc) | 独立路线hold可选原值、完整存储恢复及统一编码校验 | 否 |
| 源码／工程登记 | [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 驻留独立勾选／秒数／回填／取消／清除／列表显示 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionDwellEncoding.h](../../custom/src/Swarm/FleetMissionDwellEncoding.h) | 四值数值模型与保守累加API | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionDwellEncoding.cc](../../custom/src/Swarm/FleetMissionDwellEncoding.cc) | Float32／微秒归零拒绝、条件上取与数值环境门 | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 保原hold请求，拒不满足实际编码模型的正值 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionPayloadBuilder.cc](../../custom/src/Swarm/FleetMissionPayloadBuilder.cc) | 点解析与NAV canonical p1共享数值规则，原raw／delta保持 | 否 |
| 源码／工程登记 | [custom/custom.pri](../../custom/custom.pri) | 实际qmake入口登记新Dwell helper源和头 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md) | 对应537真实消费者／数值／编辑契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupGeographicMotion.md](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.md) | 对应537真实消费者／数值／编辑契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupIntegerDomain.md](../../custom/src/Swarm/FleetMissionStartupIntegerDomain.md) | 对应537真实消费者／数值／编辑契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainCover.md](../../custom/src/Swarm/FleetMissionStartupDomainCover.md) | 对应537真实消费者／数值／编辑契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionJointSpatialReview.md](../../custom/src/Swarm/FleetMissionJointSpatialReview.md) | 对应537真实消费者／数值／编辑契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md) | 对应537真实消费者／数值／编辑契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionDwellEncoding.md](../../custom/src/Swarm/FleetMissionDwellEncoding.md) | 对应537真实消费者／数值／编辑契约 | 是 |
| 文档 | [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md) | 对应537真实消费者／数值／编辑契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionPayloadBuilder.md](../../custom/src/Swarm/FleetMissionPayloadBuilder.md) | 对应537真实消费者／数值／编辑契约 | 否 |
| 文档 | [README.md](../../README.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [docs/INDEX.md](../INDEX.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../MERIVUS_FLEET_OS_COLLABORATION.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../DOCUMENT_AUDIT.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../PROJECT_STATUS.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [docs/handoff/README.md](README.md) | 当前增量、交接入口及原目标未完成边界 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md) | 限定直接静读及普通材料核对 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 40项工程场景全部未执行 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md) | 33路径完整清单 | 否 |

本表仅统计本批材料，不是功能完成百分比。原请求／实际上传／声明微秒等待／预算分开；精确可表示的时间不无故扩成更大窗口。只有限定静读和普通材料核对，没有运行或构建证明。40项场景见[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)，六类剩余源码继续。下方536及更早为历史。



## 536：逐 NAV 航点机头方向（60路径）

28源码／工程登记＋32文档=60路径，新增仅HeadingEncoding.h／cc／md三条。原本已有但尚未纳入Git的文件不算本批新建；本表只列536，不覆盖累计其它修改。无新外部依赖、PX4改动或新的飞行命令类型；沿现有NAV_WAYPOINT.param4表达可选机头方向，实际构建入口仅登记custom.pri。未提交、更新PR或打包。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [custom/custom.pri](../../custom/custom.pri) | 登记共享HeadingEncoding源文件和头文件；其它构建入口未验证 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupGeographicMotion.cc](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.cc) | 严格canonical项接受明确NAV tick，不放宽其它字段 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupIntegerDomain.cc](../../custom/src/Swarm/FleetMissionStartupIntegerDomain.cc) | 整数域消费者共享严格上传朝向校验 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupDomainCover.cc](../../custom/src/Swarm/FleetMissionStartupDomainCover.cc) | 整域覆盖消费者共享严格朝向校验 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionJointSpatialReview.cc](../../custom/src/Swarm/FleetMissionJointSpatialReview.cc) | 原请求／source／诊断10与raw／canonical一致性 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc) | 接受明确v3语义域且仍核同一原载荷 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetFirmwareAdapterCatalog.h](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.h) | 目录语义域能力检查接口 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetFirmwareAdapterCatalog.cc](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.cc) | 明确v3语义声明、当前目录对应及旧v2子集 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc) | 实际逐成员编码域进入当前固件声明审阅门禁 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartReadiness.cc](../../custom/src/Swarm/FleetMissionStartReadiness.cc) | 首尾实际证据均核当前目录对应载荷语义域 | 否 |
| 源码／工程登记 | [custom/res/Merivus/FleetMissionDeploymentPanel.qml](../../custom/res/Merivus/FleetMissionDeploymentPanel.qml) | 新回读shape及原值／规范值／误差界／错误显示 | 否 |
| 源码／工程登记 | [custom/res/Merivus/FleetMissionSpatialPanel.qml](../../custom/res/Merivus/FleetMissionSpatialPanel.qml) | 原请求、上传tick和量化差独立展示 | 否 |
| 源码／工程登记 | [custom/res/Merivus/FleetMissionRemainderRoutePanel.qml](../../custom/res/Merivus/FleetMissionRemainderRoutePanel.qml) | 余段计划朝向量化诊断与absolute-degrees显示 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionHeadingEncoding.h](../../custom/src/Swarm/FleetMissionHeadingEncoding.h) | 逐NAV可选请求、tick网格、回读界和诊断API | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionHeadingEncoding.cc](../../custom/src/Swarm/FleetMissionHeadingEncoding.cc) | 连续请求校验、圆周最近tick、严格上传／回读语义与schema选择 | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionDraft.h](../../custom/src/Swarm/FleetMissionDraft.h) | 带当前revision的独立路线整点更新API | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc) | 独立路线点保原heading，完整恢复和整点替换 | 否 |
| 源码／工程登记 | [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 辅助出返程逐点朝向、新增／编辑／取消及原输入来源保护 | 否 |
| 源码／工程登记 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | append／update／remove携草稿来源与revision，通知前后复核 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 完整路线保连续heading／yaw请求，拒非法别名 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionPayloadBuilder.cc](../../custom/src/Swarm/FleetMissionPayloadBuilder.cc) | 逐NAV上传tick、诊断10、逐成员v2／v3和未编码达向要求 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionDeployment.cc](../../custom/src/Swarm/FleetMissionDeployment.cc) | 上传用原tick，普通回读保actualRaw并共享解码 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionReadbackDiff.cc](../../custom/src/Swarm/FleetMissionReadbackDiff.cc) | 新schema2／heading观察；匹配也保原值，异常有限值不丢 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionReconciliation.cc](../../custom/src/Swarm/FleetMissionReconciliation.cc) | RAW INT语义比较域与原始指纹分离 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionProgressObservation.cc](../../custom/src/Swarm/FleetMissionProgressObservation.cc) | 进度观察回读同一模式规则，不推导真实达向 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartContentCheck.cc](../../custom/src/Swarm/FleetMissionStartContentCheck.cc) | 新鲜整表RAW校验与v2／v3对应 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc) | 地面再审RAW回读保原值、共享严格解码 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionPayloadBuilder.h](../../custom/src/Swarm/FleetMissionPayloadBuilder.h) | 公开共享语义编码的接口契约说明 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupGeographicMotion.md](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupIntegerDomain.md](../../custom/src/Swarm/FleetMissionStartupIntegerDomain.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainCover.md](../../custom/src/Swarm/FleetMissionStartupDomainCover.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionJointSpatialReview.md](../../custom/src/Swarm/FleetMissionJointSpatialReview.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetFirmwareAdapterCatalog.md](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartReadiness.md](../../custom/src/Swarm/FleetMissionStartReadiness.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionSpatialPanel.md](../../custom/src/Swarm/FleetMissionSpatialPanel.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderRoutePanel.md](../../custom/src/Swarm/FleetMissionRemainderRoutePanel.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionHeadingEncoding.md](../../custom/src/Swarm/FleetMissionHeadingEncoding.md) | 网格／Float32模型推导及物理未认证边界 | 是 |
| 文档 | [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionPayloadBuilder.md](../../custom/src/Swarm/FleetMissionPayloadBuilder.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionDeployment.md](../../custom/src/Swarm/FleetMissionDeployment.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionReadbackDiff.md](../../custom/src/Swarm/FleetMissionReadbackDiff.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionReconciliation.md](../../custom/src/Swarm/FleetMissionReconciliation.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionProgressObservation.md](../../custom/src/Swarm/FleetMissionProgressObservation.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartContentCheck.md](../../custom/src/Swarm/FleetMissionStartContentCheck.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md) | 对应实际消费者的536输入、编码、回读和权限契约 | 否 |
| 文档 | [README.md](../../README.md) | 本批源码能力与非放行边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 60路径变更入口 | 否 |
| 文档 | [docs/INDEX.md](../INDEX.md) | 朝向合同、审计和交接索引 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md) | 536增量，保原25目标及六类未完成范围 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../MERIVUS_FLEET_OS_COLLABORATION.md) | 职责边界与工程接续 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md) | 源码／工程验证分开记录 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../DOCUMENT_AUDIT.md) | 材料范围及静态检查口径 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../PROJECT_STATUS.md) | 当前源码状态和未完成能力 | 否 |
| 文档 | [docs/handoff/README.md](README.md) | 工程交接统一入口 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md) | 限定直接静读、交叉审阅及材料核对记录 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 48项全部未执行的工程场景与实际固件证据要求 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md) | 本批28源码／32文档完整列举 | 否 |

范围仅用于材料清点，不是代码行数、功能完成数量、编译或测试通过率。原请求／上传值／actualRaw／canonical四者分开；缺省旧v2身份不自动升级，明确v3必须匹配当前工程声明。最终StartPermit及六类实质开发仍未完成，48项场景见[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)，全部未执行。下方535及更早为历史。



## 535：异机完整包络比较（36路径，源码静态收口）

15源码／工程登记＋21文档=36路径，新增仅 InterVehicleEnvelopeReview.h／cc／md 三条。无新外部依赖、UI、网络命令或 PX4 改动；新纯值模块登记于既有 custom.pri。仅列本批范围，其余累计本地修改保留，未提交、更新 PR 或打包。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [custom/custom.pri](../../custom/custom.pri) | 新纯值模块源／头登记 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.h](../../custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.h) | 封闭完整组件与异机比较接口 | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.cc](../../custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.cc) | 全异机闭包络求交与报告匹配 | 是 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupDomainReview.h](../../custom/src/Swarm/FleetMissionStartupDomainReview.h) | 同次完整原生域可选输出 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupDomainReview.cc](../../custom/src/Swarm/FleetMissionStartupDomainReview.cc) | 完整Domain组件封闭导出 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionWaypointSwitchReview.h](../../custom/src/Swarm/FleetMissionWaypointSwitchReview.h) | 同次NAV／起降组件可选输出 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionWaypointSwitchReview.cc](../../custom/src/Swarm/FleetMissionWaypointSwitchReview.cc) | 完整Waypoint组件封闭导出 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupClearanceWorker.h](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.h) | exact8结果及共同准入合同 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupClearanceWorker.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.cc) | 同次计算／匹配／全部结果消费 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc) | 普通审阅的异机未知／冲突阻断 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc) | 地面再审阅与实际风险消费 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupClearanceWitness.h](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.h) | 原源Witness合同 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupClearanceWitness.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.cc) | 工厂共同结论复核 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionStartupAuthority.cc](../../custom/src/Swarm/FleetMissionStartupAuthority.cc) | exact8及原Witness接管 | 否 |
| 源码／工程登记 | [custom/src/Swarm/FleetMissionGroundStartupAuthority.cc](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.cc) | 地面exact8及原Witness接管 | 否 |
| 文档 | [README.md](../../README.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 最新阶段入口与完整目标边界 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.md](../../custom/src/Swarm/FleetMissionInterVehicleEnvelopeReview.md) | 完整算法与报告合同 | 是 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainReview.md](../../custom/src/Swarm/FleetMissionStartupDomainReview.md) | 原生Domain输出合同 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionWaypointSwitchReview.md](../../custom/src/Swarm/FleetMissionWaypointSwitchReview.md) | 原生Waypoint输出合同 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupClearanceWorker.md](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md) | 后台与共同准入合同 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md) | 普通审阅接线 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md) | 地面审阅接线 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupClearanceWitness.md](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md) | 原生来源延续 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionStartupAuthority.md](../../custom/src/Swarm/FleetMissionStartupAuthority.md) | 普通启动依据绑定 | 否 |
| 文档 | [custom/src/Swarm/FleetMissionGroundStartupAuthority.md](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md) | 地面启动依据绑定 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 限定静态审阅与材料记录 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 48项工程场景全部未执行 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本批完整路径清单 | 否 |

[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)48项全部未执行，限定直接静读与材料核对见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。下方534及更早为历史。

## 534：参数导入原生事务（27路径，源码静态收口）

11源码／工程登记＋16文档=27路径，新增两份合同；没有新服务、外部依赖、wire协议或PX4修改。本表只列本批范围，其余累计本地修改保留；没有提交、更新PR或打包。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [src/FactSystem/ParameterManager.h](../../src/FactSystem/ParameterManager.h) | native导入API、只读状态与入站票据接口 | 否 |
| 源码／工程登记 | [src/FactSystem/ParameterManager.cc](../../src/FactSystem/ParameterManager.cc) | 独立事务、来源、调度、匹配和竞合 | 否 |
| 源码／工程登记 | [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h) | private typed桥与writer ledger | 否 |
| 源码／工程登记 | [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc) | 最早RAW票据及最终writer前来源校验 | 否 |
| 源码／工程登记 | [src/FirmwarePlugin/APM/APMFirmwarePlugin.h](../../src/FirmwarePlugin/APM/APMFirmwarePlugin.h) | 真实组件encoding查询声明 | 否 |
| 源码／工程登记 | [src/FirmwarePlugin/APM/APMFirmwarePlugin.cc](../../src/FirmwarePlugin/APM/APMFirmwarePlugin.cc) | 只读actual APM numeric-float模式查询 | 否 |
| 源码／工程登记 | [src/QmlControls/ParameterEditorController.h](../../src/QmlControls/ParameterEditorController.h) | 状态／准确取消接口 | 否 |
| 源码／工程登记 | [src/QmlControls/ParameterEditorController.cc](../../src/QmlControls/ParameterEditorController.cc) | 私有候选原生接纳，不乐观改live Fact | 否 |
| 源码／工程登记 | [src/QmlControls/ParameterEditor.qml](../../src/QmlControls/ParameterEditor.qml) | 原事务状态与准确ID停止入口 | 否 |
| 源码／工程登记 | [src/QmlControls/ParameterDiffDialog.qml](../../src/QmlControls/ParameterDiffDialog.qml) | 确认文案改为接纳／排队 | 否 |
| 源码／工程登记 | [src/FirmwarePlugin/CMakeLists.txt](../../src/FirmwarePlugin/CMakeLists.txt) | CMake实际APM编入与共享开关一致 | 否 |
| 文档 | [README.md](../../README.md) | 最新阶段入口 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 最新修改入口 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 事务／传输／界面合同索引 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 完整目标与本轮边界 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 代码／工程分工 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 真实发送增量与剩余 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 材料核对记录 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 最新阶段状态 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 工程交接入口 | 否 |
| 文档 | [src/FactSystem/ParameterImportTransaction.md](../../src/FactSystem/ParameterImportTransaction.md) | PM原生事务完整合同 | 是 |
| 文档 | [src/Vehicle/ParameterImportTransport.md](../../src/Vehicle/ParameterImportTransport.md) | Vehicle typed传输完整合同 | 是 |
| 文档 | [src/FactSystem/ParameterManager.md](../../src/FactSystem/ParameterManager.md) | 实际PM入口增量 | 否 |
| 文档 | [src/QmlControls/ParameterEditorController.md](../../src/QmlControls/ParameterEditorController.md) | Controller接纳与状态区合同 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 直接静读和材料审计 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 64项全部未执行 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本表27路径 | 否 |

[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)64项全部未执行；静态审阅见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。下方533及更早为历史。


## 533：参数文件实际导入链（17路径，源码限定静态收口）

4源码／QML＋13文档=17路径，新增ParameterEditorController.md一份合同；无新依赖、工程登记或PX4变更。仅列本批范围，保留其余累计本地修改，没有提交、更新PR或打包。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [src/QmlControls/ParameterEditorController.h](../../src/QmlControls/ParameterEditorController.h) | 预览token／错误与私有候选接口 | 否 |
| 源码／工程登记 | [src/QmlControls/ParameterEditorController.cc](../../src/QmlControls/ParameterEditorController.cc) | 完整文件、独立预览与受控逐项提交 | 否 |
| 源码／工程登记 | [src/QmlControls/ParameterEditor.qml](../../src/QmlControls/ParameterEditor.qml) | 复制原预览token与载入失败提示 | 否 |
| 源码／工程登记 | [src/QmlControls/ParameterDiffDialog.qml](../../src/QmlControls/ParameterDiffDialog.qml) | 提交/勾选/关闭关联原token与过时反馈 | 否 |
| 文档 | [README.md](../../README.md) | 最新阶段入口 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 最新改动入口 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 导入合同和交接索引 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 原目标及本批范围 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 工程师分工 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 实际入口与剩余sender边界 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 普通文档审计 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 最新阶段状态 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 工程交接入口 | 否 |
| 文档 | [src/QmlControls/ParameterEditorController.md](../../src/QmlControls/ParameterEditorController.md) | 实际导入行为和未验证范围 | 是 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 直接静读与材料核对记录 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 48项未执行场景 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本表17条路径 | 否 |

[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)48项全部未执行；静态复核见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。下方532及更早为历史。


## 532：元数据后台调度（27路径，源码静态收口）

10源码／工程登记＋17文档=27路径，新增3条。仅本批路径，不等于相对HEAD全部累计改动；保留其他本地修改。没有提交、更新PR或打包。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataWorkScheduler.h](../../src/Vehicle/ComponentMetadataWorkScheduler.h) | 进程级调度API／一次受理与槽定义 | 是 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataWorkScheduler.cc](../../src/Vehicle/ComponentMetadataWorkScheduler.cc) | FIFO、取消／排队期限、actual finished释放 | 是 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataArtifactWorker.h](../../src/Vehicle/ComponentMetadataArtifactWorker.h) | Artifact受理与neverStarted接口 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataArtifactWorker.cc](../../src/Vehicle/ComponentMetadataArtifactWorker.cc) | Artifact调度／取消和未启动清理 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataTranslationWorker.h](../../src/Vehicle/ComponentMetadataTranslationWorker.h) | Summary／Translate调度接口 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataTranslationWorker.cc](../../src/Vehicle/ComponentMetadataTranslationWorker.cc) | 两阶段共用预算和取消终态 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentInformationManager.cc](../../src/Vehicle/ComponentInformationManager.cc) | 实际Artifact消费者与拒绝／取消分支 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentInformationTranslation.cc](../../src/Vehicle/ComponentInformationTranslation.cc) | 实际翻译消费者与旧早退旁路移除 | 否 |
| 源码／工程登记 | [src/Vehicle/CMakeLists.txt](../../src/Vehicle/CMakeLists.txt) | CMake登记新scheduler | 否 |
| 源码／工程登记 | [qgroundcontrol.pro](../../qgroundcontrol.pro) | qmake登记同一scheduler | 否 |
| 文档 | [README.md](../../README.md) | 当前批次入口 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 最新修改入口 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 调度和交接索引 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 路线图增量与完整目标 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 职责边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 目标实际接线状态 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 文档核对记录 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前代码阶段状态 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 工程交接入口 | 否 |
| 文档 | [src/Vehicle/ComponentMetadataWorkScheduler.md](../../src/Vehicle/ComponentMetadataWorkScheduler.md) | 调度范围和生命周期合同 | 是 |
| 文档 | [src/Vehicle/ComponentMetadataArtifactWorker.md](../../src/Vehicle/ComponentMetadataArtifactWorker.md) | Artifact调度消费合同 | 否 |
| 文档 | [src/Vehicle/ComponentMetadataTranslationWorker.md](../../src/Vehicle/ComponentMetadataTranslationWorker.md) | 两阶段worker调度合同 | 否 |
| 文档 | [src/Vehicle/ComponentInformationManager.md](../../src/Vehicle/ComponentInformationManager.md) | 实际Manager接线说明 | 否 |
| 文档 | [src/Vehicle/ComponentInformationTranslation.md](../../src/Vehicle/ComponentInformationTranslation.md) | 实际Translation接线说明 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 独立静读和材料核对记录 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 40项全部未执行工程场景 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本表27条准确路径 | 否 |

[工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)40项全部未执行；静态复核进度见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。下方531及更早为历史。


## 531：参数缓存 v3（22路径，源码静态收口）

7源码／工程登记＋15文档=22条准确路径，新增3条。不是相对HEAD全部dirty diff，不把其它累计改动归入本批；数量不是产品完成率或运行验收。没有提交、PR更新或打包，实际PX4代码仅只读证据、不修改。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [src/FactSystem/ParameterCacheFile.h](../../src/FactSystem/ParameterCacheFile.h) | v3纯值候选、规范值和PX4窄CRC贡献接口 | 是 |
| 源码／工程登记 | [src/FactSystem/ParameterCacheFile.cc](../../src/FactSystem/ParameterCacheFile.cc) | 有界显式十型LE格式、完整EOF／位模式／排序校验 | 是 |
| 源码／工程登记 | [src/FactSystem/ParameterManager.h](../../src/FactSystem/ParameterManager.h) | 原cache source／activity／消费fence私有契约 | 否 |
| 源码／工程登记 | [src/FactSystem/ParameterManager.cc](../../src/FactSystem/ParameterManager.cc) | 真实HASH入口、typed CRC／读写／模型与完成／固定ACK接线 | 否 |
| 源码／工程登记 | [src/FactSystem/Fact.cc](../../src/FactSystem/Fact.cc) | 实际IEEE位模式相等与NaN容器通知尾 | 否 |
| 源码／工程登记 | [src/FactSystem/CMakeLists.txt](../../src/FactSystem/CMakeLists.txt) | 登记ParameterCacheFile两个新hcc | 否 |
| 源码／工程登记 | [qgroundcontrol.pro](../../qgroundcontrol.pro) | qmake登记同一两个新hcc | 否 |
| 文档 | [README.md](../../README.md) | 本批阶段和未验证边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前变更入口 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前codec／consumer／交接索引 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 531增量、原25目标与六类剩余保持 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 源码与工程师责任边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 实际功能链增量，不提高验收率 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 文档及普通材料核对边界 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 最新源码阶段状态 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 最新工程交接入口 | 否 |
| 文档 | [src/FactSystem/ParameterCacheFile.md](../../src/FactSystem/ParameterCacheFile.md) | 格式／十型／IEEE与本地6C CRC域 | 是 |
| 文档 | [src/FactSystem/ParameterManager.md](../../src/FactSystem/ParameterManager.md) | 原PRIMARY／metadata／volatile／完成与原子写入合同 | 否 |
| 文档 | [src/FactSystem/Fact.md](../../src/FactSystem/Fact.md) | 531 IEEE容器更新及保留的530边界 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 限定独立静读与普通材料记录 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 64项全部未执行工程合同 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本表22条准确路径 | 否 |

[64项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，限定静读和普通材料核对记于[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。不是安装包或可发布版本。原25目标／六类实质剩余及约40%源码／60%剩余粗估保持。下方530及更早为历史。


## 530：离线参数实际值（22路径，源码静态收口）

7源码／工程登记＋15文档=22条准确路径，新增5条。只列本批准确范围，不把累计dirty变更归入本批，不以路径或批次数推算产品完成率。保所有其它本地改动；没有提交、PR更新、打包或实际参数／文件业务IO。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [src/FactSystem/OfflineParameterFile.h](../../src/FactSystem/OfflineParameterFile.h) | 纯值五列文件候选接口、4MiB上限与精确值比较 | 是 |
| 源码／工程登记 | [src/FactSystem/OfflineParameterFile.cc](../../src/FactSystem/OfflineParameterFile.cc) | 十数型完整校验、整数域／有限实数／负零／预算／坏尾整份拒 | 是 |
| 源码／工程登记 | [src/FactSystem/ParameterManager.h](../../src/FactSystem/ParameterManager.h) | 离线候选加载防重入私有状态 | 否 |
| 源码／工程登记 | [src/FactSystem/ParameterManager.cc](../../src/FactSystem/ParameterManager.cc) | 真实有界读取、原来源及全Fact候选、实际值安装和一次发布 | 否 |
| 源码／工程登记 | [src/FactSystem/Fact.cc](../../src/FactSystem/Fact.cc) | 容器实际类型与浮点零符号赋值、转换及通知尾复核 | 否 |
| 源码／工程登记 | [src/FactSystem/CMakeLists.txt](../../src/FactSystem/CMakeLists.txt) | 登记OfflineParameterFile两个新hcc | 否 |
| 源码／工程登记 | [qgroundcontrol.pro](../../qgroundcontrol.pro) | qmake登记同一codec两个新hcc | 否 |
| 文档 | [README.md](../../README.md) | 本批源码阶段入口及未验证边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前修改入口 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | codec／loader／Fact合同与交接索引 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 530增量、原25目标／六类剩余不缩减 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 源码与工程师责任边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 实际消费链增量，不提高验收率 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 文档及普通材料核对边界 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 最新源码阶段状态 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 最新工程交接入口 | 否 |
| 文档 | [src/FactSystem/OfflineParameterFile.md](../../src/FactSystem/OfflineParameterFile.md) | 文件格式、十数型、资源预算及历史模板兼容域 | 是 |
| 文档 | [src/FactSystem/ParameterManager.md](../../src/FactSystem/ParameterManager.md) | 构造窗口、候选／元数据所有权、原来源及发布合同 | 是 |
| 文档 | [src/FactSystem/Fact.md](../../src/FactSystem/Fact.md) | 实际typed0／负零与容器通知边界 | 是 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 限定独立静读与普通文本核对记录 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 48项全未执行工程合同 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本表22条准确路径 | 否 |

源码实现及限定静态交叉审阅已收口，普通材料核对记录于[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。[48项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；不是安装包或可发布版本。原25目标、六类实质剩余及约40%源码／60%剩余粗估保持。下方529及更早为历史。


## 529：后台有界翻译（25路径，源码静态收口）

9源码／工程登记＋16文档=25条准确路径，新增6条。不把其它全仓dirty变更算入本批；数量不是产品完成率、真实构建结果或安装包。未提交、更新PR、打包或执行实际业务IO。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [src/Vehicle/ComponentTranslationEngine.h](../../src/Vehicle/ComponentTranslationEngine.h) | 纯native summary/翻译接口与原clock/cancel约束 | 是 |
| 源码／工程登记 | [src/Vehicle/ComponentTranslationEngine.cc](../../src/Vehicle/ComponentTranslationEngine.cc) | 完整有界文件/JSON/XML/规则/合法递归及unique输出 | 是 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataTranslationWorker.h](../../src/Vehicle/ComponentMetadataTranslationWorker.h) | 两段parentlessworker及自持纯native输入 | 是 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataTranslationWorker.cc](../../src/Vehicle/ComponentMetadataTranslationWorker.cc) | XZ/engine后台处理、独立结果及输出转移 | 是 |
| 源码／工程登记 | [src/Vehicle/ComponentInformationTranslation.h](../../src/Vehicle/ComponentInformationTranslation.h) | artifact原生ID入口/独立信号/精确取消 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentInformationTranslation.cc](../../src/Vehicle/ComponentInformationTranslation.cc) | Summary→下载→Translate actualworker双尾协调 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentInformationManager.cc](../../src/Vehicle/ComponentInformationManager.cc) | 真实调用者/外国输出隔离/来源退休晚取消 | 否 |
| 源码／工程登记 | [src/Vehicle/CMakeLists.txt](../../src/Vehicle/CMakeLists.txt) | 登记Engine/Worker四新hcc | 否 |
| 源码／工程登记 | [qgroundcontrol.pro](../../qgroundcontrol.pro) | qmake登记同一四新hcc | 否 |
| 文档 | [README.md](../../README.md) | 本批进度与未验证边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 本批修改入口 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前核心/worker/交接索引 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 原25目标/六组剩余及529增量 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 源码与工程师责任边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 实际接入增量，不提高产品验收率 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 文档核对边界 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 最新源码阶段状态 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 最新交接入口 | 否 |
| 文档 | [src/Vehicle/ComponentTranslationEngine.md](../../src/Vehicle/ComponentTranslationEngine.md) | 语义/资源/递归/数字context边界 | 是 |
| 文档 | [src/Vehicle/ComponentMetadataTranslationWorker.md](../../src/Vehicle/ComponentMetadataTranslationWorker.md) | 实际worker输入与输出寿命 | 是 |
| 文档 | [src/Vehicle/ComponentInformationTranslation.md](../../src/Vehicle/ComponentInformationTranslation.md) | 两段后台及legacy/domain合同 | 否 |
| 文档 | [src/Vehicle/ComponentInformationManager.md](../../src/Vehicle/ComponentInformationManager.md) | 529生产真实接线及权限边界 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 限定独立静读/普通文本检查记录 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 48项全部未执行工程合同 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本表25条准确路径 | 否 |

源码与限定独立静读已收口，普通材料核对记于[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。[48项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25目标及六类实质剩余保持，约40%源码／60%剩余仅粗估。下方528及更早是历史。


## 528：下载原件唯一所有权（34路径，源码静态收口）

15源码／工程登记＋19文档，共34条准确路径；其中6条新增。计数不是25目标完成率，也不是已构建安装包。原目录的其它累计改动保留，不把全仓dirty diff归为本批。没有提交、PR更新、打包或实际下载／缓存／临时文件业务IO。

| 类别 | 路径 | 本批用途 | 新增 |
| --- | --- | --- | --- |
| 源码／工程登记 | [src/CMakeLists.txt](../../src/CMakeLists.txt) | 登记共同原输入owner的h/cc，不增加外部依赖 | 否 |
| 源码／工程登记 | [qgroundcontrol.pro](../../qgroundcontrol.pro) | qmake登记同一共同owner | 否 |
| 源码／工程登记 | [src/QGCOwnedDownloadFile.h](../../src/QGCOwnedDownloadFile.h) | 原生fresh唯一路径共享owner接口 | 是 |
| 源码／工程登记 | [src/QGCOwnedDownloadFile.cc](../../src/QGCOwnedDownloadFile.cc) | 独占创建/关闭转移及准确原件尽力清理 | 是 |
| 源码／工程登记 | [src/QGCFileDownload.h](../../src/QGCFileDownload.h) | opt-in unique API与独立完成信号 | 否 |
| 源码／工程登记 | [src/QGCFileDownload.cc](../../src/QGCFileDownload.cc) | 原job/reply/redirect/期限及32MiB完整IO | 否 |
| 源码／工程登记 | [src/QGCCachedFileDownload.h](../../src/QGCCachedFileDownload.h) | 独立缓存任务上下文/API/信号 | 否 |
| 源码／工程登记 | [src/QGCCachedFileDownload.cc](../../src/QGCCachedFileDownload.cc) | 原job缓存年龄、一次cache-only回退/取消 | 否 |
| 源码／工程登记 | [src/Vehicle/FTPManager.h](../../src/Vehicle/FTPManager.h) | metadata-only unique接口与原请求状态 | 否 |
| 源码／工程登记 | [src/Vehicle/FTPManager.cc](../../src/Vehicle/FTPManager.cc) | 准确原件/32MiB/完整写尾/serial与取消 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentInformationManager.cc](../../src/Vehicle/ComponentInformationManager.cc) | 实际HTTP/FTP入口、ID/path围栏与受理退场 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataArtifactWorker.h](../../src/Vehicle/ComponentMetadataArtifactWorker.h) | 实际worker自行持原输入owner | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentMetadataArtifactWorker.cc](../../src/Vehicle/ComponentMetadataArtifactWorker.cc) | 路径一致门与parentless输入寿命 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentInformationTranslation.h](../../src/Vehicle/ComponentInformationTranslation.h) | 原TS请求与析构/准确终结 | 否 |
| 源码／工程登记 | [src/Vehicle/ComponentInformationTranslation.cc](../../src/Vehicle/ComponentInformationTranslation.cc) | TS fresh输入、同步受理与处理栈持有 | 否 |
| 文档 | [README.md](../../README.md) | 当前源码进度/未验证边界 | 否 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 本批修改入口 | 否 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 新下载所有权与实际接线索引 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 原25目标/六组剩余及528进度 | 否 |
| 文档 | [docs/MERIVUS_FLEET_OS_COLLABORATION.md](../../docs/MERIVUS_FLEET_OS_COLLABORATION.md) | 本批源码/工程师责任边界 | 否 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前接入增量，不缩目标/上调粗估 | 否 |
| 文档 | [docs/DOCUMENT_AUDIT.md](../../docs/DOCUMENT_AUDIT.md) | 528文档核对边界 | 否 |
| 文档 | [src/QGCOwnedDownloadFile.md](../../src/QGCOwnedDownloadFile.md) | fresh owner/single-transfer/清理边界合同 | 是 |
| 文档 | [src/QGCFileDownload.md](../../src/QGCFileDownload.md) | 新HTTP与legacy域隔离合同 | 是 |
| 文档 | [src/QGCCachedFileDownload.md](../../src/QGCCachedFileDownload.md) | 新Cached一次回退与期限合同 | 是 |
| 文档 | [src/Vehicle/FTPManager.md](../../src/Vehicle/FTPManager.md) | 新FTP完整IO/来源与legacy边界 | 是 |
| 文档 | [src/Vehicle/ComponentInformationManager.md](../../src/Vehicle/ComponentInformationManager.md) | 528实际接线与原source/phase合同 | 否 |
| 文档 | [src/Vehicle/ComponentMetadataArtifactWorker.md](../../src/Vehicle/ComponentMetadataArtifactWorker.md) | worker实际持源/缓存不接管 | 否 |
| 文档 | [src/Vehicle/ComponentInformationTranslation.md](../../src/Vehicle/ComponentInformationTranslation.md) | TS原件/解压/译文分属不同owner | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 限定静读/普通材料核对记录 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 64项全部未执行的工程合同 | 否 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本表34路径的准确范围 | 否 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 最新项目状态及未执行边界 | 否 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 最新工程交接入口 | 否 |

实际代码与限定独立静读已收口，普通材料核对记录于[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)；[64项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。保持原25目标及六类实质开发，约40%源码／60%剩余仅粗估。下方527及更早为历史。


## 527 本批范围（源码静态收口）：12源码／工程登记＋13文档=25路径

本批从参数JSON实际消费者消除坏文件半装和raw模板外借，新增参数专用纯值校验／构造、候选目录切换、actual Fact-owned副本及已知volatile查询；Manager补各组件目录退休，FirmwarePlugin补现有opaque所有权合同。原HASH快径增加first-source／目录revision及私有固定ACK实际write入口门，不扩其它generic sender。原通用factory不改；不是相对HEAD全部dirty差异，不以累计diff声称本批行数。

新路径4条：ComponentParameterMetadata h／cc／md及CompInfoParam.md。无新UI／QML服务、wire、外部依赖、数据库schema、参数写权限或PX4改动。

### 源码与工程登记（12）

- `src/Vehicle/ComponentParameterMetadata.h`
- `src/Vehicle/ComponentParameterMetadata.cc`
- `src/Vehicle/CompInfoParam.h`
- `src/Vehicle/CompInfoParam.cc`
- `src/FactSystem/ParameterManager.cc`
- `src/Vehicle/ComponentInformationManager.h`
- `src/Vehicle/ComponentInformationManager.cc`
- `src/FirmwarePlugin/FirmwarePlugin.h`
- `src/Vehicle/CMakeLists.txt`
- `qgroundcontrol.pro`
- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`

### 配套文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `src/Vehicle/ComponentParameterMetadata.md`
- `src/Vehicle/CompInfoParam.md`
- `src/Vehicle/ComponentInformationManager.md`

本批源码实现与限定静态交叉审阅已收口。58项工程场景全部未执行；未编译、执行项目／parser／helper／CRC／hash／文件业务IO／下载／缓存、测试、仿真、硬件／PX4、刷机、放行、提交、更新PR或打包。原XML／QDataStream完整schema及预算、已发生cache Fact前缀、既有Fact热刷新、底层下载basename／翻译完整图预算等仍独立待后续。以下为历史。

## 526 本批范围（源码静态收口）：18源码／工程登记＋15文档=33路径

本批将组件元数据现下载／缓存路径统一接后台字节核对与独立JSON快照，增加raw缓存namespace、精确坏项退休、有界解压、General目录原子接纳、唯一翻译输出及回调寿命／Vehicle早关闭门；不是相对HEAD全部dirty差异，未统计本批新增／删除行数。新路径7条为Worker h／cc／md及Manager／Cache／General／Translation合同；没有新UI、QML服务、wire、外部依赖、SQL schema、权限或PX4改动。

### 源码与工程登记（18）

- `src/Vehicle/ComponentInformationManager.h`
- `src/Vehicle/ComponentInformationManager.cc`
- `src/Vehicle/ComponentMetadataArtifactWorker.h`
- `src/Vehicle/ComponentMetadataArtifactWorker.cc`
- `src/Vehicle/ComponentInformationCache.h`
- `src/Vehicle/ComponentInformationCache.cc`
- `src/Vehicle/CompInfo.cc`
- `src/Vehicle/CompInfoGeneral.h`
- `src/Vehicle/CompInfoGeneral.cc`
- `src/Compression/QGCLZMA.h`
- `src/Compression/QGCLZMA.cc`
- `src/Vehicle/InitialConnectStateMachine.cc`
- `src/Vehicle/CMakeLists.txt`
- `qgroundcontrol.pro`
- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `src/Vehicle/ComponentInformationTranslation.h`
- `src/Vehicle/ComponentInformationTranslation.cc`

### 配套文档（15）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `src/Vehicle/ComponentInformationManager.md`
- `src/Vehicle/ComponentMetadataArtifactWorker.md`
- `src/Vehicle/ComponentInformationCache.md`
- `src/Vehicle/CompInfoGeneral.md`
- `src/Vehicle/ComponentInformationTranslation.md`

本批源码实现及限定静态交叉审阅已收口；48工程场景全部未执行，需完整累计工程，不是可运行安装包。没有执行CRC／hash／解压／下载／缓存或业务档案IO、编译、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。参数工厂、底层下载输入重名与翻译完整递归图预算仍待后续。以下为历史。
## 525 本批范围（源码静态收口）：10源码／资源＋15文档=25路径

相对524增加独立原生备份依据、真实只读清理预览、Windows同handle原件删除、不可覆盖意图／终态收据与原维护页嵌入；旧改名、备份、诊断／导入合同不放宽。此为本批范围，不是相对HEAD全部dirty差异；未统计批次新增／删除行数。新路径仅PurgePanel.qml及其合同两条，无新后端服务、wire、外部依赖、SQL schema、权限或PX4改动。

### 源码与资源登记（10）

- `custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h`
- `custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc`
- `custom/src/Swarm/FleetPersistentEventStore.h`
- `custom/src/Swarm/FleetPersistentEventStore.cc`
- `custom/res/Merivus/FleetArchiveMaintenancePanel.qml`
- `custom/res/Merivus/FleetRetiredArchivePurgePanel.qml`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetArchiveHistoryPanel.qml`
- `custom/qgroundcontrol.qrc`
- `custom/res/Merivus/qmldir`

### 配套文档（15）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md`
- `custom/src/Swarm/FleetPersistentEventStore.md`
- `custom/src/Swarm/FleetArchiveMaintenancePanel.md`
- `custom/src/Swarm/FleetRetiredArchivePurgePanel.md`
- `custom/src/Swarm/FleetArchiveHistoryPanel.md`

本批源码及限定静态审阅已收口，64项工程场景全部未执行；需完整累计工程，不是可运行安装包。没有实际业务归档IO／删除、编译、测试、仿真、硬件、刷机、放行、提交、PR更新或打包。下方为历史。

## 524 本批范围：19源码／资源登记＋20文档=39路径

本批相对523增加已闭旧本地部署的明确归档、回读后原生退休与实际销毁回收槽，以及独立文件历史的单报告文本浏览。不是相对HEAD全部dirty差异，未统计批次新增／删除行数。新路径6条：Episode合同、worker h／cc／md、ArchivePanel.qml及合同；其它33路径为既有累计工程修改。无新QML后端服务、wire、外部依赖、SQL schema、权限或PX4改动，不含安装包／二进制／PR更新。

### 源码与资源登记（19）

- `custom/src/Swarm/FleetMissionDeploymentLifecycle.h`
- `custom/src/Swarm/FleetMissionDeploymentLifecycle.cc`
- `custom/src/Swarm/FleetMissionDeploymentEpisode.h`
- `custom/src/Swarm/FleetMissionDeploymentEpisode.cc`
- `custom/src/Swarm/SwarmMissionOrchestrator.h`
- `custom/src/Swarm/SwarmMissionOrchestrator.cc`
- `custom/src/Swarm/FleetMissionStartupTransaction.h`
- `custom/src/Swarm/FleetMissionStartupTransaction.cc`
- `custom/src/Swarm/FleetMissionExecutionObservation.cc`
- `custom/src/Swarm/FleetDeploymentArchiveWorker.h`
- `custom/src/Swarm/FleetDeploymentArchiveWorker.cc`
- `custom/src/Swarm/FleetMissionExecutionReport.h`
- `custom/src/Swarm/FleetMissionExecutionReport.cc`
- `custom/res/Merivus/FleetMissionLifecyclePanel.qml`
- `custom/res/Merivus/FleetDeploymentArchivePanel.qml`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/custom.pri`
- `custom/qgroundcontrol.qrc`
- `custom/res/Merivus/qmldir`

### 配套文档（20）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetMissionDeploymentLifecycle.md`
- `custom/src/Swarm/FleetMissionDeploymentEpisode.md`
- `custom/src/Swarm/SwarmMissionOrchestrator.md`
- `custom/src/Swarm/FleetMissionStartupTransaction.md`
- `custom/src/Swarm/FleetMissionExecutionObservation.md`
- `custom/src/Swarm/FleetDeploymentArchiveWorker.md`
- `custom/src/Swarm/FleetMissionExecutionReport.md`
- `custom/src/Swarm/FleetMissionLifecyclePanel.md`
- `custom/src/Swarm/FleetDeploymentArchivePanel.md`
- `custom/src/Swarm/FleetSnapshotFileIo.md`

上述39路径须与完整累计工程共同交接。仅源码与限定静态审阅；56项工程场景全部未执行，未编译、运行项目实现、测试、仿真、硬件、刷机、放行、提交或打包。下方保留历史范围。

## 523 本批范围：18源码／资源登记＋18文档=36路径

相对522增加真实GNSS报告的独立有界集合、冻结保存／独立重开和历史来源地图，不是相对HEAD全部dirty差异。新增8路径为共享文件IO h／cc／md、专用GNSS工作worker h／cc／md与Panel.qml／合同md；新worker不是公开QML服务，没有新wire、外部依赖、数据库schema、权限或PX4改动。

### 源码与资源登记（18）

- `custom/src/Swarm/FleetOperationalMemory.h`
- `custom/src/Swarm/FleetOperationalMemory.cc`
- `custom/src/Swarm/FleetTelemetryGnssReview.h`
- `custom/src/Swarm/FleetTelemetryGnssReview.cc`
- `custom/src/Swarm/FleetTelemetryArchive.h`
- `custom/src/Swarm/FleetTelemetryArchive.cc`
- `custom/src/Swarm/FleetSnapshotFileIo.h`
- `custom/src/Swarm/FleetSnapshotFileIo.cc`
- `custom/src/Swarm/FleetGnssMemoryWorkspaceWorker.h`
- `custom/src/Swarm/FleetGnssMemoryWorkspaceWorker.cc`
- `custom/src/Swarm/FleetMaintenanceWorkspaceWorker.cc`
- `custom/res/Merivus/FleetGnssMemoryWorkspacePanel.qml`
- `custom/res/Merivus/FleetTelemetryHistoryPanel.qml`
- `custom/res/Merivus/FlyViewMap.qml`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/custom.pri`
- `custom/qgroundcontrol.qrc`
- `custom/res/Merivus/qmldir`

### 文档（18）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetOperationalMemory.md`
- `custom/src/Swarm/FleetTelemetryGnssReview.md`
- `custom/src/Swarm/FleetTelemetryArchive.md`
- `custom/src/Swarm/FleetSnapshotFileIo.md`
- `custom/src/Swarm/FleetGnssMemoryWorkspaceWorker.md`
- `custom/src/Swarm/FleetMaintenanceWorkspace.md`
- `custom/src/Swarm/FleetGnssMemoryWorkspacePanel.md`
- `custom/src/Swarm/FleetTelemetryHistoryPanel.md`

原FleetOperationalMemory承载独立32报告／2048样本集合，不走旧宽松归一化、trim／KPI；新纯值校验及去重复用到worker。新Archive native getter仅实际token来源；Root只桥接原两个native owner和三类历史地图来源。保存冻结字节，不保存当前live；打开文件只独立槽，不导入active或恢复权限。

共享IO抽取原维护路径／读写／commit-readback语义，原维护32MiB／error／reply不改，GNSS用4MiB。Panel沿原History两个宿主接入，分层明确确认，目录每页8、选单份64，不自动选图／续读／移图。所有48工程场景未执行；未编译、执行项目／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、预览、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 522 本批范围：8源码／工程登记＋13文档=21路径

相对521新增同包历史GPS_RAW_INT诊断与显式可选地图，不是相对HEAD所有dirty差异。新路径仅FleetTelemetryGnssReview h／cc／md三条；新helper是纯值类，不注册QML服务，没有新资源／wire／外部依赖／数据库schema／权限或PX4改动。

### 源码与工程登记（8）

- `custom/src/Swarm/FleetTelemetryGnssReview.h`
- `custom/src/Swarm/FleetTelemetryGnssReview.cc`
- `custom/src/Swarm/FleetTelemetryArchive.h`
- `custom/src/Swarm/FleetTelemetryArchive.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetTelemetryHistoryPanel.qml`
- `custom/res/Merivus/FlyViewMap.qml`
- `custom/custom.pri`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetTelemetryGnssReview.md`
- `custom/src/Swarm/FleetTelemetryArchive.md`
- `custom/src/Swarm/FleetTelemetryHistoryPanel.md`

报告复用现MotionReview完整原生页校验再单包解码；fix和未知值分类、完整来源／计数保留。Root只按原实际报告token选图，保独立选择代次与撤销lease，公共只读投影固定深度冻结。界面生成与选图分开，地图不连线／移图／控制；旧页不能撤另一页新选择。原运动／维护和采集链不重写。

仅源码、直接静读和普通文本／路径／Markdown／空白核对；40工程场景全部未执行。未编译、执行项目／helper／QML／JS／业务／模型／认证／SQL／hash实现、测试、预览、仿真、硬件／PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 521 本批范围：9源码＋18文档=27既有路径

相对520补普通区域任务的两项性能下限，不是相对HEAD全部dirty差异。无新路径、类／资源、wire、外部依赖、数据库schema、权限或PX4改动；不发送速度设置或飞行命令。草稿来源任意floor才schema6，无floor的原1..5保持。

### 源码（9）

- `custom/src/Swarm/FleetMissionDraft.h`
- `custom/src/Swarm/FleetMissionDraft.cc`
- `custom/res/Merivus/FleetPerformancePolicyEditor.qml`
- `custom/src/Swarm/FleetMissionCompiler.cc`
- `custom/src/Swarm/FleetMissionPlanStore.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/src/Swarm/FleetMissionPayloadBuilder.cc`
- `custom/src/Swarm/FleetCapabilityMatcher.cc`
- `custom/src/Swarm/FleetRoleAllocator.cc`

### 文档（18）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetMissionDraft.md`
- `custom/src/Swarm/FleetPerformancePolicyEditor.md`
- `custom/src/Swarm/FleetMissionCompiler.md`
- `custom/src/Swarm/FleetMissionPlanStore.md`
- `custom/src/Swarm/FleetMissionPayloadBuilder.md`
- `custom/src/Swarm/FleetCapabilityMatcher.md`
- `custom/src/Swarm/FleetRoleAllocator.md`
- `custom/src/Swarm/FleetMissionDraftStore.md`

atomic政策setter一次替换strict和两floor，Matcher／Allocator／Compiler的正下限核对独立于严格开关，缺／−1不能达标；PlanStore和Payload按原area source保完整字段及数值。UI保原来源／revision／pending／确认门，不认证实测性能或恢复批准。仅源码、直接静读和普通文本／路径／Markdown／空白核对；48项工程场景全部未执行。未编译、执行项目／helper／QML／JS／业务／模型／认证／SQL／hash、测试、仿真、硬件／PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 520 本批范围：2源码＋13文档=15既有路径

相对519补备用出勤的显式只读刷新、原请求状态和精确取消，不是相对HEAD全部脏文件。无新路径、组件／资源、C++、wire、外部依赖、数据库schema、权限或PX4修改。底层仍用现FleetMissionEvidenceRefresh；默认地面／角色／占用条件不放宽。

### 源码（2）

- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetReserveDispatchPanel.qml`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetReserveDispatchWorkflow.md`
- `custom/src/Swarm/FleetReserveDispatchPanel.md`
- `custom/src/Swarm/FleetReserveDispatchProfileEditor.md`

源码只增加本页capture／所选备用／actualRefresh／requestId／generation的私有归属与附加撤权围栏，并沿原Qt主题显示有界状态和取消。刷新前退休旧资格但保档案事实；完成后须手动重新观察并采用配置，不自动批准。仅编辑、直接静读和普通文本／路径／Markdown／空白检查；40项工程场景全部未执行。未编译、运行项目／helper／QML／JS、测试、仿真、硬件／PX4操作、刷机、放行、提交、PR更新或打包。下方为历史。

## 519 本批范围：5源码／资源接线＋13文档=18路径

相对518新增备用独立出勤的实际参数offer与结构配置入口，不是相对HEAD全部脏文件。新增路径仅ProfileEditor.qml及对应合同md两条；无新C++、wire、外部依赖、数据库schema或权限，未修改PX4。

### 源码与资源登记（5）

- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetReserveDispatchPanel.qml`
- `custom/res/Merivus/FleetReserveDispatchProfileEditor.qml`
- `custom/res/Merivus/qmldir`
- `custom/qgroundcontrol.qrc`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetReserveDispatchWorkflow.md`
- `custom/src/Swarm/FleetReserveDispatchPanel.md`
- `custom/src/Swarm/FleetReserveDispatchProfileEditor.md`

实际私有capture／备用ID→现Coordinator真实12参数offer19→Root exact10值回执及私有新offerToken→结构配置与原启动策略编辑器→preview再次核fingerprint／profileHash→原Planner／Compiler／Review。底层exact3请求与profile schema不扩，不以public Map恢复真实来源。新读取、变源或旧token不得借同值hash续用。

巡航／返航仍同原备用cruise，垂直速度仍按预算及当前参数；起降点／策略不从旧主机、GNSS或模板补齐。只有明确人工动作生成新完整profile，不自动出勤、接管、改派、上传、授性能认证或StartPermit。

[40项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[工作流合同](../../custom/src/Swarm/FleetReserveDispatchWorkflow.md)。仅源码／直接静读及普通材料核对，未编译、运行项目或helper、测试、仿真、硬件、刷机、放行、提交、更新PR或打包。原25目标与六类实质剩余保持，约40%／60%仍仅源码工作量粗估。下方为历史。

## 518 本批范围：10源码／资源接线＋13文档=23路径

只描述相对517的维护文件工作区增量，不代表相对HEAD全部脏文件。新路径仅worker h／cc、工作区Panel.qml及合同md四条；没有新增QML服务类型、wire、外部依赖、SQL schema、权限或PX4修改。

### 源码与资源登记（10）

- `custom/src/Swarm/FleetMaintenancePassport.h`
- `custom/src/Swarm/FleetMaintenancePassport.cc`
- `custom/src/Swarm/FleetMaintenanceWorkspaceWorker.h`
- `custom/src/Swarm/FleetMaintenanceWorkspaceWorker.cc`
- `custom/res/Merivus/FleetMaintenanceWorkspacePanel.qml`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/CommandCenterOverlay.qml`
- `custom/res/Merivus/qmldir`
- `custom/qgroundcontrol.qrc`
- `custom/custom.pri`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetMaintenancePassport.md`
- `custom/src/Swarm/FleetMaintenanceHistoryImport.md`
- `custom/src/Swarm/FleetMaintenanceWorkspace.md`

实际Passport私有单槽、分turn纯值预检和一次性worker接当前records、不可变导出bytes及独立文件历史。公开request9／receipt11／state11／result11、descriptor18；来源组13和记录8只给有界目录行，完整canonical JSON用detail10／2048 UTF16片，非整条巨型备注推QML。旧legacy根5／历史根6哨兵及source7/evidence21不扩。

保存只使用原session私有snapshotId/hash，另行确认原路径／覆盖风险，不再export最新active records。save回执11分实际写入Known／Written与独立readback，取消不抹commit事实；文件32MiB，详情64MiB，fsync/durable/authenticated均false。打开只替换独立file槽，失败保原来源；同步clear只退休准确槽，不删文件或当前记录，不制造异步job，最后文件回执保留。

[48项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[完整合同](../../custom/src/Swarm/FleetMaintenanceWorkspace.md)。只编辑、直接静读和普通材料文本核对，未编译／执行项目或helper／测试／仿真／设备／刷机／放行／提交／PR／打包。原25目标／六类实质剩余保持，整体约40%／60%仍工作量粗估。下方为历史。

## 517 本批范围：10源码／资源接线＋13文档=23路径

只描述相对516的历史维护数据接线增量，不是相对HEAD全部脏文件。新增路径仅 `custom/res/Merivus/FleetMaintenanceHistoryPanel.qml` 与 `custom/src/Swarm/FleetMaintenanceHistoryImport.md`；无新增C++类型、外部依赖、wire、数据库schema、权限或PX4修改。

### 源码与资源登记（10）

- `custom/src/Swarm/FleetTelemetryArchive.h`
- `custom/src/Swarm/FleetTelemetryArchive.cc`
- `custom/src/Swarm/FleetMaintenancePassport.h`
- `custom/src/Swarm/FleetMaintenancePassport.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetTelemetryHistoryPanel.qml`
- `custom/res/Merivus/FleetMaintenanceHistoryPanel.qml`
- `custom/res/Merivus/CommandCenterOverlay.qml`
- `custom/res/Merivus/qmldir`
- `custom/qgroundcontrol.qrc`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `custom/src/Swarm/FleetTelemetryArchive.md`
- `custom/src/Swarm/FleetMaintenancePassport.md`
- `custom/src/Swarm/FleetMaintenanceHistoryImport.md`

当前原生page36/frame29和旧public summary保持，新候选6／每来源5、私有preview14／source7／evidence21进入实际Passport。公开追加回执5不授认证、实物身份、当前健康或执行许可。文件schema1保留严格历史元数据：纯legacy根5不变，含历史必须带historySourceContract固定字符串成为根6，旧版拒未知根字段，不静默降为同机号legacy。历史按完整来源独立、重叠拒绝、满2000不淘汰。此入口只写内存，不自动保存，既有导出／文件交接可保新元数据但本批未新增保存／重开UI。

[40项场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)仅限定直接静读与普通文本核对。原25目标／六类剩余保持，整体约40%／60%不按本批文件或入口上调。下方为历史。

## 516 本批范围：11源码／接线＋18文档=29既有路径

仅描述相对515的SOC下降窗业务增量，不是相对HEAD全部脏文件。全部29路径既有；没有新增文件、资源、wire、依赖、数据库schema、权限或飞控改动，不从untracked数量推算新增。

### 源码与接线（11）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetNativeBatteryEvidence.h`
- `custom/src/Swarm/FleetNativeBatteryEvidence.cc`
- `custom/src/Swarm/FleetEnergyScheduler.h`
- `custom/src/Swarm/FleetEnergyScheduler.cc`
- `custom/src/Swarm/FleetMissionReviewCoordinator.h`
- `custom/src/Swarm/FleetMissionReviewCoordinator.cc`
- `custom/src/Swarm/FleetRegistry.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetMissionPlannerPanel.qml`

### 文档（18）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetNativeBatteryObservation.md`
- `custom/src/Swarm/FleetNativeBatteryEvidence.md`
- `custom/src/Swarm/FleetEnergyScheduler.md`
- `custom/src/Swarm/FleetMissionReviewCoordinator.md`
- `custom/src/Swarm/FleetNativeBatteryBridge.md`
- `custom/src/Swarm/FleetRegistry.md`
- `custom/src/Swarm/FleetMissionEnergyBudgetPanel.md`

原旧电池20字段及assignment18/21保持；新native top25/window8、adjustment14及bridge6沿实际Registry→Scheduler→Root审阅→原归档消费。仅进行直接静态阅读与普通文本／路径／Markdown／git空白核对；[48项场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，最终材料结果见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。未编译、执行项目或helper、测试、仿真、操作设备／PX4、刷机、放行、提交、更新PR或打包；原25目标／六类剩余保持，约40%源码／60%剩余不因局部字段上调。下方为历史。

## 515 本批范围：6源码／资源接线＋15文档=21路径

本清单只描述相对514的业务增量，不代表相对HEAD全部脏文件。新增路径仅ReviewPanel.qml／ReviewPanel.md两条，其余19条为既有路径；不能从git未跟踪数推算本轮新增。没有新C++、wire、外部依赖、权限、数据库schema、飞控改动或安装包。

Root范围含三处准确短退休收据：初次来源捕获Store、重审Store和末尾直接Intent失效；实际archiveChanged／stateChanged消费同一原来源，初次正常状态推进只容一次，不误退通知中新建的规划。该修正未增加源码路径或改变原无收据调用者；待验证重入与正常推进见交接第36／38项。

### 源码与资源登记（6）

- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetReserveDispatchPanel.qml`
- `custom/res/Merivus/FleetReserveDispatchReviewPanel.qml`
- `custom/res/Merivus/FleetMissionDeploymentPanel.qml`
- `custom/qgroundcontrol.qrc`
- `custom/res/Merivus/qmldir`

### 文档（15）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetReserveDispatchReviewPanel.md`
- `custom/src/Swarm/FleetReserveDispatchPanel.md`
- `custom/src/Swarm/FleetReserveDispatchWorkflow.md`
- `custom/src/Swarm/FleetMissionDeployment.md`

只有直接静态对读及普通文本／存在路径／Markdown／git空白核对。[48项交接场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，最终材料核对见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。未编译、运行项目或解析器、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包；原25目标／六类实质剩余与约40%源码／60%剩余粗估保持，不把本批字段显示和重审当整目标验收。下方为历史。

## 514 本批范围：8源码／资源接线＋18文档=26路径

本清单描述相对于513继续工作的业务增量，不是相对于HEAD的全部脏文件，也不把所有untracked视为本轮新建。新路径仅下列三条：

- `custom/res/Merivus/FleetReserveDispatchPanel.qml`：实际人工备用独立出勤区。
- `custom/src/Swarm/FleetReserveDispatchPanel.md`：面板生命周期、确认与批准合同。
- `custom/src/Swarm/FleetReserveDispatchWorkflow.md`：真实Root来源、回执、归档与共享部署合同。

其余23条均为既有路径；无新C++、外部依赖、wire、权限、数据库schema、飞控改动或安装包。明确采用与本地批准并非上传或飞行许可；当前约40%源码／60%剩余仍仅完整路线图粗估。

### 源码与资源登记（8）

- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetReserveDispatchPanel.qml`
- `custom/res/Merivus/FleetMissionPlannerPanel.qml`
- `custom/res/Merivus/FleetMissionDeploymentPanel.qml`
- `custom/res/Merivus/FleetMissionRemainderPanel.qml`
- `custom/qgroundcontrol.qrc`
- `custom/res/Merivus/qmldir`
- `custom/res/Merivus/FlyViewMap.qml`

### 文档（18）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetReserveDispatchWorkflow.md`
- `custom/src/Swarm/FleetReserveDispatchPanel.md`
- `custom/src/Swarm/FleetMissionOperatorWorkflow.md`
- `custom/src/Swarm/FleetMissionDeployment.md`
- `custom/src/Swarm/FleetMissionRemainderPanel.md`
- `custom/src/Swarm/FleetReserveDispatchPlanner.md`
- `custom/src/Swarm/FleetMissionRemainderRoutePanel.md`

### 核对与未执行边界

仅直接静态阅读、普通文本／存在路径／Markdown本地目标／git空白检查。最终逐项核对见 [静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)；[56项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。没有编译、执行项目实现或解析器、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包；原25目标和六类实质剩余保持。下方为历史。

## 513 本批范围：10源码／工程登记＋18文档=28路径

独立备用出勤任务的原生后台及实际编译、编码、归档接线。不是自动接管、已有可见操作入口或完整飞行执行；下一阶段仍需Root真实来源捕获、人工新scope确认与现审阅／共享部署入口。

### 源码／工程登记（10）

- `custom/src/Swarm/FleetReserveDispatchPlanner.h`
- `custom/src/Swarm/FleetReserveDispatchPlanner.cc`
- `custom/src/Swarm/FleetMissionCompiler.cc`
- `custom/src/Swarm/FleetMissionPlanStore.cc`
- `custom/src/Swarm/FleetMissionReviewCoordinator.h`
- `custom/src/Swarm/FleetMissionReviewCoordinator.cc`
- `custom/custom.pri`
- `custom/src/Swarm/FleetMissionRemainderPlanner.cc`
- `custom/src/Swarm/FleetMissionGroupRemainderPlanner.cc`
- `custom/src/Swarm/FleetMissionPayloadBuilder.cc`

### 文档（18）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetReserveDispatchPlanner.md`
- `custom/src/Swarm/FleetMissionCompiler.md`
- `custom/src/Swarm/FleetMissionPlanStore.md`
- `custom/src/Swarm/FleetMissionReviewCoordinator.md`
- `custom/src/Swarm/FleetMissionRemainderPlanner.md`
- `custom/src/Swarm/FleetMissionGroupRemainderPlanner.md`
- `custom/src/Swarm/FleetMissionPayloadBuilder.md`

新增路径恰为custom/src/Swarm/FleetReserveDispatchPlanner.h／.cc／.md三条；其余均为本批前既有路径，不以git未跟踪数认定新文件。pri只登记两代码，不新增QML类型／UI资源、外部依赖、协议命令、账号或数据库schema。

Compiler前置validate＋matchesTask及输出前matchesPlan、PlanStore闭字段归档、PayloadBuilder normal来源门禁均核新schema1／17；四来源互斥，不丢旧来源择一。旧single／group清理已识别的新旧scope provenance，而非删共同安全政策。原主完整heading／hold、备用独立端点、新profile和全部floor进入新task／route摘要，严格性能布尔不能整体删去降级；未知成员政策不能被编译静默丢掉后匹配成功。原clearance不自动映射，最终许可仍未满足。

[新合同](../../custom/src/Swarm/FleetReserveDispatchPlanner.md)、[40项全部未执行工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未运行构建、解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现或测试，未仿真／操作硬件／PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持。下方为历史。


## 512 本批范围：4源码／接线＋13文档=17既有路径

联合余段任务的新备用保障：显式完整新reservePlans，从现面板与Root的新四键请求，进入同一GroupRemainderPlanner及现Compiler／能量审阅；不自动重绑旧ID、借旧端点、删除旧安全拒绝或恢复旧执行权限。

### 源码／实际接线（4）

- `custom/src/Swarm/FleetMissionGroupRemainderPlanner.h`
- `custom/src/Swarm/FleetMissionGroupRemainderPlanner.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetMissionGroupRemainderPanel.qml`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetMissionGroupRemainderPlanner.md`
- `custom/src/Swarm/FleetMissionGroupRemainderPanel.md`

全部为既有路径，没有新源码文件、模块、UI资源、依赖、命令、权限或数据库schema；不按未跟踪状态推新增文件数。Compiler／PlanStore／Coordinator／ReserveEnergyPlan／EnergyScheduler现实际消费者未改：由Planner新schema3在真实plan.reserveMembers.energyPlan闭合匹配，不添加它们不存在的plan.reservePlans字段。

旧request3／provenance1–2保留；新request4／provenance3保完整canonical descriptor及routeBoundaryPolicy。result7／summary18不扩，task／plan匹配分入口，不让task投影伪装actualcompiled预算。原政策hash表示改前，完整新task和routes绑定显式新备用运行配置；其余原最低数量／安全／能力／能量floor保留，旧clearanceInput不自动改机号或来源。

[规划器合同](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md)、[面板合同](../../custom/src/Swarm/FleetMissionGroupRemainderPanel.md)、[48项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码、直接静读与普通文本核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。原25目标／六类实质剩余及约40%／60%粗估保持，不将预算或编译值匹配当当前权、正式结果或StartPermit。下方为历史。

## 511 本批范围：2源码＋13文档=15既有路径

原生MODE queueBegan在实际发送前立即提交同一受保护TaskRecord。begin只有模型接纳成功且原对象／ReadContext／serial／queue与跟踪状态仍匹配时才true；失败不撤回已安装的事实。旧实际write后commit不以新bool失败回滚，Vehicle的原发送前binding／授权／immutable围栏不变。

### 源码（2）

- `custom/src/Swarm/FleetMissionExecutionObservation.h`
- `custom/src/Swarm/FleetMissionExecutionObservation.cc`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetMissionExecutionObservation.md`
- `custom/src/Swarm/SwarmMissionOrchestrator.md`

没有新增路径、资源、工程登记、依赖、命令、权限或公开schema。updateTaskModel仅private返回bool；其余原有caller可忽略返回，原清理及历史事实语义保留。完整Evidence仍会调用Link虚函数，不是无外调pure；Target引用不跨此更新，返回后先QQ及完整原上下文再重新定位目标。

[执行观察合同](../../custom/src/Swarm/FleetMissionExecutionObservation.md)、[原生事件链](../../custom/src/Swarm/SwarmMissionOrchestrator.md)、[28项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批不重复计算510的8源码／17文档，不将已有未跟踪文件当新增，也未导出补丁、安装包、提交或更新PR。

仅源码、直接静读与普通文本核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、硬件／PX4操作、刷机或放行。原25目标与六类实质剩余保持，约40%／60%仅源码工作量粗估；本检查点不是最终启动许可或正式任务结果。下方为历史。

## 510 本批范围：8源码／接线＋17文档=25路径

实际受保护TaskRecord的低频语义转换，经独立有界FIFO和临时原生收据进入现BlackBox，再以v3固定投影进入现持久历史／任务哈希下钻、两历史面板和报告消费者。没有新任务模型或自动控制，不把候选和关闭视为正式Completed／Failed。

### 源码／实际接线（8）

- `custom/src/Swarm/SwarmMissionOrchestrator.h`
- `custom/src/Swarm/SwarmMissionOrchestrator.cc`
- `custom/src/Swarm/FleetEventArchiveCodec.h`
- `custom/src/Swarm/FleetEventArchiveCodec.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetArchiveHistoryPanel.qml`
- `custom/res/Merivus/FleetArchiveTaskHistoryPanel.qml`
- `custom/src/Swarm/FleetMissionReportBuilder.cc`

### 文档（17）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/SwarmMissionOrchestrator.md`
- `custom/src/Swarm/FleetRunObservationEvent.md`
- `custom/src/Swarm/FleetRunObservationJournal.md`
- `custom/src/Swarm/FleetArchiveHistoryPanel.md`
- `custom/src/Swarm/FleetArchiveTaskHistoryPanel.md`
- `custom/src/Swarm/FleetMissionReportBuilder.md`

本批新路径只有FleetRunObservationEvent.md和FleetRunObservationJournal.md两份模块合同；其余路径此前已存在，不把未跟踪路径数当本批新文件数。没有新增代码文件、工程登记、资源、数据库schema、依赖、权限动作或MAVLink命令。旧v1／v2投影与旧查询合同保留，同库可混档。

FIFO最多512行／2MiB canonical payload、单行4096字节、每turn64条；这是序列化payload预算，不是实际堆内存硬上限。历史异常位图只在事件流中保曾观察事实。溢出保旧记录并追加全局capture-gap，不凭时间猜入某一任务。Root接纳计数不是SQL commit／durable；录制默认关闭，历史始终不授执行权。原生捕获UTC与BlackBox实际接纳UTC分开保留，不改写或偷偷排序。

静态收口修复报告的type／source精确非null字符串门，避免QVariant转换比较将非字符串解释为Run事件；正常reviewRequired=false转换仅为info，异常只供warning人工复核，raw仍保原事件。gap固定值必须带正累计丢弃计数，零不是一个已知缺口。

[事件合同](../../custom/src/Swarm/FleetRunObservationEvent.md)、[日志接线](../../custom/src/Swarm/FleetRunObservationJournal.md)、[60项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标与六类实质剩余保持，约40%完成／60%剩余只为源码工作量粗估；不能按本批路径或事件数上调。

仅源码、直接静态阅读及普通文本／路径／Markdown／git空白核对；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、硬件／PX4操作、刷机、放行、提交、更新PR或打包。下方为历史。

## 509 本批范围：14源码／接线＋16文档=30路径

人工单流启停与状态回读：独立 control action／typed双账号purpose、从实际Capture一次seal的opaque Basis、原30秒Review、原生单次2502／2503→512／270和独立Root／现有面板消费者。不是自动QoS、实际视频／带宽认证或飞行控制。

### 源码／实际接线（14）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetCameraStreamControl.h`
- `custom/src/Swarm/FleetCameraStreamControl.cc`
- `custom/src/Swarm/SwarmController.h`
- `custom/src/Swarm/SwarmController.cc`
- `custom/src/Swarm/FleetCameraStreamCapture.h`
- `custom/src/Swarm/FleetCameraStreamCapture.cc`
- `custom/src/Swarm/FleetOperatorSession.h`
- `custom/src/Swarm/FleetOperatorSession.cc`
- `custom/src/Swarm/FleetRolePolicy.cc`
- `custom/custom.pri`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetCameraStreamCapturePanel.qml`

### 文档（16）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRolePolicy.md`
- `custom/src/Swarm/FleetOperatorSession.md`
- `custom/src/Swarm/FleetCameraStreamCapture.md`
- `custom/src/Swarm/FleetCameraStreamControl.md`
- `custom/res/Merivus/FleetCameraStreamCapturePanel.md`

本批新路径仅FleetCameraStreamControl.h／.cc／.md共3个；其它路径此前已存在，不按未跟踪状态推新增数。沿现有Panel和双宿主，不增qrc／qmldir／host或外部依赖；新Control的h／cc在pri登记。此清单不是累计脏树统计、已导出patch／安装包或25项目标验收。

[控制合同](../../custom/src/Swarm/FleetCameraStreamControl.md)、[面板合同](../../custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[64项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持，约40%完成／60%剩余仍仅为源码工作量粗估。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 508 本批精确范围：14源码／接线＋14文档=28路径

人工单流 Info→Status READ：独立权限、实际服务／Root／原生 Run、两处可见宿主和精确取消；不把本地匹配报告当实际视频控制、带宽或飞行许可。

### 源码／实际接线（14）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetCameraStreamCapture.h`
- `custom/src/Swarm/FleetCameraStreamCapture.cc`
- `custom/src/Swarm/SwarmController.h`
- `custom/src/Swarm/SwarmController.cc`
- `custom/src/Swarm/FleetRolePolicy.cc`
- `custom/custom.pri`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetCameraStreamCapturePanel.qml`
- `custom/res/Merivus/CommandCenterOverlay.qml`
- `custom/res/Merivus/FleetMissionPlannerPanel.qml`
- `custom/qgroundcontrol.qrc`
- `custom/res/Merivus/qmldir`

### 文档（14）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRolePolicy.md`
- `custom/src/Swarm/FleetCameraStreamCapture.md`
- `custom/res/Merivus/FleetCameraStreamCapturePanel.md`

本批新路径仅 Service 的 h／cc／md 与 Panel 的 qml／md，共5个；其它路径此前已存在。新增 Service 的 pri、Panel 的 qrc／qmldir和双宿主均应与源码对应；无新增外部依赖。本清单是本批人工范围，不按累计脏树或未跟踪状态统计，不代表已导出 patch／安装包或25项目标完成。

[服务合同](../../custom/src/Swarm/FleetCameraStreamCapture.md)、[面板合同](../../custom/res/Merivus/FleetCameraStreamCapturePanel.md)、[48项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。原25目标和六类实质剩余保持；约40%完成／60%剩余仅为源码工作量粗估。

本阶段只修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 507 当前精确范围：10源码＋15文档=25路径

新增原PRIMARY相机VIDEO_STREAM_INFORMATION269／STATUS270被动账本，经Registry实际绑定一次读取、Policy第五参数和Guided原五秒周期接入既有指挥中心摘要；不发新请求或控制命令。另修上一轮留下的setVideoMode／setPhotoMode实际caller，以及其toggle／reset模式意图衔接。

### 源码／实际接线（10）

- `src/Camera/QGCCameraControl.h`
- `src/Camera/QGCCameraControl.cc`
- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetRegistry.h`
- `custom/src/Swarm/FleetRegistry.cc`
- `custom/src/Swarm/FleetLinkDegradationPolicy.h`
- `custom/src/Swarm/FleetLinkDegradationPolicy.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/CommandCenterOverlay.qml`

### 文档（15）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetCameraStreamObservation.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`
- `docs/handoff/MERIVUS_VIDEO_LIFECYCLE.md`
- `custom/src/Swarm/FleetRegistry.md`

新文件仅custom/src/Swarm/FleetCameraStreamObservation.md；其余均为本批开始前已存在路径。清单不按累计脏树或未跟踪状态推本批新增量，不代表已导出patch／安装包或25项产品目标已经完成。

[完整合同](../../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 506 当前精确范围：7源码＋12文档=19路径

本轮修改现有CameraManager／CameraControl→VideoManager真实调用链：本地播放意图、原相机／流选择代次、有限接收器恢复、活动飞机切换和通知回调后的对象存活检查；共享QmlObjectListModel补通知后的自身存活尾核与匹配reset。不是新增只读helper，也不是完整Fleet视频降级／受控停流事务。

### 源码（7）

- `src/Camera/QGCCameraControl.h`
- `src/Camera/QGCCameraControl.cc`
- `src/Camera/QGCCameraManager.h`
- `src/Camera/QGCCameraManager.cc`
- `src/VideoManager/VideoManager.h`
- `src/VideoManager/VideoManager.cc`
- `src/QmlControls/QmlObjectListModel.cc`

### 文档（12）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `docs/handoff/MERIVUS_VIDEO_LIFECYCLE.md`

本批新文件仅MERIVUS_VIDEO_LIFECYCLE.md；其余路径原已存在，未跟踪状态不能当本批新增数。没有新UI、qrc／qmldir／工程资源项或外部依赖，原源码登记由既有工程包含。

[合同](MERIVUS_VIDEO_LIFECYCLE.md)、[50项全部未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本清单不是累计diff统计、已生成patch文件、安装包或编译验收。原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径、接口或局部保护数量上调。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 505 当前精确范围：2源码＋12文档=14路径

本轮修现有LogDownloadPage→LogDownloadController→LOG_REQUEST_LIST／DATA→本地文件的真实消费者：同一目录／批次来源、冻结选择、有界缺块补读、完整写入后位图与去重计数、NewOnly partial及完整刷新／改名后才显示Downloaded。不是另建只读建议helper，也不是完整Fleet控制中心补传。

### 源码（2）

- `src/AnalyzeView/LogDownloadController.h`
- `src/AnalyzeView/LogDownloadController.cc`

### 文档（12）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `docs/handoff/MERIVUS_LOG_DOWNLOAD_RECOVERY.md`

新增文件仅MERIVUS_LOG_DOWNLOAD_RECOVERY.md；没有新增UI、qrc／pri／qmldir登记、权限或外部接收端。范围不是累计脏树统计或已生成patch／安装包。

[完整合同](MERIVUS_LOG_DOWNLOAD_RECOVERY.md)、[40项全未执行](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 504 当前精确范围：16源码／接线＋15文档=31路径

504新增独立人工单次定向TIMESYNC往返耗时采集：实际主账号／Role／Root原意图→Swarm私有服务／原生Run→真实同车绑定review→Policy及可见面板。最早四秒包含授权核验和后续100ms轮询，不自动采集、重试或续期，不回退PING或广播。

### 源码／实际接线（16）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetLinkLatencyCapture.h`
- `custom/src/Swarm/FleetLinkLatencyCapture.cc`
- `custom/src/Swarm/SwarmController.h`
- `custom/src/Swarm/SwarmController.cc`
- `custom/src/Swarm/FleetRolePolicy.cc`
- `custom/custom.pri`
- `custom/src/Swarm/FleetLinkDegradationPolicy.h`
- `custom/src/Swarm/FleetLinkDegradationPolicy.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetLinkLatencyPanel.qml`
- `custom/res/Merivus/CommandCenterOverlay.qml`
- `custom/res/Merivus/FleetMissionPlannerPanel.qml`
- `custom/res/Merivus/qmldir`
- `custom/qgroundcontrol.qrc`

### 文档（15）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRolePolicy.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`
- `custom/src/Swarm/FleetLocalReceptionObservation.md`
- `custom/src/Swarm/FleetLinkLatencyCapture.md`

新文件仅Service.h／.cc／.md与Panel.qml四条。pri的Service头／源各登记一次，qrc／qmldir的Panel各一次，两实际宿主各使用一次；无CustomPlugin构造注册、Session批准purpose、PX4或外部依赖变更。旧接收native22／window9／Registry wrapper7保持；新RTT26、Service18/result5、独立wrapper7、Policy23/localLatency10不能当可执行凭据。

这是人工登记的本批范围，不是累计dirty tree、已生成patch或安装包。[44项交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 503 当前精确范围：9源码＋14文档=23路径

503将实际PRIMARY／本车默认component的原始解码帧接到独立五秒接收窗，经Registry实际绑定读取、Policy独立消费、Guided原五秒周期和指挥中心只读摘要。记录接收帧数、编码MAVLink字节率与最大内部间隔；不把它们当可用带宽、RTT、丢包率或远端发布率。

### 源码／实际消费接线（9）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetRegistry.h`
- `custom/src/Swarm/FleetRegistry.cc`
- `custom/src/Swarm/SwarmController.cc`
- `custom/src/Swarm/FleetLinkDegradationPolicy.h`
- `custom/src/Swarm/FleetLinkDegradationPolicy.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/CommandCenterOverlay.qml`

### 文档（14）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRegistry.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`
- `custom/src/Swarm/FleetLocalReceptionObservation.md`

新文件仅FleetLocalReceptionObservation.md一条。没有新增C++／QML类型、pri／qrc资源、依赖或权限；native22／window9、wrapper7、新Policy22（原20＋2）、localReception10为本批固定值形。旧evaluate20及500–502可选遥测15／21／6、授权／九阶段／30秒合同不变。

这是人工登记的本批范围，不是累计dirty tree或已生成patch／安装包。[40项交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 502 当前精确范围：6源码／UI资源接线＋18文档=24路径

502新增FleetOptionalTelemetryPanel可见界面，在指挥中心和地图规划页复用同一actual Root。操作员显式只读采集→另行审阅统一目标→两项确认后独立账号审批→逐项配置与回读；固定62／74／83，原501实际后端和授权合同不变。不会自动采集、Apply、续期、重试或回滚。

### 源码／可见UI与资源登记（6）

- `custom/res/Merivus/FleetOptionalTelemetryPanel.qml`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/CommandCenterOverlay.qml`
- `custom/res/Merivus/FleetMissionPlannerPanel.qml`
- `custom/res/Merivus/qmldir`
- `custom/qgroundcontrol.qrc`

### 文档（18）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRegistry.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`
- `custom/src/Swarm/FleetRolePolicy.md`
- `custom/src/Swarm/FleetOptionalTelemetryCapture.md`
- `custom/src/Swarm/FleetOperatorSession.md`
- `custom/src/Swarm/FleetOptionalTelemetryConfiguration.md`
- `custom/res/Merivus/FleetOptionalTelemetryPanel.md`

新文件仅Panel.qml／.md两条。Root两projection四return补uiWiringPresent及Configuration operatorIdentity，另两request／source围栏接本次调用的可选cancelState，仅sticky撤权，旧签名调用保持，runtimeValidated=false；原生发送／Session／Role／Registry／Policy与native输出合同不变。无新依赖、任意QML构造服务或PX4变更。两个宿主沿现有controller与Layout接线，qrc和qmldir各登记一次。

此表是人工登记的本批范围，不是累计dirty tree或已生成patch／安装包。[44项交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501 当前精确范围：13源码／接线＋17文档=30路径

501新增独立双账号的地面可选遥测配置：从未来新成功Capture的三条真实native上下文封存不可序列化Basis，在同一最早30秒内独立审阅／批准，再逐项511→510并最终三项510复核。仅固定62／74／83、同一正int32且精确REAL32间隔，目标不得比任一原正基线更短；不启停流、不降频P0、不改视频或编队速度。

### 源码／后台桥接与工程登记（13）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetOptionalTelemetryCapture.h`
- `custom/src/Swarm/FleetOptionalTelemetryCapture.cc`
- `custom/src/Swarm/FleetOptionalTelemetryConfiguration.h`
- `custom/src/Swarm/FleetOptionalTelemetryConfiguration.cc`
- `custom/src/Swarm/FleetOperatorSession.h`
- `custom/src/Swarm/FleetOperatorSession.cc`
- `custom/src/Swarm/FleetRolePolicy.cc`
- `custom/src/Swarm/SwarmController.h`
- `custom/src/Swarm/SwarmController.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/custom.pri`

### 文档（17）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRegistry.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`
- `custom/src/Swarm/FleetRolePolicy.md`
- `custom/src/Swarm/FleetOptionalTelemetryCapture.md`
- `custom/src/Swarm/FleetOperatorSession.md`
- `custom/src/Swarm/FleetOptionalTelemetryConfiguration.md`

新文件仅Configuration.h／.cc／.md三条。Capture仍是500引入的原私有对象，本批补opaque移交；Session和Role新增专用配置审批。FleetRegistry／FleetLinkDegradationPolicy源码、15／21／6旧合同、P0证据与飞控不改；无新qrc、依赖或可任意QML构造类型。

start仅认领ready、不写包；100ms后推进九个wire阶段。每条至多4秒，510 accepted ACK后至多1秒且只缩，所有阶段与审批共用原Capture最早30秒。慢链路可能无法完成；截止／失败／取消保留已发事实，不重试、不自动回滚。三项最终回读只给reported-values-match，不认证无线因果、实际发布率、带宽或飞行许可。

此表是本批人工登记的范围，不是累计dirty tree、git patch或安装包；[46项交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 500 当前精确范围：9源码／接线＋15文档=24路径

本批新增显式只读采集，不配置／启停流。新文件仅Capture.h／.cc／.md；源码中的custom.pri为工程登记，Guided仅后台意图／值投影，无新可见按钮或qrc。FleetRegistry／FleetLinkDegradationPolicy源码不改。

### 源码／后台桥接与工程登记（9）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetOptionalTelemetryCapture.h`
- `custom/src/Swarm/FleetOptionalTelemetryCapture.cc`
- `custom/src/Swarm/FleetRolePolicy.cc`
- `custom/src/Swarm/SwarmController.h`
- `custom/src/Swarm/SwarmController.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/custom.pri`

### 文档（15）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRegistry.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`
- `custom/src/Swarm/FleetRolePolicy.md`
- `custom/src/Swarm/FleetOptionalTelemetryCapture.md`

新service只由原Swarm私有构造持有，不新增可任意QML构造的注册类型。此表是本批静态修改范围，不是累计脏树、已生成patch／安装包或产品完成数量。[40项交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行。

本批只实现地面站单车固定62／74／83的显式只读间隔采集，不发送511配置，不启用／停用流，不更改P0证据或控制协议。实际固件若拒绝510或未回244就失败，不自动改用未证实的512索引查询；完整目标14仍需独立配置权限／必要二审、逐步Apply与实际回读和效果验证。原25目标和六类实质剩余保持。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 499 当前精确范围：5源码＋13文档=18路径

只修GroundStation现有消息查询／配置链、旧512的实际等待／终结与两个调用者的寿命／发布归属；不是新增Fleet主动采集或配置器。没有新文件、QObject类型注册、依赖、构建／资源登记或界面，不修改飞控。

### 源码（5）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `src/Vehicle/InitialConnectStateMachine.cc`
- `src/Vehicle/StandardModes.h`
- `src/Vehicle/StandardModes.cc`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRegistry.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`

Registry、Policy、Guided及协议头只读未改，原15／21／6合同保持；proto510事实来自地面站随包头，不读取PX4仓库。原文件已在既有工程中，无新增登记。此表是本批范围，不是累计Git脏树、产品计数或已生成patch／安装包。

本批修Vehicle现有消息速率查询／配置发送链和通用512等待生命周期；尚不新增Fleet主动capture服务、独立actual Session／角色scope或二审purpose、Apply UI／逐步配置结果。目标14及原25项目标六类实质剩余继续，不能将本地lane、generation或报告值当无线因果、实际生效／发布率／带宽或飞行许可。

[42项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 498 当前精确范围：5源码＋13文档=18路径

本批补目标14真正实际绑定的被动资料与只读数值消费者，不实现配置器。均为既有文件；无新文件、QObject类型、依赖、构建／资源登记、配置UI或权限。仍只优化地面站，不读取／修改PX4飞控。

### 源码（5）

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/src/Swarm/FleetRegistry.h`
- `custom/src/Swarm/FleetRegistry.cc`
- `custom/res/Merivus/GuidedActionsController.qml`

### 文档（13）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetRegistry.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`

原custom/qgroundcontrol.qrc已指实际Guided，custom/custom.pri已登记FleetRegistry；本批不改登记或src默认Guided副本。清单不是累计Git脏树、产品完成数量或已打包补丁。

本批为目标14补真实被动资料与只读数值消费者，不发送512查询或511配置，也不授予配置／飞行权限。真正的主动基线采集、同源同期限回读、独立实际Session／角色scope与必要二审、511／512共享排他及变更代次、逐步apply和部分已发结果仍待开发；不能借旧refresh／start／capability凭据，不能降频P0证据或控制协议。

[36项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。


## 497 当前精确范围：6源码＋12文档=18路径

本批只修GroundStation现有PID／ESC消息频率请求的本地结果归属、回调重入与公共511来源/超时前置；没有新增Fleet降频执行器。无新文件、QObject类型注册、构建／资源登记、UI或依赖。GroundStation里的PX4FirmwarePlugin是地面站适配器，不是PX4飞控仓库。

### 源码（6）

- `src/Vehicle/MAVLinkStreamConfig.h`
- `src/Vehicle/MAVLinkStreamConfig.cc`
- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `src/FirmwarePlugin/PX4/PX4FirmwarePlugin.h`
- `src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc`

### 文档（12）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetLinkDegradationPolicy.md`

PID与ESC各自一次handler/context消费自己的本地请求，删除原任意live511推进PID／ESC的分支。STATUS5Hz→INFO1Hz、原PID两消息100Hz及手动默认恢复入口保持；拒绝／超时不自动续链。same-source迟到重复wire ACK仍可能歧义：协议不回显messageId，本地ID／UUID／来源核对不是wire因果认证，accepted也不证明实际频率、带宽或安全效果。

[36项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行；此范围不是累计Git脏改动或产品完成计数。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 496 当前精确范围：4源码＋15文档=19路径

本批只改Contact与Waypoint两组源／头；无新文件、QObject、UI、依赖或构建／资源登记。Worker、Joint、Domain、Readiness源码只读未改，公开report31/schema1／result3／Worker7保持。此清单不是累计Git脏改动或整目标完成数量。

### 源码（4）

- `custom/src/Swarm/FleetMissionContactModel.h`
- `custom/src/Swarm/FleetMissionContactModel.cc`
- `custom/src/Swarm/FleetMissionWaypointSwitchReview.h`
- `custom/src/Swarm/FleetMissionWaypointSwitchReview.cc`

### 文档（15）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetMissionContactModel.md`
- `custom/src/Swarm/FleetMissionWaypointSwitchReview.md`
- `custom/src/Swarm/FleetMissionStartupClearanceWorker.md`
- `custom/src/Swarm/FleetMissionJointSpatialReview.md`

实质增量是同次TakeoffReference与完整current/top两端编码盒的垂直参考体；schema2原窗口完整约束和有限pad交集判据保留，v5退休v4。旧名义components、LAND、NAV计数与所有认证／许可false不改。[36项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。

原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 495 当前精确范围：2源码＋14文档=16路径

仅Waypoint源／头改变实质几何；无新文件、QObject、UI、依赖、工程／资源登记。Worker、Joint、Contact及Readiness源码只读未改，原输入输出合同保持。

### 源码（2）

- `custom/src/Swarm/FleetMissionWaypointSwitchReview.h`
- `custom/src/Swarm/FleetMissionWaypointSwitchReview.cc`

### 文档（14）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetMissionWaypointSwitchReview.md`
- `custom/src/Swarm/FleetMissionStartupClearanceWorker.md`
- `custom/src/Swarm/FleetMissionJointSpatialReview.md`

新增起飞完整编码参考top到首NAV全帽连接，旧NAV／LAND域和NAV计数保持；ModelName v4退休旧v3，不新增孤立flag或Worker字段。此范围非累计Git脏改动，不能当产品完成数。[36项交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 494 当前精确范围：4源码／UI＋14文档=18路径

均为既有路径；两新invokable在现报告类，不新增QObject／依赖／源码登记／QRC别名或PX4改动。实际QML已有custom资源映射，未改src默认副本。

### 源码／UI（4）

- `custom/src/Swarm/FleetMissionExecutionReport.h`
- `custom/src/Swarm/FleetMissionExecutionReport.cc`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/res/Merivus/FleetMissionExecutionReportPanel.qml`

### 文档（14）

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetMissionExecutionReport.md`
- `custom/src/Swarm/FleetMissionExecutionReportPanel.md`
- `custom/src/Swarm/FleetProtectedRunReport.md`

功能为独立保护报告原文本保存／完整重开与明确历史UI，旧full9／handoff合同保持。范围不是累计Git脏改动统计，不是目标完成计数。[36工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原25项目标与六类实质剩余保持，约40%完成／60%剩余仍只是源码工作量粗估，不按本批路径、接口或报告数量上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍待继续。

本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 493 当前准确范围：6源码／UI、14文档，共20路径

全部为既有路径，无新文件、QObject、工程／资源登记、依赖或PX4修改；custom qrc已指向下列实际QML，未改src默认副本。

| 源码／UI（6） | 当前修改 |
| --- | --- |
| `custom/src/Swarm/FleetTelemetryArchiveHistoryReader.h` | 独立manifest／search请求／结果三static合同 |
| `custom/src/Swarm/FleetTelemetryArchiveHistoryReader.cc` | search8／result34／row10，实际逐file校验、分批与130MiB预留 |
| `custom/src/Swarm/FleetTelemetryArchive.h` | 独立historySearch／closed search8及私有同hash首页入口 |
| `custom/src/Swarm/FleetTelemetryArchive.cc` | 原catalog／完整manifest来源、native cursor、public36和双终结失效 |
| `custom/res/Merivus/GuidedActionsController.qml` | 只读search值与独立8字段窄facade，旧6字段口不变 |
| `custom/res/Merivus/FleetTelemetryHistoryPanel.qml` | 显式筛选／首批续批／选命中／同hash首页及代次防旧意图 |

文档准确14路径：

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetTelemetryArchive.md`
- `custom/src/Swarm/FleetTelemetryArchiveHistoryReader.md`
- `custom/src/Swarm/FleetTelemetryHistoryPanel.md`

[36项未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)及[审计](MERIVUS_SWARM_STATIC_AUDIT.md)同步。旧catalog／page与原位置同意、运动报告不被搜索替换；本项不是持久全集索引、任务Run关联或实际运营／飞行验收。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 492 当前准确范围：7源码、15文档，共22路径

均现有路径，无新文件、QObject、UI、工程／资源登记、依赖或PX4改动。仅执行观察lane，不把其它telemetry／sensor来源链自动算作同样已修。

| 源码（7） | 本批内容 |
| --- | --- |
| `src/Vehicle/Vehicle.h` | 一次pin的原来源QQ、私有slot代次及probe状态 |
| `src/Vehicle/Vehicle.cc` | 构造期原来源pin及三live检查、bind／BindingMatches／RAW／recheck／queue尾核 |
| `custom/src/Swarm/FleetMissionExecutionObservation.h` | 一次pin的原app／toolbox／MVM、私有Run变更代次及快照辅助 |
| `custom/src/Swarm/FleetMissionExecutionObservation.cc` | owned目标／组快照、回调后原行重定位、生命周期与历史commit |
| `custom/src/Swarm/FleetMissionStartupTransaction.cc` | observer getter冻结原对象／command context及关闭上下文外调尾核 |
| `custom/src/Swarm/SwarmMissionOrchestrator.h` | private完整TaskRecord比较声明 |
| `custom/src/Swarm/SwarmMissionOrchestrator.cc` | 全24字段比较、四Evidence调用者QQ／重找及stage连接后原证据复核 |

| 文档（15） | 用途 |
| --- | --- |
| `README.md` | 当前入口 |
| `MODIFIED_FILES.md` | 准确范围 |
| `docs/PROJECT_STATUS.md` | 当前状态与原剩余 |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` | P0优先与动态域／运营后续 |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` | 原目标边界 |
| `docs/INDEX.md` | 文档索引 |
| `docs/handoff/README.md` | 接手入口 |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` | 36项全未执行场景 |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` | 本清单 |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` | 直接静读／普通文本证据 |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` | 原来源／commit／许可边界 |
| `src/Vehicle/VehicleMissionExecutionObserver.md` | Vehicle执行观察lane |
| `custom/src/Swarm/FleetMissionExecutionObservation.md` | Run快照及生命周期 |
| `custom/src/Swarm/FleetMissionStartupTransaction.md` | 同observer返回尾栅 |
| `custom/src/Swarm/SwarmMissionOrchestrator.md` | 原完整TaskRecord重定位 |

公开观察／report／queue JSON及原动态／认证／完成false不扩形；私有代次不是证书、远端token或审批续期。见[未执行交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 491 当前准确范围：4源码、15文档，共19路径

均为现有文件，无新文件／QObject／UI、工程／资源登记、依赖或PX4改动；新增LandingReference是ContactModel内部原生值结构，不是新消息或JSON凭证。

| 源码（4） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetMissionContactModel.h` | LandingReference及Result原生向量 |
| `custom/src/Swarm/FleetMissionContactModel.cc` | 从同次已应用LAND support产出原参考／有限pad |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.h` | 原NAV＋LAND声明范围及非认证边界 |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.cc` | v3参数／参考绑定、入口和完整XY下降体、五类环境消费、有限接地检查与matcher |

| 文档（15） | 用途 |
| --- | --- |
| `README.md` | 当前入口 |
| `MODIFIED_FILES.md` | 准确范围 |
| `docs/PROJECT_STATUS.md` | 当前进度和真实剩余 |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` | LAND分支与下一动态域 |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` | 目标6／12边界 |
| `docs/INDEX.md` | 模块索引 |
| `docs/handoff/README.md` | 工程接手入口 |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` | 36项未执行场景 |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` | 本清单 |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` | 静读与普通文本核对 |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` | 原来源与豁免／许可边界 |
| `custom/src/Swarm/FleetMissionContactModel.md` | 原生参考生产者 |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.md` | 完整参考体算法及消费 |
| `custom/src/Swarm/FleetMissionStartupClearanceWorker.md` | 原结果／准入链实际接入 |
| `custom/src/Swarm/FleetMissionJointSpatialReview.md` | 原名义范围不冒充接受域 |

Joint、Worker、两Review／Authority及Witness源码未改；它们既有调用和严格resultMatches接新Waypoint模型，不新增独立授权器或报告槽。原Contact components仍segments×2、序号／载荷绑定及JSON结构保持，原预算不增额。见[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 490 当前准确范围：8源码、16文档，共24路径

仅修改下列现有路径，无新文件、C++类、QObject、UI、工程／资源登记、依赖或PX4改动。仓库累计未跟踪／修改状态不能当本轮diff统计。

| 源码（8） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetMissionClearanceGeometry.h` | coveredByIndexedPrisms声明，旧API保持 |
| `custom/src/Swarm/FleetMissionClearanceGeometry.cc` | 原完整query校验、原凸柱候选、闭面重建及旧union |
| `custom/src/Swarm/FleetMissionStartupDomainReview.cc` | 三类覆盖索引准备及原完整域消费者 |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.cc` | 三类覆盖索引准备及原完整NAV凸包消费者 |
| `src/Vehicle/FleetStartupMemorySeal.h` | capture必填实际PM／原弱owner／原29快照 |
| `src/Vehicle/FleetStartupMemorySeal.cc` | 独立持有原6键／29行，current末尾PM29内存核对 |
| `custom/src/Swarm/FleetMissionStartupTransaction.h` | Target持有首次PRIMARY弱控制块 |
| `custom/src/Swarm/FleetMissionStartupTransaction.cc` | 初始原29核对及唯一capture调用接齐 |

| 文档（16） | 用途 |
| --- | --- |
| `README.md` | 当前批次入口 |
| `MODIFIED_FILES.md` | 当前准确范围 |
| `docs/PROJECT_STATUS.md` | 当前状态与未完成项 |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` | 原25目标及下一真实动作域 |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` | 目标6／12的增量边界 |
| `docs/INDEX.md` | 模块与交接索引 |
| `docs/handoff/README.md` | 工程接手入口 |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` | 36项全未执行场景 |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` | 本清单 |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` | 直接静读及普通文本核对范围 |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` | 原源／receipt／许可边界 |
| `custom/src/Swarm/FleetMissionClearanceGeometry.md` | 索引覆盖API合同 |
| `custom/src/Swarm/FleetMissionStartupDomainReview.md` | 整域覆盖与terrain分离 |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.md` | 完整NAV查询面与预算 |
| `src/Vehicle/FleetStartupMemorySeal.md` | 原参数快照及最终尾栅 |
| `custom/src/Swarm/FleetMissionStartupTransaction.md` | 原控制块与原29贯穿stage |

[36项工程交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。原数学模型／报告schema、原事务／阶段期限、接触判断和许可边界保持；参数快照schemaVersion=1且6键，不混称schemaVersion=6。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 489 当前准确范围：8源码／UI、15文档，共23路径

仅新增 `custom/src/Swarm/FleetProtectedRunReport.md` 一份模块说明；没有新C++／QML文件、QObject、工程／资源登记、依赖或PX4改动，不将累计脏树当本批diff。

| 源码／UI（8） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/SwarmMissionOrchestrator.h` | 私有轻目录／record捕获与本实例sourceId |
| `custom/src/Swarm/SwarmMissionOrchestrator.cc` | 元数据token、原private保护记录单条读取 |
| `custom/src/Swarm/SwarmController.h` | 只读catalog和独立capture窄调用 |
| `custom/src/Swarm/SwarmController.cc` | once原生源、双完整尾核、native jsonText和NOTIFY |
| `custom/src/Swarm/FleetMissionExecutionReport.h` | 新独立static入口，旧API保持 |
| `custom/src/Swarm/FleetMissionExecutionReport.cc` | 原compact完整校验、独立report8／summary13 |
| `custom/res/Merivus/GuidedActionsController.qml` | 值目录和四字段选择转发，不进旧报告槽 |
| `custom/res/Merivus/FleetMissionExecutionReportPanel.qml` | 明确选择／确认、单份捕获／复制及通知失效保护 |

| 文档（15，含新模块1） |
| --- |
| `README.md` |
| `MODIFIED_FILES.md` |
| `docs/PROJECT_STATUS.md` |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` |
| `docs/INDEX.md` |
| `docs/handoff/README.md` |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` |
| `custom/src/Swarm/SwarmMissionOrchestrator.md` |
| `custom/src/Swarm/FleetMissionExecutionReport.md` |
| `custom/src/Swarm/FleetMissionExecutionReportPanel.md` |
| `custom/src/Swarm/FleetProtectedRunReport.md`（新） |

[36项工程场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行，源码／文档静态核对不等于可安装、运行或放行。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 488 当前准确范围：2源码、13文档，共15路径

无新文件、头／源登记、资源、依赖、UI、observer或协议；只统计本批实际修改，不统计原有累计脏树。不创建包、提交或更新PR。

| 源码（2） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetMissionExecutionObservation.cc` | 正式MISSION_CURRENT回退后保原报告／旧历史并结束单机关联 |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.cc` | v2整数XY凸包棱柱、正确外扩、完整覆盖及粗筛后凸域接触 |

| 文档（13） |
| --- |
| `README.md` |
| `MODIFIED_FILES.md` |
| `docs/PROJECT_STATUS.md` |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` |
| `docs/INDEX.md` |
| `docs/handoff/README.md` |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` |
| `custom/src/Swarm/FleetMissionExecutionObservation.md` |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.md` |

[32项工程验证](MERIVUS_SWARM_ENGINEER_HANDOFF.md)全部未执行。旧公开shape及原预算不扩大，静态模型不当真实轨迹或执行许可；实际结果／动态净空及原25目标／六类实质剩余保持。本批仅地面站源码／文档与静态核对，未编译、运行模型／helper、测试、仿真、改PX4、硬件／刷机／放行、提交、更新PR或打包。下方为历史。

## 487 当前准确范围：6源码／UI、15文档，共21路径

仅新增观察卷宗模块说明1文件，无新C++／QML文件、工程／资源登记、依赖、observer、协议或PX4修改。只统计本批，不统计累计脏树。

| 源码／UI（6） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetMissionReportBuilder.h` | 独立static历史卷宗，不改旧任务报告API |
| `custom/src/Swarm/FleetMissionReportBuilder.cc` | 独立static历史卷宗，不改旧任务报告API |
| `custom/src/Swarm/FleetTelemetryArchive.h` | 同一原页一次生成、双缓存共同当前性与完整校验 |
| `custom/src/Swarm/FleetTelemetryArchive.cc` | 同一原页一次生成、双缓存共同当前性与完整校验 |
| `custom/res/Merivus/GuidedActionsController.qml` | 仅新增readonly motionDossier |
| `custom/res/Merivus/FleetTelemetryHistoryPanel.qml` | 同一快照采用／四视角／资料发现与可选择复制文本 |

| 文档（15，含新模块1） |
| --- |
| `README.md` |
| `MODIFIED_FILES.md` |
| `docs/PROJECT_STATUS.md` |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` |
| `docs/INDEX.md` |
| `docs/handoff/README.md` |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` |
| `custom/src/Swarm/FleetMissionReportBuilder.md` |
| `custom/src/Swarm/FleetTelemetryArchive.md` |
| `custom/src/Swarm/FleetTelemetryHistoryPanel.md` |
| `custom/src/Swarm/FleetTelemetryObservationReport.md` |

旧14文档新增顶部487块并保历史，新模块说明独立成文。34项工程场景全部未执行；独立交叉静读、15文档终审与最终文本统计已记本批审计，不是构建或运行验收。原25目标及六类实质剩余保持；整体约40%完成／60%剩余仍仅源码工作量粗估，不按本批路径或视图数上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。

本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486 当前准确范围：11源码／UI／登记、14文档，共25路径

新增MotionReview.h/.cc、TelemetryHistoryPanel.qml及两模块md共5文件；不改Codec／Reader磁盘格式，不新增observer／数据流／依赖或PX4代码。custom.pri新源和头各一次，qrc／qmldir新组件各一次。只统计本批，不统计累计脏树。

| 源码／UI／登记（11） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetTelemetryArchive.h` | 固定本地动作／原实际完成页消费／报告来源 |
| `custom/src/Swarm/FleetTelemetryArchive.cc` | 固定本地动作／原实际完成页消费／报告来源 |
| `custom/src/Swarm/FleetTelemetryMotionReview.h` | 新纯单页分段报告值审阅 |
| `custom/src/Swarm/FleetTelemetryMotionReview.cc` | 新纯单页分段报告值审阅 |
| `custom/res/Merivus/GuidedActionsController.qml` | 窄只读facade |
| `custom/res/Merivus/FleetTelemetryHistoryPanel.qml` | 新只读面板与两次位置确认 |
| `custom/res/Merivus/CommandCenterOverlay.qml` | 实际历史入口宿主 |
| `custom/res/Merivus/FleetMissionPlannerPanel.qml` | 实际历史入口宿主 |
| `custom/custom.pri` | 新C++／QML资源登记 |
| `custom/qgroundcontrol.qrc` | 新C++／QML资源登记 |
| `custom/res/Merivus/qmldir` | 新C++／QML资源登记 |

| 文档（14，含新模块2） |
| --- |
| `README.md` |
| `MODIFIED_FILES.md` |
| `docs/PROJECT_STATUS.md` |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` |
| `docs/INDEX.md` |
| `docs/handoff/README.md` |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` |
| `custom/src/Swarm/FleetTelemetryArchive.md` |
| `custom/src/Swarm/FleetTelemetryMotionReview.md` |
| `custom/src/Swarm/FleetTelemetryHistoryPanel.md` |

旧12文档新增顶部486块保留历史，两新模块说明独立成文。32工程场景全部未执行；独立交叉静读、14文档终审及最终文本统计已记本批审计，不是构建或运行验收。原25目标及六类实质剩余保持；整体约40%／剩余60%仍仅源码工作量粗估，不按本批文件或入口上调。完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态仍要继续。

仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。 下方为历史。

## 485 当前准确范围：9源码／UI／资源、19文档，共28路径

只统计本轮，不统计累计脏树。新增FleetPerformancePolicyEditor.qml和模块md两文件；qgroundcontrol.qrc／qmldir各登记一次，无新依赖、C++类型、PX4、MAVLink或命令权限。未声明源保持原1..4，存在true／false则source5；外层来源exact5不扩。

| 源码／UI／资源（9） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetMissionDraft.h` | optional Bool set/clear和schema5合同 |
| `custom/src/Swarm/FleetMissionDraft.cc` | 规范setting／source/load/build真实全链 |
| `custom/src/Swarm/FleetMissionCompiler.cc` | source重建计数／全键匹配及area false保留 |
| `custom/src/Swarm/FleetMissionPlanStore.cc` | 源task与plan的presence＋actual Bool一致性 |
| `custom/res/Merivus/GuidedActionsController.qml` | 窄facade原源CAS与集中source版本门 |
| `custom/res/Merivus/FleetMissionPlannerPanel.qml` | 新组件挂载／pending与各编辑器互斥 |
| `custom/res/Merivus/FleetPerformancePolicyEditor.qml` | 新三态政策编辑／原owner修订确认及取消 |
| `custom/qgroundcontrol.qrc` | 新组件资源alias登记一次 |
| `custom/res/Merivus/qmldir` | 新组件模块声明登记一次 |

| 文档（19，含新模块说明1） | 本批内容 |
| --- | --- |
| `README.md` | 485当前合同／交接／索引或审计 |
| `MODIFIED_FILES.md` | 485当前合同／交接／索引或审计 |
| `docs/PROJECT_STATUS.md` | 485当前合同／交接／索引或审计 |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` | 485当前合同／交接／索引或审计 |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` | 485当前合同／交接／索引或审计 |
| `docs/INDEX.md` | 485当前合同／交接／索引或审计 |
| `docs/handoff/README.md` | 485当前合同／交接／索引或审计 |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` | 485当前合同／交接／索引或审计 |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` | 485当前合同／交接／索引或审计 |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` | 485当前合同／交接／索引或审计 |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` | 485当前合同／交接／索引或审计 |
| `custom/src/Swarm/FleetMissionDraft.md` | 485当前合同／交接／索引或审计 |
| `custom/src/Swarm/FleetMissionCompiler.md` | 485当前合同／交接／索引或审计 |
| `custom/src/Swarm/FleetMissionPlanStore.md` | 485当前合同／交接／索引或审计 |
| `custom/src/Swarm/FleetPerformancePolicyEditor.md` | 485当前合同／交接／索引或审计 |
| `custom/src/Swarm/FleetMissionDraftStore.md` | 485当前合同／交接／索引或审计 |
| `custom/src/Swarm/FleetMissionRepairDraftPanel.md` | 485当前合同／交接／索引或审计 |
| `custom/src/Swarm/FleetMissionArchiveDraftPanel.md` | 485当前合同／交接／索引或审计 |
| `custom/src/Swarm/FleetMissionSavedDraftPanel.md` | 485当前合同／交接／索引或审计 |

旧18文档仅新增顶部485段，历史保留；新模块说明单独成文。32工程场景全部未执行，本批审计已记录三路独立交叉静读、19文档终审及最终普通文本统计，不是构建或运行验收。原25目标及六类实质剩余保持：完整执行许可／动态轨迹与正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按28路径或新入口上调。

本轮仅静态源码／文档；未编译、执行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包，无本批安装包。 下方为历史。

## 484 当前准确范围：12源码／UI、19文档，共31路径

仅统计本轮，不统计既有累计脏树。全部为已有文件，无新文件、依赖、工程登记、资源或QML类型；不改PX4／MAVLink／飞行权限。旧四操作／preview2/18／current30／result16／receipt5／approval14保持；仅新性能操作preview3/20、独立purpose及type。

| 源码／UI路径（12） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetRegistry.h` | native-only preview／一次CAS接口 |
| `custom/src/Swarm/FleetRegistry.cc` | 规范数字stage、原源typed指纹及提交事实 |
| `custom/src/Swarm/FleetCapabilityReview.h` | 独立性能请求与私有proposal绑定 |
| `custom/src/Swarm/FleetCapabilityReview.cc` | 专用请求、preview3/20及实际一次提交 |
| `custom/src/Swarm/FleetOperatorSession.h` | 独立审批purpose合同 |
| `custom/src/Swarm/FleetOperatorSession.cc` | closed purpose／提示／consume／已消费尾核 |
| `custom/res/Merivus/GuidedActionsController.qml` | 专用Root值桥、两proposal来源与独立审计 |
| `custom/res/Merivus/FleetCapabilityReviewPanel.qml` | 第五操作、数字／unknown／预览与确认 |
| `custom/src/Swarm/FleetEventArchiveCodec.cc` | 新operation/type/hash及原21键摘要 |
| `custom/src/Swarm/FleetEventArchiveHistoryReader.cc` | 跨库操作搜索闭表 |
| `custom/src/Swarm/FleetPersistentEventStore.cc` | 摘要记录type及本地search政策 |
| `custom/res/Merivus/FleetArchiveHistoryPanel.qml` | 当前页／跨库操作筛选与摘要辨认 |

| 文档路径（19） | 本批内容 |
| --- | --- |
| `README.md` | 484当前入口、合同或交接／审计 |
| `MODIFIED_FILES.md` | 484当前入口、合同或交接／审计 |
| `docs/PROJECT_STATUS.md` | 484当前入口、合同或交接／审计 |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` | 484当前入口、合同或交接／审计 |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` | 484当前入口、合同或交接／审计 |
| `docs/INDEX.md` | 484当前入口、合同或交接／审计 |
| `docs/handoff/README.md` | 484当前入口、合同或交接／审计 |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` | 484当前入口、合同或交接／审计 |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` | 484当前入口、合同或交接／审计 |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` | 484当前入口、合同或交接／审计 |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` | 484当前入口、合同或交接／审计 |
| `custom/src/Swarm/FleetRegistry.md` | 484当前入口、合同或交接／审计 |
| `custom/src/Swarm/FleetCapabilityReview.md` | 484当前入口、合同或交接／审计 |
| `custom/src/Swarm/FleetOperatorSession.md` | 484当前入口、合同或交接／审计 |
| `custom/src/Swarm/FleetCapabilityReviewPanel.md` | 484当前入口、合同或交接／审计 |
| `custom/src/Swarm/FleetEventArchiveCodec.md` | 484当前入口、合同或交接／审计 |
| `custom/src/Swarm/FleetEventArchiveHistoryReader.md` | 484当前入口、合同或交接／审计 |
| `custom/src/Swarm/FleetPersistentEventStore.md` | 484当前入口、合同或交接／审计 |
| `custom/src/Swarm/FleetArchiveHistoryPanel.md` | 484当前入口、合同或交接／审计 |

19份文档仅新增顶部484段，旧历史保留。32项工程场景全部未执行，本批审计已记录独立静读、两处修正及最终普通文本核对，不是运行验收。原25目标与六类实质剩余保持：完整执行许可／动态轨迹及正式结果、可信实测性能、自愈机载改派、企业跨站控制、长期运营、真实模型／插件生态。整体约40%完成／60%剩余仍仅源码工作量粗估，不按31路径或新入口上调。

本轮仅静态源码／文档工作；未编译、运行helper／parser／AST／业务／模型／QML／应用／认证／SQL，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。工程师后续验证；没有本批安装包。 下方为历史。

## 483 当前准确范围：12源码、19文档，共31路径

仅统计本轮范围，不统计整个既有脏树。无新文件、依赖、工程登记、UI、PX4或命令权限变化；本轮只消费既有批准数字，不新增性能录入器／实测清单或认证。原批准／identity hash schema保持。

| 源码路径（12） | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetCapabilityEvidence.h` | 纯值currentDeclaredPerformance API及边界 |
| `custom/src/Swarm/FleetCapabilityEvidence.cc` | 当前批准＋完整基线、exact11规范数值、独立身份存储 |
| `custom/src/Swarm/FleetCapabilityMatcher.h` | 严格声明消费合同 |
| `custom/src/Swarm/FleetCapabilityMatcher.cc` | 严格候选／阈值、逐机证据及重复ID处理 |
| `custom/src/Swarm/FleetEnergyScheduler.h` | 严格声明性预算合同 |
| `custom/src/Swarm/FleetEnergyScheduler.cc` | 声明续航×原电量、native direct仅取小、完整来源 |
| `custom/src/Swarm/FleetReserveEnergyPlan.h` | 第5个原QVariant策略、默认false旧口 |
| `custom/src/Swarm/FleetReserveEnergyPlan.cc` | 备用自身声明速度约束及严格来源 |
| `custom/src/Swarm/FleetRoleAllocator.cc` | 自动角色打分前真实声明／全局阈值过滤 |
| `custom/src/Swarm/FleetMissionCompiler.cc` | 主备阈值／floor／实际速度、两备用调用、strict 0.8身份 |
| `custom/src/Swarm/FleetMissionReviewCoordinator.cc` | capabilityEvidence/schema3完整声明及usable纳fingerprint |
| `custom/src/Swarm/FleetMissionPlanStore.cc` | 计划bool白名单与0.8.0必保true策略 |

| 文档路径（19） | 本批内容 |
| --- | --- |
| `README.md` | 当前入口及未完成边界 |
| `MODIFIED_FILES.md` | 31路径增量入口 |
| `docs/INDEX.md` | 八模块及本批交接索引 |
| `docs/PROJECT_STATUS.md` | 有限进展、六类剩余与40／60粗估 |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` | 可信性能消费竖切及后续未完成 |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` | 源码边界与未验收范围 |
| `docs/handoff/README.md` | 工程师接收顺序 |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` | 本轮范围、独立静读和最终普通文本核对 |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` | 30项全部未执行矩阵 |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` | 本清单 |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` | 工程声明、原生来源与权限分离 |
| `custom/src/Swarm/FleetCapabilityEvidence.md` | exact11、unknown、别名与所有权合同 |
| `custom/src/Swarm/FleetCapabilityMatcher.md` | 严格选机与旧规划兼容 |
| `custom/src/Swarm/FleetEnergyScheduler.md` | 逐机严格估算及来源 |
| `custom/src/Swarm/FleetReserveEnergyPlan.md` | 原约束及备用第5参数合同 |
| `custom/src/Swarm/FleetMissionCompiler.md` | 主备实际消费与版本／身份 |
| `custom/src/Swarm/FleetMissionReviewCoordinator.md` | schema3完整声明指纹 |
| `custom/src/Swarm/FleetMissionPlanStore.md` | 新策略标记归档合同 |
| `custom/src/Swarm/FleetRoleAllocator.md` | 自动分配的真实严格过滤 |

19份文档只新增顶部483段，旧历史完整保留。本批审计已记录独立静读和最终普通文本核对，不是运行验收。30项工程场景全部未执行；未编译、运行helper／模型／业务／QML、测试、仿真、硬件、PX4修改、刷机、放行、提交、更新PR或打包，无安装包／构建产物。原25目标、六类实质剩余与约40%／60%粗估不按31路径上调。下方为历史。

## 482 当前准确范围：19源码／登记／UI、22文档，共41路径

本表只统计482范围，不统计整个既有脏树。新增无QObject纯值 `FleetMissionWaypointSwitchReview.h/.cc` 与模块文档，既有custom.pri登记其源／头；没有PX4、新依赖、MAVLink消息、QML类型、硬件标准或命令权限变更。旧12hash、公开19键和PM12／14／28三口精确旧合同保持。

| 源码／登记／UI路径（19） | 本批内容 |
| --- | --- |
| `src/FactSystem/ParameterManager.h` | 独立arrival名单、29行原源尾核及严格快照codec API |
| `src/FactSystem/ParameterManager.cc` | 固定12／2／14／1恢复、RAMP负零、同原owner／revision／Fact／年龄核验 |
| `custom/src/Swarm/FleetMissionEvidenceRefresh.cc` | 固定联合29项、真实arrival类型／数域，原期限不续期 |
| `custom/src/Swarm/FleetMissionVehicleEvidence.cc` | schema5独立arrivalEvidence、双稳定指纹及原29最终尾核 |
| `custom/src/Swarm/FleetMissionStartReadiness.cc` | 同次原29snapshot、exact6风险材料、两处回调尾核与三嵌套失效 |
| `custom/src/Swarm/FleetMissionStartReview.h` | 普通审阅保原参数快照的私有状态 |
| `custom/src/Swarm/FleetMissionStartReview.cc` | 原29冻结进Worker row4、异步原源核验与完整第三报告 |
| `custom/src/Swarm/FleetMissionArmedGroundReview.h` | 新地面审阅原参数快照状态 |
| `custom/src/Swarm/FleetMissionArmedGroundReview.cc` | 当前phase同次冻结、row4、finished及风险／Witness绑定 |
| `custom/src/Swarm/FleetMissionStartupClearanceWitness.h` | capture明确接原PRIMARY弱owner map |
| `custom/src/Swarm/FleetMissionStartupClearanceWitness.cc` | 独立复制原弱map、原29快照真实memory尾核，不换源续期 |
| `custom/src/Swarm/FleetMissionStartupAuthority.cc` | 完整result7和同一原生Witness接管，不放宽权限 |
| `custom/src/Swarm/FleetMissionGroundStartupAuthority.cc` | 地面完整result7绑定及原Witness边界 |
| `custom/src/Swarm/FleetMissionStartupClearanceWorker.h` | schema3 exact8／row4、result7合同 |
| `custom/src/Swarm/FleetMissionStartupClearanceWorker.cc` | 实际第三报告计算、完整匹配和共用准入消费 |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.h` | 新静态NAV接受／连接审阅API（新增） |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.cc` | 精确接受界、完整AABB、protected／support环境比较及全组预算（新增） |
| `custom/custom.pri` | 新Review源／头工程登记 |
| `custom/res/Merivus/FleetMissionDeploymentPanel.qml` | 只读刷新29项说明／确认文字，保原门禁 |

| 文档路径（22） | 本批内容 |
| --- | --- |
| `README.md` | 482 current入口、实际增量与未完成边界 |
| `MODIFIED_FILES.md` | 本批41路径入口 |
| `docs/PROJECT_STATUS.md` | 实际参数／模型／消费者与剩余状态 |
| `docs/MERIVUS_FLEET_OS_ROADMAP.md` | P0静态NAV增量、下一步动态模型 |
| `docs/MERIVUS_SWARM_TARGET_MATRIX.md` | 源码增量／未完成区分，整目标数不升级 |
| `docs/INDEX.md` | 当前模块及交接索引 |
| `docs/handoff/README.md` | 工程接收入口与限制 |
| `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md` | 32项全部未执行场景 |
| `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md` | 本准确清单 |
| `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md` | 直接静读与最终普通文本核对的限定记录 |
| `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md` | 原source／owner／期限与权限边界 |
| `custom/src/Swarm/FleetMissionVehicleEvidence.md` | schema5与独立arrivalEvidence |
| `custom/src/Swarm/FleetMissionStartReadiness.md` | 同次exact6原29快照及风险身份 |
| `custom/src/Swarm/FleetMissionEvidenceRefresh.md` | 联合29项及PM codec／旧接口边界 |
| `custom/src/Swarm/FleetMissionStartReview.md` | 普通审阅原源与第三报告 |
| `custom/src/Swarm/FleetMissionArmedGroundReview.md` | 新地面审阅同次参数及原源尾核 |
| `custom/src/Swarm/FleetMissionStartupClearanceWitness.md` | 原弱owner map及29行memory |
| `custom/src/Swarm/FleetMissionStartupAuthority.md` | result7／同Witness接管 |
| `custom/src/Swarm/FleetMissionGroundStartupAuthority.md` | 地面result7／同Witness接管 |
| `custom/src/Swarm/FleetMissionStartupClearanceWorker.md` | schema3 exact8／row4／result7及实际消费 |
| `custom/src/Swarm/FleetMissionDeployment.md` | 29项UI静态说明与旧hash边界 |
| `custom/src/Swarm/FleetMissionWaypointSwitchReview.md` | 新模块合同、几何、预算、证明限制（新增） |

文档只新增顶部482/current段，旧历史完整保留，不批量改旧数字。本批无提交、新PR、安装包、构建或测试产物；仅完成直接静读与最终普通文本／路径／Markdown／空白核对，详见本批静态审计，不代表运行验收。未编译、执行helper／模型／业务／QML、测试、仿真、操作硬件、修改PX4、刷机或放行。原25目标及六类实质剩余、整体约40%源码工作量粗估保持。下方为历史。

## 481 准确本批范围：16源码／登记、23文档，共39路径

新增StartupIntegerDomain.h／cc／md三个文件、一个无QObject原生不可变类；custom.pri只登记其SOURCES／HEADERS。无新QML类型、资源、依赖、MAVLink消息或PX4变更。不统计整个脏树。

| 源码／登记路径 | 本批内容 |
| --- | --- |
| `custom/custom.pri` | 新不可变整数域源／头登记 |
| `custom/src/Swarm/FleetMissionContactModel.h` | 精确binary64毫米界和整数E7/mm投影API |
| `custom/src/Swarm/FleetMissionContactModel.cc` | 完整frame／整数范围／失败原子输出 |
| `custom/src/Swarm/FleetMissionExecutionProfile.h` | 新可选显式空间guard合同 |
| `custom/src/Swarm/FleetMissionExecutionProfile.cc` | 严格guard字段与旧profile兼容 |
| `custom/res/Merivus/FleetExecutionProfileEditor.qml` | 显式选择及完整保存、确认／重入互斥 |
| `custom/src/Swarm/FleetMissionStartupIntegerDomain.h` | 完整raw域不可变API |
| `custom/src/Swarm/FleetMissionStartupIntegerDomain.cc` | 两帽全域／整数slab／membership／完整source |
| `custom/src/Swarm/FleetMissionStartupGeographicMotion.h` | 保原Ptr及新source2接口 |
| `custom/src/Swarm/FleetMissionStartupGeographicMotion.cc` | 实际contains整数gate、选择／源绑定 |
| `custom/src/Swarm/FleetMissionStartupDomainCover.h` | 原生域投影结果合同 |
| `custom/src/Swarm/FleetMissionStartupDomainCover.cc` | 同原域完整整数投影与旧模型分流 |
| `custom/src/Swarm/FleetMissionStartupDomainReview.h` | 报告语义注释 |
| `custom/src/Swarm/FleetMissionStartupDomainReview.cc` | schema2逐机包含与环境结论分离 |
| `custom/src/Swarm/FleetMissionStartupClearanceWorker.h` | 实际准入及版本化来源注释 |
| `custom/src/Swarm/FleetMissionStartupClearanceWorker.cc` | profile双向绑定、值型重现与实际路径条件 |

文档23份：

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetMissionStartupIntegerDomain.md`
- `custom/src/Swarm/FleetMissionStartupGeographicMotion.md`
- `custom/src/Swarm/FleetMissionExecutionProfile.md`
- `custom/src/Swarm/FleetExecutionProfileEditor.md`
- `custom/src/Swarm/FleetMissionContactModel.md`
- `custom/src/Swarm/FleetMissionStartupDomainCover.md`
- `custom/src/Swarm/FleetMissionStartupDomainReview.md`
- `custom/src/Swarm/FleetMissionStartupClearanceWorker.md`
- `custom/src/Swarm/FleetMissionStartupClearanceWitness.md`
- `custom/src/Swarm/FleetMissionStartupTransaction.md`
- `custom/src/Swarm/FleetMissionStartReview.md`
- `custom/src/Swarm/FleetMissionArmedGroundReview.md`

没有改旧12hash／公开19键、startup policy1–4 schema、实际命令发送器、Run／占用或名义Contact豁免。新增profile显式选择和新模型身份不替旧批准升级。当前实际路径未证明，StartPermit及其余原目标保持未完成。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 480 准确本批范围：9源码／登记、18文档，共27路径

新增三个文件：FleetMissionActionPhaseReview.h／cc／md；一个无QObject纯值类，custom.pri只增加其SOURCES／HEADERS登记。无QML类型、依赖、资源或PX4变更。仅列本批，不统计整个脏树。

| 源码／登记路径 | 本批内容 |
| --- | --- |
| `custom/custom.pri` | 动作参数纯值模型源／头登记 |
| `custom/src/Swarm/FleetMissionActionPhaseReview.h` | 固定12＋14输入与8键结果合同 |
| `custom/src/Swarm/FleetMissionActionPhaseReview.cc` | SYS_RESP一致、起降／RC分支及明确动作兼容前置 |
| `src/FactSystem/ParameterManager.h` | 固定phase14及独立三集合28行memory gate |
| `src/FactSystem/ParameterManager.cc` | 旧12／14口保持，原28来源／类型／revision／年龄终检 |
| `custom/src/Swarm/FleetMissionEvidenceRefresh.cc` | 联合排序28固定读取，原期限／权限不续期 |
| `custom/src/Swarm/FleetMissionVehicleEvidence.cc` | schema4独立动作证据、真实不兼容消费者及原28尾核 |
| `custom/src/Swarm/FleetMissionStartReadiness.cc` | 原28快照贯穿回调，稳定摘要与两处失效清资格 |
| `custom/res/Merivus/FleetMissionDeploymentPanel.qml` | 刷新说明及勾选文字同步28项，门禁不变 |

文档18份：

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetMissionActionPhaseReview.md`
- `custom/src/Swarm/FleetMissionEvidenceRefresh.md`
- `custom/src/Swarm/FleetMissionVehicleEvidence.md`
- `custom/src/Swarm/FleetMissionStartReadiness.md`
- `custom/src/Swarm/FleetMissionExecutionProfile.md`
- `custom/src/Swarm/FleetMissionReviewCoordinator.md`
- `custom/src/Swarm/FleetMissionDeployment.md`

没有更改profile schema／旧12hash／公开19键观察接口代码、Compiler／PayloadBuilder、MAVLink消息、实际飞行命令或最终StartPermit。原Contact／接地例外及exact27 takeoffDerivation保持。旧批准不迁移或自动恢复。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数分支一致不认证真机二进制、当前控制器阶段、真实速度／起降轨迹／完成时刻／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 479 准确本批范围：9源码／登记、18文档，共27路径

新增三个文件：FleetMissionRateOverrideReview.h／cc／md；一个纯值C++类，custom.pri只增加其SOURCES／HEADERS登记。无QObject／QML类型、依赖、资源或PX4变更。仅列本批，不统计整个脏树。

| 源码／登记路径 | 本批内容 |
| --- | --- |
| `custom/custom.pri` | 纯C++模型源／头登记 |
| `custom/src/Swarm/FleetMissionRateOverrideReview.h` | 原12＋2内容与本地float32覆写规则逐值复核 |
| `custom/src/Swarm/FleetMissionRateOverrideReview.cc` | 原12＋2内容与本地float32覆写规则逐值复核 |
| `src/FactSystem/ParameterManager.h` | 固定guard观察及原12＋2纯内存终检，旧12口保留 |
| `src/FactSystem/ParameterManager.cc` | 固定guard观察及原12＋2纯内存终检，旧12口保留 |
| `custom/src/Swarm/FleetMissionEvidenceRefresh.cc` | 明确读取14固定项，原权限／时限不续期 |
| `custom/src/Swarm/FleetMissionVehicleEvidence.cc` | 独立guard证据、真实失配消费者与所有getter之后原14尾核 |
| `custom/src/Swarm/FleetMissionStartReadiness.cc` | 普通Readiness／phase原快照贯穿所有回调，最终只核原内存 |
| `custom/res/Merivus/FleetMissionDeploymentPanel.qml` | 说明及勾选文案同步实际14项范围，原门禁不变 |

文档18份：

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetMissionRateOverrideReview.md`
- `custom/src/Swarm/FleetMissionEvidenceRefresh.md`
- `custom/src/Swarm/FleetMissionVehicleEvidence.md`
- `custom/src/Swarm/FleetMissionStartReadiness.md`
- `custom/src/Swarm/FleetMissionExecutionProfile.md`
- `custom/src/Swarm/FleetMissionReviewCoordinator.md`
- `custom/src/Swarm/FleetMissionDeployment.md`

未修改Compiler／PayloadBuilder／profile schema／公开参数hash观察接口代码、MAVLink消息、实际飞行命令或启动许可。旧12参数profile绑定与14项当前证据分开，不迁移历史批准。

本批仅修改地面站源码、工程登记和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或模型实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。参数覆盖模型一致不认证实际二进制、内部当前值、真实速度／起降轨迹／时序／动态净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍仅源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 478 准确本批范围：10源码、21文档，共31路径

没有新增文件、类型、第三方依赖或工程／资源登记。不计整个脏树；没有改PX4、MAVLink消息、实际发送器或Run／占用／StartPermit。

| 源码路径 | 本批内容 |
| --- | --- |
| `custom/src/Swarm/FleetAreaCoveragePlanner.h` | 原坐标canonical policy三API与原Domain共享 |
| `custom/src/Swarm/FleetAreaCoveragePlanner.cc` | 原坐标canonical policy三API与原Domain共享 |
| `custom/src/Swarm/FleetMissionCompiler.cc` | 0.7／plan1.1完整policy保存与身份 |
| `custom/src/Swarm/FleetMissionPlanStore.cc` | 白名单、strictpolicy、完整编辑源及覆盖摘要对应 |
| `custom/src/Swarm/FleetMissionPayloadBuilder.h` | 严格当前构建、独立旧档重现、整组实际编码后边界复核 |
| `custom/src/Swarm/FleetMissionPayloadBuilder.cc` | 严格当前构建、独立旧档重现、整组实际编码后边界复核 |
| `custom/src/Swarm/FleetMissionRemainderPlanner.cc` | single provenance3／4、来源互斥与原task／plan政策对应 |
| `custom/src/Swarm/FleetMissionGroupRemainderPlanner.cc` | group provenance2与完整policy task／plan对应 |
| `custom/src/Swarm/FleetMissionStartupTransaction.cc` | 3处资料链中的两历史重放与task／plan互核 |
| `custom/src/Swarm/FleetMissionStartReadiness.cc` | 单机重建和inputHash保原完整policy |

文档21份：

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetAreaCoveragePlanner.md`
- `custom/src/Swarm/FleetMissionCompiler.md`
- `custom/src/Swarm/FleetMissionPlanStore.md`
- `custom/src/Swarm/FleetMissionPayloadBuilder.md`
- `custom/src/Swarm/FleetMissionRemainderPlanner.md`
- `custom/src/Swarm/FleetMissionGroupRemainderPlanner.md`
- `custom/src/Swarm/FleetMissionStartupTransaction.md`
- `custom/src/Swarm/FleetMissionStartReadiness.md`
- `custom/src/Swarm/FleetMissionRemainderSource.md`
- `custom/src/Swarm/FleetMissionRemainderView.md`

普通文本／路径／Markdown目标／空白核对结果在静态审计记录；不将其称为编译、类型检查、算法执行或运行验收。下方保留477及更早准确范围。

本批仅修改地面站源码和交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。编码后名义二维检查不认证数值误差、动力学、真实障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍是源码工作量粗估，不按批次或文件计验收率。

下方为历史记录。

## 477 准确本批范围：8源码、15文档，共23路径

无新增文件、类型、第三方包或工程／资源登记。现有QtGui与Planner原登记保留；原QGCMapPolygon offset未修改。只列本轮负责路径，不计整个脏树或批次类数；未改PX4、MAVLink、原Run／占用／StartPermit／实际飞行权限。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码 | [custom/src/Swarm/FleetAreaCoveragePlanner.h](../../custom/src/Swarm/FleetAreaCoveragePlanner.h) | 明确正退让和共享显式路线合同 |
| 源码 | [custom/src/Swarm/FleetAreaCoveragePlanner.cc](../../custom/src/Swarm/FleetAreaCoveragePlanner.cc) | 保守候选／原域整段欧氏距边／子分量分配／显式独立检查 |
| 源码 | [custom/src/Swarm/FleetMissionDraft.h](../../custom/src/Swarm/FleetMissionDraft.h) | 区域完整策略revision-CAS声明 |
| 源码 | [custom/src/Swarm/FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc) | source4、严格可选退让与一次原子提交 |
| 源码 | [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 明确已有全路线checker同时消费正退让 |
| 源码 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 完整source4预检及冻结backend的同次策略门面 |
| 源码 | [custom/res/Merivus/FleetCoverageExclusionsEditor.qml](../../custom/res/Merivus/FleetCoverageExclusionsEditor.qml) | 两字段保存／同意失效／通知尾核及取消ticket |
| 源码 | [custom/res/Merivus/FlyViewMap.qml](../../custom/res/Merivus/FlyViewMap.qml) | 原输入边界的名义退让文字，不伪画安全域 |
| 文档 | [README.md](../../README.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [custom/src/Swarm/FleetAreaCoveragePlanner.md](../../custom/src/Swarm/FleetAreaCoveragePlanner.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [custom/src/Swarm/FleetCoverageExclusionsEditor.md](../../custom/src/Swarm/FleetCoverageExclusionsEditor.md) | 合同／状态／范围／审计或工程师交接 |

候选使用方形支撑缓冲的多边形布尔，完整原域／所有边距离复核独立；全部候选分量保留并按端点／负载分配。共享显式checker不生成候选，Compiler继续全主／辅助／备用预算路线消费。m仅为明确输入，不是推断的实物安全距离。编码量化和真实运动不在本批证明范围。

本批仅修改地面站源码和交接材料；核对限直接静态阅读、Qt官方API文档与普通文本／路径／Markdown目标／git空白，未编译、运行helper／解析器／业务脚本／QML／应用或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。名义二维退让不认证误差、动力学、障碍净空或启动许可。原25目标及六类实质剩余保持；总体约40%仍只是源码工作量粗估，不按文件／批次提高验收率。

下方为历史记录。

## 476 准确本批范围：11源码／资源、15文档，共26路径

新增FleetCoverageExclusionsEditor.qml及其模块说明两个文件；其余为既有路径增量。只列本轮实际负责路径，不计整个脏树或类数量。不改PX4、原Run注册／占用／启动许可、MAVLink消息或机载协议。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码／资源 | [custom/src/Swarm/FleetAreaCoveragePlanner.h](../../custom/src/Swarm/FleetAreaCoveragePlanner.h) | 带孔生成及共享显式路线检查声明 |
| 源码／资源 | [custom/src/Swarm/FleetAreaCoveragePlanner.cc](../../custom/src/Swarm/FleetAreaCoveragePlanner.cc) | 完整域拓扑／扫描／两种可视图和去返桥接 |
| 源码／资源 | [custom/src/Swarm/FleetMissionDraft.h](../../custom/src/Swarm/FleetMissionDraft.h) | revision-CAS排除区设置／清除API |
| 源码／资源 | [custom/src/Swarm/FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc) | source3与128点整体预算／保存恢复源 |
| 源码／资源 | [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 最终全成员／全局回退／备用名义路线真实检查 |
| 源码／资源 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | source3预检与冻结原backend的完整设置／孔编辑 |
| 源码／资源 | [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 实际编辑实例及互斥pending／焦点 |
| 源码／资源 | [custom/res/Merivus/FleetCoverageExclusionsEditor.qml](../../custom/res/Merivus/FleetCoverageExclusionsEditor.qml) | 新明确替换编辑／失败隐藏撤同意／来源保留 |
| 源码／资源 | [custom/res/Merivus/FlyViewMap.qml](../../custom/res/Merivus/FlyViewMap.qml) | 完整草稿孔图层及文字标识，不显示成功前缀 |
| 源码／资源 | [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 新增编辑器资源alias |
| 源码／资源 | [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | 新增编辑器模块类型 |
| 文档 | [README.md](../../README.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [custom/src/Swarm/FleetAreaCoveragePlanner.md](../../custom/src/Swarm/FleetAreaCoveragePlanner.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md) | 合同／状态／范围／审计或工程师交接 |
| 文档 | [custom/src/Swarm/FleetCoverageExclusionsEditor.md](../../custom/src/Swarm/FleetCoverageExclusionsEditor.md) | 合同／状态／范围／审计或工程师交接 |

完整名义路线检查包括所有主／辅助路线、合法全局回退、去返连续边及完整备用预算。备用只作为检查副本，不加入执行assignments；孔声明本身不上传为机载围栏或传感避障。边界可沿、没有退让／动态净空，旧permission false及pending保持。

本批仅地面站源码、资源登记与工程交接材料；只做直接静态阅读及普通文本／路径／Markdown目标／git空白核对，未编译、运行解析器／业务脚本／QML／应用／认证／SQL或几何实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；约40%只是源码工作量粗估，不按文件／批次计验收率。

下方为历史记录。

## 475 准确本批范围：15源码／登记、18文档，共33路径

新增GroupRemainderPlanner.h／cc／md及GroupRemainderPanel.qml／md五文件。只列本轮负责路径，不计整个脏树、类数量或记录编号；未改原Run注册、Observation、占用、PX4或完整启动许可，无新PR／安装包／构建或测试产物。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码／登记 | [custom/src/Swarm/FleetMissionGroupRemainderPlanner.h](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.h) | 新纯联合算法与来源API |
| 源码／登记 | [custom/src/Swarm/FleetMissionGroupRemainderPlanner.cc](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.cc) | 完整闭合原组、政策／逐机预算和联合hash |
| 源码／登记 | [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 三来源互斥及逐机真实速度证据门槛 |
| 源码／登记 | [custom/src/Swarm/FleetMissionPlanStore.cc](../../custom/src/Swarm/FleetMissionPlanStore.cc) | 完整联合provenance归档／导入消费 |
| 源码／登记 | [custom/src/Swarm/FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h) | 联合提案与显示薄接口 |
| 源码／登记 | [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc) | 实际组提案／View及本机返航floor能量消费 |
| 源码／登记 | [custom/src/Swarm/FleetMissionRemainderView.h](../../custom/src/Swarm/FleetMissionRemainderView.h) | 独立buildGroup显示声明 |
| 源码／登记 | [custom/src/Swarm/FleetMissionRemainderView.cc](../../custom/src/Swarm/FleetMissionRemainderView.cc) | 全组几何／单次整组编码且无成功前缀 |
| 源码／登记 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 私有组来源及共用一次审阅／批准／部署接线 |
| 源码／登记 | [custom/res/Merivus/FleetMissionGroupRemainderPanel.qml](../../custom/res/Merivus/FleetMissionGroupRemainderPanel.qml) | 新逐成员联合编辑、完整结果和精确取消／退出 |
| 源码／登记 | [custom/res/Merivus/FleetMissionRemainderPanel.qml](../../custom/res/Merivus/FleetMissionRemainderPanel.qml) | 单／组状态隔离，不显示或取消组后台 |
| 源码／登记 | [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 互斥pending及共享完整组部署源 |
| 源码／登记 | [custom/custom.pri](../../custom/custom.pri) | 新纯C++类工程登记 |
| 源码／登记 | [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 新GroupPanel资源alias |
| 源码／登记 | [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | 新GroupPanel模块登记 |
| 文档 | [README.md](../../README.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [custom/src/Swarm/FleetMissionGroupRemainderPlanner.md](../../custom/src/Swarm/FleetMissionGroupRemainderPlanner.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderView.md](../../custom/src/Swarm/FleetMissionRemainderView.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [custom/src/Swarm/FleetMissionGroupRemainderPanel.md](../../custom/src/Swarm/FleetMissionGroupRemainderPanel.md) | 合同／状态／准确范围／静审或工程师接手同步 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderPanel.md](../../custom/src/Swarm/FleetMissionRemainderPanel.md) | 合同／状态／准确范围／静审或工程师接手同步 |

整组新任务是明确人工新规划，不是原Run续飞、控制租约、自动Leader接替或正式完成认证。每个显式要求未被来源／性能／预算／空域支持时保持阻断，不按有代码类或文件数计完成率。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

## 474 准确本批范围：6源码、16文档，共22路径

只列本批负责路径；均为既有文件，无新增类型、资源登记、构建／测试产物、PR或安装包。没有修改原Run注册、Observation、占用、原single Planner／Compiler、启动许可或PX4。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码 | [custom/src/Swarm/FleetMissionStartupTransaction.h](../../custom/src/Swarm/FleetMissionStartupTransaction.h) | 新增真实整组getter及共享纯值集合／成员选择API |
| 源码 | [custom/src/Swarm/FleetMissionStartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc) | 一次原生观察、全组载荷／映射／预算与尾核 |
| 源码 | [custom/src/Swarm/FleetMissionRemainderArchive.h](../../custom/src/Swarm/FleetMissionRemainderArchive.h) | 双版本包、窄目录及显式成员选择API |
| 源码 | [custom/src/Swarm/FleetMissionRemainderArchive.cc](../../custom/src/Swarm/FleetMissionRemainderArchive.cc) | v2/common单副本、全包校验和指定成员历史投影 |
| 源码 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 私有metadata、整组捕获／成员选用及真实scope桥接 |
| 源码 | [custom/res/Merivus/FleetMissionRemainderArchivePanel.qml](../../custom/res/Merivus/FleetMissionRemainderArchivePanel.qml) | 完整成员目录／分页／显式选择和ABA确认尾核 |
| 文档 | [README.md](../../README.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionStartupTransaction.md](../../custom/src/Swarm/FleetMissionStartupTransaction.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderArchive.md](../../custom/src/Swarm/FleetMissionRemainderArchive.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderArchivePanel.md](../../custom/src/Swarm/FleetMissionRemainderArchivePanel.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderPanel.md](../../custom/src/Swarm/FleetMissionRemainderPanel.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderSource.md](../../custom/src/Swarm/FleetMissionRemainderSource.md) | 本批合同／状态／准确范围／静审或待执行交接同步 |

原组所有目标完整保存，只有提交成员可选；旧单成员文件不猜测升级。新历史source20仍进原single提案／新审阅。单次原生读取不是远端同时采样；hash不是认证。联合多成员新计划及其它六类实质开发继续。

仅地面站源码及交接文档，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、运行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估，不按批次或文件数上调。下方为历史。

## 473 准确本批范围：11源码／登记资源、19文档，共30路径

仅列本批负责路径，不计全脏树或按记录编号计完成率。本次新增Archive.h／cc／md及ArchivePanel.qml／md五文件，其余是本批既有路径。未修改原Run注册／Observation／报告／占用／PX4，也不改变完整启动许可或运动数值算法；无工具、测试或构建产物，无新PR或安装包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码／登记资源 | [custom/src/Swarm/FleetMissionRemainderPlanner.h](../../custom/src/Swarm/FleetMissionRemainderPlanner.h) | 共享完整源validateSource API |
| 源码／登记资源 | [custom/src/Swarm/FleetMissionRemainderPlanner.cc](../../custom/src/Swarm/FleetMissionRemainderPlanner.cc) | 原生18／历史20、provenance28／30及原余段算法共用 |
| 源码／登记资源 | [custom/src/Swarm/FleetMissionRemainderArchive.h](../../custom/src/Swarm/FleetMissionRemainderArchive.h) | 新无状态私有包服务及精确API |
| 源码／登记资源 | [custom/src/Swarm/FleetMissionRemainderArchive.cc](../../custom/src/Swarm/FleetMissionRemainderArchive.cc) | exact8完整包、历史转换、实际保存与整文件读回 |
| 源码／登记资源 | [custom/custom.pri](../../custom/custom.pri) | 新Archive h／cc工程登记 |
| 源码／登记资源 | [custom/src/CustomPlugin.cc](../../custom/src/CustomPlugin.cc) | 新MerivusInternal Archive类型注册 |
| 源码／登记资源 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 私有单槽、文件接口、历史捕获与精确退出／原审阅链 |
| 源码／登记资源 | [custom/res/Merivus/FleetMissionRemainderPanel.qml](../../custom/res/Merivus/FleetMissionRemainderPanel.qml) | 实际历史采用、原编辑／审阅及retained来源退出 |
| 源码／登记资源 | [custom/res/Merivus/FleetMissionRemainderArchivePanel.qml](../../custom/res/Merivus/FleetMissionRemainderArchivePanel.qml) | 新冻结／路径确认／打开／选用UI及回调归属 |
| 源码／登记资源 | [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 新资料面板资源alias |
| 源码／登记资源 | [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | 新资料面板模块登记 |
| 文档 | [README.md](../../README.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 当前状态／准确清单／限定静审或工程师交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderPlanner.md](../../custom/src/Swarm/FleetMissionRemainderPlanner.md) | 共享完整源与两版provenance合同 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderArchive.md](../../custom/src/Swarm/FleetMissionRemainderArchive.md) | 新exact8包、文件事实和历史边界 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderArchivePanel.md](../../custom/src/Swarm/FleetMissionRemainderArchivePanel.md) | 新实际工作流、Root私有值、精确退出及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderPanel.md](../../custom/src/Swarm/FleetMissionRemainderPanel.md) | 历史采用／原新审阅和退出／换包可达性 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderSource.md](../../custom/src/Swarm/FleetMissionRemainderSource.md) | 原native getter不变与历史包来源分工 |
| 文档 | [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md) | 历史provenance中央消费与原外层格式不变 |
| 文档 | [custom/src/Swarm/FleetMissionRemainderView.md](../../custom/src/Swarm/FleetMissionRemainderView.md) | 历史provenance校验及实际新计划显示边界 |
| 文档 | [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md) | 中央源／计划校验消费、原当前审阅门禁保持 |

原生完整来源18字段仍来自实际Run；新历史20字段及包ID／hash保独立kind，进入原包含式余段／完整停留／新任务审阅，不恢复旧状态。保存实际commit与后续读回／当前性分开。收起／退出的后台来源残留已修，只清精确本地关联，不删原归档或Run。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际资料包文件读写／hash；未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍只是源码工作量粗估。历史资料不恢复Run、占用、批准或执行许可。下方为历史。

## 472 准确本批范围：11源码／登记资源、17文档，共28路径

仅列本批负责路径，不计全脏树或按补丁编号计进度。本次新增Store.h／cc／md和SavedPanel.qml／md五文件；其余为本批既有路径。Compiler、Vehicle、运动数值算法和PX4不改；无工具、测试、构建产物、新PR或安装包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码／登记资源 | [custom/src/Swarm/FleetMissionDraft.h](../../custom/src/Swarm/FleetMissionDraft.h) | 新增完整编辑源验证／干净对象加载API |
| 源码／登记资源 | [custom/src/Swarm/FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc) | 新editing与旧strict分路、完整字段校验及local1／datumfalse |
| 源码／登记资源 | [custom/src/Swarm/FleetMissionDraftStore.h](../../custom/src/Swarm/FleetMissionDraftStore.h) | 新私有Store类型、exact结果及来源预览状态 |
| 源码／登记资源 | [custom/src/Swarm/FleetMissionDraftStore.cc](../../custom/src/Swarm/FleetMissionDraftStore.cc) | 新固定单槽、全字节CAS、实际提交／读回及调用前修订冻结 |
| 源码／登记资源 | [custom/custom.pri](../../custom/custom.pri) | Store h／cc工程登记 |
| 源码／登记资源 | [custom/src/CustomPlugin.cc](../../custom/src/CustomPlugin.cc) | 私有MerivusInternal原生类型注册 |
| 源码／登记资源 | [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 新saved私有来源、完整重复取源及原candidate／撤审批／备用流程 |
| 源码／登记资源 | [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 实际保存面板及inputSettled／planningBusy门禁 |
| 源码／登记资源 | [custom/res/Merivus/FleetMissionSavedDraftPanel.qml](../../custom/res/Merivus/FleetMissionSavedDraftPanel.qml) | 新明确保存恢复入口、请求冻结及上下文绑定反馈 |
| 源码／登记资源 | [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 新Panel资源alias |
| 源码／登记资源 | [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | 新Panel模块类型登记 |
| 文档 | [README.md](../../README.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 当前状态／准确清单／限定审阅或工程师交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md) | 双源校验／加载合同及完整编辑态边界 |
| 文档 | [custom/src/Swarm/FleetMissionDraftStore.md](../../custom/src/Swarm/FleetMissionDraftStore.md) | 新exact6／15／12、容量／锁／CAS／提交事实合同 |
| 文档 | [custom/src/Swarm/FleetMissionSavedDraftPanel.md](../../custom/src/Swarm/FleetMissionSavedDraftPanel.md) | 新实际交互／Root来源与未执行验证说明 |
| 文档 | [custom/src/Swarm/FleetMissionArchiveDraftPanel.md](../../custom/src/Swarm/FleetMissionArchiveDraftPanel.md) | 旧历史严格源与新编辑保存来源、共享备用说明 |
| 文档 | [custom/src/Swarm/FleetMissionRepairDraft.md](../../custom/src/Swarm/FleetMissionRepairDraft.md) | 旧失败部署严格源及新来源隔离说明 |
| 文档 | [custom/src/Swarm/FleetMissionRepairDraftPanel.md](../../custom/src/Swarm/FleetMissionRepairDraftPanel.md) | 旧入口与新保存面板、共用备用／撤审批边界 |

新saved-draft只恢复完整规范编辑内容，不要求buildTask；旧history／repair strict保持。固定单槽先预览后明确确认，原生实际Draft／修订／全源及磁盘全baseline必须对应；成功文件提交与后续current分开。恢复沿原candidate／撤审批／来源重读／共享备用发布，不恢复Run、机载状态或执行权限。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译或执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际草稿文件读写／hash；未测试、仿真、改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仅源码工作量粗估。保存恢复编辑态不恢复审批、机载状态或启动许可。下方为历史。

## 471 准确本批范围：13源码／18文档，共31路径

本批只列既有路径，不计全脏树、旧源码或改动记录编号。无新文件／工程与资源登记／QML／工具／测试或构建产物；470 Geometry索引、Compiler、Transition、Vehicle及PX4均不改。不提交、更新PR或打包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码 | [custom/src/Swarm/FleetMissionStartupGeographicMotion.h](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.h) | Models及不重算Qt的matchesSource原生API |
| 源码 | [custom/src/Swarm/FleetMissionStartupGeographicMotion.cc](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.cc) | 有界源数据深持有、完整参数／SHA／null yaw对应 |
| 源码 | [custom/src/Swarm/FleetMissionStartupClearanceWorker.h](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.h) | 原Models必需构造参数、native对应和原集合getter |
| 源码 | [custom/src/Swarm/FleetMissionStartupClearanceWorker.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.cc) | 独立外层原Ptr持有、native输入校验及实际传递 |
| 源码 | [custom/src/Swarm/FleetMissionStartupDomainReview.h](../../custom/src/Swarm/FleetMissionStartupDomainReview.h) | actual build必传原Models，明确value-only matcher边界 |
| 源码 | [custom/src/Swarm/FleetMissionStartupDomainReview.cc](../../custom/src/Swarm/FleetMissionStartupDomainReview.cc) | actual直接用原Ptr，只有值型结构核对允许临时重建 |
| 源码 | [custom/src/Swarm/FleetMissionStartReview.h](../../custom/src/Swarm/FleetMissionStartReview.h) | 普通Review私有原Models |
| 源码 | [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc) | 首次全组创建／原对象复用／finished与重入同集合核对 |
| 源码 | [custom/src/Swarm/FleetMissionArmedGroundReview.h](../../custom/src/Swarm/FleetMissionArmedGroundReview.h) | 地面Review原集合及待tick完成集合 |
| 源码 | [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc) | 完整原集合提交／actualfinished／待tick原对象保存 |
| 源码 | [custom/src/Swarm/FleetMissionStartupClearanceWitness.h](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.h) | capture原Models入参和原对象getter合同 |
| 源码 | [custom/src/Swarm/FleetMissionStartupClearanceWitness.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.cc) | 入口独立原集合、const State及Owner全集核对 |
| 源码 | [custom/src/Swarm/FleetMissionStartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc) | 资料分支matchesSource持原Ptr，无资料一次prepare保留 |
| 文档 | [README.md](../../README.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 当前状态、准确范围、限定审阅或工程师交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionStartupGeographicMotion.md](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.md) | 对应当前原模型／生命周期合同与未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupClearanceWorker.md](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md) | 对应当前原模型／生命周期合同与未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainReview.md](../../custom/src/Swarm/FleetMissionStartupDomainReview.md) | 对应当前原模型／生命周期合同与未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md) | 对应当前原模型／生命周期合同与未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md) | 对应当前原模型／生命周期合同与未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupClearanceWitness.md](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md) | 对应当前原模型／生命周期合同与未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupTransaction.md](../../custom/src/Swarm/FleetMissionStartupTransaction.md) | 对应当前原模型／生命周期合同与未执行验证 |

实际原Ptr从两Review贯穿Worker、Domain、Witness和Txn，不由source12重建替代；仅值型结构核对可临时prepare。模型源数据经完整校验后独立持有，matchesSource保全部原参数／SHA／null yaw语义。原input8／source12／result6／report28、contains、预算、权限／时限及numericfalse保持；不是数值完整包含、动作许可或正式结果完成。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。模型同一性不等于Qt数值包含或最终启动许可。下方为历史。

## 470 准确本批范围：3源码／13文档，共16路径

只列本批实际负责的既有路径，不计全脏树、历史源码或改动记录编号。无新文件／工程登记／QML资源／工具／测试或构建产物；原Motion／Input／ContactModel／Compiler／Vehicle／PX4不改。不提交、更新PR或打包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码 | [custom/src/Swarm/FleetMissionClearanceGeometry.h](../../custom/src/Swarm/FleetMissionClearanceGeometry.h) | 私有copy-only自持索引及完整准备／候选／同源indexed SAT三个API |
| 源码 | [custom/src/Swarm/FleetMissionClearanceGeometry.cc](../../custom/src/Swarm/FleetMissionClearanceGeometry.cc) | 整数Morton／可取消归并／闭XYZ树／根快路／完整原行候选及同源SAT |
| 源码 | [custom/src/Swarm/FleetMissionStartupDomainReview.cc](../../custom/src/Swarm/FleetMissionStartupDomainReview.cc) | 同Budget完整建两个索引，完整候选后按原行真实SAT和资料ID |
| 文档 | [README.md](../../README.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 当前状态、准确清单、限定静态审阅或工程师交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionClearanceGeometry.md](../../custom/src/Swarm/FleetMissionClearanceGeometry.md) | 对应当前索引／消费者合同、数学边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainReview.md](../../custom/src/Swarm/FleetMissionStartupDomainReview.md) | 对应当前索引／消费者合同、数学边界及未执行验证 |

索引完整持有原凸柱，与最终SAT同源；候选仅保守筛选，接触不剪，失败无前缀。原三种并集、完整高度带和同组2000000 work／2048 fragments保持，密集部分重叠不保证省work。原v2／report28／limits8／violation row6／Worker6及numericfalse不变，完整启动许可和正式结果未完成。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 469 准确本批范围：3源码／13文档，共16路径

只列本批实际负责路径，不计全脏树、历史源码或批次进度。新增API位于既有Geometry类及原native接入，无新文件／工程登记／QML资源／工具／测试或构建产物；原Motion／Input／ContactModel／Compiler／Vehicle／PX4均不改。不提交、更新PR或打包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码 | [FleetMissionClearanceGeometry.h](../../custom/src/Swarm/FleetMissionClearanceGeometry.h) | Point2、私有copy-only PreparedPrism及两种纯几何API |
| 源码 | [FleetMissionClearanceGeometry.cc](../../custom/src/Swarm/FleetMissionClearanceGeometry.cc) | 完整凸柱准备、整数双向闭SAT、共享预算及取消清输出 |
| 源码 | [FleetMissionStartupDomainReview.cc](../../custom/src/Swarm/FleetMissionStartupDomainReview.cc) | 一次完整scene准备，同Budget实际SAT替代地形／障碍逐对通用枚举 |
| 文档 | [README.md](../../README.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 状态、准确清单、路线图或限定静态交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionClearanceGeometry.md](../../custom/src/Swarm/FleetMissionClearanceGeometry.md) | 对应模块当前合同、数学边界和未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainReview.md](../../custom/src/Swarm/FleetMissionStartupDomainReview.md) | 对应模块当前合同、数学边界和未执行验证 |

PreparedPrism不是序列化报告或许可；全部scene先准备再查询，同一2M／2048 Budget贯穿。新SAT只用于完整AABB×严格凸闭竖柱，generic扫掠／并集保持。数学模型名v2、result3／report28／limits8／违反row6／Worker6不变，numericfalse和Compiler pending保持；没有运行几何或测速。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何函数，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按算法、文件或批次计验收比例。下方为历史。

## 468 准确本批范围：2源码／13文档，共15路径

仅列本批负责路径，不计全脏树或历史文件数。只修改两份既有后端实现，无新h／类／工程登记／资源／工具／测试文件或构建产物，不改原Motion／ContactModel／Geometry／Compiler／Vehicle／QML／PX4，不提交、更新PR或打包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码 | [FleetMissionStartupDomainCover.cc](../../custom/src/Swarm/FleetMissionStartupDomainCover.cc) | 完整cap纬度五带下界、ceil经度半宽及闭反经线 |
| 源码 | [FleetMissionStartupDomainReview.cc](../../custom/src/Swarm/FleetMissionStartupDomainReview.cc) | 模型v2、不完整明细前缀及地形覆盖去冗余 |
| 文档 | [README.md](../../README.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 状态、路线图、准确清单或静态交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainCover.md](../../custom/src/Swarm/FleetMissionStartupDomainCover.md) | 当前模块算法／合同和未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainReview.md](../../custom/src/Swarm/FleetMissionStartupDomainReview.md) | 当前模块算法／合同和未执行验证 |

当前算法名ideal-sphere-E7-two-cap-slab-outer-cover-v2，旧v1报告不接纳；原source12模型v1、input8及Worker result6不变。理想球面、完整slab和原允许运动规则不改，numericfalse、原接触边界及Compiler pending保持。去掉冗余平面可改变算法workUnits，不将其称为实测性能或飞行认证。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读、纸面推导和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际几何求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不因本批或分项主观估算增加验收比例。下方为历史。

## 467 准确本批范围：14源码／登记、20文档，共34路径

仅列本批实际负责路径，不按全脏树或历史改动统计进度。新增两个后端算法类及原生接入；不新增QML、资源、工具、测试文件或构建产物，不修改原Motion／ContactModel／Compiler／Vehicle／PX4，不提交、更新PR或打包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码／登记 | [custom.pri](../../custom/custom.pri) | 新增两组h／cc的工程登记，保既有脏树登记 |
| 源码／登记 | [FleetMissionStartupDomainCover.h](../../custom/src/Swarm/FleetMissionStartupDomainCover.h) | 原生完整理想域cover API |
| 源码／登记 | [FleetMissionStartupDomainCover.cc](../../custom/src/Swarm/FleetMissionStartupDomainCover.cc) | 整数两端帽、闭反经线、完整slab及whole-frame投影 |
| 源码／登记 | [FleetMissionStartupDomainReview.h](../../custom/src/Swarm/FleetMissionStartupDomainReview.h) | 整域result3及来源matcher API |
| 源码／登记 | [FleetMissionStartupDomainReview.cc](../../custom/src/Swarm/FleetMissionStartupDomainReview.cc) | 全组保护／支撑环境计算，不转移名义动作接触豁免 |
| 源码／登记 | [FleetMissionClearanceReview.h](../../custom/src/Swarm/FleetMissionClearanceReview.h) | 共享geometrySourceValid声明 |
| 源码／登记 | [FleetMissionClearanceReview.cc](../../custom/src/Swarm/FleetMissionClearanceReview.cc) | 复用原限制、规范化与完整Joint来源校验 |
| 源码／登记 | [FleetMissionStartupClearanceWorker.h](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.h) | 统一接纳API与严格result6说明 |
| 源码／登记 | [FleetMissionStartupClearanceWorker.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.cc) | 实际整域build／matcher／失败三空map及数值门禁 |
| 源码／登记 | [FleetMissionStartupClearanceWitness.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.cc) | 工厂独立要求名义＋整域＋数值包含 |
| 源码／登记 | [FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc) | 保完整两报告，原Qt未证明确clearance-blocked |
| 源码／登记 | [FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc) | 地面再审同门禁及两报告reason合并 |
| 源码／登记 | [FleetMissionStartupAuthority.cc](../../custom/src/Swarm/FleetMissionStartupAuthority.cc) | 消费严格result6及非空整域报告 |
| 源码／登记 | [FleetMissionGroundStartupAuthority.cc](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.cc) | 地面Authority同步result6 |
| 文档 | [README.md](../../README.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/INDEX.md](../../docs/INDEX.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/handoff/README.md](../../docs/handoff/README.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md) | 修改清单、当前状态、路线图或工程师静态交接同步 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainCover.md](../../custom/src/Swarm/FleetMissionStartupDomainCover.md) | 对应模块当前合同、边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupDomainReview.md](../../custom/src/Swarm/FleetMissionStartupDomainReview.md) | 对应模块当前合同、边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionClearanceReview.md](../../custom/src/Swarm/FleetMissionClearanceReview.md) | 对应模块当前合同、边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupClearanceWorker.md](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md) | 对应模块当前合同、边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupClearanceWitness.md](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md) | 对应模块当前合同、边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md) | 对应模块当前合同、边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md) | 对应模块当前合同、边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionStartupAuthority.md](../../custom/src/Swarm/FleetMissionStartupAuthority.md) | 对应模块当前合同、边界及未执行验证 |
| 文档 | [custom/src/Swarm/FleetMissionGroundStartupAuthority.md](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md) | 对应模块当前合同、边界及未执行验证 |

核心合同：Worker input8／schema2不变，结果严格6；DomainReview result3／report28／limits8／违反row6。原Qt数值包含固定false，整域不继承原TAKEOFF／LAND动作接地豁免；当前有资料路径保持clearance-blocked，不发布Witness或解除Compiler pending。未跟踪文件仍属于本地源码状态，不将登记或清单冒充已提交补丁包。

本批仅修改地面站源码、工程登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际环境求解，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量管理粗估，分项估算不是验收统计，不按本批文件数提高。下方为历史。

## 466 准确本批范围：6源码／14文档，共20路径

仅列本批实际负责路径，不按全脏树或历史文件数计算进度。复用原类／工程／资源，无新增工具、测试文件、构建产物、PR更新或交付包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码 | [Worker.h](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h) | 导入私有入口 |
| 源码 | [Worker.cc](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc) | import5／38／27、暂存读回、原生无覆盖发布 |
| 源码 | [Store.h](../../custom/src/Swarm/FleetPersistentEventStore.h) | 实际私有诊断来源与导入上下文 |
| 源码 | [Store.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc) | 三动作7、preview29、固定目标尾核及原结果20 |
| 源码 | [GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 新动作只传diagnosticsId，不传替换源或目标路径 |
| 源码 | [FleetArchiveMaintenancePanel.qml](../../custom/res/Merivus/FleetArchiveMaintenancePanel.qml) | readable选择、原预览／确认及38／27事实 |
| 文档 | [README](../../README.md) | 当前功能入口 |
| 文档 | [MODIFIED_FILES](../../MODIFIED_FILES.md) | 本批改动入口 |
| 文档 | [PROJECT_STATUS](../PROJECT_STATUS.md) | 进展与剩余范围 |
| 文档 | [ROADMAP](../MERIVUS_FLEET_OS_ROADMAP.md) | 原25目标保持 |
| 文档 | [TARGET_MATRIX](../MERIVUS_SWARM_TARGET_MATRIX.md) | 不新增整目标验收 |
| 文档 | [INDEX](../INDEX.md) | 当前交接索引 |
| 文档 | [交接README](README.md) | 阅读顺序 |
| 文档 | [ENGINEER_HANDOFF](MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 全部未执行反例 |
| 文档 | [PATCH_MANIFEST](MERIVUS_SWARM_PATCH_MANIFEST.md) | 准确20路径 |
| 文档 | [STATIC_AUDIT](MERIVUS_SWARM_STATIC_AUDIT.md) | 限定静态证据 |
| 文档 | [ACK_IDENTITY_AUDIT](MERIVUS_ACK_IDENTITY_AUDIT.md) | 历史导入不恢复飞行授权 |
| 文档 | [Worker说明](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md) | 暂存／发布／确认分层 |
| 文档 | [Store说明](../../custom/src/Swarm/FleetPersistentEventStore.md) | 私有来源与工单生命周期 |
| 文档 | [面板说明](../../custom/src/Swarm/FleetArchiveMaintenancePanel.md) | 选择／确认及显示边界 |

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 465 准确本批范围：6源码／14文档，共20路径

本表仅列当前465实际负责路径，不是全脏树差异，也不重复计入464的Inventory类、资源登记或其他历史模块。未新增测试文件、编译产物、恢复工具、PR提交或交付包。

| 类别 | 路径 | 本批作用 |
|---|---|---|
| 源码 | [Worker.h](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h) | 新诊断私有入口 |
| 源码 | [Worker.cc](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc) | raw候选分类、23／row4契约、全局取消与尾核 |
| 源码 | [Store.h](../../custom/src/Swarm/FleetPersistentEventStore.h) | 只读诊断属性、信号和存储 |
| 源码 | [Store.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc) | 独立action7→request2、public26／row5与资格退休 |
| 源码 | [GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 窄只读属性和明确第七路径字段 |
| 源码 | [FleetArchiveMaintenancePanel.qml](../../custom/res/Merivus/FleetArchiveMaintenancePanel.qml) | 原结果完整接纳、逐项原因、路径ABA和安全显示 |
| 文档 | [README](../../README.md) | 当前交付入口 |
| 文档 | [MODIFIED_FILES](../../MODIFIED_FILES.md) | 本批改动说明 |
| 文档 | [PROJECT_STATUS](../PROJECT_STATUS.md) | 进展与剩余边界 |
| 文档 | [ROADMAP](../MERIVUS_FLEET_OS_ROADMAP.md) | 原25目标范围保持 |
| 文档 | [TARGET_MATRIX](../MERIVUS_SWARM_TARGET_MATRIX.md) | 不新增整目标验收 |
| 文档 | [INDEX](../INDEX.md) | 模块与交接入口 |
| 文档 | [交接README](README.md) | 交接阅读顺序 |
| 文档 | [ENGINEER_HANDOFF](MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 全部未执行验证项 |
| 文档 | [PATCH_MANIFEST](MERIVUS_SWARM_PATCH_MANIFEST.md) | 准确20路径 |
| 文档 | [STATIC_AUDIT](MERIVUS_SWARM_STATIC_AUDIT.md) | 限定静态证据 |
| 文档 | [ACK_IDENTITY_AUDIT](MERIVUS_ACK_IDENTITY_AUDIT.md) | 不涉及飞行ACK或权限恢复 |
| 文档 | [Worker说明](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md) | 23／row4和失败分类 |
| 文档 | [Store说明](../../custom/src/Swarm/FleetPersistentEventStore.md) | public26／row5与actualfinished |
| 文档 | [面板说明](../../custom/src/Swarm/FleetArchiveMaintenancePanel.md) | 完整显示和未验证UI边界 |

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际文件hash／备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 464：独立备份清点与实际只读消费的准确范围

本批8源码：

- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h)
- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc)
- [custom/src/Swarm/FleetArchiveDirectoryInventory.h](../../custom/src/Swarm/FleetArchiveDirectoryInventory.h)
- [custom/src/Swarm/FleetArchiveDirectoryInventory.cc](../../custom/src/Swarm/FleetArchiveDirectoryInventory.cc)
- [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h)
- [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetArchiveMaintenancePanel.qml](../../custom/res/Merivus/FleetArchiveMaintenancePanel.qml)

本批15文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)
- [custom/src/Swarm/FleetArchiveDirectoryInventory.md](../../custom/src/Swarm/FleetArchiveDirectoryInventory.md)
- [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md)
- [custom/src/Swarm/FleetArchiveMaintenancePanel.md](../../custom/src/Swarm/FleetArchiveMaintenancePanel.md)

合计23路径。复用原已登记类、资源和维护单槽，不新建数据库／SQL schema／Codec，不改PX4、MAVLink、Vehicle、命令／任务／审批或飞行权限。全脏树及历史文件数不是本批规模，不创建／更新PR、源码包或安装包。

实际链为Guided的backup-catalog七字符串intent → Store独立无库存分支／新query UUID → Worker request2 → EventBackups原生全枚举及每文件完整SHA／SQLite身份／二次SHA → 完整result21 → Store新只读catalog24／entry9 → 原线程actualfinished后UI逐项对应显示。来源是明确目录与实际Worker，不从public DTO、任意源SQL或旧copy结果恢复。

目标directory只在后台访问，原归档路径不必可用，读取不接触原源锁／当前库，也不自动mkdir／复制／改名／清理。固定目标合作锁可能创建／释放，不是全目录零写。坏末项、partial、sidecars、非法名字、EOF／close／目录变化、超限／取消整次失败，不把早期有效条目当清单成功。

新catalog格式独立，originalSourceVerified／backupSetCompletenessVerified固定false；本次测得SHA不是已丢失原源的hash证明，文件名第三UUID不是作者认证。完整枚举不代表备份全集／任务结果／完整遥测或恢复权限。旧inventory19、move27、backup31及其current19／23保持；本批没有导入／恢复消费者。

UI路径按下／click及生命周期版本防同值恢复，已发工单按原目录接纳结果，旧catalog只历史阅读；与原source库存展示分开，列表128有界虚拟化。技能用于明确独立读取、文字语义、错误／取消／旧值、主题和键盘焦点，不执行技能helper或UI。未执行验证见[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)，限定结论见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或实际备份／读回／恢复，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。


## 463：单段显式备份与完整读回的准确范围

本批7源码：

- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h)
- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc)
- [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h)
- [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetArchiveMaintenancePanel.qml](../../custom/res/Merivus/FleetArchiveMaintenancePanel.qml)
- [custom/res/Merivus/FleetArchiveHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveHistoryPanel.qml)

本批15文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)
- [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md)
- [custom/src/Swarm/FleetArchiveMaintenancePanel.md](../../custom/src/Swarm/FleetArchiveMaintenancePanel.md)
- [custom/src/Swarm/FleetArchiveHistoryPanel.md](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)

合计22路径。扩展原已登记MaintenanceWorker、Store及维护面板，Guided增加限定新动作，原历史折叠按钮明示“备份”；不新增类、资源、SQL schema／Codec或新数据库。没有改PX4、Vehicle、MAVLink、命令事务、任务执行、审批或飞行权限；全脏树／历史文件数不等于本批范围。

旧maintenance intent6及inventory／move／current19／27保持；仅prepare-backup intent7多backupDirectory，backup预览schema2 exact26。commit旧6只能用原private路径和原preview UUID生成Worker backup5，完整目标名在预览已明确；不接任意源路径或目标文件名，不在GUI扫描／hash／SQL或建目录。原30秒仅提交窗口，原实际维护QThread::finished才退槽。

Worker新备份result31/current23与旧移动facts分开。固定源owner锁及目标合作锁、原真实库存／全文件SHA／SQLite身份、目录对象和source descriptor重核；新UUID文件通过QFile NewOnly无覆盖创建，64KiB有界复制且处理短写，完整后重开目标核SHA与只读SQL身份，再核原源。原文件不移动或删除；原封存段扫描上限／32段恢复政策不作为备份数量放宽或上限。

决定／创建尝试／可能写入、目标已创建、复制字节、copyCompleted和backupVerified分别保留。copyCompleted只表示全部字节接纳与流hash匹配，可仍伴flush／close错误，不是已核对或持久化；晚到取消不丢已完成写入事实。失败／取消可能留下partial，不删除、覆盖、自动重试、修复或回滚。读回成功也不证明全部事件真实／签名、完整历史、独立介质或灾后恢复。

UI路径使用独立单调版本，旧备份预览／确认不能因改回同值复活，原移出／恢复不受无关路径编辑影响；已发备份原结果也不因之后路径改变而丢失。技能用于明确已有目录／完整目标、影响和独立确认、文字状态、焦点与主题复用；不运行搜索脚本或UI。工程师未执行项见[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)，限定核对见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。

本批仅修改地面站源码及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／归档扫描或实际备份／改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 462：单文件可恢复归档维护的准确范围

本批13源码／工程及资源登记：

- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.h)
- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.cc)
- [custom/src/Swarm/FleetArchiveDirectoryInventory.h](../../custom/src/Swarm/FleetArchiveDirectoryInventory.h)
- [custom/src/Swarm/FleetArchiveDirectoryInventory.cc](../../custom/src/Swarm/FleetArchiveDirectoryInventory.cc)
- [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h)
- [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc)
- [custom/custom.pri](../../custom/custom.pri)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetArchiveMaintenancePanel.qml](../../custom/res/Merivus/FleetArchiveMaintenancePanel.qml)
- [custom/res/Merivus/FleetArchiveHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveHistoryPanel.qml)
- [custom/res/Merivus/FleetArchiveTaskHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveTaskHistoryPanel.qml)
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)

本批17文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md](../../custom/src/Swarm/FleetEventArchiveMaintenanceWorker.md)
- [custom/src/Swarm/FleetArchiveDirectoryInventory.md](../../custom/src/Swarm/FleetArchiveDirectoryInventory.md)
- [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md)
- [custom/src/Swarm/FleetArchiveMaintenancePanel.md](../../custom/src/Swarm/FleetArchiveMaintenancePanel.md)
- [custom/src/Swarm/FleetArchiveHistoryPanel.md](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)
- [custom/src/Swarm/FleetArchiveTaskHistoryPanel.md](../../custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)

合计30路径。新增维护Worker h／cc／md及维护面板／说明；复用原Store、目录清点和历史面板。普通历史只查sealed命名空间；retired保留实际文件，退役不释放磁盘，恢复只恢复可检索性。当前录制库、SQL schema／Codec、PX4、MAVLink、Vehicle、任务编排、飞行命令及批准权限不在本批改写范围。全脏树及历史文件数不是本批改动量。

实际链为：Guided窄facade六字符串intent → Store原生库存／entry／单次30秒预览 → parentless单文件Worker → 原生完整结果 → 实际finished后UI接纳。固定目录与同一owner锁、全文件SHA前后核、SQLite头／实际epoch／seal run核对和完整目录枚举共同保留；不将accepted当成功，不根据取消、系统调用失败或结果缺失猜“文件没变”。

Windows／Linux走原生无覆盖改名，不使用Qt改名的复制／删除回退；不支持明确拒绝。决定后保留mayHaveMoved并继续后置观察，不自动重试、覆盖、回滚、修库或永久删除。库存最多128 sealed／128 retired，恢复按实际产品32段限额；不是自动留存清理、外部备份或灾后恢复完成。

内部Worker请求exact1／5，库存19／移动27／状态19；Store公开库存22／entry8、预览25、结果20。inventory／commit先退休原历史页、目录、search和任务两类真实续扫上下文。所有原生入口独立维护互斥，维护槽不因resultReady或取消提前复用。旧面板只补维护busy，原461按任务筛选与预算语义保留。

UI/UX技能用于现有QGC主题／文字状态／焦点与按需披露，以及“读取清单→影响预览→独立确认→等待实际结果”；未运行UI或技能搜索脚本。完整检查结论以[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)为准，未执行场景见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批不提交或更新PR、源码包或安装包。

本批仅修改地面站源码、工程／资源登记及交接材料；核对仅直接静态阅读与普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／目录扫描或真实文件改名，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持；整体约40%仍只是源码工作量粗估，不按批次／文件数提高。下方为历史。

## 461：留存任务目录与原记录查询的准确范围

本批9源码／资源登记：

- [custom/src/Swarm/FleetEventArchiveHistoryReader.h](../../custom/src/Swarm/FleetEventArchiveHistoryReader.h)
- [custom/src/Swarm/FleetEventArchiveHistoryReader.cc](../../custom/src/Swarm/FleetEventArchiveHistoryReader.cc)
- [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h)
- [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetArchiveHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveHistoryPanel.qml)
- [custom/res/Merivus/FleetArchiveTaskHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveTaskHistoryPanel.qml)
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)

本批15文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetEventArchiveHistoryReader.md](../../custom/src/Swarm/FleetEventArchiveHistoryReader.md)
- [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md)
- [custom/src/Swarm/FleetArchiveHistoryPanel.md](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)
- [custom/src/Swarm/FleetArchiveTaskHistoryPanel.md](../../custom/src/Swarm/FleetArchiveTaskHistoryPanel.md)

合计24路径。新增FleetArchiveTaskHistoryPanel.qml／同名契约文档，登记原qrc／qmldir；复用原Reader／Store，不新建数据库或修改SQL／Codec／PX4、MAVLink、Vehicle、命令事务、审批或飞行许可。本批不生成或更新PR、源码包、安装包。全脏树／历史文件数不等于本批范围或整目标验收数。

Reader旧request5→schema1 result28不变，明确提供taskIdHash才是request6→schema2 result29。原Store新增目录35字段／task row9、原记录31字段，目录最多256个任务哈希／8192累计扫描行；按任务查记录也保独立累计扫描预算。全部实际search批次核完整契约后才合并，预算不足不接纳行前缀。固定目录／policy、原清单／文件／行号和实际线程结束继续核对。

Guided只增加两只读值与tasks六字符串入口；目录／原记录与旧search不混游标，显式首次／继续／取消。UI技能用于主题复用、清楚文字状态、焦点和有界列表。所谓completed只表示本次保留范围末尾，不是任务执行完成；部分／预算／取消／失败不变完整历史。旧summary未留存的完成状态／成员／能耗不补造，NDJSON runId不强制JOIN。

本批仅地面站源码、资源登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持；整体约40%仍仅源码工作量粗估，不按批次／文件数提高。下方为历史。

## 460：原生电池负面到能量／健康／末端尾栅的准确范围

本批11源码／工程登记：

- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)
- [src/Vehicle/FleetStartupMemorySeal.cc](../../src/Vehicle/FleetStartupMemorySeal.cc)
- [custom/src/Swarm/FleetNativeBatteryEvidence.h](../../custom/src/Swarm/FleetNativeBatteryEvidence.h)
- [custom/src/Swarm/FleetNativeBatteryEvidence.cc](../../custom/src/Swarm/FleetNativeBatteryEvidence.cc)
- [custom/src/Swarm/FleetEnergyScheduler.h](../../custom/src/Swarm/FleetEnergyScheduler.h)
- [custom/src/Swarm/FleetEnergyScheduler.cc](../../custom/src/Swarm/FleetEnergyScheduler.cc)
- [custom/src/Swarm/FleetRegistry.cc](../../custom/src/Swarm/FleetRegistry.cc)
- [custom/src/Swarm/FleetHealthCenter.cc](../../custom/src/Swarm/FleetHealthCenter.cc)
- [custom/src/Swarm/SwarmController.cc](../../custom/src/Swarm/SwarmController.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/custom.pri](../../custom/custom.pri)

本批19文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../INDEX.md)
- [docs/handoff/README.md](README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetNativeBatteryObservation.md](../../custom/src/Swarm/FleetNativeBatteryObservation.md)
- [custom/src/Swarm/FleetNativeBatteryEvidence.md](../../custom/src/Swarm/FleetNativeBatteryEvidence.md)
- [custom/src/Swarm/FleetEnergyScheduler.md](../../custom/src/Swarm/FleetEnergyScheduler.md)
- [custom/src/Swarm/FleetRegistry.md](../../custom/src/Swarm/FleetRegistry.md)
- [custom/src/Swarm/FleetHealthCenter.md](../../custom/src/Swarm/FleetHealthCenter.md)
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [custom/src/Swarm/FleetMissionRiskEvaluator.md](../../custom/src/Swarm/FleetMissionRiskEvaluator.md)
- [src/Vehicle/FleetStartupMemorySeal.md](../../src/Vehicle/FleetStartupMemorySeal.md)

合计30路径。新增FleetNativeBatteryEvidence三文件并登记custom.pri；Vehicle.h与Seal.h本批未改，未改PX4／MAVLink生成头、原公开命令／权限或生成安装包。RiskEvaluator与ReviewCoordinator本批只同步文档，原源码既有blocking链继续消费。全脏树／历史变更及新增类数不是本批范围或原25目标验收数。

原native outer20升schema2／row19，原value10完整记录charge／fault；统一完整解析替代Scheduler旧重复实现。Registry以valid保合法负面，HealthCenter在FTC前核原aliases、后生成check4并独立并入阻断ID；Swarm保合法源，Guided固定健康内容／fresh。原phase seal补typed电池账本、原owner／weak及capture时fresh行的原期限，不从新包续旧依据。没有新增命令或物理认证；schema1／缺扩展未知不一律当故障。

本批仅地面站源码、工程登记及交接材料；核对限直接静态阅读和普通文本／路径／Markdown目标／git空白。未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标与六类实质开发保持，整体约40%仍仅源码工作量粗估，不按本批文件数上调。 下方为历史。

## 459：原生地理运动模型一次绑定／阶段复用的准确范围

本批5源码：

- [custom/src/Swarm/FleetMissionStartupClearanceWitness.h](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.h)
- [custom/src/Swarm/FleetMissionStartupClearanceWitness.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.cc)
- [custom/src/Swarm/FleetMissionStartupTransaction.h](../../custom/src/Swarm/FleetMissionStartupTransaction.h)
- [custom/src/Swarm/FleetMissionStartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc)
- [custom/src/Swarm/FleetMissionStartupTransition.cc](../../custom/src/Swarm/FleetMissionStartupTransition.cc)

本批16文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../INDEX.md)
- [docs/handoff/README.md](README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionStartupClearanceWitness.md](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)
- [custom/src/Swarm/FleetMissionStartupTransaction.md](../../custom/src/Swarm/FleetMissionStartupTransaction.md)
- [custom/src/Swarm/FleetMissionStartupTransition.md](../../custom/src/Swarm/FleetMissionStartupTransition.md)
- [custom/src/Swarm/FleetMissionStartupGeographicMotion.md](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.md](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)

合计21路径，不计整个脏工作树或原25目标验收数。本批没有新源码文件／工程登记／公开DTO／策略schema，没有修改Vehicle、PX4、权限或原RAW／收据合同。GeographicMotion和Worker仅同步文档，其源码本批未改。

Witness原工厂逐source12缓存不可变模型；Txn新请求从原beforeArm及完整原载荷／策略／上限一次prepare，有资料须与原缓存完整source相同并持同Ptr，无资料持本次Ptr；Transition只取实际owner缓存，无phase重建或失败回退。既有源指纹已经包含GPI，不能把本优化计作已证实重基漏洞修复。原时限／UTC／权限与phase seal保持，完整域／最终StartPermit及正式结果仍未完成。

本批仅地面站源码与交接文档；核对限直接静态阅读、普通文本／路径／Markdown目标和git空白。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质工作保持，整体约40%仍仅源码工作量粗估，不按文件／批次上调。 下方为历史。

## 458：共享实际地理运动模型与冻结审阅输入的准确范围

本批8源码／工程登记：

- [custom/src/Swarm/FleetMissionStartupGeographicMotion.h](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.h)
- [custom/src/Swarm/FleetMissionStartupGeographicMotion.cc](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.cc)
- [custom/src/Swarm/FleetMissionStartupTransition.cc](../../custom/src/Swarm/FleetMissionStartupTransition.cc)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.h](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.h)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.cc)
- [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [custom/custom.pri](../../custom/custom.pri)

本批18文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../INDEX.md)
- [docs/handoff/README.md](README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionStartupGeographicMotion.md](../../custom/src/Swarm/FleetMissionStartupGeographicMotion.md)
- [custom/src/Swarm/FleetMissionStartupTransition.md](../../custom/src/Swarm/FleetMissionStartupTransition.md)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.md](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)
- [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)
- [custom/src/Swarm/FleetMissionStartupClearanceWitness.md](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)
- [custom/src/Swarm/FleetMissionJointSpatialReview.md](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)

合计26路径，不是整个脏工作树、原25目标完成数或所有历史增删。新增GeographicMotion三文件；Transition复用一次准备模型，Worker及两类Review接必需source12、input2 row3与完整原计划内容配对，custom.pri登记。Witness与Joint本批仅同步文档，其源码接口未改；未改Vehicle／Txn／PX4、生成补丁包或更新PR。

普通策略取源确定错配已修：同原member提前规范化reviewedProfile，模型与原审阅行共用reviewedPolicy；不把仅captureForStartup输出的policy当普通capture字段。地面原completed READ风险／payload不重采。名义环境报告不扩大为全运动域，numericContainmentProven及认证／释放false，Compiler pending、完整StartPermit和正式结果仍未完成。

本批仅修改地面站源码、工程登记及交接材料；只做直接静态阅读和普通文本／路径／Markdown目标／git空白核对。未编译、执行helper／parser／AST／业务脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批文件／类／记录数量上调。 下方为历史。

## 457：普通RAW与整组阶段memory栅的准确范围

本批10源码／工程登记：

- [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h)
- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)
- [src/Vehicle/FleetStartupMemorySeal.h](../../src/Vehicle/FleetStartupMemorySeal.h)
- [src/Vehicle/FleetStartupMemorySeal.cc](../../src/Vehicle/FleetStartupMemorySeal.cc)
- [custom/src/Swarm/FleetMissionStartupTransaction.h](../../custom/src/Swarm/FleetMissionStartupTransaction.h)
- [custom/src/Swarm/FleetMissionStartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc)
- [custom/src/Swarm/FleetMissionExecutionProfile.h](../../custom/src/Swarm/FleetMissionExecutionProfile.h)
- [custom/src/Swarm/FleetMissionExecutionProfile.cc](../../custom/src/Swarm/FleetMissionExecutionProfile.cc)
- [qgroundcontrol.pro](../../qgroundcontrol.pro)
- [src/Vehicle/CMakeLists.txt](../../src/Vehicle/CMakeLists.txt)

本批17文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [src/Vehicle/FleetStartupMemorySeal.md](../../src/Vehicle/FleetStartupMemorySeal.md)
- [src/Vehicle/VehicleStartupRawIngress.md](../../src/Vehicle/VehicleStartupRawIngress.md)
- [custom/src/Swarm/FleetMissionStartupTransaction.md](../../custom/src/Swarm/FleetMissionStartupTransaction.md)
- [custom/src/Swarm/FleetMissionStartupTransition.md](../../custom/src/Swarm/FleetMissionStartupTransition.md)
- [custom/src/Swarm/FleetMissionStartupClearanceWitness.md](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)
- [custom/src/Swarm/FleetMissionExecutionProfile.md](../../custom/src/Swarm/FleetMissionExecutionProfile.md)

合计27路径。新增Seal三文件及RAW说明；Vehicle与Txn为实际消费链，ExecutionProfile只抽原共享纯政策校验入口，qmake／CMake登记新Seal。不把全部脏工作树或历史变更计成本批。原step33和公开DTO保持，没有安装包／二进制／PR。

本批修首回调原包归属及跨机阶段尾核，不证明整个允许运动域的已审空间包含；Compiler pending、完整StartPermit和正式机载结果仍未完成。本批仅地面站源码、工程登记及交接材料，核对仅直接静态阅读、普通文本／路径／Markdown目标和git空白；未编译、运行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按批次／文件数上调。 下方456为历史。

## 456：共享原生Witness的准确文件范围

本批13源码／工程登记：

- [custom/src/Swarm/FleetMissionStartupClearanceWitness.h](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.h)
- [custom/src/Swarm/FleetMissionStartupClearanceWitness.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.cc)
- [custom/src/Swarm/FleetMissionStartReview.h](../../custom/src/Swarm/FleetMissionStartReview.h)
- [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc)
- [custom/src/Swarm/FleetMissionArmedGroundReview.h](../../custom/src/Swarm/FleetMissionArmedGroundReview.h)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [custom/src/Swarm/FleetMissionStartupAuthority.h](../../custom/src/Swarm/FleetMissionStartupAuthority.h)
- [custom/src/Swarm/FleetMissionStartupAuthority.cc](../../custom/src/Swarm/FleetMissionStartupAuthority.cc)
- [custom/src/Swarm/FleetMissionGroundStartupAuthority.h](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.h)
- [custom/src/Swarm/FleetMissionGroundStartupAuthority.cc](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.cc)
- [custom/src/Swarm/FleetMissionStartupTransaction.h](../../custom/src/Swarm/FleetMissionStartupTransaction.h)
- [custom/src/Swarm/FleetMissionStartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc)
- [custom/custom.pri](../../custom/custom.pri)

本批19文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionStartupClearanceWitness.md](../../custom/src/Swarm/FleetMissionStartupClearanceWitness.md)
- [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)
- [custom/src/Swarm/FleetMissionStartupAuthority.md](../../custom/src/Swarm/FleetMissionStartupAuthority.md)
- [custom/src/Swarm/FleetMissionGroundStartupAuthority.md](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)
- [custom/src/Swarm/FleetMissionStartupTransaction.md](../../custom/src/Swarm/FleetMissionStartupTransaction.md)
- [custom/src/Swarm/FleetMissionClearanceSource.md](../../custom/src/Swarm/FleetMissionClearanceSource.md)
- [custom/src/Swarm/FleetMissionClearanceDatum.md](../../custom/src/Swarm/FleetMissionClearanceDatum.md)

合计32路径，新增Witness三文件；其余既有增量。不把整个脏工作树或过去Vehicle改动算进本批；Vehicle已有最终commandContextCurrent钩子通过Txn消费新全组memory，本批未改Vehicle文件、没有二进制／PR／安装包。实际Review→两类Authority→Txn共享原对象、原时钟及原UTC水位；原pending保持，运动包络包含／普通RAW首回调锁存／全组motion末尾栅与完整StartPermit／正式结果继续。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方455为历史。

## 455：启动后台报告消费的准确文件范围

本批11源码／登记：

- [custom/src/Swarm/FleetMissionJointSpatialReview.h](../../custom/src/Swarm/FleetMissionJointSpatialReview.h)
- [custom/src/Swarm/FleetMissionJointSpatialReview.cc](../../custom/src/Swarm/FleetMissionJointSpatialReview.cc)
- [custom/src/Swarm/FleetMissionClearanceReview.h](../../custom/src/Swarm/FleetMissionClearanceReview.h)
- [custom/src/Swarm/FleetMissionClearanceReview.cc](../../custom/src/Swarm/FleetMissionClearanceReview.cc)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.h](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.h)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.cc](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.cc)
- [custom/src/Swarm/FleetMissionStartReview.h](../../custom/src/Swarm/FleetMissionStartReview.h)
- [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc)
- [custom/src/Swarm/FleetMissionArmedGroundReview.h](../../custom/src/Swarm/FleetMissionArmedGroundReview.h)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [custom/custom.pri](../../custom/custom.pri)

本批18文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionJointSpatialReview.md](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)
- [custom/src/Swarm/FleetMissionClearanceReview.md](../../custom/src/Swarm/FleetMissionClearanceReview.md)
- [custom/src/Swarm/FleetMissionStartupClearanceWorker.md](../../custom/src/Swarm/FleetMissionStartupClearanceWorker.md)
- [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)
- [custom/src/Swarm/FleetMissionClearanceSource.md](../../custom/src/Swarm/FleetMissionClearanceSource.md)
- [custom/src/Swarm/FleetMissionClearanceDatum.md](../../custom/src/Swarm/FleetMissionClearanceDatum.md)

合计29路径，新Worker三文件，其余既有增量，无二进制／PR／安装包。包括真实Review消费者，不只有独立helper；但两Authority／Transaction到最后write与已审运动范围仍未完成，原pending保持。[限定审计](MERIVUS_SWARM_STATIC_AUDIT.md)与[交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)采用同一边界。本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方454为历史。

## 454：工程高程对应进入原生审阅来源

本批11源码／工程登记：

- [custom/src/Swarm/FleetFirmwareAdapterCatalog.h](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.h)
- [custom/src/Swarm/FleetFirmwareAdapterCatalog.cc](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.cc)
- [custom/src/Swarm/FleetMissionClearanceDatum.h](../../custom/src/Swarm/FleetMissionClearanceDatum.h)
- [custom/src/Swarm/FleetMissionClearanceDatum.cc](../../custom/src/Swarm/FleetMissionClearanceDatum.cc)
- [custom/src/Swarm/FleetMissionStartReview.h](../../custom/src/Swarm/FleetMissionStartReview.h)
- [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc)
- [custom/src/Swarm/FleetMissionArmedGroundReview.h](../../custom/src/Swarm/FleetMissionArmedGroundReview.h)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h)
- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)
- [custom/custom.pri](../../custom/custom.pri)

本批17文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetFirmwareAdapterCatalog.md](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)
- [custom/src/Swarm/FleetMissionClearanceDatum.md](../../custom/src/Swarm/FleetMissionClearanceDatum.md)
- [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)
- [custom/src/Swarm/FleetMissionClearanceSource.md](../../custom/src/Swarm/FleetMissionClearanceSource.md)
- [custom/src/Swarm/FleetFirmwareCatalogReload.md](../../custom/src/Swarm/FleetFirmwareCatalogReload.md)

合计28准确路径，不把整个累计dirty diff当本批。新Datum三文件，其余既有增量；没有新命令、二进制、PR或安装包。原目录1兼容，明确目录2关系、native receipt形状修正、catalog缓存和Vehicle纯内存版本／非拥有PRIMARY读取由真实审阅消费。Datum原生UTC下界与最终观察输出接两Review水位，不改变exact5／14或稳定hash，不更新原until。物理高程／当前全球分支不认证，原pending与最终启动到write／正式结果仍须开发。

[限定审计](MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)保持同一边界；原25目标及六类实质剩余不缩减。仅地面站源码及限定静态核对；未编译、运行helper／解析器／脚本／UI／认证／SQL／应用／设备或仿真，未测试、改PX4、刷机、放行、提交、更新PR或打包。 以下453为历史。

## 453：当前实际构型来源与资料时效的审阅消费

本批15源码／工程登记：

- [custom/src/Swarm/FleetRegistry.h](../../custom/src/Swarm/FleetRegistry.h)
- [custom/src/Swarm/FleetRegistry.cc](../../custom/src/Swarm/FleetRegistry.cc)
- [custom/src/Swarm/SwarmController.cc](../../custom/src/Swarm/SwarmController.cc)
- [custom/src/Swarm/FleetNativeConfigurationObservation.h](../../custom/src/Swarm/FleetNativeConfigurationObservation.h)
- [custom/src/Swarm/FleetNativeConfigurationObservation.cc](../../custom/src/Swarm/FleetNativeConfigurationObservation.cc)
- [custom/src/Swarm/FleetMissionClearanceSource.h](../../custom/src/Swarm/FleetMissionClearanceSource.h)
- [custom/src/Swarm/FleetMissionClearanceSource.cc](../../custom/src/Swarm/FleetMissionClearanceSource.cc)
- [custom/src/Swarm/FleetMissionStartReview.h](../../custom/src/Swarm/FleetMissionStartReview.h)
- [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc)
- [custom/src/Swarm/FleetMissionArmedGroundReview.h](../../custom/src/Swarm/FleetMissionArmedGroundReview.h)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [custom/custom.pri](../../custom/custom.pri)
- [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h)
- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)
- [qgroundcontrol.pro](../../qgroundcontrol.pro)

本批16文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetRegistry.md](../../custom/src/Swarm/FleetRegistry.md)
- [custom/src/Swarm/FleetNativeConfigurationObservation.md](../../custom/src/Swarm/FleetNativeConfigurationObservation.md)
- [custom/src/Swarm/FleetMissionClearanceSource.md](../../custom/src/Swarm/FleetMissionClearanceSource.md)
- [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)

合计30准确路径，不把全部累计dirty diff当本批。不新增命令、批准绕过、二进制、PR或安装包。配置／时间检查进入两类实际审阅，但共同datum、真实启动净空报告、两Authority至最终write、最终StartPermit／正式结果及原六类仍有实质代码；原clearance pending保持。

只直接源码与限定文本核对，未编译、运行helper／脚本／解析器／UI／认证／应用／SQL／线程或设备、测试、改PX4、刷机、放行、提交、更新PR或打包。原25目标不缩减；[审计](MERIVUS_SWARM_STATIC_AUDIT.md)与[工程师待办](MERIVUS_SWARM_ENGINEER_HANDOFF.md)保持同一边界。以下452为历史。

## 452：明确接地输入、非对称扫掠与实际报告消费者

本批9源码／登记：

- [custom/src/Swarm/FleetMissionClearanceInput.h](../../custom/src/Swarm/FleetMissionClearanceInput.h)
- [custom/src/Swarm/FleetMissionClearanceInput.cc](../../custom/src/Swarm/FleetMissionClearanceInput.cc)
- [custom/src/Swarm/FleetMissionClearanceGeometry.h](../../custom/src/Swarm/FleetMissionClearanceGeometry.h)
- [custom/src/Swarm/FleetMissionClearanceGeometry.cc](../../custom/src/Swarm/FleetMissionClearanceGeometry.cc)
- [custom/src/Swarm/FleetMissionContactModel.h](../../custom/src/Swarm/FleetMissionContactModel.h)
- [custom/src/Swarm/FleetMissionContactModel.cc](../../custom/src/Swarm/FleetMissionContactModel.cc)
- [custom/src/Swarm/FleetMissionClearanceReview.h](../../custom/src/Swarm/FleetMissionClearanceReview.h)
- [custom/src/Swarm/FleetMissionClearanceReview.cc](../../custom/src/Swarm/FleetMissionClearanceReview.cc)
- [custom/custom.pri](../../custom/custom.pri)

本批18文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionClearanceInput.md](../../custom/src/Swarm/FleetMissionClearanceInput.md)
- [custom/src/Swarm/FleetMissionClearanceGeometry.md](../../custom/src/Swarm/FleetMissionClearanceGeometry.md)
- [custom/src/Swarm/FleetMissionContactModel.md](../../custom/src/Swarm/FleetMissionContactModel.md)
- [custom/src/Swarm/FleetMissionClearanceReview.md](../../custom/src/Swarm/FleetMissionClearanceReview.md)
- [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md)
- [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md)
- [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md)

共27条路径；新ContactModel h／cc／md，其余既有文件增量。Input内部版本2通过原Draft／Compiler／Store统一链保存，Review实际生成双组件报告3并严格匹配；不改PX4、原wire或原审批权限。纯值模型仍不认证真实来源／触地，原pending保持。完整源／文字限定审阅见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)，实操留[工程师](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、运行、测试、提交、更新PR或打包，原25目标继续；下方451为上一批。

## 451：取消传播、单任务Worker与三入口消费

本批15源码／登记文件：

- [custom/src/Swarm/FleetMissionClearanceGeometry.h](../../custom/src/Swarm/FleetMissionClearanceGeometry.h)
- [custom/src/Swarm/FleetMissionClearanceGeometry.cc](../../custom/src/Swarm/FleetMissionClearanceGeometry.cc)
- [custom/src/Swarm/FleetMissionClearanceReview.h](../../custom/src/Swarm/FleetMissionClearanceReview.h)
- [custom/src/Swarm/FleetMissionClearanceReview.cc](../../custom/src/Swarm/FleetMissionClearanceReview.cc)
- [custom/src/Swarm/FleetMissionCompiler.h](../../custom/src/Swarm/FleetMissionCompiler.h)
- [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc)
- [custom/src/Swarm/FleetMissionPlanningWorker.h](../../custom/src/Swarm/FleetMissionPlanningWorker.h)
- [custom/src/Swarm/FleetMissionPlanningWorker.cc](../../custom/src/Swarm/FleetMissionPlanningWorker.cc)
- [custom/src/Swarm/FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h)
- [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [custom/res/Merivus/MerivusAIAssistantPanel.qml](../../custom/res/Merivus/MerivusAIAssistantPanel.qml)
- [custom/res/Merivus/FleetMissionRemainderPanel.qml](../../custom/res/Merivus/FleetMissionRemainderPanel.qml)
- [custom/custom.pri](../../custom/custom.pri)

本批17文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionClearanceGeometry.md](../../custom/src/Swarm/FleetMissionClearanceGeometry.md)
- [custom/src/Swarm/FleetMissionClearanceReview.md](../../custom/src/Swarm/FleetMissionClearanceReview.md)
- [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md)
- [custom/src/Swarm/FleetMissionPlanningWorker.md](../../custom/src/Swarm/FleetMissionPlanningWorker.md)
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [custom/src/Swarm/FleetMissionRemainderPanel.md](../../custom/src/Swarm/FleetMissionRemainderPanel.md)

共32路径。新增Worker h／cc／md；其余为既有文件增量。不是累计dirty文件数，不提供虚构行数或完成计数。完整可取消编译运行在纯值worker，实际原生源保GUI；Root各来源／代次与归档CAS保留。取消、坏源及陈旧完成不释放命令，也不回滚既有归档事实。

限定静态阅读与直接文本／路径／空白核对见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)，未编译、运行或测试。原25目标／六类实质工作保持；没有提交、更新PR或打包。以下450为上一批。

## 450：规划净空计算、独立几何basis和归档同源

本批9源码／17文档，共26路径，无新文件／工程登记／UI／资源／QML类型／飞行入口。不用累计dirty diff、文件或批次计完成率。

### 源码（9）

- [FleetMissionJointSpatialReview.h](../../custom/src/Swarm/FleetMissionJointSpatialReview.h)：声明明确规划scope和独立basis入口。
- [FleetMissionJointSpatialReview.cc](../../custom/src/Swarm/FleetMissionJointSpatialReview.cc)：复用完整编码／来源提取，不虚构启动；来源重建不跑旧checker。
- [FleetMissionStartupSpatialReview.h](../../custom/src/Swarm/FleetMissionStartupSpatialReview.h)：独立geometryBasis入口，旧显示API保持。
- [FleetMissionStartupSpatialReview.cc](../../custom/src/Swarm/FleetMissionStartupSpatialReview.cc)：复用同次输入投影而不借最终reviewFingerprint。
- [FleetMissionClearanceReview.h](../../custom/src/Swarm/FleetMissionClearanceReview.h)：schema2求解和规划结果源匹配接口。
- [FleetMissionClearanceReview.cc](../../custom/src/Swarm/FleetMissionClearanceReview.cc)：明确scope／basis，复用完整环境计算，严格规划来源匹配。
- [FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc)：真实规划求解与完整结果冻结，失败增加具体阻断，原pending保持。
- [FleetMissionPlanStore.cc](../../custom/src/Swarm/FleetMissionPlanStore.cc)：新结果白名单、成功同源及失败精确诊断校验。
- [FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)：原样投影编译结果，原风险／空域门禁保持。

### 文档（17）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [FleetMissionJointSpatialReview.md](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)。
- [FleetMissionStartupSpatialReview.md](../../custom/src/Swarm/FleetMissionStartupSpatialReview.md)。
- [FleetMissionClearanceReview.md](../../custom/src/Swarm/FleetMissionClearanceReview.md)。
- [FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md)。
- [FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md)。
- [FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)。

原pending和实际许可门槛不解除，完整native启动／接地／现实对应仍需开发；原25目标保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方449及更早为历史。

## 449：精确整段扫掠、闭区域并集与独立净空报告

本批5源码／工程＋15文档，共20路径。仅本批触及，不按累计dirty diff、文件数或批次计完成率，无UI／资源／QML类型／飞行入口。

### 源码／工程（5）

- [FleetMissionClearanceGeometry.h](../../custom/src/Swarm/FleetMissionClearanceGeometry.h)：有界平面、bounds、polytope、预算和三态API。
- [FleetMissionClearanceGeometry.cc](../../custom/src/Swarm/FleetMissionClearanceGeometry.cc)：精确256位有理顶点、strict可行性、整段box扫掠与完整闭并集差集。
- [FleetMissionClearanceReview.h](../../custom/src/Swarm/FleetMissionClearanceReview.h)：纯值声明模型报告接口。
- [FleetMissionClearanceReview.cc](../../custom/src/Swarm/FleetMissionClearanceReview.cc)：完整来源／投影／包络与覆盖、走廊、地形、障碍比较，显式未知和接地缺口。
- [custom/custom.pri](../../custom/custom.pri)：登记两个helper的头／源。

### 文档（15）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [FleetMissionClearanceGeometry.md](../../custom/src/Swarm/FleetMissionClearanceGeometry.md)。
- [FleetMissionClearanceReview.md](../../custom/src/Swarm/FleetMissionClearanceReview.md)。
- [FleetMissionClearanceInput.md](../../custom/src/Swarm/FleetMissionClearanceInput.md)。
- [FleetMissionJointSpatialReview.md](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)。

Compiler pending保持，实际规划／启动同源消费与接地上下文／最终许可未完成；原25目标继续。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方448及更早为历史。

## 448：工程场地／逐机包络资料与来源版本接线

本批8源码／工程登记、17文档，共25路径。新增FleetMissionClearanceInput的h／cc／md，无新QML类型／资源或飞行入口。不以累计dirty diff、文件或批次推导完成率，原25目标保持。

### 源码／工程（8）

- [FleetMissionClearanceInput.h](../../custom/src/Swarm/FleetMissionClearanceInput.h)：新增纯normalize／inspect接口。
- [FleetMissionClearanceInput.cc](../../custom/src/Swarm/FleetMissionClearanceInput.cc)：完整版本资料、毫米凸多边形及同序主机检查。
- [FleetMissionDraft.h](../../custom/src/Swarm/FleetMissionDraft.h)：声明原生CAS设置／清除与来源1／2边界。
- [FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc)：settings11／12、完整来源保存／恢复和实际build约束。
- [FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc)：原坏约束保留、身份对应与实际pending风险。
- [FleetMissionPlanStore.cc](../../custom/src/Swarm/FleetMissionPlanStore.cc)：独立完整规范值、原source与plan同资料验证。
- [GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)：两处恢复的来源版本／字段配对预检，不改变布局。
- [custom/custom.pri](../../custom/custom.pri)：登记新helper头／源。

### 文档（17）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [custom/src/Swarm/FleetMissionClearanceInput.md](../../custom/src/Swarm/FleetMissionClearanceInput.md)。
- [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md)。
- [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md)。
- [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md)。
- [custom/src/Swarm/FleetMissionArchiveDraftPanel.md](../../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)。
- [custom/src/Swarm/FleetMissionRepairDraft.md](../../custom/src/Swarm/FleetMissionRepairDraft.md)。

新声明合法仍几何待审阻断，未完成solver／实际启动消费或最终许可；旧无资料来源不补默认。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方447及更早为历史。

## 447：业务阻断风险的生成、审阅与原生消费

本批6源码、16文档，共22路径，无新文件／QML类型／资源或工程登记。只列本批触及，不用累计dirty diff、文件或批次推导完成率，原25目标保持。

### 源码（6）

- [FleetMissionRiskEvaluator.h](../../custom/src/Swarm/FleetMissionRiskEvaluator.h)：声明native-only纯static无阻断检查。
- [FleetMissionRiskEvaluator.cc](../../custom/src/Swarm/FleetMissionRiskEvaluator.cc)：严格空原因容器与inputValid消费，未命名严重风险统一有效code。
- [FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)：needsReview真正消费业务阻断。
- [FleetMissionDeployment.cc](../../custom/src/Swarm/FleetMissionDeployment.cc)：实际冻结评估与准确归档同内容、无阻断双检查。
- [FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc)：原记录绑定／hash后独立核review flags与风险。
- [FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)：准确原记录／evidenceHash及同一风险门槛覆盖新READ至审批上下文。

### 文档（16）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [custom/src/Swarm/FleetMissionRiskEvaluator.md](../../custom/src/Swarm/FleetMissionRiskEvaluator.md)。
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)。
- [custom/src/Swarm/FleetMissionDeployment.md](../../custom/src/Swarm/FleetMissionDeployment.md)。
- [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md)。
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)。

不混用Intent的独立Radar合同，不新增权限、命令或许可；blocked历史保只读，需要新原生审阅而不是修改旧标签。完整净空／最终StartPermit、正式结果及原25目标继续。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包，下方446及更早为历史。

## 446：原始高度操作数的精确残差／漂移比较

本批4源码／工程登记、15文档，共19路径；只列本批触及范围，不以累计dirty diff、批次或文件数推导完成率。原25目标保持。

### 源码／工程登记（4）

- [FleetMissionAltitudeArithmetic.h](../../custom/src/Swarm/FleetMissionAltitudeArithmetic.h)：新增1–4项有界精确线性组合接口。
- [FleetMissionAltitudeArithmetic.cc](../../custom/src/Swarm/FleetMissionAltitudeArithmetic.cc)：binary64原位分解、固定36个uint32 limb正负累加／绝对差及闭区间比较。
- [FleetMissionStartupAltitude.cc](../../custom/src/Swarm/FleetMissionStartupAltitude.cc)：三残差与三首次基准漂移改用原高度及原整数毫米，不先浮点相减或除法。
- [custom/custom.pri](../../custom/custom.pri)：登记新算术helper头／源。

### 文档（15）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [custom/src/Swarm/FleetMissionAltitudeArithmetic.md](../../custom/src/Swarm/FleetMissionAltitudeArithmetic.md)。
- [custom/src/Swarm/FleetMissionStartupAltitude.md](../../custom/src/Swarm/FleetMissionStartupAltitude.md)。
- [custom/src/Swarm/FleetMissionExecutionProfile.md](../../custom/src/Swarm/FleetMissionExecutionProfile.md)。
- [src/Vehicle/VehicleStartupAltitude.md](../../src/Vehicle/VehicleStartupAltitude.md)。

六项工程容差及其实际binary64值、reference20／sample20、v4字段和原生来源／粘性scope门槛保持。恰等于阈值接受，零容差只接受精确为零；不增加epsilon、默认余量或认证。新helper的h／cc／md为新增文件，既有高度cc／md为本批更新。

未编译、测试、运行helper／UI／应用、改PX4、刷机、提交、更新PR或打包；下方445及更早为历史。

## 445：显式高度关系容差、首次基准与原生scope监视

本批17源码／工程登记、24文档，共41路径。新增FleetMissionStartupAltitude的h／cc／md，复用既有策略组件，无新QML类型／资源、消息请求或PX4修改。仅列本批触及范围，不把累计dirty diff、文件或批次当完成率；原25目标保持。

### 源码／工程登记（17）

- [custom/src/Swarm/FleetMissionStartupAltitude.h](../../custom/src/Swarm/FleetMissionStartupAltitude.h)：新增纯关系契约接口，冻结稳定参考和首次样本。
- [custom/src/Swarm/FleetMissionStartupAltitude.cc](../../custom/src/Swarm/FleetMissionStartupAltitude.cc)：严格封套／原参考残差及相对原样本漂移。
- [custom/src/Swarm/FleetMissionExecutionProfile.h](../../custom/src/Swarm/FleetMissionExecutionProfile.h)：明确第四版策略与旧版本边界。
- [custom/src/Swarm/FleetMissionExecutionProfile.cc](../../custom/src/Swarm/FleetMissionExecutionProfile.cc)：严格v4／七字段高度包线规范化。
- [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h)：独立原生scope监视接口与固定大小状态。
- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)：原包粘性违规／gap、一次绑定与实际write前核验。
- [custom/src/Swarm/FleetMissionStartupTransaction.h](../../custom/src/Swarm/FleetMissionStartupTransaction.h)：原实际target基准只读访问器。
- [custom/src/Swarm/FleetMissionStartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc)：固定已阅基准取源及v4策略接线，不恢复历史权限。
- [custom/src/Swarm/FleetMissionStartReadiness.cc](../../custom/src/Swarm/FleetMissionStartReadiness.cc)：普通／阶段v4首次基准和尾部原生来源核对。
- [custom/src/Swarm/FleetMissionStartupTransition.cc](../../custom/src/Swarm/FleetMissionStartupTransition.cc)：同原基准容差与实际scope监视消费。
- [custom/src/Swarm/FleetMissionStartupAuthority.cc](../../custom/src/Swarm/FleetMissionStartupAuthority.cc)：当前值对原sample比较，稳定身份隔离动态值。
- [custom/src/Swarm/FleetMissionGroundStartupAuthority.cc](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.cc)：地面实际基准连续性及当前容差比较。
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)：地面READ／首次风险／当前风险保留原sample。
- [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc)：首次完整hash保留与当前容差复核。
- [custom/res/Merivus/FleetStartupPolicyEditor.qml](../../custom/res/Merivus/FleetStartupPolicyEditor.qml)：明确v4选择、六容差、完整预览与人工采用。
- [custom/res/Merivus/FleetMissionDeploymentPanel.qml](../../custom/res/Merivus/FleetMissionDeploymentPanel.qml)：既有冻结策略摘要支持v4与六容差，不新增动作。
- [custom/custom.pri](../../custom/custom.pri)：登记新Swarm helper头／源。

### 文档（24）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [custom/src/Swarm/FleetMissionStartupAltitude.md](../../custom/src/Swarm/FleetMissionStartupAltitude.md)。
- [custom/src/Swarm/FleetMissionExecutionProfile.md](../../custom/src/Swarm/FleetMissionExecutionProfile.md)。
- [src/Vehicle/VehicleStartupAltitude.md](../../src/Vehicle/VehicleStartupAltitude.md)。
- [custom/src/Swarm/FleetMissionStartupTransaction.md](../../custom/src/Swarm/FleetMissionStartupTransaction.md)。
- [custom/src/Swarm/FleetMissionStartReadiness.md](../../custom/src/Swarm/FleetMissionStartReadiness.md)。
- [custom/src/Swarm/FleetMissionStartupTransition.md](../../custom/src/Swarm/FleetMissionStartupTransition.md)。
- [custom/src/Swarm/FleetMissionStartupAuthority.md](../../custom/src/Swarm/FleetMissionStartupAuthority.md)。
- [custom/src/Swarm/FleetMissionGroundStartupAuthority.md](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)。
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)。
- [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md)。
- [custom/src/Swarm/FleetStartupPolicyEditor.md](../../custom/src/Swarm/FleetStartupPolicyEditor.md)。
- [custom/src/Swarm/FleetExecutionProfileEditor.md](../../custom/src/Swarm/FleetExecutionProfileEditor.md)。
- [custom/src/Swarm/FleetMissionStartupPanel.md](../../custom/src/Swarm/FleetMissionStartupPanel.md)。

v4沿v2 motion／heading与固定HOME，六项容差须明确给出；旧v1–v3和step33等合同保持。首次样本完整保留，当前比较不能仅删除动态值而跳过容差；scope内违规／断档恢复后仍保留失败。scope建立前无连续覆盖承诺，不认证内部参考／同采样／净空或最终许可。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包；下方444及更早为历史。


## 444：被动高度账本与冻结启动风险诊断

本批11源码／工程登记、18文档，共29路径。新增decoder的h／cc／md和Vehicle账本文档，无新QML组件、资源、Swarm类型或飞行入口。仅列本批触及范围，不以累计dirty diff／文件／批次计完成率；原25目标保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

### 源码／工程登记（11）

- [src/Vehicle/FleetStartupAltitude.h](../../src/Vehicle/FleetStartupAltitude.h)：新增被动消息141纯解码／状态投影接口。
- [src/Vehicle/FleetStartupAltitude.cc](../../src/Vehicle/FleetStartupAltitude.cc)：严格同包18字段、逐项原始位值／未知和三项报告差值。
- [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h)：独立高度观察账本和native27只读接口声明。
- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)：最早原包观察、来源与接收隔离、类别／时钟计数，不接原启动门槛。
- [custom/src/Swarm/FleetMissionStartReadiness.cc](../../custom/src/Swarm/FleetMissionStartReadiness.cc)：固定11字段冻结诊断及阶段指纹隔离。
- [custom/src/Swarm/FleetMissionStartReview.cc](../../custom/src/Swarm/FleetMissionStartReview.cc)：普通风险摘要与首次材料／当前风险身份分离。
- [custom/src/Swarm/FleetMissionStartupAuthority.cc](../../custom/src/Swarm/FleetMissionStartupAuthority.cc)：稳定阶段依据排除独立动态诊断。
- [custom/src/Swarm/FleetMissionGroundStartupAuthority.cc](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.cc)：稳定地面依据排除独立动态诊断。
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)：地面风险摘要及稳定依据隔离，物理READ白名单不扩大。
- [src/Vehicle/CMakeLists.txt](../../src/Vehicle/CMakeLists.txt)：登记新Vehicle decoder头／源。
- [qgroundcontrol.pro](../../qgroundcontrol.pro)：登记新Vehicle decoder头／源。

### 文档（18）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [src/Vehicle/FleetStartupAltitude.md](../../src/Vehicle/FleetStartupAltitude.md)。
- [src/Vehicle/VehicleStartupAltitude.md](../../src/Vehicle/VehicleStartupAltitude.md)。
- [custom/src/Swarm/FleetMissionStartReadiness.md](../../custom/src/Swarm/FleetMissionStartReadiness.md)。
- [custom/src/Swarm/FleetMissionStartReview.md](../../custom/src/Swarm/FleetMissionStartReview.md)。
- [custom/src/Swarm/FleetMissionStartupAuthority.md](../../custom/src/Swarm/FleetMissionStartupAuthority.md)。
- [custom/src/Swarm/FleetMissionGroundStartupAuthority.md](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)。
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)。

全部新高度内容仅作捕获时的冻结诊断；原阶段／审批当前身份、启动门槛、原点与step等合同不变。差值不能认证共同参考／内部valid／样本鲜度／净空或最终StartPermit。其余实质代码继续。下方443及更早为历史。


## 443：被动原点账本与实际启动连续性

本批13源码／工程登记、18文档，共31路径；新增纯decoder与Swarm helper各h／cc／md及Vehicle账本文档，无新QML组件或资源。仅列本批触及范围，不以累计dirty diff／文件／批次计完成率；原25目标保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

### 源码／工程登记（13）

- [src/Vehicle/FleetStartupGlobalOrigin.h](../../src/Vehicle/FleetStartupGlobalOrigin.h)：新增被动消息49纯值接口。
- [src/Vehicle/FleetStartupGlobalOrigin.cc](../../src/Vehicle/FleetStartupGlobalOrigin.cc)：严格wire解码18／参考8、零扩展及未认证边界。
- [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h)：独立原点账本／native getter／来源核对与私有发送basis声明。
- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)：最早原包观察、来源退休／revision／整栈隔离及实际发送连续性。
- [custom/src/Swarm/FleetMissionStartupGlobalOrigin.h](../../custom/src/Swarm/FleetMissionStartupGlobalOrigin.h)：新增native24→stable18纯契约声明。
- [custom/src/Swarm/FleetMissionStartupGlobalOrigin.cc](../../custom/src/Swarm/FleetMissionStartupGlobalOrigin.cc)：严格capture／admissible／same，不赋予许可。
- [custom/src/Swarm/FleetMissionStartReadiness.cc](../../custom/src/Swarm/FleetMissionStartReadiness.cc)：原点依据进入phase身份、前后native核对与阻断原因。
- [custom/src/Swarm/FleetMissionStartupTransition.cc](../../custom/src/Swarm/FleetMissionStartupTransition.cc)：原baseline／当前／直接native／尾部参考一致性。
- [src/Vehicle/CMakeLists.txt](../../src/Vehicle/CMakeLists.txt)：登记新Vehicle纯解码头／源。
- [qgroundcontrol.pro](../../qgroundcontrol.pro)：登记新Vehicle纯解码头／源。
- [custom/custom.pri](../../custom/custom.pri)：登记新Swarm纯参考helper头／源。
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)：原点四字段进入地面READ物理冻结及只读摘要。
- [custom/src/Swarm/FleetMissionStartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc)：终态再审逐机原点四字段、13字段row和原basis尾核对。

### 文档（18）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [src/Vehicle/FleetStartupGlobalOrigin.md](../../src/Vehicle/FleetStartupGlobalOrigin.md)。
- [src/Vehicle/VehicleStartupGlobalOrigin.md](../../src/Vehicle/VehicleStartupGlobalOrigin.md)。
- [custom/src/Swarm/FleetMissionStartupGlobalOrigin.md](../../custom/src/Swarm/FleetMissionStartupGlobalOrigin.md)。
- [custom/src/Swarm/FleetMissionStartReadiness.md](../../custom/src/Swarm/FleetMissionStartReadiness.md)。
- [custom/src/Swarm/FleetMissionStartupTransition.md](../../custom/src/Swarm/FleetMissionStartupTransition.md)。
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)。
- [custom/src/Swarm/FleetMissionStartupTransaction.md](../../custom/src/Swarm/FleetMissionStartupTransaction.md)。

原点变化和来源退休进入旧启动依据失效；消息为事件型，无周期TTL／自动请求。仍不认证HOME／local／ODOM／估计器共同原点、内部有效分支、净空、最终StartPermit或正式机载结果。其余实质源码工作保持。下方442及更早为历史。


## 442：完整请求链与独立启动参考的联合名义空间报告

本批10源码、17文档，共27路径；新增联合helper的h／cc／md，无新QML组件或资源登记。下列为本批触及范围，不以累计diff／批次／文件数计算完成率。原25目标保持，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

### 源码（10）

- [custom/src/Swarm/FleetMissionJointSpatialReview.h](../../custom/src/Swarm/FleetMissionJointSpatialReview.h)：新增联合纯值接口。
- [custom/src/Swarm/FleetMissionJointSpatialReview.cc](../../custom/src/Swarm/FleetMissionJointSpatialReview.cc)：完整canonical请求链／独立启动假设、逐机覆盖与同源区域检查。
- [custom/src/Swarm/FleetAirspaceConflictChecker.h](../../custom/src/Swarm/FleetAirspaceConflictChecker.h)：新增联合段／区域静态检查声明。
- [custom/src/Swarm/FleetAirspaceConflictChecker.cc](../../custom/src/Swarm/FleetAirspaceConflictChecker.cc)：严格区域与三维几何／预算／计数，保留旧入口。
- [custom/src/Swarm/FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h)：新增同线程联合纯值方法。
- [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)：联合helper转调，不修改lastReview或权限。
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)：同record构建、整组身份首尾核对及同token缓存发布。
- [custom/res/Merivus/FlyViewMap.qml](../../custom/res/Merivus/FlyViewMap.qml)：仅适配九字段capture，不新增图层或发送。
- [custom/res/Merivus/FleetMissionSpatialPanel.qml](../../custom/res/Merivus/FleetMissionSpatialPanel.qml)：联合摘要、精确合同及现有有界字段浏览。
- [custom/custom.pri](../../custom/custom.pri)：登记新helper头／源。

### 文档（17）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [custom/src/Swarm/FleetMissionJointSpatialReview.md](../../custom/src/Swarm/FleetMissionJointSpatialReview.md)：新增。
- [custom/src/Swarm/FleetAirspaceConflictChecker.md](../../custom/src/Swarm/FleetAirspaceConflictChecker.md)。
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)。
- [custom/src/Swarm/FleetMissionSpatialPanel.md](../../custom/src/Swarm/FleetMissionSpatialPanel.md)。
- [custom/src/Swarm/FleetMissionStartupSpatialPanel.md](../../custom/src/Swarm/FleetMissionStartupSpatialPanel.md)。
- [custom/src/Swarm/FleetMissionStartupWorkbench.md](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)。

新报告不改变原归档空域scope、批准或发送门禁，不认证实际轨迹／时序／动力学／净空。最终StartPermit、正式机载结果及其余实质源码继续。下方441及更早为历史。

## 441：原始空域输入的审阅／归档／审批一致性

本批7源码、15文档，共22路径；其中新建Intent与全链说明两份文档，无新服务、资源或工程登记。只列本批触及范围，不将累计工作树差异、批次号或文件数当完成率。原25目标保持，未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

### 源码（7）

- [custom/src/Swarm/FleetAirspaceConflictChecker.h](../../custom/src/Swarm/FleetAirspaceConflictChecker.h)：原值QVariant入口与纯检查声明。
- [custom/src/Swarm/FleetAirspaceConflictChecker.cc](../../custom/src/Swarm/FleetAirspaceConflictChecker.cc)：完整类型／JSON预算、双源阻断与无效间距不完整报告。
- [custom/src/Swarm/FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h)：原值审阅、纯inspect及快照接口声明。
- [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)：采集前快照、双源核对、风险报告与当前epoch提交。
- [custom/src/Swarm/FleetIntentTask.h](../../custom/src/Swarm/FleetIntentTask.h)：仅新增私有不回绕通知修订号。
- [custom/src/Swarm/FleetIntentTask.cc](../../custom/src/Swarm/FleetIntentTask.cc)：私有输入门禁、先整体提交再通知及销毁／重入隔离。
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)：原值传入、冻结副本归档、当前私有输入指纹与原审批首尾核对。

### 文档（15）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](../../docs/handoff/README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [custom/src/Swarm/FleetAirspaceConflictChecker.md](../../custom/src/Swarm/FleetAirspaceConflictChecker.md)。
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)。
- [custom/src/Swarm/FleetIntentTask.md](../../custom/src/Swarm/FleetIntentTask.md)：新增。
- [custom/src/Swarm/FleetMissionAirspaceInputGate.md](../../custom/src/Swarm/FleetMissionAirspaceInputGate.md)：新增。

坏JSON可保存明确阻断诊断，不恢复批准；opaque／非有限／超限原值没有安全快照则不生成新归档。完整联合空间检查、最终StartPermit／正式结果及其余实质开发仍继续。下方440及更早为历史。

## 440：完整原任务与归档空域共同冻结显示

本批14源码／资源、18文档，共32路径。只列本批触及范围，不把累计git差异／批次号当完成率；原25目标保持。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

### 源码／资源（14）

- [custom/src/Swarm/FleetMissionSpatialReview.h](../../custom/src/Swarm/FleetMissionSpatialReview.h)：新完整任务纯值接口。
- [custom/src/Swarm/FleetMissionSpatialReview.cc](../../custom/src/Swarm/FleetMissionSpatialReview.cc)：全成员规划／返程／动作及原整组本地编码投影。
- [custom/src/Swarm/FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h)：声明完整任务只读方法。
- [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)：同线程纯值转调与原生输入形状描述，不改变lastReview／信号。
- [custom/src/Swarm/FleetAirspaceConflictChecker.h](../../custom/src/Swarm/FleetAirspaceConflictChecker.h)：声明归档空域纯值方法。
- [custom/src/Swarm/FleetAirspaceConflictChecker.cc](../../custom/src/Swarm/FleetAirspaceConflictChecker.cc)：原报告／zone列表／实际阈值与未知scope投影。
- [custom/src/Swarm/FleetMissionPlanStore.cc](../../custom/src/Swarm/FleetMissionPlanStore.cc)：可选四字段原空域输入元数据严格保存／导入检查。
- [custom/res/Merivus/FleetMissionSpatialPanel.qml](../../custom/res/Merivus/FleetMissionSpatialPanel.qml)：新原任务分页明细及动作／协议项显示。
- [custom/res/Merivus/FleetMissionStartupSpatialPanel.qml](../../custom/res/Merivus/FleetMissionStartupSpatialPanel.qml)：同一capture内嵌完整任务，不新建捕获流程。
- [custom/res/Merivus/FlyViewMap.qml](../../custom/res/Merivus/FlyViewMap.qml)：独立原任务图层／动作点及同源交互隔离。
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)：准确record共同捕获／全目标核对与新归档输入形状。
- [custom/custom.pri](../../custom/custom.pri)：登记新helper头／源。
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)：登记新QML资源。
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)：登记新QML组件。

### 文档（18）

- [README.md](../../README.md)。
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)。
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)。
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)。
- [docs/INDEX.md](../../docs/INDEX.md)。
- [docs/handoff/README.md](README.md)。
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md)。
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md)。
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](MERIVUS_ACK_IDENTITY_AUDIT.md)。
- [custom/src/Swarm/FleetMissionSpatialReview.md](../../custom/src/Swarm/FleetMissionSpatialReview.md)。
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)。
- [custom/src/Swarm/FleetAirspaceConflictChecker.md](../../custom/src/Swarm/FleetAirspaceConflictChecker.md)。
- [custom/src/Swarm/FleetMissionSpatialPanel.md](../../custom/src/Swarm/FleetMissionSpatialPanel.md)。
- [custom/src/Swarm/FleetMissionStartupSpatialPanel.md](../../custom/src/Swarm/FleetMissionStartupSpatialPanel.md)。
- [custom/src/Swarm/FleetMissionStartupWorkbench.md](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)。
- [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md)。

新增5文件：完整任务helper的h／cc／md及完整明细Panel的qml／md。原生成器直接消费整份plan，规划返程与本地协议请求分开，编码失败不补路线或输出成功前缀。原空域报告仅覆盖其原规划路线／返程，不覆盖启动独立参考和终结下降；联合重审、真实空域／时间／净空、最终StartPermit及正式结果等实质工作继续。下方439及更早为历史。

## 439：同一冻结启动风险的空间明细、地图与名义间距候选

本批13源码／资源、17文档，共30路径。只列本批范围，不以累计git差异、文件数或批次号计算25项目标完成率。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

### 源码／资源（13）

| 路径 | 本批作用 |
| --- | --- |
| [FleetMissionStartupSpatialReview.h](../../custom/src/Swarm/FleetMissionStartupSpatialReview.h) | 新增纯冻结值几何构造入口 |
| [FleetMissionStartupSpatialReview.cc](../../custom/src/Swarm/FleetMissionStartupSpatialReview.cc) | 全目标／未知状态、三种独立名义段及原高度依据 |
| [FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h) | 声明只读startupSpatialReview接口 |
| [FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc) | 同线程纯值转调，不改lastReview或发信号 |
| [FleetAirspaceConflictChecker.h](../../custom/src/Swarm/FleetAirspaceConflictChecker.h) | 新增独立静态线段比较声明 |
| [FleetAirspaceConflictChecker.cc](../../custom/src/Swarm/FleetAirspaceConflictChecker.cc) | 明确阈值、有限局部投影、完整跨机对与有界候选明细 |
| [FleetMissionStartupSpatialPanel.qml](../../custom/res/Merivus/FleetMissionStartupSpatialPanel.qml) | 明确捕获／地图选择、逐机分页和高度／候选明细 |
| [FleetMissionStartupPanel.qml](../../custom/res/Merivus/FleetMissionStartupPanel.qml) | 在原启动工作台嵌入只读空间核对 |
| [FlyViewMap.qml](../../custom/res/Merivus/FlyViewMap.qml) | 同源名义图层、垂直段标记、只读地图隔离与退出恢复 |
| [GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 当前原生审阅／身份／binding首尾核对与准确归档版本取源 |
| [custom.pri](../../custom/custom.pri) | 登记新纯C++ helper的头文件与实现 |
| [qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 登记新QML资源 |
| [qmldir](../../custom/res/Merivus/qmldir) | 登记新QML组件 |

### 文档（17）

| 路径 | 内容 |
| --- | --- |
| [README.md](../../README.md) | 当前源码增量及限制 |
| [MODIFIED_FILES.md](../../MODIFIED_FILES.md) | 本批范围与索引 |
| [PROJECT_STATUS.md](../PROJECT_STATUS.md) | 当前状态与后续缺口 |
| [MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md) | 原25目标内的目标6／12推进 |
| [MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md) | 实现与未完成边界 |
| [INDEX.md](../INDEX.md) | 新合同入口 |
| [handoff/README.md](README.md) | 工程交接入口 |
| [MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md) | 未执行的构建／运行／数值／地图验收事项 |
| [MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md) | 本30路径清单 |
| [MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md) | 限定源码和文本核对，不代替运行验证 |
| [MERIVUS_ACK_IDENTITY_AUDIT.md](MERIVUS_ACK_IDENTITY_AUDIT.md) | 冻结来源、历史显示与权限边界 |
| [FleetMissionStartupSpatialReview.md](../../custom/src/Swarm/FleetMissionStartupSpatialReview.md) | 新几何helper输入／输出与未知值合同 |
| [FleetAirspaceConflictChecker.md](../../custom/src/Swarm/FleetAirspaceConflictChecker.md) | 独立名义比较接口及数值／候选预算 |
| [FleetMissionStartupSpatialPanel.md](../../custom/src/Swarm/FleetMissionStartupSpatialPanel.md) | 新面板、地图显示和交互约束 |
| [FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md) | 纯值接口与原审阅隔离 |
| [FleetMissionStartupWorkbench.md](../../custom/src/Swarm/FleetMissionStartupWorkbench.md) | Root精确来源／版本捕获及独立地图选择 |
| [FleetMissionStartupPanel.md](../../custom/src/Swarm/FleetMissionStartupPanel.md) | 启动工作台新增只读组件 |

新增5文件为StartupSpatialReview的h／cc／md及StartupSpatialPanel的qml／md；其余为现有文件增量。实际normal／ground风险与准确原record是唯一来源，不从最新草稿、实时坐标或历史Run补缺。名义段、全部目标几何覆盖、已表示段比较完成和真实安全许可各自独立；没有最终StartPermit、完整原任务／返程／结束动作及真实空域共同报告。原25目标与其余实质开发保持，下方438及更早为历史。

## 438：完整启动策略表单与实际草稿绑定

本批6源码／资源、14文档，共20路径。仅列本批触及范围，不以累计git差异或文件数估算完成率。未编译、运行、测试、改PX4、刷机、提交、更新PR或打包。

### 源码／资源（6）

- [custom/res/Merivus/FleetStartupPolicyEditor.qml](../../custom/res/Merivus/FleetStartupPolicyEditor.qml)
- [custom/res/Merivus/FleetExecutionProfileEditor.qml](../../custom/res/Merivus/FleetExecutionProfileEditor.qml)
- [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)

### 文档（14）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionExecutionProfile.md](../../custom/src/Swarm/FleetMissionExecutionProfile.md)
- [custom/src/Swarm/FleetExecutionProfileEditor.md](../../custom/src/Swarm/FleetExecutionProfileEditor.md)
- [custom/src/Swarm/FleetStartupPolicyEditor.md](../../custom/src/Swarm/FleetStartupPolicyEditor.md)

新增一个QML组件及两份模块文档，qrc／qmldir已登记。原C++规范化与发送后端不改；Root只增加草稿选择值及实际对象核对，Planner补来源与pendingInput。原25目标及六类实质工作继续，下方437及更早为历史。

## 437：当前部署启动与地面再审阅工作台

本批7源码／资源、14文档，合计21路径。仅列本批触及范围，不以累计工作树差异或文件数计算完成率。未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。

### 源码／资源（7）

- [custom/src/Swarm/FleetMissionArmedGroundReview.h](../../custom/src/Swarm/FleetMissionArmedGroundReview.h)
- [custom/src/Swarm/FleetMissionArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetMissionStartupPanel.qml](../../custom/res/Merivus/FleetMissionStartupPanel.qml)
- [custom/res/Merivus/CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml)
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)

### 文档（14）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionArmedGroundReview.md](../../custom/src/Swarm/FleetMissionArmedGroundReview.md)
- [custom/src/Swarm/FleetMissionStartupPanel.md](../../custom/src/Swarm/FleetMissionStartupPanel.md)
- [custom/src/Swarm/FleetMissionStartupWorkbench.md](../../custom/src/Swarm/FleetMissionStartupWorkbench.md)

新增一个QML组件、两份模块文档；GroundReview新增只读riskSummary属性，保持原current形状和操作门禁。Root统一值投影与20动作，指挥中心入口及qrc／qmldir登记同步；现有原生类登记复用，无新增C++类或pri条目。最终许可／净空／正式结果及原25目标其余实质工作保持。下方436及更早为历史。

## 436：实际Run简报交接与本地收件箱

本轮7个源码路径、15个文档路径，合计22路径。只描述本轮触及范围，不以工作树累计差异或文件数推导功能完成率。没有编译、运行、测试、改飞控、提交、更新PR或打包。

### 源码（7）

- [custom/src/Swarm/FleetMissionExecutionReport.h](../../custom/src/Swarm/FleetMissionExecutionReport.h)
- [custom/src/Swarm/FleetMissionExecutionReport.cc](../../custom/src/Swarm/FleetMissionExecutionReport.cc)
- [custom/src/Swarm/MissionHandoffManager.h](../../custom/src/Swarm/MissionHandoffManager.h)
- [custom/src/Swarm/MissionHandoffManager.cc](../../custom/src/Swarm/MissionHandoffManager.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetHandoffPanel.qml](../../custom/res/Merivus/FleetHandoffPanel.qml)
- [custom/res/Merivus/CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml)

### 文档（15）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionExecutionReport.md](../../custom/src/Swarm/FleetMissionExecutionReport.md)
- [custom/src/Swarm/MissionHandoffManager.md](../../custom/src/Swarm/MissionHandoffManager.md)
- [custom/src/Swarm/FleetHandoffPanel.md](../../custom/src/Swarm/FleetHandoffPanel.md)
- [custom/src/Swarm/FleetMissionHandoffWorkflow.md](../../custom/src/Swarm/FleetMissionHandoffWorkflow.md)

Manager与ExecutionReport沿原工程登记，不新增类／资源登记；CommandCenter给原交接面板接入实际controller。新增3份模块文档，其余路径为已有文件修改。跨站同步、真实控制权交接／中途加入及原25目标仍有实质开发。下方435及更早为历史。

## 435：同一新余段记录的地图、规划点与协议项

本轮12个源码／工程资源路径、16个文档路径，合计28路径。数量只描述本轮触及范围，不是功能验收率；工作树此前累积修改保持。本批未编译、运行、测试、提交、更新PR或打包。

### 源码／工程资源（12）

- [custom/custom.pri](../../custom/custom.pri)
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FlyViewMap.qml](../../custom/res/Merivus/FlyViewMap.qml)
- [custom/res/Merivus/FleetMissionRemainderPanel.qml](../../custom/res/Merivus/FleetMissionRemainderPanel.qml)
- [custom/res/Merivus/FleetMissionRemainderRoutePanel.qml](../../custom/res/Merivus/FleetMissionRemainderRoutePanel.qml)
- [custom/res/Merivus/FleetMissionDeploymentPanel.qml](../../custom/res/Merivus/FleetMissionDeploymentPanel.qml)
- [custom/src/Swarm/FleetMissionRemainderView.h](../../custom/src/Swarm/FleetMissionRemainderView.h)
- [custom/src/Swarm/FleetMissionRemainderView.cc](../../custom/src/Swarm/FleetMissionRemainderView.cc)
- [custom/src/Swarm/FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h)
- [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)

新helper从同一actual plan一次构建有限显示投影并做显式JSON空值桥接；Coordinator纯值入口、Root私有缓存／第12显示字段、QML分页与地图来源切换贯通。Map新增只读核对模式保持规划隔离，Deployment仅更新提示；原审批／上传／启动协议不变。

### 文档（16）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionRemainderView.md](../../custom/src/Swarm/FleetMissionRemainderView.md)
- [custom/src/Swarm/FleetMissionRemainderRoutePanel.md](../../custom/src/Swarm/FleetMissionRemainderRoutePanel.md)
- [custom/src/Swarm/FleetMissionRemainderPanel.md](../../custom/src/Swarm/FleetMissionRemainderPanel.md)
- [custom/src/Swarm/FleetMissionDeployment.md](../../custom/src/Swarm/FleetMissionDeployment.md)
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)

下方434及更早是历史批次，不计入本批范围。

## 434：余段新审阅采用、批准与共用地面部署

本批4源码、13文档，共17路径。未新增工程／资源登记，无C++或协议改动；依赖433及更早累计源码（含未跟踪文件），不是独立安装包、提交或PR。全25目标不缩减。

### 源码（4）

| 路径 | 本批职责 |
| --- | --- |
| [GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 私有新审阅上下文、九字段绑定／当前组校验、明确采用与精确token审批／退出、本地提交事实回执 |
| [FleetMissionRemainderPanel.qml](../../custom/res/Merivus/FleetMissionRemainderPanel.qml) | 结束编辑／采用、独立批准按钮、输入锁定与明确退出、过期／后置提交反馈 |
| [FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 选择唯一部署面板的数据来源及刷新目标，已采用余段时停用区域批准 |
| [FleetMissionDeploymentPanel.qml](../../custom/res/Merivus/FleetMissionDeploymentPanel.qml) | 显式来源合同、同源刷新、来源变化清确认、保持原native上传链与地面限制 |

### 文档（13）

- [README](../../README.md)、[MODIFIED_FILES](../../MODIFIED_FILES.md)
- [项目状态](../PROJECT_STATUS.md)、[路线图](../MERIVUS_FLEET_OS_ROADMAP.md)、[目标矩阵](../MERIVUS_SWARM_TARGET_MATRIX.md)、[文档索引](../INDEX.md)
- [交接入口](README.md)、[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)、本清单、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[身份审计](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [余段界面合同](../../custom/src/Swarm/FleetMissionRemainderPanel.md)、[地面部署合同](../../custom/src/Swarm/FleetMissionDeployment.md)

仅源码及限定文本检查；未编译、测试、运行helper／UI／认证／应用／设备，未修改PX4、刷机、提交、更新PR或打包。下方433及更早是历史范围。

## 433：实际Run到独立余段提案的源码链

本轮16源码/工程资源、17文档，共33路径。属于累计工作树增量，不是独立可构建补丁、安装包、新提交或PR；未跟踪依赖同样必须交接。原25目标及六类实质剩余开发不变。

### 源码与登记（16）

| 路径 | 本批职责 |
| --- | --- |
| [custom/custom.pri](../../custom/custom.pri) | 登记纯值helper h/cc |
| [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc) | 登记新增面板资源 |
| [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir) | 登记QML类型 |
| [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 实际Run取源/私有提案/新审阅及旧token撤销 |
| [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 输入互锁与既有滚动焦点接线 |
| [custom/res/Merivus/FleetMissionRemainderPanel.qml](../../custom/res/Merivus/FleetMissionRemainderPanel.qml) | 明确索引/新端点/profile/双阶段确认与退出 |
| [custom/src/Swarm/FleetMissionDeployment.h](../../custom/src/Swarm/FleetMissionDeployment.h) | 原任务纯值历史访问接口 |
| [custom/src/Swarm/FleetMissionDeployment.cc](../../custom/src/Swarm/FleetMissionDeployment.cc) | 实际原参数冻结，缺源不改变上传门槛 |
| [custom/src/Swarm/FleetMissionStartupTransaction.h](../../custom/src/Swarm/FleetMissionStartupTransaction.h) | 只读Run取源及私有历史声明 |
| [custom/src/Swarm/FleetMissionStartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc) | actual Run/member/payload/progress首尾核对 |
| [custom/src/Swarm/FleetMissionRemainderPlanner.h](../../custom/src/Swarm/FleetMissionRemainderPlanner.h) | build及共享一致性接口 |
| [custom/src/Swarm/FleetMissionRemainderPlanner.cc](../../custom/src/Swarm/FleetMissionRemainderPlanner.cc) | 包含式后缀、政策/预算保留与有界hash |
| [custom/src/Swarm/FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h) | 纯值构建入口声明 |
| [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc) | 同线程桥接，复用原审阅链 |
| [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 0.6.0来源/派生task/实际规范plan一致性 |
| [custom/src/Swarm/FleetMissionPlanStore.cc](../../custom/src/Swarm/FleetMissionPlanStore.cc) | 保存/导入共同验证来源与新路线 |

### 文档（17）

- [custom/src/Swarm/FleetMissionRemainderSource.md](../../custom/src/Swarm/FleetMissionRemainderSource.md)
- [custom/src/Swarm/FleetMissionRemainderPlanner.md](../../custom/src/Swarm/FleetMissionRemainderPlanner.md)
- [custom/src/Swarm/FleetMissionRemainderPanel.md](../../custom/src/Swarm/FleetMissionRemainderPanel.md)
- [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md)
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md)
- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)

### 交付边界

只接受实际原Run规划来源；人工明确选0基原出航索引，包含该点及完整后缀停留。新目标自身端点/速度与新profile形成单成员提案，原安全/能力/能量/模板政策保留，当前证据重新审阅。Compiler和PlanStore共享来源/内容核对；新记录不恢复旧批准或执行身份。

无自动跳点、机载改派/上传/起飞或旧Run修改；失联/退休progress仅历史提示，不证明已完成前缀。纯值来源hash不认证硬件或控制权。静态范围见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)，运行待办见[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。未编译、测试、运行helper/解析器/UI/应用/仿真/设备，未改PX4、刷机、提交、更新PR或打包；下方432及更早为历史。

## 432：备用机独立待命与完整路线重做预算

本批11源码／工程登记、18文档，共29路径。下列文件是累计工作树本轮范围；不表示独立可构建补丁，也不是安装包、提交或新PR。既有未跟踪源码依赖必须一并接收。新增 helper h/cc 与 custom.pri 登记，无新QML类型或资源文件。

### 源码／工程登记（11）

| 路径 | 本批职责 |
| --- | --- |
| [custom/custom.pri](../../custom/custom.pri) | 登记备用纯计算类，不增加运行入口 |
| [custom/src/Swarm/FleetReserveEnergyPlan.h](../../custom/src/Swarm/FleetReserveEnergyPlan.h) | 备用策略规范化与估算接口 |
| [custom/src/Swarm/FleetReserveEnergyPlan.cc](../../custom/src/Swarm/FleetReserveEnergyPlan.cc) | 自身端点、目标完整航点、待命和路径预算 |
| [custom/src/Swarm/FleetMissionDraft.h](../../custom/src/Swarm/FleetMissionDraft.h) | 草稿整份策略保存／清除接口 |
| [custom/src/Swarm/FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc) | 可选编辑源兼容、角色保留及独立reservePlans |
| [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc) | 备用成员分离、明确共享路线预算副本与全组容量处理 |
| [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc) | 独立备用账单、共同下限和缺预算未知语义 |
| [custom/src/Swarm/FleetEnergyScheduler.h](../../custom/src/Swarm/FleetEnergyScheduler.h) | 标明独立备用预算与unknown合同 |
| [custom/src/Swarm/FleetEnergyScheduler.cc](../../custom/src/Swarm/FleetEnergyScheduler.cc) | 保留主机账单、独立拒绝未知备用 |
| [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml) | 原实际Draft／版本绑定、撤旧审阅后保存／清除／端点 |
| [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml) | 备用编辑、取消、清除、独立端点及账单分项 |

### 文档（18）

- [custom/src/Swarm/FleetReserveEnergyPlan.md](../../custom/src/Swarm/FleetReserveEnergyPlan.md)
- [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md)
- [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md)
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [custom/src/Swarm/FleetEnergyScheduler.md](../../custom/src/Swarm/FleetEnergyScheduler.md)
- [custom/src/Swarm/FleetMissionEnergyBudgetPanel.md](../../custom/src/Swarm/FleetMissionEnergyBudgetPanel.md)
- [custom/src/Swarm/FleetReserveEnergyPlanPanel.md](../../custom/src/Swarm/FleetReserveEnergyPlanPanel.md)
- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)

### 范围边界

备用路径只进入 reserveMembers，不成为实际任务成员、上传航点或启动指令。完整估算只重做目标完整航点，缺少剩余航段／实际改派、真实耗能及净空模型。合法未知备用不抹主机账单，仍阻断该备用资格；超容量不保留有效前缀。旧source无策略时不注入默认值，原内容身份／审批和431电池来源／扣龄门禁保持。

仅源码静态阅读及文本检查，未编译、测试、运行helper／UI／应用／仿真／设备，未改PX4、刷机、提交、更新PR或打包。原25目标及六类实质开发继续；下方431及更早为历史。

## 431：实际电池遥测与能量审阅来源链

本批9个源码路径、16份文档，共25条路径。是累计工作树增量，不是独立可构建补丁、安装包或新PR；既有未跟踪依赖同样需要交接。没有新增QML类型、资源或构建登记。

### 源码（9）

| 路径 | 本批职责 |
|---|---|
| `src/Vehicle/Vehicle.h` | 独立电池观察账本、来源及原生只读接口 |
| `src/Vehicle/Vehicle.cc` | 原始PRIMARY收包、单推进聚合、扣龄估算与来源生命周期 |
| `custom/src/Swarm/SwarmController.cc` | 实际来源绑定、取消旧Fact平均、前后捕获核对及状态恢复 |
| `custom/src/Swarm/FleetRegistry.h` | 私有原生绑定与动态投影声明 |
| `custom/src/Swarm/FleetRegistry.cc` | 四种快照和健康汇总投影、导入隔离及生命周期 |
| `custom/src/Swarm/FleetEnergyScheduler.h` | 原生估算与18字段逐机账单契约 |
| `custom/src/Swarm/FleetEnergyScheduler.cc` | 原生DTO／标量／用途核对、估算依据与拒绝来源保留 |
| `custom/res/Merivus/GuidedActionsController.qml` | 稳定电池审阅身份与原预算的实时扣龄复核 |
| `custom/res/Merivus/FleetMissionPlannerPanel.qml` | 现有只读账单的来源、年龄与估算说明 |

### 文档（16）

`README.md`、`MODIFIED_FILES.md`；`docs/PROJECT_STATUS.md`、`docs/MERIVUS_FLEET_OS_ROADMAP.md`、`docs/MERIVUS_SWARM_TARGET_MATRIX.md`、`docs/INDEX.md`；`docs/handoff/README.md`、`MERIVUS_SWARM_ENGINEER_HANDOFF.md`、`MERIVUS_SWARM_PATCH_MANIFEST.md`、`MERIVUS_SWARM_STATIC_AUDIT.md`、`MERIVUS_ACK_IDENTITY_AUDIT.md`（后四项同在`docs/handoff/`）；`custom/src/Swarm/FleetNativeBatteryObservation.md`、`FleetNativeBatteryBridge.md`、`FleetRegistry.md`（后两项同在`custom/src/Swarm/`，这三项新增）；`custom/src/Swarm/FleetEnergyScheduler.md`、`FleetMissionEnergyBudgetPanel.md`（最后一项同在`custom/src/Swarm/`）。

仅源码与限定静态核对，未编译、测试、运行、修改PX4、刷机、提交、更新PR或打包。time_remaining是上报估算，不是实测性能；真实模型、多电池支持、完整25目标及六类实质工作仍未完成。

## 430：原成员逐机能量账单与实际审阅消费

本批7个源码路径、17份文档，共24条路径。是当前累计工作树增量，不是独立可构建补丁、安装包或新PR；既有未跟踪文件也需随完整依赖交接。没有新增QML类型／资源注册。

### 源码（7）

| 路径 | 本批职责 |
|---|---|
| `custom/src/Swarm/FleetEnergyScheduler.h` | 逐机绑定预算接口注释 |
| `custom/src/Swarm/FleetEnergyScheduler.cc` | 固定原身份、完整账单、保守多依据核算和预测修正 |
| `custom/src/Swarm/FleetMissionReviewCoordinator.cc` | 实际编译成员的独立任务／返程预算 |
| `custom/src/Swarm/FleetCapabilityMatcher.cc` | 核心性能、电量、链路数值-1未知口径 |
| `custom/src/Swarm/FleetRoleAllocator.cc` | 电量／链路未知与原证据要求兼容 |
| `custom/src/Swarm/FleetMissionRiskEvaluator.cc` | 绑定主／备用身份和实际合格子集核对 |
| `custom/res/Merivus/FleetMissionPlannerPanel.qml` | 默认折叠、分页只读能量账单 |

### 文档（17）

`README.md`、`MODIFIED_FILES.md`；`docs/PROJECT_STATUS.md`、`docs/MERIVUS_FLEET_OS_ROADMAP.md`、`docs/MERIVUS_SWARM_TARGET_MATRIX.md`、`docs/INDEX.md`；`docs/handoff/README.md`、`MERIVUS_SWARM_ENGINEER_HANDOFF.md`、`MERIVUS_SWARM_PATCH_MANIFEST.md`、`MERIVUS_SWARM_STATIC_AUDIT.md`、`MERIVUS_ACK_IDENTITY_AUDIT.md`（后四项同在`docs/handoff/`）；`custom/src/Swarm/FleetEnergyScheduler.md`、`FleetMissionReviewCoordinator.md`、`FleetCapabilityMatcher.md`、`FleetRoleAllocator.md`、`FleetMissionRiskEvaluator.md`、`FleetMissionEnergyBudgetPanel.md`（后五项同在`custom/src/Swarm/`，最后一项新增）。

没有新增真实电池遥测、机载接替执行或飞行许可。未编译、测试、运行、修改PX4、刷机、提交、更新PR或打包；原25目标及六类实质工作保持。

## 429：真实运行观察报告与显式文件交接

本批9个源码／资源登记路径、15份文档，共24条路径。是当前累计工作树增量，不是独立可构建补丁、安装包或新的PR；新建及既有未跟踪文件都必须纳入工程师交接。

### 源码与资源（9）

| 路径 | 本批职责 |
|---|---|
| `custom/src/Swarm/FleetMissionExecutionReport.h` | 独立只读报告和文件API |
| `custom/src/Swarm/FleetMissionExecutionReport.cc` | 有界Run摘要、内容校验、显式保存／读取 |
| `custom/res/Merivus/GuidedActionsController.qml` | 实际selected来源、冻结私有报告、分离提交事实的结果 |
| `custom/res/Merivus/FleetMissionExecutionReportPanel.qml` | 生成、摘要明细、两步保存、离线打开 |
| `custom/res/Merivus/FleetMissionObservationPanel.qml` | 独立报告入口，不借用关闭监视权限 |
| `custom/custom.pri` | h／cc登记 |
| `custom/src/CustomPlugin.cc` | 私有MerivusInternal类型登记 |
| `custom/qgroundcontrol.qrc` | QML资源登记 |
| `custom/res/Merivus/qmldir` | 面板类型登记 |

### 文档（15）

`README.md`、`MODIFIED_FILES.md`；`docs/PROJECT_STATUS.md`、`docs/MERIVUS_FLEET_OS_ROADMAP.md`、`docs/MERIVUS_SWARM_TARGET_MATRIX.md`、`docs/INDEX.md`；`docs/handoff/README.md`、`MERIVUS_SWARM_ENGINEER_HANDOFF.md`、`MERIVUS_SWARM_PATCH_MANIFEST.md`、`MERIVUS_SWARM_STATIC_AUDIT.md`、`MERIVUS_ACK_IDENTITY_AUDIT.md`（后四项同在`docs/handoff/`）；`custom/src/Swarm/FleetMissionExecutionReport.md`、`FleetMissionExecutionReportPanel.md`、`FleetMissionReportBuilder.md`、`FleetMissionObservationPanel.md`（后四项同在`custom/src/Swarm/`）。

未修改PX4、编译、运行、测试、刷机、提交、更新PR或打包。没有新增机载完成判定或飞行许可，原25目标和六类实质工作保持。

## 428：历史规划版本选择与独立编辑恢复

本批7个源码／资源登记路径、14份文档，共21条路径；是累计工作树增量，不是独立可构建补丁或安装包。新面板必须随qrc／qmldir登记及427依赖一起交接；未跟踪文件不能用普通git diff代替交付。

### 源码与资源登记（7）

- [custom/src/Swarm/FleetMissionPlanStore.h](../../custom/src/Swarm/FleetMissionPlanStore.h)
- [custom/src/Swarm/FleetMissionPlanStore.cc](../../custom/src/Swarm/FleetMissionPlanStore.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [custom/res/Merivus/FleetMissionArchiveDraftPanel.qml](../../custom/res/Merivus/FleetMissionArchiveDraftPanel.qml)
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)

### 文档（14）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md)
- [custom/src/Swarm/FleetMissionArchiveDraftPanel.md](../../custom/src/Swarm/FleetMissionArchiveDraftPanel.md)
- [custom/src/Swarm/FleetMissionRepairDraftPanel.md](../../custom/src/Swarm/FleetMissionRepairDraftPanel.md)

默认档案原schema及预算不变。仅对已加载有效历史提供版本选择／取源／编辑副本，不恢复审批、执行或控制权；未编译、测试、运行、改PX4、刷机、提交、更新PR或打包。下方为历史批次。

## 427：失败部署的原草稿封存与人工修复副本

本批8个源码路径、17份文档，共25条路径，是累计工作树增量，不是独立安装包或完整可构建补丁。复用现有Draft／Compiler／PlanStore／Deployment与QML资源，不新增工程类型；未跟踪新文件须随累计依赖交付，不能只交普通git diff。

### 源码（8）

- [custom/src/Swarm/FleetMissionDraft.h](../../custom/src/Swarm/FleetMissionDraft.h)
- [custom/src/Swarm/FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc)
- [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc)
- [custom/src/Swarm/FleetMissionPlanStore.cc](../../custom/src/Swarm/FleetMissionPlanStore.cc)
- [custom/src/Swarm/FleetMissionDeployment.h](../../custom/src/Swarm/FleetMissionDeployment.h)
- [custom/src/Swarm/FleetMissionDeployment.cc](../../custom/src/Swarm/FleetMissionDeployment.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetMissionDeploymentPanel.qml](../../custom/res/Merivus/FleetMissionDeploymentPanel.qml)

### 文档（17）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md)
- [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md)
- [custom/src/Swarm/FleetMissionPlanStore.md](../../custom/src/Swarm/FleetMissionPlanStore.md)
- [custom/src/Swarm/FleetMissionDeployment.md](../../custom/src/Swarm/FleetMissionDeployment.md)
- [custom/src/Swarm/FleetMissionRepairDraft.md](../../custom/src/Swarm/FleetMissionRepairDraft.md)
- [custom/src/Swarm/FleetMissionRepairDraftPanel.md](../../custom/src/Swarm/FleetMissionRepairDraftPanel.md)

原完整编辑源与编译任务绑定，新副本不改变机载任务或原归档；人工修复仍需重新确认基准、编译提案、原生审阅与批准。旧记录缺源不补造，通用任务不冒称无损导入。未编译、测试、运行helper／UI／应用、改PX4、刷机、提交、更新PR或打包；下方426及更早为历史。


## 426：独立整组重新核对

本批13个源码／工程登记路径、16份文档，共29条路径，属于累计工作树的本轮增量，不是独立可构建补丁或安装包。新Reconciliation只作为实际Deployment的私有子对象；不增加可替换QML服务，不扩大Episode原服务集合。必须包含未跟踪新文件，不能仅交普通git diff。

### 源码与工程登记（13）

- [custom/custom.pri](../../custom/custom.pri)
- [custom/src/Swarm/FleetRolePolicy.cc](../../custom/src/Swarm/FleetRolePolicy.cc)
- [custom/src/Swarm/FleetMissionDeploymentEpisode.cc](../../custom/src/Swarm/FleetMissionDeploymentEpisode.cc)
- [custom/src/Swarm/FleetMissionReconciliation.h](../../custom/src/Swarm/FleetMissionReconciliation.h)
- [custom/src/Swarm/FleetMissionReconciliation.cc](../../custom/src/Swarm/FleetMissionReconciliation.cc)
- [custom/src/Swarm/FleetMissionDeployment.h](../../custom/src/Swarm/FleetMissionDeployment.h)
- [custom/src/Swarm/FleetMissionDeployment.cc](../../custom/src/Swarm/FleetMissionDeployment.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetMissionDeploymentPanel.qml](../../custom/res/Merivus/FleetMissionDeploymentPanel.qml)
- [src/MissionManager/PlanManager.h](../../src/MissionManager/PlanManager.h)
- [src/MissionManager/PlanManager.cc](../../src/MissionManager/PlanManager.cc)
- [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h)
- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)

### 文档（16）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetRolePolicy.md](../../custom/src/Swarm/FleetRolePolicy.md)
- [custom/src/Swarm/FleetMissionReconciliation.md](../../custom/src/Swarm/FleetMissionReconciliation.md)
- [custom/src/Swarm/FleetMissionDeployment.md](../../custom/src/Swarm/FleetMissionDeployment.md)
- [custom/src/Swarm/FleetMissionReconciliationTransport.md](../../custom/src/Swarm/FleetMissionReconciliationTransport.md)
- [custom/src/Swarm/FleetMissionReconciliationPanel.md](../../custom/src/Swarm/FleetMissionReconciliationPanel.md)

读取只在原失败／取消部署完全排空后由明确操作发起；角色、实际来源及独立RAW INT结果与原Start收据分开。未修改PX4、编译、测试、执行helper／UI／认证／应用／设备、刷机、提交、更新PR或打包。完整路线图仍未完成，下方425及更早保留为历史。

## 425：普通任务回读的逐项诊断

本批8个源码／工程登记路径、15份文档，共23条路径，均为累计工作树增量；不是独立可构建补丁或安装包。新增纯值比较helper及PlanManager私有完成状态查询，不新增QML类型、资源文件、飞行命令或独立重新读取接口。当前Git中存在未跟踪文件，不能仅用普通git diff替代完整源码交接。

### 源码与工程登记（8）

- [custom/custom.pri](../../custom/custom.pri)
- [custom/src/Swarm/FleetMissionReadbackDiff.h](../../custom/src/Swarm/FleetMissionReadbackDiff.h)
- [custom/src/Swarm/FleetMissionReadbackDiff.cc](../../custom/src/Swarm/FleetMissionReadbackDiff.cc)
- [custom/src/Swarm/FleetMissionDeployment.h](../../custom/src/Swarm/FleetMissionDeployment.h)
- [custom/src/Swarm/FleetMissionDeployment.cc](../../custom/src/Swarm/FleetMissionDeployment.cc)
- [custom/res/Merivus/FleetMissionDeploymentPanel.qml](../../custom/res/Merivus/FleetMissionDeploymentPanel.qml)
- [src/MissionManager/PlanManager.h](../../src/MissionManager/PlanManager.h)
- [src/MissionManager/PlanManager.cc](../../src/MissionManager/PlanManager.cc)

### 文档（15）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../INDEX.md)
- [docs/handoff/README.md](README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetMissionReadbackDiff.md](../../custom/src/Swarm/FleetMissionReadbackDiff.md)
- [custom/src/Swarm/FleetMissionDeployment.md](../../custom/src/Swarm/FleetMissionDeployment.md)
- [custom/src/Swarm/FleetMissionDeploymentTransport.md](../../custom/src/Swarm/FleetMissionDeploymentTransport.md)
- [custom/src/Swarm/FleetMissionReadbackDiagnosticsPanel.md](../../custom/src/Swarm/FleetMissionReadbackDiagnosticsPanel.md)

未修改GuidedActionsController：既有只读current/history投影已经携带原生vehicles值。未编译、测试、运行、提交、推送、更新PR或生成包；未修改PX4。差异详情只属实际普通回读快照，失败后的独立重新对账／修复草稿仍是后续工作。

## 424：跨日志库有界检索

本批6个源码路径、14份文档，共20条路径；为累计工作树的增量。复用Reader/Store/既有归档面板及Root私有facade，不新增编译单元、资源注册、数据库schema或飞行命令。Reader.h新增纯值静态合同校验，Store新增独立历史检索属性/入口。未提交、推送、更新PR、生成补丁文件或安装包；未跟踪新文件必须连同累计工作树接收，普通git diff不足以打包。

### 源码（6）

- [custom/src/Swarm/FleetEventArchiveHistoryReader.h](../../custom/src/Swarm/FleetEventArchiveHistoryReader.h)
- [custom/src/Swarm/FleetEventArchiveHistoryReader.cc](../../custom/src/Swarm/FleetEventArchiveHistoryReader.cc)
- [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h)
- [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc)
- [custom/res/Merivus/FleetArchiveHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveHistoryPanel.qml)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)

### 文档（14）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetEventArchiveHistoryReader.md](../../custom/src/Swarm/FleetEventArchiveHistoryReader.md)
- [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md)
- [custom/src/Swarm/FleetArchiveHistoryPanel.md](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)

实现、静态审阅及待运行项目分别见[Reader合同](../../custom/src/Swarm/FleetEventArchiveHistoryReader.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅为源码阶段，不是可执行安装包或完整25目标验收。


## 423：未封存库离线只读分页

本批4源码、14文档，共18条路径；是既有累计工程的增量。没有新增C++类型/编译单元或QML资源，没有改SQL schema、Codec或PX4；custom.pri/qrc/qmldir沿用此前登记。Reader.h接口未变，不列为本批修改。未提交、更新PR、生成补丁文件或安装包。

### 源码（4）

- [custom/src/Swarm/FleetEventArchiveHistoryReader.cc](../../custom/src/Swarm/FleetEventArchiveHistoryReader.cc)
- [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h)
- [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc)
- [custom/res/Merivus/FleetArchiveHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveHistoryPanel.qml)

Reader.cc新增固定current库的只读文件/SQL路径和协作锁；Store.h/.cc新增私有离线游标、实际writerActive和offline产品请求/结果；Panel.qml新增明确离线入口与独立页作用域。全部依赖422及更早累计源码，未跟踪文件不能靠普通git diff遗漏。

### 文档（14）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetEventArchiveHistoryReader.md](../../custom/src/Swarm/FleetEventArchiveHistoryReader.md)
- [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md)
- [custom/src/Swarm/FleetArchiveHistoryPanel.md](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)

### 合同与接收边界

产品仍intent6/return9，新增action=offline；Reader离线请求恰6键kind/databaseEpoch/fileFingerprint/runFilter/afterRowId/limit，首读空epoch/hash、后页actual绑定，runFilter固定空。结果沿原history-page，新增固定basename、offlineCurrentOnly、sealedSegmentOnly=false、fileFingerprint、cooperativeOwnerLockHeldDuringRead及权限false标志；不冒充catalog或封存段，不提供任意路径。

当前实例必须实际无writer，读取线程持同Writer owner锁，只读打开已有库；只锁文件会创建／释放，库和sidecar不修复或删除。正常停止/无残留sidecar的已有库可申请读取；不存在、损坏、热日志或锁冲突拒绝，不自动重启录制。每页64行，原2MiB页预算和逐行unclean/drop/discard/retired状态保留。

限定结果见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)，未执行验证见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批不等于长期运营、全库搜索、完整构型历史或原25目标完成。

## 422：固定本机归档入口与工程审批摘要

本批11源码／资源、16文档，共27条路径；是累计工作树增量，不是独立源码包或安装包。未跟踪文件也在本清单中，普通 git diff 不会包含它们；不能只导出本批11文件而丢弃既有依赖。本批未提交、更新 PR 或打包。

### 源码／资源（11）

- [custom/src/Swarm/FleetEventArchiveCodec.h](../../custom/src/Swarm/FleetEventArchiveCodec.h)
- [custom/src/Swarm/FleetEventArchiveCodec.cc](../../custom/src/Swarm/FleetEventArchiveCodec.cc)
- [custom/src/Swarm/FleetPersistentEventStore.h](../../custom/src/Swarm/FleetPersistentEventStore.h)
- [custom/src/Swarm/FleetPersistentEventStore.cc](../../custom/src/Swarm/FleetPersistentEventStore.cc)
- [custom/res/Merivus/FleetArchiveHistoryPanel.qml](../../custom/res/Merivus/FleetArchiveHistoryPanel.qml)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetCapabilityReviewPanel.qml](../../custom/res/Merivus/FleetCapabilityReviewPanel.qml)
- [custom/res/Merivus/CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml)
- [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)

Codec两源增加严格工程摘要和兼容v1读取；Store两源增加固定政策、明确动作、查询归属和实际BlackBox摘要接纳。新Panel、Root和审批Panel连接请求与事实展示，两个宿主页提供入口，qrc/qmldir登记新QML。没有新增C++单元，不需要改custom.pri；Q_INVOKABLE／Q_PROPERTY和资源实际构建仍待工程师。

### 文档（16）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetEventArchiveCodec.md](../../custom/src/Swarm/FleetEventArchiveCodec.md)
- [custom/src/Swarm/FleetPersistentEventStore.md](../../custom/src/Swarm/FleetPersistentEventStore.md)
- [custom/src/Swarm/FleetArchiveHistoryPanel.md](../../custom/src/Swarm/FleetArchiveHistoryPanel.md)
- [custom/src/Swarm/FleetCapabilityReview.md](../../custom/src/Swarm/FleetCapabilityReview.md)
- [custom/src/Swarm/FleetCapabilityReviewPanel.md](../../custom/src/Swarm/FleetCapabilityReviewPanel.md)

### 接收边界

复用原SQL/schema与wrapper18/schema1；新工程摘要只使payloadProjection为v2，旧v1继续只读。选项exact7、动作intent6／return9、工程摘要outer2／inner21见模块合同。开始／停止／封存成功的requestId为空，查询ID由原生生成，取消只携带原本页ID，不混用动作与查询回执。

固定AppData目录、活动库100000条／256 MiB、队列512条／8 MiB、批64、250 ms忙等、历史页2 MiB、请求64行、封存32段；无自动启动、自动删除或完整磁盘配额。当前页只读正在录制的run；离线仅封存段。历史不恢复批准或机载任务，内存接纳不证明SQL写入。检查证据见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)，未执行项目见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

## 421：构型基线的实际双账号操作链

本批9源码、17文档，共26条交接路径；是累计工程上的增量，不是独立安装包或全25目标验收。部分源码和文档仍未跟踪，普通 git diff 不自动包含，交接须保留累计依赖。本批未提交、更新 PR 或打包。

### 源码（9）

- [custom/src/Swarm/FleetRegistry.h](../../custom/src/Swarm/FleetRegistry.h)
- [custom/src/Swarm/FleetRegistry.cc](../../custom/src/Swarm/FleetRegistry.cc)
- [custom/src/Swarm/FleetCapabilityReview.h](../../custom/src/Swarm/FleetCapabilityReview.h)
- [custom/src/Swarm/FleetCapabilityReview.cc](../../custom/src/Swarm/FleetCapabilityReview.cc)
- [custom/src/Swarm/FleetOperatorSession.h](../../custom/src/Swarm/FleetOperatorSession.h)
- [custom/src/Swarm/FleetOperatorSession.cc](../../custom/src/Swarm/FleetOperatorSession.cc)
- [custom/src/Swarm/FleetRolePolicy.cc](../../custom/src/Swarm/FleetRolePolicy.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetCapabilityReviewPanel.qml](../../custom/res/Merivus/FleetCapabilityReviewPanel.qml)

Registry 两源负责实际记录、四参数提交与14键回执；Review 两源、Session 两源和 RolePolicy 负责四操作、精确双账号用途及最终围栏；Root/Panel 两源接实际入口、冻结预览和事实回执。没有新 C++ 编译单元或 QML 资源，无需新增 pri/qrc/qmldir 条目；实际构建和元对象生成仍由工程师完成。

### 文档（17）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetConfigurationBaselineEvidence.md](../../custom/src/Swarm/FleetConfigurationBaselineEvidence.md)
- [custom/src/Swarm/FleetCapabilityEvidence.md](../../custom/src/Swarm/FleetCapabilityEvidence.md)
- [custom/src/Swarm/FleetCapabilityReview.md](../../custom/src/Swarm/FleetCapabilityReview.md)
- [custom/src/Swarm/FleetCapabilityReviewPanel.md](../../custom/src/Swarm/FleetCapabilityReviewPanel.md)
- [custom/src/Swarm/FleetOperatorSession.md](../../custom/src/Swarm/FleetOperatorSession.md)
- [custom/src/Swarm/FleetRolePolicy.md](../../custom/src/Swarm/FleetRolePolicy.md)

### 接收边界

原 current30/result16/preview2/18、approvalEvidence8 和能力 context6/approval14 不扩字段。新增基线 mutation 回执虽为14键，但不是能力 mutation 回执或能力批准，字段合同见[基线说明](../../custom/src/Swarm/FleetConfigurationBaselineEvidence.md)。能力与基线的操作、角色动作、账号用途和事件类型分开；重复基线批准也退役旧能力依据。

仅限定静态核对，见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)；运行待办见[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。完整有类型基线仍无持久化，实物/性能来源和原25目标/六类仍有实质源码工作。下方420及更早清单是历史，不用旧“无基线UI”覆盖本批。

## 420：完整基线投影、统一消费与审阅归档

本批12源码、20文档，共32条交接路径。都是累计工作树上的增量；没有新的源码包、安装包或PR。源文件包括前批新增且尚未跟踪的文件，不能仅依赖 git diff 导出。数量不代表原25项目标完成率。

### 源码（12）

- [custom/src/Swarm/FleetRegistry.h](../../custom/src/Swarm/FleetRegistry.h)
- [custom/src/Swarm/FleetRegistry.cc](../../custom/src/Swarm/FleetRegistry.cc)
- [custom/src/Swarm/FleetCapabilityEvidence.h](../../custom/src/Swarm/FleetCapabilityEvidence.h)
- [custom/src/Swarm/FleetCapabilityEvidence.cc](../../custom/src/Swarm/FleetCapabilityEvidence.cc)
- [custom/src/Swarm/FleetCapabilityMatcher.cc](../../custom/src/Swarm/FleetCapabilityMatcher.cc)
- [custom/src/Swarm/FleetRoleAllocator.cc](../../custom/src/Swarm/FleetRoleAllocator.cc)
- [custom/src/Swarm/FleetMissionCompiler.cc](../../custom/src/Swarm/FleetMissionCompiler.cc)
- [custom/src/Swarm/FleetMissionContinuityPlanner.cc](../../custom/src/Swarm/FleetMissionContinuityPlanner.cc)
- [custom/src/Swarm/FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h)
- [custom/src/Swarm/FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)
- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/src/Swarm/FleetMissionDeployment.cc](../../custom/src/Swarm/FleetMissionDeployment.cc)

本批没有新增C++编译单元或QML资源，沿用已有累计登记；Coordinator新增只读方法须由工程师在实际构建时重新生成元对象。Registry/Evidence共4源负责原生投影与纯值校验，四类消费者共4源，Coordinator2源、Guided及Deployment共4源接续身份和归档。

### 文档（20）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetConfigurationBaselineEvidence.md](../../custom/src/Swarm/FleetConfigurationBaselineEvidence.md)
- [custom/src/Swarm/FleetCapabilityEvidence.md](../../custom/src/Swarm/FleetCapabilityEvidence.md)
- [custom/src/Swarm/FleetConfigurationDiff.md](../../custom/src/Swarm/FleetConfigurationDiff.md)
- [custom/src/Swarm/FleetCapabilityMatcher.md](../../custom/src/Swarm/FleetCapabilityMatcher.md)
- [custom/src/Swarm/FleetRoleAllocator.md](../../custom/src/Swarm/FleetRoleAllocator.md)
- [custom/src/Swarm/FleetMissionCompiler.md](../../custom/src/Swarm/FleetMissionCompiler.md)
- [custom/src/Swarm/FleetMissionContinuityPlanner.md](../../custom/src/Swarm/FleetMissionContinuityPlanner.md)
- [custom/src/Swarm/FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [custom/src/Swarm/FleetMissionDeployment.md](../../custom/src/Swarm/FleetMissionDeployment.md)

### 合同与后续

新基线投影1/14，稳定身份2键；能力归档升2/8、member9。原能力context6/approval14、构型Review preview2/18和PlanStore计划顶层不变。完整A→未知→A的新审阅可以再次符合参考基线，旧批准/建议不可复活；可选政策不隐式变必需。仍缺实际基线批准界面、历史持久化与实物/性能来源及完整产品六类工作。

仅静态审阅与文本/链接检查，无编译、测试或运行结果。详见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)和[工程师说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。下方419及更早为历史。

## 419：构型快照与有类型字段差异

本批明确8个源码/工程登记路径、15份文档，共23条交接路径。数量只说明增量范围，不是产品完成率；应连同累计工程接收，不可仅复制新helper。没有新增提交、PR或安装包。新文件仍需工程师在实际构建工程中包含，git diff不会自动包含未跟踪文件。

### 源码/工程登记（8）

- [custom/src/Swarm/FleetConfigurationDiff.h](../../custom/src/Swarm/FleetConfigurationDiff.h)
- [custom/src/Swarm/FleetConfigurationDiff.cc](../../custom/src/Swarm/FleetConfigurationDiff.cc)
- [custom/src/Swarm/FleetRegistry.h](../../custom/src/Swarm/FleetRegistry.h)
- [custom/src/Swarm/FleetRegistry.cc](../../custom/src/Swarm/FleetRegistry.cc)
- [custom/src/Swarm/FleetCapabilityReview.h](../../custom/src/Swarm/FleetCapabilityReview.h)
- [custom/src/Swarm/FleetCapabilityReview.cc](../../custom/src/Swarm/FleetCapabilityReview.cc)
- [custom/res/Merivus/FleetCapabilityReviewPanel.qml](../../custom/res/Merivus/FleetCapabilityReviewPanel.qml)
- [custom/custom.pri](../../custom/custom.pri)

其中FleetConfigurationDiff.h/.cc为本批新增；其余为已有工作树文件的继续修改。Registry保存有类型历史并在merge/approve/import/能力提交中一致更新；Review绑定实际比较，Panel消费schema2/18，pri登记helper。原资源入口不变，因此本批不再次修改qrc/qmldir/两个宿主。

### 文档（15）

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](../../docs/handoff/README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetConfigurationDiff.md](../../custom/src/Swarm/FleetConfigurationDiff.md)
- [custom/src/Swarm/FleetCapabilityReview.md](../../custom/src/Swarm/FleetCapabilityReview.md)
- [custom/src/Swarm/FleetCapabilityReviewPanel.md](../../custom/src/Swarm/FleetCapabilityReviewPanel.md)
- [custom/src/Swarm/FleetCapabilityEvidence.md](../../custom/src/Swarm/FleetCapabilityEvidence.md)

### 合同与验收边界

preview独立schema2/18；Registry wrapper16/report13/row6/value9；原context6/approval14及current30/result16/Root5保持。hash-only之后残留map不能新批准构型基线；完整可用性切换和typed变化使旧能力批准失效；审批-only导入亦退役触达record。完整是显式非空map的输入约定，不是机型字段完整/硬件认证。

静态阅读与字符/空白/本地链接目标检查不代替编译或运行。工程师待验收见[交接说明](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。旧configurationState/requireConfigurationBaseline仍为哈希级，统一真实基线证据消费是下一源码切点；25目标/六类继续，不能当作全部已完成。

## 418：能力声明双账号审阅与实际双入口

本阶段16个源码/资源路径、17份文档，共33路径。是累计工程增量，不能只复制本批文件称可构建或已验证；不按路径数计算25目标完成率。

- [FleetRegistry.h](../../custom/src/Swarm/FleetRegistry.h)
- [FleetRegistry.cc](../../custom/src/Swarm/FleetRegistry.cc)
- [FleetCapabilityReview.h](../../custom/src/Swarm/FleetCapabilityReview.h)
- [FleetCapabilityReview.cc](../../custom/src/Swarm/FleetCapabilityReview.cc)
- [FleetOperatorSession.h](../../custom/src/Swarm/FleetOperatorSession.h)
- [FleetOperatorSession.cc](../../custom/src/Swarm/FleetOperatorSession.cc)
- [FleetRolePolicy.cc](../../custom/src/Swarm/FleetRolePolicy.cc)
- [SwarmController.h](../../custom/src/Swarm/SwarmController.h)
- [SwarmController.cc](../../custom/src/Swarm/SwarmController.cc)
- [GuidedActionsController.qml（实际 custom）](../../custom/res/Merivus/GuidedActionsController.qml)
- [FleetCapabilityReviewPanel.qml](../../custom/res/Merivus/FleetCapabilityReviewPanel.qml)
- [CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml)
- [FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [custom.pri](../../custom/custom.pri)
- [qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)
- [qmldir](../../custom/res/Merivus/qmldir)

本批文档17路径：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [INDEX.md](../../docs/INDEX.md)
- [handoff README](../../docs/handoff/README.md)
- [MERIVUS_SWARM_ENGINEER_HANDOFF.md](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [MERIVUS_SWARM_PATCH_MANIFEST.md](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)
- [MERIVUS_SWARM_STATIC_AUDIT.md](../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)
- [MERIVUS_ACK_IDENTITY_AUDIT.md](../../docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md)
- [FleetCapabilityEvidence.md](../../custom/src/Swarm/FleetCapabilityEvidence.md)
- [FleetOperatorSession.md](../../custom/src/Swarm/FleetOperatorSession.md)
- [FleetRolePolicy.md](../../custom/src/Swarm/FleetRolePolicy.md)
- [FleetMissionOperatorWorkflow.md](../../custom/src/Swarm/FleetMissionOperatorWorkflow.md)
- [FleetCapabilityReview.md](../../custom/src/Swarm/FleetCapabilityReview.md)
- [FleetCapabilityReviewPanel.md](../../custom/src/Swarm/FleetCapabilityReviewPanel.md)

Registry新增带最终围栏的回执重载保旧bool；Swarm原Registry改QPointer及直接刷新链源存活检查；新Review、专用Session/Role、Root和Panel形成明确采纳/撤销链。原context6/approval14不变。仅本地声明，不改基线/参数/任务执行，不发命令。

原生账号记录、提交事实与本地日志追加分别保留。未编译、测试、运行或放行，未修改PX4、更新PR或打包；旧阶段清单保留为累计依赖，不覆盖本节范围。

## 417：工程目录重载与全球任务估计器门禁

本阶段精确19个源码/资源路径如下；另同步19份文档，共38路径。是累计工程增量，不能只复制这19项就认定可构建或已验证，不把路径数量当25目标完成率。

- [FleetMissionStartupEstimator.h](../../custom/src/Swarm/FleetMissionStartupEstimator.h)
- [FleetMissionStartupEstimator.cc](../../custom/src/Swarm/FleetMissionStartupEstimator.cc)
- [Vehicle.cc](../../src/Vehicle/Vehicle.cc)
- [FleetFirmwareAdapterCatalog.h](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.h)
- [FleetFirmwareAdapterCatalog.cc](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.cc)
- [FleetFirmwareCatalogReload.h](../../custom/src/Swarm/FleetFirmwareCatalogReload.h)
- [FleetFirmwareCatalogReload.cc](../../custom/src/Swarm/FleetFirmwareCatalogReload.cc)
- [FleetOperatorSession.h](../../custom/src/Swarm/FleetOperatorSession.h)
- [FleetOperatorSession.cc](../../custom/src/Swarm/FleetOperatorSession.cc)
- [FleetRolePolicy.cc](../../custom/src/Swarm/FleetRolePolicy.cc)
- [FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h)
- [FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)
- [GuidedActionsController.qml（实际 custom 覆盖）](../../custom/res/Merivus/GuidedActionsController.qml)
- [FleetFirmwareCatalogPanel.qml](../../custom/res/Merivus/FleetFirmwareCatalogPanel.qml)
- [CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml)
- [FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [custom.pri](../../custom/custom.pri)
- [qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)
- [qmldir](../../custom/res/Merivus/qmldir)

本批文档19路径：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [PROJECT_STATUS.md](../PROJECT_STATUS.md)
- [MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md)
- [MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md)
- [INDEX.md](../INDEX.md)
- [handoff README](README.md)
- [ENGINEER_HANDOFF](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [PATCH_MANIFEST](MERIVUS_SWARM_PATCH_MANIFEST.md)
- [STATIC_AUDIT](MERIVUS_SWARM_STATIC_AUDIT.md)
- [ACK_IDENTITY_AUDIT](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [FleetFirmwareCatalogReload.md](../../custom/src/Swarm/FleetFirmwareCatalogReload.md)
- [FleetFirmwareCatalogPanel.md](../../custom/src/Swarm/FleetFirmwareCatalogPanel.md)
- [FleetFirmwareAdapterCatalog.md](../../custom/src/Swarm/FleetFirmwareAdapterCatalog.md)
- [FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [FleetOperatorSession.md](../../custom/src/Swarm/FleetOperatorSession.md)
- [FleetRolePolicy.md](../../custom/src/Swarm/FleetRolePolicy.md)
- [FleetMissionStartupEstimator.md](../../custom/src/Swarm/FleetMissionStartupEstimator.md)
- [FleetMissionOperatorWorkflow.md](../../custom/src/Swarm/FleetMissionOperatorWorkflow.md)

固定现有 AppData 档案只读，纯预览完整旧/新声明与 diff/哈希、独立两账号一用、原目录 epoch/revision/CAS 与实际 scope/期限/最终复读及纯围栏后整体替换内存 catalog。目录改动使旧 review 失效，不停止 Run、不释放占用；原 startup 自动 load 不强制走新审批。另补全球任务 ABS 水平/垂直且非 CONST_POS，不改变 RAW/通用 Health/室内/GNSS 硬件。

限定交叉读回和后续文本清点范围见[审计](MERIVUS_SWARM_STATIC_AUDIT.md)。未编译、测试、运行解析/helper/UI/认证/应用/SQL/设备、改 PX4、刷机、更新 PR 或打包；原25目标/六类继续。以下旧批次保留历史。

## 416：逐机执行配置与只读参数绑定

本阶段11个源码/资源路径如下；另同步15份文档，共26路径。接收时必须保留累计依赖，不是独立安装包：

- [FleetMissionDraft.h](../../custom/src/Swarm/FleetMissionDraft.h)
- [FleetMissionDraft.cc](../../custom/src/Swarm/FleetMissionDraft.cc)
- [FleetMissionReviewCoordinator.h](../../custom/src/Swarm/FleetMissionReviewCoordinator.h)
- [FleetMissionReviewCoordinator.cc](../../custom/src/Swarm/FleetMissionReviewCoordinator.cc)
- [ParameterManager.h](../../src/FactSystem/ParameterManager.h)
- [ParameterManager.cc](../../src/FactSystem/ParameterManager.cc)
- [GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [FleetExecutionProfileEditor.qml](../../custom/res/Merivus/FleetExecutionProfileEditor.qml)
- [qmldir](../../custom/res/Merivus/qmldir)
- [qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)

文档同步公共说明11份与模块说明4份，以下15路径；限定检查见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [PROJECT_STATUS.md](../PROJECT_STATUS.md)
- [MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md)
- [MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md)
- [INDEX.md](../INDEX.md)
- [handoff README](README.md)
- [ENGINEER_HANDOFF](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [PATCH_MANIFEST](MERIVUS_SWARM_PATCH_MANIFEST.md)
- [STATIC_AUDIT](MERIVUS_SWARM_STATIC_AUDIT.md)
- [ACK_IDENTITY_AUDIT](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [FleetMissionDraft.md](../../custom/src/Swarm/FleetMissionDraft.md)
- [FleetMissionExecutionProfile.md](../../custom/src/Swarm/FleetMissionExecutionProfile.md)
- [FleetMissionReviewCoordinator.md](../../custom/src/Swarm/FleetMissionReviewCoordinator.md)
- [FleetMissionOperatorWorkflow.md](../../custom/src/Swarm/FleetMissionOperatorWorkflow.md)

实际链是逐机完整 profile CAS、保留/删除语义、真实12参数观察与保存前指纹复读、旧 review 失效及原 Compiler 消费。reserve 不活动输出；通知后 false 可能已有本地提交。高级政策仍为完整 JSON schema1–3，可空保存规划但不能启动。不改 PX4/参数，不编译、测试、运行、更新 PR 或打包；限定静态检查结果已记入本批审计。原25目标/六类继续，下方清单为历史。

## 411–415：恢复复核接线精确增量（9 + 18 = 27路径）

本节覆盖旧阶段“未加UI”的描述，保留其它累计依赖。源码/资源9路径：

- [custom/res/Merivus/GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)
- [custom/res/Merivus/FleetOccupancyRecoveryPanel.qml](../../custom/res/Merivus/FleetOccupancyRecoveryPanel.qml)
- [custom/res/Merivus/CommandCenterOverlay.qml](../../custom/res/Merivus/CommandCenterOverlay.qml)
- [custom/res/Merivus/FleetMissionPlannerPanel.qml](../../custom/res/Merivus/FleetMissionPlannerPanel.qml)
- [custom/res/Merivus/qmldir](../../custom/res/Merivus/qmldir)
- [custom/qgroundcontrol.qrc](../../custom/qgroundcontrol.qrc)
- [custom/src/Swarm/FleetOccupancyRecoveryReview.h](../../custom/src/Swarm/FleetOccupancyRecoveryReview.h)
- [custom/src/Swarm/FleetOccupancyRecoveryReview.cc](../../custom/src/Swarm/FleetOccupancyRecoveryReview.cc)
- [custom/src/Swarm/SwarmController.cc](../../custom/src/Swarm/SwarmController.cc)

文档18路径：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../../docs/PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../../docs/MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../../docs/MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../../docs/INDEX.md)
- [docs/handoff/README.md](README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetNativeOccupancyEvidence.md](../../custom/src/Swarm/FleetNativeOccupancyEvidence.md)
- [custom/src/Swarm/FleetMissionContinuityPlanner.md](../../custom/src/Swarm/FleetMissionContinuityPlanner.md)
- [custom/src/Swarm/SwarmMissionOrchestrator.md](../../custom/src/Swarm/SwarmMissionOrchestrator.md)
- [custom/src/Swarm/FleetOccupancyJournal.md](../../custom/src/Swarm/FleetOccupancyJournal.md)
- [custom/src/Swarm/FleetOperatorSession.md](../../custom/src/Swarm/FleetOperatorSession.md)
- [custom/src/Swarm/FleetOccupancyRecovery.md](../../custom/src/Swarm/FleetOccupancyRecovery.md)
- [custom/src/Swarm/FleetOccupancyRecoveryPanel.md](../../custom/src/Swarm/FleetOccupancyRecoveryPanel.md)

必须接完整累计工程（含此前Evidence、Vehicle、Session、Role、pri及原生依赖），不能把以上9源码/资源当独立构建或安装包。Root actual来源与原收据、controller模式和显示schema须同批；原生九键占用快照、11键磁盘文件及参数/飞控协议不改变。未编译、测试、运行、修改PX4、刷机、更新PR或打包；25目标/六类继续。

## 406–410：原生重启／历史占用专用复核

[恢复复核合同](../../custom/src/Swarm/FleetOccupancyRecovery.md)新增实际Swarm唯一拥有的原生工作流，公开只读对象和request/独立审批/取消入口；未加QML操作界面。bootstrap历史待审与本轮真实unknown分集合，整组最多32个选中地址只可解除restart/history holds，真实unknown、实际owner、任务、临时任务、隔离和everObserved历史不清除。

原Vehicle/MM/参数/VLM/MVM/model及主链路共享所有者冻结，新鲜2秒内原始HB未解锁与已知ON_GROUND、完整本机队列排空进入30秒复核。专用fleet.occupancy.recover.approve强制不同真实账号，一次消费不生成lease；原账号TTL、当前Episode/角色在最终无回调尾栅再核。现场处置编号和地面检查是明确人工声明，不认证物理停机、远端无任务或控权。

所有可回调读取后，整组再以纯native原RAW语义revision/hash/value/鲜度/队列核对，并核原worker、实际占用和任务map；同栈整体移除本地待审后bump，新actual身份须由原worker实际提交才恢复既有continuity getter/CAS。留存原QQ、源epoch与primary控制块，换源在getter/最终CAS即时重加hold，不只等timer；正常收包、合法起飞和新MM工作不冒充换源。

本批12源码/工程登记、17文档，共29路径。原生recoveryWorkflowImplemented与UI集成分开；localHoldsReleased仅历史动作事实，selectedReviewRequiredVehicleIds另报当前待审。仍缺UI、离线/报废现实处置、本轮真实unknown受控解除和完整机载自愈；完整25目标/六类实质开发保持。未编译、测试、运行helper/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方401–405及更早是历史状态，不以旧“无任何再审入口”覆盖本批native入口，也不宣称全部恢复产品已完成。

### 406–410 精确增量路径

本批12个源码/工程登记路径与17份文档，共29路径；不是29个产品目标，也不是可独立构建的12文件包。交接须保留完整当前工程及此前相依增量，不只复制本表。

源码/工程登记：

- [custom/custom.pri](../../custom/custom.pri)
- [custom/src/Swarm/FleetOccupancyRecoveryEvidence.h](../../custom/src/Swarm/FleetOccupancyRecoveryEvidence.h)
- [custom/src/Swarm/FleetOccupancyRecoveryEvidence.cc](../../custom/src/Swarm/FleetOccupancyRecoveryEvidence.cc)
- [custom/src/Swarm/FleetOccupancyRecoveryReview.h](../../custom/src/Swarm/FleetOccupancyRecoveryReview.h)
- [custom/src/Swarm/FleetOccupancyRecoveryReview.cc](../../custom/src/Swarm/FleetOccupancyRecoveryReview.cc)
- [custom/src/Swarm/SwarmController.h](../../custom/src/Swarm/SwarmController.h)
- [custom/src/Swarm/SwarmController.cc](../../custom/src/Swarm/SwarmController.cc)
- [custom/src/Swarm/FleetOperatorSession.h](../../custom/src/Swarm/FleetOperatorSession.h)
- [custom/src/Swarm/FleetOperatorSession.cc](../../custom/src/Swarm/FleetOperatorSession.cc)
- [custom/src/Swarm/FleetRolePolicy.cc](../../custom/src/Swarm/FleetRolePolicy.cc)
- [src/Vehicle/Vehicle.h](../../src/Vehicle/Vehicle.h)
- [src/Vehicle/Vehicle.cc](../../src/Vehicle/Vehicle.cc)

文档：

- [README.md](../../README.md)
- [MODIFIED_FILES.md](../../MODIFIED_FILES.md)
- [docs/PROJECT_STATUS.md](../PROJECT_STATUS.md)
- [docs/MERIVUS_FLEET_OS_ROADMAP.md](../MERIVUS_FLEET_OS_ROADMAP.md)
- [docs/MERIVUS_SWARM_TARGET_MATRIX.md](../MERIVUS_SWARM_TARGET_MATRIX.md)
- [docs/INDEX.md](../INDEX.md)
- [docs/handoff/README.md](README.md)
- [docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md](MERIVUS_SWARM_ENGINEER_HANDOFF.md)
- [docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md](MERIVUS_SWARM_PATCH_MANIFEST.md)
- [docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md](MERIVUS_SWARM_STATIC_AUDIT.md)
- [docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md](MERIVUS_ACK_IDENTITY_AUDIT.md)
- [custom/src/Swarm/FleetNativeOccupancyEvidence.md](../../custom/src/Swarm/FleetNativeOccupancyEvidence.md)
- [custom/src/Swarm/FleetMissionContinuityPlanner.md](../../custom/src/Swarm/FleetMissionContinuityPlanner.md)
- [custom/src/Swarm/SwarmMissionOrchestrator.md](../../custom/src/Swarm/SwarmMissionOrchestrator.md)
- [custom/src/Swarm/FleetOccupancyJournal.md](../../custom/src/Swarm/FleetOccupancyJournal.md)
- [custom/src/Swarm/FleetOperatorSession.md](../../custom/src/Swarm/FleetOperatorSession.md)
- [custom/src/Swarm/FleetOccupancyRecovery.md](../../custom/src/Swarm/FleetOccupancyRecovery.md)

## 401–405：原生异步占用留存与重启待审

[占用留存合同](../../custom/src/Swarm/FleetOccupancyJournal.md)已把严格11键Codec和单槽合并worker接到实际Swarm构造/原生timer，不依赖无人调用的save/load或QML可写路径。固定AppData目录、本地合作锁、原文件基线/目录身份前后核验、无直接覆盖回退的原子替换及读回，与当前实际源身份分开；不是完整捕获、write-ahead或断电durable证明。

每个新Swarm先为1–255协议地址建立独立restart-review holds。历史ID只并入真实未知，不重建owner、任务、Run、账号或权限；空/缺失/旧open文件和写入成功都不能解除这些holds。纯重启占位不冒充已观察到的飞机记录。当前尚无专用再审解除流程，所以本阶段连续性改派保持待再审限制，不能称恢复后已可调度；此流程仍须继续开发。

公开native占用快照必须等原worker bootstrap、当前完整身份已实际提交并读回，且actual==offered==committed；存储失败/失源/坏文件/代次耗尽拒连续性提交。磁盘IO只在worker线程，原Land/RTL/ABORT不等IO、不因账本失败新增否决；析构只协作stop，不写clean或释放飞行占用。只读状态标明last-poll和recoveryWorkflowImplemented=false。

本批7源码/工程登记、15文档，共22路径；不是全部25产品目标、安装包或完整交付。QSaveFile结果仅记atomicReplacementConfirmed，osSyncVerified/durableVerified始终false。未知授权解决、正式受控执行/StartPermit、可信性能、企业跨站、长期运营及真实模型/生态等六类仍有实质源码工作。仅源码和限定文本读回；未编译、测试、运行解析/helper/业务/认证/应用/SQL/仿真/设备，未改PX4、刷机、更新PR或打包。下方396–400及更早为历史，不覆盖本节新的重启待审门禁。

## 396–400 本轮实际文件范围

12个源码/工程登记路径：

- `custom/custom.pri`
- `custom/src/Swarm/FleetNativeOccupancyEvidence.h`
- `custom/src/Swarm/FleetNativeOccupancyEvidence.cc`
- `custom/src/Swarm/CommandTransaction.h`
- `custom/src/Swarm/CommandTransaction.cc`
- `custom/src/Swarm/SwarmController.h`
- `custom/src/Swarm/SwarmController.cc`
- `custom/src/Swarm/SwarmMissionOrchestrator.h`
- `custom/src/Swarm/SwarmMissionOrchestrator.cc`
- `custom/src/Swarm/FleetMissionContinuityPlanner.h`
- `custom/src/Swarm/FleetMissionContinuityPlanner.cc`
- `custom/res/Merivus/GuidedActionsController.qml`

15份文档：

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetNativeOccupancyEvidence.md`
- `custom/src/Swarm/FleetMissionContinuityPlanner.md`
- `custom/src/Swarm/SwarmMissionOrchestrator.md`
- `custom/src/Swarm/FleetReplacementMatcher.md`

合计27路径，仅为累计脏工作树本批触点，不是25项产品完成、独立安装包或全部历史修改量。未提交/更新PR、生成包或改飞控。

## 396–400：实际本地占用进入连续性审阅与提交

[原生占用合同](../../custom/src/Swarm/FleetNativeOccupancyEvidence.md)新增actual Swarm快照和最终本地角色CAS守门。部署撤权不删除raw占用；编队、创建/预抢占栈、活事务、安全owner、临时任务/清理及内容隔离分别保留。已知结果不明不会因原记录清理变成可用；当前账本没有自动清除或跨重启恢复证明。

Continuity排除所有held/unknown候选；原成员任一实际占用即整任务hold，不通过换掉成员来假装释放。Guided刷新/应用要求原实际快照，稳定原生身份进入建议；缺失或坏证据不能产生产品可应用的完整提案。旧孤立值诊断仍明确requiresNativeOccupancyReview，不授来源认证。

Swarm与Orchestrator双向sticky pin，源死不换空实例。新六参数角色CAS在同栈提交前核冻结身份与old∪new实际状态；绑定后的旧本地入口也不绕过当前占用检查。不改实际owner、不发命令、不解除native Run保护；只代表本地模型变更，不是飞机空闲、正式机载改派或控制租约。

本批12源码/工程登记触点、15份文档，是累计工程增量，不是完整25目标完成或安装包。包括直接相关的CommandTransaction通知后存活/原尝试检查及嵌套start/retry保护，不是全仓回调安全认证。未知状态的授权解决/跨重启留存、完整受控机载执行、性能/能量与企业跨站等六类仍继续。仅源码与限定文本审阅，未编译、测试、解析/运行helper、业务/认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅摘要占用/五参产品应用”的描述由本节覆盖。

## 391–395 本轮实际文件范围

10个源码/工程登记路径：

- `custom/custom.pri`
- `custom/src/Swarm/FleetReplacementMatcher.h`
- `custom/src/Swarm/FleetReplacementMatcher.cc`
- `custom/src/Swarm/FleetRoleAllocator.h`
- `custom/src/Swarm/FleetRoleAllocator.cc`
- `custom/src/Swarm/FleetMissionContinuityPlanner.h`
- `custom/src/Swarm/FleetMissionContinuityPlanner.cc`
- `custom/src/Swarm/SwarmMissionOrchestrator.h`
- `custom/src/Swarm/SwarmMissionOrchestrator.cc`
- `custom/res/Merivus/GuidedActionsController.qml`

16份文档：

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetReplacementMatcher.md`
- `custom/src/Swarm/FleetRoleAllocator.md`
- `custom/src/Swarm/FleetMissionContinuityPlanner.md`
- `custom/src/Swarm/SwarmMissionOrchestrator.md`
- `custom/src/Swarm/FleetCapabilityEvidence.md`

合计26路径，只是累计工程本轮触点；没有生成新安装包/源码包或更新PR，不能只拿这10文件替代累计完整依赖。

## 391–395：角色备用覆盖、整组替补与本地角色CAS

[一对一替补合同](../../custom/src/Swarm/FleetReplacementMatcher.md)新增有界、确定性增广匹配。RoleAllocator先核全局与具体角色声明，再挑选能接替实际主角色的备用；单角色可覆盖与多机同时缺失的覆盖分开，一架通用备用不能重复计算。原主角色feasible不改名成备用保障或实测性能。

连续性从任务自身的完整roleAssignments读取角色；有角色政策却缺来源时要求人工补齐，不用Registry显示身份猜专业角色。Leader与成员全部缺口在同一图中匹配，完整才生成一个完整替补提案；不足只作hold/split建议。同批建议候选承诺另账排他，完整映射进入稳定建议身份，不冒充实际Swarm/机载占用。

原生角色感知CAS在同一任务revision下提交成员、Leader和角色图；旧ID-only入口不能照搬过期图。Guided保留当前机队/任务占用重算、原权限与明确确认，只消费完整当前提案，不静默丢健康成员或另外的故障缺口。native Run仍禁止通用连续性接管；本地任务模型改变不是机载变更或自动自愈成功。

本轮10个源码/工程登记触点、16份文档；不改PlanStore计划schema或旧批准、不加UI按钮/飞行命令、不放宽最终StartPermit。角色等价实测性能、实际占用/受控执行、可信能力/能量模型、跨站权限和原25目标/六类仍有实质开发。仅源码及限定文本审阅，未编译、测试、解析/运行helper、认证、应用/UI、SQL、仿真、设备、刷机、修改PX4、更新PR或打包。下方较早批次为历史，旧“仅通用备用/分开的残缺提案/ID-only应用”描述不覆盖本节。

## 386–390 本轮实际文件范围

14个源码/工程登记路径：

- `custom/custom.pri`
- `custom/res/Merivus/GuidedActionsController.qml`
- `custom/src/Swarm/FleetCapabilityEvidence.h`
- `custom/src/Swarm/FleetCapabilityEvidence.cc`
- `custom/src/Swarm/FleetRegistry.h`
- `custom/src/Swarm/FleetRegistry.cc`
- `custom/src/Swarm/FleetCapabilityDiscovery.cc`
- `custom/src/Swarm/VehicleCapability.cc`
- `custom/src/Swarm/FleetCapabilityMatcher.cc`
- `custom/src/Swarm/FleetRoleAllocator.cc`
- `custom/src/Swarm/FleetTaskTemplateRegistry.cc`
- `custom/src/Swarm/FleetMissionContinuityPlanner.cc`
- `custom/src/Swarm/FleetMissionCompiler.cc`
- `custom/src/Swarm/FleetMissionReviewCoordinator.cc`

另同步18份文档：

- `README.md`
- `MODIFIED_FILES.md`
- `docs/PROJECT_STATUS.md`
- `docs/MERIVUS_FLEET_OS_ROADMAP.md`
- `docs/MERIVUS_SWARM_TARGET_MATRIX.md`
- `docs/INDEX.md`
- `docs/handoff/README.md`
- `docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md`
- `docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md`
- `docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md`
- `docs/handoff/MERIVUS_ACK_IDENTITY_AUDIT.md`
- `custom/src/Swarm/FleetCapabilityEvidence.md`
- `custom/src/Swarm/FleetCapabilityDiscovery.md`
- `custom/src/Swarm/FleetCapabilityMatcher.md`
- `custom/src/Swarm/FleetRoleAllocator.md`
- `custom/src/Swarm/FleetMissionContinuityPlanner.md`
- `custom/src/Swarm/FleetMissionReviewCoordinator.md`
- `custom/src/Swarm/FleetMissionCompiler.md`

合计32路径，为累计脏工作树的本轮触点，不是全部历史改动量、独立补丁安装包或验收进度；后续交接必须包含累计完整工程依赖，不能只复制本轮14文件。未生成新源码包/安装包、提交、更新PR或修改飞控。

## 386–390：构型绑定的本地工程能力审阅

[当前合同](../../custom/src/Swarm/FleetCapabilityEvidence.md)把遥测/清单声明与当前本地工程审阅分开。Registry 私有持有审批上下文与最新记录；构型、静态依据、批准或撤销推进 revision，A→B→A、重新注册或护照导入不恢复旧批准。普通自有快照回显须与实际私有投影完全一致，不把复制字段当授权。

Matcher、角色、模板、连续性和最终任务成员统一消费当前记录；手工分配不能绕过显式能力政策。Guided 的稳定机队身份与审阅归档纳入该记录，普通收包/时间增长不因此触发构型重审。旧 verified 标签仅保留为声明，不再自动获得当前审阅。

本批14个源码/工程登记文件、18份文档，是累计工程增量，不是完整产品或独立安装包。没有新增授权界面或飞行命令；本地工程声明不认证实物、实测性能、账户或最终飞行许可。角色等价替补、可信能力/性能来源、完整执行闭环和原25目标/六类实质开发仍继续。仅源码及限定文本审阅；未编译、测试、运行 helper/认证/应用/UI/SQL/仿真/设备、改PX4、刷机、更新PR或打包。下方较早批次为历史，旧协议/manifest自动可信描述不覆盖本节。

### 381–385 本轮文件范围（不是独立安装包）

本轮相关源码/工程登记12个：

- `src/Vehicle/Vehicle.h`
- `src/Vehicle/Vehicle.cc`
- `custom/custom.pri`
- `custom/src/Swarm/FleetMissionStartupEstimator.h`
- `custom/src/Swarm/FleetMissionStartupEstimator.cc`
- `custom/src/Swarm/FleetMissionStartReadiness.cc`
- `custom/src/Swarm/FleetMissionStartupTransition.cc`
- `custom/src/Swarm/FleetMissionArmedGroundReview.cc`
- `custom/src/Swarm/FleetMissionStartReview.cc`
- `custom/src/Swarm/FleetMissionStartupTransaction.h`
- `custom/src/Swarm/FleetMissionStartupTransaction.cc`
- `custom/src/Swarm/FleetMissionExecutionObservation.cc`

另同步21份相关文档（含新 `FleetMissionStartupEstimator.md`）；这些是累计脏工作树中的本轮触点，不是全部累计修改量，也不能只拷这12个文件替代完整工程。未生成新安装包、提交、更新PR或改飞控。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 376–380：原生健康时效与同任务观测结果

[健康证据](../../src/Vehicle/VehicleHealthTelemetry.md)新增实际PRIMARY原包SYS_STATUS/ESTIMATOR_STATUS独立双通道：各自本机单调年龄、receipt、语义revision与来源失效，旧缓存/机队定时刷新不再续age=0。Swarm只在两必需流均新鲜时给故障评估Known、age取最大值；位置另须新鲜原生GLOBAL或LOCAL与估计器位。ESC/FTC/arming缓存只能保守增加坏值或未知，不能替代估计器或补新鲜；HealthCenter以Registry为许可上限，旧FTC不能覆盖当前bad/unknown，未知Healthy=false占位也不伪造critical。

[本次Run观测](../../custom/src/Swarm/FleetMissionExecutionObservation.md)在同一受保护TaskRecord新增两个轴：历史末项/HOLD/有序LAND候选与当前观察窗口。11个有界原接收引用只取实际MODE committed之后，provisional不回填；MISSION_CURRENT原state/mode/total的零扩展仍未知，合法无任务/不匹配先保留负报告。来源退休/owner销毁仅收窗口、保留历史与unknown，不自动Completed、控制或释放占用；整组只报告none/partial/all MODE commit与候选数量，不冒充整组成功。

本批7源码/19文档，仅源码和限定文本/交叉审阅，不改原六flight通道、startup32收据、profile1–3或参数schema。原25目标/六类、最终StartPermit、可信HOME/local/净空、机载正式实例/结果、EST完整逐阶段断流账本及长期授权/容量生命周期仍有实质源码工作；outboundIntegrationComplete=false保持。未编译、测试、运行helper/认证/应用/UI/SQL/仿真/设备、改飞控/刷机、更新PR或打包。以下371–375及更早为历史状态，本节不提升运行/安全验收结论。

实际7源码为 `src/Vehicle/Vehicle.h/.cc`、`custom/src/Swarm/SwarmController.cc`、`FleetHealthCenter.cc`、`FleetMissionExecutionObservation.h/.cc` 和 `SwarmMissionOrchestrator.cc`（后三组在同一Swarm目录）。19文档为13份公共/工作流说明，加新VehicleHealthTelemetry与ExecutionObservation、Orchestrator、HealthCenter、VehicleMissionExecutionObserver、ProgressObservation六模块说明；合计26路径，是累计工程增量，不是七文件独立构建包。

## 371–375：8源码 + 19文档，合计27路径

相对前批的8个源码/资源路径如下；它们是累计工程增量，不是可单独构建或安装的8文件包。

- `custom/src/Swarm/FleetMissionDeploymentLifecycle.h`、`custom/src/Swarm/FleetMissionDeploymentLifecycle.cc`：两个 `ForController` 专用 transition、私有栈 permit、连续调用占用与12键结果，commit事实独立于end，不经释放后 legacy mutation。
- `custom/res/Merivus/GuidedActionsController.qml`：prepare/select 两个固定 public API、独立 JS transition ticket/busy、12键只读 metadata/6键组行、本地旧 token/身份意图协调，raw refs 留在私有实际 scope。
- 新 `custom/res/Merivus/FleetMissionLifecyclePanel.qml`：本地组/历史选择、双步确认、默认关闭/取消、不以 Enter 提交、view/press ticket。
- `custom/res/Merivus/CommandCenterOverlay.qml`、`custom/res/Merivus/FleetMissionPlannerPanel.qml`：各一处面板实例；`custom/qgroundcontrol.qrc`、`custom/res/Merivus/qmldir`：新组件资源/模块登记。

19文档为13份公共状态/清单/交接/StartupTransaction/OperatorWorkflow，加 `FleetMissionDeploymentLifecycle.md`、`FleetMissionControllerScope.md`、`FleetOperatorSession.md`、`FleetMissionNativeOwnerCallbacks.md`、`FleetMissionObservationPanel.md` 与新增 [FleetMissionLifecyclePanel.md](../../custom/src/Swarm/FleetMissionLifecyclePanel.md)。本批不改原18键 switchEvidence、22键 scope、传输/账号 schema、飞控或控制参数。

prepare 不自动切 selected、登录/继承 grant、停止旧 Run 或释放占用；失败尝试仍计8组上限，`localObjectGroupAllocated=false` 不证明未计费。`outboundIntegrationComplete=false`、25目标/六类保留。限定文本检查记录已同步本轮静态审计；未编译、测试、解析/运行 helper、认证、应用/设备，未更新 PR/包。以下366–370及更早清单保留为历史，不以旧无入口/API说明覆盖本批。

## 366–370：5源码 + 18文档的增量范围

30入口指业务范围，`runCurrent`另含登录/退出2个，`runObservation`有4个；24为来源/上下文回调。raw引用/private id、public entry→private bridge、callback→原scope helper须区分，不声明所有函数private。

这些路径只列相对前批的增量，不能仅复制5源码构建；交接须包含累计完整源码、pri、注册及原生依赖。未知/已queue不回滚，无法核实原scope撤销时 `retiredCurrentScope` 持续阻同组current入口/回调，selected仍可读。Qt exact Int/QObjectStar往返、parent与销毁尚未验证；未打包/更新PR。

源码为 [Lifecycle.h](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.h)、[Lifecycle.cc](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.cc)、[CustomPlugin.cc](../../custom/src/CustomPlugin.cc)、[GuidedActionsController.qml](../../custom/res/Merivus/GuidedActionsController.qml)、[FleetMissionObservationPanel.qml](../../custom/res/Merivus/FleetMissionObservationPanel.qml)。分别新增三 native call/忙门禁、两类注册、私有工厂14引用与30入口/24回调/明确身份意图/retained Close、selected token/完整收据取消；加18份既有文档，本批合计23路径。不新增资源文件，不改 PX4、控制参数、原18键 switchEvidence 或原22键 scope。

initial 已由构造提供，不自动 prepare；scope 非许可、无 legacy fallback/公开 raw refs。finally 只结束原 owner+ID，pure 回调不因 busy 自拒，未知/已 queue 不回滚；四个观察值同 selected scope，token 含选择代次，取消只走原 issued 收据。新部署/选择入口及专用 transition API 仍缺，`outboundIntegrationComplete=false`，原25目标/六类继续。已完成限定文本审阅，具体证据见本轮静态审计，不是运行或安全通过，未编译、测试、运行、更新PR或打包。以下清单为历史增量。

## 361–365：原生引用scope与全进程认证提示栈

[控制器作用域契约](../../custom/src/Swarm/FleetMissionControllerScope.md)新增五个纯native查询：current、selected观察和按实际Close寻原组的三种22键引用scope及精确比对。factory epoch/episode/14owned实际对象与独立outbound/selection代次分开；retained-close不要求current/selected。map内raw QObject只比较地址，不解引用调用者旧值；引用匹配不代表shared5/账户/任务/风险/权限被冻结。

Session两个认证入口现共用单进程GUI提示栈锁，从任何OS/identity/changed前保持到整函数退出，原Session被毁或_promptActive提前false仍不放锁，nested拒绝不清凭据。Lifecycle切代/改选择在入口、回调后及最终mutation前实时拒绝提示busy；pure scope与原identity/fullgate/Close消费不受该busy判定而自拒。current.identityPromptBusy仅at-read诊断，无全局实时NOTIFY保证，不是门禁缓存或跨站租约。

实际修改Lifecycle/Session两组h/cc共4源码，17文档；没有QML注册/14对象迁移/新部署或历史入口，外层QML scope/finally、shared Role身份意图与selected Run/Series/Close/Session整组投影及界面模态协调仍待接线，outboundIntegrationComplete=false保持。原25目标/六类继续；只改源码/文档与限定静态审阅，未编译、测试、执行helper/认证/API/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

本轮实际4源码：

- `custom/src/Swarm/FleetMissionDeploymentLifecycle.h`、`custom/src/Swarm/FleetMissionDeploymentLifecycle.cc`：5个纯scope入口、22键严格型引用/独立代次、actualClose原组及raw地址安全、切代/选择提示busy与at-read诊断。
- `custom/src/Swarm/FleetOperatorSession.h`、`custom/src/Swarm/FleetOperatorSession.cc`：non-invokable static提示忙getter、两认证全函数RAII token；不改OS提示实现或身份期限。

没有新增工程路径/注册/QML编辑；17文档为16份既有状态/清单/模块/交接及新增`custom/src/Swarm/FleetMissionControllerScope.md`。本轮限定文本范围20源码/工程路径，不把它当编译覆盖。四文件仍未跟踪，空tracked diff不能覆盖它们；不能把基于HEAD的累计行数当本轮补丁规模，不能漏交这些实际文件或把本轮当独立可编译/可安装产品包。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](../../custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

本轮实际13源码：

- `custom/src/Swarm/FleetMissionDeployment.cc`
- `custom/src/Swarm/FleetMissionEvidenceRefresh.cc`
- `custom/src/Swarm/FleetMissionStartContentCheck.cc`
- `custom/src/Swarm/FleetMissionStartReview.cc`
- `custom/src/Swarm/FleetOperationApproval.cc`
- `custom/src/Swarm/FleetMissionStartApproval.cc`
- `custom/src/Swarm/FleetMissionStartupAuthority.cc`
- `custom/src/Swarm/FleetMissionGroundStartupAuthority.cc`
- `custom/src/Swarm/FleetMissionArmedGroundReview.cc`
- `custom/src/Swarm/FleetMissionGroundStartApproval.cc`
- `custom/src/Swarm/FleetMissionStartupTransaction.cc`
- `custom/src/Swarm/FleetMissionObservationClose.cc`
- `custom/res/Merivus/GuidedActionsController.qml`

本轮未改header、pri/qrc/CustomPlugin或磁盘格式。12cc均未跟踪，不能只导出tracked diff漏掉真实源码。26文档包含25份既有清单/状态/模块/交接说明和新增`custom/src/Swarm/FleetMissionNativeOwnerCallbacks.md`；其中9份模块文档的11行旧回调签名已原位同步。既有Git总numstat含此前累计修改，不是本轮行数。没有生成新PR/包。

## 351–355：普通部署的底层逐包发送范围

[原生传输契约](../../custom/src/Swarm/FleetMissionDeploymentTransport.md)为Deployment普通上传/回读接入私有PlanManager/Vehicle scoped路径：固定原实际owner/Vehicle/MM/primary控制块/部署/代次/操作号，纯范围检查不调用OS/QML，完整门禁在插件前后及最终queue前复核。COUNT/ITEM_INT/REQUEST_LIST/REQUEST_INT/末尾ACK及重试均走该路径；closed scope直到原栈退出才清理，不回落legacy。原公开普通接口和独立RAW读取不继承该许可。

实际queue先记issued再通知；真实COUNT(0)与未知-1分开，未queue末尾ACK不能报回读成功。原Vehicle sender忙纳入本地quiet及Deployment排空，MM失效不把仍活的发送栈当已退出；Episode诊断记录实际捕获primary。不是原批准payload逐字段再认证、机载原子快照或StartPermit。

本轮实际修改7个源码文件；没有新QML类型/界面/部署入口，Guided实际Episode/epoch、shared Role身份意图、全retained modal忙锁及selected旧观察整组投影仍待开发，outboundIntegrationComplete=false保持。原25目标/六类继续；仅源码/文档和限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史批次，不以旧的core缺口说明覆盖本轮源码进展。

本轮7个实际源码路径（不含此前累计改动）：

- `src/MissionManager/PlanManager.h`、`src/MissionManager/PlanManager.cc`：私有范围、入口与逐包路由，旧公开API/独立RAW行为分开。
- `src/Vehicle/Vehicle.h`、`src/Vehicle/Vehicle.cc`：新私有sender、sender busy/quiet及实际queue事实。
- `custom/src/Swarm/FleetMissionDeployment.h`、`custom/src/Swarm/FleetMissionDeployment.cc`：原生pure上下文、捕获primary、普通upload/readback及排空。
- `custom/src/Swarm/FleetMissionDeploymentEpisode.cc`：原固定诊断结构中填入实际捕获primary，schema不变。

文档为15份既有文档顶部状态同步及新增`custom/src/Swarm/FleetMissionDeploymentTransport.md`，共16份；源码/关联工程限定文本检查22路径。未新增pri/qrc注册。git基线含此前累计改动，不能把其总numstat当成本轮行数；未跟踪的Deployment/Episode必须随源码交接，不能只导出tracked diff。

## 346–350：十二类原生服务（12个既有源码路径）

均位于`custom/src/Swarm/`：

- `FleetMissionDeployment.cc`、`FleetMissionEvidenceRefresh.cc`、`FleetMissionStartContentCheck.cc`、`FleetMissionArmedGroundReview.cc`。
- `FleetOperationApproval.cc`、`FleetMissionStartReview.cc`、`FleetMissionStartApproval.cc`、`FleetMissionStartupAuthority.cc`。
- `FleetMissionGroundStartApproval.cc`、`FleetMissionGroundStartupAuthority.cc`、`FleetMissionStartupTransaction.cc`、`FleetMissionStartupSeries.cc`。

新请求、live依据、原scoped prequeue、消费/移交前后接current/sticky实际owned peer同组；Series一次真实未发布bootstrap单独处理。Authority/Txn认证移交上下文改pure native，完整OS/风险门禁仍在外层；3处primary LinkGUI线程误拒已删，Vehicle/MM/PM/Links线程不放宽。原cancel/revoke/drain、旧Run/Close及queue/uncertain记账保留，无头/API/receipt/磁盘schema变更。

限定检查35路径：本12组h/cc、Episode/Lifecycle/Session/Close/Run五组h/cc及custom.pri；25文档为原公共13份、本12模块说明（Txn已计入公共13，另11份）及Lifecycle说明。最终结果见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。

剩余真实边界：普通Deployment legacy PlanManager每包/重试/插件后prequeue无Episode fence；Root/QML actual Epoch/共享Role/selected旧观察身份与入口仍未接入，outboundIntegrationComplete=false。未改core Vehicle/PlanManager或注册类型/新部署UI，不编译、测试、运行认证/API/helper/应用/设备、更新PR或打包；原25目标/六类继续。

## 345：非 Close 身份用途接线（与核心合并23路径）

新增实际增量为`custom/src/Swarm/FleetOperatorSession.cc`，及`FleetOperatorSession.md`说明；Episode.h/.cc的native `outboundOwnersCompatible`和Session outbound策略包含在下方核心22路径中，不重复计数。合并限定范围为23个源码/工程路径、18份文档。

非Close审批和upload/start/ground消费、移交、Current证明受current Episode/实际同组配对限制；OS/owner回调后和不可逆消费/移交前复核。普通身份事实、显式登录/退出、Close专用用途及精确撤销/清理保留。未接其余12类及Guided全链、未注册QML/新部署入口；未编译、测试、运行认证/应用/设备、更新PR或打包。最终有限文本检查见[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)。

## 340–344：新部署生命周期原生核心（未接 QML）

核心源码/登记22路径（后续345的Session实现另列）：

- 新4文件：`custom/src/Swarm/FleetMissionDeploymentEpisode.h/.cc`、`FleetMissionDeploymentLifecycle.h/.cc`。
- 既有16个 friend headers：`FleetMissionDeployment.h`、`FleetOperatorSession.h`、`FleetOperationApproval.h`、`FleetMissionEvidenceRefresh.h`、`FleetMissionStartContentCheck.h`、`FleetMissionStartReview.h`、`FleetMissionStartApproval.h`、`FleetMissionStartupAuthority.h`、`FleetMissionStartupTransaction.h`、`FleetMissionArmedGroundReview.h`、`FleetMissionGroundStartApproval.h`、`FleetMissionGroundStartupAuthority.h`、`FleetMissionStartupSeries.h`、`FleetMissionObservationClose.h`、`FleetMissionExecutionObservation.h`、`SwarmController.h`（均位于 `custom/src/Swarm/`）。
- `custom/custom.pri`：新四个 h/cc 各登记一次；不是 QML 类型注册。
- `custom/src/Swarm/FleetMissionStartupSeries.cc`：新ground child公开前继承实际Series的sticky managed/owner原生来源；14个owned头同时保存私有provenance，不通过可写动态属性或parent变更将旧对象降格为legacy。

核心文档17份：README、MODIFIED_FILES、PROJECT_STATUS、ROADMAP、TARGET_MATRIX、INDEX、handoff README/ENGINEER_HANDOFF/PATCH_MANIFEST/STATIC_AUDIT/ACK_IDENTITY、Txn/OperatorWorkflow/Series/ExecutionObservation/Orchestrator 说明，以及新增 [Lifecycle.md](../../custom/src/Swarm/FleetMissionDeploymentLifecycle.md)；345另含Session说明，合并18份。限定检查结果统一见本文件上方静态审计链接。

核心新增14对象/代、fresh signed-out Session、保留旧实际组/Run/占用、五个冻结实际共享来源、全 retained native 静默证据和8次分配上限（初始/失败尝试计入，不驱逐）。**未注册 QML、未替换 Guided、无新部署入口，当前应用仍单链。** native sticky outbound helper 不代表 owned 门禁/QML epoch/source callbacks/历史 Close 身份已完成隔离；这些仍需后续代码。未编译、测试、运行业务/认证/应用/SQL/设备/刷机，未更新 PR 或打包。

## 335–339：监视面板与两个固定入口（2026-10-15）

本批实际6个源码/资源/模块路径：新 `custom/res/Merivus/FleetMissionObservationPanel.qml`；`GuidedActionsController.qml`、`CommandCenterOverlay.qml`、`FleetMissionPlannerPanel.qml`；`custom/res/Merivus/qmldir`、`custom/qgroundcontrol.qrc`。新面板资源别名/模块类型各1、入口各1，沿用原controller/backend；不加C++注册/pri或后端权限，不改持久化格式。

新 `custom/src/Swarm/FleetMissionObservationPanel.md`、四份Close/Run/Orch/Series契约与13份公共/交接/Txn/Operator说明同步，共18份文档。不得漏掉未跟踪的新QML/md；旧PR或源码包不代表本批。没有制作安装包、提交/更新PR、编译/测试、启动应用/认证/helper或操作设备。

[交互契约](../../custom/src/Swarm/FleetMissionObservationPanel.md)：严格实际Run、每页完整review token、两个入口隔离、三项确认与当前双账号原生审批、modal调用ticket、固定期限保守显示及三态结果。界面不替代native重验；本地monitor关闭不是STOP/LAND/DISARM/Completed、跨站接管或成员可重新分配。完整StartPermit/正式结果/生命周期及其它原25目标/六类继续。

## 330–334：独立关闭监视与实际占用快照（2026-10-15）

本批实际14个源码/工程路径：

- 新 `custom/src/Swarm/FleetMissionObservationClose.h/.cc`（2）。
- `FleetOperatorSession.h/.cc`、`FleetRolePolicy.cc`（3）；`FleetMissionStartupTransaction.h/.cc`、`SwarmController.h/.cc`、`SwarmMissionOrchestrator.h/.cc`（6）。
- `custom/custom.pri`、`custom/src/CustomPlugin.cc`、`custom/res/Merivus/GuidedActionsController.qml`（3）。

新增Close说明与Role说明；Session、Run、Orchestrator、Series及13份公共/交接/Txn/Operator文档同步，总计19份文档。新Close h/cc在pri各登记一次，MerivusInternal类型一次；Run仍不注册QML，controller只readonly值与确认函数，无新视觉UI。必须包含新未跟踪文件，不能只取旧PR或tracked diff。

[完整契约](../../custom/src/Swarm/FleetMissionObservationClose.md)：context18与actual ownerQQ、实际占用map/撤销独立、完整binding7/9、不同账号专用purpose、原30秒、三项确认和一次原生commit。监视结束不发飞行命令，不认证Completed、落地或成员可重新分配；释放结果缺源未知而非成功。无新持久化格式、飞控修改或设备操作，未编译/测试/执行认证/业务/helper、提交/更新PR/打包。后续正式结果/StartPermit/长期生命周期及原25目标/六类仍继续。

## 325–329：实际 Run 与受保护观察任务（2026-10-15）

源码增量10路径：`SwarmMissionOrchestrator.h/.cc`、`SwarmController.h/.cc`、`FleetMissionExecutionObservation.h/.cc`、`FleetMissionStartupTransaction.cc`、`FleetMissionContinuityPlanner.cc`、`FleetOperationalMemory.cc`、`GuidedActionsController.qml`。新增文档 `custom/src/Swarm/SwarmMissionOrchestrator.md`；其它模块/公共状态与交接文档同步。不新增QObject类型、QML类型注册或工程源路径；所有C++文件原登记继续。

实际Run从同一Swarm的一次原生关联获取Orchestrator，MODE前登记另一task UUID，完整6键来源身份及原7/9键binding不投影；所有generic变更拒绝sticky native记录。观察/结构版本和通知分开、native属性只读；真正queue fact同步保存，公共通知延迟合并。未知/监视结束不伪造任务成功，后续正式远端结果/独立关闭审批及长期容量管理仍待开发。

Planner和Memory必须同批交付，不能只拷Run/Orchestrator：否则旧Planner不识别新phase会阻断整份任务集合，旧运营统计也缺观察/成功区分。新md及未跟踪源码必须包含在工程师完整源码交接中；本阶段不制作安装包、不更新PR、不提交合并，不等于已编译可用。

325–329（2026-10-15）新增[实际 Run 的受保护任务记录](../../custom/src/Swarm/SwarmMissionOrchestrator.md)：同一原生观察源在 MODE 前登记独立任务 UUID，冻结 Run/启动/部署/完整7或9字段 binding，通用改派/完成/进度等不接管；观测与控制版本、通知分开，监视结束或销毁只记 observation-closed，不冒充正式完成。连续性保留成员占用并要求原生工作流，运营统计不把观察关闭算飞行成功；原25目标/六类及正式结果认证继续。仅源码/限定静态审阅，未编译、测试、运行应用/helper、改PX4、设备/刷机、更新PR或打包。下方320–324及更早是历史记录。

## 320–324：显式 HOME 观测预算与接续（2026-10-15）

新增 `custom/src/Swarm/FleetMissionStartupHome.h/.cc/.md`。源码/工程增量另含 ExecutionProfile.cc、StartReadiness.cc、StartupTransaction.cc、StartupTransition.cc、StartReview.cc、FleetMissionDeploymentPanel.qml 和 custom.pri，总计9个源码/工程路径；19份配套/公共文档同步。新helper h/cc在pri各一次，无QObject/QMLtype注册，不新增磁盘格式、传感器或飞控修改。

schema3仅由全新完整计划明确提供，保留v2全部观察约束；新增HOME包线恰8字段，两个次数上限和五个米制上限均无默认。原始基线不滚动，真实完整coordinate事件计次，geo双叶配对和coords-only分开；relative-only仅真实ARM stage1，现有ACK/HB/地面与组屏障保持。两风险revision域/最后实际当前值核对不绕开。v1/v2解析形状/政策原样保留。

StartReview逐机artifact保留本次完整规范政策，面板明示全部限制/单位/未知态，不自动已阅或开放执行按钮。只凭wire hash相同不能沿用旧批准，完整record/plan/profile/审阅身份仍须匹配。没有编译、测试、运行helper/应用或更新PR/安装包；工程师须取包含新未跟踪文件的完整源码交接，不得只导出已跟踪diff。

320–324（2026-10-15）新增[显式 HOME 观测过渡](../../custom/src/Swarm/FleetMissionStartupHome.md)：独立 schema3 九字段及 HOME 八字段包线，按原冻结基线累计次数/位移；真实 owned ARM/MODE 的双 HOME 原始叶配对、ARM 阶段 relative-only 和独立风险 revision 已接线。v1/v2 不自动升级，原 ODOM/gap/ACK/整组门禁保持；风险审阅按机显示冻结范围与单位，不补默认或新增执行按钮。仅源码与限定静态审阅，不证明 HOME 内部有效性、同采样、净空或最终许可；原25目标/六类继续，未编译/测试/运行应用或 helper、改 PX4、刷机、更新 PR 或打包。下方315–319及更早是历史记录。

## 315–319：连续 Run 观察与本地占用移交（2026-10-15）

新增 core `src/Vehicle/VehicleMissionExecutionObserver.h/.md`、custom `FleetMissionExecutionObservation.h/.cc/.md`；修改 Vehicle.h/.cc、core CMakeLists/qgroundcontrol.pro、StartupTransaction.h/.cc、SwarmController.h/.cc、custom.pri 和 readonly controller，实际13个源码/工程路径。根状态/清单/审计/工程交接与模块文档共18份同步；不是全工作树总量或25目标完成计数。

只在真实 MODE 排队前私有绑定独立原生观察，write调用返回后保存 captured commit；provisional不回填post-commit，原receipt32与磁盘格式不变。旧启动授权结束仍保留运行观测及partial事实，协议排空后用另一UUID原子移交整组local占用；旧release不影响新Run，安全撤销不重建。QML只有只读值，无新启动/停机按钮或Run类型注册；native明确停止监视不等于停止飞行。

315–319（2026-10-15）新增[本次启动的连续运行观测](../../custom/src/Swarm/FleetMissionExecutionObservation.md)：真实MODE write前独立原生切点、captured queue commit、post-commit进度与原子整组本地占用移交已接线；启动认证退休不抹已排队事实，也不终止独立观察。provisional不回填正式进度，旧样本明确lastObserved，HOLD/LAND候选不冒充完成。未编译/测试/运行API或操作设备，未改PX4/更新PR/包；原25目标/六类及最终许可/正式结果归属继续。下方310–314及更早为历史批次。

## 310–314：目录枚举错误边界（2026-10-15）

新增 `custom/src/Swarm/FleetArchiveDirectoryInventory.h/.cc/.md`，在 `custom/custom.pri` 登记 h/cc 各一次。源码增量另含遥测与事件的两组 Worker/HistoryReader `.cc`、两个根门面 `.cc`、Telemetry Codec `.cc`，共10个源码/工程路径。模块文档8份及公共交接/状态文档13份同步，共21份文档；本数字只统计本轮，不是整个工作树总量。

原遥测 Header19/Frame29/Footer31、catalog entry8/page request11 与事件 SQL schema/entry7 保持；仅成功 native catalog 顶层新增 `directoryInventoryEnumerationCompleted` 的真实 bool 事实并在根门面复核。Telemetry Worker 错误白名单23项，未新增执行接口、QML 类型或权限恢复。新 helper 失败清空 partial inventory，未支持平台拒绝，不回退 Qt iterator。

310–314（2026-10-15）新增[错误可区分的原生目录清点](../../custom/src/Swarm/FleetArchiveDirectoryInventory.md)：遥测与封存事件的两个writer/两个reader均接入，正常EOF、取消/失败、关闭和目录对象检查分开；两个native catalog门面要求真实枚举完成旗标。原磁盘格式、分页、控制与权限不变，不把完成枚举当全集/认证/原子快照。仅源码及限定文本审阅，未编译/测试/运行helper或操作设备；未更新PR/包，原25目标/六类剩余范围保留。下方305–309及更早是历史批次。

## 305–309 对应路径

- `custom/src/Swarm/FleetTelemetryArchiveCodec.h/.cc/.md`：原frame格式不变，新增非QObject Header19/Footer31验证、读政策4，Footer核实际行扫描3镜像，不伪造磁盘lastFlushSeq。
- 新 `custom/src/Swarm/FleetTelemetryArchiveHistoryReader.h/.cc/.md`：独立readOnly QFile/目录、有界完整行/整文件页与重开hash，支持invalid候选诊断/有效前缀/partialTail，不创建锁/档案或回灌live状态。
- `custom/src/Swarm/FleetTelemetryArchive.h/.cc/.md`：native catalog/page/cancel与最新detail；actualcatalogId/private source/实际filter+定位+hash游标，终态交付且actualfinished才释放单槽，固定QML元数据投影。
- `custom/custom.pri`登记Reader h/cc各一次；`custom/res/Merivus/GuidedActionsController.qml`仅追加historyCatalog/historyPage值，无新QMLtype/视觉按钮/文件接口。
- 共同修改清单/路线图/目标矩阵/索引/审计/交接与旧流程说明同步305–309；限定文本检查16源码/工程路径、18文档，含未改writer/core来源作契约核对。

本批修正文档格式/flags/queryId/null空值的实际对接错误，确认构造4arg/11键请求/8键descriptor/6键publicrow与来源/预算域一致。尚无真正运行/编译验证，不生成或更新PR/补丁包/安装包。

305–309 新增[遥测只读历史检索](../../custom/src/Swarm/FleetTelemetryArchive.md)：Codec共享Header19/Footer31及readPolicy4、parentless目录/整文件页Reader、native实际catalog/每文件filter+定位+hash游标、合作取消与fixed public元数据已接线。最新raw页仅C++确认获取，新请求清旧raw，终态交付且actualfinished才释放槽；不建/写档案、不回灌/控制。完整行前缀、partialTail/缺footer与非法内部记录分开，实际scope/序号/三计数镜像及重开hash复核不认证身份/全集/closed或durable。只改源码/文档，未运行读取/hash/解析helper/编译/测试/SQL/设备，未改PX4/刷机/更新PR/打包；原25目标/六类继续，下方300–304及更早为历史批次。

## 300–304 对应路径

- 新 `src/Vehicle/VehicleTelemetryObserver.h/.md` 与 `Vehicle.h/.cc`：抽象native观察者、actual注册机/PRIMARY、单实例UUID/序号、源退休和派生析构解绑；不是逐包Qt信号或控制API。
- 新 `custom/src/Swarm/FleetTelemetryArchiveCodec.h/.cc/.md`：19基础+8单独定位同意的固定消息，原len payload、29字段与严格镜像；无UID/自由文本/签名原字节。
- 新 `custom/src/Swarm/FleetTelemetryArchiveWorker.h/.cc/.md`：parentless值队列、固定UUID NewOnly文件、来源/consent冻结、文件/队列/扫描上限与writer独占；不读SQL、不删历史。
- 新 `custom/src/Swarm/FleetTelemetryArchive.h/.cc/.md`：native明确目录/政策/1..32实际目标，来源失效停整组，状态轮询与actualfinished寿命；默认关闭。
- `qgroundcontrol.pro`、`src/Vehicle/CMakeLists.txt` 登记core观察者头；`custom/custom.pri` 登记3个新h/cc，`CustomPlugin.cc`仅注册Archive internaltype，controller只添加内部实例及公开值，不加视觉/执行按钮。
- 本批14个源码/工程路径和17个模块/共同文档列入限定文本检查；未跟踪新文件单独读取。旧诊断SQL/历史Reader及.tlog/ULog保持原行为，不用新flags声称旧日志已脱敏。

每条修改只在本地工作树，未生成/更新补丁包、安装程序、PR或远端提交；源码接线不是编译/运行验证。

300–304 新增[原生入站遥测记录器](../../custom/src/Swarm/FleetTelemetryArchive.md)：独立Vehicle pre-plugin被动入口、实际PRIMARY/Vehicle/observer绑定与退休、27消息固定Codec、parentless有界NDJSON worker及native确认/公开只读值已接线。新增默认关闭，不改旧.tlog，不请求数据流或控制；定位类另明确确认，原len payload与RX序号只观察，签名/CRC/身份不认证。文件NewOnly不覆盖或删除，入队/write/flush/durable分层，源失效停止整组并保留unknown尾部；fullTelemetryCaptured始终false，不把此子集当完整遥测或任务许可。运行/编译/测试/解析helper/设备均未执行，未改PX4/刷机/更新PR/打包；原25目标/六类不缩减。下方295–299及更早为历史批次。

## 295–299 对应路径

`FleetEventArchiveSql.h/.cc/.md`抽统一schema/strict身份与页、`FleetEventArchiveHistoryReader.h/.cc/.md`加独立READONLY封存目录/页、`FleetEventArchiveWorker.h/.cc/.md`改共享调用并保持私有写/封存生命周期、`FleetPersistentEventStore.h/.cc/.md`加actualcatalog/segment/filter游标、全history槽/取消与inert值。`custom/custom.pri`登记新两模块h/cc、`GuidedActionsController.qml`仅historyCatalog值接线，不增加QMLtype/资源/视觉UI。路线图/状态/矩阵、修改/审计和工程交接同步；未迁库、运行SQL/hash/目录、编译/测试、改飞控、PR或打包。

295–299 新增[跨封存段只读浏览](../../custom/src/Swarm/FleetPersistentEventStore.md)：Sql唯一schema/身份/严格页抽取、parentless单请求Reader与Store native目录/分页/取消及public值facade已接。actual目录条目生成本catalogId，页域绑定文件/DBepoch/sealRun/filter，最多128个每段单filter游标，只0或实际next；当前页/Reader共单槽，取消与错误不提前复用，历史不回灌rules/Replay/任务/权限。目录128段/4096条目与JSON页有界，合法1MiB政策的内部预留预算已修；目录全集、全库记录hash、跨段原子快照/飞行序都不宣称已验证。仍缺全集索引/自动归档清理、完整遥测/运营集成及最终StartPermit等原六类/25目标；未编译/测试、运行SQL/目录或hash helper、改飞控/刷机、更新PR或打包。下方290–294及更早说明为历史批次。

## 290–294 维护轮换源码路径

`FleetEventArchiveWorker.h/.cc/.md` 增协作owner锁和native排空封存、末端SQL身份/文件内容与namespace容量复核、失败/可能已移动状态；`FleetPersistentEventStore.h/.cc/.md` 增explicit维护请求、同实际源/库/文件精确匹配、异步新run与暂停窗口诊断、取消不抹掉物理事实。原pri/type/controller登记不增加新项；全局状态、路线图/矩阵、修改/静态/ACK审计与工程交接同步。没有运行文件轮换/锁/SQL、编译/测试、改飞控、更新PR或打包。

290–294 新增[显式维护封存轮换](../../custom/src/Swarm/FleetPersistentEventStore.md)：实际worker协作锁贯穿排空/SQL关闭与同目录旧库保留，native确认后才请求，不自动删旧段；actual clean/sealed/finished、精确DBepoch/文件名和同源QQ/epoch核对后才沿本次明确配置建立新run。取消/退出/源丢失只退休续建，不抹掉可能已发生的封存事实；暂停序号窗口显式known/unknown及缺口摘要，不称连续无损。封存段数有界、未知文件不覆盖，hash/协作锁不认证或保证恶意并发/失电安全。仍缺跨段查询/自动归档清理、全遥测/运营集成及最终StartPermit等原六类/25完整目标；未编译/测试/运行库或锁/文件轮换、执行SQL、改飞控/刷机、更新PR或打包。下方285–289及更早记录为历史批次。

## 285–289 对应源码路径

- `custom/src/Swarm/FleetEventBlackBox.h/.cc`：native临时epoch/sequence/frame收据、同类型递归身份及同线程修改/删除保护，原快照不产生新收据。
- `custom/src/Swarm/FleetEventArchiveCodec.h/.cc/.md`：18/4/6–21固定诊断投影、严格JSON/typed-null/Unicode资源边界、canonical损坏检查而非认证。
- `custom/src/Swarm/FleetEventArchiveWorker.h/.cc/.md`：parentless QThread SQL、有界队列/批次/历史/主库page、精确库身份、事务后durable/row保留及clean/gap摘要。
- `custom/src/Swarm/FleetPersistentEventStore.h/.cc/.md`：默认disabled、native显式配置、actual BlackBox窗口、低频状态和单槽独立历史，不恢复或下发控制。
- `custom/custom.pri`、`custom/src/CustomPlugin.cc`、`custom/res/Merivus/GuidedActionsController.qml`：三模块h/cc各一次，Store内部类型、私有objectName/实际source绑定和public值facade；Codec/Worker不注册QML类型，无新增资源文件。
- README/修改清单、项目状态、路线图/矩阵、索引、交接/静态与ACK审计、事件回放/操作说明同步。完整遥测和长期轮换等仍待开发；本批未编译/测试、建库/SQL执行、改飞控、提交PR或打包。

285–289 新增[默认关闭的连续诊断归档](../../custom/src/Swarm/FleetPersistentEventStore.md)：实际BlackBox暂时收据/同类型事件身份、固定摘要Codec、专属QtSql线程与Store门面已接pri/type/controller；仅native显式配置/确认后异步开库，默认不读取或写盘，不回录开库前历史。queue+in-flight/批次/主库页/历史结果有界，实际commit后才durable，来源/投影/入队缺口保留诊断；历史单槽直到GUI交付，不进入实时回放、规则、机队、审批或控制链。无原payload/凭据/精确GPS，哈希非签名/匿名化；clean非完整性认证，未知库与需hot-journal恢复库拒绝不重建。仍缺全遥测/长期轮换/运营集成及最终StartPermit等原六类/25完整目标；未编译/运行/测试、建库/SQL执行、改飞控、刷机、更新PR或打包。下方280–284及更早批次为历史记录。

## 280–284 源码入口

- 新增：[StartupMotion.h](../../custom/src/Swarm/FleetMissionStartupMotion.h)、[StartupMotion.cc](../../custom/src/Swarm/FleetMissionStartupMotion.cc)、[观察数学契约](../../custom/src/Swarm/FleetMissionStartupMotion.md)。
- 输入规范化：[ExecutionProfile.h](../../custom/src/Swarm/FleetMissionExecutionProfile.h)、[ExecutionProfile.cc](../../custom/src/Swarm/FleetMissionExecutionProfile.cc)，原schema1六字段与新schema2八字段独立，无默认迁移。
- 真实消费/冻结：[StartupTransaction.h](../../custom/src/Swarm/FleetMissionStartupTransaction.h)、[StartupTransaction.cc](../../custom/src/Swarm/FleetMissionStartupTransaction.cc)、[StartReadiness.cc](../../custom/src/Swarm/FleetMissionStartReadiness.cc)、[ArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)、[StartupTransition.cc](../../custom/src/Swarm/FleetMissionStartupTransition.cc)。
- ODOM period/gap：[Vehicle.h](../../src/Vehicle/Vehicle.h)、[Vehicle.cc](../../src/Vehicle/Vehicle.cc)；纯helper在[custom.pri](../../custom/custom.pri)的h/cc各一次，无新增QML类型或资源。

限定本批12个源码/工程文件和19文档，不是整个工作树总数、目标数或完成率。完整任务仍未收口；无编译/测试/业务接口运行/PX4修改/PR或打包更新。

280–284 新增[显式局部运动与航向观察政策](../../custom/src/Swarm/FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

## 275–279 本批源码路径

- 新增：[FleetStartupOdometry.h](../../src/Vehicle/FleetStartupOdometry.h)、[FleetStartupOdometry.cc](../../src/Vehicle/FleetStartupOdometry.cc)、[原生契约](../../src/Vehicle/FleetStartupOdometry.md)。
- 原生账本与整接收栈：[Vehicle.h](../../src/Vehicle/Vehicle.h)、[Vehicle.cc](../../src/Vehicle/Vehicle.cc)；直接retained静默栈校验：[PlanManager.cc](../../src/MissionManager/PlanManager.cc)。
- 稳定参考/风险与阶段复核：[StartReadiness.cc](../../custom/src/Swarm/FleetMissionStartReadiness.cc)、[StartupTransition.cc](../../custom/src/Swarm/FleetMissionStartupTransition.cc)；READ物理基线：[ArmedGroundReview.cc](../../custom/src/Swarm/FleetMissionArmedGroundReview.cc)。
- 工程登记：[qgroundcontrol.pro](../../qgroundcontrol.pro)、[Vehicle CMakeLists](../../src/Vehicle/CMakeLists.txt)，helper h/cc各一次；无新增QML资源/类型。

本批10个源码/工程文件与17个配套文档路径；该数字是本次限定修改范围，不是整个工作树文件数、产品目标或完成率。以工作树完整diff和新增文件为交接依据；没有更新PR、源码压缩包、安装包或可执行发行物。

275–279 新增[被动 ODOMETRY 原生账本与契约](../../src/Vehicle/FleetStartupOdometry.md)：原始 MAV2 解码50/reference42，Vehicle full27/reference21；实际 PRIMARY/PX4/版本与飞行来源、接收整栈、prepare前计数和token/epoch防嵌套覆盖。sticky everObservedRawFrame 不把曾断流/无效/被过滤的源当初始缺流；Readiness仅存去age的20字段参考投影，普通收包/时间/运动数字/quality分值不造成身份抖动，reset/frame/状态与已观察ABA撤销旧依据；地面READ冻结该投影，Transition前后重核实际来源。稀疏协方差NaN保留未知，扩展尾零/LOCAL_FRD/时钟不认证参考有效性或同采样。不增加传感器、不请求/启用流、不改原六通道/read34/startup32、不放宽HOME/local严格政策。源码接线与有限静态审阅，不是完整运动/转向/净空/最终StartPermit或执行完成；原25目标/六类继续，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方270–274及更早批次是历史记录。

270–274 新增[有界实际启动事务序列](../../custom/src/Swarm/FleetMissionStartupSeries.md)及多轮地面再审后端：最新已初始化owner不回退，逐轮冻结actual QQ/UUID/epoch/generation/hash，旧转交链精确退休后重新完整读取/已阅/两账号审批；每次新消费者保留前代，初始/终态对象不能重用覆盖。native固定历史摘要6防递归复制，完整当轮任务/新风险/真实queued事实仍保留；同一原组/记录/部署，最多8个ground child（空候选也计费），源丢失/容量满/MODE已可能排队/未排空均不淘汰或恢复授权。实际pri/type/controller已接，限定源码复核完成，未编译/运行/测试/API/飞控改动/刷机/PR或打包。容量/新部署重置、HOME有效分支/local frame/运动转向、净空/最终StartPermit/时序/执行完成及原25目标/六类仍待开发。下方265–269及更早批次是历史记录。

265–269 新增[地面再审后的分阶段启动事务](../../custom/src/Swarm/FleetMissionStartupTransaction.md)源码：actual8/context27、一次28→独立30（Session6/Authority10）、同UUID连续占用；独立第二实例不覆盖旧终态。已解锁成员不发ARM，但仍核实际Seq0完整journal与整组屏障；未解锁成员仅普通ARM(param2=0)，之后仅mode-only。首次Seq0前要求实际周期RAW基线/政策gap，错误实例在换generation前拒绝；后端接线且限定源码契约复核完成，无新执行按钮。本机未调用接口/编译/运行/测试、改飞控、刷机或更新PR/包。最终StartPermit/净空/完成、第二事务再次终止后的实际owner重审路线、HOME有效分支/local frame/运动转向及原25目标/六类仍待开发；旧第一终态不能代替第二事务事实。下方260–264及更早批次为历史记录。

260–264 新增[地面再审启动依据一次接管](../../custom/src/Swarm/FleetMissionGroundStartupAuthority.md)源码：实际10来源/原始context27、完整新RAW/全风险/记录/typed当前地面状态进入新owner，旧22一次移交Session独立28（6字段返回真实remaining）、Ledger10字段接管。旧账本退休不盲撤新owner，原最早READ START/两身份期限只缩短，独立实时查身份/WTS/源与整组占用；public不泄露SID/RAW，所有执行/启动/飞行许可false。实际后端/pri/type已接，本批限定源码契约静态收口已完成，无读取/ARM/MODE或UI执行按钮。真正分阶段模式消费者、HOME分支/local frame/运动转向、净空/最终StartPermit/完成及其它六类/原25项目标继续；未编译/运行/测试、调用接口、改飞控、更新PR/源码包/安装包。下方255–259及更早批次为历史记录。

255–259 新增[地面再审独立账号条件意图](../../custom/src/Swarm/FleetMissionGroundStartApproval.md)源码：实际新 Review20/九对象/冻结整组与 armed 状态、完整最新记录/风险/新 RAW，真实两账号新提示和 Session 独立22证明，一次消费/撤销及 native14 grant；public27 receipt 无 SID/RAW。最早 READ START 原30秒不续，旧证明不恢复，实际提示期间主身份仍查 SID/role/WTS/旧期限；active READ与消费不放宽。实际 backend/pri/内部类型已接，本批已完成限定源码契约静态收口，没有新增执行按钮/新 startup authority/MODE续作。最终许可/净空/HOME有效分支/local运动转向/完成及其它六类/原25目标继续；未编译/运行/测试或调用接口、改飞控、更新PR/源码包/安装包。下方250–254及更早批次为历史记录。

250–254 最新源码增量：[停止事务后的新整组地面风险审阅](../../custom/src/Swarm/FleetMissionArmedGroundReview.md) 已接原生专用 READ/完整 RAW、新风险捕获与显式已阅、真实八对象后端及工程注册。原整组逐机 armed 状态冻结，MODE 已可能排队不允许重审；旧批准不恢复，最早实际 READ START 的新 30 秒期限不从完成/已阅续期。初始终态要求全栈排空，持续只允许本实际 READ 接收栈；原未解锁读不放宽。新独立账号授权/后续模式事务、HOME 分支/本地参考/运动/转向、净空/最终 StartPermit/执行完成及其它六类仍待开发。仅源码与有限文本静态复核，未编译/运行/测试、调用接口、修改飞控、提交或更新 PR/源码包/安装包；下面 244–249 及更早批次为历史记录，编号不是目标完成数量。

244–249新增StartupTransaction和StartupTransition各h/cc/md；修改Vehicle、OperatorSession、StartupAuthority、ExecutionProfile、VehicleEvidence、StartReadiness、CustomPlugin、custom.pri、controller后端和相关模块/清单/状态/路线图/矩阵/审计/工程师文档。固定三步、持续风险历史、有限原期限及partial queue取消已写入，本批有限源码静态收口已完成。HOME再审/local参考/运动/转向、最终StartPermit/净空/完成仍待开发；未编译/运行/测试、改飞控、提交或更新PR/源码包/安装包。

本清单对应 2026-10-14 当前工作树的源码改动。它不是 Git commit，也不是编译产物；工程师接手时应先在仓库中执行 `git status --short`，确认这些文件与交接包一致，再自行建立分支或提交。

静态检查记录见 [`MERIVUS_SWARM_STATIC_AUDIT.md`](MERIVUS_SWARM_STATIC_AUDIT.md)；底层 ACK/任务消息操作身份专项审计见 [`MERIVUS_ACK_IDENTITY_AUDIT.md`](MERIVUS_ACK_IDENTITY_AUDIT.md)。

## 本批次代表性源码与文档锚点

239–243 新增 `custom/src/Swarm/FleetMissionStartupAuthority.h/.cc/.md`；修改 `FleetOperatorSession.h/.cc/.md`、`FleetMissionStartApproval.h/.cc/.md`、`src/MissionManager/PlanManager.h/.cc`、`FleetMissionStartReadiness.h/.cc/.md`、`FleetMissionStartReview.h/.cc/.md`、`GuidedActionsController.qml`、`custom.pri`、`CustomPlugin.cc`与模块/修改清单/状态/路线图/矩阵/索引/身份与静态审计/工程交接。原生真实一次移交/ownerID/原TTL与物理阶段候选、passive retained RAW证明、whole-group有限本地占用和controller接线；未新增发命令或界面，仍非最终StartPermit，真正受控状态过渡与原六类完整目标继续。未编译/运行/认证/API/测试、改飞控、提交或更新PR/源码包/安装包。

234–238 新增 `custom/src/Swarm/FleetMissionStartContentCheck.h/.cc/.md`；修改 `src/MissionManager/PlanManager.h/.cc`、`src/Vehicle/Vehicle.h/.cc`、`FleetMissionDeployment.h/.cc/.md`、`FleetMissionStartReview.h/.cc/.md`、`FleetRolePolicy.cc`、`GuidedActionsController.qml`、`FleetMissionDeploymentPanel.qml`、`FleetMissionOperatorWorkflow.md`、`CustomPlugin.cc`、`custom.pri` 和修改清单/状态/路线图/矩阵/索引/静态及ACK身份审计/工程交接。实际scope/fence/RAW INT全读与完整载荷核对、最早READ START时效、18字段context/九参Review/默认撤销确认接齐；取消后不落legacy发送，失败不覆盖旧cache，新读取不当原子/认证/StartPermit或完成。未编译/运行/测试、改飞控、提交或更新PR/源码包/安装包；完整六类剩余范围保留。

229–233 新增 `custom/src/Swarm/FleetMissionStartApproval.h/.cc/.md`；修改 `FleetOperatorSession.h/.cc/.md`、`FleetMissionStartReview.h/.cc/.md`、`FleetMissionStartReadiness.cc/.md`、`FleetMissionProgressObservation.cc/.md`、`src/Vehicle/Vehicle.h/.cc`、`FleetRolePolicy.cc`、`GuidedActionsController.qml`、`CustomPlugin.cc`、`custom.pri`与清单/状态/路线图/矩阵/索引/模块/审计/工程交接。真实账号条件意图审批、一次原生认证依据、原30秒native已阅来源、风险语义v2与原RAW分离及实际健康风险撤销已写入；不产生StartPermit或调用命令，不加启动按钮，不恢复旧审阅或自动延期。未运行/编译/认证/测试、改飞控、提交或更新PR/安装包。

225–228 新增 `custom/src/Swarm/FleetMissionEvidenceRefresh.h/.cc/.md`；修改 `src/Vehicle/Vehicle.h/.cc`、`src/FactSystem/ParameterManager.h/.cc`、`FleetRolePolicy.cc`、`GuidedActionsController.qml`、`FleetMissionDeploymentPanel.qml`、`custom.pri`、`CustomPlugin.cc` 及模块/清单/状态/路线图/矩阵/索引/审计/工程交接。原生成功 RAW 计数、实际 Link 来源、单次版本命令与固定十二只读参数、有界刷新/身份/占用、显式撤销确认接齐；完整16字节名称允许、缺地面状态仅有限等待，不修改参数/cache/wait maps或自动批准/飞行。只改地面站源码和静态复核，不更新PR/安装包，不运行接口、编译或测试。

221–224 更新 `FleetOperatorSession.h/.cc/.md`、`FleetOperationApproval.h/.cc/.md`、`FleetMissionDeployment.cc/.md`、`FleetMissionDeploymentPanel.qml`、`FleetMissionOperatorWorkflow.md`、`custom/custom.pri` 和本清单/修改清单/状态/路线图/矩阵/审计/工程交接。有限请求与明确消费后本次部署租约分开，Windows WTS桌面来源撤权及Wtsapi32链接接线；receipt22/grant13为schema2，旧20/8不能当新租约。没有编译、运行、登录、WTS/API调用、测试、实际上传/飞行、提交或更新PR/安装包。下方217–220为历史边界。

217–220 新增 `custom/src/Swarm/FleetOperatorSession.h/.cc/.md` 和 `FleetOperationApproval.h/.cc/.md`；更新 `FleetMissionDeployment.h/.cc/.md`、`GuidedActionsController.qml`、`FleetMissionDeploymentPanel.qml`、`FleetMissionOperatorWorkflow.md`、`custom/custom.pri`、`CustomPlugin.cc` 与修改清单/路线图/状态/矩阵/索引/审计/工程交接。实际 Windows 本地认证、固定专用组角色来源和独立账号短期审批接入地面上传，默认无配置仍拒绝；不授予启动、企业身份、跨站租约或两人证明。源码写入 Windows 链接库不代表已编译；未运行、登录、测试、真实上传、提交或更新 PR/安装包。

212–216 新增 `custom/src/Swarm/FleetMissionStartReview.h/.cc/.md`、`FleetMissionOperatorWorkflow.md`、`custom/res/Merivus/FleetMissionDeploymentPanel.qml`；修改 `FleetRolePolicy.h/.cc`、`FleetMissionDeployment.h/.cc/.md`、`GuidedActionsController.qml`、`FleetMissionPlannerPanel.qml`、`custom.pri`、`CustomPlugin.cc`、`custom/qgroundcontrol.qrc`、`custom/res/Merivus/qmldir`和清单/状态/路线图/矩阵/索引/审计/工程交接。上传UI与严格原生本地角色快照、全组历史回读来源、新独立30秒风险审阅接齐；未认证账号/缺额外批准不伪造放行，已阅非执行许可。未编译、运行、渲染、测试、提交、修改飞控或更新PR/安装包。

208–211 新增 `custom/src/Swarm/FleetMissionStartReadiness.h/.cc/.md` 与 `FleetMissionProgressObservation.h/.cc/.md`；修改 `src/Vehicle/Vehicle.h/.cc`、`FleetMissionDeployment.h/.cc/.md`、`custom/res/Merivus/GuidedActionsController.qml`、`custom/custom.pri` 及修改清单/状态/路线图/矩阵/索引/静态审计/工程交接。六通道真实raw状态、启动前置证据及任务序号/完整当前缓存对照接齐；boot-time小差回退保守撤销版本/参数/flight可用证据而不认证重启。主动检查只读取冻结实例，1秒显示snapshot不授予权限/恢复批准。独立启动、上传/启动UI和完成归属仍待开发；未修改飞控，未编译、运行、测试、提交或更新PR/安装包。

204–207 新增 `custom/src/Swarm/FleetFirmwareAdapterCatalog.h/.cc/.md`；修改 `src/Vehicle/Vehicle.h/.cc`、`src/FactSystem/ParameterManager.cc`、`FleetMissionVehicleEvidence.h/.cc/.md`、`FleetMissionReviewCoordinator.h/.cc/.md`、`FleetMissionDeployment.cc/.md`、`custom/res/Merivus/GuidedActionsController.qml`、`custom/custom.pri` 及状态/清单/路线图/审计/工程交接文档。原始版本回报与缓存分离，声明匹配不产生认证/启动许可；版本首次/变化撤销参数源，原生 evidence/目录身份绑定贯通。未修改飞控，仅仅读本地版本发包布局；没有编译、测试、运行、提交或更新 PR/安装包。

199–203 新增 `custom/src/Swarm/FleetMissionVehicleEvidence.h/.cc/.md`；更新 `src/FactSystem/ParameterManager.h/.cc`、`src/FactSystem/Fact.cc`、`src/Vehicle/Vehicle.h/.cc`、`src/Vehicle/InitialConnectStateMachine.cc`、`FleetMissionReviewCoordinator.h/.cc/.md`、`FleetMissionDeployment.h/.cc/.md`、`custom/res/Merivus/GuidedActionsController.qml` 和 `custom/custom.pri`，同步状态/路线图/清单/审计/交接文档。该批接齐十二项真实参数来源观测、原生审阅和版本身份、部署最终复核，并收紧既有本地 PARAM_SET 发送边缘和返回结果；新 evidence 路径不请求/写入参数，不新增飞行启动许可。几何暂存仍不可执行。未修改飞控、编译/运行/测试、提交或更新 PR/安装包。

195–198 新增 `custom/src/Swarm/FleetMissionExecutionProfile.h/.cc/.md`；更新 `FleetMissionCompiler.h/.cc/.md`、`FleetMissionPayloadBuilder.h/.cc/.md`、`FleetMissionDeployment.cc/.md`、`FleetMissionReviewCoordinator.cc/.md`、`FleetMissionPlanStore.cc/.md` 及 `custom/custom.pri` 和顶层状态/清单/审计/交接文档。该批将明确支持的速度/起飞/驻留/结束动作请求写入有界载荷并核对全部 wire 身份，仍无 Fleet 启动许可、上传 UI 或执行成功声明；不修改飞控，未编译/运行/测试或更新 PR/源码包/安装包。

193–194 继续更新 `src/MissionManager/PlanManager.h/.cc`、`MissionManager.cc`、`MissionController.h/.cc` 与 `custom/res/Merivus/GuidedActionsController.qml`：建立真实上传对象 parent、native 回调生命周期防护，以及绑定原请求/原实例/操作/ACK 内容的有界延迟恢复通知；启动锁定恢复生成后的新任务快照，抑制同请求上传期间的自动普通启动弹窗。没有新增 Fleet 启动许可，也没有构建/运行/测试、提交或更新 PR/安装包。

2026-10-14（187–192）新增 `FleetMissionPayloadBuilder.h/.cc/.md`、`FleetMissionDeployment.h/.cc/.md`；更新 `FleetAreaCoveragePlanner.cc/.md`、`SwarmController.h/.cc`、`GuidedActionsController.qml`、`CustomPlugin.cc`、`custom.pri`、`src/MissionManager/PlanManager.h/.cc`、`MissionManager.cc` 及本地状态/路线图/审计/交接文档。这是地面几何暂存/回读、占用与底层重入防护的源码增量，不是完整任务执行、飞行授权或安装产物；没有构建/运行/测试，也未更新 PR/源码包/安装包。

183–186 新增 `custom/src/Swarm/FleetMissionDraft.h/.cc/.md` 和 `custom/res/Merivus/FleetMissionPlannerPanel.qml`；修改 `FlyViewMap.qml`、`CommandCenterOverlay.qml`、`GuidedActionsController.qml`、`CustomPlugin.cc`、`custom.pri`、`qgroundcontrol.qrc`、`qmldir` 及状态/路线图/交接文档。界面只编辑、生成和审阅本地提案，未更新 PR、源码包或可执行安装包，也未编译/运行/测试。下文 179–182/174–178 为历史批次边界，新增地图入口不意味着执行闭环完成。

179–182 新增 `custom/src/Swarm/FleetMissionPlanStore.h/.cc/.md`，更新覆盖规划器、编译器、审阅协调器、`FleetIntentTask.h/.cc`、`GuidedActionsController.qml`、`CustomPlugin.cc` 和 `custom.pri`。端点/版本/批准回执只在地面站提案与历史层接线，未更新可执行安装包或 PR，未构建/运行/测试。

2026-10-13 区域规划增量（修改清单 174–178）：新增 `custom/src/Swarm/FleetAreaCoveragePlanner.h/.cc/.md`，更新 `custom/custom.pri`、`FleetMissionCompiler.h/.cc/.md`、`FleetMissionReviewCoordinator.cc/.md`、`FleetAirspaceConflictChecker.cc/.md` 和 `GuidedActionsController.qml`。这是区域几何提案/逐机路线与时序的源码接线，不含二进制、地图绘图或上传执行；辅助/返航路径必须保留一致的显式基准，估算时序不能升级为 4D 放行证据。覆盖预览日志只含有界摘要和计划 ID，显式不含完整任务快照；任务安全间距和空域基准/资源门禁同步收口。

下面列出便于定位的代表性源码与文档锚点；其中既包含新增文件，也包含对已有文件的修改，不能把本节路径理解为完整文件清单或“未跟踪新增文件”列表。当前改动还覆盖通用 QGC 的 Vehicle、MissionManager、FactSystem、Joystick、QmlControls、FlightDisplay、AutoPilotPlugins、AnalyzeView 和相关 QML 资源。工程师接手时必须以 `git status --short` 和实际 diff 为准，确认所有需要提交的文件。

```text
src/Vehicle/Vehicle.cc
src/Vehicle/Vehicle.h
src/MissionManager/PlanManager.cc
src/MissionManager/MissionManager.cc
src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc
custom/src/Swarm/CommandTransaction.cc
custom/src/Swarm/CommandTransaction.h
custom/src/Swarm/FaultToleranceManager.cc
custom/src/Swarm/FaultToleranceManager.h
custom/src/Swarm/FleetRegistry.cc
custom/src/Swarm/FleetRegistry.h
custom/src/Swarm/FormationPlanner.cc
custom/src/Swarm/FormationPlanner.h
custom/src/Swarm/FleetCapabilityMatcher.cc
custom/src/Swarm/FleetCapabilityMatcher.h
custom/src/Swarm/FleetIntentTask.cc
custom/src/Swarm/FleetIntentTask.h
custom/src/Swarm/FleetRiskRadar.cc
custom/src/Swarm/FleetRiskRadar.h
custom/src/Swarm/FleetEventBlackBox.cc
custom/src/Swarm/FleetEventBlackBox.h
custom/src/Swarm/FleetTimelineReplay.cc
custom/src/Swarm/FleetTimelineReplay.h
custom/src/Swarm/FleetMissionSimulator.cc
custom/src/Swarm/FleetMissionSimulator.h
custom/src/Swarm/FleetMissionDigitalTwin.cc
custom/src/Swarm/FleetMissionDigitalTwin.h
custom/src/Swarm/FleetMissionDigitalTwin.md
custom/src/Swarm/FleetExtensionRegistry.cc
custom/src/Swarm/FleetExtensionRegistry.h
custom/src/Swarm/FleetAutonomyPolicy.cc
custom/src/Swarm/FleetAutonomyPolicy.h
custom/src/Swarm/FleetRolePolicy.cc
custom/src/Swarm/FleetRolePolicy.h
custom/src/Swarm/FleetTaskTemplateRegistry.cc
custom/src/Swarm/FleetTaskTemplateRegistry.h
custom/src/Swarm/FleetTaskTemplateRegistry.md
custom/src/Swarm/FleetAutonomyPolicy.md
custom/src/Swarm/MissionHandoffManager.cc
custom/src/Swarm/MissionHandoffManager.h
custom/src/Swarm/SwarmMissionOrchestrator.cc
custom/src/Swarm/SwarmMissionOrchestrator.h
custom/src/Swarm/VehicleCapability.cc
custom/src/Swarm/VehicleCapability.h
custom/src/Swarm/FleetCapabilityDiscovery.cc
custom/src/Swarm/FleetCapabilityDiscovery.h
custom/src/Swarm/FleetCapabilityDiscovery.md
custom/src/Swarm/FleetHealthCenter.cc
custom/src/Swarm/FleetHealthCenter.h
custom/src/Swarm/FleetHealthCenter.md
custom/src/Swarm/FleetEventRuleEngine.cc
custom/src/Swarm/FleetEventRuleEngine.h
custom/src/Swarm/FleetMissionCompiler.cc
custom/src/Swarm/FleetMissionCompiler.h
custom/src/Swarm/FleetMissionCompiler.md
custom/src/Swarm/FleetAirspaceConflictChecker.cc
custom/src/Swarm/FleetAirspaceConflictChecker.h
custom/src/Swarm/FleetAirspaceConflictChecker.md
custom/src/Swarm/FleetRoleAllocator.cc
custom/src/Swarm/FleetRoleAllocator.h
custom/src/Swarm/FleetRoleAllocator.md
custom/src/Swarm/FleetEnergyScheduler.cc
custom/src/Swarm/FleetEnergyScheduler.h
custom/src/Swarm/FleetEnergyScheduler.md
custom/src/Swarm/FleetMaintenancePassport.cc
custom/src/Swarm/FleetMaintenancePassport.h
custom/src/Swarm/FleetMaintenancePassport.md
custom/src/Swarm/FleetLinkDegradationPolicy.cc
custom/src/Swarm/FleetLinkDegradationPolicy.h
custom/src/Swarm/FleetLinkDegradationPolicy.md
custom/src/Swarm/FleetMissionRiskEvaluator.cc
custom/src/Swarm/FleetMissionRiskEvaluator.h
custom/src/Swarm/FleetMissionRiskEvaluator.md
custom/src/Swarm/FleetMissionEvidenceChain.cc
custom/src/Swarm/FleetMissionEvidenceChain.h
custom/src/Swarm/FleetMissionEvidenceChain.md
custom/src/Swarm/FleetMissionReportBuilder.cc
custom/src/Swarm/FleetMissionReportBuilder.h
custom/src/Swarm/FleetMissionReportBuilder.md
custom/src/Swarm/FleetMissionReviewCoordinator.cc
custom/src/Swarm/FleetMissionReviewCoordinator.h
custom/src/Swarm/FleetMissionReviewCoordinator.md
custom/src/Swarm/FleetMissionContinuityPlanner.cc
custom/src/Swarm/FleetMissionContinuityPlanner.h
custom/src/Swarm/FleetMissionContinuityPlanner.md
custom/src/Swarm/FleetOperationalMemory.cc
custom/src/Swarm/FleetOperationalMemory.h
custom/src/Swarm/FleetOperationalMemory.md
docs/MERIVUS_SWARM_ARCHITECTURE.md
docs/MERIVUS_SWARM_TARGET_MATRIX.md
docs/MERIVUS_FLEET_OS_COLLABORATION.md
docs/fleet-event-replay.md
docs/fleet-event-rules.md
custom/src/Swarm/FleetIntentRisk.md
docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md
docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md
```

## 修改文件

```text
custom/custom.pri
custom/src/AutoPilotPlugin/CustomAutoPilotPlugin.cc
custom/src/AutoPilotPlugin/CustomAutoPilotPlugin.h
custom/src/FirmwarePlugin/CustomFirmwarePlugin.cc
custom/src/FirmwarePlugin/CustomFirmwarePlugin.h
custom/src/FirmwarePlugin/CustomFirmwarePluginFactory.cc
custom/src/FirmwarePlugin/CustomFirmwarePluginFactory.h
custom/src/CustomPlugin.cc
custom/src/Swarm/SwarmController.cc
custom/src/Swarm/SwarmController.h
custom/src/Swarm/CommandTransaction.cc
custom/src/Swarm/FleetRegistry.cc
custom/src/Swarm/FleetRegistry.h
custom/src/Swarm/VehicleCapability.cc
custom/src/Swarm/FleetCapabilityDiscovery.cc
custom/src/Swarm/FleetCapabilityDiscovery.h
custom/src/Swarm/FleetCapabilityMatcher.cc
custom/src/Swarm/FleetIntentTask.cc
custom/src/Swarm/FaultToleranceManager.cc
custom/src/Swarm/FaultToleranceManager.h
custom/src/Swarm/SwarmMissionOrchestrator.cc
custom/res/Merivus/GuidedActionsController.qml
custom/res/Merivus/MerivusAIAssistantPanel.qml
custom/res/Merivus/FleetContinuityPanel.qml
custom/res/Merivus/FleetDigitalTwinPanel.qml
custom/res/Merivus/FleetOperationalMemoryPanel.qml
custom/res/Merivus/qmldir
custom/qgroundcontrol.qrc
custom/res/Merivus/FlyViewMap.qml
custom/res/Merivus/CommandCenterOverlay.qml
docs/INDEX.md
docs/architecture/CURRENT_STATE.md
docs/development/SITL_MULTI_VEHICLE_MISSION_TEST.md
docs/development/SITL_SWARM_TASK_ISOLATION_FEATURE_BRIEF.md
docs/development/SITL_SWARM_TASK_ISOLATION_SAFETY_REVIEW.md
docs/handoff/README.md
```

## 变更分层

| 层 | 内容 | 是否直接控制飞行器 |
| --- | --- | --- |
| 数据层 | `VehicleCapability`、`FleetRegistry` | 否 |
| 健康聚合层 | `FleetHealthCenter` | 否；只读汇总并生成可解释门禁建议 |
| 产品规划层 | `FleetIntentTask`、`FleetCapabilityMatcher`、`FleetRiskRadar` | 否；只生成方案、角色和风险解释 |
| 连续性建议层 | `FleetMissionContinuityPlanner` | 否；只读任务/机队快照，生成 Leader 接替、重分配、拆分、恢复和中途加入建议 |
| 事务层 | `CommandTransaction`、`SwarmController` ACK/遥测关联 | 仅经现有 QGC 人工入口发起；不自动决策 |
| 任务层 | `SwarmMissionOrchestrator`、`FaultToleranceManager` | 否；故障模块只产生建议 |
| 审计/预演层 | `FleetEventBlackBox`、`FleetTimelineReplay`、`FleetMissionSimulator` | 否；仅记录、回放和故障模拟 |
| 数字孪生/确定性预演层 | `FleetMissionDigitalTwin`、`FleetDigitalTwinPanel.qml` | 否；只消费版本化 fleet/mission/map/link/energy 快照，生成稳定预演和故障建议，固定 `flightCommandReleased=false` |
| 运营记忆/指标层 | `FleetOperationalMemory`、`FleetOperationalMemoryPanel` | 否；只读记录热点、历史异常和运营指标，估算字段显式标记并支持 JSON 交接 |
| 协同/扩展层 | `FleetRolePolicy`、`MissionHandoffManager`、`FleetTaskTemplateRegistry`、`FleetExtensionRegistry` | 否；权限和目录元数据不释放飞行命令 |
| 分级自主/平台适配层 | `FleetAutonomyPolicy`、`FleetRolePolicy`、`FleetExtensionRegistry`、`AiCommandPolicy` | 否；只读声明阶段、能力、权限、签名/沙箱占位，固定 `flightCommandReleased=false` |
| 编队层 | `FormationPlanner`、编队阶段事务 | 需要工程师确认机载协议后才能用于真机 |
| 界面层 | Guided Actions、地图、指挥中心 | 否；只展示和转发人工操作 |

## 当前静态检查结果

- `git diff --check`：已通过。
- `QStringList` 输入边界已在审阅、任务编译、模板、空域、数字孪生、报告、风险和角色分配辅助函数中显式展开，避免依赖隐式 `QVariant::toList()` 转换；扩展元数据的重复 `flightCommandRelease` 校验已去重。
- `FleetHealthCenter` 门禁目标 ID 现在严格限制为 1--255 的有限整数，并拒绝 identity 别名冲突或隐式截断。
- `FleetCapabilityDiscovery` 和能力护照审批导入不再用 `toInt()` 推断身份；发现状态与审批 ID 均要求有限整数语义。
- `custom/custom.pri` 中新增源文件和头文件路径：已逐项确认存在。
- 交接前静态审查发现的编队 ACK 监听缺口已修复：`_ensureFormationCommandConnection()` 现在同时接入通用事务处理器，编队事务可接收同一条 `mavCommandResult`。
- `FaultToleranceManager.cc` 已显式包含 `<QStringList>`，避免依赖传递包含。
- Leader 故障转移角色已对齐：UAV-1 的注册角色使用 `leader`，掉线时可生成 `reassign` 建议，不会被误判为普通成员而只执行 `hold`。
- `SwarmController::_refreshFleetRegistry()` 已把可读的电池平均值、健康检查摘要和 RADIO_STATUS RSSI 估算值持续写入 `FleetRegistry`；估算字段保留原始 dBm 与来源标记，未知值不转为健康。`HealthAndArmingCheckReport` 另以 update sequence 记录本地观察年龄，避免每秒 registry tick 把停滞报告刷新成 age=0；由于没有源 MAVLink 时间戳，这仍不是飞控样本新鲜度证明。
- P0 健康状态已收紧：在线但没有任何有效电量、链路、估计器或位置健康证据的车辆现在为 `unknown`，不再计入 `healthyVehicleCount`；汇总新增 `unknownVehicleCount`，未知状态的健康分数为 0。
- 无效全球坐标不再被写成已知的 `positionHealthy=false`，避免室内或无 GNSS 车辆被误报为定位故障；收到有效全球坐标或有限的 `localPosition` x/y/z 遥测时才发布正向位置健康证据，并写入 `positionEvidenceSource`。当前 QGC FactGroup 没有独立的本地位置时间戳，因此 `telemetryAvailable` 不等价于新鲜度证明；这只是位置证据，不代表全球任务坐标可用，最终仍由 PX4 任务/解锁检查决定。
- 位置和估计器数据源停止报告时会显式清除 `*HealthKnown`，避免旧健康值无限期沿用。
- `FleetRegistry` 新增版本化能力护照 JSON 导出/导入及 `saveCapabilityPassportToFile()`/`loadCapabilityPassportFromFile()` 本地文件接口；保存使用 `QSaveFile` 原子提交，空路径、读写失败和超过 32 MiB 的输入拒绝；护照同时交接本地审批元数据并校验来源、指纹、时间和车辆关联；导入前校验结构和系统 ID，在临时注册表全量成功后原子替换，导入后重新计算运行健康状态，不直接信任旧快照中的健康字段；配置/基线指纹与对应证据图不一致时拒绝导入。
- `FleetRegistry::importCapabilityPassport()` 的增量导入现在合并审批记录而不是覆盖未出现在文件中的本地审批；显式 `local_approval` 基线必须有同车辆、同指纹的本地审批记录，切换/清除基线时会清掉旧审批并拒绝不一致审批，避免只靠描述字段伪造构型批准。
- `FleetCapabilityDiscovery` 现在对 heartbeat/AUTOPILOT_VERSION/组件元数据的所有身份别名做跨来源一致性检查；冲突时返回无身份的 `discoveryState=invalid`，不静默选用第一来源。静态能力字段优先协议/组件元数据，动态心跳字段保持 heartbeat 优先。
- `FleetRegistry::_mergeDescriptor()` 将 `metadata.configuration` 视为顶层配置的同一证据来源；hash-only 指纹发生变化且没有完整配置图时会清理旧配置图，导出的护照不再携带互相矛盾的 map/fingerprint。
- 能力护照导入现在额外校验配置指纹格式/与配置内容一致性，以及 baseline 来源和哈希完整性；不接受结构合法但指纹自相矛盾的快照。
- 新增 `FleetCapabilityDiscovery` 归一化入口：把已解码的 HEARTBEAT、AUTOPILOT_VERSION、组件元数据、参数快照和本地可信清单整理成统一 descriptor，并校验配置/metadata 类型、配置与基线指纹、baseline 来源和证据完整性；它不解析 MAVLink 原始帧，也不发送飞行命令。
- `SwarmController` 新增共享 `FleetCapabilityDiscovery` 入口并在 Registry 刷新前归一化基础心跳 descriptor；`GuidedActionsController` 暴露只读 `capabilityDiscovery`，但真实 AUTOPILOT_VERSION/组件元数据/参数适配仍需工程师接入。
- `FleetRegistry` 普通注册、部分更新和心跳入口增加严格门禁：identity 别名必须是同一 1--255 整数，布尔字段必须为布尔值，电量/链路/性能数值必须有限且在允许范围内，配置必须是 map 且指纹必须为匹配的 SHA-256；非法输入原子拒绝、不改旧快照。健康刷新同时限制未来 `lastSeen` 的时钟偏差，防止伪造未来时间绕过离线超时。
- 新增 `FleetEventRuleEngine`：规则可注册、启停、删除并按事件类型和简单条件匹配；`evaluate()` 仅返回带 `recommendationOnly` 标记的建议动作，不调用 Vehicle、MAVLink 或任务执行接口。规则重复 ID 拒绝覆盖，条件按 AND 关系处理。
- `FleetEventRuleEngine` 增加严格校验的 `cooldownMs`、`previousEventType` 跨事件门槛及可查询/重置运行时状态；冷却和顺序状态只影响建议去重，不改变任务或释放飞行命令。
- `FleetEventRuleEngine` 增加版本化 `exportJson()`/`importJson()`、`saveToFile()`/`loadFromFile()`；快照只包含规则定义和总开关，导入严格拒绝未知字段/重复 ID/错误版本/`flightCommandReleased=true`，并始终清空运行时冷却与前序事件状态。
- 新增 `FleetHealthCenter`：合并 `FleetRegistry` 与 `FaultToleranceManager` 的逐机健康报告，提供健康摘要、逐机原因以及保守的起飞/任务门禁建议；只读、不发送 MAVLink、不修改参数。未知健康、离线、严重故障、临界电量和极差链路默认阻断，降级状态返回人工复核。
- `GuidedActionsController.qml` 现在统一挂接 `FleetHealthCenter`，指挥中心读取该聚合层作为健康摘要源；`FleetHealthCenter` 只聚合 0–100 的有效评分并显式返回 `scoreKnown`，无证据的界面显示待评分；该接线只改变展示/审阅数据，不绕过原有人工确认或飞行命令门禁。
- `GuidedActionsController.prepareFleetIntent()` 缺省将 `FleetHealthCenter.vehicleHealth` 作为审阅健康快照，避免意图预览因接线遗漏恒定丢失健康证据；显式空数组不被改写，仍会进入 `needsReview`。
- `prepareFleetIntent()` 额外刷新共享 `FleetCapabilityMatcher` 的 `lastPlan`，使 QML/上层读取的能力计划与 `FleetIntentTask` 预览输入一致；该入口只做纯计算。
- `GuidedActionsController` 新增共享 `FleetMissionEvidenceChain`、`FleetMaintenancePassport`、`FleetLinkDegradationPolicy` 实例；普通 QML 仅收到证据/维护/链路值快照，append/import/load/clear 等写入口保留在控制器/受信任后端；维护/链路服务不自动写入任务、不改链路、不发送命令，证据链仅由显式意图生命周期和已接线的白名单事件摘要追加。
- `GuidedActionsController` 以约 5 秒频率用 Registry 的在线/链路质量快照运行 `FleetLinkDegradationPolicy`，`CommandCenterOverlay` 只显示正常/降级/证据不足数量；没有带宽限流、视频调度、编队调速或命令副作用。
- `FleetMaintenancePassport.recordsChanged` 只驱动 `assessAll()` 的只读刷新，指挥中心显示已评估/需复核数量；空护照明确显示证据不足，不自动写入维护记录或改变健康门禁。
- `FleetMissionReviewCoordinator` 新增可选 `options.events` → `FleetMissionReportBuilder::build(..., events)` 接线；意图预览默认带入黑匣子事件快照，报告可生成 timeline/incidents，但该值对象接线不持久化、不改变任务、不释放命令。
- `GuidedActionsController` 暴露最近一次只读 `missionReport`，`CommandCenterOverlay` 仅显示 verdict/异常/未验证计数；没有报告批准、上传或命令释放路径。
- `GuidedActionsController` 在意图预览、申请审批、批准和拒绝节点追加 `FleetMissionEvidenceChain` 摘要记录；追加失败不改变审批状态、不阻止安全路径，也不会释放飞行命令。
- `MerivusAIAssistantPanel` 对通过 QGC 本地策略的飞行/任务 proposal 以及本地规则模式待确认建议提供显式“生成舰队审阅草稿”入口；它只调用 `prepareFleetIntent()` 做只读审阅，不请求/批准意图、不上传任务、不下发命令。`requestFleetIntentApproval()`/`approveFleetIntent()` 还会核对同一轮 `taskId/planId` 和 `needsReview=false`。
- `GuidedActionsController` 监听 `FleetIntentTask` 的意图/参数变更和重新进入 `preview`，清除旧审阅身份；直接调用 `FleetIntentTask.preparePreview()` 不能复用上一轮协调器结果。
- `GuidedActionsController` 还监听 `FleetMissionReviewCoordinator.reviewChanged`；外部重审即使复用同一 `planId` 也会清除审批身份，避免健康、空域或风险证据已经变化而继续沿用旧审阅。
- 不再把原始 `FleetIntentTask` 暴露为 `GuidedActionsController.intentTask`；指挥界面只读 `intentTaskState/intentTaskId`，审批只能经控制器包装入口执行，避免同引擎 QML 直接调用任务对象的 `approve()` 绕过审阅门禁。
- 单机地图 Goto 纳入同一健康门禁：`actionGoto` 在确认时冻结活动机 ID 与坐标，确认和执行前以同一目标调用 `FleetHealthCenter.evaluateTaskGate()`，执行改走 `SwarmController.executeGoto()`，获得事务/ACK 路径和最终预检，不再直接调用 `activeVehicle.guidedModeGotoLocation()`。
- 地图普通右键 Goto 不再直接调用 `SwarmController.executeGoto()` 绕过确认；`FlyViewMap.executeRightClickFly()` 将单机/框选目标与坐标交给 `GuidedActionsController.confirmAction()`，由统一的冻结目标、健康门禁和事务执行路径处理。
- 临时航点队列增加一次性 `_shiftMissionApproved` 确认令牌：只有 `GuidedActionConfirm` 的 map adapter 在确认后才能进入 `executeBatchQueuedFly()`，直接调用该 QML helper 会立即返回；令牌消费或取消后清零，不改变最终 `executeQueuedGoto()` 的健康门禁与事务检查。
- 故障连续性回调改为建议-only：掉线/低电量不再从 QML 信号槽直接修改 `SwarmMissionOrchestrator` 任务；控制器记录事件、保留 `sourceTaskRevision` 并刷新规划器，后续任务变更必须经过人工确认和版本匹配。
- `GuidedActionsController` 对黑匣子中的事务更新、编队异常、舰队降级和任务状态追加原始事件证据，形成任务审计链；该接线仍不修改任务、不发送命令，证据链容量/校验失败只影响审计记录。
- 新增 `FleetHandoffPanel`：身份由 `FleetRolePolicy.currentUserId` 提供且必须具备 `task.handoff` 权限，面板不再使用可编辑 Settings 自报身份；通过 controller facade 创建固定二次确认、300 秒过期的交接请求，并以 `recordRevision` 调用 `acceptHandoff()`/`rejectHandoff()`/`cancelHandoff()`；面板没有任务提交、飞行控制或命令释放路径。
- `FleetHandoffPanel` 无待办时默认紧凑收起；只有点击“发起”展开创建表单，有待办时自动展开，避免低分辨率指挥中心被空面板挤压。
- 新增 `FleetCatalogPanel`：指挥中心只读取模板、扩展和分级自主策略的值快照，显示版本/能力/签名沙箱状态；不暴露 registry 的注册、导入、启用、删除或插件加载方法。
- `FleetRegistry` 的实时注册、遥测写入、配置批准、能力护照导入/保存和清空入口不再标记为 `Q_INVOKABLE`；C++ 发现/后端调用不变，QML 仅可使用只读查询和属性，减少同引擎旁路写入。
- `FleetTaskTemplateRegistry` 与 `FleetExtensionRegistry` 的注册、启用、删除、导入/加载入口不再标记为 `Q_INVOKABLE`；默认目录由 C++ 构造阶段建立，QML 仅使用只读快照和任务校验。
- `CommandCenterOverlay` 新增证据链记录数/头哈希前缀的只读显示；`eventBlackBox`、`eventRuleEngine`、证据链、运营记忆、维护护照、链路策略、自主策略和模板/扩展 registry 均以值快照暴露，不引入签名、权限或命令释放逻辑。
- 新增 `FleetEnergyScheduler`：按任务时长、返航余量、电池/续航证据和替补阈值生成主机与备用机建议，输出每机能量余量、预计任务末端电量、拒绝原因和风险信号；仅做纯计算，始终标记需人工确认且不释放飞行命令。
- 新增 `FleetMaintenancePassport`：追加并导入/导出带版本的维护记录，提供 `saveToFile()`/`loadFromFile()` 本地原子交接（空路径、读写失败、非法 UTF-8、超过 32 MiB 拒绝），强制校验 ISO 8601 时间戳并保存 UTC `timestampMs`，乱序交接记录按来源时间排序后再评估，避免把缺失时间证据伪造为当前记录；按电流、温度、振动、电池衰减、飞行次数和改装记录生成维护状态、证据缺口、阈值原因、建议检查项和预测性风险估算；通用阈值仅供审阅，结果固定标记估算/人工复核，不访问 Vehicle/MAVLink/PX4、不改参数、不发送命令。
- 维护护照温度字段修正为保留 -100 至 300 摄氏度的有效负温度；缺失哨兵与低温读数分离，趋势和证据缺口不再把低温误报为缺失。
- 新增 `FleetMissionRiskEvaluator`：合并已编译任务、逐机健康快照、能量计划和额外风险信号，输出最坏结果、风险项、启发式完成概率、覆盖率证据状态和人工替代方案；概率明确是估算，不是安全保证，模块不修改任务、不发送命令。
- 收紧 `FleetMissionRiskEvaluator` 输入：航点/覆盖计数和 `reserveCount` 仅接受有限非负整数，`reserveVehicleIds` 必须是列表且空列表不能绕过备用短缺判断；显式风险严重级别与权重必须可识别且在约定范围内，非法类型统一生成阻断输入风险，避免 Qt 隐式转换或默认值静默放行。
- `FleetExtensionRegistry` 增加严格元数据快照导入/导出及 `saveToFile()`/`loadFromFile()` 本地原子交接；导入校验扩展 ID、版本、类型、能力、入口和 metadata，拒绝超过 32 MiB/非法 UTF-8/坏 schema/命令释放标志，不加载二进制、不隐式启用扩展。签名、沙箱和真实适配器仍由工程师接入。
- 新增 `FleetMissionEvidenceChain`：以 SHA-256 前后哈希串联任务计划摘要、能力/构型指纹、审批、风险评估、关键事件和最终结果；`importJson()` 在替换内存快照前逐条验证格式、字段、序号、链接和哈希，支持 `saveToFile()`/`loadFromFile()` 本地原子交接，空路径、读写失败和超过 32 MiB 输入拒绝，失败时保持原链不变。模块只有追加、导出、导入、文件交接和验证接口，不访问 Vehicle/MAVLink/PX4，不发送命令。
- 收紧本地审计输入边界：`FleetMissionEvidenceChain` 对追加和导入共用类型长度、payload 递归深度（16）、节点数（50000）及单条 payload 大小（512 KiB）限制，避免深层/超大证据在规范化与哈希时耗尽资源；`FleetOperationalMemory` 和 `FleetMaintenancePassport` 的 JSON 导入拒绝未声明根字段，维护护照同时校验约定的启发式 `riskModel`。这些模块仍是未签名本地证据，不能提供不可抵赖身份或放行证明。
- 新增 `FleetMissionReportBuilder`：把任务计划、风险、健康、能量、证据和事件快照整理成 `commander/customer/engineer/safety` 四种结构化报告视图，归一化事故/异常、时间线和未验证项；非对象、空事件或无有效正时间戳的事件会计入 `events_input_invalid`/`inputs.invalidEventCount`，不会被静默丢弃后生成清晰报告；提供 JSON 导出及 `saveToFile()`/`loadFromFile()` 本地原子交接（空路径、读写失败、非法 UTF-8、超过 32 MiB 拒绝），固定报告只读边界，不访问 Vehicle/MAVLink、不修改任务、不释放飞行命令。
- 新增 `FleetOperationalMemory`：以有限长度观测记录聚合链路盲区、GNSS 不稳定、风场、高耗电航段、异常历史及任务完成率、覆盖率、响应时间、能耗、人工干预、利用率和维护成本；输出位置格网、显式 `estimateFields`/`<metric>IsEstimate` 标记与版本化 JSON 交接，支持 `saveToFile()`/`loadFromFile()` 本地原子文件，空路径、读写失败和超过 32 MiB 输入拒绝，导入先校验后原子替换。模块不访问 Vehicle/MAVLink/PX4、不修改任务、不释放飞行命令。
- 新增 `FleetOperationalMemoryPanel`：在指挥中心只读显示热点、指标和观测/估算状态；`GuidedActionsController` 以较低频率采集既有 FleetRegistry/任务摘要，不建立第二条命令路径。
- `FleetEventBlackBox` 增加带 `format`/`schemaVersion` 的快照导出导入及 `saveSnapshotToFile()`/`loadSnapshotFromFile()` 本地文件交接，`FleetTimelineReplay` 增加版本化时间轴 JSON 交接及 `saveToFile()`/`loadFromFile()`；保存使用 `QSaveFile` 原子提交，空路径、读写失败和超过 32 MiB 的输入拒绝，导入先校验数据再替换内存状态，失败时保留旧状态；有效黑匣子快照同步 `maxEvents`，不恢复录制开关、不发送命令。签名、加密、权限和保留/清理策略仍由工程师/产品接入与验收。
- `FleetEventBlackBox` 的事件导入现在要求每条记录完整包含 `timestampMs`、`type`、`source` 和 `payload`，拒绝缺失时间/来源/载荷的“半条记录”；这样回放和审计不会接收无法排序或无法解释的事件。该变更不改变录制开关恢复边界，也不增加任何飞行命令路径。
- `FleetEventBlackBox` 的录入/导入现在对 `type`/`source` 和单条 `payload` 施加共同边界：递归深度最多 16 层、节点最多 50000 个、标量/键字节最多 512 KiB，超限事件在进入保留队列前拒绝；避免单条损坏交接数据绕过整份 32 MiB 快照限制并拖垮回放/审计线程。
- `GuidedActionsController.qml` 已把 `FleetEventBlackBox.eventRecorded` 接到 `FleetEventRuleEngine`，通过只读 `eventRuleSuggestions`/`eventRuleSuggestionCount` 更新规则建议；默认登记舰队降级、编队异常两个仅供人工复核的规则，并在黑匣子清空时清除旧建议。该接线不调用 `SwarmController`、Vehicle、MAVLink 或任务写入口。
- `GuidedActionsController.qml` 同步黑匣子事件到 `FleetTimelineReplay`，回放事件复用只读规则评估；导入非空日志时重建规则运行态，回放期间不因新事件重置位置。`CommandCenterOverlay` 只显示回放位置/播放状态，没有新增播放控制或飞行命令路径。
- `MissionHandoffManager` 增加带 `format`/`schemaVersion`、全局 `revision` 和记录 `recordRevision` 的交接快照导出导入及 `saveToFile()`/`loadFromFile()` 本地文件交接；保存使用 `QSaveFile` 原子提交，空路径、读写失败、非整数版本、错误字段类型和超过 32 MiB/100000 条记录的输入拒绝；导入先校验记录、状态、时间戳、重复 ID、根/记录未知字段以及完整的全局/记录 revision，再原子替换内存集合，不恢复控制会话、不发送命令；接受/拒绝/取消可携带 `expectedRevision`，版本不一致拒绝写入，且在解析动作内再次检查截止时间，避免旧界面或过期定时器间隙放行交接。
- 新增 `FleetRoleAllocator`：按角色需求、能力标签、健康状态、构型基线、链路、电量和距离生成 `leader/scout/relay/responder/reserve` 分配建议；输出 assignments、rejected、risk 和人工确认标志，仅做纯计算，不修改任务、不连接 Vehicle/MAVLink、不释放飞行命令。
- 新增 `FleetLinkDegradationPolicy`：根据已归一化的链路质量、带宽、延迟、丢包和任务优先级，生成视频/遥测/编队速度/数据优先级、恢复补传建议及风险解释；固定为推荐结果，不修改链路配置、Vehicle 或任务，不发送命令。
- 新增 `FleetMissionCompiler`：把意图任务的区域/航点、时间限制、角色分配和备用机整理成带 `planId`/版本的可审阅计划；固定 `manualConfirmationRequired=true` 与 `flightCommandReleased=false`，不访问 Vehicle/MAVLink、不上传 Mission。
- 新增 `FleetAirspaceConflictChecker`：对多机路径执行保守的 4D 最小间距、禁飞区、返航通道、坐标系和输入完整性检查，输出冲突片段、时间窗、严重度、原因与替代方案；只读、不修改任务、不发送 Vehicle/MAVLink 命令。
- 新增 `FleetMissionReviewCoordinator`：把角色分配、任务编译、能量/备用机、4D 空域检查、任务风险评估和审计报告串成单次只读 `review()`；明确返回 `needsReview` 与各阶段原始结果，缺失健康/能量/路线/空域证据不视为安全通过；GuidedActionsController 的意图预览记录该审阅结果，但不接入 SwarmController。结果固定 `manualConfirmationRequired=true`、`flightCommandReleased=false`、`missionMutated=false`。
- `GuidedActionsController.prepareFleetIntent()` 对显式 `templateId` 调用 `FleetTaskTemplateRegistry::validateTask()`；模板校验结果写入审阅 task/plan，模板缺失或约束失败追加阻断风险并保持 `needsReview=true`。AI 面板只透传有限长度的字符串 `templateId`，不自动推断模板。
- 删除 QML 对 `industrial_inspection` 的重复注册；该模板以 C++ 内置定义为唯一来源，避免运行时覆盖 `positioning/camera` 能力约束和 `captureIntervalSeconds` 默认值。
- 修正 `SwarmController::_refreshFleetRegistry()` 的 RSSI 归一化：Vehicle 的未知/未初始化 `RADIO_STATUS` 值不再被换算为高链路质量，只有负 dBm 证据参与百分比估算，缺失值保持 `-1` 未知。
- 收紧 `FleetCapabilityDiscovery::validateDescriptor()`：在 Registry 前拒绝字符串/小数 ID、冲突别名、非布尔健康字段、越界/非有限数值、未来时间戳和非字符串静态字段，并保持配置/基线指纹校验。
- 收紧 `FleetTaskTemplateRegistry::importJson()`：未知 root/template/signature 字段会被拒绝，完整校验通过后才原子替换模板目录。
- 收紧 `FleetRegistry::importCapabilityPassport()`：未知根字段、未知审批字段和非法导出时间戳会被拒绝，车辆与审批全部校验后才原子替换。
- `FleetTaskTemplateRegistry` 增强为版本化模板目录：`validateTask()` 合并模板/任务能力与数量、时长、字段和结果格式约束，输出可解释拒绝原因；`exportJson()`/`importJson()` 使用 `schemaVersion=1` 和原子替换，并提供 `saveToFile()`/`loadFromFile()` 本地原子交接（空路径、读写失败、非法 UTF-8、超过 32 MiB 拒绝），签名字段仅为外部未验证占位，不执行命令、不连接 Vehicle/MAVLink。
- `FleetRegistry` 增加本地构型审批、审批查询和回滚接口；普通 `registerVehicle/updateVehicle/markSeen` 路径会剥离或忽略 baseline 字段，只有显式护照导入或人工审批能改变地面站批准记录。回滚只撤销审批，不恢复物理机体或飞控参数。
- `GuidedActionsController` 对外的 `faultToleranceManager` 改为 `vehicleReports/recommendations/fleetSummary` 值快照；可写的 `reportVehicle()/reportBattery()/clear()` 仍只由内部遥测接线使用，普通 QML 不能伪造健康状态影响 `FleetHealthCenter` 或命令前置门禁。
- `FleetIntentTask::approve()` 现在要求预演匹配结果 `feasible=true`；不可执行的任务不会被 UI 标记为已批准，且原有“永不释放飞行命令”边界保持不变。
- 新增 `FleetAutonomyPolicy`：只读发布人工/辅助/监督/自动化阶段，以及 PX4、ArduPilot、伴随视觉插件的能力和权限声明；签名、沙箱、版本兼容和启用状态均是未验证/待接入占位，不加载插件、不访问 Vehicle/MAVLink、不释放飞行命令。
- `FleetIntentTask::preparePreview()` 现在将能力匹配结果与 `FleetRiskRadar` 合并，预览同时输出候选、备用机、风险等级、分数和解释。
- `FleetIntentTask` 修改 intent/parameters 后会使旧预演和待审批状态失效，防止审批针对已经改变的任务内容。
- `FleetRiskRadar` 不再接受负风险权重，插件或回放数据不能用负数抵消已有安全信号。
- `FleetRiskRadar::assess()` 现在严格校验任务 `intent/parameters`、布尔门禁、链路/电量百分比、备用机计数及风险信号的 Map/weight/reason；字符串布尔/数值、NaN、越界和非 Map 输入返回 `inputValid=false`、`needsReview=true` 并提升为阻断分数，不再由 `QVariant::toBool()/toInt()` 静默改变风险结论。
- `FleetCapabilityMatcher` 支持可选最低速度、最低续航和 `requirePerformanceEvidence` 门禁；普通任务保留未知性能，安全任务可拒绝缺失性能证据。
- `FleetMissionSimulator::preview()` 对失联、Leader 故障、低电量、链路下降和 GNSS 不可用故障给出受影响飞机、降级建议和人工复核标记；它仍不会执行命令。
- 新增 `FleetMissionDigitalTwin`：以 `schemaVersion=1` 保存五类输入快照和故障覆盖，按稳定 ID 生成可重复时间线、快照/预演哈希、风险摘要及人工故障建议；支持 `saveToFile()`/`loadFromFile()` 本地原子交接（空路径、读写失败、非法 UTF-8、超过 32 MiB 拒绝），`importSnapshotJson()` 完整校验后原子替换，任何输出均固定 `recommendationOnly=true`、`flightCommandReleased=false`，不访问 Vehicle/MAVLink/PX4。
- `FleetAirspaceConflictChecker::check()` 计算任务路径间的 4D 相对运动最小距离，检查禁飞区及返航路径；无时间戳、无坐标、混用坐标系或缺少返航通道时保守地进入人工复核/阻断结果。
- `FleetMissionCompiler` 对 `constraints.geofenceRequired` 增加严格布尔门禁；字符串/数值类型错误会生成 `invalid_geofence_required` 阻断风险并使计划不可行，避免 QVariant 隐式布尔转换改变地理围栏要求。
- `SwarmMissionOrchestrator::reassignTask()` 拒绝空成员集合，避免运行中的任务被重分配成无主任务；需要停掉任务时必须显式暂停/失败/取消。
- `SwarmMissionOrchestrator` 为每个任务摘要输出单调 `revision/taskRevision`；进度、成员或生命周期状态每次有效变更都会递增，`FleetMissionContinuityPlanner` 在建议中保留来源版本，`reassignTaskAtRevision()` 拒绝过期版本，避免人工审阅期间旧方案覆盖新任务。该检查只作用于地面站内存状态，不是服务端鉴权或飞行授权。
- `GuidedActionsController` 不再把可变 `SwarmMissionOrchestrator` 对象暴露给普通 QML，只公开 `missionSummary`/`missionTaskSummaries` 值快照；新增 `applyContinuitySuggestion(suggestionId, true)` 作为人工复核接缝，除一次性人工确认外还要求 `FleetRolePolicy.currentUserId/currentRole` 和 `task.continuity.apply` 权限；入口只能从当前规划器重新取出 `replace_leader`/`reallocate_task` 建议，在写入前用当前 Registry 快照重新规划并核对 `recommendationOnly`、`requiresOperatorReview`、`sourceTaskRevision`、当前任务版本和完整目标集合后调用 `reassignTaskAtRevision()`；规划器的 `proposedAssignments` 保留健康成员再加入替补，避免缩掉原任务成员。所有拒绝/成功均记录 `continuity.apply_blocked`/`continuity.applied` 和操作者身份，该入口不自动接线，也不释放飞行命令。
- `SwarmController::_selectedVehicles()` 增加 QGC toolbox 空指针保护，初始化/关闭/Mock 阶段不会因选择查询直接崩溃。
- `SwarmMissionOrchestrator` 禁止零成员任务进入 `running`，并禁止单个任务进度无显式重规划地回退。
- `CommandTransaction::setTimeoutMs()` 在运行中修改策略时同步重启实际定时器，避免 UI 显示期限与后台截止时间不一致。
- `CommandTransaction::canRetry()` 现在要求至少一次有效 attempt 已启动、命令/目标集合非空且仍存在失败/超时目标；创建阶段校验失败不能通过 `beginRetry()` 伪装成新的 Running 事务。超时回调会重新核对当前 deadline，残留 QTimer 事件不会覆盖修改后的 timeout policy。
- `CommandTransaction::resultSummary()` 现在显式输出与 `Q_PROPERTY terminal` 一致的 `terminal` 布尔值；`SwarmController` 的临时 Mission 延迟回调可据此识别失败/超时终态并停止继续推进，避免残留机载 Mission 被误报为成功。
- `SwarmMissionOrchestrator` 的生命周期信号先复制稳定 `taskId` 再发射，避免 direct slot 在信号处理中移除/重排任务后继续解引用失效的 `QHash` 指针；`setTaskProgress()` 拒绝 NaN/Inf 进度。
- 终端命令事务现在按上限归档并保留摘要，避免长期运行后 `_transactions` 和 QML 列表无限增长；归档结果仍可通过 `transactionSummary()` 查询。
- `FleetCapabilityMatcher` 默认拒绝 `unknown` 或缺失健康状态的飞机，只有任务显式设置 `allowUnverified=true` 时才允许其进入规划候选。
- `FaultToleranceManager` 现在传递并检查估计器/位置健康字段的 `*HealthKnown`，未知健康状态不能自动成为 Leader 候选。
- `FaultToleranceManager` 的故障健康部分报告现在要求 `faultHealthKnown` 与非负 `faultEvidenceAgeMs` 成对提交；仅翻转 known、或提交非负年龄却未声明 known 的输入被拒绝，清除 known 时清除旧年龄。`FleetHealthCenter` 使用 `offlineTimeoutMs` 配置作为同一新鲜度窗口，避免故障证据在两个聚合层出现不同结论。
- 编队 ABORT 阶段现在记录拒绝成员；只在全部 ABORT ACK 成功时报告回滚完成，否则报告 `rollback incomplete` 并保留失败成员列表。
- `SwarmController` 对同一车辆、同一 MAVLink 命令的活动事务增加互斥；由于 `COMMAND_ACK` 不携带地面站事务 ID，重复操作会被拒绝并返回 skipped，而不会把 ACK 错配到另一条事务。
- 事务新增逐机 `dispatchStarted`/`dispatchEpoch` 证据：事务建立和错峰排队不再等同于命令已发送；只有实际交给 Vehicle/MissionManager 后，MAVLink ACK 或 Land/RTL/Mission 模式遥测才可以终结对应逐机事务。Land/RTL 作为安全逃生仍可抢占普通事务，但同一车辆的 safety escape 现在有唯一 owner，重复的 Land/RTL 请求被拒绝，通用 ACK 不会广播给旧 owner。
- 临时任务上传、执行、到达、清理现在纳入 `CommandTransaction`；上传失败、超时、被直接 goto 替代或清理失败都会结束对应车辆事务，成功只在任务完成并清理确认后成立。
- 普通 goto/takeoff/land/RTL 与临时 Mission 的延迟回调在实际发送前重新核对事务整体终态和本车逐机状态；已归档、已失败/超时或被 direct goto 替代的事务不会再发排队命令，车辆对象消失会记录逐机失败。
- `SwarmController::cancelTransaction()` 提供受控地面站取消入口：事务终止后排队普通命令和临时 Mission 回调不再发送，临时 Mission 转入清理路径；该入口不回滚已发出的机载动作，工程师需验证清理 ACK 和残留状态。
- 临时 Mission 替换现在先请求旧任务清理并返回 `replacementStarted=true`，清理 ACK 到达前锁定车辆，不覆盖旧的 `_temporaryMissions` 状态；操作员需在清理完成后重试，避免不带任务代号的 `removeAllComplete` 迟到回调误清理/完成新上传。
- 临时 Mission 的 `sendComplete/removeAllComplete` 仍是 PlanManager 共享信号，代码现在增加本地上传/清理 pending 标记及结果航线（数量、终点）核对：未处于本控制器期待阶段、结果不匹配或无法归属的回调只结束本地事务并保留现场，不调用 `removeAll`；上传失败/无法归属还会写入 `staleTemporaryMissionIds`，阻止下一次任务覆盖现场并要求后续 direct goto/替换显式清理；只有本控制器已登记的清理请求才消费 `removeAllComplete`。由于底层信号仍没有 operation token，工程师必须在 Mock/SITL 中验证外部 MissionController 并发操作和人工恢复路径。
- QML 操作员流现在贯通事务取消和健康门禁：`GuidedActionsController` 在起飞/编队/临时 Mission 的确认与执行边界使用 `FleetHealthCenter`，`CommandCenterOverlay` 显示选中目标门禁、容错建议和活动事务取消入口；临时 Mission 由 `FlyViewMap::executeBatchQueuedFly()` 在真正 `executeQueuedGoto()` 前再次检查，避免 map-indicator 回调绕过确认后的最新健康状态。门禁仅作用于这些高风险批量/临时任务动作，RTL/降落/急停保留为安全逃生路径。
- 收紧 `CommandCenterOverlay.applyControlCommand()`：高度、速度和 `MPC_TKO_SPEED` 不再从面板直接访问 `Vehicle`，统一调用 `GuidedActionsController.requestControlCommand()`。控制器冻结 `{vehicleId, kind, value, requestId}`，在请求和确认执行两侧重新检查连接、Guided/飞行模式、机队健康和参数范围，并记录 `command.requested/confirmed/sent/rejected`。`sent` 只表示已提交给 Vehicle API；参数写入和 Guided 指令均保留 `pending_telemetry`/“未观测 ACK”，不把 QML 方法返回值误报为飞控确认。
- `MPC_TKO_SPEED` 如本地 setter 接受，控制器在最多 5 秒内以 `guidedTakeoffSpeed()` 回读轮询参数；匹配时记录 `command.acknowledged` 和 `parameter_readback`，超时记录 `command.ack_timeout`。这只是地面站回读证据，不代替 MAVLink 参数协议的完整 ACK 连续性验证。
- `GuidedActionsController` 的标准 `actionChangeAlt/actionChangeSpeed` 同样冻结确认前的单机 ID，执行时重查连接、Guided/飞行模式和 FleetHealthCenter；滑块指令记录同一套 `command.confirmed/sent/rejected` 事件，不因切换 active vehicle 而改发送目标。
- 收紧 QML 控制面：`GuidedActionsController` 对外以只读 facade 替代 raw `FleetHealthCenter`、`FleetRegistry`、`FormationPlanner`；地图拿到的 `swarmController` 仅保留临时航点查询/下发与完成信号，原始 SwarmController 不再从公共属性泄露；健康 facade 对非布尔 `requireLeader` 直接阻断。任务完成面板的 Resume 按钮改走 `confirmAction(actionResumeMission)`，恢复目标先冻结并经过健康门禁。
- 标准任务启动旁路收口：`actionStartMission/actionContinueMission/actionMVStartMission` 不再直接调用 `Vehicle.startMission()`，确认时冻结目标 ID 和任务项快照，统一调用 `SwarmController::executeStartMission()`；C++ 为每机建立 `mission_start` 事务，等待 `MAV_CMD_MISSION_START` 接受和 Mission 模式遥测，超时/断链/健康恶化会失败，不把请求接受当作任务完成。
- 标准任务快照证据：`executeStartMission()` 在每机启动前重新读取机载 MissionManager 的任务项数量/当前索引，并按 `FirmwarePlugin::sendHomePositionToVehicle()` 转换到与 QML 相同的任务序号；同时保留 `rawCurrentIndex`，输出由 MissionItem JSON 生成的 SHA-256 `snapshotFingerprint`。冻结的数量或转换后的当前索引不匹配会跳过该机；延迟发送前再次比较数量、序号和指纹，任务在窗口内变化则事务失败且不调用 `startMission()`；Mission 模式遥测完成时，指纹会写入该机事务结果的 `missionEvidence`。该指纹仍未与 QML 侧独立的任务版本服务对接。
- 标准任务延时发送窗口现在还会再次调用完整的 `_vehicleMissionStartReady()`；模式切换、链路丢失、MissionManager 写入进行中或任务项被清空时，事务逐机失败且不会调用 `startMission()`。临时 Mission 上传完成进入 `Starting` 后同样复用该完整就绪检查，失败会暂停并进入清理。
- 恢复任务路径已收口：`GuidedActionsController` 冻结恢复目标并通过健康门禁，`MissionController::resumeMission()` 的异步上传只在 `resumeMissionReady` 到达后将新机载快照交给 `SwarmController::executeStartMission()`；恢复上传失败/目标切换/快照无效不会直接启动任务。
- `SwarmController::_createCommandTransaction()` 对同一车辆的活动普通飞行/任务事务实施逐机互斥，避免 mission-start、temporary-mission、goto、takeoff、formation 等不同 MAVLink 命令并发占用同一 ACK/延时发送通道；RTL、Land 和 formation ABORT 保留安全逃生/回滚例外。
- `SwarmController::_refreshFleetRegistry()` 现在把现有 Vehicle API 的构型与健康证据写入能力护照：固件版本/git hash/UID、机型与电机布局、传感器位图、摄像头/避障/测距能力，以及 estimator、SYS_STATUS、ESC、FTC 的可用性/故障摘要。它们是 observed evidence，不会自动建立本地批准基线；FactGroup 的 `telemetryAvailable` 仍不等于带时间戳的新鲜度。
- 数字孪生五类快照 QML 属性改为只读，新增快照只能走 `captureSnapshot()`/`setSnapshot()` 原子入口；时间线回放的 `events` 属性也改为只读，QML 通过逐行形状检查的 `replaceEvents()` 更新，JSON 导入拒绝未知根字段。
- `FleetMissionDigitalTwin` 与 `FleetMissionSimulator` 的故障注入现在只接受版本化支持的故障枚举（失联、链路、低电量、GNSS、Leader、通信延迟）；未知 kind 不再生成无语义的泛化预演结果。
- 临时 Mission 上传完成后先进入 `Starting` 阶段；启动 ACK 或 Mission 模式遥测到达后才进入 `Executing`，启动拒绝/超时会记录失败并清理机载任务，清理完成才允许事务结束。
- 临时 Mission 的上传与后续 `MAV_CMD_MISSION_START` 具有独立的逐机 dispatch 标记和单调 epoch；只有真正执行启动调用后，该命令 ACK 才能归属当前事务，启动前失去所有权会暂停并清理。
- `GuidedActionsController` 对审阅协调器、风险雷达、能力匹配、时间线回放、任务模拟器和数字孪生新增受控 facade；公开界面只得到值快照及必要的播放/预览入口，原始导入、注入、清空和替换 invokable 保留在控制器内部。
- 手动参数和标准 Guided 辅助函数在进入 Vehicle API 前再次校验动作码与冻结参数类型一致；错误动作码、过期/复制确认对象直接记录拒绝。
- `src/FlightDisplay/GuidedActionConfirm.qml` 在真正显示的确认框/地图指示器被取消时通知 `GuidedActionsController.confirmationCancelled()`；控制器清除手动参数、滑块目标和确认数据快照，避免取消后被旧 QML 对象重放。因该组件为通用 QML 界面，这是协议级的使用约束，不代替 C++ 权限边界。
- QML 显式单架目标修正：当 CommandCenterOverlay 冻结了一个选中 ID 时，GuidedActionsController 的 Takeoff/Land/RTL 不再误回落到 `activeVehicle`，而是统一创建 SwarmController 事务并在确认文案中显示该冻结目标；未提供选择的单机 Guided 操作保持原路径。
- 安全逃生告警可见性：GuidedActionsController 的批量结果对话框显示 `healthGate.warningVehicleIds`，让 Land/RTL 在保留可达路径的同时不隐藏 FleetRegistry 的风险证据。
- `SwarmController` 的起飞、Goto 和临时 Mission 路径现在共享命令级 health preflight：在创建事务、延迟发送和临时 Mission 上传完成后再次检查 `FleetRegistry`。确认离线/严重状态、故障、临界电量或极差链路会逐机阻断并进入事务失败/跳过结果；没有 Registry/车辆快照或 `discoveryState=invalid` 时普通命令 fail-closed；批量降落/返航保留安全逃生路径，FleetRegistry 的 advisory block 只进入 `healthGate.warningVehicleIds`，实时 Vehicle/PX4 链路、模式和飞行前置条件仍然有效；快照内部单个可选字段缺失仍保持 unknown，不被伪造为健康。该层仍不替代 PX4 安全逻辑。
- `SwarmController::_refreshFleetRegistry()` 在电池样本或 RADIO_STATUS 消失时显式写入 `-1` 未知值，清除旧电量/链路证据，避免 health preflight 继续使用过期遥测。
- 车辆离线后若仍存在旧健康证据，刷新入口会执行一次离线快照清理；已清理的记录不重复触发每秒更新，诊断/回放不会把旧遥测当成当前值。
- 编队 ACK 监听按 phase generation 绑定，旧会话的排队 ACK 不会推进新会话；`MAV_RESULT_IN_PROGRESS` 保持 pending，最终 ACK/拒绝由当前阶段事务记录，避免把长耗时 COMMIT 误判为失败。
- 编队命令发送增加实时主链路连接与同组件/同命令 pending 预检；针对 `Vehicle::sendMavCommand()` 可能同步返回的本地失败/成功 ACK，延迟阶段推进或 ABORT 到当前成员发送循环结束，并保留同步 ACK 的成功/失败集合，避免回滚重入后继续发送旧阶段命令或丢失失败证据。
- Capability descriptor resource contract: `FleetCapabilityDiscovery` and `FleetRegistry` share 128-item/128-character capability-list limits plus 8-level, 2048-node and 128 KiB cumulative key/scalar limits for descriptor metadata; non-JSON-safe custom QVariant values are rejected before configuration hashing or registry merge on discovery, register, update and heartbeat paths.
- Capability matcher resource contract: `FleetCapabilityMatcher` bounds task/vehicle capability lists to 128 items/128 characters and stops before iterating a Fleet larger than 2048 candidates, returning `parametersValid=false` with `fleet_resource_bounds` instead of sorting or allocating from an unbounded input.
- Resource-flow follow-up: Discovery returns bounded invalid diagnostics before hashing over-limit input, checks combined configuration budgets, and preserves discovery errors; Registry stages and checks accumulated metadata before committing; invalid Matcher parameters do not enter the candidate loop. Nulls count as nodes and capability lengths use raw text.
- Task record contract: `SwarmMissionOrchestrator` bounds task type/reason/count and recursive metadata; rejected creation/status writes do not alter task revision or history.
- Local task reservation contract: running/paused task members cannot be claimed by another active task at start/resume/reassignment. `assignmentAvailability()` and `busyVehicleIds` explain conflicts. The formation controller reserves before dispatch and continuity revalidation uses all task summaries; this is not a cross-station lease or MAVLink authority.
- Continuity ownership contract: collection planning excludes members reserved by other running/paused/pending tasks, blocks malformed/conflicting collections, and binds suggestion IDs to `sourceTaskOwnershipFingerprint`. Isolated `planMission()` cannot establish availability; the controller rejects its application without ownership evidence and the orchestrator checks the final write.
- Goto preemption follow-up: full preflight/transaction-conflict review precedes replacement cleanup; direct Goto reserves a successor before superseding an old temporary mission, waits for Pause DO_REPOSITION pending to clear with deadline/live gates, and avoids repeated mode requests/Pause while clearing. Synchronous ABORT completion is not overwritten. This remains local sequencing, not protocol-token or distributed-atomic execution.
- Batch target-set contract: normal multi-vehicle Takeoff, Goto/temporary Mission and Mission Start now fail closed before transaction creation when any frozen target is missing or fails health/link/mission-state preflight; Land/RTL remain explicit safety-escape partial paths, and Goto temporary-mission cleanup is deferred until the complete batch transaction exists.
- `FleetCapabilityDiscovery` 的 baseline 只接受 `trusted_manifest` 或已有 `local_approval` 来源；车辆回传的同名字段不会自动成为批准基线，避免改装机自报 baseline 绕过任务门禁。
- `FleetCapabilityMatcher` 的 identity 解析改为逐字段严格校验：数字字符串、分数/非有限 ID 及 `systemId`/`vehicleId`/`id` 冲突均逐机拒绝，不再静默取第一个别名。
- `FleetRoleAllocator` 的 identity/计数解析改为严格门禁：逐字段检查 `systemId`/`vehicleId`/`id` 并要求一致，拒绝数字字符串、空值、分数/非有限 ID；`leaderRequired` 和 `*Count` 不再使用隐式 `toBool()`/`toInt()`。任务参数错误时保持 assignments/reserve 为空，仅输出可解释拒绝与阻断风险。
- `FleetIntentTask::approve()` 现在对预览中的 `feasible` 和嵌入的 `FleetRiskRadar.inputValid` 做严格布尔检查；只有真实布尔 `feasible=true` 且风险输入有效时才允许进入批准状态，避免字符串/数值证据被审批流程静默接受。
- `FleetTaskTemplateRegistry::validateTask()` 对 `allowUnverified`、车辆在线/能力布尔证据、车辆身份别名和任务时长改用严格类型/范围校验；数字字符串、冲突 ID、字符串布尔和非法数值进入可解释拒绝原因，不再由 QVariant 隐式转换改变模板候选。
- `FleetEventRuleEngine` 对条件路径、数量、列表、文本和比较类型增加严格 schema；比较数字必须为有限真实数值，`exists` 期望值必须为真实布尔，字符串数字/布尔不能伪造命中。规则输出仍固定为人工复核建议，不连接 Vehicle/MAVLink。
- `FleetCapabilityDiscovery` 对能力列表和构型 map 增加严格类型/来源处理，输出 observed-unverified 与静态/受信 `verifiedCapabilities`，并校验 `discoveredAtUtc`；`FleetCapabilityMatcher` 增加 `requireVerifiedCapabilities` 任务门槛，安全审阅可拒绝只由 heartbeat 提供的能力。
- `FleetOperationalMemory` 对观测 payload 的布尔证据改为严格类型：只有真实 JSON/QVariant `bool` 才能触发 `online=false`、`gnssHealthy=false`、`energyHotspot`、`anomaly`、`hasFault`、`operatorIntervention` 或 `*IsEstimate=true`；字符串 `"false"`/`"true"` 不会被隐式当作证据，避免损坏交接数据污染运营记忆热点和异常指标。
- `FleetAirspaceConflictChecker` 对全局/任务级 `returnRequired` 使用严格 QVariant `bool`；字符串布尔值不再改变返航通道检查，显式类型错误会产生 blocking 违规。
- Guided 动作确认现在签发一次性 `confirmationToken`：期望 token 保存在控制器/确认组件的 lexical child-object 状态中，`GuidedActionConfirm` 只在确认手势内原样回传；`GuidedActionsController.executeAction()` 在任何 Vehicle/SwarmController 调用前核对 token、动作码和冻结 `actionData` 身份并立即消费；缺失/复制/过期/重复回调会记录 `command.rejected`。RTL/Land/急停只保留 health 门禁的安全逃生例外，不再存在无确认的 executeAction 旁路。
- 意图审阅现在绑定稳定 FleetHealthCenter 指纹：忽略 `lastSeenUtc` 与连续增长的年龄字段，健康风险字段真正变化才失效旧审批；调用方传入的外部健康列表不覆盖当前快照，指纹不一致会让 `FleetMissionReviewCoordinator` 强制保持 `needsReview`。
- `SwarmController::endFormationSession()` 区分操作员停止与故障 ABORT：停止请求不在回滚启动瞬间发出 `formationFault`，只有后续 ABORT 拒绝/超时等具体失败才上报故障；`SwarmMissionOrchestrator` 只接受 1--255 的有限整数成员 ID，创建/重分配中的非法或重复输入整体拒绝，非法或冲突的显式 Leader 元数据也 fail-closed，不自动替换成首个成员；`FleetRegistry` 支持独立 `configurationClear=true` 清理旧配置证据。
- `CustomPlugin` 将可变任务/健康/命令类型注册到 `MerivusInternal 1.0`，GuidedActionsController 显式导入该 URI；公开 `Merivus 1.0` 只保留 facade/值快照消费。该分层不是 C++ 权限或不可信插件隔离。
- 连续性建议 ID 绑定任务 revision 与稳定 Fleet 快照 SHA-256（排除心跳时间戳/连续年龄，纳入健康、链路、电量、能力、构型等会改变候选的字段）；刷新后的不同 Fleet 证据不会被旧 UI 建议 ID 重新解释为新建议。
- 恢复任务启动使用确认时冻结的 `resumeMissionIndex`（lexical child-object 状态），不再把异步上传完成时可能仍指向旧计划的 `currentMissionIndex` 当作启动索引；上传前与启动前均拒绝负数、小数、非有限和超出任务项范围的索引。
- 标准任务启动增加确认时 `missionStartSnapshot()` 和逐机 fingerprint lock；创建事务、延迟发送前都重新计算指纹，确认后 MissionManager 内容改变会阻断 `startMission()`，结果保留 `lockedFingerprints`。
- 连续性规划器限制可变更阶段为 pending/running/paused，拒绝未分配的显式 Leader，支持任务级能力可信/构型基线/中途加入策略；`replace_leader` 通过版本保护的编排器入口原子更新成员与 Leader 字段，fault freshness 只按 fresh/stale 离散桶进入 Fleet 指纹。
- 连续性替补候选使用稳定的证据质量排序（baseline、verified 能力、电量、链路、ID），不再把车辆发现顺序当作 Leader/替补优先级。
- 多机标准 Mission 启动不再把当前活动机的任务项数量/索引当成全队快照；`actionMVStartMission` 将数量/索引门禁留给逐机 `snapshotFingerprint`，单机启动仍同时核对数量、UI 当前索引和指纹。
- `CommandTransaction` 不允许未真正开始尝试的空/无目标失败事务进入重试；活动事务调整 timeout 时同步 timer，过期队列回调重新核对 deadline，并在结果摘要中明确输出 `terminal`。
- `FleetHealthCenter` 聚合层新增严格布尔/数值读取，损坏的在线、估计器、故障、故障年龄、电量、链路或健康分数字段转为 unknown/阻断，不再依赖 `QVariant::toBool()/toDouble()/toInt()` 的隐式转换。
- `FleetHealthCenter` 不再信任来源 `healthState=healthy` 覆盖字段证据；状态由严格字段推导，来源的 critical/degraded/offline 仅可保守升级，原始声明保留为 `reportedHealthState`。
- `FleetMissionCompiler` 的 assignments/members/reserve 身份解析与 Registry/Orchestrator 对齐，只接受真实且别名一致的 1–255 有限整数，拒绝数字字符串、文本别名、分数、布尔、越界值和别名冲突。
- `FleetHealthCenter` 的 takeoff/task gate 对选中目标执行全量身份校验；混合合法/非法列表不再缩小为合法子集，返回 `invalid_vehicle_id`、`invalidSelection=true` 并阻断；`requireLeader` 找不到安全 Leader 时通过 `globalBlocking=true` 阻断，即使没有可加入 `blockingVehicleIds` 的正数 Leader ID。
- `FleetMissionCompiler` 在 `fleet` 非空时把机队快照视为闭合约束：非法 fleet 条目、未知 assignment 和未知 reserve 都进入 blocking unmet/risk；只有明确未提供 fleet 的 planning-only 模式才不做成员存在性比对。
- `FleetMissionCompiler::idList()` 现在报告被拒绝的 reserve 条目；备用机列表的非法 ID 会进入 `invalid_reserve_vehicle_id` blocking 风险，不会被静默删除后满足空的 reserve 约束。
- `FleetMissionCompiler` 的 geofenceRequired 门禁会验证非空三点边界或有效 bounds；字段存在但为 null/空/错误类型/不完整边界时生成 `geofence_missing` blocking unmet。
- `FleetMissionCompiler` 的 geofence bounds 校验与 AirspaceConflictChecker 对齐：地理 `west/east/south/north` 检查经纬度范围和正向顺序，本地 `minX/maxX/minY/maxY` 检查有限性和正向顺序；无坐标系声明的四数字 bounds 列表会被拒绝。
- `FleetMissionCompiler` 对 assignment role 执行严格 String + 支持集合校验（leader/scout/relay/responder），不再把数字/布尔/未知文本角色编译成可行成员。
- `FleetHealthCenter` 在聚合/门禁层对 battery/link 再做 `-1..100` 范围校验，越界输入即使绕过 Registry 也不会成为健康或可放行证据。
- `FleetMissionCompiler` 进一步闭合编译输入：fleet ID 必须唯一，assignment role 别名必须一致且只有一个 leader，显式 leader 元数据必须匹配；geofence 顶点逐项检查有限坐标/局部坐标，不再以顶点数量 alone 作为证据。
- 编译计划新增 `fleetProvided` 与 `planningOnly` 明确标记，避免审阅界面把未提供机队快照的规划结果误读为闭合可执行计划。
- Qt/MSVC/qmake 编译：本机工具链不可用，未执行。
- Mock/SITL、日志回放、硬件测试、刷机：未执行。

- `FleetRegistry`/`FaultToleranceManager` 新增 fault evidence 观测锚点；健康刷新按墙钟推进 `faultEvidenceAgeMs`，没有新的成对故障报告时证据会跨过 `offlineTimeoutMs` 自动失效。
- `FaultToleranceManager` 的删除/清空/Leader 设置路径收口：删除后重建全部建议，未注册 Leader 被拒绝，空报告集合也不会留下幽灵 Leader 或 split/recovery 状态。
- `FleetRegistry`/`FleetCapabilityDiscovery` 拒绝 `configurationClear=true` 与新 configuration/fingerprint 的矛盾组合，并显式处理 null 年龄，避免隐式转换改变 fault evidence 语义。
- Fault evidence 年龄字段必须与 `faultHealthKnown` 成对提交；age-only（含 `-1`）输入进入 `fault_evidence_pair_missing`，不会污染既有 known/anchor 状态。
- Leader 不健康且有候选时，FaultToleranceManager 在更新只读 Leader 属性前发出统一 `reassign` 建议，事件携带旧/新 ID，避免替换路径漏记连续性人工复核事件。
- FleetHealthCenter 逐机快照新增有效 `offlineTimeoutMs`，让连续性规划器、健康门禁和 FTC 的可配置新鲜度窗口保持一致。
- ContinuityPlanner 的候选快照路径同步使用 `offlineTimeoutMs`，消除指纹路径已支持、实际候选过滤仍硬编码 3000 ms 的分叉。
- GuidedActionsController 的健康审阅 fingerprint 纳入 `offlineTimeoutMs`，健康新鲜度策略变化会触发重新审阅。
- FleetHealthCenter 的 fleet summary 同步带出 `offlineTimeoutMs`，覆盖空机队/摘要-only 的策略变更审阅场景。
- Registry refresh 在 fault age 变化时发布 `registryChanged/healthChanged`，并补齐 remove-Leader 无候选的 hold 事件，避免诊断或连续性只看到 ID 变化而没有原因。
- Leader recovery 现在按完整 `_isSafeLeaderCandidate()` 判定，不安全但仍在线的 Leader 在无替补时也进入全局 hold；不会因在线位掩盖低电量、估计器/定位故障或 stale fault evidence。
- Registry/FTC 重连路径按字段清除离线前遗留的缺失健康证据；裸 heartbeat 只恢复在线，不恢复健康/Leader 资格。
- `FleetCapabilityDiscovery::buildDescriptor()` 对规范化结果执行自审计，`SwarmController::_refreshFleetRegistry()` 在加入实时健康字段后再执行一次完整校验；错误保留在 `discoveryErrors` 并把 `discoveryState` 标为 `invalid`，最终仍经过 FleetRegistry 的原子 ingress 校验，不把未经验证的能力护照当作可用构型。
- Guided 单机 Takeoff/Land/RTL 在确认时冻结为单元素目标列表，执行侧不再回落到可变的 `activeVehicle` 直调；确认数组增加数据指纹，原地修改会使一次性 token 失效。普通单机 Guided 旁路也记录冻结目标并在 active vehicle 改变时 fail-closed，但其底层 Vehicle API 尚未全部事务化。
- `executeQueuedGoto()` 对混合有效/无效航点整笔拒绝；Formation command timer 通过 phase/deadline 门禁忽略过期 queued timeout，并避免同步 ACK 推进阶段后的无条件旧 timer。
- 任务/降落/返航的完成遥测带 dispatch 前模式和时间戳，已处于目标模式的旧遥测不会直接完成当前事务；临时 Mission 清理成功回调增加机载 MissionManager 为空的后置条件。底层共享 `sendComplete/removeAllComplete` 仍无操作 token，不能把该项描述成完全隔离。
- 事务快照增加单调 `createdSequence` 并按创建顺序返回，QML 取消入口不再按随机 UUID 字典序猜“最近事务”。
- legacy Guided 的 Arm/ForceArm/Disarm/EmergencyStop、暂停/航点序号/Orbit/ROI/Gripper/SetHome/VTOL 仍是直接 Vehicle API；本轮另补了 C++ transport/输入边界，但仍未形成统一 ACK/timeout 事务、角色权限和审计闭环。
- 初次能力注册链路已闭合：`VehicleCapability::fromVariantMap()` 将发现适配器的 verified/证据状态、构型证据来源、发现状态/错误、时间戳和位置证据源保留到 metadata，`toVariantMap()` 再以顶层兼容字段输出；`FleetRegistry` 对这些字段执行 String/StringList/List<String> 与时间戳校验，避免首次注册或往返导入丢失 discovery invalid 或接受损坏护照。
- Verified-only 匹配边界已收紧：`FleetCapabilityMatcher` 在 `requireVerifiedCapabilities=true` 时不再把 `canUse*` 运行时布尔观测提升为已验证能力，只消费显式 `verifiedCapabilities`；非 verified 的规划模式行为保持不变。
- ContinuityPlanner 已同步该边界：verified-only 替补过滤/排序支持顶层或 metadata verified 标签，不再由 `canUse*` 观测绕过；fleet fingerprint 纳入 verified/discovery 证据字段，避免旧建议跨可信度变化复用。
- Matcher 机队输入现在严格验证 `capabilities/verifiedCapabilities`（含 metadata）为 String/StringList/List<String>；错误类型输出 `invalid_capability_evidence`，不再被 QVariant 自动转成可匹配文本。
- ContinuityPlanner fleet snapshot 入口同样拒绝坏能力列表/metadata verified evidence，阻止不完整或畸形护照进入替补规划。
- Matcher 任务能力要求现在执行元素级 String/StringList/List<String> 校验，坏 required capability 输入会让计划参数无效，不再静默转换。
- RoleAllocator 增加 `requireVerifiedCapabilities` 输出/门禁，角色能力只消费显式 verified 标签；能力列表和角色能力参数均执行严格字符串列表校验。
- TaskTemplateRegistry 增加模板/任务级 `requireVerifiedCapabilities`，能力审查与 Matcher/RoleAllocator 使用同一 verified-only 语义，并严格验证任务/车辆能力列表类型。
- Matcher 参数失败路径现在清空选择/分配输出，只保留 `parametersValid=false`、拒绝车辆和风险信号，避免下游误用残留候选。
- 多机列表的启动任务、单机暂停、单机 RTL 统一进入 GuidedActionsController；直接解锁/接管模式按钮暂禁。批量暂停执行确认时冻结的车辆 ID 快照，显式单机任务确认显示目标 ID。
- PlanView 的上传/清空和 Mission complete 清理增加 active Vehicle 对象/ID、链路和 FleetHealthCenter 二次检查，阻止延迟回调因切换目标而误写；仍未形成统一 mission write transaction 或 ACK 回读。
- Mission resume 的失败/ready signal 增加地面站 request identity、vehicleId、resumeIndex 和 itemCount 快照；孤立 signal 被忽略，ready 阶段要求当前任务项数量与确认快照一致。底层 operation token 仍待工程师在 MissionManager/协议层补齐。
- `GuidedActionList.qml` 的自定义 MAV_CMD 按钮改走 `GuidedActionsController.confirmCustomAction()`；目标、链路、FleetHealth、人工确认和审计先于 `CustomAction.sendTo()`。自由命令 C++ 接口本身仍待白名单/角色/维护模式边界。
- 三个模式菜单入口改走 `requestFlightModeChange()`；普通模式需要健康门禁/确认，RTL/SmartRTL/Land 保留安全逃生路径。通用 QGC 无控制器时保留兼容回退，最终产品需由工程师检查资源映射和模式 ACK。
- `CustomAction`/配置增加 `safetyClass`；未知动作默认 disarmed-only，内置相机动作声明 inflight-safe，发送侧检查 link/initial connect/armed/flying，控制器核对策略并处理 bool 结果。命令白名单、权限和 ACK 仍待工程师补齐。

 - Joystick 自定义 MAV_CMD 增加同一 `safetyClass` 契约；现有 `MAV_CMD_USER_1/2` 显式为 `disarmed_only`，未知命令也默认该等级。`JoystickMavCommand::send()` 在初始连接未完成、链路丢失、已解锁或飞行中拒绝并返回 false，调用方记录策略阻断而不伪造发送成功。
 - Mavlink Console 维护通道增加 `commandAllowed()` 与 `sendCommand()` 双重门禁，仅允许已连接、链路正常、未解锁且未飞行时提交自由文本命令；Console QML 的键盘、粘贴和发送入口统一在失败时显示阻止提示。该层不提供 MAV_CMD 白名单、角色认证或 ACK/超时证明，工程师需在目标 Qt/Mock/SITL 环境复核所有调用方。
 - `CommandTransaction` 车辆身份键改为严格数值契约：`custom/src/Swarm/CommandTransaction.cc` 只接受真实有限整数 1–255，使用规范化十进制键完成事务去重和 ACK 回填，拒绝字符串、布尔、分数、越界和任意文本别名。
 - PlanView 上传入口收口：`src/PlanView/PlanView.qml`、`src/PlanView/PlanToolBarIndicators.qml` 清理/检查 `globals.planMasterControllerPlanView`，顶部工具栏不再无条件调用 controller；两个按钮都进入已有的 QML `upload()` 目标/健康/预检包装。
 - Guided legacy 单目标执行时链路门禁：`custom/res/Merivus/GuidedActionsController.qml` 在确认 token 后统一复核目标初始连接、链路对象和 `communicationLost`，安全逃生动作不绕过传输有效性。
 - 飞行模式旁路收口：`src/QmlControls/PIDTuning.qml` 与 `src/QmlControls/FlightModeDropdown.qml` 在 MERIVUS 上下文调用 `requestFlightModeChange()`，通用 QGC 无该 controller 时保留兼容回退。
 - RC 参数维护入口收口：`src/QmlControls/RCToParamDialog.qml` 在 `sendParamMapRC` 前执行连接/链路/armed/flying/有限数值及 MERIVUS FleetHealth 门禁，失败不发送。
 - 链路缺失语义统一：`custom/res/Merivus/GuidedActionsController.qml`、`custom/res/Merivus/CommandCenterOverlay.qml` 和 `src/PlanView/PlanView.qml` 不再把空 `vehicleLinkManager` 当作正常链路；UI 与执行门禁均 fail-closed。
 - PX4 相机重启入口门禁：`src/AutoPilotPlugins/PX4/CameraComponent.qml` 的 Apply and Restart 在调用 `rebootVehicle()` 前检查连接、链路、armed/flying 状态。
 - 参数维护批量写入门禁：`ParameterEditorController` 的 `sendDiff()` 与两个 reset API 在 C++ 侧验证活动 Vehicle 身份、初始连接、链路和 disarmed/on-ground 状态；参数文件 vehicleId 解析对缺失/非法/混合/跨机输入 fail-closed。`ParameterDiffDialog`、`ParameterEditor` Tools 入口和 `ParameterEditorDialog` 单参数/恢复默认写入同步提示并阻断不安全状态；仍需工程师验证所有其它 Fact 控件写路径和参数 ACK。
 - 兼容 Guided fallback 执行门禁：`src/FlightDisplay/GuidedActionsController.qml` 的 `executeAction()` 重新检查目标初始连接、`vehicleLinkManager` 和通信状态；多机 Start Mission/Pause 发送前预检全量车辆，任一目标掉线则整批拒绝并返回 `false`，避免部分执行。该改动仍不是 MAVLink ACK/角色权限/事务闭环。
 - 兼容 Guided Mission resume 目标链：`actionResumeMission` 不再在 `confirmAction()` 中早退，Mission complete 对话框进入正常确认/冻结目标/执行时链路检查；执行终态清理 `_confirmedVehicle`，避免陈旧确认被复用。工程师仍需验证 MissionManager 恢复上传、operation identity、ACK/超时和页面生命周期。
 - PX4 `SensorsSetup.qml` 校准后重启与 Mission complete 清理入口补强：两者在实际调用前检查初始连接、link manager、通信状态和目标状态，竞态失败关闭；重启/任务清理回执与 QML 生命周期仍待工程师验证。
 - Mission resume/编队回滚边界：`MissionManager::generateResumeMission()` 防止空/短任务索引越界；`SwarmController` 的 Formation ABORT 将 skipped 成员计入失败、未实际下发时保持 rollback incomplete，并在 ABORT 已进行时返回原事务避免重复 ACK handler。工程师需验证空任务、DO_JUMP、ABORT ACK/拒绝/超时和并发归属。
 - APM 维护重启门禁：传感器、安全、电源三个组件的重启按钮和点击路径统一检查连接/链路/armed/flying；通用 MotorTest 与 ParameterManager 已有 C++ 维护门禁，实际重启/MotorTest/参数 ACK、角色权限和其它旁路仍待工程师审查。
 - Formation cancel 旁路：拒绝 `formation.*` 的 generic transaction cancel，要求显式 End Formation/ABORT；QML 记录 `transaction.cancel_rejected`，防止地面事务取消掩盖机载编队状态。
 - Intent fleet identity：预演与批准之间增加 FleetRegistry 稳定快照指纹，绑定构型/发现/能力证据；当前机队变化或外部 stale fleet 会使 intent approval fail-closed。工程师仍需验证 registryChanged、改装证据更新和 QML QVariant 列表映射。
 - Mission resume identity/expiry：QML 恢复上下文保存 request identity、任务快照和签发时间，ready/fail/execute 均有 120 秒过期检查及恢复条件门禁；底层 MissionManager operation token/ACK 仍待工程师补齐。
 - MotorTest maintenance gate：通用/APM 电机测试 UI 与 `Vehicle::motorTest()` 均在连接、链路和 grounded/disarmed 状态不满足时 fail-closed，拖动期间竞态也会阻止发送；角色权限、ACK 和硬件回执仍待工程师验证。
 - Mission resume temporary-item lifetime：`MissionManager::generateResumeMission()` 在去重/过滤临时恢复列表时同步删除被移除的 `MissionItem`，避免重复恢复任务的本地对象泄漏；不改变筛选结果，仍需工程师验证异常任务和真实上传回执。
 - ACK ownership audit：`Vehicle::mavCommandResult` 与 `PlanManager` 的 Mission ACK/完成信号仍缺少 operation/session generation；Swarm 本地 epoch、互斥和后置快照不能证明协议 ACK 属于本次发送。工程师必须在 Vehicle/PlanManager 层增加 token 或超时后的 ACK quarantine，并验证迟到旧 ACK 不会完成新事务。
- Vehicle timeout quarantine：无响应终态在同组件/命令上建立单调时钟隔离，窗口内的新发送 fail-closed、迟到 ACK 被忽略；`REQUEST_MESSAGE`、`SET_MESSAGE_INTERVAL`、`DO_SET_MODE`、`DO_MOTOR_TEST`、`COMPONENT_ARM_DISARM` 例外以保留连接初始化、流配置、模式逃生、连续电机测试和受控解锁/停机语义；孤立 `SET_MESSAGE_INTERVAL` ACK 不推进流配置。该隔离是兼容性缓解，不是 MAVLink operation token。
- PlanManager mission operation identity：`PlanManager` 为 read/write/remove-all 与 ArduPilot guided/resume 写入生成单调本地 `quint64`，保留既有 bool/错误信号并补充带 `operationId` 的完成/错误信号；`PlanMasterController` 的 Mission 串联只接受自己发起的 identity，`SwarmController` 临时 Mission 上传/清理保存对应 upload/clear identity 后再处理回调，并继续校验机载 Mission 列表后置状态。该 identity 仅用于地面站回调/日志归属，不会出现在 MAVLink 消息中，不能替代协议级 token。
- MissionController/PlanMaster 全计划串联：`MissionController` 以 `operationStarted` 和带 identity 的读写/清空/恢复信号驱动 UI 更新，忽略旧无身份完成；`PlanMasterController` 对 Mission、GeoFence、Rally 分别保存本次 operation ID，拒绝迟到/外部回调，Mission 写失败时不再继续 fence/rally 上传。旧 wrapper 信号保留，工程师仍需验证初始自动同步和真实 ACK 时序。
- Plan identity race and entry gates: PlanMaster captures `operationStarted` before manager calls, GeoFence/Rally controllers consume identity-bearing completions, load/send/remove-all fail closed on connection/armed/flying state, and transient send controllers self-clean on missing/failing operations.
- Fact write boundary: ParameterManager blocks direct Fact-to-PARAM_SET writes unless initial connection, primary link, communication and grounded/disarmed state are all valid; a live link requests the authoritative value back.
- Initial-connect identity: Vehicle initial Mission/GeoFence/Rally loads use read operation IDs; old no-argument completion slots are retained only for explicit state-machine skip branches.
- Maintenance and custom-command safety: APM sub-motor page no longer auto-disarms on open and adds confirmation/live gates; CustomAction/Joystick reject invalid/non-finite JSON and only permit reviewed camera capture commands as `inflight_safe`; mode/arm ACK timeouts are quarantined too.
- Bootloader safety boundary: `Vehicle::flashBootloader()` and the Firmware Upgrade advanced button require initial connection, a live primary link, grounded/disarmed state, and a second confirmation; no flash operation is accepted from a stale or airborne target.
- Guided geometry/task snapshots: Orbit stores primitive latitude/longitude/altitude/radius/direction/vehicle identity and rebuilds the coordinate only after execution validation; Set Home/ROI use the same primitive coordinate boundary, and Set Waypoint rejects a changed task-item count.
- Multi-vehicle Pause snapshot: compatibility and MERIVUS Guided paths freeze vehicle IDs at confirmation, preflight every target's transport/API before the first pause, and never pause vehicles that joined after confirmation.
- Backend command gates: `Vehicle` now applies C++ transport/maintenance gates to reboot, calibration, UAVCAN setup, motor test, bootloader flashing and flight-capable command entry points; Guided coordinates/speeds/radius and asynchronous Set Home terrain results are validated again at execution time.
- Mission asynchronous gates: `PlanManager`/`MissionManager` re-check connection and policy at read/write/remove/retry/request/item boundaries; ordinary mission/fence/rally writes and clear-all stop when armed/flying or disconnected, while ArduPilot guided-item writes retain an explicit in-flight exception. Parameter timeout retries reuse the Fact write gate.
- Swarm temporary Mission dispatch: the delayed upload rechecks live target readiness, health, armed/flight state and every frozen coordinate immediately before `writeMissionItems()`.
- Direct Vehicle controls: gimbal, camera trigger, gripper, joystick manual control and RC parameter-map commands now use C++ transport/maintenance gates; RC map values and gimbal angles are finite and bounded. Automatic Home updates and terrain callbacks reject stale transport, invalid coordinates and empty terrain results.
- Initial-connect compatibility: `InitialConnectStateMachine` explicitly passes `loadFromVehicle(true)` for its passive Mission/GeoFence/Rally reads; ordinary UI reads retain the completed-handshake requirement. Mission sequence remapping rejects sequence 0 when the home item is not sent, and successful resume generation releases temporary clone objects.
- Resume index boundary: direct resume generation rejects negative/out-of-range indices instead of silently clamping to item 0.
- Temporary Mission cleanup: `_requestTemporaryMissionClear()` defers `removeAll()` until initial connection, link, communication and grounded/disarmed state are valid; the temporary mission state header no longer contains a duplicate member.
- Generic MAVLink command queue: `_sendMavCommandWorker()` now fail-closes on an explicit communication-lost state and completes the caller with a failure code while retaining pre-initial-connect benign handshake commands.
- The same worker rejects a null VehicleLinkManager with a failure callback instead of dereferencing a stale target.
- The initial-connect read exception is scoped to an active `TransactionRead`; it cannot leak through synchronous completion into a newly started write.
- Airspace checker geometry hardening: `FleetAirspaceConflictChecker` now uses boundary-inclusive segment/polygon intersection and conservative segment altitude/time overlap, and blocks bounded-zone review when waypoint altitude evidence is missing. `timeStepSeconds` remains validation-only and is not a safety certification parameter.
- Motor-test compatibility: `Vehicle::motorTest()` accepts the existing `timeoutSecs=0` stop/disable value and rejects only negative durations; the maintenance transport/grounded/disarmed gate remains required.
- Review timing propagation: `FleetMissionReviewCoordinator` now forwards task/constraint duration evidence to each airspace mission-plan snapshot and derives finite `deadline - startAt` when available; malformed or reversed timing remains blocking instead of being silently downgraded.
- Joystick numeric boundary: `Vehicle::sendJoystickDataThreadSafe()` rejects non-finite axes and clamps finite overshoot to `[-1, 1]` before converting to MAVLink `int16`; this is a local input guard and still requires engineer link/flight-state regression.
- Airspace bounds semantics: geodetic `west/east/south/north` bounds are converted to latitude/longitude vertices, while `minX/maxX/minY/maxY` remains an explicit local-frame form; malformed or reverse-ordered bounds stay blocking.
- Vehicle link lifecycle: `requestDataStream()` keeps the initial-handshake path but rejects offline/stale link managers; queued repeated messages are dropped when the primary link is lost, PID telemetry stream changes fail closed without a live link, and `closeVehicle()` is null-safe.
- Request-message lifecycle: `_sendMavCommandWorker()` now completes the caller's failure callback when the primary link disappears before queue insertion; `requestMessage()` rejects a null callback, a second outstanding waiter, or unavailable transport, and releases request contexts on command failure/message-timeout completion instead of leaving stale wait state or leaking the context.
- Vehicle close/ACK side-effect lifecycle: `closeVehicle()` stops command timers, clears queued retries and completes pending command/message waiters as failures; terminal ROI, parameter-reset and Bootloader UI side effects now run only for ACKs matched to a live command entry, so orphan/late ACKs cannot claim success.
- Risk evaluator health/energy contract: `FleetMissionRiskEvaluator` now normalizes degraded/warning/limited as major review, fault/unhealthy/emergency/lost/disconnected/maintenance as unavailable, treats unknown states as unknown rather than healthy, requires explicit boolean `online` and rejects health reports for vehicles outside the reviewed mission; non-empty energy plans must explicitly carry boolean `feasible` and `parametersValid`, with missing/invalid evidence fail-closed.
- Report-builder health contract: `FleetMissionReportBuilder` uses the same degraded/unavailable/unknown taxonomy and strict boolean `online` parsing; absent/invalid online evidence, string booleans and unknown states generate explicit critical/major issues instead of silently entering a healthy report.
- Candidate health taxonomy contract: `FleetCapabilityMatcher`, `FleetRoleAllocator`, `FleetTaskTemplateRegistry` and `FleetEnergyScheduler` reject `degraded/warning/limited` with `health_degraded`; degraded vehicles do not silently enter capability, role, template or energy candidates.
- Energy candidate identity contract: `FleetMissionReviewCoordinator` forwards compiler/role members and reserves as `candidateVehicleIds`; `FleetEnergyScheduler` filters only that authoritative set and rejects invalid/duplicate IDs, keeping energy feasibility aligned with the actual reviewed assignment.
- Timeline/airspace timing contract: `FleetTimelineReplay` validates black-box-compatible event schema, bounded payloads and monotonic timestamps; `FleetAirspaceConflictChecker` keeps partial-timestamp paths conservative and leaves `timingKnown=false` after interpolation.
- Evidence-chain audit contract: `FleetMissionEvidenceChain` rejects timestamps older than the current chain tail during append/import; SHA-256 linkage remains integrity-only and is not presented as operator authentication or a digital signature.
- Link-metric wiring contract: `GuidedActionsController` forwards descriptor/metadata/linkMetrics quality, bandwidth, latency, packet-loss and state evidence to `FleetLinkDegradationPolicy`; numeric `-1` is treated as not observed, while wrong types/ranges remain strict input errors and no automatic link/flight change is connected.
- Verified capability query contract: `FleetRegistry::vehiclesWithCapability()` accepts an explicit `requireVerified` gate; verified mode matches only `metadata.verifiedCapabilities`, while the default observed query remains compatibility-only and is not a release gate.
- Maintenance passport UI contract: `CommandCenterOverlay` derives one shared review count for maintenance assessments; any non-explicitly-clear status keeps the read-only summary in warning color, so the presence of a record is not presented as proof of maintenance clearance. No health gate or command path is changed.
- Evidence actor-context contract: controller-generated mission evidence now carries a `groundStationAudit` snapshot with operator/role context and explicit `authenticated=false/signatureVerified=false` markers. This improves attribution and missing-identity visibility without claiming cryptographic signing or releasing any command.
- Transaction target-set contract: `CommandTransaction` preserves the requested target list and fails closed on any invalid, duplicate or alias vehicle identity before dispatch; its result exposes `targetInputValid` and `requestedTargetCount` so a partial internal normalization cannot look like a successful full-batch command.
- Extension metadata resource contract: `FleetExtensionRegistry` applies a 2048-entry snapshot cap plus shared depth/node/key/scalar-size limits on both registration and snapshot import; over-limit entries or metadata are rejected without loading binaries, enabling extensions, or releasing flight commands.
- Mission handoff backend policy contract: `MissionHandoffManager::requestHandoff()` enforces second approval and a 1–86400 second expiry bound even for non-facade callers; the QML facade remains fixed at 300 seconds and the manager remains audit-only.
- Mission handoff record-boundary contract: `MissionHandoffManager` bounds task/operator/reason/note text and recursive metadata resources on request/import, rejects oversized raw resolve notes/operators, requires a non-zero `expectedRevision` for interactive accept/reject/cancel (only internal expiry may resolve without a token), requires a real `requiresSecondApproval=true` plus a positive <=24-hour imported expiry interval, and rejects resolved timestamps earlier than creation; complete snapshot validation still precedes atomic replacement and remains audit-only.
- Airspace identity contract: `FleetAirspaceConflictChecker::idFor()` now accepts only a real finite MAVLink system ID 1–255 and no longer parses numeric strings or display-name aliases, matching MissionCompiler and ContinuityPlanner identity semantics.
- Digital-twin snapshot/online contract: `FleetMissionDigitalTwin::setSnapshot()` verifies a supplied `snapshotHash` before accepting in-memory snapshots, and preview fleet rows parse `online` only as a real boolean; missing/invalid online evidence becomes unknown rather than treating a string such as `"false"` as online.

 - Swarm task assignment identity contract: `SwarmMissionOrchestrator::createTask()` and reassignment APIs now reject the entire input when any member ID is invalid or duplicated, rather than silently shrinking the target set; generic reassignment also rejects malformed or conflicting explicit Leader metadata instead of promoting an unrelated first member.

## 工程师操作建议

1. 先阅读 [`MERIVUS_SWARM_ENGINEER_HANDOFF.md`](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。
2. 在独立分支保存当前工作树后生成 qmake 并编译。
3. 只根据编译日志修复接口/Qt 兼容问题；不要删除事务、ACK、超时或安全前置检查来绕过错误。
4. 编译通过后按交接文档执行 Mock/SITL 和故障注入，再决定是否进入硬件流程。

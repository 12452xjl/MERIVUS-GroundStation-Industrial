# 未完成编辑草稿的本机保存与恢复（472，静态交接）

## 485：待确认性能政策不混入已保存编辑态

原保存／恢复入口继续只使用native已提交editingSource。schema5的false／true被完整保存／重读／恢复，未声明的原1..4不自动补false；无本面板额外字段拆解或格式改变。中央Draft validator、Root版本预检及原完整source hash共同保留三态，不靠一个UI开关猜历史含义。

新政策编辑器pending接入父Planner inputSettled；没保存或取消政策选择时，不允许保存、恢复或换原草稿，藏起界面也不能绕过。明确取消只放弃UI拟选择，不删除native已保存政策；清除政策须走独立确认后的窄CAS。

恢复仍只是结构合法、可能不完整的编辑副本：revision1、datum撤确认、原审批失效，不恢复任何当前设备或性能证据。未执行保存／读取／恢复／UI／编译或测试；工程场景全未执行。下方为历史。

## 实际入口

[Planner](../../res/Merivus/FleetMissionPlannerPanel.qml) 在原历史版本恢复面板之前接入
[SavedDraftPanel](../../res/Merivus/FleetMissionSavedDraftPanel.qml)。类型列入
[qmldir](../../res/Merivus/qmldir) 和 [qrc](../../qgroundcontrol.qrc)；
[Root](../../res/Merivus/GuidedActionsController.qml) 私有持有
[Store](FleetMissionDraftStore.md)，公开只给脱离原对象的状态／来源摘要，不暴露完整源或文件路径写入接口。

本功能保存已应用到原生 [Draft](FleetMissionDraft.md) 的完整编辑状态；允许区域不足三点、成员为空、
地图基准尚未确认或当前角色路线尚未完成。它不是已审计划、运行任务、审批或飞行状态恢复。
未应用的输入框文字不在原生源内，不会被保存；不是全表单文本自动保存。

## 显式操作与文件效果

1. 构造、展开、读取属性及重启均不自动访问保存文件，不自动保存或恢复。
2. “准备保存预览”锁实际当前 Draft、原修订、完整规范编辑源及现有保存槽的完整原始字节。
   显示拟保存修订及是否覆盖；此时不建目录、锁或保存文件。
3. 独立勾选保存确认后才提交。固定槽为应用数据目录内 `fleet-os/editing-draft-v1.json`；
   成功替换后完整读回，不接受界面传入任意路径。坏档不是空槽，外部变更不被自动覆盖。
4. “检查上次保存”显式读取并验证完整快照，只发布来源预览，不改变当前 Draft。
5. 独立确认恢复后才创建干净新 Draft。恢复前、撤销旧审批后都重复读原保存槽并核完整原始字节，
   随后才发布新当前对象；本地修订从 1 开始，地图基准必须重新确认。
6. 原当前草稿成为一份共享会话备用；已有备用时另需明确替换确认。
   切回只交换会话内草稿，不是读取磁盘旧版本；三类来源共用同一有界对象／销毁管理。
7. 取消只退休本次来源预览，不删除保存文件、不恢复旧批准、不自动重试。

文件 schema／exact6 结果／exact15 摘要／exact12 信封、容量、完整 hash、字节 CAS、并发锁及不确定提交
的精确边界见 [Store 合同](FleetMissionDraftStore.md)。历史源修订允许 `2^53`，只是救出已耗尽的编辑内容；
不恢复为活跃修订。保存槽修订使用十进制 uint64 字符串，最大值拒绝再增。

## Root 来源与权限边界

新增 `saved-draft` 与既有 [历史计划](FleetMissionArchiveDraftPanel.md)、
[失败部署修复](FleetMissionRepairDraft.md) 区分。只有新来源调用 `loadEditingSource`；
旧 `validatePlanningSource`／`loadPlanningSource` 及 Compiler／PlanStore 的完整可构建要求不放宽。
三来源仍共用原本地切换 ticket、Episode 来源、空闲门禁、候选对象、审批撤销及发布后当前性尾核。

Root 版本预检同时保留无净空资料来源1、完整净空资料来源2；内层净空资料1的四字段及
资料2的五字段（含 contacts）均由原生完整校验，不能用版本号跳过任何保留路线／profile／备用预算。

保存不会改变当前 Draft 或自动撤销审批；恢复必走旧批准撤销与失效流程。
只追加有界 `mission.editing_checkpoint_saved`／`mission.saved_draft_*` 审计摘要；
审计失败不回滚已发生的文件／草稿提交。不会恢复 Run、Vehicle、占用、上传收据、账号、原生证明或启动权限。
原 numericfalse、Compiler pending、完整 StartPermit 和正式结果缺口均保持。

## 输入、确认与反馈

- Planner `inputSettled` 与 planningBusy 共同阻断未应用的设置、顶点、端点、辅助路线、profile、
  备用预算、余段和地图选点等状态；必须先应用或取消，不能默默丢弃文字。
- 保存／恢复／替换备用／切回确认默认不勾选。按下捕获实际控制器、Draft、来源、原生预览、
  Episode、交互 epoch 和替换确认；点击及 busy 通知后重新核对，再调用 Root。
- 独立 contextEpoch 退休控制器、Episode、输入待提交、显隐及展开变化，含 A→B→A；
  正常恢复／切回导致 Draft token 改变，不据此误判原操作失败。
- 单次 feedbackState 将结果与原上下文绑定。后续切换只能显示“原操作已提交但当前来源需重查”，
  不能显示旧来源当前通过；`committed=true/current=false` 不冒充未发生，也不自动重试。
- 沿既有 QGC 原生按钮、主题、键盘焦点及可换行文字。错误摘要聚焦及滚动请求分别复核原上下文；
  新上下文不接收旧错误的滚动请求。这些是 UI/UX 静态原则应用，不是实测无障碍或对比度认证。

## 尚未执行的工程师验证

完整案例见 [交接说明](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md) 的472段。
至少覆盖空／不完整源与旧 strict 拒绝、全部非活动保留字段、clearance1／2＋contacts、历史修订上限、
坏档和外部变更、锁竞争／权限／短写／commit不确定／读回失败、原对象销毁／同修订替换、同步回调换源、
重复 restoreSource 同 token 全文重读、原审批撤销后的复核、共享备用替换、按下至返回的上下文 ABA、
实际提交但随后失效、焦点回调销毁／换站、窄窗口／键盘／主题。

本批只做源码静读和普通文本／路径／Markdown／git 空白核对；没有编译、moc／QML解析、
运行 helper／业务脚本／文件保存恢复／hash／UI、测试或仿真。同步文件操作有容量界但没有延迟上界；
不宣称界面不卡顿。单槽不是多版本备份、跨设备同步、对恶意文件系统竞态的安全边界或断电持久性证明。
工程师后续编译、仿真和实机验证；本模块不得作为飞行放行依据。

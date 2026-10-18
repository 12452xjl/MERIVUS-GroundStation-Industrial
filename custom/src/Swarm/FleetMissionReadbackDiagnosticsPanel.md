# 上传后普通回读差异诊断 · 只读界面

## 425 实际入口与边界

在现有 [FleetMissionDeploymentPanel.qml](../../res/Merivus/FleetMissionDeploymentPanel.qml) 的 `deployment.vehicles` 每机行增加诊断摘要和按需明细；不增加独立页面、QML 注册、Root 方法、命令、重新读取、上传、审批或放行入口。输入只是 [FleetMissionDeployment](FleetMissionDeployment.h) 当前值中的 `readbackDiagnostic`，不调用原生传输或比较方法。

这份诊断解释**当次上传后完成的普通 decoded MissionItem 回读**。它不是 RAW MISSION_ITEM_INT 来源认证、当前机载内容、新鲜度、无线请求因果、身份或启动许可。协议失败、取消、门禁失败、完成证据未确认等未取得有效观察的情况，不用合成空列表冒充 COUNT0；空 wrapper 明确显示未取得或无法绑定诊断。

现有面板的独立 `StartContentCheck` 整组读取与本诊断不同，沿用其原前置条件。本批没有实现失败后的整组重新对账、差异重试或一键修复，也没有新增/改变这些命令按钮，不能把已有成功部署前置流程写成本批新增能力。

## 诊断归属与展开状态

逐行核对 wrapper 的 16 键信封、schemaVersion、固定观察域和历史/非许可标志；deploymentId/generation 必须等于当前部署，vehicleId/writeOperationId/readOperationId 必须等于原目标行。generation、读写 ID 和 observedAtMonotonicMs 是规范十进制字符串，不转 JavaScript 浮点数而损失整数精度。

展开状态绑定实际 controller、原部署、generation、vehicleId、读写 ID、观察时间和捕获状态。身份或 controller 改变立即收起；按下到点击之间再次核对，不能将旧行的展开误用到新部署。切换只影响本地只读展示，不发送请求、停止传输或恢复批准。

`captureState=captured` 才读取 helper 比较；若原生防守分支给出 `item-limit-exceeded` 且实际项数超过 2048，则明确显示未比较。正常协议路径已可能先拒绝超限 COUNT，从而没有确认回读和 wrapper；不承诺所有超限都能显示实际计数。没有比较报告、未知格式或不一致归属均不能显示“没有差异”。没有把旧历史值补到当前 target。

## 默认摘要与字段明细

默认只显示比较状态、预期/实际数量、已比较/未比较项数、已发现差异数、计数完整性、实际展示行数、截断、原因和比较依据。`matched` 文案限定当次规范化内容相同且仅历史；`unknown` 不转成不同或相同，零差异/空 rows 不能独立证明一致。

显式展开后才由行级 Loader 创建最多 64 个差异字段控件，关闭时不常驻 32 × 64 个嵌套明细。展开内容包括归属 ID、单调时间、规范化有效性/错误位置/指纹及原生已给出的字段差异；不额外截断或猜测缺失行。采用 QGC 主题、键盘焦点、可访问名称、PlainText 和长字段换行；公共限制文字不逐机重复堆积。

每行保留 expected/observedState 和原 Type，区分 `missing`、`null`、`typed-null`、`invalid-number`、`unsupported-value`、`item-missing`、`item-invalid`；不将它们转换为零、false 或空字符串。整项存在但 map/list 的 value=null 只表示未输出完整对象，不意味着整项不存在。标量中的 false/0 按原值显示；字符串带引号。itemIndex=-1 明确标整表数量，其他 itemIndex 是 **0 基比较列表位置**，同时注明对应列表第 N+1 项；它不冒充 observed.sequenceNumber，实际协议序号不一致另由 sequenceNumber 字段差异显示。

报告的差异计数可能大于最多 64 行展示；未知/raw-fields-only 报告仍可包含已发现的原始字段差异，但不能宣称是完整规范差异。真实成功空读可被原生标为 different，但 UI 不将其写成双方都完成规范化。invalid-number 包括非有限数及超出安全显示范围的有限数；字段名、状态、类型、错误码和 yaw 字符串按 helper 闭集核对，不展示任意长字符串或复杂值。错误索引 -1 表示未知/不指向具体项。指纹只标识当次规范化内容，不认证远端或提供权限。

## 静态交接

本批只编辑本面板与本说明。依据 `ui-ux-pro-max` 的渐进披露、明确未知状态、原生主题/焦点及纯文本原则实施；未运行技能检索 helper、QML/AST 解析、编译、测试、SQL、UI 渲染、认证或硬件操作。

静态读回对照了 helper 的 28 键报告/9 键行、类型和状态闭集、正常/未知/真实空读比较，以及 Deployment 的 16 键 wrapper、current.generation、原操作 ID 和完成确认后才生成的接线。没有执行这些业务路径。

工程师需另行确认 Qt 信号/绑定、外层布局滚动、展开与来源更换、0/false/null/typed-null 显示、正常/失败/超限/截断诊断、键盘与读屏及实际回读链路。静态文本核对不能证明显示、真实回读或可发布状态。

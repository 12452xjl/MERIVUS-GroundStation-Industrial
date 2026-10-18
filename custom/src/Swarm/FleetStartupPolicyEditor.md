# FleetStartupPolicyEditor：445 显式高度关系策略输入

新增明确选择的 schema 4，沿 v2 的局部运动／航向观察与 `stop-and-review` HOME，增加 `altitudeReferenceEnvelope`，不同时允许 v3 的 HOME 更新包线。固定 `referenceBasis=reported-altitude-relations-not-origin-validity-or-same-sample-proof`；六个实际数值均为 0–1000 m，允许显式 0，空白不是 0，范围不是安全推荐值：

- 相对原审阅基线的三差值变化上限：`maxAmslMinusLocalDriftMeters`、`maxAmslMinusRelativeDriftMeters`、`maxRelativeMinusLocalDriftMeters`。
- 对原报告参考的三残差上限：`maxAmslMinusLocalToOriginResidualMeters`、`maxAmslMinusRelativeToHomeResidualMeters`、`maxRelativeMinusLocalToHomeLocalZResidualMeters`。

schema 4 顶层恰 9 字段，新包线恰 7 字段（固定声明加六容差），沿用共同 `maximumRawGapMs`／`maximumPhaseDurationMs`，不补默认值或隐式升级。界面版本选择、严格 JSON 读入、按版本字段、完整预览和采用共用同一套字段定义；v4 显示 24 个数值字段，v1／v2／v3 仍为 9／18／25 个。

界面明确说明：依赖原始 ALTITUDE／原点／HOME 报告，检查只是报告关系，不证明共同原点、内部 valid、同采样、净空或启动许可。原生连续性从实际启动 scope 绑定才开始，不把审阅等待期说成已连续覆盖。关闭／隐藏、未采用输入、人工勾选和来源失效规则保持。下方438为原三版本流程说明，版本范围由本节扩展；未编译、解析、测试、运行或渲染界面。

此组件是[逐机执行配置](FleetMissionExecutionProfile.md)的本地表单子组件，提供 `startupTransitionPolicy` schema 1、2、3 的全部字段；不改变外层 execution profile 的 schema 1，不取代原生规范化、参数来源绑定、草稿 revision CAS 或后续审阅。数值范围只是输入契约，不是任何机型的安全推荐值。

## 单一文本所有者与接口

- `policyText`：父编辑器 `startupText` 的唯一规范文本，子组件不会直接赋值。
- `editingEnabled`：父编辑会话是否允许本地编辑；不代表飞行授权。
- `sessionToken`：父 `policySession` 对象身份，仅父 begin/cancel 等明确换会话时改变，隐藏不换。
- `pendingInput`：表单已打开且未采用/未明确放弃。父必须据此阻止保存完整配置和并行编辑高级 JSON，避免两个输入源覆盖。
- `policyEdited(nextText, expectedSession, expectedOriginalText)`：只在完整预览、明确勾选和按下冻结后发出。父先核当前 editing、会话对象以及原文本精确相等，再接收文本并调用既有输入失效逻辑；不会直接写入 Draft。
- `load()`：读取并严格校验当前文本；已有未采用输入时拒绝覆盖。`build()` 返回本地 `{ok,error,policy,text}`，`validate()` 返回 `{ok,error}`；都不提交内容。
- `reset()`：父明确开始/取消/换成员时重置本地临时状态；`focusVisibilityRequested(item)` 供父滚动到入口。

关闭 Popup、Escape 或隐藏只清预览确认，不丢弃未采用字段，也不释放 `pendingInput`。明确放弃才保留父原文本并释放 pending。换会话可重置，是父已取消/换成员的结果。父文本被外部修改时保留本地字段但拒绝覆盖，要求明确放弃后重读。父来源失效使 `editingEnabled=false` 时关闭窗口并明确提示；仍可重开查看保留字段或明确放弃，不会因禁用而吞掉 buffer。

采用只表示父配置的未保存文本被接收。同步通知后检查实际父文本和会话；失源、异常或父拒收时不自动重试、不回滚可能已经发生的文本变更。原生保存仍通过父完整 profile、当前参数观察与 revision 的既有入口。组件不发认证、参数、任务、ARM 或 MODE 请求。

## 三版本完整格式

输入非空 JSON 上限仍为 16384 UTF-16 代码单元。严格核顶层及各包线字段数量、字段名、固定声明、真实数值类型、范围和交叉关系；字符串数字、bool、null、缺字段、额外字段与未知版本拒绝。非法原文本不被部分载入或默默丢字段。操作员可明确新建替换表单，但原文直到完整预览并确认采用才改变。

| 部分 | 明确字段与边界 |
| --- | --- |
| 共同时间 | `maximumRawGapMs` 整数 100–3000 ms；`maximumPhaseDurationMs` 整数 100–30000 ms，不续期原审阅或认证 |
| 全球包线（7） | AMSL 最低/最高各 -1000–20000 m，最低严格小于最高；水平走廊 0.01–1000 m；水平速度 0.01–300 m/s；爬升/下降速度各 0.01–20 m/s；相对高度绝对值 0.01–21000 m |
| v2 / v3 局部包线（7） | 固定 ODOMETRY / NED 观察声明；水平/爬升/下降位移各 0.01–1000 m；三速度与全球同范围，且分别不大于全球同项 |
| v2 / v3 航向包线（3） | 偏离/相邻变化各 0.01–180°；累计已观察转角 0.01–3600°；相邻不大于偏离、累计不小于相邻 |
| v3 HOME 包线（8） | 固定原审阅 HOME 观察声明；HOME 更新次数整数 1–128，模式前相对高度变化次数整数 1–256；五项位移上限各 0–1000 m，合法 0 显式保留 |

schema 1 顶层恰 6 字段、`stop-and-review` / `preserve-reviewed`；schema 2 恰 8 字段、`stop-and-review` / `bounded-observed`；schema 3 恰 9 字段、`bounded-observed-after-owned-arm-or-mode` / `bounded-observed`。固定声明随人工所选版本显示，并进入完整预览；不是自行推断的数值默认。切换版本不自动升级、不填入安全容差；采用时只输出所选版本字段。未选版本保留缺政策，不自动创建对象。

局部参考固定 `observed-native-odometry-ned-not-origin-proof`，HOME 参考固定 `original-reviewed-home-observation-not-frame-rebase`；不证明共原点、真实北向、未观察运动、HOME 有效性或净空。完整语义以[原生执行配置](FleetMissionExecutionProfile.md)、[运动观察](FleetMissionStartupMotion.md)和[HOME 观察](FleetMissionStartupHome.md)为准。

明确移除也要预览并确认，输出空文本；父可按原流程暂存缺政策配置，但原受控启动门禁继续拒绝，不能借删除政策解除限制。

## 呈现与工程师检查

沿现有 QGC 调色板和字体，Modal Popup 位于 Overlay，不撑高父规划布局；固定关闭入口、Escape、限高滚动体、纯文本、长声明折行、至少 44 px 控件、键盘焦点滚入可见区。数值字段最多 25 个，按版本显示 9 / 18 / 25 个，无无限列表；失焦及预览时显示范围错误，失败保留输入。确认默认未勾选，原文、会话、字段、版本、隐藏或关闭变化使旧确认失效。

本轮只修改源码并做静态对照，没有执行解析器、技能脚本、编译、测试、UI、认证或设备操作。工程师需验证 Qt 5.15.2 构建、三版本往返、全部边界与交叉条件、HOME 0、非法原文保留、版本切换、父文本同步拒收与通知重入、隐藏/关闭/取消后 pending、窄屏/大字体、滚动焦点和深浅色可读性。

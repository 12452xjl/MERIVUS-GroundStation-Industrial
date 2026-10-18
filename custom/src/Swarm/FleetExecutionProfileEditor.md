# 逐机完整执行配置编辑

## 543：查看后明确采用当前飞控配置速度

原“读取该机当前参数依据（只读）”仍只读，不自动修改任何输入；当前原生 observation 在既有19字段后增加 `speedSuggestion`，为闭9字段 `schemaVersion/type/usable/reason/speeds/sourceParameters/returnSpeedPolicy/configurationOnly/performanceCertified`。`usable=true` 才提供四个有限实际数值；无建议时 `speeds={}`，保原原因，不能把 unknown/null/坏值补0或沿用旧建议。建议是原实际 PX4 多旋翼配置参考，不是最大可达能力、实测性能或续航声明。

完整12参数及本次建议先显示，新增独立“采用当前飞控配置速度”按钮，不与读取或保存合并。四速分别是 MPC_XY_CRUISE、同一巡航值、MPC_Z_V_AUTO_UP、MPC_Z_V_AUTO_DN；第二项是操作者明确沿用巡航的独立返程 NAV 请求，不是独立返航/RTL 参数。原生已检查输入范围、XY与垂直上下限，不由界面截断／clamp出一个看似可用值。`sourceParameters` 四固定参数名及 `returnSpeedPolicy=inherit-observed-cruise-for-return-nav-not-rtl` 同时严格核对。

按下冻结本页 Draft／sourceIdentity／editSession／editorBinding／原revision、输入代次与完整原输入串、原 observation 对象、原 fingerprint/hash 和完整建议；清按下值后先 plain lifetime 检查。采纳同步重读 `observeExecutionProfile(id)`，必须仍是本次原来源、原输入、无未处理策略输入，且旧 fingerprint、内容hash及整份建议全部相同。任一变化只撤旧建议与确认，保原手工输入并要求重新读取查看；不把新的同机号来源重新授给旧点击。

通过后先烧旧确认、退休原预演并尾核；退休未确认时不替换四项输入，防 `fieldsChanged` 内直接调用底层保存函数借用旧确认。随后一次替换整个本地 `fields` 对象，只改变巡航／返程NAV／自动爬升／自动下降四键；原起降坐标、动作、策略JSON、子表单及空间域选择不改，再确认本次不保旧确认。每次通知后核原会话／binding／已安装对象和输入代次，不能覆盖通知中新输入或来源。已有 plain exclusive-edit 闩覆盖读取、确认退休、原预演通知、整笔采用与最终释放，不因自己的 busy 通知自锁。新输入仍需明确整体确认，并走原 Root 重捕双hash、整份规范化和 Draft revision CAS 保存；采纳不自动保存、刷新/写入飞控参数、规划、审批或派发。

工程检查均未执行：有效建议先看后采、范围/上下限不符但参数可查看、缺值/未知不补、返程语义说明、参数过期/同值来源换代/内容修订变化、按下到点击输入ABA、隐藏/换Draft/策略pending/通知销毁，四键整笔替换与保留其它输入、CAS失败及原预演退休未确认。按 UI 技能静态指导复用 QGC 主题、PlainText换行、StrongFocus和至少44源码高度；没有运行技能搜索、渲染、预览、QML、编译或测试，不据此宣称视觉／交互／飞行验收。

下方为此前模块历史。

## 481：独立显式整数域选择与原完整配置同次保存

新增默认不选的语义 CheckBox，对应 profile 可选 `startupSpatialGuard="integer-e7-amsl-mm-v1"`，不改变原 `startupText`、策略表单或 12 项参数观察／哈希。控件有键盘焦点、独立 PlainText 换行说明及原调色板；说明明确运行与审阅共享完整整数 E7／AMSL 毫米域，并附加限制原 Qt 异常域，不认证动力学、实际路径、净空或 StartPermit，旧审阅／审批不能沿用。

编辑态分开保存原字段是否存在与实际值：新旧配置完整载入，缺字段不补值，null／布尔／未知字符串保留并显示错误，禁止保存，不自动当作未选或删除。删除策略 JSON 不会静默取消已选 guard；没有可识别的显式策略时不可新选或保存 guard，必须修复完整策略或明确取消该选择。最终策略完整合法性仍由同一个原生规范化器判断，不凭前端版本标记证明合法。

选择先撤旧确认，再发布新值；同次同步处理有原生通知兼容的 plain busy 闩与 UI busy 状态，阻止选择通知中重入开始／读取／保存／清除／取消。选择前后核原编辑会话、Draft／选择代次、机号与 revision；来源变化保留旧输入但禁止换源提交。隐藏保留选择与未完成文字，撤参数 offer 和确认；明确取消只丢本地编辑，不改已保存 profile。整份保存仍需本次原 12 项参数依据与 CAS，成功必须重新规划／审阅，不自动提交或恢复审批。

工程师待验证（未执行）：默认不选、旧／新／坏值载入，选中后删除／修复 policy，显式取消 guard，选择使原确认失效，选择通知中换源／取消／保存／销毁，隐藏返回和 CAS 失败时保留状态，原 12 项哈希仍可读取及最终后端严格拒绝坏 policy。未运行搜索脚本、解析器、预览、QML／应用、编译、测试或设备操作。UI/UX 技能仅用于静态的语义控件、焦点、禁用态和长说明排版指导，未测量实际布局或对比度。

下方为历史记录。

## 445：显式第四版高度关系策略

子表单新增明确选择的 `startupTransitionPolicy` schema 4，提供六个无默认值的报文差值／参考残差容差；原高级 JSON、唯一 `startupText`、完整预览／采用、参数观察与整份 profile 保存链保持。v4 沿 v2 运动／航向观察，HOME 仍变化即停止再审，不把 v3 HOME 更新包线合并进 v4。

外层 profile 仍 schema 1；新策略必须保存到新草稿版本并重新审阅。表单只负责本地输入，不产生原生高度观察、监视scope、批准或启动许可。新纯契约见[高度关系](FleetMissionStartupAltitude.md)，字段及保留规则见[策略子表单](FleetStartupPolicyEditor.md)。未执行编译、QML解析、界面、测试或设备操作；下方438为沿用的来源／保存流程。

## 438：结构化启动策略与真实草稿来源

[启动策略子表单](FleetStartupPolicyEditor.md)通过父编辑器进入既有整份profile保存链，不创建单独的策略存储或启动入口。父`startupText`仍是唯一规范文本；子表单维护尚未采用的输入，只有明确采用后才更新该文本。高级JSON沿同一文本保留，子表单未处理期间禁用JSON编辑和完整配置保存，不能保存旧策略而忽略屏幕上的新输入。

子组件接口：`policyText`、`editingEnabled`、`sessionToken`、只读`pendingInput`，以及`policyEdited(nextText,expectedSession,expectedOriginalText)`信号。父接收时重新核当前编辑、原策略会话与原规范文本，来源变化不能把旧表单采用到新草稿。采用只修改本地编辑文本，不直接保存Draft、写参数、上传或发命令。

## 来源、保留与取消

`missionDraft.sourceToken`来自Root实际草稿选择代次，并依赖原publication通知；长期facade的地址不能代替它。`Planner`将该标识传为`sourceIdentity`。父开始编辑时冻结facade、选择标识、机号及原revision；参数观察、完整配置保存及清除二次确认都核同一范围。

来源／机号变化后保留原编辑文字并说明原机号和修订号，撤销参数依据与确认，禁用向当前来源保存；明确取消才可重新开始。隐藏保留原字段、策略子表单buffer及policySession，只退休在途editSession与确认／参数offer；返回后须重新读取参数。子表单关闭不隐式采用或放弃，待处理状态继续阻止保存。父明确取消、成功保存或开始新编辑时结束原策略会话。

原草稿revision变化也使`sourceCurrent=false`；不能靠相同ID或相同新revision继续旧范围。Planner成员／角色切换现在统一检查pendingInput，连同配置清除二次确认一起阻断，不静默切换目标。

## 原生提交链

仍通过`setExecutionProfile(id,profile,expectedRevision,observationFingerprint)`保存整个profile，顶层schemaVersion固定1。Root在invalidate回调和实际参数只读观察前后，另外冻结并检查真实backing Draft对象与选择代次；底层草稿替换即使机号和revision相同，也不能承接旧调用。

保存仍须重新取得相同的12项参数内容哈希和原生观察指纹，再由`FleetMissionDraft`执行完整`FleetMissionExecutionProfile::normalize`与revision CAS。清除同样核原实际对象／代次／revision。没有新增参数刷新、写入、权限、批准或发送路径。

调用后返回false、来源变化或通知中断不证明原草稿未提交；界面保留输入供检查，不自动重试或回滚。新政策／配置必须进入新计划和新审阅，不能沿用旧批准。

## 工程验证边界

本批只改源码和文档。未编译、运行QML／解析器／helper、认证、测试、仿真、硬件或刷机。工程师需核表单与高级JSON互斥、关闭／隐藏保留、不同实际Draft同机号同revision切换、回调中换源、原生CAS结果、参数offer失效以及真实分辨率／字体／键盘交互。完整最终启动许可、可信HOME／原点、净空和正式机载结果仍未完成。

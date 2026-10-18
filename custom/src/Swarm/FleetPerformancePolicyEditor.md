# 区域任务性能准入政策编辑器

## 521：strict三态与两项可选任务门槛同次原子保存

既有编辑器增加`minimumSpeedMps`（最低速度能力，m/s）与`minimumEnduranceSeconds`（最低续航能力，秒），不新建组件或资源。两项分别明确启用／移除，初始未保存的旧缺省不暗加门槛；重新编辑显示原草稿实际值，包括显式0。未勾选代表本次保存移除该键，数值0代表明确没有正数下限，两者仍保不同来源身份。它们不是本次巡航／返程速度、对工程max声明的改写，也不是当前剩余电量／飞行时间或试飞认证。

启用后须填写有限非负十进制数，域`0..9007199254740991`；空白、单位、逗号、指数、负数、非有限和下溢不静默变为0。用户文本先验证再转为实际Number，传给后端的完整policyMap仅含0到3个可选键：`requirePerformanceEvidence`实际Bool，以及两项实际Number；没有数字字符串／bool数值转换。已保存的合法小Double可在表单中展开成无指数十进制，保留原值可再次编辑。将保存的实际规范数值与未设置项在确认前并列显示。

通过原窄Draft facade一次调用`setPerformanceAdmissionPolicy(policyMap,expectedRevision)`。Map内缺失的这三个键明确移除，其它settings／预算保留；空Map清三键，不把关闭strict当作自动删除门槛，也不根据门槛自动改strict。新schema6只在存在至少一floor时使用；无floor仍保原source1..5合同，旧strict专用API只改Bool而保已有floor。本页不自行生成规划来源、hash或批准。

编辑session固定actual draft、sourceIdentity、原revision及完整三键presence/value身份；门槛文本、启用状态、policy选择、确认代次与按压对象全部参与收尾。纯文本编辑持plain重入票据而不逐字切换控件enabled／焦点；改变任一拟值先撤确认，旧确认false→true或数值A→B→A不能复用旧pressed。保存票据跨调用及所有结果通知；同完整policyMap为no-op不升revision，真实修改预期仅加1，并核实际savedMap完整对应。false或通知后换源可能已有本地提交，只报告未确认／核对原草稿，不自动重试、恢复旧审阅或推断回滚。

`pendingInput`仍覆盖完整编辑与调用，父Planner排除本子pending来计算本子inputSettled；未完成输入阻断其它预览，隐藏／禁用只撤同意而不默默保存或采用。取消只清本地编辑，明确移除floor须另外确认并实际保存。严格模式由当前本机工程声明及完整构型基线满足正门槛；非严格仍按原规划描述检查，不能把未知／−1当作正数门槛通过。原Compiler／Matcher／RoleAllocator的真实消费者仍为权威，不能用表单显示代替来源或性能认证。

UI沿原QGC主题、明确单位、独立开关、PlainText换行、44以上高度、键盘焦点、近场错误和取消入口；ui-ux-pro-max仅使用完整技能及两参考的静态通用指导。未运行技能搜索、QML／helper／应用／解析、编译、测试、仿真或设备；本批工程场景均未执行。下方485三态单键保存描述为历史；本节同次三键编辑优先。

## 485：实际三态入口与原源提交

[QML组件](../../res/Merivus/FleetPerformancePolicyEditor.qml)进入现有[Planner](../../res/Merivus/FleetMissionPlannerPanel.qml)，由[qrc](../../qgroundcontrol.qrc)和[qmldir](../../res/Merivus/qmldir)各登记一次，无新C++类型／依赖。它只接原窄Draft facade，不拿Registry、Session、Vehicle或任意审批引用。

选择分三态：未声明（清新键、回原source1..4）、明确false（新source5但不启用严格）、true（新source5且由483消费者严格准入）。准备编辑读取原态，不默认选true／补false；选择需明确确认，再保存到冻结的原draft/sourceIdentity/revision。两种降级也明确警告旧审阅失效，不能误解成性能达标或其它安全门被关闭。

编辑使用独立lifetime／ticket／contextEpoch，所有可通知步骤核原source／修订／选择。切选项先撤确认，按压绑定session／epoch／policy；Enter不直接保存，准备后焦点先给取消。native同值no-op与实际变更＋1分别核收尾；通知中换源／取消／尾核失败只说明未确认或可能已提交，不重试、不回滚。确认只是本地编辑意图，不是飞行授权。

pendingInput始终editing或callBusy；隐藏／disabled／其它输入未settled时撤确认但不清editing，原选择保留，不会因藏起控件让旧值进入保存／预览。父Planner将它加入统一待处理门和其它三编辑器互斥门，自身inputSettled排除自身pending，避免正常自锁。明确取消只清本地编辑，不调用native清除政策；恢复旧缺省必须独立确认并实际调用clear接口。

UI/UX技能影响三态说明、PlainText换行、44像素以上控件、文字／非颜色状态、默认取消／焦点／明确确认。仅静态读源，未运行技能搜索、QML、布局／暗色对比度／读屏／键盘／翻译测试；这些列入[32项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。不认证实测性能、机体配置或飞行许可。

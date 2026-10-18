# FleetCoverageExclusionsEditor · 476 排除区编辑与地图标识

## 477：孔与名义退让同次确认／提交

原组件新增setbackText及明确可选距离标签，不新建页面或默认安全值。开始时从原settings读取，留空移除退让声明，0保显式不退让，正值范围0..500只作名义规划输入。JSON与距离一份确认，经Root.setAreaPlanningPolicy→native同次revision-CAS完整提交；不能先保存新孔、再以旧版本保存退让。编辑器仍保显式exclusionPolygons（包括[]），所以移除退让后通常为source3；原生完整configure可明确移除全部optional回到旧1/2。

两种文本实际改动都先撤确认／换opaque epoch，再发布文字，防Changed通知中旧同意提交新值；任何字段错误保两份文字、撤同意，不把错误/空白改成默认0。保存成功后的session／确认／editing／message各通知后重核原source／committedRevision／epoch／两原文及期望状态，变化即保当前编辑，不声称回滚已经发生的本地提交。取消也持plain+QML ticket到所有清理通知后，新begin不能在中间被打开又清空。隐藏／换源／销毁和父pending互斥保持。

地图仍绘制原输入边界／孔，并明确标注已声明名义退让但未画派生可用域，不把原轮廓当已退让安全域。生成的实际审阅路线由原绘制链显示；正退让不证明编码量化后路线、真实障碍净空、定位误差或动态运动。UI/UX静态指南继续影响标签、取消、失败原文与重新确认，未运行脚本／界面／可访问性检查。

此QML位于`custom/res/Merivus/FleetCoverageExclusionsEditor.qml`，实际实例在PlannerPanel区域边界后，沿用QGC控件、语义主题、可换行文字与键盘操作。没有新增命令接口、Vehicle对象或飞行权限。UI/UX静态指南影响了明确标签、原文保留、就近反馈／焦点、默认未确认和隐藏撤同意；没有运行技能脚本、QML、预览或可访问性测量。

## 当前编辑合同

- `draft`是既有窄门面，`sourceIdentity`绑定实际draftSelection的sourceToken，`inputSettled`排除本组件自己的pending，避免自锁；父面板的整体pending包含本组件，未提交编辑阻断普通预览／批准、保存／恢复和其它余段输入。
- 明确开始编辑先冻结原门面／source／revision并撤销旧审阅；JSON文本来自本地已保存值，不从地图或实时飞机位置猜点。原输入仅本地暂存，最大提交文本32768字符；最多8个完整环，每环3–64个保留点，总外环加孔128点由原生Draft再次验证。
- 输入格式为`[[{"latitude":数值,"longitude":数值},…],…]`。`[]`表示明确替换为空几何排除区，需要重新勾选确认；它仍保source3的显式空声明。原生clear API另可移除可选键，但本编辑器不隐式调用它或篡改其它设置。
- 每次输入、失败、隐藏、禁用或外部输入未完成均撤销同意。来源变化保留原文字但使原编辑版本失效，必须明确取消后才能编辑新来源；不自动把旧输入搬到新草稿。
- 保存经Root门面冻结原backend／selection／revision，通知前后重核；原生setter再次CAS后完整提交。成功后必须重新当前审阅，旧批准不恢复。失败或上下文在通知期间改变不能推断原本地提交已回滚，应核实际草稿版本。
- 独立opaque contextEpoch防源／可见性／输入ABA使已开始的动作续作；QML callTicket负责ready通知，plain lifetime ticket保留到busy与ticket所有发布完成，防重入和永久禁用。通知及焦点造成销毁时不再续写item。
- qrc alias与qmldir登记，无新C++服务或第三方依赖。功能状态只以代码和有限静读为依据，未确认实际界面行为。

## 地图与真实消费者

FlyViewMap仅在当前普通草稿图层读取完整settings.exclusionPolygons；完整校8环、单环点数与总128点后一次发布，任一环不合则不画孔前缀。橙色边界／填充另配“排除区N · 待审几何，非净空证明”文字，不只靠颜色区分。切到启动／余段／历史地图来源时不混画当前草稿孔，也不将屏幕几何当已批准来源。

草稿source3→task.area.holes→[真实覆盖算法及共享检查](FleetAreaCoveragePlanner.md)→[Compiler最终所有名义路线](FleetMissionCompiler.md)；保存／恢复仍使用[中央Draft合同](FleetMissionDraft.md)。旧schema1／2不允许偷偷携带排除字段，source3可以同时保留完整clearanceInput。现PlanStore与DraftStore通过原生中央验证保存完整源，不从摘要重建孔。

排除区仅是名义二维内部约束：允许沿边界，无安全退让、机体半径、3D地形／障碍、动态轨迹或执行许可。地图可以显示尚未通过拓扑检查的编辑输入，不能当作绿色准飞提示；需要重新审阅并独立工程验证。

工程师未执行场景见[交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批未编译、运行或测试，不证明可交付实飞。

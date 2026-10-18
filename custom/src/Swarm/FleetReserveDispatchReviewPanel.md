# FleetReserveDispatchReviewPanel：独立备用出勤审阅明细

## 515 范围与真实输入

这是嵌入既有备用出勤流程、审阅后采用前的只读子面板。唯一业务输入为父页**自身持有**的实际 `reviewContext`；没有 controller、原生来源读取、文件导入、审批、上传或飞行接口。`focusVisibilityRequested(item)` 仅用于父级让键盘焦点进入可见区域。

输入须为 `kind=reserve-dispatch`，有对应 token/proposalToken、当前性／采用／批准资格的实际 Bool 和固定 `flightCommandReleased=false/executable=false`。记录的 recordId、sourceTaskId、storeEpoch、正 recordRevision、planId/contentHash/reviewEvidenceHash 与唯一新成员、角色和新任务 sourceTaskId 对应；来源仅接实际 `reserveDispatchPlanningSource`，保留 `historicalOnly=true`、`authorityRestored=false` 等边界。外层不固定键数，兼容真实 Root 新增的 reviewAttemptToken 等工作流字段。该显示检查和内容 hash 都不认证来源或恢复权限。

协议显示读取同一 `reviewContext.view`，由现有 [FleetMissionSpatialReview](FleetMissionSpatialReview.md) 从实际归档计划／审阅依据独立重建；**没有构造假的启动 capture**，没有把局部编码重新交给 Builder。以当前 `.cc` 为准：整份 view exact17、唯一 vehicle row **exact15**、每 encoded item exact9。旧文档或手工计数不得成为加字段／漏字段的理由。

## 协议项与编码失败

默认折叠；只在主动展开后实例化最多8条协议委托与最多8条当前层级字段委托。协议页保持原 sequenceNumber、command、frame、sourcePath、sourcePointIndexKnown/sourcePointIndex；未知索引保留 null，不补0，不把源点位置当协议 seq，也不把速度／动作参数解释成位置。

点击某项后，可逐字段查看原 `rawItem`、`canonicalItem` 与 `wireQuantization.coordinate/floatParameters`，包括其原浮点／E7／量化细节、来源和序号，不再量化，不修改 payloadHash。items 全体的9键、顺序、三组映射序号和同一成员绑定必须一致才显示，不发布成功前缀。

几何和编码分别显示。`encodingAvailable=false` 时保留完整错误对象、未编码要求、动作／源时间以及父页原路线资料；空 items／空 hash 不等于任务没有协议项。明细缺失或 Spatial 合同不一致时，只报告显示不可用，仍允许浏览同一已绑定审阅的真实能量、风险、空域、协议构建结果和其余原因；不因此改变原审阅／批准门禁或已归档事实。

## 完整值的有界浏览

选择器包含：单项完整协议字段、完整 energyPlan、riskAssessment、airspaceReport、airspaceInputCheck、归档空域显示依据、几何／时间／动作 row、完整协议构建结果、完整新 review 和显示 limitations。既不裁成少数风险摘要，也不整份 stringify／pretty 巨大对象；只有小型稳定身份采用 JSON 字符串比较。

每层最多8个可见子项，保留完整子项总数和页号，并可返回上一级；数组按原索引、对象按完整 key 排序。支持 Root 实际快照合同的每数组32768项、每对象1024键、最大24层。超出界限明确显示“未显示有效前缀／未被核验”，不能把资源拒绝当完整空对象或成功。这里不是新的全树验证器，未展开不等于工程师已经核对。

长 key 的列表标签明确写出只是首片；进入该字段后可以逐片查看完整 key。字符串和值每片约256个 UTF-16 单位，代理对跨边界时保留完整字符、相邻片可重叠该字符，不丢原字符。类型与原字符串长度单独展示，缺失／undefined、null、false、0、空字符串、空数组、空对象不互补；负能量 margin、-1和其它数值均按原值显示，不自行解释为可用／未知或通过。

风险概率仍是原确定性启发式，能量仍是原声明、观测和估算组合；空域 feasible／analysisComplete 不变成净空认证。显示协议请求也不证明机载接纳、写入、任务完成或真实轨迹。所有文字和动态数据使用 PlainText，不执行资料中的字符串。

## 身份与界面生命周期

仅稳定 token 与归档7项身份、planId／成员变化时重置展开状态、各层路径与分页。`current/accepted/approvalAllowed` 变化保留同一记录的浏览页，并准确显示其当前投影／历史状态；没有恢复父页确认或领取共享其它结果。

浏览状态采用一次终结式赋值；按压冻结小型身份与原 navigation 对象，真正换记录或 A→B→A 后旧按压不能导航新记录。通知后的续作用原局部 lifetime 检查；聚焦后核原身份／导航再发父级可见信号。折叠不实例化明细委托，不自动重采、重审、轮询或操作飞机。

沿用 QGC 主题和字体，主按钮可换行、最小高度44、支持键盘焦点；分页窄宽时改为纵向，不靠颜色传达状态。实际布局、对比度与辅助技术表现尚未运行验证。

## 工程交接（全部未执行）

需工程师核验 Qt5.12/QML/资源编译、窄宽和字体放大、键盘下钻／返回／焦点滚动；实际绑定7项、sourceTaskId、role、唯一成员与固定false；view17/row15/item9和 sourcePointIndex=null；2048项分页与失败整份不显示前缀；raw/canonical/量化/原源时间；几何可用但编码失败；完整energyPlan/risk/airspace负值和未知标签；32768数组／1024键／24层界限；长字符串、长key、代理对与空值区分；折叠无委托、current变化不跳页、新记录与A→B→A撤旧按压；销毁和焦点通知重入；本面板无Root调用、审批、上传或飞行命令。

本轮仅源码、文档及直接静态阅读，未执行技能脚本、解析器、项目、QML、编译、测试、仿真或真机操作。

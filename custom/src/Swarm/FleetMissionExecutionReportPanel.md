# 冻结运行复盘报告界面（429，静态交接）

## 541：模式退出证据与旧版未采集

原 full 报告与独立 protected 区接纳版本1/2并核 report／observation／record／targetOutcome 的匹配版本。v1 逐机摘要仍 exact14，不容新字段；v2 exact15 必有 `missionModeExitObservation`，由原生完整共享 codec 校验后再作有界显示检查。protected 文件仍解析原生 jsonText；其退出值不从当前飞机或另一份报告补入。

full 原有“逐机历史观察”展开显示新证据；protected 的原 JSON 展开按钮同时按需显示最多32机的首／末退出证据。仅实例化展开内容，沿原滚动、PlainText、长 ID 换行、主题与焦点，不增加控制按钮、自动捕获或自动重试。显示原 customMode/baseMode/systemStatus、两条 sourceEpoch／完整序号／本机单调毫秒接收引用和当时连续性未知；退出计数不转 Number。原四种终端候选文字不变。

v1 缺字段明确“旧版未采集”，不是零次退出；v2 count0 明确“本地观察中未见”，不证明持续执行、任务完成或停飞。首末两条不冒完整历史，模式退出不冒中止原因或任务结果。资料换源、隐藏、文件确认及实际提交事实仍沿原生命周期，不由新显示恢复资格。

已全文读取 UI/UX 技能及静态参考，依用户限制未跑技能脚本、预览、布局、键盘、读屏、QML、编译或测试。下方为历史合同。

## 494 当前：本区显式文件历史与待确认意图

在原protected折叠区新增独立保存／重开动作，仍单面板持一份snapshot。captured与imported来源明确区分；导入仅解析native已验证canonical JSON，绝不喂旧full9全局槽、catalog、protectedSelection或原Run。即使文件sourceId与目录巧合同值，也不是当前原生捕获。目录失源不清导入历史或悄悄更新它。

文件选择与全页fileDialog互斥；路径先冻结controller、life／protected／file代次、同一snapshot对象、ID/hash/jsonText，返回后仍需明确确认，保存另告知可能覆盖。取消／报告替换／选择ABA／折叠隐藏或换controller退休旧路径、同意、按压和窗口；按压后的确认取消再恢复不可复活旧意图。持ticket跨公开属性通知，逐次核guard和原上下文，不能用busy期间必false的ready代替原操作尾核。

文件打开不要求当前飞机／Run／有效目录或账号；只是本页外部历史。save调用Root新五参确认口，load新两参确认口；保存actual writeAttempted／written含未知与current资格分开，不自动重试或把后续失效说成回滚。新文件接口由[完整原生codec](FleetMissionExecutionReport.md)验证，前端仅核显示对应，不自证来源。

UI/UX技能只静态指导现QGC控件、可见标签／确认／取消、纯文本长ID路径换行、主题／焦点与44px操作高度；未运行搜索脚本或加载界面，非视觉或可访问性验证。[36项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)保留。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 489 当前：保留记录独立选取／单份捕获与原生JSON

在现面板增加默认折叠的“本进程保留Run”区，不新建页面或改变当前观察Episode。Root只给Swarm轻目录值和选择请求窄函数；旧full报告槽、文件弹窗／保存／导入／handoff路径不变。新JSON格式明确独立，不交旧接口处理。

目录无默认首选；用户明确选row13、核task／run／修订／token、确认后按压捕获。selection保存四字符串，不把UInt64转Number。选择与清理先持票据，再逐通知核原lifetime／historyEpoch；捕获持票据跨busy清理，通知回调不能用旧row覆盖新选择或清理后再写旧消息。hide／collapse／换controller／源丢失清本区；选中记录推进撤选择但旧同源报告仅作为冻结历史保留，不自动更新或认领他面板报告。

返回4含native jsonText；Panel解析该文本并核report8／record16／summary13／claims22及原请求，单份快照同时保存parsed报告与原始文本。不JSON.stringify Qt map，避免undefined使未知键在复制时丢失。原生完整语义校验不由前端显示检查替代。文本需显式展开，只读可人工选择复制；不写文件／剪贴板、不上传。

UI/UX指导用于短确认标签、PlainText／长ID换行、主题色、44px最小控制高度、明显历史／未知／耗尽和失效说明、键盘焦点与显式空选择。未运行技能搜索、QML、布局／焦点／读屏／对比度或交互验证。[完整后端合同](FleetProtectedRunReport.md)及[36项未执行矩阵](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)保边界。仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

## 入口与语义

`FleetMissionObservationPanel` 的独立入口区增加默认折叠的 `FleetMissionExecutionReportPanel`，不放进结束监视审批弹窗。展开不会捕获、选择文件、读取或写盘；本批没有定时器、自动捕获或自动导出。此报告不是飞前审阅报告，也不关闭、退休或修改 Run。

展开内容放在独立 `Flickable` 内，摘要、按需逐机明细和文件确认操作共用同一个滚动体；高度随较小内容收缩，上限为 18 个默认字体行，避免撑高 Planner 的固定入口区域。按钮和确认框具有可见键盘焦点，聚焦时滚入此区域内。没有改动 Planner 或指挥中心的外层布局。

用户可以明确捕获当前选中的实际 Run，或在没有飞机、账号、当前部署时打开一份 JSON。捕获冻结的是调用时的真实本地观察；后续实时 Run 更新不更新此报告。新捕获或打开会替换本页报告，按钮明确提示替换。清除仅清本地冻结值，不删除已导出的文件，不恢复或撤销任何任务许可。

来源分别标记为本地捕获的历史、外部导入的未认证只读历史。内容 hash 和结构校验不是来源认证、签名、遥测完整性、当前机载内容证明或任务结果认证。导入不恢复任务、批准、执行权或飞行指令。捕获时源绑定不表示持续绑定或新鲜度；监视退休不表示停飞、落地或任务完成。

## 统计显示

显示固定 report 9 字段中的标识、UTC 捕获时间、来源、Run/启动/部署和本地观察任务标识；未登记的观察任务显示未知，不补造成功状态。只展开固定 summary 13 字段以及最多 32 架的 14 字段逐机摘要；收起时不实例化逐机行。全部外部文本采用 `Text.PlainText`，标识和路径可换行。

MODE 计数称为“本地 MODE 入队覆盖”，不是执行率或起飞率；零提交不代表整组失败或从未起飞，前序操作仍可能发生。来源未知计数可与线索类别重叠，不能从剩余成员推出成功。终端项、无限期保持和终端后落地均仅是历史线索，未观察到线索也不是失败结论。current/reached 序号明确为 0 基协议序号，null 表示未观察到，不改写为零。

## 文件选择、确认与失源

读取与保存分别使用固定 `selectExisting: true/false` 的 `QGCFileDialog` Component，每次显式打开创建独立对象并冻结控制器、视图代次和报告 ID；不在旧对话对象上复用新意图。空 folder 走现有完整文件选择器，不进入该控件有目录浏览/删除菜单的自定义 mobile 分支。本批未修改或实际打开文件选择控件。

保存分两步：用户先选路径，界面仅暂存冻结 reportId、contentHash 与路径；再显示完整路径及可能覆盖同名文件的警告，默认未勾选，明确勾选并再次点击才调用保存方法。报告换 ID 或内容、隐藏/折叠或更换控制器会废弃待写路径、确认、按下意图与旧对话。额外绑定 contentHash，避免不同外部报告恰好使用同 ID 时沿用旧选择；该 hash 仍不是来源认证。保存时再次核对冻结值，不能把后来捕获的新报告替代用户原先选择的那份。取消路径不写盘。

每个对话回调检查独立对象身份及存活 guard，旧窗口关闭后不能借新窗口意图执行。选择期间上下文变化会拒绝路径；打开外部报告不要求当前 Run/飞机/账号。异常提供明确文字，不自动重试或宣称回滚。

## Root 接口与回执

只读取 `fleetExecutionReport = {report, origin, busy, error}`。显式调用：

- `captureFleetExecutionReport(expectedSourceToken, expectedRunId)`；不把 UI 拼出的观察当成真实来源。
- `saveFleetExecutionReport(filePath, expectedReportId)`；仅在上述第二步确认后。
- `loadFleetExecutionReport(filePath)`；只读校验并替换本地报告。
- `clearFleetExecutionReport(expectedReportId)`；不删除文件或改变 Run。

各调用返回 `{ok, error, committed, current, fileWriteAttempted, fileWritten, reportId}`。界面区分本地/文件已提交事实与结果仍属当前：已写成功即使后续失源仍显示已写；`fileWritten: null` 或异常/未知回执明确提示文件可能已写，应人工核对，不能把失败当作未写入。未确认的本地替换也不能声称报告完全没变。此结果不是任务完成或飞行许可。

## 本批检查与后续验证

仅做源码静态读回、DTO 对照与定向 `git diff --check`，未编译、运行 QML/技能脚本/解析器、测试、打开文件选择器或连接设备。工程师应验证真实 Run 的零/部分/全部本地 MODE 提交、未知来源和退休观察、外部只读导入、非法/超限文件拒绝、具体报告 ID 导出、确认覆盖、取消与重新打开、报告替换/控制器替换/隐藏/销毁、已提交后失源和写入失败回执，并验证窄屏、最大字号、32 架明细滚动及键盘焦点可见。不得据此宣称运行、遥测完整性或飞行放行已验证。

按 `ui-ux-pro-max` 静态指导复用 QGC 控件/调色板，采用明确文字状态、可取消路径、默认折叠、按需有界明细、键盘焦点与长文本换行；受用户限制，未执行技能的脚本或视觉验证步骤。

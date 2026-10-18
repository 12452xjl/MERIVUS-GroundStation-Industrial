# 本地运行简报交接面板

## 541：简报 v2 的任务模式退出证据

现同一 Run 预览、收件详情及独立文件历史候选均接纳 handoff17/schema1或2；schema1逐机 exact14 无退出字段，schema2 exact15 必带完整 `missionModeExitObservation`。版本按原 native codec／摘要匹配，不把旧文件升级、缺证据补0或从当前 Run 恢复文件来源。

沿原按需最多32机明细展示规范计数、首／末 HEARTBEAT customMode/baseMode/systemStatus、此前任务模式及退出接收引用（原 sourceEpoch、UInt64序号、本机单调毫秒）、当时连续性未知。旧版显示“未采集”，新零计数显示“未观察到”；两者都不证明未中止、持续运行、任务完成或停飞。首末值不是完整历史，接收顺序不证明请求因果或远端结果。文件候选沿原明确未认证提示，不变成真实 Run 捕获。

不改变交接发送、接受／拒绝、文件保存／加载／合并、来源确认和已提交事实生命周期；不转移飞行控制或批准。UI 技能仅用于静态纯文本、换行与按需展开指导，未执行技能脚本、QML、预览、编译或测试。下方为历史合同。

## 539：交接历史文件闭环

原模态窗口中新增独立文件历史区，保留原 Run 简报冻结／发送／处理流程。明确保存当前记录到本地 JSON；明确加载只形成整份校验后的历史候选，不替换活动集合；随后必须另行准备与二次确认，才请求后台 append-only 历史合并。未创建第二个 Manager、独立窗口或新的传输／控制架构。

文件历史无需真实 Run、账户或飞行权限即可离线读取；这不恢复认证。新增记录永远标作只读导入历史，包括通用 `pending` 记录：原状态保留供审计，不进入待处理徽标／收件箱，不允许接收、拒绝、撤销或定时器改写。原本实例 Run 记录及旧接口的历史兼容策略仍由原生模块负责，不由页面重写。

### 明确请求与原任务

门面为 `historyState/historyPreview/historyResult`、`requestHistory(intent,true)`、`historyPreviewPage(previewId,page)` 与 `historyPreviewRecord(previewId,handoffId)`。公开请求精确五键：`action/expectedJobId/filePath/previewId/overwriteConfirmed`，四个实际字符串与一个 Bool；不传文件内容、候选 map、hash、记录表、权限或任意 live replacement。

| action | 页面规则 |
| --- | --- |
| save | 独立本地 JSON 路径与明确覆盖确认；路径变化撤销原覆盖确认。复选框也在按下时冻结 owner／路径／页面代次／原 job，旧释放不为新路径授予覆盖确认。冻结并保存实际原生集合，不自动保存，不以 Run 的发送回执冒充文件保存。 |
| load | 仅路径；覆盖位为 false；先后台完整读取与校验，再发布历史候选。失败不导入合法前缀。 |
| merge | 仅原 native previewId；先准备、再单独确认；确认绑定原 owner、页面代次、路径、原预览、原 job 与集合 publication。只追加；同 ID 同规范内容去重，异内容整笔拒绝。 |
| discard | 只丢弃准确原候选；不删除文件、已追加记录或飞机动作，保留此前文件结果。 |
| cancel | 只取消本页接纳的准确原 running job；不取消 Run 监视或飞机动作，且必须等待实际后台结束。 |

所有请求带原 `historyState.jobId` 为 `expectedJobId`，初始为空。`save/load/merge` 各生成新 UUID 异步 job；十一字段 receipt 的 accepted 仅代表受理，绝不当作写入／加载／合并完成。原生等待 actual `QThread::finished` 后发布终态；页面再按原 owner、action、jobId、闭字段结果与非 busy 状态接纳。`merge` 也不在点击或受理时显示“已追加”；后台完整比较／构造后，原集合 generation 与原私有候选 CAS 通过才产生新增事实。

`discard/cancel` 沿原 job ID，不制造新 job。discard 仅清 native 预览，原 state.action/stage/job/result 保留；取消不提前释放 busy。取消／换源／隐藏都不倒置已提交的保存或合并事实，失败不推断“此前无记录”，不自动重试。合并失败仍由原生保留可用预览时，可重新准备并明确确认；旧确认不因重新同值而自动复活。

### 小值合同与候选浏览

- state 精确十一键：`schemaVersion/busy/jobId/action/stage/cancelRequested/previewId/historicalOnly/permissionsRestored/executionPermit/flightCommandReleased`；阶段 `idle/running/cancelling/complete/failed/cancelled`。
- 受理回执精确十一键：`schemaVersion/action/accepted/jobId/reason/requestOnly/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。
- 结果精确十一键：`schemaVersion/jobId/action/ok/error/reply/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。初始 `{}`，失败可 `reply={}`；这种情况明确提交未知，不虚构零新增或未写入。
- load 的 reply 为 `{preview}`。预览精确十二键：`schemaVersion/previewId/jobId/path/contentHash/recordCount/runRecordCount/historicalOnly/authenticated/permissionsRestored/executionPermit/flightCommandReleased`。显示整份完整校验数量（最多 100000 条，其中 Run 简报最多 256）、文件路径与 SHA-256；不是把 GUI 的四条当前页当全文件。SHA 只是内容完整性标识，不是签名。
- 保存 reply 沿原十一键：`snapshotId/contentHash/path/fileWriteAttempted/fileWrittenKnown/fileWritten/readbackVerified/readbackError/fsyncVerified/durable/authenticated`。snapshotId 是原 save job；原路径字串保留，不做 UI 路径转换。分别显示尝试、写入已知性、实际写入和回读；写入未知不把 false 当作未写入；commit 后回读失败仍保已写事实。fsync／断电耐久／认证均未验证。
- 成功 merge 的 reply 精确四键：`previewId/addedCount/duplicateCount/recordCount`。只有实际成功结果才显示该新增／重复／合并后总数；`addedCount+duplicateCount` 须等于该原候选完整数量。成功后 native 清候选，不能借旧副本再合并。
- 候选页沿原四键 `page/pageCount/total/records` 与十四键摘要；候选详情取得原完整十六键 DTO，所有 origin 都必须为 `imported`。界面显示 ID／状态、收发人、创建／到期／处理时间与处理人、原因／备注，以及 Run 简报摘要；最多 32 架逐机观察由现有 `vehicleText` 按需展开。复用旧简报文本之前明确提示：Run 身份、冻结时间和逐机观察均为文件声称，未认证，不证明本次从本地真实 Run 取源或真实飞行结果；不改变原实际 Run helper 的文案域。通用 metadata 不作任意原文编辑器，也不宣称所有通用字段都已渲染。明确点击首页、翻页和记录才按原 previewId 读取，页数／总数须对应完整候选；失败不拿当前 live collection 作为候选。候选详情只有历史展示，没有接收、拒绝或撤销按钮。

整份文件与候选记录保存在原生后台／私有值中，不把完整文件 JSON 或全部记录传给 QML；界面只复制小结果、每页四条与一条按需详情。原生文件安全、编码／图预算、32 MiB、取消与合并全库预算仍见 [MissionHandoffManager](MissionHandoffManager.md)，不是 UI 自行解析、校验或 hash 文件。

新历史区独立于原 Run 预览确认代次，避免自身合并 publication 变化抹掉实际提交回执。owner／身份／路径／窗口可见性改变使旧本页候选与确认只读，保存输入与已核对回执不清空；关闭不自动取消后台文件任务，重开不自动加载或合并。外来新 job 退旧资格，本页同步受理通知先 dirty 后处理，先安装实际新 job，再核归属。按钮按下冻结原操作，释放清本地字段后先核 plain lifetime；确认和放弃另精确绑定原确认对象，旧释放不能作用于新候选／新确认。

使用 UI/UX 技能的静态表单建议：沿原 QGC 主题／模态窗口／滚动与焦点可见性，独立可见路径标签、就地错误、明确等待状态、覆盖确认、二次合并确认和可换行长 ID。只有新覆盖确认使用本文件已导入的 Controls2 `CheckBox`，以提供真实 pressed/canceled 信号并沿用 QGC 文本、主题和已有勾选 SVG；不升级实际继承 Controls1 的全局 `QGCCheckBox` 或旧 Run 控件。没有运行技能搜索、UI 预览或渲染脚本。

本次仅源码修改、直接读回和有限静态合同审阅；未执行文件 IO、解析器／业务／QML／JavaScript、编译、测试、仿真、硬件或刷机。工程师仍须覆盖保存失败／commit 不确定／回读失败、坏尾整拒、空与上限集合、同 ID 同内容重复与异内容冲突、集合变化 CAS 整拒、后台取消和提交后取消、线程结束／集合变化／界面通知的先后、换源／换身份／隐藏、按下释放 ABA、离线候选分页与导入 pending 隔离。此闭环不等于跨站授权交接、任务继续运行或飞机控制权转移。

## 范围

`FleetHandoffPanel.qml` 位于原 CommandCenter 交接位置，使用同一 Guided Controller 的窄 facade；不创建第二个 Manager。面板只发起本实例的人际审计交接，不传递飞行控制权、不恢复批准、不改变原任务或监视，不上传任务、不启动飞机。新增本地历史文件保存／候选／追加 UI，不含网络投递或跨站接管。

## 操作顺序

1. 在现有运行观察入口选择真实 Run。交接页显示该 Run，不再允许手填任务 ID。
2. 输入接收账户的准确 Windows SID 和可选原因。SID 语法检查不证明账户存在、在线或有接收权限；发起人只取当前真实身份投影。
3. 明确冻结简报，核对来源、接收人、原归档版本及逐机历史观察。简报生成后不会随遥测刷新；MODE 入队计数、终点 / 停留 / 落地候选均不是执行成功率或任务完成证明。
4. 勾选本次已阅确认，再创建固定五分钟的本地待收记录。再次冻结、修改输入、身份会话变化、切换来源、隐藏 / 关闭窗口都会废弃旧本地确认，不自动重试。
5. 在本地收件箱显式刷新、打开具体详情、填写可选备注并另行勾选确认。只有接收 SID 可以接受 / 拒绝，只有发起 SID 可以撤销；后台仍复核权限、期限、版本及简报摘要。

## 来源与结果绑定

- 冻结使用 `fleetObservationSourceToken` 与实际所选 Run ID，真正取源和首尾核对由 Root / native 负责，不从报告 JSON 或界面文本构造 Run。
- 简报使用原生 17 字段值，包含原始运行身份、原绑定 7 / 9 字段、13 字段摘要、最多 32 个 14 字段逐机摘要和明确只读声明。未知协议序号保持 `null`，不补零。
- 每次按钮按下冻结 manager、controller、身份 token、操作者、来源 / Run、收件 SID、原因以及预览 token；详情操作另绑定 handoffId、recordRevision、briefHash 与备注。`identityToken` 能使同 SID 的新会话废弃旧确认。
- 操作回执分开显示 `committed` 与 `current`。`committed=null` 或异常不推断“没有提交”；来源变化不抹除已经发生的本地动作。不会自动重试。
- 本地确认序号只在 JavaScript 安全整数范围内递增；耗尽即关闭操作门禁，不回绕复用旧意图。收件 SID 使用与 Root 相同的 `S-1-…` 格式校验。
- 取消待发送预览保留输入；它不删除已经创建的交接记录，更不取消飞机动作。

## 本地收件箱

- 四种筛选：本人待收、本人待发、本人相关历史（含只读导入）、全部本地记录。全部本地记录可无飞行账户离线查阅；没有账户时不能处理请求。
- 每页固定四条，后台快照按最新插入在前。仅当前页建立 delegate；逐机简报按需展开，最多 32 架。
- 只在明确刷新、翻页、打开详情时查询本地 facade；`publication` 变化只标记旧页 / 旧详情并撤销确认，不轮询或自动刷新。
- 导入记录始终只读。旧无简报记录明确显示为旧审计记录，不包装成真实 Run；只有原生允许的本实例待处理旧记录可按空 briefHash 确认。
- 终态、已变旧、导入或非对应操作者的记录不可处理；摘要与错误采用纯文本并允许长 ID 折行。

## 布局与工程检查

原 CommandCenter 固定布局中只保留一个打开按钮，正文置于同一组件的模态 Popup，阻止背景飞行控件误操作，不挤压后续控件。窗口宽高受当前应用窗口约束，正文采用随内容收缩、最大约 18 字体行的 Flickable；明确关闭按钮固定在滚动区之外，也可用 Escape 关闭。关闭保留输入和提交回执、废弃本地确认，不执行后台取消。

页面控件保持 QGC palette、可见键盘焦点、明确禁用状态和最小 44 像素高度；短复选框文案配合完整可访问名称。翻页后通过内容高度变化保持已聚焦分页按钮可见；没有新增飞行按钮。

本轮只做源码与静态契约核对，未运行技能脚本、QML、编译或测试。工程师仍需验证窄屏 / 字体缩放 / 深浅主题、键盘滚动、换身份与隐藏时确认失效、五分钟过期、源切换及同步通知下的提交回执。该说明不是运行验证或飞行放行证明。

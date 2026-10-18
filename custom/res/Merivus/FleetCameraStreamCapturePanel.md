# 人工单相机单流读取与控制审阅面板

## 509 当前：独立控制审阅、双账号批准与原生状态回读

本批仅在现有 Panel.qml、GuidedActionsController.qml 和本合同三路径增加可见控制链；既有 Overlay／Planner 宿主、qrc／qmldir 与 READ 接口保持，不增加新的页面资源。打开页面、原五秒策略刷新和读取成功都不会自动申请或执行控制。

### 显式产品流程

1. 本页按原独立 `fleet.video.stream.capture` READ 完成 Info→Status，取得实际返回并匹配原 ID／generation／scope／目标的读取回执。共享快照、历史资料、文件或自报 Map 不能替代它。
2. 在独立控制区明确选择 STOP 或 START，默认未选择，不按 RUNNING 位自动选择。第一次默认未勾选的确认只创建控制审阅，尚不发送启停命令。
3. 原生从实际 Capture 一次取得 opaque Basis：初取两报告各自新鲜且本地匹配，原 RUNNING 报告必须与目标相反。同目标、过期、来源变化或已烧毁基线均拒绝；旧读取不能反复试另一个动作。
4. 面板并列展示本页原审阅的目标、原报告 RUNNING／not-running（仅报告、非物理证明）、拟 START／STOP、状态和最初期限。第二个独立默认未勾选的确认，才调用不同账号批准并执行原审阅。仅本页实际成功回执可拥有该 review；其它面板的 review／共享 current 不成为执行或取消资格。
5. 后续定时推进一次目标启停命令、一次 Status 请求和实际报告消费。完成只称“回读报告与目标一致”，不认证物理送流／停流或无线因果。STOP 可能使操作员失去视频，包括飞行中的可视性；START 也不是安全恢复认证。

控制 action 为 `fleet.video.stream.control`，独立二审 purpose 为 `fleet.video.stream.control.approve`。沿实际 Session／Role 和私有 Swarm 服务，不借 READ、RTT、interval configure、起飞、上传或其它批准凭据。它不套 RTT 地面门，不操作飞行模式；真实原生来源、协议、P0 与共享通道围栏仍可拒绝请求。

### Root 和 Panel 的真实归属

Root 新增独立 control owner、snapshot、request／approve／cancel facade 及 `_fleetCameraStreamControlSourcesCurrent(actualControl, actualSession, actualRoles, actualRegistry, actualSwarm, scopeToken)` 六参数原生围栏，接入 lifecycle stale／原 scope 取消／身份请求退休三个既有入口。

`current.captureId` 和面板回执选择器是实际 **Service Capture requestId**；Basis20 的 `captureRequestId` 是独立 **native Capture Run ID**，两者真实生产者不同，不要求相等或互换。非空 review 必须带完整 Basis20；非空 Run34 必须属于该 Basis 的 `basisId / captureRequestId / originalRunning / targetRunning / sourceEpoch / transportEpoch / versionEpoch`，不能借另一个 native 基线。Panel 只持 Service Capture ID 作实际私有服务选择，不从 Basis 的公开值恢复 handoff。

`requestFleetCameraStreamControl(sourceToken, captureId, targetRunning, confirmed, cancellationState)` 只接 actual Bool 目标。三目标 ID 从实际 Capture/Basis 取得，不从调用者报告恢复。原意图冻结 actual 服务、原 scope、primary identity 五字段、Role policy 九字段、capture ID 和目标 Bool；入队后还绑定真实 reviewId、generation、bindingHash 和目标三元组。

`approveFleetCameraStreamControl(reviewId, bindingHash, confirmed)` 仅接原 issued 审阅。Service 只建立原生 ready Run，Root 收到真实执行接纳并设置本原意图 executionAccepted 后，后续100ms turn 才可发送。源回调不能借准备 ID 或临时共享 current 建立新权限。

Panel 使用独立 controlEpoch／held／pending、两次确认各自代次和按下快照。创建控制审阅前仅退休旧 control，不调用会删除原 READ 回执的共享 retire；正常 Basis 消费或本次 service generation 变化不误当 scope 变化。输入／目标状态改变、确认 ABA、关闭／隐藏／禁用、controller 或来源变化，会退休原按压和本页后续意图；晚返回只精确清理旧返回 ID。

同引用 cancellationState 只有撤销作用，false 不能恢复批准。确认通知期间先持 plain ticket，自己的清除动作只允许精确预期代次；其它确认变化立即撤销 pending。busy=false 通知期间仍持原 ticket，随后同引用清理。退休在通知前撤旧身份／epoch，通知后再核原对象、代次和原空状态，不能抹掉回调中新建的请求。

原 Session current 仍按其真实 identitySnapshot 语义生成 sourceToken，不借审批提示升级 authenticated。`_syncFleetOperatorIdentity` 只对本实际 approve 栈、原 primary5／Role9、原 scope／reviewId／bindingHash 和服务 approving 状态保留原 Role 显示，避免正常提示造成自取消；不赋新角色、不消费其它 purpose 或产生批准，真正身份／用途核对仍由 Session 和 Service 完成。

### 闭合状态与事实

- Service current exact30，result exact5（`ok / requestAccepted / reviewId / reason / flightCommandReleased:false`）。外层 Root projection 不是 current30，另含稳定 scope／identity／policy token 和只读 capture 选择摘要。
- native Basis exact20，Run exact34，action exact8，statusQuery exact12，Info／Status 各 exact15。Root 只为这一闭合新形状允许34键 Map；原 READ 和其它复制上限不放宽。规范 uint64 保字符串，有限 REAL32 framerate 完整保留，不裁字段或 partial。
- Service 状态为 empty／preparing／review／approving／applying／reported-target-match／failed／stale／cancelled。Run 状态为 ready／waiting-action／waiting-status／reported-target-match／failed／cancelled，stage0／1／2。
- 分别展示目标启停命令已可能发送、命令 ACK、Status 请求已可能发送／ACK／实际报告与失败原因。未观察为未知，不补成功，不把“未能取得当前来源”解释成“没有发送”。Vehicle 同步销毁时仅接原 owner 的静态历史投影，不从死 raw 重建来源。
- 已完成 Run 后失源／RAW 暂不可当前／死 Vehicle 的历史允许保留 `state=reported-target-match`，但 `sourceCurrent=false / reportedTargetMatch=false`；仍保完整发送／ACK／报告事实。只有 reportedTargetMatch 为 true 时才要求目标匹配状态、stage2 和 sourceCurrent；状态文字本身不能反推当前完成证明。
- snapshotOnly／revalidationRequired 为 true，认证、无线因果、远端实际运行／停止、可用带宽及配置／执行／飞行／权限恢复旗标保持 false。这些是不能携带复用权限的结果字段，不表示已批准流程从未发送实际命令。

### 原期限与未完成边界

最初 Review 入口起30秒包含来源准备和人工审批，不续原期限；原生每 wire 最多4秒，512 accepted ACK 后报告窗口最多1秒且只缩短。初取 Basis 才要求原两样本3秒新鲜，审批期间不把旧样本一直强制3秒来制造正常自锁；自己实际动作写入前仍须原完整 Info／Status 依据不变。仅本 Run 精确动作实际写入后允许 Info 的 RUNNING 位状态改变，私有 configurationRevision 仍冻结其它配置语义并防 A→B→A，不能从新报告重建权限。

公开 Info 完整 revision 可因 RUNNING-only 更新而改变，不要求回读 Run 的 Info revision 等于旧 Basis revision；回读必须采用新270同次的真实 Info／Status pair，保原 receipt／年龄。若270先于ACK、随后Info更新，保旧 partial 事实，在原期限内等待新的真实270，不能把旧 Status 重绑新 Info 或刷新为新鲜。其它 flags／分辨率／framerate／bitrate／count／有效性及私有 name／URI 配置变化或配置 ABA 均退休原依据。这些代次仅是本地观察，不认证远端因果或实际物理效果。

实际命令为单流 2502／2503（param1 原 stream ID），然后新512请求270（param2 原 stream ID）。270 索引仍是有界兼容尝试，不支持／错流／超时不改协议、不退0／all／广播。无自动重试、反向回滚或恢复。任何下一次 START／停止／恢复都须新 capture、新 review 和新二审；终态旧批准不能 resume。

命令 ACK、Status ACK、本地序列和匹配报告不能排除同源迟到消息或认证物理启停、视频送达、容量下降、目录完整性。远端批量视频降级、码率／分辨率配置、自动策略 Apply 和自动恢复仍未完成。

UI 继续用现有 Qt／QGC 主题、PlainText 换行、单列滚动、明确取消／关闭、短确认标签、焦点可见和至少44px动作高度。UIUX 技能本轮完整静读，因限制只作静态 fallback；未运行检索脚本、QML／JS、渲染、键盘／辅助技术或窄窗验证。

本批精确范围和全部未执行工程场景由[清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[交接说明](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)记录。未编译、运行实现／解析器／helper／模型／认证／hash、测试、仿真、操作硬件、修改 PX4、刷机、放行、提交、更新 PR 或打包。原25目标、六类实质剩余及40%／60%粗估不因本局部控制链上调。下方508是保留的原 READ 合同。

## 508 当前源码合同

`FleetCameraStreamCapturePanel.qml` 通过实际 `GuidedActionsController` facade 发起一次人工 READ。入口分别放在 CommandCenterOverlay 和 FleetMissionPlannerPanel 的既有面板区；同一 QML 资源在 qrc／qmldir 各登记一次。打开、显示状态、改变输入、认证账号和原五秒策略刷新都不自动发相机请求。

独立 action 为 `fleet.video.stream.capture`，沿现有实际 Session／Role 权限源，无二审，不借 RTT、启动、上传、视频配置或其它 READ 凭据。这里只读相机元数据；不额外套 RTT 的地面／未起飞门，实际原生来源、传输与 P0 检查仍可拒绝飞行中或其它情况下的请求。显示出来的 Map 不建立身份、来源或权限。

### 1. 明确目标与人工确认

- 机号为规范十进制 1–255，相机 component 为 100–105，流 ID 为 1–255。不接受空白、前导零、0／all、广播或静默修正；无效输入旁显示具体范围说明。
- 默认文本 1／100／1 只提供可编辑起始值，不来自自动发现或自动选中，也不会自动提交。每次读取需要当前账号权限、有效目标和默认未勾选的独立确认。
- 复选框使用短标签“确认仅读取此目标”，完整说明另行 PlainText 换行；目标改变、来源／账号／Role scope 改变、隐藏、禁用、关闭或 controller 更换会撤销旧确认与本页后续读取意图。
- 仅请求 `MAV_CMD_REQUEST_MESSAGE` 512：先 269 Info，再 270 Status，param2 为所填单流 ID。269 的索引声明与 270 的兼容尝试不等价；270 不支持或没有实际匹配目标报告即失败，不退回 0／all、广播或其它目标。
- 不发 2504／2505，不启停、暂停、恢复或配置视频流，不自动重试。

### 2. Root 与实际请求归属

Root 的 `requestFleetCameraStreamCapture(sourceToken, vehicleId, componentId, streamId, confirmed, cancellationState)` 只从实际私有 Swarm 服务取得结果。它冻结原 Session／Episode scope、primary identity 五字段、Role policy 九字段、目标三元组和实际服务对象；原生回调 `_fleetCameraStreamCaptureSourcesCurrent` 以六个实际对象／scope 参数复核原意图，不从调用者报告恢复来源。

Panel 在按下时保存完整 sourceToken、controller、输入、view／dialog、原 epoch、确认代次和本页 held 身份。提交前逐通知尾核完整 token；正常本次请求使服务 generation／requestId 改变后，仅以原稳定 scope＋identity＋policy 核对，不能把正常接纳误判成来源更换。原 sourceToken 仍不会被新共享状态替换成授权。

只有实际返回 result 五键成功，并与实际 current 二十键的 requestId、generation、原 scope、目标三元组一致，Panel 才取得本页取消资格。共享状态、其它面板请求和其它 scope 保留快照可以只读展示，但不能被本页认领。不会选择／取消共享槽中的任意最新 ID。

当前服务正在准备、尚未返回 ID 时，本页可使同一引用的 plain cancellationState 失效；该 Map 只有撤销作用，不能授予／恢复权限。已返回 ID 则只取消原 held ID。晚返回结果若上下文已变，精确清理旧返回 ID，不取消回调中新建的请求。Root 在实际 lifecycle stale、原 scope 取消与身份请求退休三处接入相同独立退休流程。

### 3. 状态与数值合同

- 实际 service result 为 exact5：`ok / requestAccepted / requestId / reason / flightCommandReleased:false`。
- `workflow` 为原服务 current exact20：包括原 requestId／generation／scope、机号／component／流、状态／active／剩余期、完整 observation，以及 snapshot／协议和固定非许可字段。外层 Root projection 另带 sourceToken、busy、operatorIdentity、workflowScopeMatches／workflowHistorical 和接线说明，外层不冒充 exact20。
- observation 为空或完整原生 exact30；其 informationQuery／statusQuery 各 exact12，information／status 各 exact15。Root 逐字段闭合校验并复制全部字段，不裁剪 partial 事实。规范 uint64 计数保留十进制字符串；framerate 按有限 REAL32 数域保留，不强制成安全整数、字符串或四舍五入值。
- 服务状态为 empty／preparing／capturing／observed／failed／stale／cancelled。原生状态为 ready／waiting-info／waiting-status／complete／failed／cancelled。服务接纳不是已经发出请求，原生 complete 也不是实际运行视频认证。
- 页面分别展示两阶段“已可能发送”、ACK 结果、实际目标报告、状态与原因。ACK 未观察为未知，不把未知补成 ACCEPTED；已发、ACK、报告三者不互相替代。失败／取消后仍显示原生保留下来的部分事实，不声称撤回已发字节或完成回滚。
- Info／Status 的年龄与 fresh 独立，最大年龄参数为 3000ms；状态关联依赖原实际信息语义 revision。终态 current 是保留的读取快照，不是每次展示都重新采样的实时来源证明，也不保证两项此刻同时新鲜。

### 4. 原期限、重入与退出

最初服务入口起总期不超过 8 秒，包含身份／来源校验；Info、Status 各原生阶段不超过 4 秒，不能把两项都承诺为完整各 4 秒。服务只接纳后开启 100ms timer：首次原生 prepare 不发送，后续 turn 才推进实际请求。RAW／忙栈仅在原期限内等待，不延期、不重基或自动重试。

Panel 在公开 busy／receipt／message 通知前持有 plain ticket；同步回调改变 controller、scope、输入、确认、可见性或 view 后，后续提交／认领被拒。确认 false→true、路径式输入 A→B→A、按压后禁用再启用都不能使旧按压重新有效。finally 在 busy=false 通知期间仍持有原 ticket，再按同引用清理；取消和析构优先退休原意图，不恢复旧授权。

关闭仅请求取消本页后续工作，不能撤回已经写入传输层的字节。原生保留已可能发送／ACK／报告事实；若 Vehicle 在同步回调中销毁，则只能使用原 opaque Run 的历史投影，不能将缺对象解释成“未发送”。

### 5. 界面与已知边界

沿现有 QGC 控件、主题色、PlainText／WordWrap、可滚动 dialog、关闭／取消与键盘焦点方式，动作和输入最小高度为 44px。确认文本独立换行，输入错误近场解释；不修改共享 QGCCheckBox。界面未做渲染、键盘实测或窄窗运行验收。

本地收到目标相机的报告、ACCEPTED ACK、RUNNING 位和语义 revision 匹配均不证明请求造成该报告，不认证远端真正送流／停流、完整目录、可用带宽、包丢失、控制权或任何飞行／配置／恢复许可。270 索引兼容性仍需工程验证。完整视频降级、受控暂停／恢复与多车视频调度仍未完成。

本批总范围与工程矩阵见[精确清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)及[48 项全未执行场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。这里只做源码／文档和直接静态阅读；未编译、运行 QML／解析器／helper／业务／模型／认证／hash，未测试、仿真、操作硬件、修改 PX4、刷机、放行、提交、更新 PR 或打包。原 25 项目标及六类实质剩余保持，约 40%／60% 只是源码工作量粗估，不因本批路径数上调。

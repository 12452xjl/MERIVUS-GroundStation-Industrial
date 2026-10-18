# 备用独立出勤配置向导

## 543：兼容20键参数观察，不采用额外建议

Coordinator 当前原19键观察增加 `speedSuggestion` 为20键。本纯值编辑器及 Root `_fleetReserveDispatchProfileObservationKnown` 严格兼容旧19或新20：只在该字段实际存在时验证它的闭9键、固定4参数来源名、固定返程NAV沿巡航政策、实际Bool／String／Map，以及可用时四个有限数值和原profile范围；不可用时四速Map必须空。可用建议原因必须空，不可用原因须非空，均拒NUL。未知额外顶键、坏形状或字符串伪数值仍拒绝。

这只是防新增字段使原备用读取全部错拒，不新增速度采用通道。原备用流程仍只明确采用同一保留备用预算的四项速度，并保原budget、offerToken、fingerprint／hash重读和一次消费，不用该额外建议覆盖原备用策略、解除不匹配或取得任何权限。下方519所述原19形状为历史；其它旧流程不变。本次仅源码静读／编辑，未编译、运行、测试或渲染。

## 520：显式刷新与纯值表单的边界

刷新由实际父面板独立确认、调用既有只读服务；本纯值编辑器仍不持有Controller、账号、Vehicle或发包接口。读取缓存的`observe`与发送版本／参数只读请求的`refresh`是不同动作，后者仍限真实地面、未解锁和原角色／链路门禁，不能借`airborne-hold`规划意图放宽。

刷新开始前父面板退休原offer、profile采用和旧提案／审阅资格；同完整sourceIdentity下本页手填的动作／坐标及已明确采用的canonical启动策略仅作纯拟值保留，不保hash、速度采用、确认或批准。未采用的启动策略子表单须先明确采用或放弃；刷新、隐藏和关闭均不冒充采用。

原生刷新完成不代表519的12参数此刻全部在30秒内，也不提供参数原子快照或权限。必须再次人工读取本次原生offer、明确采用预算速度和完整新profile，再经预览重新取证及原审阅／审批。取消不会撤回已发送请求或抹掉记录。完整请求归属、状态和精确取消见[工作流合同](FleetReserveDispatchWorkflow.md)及[面板合同](FleetReserveDispatchPanel.md)。

本增量只静态接线，工程场景全部未执行；下方519数据／表单合同继续有效。

## 519 当前合同

本模块只编辑本页新执行配置的纯值，不持有 `Draft`、Controller、Vehicle、ParameterManager、认证或飞行接口。实际入口仍是[备用出勤面板](../../res/Merivus/FleetReserveDispatchPanel.qml)，新组件为[FleetReserveDispatchProfileEditor.qml](../../res/Merivus/FleetReserveDispatchProfileEditor.qml)。启动策略复用现有纯值 [FleetStartupPolicyEditor](../../res/Merivus/FleetStartupPolicyEditor.qml)，不伪造草稿对象，也不复制一套启动许可。

### 1. 实际来源与一次只读依据

操作员先冻结原备用方案、明确选择备用机，再单独确认并调用 Root `observeFleetReserveDispatchProfile(captureToken, reserveVehicleId, true)`。它只读当前实际原生参数缓存，不请求参数刷新、不写参数、不读 Home、不发送飞行命令。不能以同数字ID、共享latest、旧主机profile、文件或手填hash替代本次来源。

返回固定10字段：`ok/error/captureToken/reserveVehicleId/offerToken/policy/observation/historicalOnly/executionPermit/flightCommandReleased`。成功时 `historicalOnly=true`，两个许可恒false；`policy`为所选原备用描述符的完整8字段预算意图，`observation`为实际Coordinator原19字段快照。面板仅采用本次真实返回、匹配本页原capture和选择ID的offerToken。失败不回退旧依据。

`policy`为 `schemaVersion/replacementVehicleId/standbyMode/standbyDurationSeconds/standbyConsumptionFactor/cruiseSpeedMetersPerSecond/climbSpeedMetersPerSecond/descentSpeedMetersPerSecond`。原待命方式、时长及耗电系数只显示，不在新独立任务重演，也不宣称待命已发生或耗电模型真实。

成功原生19字段包括12参数诊断、managerEpoch、parameterProfileHash和observationFingerprint；本UI闭键检查成功source、固定false边界、12行排序唯一名称与各行15字段。采样年龄是读取时本地收包年龄，不会随显示续鲜。hash仅作原profile绑定，不是发送者、物理飞机、参数原子性或执行许可认证。

### 2. 结构化编辑，无暗填物理输入

操作员须按“明确采用本次备用预算速度”按钮后才采用速度。巡航与返航都等于原 `policy.cruiseSpeedMetersPerSecond`，爬升／下降分别取同次原policy两值；不是旧主机配置或当前参数值的静默继承。原预算若超出执行profile范围，保持错误并返回原方案重做，不偷偷取min、改预算或写参数。

新起飞动作须明确选 `takeoff` 或 `none`；结束动作须明确选 `hold/land/rtl`。需要坐标的动作必须手填完整 `geodetic/amsl` 纬经高（纬度±80°、经度±180°、AMSL海拔−1000至20000m），空白不补0，不以Home／原主机／路线点自动填充。`none`及`rtl`不生成坐标。这里的AMSL不是离地高。

时序须明确选择，独立出勤仅支持 `relative-dwell`。用户选择 `absolute-required`时明确阻断，不自动降级或宣称支持。固定适配器为 `px4-multirotor-mission-v1`；巡航／返航0.1–300m/s，垂直速度0.1–20m/s只是格式边界，不是安全推荐。

启动策略通过原共享结构化表单明确选择schema1–4并逐项填写，仍可保持缺省以保存规划，但受控启动原门禁继续拒绝缺策略。未采用的策略表单会阻止采用完整profile；可明确采用或放弃，不把关闭弹窗等同采用。可选整数空间保护须明确勾选，且要求完整启动策略，固定值 `integer-e7-amsl-mm-v1`，不生成默认容差。

完整新profile的 `verticalPolicy.requiredProfileHash`只取本次actual观察。当前垂直参数与原预算不匹配、水平速度超原参数上限等仍由真实审阅拒绝；面板显示原参数值便于解释，不私改预算／参数，也不以12参数offer代替后续完整29参数、固件、净空、动作、性能及其他来源门。

### 3. 本地采用、预览和重核

新组件只发 `inputEdited(expectedSession)` 和 `profileEdited(nextText, expectedSession, expectedOriginalText)`纯值信号。父面板必须核原offer对象、capture、ID、scope及原文本；实际编辑撤旧提案／确认，但不借新表单恢复旧批准。完整profile最多16384字符；生成JSON只读核对，不要求手填整个对象或hash。

组件以原session对象、form对象及非回绕表单serial冻结按下快照；任何编辑均撤旧确认。隐藏、禁用、换源、重新读取或换备用机不能复活原按压／确认。通知前持有独立plain票据，通知返回后核原生命及来源；销毁后的finally只清plain原票据，不依赖旧Qt对象继续动作。

只有本页实际采用且仍绑定当前offer的完整profile、没有未采用表单时才可明确预览。Root调用为 `previewFleetReserveDispatch(captureToken, requestExact3, offerToken)`；request仍只有 `reserveVehicleId/executionProfile/operatorConfirmed`，第三参必须原样传本次真实offerToken。Root重新读取actual19，重核参数hash、观察fingerprint及原capture/provider/scope，不能只比较内容hash而接受来源／参数ABA。合法preview为一次使用，后续原生／Planner失败也不恢复offer；Panel在实际调用返回后精确退休自己的offer及采用资格，保完整profileText只读核对，失败重试须重新明确读取。

正常本地流程是：本页编辑 → 父面板退休旧提案但保同一offer对象 → 策略表单明确采用 → 确认生成完整profile → 父文本采用 → 组件记录本次采用。自己的纯编辑退休不更换offer session，因此不会把正常采用误判换源；真正换源／隐藏会清依据，重新读取后才可采用新配置。独立sourceIdentity冻结captureToken、reserveId、sourceTaskId、planId、完整binding与部署scope，不依赖offer是否为null。同captureToken、同reserveId及完整sourceIdentity下的显式重读仅保本页操作员自己输入的动作／坐标／已采用启动策略纯拟值；每次null／重读均不保速度采用、确认、adopted、hash或原offer资格，须重新明确采用预算速度和完整profile。换capture、备用ID或其它sourceIdentity项清除这些拟值，不从旧profileText倒灌或旧主机profile加载。启动策略尚有未采用表单时，按钮与实际observe入口／调用前均禁止本页重新observe，须先继续编辑并明确采用或放弃。原策略编辑器的session退休会reset未采用子表单；收起／换源因此丢弃的子表单不算采用，只有已经发给父组件的canonical startup文本能作为本页纯拟值保留。完整native Planner/Compiler仍执行原规范化与预算绑定，UI检查不替代它们。

预览后继续原新任务审阅、明确采用、本地申请／批准及唯一共享部署入口。本模块没有上传、ARM、任务模式、StartPermit、自动接管、自动重试或恢复旧权限接口。

### 4. 界面与未验证边界

沿原QGC主题、单列布局、明确文本标签、PlainText换行、至少44px控制高度和键盘焦点。错误保留可核对文字，失败可聚焦说明，成功不抢焦点；起降点、策略及预算意图与参数实值分别展示。技能仅采用已直接阅读的静态指导，没有运行技能搜索、界面或预览。

本批未运行QML／JS／项目、解析器／helper／模型／认证／hash实现、编译、测试、仿真、硬件／PX4、刷机、飞行、提交、PR或打包。字体缩放、窄窗／横屏、主题对比、键盘／辅助技术、参数回调重入、来源ABA和完整工程操作场景仍待工程验证。原25项目标及六类实质剩余不因此完成。

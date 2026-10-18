# 固件工程目录界面合同

## 417：同一私有工作流的两个源码入口

[FleetFirmwareCatalogPanel.qml](../../res/Merivus/FleetFirmwareCatalogPanel.qml)已登记在 qrc/qmldir，接到指挥中心及任务规划面板；沿用原 Guided 私有 Coordinator，不新增 Catalog 或替代 Session。界面读取 `fleetFirmwareCatalogReload` 值投影，只调用登录、申请预览、精确收据审批和精确取消四类窄入口，不获得原生引用或任意文件路径权限。

预览前明确确认读取固定文件；之后分别核对声明和确认整批替换，再调用不同账号审批。默认焦点为关闭，不用 Enter 隐式提交。固定路径、原文件与规范哈希、增删改数量、每项旧新 engineeringReview 四字段、完整原目录／新规范 JSON 都保留；完整内容折叠后懒加载为只读 PlainText，不截断或执行文件内文本。

实际旧 source token、view、controller、epoch、调用 ticket 和原五字段 receipt 构成该界面自己的意图。模态期间关闭、隐藏、来源变化或销毁只取消该原意图；另一个入口的审阅不能被接管。Root 在安装 active 意图后再次核原显示 token，并以 `contextMatches` 在原生捕获身份后核原身份五字段及政策九字段，不能让中途换账号复用旧显示确认。

原生 30 秒期限不从界面重置；本地显示倒计时保守计算，显示时钟回退即废弃确认。超时不自动重试或升级权限。会话无法认证、文件缺失或实际来源不明均显示未知／拒绝，而非假装目录为空或具备许可。

提交事实独立于当前确认：即使目录变化使 source token 过期、原调用 `ok=false` 或后置审计失败，也保留已返回的 `reloadCommitted` 和完整 `commitReceipt`。这不证明该版本仍当前、不证明无人机停飞，也不证明目录或源码可信。

Root `uiWiringPresent=true` 仅为源码接线声明，`runtimeValidated=false` 与原生 `reviewUiIntegrated=false` 保持。UI/UX 技能仅用于默认取消、确认分步、焦点／键盘、错误反馈与原主题沿用；未运行技能搜索脚本、渲染、界面测试、认证或应用。工程师仍需验证两入口、资源／类型、缩放主题、焦点滚动、实际模态与返回事实。

详见[原生重载合同](FleetFirmwareCatalogReload.md)、[修改清单](../../../MODIFIED_FILES.md)及[工程师交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。此模块不是完整固件配置管理、签名认证或原 25 项产品全部完成。

# 固件工程声明目录的受控重载

## 454：目录2的高程关系沿用原预览与整批提交

档案外层schema1和固定`firmware-adapters-v1.json`路径不改；[Catalog](FleetFirmwareAdapterCatalog.md)中的descriptor1原8字段保持，显式descriptor2增加完整datumRelations。原规范预览、输入／规范hash、整批替换和目录epoch／revision继续包含整份关系，不从旧descriptor1补默认关系，不因文件恢复赋予新批准或真实高程认证。

工程人员需提供原scene版本／来源文档hash／垂直datum、完整JSON构型指纹、实际GLOBAL_ORIGIN参考hash及关系审阅引用。只支持显式同datum和offsetMm=0的工程声明；非零偏移需要真正坐标变换，不能在文件里填数后仍按零计算。实际[Datum来源](FleetMissionClearanceDatum.md)还核当前原生VERSION、PRIMARY及原点／构型来源，缺关系不能仅凭重载成功通过。

此增量没有改变构造时自动载入仍为声明、原native API范围或企业／身份／签名边界；原Reload两账号只批准本地精确文件替换，不认证工程资料实测真值或授予飞行许可。仍未编译、运行、测试或操作设备。以下417流程继续适用。

## 417：固定文件预览与整批替换，源码接线未运行验收

[原生工作流](FleetFirmwareCatalogReload.h)由唯一实际 [Coordinator](FleetMissionReviewCoordinator.md)私有构造并拥有，与其实际 Catalog 配对。文件来源固定为 `QStandardPaths::AppDataLocation/fleet-os/firmware-adapters-v1.json`。操作员不能传入路径或 JSON；工作流不创建目录、不写配置文件、不刷固件、不发飞行指令。

本入口是在运行中明确重载已由工程人员放置的本地文件。原 Coordinator 构造时自动读取此文件的历史行为没有改为双账号审批；其他原生目录维护 API 也没有统一纳入此工作流。不能因此声称全配置生命周期、企业权限或固件签名体系已经完成。

### 操作与绑定

1. 维修员或管理员以现有 Windows 本地账号登录，明确申请固定文件预览。实际 Session SID/角色须与实际 RolePolicy 的 `firmware.catalog.reload` 快照相符；任意可编辑角色值不等于身份认证。
2. 读取不超过 1 MiB 的普通单硬链接文件，拒绝符号链接／Windows reparse point、非法 UTF-8、非固定目录与异常 EOF；冻结文件及父目录／AppData 目录身份、完整字节 SHA-256。原生平台不支持所需安全读取时拒绝，不回退弱读取。
3. Catalog 的 `previewImport` 只校验和规范化，不修改实际目录。完整旧 `previousEntries` 和新 `descriptors`、原始 `engineeringReview`、增删改及未变 ID、规范档案和哈希进入预览。旧审阅人只是文件声明，不被替换成当前操作员，也不认证其身份。
4. 实际对象、原 Catalog epoch/revision/fingerprint、原 Session 身份、角色政策、Root scope token、文件身份及预览共同进入绑定。30 秒期限从申请开始计时，账号提示不续期。Root 保留实际引用，界面仅持五字段值收据。
5. 明确核对并确认**整批替换**，由不同 SID 的安全员、总指挥或管理员审批精确用途 `firmware.catalog.reload.approve`。一次消费不产生上传／启动租约；消费失败也不证明旧凭据没有被烧掉，不自动重试。
6. 所有账号／Root 回调完成后，再读同一固定文件并比较目录身份、文件身份、完整内容与 SHA-256。其后只做纯内存原来源／政策／已消费身份／原期限核对；Catalog 的三参数导入在实际整批提交前再调用最终围栏并核原 epoch/revision。

空的合法声明列表会清空目录；较低档案 revision 仍可能恢复旧声明；相同内容也推进实际目录 epoch/revision。以上都显示为整批替换，不隐瞒为无副作用刷新。没有证明抵抗同进程恶意代码、所有文件系统 TOCTOU、恶意管理员或密码学篡改。

### 接口与事实

原生入口为 `requestPreview(actualSession, actualRoles, actualRoot, scopeToken, confirmed)`、`approveAndReload(reviewId, replacementConfirmed)` 和 `cancel(reviewId)`。只读 `contextMatches` 核实际 Session/Role 及原身份五字段／政策九字段，不能认证、续期或授权。原 Root 回调 `_fleetFirmwareCatalogReloadSourcesCurrent` 有六个 QVariant 参数，依次为实际 Reload、Session、Role、Catalog、Coordinator、原 token。

`current` 固定 28 键：schema/工作流与审阅标识、generation/state/reason/token/binding、剩余期限和忙／提示状态、固定路径／文件观察／目录身份／完整预览、申请人／审批人、人工确认／消费／提交事实／提交回执、实际授权 owner 保留状态、source，以及五个保持 false 的 UI 验收／固件身份／签名／执行／发令标志。原生 result 固定 13 键；Root 可另附五字段 receipt、审计结果与外层调用状态。

五字段收据为 `workflowEpoch`、`reviewId`、`reviewGeneration`、`bindingHash`、`scopeToken`。无 raw QObject、任意后端选择或通用 ID 查询。两处界面不能接管或取消彼此未持有的收据。关闭／失源只退役原审阅，不停止 Run、释放飞行占用或撤销已经提交的目录。

Root 对调用阶段另行限制：无 active 时只核已发出的 review；preparing 必须属于原 request，approving 必须属于原 approve 及真实当前 controller-call。进入审批阶段后、弹 OS 窗口之前再核一次，不借旧 issued 绕过调用。申请返回时还在 active 清理及 endControllerCall 通知之后复核原收据，后置取消不能显示为可审批的成功预览。

`reloadCommitted=true` 与 `commitReceipt.mutationCommitted=true` 是已发生的本地替换事实。Catalog.changed 会使旧任务审阅失效，也可能使外层调用失效；`ok=false`、`stillCurrent=false`、取消或审计写入未确认不能抹掉该事实。Root 的审计追加不证明持久化；原生忙标志退出后仅排队通知，不能据此宣称界面线程或模态行为已经验证。

### 交接边界

[界面合同](FleetFirmwareCatalogPanel.md)和[工程师清单](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)须随累计源码交接。需由工程师验证 Qt 类型／moc／资源、Windows 实际账号与不同 SID、两入口、modal 销毁、超时、文件变化、完整替换、审阅失效和提交后重入。此阶段只做源码及限定静态审阅，未编译、测试、运行 UI／认证／读取实际配置 helper、操作设备、更新 PR 或打包。完整 25 项及六类剩余工作继续。

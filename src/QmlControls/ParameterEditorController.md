# ParameterEditorController — 完整预览与原生导入接纳

本模块接实际 `ParameterEditor.qml → buildDiffFromFile → ParameterDiffDialog → sendDiff → ParameterManager::beginParameterImport` 路径。533建立完整预览，534替换实际接纳和发送事务；旧 `readParametersFromStream` 没有被此界面调用。普通单项编辑、旧流导入与本原生事务分开，不增加参数协议或飞行权限。

## 534 当前：接纳原生导入事务与实际状态区（源码静态收口）

保留533建立的完整文件、独立metadata格式化、原preview token和所有选中项基线检查；应用阶段由本节替代。Controller冻结原PRIMARY及weak owner，只有私有typed选中行可以调用原生beginParameterImport；admissionFence在同步接纳期间核原source、Fact／metadata／值及写门。PM接纳后自行持有来源与值，不靠Controller或已关闭预览续权，不再通过forceSetRawValue或通用worker乐观写live Fact。

sendDiff=true仅表示原生事务已接纳（或明确零选择无事务），不是已全部发送或远端确认。接纳拒绝尚未消费预览，可调整选择；接纳成功一次消费，正常clearDiff只清预览。PM实际状态通过原Controller绑定的PM转发，不随activeVehicle替换为另一架机。参数页显示原runId、writer入口行数、pending、匹配报告、超时和被本地修改替代计数，并明确匹配不证明因果ACK。

停止按钮另有确认，打开确认时冻结原Controller与原runId；确认后只取消该run的剩余发送及重试，不清后来run或普通参数读写，也不回滚已写。状态区可滚动且不盖住原分类／参数列表；按钮限定父宽、PlainText换行、StrongFocus及主题焦点框，键盘焦点进入时滚入可见范围。文件关闭／预览构建／对话框创建／提交与停止回调后先核plain lifetime，原页面销毁后不续读QObject或借后来token。UI/UX技能只静态指导这些交互，未执行技能搜索、界面预览或测试。

源码实现及限定静态交叉审阅完成。原生源、批次预算、每行重试／匹配语义见[事务](../FactSystem/ParameterImportTransaction.md)，最后发送校验见[传输桥](../Vehicle/ParameterImportTransport.md)。本文件按534更新当前语义，533逐项setter／旧sender缺口见历史审计，不再用于本界面。


## 接口和所有权

- `bool buildDiffFromFile(const QString& filename)`：先冻结实际来源，再退休旧预览；完整文件和所有候选通过后，一次发布差异模型。失败不发布有效前缀。
- `QString diffToken` / `diffTokenChanged()`：每次完整发布新 UUID，不能从旧对话框或相同内容重建资格。
- `QString diffError` / `diffErrorChanged()`：可见固定诊断；解码错误带 codec 行号，不回显原始文件内容。原生接纳被拒时显示原因，接纳后的发送事实另从parameterImportState读取。
- `bool sendDiff(const QString& expectedToken)`：必须准确本页token，不能缺省或空参应用。整批选中行预检；接纳成功消费原预览，接纳拒绝可调整选择后重试。零选择不建run。
- `void clearDiff(const QString& expectedToken)`：只退休准确 token；旧对话框析构不能清后来新预览。内部换文件走私有清退，不通过无 token 的公开接口。

公开 `ParameterEditorDiff` 仅保原展示字段和可选 `load`。原生私有记录保存 canonical 行、原实际 Fact/metadata、值基线、来源及行对象。应用先冻结全部 `load` 选择，绝不从公开展示 payload 反构目标、类型或值。通知中可准确退休本次 token，但不能重入另一次 build/apply；不会回滚通知中他人新状态。

实际对话框创建时复制 token，不绑定后来预览；过时页面不再显示后来模型，确认、单项选择和全选都核原 token，全选每次通知后再核。关闭只清准确原预览，仍保 Cancel。载入失败明确显示 `diffError`，而不是只返回 false 后静默结束。UI/UX 技能仅以静态的错误反馈、状态清晰和恢复路径指导现有 Qt 控件；没有执行技能脚本、QML、布局预览或交互测试。

## 整份文件和预览

普通非符号链接文件，以 64 KiB 分块读取，最多 4 MiB，核实际 EOF、错误、大小、canonical 路径、修改时间及关闭；这是有限的路径/字节一致性检查，不是文件对象认证或防任意本地篡改。复用 `OfflineParameterFile::decode`：最多 16384 行记录、65536 行、每正文行 4096 字符、严格五列、统一 0..255 系统号声明、1..255 组件、合法名称/类型/完整数值、重复拒绝；原带 NUL 的历史注释按 codec 既有规则跳过。文件来源系统号仅值声明，不自动改当前 Vehicle.id；异机文件可显示差异但不可应用。

文件十种数型保留精确 typed 值（含浮点负零）；已知 Fact 的文件声明类型必须与实际 Fact.type 相等，并通过其真实 metadata 的 raw 范围转换/核值，不能悄悄按当前类型重新解释文件。只读或完全相同值不进入可选差异。缺失参数保留原用户可选导入路径，typed 值仍来自完整 codec，不补伪 metadata 或当前观测。

预览格式化使用临时 Fact 和真正 `FactMetaData` 深副本，读侧 `_containerSetRawValue` 仅装临时对象；不触实际 Fact 值，不改实际 metadata reboot flag，不共享实际 metadata。包括可能新增 unknown enum 项的格式化也只在副本执行。临时对象在本调用精确清理。

## 来源与基线

冻结原实际 Vehicle/ParameterManager、当前机选择、实际 ComponentInformationManager、PRIMARY 弱 owner/对象、原 plugin 地址、系统/默认组件/固件/机型、版本来源 epoch、实际七个版本数值及 PM 观察 epoch。来源判定不借 PX4-only cache 快载上下文，因此不把 APM 普通编辑器改成 PX4 专用。PRIMARY/当前选择/两个版本信号建立监听后的变化用 sticky 原子位退休资格；第一条 connectNotify 中另一尚未监听信号的隐蔽 ABA 不由本地监听证明，不声称全局原生 setter 代次或跨线程支持。

已知 Fact 所在组件实际 provider 的 metadataRevision 及每行 Fact-owned metadata 身份、线程、名称/类型、父对象、值基线、范围/只读/枚举/单位/翻译器等相关语义均在可见边界核对；原本缺失的参数不伪造 metadata/provider 证据。刷新/换源改变 PM epoch 会使旧预览失效；正常单行 `_invalidateParameterObservation` 只退休 cache attempt/该行观察，不推进全表 epoch，因此本次正常多行写入不会自锁，也不会靠重捕新 epoch 延续旧预览。Fact 删除不能被降级成“原本缺失参数”。

## 应用与事实边界

应用前全选择必须都仍适用且当前 `writeAllowed`：仍选原机、初始化完成、连接未丢、未解锁且地面。现 PM 的 `PARAM_SET` writer 仅支持 MAV 数型 1..6、9；选中 7/8/10（Uint64/Int64/Double）整批在零尝试时拒绝，不能落入旧 writer 的 Int32 兜底。未选中的这三型可留在预览供人工查看。

选中值仅来自私有typed候选，不从公开展示字段反构。同步admissionFence复核原来源、所有选中基线及写门；原PM完整接纳后固定原值、原PRIMARY、GCS身份和channel，独立等待／有界重试。已知和缺失参数均不经通用setter乐观修改现场值，不使用读侧容器setter假装写入。真实PARAM_VALUE仍按正常路径更新Fact。

非空选择的`sendDiff=true`只保原生已接纳事实；之后通知内取消／销毁／换源不能伪造“未接纳”，实际退役和writer计数由独立PM状态表达。无选中项在清错误通知后仍核原预览及写门，完成零次调用但不制造事务或远端确认。

PM经[专用发送桥](../Vehicle/ParameterImportTransport.md)在writer前核原来源与固定payload，重试不读后来Fact值；最早RAW票据仅作同源同类型同值报告匹配。writer入口、网络送达、机载接纳、持久保存和因果ACK不是同一事实。取消只停剩余发送，不回滚已入writer字节；待事务终结后由工程师明确回读核实。533旧setter尝试语义仅保留于历史审计，不再是当前实现。

## 静态交接（全部未执行）

需工程师后续验证：坏尾/超预算/重复行/混源无前缀；未知类型与数值越界；实际类型冲突；已知只读；缺失参数选择；Float ±0；选中三种不支持 wire 类型全批拒绝、取消选择后保可用行；旧对话框 token 析构不清新预览；模型 reset/metadata/enum/值通知内销毁和准确 clear；active/PRIMARY/版本/provider 变化与已监听 ABA；中途改变下行基线/解锁/断连后停止；原生接纳、部分writer入口事实、固定重试、准确取消和真实回读。只做源码静读与编辑，未运行解析器、业务 IO、QML、编译或测试，不是运行验收证据。

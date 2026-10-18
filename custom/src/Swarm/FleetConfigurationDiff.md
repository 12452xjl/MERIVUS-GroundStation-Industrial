# 本地构型快照与有类型字段比较

## 420：比较历史进入任务基线校验

[新基线合同](FleetConfigurationBaselineEvidence.md)复用本模块原有类型指纹与 exactEqual，Registry 在历史变更时缓存当前、基线和原批准5键的指纹及匹配结果；getter 不重跑完整差异。显示截断不等于原配置未知，也不替换原类型判定；当前完整性丢失则明确无正向资格。

原比较 report13/row6/value9、Review preview2/18 及 Panel 不改合同。A→未知→A 可在新审阅匹配旧参考基线，但观察代次阻止旧批准复活；没有新增持久化或原生构型批准界面。统一任务消费者不再仅看旧 JSON hash 标签，详见新合同。未编译、测试或运行。

## 419 范围

[FleetConfigurationDiff](FleetConfigurationDiff.h)由实际[FleetRegistry](FleetRegistry.h)消费；结果进入[FleetCapabilityReview](FleetCapabilityReview.md)的冻结审批依据与[现有面板](FleetCapabilityReviewPanel.md)。它比较地面站已收到并保留的值，不发现没有上报的硬件变化，不认证实物身份、改装原因或性能。

原配置哈希和passport schema保留。新增有类型指纹只用于本进程比较，不能当作签名、跨Qt版本格式或跨重启认证。完整原生快照不通过此DTO导出。

## 观察与基准

- 正常输入提供非空configuration（含已有metadata别名）时，按输入约定视作一次完整上报，保留最近两份不同的有类型值。这不自动核验每种机型的必需字段或实物齐全。重复同一份和普通健康/心跳不滚动历史；A→B→A比较B→A。
- hash-only、明确清除或不完整配置证据不能用合并后保留的旧map冒充新完整观察；当前完整值未知时比较为unknown。
- 只有本机明确批准构型且确有当前完整观察，才保存对应的完整批准快照。能力approve/revoke不调用构型批准；旧哈希、导入的local_approval标签不能补造原批准快照。原生approveCurrentConfiguration收紧：hash-only后即使旧map仍被合并保留，也拒绝新批准且不改旧记录；重新收到完整值后仍需明确批准。
- 优先与本机保留的有效完整批准快照比较，否则与前次不同完整观察比较；两者都缺失则unknown。前次观察不是批准基线。
- 删除重加、FleetRegistry::clear()和导入触及条目退役旧观察身份及完整基线；configurationClear=true仅使当前完整观察未知，保留历史作诊断。导入触及范围包括descriptor和仅出现于审批记录中的ID；后者也退役旧能力记录/批准，不能仅换观察epoch。增量导入未触及条目保留；JSON解码后的新观察不是原Qt类型历史恢复。

有类型值变化或currentFullKnown切换会使旧能力批准失效；旧identity已经推进时不重复增加revision。重新获得相同完整值也不会恢复失效批准。比较缓存仅随实际观察/基线变化重建，getter不重复执行字段遍历；最终能力变更围栏同时复核原观察身份。

## 值合同

Registry原生只读configurationComparison(id)返回schema1/16键：schemaVersion、systemId、registryContext、observationEpoch、comparisonRevision、basis、localBaselineKnown、previousObservationKnown、currentFullKnown、baselineConfigurationFingerprint、previousConfigurationFingerprint、currentConfigurationFingerprint、comparison、hardwareIdentityAuthenticated、performanceCertified、flightCommandReleased。末三项恒false。无有效条目、错误线程或耗尽状态不提供可用比较。

basis为local-approved-baseline、previous-different-observed或unknown；它描述比较前值来源，不单独证明当前值可用。独立观察epoch/revision与原context6一起冻结，避免仅凭同一地址或旧JSON哈希恢复原审阅。comparisonRevision以十进制字符串传输，不经JS浮点数。

嵌套comparison恰13键：schemaVersion、comparisonState、comparisonKnown、complete、truncated、reason、beforeKnown、afterKnown、beforeTypedFingerprint、afterTypedFingerprint、changeCount、reportedChangeCount、changes。状态为unknown/same/different/truncated/invalid。未知的零行不是相同；截断不是完整。

每行恰6键：path、kind、beforePresent、afterPresent、beforeValue、afterValue。path为JSON Pointer，波浪号和斜线分别编码成~0/~1；界面额外转义控制和双向格式字符，仅改变显示。kind为added/removed/type-changed/null-changed/value-changed。Map/Hash按键排序；数组按下标，不推测移动；整个子树的新增、删除或换类型计一行。

每个安全展示值恰9键：type、valid、null、representation、display、displayTruncated、byteCount、elementCount、typedFingerprint。缺失通过present=false明确表示，不与invalid、typed-null、空字符串或空容器混同。64位整数用十进制文本，字节用十六进制；容器摘要省略成员时明确标截断。

## 类型与预算

每份输入独立限制深度8、2048节点和128 KiB标量字节，沿Registry既有域（QStringList元素计节点但不增加元素深度）。非空未知类型拒绝；历史域接受的未知null类型只比较本机metatype ID/valid/null，不调用自定义比较器或转换。

指纹编码保留类型、valid/null、长度、UTF-16原代码单元及字节；不依赖Qt JSON。Double的-0/+0按相等处理；QDateTime在原类型/null/有效性一致后按UTC毫秒比较，不把同一时刻的显示时区差异当构型变化。时区等仅作显示说明。

最多128行，每个display最多512 UTF-16单元；报告路径和前后display合计最多64 Ki单元。实际有界遍历仍计算完整changeCount；超预算不切断路径成为另一个字段，而是省略该行并标truncated。任何值截断、非空容器摘要或行省略都会令complete=false，不能报告same。

## 接线与验证边界

能力预览单独升级为schema2/18键，新增configurationComparison。current30、result16、Root收据5、能力context6/approval14不扩字段。原生冻结原map和比较依据，复核独立epoch/revision、前后有类型指纹及来源；显示JSON不替代此核对。未知比较本身不禁止本地工程能力声明，原有效context、账号、期限及最终任务门禁仍必须满足。

私有观察历史只在本进程保留，没有新增持久化、全量构型版本库或构型基线批准UI。不能从passport导入还原以前的原生完整快照。仅源码与限定静态阅读；没有编译、运行helper/UI、测试或设备验证。

明确未收口：旧configurationState及requireConfigurationBaseline消费者仍依据旧哈希状态；先前已批准后收到同哈希的hash-only观察，可能仍保留baseline标签。本批不把这个标签当完整有类型批准证据，也不声称所有任务基线门禁已经升级。下一步需统一原生可信基线投影与Matcher/RoleAllocator/Compiler消费，不能只修改一个入口的标签。

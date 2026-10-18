# Fleet 任务空域原始输入门禁

## 范围与调用链

本批441只修改地面站源码，不修改PX4，不新增命令、自动任务执行或启动许可。原规划／审批／部署边界保持；原25目标仍须继续实现。

`GuidedActionsController.prepareFleetIntent()` → `FleetIntentTask`私有参数／预览 → `FleetMissionReviewCoordinator.reviewWithNativeEvidence()` → `FleetAirspaceConflictChecker.check()` → 原风险评估与报告 → 同源PlanStore归档 → 原人工审批。

这是一条输入保真和阻断链，不是完整4D／净空认证。各层职责见[Checker](FleetAirspaceConflictChecker.md)、[Coordinator](FleetMissionReviewCoordinator.md)、[Intent](FleetIntentTask.md)及[PlanStore](FleetMissionPlanStore.md)。

## 原值与缺字段

Root只在reviewTask真的没有自身noFlyZones键时传兼容空列表。显式false、0、空字符串、map或null保留原值；显式undefined变为无效null，不变为[]。原生第4个审阅参数及Checker第3参数均为QVariant，在完整检查前不调用toList。

共享纯检查恰好5字段：valid、state、reason、zoneCount、fingerprint。实际根List、最多1024项、16MiB／20层／1M节点及纯JSON预算通过后，才有完整compact JSON指纹；失败计数为null、指纹为空。合法[]与缺字段仍不等于加载了权威／完整／新鲜空域；几何合法性由原Checker另检。

## 双源一致性与风险传播

Coordinator在任何原生采集回调前复制原参数，并独立检查task实际存在的字段。task有字段时两份必须都有效且指纹相同。结果airspaceInputCheck为恰好7字段：

```text
valid, reason, argument:inspect5, taskProvided, task:inspect5|{},
sourceMatches, coverageCertified:false
```

caller的noFlyZonesSourceInvalid及reason会被移除，只按本次真实输入重算。坏类型／JSON／预算、坏task、有效列表不同或坏minimumSeparation均产生原生不完整报告：analysisComplete=false、feasible=false、requiresReview=true、reviewStatus=blocked-incomplete。固定code的blocking违规沿原RiskEvaluator／ReportBuilder进入当前结果，Coordinator明确needsReview=true。

没有用错误早退留下旧可批准报告：正常epoch替换lastReview及本次原生缓存，发送reviewChanged，原Root审阅代次和撤销链生效。不完整报告没有应用完整阈值，minimumSeparationMeters=null；没有默认5米上的成功前缀。正常省略间距的旧5米兼容行为不变，439独立名义线段入口仍要求显式正数。

## 冻结归档，不重读可变参数

本次原生结果提供恰好4字段airspaceInputSnapshot：available、provided、value、evidence。provided是task实际键存在性，evidence是原440四字段形状描述。仅有界纯JSON原值可输出；原List必须通过自身完整检查，不能以单项包装绕过1024区域上限。非List的坏scalar／map／null可经保守单项包装预算后保留，用于诊断。opaque、非有限及超限值available=false、value=null。

Root先规范化原生返回值，再验证snapshot／check精确字段及元数据一致。只有此冻结value和evidence写入新reviewEvidence；不会在原生回调结束后重新读取proposalParameters.noFlyZones。没有安全快照时，本次blocked审阅仍可见，但不调用recordReview生成新归档。Root缺键对应[]，不把直接原生API的独立非空参数重新标成缺省Intent数据。

有效来源在归档前、recordReview返回后均调用纯inspect入口检查当前原生Intent私有parameters：键存在性、完整指纹和计数须等于本次native check；并保留实际Intent／Coordinator实例、原输入／审阅代次、matchesInputs与PlanStore完整binding首尾核对。防JS调用者在原生采集期间A→B→A，把审阅B归档成A。matchesInputs只是当前值比较，不能单独证明未被替换。

安全坏JSON可以作为needsReview阻断诊断保存；这类记录不是可审批绑定，不会因保存成功而获得部署或启动权限。旧记录缺少新检查不回填通过，不从旧空数组推导加载事实。

## 审批与通知重入

Root的_reviewAllowsIntentApproval在既有健康、机队、原生观察和binding门禁之外，首尾核本次airspaceInputCheck有效且来源相同，当前原生Intent提供性／计数／完整指纹相同。消费的是实际lastReview和私有参数，不是显示副本或导入记录。缺少新检查、坏来源、后续换值都不能请求或完成审批。

Intent的preparePreview保留坏参数和诊断，但明确feasible=false、风险inputValid=false；requestApproval／approve每次重查当前私有参数。setter先整体提交输入及draft／清预览，再通知；每次实际输入／状态变更和重新预览推进私有不回绕revision，每个同步signal后核QPointer及revision。回调销毁或A→B→A时，旧外层不继续覆盖新状态或返回当前批准成功。该数字不公开、不作为授权凭据。

## 工程边界

本批只经源码阅读、独立生产／消费对照和直接文本检查。未编译、qmake／moc／QML解析、运行helper／界面／认证／SQL／应用、测试、仿真、访问设备、刷机、提交、更新PR或打包。实际Qt数组／null映射、同步重入和性能由工程师后续核验，不能称运行通过。

仍需完整原路线／返程、三类独立启动假设及终结动作对同一冻结区域的联合空间重审；权威空域、可信原点／净空／动力学、最终StartPermit与正式机载结果均未完成。真实性能／耗能、自愈、跨站、长期运营及模型生态不从本批中删除。


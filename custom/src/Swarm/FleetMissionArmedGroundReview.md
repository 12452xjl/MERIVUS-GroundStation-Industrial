# 停止启动事务后的新整组地面风险审阅

## 536：新地面审阅的RAW内容保原NAV朝向

本次完整raw16解码不再把所有geographic param4默认为null。实际有限NAV由共享 [HeadingEncoding](FleetMissionHeadingEncoding.md) 保原float32并标absolute-degrees，再按同一冻结payload的v2／v3语义域核完整items、schema和contentHash；超出唯一tick声明界保持不匹配／未知，不忽略朝向。起降／HOLD默认yaw和速度reserved-zero原义保持，RAW原始内容／指纹不被规范值替代。

当前实际payload对应的原生声明政策由共用StartReadiness在fresh evidence入口及尾部核验；本消费者不另造声明或恢复旧审批。原PM29、Motion、第四异机报告、完整risk／artifact、READ来源、期限、原实际路径false及最终StartPermit门保持。仅源码与静态阅读，未执行helper／solver／parser、业务IO、编译、测试或仿真。下方为历史。

## 535 当前：新地面审阅共同消费完整异机扩张包络

实际 [Worker](FleetMissionStartupClearanceWorker.md) exact8含新增 `interVehicleEnvelopeReview` result3／report28，整份进入原地面artifact／sourceFingerprint及共同准入；不从旧任务或较晚位置补一份名义报告。共享门要求新几何／全异机组合比较完整、声明模型满足且零冲突，之后Witness工厂仍独立核完整结果与同一原模型组。缺／未知／超预算／交叠候选保持clearance-blocked，不借停止旧事务恢复旧批准。

只读riskSummary的clearance-blocked投影同步要求第四报告并保其reasonCodes；不会只显示原三报告而隐藏新增阻断。新阻断原因区分比较不完整、声明包络交叠候选及模型未满足。完整结果受原32MiB artifact和2MiB展示预算约束，不裁掉必需报告；投影不是原报告或实际碰撞证明。无完整时序按全空间保守，原minimumSeparation独立检查不变；terrain接地豁免不进入异机比较。

原READ／PM29／PRIMARY／Motion原Ptr、实际finished、会话、角色及期限全部保留；原实际路径认证false与最终StartPermit阻断不变。本批仅源码和静态／普通文本核对，未编译、执行项目／helper／解析器／solver、测试、仿真、设备操作或改PX4。下方为历史。

## 482 当前：新地面审阅沿原29源绑定进入NAV连接比较

本轮物理phase／Readiness同时冻结 `originalExecutionParameters` exact6，保Targets最初Vehicle／PM／links／PRIMARY弱owner，后续风险再取只可匹配原快照，不替换它。与原Motion Ptr一并送Worker/schema3的完整row4，旧READ／Review时钟、双账号和一次移交边界不延长。

实际结果exact7追加 [WaypointSwitchReview](FleetMissionWaypointSwitchReview.md)，整份进入原地面risk／artifact和共用准入；不是UI单挂一个半径字段。异步排队／finished、来源getter和Witness交接之后以冻结原29行做真实PM尾核；Witness只接本次原PRIMARY弱map，不能换源续鲜度。

finished中的same／source fence及publication通知后的边界都保原29检查；同步通知内改PM、断链或换component不能保留通知前的成功状态。取消／替换不复活旧结果，原worker清理路径不被阻断。

普通NAV接受帽和邻接连接AABB的环境比较与独立TAKEOFF／LAND／HOLD动作边界分开，接地例外不迁移。新静态模型不能认证动态高度接受、平滑器或采样间运动；原actualPathCertified=false阻断仍在，停旧事务不恢复旧批准或生成StartPermit。

仅静态源码／文档工作，未编译、运行、测试、仿真或放行；最终文本核对已记录于本批静态审计。下方为历史。

## 481：新地面审阅保完整显式空间域选择

原地面review的Motion工厂继续消费当前完整payload和规范policy，显式guard生成source2及唯一IntegerDomain，否则保旧source1。原Models随本次实际Worker走原finished／Witness／Authority链，不借停止旧Txn、旧资料或相同数值域恢复许可。

新数值包含结论不扩展名义接地pad，不证明动作／采样间运动，actualPathCertified=false仍被真实准入消费。全部原地面状态、遥测、身份、构型／datum与最终write检查继续，旧审批不迁移。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 471：首次 completed-READ 模型原 Ptr 保留至实际计算和交接

`_beginClearanceCalculation` 仍只取本次原 completed READ 的完整 currentRisk、同一 target.payload、规范 startupTransitionPolicy 和明确 ceiling；逐机首次创建的 [Motion](FleetMissionStartupGeographicMotion.md) 对象现保存在局部完整 `Motion::Models`。原 input2 snapshot、完整组 source 值对应及原 `_gate` 前后同源／期限核对都通过，才一起提交私有 `_clearanceWorkerInput` 和 `_clearanceGeographicModels`。开始前／同源 fence 要求原 map 身份未变，不在普通 tick 或完成回调用新位置、同 source 替身或部分模型重建原组。

实际 [Worker](FleetMissionStartupClearanceWorker.md) 构造接收同一完整原 Models 和本次独立取消 token，真实 finished 回调也固定该 token。回收实际 worker 后，将它返回的完整原 Ptr 组与 Review 私有 map 逐键比较，并核 `nativeModelsMatchInput`；仅同一个 worker、reviewId／epoch／generation、原 token／state 和完整同对象组才建立待接纳结果。身份不符在当前活动计算中明确失败，不以失配结果继续等待或恢复来源。

真实 finished 可能在已有 gate 的嵌套事件循环内到来，因此保留原“普通 tick 接纳”规则。待接纳结果同时保存 `_clearanceCompletedModels` 的完整原 Ptr 组，而不只保存 result JSON。tick 取独立外层副本，消费 pending 标记／副本后，整个 `_acceptClearanceCalculation` 的 same 栅持续要求私有 owner map 与这份实际 Worker 原组相同；来源／UTC／账号／artifact 回调不能以相同 source12 的新实例替换它。

若原全部准入条件满足，[Witness](FleetMissionStartupClearanceWitness.md) 第三参传这份同一原 Models；构造中及返回后的 fence 包含原 Ptr 身份，返回后还逐机核 Witness native getter 的 Ptr 相同。已确认 Review 的 Witness getter也要求与保留原组对应。当前原 Qt 数值包含仍未证明，`numericContainmentProven=false` 与真实 clearance-blocked 保持，不能因这条对象所有权链而解除467的共同准入条件。

取消／失败／退休／代际替换清 Review 自己的 owner、pending 模型 maps、pending result 和 Witness 引用。Worker 构造时已独立复制外层并持原不可变 Ptr，故退休不删除运行对象、不把取消当线程完成，也不提前释放原 transport reservation。成功接纳的原 map 保留给后续同源 Witness 交接；所有实际线程排空、原最早 READ START 派生期限和只缩短 UTC 水位规则保持。

source12、input2、Worker result6、原32 MiB完整 artifact／2 MiB展示预算与全部权限 false 不变。公开纯值 snapshot／resultMatches 仍可为结构验证准备临时模型；它们不替代实际 Worker／Domain／Witness 使用的首次对象，不承诺零耗时或已测性能提升。本轮没有编译、运行 helper／线程／脚本／parser／UI、测试、仿真或设备操作。工程师后续需验证 pending 等待期间 Ptr 组被替换、同源新实例、缺成员、token错配、嵌套完成、取消与 source 重入、末成员失败、销毁／排空和 Witness 原对象交接；同对象不认证原 Qt 数值域或最终 StartPermit。下方历史中的 source 值重建对象方式由本节取代。

## 467：原 READ 同源整域报告参与地面再审门禁

后台 [Worker](FleetMissionStartupClearanceWorker.md) 现返回必需 exact6，包含原名义 `clearanceReview` 和新增 `startupDomainReview`。实际 finished 后先核完整匹配，再通过原 completed READ／来源／UTC／期限尾核；整份结果写入原 artifact 后计算 sourceFingerprint。两份报告都是必需材料，原32 MiB artifact 预算失败即拒绝，不允许为收口而省略整域；仍只允许原可选高度显示诊断按旧规则移除。

原局部名义满足判定改为 Worker 共享 `startupModelSatisfied`，独立要求整域 outerCoverComplete／comparisonComplete／contactContextComplete／declaredModelSatisfied／numericContainmentProven 全部真实 true 且零违规。当前最后一项明确 false，因此完整两报告保留，状态为 `clearance-blocked`，理由为 `startup-clearance-original-qt-domain-containment-unproven`；公开 riskSummary 在原形状内合并两报告的 reasonCodes，避免只说名义接地失败或让理想外包通过掩盖数值域未知。该状态不能确认、取得审批上下文或发布 Witness。

原 READ 起始时钟、同一 source18／datum14、原 v4 首样本、账号／占用／取消／排空与无资料旧路径不变；不恢复之前任务或权限。Compiler pending 和完整 StartPermit 仍未完成。仅修改源码及文档并直接静态核对，未编译、测试、运行、仿真或操作设备。下方 result5 和局部名义判定是历史，由本467升级。

## 458：首份地面风险保留完整地理运动来源

仅对提供净空资料的任务，后台计算从本次 `_artifact.physicalTargets` 已冻结的完整 `currentRisk` 读取 `flightStateSummary.channels.globalPosition.value` 和规范 `startupTransitionPolicy`，并使用同索引原 `_targets.payload` 与原计划 `constraints.maxAltitudeMeters` 创建 [地理运动模型](FleetMissionStartupGeographicMotion.md)。先核车辆身份和 artifact 内 payload 与原 Target 完全相同；不使用删减的公开 `riskSummary`，不增加位置 getter，不重新采样替换本次首次风险或 v4 基准。缺字段、坏类型或模型无法构造均拒绝启动后台计算。

Worker 输入外层仍为8键，`schemaVersion` 改为2；每机为 `vehicleId/readinessSummary/geographicMotionModel` 三键。原四项几何摘要保持，12字段模型来源作为必需值进入完整冻结输入与指纹，不属于可移除诊断。原 READ、实际目标对象、source／datum／UTC 水位、资源上限及从本次 READ 起点只缩短的期限不变；异步完成或再次确认不另建基准、不续期。

现有 result5 和报告仍限名义几何范围，新增模型来源并不表示整个允许地理走廊与 AMSL 高度区间已纳入净空 solver，也不把 LOCAL 运动范围当全球净空约束。`numericContainmentProven=false`、原 Compiler pending 和全部许可边界保持。无资料的旧路径不增加该依赖；未编译、运行、测试或操作设备。下方为历史增量。

## 456：地面同次结果的原生Witness

455后台结果通过原source／datum／UTC尾核与完整模型判断后，在原artifact首次追加报告的同一scope建立[Witness](FleetMissionStartupClearanceWitness.md)。保该次READ开始的原Review时钟、reviewExpires与资料／READ期限最小值；最终factory回调不能换reviewId／epoch／代次／原输入／原artifact或取消token。计算结束、接收已阅及后续授权不续期。

新增非QML startupClearanceWitness(reviewId)仅已确认、非active／held／draining且原对象来源有效时返回同ptr。后续地面Authority和Txn独立共享材料；Review本身自然退休不等于物理来源失效，原控制权限与取消门禁不因持有Witness放宽。原pending、完整运动检查和最终StartPermit仍未完成。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：同次地面风险快照的后台启动净空计算

仅当原计划提供 `constraints.clearanceInput` 时，在整组新 RAW READ 完成后，把 `_captureRisk` 首次实际捕获的风险材料、逐机 `reviewReadiness`、原 source18／datum14 和原截止时间冻结，再进入 `clearance-computing`。未提供资料的旧任务仍沿原同步风险捕获路径。本增量不改变原 READ 指令、目标顺序、完整回读匹配要求或当前来源检查。

后台使用独立、无父对象的 [FleetMissionStartupClearanceWorker](FleetMissionStartupClearanceWorker.md)。输入严格为8键：`schemaVersion`、`captureIdentity`、`plan`、`reviewEvidence`、`vehicles`、`minimumSeparationMeters`、`clearanceSourceEvidence`、`clearanceDatumEvidence`。身份内为 `reviewKind=ground`、原 `reviewEpoch`、正十进制 `reviewGeneration`、原 `reviewId`；逐机只取首份风险中的 `launchType/currentPositionAltitudeSummary/takeoffDerivation/currentPositionToFirstWaypoint` 四个几何字段。审阅证据来自与 `_recordBytes` 同次完整校验的确切记录；间距仅取其原归档空域报告实际应用值，缺失／无效保持未知，不取最新地图或默认值。原生类型的两份来源证据仍由本对象私有保留，线程副本不能反向恢复原生来源。

计算期间 `active=true`，整组预约仍被持有；原120秒读取上限、从原 READ 起点扣除已用时长的最多30秒风险期限、资料 UTC／单调截止均同时生效。后台耗时计入这些原期限，完成通知、重核和确认均不续期。定时／完成门禁可以重新读取以验证原来源与首份风险仍相容，但不会再次调用 `_captureRisk`，也不会用容差内的新 v4 sample 或新 pose 替换最初材料。

只在实际 `QThread::finished` 后读取结果。原 worker 指针、generation、reviewId／epoch、取消 token、完整冻结输入和 artifact 都必须仍属于同一计算；若完成通知落在旧门禁的嵌套事件循环内，先暂存结果，交普通 tick 接纳，不递归进入旧门禁。`resultMatches` 只重建有界来源／几何投影并核报告绑定，不在 GUI 线程重跑环境 solver。接纳前后继续核完整原组的 READ、身份、健康、构型、datum、UTC 及原期限。

最终 artifact 从计算前的原材料附加**完整5字段结果** `startupClearanceReview`，然后才计算最终 `_sourceFingerprint`；该字段为必需，不能因诊断预算不足省略。若原可选 `startupAltitudeDiagnostic` 占用余量导致完整材料超限，只剔除逐机这个显示诊断后重试一次；不移除 v4 首份 reference/sample、不截报告，仍超限则整份失败。输入／几何 basis 不含这个最终报告或最终 artifact hash，避免自引用。只有 worker `ok=true`，且报告 `scopeCoverageComplete/comparisonComplete/contactContextComplete/declaredModelSatisfied` 四项均为真实 `true`、`violationCount` 精确为0，才进入原 `content-ready` 风险已阅流程。完整失败信封进入 `failed`，构造成功但模型不完整／有违规进入 `clearance-blocked`，保留已绑定完整诊断；后者只能通过原24字段 `riskSummary` 查看明确阻断原因，不属于 `_live`，不能确认、获取审批上下文或进入 Authority。完整报告明细界面不在本增量范围。

取消、来源失效、过期立即单向设置取消 token 并退休接纳资格，但不提前清除运行 worker 槽。预约释放必须同时满足实际 worker 已结束和原 native 传输已排空；此时才能允许新请求。析构不等待或删除运行线程：独立 `finished/deleteLater` 清理继续生效，脱离原对象的排空定时器只保存少量 Vehicle／MissionManager 指针、读取 scope 和原预约 ID，不长期保存完整任务载荷、场景或 artifact。通知期间发生取消／析构时，已建立的无父 worker 仍启动为已取消任务，从而保留真正 finished 清理路径。

`current` 和 `riskSummary` 的既有外形保持，所有执行／许可旗标仍为 false。本455只接当次实际风险到后台报告与审阅接纳；尚未接两个 Authority／Transaction／最后 write 的独立净空所有权与运动包络约束，也未解除 Compiler pending。不是完整受控执行目标已完成，更不是净空、物理 datum 或实际飞行安全认证。仅源码和静态对读，未编译、测试、运行、仿真或操作设备。

## 454：新 READ 前同时冻结工程高度基准对应

对于存在 `constraints.clearanceInput` 的原计划，453配置／时间来源成功后，立即使用同一实际 Registry、Catalog 和完整目标 Vehicle 组捕获 [FleetMissionClearanceDatum](FleetMissionClearanceDatum.md)。Catalog schema2 的对应关系须绑定原场景版本、来源文档摘要、垂直 datum、实际本地配置摘要及原生 GLOBAL_ORIGIN 参考摘要，并明确声明 GPI.alt 与该 datum 零偏移。全部成功后才冻结首份14字段证据；发生在首个 READ 与整组预约之前，而非读取完成后补选基准。

后续 `_gate`、READ 前后、风险捕获、已阅及审批上下文均重核同一份原证据，不替换原参考、声明或配置。任何已观察失效为本 generation 的粘性失败，原停止／取消／排空流程处理后续请求并保留已发生事实。只有新的显式地面审阅可重新捕获当次来源，不恢复上一轮批准。首次失败不发布部分成功证据。

`clearanceDatumEvidence` 是完整 artifact 的必需字段，在 `_sourceFingerprint` 计算前与 `clearanceSourceEvidence` 一并加入，不属于可省略的诊断；原20字段上下文、24字段风险展示外形不变。全部可回调检查之后，完整原目标组的配置来源和 datum 都经无回调内存尾检，随后才核 UTC；首次尚无变化连接时同样执行。原 Registry 轻量通知与定时 gate 复用该尾检，Catalog 变化仍撤销本轮。

跨层时间水位包含 datum 内部实际观察到的更晚 UTC，不能只记外层 Source 的早先时间。调用 datum 捕获／重捕时，以外层 UTC 和本实例历史水位的较大值为下限，用栈局部变量接收最后观察时间；内存尾检沿同一下限传递，随后本实例的最后 UTC 也不得小于返回水位。回退到两层观察时间之间同样使本轮失效。原资料终点始终取外层 `observedAtUtcMs + remainingValidityMs`，不因较晚水位重算或延长；私有水位不进入稳定证据或 artifact 摘要。

datum 不添加时间窗口，不延长原120秒读取预算、最多30秒风险期限或453资料窗口所取的最早截止时间。旧资料缺省行为保持。声明对应不等于地理／物理对齐认证、当前 PX4 全球高度有效分支证明或同采样证明；硬件、性能和启动许可旗标仍为 false。本批没有运行几何 solver、加入 Authority／write 准入或解除 Compiler pending；未编译、测试、运行或操作设备。以下453为历史增量，不把新工程声明解释成物理基准认证。

## 453：新 READ 开始前固定净空配置与时间来源

新的地面审阅从原停止事务所保留的确切 plan、Registry 和整组实际 Vehicle 构建 [FleetMissionClearanceSource](FleetMissionClearanceSource.md) 捕获。存在 `constraints.clearanceInput` 时，必须在首个 READ 和整组预约前成功冻结 `evidence`；后续 `_gate`、READ 前后、风险捕获、已阅及审批上下文都只核同一份证据，不在读取完成后重新选基准。最终 artifact 在计算 `_sourceFingerprint` 前加入必需 `clearanceSourceEvidence`，原外层20字段上下文不变。

首次 `capture` 完整规范化并摘要净空输入；后续 `_gate` 使用 `recapture`，严格核首次18字段证据、逐机原13字段配置证据与实际原生 `Current` 来源／UTC，不每次重新规范化或摘要整份场景。Registry 通知仅使用无回调内存来源及 UTC 轻量核对；原定时 gate 的其它物理／授权核对不变，不能以缓存代替实际配置检查。首次资料缺省的旧任务仍绑定原 plan 与原目标，不增加资料依赖。

本机 UTC 必须在声明 acquired／validUntil 窗口内，且跨捕获不能倒退。首次调用前的单调时间加原生剩余有效时长形成独立截止时间，再压缩原120秒读取预算及之后最多30秒的风险有效期；新包、ACK、审批、窗口内时钟调整均不能延长原截止时间。资料／本地批准配置来源变化或失效为本 generation 的粘性失败。Registry 日常通知只检查来源，不每次心跳无差别撤销；原读取、取消与排空逻辑仍负责停止后续请求并保留已发生事实。

完整 `_gate` 的所有授权、读取和物理 getter 之后，再逐机对实际原目标和冻结13字段配置证据执行 Registry 的无回调内存核对，最后检查 UTC。该尾栅不调用插件、不重新选择基准，也不会把首机在后机回调期间发生的来源变化漏作成功前缀。

没有资料的旧计划不增加字段或依赖；存在坏资料、配置指纹不对应、没有当前原生本地批准基线或声明窗口过期时，不进入可审批状态。首捕失败不发布部分成功证据。新的显式地面审阅可重新取得当次首份来源，但不会恢复上一轮批准或覆盖旧违规。

本批不求解几何，不把来源有效解释为包络、基准或硬件真实性认证，也未完成净空报告到 Authority／最后发送的完整链。原 pending、未证明项及许可 `false` 不解除。仅源码与静态对读，未编译、测试、运行或操作设备。

## 447：地面再审不绕过原规划记录的阻断风险

共用 `currentRecord` 在原当前记录绑定和完整记录编码成功后，核对这份确切 `reviewEvidence` 的哈希与绑定一致，并要求非空值的审阅 map、真实布尔 `needsReview=false`、真实字符串 `reviewStatus=reviewable`、非空值的 `riskAssessment` map，以及 `FleetMissionRiskEvaluator::hasNoBlockingRisks` 通过。共享判定只接受真实布尔 `inputValid=true` 和实际为空的 `QStringList`／`QVariantList` 阻断理由，缺失、空值或坏类型不视为空列表。

新 READ 入口、持续门禁、风险捕获、确认及后续上下文均沿原 `currentRecord` 链复核，不使用最新 Coordinator 结果覆盖原记录，不改变原终态、20 字段上下文或 artifact。允许新一次明确地面审阅建立新的物理基准，不等于允许跳过原规划阻断风险。历史查询保持只读可用；遗留阻断记录必须经问题修正和新规划审阅生成新记录，不能改旧标签使其可执行。本批未编译、运行、测试或新增飞行命令。

## 445：地面 READ 前后保留原 v4 高度基准

操作员显式请求新的地面 READ／风险审阅时，v4 从该次实际 Readiness 冻结新的首份 reference/sample；不是把上一轮 beforeArm 样本强加到所有后续审阅。原 terminal 外20／逐机13和实际 Vehicle／PRIMARY／当前来源前置仍不变，旧 scope、旧样本和已记录违规不会被改写、复活或恢复授权。

READ 前物理基准保存必需六键；READ 中、完成及后续当前风险比较对 sample 使用显式容差，稳定投影仍保 reference／政策／来源／匹配状态。完成后的 artifact 保存原基准 sample，不把READ期间最新包当首次样本。`riskSummary` 同样展示这六个强制字段，不走444可选省略路径。v1–v3 不增 ALTITUDE 依赖；不恢复旧授权或认证共同原点／内部valid。未编译、测试或运行。

## 444：新 READ 后风险材料附带冻结高度诊断

`riskSummary` 的逐机 readiness 白名单增加 [startupAltitudeDiagnostic](FleetMissionStartReadiness.md)，直接显示本次 `_artifact.physicalTargets.currentRisk` 内已保存快照；getter 不取实时遥测，不改变原 summary 外层24键／逐机8键。新 READ 完成后的首次风险材料保存获预算接纳的整份诊断，并绑定原 artifact hash。

私有 artifact 的原32MiB与公开 summary 的原2MiB／结构预算分别先容纳完整旧风险，再按原成员顺序用剩余容量加入整份诊断；不丢原风险字段、不缩组、不放宽限额。某机的可选诊断键可能因余量不足被省略，必须解释为“未提供该诊断”，不能推断正常、零值或没有传感器。完整 artifact hash 对实际最终材料计算，公开 summary 仍只读同一冻结材料。

动态诊断不加入 `physicalReadProjection` 的 READ 前后物理冻结白名单；`stableReadiness` 的后续当前／冻结比较显式排除该字段。其它原物理、GLOBAL_ORIGIN四字段、EST、配置及来源变化仍按原规则拒绝，不能借诊断例外重基。前一事务终态外20／逐机13和 step33不改变，也不把本次捕获的 ALTITUDE 冒称前一终态已有的冻结证据。

缺 ALTITUDE、字段未知、周期高度／receipt／age 或诊断预算不可用均不增加 v1–v3 门槛；原账本及捕获时原因仍可解释。此只读历史材料不认证传感器样本新鲜、共原点、PX4内部valid、净空或飞行许可；未编译、测试或运行。

## 443：原点声明纳入地面READ物理冻结

`physicalReadProjection`增加Readiness产生的四个原点字段：`startupGlobalOriginReferenceObservation`、`startupGlobalOriginReferenceRiskFingerprint`、`startupGlobalOriginReferenceCaptured`、`startupGlobalOriginReferenceMatches`。READ开始前的原stable18与过程中每次当前捕获比较；原点数值／timestamp／来源／revision变化不能像新揭示的CURRENT／REACHED数据一样获得重基例外。

前一实际终态事务的新再审上下文逐机row由9增至13，原点四字段进入其phaseFingerprint；外层20字段不变。新READ严格消费13字段row，并将当前Readiness的原点四字段与该原上下文配对，不能在上下文形成后、READ开始前换成新的参考。该新再审不是恢复旧启动批准。

只读`riskSummary.vehicles[].readinessSummary`同时保留原点stable18及captured／matches，不新增认证或发送入口，不改变外层24字段／逐机8字段形状。完整原phase fingerprint继续绑定原点四字段；稳定参考不含receipt／age／原始payload，事件型原点不套三秒周期失效。真正从未收到保持旧路径，已观察后不可用不能降级为缺流豁免。

详见[阶段契约](FleetMissionStartupGlobalOrigin.md)和[Vehicle账本](../../../src/Vehicle/VehicleStartupGlobalOrigin.md)。本批仅源码与限定静态核对，未编译、测试、运行或放行；不证明共同原点／内部有效分支／同采样／净空或最终StartPermit。下方437及更早为历史。


## 437：完整风险的有界只读展示

新增 `riskSummary` 只读 Q_PROPERTY（沿用 `changed`），不改 `current`、`acknowledgeReview`、READ、认证、审批或发送门禁。getter 只读取本实例已经冻结的 `_artifact` 和本机生命周期字段，不调用 `_gate`、Vehicle、Session/WTS、角色服务或 QML 回调，不重新采样、刷新有效期或产生信号。

返回固定 24 键：`schemaVersion`（1）、`reviewId`、`reviewEpoch`、`previousStartupId`、`sourceFingerprint`、`state`、`expired`、`available`、`reason`、`targetVehicleIds`、`constraints`、`healthGate`、`blockedReasons`、`unverifiedRequirements`、`reasonsTruncated`、`vehicles`、`limitations`、`allNewRawReadsMatched`、`readonly`、`historicalOnly`、`authenticated`、`startupPermitGranted`、`executable`、`flightCommandReleased`。

`available=true` 仅指：原完整 artifact 与本 review/epoch/generation/previousStartup 身份一致，仍为 `content-ready` 或 `acknowledged`，原最早 READ START 派生期限未到，且无活动读取、reservation 或未排空传输。它不是现场风险重新验证、账号认证、批准或最终 StartPermit。`readonly/historicalOnly=true`；认证、StartPermit、可执行、飞行命令旗标固定 false。原实时来源变化仍由既有 tick/门禁处理，不能把 getter 当成新的原生准入检查。

空、失败、取消、失效、过期或资料/预算不足，均返回同一形状，`available=false`、稳定 `reason`、空的风险 maps/lists，`allNewRawReadsMatched=false`（未提供此展示事实）；不将旧 artifact 的风险冒充当前可确认内容。`expired` 只说明原风险期限实际已到，不把尚未采集视作“未过期即可用”。上游 `reasonsTruncated`、缺少任一机或不完整原因列表使整份不可用，而不是展示成功前缀。

展示内容采用白名单，而不是序列化完整 artifact 后删除认证字段：

- `constraints` 只来自原 artifact 的 `previousReference.deploymentContext.plan.constraints`：timeLimitSeconds、deadline、startAt、minimumSeparationMeters、maxAltitudeMeters、maxAltitudeReference、geofenceRequired、returnReserveSeconds、requireLeader、synchronizedStart、synchronizationRequired。其它原任务内容不复制；这不是完整原计划导出。
- `healthGate` 保留 allowed、state、selectedVehicleIds、blockingVehicleIds、warningVehicleIds、invalidSelection、globalBlocking、requireLeader、source，以及全部 reasons。每个 health reason 是 vehicleId/code/severity/message，不用“通过”二字替换警告解释。
- `vehicles` 保持原整组顺序，最多 32 行，每行固定 8 键：vehicleId、componentId、frozenArmed、blockedReasons、unverifiedRequirements、reasonsTruncated、readinessSummary、startupTransitionPolicy。逐机原因完整保留；整组原因用 `{vehicleId, code}` 汇总；artifact 中原有未证明边界以 vehicleId=0 列入未验证要求，不将条件匹配解释为这些要求已完成。
- `readinessSummary` 白名单保留原 source/precondition/phase 旗标、launch/end、递延动作、未编码要求、实际起飞高度推导与当前位置连接、参数摘要、原六类 flight-state 风险通道、EST、坐标、ODOM/运动参考及其 known/fresh/matches、所有未认证/未许可边界。保留原取值和未知 null；不按原因字符串删掉“需要解锁”等条件风险。`startupTransitionPolicy` 保留本次原生规范化后的完整 v1/v2/v3 过渡政策，包括运动、航向和 HOME 包线，不补默认政策。

不公开 primaryIdentity、roleSnapshot、Session/角色认证收据、previousReference 全包、完整来源对象、原 member/payload、raw READ receipt 或 canonical wire items。保留下来的物理观察摘要不是原任务载荷，也不证明机载同一采样、可信 local/HOME 内部分支、飞行员输入中立或净空。JSON 纯值验证拒绝 QObject/自定义指针（包括 typed-null），深度最多 12、节点最多 32768、单 map 最多 256 字段、单 list 最多 4096 项、字符串最多 8192 字符、总 JSON 最多 2 MiB。超限整份不可用、不截断。输出经过 Qt JSON 值转换，实际未知标量成为显式 null，避免 QML 序列化丢键或补成 0/false。

本批只做源码和静态文本/正向合同审阅；未编译、测试、执行 helper/认证/应用/UI/仿真或设备，未改 PX4、提交、推送或打包。最终 StartPermit 与机载/实测证据缺口未因新增显示而解决。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestReview、acknowledgeReview、reviewContext、actualSourcesForStartApproval及pure nativeReadAdmissionCurrent核实际Session/previousTxn/Deployment同组；scoped READ预发送fence继承该检查。仅移除primary Link必须GUI线程的误拒，Vehicle/MM/PM/VehicleLinkManager实际线程与归属不放宽。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


280–284 新增[显式局部运动与航向观察政策](FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

275–279补充READ物理冻结：physicalReadProjection加入Readiness的稳定startupOdometryReferenceObservation、其risk fingerprint及captured/matches四项。真实CURRENT/REACHED读取带来的既有例外不适用于ODOM reference；采集过程中frame/reset/状态/参考revision/source/fresh变化不得在final READ中悄悄换为新基线。正常收包年龄、样本时间、运动/quality分值不在该20字段身份中，full原始观测仍由Vehicle独立保存。没有新增流请求、许可、HOME或local有效分支。见[原生契约](../../../src/Vehicle/FleetStartupOdometry.md)，未编译/运行/测试。

280–284为schema2再冻结startupMotionReferenceRequired/startupMotionReferenceMatches两个当前旗标；年龄数字不入身份，跨原政策gap门槛时匹配变false而不能从final新包重设READ基线。完整startupTransitionPolicy v2/原member/profile仍随完整风险与版本绑定，新政策必须重新计划/取证/审阅/独立审批，不能在已批准context中添加默认包线。

270–274后端支持多轮真实来源：控制器从[有界原生序列](FleetMissionStartupSeries.md)取最新已初始化actual Transaction，冻结QQ/UUID/epoch/generation/sourceHash，在旧链精确退休前后复核。Review/Approval/Authority共享该冻结QQ，不能在changed/tick自动换前代或回退旧第一owner；新的初始化消费者使Series最新指针前进，不恢复旧授权。原terminal20中的旧Authority改用固定原生摘要6与完整源hash，完整部署/任务/当前风险/真实previousSteps仍保留，不递归复制全部历史。最早新READ START期限、原整组和native无MODE/已排空/地面条件不放宽。这里只完成源码和限定静态复核，HOME分支/local frame/净空/最终StartPermit等仍待实现。

2026-10-14，修改记录 250–254。这里只描述地面站源码契约；未编译、运行、测试、调用业务或系统接口、修改飞控、刷机或放行。

## 用途与当前边界

`FleetMissionArmedGroundReview` 为已经停止并排空、从未排队 MODE 的实际 `FleetMissionStartupTransaction` 建立**新的**完整机载任务回读和风险已阅依据。可以保留原整组中当前已解锁与未解锁的地面 POSCTL 机体，但逐机冻结当前 armed 值；自动停机不是允许静默换组或重新 ARM 的理由。

本批仅补齐“重新读取 → 新风险捕获 → 显式已阅”的后端，不复活旧认证/审批、不产生新 StartPermit，不发送 seq0、ARM、MODE、DISARM、清任务或回滚。新的独立账号授权、后续模式事务、HOME 分支有效性、本地坐标参考、运动/转向、净空和执行完成仍待开发；原 25 个目标及六类剩余范围没有缩减。

## 原生来源与读取

255–259 增量：已阅且仍当前的 Review 新增 native `actualSourcesForStartApproval(reviewId)`，返回原八对象加 `groundReview` 的实际九对象；目标与目录/健康提供者 getter 只供新的独立账号 ledger 冻结实际来源，不提供旧审批或指令权限。已完成风险阶段可在真实独立账号凭据提示期间用 `Session.identitySnapshot()` 继续核对原 SID/角色/WTS/旧期限，不能因提示标志本身撤销已阅；正在执行 READ 则仍用更严格的无提示身份检查。任何身份/角色/工作站变化、源风险变化或原最早 READ START 截止仍会失效；消费认证必须等提示结束，不能利用提示期间的只读快照取得证明。

- 实际八对象：Session、RolePolicy、PlanStore、previousStartup、Deployment、SwarmController、HealthCenter、authorizationOwner。强类型、线程、实际 Vehicle/Manager/ParameterManager/Link/目录和健康提供者都冻结；不能用同 ID 的新对象接管。
- 原事务必须终态、非 sending/entry/receive/排空、未占用。每机历史 `queued[MODE]`、`uncertain[MODE]` 与原生 `requestIssued` 均拒绝 MODE 已可能提交的情况。取消不证明没有起飞。
- 初始引用用真实最新非恢复完整记录、原部署载荷和终态实际风险。原生保存完整 20 字段引用；投影排除输送元数据前必须与保存原文精确一致。只读 current 捕获不能覆盖原引用。
- 专用 `mission.start.ground.reassess` 只给默认 operator/safety_officer/commander/administrator。普通 planner/maintenance 的未解锁读权限没有被扩大。该权限只管读取及风险已阅，不是启动或二次账号批准。
- 新 `loadFleetMissionForArmedGroundReview` 冻结实际强 owner/reviewEpoch、当前 armed/disarmed POSCTL、实际 PRIMARY、flight/version epoch 和 scope。每包只允许 REQUEST_LIST、REQUEST_INT、读取结束 ACK，并在插件前后/排队前检查 mandatoryFence。
- 原 `loadFleetMissionForStart` 与原完成来源校验仍只允许未解锁读取；新成功 factory 保持 9 字段，完整 RAW 成功收据保持 31+3=34 字段，不通过 caller flags 产生例外。
- 三个所有权 getter 是纯原生查询，不反调完整 gate，避免 `Review → previous → PM owns → Vehicle → Review` 递归。初始原事务入场必须全栈静止；后续只允许本实际 scope 的 READ 接收栈继续下一包，不允许 ARM/版本/飞行/坐标接收栈旁路。

## 新依据、期限及取消

最多 32 架、单机 2048 项、全组 8192 项；采集最多 120 秒、100 毫秒轮询，逐机读取 deadline 有界。完整原始 INT 内容、顺序、count、结束 ACK、原生操作/来源/新计数/缓存指纹及全部规范载荷必须匹配。

READ 可以新获知原先未知的 CURRENT/REACHED，读取前不将这两个观测当物理冻结基线；最终风险完整包含六通道。其它物理频道、独立 LOCAL/HOME 的语义 revision、HOME `time_usec`、参数/版本/目录、机体 armed、健康与 Leader 都不能重置基线或容忍 ABA。最终依据保存完整原始读取及当前风险，不将摘要当原始证据。

新风险最多 30 秒，从全组最早的实际 READ START 年龄折算，不能从完成、点击已阅或旧审批到期重新计时；仍实时核实际登录/角色/桌面来源。旧无关审批到期本身不撤销未变的当前身份，也不能续期。

取消先关闭发送 gate，再取消实际 scope。已经排队的只读请求/远端传输状态无法回滚。只有实际本地协议/接收栈排空后才释放整组预留；对象被销毁时独立 cleanup timer 只负责排空释放，不发送命令。

public `current` 只给有界摘要；完整 SID/旧认证、原始遥测和私有 artifact 不写入 public 快照或黑匣子事件。native `reviewContext` 保持完整 20 字段，新已阅与原期限分开，所有 `previousAuthorizationRestored/startupPermitGranted/executable/flightCommandReleased` 为 false。

## 工程师后续验证项（本机未执行）

1. 受支持 Qt/MSVC 工程重新生成与编译，核对内部类型/pri/QML 后端和参数信号签名。
2. 混合已解锁/未解锁整组、自动停机、MODE 已排队、外来读取、主链路切换、对象删除/重入、早期读取超时、迟到回复和取消排空。
3. 完整 RAW/风险哈希、纯同值到达与真实 ABA、新 CURRENT/REACHED、HOME/LOCAL 参考变化及未知有效分支。
4. 确认没有旧批准恢复、隐式解锁、MODE、任务写入/删除、回滚或对远端原子/无线归属/机载有效性作虚假保证。

总进度见 [路线图](../../../docs/MERIVUS_FLEET_OS_ROADMAP.md)，启动事务见 [FleetMissionStartupTransaction](FleetMissionStartupTransaction.md)。源码和文本静态检查不是功能验证或飞行许可。

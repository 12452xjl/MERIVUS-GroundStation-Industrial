# 独立启动风险审阅收据（不是启动许可）

## 536：逐航点朝向编码与原生消费者接线

冻结 payload 的 wireSchema 接纳共享 helper 定义的旧 v2 和 heading-tick v3；历史回读收据仍须和该同一目标 payload 的 schema、hash、项数严格对应，不能混用两个域。共用 StartReadiness 已在首尾实际证据捕获中核当前目录是否明确覆盖此 schema，旧 v2 工程声明不隐式覆盖 v3。

读回匹配只表示其声明域内的当次内容比较；本审阅仍不是实际朝向达成、原子远端状态、执行结果或 StartPermit。

共享合同见 [HeadingEncoding](FleetMissionHeadingEncoding.md)。本批仅源码修改、直接静读与普通文本核对；未编译、运行实现／解析器／算法、测试、仿真、硬件操作、修改PX4、刷机、放行、提交、PR更新或打包。下方为历史。

## 535 当前：完整异机扩张包络结果不可从准入中省略

同次 [Worker](FleetMissionStartupClearanceWorker.md) 结果升级exact8，必含 `interVehicleEnvelopeReview` 完整result3／report28。原 `resultMatches` 成功后，整份结果写入首次完整artifact与原来源指纹；新报告不能为通过原2MiB预算被裁剪，仅原可选高度诊断仍可让位。实际准入消费共享 `startupModelSatisfied`，包括新报告几何完整、比较完整、声明模型满足及零冲突；缺少、Unknown、未完成组合或交叠候选都不能确认或创建Witness。

阻断原因区分异机包络比较不完整、存在交叠候选和模型未满足；声明空间交叠不是实际碰撞证据。无完整时序按全空间保守，不替代原名义minimumSeparation检查，也不借接地豁免排除异机比较。原捕获身份、Motion原Ptr／PM29／PRIMARY、实际worker finished、期限及完整风险预算不变。原实际路径认证仍false，全部最终StartPermit门保持。

仅静态源码及普通文本核对，未编译、运行实现／solver／helper／解析器、测试、仿真、设备操作或改PX4。下方为历史。

## 482 当前：原29快照、真实NAV连接报告与最终原源尾核

从同次当前Readiness的 `parameterSummary.originalExecutionParameters` 冻结四组原29快照，保最初Targets中的Vehicle／PM／links／PRIMARY及其弱owner；重入或后续采样不得替换已保快照。Worker输入升级schema3 exact8，成员row4为 `{vehicleId,readinessSummary,geographicMotionModel,parameterSnapshot}`，原Motion Ptr继续另传而非由JSON重建。

真实Worker结果升级exact7，新 `waypointSwitchReview` 与原名义、整域报告一起进入风险／artifact和完整匹配；新报告实际比较NAV接受区及邻接完整连接域的coverage／corridor／terrain／obstacle，不只是参数guard。入队前、finished和后续来源回调尾核均以冻结原快照核29项，最终Witness接收原PRIMARY弱owner map，不在完成时重新抓一个owner。

计算开始通知可以同步改变参数／断链／换component，通知返回后仍核原29行并invalidate／cancel；独立worker仍启动以确保finished清理，不留下永不finished的对象。普通finished和acknowledge的publication通知后也核原29尾部，不能因通知前检查成功保留旧成功。

共用准入同时要求新模型完整、比较完整、零violation且声明模型满足，旧整域 `actualPathCertified=true` 条件仍保。现producer仍false，新报告也明确不证明动态高度接受／平滑器／真实路径，因此仍clearance-blocked，不安装Witness或释放StartPermit。

本批仅静态修改，未编译、运行、测试、仿真、硬件或刷机；工程验证全部待做，最终文本核对已记录于本批静态审计。下方为历史。

## 481：原模型可显式为source2，不用资料声明替代

普通review现有工厂调用持完整payload和normalized policy，因而保profile显式startupSpatialGuard并生成原Motion/source2；无选择source1原义保。原Models进入实际Worker，后续finished、Witness及Txn延续同对象和完整source；仅新增IntegerDomain模块不会替旧review补认证。

新全组数值包含只对受控整数采样谓词，实际动作及采样间路径仍缺证。原环境／来源／审批／时效／两Authority最终门禁不放宽，Worker实际路径条件仍false，因此不是StartPermit完成。

本批仅修改地面站源码、工程登记与交接材料；核对限直接静态阅读、纸面包含关系及普通文本／路径／Markdown目标／git空白。未编译、执行helper／解析器／业务脚本／模型／QML／应用，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。新整数包含只针对显式受控谓词的遥测采样空间，不认证旧Qt全集、物理轨迹／动力学／采样间运动、场地真实性或StartPermit。原25目标及六类实质剩余保持，整体约40%仍仅源码工作量粗估，不按本批路径数或算法结论上调。

下方为历史记录。

## 471：同次首次 Motion 原对象贯穿计算与 Witness

提供净空资料时，首次 `_capture` 从同次原 GPI9、完整真实 payload、原规范策略和 ceiling 逐机首次创建 [Motion](FleetMissionStartupGeographicMotion.md) 不可变对象，先保存在局部 `Motion::Models`。完整组输入、全部来源尾核及原2 MiB完整风险 artifact 预算都通过后，才一起提交首次 input2 和私有 `_startupGeographicModels`；不发布可用模型前缀。外层 map 逐项独立建立，值持有原 `Ptr`，不从 source12 反建实际计算对象。

后续 `_capture` 只从私有原组取得同一 Ptr，通过 `matchesSource` 核原 GPI9、全部 canonical 请求／原完整 SHA、规范策略和 ceiling；不在本 Review 重新 prepare 对象替代首次模型。该核对复用原 yaw null／数值规范，不把 Builder invalid yaw 和明确 JSON null 的合法表达差异误作变化。live 栅同时固定原输入和逐 ID 的 Ptr 身份；当前性再捕不替换首次 pose、原 v4 reference／sample 或完整风险。公开纯值 snapshot／resultMatches 的既有有界结构验证仍可能建立临时验证模型，它们不成为实际计算或 Witness 持有的对象，不能据此声称整个值核验路径没有任何 prepare 成本。

实际 [Worker](FleetMissionStartupClearanceWorker.md) 构造现在同时传原 input 和这一完整原 Models；`nativeModelsMatchInput` 在完整冻结输入上核源值对应，但不创建替身。actual queued finished 后，Review 读取该实际 worker 的 `originalGeographicModels()`，要求其完整 key／Ptr 与本实例原组相等，再继续原 token／generation／source／artifact／期限和全部来源复核。slot 仍只由真实 finished 释放，不把取消当已经结束。

若既有全部模型准入条件满足，创建 [Witness](FleetMissionStartupClearanceWitness.md) 时第三参传**同一原组 Ptr**；可重入 fence 同时锁住该组身份，返回后还逐机核 Witness 原生 getter 返回的对象与原组相同。已确认 Review 的 native Witness getter也核同一对象，而不是只比 source hash。当前 `numericContainmentProven` 仍 false，因此467的真实 `clearance-blocked` 路径保持，不能因对象连续而绕过数学包含条件或产生 Witness／Authority。

取消／失效／失败清本 Review 的原模型 map 和 Witness 引用；正在运行的 parentless Worker 已独立持有原模型，继续按原取消和 actual finished／deleteLater 生命周期排空，不强制删除或在 GUI 等待。新请求仍不得越过活动 worker slot，代际替换完整清旧 map。描述12字段（source schemaVersion1）、input schema2／exact8、result6、原来源18／datum14、实际账号审批、原截止和权限 false 均不改变。

本轮仅源码与直接静态读回，未编译、运行 helper／线程／解析器／脚本／UI、测试或设备操作。工程师后续需验证同 descriptor 但新 prepare 得到不同 Ptr 的替换拒绝、整组缺键／末成员失败、完成再捕、原 v4 容忍样本、source 回调重入、取消／销毁后 Worker 原对象存活、实际 finished 身份、Witness 交接和原期满。对象同一不是 Qt／libm 数值域、实际构型／datum、物理净空或完整 StartPermit 证明。下方历史中按 source12 重新 prepare 建立实际对象的方式以本节为准。

## 467：整域结果成为实际启动审阅接纳条件

后台结果升级为 [Worker](FleetMissionStartupClearanceWorker.md) exact6：原名义结果及新增 `startupDomainReview` 必须共同通过完整 resultMatches，再将整份六字段结果写入**首次原 artifact**、sourceFingerprint 和 artifactFingerprint。原 v4 reference／sample、原输入／epoch／generation、来源尾核、实际 finished、原2 MiB artifact 预算和不续期规则不变；必需两报告不能被预算裁剪。

`startupModelSatisfied` 不再使用本模块的名义四 flag 局部门禁，而消费 Worker 共享判定；它同时要求名义满足及整域完整、零违规和 `numericContainmentProven=true`。当前整域报告该值固定 false，因此真实结果保存在原 artifact 后进入 `clearance-blocked`，原因为 `startup-clearance-original-qt-domain-containment-unproven`，不把理想外包模型当作原 Qt 数值谓词已被证明。该状态不能确认或取得 Witness／Authority；工厂亦独立核同一共享条件。

原无资料任务不新增默认限制；Compiler pending、账号／审批／来源／期限门禁、最终 StartPermit 和全部 false 权限声明保持。本批未编译、运行线程／脚本／UI／SQL、测试、仿真、刷机或放行；下方 result5 和只检查名义模型的内容为历史，由本467替代。

## 458：同次原生地理运动模型成为必需计算输入

仅对提供净空资料的任务，在原逐机实际 Readiness 捕获后，从**这一份** `flightStateSummary.channels.globalPosition.value`、同机原完整 payload、同一冻结成员执行配置规范化所得的 `startupTransitionPolicy` 和原计划 `constraints.maxAltitudeMeters` 创建 [地理运动模型](FleetMissionStartupGeographicMotion.md)。普通 `capture` 内部使用该成员策略，但不公开策略键；这里复用原审阅行已有的执行配置规范化，模型与审阅行共用同一策略，不误取仅 `captureForStartup` 才公开的字段。模型来源先保存在本次捕获的局部表中，不从显示用几何摘要反推、不另读位置、不放入可因预算省略的 ALTITUDE 诊断。缺少必需字段、坏类型或模型无法构造即拒绝本次捕获，不以空模型继续。

后台输入的外层仍为8键，`schemaVersion` 为2；每个成员严格为 `vehicleId/readinessSummary/geographicMotionModel` 三键。原四项 `readinessSummary` 几何字段不变，新增模型保留首次 GPI、完整 canonical 请求项、原策略与高度上限等12字段来源，进入同一完整输入指纹。后续捕获只验证当前来源仍与冻结输入相同，不替换首次 pose、原 v4 sample 或风险材料；source／datum／UTC 尾检、原资源预算和只缩短的截止时间保持。

本增量只把真实运动判断所需来源接入同次后台任务；现有 result5 和净空报告仍检查名义几何范围，尚未求解整个允许运动走廊与 AMSL 高度区间的包含关系。`numericContainmentProven=false`、原 Compiler pending 和许可边界保持；不是整域净空证明、实际轨迹认证或完整 StartPermit。没有资料的旧任务保持原路径。仅源码及静态对读，未编译、运行、测试或操作设备。下方为历史增量。

## 456：完整同次报告建立原生共享Witness

在455实际finished消费／原首v4基准保持／完整artifact hash之后，只有result ok与四项模型完整／满足true且零违规才以原input8／result5、原typed source18／datum14、实际Registry／Catalog／Vehicle集合创建[Witness](FleetMissionStartupClearanceWitness.md)。传本Review原QElapsedTimer与原_expiresAtMs，不为异步完成开新TTL；工厂回调后仍核原self／gen／state／token及固定source／artifact，再同步只缩短的截止与UTC水位。

新增非QML startupClearanceWitness(reviewId)仅已确认且当前原对象／原材料／期限有效时返回原shared ptr，实际Authority取得它；新request清空本地引用不退休别的owner同源材料。原source／审批及取消门禁保持；Witness不是StartPermit，也不解除pending。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：同次启动净空后台结果进入普通审阅

存在原生净空资料时，首次 `_capture` 仍冻结完整原风险、source18、datum14、实际读取来源及原期限；再把同一计划、准确归档 reviewEvidence、原目标的四项几何摘要与实际 review UUID／epoch／generation 复制为 [StartupClearanceWorker](FleetMissionStartupClearanceWorker.md) 的 exact8 输入。输入指纹是独立 pre-artifact geometryBasis，不含最终报告或最终风险指纹，不从页面、最新草稿或别次观察补点。

`requestReview` 成功可仅表示已受理：状态为 `clearance-computing`，此时不能已阅、申请批准或获取许可。旧无资料任务保持原同步路径。新 parentless 单任务 worker 不保存 Vehicle、账号或任何活对象；取消立即退休结果资格，任务槽只在实际 queued `finished` 清除，取消后不能在旧线程尚未结束时新开同一 owner 的计算。析构只取消，由独立 finished／deleteLater 清理，不在 GUI 等待线程。

完成时回 GUI 重新核实际 source／读取／风险与首次捕获一致；v4 新高度样本只检查是否仍在首次容差内。最终材料**从首次完整 artifact 副本**加入完整 `startupClearanceReview` result5，再生成 source／artifact 摘要；不能把完成时的新样本或新风险替换成首次基准。后续已阅／审批 `_capture` 同样只验证当前性，原 geometry input 指纹必须保持。

`resultMatches` 只重提取有界几何和核结果来源，不重跑净空 solver。只有结果 `ok=true` 且报告完整范围、完整计算、完整接地上下文及声明模型满足四项均为真实 true、违规数为0时，才进入 `reviewable`。构造失败进入 failed，模型不满足进入 clearance-blocked；均不能获取 approvalContext。报告是必需材料，仍受原2MiB artifact预算；只有可选 ALTITUDE 诊断可因余量不足省略。

原30秒、内容读取和场地期限从计算前保留，只缩短；后台耗时、完成通知、已阅及重新认证均不续期。source18／datum14仍由本机 owner 按原 native 类型保存，worker 的 JSON 副本不用于恢复原生来源或权限。

本批还没有把报告独立移交到两个 Authority／Transaction／Transition 和最后 Vehicle write，也没有证明后续允许运动包络被原 solved trackingError／localFrame 包含；Compiler 的原 pending 保持，尚不能让带资料任务通过完整部署链。报告即使满足也不是完整 StartPermit、物理净空、4D 或实飞认证。仅源码／限定静态核对，未编译、运行、测试、仿真或放行。下方454为历史。

## 454：原生构型与工程高度基准声明必须同时对应

存在 `constraints.clearanceInput` 时，原453完整配置／时间来源成功后，调用 [FleetMissionClearanceDatum](FleetMissionClearanceDatum.md)，使用同一实际 Registry、协调器所属 Catalog 和完整原目标 Vehicle 组。必须取得 Catalog schema2 中与原场景版本、文档摘要、声明垂直 datum、配置摘要及原生 GLOBAL_ORIGIN 参考摘要相符的零偏移声明；缺失、失配或不可用均使本次捕获失败，不发布成功前缀。

成功的原14字段 `clearanceDatumEvidence` 与18字段 `clearanceSourceEvidence` 一同冻结。后续物理风险捕获前后、重新确认与审批上下文，只对首次 datum 证据执行 `recapture`，不接受更新后的声明或原点作为替代基准；任何已观察失效为该 generation 的粘性失败。两份证据在原 `source` 和 artifact 计算摘要前作为必需字段加入，保留完整原预算，不能像可选高度诊断一样因预算不足省略。

所有可回调读取之后，对完整原组同时执行配置来源和 datum 的无回调内存尾检，再检查原 UTC／单调截止时间。首次捕获即执行该尾检，不依赖尚未安装的变更连接；原 Registry 通知、250 ms 轻量期限检查与审批尾栅共用此检查。原 Catalog 变化仍撤销审阅。datum 不含新期限，不续期，也不改变原30秒／内容读取／资料有效窗口的最小截止时间。

跨层 UTC 水位同时覆盖外层 Source 捕获、datum 内部重采及其最终内存尾检：调用 datum 时传入外层已观察 UTC 与本审阅历史水位的较大值，并用本次栈局部变量接收 datum 实际最后观察时间。后续读取不得退回该时间之前，即使仍晚于先前外层 Source 的时间也拒绝。资料有效终点仍为原外层 `observedAtUtcMs + remainingValidityMs`，单调截止时间仍只缩短；新水位只保存在本实例私有状态，不进入证据、artifact 或摘要。

未提供净空资料的旧计划仍走原缺省路径，不增加 datum 字段或资料依赖。这里的 `declaredDatumCorrespondenceMatches` 与 `sameDatumZeroOffsetDeclared` 仅表示本机实际来源对应一份明确工程声明；物理高度对齐、当前 PX4 全球高度有效分支、同一估计器样本、硬件真实性、性能及启动许可仍未证明。没有执行几何 solver、改变 Authority／最后发送逻辑或解除 Compiler pending；未编译、测试、运行或操作设备。以下453为前一增量，关于尚未建立基准关系的边界仍不被工程声明冒充物理证明。

## 453：真实配置与声明有效期来源固定进入审阅

对确切已绑定计划中存在的 `constraints.clearanceInput`，首次捕获调用原生 [FleetMissionClearanceSource](FleetMissionClearanceSource.md)：传入实际健康中心的 Registry 和原部署／完整读取对应的 Vehicle 对象，不能用同 systemId 的替身或 QML 提供的证据。有效结果中的稳定 `evidence` 以必需字段 `clearanceSourceEvidence` 同时加入 `source` 和最终 artifact，再计算原摘要；不是可因预算不足省略的诊断。

首次证据在本 review generation 内冻结。物理风险捕获前后、重新确认、审批上下文、Registry 变化通知及未忙时原 250 ms 期限检查都会复核相同配置来源／已批准本地基线与声明窗口。正常重复遥测通知只核对，不一律撤销；配置／本地批准语义身份变化、失源、时间失效或跨捕获观察到 UTC 回退，会使本次审阅失效，恢复旧值不能重基。回调后继续核原对象、generation、Episode 和实际目标；来源捕获期间拒绝新的重入审阅／确认。

完整净空输入只在首次 `capture` 规范化和计算摘要。之后风险捕获、已阅及审批上下文使用 `recapture`，只校验首次冻结的18字段证据、原逐机13字段配置证据及当前实际原生来源／UTC，不重新处理整份场景。原 250 ms 期限检查和 Registry 通知使用无回调内存来源核对加 UTC 检查；这不替代关键操作中的实际配置 `Current` 重核，也不是从缓存恢复身份或延长有效期。首次确认资料缺省的旧任务仍绑定原 plan 与原目标，不反复解析不存在的资料。

在全部可回调的原生读取／物理检查之后，对传入的完整原目标组（首次捕获也不能借尚未绑定的空 `_targets` 跳过）执行 Registry 的 `nativeConfigurationEvidenceMemoryCurrent`，逐机只比较冻结的13字段配置证据；随后仅检查本机 UTC 和原单调截止时间。审批上下文最后一个部署 getter 后同样保留该尾栅，不再调用插件或重采配置。

`remainingValidityMs` 在调用前取得的本对象单调时间上形成截止时间，仅与原 30 秒／内容读取截止时间取更小值；后续只缩短，不在 ACK、审批或新鲜重复包到来时续期。稳定证据不含当前 UTC、age 或剩余秒数。未提供净空资料时保留旧行为，不增加 artifact 字段；坏资料或资料与实际配置不对应不能产生可审批的审阅。

这里只证明本机原生配置对应关系和本机 UTC 落在资料声明窗口，不证明外部文件真实性、物理包络、坐标／高度基准关系、净空或启动安全。未接几何 solver、完整异步启动审阅或新的 Authority／最终发送门禁；原 Compiler pending 和所有许可 `false` 保留。仅源码与静态对读，未编译、测试、运行或设备操作。

## 447：绑定规划记录的阻断风险独立复核

普通启动审阅的共用 `recordMatches` 在原绑定、计划和审阅证据哈希核对成功后，独立检查这份确切记录的 `reviewEvidence`：必须为非空值的 map，`needsReview` 为真实布尔 `false`，`reviewStatus` 为真实字符串 `reviewable`，且 `riskAssessment` 为非空值的 map，并通过 `FleetMissionRiskEvaluator::hasNoBlockingRisks`。该共享判定要求真实布尔 `inputValid=true` 和实际空的 `QStringList`／`QVariantList` 阻断理由；缺失、空值或错误类型不能转换为空列表而通过。

首次捕获、重新确认及审批上下文继续经过原共用捕获链，不能借最新 `coordinator.lastReview` 替换已绑定记录，也不改写原 artifact、哈希、上下文形状或旧高度基准。历史阻断记录仍可只读查看；若要进入新的启动审阅，应修正实际规划问题并生成新的审阅记录，不能仅修改旧记录的状态标签。该防线不新增命令、启动许可或飞行安全证明。本批仅源码与静态阅读，未编译、运行或测试。

## 445：原已阅 v4 高度基准不可后补或重采

普通 Readiness 的 v4 六个必需高度基准字段直接进入逐机 `readinessSummary` 和首次完整 artifact/hash，不走444可选诊断预算。随后 Authority 必须使用原已阅 sample，不能把后来重采当初次依据。

再捕比较先验证两份完整 artifact/hash，逐机核原 vehicleId、相同 reference／指纹／政策及当前 sample 对首次 sample 的容差，再比较仅去 sample／444诊断的稳定身份。私有重入仍比较完整原材料。仅只读 acknowledge 可确认完整 hash 有效且整份稳定风险、原不满足基准（或原早退无基准）精确不变的 blocked 快照；这不使缺失基准通过 `approvalContext`、Authority 或启动，后三者仍要求完整匹配。两空 map 本身不构成通过依据。

v1–v3 原语义不变。材料不是共同原点、内部高度有效性、净空或执行许可证明；未编译、测试或运行。

## 444：首次高度诊断随原风险材料冻结

逐机 `readinessSummary` 白名单增加 [startupAltitudeDiagnostic](FleetMissionStartReadiness.md)；现有工作台完整 JSON 详情可显示，不新增命令、按钮或确认门槛。首份 `_artifact` 及覆盖完整材料的 `artifactFingerprint` 保存获预算接纳的当时诊断，不因之后周期 ALTITUDE 更新而改写、续期或替换原已阅／批准材料。

逐机捕获检查和原完整风险先按不含新诊断的材料执行原预算；最终 artifact 保持原2MiB及结构预算。随后仅按整组原顺序、使用剩余空间加入整份逐机诊断，不接受半份封套。容不下时可以缺少该 `readinessSummary.startupAltitudeDiagnostic` 键，含义只能是“未提供该诊断／预算不足”，不是正常、零高度或传感器未安装。原风险字段、全组成员和限额不改变；首次完整 hash 覆盖实际最终保存的材料，防守性回退仅移除可选诊断。

仅在“本次重新捕获与首次冻结材料”的风险身份比较中使用 `stableArtifactIdentity`：先检查每份完整材料的有界结构和原 `artifactFingerprint` 确为完整内容的 SHA-256，再只剔除逐机 `readinessSummary.startupAltitudeDiagnostic` 和该完整材料指纹本身；其它字段全部保留。任何形状／指纹错误使比较失败，不允许两个错误空投影相等通过。已阅和 `approvalContext` 两条再捕路径均用此规则；私有 `_sourceCapture/_artifact` 的重入／生命周期比较仍对原完整 map 精确相等，返回的批准上下文也继续携带首次完整 artifact/hash。

早退 readiness 没有 sourceFingerprint 时，其后备 hash 同样排除动态诊断。普通 sourceFingerprint 的原显式风险字段、真实角色／来源／任务／读取／期限条件没有被排除；诊断缺失、周期值或收包年龄变化不等于风险身份改变，也不代表其它真实风险可忽略。此材料是捕获时显示，不是实时遥测、传感器新鲜度、共原点、PX4 内部 valid、净空或启动许可证明；未编译、测试或运行。

## 381–385：启动 EST 证据接线与连续间隙

[独立估计器契约](../../../custom/src/Swarm/FleetMissionStartupEstimator.md)将实际来源、语义 revision、完整 flags、六个状态位和已观察时钟回退组成稳定11键；Readiness/阶段身份与地面新 READ 的物理投影冻结该依据，普通 timeUsec 增长、收包 receipt 和 age 不进入风险指纹。核心四项必须健康、无 GPS/加速度错误及回退；始终健康的 optional flags 变化只经真实 ordered journal 推进，坏后恢复不能抹掉旧坏叶。

原生启动 step receipt 显式升为 schema3/33键，增加 queueEstimatorObservation；Vehicle、Transaction、Transition、Run 未发送准备退休及 pri 必须整批接收，不能把旧2/32当具备EST连续性。原六flight/schema2、READ收据和profile policy1–3不改变。启动 gap 从同一实际原包入口独立采样 startup clock，首次 queue 起包含初始年龄、逐包与旧步骤尾段；更早审阅/审批至首次 queue 只作当前新鲜/同源语义复核，没有完整间隙历史。

本轮只改源码、工程登记及交接文档，不新增控制按钮、不修改PX4，不编译、测试、运行helper/认证/应用/UI/SQL/仿真或设备，不刷机、更新PR或打包。最终StartPermit、可信HOME/local/净空、正式机载实例/结果、长期授权与容量及其它25目标/六类实质开发仍继续，outboundIntegrationComplete=false保持。以下较早批次对startup32或“尚无EST逐阶段通道”的描述为历史，不覆盖本节的新合同。

## 356–360：Direct owner回调携带实际原生调用者

[实际对象回调契约](FleetMissionNativeOwnerCallbacks.md)已为12个服务的20处Direct调用首传原QPointer实际QObject，Guided对应19函数签名同步。严格非空/实际对象相等后才作上下文检查，成功返回前再核原服务；DeploymentBindingAllowed只接受真实Deployment或UploadApproval两个合法生产者。本地五处上传/五处共享目标预检查改私有helper，不伪装原生回调；GroundApproval复用上下文显式传真GroundReview peer。

原生回调后QQ/Episode/冻结来源/fullgate、TTL/schema和已有SourcesMatch(actualSelf,bundle)保留；Close不新增outbound-current或在线/地面要求。实际修改12个cc和1个QML，必须同批交接，不回退旧少参数合同；无新类型/资源登记、界面布局或交互变化。

这只是应用接线的一部分：Guided14对象仍未迁移current Episode、调用前后Episode/epoch/服务冻结、shared Role明确身份、全retained modal锁、selected旧观察整组投影与入口仍待开发，outboundIntegrationComplete=false保持。原25目标/六类未完成；只改源码/文档及限定静态审阅，未编译、测试、执行helper/认证/应用/SQL/仿真/设备、改PX4、刷机、更新PR或打包。以下为历史增量。

## 346–350：当前实际 Episode 的原生服务边界

requestReview、acknowledgeReview、approvalContext及_capture.live核实际Deployment/ContentCheck同组，涵盖原Direct owner/物理捕获返回边界；ack冻结借用reviewId后才进入回调。current和revokeReview不受新发送许可封禁。

所有归组门禁都使用原生sticky来源与实际owned配对；双legacy保留原行为，混配/跨组/owner丢失/reparent拒绝，不以账号、ID或binding相同代替。取消/revoke/drain、旧Run/Close和已queue/uncertain事实不按新发送许可删除。完整控制器Epoch/旧观察身份投影与普通Deployment core每包fence仍待开发；当前未注册Lifecycle QML/新部署入口，outboundIntegrationComplete=false。

仅源码修改与限定文本/同行审阅，未编译、测试、执行helper/身份/API/应用/SQL/仿真/设备或刷机，未更新PR或打包。范围与后续见[生命周期说明](FleetMissionDeploymentLifecycle.md)。


## 320–324：逐机冻结观测政策可见性

每机 artifact row 新增 startupTransitionPolicy，直接从本次实际完整 plan member 的 executionProfile 规范化获得，不读取 QML/default/caller 成功标志；无效或缺失政策保持空 map，由界面明确显示未知，不造默认值。该 row 进入完整 reviewArtifact 和原风险指纹/审批上下文，原17字段上下文、原身份/TTL/全部拒绝原因不变。wire 航点载荷可能未变，仍须按完整计划/record/revision/政策重新审批。

现有部署面板只读展示所有最多32个冻结成员的政策：版本、raw gap/阶段时长、全球高度/走廊/速度、v2/v3 local/heading 及 v3 HOME 更新/relative-only 次数和五个米制限幅与原始参考。数值仅显示原生有限数字，不将 null/bool/字符串变0，不作四舍五入或截去风险范围；完整风险限制不跟诊断原因一起折叠。过期/失效状态仍明示，显示不恢复审批。没有新增政策编辑、自动已阅、ARM/Start 按钮或实际发送入口。

UI/UX 规范仅用于明确单位、未知态、只读区别与全文换行，沿用 QGC 原生控件/主题；没有运行界面、视觉、键盘、辅助技术或布局验收。真实地面再审后端仍从新完整 record/profile/readiness 冻结原政策，而不是把本展示或旧已阅状态当新批准。本页新展示不代表地面再审的完整操作界面已开发。

## 当前 239–243：阶段候选与实际目录提供者

每机新增 typed `controlledArmPreconditionsMatch`，全组 `allControlledArmPreconditionsMatch` 另要求真实cache/参数版本/物理源、健康gate及Leader绑定，不把原manual-arm/blocked/第二审批缺口删除或改成ready。它是[Readiness](FleetMissionStartReadiness.md)声明域内的BeforeArm候选，不证明机载canArm、遥控中立、净空、固件认证或飞行许可。

新增 non-QML `nativeFirmwareCatalog()` / `nativeReviewCoordinator()` 返回此实际原生Review捕获的目录及提供者，仅同owner线程/仍live/非busy可取，不能从callerJSON生成或用活着但已被提供者替换的旧目录冒充当前来源。新的[StartupAuthority](FleetMissionStartupAuthority.md)在实际已阅+独立账号依据仍有效时冻结这些实际对象并一次接管身份/全记录/RAW来源，随后不把旧Review.valid当跨ARM授权；旧风险仍历史不可变，原最早30秒也不续期。本批尚无最终StartPermit/实际受控ARM与模式事务，未编译、运行或测试。

## 当前 234–238 契约增量：新的完整机载内容来源

新审阅必须同时接入实际 [StartContentCheck](FleetMissionStartContentCheck.md) 与精确核对 UUID；`requestReview` 现在有九个参数，旧七参数调用不能生成可用审阅。18 字段原生核对上下文绑定本次部署/版本/整组真实对象、完整原始 INT 回读及规范载荷，并通过私有实际来源回调核对。只有 `remainingLifetimeMs` 从稳定比较中排除，完整读取结果不是旧缓存或历史上传回执的别名。

新审阅的截止取“请求审阅起算最多 30 秒”和“最早原机载 READ START 的 30 秒”中更早者；本地时钟只用原生剩余时长和调用前时间作保守换算，不混比两实例绝对单调值。捕获、已阅、独立账号输入和核验都不续期。逐机顺序读取并非机载原子快照、无线请求因果或硬件认证，artifact 显式保留这三类未证明边界和全部原物理/权限风险；仍不是 StartPermit/执行/完成证明。

本次来源被撤销、替换、开始其它读写、缓存改变或最早期限到达后，须显式重新读取、重新审阅，不恢复旧批准。下面 229–233 和更早章节为原契约背景，其中“未实现新鲜读取”已由本段所述的有界原生读取接线替代；真正启动许可、受控状态过渡及完成归属仍未完成。

## 当前 229–233 契约增量

仅原生 `approvalContext(reviewId)` / `approvalContextCurrent(reviewId, context)` 为后续独立条件式人工审批提供 `conditional-human-context`，不是新的已执行或可启动状态。上下文恰好 17 字段：`schemaVersion`、`ok`、`reviewId`、`reviewEpoch`、`reviewGeneration`、`deploymentId`、`deploymentGeneration`、`binding`、`roleSnapshot`、`targetVehicleIds`、`reviewArtifact`、`sourceFingerprint`、`reviewAcknowledged`、`startupPermitGranted`、`executable`、`flightCommandReleased`、`remainingLifetimeMs`。只有实际本对象仍为 acknowledged、原始审阅尚未到期且完整重捕获匹配时才返回；后三个许可/指令字段始终为 false。该接口不是 QML 可写审批来源或可从 JSON 恢复的授权。

原审阅的 30 秒从原 nonce 建立起算，包含捕获、人工已阅确认及后续独立凭据提示的耗时；ack、取上下文、提示成功或核验均不重置期限。`approvalContextCurrent()` 核对原对象、nonce/generation、冻结 artifact 与实际来源，仅在完整比较前去除自然递减的 `remainingLifetimeMs`；仍实时执行原到期检查，不允许旧剩余时长给原审阅续期。到期不自动重审、不自动再次确认或重新审批，必须由操作员重新发起。

`HealthChanged` 不再因例行年龄/收包通知直接撤销，而是重新比较原生健康 gate 指纹以及计划 Leader 的真实注册和原生配置绑定；实际 gate/Leader 风险改变才置 stale。健康源对象更换/销毁、`sourcesChanged` 等真实来源变化仍拒绝；原生捕获发现其他实际 source/epoch/身份或内容变化也立即拒绝本次确认/上下文，不重新定基或延长寿命。

风险比较使用 `Vehicle::flightStateRiskObservation()` v2：保留完整 raw 身份，单独的 `riskContentHash` / 不回绕 `riskContentRevision` 只从 `GLOBAL_POSITION_INT` 风险 value 中去除 `timeBootMs`。Readiness/Progress 先核对 raw hash 与 risk hash，再在风险投影中去除收包计数和 age；实际机位/高度、航向、速度、armed、任务状态、未知和 fresh 状态仍参与，原生风险 revision 保留 A→B→A 变化。Raw v1 的 `timeBootMs`、receiptSequence 和同源启动时钟回退撤销保护不变，重复收包不是新的执行证明。

独立条件式启动意图审批不能代替后续独立 Startup、本次新鲜机载任务内容/执行序号绑定、净空、启动事务和完成归属。进度观测中的 reached 仍是 unbound 序号观测，不能称作 execution proof。这些是当前源码契约；本轮未编译、测试或运行任何业务/API。

`FleetMissionStartReview` 是新的非执行 QObject 后端：从实际运行中的部署实例、最新本地计划记录、原生物理/参数/任务缓存和当前本地角色策略，生成一份独立、短期、不可变的风险快照。它不恢复旧 planning/liveReview 或 grounded staging 审批，也不实现伪 `start() => false` 占位接口。

模块开发阶段仅进行源码与静态检查；本次 229–233 文档同步仅编辑既有 `.md`。没有编译、测试、执行业务 helper、启动软件、仿真、刷机、业务文件 IO、参数/任务写入或 PR 操作。

## 三个真实操作

- `requestReview(deployment, store, coordinator, health, rolePolicy, contentCheck, authorizationOwner, deploymentId, contentCheckId)`：强 cast 实际 C++ 后端，核对原生整组新读取并生成风险 artifact，返回 QVariantMap 收据。
- `acknowledgeReview(reviewId, operatorConfirmed)`：显式确认已读过**这份不可变风险快照**。完整重捕获后，来源/角色/健康/记录/实际对象及 artifact 必须与原份精确一致；不一致置 stale，要求重新审阅，不用新 pose 静默替换旧风险。
- `revokeReview(reviewId)`：撤销本地审阅收据，返回 bool。不撤销已上传机载内容、不清空或回滚任务、不修改 plan/intent/staging/Swarm。

只读 `current` / `changed` 供私有 `MerivusInternal` UI 观察；调用方负责类型注册和 facade 接线。本模块没有启动、解锁、模式切换或执行放行入口。

每次请求产生新 UUID nonce `reviewId` 与递增的十进制 `localGeneration`；counter 达 uint64 上限后拒绝创建，不回绕。有效期从本次本地 nonce 建立起算，使用单调时钟最多 30 秒，包含捕获、ack 和后续独立审批窗口耗时；确认与上下文核验不延长寿命，不自动重建过期收据。状态是 preparing、reviewable、acknowledged、stale、revoked、expired 或 failed。重复、旧 ID、超时或未经显式确认的 ack 拒绝。

## 实际来源与历史 provenance

仅接受 `FleetMissionDeployment::startReviewContext` 原生提供的完整 runtime historical receipts。要求整个冻结目标组均曾成功上传并逐项回读匹配，有正的 write/read operation ID、精确 contentHash/wireSchema/itemCount，以及本运行时单调时间收据。最多 32 架、每架最多 2048 items、整组最多 8192。不能提交 caller 自造的历史、恢复文件或仅部分上传证据。

每架必须取部署实例冻结的实际 `Vehicle*` / `MissionManager*` 及其实际 `ParameterManager*`，并在初末检查同一对象、model ID、线程和归属；所有任务管理器必须 `!inProgress && !transportCallInProgress`。旧历史 read ID 不是当前整表来源，正在传输时不生成可用的新审阅。

旧 binding **只作为 provenance**。直接检查 store.current / record 的最新 sourceTaskId、recordId、recordRevision、planId、内容/审阅证据哈希及 storeEpoch 完整一致，实际记录不是 restored，持有 plan 与冻结部署 plan 相同。`liveReview` 可以已经撤销；不调用 `matchesBinding`、不重新标记 intent approved、不使失败/ARM 后的 staging 恢复 verified。

使用实际 `FleetMissionStartReadiness::capture` 和 `FleetMissionProgressObservation::capture`。前者采当前参数/版本/目录与真实物理入口；后者对照当前 native mission cache，不以历史回读代替实时内容。若 geometry/profile 问题使 Readiness 提前返回错误，仍补取实际 `FleetMissionVehicleEvidence` 指纹，以免“同一个错误值”遮住参数/version 源变化。

artifact 明确区分：`historicalReadbackMatched`（过去的实际上传回读收据）、`currentNativeCacheMatches`（当前原生缓存对照）与 `allFreshOnboardReadsMatched`（独立实际新读取）。前两者不能替代第三种来源，第三种也不是机载原子快照、这次启动归属、真实执行或完成证据。起降净空、完整固件构建认证和启动事务仍未完成。

## 健康与 Leader 要求

调用实际 `health.evaluateTaskGate` 并冻结该健康源、返回值及 fingerprint，ack 时重新验证。Leader 门槛由原冻结请求中的真实 bool `leaderRequired` / `requireLeader` / `requiresLeader` 与实际 planned Leader role 推导；别名类型/冲突拒绝。全 Scout 且没有原 Leader 需求的计划，不新增全局 Leader 门槛。

有 planned Leader 时，单独对照它的具体 ID 是否仍在 actual registry 中，以及 native fault policy 的配置 Leader 是否正是这个 ID；不能让全局 fallback candidate 代替本计划 Leader。`leaderBinding` 只是原生配置对照，`leaderRoleDeliveredOrExecutionVerified=false`，不设置 Leader、不宣称角色已投递。

`allPreconditionsMatch` 的范围固定为 observed-physical-cache-health-not-authorized。健康/参数/物理状态被阻断时仍允许形成可审阅风险 artifact，并显示 blockedReasons；“已阅读风险”与“允许执行任务”是不同动作。

## 本地角色与第二人审批

读取 actual `FleetRolePolicy::actionSnapshot("mission.start.review")`，自己严格检查全部九字段的实际类型：actorId、role、action、allowed、requiresSecondApproval、policyEpoch UUID、policyRevision 十进制、actorAuthenticated=false、executionPermitted=false。空 actor、未知/空 role、权限 false 或策略来源不符拒绝，不用 QVariant 的 bool/数字兼容转换认证字段。

授权 owner 的 DirectConnection 回调必须返回真实 QVariant bool true：

- `fleetStartReviewAuthorizationSourceMatches(actualStartReview, actualPolicyQObject)`：核本窗口真正的私有 policy 实例。
- `fleetStartReviewContextAllowed(actualStartReview, deploymentId, binding)`：核冻结部署/binding、最新非恢复记录与当前任务占用；不能调用旧 liveReview / matchesBinding 路径来提升审批。

QML 回调可运行嵌套事件循环，因此在物理捕获前调用，并在返回后重查 frozen native 依赖；最终捕获后不再调用会偷偷换 pose 的 UI 回调。本地 QML/角色模型不是对恶意进程的安全认证，actorAuthenticated 始终为 false。

若 requiresSecondApproval=true，明确 `requiresAdditionalApproval=true`、`secondApprovalEvidencePresent=false` 并保留独立第二人证据缺口。一个操作员的 bool 只表示已阅，不能冒充两人审批，也不会因为缺第二人就禁止“记录看过风险”这一本地行为。确认收据包含原份 roleSnapshot 与 configured actor ID，不称其为已认证身份。

## 生命周期、失效与显示边界

所有方法限定 GUI/对象所属线程。冻结 QPointer、generation、nonce、原 artifact 与 source capture；回调重入、撤销、对象销毁、source/model 更换或权限撤销均拒绝，不接管同数字 ID 的新 Vehicle。changed 之前全部 state 提交；接收者删除对象或重入后，不访问已销毁状态，也不把上一代收据覆盖到新一代。

角色身份/策略、计划记录/epoch、部署、目录/健康实际源、ARM/flying/version source、参数 pending/ready/missing 和 mission operation/inProgress 等已接线语义信号使本地收据 stale；不盲目监听每个常规 raw heartbeat。例行 `healthChanged` 只在重新核验原健康 gate/具体 Leader 绑定有变化时撤销，不因相同风险的收包或年龄通知重新定基。参数值、实际机位/航向/速度及未知/fresh 边界的变化不保证都产生即时 changed，所以 current 始终标 `evidenceAtCaptureOnly=true`、`revalidationRequired=true`、`snapshotOnly=true`。**30 秒是本地收据寿命，不是源证据保证新鲜 30 秒。** ack 和原生审批上下文核验永远重捕获风险 fingerprint、健康、角色与记录；风险身份变化要求重审，相同风险的重复报文不单凭 receipt 推进使审阅失效。

artifact 不复制完整 plan、payloadBuild、全部参数或整个目录；保留有界每机摘要、原生来源 fingerprint、历史 operation 收据、真实缺口、健康原因与目录身份。内部输入上下文还限定 32 MiB、深度 16、节点 2000000、每层 128 keys、每字符串 4096 字符；公开 artifact 最大 2 MiB，每组/每机理由最多 32 条、单条最多 256 字符。

`startupPermitGranted`、`executable`、`flightCommandReleased`、`executionVerified`、`missionCompleted`、`actorAuthenticated` 永远为 false；reviewMeaning 固定 not-start-authorization。acknowledged 从不改名 approved/ready/executable，也不能被调度器作为启动许可。

后续独立 Startup/dispatcher、启动事务的完整独立授权链、已批准固件构建/二进制认证、本次新鲜机载任务/起始序号绑定、净空与执行完成归属仍不能由此审阅或条件式人工上下文代替；实际编译、仿真、硬件验证与发布放行仍由工程师完成。

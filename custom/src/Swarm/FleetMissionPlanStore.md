# FleetMissionPlanStore 本地提案版本库

## 544：完整保留普通全局路线的任务根声明

计划根闭白名单增加 25 个可选原值字段，与 Compiler 仅在实际展开普通全局路线时保留的任务根字段对应：时间数值族 `timeSeconds/time/timestampSeconds/timestamp/t/etaSeconds/startTime/startAt/endTime/deadline/durationSeconds/missionDurationSeconds/taskDurationSeconds/returnStartTime/returnEndTime/returnDurationSeconds/timeLimitSeconds/returnReserveSeconds`，证据标签 `timeReference/returnTimeReference/timingEvidence/returnTimingEvidence`，以及参考声明 `coordinateSystem/altitudeReference/localFrameId`。缺字段不补默认值，显式零、别名和原值不被本库删改；这些字段随完整 plan 进入原内容摘要与版本记录，不另造归档 schema。

本次只补内容保留，不将本库变为另一套 Compiler。原完整快照检查、16 MiB 单份／32 MiB 档案预算、已有来源 validator、计划／证据摘要、CAS 和惰性历史边界不变；安全快照域内的错误或冲突原值仍可供 blocked 诊断归档，不以存入成功声称时间语义有效、已编码或可执行。一般任务没有完整原 task 保存在本库，不能仅凭同名字段证明来源；有区域／余段／备用出勤来源的记录仍走原中央对应检查。

PayloadBuilder 继续在实际构建时验证计划根时间与参考声明，并将合法时间保存在未编码的 `sourceSchedule`；本库不把任务根字段提升到成员，不改变原逐机、全局包装、约束、任务根之间的消费优先级。旧无字段档案不补造声明、不展开旧路线、不重新计算其 planId，不恢复批准、占用、Run 或飞行权限。仅静态修改和直接读回，未编译、运行、测试或实际读写档案。下方为历史合同。

## 521：schema6 明确门槛进入完整存档

计划根闭白名单新增可选 `minimumSpeedMps`、`minimumEnduranceSeconds`。每个提供值必须为实际非 null 数值、有限且在 `0..9007199254740991`；不接字符串／Bool，不从缺省补零或删除显式零。带区域 `planningSource` 时，中央 Draft validator 返回完整 schema6 task；两个门槛分别要求源 task 与实际 plan 同 presence、同值，并保持原 `requirePerformanceEvidence` Bool presence／值对应。没有门槛的旧 schema1–5 不可旁加计划门槛，schema6 也不可删门槛降级。

保存、导入、读取及区域来源恢复继续共享完整 `validPlan` 和 Draft validator，不新增磁盘格式、预算或文件副作用。合法历史值只是惰性内容，恢复后仍要原完整新审阅／当前证据；门槛或内容 hash 不认证性能、已批准、已上传、已执行或 StartPermit。正常 PayloadBuilder 还独立复核区域源两门槛，不能靠绕过 Store 的直接编码丢失政策。单机／联合余段和独立备用出勤继续用原完整 provenance 消费者，不从历史计划恢复权限。

待工程师验证 schema6 两门槛的缺／零／正值及 strict 三态完整往返、typed-null／字符串／Bool／非有限／越界拒绝、源与 plan 单边删除／改值／旁加拒绝、旧 schema1–5 合法记录保持。全部仅静态修改／对读，未编译、运行、测试或实际保存／导入文件。下方为历史合同。

## 513：备用独立出勤来源保留完整实际计划一致性

计划白名单仅新增可选 `reserveDispatchPlanningSource`，与区域、单机余段、联合余段三个来源四者互斥。提供即须实际非 null Map；经 [ReserveDispatchPlanner](FleetReserveDispatchPlanner.md) 中央 `validateProvenance` 后必须与完整规范 provenance 的紧凑 JSON 相同，再以 `matchesPlan` 核真实成员／完整路线与停留／profile／constraints／边界／性能政策，而不是只凭新来源字段名、summary 或自报 hash 接受。

严格 true 不得从完整新任务／计划投影中剥掉或降为非严格；合法 false／缺省的规范非严格语义不改变原 compiler 版本要求，坏 Bool／null 仍拒。区域 schema5 原 presence 规则单独保持。原存档 schema、内容摘要、CAS、预算、完整安全／边界／净空／profile 校验和保存／导入／读取共用 `validPlan` 的路径不变；新出勤不是区域 Draft 恢复源，不机械加入区域修复／回填路径。

保留来源仅用于新提案与 inert 历史的一致性，不认证原来源、实测性能、执行结果或历史计算。不能恢复旧 Run、批准、占用、控制权或启动许可，仍须实际当前证据与完整新审阅。本批后台接线不表示已经提供 Root／可见出勤入口；未编译、运行、测试或操作设备。下方为历史合同。

## 485：完整区域来源与计划政策严格对应

对带planningSource的计划，中央validator重建完整sourceTask之后，requirePerformanceEvidence在源task和plan.top必须同presence；存在时两边都实际非null Bool并且值相同。不靠toBool、不从缺字段补false，不许schema5 true→plan false／缺键、schema5 false→缺键，或旧1..4源旁加新flag。原compiler0.8.0必须true检查保留，0.7.0合法区域显式false不会被误拒。

同一validPlan用于保存／导入／读取历史的完整性，source5整体保留。原完整几何、clearance对应、计划／证据hash、来源互斥及总预算不变；历史仍inert，字段合法或hash相同不恢复能力批准、性能认证或执行许可。

恢复仍经严格完整planning source校验，新副本重置datum／当前revision／审批。没有新增磁盘格式、SQL或文件执行；32工程场景全未执行，未编译／运行／测试。下方为历史。

## 483：严格性能策略的版本标记不被归档剥离

计划白名单新增可选 `requirePerformanceEvidence`，提供即须实际非null Bool；`compilerVersion="0.8.0"` 必须保true，删flag或改false不能以该版本通过归档合同。false／缺字段的旧计划仍只是原inert历史，不由旧raw数字、旧cap批准或新字段名自动取得当前严格性能结论。

新Compiler的planId身份已经绑定严格算法token及完整声明来源，Coordinator schema3另保每成员完整证据进入原审阅fingerprint。Store不重造工程批准、不认证caller／实测性能、不把旧档回填true，也不靠版本标记恢复权限。原routeBoundaryPolicy、来源匹配、内容hash、CAS及总预算保持；本批没有新磁盘格式、档案写入执行、性能录入器或UI。

待工程师验证0.8缺flag／false／坏类型拒、正常严格计划保留、旧记录不升级，以及所有后续来源变化使旧审阅失效。30项场景全部未执行，未编译／运行／测试；下方为历史。

## 478：完整原始边界策略的严格存档与同源绑定

计划白名单新增 `routeBoundaryPolicy`。只要提供，就须为真实非 null Map，并由 [Planner](FleetAreaCoveragePlanner.md) 的中央严格规范形与原域检查完整接受；不修复、截短或按 summary 补几何。`planVersion=1.1` 或 `compilerVersion=0.7.0` 的新计划必须带此字段，包含显式 inactive。旧无字段计划继续仅作 inert 历史读写；[PayloadBuilder](FleetMissionPayloadBuilder.md) 的当前部署构建对所有版本都要求完整策略，删版本号不能绕过，旧档案也不会因此获得新的边界检查证明或执行资格。

若有区域 `planningSource`，在其完整原生 validator 成功后，从返回的完整 `task.area` 和计划成员重新构建策略，要求与计划策略完整紧凑 JSON 相同。不能把各自独立合法的源区域 A 和计划区域 B 拼在一起，即使已重新计算本地 hash。单机／联合余段分支继续调用中央 `matchesPlan`：新 provenance 必须保留并匹配完整策略，旧 provenance 只配旧缺策略的 inert 历史，不认证原源或恢复权限。

带策略计划的 `coveragePlan` 若含 `holes/holeCount/boundarySetbackMeters` 任一项，三项必须完整：孔为真实非 null List、最多 8 环，计数为实际整数且精确相等，策略必须 active。以策略完整原外环／帧配入该孔／退让，再由中央接口重建规范策略，整份值必须与源策略相同；不能靠摘要重复字段不一致改变后续意义。active 策略且覆盖已生成却三项全删同样拒绝，不依赖可删除的 plannerVersion 标记。规划失败而没有生成路线的覆盖结果不被伪装为成功。

外层存档 schema、完整 plan／evidence 内容摘要、版本 CAS 与快照／总量预算不变。这里验证原几何声明及源一致性，不重复执行完整路线／编码检查，也不把内容 hash、历史记录或策略视为数值认证、可信来源、实际净空、批准、控制权、Run 或启动许可。

工程师待执行（本轮未执行）：新计划缺策略拒绝、旧缺策略保留 inert；主动／非主动规范形与坏输入整份拒绝；合法但不同 Draft 源／策略错配；孔摘要三项删一／全删／错型／计数错／内容或退让替换；新旧余段 provenance 混用；保存／导入／历史完整性读取使用同一规则，不修改原档。未编译、运行、测试或仿真。

## 475：独立联合余段来源的完整历史记录

计划白名单新增 `groupRemainderPlanningSource`；与区域 `planningSource`、旧单机 `remainderPlanningSource` 三者最多一种。实际 `validPlan()` 对新来源复用 [GroupPlanner](FleetMissionGroupRemainderPlanner.md) 的完整中央规范化、原值一致性和 `matchesPlan`，核全部成员与共同政策，不以首机／summary 重建来源，也不接受成功前缀。外层磁盘 schema、准确计划／证据内容哈希、版本及 CAS 不放宽。

原 v1／单成员记录继续旧分支，不转换为联合来源；新 provenance 保留完整原包 ID／内容哈希、原组和新目标有序映射。归档只是 inert 规划历史，哈希不认证来源或现实完成，不恢复旧 Run、占用、批准、控制权或命令许可。当前能力、每机能量、空域和原安全门禁须重新审阅。仅静态源码修改／对读，未编译、执行或测试；下方旧批次保留为历史。

## 473：历史资料余段 provenance 自然进入原版本库

新[余段规划资料包](FleetMissionRemainderArchive.md)及其[显式保存／打开入口](FleetMissionRemainderArchivePanel.md)提供单 Run、单成员的完整原任务／计划／载荷／观察材料；它不是复盘摘要补源，也不是本库恢复原 Run 的接口。原 native18 getter 不变；完整文件经独立校验后产生历史20／schema2，并携原包 ID／内容摘要进入新的余段提案。

`validPlan()` 原有 `remainderPlanningSource` 分支已实际调用 [Planner 中央 validateProvenance／matchesPlan](FleetMissionRemainderPlanner.md)，自然接受 native provenance28／schema1 或 historical provenance30／schema2；历史分支严格保留两项资料包身份，不能冒充原生来源。无需新增计划根字段、放宽本库外层 schema 或绕过完整 plan／evidence 内容哈希、唯一新成员／完整路线／profile／constraints／无备用核对。旧记录不改写，坏来源不删字段后存入；既有区域 `historicalDraftSource()` 合同不因此改变。

完整原政策、包含式原航点和完整停留仍由共享 Planner 保持，原绝对时序拒绝及新当前证据审阅不变。新记录仍是独立规划历史，不恢复旧批准、实际 Run、占用、控制权、执行权限或正式任务结果；权限恢复与命令放行标记保持 false。哈希只核内容一致，不认证历史资料或现实完成。本批仅源码静态核对，未编译、执行或测试；下方旧批次保留为历史。

## 452：输入2／报告3的完整来源匹配

原统一`inspectForPlan`、编辑源／constraints完整JSON一致性和`planningResultMatches`消费者不另设旁路；输入内部schema2通过同一校验与完整内容哈希保存。matcher仅支持明确input1/report2或input2/report3配对，新版重建同一Joint源和ContactModel纯参考几何，核完整contactApplications／contactIssues、组件身份、计数及限制，不再次执行环境solver，不认证历史计算结论或实际触地。

旧资料／旧报告不补尺寸、默认contact、现场认证或新权限。完整plan在报告加入后才计算contentHash；contact只引用原payload项摘要／序号，不引用未来planId／报告hash／最终审阅artifact，避免自循环。导入历史仍不能恢复live审批或飞行许可；所有原资源及CAS门禁保持，未编译、运行或测试。

## 450：可选规划净空结果的完整来源一致性

计划白名单新增planningClearanceReview；只有原constraints确实含完整clearanceInput时才允许该字段。result须真实非null map、exact3 ok／error／report且ok真实非nullBool。成功交由[planningResultMatches](FleetMissionClearanceReview.md)严格核报告schema2／encoded-planning范围、未包含实际启动、输入指纹／完整同序成员／planId／独立basis与完整源视图指纹；它仅重建有界完整编码来源，不重复2M净空或旧1M空间checker。**同源不认证计算通过或现实安全**。

失败结果只允许真实false、非空无控制字符／已修剪／至多128字符的error以及真实非null空report，不保存成功前缀。字段缺失的旧历史继续原规则，不倒推旧任务已做净空计算。原归档整体内容hash包括该结果，导入不修复／改写报告、按当前时间销毁旧档或恢复批准。

原geometry pending保留，新结果不是实际启动许可；完整native源／接地／高程构型对应和最后write仍需实现。本批未编译、测试或运行；下方448及更早为历史。

## 448：可选工程净空输入的完整归档校验

`constraints.clearanceInput` 不新增计划根字段或归档格式。仅当实际非 null constraints map 含该键时，`validPlan()` 调用 [FleetMissionClearanceInput](FleetMissionClearanceInput.md) 的 `inspectForPlan()`，由它执行完整 `normalize()`：必须是完整合法声明、对应全部活动成员的原 ID 和顺序，且原输入与检查返回的完整规范输入具有相同紧凑 JSON。不重复执行同一资料的多边形／树规范化；这里只兼容真实数值宽度的 JSON 等价，不修剪／修复资料，不把备用机或另一构型的 envelope 自动匹配给目标机。

该检查共用于保存、导入和选中历史记录的完整性读取。无新字段的历史记录维持旧规则；本批不扩大其它历史 constraints 类型规则。校验不读取当前时钟，不因资料在今天已经过期而拒绝历史档案，也不认证资料来源、覆盖、时效或几何净空。它保留原声明和摘要，不用规范值回写旧记录。区域规划源通过既有 Draft validator 支持无资料的 schema 1 与有资料的 schema 2，外层仍为五字段。

含 `planningSource` 的记录还独立核对 `source.settings.clearanceInput` 与 `plan.constraints.clearanceInput`：任一存在即要求两边都存在、完整紧凑 JSON 一致。不能让各自合法且重新计算过哈希的源场景 A 与计划场景 B 同存，随后恢复成另一套资料。两边都缺失的旧源仍按原规则；没有区域编辑源的一般任务不新增该配对要求。

有效完整资料在本批 Compiler 中仍有 `clearance_geometry_review_pending` blocking 风险；这种合法计划的 blocked 审阅记录允许归档，不等于批准。坏资料、成员身份不符或 canonical JSON 不等价则拒绝**整份**记录：不保存有效前缀、不删新字段后保存，也不修改原历史版本。编译器的坏值诊断可以显示，但不保证能存入该严格版本库；不安全／超限值仍受原整份快照预算限制。

本批没有增加文件写入入口、实时求解器、当前来源认证、审批恢复或飞行许可。仅修改源码并静态读回，未编译、测试或运行。

## 440：可选原始空域输入形状证据

新的Guided审阅归档增加`reviewEvidence.airspaceInputEvidence`，精确四字段`{schemaVersion:1,noFlyZonesProvided,noFlyZonesListValid,noFlyZonesCount}`。保存、导入和历史完整性检查共用严格类型／键集合验证；真实列表要求provided=true、数值整数count与同份归档noFlyZones长度一致，非列表count必须明确null。未提供只兼容缺字段或旧默认空列表；显式坏型原输入可以记录为invalid，不冒充有效空列表。

新字段随整份reviewEvidence进入原哈希和批准身份，不增加写盘路径、来源认证或飞行权限。旧无此字段的档案继续接受，但旧空列表不能倒推出原来明确提供、数据已加载、覆盖完整或空域安全；显示消费者必须保留legacy-unknown。四字段只是输入形状证据，不验证禁飞区实物来源、时效或区域覆盖。

未编译、运行或测试；未修改原计划schema、历史版本选择或审批恢复边界。

## 433：余段提案的独立历史来源归档

计划白名单新增可选 `remainderPlanningSource`。保存/加载统一调用 [RemainderPlanner](FleetMissionRemainderPlanner.md) 的 exact28 校验与 `matchesPlan()`，要求原值和规范值的JSON一致、实际单成员路线/profile/估算/约束匹配、无备用且assignments等于members。不能同时携带区域编辑源 `planningSource`；不会修补、截断或静默删除坏来源后接受原记录。

完整来源进入原有contentHash；旧无该字段记录保持兼容，新档案须交付本批消费者源码。这里不持有完整原task，不能重算其政策摘要或认证Run所有者；原任务到新提案的检查由实际取源、helper和Compiler完成。导入归档仍只是历史，不恢复审批、控制权、机载任务或进度完成状态。

现有 `historicalDraftSource()` 仍专用于区域编辑源；余段提案不会伪装成可恢复多边形，也不增加从外部报告生成实际Run的入口。未编译、测试或运行。

## 428：跨重启历史版本目录与独立草稿取源

新增两个纯值只读入口，将已归档的完整规划源交给新的独立草稿；不恢复部署、机载任务、旧批准或控制权。下面的入口补上 427 当时未提供的历史取源能力。默认档案仍由既有构造/加载路径读取，这两个 getter 自身不读写文件、不导入档案、不发送信号、不修改 `lastError`、live 标记或写入隔离状态，也不要求飞机在线。

- `historyVersions(sourceTaskId, expectedStoreEpoch)`：精确核对本实例当前 epoch，返回指定 source 的全部历史版本，按 revision 降序，不借 `current()` 的最新版本代替旧版本。每 source 最多 128 条；先检查有界元数据，再按紧凑 JSON 信封及每条元数据的实际字节数累计，目录总计最多 256 KiB，超限整份拒绝、不截断。目录不复制 plan、reviewEvidence 或完整源。
- `historicalDraftSource(expectedBinding)`：只接受精确 7 字段选择，从私有不可变记录中找到明确正 revision，不接受 `0/latest`。核对原记录身份、原 plan/evidence 合法性、两个内容摘要、planId 及完整 `planningSource`；成功返回规范源的独立值副本。旧无源记录仍可以列出，但报 `history_planning_source_not_recorded`，不能从编译路线猜造原规划输入。

目录成功返回精确 10 字段：`ok=true`、`schemaVersion=1`、`storeEpoch`、`sourceTaskId`、`versions`、`versionCount`、`allVersionsForSource=true`、`historicalOnly=true`、`flightCommandReleased=false`、`executable=false`。

每个版本行精确 10 字段：`storeEpoch/sourceTaskId/recordId/recordRevision/planId/contentHash/reviewEvidenceHash` 这 7 个绑定字段，加上 `recordedAtUtc/restoredFromArchive/planningSourcePresent`。`planningSourcePresent` 仅表示字段存在，不是完整源已验证、可恢复批准或可执行的证明。`sourceTaskId/recordId` 沿用原档案的非空有界 identifier 规则，不强制 UUID；revision 接受 JSON 恢复的真实整数数值，不接受数字字符串、布尔值或零。历史输入不会因为刚追加了另一个版本而失效；重新导入/加载更换 epoch 后，旧选择必须重新取得。

取源成功返回精确 10 字段：`ok=true`、`schemaVersion=1`、`binding`（精确上述 7 字段）、`planningSource`（427 完整 5 字段源）、`restoredFromArchive`、`historicalOnly=true`、`manualConfirmationRequired=true`、`approvalRestored=false`、`flightCommandReleased=false`、`executable=false`。目录和取源的拒绝均为精确 4 字段：`ok=false/error/flightCommandReleased=false/executable=false`。返回 schema/count 为本地整数；绑定 revision 规范为精确范围内整数，源数值由 `FleetMissionDraft::validatePlanningSource` 规范化，接受原生和 JSON 数值宽度的等价形式，不修复坏源。

稳定错误包括 `history_wrong_thread`、`history_invalid_selection`、`history_stale_store_epoch`、`history_source_not_found`、`history_directory_budget_exceeded`、`history_invalid_record`、`history_stale_record_binding`、`history_record_not_found`、`history_planning_source_not_recorded`、`history_invalid_record_integrity`、`history_invalid_planning_source`。两个入口均要求对象所属线程。完整内容校验只对选中记录进行；源非法也可先因 plan 完整性校验失败而返回 `history_invalid_record_integrity`。

恢复记录的 `restoredFromArchive=true` 是真实历史标记，绝不改成 live。这里刻意不调用 `matchesBinding()`：它要求最新且仍 live 的批准候选，不能用来读取跨重启历史源。`writeBlocked=true` 时仍允许读取已成功加载的内存历史，但不会清除隔离，也不保证新草稿后续能够归档。Root 应新建独立 Draft、明确人工确认、重新确认地图基准并重新预览/审阅；不得将旧 sourceTaskId、旧审阅或历史取源回执当作新的执行身份/批准。源码已实现，运行正确性、跨重启交互及资源边界仍须工程师验证。

## 427：可选完整草稿源的归档

计划白名单新增`planningSource`，使用[FleetMissionDraft](FleetMissionDraft.md)的完整有界校验，原源与规范源的JSON必须一致。只有真实数值宽度的JSON归一化被接受，不把非法来源修成有效记录、不丢弃额外成员字段。源随整个plan进入既有contentHash，存档加载也应用同一校验；原schema和旧无源记录保持兼容。旧程序不认识此字段时可能拒绝新档案，交接须同时包含本批消费者源码。

这不是计划重编译或planId认证：源与实际八个规划字段的对应关系由原[Compiler](FleetMissionCompiler.md)建立，本库只核合法完整源及整份归档完整性。恢复档案仍是历史，不能直接恢复liveReview、部署、426核对请求或执行批准。427 修复入口只读取实际失败部署冻结的原记录；跨重启的历史取源入口在上方 428 节新增。

本模块保存完整编译计划和有界审阅身份，供历史查阅、交接引用和工程师复盘。它不上传 Mission、不释放命令、不恢复批准，也不是账号认证、跨站控制锁或已执行任务数据库。

2026-10-14：计划根白名单允许可选规范 `executionProfile`，根及 `members/assignments` 里存在该字段时共用 `FleetMissionExecutionProfile` 严格复核，原值的规范 JSON 必须完全一致，不静默补默认或丢字段。旧无 profile 的计划继续兼容；非法/空 profile 不保存为可用版本，错误保留为归档拒绝，不能沿用旧批准。显式驻留和完整 profile 已由编译器进入 planId，存档 contentHash 另覆盖整个实际计划；恢复 profile 仍只恢复历史，不恢复启动权限或参数认证。

## 身份与版本

`FleetIntentTask.taskId` 是该对象创建时生成的固定 UUID，不是 `SwarmMissionOrchestrator.createTask()` 的执行任务 UUID。本库的 `sourceTaskId` 因此代表本地提案会话。每次 `prepareFleetIntent()` 都创建一个新的不可变审阅版本，包括计划内容相同的情况；机队/健康/禁飞区等审阅证据可以独立变化。

一条记录保存：

- `sourceTaskId`、compiler `planId`、独立 UUID `recordId` 和连续 `recordRevision`。
- 完整 `plan` 与独立 `reviewEvidence`。
- `contentHash`、`reviewEvidenceHash` 两个 SHA-256 完整性摘要。
- `recordedAtUtc`，使用规范 UTC ISO-8601 毫秒字符串。

`planId` 是原编译器的输入摘要，本库不能仅从输出计划重算它。`contentHash` 是存档计划的 JSON 值摘要。两者都不是数字签名或真实安全证据。

`recordReview(sourceTaskId, plan, reviewEvidence, expectedRevision)` 使用明确 CAS：新 source 仅接受 0，已有 source 必须给出当前 revision。成功先持久化再更新内存，失败不创建部分记录；每次成功递增版本，不按 planId 去重。`current()` 返回当前引用摘要，`record()` 返回指定历史版本的完整值快照。

## 持久化与资源边界

默认路径为 `QStandardPaths::AppDataLocation/fleet-os/mission-plans-v1.json`。构造时只读取历史，不恢复 live/approved 会话。保存使用 `QSaveFile`，关闭直接覆盖回退；合作本地写入者先取得 `QLockFile`，再核对磁盘基线 hash，陈旧实例不得原子覆盖另一实例刚保存的记录。

繁忙、基线冲突或文件失败都有明确错误。原文件损坏/未知版本时隔离写入，不能用空集合覆盖原档案；工程师应先保留并诊断原件，再显式恢复。任意 backend 导入只建立历史视图并阻止默认写入，成功重新读取默认档案后才能继续写入。导出不能绕过默认档案的锁/基线门禁。

上限：单份 plan/evidence 各 16 MiB、512000 节点、16 层；档案总计 32 MiB、256 个 source、每 source 128 个版本、2048 条总记录。未知根/记录字段、自定义 QVariant、非有限值和超过 JSON 精确范围的 64 位整数拒绝；拒绝损坏 UTF-8、读尺寸变化以及不连续 revision。超限不截断路线、不删除旧版本，返回阻断。长期轮换、压缩、数据库和保留策略仍待开发，这不是完整长期运营存储。

## 批准与信号边界

公共 QML 只得到 `missionPlanArchive` 只读 facade，不得到 record/import/load 可变对象；历史选择由 Root 保留精确绑定，取出的源仅供新的独立草稿。内部准备流程保存 plan、健康/机队指纹、空域/能量/风险、禁飞区与去掉事件树的 options；不会再复制巨大 report/events。存档失败或绑定被替换时不能沿用旧批准凭据。

`matchesBinding()` 核对 record ID、revision、两个 hash、planId、运行期 `storeEpoch` 和 live 标记。加载档案清除 live 标记并换 epoch，不能恢复之前的批准。每次调用前锁定输入/审阅世代；直接信号重入造成新提案时，旧外层调用不能借用新世代，也不能撤销新版本。

批准请求、批准、拒绝、取消的黑匣子与证据回执包含精确版本引用。请求/批准在状态信号返回后再次核对版本和状态；旧拒绝/取消只 CAS 撤销旧记录，不修改更新提案。健康/机队/审阅身份失效也会将旧 `awaiting-approval/approved` 视图降回待审阅，而不是保留可见的陈旧批准。

交接仅在请求 taskId 精确等于 sourceTaskId 时附加历史计划引用；不匹配的执行任务明确没有计划引用。引用标明 `approvalTransferred=false`、未认证/未签名、无控制权转移。接受交接记录不等于批准该计划。

## 工程师后续验证（本轮未执行）

- 同 planId、不同健康/禁飞区输入产生不同审阅版本及回执；旧 CAS 不得覆盖新版本。
- 在 review/state/archive 信号里重入准备、批准、拒绝、取消和导入，旧调用不得污染新版本。
- 重启/历史导入后记录只能作为历史查阅或新草稿来源，必须重新准备审阅才能申请批准。
- 历史目录覆盖同 source 的全部版本而非仅最新项；128 版本与 256 KiB 目录边界、旧非 UUID identifier、JSON 数值类型、旧无源/坏源、revision 0、旧 epoch 与错 hash 均按契约处理。
- 已选旧版本在追加新版本后仍可独立取源；导入/重载后旧选择失效。写入隔离时历史读取不清隔离、不承诺随后归档成功。
- getter 成败都不发送 archiveChanged、不改变 lastError/liveReview/原记录；新草稿重新确认地图基准，不能携带历史批准或覆盖原档案。
- 两实例写入同档案、锁繁忙、磁盘变化、读写失败、损坏 UTF-8/整数/版本/时间、容量边界均如实阻断；原件保留。
- 大覆盖路线完整存档或明确失败，不静默截断；批准回执和交接引用能关联到具体不可变版本。
- 确认本模块不触发任务上传、机载任务恢复、飞行命令或跨站控制权变更。

# 单页历史 GNSS 同包质量诊断

## 523：完整值校验、独立运营留存与显式重开地图

新增 `reportValid(report)`，只接受成功、已确认位置的完整33／7／3／18报告；失败报告不是可留存记录。闭键、真实非 null Bool／String／Map／List、有限数字、计数和分组、序号、fix／坐标／unknown／启发关系全部检查后才返回 true；沿原完整页校验拒绝 `footerPresent=true` 同时 `partialTail=true` 的矛盾覆盖状态。整数允许原生整数或 JSON 解析的有限整值 Double；接收 UTC 不超过 JS exact，页序号保正／非负 qint64 十进制字符串，sourceSequence 保完整正 uint64 字符串。按同 epoch 还保持单vehicle和原样本源序号严格递增，不把大序号转成 Double。

`reportsValid(reports)` 进一步整份检查最多32报告／2048 GPS样本。空集合合法；重复页键为 `runId/headerHash/afterSequence/nextSequence`，重复 GPS 样本键为 `runId/headerHash/sequence`。出现任一重叠则整份拒绝，不拼前缀。`fileHash` 会随原 capture 文件增长而改变，不能用来绕过同页／同样本去重；queryId、catalogId、报告 token 也不是去重依据。报告原 identity7、sourceEpoch、component、原时间和所有未定位诊断完整保留，不按数字机号拼成一架物理飞机。

以上两个入口是可在一次性文件worker内复用的**纯值一致性**检查，不读取原始 payload、当前 Archive 或任何权限，也不能认证文件、作者、CRC、设备、位置精度、物理身份、任务归属或执行结果。文件中可自洽的报告仍只是未认证历史值。

实际追加只能通过 [OperationalMemory](FleetOperationalMemory.md) 的 `appendGnssHistory(actualArchive,token,true)`：由原 Archive 的 native-only `nativeGnssReview(token,true)` 读取当前真实原页报告。该 getter 要求本次 token、当前性、非同步 local action busy，并再做完整 reportValid；public map／导入文件／caller token+报告值都不能替代原生读取。追加保存在独立集合，旧 LegacyMemory 记录／热点／KPI／trim／schema1文件入口不变。

运营集合的显式冻结、手选文件保存与独立只读重开不恢复 live 源。一次成功 `report` worker 结果并实际 finished 后，原 Memory 从私有 cache 精确 index 安装完整报告＋新 UUID token＋snapshot binding5；新业务、取消、清槽或退出使旧选择退休，不自动复活。新追加使旧冻结 descriptor.sourceCurrent=false 只是一条历史提示，不抹旧快照内容，也不升级授权。

当前 Root 地图投影已由522的历史 exact4 改为 exact5 `{active,selectionToken,reviewToken,report,origin}`；origin 分别是 `native-page`、`session-snapshot`、`file-history`。原页与 Memory 两类读取各绑自己的实际 QObject、原 token 和完整报告；Memory另核 `{snapshotId,contentHash,sourceFileHash,origin,reportIndex}`。互相的 token 通知不撤另一类新选择。无 goto／自动居中／航迹／飞行接口；历史文件/hash不证明来源或持续有效位置。

本轮只修改源码并直接静读／核对普通文本，未编译、运行项目／parser／helper／hash／QML／测试、仿真或设备。需工程师验证 JSON Double 往返、全 uint64、0／32／33报告、2048／超量、同页重读／capture增长换fileHash／重叠样本整份拒、空GPS重复、sourceEpoch／component隔离、双终结任意次序、取消／关机与保存实际commit事实，以及文件重开后显式选图不会恢复旧 live 来源。下方522章节为单页实现和历史地图合同。

## 522：真实 GPS_RAW_INT 报告值供显式地图显示

`FleetTelemetryGnssReview::review(nativePage, positionValuesConfirmed)` 是一个纯值、无 QObject 的原生入口。仅接受最多64条的完整原历史页；先检查真实非 null List 的条数，再恰好调用一次既有 `FleetTelemetryMotionReview::review(page,true)`，复用其完整页、Header／Footer、sources、29字段行、原始 Codec 字节、序号、同意和预算校验。只消费该检查的 `ok` 与 `rowCount`，不采用或拼接任何运动指标，不修改 MotionReview 接受域。整页失败没有成功样本前缀；没有 GPS_RAW_INT 的合法页仍是成功空报告，不意味没有故障。

实际 Archive 保留原生页和来源资格；本纯值接口本身不能证明调用者、原生所有权、文件真实性或设备身份。QML public page 的脱敏行不含 payload，不能重建这个入口的输入。原 Codec／Reader／Worker、存档格式、录制白名单、维护记录和 LegacyMemory 不变。

## 固定返回值

top exact33：

`schemaVersion, source, ok, reason, positionValuesConfirmed, sourceIdentity, rowCount, gnssSampleCount, locatedSampleCount, unlocatedSampleCount, qualityCautionSampleCount, qualityUnknownSampleCount, withinThresholdSampleCount, groupCount, samples, displayPolicy, hasMore, footerPresent, partialTail, historicalOnly, singlePageOnly, historyComplete, rawPayloadExposed, absoluteCoordinatesExposed, authenticated, crcVerified, accuracyVerified, taskOwnershipVerified, driftDiagnosed, trajectorySafetyVerified, executionPermit, flightCommandReleased, permissionsRestored`。

schemaVersion 为原生 Int1，source 固定 `merivus-fleet-telemetry-gnss-review`。成功 ok=true、reason 为非 null 空字符串。所有计数为 Int0..64；`located+unlocated=gnssSampleCount=samples.size`，caution／unknown／within 三计数之和严格等于 located，仅对可投点的行计数。groupCount 是 GPS 样本按实际 `sourceEpoch/sysid/compid` 三元组的本页首次出现分组数，既不跨页延续，也不认证物理飞机。

sourceIdentity exact7 为原页非 null 字符串：`queryId, catalogId, runId, headerHash, fileHash, afterSequence, nextSequence`。页序号为 canonical 非负有符号64位十进制字符串；query／catalog／run 和两个 hash 保原值。`hasMore/footerPresent/partialTail` 保原页实际 Bool，不把已验证完整行前缀当完整历史。

`historicalOnly/singlePageOnly` 固定 true；`historyComplete/rawPayloadExposed` 及所有认证、精度、归属、漂移、安全和许可旗标固定 false。`absoluteCoordinatesExposed` 严格等于 `locatedSampleCount>0`。失败仍返回同33字段，但 sourceIdentity={}、samples=[]、所有计数0、页三个状态 false、absoluteCoordinatesExposed=false；不泄漏未通过完整检查的样本。固定 displayPolicy 在失败时也保留，仅描述本算法。

displayPolicy exact3：`maximumHdop` 为 Double2.5，`minimumSatellites` 为 Int6，`semantics="display-heuristic-not-accuracy"`。这只是界面显示启发式，不是行业通用故障阈值、精度区间、净空或事故归因。

## 每个样本 exact18

`sequence, groupOrdinal, vehicleId, componentId, receivedUtcMs, fixType, positionKnown, coordinate, positionReason, hdopKnown, hdop, satellitesKnown, satellitesVisible, qualityCode, hdopCaution, satelliteCaution, sourceEpoch, sourceSequence`。

- sequence 是原页正 canonical qint64 十进制字符串；sourceSequence 是原正 canonical uint64 十进制字符串，不转换为可能丢精度的 JS Number。sourceEpoch 为原 UUID。groupOrdinal 为本页三元组首次出现顺序的 Int1..64。
- vehicleId／componentId 为原 sysid／compid 的 Int1..255；receivedUtcMs 为原本地接收 UTC 的正 qint64，范围不超过9007199254740991。不是远端统一物理时钟，也不从 `time_usec` 猜测启动时刻。
- 只解码消息24 `GPS_RAW_INT`；fixType 是原 Int0..255。只有3／4／5／6／8且同包原 E7 经纬度范围合法才 positionKnown=true，并提供 exact2 coordinate `{latitude,longitude}` Double，范围分别为[-90,90]／[-180,180]。合法 `(0,0)` 不被当未知；未知或拒绝位置用空 Map，不补 `(0,0)`，也不沿用另一个包的旧坐标。fix0／1／2／7／>8一律不投点。
- positionReason 闭集：`reported-position`、`no-gps`、`no-fix`、`fix-2d-not-used`、`static-fix-not-used`、`unknown-fix`、`coordinates-out-of-range`。2D／STATIC 不被本飞行质量图层采用，不代表原设备没有任何位置意义。
- 原 `eph` 是 HDOP×100，**不是米级误差或精度半径**。65535才未知，输出 hdopKnown=false／hdop=-1；其余输出实际 eph/100（含0），范围0..655.34。原 satellites_visible=255才未知，输出 satellitesKnown=false／satellitesVisible=-1；其余为实际 Int0..254（含0）。
- `hdopCaution=(hdopKnown && hdop>2.5)`，`satelliteCaution=(satellitesKnown && satellitesVisible<6)`。这两个 Bool 对未投点行也按同包已知数值保留；不能把 unknown 的−1送入阈值计算。
- qualityCode 闭集：未投点为 `unlocated`；定位行只要任一 caution 即 `caution`；否则任一指标 unknown 则 `unknown`；两者已知且未触阈值才 `within-display-thresholds`，不称“安全／已认证正常”。即使 caution 为false，指标未知也不成为阈值内结论。

MAVLink2 合法短 payload 只在本地零初始化的 message 里拷原 length 做标准 trailing-zero 解码；原始档案字节不被扩展、重打包、修复或认证。不使用可能因扩展截短而默认0的 h_acc 等字段推断精度，不拼 GLOBAL_POSITION_INT、GPS2_RAW、电池或其它消息的位置／质量，不插值、不画连续航迹，不检测漂移或正式任务结果。

## Archive 归属与独立选择令牌

`requestLocalHistoryAction` 新增动作 `gnss-review`，沿原 exact6 intent 和 receipt11；固定本地 catalog、当前双终结成功页、原私有 cursor／vehicle 筛选及 includePositionValues=true 完全匹配才消费原 nativeHistoryPage。另一次明确同意只允许这次位置值派生，不自动录制、读取下一页、重试或选择地图。

只读 `gnssReview` 返回当前完整报告；独立只读 `gnssReviewToken` 返回该成功原生发布的 UUID。每次真实成功审阅即使同页同值也产生新 token；报告 top33 不增 token。失败／退休同时清报告和 token。新查询、取消、接纳新 catalog、实际新 capture、退出均退休；getter还要求原固定目录／策略、latest／last／页7身份、单槽已释放及双终结、无 capture／reader忙。自身报告发布期间的 action fence 不被误当成异步 reader忙，否则 changed 回调无法读取刚发布结果。

安装 report/token 后只发原 changed，并核原 QPointer、serial、latest、原完整页、capture状态及 report/token 未被通知回调取代。返回失败／结局不明不回撤回调的新状态。原 `review` 仍为 Motion／dossier 通道，生成 GNSS 不清它们；生成 Motion 也不借此重建 GNSS 来源或旧地图选择。后续显式地图选择必须使用准确的实际 token，不从同 query／同内容重建资格。

这不是持久跨任务地理数据库、当前飞机位置、事故地图、完整遥测、安全证书或授权。可见层必须显式启用，原始记录未定位只显示计数，不应自动居中／跳转或借地图点击发命令。

## 522历史：Root 显式地图选择与只读投影（523已增加origin第五字段）

沿现有 `telemetryArchive` facade 增加只读 `gnssReview`、`gnssReviewToken`、`gnssMap`。`selectGnssMap(reviewToken,confirmed,revocation)` 只接受准确原生报告 UUID、实际 true 和同引用 plain `{cancelled:false}`；撤销对象只能退休本次显示，不提供位置、来源、账号、任务或飞行授权。Root自行取得实际 Archive 和当前报告，并保原owner、完整report JSON、query、reviewToken及不回绕选择serial；不接受外来点数组或报告作为事实。

选择回执 exact5 为 `{accepted,selectionToken,reason,readOnly:true,historicalOnly:true}`。成功选择令牌为 `reviewToken:serial`，serial从1开始且不大于9007199254740991；它不是UUID。`clearGnssMap(selectionToken)` 只清匹配的当前选择，撤销可在选择通知重入期间生效；旧页面的迟回或关闭不能清另一页面新代次。选择在读取caller撤销引用前先取得本地门锁，访问器外调后核plain lifetime、原gate／Archive及调用前原选择槽未被换掉；clear先退休准确原选择再只写旧引用，不在未知setter后访问已死Root。新历史action、原报告token变化与面板退场均退休原选择；失败不恢复旧选择，也不自动选择共享最新结果。

`gnssMap`正常为 exact4 `{active,selectionToken,reviewToken,report}`，未选时false／空字符串／空报告。每次当前性核对原native token及完整报告JSON，不以同query或同内容重新建立旧选择。公共report先从私有冻结字符串分离，再以固定深度冻结每个coordinate／sample、samples列表、sourceIdentity、displayPolicy、report及外层投影；普通readonly var不等于嵌套不可变。`Object.freeze`缺失或异常返回null，消费者必须保守判空而非绘制。没有运行此实现或确认目标引擎支持，留工程师验证。

可见地图只绘同页离散点及文字质量图例；不开精度圆、连续航迹或风险区域，不自动center／fit／Goto，也不修改全图原控制权限。合法极区坐标保在原报告，常见Mercator底图只明确省略|纬度|>85.05112878°的点并显示数量，不夹紧坐标；标记及标签吞所有鼠标按钮与双击，避免穿透原地图飞行动作。图层隐藏／禁用／PiP／销毁时只退休原选择。

## 尚需工程师执行的场景

本轮只有 apply_patch、直接源码／合同静读和普通文本检查；未编译、运行项目／helper／解析器／模型／QML／hash实现、测试、仿真或操作设备。需验证：0／64／65行及无GPS页；整页其它坏行无成功前缀；fix各枚举和未知值、旧坐标但无fix、越界和真实(0,0)、极区；eph65535／0及卫星255／0、阈值相等／两侧和部分未知；MAVLink1／2短尾包；同sequence/sysid/compid/epoch及全源序号对应；独立确认、token重算A→B→A、发布通知内新读／capture／销毁；cancel、双终结两种顺序和不同查询／筛选／位置同意不能恢复旧选择；实际地图仅显式单页点且无自动居中、goto、航迹或飞行副作用。

# FleetMissionReadbackDiff：普通任务回读的有限差异诊断

## 536：匹配也保留实际朝向读回数值

完整输入仍每侧最多2048项；超限保持未知、不扫描成功前缀。在有界输入中，任一侧存在显式NAV朝向时报告为schemaVersion2／exact29，原28键外仅增加 `headingObservations`；无显式heading的schema1／exact28不变。原sourceRepresentation和全部权限false不变。

新增Map恰5键 `{totalCount,displayedCount,truncated,complete,rows}`，完整扫描候选索引后计数，只展示按sequence升序的前64条。complete仅表示有界候选均已计数，不表示值合法、所有明细都显示或实际动作已执行；truncated明确说明其余未展示。每条恰9键 `{sequenceNumber,expectedEncoded,actualRaw,actualState,canonical,canonicalKnown,absoluteErrorBoundDegrees,error,semanticMatch}`。三个值为有限数字或显式null；expectedEncoded来自上传items的tick，未量化原请求另在payload.headingEncoding。actualState为finite／system-default／missing／invalid；actualRaw保原读回值，不重写为tick。canonicalKnown只在共享helper成功时为true；明确界为0.0006866455078125度；实际归一值另存canonical。

error闭集为明确空串、unsupported_wire_parameter、heading-observation-missing、heading-observation-system-default、heading-expected-unavailable、heading-semantic-different。semanticMatch须双方严格规范值同tick；没有真实观测或坏值不补0。直接诊断调用中的坏期望有限param4仍保在expectedEncoded供显示，字段名不证明它是合法tick；只有semanticMatch的严格检查能信用相同值。新comparisonBasis为 `canonical-heading-tick-v3`，旧canonical-v2保持；contentHash代表所选完整语义域而不是原上传字节或真实姿态。即使整份matched，也保留原actualRaw和舍入后的canonical供显示。

新heading观察的Double原值只要求finite，不套普通差异字段的2^53−1整数显示界：任意Float32到Double仍精确，因此非法大角度如1e20保actualRaw／finite及明确错误，而canonicalKnown仍false；不能将其当合法角度或发送资格。非Double整数carrier仍限原JSexact域，普通旧scalar接受域不改。NaN／Inf或非数值不补有限值，记null及对应状态。原字节和正负零位身份仍由原RAW指纹保留，不以JSON数字代替。

`summary()` 仍删除普通差异rows：旧摘要27键保持，新摘要28键额外保经闭键／类型／界／计数和数值关系核验的headingObservations，最多64条；不复制任意嵌套数据。原差异计数／截断／RAW与普通解码区分不变。仅源码及直接静态／普通文本核对，未执行比较器、helper、parser、算法、编译、测试、仿真或业务IO。下方为历史。

第 425 批新增的纯值 helper，仅比较调用方已经取得的期望原始任务项与实际普通 MissionItem 解码项。不拥有 Vehicle/MissionManager/链路，不发 READ/上传/清空/启动命令，不改变既有部署匹配判定，也不构成独立来源或权限证据。

## 输入和实际语义

`compare(expectedRawItems, observedDecodedItems)` 接收两个 QVariantList。期望值应为现有 payload 的 `items`（上传原始参数），**不是**已经规范化的 `canonicalWireItems`；实际值为真实普通任务回读完成后逐 MissionItem 提取的 13 个已知字段。调用方必须先证明本次回读完成及其真实对象/操作身份；helper 无从验证这一点。

空 observed list 仅代表调用方确实观察到完整回读数量为零；没有采集、协议失败或来源丢失不能用空列表冒充零项结果，应保留无诊断或单独的采集失败。列表中的无效 QVariant 可表示实际取得的 null MissionItem；这会得到 unknown，不伪造匹配。

每侧最多处理 2048 项。超限立即返回 unknown/none，保留实际列表 size，所有槽计入 uncompared，不遍历超限内容。每项只接受 bounded map；规范化前拒绝非 map、null map、超过 16 字段及 typed-null 值，避免其隐式转为零/false。明确无类型 null 的 geographic yaw 仍遵从 Builder 原规则。

双方分别调用现有 [PayloadBuilder](FleetMissionPayloadBuilder.h) 的 `canonicalWireItems(expected,false)` 与 `canonicalWireItems(observed,true)`，不复制或改写其坐标截断/回读四舍五入、float32 量化、支持命令及 null yaw 规则。只有双方非空、规范化成功、完整 canonical 项与 contentHash 都相同才是 matched。双方有效但不同为 different；期望有效而实际完整零项也可明确 different。其它非 canonical 情况保留 unknown，即使计数或某些原始字段已有可证明差异。

固定 `sourceRepresentation="ordinary-decoded-mission-items-not-raw-int-proof"`。现普通读取可能经 FLOAT 回退、再变为 MissionItem double；规范化匹配不证明接收过 RAW INT，不证明当前仍未变化、不证明来源认证，更不是可启动或已飞完。

## 报告：恰好 28 键

- format=`merivus-mission-readback-diff`、schemaVersion=1、sourceRepresentation（上述固定值）。
- state：matched/different/unknown。
- comparisonBasis：canonical-v2/raw-fields-only/none。
- reason：canonical-match/canonical-different/expected-canonical-invalid/observed-canonical-invalid/observed-empty/input-item-limit。
- expectedItemCount、observedItemCount：实际 Qt Int 列表长度，0..INT_MAX，不用 -1。
- expectedCanonicalValid、observedCanonicalValid：实际 bool。
- expectedContentHash、observedContentHash：有效规范化才有 64 位小写 SHA-256，否则明确空字符串。
- expectedErrorCode、observedErrorCode：成功为空；失败只保留 wire_resource_limit、invalid_wire_item、unsupported_wire_command、invalid_wire_parameter、unsupported_wire_parameter、invalid_wire_coordinate 或 diagnostic-canonical-unavailable，不输出自由错误文本。
- expectedErrorItemIndex、observedErrorItemIndex：0 基错误项下标，0..2047；无具体项为 -1。
- comparedItemCount、uncomparedItemCount：完全可比较槽／含未比较内容的槽，和为 max(两侧项数)。正常各 0..2048；超限时前者零，后者为实际较大项数。单侧缺失项的“存在／不存在”属于可比较槽，不声称已读取该缺失项字段。
- differenceCount、differenceCountComplete：有限诊断项总数和是否为完整 canonical 差异计数。最多 34817（1 个数量诊断 + 2048 × 17 个固定字段）；raw-fields-only 的计数始终标为不完整，不作为任务身份差异全集。
- displayedDifferenceCount、rowsTruncated、rows：最多 64 条，显示数量等于 rows.size，截断表示 differenceCount 大于显示数量。继续扫描全部有界项以计算总数，不把 64 行显示上限当比较终点。
- historicalOnly=true、snapshotOnly=true、currentOnboardContentsProven=false、startupPermitGranted=false、flightCommandReleased=false。

非 canonical 路径尽量比较已知 raw 字段，不把不支持值彼此都投影为 null 后当作一致；不可比较值或额外未知字段使对应槽进入 uncompared。unknown 可有差异行，但“零差异行”不等于匹配。比较计数是诊断项数量，不是受影响航点数量：数量变化既有一条 itemCount 汇总，也可有逐项存在差异。

## 差异行：每项恰好 9 键

`itemIndex`、`field`、`differenceKind`、`expectedState`、`observedState`、`expectedType`、`observedType`、`expectedValue`、`observedValue`。

itemIndex 为两侧列表对齐的 0 基比较位置，不冒充实际项中的 sequenceNumber；实际协议序号不符会作为 sequenceNumber 字段差异显示。仅总数量行使用 -1。field 是固定集合：itemCount、item、sequenceNumber、command、frame、autoContinue、isCurrentItem、param1..param7、yawMode、latitudeE7、longitudeE7、param5Integer、param6Integer。differenceKind 只允许 count/item-presence/state/type/value。

State 只允许 present/missing/null/typed-null/invalid-number/unsupported-value/item-missing/item-invalid。Type 只允许 invalid/bool/int/uint/int64/uint64/double/string/map/list/other。missing 与明确 null、typed-null 与实际 false/zero、整数与 double 的 raw 类型不同均单独保留；规范化路径只比较 Builder 规范化后的类型，不把 raw 表示差异误判为既有任务身份差异。

Value 只输出有限数值（绝对值不超过 2^53-1）、bool、三种已知 yaw 字符串 system-default/not-applicable/unsupported-command 或无类型 null。字符串不截取后冒充原值，不输出自由字符串、未知字段名、QObject、嵌套 map/list 或非有限数。需要结合 State/Type 解读 null：field=item 时 present+map 等仅表示项存在，仍不复制整项内容；缺失、明确 null、typed-null、不受支持或非法数值也都以 null 携带其不同 State/Type。

规范化将运行中的 isCurrentItem 排除在既有身份之外，本 helper 不改变这一规则；raw-fields-only 可能展示该字段的诊断差异，但不能据此反向建立 canonical 身份。

## 历史摘要

`summary(diagnostic)` 仅接受完整 28 键报告的有界标量合同，返回去掉 rows 的 27 键摘要；拒绝无效 schema/类型/闭集文本/hash/计数关系/非惯性标记。不会复制或重新解释 rows 中的任意字段。displayedDifferenceCount/rowsTruncated 保留当时报告事实，但摘要不再携带那些行。调用方在原最多 32 条部署历史中使用摘要，避免逐项明细乘数放大。

摘要不恢复权限，也不重新读取飞机；即使旧摘要显示 matched，currentOnboardContentsProven 仍 false。该 helper 不负责脱机原任务重建、部分失败后新 READ 对账、修复草稿、新上传审批或最终任务执行闭环，这些仍需后续独立实现。

## 工程交接

本批仅源码、同行静态阅读和限定文本核对，未编译、执行 helper、解析器、SQL、应用、测试或设备操作。工程师后续需验证：原参数量化完全一致；空实际读与未采集分开；数量差异、null 项、unsupported command、missing/null/typed-null/type、NaN/Infinity、额外未知字段和异常 map；64 行显示截断但总计保留；2048 项及超限拒绝；普通 FLOAT 回退不冒充 RAW INT；摘要不复制明细；原部署拒绝／成功判定不受诊断结果影响。

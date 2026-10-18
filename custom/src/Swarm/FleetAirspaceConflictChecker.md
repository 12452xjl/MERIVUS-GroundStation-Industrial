# FleetAirspaceConflictChecker 契约

`FleetAirspaceConflictChecker` 是地面站任务编译/审批前的只读空域检查器。它对输入的多机航线、时间窗、最小间距、返航路径和禁飞区做保守的空间—时间审查，输出可解释的冲突、违规和替代方案。

## 442：名义线段与区域的联合静态几何计算

新增独立纯值入口：

```cpp
static QVariantMap checkJointSpatialSegments(
    const QVariant& segmentsInput, const QVariant& noFlyZonesInput,
    const QVariant& minimumSeparationMeters);
```

本接口接收同一冻结来源明确提供的 geodetic/AMSL 名义线段、区域原值及有限正数间距。它不从当前地图、Vehicle、Home、飞行参数或旧报告补线段，不采用默认 5 米，不修改 `lastReport`、不发信号、不做 I/O、不改变任务或批准，也不发送指令。调用方负责冻结来源与原归档身份；此函数只检查值，不认证来源或区域覆盖。

**这是联合静态空间候选，不是带时间的飞行轨迹、碰撞预测、障碍物规避或净空许可。** 多架机之间不按时间排除任何线段组合；同一飞机的启动替代分支不拼接为实际航迹，也不彼此作为多机接近候选。区域时间窗只校验其输入形状，随后保守忽略，不能用它排除命中。无候选、空区域列表或计算完整均不证明安全、区域时效或任务完成。

### 完整输入与资源边界

`segmentsInput` 必须为真实、非 null 的 List，包含 1–8320 条线段，最多 32 个不同机号。每条恰好 8 键：

```text
segmentId, vehicleId, kind, from, to,
nominalOnly=true, actualPathCertified=false, timingKnown=false
```

`segmentId` 非空、无首尾空白或控制字符、最多 128 字符，并在全输入唯一；`vehicleId` 为数值整数 1–255。`kind` 仅接受以下 10 种：

```text
mission-leg, mission-anchor, mission-return-leg,
mission-return-connection, mission-launch-connection,
mission-terminal-hold, mission-terminal-land,
vertical-takeoff, takeoff-request-connection, current-first-waypoint
```

`from`、`to` 各恰好 `{latitude, longitude, altitude}`；经纬为有限数值，纬度 ±90°、经度 ±180°，高度是有限原始 AMSL 米值，不伪造缺失值或按规划高度范围夹断。零长锚点和垂直段有效，都会参与区域与跨机检查。数值字符串、布尔、null、缺键、额外键、重复段 ID 和未知 kind 均拒绝。

区域根输入先独立经过 441 的 `inspectNoFlyZoneInput`，最多 1024 项；之后才转换为列表。本次几何层不能只凭 `valid=true` 认定每项区域可解释。三个原始输入合并检查：紧凑 JSON 最大 16 MiB、深度最多 20、节点最多 1,000,000；拒绝非 JSON 值、typed-null 非字符串值、非法 Unicode、非有限值及超出 JSON 安全整数范围的 64 位整数。根区域坏值绝不转换为 `[]`。

全量计算前预估不超过 1,000,000 几何工作单位：跨机段对每对 1 单位；每个多边形预留 `n*(n-1)/2 + 3*n` 的合法性检查；每条段与该区再预留 `3*n+1`，覆盖端点包含检查、边交和高度裁剪。不得因提前命中而降低此上界，不能跳过后面的区域或只报告成功前缀。分组遍历避免单架机 8320 段产生无意义的自身平方比较。预算失败直接不完整，不启动比较。

### 支持的严格区域语法

每项为 map；仅允许下列键，未知元数据也明确拒绝，不猜测其语义：

| 类别 | 接受字段与约束 |
| --- | --- |
| 几何 | `polygon` / `vertices` / `points` / `boundary` / `coordinates` / `bounds` 六选一，必须提供，不能并存。 |
| 多边形 | 前五种为真实 List，每点恰好 `{latitude,longitude}`，可额外明确 `coordinateSystem="geodetic"`；至少 3 个顶点。点级高度、数组坐标、局部坐标、GeoJSON 嵌套、洞和未知点字段不支持，不被丢弃。 |
| 矩形 | `bounds` 恰好 `{west,east,south,north}`，经纬范围合法、west < east、south < north；不跨日期线。 |
| 标识 | `id` / `zoneId` / `name` 最多一个，与段 ID 同样的非空 128 字符约束；省略时为 `zone-1` 等按原位置生成，所有区域 ID 必须唯一。 |
| 坐标系 | 顶层可省略 `coordinateSystem`；提供时只能为 `geodetic`，不回退 x/y。 |
| 下界 | `minAltitude` / `minAltitudeMeters` / `floor` 最多一个，有限数值。 |
| 上界 | `maxAltitude` / `maxAltitudeMeters` / `ceiling` 最多一个，有限数值。 |
| 高度基准 | 任何高度界存在时必须显式 `altitudeReference="amsl"`；两界同时存在须下界 ≤ 上界。一侧缺失是真正无界，不补 ±1e30。两侧都缺失按全高度柱保守检查。显式其他基准拒绝。 |
| 时间 | `startTime` / `activeFrom` 最多一个，`endTime` / `activeUntil` 最多一个，有限数值，两者都有时 start ≤ end。可带有界非空 `timeReference` 字符串，但完全不据此证明时间对齐；所有有效时间窗保守忽略。 |

允许一个与首点完全相同的末尾闭合点；只移去这个重复末点，不修复其他重复顶点。重叠折返边、非相邻边相交/接触、自交、零面积和退化边会使整份计算不完整。直线上的正常同向连续边可保留。不支持的区域不被跳过，即使另一个区域有效或没有当前命中。

### 几何计算与范围

完整线段端点及所有区域顶点共同进入局部等距经纬近似投影，第一条段起点为参考经纬度。仅支持全组绝对纬度不超过 80°，纬度跨度、未展开的经度跨度分别不超过 0.2°。超域、极区、跨日期线或远距离区域导致整份不完整，不把远区先删除。输入经纬 ±90° 的接受范围不代表计算覆盖所有纬度。

跨机距离沿用 439 的有限线段最小三维距离计算；全组先减去共同参考高度，剩余局部坐标分量必须有限且绝对值不超过 `1e50`，避免中间高阶运算溢出。任意数值异常整份拒绝；`projection.withinSupportedDomain=true` 只说明水平局部域，不保证后续数值计算完成。距离严格小于原最小间距才形成 `nominal-spatial-proximity` 候选，不按时间或实际飞行速度排除。

区域检查先将三维段按 AMSL 高度 slab 裁剪到合法参数区间，再将裁后 XY 段与区域多边形求交，避免把“整段某处高度重叠”和“另一处 XY 相交”错误拼成命中。没有高度界时使用完整 XY 段；边界触碰、零厚度高度面、垂直段和零长锚点都保留。这里的相交仍只是名义线性段与声明区域的几何候选，不认证飞机真实轨迹或区域可信度。

### 固定输出、未知与截断

输出恰好 21 键：

```text
ok, error, comparisonComplete,
segmentCount, vehicleCount, zoneCount, zoneInputCheck,
pairCount, candidateCount, candidates, candidatesTruncated,
zoneComparisonCount, zoneHitCount, zoneHits, zoneHitsTruncated,
projection, minimumSeparationMeters,
timingKnown=false, clearanceCertified=false,
executable=false, flightCommandReleased=false
```

`zoneInputCheck` 为实际原值的 441 五字段检查，不信任调用方提交的同名元数据。`segmentCount/vehicleCount/zoneCount` 在整组结构输入解析前为明确 JSON null，解析后为真实数目；后续投影、几何或资源拒绝可保留已解析数量，但所有比较数、候选数、命中数始终为 0、列表为空，不发布经过比较的前缀。有效正数间距可保留作为诊断，未知/损坏时为 null。

`projection` 恰好 7 键：`model="local-equirectangular-approximation"`、`referenceLatitude`、`referenceLongitude`、`latitudeSpanDegrees`、`longitudeSpanDegrees`、`maxAbsoluteLatitudeDegrees`、`withinSupportedDomain`；未计算的五个数值明确为 null。

`candidates` 每项恰好 8 键：

```text
vehicleA, segmentA, kindA, vehicleB, segmentB, kindB,
distanceMeters, kind="nominal-spatial-proximity"
```

`zoneHits` 每项恰好 7 键：

```text
zoneIndex, zoneId, segmentId, vehicleId, segmentKind,
kind="nominal-zone-intersection", timeWindowIgnored
```

`zoneIndex` 是原区域列表的 0 基位置；`timeWindowIgnored=true` 说明该区域原有有效开始或结束时间，但本次没有按它排除。没有该标记也不表示时序已知，报告始终 `timingKnown=false`。

两种明细各最多 512 条，但必须继续完整计算并保存各自总数。任何一类明细超限时，`comparisonComplete=true`、`ok=false`、`error="joint-spatial-details-truncated"`，对应 truncated 标记为 true；总数完整，列表不完整，不能称全部明细已审阅。候选按遍历顺序保留、并非危险度排序。`pairCount` 只计不同机号的段对，`zoneComparisonCount` 计每段与每区，不是命中数。

输入、预算、几何、不支持域或数值失败时 `comparisonComplete=false`、`ok=false`，明确固定错误码，不保成功前缀。即使 `ok=true`，也仅表示这份受支持静态计算及其明细完整，不表示没有候选、净空通过、空域覆盖完整、时间安全或允许启动。上层必须另外说明是否覆盖原任务全部所需段，不能以本函数全部输入已检查冒充整组飞行已认证。

本批只完成源码实现和静态对读；未编译、执行、测试或连接设备。439 名义启动检查、440 归档证据显示和 441 原有 `check()` 行为保持独立。

## 441：原始禁飞区输入保真与原生阻断

既有公开入口的第三参数改为原始 `QVariant`，在原生完整检查通过之前不转换为区域列表：

```cpp
QVariantMap check(const QVariantList& missionPlans,
                  const QVariantList& vehicles = QVariantList(),
                  const QVariant& noFlyZonesInput = QVariant(QVariantList()),
                  const QVariantMap& options = QVariantMap());
static QVariantMap inspectNoFlyZoneInput(const QVariant& input);
```

省略第三参数仍是兼容的显式空 `QVariantList`；显式传入 null、布尔、数字、字符串或 map 不会被当成空列表。调用方也不得先用 `value || []`、`value ? value : []` 或 `toList()` 丢弃原类型。检查器本身不知道上层字段是否省略，省略与显式空列表的来源区别仍由协调器和归档输入元数据保留。有效空列表不证明已加载权威空域、数据完整或没有禁飞区。

`inspectNoFlyZoneInput` 是无状态、无信号、无 I/O 的纯值入口，固定恰好 5 字段：

| 字段 | 含义 |
| --- | --- |
| `valid` | 输入为真实、非 null 的 `QVariant::List`，且通过下述完整 JSON 和资源检查。 |
| `state` | 成功 `list`，拒绝 `invalid`。 |
| `reason` | 成功明确空字符串；拒绝为固定原因码。 |
| `zoneCount` | 成功为原列表项数 0–1024；拒绝为明确 JSON null，不返回有效前缀数量。 |
| `fingerprint` | 成功为完整紧凑 JSON 列表的 SHA-256 小写十六进制摘要；拒绝为空字符串。 |

三个检查失败码为 `invalid_no_fly_zones_type`、`no_fly_zone_collection_budget_exceeded`、`no_fly_zone_input_budget_or_value_invalid`。先检查实际 List 类型和最多 1024 项，再复用归档 JSON 树检查：深度最多 20、节点最多 1,000,000、紧凑 JSON 最多 16 MiB。支持正常 JSON null、空字符串、有限数值；拒绝对象指针、不支持或非字符串的 typed-null 值、非法 Unicode、非有限数值和不能用 JSON 安全整数表示的 64 位整数。嵌套原生 `QStringList` 按 JSON 数组表示，根输入仍必须是实际 `QVariant::List`。

摘要只用于比较两份完整输入值，不认证来源、覆盖、新鲜度、区域几何或时间。列表元素即使不是合法区域对象，仍可通过此层的 JSON 形状检查；其几何问题由后续既有区域检查形成 blocking 违规，而不是被筛掉或假装有效。

`check()` 首先独立调用检查器，不相信调用方提交的检查结果。自身输入失败时不执行任何区域或路线几何检查；所有正常和不完整报告均新增 `noFlyZonesInputCheck`，内容为实际独立参数的上述 5 字段检查结果。

协调器可传入原生派生的 `options.noFlyZonesSourceInvalid=true` 表示任务自带字段与独立参数来源无效或不一致。此标志只能增加阻断，不能使坏输入变好；调用方提供的同名标志应由协调器先移除，再从实际输入重算。检查器只接受 `noFlyZonesSourceInvalidReason` 三个原因码：`invalid_no_fly_zones_source`、`task_no_fly_zones_input_invalid`、`no_fly_zones_source_mismatch`；未知/错误类型归为第一个，不直接回显任意字符串。自身参数输入错误优先保留自身检查原因；任务/参数不一致时也不任选其中一份继续分析。

上述拒绝均通过原生不完整报告路径发布：`analysisComplete=false`、`feasible=false`、`requiresReview=true`、`reviewStatus=blocked-incomplete`，`violations` 包含 `type/code` 同为固定原因、`severity=blocking`、`reason` 为解释的一项，`conflicts=[]`，保留 `manualConfirmationRequired=true`、`flightCommandReleased=false`。根 `noFlyZoneCount` 在输入/来源失败时为明确 null；来源冲突时，实际独立参数的已知数量仍可在 `noFlyZonesInputCheck` 中查阅。失败会更新 `lastReport` 并发出 `reportChanged`，不保留旧成功报告、不发布检查过的前缀；它也不是撤销已发生的飞机动作。

显式最小间距损坏或 `minimumSeparationInputInvalid=true` 也改为 `invalid_minimum_separation` 原生不完整早退，不继续用默认 5 米进行几何比较。所有不完整报告的 `minimumSeparationMeters` 明确为 JSON null，表示没有发布一个完整分析实际应用的阈值；正常有效输入及未提供间距时原有 5 米兼容行为不变。其它已有集合、顶点、比较工作量和诊断资源上限继续保留，预算失败仍不发布成功前缀。

本次不改变 439 名义启动线段和 440 历史归档空间证据两个纯接口，也没有实现全路线、启动和终结动作的联合检查。仅做源码静态核对，未编译、执行、测试或连接设备。

## 440：同一任务归档的空域证据展示

新增纯值原生接口：

```cpp
static QVariantMap archivedMissionSpatialEvidence(
    const QVariantMap& plan, const QVariantMap& reviewEvidence);
```

调用方须传入同一实际 PlanStore 记录的原始 `plan` 与 `reviewEvidence`。此接口不认证记录身份；它只取回有界 JSON 证据，不调用 `check()` 或 439 检查、不读取当前地图/最新禁飞区、不重新求阈值、不补 Home/返航/启动路径、不修改 `lastReport`、不发信号、不做 I/O 或发送命令。

输出固定 12 字段：

```text
available, inputState, noFlyZones, zoneCount,
minimumSeparationMeters, thresholdSource, report,
scope="archived-planning-routes-only", limitations,
coverageCertified=false, clearanceCertified=false, timingCertified=false
```

`available` 只表示原归档存在非空、可展示的 `airspaceReport` map，不表示报告通过、输入完整、几何有效或空域安全。输入状态与报告是否可查看彼此独立；例如明确损坏的原禁飞区输入仍可能有一份应展示的阻断报告。`report` 不重新解释、补键或升级旧报告的安全声明；原有 conflicts、violations、reasons、alternatives 等内容仅作为历史证据保留。

新记录可包含 `airspaceInputEvidence`，恰好 4 字段：`schemaVersion=1`、`noFlyZonesProvided`（真实 bool）、`noFlyZonesListValid`（真实 bool）、`noFlyZonesCount`（实际列表项数或 JSON null）。后者只声明原输入是否为数组，**不证明每个区域有效或覆盖完整**。

| inputState | 含义 |
| --- | --- |
| `recorded-list` | 新元数据明确 provided/listValid 均 true，计数与原 List 完全一致；显式空列表计数为 0。没有元数据的旧非空 List 也保留为已记录列表，但附带缺少原输入存在性元数据的限制。 |
| `recorded-omitted` | 新元数据明确未提供、listValid=false、计数 null；归档列表只允许缺失或兼容旧归档表达式的空 List。 |
| `recorded-invalid` | 元数据明确原输入存在但不是列表，或原字段类型错误；元数据损坏/矛盾、JSON/资源检查失败也以此状态明确拒绝。 |
| `legacy-unknown` | 没有新元数据，且旧 noFlyZones 为缺失或空 List；不能区分当时未提供与显式提供空列表，不能称“已加载全部禁飞区”。 |

可用的原 List 原样保留为 `noFlyZones`；不把它转换成新的区域 DTO、不筛掉坏项、不生成有效前缀。原列表可能包含未通过旧检查的坐标或其他输入，查看方必须按实际类型展示，不得把列表存在当成几何已验证。只有可确定已记录列表项数时 `zoneCount` 才为整数，其余为明确 JSON null。损坏原输入可能被旧归档表达式折叠为空 List；它仍是 `recorded-invalid`，不会成为合法空区域证据。

`minimumSeparationMeters` 只取原 `airspaceReport` 中的有限正数，`thresholdSource="archived-applied-value-not-explicit-policy"`；取不到时分别为 JSON null 和 `"unavailable"`。不以当前地图、默认 5 米、`plan.constraints` 或原 `reviewOptions` 覆盖它。协调器当时可能合并更严格的任务与 options 阈值，因此原 options/plan 数值与实际报告值不同会保留为 `limitations` 诊断；缺失或损坏 options 也明确说明不能重建。实际应用过的数值本身仍不能证明当时存在明确政策。

`limitations` 始终是有界字符串列表，明确历史性、归档规划路线范围、禁飞区完整性/新鲜度/几何未认证、时序/地形/障碍/净空未知；附加项解释原输入元数据、原报告、options 与阈值差异。原报告不因此覆盖新增显示的结束动作、垂直起飞段或当前位置连接线，也不成为这些线段的净空审阅。

输入将 `{plan, reviewEvidence}` 作为一个整体检查：紧凑 JSON 最大 16 MiB、深度不超过 20、节点不超过 1,000,000。输出也使用相同预算。接受 JSON null（包括原生 invalid QVariant 及 JSON null）、正常空字符串和有限 JSON 数值；拒绝对象指针、其他不支持/typed-null 值、非法 Unicode、非有限数值及超出精确 JSON 整数范围的 64 位整数。元数据损坏/矛盾或整组超限时，固定输出 `available=false`、空 report/列表和未知计数/阈值，不发布有效前缀；具体原因在 `limitations`。出口进行 JSON 表示规范化，保持未知为明确 null，避免 QML 将缺值误当成零或删掉字段。

本增量仅完成源码与静态读回，未编译、运行或测试。它不改变既有 `check()`、439 名义启动线段检查、任务审批或飞行放行。

## 439：独立名义启动线段空间接近检查

新增纯值原生接口，不改变下文既有 `check()` 的输入、默认值、结果或信号行为：

```cpp
static QVariantMap checkNominalStartupSegments(
    const QVariantList& segments, const QVariant& minimumSeparationMeters);
```

此接口仅比较调用方明确提供的、已经冻结的名义线段。它不读取 Vehicle、飞行参数、当前地图或任务库；不修改 `lastReport`、不发信号、不做文件 I/O、不发送命令，也不建立任何启动许可。调用方仍须证明显示来源属于对应原生审阅；纯值输入本身不认证来源。

### 输入与几何含义

`segments` 为 1–96 项列表，合计最多 32 个不同的车辆 ID。每项恰好 8 个字段：

```text
segmentId: 非空、全列表唯一、最多 128 字符的字符串
vehicleId: 1–255 的真实数值整数
kind: 非空、最多 128 字符的诊断标签
from: {latitude, longitude, altitude}
to:   {latitude, longitude, altitude}
nominalOnly: true
actualPathCertified: false
timingKnown: false
```

两个端点均恰好 3 个数值字段。本接口固定解释为 WGS84 经纬度和 AMSL 米制高度，不接受本地坐标、相对 Home/地形高度、字段别名或缺失高度。经纬度分别须在 ±90°、±180° 内，高度须有限。数值字符串、布尔数值、null、非有限值，以及不符合固定值的布尔声明均拒绝。字符串不允许前后空白、NUL 或控制字符。

`kind` 只保留分支含义，不据此补点或拼成路径。当前调用方分别供应 `vertical-takeoff`、`takeoff-request-connection`、`current-first-waypoint`；同一车辆的线段不互查。尤其不会把“当前位置到首航点”诊断接到垂直段之后，也不会用规划航线替代这些线段。

`minimumSeparationMeters` 必须明确为有限正数，**不沿用既有 `check()` 的 5 米默认值**。缺失或损坏时不做比较。

### 有界局部投影与比较

复用已有 `toLocal` 和 `minSpatialDistance` 几何实现：首端点作为局部投影参考，以两条有限线段的三维空间最短距离比较阈值，不采样时间、不假定同时到达。同一组全部端点须满足绝对纬度不超过 80°、纬度数值跨度和经度数值跨度各不超过 0.2°。跨日期线的巨大经度跨度、远距离或极区输入返回不完整，不自动展开经度或截取可算前缀。

投影仍是近似米制模型，不是大地测量、路径动力学或净空认证。共同高度偏移会在计算前去除；差值非有限、局部坐标绝对值超过 `1e50`（避免既有距离运算中间乘积溢出），或计算结果非法时同样返回不完整。

最多 96 段、32 机对应最多 4464 个跨机线段对；所有跨机对都计算。距离**严格小于**阈值才计为名义接近候选。结果最多保留前 512 条候选明细，候选总数仍来自完整比较；明细顺序沿输入线段对顺序，不按风险排序。

### 返回契约

顶层固定 12 字段：

```text
ok, error, comparisonComplete, candidateCount, pairCount,
candidates, candidatesTruncated, projection, minimumSeparationMeters,
timingKnown=false, clearanceCertified=false, executable=false
```

`candidateCount/pairCount` 为 0–4464 的整数；`candidates` 最多 512 项。每项恰好 6 字段：`vehicleA`、`segmentA`、`vehicleB`、`segmentB`、`distanceMeters`、`kind="nominal-spatial-proximity"`。车辆为数值整数，线段为原始 `segmentId`，距离为有限非负米值。不输出时间窗口、碰撞概率或“安全/可起飞”结论。

`projection` 固定 7 字段：`model="local-equirectangular-approximation"`、`referenceLatitude`、`referenceLongitude`、`latitudeSpanDegrees`、`longitudeSpanDegrees`、`maxAbsoluteLatitudeDegrees`、`withinSupportedDomain`。尚未得到完整有效输入时，5 个数值为明确 JSON null；完整但投影域外时保留实际范围并将最后一项设为 false。该标志只说明水平投影域，不认证高度、时间或真实路径。无法取得有效阈值时，顶层 `minimumSeparationMeters` 同样为明确 JSON null。

- 全部对已计算且明细完整：`ok=true`、`comparisonComplete=true`、`error=""`，**即使候选数大于零也如此**；`ok` 只说明报告计算/呈现完整，不代表风险通过。
- 全部对已计算但命中超过 512：`ok=false`、`comparisonComplete=true`、`candidatesTruncated=true`、`error="candidate-details-truncated"`；总数完整、明细不完整。
- 输入、资源、投影或数值计算失败：`ok=false`、`comparisonComplete=false`、计数为 0、候选为空、`candidatesTruncated=false`，不发布已算前缀。

拒绝原因闭集为 `minimum-separation-missing-or-invalid`、`nominal-segments-missing`、`nominal-segment-budget-exceeded`、`nominal-segment-shape-invalid`、`nominal-segment-fields-invalid`、`nominal-segment-id-duplicate`、`nominal-segment-coordinate-invalid`、`nominal-vehicle-budget-exceeded`、`local-projection-domain-unsupported`、`numerical-domain-unsupported`、`nominal-distance-computation-invalid`；另有上述已完成比较但明细不足的 `candidate-details-truncated`。

即使候选为零，也没有证明真实启动轨迹、完整任务/返航路径、地形、障碍、禁飞区数据覆盖、定位误差、风扰、时间分离或净空。上述未知项不会被零候选结果消除。本批仅完成源码及静态阅读，未编译、测试或运行几何算法；工程验证仍待工程师执行。

## 输入

```text
missionPlans: {
  vehicleId,
  waypoints/route/path: [{x,y,z} 或 {latitude,longitude,altitude}],
  returnPath/returnWaypoints: [...],
  startTime/durationSeconds,
  returnRequired
}[]
vehicles: 可选的车辆快照（当前仅作为适配层输入保留；本模块不会根据 home 自动生成返航走廊）
noFlyZones: {
  id, polygon/vertices/points 或 bounds,
  minAltitude/maxAltitude,
  startTime/endTime
}[]
options: { minimumSeparationMeters, returnRequired, timeStepSeconds }
```

输入门禁：`minimumSeparationMeters` 如提供必须是有限正数；重复的
 `vehicleId`、含无效顶点的禁飞区、多边形起止时间反向都会产生 blocking
违规，不会被当作可检查的安全输入。禁飞区至少需要三个且全部有效的坐标点；禁飞区高度上下限、时间边界和 `timeStepSeconds`（如提供）也必须是有限且有序的数值。`bounds` 使用 `west/east/south/north` 时按经纬度解析，使用 `minX/maxX/minY/maxY` 时按调用方声明的本地米制坐标解析；边界必须有限且正向有序。车辆 ID 只接受真实 QVariant 数值类型中的 1–255 整数，数值字符串、`"01"` 和显示名称不会被转换成身份。
`options.returnRequired` 与每条任务的 `returnRequired` 只接受真实 QVariant `bool`；字符串
`"false"`/`"true"` 不会通过隐式转换改变返航路径要求，显式类型错误会生成 blocking 违规。

路径点带有 `timeSeconds`、`timestamp`、`time` 或 `etaSeconds` 时使用 4D 线性插值；完整一致的显式时长可用于插值，但恒速生成/其他非权威 `timingEvidence` 始终保留 `timingKnown=false`。任务超时或截止窗口不作为轨迹时长；返航时长插值必须有独立返航开始时间，不沿用出航开始时间。未知时序仍做空间检查并阻断审批，不能据此跳过时间受限禁飞区。经纬度会在首个有效点建立局部米制投影；与未声明原点的局部坐标混用会阻断审查。

路径时间字段一旦出现但类型非法、非有限或不单调，会产生 blocking 违规；没有权威时间且路径至少含两个点时同样阻断，避免把相对采样结果误当成安全的 4D 证明。`timeStepSeconds` 目前只作为输入格式门禁保留，尚未改变几何检查步长；不能把它理解成额外的安全证明。

禁飞区检查对每条路径段执行二维线段—多边形相交（包含边界），不再用固定数量采样点跳过狭窄区域；同时对段端点高度范围和禁飞区时间窗做保守相交判断。若禁飞区声明了高度上下限，而路径点缺少有限 `altitude`/`alt`/`z`/`height`，会产生 blocking 的 `path_altitude_missing`，不会把缺高点默认为地面高度放行。

## 输出

`check()` 返回并保存 `lastReport`：

- `conflicts`：不同车辆路径在时间重叠区间内低于 `minimumSeparationMeters` 的冲突，包含车辆、路径类型、片段、最小距离、时间窗口和严重度。
- `violations`：缺少路径/车辆 ID、坐标无效、禁飞区进入、返航通道缺失或坐标系不一致。
- `reasons` / `alternatives`：面向操作员的可解释原因和建议，例如错开时间、重规划或提供返航走廊。
- `feasible`、`severity`、`requiresReview`、`manualConfirmationRequired`。
- `flightCommandReleased=false`：该类不持有 Vehicle/MAVLink，不上传任务，也不自动执行重规划。

检查是保守几何相交与线性相对运动计算，不替代当地法规、飞行空域服务、飞控避障或工程师验证。真实坐标基准、禁飞区数据源、返航策略和任务执行接入由工程师另行确认。

## 2026-10-13 区域规划接线增量

审阅协调器将任务约束中的 `minimumSeparationMeters` 与 options 一并核对，使用有效输入中更严格的间距，损坏显式输入仍阻断，不能用默认值/较小 options 覆盖任务要求。

路径及禁飞区的 `localFrameId`、`altitudeReference`、`timeReference` 声明必须真实、有界且一致。已声明与未知混用、不同本地原点或高度/时间基准不一致均产生阻断。高度基准不能比较时不使用数值高度排除空间冲突，时间基准不能比较时不以时间窗排除冲突；不同本地原点不能假装处于同一平面。全部不声明的 legacy 输入只保留兼容语义，不证明其物理基准已经确认。

未知时序的两条路线用完整线段最短距离进行保守空间检查，不再仅比较固定十个同进度采样点。它仍不是真实轨迹/动力学模型。

任一比较路径缺少数值高度时，双方均降为 XY 间距，不能把缺高点的默认零值当成垂直分离证据；诊断带 `distanceDimensions` 与 `altitudeKnown`。稀疏时间戳只线性扫描间隙进行插值，仍保持未知权威时序，不重复向后搜索导致二次方工作量。

资源上限为 512 条 mission plans、1024 个禁飞区、16384 个出航/返航/区域顶点、100 万次潜在航段两两及区域边比较、2048 条合计诊断。输入及工作量先预算，诊断追加也受控。任一超限返回 `analysisComplete=false`、`feasible=false`、`reviewStatus=blocked-incomplete` 和明确预算违规；检查过的前缀及其诊断不会伪装成全集合安全。普通结果带 `coordinateComparisonAvailable/altitudeComparisonAvailable/timeComparisonAvailable` 与 `referencePolicy`。大型任务仍需后续空间索引/异步执行优化，不能绕过预算继续冻结操作界面。

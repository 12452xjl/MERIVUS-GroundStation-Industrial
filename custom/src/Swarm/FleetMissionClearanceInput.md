# 工程净空资料输入合同（448–452）

## 452：显式接地声明 schema2

`normalize` / `inspectForPlan` 的返回形状、4 MiB / 深度16 / 131072节点总预算及整份失败语义不变。旧 schema1 的根 exact4、车辆 exact6 和规范字节保持原合同；不自动升级旧资料，不给旧车辆增加支撑尺寸，不补空 contacts。

新输入根 exact5：`schemaVersion=2,scene,vehicles,policy,contacts`。scene exact13 与 policy exact9 **完全沿用下文原合同**，包括 `contactHandling="require-explicit-contact-context"`。内部资料 schema2 与编辑器 `planningSource.schemaVersion=2` 是不同版本层；后者表示保留完整 clearanceInput，不因本次内部升级改为3。

### schema2 vehicles（每项 exact9）

原六键追加三个显式整数：`supportHorizontalRadiusMm` 和 `supportBelowReferenceMm` 为1–100000毫米，`supportAboveReferenceMm` 为0–100000毫米。原有 ID、配置指纹、尺寸/误差规则不变。在 schema2 中，`bodyHorizontalRadiusMm/bodyVerticalHalfExtentMm` 声明需保护的机体/旋翼包线，**不含仅由新增三个字段声明的起落支撑结构**；不能把整架机体都当作允许接触地面的支撑部分。这只是工程模型分离，不验证实机的包线或构型。

### contacts（真实列表0–64项，每项 exact12）

完整字段：`contactId,vehicleId,kind,payloadHash,missionSequenceNumber,surfaceModel,referencePoint,padPolygon,terrainCellIds,wireGroundToleranceMm,referenceHeightToleranceMm,maximumSupportPenetrationMm`。

- contactId 遵守下文128字符标识规则，在整个 contacts 列表内唯一；每个 `(vehicleId,kind)` 组合也只能出现一次。vehicleId 必须是本份 vehicles 中的真实成员，不接受备用/未知 ID 自动替换。
- kind 只能为 `takeoff` 或 `landing`；payloadHash 为小写64位 SHA256；missionSequenceNumber 为0–2047的整数。这里只检查声明形状，不证明 hash/序号已经对应实际完整编码、实际机载内容或某个当前指令。
- surfaceModel 固定 `engineer-declared-flat-contact-pad`。referencePoint exact3：latitudeE7 为 ±800000000、longitudeE7 为 ±1800000000、groundAmslMm 为 −1000000…20000000，均为整数。
- padPolygon 使用与场景多边形相同的局部毫米坐标及严格凸、逆时针、3–16点、非共线规则，不转换坐标、不修复多边形。
- terrainCellIds 必须是真实列表，1–256个不同的既存 scene.terrainCells.cellId。所列每个 cell 的 maximumTerrainAmslMm 必须与本项 groundAmslMm 精确相等；不能把障碍或走廊 ID 当作 terrain ID。
- wireGroundToleranceMm、referenceHeightToleranceMm、maximumSupportPenetrationMm 都必须显式给出0–100000毫米整数；不使用默认容差。

平整接触面的存在和高度是 `surfaceModel` 的**独立工程声明**，并非从“地形最高高度等于该值”推导出整块地面处处平整。输入校验本身也不证明 pad 包含参考点/支撑扫掠、不检查实际接地姿态或穿透量；对应的几何、编码、实际来源消费必须另外完成。空 contacts 是显式空声明，不表示起飞或降落不需要上下文。

新整数字段与原字段一样规范成精确范围内的 Double；列表原序保留，terrain 引用不排序、不去重修复。通过公共纯值/资源检查后，接地字段、引用或重复项不合法返回 `clearance-input-contacts-invalid`；typed-null、opaque、超预算等仍使用原公共拒绝原因。所有失败的 input/fingerprint 均为空，不返回已接受的前缀。资料不包含将来的 planId/reportHash/最终审阅 artifactHash，避免把派生报告哈希反向嵌入自身来源。

本模块仍不读取当前时间、真实构型、场地或飞控。schema2 支撑和接地声明的保存不认证 datum 对应、资料当前有效、现场平整度、性能或净空，也不改变批准、上传、启动或最后发送门禁。

## 449：资料已可由真实整段计算核心消费

[FleetMissionClearanceReview](FleetMissionClearanceReview.md)现在完整消费此合同和同序Joint段，并使用[精确扫掠／闭并集](FleetMissionClearanceGeometry.md)检查场地、走廊、地形与障碍。输入schema／原normalize和inspect API不变；本合同自身仍不计算净空、不查当前时间或实际构型。

下文448“待后续solver”的几何计算部分由449独立helper推进，但实际规划／启动固定来源消费、资料时效／配置／高程对应、明确接地上下文和最终许可仍待开发。起飞或LAND缺接地上下文时模型满足false；不删段、不补默认或解除Compiler pending。

`FleetMissionClearanceInput` 是纯值校验/规范化入口。它接收工程师明确提供的场地、机体包线和误差声明，输出完整资料及内容指纹；**不是净空 solver、当前场地检查、认证或启动许可**。不读取系统时间、地图、Vehicle、Fact 或最新配置，不发送命令，不写文件，不补默认资料。

## API 与可选输入语义

- `normalize(const QVariant& source)` 返回 exact4：`ok`（Bool）、`reason`（QString，成功明确空串）、`input`（完整规范 map 或空 map）、`fingerprint`（成功为小写 SHA256，失败明确空串）。失败不返回成功前缀。
- `inspectForPlan(const QVariantMap& plan)` 返回 exact7：`provided/inputValid/reason/input/fingerprint/memberIdsMatch/snapshotOnly=true`。
- 缺 `plan.constraints`，或正常 constraints map 中缺 `clearanceInput`：`provided=false,inputValid=true,reason="",input={},fingerprint="",memberIdsMatch=false`。这仅保持未提供资料的旧计划兼容，**不表示净空满足**。
- 明确坏型/typed-null constraints：`provided=false,inputValid=false`，原因 `clearance-plan-constraints-invalid`。不能把坏容器当成合法缺省。
- 只要 `constraints.clearanceInput` 存在，`provided=true`；显式 null/坏型/坏内容均失败。资料本身完全合法、但与 plan.members 主机 ID 不匹配时，保留完整规范资料/hash供诊断，`inputValid/memberIdsMatch=false`；不是允许它用于该计划。
- 成员要求 1–32 个唯一 1–255 ID，与资料 vehicles 完整**同序**一一对应，仅 active members，不从 reserve 补充。inspect 不重新分配成员，也不认证 configurationFingerprint 与实际机体相同。

输入资料限 4 MiB 紧凑 Qt JSON、深度 16、131072 节点；先检查树和值，再编码核实际字节。opaque、非法 UTF-8 往返、非有限数值、不安全大整数、typed-null 或超预算整份拒绝。普通原生 QString 空串可进入树预算检查，但下面要求非空的字段仍拒绝。真实列表须 QVariantList，不能把标量/QStringList/空值转换为列表。此预算仅约束 clearanceInput，**不新限整个旧计划为 4 MiB**。

合法整数字段全部规范为 Double，仍处于精确整数范围。字符串不修剪、不改日期、不修复多边形，不丢弃字段；规范结果的完整紧凑 Qt JSON 用于 SHA256。hash 仅标识内容，不能作为签名、测绘真实性或性能证明。

## schema1 完整输入

根 exact4：`schemaVersion=1,scene,vehicles,policy`。所有字段必须显式提供，不接受附加字段。

### scene（exact13）

`sceneId,revision,sourceLabel,sourceDocumentHash,acquiredAtUtc,validUntilUtc,horizontalDatum,verticalDatumId,localFrame,coverageVolumes,obstacleVolumes,terrainCells,corridorVolumes`。

- sceneId/sourceLabel/verticalDatumId 为非空、已修剪、合法 UTF-8、无控制字符的至多128字符字符串。
- revision 为规范正 uint64 十进制字符串；sourceDocumentHash 为小写64位 SHA256。
- acquiredAtUtc/validUntilUtc 必须为 UTC `YYYY-MM-DDTHH:mm:ss.zzzZ`，后者严格晚于前者。不读取“现在”，因此日期合法不代表本次仍有效。
- horizontalDatum 固定 `wgs84`；verticalDatumId 是显式工程声明，不由 WGS84 推断 AMSL/椭球高转换。
- localFrame exact7：modelName 固定 `engineer-declared-local-affine-geodetic-chart`；originLatitudeE7 在 ±800000000，originLongitudeE7 在 ±1800000000；northNanometresPerLatitudeE7/eastNanometresPerLongitudeE7 为 1–200000000；maximumLatitudeDeltaE7/maximumLongitudeDeltaE7 为 1–2000000。原点加减对应 delta 不得越上述纬度/经度域。
- 不计算 cos、不引入默认投影。声明的局部仿射图表及误差不是经过认证的地理投影。

coverageVolumes 为1–128项，obstacleVolumes 为0–512项，terrainCells 为1–256项，corridorVolumes 为1–128项；合计不超过1024。四组内的 volumeId/cellId **跨组合并唯一**，字符串遵守上述128字符规则。

volume exact4：`volumeId,polygon,minimumAmslMm,maximumAmslMm`。两高度为 −1000000…20000000 毫米的整数，minimum 严格小于 maximum。terrain cell exact3：`cellId,polygon,maximumTerrainAmslMm`，高度同域。它们是待后续 solver 消费的毫米凸柱/地形上界资料，不声明覆盖域已覆盖路线。

polygon 为真实列表3–16点，每点 exact2 `eastMm/northMm`，整数各在 ±50000000。每条有向边对所有非端点的叉积都必须严格大于0，因此仅接受严格凸、逆时针、不重复、不共线的多边形；不自动反转、闭合、删除重复点或修复相交。整数差最大1e8，乘积绝对值最大1e16，叉积差最大2e16，均在 qint64 安全范围内，比较不使用浮点 epsilon。

### vehicles（1–32项，每项exact6）

`vehicleId,configurationFingerprint,bodyHorizontalRadiusMm,bodyVerticalHalfExtentMm,horizontalTrackingErrorMm,verticalTrackingErrorMm`。ID 唯一且1–255；配置指纹为小写SHA256；两机体尺寸为1–100000毫米；两跟踪误差为0–100000毫米，全部显式整数。实际配置/性能对应须由后续原生绑定者另核。

### policy（exact9）

六项 `horizontalSafetyMarginMm/verticalSafetyMarginMm/horizontalSceneErrorMm/verticalSceneErrorMm/horizontalProjectionErrorMm/verticalDatumAlignmentErrorMm` 均为显式0–100000毫米整数；`datumAlignmentDeclared` 必须是真Bool true；`contactHandling="require-explicit-contact-context"`；`coverageRequirement="entire-swept-envelope"`。

声明不能绕过接地：真实起飞/降落接触 context、整扫掠包线包含关系、障碍/地形交叠及实际准入消费仍待后续开发。不会把机体与地面的必要接触一概当成通过或一概删去。

## 验证边界

本批仅源码实现及静态文本阅读，未编译、运行 normalize/inspect、解析资料、执行测试或连接设备；未修改PX4。资料合同/ID对应完成不等于实测资料可信、当前时刻有效、实际配置匹配、solver完成、净空通过或获得StartPermit。

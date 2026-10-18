# 余段新记录的只读地图与逐项显示投影

## 478：中央来源版本与严格新编码自然进入既有显示

既有View实际调用single／group中央matchesPlan，因此新single3／4及group2完整policy两端匹配自然进入原全部成员／逐项投影，不新增QML字段或放宽原顶层固定schema。其normal PayloadBuilder保持当前严格policy及编码后边界检查，不能用历史重现口让旧未绑定政策记录看起来可上传。

旧无policy历史仍可由原几何部分展示，但编码明确不可用、需从完整源重编译／审阅；编码失败不返回成功前缀，不把地图原形或声明二维通过当StartPermit。仅源码消费者静读，未编译、运行界面或测试；下方为历史。

下方为历史记录。

## 475：联合余段的完整多成员显示，旧单机不变

新增 `buildGroup(plan)` 和 Coordinator 薄入口 `buildGroupRemainderView(plan)`，只接受独立 `groupRemainderPlanningSource` 并先用 [GroupPlanner::matchesPlan](FleetMissionGroupRemainderPlanner.md) 核完整实际计划；不把一机显示冒充全组。旧 `build(plan)` 及 schema1 exact15 完全保留。

新显示顶层 exact12：`schemaVersion=2,planId,geometryAvailable,geometryError,memberCount,pointCount,encodingAvailable,encodingError,members,limitations,flightCommandReleased=false,executable=false`。`members` 跟实际计划原序，每行 exact7：`vehicleId,role,sourceVehicleId,routes,pointCount,encodedItems,payloadHash`。所有成员的 geodetic／AMSL 两段路线、点和新旧 ID 映射均全验后一次发布；单成员最多2048点、整组最多8192点、最多32成员，任意坏项不返回几何成功前缀。每成员出航第 i≥1 点才沿其自身映射保原航点提示，新 ingress、返程和动作不造原索引。

整组只调用一次真实 PayloadBuilder，再按实际 ID 一一核完整 payload、raw／canonical／sourceItemMap／两种量化记录及序号，单机2048项／整组8192项。任何成员编码失败，全组 `encodingAvailable=false`，所有成员 `encodedItems=[]`／`payloadHash=""`；已全验的几何可独立查看，不保留编码成功前缀。只有整组通过才发布全部编码，并保留原 Builder 限制。出口继续显式 JSON null，不把未知补零；没有从历史 summary 或当前草稿补源。

显示无权限、不写磁盘、不恢复 Run、占用、批准或正式完成；地图几何和编码一致性不是场景／现实安全证明。仅源码静态修改／对读，未编译、执行或测试；全组坏末行、重复 ID、数量边界、编码失败无前缀及新旧单机兼容均待工程师验证。下方为历史。

## 473：历史资料生成的新计划沿用同一显示投影

[余段规划资料包](FleetMissionRemainderArchive.md)的[显式保存／打开入口](FleetMissionRemainderArchivePanel.md)保存单 Run、单成员的完整任务／计划／载荷／观察，不从复盘摘要补造路线。原 native18 取源不变；整包校验后生成历史20／schema2，进入新的人工余段规划，而非恢复实际 Run。

本 View 原本已实际使用 [Planner::matchesPlan](FleetMissionRemainderPlanner.md)，其中央校验现严格支持 native provenance28／schema1 与 historical provenance30／schema2，后者保留包 ID／内容摘要。View 不需放宽原外层 schema：顶层15字段、完整几何和编码逐项核对、限额及失败不返回前缀全部不变，不以历史标签替代真实新计划的完整路线。地图显示仍只画该新记录，不反向认证文件、当前飞机、执行结果或净空。

包含式原航点、该点完整停留、原数据和安全政策、绝对时序拒绝及重新当前审阅仍由上游保持；显示不自动按 current/reached 跳点，也不复活旧审批。`flightCommandReleased=false`／`executable=false` 与其它权限边界不变，不恢复 Run、控制权或正式完成结果。本批仅静态读回，未编译、执行或测试；下方原显示合同继续适用。

## 435：真实新计划的一次性显示构建

`FleetMissionRemainderView::build(plan)` 接收实际编译后的单成员余段计划；`FleetMissionReviewCoordinator::buildRemainderView(plan)` 只是同线程纯值入口，不更新 lastReview，不查飞机或磁盘，不发消息。Root 在新归档审阅的上下文创建时调用一次，并在原来源、完整 binding 和输入／审阅代次仍匹配时保存 `view`。展示 getter 不重跑编码器。

`fleetMissionRemainderReview` 从434的11字段扩为12字段，新增 `view`，其余身份／批准接口不变。Root 继续对外深拷贝纯值，显示内容不作为上传许可、占用或审批的输入。投影失败不会抹掉已经提交的归档事实；空投影明确不可查看，不能回退旧草稿或旧 Run。

## 值合同

顶层15字段：`schemaVersion=1, planId, geometryAvailable, geometryError, vehicleId, role, routes, pointCount, encodingAvailable, encodingError, encodedItems, payloadHash, limitations, flightCommandReleased=false, executable=false`。

- 几何先核结构和数量，再复用 `FleetMissionRemainderPlanner::matchesPlan` 核 provenance 与实际单成员计划一致。此哈希一致性不认证外部文件、原生对象或硬件。
- 几何成功固定两条路线，顺序为 `waypoints`、`returnPath`，每条只有 `sourcePath, points`。规划点总数不超过2048；任何坏点／超限均不返回成功前缀。
- 点14字段：`pointIndex, coordinateSystem, latitude, longitude, altitude, altitudeReference, headingKnown, heading, holdKnown, holdSeconds, elapsedKnown, estimatedElapsedSeconds, originalSourcePointIndexKnown, originalSourcePointIndex`。当前余段只支持明确 geodetic/WGS84、AMSL；纬度[-80,80]、经度[-180,180]、海拔[-1000,20000]。
- 未声明值用 `known=false` 与 null，不把未知航向、驻留或估计时间补零。各路线索引从0重新计；新出航起点没有原索引，出航第i点（i≥1）才对应 provenance.sourceWaypointIndex+i-1，返程没有原索引。
- 显示出口用 `QJsonDocument::fromVariant(...).toVariant()` 做一次有限纯值转换，使原 invalid QVariant 变成显式JSON null；否则Qt5的QML转换可能得到undefined并在Root的JSON复制中丢键。raw／canonical的默认yaw同样保留键；不改原Builder或其内容摘要。[Qt5.15.2值转换源码](https://raw.githubusercontent.com/qt/qtdeclarative/v5.15.2/src/qml/jsruntime/qv4engine.cpp)与[JSON转换源码](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/serialization/qjsoncbor.cpp)已只读核对，未运行应用验证。
- `estimatedElapsedSeconds` 是该段累计移动估算，未含停留，返程从0开始；不是绝对ETA、完整任务经过时间、实飞进度或完成证据。

## 编码与规划几何分离

编码完全复用 `FleetMissionPayloadBuilder::build(plan)`，不使用 Coordinator 的 transportEncoding 摘要推测完整载荷。失败时保留原 `errorCode/errorDetail` 及可选 `vehicleId/route/pointIndex`，没有编码项前缀；几何有效仍可以独立查看。

成功编码行9字段：`sequenceNumber, command, frame, sourcePath, sourcePointIndexKnown, sourcePointIndex, rawItem, canonicalItem, wireQuantization`。单成员完整编码含动作最多2048项；每行按同一协议序号联结原 items、canonicalWireItems、sourceItemMap 和量化记录。wireQuantization 分 `coordinate` 与 `floatParameters`，都是编码器原值，不在界面重算。

速度、起飞、结束动作也占协议序号；动作没有 sourcePointIndex。地理项默认航向 null 不等于0度；速度命令的保留参数零值不是经纬度／AMSL高度。原始规划数值与 E7／float32 规范化值分开，量化差异不是净空验证。显式 heading 当前可保留为规划要求，但原编码器会拒绝，不在显示层删除要求以制造可上传计划。

## 实际界面与边界

[逐点面板](FleetMissionRemainderRoutePanel.md) 和 FlyViewMap 消费同一新审阅上下文。只有明确采用余段后，地图规划层才切到该记录；过期不会自动切回旧区域草稿。旧区域边界／顶点／提案线在余段来源下隐藏，真实飞机及实际任务显示保留。

地图只画二维规划折线，不验证地形、障碍、实际起降轨迹或避障。地图查看、点分页和编码展开均只读，不改变任务、恢复批准或发命令。人工采用、批准与地面上传仍见[原余段工作流](FleetMissionRemainderPanel.md)。完整受控启动、正式接替、结果判定和原25目标仍继续。

本批只做源码和限定静态核对；未编译、测试、运行helper／UI／应用／设备。工程师待执行事项见[435交接说明](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

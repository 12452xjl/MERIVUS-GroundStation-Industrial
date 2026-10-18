# FleetMissionSpatialReview

## 440：同一原任务的规划路线、编码项和归档空域依据

`FleetMissionSpatialReview::build(plan, reviewEvidence)` 是无 QObject、无 I/O 的纯值显示函数。[Coordinator](FleetMissionReviewCoordinator.md) 的同线程 `missionSpatialReview(plan, reviewEvidence) const` 仅转调它，不修改 `lastReview`、不发送信号、不读取遥测或新环境数据、不上传或发送命令。输入必须由调用者绑定到同一实际归档记录；公开值函数和内容 hash 不认证来源或授予权限。

仅增加共同显示能力，既有 [PayloadBuilder](FleetMissionPayloadBuilder.md)、审批、上传、读回和启动门槛均不改变。原始任务路线、439 启动独立假设、结束动作以及原空域报告不是本轮完成的联合安全检查。

## 固定返回与预算

返回 exact3：`ok`、`error`、`view`。结构、身份、纯值类型或资源预算不合格时返回 `ok=false`、明确错误和空 `view`，不发布有效前缀；正常返回 `ok=true` 只表示显示投影完成，不代表几何完整、编码成功、空域通过或可执行。

输入 `{plan, reviewEvidence}` 合计和完整输出各限 compact JSON 16 MiB、深度20、1,000,000节点。先检查纯 JSON 类型、有限数字及预算，再调用编码器；不接受 QObject、指针、opaque 数据、非有限值或非字符串 typed-null；原生64位整数超出 JavaScript 精确整数范围（±9007199254740991）也拒绝，不经 JSON 静默舍入。普通原生空 QString 保留为空字符串；未知标量通过最终 JSON round trip 输出显式 `null`，避免 Qt5→QML 的 `undefined` 丢字段。

支持1–32个有唯一真实数值 ID（1–255）的执行成员，角色为 leader/scout/relay/responder。每机出航与返程合计最多2048个规划点，全组最多8192；原编码器另核包括速度、起飞与结束动作后的每机2048／全组8192项预算。备用 ID 有界、唯一且不能与执行成员重叠；备用能量账单路径不插入活动路线或载荷。

整份 `view` exact17：

`schemaVersion=1`、`planId`、`memberCount`、`totalPointCount`、`totalItemCount`、`geometryComplete`、`encodingAvailable`、`encodingError`、`vehicles`、`reserveVehicleIds`、`limitations`、`snapshotOnly=true`、`executable=false`、`flightCommandReleased=false`、`onboardContentsProven=false`、`clearanceCertified=false`、`airspace`。

`totalPointCount` 是实际完整展示的成员路线点数，不将坏几何行伪装成已显示；`memberCount`／`vehicles` 仍保留全部合法身份的目标。`totalItemCount` 只有整组编码合同成功时为实际总项数，否则为0并有明确错误，不能单独用0推断没有任务项。

每机 exact15：

`vehicleId`、`role`、`geometryAvailable`、`geometryError`、`routes`、`pointCount`、`executionProfileAvailable`、`launchAction`、`endAction`、`encodingAvailable`、`encodingError`、`encodedItems`、`payloadHash`、`unencodedRequirements`、`sourceSchedule`。

几何合法与编码合法独立。某机任一点、坐标声明或双路线不合格时，该机 `routes=[]`／`pointCount=0`，但身份和错误保留，不画半条路线；其他完整成员仍可显示。编码失败则全组 `encodedItems=[]`、hash为空，不把其他成员的有效编码拼成成功结果。错误形状固定5字段 `errorCode/errorDetail/vehicleId/route/pointIndex`；未知定位分别为0、空串、null。编码成功时 `encodingError={}`。

## 几何与源时间

每条 route exact4：`sourcePath`（waypoints／returnPath）、`sourceScope`（member／plan-global）、`sourceMetadata`、`points`。只有成员完全没有 `waypoints` 键时，才可把原 `plan.waypoints` 作为明确标记来源的规划显示；显式空／坏成员路线不回退。不会改变输入 plan，也不会将显示副本传给编码器。

`sourceMetadata` 固定2键 `wrapper` 和 `globalRouteTiming`，分别保留原路线包装器（去除点数组）及实际全局时间元数据，避免同名时间互相覆盖。包装器明确的 geodetic／amsl 可以作为其各点的声明，冲突拒绝；不从成员、HOME、GNSS 或相对高度推算缺失基准。plan／member 若有相反坐标声明同样拒绝该行几何。

每点 exact13：`pointIndex`、`coordinateSystem=geodetic`、`latitude`、`longitude`、`altitude`、`altitudeReference=amsl`、`headingKnown`、`heading`、`holdKnown`、`holdSeconds`、`elapsedKnown`、`estimatedElapsedSeconds`、`sourceTiming`。`pointIndex` 是该路线的0基列表位置，不是 MAVLink sequence。显示支持纬度±80°、经度±180°、AMSL −1000至20000m；所有值必须有限。高度不是离地高。

显式 heading/yaw 只作声明显示；原 Builder 仍拒绝不支持的航向要求，不会因为地图可画而编码成功。hold 0–86400秒，显式0不丢；elapsed为非负估算。未知标量为null，不填0。

`sourceTiming` 仅保留原显式时间／预算／时基／证据标签白名单；它不执行归一化、认证或自动补时间。`sourceSchedule` 固定4键 `plan/member/routes/encoded=false`；plan另保留原 constraints 时间与 routeTiming，routes保原逐点和包装器时间。完整编码成功后复用 Builder 的已校验 schedule，再补其全局 plan schedule；失败仍保原有界诊断。源时间、估算 elapsed 和相对到点停留不混用。

## 编码与结束动作

对未改动的整份原 plan **仅调用一次** `FleetMissionPayloadBuilder::build`。不补全成员路线、执行配置或坐标基准，不拆单机绕过失败。Compiler 的合法共享规划路线可能仍未满足 Builder 的“成员显式双路线”合同：可展示它，但编码必须保留原失败。

`encodedItems` 沿用余段视图 exact9：`sequenceNumber/command/frame/sourcePath/sourcePointIndexKnown/sourcePointIndex/rawItem/canonicalItem/wireQuantization`，逐项核原 items／canonical／sourceItemMap／量化列表长度和 sequence 一致。raw保持原上传度数及13键，canonical保持原 INT E7／float32身份及12键，速度项保留非坐标语义。未知 sourcePointIndex 和地理默认 yaw 均为null；没有再次量化、更改上传数据或重算 payload hash。

每个 launch／end action exact7：`type`、`declared`、`coordinateKnown`、`coordinate`、`encodingKnown`、`encoded`、`sequenceNumber`。缺／坏成员执行配置为 type=unknown、declared=false；合法 launch 为 none／takeoff，end 为 hold／land／rtl。coordinate为原规范 geodetic／amsl五键或空 map。整组 build成功后才根据实际 sourceItemMap 和 Builder 标志确认 encoded及sequence；构建失败时encodingKnown=false，不能误解释成确定没有动作。

实际 Builder 会将出航和 `returnPath` 都编码为 NAV_WAYPOINT 请求，并可编码速度、TAKEOFF、LOITER_UNLIM、LAND和相对驻留；它不是 RTL执行器。RTL/Home不被编码或绘制为凭空生成的返航轨迹。无配置时也可能生成仅含双路线的请求，但不表示起飞／结束语义完整。

TAKEOFF必须与首出航点精确一致；HOLD与返程终点一致；LAND是同XY、明确地面AMSL低于末返程进近高度的独立请求，并须在float32量化后保持上下关系。显示终结坐标不证明地面高度、实际下降路径、着陆检测或任务完成；无限HOLD也不是完成证据。439名义启动段不可与本视图拼成已认证实际路线。

## 原归档空域证据

`airspace` 仅委托 [FleetAirspaceConflictChecker::archivedMissionSpatialEvidence](FleetAirspaceConflictChecker.md)，固定12键：`available/inputState/noFlyZones/zoneCount/minimumSeparationMeters/thresholdSource/report/scope/limitations/coverageCertified=false/clearanceCertified=false/timingCertified=false`。

该值保留同一原 record 的报告和当时输入状态；不重新运行空域检查、不用当前默认值或新遥测补缺项，不把 omitted／legacy unknown 当明确空禁飞区。原报告未覆盖 startup 或结束动作轨迹，不声称新联合检查已完成。

## 静态交付边界

本轮仅源码、文档与静态阅读。未编译、测试、执行 helper、解析器或界面，也未连接设备、修改 PX4、刷机或验证实际飞行。工程师仍需核完整32机／8192项预算边界、Qt5 null桥接、global显示但编码失败、坏几何整行保留、RTL／LAND／HOLD／无配置、归档未知状态及界面分页／地图图层隔离。

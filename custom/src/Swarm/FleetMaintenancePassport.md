# FleetMaintenancePassport 维护护照契约

## 518：不可变导出与独立只读文件工作区

新增`workspaceState`／`workspaceResult`只读值和`requestWorkspace(exactIntent,confirmed)`，由Root实际Passport私有配对提供窄门面，原可变records/import/load仍不外露。入口精确9键，异步freeze/open/save/groups/records/detail各获原生jobId；clear-file／clear-frozen同步只退休准确槽、保最后文件回执，cancel请求准确当前job且幂等不重复通知。

原实际records／instance／revision被冻结；GUI每turn最多256工作步，只核允许的纯JSON类型／数值与depth24、262144节点，拒QObject／自定义QVariant及有损整数转换，不改掉原记录。文件I/O、32MiB JSON规范化／评估及详细文本准备在一次性worker本线程完成，回执与线程结束双尾后接纳。构造保持惰性，worker临时Passport不会再启动worker。

最多两个私有值cache：session和file。新冻结或打开失败保原槽；文件历史从不import替换活动护照。原517追加仍可进行并推进非回绕记录版本，旧session内容不变，`sourceCurrent=false`不影响操作员明确保存原已预览内容；它不表示当前飞机／电池身份或维护状态。字节hash只对应实际冻结／读入bytes，不认证来源。

公开descriptor18、组目录每页8行、记录目录每页8行。完整记录与组assessment以canonical JSON按2048 UTF16代码单元连续片浏览，末端不割代理对；详情单独64MiB上限，不把整条巨大note或modification交QML。选文件路径本身不I/O，保存需另确认可能覆盖，QSaveFile禁止直接写回退。`fileWrittenKnown/fileWritten`与`readbackVerified`分开，实际commit不因取消／回读失败被抹掉；fsync／durable／认证固定false。

[工作区完整合同](FleetMaintenanceWorkspace.md)、[48项未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅新增显式操作，不自动备份、清理文件、跨站同步、恢复当前状态或飞行权限；原维护预测／长期运营目标继续。下方517是历史入库合同，其“不自动保存”继续成立，本节仅新增另行确认的文件入口。

## 517：真实历史页接入与来源分组

新增`appendTelemetryHistory(actualArchive, previewId, confirmed)`，返回exact5 `{appendAccepted,error,assessment,previewId,historyScopeId}`。仅实际Archive当前私有预览可提供原record，Root固定实际对象配对，不接caller提供的历史指标。其余泛用写入接口仍不向公共QML门面暴露。

schema1记录新增成对严格`historicalTelemetrySource`（7字段）与`historicalTelemetryEvidence`（21字段）；完整保留并由完整source重新算scope，不信输入hash，也不把hash当认证。新历史记录仅允许两项真实温度／电流及固定历史元数据，不夹带寿命／飞行次数／振动或权限。import整批拒绝同scope序号闭区间重叠，失败不部分替换。满2000明确拒绝新增，不再自动淘汰旧去重依据。

纯legacy文件仍为原schema1根5键；含历史记录时导出根6键，必须增加`historySourceContract='merivus-telemetry-history-source-v1'`。新导入要求该精确字符串与实际历史记录同时存在；坏型、缺失或无历史记录却夹入标记均拒绝。原旧版根字段白名单会拒绝新字段，不能把未修改的历史文件静默读成丢失来源的legacy记录。该哨兵是格式防降级，不是数字签名或防篡改认证。

`assessAll()`分别输出legacy手工组与各完整历史组；`assessVehicle()`面对多个来源组返回未知及选择提示，不按同数字ID混合。legacy仍按时间趋势，历史仅按原序号选择最新页，不把页峰值的两点差当作物理趋势；`historicalOnly=true`、`latestHistoricalPageOnly=true`及证据缺口说明限定范围，不等于同一物理飞机或当前维护健康。

当前维护页只追加本进程记录，不自动保存。既有导出/文件接口完整保新metadata，本批不新增保存／重开UI。历史样本时间保真实原收包UTC，不用导入时刻刷新温度。通用45A／80°C等阈值仍未针对机型校准；真实维护记录、长期模型和工程师验证继续原目标21。

[完整数据与API合同](FleetMaintenanceHistoryImport.md)、[40项未执行场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。仅源码与直接静态核对，未编译、执行项目、测试或放行。下方旧API说明由本节对历史来源的限定补充。

`FleetMaintenancePassport` 是地面站侧的维护记录和风险估算模块。手工记录按机号分组，历史遥测按完整档案来源分组；它整理已有维护指标并输出证据缺口、阈值原因、建议检查项和估算风险，不认证分组对应同一物理设备。

## 边界

- 只保存和计算维护证据，不访问 `Vehicle`、MAVLink、参数系统或任务执行器；
- 不发送命令、不修改 PX4 参数、不决定解锁/起飞/返航，也不替代工程师放行；
- `estimatedRiskScore`、`predictiveRisk` 和趋势均为启发式估算，不能宣称为寿命预测或安全证明；
- `manualReviewRequired=true`、`flightCommandReleased=false` 是固定安全边界；
- 导入 JSON 是维护历史快照，不是飞控配置或权限文件。

## API

- `appendRecord(record)`：规范化并追加一条记录，返回该机维护评估；缺少有效 `vehicleId`/`systemId`、有效 `timestamp`/`recordedAt`（ISO 8601）、越界指标/阈值或坏的改装记录时拒绝；明显未来时间戳也会被拒绝（允许小幅时钟偏差）；
- `assessVehicle(vehicleId)` / `assessAll()`：手工组按最新时间和趋势估算；历史组按本节独立来源与原序号规则，不混合成当前飞机健康；
- `exportJson()` / `importJson(json)`：导出/导入 `schemaVersion=1` 的维护历史；导入严格拒绝未声明的根字段和非约定 `riskModel`，避免新版本语义被旧版本静默丢弃；
- `saveToFile(filePath)` / `loadFromFile(filePath)`：使用 `QSaveFile` 做本地原子文件交接，拒绝空路径、读写失败、非法 UTF-8 和超过 32 MiB 的输入，并在完整校验后才替换历史；
- `clear()`：清空本地维护护照。

记录必须提供可解析的 `timestamp` 或 `recordedAt`。系统统一保存 UTC `timestamp` 和 `timestampMs`，不会把缺失/非法时间替换成当前时间；手工组按时间排序。历史页保原收包UTC及回退诊断，按原序号选最新页，禁用物理两点趋势。

记录可以提供：

`currentAmps`/`peakCurrentAmps`、`temperatureC`、`vibrationRms`、`batteryDegradationPercent`（或电池健康百分比）、`flightCount`、`modificationRecords`，以及可选 `thresholds`。改装项若没有 `verified=true` 会被列为待人工核对。缺失指标不会被当作 0，而会出现在 `evidenceGaps`。

`temperatureC` 接受 -100 至 300 摄氏度的有限数值；负温度会保留在记录和趋势估算中，不会被误判为缺失。其它维护指标仍按各自非负范围校验。

当前阈值是通用审阅默认值（45 A、80 °C、1.0 RMS、20% 电池衰减、500 次飞行），不代表任何具体机型的限制；工程师必须用实际机型维护手册、试验数据和经批准的配置替换或校准它们。维护记录及 `verified=true` 改装标记仍属于未签名的本地输入，不能作为不可抵赖的维修或放行证明。

维护状态只使用 `unknown`、`monitor`、`inspection_due` 和 `grounding_recommended`。它不是“可飞”标志；任何现场使用都必须由工程师复核记录完整性、阈值来源和实际硬件状态。缺失或无法解析的时间戳属于输入错误，不会生成维护评估。

# FleetMissionDigitalTwin

## 540：完整每机路线的相对时间化预演

`mission.plan` 与 `energy.plan` 必须来自调用方同一条 PlanStore 审阅记录；本类不把摘要或文件哈希升级成实际取源认证。`preview()` 一次调用 [FleetMissionPreviewTimeline](FleetMissionPreviewTimeline.md)，追加 `planningTimeline` 小摘要；真实事件集合只保存在 native 私有槽，不塞进公开大结果。旧 timeline 字段仍只是兼容的步骤摘要。起飞、实际首点入场、末动作与真实耗能没有完成模型，不能把该预演称为整任务仿真通过或 StartPermit。

`timelinePage(expectedSnapshotVersion, expectedPreviewHash, offset)` 只读当前实际已安装 preview 的 4 条事件，回复闭 10 字段：ok/error/snapshotVersion/previewHash/offset/nextOffset/totalCount/hasMore/rows/historicalOnly。version/offset 必须是真实非负整数，不接 bool/string/小数；旧版本、旧 hash、不可用模型与越界返回空 rows。换源/故障变化/clear 立即退休事件槽，`previewWithFaults()` 不安装或替换当前分页资格。

本地版本每次完整替换和 clear 单调递增；文件声明版本只能提高下界，不能重用当前页身份。达到 INT_MAX 后拒绝新替换，不回绕；clear 可清内容但不解锁已耗尽计数。captureSnapshot 受理后若通知中对象被销毁或重入换代，返回空 map；正常返回本次完整 snapshot。preview 安装后发通知，返回事先冻结的局部结果，失源/销毁返回空，不在通知后读已毁对象。导入 faults 与五快照一次安装，通知尾核实际代次。SHA-256 仅完整性标识，不认证文件身份、路径或飞机。

UI 应仅在操作员明确捕获当前计划时冻结完整 plan；周期遥测不应自动覆盖所选计划。历史快照与当前源可用性分开显示。工程师仍需覆盖异机不同路径、独立返程衔接、坐标/高度基准缺失、速度缺失、驻留 0/量化、分页换源与通知重入等场景；本批未运行、编译、测试或刷机。

`FleetMissionDigitalTwin` 是地面站侧的纯数据数字孪生入口。它接收五类调用方快照：

- `fleet`：机队成员、构型/健康和在线状态；
- `mission`：任务/计划、成员分配、航点、航段和预计时长；
- `map`：坐标系、禁飞区、地理围栏和障碍物摘要；
- `link`：每机链路质量、延迟、丢包和链路状态；
- `energy`：每机剩余电量、剩余续航和能量证据。

推荐用 `captureSnapshot(fleet, mission, map, link, energy)` 一次性更新输入。调用会产生递增的 `snapshotVersion`；`setSnapshot()`、`exportSnapshotJson()` 和 `importSnapshotJson()` 使用 `schemaVersion=1`、非回退版本化信封，`saveToFile()`/`loadFromFile()` 提供 32 MiB 限制的本地原子文件交接。持久化导入必须提供完整信封，验证格式、版本、五类 map、SHA-256 快照哈希和故障条目类型后才替换当前状态；本地新版本还须递增，不重新使用文件版本作为分页身份。文件接口还会拒绝空路径、读写失败和非法 UTF-8。

`preview()` 将输入归一化、按稳定 ID 排序并生成确定性时间线、机队/任务/地图/链路/能量摘要、风险码和 `faultSuggestions`。结果包含 `deterministic=true`、`snapshotHash`、`previewHash`、`manualConfirmationRequired=true`、`recommendationOnly=true` 和 `flightCommandReleased=false`。缺少证据会保持未知（例如平均链路或能量为 `-1`），不会被当作安全或完成。

`injectFault(kind, details)` 只追加场景覆盖，支持 `vehicle_offline`、`link_loss`、`link_degraded`、`low_battery`、`gnss_degraded`、`leader_failure` 和 `communication_delay`（也接受少量别名）。`faultSuggestions()` 返回重分配、拆分、等待/返航、检查定位或增加时序余量等人工复核建议。建议没有副作用，不改任务、不选择飞机、不访问 `Vehicle`/MAVLink/PX4，也不会发送命令；新的 telemetry 快照不会隐式清除故障，必须显式调用 `clearFaults()`。

该模块是可重复的审阅预演，不是飞控仿真、碰撞证明、能量安全证明或真实故障保护。地图、链路、风场、动力和飞控执行模型仍属后续源码开发；工程师承担构建、Mock/SITL、故障注入和硬件实操验证，不是接替未完成的软件开发。

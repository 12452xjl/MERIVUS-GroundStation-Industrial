# FleetTaskTemplateRegistry 契约

`FleetTaskTemplateRegistry` 是地面站的行业任务模板目录。它保存可审阅的意图元数据和约束，供任务向导生成预览；不会选择 `Vehicle`、上传 Mission、调用 MAVLink 或执行任何命令。

目录注册、删除、启用和 JSON 文件交接接口仅供受控 C++ backend 使用，未暴露为 `Q_INVOKABLE`。QML 侧只读消费 `templates`、查询/校验结果；默认模板在 C++ 构造阶段注册，避免界面脚本绕过审批边界替换任务约束。

## 模板记录

每个模板都带有三段式 `version`（例如 `1.0.0`）、`taskType`、能力标签、建议机数、最大任务时长、默认参数和 `constraints`。重复注册会覆盖元数据，但会保留当前 `enabled` 状态，避免一次目录同步意外启用已禁用模板。

`constraints` 可使用以下键：

- `requiredCapabilities`：在模板能力标签之外追加能力。
- `requireVerifiedCapabilities=true`：只使用车辆显式 `verifiedCapabilities`（或 metadata fallback）满足模板能力，运行时 `canUse*` 观测不会绕过该门槛；模板 constraint 或任务字段可声明，任务字段优先但类型错误会阻断。
- `minVehicleCount` / `maxVehicleCount` / `maxDurationSeconds`：任务级数量与时长门槛。
- `requiredTaskFields`：任务请求中必须提供的非空字段。
- `requiredVehicleFields`：每架候选机必须提供的快照字段。
- `resultFormats`：结果格式白名单，例如 `json`、`geojson`、`csv`。

## 能力与任务校验

`validateTask(templateId, task, fleet)` 只返回审阅结果。它合并模板、模板约束和任务请求中的能力标签，检查任务类型、机数、时长、必填字段、结果格式，以及每架飞机的在线/健康状态、能力标签和必需快照字段。返回值包含 `valid`、`reasons`、`eligibleVehicleCount`、`rejectedVehicles`、`missingFleetCapabilities`、模板版本和结果格式。

缺失健康状态默认按 `health_unknown` 拒绝；仅当任务明确设置 `allowUnverified=true` 时才允许规划性预览继续统计候选。结果固定带 `manualConfirmationRequired=true` 和 `flightCommandReleased=false`。这不是任务批准，也不构造执行计划。

`allowUnverified`、`requireVerifiedCapabilities`、`online` 和能力标志必须是实际布尔值；车辆 `systemId/vehicleId/id` 必须是 1–255 的有限整数且别名一致，数字字符串或冲突别名会被拒绝。任务/车辆能力列表只接受 String/StringList/List<String>，损坏类型会进入 `required_capabilities_invalid` 或车辆拒绝原因，不会由 QVariant 宽松转换。

## JSON 交接与签名占位

`exportJson()` 生成带 `format=merivus.fleet.task-template-registry`、`schemaVersion=1` 和稳定排序模板数组的 JSON。`importJson()` 先解析并校验全部模板、版本、数量、约束范围、字段类型、合法 ID、整数计数和重复 ID，并拒绝未知 root/template/signature 字段，再一次性替换内存目录；任意错误都会拒绝导入并保留原目录。`saveToFile()` 使用 `QSaveFile` 原子提交，`loadFromFile()` 拒绝空路径、读写失败、非法 UTF-8 和超过 32 MiB 的输入，并沿用“先校验、后替换”语义。

每条模板的 `signature` 仅包含 `algorithm=external-unverified-placeholder`、`status` 和 `value` 字段。非空值只表示外部系统提供了待核验材料，地面站不会验证、信任或据此启用模板。生产接入必须由受控签名服务完成算法、密钥、撤销和审计设计。

## 工程师静态验收

确认版本不合法、约束范围反转、重复模板 ID、损坏 JSON、超出模板机数/时长、缺少能力或健康未知时均得到可解释拒绝；确认导入失败前后 `templates()` 不变。源码只依赖 Qt 的 JSON/容器类型，不访问 Vehicle、MAVLink、PX4，不自动运行外部命令或编译测试。

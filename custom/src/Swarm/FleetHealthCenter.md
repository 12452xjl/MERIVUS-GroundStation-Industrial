# FleetHealthCenter 观测与门禁契约

`FleetHealthCenter` 是地面站内的只读健康聚合层。它把 `FleetRegistry` 的能力/在线健康快照和 `FaultToleranceManager` 的故障策略报告合并成一个给 QML 使用的视图，并提供可解释的起飞、任务门禁建议。

## 460：实际原生电池负面进入任务门禁

`vehicleHealth()` 先对本次实际 `FleetRegistry.vehicles()` 行调用 [共享电池解析器](FleetNativeBatteryEvidence.md)，在私有局部表保留检查结果，再做原有 FTC 保守合并。不能对 FTC 已降低过电量别名的合并视图重新冒充原生 DTO 校验，也不能让 FTC 的旧正常电量或同名字段覆盖当前电池故障。

每条输出无条件重新生成只读 `nativeBatteryHealthCheck` 四字段：`required`、`valid`、`reportedUnsafe`、`reason`。它来自上述同次实际 Registry 行，不接受 FTC/metadata 同名诊断；无 Registry 的 FTC-only 行也不会因此得到原生依据。`required=false` 不等于原生电池已健康，legacy 无 marker 路径不新增一律阻断。

- 合法、当前来源且 fresh 行明确非零 `faultBitmask`：具体机器码 `native_battery_reported_fault`；明确不安全 `chargeState`：`native_battery_reported_charge_state_unsafe`。辅助电池负面也不能被推进聚合忽略。健康状态保守变为 critical，起飞/任务门禁追加 block 原因。
- 必需证据无效或不可用：保守变为 unknown（保留原 critical/offline），评分为 0，门禁追加来源/证据原因；不能被正常 FTC 缓存提升。过期/失源不解释成新的明确故障。同源已报告负面行过期时，`native_battery_fault_evidence_stale` 保持未知阻断，不因为时间流逝恢复可用。
- schema 1 或 schema 2 未观察扩展的未知状态不单独构成故障；只要原共享可用性条件满足，不因旧硬件缺扩展而自动禁用。未知扩展不是确认无故障或性能证明。

门禁只消费本次自身生成的四字段结果，原生电池阻断ID还会独立去重并入最终blockingVehicleIds，不依赖显示健康状态分级或仅追加block文字；同时继续原在线、SYS/EST、位置、低电量、链路与Leader检查，不清除其它阻断或未知原因。该结果不是认证、放飞许可、参数写入或设备操作。无新增公开方法或UI。本节仅源码接线与静态交接，未编译、测试、运行helper、应用、仿真或设备。

## 376–380 当前增量：原包年龄与保守合并

本阶段累计增量为 7 个源码文件、19 份文档；本页仅说明健康消费链，不代表全部 25 个开发目标或原六类剩余工作完成。源码链路为：实际 `Vehicle` 的原生 SYS_STATUS / ESTIMATOR_STATUS 独立观测 → `SwarmController` 当前描述符 → `FleetRegistry` → 本模块。参见 [Vehicle 源码](../../../src/Vehicle/Vehicle.cc)、[Swarm 适配](SwarmController.cc)与[健康聚合实现](FleetHealthCenter.cc)。本页下文的接入示例仍有效；旧“有限缓存坐标即可成为健康证据”的口径已由本节覆盖。

`fleetHealthTelemetryObservation()` 是独立的原生只读账本：在插件前捕获实际协议、原 Vehicle/default component 和 PRIMARY 来源的 SYS_STATUS / ESTIMATOR_STATUS 原包，每路分别保存本机单调接收年龄、receipt 与语义 revision。正常时间流逝不是新 receipt，也不会把年龄重置为 0；源失效、语义变化及 EST 时间回退按原生账本规则处理。它不是远端身份、报文签名、请求因果或整机健康认证。

Swarm 只在两路 mandatory 原生证据均符合实际来源、严格类型与新鲜度条件时，给出 `faultHealthKnown=true`，故障证据年龄取两路真实年龄的最大值。任一路缺失、失效或未知均写 `faultHealthKnown=false / faultEvidenceAgeMs=-1`，不借注册表刷新制造新原包。全球/局部位置另查各自原生 GLOBAL_POSITION_INT / LOCAL_POSITION_NED 的 available、fresh、有限值及估计器状态，不以显示缓存仍有限来推定新鲜。

本模块合并时遵守以下许可上限：

- 当前 Registry 描述符是许可证据基线；FTC 旧缓存只能更保守，不再用 `_mergeMissing` 或同名健康覆盖补齐许可字段。FTC 的 healthy、online、Known=true 或较小年龄都不能改善 Registry 当前的 bad、unknown、offline 或 stale。
- estimator / position 的 Known 必须与各自严格 bool 健康字段配对，两源合并取交集。单个源只有 paired Known 明确 true 时，其 Healthy=false 才构成明确负面；Known=false + Healthy=false 的清空占位仍为未知，不凭空生成 critical。各源合并前的真实已知负面，经保守状态保留，不因合并后的 Known 交集变 false 被擦除。
- 任一路明确 `hasFault=true` 独立保留；offline、critical 及已有故障/健康原因不能被另一份正常缓存清除。状态保守顺序为 critical、offline、unknown、degraded、healthy，unknown 不被 degraded/healthy 提升。
- 合并后的故障 Known 要求两路各自存在严格 bool Known/hasFault 与非负、有限、整数年龄配对；已知年龄取真实两路最大值，不配对或缺失则 false/-1，不补年龄 0。数值、字符串或 typed-null 不按 bool 健康值接受。
- 电量/链路只能在真实有效值间取较低值；旧缓存未知不擦掉 Registry 的低电量/低链路阻断，正值也不填 Registry 未知。没有 Registry 行的 FTC-only 报告仍可诊断展示，但三个 Known 全为 false、故障年龄 -1、显示评分 0，不能允许执行。评分不是原包新鲜度或放飞许可。

optional ESC/FTC 缓存和 health/arming report 只可增加保守负面或未知诊断；它们不是本轮新增的 ESC/FTC 原包 fresh 证明，也不能代替 EST 原包让 estimator Known 变 true。没有改动原 Flight6 / RAW32 合同或既有参数 schema。本轮并未补齐所有启动阶段的 EST 连续性/缺口处置、可信 HOME/local/净空、整体最终 StartPermit、正式远端执行/完成归属或长期授权/容量生命周期；`outboundIntegrationComplete=false` 保留，完整范围仍见[路线图](../../../docs/MERIVUS_FLEET_OS_ROADMAP.md)。

以上是源码接线与限定静态交接，不是工程验证结论：未 compile/test、运行 helper/应用/UI/认证或设备，未刷机、更新 PR 或生成新包。

## 接入

```qml
FleetRegistry { id: fleetRegistry }
FaultToleranceManager { id: faultPolicy }
FleetHealthCenter {
    id: healthCenter
    fleetRegistry: fleetRegistry
    faultToleranceManager: faultPolicy
}
```

`healthSummary` 汇总 `healthy/degraded/critical/offline/unknown` 数量、在线数量、总体状态、阻断标志和逐机原因；仅 0–100 的有效 `healthScore` 参与平均值，`scoreKnown=false` 表示没有可用评分证据，不能把显示的 0 当作健康分数；`vehicleHealth` 返回按 `systemId` 排序的合并快照。

门禁输入中的目标 ID 只接受 1–255 的有限整数；车辆快照里的 `systemId`、`vehicleId`、`id` 别名必须一致，字符串、小数、非有限值或别名冲突都会被视为无效目标。

## 门禁

- `evaluateTakeoffGate(vehicleIds)`：仅检查所选飞机是否在线、健康是否已知、是否有故障、电池/链路是否达到保守阈值。
- `evaluateTaskGate(vehicleIds, requireLeader)`：在起飞条件上增加安全 Leader 检查。
- 返回值包含 `allowed`、`state`（`ready/review/blocked`）、`blockingVehicleIds`、`warningVehicleIds` 和 `reasons`。每个 `reasons` 条目包含 `vehicleId`、机器码 `code`、`severity` 和可展示的 `message`。

未知健康、离线、严重故障、临界电量、极差链路或缺少当前故障评估默认阻断；`degraded`、低电量预警和中等链路质量只返回 `review`。`faultHealthKnown` 必须为 true，且 `faultEvidenceAgeMs` 必须是非负、未超过 `FaultToleranceManager.offlineTimeoutMs` 的有限值；FaultToleranceManager 的部分更新还必须把 known 标记和年龄作为同一份配对证据提交，不能用旧的零年龄填充新的 known 标记。`hasFault=false` 在 known=false 时只是清除占位，不代表确认无故障。这是地面站建议，不替代飞控解锁/起飞检查。

位置证据按上文当前原生消费链独立核新鲜度。实际来源标记为 `positionEvidenceSource=native_global_position_and_estimator` 或 `native_local_position_and_estimator`，未满足条件时为 unknown；局部位置用于无 GNSS 的证据分支，但不等价于全球 Mission 坐标、局部原点或净空可用，任务项坐标语义和最终解锁/任务检查仍以 PX4 为准。

## 边界

- 模块不发送 MAVLink，不调用 `Vehicle` 命令，不写参数，也不改变 Leader 或任务编排状态。
- 它只读取已接入的两个 QObject；上游实际原包接收与 Swarm 适配已有源码接线，工程师仍须验证真实协议/来源、断流、年龄推进、未知/负面配对与最终门禁行为，不能以源码存在代替验证。
- 当前阈值取 `FaultToleranceManager` 配置，故障证据新鲜度也跟随 `offlineTimeoutMs`（默认 3000 ms）；未接入策略时使用保守默认值（临界电量 15%、低电量 25%、链路临界 30%、预警 50%）。
- 本模块只做源代码静态交接，尚未执行 Qt 编译、仿真、真机测试或刷机放行。

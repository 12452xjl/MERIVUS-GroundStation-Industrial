# 分阶段编队、多机动作与实时任务隔离安全评审

## Command

- Intent/action: 1～6 机使用同一编队事务；批量起飞/降落/标准返航；普通右键 goto；Shift 临时多航点 Mission。
- Risk class: 高。
- Target source: 地图框选或排他单选。
- Frozen target system IDs: 编队成员在 PREPARE 前冻结，必须包含 UAV-1；Shift 任务在首个航点时冻结。

## Preconditions

- [x] Link and telemetry freshness
- [x] Unique system IDs
- [x] Position and RTK quality（本次校验有效位置；RTK 质量仍由飞控健康检查负责）
- [x] Flight mode/capability
- [x] Armed state and altitude
- [ ] Battery/geofence/mission constraints（沿用 Vehicle/PX4 检查，尚未形成统一策略层）

## Lifecycle

- [x] Requested/validated/confirmed
- [x] Queued/sent
- [x] ACK/rejection（编队 PREPARE 在本机预检后 ACK；COMMIT 周期返回 IN_PROGRESS，到达 Ready 后最终 ACK；RELEASE/ABORT 逐机聚合）
- [x] Executing/completed（全部成员 Ready 后才 RELEASE；地面站只有收到所有 RELEASE ACK 才标记 formationActive）
- [x] Timeout/cancel/retry
- [x] Partial-result reconciliation

## AI boundary

- [x] Versioned structured schema（不适用 AI；人工 QML 到固定 C++ API）
- [x] No free-form MAVLink fields
- [x] Ambiguity rejected
- [x] Coordinates/units validated
- [x] Deterministic preview and operator confirmation
- [x] Replay boundary（命令和位置租约绑定非零会话 ID 与 PREPARE source；故障注入测试仍待执行）

## Verification

- Mock/SITL scenario: 1～6 机四阶段事务；批量动作、跨机独立任务、Shift 替换和完成清理。
- Fault injection: 错误成员位图、过期会话、部分 ACK 丢失、PREPARE/COMMIT/RELEASE 拒绝、任一成员断链、地面站退出、Mission 上传/清理失败。
- Audit record: 当前为 UI 结果与日志；尚未接统一持久审计。
- Rollback/abort: 任一事务阶段失败或“结束编队”都会停止位置租约，向冻结成员发送 USER_4 ABORT，并等待 AUTO_LOITER ACK。
- Reviewer decision: 采用协议版本 `2`；1～6 机必须依次完成验证，不得以静态检查代替 SITL 和实机放行。

## Remaining risks

- 当前仍使用通用 `MAV_CMD_USER_1～4`，尚无独立 MERIVUS MAVLink dialect 和能力发现；地面站与固件必须成对发布。
- 地面站失联后所有成员依靠 3 秒会话租约超时转 AUTO_LOITER；该保护仍需 SITL 故障注入验证。
- 当前未校验 RTK fix type、水平精度阈值和初始编队几何，双机前仍需补齐或采用足够保守的试验间距。
- 临时 Mission 完成判定使用末航点距离和地速阈值，仍需 SITL 实测校准。
- 配套 PX4 源码位于 `E:\MERIVUS\FirmwarePX4`；地面站已完成 Release 构建，但 PX4 本机工具链不可用，实机前仍必须完成 SITL、`px4_fmu-v6c_default` 构建、刷写追溯和现场人工验收。

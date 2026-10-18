# Vehicle 普通启动 RAW 原包入口绑定（457）

仅地面站源码实现及直接静态阅读，未编译、执行或测试。此处不修改 PX4，不认证命令因果、物理参考、空间净空或最终 StartPermit。

## 原包在任何回调前绑定

`Vehicle::_mavlinkMessageReceived` 先保存原始 MAVLink 包和本机原 VERSION 单调时钟的收到时刻。对于 COMMAND_ACK、LOCAL_POSITION_NED、HOME_POSITION、ODOMETRY、HEARTBEAT、GLOBAL_POSITION_INT、EXTENDED_SYS_STATE、MISSION_CURRENT、MISSION_ITEM_REACHED，在调用 origin／ALT／电池／健康准备、遥测／执行 observer、Link 虚方法或插件之前，建立 `_fleetStartupReceiveDepth` 整栈 RAII 栅。

同一入口冻结私有 `FleetStartupRawIngress`：实际 owner／PRIMARY／protocol 的 QPointer、startupId／ownerEpoch、原 journalStepId、**该 journal 收据自身**的 operationId 和 issuedAt、VERSION／flight／health 来源 epoch、vehicle／component、原收到时刻和原 raw receipt sequence。下一步已经准备但还未 write 时，journal 仍属于上一实际已发步骤，不能改用当前新 slot 的 operation。

只有入口时已有原已发 journal，或同一步实际 write 调用正在进行，才有记账资格。排队前、非原 protocol／来源／成员的包不能因为后续回调建立新步骤而获得资格。v1 未要求 ODOMETRY 连续性时，ODOMETRY 不变成新的周期性前提；普通遥测路径仍按原逻辑处理。

## Handler 只处理原 journal

`_handleFleetStartupRawMessage` 接收该入口快照；在可回调的 `_fleetStartupTransportCurrent(false)` 前后核同一原身份、收到时刻和序列，回调后重新查原 retained record，不跨回调保存容器迭代器。坏帧、倒退、嵌套接收或处理中被丢弃，不可被后来正常包抹成从未发生。

接收 depth 不为 1，或回调后原 raw 序列已变化，令原 journal 失效，不将同毫秒嵌套接收逆序拼接。退出 guard 对尚未成功处理、但原 scope／journal 仍相同的合格包，锁存 `journalExhausted=true` 和未知 gap（-1）。如果原 scope 已取消或换成新 scope／step，则不碰新记录。

sender 已 closed 不是停止 journal 的理由：原 scope 未退休时继续记录至整组屏障／下一次真实排队。真实 write 内的同步响应可以保留，但 `requestIssued` 仍仅在实际 write 返回后公布，不倒推响应由该请求导致。

原 step schema3／33 字段保持；没有新增 QML／JSON 入口或执行权。

## 新 prepared 身份先经过完整复核

发送入口先以先前完整组核验检查原状态；建立新 unissued slot 后，先执行原 `authorizationFence` 完整整组门禁，再执行 `current()` 的纯命令上下文核验。这让新的 prepared 身份经过重新审阅，而不是拿旧组 memory seal 对新 slot 作必然失败的比较；没有跳过整组 seal 或允许 prepared slot 冒充已发证据。

完整组最终核验见 [FleetStartupMemorySeal](FleetStartupMemorySeal.md) 和 [StartupTransaction](../../custom/src/Swarm/FleetMissionStartupTransaction.md)。本入口修复不证明 Transition 全部允许运动域包含于已审空间，原 Compiler pending 保持。

## 工程师后续验证事项（本机均未执行）

- 在最早 observer／Link 回调中请求下一步骤，确认原包不能归入新步骤，且接收深度直至整栈返回才释放。
- 原已发步骤 closed、下一步仅 prepared，以及真实 write 内同步回包，分别核 journal 的原 step／operation 与实际 `requestIssued` 时序。
- 同毫秒嵌套 RAW、嵌套 EST、坏包、来源变化、收到时间倒退、early return 和对象销毁，核原 journal 失效锁存而不污染新 scope。
- 未要求 ODOMETRY／ALT 的原政策继续兼容；取消和换 owner 不恢复旧 journal、旧批准或旧期限。

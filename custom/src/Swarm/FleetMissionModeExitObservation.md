# FleetMissionModeExitObservation — 541

纯值校验器 `valid(map, expectedSourceEpoch="", expectedCount="")`，没有 QObject、文件、网络、包发送、授权或状态恢复。仅验证本地接收顺序声称的闭形状，不认证飞机、文件、退出原因、请求因果或正式结果。

`missionModeExitObservation` 闭 9：schemaVersion=1、observed、count（规范 UInt64 十进制字符串）、candidate、first、last、requestCausalityVerified=false、remoteOutcomeVerified=false、automaticActionsIssued=false。count=0 时 observed=false、candidate=not-observed，首末为 null；正数时 candidate=mission-mode-exit-observed-not-outcome。

首末各闭 6：customMode（完整 UInt32）、baseMode（UInt8）、systemStatus（有效 MAV_STATE）、priorMissionReceipt、exitReceipt、continuityUnknownAtExit。数值只接实际整数元型或有限且为整数的 JSON Double，不接 Bool/String。receipt 闭 3：规范同源 UUID、正 UInt64 sourceSequence 字符串、0..INT64_MAX receivedMonotonicMs 字符串。prior 序号严格早于 exit、时间不晚于 exit；exit 的实际 PX4 mode 谓词必须不是 AUTO_MISSION。

单次首末全部语义相同；多次最后 prior 严格晚于首次 exit、时间不回退，count 不得超过首末序号区间能容纳的 mission→nonmission 对数。可传原行 epoch/count 进行强相等核对。源间缺口/continuityUnknown 不被消除，不证明两包之间物理连续；保留的是采样退出候选，而非连续时间模式或退出原因。

无成功/降落/停飞/远端完成推断，无命令、预约释放、每帧审计或新的自动动作。本批仅源码与静态阅读，未执行校验器、解析器、编译、测试或实机。

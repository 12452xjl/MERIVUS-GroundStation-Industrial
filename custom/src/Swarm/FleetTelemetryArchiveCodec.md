# FleetTelemetryArchiveCodec：原生入站遥测观察值编码

310–314：接纳 Worker 新增的固定 `telemetry-directory-inventory-failed` 错误码；错误白名单为 23 项，Header19/Frame29/Footer31 的磁盘格式不变。目录原生枚举是否正常结束由读取器单独报告，不能提升本 Codec 的认证、完整遥测或目录全集旗标。

本模块是非 QObject 静态值编码器，编码已接收的固定白名单 MAVLink 解析结构，并校验只读历史 journal 的固定头/尾格式。原生 Vehicle、物理链路、接收入口和 sourceEpoch 的接纳由调用端负责。Codec 不请求数据流、不发送命令、不访问文件、数据库、飞控或设备，也不能把任意 C++ 调用者构造的值认证成真实飞机数据。

## 固定覆盖范围与隐私

白名单 ID 和长度直接引用本仓库生成头的宏；没有该宏的 dialect 不能凭相同数字隐式放行。当前本地头中全部 27 种定义存在：基础 19 种、额外定位 8 种。基础集合为 HEARTBEAT、SYS_STATUS、EXTENDED_SYS_STATE、BATTERY_STATUS、ATTITUDE、ATTITUDE_QUATERNION、RAW_IMU、SCALED_IMU/2/3、HIGHRES_IMU、VIBRATION、ESTIMATOR_STATUS、ESC_INFO、ESC_STATUS、ACTUATOR_OUTPUT_STATUS、SCALED_PRESSURE/2/3。

只有显式 `includePositionTelemetry=true` 才接纳 GPS_RAW_INT、GPS2_RAW、LOCAL_POSITION_NED、GLOBAL_POSITION_INT、HOME_POSITION、ODOMETRY、POSITION_TARGET_LOCAL_NED、POSITION_TARGET_GLOBAL_INT。此同意涵盖全球坐标、本地轨迹、驻地/返航点和已发布的位置目标，不只是 GNSS；禁止定位时整条拒绝，不以删除一两个字段后仍声称原包完整。POSITION_TARGET 是观测到的既有目标报告，不是 SET_POSITION_TARGET 命令的执行入口。

其它消息固定拒绝。尤其不复制 AUTH_KEY、SETUP_SIGNING、UID/版本身份包、参数字符串、STATUSTEXT、FTP/TUNNEL/SERIAL_CONTROL、不透明日志、图像/视频、相机 URL、MISSION 或 SET_POSITION_TARGET 控制消息。HIGH_LATENCY/2 同样不在白名单，不能因包含电池/健康字段而忽略其经纬度及 custom 字段。原生 payload 包含时刻、系统/组件 ID、传感器/动力状态及（获得同意后）位置；Base64 不是加密、匿名化或非敏感证明，需部署方管理文件权限与留存。

长度依据现有生成头（min/max，字节）：HEARTBEAT 9/9，SYS_STATUS 31/43，EXTENDED_SYS_STATE 2/2，BATTERY_STATUS 36/54，ATTITUDE 28/28，ATTITUDE_QUATERNION 32/48，RAW_IMU 26/29，SCALED_IMU/2/3 各 22/24，HIGHRES_IMU 62/63，VIBRATION 32/32，ESTIMATOR_STATUS 42/42，ESC_INFO 46/46，ESC_STATUS 57/57，ACTUATOR_OUTPUT_STATUS 140/140，SCALED_PRESSURE/2/3 各 14/16；GPS_RAW_INT 30/52，GPS2_RAW 35/57，LOCAL_POSITION_NED 与 GLOBAL_POSITION_INT 各 28/28，HOME_POSITION 52/60，ODOMETRY 230/233，两类 POSITION_TARGET 各 51/51。

## 帧结构与值边界

`Record` 精确持有 `ok/errorCode/lineJson/runId/sequence`。成功的 lineJson 是不含换行的紧凑 UTF-8 JSON，最多 4096 字节；换行由 worker 单独追加。失败仅返回固定 errorCode，不含消息 payload、自由文本或路径。runId/sourceEpoch 要求非零、小写、无大括号标准 UUID；archive sequence 为正 qint64，sourceSequence 为正 uint64，均编码为规范十进制字符串而非 JSON 浮点数。receivedUtcMs 为正 JS 安全整数，receivedMonotonicMs 为非负 JS 安全整数；不同 sourceEpoch 的单调时钟不可直接比较，UTC 也不证明跨飞机同步采样。

帧 envelope 精确 29 字段：`format/version/kind/runId/sequence/sourceEpoch/sourceSequence/receivedUtcMs/receivedMonotonicMs/magic/msgid/sysid/compid/rxSequence/length/incompatFlags/compatFlags/checksum/payloadBase64/signaturePresent/positionTelemetry/positionTelemetryConsent/authenticated/crcVerified/permissionsRestored/flightCommandReleased/fullTelemetryCaptured/parserAccepted/reconstructedWireCapture`。format 为 `merivus-fleet-telemetry-frame`、version 为 1、kind 为 frame。所有整数均检查类型、范围及精确性，不接受 null、bool、字符串或容器代替数值。位置两项以及 signaturePresent 必须与白名单、同意和原生标记一致。

原结构 magic 仅接受本地 MAVLink 1/2 标记，sysid/compid 为 1..255。MAVLink 1 的 msgid 必须 <=255，长度恰为生成 MIN_LEN，incompat/compat flags 均为 0；MAVLink 2 只接受 SIGNED 不兼容标记，长度为 1..生成 MAX_LEN。后者允许合法尾零裁剪低于 MIN_LEN，不能用 min 判定误拒。任一超长扩展拒绝，绝不截断。compat flags 可忽略但按原值保留。payloadBase64 严格保留 `_MAV_PAYLOAD` 的原 len 字节，不保留 parser 零填充的后续区域，也不解码浮点 NaN/未知值或改写为 0。

不使用 `mavlink_msg_to_send_buffer()` 重构。当前本地 helper 对 MAVLink 2 会再次裁剪尾零，却沿用原 checksum，因此该输出不能冒充接收时的 exact wire bytes。这里只存原生解析结构的头部字段、原长度 payload 及 checksum；13 字节签名和密钥均不落盘，只保存 signaturePresent 布尔。归档文件不是可直接重放的 .tlog、完整串口抓包或签名验证材料。

`recordConsistent()` 检查 Record 镜像字段、精确 29 键、类型、UUID/计数/时刻、白名单/长度/同意及固定旗标，重新编码必须逐字节等于原紧凑 JSON，拒绝重复键、额外键、空白改写和非规范 Base64。Qt Base64 解码可能宽松，因此再检查 ASCII、预期文本长度、解码长度和重新编码相等。该检查只能发现值/格式不一致，不能验证原生入口身份：能够重新生成整个合法 Record 的第三方仍可伪造观察陈述。

## 解析接受不等于认证或完整捕获

`parserAccepted=true` 只描述原生接纳路径的预期语义；authenticated、crcVerified、permissionsRestored、flightCommandReleased、fullTelemetryCaptured 和 reconstructedWireCapture 全部固定 false。SIGNED 位并不表示已认证。观察已有 ACTUATOR_OUTPUT_STATUS 或位置目标不执行这些输出，flightCommandReleased=false 也不证明历史上从未发送控制指令。

已静态读取本地 `mavlink_helpers.h` 的签名状态机：无 signing 对象时 signature_check 返回 true；签名收齐后的 sig_ok 分支直接写 FRAMING_OK，可能覆盖更早的 BAD_CRC 状态。这是对当前源码的静态风险判断，未运行验证，不修改通用 MAVLink parser。本 Codec 保留它已接纳的诊断结构，不新增 ck 一致性拒绝、CRC 重算或 crcVerified 承诺；解析库加固须作为单独受审查任务，不能在遥测采集补丁里暗改。

原生已接收白名单不等于“完整遥测”：未启用的新流、解析前损坏/丢失的帧、旁路链路、未绑定飞机、白名单外/未经定位同意消息、队列背压和存储失败均可能缺失。8 位 rxSequence 允许回绕/重排，只是原头字段，不是精确无线丢包计数。完整性与统计由入口/worker 明示其观察范围、排除和缺口；Codec 不提升历史数据为 FleetRegistry、估计器健康、审批、StartPermit、任务完成或控制权。

## 工程师交接

本批仅源码与文本静态检查，未运行 codec/helper、编译、测试、仿真、数据库或设备。工程师负责实际 dialect/Qt 版本编译、MAVLink 1 长度、MAVLink 2 尾零/扩展边界、签名解析风险、位置授权、NaN 原字节保留、大计数/时钟域、Base64/JSON 拒绝以及原生源生命周期检查。旧 QGC .tlog 与机载日志管理未由本模块改写；此白名单不代表其它旧日志已被脱敏。

## 305–309：共享 journal 历史头/尾校验

本次扩展不改变既有 27 类消息/29 字段 frame。新增 `inspectHeader()`、`inspectFooter()` 和 `readPolicyValid()`，均为只接受值的同步静态方法；没有 QFile/SQL/QThread/Vehicle 实例、QObject 信号或历史导入入口。所校验的 schema 来自当前 [Worker](FleetTelemetryArchiveWorker.md) 实际写入源码，而不是把 worker.current 状态字段想当然当成磁盘字段。

Header 与 Footer 的输入都是去掉最终 LF 的完整紧凑 JSON，最长 4096 字节且不允许 CR/LF。JSON 解析后重新生成紧凑表示必须逐字节相等，拒绝重复/额外键、非规范字节、未知结构和隐式类型转换。字段验证不使用 toString/toInt 的默认值替代 null 或错误类型。Canonical 是本程序 Qt 的紧凑表示，不是跨库签名规范；格式一致不认证文件作者。

### Header：精确 19 字段

共同十项固定 false：`authenticated/crcVerified/permissionsRestored/executionPermit/flightCommandReleased/fullTelemetryCaptured/fsyncVerified/durableVerified/directoryEnumerationCompletenessVerified/reconstructedWireCapture`。其它九项为 `kind/format/version/runId/openedUtcMs/positionTelemetryConsent/sourceCoverageClosed/sources/policy`，kind=header、format=`merivus-fleet-telemetry-journal`、version=1、sourceCoverageClosed=false。runId 必须与读取方从已校验 basename 获取的非零规范 UUID 完全一致，openedUtcMs 为 `1..9007199254740991` 的规范十进制字符串，定位同意为真实 JSON bool。

写策略 policy 恰六字段，接受真实、有限、整数 JSON 数值，范围与当前 worker 一致：maximumQueueFrames 16..4096、maximumQueueMiB 1..16、batchFrameLimit 1..128、maximumFileMiB 1..1024、maximumRunFiles 1..128、flushIntervalMs 100..5000。拒绝数字字符串、bool、null、浮点小数、额外字段和缺失字段。检查后才将这些数值正规化为已知 Qt Int；version 与来源 vehicleId 也正规化为 Int，不能让 JSON 转成 QVariant 的不同数值子类型决定权限。Qt 数值转换参考的是 [Qt 5.15.2 原始实现](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/corelib/serialization/qjsonvalue.cpp)，不是已运行的平台验证。

sources 为 1..32 个 JSON object，每个恰 `vehicleId/sourceEpoch/initialSequence`：vehicleId 为 1..255 的整数，sourceEpoch 为非零规范 UUID，initialSequence 必须为字符串 `"0"`；同一次头中 ID 与 epoch 都不重复，不部分截断。成功 Header 携带 `ok/errorCode/runId/includePositionTelemetry/value/sourceVehicleIds/canonicalJson`；value 仅为固定格式的历史投影，sourceVehicleIds 是 epoch→ID 值范围表，不是实际 Vehicle 绑定、设备认证或可重用的定位批准。

### Footer：精确 31 字段

共同十项 false 与 Header 相同；kind=footer、format/version/runId 与 Header 一致。clean=false、finalFlushAcknowledgementIncluded=false、writtenBytesExcludeFooter=true、positionTelemetryConsent 与 Header 一致；acceptedQueueDrained、closeCandidate、captureSummaryProvided 只能为 bool。它们是磁盘里的未认证历史陈述，不等于最终 flush/close、源连续性、完整遥测或实际飞行完成。

八项计数 `writtenFrames/writtenBytes/flushConfirmedFrames/flushConfirmedBytes/lastWrittenSequence/queueDroppedFrames/discardedFrames/failedPendingFrames` 必须为 `0..INT64_MAX` 的规范十进制字符串。writtenFrames、writtenBytes、lastWrittenSequence 必须精确匹配读取器实际扫描的帧数、Footer 前字节数和最后 offered 序号；字节数包括 Header 及每条已完成行的 LF，不包括 Footer 本身。flushConfirmedFrames 不大于 writtenFrames，flushConfirmedBytes 不大于 Footer 前字节数。当前 v1 Footer **没有** lastFlushConfirmedSequence，它仅存在于 worker.current；本校验不会捏造第 32 字段或声称验证了磁盘上的最后 flush 序号。

captureSummary 精确十项：九个 `0..INT64_MAX` 计数字符串为 receivedFrames、filteredFrames、invalidFrames、queueRejectedFrames、lastOfferedSequence、sourceMissingFrames、sourceRetiredCount、captureStartUtcMs、captureEndUtcMs；sourceCoverageClosed 为 bool。它是 writer 最近接受的采样摘要，允许落后于扫描到的帧，不能要求 received/offered 必须大于已写帧，也不能因摘要里的关闭陈述为 true 就恢复源覆盖认证。读取层的 closure/source/coverage verified 仍必须为 false。

errorCode 要求 JSON 字符串，只允许明确空字符串或当前 Worker 源码定义的固定 23 个错误码；陌生码、路径、自由文本和 null 拒绝。失败时 Footer 返回空 map，ok=false 且错误仅为 `telemetry-journal-footer-invalid` 或镜像不匹配的 `telemetry-journal-footer-scan-mismatch`；Header 失败固定 `telemetry-journal-header-invalid`。成功 Footer 返回固定历史投影，ok=true 仅说明 schema/扫描镜像一致，所有禁止提升的旗标仍为 false。Header 为公开值结构，Footer 校验重新检查其 canonicalJson 和 runId，再匹配 scope/定位属性；不信任调用者可改写的 Header.value 作为权限或源证明。

### 读取策略与尾部边界

独立原生 readPolicy 恰四个真实 Qt 整数字段：maximumFileMiB 1..1024、maximumCatalogFiles 1..128、maximumPageMiB 1..8、maximumPageFrames 1..256。拒绝 typed-null、double、bool、字符串、未知/缺失字段或越界值。它不复用磁盘写策略、定位授权或任务权限，不默认请求/接受新飞机。

本 Codec 不打开文件，也不判断 EOF/尾行是否完整。由读取器把缺失 Footer、未终止最后一行与严格格式错误区分开：有效 Header 后的完整、已校验帧前缀可作为未闭合的历史观察值，但绝不能据此宣布 clean、closure verified 或最终写盘成功；非法 Header 不可分页，完整但不合法的 Footer 不可被悄悄视为不存在。历史读取、缺尾/损坏拒绝、分页字节预算、目录扫描和平台 JSON 表示均留给工程师实际验证，本次没有执行这些场景。

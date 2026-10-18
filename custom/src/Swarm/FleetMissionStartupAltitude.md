# FleetMissionStartupAltitude：显式 v4 高度关系策略

## 445 范围

此纯值 helper 供明确选择的 `startupTransitionPolicy.schemaVersion=4` 使用。它不访问 Vehicle、Link、时钟、设备或文件，不请求数据流，不建立权限，不发送命令。实际 PRIMARY 归属、连续观察、粘性异常及发送前尾核必须由原生调用者完成。旧 v1–v3 不增加 ALTITUDE 前置。

输入是原生 [ALTITUDE 解码](../../../src/Vehicle/FleetStartupAltitude.md)的六字段报文及 [Vehicle 被动观察](../../../src/Vehicle/VehicleStartupAltitude.md)，不是 QML 声明。`sameMessageFields=true` 仅表示字段来自同一报文；内部 valid、共同原点、共同估计器样本、传感采样鲜度、认证和飞行许可始终未被证明。

## 策略与单位

v4 恰九字段：沿用 v2 的全球/局部运动、heading、RAW gap 和阶段时长，`homeChangeHandling` 仍为 `stop-and-review`；新增 `altitudeReferenceEnvelope`，不接受 v3 的 `homeUpdateEnvelope`。不隐式升级，不补任何安全容差。共享 [ExecutionProfile](FleetMissionExecutionProfile.md) 规范化整份策略。

`altitudeReferenceEnvelope` 恰七字段：

- `referenceBasis` 固定为 `reported-altitude-relations-not-origin-validity-or-same-sample-proof`。
- `maxAmslMinusLocalDriftMeters`、`maxAmslMinusRelativeDriftMeters`、`maxRelativeMinusLocalDriftMeters`：三种同包差值相对原始冻结样本的最大漂移。
- `maxAmslMinusLocalToOriginResidualMeters`、`maxAmslMinusRelativeToHomeResidualMeters`、`maxRelativeMinusLocalToHomeLocalZResidualMeters`：三种同包差值对原冻结参考的最大残差。

六个上限均为显式有限数值 0–1000 m，严格零有效；拒绝缺项、未知项、字符串数值、bool、null。归一化输出六个 Double。年龄使用实际政策 `maximumRawGapMs`；helper 支持原生 getter 的 1–60000 ms 形状范围，调用者必须传所选政策的 100–3000 ms，不得借 helper 上限扩大政策。

令 A、L、R 为同包已知 AMSL、local、relative 三个原始 binary32 字段：检查 `A-L` 对原 GPS_GLOBAL_ORIGIN AMSL、`A-R` 对原 HOME AMSL、`R-L` 对原 HOME localZ（NED）的残差。只比较报告的关系，不证明 PX4 使用了这些参考或走了有效分支。毫米与米的换算在精确整数线性组合中完成，不先计算浮点 `mm / 1000`；localZ 保原米值，不能擅自反号。446 的精确比较采用闭区间：数学上恰等于实际 binary64 上限可接受，零上限只接受真实为零的组合，不夹紧或增加隐含误差。

## 446：以原始操作数进行精确门禁比较

门禁使用 [FleetMissionAltitudeArithmetic](FleetMissionAltitudeArithmetic.md) 的固定 36 个 uint32 limb 精确累加，将原始有限 binary64 表示分解为以 `2^-1074` 为共同单位的有符号整数。三个高度原本是 binary32、精确提升为 Double；origin/HOME AMSL 是精确整数毫米。系数乘法在整数域完成，不能先浮点乘 1000、相减或除以 1000 后才送入比较器。

- 原点残差：`abs(1000*A - 1000*L - originMm) <= 1000*cap`。
- HOME AMSL 残差：`abs(1000*A - 1000*R - homeMm) <= 1000*cap`。
- HOME localZ 残差：`abs(R - L - homeLocalZ) <= cap`。
- 三种漂移分别为 `abs(Ac-Lc-Ab+Lb)`、`abs(Ac-Rc-Ab+Rb)`、`abs(Rc-Lc-Rb+Lb)`，逐项比较原来对应的 cap；下标 c/b 表示当前/最初冻结样本。

所有 cap 都是既有策略规范化后的实际 binary64 值，不是原始十进制文本的无限精度解释。合法的 subnormal cap、明确零和非零恰等边界均由同一精确比较处理；不修改政策值、支持域、基线或权限。任何不支持的值、表示或整数溢出均返回 false。`requiredValues` 仍核三个诊断差值确实等于原 Double 减法结果（包括零符号），但残差和漂移门禁均不使用这些已舍入的差值作算术操作数。reference/sample 仍各20键，API、来源/收据/时钟和同收据原值约束不变。

### 三个未运行的解析反例

以下仅为二进制浮点的数学推导，不是执行用例或验证结果。所有高度均为合法的普通 binary32，处于既有支持域。旧算法先产生 Double 差值及 `mm / 1000`，然后仅将最终非零绝对值用一次 `nextafter(+∞)` 外扩；这不覆盖之前已经丢失的数值。

1. **零容差残差。** 取 `A=1`、`L=2^-55`、`originMm=1000`、cap=0。`1-2^-55` 舍入为 Double 的 1，旧残差得到零并通过；原始报告值的真实残差是 `2^-55`，应拒绝。对最终零特殊放行不能找回第一步丢失的量。
2. **毫米除法的非零贴边。** 取 `A=1`、`L=0`、`originMm=1001`。设 `q=fl(1001/1000)=1+4503599627370*2^-52`，`r=q-1`。因为 `2^52/1000=4503599627370.496`，真实残差 `1/1000=r+507.904*2^-62`。该 r 所在区间的 Double ulp 为 `2^-62`；显式取可表示 cap=`r+2^-61`，旧外扩只得到 `r+2^-62`，会通过，但真实残差仍超限。最终一步扩大一 ulp 不足以覆盖前面除法在接近相消后放大的误差。
3. **非零漂移贴边。** 基线 `Ab=1-2^-24`、`Lb=0`，当前 `Ac=1`、`Lc=-2^-54`。当前差值 `1+2^-54` 舍入为 Double 的 1，旧漂移为 `2^-24`；外扩为 `2^-24+2^-76`。取 cap=`2^-24+2^-75`，旧比较通过，但原始四操作数的真实漂移是 `2^-24+2^-54`，仍超限。

这些反例也说明，近消减法本身有时是精确的，问题来自进入减法前已经舍入的操作数。新比较直接处理原始高度与整数毫米，既不依赖最终外扩补偿，也不为弥补数值误差擅增工程容差。

## 固定输出

`initialize(native27, vehicleId, componentId, originStable18, homeValue9, envelope, maximumAgeMs, reference*, sample*)` 成功一次产生以下两个完整 map。失败清输出；输出参数必须非空且相互不同，调用者不得把输入 map 当输出复用。

reference 恰20键：

`schemaVersion, vehicleId, componentId, sourceEpoch, flightSourceEpoch, altitudeSourceEpoch, clockDiscontinuityRevision, clockDiscontinuityObserved, originReference, homeCoordinates, referenceBasis, snapshotOnly, sameOriginProven, px4BranchValidityProven, sameEstimatorSampleProven, executionPermit, flightCommandReleased, sensorSampleFreshnessProven, authenticated, signatureVerified`。

schema 为1；`snapshotOnly=true`，末八个证明/许可 flag 均为 false。source 三个 epoch 是原生 UUID；clock revision 是规范 uint64 十进制字符串，flag 保原状态。`originReference` 保严格原18键，并要求实际已观察、可用、坐标在范围内；旧版本允许的“确实尚未观察”不满足 v4。`homeCoordinates` 保原九键 `latitudeE7/longitudeE7/altitudeAmslMm/localX/localY/localZ/localCoordinatesKnown/timeUsec/timeUsecKnown`，要求坐标与 localXYZ 已知、正 HOME timestamp；不以时间量级认证时钟。origin 的机号、组件及 source/flight epoch 必须与 ALT 一致。实际 HOME 通道归属和原参考后续不变由调用者另核。

reference 不包含 ALT 周期高度、三差值、receipt、age、timeUsec、raw hash 或全六字段 `stateRevision`。必要字段关系正常变化不会因为完整原包 hash 或无关 terrain 状态改变而每包撤审；实际来源、时钟断续 revision、原 origin/HOME 改变则不能借新参考替换旧参考。明确的新审阅可冻结 `clockDiscontinuityObserved=true` 的历史状态（此时 revision 必须为正），不是永久拒绝曾发生回退的设备；之后同一基准内 flag/revision 必须不变。原生失源重置 flag=false 而累计 revision 仍为正也合法，不能伪称从未发生过历史回退。

sample 恰20键：

`schemaVersion, vehicleId, componentId, sourceEpoch, flightSourceEpoch, altitudeSourceEpoch, clockDiscontinuityRevision, receiptSequence, timeUsec, rawPayloadHash, rawPayloadLength, altitudeAmslMeters, altitudeLocalMeters, altitudeRelativeMeters, reportedAmslMinusLocalMeters, reportedAmslMinusRelativeMeters, reportedRelativeMinusLocalMeters, snapshotOnly, executionPermit, flightCommandReleased`。

schema1；三个高度是原有限 binary32 精确提升的 Double；三个差值按原 Double 运算严格核对（包括零符号）；receipt 为正规范 uint64 字符串，远端时间为规范 uint64 字符串（允许零，不推断时钟类型），hash 为小写 SHA256，payloadLength 为1–32；末三 flag 为 true/false/false。sample 是历史事实，不是稳定审批身份，也不是每步允许刷新替代的基线。

v4 明确支持域：必需三个高度及三个差值、完整 HOME reference 中 localXYZ，绝对值均不超过 `9007199254740991`，超出拒绝而非夹紧。这与已有审阅 JSON 数值域一致，不是物理合理性认证。原 444 decoder 仍完整保留所有合法 finite binary32；无关 monotonic、terrain、bottomClearance 可未知、NaN、Inf 或超该数值域，不能单独导致 v4 关系拒绝。坏原包结构、伪造状态/原始位串或字节截断不一致仍拒绝。

## 调用分工与失效

- `normalizeEnvelope` 只规范化完整政策。
- `capture(native27, reference, envelope, maximumAgeMs, sample*)` 严格校原生27键/类型、完整已观察、可用、fresh、receiveStackQuiescent、年龄、收据、原 decoder18/state8、必要字段及三绝对残差；来源和 clock revision/flag 必须仍为原 reference，不会重采 origin/HOME。
- `sampleWithinBaseline(current, baseline, envelope)` 严格校两个 sample、相同原来源、收据/远端时间不倒退，并用原始四操作数精确比较三种相对最初 baseline 的漂移；相同收据不得改写对应时间、包身份或必要值（包括正负零符号，不能用普通QVariant数值相等掩盖），后续新收据可重复相同值。外层必须另核当前 reference 与最初 reference 相同及 capture 成功，不能只比较 sample 而漏掉绝对残差。
- `decodedMatches(decoded18, reference, baseline, envelope)` 用于真实 RAW 接收的回调之前；校原 decode、必要字段、原 baseline 残差、当前绝对残差及原始漂移基线。它不伪造 quiescent 信封，不替代实际 PRIMARY、receipt、间隙、epoch 或 A→B→A 粘性观察。

正常的 Double 周期高度、远端时间增长、收据增长、原包 hash 变化和 unrelated stateRevision 不能单独成为稳定审批身份变化。实际启动 scope 才开始连续监视；审阅与 scope 绑定之间不能声称没有未观察越界。缺报、过期、坏必需字段、失源、时钟变化、超支持域或超包线一律不能凭本 helper 返回值推测内部 valid；不得自动重采、重试发送或重建批准。

## 交接边界

本轮仅编辑源码与文档、静态对读契约。未运行 helper、解析器、编译、测试、仿真、软件界面、认证或设备；无 PX4 改动。工程师后续需核真实消息截断、NaN/Inf/负零、合法零包线、残差符号、原基线漂移、间隙与 A→B→A、失源/时钟倒退、回调重入及最终发送尾核。成功不等于完整 StartPermit、净空证明、实际任务完成或刷机放行。

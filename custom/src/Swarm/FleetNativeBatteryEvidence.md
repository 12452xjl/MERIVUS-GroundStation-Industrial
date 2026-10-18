# 516：独立原生 SOC 下降窗值校验

新增 `inspectDrain(vehicleSnapshot, vehicleId)` 与 `unknownDrainEvidence(vehicleId, reason)`。原 `inspect`、schema1／2 top20、row15／19 与负面健康规则不改。新 sibling `nativeBatteryDrainEvidence` 缺键是 legacy missing；存在但坏型、null、缺键、坏公式或跨源不一致是 invalid，不能当作没有窗口后静默回退较乐观估算。纯值检查不读取 Vehicle、文件、时钟或网络，也不认证调用方 map；实际来源仍是 Registry 私有原生绑定。

## 固定值合同

top 精确25键：`schemaVersion,reportType,vehicleId,componentId,sourceEpoch,contentRevision,batteryContentRevision,batteryId,batteryReceiptSequence,sourceCurrent,observed,state,reason,ageMs,window,drainKnown,percentPerSecond,remainingFlightSeconds,snapshotOnly,authenticated,performanceCertified,remoteMeasurementTimeVerified,configurationPermitted,executionPermit,flightCommandReleased`。schema 为数值整数1，reportType 为非null String `native-soc-decline-window`；身份、年龄、计数与布尔严格按类型／范围核对，不把字符串数字、假布尔或 null 强转。计数为 canonical uint64 十进制 String。`snapshotOnly=true`，其余认证／性能／远端测量时间／配置／执行／放行标志全部 false。

`window` 精确8键：`startReceiptSequence,endReceiptSequence,startPercent,endPercent,durationMs,sampleCount,maximumGapMs,endAgeMs`。普通 unknown 一律为计数 `"0"`、百分比／时长／间隔／年龄 `-1`、样本数0，不公开 warming 前缀。只有 `stale-decline-window` 可保完整已发布历史 window；其 rate／remaining 仍为-1，不能参与估算。

合法已知窗须持续30–120秒，下降至少2个百分点，保留样本2–256个，相邻实际 accepted RAW 最大间隔不超过3000ms；样本是约1秒留点加每次 SOC 变化，不是所有 RAW 包，不能用保留样本数推断收包频率。首末 receipt 为正且有序，末 receipt 不超过最新 selected battery receipt，末端 SOC 必须等于当前选中电池 SOC。`contentRevision` 是独立下降窗代次，已知／历史窗须为正；`batteryContentRevision` 必须等于旧 top20 的全局账本版本，不是某行版本。最新 `batteryReceiptSequence` 必须等于旧 top20 selected receipt，不能错误地要求历史末 receipt 等于每个后续 flat 包。

两个 getter 年龄不同：旧 top20 使用原晚提交年龄，新 top25 的 ageMs 是最早原 RAW 入场到最新电量报文的本机年龄，二者不强等。known 与历史窗须两侧 selected 来源可核且新 raw age 0–3000ms；known 另要求 endAgeMs≤30000，历史 stale 窗要求其>30000。普通 unknown 的 selected 报告仍可保 age=-1 或>3000诊断，不因此伪造成 malformed。明确 `receive-in-progress` 全unknown形态允许新 whole-RAW 栅比旧 battery-only 栅更保守地令 sourceCurrent=false；其余 epoch/component/global revision 仍绑定原值，不产生 usable。

已知公式：`rate=(startPercent-endPercent)/(durationMs/1000)`，`effectiveAge=max(rawAgeMs,window.endAgeMs)`，`remaining=max(0,currentSOC/rate-effectiveAge/1000)`。flat 更新当前报文 receipt／raw 年龄，但不刷新下降端点、速率或独立代次；因此重复相同 SOC 不重新扩张旧窗口预算。SOC 回升、来源／用途／类型变更、unknown／间隔断裂会由原生生产者退休旧窗，不能 A→B→A 认领旧估算。源未绑定工厂只生成明确无源 unknown，Registry 固定原因使用 `registry-*`，不生成已知窗口。

## Read 与实际消费

`DrainRead` 含 present/valid/usable/evidence/reason，以及仅 usable 时有效的 rate、remaining、effective age。新窗仍复用完整旧 `inspect`：当前 fresh 原生电池来源、唯一推进电池、aliases、辅助负面和旧故障否决均不绕过。旧健康否决时完整新窗可以 valid 供诊断，但 usable=false；三个动态输出统一-1。known 只是本地报告的近期线性外推，不是可信续航上界、真实负载模型或健康／飞行许可。

[EnergyScheduler](FleetEnergyScheduler.md) 仅 min 下调已有已知值，完整 window 进入独立 top 诊断，不扩 assignment18／21 或 ReserveEnergyPlan29／32。[Coordinator](FleetMissionReviewCoordinator.md) 用同一检查器提供小型稳定身份及动态扣龄值供 Root 实际审批尾检；自然 age／receipt 变化不改稳定身份，窗口成熟、过期、失源和语义变更仍会改变身份。未新增主动请求、传感器、命令、授权或 UI。

本批仅源码及直接静态阅读，未编译、执行 helper／解析器／模型／QML／应用、测试、仿真或操作硬件。需工程师验证正常30秒窗口、flat老化、源切换／回升／gap／用途变化、边界 stale、坏 sibling、故障否决、审批扣龄和旧缺键兼容；不据静读宣称真机可靠性或原25目标完成。

# 原生电池证据共享校验与负面准入（460）

`FleetNativeBatteryEvidence::inspect(vehicleSnapshot, vehicleId)` 是纯值检查，不访问Vehicle、文件、时钟、网络、权限或设备，不认证调用方map。实际原生来源仍由Registry私有Vehicle绑定、PRIMARY和原账本保证。没有marker的独立legacy计算返回required=false，不因此获得原生证据或健康证明。

## 输入与全部成功提交

`nativeBatteryEvidenceRequired`存在时必须为非null Bool true，`nativeBatteryEvidence`必须为非null map。envelope固定20键，继续检查完整原ID／来源epoch／component／序号／所有行／known哨兵／聚合及标量别名。兼容schema1的原row15；schema2必须row19，不自动升级旧快照，不接受半套新字段。

schema2逐行增加：

- `chargeStateKnown`：Bool；`chargeState`：known时为整数1–255，否则精确-1。协议0表示未提供，不伪做正常。
- `faultBitmaskKnown`：Bool；`faultBitmask`：known时为整数0–4294967295，否则精确-1。原生qint64与合法JSON Double都能完整保存uint32，检查使用std::floor而非返回int的qFloor。

known值须来自已观察且有有效年龄的行；当前新鲜为原sourceCurrent、observed和fresh，年龄不超过3000ms。原Vehicle只有在对应MAVLink2原长度到达offset40／50时才标扩展known；零尾截断沿原zero-fill解码，MAV1／未到字段未知。故障位0、状态1或缺字段均不是物理健康／故障检测支持认证。

全部行及原scalar aliases、推进拓扑和聚合都校验后才提交evidence、valid与reportedUnsafe；坏后缀不发布前缀。最多16行，每行固定标量合同，序号最多20字符，UUID固定36字符，聚合原因最多128字符。reported-estimate仍要求唯一fresh ALL／PROPULSION行及全部top值对应；辅助电池不计入推进续航，但负面不能被忽略。

## 输出与负面规则

原生`Read`含required、valid、usable、reportedUnsafe、evidence、reason。valid只表示值合同一致，不表示电池安全。usable只表示原计算来源当前可用且未发现下述否决，不是可飞许可。

- 任意同源已观察fresh电池行的非零faultBitmask：reportedUnsafe=true，reason为native_battery_reported_fault。
- 任意同源已观察fresh行的明确chargeState不是1：reportedUnsafe=true，reason为native_battery_reported_charge_state_unsafe。包括LOW／CRITICAL／EMERGENCY／FAILED／UNHEALTHY／CHARGING和未来非零未知枚举，均不作为本次任务准入的正常状态。只阻断新准入，不在这里发送返航、停机或其他命令。
- 同源已有负面行变旧：reportedUnsafe=false，不冒称当前新故障，但usable=false、reason为native_battery_fault_evidence_stale，不能靠老化自动恢复可用。新完整报告及新显式审阅才可重新评估；这里不认证故障已物理修复。
- 源不可用或聚合歧义：usable=false，原原因native_battery_source_unavailable_or_ambiguous；坏型／字段／aliases不一致：valid=false、evidence为空、reason为native_battery_evidence_invalid。

schema1／缺扩展不新增“一律故障”规则，但仍受原源、鲜度、拓扑和能量门槛限制。合法拒绝继续保存完整原证据；无效map不能被保存成可信来源。

## 实际消费者

Registry以valid而非usable保留原报告，只对返回副本追加当前负面／unknown，不修改持久记录或原SYS／EST known／年龄。HealthCenter在FTC合并前检查实际Registry aliases，合并后重新生成nativeBatteryHealthCheck4，不让旧FTC百分比或同名metadata洗掉原生负面；独立blocking ID保证任务门禁实际拒绝。

EnergyScheduler复用本检查器，不保第二套解析器；原主机和备用机拒绝账单仍exact18，静态endurance／耗率不能绕过负面或过期负面。原RiskEvaluator、Coordinator、Deployment及启动健康门禁消费既有blocking链，不新增批准或许可。

Swarm发现仍保留合法负面的原电量和来源，以便真正建立Registry绑定，不把故障报告抹成旧正常缓存。Guided审阅身份接schema2并保全部health行的值／known／fresh；纯故障变化也推进原semantic revision并使旧审阅不再对应，普通age／receipt增长不改变稳定身份。发送前[原生phase seal](../../../src/Vehicle/FleetStartupMemorySeal.md)另外固定实际电池账本和原源，不能用一次健康显示代替所有回调后的整组尾核。

本机仅完成源码与限定静态阅读／文本检查，未编译、执行helper／QML／解析器／线程／应用、测试、仿真或设备操作。不是可信续航、物理电池／硬件身份认证或工程放行；完整运动域、最终StartPermit、正式结果和原25目标继续。

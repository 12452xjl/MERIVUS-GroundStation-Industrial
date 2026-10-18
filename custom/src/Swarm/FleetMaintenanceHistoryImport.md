# 517：当前原生遥测历史页到维护观察预览

## 518增量：另行确认的文件工作区

本517页的追加仍只改变本进程记录，不自动读写文件。另一个[维护文件工作区](FleetMaintenanceWorkspace.md)现在提供显式冻结／保存和独立历史重开：保存仅写已预览的原私有bytes；文件历史不覆盖当前护照，不恢复517原生页资格、批准或飞行状态。两入口共享实际Passport，但文件busy不长期锁517追加；记录版本变化使旧冻结sourceCurrent失效而不换掉其内容。

本模块只从实际 `FleetTelemetryArchive` 保留的当前成功 native 历史页提取 `BATTERY_STATUS`，供实际维护护照另行确认追加。不是维护放行、物理电池身份、任务完成、飞行次数、整航次峰值或当前飞机健康证明。不请求遥测、不改变原 RAW／Codec／SQL／wire，不向界面公开 payload、精确位置或任意路径。

## 实际来源与范围

只接受 owning GUI thread、已同双终结释放读取槽、未取消且仍是同一个最新 page/query 的私有页；catalog 必须是原固定本地目录及固定 policy（最多64行）。新读取、取消或实际新 capture 退休旧私有预览，source A→B→A 不能仅因最终字段相同复活。新 capture 不会让旧 preview 在停止后重获当前资格。当前位置 payload 是否获准不作为本功能前提：只解析147电池报告，不解析或复制位置。

原 native page36／frame29、完整文件验证、源序号／三计数、footer／partial tail 与双 hash 合同不扩。公开 history summary 不是原包来源，caller 也不能提供 payload 或任意 page 重建本预览。这里的 current 仅表示本进程仍保留该份原成功页及私有预览，不重新读盘，不证明文件现在未变化、目录全集、closed／fsync／durable、发送者真实性或远端测量年龄。

## 候选与显式选择

`maintenanceHistoryCandidates(queryId,confirmed)` 返回固定6键：`schemaVersion=1,queryId,candidates,reason,historicalOnly=true,authenticated=false`。成功 reason 为空，缺电池样本或源不当前给固定原因及空列表。每个候选固定5键：`sourceEpoch,vehicleId,componentId,batteryId,sampleCount`；前三身份来自原 frame，电池 ID 来自原147解码，sampleCount为本页该组报告数1–64。相同数字机号、组件、电池 ID 不会跨 sourceEpoch 合并；列表按稳定的来源组键排列，不代表设备优先级。

`prepareMaintenanceHistory(selection,confirmed)` 只接受精确5键 `queryId,sourceEpoch,vehicleId,componentId,batteryId`，标量严格非null／数值整数。未确认、坏型、非当前页或忙入口拒绝；合法已确认的新 prepare 先清旧预览并通知，再消费本次冻结完整私有页。首个通知后、发布后通知尾分别核原 QObject、非回绕 generation、history serial 与整份 page 完全相等。嵌套新 prepare 被 busy 挡住；同 ID discard 不被 busy 挡住，可在发布通知内撤销本次资格。失败不返回旧 preview 冒成功，不重认领后来同值来源。

## 预览14与 native record

`maintenanceHistoryPreview` 及 prepare 的结果固定14键：

`schemaVersion,previewId,queryId,appendable,reason,source,evidence,metrics,historicalOnly,authenticated,physicalIdentityVerified,executionPermit,flightCommandReleased,current`。

原生成功 previewId 是 canonical非零UUID。合法页即使指标全未知，也可生成明确不可追加的诊断预览；失败回执的 ID 为空、current／appendable=false、三个内容 map 为空。`historicalOnly=true`，认证、物理身份、执行许可和飞行放行全部 false。metrics 只含实际有有效样本的 `temperatureC` 和／或 `currentAmps`，不填默认零；至少一项存在才 appendable。当前 getter 在原页失效后为空，不把缓存标成持续核验。

`source` 精确7键：`runId,headerHash,fileHash,sourceEpoch,vehicleId,componentId,batteryId`。完整 file hash 与 header hash 只是值一致性／来源组材料，不是签名或设备身份；sourceEpoch 是本档案采集来源，不是实物 UID。护照消费者须按完整来源组隔离，不能与同数字 ID 的手工记录或其他档案来源组混成同一物理设备趋势。

`nativeMaintenanceHistoryRecord(previewId,confirmed)` 不是 Q_INVOKABLE，只有原生维护消费者取用。它只在原完整页／serial／preview 仍同一、已确认、无 prepare 或原 history action busy 且 appendable 时返回 record 本体；其余返回空 map，不是 envelope。record 基础5键：`vehicleId,timestamp,source='telemetry-history-page',historicalTelemetrySource,historicalTelemetryEvidence`，再带一或两个真实 metrics，总6或7键。timestamp 是最后选中样本的原本地 receivedUtcMs 对应 UTC ISO 毫秒串，不以导入时刻刷新旧证据。不可表示的时间不能追加。

`discardMaintenanceHistory(previewId)` 只退休完全同 ID 的私有预览，错误或后来别人的 ID 不受影响；清理先发生、通知后不再访问本体。该操作不清维护护照已有记录，不回滚已追加事实，也不是飞行动作。Passport 的实际追加、完整来源组和区间去重是独立消费者合同；仅有 preview／公开 map 不等于已写护照。

本次追加只改变本进程护照记录，不自动保存到磁盘；未调用保存的记录不承诺跨重启保留，成功回执也不是 durable。appendable 只表示本预览存在可供提交的指标及可表示的历史时间，不保证护照接受；若原样本UTC相对当前本机时钟落在未来，原护照时间政策仍可能拒绝追加，不用现在时间替换它。

## evidence21与指标计算

evidence 固定21键：

- 范围4：`pageAfterSequence,pageNextSequence,firstSequence,lastSequence`，均 canonical十进制 String；选中首序号大于 pageAfter，末序号不超过 pageNext。
- 原UTC4：`firstReceivedUtcMs,lastReceivedUtcMs,minimumReceivedUtcMs,maximumReceivedUtcMs`，正的 JS-exact整数；保原顺序与极值。
- 时间诊断：`utcRegressionObserved`，发现后一个所选报告 UTC 小于前一个则 true，不排序、夹值或用现在时间修复。
- 计数7：`sampleCount,temperatureKnownCount,temperatureUnknownCount,temperatureOutOfRangeCount,currentKnownCount,currentUnknownCount,chargingCount`，样本1–64，其余0–64；温度三类之和和电流三类之和分别等于样本数。
- 页面／覆盖5：`footerPresent,partialTail,hasMore,completeRecordPrefixOnly=true,historyComplete=false`。

电池温度为原 `temperature/100` 摄氏度；INT16_MAX为未知，其余不在[-100,300]为越界样本，不夹成合法温度。电流为原 `current_battery/100` 安培；-1是未知，其余负值只计 charging，不当正放电峰值；非负值（含0）才可进入本页最大报告值。仅原有效样本参与最大值，不把未知／越界／充电填零。已有包可能没有远端传感器或有错误报告，本功能不认证数值物理正确。

这里的“最大”仅覆盖所选来源在当前页返回的有限147样本；hasMore、partialTail、缺Footer和未知计数必须保留，不能称整文件／整次飞行峰值或完整维护资料。页内没有有效温度和非负电流时，native record 为空。没有生成振动RMS、退化率、flightCount／cycleCount、任务成功次数或故障原因；原 VIBRATION 三轴报告不能冒充旧维护 RMS 阈值。

## 实际护照与可见入口

[维护护照](FleetMaintenancePassport.md)的`appendTelemetryHistory(actualArchive,previewId,confirmed)`只从实际Archive提取本次私有record，返回固定5键`appendAccepted,error,assessment,previewId,historyScopeId`。成功先保record／同来源评估／完整回执，再通知；界面被销毁或换页不代表回滚，未知结果不自动重试。Root只把私有实际Archive和实际Passport配对，公共门面仅提供`maintenanceSources`、`prepareMaintenance`、`appendMaintenance`与准确ID的`discardMaintenance`，不收任意record，也不暴露native raw getter。

指挥中心原遥测历史页嵌入`FleetMaintenanceHistoryPanel`。只有原父面板`pageInputsCurrent`时才提供当前已保存页，维护预览／追加同意不借用原读盘或位置报告同意。plain票据和完整页／来源／同意代次覆盖按下、调用、结果发布与解忙，旧确认不沿用；选择后展示完整source7和evidence21，最后单独确认追加。概要按来源组数而非飞机数统计。

记录按完整source7重算scope，legacy／其它历史组分开；同scope序号闭区间重叠拒绝，2000满时不淘汰旧去重依据。历史按原序号选最新页，不把分页峰值作长期物理趋势。纯legacy文件仍schema1根5键；含历史时增加`historySourceContract='merivus-telemetry-history-source-v1'`成为根6键，导入要求该精确标记与真实历史记录同时存在。旧版根白名单拒绝新字段，避免未修改的新历史文件被静默剥离来源后合并；标记不是签名，文件仍是未认证历史资料。

## 待工程验证

该合同仅说明源码值与生命周期：实际选择、通知重入、页更换／取消／hide、同 ID 精确退休、全部未知、负温度／电流、UTC回退、短MAVLink2零尾、范围边界及重复导入都待工程师验证。未编译、执行项目／解析器／helper／QML／模型、测试、仿真、硬件或PX4操作、刷机或放行。原25目标／六类实质剩余保持，历史观察不恢复任何权限。

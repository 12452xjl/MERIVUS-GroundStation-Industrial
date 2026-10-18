# 净空资料的原生构型对应与本机时间窗口（453）

## 456：原source18由原生Witness跨owner保持

[Witness](FleetMissionStartupClearanceWitness.md)保原typed source18，不用Worker JSON副本替换。完整sourceCurrent复核原配置与UTC窗口，继以原Datum对应及所有回调后的整组memory；同一原Review时钟副本／deadline和UTC水位随实际授权链共享。owner fence拒绝仅本次不通过，真实源／时间／期限失败永久退休；旧Review自然退休不续期也不当物理源改变。

无新资料capture或场地hash重算，不证明实物尺寸、内部valid、整个允许运动净空或最终StartPermit；原pending保持。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：实际启动后台审阅中的原来源

本类exact7／稳定18及原窗口不变。两类实际审阅将原成功18与[Datum原14](FleetMissionClearanceDatum.md)保存在原生owner，另把纯JSON副本与同次实际几何交给[启动Worker](FleetMissionStartupClearanceWorker.md)。后台结果采用前后仍核原来源与UTC水位，不能以worker副本或报告hash恢复构型绑定、替换首次材料或延长期限。

本批开始接实际审阅结果，不代表两Authority／Transaction至最后write或完整StartPermit已完成。原Compiler pending仍阻断，阶段语义需随完整消费者继续开发；本类不运行solver或释放命令。未编译、运行、测试或放行，下方454为历史。

## 454：与显式工程高程对应分工

本类原exact7／稳定evidence18保持，不把`datumRelationEstablished=false`改成true。配置对应与本机资料窗口成立，仍不证明共同高程基准。两类实际启动审阅另外消费[ClearanceDatum](FleetMissionClearanceDatum.md)：使用原inputFingerprint、原成员配置证据与当前实际VERSION／PRIMARY／原点参考，匹配[目录schema2](FleetFirmwareAdapterCatalog.md)中明确的同datum／零偏移工程关系。

新证据只表示本地工程声明与当前来源对应，物理高程对齐、当前PX4全球高度分支、同估计器采样和飞行许可仍不认证。不能把GPI或ALTITUDE的AMSL字样、有限数字或曾经收到原点当作这些证明。旧无资料任务不补默认关系；有资料而缺对应时由原生审阅阻断。原资料UTC／单调期限和失效不复活机制不变，不因新增datum再起计时。完整实际启动solver／报告、两Authority至最终write及最终StartPermit仍待开发，原pending保持。

仅源码与限定静态核对，未编译、运行、测试或放行。以下453原合同继续适用。

`FleetMissionClearanceSource` 是非 QObject、非 QML 的原生取源函数。它读取确切计划的 `constraints.clearanceInput`、实际 Registry 私有构型绑定和本机 UTC；不运行几何 solver、不写文件、不发送消息、不获取账号批准，也不建立或续期任何启动许可。

## API 与固定结果

`capture(plan, actualRegistry, actualVehicles, fence)`；`actualVehicles` 为 `QMap<int, Vehicle*>`，必须恰好包含原计划全部 active 成员，不可增加备用机或重复/替代目标。输出按规范资料中与原 plan.members 一致的成员顺序保存，不按 QMap 的排序重排。

返回 exact7：

- `provided`、`inputValid`、`configurationAndTimeCurrent`：真实 Bool。
- `reason`：非 null QString；成功及正常缺省为空字符串。
- `evidence`：成功时下述完整18键 map；任何失败均 `{}`，没有首台成功车辆的前缀。
- `remainingValidityMs`、`observedAtUtcMs`：Qt qint64。成功时均为正整数且不超过 `9007199254740991`，失败或缺省均为 −1。前者是 `validUntilUtc − 本次最终UTC`，后者是本次最终 UTC epoch 毫秒，不是单调时钟时间。

先调用 `FleetMissionClearanceInput::inspectForPlan`。缺少声明时保持 `provided=false,inputValid=true,current=false`，不要求 Registry/Vehicle 存在。显式坏 constraints/声明按该检查器失败；其中坏 constraints 可为 provided=false/inputValid=false，不能当成正常缺省。已合法声明但实际构型或当前时间不满足时保持 inputValid=true/current=false。

`recapture(frozenEvidence18, actualRegistry, actualVehicles, fence)` 返回相同 exact7，专供原生审阅 owner 对**首次 capture 成功后保存在自身私有状态**的证据复核。它严格校验小型18键/原成员2键/原生13键，重新检查实际构型来源及当前时间，不再对最高4MiB的场地输入做 normalize/hash。成功返回的 evidence 必须仍为原18键逐值不变，不能用新构型替换或更新 inputFingerprint。坏原证据返回 inputValid=false，来源或时间失效返回 inputValid=true/current=false，均无 evidence 前缀。正常缺省没有18键，不使用此入口。

recapture 不认证一个任意 caller 提供的 map；不得从 QML、归档、JSON 导入或其他 owner 的历史材料恢复运行期依据。首次完整声明校验与原任务绑定仍由 capture 和持有该私有原证据的原生工作流承担；只读 timer/来源通知可用原生 memory-current 与 UTC 检查，真正需要捕获的阶段仍调用 recapture。

## 稳定 evidence18

字段固定为 `schemaVersion=1`、`source="native-local-configuration-and-time-window"`、`inputFingerprint`、`sceneId`、`sceneRevision`、`sceneSourceDocumentHash`、`acquiredAtUtc`、`validUntilUtc`、`horizontalDatum`、`verticalDatumId`、`members`、`snapshotOnly=true`，以及以下全部 false：

`physicalEnvelopeCertified,datumRelationEstablished,hardwareIdentityAuthenticated,performanceCertified,startupPermitGranted,flightCommandReleased`。

`members` 至多32条，每条 exact2 `{vehicleId:Qt Int, configurationEvidence:原生13键}`。资料 fingerprint 是完整规范输入的摘要；场地身份和日期来自同份规范输入。证据不包含 now、remaining、age 或重新生成的审批时间，重复合法取源不会仅因时间流逝改变稳定身份。

每机通过实际 `FleetRegistry::nativeConfigurationEvidence(actualVehicle)` 取得原生 exact3 成功回执及 evidence13。这里严格复核原生字段/类型、record context6、本地完整基线14、原JSON指纹和 typed 指纹的内部关系。输入车辆的 `configurationFingerprint` **只与原生13中的 JSON configurationFingerprint 对应**，不与 typed 指纹混比。基线必须完整且当前匹配；hash-only、导入标签、metadata 或 caller 传入的 evidence 不作为来源。

当前构型仍只对应实际原生观察到的有限字段。本地基线批准及相等哈希不能检测未上报的螺旋桨/机架/载荷改装，不证明声明的 body/rotor/gear 尺寸经过实测。`verticalDatumId` 与 `datumAlignmentDeclared` 也不由此变成 datum 对应证明；模块不应用默认安全偏移，不推导共同原点、PX4 内部 valid 或净空通过。

## 原生来源、回调与整组尾核

入口先冻结值输入和 fence callable，并为 Registry 和每个实际 Vehicle 建立 QPointer。每次可能回调前后都核实际对象、原 key/vehicle ID、同调用线程。fence 不是认证：它由原生调用方绑定原 review/generation/owner；拒绝或同步销毁后立即返回，不能再用原 raw 指针。

取得各机首份13键后保留，不用后续新快照覆盖。整组再调用 `nativeConfigurationEvidenceCurrent(actualVehicle, original13)`，它可能经过实际插件捕获，因此其返回后仍检查 fence 和全部对象。最后一次外部 fence 完成后，再对**全部原成员**调用无回调的 `nativeConfigurationEvidenceMemoryCurrent(actualVehicle, original13)`；之后只有本机 UTC 和原生对象内存检查，不再调用插件、外部 fence 或发信号。

recapture 同样从原18键提取原13键，执行整组 Current 与最后无回调 memory 尾核；只省掉完整场地解析，来源回调、首尾时间和不续期限制不省略。

这能阻止后机回调造成前机的已观察绑定/epoch/记录失效却仍发布旧前缀；不宣称远端原始遥测原子快照，也不能发现完全未进入观察体系的实物变化。失败不部分发布，不重建私有绑定或批准。

## UTC 与不可续期边界

日期必须沿原规范 `YYYY-MM-DDTHH:mm:ss.zzzZ` 解码。首次源回调前与全部回调/尾核后分别读取本机 UTC，要求 `acquiredAtUtc <= now < validUntilUtc`；结束 UTC 早于开始值则本次整份拒绝。没有“过期了再补默认日期”，不接受未来采集时间。

本模块是无持久状态的取源器，因此调用方必须在**第一次**有效捕获时，用自身单调时钟冻结 `min(资料剩余时间,原READ/审阅/账号期限)`；属性读取、重新批准、Authority 移交和重复 capture 只能缩短，不能按新的 remaining 重启 deadline。跨调用观察到 UTC 倒退也须由实际原生 owner 粘性退役，不能把本函数的一次通过当成跨阶段时钟连续性证明。

本机 UTC 不是可信时间认证。历史归档可保存和展示过期声明，但不得从历史恢复此原生 current 事实、运行期 epoch 或单调截止时间。新显式审阅可重新取当前合法依据，不复活已失效旧审阅。

## 静态交付边界

仅源码和静态审阅；未编译、运行 helper/解析器/测试、打开 UI 或连接设备。工程师仍需验证 Registry 原生桥接、Qt 类型、源销毁/重入/多机尾核、UTC 前后与跨阶段倒退、首次单调期限继承及两条启动工作流的实际消费。此证据即使 current=true，也不能解除尚未完成的 datum/物理模型/实际启动净空门槛。

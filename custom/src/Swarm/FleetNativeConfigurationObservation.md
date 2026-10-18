# FleetNativeConfigurationObservation：当前本机构型观察

## 453：统一实际来源与观察内容

`capture(Vehicle*, const std::function<bool()>& fence = {})` 是非 QObject、非 QML 的原生只读接口。成功和失败均返回 exact3：`ok`（Bool）、`reason`（QString）、`configuration`（Map）。成功 reason 为明确空字符串；失败 configuration 为空，不返回部分观察。

本类提取原 Swarm 的完整构型配方：机型/固件显示字符串、固件版本、已知 gitHash/UID、可用 motorCount 及 coaxialMotors/xConfigMotors、传感器 present/enabled 位。缺省的可选字段仍保持缺省，不将未知电机数量补为零，不新增机身、旋翼、起落架、质量或性能测量。

采集固定实际 Vehicle、system/component ID、ParameterManager、FirmwarePlugin、VehicleLinkManager、当前 MVM/model、PRIMARY 的 weak owner 和原生 VERSION 来源 epoch。实际模型必须恰有一个对应 system ID 且正是该对象；不延长 PRIMARY 生命周期、不调用 Link 虚方法、不增加原模型容量限制。Vehicle、相关管理器和调用者在原拥有线程；不要求 Link 的 I/O 线程等于 Vehicle 线程。

`Vehicle::autopilotVersionObservationSourceEpochMemory()` 只提供 callback-free 生命周期标识，不表示 VERSION 消息已收到、足够新鲜、UID 已认证或远端固件确为声明版本；它与 GPS 原点数据可用性无关。

采集前后核对来源与调用者 fence。显示字符串的翻译调用和电机插件调用后均重新核对；两次有界观察必须保持完整 typed 配置一致。fence 被值复制，调用方不得用它发送命令或修改飞机。源对象、组件、PRIMARY、管理器或 VERSION 生命周期在调用中改变时拒绝，不从同编号的新 Vehicle 补值。

## Registry 接入与三种核对

只有实际 Swarm friend 可在描述提交成功后绑定原生配置来源；普通描述、QML 值、护照导入都不能建立此绑定。Swarm 的采集失败会在 Discovery 重建描述之后再次保留 `configurationClear=true`，使在线 heartbeat 明确清除旧的完整当前配置，同时解除原生绑定；并不把旧配置当作本次观察。

Registry 的原生出口见 [FleetRegistry.md](FleetRegistry.md)：

- `nativeConfigurationEvidence(actualVehicle)` 重新采集原实际源，要求与私有提交配置、当前 Registry 配置及完整本地批准基线全部对应。
- `nativeConfigurationEvidenceCurrent(actualVehicle, originalEvidence)` 重采并精确核对原 evidence，不用新值替换原审阅依据；此方法可能调用电机插件。
- `nativeConfigurationEvidenceMemoryCurrent(actualVehicle, originalEvidence)` 是整组最后使用的无回调尾栅。它不虚派发插件、不调用外部 fence、不翻译显示字符串、不调用 Link 虚方法；核原绑定、管理器、PRIMARY weak owner、组件、原生机型/固件 enum、版本/UID/传感器本机字段以及 Registry 的记录、观察、批准代次与 typed 基线。

多机调用应先完成全部可回调重采和调用者 fence，再对整组执行 memory-only 尾栅，不能把第一架的旧前缀与最后一架的新回调结果混成同时有效证据。返回仍只是该次本地捕获，不保证返回后永久有效。

## 明确边界

这里不是硬件身份认证、现场构型检验或全参数日志。未上报的物理改装无法由这些属性得知；采集不是同一远端传感器采样。电机插件字段由两次读取核对，memory 尾栅不再次虚派发插件。已知原生配方的参数依赖另外固定保存：APM Copter 的 `FRAME`、潜航分支的 `FRAME_CONFIG`。原字面名字不走 `r.` 版本映射；只接受现有原组件/原名字 Fact，不请求缺参数、不接受默认错误 Fact。绑定先装 Fact 销毁、rawValueChanged 和原组件 factAdded 退休 hook，再完整重采核原配置；memory 尾栅直接比较原 Fact 的 typed rawValue。factAdded 不解引用可能已被其它接收者销毁的原始指针，原组件任意新增均保守退休。由此这些已知依赖的 A→B→A 不能恢复旧 epoch，缺少依赖即无绑定。PX4 默认分支不增加这些 Fact 依赖。

任意一次原生构型重采失败或观察到不同配置，都会按原 bindingEpoch 退休该来源；不撤回已提交的 Registry 记录或批准事实，也不删除重入时新建的绑定。任意第三方插件的额外内部状态或未通知参数不因此成为已完整观测的硬件历史。

APM Copter 还绑定原实际 `ArduCopterFirmwarePlugin` 对象以及共享 coaxial 状态。memory 尾栅显式限定调用该类现有 `multiRotorCoaxialMotors` 固定实现，而非 Vehicle 或基类的虚派发；已静读该实现仅返回本机 bool，无回调。这样其它机处理 STATUSTEXT 改变共享状态后不能保留前机旧依据。无 APM 插件的裁剪构建通过条件登记不引用该类实现，相关不受支持分支不给原生证据；PX4 分支不受其影响。

`configurationFingerprint` 是原完整配置的 JSON 指纹，用于工程资料对应；`currentTypedFingerprint` 是本地完整 typed 配置/基线比较指纹。两者不能互换，也不能与 `startupConfigurationFingerprint` 混比。成功仍明确不认证硬件身份、物理包线和性能，不放行飞行命令，不代替账号、审批、启动准入或末端 write 门禁。

未请求参数、写参数、创建飞行任务或发送命令。本批只做源码与静态文本检查；未编译、测试、运行 helper、应用、仿真或设备。实际重入、来源切换、导入/清理、批准撤销及真实构型刷新仍需工程师验证。

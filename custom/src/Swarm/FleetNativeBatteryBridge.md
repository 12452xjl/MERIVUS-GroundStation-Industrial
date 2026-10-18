# 431：原生电池报告到逐机能量审阅

## 516：原生SOC下降窗接到实际审阅

新增独立`nativeBatteryDrainEvidence`，不修改原电池outer20/schema1或2。Vehicle只在原PRIMARY电池报告的实际提交后保有界本地下降窗；Registry在原Vehicle／recordEpoch／bindingEpoch和同一投影busy内读取两份证据，读取前后保原绑定，不借Fact、护照或静态metadata重建。新保留键从root和metadata输入剥离，返回快照也先剥静态同名值再附真实读取。来源不可用返回合法unknown；实际producer坏值保为空map的明确invalid，不能洗成unknown后借较宽静态预算通过。

30–120秒、至少下降2个百分点、相邻原始报告间隙不超过3000ms、最多256个保留点才可发布窗口；最新原RAW年龄不超过3000ms，下降终点不超过30000ms。flat不刷新终点或速率。模型扣龄取最新RAW年龄与下降终点年龄的较大值，不能凭相同SOC重复包重新扩张已发布窗口预算。原电池20字段沿原较晚收包计时，新通道另保最早RAW时刻；两者不要求逐毫秒相等，都不是远端采样时间认证。

Scheduler只min已有known时间／已有known电量投影，不用窗口独立满足严格性能声明，不把未知改成已知。完整证据及前后值进入`energyPlan.nativeBatteryDrainAdjustments`，原assignment18/21与编译预算内容不改；完整energyPlan沿现Coordinator→归档证据hash→报告／515完整明细，不新增档案schema或控制路径。

Root以Coordinator共享值校验桥读取exact6：`state,identity,usable,remainingFlightSeconds,percentPerSecond,ageMs`。有新键但坏值拒绝，缺键保legacy；identity只保窗口／来源语义，不含每包receipt及自然增加年龄，窗口失效／换源／重建会撤旧审阅。桥的ageMs是预算用effectiveAge，不是证据top的RAW age。批准期还核实际当前窗口是否覆盖原账单需求、扣龄投影是否新跨原替补阈值；不改写旧账单，不新增投影最低电量硬门，也不重新批准。合法unknown窗口仅不增加模型，不能恢复之前窗口身份所绑定的旧批准。

报告SOC和线性外推不是电功率实测、实物电池身份、未来工况／续航保证或飞行许可。仅源码及静态对读，工程场景全部未执行；未编译、运行项目或helper、测试、仿真、操作硬件／PX4、刷机、放行、提交、更新PR或打包。下方为历史。

本链路只优化地面站，不改变飞控固件、电池硬件或机载控制。`BATTERY_STATUS.time_remaining` 是飞控报告的估算，不是实测可保证续航；报文没有物理电池序列号或电池测量时间，地面站只能记录本机收包年龄。

## 实际数据路径

1. `Vehicle` 在插件／公开观察回调前记录原始 PRIMARY `BATTERY_STATUS`，独立于原健康 SYS_STATUS／ESTIMATOR_STATUS 记录及普通电池 Fact 缓存。详见[原生观察合同](FleetNativeBatteryObservation.md)。
2. `SwarmController::_refreshFleetRegistry` 早期取得原生观察，在发布健康描述前再次取源，核原机号／组件、sourceEpoch、选定电池槽位、contentRevision、百分比与3秒本地鲜度。改变、过期或未知时，本轮健康电量为 -1，不平均推进、航电、载荷或陈旧电池。
3. 原生发现调用完普通 register／markSeen／update 后，再核原实际 Vehicle 与 MVM 身份，才调用 Registry 私有 `bindNativeBatterySource`。未在原实际列表观察到的机号解除绑定。描述符、普通 metadata、护照与账号批准都不能安装这个源。
4. [Registry](FleetRegistry.md) 的公开快照按原绑定重新读取原生电池源；仅凭1Hz刷新或旧快照不能延长电池数据有效期。无源、导入、原对象更换或过期时，公开电量和动态剩余续航保持未知。
5. [Scheduler](FleetEnergyScheduler.md) 将当前报告作为逐机预算的一种估算依据；[规划页](FleetMissionEnergyBudgetPanel.md)显示报告来源与年龄。现有 Guided 审阅身份还绑定来源与内容，且在批准检查时复核当前剩余时间是否仍覆盖原账单需求。

## 两种不同时间

`ageMs` 是本机单调时钟下的收包年龄，不是飞控采样年龄或链路往返时延。动态剩余时间按“原报告秒数 - 本机已过去时间”夹到0；原报告值仍单独保留。重复报告不改变内容身份，但会更新本机接收年龄；这不证明飞控真的重新测量过电池。

原始入站／提交继续核真实连接、非模拟／回放来源；公开getter仅作无回调的本机内存连续性投影，不调用Link虚方法或临时锁定强所有者。因此sourceCurrent不表示即时远端可达。Swarm前后捕获异常标记每次重算，来源恢复不会永久保留旧异常；该标记仍不是远端电池故障诊断。

0或未提供的 MAVLink 剩余时间不作为“已知耗尽”；有效正报告扣龄后到0才是该估算已用尽。只有百分比时，仍可与原静态满电续航声明一起估算，但不能补造直接剩余时间或认证声明的物理精度。

## 有限的电池拓扑支持

首版只聚合唯一明确的 ALL／PROPULSION 电池报告。用途未知或未定义、多个推进电池、推进报告过期、槽位数量超限均不选择最乐观值，也不取平均。明确辅助电池单独保留说明，不参与推进续航计算。16槽位是资源上限，不是安全支持16块任意电池拓扑的声明。

换电可能沿用同一batteryId；本功能不能检测物理电池更换，不能证明串并联关系、备用切换逻辑或真实推进电源完整性。后续仍需拓扑声明／工程验证和相应多电池模型。

## 静态交接边界

未编译、测试、运行helper／UI／应用／仿真或设备，未改PX4、刷机、提交、更新PR或打包。工程师需验证真实PRIMARY收包、非主链路与模拟／回放排除、缺字段／短报文、同号多组件、多电池用途、收包重入／失源／过期／重连、护照导入不能恢复live数据，以及预算边界和界面未知状态。以上都是待验证项，不是执行记录。

该链路不发送命令，不改变原人工审批／启动权限，不补齐全部能量模型。风、载荷、温度、老化、起降与备用待命／接替仍未建模，原25目标和六类实质开发继续。

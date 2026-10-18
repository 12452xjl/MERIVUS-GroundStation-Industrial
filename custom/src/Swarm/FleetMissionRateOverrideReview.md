# FleetMissionRateOverrideReview 参数总速覆盖一致性

## 479：独立纯值模型，实际消费者是native证据

新增无QObject的纯C++类，`evaluate(parameters,guardParameters)`只比较输入内容，不读取Vehicle／Fact、发送请求、写参数或改变任务。`parameters`为原按名排序12行，guard为按名排序的MPC_XY_VEL_ALL／MPC_Z_VEL_ALL两行。接收有界实际Map、已知字段及真实有限数字，八个相关速率和两个guard要求MAV_PARAM_TYPE_REAL32（本地MAVLink枚举值9）且原值可精确表示为float32；不先舍入任意double制造一致性。缺项／错序／重复／未知键／typed-null／错误类型整份失败。

返回恰六键：`ok`、`reason`、`model=local-px4-rate-overrides-v1`、`expectedParameters`、`macroOverridesActive`、`actualInternalValuesProven=false`。成功的active map恰horizontal／vertical两项Bool，expected是全部活动覆写产生的按名排序name／value行，最多8项；没有活动宏时是明确两个false与空expected，不认证原12项范围或鲜度。任何失败不给expected前缀或部分active结果。

## 本地源码规则与实际比较

对读相邻源码快照 `FirmwarePX4/src/modules/mc_pos_control/MulticopterPositionControl.cpp` 参数更新分支118–139。该目录没有Git元数据，无法据其目录名／文档确定真实提交或真机二进制。本模型是对应本地已读源码的数值一致性，不是固件签名认证；实际参数／版本／工程声明与最终原源由调用者分别检查。

| 宏条件 | 要求相同的原始回执参数 | 期望float32值 |
| --- | --- | --- |
| XY_ALL≥0 | MPC_XY_CRUISE、MPC_XY_VEL_MAX | XY_ALL |
| Z_ALL≥0 | MPC_Z_V_AUTO_UP、MPC_Z_VEL_MAX_UP | Z_ALL |
| Z_ALL≥0 | MPC_Z_V_AUTO_DN、MPC_Z_VEL_MAX_DN | float(Z_ALL × 0.75f) |
| Z_ALL≥0 | MPC_TKO_SPEED | float(Z_ALL × 0.6f) |
| Z_ALL≥0 | MPC_LAND_SPEED | float(Z_ALL × 0.5f) |

各自负数表示宏关闭；正负零都满足原源码的≥0，不能fuzzy-zero处理。非正派生速度或任一原始回执不等即失败，reason固定前缀并带对应已知参数名；不把expected替换raw或写回飞控。相乘使用原float操作数和float系数，转double只为JSON安全的有限表示，不推出完整数值误差认证。

## 真实消费者与来源

[VehicleEvidence](FleetMissionVehicleEvidence.md)从PM真实固定14账本取原12与guard2，保旧12 content hash；guardRows／modelReview／usable／sourceCurrent完整进入stable证据和启动配置指纹。当前 `parameterEvidenceUsable` 同时要求原12意图／hash／范围、guard鲜度／来源／缓存／epoch和本模型ok，并在所有外部getters返回后对原14行作纯内存尾核。模型不验证age、source或批准，所以裸ok不能单独用于准入。

[Refresh](FleetMissionEvidenceRefresh.md)只在原人工确认下请求固定14项；[Readiness](FleetMissionStartReadiness.md)携同原PM／PRIMARY弱控制块／原14行到最终诊断和phase回调之后核对，不重采样覆盖。旧12profile hash／公开19键观察offer保原合同；guard材料域另受abs≤2^53−1约束，纯模型fullfinite REAL32的比较能力不能扩大实际审阅数值域。

## 没有完成的内容

本模型不观察实际飞控内部哪个相位正在工作。TKO ramp／近地起飞、LAND分段／测距／人工RC辅助、紧急制动、SYS_VEHICLE_RESP加速度／jerk改写、动态acceptance和精降都需要独立参数与内部状态合同；AUTO、TKO、LAND及MAX不是恒定实际速度或完整硬限。参数一致不证明路径、完成时刻、动力学、动态净空、StartPermit、正式结果或整段飞行持续参数历史。所有运行、构建、仿真／硬件验证均未执行。

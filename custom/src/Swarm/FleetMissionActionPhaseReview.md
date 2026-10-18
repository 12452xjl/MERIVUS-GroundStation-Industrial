# FleetMissionActionPhaseReview 起降参数分支复核

## 480：固定纯值合同，不推断当前内部相位

新增无 QObject 纯 C++ 类。`evaluate(parameters,phaseParameters)`只读已有值，既不请求／写参数，也不读取 Vehicle／Fact、调用飞控、运行状态机或生成许可。原参数是按名称排序的完整旧 12 行；phase 是按名称排序的固定 14 行：

`COM_OBS_AVOID`、`COM_SPOOLUP_TIME`、`MPC_ACC_DOWN_MAX`、`MPC_ACC_UP_MAX`、`MPC_JERK_AUTO`、`MPC_LAND_ALT1`、`MPC_LAND_ALT2`、`MPC_LAND_ALT3`、`MPC_LAND_CRWL`、`MPC_LAND_RADIUS`、`MPC_LAND_RC_HELP`、`MPC_TKO_RAMP_T`、`MPC_Z_VEL_P_ACC`、`SYS_VEHICLE_RESP`。

每行只允许与 [RateOverrideReview](FleetMissionRateOverrideReview.md) 相同的 15 个已知字段，其中 name／mavParamType／value 必须真实、非空且有正确类型。模型不采用观察元数据作为可信性证明；调用者另核原生来源、完整观察行、缓存、pending、版本、鲜度和原源连续性。缺项、重复、错序、未知键、必需值 typed-null 或非真实有限数字整份失败，不截断保留前缀。

返回恰 8 键：`ok`、`reason`、`model=local-px4-action-phase-parameters-v1`、`expectedParameters`、`actionBranches`、`takeoffCompatible`、`landCompatible`、`actualActionStateProven=false`。失败时 expected 为空列表，branches 为空 Map，两个 compatible 都为 false。成功只表示固定参数内容、支持数值域及响应宏关系一致；不是内部相位、轨迹或开始执行的证明。

两个 compatible 只描述本地前置：takeoff 要求 `COM_OBS_AVOID=0`、近地加速度有限且大于零，以及独立控制器 ramp 初始参考有限；land 要求 `COM_OBS_AVOID=0`、`MPC_LAND_RC_HELP=0` 及插值系数有限，台阶分支的有限标志为 true。打开 RC 辅助仍能完整报告参数分支，但不能把人工横移、航向及下降改变纳入既定自动 LAND 轨迹。打开避障注入亦不抹去已知内容，两个兼容前置均不成立。LAND 系数非有限只阻断 landCompatible；近地加速度下溢为零或 RAMP 为负零导致非有限值只阻断 takeoffCompatible，不把无关动作一并拒绝，也不把原输入／宏结果改为失败。它们不单独批准任务、Arm 或切模式。

## 数值域与旧 hash

旧 12 项 hash／参数列表不扩展；此模型另外消费 phase。旧 8 项速率和 phase 的 12 个浮点参数必须为 `MAV_PARAM_TYPE_REAL32=9`，且原输入精确可表示为 float32；不先舍入任意 double。phase 的两个开关必须为 `MAV_PARAM_TYPE_INT32=6` 且值只可 0／1。

所有原输入数值要求真实、有限且绝对值不超过 `2^53−1`。ALT1／2／3 支持 0..122，RAMP 支持 0..5，SPOOL 支持 0..30，RESP 支持 −1..1；radius 非负；加速度、jerk、crawl 和 Z 速度 P 增益为正。其余旧 12 范围沿现有证据合同：MIS_TAKEOFF_ALT／NAV_ACC_RAD 非负、YAW_MODE 为整数、其他为正。本模型不认证旧 12 的全部 wire 类型／固件元数据或真实飞行能力。派生 LAND 系数的材料化域单独处理，不是原参数合法性的附加范围。

上述域是本地复核支持域，不能解释为固件统一硬裁。特别是 SYS_VEHICLE_RESP=0 会产生 ACC_DOWN_MAX=0.8f 和 ACC_UP_MAX=1f，不能照搬元数据最小 2 将原生宏合法结果拒绝。ALT3 不强加小于 ALT2 的新条件；ALT1／2 相等、RAMP 为正零／负零、SPOOL=0、radius=0、RESP 为负或正负零均明确处理。RAMP 的负零属于已知原内容，不 normalize 为正零，也不只按 JSON 数字相等认定原源保持。

## 实际消费的本地源码分支

参考是相邻 `FirmwarePX4` 当前文件的静读，目录没有 Git 元数据，不据名称认定实际安装二进制、版本 SHA 或签名。没有修改 PX4。

- `MulticopterPositionControl.cpp::parameters_update`：RESP≥0（含零）时先计算 float32 `s=RESP*RESP`，再按 mathlib 原 `lerp` 顺序 `(1-s)*low+s*high` 对 DOWN=(0.8,15)、UP=(1,15)、JERK_AUTO=(1,25) 推导。每次减、乘、加保持 float32 中间量，与原回执逐值比较；负 RESP 不覆盖。成功 expected 只有活动宏的按名称排序完整 3 行 name／value，未活动则为空；不替换 raw，更不写回飞控。
- `FlightTaskAuto::updateParams`：内部 ALT1 使用 `max(rawALT1,rawALT2)`，报告是否被局部抬高。这个结果只进分支说明，不改原回执。
- `FlightTaskAuto::_updateTrajConstraints`：只有前一周期 unsmoothed Z 速度向上、当前为 TAKEOFF 且 dist_to_ground 严格小于内部 ALT1 时，采用 TKO_SPEED 和 `min(ACC_UP_MAX,TKO_SPEED/TKO_RAMP_T)`；否则普通向上分支用 AUTO_UP、ACC_UP_MAX。本模型真正计算名义近地加速度：RAMP 为正零时源除法得到正无穷，min 取 ACC_UP_MAX；RAMP 为负零时源除法得到负无穷，min 亦取负无穷。本模型不直接除零，显式保留两分支，负零时近地加速度字段为 null、nearGroundAccelerationFinite=false、nearGroundRampNegativeZero=true，takeoffCompatible=false。正数除法溢出为正无穷时，source 的 min 仍选有限 ACC_UP_MAX。合法正输入的 float32 比值若下溢为零则如实保留并令 takeoffCompatible=false，不把它变成输入／宏失败、不阻断 landCompatible，也不伪称足够加速度。上行／下行及 jerk 的数值是平滑器约束参数，不是实际速度硬界或时刻。
- `FlightTaskAuto::_prepareLandSetpoints`：距离≤ALT2 先取 LAND_SPEED，距离>内部 ALT1 取 Z_VEL_MAX_DN，中间按原 float32 斜率和截距插值；阈值相等时是台阶，不做除法。两个系数都有限且绝对值都≤`2^53−1`时才成对输出数值，否则两个字段都为 null，不返回数值前缀，不钳制、截断或扩大原输入域。有限但超过材料域不阻断 landCompatible，更不阻断 TAKEOFF；任一非有限则 interpolationCoefficientsFinite=false 且 landCompatible=false，但 ok 仍只反映参数内容／支持输入域和宏关系。null 从不表示安全或无需复核。有效有限底距≤ALT3 时 crawl 再覆盖前值；RC 辅助且 stick 有效时再乘 `(1-throttleExpo)`，名义乘子可为 0..2，并可调整 XY 和 yaw。报告 radius 是声明，不是几何包络或可证明净空。
- `mc_pos_control/Takeoff`：独立位置控制器斜坡的初始参考是 float32 `−9.80665/max(Z_VEL_P_ACC,0.01f)`，并读取 spoolup／ramp 时间；rampDisabled 在正负零时都为 true，表达该类 updateRamp 不执行渐变的分支，不代表 FlightTaskAuto 的除法约束也相同。两处斜坡语义必须分开；不能把任一者误当当前速度、起飞完成时间或另一处约束。状态、进度、armed／landed／want_takeoff、loop dt 和本次终点约束均未知。
- `PositionSmoothing`／`VelocitySmoothing`：LAND feedforward 还会经平滑器；其 downward target 受 AUTO_DN 约束，但当前内部速度／加速度与前一步轨迹不由这组参数证明。不得将 LAND 原始 feedforward、AUTO_DN 或本模型分支直接称为整段实际下降速度。

actionBranches 成功时恰 8 键：responsivenessMacroActive、takeoff、landing、positionControllerRamp、externalAvoidanceSetpointInjectionEnabled、groundDistanceAndBottomValidityKnown=false、emergencyBrakingAndSmoothingStateKnown=false、actualVelocityTimingOrTrajectoryProven=false。子 Map 是固定有界字段（takeoff 11、landing 23、positionControllerRamp 6），不复制未知输入，也不填虚构当前状态。takeoff 的 nearGroundAccelerationFinite 和 nearGroundRampNegativeZero 独立说明负零分支，非有限加速度只输出 null，绝不输出无穷或把 null 当安全；currentBranchKnown 仍为 false。landing 另有三个固定字段：interpolationCoefficientsMaterialized 仅当非台阶且两个系数完整可材料化时为 true；interpolationCoefficientsFinite 在台阶时为 true，非台阶时要求两个系数都有限；interpolationFormula 固定保留来源运算顺序及台阶规则，描述的是公式，不是已求出的当前速度。相等阈值时两系数 null、Materialized=false、Finite=true，显式台阶标志为 true。

## 未观察与未完成

真实 dist_to_ground 优先用有效底距，否则用当地 home Z，否则本地 Z；AMSL 任务点不能充当该值。没有从任务坐标推断地形、测距有效性或内部当前分支。紧急制动可改用 9.81 的加速度和 jerk，避障、精降、yaw 对齐、RC、估计器 reset、平滑器当前态和控制器外部约束也可改变真实 setpoint。

NAV_MC_ALT_RAD、动态到点半径、完整平滑模型、实际速度／飞行时间／动力学、真实障碍与动态净空、StartPermit 和正式结果不由本模型完成。仅做源码静读，未执行本模型、helper／解析器／业务脚本、编译、测试、仿真、界面、硬件或刷机验证。

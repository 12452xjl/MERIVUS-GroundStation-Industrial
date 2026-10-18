# 逐 NAV 航点机头方向编码（536）

本模块是 GroundStation 的共享纯值编码，不发送任务、不读设备、不调用 QObject、文件、时钟或网络。只为逐点 `heading`／`yaw` 生成 `MAV_CMD_NAV_WAYPOINT.param4`；不是 Draft.headingDegrees 的扫描方向、起飞／降落／HOLD yaw、实际姿态或 StartPermit。

## 原请求与上传网格

`fromPoint(point, Request&, error)` 接受 heading／yaw 的实际非空有限 Int／UInt／LongLong／ULongLong／Double，连续定义域为 `[0,360)` 度；string、bool、typed null、NaN、Inf 和域外数拒绝。两别名都在时，必须在量化前原数值相等，不能先分别归格再把冲突当一致。缺字段返回 provided=false，绝不添0；规范任务可保原 `heading` 连续请求。数值正／负零都表示显式零方向，仍与缺省 null 不同。

Request 是普通值 `{provided,requestedDegrees,tick,uploadDegrees,circularDeltaDegrees,wrappedToZero}`。只给逐 NAV 点使用，不从 launch／endAction 坐标或构型扫描方向继承。fromPoint 失败清 Request，并给现有固定错误码 `unsupported_wire_parameter`。

声明编码分辨率为 `q=1/256°`。令 `x=requestedDegrees*256`，取 `lower=floor(x)`、`fraction=x-lower`；fraction≥1/2 时取 lower+1，否则 lower。恰好半格向较大 tick；rounded=92160 时显式 wrap 到 tick0，而不是上传360。最终 tick∈0..92159、uploadDegrees=tick/256。

乘2的幂在此有限 binary64 域内精确；lower 是可精确表示的整数，fraction 的减法也精确。实现不用 `floor(x+0.5)`，避免加法舍入把“刚低于半格”推过界。网格上传值是小于2^17的整数除2^8，故全部可由 binary32 精确表示；不只支持四个 cardinal 方向。圆周量化误差绝对值≤1/512°，wrap 时差直接算 `360-request`，其余为 `upload-request`。原请求、上传值及带符号圆周差分别保留，不把任务请求暗改成上传网格。

`diagnostics(Request)` 对实际一致的 provided Request 输出 exact10：`schemaVersion,model,requestedDegrees,uploadDegrees,tick,circularDeltaDegrees,wrappedToZero,stepDegrees,maximumCircularQuantizationErrorDegrees,maximumReadbackErrorDegrees`。schemaVersion=1，model=`nav-waypoint-heading-tick256-px4-float-bound-v1`，step=1/256，最大圆周量化差=1/512；缺省或不一致 Request 返回空 map。这只是编码诊断，不是授权或已测得 heading。

## 本地 PX4 源码与运算界

只读依据是当前 `E:/feikong/FirmwarePX4` 文件，而非已认证固件基线：

- `src/modules/mavlink/mavlink_mission.cpp:1374` 写入 NAV_WAYPOINT：`yaw=wrap_2pi(math::radians(float param4))`；`:1696` 回读：`param4=math::degrees(float yaw)`。
- `src/modules/navigator/navigation.h:165` 的 yaw 是 float；`src/lib/mathlib/math/Limits.hpp:97–105` 的 radians／degrees 分别乘 Float(π)/Float(180) 和反向比值。
- `src/lib/matrix/matrix/helper_functions.hpp:27–41,105–108`：已在 `[0,Float(2π))` 直接返回；否则 floor wrap。
- `src/modules/navigator/mission_block.cpp:670–671` 把有限 yaw 送入 setpoint／yaw_valid；这不证明真实航向达到、偏航动态或安全。

下面是本版本声明的正常有限 binary32 逐运算模型，不把本机运行浮点公式当机载事实。取 `ε=2^-23`，保守使用每次正常 Float 舍入相对误差≤ε（宽于 round-to-nearest 的半ε界）。π默认常数到 Float、π/180、上传值乘比例共三次舍入，故弧度相对误差由 `(1±ε)^3` 包住。

非零最小上传值 q 的弧度大于1/65536，远离 binary32 subnormal；最大上传值是360−q。它与2π的相对间隔为1/92160，且1/92160>5ε；由于 `(1+ε)^3≤1+4ε`，即使同时保守降低 Float(2π) 边界，最大弧度仍严格在上界内。非零下界保持正数，tick0独立产生0。因此整个网格都进入 wrap 的 in-range 分支，无反复 floor／大角度求余或极小非零下溢。

再保守分别计算反向比例中的 π 舍入／除法和最终乘法，不利用两方向相同 π 误差的抵消，得到 `y/θ` 落在 `[(1−ε)^5/(1+ε),(1+ε)^5/(1−ε)]`，偏差小于8ε；θ≤360 时绝对误差小于8ε×360度。所选合同界为更宽的 `B=16ε×360=0.0006866455078125°`，不是按例子试出来的任意 epsilon。即使把正常 reciprocal-form 的两个额外舍入单列，`(1+ε)^7/(1−ε)` 的误差仍小于10ε，保守界仍覆盖该声明模型。

`B < q/2 = 1/512°`，所以最近网格接受窗不相交；一个已解码 Float32 不会同时归属两个 tick。绝对界统一保守，不声称每个 tick 的全部窗内数都必由该机实际计算产生。圆周0／360附近同样用唯一最近圆周 tick；不采用任意角度忽略、只四方位或全局“足够接近即任务相同”。

本地 CMake 仍含 `-fno-signed-zeros`／`-freciprocal-math`；本轮没有运行编译器、算法或固件，也不能认证实际编译配置、FPU模式、修改后的固件、第三方实现或消息来源。该路径不是 Git 仓库的当前事实由主线程只读检查记录，文档中的旧来源地图不能恢复当前 HEAD／子模块认证。现场适配、原 firmware/parameter/source 门及实际 RAW 回读仍须独立核对。

## 三种表示不能混淆

1. 原任务请求：连续 requestedDegrees，不能被 tick 值覆盖。
2. 上传原值：uploadDegrees=tick/256，发出的真实 Float32；对应上传字节／RAW 摘要必须仍保原来源。
3. 实际回读原值：`decodeParam4` 保存收到的 Float32 原数值，不量化、不 wrap、不从原请求推算，也不回填预期值。若有 RAW 帧／payload hash，它仍绑定实际原字节。

`canonicalParam4(command,param4,yawMode,false,...)` 仅接受 NAV_WAYPOINT 的 `absolute-degrees`、实际 Float32 可精确表示的 `[0,360)` **既有 tick**；不能把已改变的 upload row 再次量化修好。true 分支只对实际 Float32 可精确表示的0..360度，在与最近圆周 tick 的距离≤B时返回独立的语义代表；否则拒绝并保真实差异／Unknown，不能取另一个更远 tick。360边界的圆周代表是0，但调用者原 raw=360 不被修改。

`decodeParam4` 对有限 NAV 值标 absolute-degrees，保原 Float32 数值；地理命令 NaN 保 null／system-default，不能变为0。有限 TAKEOFF／LAND／HOLD 及其它命令仍标 unsupported-command，不新增其 yaw 语义；DO_CHANGE_SPEED 保原值／not-applicable，只有 canonical 验证其保留零。Inf 或不支持 Float32 平台失败，不把错误变缺省值。调用者须保 raw 输入与错误诊断，不能在失败后补0。

`canonicalParam4` 仍接受旧地理 null／system-default、速度保留零／not-applicable；非 NAV 不获得 explicit-heading 支持。错误统一为现有 `unsupported_wire_parameter`。它不执行机载 radians／degrees 仿真，也不以本机预测替换实际回读。

## 版本身份与消费者

`wireSchema(false)`=`mission-item-int-plan-v2`，`comparisonBasis(false)`=`canonical-v2`。没有显式 heading 的旧任务维持旧域。

显式 heading 使用 `wireSchema(true)`=`mission-item-int-heading-tick-v3`，`comparisonBasis(true)`=`canonical-heading-tick-v3`；`isSupportedWireSchema` 只接受这两个实际字符串。v3 canonical 中 param4 仍是 degree 数值，不是 tick 整数换单位。v3 canonical hash 表达网格及声明 Float 往返界内的 heading **语义等价**，不是实际上传／回读原字节逐位相同。actual raw 与 semantic canonical 不可合并、用同一摘要冒称字节认证或抹去实际回读差。

原 Payload／Deployment／RAW 回读／ReadbackDiff／严格模型校验共同调用此 helper；逐点原请求、上传 tick、实际 Float 和归格结论应各按原 source/sequence 对应。不能仅因一个 canonical hash 匹配就称飞控执行、物理机头保持、独立动作 yaw、净空、权限或 StartPermit 已通过。

本批仅源码、合同和直接静读；未运行编码／数学／解析／hash实现、helper、应用或算法，未编译、测试、预览、连接设备、修改 PX4、刷机、提交或打包。

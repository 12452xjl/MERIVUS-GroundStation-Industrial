# 启动高度容差的定长精确比较（446）

`FleetMissionAltitudeArithmetic` 是不依赖 QObject、Vehicle、I/O 或第三方大整数库的纯计算类。它不建立高度来源、参考有效性、认证或启动许可，只避免 v4 数值门禁被中间 binary64 舍入误放行。

## 接口与输入域

`absoluteSumAtMost(std::initializer_list<Term> terms, double maximum, int maximumScale = 1)`，其中 `Term = { double value; int coefficient; }`。精确判断**传入 binary64 所表示的数值**是否满足：

`abs(sum(value × coefficient)) <= maximum × maximumScale`

- 项数 1–4；每个 value 有限且绝对值不大于 `2^53−1 = 9007199254740991`。
- 系数为整数 −1000…1000；系数为零也不跳过该 value 的合法性检查。
- maximum 是实际 double，数学值 0…1000；maximumScale 是整数 1…1000。
- 输入非法、宿主表示不支持或任何整数溢出均返回 false；没有夹紧、epsilon、浮点除法或先算浮点差再比较。
- 正负零在这项数学比较中同为零；原始报文位模式与同 receipt 的负零一致性仍由原高度 helper 负责。

原 reference20/sample20 和原高度来源合同不变。适用调用例如以三个原始字段比较 `1000×AMSL − 1000×local − originMm` 与 `1000×容差`，而非先将毫米除以 1000 或将 AMSL/local 相减。相对首次 sample 的漂移使用四个原始高度项。显示用的 reported 差值不因此变成原生许可依据。

## 整数表示与固定容量

通过 `memcpy` 读取 IEEE binary64 位模式，不经浮点类型转换。以 `2^-1074` 为统一整数单位：

- 次正规数与零：mantissa 为 52 位 fraction，shift 为 0。
- 正规数：mantissa 为 hidden bit 加 fraction（最多 `2^53−1`），shift 为 biased exponent 减 1。
- 符号与系数符号决定分别累加到正、负两个数组，最后取精确绝对差；阈值同法独立构造。

每个数组固定为 36 个 uint32 limb，即 1152 位。输入最大 `2^53−1` 的 biased exponent 为 1075，因此最大 shift 为 1074；mantissa 乘最大系数 1000 小于 `2^63`，单项最高所需位数不超过 1137，最多四项同号累加再需至多两位，即不超过 1139 位，低于容量 1152 位。阈值不大于 1000×1000，其容量更小。这里的界是容量证明，不代替运行时溢出检查。

mantissa 乘系数前明确检查 `mantissa <= 2^53−1` 与 `coefficient <= 1000`，由此保证 uint64 乘积严格小于 `2^63`；不靠可能溢出的乘积反推合法性，不使用除法。写入时先拆成两个 uint32 chunk，再以 0…31 位 shift 生成至多 63 位 temporary；不会移位 32 位到 uint32，也不会移位 64 位到 uint64。单 limb 加 carry 至多需要 33 位；超数组尾仍带非零 carry 时返回 false。绝对差先确定大小，再逐 limb 借位，最终借位不为零同样拒绝。

最大 value/容差的合法性以正 IEEE 编码序比较；NaN、无穷、超域值和负的非零容差不能通过。极小次正规 maximum 不被舍入到零：它的原始整数 mantissa 与 scale 精确相乘。maximum 为零仅接受精确抵消为零的总和；恰好等于阈值接受，任意正的最小二进制单位越界拒绝。

宿主必须具有 IEC 559、radix 2、53 位有效数字、正确 binary64 指数范围及 32/64 位无符号整数；另核已知常量的实际位布局，非匹配/混合字节布局拒绝。固定数组没有动态分配或无界输入循环。

## 验证边界

本批只写源码并静态阅读，未编译、运行比较器、执行测试或连接设备。上面的容量与边界是源码推导，不是运行通过记录。工程师仍需进行构建、位模式/次正规/零容差/边界相等及越界、抵消、非法输入和真实调用链验证。

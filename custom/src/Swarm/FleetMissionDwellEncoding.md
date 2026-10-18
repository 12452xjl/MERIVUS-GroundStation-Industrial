# FleetMissionDwellEncoding：逐NAV显式驻留的数值模型

## 537：原请求、上传值与模型等待量分别保留

无QObject的纯值helper。`Value` 仅有 `requestedSeconds`、`uploadSeconds`、`modelWaitMicroseconds`、`budgetSeconds`；`encode(request,value)` 失败清空输出。请求只接受实际非null Int／UInt／LongLong／ULongLong／Double、finite且 `0..86400`；不接Float QVariant、字符串、Bool或null，不从ETA／扫描时间补驻留。明确0保持0，来源中的缺失与显式0仍由调用方保留。

实现明确检查IEC559二进制Float32（radix2、24位有效数）及Double（radix2、53位有效数）、支持非正规数，并要求当前 `FE_TONEAREST`。上传值为原请求一次转Float32后再精确扩大为Double。正请求必须上传值至少 `FLT_EPSILON`，且 `float(upload * 1000000.0f)` 至少1并在转无符号整数后仍至少1；不静默夹紧或把正请求变成零。规定秒数域使该乘积低于2^38，先检查再截断转u64。零不产生假等待或保守微量。

所述模型的微秒等待为 `trunc(float(upload * 1000000.0f))`。`budgetSeconds` 取原请求、实际上传秒数与模型微秒／1e6的最大值。模型微秒小于2^38，转Double不会丢整数；正商至少1e−6，`fma(quotient,1e6,-modelUs)` 的残差不下溢，只有残差负（除法下舍）时 `nextafter(+infinity)`，精确整数秒不多加ULP。`addBudget(current,addition,total)` 仅收finite非负数，任一项0直接返回另一项；两正数且和finite时用TwoSum求精确残差，仅残差正（原sum下舍）才上取相邻Double。精确和1+1仍为2，上舍结果也不再扩大；任何溢出拒绝并清total。

这些界限依赖严格IEEE RN逐步舍入、渐进下溢及正确舍入的融合fma，不允许编译器将TwoSum重结合／以额外精度隐去残差。结构和标准fenv门不认证实际工具链、硬件flush模式或远端PX4算术环境；本批未运行算式验证。若部署不满足这些声明，不能将本模型当作已证明的实际调度上界。

该源级模型仅限本批已审阅的旋翼普通NAV分支：首次满足位置接受条件开始计时，yaw等待可能同时消耗该计时；不是“连续留在半径内这么久”、精确悬停、到点时间／绝对同步、真实调度延迟或实际飞行时长认证。不扩大TAKEOFF／LAND／HOLD等动作接受域，不开放StartPermit。真实机载回读Float32直接参与原规范比较，不增加时间容差。

Compiler／Draft保原请求；Builder原raw item保原p1请求及原Float32 delta，canonical NAV p1用同helper的上传值，归档／plan／source身份不以预算上界替换。Coordinator可用同helper的budget及addBudget保守汇总，但不得由驻留下界使未知航行时长变成已知。没有新增payload键、报告map、schema或权限标记。

仅源码编辑、直接静态阅读及普通文本核对；未执行helper、浮点模型／算法、parser、hash、编译、测试、仿真、上传、业务IO或PX4修改。交接待验证：缺省／显式0／负零、实际数型与伪数型、极小正值、epsilon及微秒截断边缘、Float32向上／向下舍入、86400、去返总预算、正和上取与溢出、原p1请求／canonical／RAW回读身份，以及yaw先等待不等于额外再驻留。

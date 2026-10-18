# ComponentParameterMetadata — 参数 v1 的完整候选构造

## 527 范围与接口

本模块专用于真实 `CompInfoParam` 参数目录消费，不修改通用 `FactMetaData::createFromJsonObject`、`createMapFromJsonArray/File`、字符串／数值转换器或它们的既有 caller。旧内部资源的 defines、String／Bool／Custom／64位／Double／ElapsedSeconds 等工厂行为不在这里收紧。接口不能作为文件签名、原始来源、实测性能、当前参数值、参数写权限或 StartPermit。

namespace `ComponentParameterMetadata`：

- `bool parse(const QJsonDocument&, QVector<Definition>&, QString& error)`：纯值整份校验，成功才一次赋完整有序 definitions；任何失败输出为空，并给固定错误码，不提供成功前缀／fallback Fact。
- `FactMetaData* create(const Definition&, QObject* parent)`：在正常 QGC 元数据上下文、parent 所在线程构造；再次核纯值 Definition，失败或最终认领时对象／parent销毁返回 nullptr。调用方仍必须在外层保原请求／来源／解析批次，并只在全部候选完成后提交，不能从一个非空指针推定整批成功。

Definition 公有字段：`name/type`；`shortDescription/longDescription/units/category/group`；`decimalPlaces`；typed QVariant `minimum/maximum/defaultValue`；`defaultSpecified/defaultUnknown`；double `increment` 与 `incrementSpecified`；`enumDescriptions/enumValues/bitmaskDescriptions/bitmaskValues`；`hasControl/qgcRebootRequired/rebootRequired/volatileValue/readOnly/writeOnly`；`unitsSpecified/categorySpecified/groupSpecified`。它是原生纯值，不是可从 QML 导入的授权票据。

minimum／maximum总有该数型合法范围，未声明时为完整类型边界；三个整数 signed 型用 QVariant Int、三个 unsigned 型用 UInt，Float用实际 Float。default缺失时两个标志都false且值invalid；显式Float null为specified+unknown且值invalid，不调用旧 setter、不给可重置的虚构默认值。非unknown默认则是相同数型。未给increment为0且specifiedfalse；未给decimalPlaces为-1。

## 文档与资源域

仅接纳实际 JSON object、真实 JSON数字 version=1、parameters array。只支持本机 PX4 `src/lib/parameters/px4params/jsonout.py` 确实生成的 v1，不猜最新在线schema版本或静默升级v2/v3。

完整树先验：根深度0，最大深度32；最多262144个值节点（含container，key另计字符）；所有object各最多128个key，每个key长度也最多128个UTF16单元；所有key及String值累计最多8388608个UTF16单元，每个String值最多65536。parameters最多16384项，values／CSV enum最多256项，bitmask最多32项。未知扩展字段允许但也完整纳入深度／节点／字符预算；超界整份拒绝、不截断。这不是对完整 JSON Schema draft、所有扩展字段业务语义或总运行时间的认证。

每项name为1–16个ASCII字母／数字／`.`／`-`／`_`，另外允许准确且最多一次的 `{n}`，计入16字符长度；其它braces／NUL／非ASCII／空名拒绝。重复原名称整份拒绝。模式匹配及已下载实际参数的名称／类型关系由实际消费者另核，本模块不编译任意正则或凭placeholder证明参数存在。

## 七种数型与兼容数字字符串

type为真实String，沿既有type名称的大小写不敏感比较，但只允许 Uint8／Int8／Uint16／Int16／Uint32／Int32／Float。未知名字不降Double，也不返回默认Uint32。

数值必须真实有限JSON数字；为保本地 v1 generator 对非Float／Int32字段可能直接输出原String的行为，也允许最多128字符的严格JSON十进制数字词法String：可负号、小数、十进制指数，不含首尾空白、前导加号、十六进制、非法前导零、宏或NaN／Infinity。字符串转换还须实际成功且有限，非零词法下溢到double零拒绝。bool／null／容器不是数字。

整数String在double转换前，先从原十进制mantissa、小数位数、指数及尾零数判数学整性，再核实际8／16／32位signed／unsigned边界和显式cast；不会把`"1.00000000000000001"`或`"2147483647.00000001"`的尾数先舍掉再当整数。JSON数字则按QJsonDocument中实际已解析的有限值核整性；没有 QVariant bool／字符串宽松coercion或小型整数回绕。Float允许正常float32舍入，但超范围、转换非finite或非零转换为float零拒绝。此接口不声称恢复JSON解析前已丢失的原始数值精度。

min／max存在就必须有效数值，拒null；先按原数值核min≤max，再形成typed范围。default与mobileDefault即使本平台不使用也都验证：非null须在最终原始与typed区间内；仅Float的显式null表示未知，不安装默认。移动端明确mobileDefault优先，其它平台用default。枚举的每个值也须在同一区间，不能先装枚举／默认后才把范围缩窄。

increment为独立double步进说明，必须有限且>0；Float参数的步进还须可表示为非零有限float，不能借double存储绕过Float overflow／下溢门。不先按参数整数类型把正小数截成0，不据此变更参数类型／实际值。decimalPlaces必须真实JSON数字整值0–25，不接受String／null／负值或小数。

## 枚举、bitmask与可选字段

`values`为完整array，每项object、真实String description和有效数值value；重复typed数值（包括Float舍入后相同）拒绝，坏末项也不能只保前缀或跳过一项。

旧`enumStrings`／`enumValues`必须成对真实String，按有界CSV处理且项数一致；只对各numeric token去CSV边界空白后按同一十进制词法读取，不进行defines宏展开。空／无效数值或重复值拒绝，不忽略旧可见enum。同项出现values与任一旧CSV字段视为多enum来源冲突，整份拒绝。

bitmask可与一份合法enum并存，两者均完整验证，不按旧工厂优先级忽略另一份。每项object、真实String description、真实JSON数字整值index；只用于整数型，unsigned允许0..bits−1，signed仅0..bits−2，不允许符号位。实际mask以`quint32(1) << validatedIndex`产生，绝不做负／越宽signed移位；重复bit拒绝。bit是位选择字，不是一个独立参数枚举值，因此按类型位宽而非声明min的整值下限判断。Float bitmask拒绝。

shortDesc／longDesc／units／category／group存在须真实String（仍受完整预算）；control／qgcRebootRequired／rebootRequired／volatile／readOnly／writeOnly存在须真实Bool。control缺省true，其余flag缺省false；category/group缺失保FactMetaData原缺省，显式空串仍为空串。volatile沿旧语义强制readOnly，即使显式readOnly=false；不凭这些描述性flag授予原生参数写权限。

## 构造与生命周期

create先冻结Definition值副本，避免caller容器在后续通知中被换掉。先创建parentless FactMetaData，装最终min/max，再装文本／完整枚举／bits／units及有效默认值和flags；最后才将实际对象交给原parent。设置units仍使用原内置／用户单位转换逻辑，不能为静态校验而冒充已启动的QGC上下文。跨该setter与最终setParent都核QPointer；失败只清实际未被采用候选。调用方的完整候选批次／source/operation尾核不能由本helper替代。

不会用通用factory的fallback指针、日志、默认min/max重置、被丢掉的enum项或QVariant隐式转换来宣称参数定义已完整接受。解析成功同样不证明参数已下载、当前值可用、固件／metadata可信或操作已批准。

## 未执行工程交接

本批只有apply_patch与直接静读，无JSON/helper/项目运行、编译、测试、下载或实际参数IO。需工程师覆盖：真实v1/七型及generator数字String；版本缺失／字符串／2／3拒绝；整数各边界／小数／溢出／bool/null、严格十进制／指数／非法词法；Float舍入／overflow／下溢、Float null default unknown而不可reset；min>max、默认和enum越最终范围；两平台mobileDefault；increment0／负／非finite及正小数、decimal0/25/坏域；values+bits同时保留、CSV保留／宏不展开／双来源冲突、重复与坏尾；UInt32bit31安全、signed符号位／负／宽度外拒绝；所有预算恰界／超界／未知扩展计数、16384参数与重复名／placeholder；完整候选安装和创建通知内销毁／换源、旧通用资源接受域不变。以上均未执行，不是兼容性或可信能力验收。

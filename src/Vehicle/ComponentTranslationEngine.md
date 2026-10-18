# ComponentTranslationEngine — 529 有界纯值翻译

本模块是实际 `ComponentInformationTranslation`／后台 worker 共用的文件和值引擎，不访问 Vehicle、Link、账号或 UI，不提供认证、参数写入或执行许可。新增的三个文件只定义引擎；网络下载、输入 artifact／lease 的 pin、后台线程双终结、来源代次以及译文晚回收由调用者负责，不能用译文路径替代原请求身份。

## 1. 接口和所有权

`ComponentTranslationEngine::resolveSummary(path, locale, cancelled, clock, url, error)` 从完整 summary 的同名 locale 对象读取 `url`，只接受明确小写 `http://`／`https://` 前缀、有效 host、无 userInfo 的绝对 URL。没有 locale 退选、网络请求或参数刷新。locale 最多 128 UTF-16 单元，URL 最多 4096 单元；URL 中空格、控制字符和 DEL 被拒绝。

`ComponentTranslationEngine::translate(jsonPath, tsPath, cancelled, clock, outputPath, error)` 读取完整 JSON 和已解压 TS，整份规则及尾部通过后才产生译文。压缩识别和 XZ 解码不是此引擎的职责。两个函数均借用调用者已经启动的单调 `QElapsedTimer`，只检查原 120 秒期限，绝不重置；同一后台阶段前面的解压时间仍计入。取消标志为原 `const std::atomic_bool&`，每个有界工作步骤和大块 Qt JSON 调用前后检查，不能中断一次已经进入的 Qt／文件系统调用。

输入是借用值，永不 move、删除或覆盖；`:/` Qt 资源路径可读，不要求资源有原生绝对 canonical 名，以保留实际 TranslationTest 的资源使用。原生文件要求存在、普通、可读、非 symlink，完整 64 KiB 分块读取，检查 EOF／错误／原大小，显式 close 后复核大小、canonical 名及 mtime。此复核不是对非合作外部进程的文件身份认证。

成功输出由本引擎 `QTemporaryFile` 原生独占生成，64 KiB 分块完整写入、flush、大小和 close 检查完成后才移交路径；调用者承担准确这一唯一文件的删除责任。失败时 `outputPath` 保持空，临时输出自动回收，不发布部分译文。没有 fsync、耐久、网络来源或物理设备真实性承诺。

## 2. 预算

每个输入及最终序列化输出最多 32 MiB。JSON 在 `fromJson` 前后检查期限，随后用显式工作栈检查完整树（包括未知业务字段及不参与翻译的内容）：最大深度 64、262144 个节点、每对象最多 65536 键、每键 128 UTF-16 单元、每字符串 65536 单元、键和值字符串累计 16 Mi UTF-16；输出树重建时再次核完整累计字符及单串上界。有限数值保原 JSON 数值；其它业务字段不因翻译规则而被裁剪。

XML 逐 token 读取，不使用无界 `readElementText`：最大深度 64、524288 token、65536 个 context 及最多 65536 条有效词典项；单文本、属性值及其它计费文本最多 65536 UTF-16，累计 8 Mi UTF-16。元素名、属性名／值、注释、PI target／data、DTD 和包括未知注释区在内的文本都计费。scalar 内容跨多个 token 累积仍有单串界限。

两函数每次调用另有 2000000 步工作上限。解释器调用深度最多 256，TS 上下文／输入路径最多 16384 UTF-16，全部替换累计最多 16 Mi UTF-16。规则处理、列表和通配符展开、引用、文本替换、树重建及各读取／写入块均计工作。重建时每个物理节点只有一个 parent，插入 parent 后立即释放该子节点的中间值，避免 values 向量再按每一层保留完整子树；这仅减少可避免的中间副本，不证明 Qt 内部峰值。预算超限、取消、坏尾、坏引用或类型不符整次失败，不发布成功前缀。这些是显式接受域与协作式检查，不是对 Qt 内部分配、一次 parse／serialize、系统调用、总内存或精确 CPU 时间的形式证明。

## 3. TS 接受域

接受一个 `TS` 根、无内部子集／外部标识／实体或 notation 声明的 `<!DOCTYPE TS>`，以及 XML 内建转义、CDATA 和普通文本。自定义／外部实体、异常 DTD、namespace、scalar 内嵌元素、numerus 复数条目及未知结构元素被整份拒绝。允许普通 `comment`、`extracomment`、`translatorcomment`、`location`、`oldsource`、`oldcomment`、`userdata`、`dependencies`／`dependency`、`defaultcodec` 注释元素，但它们仍计所有预算。

本元数据字典显式要求每个 context 一个非空 name、恰好一个 message 和一个 translation；并非任意 Qt TS 的完整实现。正常非空译文才入词典；`unfinished`／`obsolete`／`vanished` 或空译文不应用，未知 translation type 拒绝。有效重复 context 拒绝，不再最后一项覆盖；多 message context 也拒绝，因此不能把带旧多条 obsolete 历史的通用 Qt TS 文件称为本模块完全兼容。无可用词典或 XML 坏尾失败。`source` 文本只作为有界 TS 内容读取，不用它认证 JSON 原文或远端作者。

## 4. 规则和实际兼容

translation 顶层只接受 `items`、`$defs`、`$ref`，至少有 `items` 或 `$ref`。规则只接受 `list`、`items`、`translate`、`translate-global`、`$ref`；list body 另可有非空 `key`。所有定义（包括未用定义）都静态核验结构和引用存在，不靠 `toObject()`／`toArray()` 把坏值变成空成功。未知规则 extension 拒绝；未知业务 JSON 内容保留。静态核验不沿引用图禁止自环。

按实际旧消费次序执行 list → 本路径 translate → translate-global → items → ref。`items` 支持字面子键和 `*` 对象子项；未出现的数据字段可保持未翻译，存在却类型不符时失败。translate 字段为字符串或全字符串数组（数组上下文使用实际索引）；不将数字／bool／null 数组值偷偷替换成译文。global 路径为 `$globals/<field>/<当前字符串>`，内建 XML 转义会恢复正常 `<>` 等字符。两个操作依序作用，同字段的 global 查询会看到先前本路径的结果，未命中词典保原文。

`$ref` 严格为 `#/$defs/` 加一个 JSON-pointer token，支持 `~0`／`~1` 解码，坏转义、多 token 或不存在定义整份拒绝。顶层 ref 统一应用定义 rule，修正旧顶层把定义误当 items 映射而与 nested ref 不同的行为；本次静读的实际 PX4 schema 与 QGC fixture 都使用顶层 items。执行期以 `(实际 rule ID, 物理 JSON 节点 ID)` 的当前调用栈检测无数据进展循环；同一个 def 随 subgroups／list 进入更深真实数据合法。TS prefix 或可能重复的 list.key 不作为循环身份，因此重复 key 不会误杀生产性递归。

list 未配 key，或实际项缺该 key，沿旧语义使用数组索引；实际 key 为字符串时原值作 context，数值时用 Qt locale-independent compact JSON 数字（包括整数 index／value）形成非空十进制 context，bool／null／容器 key 拒绝。这修复旧 `QJsonValue::toString()` 对数值返回空串的问题。JSON parse 已不保存数值原始词法；`1.0` 可规范成 `1`，不猜多个 TS 别名、不声称所有外部 Float 枚举 context 已匹配或认证，未命中仍保原文。需用工程实际 Float TS 核对。

静读依据是仓库 `src/qgcunittest/TranslationTest.json`、对应 `TranslationTest_de_DE.ts`（自递归 subgroups、字符串数组、通配符、global 转义），以及本地 PX4 `translation.schema.json`、`parameter.translation.json`、`actuators.translation.json` 和 `jsonout.py`。后者确有 numeric bitmask index、enum value、actuator param-index；没有执行这些文件或修改本地 PX4。

## 5. 核验状态和边界

核验仅限直接源码／fixture 静读，非作者结论以本轮实际审阅记录为准；本文件不声称测试通过、性能实测、实时上界或全 TS／JSON schema 兼容。本批未运行解析器、helper、单元测试、编译、应用或真实下载。后续工程验证应覆盖合法 recursive-def 与重复列表键、无进展自环／互环、pointer 转义及坏 ref、numeric context（尤其 Float 词法）、TS 坏尾／实体／复数／多 message、所有预算边界、取消／原期限、qrc 输入、唯一输出和精确清理。译后 JSON 与 TS 不属于原 metadata CRC 认证内容；原始 artifact 的 CRC 关系不能沿用到译文，也不授予飞行或执行权限。

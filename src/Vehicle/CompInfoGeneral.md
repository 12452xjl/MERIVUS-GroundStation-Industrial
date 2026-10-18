# CompInfoGeneral：新批次目录退休与完整候选安装

## 526：防止失败的新 GENERAL 请求传播旧目录

新增 native 非 Q_INVOKABLE `void resetMetadata()`，供真实 GENERAL 新 batch 开始时调用。它清空本 General 的 `_supportedTypes`，并通过 `setUriMetaData("",0)`清掉自身原声明及所有旧 fallback／translation URI和CRC有效位；不会从旧 cache、既有参数定义或上批次目录重建支持类型。请求没有成功产出新目录时，后续 `setUris()` 对不在本次目录中的消费者仍完整清其 Uris。

`CompInfo::setUriMetaData` 对空 URI 现在重置整个 Uris，primary/fallback/translation与两个CRC有效位均退休。对非空 URI 保留原正常声明行为，真实CRC数值0也保持valid，不能把0当缺失。该修改不清参数 Fact／FactMetaData 的既有定义，也不解决下游参数完整 schema 或半装问题。

## setJson 全份候选，不发布成功前缀

每次 `setJson()` 先清旧 `_supportedTypes`，包括输入路径为空、读取／JSON失败、根不是object、坏版本或坏条目的所有早退分支。只有完整本地候选Map通过后才一次安装，空合法数组则安装空目录，不保旧数据。

- version 必须是真实JSON数字、有限且整值1；字符串、bool、null、其它整值或小数拒绝。
- metadataTypes 必须array且不超过128条；每条必须object。
- type 必须真实JSON数字，有限、非负整值，范围0至INT_MAX；重复type整份拒绝。私有Map按int保存，避免把未知整数强制转换为MAVLink非fixed enum。未知合法type可保目录，但不增加消费者，现有调用者仍只请求原known类型。
- primary uri 为必需String、非空、长度至4096且无NUL；uriFallback／translationUri／translationUriFallback缺失时为空，存在时也必须真实String、至4096且无NUL。不会将bool／数字／null折成空字符串。
- fileCrc必需；fileCrcFallback可缺失，存在时须通过同一校验。二者必须真实JSON数字、有限整值，范围0至uint32最大值，不能字符串／bool／负数／小数／超限或截断。CRC0合法。
- 保留旧静态接纳政策：每条必须primary URI非空并有CRC；不增加动态metadata。未知其它字段继续不受本批额外限制，不能用新增严格字段校验偷删既有扩展域。

目录只声明可尝试的URI及CRC，不认证二进制／固件、文件作者、硬件性能或实物构型。URI语法、传输、原始CRC／GENERAL兼容CRC、压缩预算及独立JSON快照由实际Manager／worker链另核，String及数值校验本身不授权下载或执行。fallback URI无CRC时仍保原有效位false，不擅自补证。

本批只修改上述目录／声明生命周期与解析源码，未修改CompInfoParam或FactMetaData factory。即使本目录整份安装，下游参数完整定义schema和部分安装问题仍待后续开发，不能称全部组件元数据产品已完成。

## 未执行工程场景

新GENERAL批次无回应／坏CRC／坏schema后目录与旧fallback不复活；空URI与正常CRC0区分；version1/1.0与坏type；0/128/129条、未知type／INT_MAX／重复type及坏末项全份拒绝；各URI真实String、空／4096／4097／NUL与可选缺失；CRC0/uint32最大值及bool／string／负／小数／溢出；其它扩展字段不误拒；`setUris()`向原known消费者传播本次空目录；真实Manager新batch reset与异步终态归属。以上均未执行。

只进行了源码和直接静态阅读，未编译、测试、启动应用／解析器／worker、下载、CRC／解压／JSON执行或实际缓存IO；不是运行、兼容性或硬件真实性验收。

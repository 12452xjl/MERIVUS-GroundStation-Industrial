# ParameterImportTransport — 534 原生参数导入专用写入口

本模块只在现 `ParameterManager` 导入事务与实际 `Vehicle` 间增加 private／friend 桥，不是 QML、公开 Map、一般 PARAM_SET 或新的网络服务。普通 Fact 单项编辑、旧 sender 和固定 `_HASH_CHECK` 桥的行为不变；不增加参数协议、PX4 修改、自动刷新或飞行许可。

## 冻结的 native 接口

Vehicle 私有 `ParameterImportWriteLedger{quint64 entered=0;}` 由同一真实导入行的 shared pointer 跨重试保留。`_parameterImportEncoding(componentId, bool& numericFloat) const` 给出当前实际 plugin 的编码模式；失败不能当 bytewise 成功。`_sendParameterImport(LinkInterface* original, mavlink_message_t logical, const mavlink_param_set_t& expectedWire, std::function<bool()> sourceFence, const std::shared_ptr<ParameterImportWriteLedger>& ledger)` 仅由 friend PM 调用。

caller 持有原 PRIMARY 的 strong owner，冻结其 weak 控制块及实际指针，不能在发送时取后来 PRIMARY。`sourceFence` 必须核原 run／行／尝试、取消、实际 PM／Vehicle／plugin／protocol／目录、原来源及编码模式；不是 caller 提供 Bool 或公开摘要的授权。桥进场先按字节复制 expectedWire 并独立持住 ledger，即使回调退休原 PM 行也不会借失效引用。

桥只接受目标 system／component 1..255、原 `PARAM_SET` 长度、1..16 可打印非空格 ASCII 名，允许满16字节；较短名字后的字节须全0，拒 `_HASH_CHECK`。实际写域只有 UINT8／INT8／UINT16／INT16／UINT32／INT32／REAL32 七种，未知、64-bit、REAL64 不落旧 INT32 fallback。窄整数逻辑 union 的未用高位必须为0；不能借任意残高位。整数的 float 别名可能具有 NaN 位模式，只按字节读取而不把它当浮点算术值；REAL32 自身须有限并保留正负零。

## APM 明确例外而非任意改值

`APMFirmwarePlugin::parameterSetEncodingMode(systemId,componentId,bool&) const` 是同 owner GUI 线程的原生非 invokable 查询，只用 constFind 读取实际 `_ardupilotComponentMap`，不插入条目。仅该组件记录为 ArduPilot 才返回 numeric-float=true，整机 apmFirmware 不能替非 APM 组件决定模式；这个本机映射不是组件真实性认证。

Vehicle 只在实际编入 APM 的构建中对 actual APM 对象查询；qmake／CMake 的 `MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION` 须随实际编入状态一致。无此实现的 stripped／custom plugin 不猜 APM 布局，只允许其保持 bytewise 参数值；未知改值不会默认通过。

caller 先从原 zero-initialized logical union 构造 expectedWire。仅 numeric-float 模式将整数转为 float；必须在 Double 中证明转换后精确等于原整数，不能先窄 cast 回整数判断。16777217、会舍入的 INT32_MAX／UINT32_MAX 等须整批预检拒绝，桥也独立拒绝；不是悄悄四舍五入。REAL32 不再变换，-0 仍按原位模式比较。

桥按当前明确模式独立推导 expectedWire 并核 caller 值；每次重试仍把最初 logical 交实际 plugin 一次，不能把上次已调整结果再调整。plugin 后原 message id／长度／发件 system与component、目标／name／type及精确期望 value bits 全部吻合才继续。允许 APM 既有重编码产生新的包序号／封装，不把合法 numeric 转换当任意 payload 修改许可。模式中途变化则拒绝，不重新建立导入依据。

## 最后写入口与不可回滚事实

实际 Vehicle owner 线程、早期关闭门、原 plugin、原行 ledger 计数、目标、编码及 disarmed／not-flying／initial-connect 门均在 sourceFence 前后再核；原 Link `isConnected()` 虚调用后与 plugin 调整后同样核验。Link QObject 可有自己的工作线程，不强迫它等于 GUI；caller 的原 strong owner保障整个调用期间实际 Link 存活。

serialize 后最后一次 current 通过，且 ledger 未耗尽、仍等于入场计数，随即在没有通知／virtual／helper 的区间 `++entered` 再调用原 `writeBytesThreadSafe`。进入 writer 即为本地可能已排入发送的保留事实；同步回包／取消／销毁已经可以看到此计数。晚发生的 source 退休、取消或 `messagesSentChanged` 不把 true 改 false、不减计数；计数耗尽拒新发送，不能回绕。

这只证明调用原 Link writer 入口。该 writer 的 `_invokeWriteBytes`、驱动、网络送达、机载接受或持久保存不在此证明中；不是 ACK、因果、认证、整批远端原子提交或已执行结果。PM 须分别保存 partial、重试、报告匹配与终态，取消不能称撤回已排入字节，也不能清后来普通编辑的等待项。

## 原始入站票据

Vehicle 在 `_mavlinkMessageReceived` 冻结 originalMessage 后、任何 prepare／probe／公共通知／plugin 之前，从实际原 PM child 调 private `_captureParameterImportReceipt(originalMessage,originalLink)`。该方法须是纯内存 capture，只能冻结已存在 run／行及其已进入 writer 的 ledger；不得在这里创建新 run、调用 Link virtual、锁 weak owner或发通知。

同一票据随原 original RAW 栈保留，后面只有同实际 PM 的 adjusted `mavlinkMessageReceived` 才收到它。普通旧两参 caller 保持兼容，但空票据不能给导入记匹配报告；上游回调中新建 run 不能借旧包。原 RAW value bits 同时用于核 APM numeric-float 真实表示，不能只凭调整后相同值追认来源。报告仍无协议 nonce，不能据此声称已证明远端因果。

## 未执行边界

仅 apply_patch 和直接静读，未执行 bridge、parser、worker、任何参数发送／接收、项目、helper、编译或测试。工程验证至少需覆盖七型与十型文件区分、窄型残高位、16字节名字／坏尾／HASH拒绝、signed bounds／负零、APM每组件模式／不可精确整数／重复adjust拒绝、PRIMARY与mode ABA、isConnected／plugin通知换源或删除、最终write中同步取消／销毁／回包、ledger耗尽／嵌套写、最早旧包不得认领后来新run，以及普通单项编辑／HASH行为不变。不能称运行或跨平台通过。

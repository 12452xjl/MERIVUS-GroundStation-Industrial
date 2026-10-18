# ParameterManager：531 typed 参数缓存与保留的530离线初始化

## 534 当前：独立参数导入事务（源码静态收口）

参数编辑器的完整人工确认现在接入独立原生ImportRun，固定原typed值及原PRIMARY、独立定时预算／重试，不把导入项塞进旧waitingWrite表。接纳／部分writer事实／同源匹配报告／超时／准确取消分开；普通参数流仍负责真实Fact更新，导入不乐观改值。具体输入、来源、竞合、回包票据和未验证范围见[ParameterImportTransaction](ParameterImportTransaction.md)，最后writer边界见[ParameterImportTransport](../Vehicle/ParameterImportTransport.md)。

本批源码实现及限定静态交叉审阅完成；没有运行定时器、发送参数、编译或测试。下方531缓存及530离线说明保留，缓存／离线数据不进入导入匹配报告。


## 531 实际入口与完整 v3 文件

PX4 原 `PARAM_VALUE/_HASH_CHECK` 在 PM 接收入口解出实际原帧后、任何 Link
虚调、日志或 observation prepare 之前进入私有缓存尝试。只接受 UINT32 原值位、
index=65535、count1..16384，且原接收 Link 必须就是原 PRIMARY 的实际 QObject
与 weak owner。普通 helper／文件回放不能触发这条入口；APM 普通同名参数保持原流程。
这只约束 PM 的实际入口，不认证更早 Vehicle plugin/core 处理前的原始报文、远端
身份、参数真实性或任务／执行权限，也不新增协议或泛发送接口。

`parameterCacheFile` 现返回原缓存目录的 `<vehicle>_<component>.v3`。旧 `.v2`
不读取、不删除，也不降级到 QVariant/QDataStream 恢复。输入借用普通非symlink
文件；复用已有4 MiB、64 KiB分块、size/canonical路径/mtime前后值、正常EOF、
错误与close完整检查。全部bytes先由 [ParameterCacheFile](ParameterCacheFile.md)
纯值decode通过，文档ID和完整行数必须等于本次实际HASH输入，才可能创建/修改Fact。
坏尾、未知类型、乱序、重复、资源越限、CRC不符或来源变化均作完整miss，继续原机载
参数传输，不用文件前缀放行。没有任何文件对象身份、签名或hash真实性认证。

v3显式保存10种numeric的真实typed值。IEEE Float/Double包括NaN、Inf、±0按实际位
保留和比较；只在平台／Qt能完整复现原bytes时使用。PX4快载CRC域另严格限实际INT32／
FLOAT的ASCII name（无NUL）与LE4原值字节，state0累积，绝不用QVariant.constData
推断内存布局。DEBUG miss比较也同时核实际wire type与codec typed value，而非盲memcmp。
CRC不是签名，也不因相同CRC证明两个使用集合或volatile顺序相同。

## 531 来源、重入与真实模型消费

每次尝试先冻结原PM／Vehicle／actual ComponentInformationManager、原生早期已建的
LinkManager／FirmwarePlugin／MAVLinkProtocol QPointer pins，并与原raw地址匹配；
不从可能悬空的provider raw字段重新构造QQ。再冻结PRIMARY弱owner/QQ、id/defaultcomponent、
firmware/type、版本来源epoch与实际七版本数值。两个版本信号的直接连接只置共享atomic；
两监听就位后的metadata/插件/值通知ABA不可复活候选。与530相同，第一connectNotify
内另一未监听信号的隐藏ABA不由本地监听证明，不声称跨线程或全局setter代次。

独立非回绕activity serial与load/write busy防同对象重入。实际PARAM、显式refresh、
本地写、文件导入/FTP/reset、source变化永久退休原尝试。正常ready/missing通知会按
原合同推进live observation epoch，但不据此把本次仍同源的缓存完成误当换源。
元数据使用原CompInfoParam，首次成功查询可正常安装lazy fallback；随后固定实际目录
revision，并在每次外调和最终复核原provider/来源。未知volatile不当false。

所有行先完成当前provider的volatile查询与CRC核验。全volatile无预填价值，作miss。
所有缓存行逐行只安装模型，不消除任何index/name/queue等待。没有volatile时，先整表
复核实际Fact/metadata/parent/thread/type/name/raw位值、完整组件key集合/count、目录
revision与无pending write；再核当前等待index均在原count域内，才在无外调区一次记入
该完整组件的接收信用、消费其候选索引与name等待。外国name等待不清，别的组件仍需的
同号queue索引不清；重算真实等待计数，其他等待尚存时保留原timer。然后重新核完整
fence及索引空，再更新真实进度、进入受控原ready流程；plugin precheck及ready通知后
再核完整表。factory/FactAdded/值通知中途失败时，任何缓存行都未提前信用，真实流和
原index retry仍能恢复缺行，不能用先清后回滚伪造这项保证。最后仅沿原
`_sendParameterCacheHashAck`固定payload与原Link最终写入围栏发送HASH握手。

只要存在任一volatile，非volatile行最多作只读模型预填：不安装volatile文件值、不改
任何current read-index/read-name/queue项，不用旧used-index目录当本次实际接收；不
触发ready，也绝不发送HASH ACK。初始化等待仍是完整原count并保既有真实已读索引。
旧volatile名字/排序变化即使CRC与count都相同，仍须让正常bulk/原index retry逐包核
当前集合；真正数据随后正常覆盖预填。此时界面已有值不等于机载流已完整核验。

缓存回放只带本次原私有fence，不能共享布尔冒认nested真实帧；不再有逐行信任索引参数。
自身不写回缓存、不填批量重试队列、不提前调用完成。真实nested接收继续原处理，先
永久退休原serial；它可以在旧尝试已退休后完成自己的正常加载，不是让旧cache尾重基
或恢复ACK。尚活cache busy内的无fence完成仍被阻断。每个旧尾失效即止，不回滚已
发布Fact、已通知的原状态或已经进入原writer的HASH握手事实；也不制造live参数观察。
缓存专用完成在precheck返回且完整fence仍有效之后、ready通知之前才置initialComplete；
普通真实完成仍保原早置防重入。precheck内ready getter可能已true，不等于ready通知
已经发出；该已赋flag不回滚，initial仍false使后续真实bulk可补完整正常通知。ready
listener内变源也不谎称后续missing通知或HASH ACK已经发生。

新Fact先parentless再以QQ受控设原PM parent，原metadata仍实际Fact-owned。连接/通知
后核原QObject/来源；清理只删本次尚未发布的准确新Fact。缓存期actual children一次
建立原QQ pins，新Fact在实际发布点更新，避免逐行重新从raw map构造QQ或反复全树扫描。
元数据同样先按该实际Fact.children中的原地址核得QQ；existing路径、factory安装、
FactAdded及连接通知后，进入_container前后均复核实际owned metadata，坏raw字段不
被重新包装成QPointer后再解引用。新factory没有合格元数据时不发布该新行。
原750ms/500ms假进度动画移除，仅保真实等待表进度，不留旧timer覆盖新加载进度。

## 531 原子写入与未执行范围

写入只在真实完整组件map/count一致、read-index/read-name及失败索引为空、无pending
write、非缓存回放时进行。不要求ready/initial已true：原实际调用位于末行真实值安装后、
正常完成通知前。先冻结原来源和完整actual Fact/type/value集合，再normalize/encode；
每次64 KiB写入及flush/commit前后重核原集合。QSaveFile关闭direct-write fallback，
短写/失败不覆盖原文件。commit已成功的原子替换不能因随后失源被谎称回滚，但不证明
fsync、durable、实机新鲜度、可信构型或执行放行。缓存读写仍是有界同步工作，不称已
完成全机队后台调度或Qtparser抢占。

本批只源码修改与直接静读，未执行codec/文件IO/Qt对象初始化/业务、编译或测试。
工程须覆盖：完整10型/IEEE原bits与小unsigned提升、v2留存/坏v3整miss、HASH原type/index/
count/PRIMARY、metadata未知/换目录/七版本ABA、任volatile只预填不清索引不ACK、volatile
used集合同count换名移位、正常nested最后一帧独立完成、Fact/metadata/ChildAdded通知
销毁或改值、pending write、QSaveFile短写/flush/commit及提交后失源，正常PX4/APM加载。

## 530 保留的构造期离线实现（以下范围描述仅指530）

## 当前用途与范围

只关闭 `_loadOfflineEditingParams` 的真实构造期路径：原先虽然转换了文件值，却未
写入 Fact，导致非零模板值保持构造 Int0。本次完整 typed 文件候选通过后，实际
调用 `_containerSetRawValue` 安装每个值，再发布整张离线表。不是新增公开 reload、
参数写入、离线授权、仿真机载回执或后台全机队调度。

在线 QDataStream 参数缓存的读写、CRC流程、重试、FTP参数包、参数文件上传以及
`_sendParameterCacheHashAck` 固定源/实际写入围栏均不在本次修改范围。
此为530当时的范围；531已在上节独立接入v3完整typed decoder/原子写，未改FTP包解析。

## 输入整份候选

保留实际 FirmwarePlugin.offlineEditingParamFile 虚入口及 bundled qrc 路径。主输入
仅借用，不删除/移动/写入；普通文件和 qrc 均须可读普通非 symlink、非空且≤4 MiB。
64 KiB读取逐块检查负 read/预算、正常 EOF、实际 pos/字节数、size/canonical路径/
mtime前后值、QFile error与close；坏尾/短读/变化不作为成功 prefix。
这不是文件对象身份、签名或敌对同路径替换认证。

完整字节交 [OfflineParameterFile](OfflineParameterFile.md) 纯值 decode，成功之前不
创建任何候选 Fact。codec 整份约束包括5列、统一来源system声明、component/name
唯一、全部10种numeric MAV类型、范围与有限浮点/下溢/负零、资源预算。`#`注释按
原文件事实先跳过，包含当前Plane模板注释内NUL；有效字段不因此允许NUL。
sourceSystemId只是文件声称，绝不改变当前 Vehicle.id 或变成真实系统身份。

## 构造窗口和原来源

仅原构造期 pristine 槽可提交：live map/count为空、total=0、ready/initial=false、
读写等待表为空；本批不提供对已有离线表的覆盖/重载。私有 busy 防同对象重入。
ParameterManager 真正有 Vehicle parent，且原offline/source、FirmwarePlugin、
firmware/type/id/defaultcomponent、版本来源epoch、实际ComponentInformationManager，
自身map/count/加载状态/等待和debug状态均在虚调用之前冻结并逐尾校验。
另独立冻结实际major/minor/patch/versionType与customMajor/customMinor/customPatch
七个版本数值。公开setFirmwareVersion并不一定推进该来源epoch，而原APM metadata
选择真正读取版本数值，因此两类版本依据都必须在每个尾保持，不能互相替代。
本次加载还用独立共享atomic标志记录firmwareVersionChanged／firmwareCustomVersionChanged，
直接连接只置该标志、不操作QObject；退出准确断开。两连接就位后的插件／metadata
虚调及值通知中，版本A→B→A也不能恢复本次候选。不捕获会在退出后悬空的栈引用。
连接建立期间逐尾仍核原数值／来源及已接入的通知；第一条connectNotify内另一条
尚未监听信号的隐蔽ABA不由本地监听证明，不能冒称全局setter代次或跨线程加载支持。
观察epoch/revision/updateToken同样被冻结；只有本次已知无通知的初始失效函数可
更新预期观察状态，不跨其它外调重基新源。

合法构造时 `Vehicle::parameterManager()` 还未赋此新对象，不将该属性作为此处
资格门；也不要求PRIMARY、initial-connect complete、在线身份或地面/飞行状态。
close/retirement、实际对象销毁、来源/表/状态变化或观测更新均使本次停止。

每component通过真实Manager取得实际CompInfoParam。首次合格生成Fact-owned metadata
后才冻结该provider目录revision（允许首次正常lazy fallback建目录）；后续任何行
以及最终全表复核都要求相同revision，不能后行换目录仍发布早行旧元数据。
此处不是将metadata声明认证为固件、可信性能或当前机载值。

## 暂存、真实值与一次提交

新Fact先parentless构造并立即QPointer pin，再受控setParent到原ParameterManager；
ChildAdded后核原owner/Fact/parent/source，供元数据factory要求的真实parent关系。
所有Fact仍未插入live map。metadata必须实际归属该Fact、同name/type/thread；
metadata生成/安装及`_containerSetRawValue`的每个通知边界之后，重新核原来源、
actual Fact/metadata、name/type/component、目录revision和原/目标值。

只用 `_containerSetRawValue` 安装codec的canonical QVariant，不用setRawValue或
forceSetRawValue。配合本批Fact精确元型/浮点零符号比较，UInt/Float/Double等零和
负零不能被初始Int0吞掉；值匹配按codec严格型与值，不靠bool/string强转。
不连 `_containerRawValueChanged` 到在线写路径，不发PARAM_SET，也不制造live
parameterObservation或收到时间。默认元数据/实际fallback仍由原factory负责。

后行回调可能破坏先前暂存对象，所以最后再遍历完整候选核所有实际Fact/metadata/
值和所有provider目录代次。任何失败仅尽力删除本批未发布、仍属原owner或无parent的
准确新Fact；不清原map、不删除外国parent、不回滚别人的新状态，不将部分候选标ready。

默认component优先保原合法值且候选确有该component，否则选1，否则唯一component；
多个非1且无原有效选择时整份拒绝，不用行序/最后一行猜primary。
全部检查成功后，在无外调区一次提交map、每component真实count、total、默认component、
ready/initial以及debug缓存清理；Vehicle离线default setter真分支只是原生字段赋值。
不逐行改default，不调用在线`_checkInitialLoadComplete`/ready precheck/发送逻辑。
这是构造期发布，不另造运行/已验证/已飞行状态，所有在线许可门保持。

## 未执行边界

仅源码apply_patch与直接静读，未执行文件读取/解析/Qt对象初始化、业务IO、项目、
helper、编译、测试或仿真。工程需覆盖：PX4/APM实际qrc非零值、Plane NUL注释，
10型上下界/typed零/±0/float正常舍入、坏尾/重复/异源/溢出整拒、4 MiB与短读写，
默认component选择/歧义，多component计数，plugin虚调/ChildAdded/metadata/valueChanged
内销毁或换源、后行改变早行候选、目录revision变化、原constructor未赋PM属性、
失败无prefix/ready、无容器写信号/在线参数写/假fresh观察，以及旧在线加载/缓存回归。

# ComponentMetadataArtifactWorker — 有界元数据字节工件

## 532 当前：进程预算准入

新增原生 `bool schedule()`，隐藏正常接口中的 `QThread::start`；同一 worker 只尝试一次。实际所有生产入口共用 [ComponentMetadataWorkScheduler](ComponentMetadataWorkScheduler.md) 的 2 running／32 queued FIFO，排队期限120秒。schedule=false从未受理，无自身终态，caller清准确未启动对象；true后 queued取消／到期独立发 `neverStarted(requestId,reason)` 一次并 deleteLater，不伪造 resultReady／finished，也不作坏cache依据。`requestCancel`仍只设原子位，调度器巡检及获槽前核验；running只真实finished归还槽，原result+finished双尾消费工件不变。构造器最先安装只写shared atomic的finished latch，不能靠later QObject销毁猜线程完成。以下旧start说明现指受控schedule成功进入running，不保留取消也直接start旁路。未运行或编译测试。

## 528：原下载输入由实际 worker 持有

构造器新增可选末参 `const QGCOwnedDownloadFilePtr& sourceOwner={}`，由 worker 的 const shared 成员自行持有，而不是仅依赖 Manager、结果回调或路径 QString。给出 owner 时 sourceFile 必须与其准确 path 相同；原缓存命中传空 owner，不能领养／清理 cache。下载原件不与 worker 自建 raw／JSON／可转交第三 cache snapshot 混用。

即使 Manager 析构、接收连接被断或原请求被退休，parentless worker 在实际 run 退出前仍保输入；既有独立 finished→deleteLater 连接在 start 前建立。实际 result 与 finished 两尾、取消及不装部分工件规则保持。最后实际 owner 回收才尽力移除其独创原件；不是持路径即认证字节、无本地恶意篡改或崩溃耐久保证。见[输入所有权](../QGCOwnedDownloadFile.md)。以下为526历史合同，新传输件本身另受 HTTP／FTP 32MiB预算。

## 526 范围与实际消费者

这是现 ComponentInformationManager 下载／缓存流程的原生一次性后台字节检查，不是新的飞行、参数写入或能力授权服务。原实际 Manager 冻结本次文件、URI后缀及 CRC 声明，等待同一 Worker 的 `resultReady` 与实际 `QThread::finished` 两种终结，才调用 `artifact()`；之后持有 shared pointer，供原 CompInfo 解析／翻译链消费其独立 JSON 路径。不能把 public Map、缓存名字或 hash 当成实际接纳来源。

Worker 不持 Vehicle、Manager、FTP、网络或授权 QObject，只复制 QString、Bool、quint32 和原请求ID。构造为 parentless QThread，不在 GUI 等待／terminate／删除运行线程；实际拥有方须保原 worker/请求/来源代次与双终结。Manager消失时仍须安排 actual finished 后删除 Worker；未取走的工件随 Worker 最后引用释放清理。本文不把线程结果信号解释成线程已经退出。

## 固定 API 与共享只读结果

构造：`ComponentMetadataArtifactWorker(requestId, sourceFile, compressed, hasExpectedCrc, expectedCrc, allowDecodedCrc=false, makeCacheCopy=false)`。`requestCancel()` 只设原子取消；`resultReady(QString requestId, bool ok, QString error)` 仅报本次结果；`artifact() const` 在 mutex 下返回 `ComponentMetadataArtifactPtr = std::shared_ptr<const ComponentMetadataArtifact>`，失败没有 usable artifact。

原生 struct 公开十个 const 字段：

| 字段 | 原生类型与含义 |
| --- | --- |
| rawSnapshotPath | QString，本次独立原始字节临时快照 |
| jsonSnapshotPath | QString，本次独立解压后／原样 JSON 字节临时快照 |
| cacheCopyPath | QString，可选第三份原始字节副本；未请求时为非 null 空串 |
| rawBytes / jsonBytes | qint64，实际完整读取／写入的字节数，各0–33554432 |
| rawCrc32 / jsonCrc32 | quint32，QGC::crc32 显式 state=0、无 final xor |
| rawSha256 / jsonSha256 | QString，各自完整快照字节 SHA256，小写64位hex |
| crcByteDomain | QString，只有 transport / decoded-px4-general / not-compared |

private constructor 只允许本 Worker 安装实际创建的唯一临时文件路径；删除复制与赋值。shared result 不跨线程共享 QFile/QTemporaryFile。只有创建成功且完成全检查后，才把局部 QTemporaryFile 的 autoRemove 责任交给该 artifact；失败路径由局部 RAII 清理。最后 artifact 析构只尝试删除它本次创建的两份（可选三份）唯一路径，绝不移除原 sourceFile 或缓存移动后的目的路径。临时文件清理是尽力本地清理，不是已验证删除／耐久性认证，崩溃或操作系统拒绝仍需工程验证。

Manager **不得**把 immutable rawSnapshotPath/jsonSnapshotPath 交给旧 Cache::insert、rename/remove 或其它消费后删除逻辑。只有 cacheCopyPath 可转交旧 insert；其名字可能已被移动或删除，因此不是后续可读输入。Manager继续消费原 jsonSnapshotPath，不能改用旧 insert 返回的路径；未来 cache 命中也必须重新创建工件、重核字节。makeCacheCopy=true 要求 hasExpectedCrc=true，实际调用只在下载成功需缓存时启用，缓存命中不再造第三份。

## 完整有界流程与 CRC 比较域

1. 请求ID为非 null、非空、≤128 UTF16 字符；sourceFile 为非 null、非空、≤4096且无NUL/孤立surrogate 的本地文件名。读时须是实际绝对、可读普通非 symlink 文件。原始快照由 QTemporaryFile 唯一创建，不修改或移动输入；不使用来源文件名充当临时输出。读取／复制／SHA／CRC均64 KiB流式，明确拒负read、非EOF零read、超过32 MiB、大小或metadata变化、短write、flush／close未确认。
2. 原始完整写入后再次读取 raw snapshot，bytes/CRC/SHA必须与复制流相同。plain输入也另复制到独立JSON快照并读回比对，不把两个公开路径指向同一个文件。Worker自身不做JSON语法／schema解析；本批General另行整份校验目录，参数等具体消费者仍独立负责完整定义解析。字节检查成功不等于参数定义或一般metadata schema已接受。
3. compressed 由 Manager 原冻结URI后缀判定，不能从 `.cache` 或新临时文件后缀猜。新 bounded XZ overload 从已固定 raw snapshot 解压到本 Worker 已创建的独立 JSON 文件，原始32 MiB、输出32 MiB、字典最多64 MiB。64 MiB上界对应本机 PX4 general生成器 preset=9 的兼容性需要；不是按实际输出32 MiB推断字典也≤32 MiB。实际内存还包含XZ状态及每次64 KiB输入／输出buffer，不声称整个线程内存只有字典大小。
4. XZ多次调用模式采用 XZ_DYNALLOC 和明确字典上限；signed read在转size_t之前检查，逐次累计input/output预算。拒 XZ_UNSUPPORTED_CHECK、options/format/data/buffer/memory错误、超限字典、取消、截断、拼接stream与任何尾随字节（包括padding），不能在 STREAM_END 后忽略剩余输入。完整EOF、原大小、输出大小、flush/close均通过才成功；部分JSON不作为artifact发布，随后由本 Worker RAII清理。
5. CRC优先比原始transport字节：hasExpectedCrc且 rawCrc32==expectedCrc 时固定 `transport`。已知raw不匹配且未允许decoded兼容（或未压缩）的输入在解压／JSON复制之前立即CRC错拒。只有实际 Manager 明确给 allowDecodedCrc=true 的 PX4 GENERAL，才允许压缩输入的原始不匹配但 jsonCrc32==expectedCrc，必须标 `decoded-px4-general`。本机 general构建链对未压缩 component_general.json 计算CRC、URI提供.json.xz，参数子文件则对压缩文件计算；这是明确兼容例外，不能给全部压缩文件开 decoded 回退或冒称原始字节CRC匹配。两者都匹配时仍优先transport。
6. 无CRC声明时 `not-compared`，仍可供旧只读显示／解析，不伪装 CRC匹配、可信构型、真实性能、签名或授权。CRC32依据现 QGC 累积函数 state0/no final xor，与传输帧的X25 CRC及XZ容器内部check分别处理；SHA只是各自文件字节指纹。所有CRC不匹配整次失败，无合法前缀工件。
7. 如需缓存，第三个唯一临时文件从固定 raw snapshot 另复制并完整读回，bytes/CRC/SHA必须仍相同；同样有大小、取消、短write、flush/close门。每份最多32 MiB，三份快照最大合计96 MiB临时磁盘；没有第四份常驻Map或整份QML数据。实际 Manager 每机只有一项工件作业，Worker自身不创建后台队列或重试。
8. 最后短mutex只检查取消并安装shared pointer，不跨文件读写。取消在安装前使结果失败且不安装；安装后晚到取消不把已经完成的字节事实改成未做，Manager仍以原本业务取消／来源门决定是否消费。resultReady之后局部清理完成、run退出，才有actual finished。引用存续期间快照只供读取，译文另有其自己的生命周期。

## 新 bounded overload 与旧行为隔离

`QGCLZMA::inflateLZMAFile(input, output, maximumInputBytes, maximumOutputBytes, maximumDictionaryBytes, const std::atomic_bool&, QString* error=nullptr)` 是新增重载。input/output上限可为1字节至32 MiB，dict上限可为1字节至64 MiB；实际XZ字典超过所给上限即拒。output须调用方已创建的普通非symlink文件，不能与input为同canonical路径。失败保留其部分输出给调用方RAII，helper不删除任何路径。错误码是固定字符串，不把原路径、URI凭据或内容拼入错误。

旧两参数函数的正文和行为未改：它仍是原调用域，不能把本新重载的严格边界声称为旧所有下载／翻译已统一实施。新函数支持的是XZ容器，不据`.lzma`扩展承诺另一种裸LZMA格式。metadata URI声明不认证主机、缓存或文件；本地SHA/CRC成功也不证明全部FTP响应来自原PRIMARY、未被插件调整、网络证书已认证、文件签名、真实性能或任何StartPermit。

## 未执行与工程师待验证

本批只apply_patch与直接静读，没有运行Worker、下载／缓存／解压／CRC／SHA／文件IO、项目/helper/parser、编译或测试。需工程师验证：合法plain、xz CRC32/CRC64、RAW CRC与PX4 general decoded例外、两者同匹配优先transport、错误CRC、不带CRC仍只旧显示；URI后缀与.cache分离；0/32 MiB/超界、负read、短write、flush/close失败；xz截断、额外尾、拼接、unsupported check/options、>64 MiB字典及preset9；输出炸弹32 MiB及时拒、取消各阶段与安装后晚取消；三unique路径与source不变、Cache::insert移动第三副本不影响raw/json、Manager销毁/双终结/最后shared引用清理、真实Qt/SDK资源和线程行为。上述均未执行，不能称运行通过或可信能力完成。

协议依据：[MAVLink CRC](https://mavlink.io/en/guide/crc.html)；本地实现 `src/QGC.cc::crc32`，本地生成器与流声明只支持上述受限PX4 GENERAL兼容分支，不用于证明实机固件当前内容或签名。

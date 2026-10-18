# FTPManager — 528 独立原生下载输入

本批在实际 MAVLink FTP 下载器内增加 native-only `downloadUnique`／`cancelUnique`，供组件元数据使用；不是新网络服务、公开可导入的文件所有权 Map、远端身份认证或参数／飞行权限。源码仅直接静读，未运行协议、下载、文件业务 IO、解析器、编译或测试。

## 新接口与旧 caller

`bool downloadUnique(uint8_t component, const QString& uri, const QString& requestId, const QGCOwnedDownloadFilePtr& output)`：caller 先调用 [QGCOwnedDownloadFile::create](../QGCOwnedDownloadFile.h)，每次原生创建一个新的单次传输 lease，传入非空有界 requestId。本接口非 Q_INVOKABLE，不接受任意本地 path、远端 basename 或外部 Map 作输出所有权。成功／错误／取消均只发 `uniqueDownloadComplete(requestId, output.path(), error)`；拒绝受理返回 false，不发本次完成信号。true 表示本次已受理，不表示下载／CRC／解析成功；允许在返回 true 之前同步终结。

unique 不发旧 `downloadComplete` 或 `commandProgress`，避免仍连接旧共享信号的相机／参数下载 caller 把元数据输入当成自己的结果。旧 `download(component,uri,directory,filename,checksize)` 签名、basename／显式 filename、legacy 完成信号及 APM 参数文件 `checksize=false` 行为保留；旧 caller 不必迁移。两种模式共享一个 busy 槽，不能彼此覆盖。旧无身份 `cancel()` 仅针对 legacy，不能取消 unique；新取消必须原 requestId 精确匹配。

URI 先在局部解析完成才安装状态机，坏 URI／忙／错误输出不遗留 busy 或伪发完成。legacy 保原 `mftp://`／`[;comp=N]` 远端路径约定、默认 component 及旧解析转换域。unique 另有独立精确解析：可选 component 前缀只准一次且必须在真正前缀位置，整数在 cast 前须 1..255，重复／非前缀／坏 reserved marker 拒绝而不全局 rewrite；也拒空远端路径／控制字符或超单个 FTP request data 的 UTF-8 路径，不把被截断／回绕的名和 component 当完整声明。

## 单次 lease 与文件边界

共同类已通过 QTemporaryFile 独占创建、关闭一个空普通文件；FTP 不是再次 NewOnly 创建，而是打开准确 `output.path()`。受理前核绝对路径、普通非 symlink、空文件、canonical 非空；打开后再核仍空／普通及 canonical 一致。FTP 自己持 shared lease；终结先关闭实际 writer，再发冻结的原 ID／原路径，局部原 request 继续持 lease 跨所有直接完成回调。析构同样先关 writer 再释放成员 lease。

caller／后台 artifact worker 仍须保同一 lease 到自己实际读取结束，不能把 FTP 回调到达当作 worker 已读完。最后 shared owner 只尽力删除自己的唯一输入；不能删除缓存、用户文件或任意远端 basename，不能宣称删除／磁盘空间回收已确认。

共同类没有跨 transport claim 方法；原生合同要求每次新建、不得把同一 lease 并行交给两个 FTP／HTTP writer。这里只核本 FTP 忙槽互斥，不保证抵御任意恶意 C++ 重用 lease、非合作进程替换路径、symlink／hardlink 竞争或文件系统认证。

unique 声明长度最大 32 MiB。每个实际包在 seek／write 前按 64 位计算 `offset+size`，同时不超过声明长度／32 MiB，并核累计写入量；seek 与完整 write 都须成功。数据阶段零 size ACK 作为无进展错误拒绝；原文件零长度仍可按 FTP 的 NAK EOF 终结。新增 missing block 表最多 4096 项，append 前核上限，填补不能减去大于原 missing span 的长度。正常成功终结还必须 flush 成功、文件 error 清、实际 file.size 与声明／累计量一致，close 后再次核关闭／错误和普通文件最终大小。任一失败给非空 error，不以曾打开／曾写入当成功；FTP 文件成功也不证明是有效 JSON。

unique 自受理起有独立单调 120 秒总期限，包／重试／cancel 不续期；独立 timer 保证即使一直没有包也会终结，早醒只等原剩余时长。发送／接收／timeout／实际写后及终结成功前也核原期限。不可中断的本地 IO 不能被计时器物理抢占，但超过期限不报告成功。legacy 不增加这个总期限、32 MiB／4096 表项限制或零 size 接受域变化。以上不是 fsync、耐久、真实远端文件完整性／内容真实性或恒定 CPU 时限，也不宣称旧路径／nameoverride／XML caller 已完整审定。

## 原请求、状态与精确取消

FTPManager／Vehicle 持 QQ。每次受理持独立原生 request，非回绕 serial；每次状态推进另有 phase、每次发送另有非回绕 sendSerial。发送、Link 虚 getter、generic sender 返回及 strong Link 释放后，都核存活、同 request／serial／phase／sendSerial；旧栈不能用一个后来重新出现的 index 或回绕 wire sequence 继续推进。MAVLink FTP 自己的 uint16 sequence 保原协议含义，不当作原生代次或认证。

入场围栏覆盖输入解析／文件建立；终结先退休原槽、停止 timer、关文件，再释放 busy 发原结果。同步 terminal 回调中可以开始新传输，旧外层只返回自己的受理事实，不清新槽。每个 handler／发送先冻结原 packet，缺洞列表不保引用跨发送／推进；进度只在原 legacy job 仍同且没有更深同步发送续作时末尾发布。unique 接收／burst 使用 modulo-uint16 半区间判断跨 sequence wrap，旧／非法包不先停 timer；超范围 packet data length 同样忽略并保原 timer。单包等待及 burst／补洞／terminate 重试仍沿原 1 秒／3 次重试，open 不新增重试；这不替代 unique 的原 120 秒总期，legacy 不因此获得总期保证。

`cancelUnique(id)` 对准确原 active unique 请求幂等：尚未取得 session 的 open 阶段直接本地终结；session 已知时沿原 terminate-session 状态机，零长度不被旧 `fileSize>0` 门漏掉。晚取消／旧 ID 不触后来请求；取消不会将成功 error 清空，不能宣称已发 open／read 被撤回或远端 session 一定清除。旧 cancel 保原 legacy 长度门及协议。

这里隔离的是本地请求与输入文件。真实 wire 仍没有 nonce／内容签名；原 generic sender 与 FTP 重试会使用其实际链路实现，不能把本地 ID／QQ／CRC 当原 PRIMARY 全部写入阶段的认证证明。来源是否仍允许采用，由实际 [ComponentInformationManager](ComponentInformationManager.md) 的原来源／phase 围栏另核。

## 未执行工程项

需实际覆盖：legacy 相机与 APM 错误声明长度／checksize=false；同名多车输入隔离；busy 拒绝且中途其它完成信号；同步完成／callback 启动新任务／删 Manager；坏 URI 后可再次受理；唯一普通空文件／被改／已非空／坏路径；32 MiB 恰界／超声明／大 offset／不完整 write／seek／flush／close／最终 size 失败；burst 缺包、补洞、wire sequence 回绕；open 与零长度准确取消、旧 ID／重复取消；active unique 不被 legacy cancel 误清；FTP 析构但 worker 仍持输入、最后 lease 释放及尽力清理失败。以上均未执行，不是协议兼容、性能或安全验收通过。

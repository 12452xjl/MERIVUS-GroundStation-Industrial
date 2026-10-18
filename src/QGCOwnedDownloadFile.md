# QGCOwnedDownloadFile：单次下载原件的原生所有权

## 528 用途与边界

纯 C++、无 QObject 的共享只读路径所有者。`create(QString& error)` 只能在实际本机临时目录用 QTemporaryFile 独占创建一个新空普通文件，核 flush／close 后转移自动清理责任；不能从 URL、basename、QML、Map、导入路径或缓存路径领养已有文件。`path()` 不是身份、完整性、设备认证或执行权限。

实际元数据 Manager 每次下载（包括一次坏缓存重下）及 Translation 每次 TS 下载各自 create 新 owner。新 HTTP／FTP 原生接口同时持有同一个 shared owner，写句柄关闭前不能释放最后引用；后台 ArtifactWorker 自己持原输入，覆盖 Manager 已析构但实际线程尚在读取的区间。TS 处理栈亦局部持到解压／XML 读取退栈。缓存命中的原件仍是借用，绝不接管删除。

原生调用契约为 single-transfer：同一个 owner 不得并发交给其它下载实例／FTP 或另一次请求。当前实际接线只在 Cached 原任务失败、前一子下载已关闭后，清空原文件再以同一原请求作一次 cache-only 回退；不是新任务复用。这里没有全局 writer 注册表，不保护任意违规第三方 C++ 同时复用 owner；亦不把 QString 路径存续视为对恶意本地文件替换、祖先 reparse／symlink、硬链接或进程崩溃的防护。

析构只对自己实际创建的准确唯一路径作一次 QFile::remove，绝不递归清目录、不清远端 basename、不移动／删除 raw cache、worker 快照、TS 解压结果或译后 JSON。清理是尽力而为，无耐久或删除成功回执；崩溃／占用／权限失败可能遗留临时文件，后续仍需独立受控留存策略。

字节上限和实际完整写入由新 HTTP／FTP 传输层分别实施，本类本身不执行下载、不限制所有旧调用域，也不代表整体峰值内存／磁盘／CPU 上界。没有新增外部依赖、数据库、QML 服务或飞行命令。

## 工程师待验证（全部未执行）

两机同名和同 URL 各有不同实际路径；空目录／创建／flush／close 失败；成功、错误、取消与父对象析构的共享寿命；worker 实际结束后才允许最后引用清理；缓存原件不被移除；失败清理遗留与真实平台权限行为。当前只修改源码并静读，没有运行创建／读写／删除、下载、helper、编译或测试。

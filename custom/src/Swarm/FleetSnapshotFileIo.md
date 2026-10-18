# 历史快照共享文件 IO

## 524：已闭部署归档共用相同写入事实

[部署归档worker](FleetDeploymentArchiveWorker.md)成为第三个纯值后台消费者，使用原32MiB上限及独立固定错误域 `deployment-archive`。共享helper不认识Episode、Run、任务记录或退休资格；保存及回读匹配只是Lifecycle最后复核的必要条件，不会自行删除对象、清占用、回收容量或恢复任何许可。commit成功后取消／回读失败仍保已写事实；只有实际原对象与全部受控后代销毁后的原生确认才可另记容量已回收。旧维护32MiB及GNSS4MiB接受域、路径限制、save回执和readback语义不变。本批未执行任何业务文件IO、helper、编译或测试。

## 523：从原维护 Worker 提炼，不共享业务文档

[FleetSnapshotFileIo.h/.cc](FleetSnapshotFileIo.h) 是纯 native、无 QObject／线程的公共文件操作；只由一次性 Worker 调用。它不接活动来源、账号、地图或权限，也不解析业务 schema。[维护工作区](FleetMaintenanceWorkspace.md) 继续原32 MiB、旧错误前缀和回执；[GNSS 记忆工作区](FleetGnssMemoryWorkspaceWorker.md) 使用独立文档与4 MiB。GNSS 不伪装成维护记录。

`localJsonPath(QVariant)` 保原路径检查：真实非 null 非空 String≤4096、有效 UTF-16／UTF-8往返、绝对本地 `.json`；拒首尾空白、控制字符、URL、UNC与`/dev/`。Windows还拒非盘符根路径、ADS、空段、尾点／尾空格和DOS设备名。它不创建目录，不认证全祖先无重解析点，不提供恶意TOCTOU、权限或介质身份保证。

`readUtf8(path, maximumBytes, cancelled, errorPrefix, bytes*, error*)` 保原末级普通非符号链接文件检查及64 KiB分块读取；先核1..maximumBytes，读满后核额外尾字节、实际size、错误及close错误，最后核UTF-8往返。上限须为1..32 MiB，维护传32 MiB、GNSS传4 MiB。失败不输出成功前缀；不声称文件在读取后仍未变化。`errorPrefix` 仅受信任C++固定域字面量，不取文件或QML字符串。

`saveAndReadback(...)` 返回值为 `SaveResult`：`ok,error,fileWriteAttempted,fileWrittenKnown,fileWritten,readbackVerified,readbackError`。写前检查末级符号链接／非文件目标；QSaveFile禁direct-write fallback，64 KiB分块和commit前检查原子取消。commit失败保`fileWrittenKnown=false`，不武断称从未写入；实际commit成功立即保`ok=true/fileWritten=true`，后续取消、回读失败或不匹配只进入独立readbackError，不能撤销写入事实。不自动重试、回滚或删除文件；不认证fsync、断电持久化或历史真实性。

维护原错误域 `maintenance-workspace-*` 和GNSS域 `gnss-memory-workspace-*` 共用后缀：`cancelled,file-invalid,open-failed,file-budget,read-failed,utf8-invalid,save-target-invalid,save-open-failed,write-failed,commit-unconfirmed,readback-mismatch`。域内schema、选择校验、双终结／单槽和public回执仍归原业务owner，helper不复用任一业务缓存或批准。

本轮仅源码与静态对读；未执行IO、helper、编译、测试或设备操作。工程师仍须验证路径、符号链接／重解析点限制、容量／短读／close错误、取消各阶段、commit结局不明与成功后回读失败，以及旧维护11字段事实保持。

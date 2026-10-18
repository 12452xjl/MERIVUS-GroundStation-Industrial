# FleetArchiveDirectoryInventory — 有错误边界的只读目录清点

批次 464 仅追加 `Namespace::EventBackups`（前三个 enum 值与旧前缀/行为保持），固定前缀 `fleet-events-backup-`。新 namespace 同样大小写不敏感入选、完整目录 entry 上限 4096、匹配项上限 1–128、真实 EOF/error 与句柄关闭检查；不会静默跳过 backup sidecar、大小写变体、目录或链接。完整三 UUID 名字、header/SQL/SHA 和原来源不由本类认证，由 [独立备份清点消费者](FleetEventArchiveMaintenanceWorker.md) 严格核验。原 sealed/retired/telemetry namespace 不纳入合法 backup 文件。新增分支未执行枚举、编译或测试。

阶段 310–314 的非 QObject 公共辅助类。用于替代不能独立报告枚举失败的 Qt iterator 判断；不打开日志内容、不执行 SQL、不创建或修改文件、不恢复任何控制权。只有原生枚举正常结束、目录对象检查通过、原生句柄成功关闭且未取消，才发布完整的本次清点结果。

## 原生接口与结果

`scan(canonicalAbsoluteDirectory, Namespace, maximumFiles = 128, cancelled = nullptr)` 同步执行，调用方须放在已有后台 worker/reader，不在 GUI/遥测接收回调中调用。`Namespace` 支持 `TelemetryJournals`、`SealedEventArchives`、批次 462 新增的 `RetiredEventArchives` 和批次 464 的 `EventBackups`，固定前缀分别为 `fleet-telemetry-`、`fleet-events-sealed-`、`fleet-events-retired-` 和 `fleet-events-backup-`。前三种 namespace 的行为、上限和前缀保持不变；合法 retired/backup 文件不进入 sealed 扫描，各自的大小写变体与 sidecar 则进入所属 namespace 交由消费者整批拒绝，不能当作“没有文件”。

- `Entry` 仅含 `basename / regularFile / symlink`，没有完整路径、位置、payload、SQL 连接或信任标志。Windows 的 `symlink` 保守地包括所有 reparse point，不仅传统符号链接。
- 前缀大小写不敏感地入选，名称排序大小写敏感；因此大小写变体、namespace sidecar、目录及坏名不会被静默跳过。消费者仍须验证完整 UUID/后缀、普通文件、无 symlink、当前 metadata/canonical path；此类不宣称入选文件符合任一日志 schema。
- 所有非 dot entry（包括无关项、hidden/system、目录）计入 `observedEntries`，超过 4096 整体失败；计数最多到触发失败的 4097。入选项最多 `maximumFiles`，参数必须为 1–128。
- `ok / enumerationCompleted` 只在本次明确正常 EOF 和检查通过时为真；失败或取消返回空 `entries / directoryIdentity`，固定错误码，保留有界计数，不暴露 native 错误文本或路径。
- `directoryIdentity` 是观察性目录对象标识，Windows 使用 volume serial/file index，POSIX 使用 device/inode。调用方可用来拒绝前后扫描时目录被替换；它不是认证凭据，也不保证跨文件系统、重启、删除重建后全球唯一。

## 边界与平台实现

输入先限制为非空、最多 4096 个 UTF-16 code unit、无 NUL/孤立 surrogate，再作规范路径/已有目录/可读/无 symlink/canonical path 核验。UNC、Windows device namespace、drive-relative path 和 alternate stream 被拒绝。Windows 另拒绝远程映射盘和未知盘型；POSIX 上本地命名路径不证明底层没有网络挂载。

Windows 使用 `CreateFileW` 的只读属性权限、`OPEN_EXISTING`、backup semantics、open-reparse-point，持有真实目录句柄。`FindFirstFileW/FindNextFileW` 不按属性筛选清点。首调用的 `ERROR_FILE_NOT_FOUND` 仅在独立目录句柄及后续同路径重开核验通过时视为空；后续结束必须为 `ERROR_NO_MORE_FILES`，其他失败不会被当成空目录。入选项另读原生属性；`FindClose/CloseHandle` 在成功路径都须成功。

POSIX 使用 `opendir/dirfd/fstat`。每次 `readdir` 之前清零 `errno`，仅 `NULL` 且 `errno == 0` 为 EOF；不依赖 `d_type` 或固定 `sizeof(d_name)`。入选项通过 `fstatat(AT_SYMLINK_NOFOLLOW)` 分类。目录在打开前后、EOF 后、当前路径重开时对照 `dev/ino` 和变化元数据；`closedir` 失败使清点失败。不支持的原生平台或缺少 `AT_SYMLINK_NOFOLLOW` 时 fail closed，不退回 Qt iterator。

原生路径和名字采用有界、可逆转换；不能无损 round trip 或 Unicode 无效时整批拒绝，不能用替换字符将不同原生名字混为一项。资源由不可复制 RAII 对象管理；失败路径也释放已取得句柄，不重复 close 可能已经释放的 descriptor。

## 不能证明的事情

`enumerationCompleted` 是明确结束且观察检查通过的枚举结果，不是目录锁、原子目录快照、完整历史/备份证明、所有 payload 哈希校验、认证、签名、fsync、恢复完成或执行许可。目录并发变化可能不反映到即时 metadata；对象 ID 可能重用，恶意 A→B→A 替换和底层文件系统语义不在此实现的证明范围。Windows 的 64-bit file index 在 ReFS 上也不保证唯一，不能将该观察值宣传为所有 Windows 文件系统上的唯一目录身份。消费者仍须按其原有协议做二次清点和逐文件只读核验。取消只在原生调用之间及每条目检查，不保证能中断阻塞的系统调用或提供实时 deadline。

本阶段没有编译、测试、枚举实际归档目录、读取日志/数据库或运行此 API。工程师后续应在目标 Windows/POSIX 工具链核实平台头文件/宏、Unicode、正常空目录、枚举中途失败、access denied、close failure、取消、超限、reparse/symlink、目录替换及并发改名等路径；这些是待做事项，不是已执行证据。

## 原生文档依据

- Windows 首调用无匹配、后续 EOF 的区分及 search handle 释放来自 [FindFirstFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilew) 与 [FindNextFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findnextfilew)。
- 只读目录句柄、backup/open-reparse-point 来自 [CreateFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew)；对象身份观察依据 [GetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle)，file index 的复用及 ReFS 边界见 [BY_HANDLE_FILE_INFORMATION](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/ns-fileapi-by_handle_file_information)。它们不构成原子目录快照保证。
- POSIX/Linux 错误与 EOF 区分依据 Linux man-pages 项目的 [readdir(3)](https://man7.org/linux/man-pages/man3/readdir.3.html)，句柄管理依据 [opendir(3)](https://man7.org/linux/man-pages/man3/opendir.3.html)、[dirfd(3)](https://man7.org/linux/man-pages/man3/dirfd.3.html)、[closedir(3)](https://man7.org/linux/man-pages/man3/closedir.3.html)，不跟随符号链接的分类依据 [stat/fstatat(2)](https://man7.org/linux/man-pages/man2/stat.2.html)。其他 POSIX 目标须由工程师验证其平台语义，不能把 Linux 文档当作平台实测证据。

# FleetMissionRemainderArchive — 完整但惰性的历史重规划来源包（473）

## 474：完整有序 Run 成员集合与离线明确选源

本增量补齐的是：同一实际 Run 的**全部有序执行目标**完整取源和保存，离线目录中明确选择一位可用原成员，再进入原严格的单成员人工余段提案／新审阅链。它不是联合多成员新计划、自动接管、远端任务恢复或完整自愈验收；多成员联合规划的政策合并、联合来源投影及操作闭环仍待后续开发。旧 v1 包、`build`、`planningSource` 和文件读写继续兼容，不把旧单成员包猜补为原整组。

### v2 包与集合来源

包继续 exact8，`archiveType` 仍为 `fleet-run-replanning-source`，版本变为2，唯一内容键由 `nativeSource` 换成 `nativeCollection`；其余 `archiveId/capturedAtUtc/sourceContext/claims/contentHash` 及固定声明不变。v1只收 `nativeSource`，v2只收 `nativeCollection`；混用／同时出现／未知版本整份拒绝，摘要覆盖整个新包及全部成员。

`nativeCollection` 为[原实际 Transaction 取源](FleetMissionRemainderSource.md)的 exact16：`schemaVersion/ok/reason/sourceKind/runId/startupId/transactionEpoch/transactionGeneration/deploymentId/binding/sourceTask/sourcePlan/vehicleIds/targets/readonly/flightCommandReleased`。内部版本为1、种类为 `native-run-planning-collection`；`ok/readonly` 为 true、`reason` 为空、飞行释放为 false。共同原 task 与 plan 各存根层一次，不为每位成员复制或换成“最新计划”。

`targets` 按原 `vehicleIds` 完整同序，1–32行；每行 exact6 `{vehicleId, available, reason, sourceMember, sourcePayload, progress}`。每个原成员、完整原载荷及映射、原观察行都保留，包括未 MODE 入队、部分提交、失联或已退休的目标，不只留下可用前缀。`available` 只在原观察的真实 `modeQueueBegan && modeQueueCommitted` 时为 true；该行 `reason` 必须为空。合法未提交行保留 `original-target-mode-not-committed`，不是一份伪造的成功 native18 源。

全集合统一调用 `FleetMissionStartupTransaction::validateCollection` 这个纯静态值入口，校全部身份、顺序、成员、载荷和观察关系，并在共享预算内重建完整 PayloadBuilder 结果；Archive 不创建、持有或调用活动 Transaction 实例。任意坏行、重复／漏项／乱序、共同计划不符、不可选行的坏载荷或超过每机2048／全组8192项，均拒绝整个包；不能以 `available: false` 隐藏坏值。成功返回的完整集合须与原输入 JSON 值相同，未删不可选行或修造成功来源。

`buildCollection(collection, sourceContext)` 返回与旧 `build` 相同的 exact3 `{ok,error,archive}`。它只接完整原集合，校源及上下文后建立独立 JSON-owned v2 值；最终仍校整包预算与摘要。所属线程、无状态／无信号／构造无 I/O、保存 commit 后读回失败仍 `committed: true` 等原合同不变。原 File API 对v1／v2都执行完整校验，不新增目录、锁、自动重试或 Run／占用恢复行为。

### 窄目录与明确选择

新增 `metadata(archive)` 返回 exact3 `{ok,error,metadata}`；**先校完整包和全部目标**，成功后才返回独立的窄目录。`metadata` 为 exact12：`schemaVersion/archiveId/contentHash/capturedAtUtc/sourceContext/runId/startupId/transactionEpoch/transactionGeneration/deploymentId/binding/members`；版本对应原包。`members` 保原序，每行 exact6 `{vehicleId,available,reason,waypointCount,modeQueueBegan,modeQueueCommitted}`。v1正常原源投影一行，v2投影全部行；不存在按需只校某行的快速成功路径。目录不含 task、plan、原成员整树、payload、完整映射或 progress，Root可只缓存该窄值而不在公开 getter 中重复解析完整源。

目录的 `waypointCount` 是原成员自身 `waypoints` 数量，可能为0，例如旧编译任务使用共同路线；不因此丢行，也不把共同路线猜造成原独立出航路线。v2 `available` 仅表示按原队列事实可取源，不等于该路线已满足全部余段请求条件。选择后的原单成员 Planner 仍独立要求受支持原路线、人工索引、端点、速度和全新执行配置；不支持时明确失败。为保持全部原v1值域，v1 `available` 沿原中央单源验证有效性，`modeQueueCommitted` 保原true；`modeQueueBegan` 为真实Bool时原样保留，缺失或其它类型则在目录中投显式JSON null未知，绝不补false／true，也不把可用标记说成已证明begin。v2两项queue flag仍须真实Bool且available严格等于二者同时true；原v1校包／旧 `planningSource` 的合同不变。

新增 `planningSourceForVehicle(archive, sourceVehicleId)` 返回原 exact3 `{ok,error,source}`，ID须为真实数值整数1–255。它先完整校包，再按明确原 ID 取源：v1只能选择其唯一原成员；v2使用纯静态 `sourceFromCollection`，再次完整校集合后才选择原 `available` 行，并经中央 `validateSource` 验证所得 native18。不选择第一位／最近／最新／任意可用成员，不接受未提交行冒充成功来源。不存在、不可选或坏源均返回空 source，无原生来源前缀。

成功源再转换为原 historical exact20／schema2，绑定**整个v2包**的 `sourceArchiveId/sourceArchiveContentHash`，经原中央源验证器再校后返回；不建立新的 provenance 形状或绕开旧 Compiler／PlanStore／Review 消费。旧 `planningSource(archive)` 仍按原 v1 唯一成员工作；传v2时在整包校验后返回 `remainder_archive_vehicle_selection_required`，不暗自降为某位成员。

共同资料／成员进度保留原 `sourceCurrent` 和未知／退休／未认证／未完成语义，文件导入不刷新成 live 证据；一次原生组捕获也不是多机同时采样或物理原子快照。原 Run、Orchestrator、占用和批准不被创建、恢复、清理或转移。原32 MiB／深24／1,048,576节点等**全包**预算不按成员倍增，任一超限整份失败。

本增量仅源码与直接静态合同核对，未编译、执行 helper／解析器／业务代码／文件 I/O／UI、测试、仿真或设备操作。工程师仍需覆盖：v1字节／身份兼容；多成员完整有序往返；全部未提交／部分提交／全部提交与退休源；坏末行和不可选行拒绝；metadata无完整源泄漏；明确ID／不可选ID／0航点；全包与全组项数预算；整包摘要改变后旧选择失效；commit成功后读回失败事实；原历史单成员重规划与新实际审阅。不能据本节宣称联合多机任务规划或自动自愈已经交付。下方473为原单源历史说明；涉及单成员包数量的限制由本节v2集合增量补充，原许可边界继续适用。

## 范围与真实来源分工

本模块保存的是[原 Run 重规划取源](FleetMissionRemainderSource.md)的完整单成员值快照，不是运行摘要、任务结果或可恢复的 Run。源包含完整原规划政策副本、完整原计划、所选原成员、该成员的完整 PayloadBuilder 结果及其 `sourceItemMap`、本次原观察行与原身份绑定。原 `sourceTask` 已按原取源合同剥除不可继承的运行采集字段；这里不补回旧 fleet／health／批准，也不把它称为未剥除的原 Intent 全参数。完整原计划保留其它成员，不表示本包包含每个成员的全部载荷或整组原子观察。

实际 Run／Transaction／选择 scope 的原生捕获、对象身份及前后来源检查由 Root 负责。本类只消费纯值，任何调用者也可能构造内部一致的值；校验成功不认证飞机、原生实例或文件来源。构造无 I/O；对象无持久状态、属性或信号，不拥有 Vehicle、Run、Orchestrator、Session、审批或占用服务。仅在 `MerivusInternal` 注册，公开产品面板不取得原始服务对象。

已有 [ExecutionReport](FleetMissionExecutionReport.md) 继续用于观察复盘；其报告／brief／summary 中的身份和摘要不包含本包所需的完整规划源，不可反推或补造缺失任务、路线、载荷或映射。PlanStore 的“最新计划”也不能替换原 Run 冻结的具体版本。本模块不修改旧报告、PlanStore 格式、Run 注册或占用恢复路径。

## API 和整份返回

全部 QObject 方法只接受对象所属线程调用；`archiveValid` 为不访问 QObject 状态的纯静态值校验。

- `build(nativeSource18, sourceContext3)`：exact3 `{ok, error, archive}`。只收原 exact18、schema 1、`sourceKind: native-run-planning-snapshot`，不收已经导出的历史 schema 2 再伪装为新的原生捕获。成功分配新的包 UUID 和主机 UTC 捕获时间；失败的 `archive` 为空 map。
- `static archiveValid(archive)`：完整校验根、边界声明、来源、预算和内容摘要，返回 bool，不读取文件或当前设备。
- `planningSource(archive)`：exact3 `{ok, error, source}`。先完整校包，再将其原 exact18 源转换为下述独立历史 exact20，交共享源验证器再次校验。失败 `source={}`，不返回来源前缀。
- `saveToFile(absoluteLocalPath, archive)`：exact3 `{ok, error, committed}`。校验整包后才写；实际 `QSaveFile::commit()` 成功后，即使完整读回／字节／内容验证失败，仍返回 `committed: true`、`ok: false`，不得当成没有发生保存或自动重试。
- `loadFromFile(absoluteLocalPath)`：exact3 `{ok, error, archive}`。完整读入、UTF-8／JSON／根／来源／摘要全部通过才返回原包，不分配新 UUID、不刷新原捕获时间、不创建活动任务。

成功的 `error` 是明确非 null 空 QString，失败为固定错误码；`ok` 与 `committed` 均为真实 bool。所有输出来源和包只由完整已通过的值建立，无截断／有效前缀。`build` 通过 JSON 值往返建立独立容器并保显式 null，Qt 数字宽度规范化不改变 JSON 值或原载荷内容。

## 文件根与固定声明

根 exact8：

| 字段 | 合同 |
| --- | --- |
| `schemaVersion` | 实际数值整型 1；JSON 整值兼容，非字符串／bool／null |
| `archiveType` | `fleet-run-replanning-source` |
| `archiveId` | 小写、非零 UUID |
| `capturedAtUtc` | 主机 UTC `ISODateWithMs`，不是飞机时间或任务完成时刻 |
| `sourceContext` | exact3：`lifecycleEpoch` UUID、`selectionGeneration` 规范正 uint64 十进制字符串、`episodeId` UUID |
| `nativeSource` | 原 exact18／schema 1 完整来源；不能以 summary、历史 exact20 或部分政策代替 |
| `claims` | 下述 exact8 固定声明 |
| `contentHash` | 去除此键后的整个包，用 Qt 紧凑 JSON 编码计算小写 SHA-256 |

`claims` 恰八键：`snapshotOnly`、`historicalOnly`、`planningOnly` 为 true；`sourceAuthenticated`、`approvalRestored`、`missionCompleted`、`flightCommandReleased`、`executable` 为 false。未知键、改变声明、无效时间、身份或摘要均整份拒绝。

完整来源统一调用 [RemainderPlanner](FleetMissionRemainderPlanner.md) 的 `validateSource`，校验原 Run／startup／transaction epoch／generation／deployment／7或9字段 binding、原计划及内容摘要、唯一所选成员、完整重建载荷，以及 progress 与载荷映射关系。Archive 不自建另一套宽松 summary 校验器，也不向校验器拼造临时规划 request。共享验证后的完整来源必须与原输入 JSON 值相同，不能默默修复后换源。

原绑定的 `reviewEvidenceHash` 等追溯值完整保留，但本包没有另带整份原 reviewEvidence；格式／内部一致性验证不能重新认证被省略的证据或恢复其当前资格。包摘要、payload hash 和 READ／队列相关本地引用同样不是签名、远端任务实例标识或飞机端结果证明。

## 历史规划源和重新审阅

`planningSource` 保留原18字段中的值，仅把 `schemaVersion` 改为2、`sourceKind` 改为 `historical-run-planning-snapshot`，增加 `sourceArchiveId` 和 `sourceArchiveContentHash`，得到 exact20。两新增值必须分别等于已验证包 UUID 与**完整包**摘要，不是原计划摘要或新随机构造值；共享源验证器再检查完整结果，失败不发布部分来源。

原 progress 的 `sourceCurrent`、退休、连续性未知、缺失序号、历史候选和所有未认证／未完成声明原样保留，不改写成导入后 live／新鲜证据。文件捕获时间不重置原接收序列或主机单调时钟引用。最后 reached／current、末项或落地候选都不是连续完成前缀、真实覆盖率或正式任务结果；新规划仍按原人工选择航点和完整驻留重放政策，不自动跳过“已完成”区域。

导入只提供新人工重规划的历史输入。它不注册／恢复 Run，不恢复或释放占用，不接受旧 Session／批准，不续飞、重传、自动 prepare、解锁或上传。实际新提案必须经过当前成员／配置／基准／能量／净空等完整审阅和新的审批链。历史类型及包关联可供审计，但类型标签、格式正确和摘要一致本身不证明原生来源；最终执行门禁不因本模块而放宽。

## 预算、文件和失败事实

输入／整包／文件最大32 MiB，包含外层包和源；最大深度24（根为0）、1,048,576节点、单 map 1024键、单 list／string-list 32768项、键1024字符、文本32768字符。累计键／文本 UTF-8 字节和最终紧凑 JSON 字节分别受32 MiB限制。额外包络可能使原来接近来源独立限额的源被拒绝，不能靠截断、丢非活动字段或放宽来源校验来绕过。

只收纯 JSON 值和可编码 string-list；原生缺失 QVariant 与显式 JSON null 均作惰性空值，不把它们补成序号0。有限数值须在 JSON 精确整数范围内，拒绝 QObject／typed-null 指针、日期、二进制和自定义值；无效 Unicode 整份拒绝。来源内部仍受共享验证器原有更严格字段／预算条件约束。

文件 API 仅接绝对文件系统路径，拒空／首尾空白、NUL／控制字符、无效 Unicode、`://` URL、Qt资源路径或超过4096字符的路径。读写不接受目标符号链接，读取要求普通文件；不创建目录、不自动搜索／扫描／保存、无锁、无修复／删除／重试。显式文件位置和覆盖确认由 Root／面板负责，本类不能证明操作员授权或目录祖先／远端文件系统的真实性。

保存关闭 direct-write fallback，检查完整写入及真实 commit，再全量重读目标文件、检查大小／UTF-8／JSON／整包及**精确原始字节**相同才报告成功；commit后的任一失败保留 `committed: true`。没有锁或未来文件稳定性／断电持久性保证，之后的外部修改不由既有成功回执认证。读取大小／类型变化、坏JSON／版本／摘要／映射和超限都返回失败，不返回可误用的部分包。

稳定错误以 `remainder_archive_*` 为前缀，区分所属线程、输入类型／预算、原源／上下文、输出、路径、打开、写入、commit、完整读回、UTF-8、JSON和历史转换失败；具体 `committed` 必须独立读取，错误文字不能代替它。

## 源码阶段与工程师待验证

本轮仅源码编写、直接静态生产／消费核对及普通文本空白检查。未编译、运行 helper／解析器／业务代码／文件操作／UI、测试、仿真或设备操作；工程登记与类型注册不是实际构建或功能验收。

工程师后续需验证：原生18和历史20严格隔离；完整原成员／非活动资料／政策／payload／sourceItemMap往返；退休／失联／缺失序号／显式null不变；schema／上下文／绑定／载荷或progress末字段失配整份拒绝；32MiB／深度／节点及UTF-8预算；坏哈希／错误声明／Unicode／截断文件；符号链接／路径／目标覆盖／目录不存在；完整写入、commit失败以及commit成功后读回失败仍保留事实；Root同步回调销毁／选择变化时不把历史源当当前原生源。不得据本包宣称正式任务结果、完成前缀、Run或权限恢复已经实现。

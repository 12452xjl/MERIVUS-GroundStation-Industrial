# FleetMissionClearanceDatum — 454 原生工程 datum 对应

## 456：原datum14到最终原生发送消费者

[Witness](FleetMissionStartupClearanceWitness.md)保原typed datum14及source18、实际Registry／Catalog／Vehicle／PRIMARY。完整recapture和无回调memory尾核复用本helper，UTC floor／成功输出传播为跨owner共享水位；不得用Worker JSON、成功flag或新样本重建原datum。原Review copied clock截止只缩短，无新lease。

最终Txn／既有Vehicle命令所有权切点消费全组同源材料，但不认证物理高程／当前全球高度分支／同估计器样本，也不替代运动包络包含与journal全组尾栅；原pending及完整StartPermit仍未完成。

本批仅地面站源码与限定直接静态阅读、文本／路径／Markdown及git空白核对；未编译、执行helper／parser／AST／脚本／线程／UI／认证／SQL／应用／设备或仿真，未测试、修改PX4、刷机、放行、提交、更新PR或打包。原25目标和六类实质剩余保持，整体约40%仍只是源码工作量粗估，不按批次／文件数上调。 下方为历史。

## 455：后台启动报告消费保持原生依据

普通／地面审阅继续保留首次原生14及Source18，并将其纯JSON副本纳入[StartupClearanceWorker](FleetMissionStartupClearanceWorker.md)的前置几何输入指纹。副本不恢复native来源，后台线程不接本类或任何Registry／Vehicle对象。结果回到owner后仍复核原配置／固件／原点／目录及UTC水位，不以较晚样本替换首次审阅材料。

本类exact5／14及UTC原生下界／输出接口没有改变；最终Authority移交、Transaction独立持有／逐包末核和允许运动包络包含关系仍待开发，原pending保持。此增量不是物理datum、当前全球高度分支、完整净空或StartPermit认证，未编译、运行或测试。下方454为原合同与历史限定复核。

本模块把已冻结的净空场地／构型来源，与实际固件观察、本地工程声明目录及已观察的 `GPS_GLOBAL_ORIGIN` 参考对应起来。它只确认**显式声明仍对应这些本机来源**，不认证物理高程基准，不推断当前 `GLOBAL_POSITION_INT.alt` 使用哪条估计器分支，也不解除 `clearance_geometry_review_pending` 或发出飞行许可。

本轮只进行了源码静态阅读与文本检查；未编译、未运行本模块／解析器／测试／仿真／应用／真机。

## 454 静态复核后的边界收紧

初稿的“已完成最终静态收口”表述已撤回，不能作为验证结论。恢复开发后对下面三个边界作了限定修正：

- 最终 `memoryPass` 不再调用 `startupGlobalOriginObservationSourceMatches`。它直接使用原生纯内存 `startupGlobalOriginObservation()`，严格要求 observed、available、stable18 与首次材料相等；available 已包含实际 PRIMARY／VERSION／flight／epoch 的内存核对。这不依赖 SourceMatches 入口将来是否引入回调。当前所读 Vehicle 实现的 SourceMatches 本身直接转内存谓词，不能把另一个 `_startupGlobalOriginSourceAllowed` 的 Link 虚调用混为此方法的现有实现。
- 捕获阶段的 SourceMatches 返回值先保留为 Bool，随后独立调用 `fenced()` 核 QPointer／原对象／PRIMARY owner；只有核对成功后才再次读取 Vehicle。读取 origin 后、调用 Catalog 前也再次围栏，不让回调后遗留的 raw 对象继续使用。
- `ownersFor` 不再 `weak_ptr::lock()` 临时取得强引用。新 Vehicle 纯内存 `autopilotVersionObservationPrimaryLinkMemory()` 返回实际 VERSION 绑定且仍为当前 PRIMARY 的对象，Datum 以 QPointer 保存，同时仍冻结原 weak control-block 身份。避免临时强引用析构成为隐含的删除／回调边界。

另补充了实际跨层 UTC 水位缺口：Datum 内部可能观察到比外层 Source 更晚的 UTC；如果不回传该值，owner 只保存较早时间，后续回退到两者之间可能漏检。三个 native 接口因此增加可选最小已观察 UTC 与成功最终 UTC 输出（不进入 evidence／hash，见下），owner 必须按层、跨调用保留最大值。这不改变 exact5／14／4、原材料冻结、TTL 或授权语义。源码修正与静态重读不能替代工程师的编译和回调／销毁场景测试。

## 原生接口与所有权

- `capture(source18, registry, catalog, actualVehicles, fence, minimumObservedUtcMs=-1, observedAtUtcMs=nullptr)`：仅接受实际审阅 owner 保存的首次成功 [ClearanceSource](FleetMissionClearanceSource.md) 证据；先重核它的当前构型和时间窗口，再从实际对象采集 datum 对应。
- `recapture(originalDatum14, source18, registry, catalog, actualVehicles, fence, minimumObservedUtcMs=-1, observedAtUtcMs=nullptr)`：重新核对原生来源，但成功必须与首次 `originalDatum14` 按 QVariant 类型、null、字段和列表顺序完全一致，返回原材料而非替换材料。
- `memoryCurrent(originalDatum14, source18, registry, catalog, actualVehicles, minimumObservedUtcMs=-1, observedAtUtcMs=nullptr)`：最终无外部回调的本机来源／UTC 栅栏。不会调用 motor 插件、外部 fence、Link 虚方法、weak→strong 提升、原始消息请求，也不重建／哈希整个目录。它会校验小证据和其小型内容摘要；不等于远端当前可达。

三者都不是 QML invokable，也不是归档导入／权限恢复接口。传入的实际 Vehicle 集合必须与原 `source18.members` 恰好同一组 ID，1–32 架，无多余、重复、替代；输出保留原 members 顺序。Registry、Catalog、Vehicle、ParameterManager、VehicleLinkManager 必须属于调用线程；不要求通信 Link 与 Vehicle 同线程（正常 UDP Link 可有自己的线程）。

保留实际对象 QPointer、Vehicle/component 身份、PM/VLM 对象及 PRIMARY weak control-block 身份。所有 VERSION getter、VERSION SourceMatches、构型重新采集与调用方 fence 之后，整组再做无回调内存核对，避免后一个对象的回调使前一个已失效却留下成功前缀。输入 map／集合／fence 在回调前复制；销毁、换源、目录换代或任意一机失败都不返回部分 evidence。

## 返回合同

`capture` / `recapture` 固定 5 键：

`provided`, `inputValid`, `declaredDatumCorrespondenceMatches`, `reason`, `evidence`。

前三项为真实 Bool；`reason` 为 QString，成功与 absent 是明确非 null 空串；`evidence` 是 map。失败匹配为 false 且 evidence 为空，不保留有效成员前缀。`inputValid` 表示保留源／材料的输入形状，不表示当前来源或声明有效。

旧无净空输入以空 `source18={}` 表示：`provided=false,inputValid=true,declaredDatumCorrespondenceMatches=false,evidence={}`，不访问实际对象。`recapture` 对应的原 datum 也必须为空；`memoryCurrent({}, {}, ...)` 返回 true 仅表示 absent 兼容，不表示 datum 获证。

成功 evidence 固定 14 键：

| 键 | 内容 |
| --- | --- |
| `schemaVersion` | Qt Int `1` |
| `source` | `native-local-declared-clearance-datum` |
| `inputFingerprint` | 原 source18 的完整声明输入 SHA-256 |
| `catalogBinding` | 原目录固定 3 键：`catalogEpoch/revision/catalogFingerprint` |
| `members` | 原顺序成员列表，每行固定 4 键，见下 |
| `declaredDatumCorrespondenceMatches` / `sameDatumZeroOffsetDeclared` | true，仅工程声明对应 |
| `physicalDatumAlignmentCertified` / `currentGlobalAltitudeBranchProven` / `sameEstimatorSampleProven` / `hardwareIdentityAuthenticated` / `performanceCertified` / `startupPermitGranted` / `flightCommandReleased` | 全部 false |

每行 4 键：`vehicleId`（Qt Int）、`configurationEvidence`（原 source 成员的 native13，不混用 typed fingerprint 与原 JSON configuration fingerprint）、`originReference`（实际 stable18）、`datumCorrespondence`（Catalog 原生匹配的固定 10 键）。

`datumCorrespondence` 保留完整原 `firmwareBinding`（当前匹配成功为 33 键）、工程 relation12 与 relationFingerprint；其 `observationBinding` 固定 12 键，保留 Vehicle/component/固件类型、来源 epoch、contentRevision/hash、UID 已知状态、MISSION_INT 声明与固件元组。Catalog 成功 `errorCode` 的原空 null QString 保留，不用 JSON 往返改变类型。目录 receipt、VERSION receiptSequence/age 和原点接收年龄不进入稳定材料；正常同内容重复接收不自行重建批准身份。

小原生材料独立限制：树深度 12、16,000 节点、键最长 128 UTF-16 单元、累计键／字符串 UTF-8 字节最多 512 KiB；有限数字在既有安全整数幅度域内。不截断有效前缀，不放宽旧场地或审阅预算。

## 对应的内容与实际来源

Catalog query 固定 6 键，全部取原 source 或实际原点：

`sceneId/sceneRevision/sceneSourceDocumentHash/sceneVerticalDatumId/configurationFingerprint/globalOriginReferenceHash`。

只接受目录中明确 `reportedAltitudeSource=global-position-int-alt`、`relationModel=engineer-declared-same-datum-zero-offset`、整数 `offsetMm=0` 且 `reportedAltitudeDatumId=sceneVerticalDatumId` 的 relation。不会自动生成、选择近似、默认零偏移，也不改写任务坐标。legacy descriptor v1 没有此声明，不会被自动升级成成功。

VERSION 取实际 Vehicle 的 `autopilotVersionObservation(30000)`，并核实际 PRIMARY SourceMatches。末尾以新增 memory predicate 核原 sourceEpoch/contentRevision/contentHash 与同一 PRIMARY、原生源及本地年龄。该 30 秒是原生版本接收年龄规则，不是固件认证。

原点使用 [FleetMissionStartupGlobalOrigin](FleetMissionStartupGlobalOrigin.md) 的 native24 → stable18 校验，并额外要求 **observed=true、available=true**。该 helper 为其它启动逻辑保留的 never-observed 兼容分支在本模块不能成为 datum 对应依据。完整实际 referenceHash 包含原时间值；source／origin epoch、referenceRevision 也被保留，变化、失源及观察到的 A→B→A 都不能沿用原材料。不把非周期事件的年龄伪装成 3 秒周期鲜度。

最终整组依次核：Registry 原13 MemoryCurrent、实际 VERSION 原身份、原点纯内存观察 available 与新 stable18 相等、目录缓存3相等。目录 cache 在真实目录提交时更新，重复 timer 不能触发 1 MiB 目录全量序列化。

## 时间、冻结及不证明的内容

沿用原 source18 的规范 UTC：`acquiredAtUtc <= now < validUntilUtc`。有资料时 `minimumObservedUtcMs` 只接受 `-1`（未提供水位）或 `1..9007199254740991` 的 qint64；调用前 UTC 不得低于该水位。capture 记录调用前 UTC、内部 Source::recapture 成功实际观察到的 UTC 和全回调后的 UTC；后者不能小于任一已观察前值。没有新的 TTL、续期或替换原 until。memoryCurrent 同样核最低水位与前后 UTC 未倒退。

三个接口在进入时先把可选 `qint64* observedAtUtcMs` 清为 `-1`，只在**有资料且成功**时写最终 UTC。absent／任何失败（包括 recapture 新材料与原材料不相同）都不发布水位；recapture 使用局部输出，原材料完全匹配后才转交给 caller。输出指针应指向调用方本次同步调用的本地变量，不得指向可能在 fence 中被销毁的 QObject 成员。动态时间不进入 exact5／14、材料 hash 或批准身份。

真实 Review 应以 `max(外层 Source 已观察 UTC, owner 上次 UTC)` 作为 capture／recapture floor，保存返回值；最终 memory 核以该值作为 floor 并接收更晚输出，owner 再观察的 UTC 也必须不小于该输出。不能把 owner 水位重置成较早的外层 Source 时间，不能借新观察续期。

跨调用的 UTC 回退 sticky 和首次不可续期 monotonic deadline 仍由实际 Review owner 保存；本 helper 没有全局可变时钟状态，不能替代该门槛。字段名 AMSL、有限数值、原点存在及匹配哈希，均不足以证明已知 geoid 模型、测绘高程与飞控估计高程真实重合、PX4 当前 GPI 采用哪条分支、同一样本、实时地形／障碍或完整净空。工程声明即使对应，也仍需工程师核实 datum 资料与固件行为；这里不写 PX4、不请求消息、不迁移任务、不授予启动或执行权限。

## 工程师待验证（未执行）

- 无净空输入的旧任务保持 absent；有输入但无 relation、无实际原点、错误场景／构型／原点 hash 明确失败。
- 合法 descriptor2 + 原13构型 + native VERSION + 实际 observed origin 的捕获／重复捕获及普通、地面审阅完整路径。
- 同内容重复 VERSION／原点包不改变稳定材料；原点重置、版本／PRIMARY／PM／Catalog 换代后原材料不能恢复。
- fence／插件回调销毁或换源、第二架回调使第一架失效、实际成员集合改变均无成功前缀。
- UTC 在内部 source 核对后回退、到期及原 monotonic 期限到达应拒绝；不得通过 recapture 延长有效期。
- 严格 native 类型与 Qt 5 的 null QString 保留；本机编译、静态分析、仿真和硬件验证由工程师另行执行。

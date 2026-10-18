# 本地能力声明的双账号工程审阅

## 484：独立性能声明请求与拟context绑定

新增独立 `requestPerformanceReview(session,roles,root,scope,id,caps,proposedPerformance,reason,confirmed)`，闭合operation为approve-performance。旧requestReview只接受原四操作，不能改operation绕过新proposal。角色政策复用fleet.capability.review，认证用途独立fleet.performance.declaration.approve；原30秒、双账号、实际Root／Swarm／Registry所有权及最终callback-free身份尾核保持。

proposal是规范exact2，与483数域一致；完整非空caps显式提交，不继承旧清单。初始和每次recordCurrent都要求同原完整baseline14，并让实际Registry重新stage核同拟context。数字／context／scope／caps／理由的私有存储独立持有，原baseline身份纳审批hash，不能以显示JSON、caller map、旧批准或仅配置哈希代替来源。

仅新操作preview为schema3/exact20＝旧18＋proposedPerformance＋proposedContext；原四操作schema2/exact18不变，current30/result16/Root receipt5/approval14均不扩。新OS summary按数值／revision最大显示宽度保守不超过440字符，低于Session原600字符上限；没有通过放宽Session预算接受它。实际commit调用Registry approveDeclaredPerformance一次CAS，不先updateVehicle再approve；旧任务依据失效，原构型基线不变。原已撤销／缺能力批准不自动复活；在完整基线、明确新清单和新独立审批条件下可建立新的能力批准。

性能值是工程声明，不是实测清单、性能认证、二进制或改装检测。本地结果、EvidenceChain追加、原21键独立历史摘要与SQL持久化分别处理，失败不抹提交事实。32工程验证全部未执行，未编译／运行／测试。下方为历史。

## 422 独立历史摘要不改变原审批合同

Root 在原生批准返回后冻结并保留原结果，同时尽力向[实际归档 Store](FleetPersistentEventStore.md)追加固定工程摘要。审批前的 configurationComparison 先复制，基线提交回执中的 previousComparison 仍表示原审阅差异，不能把提交后的零差异当原审阅内容。两类事件严格分为能力和基线操作，不存原账号、理由、配置或审批凭据。

新增外层 archiveSummaryRecordConfirmed／archiveSummaryOutcome 只报告内存摘要接纳，不扩 native result16、commitReceipt14 或审批凭据。原 EvidenceChain 的追加与此 BlackBox 摘要独立，两者均不证明 SQL 持久化；通知中来源销毁、摘要失败或抛异常不改写已冻结的提交事实。归档关闭时不会回录此前审批，旧 v1 详情无法补回。该摘要不是可恢复的完整构型或批准，本批未运行验证。

## 421 能力声明与构型基线分用途审阅

本节为当前合同，覆盖下方418/419历史中的“两种操作”“一律要求非空context”和“本流程不改变基线”说明；旧内容保留作分阶段记录。复用同一实际 Swarm/Registry 拥有的工作流，不新增通用可写 QML 后端。

- `approve`：完整替换本地采纳能力，清单仍为1–128个规范标签。
- `revoke`：撤销原能力采纳。
- `approve-baseline`：将当前原生完整有类型构型保存为本进程的本地参考基线。
- `revoke-baseline`：撤销实际已保存的本地基线批准，不恢复硬件、飞控参数或旧配置。

后三种操作的能力清单必须为 `[]`，不从旧批准继承标签。能力操作使用精确政策 `fleet.capability.review` 与用途 `fleet.capability.review.approve`；基线操作改用精确政策 `fleet.configuration.review` 与用途 `fleet.configuration.review.approve`。两类均要求维护员/管理员申请及不同SID的合格账号独立审批。原 Session 从同一实际 typed Review 冻结用途并一次消费，最终纯内存尾栅还核原 owner/Session/Episode；不能拿能力审批凭据批准基线，也不生成飞行或上传 lease。

`approve-baseline` 必须有该实际条目当前仍完整、非空的原生有类型观察；哈希、合并残留map或显示JSON不能替代它。首次批准允许没有旧基线或前次观察，即比较的before未知、comparisonKnown=false；这不是当前after完整性的豁免。“完整观察”不证明机型必要字段齐全、实物无改装或性能已实测。

仅 `revoke-baseline` 可在实际 `registryContext={}`、当前构型未知时继续，但必须冻结 `configurationApprovalRecord(id)` 返回的实际存储批准和原比较16键，不能用旧 `configurationApproval()` 的报告哈希回退值充当批准。合法旧记录为2–5键，缺失的ID、理由或时间不补造；原生另私有冻结独立静态身份及原typed配置，空context不补造ID/指纹，也不放宽另外三种操作。旧记录source的trim、指纹的trim/lower仅用于合法性和关联比较；原始DTO仍原样冻结、显示及CAS核对。

current30/schema1、result16、preview18/schema2、Root收据5和能力approval14均不改键数。基线操作的 `preview.previousApproval` 仍是原能力批准，`preview.configurationApproval` 才是实际原基线批准，二者不能互换。原生比较、静态依据、账号、政策、作用域、输入或期限变化都会退役原审阅。

每次实际基线批准/撤销都退役旧current能力批准，并推进观察/审核版本或在版本耗尽时保守失效；同一哈希/同一基线重新批准也不例外，不能只依赖旧静态哈希变化。需要另行审阅能力与任务，不自动恢复批准，不取消既有机载飞行，也不发送任何飞行命令。

基线 fenced CAS 重载以原比较16键为expectedComparison，返回恰14键：schemaVersion、ok、reason、action、vehicleId、mutationCommitted、previousComparison、comparison、approval、baselineEvidence、stillCurrent、hardwareIdentityAuthenticated、performanceCertified、flightCommandReleased。后三项恒false；基线approval是新原生5键记录或撤销后的空map，不是能力approval14。原生先完成实际四表提交并固定回执，再通知；耗尽的保守提交可 `mutationCommitted=true/ok=false`，通知后删除或重入可使 `stillCurrent=false`，都不抹掉已经发生的提交。

Root 将能力与基线结果分别追加为 `fleet.capability.review.result` / `fleet.configuration.review.result`，保留原生 `approvalEvidence.action`。真实提交、当前有效性、外层调用核验及本地审计追加确认分开；审计失败不回滚、不自动重试。旧bool兼容入口仍仅是受信C++后端入口，不强制这套账号工作流，不是公开QML授权接口。

本节仅记录源码接线及静态交叉阅读；未编译、测试、运行认证/UI、仿真、设备或刷机。`runtimeValidated=false` 与原生 `reviewUiIntegrated=false` 保留；完整有类型基线仍是本进程记录，重启/导入不恢复成可信完整基线，也不代表硬件、性能或飞行认证。

## 419 有类型构型比较

原生从实际Registry读取[构型比较16键](FleetConfigurationDiff.md)，冻结独立观察epoch/revision、原context、比较来源和前后有类型指纹；每次来源/提交复核还要匹配同一份比较。正常健康不改变观察历史；具体字段变化即使旧JSON哈希相同，也不能沿用旧能力批准。比较未知不自动拒绝本地声明，仍须原有效context/账号/期限；没有放松任何飞行许可。

preview独立升级schema2/18键，增加configurationComparison；current30/result16/Root收据5及原context6/approval14不变。原配置按共享helper的有类型语义核对；当前完整观察未知时configurationKnown=false、configurationJson为空，不展示合并残留的旧map为新配置。下方418的无字段比较说明仅为历史，由本节覆盖。

## 418 范围与实际来源

[原生工作流](FleetCapabilityReview.h)由实际[SwarmController](SwarmController.h)创建，私有绑定同一 Swarm 的原 FleetRegistry；不注册可由界面任意替换的 QML 后端。Swarm 的 Registry 改为 QPointer，销毁通知与 ownsCapabilityReview 只说明原对象配对，不授权限。实际入口见[面板合同](FleetCapabilityReviewPanel.md)。

操作为单机 approve（完整替换本地采纳能力清单）或 revoke（撤销原批准），不改变构型基线、飞控参数、运行任务、占用或任何飞行命令。可以审阅本地已登记的离线条目，但不能由此证明当前实物或连接来源；最终任务仍必须经过原健康、参数、原生来源和执行门禁。

这是工程人员对当前本地描述的明确采纳，不是自动识别改装、硬件身份认证、实测性能证明或飞行放行。通用机型/固件/UID 标签不变成可信实物身份。

## 正向流程

1. 当前实际维护或管理员账号选择 1–255 的本地机号、approve/revoke、能力清单及工程理由。
2. Root 验证实际数值/字符串/数组类型，冻结输入与原 current scope、身份5字段、角色9字段；原生读取真实 Registry context6、旧 approval14、静态构型及基线记录，不接收 UI 自造“已验证”记录。
3. 界面显示完整冻结依据、拟采纳与当前能力差异，操作员作两项明确确认。
4. 专用 fleet.capability.review.approve 要求另一个合格实际账号；申请者与审批者 SID 不同。原30秒单调期限覆盖账号提示，不延长原身份 TTL。
5. 原 Session 一次消费本用途的批准；所有可回调来源核对结束后，只走原生内存绑定与最后提交围栏。
6. Registry 对同一原条目版本比较并提交，先固定值回执再发通知；Root 单独追加本地审计。

角色政策的 fleet.capability.review 强制双账号；原生仍独立限制申请角色为 maintenance/administrator。审批角色沿用 safety_officer/commander/administrator；本地可编辑角色标签不能替代真实账号。

## 冻结与失效

绑定原 Swarm/Registry/Root/Session/Role 对象、parent/thread、当前 Episode/scope、review UUID/代次、机号、操作、清单、理由、原 context/approval 和完整静态显示依据。删除重加、导入、clear、构型/能力/基线变化、账号/角色/作用域变化使原审阅失效；换回相同内容不恢复旧版本。

普通心跳、位置、健康年龄等不参与这份静态审阅身份。发现状态、发现错误、构型依据来源变化会使已展示依据失效；不把全机队正常收包误当改装。

原配置 map 私有保存，按 QVariant 原类型、空值和逐层内容比较，受既有深度/节点边界约束。界面只显示 configurationJson，representation 为 qt-json-display-not-typed-export；它不是可导入的原类型导出，也不替代原 map/context 的授权核对。原始类型的字节、日期、typed-null 等不能从显示 JSON 完整还原。

完整基线可用性由原生私有历史决定，不从旧哈希或导入标签反推；没有完整批准快照时可比较前次完整观察，并明确不是批准基线。缺前/后快照为unknown，截断结果不得当成全部字段。hash-only构型不会被包装成检查过完整实物。速度和续航一直标为声明值；人工可采纳未被当前观测列出的能力，但必须明确工程理由，不把该人工决定称为实测证明。

## 值合同

- current 恰30键：schemaVersion、workflowEpoch、reviewId、reviewGeneration、state、reason、scopeToken、bindingHash、remainingReviewMs、busy、promptInProgress、vehicleId、operation、capabilities、engineeringReason、registryContext、preview、applicantActorId、approverActorId、operationConfirmed、authenticationConsumed、mutationCommitted、commitReceipt、authorizationOwnerRetained、source、reviewUiIntegrated、hardwareIdentityAuthenticated、performanceCertified、executionPermitted、flightCommandReleased。
- preview为schema2、恰18键：schemaVersion、source、registryContext、previousApproval、previousReviewIdentity、configurationApproval、descriptor、configurationComparison、observedCapabilities、declaredCapabilities、effectiveCapabilities、proposedOperation、proposedCapabilities、engineeringReason、localDeclarationOnly、hardwareIdentityAuthenticated、performanceCertified、flightCommandReleased。
- 原生 result 恰16键：ok、reviewId、state、reason、vehicleId、operation、operationConfirmed、authenticationConsumed、mutationCommitted、commitReceipt、approvalEvidence，以及上述五个始终 false 的 UI/硬件/性能/执行/飞行字段。Root 可再增加收据与审计/外层状态字段。
- approvalEvidence 恰8键：source=local-account-review、applicantActorId、applicantRole、approverActorId、approverRole、action、bindingHash、authenticationConsumed；无密码或可复用凭据。
- Root 收据恰5键：workflowEpoch、reviewId、reviewGeneration、bindingHash、scopeToken；它仅选择原私有审阅，不接受任意后台引用。
- 原 context6/approval14 不变；详细含义见[能力证据](FleetCapabilityEvidence.md)。

状态为 empty/preparing/review/approving/applied/stale/expired/cancelled。applied 只表示本地发生过变更，不保证成功批准、当前仍有效或机载执行。

approve 的清单为1–128个规范小写、去重排序标签，每项最多128 UTF-16字符；revoke 清单必须空。理由为非空已 trim 的单行字符串，最多1024字符；原生拒绝坏 UTF-8 往返及控制/格式字符。

## 提交事实与兼容入口

Registry 新重载要求 finalCommitFence，并返回14键：schemaVersion、ok、reason、action、vehicleId、mutationCommitted、previousContext、context、approval、effectiveCapabilities、stillCurrent、hardwareIdentityAuthenticated、performanceCertified、flightCommandReleased。

完整 staging、复制 fence、原 QQ/私有状态复核、最后 fence 后再次纯比较，才提交。通知前固定回执；通知后的删除、重入或健康更新可以使 stillCurrent=false，不能抹掉已提交事实。版本耗尽的保守失效可能 ok=false 且 mutationCommitted=true；不可按 false 自动重试。

旧 bool C++ API 保留为受信后端兼容入口，不强制此工作流的账号授权。新 fence 重载本身也是原生提交边界，不是账号服务；双账号结论只适用于实际受控工作流/UI路径。旧 context 与 approval schema 不增加账号字段。

Root 审计事件 fleet.capability.review.result 记录操作、理由、原收据、账号依据和提交回执。账号依据与可编辑角色审计上下文分开；本地 append 确认不是持久化、签名、跨站授权或完整防篡改历史。审计失败不回滚能力变更。

## 未执行事项与剩余范围

仅源码接线和限定静态交叉阅读；没有编译、运行认证/UI、测试、仿真、设备或刷机。UI 的 uiWiringPresent=true 不改变 runtimeValidated=false，原生 reviewUiIntegrated=false 仍保留。

本地批准尚无可认证的跨重启恢复/企业历史服务，不以 passport 导入恢复旧批准。完整 StartPermit、可信 HOME/local/净空、正式机载实例/结果、性能认证、真实自愈、企业跨站、长期运营和真实模型/生态等仍属原25目标的后续实质工作。

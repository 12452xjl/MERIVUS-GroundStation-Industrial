# FleetOptionalTelemetryConfiguration：独立授权的地面可选遥测配置

## 502：地面配置已有人工可见入口

新[Panel](../../res/Merivus/FleetOptionalTelemetryPanel.md)显式采集后另行审阅，再两项确认并走不同SID二审。只实际Root返回reviewId、同当前24/gen/hash/原capture/目标才领本页资格；另一个入口或历史只读。Config projection.busy包含active，故本页审批／取消不借它自锁，实际native最终围栏不放宽。

本服务／Session／Role／Vehicle源码与15／29／9／32／24／5合同不变；新Root值视图仅uiWiringPresent及operatorIdentity，runtimeValidated=false。UI显示原30秒、已发／ACK／回读和死源历史，取消只停后续、不续期／重试／回滚。下方501“尚无可见按钮”为历史状态，不代表目标14或实际率已验证。本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 下方为历史。

## 501 当前范围

501新增独立双账号的地面可选遥测配置：从未来新成功Capture的三条真实native上下文封存不可序列化Basis，在同一最早30秒内独立审阅／批准，再逐项511→510并最终三项510复核。仅固定62／74／83、同一正int32且精确REAL32间隔，目标不得比任一原正基线更短；不启停流、不降频P0、不改视频或编队速度。

本服务仅由实际SwarmController私有构造／持有，核actual Configuration／Capture／Registry三对象、parent与线程。无可任意QML构造注册，无新qrc或依赖，无新可见Apply按钮。Root提供显式request／approve／cancel后台桥接与只读投影，不自动把链路策略变成配置。

## 从真实Capture取得一次Basis

未来新成功的[FleetOptionalTelemetryCapture](FleetOptionalTelemetryCapture.md)在释放三原native query前调用Vehicle seal。Basis只定义在Vehicle.cc，非metatype／invokable／JSON；公开15键摘要或历史UUID不能重建。三原query必须来自actual Capture、固定顺序62／74／83、同来源／version／transport／mutation／ledger，原ACK与244均终结且current；每条只seal一次。

实际Configuration一次接管前烧handoff。请求被拒、审批失败、取消或到期都不恢复原Basis；旧500已经release的Capture需重新真实采集。配置要求三条均为正基线，0或−1虽可作为Capture诊断却不能配置。

Capture的纯sourceFence保原generation／request／实际物理绑定／Root／当前Episode／Role／最早期限，不借旧Capture active和旧telemetry行阻断本Run自己的511。外部换源、Role变更、原Capture换代仍退休。handoff后正常审批通知不等于换身份，实际身份／WTS／政策由新配置proof独立核验。

## 两次明确操作与用途隔离

1. requestReview(actualSession, actualRoles, actualRoot, scopeToken, captureId, intervalUs, confirmed)：确认原三基线、统一目标及当前来源，认领一次Basis并冻结bindingHash。
2. approveAndApply(reviewId, confirmed)：另行确认、专用真实OS二审，typed消费后只认领native ready；Root先持有批准返回结果，100ms下一轮才可能发送。
3. cancel(reviewId)：精确停止原工作流，不撤回已写字节，不自动回滚或恢复凭据。

Role action为fleet.telemetry.interval.configure，强制二审；默认申请角色operator／maintenance／commander，administrator沿用*。capture读取、上传、启动、能力及构型批准不能替代配置用途。Session purpose精确fleet.telemetry.interval.configure.approve，冻结actual owner／primary／approver／hash，要求不同SID及合格审批角色。不同SID不等于证明两名自然人。

Session私有typed proof一次消费且只供该actual owner，实际wire前live核OS账户／SAM角色／WTS，最终pure尾核原内存身份与期限。Session换代／到期／撤销清proof；公开摘要不输出凭据或SID，不形成上传／启动lease。见[Session](FleetOperatorSession.md)、[Role](FleetRolePolicy.md)。

## 唯一硬期限与九个wire阶段

总截止沿最初Capture最早入口固定30秒，采集、人工审阅、OS二审及所有配置／回读均计入。服务／native时钟换算不能越过原Capture pure fence；Session的主身份、二审及私有proof截止只可进一步缩短。

| 阶段 | 请求 | 通过条件 |
| --- | --- | --- |
| 0／1 | 511设置62，510回读62 | 原511 accepted ACK，再有510 ACK＋244且报告目标值 |
| 2／3 | 511设置74，510回读74 | 同上 |
| 4／5 | 511设置83，510回读83 | 同上 |
| 6／7／8 | 再510读取62／74／83 | 三项均属最后own mutation和同一ledger、语义仍current |
| 9 | 全部阶段通过后的本地终态，不发送 | reported-values-match；失败可停在任一原阶段 |

共三次511和六次510，每条最多4秒；510 accepted ACK后最多1秒、且只缩原4秒。九条最坏等待超过30秒，人工审批还消耗剩余，因此不保证慢链路完整成功；到原期限即停并保partial，不续期／重试／回滚。RAW栈忙只核原截止并等待，不查OS、不发下一条，也不把暂时updating误判成永久换源。

## 实际发送与来源连续性

- 510／511／512共用Vehicle lane；Run活动时拒同Vehicle外来三类请求，不以此阻断其它安全飞行动作。
- 511只在原Run／当前行UUID的最终actual write前推进一次不可回绕mutation并全清旧账本；尚未write的拒绝不冒称配置已发。
- 只有该Run的精确own mutation可推进expectedMutation和下一明确同源ledger bind；外部mutation、unknown clear、version／transport／PRIMARY变化不能自动重基。
- Link／plugin／账号回调之后重核原QQ、弱控制块与实指针、完整载荷、实际地面／未解锁／参数／任务传输条件；最终write前另核独立两账号fence。
- 510必须原ACK accepted和原物理来源的244双端齐全。receipt晚于最终write前基线；准备期包不算，无本地nonce回显，不能认证wire因果。
- 后续511会使先前中间回读不current，这是历史而不是错误续证；成功必须最后三项新回读同最后mutation／ledger，A→B→A不得复活旧语义。

## 固定值合同

| 输出 | 形状 |
| --- | --- |
| requestReview／approveAndApply返回 | 5键：ok、requestAccepted、reviewId、reason、flightCommandReleased |
| Configuration current | 24键：原scope／capture／review／generation、阶段／期限、Basis／Run摘要、消费及partial事实与未证明旗标 |
| Basis摘要 | 15键，含3行各3键：messageId、intervalUs、contentRevision；非可恢复权限 |
| Run摘要 | 29键，固定3行各9键；原／期待mutation、ledger、stage和write／ACK／中间及最终query |
| Run每行 | messageId、originalIntervalUs、setRequestId、setRequestIssued、setAckObserved、setAckResult、readbackQuery、finalQuery、reportedValueMatches |
| native query | 原32键；未开始为空Map，ACK未知为null，不用0冒充已accepted |

service current／Basis／Run的snapshotOnly保持true；query32沿原readOnly合同，5键接纳回执不含snapshotOnly。service revalidationRequired保持true。对应authenticated、responseAttributionVerified、actualTransmitRateVerified、availableBandwidthVerified、configurationPermitted、executionPermit、flightCommandReleased、permissionsRestored按各合同保持false。authenticationConsumed只记该次实际消费事实，不授调用者权限。

Root私有intent固定actual scope／primary5／Role9／capture／review／binding。preparing提前可见ID仅能取消，不能当已接纳批准；scope切换后旧workflow标history。真正审批busy只保已有同身份Role显示，既不新增Role也不替代native实际双账号write fence。

## 终止、部分事实与剩余工作

失败、取消、来源失效、原截止、close／prepareDelete／owner销毁均停止后续阶段。保留已可能write、原ACK及回读事实，精确清原Run／query／lane，不触碰新owner；同步取消发生在native begin内，返回后只补同旧generation历史，不污染新review。configurationRequestsMayRemainInflight是保守提示，不是远端收到／应用证明。

若Vehicle在实际write或messagesSentChanged回调中同步销毁，非invokable static fleetOptionalTelemetryConfigurationRetiredSnapshot只允许已死Vehicle的原actual owner读取已持有opaque Run；不查OS／Link、不执行fence或发包。仍输出原29／9／32，保issued／ACK／report／计数，active／sourceCurrent／reportedMatches／许可false、remainingMs为0且标明销毁历史。owned510在任何外调／write前即把真实query挂Run，避免begin尚未返回就失去子请求事实。Config在start／tick／finish三处仅把同generation／reviewId／Run的历史合并并累积mayRemainInflight，不借历史恢复活动状态或批准。

成功名为reported-values-match，只表示该批回读报告值满足目标；ACK和244不证明真实发布率、带宽收益、硬件身份或本地请求无线因果。没有新视频／速度降级、启停流、补传、飞行控制或PX4修改。显式可见操作界面、真实测量／效果验证及完整目标14继续待开发。

[精确修改清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[46项全部未执行的工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](../../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。

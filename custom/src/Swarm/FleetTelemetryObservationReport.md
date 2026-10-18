# 遥测历史观察资料报告

## 487：真实原页到工程卷宗的独立合同

[ReportBuilder](FleetMissionReportBuilder.md)非invokable static buildTelemetryReview(nativePage,positionValuesConfirmed)，由[Archive](FleetTelemetryArchive.md)现有review实际调用。内部只一次MotionReview，保其完整31；不以外来摘要、机号或时间匹配任务，不调用旧mission build。没有新观察者、文件、后台自动处理、身份或飞行接口。

顶层exact21：schemaVersion(1)、source(merivus-fleet-telemetry-observation-report)、ok、reason、dossierId、contentHash、sourceIdentity、coverage、dataQualityFindings、motionReview、views，historicalOnly／readOnly true，authenticated／signatureVerified／taskAssociationVerified／completionVerified／executionPermit／flightCommandReleased／permissionsRestored／runtimeValidated false。成功reason空；失败同21、主要map/list空，无成功prefix。

sourceIdentity exact6为queryId／catalogId／runId／fileHash／afterSequence／nextSequence，均原页实际值并与原31一致。runId仅采集批次，不是任务run。coverage exact8为totalValidatedFrames（完整十进制string）／returnedFrameCount（Int）／positionWithheldFrames（完整十进制string）／hasMore／footerPresent／partialTail／fullTelemetryCaptured(false)／closureVerified(false)。文件／页／隐私隐藏计数范围不同，不相减推丢包；无下一页或有Footer也不认证完整飞行。

dataQualityFindings最多16个exact3{code,count,detail}；count是实际聚合Int0..256，或文字说明为1项资料限制，不截断大文件级计数。包含单页、任务与来源未验证、不足样本／拒绝样本／boot或receive断段／mask变化／单样本段／partialTail／缺Footer／后续页／隐私隐藏，以及非首页。仅标资料质量，不按阈值诊断风、重心、桨叶、调参、事故、风险或性能。

views exact4 commander／customer／engineer／safety；每项exact6为audience／headline／sections／limitations／historicalOnly(true)／flightCommandReleased(false)。sections≤8个exact3{code,title,text}，limitations≤16字符串。实际填入本页聚合帧／样本／拒绝／分组／断段／分页和资料限制，不是只有模板；customer子视图无文件身份、hash或逐机值；完整原31仅在顶层供工程核对。视角不是登录或授权边界，不产生安全通过结论。

contentHash为整份body移除dossierId/contentHash后的Qt compact JSON SHA256，小写64hex；dossierId为telemetry-observation-加该hash。没有generatedAt／当前时钟，不把hash称签名或来源认证。Archive另重核hash／结构／原页来源后成对发布；整体≤1MiB，超限不截31。跨工具交接应按约定Qt正文值重建序列化，不把任意排版JSON字节hash等同本内容标识；人工复制／分享仍需核完整性及对象。

新格式不含旧任务reportId/boundaries组合，旧view/export/save/load不直接接纳它；新链不调用旧build、不更新_lastReport／飞前missionReport／实际Run结果。旧build仍有通用Map输入，不新增类型门，不能把它当dossier拒绝器，也不恢复批准／权限。原[UI](FleetTelemetryHistoryPanel.md)二次明确位置确认后只读展示，可人工复制，没有自动保存／剪贴板写入／上传或发送。签名、报告长期存储、企业权限和真实任务关联／正式结果仍未完成。

34工程场景全部未执行；本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。

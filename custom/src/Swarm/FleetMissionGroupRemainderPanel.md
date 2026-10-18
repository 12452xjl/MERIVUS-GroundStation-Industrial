# 完整原组的联合人工余段编辑（475，源码静态交接）

## 512 当前：联合余段任务显式重建备用保障

原来的三键联合请求会保留旧reservePlans；换新执行机／备用机而描述仍指旧ID时，Compiler会有意阻断。这是此前尚未开发的业务边界，不是允许通过简单替换数字解除的保护。本轮在原面板增加显式的新备用配置输入，让操作员确认一个全新的运行方案，再进入现有完整当前审阅。

### 输入、预览与原生命周期

面板现在发送exact4 `{members,reserveVehicleIds,reservePlans,operatorConfirmed:true}`。每个新备用ID都必须恰有一个exact4描述 `{vehicleId,start,returnTo,reservePlan}`；起点／返点各为exact5的geodetic／amsl数值，不从旧备用、主机端点、HOME或当前机位代填。新替补对象必须在本次新执行成员中。没有备用时ID和描述两个列表均显式为[]，不能据此取消原最低备用数量要求。

reservePlans JSON文字最多32768字符，列表最多32项。提交提案时校验完整字段、数值、唯一性和关联，再按明确备用ID顺序排列；不是每次按键就解析或触发后端。native Planner仍独立执行闭合校验和原资源预算。Root按字段存在性选择新四键合同，undefined／null／false／Map不能丢键降级；原三键及single请求仍保原合同。

每项reservePlan复用原八键策略：schemaVersion=1、replacementVehicleId、standbyMode、standbyDurationSeconds、standbyConsumptionFactor及cruiseSpeedMetersPerSecond／climbSpeedMetersPerSecond／descentSpeedMetersPerSecond。待机方式为ground-disarmed或airborne-hold，时长0..86400秒，耗能系数大于0且不超过10，三种速度大于0且不超过1000m/s；数值可填写不等于当前飞机具备该能力。

例如下面是人工新填的描述形状，不是系统默认值、真实飞机状态或可执行指令：

```json
[{
  "vehicleId": 5,
  "start": {"coordinateSystem":"geodetic","altitudeReference":"amsl","latitude":30.1,"longitude":120.1,"altitude":100},
  "returnTo": {"coordinateSystem":"geodetic","altitudeReference":"amsl","latitude":30.1,"longitude":120.1,"altitude":100},
  "reservePlan": {
    "schemaVersion":1,"replacementVehicleId":4,"standbyMode":"ground-disarmed",
    "standbyDurationSeconds":120,"standbyConsumptionFactor":0.1,
    "cruiseSpeedMetersPerSecond":10,"climbSpeedMetersPerSecond":2,"descentSpeedMetersPerSecond":2
  }
}]
```

操作员必须明确确认替换全部备用运行配置；不能说旧待机／速度值仍全部不变。原最低备用数、其他共同安全／能力／电量／返航约束继续保留。提案预览从同一次后端成功返回的完整provenance值副本逐项显示新ID→替补目标、自身起返点、待机和速度，不根据未提交文本、旧summary或当前机位补参数。

新字段编辑沿原epoch退休提案／审阅确认；按下到点击来源、capture token、局部票据、当前原包、取消、隐藏和明确退出的合同不变。新JSON输入沿原主题、PlainText、可选择文字、可见标签、键盘焦点与原反馈区；完整请求校验及文字通知可能同步销毁面板，perform在反馈路径前冻结plain lifetime并在返回后先核alive，新的输入赋值通知之后也不访问死面板。UI/UX技能只静态应用这些规则，未搜索技能脚本、运行QML、预览、布局或可访问性验证。

### 原生完整来源与真实业务消费者

新provenance schema3/exact28保完整canonical reservePlans；旧1／2和三键请求不自动升级或获得新语义。sourceTaskPolicyHash仍指改前共同政策（含旧备用配置）；derivedTaskHash与newRoutesHash绑定显式新方案。任务匹配与compiled-plan匹配分开：后者实际核现Compiler输出的reserveMembers.energyPlan，不要求不存在的plan.reservePlans，也不能用任务投影逃避编译输出核对。完整预算／路径仍由原[备用算法](FleetReserveEnergyPlan.md)、Compiler→ReviewCoordinator→EnergyScheduler生成并消费，当前能力／电池资料仍须新读取审阅。

本轮不自动改旧clearanceInput的成员、坐标、高程或接触资料；换机后资料不适用仍应阻断。不恢复旧Run、占用、批准、账号或控制租约；新预算通过不证明实际待命、已替补、机载改派、真实耗能模型或StartPermit。原result7、summary18、32MiB源／结果、4MiB请求、64KiBprovenance、16MiB任务及生成路线预算均保持，不截断有效前缀。

见[联合规划器合同](FleetMissionGroupRemainderPlanner.md)、[本批修改清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[48项全部未执行工程交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本批仅源码及直接静态阅读／普通文本核对；未编译、执行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现、测试、仿真、设备／PX4操作、刷机、放行、提交、更新PR或打包。原25目标／六类实质剩余保持，约40%／60%仍仅源码工作量粗估。下方为先前合同历史，新四键输入以本节为准。

新 `FleetMissionGroupRemainderPanel` 为既有Planner中的独立联合入口，沿现有QGC控件／主题；不代替实际启动、空中改派或跨站租约。上方原资料入口负责打开／保存v2包；本面板只显式选择已完整验证的当前私有包，再逐成员编辑一个全新联合任务。v1包不能补造整组。

## 实际后端链

Root的 `captureFleetMissionGroupRemainderArchive(archiveToken,true)` 核精确v2槽和当前destination scope，退休旧本地审阅后保同一完整包；kind=group的私有capture不恢复Run、占用、批准或远端机载状态。公开成员只提供编辑所需原ID／角色／路线及有限观察提示；完整task／payload仍在私有包。

`previewFleetMissionGroupRemainder(captureToken,request)` 调用Coordinator薄口与[纯联合规划器](FleetMissionGroupRemainderPlanner.md)，输出一个含全部assignment的新task及独立group provenance；和single预览在修改旧提案前核对kind。每原成员必须同序恰一次，新执行机／备用机唯一，任何未提交或不支持成员整体拒，不静默排除。

请求exact3：`{members,reserveVehicleIds,operatorConfirmed:true}`。每行exact10为 `sourceVehicleId` 加原single request9：原索引、新目标、自身新起返点、三速度、新profile、独立行确认。11个数值编辑字段组成上述规范对象，profile最多16384字符，备用IDs为明确JSON数组；不从Home、旧profile、实时机位或其它成员默认代填。每成员编辑文字独立保存，切换成员不丢输入；字段改变退休原提案确认。

一次 `reviewFleetMissionRemainder(proposalToken,true)` 对私有完整联合task调用一次原 `prepareFleetIntent`，不循环覆盖几次单机审阅。全部当前能力、逐机速度／能量、空域、风险与版本化归档沿既有链；异步完成只在同capture／proposal／destination scope仍对应时发布当前结果，已发生归档与是否当前分开。

Root的新reviewContext.kind=group，与single采用同一精确binding／记录／当前性／本地批准和唯一共享部署区。UI完整显示新目标组和备用ID，独立默认未勾选后才采用；blocked审阅可作为显示来源，但采用不等于通过，批准仍需原门禁。只剩一个明确共享部署源，不向每架独立发送命令。

## 全部成员的显示

[GroupView](FleetMissionRemainderView.md)根schema2/exact12，每成员exact7；完整核全组几何后才发members。单次PayloadBuilder完整匹配全部目标，编码任一失败时所有成员均不返回成功编码前缀，已全验几何可保留。面板有全组摘要、逐成员原点和新出航／返程／协议分页，每页8条，不把第一机显示称整组，也不把协议sequence等同原航点索引。

原路线和完整停留是规划需求，不是已完成证明。AMSL不是离地高，几何与常速估算不证明感知覆盖、动力学、障碍净空或性能认证。原绝对时序与净空声明不得自动降级、裁剪或重绑；无法忠实对应时新审阅仍阻断。

## 确认、取消和退出

- 意图冻结按下时controller、输入代、Episode、包、capture、proposal、审阅版本及完整目标组；输入／Episode变化与ABA会退休旧确认。busy通知后复核原意图，不调用包含自身busy的ready尾核误拒正常操作。
- 独立可通知callTicket负责ready绑定；plain lifetime.ticket保留到busyfalse及callTicketnull所有通知结束，阻止重入，避免普通对象字段清空不通知导致按钮永远禁用。
- 取消冻结原requestId和pending proposal，在调用前再核同capture／owner／输入代；不会读取点击时live新请求并误取消它。取消只退本地计算，不取消机载任务。
- 单／组编辑各自排除自己的pending并互斥。旧single区过滤group状态，不显示或取消联合后台；联合区不采用single审阅。
- 收起／隐藏退休确认、保留本地编辑和文字；明确退出可在其它输入未结束时执行，但仍冻结原来源。退出只清精确本地capture／review，不删归档、不关Run、不释放占用、不取消飞机动作。
- 反馈／焦点只在仍属原上下文时请求可见；主题／长文／键盘及实际焦点行为尚未运行验证。

源码／资源登记为 `custom.pri`、qrc alias及qmldir；不新增QObject服务注册或第三方依赖。28项工程师待验证场景见[交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

本批仅地面站源码、工程／资源登记与交接材料，核对限直接静态阅读、普通文本／路径／Markdown目标／git空白；未编译、执行helper／parser／AST／业务脚本／QML／线程／UI／认证／SQL／应用或实际文件读写／hash，未测试、仿真、修改PX4、刷机、放行、提交、更新PR或打包。原25目标及六类实质开发保持，整体约40%仍仅源码工作量粗估，不按批次或文件数上调。下方为历史。

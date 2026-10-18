# 实际部署启动工作台

## 442：同一原记录的联合名义空间报告

公开fleetStartupSpatialReview当前准确九字段：captureToken／sourceToken／binding／current／mapSelected／view／missionReview／jointReview／error。原启动view15与完整任务missionReview3不改，jointReview另为独立ok／error／view。新[联合helper](FleetMissionJointSpatialReview.md)仅消费本次两个已冻结投影，不重建或拆分原plan，不查当前地图或补HOME。

Root仍在同一runCurrent内核实际Episode／Session身份、原审阅指纹、精确record／完整binding及有序整组目标；新报告planId／reviewKind／reviewId／reviewFingerprint与逐机顺序须相同，尾部再次核原来源与store epoch。任一步换源不发布混合capture。失败联合结果明确保留，不取旧成功替代；其计算结果不进入审批或发送权限。

完整任务与联合值共用只随publication重建的缓存，并以captureToken首尾配对，防绑定求值顺序将旧报告接新捕获；current年龄变化不逐次复制全部段。原显式地图token选择及隔离保持，Map只兼容新增信封字段，不根据新报告自动绘线或切换来源。

联合报告检查本地canonical请求链（含出航／返程连接、TAKEOFF连接、HOLD锚点／LAND下降）与独立启动假设。原归档空域报告scope不改写；旧空／缺输入保持未知，无阈值不借默认5米。没有权威空域、实际航迹／净空认证、最终StartPermit或正式结果。未编译、测试或运行；下方440八字段及439七字段为历史。

## 440：同一捕获新增原完整任务审阅

公开`fleetStartupSpatialReview`由439七字段升级为八字段，新增`missionReview:{ok,error,view}`；原`view`仍是439独立起飞几何，不能合并成已证明航迹。新[完整任务](FleetMissionSpatialReview.md)直接消费同一准确record的plan／reviewEvidence，原整plan只生成一次本地载荷，成功时另核全部成员顺序及planId与原binding一致。

失败的完整视图保留明确错误，不用当前草稿／单机合成plan补缺；独立起飞几何仍可查看，但界面不能称整组完整任务已审完。完整脱离值只随捕获／选择publication重建，避免每次实时current性通知拷贝全部协议项；该公开值仅用于显示，原私有绑定和全部权限门禁不消费它。

新归档另外记录原noFlyZones是否提供及真实List类型／长度；列表判断经Coordinator纯值接口在原生QVariant边界完成，不把合法Qt列表包装因JS Array.isArray结果而误标无效。原空域报告按准确版本展示，不升级scope或重算成净空许可。两个明确capture／select接口及历史查看／旧按压隔离保持。未编译、测试或运行。下方439七字段为历史接口形状。

## 439：冻结启动空间的独立只读来源

[空间投影](FleetMissionStartupSpatialReview.md)与[空间面板](FleetMissionStartupSpatialPanel.md)使用同一份明确捕获的原生风险几何；不会用当前草稿、实时Vehicle或最新规划补缺。当前位置到首NAV_WAYPOINT、名义垂直起飞、垂直起飞后到NAV_TAKEOFF请求点是三个独立提示，不拼成已证明的执行航迹。

Root公开`fleetStartupSpatialReview`准确七字段：`captureToken/sourceToken/binding/current/mapSelected/view/error`。没有捕获时view为空；有捕获后保持冻结值，原审阅过期／换源／换身份只使current=false，不自动采用新审阅或旧路线。

`prepareFleetStartupSpatialReview(kind,expectedWorkbenchSnapshot,confirmed)`接受normal／ground、工作台准确三字段snapshot和明确true。在原runCurrent范围内核同一实际Episode、身份、部署binding、risk reviewId／fingerprint／状态／期限；按完整binding读取精确归档记录，取得其声明的minimumSeparationMeters，不借默认5m，也不拿最新草稿阈值。记录不是禁飞区／地形数据。

只把有序整组真实vehicleId和readinessSummary送入Coordinator的纯值入口，前后核同一来源及显示版本；绑定的record ID／revision／plan／content／review hash和store epoch不能互换。原7／9字段binding完整保留；记录匹配核原6个内容身份及store epoch，不要求记录包含UI额外两代次。

`selectFleetStartupSpatialMap(captureToken,enabled,confirmed)`仅选择／退出准确冻结token的地图图层；明确允许查看已过期历史，始终标明current=false，不恢复原审阅、已阅、批准或权限。新捕获会先退出旧地图选择，必须再次明确选新token。两个方法均返回`{ok,error,token,current}`；后置范围失效不删除已经生成的本地token，调用不自动重试或恢复权限。

[名义空间接近候选](FleetAirspaceConflictChecker.md)逐对比较独立线段，输入间距必须明确；时间、地形、禁飞区、动力学和净空仍未知。报告的局部平面投影域／缺口、全比较数量和有界候选明细分别展示，不能据candidateCount=0称安全。二维地图垂直段用位置标记与AMSL高度明细，不把不可见的竖线称已检查。

本批只新增只读捕获／几何／地图，不修改原20动作或发送门禁；未编译、测试、运行或放行。完整原任务航线／返程／结束动作与禁飞区的共同冻结空间报告、可信原点／净空及最终StartPermit仍待开发。

## 437：当前部署的逐步操作与真实提交事实

[FleetMissionStartupPanel](FleetMissionStartupPanel.md)接入指挥中心；沿用现有的内容核对、风险审阅、独立账号审批、认证移交和三步事务后端。每次只执行操作员明确选择并确认的一步，不自动串联。原部署页的内容核对和风险生成入口保留，两个界面消费同一实际对象。

本批为原目标6／13的源码接线，未编译、测试、运行UI／认证／应用／设备或刷机。条件认证允许实际发送，但最终StartPermit、可信HOME／原点、起降净空和正式任务结果仍未实现完整认证；界面不得把条件审批解释为这些要求已满足。

## 当前来源与值投影

`GuidedActionsController.fleetStartupWorkbench`从实际current-outbound Episode一次取源，不采用selected-observation或旧Run。公开脱离Qt包装的JSON值，不公开Session、Authority、Transaction或认证凭据QObject。

投影包含`sourceKnown/sourceToken/identityToken/identity/deployment/contentCheck/review/approval/authority/transaction/groundReview/groundRisk/groundApproval/groundAuthority/groundTransaction/series/snapshot`。身份展示是真实Session当前收据；角色显示不单独授权。部署保留ID、generation、state、binding、从实际vehicles提取的目标及占用／排空标记。地面事务取同一Series实际最新consumer的完整current，并核同一对象及原摘要ID／代次／状态；不把缺少逐机步骤的consumerSnapshot当完整事务，不回退初始事务。

`snapshot`准确三字段`{sourceToken,identityToken,stamp}`。stamp为最多64KiB的JSON文本，绑定各阶段ID／代次／状态／来源指纹、完整binding、目标集合和事务逐机步骤事实。递减的remainingMs和remainingLifetimeMs只供显示，不每毫秒撤销确认；过期状态或有效性改变仍进入版本检查。UI在按下时冻结snapshot，点击后再次核对；显示版本变化需重新选择并确认。

## 单步调用合同

`performFleetStartupAction(action,expected,operatorConfirmed)`只接受明确true、准确三字段snapshot和以下固定动作。操作参数从原实际Episode的冻结投影取出，不接受手填UUID或任意方法名：

| 动作 | 原后端用途 |
| --- | --- |
| content-read／content-cancel | 请求／取消整组任务内容只读核对 |
| review-create／review-ack／review-revoke | 生成风险、明确已阅、撤销审阅 |
| approval-request／approval-revoke | 请求实际独立账号条件审批／撤销 |
| authority-request／authority-revoke | 一次性原生认证移交／撤销 |
| start／cancel | 请求／取消初次Seq0、普通ARM、MISSION事务 |
| ground-read／ground-ack／ground-cancel | 最新实际事务的地面新读取、新风险已阅／取消 |
| ground-approval-request／ground-approval-revoke | 新地面用途双账号审批／撤销 |
| ground-authority-request／ground-authority-revoke | 新地面用途认证移交／撤销 |
| ground-start／ground-stop | 由Series创建独立新consumer并请求／取消事务 |

Root先设置私有重入锁，再进入现有`runCurrent`原生调用范围；在该范围内调用原`ForScope`实现，不嵌套另开控制器调用范围。操作前重新捕获并严格比较显示snapshot，原生各用途会话、角色、期限、来源、健康与发送门禁继续执行。

返回准确七字段`{ok,reason,invoked,accepted,current,flightCommandReleased,result}`。`invoked`只表示已经进入原操作；`accepted`为true／false／null，null表示未得到可信原生返回。`result`保留原返回（bool规范成`{ok:bool}`）。`accepted=true`只说明该步被接纳，不证明任务成功。`flightCommandReleased`只有原返回明确boolean才提供，否则null；不能从bool取消结果推断之前没有发出命令。

原生调用后才丢失来源、账号或控制器范围时，ok可为false，已收到的accepted及原提交事实仍保留。调用中断／结果未知应查看实际逐机状态，不能自动重试。`current`仅表示操作返回时仍为原Episode和身份上下文；不是遥测新鲜或执行许可。

## 风险与取消的含义

初次review展示原冻结风险工件；地面`riskSummary`由原已捕获风险白名单生成，保留逐机currentRisk、健康门禁及未证明事项，不暴露原账号凭据或任务载荷。原生已阅仍重新检查同一来源及期限，阅读UI不是原生授权收据。

真正启动按钮会请求真实Seq0、普通ARM（param2=0）和MISSION模式事务。Seq0本身改变机载任务索引；即使flightCommandReleased=false，也不能称完全没有机载变更。没有新增强制ARM、自动串行启动、自动重试或MISSION_START路径。

逐机三步分别显示requestIssued／rawStateObserved／mayHaveCommitted及地面再审已解锁跳过标记。started-observed只表示观察到规定状态，不是已完成任务。取消阻止后续发送、撤销旧认证并等待排空；不会自动DISARM、降落或回滚已发命令。存在或可能存在MODE提交时，不能靠取消释放占用并重新启动。

地面续审必须采用Series最新actual owner、新整组读取、新风险、新双账号凭据和独立新事务；容量最多8个地面consumer，不恢复旧批准。失源、排空、仍被占用、资格未知或容量耗尽均按原生原因处理。

待工程师验证项目见[交接说明](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)，本批路径见[补丁清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)。

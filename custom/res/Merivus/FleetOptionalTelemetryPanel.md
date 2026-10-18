# FleetOptionalTelemetryPanel：可选遥测的人工操作界面

## 502 当前范围

502新增FleetOptionalTelemetryPanel可见界面，在指挥中心和地图规划页复用同一actual Root。操作员显式只读采集→另行审阅统一目标→两项确认后独立账号审批→逐项配置与回读；固定62／74／83，原501实际后端和授权合同不变。不会自动采集、Apply、续期、重试或回滚。

入口分别位于CommandCenterOverlay和FleetMissionPlannerPanel的固件目录面板之后，共用各自原actual Root。qgroundcontrol.qrc和同目录qmldir各登记一次。Root两个projection四个return补uiWiringPresent=true，并为配置提供现有账号值视图；原两request／source围栏另接本次调用的可选撤销态，runtimeValidated=false。原500／501原生发送、Session、Role及native输出合同不改。

## 三步操作，不自动串联

1. 显式认证本地操作员，输入MAVLink System ID并勾选只读确认，按按钮启动新的62→74→83采集。
2. 三正基线均完整后，输入统一正int32且REAL32精确的微秒间隔，目标不得小于任一原基线；另勾选建立审阅，不发送511。
3. 核对机号、原三基线、统一目标和不回滚边界，两项新确认后由不同SID实际独立审批，再由原后端逐项配置／回读。

没有从Component、打开、状态变化、勾选或显示计时器自动发起操作。配置仍不启停流、不降频P0、不改视频或编队速度。申请角色与原Role一致，真实OS提示／purpose／typed proof及最终write门不由UI替代。不同SID不证明两名自然人。

## 值视图与本页归属

只接actual Root公开的capture／configuration值投影和显式函数，不拿Swarm／Session／原生服务指针或setter。capture和configuration的workflow都保持24键，实际返回均为5键；只在返回ok／requestAccepted、原UUID、当前同机号／scope／代次／capture和hash均一致后领用。本页receipt来自实际返回，不从共享workflow或preparing可见ID恢复。

只读共享状态／历史始终可查看，但没有本页真实回执就不能审批或取消它。采集成功terminal仍保本页ID供后续独立审阅；失败／取消后的opaque Basis不被UI复用，须按原native拒绝原因重新采集。

完整sourceToken包含自己的ID／代次／状态，不能变化就取消。稳定context仅取原scope、主身份5和读取／配置双政策9；这些真正变化才退休本页。完整token仍在每个新请求前精确比较，stable context不构成权限。Config busy包含本审阅active，approve／cancel不用它自拒，仍严格核本页review／hash／state和实际后端许可。

## 取消、通知与晚返回

每次按下冻结原controller／view／epoch／稳定来源／输入／原回执及相应完整token，点击只使用该次明确确认。capture在首个旧操作清理通知前持原ticket，且只允许自身一次epoch+1；额外取消、输入或来源ABA不能借当前新epoch继续旧意图。

retire先清本地JS拥有资格，再清回执和勾选；两条原owner／ID取消责任留在独立finally闭包。任一属性通知、清勾选或首cancel销毁面板，也必须尝试两个原取消，不触碰新owner。activeCall跨callBusy=false通知保留到尾核，方法入口直接拒重入；按钮状态不依赖无通知的嵌套JS字段而永久自锁。

关闭／隐藏／controller更换／销毁或真正来源变化退休本页并取消原ID。改机号须重新采集；改目标间隔仅退休原配置审阅并清确认，不把它当新的采集期限。异步请求尚未返回时，本页最早将该次新cancelState.cancelled置true；Root仅持该实际调用的同一引用，入场／source围栏看到撤销即sticky退休原intent，不借公开token寻找他人，也不由false授新权。capture仅自身第一次清旧操作豁免精确新ticket，额外retire仍撤销。实际返回后仍用冻结原controller取消自己的ID，不重新领权。Root旧三／四参数调用保持，新增可选capture第4／configuration第5参数只用于撤销。

已发／ACK／中间及最终回读／dead-Vehicle历史通过共享native值只读展示；取消只阻止后续，不代表字节撤回或远端回滚。面板销毁不承诺其局部UI数据持久化，原后台历史与其它归档模块各守原合同。

## 期限与结果语义

Capture终态的remainingMs=0并不重开新期限。本页显示从自己最早capture调用前取时+30秒开始，采集／审阅／独立审批都计入；它只可保守提前撤销，最终仍受原native单调期限制。显示时钟后退取消，前跳只提前到期。九个wire阶段最坏等待超过总期，不能保证慢链路或长人工审批能完整成功。

reported-values-match始终显示“报告值匹配（非实际频率验证）”。字段认证／执行许可保持原native false；uiWiringPresent只说明代码接线，不代表Qt加载、实际传输或工程验收。

配置终态后后端保存的是该次Run快照，不持续重核其中sourceCurrent／reportedValuesMatch。界面因此明确标“该次快照采集时来源检查／该次报告值匹配（均非持续核验）”，即便值为true也不称当前live或持续可用。

## 交互与可访问性静态设计

沿用QGC原生控件、Palette和ScreenTools；没有外部字体／图标／动画依赖。长按钮覆盖受限宽PlainText标签并换行，保主题／禁用反馈与焦点边框；输入有可见单位和范围，状态不是只靠颜色。错误摘要可聚焦，控件焦点滚入主Flickable；Esc／关闭可取消，Enter不默认触发批准，Space沿原生按钮press/click捕获。

详情仅显式复制当时完整JSON为只读PlainText，不在实时通知里反复重建，也不导入／恢复权限。UI/UX技能采用已完整阅读的静态参考，按用户限制未运行技能检索／预览脚本或界面；所有宽度、字体、主题对比、焦点、屏幕阅读器及OS提示行为都待工程师实际核验。

[修改清单](../../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[44项全未执行交接](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)、[静态审计](../../../docs/handoff/MERIVUS_SWARM_STATIC_AUDIT.md)、[只读采集](../../src/Swarm/FleetOptionalTelemetryCapture.md)、[独立配置后端](../../src/Swarm/FleetOptionalTelemetryConfiguration.md)。

原25目标和六类实质剩余保持；约40%完成／60%剩余仍仅源码工作量粗估，不按批次、路径或接口数量上调。完整任务执行闭环、可信性能、自愈机载改派、跨站协作、长期运营及真实模型／插件生态仍需继续开发。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。

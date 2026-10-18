# 完整计划预演界面与来源

540新增链路位于现有`GuidedActionsController.qml`和`FleetDigitalTwinPanel.qml`，没有新增飞行操作、文件写入、网络请求或执行许可。

## 捕获

点击“捕获计划”时先冻结原始九字段计划绑定、实际PlanStore/Intent/Twin对象，再发布busy。必须是当前input/review generation的完整有效绑定，且原Store仍认可。一次`record(sourceTaskId,recordRevision)`读取同时提供完整`plan`与`reviewEvidence.energyPlan`，不能把另一次review或当前电量混入。这里的本地记录关系不是远端身份、实际飞机构型或飞行批准。

实际快照接纳与preview返回后才发布所捕获来源；公开sourceCurrent另需非busy和原snapshotVersion/previewHash对应。输入或计划变化保留旧只读估算，但标历史，不自动重算或恢复批准。5秒概览刷新不会覆盖已明确捕获的计划；重新捕获只能由操作员发起。失败显示原因，旧资料不冒充本次成功。

## 显示与分页

原生[Timeline](FleetMissionPreviewTimeline.md)保留完整有界事件。界面凭当前snapshotVersion与previewHash读取，每页最多四条；换预演或owner清旧页，原生调用后先核plain lifetime再读页面对象。第一页、上一页、下一页均只读，不会发送飞机指令。

事件按成员和原路线顺序显示，不按不同飞机的相对时间拼成同步调度。每机首规划点为0；相对起止与本段时长分别显示，缺失显示“未知”而不是0。事件旁显示同一计划记录中的该机可用时长和剩余电量估算，未知标志优先，不当作当前遥测。公开说明起降、转弯、风和实际飞控动态未建模，不能作为完整任务完成时间、碰撞结论或放飞依据。

UI/UX静态指引用于文字标签、可见错误、只读区别、四条分页及随内容扩展高度；没有运行技能搜索、界面预览、编译或测试。键盘、窄窗、字体缩放及深浅主题由工程师验证。

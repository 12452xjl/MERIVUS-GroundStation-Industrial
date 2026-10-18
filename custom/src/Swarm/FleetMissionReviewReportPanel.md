# 四类飞前审阅报告界面（546）

指挥中心入口打开有限高Popup＋Flickable，保持关闭/取消可达；使用实际QGC/QtQuick.Controls2控件和主题。表单有可见路径标签、原路径覆盖确认、就地错误、键盘焦点、PlainText换行，无新增视觉系统。UI/UX技能只作静态表单/错误/状态/键盘指导，依用户禁止未运行搜索、界面预览或测试。

用户明确冻结当前已存档审阅，等本页原job/action实际终态，再领用summary.snapshotId。四类视角由后台完整预排，显示首页/下一片按原snapshot/audience分页，每片≤2048UTF16字符；GUI不解析完整32MiB报告。旧/外来任务结果只显示诊断，不自动领用；新捕获/重开期间保原已领用副本，失败后仍可明确读取/保存，不把它冒称新资料。

文件为≤4096字符本地绝对JSON路径，保存要求同一路径/来源的显式覆盖确认。按下冻结owner/epoch/job/snapshot/path，capture额外核sourceToken；路径、原job、冻结快照变化退确认，实时新审阅不撤销无关历史副本的保存确认。Controls2的pressed/canceled确实存在。按钮清按压状态及callBusy/确认字段通知后检查plain lifetime，旧释放不授予新路径确认。取消精确job，等待actual finished，不声称回滚已提交文件；关闭仅退本页确认，不冒称后台已取消。按钮/输入有44px下界，滚动条与焦点自动显露辅助长表单键盘操作，未作实际渲染验证。

保存始终导出完整工程报告，而不是所选客户视角。弹窗显著提醒完整文件含工程原值，客户显示裁剪不是脱敏/权限。显示captured-review/file-review；文件身份/生成时间是声称；verdict是原审阅建议，不是正式完成/安全放行。清除只清本地副本，不删文件、不改变审阅/批准/Run。

原[FleetMissionExecutionReportPanel](FleetMissionExecutionReportPanel.md)继续负责实际Run报告，两种格式不互换。原生API和资源预算见[工作区](FleetMissionReportWorkspace.md)。本次仅源码静态阅读；未编译、执行QML/文件往返/模型/解析/hash/测试、仿真或硬件操作。

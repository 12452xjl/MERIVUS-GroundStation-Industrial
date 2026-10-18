# 编队会话待办面板

## 542：真实入口与范围

[FleetFormationQueuePanel.qml](../../res/Merivus/FleetFormationQueuePanel.qml) 位于 [CommandCenterOverlay](../../res/Merivus/CommandCenterOverlay.qml) 的选机摘要之后、原规划按钮之前，独立于原编队启用、占用、健康和 `formationBusy` 控件显示条件。默认折叠，折叠时仍保留明确的“保存选中编队到待办”按钮；宿主整体隐藏／折叠时仍受原宿主显示规则约束。不打开新后台服务、不保存磁盘，不发送飞行命令。

保存只调用当前 `controller.queueSelectedFormation()`，固定本次原生选中机号；即使原编队正在忙也不因此禁用保存。原编队协议只接受 UAV 1–6，选中 1–6 个不同地址且必须包含 UAV 1；这些仅是待办输入，不认证实物身份、健康、空闲、控制权或飞行许可。队列 FIFO，上限 32 条包括终态，不自动删除终态以腾容量。只在本进程会话内保留，待办 30 分钟期限由原生单调时钟决定；UTC 创建／过期字符串仅作诊断显示，本页不据系统时间取得资格。

## 实际公开合同

- `formationQueueRows`：最多 32 条，逐行闭 11 字段 `id, revision, vehicleIds, state, reason, createdAt, expiresAt, canReview, canCancel, canRemove, taskId`。`id` 为规范 UUID，`revision` 为正完整 uint64 十进制字符串，永不转为 `Number`；`vehicleIds` 为 1–6 个唯一 1–6 整数，必须含 1。两个 UTC 时刻是字符串。
- `formationQueueStatus`：闭 5 字段 `count, limit, sourceCurrent, automaticDispatch, persistent`，`limit=32`，后两项固定 false。不完整／未知数据明确显示不可用，不当空队列。
- 本页只调用 `queueSelectedFormation()`、`reviewQueuedFormation(id, revision)`、`cancelQueuedFormation(id, revision)`、`removeQueuedFormation(id, revision)`；公开结果闭 `{ok, reason}`，不是原生内部的队列／review 收据。调用者不能用本页公开 row 构造原生来源。

展开每页 4 条，保原 FIFO 顺序，显示完整机号、状态、原因（最多原生允许的 4096 字符）、UTC 时刻、待办 ID／原字符串修订及关联任务 ID。`waiting` 不表示健康已允许或飞机自由；`submitted` 仅显示“已提交派发请求”，不表示飞控接受、任务启动或任务成功。`rejected` 显示“请求被拒或结果未确认”，保留原原因码；partial／unknown 可能已有部分提交，不能由失败状态推断从未发送、任务已撤销或可自动重排。

固定原因码提供中文说明，并一并保留原始“代码”；原生占用／任务占用后缀显示实际阻塞机号与任务 UUID。未知原因原样 `PlainText` 显示，不通过字符串猜测释放、成功或自动重试；中文映射只是解释，不参与资格判定。

“重新审阅并确认”只在原行 `canReview` 且当前来源可用时调用原 Guided 确认流程；本页无自动派发、自动重试或替代确认。“取消待办”只按 `canCancel`；它不是停机、结束编队、撤回已提交任务或回滚已发生事实。“移除终态记录”只按 `canRemove`。等待原确认对话框的期限／撤销和派发中不可取消等语义由实际后端决定，不能用状态文字取得资格。

## 输入、通知与生命周期

保存按下冻结原 controller、页面代次与其实际 `selectedVehicleIds` 地址集合（宿主显式绑定同一选机源）；行操作冻结原 controller、待办 ID 和完整字符串 revision。点击清 press 后先核 plain lifetime，实际调用前再核同来源／修订及当次 `can*`。控制器、选机、可见性、enabled、展开／分页变化撤销旧按下资格。只给 native 传原 ID／revision，不回传可编辑 row payload。

同步调用先装 plain ticket，再发布 `callBusy`；所有外调后先核 lifetime／原 ticket／原 controller，释放通知后只处理 plain ticket。成功回执显示对应本地事实，不把调用后的旧修订失效误当回滚，也不从共享 latest 猜成功。页面销毁或隐藏不自动取消已排待办或已进入原确认／派发的业务。

UI 使用现有 QGC 主题与文本控件，所有动态字符串 `PlainText`，原因／ID 换行；不以颜色单独表达状态。按钮明确标签、StrongFocus、主题焦点下划线、最少 44 源码高度。宿主旧左列实际裁剪且没有滚动，因此仅本新面板在两个入口按钮下使用有界竖向 `QGCFlickable`，容纳说明、最多4行和错误；分页固定在滚动区域之外，翻页／折叠归零本页滚动位置。宿主以可用高度的30%与最小字体高度给 body 上限，不改其它旧命令布局，不形成嵌套滚动。键盘聚焦尝试将实际控件滚入此区域，不自动聚焦成功回执；错误可聚焦摘要保留，不自动重试。整体宿主其它旧面板可达性及实际小屏／大字布局仍未运行验收。

登记于 [qmldir](../../res/Merivus/qmldir) 与 [custom qrc](../../qgroundcontrol.qrc)。本批按 UI 技能静态指导复用 QGC 风格；用户禁止执行，因此未运行技能搜索、QML 预览、屏读、布局／对比度／键盘／视觉验收、编译或测试。上述为源码设计，不是运行通过结论。

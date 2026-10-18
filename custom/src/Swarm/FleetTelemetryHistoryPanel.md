# 遥测档案只读审阅面板

## 523：独立GNSS历史记忆工作区

原面板在522当前页GNSS区下方嵌入[FleetGnssMemoryWorkspacePanel](FleetGnssMemoryWorkspacePanel.md)，只传实际 `archive.gnssMemory` facade、仍对应本页的reportToken及只供计数核对的reportPreview；不把报告Map交为追加authority。另一次本页追加确认后，原生从actual Archive真实token取完整报告，最多32报告／2048样本整次接纳，不淘汰、不从旧页或文件恢复当前页资格。

新工作区独立管理session冻结槽和只读file槽、显式路径与文件确认、目录每页8、逐报告完整64样本显示及另一次明确选图；不借维护护照，不导入覆盖active运营记忆，不自动追加／选槽／读首项／续页／保存或启用地图。sourceCurrent变旧仍可显式保存原固定snapshotId/hash；write、commit、fileWritten与readback事实分开，fsync／durable未验证。旧522仅本页报告与其独立确认继续保留。

地图仍共用Root精确lease及[FlyViewMap](../../res/Merivus/FlyViewMap.qml)离散点renderer，投影从522的exact4升级为exact5，新增origin区分native-page／session-snapshot／file-history；图例明确历史来源未认证，文件历史不冒充当前飞机或native页。原selectionToken精确关闭和零自动导航／连线／飞行副作用不变。多次采集runId不是可信任务实例，数字机号不认证同一物理飞机，不声称跨任务完成率。

本轮完整静读UI技能及必要参考，沿QGC主题／PlainText／最小44像素动作作静态fallback；只apply_patch与直接静读，未运行QML／JS／技能脚本／项目／解析器／helper、预览、编译或测试。工程场景均待执行，原25目标及总体工作量口径不因此完成。下方522为前一版公开地图形状，当前以本节exact5为准。

## 522：同包GNSS质量值及明确选择的离散历史地图

本次沿[原History面板](../../res/Merivus/FleetTelemetryHistoryPanel.qml)、实际[Guided窄facade](../../res/Merivus/GuidedActionsController.qml)和custom [FlyViewMap](../../res/Merivus/FlyViewMap.qml)贯通[原生GNSS报告](FleetTelemetryGnssReview.md)，不新增页面、组件资源、实时Vehicle来源或飞行API。原目录／搜索／页、Motion31和dossier21接口不变。以下为当前合同；旧节“不公开绝对坐标”的描述仅是当时运动报告／原始page的边界，不适用于本次另行明确同意的GNSS派生坐标。

先由用户选本页 `includePositionValues`，仍要求原档案Header允许位置记录及本次读盘确认；然后独立勾选“确认显示本页GNSS坐标”，只调用原exact6 request的新 `gnss-review`。不借 `reviewConfirmed`，不自动读盘／续页／选地图。按压冻结原owner／options／inputEpoch／consentEpoch／完整intent；plain ticket覆盖确认消费、native调用、报告采用及callBusy通知尾。独立GNSS确认也计入不回绕consent代次，false→true不能复活旧按压。

仅在本界面实际成功页仍当前时采用完整top33／sourceIdentity7／displayPolicy3／row18；identity逐项等于savedPage的query／catalog／run／headerHash／fileHash／after／next，页三个尾部状态和帧数对应。每个GPS24样本的sequence／vehicle／component／messageId与实际公开page.rows核对；该公开row6本身没有UTC、epoch或sourceSequence，不能伪称从它们二次核得。原生派生报告已完成整页raw与来源核验，QML只做值与当前来源关联，不从Map恢复原生资格。

row按原页有符号64位sequence递增；sourceSequence独立接受全正uint64规范字符串（最多18446744073709551615），按同sourceEpoch／vehicle／component分组严格递增，不转JS Number。组序号按本页首次出现顺序核；总计、可定位／不定位、三项定位质量计数以及所有固定false边界闭合。异常整份拒绝，不展示成功前缀；合法无GPS页明确显示0，不当故障不存在。

列表最多64条，逐条显示本地接收UTC、来源epoch／序号、机号／组件、fixType、同包reported经纬度或拒绝原因、HDOP及卫星数。fix仅3／4／5／6／8且原经纬度合法才定位，(0,0)仍合法；无fix、2D、STATIC、未知fix或越界不借旧坐标。HDOP=原eph/100，无单位且**不是米**；eph65535、卫星255用独立unknown，真实0保留。HDOP>2.5或已知卫星<6仅称“阈值提示”；部分unknown不能变成“显示阈值内”，后者也不是认证正常、精度、安全或漂移结论。

`savedGnss`还持actual `gnssReviewToken`（独立UUID，不在top33内）；同页同值重算也产生新token。每次生成只显示报告，地图仍默认关闭。用户必须另按“明确在地图显示此页历史点”，只把准确reviewToken和同引用plain `{cancelled:false}`交facade，不传点或任意报告。返回exact5的selectionToken是 `reviewToken:正serial`，serial≤9007199254740991且无前导0，**不是UUID**。面板只保本次原lease；输入、确认、查询、收起、隐藏、disabled、owner变化和销毁先撤该引用，后精确clear自己的selectionToken，晚回ID也只清原token，不清另一面板后来选择。

facade `gnssMap` 是exact4 `{active,selectionToken,reviewToken,report}` 的只读固定图，非授权；冻结投影不可取得时可为null，消费者保守不绘。FlyView从实际 `globals.guidedControllerFlyView.telemetryArchive`取它，不另造SwarmController。仅绘≤64个MapQuickItem离散点，无Polyline／精度圆／路线／自动center、goto或任务动作；标记及完整label以AllButtons吞按压、点击与双击，防穿透原地图飞行菜单。图例保文件身份、组数、质量／未定位计数及后续页／Footer／尾行，不称完整历史。合法纬度±90仍完整留在报告；底图对|纬度|>85.05112878°的点仅明确省略并展示数量，不夹紧、替换或推断无风险。地图隐藏／disabled／PiP、换实际facade或原选择失效时精确退休原图层；关闭图层不停止飞机，不改全图已有操作。

沿原Qt主题、PlainText换行、≥44像素动作、可见单位／unknown、焦点标签及图例键盘滚动；使用ui-ux-pro-max及quick-reference／pro-rules作静态通用指导，未执行技能搜索脚本、UI／渲染／可访问性测量或交互测试。仅apply_patch及直接静读；未运行任何项目实现、QML／JS／解析器／helper／hash／SQL、编译、测试、仿真、硬件、PX4、提交、PR或打包。工程交接场景均待执行；本批不是原25目标全部完成、运行验收、定位真实性、任务归属、完整航迹或放飞许可。

## 493：显式跨文件搜索、人工选命中及同hash首页

实际[Panel QML](../../res/Merivus/FleetTelemetryHistoryPanel.qml)和custom [Guided facade](../../res/Merivus/GuidedActionsController.qml)在原qrc两个宿主继续使用，无新资源／页面。facade保旧request6及options7，增加独立historySearch／requestSearch8，不输出Archive原生对象、目录身份、raw页、任意path／policy／cursor或hash写口。

消息输入空或-1为全部、0是HEARTBEAT，机号输入空转native0。首批／同条件续批／锁摘要打开都需本次明确读盘确认；打开还沿原位置选择／header同意，报告另一次确认。新savedSearch与pending kind独立核完整36／row10及计数、来源、预算与连续文件index，不落入旧page／report分支；不认领另一个面板的global query。

三个search动作沿原press→ticket→撤双确认→仅一次latest推进→shared pending／双终结settle。筛选另有单调criteriaEpoch，改后改回也不能复用旧资格。每个检查file行含命中／无命中／未知，空字符串显示未知，不当0；旧批可留显示但明确非当前。不自动选首条或续读。选命中仅同步原文件选择并撤确认，不读盘；普通picker A→B→A、收起／隐藏／disabled清选择，重开需重新明确选取。

按压快照包含原完整row/hash与来源，采用前后尾核；search-open pending还核返回页的同fileHash。打开的是整个原文件首页，仅按机号，不按搜索msgid且可能没有命中；首末seq仅供显示，不跳转。多Run元数据不自动派生位置／运动值，不替代本页31及卷宗21。

主代理已完整读取ui-ux-pro-max与所需quick-reference／pro-rules，按用户约束未执行技能检索脚本；本批采用通用静态规则而非数据库匹配。保原QGC主题、44像素最小控件、PlainText换行、可见标签／错误、明确取消及无自动导航；没有渲染、对比度／大字／小窗／键盘／读屏／交互实测。本批仅修改地面站源码／文档并做直接静态阅读与普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。 [36场景](../../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)全未执行，下方为历史。

## 487：成对观察资料与四展示视角

原二次位置确认现明确同时生成位置／速度派生数值和资料报告，不新增action或读盘。native成功回执后，核dossier21／来源6／覆盖8／嵌套原31相同／四视角6及section3，采用前后仍核原context和已消费确认；一次savedReport同时持31及dossier，不分别通知采半份、不认领全局变化。

成对原生getter最新性及本面板pageInputsCurrent共同控制显示，隐藏／输入／query／options变化沿原invalidate清整个快照。视角在本地切换，无I/O或角色授权；未知不回退别视角。四视角真实填本页聚合数与限制，资料质量发现不是故障、事故、风险等级或任务完成。

可展开当前视角的只读结构化文本以人工选择复制；仅工程展示视角可另行展开完整卷宗，并明确其包含全部已确认派生数据。不是访问控制，不自动写文件／剪贴板／上传／发送，不调用旧Builder导出。Loader失效时文本绑定允许null，不解引用已清savedReport。UI/UX静态规则用于短确认、PlainText换行、单位／未知、分享提示、主题与44像素控件；未运行搜索脚本或UI／可访问性验证。34工程场景全未执行；本批仅静态源码／文档及普通文本核对；未编译、执行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机、放行、提交、更新PR或打包。 下方为历史。

## 486：两个实际宿主、原生页与分层位置同意

[QML](../../res/Merivus/FleetTelemetryHistoryPanel.qml)由qrc／qmldir登记，在[指挥中心](../../res/Merivus/CommandCenterOverlay.qml)及[Planner](../../res/Merivus/FleetMissionPlannerPanel.qml)挂载。原[Root](../../res/Merivus/GuidedActionsController.qml)只暴露固定值和exact6 request窄facade，不把Archive／Vehicle／raw／path／policy／cursor开放给组件。多个面板共享同一原生单槽，不能互相代收旧查询或取消他人请求。

展开不读盘，默认无确认／无文件自动选择；目录、首页、续页均一次明确读盘确认。位置载荷默认隐藏，要原档案header同意和本次include选择；即使读取位置成功，也需另一次同意才生成[派生摘要](FleetTelemetryMotionReview.md)。输入／latest/options代次变化、隐藏／disabled／收起均撤确认并清派生显示；下一页条件改变须从首页重读，不自动续查。

按压冻结owner／options／inputEpoch／consentEpoch／query／字段。pure ticket在callBusy通知前安装，提交后尾核只允许read正常一次query推进；review／cancel不推进。旧意图或通知中换输入／隐藏不得采报告。票据保过callBusy=false通知；settle自持票据，按原pending／owner／query逐步尾核，只CAS清同一pending，不让旧收尾删新请求。接受只表示请求，失败或结局未知不自动重试／推断回滚。

仅在busy=false、accepted／未取消及本界面query对应后采目录或公开摘要；旧页明确标是否仍当前，三计数分别显示，不相减成丢包率。派生31/5/16核原页六身份和全部限制，按组／段展示单位、未知、断段原因、报告值变化和速度；不显示raw／UTC／绝对坐标／epoch。最多64帧以虚拟列表显示分段，不跨页累积或插值。

UI/UX静态指导用于主题、PlainText／换行、44像素控件、文字状态／错误、明确取消、键盘防误操作；没有技能脚本、界面加载、对比度、小窗／大字、读屏、翻译或键盘实测。32工程场景全部未执行；仅静态源码与工程交接；未编译、运行helper／解析器／业务／模型／QML／应用／认证／SQL、测试、仿真、硬件操作、PX4修改、刷机或放行，未提交、更新PR或打包。

# 相机与视频生命周期交接合同（506）

## 507 补充：模式caller待修已纳入本轮源码

下方506指出的setVideoMode／setPhotoMode Fact／Vehicle外调后caller尾，本批已加入实际源修补：预冻原本地Fact，逐虚调／Fact写／Vehicle发送后核QQ、原来源及独立mode代次；toggle单次虚读、reset入场撤旧意图、同值请求和耗尽同管。保原公开API／payload，已可能写入不回滚。仍未认证全部旧相机API、网络参数下载／存储／渲染线程或远端模式执行。

新增的原始相机报告另见[完整合同](../../custom/src/Swarm/FleetCameraStreamObservation.md)、[25路径清单](../../docs/handoff/MERIVUS_SWARM_PATCH_MANIFEST.md)、[52项全部未执行](../../docs/handoff/MERIVUS_SWARM_ENGINEER_HANDOFF.md)。

Info／Status独立3秒年龄，32行上限、语义revision和原来源分别核对；URI／name不进入Fleet报告。RUNNING位只是相机报告，本地匹配不认证实际送流／停流、完整目录、带宽或恢复许可。远端受控视频配置／暂停／恢复、容量／丢包来源和完整视频降级目标仍待开发。

下方为506历史合同，其当时的待修结论不代表本轮已执行工程验证。

本批仅修改地面站源码／文档并直接静态阅读、核对普通文本；未编译、运行解析器／helper／业务／模型／QML／应用／认证／SQL／hash实现，未测试、仿真、操作硬件、修改PX4、刷机、放行、提交、更新PR或打包。下方为历史。

本合同对应现有相机发现、相机／视频流选择、活动飞机切换和本地接收器启停的源码修改。它不是新的 Fleet 视频控制服务，不提供受控远端降频／停流事务，也不是全部相机接口、Qt 模型或渲染线程的安全认证。

本批共 **7 份源码、12 份文档，合计 19 条路径**；唯一新增文件为本合同。保留原相机与 VideoManager 的 QML／公开控制入口，没有新增可见界面、资源／工程登记、权限或依赖。精确路径见[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)，工程师后续任务见[交接说明的 506 节](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。其中 **50 项工程场景全部未执行**。

## 1. 实际修改的消费者

| 源码 | 本批实际作用 |
| --- | --- |
| [QGCCameraManager.h](../../src/Camera/QGCCameraManager.h)、[QGCCameraManager.cc](../../src/Camera/QGCCameraManager.cc) | 原心跳发现／工厂、超时退休、相机选择、操纵杆输入和实际流查询。 |
| [QGCCameraControl.h](../../src/Camera/QGCCameraControl.h)、[QGCCameraControl.cc](../../src/Camera/QGCCameraControl.cc) | 原流目录／状态／选择与 STOP／START 命令续作；限定的拍照和模式通知尾链。 |
| [VideoManager.h](../../src/VideoManager/VideoManager.h)、[VideoManager.cc](../../src/VideoManager/VideoManager.cc) | 原普通／thermal 接收器、配置解析、活动飞机切换、本地播放意图、有限失败恢复和媒体动作。 |
| [QmlObjectListModel.cc](../../src/QmlControls/QmlObjectListModel.cc) | 上述实际模型调用所依赖的 begin／end、连接、dirty／count 通知和清理后的自身存活尾核。 |

实际链仍为原 Vehicle／CameraManager → CameraControl → VideoManager／VideoReceiver，以及原通用 MAVLink 命令发送器。没有用独立 helper 替代真实消费者，也没有改变飞控或消息协议。

## 2. 对象来源与旧续作失效

- 默认 CameraManager 以实际 Vehicle 为 QObject 父对象；持有实际 Vehicle、Toolbox、MultiVehicleManager、JoystickManager 和 FirmwarePlugin 的持续 QPointer。Vehicle 原 prepareDelete 的相机管理器清理与 QObject 父子销毁并存，不新增第二个生命周期所有者。
- Manager／Control 的新 C++ 原生只读选择代次分别为 `cameraSelectionEpoch()`／`streamSelectionEpoch()`。选择、目录、相关来源或显式启停意图变化可使旧续作失效；同索引的显式选择也不保留旧续作资格。代次不回绕，耗尽后不能重新成为新鲜可控制来源。VideoManager 的绑定耗尽同样是永久不可用，不允许后来显式 start 借旧飞机来源复活。
- 调用虚工厂、虚 getter、发送函数或可通知的模型／属性后，后续步骤核对原 QPointer、来源、对象身份和本次代次；A→B→A 不以最终数值相同承接旧操作。只读代次是本地继续执行的栅栏，不是授权、原始接收证据或远端执行凭据。
- 虚工厂／getter 返回的 raw 地址只与原来持续持有的对象或实际活 QObject 子对象比对，不从可能已失效或 foreign 的返回值重新构造弱引用。`streamModelNative()` 仅提供实际 CameraControl 成员模型；Manager 在调用虚 `streams()` 前先持有这个原成员，再核虚返回地址和两级代次。
- 自定义插件返回非本次实际父子关系的 Manager、CameraControl、stream 或 receiver 时，相关自动控制路径可保守拒绝；本合同不承诺任意自定义对象所有权模式均兼容。视频 sink 本身为 `void*`，不是 QObject：工厂返回后核原 plugin／window／widget／receiver 和操作，失效 sink 沿既有 GStreamer 释放路径处理。

这些限制不等于远端相机身份认证。Manager 仍处理原 Vehicle 调整后的消息缓存；本批没有把它变成原始 PRIMARY 接收切点、带时限状态证明或原生授权来源。

## 3. 相机发现、工厂与退休

1. 保持原支持的相机 component 与消息入口。原生请求记录以 component 和本次 CameraStruct 的实际对象为身份，不用 modelName 作为删除／选择身份。
2. 保留 500 ms 检查、超过 2 秒后才再次请求的间隔，以及初次请求加最多 11 次重试的现有兼容值。心跳只更新相机存活时间，不重置请求间隔、重试数或 gaveUp；仍持续心跳的 gaveUp 记录不会自动获得一轮新重试预算。
3. 工厂处理中单独标记 pending。只有实际 CameraControl 被接纳并准备发布时才标记 `infoReceived`；null、foreign 或回调中被删除的返回值不能把发现永久误置为成功。失败也不重新开始旧重试预算。
4. 原来源仍有效时，所有记录——包括未发现成功、工厂失败或已 gaveUp 的记录——在心跳消失超过 5 秒后按原身份退休；来源已失效时只允许精确清理已销毁的发布对象，不借失效来源完成普通超时续作。旧记录清理不删除同步回调中新建的同 component 请求；之后新的心跳发现不继承已经退休的 gaveUp。
5. 退休资格先于虚 getter／模型通知撤销。删除相机标签与行使用原实际身份，不因同名相机误删；移除非当前相机时尽量保留原选中的实际对象并调整索引。
6. 默认 Manager 的 ready／activeVehicle／joystick 连接不随重复 ready=true 叠加。只有当前实际 Vehicle 且实际 ready 的操纵杆输入可继续；换活动飞机会退休旧连接。连接／断开通知可删除对象时，仅清理局部持有的旧连接，不续用死成员容器。

本批保持原操纵杆节流兼容值：zoom 40 ms、相机／流切换 1000 ms。流切换按普通流标签数量循环，不把 thermal 行计入普通选择的上界；实际 Control 仍按非 thermal 的活对象顺序取目标，而不是按名字寻址。同名普通流因此不再指向同名第一行。

## 4. 流目录、选择与状态续作

- `VIDEO_STREAM_INFORMATION` 只接纳一基 stream_id／非零 count 及声明范围内的行，避免零 ID 从目录进入 STOP／START 全流目标。固定长度 vendor／model／definition URI／stream name／URI 使用有界 UTF-8 读取。
- 同一流、同 count 的完全重复 Info 不推进选择代次，不重复发布 ready 导致解码器重启。真实元数据变化更新原实际流对象、标签、thermal 状态和选择；同名不等于同对象。
- 普通流当前选择保存实际对象身份及非 thermal ordinal。一个流销毁导致 ordinal 前移时，仍存活的原选择可保持；原选中对象已死则只做本地存活行回退，不因此自动发送 START。待死行清理／新标签发布前，`setCurrentStream` 拒绝拿旧标签 index 选中后继对象。
- 改流原次序仍为旧流 STOP → 选定实际新对象 → 新流 START → 状态查询／通知；每个外调后停止已失效的旧续作。显式同索引选择、stop 或 resume 都会撤销旧意图，不能让较早 STOP 回调后的尾部覆盖后来选择。
- 状态请求另有本地代次、原目标对象和 ID；发送期间同步收到对应状态后，旧发送续作不能再启动过时 timer。旧状态超时也不能改查后来选中的流。保留原状态／Info 定时和重试兼容逻辑，不增加新的无限同步请求链。
- `takePhoto`、`stopTakePhoto`、`_setCameraMode` 和其直接 `handleSettings` 续作增加了限定的 QPointer／原操作尾核。它们可能已经交接的命令不被当作可回滚。

最后一项**不能外推至所有模式或相机 API**：旧 `setVideoMode`／`setPhotoMode` 当前已确认在 Fact／Vehicle 外调后直接继续 `_setCameraMode` 的 caller 缺少 QPointer 存活尾核，属于后续已知待修，本批没有关闭；全部拍照／存储／参数下载工作流也未在本批完整审定。

## 5. 本地播放意图与有限恢复

`VideoManager::stopVideo()` 控制本地接收／解码；相机录像 stop 和 MAVLink `VIDEO_STOP_STREAMING` 是不同动作。本地停止不证明远端已经停止发送，录像停止也不证明链路占用已经下降。

- 保留原默认初始化播放行为；初始化不是新的显式 start 意图。如果此前通知回调已手动停止，初始化、源／端口／延迟变化、旧完成回调或切活动飞机不能把本地停止意图改回播放。
- 显式 `startVideo()` 重新建立本地播放意图，仍受实际设置、streamEnabled、已解析 URI、实际对象来源和耗尽状态限制。
- 普通与 thermal 两个 receiver 分别持有 start／stop pending、URI、低延迟模式、绑定／播放意图和实际流选择。先冻结操作，再调用接收器虚方法；不把一个 receiver 的完成用于另一个。
- start 未完成时 stop 先撤销后续解码资格；旧 start 完成若已过时，按实际启动状态停止，而不是打开旧解码器。没有真正启动／空 URI 的 stop 不制造等待一个不会出现的完成信号。
- GStreamer watchdog 自停作为本地观察处理，可进入有限恢复。每个稳定本地意图、每个 receiver 的失败恢复最多 5 次，每次间隔 1000 ms，不在同步完成栈中递归重试；一次 start 成功本身不重置此预算。新的显式／来源／设置意图才重新建立自己的预算。
- 缺少新 thermal 源时清除旧 thermal URI；手动源 fallback 仅用于普通 id0。Taisync 条件分支直接解析普通 tsusb 地址并清热流地址，不递归调用 URI 更新自身。
- 录像、停止录像和截图操作在两个接收器／通知之间核原媒体动作、实际对象及来源，避免旧第一路回调之后继续操作后来第二路。

原活动飞机自动切换行为仍保留：可对冻结的旧 camera stop、对实际新 camera resume，但虚调后必须还是原绑定／原选择／原意图；同一 Vehicle 的源重绑不另借一次“换车”意图自动 resume。手动本地停止仍阻止本地重新播放。这不是新增经过 Session／角色／二审的受控远端暂停／恢复服务。

## 6. 模型内对象销毁与局部结构串行

本批不替换 embedded model、raw objectList 或公开模型接口。Manager／Control 的结构 busy 栅栏用于阻止自身在 append／remove 通知中嵌套结构修改；共享模型 callee 则补相应存活尾核。

- 已发布 camera 独立销毁时，按冻结的原发布地址扫描**实际模型全部 raw 行**，立即把匹配行置为空 tombstone；不解引用／重 pin 死地址，也不依赖可能已经提前移动的平行 native 索引。随后按同原请求身份清理，而不是 `indexOf(nullptr)` 任意删空行。
- 已发布 stream 独立销毁时，实际匹配 raw 行立即替换为本 Control 的活成员 tombstone，再按该独有地址移除。标签和选择按实际存活对象重建，不使用死对象名称。
- 结构忙时不在 begin／end 通知内再 remove；退出本 owner 的结构作用域后排一次清理，避免零延时忙循环。即使旧新鲜来源已失效，仍可进行精确旧死行的本地清理，但不能以此续发新相机命令。
- QmlObjectListModel 的 TextRole 对空 tombstone 不调用 objectName；相关 begin／end、连接／断开、dirty／count 和 clearAndDeleteContents 路径在可通知步骤后核自身存活，clearAndDeleteContents 不再重复不匹配 reset。

这些是实际调用链的局部生命周期修复，**不是**任意外部 C++ 直接突变 raw list、恶意删除模型／条目、所有 Qt 模型嵌套修改或所有线程的通用安全保证。

## 7. 必须保留的未完成边界

1. 本地代次、对象亲缘及 PRIMARY 弱所有权核对只限制本地续作，不能证明远端身份、消息新鲜度或实际执行。
2. 原通用相机命令 ACK 没有本批新增的请求 nonce／精确 stream 结果关联；接收器的完成信号也只有状态，没有唯一 requestId。旧本地串行状态不消除同实例晚到／重复完成的全部因果歧义。
3. 不认证远端 STOP／START、带宽下降、画质／发布率、链路容量、恢复配置许可、任务完成或飞行安全；不授任何飞行／配置权限。
4. SubtitleWriter 内部、Qt 渲染 job／接收器线程、既有跨线程标志及所有插件行为未完整审定；sink 保留既有 GStreamer 特定释放机制，不宣称通用生命周期抽象已完成。
5. 全部旧 CameraControl 入口、模式 caller、存储／参数／网络下载链没有因本批限定修复而自动获得安全认证。
6. 没有新增受控 Fleet 视频暂停／恢复／QoS 状态机，也没有与远端配置协议形成完成闭环。原 25 项目标与六类实质剩余不缩减；约 40%／60% 仍仅源码工作量粗估，不能由本批路径、代次或局部检查数量上调。

## 8. 工程交接与验证状态

源码作者与非作者仅直接静态阅读并核对普通文本。工程师需按[506 的 50 项场景](MERIVUS_SWARM_ENGINEER_HANDOFF.md)验证正常发现／选择／播放、同步重入、对象销毁、普通／thermal 同名场景、代次耗尽及条件编译组合，不能把本合同或源码静读当作构建／测试通过。

本批**未编译，未运行解析器、helper、业务、模型、QML、应用、认证、SQL 或 hash 实现，未测试／仿真，未操作硬件，未读取或修改 PX4，未刷机／放行，未提交或更新 PR，未打包**。本合同不提供可部署或可飞保证。

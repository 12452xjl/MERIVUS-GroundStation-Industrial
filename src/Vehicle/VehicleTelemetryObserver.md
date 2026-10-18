# 原生被动遥测观察入口

300–304 源码增量；未编译、运行、测试、接设备或改变 PX4。

## 来源和接口

[`VehicleTelemetryObserver.h`](VehicleTelemetryObserver.h) 是 QObject 派生的纯 C++
抽象接口，没有 Q_OBJECT、逐包信号、QML 类型、invokable、导入或控制入口。
Vehicle 的 native `bindFleetTelemetryObserver()` 只接受同 GUI 线程的实际观察者，
每架当前注册机只绑定一个，不覆盖其它记录器。成功后冻结 PRIMARY Link、UUID
epoch 与从零起算的非回绕 uint64 接收序号；`unbindFleetTelemetryObserver()` 必须
匹配实际观察者和 epoch。该绑定不请求数据流、不改变现有日志或飞行状态。

接收入口先复制 `originalMessage`，在任何 prepare/通知及 firmware/core 改写前
同步调用受保护接口。只接实际工具箱 MAVLinkProtocol 的入站回调、实际
MultiVehicleManager 按 ID 注册的同一个 Vehicle、当前 PRIMARY 的实际已连接成员，
排除离线编辑、Mock 和 LogReplay；只接受本 sysid 的非零 component，非主链路和
旧 sysid=0/RADIO_STATUS 例外不进入新档案。协议/Vehicle/观察者/管理器同 GUI
线程；LinkInterface 自身可能是工作线程对象，不能错误要求 Link GUI affinity。
不以估计器健康、版本取证或 armed 状态筛掉异常遥测。

源码在回调前冻结值、递增序号并持 QPointer，接收外层回调后重新查 Vehicle/Link。
忙栈重入或序号耗尽只退休本记录器，不改原控制/解析逻辑。序号包括白名单后来
过滤掉的消息，因此序号空档不是准确无线丢包；MAVLink 八位 seq 也不是本计数。

## 来源退休和线程寿命

PRIMARY 的任何变更通知（含观察到的 ABA）、失联、成员状态不再匹配、Link销毁、
Vehicle 移除/close/prepareDelete 均清除绑定和连接后才回调固定退休原因。
close/prepareDelete 先设独立 removing 状态并持 QPointer，禁止退休回调重绑将被
删除的飞机；析构只解绑，不对半析构 Vehicle 发虚接口回调。旧 destroyed 的
lambda 绑定 epoch，不退休新绑定。观察者派生析构必须先解绑，不能等 QObject
destroyed 才拆 pure virtual 接口。[记录控制器](../../custom/src/Swarm/FleetTelemetryArchive.md)
实现了该规则；退出/源失效不自动续建或追随同 ID 新飞机。

退休函数先核 Vehicle 当前线程，只有仍同线程的观察者才直接回调；非法移动
管理器/观察者的通知不在 foreign 线程改 Vehicle 或派生字段，GUI ingress/轮询
仍会拒绝来源并保留 unknown 尾部。这不承诺任意非法 native 跨线程操作安全。

这是地面站内观察来源绑定，不认证无线发送者、硬件、账号、CRC、签名、飞行
许可或全数据流完整性。当前 parser 的签名/CRC 静态边界见
[Codec 契约](../../custom/src/Swarm/FleetTelemetryArchiveCodec.md)。平台通知时序和
各实际串口/UDP/TCP 链路兼容性由工程师后续验证；本批未调用这些路径。

# 启动阶段局部运动与航向观察计算

320–324补充：[HOME schema3](FleetMissionStartupHome.md) 继承本模块的 schema2 local/heading 全部限制，仍只在真实 owned MODE 后消费局部运动，继续独立核 ODOM 参考和周期 gap。原始 HOME/GP 参考由另一个纯值 helper 明确冻结，不由本模块重设原点；v1/v2 原策略不升级。下方280–284的“HOME仍stop-review”仅指原v1/v2，不否定新v3已明确审阅的有界观察分支，任何版本都不认证HOME内部有效性/同origin/同样本或最终许可。

280–284 新增[显式局部运动与航向观察政策](FleetMissionStartupMotion.md)：原startupPolicy v1六字段/XYZ与航向严格保留，独立v2八字段明确local7/heading3包线，不补默认值；仅实际owned MODE后接受包线内LPN位移/速度与已知heading变化，全球走廊仍独立核验。累计观察转角按全部原始journal重建，跨零不误作整圈，相同heading不累计；两个native风险revision域/队列及final投影不删除。v2必须实际NED/NED、显式reset/estimator8的ODOM参考与政策gap，原生期初/逐包/尾间隙覆盖ODOM，v1不增加周期前置。Txn真实6/8门禁、pure冻结政策、Readiness/READ/Transition及pri已接；HOME仍stop-review，不认证两个uORB流同origin/同sample/内部有效分支或未观察旋转。源码与限定静态审阅，仍无净空/最终StartPermit/完整时序与执行完成；原六类/25目标不缩减，未编译/运行/测试/API/飞控改动/刷机/PR或打包。下方275–279及更早批次为历史记录。

280–284新增独立非QObject静态模块。两个接口只做固定规模的值校验和double数学计算，不读取Vehicle、事务、时间、来源或审批，不请求遥测、不发包、不调用业务接口。仅完成源码与有限文本检查，未编译、运行或测试；下面示例是契约解释，不是测试结果。

## 调用边界

`localMatches(observed, reviewed, envelope)`与`headingMatches(observed, reviewed, previous, envelope, &observedTravelDegrees)`的true只表示所给观察值符合明确数值范围，不是StartPermit、参考有效性、净空、任务完成或真实物理运动证明。

上层必须先核实际冻结Transaction/阶段、schema2原始journal、原始值与hash、单调计数及完整来源，不能提交调用者拼装的JSON替代真实证据。局部运动还需核实际Vehicle原生ODOMETRY NED来源、参考metadata和reset/reference revision；该来源核对仍不证明LOCAL_POSITION_NED与ODOMETRY同样本、同原点或估计器分支有效。模块不以ODOMETRY pose替换LOCAL_POSITION_NED。

## 局部NED观察空间

两个值map均恰有`x/y/z/vx/vy/vz`六键。所有值须为真实有限数值型（Int/UInt/LongLong/ULongLong/Double），绝对值不超过1e7，并满足`double(float(value)) == value`；字符串、bool、null、NaN、Infinity和不可精确表示为binary32的值拒绝。reviewed的速度虽不参与差分，也同样校验。

envelope恰有以下七键，没有默认值：

| 键 | 单位／允许值 |
| --- | --- |
| referenceBasis | 精确字符串`observed-native-odometry-ned-not-origin-proof` |
| maxHorizontalDisplacementMeters | 米，0.01–1000 |
| maxClimbDisplacementMeters | 米，0.01–1000 |
| maxDescentDisplacementMeters | 米，0.01–1000 |
| maxHorizontalSpeedMetersPerSecond | 米／秒，0.01–300 |
| maxClimbSpeedMetersPerSecond | 米／秒，0.01–20 |
| maxDescentSpeedMetersPerSecond | 米／秒，0.01–20 |

范围只是输入解析边界，不是机体能力建议。位置差为当前值减原reviewed值；横向限制比较`hypot(dx,dy)`，上升比较`max(0,-dz)`，下降比较`max(0,dz)`，因为NED z向下。速度只取当前`hypot(vx,vy)`、`max(0,-vz)`、`max(0,vz)`，不把速度变化误当当前速度。

dx/dy/dz及后续指标须有限。横向差分分量绝对值先向正无穷做nextafter，再求hypot；所有六个最终比较指标也向正无穷做nextafter。限制值不向外扩大，编码值正好落在上限的情况可能保守拒绝，不增加任意米级误差预算。没有全球坐标、HOME原点转换、同样本推断或定位误差容差。

说明例：原局部位置`(0,0,0)`、新位置`(3,4,-2)`，编码空间横向位移为5米、上升位移为2米、下降位移为0；上限恰设5米或2米会因向外包围保守拒绝，应由原审批政策明确给出实际边界，而非运行时偷偷补容差。

## 航向观察空间

三个值map均恰有GP风险九键：`latitudeE7/longitudeE7/altitudeAmslMm/relativeAltitudeMm/velocityNorthCmPerSecond/velocityEastCmPerSecond/velocityDownCmPerSecond/headingCdeg/headingKnown`。本模块只解释`headingKnown`（必须真实bool true）和`headingCdeg`（真实数值型整数0–35999），其余七值仍由上层原始GP证据契约校验。未知航向65535、分数、字符串和false均拒绝。

envelope恰有`maxDeviationDegrees/maxAdjacentChangeDegrees/maxObservedTravelDegrees`三键，前两项为真实有限数字0.01–180度，累计项0.01–3600度；adjacent不得大于deviation，travel不得小于adjacent，没有默认值。

当前相对原reviewed航向的偏离、当前相对上一真实journal航向的变化均取最短圆周绝对差。每对差加0.02度，保守包围两个0.01度编码区间；除以100及相加均nextafter向正无穷。即使编码相同，单次偏离／相邻检查仍有该编码包围，因此合法输入边界0.01度不保证观察检查可通过。

`observedTravelDegrees`须为非null指针，输入有限且0–3600度。只有当前编码不同于previous才将包围后的相邻变化加入累计值，并向正无穷nextafter；累计限制比较同样保守向外。任何失败均不改指针，编码相同时通过也不改变累计值。调用方须保持同一真实冻结观察链的累计值及顺序，不得每包归零、用新reviewed值重设基线或替换previous跳过journal。

说明例：`35999 → 0`是编码最短差0.01度而非359.99度；单对保守上界略大于0.03度。取明确0.05度相邻上限可覆盖这一编码变化；取恰0.03度则保守拒绝。连续收到相同`0`编码不再累加转动，但仍核相对原reviewed的偏离和当前累计限制。

没有飞控时间到地面时间映射、角速度检查、未观测半圈／多圈转动检测或无线间隙内物理连续性认证。最大累计观察变化不等于实际累计旋转角，更不证明没有绕圈或转向安全；上层原始gap、队列前基线及journal边界不能因helper通过而删除。

## 交接与后续

本模块仅作为[阶段复核](FleetMissionStartupTransition.md)和[启动事务](FleetMissionStartupTransaction.md)的观察数学部件。[原生ODOMETRY说明](../../../src/Vehicle/FleetStartupOdometry.md)保留参考有效性／同样本未认证边界。工程师后续需验证真实数值型、float32往返、NED符号、编码边界、35999/0跨界、保守等号、失败指针不变、journal顺序／ABA／gap等行为；本轮不执行这些验证。

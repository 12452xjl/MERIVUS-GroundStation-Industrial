# 组织仓库源码提交与整合记录

日期：2026-10-18。用户要求将已交付本地源码提交到 `Merivus-Industrial/MERIVUS-GroundStation`，提交描述使用中文，保留此前不擅自合并及不编译/测试/刷机的约定。

## 来源

- 接收仓库：`https://github.com/Merivus-Industrial/MERIVUS-GroundStation`，目标 `main` 基线为 `ffe42d8fd209335df66c9a0855917f0122341101`。
- 原本地基线：`006199cc71375619f8a21a861079a13c31e8a6fd`；完整交付快照提交：`51c2b58ed0616de9928360e43c9664f7f8192f87`。
- 共同基线：`8ea4380a472e08ca7bf03c7bc8bd82eddcf7eee0`。
- 发布分支：`merivus/source-handoff-20261018`；当前账号对接收仓库只有读取权限，因此通过其 Fork `12452xjl/MERIVUS-GroundStation-Industrial` 提交 PR。
- 在独立工作区完成整合，原 `E:/feikong/GroundStation` 的未提交文件与最终交付 ZIP 保持原样。目标远端配置的添加不改源码。

## 本次内容

提交完整 Fleet OS 本地开发源码及工程文档，不仅最后五条操作链。包含原生能力/构型/健康证据、人工任务审阅与上传回读、受限启动与运行观察、任务交接、预演/预算、归档和工作区，以及规则待审、Run检查点、用户模板、显式留存与四受众报告。

与目标主分支做三方整合，保留其现有 FTC FactGroup/消息方言/状态面板、ReviewRange/ReviewVideo、Linux打包及原文档。具体冲突处理：

- `custom/custom.pri`、`CustomPlugin.cc` 同时保留全套本地原生舰队类型和目标复盘类型登记；SwarmController仍只在原私有模块注册，不重新开放公开命令入口。
- `FlyViewMap.qml` 同时保留地图交互ID和复盘检查时的交互禁用；`CommandCenterOverlay.qml` 保留目标ReviewVideoController与本地舰队面板。
- README、文档审计、项目状态及当前架构状态保留两侧记录；新增本次发布说明，区分历史验证与本次未验证源码。
- 自动合并的资源/类型表、qmake/CMake和Vehicle接线直接静读核对，FTC与原生能力/任务观察/元数据接口均保留。

## 本次检查和未执行事项

本次仅源码直接阅读、Git冲突/空白与路径/资源登记核对，以及上传范围的常见凭据格式/大文件检查。未运行项目、业务helper、解析器、算法、QML、编译、测试、仿真或硬件操作；提交使用`[skip ci]`标记。本次检查不证明构建成功、功能可运行或没有缺陷，原工程验证清单仍全部待执行。

目标仓库保留的 Qt构建、FTC测试及链路结果属于其明确标注的旧源码版本，没有据此认证本次整合代码。新版本需工程师实际构建并验证，代码问题反馈后继续修正。

本次只提交审阅，不直接写目标main、不强推、不合并旧PR或新PR。原Ale-xl仓库PR #7不会自动包含此次组织仓库提交。

## 交付边界

[最终范围与外部依赖](MERIVUS_FLEET_DELIVERY_SCOPE.md)继续有效。跨站身份/撤销/控制租约、动态插件运行时/签名/隔离、正式机载任务结果、权威环境数据及实测模型仍未完成或缺少输入；不能把源码提交视为原25项完整产品能力全部落地。

工程审阅入口：[修改清单](MERIVUS_SWARM_PATCH_MANIFEST.md)、[静态审计](MERIVUS_SWARM_STATIC_AUDIT.md)、[工程师交接](MERIVUS_SWARM_ENGINEER_HANDOFF.md)。本分支是包含目标已有更新的整合版本，不与此前封包源码逐字节相同；旧ZIP仍准确记录其封包时点的交付内容。

# FleetExtensionRegistry 扩展元数据契约

`FleetExtensionRegistry` 只登记地面站扩展的元数据，不加载任意二进制、不执行入口、不释放飞行命令。扩展即使在快照中标记为启用，也只是本地 UI/策略状态；签名验证、沙箱、版本兼容和真正的适配器执行仍须由工程师接入可信宿主。

注册、启用、删除和快照导入/加载接口仅供受控 C++ backend 使用，未暴露为 `Q_INVOKABLE`。QML 侧只读取扩展值快照和查询结果；默认扩展在 C++ 构造阶段登记，目录展示不等于插件已加载或获得任务权限。

## 本地快照

- `exportSnapshot()` 输出 `schemaVersion=1`、稳定排序的扩展列表和固定的 `flightCommandRelease=false`。
- `importSnapshot()` 先完整校验，再一次性替换/合并；扩展 ID、版本、类型、能力、入口和 metadata 的类型/长度错误、未知字段或 `flightCommandRelease=true` 会整体拒绝。
- `metadata` 不是无界 JSON：递归深度最多 8 层、节点最多 2048 个、单个键最多 128 个字符，字符串/字节累计最多 128 KiB；注册和导入使用同一门禁，非 JSON-safe 的自定义 QVariant 也会拒绝。
- 单个扩展快照最多 2048 个条目；超出条目数或文件 32 MiB 上限的导入会在替换现有目录前失败。
- `saveToFile()` 使用 `QSaveFile` 原子提交；`loadFromFile()` 拒绝空路径、不可读文件、超过 32 MiB、非法 UTF-8、坏 JSON、错误 schema 或试图打开命令释放边界的快照。
- 导入不会隐式启用扩展，也不会执行 `entryPoint`。

工程师仍需验证签名、权限、账号隔离、跨站同步、沙箱和长期存储策略；本模块不是插件安全边界，也不代表扩展已经可用于飞行任务。

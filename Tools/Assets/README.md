# 资源容器与迁移

Texture、Mesh、Material、MaterialInstance、Actor、World 使用 `.asset`；
Shader 使用 `.shader` JSON 描述文件，源码和 include 继续独立保存。
资源引用为 `{ "id": "UUID", "path": "Contents 下的相对路径" }`。
ID 决定身份，路径仅用于阅读；缺失或重复 ID 会报错。

Material、MaterialInstance、Actor、World 的 `.asset` 是缩进 UTF-8 JSON：

```json
{
  "resource": {
    "id": "稳定 UUID",
    "type": "MaterialInstance",
    "dependencies": [{"id": "源材质 UUID", "path": "material/source.asset", "kind": "hard"}]
  },
  "data": {"origin": {"id": "源材质 UUID", "path": "material/source.asset"}}
}
```

描述及业务字段通过 `AssetFile` 统一读取、保存。保存从正文自动生成依赖摘要；
读取时校验摘要与正文的 ID、Hard/Soft 分类一致，重复、遗漏和多余项均报错。
材质、实例和 Actor 引用是 Hard；World 的 Chunks/Actors、Chunks/Bundle 是 Soft，
只供浏览器和流式加载定位。同一个 ID 同时被必需内容引用时按 Hard 处理。
可读路径更新不影响身份。正文及描述中的未知扩展字段保留。

Texture、Mesh 保持二进制容器。48 字节文件头按小端编码：8 字节 `ELASSET\0`、
u32 容器版本、u32 头长度、u64 描述长度、u64 数据起点、u64 数据长度、
u64 保留字段。UTF-8 JSON 描述包含 `resource` 和 `sections`，section 的 offset
相对数据区起点。资源 `dataVersion` 独立于容器版本。二进制描述读取不访问大数据区；
文本资产读取完整 JSON。两种格式保存均先写临时文件再替换。

加载链先由 Worker 读取当前描述，GameThread 按 Hard 引用请求子资源并连接发布事件。
本资源 CPU 解析与子资源加载并行，汇合后再准备依赖、初始化 GPU 和发布。
类型解析只保存具名引用；准备阶段从本次操作提供的依赖表取对象，不另行发起加载。
循环、依赖失败和元数据在加载中改变均结束本次操作，保留已有可用数据。
Shader 的源码/include 依赖继续独立追踪。

Texture 包含 `texture` 布局、`import` 设置、`source0...N` 原图和
`data` 像素区。RGBA8 或线性小端 IEEE-754 RGBA32F；首版单 mip。
Cube 面顺序为 +X、-X、+Y、-Y、+Z、-Z，数组按导入列表顺序排列。
重新导入只使用内嵌原图，失败不会覆盖旧文件，ID 保持不变。

```powershell
$env:PATH = 'S:\VulkanSDK\Bin;' + $env:PATH
& Binaries/Win64/EditorDebug/ElaineAssetTool.exe readable Contents
& Binaries/Win64/EditorDebug/ElaineAssetTool.exe validate Contents
& Binaries/Win64/EditorDebug/ElaineAssetTool.exe inspect Contents/texture/sky/sky.asset
& Binaries/Win64/EditorDebug/ElaineAssetTool.exe reimport Contents/texture/sky/sky.asset
python Tools/Assets/audit_migration.py Contents Intermediate/AssetMigrationBackup/42fae94b-85fb-4677-b699-fb69530af267 --output Build/asset-migration-audit.json
```

`readable Contents` 将旧业务容器转换为文本，先在 `Intermediate/ReadableAssetsBackup`
备份及暂存校验；全部成功后原子替换，失败恢复。已是文本的文件跳过。

旧格式只由迁移工具读取：`migrate Contents` 预览，增加 `--apply` 执行。
执行时先在 `Contents` 外备份，验证全部转换结果后发布并移除旧文件。
本次迁移备份与路径对应表位于
`Intermediate/AssetMigrationBackup/42fae94b-85fb-4677-b699-fb69530af267`。
实施前工作区另存于 `Intermediate/AssetMigrationBackup/20261023-121939/initial-worktree.zip`。

注册表在 Worker 扫描，GameThread 发布查询快照；缓存位于
`Intermediate/ResourceRegistry`，可删除后重建。浏览器查询不触发资源加载。
资源移动与删除通过加载完成事件串行处理，删除不修改引用方保存的 ID。

编辑器 Content Browser 提供目录、搜索、类型/属性筛选、描述与引用查询。
`Import texture` 支持单图、六面 Cube 和数组，导入前可以调整顺序；
`Reimport embedded source` 只使用资源内嵌原图。导入操作由注册表管理，
完成事件在 GameThread 发布描述之后结束，关闭时等待尚未结束的操作。
`Rename / move` 接受 Contents 下的相对路径，`Delete` 展示引用方并要求确认。

本轮可读资产与依赖加载验收见 [READABLE_ASSETS_VALIDATION.md](READABLE_ASSETS_VALIDATION.md)。
最初统一容器迁移记录见 [VALIDATION.md](VALIDATION.md)。

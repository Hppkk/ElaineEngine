# 公共资源导入

`ElaineAssetImport` 是 Editor、AssetTool 和测试链接的 CPU 导入库。运行时只加载保存好的 `.asset`，不调用 Assimp 或图片解码器。

## 编辑器调用

```cpp
MeshImportSettings Settings;
Settings.Scale = 1.0f;
auto Event = ResourceImportService::Get().ImportMesh(
    SourceFile, "model/face.asset", Settings,
    [](const ResourceImportResult& Result) {
        if (Result.Succeeded()) {
            // 此处已保存整批文件并发布注册表；GPU 资源仍按需加载。
            MeshPtr Mesh = MeshManager::instance()->GetResource(Result.Primary.Path);
        }
    });
```

纹理使用 `ImportTexture(Sources, Destination, TextureImportSettings, Completion)`，支持 `2d`、`cube` 和 `2darray`。Cube 顺序为 `+X, -X, +Y, -Y, +Z, -Z`。重导入使用 `Reimport(Path, Completion)`。

请求持有设置的独立副本；完成回调排入 GameThread 的业务回调批次，不内联执行。服务必须在 TaskGraph、ResourceLoader、FileMonitor 和 Registry 关闭前 `Shutdown()`。关闭会等待内部工作收尾，并跳过尚未交付的业务回调。

## 职责

- `ResourceImporter`：声明格式和设置，CPU 转换并返回 `ImportedAsset`；不创建 GPU 资源、不修改注册表、不自行覆盖目标文件。
- `ResourceImportService`：注册导入器、路径和 ID、TaskGraph 调度、加载 gate、文件监控协调、暂存和回滚、整批发布、重导入三方合并。
- `ResourceRegistry`：查询、依赖索引、资产位置和描述发布；不调度转换。

离线工具构造 `ResourceImportService(ContentsPath)`，使用 `ImportBlocking()` / `ImportTextureBlocking()` / `ReimportBlocking()`。无需启动 TaskGraph 或 GPU；阻塞接口不用于带活动 Registry 的服务。

## 模型输出

支持 FBX、OBJ、glTF、GLB。节点变换烘焙成一个 Mesh，节点实例保留独立 SubMesh；纹理和材质实例输出至 Mesh 同名目录的 `Textures/`、`Materials/`。默认使用 `material/ImportedPBR.asset`。

纹理按源身份、用途和色彩空间在批次内复用。独立金属度／粗糙度贴图转换为 B/G 通道。PBR 支持基础不透明和遮罩；混合透明、骨骼、动画、额外 UV 和其他不支持属性返回警告。

材质槽保存默认材质的稳定引用及 Hard 依赖。组件显式覆盖优先，清空覆盖后恢复 Mesh 默认材质。Mesh 的 `mesh` 区保存整体及 SubMesh 的局部 AABB；正常加载读取并校验，不扫描顶点重算。

源材质使用源表索引作为身份，名称只用于首次输出的文件名；修改源材质名称保持输出 ID 和路径。源文件若重新排序材质表，需要核对映射，当前不推测源格式未提供的持久 GUID。

导入记录保存在描述扩展字段 `import` 中，包含源文件、设置、源身份到输出 ID 的映射及上次导入值。重导入只更新仍等于旧导入值的材质字段，保留手工修改。移除的输出保留文件并返回 `UnusedAssets`，其映射也保留以便源内容恢复；模型重导入仍需要原模型文件。单独纹理可从内嵌原图重新处理。

文件先在 Contents 外暂存，目标发生并发修改时拒绝覆盖；提交失败恢复已写文件。恢复本身遇到磁盘错误时，错误信息会给出保留的恢复目录。

## AssetTool

```text
ElaineAssetTool import <Contents> <source> <destination.asset> [--scale N]
ElaineAssetTool import <Contents> <image> <destination.asset> [--srgb] [--dimension 2d|cube|2darray] [--source image ...] [--usage color|normal|data|hdr]
ElaineAssetTool reimport <Contents> <relative.asset>
ElaineAssetTool upgrade-mesh <Contents>
ElaineAssetTool validate <Contents>
```

`upgrade-mesh` 仅在离线阶段为旧 Mesh 增加包围盒区，不修改几何字节或 ID，不推测缺失的源模型和材质。

# 资源迁移验收记录

验证日期：2026-10-23。Git 未暂存、未提交，保留实施前的已有改动。

## 实际迁移与独立审计

| 类型 | 数量 |
|---|---:|
| Material | 3 |
| MaterialInstance | 2 |
| Mesh | 1 |
| Texture | 2 |
| Actor | 1 |
| World | 1 |
| Shader | 10 |

共 10 个 `.asset` 和 10 个 `.shader`。Shader 源码及配置保留；旧资源文件与
七张已内嵌的独立图片已移除。`ElaineAssetTool validate Contents` 验证全部
ID 唯一、引用有效、纹理大小与资源依赖摘要一致。

`audit_migration.py` 对照备份再次检查：

- 全部 ID、类型及目标路径与迁移清单一致。
- Material、实例、Actor、World 的原 JSON 内容在引用转换后完全一致，
  包括参数、组件、Actor GUID 与 World 流式配置。
- Mesh 二进制数据与原文件逐字节一致。
- sky 六面原图和 night EXR 原图逐字节一致，并记录 SHA256。
- night 为线性 RGBA32F，最大像素分量为 2.0078125，保留超过 1 的动态范围。

备份及迁移对应表：
`Intermediate/AssetMigrationBackup/42fae94b-85fb-4677-b699-fb69530af267/manifest.json`。
实施前工作区：
`Intermediate/AssetMigrationBackup/20261023-121939/initial-worktree.zip`。
审计输出：`Build/asset-migration-audit.json`。

## 编译与自动测试

- EditorDebug：ElaineEditor、ElaineCore、ElaineEngine、ElaineAssetTool 和资源测试目标编译通过。
- RuntimeDebug：ElaineGame、ElaineCore、ElaineEngine、ElaineAssetTool 和全部回归测试目标编译通过。
- 两个配置的 CTest 均为 **6/6 通过**：资源、Shader 参数、TaskGraph、
  InputRouter、WindowsInput、ImGuiInput。
- 新增容器、截断/越界、内嵌重导入、失败保留、HDR、Cube/数组顺序、
  重复 ID、失效 ID、增量/完整扫描、注册表失败传播、导入失败、
  移动等待加载、首次保存的稳定对象与删除回归。

日志：`Build/asset-final-editor.log`、`Build/asset-final-runtime.log`、
`Build/asset-tests-editor-final.log`、`Build/asset-tests-runtime-final.log`。

运行测试前需将 Vulkan SDK DLL 路径加入 PATH：

```powershell
$env:PATH = 'S:\VulkanSDK\Bin;' + $env:PATH
ctest --test-dir Build -C EditorDebug --output-on-failure
ctest --test-dir Build -C RuntimeDebug --output-on-failure
```

## 实际 Vulkan 验证

启用 Validation 及同步验证，两种 `UseRHIThread` 模式都已运行：

- 2D、Cube、数组、16/32 位索引 Mesh 的真实上传与绘制回读。
- 多物体、Uniform 池、多视口、具名纹理/只读 Buffer 自动绑定回读。
- Shader/材质重载、失败回退、在途资源卸载和退出。
- 真实迁移资源加载、night EXR、sky、Actor GUID、World 流式 Actor 数量。

上述功能检查均执行完成，**严格 GPU 测试最终返回 1，未通过零验证错误验收**。
原因是现有 `ElaineVulkanDevice.cpp` 仍在 `VkDeviceCreateInfo` 设置 device layers，
触发 `VUID-VkDeviceCreateInfo-enabledLayerCount-12384`。当前 Vulkan 规范要求
该字段为 0。本轮未修改这条既有 RHI 设备初始化路径。

`UniversalPBR.vs` 和 `UniversalPBR.ps` 原本是空文件，已保留内容并建立描述；
实际加载测试跳过这两个空阶段，未将其报告为可运行 Shader。
GBuffer 存在未使用输出警告。负向测试中的非法 Shader、缺失 Buffer、
类型不符诊断属于预期检查，与上述 Vulkan 验证错误分开统计。

日志：`Build/asset-gpu-dedicated.log`、`Build/asset-gpu-inline.log`、
`Build/asset-migrated-dedicated.log`、`Build/asset-migrated-inline.log`。

复现：

```powershell
$env:PATH = 'S:\VulkanSDK\Bin;' + $env:PATH
$env:VK_LAYER_VALIDATE_SYNC = '1'
& Binaries/Win64/EditorDebug/ElaineResourceTests.exe --gpu
& Binaries/Win64/EditorDebug/ElaineResourceTests.exe --gpu inline
& Binaries/Win64/EditorDebug/ElaineMigratedAssetTests.exe
& Binaries/Win64/EditorDebug/ElaineMigratedAssetTests.exe inline
```

## 编辑器交互

两个 RHI 模式均完成 `ElaineEditor --resource-smoke` 的 180 帧启动、运行与退出检查。
进程返回 1，原因同上述 Vulkan device layers 验证错误；现有 DX11/Vulkan 共享纹理
打开警告触发 staging fallback，未导致启动失败。

尚未完成人工点击/视觉验收：目录与筛选、引用跳转、导入面顺序、移动、
重导入、删除确认，以及迁移场景的视觉检查。自动 smoke 不替代这些验收。
日志：`Build/asset-editor-smoke.log`、`Build/asset-editor-smoke-inline.log`。

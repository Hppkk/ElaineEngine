# 可读资产与元数据依赖验收

日期：2026-10-23。Git 未暂存、未提交。对照实施前源码快照确认本轮未修改 RHI、Vulkan 或 TaskGraph 实现。

后续 Vulkan 问题已单独修复并复验，最新结果见 [VULKAN_VALIDATION_FIXES.md](VULKAN_VALIDATION_FIXES.md)。本页保留资源改造当时的原始结果。

## 交付

- Material 3 个、MaterialInstance 2 个、Actor 1 个、World 1 个，共七个业务资产已转换为缩进 UTF-8 JSON，顶层为 `resource` 和 `data`。
- Texture、Mesh 三个二进制容器与转换前逐字节相同。Shader 描述及源码保持原样。
- `ResourceDependency` 复用 `ResourceReference`，显式记录 Hard/Soft。保存自动生成，读取检查遗漏、多余、重复、未知策略和错误分类；比较身份及策略，允许可读路径过时。
- 材质、实例、Actor 的必需引用为 Hard；World 的流式 Actor/Bundle 为 Soft。相同 ID 同时参与必需用途时采用 Hard。
- Worker 读取当前描述，GameThread 建立 Hard 依赖及发布事件边。本资源 CPU 解析和子资源加载并行，汇合后准备依赖、初始化 GPU、发布。类型解析不请求子资源，准备阶段只查询本次操作的依赖表。
- 保留稳定对象、并发去重、循环检查、GameThread 回调、取消、卸载和失败回退。取消后的发布额外等待正在解析的 CPU 任务，防止提前释放临时数据。
- 浏览器的依赖列表标识 Hard/Soft；反向索引包含两类引用。World 继续由流式加载方请求 Actor，Shader include 继续独立追踪。

## 迁移备份及独立审计

本轮实施前源码与业务资产：
`Intermediate/ReadableAssetsBackup/20261023-142211/`，包含 `source-before.zip`、原始七个资产及带原始 SHA256 的 `manifest.json`。

转换工具的备份、暂存结果及清单：
`Intermediate/ReadableAssetsBackup/b2f7f092-88e2-4426-9071-ed7b4b977012/`。

`ElaineAssetTool readable Contents` 已实际执行；再次执行只验证，未重复改写。
`ElaineAssetTool validate Contents` 验证全部 20 个资源的 ID、引用及依赖摘要。
独立 Python 审计对照最初迁移备份，验证全部 ID、类型、业务内容、引用分类、Mesh 数据和七张内嵌原图；结果为 `Build/readable-migration-audit.json`。

## 编译与自动测试

| 项目 | 结果 |
|---|---|
| EditorDebug：ElaineEditor、Core、Engine、工具和全部测试目标 | 编译通过 |
| RuntimeDebug：ElaineGame、Core、Engine、工具和全部测试目标 | 编译通过 |
| EditorDebug CTest | 6/6 通过 |
| RuntimeDebug CTest | 6/6 通过 |

回归包含资源、Shader 参数、TaskGraph、InputRouter、WindowsInput、ImGuiInput。
新增 `MetadataDependencyTests.inl` 覆盖四类业务文本往返、UTF-8、描述与正文扩展字段、ID 保留、损坏/截断、摘要校验、元数据建边、CPU 与依赖并行、去重、缺失依赖、文件级循环、依赖类型检查、加载期间元数据改变及失败保留旧数据。
原有回调线程/批次、订阅取消、缓存命中、过期任务、卸载、上传及关闭测试继续通过。

日志：`Intermediate/readable-build-editor.log`、`readable-build-resource-tests.log`、`readable-build-runtime.log`、`readable-tests-editor.log`、`readable-tests-runtime.log`。
第三方 SPIRV-Cross 缺失 PDB 的 LNK4099 警告未阻止编译；未出现 Assimp 编译阻碍。

## 真实 GPU 与程序冒烟

启用 Validation 与同步验证，分别运行两种 RHI 线程模式：

| 检查 | 功能结果 | 严格退出结果 |
|---|---|---|
| 资源真实上传、Mesh 绘制回读、自动绑定、Uniform、多视口、Shader/材质重载与退出 | 功能断言完成 | 两模式均返回 1 |
| 全部有效迁移资源独立加载、EXR、Actor GUID、World 流式加载 | 功能断言完成 | 两模式均返回 1 |
| 远处 Chunk 的 World 打开及流式更新，不请求其 Actor | 断言通过 | 包含于迁移测试 |
| Editor 180 帧初始化/更新/退出 | 执行完成 | 两模式均返回 1 |
| Runtime 180 帧初始化/更新/退出 | 执行完成 | 两模式均返回 1 |

以上不代表零验证错误通过：

1. `ElaineVulkanDevice.cpp` 现有设备初始化设置 device layers，触发 `VUID-VkDeviceCreateInfo-enabledLayerCount-12384`，影响全部严格 GPU 检查。
2. Runtime 冒烟另外检测到 `ElaineVulkanSwapChain.cpp::CreateTextureView()` 对尚未 Acquire 的 swapchain images 录制布局转换及清除。该路径与实施前源码逐字节一致，本轮未扩展修改。
3. Editor 的 Vulkan/DX11 共享纹理导入失败后使用既有 staging/readback 回退。GBuffer 未使用输出警告及负向测试的非法 Shader/绑定错误单独记录。

`UniversalPBR.vs/.ps` 原本为空；独立加载测试仍跳过这两个无效 Shader 阶段，七个业务资产均实际加载。
日志为 `Intermediate/readable-resource-*.log`、`readable-migrated-*.log`、`readable-editor-*.log`、`readable-runtime-*.log`。

## 浏览器人工验收

本轮完成浏览器代码编译和 Editor 冒烟，尚未人工点击验证目录、筛选、依赖跳转和视觉呈现。自动化资源/注册表测试不代替此项验收。

## 复现

```powershell
$env:PATH = 'S:/VulkanSDK/Bin;' + $env:PATH
ctest --test-dir Build -C EditorDebug --output-on-failure
ctest --test-dir Build -C RuntimeDebug --output-on-failure
& Binaries/Win64/EditorDebug/ElaineAssetTool.exe validate Contents
python Tools/Assets/audit_migration.py Contents Intermediate/AssetMigrationBackup/42fae94b-85fb-4677-b699-fb69530af267 --output Build/readable-migration-audit.json
$env:VK_LAYER_VALIDATE_SYNC = '1'
& Binaries/Win64/EditorDebug/ElaineResourceTests.exe --gpu
& Binaries/Win64/EditorDebug/ElaineResourceTests.exe --gpu inline
& Binaries/Win64/EditorDebug/ElaineMigratedAssetTests.exe
& Binaries/Win64/EditorDebug/ElaineMigratedAssetTests.exe inline
& Binaries/Win64/EditorDebug/ElaineEditor.exe --resource-smoke
& Binaries/Win64/EditorDebug/ElaineEditor.exe --resource-smoke --rhi-inline
& Binaries/Win64/RuntimeDebug/ElaineGame.exe --resource-smoke
& Binaries/Win64/RuntimeDebug/ElaineGame.exe --resource-smoke --rhi-inline
```

两套 CTest 顺序执行，避免测试共用的 fixture 目录发生竞争。

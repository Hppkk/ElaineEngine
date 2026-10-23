# Vulkan 验证问题修复与复验

日期：2026-10-23。硬件：NVIDIA GeForce RTX 5070，驱动报告 Vulkan 1.4.312；引擎使用 Vulkan 1.3。Git 未暂存、未提交。

后续更新：第 6 项的临时 readback 处理已由实际 Vulkan/DX11 共享纹理实现替代，见 [VULKAN_DX11_INTEROP.md](VULKAN_DX11_INTEROP.md)。下文保留当时的修复及验证记录。

本记录接续 `READABLE_ASSETS_VALIDATION.md`，取代其中三个 Vulkan 问题的未解决状态。

## 修复内容

1. **Device layers**：`VkDeviceCreateInfo.enabledLayerCount = 0`，`ppEnabledLayerNames = nullptr`。保留 Instance 上的 Validation，不再触发 `VUID-VkDeviceCreateInfo-enabledLayerCount-12384`。
2. **Swapchain 首次使用**：创建时只包装图像、创建视图，不录制清除、布局转换或提交。每张图像初始为 `UNDEFINED / Unknown`；Acquire 后代理同步到实际图像状态，第一次使用才录制转换。`VK_SUBOPTIMAL_KHR` 仍视为成功取得图像。
3. **Acquire 同步与 Present**：Acquire semaphore 等待覆盖 `ALL_COMMANDS`，包含首次布局转换；初始布局的 source access 使用 Unknown。RenderPass 不再隐式切换交换链图像到 Present，统一沿用调用者/RenderGraph 的显式 Present barrier，避免实际布局与跟踪状态分离。
4. **交换链所有权和尺寸**：交换链持有 backbuffer 与代理的 RHI 强句柄，移除与命令持有方式冲突的直接删除；释放继续经过原有回收路径。创建尺寸采用 surface 的固定 `currentExtent`，非固定尺寸则限制在 surface 范围内。
5. **GBuffer 输出警告**：没有附件格式的材质先创建 Shader、反射、描述符布局和 Pipeline layout，原生 PSO 沿用现有 `RHIPrepareGfxPipeline`，在 RenderThread 同步窗口按实际 RenderPass 准备，不再先创建一个假定单颜色目标的 PSO。显式多目标 Pipeline 的 blend attachment 数量同步到目标数量。
6. **Vulkan/DX11 互操作**：当前导出的 `OPAQUE_WIN32` 分配句柄不能当作 DXGI 纹理句柄打开；原桥接还缺少跨 API GPU 同步和队列所有权交接。因此 Vulkan 编辑器明确选用既有 fence 检查的双缓冲 staging/readback 路径，不再创建无用的 External 离屏纹理或尝试 `OpenSharedResource*`。其他后端的既有分支保留。

第 6 项是明确选择安全回退，**没有实现 Vulkan/DX11 零拷贝互操作**。若后续建设，应同时处理正确的共享纹理导入方向、设备匹配、外部同步、所有权和关闭生命周期，不能只替换 HANDLE 类型。

未更换通用 RHI 接口、命令宏、分配器、命令链、排序或执行器；未改变资源格式或 Contents 内容。DeferredLighting 完整绘制仍不在本轮范围内。

## 自动化与实际 GPU 验证

| 检查 | 结果 |
|---|---|
| EditorDebug：Editor、Core、Engine、资源工具及全部测试目标 | 编译通过 |
| RuntimeDebug：Game、Core、Engine、资源工具及全部测试目标 | 编译通过 |
| EditorDebug CTest | 6/6 通过 |
| RuntimeDebug CTest | 6/6 通过 |
| 两配置 × 两种 RHI 模式：资源 GPU 测试、迁移资源加载、程序 180 帧冒烟 | 共 12 组全部返回 0；零 Vulkan 验证错误、零验证警告、零同步 hazard |
| 资产独立审计 | 20 个资产的 ID、引用、JSON/Mesh 数据及内嵌原图全部通过 |

`ResourceTests.cpp` 新增两项实际 GPU 回归：

- 独立于场景资源是否已就绪，连续执行 16 次 Acquire → clear RenderPass → 显式 Present 转换 → Present，覆盖首次与重复使用、提交、引用保活和关闭。
- 三颜色输出 Shader，分别验证未指定附件格式及显式 MRT 格式的 Pipeline；实际绘制后逐附件回读 RGB 和 Alpha 像素。

保留的负向测试日志：非法 Shader 编译错误、缺失/越界/用途错误的 `CustomData`、类型错误的 `CubeMask`。测试明确断言这些失败，不将业务校验错误混作 Vulkan Validation 失败，也没有屏蔽 Validation。

未做人工编辑器点击、视觉质量或不同硬件的互操作验收。程序冒烟与 GPU 像素回读分别报告，不代替人工交互。

## 日志与复现

- 编译：`Intermediate/vulkan-final-build-editor.log`、`vulkan-final-build-runtime.log`。
- CTest：`Intermediate/vulkan-final-tests-editor.log`、`vulkan-final-tests-runtime.log`。
- GPU 总表：`Intermediate/vulkan-validation-results.json`。
- 各组原始日志：`Intermediate/vulkan-validation-{EditorDebug|RuntimeDebug}-{resource|migrated|app}-{dedicated|inline}.log`。
- 资产审计：`Build/vulkan-fix-migration-audit.json`。
- 本轮修改前快照：`Intermediate/VulkanValidationBackup/20261023-152455/`。

当前 SDK 的同步验证设置采用以下实际支持的变量；旧报告中的 `VK_LAYER_VALIDATE_SYNC` 不作为本轮启用同步验证的依据。

```powershell
$env:PATH = 'S:/VulkanSDK/Bin;' + $env:PATH
$env:VK_KHRONOS_VALIDATION_VALIDATE_SYNC = 'true'
$env:VK_KHRONOS_VALIDATION_SYNCVAL_SHADER_ACCESSES_HEURISTIC = 'true'
ctest --test-dir Build -C EditorDebug --output-on-failure
ctest --test-dir Build -C RuntimeDebug --output-on-failure
& Binaries/Win64/EditorDebug/ElaineResourceTests.exe --gpu
& Binaries/Win64/EditorDebug/ElaineResourceTests.exe --gpu inline
& Binaries/Win64/EditorDebug/ElaineMigratedAssetTests.exe
& Binaries/Win64/EditorDebug/ElaineMigratedAssetTests.exe inline
& Binaries/Win64/EditorDebug/ElaineEditor.exe --resource-smoke
& Binaries/Win64/EditorDebug/ElaineEditor.exe --resource-smoke --rhi-inline
# RuntimeDebug 同样运行 ResourceTests、MigratedAssetTests，程序使用 ElaineGame.exe。
```

两套 CTest 和资源 GPU 测试依次运行，避免共用 fixture 的竞争。

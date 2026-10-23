# Vulkan / DX11 编辑器共享纹理

日期：2026-10-23。实际验证设备：NVIDIA GeForce RTX 5070，Vulkan 1.4.312 / 引擎 Vulkan 1.3。

本记录接续 `VULKAN_VALIDATION_FIXES.md`：此前第 6 项仅使用 readback 的临时处理，已由真正的共享纹理路径替代。设备 layers、交换链 Acquire / Present、MRT 的已有修复继续保留。Git 未暂存、未提交。

## 实现

- ImGui DX11 Device 根据已选 Vulkan 设备的 LUID 创建。匹配设备无法创建 DX11 Device 时允许使用默认 DX11 Device；共享初始化会再次检查 LUID，跨显卡只允许 readback。
- DX11 创建三张 RGBA8 UNORM、2D、单层 / 单 mip / 单采样纹理，使用 `SHARED_NTHANDLE | SHARED_KEYEDMUTEX`，将 `CreateSharedHandle()` 返回的 NT 句柄交给 Vulkan 导入。
- Vulkan 后端检查实际格式 / usage 的 `D3D11_TEXTURE_BIT` 导入能力，使用 dedicated allocation，并取 Vulkan image 与 Win32 handle 的 memory type 交集。NT 句柄在导入后关闭，导入内存进入既有分配统计及延迟删除路径。
- Forward / Deferred 的现有离屏最终颜色输出直接使用目标纹理；真正的 swapchain 继续沿用原来的复制与 Present。空场景也会清屏提交。未补建尚未完成的 DeferredLighting。
- 原有命令宏、malloc / placement new、命令链、排序、交换及执行器保持原样。共享资源仍使用真实 `RHITextureHandle`，命令缓冲继续持有到实际 fence 完成。
- 平台接口位于 `ElaineVulkanDX11Interop.h`，编辑器槽位及显示管理位于 `EditorViewportInterop`。通用 RHI 没有加入 Vulkan、DX11 或 Win32 类型。

## 同步与生命周期

每张纹理独立使用 key `0 → 1 → 0`：

1. Vulkan 实际使用时登记到当前命令缓冲，在原 `vkQueueSubmit()` 上 acquire `0`。
2. 录制 `EXTERNAL → graphics` 所有权交接与附件布局转换。命令缓冲结束时统一转换到 `GENERAL`，执行 `graphics → EXTERNAL` 交接，并在同一提交 release `1`。
3. GameThread 只对已经结束 CPU 提交的槽位尝试 `AcquireSync(1, 0)`，按帧号从新到旧选择。只有 `S_OK` 表示取得所有权，`WAIT_TIMEOUT` 保留原显示，`WAIT_ABANDONED` / 其他错误结束运行，不切换成回读掩盖故障。
4. 当前显示纹理由 DX11 持续持有；主窗口及所有 ImGui 平台窗口绘制结束后，解绑旧 SRV、Flush，再 release `0`。被丢弃的旧完成帧同样归还。
5. 三个槽位全部占用时，仅跳过该视口本帧渲染，World 和 UI 继续运行。

提交成功只表示 GPU 工作已提交；keyed mutex 确认跨 API 可见性，Vulkan fence 与 DX11 event query 分别确认旧代次的两端使用结束。普通帧没有 `WaitIdle()`；关闭时才停止生产并排空两端工作。

每次预留有独立 CPU 票据。列表放弃、未执行及部分执行后失败均结束该票据；部分提交的失败画面不会发布，但仍取得并归还其 mutex，避免耗尽槽位。重建和零尺寸暂停会退休旧代次，新纹理完成前显示为空，面板不会借用已退休的 SRV。

## 自动验证结果

最终矩阵共 **38/38** 项通过，日志中零 Vulkan Validation 错误、警告及同步 hazard。共享 GPU 测试同时检查 DX11 Debug Layer 的 warning / error。

| 类别 | 结果 |
|---|---|
| EditorDebug 全部默认目标 | 编译通过 |
| RuntimeDebug：Game、Core、Engine、资源工具及全部测试目标 | 编译通过 |
| 两配置 CTest | 各 6/6，通过资源、TaskGraph、Shader 参数和输入回归 |
| 两配置 × 两种 RHI 模式的共享 GPU 测试 | 4/4，通过真实 Vulkan 绘制 → DX11 采样 → 输出像素核对 |
| 两配置 × 两种 RHI 模式的资源 GPU / 迁移资产测试 | 8/8，包含既有上传、重载、WSI、MRT、回收回归 |
| Editor 两种 RHI 模式 | 22/22，包含默认共享、强制共享、readback、重建、暂停恢复、最小化恢复及错误注入 |
| Runtime Game 两种 RHI 模式 | 2/2，180 帧冒烟通过 |

共享 GPU 测试覆盖：

- 三槽耗尽与跳帧、取消、失败、未执行列表、已录制部分命令后的真实异常；失败画面归还且不显示。
- 最新完成帧、重复显示旧帧、旧 ready 帧归还、连续变化的双色 Shader 输出。
- **两个真实 ImGui 浮动平台窗口**同时采样共享 SRV，平台窗口创建数量与跨 API 像素均有断言。
- 在 Vulkan 提交与 DX11 显示都存在时重建，零尺寸暂停、恢复、重复重建、旧代次退休和重复关闭。
- Adapter 不匹配、能力缺失、导入失败、mutex 超时与失效的注入。自动模式正确回读回退；强制共享和运行期故障按预期返回非零，不误报成功。

正常共享路径的 viewport readback 请求数为 **0**。像素测试只回读 DX11 采样后的测试输出，不对共享 Vulkan 图像做 readback，也没有通过额外 GPU copy 桥接画面。

负向测试中的非法 Shader、非法材质绑定及 `Expected interop partial-batch failure` 是明确断言的预期错误，不属于 Vulkan 验证失败。

### 独立构建限制

直接执行 RuntimeDebug 的 `ALL_BUILD` 还会构建 ElaineEditor。现有生成文件中的编辑器 drawer 会访问 Actor / Component 的私有成员，而该配置没有 `_HAS_EDITOR_` 生成的 friend 声明，因此产生 `C2248`。本轮没有修改生成文件或反射配置；Runtime 所需目标和测试已单独构建通过。

尚未完成由人操作的编辑器画面、拖拽和布局验收。自动浮动窗口与像素验证不等同于人工交互验收；也未将本机结果推广为其他 GPU / 驱动的共享兼容性结果。

## 复现

```powershell
$env:PATH = 'S:/VulkanSDK/Bin;' + $env:PATH
$env:VK_KHRONOS_VALIDATION_VALIDATE_SYNC = 'true'
$env:VK_KHRONOS_VALIDATION_SYNCVAL_SHADER_ACCESSES_HEURISTIC = 'true'

cmake --build Build --config EditorDebug --parallel 1
cmake --build Build --config RuntimeDebug --target ElaineGame ElaineAssetTool ElaineResourceTests ElaineTaskGraphTests ElaineShaderParameterTests ElaineInputRouterRegression ElaineWindowsInputRegression ElaineImGuiInputRegression ElaineMigratedAssetTests ElaineViewportInteropTests --parallel 1

ctest --test-dir Build -C EditorDebug --output-on-failure
ctest --test-dir Build -C RuntimeDebug --output-on-failure
& Binaries/Win64/EditorDebug/ElaineViewportInteropTests.exe
& Binaries/Win64/EditorDebug/ElaineViewportInteropTests.exe inline
# RuntimeDebug 同样运行上述共享测试。

& Binaries/Win64/EditorDebug/ElaineEditor.exe --viewport-interop-stress --viewport-interop=shared
& Binaries/Win64/EditorDebug/ElaineEditor.exe --viewport-interop-stress --viewport-interop=shared --rhi-inline
& Binaries/Win64/EditorDebug/ElaineEditor.exe --viewport-interop-stress --viewport-interop=readback
& Binaries/Win64/EditorDebug/ElaineEditor.exe --resource-smoke --viewport-interop=auto --viewport-interop-fault=import
# 下例预期退出码为 1，不允许偷偷回退：
& Binaries/Win64/EditorDebug/ElaineEditor.exe --resource-smoke --viewport-interop=shared --viewport-interop-fault=import
```

未指定 `--viewport-interop` 时默认为 `auto`。错误注入值为 `adapter`、`capability`、`import`、`mutex`、`timeout`，仅供显式回归；正常运行不启用。自动 smoke 禁止保存 ImGui ini，避免覆盖用户布局。

证据：`Intermediate/ViewportInteropValidation/results.json` 及同目录逐项日志；最终构建日志为 `Intermediate/interop-final-build-editor.log`、`interop-final-build-runtime.log`；Runtime ALL_BUILD 的独立错误保存在 `Intermediate/interop-full-build-runtime.log`。实施前逐文件备份保存在 `Intermediate/VulkanDX11InteropBackup/`。

# RenderDoc Vulkan 截帧

编辑器的场景由 Vulkan 渲染，ImGui 界面由 DX11 显示。普通 Present 截帧可能选中 DX11；本模块通过 Vulkan Instance 明确选择场景 API，在实际执行线程捕获一个视图帧的录制与提交，不依赖 Vulkan Present。

## 启用

`ELAINE_WITH_RENDERDOC` 默认开启，仅在 `EditorDebug`、`RuntimeDebug` 提供有效实现。无需链接、复制或发布 RenderDoc DLL，构建只使用附带 MIT 许可的官方 `ThirdParty/RenderDoc/renderdoc_app.h`。

可以在 RenderDoc 中启动 `ElaineEditor.exe`，也可以直接运行：

```powershell
.\Binaries\Win64\EditorDebug\ElaineEditor.exe --renderdoc
.\Binaries\Win64\EditorDebug\ElaineEditor.exe "--renderdoc=C:\Program Files\RenderDoc\renderdoc.dll"
```

普通启动只检测已注入的 RenderDoc，不主动加载。`--renderdoc` 查询本机安装信息；显式 DLL 路径必须为绝对路径。需要 RenderDoc Application API 1.6 或更高版本。图形设备创建后不支持补加载，必须重新启动。

主动加载使用同目录的 `renderdoc.json` 配置当前进程的 Vulkan 隐式捕获层，不写系统注册表；不会把 RenderDoc 放进 `enabledLayerNames`。启动时查询所选物理设备的 Vulkan tooling 信息，确认捕获层实际生效。显式启用失败会报告原因并以非零状态退出。

## 编辑器操作

打开 `Tools → RenderDoc`：

- `Capture Next Vulkan Frame`：等待下一次实际场景帧，完成后显示 `.rdc` 路径。
- `Open Last Capture`：使用当前 RenderDoc 安装打开最近的捕获。
- 视口隐藏或没有场景帧时，请求保持等待；重复请求合并。外部捕获尚未结束时延后处理，关闭时取消未完成请求。

引擎连接后关闭 RenderDoc 默认截帧快捷键，避免 F12 再次选择 DX11；请使用上述菜单。一次捕获对应一个视图帧，不合并 DX11 UI 与 Vulkan 场景。Pass 名称通过 Vulkan debug labels 显示。

## 自动截帧与输出

编辑器和 Runtime 均支持：

```powershell
.\Binaries\Win64\EditorDebug\ElaineEditor.exe --renderdoc --renderdoc-capture-frame=120
.\Binaries\Win64\RuntimeDebug\ElaineGame.exe --renderdoc --renderdoc-capture-frame=120 --rhi-inline
```

- `--renderdoc-capture-frame=N`：在第 N 个实际 Vulkan 场景帧捕获，N 从 1 开始，不计算资源上传批次。若外部捕获占用，则顺延到可用场景帧。
- `--renderdoc-output=<目录>`：指定输出目录，默认项目 `Intermediate/RenderDoc/`；相对路径按当前工作目录解析。
- 从 RenderDoc 启动时可以只传截帧及输出参数，无需重复传 `--renderdoc`。
- 同一目录多次捕获的实际文件名由 RenderDoc 分配，以菜单或日志返回的路径为准。
- 自动截帧未完成就退出时，Editor、Runtime 返回非零状态，不将启动成功当作捕获成功。

## 共享纹理与排查

检测到 RenderDoc 后，编辑器的 `--viewport-interop=auto` 在初始化阶段使用现有 readback 路径，避免捕获依赖外部共享内存。普通启动仍优先共享纹理。显式 `shared`、`readback` 不会被覆盖；不在点击截帧时切换传输模式。

共享路径的外部内存和 keyed mutex 是否能捕获、独立回放，取决于 RenderDoc 支持情况；必须分别验收。建议先在默认 readback 调试模式验证 Vulkan 绘制，再专项排查共享互操作。

若菜单不可用，检查状态说明与启动日志：

- 未安装或安装路径不匹配：传完整 DLL 路径，并确认同目录存在匹配的 `renderdoc.json`。
- DLL 已加载但 Vulkan 未挂接：检查 Vulkan 捕获层安装、当前进程的层禁用环境变量及其他图形工具冲突；不能只看 DLL 是否存在。
- 只有 DX11 调用：确认打开的是引擎菜单产生的 Vulkan 捕获，文件 API 应显示 `Vulkan`。
- 捕获成功但回放失败：先使用 `--viewport-interop=readback`，确认图形驱动及 RenderDoc 版本；保留原始捕获和错误日志。

模块不添加 GPU 空闲等待，不改变 RHI 命令宏、分配、排序或 fence 回收；RenderDoc 自身可能在截帧期间产生调试开销。

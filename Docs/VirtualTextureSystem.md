# ElaineEngine Virtual Texture System / 虚拟纹理系统

> **Version**: 1.0  
> **Last Updated**: 2026-04-01  
> **Module**: `ElaineCore/VirtualTexture`

---

## Table of Contents / 目录

1. [Overview / 概述](#1-overview--概述)
2. [Architecture / 整体架构](#2-architecture--整体架构)
3. [Core Data Structures / 核心数据结构](#3-core-data-structures--核心数据结构)
4. [SVT: Streaming Virtual Texture / 流式虚拟纹理](#4-svt-streaming-virtual-texture--流式虚拟纹理)
5. [RVT: Runtime Virtual Texture / 运行时虚拟纹理](#5-rvt-runtime-virtual-texture--运行时虚拟纹理)
6. [Multi-Thread Model / 多线程模型](#6-multi-thread-model--多线程模型)
7. [Shader Sampling / Shader 采样](#7-shader-sampling--shader-采样)
8. [Material Integration / 材质系统集成](#8-material-integration--材质系统集成)
9. [Deferred Pipeline Integration / 延迟管线集成](#9-deferred-pipeline-integration--延迟管线集成)
10. [File Format (.evt) / 文件格式](#10-file-format-evt--文件格式)
11. [Debug Visualization / 调试可视化](#11-debug-visualization--调试可视化)
12. [File Inventory / 文件清单](#12-file-inventory--文件清单)

---

## 1. Overview / 概述

### English

The **Virtual Texture (VT)** system in ElaineEngine allows rendering of extremely large textures (e.g., 16K×16K or larger) without loading the entire texture into GPU memory. Instead, only the tiles currently visible on screen are loaded, dramatically reducing memory usage.

The system supports two modes:

| Mode | Full Name | Description |
|------|-----------|-------------|
| **SVT** | Streaming Virtual Texture | Tiles are loaded from disk (`.evt` files) on demand |
| **RVT** | Runtime Virtual Texture | Tiles are rendered on-the-fly by capturing scene content |

Both modes share the same underlying infrastructure: **Page Table**, **Physical Tile Pool**, **Indirection Texture**, and **Feedback Buffer**.

### 中文

ElaineEngine 的 **虚拟纹理 (Virtual Texture, VT)** 系统允许渲染超大纹理（如 16K×16K 或更大），而无需将整张纹理加载到 GPU 显存中。系统只加载当前屏幕可见的 Tile（瓦片），从而大幅降低显存占用。

系统支持两种模式：

| 模式 | 全称 | 说明 |
|------|------|------|
| **SVT** | 流式虚拟纹理 | 按需从磁盘（`.evt` 文件）加载 Tile |
| **RVT** | 运行时虚拟纹理 | 运行时通过捕获场景内容实时渲染 Tile |

两种模式共享相同的底层基础设施：**页表 (Page Table)**、**物理 Tile 池 (Physical Tile Pool)**、**间接纹理 (Indirection Texture)** 和 **反馈缓冲区 (Feedback Buffer)**。

---

## 2. Architecture / 整体架构

### System Block Diagram / 系统框图

```
┌──────────────────────────────────────────────────────────────────┐
│                    VirtualTextureSystem (Singleton)               │
│                    虚拟纹理系统（单例管理器）                        │
│                                                                   │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────────────┐ │
│  │ VTSpace [0]  │  │ VTSpace [1]  │  │ VTSpace [N]            │ │
│  │ (SVT 4K×4K) │  │ (RVT 8K×8K) │  │ ...                    │ │
│  │  ┌─────────┐│  │  ┌──────────┐│  │  Max 16 Spaces         │ │
│  │  │PageTable││  │  │PageTable ││  │                         │ │
│  │  └─────────┘│  │  └──────────┘│  └─────────────────────────┘ │
│  └─────────────┘  └──────────────┘                               │
│                                                                   │
│  ┌───────────────────────────────────────────────────────────┐   │
│  │              Physical Tile Pool (共享)                     │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐    │   │
│  │  │ Atlas L0 │ │ Atlas L1 │ │ Atlas L2 │ │ Atlas L3 │    │   │
│  │  │BaseColor │ │ Normal   │ │ RMA      │ │ Emissive │    │   │
│  │  │4352×4352 │ │4352×4352 │ │4352×4352 │ │4352×4352 │    │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘    │   │
│  │  32×32 tiles = 1024 slots, LRU eviction                  │   │
│  └───────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────────┐  ┌──────────────────┐                     │
│  │ FeedbackAnalyzer │  │ StreamingManager │                     │
│  │ (GPU Readback)   │  │ (Async I/O)      │                     │
│  └──────────────────┘  └──────────────────┘                     │
└──────────────────────────────────────────────────────────────────┘
```

### Per-Frame Update Pipeline / 每帧更新流水线

```
Frame N:
┌──────────┐    ┌─────────────────┐    ┌──────────────────┐
│ 1. Analyze│───▶│ 2. Process Tile │───▶│ 3. Update Page   │
│ Feedback  │    │    Requests     │    │    Tables         │
│ (CPU)     │    │ (Priority Sort) │    │ (CPU → State)     │
└──────────┘    └─────────────────┘    └──────────────────┘
                                              │
┌──────────────────┐    ┌──────────────┐      │
│ 5. Render        │◀───│ 4. Update    │◀─────┘
│ Feedback Pass    │    │ Indirection  │
│ (Next frame src) │    │ Texture (GPU)│
└──────────────────┘    └──────────────┘
```

**English**: Each frame, the system reads back the feedback buffer from the *previous* frame, analyzes which tiles are needed, loads/renders them, updates the page table and indirection texture, then renders a new feedback pass for the *next* frame.

**中文**：每一帧，系统读回*上一帧*的反馈缓冲区，分析需要哪些 Tile，加载/渲染它们，更新页表和间接纹理，然后为*下一帧*渲染新的反馈 Pass。这形成了一个延迟一帧的反馈循环。

---

## 3. Core Data Structures / 核心数据结构

### 3.1 VTTileCoord — Tile 坐标

```
English: Uniquely identifies a tile in the virtual texture hierarchy.
中文：唯一标识虚拟纹理层级中的一个 Tile。

┌──────────────────────────────────────────┐
│ VTTileCoord                              │
│   X        : uint16  (Tile X at this mip)│
│   Y        : uint16  (Tile Y at this mip)│
│   MipLevel : uint8   (0 = highest detail)│
│   SpaceID  : uint8   (VT space index)    │
└──────────────────────────────────────────┘

Packed into 32 bits for GPU feedback:
  [SpaceID:4][MipLevel:4][Y:12][X:12]

Example / 示例:
  4096×4096 texture, TileSize=128 → Mip0 has 32×32 = 1024 tiles
  Tile (5, 3) at Mip 2 in Space 0 → Pack: 0x02003005
```

### 3.2 PhysicalTileLocation — 物理位置

```
English: Where a tile lives in the physical atlas texture.
中文：Tile 在物理 Atlas 纹理中的位置。

┌──────────────────────────────────────────┐
│ PhysicalTileLocation                     │
│   AtlasX    : uint16 (grid position X)   │
│   AtlasY    : uint16 (grid position Y)   │
│   PoolIndex : uint8  (which pool)        │
└──────────────────────────────────────────┘

Pixel position = AtlasX × TileSizeWithBorder
               = AtlasX × (128 + 4×2)
               = AtlasX × 136
```

### 3.3 IndirectionEntry — 间接纹理条目

```
English: Stored in the Indirection Texture (R16G16B16A16_UINT).
         The GPU shader reads this to find the physical atlas UV.
中文：存储在间接纹理中（R16G16B16A16_UINT 格式）。
      GPU Shader 通过读取此数据找到物理 Atlas 的 UV 坐标。

┌────────────────────────────────────────────┐
│ IndirectionEntry (per texel)               │
│   R = PhysicalTileX   (uint16)             │
│   G = PhysicalTileY   (uint16)             │
│   B = [PoolIndex:8 | MipBias:8]  (uint16)  │
│   A = Flags (bit 0 = IsResident) (uint16)   │
└────────────────────────────────────────────┘
```

### 3.4 Constants / 常量

| Constant | Value | Description (EN) | 说明 (CN) |
|----------|-------|-------------------|-----------|
| `TileSize` | 128 px | Content pixels per tile | 每个 Tile 的内容像素 |
| `TileBorderSize` | 4 px | Border for bilinear filtering | 双线性滤波的边框像素 |
| `TileSizeWithBorder` | 136 px | Total tile footprint | 含边框的 Tile 总尺寸 |
| `PhysicalAtlasSizeInTiles` | 32 | Atlas grid dimension | Atlas 网格尺寸 |
| `PhysicalAtlasSize` | 4352 px | Atlas pixel dimension (32×136) | Atlas 像素尺寸 |
| `MaxLayers` | 4 | BaseColor, Normal, RMA, Emissive | 最多 4 层纹理 |
| `MaxSpaces` | 16 | Max VT spaces | 最多 16 个 VT 空间 |
| `FeedbackDownscaleFactor` | 8 | Feedback RT is 1/8 resolution | 反馈 RT 为 1/8 分辨率 |
| `MaxTileUploadsPerFrame` | 64 | Upload budget per frame | 每帧最多上传 64 个 Tile |

---

## 4. SVT: Streaming Virtual Texture / 流式虚拟纹理

### 4.1 How It Works / 工作原理

```
English:
SVT tiles are pre-baked into .evt files offline. At runtime, the system
loads only the tiles visible on screen, streaming them from disk into
the Physical Tile Pool.

中文：
SVT 的 Tile 数据由离线工具预烘焙到 .evt 文件中。运行时，系统仅加载
屏幕上可见的 Tile，从磁盘流式加载到物理 Tile 池中。
```

### 4.2 SVT Data Flow / SVT 数据流

```
                    ┌─────────────┐
                    │   .evt File │  ← Offline baked / 离线烘焙
                    │  (on disk)  │
                    └──────┬──────┘
                           │ Async I/O (Worker Thread)
                           │ 异步 I/O（工作线程）
                           ▼
                    ┌─────────────────┐
                    │ VTStreamingMgr  │
                    │ ┌─────────────┐ │
                    │ │ PriorityQ   │ │  ← Sorted by screen coverage
                    │ │ (min-heap)  │ │     按屏幕覆盖面积排序
                    │ └──────┬──────┘ │
                    │        │        │
                    │ ┌──────▼──────┐ │
                    │ │ LoadTileTask│ │  ← TaskGraph worker thread
                    │ │ (decompress)│ │     TaskGraph 工作线程
                    │ └──────┬──────┘ │
                    │        │        │
                    │ ┌──────▼──────┐ │
                    │ │ CompletedQ  │ │  ← Ready for upload
                    │ │ (staging)   │ │     等待上传
                    │ └─────────────┘ │
                    └────────┬────────┘
                             │ Render Thread: ProcessCompletedTiles()
                             │ 渲染线程：处理已完成的 Tile
                             ▼
┌─────────────────────────────────────────────────┐
│              Physical Tile Pool                  │
│  ┌────┬────┬────┬────┬────┬────┬────┬────┐     │
│  │T(0)│T(1)│T(2)│    │    │    │    │... │     │
│  │    │    │    │FREE│FREE│    │    │    │     │
│  └────┴────┴────┴────┴────┴────┴────┴────┘     │
│  LRU eviction when pool is full                  │
│  池满时使用 LRU 策略驱逐最久未用的 Tile            │
└─────────────────────────────────────────────────┘
```

### 4.3 Feedback Buffer System / 反馈缓冲区系统

```
English:
The Feedback Buffer is the key mechanism that tells the CPU which tiles
the GPU actually needs. It works as follows:

中文：
反馈缓冲区是告诉 CPU "GPU 实际需要哪些 Tile" 的关键机制，工作流程如下：

Step 1: Feedback Pass (GPU, 1/8 resolution)
步骤 1：反馈 Pass（GPU，1/8 分辨率渲染）
┌─────────────────────────────────────────────┐
│  For each pixel:                             │
│    1. Compute virtual UV                     │
│    2. Compute mip level from dFdx/dFdy       │
│    3. Pack (SpaceID, Mip, TileX, TileY)      │
│    4. Output as R32_UINT                     │
│                                              │
│  对每个像素：                                  │
│    1. 计算虚拟 UV                             │
│    2. 从 dFdx/dFdy 计算 Mip Level             │
│    3. 打包 (SpaceID, Mip, TileX, TileY)       │
│    4. 输出为 R32_UINT 格式                     │
└─────────────────────────────────────────────┘
                    │
                    ▼ GPU Readback (1 frame latency)
                      GPU 回读（延迟 1 帧）
Step 2: CPU Analysis (VTFeedbackAnalyzer)
步骤 2：CPU 端分析（VTFeedbackAnalyzer）
┌─────────────────────────────────────────────┐
│  1. Read back feedback RT pixels             │
│  2. Unpack each pixel → VTTileCoord          │
│  3. Deduplicate (hash set)                   │
│  4. Count pixels per tile (= screen area)    │
│  5. Sort by priority (Critical > High > ...)  │
│  6. Output: vector<VTTileRequest>            │
│                                              │
│  1. 读回反馈 RT 像素数据                       │
│  2. 解包每个像素 → VTTileCoord                 │
│  3. 去重（哈希集合）                            │
│  4. 统计每个 Tile 的像素数（= 屏幕覆盖面积）     │
│  5. 按优先级排序                               │
│  6. 输出：vector<VTTileRequest>               │
└─────────────────────────────────────────────┘
```

### 4.4 Page Table & Tile States / 页表与 Tile 状态

```
English: Each tile can be in one of these states:
中文：每个 Tile 可以处于以下状态之一：

  NotLoaded ──▶ Pending ──▶ Loading ──▶ Uploading ──▶ Resident
      ▲                                                  │
      │                                                  │
      └──────────── Evicted ◀────────────────────────────┘
                  (LRU policy / LRU 策略)

State Machine / 状态机:
  NotLoaded  : Tile has never been requested / 从未被请求
  Pending    : Request submitted, waiting in queue / 已提交请求，等待排队
  Loading    : I/O in progress (worker thread) / I/O 进行中（工作线程）
  Uploading  : Data ready, uploading to GPU / 数据就绪，上传到 GPU
  Resident   : In physical pool, mapped in indirection / 在物理池中，已映射
  Evicted    : Removed from pool to make room / 被驱逐以腾出空间
```

---

## 5. RVT: Runtime Virtual Texture / 运行时虚拟纹理

### 5.1 Concept / 概念

**English**: Unlike SVT where tile data comes from files, RVT **renders** tile content at runtime by capturing the scene from a top-down orthographic camera. This is ideal for terrain blending, large-area decals, and landscape painting — any use case where the texture content is generated from the 3D scene itself.

**中文**：与 SVT 从文件加载 Tile 数据不同，RVT 在运行时通过从顶向下的正交相机**渲染**捕获场景内容来生成 Tile。这非常适合地形混合、大面积贴花和景观绘制——任何纹理内容由 3D 场景本身生成的场景。

### 5.2 RVT Architecture / RVT 架构

```
┌─────────────────────────────────────────────────────────────┐
│                 Logic Thread (逻辑线程)                       │
│                                                              │
│   RVTVolumeComponent (ElaineEngine 模块)                     │
│   ├─ Config: WorldBounds, Resolution, NumLayers              │
│   ├─ mRenderProxy* ──── bridge ────────────┐                │
│   ├─ SetWorldBounds()   → ENQUEUE ─────┐   │                │
│   ├─ InvalidateRegion() → ENQUEUE ─┐   │   │                │
│   └─ MarkRenderStateDirty() ───┐   │   │   │                │
│                                │   │   │   │                │
└────────────────────────────────┼───┼───┼───┼────────────────┘
                                 │   │   │   │
                  ENQUEUE_RENDER_COMMAND (值拷贝)
                                 │   │   │   │
┌────────────────────────────────┼───┼───┼───┼────────────────┐
│                 Render Thread (渲染线程)      │                │
│                                │   │   │   │                │
│   RVTRenderProxy (ElaineCore 模块)  ◀──┘   │                │
│   ├─ RuntimeVirtualTexture*                                  │
│   │   ├─ VTSpaceDesc (world bounds, resolution)              │
│   │   ├─ SetWorldBounds()  ◀───────────────┘                │
│   │   ├─ InvalidateWorldRegion() ◀─────────┘                │
│   │   └─ InvalidateAll()                                     │
│   │                                                          │
│   ├─ RVTTileRenderer*                                        │
│   │   ├─ Ortho Camera Setup                                  │
│   │   ├─ MRT Render Target (4 layers)                        │
│   │   └─ RenderPendingTiles(CmdList, RVT)                   │
│   │                                                          │
│   └─ UpdateTiles(CmdList)  ← called by RenderPipeline       │
└──────────────────────────────────────────────────────────────┘
```

### 5.3 RVT Tile Rendering / RVT Tile 渲染流程

```
English: When a tile is needed, the RVTTileRenderer captures the scene:
中文：当需要某个 Tile 时，RVTTileRenderer 捕获场景内容：

Step 1: Compute tile world bounds / 计算 Tile 的世界空间范围
┌──────────────────────────────────────────────────────┐
│  TileWorldMinX = VolumeMinX + TileX × TileWorldSize  │
│  TileWorldMinY = VolumeMinY + TileY × TileWorldSize  │
│  TileWorldSize = VolumeSize / TileCount_at_Mip       │
│                                                       │
│  + Extend by TileBorderSize for seamless filtering    │
│  + 扩展 TileBorderSize 像素以确保无缝滤波              │
└──────────────────────────────────────────────────────┘

Step 2: Setup orthographic camera / 设置正交相机
┌──────────────────────────────────────────────────────┐
│  Camera looks straight down (Y-axis)                  │
│  相机从正上方向下看（Y 轴）                             │
│                                                       │
│  OrthoProjection(left, right, bottom, top, near, far) │
│  ViewMatrix = LookAt(center + up, center, forward)    │
│                                                       │
│  Viewport = TileSizeWithBorder × TileSizeWithBorder   │
│           = 136 × 136 pixels                          │
└──────────────────────────────────────────────────────┘

Step 3: Render to MRT / 渲染到多渲染目标
┌──────────────────────────────────────────────────────┐
│  Using RVTCapture.vs + RVTCapture.ps shaders:         │
│  使用 RVTCapture 着色器：                               │
│                                                       │
│  MRT Output:                                          │
│    RT0 → BaseColor.rgb + Metallic.a                   │
│    RT1 → Normal.rgb + Roughness.a                     │
│    RT2 → Roughness.r + Metallic.g + AO.b              │
│    RT3 → Emissive.rgb                                 │
│                                                       │
│  Objects render their material properties directly,   │
│  NO lighting applied (material properties only).      │
│  物体直接输出材质属性，不应用光照。                       │
└──────────────────────────────────────────────────────┘

Step 4: Copy to Physical Atlas / 拷贝到物理 Atlas
┌──────────────────────────────────────────────────────┐
│  CopyTextureRegion(                                   │
│    src = temp_RT[layer],                              │
│    dst = PhysicalAtlas[layer],                        │
│    dstOffset = PhysicalTileLocation.GetPixelXY()      │
│  )                                                    │
│                                                       │
│  Then update PageTable → Resident                     │
│  Then update IndirectionTexture                       │
│  然后更新页表状态为 Resident，更新间接纹理               │
└──────────────────────────────────────────────────────┘
```

### 5.4 RVT Invalidation / RVT 失效机制

**English**: When scene content changes (e.g., a tree falls down, snow accumulates), affected tiles must be re-rendered. The system supports:
- **Region invalidation**: Only tiles overlapping the changed world-space region are re-rendered
- **Full invalidation**: All tiles are marked dirty (used when volume bounds change)

**中文**：当场景内容发生变化（如树木倒下、积雪堆积）时，受影响的 Tile 必须重新渲染。系统支持：
- **区域失效**：仅重新渲染与变化的世界空间区域重叠的 Tile
- **全量失效**：所有 Tile 标记为脏（当 Volume 边界改变时使用）

---

## 6. Multi-Thread Model / 多线程模型

### 6.1 Engine Thread Architecture / 引擎线程架构

```
┌────────────────────────────────────────────────────┐
│              Logic Thread (主线程/逻辑线程)           │
│                                                     │
│  World → GameObject → RVTVolumeComponent            │
│                         ├─ Config (逻辑线程数据)     │
│                         └─ mRenderProxy* (桥接指针)  │
│                                                     │
│  Data Ownership Rule / 数据所有权规则:               │
│  ✅ Logic thread OWNS: Config, WorldBounds, Name     │
│  ❌ Logic thread NEVER touches: RVT*, TileRenderer*  │
│  ✅ 逻辑线程拥有：配置、世界边界、名称                 │
│  ❌ 逻辑线程绝不接触：RVT*、TileRenderer*             │
└───────────────────────┬─────────────────────────────┘
                        │
                        │ ENQUEUE_RENDER_COMMAND
                        │ (Value copy / 值拷贝传递)
                        │
                        │ Three patterns / 三种模式:
                        │ A: Create (OnRegisterWorld)
                        │ B: Update (MarkRenderStateDirty)
                        │ C: Destroy (OnUnregisterWorld)
                        │
┌───────────────────────▼─────────────────────────────┐
│              Render Thread (渲染线程)                  │
│                                                      │
│  SceneManager → RVTRenderProxy                       │
│                   ├─ RuntimeVirtualTexture*           │
│                   ├─ RVTTileRenderer*                 │
│                   ├─ UpdateTiles(CmdList*)            │
│                   └─ All GPU resources                │
│                                                      │
│  Data Ownership Rule / 数据所有权规则:                │
│  ✅ Render thread OWNS: RVT*, Tiles, Indirection Tex │
│  ✅ 渲染线程拥有：RVT*、Tile 数据、间接纹理          │
└──────────────────────────────────────────────────────┘
```

### 6.2 ENQUEUE_RENDER_COMMAND Patterns / ENQUEUE 模式

```cpp
// ============================================================
// Pattern A: Create Proxy (OnRegisterWorld)
// 模式 A：创建 Proxy（注册到 World 时）
// ============================================================
void RVTVolumeComponent::OnRegisterWorldImpl(World* InWorld)
{
    // Step 1: Snapshot config on logic thread (value copy, NO race)
    // 步骤 1：在逻辑线程上快照配置（值拷贝，无竞态）
    RVTRenderProxy::Config ConfigSnapshot;
    ConfigSnapshot.Name = mVolumeName;     // copy string
    ConfigSnapshot.WorldMinX = mWorldMinX; // copy float
    // ...

    // Step 2: Enqueue to render thread
    // 步骤 2：投递到渲染线程
    ENQUEUE_RENDER_COMMAND(CreateRVTProxy)(
        [this, ConfigSnapshot](RenderContext& Context) {
            // Runs on render thread / 在渲染线程执行
            mRenderProxy = SceneMgr->CreateRenderProxy(EProxyType::RVTVolume);
            mRenderProxy->InitializeRVT(ConfigSnapshot, SceneMgr);
        });
}

// ============================================================
// Pattern B: Update Proxy (MarkRenderStateDirty)
// 模式 B：更新 Proxy（标记渲染状态脏）
// ============================================================
void RVTVolumeComponent::SetWorldBounds(float MinX, float MinY, ...)
{
    // Step 1: Update logic-thread data
    // 步骤 1：更新逻辑线程数据
    mWorldMinX = MinX; // Logic thread owns this

    // Step 2: Copy pointer + values, then enqueue
    // 步骤 2：拷贝指针和值，然后投递
    RVTRenderProxy* Proxy = mRenderProxy;  // copy pointer
    ENQUEUE_RENDER_COMMAND(UpdateBounds)(
        [Proxy, MinX, MinY, MaxX, MaxY](RenderContext& Ctx) {
            Proxy->SetWorldBounds(MinX, MinY, MaxX, MaxY);
        });
}

// ============================================================
// Pattern C: Destroy Proxy (OnUnregisterWorld)
// 模式 C：销毁 Proxy（从 World 注销时）
// ============================================================
void RVTVolumeComponent::OnUnregisterWorldImpl()
{
    RVTRenderProxy* ProxyToDestroy = mRenderProxy;
    mRenderProxy = nullptr;  // ✅ Disconnect immediately on logic thread
                              // ✅ 逻辑线程立即断开引用

    ENQUEUE_RENDER_COMMAND(DestroyProxy)(
        [ProxyToDestroy, this](RenderContext& Ctx) {
            ProxyToDestroy->ShutdownRVT();
            SceneMgr->DestroyRenderProxy(ProxyToDestroy);
        });
}
```

### 6.3 VTMaterialBindingManager Thread Safety / 线程安全

```
⚠️ RENDER THREAD ONLY / 仅渲染线程

English:
VTMaterialBindingManager is a render-thread-only singleton. Its internal
hash map has NO mutex because all access is serialized on the render
thread. Logic thread code that needs to register/unregister VT materials
MUST use ENQUEUE_RENDER_COMMAND.

In Debug builds, every public method has an ASSERT_RENDER_THREAD() check
that logs an error if called from the wrong thread.

中文：
VTMaterialBindingManager 是渲染线程专属的单例。其内部哈希表没有互斥锁，
因为所有访问都在渲染线程上序列化。逻辑线程需要注册/注销 VT 材质时，
必须使用 ENQUEUE_RENDER_COMMAND 投递命令。

在 Debug 构建中，每个公共方法都有 ASSERT_RENDER_THREAD() 检查，
如果从错误的线程调用会记录错误日志。
```

### 6.4 Streaming Manager Thread Model / 流式加载线程模型

```
┌──────────────────────────────────────────────────────────────┐
│                                                               │
│  Render Thread          Worker Threads         I/O Thread     │
│  ──────────────         ──────────────         ──────────     │
│                                                               │
│  SubmitRequest() ─┐                                           │
│  (thread-safe,    │    ┌──────────────┐                      │
│   mutex-protected)│───▶│ PriorityQueue│                      │
│                   │    │ (pending)     │                      │
│                        └──────┬───────┘                      │
│                               │                               │
│  DispatchLoadTasks()          │                               │
│  (render thread) ────────────▶│                               │
│                               ▼                               │
│                        ┌──────────────┐   ┌──────────────┐   │
│                        │LoadTileTask()│──▶│ File Read    │   │
│                        │(TaskGraph    │   │ (async I/O)  │   │
│                        │ worker)      │   └──────────────┘   │
│                        │              │                       │
│                        │ Decompress   │                       │
│                        └──────┬───────┘                      │
│                               │                               │
│                        ┌──────▼───────┐                      │
│  ProcessCompleted() ◀──│ CompletedQ   │                      │
│  (render thread,       │ (mutex-      │                      │
│   uploads to GPU)      │  protected)  │                      │
│                        └──────────────┘                      │
│                                                               │
└──────────────────────────────────────────────────────────────┘
```

---

## 7. Shader Sampling / Shader 采样

### 7.1 VirtualTexture.inl Overview / 着色器库概述

**English**: The `VirtualTexture.inl` GLSL include file provides all sampling functions. Any shader that reads from a VT includes this file.

**中文**：`VirtualTexture.inl` GLSL 头文件提供所有采样函数。任何需要从 VT 读取数据的 Shader 都需要包含此文件。

### 7.2 Sampling Algorithm / 采样算法

```glsl
// Step-by-step: How VTSampleGrad() works
// 逐步说明：VTSampleGrad() 的工作原理

vec4 VTSampleGrad(indirectionTex, physicalAtlas, virtualUV, ddxUV, ddyUV, params)
{
    // ─── Step 1: Compute Mip Level / 计算 Mip 级别 ───
    // Uses explicit gradients to avoid divergence issues
    // 使用显式梯度避免分支中的发散问题
    float mipLevel = 0.5 * log2(max(dot(dx,dx), dot(dy,dy)));
    //           where dx = ddxUV * VirtualTextureSize

    // ─── Step 2: Read Indirection Texture / 读取间接纹理 ───
    // texelFetch at (virtualUV * indTexSize) at the computed mip
    // 在计算出的 mip 级别下，对 (virtualUV * 间接纹理尺寸) 做 texelFetch
    uvec4 indData = texelFetch(indirectionTex, indCoord, int(mip));
    //   .rg = physical tile XY in atlas grid
    //   .b  = mipBias (how many mips coarser than requested)
    //   .a  = flags (IsResident)

    // ─── Step 3: Compute Physical UV / 计算物理 UV ───
    //
    //  Virtual UV  ────▶  Tile-local fraction  ────▶  Physical pixel pos
    //  虚拟 UV          Tile 内的分数位置             物理像素位置
    //
    //  physicalPixelPos = tileOrigin          ← Atlas grid position × 136
    //                   + borderOffset        ← Skip 4px border
    //                   + withinTile × 128    ← Content position
    //
    //  physicalUV = physicalPixelPos / atlasSize
    //             = physicalPixelPos / 4352

    // ─── Step 4: Sample Physical Atlas / 采样物理 Atlas ───
    // Use textureGrad() with scaled gradients for proper aniso filtering
    // 使用 textureGrad() 配合缩放后的梯度实现正确的各向异性滤波
    return textureGrad(physicalAtlas, physicalUV, physDdx, physDdy);
}
```

### 7.3 Why VTSampleGrad instead of VTSample? / 为什么用 VTSampleGrad？

**English**: `dFdx()`/`dFdy()` produce undefined results inside divergent flow control (e.g., after `if (alpha < threshold) discard`). By computing gradients *before* any branching and passing them explicitly to `VTSampleGrad`, we avoid artifacts. This is critical in the GBuffer pass where alpha-testing is common.

**中文**：`dFdx()`/`dFdy()` 在发散的流控制内（如 `if (alpha < threshold) discard` 之后）会产生未定义的结果。通过在任何分支*之前*计算梯度并显式传递给 `VTSampleGrad`，我们可以避免瑕疵。这在 GBuffer Pass 中尤其关键，因为 alpha 测试很常见。

### 7.4 Available Functions / 可用函数

| Function | Description (EN) | 说明 (CN) | When to Use |
|----------|------------------|-----------|-------------|
| `VTSample()` | Auto mip from derivatives | 自动从导数计算 mip | Simple cases, no branching |
| `VTSampleGrad()` | Explicit gradients | 显式梯度 | Inside flow control, GBuffer |
| `VTSampleLevel()` | Explicit mip level | 指定 mip 级别 | Debugging, forced LOD |
| `VTComputeFeedback()` | Output packed tile request | 输出打包的 Tile 请求 | Feedback pass only |
| `VTDebugMipColor()` | False-color mip visualization | Mip 伪彩色可视化 | Debug overlay |
| `VTDebugTileGrid()` | Tile boundary grid | Tile 边界网格 | Debug overlay |

---

## 8. Material Integration / 材质系统集成

### 8.1 Tag-Based Binding / 基于 Tag 的绑定机制

**English**: Rather than modifying the base `MaterialInterface` class (which would be invasive), we use an external "Tag" system. The `VTMaterialBindingManager` singleton maps `material pointer → VTMaterialBinding` using a hash map.

**中文**：我们没有修改基类 `MaterialInterface`（那样做会过于侵入），而是使用外部的 "Tag" 系统。`VTMaterialBindingManager` 单例通过哈希表将 `材质指针 → VTMaterialBinding` 进行映射。

```
┌────────────────────────────────────────────────────────────┐
│ MaterialInterface (Base class, UNCHANGED / 基类，未修改)     │
│   ├─ Shader*                                               │
│   ├─ Textures[]                                            │
│   └─ Uniforms                                              │
└────────────────────────────────────────────────────────────┘
                         │
                  External Tag Lookup / 外部 Tag 查询
                         │
                         ▼
┌────────────────────────────────────────────────────────────┐
│ VTMaterialBindingManager::GetBinding(materialPtr)          │
│   Returns → VTMaterialBinding                              │
│     ├─ VTMaterialFlags                                     │
│     │   ├─ bUseVirtualTexture = true                       │
│     │   ├─ SpaceID = 0                                     │
│     │   ├─ LayerMask = 0x0F (all 4 layers)                 │
│     │   └─ VTType = SVT / RVT                              │
│     │                                                      │
│     ├─ IndirectionTexture*        ← GPU resource           │
│     ├─ PhysicalAtlas[4]*          ← GPU resources          │
│     ├─ SpaceParamsUBO*            ← Uniform buffer         │
│     └─ bResourcesResolved        ← Lazy resolution         │
└────────────────────────────────────────────────────────────┘
```

### 8.2 VTDescriptorSetBinder / VT 描述符集绑定器

**English**: During rendering, for each draw call, the `VTDescriptorSetBinder` checks if the material has a VT tag. If yes, it swaps Vulkan descriptor `set=2` (Per-Material) to bind VT resources instead of traditional textures.

**中文**：渲染时，对每个 Draw Call，`VTDescriptorSetBinder` 检查材质是否有 VT Tag。如果有，就将 Vulkan 描述符 `set=2`（Per-Material）切换为绑定 VT 资源，而不是传统纹理。

```
Descriptor Set Layout / 描述符集布局:

  set=0: Per-Frame    (Camera, Lights, Time)     ← 每帧数据
  set=1: Per-Object   (WorldMatrix, ObjectID)    ← 每对象数据
  set=2: Per-Material (Textures / VT Resources)  ← 每材质数据 ⭐ 动态切换

  For VT materials, set=2 binds / VT 材质时 set=2 绑定:
  ┌────────────────────────────────────────────────┐
  │ binding 0: vtIndirection    (usampler2D)       │
  │ binding 1: vtPhysicalBC     (sampler2D, L0)    │
  │ binding 2: vtPhysicalNorm   (sampler2D, L1)    │
  │ binding 3: vtPhysicalRMA    (sampler2D, L2)    │
  │ binding 4: vtPhysicalEmis   (sampler2D, L3)    │
  │ binding 5: vtSpaceParams    (UBO)              │
  └────────────────────────────────────────────────┘
```

---

## 9. Deferred Pipeline Integration / 延迟管线集成

### 9.1 Render Pass Order / 渲染 Pass 顺序

```
┌──────────────────────────────────────────────────────────────┐
│             ElaineDeferredRenderPipeline                       │
│                                                               │
│  Pass 1: VT Feedback Pass (1/8 resolution)                   │
│  Pass 1: VT 反馈 Pass（1/8 分辨率）                            │
│    └─ All VT objects render tile requests to R32_UINT RT      │
│                                                               │
│  Pass 2: Shadow Pass (depth-only)                            │
│  Pass 2: 阴影 Pass（仅深度）                                   │
│                                                               │
│  Pass 3: GBuffer Pass (MRT, Full resolution)                 │
│  Pass 3: GBuffer Pass（MRT，全分辨率）                          │
│    ├─ Standard materials: DeferredGBuffer.ps                  │
│    └─ VT materials: DeferredGBufferVT.ps ← Uses VTSampleGrad │
│                                                               │
│  Pass 4: Deferred Lighting Pass (full-screen quad)           │
│  Pass 4: 延迟光照 Pass（全屏四边形）                             │
│    └─ PBR lighting from GBuffer                               │
│                                                               │
│  Pass 5: Transparent Forward Pass                            │
│  Pass 5: 透明物体前向 Pass                                     │
│                                                               │
│  Pass 6: Copy to Back Buffer                                 │
│  Pass 6: 拷贝到后缓冲                                          │
└──────────────────────────────────────────────────────────────┘
```

### 9.2 GBuffer Layout / GBuffer 布局

| RT | Format | Content | 内容 |
|----|--------|---------|------|
| RT0 | R8G8B8A8 | BaseColor.rgb + Metallic.a | 基础色 + 金属度 |
| RT1 | R16G16B16A16F | WorldNormal.rgb + Roughness.a | 世界法线 + 粗糙度 |
| RT2 | R8G8B8A8 | Emissive.rgb + AO.a | 自发光 + 环境遮蔽 |
| Depth | D32F | Depth | 深度 |

### 9.3 GBufferVT Shader Flow / GBufferVT 着色器流程

```glsl
// DeferredGBufferVT.ps — Simplified overview
// DeferredGBufferVT.ps — 简化流程

void main()
{
    // ⚠️ CRITICAL: Compute gradients BEFORE any divergent flow
    // ⚠️ 关键：在任何发散流控制之前计算梯度
    vec2 ddxUV = dFdx(vUV);
    vec2 ddyUV = dFdy(vUV);

    // Sample all 4 VT layers
    // 采样所有 4 个 VT 层
    vec4 baseColorSample = VTSampleGrad(indirection, atlasBC,   vUV, ddxUV, ddyUV, params);
    vec4 normalSample    = VTSampleGrad(indirection, atlasNorm, vUV, ddxUV, ddyUV, params);
    vec4 rmaSample       = VTSampleGrad(indirection, atlasRMA,  vUV, ddxUV, ddyUV, params);
    vec4 emissiveSample  = VTSampleGrad(indirection, atlasEmis, vUV, ddxUV, ddyUV, params);

    // Output to GBuffer MRTs
    // 输出到 GBuffer MRT
    outRT0 = vec4(albedo, metallic);
    outRT1 = vec4(worldNormal, roughness);
    outRT2 = vec4(emissive, ao);
}
```

---

## 10. File Format (.evt) / 文件格式

### 10.1 EVT File Structure / EVT 文件结构

```
┌──────────────────────────────────────────────────────────────┐
│                    .evt File Layout                            │
│                                                               │
│  Offset 0x0000: ┌────────────────────┐                       │
│                  │ EVTFileHeader      │  72 bytes              │
│                  │  Magic  = "EVT\0"  │                       │
│                  │  Version = 1       │                       │
│                  │  VirtualSizeX/Y    │                       │
│                  │  NumMipLevels      │                       │
│                  │  NumLayers         │                       │
│                  │  TileSize = 128    │                       │
│                  │  TileBorderSize = 4│                       │
│                  │  LayerFormats[4]   │                       │
│                  │  TotalTileCount    │                       │
│                  │  TileIndexOffset   │─ ─ ─ ─ ─ ─ ┐         │
│                  │  TileDataOffset    │─ ─ ─ ┐     │         │
│                  └────────────────────┘      │     │         │
│                                              │     │         │
│  TileIndexOffset: ┌──────────────────┐ ◀ ─ ─ ┘     │         │
│                   │ EVTTileIndexEntry │ × TotalCount │         │
│                   │  PackedCoord      │              │         │
│                   │  DataOffset       │─ ─ ─ ┐     │         │
│                   │  CompressedSize   │      │     │         │
│                   │  UncompressedSize │      │     │         │
│                   └──────────────────┘      │     │         │
│                   │ ... (repeat)      │      │     │         │
│                   └──────────────────┘      │     │         │
│                                              │     │         │
│  TileDataOffset:  ┌──────────────────┐ ◀ ─ ─ ┘ ◀ ─ ┘         │
│                   │ Tile pixel data   │                       │
│                   │ (per-layer,       │                       │
│                   │  optionally       │                       │
│                   │  compressed)      │                       │
│                   └──────────────────┘                       │
└──────────────────────────────────────────────────────────────┘

English:
  The .evt file stores all tiles for a virtual texture in a compact format.
  The TileIndexTable provides O(1) lookup by packed coordinate.
  Tile data can be optionally compressed (LZ4/BC compression).

中文：
  .evt 文件以紧凑格式存储虚拟纹理的所有 Tile 数据。
  TileIndexTable 通过打包坐标提供 O(1) 查找。
  Tile 数据可选压缩（LZ4/BC 压缩）。
```

---

## 11. Debug Visualization / 调试可视化

### 11.1 Available Debug Modes / 可用调试模式

| Mode | Description (EN) | 说明 (CN) | Shader Function |
|------|------------------|-----------|-----------------|
| Mip Heatmap | False-color by mip level | 按 Mip 级别伪彩色 | `VTDebugMipColor()` |
| Tile Grid | Tile boundary overlay | Tile 边界网格叠加 | `VTDebugTileGrid()` |
| Residency | Green=Resident, Red=Missing | 绿色=已加载，红色=缺失 | `indirection.isResident` |
| Pool Usage | Physical pool utilization | 物理池利用率 | `VTStatistics` |

### 11.2 Mip Color Mapping / Mip 颜色映射

```
  Mip 0 = 🔴 Red       (highest detail / 最高精度)
  Mip 1 = 🟠 Orange
  Mip 2 = 🟡 Yellow
  Mip 3 = 🟢 Green
  Mip 4 = 🔵 Cyan
  Mip 5 = 🟣 Blue
  Mip 6 = 🟪 Purple
  Mip 7+= 🩷 Magenta   (lowest detail / 最低精度)
```

---

## 12. File Inventory / 文件清单

### Core Module (ElaineCore) — Render Thread / 渲染线程

| File | Description (EN) | 说明 (CN) |
|------|------------------|-----------|
| `Include/VirtualTexture/ElaineVirtualTextureTypes.h` | All types, enums, constants, interfaces | 所有类型、枚举、常量、接口 |
| `Include/VirtualTexture/ElaineVirtualTextureSpace.h` | VT space with page table | VT 空间（含页表） |
| `Include/VirtualTexture/ElainePhysicalTilePool.h` | Physical atlas manager (LRU) | 物理 Atlas 管理器（LRU） |
| `Include/VirtualTexture/ElaineVTFeedbackAnalyzer.h` | GPU feedback readback & analysis | GPU 反馈回读与分析 |
| `Include/VirtualTexture/ElaineVTStreamingManager.h` | Async tile I/O with priority queue | 带优先级队列的异步 Tile I/O |
| `Include/VirtualTexture/ElaineVTIndirectionTexture.h` | Indirection texture GPU update | 间接纹理 GPU 更新 |
| `Include/VirtualTexture/ElaineVTMaterialBinding.h` | Tag-based VT material binding | 基于 Tag 的 VT 材质绑定 |
| `Include/VirtualTexture/ElaineVTDescriptorSetBinder.h` | Vulkan descriptor set=2 switching | Vulkan 描述符 set=2 动态切换 |
| `Include/VirtualTexture/ElaineRuntimeVirtualTexture.h` | RVT core class | RVT 核心类 |
| `Include/VirtualTexture/ElaineRVTTileRenderer.h` | RVT tile capture renderer | RVT Tile 捕获渲染器 |
| `Include/RenderProxy/ElaineRVTRenderProxy.h` | RVT render proxy (render thread) | RVT 渲染代理（渲染线程） |
| `Source/VirtualTexture/*.cpp` | All implementations | 所有实现文件 |
| `Source/RenderProxy/ElaineRVTRenderProxy.cpp` | RVT proxy implementation | RVT 代理实现 |

### Engine Module (ElaineEngine) — Logic Thread / 逻辑线程

| File | Description (EN) | 说明 (CN) |
|------|------------------|-----------|
| `Public/GamePlay/ElaineRVTVolumeComponent.h` | RVT volume component (logic thread) | RVT 体积组件（逻辑线程） |
| `Private/GamePlay/ElaineRVTVolumeComponent.cpp` | Component impl with ENQUEUE | 组件实现（含 ENQUEUE） |

### Shaders / 着色器

| File | Description (EN) | 说明 (CN) |
|------|------------------|-----------|
| `Contents/shader/vulkan/VirtualTexture.inl` | VT sampling library (GLSL) | VT 采样库（GLSL） |
| `Contents/shader/vulkan/VTFeedback.vs/.ps` | Feedback pass shaders | 反馈 Pass 着色器 |
| `Contents/shader/vulkan/DeferredGBufferVT.ps` | VT-enabled GBuffer fragment shader | VT 版 GBuffer 片段着色器 |
| `Contents/shader/vulkan/RVTCapture.vs/.ps` | RVT tile capture shaders | RVT Tile 捕获着色器 |

### Render Pipeline / 渲染管线

| File | Description (EN) | 说明 (CN) |
|------|------------------|-----------|
| `Include/ElaineDeferredRenderPipeline.h` | Deferred pipeline with VT integration | 集成 VT 的延迟管线 |
| `Source/ElaineDeferredRenderPipeline.cpp` | 6-pass deferred renderer | 6-Pass 延迟渲染器 |

---

## Glossary / 术语表

| Term | Full Name | 中文 |
|------|-----------|------|
| VT | Virtual Texture | 虚拟纹理 |
| SVT | Streaming Virtual Texture | 流式虚拟纹理 |
| RVT | Runtime Virtual Texture | 运行时虚拟纹理 |
| Tile | A fixed-size piece of texture data | 瓦片（固定尺寸的纹理块） |
| Page Table | Maps virtual tiles → physical locations | 页表（虚拟 Tile → 物理位置映射） |
| Indirection Texture | GPU-side page table for shader lookup | 间接纹理（GPU 端的页表） |
| Physical Tile Pool | Atlas texture holding resident tiles | 物理 Tile 池（存放已加载 Tile 的 Atlas） |
| Feedback Buffer | Low-res RT recording which tiles are needed | 反馈缓冲区（记录需要哪些 Tile 的低分辨率 RT） |
| LRU | Least Recently Used (eviction policy) | 最近最少使用（驱逐策略） |
| MRT | Multiple Render Targets | 多渲染目标 |
| GBuffer | Geometry Buffer (deferred shading) | 几何缓冲区（延迟着色） |
| ENQUEUE | ENQUEUE_RENDER_COMMAND macro | 渲染线程命令投递宏 |
| RenderProxy | Render-thread mirror of a logic-thread Component | 逻辑线程 Component 的渲染线程镜像 |

---

*End of Document / 文档结束*

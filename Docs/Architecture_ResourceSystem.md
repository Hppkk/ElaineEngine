# 资源管理系统架构文档

---

## 1. 概述

Elaine Engine 的资源管理系统位于 **ElaineCore** 的 `Resource` 子模块，提供统一的资源加载、卸载、引用计数和异步加载机制。系统基于 **TaskGraph** 实现异步加载，通过 **ResourcePtr** 智能指针管理资源生命周期。

---

## 2. 核心类层次

```mermaid
classDiagram
    class ResourceBase {
        <<abstract>>
        +LoadResource(bool async)
        +UnloadResource()
        +ReloadResource()
        +SaveResource()
        +ResourceArrived()
        +GetLoadState() LoadState
        +IsLoaded() bool
        +GetPath() string
        +RegisterLoadCompleteCallback(callback)
        +GetDirectDependencies(vector)
        #LoadImpl() bool
        #UnloadImpl()
        #SaveResourceImpl()
        #ResourceArrivedImpl()
        #NotifyLoadComplete()
        -mLoadState: atomic~LoadState~
        -mLoadTask: GraphTaskPtr
        -mLoadCompleteCallbacks: vector
    }

    class Mesh {
        +LoadImpl() bool
        +顶点/索引数据管理
    }

    class Material {
        +LoadImpl() bool
        +着色器引用
        +参数定义
    }

    class MaterialInstanceStatic {
        +LoadImpl() bool
        +参数快照
    }

    class TextureResource {
        +LoadImpl() bool
        +纹理数据管理
    }

    class Shader {
        +LoadImpl() bool
        +SPIR-V/HLSL 编译
    }

    class GameObjectInfo {
        +LoadImpl() bool
        +ImportData(JsonCpp)
        +ExportData(JsonCpp)
        +ExportToFile()
    }

    ResourceBase <|-- Mesh
    ResourceBase <|-- Material
    ResourceBase <|-- MaterialInstanceStatic
    ResourceBase <|-- TextureResource
    ResourceBase <|-- Shader
    ResourceBase <|-- GameObjectInfo
```

---

## 3. 资源管理器

```mermaid
classDiagram
    class ResourceManager {
        <<abstract>>
        +GetResource(string, bool async) ResourceBasePtr
        +CreateEmptyResource(string) ResourceBasePtr
        #CreateResourceImpl(string) ResourceBasePtr
        -mResourceType: ResourceType
        -mResources: map~string, ResourceBasePtr~
        -mMtx: mutex
    }

    class MeshManager {
        #CreateResourceImpl() ResourceBasePtr
    }

    class MaterialManager {
        #CreateResourceImpl() ResourceBasePtr
    }

    class MaterialInstanceStaticManager {
        #CreateResourceImpl() ResourceBasePtr
    }

    class TextureManager {
        #CreateResourceImpl() ResourceBasePtr
    }

    class ShaderManager {
        #CreateResourceImpl() ResourceBasePtr
    }

    ResourceManager <|-- MeshManager
    ResourceManager <|-- MaterialManager
    ResourceManager <|-- MaterialInstanceStaticManager
    ResourceManager <|-- TextureManager
    ResourceManager <|-- ShaderManager
```

### 3.1 资源类型枚举

```cpp
enum ResourceType : uint8_t {
    RT_Texture,
    RT_Material,
    RT_MaterialInstance,
    RT_Shader,
    RT_Mesh,
    RT_GameObject,
    RT_Count
};
```

### 3.2 ResourceModule

`ResourceModule` 是资源系统的入口，管理所有 `ResourceManager` 实例。

---

## 4. 资源加载流程

### 4.1 同步加载

```mermaid
sequenceDiagram
    participant Client as 调用方
    participant RM as ResourceManager
    participant RB as ResourceBase

    Client->>RM: GetResource(path, async=false)
    RM->>RM: 查找缓存
    alt 已缓存
        RM-->>Client: 返回缓存资源
    else 未缓存
        RM->>RB: CreateResourceImpl(path)
        RB->>RB: LoadResource(async=false)
        RB->>RB: LoadImpl() [同步执行]
        RB->>RB: 状态 → Loaded
        RB->>RB: NotifyLoadComplete()
        RM-->>Client: 返回新资源
    end
```

### 4.2 异步加载

```mermaid
sequenceDiagram
    participant Client as 调用方
    participant RM as ResourceManager
    participant RB as ResourceBase
    participant TG as TaskGraph

    Client->>RM: GetResource(path, async=true)
    RM->>RB: CreateResourceImpl(path)
    RB->>RB: LoadResource(async=true)
    RB->>RB: 状态 → Pending
    RB->>TG: CreateAndDispatchWhenReady(LoadImpl)
    RB->>RB: 状态 → Loading
    RM-->>Client: 返回资源 (状态: Loading)
    
    Note over TG: 后台线程执行
    TG->>RB: LoadImpl()
    RB->>RB: 状态 → Loaded
    RB->>RB: NotifyLoadComplete()
    RB->>Client: 回调通知
```

---

## 5. 资源状态机

```mermaid
stateDiagram-v2
    [*] --> Unloaded
    Unloaded --> Pending: LoadResource()
    Pending --> Loading: TaskGraph 调度
    Loading --> Loaded: LoadImpl() 成功
    Loading --> Failed: LoadImpl() 失败
    Loading --> Missing: 文件不存在
    Loaded --> Unloading: UnloadResource()
    Unloading --> Unloaded: UnloadImpl() 完成
    Loaded --> Loading: ReloadResource()
```

| 状态 | 描述 |
|------|------|
| `Unloaded` | 未加载 |
| `Pending` | 等待加载（已入队） |
| `Loading` | 正在加载中 |
| `Loaded` | 加载完成 |
| `Unloading` | 正在卸载 |
| `Failed` | 加载失败 |
| `Missing` | 资源文件缺失 |

---

## 6. 引用计数智能指针 (ResourcePtr)

```mermaid
classDiagram
    class ResourcePtr~T~ {
        +get() T*
        +operator->() T*
        +operator*() T&
        +IsValid() bool
        +Reset()
        -mPtr: T*
        -mRefCount: UseCount*
    }

    class UseCount {
        +AddRef()
        +Release()
        +GetRefCount() uint32
    }

    ResourcePtr --> UseCount
```

**特性：**
- 侵入式引用计数
- 支持 `static_pointer_cast` 类型转换
- 线程安全的引用计数操作

---

## 7. 资源依赖

### 7.1 依赖声明

```cpp
// ResourceBase 提供依赖查询接口
virtual void GetDirectDependencies(
    std::vector<ResourceBasePtr>& OutDependencies
) const {}
```

### 7.2 RenderProxy 资源追踪

RenderProxy 提供了更高级的资源追踪机制：

```mermaid
sequenceDiagram
    participant RP as RenderProxy
    participant R1 as Resource A
    participant R2 as Resource B (依赖)

    RP->>RP: TrackResource(R1)
    RP->>R1: 注册加载回调
    RP->>R1: GetDirectDependencies()
    R1-->>RP: [R2]
    RP->>R2: TrackResource(R2) [递归]
    RP->>RP: BeginInitialization()
    
    Note over R2: 后台加载
    R2->>RP: OnResourceLoaded(R2)
    RP->>RP: mPendingResourceCount--
    
    Note over R1: 后台加载
    R1->>RP: OnResourceLoaded(R1)
    RP->>RP: mPendingResourceCount--
    RP->>RP: TryInitializeBindings()
    RP->>RP: InitializeResourceBinding() [虚函数]
    RP->>RP: mBindingsInitialized = true
```

---

## 8. 加载完成回调

```cpp
// 注册加载完成回调
resource->RegisterLoadCompleteCallback([](ResourceBase* res) {
    // 资源加载完成后执行
    LOG_INFO("Resource loaded: {}", res->GetPath());
});
```

**回调特性：**
- 线程安全（内部使用 `mCallbackMutex`）
- 支持多个回调注册
- 资源已加载时立即触发

---

## 9. 文件系统

### 9.1 核心类

| 类 | 职责 |
|----|------|
| `FileManager` | 文件管理器，路径解析 |
| `FileSystem` | 文件系统抽象 |
| `DataStream` | 数据流读写 |
| `DataStreamMgr` | 数据流管理器 |

### 9.2 资源目录结构

```
Contents/
├── config/              # 引擎与项目配置 (JSON)
├── material/            # 材质定义文件
├── material_instance/   # 材质实例文件
├── model/               # 3D 模型 (通过 assimp 导入)
├── render/              # 渲染管线配置
├── shader/              # 着色器源码 (GLSL/HLSL → SPIR-V)
├── texture/             # 纹理图片
└── worlds/              # 世界/关卡存档 (JSON)
```

---

## 10. 内存管理

### 10.1 MemoryMap

`ElaineMemoryMap` 提供内存映射文件支持，用于大文件的高效读取。

### 10.2 GPU 内存

- 支持专用分配 (Dedicated Allocation)、子分配 (Sub-Allocation) 等策略

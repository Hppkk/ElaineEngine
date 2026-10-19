# Elaine Engine

-----------------

![ElaineEngine](Docs/Images/ElaineEngine.png)

Welcome to the Elaine Engine source code!

You can build the Elaine Editor for Windows(Other platforms will be gradually opened in the future); compile Elaine Engine games for target platforms. Modify the code in any way you can imagine, and share your changes with others.

![image-20260329230101040](Docs/Images/main_scene_00.png)

# Architecture

The engine consists of the following main modules:

```mermaid
graph TB
    subgraph "Application Layer"
        Editor["ElaineEditor<br/>(.exe)"]
        Sample["Samples<br/>(Sample application)"]
    end

    subgraph "Engine Layer"
        Engine["ElaineEngine<br/>(.dll)"]
    end

    subgraph "Core Layer"
        Core["ElaineCore<br/>(.dll)"]
    end

    subgraph "Infrastructure Layer"
        Reflection["ElaineReflection<br/>(.dll)"]
    end

    subgraph "Toolchain"
        BuildTool["ElaineBuildTool<br/>(.exe)"]
    end

    Editor --> Engine
    Editor --> Core
    Editor --> Reflection
    Sample --> Engine
    Engine --> Core
    Engine --> Reflection
    BuildTool -.->|"Generate reflection code"| Engine
    BuildTool -.->|"Generate editor code"| Editor
```

```
┌─────────────────────────────────────────────────────────┐
│                     应用层 (Application)                  │
│  ┌───────────────────┐   ┌────────────────────────────┐  │
│  │   ElaineEditor    │   │       Samples / Game       │  │
│  │  (ImGui + DX11)   │   │                            │  │
│  └────────┬──────────┘   └─────────────┬──────────────┘  │
├───────────┼────────────────────────────┼─────────────────┤
│                     引擎层 (Engine)                       │
│  ┌────────┴────────────────────────────┴──────────────┐  │
│  │              ElaineEngine                          │  │
│  │  World ─ Level ─ GameObject ─ Component            │  │
│  │  Viewport ─ CameraControl ─ TickManager            │  │
│  └────────────────────────┬──────────────────────────┘  │
├───────────────────────────┼──────────────────────────────┤
│                     核心层 (Core)                         │
│  ┌────────────────────────┴──────────────────────────┐  │
│  │                  ElaineCore                        │  │
│  │  ┌──────────┐ ┌──────────┐ ┌───────────────────┐  │  │
│  │  │ 渲染系统  │ │ 资源管理  │ │   场景管理         │  │  │
│  │  │ RHI      │ │ Resource │ │   SceneManager    │  │  │
│  │  │ Vulkan   │ │ Mesh     │ │   QuadTree        │  │  │
│  │  │ DX12     │ │ Material │ │   RenderProxy     │  │  │
│  │  └──────────┘ │ Texture  │ └───────────────────┘  │  │
│  │  ┌──────────┐ │ Shader   │ ┌───────────────────┐  │  │
│  │  │ 多线程    │ └──────────┘ │   渲染管线         │  │  │
│  │  │TaskGraph │               │   RenderGraph     │  │  │
│  │  │ThreadMgr │               │   Deferred/Fwd    │  │  │
│  │  └──────────┘               └───────────────────┘  │  │
│  │  ┌──────────┐ ┌──────────┐ ┌───────────────────┐  │  │
│  │  │ 数学库    │ │ 平台抽象  │ │   文件系统         │  │  │
│  │  │Vector/Mat│ │ Window   │ │   DataStream      │  │  │
│  │  │Quaternion│ │ Input    │ │   FileManager     │  │  │
│  │  └──────────┘ └──────────┘ └───────────────────┘  │  │
│  └───────────────────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────┤
│                   基础设施层 (Infrastructure)              │
│  ┌───────────────────┐  ┌────────────────────────────┐  │
│  │  ElaineReflection │  │     ElaineBuildTool        │  │
│  │  TypeDescriptor   │  │     HeaderParser           │  │
│  │  TypeRegistry     │  │     CodeGenerator          │  │
│  │  Serializer       │  │                            │  │
│  └───────────────────┘  └────────────────────────────┘  │
├──────────────────────────────────────────────────────────┤
│                   第三方库 (ThirdParty)                   │
│  Vulkan SDK │ VMA │ GLFW │ spdlog │ assimp │ stb         │
│  ImGui │ ImGuizmo │ cJSON │ nlohmann/json │ cppast       │
└──────────────────────────────────────────────────────────┘
```

# Branch

-----------------------
The `master` is our Main Branch. For your own development,  you can run
```
git checkout -b <your-branch> origin/master
```

# Getting  and running

------------------------

The steps below take you through cloning your own private fork, then compiling and running the editor yourself:

Elaine Engine can be easily build by CMake. Make sure your CMake version is 3.26 or higher.

## Windows

1. Download Git on your computer and use the following commond:
   
   ```
   git clone https://github.com/Hppkk/ElaineEngine.git
   ```
   
   When updating existing repository, don't forget to update all submodules:
   
   ```
   git submodule update --init --recursive
   ```
   
2. Install Visual Studio 2022 or Visual Studio 2026. 

3. Run the batch file `setup_windows.bat`. That will generate the Visual Studio solution in the path `Build`. Open the `ElanieEngine.sln`with Visual Studio to run the project. Now you can start creating your own.

4. Choose one config between EditorDebug or EditorRelease  to debug your Editor.

5. Choose one config between RuntimeDebug or RuntimeRelease to debug your Game Client.

# Features

-----------------------------

1. The code in the Elaine Engine is written by C++20. 

2. Elaine Engine Core using the Vulkan and DX12 to rendering, you can see the source code in the core layer.

3. We use the [Geometric Tools](http://www.geometrictools.com/) for our math library.

4. Use TaskGraph for multi thread and RenderGraph base for render pipeline.

# Contributions

---------------------------

We welcome contributions to Elaine Engine development through pull requests on GitHub.
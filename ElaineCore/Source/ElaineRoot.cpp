#include "ElainePrecompiledHeader.h"
#include "ElaineTimer.h"
#include "ElaineInputSystem.h"
#include "ElaineDataStreamMgr.h"
#include "ElaineThreadManager.h"
#include "ElaineFileManager.h"
#include "ElaineMemoryMap.h"
#include "ElaineTextureUtils.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "ElaineModuleManager.h"
#include "ElaineFileMonitor.h"
#include "common/ElaineRenderModule.h"
#include "ElaineForwardRenderPipeline.h"
#include "ElaineDeferredRenderPipeline.h"
#include "ElaineRenderCommandQueue.h"
#include "platform/ElaineSystemInfo.h"
#include "ElaineTextureManager.h"
#include "ElaineMaterialManager.h"
#include "ElaineMaterialInstanceStaticManager.h"
#include "ElaineMeshManager.h"
#include "ElaineShaderManager.h"
#include "Resource/ElaineResourcePipeline.h"
#include "render/common/ElaineRHI.h"
#include <filesystem>

namespace Elaine
{
	extern EBarrier* LogicToRender_Barrier = new EBarrier();

	extern EBarrier* RenderToLogic_Barrier = new EBarrier();

	Root::Root()
	{
		//new LogSystem();
	}

	Root::~Root()
	{
		Terminate();
	}

	void Root::Initialize(const RHI_PARAM_DESC& InDesc)
	{
        ResourcePipeline::Get().Start();
		TextureUtils::Initialize();
		SystemInfo::LogSystemInfo();
		new FileManager();
		mTimer = new Timer();
		new FileMonitor();
#if  ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		char szFilePath[MAX_PATH + 1] = { 0 };
		GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
		for (int i = 0; i < MAX_PATH + 1; ++i)
		{
			if (szFilePath[i] == '\\')
				szFilePath[i] = '/';
		}
		mAppPath = szFilePath;
		auto pos = mAppPath.find_last_of('/');
		mAppPath = mAppPath.substr(0, pos);
		mResourcePath = mAppPath + "/../../../Contents/";
#endif 
		
		LoadConfig(mResourcePath + "config/EngineConfig.cfg");
		new ThreadManager();
        ThreadManager::instance()->InitilizeThread(NamedThread::GameThread);
		new TaskGraph::TaskGraph();
		TaskGraph::TaskGraph::instance()->Initialize();
		new ModuleManager();
		ModuleManager::instance()->Initialize();
		// 文件监控只产生事件，所有资源重载都在逻辑线程的 TickTime 中执行。
		FileMonitor::instance()->RegisterCallback([this](const std::string& ChangedPath)
		{
			ShaderManager::instance()->ReloadIfFileChanged(ChangedPath);
			TextureManager::instance()->ReloadIfFileChanged(ChangedPath);
			MeshManager::instance()->ReloadIfFileChanged(ChangedPath);
			MaterialManager::instance()->ReloadIfFileChanged(ChangedPath);
			MaterialInstanceStaticManager::instance()->ReloadIfFileChanged(ChangedPath);
		});
		FileMonitor::instance()->Start(mResourcePath);
		GETMODULE(RenderModule)->LoadDynamicRHI(InDesc);
		//new WindowSystem();

		new InputSystem();
		//mMainSceneMgr = new SceneManager("Main SceneManager");
		//mSceneMgrs.emplace("Main SceneManager", mMainSceneMgr);

		mRenderPipeline[RP_Forward] = new ForwardRenderPipeline();
		mRenderPipeline[RP_Forward]->Initialize();
		mRenderPipeline[RP_Defferred] = new DeferredRenderPipeline();
		mRenderPipeline[RP_Defferred]->Initialize();

		mRenderThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RenderThread, &Root::RenderOneFrame, this);
	}

	void Root::PostInitialize()
	{
		mbInitialize = true;
	}

	float Root::CalculateDeltaTime()
	{
		float dt = .0f;
		using namespace std::chrono;
		steady_clock::time_point tick_time_point = steady_clock::now();
		duration<float> time_span = duration_cast<duration<float>>(tick_time_point - mLastTickTimePoint);
		dt = time_span.count();
		mLastTickTimePoint = tick_time_point;
		mDeltaTime = dt;
		return dt;
	}

	void Root::CalculateFPS(float dt)
	{
		mFrameCount++;
		if (mFrameCount == 1)
		{
			mAverageDuration = dt;
		}
		else
		{
			mAverageDuration = mAverageDuration * (1 - mFPSAlpha) + dt * mFPSAlpha;
		}
		mFPS = static_cast<int>(1.f / mAverageDuration);
	}

	void Root::PreFrame(float dt)
	{

	}

	void Root::BeginFrame(float dt)
	{
		
	}
	void Root::FixedUpdate(float dt)
	{

	}
	void Root::EndFrame(float dt)
	{

	}

	void Root::PostFrame(float dt)
	{

	}

    void Root::RenderOneFrame()
    {
        RenderToLogic_Barrier->Signal();
        while (!mStopping)
        {
            const bool HasFrame = LogicToRender_Barrier->WaitFor(2);
            if (mStopping) break;
            // The original RHI frame barrier also protects immediate resource creation.
            WaitForRenderThread_Gfx();
            if (!mShutdownRender) RenderSystem::instance()->InitializeRenderResources();
            if (HasFrame) RenderSystem::instance()->GetRenderCommandQueue()->Execute();
            TaskGraph::TaskGraph::instance()->ExecuteInThread(NamedThread::RenderThread);
            if (mShutdownRender)
            {
                if (!mRenderStopped)
                {
                    for (auto* Scene : mSceneMgrs) delete Scene;
                    mSceneMgrs.clear(); mRenderViews.clear();
                    for (auto*& Pipeline : mRenderPipeline) { delete Pipeline; Pipeline = nullptr; }
                    RenderSystem::instance()->ReleaseRenderResources();
                    mRenderStopped = true;
                }
            }
            else if (HasFrame && mbInitialize)
            {
                CalculateDeltaTime(); CalculateFPS(mDeltaTime);
                RenderToLogic_Barrier->Signal();
                for (auto* View : mRenderViews)
                    if (View->IsValid() && View->IsActive()) mRenderPipeline[RP_Forward]->Render(View);
            }
            // Resource lists progress even during startup or with no active viewport.
            NotifyForRHIThread_Gfx();
        }
    }

	void Root::TickTime()
	{
		DispatchFileChanges();
		CalculateDeltaTime();
		CalculateFPS(mDeltaTime);
	}

	void Root::DispatchFileChanges()
	{
		if (FileMonitor::instance()->IsRunning())
			FileMonitor::instance()->DispatchChanges();
	}

	SceneManager* Root::GetSceneManager(const String& name)
	{
		//auto iter = mSceneMgrs.find(name);
		//if (iter != mSceneMgrs.end())
		//	return iter->second;

		return nullptr;
	}

	SceneManager* Root::GetMainSceneManager()
	{
		return mMainSceneMgr;
	}

	SceneManager* Root::CreateSceneManager(const String& name)
	{
		auto mgr = new SceneManager(name);
		mSceneMgrs.emplace(mgr);
		return mgr;
	}

	void Root::DestroySceneManager(SceneManager* InSceneManager)
	{
		if (InSceneManager == nullptr)
			return;

		mSceneMgrs.erase(InSceneManager);
		SAFE_DELETE(InSceneManager);
	}

	void Root::RegisterRenderView(RenderView* InRenderView)
	{
		if (InRenderView == nullptr)
			return;

		mRenderViews.insert(InRenderView);
	}

	void Root::UnregisterRenderView(RenderView* InRenderView)
	{
		if (InRenderView == nullptr)
			return;

		mRenderViews.erase(InRenderView);
	}

	bool Root::CheckThread(NamedThread InNamedThread)
	{
		return ThreadManager::instance()->CheckThread(InNamedThread);
	}

	void Root::LoadConfig(const std::string& InPath)
	{
		 MemoryMapFile mapFile(InPath);
		 char* stream = static_cast<char*>(mapFile.MapPointer());
		 JsonCpp ConfigJson = JsonCpp::parse(stream);
		 JsonCpp WinConfigJson = ConfigJson["Windows"];
		 mRHIType = (ERHI_TYPE)WinConfigJson.value("RenderRHI", 0);
	}

    void Root::FlushRenderCommands()
    {
        if (!mRenderThread || CheckThread(NamedThread::RenderThread)) return;
        auto Done = std::make_shared<std::promise<void>>();
        auto Ready = Done->get_future();
        ENQUEUE_RENDER_COMMAND(FlushCommands)([Done](RenderContext&) { Done->set_value(); });
        LogicToRender_Barrier->Signal();
        Ready.get();
    }

    void Root::Terminate()
    {
        if (!mRenderThread) return;
        if (FileMonitor::instance()->IsRunning()) FileMonitor::instance()->Stop();
        ResourcePipeline::Get().Shutdown();
        mShutdownRender = true;
        LogicToRender_Barrier->Signal();
        while (!mRenderStopped) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // The render thread remains available to consume resource batches in inline-RHI mode.
        delete ModuleManager::instance();
        delete RenderSystem::instance();
        mStopping = true; LogicToRender_Barrier->Signal();
        ThreadManager::instance()->DestroyThread(mRenderThread); mRenderThread = nullptr;
        delete TaskGraph::TaskGraph::instance();
        delete ThreadManager::instance();
        SAFE_DELETE(mTimer);
        delete InputSystem::instance();
        delete FileManager::instance();
        delete FileMonitor::instance();
    }
}

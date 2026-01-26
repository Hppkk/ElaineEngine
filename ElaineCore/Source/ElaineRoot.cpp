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
#include "ElaineRenderCommandQueue.h"
#include "platform/ElaineSystemInfo.h"

namespace Elaine
{
	extern EBarrier* LogicToRender_Barrier = new EBarrier();

	extern EBarrier* RenderToLogic_Barrier = new EBarrier();

	Root::Root()
	{
		new LogSystem();
	}

	Root::~Root()
	{
		Terminate();
	}

	void Root::Initialize(const RHI_PARAM_DESC& InDesc)
	{
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
		new TaskGraph::TaskGraph();
		TaskGraph::TaskGraph::instance()->Initialize();
		new ModuleManager();
		ModuleManager::instance()->Initialize();
		GETMODULE(RenderModule)->LoadDynamicRHI(InDesc);
		//new WindowSystem();

		new InputSystem();
		//mMainSceneMgr = new SceneManager("Main SceneManager");
		//mSceneMgrs.emplace("Main SceneManager", mMainSceneMgr);

		mRenderPipeline[RP_Forward] = new ForwardRenderPipeline();
		mRenderPipeline[RP_Forward]->Initialize();

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
		while (true)
		{
			if (!mbInitialize)
				continue;
			
			
			CalculateDeltaTime();
			CalculateFPS(mDeltaTime);
			//PreFrame(mDeltaTime);
			//BeginFrame(mDeltaTime);
			//FixedUpdate(mDeltaTime);
			//EndFrame(mDeltaTime);
			//PostFrame(mDeltaTime);
			LogicToRender_Barrier->Wait();
			RenderSystem::instance()->GetRenderCommandQueue()->Execute();
			RenderToLogic_Barrier->Signal();

			WaitForRenderThread_Gfx();
			for (RenderView* CurrentView : mRenderViews)
			{
				if (!CurrentView->IsValid() || !CurrentView->IsActive())
					continue;

				//TODO: GetPipeline()
				mRenderPipeline[RP_Forward]->Render(CurrentView);
			}

			TaskGraph::TaskGraph::instance()->ExecuteInThread(NamedThread::RenderThread);
			NotifyForRHIThread_Gfx();
		}
	}

	void Root::TickTime()
	{
		CalculateDeltaTime();
		CalculateFPS(mDeltaTime);
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
		 mRHIType = (RHITYPE)WinConfigJson.value("RenderRHI", 0);
	}

	void Root::Terminate()
	{
		delete RenderSystem::instance();
		delete ThreadManager::instance();
		delete LogSystem::instance();
		//delete WindowSystem::instance();
		SAFE_DELETE(mTimer);
		delete InputSystem::instance();
		delete FileManager::instance();
		delete TaskGraph::TaskGraph::instance();
		for (auto iter : mSceneMgrs)
		{
			SAFE_DELETE(iter);
		}
		delete ModuleManager::instance();
		delete FileMonitor::instance();
	}
}
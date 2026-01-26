#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include "ElaineTimer.h"
#include "ElaineSceneManager.h"
#include "common/ElaineRHI.h"
#include "ElaineThreadManager.h"
#include "ElaineRenderView.h"
#include "ElaineRenderPipeline.h"
#include "ElaineBarrier.h"

namespace Elaine
{
	//enum ThreadMode
	//{
	//	tm_Thread0, //逻辑渲染同一个线程
	//	tm_Thread1, //逻辑渲染单独线程
	//};

	class ElaineCoreExport Root :public Singleton<Root>
	{
	public:
		Root();
		~Root();
		void					Initialize(const RHI_PARAM_DESC& InDesc);
		void					PostInitialize();
		std::string&			GetAppPath() const { return mAppPath; }
		std::string&			GetResourcePath()const { return mResourcePath; }
		float					CalculateDeltaTime();
		float					GetDeltaTime() const { return mDeltaTime; }
		void					CalculateFPS(float dt);
		int						GetFPS() const { return mFPS; }
		Timer*					GetTimer() { return mTimer; }
		void					PreFrame(float dt);
		void					BeginFrame(float dt);
		void					FixedUpdate(float dt);
		void					EndFrame(float dt);
		void					PostFrame(float dt);
		void					RenderOneFrame();
		void					TickTime();
		SceneManager*			GetSceneManager(const String& name);
		SceneManager*			GetMainSceneManager();
		SceneManager*			CreateSceneManager(const String& name);
		void					DestroySceneManager(SceneManager* InSceneManager);

		RHITYPE					GetRHIType() const { return mRHIType;}
		bool					IsEnableVulkanValidationLayer() const { return mbEnableVulkanValidationLayer;}
		bool					CheckThread(NamedThread InNamedThread);
		void					RegisterRenderView(RenderView* InRenderView);
		void					UnregisterRenderView(RenderView* InRenderView);
	private:
		// shutdown
		void					Terminate();
		void					LoadConfig(const std::string& InPath);
	private:
		mutable std::string						mAppPath;
		mutable std::string						mResourcePath;
		std::chrono::steady_clock::time_point	mLastTickTimePoint = std::chrono::steady_clock::now();
		const float								mFPSAlpha = 1.f / 100;
		std::atomic_int							mFPS = 0;
		float									mAverageDuration = 0.f;
		std::atomic_int							mFrameCount = 0;
		float									mDeltaTime = 0.0f;
		Timer*									mTimer = nullptr;
		std::set<SceneManager*>					mSceneMgrs;
		std::set<RenderView*>					mRenderViews;
		SceneManager*							mMainSceneMgr = nullptr;
		RHITYPE									mRHIType = Vulkan;
		bool									mbEnableVulkanValidationLayer = true;
		ThreadWrap*								mRenderThread = nullptr;
		RenderPipeline*							mRenderPipeline[RP_Count] = {};
		std::atomic<bool>						mbInitialize = false;
	};

#ifndef CheckInThread
#define CheckInThread(InThreadName) Root::instance()->CheckThread(InThreadName);
#endif

	extern ElaineCoreExport EBarrier* LogicToRender_Barrier;
	extern ElaineCoreExport EBarrier* RenderToLogic_Barrier;
}
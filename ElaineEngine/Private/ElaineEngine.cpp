#include "ElaineEngine.h"
#include "ElaineLogSystem.h"
#include "ElaineWorld.h"
#include "ElaineWorldManager.h"
#include "ElaineComponentSystem.h"
#include "ElaineRenderCommandQueue.h"

namespace Elaine
{
	ElaineEngine* G_ElaineEngine = nullptr;

	ElaineEngine::ElaineEngine()
	{
		G_ElaineEngine = this;
	}

	ElaineEngine::~ElaineEngine()
	{
		DestroyEngine();
		G_ElaineEngine = nullptr;
	}

	void ElaineEngine::Initialize(const RHI_PARAM_DESC& InRendererDesc)
	{
		new Root();
		LOG_INFO("Elaine Engine Initialize...");
		Root::instance()->Initialize(InRendererDesc);
		mWorldManager = new WorldManager();
		new ComponentSystem();
		LOG_INFO("Elaine Engine Initialize Complete!");
	}

	void ElaineEngine::PostInitialize()
	{
		Root::instance()->PostInitialize();
	}

	void ElaineEngine::RenderOneFrame()
	{
		TickTime();
		for (World* CurrentWorld : mWorlds)
		{
			CurrentWorld->Tick(Root::instance()->GetDeltaTime());
		}
	}

	void ElaineEngine::RenderOneFrame(float InDeltaTime)
	{

	}

	void ElaineEngine::DestroyEngine()
	{
		LOG_INFO("Elaine Engine Destroy...");
		delete ComponentSystem::instance();
		delete mWorldManager;
		delete Root::instance();
	}

	World* ElaineEngine::CreateWorld()
	{
		World* NewWorld = mWorldManager->CreateWorld();
		mWorlds.push_back(NewWorld);
		return NewWorld;
	}

	void ElaineEngine::DestroyWorld(World* InWorld)
	{
		for (auto Iter = mWorlds.begin(); Iter != mWorlds.end(); ++Iter)
		{
			if (*Iter == InWorld)
			{
				mWorlds.erase(Iter);
				break;
			}
		}
		mWorldManager->DestroyWorld(InWorld);
	}

	void ElaineEngine::SyncToRenderThread()
	{

	}

	Viewport* ElaineEngine::CreateViewport(const ViewportDesc& InDesc)
	{
		Viewport* NewView = new Viewport(InDesc);
		mViewports.insert(NewView);
		return NewView;
	}

	void ElaineEngine::DestroyViewport(Viewport* InView)
	{
		mViewports.erase(InView);
		SAFE_DELETE(InView);
	}

	void ElaineEngine::RegisterViewport(Viewport* InView)
	{
		if (InView == nullptr)
			return;

		ENQUEUE_RENDER_COMMAND(RegisterViewport)([InView](RenderContext& Context)
		{
			Root::instance()->RegisterRenderView(InView->GetRenderView());
		});
		mActiveViewports.insert(InView);
	}

	void ElaineEngine::UnregisterViewport(Viewport* InView)
	{
		if (InView == nullptr)
			return;

		RenderView* RView = InView->GetRenderView();

		ENQUEUE_RENDER_COMMAND(UnregisterViewport)([RView](RenderContext& Context)
		{
			Root::instance()->UnregisterRenderView(RView);
		});
		mActiveViewports.erase(InView);
	}

	float ElaineEngine::CalculateDeltaTime()
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

	void ElaineEngine::CalculateFPS(float dt)
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

	void ElaineEngine::TickTime()
	{
		CalculateDeltaTime();
		CalculateFPS(mDeltaTime);
	}

	ElaineEngine* GetElaineEngine()
	{
		return G_ElaineEngine;
	}
}
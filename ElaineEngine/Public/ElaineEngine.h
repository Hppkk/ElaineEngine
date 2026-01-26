#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineRoot.h"
#include "ElaineViewport.h"


namespace Elaine
{
	class World;
	class WorldManager;
	class Viewport;

	class ElaineEngineExport ElaineEngine
	{
	public:
		ElaineEngine();
		virtual ~ElaineEngine();

		void Initialize(const RHI_PARAM_DESC& InRendererDesc);
		void PostInitialize();
		void RenderOneFrame();
		void RenderOneFrame(float InDeltaTime);
		void DestroyEngine();
		void DestroyWorld(World* InWorld);
		World* CreateWorld();
		void SyncToRenderThread();
		Viewport* CreateViewport(const ViewportDesc& InDesc);
		void DestroyViewport(Viewport* InView);
		void RegisterViewport(Viewport* InView);
		void UnregisterViewport(Viewport* InView);
		float CalculateDeltaTime();
		void CalculateFPS(float dt);
		void TickTime();
		int GetFPS() const { return mFPS; }

	private:
		WorldManager* mWorldManager = nullptr;
		std::vector<World*> mWorlds;
		std::set<Viewport*> mViewports;
		std::set<Viewport*> mActiveViewports;
		const float	mFPSAlpha = 1.f / 100;
		int		mFPS = 0;
		float	mAverageDuration = 0.f;
		int		mFrameCount = 0;
		float	mDeltaTime = 0.0f;
		std::chrono::steady_clock::time_point mLastTickTimePoint = std::chrono::steady_clock::now();
	};

	extern ElaineEngineExport ElaineEngine* GetElaineEngine();
}
#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineRVTRenderProxy.h"
#include "VirtualTexture/ElaineRuntimeVirtualTexture.h"
#include "VirtualTexture/ElaineRVTTileRenderer.h"
#include "render/common/ElaineRHI.h"
#include "render/common/ElaineRHICommandList.h"
#include "ElaineRenderQueue.h"

namespace Elaine
{
	RVTRenderProxy::RVTRenderProxy()
	{
		mType = EProxyType::RVTVolume;
		mbVisible = true;
	}

	RVTRenderProxy::~RVTRenderProxy()
	{
		if (mRVTInitialized)
			ShutdownRVT();
	}

	void RVTRenderProxy::InitializeRVT(const Config& InConfig, SceneManager* InSceneManager)
	{
		if (mRVTInitialized) return;
		if (!InSceneManager) return;

		mConfig = InConfig;
		mSceneManager = InSceneManager;

		// Update world AABB for spatial queries
		mWorldAABB.setExtents(
			Vector3(InConfig.WorldMinX, -1000.0f, InConfig.WorldMinY),
			Vector3(InConfig.WorldMaxX,  1000.0f, InConfig.WorldMaxY)
		);

		// Build VTSpaceDesc from config
		VTSpaceDesc SpaceDesc;
		SpaceDesc.Name = InConfig.Name;
		SpaceDesc.VirtualSizeX = InConfig.VirtualSizeX;
		SpaceDesc.VirtualSizeY = InConfig.VirtualSizeY;
		SpaceDesc.NumMipLevels = InConfig.NumMipLevels;
		SpaceDesc.NumLayers = InConfig.NumLayers;
		SpaceDesc.Type = EVirtualTextureType::RVT;
		SpaceDesc.WorldBoundsMinX = InConfig.WorldMinX;
		SpaceDesc.WorldBoundsMinY = InConfig.WorldMinY;
		SpaceDesc.WorldBoundsMaxX = InConfig.WorldMaxX;
		SpaceDesc.WorldBoundsMaxY = InConfig.WorldMaxY;

		for (uint8 i = 0; i < VTConstants::MaxLayers; ++i)
		{
			SpaceDesc.LayerFormats[i] = InConfig.LayerFormats[i];
		}

		// Create RuntimeVirtualTexture (render thread resource)
		mRVT = new RuntimeVirtualTexture(SpaceDesc, InSceneManager);
		mRVT->Initialize();

		// Create tile renderer (render thread resource)
		mTileRenderer = new RVTTileRenderer();
		RHICommandContext* Context = GetDynamicRHI()->GetDefaultCommandContext();
		mTileRenderer->Initialize(Context);

		mRVTInitialized = true;
	}

	void RVTRenderProxy::ShutdownRVT()
	{
		if (!mRVTInitialized) return;

		if (mTileRenderer)
		{
			mTileRenderer->Shutdown();
			delete mTileRenderer;
			mTileRenderer = nullptr;
		}

		if (mRVT)
		{
			mRVT->Shutdown();
			delete mRVT;
			mRVT = nullptr;
		}

		mSceneManager = nullptr;
		mRVTInitialized = false;
	}

	void RVTRenderProxy::UpdateTiles(RHICommandList* CmdList)
	{
		if (!mRVTInitialized || !mRVT || !mTileRenderer || !CmdList)
			return;

		// Render any pending tiles
		mTileRenderer->RenderPendingTiles(CmdList, mRVT);
	}

	void RVTRenderProxy::SetWorldBounds(float MinX, float MinY, float MaxX, float MaxY)
	{
		mConfig.WorldMinX = MinX;
		mConfig.WorldMinY = MinY;
		mConfig.WorldMaxX = MaxX;
		mConfig.WorldMaxY = MaxY;

		// Update AABB for spatial queries
		mWorldAABB.setExtents(
			Vector3(MinX, -1000.0f, MinY),
			Vector3(MaxX,  1000.0f, MaxY)
		);

		if (mRVT)
		{
			mRVT->SetWorldBounds(MinX, MinY, MaxX, MaxY);
		}
	}

	void RVTRenderProxy::InvalidateRegion(float MinX, float MinY, float MaxX, float MaxY)
	{
		if (mRVT)
		{
			mRVT->InvalidateWorldRegion(MinX, MinY, MaxX, MaxY);
		}
	}

	void RVTRenderProxy::InvalidateAll()
	{
		if (mRVT)
		{
			mRVT->InvalidateAll();
		}
	}

	uint8 RVTRenderProxy::GetSpaceID() const
	{
		return mRVT ? mRVT->GetSpaceID() : 0;
	}

	void RVTRenderProxy::UpdateConfig(const Config& InConfig)
	{
		bool bNeedsRebuild =
			(InConfig.VirtualSizeX != mConfig.VirtualSizeX) ||
			(InConfig.VirtualSizeY != mConfig.VirtualSizeY) ||
			(InConfig.NumLayers != mConfig.NumLayers) ||
			(InConfig.NumMipLevels != mConfig.NumMipLevels);

		if (bNeedsRebuild && mRVTInitialized)
		{
			// Resolution or layer count changed → full rebuild
			SceneManager* SceneMgrBackup = mSceneManager;
			ShutdownRVT();
			InitializeRVT(InConfig, SceneMgrBackup);
		}
		else
		{
			// Only bounds or soft parameters changed
			mConfig.MaxTileRendersPerFrame = InConfig.MaxTileRendersPerFrame;
			mConfig.Name = InConfig.Name;

			if (InConfig.WorldMinX != mConfig.WorldMinX ||
				InConfig.WorldMinY != mConfig.WorldMinY ||
				InConfig.WorldMaxX != mConfig.WorldMaxX ||
				InConfig.WorldMaxY != mConfig.WorldMaxY)
			{
				SetWorldBounds(InConfig.WorldMinX, InConfig.WorldMinY,
					InConfig.WorldMaxX, InConfig.WorldMaxY);
			}
		}
	}

	void RVTRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
	{
		// RVT volumes do not submit draw calls to the normal render queue.
		// Their tile rendering is handled separately by UpdateTiles().
		// However, we keep the proxy visible so the SceneManager tracks it.
	}

} // namespace Elaine

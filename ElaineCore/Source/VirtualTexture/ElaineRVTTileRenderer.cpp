#include "ElainePrecompiledHeader.h"
#include "VirtualTexture/ElaineRVTTileRenderer.h"
#include "VirtualTexture/ElaineRuntimeVirtualTexture.h"
#include "VirtualTexture/ElainePhysicalTilePool.h"
#include "render/common/ElaineRHI.h"
#include "render/common/ElaineRHICommandList.h"
#include "ElaineSceneManager.h"

namespace Elaine
{
	RVTTileRenderer::RVTTileRenderer()
	{
	}

	RVTTileRenderer::~RVTTileRenderer()
	{
		Shutdown();
	}

	void RVTTileRenderer::Initialize(RHICommandContext* InContext)
	{
		if (mInitialized || !InContext)
			return;

		EnsureTempRTsCreated(InContext);
		mInitialized = true;
	}

	void RVTTileRenderer::Shutdown()
	{
		if (!mInitialized) return;

		for (auto& RT : mTempRT)
		{
			if (RT)
			{
				GetDynamicRHI()->DestroyTexture(RT);
				RT = nullptr;
			}
		}
		if (mTempDepth)
		{
			GetDynamicRHI()->DestroyTexture(mTempDepth);
			mTempDepth = nullptr;
		}
		if (mTileUBO)
		{
			GetDynamicRHI()->DestroyBuffer(mTileUBO);
			mTileUBO = nullptr;
		}

		mInitialized = false;
	}

	void RVTTileRenderer::EnsureTempRTsCreated(RHICommandContext* Context)
	{
		const uint32 TileSize = VTConstants::TileSizeWithBorder;

		// Create temp RTs for each layer
		RHITextureCreateInfo RTInfo;
		RTInfo.Width = TileSize;
		RTInfo.Height = TileSize;
		RTInfo.Depth = 1;
		RTInfo.MipLevels = 1;
		RTInfo.Format = PF_R8G8B8A8;
		RTInfo.Usage = TextureCreateFlags::RenderTargetable | TextureCreateFlags::CopySrc;

		for (uint32 i = 0; i < VTConstants::MaxLayers; ++i)
		{
			if (!mTempRT[i])
			{
				mTempRT[i] = GetDynamicRHI()->CreateTexture(RTInfo);
			}
		}

		// Create temp depth
		if (!mTempDepth)
		{
			RHITextureCreateInfo DepthInfo;
			DepthInfo.Width = TileSize;
			DepthInfo.Height = TileSize;
			DepthInfo.Depth = 1;
			DepthInfo.MipLevels = 1;
			DepthInfo.Format = PF_DepthStencil;
			DepthInfo.Usage = TextureCreateFlags::DepthStencilTargetable;
			mTempDepth = GetDynamicRHI()->CreateTexture(DepthInfo);
		}

		// Create tile UBO
		if (!mTileUBO)
		{
			RHIBufferCreateInfo UBOInfo;
			UBOInfo.Size = sizeof(TileRenderUBO);
			UBOInfo.Usage = BufferUsage::UniformBuffer;
			UBOInfo.MemoryFlags = MemoryPropertyFlags::HostVisible | MemoryPropertyFlags::HostCoherent;
			mTileUBO = GetDynamicRHI()->CreateBuffer(UBOInfo);
		}
	}

	//=========================================================================
	// Render All Pending Tiles
	//=========================================================================

	void RVTTileRenderer::RenderPendingTiles(RHICommandList* CmdList, RuntimeVirtualTexture* RVT)
	{
		if (!CmdList || !RVT || !mInitialized)
			return;

		const auto& PendingRequests = RVT->GetPendingRenderRequests();
		if (PendingRequests.empty())
			return;

		// Render each tile
		for (const auto& Request : PendingRequests)
		{
			RenderTile(CmdList, RVT, Request);
		}

		// Clear pending requests after processing
		RVT->ClearPendingRenderRequests();
	}

	//=========================================================================
	// Render Single Tile
	//=========================================================================

	void RVTTileRenderer::RenderTile(RHICommandList* CmdList, RuntimeVirtualTexture* RVT,
		const RVTTileRenderRequest& Request)
	{
		if (!CmdList || !RVT)
			return;

		// Step 1: Set up orthographic projection
		SetupTileRenderState(CmdList, RVT, Request);

		// Step 2: Render scene to temporary RTs
		SceneManager* SceneMgr = RVT->GetSceneManager();
		if (SceneMgr)
		{
			RenderSceneToTile(CmdList, SceneMgr, Request);
		}

		// Step 3: Copy temporary RTs to physical atlas
		CopyTileToPhysicalAtlas(CmdList, Request);

		// Step 4: Notify RVT that tile is complete
		RVT->OnTileRenderComplete(Request.Coord, Request.PhysTarget);
	}

	//=========================================================================
	// Setup Tile Render State
	//=========================================================================

	void RVTTileRenderer::SetupTileRenderState(RHICommandList* CmdList,
		RuntimeVirtualTexture* RVT, const RVTTileRenderRequest& Request)
	{
		const uint32 TileSize = VTConstants::TileSizeWithBorder;

		// Set viewport to tile size
		CmdList->SetViewport(0, 0, 0, (float)TileSize, (float)TileSize, 1.0f);
		CmdList->SetScissorRect(true, 0, 0, TileSize, TileSize);

		// Compute and upload orthographic projection matrices
		TileRenderUBO UBOData;
		RVT->ComputeTileProjection(Request.Coord, UBOData.ViewMatrix, UBOData.ProjMatrix);
		UBOData.WorldBoundsMin[0] = Request.WorldMinX;
		UBOData.WorldBoundsMin[1] = Request.WorldMinY;
		UBOData.WorldBoundsMax[0] = Request.WorldMaxX;
		UBOData.WorldBoundsMax[1] = Request.WorldMaxY;

		// Upload to UBO
		if (mTileUBO)
		{
			void* Mapped = GetDynamicRHI()->MapBuffer(mTileUBO);
			if (Mapped)
			{
				memcpy(Mapped, &UBOData, sizeof(TileRenderUBO));
				GetDynamicRHI()->UnmapBuffer(mTileUBO);
			}
		}

		// Begin render pass with MRT (one RT per layer)
		// Clear all temporary RTs
		for (uint32 i = 0; i < VTConstants::MaxLayers; ++i)
		{
			if (mTempRT[i])
			{
				CmdList->ClearRenderTarget(mTempRT[i], 0.0f, 0.0f, 0.0f, 0.0f);
			}
		}
		if (mTempDepth)
		{
			CmdList->ClearDepthStencil(mTempDepth, 1.0f, 0);
		}
	}

	//=========================================================================
	// Render Scene Objects to Tile
	//=========================================================================

	void RVTTileRenderer::RenderSceneToTile(RHICommandList* CmdList,
		SceneManager* SceneMgr, const RVTTileRenderRequest& Request)
	{
		// Cull scene objects against the tile's world-space AABB
		// Render visible objects using the RVTCapture shader

		// The scene manager provides methods to render objects within a frustum.
		// For RVT, we use the orthographic frustum defined by the tile bounds.
		//
		// The RVTCapture.vs/ps shader outputs:
		//   RT0: BaseColor.rgb + Metallic.a
		//   RT1: Normal.rgb (tangent-space or world-space) + Roughness.a
		//   RT2: Roughness.r + Metallic.g + AO.b + 0.a
		//   RT3: Emissive.rgb + 0.a
		//
		// This is essentially a mini GBuffer pass with orthographic projection.

		// Bind the tile UBO as the common uniform buffer override
		// (replaces the main camera's view/proj matrices with the tile's ortho matrices)
		if (mTileUBO)
		{
			CmdList->BindUniformBuffer(0, 0, mTileUBO);
		}

		// Bind MRT
		for (uint32 i = 0; i < VTConstants::MaxLayers; ++i)
		{
			if (mTempRT[i])
			{
				CmdList->SetRenderTarget(i, mTempRT[i]);
			}
		}
		if (mTempDepth)
		{
			CmdList->SetDepthStencilTarget(mTempDepth);
		}

		// Render objects within the tile bounds using RVTCapture material override
		// SceneMgr->RenderObjectsInRegion() would cull and render
		// For now, we render all visible objects with the RVTCapture override
		SceneMgr->RenderWithMaterialOverride(CmdList, "RVTCapture",
			Request.WorldMinX, Request.WorldMinY,
			Request.WorldMaxX, Request.WorldMaxY);
	}

	//=========================================================================
	// Copy Tile to Physical Atlas
	//=========================================================================

	void RVTTileRenderer::CopyTileToPhysicalAtlas(RHICommandList* CmdList,
		const RVTTileRenderRequest& Request)
	{
		if (!Request.PhysTarget.IsValid())
			return;

		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem) return;

		PhysicalTilePool* Pool = VTSystem->GetPhysicalPool(Request.PhysTarget.PoolIndex);
		if (!Pool) return;

		// Copy each layer from temp RT to the physical atlas at the target location
		uint32 DestPixelX = Request.PhysTarget.GetPixelX();
		uint32 DestPixelY = Request.PhysTarget.GetPixelY();

		RHICopyTextureInfo CopyInfo;
		CopyInfo.Size.x = VTConstants::TileSizeWithBorder;
		CopyInfo.Size.y = VTConstants::TileSizeWithBorder;
		CopyInfo.Size.z = 1;
		CopyInfo.SourcePosition.x = 0;
		CopyInfo.SourcePosition.y = 0;
		CopyInfo.SourcePosition.z = 0;
		CopyInfo.DestPosition.x = DestPixelX;
		CopyInfo.DestPosition.y = DestPixelY;
		CopyInfo.DestPosition.z = 0;

		for (uint32 Layer = 0; Layer < VTConstants::MaxLayers; ++Layer)
		{
			RHITexture* AtlasTex = Pool->GetAtlasTexture(Layer);
			if (mTempRT[Layer] && AtlasTex)
			{
				CmdList->CopyTexture(mTempRT[Layer], AtlasTex, CopyInfo);
			}
		}
	}

} // namespace Elaine

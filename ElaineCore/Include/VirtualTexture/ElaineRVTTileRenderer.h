#pragma once
#include "Common/ElaineCorePrerequirements.h"
#include "VirtualTexture/ElaineVirtualTextureTypes.h"

namespace Elaine
{
	class RuntimeVirtualTexture;
	class RHICommandList;
	class RHICommandContext;
	class RHITexture;
	struct RVTTileRenderRequest;
	class SceneManager;

	/**
	 * RVTTileRenderer - Renders tiles for Runtime Virtual Textures
	 *
	 * When a tile is requested (via feedback analysis or explicit request),
	 * this renderer captures the scene from a top-down orthographic view
	 * into temporary render targets, then copies the results into the
	 * physical tile atlas.
	 *
	 * Rendering flow per tile:
	 *   1. Compute orthographic projection from tile's world-space AABB
	 *   2. Set viewport to TileSizeWithBorder x TileSizeWithBorder
	 *   3. Render scene objects within the tile's frustum (using RVTCapture shader)
	 *   4. MRT output to temporary RTs: BaseColor, Normal, RMA (per layer)
	 *   5. Copy each RT layer to the corresponding physical atlas at PhysTarget location
	 *   6. Notify RuntimeVirtualTexture::OnTileRenderComplete()
	 *
	 * The temporary RTs are allocated once and reused across all tile renders.
	 * Only one tile is rendered at a time (no batching across tiles).
	 */
	class ElaineCoreExport RVTTileRenderer
	{
	public:
		RVTTileRenderer();
		~RVTTileRenderer();

		/** Initialize temporary render targets */
		void Initialize(RHICommandContext* InContext);

		/** Release GPU resources */
		void Shutdown();

		/**
		 * Render all pending tiles for the given RVT.
		 * Called once per frame from the render pipeline, after VT CPU update.
		 *
		 * @param CmdList - Active command list
		 * @param RVT - The runtime virtual texture whose tiles need rendering
		 */
		void RenderPendingTiles(RHICommandList* CmdList, RuntimeVirtualTexture* RVT);

		/**
		 * Render a single tile.
		 *
		 * @param CmdList - Active command list
		 * @param RVT - The runtime virtual texture
		 * @param Request - Tile render request with world bounds and target location
		 */
		void RenderTile(RHICommandList* CmdList, RuntimeVirtualTexture* RVT,
			const RVTTileRenderRequest& Request);

	private:
		/** Set up orthographic projection and viewport for a tile */
		void SetupTileRenderState(RHICommandList* CmdList,
			RuntimeVirtualTexture* RVT,
			const RVTTileRenderRequest& Request);

		/** Render scene objects into temporary RTs */
		void RenderSceneToTile(RHICommandList* CmdList,
			SceneManager* SceneMgr,
			const RVTTileRenderRequest& Request);

		/** Copy temporary RT layers to physical atlas */
		void CopyTileToPhysicalAtlas(RHICommandList* CmdList,
			const RVTTileRenderRequest& Request);

		/** Ensure temporary RTs are created */
		void EnsureTempRTsCreated(RHICommandContext* Context);

	private:
		bool mInitialized = false;

		// Temporary render targets (TileSizeWithBorder x TileSizeWithBorder)
		// One per layer: BaseColor, Normal, RMA, Emissive
		RHITexture* mTempRT[VTConstants::MaxLayers] = {};

		// Depth buffer for tile rendering
		RHITexture* mTempDepth = nullptr;

		// Tile UBO for per-tile projection matrices
		RHIBuffer* mTileUBO = nullptr;

		// Per-tile uniform data
		struct TileRenderUBO
		{
			float ViewMatrix[16];
			float ProjMatrix[16];
			float WorldBoundsMin[2];
			float WorldBoundsMax[2];
		};
	};

} // namespace Elaine

#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "VirtualTexture/ElaineVirtualTextureTypes.h"

namespace Elaine
{
	class RuntimeVirtualTexture;
	class RVTTileRenderer;
	class RHICommandList;
	class RHICommandContext;

	/**
	 * RVTRenderProxy - 渲染线程端的 RVT Volume 代理
	 *
	 * 生命周期由 SceneManager 管理（在渲染线程创建和销毁）。
	 * 逻辑线程端的 RVTVolumeComponent 通过 ENQUEUE_RENDER_COMMAND 
	 * 向本 Proxy 传递配置变更和失效请求。
	 *
	 * 所有成员变量仅在渲染线程访问，无需加锁。
	 *
	 * 数据流：
	 *   Logic Thread:  RVTVolumeComponent --ENQUEUE--> RVTRenderProxy
	 *   Render Thread: RenderPipeline --> RVTRenderProxy::UpdateTiles()
	 */
	class ElaineCoreExport RVTRenderProxy : public RenderProxy
	{
	public:
		//=====================================================================
		// Configuration (set from logic thread via ENQUEUE_RENDER_COMMAND)
		//=====================================================================

		struct Config
		{
			/** Human-readable name for debugging */
			std::string Name = "RVTVolume";

			/** World-space bounds of the volume */
			float WorldMinX = -500.0f;
			float WorldMinY = -500.0f;
			float WorldMaxX =  500.0f;
			float WorldMaxY =  500.0f;

			/** Virtual texture resolution (pixels, must be power of 2) */
			uint32 VirtualSizeX = 4096;
			uint32 VirtualSizeY = 4096;

			/** Number of layers (1-4) */
			uint8 NumLayers = 4;

			/** Per-layer pixel formats */
			PixelFormat LayerFormats[VTConstants::MaxLayers] = {
				PF_R8G8B8A8,   // BaseColor + Metallic
				PF_R8G8B8A8,   // Normal + Roughness
				PF_R8G8B8A8,   // RMA packed
				PF_R8G8B8A8    // Emissive
			};

			/** Maximum number of tiles to render per frame (performance budget) */
			uint32 MaxTileRendersPerFrame = 16;

			/** Number of mip levels (0 = auto-compute) */
			uint8 NumMipLevels = 0;
		};

		//=====================================================================
		// Lifecycle (render thread only)
		//=====================================================================

		RVTRenderProxy();
		virtual ~RVTRenderProxy();

		/**
		 * Initialize RVT resources on the render thread.
		 * Creates RuntimeVirtualTexture and RVTTileRenderer.
		 * Called from ENQUEUE_RENDER_COMMAND in RVTVolumeComponent::OnRegisterWorldImpl.
		 *
		 * @param InConfig - Volume configuration (copied from logic thread)
		 * @param InSceneManager - The scene manager (render thread object)
		 */
		void InitializeRVT(const Config& InConfig, SceneManager* InSceneManager);

		/**
		 * Shutdown and release all RVT resources.
		 * Called before destruction (from ENQUEUE_RENDER_COMMAND or DestroyRenderProxy).
		 */
		void ShutdownRVT();

		/** Is this proxy fully initialized? */
		bool IsRVTInitialized() const { return mRVTInitialized; }

		//=====================================================================
		// Per-Frame Update (render thread only)
		//=====================================================================

		/**
		 * Called once per frame from RenderPipeline to process pending tile
		 * render requests. This is the main entry point for RVT tile updates.
		 *
		 * @param CmdList - Active command list
		 */
		void UpdateTiles(RHICommandList* CmdList);

		//=====================================================================
		// Configuration Updates (render thread only, set via ENQUEUE)
		//=====================================================================

		/**
		 * Update world-space bounds. Invalidates all tiles.
		 */
		void SetWorldBounds(float MinX, float MinY, float MaxX, float MaxY);

		/**
		 * Invalidate a world-space region (tiles overlapping will be re-rendered).
		 */
		void InvalidateRegion(float MinX, float MinY, float MaxX, float MaxY);

		/**
		 * Invalidate all tiles (force full re-render next frame).
		 */
		void InvalidateAll();

		/**
		 * Update the full configuration. If resolution/layers changed,
		 * this will shutdown and re-initialize the RVT with the new config.
		 * If only bounds changed, it will just update bounds and invalidate.
		 *
		 * @param InConfig - New configuration
		 */
		void UpdateConfig(const Config& InConfig);

		//=====================================================================
		// Accessors (render thread only)
		//=====================================================================

		/** Get the underlying RuntimeVirtualTexture */
		RuntimeVirtualTexture* GetRVT() const { return mRVT; }

		/** Get the tile renderer */
		RVTTileRenderer* GetTileRenderer() const { return mTileRenderer; }

		/** Get the VT space ID */
		uint8 GetSpaceID() const;

		/** Get current config */
		const Config& GetConfig() const { return mConfig; }

		//=====================================================================
		// RenderProxy overrides (render thread only)
		//=====================================================================

		void UpdateRenderQueue(RenderQueueSet* InRenderQueue) override;

	private:
		Config mConfig;
		bool mRVTInitialized = false;

		/** The runtime virtual texture managed by this proxy */
		RuntimeVirtualTexture* mRVT = nullptr;

		/** Tile renderer for producing tile content */
		RVTTileRenderer* mTileRenderer = nullptr;

		/** Scene manager reference (render thread object, not owned) */
		SceneManager* mSceneManager = nullptr;
	};

} // namespace Elaine

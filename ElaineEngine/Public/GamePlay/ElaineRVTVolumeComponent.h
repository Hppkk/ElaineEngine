#pragma once
#include "ElaineEnginePrerequirements.h"
#include "GamePlay/ElaineComponent.h"
#include "GamePlay/ElaineComponentFactory.h"
#include "ElaineReflectionDefines.h"
#include "VirtualTexture/ElaineVirtualTextureTypes.h"
#include "ElaineRVTVolumeComponent.generated.h"

namespace Elaine
{
	class RVTRenderProxy;

	/**
	 * RVTVolumeComponentInfo - Serialization info for RVT volumes
	 */
	class ElaineEngineExport RVTVolumeComponentInfo : public ComponentInfo
	{
	public:
		/** Human-readable name */
		std::string VolumeName = "RVTVolume";

		/** World bounds */
		float WorldMinX = -500.0f;
		float WorldMinY = -500.0f;
		float WorldMaxX =  500.0f;
		float WorldMaxY =  500.0f;

		/** Resolution */
		uint32 VirtualSizeX = 4096;
		uint32 VirtualSizeY = 4096;

		/** Layers */
		uint8 NumLayers = 4;
		uint8 NumMipLevels = 0;
		uint32 MaxTileRendersPerFrame = 16;
	};

	/**
	 * RVTVolumeComponent - Logic thread component for defining RVT regions.
	 *
	 * Architecture:
	 *   Logic Thread:  RVTVolumeComponent (this class)
	 *                    ├─ Config data (world bounds, resolution, layers)
	 *                    └─ mRenderProxy* (bridge pointer to render thread)
	 *
	 *   Render Thread: RVTRenderProxy (in ElaineCore)
	 *                    ├─ RuntimeVirtualTexture*
	 *                    ├─ RVTTileRenderer*
	 *                    └─ All GPU resources
	 *
	 * All render-thread operations are dispatched via ENQUEUE_RENDER_COMMAND.
	 * This component NEVER directly accesses render-thread resources.
	 */
	ECLASS(DisplayName = "RVT Volume")
	class ElaineEngineExport RVTVolumeComponent : public Component
	{
		GENERATED_BODY()
	public:
		RVTVolumeComponent(GameObject* InObject);
		virtual ~RVTVolumeComponent();

		//=====================================================================
		// Component interface overrides
		//=====================================================================

		const Name& GetType() const override;
		void OnRegisterWorldImpl(World* InWorld) override;
		void OnUnregisterWorldImpl() override;

		//=====================================================================
		// Configuration (logic thread, set before or after registration)
		//=====================================================================

		/**
		 * Set the world-space bounds of the RVT volume.
		 * If already registered, this will ENQUEUE an update to the render proxy
		 * which invalidates all tiles and triggers re-render.
		 */
		EFUNCTION(Category = "RVT")
		void SetWorldBounds(float MinX, float MinY, float MaxX, float MaxY);

		/** Get the current world bounds */
		void GetWorldBounds(float& OutMinX, float& OutMinY,
			float& OutMaxX, float& OutMaxY) const;

		/**
		 * Invalidate a world-space region.
		 * Tiles overlapping this region will be re-rendered next frame.
		 * Dispatched to render thread via ENQUEUE.
		 */
		EFUNCTION(Category = "RVT")
		void InvalidateRegion(float MinX, float MinY, float MaxX, float MaxY);

		/**
		 * Invalidate all tiles (force full re-render).
		 */
		EFUNCTION(Category = "RVT")
		void InvalidateAll();

		/** Set volume name (for debugging) */
		EFUNCTION(Category = "RVT")
		void SetVolumeName(const std::string& InName);

		/** Set virtual texture resolution */
		EFUNCTION(Category = "RVT")
		void SetResolution(uint32 SizeX, uint32 SizeY);

		/** Set number of layers (1-4) */
		EFUNCTION(Category = "RVT")
		void SetNumLayers(uint8 InNumLayers);

		/** Set max tiles to render per frame */
		EFUNCTION(Category = "RVT")
		void SetMaxTileRendersPerFrame(uint32 InMax);

	private:
		/** Enqueue full config update to render proxy */
		void MarkRenderStateDirty();

	private:
		//=====================================================================
		// Logic thread data (NEVER accessed by render thread directly)
		//=====================================================================

		/** Bridge pointer to the render proxy (owned by SceneManager) */
		RVTRenderProxy* mRenderProxy = nullptr;

		/** Configuration — all values are logic-thread copies */
		EPROPERTY(DisplayName = "Volume Name", Category = "RVT", Tooltip = "Debug name for the RVT volume")
		std::string mVolumeName = "RVTVolume";

		EPROPERTY(DisplayName = "World Min X", Category = "RVT Bounds")
		float mWorldMinX = -500.0f;

		EPROPERTY(DisplayName = "World Min Y", Category = "RVT Bounds")
		float mWorldMinY = -500.0f;

		EPROPERTY(DisplayName = "World Max X", Category = "RVT Bounds")
		float mWorldMaxX = 500.0f;

		EPROPERTY(DisplayName = "World Max Y", Category = "RVT Bounds")
		float mWorldMaxY = 500.0f;

		EPROPERTY(DisplayName = "Virtual Size X", Category = "RVT Resolution", Min = 256, Max = 16384)
		uint32 mVirtualSizeX = 4096;

		EPROPERTY(DisplayName = "Virtual Size Y", Category = "RVT Resolution", Min = 256, Max = 16384)
		uint32 mVirtualSizeY = 4096;

		EPROPERTY(DisplayName = "Num Layers", Category = "RVT Layers", Min = 1, Max = 4)
		uint8 mNumLayers = 4;

		EPROPERTY(DisplayName = "Num Mip Levels", Category = "RVT Layers", Tooltip = "0 = auto-compute")
		uint8 mNumMipLevels = 0;

		EPROPERTY(DisplayName = "Max Tile Renders Per Frame", Category = "RVT Performance", Min = 1, Max = 64)
		uint32 mMaxTileRendersPerFrame = 16;
	};

	DEFINE_COM_FACTORY(RVTVolumeComponent);
}

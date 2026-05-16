#pragma once
#include "Common/ElaineCorePrerequirements.h"
#include "VirtualTexture/ElaineVirtualTextureTypes.h"

namespace Elaine
{
	class RHITexture;
	class RHIBuffer;
	class VirtualTextureSpace;

	//=========================================================================
	// VTMaterialFlags - Tag-based VT binding for materials
	//=========================================================================
	/**
	 * Standalone VT binding info attached to materials via tag/flag mechanism.
	 * Does NOT modify MaterialInterface base class.
	 *
	 * Usage in material JSON:
	 *   "virtual_texture": {
	 *       "enabled": true,
	 *       "space_id": 0,
	 *       "layer_count": 4
	 *   }
	 *
	 * Usage in code:
	 *   VTMaterialBinding* vtBinding = VTMaterialBindingManager::Get(materialPtr);
	 *   if (vtBinding && vtBinding->IsEnabled())
	 *       // bind VT resources instead of traditional textures
	 */
	struct VTMaterialFlags
	{
		/** Whether this material uses Virtual Texturing */
		bool bUseVirtualTexture = false;

		/** The VT space ID this material samples from */
		uint8 SpaceID = 0;

		/** Number of VT layers this material uses (1-4) */
		uint8 LayerCount = 1;

		/** Which layers are enabled (bitmask) */
		uint8 LayerMask = 0x01; // bit 0=BaseColor, 1=Normal, 2=RMA, 3=Emissive

		/** VT type (SVT or RVT) */
		EVirtualTextureType VTType = EVirtualTextureType::SVT;

		bool HasBaseColorLayer() const { return (LayerMask & (1 << 0)) != 0; }
		bool HasNormalLayer()    const { return (LayerMask & (1 << 1)) != 0; }
		bool HasRMALayer()       const { return (LayerMask & (1 << 2)) != 0; }
		bool HasEmissiveLayer()  const { return (LayerMask & (1 << 3)) != 0; }

		bool IsEnabled() const { return bUseVirtualTexture; }
	};

	//=========================================================================
	// VTMaterialBinding - Runtime binding that holds GPU resource references
	//=========================================================================
	/**
	 * Holds the actual GPU resource references for a VT material.
	 * Created and cached by VTMaterialBindingManager when a VT material
	 * is first encountered during rendering.
	 */
	struct VTMaterialBinding
	{
		VTMaterialFlags Flags;

		//---------------------------------------------------------------------
		// GPU Resources (resolved at bind time from VirtualTextureSystem)
		//---------------------------------------------------------------------

		/** Indirection texture for this space (R16G16B16A16_UINT) */
		RHITexture* IndirectionTexture = nullptr;

		/** Physical atlas textures per layer */
		RHITexture* PhysicalAtlas[VTConstants::MaxLayers] = {};

		/** VT space parameters UBO (VTSpaceParams struct, uploaded to GPU) */
		RHIBuffer* SpaceParamsUBO = nullptr;

		//---------------------------------------------------------------------
		// Cached parameters (mirrors VTSpaceParams shader struct)
		//---------------------------------------------------------------------
		struct SpaceParamsGPU
		{
			float VirtualTextureSizeX = 4096.0f;
			float VirtualTextureSizeY = 4096.0f;
			float InvVirtualTextureSizeX = 1.0f / 4096.0f;
			float InvVirtualTextureSizeY = 1.0f / 4096.0f;
			float TileSize = 128.0f;
			float InvTileSize = 1.0f / 128.0f;
			float MaxMipLevel = 5.0f;
			float PhysicalAtlasSize = 4352.0f; // 32 * 136
		};
		SpaceParamsGPU CachedSpaceParams;

		/** Whether GPU resources have been resolved */
		bool bResourcesResolved = false;

		/** Resolve GPU resources from the VirtualTextureSystem */
		void ResolveResources(VirtualTextureSpace* Space, uint8 PoolIndex);

		/** Check if this binding is valid and ready for rendering */
		bool IsValid() const
		{
			return Flags.IsEnabled() && bResourcesResolved && IndirectionTexture != nullptr;
		}
	};

	//=========================================================================
	// VTMaterialBindingManager - Singleton that maps materials to VT bindings
	//=========================================================================
	/**
	 * Manages the association between materials and their VT binding info.
	 * Uses a hash map keyed by material pointer (or material ID).
	 *
	 * This implements the "Tag" pattern: materials don't know about VT,
	 * but the VT system can attach VT info to them externally.
	 *
	 * ⚠️ THREAD SAFETY:
	 * This manager is a RENDER THREAD ONLY object. All methods must be called
	 * from the render thread. When logic-thread code needs to register/unregister
	 * a VT material, it MUST use ENQUEUE_RENDER_COMMAND to dispatch the call:
	 *
	 *   ENQUEUE_RENDER_COMMAND(RegisterVT)([MatPtr, Flags](RenderContext& Ctx) {
	 *       VTMaterialBindingManager::instance()->RegisterVTMaterial(MatPtr, Flags);
	 *   });
	 *
	 * The internal map (mBindings) is NOT protected by any mutex because all
	 * access is serialized on the render thread.
	 */
	class ElaineCoreExport VTMaterialBindingManager : public Singleton<VTMaterialBindingManager>
	{
	public:
		VTMaterialBindingManager();
		~VTMaterialBindingManager();

		//---------------------------------------------------------------------
		// Registration (RENDER THREAD ONLY)
		//---------------------------------------------------------------------

		/**
		 * Register a material as a VT material.
		 * Called during material loading when "virtual_texture" section is found.
		 * ⚠️ Must be called on the render thread (via ENQUEUE_RENDER_COMMAND).
		 */
		void RegisterVTMaterial(void* MaterialPtr, const VTMaterialFlags& Flags);

		/**
		 * Unregister a material (called during material destruction).
		 * ⚠️ Must be called on the render thread (via ENQUEUE_RENDER_COMMAND).
		 */
		void UnregisterVTMaterial(void* MaterialPtr);

		//---------------------------------------------------------------------
		// Queries (RENDER THREAD ONLY)
		//---------------------------------------------------------------------

		/** Get the VT binding for a material, or nullptr if not a VT material */
		VTMaterialBinding* GetBinding(void* MaterialPtr);
		const VTMaterialBinding* GetBinding(void* MaterialPtr) const;

		/** Quick check if a material uses VT */
		bool IsVTMaterial(void* MaterialPtr) const;

		//---------------------------------------------------------------------
		// Resource Resolution (RENDER THREAD ONLY)
		//---------------------------------------------------------------------

		/**
		 * Resolve GPU resources for all registered VT materials.
		 * Called once during initialization or when VT spaces change.
		 */
		void ResolveAllBindings();

		/**
		 * Invalidate all resolved bindings (e.g., when physical pools are rebuilt).
		 */
		void InvalidateAllBindings();

	private:
		/**
		 * Map from material pointer to its VT binding.
		 * No mutex needed — all access is on the render thread.
		 */
		std::unordered_map<void*, VTMaterialBinding> mBindings;
	};

} // namespace Elaine

#include "ElainePrecompiledHeader.h"
#include "VirtualTexture/ElaineVTMaterialBinding.h"
#include "VirtualTexture/ElaineVirtualTextureSpace.h"
#include "VirtualTexture/ElainePhysicalTilePool.h"
#include "render/common/ElaineRHI.h"
#include "ElaineThreadManager.h"

// Render thread assertion macro
// In debug builds, asserts that the current code is running on the render thread.
// In release builds, this is a no-op for performance.
#ifdef _DEBUG
#define ASSERT_RENDER_THREAD() \
	do { \
		if (Root::instanceExists() && !Root::instance()->CheckThread(NamedThread::RenderThread)) \
		{ \
			LOG_ERROR("VTMaterialBindingManager: Method called from non-render thread!"); \
		} \
	} while(0)
#else
#define ASSERT_RENDER_THREAD() ((void)0)
#endif

namespace Elaine
{
	//=========================================================================
	// VTMaterialBinding
	//=========================================================================

	void VTMaterialBinding::ResolveResources(VirtualTextureSpace* Space, uint8 PoolIndex)
	{
		if (!Space || !Flags.IsEnabled())
		{
			bResourcesResolved = false;
			return;
		}

		// Get indirection texture from the space
		IndirectionTexture = Space->GetIndirectionTexture();

		// Get physical atlas textures from the VT system's physical pool
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (VTSystem)
		{
			PhysicalTilePool* Pool = VTSystem->GetPhysicalPool(PoolIndex);
			if (Pool)
			{
				for (uint8 i = 0; i < VTConstants::MaxLayers; ++i)
				{
					if (i < Flags.LayerCount && (Flags.LayerMask & (1 << i)))
					{
						PhysicalAtlas[i] = Pool->GetAtlasTexture(i);
					}
					else
					{
						PhysicalAtlas[i] = nullptr;
					}
				}
			}
		}

		// Update cached space parameters
		const VTSpaceDesc& Desc = Space->GetDesc();
		CachedSpaceParams.VirtualTextureSizeX = (float)Desc.VirtualSizeX;
		CachedSpaceParams.VirtualTextureSizeY = (float)Desc.VirtualSizeY;
		CachedSpaceParams.InvVirtualTextureSizeX = 1.0f / (float)Desc.VirtualSizeX;
		CachedSpaceParams.InvVirtualTextureSizeY = 1.0f / (float)Desc.VirtualSizeY;
		CachedSpaceParams.TileSize = (float)VTConstants::TileSize;
		CachedSpaceParams.InvTileSize = 1.0f / (float)VTConstants::TileSize;
		CachedSpaceParams.MaxMipLevel = (float)(Space->GetNumMipLevels() - 1);
		CachedSpaceParams.PhysicalAtlasSize = (float)VTConstants::PhysicalAtlasSize;

		// Create or update the UBO for space params
		if (!SpaceParamsUBO)
		{
			RHIBufferCreateInfo UBOInfo;
			UBOInfo.Size = sizeof(SpaceParamsGPU);
			UBOInfo.Usage = BufferUsage::UniformBuffer;
			UBOInfo.MemoryFlags = MemoryPropertyFlags::HostVisible | MemoryPropertyFlags::HostCoherent;
			SpaceParamsUBO = GetDynamicRHI()->CreateBuffer(UBOInfo);
		}

		if (SpaceParamsUBO)
		{
			void* Mapped = GetDynamicRHI()->MapBuffer(SpaceParamsUBO);
			if (Mapped)
			{
				memcpy(Mapped, &CachedSpaceParams, sizeof(SpaceParamsGPU));
				GetDynamicRHI()->UnmapBuffer(SpaceParamsUBO);
			}
		}

		bResourcesResolved = (IndirectionTexture != nullptr);
	}

	//=========================================================================
	// VTMaterialBindingManager
	//=========================================================================

	VTMaterialBindingManager::VTMaterialBindingManager()
	{
	}

	VTMaterialBindingManager::~VTMaterialBindingManager()
	{
		mBindings.clear();
	}

	void VTMaterialBindingManager::RegisterVTMaterial(void* MaterialPtr, const VTMaterialFlags& Flags)
	{
		ASSERT_RENDER_THREAD();
		if (!MaterialPtr) return;

		VTMaterialBinding Binding;
		Binding.Flags = Flags;
		Binding.bResourcesResolved = false;

		mBindings[MaterialPtr] = std::move(Binding);
	}

	void VTMaterialBindingManager::UnregisterVTMaterial(void* MaterialPtr)
	{
		ASSERT_RENDER_THREAD();
		if (!MaterialPtr) return;

		auto It = mBindings.find(MaterialPtr);
		if (It != mBindings.end())
		{
			// Clean up UBO if owned
			if (It->second.SpaceParamsUBO)
			{
				GetDynamicRHI()->DestroyBuffer(It->second.SpaceParamsUBO);
				It->second.SpaceParamsUBO = nullptr;
			}
			mBindings.erase(It);
		}
	}

	VTMaterialBinding* VTMaterialBindingManager::GetBinding(void* MaterialPtr)
	{
		auto It = mBindings.find(MaterialPtr);
		if (It != mBindings.end())
		{
			return &It->second;
		}
		return nullptr;
	}

	const VTMaterialBinding* VTMaterialBindingManager::GetBinding(void* MaterialPtr) const
	{
		auto It = mBindings.find(MaterialPtr);
		if (It != mBindings.end())
		{
			return &It->second;
		}
		return nullptr;
	}

	bool VTMaterialBindingManager::IsVTMaterial(void* MaterialPtr) const
	{
		auto It = mBindings.find(MaterialPtr);
		return It != mBindings.end() && It->second.Flags.IsEnabled();
	}

	void VTMaterialBindingManager::ResolveAllBindings()
	{
		ASSERT_RENDER_THREAD();
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem || !VTSystem->IsInitialized())
			return;

		for (auto& [MatPtr, Binding] : mBindings)
		{
			if (Binding.Flags.IsEnabled() && !Binding.bResourcesResolved)
			{
				VirtualTextureSpace* Space = VTSystem->GetSpace(Binding.Flags.SpaceID);
				if (Space)
				{
					// Use pool index 0 by default (could be customized per-material)
					Binding.ResolveResources(Space, 0);
				}
			}
		}
	}

	void VTMaterialBindingManager::InvalidateAllBindings()
	{
		ASSERT_RENDER_THREAD();
		for (auto& [MatPtr, Binding] : mBindings)
		{
			Binding.bResourcesResolved = false;
			Binding.IndirectionTexture = nullptr;
			for (auto& Atlas : Binding.PhysicalAtlas)
				Atlas = nullptr;
		}
	}

} // namespace Elaine

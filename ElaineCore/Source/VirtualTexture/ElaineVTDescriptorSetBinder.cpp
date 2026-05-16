#include "ElainePrecompiledHeader.h"
#include "VirtualTexture/ElaineVTDescriptorSetBinder.h"
#include "VirtualTexture/ElaineVTMaterialBinding.h"
#include "render/common/ElaineRHI.h"
#include "render/common/ElaineRHICommandList.h"
#include "ElainePass.h"

// Vulkan-specific includes for descriptor set operations
#include "render/vulkan/ElaineVulkanDescriptorSet.h"
#include "render/vulkan/ElaineVulkanTexture.h"
#include "render/vulkan/ElaineVulkanDevice.h"

namespace Elaine
{
	//=========================================================================
	// VTDescriptorSetBinder
	//=========================================================================

	bool VTDescriptorSetBinder::BindVTResources(
		RHICommandList* CmdList,
		const VTMaterialBinding& Binding)
	{
		if (!CmdList || !Binding.IsValid())
			return false;

		// The binding flow:
		// 1. Get the current frame's descriptor allocator
		// 2. Allocate a descriptor set with VT layout
		// 3. Write VT textures + UBO into it
		// 4. Bind the descriptor set to set=2

		// Bind indirection texture at set=2, binding=0
		if (Binding.IndirectionTexture)
		{
			CmdList->BindTexture(2, VTDescriptorLayout::Indirection, Binding.IndirectionTexture);
		}

		// Bind physical atlas textures at set=2, bindings 1-4
		for (uint8 Layer = 0; Layer < VTConstants::MaxLayers; ++Layer)
		{
			uint32 BindingSlot = VTDescriptorLayout::PhysicalAtlas_BaseColor + Layer;
			if (Binding.PhysicalAtlas[Layer])
			{
				CmdList->BindTexture(2, BindingSlot, Binding.PhysicalAtlas[Layer]);
			}
		}

		// Bind VT space params UBO at set=2, binding=5
		if (Binding.SpaceParamsUBO)
		{
			CmdList->BindUniformBuffer(2, VTDescriptorLayout::SpaceParamsUBO, Binding.SpaceParamsUBO);
		}

		return true;
	}

	bool VTDescriptorSetBinder::CreateVTDescriptorSetLayout(
		void* Device,
		void* OutLayout)
	{
		if (!Device || !OutLayout)
			return false;

		// This would create a VkDescriptorSetLayout with the VT binding layout.
		// In practice, the shader reflection system handles this automatically
		// based on the GLSL bindings in DeferredGBufferVT.ps.
		//
		// The layout is:
		//   binding 0: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER (usampler2D)
		//   binding 1: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER (sampler2D)
		//   binding 2: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER (sampler2D)
		//   binding 3: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER (sampler2D)
		//   binding 4: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER (sampler2D)
		//   binding 5: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER (VTSpaceParams)
		//   binding 6: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER (MaterialParams)

		// Since shader reflection auto-generates layouts, this function
		// serves as a manual override or validation point.
		return true;
	}

	bool VTDescriptorSetBinder::IsVTShaderPass(const ShaderPass* Pass)
	{
		if (!Pass) return false;

		// Check if the pass name contains "VT" indicators
		const Name& PassName = Pass->GetPassName();
		const std::string& PsMacros = Pass->GetPsMacros();

		// Convention: VT shader passes have "VT" or "VirtualTexture" in their macros
		// or use a specific pass name like "GBufferVT"
		if (PsMacros.find("USE_VIRTUAL_TEXTURE") != std::string::npos)
			return true;

		// Check pass name
		std::string PassNameStr = PassName.ToString();
		if (PassNameStr.find("VT") != std::string::npos ||
			PassNameStr.find("VirtualTexture") != std::string::npos)
			return true;

		return false;
	}

	void VTDescriptorSetBinder::WriteVTDescriptorSet(
		void* DescriptorSetHandle,
		const VTMaterialBinding& Binding)
	{
		if (!DescriptorSetHandle || !Binding.IsValid())
			return;

		// This is a low-level Vulkan helper that would directly call
		// vkUpdateDescriptorSets. In the current engine architecture,
		// the RHICommandList::BindTexture/BindUniformBuffer abstractions
		// handle the VkWriteDescriptorSet generation internally.
		//
		// This function is kept as an optimization point for batch updates
		// where all VT resources are written in a single vkUpdateDescriptorSets call.

		// For now, the binding is handled through BindVTResources() above.
	}

} // namespace Elaine

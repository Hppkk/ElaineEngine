#pragma once
#include "Common/ElaineCorePrerequirements.h"

namespace VulkanRHI
{
	class VulkanDescriptorSet;
	class VulkanDescriptorSetManager;
	class VulkanTexture;
	class VulkanDevice;
}

namespace Elaine
{
	class RHICommandList;
	class RHITexture;
	class RHIBuffer;
	struct VTMaterialBinding;

	/**
	 * VT Descriptor Set Binder
	 *
	 * Manages the dynamic switching of Descriptor Set 2 (per-material)
	 * between traditional texture bindings and VT texture bindings.
	 *
	 * Traditional layout (set=2):
	 *   binding 0: sampler2D texBaseColor
	 *   binding 1: sampler2D texNormal
	 *   binding 2: sampler2D texMetallicRoughness
	 *   binding 3: sampler2D texEmissive
	 *   binding 4: sampler2D texAO
	 *   binding 5: uniform MaterialParams
	 *
	 * VT layout (set=2):
	 *   binding 0: usampler2D vtIndirection        (R16G16B16A16_UINT)
	 *   binding 1: sampler2D  vtPhysicalBaseColor   (physical atlas layer 0)
	 *   binding 2: sampler2D  vtPhysicalNormal      (physical atlas layer 1)
	 *   binding 3: sampler2D  vtPhysicalRMA         (physical atlas layer 2)
	 *   binding 4: sampler2D  vtPhysicalEmissive    (physical atlas layer 3)
	 *   binding 5: uniform VTSpaceParams            (VT space parameters UBO)
	 *   binding 6: uniform MaterialParams           (material parameters - same as traditional)
	 *
	 * The layout is selected per-drawcall based on VTMaterialBinding::IsValid().
	 */
	class ElaineCoreExport VTDescriptorSetBinder
	{
	public:
		/**
		 * Bind VT resources to the per-material descriptor set.
		 * Called in the GBuffer pass execute lambda when a VT material is detected.
		 *
		 * @param CmdList - Active command list
		 * @param Binding - Resolved VT material binding with GPU resource references
		 * @return true if binding succeeded
		 */
		static bool BindVTResources(
			RHICommandList* CmdList,
			const VTMaterialBinding& Binding);

		/**
		 * Create a VkDescriptorSetLayout suitable for VT material bindings.
		 * Used when building the VT variant of a ShaderPass pipeline.
		 *
		 * @param Device - Vulkan device
		 * @param OutLayout - Output layout handle
		 * @return true if creation succeeded
		 */
		static bool CreateVTDescriptorSetLayout(
			void* Device,
			void* OutLayout);

		/**
		 * Check if the given shader pass should use VT descriptor layout.
		 * This is determined by the shader's pass name or macros.
		 */
		static bool IsVTShaderPass(const class ShaderPass* Pass);

	private:
		/**
		 * Helper: write VT textures + UBO into descriptor set via Vulkan API.
		 * Uses VkWriteDescriptorSet array for batched update.
		 */
		static void WriteVTDescriptorSet(
			void* DescriptorSetHandle,
			const VTMaterialBinding& Binding);
	};

	//=========================================================================
	// VT Descriptor Layout Bindings (matches GLSL set=2 layout for VT)
	//=========================================================================
	namespace VTDescriptorLayout
	{
		/** Binding indices for VT descriptor set (set=2) */
		enum VTBindingIndex : uint32
		{
			/** usampler2D - Indirection texture (R16G16B16A16_UINT) */
			Indirection = 0,

			/** sampler2D - Physical atlas for BaseColor layer */
			PhysicalAtlas_BaseColor = 1,

			/** sampler2D - Physical atlas for Normal layer */
			PhysicalAtlas_Normal = 2,

			/** sampler2D - Physical atlas for Roughness/Metallic/AO layer */
			PhysicalAtlas_RMA = 3,

			/** sampler2D - Physical atlas for Emissive layer */
			PhysicalAtlas_Emissive = 4,

			/** UBO - VTSpaceParams (VT space parameters) */
			SpaceParamsUBO = 5,

			/** UBO - MaterialParams (shared with traditional layout) */
			MaterialParamsUBO = 6,

			/** Total number of bindings */
			BindingCount = 7
		};
	}

} // namespace Elaine

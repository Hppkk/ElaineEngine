#pragma once

#include "render/vulkan/ElaineVulkanShader.h"

namespace VulkanRHI
{
    // Reflects members of every set=2 uniform buffer in a SPIR-V module.
    // The result is suitable for Material JSON validation and std140 packing.
    ElaineCoreExport bool ReflectMaterialParameters(const SPVCodeDesc& InCode, std::vector<RHIPipeline::MaterialParameterInfo>& OutParameters);
}

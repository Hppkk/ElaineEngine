#include "ElainePrecompiledHeader.h"
#include "render/common/ElaineRHI.h"
#include "render/vulkan/ElaineVulkanRHI.h"

namespace Elaine
{
    DynamicRHI* CreateDynamicRHI(ERHI_TYPE Type)
    {
        switch (Type)
        {
        case Vulkan: return new VulkanRHI::VulkanDynamicRHI();
        default: throw std::runtime_error("The requested RHI backend is not implemented");
        }
    }
}

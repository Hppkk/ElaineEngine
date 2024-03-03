#pragma once
#include "ElaineCorePrerequirements.h"
#include "vulkan_core.h"

namespace Elaine
{
	class ElaineCoreExport VulkanDevice
	{
	public:
		VulkanDevice();
		VkDevice mDevice;
	};
}